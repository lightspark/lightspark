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
	c->setDeclaredMethodByQName("load","",Class<IFunction>::getFunction(c->getSystemState(),load),NORMAL_METHOD,true);
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
	ARG_CHECK(ARG_UNPACK(strurl)(target)(method,"POST"));

	if (target)
	{
		if (target->is<AVM1LoadVars>())
		{
			AVM1LoadVars* t = target->as<AVM1LoadVars>();
			th->copyValues(t,wrk);
			if (t->loader.isNull())
				t->loader = _MR(Class<URLLoader>::getInstanceS(wrk));
			t->incRef();
			URLRequest* req = Class<URLRequest>::getInstanceS(wrk,strurl,method,_MR(t));
			asAtom urlarg = asAtomHandler::fromObjectNoPrimitive(req);
			asAtom loaderobj = asAtomHandler::fromObjectNoPrimitive(t->loader.getPtr());
			URLLoader::load(ret,wrk,loaderobj,&urlarg,1);
		}
		else
			LOG(LOG_NOT_IMPLEMENTED,"LoadVars.sendAndLoad with target "<<target->toDebugString());
	}
	else
		LOG(LOG_ERROR,"LoadVars.sendAndLoad called without target");
}
ASFUNCTIONBODY_ATOM(AVM1LoadVars,load)
{
	AVM1LoadVars* th = asAtomHandler::as<AVM1LoadVars>(obj);
	tiny_string strurl;
	ARG_CHECK(ARG_UNPACK(strurl));

	if (th->loader.isNull())
		th->loader = _MR(Class<URLLoader>::getInstanceS(wrk));
	th->incRef();
	URLRequest* req = Class<URLRequest>::getInstanceS(wrk,strurl,"GET",_MR(th));
	asAtom urlarg = asAtomHandler::fromObjectNoPrimitive(req);
	asAtom loaderobj = asAtomHandler::fromObjectNoPrimitive(th->loader.getPtr());
	URLLoader::load(ret,wrk,loaderobj,&urlarg,1);
}
bool AVM1LoadVars::destruct()
{
	getSystemState()->stage->AVM1RemoveEventListener(this);
	return URLVariables::destruct();
}
multiname* AVM1LoadVars::setVariableByMultiname(multiname& name, asAtom& o, CONST_ALLOWED_FLAG allowConst, bool *alreadyset,ASWorker* wrk)
{
	multiname* res = URLVariables::setVariableByMultiname(name,o,allowConst,alreadyset,wrk);
	if (name.name_s_id == BUILTIN_STRINGS::STRING_ONLOAD || name.name_s_id == BUILTIN_STRINGS::STRING_ONDATA)
	{
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
			m.name_s_id=BUILTIN_STRINGS::STRING_ONDATA;
			getVariableByMultiname(func,m,GET_VARIABLE_OPTION::NONE,getInstanceWorker());
			if (asAtomHandler::is<AVM1Function>(func))
			{
				asAtom ret=asAtomHandler::invalidAtom;
				asAtom obj = asAtomHandler::fromObject(this);
				asAtom args[1];
				args[0] = asAtomHandler::fromObject(loader->getData());
				asAtomHandler::as<AVM1Function>(func)->call(&ret,&obj,args,1);
				asAtomHandler::as<AVM1Function>(func)->decRef();
			}
			else
			{
				m.name_s_id=BUILTIN_STRINGS::STRING_ONLOAD;
				getVariableByMultiname(func,m,GET_VARIABLE_OPTION::NONE,getInstanceWorker());
				if (asAtomHandler::is<AVM1Function>(func))
				{
					if (loader->getDataFormat()=="text")
					{
						// TODO how are '&' or '=' handled when inside keys/values?
						tiny_string s = loader->getData()->toString();
						std::list<tiny_string> spl = s.split((uint32_t)'&');
						for (auto it = spl.begin(); it != spl.end(); it++)
						{
							std::list<tiny_string> spl2 = (*it).split((uint32_t)'=');
							if (spl2.size() ==2)
							{
								multiname mdata(nullptr);
								mdata.name_type=multiname::NAME_STRING;
								mdata.isAttribute = false;
								tiny_string key =spl2.front();
								mdata.name_s_id = getSystemState()->getUniqueStringId(key);
								asAtom value = asAtomHandler::fromString(getSystemState(),spl2.back());
								this->setVariableByMultiname(mdata,value,ASObject::CONST_ALLOWED,nullptr,this->getInstanceWorker());
							}
						}
					}
					asAtom ret=asAtomHandler::invalidAtom;
					asAtom obj = asAtomHandler::fromObject(this);
					asAtom args[1];
					args[0] = e->type == "complete" ? asAtomHandler::trueAtom : asAtomHandler::falseAtom;
					asAtomHandler::as<AVM1Function>(func)->call(&ret,&obj,args,1);
					asAtomHandler::as<AVM1Function>(func)->decRef();
				}
			}
		}
	}
}
void AVM1NetConnection::sinit(Class_base *c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_DYNAMIC_NOT_FINAL);
	c->setDeclaredMethodByQName("connect","",Class<IFunction>::getFunction(c->getSystemState(),connect),NORMAL_METHOD,true);
}

void AVM1XMLSocket::sinit(Class_base* c)
{
	XMLSocket::sinit(c);
	c->isSealed=false;
}

void AVM1NetStream::sinit(Class_base *c)
{
	CLASS_SETUP(c, EventDispatcher, _constructor, CLASS_DYNAMIC_NOT_FINAL);
	c->setDeclaredMethodByQName("play","",Class<IFunction>::getFunction(c->getSystemState(),play),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("pause","",Class<IFunction>::getFunction(c->getSystemState(),avm1pause),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("close","",Class<IFunction>::getFunction(c->getSystemState(),close),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("seek","",Class<IFunction>::getFunction(c->getSystemState(),seek),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("bytesLoaded","",Class<IFunction>::getFunction(c->getSystemState(),_getBytesLoaded),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("bytesTotal","",Class<IFunction>::getFunction(c->getSystemState(),_getBytesTotal),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("time","",Class<IFunction>::getFunction(c->getSystemState(),_getTime),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("currentFPS","",Class<IFunction>::getFunction(c->getSystemState(),_getCurrentFPS),GETTER_METHOD,true);
	REGISTER_GETTER(c, bufferLength);
	REGISTER_GETTER(c, bufferTime);
	c->setDeclaredMethodByQName("setBufferTime","",Class<IFunction>::getFunction(c->getSystemState(),_setter_bufferTime),NORMAL_METHOD,true);
}
ASFUNCTIONBODY_ATOM(AVM1NetStream,avm1pause)
{
	AVM1NetStream* th=asAtomHandler::as<AVM1NetStream>(obj);
	bool pause;
	if (argslen==0)
		th->togglePause(ret,wrk,obj, nullptr, 0);
	else
	{
		ARG_CHECK(ARG_UNPACK(pause));
		if (pause)
			th->pause(ret,wrk,obj, nullptr, 0);
		else
			th->resume(ret,wrk,obj, nullptr, 0);
	}
	
}
void AVM1NetStream::AVM1HandleEvent(EventDispatcher *dispatcher, Event* e)
{
	if (dispatcher == this)
	{
		if (e->type == "netStatus")
		{
			asAtom func=asAtomHandler::invalidAtom;
			multiname m(nullptr);
			m.name_type=multiname::NAME_STRING;
			m.isAttribute = false;
			m.name_s_id=getSystemState()->getUniqueStringId("onStatus");
			getVariableByMultiname(func,m,GET_VARIABLE_OPTION::NONE,dispatcher->getInstanceWorker());
			if (asAtomHandler::isInvalid(func)) //class AVM1NetStream is dynamic, so we also have to check the prototype variable
			{
				multiname mproto(nullptr);
				mproto.name_type=multiname::NAME_STRING;
				mproto.isAttribute = false;
				mproto.name_s_id=BUILTIN_STRINGS::PROTOTYPE;
				asAtom proto = asAtomHandler::invalidAtom;
				getVariableByMultiname(proto,mproto,GET_VARIABLE_OPTION::NONE,dispatcher->getInstanceWorker());
				if (asAtomHandler::isObject(proto))
				{
					asAtomHandler::getObjectNoCheck(proto)->getVariableByMultiname(func,m,GET_VARIABLE_OPTION::NONE,dispatcher->getInstanceWorker());
				}
			}
			
			if (asAtomHandler::is<AVM1Function>(func))
			{
				asAtom ret=asAtomHandler::invalidAtom;
				asAtom obj = asAtomHandler::fromObject(this);
				
				multiname minfo(nullptr);
				minfo.name_type=multiname::NAME_STRING;
				minfo.isAttribute = false;
				minfo.name_s_id=getSystemState()->getUniqueStringId("info");
				asAtom arg0 = asAtomHandler::invalidAtom;
				e->getVariableByMultiname(arg0,minfo,GET_VARIABLE_OPTION::NONE,dispatcher->getInstanceWorker());
				asAtomHandler::as<AVM1Function>(func)->call(&ret,&obj,&arg0,1);
				asAtomHandler::as<AVM1Function>(func)->decRef();
			}
		}
	}
}
