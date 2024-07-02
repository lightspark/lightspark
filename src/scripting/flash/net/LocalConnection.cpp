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
#include "scripting/flash/display/RootMovieClip.h"
#include "scripting/argconv.h"

using namespace std;
using namespace lightspark;

LocalConnection::LocalConnection(ASWorker* wrk, Class_base* c):
	EventDispatcher(wrk,c),connectionNameID(UINT32_MAX),isSupported(true)
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
	REGISTER_GETTER_RESULTTYPE(c,isSupported,Boolean);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,client,ASObject);
}

ASFUNCTIONBODY_GETTER(LocalConnection, isSupported)

void LocalConnection::_getter_client(asAtom& ret, ASWorker* wrk, asAtom& obj, asAtom* args, const unsigned int argslen)
{
	if(!asAtomHandler::is<LocalConnection>(obj))
	{
		createError<ArgumentError>(wrk,0,"Function applied to wrong object");
		return;
	}
	LocalConnection* th = asAtomHandler::as<LocalConnection>(obj);
	// empty client means th is the client
	if (th->client.isNull())
		ret = asAtomHandler::fromObjectNoPrimitive(th);
	else
		ret = asAtomHandler::fromObjectNoPrimitive(th->client.getPtr());
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
	ASObject* o = asAtomHandler::toObject(args[0],wrk);
	if (o == th->client.getPtr() || (o==th && th->client.isNull()))
		return;
	if (th->client)
	{
		th->client->removeStoredMember();
		th->client.fakeRelease();
	}
	_NR<ASObject> cl;
	if (o != th)
	{
		o->incRef();
		o->addStoredMember();
		th->client = _MR(o);
		cl = th->client;
	}
	else
	{
		th->incRef();
		cl = _MR(th);
	}
	wrk->getSystemState()->setLocalConnectionClient(th->connectionNameID,cl);
}


void LocalConnection::finalize()
{
	EventDispatcher::finalize();
	if (client)
	{
		client->removeStoredMember();
		client.fakeRelease();
	}
}

bool LocalConnection::destruct()
{
	connectionNameID=UINT32_MAX;
	isSupported=true;
	if (client)
	{
		client->removeStoredMember();
		client.fakeRelease();
	}
	return EventDispatcher::destruct();
}

bool LocalConnection::countCylicMemberReferences(garbagecollectorstate& gcstate)
{
	if (gcstate.checkAncestors(this))
		return false;
	bool ret = EventDispatcher::countCylicMemberReferences(gcstate);
	if (client)
		ret = client->countAllCylicMemberReferences(gcstate) || ret;
	return ret;
}

void LocalConnection::prepareShutdown()
{
	if (preparedforshutdown)
		return;
	EventDispatcher::prepareShutdown();
	if (client)
		client->prepareShutdown();
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
	//LocalConnection* th=asAtomHandler::as<LocalConnection>(obj);
	asAtom connectionName = asAtomHandler::invalidAtom;
	asAtom methodName = asAtomHandler::invalidAtom;
	ARG_CHECK(ARG_UNPACK_MORE_ALLOWED(connectionName)(methodName));
	uint32_t nameID = asAtomHandler::toStringId(connectionName,wrk);
	uint32_t methodID = asAtomHandler::toStringId(methodName,wrk);
	if (nameID==BUILTIN_STRINGS::EMPTY || methodID==BUILTIN_STRINGS::EMPTY)
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError, "value of either connectionName or methodName is an empty string");
		return;
	}
	uint32_t numargs = argslen-2;
	
	getVm(wrk->getSystemState())->addEvent(NullRef,_MR(new (wrk->getSystemState()->unaccountedMemory) LocalConnectionEvent(nameID,methodID,args+2,numargs)));
}
ASFUNCTIONBODY_ATOM(LocalConnection, connect)
{
	LocalConnection* th=asAtomHandler::as<LocalConnection>(obj);
	asAtom connectionName = asAtomHandler::invalidAtom;
	ARG_CHECK(ARG_UNPACK(connectionName));
	th->connectionNameID = asAtomHandler::toStringId(connectionName,wrk);
	
	_NR<ASObject> cl = th->client;
	if (cl.isNull()) // th is the client
	{
		th->incRef();
		cl = _MR(th);
	}
	wrk->getSystemState()->setLocalConnectionClient(th->connectionNameID,cl);
}
ASFUNCTIONBODY_ATOM(LocalConnection, close)
{
	LocalConnection* th=asAtomHandler::as<LocalConnection>(obj);
	if (th->connectionNameID==UINT32_MAX)
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError, "LocalConnection is not connected");
		return;
	}
	wrk->getSystemState()->removeLocalConnectionClient(th->connectionNameID);
	th->connectionNameID=UINT32_MAX;
}
