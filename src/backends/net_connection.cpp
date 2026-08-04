/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2013  Alessandro Pignotti (a.pignotti@sssup.it)
    Copyright (C) 2026  mr b0nk 500 (b0nk@b0nk.xyz)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#include <atomic>
#include <sstream>

#include "backends/net_connection.h"
#include "backends/security.h"
#include "platforms/engineutils.h"

using namespace lightspark;

// this is a global counter to produce uinque IDs for NetConnections
// TODO maybe it would be better to use some form of GUID
std::atomic<uint64_t> nearIDcounter(0);

NetConnection::NetConnection() :
_connected(false),
downloader(nullptr),
messageCount(0),
objectEncoding(getDefaultObjectEncoding()),
proxyType(PT_NONE),
maxPeerConnections(8),
nearID((std::stringstream() << "nearID" << ++nearIDCounter).str())
{
}

void NetConnection::call
(
	const tiny_string& cmd,
	const AMFValue& msg,
	const ResponderObject& _responder
)
{
	//Arguments are:
	//1) A string for the command
	//2) A Responder instance (optional)
	//And other arguments to be passed to the server

	++messageCount;

	if (!uri.isValid())
		return;

	if (uri.isRTMP())
	{
		LOG
		(
			LOG_NOT_IMPLEMENTED,
			"`NetConnection::call()`: RTMP not yet supported."
		);
		return;
	}

	responder = _responder;
	message = AMFMessage
	(
		cmd,
		(std::stringstream() << '/' << messageCount).str(),
		msg
	);

	auto obj = getASObj();
	if (!obj.isNull())
	{
		//To be decreffed in jobFence
		obj->incRef();
	}

	getSys()->addJob(this);
}

void NetConnection::execute()
{
	LOG(LOG_CALLS, "NetConnection async execution " << uri);
	assert(message.hasValue());

	auto sys = getSys();
	auto removeDownloader = [&]
	{
		//Acquire the lock to ensure consistency in threadAbort
		Locker l(downloaderLock);
		sys->downloadManager->destroy(downloader);
		downloader = nullptr;
	}

	auto cache = _MR(new MemoryStreamCache(sys));
	downloader = sys->downloadManager->downloadWithData
	(
		uri,
		cache,
		message->toBytes(),
		{ "Content-Type: application/x-amf" },
		nullptr
	);

	//Get the whole answer
	cache->waitForTermination();
	if (cache->hasFailed()) //Check to see if the download failed for some reason
	{
		LOG
		(
			LOG_ERROR,
			"`NetConnection::execute()`: "
			"Download of URL failed: " << uri
		);

		if (!getAVM1Obj().isNull())
		{
			handleAVM1StatusEvent({});
			return;
		}

		auto obj = getASObj();
		if (obj.isNull())
			return;

		auto wrk = obj->getInstanceWorker();
		handleAVM2StatusEvent
		({
			{ "code", "NetConnection.Call.Failed" },
			{ "level", "error" }
		});

		obj->incRef();
		getVm(sys)->addEvent
		(
			_MR(obj),
			_MR(Class<IOErrorEvent>::getInstanceS(wrk))
		);
		removeDownloader();
		return;
	}

	auto sbuf = cache->createReader();
	std::istream s(sbuf);

	std::vector<uint8_t> msgData(downloader->getLength());
	s.read(msgData.data(), msgData.size());
	//Download is done, destroy it
	delete sbuf;
	removeDownloader();

	AMFPacket responsePkt(makeSpan(msgData));
	if (!responsePkt.isValid())
	{
		responder.reset();
		return;
	}

	for (const auto& msg : responsePkt.getMessages())
	{
		const auto& targetURI = msg.getTargetURI();
		auto _targetURI = targetURI.tryStripPrefix('/');
		if (!_targetURI.hasValue())
			continue;

		bool isStatus = false;
		auto idx = _targetURI->tryStripSuffix
		(
			"/onResult"
		).orElse([&]
		{
			auto ret = _targetURI->tryStripSuffix("/onStatus");
			isStatus = ret.hasValue();
			return ret;
		}).andThen([&](const auto& str)
		{
			return str.tryToNumber<size_t>();
		});

		if (!idx.hasValue())
			continue;

		auto avm1Obj = responder.getAVM1Obj();
		if (!avm1Obj.isNull())
		{
			avm1Obj->sendCallback
			(
				sys,
				isStatus,
				msg.getValue()
			);
			continue;
		}

		auto asObj = responder.getASObj();
		if (asObj.isNull())
			continue;

		asObj->incRef();
		auto unaccountedMem = sys->unaccountedMemory;
		auto ev = _MR(new (unaccountedMem) RPCMessageEvent
		(
			msg.getValue(),
			client,
			asObj
		));

		getVm(sys)->addEvent(NullRef, ev);
	}

	responder.reset();
}

void NetConnection::threadAbort()
{
	//We have to stop the downloader
	Locker l(downloaderLock);
	if (downloader != nullptr)
		downloader->stop();
}

void NetConnection::jobFence()
{
	auto obj = getASObj();
	if (!obj.isNull())
		obj->decRef();
}

void NetConnection::connect()
{
	//Null argument means local file or web server, the spec only mentions NULL, but youtube uses UNDEFINED, so supporting that too.
	_connected = false;
	handleStatusEvent
	({
		{ "level", "status" },
		{ "code", "NetConnection.Connect.Success" }
	});

	if (!getAVM1Obj().isNull())
		_connected = true;
}

void NetConnection::connect(const URLInfo& url)
{
	auto sys = getSys();
	auto secMgr = sys->securityManager;
	//This seems strange:
	//LOCAL_WITH_FILE may not use connect(), even if it tries to connect to a local file.
	//I'm following the specification to the letter. Testing showed
	//that the official player allows connect(null) in localWithFile.
	if
	(
		url.isValid() &&
		secMgr->evaluateSandbox(SecurityManager::LOCAL_WITH_FILE)
	)
	{
		handleSecurityError
		(
			"NetConnection.Connect.Failed",
			"`NetConnection::connect()`: "
			"Tried to connect to " + url + " from a "
			"LOCAL_WITH_FILE sandbox."
		);
		return;
	}

	bool isRTMP = false;
	//bool isRPC = false;

	_connected = false;

	//String argument means Flash Remoting/Flash Media Server
	uri = url;

	if
	(
		secMgr->evaluatePoliciesURL(uri, true) !=
		SecurityManager::ALLOWED
	)
	{
		//TODO: find correct way of handling this case
		handleSecurityError
		(
			"NetConnection.Connect.Failed",
			"Connection to domain not allowed by securityManager"
		);
		return;
	}

	//By spec NetConnection::connect is true for RTMP and remoting and false otherwise
	if (uri.isRTMP())
	{
		isRTMP = true;
		// it seems that the connected flag should only be set after the NetConnection.Connect.Success event is handled
		//th->_connected = true;
	}
	else if
	(
		uri.getProtocol() == "http" ||
		uri.getProtocol() == "https"
	)
	{
		// it seems that the connected flag should only be set after the NetConnection.Connect.Success event is handled
		//th->_connected = true;
		//isRPC = true;
	}
	else
	{
		LOG
		(
			LOG_ERROR,
			"`NetConnection::connect()`: "
			"Unsupported protocol " << uri.getProtocol()
		);
		throw UnsupportedException
		(
			"`NetConnection::connect()`: "
			"protocol not supported"
		);
	}

	// We actually create the connection later in
	// NetStream::play() or NetConnection.call()

	if (!isRTMP)
		return;
	//When the URI is undefined the connect is successful (tested on Adobe player)
	handleStatusEvent
	({
		{ "level", "status" },
		{ "code", "NetConnection.Connect.Success" }
	});

	if (!getAVM1Obj().isNull())
		_connected = true;
}

void NetConnection::handleStatusEvent(Span<const KVPair> values)
{
	if (!getAVM1Obj().isNull())
		handleAVM1StatusEvent(values);
	else if (!getASObj().isNull())
		handleAVM2StatusEvent(values);
}

void NetConnection::handleSecurityError
(
	const tiny_string& errorCode,
	const tiny_string& reason
)
{
	handleStatusEvent
	({
		{ "level", "error" },
		{ "code", errorCode }
	});

	auto obj = getASObj();
	if (obj.isNull())
		return;

	createError<SecurityError>
	(
		obj->getInstanceWorker(),
		0,
		"SecurityError: " + reason
	);
}

void NetConnection::handleAVM1StatusEvent(Span<const KVPair> values)
{
	auto obj = getAVM1Obj();
	assert(!obj.isNull());

	AVM1Activation act
	(
		getSys(),
		"[NetConnection Status Event]"
		getSys()->stage->mainClip
	);

	auto infoObj = NEW_GC_PTR(act.getGcCtx(), AVM1Object
	(
		act.getGcCtx(),
		act.getPrototypes()->object->proto
	));

	for (const auto& pair : values)
		infoObj->setProp(act, pair.first, pair.second);

	try
	{
		(void)obj->callMethod
		(
			act,
			"onStatus",
			{ infoObj },
			AVM1ExecutionReason::Special
		);
	}
	catch (std::exception& e)
	{
		LOG
		(
			LOG_ERROR,
			"Got error while handling an AVM1 `onStatus` "
			"handler from a `NetConnection`. "
			"Reason: " << e.what()
		);
	}
}

void NetConnection::handleAVM2StatusEvent(Span<const KVPair> values)
{
	auto tryGetVal = [&](const tiny_string& name)
	{
		auto it = std::find_if
		(
			values.begin(),
			values.end(),
			[&](const auto& pair)
			{
				return pair.first == name;
			}
		);
		return it != values.end() ? it->second : "";
	};

	auto obj = getASObj();
	assert(!obj.isNull());

	obj->incRef();
	getVm(getSys())->addEvent
	(
		_MR(obj),
		_MR(Class<NetStatusEvent>::getInstanceS
		(
			obj->getInstanceWorker(),
			tryGetVal("level"),
			tryGetVal("status")
		))
	);
}

const tiny_string& NetConnection::getConnectedProxyType() const
{
	return _connected ? "none" : "";
}

const OBJECT_ENCODING& NetConnection::getDefaultObjectEncoding()
{
	return getSys()->staticNetConnectionDefaultObjectEncoding;
}

void NetConnection::setDefaultObjectEncoding(const OBJECT_ENCODING& encoding)
{
	getSys()->staticNetConnectionDefaultObjectEncoding = encoding;
}

void NetConnection::close()
{
	if (!_connected)
		return;
	threadAbort();
	_connected = false;
}
