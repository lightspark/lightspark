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

#ifndef SCRIPTING_AVM1_AVM1NET_H
#define SCRIPTING_AVM1_AVM1NET_H

#include "asobject.h"
#include "scripting/flash/net/flashnet.h"
#include "scripting/flash/net/LocalConnection.h"
#include "scripting/flash/net/XMLSocket.h"

namespace lightspark
{

class AVM1SharedObject: public SharedObject
{
public:
	AVM1SharedObject(ASWorker* wrk,Class_base* c):SharedObject(wrk,c){}
	static void sinit(Class_base* c);
};

class AVM1LocalConnection: public LocalConnection
{
public:
	AVM1LocalConnection(ASWorker* wrk,Class_base* c):LocalConnection(wrk,c){}
	static void sinit(Class_base* c);
};

class AVM1LoadVars: public URLVariables
{
	_NR<URLLoader> loader;
public:
	AVM1LoadVars(ASWorker* wrk,Class_base* c):URLVariables(wrk,c){}
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(sendAndLoad);
	ASFUNCTION_ATOM(load);
	bool destruct() override;
	multiname* setVariableByMultiname(multiname& name, asAtom& o, CONST_ALLOWED_FLAG allowConst, bool* alreadyset, ASWorker* wrk) override;
	void AVM1HandleEvent(EventDispatcher* dispatcher, Event* e) override;
};
class AVM1NetConnection: public NetConnection
{
public:
	AVM1NetConnection(ASWorker* wrk,Class_base* c):NetConnection(wrk,c){}
	static void sinit(Class_base* c);
};

class AVM1XMLSocket: public XMLSocket
{
public:
	AVM1XMLSocket(ASWorker* wrk,Class_base* c):XMLSocket(wrk,c){}
	static void sinit(Class_base* c);
};

class AVM1NetStream: public NetStream
{
public:
	AVM1NetStream(ASWorker* wrk,Class_base* c):NetStream(wrk,c){}
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(avm1pause);
	void AVM1HandleEvent(EventDispatcher *dispatcher, Event *e) override;
};

}
#endif // SCRIPTING_AVM1_AVM1NET_H
