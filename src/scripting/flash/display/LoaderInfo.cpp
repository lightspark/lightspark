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

#include <list>

#include "scripting/abc.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include "scripting/flash/display/LoaderInfo.h"
#include "scripting/flash/display/Loader.h"
#include "scripting/flash/display/RootMovieClip.h"
#include "scripting/flash/events/HttpStatusEvent.h"
#include "scripting/toplevel/Number.h"
#include "scripting/toplevel/Integer.h"
#include "scripting/toplevel/UInteger.h"

using namespace std;
using namespace lightspark;

LoaderInfo::LoaderInfo(ASWorker* wrk,Class_base* c):EventDispatcher(wrk,c),applicationDomain(NullRef),securityDomain(NullRef),
	contentType("application/x-shockwave-flash"),
	local_pt(nullptr),sbuf(nullptr),
	bytesLoaded(0),bytesLoadedPublic(0),bytesTotal(0),sharedEvents(NullRef),
	loader(nullptr),content(nullptr),bytesData(NullRef),progressEvent(nullptr),
	loadStatus(LOAD_START),
	fromByteArray(false),
	hasavm1target(false),
	actionScriptVersion(3),swfVersion(0),
	childAllowsParent(true),uncaughtErrorEvents(NullRef),parentAllowsChild(true),frameRate(0)
{
	subtype=SUBTYPE_LOADERINFO;
	sharedEvents=_MR(Class<EventDispatcher>::getInstanceS(wrk));
	parameters = _MR(new_asobject(wrk));
	uncaughtErrorEvents = _MR(Class<UncaughtErrorEvents>::getInstanceS(wrk));
	LOG(LOG_NOT_IMPLEMENTED,"LoaderInfo: childAllowsParent and parentAllowsChild always return true");
}

LoaderInfo::LoaderInfo(ASWorker* wrk, Class_base* c, Loader* l):EventDispatcher(wrk,c),applicationDomain(NullRef),securityDomain(NullRef),
	contentType("application/x-shockwave-flash"),
	local_pt(nullptr),sbuf(nullptr),
	bytesLoaded(0),bytesLoadedPublic(0),bytesTotal(0),sharedEvents(NullRef),
	loader(l),content(nullptr),bytesData(NullRef),progressEvent(nullptr),
	loadStatus(LOAD_START),
	fromByteArray(false),
	hasavm1target(false),
	actionScriptVersion(3),swfVersion(0),
	childAllowsParent(true),uncaughtErrorEvents(NullRef),parentAllowsChild(true),frameRate(0)
{
	subtype=SUBTYPE_LOADERINFO;
	sharedEvents=_MR(Class<EventDispatcher>::getInstanceS(wrk));
	parameters = _MR(new_asobject(wrk));
	uncaughtErrorEvents = _MR(Class<UncaughtErrorEvents>::getInstanceS(wrk));
	LOG(LOG_NOT_IMPLEMENTED,"LoaderInfo: childAllowsParent and parentAllowsChild always return true");
}

void LoaderInfo::parseData(streambuf *_sbuf)
{
	sbuf=_sbuf;
	istream s(sbuf);

	local_pt = new ParseThread(s,applicationDomain,securityDomain,loader,url);
	local_pt->execute();
}

void LoaderInfo::sinit(Class_base* c)
{
	CLASS_SETUP(c, EventDispatcher, _constructor, CLASS_SEALED);
	c->isReusable = true;
	c->setDeclaredMethodByQName("loaderURL","",c->getSystemState()->getBuiltinFunction(_getLoaderURL,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("loader","",c->getSystemState()->getBuiltinFunction(_getLoader,0,Class<Loader>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("content","",c->getSystemState()->getBuiltinFunction(_getContent,0,Class<DisplayObject>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("url","",c->getSystemState()->getBuiltinFunction(_getURL,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("bytesLoaded","",c->getSystemState()->getBuiltinFunction(_getBytesLoaded,0,Class<UInteger>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("bytesTotal","",c->getSystemState()->getBuiltinFunction(_getBytesTotal,0,Class<UInteger>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("bytes","",c->getSystemState()->getBuiltinFunction(_getBytes,0,Class<ByteArray>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("applicationDomain","",c->getSystemState()->getBuiltinFunction(_getApplicationDomain,0,Class<ApplicationDomain>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("sharedEvents","",c->getSystemState()->getBuiltinFunction(_getSharedEvents,0,Class<EventDispatcher>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("width","",c->getSystemState()->getBuiltinFunction(_getWidth,0,Class<Integer>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("height","",c->getSystemState()->getBuiltinFunction(_getHeight,0,Class<Integer>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	REGISTER_GETTER_RESULTTYPE(c,parameters,ASObject);
	REGISTER_GETTER_RESULTTYPE(c,actionScriptVersion,UInteger);
	REGISTER_GETTER_RESULTTYPE(c,swfVersion,UInteger);
	REGISTER_GETTER_RESULTTYPE(c,childAllowsParent,Boolean);
	REGISTER_GETTER_RESULTTYPE(c,contentType,ASString);
	REGISTER_GETTER_RESULTTYPE(c,uncaughtErrorEvents,UncaughtErrorEvents);
	REGISTER_GETTER_RESULTTYPE(c,parentAllowsChild,Boolean);
	REGISTER_GETTER_RESULTTYPE(c,frameRate,Number);
}

ASFUNCTIONBODY_GETTER(LoaderInfo,parameters)
ASFUNCTIONBODY_GETTER(LoaderInfo,actionScriptVersion)
ASFUNCTIONBODY_GETTER(LoaderInfo,childAllowsParent)
ASFUNCTIONBODY_GETTER(LoaderInfo,contentType)
ASFUNCTIONBODY_GETTER(LoaderInfo,swfVersion)
ASFUNCTIONBODY_GETTER(LoaderInfo,uncaughtErrorEvents)
ASFUNCTIONBODY_GETTER(LoaderInfo,parentAllowsChild)
ASFUNCTIONBODY_GETTER(LoaderInfo,frameRate)

bool LoaderInfo::destruct()
{
	Locker l(spinlock);
	if (sbuf)
		delete sbuf;
	sbuf = nullptr;

	sharedEvents.reset();
	loader=nullptr;
	if (content)
		content->removeStoredMember();
	content=nullptr;
	applicationDomain.reset();
	securityDomain.reset();
	bytesData.reset();
	contentType = "application/x-shockwave-flash";
	bytesLoaded = 0;
	bytesLoadedPublic = 0;
	bytesTotal = 0;
	loadStatus =LOAD_START;
	actionScriptVersion = 3;
	swfVersion = 0;
	childAllowsParent = true;
	uncaughtErrorEvents.reset();
	parentAllowsChild =true;
	frameRate =0;
	parameters.reset();
	uncaughtErrorEvents.reset();
	assert(!progressEvent);
	progressEvent=nullptr;
	loaderevents.clear();
	if(local_pt)
		delete local_pt;
	local_pt=nullptr;
	return EventDispatcher::destruct();
}

void LoaderInfo::finalize()
{
	Locker l(spinlock);
	if (sbuf)
		delete sbuf;
	sbuf = nullptr;
	if(local_pt)
		delete local_pt;
	local_pt=nullptr;
	sharedEvents.reset();
	loader=nullptr;
	if (content)
		content->removeStoredMember();
	content=nullptr;
	applicationDomain.reset();
	securityDomain.reset();
	bytesData.reset();
	uncaughtErrorEvents.reset();
	parameters.reset();
	uncaughtErrorEvents.reset();
	assert(!progressEvent);
	progressEvent=nullptr;
	loaderevents.clear();
	EventDispatcher::finalize();
}

void LoaderInfo::prepareShutdown()
{
	if (preparedforshutdown)
		return;
	EventDispatcher::prepareShutdown();
	if (sharedEvents)
		sharedEvents->prepareShutdown();
	if (applicationDomain)
		applicationDomain->prepareShutdown();
	if (securityDomain)
		securityDomain->prepareShutdown();
	if (bytesData)
		bytesData->prepareShutdown();
	if (uncaughtErrorEvents)
		uncaughtErrorEvents->prepareShutdown();
	if (parameters)
		parameters->prepareShutdown();
	if (uncaughtErrorEvents)
		uncaughtErrorEvents->prepareShutdown();
	if (content)
		content->prepareShutdown();
}

bool LoaderInfo::countCylicMemberReferences(garbagecollectorstate& gcstate)
{
	bool ret = EventDispatcher::countCylicMemberReferences(gcstate);
	if (content)
		ret = content->countAllCylicMemberReferences(gcstate) || ret;
	return ret;
}

void LoaderInfo::beforeHandleEvent(Event* ev)
{
	if (ev->is<ProgressEvent>() && loadStatus == LOAD_OPENED)
		loadStatus = LOAD_PROGRESSING;
	else if (ev->is<ProgressEvent>() && ev->as<ProgressEvent>()->bytesLoaded == ev->as<ProgressEvent>()->bytesTotal)
		loadStatus = LOAD_DOWNLOAD_DONE;
	else if (ev->type=="unload" && this->content && !this->content->needsActionScript3())
		this->content->AVM1HandleEvent(this,ev);
}
void LoaderInfo::afterHandleEvent(Event* ev)
{
	Locker l(spinlock);
	if (ev == progressEvent)
	{
		progressEvent->decRef();
		progressEvent=nullptr;
	}

	auto it = loaderevents.find(ev);
	if (it != loaderevents.end())
	{
		this->loader->removeStoredMember();
		loaderevents.erase(it);
	}
}

void LoaderInfo::addLoaderEvent(Event* ev)
{
	if (this->loader)
	{
		Locker l(spinlock);
		auto it = loaderevents.find(ev);
		if (it == loaderevents.end())
		{
			this->loader->incRef();
			this->loader->addStoredMember();
			loaderevents.insert(ev);
		}
	}
}

void LoaderInfo::setOpened(bool fromBytes)
{
	if (loadStatus >= LOAD_OPENED)
		return;
	fromByteArray = fromBytes;
	if (fromByteArray)
		loadStatus = LOAD_DOWNLOAD_DONE;
	else
		loadStatus = LOAD_OPENED;

	if (bytesData.isNull())
		bytesData = _NR<ByteArray>(Class<ByteArray>::getInstanceS(getInstanceWorker()));
	// it seems an additional ProgressEvent is always added at the start of loading (see ruffle test avm2/large_preload_from_*)
	ProgressEvent* p = Class<ProgressEvent>::getInstanceS(getInstanceWorker(),0,bytesTotal);
	if (!fromByteArray)
	{
		auto ev = Class<Event>::getInstanceS(getInstanceWorker(),"open");
		this->incRef();
		if (getVm(getSystemState())->addEvent(_MR(this),_MR(ev)))
			addLoaderEvent(ev);
		this->incRef();
		if (getVm(getSystemState())->addEvent(_MR(this),_MR(p)))
			addLoaderEvent(p);
	}
	else
	{
		if (isVmThread())
		{
			this->incRef();
			getVm(getSystemState())->publicHandleEvent(this,_MR(p));
		}
		else
		{
			this->incRef();
			if (getVm(getSystemState())->addEvent(_MR(this),_MR(p)))
				addLoaderEvent(p);
		}
	}
}

DisplayObject* LoaderInfo::getParsedObject() const
{
	return content;
}

void LoaderInfo::resetState()
{
	Locker l(spinlock);
	bytesLoaded=0;
	bytesLoadedPublic = 0;
	bytesTotal=0;
	if(!bytesData.isNull())
		bytesData->setLength(0);
	loadStatus=LOAD_START;
}

void LoaderInfo::setComplete()
{
	Locker l(spinlock);
	if (loadStatus< LOAD_INIT_SENT)
	{
		sendInit();
	}
	else if (loader && !loader->loadedFrom->usesActionScript3)
	{
		this->incRef();
		auto ev = Class<Event>::getInstanceS(getInstanceWorker(),"avm1_init");
		if (getVm(getSystemState())->addIdleEvent(_MR(this),_MR(ev)))
			this->addLoaderEvent(ev);
		
	}
}

void LoaderInfo::setContent(DisplayObject* c)
{
	if (content == c)
		return;
	if (content)
		content->removeStoredMember();
	content=c;
	if (content)
		content->addStoredMember();
}
void LoaderInfo::setBytesLoaded(uint32_t b)
{
	if(b!=bytesLoaded)
	{
		spinlock.lock();
		bytesLoaded=b;
		if(getVm(getSystemState()) && loadStatus >= LOAD_OPENED)
		{
			if (fromByteArray)
			{
				assert(bytesLoaded==bytesTotal);
				ProgressEvent* p = Class<ProgressEvent>::getInstanceS(getInstanceWorker(),bytesLoaded,bytesTotal);
				this->incRef();

				if (isVmThread())
					getVm(getSystemState())->publicHandleEvent(this,_MR(p));
				else
				{
					if (getVm(getSystemState())->addEvent(_MR(this),_MR(p)))
						addLoaderEvent(p);
				}
			}
			else
			{
				// make sure that the event queue is not flooded with progressEvents
				if (!progressEvent)
				{
					progressEvent = Class<ProgressEvent>::getInstanceS(getInstanceWorker(),bytesLoaded,bytesTotal);
					this->addLoaderEvent(progressEvent);
					this->incRef();
					progressEvent->incRef();
					spinlock.unlock();
					getVm(getSystemState())->addEvent(_MR(this),_MR(progressEvent));
				}
				else
				{
					// event already exists, we only update the values
					Locker l(progressEvent->accesmutex);
					progressEvent->bytesLoaded = bytesLoaded;
					progressEvent->bytesTotal = bytesTotal;
					// if event is already in event queue, we don't need to add it again
					if (!progressEvent->queued)
					{
						this->addLoaderEvent(progressEvent);
						this->incRef();
						progressEvent->incRef();
						getVm(getSystemState())->addEvent(_MR(this),_MR(progressEvent));
						spinlock.unlock();
					}
					else
						spinlock.unlock();
				}
			}
			checkSendComplete();
		}
		else
			spinlock.unlock();
	}
}

void LoaderInfo::sendInit()
{
	// loader.content has to be set before "init" event is dispatched
	if (loader && content)
	{
		// we have a loader, so it is not the main clip
		content->incRef();
		loader->incRef();
		getVm(loader->getSystemState())->addEvent(NullRef,_MR(new (loader->getSystemState()->unaccountedMemory) SetLoaderContentEvent(_MR(content), _MR(loader))));
	}
	this->incRef();
	auto ev = Class<Event>::getInstanceS(getInstanceWorker(),"init");
	if (getVm(getSystemState())->addEvent(_MR(this),_MR(ev)))
		this->addLoaderEvent(ev);
	assert(loadStatus<LOAD_INIT_SENT);
	loadStatus=LOAD_INIT_SENT;
	checkSendComplete();
}
void LoaderInfo::checkSendComplete()
{
	if(loadStatus==LOAD_INIT_SENT && bytesTotal && bytesLoaded==bytesTotal)
	{
		if (!this->url.empty())
		{
			this->incRef();
			auto ev = Class<HTTPStatusEvent>::getInstanceS(getInstanceWorker());
			if (getVm(getSystemState())->addEvent(_MR(this),_MR(ev)))
				this->addLoaderEvent(ev);
		}

		//The clip is also complete now
		this->incRef();
		auto ev = Class<Event>::getInstanceS(getInstanceWorker(),"complete");
		if (getVm(getSystemState())->addEvent(_MR(this),_MR(ev)))
			this->addLoaderEvent(ev);
		loadStatus=LOAD_COMPLETE;
	}
}

void LoaderInfo::setURL(const tiny_string& _url, bool setParameters)
{
	url=_url;

	//Specs says that parameters should be set from the *main* SWF
	//URL query string, but testing shows that it should be the
	//loaded URL.
	//
	//TODO: the parameters should only be set if the loaded clip
	//uses AS3. See specs.
	if (setParameters)
	{
		parameters = _MR(new_asobject(getInstanceWorker()));
		SystemState::parseParametersFromURLIntoObject(url, parameters);
	}
}

ASFUNCTIONBODY_ATOM(LoaderInfo,_constructor)
{
	//LoaderInfo* th=static_cast<LoaderInfo*>(obj);
	EventDispatcher::_constructor(ret,wrk,obj,nullptr,0);
}

ASFUNCTIONBODY_ATOM(LoaderInfo,_getLoaderURL)
{
	LoaderInfo* th=asAtomHandler::as<LoaderInfo>(obj);
	ret = asAtomHandler::fromObject(abstract_s(wrk,th->loaderURL));
}

ASFUNCTIONBODY_ATOM(LoaderInfo,_getContent)
{
	//Use Loader::getContent
	LoaderInfo* th=asAtomHandler::as<LoaderInfo>(obj);
	if(!th->loader)
		asAtomHandler::setUndefined(ret);
	else
	{
		asAtom a = asAtomHandler::fromObject(th->loader);
		Loader::_getContent(ret,wrk,a,nullptr,0);
	}
}

ASFUNCTIONBODY_ATOM(LoaderInfo,_getLoader)
{
	LoaderInfo* th=asAtomHandler::as<LoaderInfo>(obj);
	if(!th->loader)
		asAtomHandler::setUndefined(ret);
	else
	{
		th->loader->incRef();
		ret = asAtomHandler::fromObject(th->loader);
	}
}

ASFUNCTIONBODY_ATOM(LoaderInfo,_getSharedEvents)
{
	LoaderInfo* th=asAtomHandler::as<LoaderInfo>(obj);

	th->sharedEvents->incRef();
	ret = asAtomHandler::fromObject(th->sharedEvents.getPtr());
}

ASFUNCTIONBODY_ATOM(LoaderInfo,_getURL)
{
	LoaderInfo* th=asAtomHandler::as<LoaderInfo>(obj);
	if (th->fromByteArray && th->loadStatus >= LOAD_INIT_SENT)
		ret = asAtomHandler::fromString(wrk->getSystemState(),"file:///");
	else if (th->url.empty() || (th->loadStatus < LOAD_INIT_SENT && th->loader))
		ret = asAtomHandler::nullAtom;
	else
		ret = asAtomHandler::fromObject(abstract_s(wrk,th->url));
}

ASFUNCTIONBODY_ATOM(LoaderInfo,_getBytesLoaded)
{
	LoaderInfo* th=asAtomHandler::as<LoaderInfo>(obj);
	// we always return bytesLoadedPublic to ensure it shows the same value as the last handled ProgressEvent
	asAtomHandler::setUInt(ret,wrk,th->bytesLoadedPublic);
}

ASFUNCTIONBODY_ATOM(LoaderInfo,_getBytesTotal)
{
	LoaderInfo* th=asAtomHandler::as<LoaderInfo>(obj);

	if (th->loadStatus == LOAD_START || th->loadStatus==LOAD_OPENED)
		asAtomHandler::setUInt(ret,wrk,0);
	else
		asAtomHandler::setUInt(ret,wrk,th->bytesTotal);
}

bool LoaderInfo::fillBytesData(ByteArray* data)
{
	if (this->loadStatus < LOAD_OPENED)
		return false;
	if (bytesData.isNull())
		bytesData = _NR<ByteArray>(Class<ByteArray>::getInstanceS(getInstanceWorker()));
	if (loader && this->loadStatus < LOAD_DOWNLOAD_DONE)
		return true;
	if (data)
	{
		bytesData->setLength(0);
		bytesData->append(data->getBufferNoCheck(),data->getLength());
		return true;
	}
	else if (!loader) //th is the LoaderInfo of the main clip
	{
		if (bytesData->getLength() == 0 && getSystemState()->mainClip->parsethread)
			getSystemState()->mainClip->parsethread->getSWFByteArray(bytesData.getPtr());
		return true;
	}
	else if (local_pt)
	{
		if (loadStatus >= LOAD_OPENED)
			local_pt->getSWFByteArray(bytesData.getPtr());
		return true;
	}
	else if (loader->getContent())
	{
		if (bytesData.isNull())
			bytesData = _NR<ByteArray>(Class<ByteArray>::getInstanceS(getInstanceWorker()));
		bytesData->writeObject(loader->getContent(),getInstanceWorker());
		return true;
	}
	return !bytesData.isNull();
}

ASFUNCTIONBODY_ATOM(LoaderInfo,_getBytes)
{
	LoaderInfo* th=asAtomHandler::as<LoaderInfo>(obj);

	if (th->fillBytesData(nullptr))
	{
		th->bytesData->incRef();
		ret = asAtomHandler::fromObject(th->bytesData.getPtr());
	}
	else
	{
		ret = asAtomHandler::nullAtom;
		return;
	}
}

ASFUNCTIONBODY_ATOM(LoaderInfo,_getApplicationDomain)
{
	LoaderInfo* th=asAtomHandler::as<LoaderInfo>(obj);

	if(th->applicationDomain.isNull())
	{
		asAtomHandler::setNull(ret);
		return;
	}

	th->applicationDomain->incRef();
	ret = asAtomHandler::fromObject(th->applicationDomain.getPtr());
}

ASFUNCTIONBODY_ATOM(LoaderInfo,_getWidth)
{
	LoaderInfo* th=asAtomHandler::as<LoaderInfo>(obj);

	if(!th->loader)
	{
		if (th == wrk->getSystemState()->mainClip->loaderInfo)
		{
			asAtomHandler::setInt(ret,wrk,wrk->getSystemState()->mainClip->getNominalWidth());
			return;
		}
		asAtomHandler::setInt(ret,wrk,0);
		return;
	}
	DisplayObject* o=th->loader->getContent();
	if (!o)
	{
		asAtomHandler::setInt(ret,wrk,0);
		return;
	}

	asAtomHandler::setInt(ret,wrk,o->getNominalWidth());
}

ASFUNCTIONBODY_ATOM(LoaderInfo,_getHeight)
{
	LoaderInfo* th=asAtomHandler::as<LoaderInfo>(obj);

	if(!th->loader)
	{
		if (th == wrk->getSystemState()->mainClip->loaderInfo)
		{
			asAtomHandler::setInt(ret,wrk,wrk->getSystemState()->mainClip->getNominalHeight());
			return;
		}
		asAtomHandler::setInt(ret,wrk,0);
		return;
	}
	DisplayObject* o=th->loader->getContent();
	if (!o)
	{
		asAtomHandler::setInt(ret,wrk,0);
		return;
	}

	asAtomHandler::setInt(ret,wrk,o->getNominalHeight());
}
