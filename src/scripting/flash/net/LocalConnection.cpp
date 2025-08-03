/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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

#include "scripting/abc.h"
#include "scripting/flash/net/LocalConnection.h"
#include "scripting/flash/events/LocalConnectionEvent.h"
#include "scripting/flash/display/RootMovieClip.h"
#include "scripting/argconv.h"

using namespace std;
using namespace lightspark;

LocalConnection::LocalConnection(ASWorker* wrk, Class_base* c):
	EventDispatcher(wrk,c)
	,connectionNameID(UINT32_MAX)
	,connectionstatus(LOCALCONNECTION_CLOSED)
	,client(asAtomHandler::invalidAtom)
{
	subtype=SUBTYPE_LOCALCONNECTION;
}

void LocalConnection::sinit(Class_base* c)
{
	CLASS_SETUP(c, EventDispatcher, _constructor, CLASS_SEALED);
	c->isReusable=true;
	c->setDeclaredMethodByQName("allowDomain","",c->getSystemState()->getBuiltinFunction(allowDomain),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("allowInsecureDomain","",c->getSystemState()->getBuiltinFunction(allowInsecureDomain),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("send","",c->getSystemState()->getBuiltinFunction(send),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("connect","",c->getSystemState()->getBuiltinFunction(connect),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("close","",c->getSystemState()->getBuiltinFunction(close),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("domain","",c->getSystemState()->getBuiltinFunction(domain,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("isSupported","",c->getSystemState()->getBuiltinFunction(isSupported),GETTER_METHOD,false);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,client,ASObject);
}

ASFUNCTIONBODY_ATOM(LocalConnection, isSupported)
{
	ret = asAtomHandler::trueAtom;
}

void LocalConnection::_getter_client(asAtom& ret, ASWorker* wrk, asAtom& obj, asAtom* args, const unsigned int argslen)
{
	if(!asAtomHandler::is<LocalConnection>(obj))
	{
		createError<ArgumentError>(wrk,0,"Function applied to wrong object");
		return;
	}
	LocalConnection* th = asAtomHandler::as<LocalConnection>(obj);
	// empty client means th is the client
	if (asAtomHandler::isInvalid(th->client))
		ret = asAtomHandler::fromObjectNoPrimitive(th);
	else
		ret = th->client;
	ASATOM_INCREF(ret);
}
void LocalConnection::_setter_client(asAtom& ret, ASWorker* wrk, asAtom& obj, asAtom* args, const unsigned int argslen)
{
	if(!asAtomHandler::is<LocalConnection>(obj))
	{
		createError<ArgumentError>(wrk,0,"Function applied to wrong object");
		return;
	}
	if(argslen != 1)
	{
		createError<ArgumentError>(wrk,0,"Arguments provided in setter");
		return;
	}
	if (asAtomHandler::isNull(args[0]))
	{
		createError<TypeError>(wrk,kConvertNullToObjectError);
		return;
	}
	if (asAtomHandler::isUndefined(args[0]))
	{
		createError<TypeError>(wrk,kConvertUndefinedToObjectError);
		return;
	}
	LocalConnection* th = asAtomHandler::as<LocalConnection>(obj);
	if (args[0].uintval == th->client.uintval || (asAtomHandler::getObject(args[0])==th && asAtomHandler::isInvalid(th->client)))
		return;
	ASATOM_REMOVESTOREDMEMBER(th->client);
	if (asAtomHandler::getObject(args[0])!=th)
	{
		th->client = args[0];
		ASATOM_ADDSTOREDMEMBER(th->client);
	}
}


void LocalConnection::finalize()
{
	EventDispatcher::finalize();
	ASATOM_REMOVESTOREDMEMBER(client);
	client = asAtomHandler::invalidAtom;
}

bool LocalConnection::destruct()
{
	connectionNameID=UINT32_MAX;
	ASATOM_REMOVESTOREDMEMBER(client);
	client = asAtomHandler::invalidAtom;
	return EventDispatcher::destruct();
}

bool LocalConnection::countCylicMemberReferences(garbagecollectorstate& gcstate)
{
	bool ret = EventDispatcher::countCylicMemberReferences(gcstate);
	ASObject* o = asAtomHandler::getObject(client);
	if (o)
		ret = o->countAllCylicMemberReferences(gcstate) || ret;
	return ret;
}

void LocalConnection::prepareShutdown()
{
	if (preparedforshutdown)
		return;
	EventDispatcher::prepareShutdown();
	ASObject* o = asAtomHandler::getObject(client);
	if (o)
		o->prepareShutdown();
}

uint32_t LocalConnection::getConnectionName()
{
	Locker l(connectionMutex);
	return connectionNameID;
}

void LocalConnection::setConnectionName(uint32_t newconnectionname)
{
	Locker l(connectionMutex);
	if (newconnectionname == UINT32_MAX)
		connectionstatus = LOCALCONNECTION_CLOSED;
	else
		connectionstatus = LOCALCONNECTION_CONNECTED;
	connectionNameID=newconnectionname;
}

ASFUNCTIONBODY_ATOM(LocalConnection,_constructor)
{
	EventDispatcher::_constructor(ret,wrk,obj, nullptr, 0);
}
ASFUNCTIONBODY_ATOM(LocalConnection, domain)
{
	tiny_string res = wrk->getSystemState()->mainClip->getOrigin().getHostname();
	if (wrk->getSystemState()->flashMode == SystemState::AIR)
		LOG(LOG_NOT_IMPLEMENTED,"LocalConnection::domain is not implemented for AIR mode");
	
	if (res.empty())
		res = "localhost";
	ret = asAtomHandler::fromString(wrk->getSystemState(),res);
}
ASFUNCTIONBODY_ATOM(LocalConnection, allowDomain)
{
	//LocalConnection* th=obj.as<LocalConnection>();
	LOG(LOG_NOT_IMPLEMENTED,"LocalConnection::allowDomain is not implemented");
}
ASFUNCTIONBODY_ATOM(LocalConnection, allowInsecureDomain)
{
	//LocalConnection* th=obj.as<LocalConnection>();
	LOG(LOG_NOT_IMPLEMENTED,"LocalConnection::allowInsecureDomain is not implemented");
}
ASFUNCTIONBODY_ATOM(LocalConnection, send)
{
	LocalConnection* th=asAtomHandler::as<LocalConnection>(obj);
	ret = asAtomHandler::falseAtom; // for AVM1
	asAtom connectionName = asAtomHandler::invalidAtom;
	asAtom methodName = asAtomHandler::invalidAtom;
	ARG_CHECK(ARG_UNPACK_MORE_ALLOWED(connectionName)(methodName));

	if (asAtomHandler::isNull(connectionName))
	{
		if (wrk->needsActionScript3())
			createError<TypeError>(wrk,kNullPointerError, "connectionName");
		return;
	}
	if (asAtomHandler::toStringId(connectionName,wrk)==BUILTIN_STRINGS::EMPTY)
	{
		if (wrk->needsActionScript3())
			createError<ArgumentError>(wrk,kEmptyStringError, "connectionName");
		return;
	}
	if (!asAtomHandler::isString(connectionName) )
	{
		if (wrk->needsActionScript3())
			createError<ArgumentError>(wrk,kInvalidArgumentError, "connectionName");
		return;
	}

	if (asAtomHandler::isNull(methodName))
	{
		if (wrk->needsActionScript3())
			createError<TypeError>(wrk,kNullPointerError, "methodName");
		return;
	}
	if (!asAtomHandler::isString(methodName) )
	{
		if (wrk->needsActionScript3())
			createError<ArgumentError>(wrk,kInvalidArgumentError, "methodName");
		return;
	}
	if (asAtomHandler::toStringId(methodName,wrk)==BUILTIN_STRINGS::EMPTY)
	{
		if (wrk->needsActionScript3())
			createError<ArgumentError>(wrk,kEmptyStringError, "methodName");
		return;
	}
	tiny_string s = asAtomHandler::toString(methodName,wrk);
	if (s.empty()
		|| s=="send"
		|| s=="connect"
		|| s=="close"
		|| s=="allowDomain"
		|| s=="allowInsecureDomain"
		|| s=="domain")
	{
		if (wrk->needsActionScript3())
			createError<ArgumentError>(wrk,kInvalidParamError);
		return;
	}
	uint32_t methodID = asAtomHandler::toStringId(methodName,wrk);
	tiny_string cname = asAtomHandler::toString(connectionName,wrk);
	if(!cname.contains(":") && !cname.startsWith("_"))
	{
		tiny_string fullname = wrk->getSystemState()->mainClip->getOrigin().getHostname();
		if (fullname.empty())
			fullname="localhost";
		fullname += ":";
		fullname += cname;
		cname = fullname;
	}
	uint32_t nameID = wrk->getSystemState()->getUniqueStringId(cname,false);
	uint32_t numargs = argslen-2;
	ret = asAtomHandler::trueAtom; // for AVM1
	getVm(wrk->getSystemState())->addEvent(NullRef,_MR(new (wrk->getSystemState()->unaccountedMemory) LocalConnectionEvent(th,nameID,methodID,args+2,numargs,LOCALCONNECTION_EVENTTYPE::LOCALCONNECTION_SEND)));
}
ASFUNCTIONBODY_ATOM(LocalConnection, connect)
{
	ret = asAtomHandler::falseAtom; // for AVM1
	if (!asAtomHandler::is<LocalConnection>(obj))
		return;
	LocalConnection* th=asAtomHandler::as<LocalConnection>(obj);
	asAtom connectionName = asAtomHandler::invalidAtom;
	ARG_CHECK(ARG_UNPACK(connectionName));
	Locker l(th->connectionMutex);
	if (th->connectionstatus == LOCALCONNECTION_CONNECTED)
	{
		// already connected
		if (!th->is<AVM1LocalConnection>())
			createErrorWithMessage<ArgumentError>(wrk,2082, "Error #2082: Connect failed because the object is already connected.");
		return;
	}
	if (asAtomHandler::isNull(connectionName))
	{
		if (wrk->needsActionScript3())
			createError<TypeError>(wrk,kNullPointerError, "connectionName");
		return;
	}
	if (asAtomHandler::toStringId(connectionName,wrk)==BUILTIN_STRINGS::EMPTY)
	{
		if (wrk->needsActionScript3())
			createError<ArgumentError>(wrk,kEmptyStringError, "connectionName");
		return;
	}
	if (!asAtomHandler::isString(connectionName))
	{
		if (!th->is<AVM1LocalConnection>())
			createError<ArgumentError>(wrk,kInvalidArgumentError, "connectionName");
		return;
	}
	tiny_string s = asAtomHandler::toString(connectionName,wrk);
	if (s.empty() || s.contains(":"))
	{
		if (!th->is<AVM1LocalConnection>())
			createError<ArgumentError>(wrk,kInvalidParamError);
		return;
	}
	tiny_string cname;
	if (s.startsWith("_"))
		cname=s;
	else
	{
		cname = wrk->getSystemState()->mainClip->getOrigin().getHostname();
		if (cname.empty())
			cname="localhost";
		cname += ":";
		cname += s;
	}
	uint32_t newConnectionName = wrk->getSystemState()->getUniqueStringId(cname,false);
	LocalConnection* currentconnection = wrk->getSystemState()->getLocalConnectionClient(newConnectionName);
	if (currentconnection
		&& currentconnection->connectionNameID == newConnectionName)
	{
		if (currentconnection->connectionstatus ==LOCALCONNECTION_CLOSING
			|| currentconnection->connectionstatus ==LOCALCONNECTION_CLOSED)
		{
			wrk->getSystemState()->setLocalConnectionClient(newConnectionName,th);
			th->connectionstatus = LOCALCONNECTION_CONNECTED;
			th->connectionNameID = newConnectionName;
			ret = asAtomHandler::trueAtom; // for AVM1
		}
		else
		{
			if (!th->is<AVM1LocalConnection>())
				createErrorWithMessage<ArgumentError>(wrk,2082, "Error #2082: Connect failed because the object is already connected.");
			return;
		}
	}
	else
	{
		wrk->getSystemState()->setLocalConnectionClient(newConnectionName,th);
		th->connectionstatus = LOCALCONNECTION_CONNECTING;
		th->connectionNameID = newConnectionName;
		getVm(wrk->getSystemState())->addEvent(NullRef,_MR(new (wrk->getSystemState()->unaccountedMemory) LocalConnectionEvent(th,newConnectionName,0,nullptr,0,LOCALCONNECTION_EVENTTYPE::LOCALCONNECTION_CONNECT)));
		ret = asAtomHandler::trueAtom; // for AVM1
	}
}
ASFUNCTIONBODY_ATOM(LocalConnection, close)
{
	if (!asAtomHandler::is<LocalConnection>(obj))
		return;
	LocalConnection* th=asAtomHandler::as<LocalConnection>(obj);
	Locker l(th->connectionMutex);
	if (th->connectionNameID == UINT32_MAX
		|| th->connectionstatus == LOCALCONNECTION_CLOSED
		|| th->connectionstatus == LOCALCONNECTION_CLOSING)
	{
		if (!th->is<AVM1LocalConnection>())
			createErrorWithMessage<ArgumentError>(wrk,2083, "Error #2083: Close failed because the object is not connected.");
		return;
	}
	th->connectionstatus = LOCALCONNECTION_CLOSING;
	getVm(wrk->getSystemState())->addEvent(NullRef,_MR(new (wrk->getSystemState()->unaccountedMemory) LocalConnectionEvent(th,th->connectionNameID,0,nullptr,0,LOCALCONNECTION_EVENTTYPE::LOCALCONNECTION_CLOSE)));
	ret = asAtomHandler::trueAtom; // for AVM1
}
