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

#include "backends/local_connection.h"
#include "backends/urlutils.h"
#include "swf.h"
#include "tiny_string.h"

using namespace lightspark;

// Based off Ruffle's `local_connection` crate.

void LocalConnectionObject::sendStatus(const tiny_string& status)
{
	if (isNull())
		return;

	if (isAVM1())
	{
		AVM1LocalConnection::sendStatus
		(
			sys->getAVM1Ctx(),
			getAVM1Obj()
			status
		);
		return;
	}

	auto obj = getASObj();
	auto wrk = obj->getInstanceWorker();
	obj->incRef();
	getVm(sys)->addEvent
	(
		_MR(obj),
		_MR(Class<StatusEvent>::getInstanceS(wrk, status))
	);
}

void LocalConnectionObject::runMethod
(
	const tiny_string& methodName,
	const Span<AMFValue>& args
)
{
	if (isNull())
		return;

	if (isAVM1())
	{
		AVM1LocalConnection::runMethod
		(
			sys->getAVM1Ctx(),
			getAVM1Obj(),
			methodName,
			args
		);
		return;
	}

	auto nameID = sys->getUniqueStringId(methodName);
	auto obj = getASObj()->client;
	auto wrk = obj->getInstanceWorker();

	auto func = asAtomHandler::invalidAtom;
	multiname m(nullptr);
	m.name_type = multiname::NAME_STRING;
	m.name_s_id = nameID;
	m.isAttribute = false;

	obj->getVariableByMultiname
	(
		func,
		m,
		GET_VARIABLE_OPTION::NONE,
		wrk
	);

	if (asAtomHandler::isFunction(func))
	{
		obj->incRef();
		auto unaccMem = sys->unaccountedMemory;
		auto _obj = asAtomHandler::fromObject(obj);
		getVm(sys)->addEvent
		(
			NullRef
			_MR(new (unaccMem) FunctionAsyncEvent
			(
				func,
				_obj,
				args
			))
		);
		return;
	}

	if (!obj->is<EventDispatcher>())
		return;

	obj->incRef();
	auto _obj = obj->as<EventDispatcher>();
	auto e = Class<AsyncErrorEvent>::getInstanceS(wrk);
	e->error = Class<ReferenceError>::getInstanceS
	(
		wrk,
		createErrorMessage
		(
			kReadSealedError,
			nameID,
			obj->getClass()->getName(true),
			""
		),
		kReadSealedError
	);

	getVm(sys)->addEvent(_MR(_obj), _MR(e));
}

LocalConnectionManager::LocalConnectionManager
(
	SystemState* _sys
) : sys(_sys), thread(this, "LocalConnectionManager")
{
}

LocalConnectionManager::~LocalConnectionManager()
{
	stop();
}

void LocalConnectionManager::stop()
{
	thread.stop(msgCond);
}

void LocalConnectionManager::wait()
{
	thread.wait(msgCond);
}

Optional<const LCObj&> LocalConnectionManager::getConnection
(
	const tiny_string& name
) const
{
	Locker l(connectionMutex);
	return getConnectionNoLock(name);
}

Optional<const LCObj&> LocalConnectionManager::getConnectionNoLock
(
	const tiny_string& name
) const
{
	auto it = connections.find(name);
	if (it == connections.end())
		return {};
	return makeOptionalRef(it->second);
}

Optional<LCMessage> LocalConnectionManager::getMsg()
{
	Locker l(msgMutex);
	while (messages.empty())
	{
		msgCond.wait(msgMutex);
		if (thread.isStopped())
			return {};
	}
	auto msg = messages.front();
	messages.pop_front();
	return msg;
}

int LocalConnectionManager::worker()
{
	for (;;)
	{
		auto msg = getMsg();
		if (!msg.hasValue())
			break;

		auto _msg = msg->getMsg();
		if (!_msg.hasValue())
		{
			_msg->getObj().sendStatus("error");
			continue;
		}

		auto receiver = getConnection(_msg->name);
		if (!receiver.hasValue())
		{
			msg.getObj().sendStatus("error");
			continue;
		}

		msg->getObj().sendStatus("status");
		receiver->runMethod(_msg->methodName, _msg->args);
	}

	return 0;
}

void LocalConnectionManager::connect
(
	const URLInfo& url,
	const LCObj& obj,
	const tiny_string& name
)
{
	auto key = name.startsWith('_') ? name.lowercase() :
	(
		getSuperdomain(url) +
		':' +
		name
	).lowercase();

	Locker l(connectionMutex);
	if (getConnectionNoLock(key).hasValue())
		return;
	connections.emplace(key, obj);
}

void LocalConnectionManager::send
(
	const URLInfo& url,
	const LCObj& obj,
	const tiny_string& name,
	const tiny_string& methodName,
	const Span<AMFValue>& args
)
{
	auto _name = name.lowercase();
	if (!_name.contains(':') && !_name.startsWith('_'))
		_name = getSuperdomain(url) + ':' + _name;

	Locker l(msgMutex);
	if (getListener(_name).hasValue())
		messages.emplace_back(obj, _name, methodName, args);
	else
		messages.emplace_back(obj);
	msgCond.signal();
}

void LocalConnectionManager::disconnect(const tiny_string& name)
{
	Locker l(connectionMutex);
	connections.erase(name);
}

tiny_string LocalConnectionManager::getSuperdomain(const URLInfo& url)
{
	const auto& hostname = url.getHostname();
	auto pos = hostname.rfind('.');
	return
	(
		pos != tiny_string::npos ?
		hostname.substr(pos, tiny_string::npos) :
		url.getParsedURL()
	);
}

tiny_string LocalConnectionManager::getDomain(const URLInfo& url)
{
	if (!url.isValid())
	{
		LOG
		(
			LOG_ERROR,
			"`LocalConnectionManager::getDomain()`: "
			"URL \"" << url << "\" is invalid."
		);
		return "unknown";
	}

	return
	(
		url.getProtocol() == "file" ||
		// NOTE: An empty domain/hostname is treated as `localhost`.
		url.getHostname().empty()
	) ? "localhost" : url.getHostname();
}
