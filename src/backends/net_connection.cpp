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
	const AMF0Value& msg,
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
	message = msg;
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
		AMFPacket(message).toBytes(),
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
		this->incRef();
		getVm(getSystemState())->addEvent(_MR(this),_MR(Class<IOErrorEvent>::getInstanceS(getInstanceWorker())));
		removeDownloader();
		return;
	}

	auto sbuf = cache->createReader();
	std::istream s(sbuf);

	std::vector<uint8_t> msg(downloader->getLength());
	s.read(msg.data(), msg.size());
	//Download is done, destroy it
	delete sbuf;
	removeDownloader();

	auto event = _MR(new (sys->unaccountedMemory) ParseRPCMessageEvent
	(
		message,
		client,
		responder
	));

	getVm(sys)->addEvent(NullRef, event);
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
	decRef();
}

void NetConnection::connect(const URLInfo& url)
{
	auto sys = getSys();
	//This seems strange:
	//LOCAL_WITH_FILE may not use connect(), even if it tries to connect to a local file.
	//I'm following the specification to the letter. Testing showed
	//that the official player allows connect(null) in localWithFile.
	if
	(
		url.isValid() &&
		sys->securityManager->evaluateSandbox
		(
			SecurityManager::LOCAL_WITH_FILE
		)
	)
	{
		createError<SecurityError>(wrk,0,"SecurityError: NetConnection::connect "
				"from LOCAL_WITH_FILE sandbox");
		return;
	}

	bool isNull = false;
	bool isRTMP = false;
	//bool isRPC = false;

	_connected = false;
	//Null argument means local file or web server, the spec only mentions NULL, but youtube uses UNDEFINED, so supporting that too.
	if (url.isEmpty())
	{
		incRef();
		getVm(sys)->addEvent
		(
			_MR(this),
			_MR(Class<NetStatusEvent>::getInstanceS
			(
				wrk,
				"status",
				"NetConnection.Connect.Success"
			))
		);
		return;
	}

	//String argument means Flash Remoting/Flash Media Server
	uri = url;

	if (sys->securityManager->evaluatePoliciesURL(uri, true) != SecurityManager::ALLOWED)
	{
		//TODO: find correct way of handling this case
		createError<SecurityError>(wrk,0,"SecurityError: connection to domain not allowed by securityManager");
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
	incRef();
	getVm(sys)->addEvent
	(
		_MR(this),
		_MR(Class<NetStatusEvent>::getInstanceS
		(
			wrk,
			"status",
			"NetConnection.Connect.Success"
		))
	);
}
void NetConnection::afterExecution(_R<Event> ev)
{
	if (ev->is<NetStatusEvent>())
	{
		// it seems that the connected flag should only be set after the NetConnection.Connect.Success event is handled
		if (ev->as<NetStatusEvent>()->statuscode == "NetConnection.Connect.Success")
			this->_connected = true;
	}
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
