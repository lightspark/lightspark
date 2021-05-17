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
	CLASS_SETUP(c, URLVariables, _constructor, CLASS_DYNAMIC_NOT_FINAL);
	c->setDeclaredMethodByQName("sendAndLoad","",Class<IFunction>::getFunction(c->getSystemState(),sendAndLoad),NORMAL_METHOD,true);
}
ASFUNCTIONBODY_ATOM(AVM1LoadVars,_constructor)
{
}
ASFUNCTIONBODY_ATOM(AVM1LoadVars,sendAndLoad)
{
	AVM1LoadVars* th = asAtomHandler::as<AVM1LoadVars>(obj);
	tiny_string strurl;
	tiny_string method;
	_NR<ASObject> target;
	ARG_UNPACK_ATOM (strurl)(target)(method,"POST");

	if (target)
	{
		if (target->is<AVM1LoadVars>())
		{
			AVM1LoadVars* t = target->as<AVM1LoadVars>();
			th->copyValues(t);
			if (t->loader.isNull())
				t->loader = _MR(Class<URLLoader>::getInstanceS(sys));
			t->incRef();
			URLRequest* req = Class<URLRequest>::getInstanceS(sys,strurl,method,_MR(t));
			asAtom urlarg = asAtomHandler::fromObjectNoPrimitive(req);
			asAtom loaderobj = asAtomHandler::fromObjectNoPrimitive(t->loader.getPtr());
			URLLoader::load(ret,sys,loaderobj,&urlarg,1);
		}
		else
			LOG(LOG_NOT_IMPLEMENTED,"LoadVars.sendAndLoad with target "<<target->toDebugString());
	}
	else
		LOG(LOG_ERROR,"LoadVars.sendAndLoad called without target");
}
multiname* AVM1LoadVars::setVariableByMultiname(multiname& name, asAtom& o, CONST_ALLOWED_FLAG allowConst, bool *alreadyset)
{
	multiname* res = URLVariables::setVariableByMultiname(name,o,allowConst,alreadyset);
	if (name.name_s_id == BUILTIN_STRINGS::STRING_ONLOAD)
	{
		this->incRef();
		getSystemState()->stage->AVM1AddEventListener(this);
		setIsEnumerable(name, false);
	}
	return res;
}

void AVM1LoadVars::AVM1HandleEvent(EventDispatcher *dispatcher, Event* e)
{
	if (dispatcher == loader.getPtr())
	{
		if (e->type == "complete" || e->type == "ioError")
		{
			asAtom func=asAtomHandler::invalidAtom;
			multiname m(nullptr);
			m.name_type=multiname::NAME_STRING;
			m.isAttribute = false;
			m.name_s_id=BUILTIN_STRINGS::STRING_ONLOAD;
			getVariableByMultiname(func,m);
			if (asAtomHandler::is<AVM1Function>(func))
			{
				asAtom ret=asAtomHandler::invalidAtom;
				asAtom obj = asAtomHandler::fromObject(this);
				asAtom args[1];
				args[0] = e->type == "complete" ? asAtomHandler::trueAtom : asAtomHandler::falseAtom;
				asAtomHandler::as<AVM1Function>(func)->call(&ret,&obj,args,1);
			}
		}
	}
}
void AVM1NetConnection::sinit(Class_base *c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_DYNAMIC_NOT_FINAL);
	c->setDeclaredMethodByQName("connect","",Class<IFunction>::getFunction(c->getSystemState(),connect),NORMAL_METHOD,true);
}

