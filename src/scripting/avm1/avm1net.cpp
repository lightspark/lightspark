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

#include "scripting/avm1/avm1net.h"
#include "scripting/class.h"
#include "scripting/argconv.h"

using namespace std;
using namespace lightspark;

void AVM1SharedObject::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, EventDispatcher, CLASS_SEALED);
	c->setDeclaredMethodByQName("getLocal","",Class<IFunction>::getFunction(c->getSystemState(),getLocal),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("size","",Class<IFunction>::getFunction(c->getSystemState(),_getSize),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("flush","",Class<IFunction>::getFunction(c->getSystemState(),flush),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("clear","",Class<IFunction>::getFunction(c->getSystemState(),clear),NORMAL_METHOD,true);
	REGISTER_GETTER(c,data);
}

void AVM1LocalConnection::sinit(Class_base *c)
{
	CLASS_SETUP(c, EventDispatcher, _constructor, CLASS_DYNAMIC_NOT_FINAL);
	c->setDeclaredMethodByQName("allowDomain","",Class<IFunction>::getFunction(c->getSystemState(),allowDomain),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("allowInsecureDomain","",Class<IFunction>::getFunction(c->getSystemState(),allowInsecureDomain),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("send","",Class<IFunction>::getFunction(c->getSystemState(),send),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("connect","",Class<IFunction>::getFunction(c->getSystemState(),connect),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("close","",Class<IFunction>::getFunction(c->getSystemState(),close),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("domain","",Class<IFunction>::getFunction(c->getSystemState(),domain),GETTER_METHOD,true);
}
void AVM1LoadVars::sinit(Class_base *c)
{
	CLASS_SETUP(c, EventDispatcher, _constructor, CLASS_DYNAMIC_NOT_FINAL);
	c->setDeclaredMethodByQName("sendAndLoad","",Class<IFunction>::getFunction(c->getSystemState(),sendAndLoad),NORMAL_METHOD,true);
}
ASFUNCTIONBODY_ATOM(AVM1LoadVars,_constructor)
{
}
ASFUNCTIONBODY_ATOM(AVM1LoadVars,sendAndLoad)
{
	AVM1LoadVars* th = asAtomHandler::as<AVM1LoadVars>(obj);
	tiny_string strurl;
	_NR<ASObject> target;
	tiny_string method;
	ARG_UNPACK_ATOM (strurl)(target)(method,"POST");
	LOG(LOG_NOT_IMPLEMENTED,"LoadVars.sendAndLoad does only call onLoad with parameter false(unsuccessful) "<<strurl);

	if (target)
	{
		// TODO generate a loader and realy start a request to the provided url
		asAtom func=asAtomHandler::invalidAtom;
		multiname m(nullptr);
		m.name_type=multiname::NAME_STRING;
		m.isAttribute = false;
		m.name_s_id=BUILTIN_STRINGS::STRING_ONLOAD;
		target->getVariableByMultiname(func,m);
		if (asAtomHandler::is<AVM1Function>(func))
		{
			asAtom ret=asAtomHandler::invalidAtom;
			asAtom obj = asAtomHandler::fromObject(th);
			
			asAtom args[1];
			args[0] = asAtomHandler::falseAtom;
			Log::setLogLevel(LOG_CALLS);
			asAtomHandler::as<AVM1Function>(func)->call(&ret,&obj,args,1);
			Log::setLogLevel(LOG_INFO);
		}
	}
}

void AVM1NetConnection::sinit(Class_base *c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_DYNAMIC_NOT_FINAL);
	c->setDeclaredMethodByQName("connect","",Class<IFunction>::getFunction(c->getSystemState(),connect),NORMAL_METHOD,true);
}

