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
#include "scripting/flash/display/Stage.h"
#include "scripting/flash/events/StatusEvent.h"
#include "scripting/toplevel/AVM1Function.h"
#include "scripting/class.h"
#include "scripting/argconv.h"

using namespace std;
using namespace lightspark;

void AVM1SharedObject::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, EventDispatcher, CLASS_SEALED);
	c->setDeclaredMethodByQName("getLocal","",c->getSystemState()->getBuiltinFunction(getLocal),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("size","",c->getSystemState()->getBuiltinFunction(_getSize),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("flush","",c->getSystemState()->getBuiltinFunction(flush),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("clear","",c->getSystemState()->getBuiltinFunction(clear),NORMAL_METHOD,true);
	REGISTER_GETTER(c,data);
}

AVM1LocalConnection::AVM1LocalConnection(ASWorker* wrk, Class_base* c):LocalConnection(wrk,c)
{
	subtype=SUBTYPE_AVM1LOCALCONNECTION;
}

void AVM1LocalConnection::sinit(Class_base *c)
{
	CLASS_SETUP(c, EventDispatcher, _constructor, CLASS_DYNAMIC_NOT_FINAL);
	c->prototype->setDeclaredMethodByQName("allowDomain","",c->getSystemState()->getBuiltinFunction(allowDomain),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("allowInsecureDomain","",c->getSystemState()->getBuiltinFunction(allowInsecureDomain),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("send","",c->getSystemState()->getBuiltinFunction(send),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("connect","",c->getSystemState()->getBuiltinFunction(connect),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("close","",c->getSystemState()->getBuiltinFunction(close),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("domain","",c->getSystemState()->getBuiltinFunction(domain),NORMAL_METHOD,false);
}

void AVM1LocalConnection::AVM1HandleEvent(EventDispatcher* dispatcher, Event* e)
{
	if (dispatcher == this)
	{
		if (e->is<StatusEvent>())
		{
			asAtom func=asAtomHandler::invalidAtom;
			multiname m(nullptr);
			m.name_type=multiname::NAME_STRING;
			m.isAttribute = false;
			m.name_s_id=BUILTIN_STRINGS::STRING_ONSTATUS;
			getVariableByMultiname(func,m,GET_VARIABLE_OPTION::NONE,getInstanceWorker());
			if (asAtomHandler::is<AVM1Function>(func))
			{
				asAtom ret=asAtomHandler::invalidAtom;
				asAtom obj = asAtomHandler::fromObject(this);
				asAtom args[1];
				ASObject* infoobj = new_asobject(getInstanceWorker());
				multiname mlvl(nullptr);
				mlvl.name_type=multiname::NAME_STRING;
				mlvl.isAttribute = false;
				mlvl.name_s_id=BUILTIN_STRINGS::STRING_LEVEL;
				asAtom lvl = asAtomHandler::fromString(getSystemState(),e->as<StatusEvent>()->level);
				infoobj->setVariableByMultiname(mlvl,lvl,CONST_NOT_ALLOWED,nullptr,getInstanceWorker());
				args[0] = asAtomHandler::fromObject(infoobj);
				asAtomHandler::as<AVM1Function>(func)->call(&ret,&obj,args,1);
				ASATOM_DECREF(func);
				infoobj->decRef();
			}
		}
	}

}

void AVM1LoadVars::sinit(Class_base *c)
{
	CLASS_SETUP(c, URLVariables, _constructor, CLASS_DYNAMIC_NOT_FINAL);
	c->isReusable=true;
	c->prototype->setDeclaredMethodByQName("sendAndLoad","",c->getSystemState()->getBuiltinFunction(sendAndLoad),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("load","",c->getSystemState()->getBuiltinFunction(load),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("getBytesLoaded","",c->getSystemState()->getBuiltinFunction(getBytesLoaded),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("getBytesTotal","",c->getSystemState()->getBuiltinFunction(getBytesTotal),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("loaded","",c->getSystemState()->getBuiltinFunction(loaded),GETTER_METHOD,false);
}

ASFUNCTIONBODY_ATOM(AVM1LoadVars,_constructor)
{
}

ASFUNCTIONBODY_ATOM(AVM1LoadVars,sendAndLoad)
{
	AVM1LoadVars* th = asAtomHandler::as<AVM1LoadVars>(obj);
	ret = asAtomHandler::falseAtom;
	if (argslen<2)
		return;
	tiny_string strurl;
	tiny_string method;
	asAtom target = asAtomHandler::nullAtom;
	ARG_CHECK(ARG_UNPACK(strurl)(target)(method,"POST"));
	
	if (asAtomHandler::is<AVM1LoadVars>(target))
	{
		AVM1LoadVars* t = asAtomHandler::as<AVM1LoadVars>(target);
		th->copyValues(t,wrk);
		if (!t->loader)
		{
			t->loader = Class<URLLoader>::getInstanceSNoArgs(wrk);
			t->loader->addStoredMember();
		}
		URLRequest* req = Class<URLRequest>::getInstanceS(wrk,strurl,method,target);
		asAtom urlarg = asAtomHandler::fromObjectNoPrimitive(req);
		asAtom loaderobj = asAtomHandler::fromObjectNoPrimitive(t->loader);
		URLLoader::load(ret,wrk,loaderobj,&urlarg,1);
		if (ret.uintval == asAtomHandler::trueAtom.uintval)
			t->incRef(); // keep target alive until the download is done, will be decreffed in AVM1HandleEvent
		req->decRef();
	}
	else if (!asAtomHandler::isUndefined(target) && !asAtomHandler::isNull(target))
		LOG(LOG_NOT_IMPLEMENTED,"LoadVars.sendAndLoad with target "<<asAtomHandler::toDebugString(target));
}
ASFUNCTIONBODY_ATOM(AVM1LoadVars,load)
{
	AVM1LoadVars* th = asAtomHandler::as<AVM1LoadVars>(obj);
	ret = asAtomHandler::falseAtom;
	if (argslen==0)
		return;
	tiny_string strurl;
	ARG_CHECK(ARG_UNPACK(strurl));

	if (!th->loader)
	{
		th->loader = Class<URLLoader>::getInstanceS(wrk);
		th->loader->addStoredMember();
	}
	URLRequest* req = Class<URLRequest>::getInstanceS(wrk,strurl,"GET",asAtomHandler::fromObjectNoPrimitive(th));
	asAtom urlarg = asAtomHandler::fromObjectNoPrimitive(req);
	asAtom loaderobj = asAtomHandler::fromObjectNoPrimitive(th->loader);
	URLLoader::load(ret,wrk,loaderobj,&urlarg,1);
	if (ret.uintval == asAtomHandler::trueAtom.uintval)
		th->incRef(); // keep this alive until the download is done, will be decreffed in AVM1HandleEvent
	req->decRef();
}

ASFUNCTIONBODY_ATOM(AVM1LoadVars,getBytesLoaded)
{
	AVM1LoadVars* th = asAtomHandler::as<AVM1LoadVars>(obj);
	if (th->loader)
		ret = asAtomHandler::fromUInt(th->loader->bytesLoaded);
}
ASFUNCTIONBODY_ATOM(AVM1LoadVars,getBytesTotal)
{
	AVM1LoadVars* th = asAtomHandler::as<AVM1LoadVars>(obj);
	if (th->loader)
		ret = asAtomHandler::fromUInt(th->loader->bytesTotal);
}
ASFUNCTIONBODY_ATOM(AVM1LoadVars,loaded)
{
	AVM1LoadVars* th = asAtomHandler::as<AVM1LoadVars>(obj);
	ret = asAtomHandler::falseAtom;
	if (th->loader &&
		th->loader->bytesTotal &&
		th->loader->bytesLoaded == th->loader->bytesTotal)
		ret = asAtomHandler::trueAtom;
}

void AVM1LoadVars::finalize()
{
	getSystemState()->stage->AVM1RemoveEventListener(this);
	URLVariables::finalize();
	if (loader)
		loader->removeStoredMember();
	loader=nullptr;
}
bool AVM1LoadVars::destruct()
{
	getSystemState()->stage->AVM1RemoveEventListener(this);
	if (loader)
		loader->removeStoredMember();
	loader=nullptr;
	return URLVariables::destruct();
}
void AVM1LoadVars::prepareShutdown()
{
	if (preparedforshutdown)
		return;
	URLVariables::prepareShutdown();
	if (loader)
		loader->prepareShutdown();
}
bool AVM1LoadVars::countCylicMemberReferences(garbagecollectorstate& gcstate)
{
	bool ret = URLVariables::countCylicMemberReferences(gcstate);
	if (loader)
		ret = loader->countAllCylicMemberReferences(gcstate) || ret;
	return ret;
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
	if (dispatcher == loader)
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
				if (asAtomHandler::isValid(loader->getData()))
					args[0] = loader->getData();
				else
					args[0] = asAtomHandler::nullAtom;
				asAtomHandler::as<AVM1Function>(func)->call(&ret,&obj,args,1);
				ASATOM_DECREF(func);
			}
			else
			{
				ASATOM_DECREF(func);
				func = asAtomHandler::invalidAtom;
				m.name_s_id=BUILTIN_STRINGS::STRING_ONLOAD;
				getVariableByMultiname(func,m,GET_VARIABLE_OPTION::NONE,getInstanceWorker());
				if (asAtomHandler::is<AVM1Function>(func))
				{
					if (loader->getDataFormat()=="text" && asAtomHandler::isValid(loader->getData()))
					{
						tiny_string s = asAtomHandler::toString(loader->getData(),getInstanceWorker());
						decode(s);
					}
					asAtom ret=asAtomHandler::invalidAtom;
					asAtom obj = asAtomHandler::fromObject(this);
					asAtom args[1];
					args[0] = e->type == "complete" ? asAtomHandler::trueAtom : asAtomHandler::falseAtom;
					asAtomHandler::as<AVM1Function>(func)->call(&ret,&obj,args,1);
				}
				ASATOM_DECREF(func);
			}
			this->decRef(); // was increffed in load/sendAndLoad
		}
	}
}
void AVM1NetConnection::sinit(Class_base *c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_DYNAMIC_NOT_FINAL);
	c->setDeclaredMethodByQName("connect","",c->getSystemState()->getBuiltinFunction(connect),NORMAL_METHOD,true);
}

void AVM1XMLSocket::sinit(Class_base* c)
{
	XMLSocket::sinit(c);
	c->isSealed=false;
}

void AVM1NetStream::sinit(Class_base *c)
{
	CLASS_SETUP(c, EventDispatcher, _constructor, CLASS_DYNAMIC_NOT_FINAL);
	c->setDeclaredMethodByQName("play","",c->getSystemState()->getBuiltinFunction(play),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("pause","",c->getSystemState()->getBuiltinFunction(avm1pause),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("close","",c->getSystemState()->getBuiltinFunction(close),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("seek","",c->getSystemState()->getBuiltinFunction(seek),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("bytesLoaded","",c->getSystemState()->getBuiltinFunction(_getBytesLoaded),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("bytesTotal","",c->getSystemState()->getBuiltinFunction(_getBytesTotal),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("time","",c->getSystemState()->getBuiltinFunction(_getTime),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("currentFPS","",c->getSystemState()->getBuiltinFunction(_getCurrentFPS),GETTER_METHOD,true);
	REGISTER_GETTER(c, bufferLength);
	REGISTER_GETTER(c, bufferTime);
	c->setDeclaredMethodByQName("setBufferTime","",c->getSystemState()->getBuiltinFunction(_setter_bufferTime),NORMAL_METHOD,true);
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

void AVM1FileReference::finalize()
{
	EventDispatcher::finalize();
	getSystemState()->stage->AVM1RemoveEventListener(this);
	auto itlst = listeners.begin();
	while (itlst != listeners.end())
	{
		ASObject* o = (*itlst);
		itlst = listeners.erase(itlst);
		o->removeStoredMember();
	}
}

bool AVM1FileReference::destruct()
{
	getSystemState()->stage->AVM1RemoveEventListener(this);
	auto itlst = listeners.begin();
	while (itlst != listeners.end())
	{
		ASObject* o = (*itlst);
		itlst = listeners.erase(itlst);
		o->removeStoredMember();
	}
	return EventDispatcher::destruct();
}

void AVM1FileReference::prepareShutdown()
{
	if (preparedforshutdown)
		return;
	EventDispatcher::prepareShutdown();
	auto itlst = listeners.begin();
	while (itlst != listeners.end())
	{
		(*itlst)->prepareShutdown();
		itlst++;
	}
}
bool AVM1FileReference::countCylicMemberReferences(garbagecollectorstate& gcstate)
{
	bool ret = EventDispatcher::countCylicMemberReferences(gcstate);
	auto itlst = listeners.begin();
	while (itlst != listeners.end())
	{
		ret = (*itlst)->countAllCylicMemberReferences(gcstate) || ret;
		itlst++;
	}
	return ret;
}
void AVM1FileReference::sinit(Class_base* c)
{
	CLASS_SETUP(c, EventDispatcher,_constructor, CLASS_SEALED);
	c->prototype->setDeclaredMethodByQName("browse","",c->getSystemState()->getBuiltinFunction(browse),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("download","",c->getSystemState()->getBuiltinFunction(download),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("addListener","",c->getSystemState()->getBuiltinFunction(addListener),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("removeListener","",c->getSystemState()->getBuiltinFunction(removeListener),NORMAL_METHOD,false);
}

ASFUNCTIONBODY_ATOM(AVM1FileReference,addListener)
{
	AVM1FileReference* th=asAtomHandler::as<AVM1FileReference>(obj);

	wrk->getSystemState()->stage->AVM1AddEventListener(th);
	ASObject* o = asAtomHandler::toObject(args[0],wrk);

	o->incRef();
	o->addStoredMember();
	th->listeners.insert(o);
}
ASFUNCTIONBODY_ATOM(AVM1FileReference,removeListener)
{
	AVM1FileReference* th=asAtomHandler::as<AVM1FileReference>(obj);

	ASObject* o = asAtomHandler::toObject(args[0],wrk);

	auto it = th->listeners.find(o);
	if (it != th->listeners.end())
	{
		th->listeners.erase(o);
		o->removeStoredMember();
	}
}
void AVM1FileReferenceList::finalize()
{
	EventDispatcher::finalize();
	getSystemState()->stage->AVM1RemoveEventListener(this);
	auto itlst = listeners.begin();
	while (itlst != listeners.end())
	{
		ASObject* o = (*itlst);
		itlst = listeners.erase(itlst);
		o->removeStoredMember();
	}
}

bool AVM1FileReferenceList::destruct()
{
	getSystemState()->stage->AVM1RemoveEventListener(this);
	auto itlst = listeners.begin();
	while (itlst != listeners.end())
	{
		ASObject* o = (*itlst);
		itlst = listeners.erase(itlst);
		o->removeStoredMember();
	}
	return EventDispatcher::destruct();
}

void AVM1FileReferenceList::prepareShutdown()
{
	if (preparedforshutdown)
		return;
	EventDispatcher::prepareShutdown();
	auto itlst = listeners.begin();
	while (itlst != listeners.end())
	{
		(*itlst)->prepareShutdown();
		itlst++;
	}
}
bool AVM1FileReferenceList::countCylicMemberReferences(garbagecollectorstate& gcstate)
{
	bool ret = EventDispatcher::countCylicMemberReferences(gcstate);
	auto itlst = listeners.begin();
	while (itlst != listeners.end())
	{
		ret = (*itlst)->countAllCylicMemberReferences(gcstate) || ret;
		itlst++;
	}
	return ret;
}

void AVM1FileReferenceList::sinit(Class_base* c)
{
	CLASS_SETUP(c, EventDispatcher,_constructor, CLASS_SEALED);
	c->prototype->setDeclaredMethodByQName("browse","",c->getSystemState()->getBuiltinFunction(browse),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("addListener","",c->getSystemState()->getBuiltinFunction(addListener),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("removeListener","",c->getSystemState()->getBuiltinFunction(removeListener),NORMAL_METHOD,false);
}
ASFUNCTIONBODY_ATOM(AVM1FileReferenceList,addListener)
{
	AVM1FileReferenceList* th=asAtomHandler::as<AVM1FileReferenceList>(obj);

	wrk->getSystemState()->stage->AVM1AddEventListener(th);
	ASObject* o = asAtomHandler::toObject(args[0],wrk);

	o->incRef();
	o->addStoredMember();
	th->listeners.insert(o);
}
ASFUNCTIONBODY_ATOM(AVM1FileReferenceList,removeListener)
{
	AVM1FileReferenceList* th=asAtomHandler::as<AVM1FileReferenceList>(obj);

	ASObject* o = asAtomHandler::toObject(args[0],wrk);

	auto it = th->listeners.find(o);
	if (it != th->listeners.end())
	{
		th->listeners.erase(o);
		o->removeStoredMember();
	}
}
