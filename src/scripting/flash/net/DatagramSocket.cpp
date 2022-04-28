/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2011-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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

#include "DatagramSocket.h"
#include "class.h"
#include "argconv.h"
#include "scripting/toplevel/Integer.h"

using namespace lightspark;

DatagramSocket::DatagramSocket(ASWorker* wrk, Class_base* c) : EventDispatcher(wrk,c),bound(false),connected(false),isSupported(false),localPort(0),remotePort(0)
{
	subtype = SUBTYPE_DATAGRAMSOCKET;
}
void DatagramSocket::sinit(Class_base* c)
{
	CLASS_SETUP(c, EventDispatcher, _constructor, CLASS_SEALED);
	c->setDeclaredMethodByQName("close","",Class<IFunction>::getFunction(c->getSystemState(),_close),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("connect","",Class<IFunction>::getFunction(c->getSystemState(),_connect),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("bind","",Class<IFunction>::getFunction(c->getSystemState(),_bind),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("receive","",Class<IFunction>::getFunction(c->getSystemState(),_receive),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("send","",Class<IFunction>::getFunction(c->getSystemState(),_send),NORMAL_METHOD,true);
	REGISTER_GETTER_RESULTTYPE(c,bound,Boolean);
	REGISTER_GETTER_RESULTTYPE(c,connected,Boolean);
	REGISTER_GETTER_STATIC_RESULTTYPE(c,isSupported,Boolean);
	REGISTER_GETTER_RESULTTYPE(c,localAddress,ASString);
	REGISTER_GETTER_RESULTTYPE(c,localPort,Integer);
	REGISTER_GETTER_RESULTTYPE(c,remoteAddress,ASString);
	REGISTER_GETTER_RESULTTYPE(c,remotePort,Integer);
}
ASFUNCTIONBODY_GETTER_NOT_IMPLEMENTED(DatagramSocket,bound)
ASFUNCTIONBODY_GETTER_NOT_IMPLEMENTED(DatagramSocket,connected)
ASFUNCTIONBODY_GETTER_NOT_IMPLEMENTED(DatagramSocket,isSupported)
ASFUNCTIONBODY_GETTER_NOT_IMPLEMENTED(DatagramSocket,localAddress)
ASFUNCTIONBODY_GETTER_NOT_IMPLEMENTED(DatagramSocket,localPort)
ASFUNCTIONBODY_GETTER_NOT_IMPLEMENTED(DatagramSocket,remoteAddress)
ASFUNCTIONBODY_GETTER_NOT_IMPLEMENTED(DatagramSocket,remotePort)

ASFUNCTIONBODY_ATOM(DatagramSocket,_constructor)
{
	LOG(LOG_NOT_IMPLEMENTED,"class DatagramSocket is a stub");
	EventDispatcher::_constructor(ret,wrk,obj, nullptr, 0);
}
ASFUNCTIONBODY_ATOM(DatagramSocket,_close)
{
	LOG(LOG_NOT_IMPLEMENTED,"DatagramSocket.close does nothing");
}
ASFUNCTIONBODY_ATOM(DatagramSocket,_connect)
{
	LOG(LOG_NOT_IMPLEMENTED,"DatagramSocket.connect does nothing");
}
ASFUNCTIONBODY_ATOM(DatagramSocket,_bind)
{
	LOG(LOG_NOT_IMPLEMENTED,"DatagramSocket.bind does nothing");
}
ASFUNCTIONBODY_ATOM(DatagramSocket,_receive)
{
	LOG(LOG_NOT_IMPLEMENTED,"DatagramSocket.receive does nothing");
}
ASFUNCTIONBODY_ATOM(DatagramSocket,_send)
{
	LOG(LOG_NOT_IMPLEMENTED,"DatagramSocket.send does nothing");
}

