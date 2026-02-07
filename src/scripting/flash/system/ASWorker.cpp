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

#include "scripting/flash/system/ASWorker.h"
#include "scripting/flash/system/flashsystem.h"
#include "scripting/flash/system/messagechannel.h"
#include "scripting/flash/display/RootMovieClip.h"
#include "scripting/flash/display/Stage.h"
#include "scripting/flash/display/Loader.h"
#include "scripting/flash/errors/flasherrors.h"
#include "scripting/flash/utils/ByteArray.h"
#include "scripting/flash/system/ApplicationDomain.h"
#include "scripting/toplevel/IFunction.h"
#include "scripting/toplevel/toplevel.h"
#include "scripting/avm1/scope.h"
#include "scripting/abc.h"
#include "scripting/argconv.h"
#include "parsing/tags.h"
#include "parsing/streams.h"
#include "compat.h"

using namespace lightspark;

extern uint32_t asClassCount;

ASWorker::ASWorker(SystemState* s):
	EventDispatcher(this,nullptr),parser(nullptr),
	giveAppPrivileges(false),started(false),inGarbageCollection(false),inShutdown(false),inFinalize(false),
	stage(nullptr),
	freelist(new asfreelist[asClassCount]),currentCallContext(nullptr),
	cur_recursion(0),AVM1_cur_recursion_function(0),AVM1_cur_recursion_internal(0),
	isPrimordial(true),state("running"),
	nativeExtensionCallCount(0)
{
	subtype = SUBTYPE_WORKER;
	setSystemState(s);
	// TODO: it seems that AIR applications have a higher default value for max_recursion
	// I haven't found any documentation about that, so we just set it to a value that seems to work...
	limits.max_recursion = s->flashMode == SystemState::AIR ? 2048 : 256;
	limits.script_timeout = 20;
	stacktrace = new stacktrace_entry[limits.max_recursion];
	last_garbagecollection = compat_msectiming();
	// start and end of gc list point to this to ensure that every added object gets a valid pointer set as its next/prev pointers
	gcNext=this;
	gcPrev=this;
}

ASWorker::ASWorker(Class_base* c):
	EventDispatcher(c->getSystemState()->worker,c),parser(nullptr),
	giveAppPrivileges(false),started(false),inGarbageCollection(false),inShutdown(false),inFinalize(false),
	stage(nullptr),
	freelist(new asfreelist[asClassCount]),currentCallContext(nullptr),
	cur_recursion(0),AVM1_cur_recursion_function(0),AVM1_cur_recursion_internal(0),
	isPrimordial(false),state("new"),
	nativeExtensionCallCount(0)
{
	subtype = SUBTYPE_WORKER;
	// TODO: it seems that AIR applications have a higher default value for max_recursion
	// I haven't found any documentation about that, so we just set it to a value that seems to work...
	limits.max_recursion = c->getSystemState()->flashMode == SystemState::AIR ? 2048 : 256;
	limits.script_timeout = 20;
	stacktrace = new stacktrace_entry[limits.max_recursion];
	loader = _MR(Class<Loader>::getInstanceS(this));
	last_garbagecollection = compat_msectiming();
	// start and end of gc list point to this to ensure that every added object gets a valid pointer set as its next/prev pointers
	gcNext=this;
	gcPrev=this;
}
ASWorker::ASWorker(ASWorker* wrk, Class_base* c):
	EventDispatcher(wrk,c),parser(nullptr),
	giveAppPrivileges(false),started(false),inGarbageCollection(false),inShutdown(false),inFinalize(false),
	stage(nullptr),
	freelist(new asfreelist[asClassCount]),currentCallContext(nullptr),
	cur_recursion(0),AVM1_cur_recursion_function(0),AVM1_cur_recursion_internal(0),
	isPrimordial(false),state("new"),
	nativeExtensionCallCount(0)
{
	subtype = SUBTYPE_WORKER;
	// TODO: it seems that AIR applications have a higher default value for max_recursion
	// I haven't found any documentation about that, so we just set it to a value that seems to work...
	limits.max_recursion = c->getSystemState()->flashMode == SystemState::AIR ? 2048 : 256;
	limits.script_timeout = 20;
	stacktrace = new stacktrace_entry[limits.max_recursion];
	loader = _MR(Class<Loader>::getInstanceS(this));
	last_garbagecollection = compat_msectiming();
	// start and end of gc list point to this to ensure that every added object gets a valid pointer set as its next/prev pointers
	gcNext=this;
	gcPrev=this;
}

void ASWorker::finalize()
{
	if (inFinalize)
		return;
	inFinalize=true;
	for (auto it = avm1ClassConstructorsCaseSensitive.begin(); it != avm1ClassConstructorsCaseSensitive.end(); it++)
	{
		it->second->removeStoredMember();
	}
	avm1ClassConstructorsCaseSensitive.clear();
	for (auto it = avm1ClassConstructorsCaseInsensitive.begin(); it != avm1ClassConstructorsCaseInsensitive.end(); it++)
	{
		it->second->removeStoredMember();
	}
	avm1ClassConstructorsCaseInsensitive.clear();
	if (!this->preparedforshutdown)
		this->prepareShutdown();
	protoypeMap.clear();
	// remove all references to freelists
	for (auto it = constantrefs.begin(); it != constantrefs.end(); it++)
	{
		(*it)->prepareShutdown();
	}
	if (inShutdown)
		return;
	processGarbageCollection(true);
	if (!isPrimordial)
	{
		threadAborting = true;
		parsemutex.lock();
		if (parser)
			parser->threadAborting = true;
		parsemutex.unlock();
		threadAbort();
		sem_event_cond.signal();
		started = false;
		if (rootClip)
		{
			for(auto it = rootClip->applicationDomain->customClasses.begin(); it != rootClip->applicationDomain->customClasses.end(); ++it)
				it->second->finalize();
			for(auto it = rootClip->applicationDomain->templates.begin(); it != rootClip->applicationDomain->templates.end(); ++it)
				it->second->finalize();
			for(auto i = rootClip->applicationDomain->customClasses.begin(); i != rootClip->applicationDomain->customClasses.end(); ++i)
				i->second->decRef();
			for(auto i = rootClip->applicationDomain->templates.begin(); i != rootClip->applicationDomain->templates.end(); ++i)
				i->second->decRef();
			rootClip->applicationDomain->customClasses.clear();
			rootClip.reset();
		}
		for(size_t i=0;i<contexts.size();++i)
		{
			delete contexts[i];
		}
	}
	constantrefs.erase(stage);
	stage->destruct();
	stage->finalize();
	stage=nullptr;
	destroyContents();
	loader.reset();
	swf.reset();
	rootClip.reset();
	EventDispatcher::finalize();
	constantrefs.erase(this);
	ASObject* ogc = this->gcNext;
	while (ogc && ogc != this)
	{
		ogc->prepareShutdown();
		ogc=ogc->gcNext;
	}
	// remove all references to variables as they might point to other constant reffed objects
	for (auto it = constantrefs.begin(); it != constantrefs.end(); it++)
	{
		(*it)->destruct();
		(*it)->finalize();
	}
	processGarbageCollection(true);
	inShutdown=true;
	if (getWorker()==this)
		setTLSWorker(nullptr);
	// destruct all constant refs
	auto it = constantrefs.begin();
	it = constantrefs.begin();
	while (it != constantrefs.end())
	{
		ASObject* o = (*it);
		if (o->is<ASWorker>()
				|| o == getSystemState()->stage
				|| o == getSystemState()->workerDomain
				|| o == getSystemState()->mainClip
				|| o == getSystemState()->systemDomain
				|| o == getSystemState()->getObjectClassRef()
				)
		{
			// these will be deleted later
			it++;
			continue;
		}
		it = constantrefs.erase(it);
		delete o;
	}
	constantrefs.clear();
	delete[] stacktrace;
	delete[] freelist;
	freelist=nullptr;
}

void ASWorker::prepareShutdown()
{
	if(preparedforshutdown)
		return;
	parsemutex.lock();
	EventDispatcher::prepareShutdown();
	if (rootClip)
	{
		for(auto it = rootClip->applicationDomain->customClasses.begin(); it != rootClip->applicationDomain->customClasses.end(); ++it)
			it->second->prepareShutdown();
		for(auto it = rootClip->applicationDomain->templates.begin(); it != rootClip->applicationDomain->templates.end(); ++it)
			it->second->prepareShutdown();
		rootClip->prepareShutdown();
	}
	if (loader)
		loader->prepareShutdown();
	if (swf)
		swf->prepareShutdown();
	if (stage)
		stage->prepareShutdown();
	ASObject* ogc = this->gcNext;
	assert(ogc);
	while (ogc != this)
	{
		ogc->prepareShutdown();
		ogc = ogc->gcNext;
	}
	parsemutex.unlock();
}

Prototype* ASWorker::getClassPrototype(const Class_base* cls)
{
	auto it = protoypeMap.find(cls);
	if (it == protoypeMap.end())
	{
		if (inFinalize)
			return nullptr;
		Prototype* p = cls->prototype->clonePrototype(this);
		it = protoypeMap.insert(make_pair(cls,_MR(p))).first;
	}
	return it->second.getPtr();
}

void ASWorker::sinit(Class_base* c)
{
	CLASS_SETUP(c, EventDispatcher, _constructorNotInstantiatable, CLASS_SEALED | CLASS_FINAL);
	c->setDeclaredMethodByQName("current","",c->getSystemState()->getBuiltinFunction(_getCurrent,0,Class<ASWorker>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("getSharedProperty","",c->getSystemState()->getBuiltinFunction(getSharedProperty,1,Class<ASObject>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("isSupported","",c->getSystemState()->getBuiltinFunction(isSupported,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
	REGISTER_GETTER_RESULTTYPE(c, isPrimordial,Boolean);
	REGISTER_GETTER_RESULTTYPE(c, state,ASString);
	c->setDeclaredMethodByQName("addEventListener","",c->getSystemState()->getBuiltinFunction(_addEventListener),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("createMessageChannel","",c->getSystemState()->getBuiltinFunction(createMessageChannel,1,Class<MessageChannel>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("removeEventListener","",c->getSystemState()->getBuiltinFunction(_removeEventListener),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setSharedProperty","",c->getSystemState()->getBuiltinFunction(setSharedProperty),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("start","",c->getSystemState()->getBuiltinFunction(start),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("terminate","",c->getSystemState()->getBuiltinFunction(terminate,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
}

void ASWorker::fillStackTrace(StackTraceList& strace)
{
	for (uint32_t i = cur_recursion; i > 0; i--)
	{
		Class_base* c = asAtomHandler::getClass(stacktrace[i-1].object,getSystemState());
		SyntheticFunction* f = stacktrace[i-1].function;
		stacktrace_string_entry e;
		e.clsname = c ? c->getQualifiedClassNameID() : (uint32_t)BUILTIN_STRINGS::EMPTY;
		e.function = f->functionname;
		e.init = (uint32_t)BUILTIN_STRINGS::EMPTY;
		if (f->isScriptInit() && !f->inClass)
			e.init = BUILTIN_STRINGS::STRING_INIT;
		else if (f->isClassInit())
			e.init = getSystemState()->getUniqueStringId("cinit");
		else if (f->inClass && f->isStatic && f->functionname != BUILTIN_STRINGS::EMPTY)
			e.init = UINT32_MAX; // indicates static class method
		else if (f->isFromNewFunction())
			e.clsname=BUILTIN_STRINGS::EMPTY;
		e.isGetter=f->isGetter;
		e.isSetter=f->isSetter;
		e.ns = f->namespaceNameID;
		e.methodnumber = f->getMethodNumber();
		strace.push_back(e);
	}
}

tiny_string ASWorker::getStackTraceString(SystemState* sys,const StackTraceList& strace, ASObject* error)
{
	tiny_string ret;
	if (error)
	{
		ret = error->toString();
	}
	for (auto it = strace.begin(); it != strace.end(); it++)
	{
		ret += "\n\tat ";
		ret += sys->getStringFromUniqueId((*it).clsname);
		if ((*it).init != BUILTIN_STRINGS::EMPTY)
		{
			ret += "$";
			if ((*it).init != UINT32_MAX)
				ret += sys->getStringFromUniqueId((*it).init);
		}
		if ((*it).function != BUILTIN_STRINGS::EMPTY)
		{
			if ((*it).clsname != BUILTIN_STRINGS::EMPTY)
				ret += "/";
			if ((it)->isGetter)
				ret +="get ";
			if ((it)->isSetter)
				ret += "set ";
			if ((it)->ns != BUILTIN_STRINGS::EMPTY)
			{
				ret += sys->getStringFromUniqueId((*it).ns);
				ret +="::";
			}
			ret += sys->getStringFromUniqueId((*it).function);
		}
		else if((*it).methodnumber != UINT32_MAX)
		{
			char buf[100];
			sprintf(buf,"MethodInfo-%i",(*it).methodnumber);
			ret += buf;
		}

		ret += "()";
		if (sys->use_testrunner_date && !sys->isShuttingDown()
			&& (*it).clsname==BUILTIN_STRINGS::STRING_GLOBAL
			&& (*it).init==BUILTIN_STRINGS::STRING_INIT)
		{
			// for some reason ruffle adds this (adobe doesn't) when we are at a scriptinit function.
			// So we do the same if we are in the testrunner
			ret +=" [TU=]";
		}
	}
	return ret;
}

void ASWorker::AVM1registerTagClass(uint32_t nameID, IFunction* theClassConstructor)
{
	unordered_map<uint32_t,IFunction*>* m = AVM1isCaseSensitive() ? &avm1ClassConstructorsCaseSensitive :&avm1ClassConstructorsCaseInsensitive;
	auto it = m->find(nameID);
	if (it != m->end())
	{
		it->second->removeStoredMember();
		m->erase(it);
	}
	if (theClassConstructor)
	{
		theClassConstructor->incRef();
		theClassConstructor->addStoredMember();
		m->insert(make_pair(nameID,theClassConstructor));
	}
}
AVM1Function* ASWorker::AVM1getClassConstructor(DisplayObject* d)
{
	uint32_t nameID = d->name;
	if (d->getTagID() != UINT32_MAX)
	{
		DictionaryTag* t = d->loadedFrom->dictionaryLookup(d->getTagID());
		if (t)
			nameID =t->nameID;
	}
	unordered_map<uint32_t,IFunction*>* m = AVM1isCaseSensitive() ? &avm1ClassConstructorsCaseSensitive :&avm1ClassConstructorsCaseInsensitive;
	auto it = m->find(nameID);
	return it != m->end() && it->second->is<AVM1Function>() ? it->second->as<AVM1Function>() : nullptr;
}

void ASWorker::throwStackOverflow()
{
	createError<StackOverflowError>(this,kStackOverflowError);
}

void ASWorker::execute()
{
	setTLSWorker(this);

	streambuf *sbuf = new bytes_buf(swf->bytes,swf->getLength());
	istream s(sbuf);
	parsemutex.lock();
	parser = new ParseThread(s,_MR(Class<ApplicationDomain>::getInstanceS(this,_MR(getSystemState()->systemDomain))),getSystemState()->mainClip->securityDomain,loader.getPtr(),"");
	parser->setForBackgroundWorker(swf->getLength());

	parsemutex.unlock();
	getSystemState()->addWorker(this);
	this->incRef();
	getVm(getSystemState())->addEvent(_MR(this),_MR(Class<Event>::getInstanceS(getInstanceWorker(),"workerState")),true);
	if (!this->threadAborting)
	{
		LOG(LOG_INFO,"start worker "<<this->toDebugString()<<" "<<this->isPrimordial<<" "<<this);
		parser->execute();
	}
	parsemutex.lock();
	delete parser;
	parser = nullptr;
	parsemutex.unlock();
	loader.reset();
	if (swf)
	{
		swf->objfreelist=nullptr;
		swf.reset();
	}
	while (true)
	{
		event_queue_mutex.lock();
		while(events_queue.empty() && !this->threadAborting)
			sem_event_cond.wait(event_queue_mutex);
		processGarbageCollection(false);
		if (this->threadAborting)
		{
			if(events_queue.empty())
			{
				event_queue_mutex.unlock();
				break;
			}
		}

		_NR<EventDispatcher> dispatcher=events_queue.front().first;
		_R<Event> e=events_queue.front().second;
		events_queue.pop_front();

		event_queue_mutex.unlock();
		try
		{
			//LOG(LOG_INFO,"worker handle event:"<<e->type);
			if (dispatcher)
			{
				dispatcher->handleEvent(e);
				dispatcher->afterHandleEvent(e.getPtr());
			}
			else
				handleInternalEvent(e.getPtr());
		}
		catch(LightsparkException& e)
		{
			LOG(LOG_ERROR,"Error in worker " << e.cause);
			getSystemState()->setError(e.cause);
			threadAborting = true;
		}
		catch(ASObject*& e)
		{
			StackTraceList stacktrace;
			fillStackTrace(stacktrace);

			if(e->getClass())
				LOG(LOG_ERROR,"Unhandled ActionScript exception in worker " << e->toString());
			else
				LOG(LOG_ERROR,"Unhandled ActionScript exception in worker (no type)");
			if (e->is<ASError>())
			{
				LOG(LOG_ERROR,"Unhandled ActionScript exception in worker " << e->as<ASError>()->getStackTraceString());
				if (getSystemState()->ignoreUnhandledExceptions)
					return;
				getSystemState()->setError(e->as<ASError>()->getStackTraceString());
			}
			else
				getSystemState()->setError(getStackTraceString(getSystemState(),stacktrace,e));
			threadAborting = true;
		}
		if (threadAborting)
		{
			while(!events_queue.empty())
			{
				_R<Event> e=events_queue.front().second;
				events_queue.pop_front();
			}
			threadAbort();
			started = false;
		}
	}
	delete sbuf;
}
void ASWorker::handleInternalEvent(Event* e)
{
	switch(e->getEventType())
	{
		case BIND_CLASS:
		{
			BindClassEvent* ev=static_cast<BindClassEvent*>(e);
			LOG(LOG_CALLS,"Binding of " << ev->class_name);
			if(ev->tag)
				ev->tag->loadedFrom->buildClassAndBindTag(ev->class_name.raw_buf(),ev->tag);
			else
				ev->base->loadedFrom->buildClassAndInjectBase(ev->class_name.raw_buf(),ev->base);
			LOG(LOG_CALLS,"End of binding of " << ev->class_name);
			break;
		}
		default:
			LOG(LOG_ERROR,"executing internal event in worker, should not happen:"<<e->type);
			break;
	}
}

void ASWorker::jobFence()
{
	state ="terminated";
	rootClip.reset();
	this->incRef();
	getVm(getSystemState())->addEvent(_MR(this),_MR(Class<Event>::getInstanceS(getInstanceWorker(),"workerState")),true);
	sem_event_cond.signal();
}

void ASWorker::threadAbort()
{
	threadAborting=true;
	sem_event_cond.signal();
	if (this->isPrimordial)
		getSystemState()->removeWorker(this);
}

void ASWorker::afterHandleEvent(Event* ev)
{
	if (ev->type=="workerState" && state=="terminated")
	{
		getSystemState()->removeWorker(this);
	}
}
bool ASWorker::addEvent(_NR<EventDispatcher> obj, _R<Event> ev)
{
	if (this->threadAborting)
	{
		if (obj)
			obj->afterHandleEvent(ev.getPtr());
		return false;
	}
	Locker l(event_queue_mutex);
	events_queue.push_back(pair<_NR<EventDispatcher>,_R<Event>>(obj, ev));
	RELEASE_WRITE(ev->queued,true);
	sem_event_cond.signal();
	return true;
}

tiny_string ASWorker::getDefaultXMLNamespace() const
{
	return getSystemState()->getStringFromUniqueId(currentCallContext ? currentCallContext->defaultNamespaceUri : (uint32_t)BUILTIN_STRINGS::EMPTY);
}

uint32_t ASWorker::getDefaultXMLNamespaceID() const
{
	return currentCallContext ? currentCallContext->defaultNamespaceUri : (uint32_t)BUILTIN_STRINGS::EMPTY;
}

void ASWorker::dumpStacktrace()
{
	StackTraceList l;
	fillStackTrace(l);

	tiny_string strace = getStackTraceString(getSystemState(),l,nullptr);
	LOG(LOG_INFO,"current stacktrace:" << strace);
}

void ASWorker::addObjectToGarbageCollector(ASObject* o)
{
	if (o->gcPrev || o->gcNext || this->gcNext == o)
		return;
	assert(o!=this);
	assert (this->gcNext);
	this->gcPrev=o;
	o->gcNext=this->gcNext;
	o->gcPrev=this;
	this->gcNext->gcPrev=o;
	this->gcNext=o;
}

void ASWorker::removeObjectFromGarbageCollector(ASObject* o)
{
	if (!o->gcPrev || !o->gcNext || o==this)
		return;
	o->gcPrev->gcNext = o->gcNext;
	o->gcNext->gcPrev = o->gcPrev;
	if (o->is<ASWorker>())
	{
		o->gcNext=o;
		o->gcPrev=o;
	}
	else
	{
		o->gcNext=nullptr;
		o->gcPrev=nullptr;
	}
}

void ASWorker::processGarbageCollection(bool force)
{
	uint64_t currtime = compat_msectiming();
	int64_t diff =  currtime-last_garbagecollection;
	if (!force && !getSystemState()->use_testrunner_date && diff < 10000) // ony execute garbagecollection every 10 seconds
		return;
	last_garbagecollection = currtime;
	if (this->stage)
		this->stage->cleanupDeadHiddenObjects();
	inGarbageCollection=true;
	bool hasEntries=this->gcNext && this->gcNext != this;
	// use two loops to make sure objects added during inner loop are handled _after_ the inner loop is complete
	LOG(LOG_CALLS,"start garbage collection");
	while (hasEntries)
	{
		ASObject* ogc = this->gcNext;
		hasEntries=false;
		while (ogc && ogc != this)
		{
			ASObject* ogcnext = ogc->gcNext;
			if (!ogc->deletedingarbagecollection)
			{
				this->removeObjectFromGarbageCollector(ogc);
				ogc->markedforgarbagecollection = false;
				if (ogc->handleGarbageCollection())
				{
					assert(ogc->deletedingarbagecollection);
					hasEntries=true;
				}
			}
			ogc = ogcnext;
		}
	}
	inGarbageCollection=false;
	// delete all objects that were destructed during gc
	ASObject* ogc = this->gcNext;
	while (ogc && ogc != this)
	{
		ASObject* ogcnext = ogc->gcNext;
		if (ogc->deletedingarbagecollection)
		{
			ogc->deletedingarbagecollection=false;
			ogc->removefromGarbageCollection();
			ogc->resetRefCount();
			ogc->setConstant(false);
			ogc->decRef();
		}
		ogc = ogcnext;
	}
	if (force && this->gcNext && this->gcNext != this)
		processGarbageCollection(true);
	LOG(LOG_CALLS,"garbage collection done");
}

void ASWorker::registerConstantRef(ASObject* obj)
{
	if (inFinalize || obj->getConstant())
		return;
	constantrefmutex.lock();
	constantrefs.insert(obj);
	constantrefmutex.unlock();
}

asAtom ASWorker::getCurrentGlobalAtom(const asAtom& defaultObj)
{
	// get the current Global object
	asAtom ret = asAtomHandler::invalidAtom;
	call_context* cc = currentCallContext;
	if (!cc)
		ret=defaultObj;
	else if (cc->parent_scope_stack && cc->parent_scope_stack->scope.size() > 0)
		ret = cc->parent_scope_stack->scope[0].object;
	else
	{
		assert_and_throw(cc->curr_scope_stack > 0);
		ret = cc->scope_stack[0];
	}
	return ret;
}

ASFUNCTIONBODY_GETTER(ASWorker, state)
ASFUNCTIONBODY_GETTER(ASWorker, isPrimordial)

ASFUNCTIONBODY_ATOM(ASWorker,_getCurrent)
{
	wrk->incRef();
	ret = asAtomHandler::fromObject(wrk);
}
ASFUNCTIONBODY_ATOM(ASWorker,getSharedProperty)
{
	tiny_string key;
	ARG_CHECK(ARG_UNPACK(key));
	Locker l(wrk->getSystemState()->workerDomain->workersharedobjectmutex);

	multiname m(nullptr);
	m.name_type=multiname::NAME_STRING;
	m.name_s_id=wrk->getSystemState()->getUniqueStringId(key);
	m.ns.push_back(nsNameAndKind(wrk->getSystemState(),"",NAMESPACE));
	m.isAttribute = false;
	if (wrk->getSystemState()->workerDomain->workerSharedObject->hasPropertyByMultiname(m,true,false,wrk))
		wrk->getSystemState()->workerDomain->workerSharedObject->getVariableByMultiname(ret,m,GET_VARIABLE_OPTION::NONE,wrk);
	else
		asAtomHandler::setNull(ret);
}
ASFUNCTIONBODY_ATOM(ASWorker,isSupported)
{
	asAtomHandler::setBool(ret,true);
}
ASFUNCTIONBODY_ATOM(ASWorker,_addEventListener)
{
	ASWorker* th = asAtomHandler::as<ASWorker>(obj);
	EventDispatcher::addEventListener(ret,th,obj,args,argslen);
}
ASFUNCTIONBODY_ATOM(ASWorker,createMessageChannel)
{
	ASWorker* th = asAtomHandler::as<ASWorker>(obj);
	_NR<ASWorker> receiver;
	ARG_CHECK(ARG_UNPACK(receiver));
	if (receiver.isNull())
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError,"receiver");
		return;
	}
	MessageChannel* channel = Class<MessageChannel>::getInstanceSNoArgs(th);
	th->incRef();
	th->addStoredMember();
	channel->sender = th;
	receiver->incRef();
	receiver->addStoredMember();
	channel->receiver = receiver.getPtr();
	wrk->getSystemState()->workerDomain->addMessageChannel(channel);
	ret = asAtomHandler::fromObjectNoPrimitive(channel);
}
ASFUNCTIONBODY_ATOM(ASWorker,_removeEventListener)
{
	EventDispatcher::removeEventListener(ret,wrk,obj,args,argslen);
}
ASFUNCTIONBODY_ATOM(ASWorker,setSharedProperty)
{
	tiny_string key;
	asAtom value=asAtomHandler::invalidAtom;
	ARG_CHECK(ARG_UNPACK(key)(value));
	Locker l(wrk->getSystemState()->workerDomain->workersharedobjectmutex);
	ASATOM_INCREF(value);
	multiname m(nullptr);
	m.name_type=multiname::NAME_STRING;
	m.name_s_id=wrk->getSystemState()->getUniqueStringId(key);
	m.ns.push_back(nsNameAndKind(wrk->getSystemState(),"",NAMESPACE));
	m.isAttribute = false;
	wrk->getSystemState()->workerDomain->workerSharedObject->setVariableByMultiname(m,value,CONST_NOT_ALLOWED,nullptr,wrk);
}
ASFUNCTIONBODY_ATOM(ASWorker,start)
{
	ASWorker* th = asAtomHandler::as<ASWorker>(obj);
	if (th->started)
	{
		createError<ASError>(wrk,kWorkerAlreadyStarted);
		return;
	}
	if (!th->swf.isNull())
	{
		th->started = true;
		wrk->getSystemState()->addJob(th);
	}
	asAtomHandler::setUndefined(ret);
}
ASFUNCTIONBODY_ATOM(ASWorker,terminate)
{
	ASWorker* th = asAtomHandler::as<ASWorker>(obj);
	if (th->isPrimordial)
		asAtomHandler::setBool(ret,false);
	else
	{
		th->threadAborting = true;
		th->parsemutex.lock();
		if (th->parser)
			th->parser->threadAborting = true;
		th->parsemutex.unlock();
		th->threadAbort();
		asAtomHandler::setBool(ret,th->started);
		th->started = false;
		th->getSystemState()->removeWorker(th);
	}
}
