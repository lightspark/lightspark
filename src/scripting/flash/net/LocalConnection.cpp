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
	c->setDeclaredMethodByQName("allowDomain","",Class<IFunction>::getFunction(c->getSystemState(),allowDomain),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("allowInsecureDomain","",Class<IFunction>::getFunction(c->getSystemState(),allowInsecureDomain),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("send","",Class<IFunction>::getFunction(c->getSystemState(),send),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("connect","",Class<IFunction>::getFunction(c->getSystemState(),connect),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("close","",Class<IFunction>::getFunction(c->getSystemState(),close),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("domain","",Class<IFunction>::getFunction(c->getSystemState(),domain,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	REGISTER_GETTER_RESULTTYPE(c,isSupported,Boolean);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,client,ASObject);
}
ASFUNCTIONBODY_GETTER(LocalConnection, isSupported)
ASFUNCTIONBODY_GETTER_SETTER_CB(LocalConnection, client,setclient_cb)

void LocalConnection::setclient_cb(_NR<ASObject> /*oldValue*/)
{
	getSystemState()->setLocalConnectionClient(this->connectionNameID,this->client);
}

ASFUNCTIONBODY_ATOM(LocalConnection,_constructor)
{
	EventDispatcher::_constructor(ret,wrk,obj, nullptr, 0);
	LocalConnection* th=Class<LocalConnection>::cast(asAtomHandler::getObject(obj));
	th->incRef();
	th->client = _NR<LocalConnection>(th);
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
	wrk->getSystemState()->setLocalConnectionClient(th->connectionNameID,th->client);
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
