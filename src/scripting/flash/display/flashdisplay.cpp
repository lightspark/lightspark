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

#include "backends/security.h"
#include "scripting/abc.h"
#include "scripting/flash/display/flashdisplay.h"
#include "scripting/flash/display/NativeWindow.h"
#include "scripting/avm1/avm1display.h"
#include "scripting/flash/display/Graphics.h"
#include "swf.h"
#include "scripting/flash/geom/flashgeom.h"
#include "scripting/flash/system/flashsystem.h"
#include "parsing/streams.h"
#include "compat.h"
#include "scripting/class.h"
#include "backends/rendering.h"
#include "backends/geometry.h"
#include "backends/input.h"
#include "scripting/flash/accessibility/flashaccessibility.h"
#include "scripting/flash/media/flashmedia.h"
#include "scripting/flash/display/BitmapData.h"
#include "scripting/flash/net/flashnet.h"
#include "scripting/flash/ui/ContextMenu.h"
#include "scripting/flash/ui/keycodes.h"
#include "scripting/flash/display3d/flashdisplay3d.h"
#include "scripting/argconv.h"
#include "scripting/toplevel/Number.h"
#include "scripting/toplevel/Integer.h"
#include "scripting/toplevel/UInteger.h"
#include "scripting/toplevel/Vector.h"
#include "scripting/avm1/avm1text.h"
#include "parsing/tags.h"
#include <algorithm>

#define FRAME_NOT_FOUND 0xffffffff //Used by getFrameIdBy*

using namespace std;
using namespace lightspark;

std::ostream& lightspark::operator<<(std::ostream& s, const DisplayObject& r)
{
	s << "[" << r.getClass()->class_name << "]";
	if(r.name != BUILTIN_STRINGS::EMPTY)
		s << " name: " << r.name;
	return s;
}

LoaderInfo::LoaderInfo(ASWorker* wrk,Class_base* c):EventDispatcher(wrk,c),applicationDomain(NullRef),securityDomain(NullRef),
	contentType("application/x-shockwave-flash"),
	bytesLoaded(0),bytesLoadedPublic(0),bytesTotal(0),sharedEvents(NullRef),
	loader(nullptr),bytesData(NullRef),loadStatus(STARTED),actionScriptVersion(3),swfVersion(0),
	childAllowsParent(true),uncaughtErrorEvents(NullRef),parentAllowsChild(true),frameRate(0)
{
	subtype=SUBTYPE_LOADERINFO;
	sharedEvents=_MR(Class<EventDispatcher>::getInstanceS(wrk));
	parameters = _MR(Class<ASObject>::getInstanceS(wrk));
	uncaughtErrorEvents = _MR(Class<UncaughtErrorEvents>::getInstanceS(wrk));
	LOG(LOG_NOT_IMPLEMENTED,"LoaderInfo: childAllowsParent and parentAllowsChild always return true");
}

LoaderInfo::LoaderInfo(ASWorker* wrk, Class_base* c, Loader* l):EventDispatcher(wrk,c),applicationDomain(NullRef),securityDomain(NullRef),
	contentType("application/x-shockwave-flash"),
	bytesLoaded(0),bytesLoadedPublic(0),bytesTotal(0),sharedEvents(NullRef),
	loader(l),bytesData(NullRef),loadStatus(STARTED),actionScriptVersion(3),swfVersion(0),
	childAllowsParent(true),uncaughtErrorEvents(NullRef),parentAllowsChild(true),frameRate(0)
{
	subtype=SUBTYPE_LOADERINFO;
	sharedEvents=_MR(Class<EventDispatcher>::getInstanceS(wrk));
	parameters = _MR(Class<ASObject>::getInstanceS(wrk));
	uncaughtErrorEvents = _MR(Class<UncaughtErrorEvents>::getInstanceS(wrk));
	LOG(LOG_NOT_IMPLEMENTED,"LoaderInfo: childAllowsParent and parentAllowsChild always return true");
}

void LoaderInfo::sinit(Class_base* c)
{
	CLASS_SETUP(c, EventDispatcher, _constructor, CLASS_SEALED);
	c->isReusable = true;
	c->setDeclaredMethodByQName("loaderURL","",Class<IFunction>::getFunction(c->getSystemState(),_getLoaderURL,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("loader","",Class<IFunction>::getFunction(c->getSystemState(),_getLoader,0,Class<Loader>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("content","",Class<IFunction>::getFunction(c->getSystemState(),_getContent,0,Class<DisplayObject>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("url","",Class<IFunction>::getFunction(c->getSystemState(),_getURL,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("bytesLoaded","",Class<IFunction>::getFunction(c->getSystemState(),_getBytesLoaded,0,Class<UInteger>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("bytesTotal","",Class<IFunction>::getFunction(c->getSystemState(),_getBytesTotal,0,Class<UInteger>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("bytes","",Class<IFunction>::getFunction(c->getSystemState(),_getBytes,0,Class<ByteArray>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("applicationDomain","",Class<IFunction>::getFunction(c->getSystemState(),_getApplicationDomain,0,Class<ApplicationDomain>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("sharedEvents","",Class<IFunction>::getFunction(c->getSystemState(),_getSharedEvents,0,Class<EventDispatcher>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("width","",Class<IFunction>::getFunction(c->getSystemState(),_getWidth,0,Class<Integer>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("height","",Class<IFunction>::getFunction(c->getSystemState(),_getHeight,0,Class<Integer>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
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
	sharedEvents.reset();
	loader=nullptr;
	applicationDomain.reset();
	securityDomain.reset();
	waitedObject.reset();
	bytesData.reset();
	contentType = "application/x-shockwave-flash";
	bytesLoaded = 0;
	bytesLoadedPublic = 0;
	bytesTotal = 0;
	loadStatus =STARTED;
	actionScriptVersion = 3;
	swfVersion = 0;
	childAllowsParent = true;
	uncaughtErrorEvents.reset();
	parentAllowsChild =true;
	frameRate =0;
	parameters.reset();
	uncaughtErrorEvents.reset();
	progressEvent.reset();
	loaderevents.clear();
	return EventDispatcher::destruct();
}

void LoaderInfo::finalize()
{
	Locker l(spinlock);
	sharedEvents.reset();
	loader=nullptr;
	applicationDomain.reset();
	securityDomain.reset();
	waitedObject.reset();
	bytesData.reset();
	uncaughtErrorEvents.reset();
	parameters.reset();
	uncaughtErrorEvents.reset();
	progressEvent.reset();
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
	if (waitedObject)
		waitedObject->prepareShutdown();
	if (bytesData)
		bytesData->prepareShutdown();
	if (uncaughtErrorEvents)
		uncaughtErrorEvents->prepareShutdown();
	if (parameters)
		parameters->prepareShutdown();
	if (uncaughtErrorEvents)
		uncaughtErrorEvents->prepareShutdown();
	if (progressEvent)
		progressEvent->prepareShutdown();
}

void LoaderInfo::afterHandleEvent(Event* ev)
{
	Locker l(spinlock);
	auto it = loaderevents.find(ev);
	if (it != loaderevents.end())
	{
		this->loader->decRef();
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
			loaderevents.insert(ev);
		}
	}
}

void LoaderInfo::resetState()
{
	Locker l(spinlock);
	bytesLoaded=0;
	bytesLoadedPublic = 0;
	bytesTotal=0;
	if(!bytesData.isNull())
		bytesData->setLength(0);
	loadStatus=STARTED;
}

void LoaderInfo::setComplete()
{
	Locker l(spinlock);
	if (loadStatus==STARTED)
	{
		sendInit();
	}
}
void LoaderInfo::setContent(DisplayObject *o)
{
	if (loader)
	{
		loader->setContent(o);
	}
}
void LoaderInfo::setBytesLoaded(uint32_t b)
{
	if(b!=bytesLoaded)
	{
		Locker l(spinlock);
		bytesLoaded=b;
		if(getVm(getSystemState()))
		{
			// make sure that the event queue is not flooded with progressEvents
			if (progressEvent.isNull())
			{
				this->incRef();
				progressEvent = _MR(Class<ProgressEvent>::getInstanceS(getInstanceWorker(),bytesLoaded,bytesTotal));
				this->addLoaderEvent(progressEvent.getPtr());
				getVm(getSystemState())->addIdleEvent(_MR(this),progressEvent);
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
					this->incRef();
					this->addLoaderEvent(progressEvent.getPtr());
					getVm(getSystemState())->addIdleEvent(_MR(this),progressEvent);
				}
			}
		}
		if(bytesLoaded == bytesTotal && loadStatus==INIT_SENT)
		{
			//The clip is also complete now
			if(getVm(getSystemState()))
			{
				this->incRef();
				auto ev = Class<Event>::getInstanceS(getInstanceWorker(),"complete");
				this->addLoaderEvent(ev);
				getVm(getSystemState())->addIdleEvent(_MR(this),_MR(ev));
			}
			loadStatus=COMPLETE;
		}
	}
}

void LoaderInfo::sendInit()
{
	this->incRef();
	auto ev = Class<Event>::getInstanceS(getInstanceWorker(),"init");
	if (getVm(getSystemState())->addEvent(_MR(this),_MR(ev)))
		this->addLoaderEvent(ev);
	assert(loadStatus==STARTED);
	loadStatus=INIT_SENT;
	if(bytesTotal && bytesLoaded==bytesTotal)
	{
		//The clip is also complete now
		this->incRef();
		auto ev = Class<Event>::getInstanceS(getInstanceWorker(),"complete");
		if (getVm(getSystemState())->addIdleEvent(_MR(this),_MR(ev)))
			this->addLoaderEvent(ev);
		loadStatus=COMPLETE;
	}
}

void LoaderInfo::setWaitedObject(_NR<DisplayObject> w)
{
	Locker l(spinlock);
	waitedObject = w;
}

void LoaderInfo::objectHasLoaded(DisplayObject* obj)
{
	Locker l(spinlock);
	if(waitedObject != obj)
		return;
	if(loader && obj==waitedObject.getPtr())
		loader->setContent(obj);

	if (loader && !loader->getParent() && !loader->hasAVM1Target() && waitedObject->is<MovieClip>()) // loader has no parent, ensure init/complete events are sended anyway
		loader->getSystemState()->stage->addHiddenObject(waitedObject->as<MovieClip>());
		
	waitedObject.reset();
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
		parameters = _MR(Class<ASObject>::getInstanceS(getInstanceWorker()));
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

	asAtomHandler::setUInt(ret,wrk,th->bytesTotal);
}

ASFUNCTIONBODY_ATOM(LoaderInfo,_getBytes)
{
	LoaderInfo* th=asAtomHandler::as<LoaderInfo>(obj);

	if (th->bytesData.isNull())
		th->bytesData = _NR<ByteArray>(Class<ByteArray>::getInstanceS(wrk));
	if (!th->loader) //th is the LoaderInfo of the main clip
	{
		if (th->bytesData->getLength() == 0 && wrk->getSystemState()->mainClip->parsethread)
			wrk->getSystemState()->mainClip->parsethread->getSWFByteArray(th->bytesData.getPtr());
	}
	else if (th->loader->getContent())
		th->bytesData->writeObject(th->loader->getContent(),wrk);
	th->bytesData->incRef();
	ret = asAtomHandler::fromObject(th->bytesData.getPtr());
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
		if (th == wrk->getSystemState()->mainClip->loaderInfo.getPtr())
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
		if (th == wrk->getSystemState()->mainClip->loaderInfo.getPtr())
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

LoaderThread::LoaderThread(_R<URLRequest> request, _R<Loader> ldr)
  : DownloaderThreadBase(request, ldr.getPtr()), loader(ldr), loaderInfo(ldr->getContentLoaderInfo()), source(URL)
{
}

LoaderThread::LoaderThread(_R<ByteArray> _bytes, _R<Loader> ldr)
  : DownloaderThreadBase(NullRef, ldr.getPtr()), bytes(_bytes), loader(ldr), loaderInfo(ldr->getContentLoaderInfo()), source(BYTES)
{
}

void LoaderThread::execute()
{
	assert(source==URL || source==BYTES);

	streambuf *sbuf = 0;
	if(source==URL)
	{
		_R<MemoryStreamCache> cache(_MR(new MemoryStreamCache(loader->getSystemState())));
		if(!createDownloader(cache, loaderInfo, loaderInfo.getPtr(), false))
			return;

		sbuf = cache->createReader();
		
		// Wait for some data, making sure our check for failure is working
		sbuf->sgetc(); // peek one byte
		if(downloader->hasEmptyAnswer())
		{
			LOG(LOG_INFO,"empty answer:"<<url);
			return;
		}

		if(cache->hasFailed()) //Check to see if the download failed for some reason
		{
			LOG(LOG_ERROR, "Loader::execute(): Download of URL failed: " << url);
			auto ev = Class<IOErrorEvent>::getInstanceS(loader->getInstanceWorker());
			if (getVm(loader->getSystemState())->addEvent(loaderInfo,_MR(ev)))
				loaderInfo->addLoaderEvent(ev);

			getVm(loader->getSystemState())->addEvent(loader,_MR(Class<IOErrorEvent>::getInstanceS(loader->getInstanceWorker())));
			delete sbuf;
			// downloader will be deleted in jobFence
			return;
		}
		auto ev = Class<Event>::getInstanceS(loader->getInstanceWorker(),"open");
		if (getVm(loader->getSystemState())->addEvent(loaderInfo,_MR(ev)))
			loaderInfo->addLoaderEvent(ev);
	}
	else if(source==BYTES)
	{
		assert_and_throw(bytes->bytes);

		auto ev = Class<Event>::getInstanceS(loader->getInstanceWorker(),"open");
		if (getVm(loader->getSystemState())->addEvent(loaderInfo,_MR(ev)))
			loaderInfo->addLoaderEvent(ev);

		loaderInfo->setBytesTotal(bytes->getLength());
		loaderInfo->setBytesLoaded(bytes->getLength());

		sbuf = new bytes_buf(bytes->bytes,bytes->getLength());

// extract embedded swf to separate file
//		char* name_used=nullptr;
//		int fd = g_file_open_tmp("lightsparkXXXXXX.swf",&name_used,nullptr);
//		write(fd,bytes->bytes,bytes->getLength());
//		close(fd);
//		g_free(name_used);
	}

	istream s(sbuf);
	ParseThread local_pt(s,loaderInfo->applicationDomain,loaderInfo->securityDomain,loader.getPtr(),url.getParsedURL());
	local_pt.execute();

	// Delete the bytes container (cache reader or bytes_buf)
	delete sbuf;
	sbuf = nullptr;
	if (source==URL) {
		//Acquire the lock to ensure consistency in threadAbort
		Locker l(downloaderLock);
		if(downloader)
			loaderInfo->getSystemState()->downloadManager->destroy(downloader);
		downloader=nullptr;
	}

	bytes.reset();

	_NR<DisplayObject> obj=local_pt.getParsedObject();
	if(obj.isNull())
	{
		// The stream did not contain RootMovieClip or Bitmap
		if(!threadAborting)
		{
			auto ev = Class<IOErrorEvent>::getInstanceS(loader->getInstanceWorker());
			if (getVm(loader->getSystemState())->addEvent(loaderInfo,_MR(ev)))
				loaderInfo->addLoaderEvent(ev);
		}
		return;
	}
	if (loader.getPtr() && local_pt.getRootMovie() && local_pt.getRootMovie()->hasFinishedLoading())
	{
		if (local_pt.getRootMovie() != loader->getSystemState()->mainClip && !local_pt.getRootMovie()->hasMainClass)
		{
			if (!local_pt.getRootMovie()->usesActionScript3)
				local_pt.getRootMovie()->setIsInitialized(false);
			local_pt.getRootMovie()->incRef();
			loader->setContent(local_pt.getRootMovie());
		}
	}
}

ASFUNCTIONBODY_ATOM(Loader,_constructor)
{
	Loader* th=asAtomHandler::as<Loader>(obj);
	DisplayObjectContainer::_constructor(ret,wrk,obj,nullptr,0);
	if (!th->contentLoaderInfo)
		th->contentLoaderInfo=_MR(Class<LoaderInfo>::getInstanceS(wrk,th));
	th->contentLoaderInfo->setLoaderURL(th->getSystemState()->mainClip->getOrigin().getParsedURL());
	th->uncaughtErrorEvents = _MR(Class<UncaughtErrorEvents>::getInstanceS(wrk));
}

ASFUNCTIONBODY_ATOM(Loader,_getContent)
{
	Loader* th=asAtomHandler::as<Loader>(obj);
	Locker l(th->spinlock);
	ASObject* res=th->content;
	if(!res)
	{
		asAtomHandler::setUndefined(ret);
		return;
	}

	res->incRef();
	ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(Loader,_getContentLoaderInfo)
{
	Loader* th=asAtomHandler::as<Loader>(obj);
	th->contentLoaderInfo->incRef();
	ret = asAtomHandler::fromObject(th->contentLoaderInfo.getPtr());
}

ASFUNCTIONBODY_ATOM(Loader,close)
{
	Loader* th=asAtomHandler::as<Loader>(obj);
 	Locker l(th->spinlock);
	for (auto j=th->jobs.begin(); j!=th->jobs.end(); j++)
		(*j)->threadAbort();
}

ASFUNCTIONBODY_ATOM(Loader,load)
{
	Loader* th=asAtomHandler::as<Loader>(obj);

	th->unload();
	_NR<URLRequest> r;
	_NR<LoaderContext> context;
	ARG_CHECK(ARG_UNPACK (r)(context, NullRef));
	th->loadIntern(r.getPtr(),context.getPtr());
}
void Loader::loadIntern(URLRequest* r, LoaderContext* context)
{
	this->url=r->getRequestURL();
	this->contentLoaderInfo->setURL(this->url.getParsedURL());
	this->contentLoaderInfo->resetState();
	//Check if a security domain has been manually set
	SecurityDomain* secDomain=nullptr;
	SecurityDomain* curSecDomain=getInstanceWorker()->rootClip->securityDomain.getPtr();
	if(context)
	{
		if (context->securityDomain)
		{
			//The passed domain must be the current one. See Loader::load specs.
			if(context->securityDomain!=curSecDomain)
			{
				createError<SecurityError>(this->getInstanceWorker(),0,"SecurityError: securityDomain must be current one");
				return;
			}
			secDomain=curSecDomain;
		}

		bool sameDomain = (secDomain == curSecDomain);
		this->allowCodeImport = !sameDomain || context->getAllowCodeImport();

		if (!context->parameters.isNull())
			this->contentLoaderInfo->setParameters(context->parameters);
	}
	//Default is to create a child ApplicationDomain if the file is in the same security context
	//otherwise create a child of the system domain. If the security domain is different
	//the passed applicationDomain is ignored
	RootMovieClip* currentRoot=getInstanceWorker()->rootClip.getPtr();
	// empty origin is possible if swf is loaded by loadBytes()
	if(currentRoot->getOrigin().isEmpty() || currentRoot->getOrigin().getHostname()==this->url.getHostname() || secDomain)
	{
		//Same domain
		_NR<ApplicationDomain> parentDomain = currentRoot->applicationDomain;
		//Support for LoaderContext
		if(!context || context->applicationDomain.isNull())
			this->contentLoaderInfo->applicationDomain = _MR(Class<ApplicationDomain>::getInstanceS(this->getInstanceWorker(),parentDomain));
		else
			this->contentLoaderInfo->applicationDomain = context->applicationDomain;
		curSecDomain->incRef();
		this->contentLoaderInfo->securityDomain = _NR<SecurityDomain>(curSecDomain);
	}
	else
	{
		//Different domain
		_NR<ApplicationDomain> parentDomain = _MR(this->getSystemState()->systemDomain);
		this->contentLoaderInfo->applicationDomain = _MR(Class<ApplicationDomain>::getInstanceS(this->getInstanceWorker(),parentDomain));
		this->contentLoaderInfo->securityDomain = _MR(Class<SecurityDomain>::getInstanceS(this->getInstanceWorker()));
	}

	if(!this->url.isValid())
	{
		//Notify an error during loading
		this->incRef();
		this->getSystemState()->currentVm->addEvent(_MR(this),_MR(Class<IOErrorEvent>::getInstanceS(this->getInstanceWorker())));
		return;
	}

	SecurityManager::checkURLStaticAndThrow(this->url, ~(SecurityManager::LOCAL_WITH_FILE),
		SecurityManager::LOCAL_WITH_FILE | SecurityManager::LOCAL_TRUSTED, true);

	if (context && context->getCheckPolicyFile())
	{
		//TODO: this should be async as it could block if invoked from ExternalInterface
		SecurityManager::EVALUATIONRESULT evaluationResult;
		evaluationResult = this->getSystemState()->securityManager->evaluatePoliciesURL(this->url, true);
		if(evaluationResult == SecurityManager::NA_CROSSDOMAIN_POLICY)
		{
			// should this dispatch SecurityErrorEvent instead of throwing?
			createError<SecurityError>(this->getInstanceWorker(),0,
				"SecurityError: connection to domain not allowed by securityManager");
			return;
		}
	}

	this->incRef();
	r->incRef();
	LoaderThread *thread=new LoaderThread(_MR(r), _MR(this));

	Locker l(this->spinlock);
	this->jobs.push_back(thread);
	this->getSystemState()->addJob(thread);
}

ASFUNCTIONBODY_ATOM(Loader,loadBytes)
{
	Loader* th=asAtomHandler::as<Loader>(obj);


	th->unload();

	_NR<ByteArray> bytes;
	_NR<LoaderContext> context;
	ARG_CHECK(ARG_UNPACK (bytes)(context, NullRef));

	_NR<ApplicationDomain> parentDomain = ABCVm::getCurrentApplicationDomain(wrk->currentCallContext);
	if(context.isNull() || context->applicationDomain.isNull())
		th->contentLoaderInfo->applicationDomain = _MR(Class<ApplicationDomain>::getInstanceS(wrk,parentDomain));
	else
		th->contentLoaderInfo->applicationDomain = context->applicationDomain;
	//Always loaded in the current security domain
	_NR<SecurityDomain> curSecDomain=ABCVm::getCurrentSecurityDomain(wrk->currentCallContext);
	th->contentLoaderInfo->securityDomain = curSecDomain;

	th->allowCodeImport = context.isNull() || context->getAllowCodeImport();

	if (!context.isNull() && !context->parameters.isNull())
		th->contentLoaderInfo->setParameters(context->parameters);

	if(bytes->getLength()!=0)
	{
		th->incRef();
		// better work on a copy of the source bytearray as it may be modified by actionscript before loading is completed
		ByteArray* b = Class<ByteArray>::getInstanceSNoArgs(wrk);
		b->writeBytes(bytes->getBufferNoCheck(),bytes->getLength());
		bytes = _MR(b);

		LoaderThread *thread=new LoaderThread(_MR(bytes), _MR(th));
		Locker l(th->spinlock);
		th->jobs.push_back(thread);
		wrk->getSystemState()->addJob(thread);
	}
	else
		LOG(LOG_INFO, "Empty ByteArray passed to Loader.loadBytes");
}

ASFUNCTIONBODY_ATOM(Loader,_unload)
{
	Loader* th=asAtomHandler::as<Loader>(obj);
	th->unload();
}
ASFUNCTIONBODY_ATOM(Loader,_unloadAndStop)
{
	Loader* th=asAtomHandler::as<Loader>(obj);
	th->unload();
	LOG(LOG_NOT_IMPLEMENTED,"unloadAndStop does not execute any stopping actions");
	/* TODO: (taken from specs)
	Sounds are stopped.
	Stage event listeners are removed.
	Event listeners for enterFrame, frameConstructed, exitFrame, activate and deactivate are removed.
	Timers are stopped.
	Camera and Microphone instances are detached
	Movie clips are stopped.
	*/
}

void Loader::unload()
{
	DisplayObject* content_copy = nullptr;
	{
		Locker l(spinlock);
		for (auto j=jobs.begin(); j!=jobs.end(); j++)
			(*j)->threadAbort();

		content_copy=content;
		
		content = nullptr;
	}
	
	if(loaded)
	{
		auto ev = Class<Event>::getInstanceS(getInstanceWorker(),"unload");
		if (getVm(getSystemState())->addEvent(contentLoaderInfo,_MR(ev)))
			contentLoaderInfo->addLoaderEvent(ev);
		loaded=false;
	}

	// removeChild may execute AS code, release the lock before
	// calling
	if(content_copy)
	{
		_removeChild(content_copy);
		content_copy->removeStoredMember();
	}

	contentLoaderInfo->resetState();
}

void Loader::finalize()
{
	DisplayObjectContainer::finalize();
	if (content)
		content->removeStoredMember();
	content=nullptr;
	contentLoaderInfo.reset();
	avm1target.reset();
	uncaughtErrorEvents.reset();
}
bool Loader::destruct()
{
	url = URLInfo();
	loaded=false;
	allowCodeImport=true;
	if (content)
		content->removeStoredMember();
	content=nullptr;
	contentLoaderInfo.reset();
	avm1target.reset();
	uncaughtErrorEvents.reset();
	return DisplayObjectContainer::destruct();
}

bool Loader::countCylicMemberReferences(garbagecollectorstate& gcstate)
{
	if (gcstate.checkAncestors(this))
		return false;
	bool ret = DisplayObjectContainer::countCylicMemberReferences(gcstate);
	Locker l(mutexDisplayList);
	if (content)
		ret = content->countAllCylicMemberReferences(gcstate) || ret;
	return ret;
}
void Loader::prepareShutdown()
{
	if (preparedforshutdown)
		return;
	DisplayObjectContainer::prepareShutdown();
	if (content)
		content->prepareShutdown();
	if (contentLoaderInfo)
		contentLoaderInfo->prepareShutdown();
	if (avm1target)
		avm1target->prepareShutdown();
	if (uncaughtErrorEvents)
		uncaughtErrorEvents->prepareShutdown();
}
Loader::Loader(ASWorker* wrk, Class_base* c):DisplayObjectContainer(wrk,c),content(nullptr),contentLoaderInfo(NullRef),loaded(false), allowCodeImport(true),uncaughtErrorEvents(NullRef)
{
	contentLoaderInfo=_MR(Class<LoaderInfo>::getInstanceS(wrk,this));
}

Loader::~Loader()
{
}

void Loader::sinit(Class_base* c)
{
	CLASS_SETUP(c, DisplayObjectContainer, _constructor, CLASS_SEALED);
	c->isReusable=true;
	c->setDeclaredMethodByQName("contentLoaderInfo","",Class<IFunction>::getFunction(c->getSystemState(),_getContentLoaderInfo,0,Class<LoaderInfo>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("content","",Class<IFunction>::getFunction(c->getSystemState(),_getContent,0,Class<DisplayObject>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("close","",Class<IFunction>::getFunction(c->getSystemState(),close),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("loadBytes","",Class<IFunction>::getFunction(c->getSystemState(),loadBytes),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("load","",Class<IFunction>::getFunction(c->getSystemState(),load),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("unload","",Class<IFunction>::getFunction(c->getSystemState(),_unload),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("unloadAndStop","",Class<IFunction>::getFunction(c->getSystemState(),_unloadAndStop),NORMAL_METHOD,true);
	REGISTER_GETTER_RESULTTYPE(c,uncaughtErrorEvents,UncaughtErrorEvents);
}

ASFUNCTIONBODY_GETTER(Loader,uncaughtErrorEvents)

void Loader::threadFinished(IThreadJob* finishedJob)
{
	Locker l(spinlock);
	jobs.remove(finishedJob);
	delete finishedJob;
}

void Loader::setContent(DisplayObject* o)
{
	// content may have already been set.
	// this can happen if setContent was already called from ObjectHasLoaded
	// and is called again at the end of LoaderThread::execute
	if (o->getParent() == this || (!avm1target.isNull() && o->getParent()))
		return;
	{
		Locker l(mutexDisplayList);
		clearDisplayList();
	}

	{
		Locker l(spinlock);
		o->incRef();
		o->addStoredMember();
		content=o;
		content->isLoadedRoot = true;
		loaded=true;
	}
	// _addChild may cause AS code to run, release locks beforehand.

	if (!avm1target.isNull())
	{
		o->tx = avm1target->tx;
		o->ty = avm1target->ty;
		o->tz = avm1target->tz;
		o->rotation = avm1target->rotation;
		o->sx = avm1target->sx;
		o->sy = avm1target->sy;
		o->sz = avm1target->sz;
		DisplayObjectContainer* p = avm1target->getParent();
		if (p)
		{
			int depth=0;
			if (p->is<Stage>())
			{
				p->_removeChild(avm1target.getPtr());
			}
			else
			{
				depth =p->findLegacyChildDepth(avm1target.getPtr());
				p->deleteLegacyChildAt(depth,false);
			}
			o->incRef();
			p->_addChildAt(o,depth);
		}
	}
	else
	{
		o->incRef();
		_addChildAt(o, 0);
	}
	if (!o->loaderInfo.isNull())
		o->loaderInfo->setComplete();
}

Sprite::Sprite(ASWorker* wrk, Class_base* c):DisplayObjectContainer(wrk,c),TokenContainer(this),graphics(NullRef),soundstartframe(UINT32_MAX),streamingsound(false),hasMouse(false),dragged(false),buttonMode(false),useHandCursor(true)
{
	subtype=SUBTYPE_SPRITE;
}

bool Sprite::destruct()
{
	resetToStart();
	graphics.reset();
	hitArea.reset();
	hitTarget.reset();
	tokens.clear();
	dragged = false;
	buttonMode = false;
	useHandCursor = true;
	streamingsound=false;
	hasMouse=false;
	tokens.clear();
	sound.reset();
	soundtransform.reset();
	return DisplayObjectContainer::destruct();
}

void Sprite::finalize()
{
	resetToStart();
	graphics.reset();
	hitArea.reset();
	hitTarget.reset();
	tokens.clear();
	sound.reset();
	soundtransform.reset();
	DisplayObjectContainer::finalize();
}

void Sprite::prepareShutdown()
{
	if (preparedforshutdown)
		return;
	DisplayObjectContainer::prepareShutdown();
	if (graphics)
		graphics->prepareShutdown();
	if (hitArea)
		hitArea->prepareShutdown();
	if (hitTarget)
		hitTarget->prepareShutdown();
	if (sound)
		sound->prepareShutdown();
	if (soundtransform)
		soundtransform->prepareShutdown();
}

void Sprite::startDrawJob()
{
	if (graphics)
		graphics->startDrawJob();
}

void Sprite::endDrawJob()
{
	if (graphics)
		graphics->endDrawJob();
}

void Sprite::sinit(Class_base* c)
{
	CLASS_SETUP(c, DisplayObjectContainer, _constructor, CLASS_SEALED);
	c->isReusable = true;
	c->setDeclaredMethodByQName("graphics","",Class<IFunction>::getFunction(c->getSystemState(),_getGraphics,0,Class<Graphics>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("startDrag","",Class<IFunction>::getFunction(c->getSystemState(),_startDrag),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("stopDrag","",Class<IFunction>::getFunction(c->getSystemState(),_stopDrag),NORMAL_METHOD,true);
	REGISTER_GETTER_SETTER_RESULTTYPE(c, buttonMode,Boolean);
	REGISTER_GETTER_SETTER_RESULTTYPE(c, hitArea,Sprite);
	REGISTER_GETTER_SETTER_RESULTTYPE(c, useHandCursor,Boolean);
	c->setDeclaredMethodByQName("soundTransform","",Class<IFunction>::getFunction(c->getSystemState(),getSoundTransform,0,Class<SoundTransform>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("soundTransform","",Class<IFunction>::getFunction(c->getSystemState(),setSoundTransform),SETTER_METHOD,true);
}

ASFUNCTIONBODY_GETTER_SETTER(Sprite, buttonMode)
ASFUNCTIONBODY_GETTER_SETTER_CB(Sprite, useHandCursor,afterSetUseHandCursor)

void Sprite::afterSetUseHandCursor(bool /*oldValue*/)
{
	handleMouseCursor(hasMouse);
}

IDrawable* Sprite::invalidate(DisplayObject* target, const MATRIX& initialMatrix, bool smoothing, InvalidateQueue* q, _NR<DisplayObject>* cachedBitmap)
{
	if (this->graphics)
		this->graphics->refreshTokens();
	return TokenContainer::invalidate(target, initialMatrix,smoothing ? SMOOTH_MODE::SMOOTH_ANTIALIAS : SMOOTH_MODE::SMOOTH_NONE,q,cachedBitmap,true);
}

ASFUNCTIONBODY_ATOM(Sprite,_startDrag)
{
	Sprite* th=asAtomHandler::as<Sprite>(obj);
	bool lockCenter = false;
	const RECT* bounds = nullptr;
	ARG_CHECK(ARG_UNPACK(lockCenter,false));
	if(argslen > 1)
	{
		Rectangle* rect = Class<Rectangle>::cast(asAtomHandler::getObject(args[1]));
		if(!rect)
		{
			createError<ArgumentError>(wrk,kInvalidArgumentError,"Wrong type");
			return;
		}
		bounds = new RECT(rect->getRect());
	}

	Vector2f offset;
	if(!lockCenter)
	{
		offset = -th->getParent()->getLocalMousePos();
		offset += th->getXY();
	}

	th->incRef();
	wrk->getSystemState()->getInputThread()->startDrag(_MR(th), bounds, offset);
}

ASFUNCTIONBODY_ATOM(Sprite,_stopDrag)
{
	Sprite* th=asAtomHandler::as<Sprite>(obj);
	wrk->getSystemState()->getInputThread()->stopDrag(th);
}

ASFUNCTIONBODY_GETTER(Sprite, hitArea)

ASFUNCTIONBODY_ATOM(Sprite,_setter_hitArea)
{
	Sprite* th=asAtomHandler::as<Sprite>(obj);
	_NR<Sprite> value;
	ARG_CHECK(ARG_UNPACK(value));

	if (!th->hitArea.isNull())
		th->hitArea->hitTarget.reset();

	th->hitArea = value;
	if (!th->hitArea.isNull())
	{
		th->incRef();
		th->hitArea->hitTarget = _MNR(th);
	}
}

ASFUNCTIONBODY_ATOM(Sprite,getSoundTransform)
{
	Sprite* th=asAtomHandler::as<Sprite>(obj);
	if (th->sound && th->sound->soundTransform)
	{
		ret = asAtomHandler::fromObject(th->sound->soundTransform.getPtr());
		ASATOM_INCREF(ret);
	}
	else
		asAtomHandler::setNull(ret);
	if (!th->soundtransform)
	{
		if (th->sound)
			th->soundtransform = th->sound->soundTransform;
	}
	if (!th->soundtransform)
	{
		th->soundtransform = _MR(Class<SoundTransform>::getInstanceSNoArgs(wrk));
		if (th->sound)
			th->sound->soundTransform = th->soundtransform;
	}
	ret = asAtomHandler::fromObject(th->soundtransform.getPtr());
	ASATOM_INCREF(ret);
}
ASFUNCTIONBODY_ATOM(Sprite,setSoundTransform)
{
	Sprite* th=asAtomHandler::as<Sprite>(obj);
	if (argslen == 0 || !asAtomHandler::is<SoundTransform>(args[0]))
		return;
	ASATOM_INCREF(args[0]);
	th->soundtransform =  _MR(asAtomHandler::getObject(args[0])->as<SoundTransform>());
}

bool DisplayObjectContainer::boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax)
{
	bool ret = false;

	if(dynamicDisplayList.empty())
		return false;
	if (!boundsrectdirty)
	{
		xmin = boundsrectXmin;
		ymin = boundsrectYmin;
		xmax = boundsrectXmax;
		ymax = boundsrectYmax;
		return true;
	}

	Locker l(mutexDisplayList);
	auto it=dynamicDisplayList.begin();
	for(;it!=dynamicDisplayList.end();++it)
	{
		number_t txmin,txmax,tymin,tymax;
		if((*it)->getBounds(txmin,txmax,tymin,tymax,(*it)->getMatrix()))
		{
			if(ret==true)
			{
				xmin = min(xmin,txmin);
				xmax = max(xmax,txmax);
				ymin = min(ymin,tymin);
				ymax = max(ymax,tymax);
			}
			else
			{
				xmin=txmin;
				xmax=txmax;
				ymin=tymin;
				ymax=tymax;
				ret=true;
			}
		}
	}
	boundsrectXmin=xmin;
	boundsrectYmin=ymin;
	boundsrectXmax=xmax;
	boundsrectYmax=ymax;
	boundsrectdirty=false;
	return ret;
}

bool Sprite::boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax)
{
	bool ret;
	ret = DisplayObjectContainer::boundsRect(xmin,xmax,ymin,ymax);
	number_t txmin,txmax,tymin,tymax;
	if (graphics)
		graphics->refreshTokens();
	if(TokenContainer::boundsRect(txmin,txmax,tymin,tymax))
	{
		if(ret==true)
		{
			xmin = min(xmin,txmin);
			xmax = max(xmax,txmax);
			ymin = min(ymin,tymin);
			ymax = max(ymax,tymax);
		}
		else
		{
			xmin=txmin;
			xmax=txmax;
			ymin=tymin;
			ymax=tymax;
		}
		ret=true;
	}
	return ret;
}

void Sprite::requestInvalidation(InvalidateQueue* q, bool forceTextureRefresh)
{
	if (requestInvalidationForCacheAsBitmap(q))
		return;
	DisplayObjectContainer::requestInvalidation(q,forceTextureRefresh);
	if (graphics)
	{
		if(skipRender())
			return;
		incRef();
		if (forceTextureRefresh)
			setNeedsTextureRecalculation();
		q->addToInvalidateQueue(_MR(this));
	}
	else
		TokenContainer::requestInvalidation(q,forceTextureRefresh);
}

bool DisplayObjectContainer::renderImpl(RenderContext& ctxt)
{
	bool renderingfailed = false;
	if (computeCacheAsBitmap(false) && ctxt.contextType == RenderContext::GL)
	{
		_NR<DisplayObject> d=getCachedBitmap(); // this ensures bitmap is not destructed during rendering
		if (d)
			d->Render(ctxt);
		return renderingfailed;
	}
	Locker l(mutexDisplayList);
	//Now draw also the display list
	auto it=dynamicDisplayList.begin();
	for(;it!=dynamicDisplayList.end();++it)
	{
		(*it)->Render(ctxt);
	}
	return renderingfailed;
}

void DisplayObjectContainer::LegacyChildEraseDeletionMarked()
{
	auto it = legacyChildrenMarkedForDeletion.begin();
	while (it != legacyChildrenMarkedForDeletion.end())
	{
		deleteLegacyChildAt(*it,false);
		it = legacyChildrenMarkedForDeletion.erase(it);
	}
	namedRemovedLegacyChildren.clear();
}

bool DisplayObjectContainer::LegacyChildRemoveDeletionMark(int32_t depth)
{
	auto it = legacyChildrenMarkedForDeletion.find(depth);
	if (it != legacyChildrenMarkedForDeletion.end())
	{
		legacyChildrenMarkedForDeletion.erase(it);
		return true;
	}
	return false;
}

DisplayObject *DisplayObjectContainer::findRemovedLegacyChild(uint32_t name)
{
	auto it = namedRemovedLegacyChildren.find(name);
	return (it == namedRemovedLegacyChildren.end() ? nullptr : (*it).second.getPtr());
}

void DisplayObjectContainer::eraseRemovedLegacyChild(uint32_t name)
{
	namedRemovedLegacyChildren.erase(name);
}

bool Sprite::renderImpl(RenderContext& ctxt)
{
	if (this->graphics)
		this->graphics->refreshTokens();
	//Draw the dynamically added graphics, if any
	bool ret = TokenContainer::renderImpl(ctxt);

	bool ret2 =DisplayObjectContainer::renderImpl(ctxt);
	return ret && ret2;
}

_NR<DisplayObject> DisplayObjectContainer::hitTestImpl(number_t x, number_t y, DisplayObject::HIT_TYPE type,bool interactiveObjectsOnly)
{
	_NR<DisplayObject> ret = NullRef;
	bool hit_this=false;
	//Test objects added at runtime, in reverse order
	Locker l(mutexDisplayList);
	auto j=dynamicDisplayList.rbegin();
	for(;j!=dynamicDisplayList.rend();++j)
	{
		//Don't check masks
		if((*j)->isMask())
			continue;

		if(!(*j)->getMatrix().isInvertible())
			continue; /* The object is shrunk to zero size */

		number_t localX, localY;
		(*j)->getMatrix().getInverted().multiply2D(x,y,localX,localY);
		ret=(*j)->hitTest(localX,localY,type,interactiveObjectsOnly);
		
		if (!ret.isNull())
		{
			if (interactiveObjectsOnly)
			{
				if (!mouseChildren) // When mouseChildren is false, we should get all events of our children
				{
					if (mouseEnabled)
					{
						this->incRef();
						ret =_MNR(this);
					}
					else
					{
						ret.reset();
						continue;
					}
				}
				else
				{
					if (mouseEnabled)
					{
						if (!ret->is<InteractiveObject>())
						{
							// we have hit a non-interactive object, so "this" may be the hit target
							// but we continue to search the children as there may be an InteractiveObject that is also hit
							hit_this=true;
							ret.reset();
							continue;
						}
						else if (!ret->as<InteractiveObject>()->isHittable(type))
						{
							// hit is a disabled InteractiveObject, so so "this" may be the hit target
							// but we continue to search the children as there may be an enabled InteractiveObject that is also hit
							hit_this=true;
							ret.reset();
							continue;
						}
					}
				}
			}
			break;
		}
	}
	if (hit_this && ret.isNull())
	{
		this->incRef();
		ret =_MNR(this);
	}
	return ret;
}

_NR<DisplayObject> Sprite::hitTestImpl(number_t x, number_t y, DisplayObject::HIT_TYPE type,bool interactiveObjectsOnly)
{
	//Did we hit a children?
	_NR<DisplayObject> ret = NullRef;
	if (dragged) // no hitting when in drag/drop mode
		return ret;
	if (!hitArea.isNull() && interactiveObjectsOnly)
	{
		number_t xout,yout,xout2,yout2;
		localToGlobal(x,y,xout,yout);
		hitArea->globalToLocal(xout,yout,xout2,yout2);
		ret = hitArea->hitTestImpl(xout2,yout2, type,interactiveObjectsOnly);
		if (!ret.isNull())
		{
			this->incRef();
			ret = _MR(this);
		}
		return ret;
	}
	ret = DisplayObjectContainer::hitTestImpl(x,y, type,interactiveObjectsOnly);
	if (ret.isNull() && hitArea.isNull())
	{
		//The coordinates are locals
		if (TokenContainer::hitTestImpl(x,y))
		{
			this->incRef();
			ret = _MR(this);
		}

		if (!ret.isNull())  // we hit the sprite?
		{
			if (!hitTarget.isNull())
			{
				//Another Sprite has registered us
				//as its hitArea -> relay the hit
				ret = hitTarget;
			}
		}
	}

	return ret;
}

void Sprite::resetToStart()
{
	if (sound && this->getTagID() != UINT32_MAX)
	{
		sound->threadAbort();
	}
}

void Sprite::setSound(SoundChannel *s,bool forstreaming)
{
	sound = _MR(s);
	streamingsound = forstreaming;
	if (sound->soundTransform)
		this->soundtransform = sound->soundTransform;
	else
		sound->soundTransform = this->soundtransform;
}

void Sprite::appendSound(unsigned char *buf, int len, uint32_t frame)
{
	if (sound)
		sound->appendStreamBlock(buf,len);
	if (soundstartframe == UINT32_MAX)
		soundstartframe=frame;
}

void Sprite::checkSound(uint32_t frame)
{
	if (sound && streamingsound && soundstartframe==frame)
		sound->play();
}

void Sprite::stopSound()
{
	if (sound)
		sound->threadAbort();
}

void Sprite::markSoundFinished()
{
	if (sound)
		sound->markFinished();
}

ASFUNCTIONBODY_ATOM(Sprite,_constructor)
{
	//Sprite* th=Class<Sprite>::cast(obj);
	DisplayObjectContainer::_constructor(ret,wrk,obj,nullptr,0);
}

_NR<Graphics> Sprite::getGraphics()
{
	//Probably graphics is not used often, so create it here
	if(graphics.isNull())
		graphics=_MR(Class<Graphics>::getInstanceS(getInstanceWorker(),this));
	return graphics;
}

void Sprite::handleMouseCursor(bool rollover)
{
	if (rollover)
	{
		hasMouse=true;
		if (buttonMode)
			getSystemState()->setMouseHandCursor(this->useHandCursor);
	}
	else
	{
		getSystemState()->setMouseHandCursor(false);
		hasMouse=false;
	}
}

ASFUNCTIONBODY_ATOM(Sprite,_getGraphics)
{
	Sprite* th=asAtomHandler::as<Sprite>(obj);
	_NR<Graphics> g = th->getGraphics();

	g->incRef();
	ret = asAtomHandler::fromObject(g.getPtr());
}

FrameLabel::FrameLabel(ASWorker* wrk,Class_base* c):ASObject(wrk,c)
{
}

FrameLabel::FrameLabel(ASWorker* wrk, Class_base* c, const FrameLabel_data& data):ASObject(wrk,c),FrameLabel_data(data)
{
}

void FrameLabel::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setDeclaredMethodByQName("frame","",Class<IFunction>::getFunction(c->getSystemState(),_getFrame),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("name","",Class<IFunction>::getFunction(c->getSystemState(),_getName),GETTER_METHOD,true);
}

ASFUNCTIONBODY_ATOM(FrameLabel,_getFrame)
{
	FrameLabel* th=asAtomHandler::as<FrameLabel>(obj);
	asAtomHandler::setUInt(ret,wrk,th->frame);
}

ASFUNCTIONBODY_ATOM(FrameLabel,_getName)
{
	FrameLabel* th=asAtomHandler::as<FrameLabel>(obj);
	ret = asAtomHandler::fromObject(abstract_s(wrk,th->name));
}

/*
 * Adds a frame label to the internal vector and keep
 * the vector sorted with respect to frame
 */
void Scene_data::addFrameLabel(uint32_t frame, const tiny_string& label)
{
	for(vector<FrameLabel_data>::iterator j=labels.begin();
		j != labels.end();++j)
	{
		FrameLabel_data& fl = *j;
		if(fl.frame == frame)
		{
			LOG(LOG_INFO,"existing frame label found:"<<fl.name<<", new value:"<<label);
			fl.name = label;
			return;
		}
		else if(fl.frame > frame)
		{
			labels.insert(j,FrameLabel_data(frame,label));
			return;
		}
	}

	labels.push_back(FrameLabel_data(frame,label));
}

Scene::Scene(ASWorker* wrk,Class_base* c):ASObject(wrk,c)
{
}

Scene::Scene(ASWorker* wrk, Class_base* c, const Scene_data& data, uint32_t _numFrames):ASObject(wrk,c),Scene_data(data),numFrames(_numFrames)
{
}

void Scene::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setDeclaredMethodByQName("labels","",Class<IFunction>::getFunction(c->getSystemState(),_getLabels,0,Class<Array>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("name","",Class<IFunction>::getFunction(c->getSystemState(),_getName,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("numFrames","",Class<IFunction>::getFunction(c->getSystemState(),_getNumFrames,0,Class<Integer>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
}

ASFUNCTIONBODY_ATOM(Scene,_getLabels)
{
	Scene* th=asAtomHandler::as<Scene>(obj);
	Array* res = Class<Array>::getInstanceSNoArgs(wrk);
	res->resize(th->labels.size());
	for(size_t i=0; i<th->labels.size(); ++i)
	{
		asAtom v = asAtomHandler::fromObject(Class<FrameLabel>::getInstanceS(wrk,th->labels[i]));
		res->set(i, v,false,false);
	}
	ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(Scene,_getName)
{
	Scene* th=asAtomHandler::as<Scene>(obj);
	ret = asAtomHandler::fromObject(abstract_s(wrk,th->name));
}

ASFUNCTIONBODY_ATOM(Scene,_getNumFrames)
{
	Scene* th=asAtomHandler::as<Scene>(obj);
	ret = asAtomHandler::fromUInt(th->numFrames);
}

void Frame::destroyTags()
{
	auto it=blueprint.begin();
	for(;it!=blueprint.end();++it)
		delete (*it);
}

void Frame::execute(DisplayObjectContainer* displayList, bool inskipping)
{
	auto it=blueprint.begin();
	for(;it!=blueprint.end();++it)
		(*it)->execute(displayList,inskipping);
	displayList->checkClipDepth();
}
void Frame::AVM1executeActions(MovieClip* clip)
{
	auto it=blueprint.begin();
	for(;it!=blueprint.end();++it)
	{
		if ((*it)->getType() == AVM1ACTION_TAG)
			(*it)->execute(clip,false);
	}
}

FrameContainer::FrameContainer():framesLoaded(0)
{
	frames.emplace_back(Frame());
	scenes.resize(1);
}

FrameContainer::FrameContainer(const FrameContainer& f):frames(f.frames),scenes(f.scenes),framesLoaded((int)f.framesLoaded)
{
}

/* This runs in parser thread context,
 * but no locking is needed here as it only accesses the last frame.
 * See comment on the 'frames' member. */
void FrameContainer::addToFrame(DisplayListTag* t)
{
	frames.back().blueprint.push_back(t);
}
/**
 * Find the scene to which the given frame belongs and
 * adds the frame label to that scene.
 * The labels of the scene will stay sorted by frame.
 */
void FrameContainer::addFrameLabel(uint32_t frame, const tiny_string& label)
{
	for(size_t i=0; i<scenes.size();++i)
	{
		if(frame < scenes[i].startframe)
		{
			scenes[i-1].addFrameLabel(frame,label);
			return;
		}
	}
	scenes.back().addFrameLabel(frame,label);
}

void MovieClip::sinit(Class_base* c)
{
	CLASS_SETUP(c, Sprite, _constructor, CLASS_DYNAMIC_NOT_FINAL);
	c->isReusable = true;
	c->setDeclaredMethodByQName("currentFrame","",Class<IFunction>::getFunction(c->getSystemState(),_getCurrentFrame,0,Class<Integer>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("totalFrames","",Class<IFunction>::getFunction(c->getSystemState(),_getTotalFrames,0,Class<Integer>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("framesLoaded","",Class<IFunction>::getFunction(c->getSystemState(),_getFramesLoaded,0,Class<Integer>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("currentFrameLabel","",Class<IFunction>::getFunction(c->getSystemState(),_getCurrentFrameLabel,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("currentLabel","",Class<IFunction>::getFunction(c->getSystemState(),_getCurrentLabel,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("currentLabels","",Class<IFunction>::getFunction(c->getSystemState(),_getCurrentLabels,0,Class<Array>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("scenes","",Class<IFunction>::getFunction(c->getSystemState(),_getScenes,0,Class<Array>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("currentScene","",Class<IFunction>::getFunction(c->getSystemState(),_getCurrentScene,0,Class<Scene>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("stop","",Class<IFunction>::getFunction(c->getSystemState(),stop),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("play","",Class<IFunction>::getFunction(c->getSystemState(),play),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("gotoAndStop","",Class<IFunction>::getFunction(c->getSystemState(),gotoAndStop),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("gotoAndPlay","",Class<IFunction>::getFunction(c->getSystemState(),gotoAndPlay),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("prevFrame","",Class<IFunction>::getFunction(c->getSystemState(),prevFrame),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("nextFrame","",Class<IFunction>::getFunction(c->getSystemState(),nextFrame),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("addFrameScript","",Class<IFunction>::getFunction(c->getSystemState(),addFrameScript),NORMAL_METHOD,true);
	REGISTER_GETTER_SETTER_RESULTTYPE(c, enabled,Boolean);
}

ASFUNCTIONBODY_GETTER_SETTER(MovieClip, enabled)

MovieClip::MovieClip(ASWorker* wrk, Class_base* c):Sprite(wrk,c),fromDefineSpriteTag(UINT32_MAX),lastFrameScriptExecuted(UINT32_MAX),lastratio(0),inExecuteFramescript(false)
  ,inAVM1Attachment(false),isAVM1Loaded(false)
  ,actions(nullptr),totalFrames_unreliable(1),enabled(true)
{
	subtype=SUBTYPE_MOVIECLIP;
}

MovieClip::MovieClip(ASWorker* wrk, Class_base* c, const FrameContainer& f, uint32_t defineSpriteTagID):Sprite(wrk,c),FrameContainer(f),fromDefineSpriteTag(defineSpriteTagID),lastFrameScriptExecuted(UINT32_MAX),lastratio(0),inExecuteFramescript(false)
  ,inAVM1Attachment(false),isAVM1Loaded(false)
  ,actions(nullptr),totalFrames_unreliable(frames.size()),enabled(true)
{
	subtype=SUBTYPE_MOVIECLIP;
	//For sprites totalFrames_unreliable is the actual frame count
	//For the root movie, it's the frame count from the header
}

bool MovieClip::destruct()
{
	getSystemState()->stage->removeHiddenObject(this);
	frames.clear();
	inAVM1Attachment=false;
	isAVM1Loaded=false;
	setFramesLoaded(0);
	auto it = frameScripts.begin();
	while (it != frameScripts.end())
	{
		ASATOM_DECREF(it->second);
		it++;
	}
	frameScripts.clear();
	
	fromDefineSpriteTag = UINT32_MAX;
	lastFrameScriptExecuted = UINT32_MAX;
	lastratio=0;
	totalFrames_unreliable = 1;
	inExecuteFramescript=false;

	scenes.clear();
	setFramesLoaded(0);
	frames.emplace_back(Frame());
	scenes.resize(1);
	state.reset();
	actions=nullptr;

	enabled = true;
	avm1loader.reset();
	return Sprite::destruct();
}

void MovieClip::finalize()
{
	getSystemState()->stage->removeHiddenObject(this);
	frames.clear();
	auto it = frameScripts.begin();
	while (it != frameScripts.end())
	{
		ASATOM_DECREF(it->second);
		it++;
	}
	frameScripts.clear();
	scenes.clear();
	state.reset();
	avm1loader.reset();
	Sprite::finalize();
}

void MovieClip::prepareShutdown()
{
	if (preparedforshutdown)
		return;
	Sprite::prepareShutdown();
	auto it = frameScripts.begin();
	while (it != frameScripts.end())
	{
		ASObject* o = asAtomHandler::getObject(it->second);
		if (o)
			o->prepareShutdown();
		it++;
	}
	if (avm1loader)
		avm1loader->prepareShutdown();
}

/* Returns a Scene_data pointer for a scene called sceneName, or for
 * the current scene if sceneName is empty. Returns nullptr, if not found.
 */
const Scene_data *MovieClip::getScene(const tiny_string &sceneName) const
{
	if (sceneName.empty())
	{
		return &scenes[getCurrentScene()];
	}
	else
	{
		//Find scene by name
		for (auto it=scenes.begin(); it!=scenes.end(); ++it)
		{
			if (it->name == sceneName)
				return &*it;
		}
	}

	return nullptr;  //Not found!
}

/* Return global frame index for a named frame. If sceneName is not
 * empty, return a frame only if it belong to the named scene.
 */
uint32_t MovieClip::getFrameIdByLabel(const tiny_string& label, const tiny_string& sceneName) const
{
	if (sceneName.empty())
	{
		//Find frame in any scene
		for(size_t i=0;i<scenes.size();++i)
		{
			for(size_t j=0;j<scenes[i].labels.size();++j)
				if(scenes[i].labels[j].name == label || scenes[i].labels[j].name.lowercase() == label.lowercase())
					return scenes[i].labels[j].frame;
		}
	}
	else
	{
		//Find frame in the named scene only
		const Scene_data *scene = getScene(sceneName);
		if (scene)
		{
			for(size_t j=0;j<scene->labels.size();++j)
			{
				if(scene->labels[j].name == label || scene->labels[j].name.lowercase() == label.lowercase())
					return scene->labels[j].frame;
			}
		}
	}

	return FRAME_NOT_FOUND;
}

/* Return global frame index for frame i (zero-based) in a scene
 * called sceneName. If sceneName is empty, use the current scene.
 */
uint32_t MovieClip::getFrameIdByNumber(uint32_t i, const tiny_string& sceneName) const
{
	const Scene_data *sceneData = getScene(sceneName);
	if (!sceneData)
		return FRAME_NOT_FOUND;

	//Should we check if the scene has at least i frames?
	return sceneData->startframe + i;
}

ASFUNCTIONBODY_ATOM(MovieClip,addFrameScript)
{
	MovieClip* th=Class<MovieClip>::cast(asAtomHandler::getObject(obj));
	assert_and_throw(argslen>=2 && argslen%2==0);

	for(uint32_t i=0;i<argslen;i+=2)
	{
		uint32_t frame=asAtomHandler::toInt(args[i]);

		if(!asAtomHandler::isFunction(args[i+1]))
		{
			LOG(LOG_ERROR,"Not a function");
			return;
		}
		IFunction* func = asAtomHandler::as<IFunction>(args[i+1]);
		func->incRef();
		ASObject* c = func->closure_this;
		func->closure_this=nullptr;
		th->frameScripts[frame]=args[i+1];
		if (c)
			c->removeStoredMember();
	}
}

ASFUNCTIONBODY_ATOM(MovieClip,swapDepths)
{
	LOG(LOG_NOT_IMPLEMENTED,"Called swapDepths");
}

ASFUNCTIONBODY_ATOM(MovieClip,stop)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	th->setStopped();
}

ASFUNCTIONBODY_ATOM(MovieClip,play)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	th->setPlaying();
}
void MovieClip::setPlaying()
{
	if (state.stop_FP)
	{
		state.stop_FP=false;
		if (!needsActionScript3() && state.next_FP == state.FP)
		{
			if (state.FP == getFramesLoaded()-1)
				state.next_FP = 0;
			else
				state.next_FP++;
		}
		if (isOnStage())
		{
			if (needsActionScript3())
				advanceFrame(true);
		}
		else
			getSystemState()->stage->addHiddenObject(this);
	}
}
void MovieClip::setStopped()
{
	if (!state.stop_FP)
	{
		state.stop_FP=true;
		state.next_FP=state.FP;
	}
}
void MovieClip::gotoAnd(asAtom* args, const unsigned int argslen, bool stop)
{
	uint32_t next_FP=0;
	tiny_string sceneName;
	assert_and_throw(argslen==1 || argslen==2);
	if(argslen==2 && needsActionScript3())
	{
		sceneName = asAtomHandler::toString(args[1],getInstanceWorker());
	}
	uint32_t dest=FRAME_NOT_FOUND;
	if(asAtomHandler::isString(args[0]))
	{
		tiny_string label = asAtomHandler::toString(args[0],getInstanceWorker());
		dest=getFrameIdByLabel(label, sceneName);
		if(dest==FRAME_NOT_FOUND)
		{
			number_t ret=0;
			if (Integer::fromStringFlashCompatible(label.raw_buf(),ret,10,true))
			{
				// it seems that at least for AVM1 Adobe treats number strings as frame numbers
				dest = getFrameIdByNumber(ret-1, sceneName);
			}
			if(dest==FRAME_NOT_FOUND)
			{
				dest=0;
				LOG(LOG_ERROR, (stop ? "gotoAndStop: label not found:" : "gotoAndPlay: label not found:") <<asAtomHandler::toString(args[0],getInstanceWorker())<<" in scene "<<sceneName<<" at movieclip "<<getTagID()<<" "<<this->state.FP);
			}
		}
	}
	else
	{
		uint32_t inFrameNo = asAtomHandler::toInt(args[0]);
		if(inFrameNo == 0)
			inFrameNo = 1;

		dest = getFrameIdByNumber(inFrameNo-1, sceneName);
	}
	if (dest!=FRAME_NOT_FOUND)
	{
		next_FP=dest;
		while(next_FP >= getFramesLoaded())
		{
			if (hasFinishedLoading())
			{
				if (next_FP >= getFramesLoaded())
				{
					LOG(LOG_ERROR, next_FP << "= next_FP >= state.max_FP = " << getFramesLoaded() << " on "<<this->toDebugString()<<" "<<this->getTagID());
					next_FP = getFramesLoaded()-1;
				}
				break;
			}
			else
				compat_msleep(100);
		}
	}
	bool newframe = state.FP != next_FP;
	state.next_FP = next_FP;
	state.explicit_FP = true;
	state.stop_FP = stop;
	currentFrameChanged(newframe);
}
void MovieClip::currentFrameChanged(bool newframe)
{
	if (!needsActionScript3())
	{
		advanceFrame(false);
		return;
	}
	if (inExecuteFramescript)
		return; // we are currently executing a framescript, so advancing to the new frame will be done through the normal SystemState tick;

	if (newframe && !state.creatingframe)
		lastFrameScriptExecuted=UINT32_MAX;
	if (!this->isOnStage())
	{
		if (state.stop_FP)
		{
			advanceFrame(false);
			initFrame();
			this->incRef();
			this->getSystemState()->currentVm->addEvent(NullRef, _MR(new (this->getSystemState()->unaccountedMemory) ExecuteFrameScriptEvent(_MR(this))));
		}
		else
			this->getSystemState()->stage->addHiddenObject(this);
	}
	else if (newframe && getSystemState()->getSwfVersion()>= 10)
	{
		// according to http://www.senocular.com/flash/tutorials/orderofoperations/
		// a subset of the normal events are added when navigation commands are executed when changing to a new frame by actionscript
		advanceFrame(false);
		initFrame();
		this->incRef();
		getVm(getSystemState())->addEvent(_MR(this),_MR(Class<Event>::getInstanceS(this->getInstanceWorker(),"frameConstructed")));
		this->incRef();
		getVm(getSystemState())->addEvent(NullRef, _MR(new (this->getSystemState()->unaccountedMemory) ExecuteFrameScriptEvent(_MR(this))));
		this->incRef();
		getVm(getSystemState())->addEvent(_MR(this),_MR(Class<Event>::getInstanceS(this->getInstanceWorker(),"exitFrame")));
	}
}
void MovieClip::AVM1gotoFrameLabel(const tiny_string& label,bool stop, bool switchplaystate)
{
	uint32_t dest=getFrameIdByLabel(label, "");
	if(dest==FRAME_NOT_FOUND)
	{
		LOG(LOG_ERROR, "gotoFrameLabel: label not found:" <<label);
		return;
	}
	AVM1gotoFrame(dest, stop, switchplaystate,true);
}
void MovieClip::AVM1gotoFrame(int frame, bool stop, bool switchplaystate, bool advanceFrame)
{
	if (frame < 0)
		frame = 0;
	state.next_FP = frame;
	state.explicit_FP = true;
	bool newframe = (int)state.FP != frame;
	if (switchplaystate)
		state.stop_FP = stop;
	if (advanceFrame)
		currentFrameChanged(newframe);
}

ASFUNCTIONBODY_ATOM(MovieClip,gotoAndStop)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	th->gotoAnd(args,argslen,true);
}

ASFUNCTIONBODY_ATOM(MovieClip,gotoAndPlay)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	th->gotoAnd(args,argslen,false);
}

ASFUNCTIONBODY_ATOM(MovieClip,nextFrame)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	assert_and_throw(th->state.FP<th->getFramesLoaded());
	bool newframe = th->state.FP != th->getFramesLoaded()-1;
	th->state.next_FP = th->state.FP == th->getFramesLoaded()-1 ? th->state.FP : th->state.FP+1;
	th->state.explicit_FP=true;
	th->state.stop_FP=true;
	th->currentFrameChanged(newframe);
}

ASFUNCTIONBODY_ATOM(MovieClip,prevFrame)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	assert_and_throw(th->state.FP<th->getFramesLoaded());
	bool newframe = th->state.FP != 0;
	th->state.next_FP = th->state.FP == 0 ? th->state.FP : th->state.FP-1;
	th->state.explicit_FP=true;
	th->state.stop_FP=true;
	th->currentFrameChanged(newframe);
}

ASFUNCTIONBODY_ATOM(MovieClip,_getFramesLoaded)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	asAtomHandler::setUInt(ret,wrk,th->getFramesLoaded());
}

ASFUNCTIONBODY_ATOM(MovieClip,_getTotalFrames)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	asAtomHandler::setUInt(ret,wrk,th->totalFrames_unreliable);
}

ASFUNCTIONBODY_ATOM(MovieClip,_getScenes)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	Array* res = Class<Array>::getInstanceSNoArgs(wrk);
	res->resize(th->scenes.size());
	uint32_t numFrames;
	for(size_t i=0; i<th->scenes.size(); ++i)
	{
		if(i == th->scenes.size()-1)
			numFrames = th->totalFrames_unreliable - th->scenes[i].startframe;
		else
			numFrames = th->scenes[i].startframe - th->scenes[i+1].startframe;
		asAtom v = asAtomHandler::fromObject(Class<Scene>::getInstanceS(wrk,th->scenes[i],numFrames));
		res->set(i, v,false,false);
	}
	ret = asAtomHandler::fromObject(res);
}

uint32_t MovieClip::getCurrentScene() const
{
	for(size_t i=0;i<scenes.size();++i)
	{
		if(state.FP < scenes[i].startframe)
			return i-1;
	}
	return scenes.size()-1;
}

ASFUNCTIONBODY_ATOM(MovieClip,_getCurrentScene)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	uint32_t numFrames;
	uint32_t curScene = th->getCurrentScene();
	if(curScene == th->scenes.size()-1)
		numFrames = th->totalFrames_unreliable - th->scenes[curScene].startframe;
	else
		numFrames = th->scenes[curScene].startframe - th->scenes[curScene+1].startframe;

	ret = asAtomHandler::fromObject(Class<Scene>::getInstanceS(wrk,th->scenes[curScene],numFrames));
}

ASFUNCTIONBODY_ATOM(MovieClip,_getCurrentFrame)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	//currentFrame is 1-based and relative to current scene
	if (th->state.explicit_FP)
		// if frame is set explicitly, the currentframe property should be set to next_FP, even if it is not displayed yet
		asAtomHandler::setInt(ret,wrk,th->state.next_FP+1 - th->scenes[th->getCurrentScene()].startframe);
	else
		asAtomHandler::setInt(ret,wrk,th->state.FP+1 - th->scenes[th->getCurrentScene()].startframe);
}

ASFUNCTIONBODY_ATOM(MovieClip,_getCurrentFrameLabel)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	for(size_t i=0;i<th->scenes.size();++i)
	{
		for(size_t j=0;j<th->scenes[i].labels.size();++j)
			if(th->scenes[i].labels[j].frame == th->state.FP)
			{
				ret = asAtomHandler::fromObject(abstract_s(wrk,th->scenes[i].labels[j].name));
				return;
			}
	}
	asAtomHandler::setNull(ret);
}

ASFUNCTIONBODY_ATOM(MovieClip,_getCurrentLabel)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	tiny_string label;
	for(size_t i=0;i<th->scenes.size();++i)
	{
		if(th->scenes[i].startframe > th->state.FP)
			break;
		for(size_t j=0;j<th->scenes[i].labels.size();++j)
		{
			if(th->scenes[i].labels[j].frame > th->state.FP)
				break;
			if(!th->scenes[i].labels[j].name.empty())
				label = th->scenes[i].labels[j].name;
		}
	}

	if(label.empty())
		asAtomHandler::setNull(ret);
	else
		ret = asAtomHandler::fromObject(abstract_s(wrk,label));
}

ASFUNCTIONBODY_ATOM(MovieClip,_getCurrentLabels)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	Scene_data& sc = th->scenes[th->getCurrentScene()];

	Array* res = Class<Array>::getInstanceSNoArgs(wrk);
	res->resize(sc.labels.size());
	for(size_t i=0; i<sc.labels.size(); ++i)
	{
		asAtom v = asAtomHandler::fromObject(Class<FrameLabel>::getInstanceS(wrk,sc.labels[i]));
		res->set(i, v,false,false);
	}
	ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(MovieClip,_constructor)
{
	Sprite::_constructor(ret,wrk,obj,nullptr,0);
/*	th->setVariableByQName("swapDepths","",Class<IFunction>::getFunction(c->getSystemState(),swapDepths));
	th->setVariableByQName("createEmptyMovieClip","",Class<IFunction>::getFunction(c->getSystemState(),createEmptyMovieClip));*/
}

void MovieClip::addScene(uint32_t sceneNo, uint32_t startframe, const tiny_string& name)
{
	if(sceneNo == 0)
	{
		//we always have one scene, but this call may set its name
		scenes[0].name = name;
	}
	else
	{
		assert(scenes.size() == sceneNo);
		scenes.resize(sceneNo+1);
		scenes[sceneNo].name = name;
		scenes[sceneNo].startframe = startframe;
	}
}

void MovieClip::afterLegacyInsert()
{
	if(!needsActionScript3())
	{
		asAtom obj = asAtomHandler::fromObjectNoPrimitive(this);
		getClass()->handleConstruction(obj,nullptr,0,true);
	}
	Sprite::afterLegacyInsert();
}

void MovieClip::afterLegacyDelete(DisplayObjectContainer *parent,bool inskipping)
{
	getSystemState()->stage->AVM1RemoveMouseListener(this);
	getSystemState()->stage->AVM1RemoveKeyboardListener(this);
}
bool MovieClip::AVM1HandleKeyboardEvent(KeyboardEvent *e)
{
	if (this->actions)
	{
		for (auto it = actions->ClipActionRecords.begin(); it != actions->ClipActionRecords.end(); it++)
		{
			if( (e->type == "keyDown" && it->EventFlags.ClipEventKeyDown) ||
				(e->type == "keyUp" && it->EventFlags.ClipEventKeyDown))
			{
				std::map<uint32_t,asAtom> m;
				ACTIONRECORD::executeActions(this,this->getCurrentFrame()->getAVM1Context(),it->actions,it->startactionpos,m);
			}
		}
	}
	Sprite::AVM1HandleKeyboardEvent(e);
	return false;
}
bool MovieClip::AVM1HandleMouseEvent(EventDispatcher *dispatcher, MouseEvent *e)
{
	if (!this->isOnStage() || !this->enabled)
		return false;
	if (dispatcher->is<DisplayObject>())
	{
		DisplayObject* dispobj=nullptr;
		if (dispatcher == this)
			dispobj=this;
		else
		{
			number_t x,y,xg,yg;
			dispatcher->as<DisplayObject>()->localToGlobal(e->localX,e->localY,xg,yg);
			this->globalToLocal(xg,yg,x,y);
			_NR<DisplayObject> d =hitTest(x,y, DisplayObject::MOUSE_CLICK,true);
			dispobj = d.getPtr();
		}
		if (actions)
		{
			for (auto it = actions->ClipActionRecords.begin(); it != actions->ClipActionRecords.end(); it++)
			{
				// according to https://docstore.mik.ua/orelly/web2/action/ch10_09.htm
				// mouseUp/mouseDown/mouseMove events are sent to all MovieClips on the Stage
				if( (e->type == "mouseDown" && it->EventFlags.ClipEventMouseDown)
					|| (e->type == "mouseUp" && it->EventFlags.ClipEventMouseUp)
					|| (e->type == "mouseMove" && it->EventFlags.ClipEventMouseMove)
					)
				{
					std::map<uint32_t,asAtom> m;
					ACTIONRECORD::executeActions(this,this->getCurrentFrame()->getAVM1Context(),it->actions,it->startactionpos,m);
				}
				if( dispobj &&
					((e->type == "mouseUp" && it->EventFlags.ClipEventRelease)
					|| (e->type == "mouseDown" && it->EventFlags.ClipEventPress)
					|| (e->type == "rollOver" && it->EventFlags.ClipEventRollOver)
					|| (e->type == "rollOut" && it->EventFlags.ClipEventRollOut)
					|| (e->type == "releaseOutside" && it->EventFlags.ClipEventReleaseOutside)
					))
				{
					std::map<uint32_t,asAtom> m;
					ACTIONRECORD::executeActions(this,this->getCurrentFrame()->getAVM1Context(),it->actions,it->startactionpos,m);
				}
			}
		}
		AVM1HandleMouseEventStandard(dispobj,e);
	}
	return false;
}
void MovieClip::AVM1HandleEvent(EventDispatcher *dispatcher, Event* e)
{
	std::map<uint32_t,asAtom> m;
	if (dispatcher == this)
	{
		if (this->actions)
		{
			for (auto it = actions->ClipActionRecords.begin(); it != actions->ClipActionRecords.end(); it++)
			{
				if (e->type == "complete" && it->EventFlags.ClipEventLoad)
				{
					ACTIONRECORD::executeActions(this,this->getCurrentFrame()->getAVM1Context(),it->actions,it->startactionpos,m);
				}
			}
		}
	}
}

void MovieClip::AVM1AfterAdvance()
{
	state.frameadvanced=false;
	state.last_FP=state.FP;
	state.explicit_FP=false;
}


void MovieClip::setupActions(const CLIPACTIONS &clipactions)
{
	actions = &clipactions;
	if (clipactions.AllEventFlags.ClipEventMouseDown ||
			clipactions.AllEventFlags.ClipEventMouseMove ||
			clipactions.AllEventFlags.ClipEventRollOver ||
			clipactions.AllEventFlags.ClipEventRollOut ||
			clipactions.AllEventFlags.ClipEventPress ||
			clipactions.AllEventFlags.ClipEventMouseUp)
	{
		setMouseEnabled(true);
		getSystemState()->stage->AVM1AddMouseListener(this);
	}
	if (clipactions.AllEventFlags.ClipEventKeyDown ||
			clipactions.AllEventFlags.ClipEventKeyUp)
		getSystemState()->stage->AVM1AddKeyboardListener(this);

	if (clipactions.AllEventFlags.ClipEventLoad)
		getSystemState()->stage->AVM1AddEventListener(this);
	if (clipactions.AllEventFlags.ClipEventEnterFrame)
	{
		getSystemState()->registerFrameListener(this);
		getSystemState()->stage->AVM1AddEventListener(this);
	}
}

void MovieClip::AVM1SetupMethods(Class_base* c)
{
	DisplayObject::AVM1SetupMethods(c);
	c->setDeclaredMethodByQName("attachMovie","",Class<IFunction>::getFunction(c->getSystemState(),AVM1AttachMovie),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("loadMovie","",Class<IFunction>::getFunction(c->getSystemState(),AVM1LoadMovie),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("unloadMovie","",Class<IFunction>::getFunction(c->getSystemState(),AVM1UnloadMovie),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("createEmptyMovieClip","",Class<IFunction>::getFunction(c->getSystemState(),AVM1CreateEmptyMovieClip),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("removeMovieClip","",Class<IFunction>::getFunction(c->getSystemState(),AVM1RemoveMovieClip),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("duplicateMovieClip","",Class<IFunction>::getFunction(c->getSystemState(),AVM1DuplicateMovieClip),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("clear","",Class<IFunction>::getFunction(c->getSystemState(),AVM1Clear),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("moveTo","",Class<IFunction>::getFunction(c->getSystemState(),AVM1MoveTo),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("lineTo","",Class<IFunction>::getFunction(c->getSystemState(),AVM1LineTo),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("lineStyle","",Class<IFunction>::getFunction(c->getSystemState(),AVM1LineStyle),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("beginFill","",Class<IFunction>::getFunction(c->getSystemState(),AVM1BeginFill),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("beginGradientFill","",Class<IFunction>::getFunction(c->getSystemState(),AVM1BeginGradientFill),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("endFill","",Class<IFunction>::getFunction(c->getSystemState(),AVM1EndFill),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("useHandCursor","",Class<IFunction>::getFunction(c->getSystemState(),Sprite::_getter_useHandCursor),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("useHandCursor","",Class<IFunction>::getFunction(c->getSystemState(),Sprite::_setter_useHandCursor),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("getNextHighestDepth","",Class<IFunction>::getFunction(c->getSystemState(),AVM1GetNextHighestDepth),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("attachBitmap","",Class<IFunction>::getFunction(c->getSystemState(),AVM1AttachBitmap),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("gotoAndStop","",Class<IFunction>::getFunction(c->getSystemState(),gotoAndStop),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("gotoAndPlay","",Class<IFunction>::getFunction(c->getSystemState(),gotoAndPlay),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("gotoandstop","",Class<IFunction>::getFunction(c->getSystemState(),gotoAndStop),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("gotoandplay","",Class<IFunction>::getFunction(c->getSystemState(),gotoAndPlay),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("stop","",Class<IFunction>::getFunction(c->getSystemState(),stop),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("play","",Class<IFunction>::getFunction(c->getSystemState(),play),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getInstanceAtDepth","",Class<IFunction>::getFunction(c->getSystemState(),AVM1getInstanceAtDepth),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getSWFVersion","",Class<IFunction>::getFunction(c->getSystemState(),AVM1getSWFVersion),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("menu","",Class<IFunction>::getFunction(c->getSystemState(),_getter_contextMenu),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("menu","",Class<IFunction>::getFunction(c->getSystemState(),_setter_contextMenu),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("prevFrame","",Class<IFunction>::getFunction(c->getSystemState(),prevFrame),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("nextFrame","",Class<IFunction>::getFunction(c->getSystemState(),nextFrame),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("createTextField","",Class<IFunction>::getFunction(c->getSystemState(),AVM1CreateTextField),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("enabled","",Class<IFunction>::getFunction(c->getSystemState(),InteractiveObject::_getMouseEnabled,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("enabled","",Class<IFunction>::getFunction(c->getSystemState(),InteractiveObject::_setMouseEnabled),SETTER_METHOD,true);
}

void MovieClip::AVM1ExecuteFrameActionsFromLabel(const tiny_string &label)
{
	uint32_t dest=getFrameIdByLabel(label, "");
	if(dest==FRAME_NOT_FOUND)
	{
		LOG(LOG_INFO, "AVM1ExecuteFrameActionsFromLabel: label not found:" <<label);
		return;
	}
	AVM1ExecuteFrameActions(dest);
}

void MovieClip::AVM1ExecuteFrameActions(uint32_t frame)
{
	auto it=frames.begin();
	uint32_t i=0;
	while(it != frames.end() && i < frame)
	{
		++i;
		++it;
	}
	if (it != frames.end())
	{
		it->AVM1executeActions(this);
	}
}

ASFUNCTIONBODY_ATOM(MovieClip,AVM1AttachMovie)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	if (argslen != 3 && argslen != 4)
		throw RunTimeException("AVM1: invalid number of arguments for attachMovie");
	int Depth = asAtomHandler::toInt(args[2]);
	uint32_t nameId = asAtomHandler::toStringId(args[1],wrk);
	DictionaryTag* placedTag = th->loadedFrom->dictionaryLookupByName(asAtomHandler::toStringId(args[0],wrk));
	if (!placedTag)
	{
		ret=asAtomHandler::undefinedAtom;
		return;
	}
	ASObject *instance = placedTag->instance();
	DisplayObject* toAdd=dynamic_cast<DisplayObject*>(instance);
	if(!toAdd)
	{
		if (instance)
			LOG(LOG_NOT_IMPLEMENTED, "AVM1: attachMovie adding non-DisplayObject to display list:"<<instance->toDebugString());
		else
			LOG(LOG_NOT_IMPLEMENTED, "AVM1: attachMovie couldn't create instance of item:"<<placedTag->getId());
		return;
	}
	toAdd->name = nameId;
	if (argslen == 4)
	{
		ASObject* o = asAtomHandler::getObject(args[3]);
		if (o)
			o->copyValues(toAdd,wrk);
	}
	if (toAdd->is<MovieClip>())
		toAdd->as<MovieClip>()->inAVM1Attachment=true;
	if(th->hasLegacyChildAt(Depth) )
	{
		th->deleteLegacyChildAt(Depth,false);
		th->insertLegacyChildAt(Depth,toAdd,false,false);
	}
	else
		th->insertLegacyChildAt(Depth,toAdd,false,false);
	if (toAdd->is<MovieClip>())
		toAdd->as<MovieClip>()->inAVM1Attachment=false;
	toAdd->incRef();
	if (argslen == 4)
	{
		// update all bindings _after_ the clip is constructed
		ASObject* o = asAtomHandler::getObject(args[3]);
		if (o)
			o->AVM1UpdateAllBindings(toAdd,wrk);
	}
	ret=asAtomHandler::fromObjectNoPrimitive(toAdd);
}
ASFUNCTIONBODY_ATOM(MovieClip,AVM1CreateEmptyMovieClip)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	if (argslen < 2)
		throw RunTimeException("AVM1: invalid number of arguments for CreateEmptyMovieClip");
	int Depth = asAtomHandler::toInt(args[1]);
	uint32_t nameId = asAtomHandler::toStringId(args[0],wrk);
	AVM1MovieClip* toAdd= Class<AVM1MovieClip>::getInstanceSNoArgs(wrk);
	toAdd->name = nameId;
	toAdd->setMouseEnabled(false);
	if(th->hasLegacyChildAt(Depth) )
	{
		th->deleteLegacyChildAt(Depth,false);
		th->insertLegacyChildAt(Depth,toAdd,false,false);
	}
	else
		th->insertLegacyChildAt(Depth,toAdd,false,false);
	toAdd->constructionComplete();
	toAdd->afterConstruction();
	toAdd->incRef();
	ret=asAtomHandler::fromObject(toAdd);
}
ASFUNCTIONBODY_ATOM(MovieClip,AVM1RemoveMovieClip)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	if (th->getParent() && !th->is<RootMovieClip>())
	{
		if (th->name != BUILTIN_STRINGS::EMPTY)
		{
			multiname m(nullptr);
			m.name_type=multiname::NAME_STRING;
			m.name_s_id=th->name;
			m.ns.emplace_back(wrk->getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
			m.isAttribute = false;
			// don't remove the child by name here because another DisplayObject may have been added with this name after this clip was added
			th->getParent()->deleteVariableByMultinameWithoutRemovingChild(m,wrk);
		}
		th->getParent()->_removeChild(th);
	}
}
ASFUNCTIONBODY_ATOM(MovieClip,AVM1DuplicateMovieClip)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	if (argslen < 2)
		throw RunTimeException("AVM1: invalid number of arguments for DuplicateMovieClip");
	if (!th->getParent())
	{
		LOG(LOG_ERROR,"calling DuplicateMovieClip on clip without parent");
		ret = asAtomHandler::undefinedAtom;
		return;
	}
	int Depth = asAtomHandler::toInt(args[1]);
	ASObject* initobj = nullptr;
	if (argslen > 2)
		initobj = asAtomHandler::toObject(args[2],wrk);
	MovieClip* toAdd=th->AVM1CloneSprite(args[0],Depth,initobj);
	toAdd->incRef();
	ret=asAtomHandler::fromObject(toAdd);
}
MovieClip* MovieClip::AVM1CloneSprite(asAtom target, uint32_t Depth,ASObject* initobj)
{
	uint32_t nameId = asAtomHandler::toStringId(target,getInstanceWorker());
	AVM1MovieClip* toAdd=nullptr;
	DefineSpriteTag* tag = (DefineSpriteTag*)loadedFrom->dictionaryLookup(getTagID());
	if (tag)
	{
		toAdd= tag->instance()->as<AVM1MovieClip>();
		toAdd->legacy=true;
	}
	else
		toAdd= Class<AVM1MovieClip>::getInstanceSNoArgs(getInstanceWorker());
	
	if (initobj)
		initobj->copyValues(toAdd,getInstanceWorker());
	toAdd->loadedFrom=this->loadedFrom;
	toAdd->setLegacyMatrix(getMatrix());
	toAdd->colorTransform = this->colorTransform;
	toAdd->name = nameId;
	if (this->actions)
		toAdd->setupActions(*actions);
	toAdd->tokens.filltokens = this->tokens.filltokens;
	toAdd->tokens.stroketokens = this->tokens.stroketokens;
	if(getParent()->hasLegacyChildAt(Depth))
	{
		getParent()->deleteLegacyChildAt(Depth,false);
		getParent()->insertLegacyChildAt(Depth,toAdd,false,false);
	}
	else
		getParent()->insertLegacyChildAt(Depth,toAdd,false,false);
	toAdd->constructionComplete();
	if (state.creatingframe)
		toAdd->advanceFrame(true);
	return toAdd;
}

string MovieClip::toDebugString() const
{
	std::string res = Sprite::toDebugString();
	res += " state=";
	char buf[100];
	sprintf(buf,"%d/%u/%u%s",state.last_FP,state.FP,state.next_FP,state.stop_FP?" stopped":"");
	res += buf;
	return res;
}
ASFUNCTIONBODY_ATOM(MovieClip,AVM1Clear)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	Graphics* g = th->getGraphics().getPtr();
	asAtom o = asAtomHandler::fromObject(g);
	Graphics::clear(ret,wrk,o,args,argslen);
}
ASFUNCTIONBODY_ATOM(MovieClip,AVM1MoveTo)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	Graphics* g = th->getGraphics().getPtr();
	asAtom o = asAtomHandler::fromObject(g);
	Graphics::moveTo(ret,wrk,o,args,argslen);
}
ASFUNCTIONBODY_ATOM(MovieClip,AVM1LineTo)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	Graphics* g = th->getGraphics().getPtr();
	asAtom o = asAtomHandler::fromObject(g);
	Graphics::lineTo(ret,wrk,o,args,argslen);
}
ASFUNCTIONBODY_ATOM(MovieClip,AVM1LineStyle)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	Graphics* g = th->getGraphics().getPtr();
	asAtom o = asAtomHandler::fromObject(g);
	Graphics::lineStyle(ret,wrk,o,args,argslen);
}
ASFUNCTIONBODY_ATOM(MovieClip,AVM1BeginFill)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	Graphics* g = th->getGraphics().getPtr();
	asAtom o = asAtomHandler::fromObject(g);
	if(argslen>=2)
		args[1]=asAtomHandler::fromNumber(wrk,asAtomHandler::toNumber(args[1])/100.0,false);
	
	Graphics::beginFill(ret,wrk,o,args,argslen);
}
ASFUNCTIONBODY_ATOM(MovieClip,AVM1BeginGradientFill)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	Graphics* g = th->getGraphics().getPtr();
	asAtom o = asAtomHandler::fromObject(g);
	Graphics::beginGradientFill(ret,wrk,o,args,argslen);
}
ASFUNCTIONBODY_ATOM(MovieClip,AVM1EndFill)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	Graphics* g = th->getGraphics().getPtr();
	asAtom o = asAtomHandler::fromObject(g);
	Graphics::endFill(ret,wrk,o,args,argslen);
}
ASFUNCTIONBODY_ATOM(MovieClip,AVM1GetNextHighestDepth)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	uint32_t n = th->getMaxLegacyChildDepth();
	asAtomHandler::setUInt(ret,wrk,n == UINT32_MAX ? 0 : n+1);
}
ASFUNCTIONBODY_ATOM(MovieClip,AVM1AttachBitmap)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	if (argslen < 2)
		throw RunTimeException("AVM1: invalid number of arguments for attachBitmap");
	int Depth = asAtomHandler::toInt(args[1]);
	if (!asAtomHandler::is<BitmapData>(args[0]))
	{
		LOG(LOG_ERROR,"AVM1AttachBitmap invalid type:"<<asAtomHandler::toDebugString(args[0]));
		throw RunTimeException("AVM1: attachBitmap first parameter is no BitmapData");
	}

	AVM1BitmapData* data = asAtomHandler::as<AVM1BitmapData>(args[0]);
	data->incRef();
	Bitmap* toAdd = Class<AVM1Bitmap>::getInstanceS(wrk,_MR(data));
	if (argslen > 2)
		toAdd->pixelSnapping = asAtomHandler::toString(args[2],wrk);
	if (argslen > 3)
		toAdd->smoothing = asAtomHandler::Boolean_concrete(args[3]);
	if(th->hasLegacyChildAt(Depth) )
	{
		th->deleteLegacyChildAt(Depth,false);
		th->insertLegacyChildAt(Depth,toAdd,false,false);
	}
	else
		th->insertLegacyChildAt(Depth,toAdd,false,false);
	toAdd->incRef();
	ret=asAtomHandler::fromObject(toAdd);
}
ASFUNCTIONBODY_ATOM(MovieClip,AVM1getInstanceAtDepth)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	int32_t depth;
	ARG_CHECK(ARG_UNPACK(depth));
	if (th->hasLegacyChildAt(depth))
	{
		DisplayObject* o = th->getLegacyChildAt(depth);
		o->incRef();
		ret = asAtomHandler::fromObjectNoPrimitive(o);
	}
	else
		ret = asAtomHandler::undefinedAtom;
}
ASFUNCTIONBODY_ATOM(MovieClip,AVM1getSWFVersion)
{
	asAtomHandler::setUInt(ret,wrk,wrk->getSystemState()->getSwfVersion());
}
ASFUNCTIONBODY_ATOM(MovieClip,AVM1LoadMovie)
{
	AVM1MovieClip* th=asAtomHandler::as<AVM1MovieClip>(obj);
	tiny_string url;
	tiny_string method;
	ARG_CHECK(ARG_UNPACK(url,"")(method,"GET"));
	
	AVM1MovieClipLoader* ld = Class<AVM1MovieClipLoader>::getInstanceSNoArgs(wrk);
	th->avm1loader = _MR(ld);
	ld->load(url,method,th);
}
ASFUNCTIONBODY_ATOM(MovieClip,AVM1UnloadMovie)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	th->setOnStage(false,false);
	th->tokens.clear();
}
ASFUNCTIONBODY_ATOM(MovieClip,AVM1CreateTextField)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	tiny_string instanceName;
	int depth;
	int x;
	int y;
	uint32_t width;
	uint32_t height;
	ARG_CHECK(ARG_UNPACK(instanceName)(depth)(x)(y)(width)(height));
	AVM1TextField* tf = Class<AVM1TextField>::getInstanceS(wrk);
	tf->name = wrk->getSystemState()->getUniqueStringId(instanceName);
	tf->setX(x);
	tf->setY(y);
	tf->width = width;
	tf->height = height;
	th->_addChildAt(tf,depth);
	if(tf->name != BUILTIN_STRINGS::EMPTY)
	{
		tf->incRef();
		multiname objName(nullptr);
		objName.name_type=multiname::NAME_STRING;
		objName.name_s_id=tf->name;
		objName.ns.emplace_back(wrk->getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
		asAtom v = asAtomHandler::fromObjectNoPrimitive(tf);
		th->setVariableByMultiname(objName,v,ASObject::CONST_NOT_ALLOWED,nullptr,wrk);
	}
	if (wrk->getSystemState()->getSwfVersion() >= 8)
	{
		tf->incRef();
		ret = asAtomHandler::fromObjectNoPrimitive(tf);
	}
}


void DisplayObjectContainer::sinit(Class_base* c)
{
	CLASS_SETUP(c, InteractiveObject, _constructor, CLASS_SEALED);
	c->isReusable = true;
	c->setDeclaredMethodByQName("numChildren","",Class<IFunction>::getFunction(c->getSystemState(),_getNumChildren,0,Class<Integer>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("getChildIndex","",Class<IFunction>::getFunction(c->getSystemState(),_getChildIndex,1,Class<Integer>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setChildIndex","",Class<IFunction>::getFunction(c->getSystemState(),_setChildIndex),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getChildAt","",Class<IFunction>::getFunction(c->getSystemState(),getChildAt,1,Class<DisplayObject>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getChildByName","",Class<IFunction>::getFunction(c->getSystemState(),getChildByName,1,Class<DisplayObject>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getObjectsUnderPoint","",Class<IFunction>::getFunction(c->getSystemState(),getObjectsUnderPoint,1,Class<Array>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("addChild","",Class<IFunction>::getFunction(c->getSystemState(),addChild,1,Class<DisplayObject>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("removeChild","",Class<IFunction>::getFunction(c->getSystemState(),removeChild,1,Class<DisplayObject>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("removeChildAt","",Class<IFunction>::getFunction(c->getSystemState(),removeChildAt,1,Class<DisplayObject>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("removeChildren","",Class<IFunction>::getFunction(c->getSystemState(),removeChildren),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("addChildAt","",Class<IFunction>::getFunction(c->getSystemState(),addChildAt,2,Class<DisplayObject>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("swapChildren","",Class<IFunction>::getFunction(c->getSystemState(),swapChildren),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("swapChildrenAt","",Class<IFunction>::getFunction(c->getSystemState(),swapChildrenAt),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("contains","",Class<IFunction>::getFunction(c->getSystemState(),contains),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("mouseChildren","",Class<IFunction>::getFunction(c->getSystemState(),_setMouseChildren,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("mouseChildren","",Class<IFunction>::getFunction(c->getSystemState(),_getMouseChildren),GETTER_METHOD,true);
	REGISTER_GETTER_SETTER(c, tabChildren);
}

ASFUNCTIONBODY_GETTER_SETTER(DisplayObjectContainer, tabChildren)

DisplayObjectContainer::DisplayObjectContainer(ASWorker* wrk, Class_base* c):InteractiveObject(wrk,c),mouseChildren(true),
	boundsrectXmin(0),boundsrectYmin(0),boundsrectXmax(0),boundsrectYmax(0),boundsrectdirty(true),tabChildren(true)
{
	subtype=SUBTYPE_DISPLAYOBJECTCONTAINER;
}

void DisplayObjectContainer::markAsChanged()
{
	for (auto it = dynamicDisplayList.begin(); it != dynamicDisplayList.end(); it++)
		(*it)->markAsChanged();
	DisplayObject::markAsChanged();
}

void DisplayObjectContainer::markBoundsRectDirtyChildren()
{
	markBoundsRectDirty();
	for (auto it = dynamicDisplayList.begin(); it != dynamicDisplayList.end(); it++)
	{
		if ((*it)->is<DisplayObjectContainer>())
			(*it)->as<DisplayObjectContainer>()->markBoundsRectDirtyChildren();
	}
}

void DisplayObjectContainer::setChildrenCachedAsBitmapOf(DisplayObject* cachedBitmapObject)
{
	if (this != cachedBitmapObject)
		this->cachedAsBitmapOf=cachedBitmapObject;
	for (auto it = dynamicDisplayList.begin(); it != dynamicDisplayList.end(); it++)
	{
		if ((*it)->is<DisplayObjectContainer>())
			(*it)->as<DisplayObjectContainer>()->setChildrenCachedAsBitmapOf(cachedBitmapObject);
		else
			(*it)->cachedAsBitmapOf=cachedBitmapObject;
	}
}

bool DisplayObjectContainer::hasLegacyChildAt(int32_t depth)
{
	auto i = mapDepthToLegacyChild.find(depth);
	return i != mapDepthToLegacyChild.end();
}
DisplayObject* DisplayObjectContainer::getLegacyChildAt(int32_t depth)
{
	return mapDepthToLegacyChild.at(depth);
}


void DisplayObjectContainer::setupClipActionsAt(int32_t depth,const CLIPACTIONS& actions)
{
	if(!hasLegacyChildAt(depth))
	{
		LOG(LOG_ERROR,"setupClipActionsAt: no child at depth "<<depth);
		return;
	}
	DisplayObject* o = getLegacyChildAt(depth);
	if (o->is<MovieClip>())
		o->as<MovieClip>()->setupActions(actions);
}

void DisplayObjectContainer::checkRatioForLegacyChildAt(int32_t depth,uint32_t ratio,bool inskipping)
{
	if(!hasLegacyChildAt(depth))
	{
		LOG(LOG_ERROR,"checkRatioForLegacyChildAt: no child at that depth "<<depth<<" "<<this->toDebugString());
		return;
	}
	mapDepthToLegacyChild.at(depth)->checkRatio(ratio,inskipping);
	this->hasChanged=true;
}
void DisplayObjectContainer::checkColorTransformForLegacyChildAt(int32_t depth,const CXFORMWITHALPHA& colortransform)
{
	if(!hasLegacyChildAt(depth))
		return;
	DisplayObject* o = mapDepthToLegacyChild.at(depth);
	if (o->colorTransform.isNull())
		o->colorTransform=_NR<ColorTransform>(Class<ColorTransform>::getInstanceS(getInstanceWorker(),colortransform));
	else
		o->colorTransform->setProperties(colortransform);
	markAsChanged();
}

void DisplayObjectContainer::deleteLegacyChildAt(int32_t depth, bool inskipping)
{
	if(!hasLegacyChildAt(depth))
		return;
	DisplayObject* obj = mapDepthToLegacyChild.at(depth);
	if(obj->name != BUILTIN_STRINGS::EMPTY)
	{
		// it seems adobe keeps removed objects and reuses them if they are added through placeObjectTags again
		obj->incRef();
		namedRemovedLegacyChildren[obj->name] = _MR(obj);
		//The variable is not deleted, but just set to null
		//This is a tested behavior
		multiname objName(nullptr);
		objName.name_type=multiname::NAME_STRING;
		objName.name_s_id=obj->name;
		objName.ns.emplace_back(getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
		setVariableByMultiname(objName,needsActionScript3() ? asAtomHandler::nullAtom : asAtomHandler::undefinedAtom, ASObject::CONST_NOT_ALLOWED,nullptr,loadedFrom->getInstanceWorker());
		
	}

	obj->afterLegacyDelete(this,inskipping);
	//this also removes it from depthToLegacyChild
	bool ret = _removeChild(obj,false,inskipping);
	assert_and_throw(ret);
}

void DisplayObjectContainer::insertLegacyChildAt(int32_t depth, DisplayObject* obj, bool inskipping, bool fromtag)
{
	if(hasLegacyChildAt(depth))
	{
		LOG(LOG_ERROR,"insertLegacyChildAt: there is already one child at that depth");
		return;
	}
	
	uint32_t insertpos = 0;
	// find DisplayObject to insert obj after
	DisplayObject* preobj=nullptr;
	for (auto it = mapDepthToLegacyChild.begin(); it != mapDepthToLegacyChild.end();it++)
	{
		if (it->first > depth)
			break;
		preobj = it->second;
	}
	if (preobj)
	{
		auto it=find(dynamicDisplayList.begin(),dynamicDisplayList.end(),preobj);
		if(it!=dynamicDisplayList.end())
		{
			insertpos = it-dynamicDisplayList.begin()+1;
			it++;
			if(fromtag)
			{
				while(it!=dynamicDisplayList.end() && !(*it)->legacy)
				{
					it++; // skip all children previously added through actionscript
					insertpos++;
				}
			}
		}
	}
	
	_addChildAt(obj,insertpos,inskipping);
	if(obj->name != BUILTIN_STRINGS::EMPTY)
	{
		multiname objName(nullptr);
		objName.name_type=multiname::NAME_STRING;
		objName.name_s_id=obj->name;
		objName.ns.emplace_back(getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
		bool set=true;
		if (!loadedFrom->usesActionScript3 && !obj->legacy)
		{
			variable* v = this->findVariableByMultiname(objName,this->getClass(),nullptr,nullptr,true,loadedFrom->getInstanceWorker());
			if (v && asAtomHandler::is<DisplayObject>(v->var))
			{
				// it seems that in AVM1 the variable for a named child is not set if
				// - a variable with the same name already exists and
				// - that variable is a DisplayObject and
				// - the new displayobject was constructed from actionscript and
				// - the depth of the existing DisplayObject is lower than that of the new DisplayObject
				auto it = this->mapLegacyChildToDepth.find(asAtomHandler::as<DisplayObject>(v->var));
				if (it != this->mapLegacyChildToDepth.end() && it->second < depth)
					set = false;
			}
		}
		if (set)
		{
			obj->incRef();
			asAtom v = asAtomHandler::fromObject(obj);
			setVariableByMultiname(objName,v,ASObject::CONST_NOT_ALLOWED,nullptr,loadedFrom->getInstanceWorker());
		}
	}

	mapDepthToLegacyChild.insert(make_pair(depth,obj));
	mapLegacyChildToDepth.insert(make_pair(obj,depth));
	obj->afterLegacyInsert();
}
DisplayObject *DisplayObjectContainer::findLegacyChildByTagID(uint32_t tagid)
{
	auto it = mapLegacyChildToDepth.begin();
	while (it != mapLegacyChildToDepth.end())
	{
		if (it->first->getTagID() == tagid)
			return it->first;
		it++;
	}
	return nullptr;
}
int DisplayObjectContainer::findLegacyChildDepth(DisplayObject *obj)
{
	auto it = mapLegacyChildToDepth.find(obj);
	if (it != mapLegacyChildToDepth.end())
		return it->second;
	return 0;
}

void DisplayObjectContainer::transformLegacyChildAt(int32_t depth, const MATRIX& mat)
{
	if(!hasLegacyChildAt(depth))
		return;
	mapDepthToLegacyChild.at(depth)->setLegacyMatrix(mat);
}

void DisplayObjectContainer::purgeLegacyChildren()
{
	auto i = mapDepthToLegacyChild.begin();
	while( i != mapDepthToLegacyChild.end() )
	{
		if (i->first < 0)
		{
			legacyChildrenMarkedForDeletion.insert(i->first);
			DisplayObject* obj = i->second;
			obj->markedForLegacyDeletion=true;
			if(obj->name != BUILTIN_STRINGS::EMPTY)
			{
				multiname objName(nullptr);
				objName.name_type=multiname::NAME_STRING;
				objName.name_s_id=obj->name;
				objName.ns.emplace_back(getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
				setVariableByMultiname(objName,needsActionScript3() ? asAtomHandler::nullAtom : asAtomHandler::undefinedAtom,ASObject::CONST_NOT_ALLOWED,nullptr,loadedFrom->getInstanceWorker());
			}
		}
		i++;
	}
}
uint32_t DisplayObjectContainer::getMaxLegacyChildDepth()
{
	auto it = mapDepthToLegacyChild.begin();
	int32_t max =-1;
	while (it !=mapDepthToLegacyChild.end())
	{
		if (it->first > max)
			max = it->first;
		it++;
	}
	return max >= 0 ? max : UINT32_MAX;
}
void DisplayObjectContainer::checkClipDepth()
{
	DisplayObject* clipobj = nullptr;
	int depth = 0;
	for (auto it=mapDepthToLegacyChild.begin(); it != mapDepthToLegacyChild.end(); it++)
	{
		DisplayObject* obj = it->second;
		depth = it->first;
		if (obj->ClipDepth)
		{
//			if (clipobj)
//				clipobj->hasChanged = false; // ensure clipobj is not rendered
			clipobj = obj;
		}
		else if (clipobj && clipobj->ClipDepth > depth)
		{
			clipobj->incRef();
			obj->setMask(_NR<DisplayObject>(clipobj));
		}
		else
			obj->setMask(NullRef);
	}
//	if (clipobj)
//		clipobj->hasChanged = false; // ensure clipobj is not rendered
}

bool DisplayObjectContainer::destruct()
{
	// clear all member variables in the display list first to properly handle cyclic reference detection
	prepareDestruction();
	clearDisplayList();
	mouseChildren = true;
	boundsrectdirty = true;
	tabChildren = true;
	legacyChildrenMarkedForDeletion.clear();
	mapDepthToLegacyChild.clear();
	mapLegacyChildToDepth.clear();
	namedRemovedLegacyChildren.clear();
	return InteractiveObject::destruct();
}

void DisplayObjectContainer::finalize()
{
	// clear all member variables in the display list first to properly handle cyclic reference detection
	prepareDestruction();
	clearDisplayList();
	legacyChildrenMarkedForDeletion.clear();
	mapDepthToLegacyChild.clear();
	mapLegacyChildToDepth.clear();
	namedRemovedLegacyChildren.clear();
	InteractiveObject::finalize();
}

void DisplayObjectContainer::prepareShutdown()
{
	if (this->preparedforshutdown)
		return;
	InteractiveObject::prepareShutdown();
	for (auto it = dynamicDisplayList.begin(); it != dynamicDisplayList.end(); it++)
		(*it)->prepareShutdown();
	for (auto it = namedRemovedLegacyChildren.begin(); it != namedRemovedLegacyChildren.end(); it++)
	{
		if ((*it).second)
			(*it).second->prepareShutdown();
	}
}

bool DisplayObjectContainer::countCylicMemberReferences(garbagecollectorstate& gcstate)
{
	if (gcstate.checkAncestors(this))
		return false;
	bool ret = InteractiveObject::countCylicMemberReferences(gcstate);
	Locker l(mutexDisplayList);
	for (auto it = dynamicDisplayList.begin(); it != dynamicDisplayList.end(); it++)
	{
		ret = (*it)->countAllCylicMemberReferences(gcstate) || ret;
	}
	return ret;
}

void DisplayObjectContainer::cloneDisplayList(std::vector<Ref<DisplayObject> >& displayListCopy)
{
	Locker l(mutexDisplayList);
	displayListCopy.reserve(dynamicDisplayList.size());
	for (auto it = dynamicDisplayList.begin(); it != dynamicDisplayList.end(); it++)
	{
		(*it)->incRef();
		displayListCopy.push_back(_MR(*it));
	}
}

InteractiveObject::InteractiveObject(ASWorker* wrk, Class_base* c):DisplayObject(wrk,c),mouseEnabled(true),doubleClickEnabled(false),accessibilityImplementation(NullRef),contextMenu(NullRef),tabEnabled(false),tabIndex(-1)
{
	subtype=SUBTYPE_INTERACTIVE_OBJECT;
}

void InteractiveObject::setOnStage(bool staged, bool force,bool inskipping)
{
	if (!staged)
		getSystemState()->stage->checkResetFocusTarget(this);
	DisplayObject::setOnStage(staged,force,inskipping);
}
InteractiveObject::~InteractiveObject()
{
}

ASFUNCTIONBODY_ATOM(InteractiveObject,_constructor)
{
	EventDispatcher::_constructor(ret,wrk,obj,nullptr,0);
}

ASFUNCTIONBODY_ATOM(InteractiveObject,_setMouseEnabled)
{
	InteractiveObject* th=asAtomHandler::as<InteractiveObject>(obj);
	assert_and_throw(argslen==1);
	th->mouseEnabled=asAtomHandler::Boolean_concrete(args[0]);
}

ASFUNCTIONBODY_ATOM(InteractiveObject,_getMouseEnabled)
{
	InteractiveObject* th=asAtomHandler::as<InteractiveObject>(obj);
	asAtomHandler::setBool(ret,th->mouseEnabled);
}

ASFUNCTIONBODY_ATOM(InteractiveObject,_setDoubleClickEnabled)
{
	InteractiveObject* th=asAtomHandler::as<InteractiveObject>(obj);
	assert_and_throw(argslen==1);
	th->doubleClickEnabled=asAtomHandler::Boolean_concrete(args[0]);
}

ASFUNCTIONBODY_ATOM(InteractiveObject,_getDoubleClickEnabled)
{
	InteractiveObject* th=asAtomHandler::as<InteractiveObject>(obj);
	asAtomHandler::setBool(ret,th->doubleClickEnabled);
}

bool InteractiveObject::destruct()
{
	contextMenu.reset();
	mouseEnabled = true;
	doubleClickEnabled =false;
	accessibilityImplementation.reset();
	focusRect.reset();
	tabEnabled = false;
	tabIndex = -1;
	return DisplayObject::destruct();
}
void InteractiveObject::finalize()
{
	contextMenu.reset();
	accessibilityImplementation.reset();
	focusRect.reset();
	DisplayObject::finalize();
}

void InteractiveObject::prepareShutdown()
{
	if (preparedforshutdown)
		return;
	DisplayObject::prepareShutdown();
	if (contextMenu)
		contextMenu->prepareShutdown();
	if (accessibilityImplementation)
		accessibilityImplementation->prepareShutdown();
	if (focusRect)
		focusRect->prepareShutdown();
}

bool InteractiveObject::countCylicMemberReferences(garbagecollectorstate& gcstate)
{
	if (gcstate.checkAncestors(this))
		return false;
	bool ret = DisplayObject::countCylicMemberReferences(gcstate);
	if (contextMenu)
		ret = contextMenu->countAllCylicMemberReferences(gcstate) || ret;
	if (accessibilityImplementation)
		ret = accessibilityImplementation->countAllCylicMemberReferences(gcstate) || ret;
	if (focusRect)
		ret = focusRect->countAllCylicMemberReferences(gcstate) || ret;
	return ret;
}

void InteractiveObject::defaultEventBehavior(Ref<Event> e)
{
	if(mouseEnabled && e->type == "contextMenu")
	{
		SDL_Event event;
		SDL_zero(event);
		event.type = LS_USEREVENT_OPEN_CONTEXTMENU;
		event.user.data1 = (void*)this;
		SDL_PushEvent(&event);
	}
}

_NR<InteractiveObject> InteractiveObject::getCurrentContextMenuItems(std::vector<_R<NativeMenuItem>>& items)
{
	if (this->contextMenu.isNull() || !this->contextMenu->is<ContextMenu>())
	{
		if (this->getParent())
			return getParent()->getCurrentContextMenuItems(items);
		ContextMenu::getVisibleBuiltinContextMenuItems(nullptr,items,getInstanceWorker());
	}
	else
		this->contextMenu->as<ContextMenu>()->getCurrentContextMenuItems(items);
	this->incRef();
	return _MR<InteractiveObject>(this);
}

void InteractiveObject::sinit(Class_base* c)
{
	CLASS_SETUP(c, DisplayObject, _constructor, CLASS_SEALED);
	c->isReusable = true;
	c->setDeclaredMethodByQName("mouseEnabled","",Class<IFunction>::getFunction(c->getSystemState(),_setMouseEnabled),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("mouseEnabled","",Class<IFunction>::getFunction(c->getSystemState(),_getMouseEnabled,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("doubleClickEnabled","",Class<IFunction>::getFunction(c->getSystemState(),_setDoubleClickEnabled),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("doubleClickEnabled","",Class<IFunction>::getFunction(c->getSystemState(),_getDoubleClickEnabled,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	REGISTER_GETTER_SETTER_RESULTTYPE(c, accessibilityImplementation,AccessibilityImplementation);
	REGISTER_GETTER_SETTER_RESULTTYPE(c, contextMenu,ASObject);
	REGISTER_GETTER_SETTER_RESULTTYPE(c, tabEnabled,Boolean);
	REGISTER_GETTER_SETTER_RESULTTYPE(c, tabIndex,Integer);
	REGISTER_GETTER_SETTER_RESULTTYPE(c, focusRect,ASObject);
}

ASFUNCTIONBODY_GETTER_SETTER(InteractiveObject, accessibilityImplementation)
ASFUNCTIONBODY_GETTER_SETTER_CB(InteractiveObject, contextMenu,onContextMenu)
ASFUNCTIONBODY_GETTER_SETTER(InteractiveObject, tabEnabled)
ASFUNCTIONBODY_GETTER_SETTER(InteractiveObject, tabIndex)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(InteractiveObject, focusRect) // stub

void InteractiveObject::onContextMenu(_NR<ASObject> /*oldValue*/)
{
	if (this->contextMenu->is<ContextMenu>())
		this->contextMenu->as<ContextMenu>()->owner = this;
}

void DisplayObjectContainer::dumpDisplayList(unsigned int level)
{
	tiny_string indent(std::string(2*level, ' '));
	auto it=dynamicDisplayList.begin();
	for(;it!=dynamicDisplayList.end();++it)
	{
		Vector2f pos = (*it)->getXY();
		LOG(LOG_INFO, indent << (*it)->toDebugString() <<
		    " (" << pos.x << "," << pos.y << ") " <<
		    (*it)->getNominalWidth() << "x" << (*it)->getNominalHeight() << " " <<
		    ((*it)->isVisible() ? "v" : "") <<
		    ((*it)->isMask() ? "m" : "") << " cd=" <<(*it)->ClipDepth<<" ca=" <<(*it)->computeCacheAsBitmap()<<"/"<<(*it)->cachedAsBitmapOf<<" "<<
			"a=" << (*it)->clippedAlpha() <<" '"<<getSystemState()->getStringFromUniqueId((*it)->name)<<"'");

		if ((*it)->is<DisplayObjectContainer>())
		{
			(*it)->as<DisplayObjectContainer>()->dumpDisplayList(level+1);
		}
	}
	auto i = mapDepthToLegacyChild.begin();
	while( i != mapDepthToLegacyChild.end() )
	{
		LOG(LOG_INFO, indent << "legacy:"<<i->first <<" "<<i->second->toDebugString());
		i++;
	}
}

void DisplayObjectContainer::setOnStage(bool staged, bool force,bool inskipping)
{
	if(staged!=onStage||force)
	{
		//Make a copy of display list before calling setOnStage
		std::vector<_R<DisplayObject>> displayListCopy;
		cloneDisplayList(displayListCopy);
		InteractiveObject::setOnStage(staged,force,inskipping);
		//Notify children
		//calling InteractiveObject::setOnStage may have changed the onStage state of the children,
		//but the addedToStage/removedFromStage event must always be dispatched
		auto it=displayListCopy.begin();
		for(;it!=displayListCopy.end();++it)
			(*it)->setOnStage(staged,true,inskipping);
	}
}

ASFUNCTIONBODY_ATOM(DisplayObjectContainer,_constructor)
{
	InteractiveObject::_constructor(ret,wrk,obj,nullptr,0);
}

ASFUNCTIONBODY_ATOM(DisplayObjectContainer,_getNumChildren)
{
	DisplayObjectContainer* th=asAtomHandler::as<DisplayObjectContainer>(obj);
	asAtomHandler::setInt(ret,wrk,(int32_t)th->dynamicDisplayList.size());
}

ASFUNCTIONBODY_ATOM(DisplayObjectContainer,_getMouseChildren)
{
	DisplayObjectContainer* th=asAtomHandler::as<DisplayObjectContainer>(obj);
	asAtomHandler::setBool(ret,th->mouseChildren);
}

ASFUNCTIONBODY_ATOM(DisplayObjectContainer,_setMouseChildren)
{
	DisplayObjectContainer* th=asAtomHandler::as<DisplayObjectContainer>(obj);
	assert_and_throw(argslen==1);
	th->mouseChildren=asAtomHandler::Boolean_concrete(args[0]);
}

void DisplayObjectContainer::requestInvalidation(InvalidateQueue* q, bool forceTextureRefresh)
{
	DisplayObject::requestInvalidation(q);
	if (requestInvalidationForCacheAsBitmap(q))
		return;
	mutexDisplayList.lock();
	auto tmp = dynamicDisplayList; // use copy of displaylist to avoid deadlock when computing boundsrect for cached bitmaps
	mutexDisplayList.unlock();
	auto it=tmp.begin();
	for(;it!=tmp.end();++it)
	{
		(*it)->hasChanged = true;
		(*it)->requestInvalidation(q,forceTextureRefresh);
	}
}

void DisplayObjectContainer::_addChildAt(DisplayObject* child, unsigned int index, bool inskipping)
{
	//If the child has no parent, set this container to parent
	//If there is a previous parent, purge the child from his list
	if(child->getParent() && !getSystemState()->isInResetParentList(child))
	{
		//Child already in this container
		if(child->getParent()==this)
			return;
		else
		{
			child->getParent()->_removeChild(child,inskipping,this->isOnStage());
		}
	}
	getSystemState()->removeFromResetParentList(child);
	if(!child->needsActionScript3())
		getSystemState()->stage->AVM1AddDisplayObject(child);
	child->setParent(this);
	{
		Locker l(mutexDisplayList);
		//We insert the object in the back of the list
		if(index >= dynamicDisplayList.size())
			dynamicDisplayList.push_back(child);
		else
		{
			auto it=dynamicDisplayList.begin();
			for(unsigned int i=0;i<index;i++)
				++it;
			dynamicDisplayList.insert(it,child);
		}
		child->addStoredMember();
	}
	if (!onStage || child != getSystemState()->mainClip)
		child->setOnStage(onStage,false,inskipping);
	if (computeCacheAsBitmap())
		child->cachedAsBitmapOf=this;
	else
		child->cachedAsBitmapOf=cachedAsBitmapOf;
	if (child->is<DisplayObjectContainer>())
		child->as<DisplayObjectContainer>()->setChildrenCachedAsBitmapOf(child->cachedAsBitmapOf);
}

bool DisplayObjectContainer::_removeChild(DisplayObject* child,bool direct,bool inskipping, bool keeponstage)
{
	if(!child->getParent() || child->getParent()!=this)
		return false;
	if (!needsActionScript3())
		child->removeAVM1Listeners();

	{
		Locker l(mutexDisplayList);
		auto it=find(dynamicDisplayList.begin(),dynamicDisplayList.end(),child);
		if(it==dynamicDisplayList.end())
			return getSystemState()->isInResetParentList(child);
	}
	if (!keeponstage)
		child->setOnStage(false,false,inskipping);
	child->cachedAsBitmapOf=nullptr;
	if (child->is<DisplayObjectContainer>())
		child->as<DisplayObjectContainer>()->setChildrenCachedAsBitmapOf(nullptr);

	{
		Locker l(mutexDisplayList);
		auto it=find(dynamicDisplayList.begin(),dynamicDisplayList.end(),child);

		if (direct || !this->isOnStage() || inskipping)
			child->setParent(nullptr);
		else
			getSystemState()->addDisplayObjectToResetParentList(child);
		child->setMask(NullRef);
		
		//Erase this from the legacy child map (if it is in there)
		auto it2 = mapLegacyChildToDepth.find(child);
		if (it2 != mapLegacyChildToDepth.end())
		{
			mapDepthToLegacyChild.erase(it2->second);
			mapLegacyChildToDepth.erase(it2);
		}
		(*it)->removeStoredMember();
		dynamicDisplayList.erase(it);
	}
	return true;
}

void DisplayObjectContainer::_removeAllChildren()
{
	Locker l(mutexDisplayList);
	auto it=dynamicDisplayList.begin();
	while (it!=dynamicDisplayList.end())
	{
		DisplayObject* child = *it;
		child->setOnStage(false,false);
		getSystemState()->addDisplayObjectToResetParentList(child);
		child->setMask(NullRef);
		if (!needsActionScript3())
			child->removeAVM1Listeners();

		//Erase this from the legacy child map (if it is in there)
		auto it2 = mapLegacyChildToDepth.find(child);
		if (it2 != mapLegacyChildToDepth.end())
		{
			mapDepthToLegacyChild.erase(it2->second);
			mapLegacyChildToDepth.erase(it2);
		}
		it = dynamicDisplayList.erase(it);
		child->removeStoredMember();
	}
}

void DisplayObjectContainer::removeAVM1Listeners()
{
	if (needsActionScript3())
		return;
	Locker l(mutexDisplayList);
	auto it=dynamicDisplayList.begin();
	while (it!=dynamicDisplayList.end())
	{
		(*it)->removeAVM1Listeners();
		it++;
	}
	DisplayObject::removeAVM1Listeners();
}

bool DisplayObjectContainer::_contains(DisplayObject* d)
{
	if(d==this)
		return true;

	auto it=dynamicDisplayList.begin();
	for(;it!=dynamicDisplayList.end();++it)
	{
		if(*it==d)
			return true;
		if((*it)->is<DisplayObjectContainer>() && (*it)->as<DisplayObjectContainer>()->_contains(d))
			return true;
	}
	return false;
}

ASFUNCTIONBODY_ATOM(DisplayObjectContainer,contains)
{
	DisplayObjectContainer* th=asAtomHandler::as<DisplayObjectContainer>(obj);
	assert_and_throw(argslen==1);
	if(!asAtomHandler::is<DisplayObject>(args[0]))
	{
		asAtomHandler::setBool(ret,false);
		return;
	}

	//Cast to object
	DisplayObject* d=asAtomHandler::as<DisplayObject>(args[0]);
	bool res=th->_contains(d);
	asAtomHandler::setBool(ret,res);
}

//Only from VM context
ASFUNCTIONBODY_ATOM(DisplayObjectContainer,addChildAt)
{
	DisplayObjectContainer* th=asAtomHandler::as<DisplayObjectContainer>(obj);
	assert_and_throw(argslen==2);
	if(asAtomHandler::isClass(args[0]) || asAtomHandler::isNull(args[0]))
	{
		asAtomHandler::setNull(ret);
		return;
	}
	//Validate object type
	assert_and_throw(asAtomHandler::is<DisplayObject>(args[0]));

	int index=asAtomHandler::toInt(args[1]);

	//Cast to object
	DisplayObject* d=asAtomHandler::as<DisplayObject>(args[0]);
	assert_and_throw(index >= 0 && (size_t)index<=th->dynamicDisplayList.size());
	d->incRef();
	th->_addChildAt(d,index);

	//Notify the object
	d->incRef();
	getVm(wrk->getSystemState())->addEvent(_MR(d),_MR(Class<Event>::getInstanceS(wrk,"added")));

	//incRef again as the value is getting returned
	d->incRef();
	ret = asAtomHandler::fromObject(d);
}

ASFUNCTIONBODY_ATOM(DisplayObjectContainer,addChild)
{
	DisplayObjectContainer* th=asAtomHandler::as<DisplayObjectContainer>(obj);
	assert_and_throw(argslen==1);
	if(asAtomHandler::isClass(args[0]) || asAtomHandler::isNull(args[0]))
	{
		asAtomHandler::setNull(ret);
		return;
	}
	//Validate object type
	assert_and_throw(asAtomHandler::is<DisplayObject>(args[0]));

	//Cast to object
	DisplayObject* d=asAtomHandler::as<DisplayObject>(args[0]);
	d->incRef();
	th->_addChildAt(d,numeric_limits<unsigned int>::max());

	//Notify the object
	d->incRef();
	getVm(wrk->getSystemState())->addEvent(_MR(d),_MR(Class<Event>::getInstanceS(wrk,"added")));

	d->incRef();
	ret = asAtomHandler::fromObject(d);
}

//Only from VM context
ASFUNCTIONBODY_ATOM(DisplayObjectContainer,removeChild)
{
	DisplayObjectContainer* th=asAtomHandler::as<DisplayObjectContainer>(obj);
	assert_and_throw(argslen==1);
	if(!asAtomHandler::is<DisplayObject>(args[0]))
	{
		asAtomHandler::setNull(ret);
		return;
	}
	//Cast to object
	DisplayObject* d=asAtomHandler::as<DisplayObject>(args[0]);
	//As we return the child we have to incRef it
	d->incRef();

	if(!th->_removeChild(d))
	{
		createError<ArgumentError>(wrk,2025,"removeChild: child not in list");
		return;
	}

	ret = asAtomHandler::fromObject(d);
}

//Only from VM context
ASFUNCTIONBODY_ATOM(DisplayObjectContainer,removeChildAt)
{
	DisplayObjectContainer* th=asAtomHandler::as<DisplayObjectContainer>(obj);
	assert_and_throw(argslen==1);
	//Validate object type
	int32_t index=asAtomHandler::toInt(args[0]);

	DisplayObject* child;
	{
		Locker l(th->mutexDisplayList);
		if(index>=int(th->dynamicDisplayList.size()) || index<0)
		{
			createError<RangeError>(wrk,2025,"removeChildAt: invalid index");
			return;
		}
		auto it=th->dynamicDisplayList.begin();
		for(int32_t i=0;i<index;i++)
			++it;
		child=(*it);
		//Erase this from the legacy child map (if it is in there)
		auto it2 = th->mapLegacyChildToDepth.find(child);
		if (it2 != th->mapLegacyChildToDepth.end())
		{
			th->mapDepthToLegacyChild.erase(it2->second);
			th->mapLegacyChildToDepth.erase(it2);
		}
		child->setOnStage(false,false);
		wrk->getSystemState()->addDisplayObjectToResetParentList(child);
		//As we return the child we incRef it
		child->incRef();
		th->dynamicDisplayList.erase(it);
		child->removeStoredMember();
	}
	ret = asAtomHandler::fromObject(child);
}

ASFUNCTIONBODY_ATOM(DisplayObjectContainer,removeChildren)
{
	uint32_t beginindex;
	uint32_t endindex;
	ARG_CHECK(ARG_UNPACK(beginindex,0)(endindex,0x7fffffff));
	DisplayObjectContainer* th=asAtomHandler::as<DisplayObjectContainer>(obj);
	{
		Locker l(th->mutexDisplayList);
		if (endindex > th->dynamicDisplayList.size())
			endindex = (uint32_t)th->dynamicDisplayList.size();
		auto it = th->dynamicDisplayList.begin()+beginindex;
		while (it != th->dynamicDisplayList.begin()+endindex)
		{
			(*it)->removeStoredMember();
			it++;
		}
		th->dynamicDisplayList.erase(th->dynamicDisplayList.begin()+beginindex,th->dynamicDisplayList.begin()+endindex);
	}
}
ASFUNCTIONBODY_ATOM(DisplayObjectContainer,_setChildIndex)
{
	DisplayObjectContainer* th=asAtomHandler::as<DisplayObjectContainer>(obj);
	_NR<DisplayObject> ch;
	int32_t index;
	ARG_CHECK(ARG_UNPACK(ch)(index))
	if (ch.isNull() || ch->getParent() != th)
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError);
		return;
	}
	DisplayObject* child = ch.getPtr();
	int curIndex = th->getChildIndex(child);
	if(curIndex == index || curIndex < 0)
		return;
	Locker l(th->mutexDisplayList);
	if (index < 0 || index > (int)th->dynamicDisplayList.size())
	{
		createError<RangeError>(wrk,kParamRangeError);
		return;
	}
	auto itrem = th->dynamicDisplayList.begin()+curIndex;
	th->dynamicDisplayList.erase(itrem); //remove from old position

	auto it=th->dynamicDisplayList.begin();
	int i = 0;
	for(;it != th->dynamicDisplayList.end(); ++it)
		if(i++ == index)
		{
			th->dynamicDisplayList.insert(it, child);
			return;
		}
	th->dynamicDisplayList.push_back(child);
}

ASFUNCTIONBODY_ATOM(DisplayObjectContainer,swapChildren)
{
	DisplayObjectContainer* th=asAtomHandler::as<DisplayObjectContainer>(obj);
	assert_and_throw(argslen==2);
	
	//Validate object type
	assert_and_throw(asAtomHandler::is<DisplayObject>(args[0]));
	assert_and_throw(asAtomHandler::is<DisplayObject>(args[1]));

	if (asAtomHandler::getObject(args[0]) == asAtomHandler::getObject(args[1]))
	{
		// Must return, otherwise crashes trying to erase the
		// same object twice
		return;
	}

	//Cast to object
	DisplayObject* child1=asAtomHandler::as<DisplayObject>(args[0]);
	DisplayObject* child2=asAtomHandler::as<DisplayObject>(args[1]);

	{
		Locker l(th->mutexDisplayList);
		auto it1=find(th->dynamicDisplayList.begin(),th->dynamicDisplayList.end(),child1);
		auto it2=find(th->dynamicDisplayList.begin(),th->dynamicDisplayList.end(),child2);
		if(it1==th->dynamicDisplayList.end() || it2==th->dynamicDisplayList.end())
		{
			createError<ArgumentError>(wrk,2025,"Argument is not child of this object");
			return;
		}

		std::iter_swap(it1, it2);
	}
}

ASFUNCTIONBODY_ATOM(DisplayObjectContainer,swapChildrenAt)
{
	DisplayObjectContainer* th=asAtomHandler::as<DisplayObjectContainer>(obj);
	int index1;
	int index2;
	ARG_CHECK(ARG_UNPACK(index1)(index2));

	if ((index1 < 0) || (index1 > (int)th->dynamicDisplayList.size()) ||
		(index2 < 0) || (index2 > (int)th->dynamicDisplayList.size()))
	{
		createError<RangeError>(wrk,kParamRangeError);
		return;
	}
	if (index1 == index2)
	{
		return;
	}

	{
		Locker l(th->mutexDisplayList);
		std::iter_swap(th->dynamicDisplayList.begin() + index1, th->dynamicDisplayList.begin() + index2);
	}
}

//Only from VM context
ASFUNCTIONBODY_ATOM(DisplayObjectContainer,getChildByName)
{
	DisplayObjectContainer* th=asAtomHandler::as<DisplayObjectContainer>(obj);
	assert_and_throw(argslen==1);
	uint32_t wantedName=asAtomHandler::toStringId(args[0],wrk);
	auto it=th->dynamicDisplayList.begin();
	ASObject* res=nullptr;
	for(;it!=th->dynamicDisplayList.end();++it)
	{
		if((*it)->name==wantedName)
		{
			res=(*it);
			break;
		}
	}
	if(res)
	{
		res->incRef();
		ret = asAtomHandler::fromObject(res);
	}
	else
		asAtomHandler::setUndefined(ret);
}

//Only from VM context
ASFUNCTIONBODY_ATOM(DisplayObjectContainer,getChildAt)
{
	DisplayObjectContainer* th=asAtomHandler::as<DisplayObjectContainer>(obj);
	assert_and_throw(argslen==1);
	unsigned int index=asAtomHandler::toInt(args[0]);
	if(index>=th->dynamicDisplayList.size())
	{
		createError<RangeError>(wrk,2025,"getChildAt: invalid index");
		return;
	}
	auto it=th->dynamicDisplayList.begin();
	for(unsigned int i=0;i<index;i++)
		++it;

	(*it)->incRef();
	ret = asAtomHandler::fromObject(*it);
}

int DisplayObjectContainer::getChildIndex(DisplayObject* child)
{
	auto it = dynamicDisplayList.begin();
	int ret = 0;
	do
	{
		if(it == dynamicDisplayList.end())
		{
			createError<ArgumentError>(getInstanceWorker(),2025,"getChildIndex: child not in list");
			return -1;
		}
		if(*it == child)
			break;
		ret++;
		++it;
	}
	while(1);
	return ret;
}

//Only from VM context
ASFUNCTIONBODY_ATOM(DisplayObjectContainer,_getChildIndex)
{
	DisplayObjectContainer* th=asAtomHandler::as<DisplayObjectContainer>(obj);
	assert_and_throw(argslen==1);
	//Validate object type
	assert_and_throw(asAtomHandler::is<DisplayObject>(args[0]));

	//Cast to object
	DisplayObject* d= asAtomHandler::as<DisplayObject>(args[0]);

	asAtomHandler::setInt(ret,wrk,th->getChildIndex(d));
}

ASFUNCTIONBODY_ATOM(DisplayObjectContainer,getObjectsUnderPoint)
{
	DisplayObjectContainer* th=asAtomHandler::as<DisplayObjectContainer>(obj);
	_NR<Point> point;
	ARG_CHECK(ARG_UNPACK(point));
	Array* res = Class<Array>::getInstanceSNoArgs(wrk);
	if (!point.isNull())
		th->getObjectsFromPoint(point.getPtr(),res);
	ret = asAtomHandler::fromObject(res);
}

void DisplayObjectContainer::getObjectsFromPoint(Point* point, Array *ar)
{
	number_t xmin,xmax,ymin,ymax;
	MATRIX m;
	{
		Locker l(mutexDisplayList);
		auto it = dynamicDisplayList.begin();
		while (it != dynamicDisplayList.end())
		{
			(*it)->incRef();
			if ((*it)->getBounds(xmin,xmax,ymin,ymax,m))
			{
				if (xmin <= point->getX() && xmax >= point->getX()
						&& ymin <= point->getY() && ymax >= point->getY())
				{
					(*it)->incRef();
					ar->push(asAtomHandler::fromObject(*it));
				}
			}
			if ((*it)->is<DisplayObjectContainer>())
				(*it)->as<DisplayObjectContainer>()->getObjectsFromPoint(point,ar);
			it++;
		}

	}
}

void DisplayObjectContainer::clearDisplayList()
{
	auto it = dynamicDisplayList.rbegin();
	while (it != dynamicDisplayList.rend())
	{
		DisplayObject* c = (*it);
		c->setParent(nullptr);
		getSystemState()->removeFromResetParentList(c);
		dynamicDisplayList.pop_back();
		it = dynamicDisplayList.rbegin();
		c->removeStoredMember();
	}
}

bool Shape::boundsRect(number_t &xmin, number_t &xmax, number_t &ymin, number_t &ymax)
{
	if (!this->legacy || (fromTag==nullptr))
	{
		if (graphics)
			graphics->refreshTokens();
		return TokenContainer::boundsRect(xmin,xmax,ymin,ymax);
	}
	xmin=fromTag->ShapeBounds.Xmin/20.0;
	xmax=fromTag->ShapeBounds.Xmax/20.0;
	ymin=fromTag->ShapeBounds.Ymin/20.0;
	ymax=fromTag->ShapeBounds.Ymax/20.0;
	return true;
}

_NR<DisplayObject> Shape::hitTestImpl(number_t x, number_t y, DisplayObject::HIT_TYPE type, bool interactiveObjectsOnly)
{
	number_t xmin, xmax, ymin, ymax;
	boundsRect(xmin, xmax, ymin, ymax);
	if (x<xmin || x>xmax || y<ymin || y>ymax)
		return NullRef;
	if (TokenContainer::hitTestImpl(x-xmin,y-ymin))
	{
		this->incRef();
		return _MR(this);
	}
	return NullRef;
}

Shape::Shape(ASWorker* wrk, Class_base* c):DisplayObject(wrk,c),TokenContainer(this),graphics(NullRef),fromTag(nullptr)
{
	subtype=SUBTYPE_SHAPE;
}

void Shape::setupShape(DefineShapeTag* tag, float _scaling)
{
	tokens.filltokens.assign(tag->tokens->filltokens.begin(),tag->tokens->filltokens.end());
	tokens.stroketokens.assign(tag->tokens->stroketokens.begin(),tag->tokens->stroketokens.end());
	tokens.canRenderToGL = tag->tokens->canRenderToGL;
	tokens.boundsRect = tag->tokens->boundsRect;
	fromTag = tag;
	// TODO caching of texture currently doesn't work if the DefineShapeTag is used by multiple shape objects with different scaling
//	cachedSurface.isChunkOwner=false;
//	cachedSurface.tex=&tag->chunk;
//	if (tag->chunk.isValid()) // Shape texture was already created, so we don't have to redo it
//		resetNeedsTextureRecalculation();
	scaling=_scaling;
}

uint32_t Shape::getTagID() const 
{
	return fromTag ? fromTag->getId() : UINT32_MAX; 
}

bool Shape::destruct()
{
	graphics.reset();
	fromTag=nullptr;
	tokens.clear();
	return DisplayObject::destruct();
}

void Shape::finalize()
{
	graphics.reset();
	tokens.clear();
	DisplayObject::finalize();
}

void Shape::prepareShutdown()
{
	if (preparedforshutdown)
		return;
	DisplayObject::prepareShutdown();
	if (graphics)
		graphics->prepareShutdown();
}

void Shape::startDrawJob()
{
	if (graphics)
		graphics->startDrawJob();
}

void Shape::endDrawJob()
{
	if (graphics)
		graphics->endDrawJob();
}

void Shape::sinit(Class_base* c)
{
	CLASS_SETUP(c, DisplayObject, _constructor, CLASS_SEALED);
	c->isReusable=true;
	c->setDeclaredMethodByQName("graphics","",Class<IFunction>::getFunction(c->getSystemState(),_getGraphics,0,Class<Graphics>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
}

void Shape::requestInvalidation(InvalidateQueue* q, bool forceTextureRefresh)
{
	if (this->graphics)
		this->graphics->refreshTokens();
	TokenContainer::requestInvalidation(q,forceTextureRefresh);
}

IDrawable *Shape::invalidate(DisplayObject *target, const MATRIX &initialMatrix, bool smoothing, InvalidateQueue* q, _NR<DisplayObject>* cachedBitmap)
{
	if (this->graphics)
		this->graphics->refreshTokens();
	return TokenContainer::invalidate(target, initialMatrix,smoothing ? SMOOTH_MODE::SMOOTH_ANTIALIAS : SMOOTH_MODE::SMOOTH_NONE,q,cachedBitmap,!graphics.isNull());
}

ASFUNCTIONBODY_ATOM(Shape,_constructor)
{
	DisplayObject::_constructor(ret,wrk,obj,nullptr,0);
}

ASFUNCTIONBODY_ATOM(Shape,_getGraphics)
{
	Shape* th=asAtomHandler::as<Shape>(obj);
	if(th->graphics.isNull())
		th->graphics=_MR(Class<Graphics>::getInstanceS(wrk,th));
	th->graphics->incRef();
	ret = asAtomHandler::fromObject(th->graphics.getPtr());
}

MorphShape::MorphShape(ASWorker* wrk, Class_base* c):DisplayObject(wrk,c),TokenContainer(this),morphshapetag(nullptr),currentratio(0)
{
	subtype=SUBTYPE_MORPHSHAPE;
	scaling = 1.0f/20.0f;
}

MorphShape::MorphShape(ASWorker* wrk,Class_base *c, DefineMorphShapeTag* _morphshapetag):DisplayObject(wrk,c),TokenContainer(this),morphshapetag(_morphshapetag),currentratio(0)
{
	subtype=SUBTYPE_MORPHSHAPE;
	scaling = 1.0f/20.0f;
	if (this->morphshapetag)
		this->morphshapetag->getTokensForRatio(tokens,0);
}

void MorphShape::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, DisplayObject, CLASS_SEALED | CLASS_FINAL);
}

IDrawable* MorphShape::invalidate(DisplayObject* target, const MATRIX& initialMatrix, bool smoothing, InvalidateQueue* q, _NR<DisplayObject>* cachedBitmap)
{
	return TokenContainer::invalidate(target, initialMatrix,smoothing ? SMOOTH_MODE::SMOOTH_ANTIALIAS : SMOOTH_MODE::SMOOTH_NONE,q,cachedBitmap,false);
}

bool MorphShape::boundsRect(number_t &xmin, number_t &xmax, number_t &ymin, number_t &ymax)
{
	if (!this->legacy || (morphshapetag==nullptr))
		return TokenContainer::boundsRect(xmin,xmax,ymin,ymax);

	float curratiofactor = float(currentratio)/65535.0;
	xmin=(morphshapetag->StartBounds.Xmin + (float(morphshapetag->EndBounds.Xmin - morphshapetag->StartBounds.Xmin)*curratiofactor))/20.0;
	xmax=(morphshapetag->StartBounds.Xmax + (float(morphshapetag->EndBounds.Xmax - morphshapetag->StartBounds.Xmax)*curratiofactor))/20.0;
	ymin=(morphshapetag->StartBounds.Ymin + (float(morphshapetag->EndBounds.Ymin - morphshapetag->StartBounds.Ymin)*curratiofactor))/20.0;
	ymax=(morphshapetag->StartBounds.Ymax + (float(morphshapetag->EndBounds.Ymax - morphshapetag->StartBounds.Ymax)*curratiofactor))/20.0;

	return true;
}

_NR<DisplayObject> MorphShape::hitTestImpl(number_t x, number_t y, HIT_TYPE type, bool interactiveObjectsOnly)
{
	number_t xmin, xmax, ymin, ymax;
	boundsRect(xmin, xmax, ymin, ymax);
	if (x<xmin || x>xmax || y<ymin || y>ymax)
		return NullRef;
	if (TokenContainer::hitTestImpl(x-xmin,y-ymin))
	{
		this->incRef();
		return _MR(this);
	}
	return NullRef;
}

void MorphShape::checkRatio(uint32_t ratio, bool inskipping)
{
	if (inskipping)
		return;
	currentratio = ratio;
	if (this->morphshapetag)
	{
		this->morphshapetag->getTokensForRatio(tokens,ratio);
		geometryChanged();
	}
	this->hasChanged = true;
	this->setNeedsTextureRecalculation(true);
	if (isOnStage())
		requestInvalidation(getSystemState());
}

uint32_t MorphShape::getTagID() const
{
	return morphshapetag ? morphshapetag->getId():UINT32_MAX;
}


void Stage::sinit(Class_base* c)
{
	CLASS_SETUP(c, DisplayObjectContainer, _constructor, CLASS_SEALED);
	c->setDeclaredMethodByQName("allowFullScreen","",Class<IFunction>::getFunction(c->getSystemState(),_getAllowFullScreen,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("allowFullScreenInteractive","",Class<IFunction>::getFunction(c->getSystemState(),_getAllowFullScreenInteractive,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("colorCorrectionSupport","",Class<IFunction>::getFunction(c->getSystemState(),_getColorCorrectionSupport,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("fullScreenHeight","",Class<IFunction>::getFunction(c->getSystemState(),_getStageHeight,0,Class<UInteger>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("fullScreenWidth","",Class<IFunction>::getFunction(c->getSystemState(),_getStageWidth,0,Class<UInteger>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("stageWidth","",Class<IFunction>::getFunction(c->getSystemState(),_getStageWidth,0,Class<UInteger>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("stageWidth","",Class<IFunction>::getFunction(c->getSystemState(),_setStageWidth),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("stageHeight","",Class<IFunction>::getFunction(c->getSystemState(),_getStageHeight,0,Class<UInteger>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("stageHeight","",Class<IFunction>::getFunction(c->getSystemState(),_setStageHeight),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("width","",Class<IFunction>::getFunction(c->getSystemState(),_getStageWidth,0,Class<UInteger>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("height","",Class<IFunction>::getFunction(c->getSystemState(),_getStageHeight),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("scaleMode","",Class<IFunction>::getFunction(c->getSystemState(),_getScaleMode,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("scaleMode","",Class<IFunction>::getFunction(c->getSystemState(),_setScaleMode),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("loaderInfo","",Class<IFunction>::getFunction(c->getSystemState(),_getLoaderInfo,0,Class<LoaderInfo>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("stageVideos","",Class<IFunction>::getFunction(c->getSystemState(),_getStageVideos,0,Class<Vector>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("focus","",Class<IFunction>::getFunction(c->getSystemState(),_getFocus,0,Class<InteractiveObject>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("focus","",Class<IFunction>::getFunction(c->getSystemState(),_setFocus),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("frameRate","",Class<IFunction>::getFunction(c->getSystemState(),_getFrameRate,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("frameRate","",Class<IFunction>::getFunction(c->getSystemState(),_setFrameRate),SETTER_METHOD,true);
	// override the setter from DisplayObjectContainer
	c->setDeclaredMethodByQName("tabChildren","",Class<IFunction>::getFunction(c->getSystemState(),_setTabChildren),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("wmodeGPU","",Class<IFunction>::getFunction(c->getSystemState(),_getWmodeGPU,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("invalidate","",Class<IFunction>::getFunction(c->getSystemState(),_invalidate),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("color","",Class<IFunction>::getFunction(c->getSystemState(),_getColor,0,Class<UInteger>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("color","",Class<IFunction>::getFunction(c->getSystemState(),_setColor),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("isFocusInaccessible","",Class<IFunction>::getFunction(c->getSystemState(),_isFocusInaccessible,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,align,ASString);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,colorCorrection,ASString);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,displayState,ASString);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,fullScreenSourceRect,Rectangle);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,showDefaultContextMenu,Boolean);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,quality,ASString);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,stageFocusRect,Boolean);
	REGISTER_GETTER_RESULTTYPE(c,allowsFullScreen,Boolean);
	REGISTER_GETTER_RESULTTYPE(c,stage3Ds,Vector);
	REGISTER_GETTER_RESULTTYPE(c,softKeyboardRect,Rectangle);
	REGISTER_GETTER_RESULTTYPE(c,contentsScaleFactor,Number);
	REGISTER_GETTER_RESULTTYPE(c,nativeWindow,NativeWindow);
}

ASFUNCTIONBODY_GETTER_SETTER_STRINGID_CB(Stage,align,onAlign)
ASFUNCTIONBODY_GETTER_SETTER_CB(Stage,colorCorrection,onColorCorrection)
ASFUNCTIONBODY_GETTER_SETTER_CB(Stage,displayState,onDisplayState)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(Stage,showDefaultContextMenu)
ASFUNCTIONBODY_GETTER_SETTER_CB(Stage,fullScreenSourceRect,onFullScreenSourceRect)
ASFUNCTIONBODY_GETTER_SETTER(Stage,quality)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(Stage,stageFocusRect)
ASFUNCTIONBODY_GETTER_NOT_IMPLEMENTED(Stage,allowsFullScreen)
ASFUNCTIONBODY_GETTER(Stage,stage3Ds)
ASFUNCTIONBODY_GETTER_NOT_IMPLEMENTED(Stage,softKeyboardRect)
ASFUNCTIONBODY_GETTER_NOT_IMPLEMENTED(Stage,contentsScaleFactor)
ASFUNCTIONBODY_GETTER(Stage,nativeWindow)

void Stage::onDisplayState(const tiny_string&)
{
	tiny_string s = displayState.lowercase();

	// AVM1 allows case insensitive values, so we correct them here
	if (s=="normal")
		displayState="normal";
	if (s=="fullscreen")
		displayState="fullScreen";
	if (s=="fullscreeninteractive")
		displayState="fullScreenInteractive";

	if (displayState != "normal" && displayState != "fullScreen" && displayState != "fullScreenInteractive")
	{
		LOG(LOG_ERROR,"invalid value for DisplayState");
		return;
	}
	if (!getSystemState()->allowFullscreen && displayState == "fullScreen")
	{
		if (needsActionScript3())
			createError<SecurityError>(getInstanceWorker(),kInvalidParamError);
		return;
	}
	if (!getSystemState()->allowFullscreenInteractive && displayState == "fullScreenInteractive")
	{
		if (needsActionScript3())
			createError<SecurityError>(getInstanceWorker(),kInvalidParamError);
		return;
	}
	LOG(LOG_NOT_IMPLEMENTED,"setting display state does not check for the security sandbox!");
	getSystemState()->getEngineData()->setDisplayState(displayState,getSystemState());
}

void Stage::onAlign(uint32_t /*oldValue*/)
{
	LOG(LOG_NOT_IMPLEMENTED, "Stage.align = " << getSystemState()->getStringFromUniqueId(align));
}

void Stage::onColorCorrection(const tiny_string& oldValue)
{
	if (colorCorrection != "default" && 
	    colorCorrection != "on" && 
	    colorCorrection != "off")
	{
		colorCorrection = oldValue;
		createError<ArgumentError>(this->getInstanceWorker(),kInvalidEnumError, "colorCorrection");
	}
}

void Stage::onFullScreenSourceRect(_NR<Rectangle> /*oldValue*/)
{
	LOG(LOG_NOT_IMPLEMENTED, "Stage.fullScreenSourceRect");
	fullScreenSourceRect.reset();
}

void Stage::defaultEventBehavior(_R<Event> e)
{
	if (e->type == "keyDown")
	{
		KeyboardEvent* ev = e->as<KeyboardEvent>();
		uint32_t modifiers = ev->getModifiers() & (KMOD_LSHIFT | KMOD_RSHIFT |KMOD_LCTRL | KMOD_RCTRL | KMOD_LALT | KMOD_RALT);
		if (modifiers == KMOD_NONE)
		{
			switch (ev->getKeyCode())
			{
				case AS3KEYCODE_ESCAPE:
					if (getSystemState()->getEngineData()->inFullScreenMode())
						getSystemState()->getEngineData()->setDisplayState("normal",getSystemState());
					break;
				default:
					break;
			}
		}
	}
}

void Stage::eventListenerAdded(const tiny_string& eventName)
{
	if (eventName == "stageVideoAvailability")
	{
		// StageVideoAvailabilityEvent is dispatched directly after an eventListener is added
		// see https://www.adobe.com/devnet/flashplayer/articles/stage_video.html 
		this->incRef();
		getVm(getSystemState())->addEvent(_MR(this),_MR(Class<StageVideoAvailabilityEvent>::getInstanceS(getInstanceWorker())));
	}
}
bool Stage::renderStage3D()
{
	for (uint32_t i = 0; i < stage3Ds->size(); i++)
	{
		asAtom a=stage3Ds->at(i);
		if (!asAtomHandler::as<Stage3D>(a)->context3D.isNull()
				&& asAtomHandler::as<Stage3D>(a)->context3D->backBufferHeight != 0
				&& asAtomHandler::as<Stage3D>(a)->context3D->backBufferWidth != 0
				&& asAtomHandler::as<Stage3D>(a)->visible)
			return true;
	}
	return false;
}
bool Stage::renderImpl(RenderContext &ctxt)
{
	bool has3d = false;
	for (uint32_t i = 0; i < stage3Ds->size(); i++)
	{
		asAtom a=stage3Ds->at(i);
		if (asAtomHandler::as<Stage3D>(a)->renderImpl(ctxt))
			has3d = true;
	}
	if (has3d)
	{
		// setup opengl state for additional 2d rendering
		getSystemState()->getEngineData()->exec_glActiveTexture_GL_TEXTURE0(0);
		getSystemState()->getEngineData()->exec_glBlendFunc(BLEND_ONE,BLEND_ONE_MINUS_SRC_ALPHA);
		getSystemState()->getEngineData()->exec_glUseProgram(((RenderThread&)ctxt).gpu_program);
		((GLRenderContext&)ctxt).lsglLoadIdentity();
		((GLRenderContext&)ctxt).setMatrixUniform(GLRenderContext::LSGL_MODELVIEW);
	}
	return DisplayObjectContainer::renderImpl(ctxt);
}
bool Stage::destruct()
{
	focus.reset();
	root.reset();
	stage3Ds.reset();
	nativeWindow.reset();
	for (auto it = hiddenobjects.begin(); it != hiddenobjects.end(); it++)
		(*it)->removeStoredMember();
	hiddenobjects.clear();
	fullScreenSourceRect.reset();
	softKeyboardRect.reset();

	// erasing an avm1 listener might change the list, so we can't use clear() here
	while (!avm1KeyboardListeners.empty())
	{
		avm1KeyboardListeners.back()->decRef();
		if (!avm1KeyboardListeners.empty())
			avm1KeyboardListeners.pop_back();
	}
	while (!avm1MouseListeners.empty())
	{
		avm1MouseListeners.back()->decRef();
		if (!avm1MouseListeners.empty())
			avm1MouseListeners.pop_back();
	}
	avm1EventListeners.clear();
	while (!avm1ResizeListeners.empty())
	{
		avm1ResizeListeners.back()->decRef();
		if (!avm1ResizeListeners.empty())
			avm1ResizeListeners.pop_back();
	}

	return DisplayObjectContainer::destruct();
}

void Stage::finalize()
{
	focus.reset();
	root.reset();
	stage3Ds.reset();
	nativeWindow.reset();
	for (auto it = hiddenobjects.begin(); it != hiddenobjects.end(); it++)
		(*it)->removeStoredMember();
	hiddenobjects.clear();
	fullScreenSourceRect.reset();
	softKeyboardRect.reset();

	// erasing an avm1 listener might change the list, so we can't use clear() here
	while (!avm1KeyboardListeners.empty())
	{
		avm1KeyboardListeners.back()->decRef();
		if (!avm1KeyboardListeners.empty())
			avm1KeyboardListeners.pop_back();
	}
	while (!avm1MouseListeners.empty())
	{
		avm1MouseListeners.back()->decRef();
		if (!avm1MouseListeners.empty())
			avm1MouseListeners.pop_back();
	}
	avm1EventListeners.clear();
	while (!avm1ResizeListeners.empty())
	{
		avm1ResizeListeners.back()->decRef();
		if (!avm1ResizeListeners.empty())
			avm1ResizeListeners.pop_back();
	}

	DisplayObjectContainer::finalize();
}

void Stage::prepareShutdown()
{
	if (this->preparedforshutdown)
		return;
	DisplayObjectContainer::prepareShutdown();
	if (fullScreenSourceRect)
		fullScreenSourceRect->prepareShutdown();
	if (stage3Ds)
		stage3Ds->prepareShutdown();
	if (softKeyboardRect)
		softKeyboardRect->prepareShutdown();
	if (nativeWindow)
		nativeWindow->prepareShutdown();
	if (focus)
		focus->prepareShutdown();
	if (root)
		root->prepareShutdown();
	for (auto it = hiddenobjects.begin(); it != hiddenobjects.end(); it++)
		(*it)->prepareShutdown();
	for (auto it = avm1KeyboardListeners.begin(); it != avm1KeyboardListeners.end(); it++)
		(*it)->prepareShutdown();
	for (auto it = avm1MouseListeners.begin(); it != avm1MouseListeners.end(); it++)
		(*it)->prepareShutdown();
	avm1EventListeners.clear();
	for (auto it = avm1ResizeListeners.begin(); it != avm1ResizeListeners.end(); it++)
		(*it)->prepareShutdown();
}

Stage::Stage(ASWorker* wrk, Class_base* c):DisplayObjectContainer(wrk,c)
  ,avm1DisplayObjectFirst(nullptr),avm1DisplayObjectLast(nullptr),hasAVM1Clips(false)
  ,align(c->getSystemState()->getUniqueStringId("TL")), colorCorrection("default"),displayState("normal"),showDefaultContextMenu(true),quality("high")
  ,stageFocusRect(false),allowsFullScreen(false),contentsScaleFactor(1.0)
{
	subtype = SUBTYPE_STAGE;
	RELEASE_WRITE(this->invalidated,false);
	onStage = true;
	asAtom v=asAtomHandler::invalidAtom;
	RootMovieClip* root = wrk->rootClip.getPtr();
	Template<Vector>::getInstanceS(wrk,v,root,Class<Stage3D>::getClass(getSystemState()),NullRef);
	stage3Ds = _R<Vector>(asAtomHandler::as<Vector>(v));
	stage3Ds->setRefConstant();
	// according to specs, Desktop computers usually have 4 Stage3D objects available
	ASObject* o = Class<Stage3D>::getInstanceS(wrk);
	o->setRefConstant();
	v =asAtomHandler::fromObject(o);
	stage3Ds->append(v);

	o = Class<Stage3D>::getInstanceS(wrk);
	o->setRefConstant();
	v =asAtomHandler::fromObject(o);
	stage3Ds->append(v);

	o = Class<Stage3D>::getInstanceS(wrk);
	o->setRefConstant();
	v =asAtomHandler::fromObject(o);
	stage3Ds->append(v);

	o = Class<Stage3D>::getInstanceS(wrk);
	o->setRefConstant();
	v =asAtomHandler::fromObject(o);
	stage3Ds->append(v);

	softKeyboardRect = _R<Rectangle>(Class<Rectangle>::getInstanceS(wrk));
	if (wrk->getSystemState()->flashMode == SystemState::AIR)
	{
		nativeWindow = _MR(Class<NativeWindow>::getInstanceSNoArgs(wrk));
		nativeWindow->setRefConstant();
	}
}

_NR<Stage> Stage::getStage()
{
	this->incRef();
	return _MR(this);
}

ASFUNCTIONBODY_ATOM(Stage,_constructor)
{
}

_NR<DisplayObject> Stage::hitTestImpl(number_t x, number_t y, DisplayObject::HIT_TYPE type,bool interactiveObjectsOnly)
{
	_NR<DisplayObject> ret;
	ret = DisplayObjectContainer::hitTestImpl(x, y, type, interactiveObjectsOnly);
	if(!ret)
	{
		/* If nothing else is hit, we hit the stage */
		this->incRef();
		ret = _MNR(this);
	}
	return ret;
}

_NR<RootMovieClip> Stage::getRoot()
{
	return root;
}

void Stage::setRoot(_NR<RootMovieClip> _root)
{
	root = _root;
}

uint32_t Stage::internalGetWidth() const
{
	uint32_t width;
	if(getSystemState()->scaleMode==SystemState::NO_SCALE)
		width=getSystemState()->getRenderThread()->windowWidth;
	else
	{
		RECT size=getSystemState()->mainClip->getFrameSize();
		width=(size.Xmax-size.Xmin)/20;
	}
	return width;
}

uint32_t Stage::internalGetHeight() const
{
	uint32_t height;
	if(getSystemState()->scaleMode==SystemState::NO_SCALE)
		height=getSystemState()->getRenderThread()->windowHeight;
	else
	{
		RECT size=getSystemState()->mainClip->getFrameSize();
		height=(size.Ymax-size.Ymin)/20;
	}
	return height;
}

ASFUNCTIONBODY_ATOM(Stage,_getStageWidth)
{
	Stage* th=asAtomHandler::as<Stage>(obj);
	asAtomHandler::setUInt(ret,wrk,th->internalGetWidth());
}

ASFUNCTIONBODY_ATOM(Stage,_setStageWidth)
{
	//Stage* th=asAtomHandler::as<Stage>(obj);
	LOG(LOG_NOT_IMPLEMENTED,"Stage.stageWidth setter");
}

ASFUNCTIONBODY_ATOM(Stage,_getStageHeight)
{
	Stage* th=asAtomHandler::as<Stage>(obj);
	asAtomHandler::setUInt(ret,wrk,th->internalGetHeight());
}

ASFUNCTIONBODY_ATOM(Stage,_setStageHeight)
{
	//Stage* th=asAtomHandler::as<Stage>(obj);
	LOG(LOG_NOT_IMPLEMENTED,"Stage.stageHeight setter");
}

ASFUNCTIONBODY_ATOM(Stage,_getLoaderInfo)
{
	asAtom a = asAtomHandler::fromObject(wrk->getSystemState()->mainClip);
	RootMovieClip::_getLoaderInfo(ret,wrk,a,nullptr,0);
}

ASFUNCTIONBODY_ATOM(Stage,_getScaleMode)
{
	//Stage* th=asAtomHandler::as<Stage>(obj);
	switch(wrk->getSystemState()->scaleMode)
	{
		case SystemState::EXACT_FIT:
			ret = asAtomHandler::fromString(wrk->getSystemState(),"exactFit");
			return;
		case SystemState::SHOW_ALL:
			ret = asAtomHandler::fromString(wrk->getSystemState(),"showAll");
			return;
		case SystemState::NO_BORDER:
			ret = asAtomHandler::fromString(wrk->getSystemState(),"noBorder");
			return;
		case SystemState::NO_SCALE:
			ret = asAtomHandler::fromString(wrk->getSystemState(),"noScale");
			return;
	}
}

ASFUNCTIONBODY_ATOM(Stage,_setScaleMode)
{
	//Stage* th=asAtomHandler::as<Stage>(obj);
	const tiny_string& arg0=asAtomHandler::toString(args[0],wrk);
	SystemState::SCALE_MODE oldScaleMode = wrk->getSystemState()->scaleMode;
	if(arg0=="exactFit")
		wrk->getSystemState()->scaleMode=SystemState::EXACT_FIT;
	else if(arg0=="showAll")
		wrk->getSystemState()->scaleMode=SystemState::SHOW_ALL;
	else if(arg0=="noBorder")
		wrk->getSystemState()->scaleMode=SystemState::NO_BORDER;
	else if(arg0=="noScale")
		wrk->getSystemState()->scaleMode=SystemState::NO_SCALE;

	if (oldScaleMode != wrk->getSystemState()->scaleMode)
	{
		RenderThread* rt=wrk->getSystemState()->getRenderThread();
		rt->requestResize(UINT32_MAX, UINT32_MAX, true);
	}
}

ASFUNCTIONBODY_ATOM(Stage,_getStageVideos)
{
	LOG(LOG_NOT_IMPLEMENTED, "Accelerated rendering through StageVideo not implemented, SWF should fall back to Video");
	RootMovieClip* root = wrk->rootClip.getPtr();
	Template<Vector>::getInstanceS(wrk,ret,root,Class<StageVideo>::getClass(wrk->getSystemState()),NullRef);
}

ASFUNCTIONBODY_ATOM(Stage,_isFocusInaccessible)
{
	LOG(LOG_NOT_IMPLEMENTED,"Stage.isFocusInaccessible always returns false");
	ret = asAtomHandler::falseAtom;
}

_NR<InteractiveObject> Stage::getFocusTarget()
{
	Locker l(focusSpinlock);
	if (focus.isNull() || !focus->isOnStage() || !focus->isVisible())
	{
		incRef();
		return _MNR(this);
	}
	else
	{
		return focus;
	}
}

void Stage::setFocusTarget(_NR<InteractiveObject> f)
{
	Locker l(focusSpinlock);
	if (focus)
	{
		focus->lostFocus();
		getVm(getSystemState())->addEvent(_MR(focus),_MR(Class<FocusEvent>::getInstanceS(getInstanceWorker(),"focusOut")));
	}
	focus = f;
	if (focus)
	{
		focus->gotFocus();
		getVm(getSystemState())->addEvent(_MR(focus),_MR(Class<FocusEvent>::getInstanceS(getInstanceWorker(),"custcfocusIn")));
	}
}

void Stage::checkResetFocusTarget(InteractiveObject* removedtarget)
{
	Locker l(focusSpinlock);
	if (focus.getPtr() == removedtarget)
		focus=NullRef;
}

void Stage::addHiddenObject(MovieClip* o)
{
	if (!o->getInstanceWorker()->isPrimordial)
		return;
	auto it = hiddenobjects.find(o);
	if (it != hiddenobjects.end())
		return;
	o->incRef();
	o->addStoredMember();
	hiddenobjects.insert(o);
}

void Stage::removeHiddenObject(MovieClip* o)
{
	auto it = hiddenobjects.find(o);
	if (it != hiddenobjects.end())
	{
		hiddenobjects.erase(it);
		o->removeStoredMember();
	}
}

void Stage::AVM1AddDisplayObject(DisplayObject* dobj)
{
	if (!hasAVM1Clips)
		return;
	Locker l(avm1DisplayObjectMutex);
	if (dobj->avm1PrevDisplayObject || dobj->avm1NextDisplayObject || this->avm1DisplayObjectFirst == dobj)
		return;
	if (!this->avm1DisplayObjectFirst)
	{
		this->avm1DisplayObjectFirst=dobj;
		this->avm1DisplayObjectLast=dobj;
	}
	else
	{
		dobj->avm1NextDisplayObject=this->avm1DisplayObjectFirst;
		this->avm1DisplayObjectFirst->avm1PrevDisplayObject=dobj;
		this->avm1DisplayObjectFirst=dobj;
	}
}

void Stage::AVM1RemoveDisplayObject(DisplayObject* dobj)
{
	if (!hasAVM1Clips)
		return;
	Locker l(avm1DisplayObjectMutex);
	if (!dobj->avm1PrevDisplayObject && !dobj->avm1NextDisplayObject)
		return;
	if (dobj->avm1PrevDisplayObject)
		dobj->avm1PrevDisplayObject->avm1NextDisplayObject=dobj->avm1NextDisplayObject;
	else
	{
		this->avm1DisplayObjectFirst=dobj->avm1NextDisplayObject;
		this->avm1DisplayObjectFirst->avm1PrevDisplayObject=nullptr;
	}
	if (dobj->avm1NextDisplayObject)
		dobj->avm1NextDisplayObject->avm1PrevDisplayObject=dobj->avm1PrevDisplayObject;
	else
	{
		this->avm1DisplayObjectLast=dobj->avm1PrevDisplayObject;
		dobj->avm1PrevDisplayObject->avm1NextDisplayObject=nullptr;
	}
	dobj->avm1PrevDisplayObject=nullptr;
	dobj->avm1NextDisplayObject=nullptr;
}

void Stage::AVM1AddScriptToExecute(AVM1scriptToExecute& script)
{
	assert(!script.clip->needsActionScript3());
	avm1scriptstoexecute.push_back(script);
}

void Stage::advanceFrame(bool implicit)
{
	if (getSystemState()->mainClip->usesActionScript3)
	{
		DisplayObjectContainer::advanceFrame(implicit);
		unordered_set<MovieClip*> tmp = hiddenobjects; // work on copy as hidden object list may be altered during calls
		auto it = tmp.begin();
		while (it != tmp.end())
		{
			(*it)->advanceFrame(implicit);
			it++;
		}
	}
	if (hasAVM1Clips)
	{
		// scripts on AVM1 clips are executed in order of instantiation
		avm1DisplayObjectMutex.lock();
		DisplayObject* dobj = avm1DisplayObjectFirst;
		avm1DisplayObjectMutex.unlock();
		DisplayObject* prevdobj = nullptr;
		DisplayObject* nextdobj = nullptr;
		while (dobj)
		{
			dobj->incRef();
			if (!dobj->needsActionScript3())
				dobj->advanceFrame(implicit);
			avm1DisplayObjectMutex.lock();
			if (!dobj->avm1NextDisplayObject && !dobj->avm1PrevDisplayObject) // clip was removed from list during frame advance
			{
				if (prevdobj)
					nextdobj = prevdobj->avm1NextDisplayObject;
				else if (dobj != avm1DisplayObjectFirst)
					nextdobj = avm1DisplayObjectFirst;
				else
					nextdobj = nullptr;
			}
			else 
			{
				nextdobj = dobj->avm1NextDisplayObject;
				prevdobj = dobj;
			}
			avm1DisplayObjectMutex.unlock();
			dobj->decRef();
			dobj = nextdobj;
		}
		auto itscr = avm1scriptstoexecute.begin();
		while (itscr != avm1scriptstoexecute.end())
		{
			if ((*itscr).clip->isOnStage())
				(*itscr).execute();
			else if ((*itscr).event_name_id != UINT32_MAX)
				(*itscr).clip->decRef(); // was increffed in AVM1AddScriptEvents 
			itscr = avm1scriptstoexecute.erase(itscr);
		}
		
		avm1DisplayObjectMutex.lock();
		dobj = avm1DisplayObjectFirst;
		while (dobj)
		{
			dobj->AVM1AfterAdvance();
			dobj = dobj->avm1NextDisplayObject;
		}
		avm1DisplayObjectMutex.unlock();
		AVM1AfterAdvance();
	}
}
void Stage::initFrame()
{
	DisplayObjectContainer::initFrame();
	unordered_set<MovieClip*> tmp = hiddenobjects; // work on copy as hidden object list may be altered during calls
	auto it = tmp.begin();
	while (it != tmp.end())
	{
		(*it)->initFrame();
		it++;
	}
}

void Stage::executeFrameScript()
{
	DisplayObjectContainer::executeFrameScript();
	unordered_set<MovieClip*> tmp = hiddenobjects; // work on copy as hidden object list may be altered during calls
	auto it = tmp.begin();
	while (it != tmp.end())
	{
		(*it)->executeFrameScript();
		it++;
	}
}

void Stage::AVM1HandleEvent(EventDispatcher* dispatcher, Event* e)
{
	if (e->is<KeyboardEvent>())
	{
		if (e->type =="keyDown")
		{
			getSystemState()->getInputThread()->setLastKeyDown(e->as<KeyboardEvent>());
		}
		else if (e->type =="keyUp")
		{
			getSystemState()->getInputThread()->setLastKeyUp(e->as<KeyboardEvent>());
		}
		avm1listenerMutex.lock();
		vector<ASObject*> tmplisteners = avm1KeyboardListeners;
		avm1listenerMutex.unlock();
		// eventhandlers may change the listener list, so we work on a copy
		auto it = tmplisteners.rbegin();
		while (it != tmplisteners.rend())
		{
			(*it)->incRef();
			if ((*it)->AVM1HandleKeyboardEvent(e->as<KeyboardEvent>()))
			{
				(*it)->decRef();
				break;
			}
			(*it)->decRef();
			it++;
		}
	}
	else if (e->is<MouseEvent>())
	{
		avm1listenerMutex.lock();
		vector<ASObject*> tmplisteners = avm1MouseListeners;
		avm1listenerMutex.unlock();
		// eventhandlers may change the listener list, so we work on a copy
		auto it = tmplisteners.rbegin();
		while (it != tmplisteners.rend())
		{
			(*it)->incRef();
			if ((*it)->AVM1HandleMouseEvent(dispatcher, e->as<MouseEvent>()))
			{
				(*it)->decRef();
				break;
			}
			(*it)->decRef();
			it++;
		}
	}
	else
	{
		avm1listenerMutex.lock();
		vector<ASObject*> tmplisteners = avm1EventListeners;
		avm1listenerMutex.unlock();
		// eventhandlers may change the listener list, so we work on a copy
		auto it = tmplisteners.rbegin();
		while (it != tmplisteners.rend())
		{
			(*it)->incRef();
			(*it)->AVM1HandleEvent(dispatcher, e);
			(*it)->decRef();
			it++;
		}
		if (!avm1ResizeListeners.empty() && dispatcher==this && e->type=="resize")
		{
			avm1listenerMutex.lock();
			vector<ASObject*> tmplisteners = avm1ResizeListeners;
			avm1listenerMutex.unlock();
			// eventhandlers may change the listener list, so we work on a copy
			auto it = tmplisteners.rbegin();
			while (it != tmplisteners.rend())
			{
				(*it)->incRef();
				asAtom func=asAtomHandler::invalidAtom;
				multiname m(nullptr);
				m.name_type=multiname::NAME_STRING;
				m.isAttribute = false;
				m.name_s_id=getSystemState()->getUniqueStringId("onResize");
				(*it)->getVariableByMultiname(func,m,GET_VARIABLE_OPTION::NONE,getInstanceWorker());
				if (asAtomHandler::is<AVM1Function>(func))
				{
					asAtom ret=asAtomHandler::invalidAtom;
					asAtom obj = asAtomHandler::fromObject(this);
					asAtomHandler::as<AVM1Function>(func)->call(&ret,&obj,nullptr,0);
					asAtomHandler::as<AVM1Function>(func)->decRef();
				}
				(*it)->decRef();
				it++;
			}
		}
	}
}

void Stage::AVM1AddKeyboardListener(ASObject *o)
{
	Locker l(avm1listenerMutex);
	for (auto it = avm1KeyboardListeners.begin(); it != avm1KeyboardListeners.end(); it++)
	{
		if ((*it) == o)
			return;
	}
	o->incRef();
	avm1KeyboardListeners.push_back(o);
}

void Stage::AVM1RemoveKeyboardListener(ASObject *o)
{
	Locker l(avm1listenerMutex);
	for (auto it = avm1KeyboardListeners.begin(); it != avm1KeyboardListeners.end(); it++)
	{
		if ((*it) == o)
		{
			avm1KeyboardListeners.erase(it);
			o->decRef();
			break;
		}
	}
}
void Stage::AVM1AddMouseListener(ASObject *o)
{
	Locker l(avm1listenerMutex);
	for (auto it = avm1MouseListeners.begin(); it != avm1MouseListeners.end(); it++)
	{
		if ((*it) == o)
			return;
	}
	o->incRef();
	avm1MouseListeners.push_back(o);
}

void Stage::AVM1RemoveMouseListener(ASObject *o)
{
	Locker l(avm1listenerMutex);
	for (auto it = avm1MouseListeners.begin(); it != avm1MouseListeners.end(); it++)
	{
		if ((*it) == o)
		{
			avm1MouseListeners.erase(it);
			o->decRef();
			break;
		}
	}
}
void Stage::AVM1AddEventListener(ASObject *o)
{
	Locker l(avm1listenerMutex);
	for (auto it = avm1EventListeners.begin(); it != avm1EventListeners.end(); it++)
	{
		if ((*it) == o)
			return;
	}
	avm1EventListeners.push_back(o);
}
void Stage::AVM1RemoveEventListener(ASObject *o)
{
	Locker l(avm1listenerMutex);
	for (auto it = avm1EventListeners.begin(); it != avm1EventListeners.end(); it++)
	{
		if ((*it) == o)
		{
			avm1EventListeners.erase(it);
			break;
		}
	}
}

void Stage::AVM1AddResizeListener(ASObject *o)
{
	Locker l(avm1listenerMutex);
	for (auto it = avm1ResizeListeners.begin(); it != avm1ResizeListeners.end(); it++)
	{
		if ((*it) == o)
			return;
	}
	o->incRef();
	o->addStoredMember();
	avm1ResizeListeners.push_back(o);
}

bool Stage::AVM1RemoveResizeListener(ASObject *o)
{
	Locker l(avm1listenerMutex);
	for (auto it = avm1ResizeListeners.begin(); it != avm1ResizeListeners.end(); it++)
	{
		if ((*it) == o)
		{
			avm1ResizeListeners.erase(it);
			o->decRef();
			// it's not mentioned in the specs but I assume we return true if we found the listener object
			return true;
		}
	}
	return false;
}

ASFUNCTIONBODY_ATOM(Stage,_getFocus)
{
	Stage* th=asAtomHandler::as<Stage>(obj);
	_NR<InteractiveObject> focus = th->getFocusTarget();
	if (focus.isNull())
	{
		return;
	}
	else
	{
		focus->incRef();
		ret = asAtomHandler::fromObject(focus.getPtr());
	}
}

ASFUNCTIONBODY_ATOM(Stage,_setFocus)
{
	Stage* th=asAtomHandler::as<Stage>(obj);
	_NR<InteractiveObject> focus;
	ARG_CHECK(ARG_UNPACK(focus));
	th->setFocusTarget(focus);
}

ASFUNCTIONBODY_ATOM(Stage,_setTabChildren)
{
	// The specs says that Stage.tabChildren should throw
	// IllegalOperationError, but testing shows that instead of
	// throwing this simply ignores the value.
}

ASFUNCTIONBODY_ATOM(Stage,_getFrameRate)
{
	Stage* th=asAtomHandler::as<Stage>(obj);
	_NR<RootMovieClip> root = th->getRoot();
	if (root.isNull())
		asAtomHandler::setNumber(ret,wrk,wrk->getSystemState()->mainClip->getFrameRate());
	else
		asAtomHandler::setNumber(ret,wrk,root->getFrameRate());
}

ASFUNCTIONBODY_ATOM(Stage,_setFrameRate)
{
	Stage* th=asAtomHandler::as<Stage>(obj);
	number_t frameRate;
	ARG_CHECK(ARG_UNPACK(frameRate));
	_NR<RootMovieClip> root = th->getRoot();
	if (!root.isNull())
		root->setFrameRate(frameRate);
}

ASFUNCTIONBODY_ATOM(Stage,_getAllowFullScreen)
{
	asAtomHandler::setBool(ret,wrk->getSystemState()->allowFullscreen);
}

ASFUNCTIONBODY_ATOM(Stage,_getAllowFullScreenInteractive)
{
	asAtomHandler::setBool(ret,wrk->getSystemState()->allowFullscreenInteractive);
}

ASFUNCTIONBODY_ATOM(Stage,_getColorCorrectionSupport)
{
	asAtomHandler::setBool(ret,false); // until color correction is implemented
}

ASFUNCTIONBODY_ATOM(Stage,_getWmodeGPU)
{
	asAtomHandler::setBool(ret,false);
}
ASFUNCTIONBODY_ATOM(Stage,_invalidate)
{
	Stage* th=asAtomHandler::as<Stage>(obj);
	th->forceInvalidation();
}
void Stage::forceInvalidation()
{
	RELEASE_WRITE(this->invalidated,true);
	this->incRef();
	_R<FlushInvalidationQueueEvent> event=_MR(new (getSystemState()->unaccountedMemory) FlushInvalidationQueueEvent());
	getVm(getSystemState())->addEvent(_MR(this),event);
}
ASFUNCTIONBODY_ATOM(Stage,_getColor)
{
	Stage* th=asAtomHandler::as<Stage>(obj);
	RGB rgb;
	_NR<RootMovieClip> root = th->getRoot();
	if (!root.isNull())
		rgb = root->getBackground();
	asAtomHandler::setUInt(ret,wrk,rgb.toUInt());
}

ASFUNCTIONBODY_ATOM(Stage,_setColor)
{
	Stage* th=asAtomHandler::as<Stage>(obj);
	uint32_t color;
	ARG_CHECK(ARG_UNPACK(color));
	RGB rgb(color);
	_NR<RootMovieClip> root = th->getRoot();
	if (!root.isNull())
		root->setBackground(rgb);
}


void StageScaleMode::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("EXACT_FIT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"exactFit"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NO_BORDER",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"noBorder"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NO_SCALE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"noScale"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("SHOW_ALL",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"showAll"),CONSTANT_TRAIT);
}

void StageAlign::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("BOTTOM",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"B"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("BOTTOM_LEFT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"BL"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("BOTTOM_RIGHT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"BR"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("LEFT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"L"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("RIGHT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"R"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("TOP",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"T"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("TOP_LEFT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"TL"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("TOP_RIGHT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"TR"),CONSTANT_TRAIT);
}

void StageQuality::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("BEST",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"best"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("HIGH",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"high"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("LOW",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"low"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("MEDIUM",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"medium"),CONSTANT_TRAIT);

	c->setVariableAtomByQName("HIGH_16X16",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"16x16"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("HIGH_16X16_LINEAR",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"16x16linear"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("HIGH_8X8",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"8x8"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("HIGH_8X8_LINEAR",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"8x8linear"),CONSTANT_TRAIT);
}

void StageDisplayState::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("FULL_SCREEN",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"fullScreen"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("FULL_SCREEN_INTERACTIVE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"fullScreenInteractive"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NORMAL",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"normal"),CONSTANT_TRAIT);
}

Bitmap::Bitmap(ASWorker* wrk, Class_base* c, _NR<LoaderInfo> li, std::istream *s, FILE_TYPE type):
	DisplayObject(wrk,c),cachedBitmapOwner(nullptr),smoothing(false)
{
	subtype=SUBTYPE_BITMAP;
	if(li)
	{
		loaderInfo = li;
		this->incRef();
		loaderInfo->setWaitedObject(_MR(this));
	}

	bitmapData = _MR(Class<BitmapData>::getInstanceS(wrk));
	bitmapData->addUser(this);
	if(!s)
		return;

	if(type==FT_UNKNOWN)
	{
		// Try to detect the format from the stream
		UI8 Signature[4];
		(*s) >> Signature[0] >> Signature[1] >> Signature[2] >> Signature[3];
		type=ParseThread::recognizeFile(Signature[0], Signature[1],
						Signature[2], Signature[3]);
		s->putback(Signature[3]).putback(Signature[2]).
		   putback(Signature[1]).putback(Signature[0]);
	}

	switch(type)
	{
		case FT_JPEG:
			bitmapData->getBitmapContainer()->fromJPEG(*s);
			break;
		case FT_PNG:
			bitmapData->getBitmapContainer()->fromPNG(*s);
			break;
		case FT_GIF:
			LOG(LOG_NOT_IMPLEMENTED, "GIFs are not yet supported");
			break;
		default:
			LOG(LOG_ERROR,"Unsupported image type");
			break;
	}
	Bitmap::updatedData();
	afterConstruction();
}

Bitmap::Bitmap(ASWorker* wrk, Class_base* c, _R<BitmapData> data, bool startupload, DisplayObject* owner) : DisplayObject(wrk,c),cachedBitmapOwner(owner),smoothing(false)
{
	subtype=SUBTYPE_BITMAP;
	bitmapData = data;
	bitmapData->addUser(this,startupload);
	Bitmap::updatedData();
}

Bitmap::~Bitmap()
{
}

bool Bitmap::destruct()
{
	if(!bitmapData.isNull())
		bitmapData->removeUser(this);
	bitmapData.reset();
	cachedBitmapOwner=nullptr;
	smoothing = false;
	return DisplayObject::destruct();
}

void Bitmap::finalize()
{
	bitmapData.reset();
	cachedBitmapOwner=nullptr;
	DisplayObject::finalize();
}

void Bitmap::prepareShutdown()
{
	if (preparedforshutdown)
		return;
	ASObject::prepareShutdown();
	if (bitmapData)
		bitmapData->prepareShutdown();
}

void Bitmap::setOnStage(bool staged, bool force, bool inskipping)
{
	DisplayObject::setOnStage(staged,force,inskipping);
	if (bitmapData && isOnStage())
		bitmapData->checkForUpload();
}

void Bitmap::sinit(Class_base* c)
{
	CLASS_SETUP(c, DisplayObject, _constructor, CLASS_SEALED);
	c->isReusable = true;
	REGISTER_GETTER_SETTER_RESULTTYPE(c,bitmapData,BitmapData);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,smoothing,Boolean);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,pixelSnapping,ASString);
}

ASFUNCTIONBODY_ATOM(Bitmap,_constructor)
{
	tiny_string _pixelSnapping;
	_NR<BitmapData> _bitmapData;
	Bitmap* th = asAtomHandler::as<Bitmap>(obj);
	ARG_CHECK(ARG_UNPACK(_bitmapData, NullRef)(_pixelSnapping, "auto")(th->smoothing, false));

	DisplayObject::_constructor(ret,wrk,obj,nullptr,0);

	if(_pixelSnapping!="auto")
		LOG(LOG_NOT_IMPLEMENTED, "Bitmap constructor doesn't support pixelSnapping:"<<_pixelSnapping);
	th->pixelSnapping = _pixelSnapping;

	if(!_bitmapData.isNull())
	{
		th->bitmapData=_bitmapData;
		th->bitmapData->addUser(th);
		th->updatedData();
	}
}

void Bitmap::onBitmapData(_NR<BitmapData> old)
{
	if(!old.isNull())
		old->removeUser(this);
	if(!bitmapData.isNull())
		bitmapData->addUser(this);
	geometryChanged();
	Bitmap::updatedData();
}

void Bitmap::onSmoothingChanged(bool /*old*/)
{
	updatedData();
}

void Bitmap::onPixelSnappingChanged(tiny_string snapping)
{
	if(snapping!="auto")
		LOG(LOG_NOT_IMPLEMENTED, "Bitmap doesn't support pixelSnapping:"<<snapping);
	pixelSnapping = snapping;
}

bool Bitmap::renderImpl(RenderContext& ctxt)
{
	if (computeCacheAsBitmap() && ctxt.contextType == RenderContext::GL)
	{
		_NR<DisplayObject> d=getCachedBitmap(); // this ensures bitmap is not destructed during rendering
		if (d)
			d->Render(ctxt);
		return false;
	}
	return defaultRender(ctxt);
}

ASFUNCTIONBODY_GETTER_SETTER_CB(Bitmap,bitmapData,onBitmapData)
ASFUNCTIONBODY_GETTER_SETTER_CB(Bitmap,smoothing,onSmoothingChanged)
ASFUNCTIONBODY_GETTER_SETTER_CB(Bitmap,pixelSnapping,onPixelSnappingChanged)

void Bitmap::updatedData()
{
	if(bitmapData.isNull() || bitmapData->getBitmapContainer().isNull())
	{
		if (cachedSurface.isChunkOwner && cachedSurface.tex)
			cachedSurface.tex->makeEmpty();
		else
			cachedSurface.tex=nullptr;
		return;
	}
	if (this->isOnStage())
		bitmapData->checkForUpload();
	cachedSurface.tex = &bitmapData->getBitmapContainer()->bitmaptexture;
	cachedSurface.isChunkOwner=false;
	hasChanged=true;
	DisplayObject* d = this->cachedBitmapOwner;
	if (d)
	{
		d->hasChanged=true;
		d->setNeedsTextureRecalculation();
		if(d->isOnStage())
		{
			bitmapData->checkForUpload();
			cachedSurface.isValid=true;
			d->requestInvalidation(getSystemState());
		}
	}
	if(onStage)
	{
		cachedSurface.isValid=true;
		requestInvalidation(getSystemState());
	}
}
bool Bitmap::boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax)
{
	if (bitmapData.isNull())
		return false;
	xmin = 0;
	ymin = 0;
	xmax = bitmapData->getWidth();
	ymax = bitmapData->getHeight();
	return true;
}

_NR<DisplayObject> Bitmap::hitTestImpl(number_t x, number_t y, DisplayObject::HIT_TYPE type,bool interactiveObjectsOnly)
{
	//Simple check inside the area, opacity data should not be considered
	//NOTE: on the X axis the 0th line must be ignored, while the one past the width is valid
	//NOTE: on the Y asix the 0th line is valid, while the one past the width is not
	//NOTE: This is tested behaviour!
	if(!bitmapData.isNull() && x > 0 && x <= bitmapData->getWidth() && y >=0 && y < bitmapData->getHeight())
	{
		this->incRef();
		return _MR(this);
	}
	return NullRef;
}

IntSize Bitmap::getBitmapSize() const
{
	if(bitmapData.isNull())
		return IntSize(0, 0);
	else
		return IntSize(bitmapData->getWidth(), bitmapData->getHeight());
}

void Bitmap::requestInvalidation(InvalidateQueue *q, bool forceTextureRefresh)
{
	if(skipRender())
		return;
	if (requestInvalidationForCacheAsBitmap(q))
		return;
	incRef();
	// texture recalculation is never needed for bitmaps
	resetNeedsTextureRecalculation();
	q->addToInvalidateQueue(_MR(this));
}

IDrawable *Bitmap::invalidate(DisplayObject *target, const MATRIX &initialMatrix, bool smoothing, InvalidateQueue* q, _NR<DisplayObject>* cachedBitmap)
{
	if (cachedBitmap && computeCacheAsBitmap() && (!q || !q->getCacheAsBitmapObject() || q->getCacheAsBitmapObject().getPtr()!=this))
	{
		return getCachedBitmapDrawable(target, initialMatrix, cachedBitmap);
	}
	return invalidateFromSource(target, initialMatrix, this->smoothing, this, MATRIX(),nullptr);
}

IDrawable *Bitmap::invalidateFromSource(DisplayObject *target, const MATRIX &initialMatrix, bool smoothing, DisplayObject* matrixsource, const MATRIX& sourceMatrix,DisplayObject* originalsource)
{
	number_t rx,ry;
	number_t rwidth,rheight;
	number_t bxmin,bxmax,bymin,bymax;
	if(!boundsRectWithoutChildren(bxmin,bxmax,bymin,bymax))
	{
		//No contents, nothing to do
		return nullptr;
	}
	//Compute the matrix and the masks that are relevant
	MATRIX totalMatrix;
	std::vector<IDrawable::MaskData> masks;

	bool isMask;
	if (target)
	{
		if (matrixsource)
			matrixsource->computeMasksAndMatrix(target,masks,totalMatrix,false,isMask,mask);
	}
	float redMultiplier=1.0;
	float greenMultiplier=1.0;
	float blueMultiplier=1.0;
	float alphaMultiplier=1.0;
	float redOffset=0.0;
	float greenOffset=0.0;
	float blueOffset=0.0;
	float alphaOffset=0.0;
	MATRIX totalMatrix2;
	std::vector<IDrawable::MaskData> masks2;
	if (target)
	{
		if (matrixsource)
			matrixsource->computeMasksAndMatrix(target,masks2,totalMatrix2,true,isMask,mask);
		totalMatrix2=initialMatrix.multiplyMatrix(totalMatrix2);
	}
	totalMatrix2 = totalMatrix2.multiplyMatrix(sourceMatrix);
	computeBoundsForTransformedRect(bxmin,bxmax,bymin,bymax,rx,ry,rwidth,rheight,totalMatrix2);
	ColorTransform* ct = colorTransform.getPtr();
	DisplayObjectContainer* p = matrixsource ? matrixsource->getParent() :nullptr;
	while (!ct && p)
	{
		ct = p->colorTransform.getPtr();
		p = p->getParent();
	}
	if (rwidth==0 || rheight==0)
		return nullptr;
	if (ct)
	{
		redMultiplier=ct->redMultiplier;
		greenMultiplier=ct->greenMultiplier;
		blueMultiplier=ct->blueMultiplier;
		alphaMultiplier=ct->alphaMultiplier;
		redOffset=ct->redOffset;
		greenOffset=ct->greenOffset;
		blueOffset=ct->blueOffset;
		alphaOffset=ct->alphaOffset;
	}
	cachedSurface.isValid=true;
	if (originalsource)
	{
		if (!isMask)
			isMask = originalsource->ClipDepth || originalsource->ismask;
		if (!mask)
			mask = originalsource->mask;
	}
	return new BitmapRenderer(this->bitmapData->getBitmapContainer()
				, bxmin, bymin, this->bitmapData->getWidth(), this->bitmapData->getHeight()
				, rx, ry, round(rwidth), round(rheight), 0
				, 1, 1
				, isMask, mask
				, originalsource ? originalsource->getConcatenatedAlpha() : getConcatenatedAlpha(), masks
				, redMultiplier,greenMultiplier,blueMultiplier,alphaMultiplier
				, redOffset,greenOffset,blueOffset,alphaOffset,smoothing ? SMOOTH_MODE::SMOOTH_ANTIALIAS:SMOOTH_MODE::SMOOTH_NONE,totalMatrix2);
}

void SimpleButton::sinit(Class_base* c)
{
	CLASS_SETUP(c, InteractiveObject, _constructor, CLASS_SEALED);
	c->isReusable=true;
	c->setDeclaredMethodByQName("upState","",Class<IFunction>::getFunction(c->getSystemState(),_getUpState),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("upState","",Class<IFunction>::getFunction(c->getSystemState(),_setUpState),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("downState","",Class<IFunction>::getFunction(c->getSystemState(),_getDownState),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("downState","",Class<IFunction>::getFunction(c->getSystemState(),_setDownState),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("overState","",Class<IFunction>::getFunction(c->getSystemState(),_getOverState),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("overState","",Class<IFunction>::getFunction(c->getSystemState(),_setOverState),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("hitTestState","",Class<IFunction>::getFunction(c->getSystemState(),_getHitTestState),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("hitTestState","",Class<IFunction>::getFunction(c->getSystemState(),_setHitTestState),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("enabled","",Class<IFunction>::getFunction(c->getSystemState(),_getEnabled),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("enabled","",Class<IFunction>::getFunction(c->getSystemState(),_setEnabled),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("useHandCursor","",Class<IFunction>::getFunction(c->getSystemState(),_getUseHandCursor),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("useHandCursor","",Class<IFunction>::getFunction(c->getSystemState(),_setUseHandCursor),SETTER_METHOD,true);
}

void SimpleButton::afterLegacyInsert()
{
	getSystemState()->stage->AVM1AddKeyboardListener(this);
	getSystemState()->stage->AVM1AddMouseListener(this);
	DisplayObjectContainer::afterLegacyInsert();
}

void SimpleButton::afterLegacyDelete(DisplayObjectContainer *parent,bool inskipping)
{
	getSystemState()->stage->AVM1RemoveKeyboardListener(this);
	getSystemState()->stage->AVM1RemoveMouseListener(this);
}

bool SimpleButton::AVM1HandleMouseEvent(EventDispatcher* dispatcher, MouseEvent *e)
{
	if (!this->isOnStage() || !this->enabled || !this->isVisible() || this->loadedFrom->needsActionScript3())
		return false;
	if (!dispatcher->is<DisplayObject>())
		return false;
	DisplayObject* dispobj=nullptr;
	if(e->type == "mouseOut")
	{
		if (dispatcher!= this)
			return false;
	}
	else
	{
		if (dispatcher == this)
			dispobj=this;
		else
		{
			number_t x,y;
			dispatcher->as<DisplayObject>()->localToGlobal(e->localX,e->localY,x,y);
			number_t x1,y1;
			this->globalToLocal(x,y,x1,y1);
			_NR<DisplayObject> d = hitTest(x1,y1, DisplayObject::MOUSE_CLICK,true);
			dispobj=d.getPtr();
		}
		if (dispobj!= this)
			return false;
	}
	BUTTONSTATE oldstate = currentState;
	if(e->type == "mouseDown")
	{
		currentState = DOWN;
		reflectState(oldstate);
	}
	else if(e->type == "mouseUp")
	{
		currentState = UP;
		reflectState(oldstate);
	}
	else if(e->type == "mouseOver")
	{
		currentState = OVER;
		reflectState(oldstate);
	}
	else if(e->type == "mouseOut")
	{
		currentState = STATE_OUT;
		reflectState(oldstate);
	}
	bool handled = false;
	if (buttontag)
	{
		for (auto it = buttontag->condactions.begin(); it != buttontag->condactions.end(); it++)
		{
			if (  (it->CondIdleToOverDown && currentState==DOWN)
				||(it->CondOutDownToIdle && oldstate==DOWN && currentState==STATE_OUT)
				||(it->CondOutDownToOverDown && oldstate==DOWN && currentState==OVER)
				||(it->CondOverDownToOutDown && (oldstate==DOWN || oldstate==OVER) && currentState==STATE_OUT)
				||(it->CondOverDownToOverUp && (oldstate==DOWN || oldstate==OVER) && currentState==UP)
				||(it->CondOverUpToOverDown && (oldstate==UP || oldstate==OVER) && currentState==DOWN)
				||(it->CondOverUpToIdle && (oldstate==UP || oldstate==OVER) && currentState==STATE_OUT)
				||(it->CondIdleToOverUp && oldstate==STATE_OUT && currentState==OVER)
				||(it->CondOverDownToIdle && oldstate==DOWN && currentState==OVER)
				)
			{
				DisplayObjectContainer* c = getParent();
				while (c && !c->is<MovieClip>())
					c = c->getParent();
				if (c)
				{
					std::map<uint32_t,asAtom> m;
					ACTIONRECORD::executeActions(c->as<MovieClip>(),c->as<MovieClip>()->getCurrentFrame()->getAVM1Context(),it->actions,it->startactionpos,m);
					handled = true;
				}
				
			}
		}
	}
	handled |= AVM1HandleMouseEventStandard(dispobj,e);
	return handled;
}

void SimpleButton::handleMouseCursor(bool rollover)
{
	if (rollover)
	{
		hasMouse=true;
		getSystemState()->setMouseHandCursor(this->useHandCursor);
	}
	else
	{
		getSystemState()->setMouseHandCursor(false);
		hasMouse=false;
	}
}

bool SimpleButton::AVM1HandleKeyboardEvent(KeyboardEvent *e)
{
	bool handled=false;
	for (auto it = this->buttontag->condactions.begin(); it != this->buttontag->condactions.end(); it++)
	{
		bool execute=false;
		uint32_t code = e->getSDLScanCode();
		if (e->getModifiers() & KMOD_SHIFT)
		{
			switch (it->CondKeyPress)
			{
				case 33:// !
					execute = code==SDL_SCANCODE_1;break;
				case 34:// "
					execute = code==SDL_SCANCODE_APOSTROPHE;break;
				case 35:// #
					execute = code==SDL_SCANCODE_3;break;
				case 36:// $
					execute = code==SDL_SCANCODE_4;break;
				case 37:// %
					execute = code==SDL_SCANCODE_5;break;
				case 38:// &
					execute = code==SDL_SCANCODE_7;break;
				case 40:// (
					execute = code==SDL_SCANCODE_9;break;
				case 41:// )
					execute = code==SDL_SCANCODE_0;break;
				case 42:// *
					execute = code==SDL_SCANCODE_8;break;
				case 43:// +
					execute = code==SDL_SCANCODE_EQUALS;break;
				case 58:// :
					execute = code==SDL_SCANCODE_SEMICOLON;break;
				case 60:// <
					execute = code==SDL_SCANCODE_COMMA;break;
				case 62:// >
					execute = code==SDL_SCANCODE_PERIOD;break;
				case 63:// ?
					execute = code==SDL_SCANCODE_SLASH;break;
				case 64:// @
					execute = code==SDL_SCANCODE_2;break;
				case 94:// ^
					execute = code==SDL_SCANCODE_6;break;
				case 95:// _
					execute = code==SDL_SCANCODE_MINUS;break;
				case 123:// {
					execute = code==SDL_SCANCODE_LEFTBRACKET;break;
				case 124:// |
					execute = code==SDL_SCANCODE_BACKSLASH;break;
				case 125:// }
					execute = code==SDL_SCANCODE_RIGHTBRACKET;break;
				case 126:// ~
					execute = code==SDL_SCANCODE_GRAVE;break;
				default:// A-Z
					execute = it->CondKeyPress>=65
							&& it->CondKeyPress<=90
							&& code-SDL_SCANCODE_A==it->CondKeyPress-65;
					break;
			}
		}
		else
		{
			switch (it->CondKeyPress)
			{
				case 1:
					execute = code==SDL_SCANCODE_LEFT;break;
				case 2:
					execute = code==SDL_SCANCODE_RIGHT;break;
				case 3:
					execute = code==SDL_SCANCODE_HOME;break;
				case 4:
					execute = code==SDL_SCANCODE_END;break;
				case 5:
					execute = code==SDL_SCANCODE_INSERT;break;
				case 6:
					execute = code==SDL_SCANCODE_DELETE;break;
				case 8:
					execute = code==SDL_SCANCODE_BACKSPACE;break;
				case 13:
					execute = code==SDL_SCANCODE_RETURN;break;
				case 14:
					execute = code==SDL_SCANCODE_UP;break;
				case 15:
					execute = code==SDL_SCANCODE_DOWN;break;
				case 16:
					execute = code==SDL_SCANCODE_PAGEUP;break;
				case 17:
					execute = code==SDL_SCANCODE_PAGEDOWN;break;
				case 18:
					execute = code==SDL_SCANCODE_TAB;break;
				case 19:
					execute = code==SDL_SCANCODE_ESCAPE;break;
				case 32:
					execute = code==SDL_SCANCODE_SPACE;break;
				case 39:// '
					execute = code==SDL_SCANCODE_APOSTROPHE;break;
				case 44:// ,
					execute = code==SDL_SCANCODE_COMMA;break;
				case 45:// -
					execute = code==SDL_SCANCODE_MINUS;break;
				case 46:// .
					execute = code==SDL_SCANCODE_PERIOD;break;
				case 47:// /
					execute = code==SDL_SCANCODE_SLASH;break;
				case 48:// 0
					execute = code==SDL_SCANCODE_0;break;
				case 49:// 1
					execute = code==SDL_SCANCODE_1;break;
				case 50:// 2
					execute = code==SDL_SCANCODE_2;break;
				case 51:// 3
					execute = code==SDL_SCANCODE_3;break;
				case 52:// 4
					execute = code==SDL_SCANCODE_4;break;
				case 53:// 5
					execute = code==SDL_SCANCODE_5;break;
				case 54:// 6
					execute = code==SDL_SCANCODE_6;break;
				case 55:// 7
					execute = code==SDL_SCANCODE_7;break;
				case 56:// 8
					execute = code==SDL_SCANCODE_8;break;
				case 57:// 9
					execute = code==SDL_SCANCODE_9;break;
				case 59:// ;
					execute = code==SDL_SCANCODE_SEMICOLON;break;
				case 61:// =
					execute = code==SDL_SCANCODE_EQUALS;break;
				case 91:// [
					execute = code==SDL_SCANCODE_LEFTBRACKET;break;
				case 92:// 
					execute = code==SDL_SCANCODE_BACKSLASH;break;
				case 93:// ]
					execute = code==SDL_SCANCODE_RIGHTBRACKET;break;
				case 96:// `
					execute = code==SDL_SCANCODE_GRAVE;break;
				default:// a-z
					execute = it->CondKeyPress>=97
							&& it->CondKeyPress<=122
							&& code-SDL_SCANCODE_A==it->CondKeyPress-97;
					break;
			}
		}
		if (execute)
		{
			DisplayObjectContainer* c = getParent();
			while (c && !c->is<MovieClip>())
				c = c->getParent();
			std::map<uint32_t,asAtom> m;
			ACTIONRECORD::executeActions(c->as<MovieClip>(),c->as<MovieClip>()->getCurrentFrame()->getAVM1Context(),it->actions,it->startactionpos,m);
			handled=true;
		}
	}
	if (!handled)
		DisplayObjectContainer::AVM1HandleKeyboardEvent(e);
	return handled;
}


_NR<DisplayObject> SimpleButton::hitTestImpl(number_t x, number_t y, DisplayObject::HIT_TYPE type,bool interactiveObjectsOnly)
{
	_NR<DisplayObject> ret = NullRef;
	if(hitTestState)
	{
		if(!hitTestState->getMatrix().isInvertible())
			return NullRef;

		number_t localX, localY;
		hitTestState->getMatrix().getInverted().multiply2D(x,y,localX,localY);
		ret = hitTestState->hitTest(localX,localY, type,false);
	}
	/* mouseDown events, for example, are never dispatched to the hitTestState,
	 * but directly to this button (and with event.target = this). This has been
	 * tested with the official flash player. It cannot work otherwise, as
	 * hitTestState->parent == nullptr. (This has also been verified)
	 */
	if(ret)
	{
		if(interactiveObjectsOnly && !isHittable(type))
			return NullRef;

		if (ret.getPtr() != this)
		{
			this->incRef();
			ret = _MR(this);
		}
	}
	return ret;
}

void SimpleButton::defaultEventBehavior(_R<Event> e)
{
	BUTTONSTATE oldstate = currentState;
	if(e->type == "mouseDown")
	{
		currentState = DOWN;
		reflectState(oldstate);
	}
	else if(e->type == "mouseUp")
	{
		currentState = UP;
		reflectState(oldstate);
	}
	else if(e->type == "mouseOver")
	{
		currentState = OVER;
		reflectState(oldstate);
	}
	else if(e->type == "mouseOut")
	{
		currentState = STATE_OUT;
		reflectState(oldstate);
	}
	else
		DisplayObjectContainer::defaultEventBehavior(e);
}

bool SimpleButton::boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax)
{
	bool ret = false;
	number_t txmin,txmax,tymin,tymax;
	if (!upState.isNull() && upState->getBounds(txmin,txmax,tymin,tymax,upState->getMatrix()))
	{
		if(ret==true)
		{
			xmin = min(xmin,txmin);
			xmax = max(xmax,txmax);
			ymin = min(ymin,tymin);
			ymax = max(ymax,tymax);
		}
		else
		{
			xmin=txmin;
			xmax=txmax;
			ymin=tymin;
			ymax=tymax;
			ret=true;
		}
	}
	if (!overState.isNull() && overState->getBounds(txmin,txmax,tymin,tymax,overState->getMatrix()))
	{
		if(ret==true)
		{
			xmin = min(xmin,txmin);
			xmax = max(xmax,txmax);
			ymin = min(ymin,tymin);
			ymax = max(ymax,tymax);
		}
		else
		{
			xmin=txmin;
			xmax=txmax;
			ymin=tymin;
			ymax=tymax;
			ret=true;
		}
	}
	if (!downState.isNull() && downState->getBounds(txmin,txmax,tymin,tymax,downState->getMatrix()))
	{
		if(ret==true)
		{
			xmin = min(xmin,txmin);
			xmax = max(xmax,txmax);
			ymin = min(ymin,tymin);
			ymax = max(ymax,tymax);
		}
		else
		{
			xmin=txmin;
			xmax=txmax;
			ymin=tymin;
			ymax=tymax;
			ret=true;
		}
	}
	return ret;
}

SimpleButton::SimpleButton(ASWorker* wrk, Class_base* c, DisplayObject *dS, DisplayObject *hTS,
				DisplayObject *oS, DisplayObject *uS, DefineButtonTag *tag)
	: DisplayObjectContainer(wrk,c), downState(dS), hitTestState(hTS), overState(oS), upState(uS),
	  buttontag(tag),currentState(STATE_OUT),enabled(true),useHandCursor(true),hasMouse(false)
{
	subtype = SUBTYPE_SIMPLEBUTTON;
	/* When called from DefineButton2Tag::instance, they are not constructed yet
	 * TODO: construct them here for once, or each time they become visible?
	 */
	if(dS)
	{
		dS->advanceFrame(false);
		dS->initFrame();
	}
	if(hTS)
	{
		hTS->advanceFrame(false);
		hTS->initFrame();
	}
	if(oS)
	{
		oS->advanceFrame(false);
		oS->initFrame();
	}
	if(uS)
	{
		uS->advanceFrame(false);
		uS->initFrame();
	}
	if (!needsActionScript3())
	{
		asAtom obj = asAtomHandler::fromObjectNoPrimitive(this);
		getClass()->handleConstruction(obj,nullptr,0,true);
	}
	if (tag && tag->sounds)
	{
		if (tag->sounds->SoundID0_OverUpToIdle)
		{
			DefineSoundTag* sound = dynamic_cast<DefineSoundTag*>(tag->loadedFrom->dictionaryLookup(tag->sounds->SoundID0_OverUpToIdle));
			if (sound)
				soundchannel_OverUpToIdle = _MR(sound->createSoundChannel(&tag->sounds->SoundInfo0_OverUpToIdle));
			else
				LOG(LOG_ERROR,"ButtonSound not found for OverUpToIdle:"<<tag->sounds->SoundID0_OverUpToIdle << " on button "<<tag->getId());
		}
		if (tag->sounds->SoundID1_IdleToOverUp)
		{
			DefineSoundTag* sound = dynamic_cast<DefineSoundTag*>(tag->loadedFrom->dictionaryLookup(tag->sounds->SoundID1_IdleToOverUp));
			if (sound)
				soundchannel_IdleToOverUp = _MR(sound->createSoundChannel(&tag->sounds->SoundInfo1_IdleToOverUp));
			else
				LOG(LOG_ERROR,"ButtonSound not found for IdleToOverUp:"<<tag->sounds->SoundID1_IdleToOverUp << " on button "<<tag->getId());
		}
		if (tag->sounds->SoundID2_OverUpToOverDown)
		{
			DefineSoundTag* sound = dynamic_cast<DefineSoundTag*>(tag->loadedFrom->dictionaryLookup(tag->sounds->SoundID2_OverUpToOverDown));
			if (sound)
				soundchannel_OverUpToOverDown = _MR(sound->createSoundChannel(&tag->sounds->SoundInfo2_OverUpToOverDown));
			else
				LOG(LOG_ERROR,"ButtonSound not found for OverUpToOverDown:"<<tag->sounds->SoundID2_OverUpToOverDown << " on button "<<tag->getId());
		}
		if (tag->sounds->SoundID3_OverDownToOverUp)
		{
			DefineSoundTag* sound = dynamic_cast<DefineSoundTag*>(tag->loadedFrom->dictionaryLookup(tag->sounds->SoundID3_OverDownToOverUp));
			if (sound)
				soundchannel_OverDownToOverUp = _MR(sound->createSoundChannel(&tag->sounds->SoundInfo3_OverDownToOverUp));
			else
				LOG(LOG_ERROR,"ButtonSound not found for OverUpToOverDown:"<<tag->sounds->SoundID3_OverDownToOverUp << " on button "<<tag->getId());
		}
	}
	tabEnabled = true;
}

void SimpleButton::constructionComplete()
{
	reflectState(STATE_OUT);
	DisplayObjectContainer::constructionComplete();
}
void SimpleButton::finalize()
{
	DisplayObjectContainer::finalize();
	downState.reset();
	hitTestState.reset();
	overState.reset();
	upState.reset();
	enabled=true;
	useHandCursor=true;
	hasMouse=false;
	buttontag=nullptr;
}

bool SimpleButton::destruct()
{
	downState.reset();
	hitTestState.reset();
	overState.reset();
	upState.reset();
	enabled=true;
	useHandCursor=true;
	hasMouse=false;
	buttontag=nullptr;
	return DisplayObjectContainer::destruct();
}

void SimpleButton::prepareShutdown()
{
	if (preparedforshutdown)
		return;
	DisplayObjectContainer::prepareShutdown();
	if(downState)
		downState->prepareShutdown();
	if(hitTestState)
		hitTestState->prepareShutdown();
	if(overState)
		overState->prepareShutdown();
	if(upState)
		upState->prepareShutdown();
	if(soundchannel_OverUpToIdle)
		soundchannel_OverUpToIdle->prepareShutdown();
	if(soundchannel_IdleToOverUp)
		soundchannel_IdleToOverUp->prepareShutdown();
	if(soundchannel_OverUpToOverDown)
		soundchannel_OverUpToOverDown->prepareShutdown();
	if(soundchannel_OverDownToOverUp)
		soundchannel_OverDownToOverUp->prepareShutdown();
}
IDrawable *SimpleButton::invalidate(DisplayObject *target, const MATRIX &initialMatrix, bool smoothing, InvalidateQueue* q, _NR<DisplayObject>* cachedBitmap)
{
	if (computeCacheAsBitmap() && (!q || !q->getCacheAsBitmapObject() || q->getCacheAsBitmapObject().getPtr()!=this))
		return getCachedBitmapDrawable(target, initialMatrix, cachedBitmap);
	return nullptr;
}
void SimpleButton::requestInvalidation(InvalidateQueue* q, bool forceTextureRefresh)
{
	if (requestInvalidationForCacheAsBitmap(q))
	{
		if (!q->isSoftwareQueue)
			return;
	}
	else if (computeCacheAsBitmap())
	{
		incRef();
		q->addToInvalidateQueue(_MR(this));
	}
	if (!upState.isNull())
	{
		upState->hasChanged = true;
		upState->requestInvalidation(q,forceTextureRefresh);
	}
	if (!overState.isNull())
	{
		overState->hasChanged = true;
		overState->requestInvalidation(q,forceTextureRefresh);
	}
	if (!downState.isNull())
	{
		downState->hasChanged = true;
		downState->requestInvalidation(q,forceTextureRefresh);
	}
	if (!hitTestState.isNull())
	{
		hitTestState->hasChanged = true;
		hitTestState->requestInvalidation(q,forceTextureRefresh);
	}
	DisplayObjectContainer::requestInvalidation(q,forceTextureRefresh);
}

uint32_t SimpleButton::getTagID() const
{
	return buttontag ? uint32_t(buttontag->getId()) : 0;
}

ASFUNCTIONBODY_ATOM(SimpleButton,_constructor)
{
	/* This _must_ not call the DisplayObjectContainer
	 * see note at the class declaration.
	 */
	InteractiveObject::_constructor(ret,wrk,obj,nullptr,0);
	SimpleButton* th=asAtomHandler::as<SimpleButton>(obj);
	_NR<DisplayObject> upState;
	_NR<DisplayObject> overState;
	_NR<DisplayObject> downState;
	_NR<DisplayObject> hitTestState;
	ARG_CHECK(ARG_UNPACK(upState, NullRef)(overState, NullRef)(downState, NullRef)(hitTestState, NullRef));

	if (!upState.isNull())
		th->upState = upState;
	if (!overState.isNull())
		th->overState = overState;
	if (!downState.isNull())
		th->downState = downState;
	if (!hitTestState.isNull())
		th->hitTestState = hitTestState;
}

void SimpleButton::reflectState(BUTTONSTATE oldstate)
{
	assert(dynamicDisplayList.empty() || dynamicDisplayList.size() == 1);
	if(!dynamicDisplayList.empty())
	{
		_removeChild(dynamicDisplayList.front(),true);
	}

	if((currentState == UP || currentState == STATE_OUT) && !upState.isNull())
	{
		upState->incRef();
		_addChildAt(upState.getPtr(),0);
	}
	else if(currentState == DOWN && !downState.isNull())
	{
		downState->incRef();
		_addChildAt(downState.getPtr(),0);
	}
	else if(currentState == OVER && !overState.isNull())
	{
		overState->incRef();
		_addChildAt(overState.getPtr(),0);
	}
	if ((oldstate == OVER || oldstate == UP) && currentState == STATE_OUT && soundchannel_OverUpToIdle)
		soundchannel_OverUpToIdle->play();
	if (oldstate == STATE_OUT && (currentState == OVER || currentState == UP) && soundchannel_IdleToOverUp)
		soundchannel_IdleToOverUp->play();
	if ((oldstate == OVER || oldstate == UP) && currentState == DOWN && soundchannel_OverUpToOverDown)
		soundchannel_OverUpToOverDown->play();
	if (oldstate == DOWN && currentState == UP && soundchannel_OverDownToOverUp)
		soundchannel_OverDownToOverUp->play();
}

ASFUNCTIONBODY_ATOM(SimpleButton,_getUpState)
{
	SimpleButton* th=asAtomHandler::as<SimpleButton>(obj);
	if(!th->upState)
	{
		asAtomHandler::setNull(ret);
		return;
	}

	th->upState->incRef();
	ret = asAtomHandler::fromObject(th->upState.getPtr());
}

ASFUNCTIONBODY_ATOM(SimpleButton,_setUpState)
{
	assert_and_throw(argslen == 1);
	SimpleButton* th=asAtomHandler::as<SimpleButton>(obj);
	th->upState = _MNR(asAtomHandler::as<DisplayObject>(args[0]));
	th->upState->incRef();
	th->reflectState(th->currentState);
}

ASFUNCTIONBODY_ATOM(SimpleButton,_getHitTestState)
{
	SimpleButton* th=asAtomHandler::as<SimpleButton>(obj);
	if(!th->hitTestState)
	{
		asAtomHandler::setNull(ret);
		return;
	}

	th->hitTestState->incRef();
	ret = asAtomHandler::fromObject(th->hitTestState.getPtr());
}

ASFUNCTIONBODY_ATOM(SimpleButton,_setHitTestState)
{
	assert_and_throw(argslen == 1);
	SimpleButton* th=asAtomHandler::as<SimpleButton>(obj);
	th->hitTestState = _MNR(asAtomHandler::as<DisplayObject>(args[0]));
	th->hitTestState->incRef();
}

ASFUNCTIONBODY_ATOM(SimpleButton,_getOverState)
{
	SimpleButton* th=asAtomHandler::as<SimpleButton>(obj);
	if(!th->overState)
	{
		asAtomHandler::setNull(ret);
		return;
	}

	th->overState->incRef();
	ret = asAtomHandler::fromObject(th->overState.getPtr());
}

ASFUNCTIONBODY_ATOM(SimpleButton,_setOverState)
{
	assert_and_throw(argslen == 1);
	SimpleButton* th=asAtomHandler::as<SimpleButton>(obj);
	th->overState = _MNR(asAtomHandler::as<DisplayObject>(args[0]));
	th->overState->incRef();
	th->reflectState(th->currentState);
}

ASFUNCTIONBODY_ATOM(SimpleButton,_getDownState)
{
	SimpleButton* th=asAtomHandler::as<SimpleButton>(obj);
	if(!th->downState)
	{
		asAtomHandler::setNull(ret);
		return;
	}

	th->downState->incRef();
	ret = asAtomHandler::fromObject(th->downState.getPtr());
}

ASFUNCTIONBODY_ATOM(SimpleButton,_setDownState)
{
	assert_and_throw(argslen == 1);
	SimpleButton* th=asAtomHandler::as<SimpleButton>(obj);
	th->downState = _MNR(asAtomHandler::as<DisplayObject>(args[0]));
	th->downState->incRef();
	th->reflectState(th->currentState);
}

ASFUNCTIONBODY_ATOM(SimpleButton,_setEnabled)
{
	SimpleButton* th=asAtomHandler::as<SimpleButton>(obj);
	assert_and_throw(argslen==1);
	th->enabled=asAtomHandler::Boolean_concrete(args[0]);
}

ASFUNCTIONBODY_ATOM(SimpleButton,_getEnabled)
{
	SimpleButton* th=asAtomHandler::as<SimpleButton>(obj);
	asAtomHandler::setBool(ret,th->enabled);
}

ASFUNCTIONBODY_ATOM(SimpleButton,_setUseHandCursor)
{
	SimpleButton* th=asAtomHandler::as<SimpleButton>(obj);
	assert_and_throw(argslen==1);
	th->useHandCursor=asAtomHandler::Boolean_concrete(args[0]);
	th->handleMouseCursor(th->hasMouse);
}

ASFUNCTIONBODY_ATOM(SimpleButton,_getUseHandCursor)
{
	SimpleButton* th=asAtomHandler::as<SimpleButton>(obj);
	asAtomHandler::setBool(ret,th->useHandCursor);
}

void GradientType::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("LINEAR",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"linear"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("RADIAL",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"radial"),CONSTANT_TRAIT);
}

void BlendMode::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("ADD",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"add"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("ALPHA",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"alpha"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("DARKEN",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"darken"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("DIFFERENCE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"difference"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("ERASE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"erase"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("HARDLIGHT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"hardlight"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("INVERT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"invert"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("LAYER",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"layer"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("LIGHTEN",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"lighten"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("MULTIPLY",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"multiply"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NORMAL",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"normal"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("OVERLAY",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"overlay"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("SCREEN",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"screen"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("SUBTRACT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"subtract"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("SHADER",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"shader"),CONSTANT_TRAIT);
}

void SpreadMethod::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("PAD",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"pad"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("REFLECT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"reflect"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("REPEAT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"repeat"),CONSTANT_TRAIT);
}

void InterpolationMethod::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("RGB",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"rgb"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("LINEAR_RGB",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"linearRGB"),CONSTANT_TRAIT);
}

void GraphicsPathCommand::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("CUBIC_CURVE_TO",nsNameAndKind(),asAtomHandler::fromUInt(6),CONSTANT_TRAIT);
	c->setVariableAtomByQName("CURVE_TO",nsNameAndKind(),asAtomHandler::fromUInt(3),CONSTANT_TRAIT);
	c->setVariableAtomByQName("LINE_TO",nsNameAndKind(),asAtomHandler::fromUInt(2),CONSTANT_TRAIT);
	c->setVariableAtomByQName("MOVE_TO",nsNameAndKind(),asAtomHandler::fromUInt(1),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NO_OP",nsNameAndKind(),asAtomHandler::fromUInt(0),CONSTANT_TRAIT);
	c->setVariableAtomByQName("WIDE_LINE_TO",nsNameAndKind(),asAtomHandler::fromUInt(5),CONSTANT_TRAIT);
	c->setVariableAtomByQName("WIDE_MOVE_TO",nsNameAndKind(),asAtomHandler::fromUInt(4),CONSTANT_TRAIT);
}

void GraphicsPathWinding::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("EVEN_ODD",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"evenOdd"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NON_ZERO",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"nonZero"),CONSTANT_TRAIT);
}

void PixelSnapping::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("ALWAYS",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"always"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("AUTO",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"auto"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NEVER",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"never"),CONSTANT_TRAIT);

}

void CapsStyle::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("NONE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"none"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("ROUND",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"round"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("SQUARE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"square"),CONSTANT_TRAIT);
}

void JointStyle::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("BEVEL",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"bevel"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("MITER",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"miter"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("ROUND",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"round"),CONSTANT_TRAIT);
}

void DisplayObjectContainer::declareFrame(bool implicit)
{
	if (needsActionScript3())
	{
		// elements of the dynamicDisplayList may be removed/added during declareFrame() calls,
		// so we create a temporary list containing all elements
		std::vector<_R<DisplayObject>> tmplist;
		cloneDisplayList(tmplist);
		auto it=tmplist.begin();
		for(;it!=tmplist.end();it++)
			(*it)->declareFrame(true);
	}
	DisplayObject::declareFrame(implicit);
}

/* Go through the hierarchy and add all
 * legacy objects which are new in the current
 * frame top-down. At the same time, call their
 * constructors in reverse order (bottom-up).
 * This is called in vm's thread context */
void DisplayObjectContainer::initFrame()
{
	/* init the frames and call constructors of our children first */

	// elements of the dynamicDisplayList may be removed during initFrame() calls,
	// so we create a temporary list containing all elements
	std::vector<_R<DisplayObject>> tmplist;
	cloneDisplayList(tmplist);
	auto it=tmplist.begin();
	for(;it!=tmplist.end();it++)
		(*it)->initFrame();
	/* call our own constructor, if necassary */
	DisplayObject::initFrame();
}

void DisplayObjectContainer::executeFrameScript()
{
	// elements of the dynamicDisplayList may be removed during executeFrameScript() calls,
	// so we create a temporary list containing all elements
	std::vector<_R<DisplayObject>> tmplist;
	cloneDisplayList(tmplist);
	auto it=tmplist.begin();
	for(;it!=tmplist.end();it++)
		(*it)->executeFrameScript();
}

multiname *DisplayObjectContainer::setVariableByMultiname(multiname& name, asAtom &o, ASObject::CONST_ALLOWED_FLAG allowConst, bool *alreadyset, ASWorker* wrk)
{
// TODO I disable this for now as gamesmenu from homestarrunner doesn't work with it (I don't know which swf file required this...)
//	if (asAtomHandler::is<DisplayObject>(o))
//	{
//		// it seems that setting a new value for a named existing dynamic child removes the child from the display list
//		variable* v = findVariableByMultiname(name,this->getClass());
//		if (v && v->kind == TRAIT_KIND::DYNAMIC_TRAIT && asAtomHandler::is<DisplayObject>(v->var))
//		{
//			DisplayObject* obj = asAtomHandler::as<DisplayObject>(v->var);
//			if (!obj->legacy)
//			{
//				if (v->var.uintval == o.uintval)
//					return nullptr;
//				obj->incRef();
//				_removeChild(obj);
//			}
//		}
//	}
	return InteractiveObject::setVariableByMultiname(name,o,allowConst,alreadyset,wrk);
}

bool DisplayObjectContainer::deleteVariableByMultiname(const multiname &name, ASWorker* wrk)
{
	variable* v = findVariableByMultiname(name,this->getClass(),nullptr,nullptr,false,wrk);
	if (v && v->kind == TRAIT_KIND::DYNAMIC_TRAIT && asAtomHandler::is<DisplayObject>(v->var))
	{
		DisplayObject* obj = asAtomHandler::as<DisplayObject>(v->var);
		if (!obj->legacy)
		{
			_removeChild(obj);
		}
	}
	return InteractiveObject::deleteVariableByMultiname(name,wrk);
}

void MovieClip::AVM1HandleConstruction()
{
	if (inAVM1Attachment || constructorCallComplete)
		return;
	setConstructIndicator();
	getSystemState()->stage->AVM1AddDisplayObject(this);
	setConstructorCallComplete();
	AVM1Function* constr = this->loadedFrom->AVM1getClassConstructor(fromDefineSpriteTag);
	if (constr)
	{
		constr->incRef();
		_NR<ASObject> pr = _MNR(constr);
		setprop_prototype(pr);
		asAtom ret = asAtomHandler::invalidAtom;
		asAtom obj = asAtomHandler::fromObjectNoPrimitive(this);
		constr->call(&ret,&obj,nullptr,0);
		AVM1registerPrototypeListeners();
	}
	afterConstruction();
}
/* Go through the hierarchy and add all
 * legacy objects which are new in the current
 * frame top-down. At the same time, call their
 * constructors in reverse order (bottom-up).
 * This is called in vm's thread context */
void MovieClip::declareFrame(bool implicit)
{
	if (state.frameadvanced && (implicit || needsActionScript3()))
		return;
	/* Go through the list of frames.
	 * If our next_FP is after our current,
	 * we construct all frames from current
	 * to next_FP.
	 * If our next_FP is before our current,
	 * we purge all objects on the 0th frame
	 * and then construct all frames from
	 * the 0th to the next_FP.
	 * We also will run the constructor on objects that got placed and deleted
	 * before state.FP (which may get us an segfault).
	 *
	 */
	if((int)state.FP < state.last_FP)
	{
		purgeLegacyChildren();
		resetToStart();
	}
	//Declared traits must exists before legacy objects are added
	if (getClass())
		getClass()->setupDeclaredTraits(this);

	// contrary to http://www.senocular.com/flash/tutorials/orderofoperations/
	// the constructors of the children are _not_ really called bottom-up but in a "mixed" fashion:
	// - the constructor of the parent is called first. that leads to calling the constructors of all super classes of the parent
	// - after the builtin super constructor of the parent was called, the constructors of the children are called
	// - after that, the remaining code of the the parents constructor is executed
	// this ensures that code from the constructor that is placed _before_ the super() call is executed before the children are constructed
	// example:
	// class testsprite : MovieClip
	// {
	//   public var childclip:MovieClip;
	//   public function testsprite()
	//   {
	//      // code here will be executed _before_ childclip is constructed
	//      super();
	//      // code here will be executed _after_ childclip was constructed
	//  }
	if (!this->getConstructIndicator())
	{
		if (needsActionScript3())
		{
			asAtom obj = asAtomHandler::fromObjectNoPrimitive(this);
			getClass()->handleConstruction(obj,nullptr,0,true);
		}
	}
	bool newFrame = (int)state.FP != state.last_FP;
	if (!needsActionScript3() && implicit && !state.frameadvanced)
		AVM1AddScriptEvents();
	if (newFrame ||!state.frameadvanced)
	{
		if(getFramesLoaded())
		{
			auto iter=frames.begin();
			for(uint32_t i=0;i<=state.FP;i++)
			{
				if((int)state.FP < state.last_FP || (int)i > state.last_FP)
				{
					iter->execute(this,i!=state.FP);
				}
				++iter;
			}
		}
		if (newFrame)
			state.frameadvanced=true;
	}
	// remove all legacy objects that have not been handled in the PlaceObject/RemoveObject tags
	LegacyChildEraseDeletionMarked();
	if (needsActionScript3())
		DisplayObjectContainer::declareFrame(implicit);
	
	if(this->isOnStage() && getSystemState()->getSwfVersion()>= 10 && frameScripts.count(0) && state.FP == 0)
	{
		// execute framescript of frame 0 after declaration is completed
		// only if we are already on stage and state.FP was not changed during construction
		this->executeFrameScript();
	}
}
void MovieClip::AVM1AddScriptEvents()
{
	if (actions)
	{
		for (auto it = actions->ClipActionRecords.begin(); it != actions->ClipActionRecords.end(); it++)
		{
			if ((it->EventFlags.ClipEventLoad && !isAVM1Loaded) ||
				(it->EventFlags.ClipEventEnterFrame && isAVM1Loaded))
			{
				AVM1scriptToExecute script;
				script.actions = &(*it).actions;
				script.startactionpos = (*it).startactionpos;
				script.avm1context = this->getCurrentFrame()->getAVM1Context();
				script.event_name_id = UINT32_MAX;
				script.clip=this;
				getSystemState()->stage->AVM1AddScriptToExecute(script);
			}
		}
	}
	AVM1scriptToExecute script;
	script.actions = nullptr;;
	script.startactionpos = 0;
	script.avm1context = nullptr;
	this->incRef(); // will be decreffed after script handler was executed 
	script.clip=this;
	script.event_name_id = isAVM1Loaded ? BUILTIN_STRINGS::STRING_ONENTERFRAME : BUILTIN_STRINGS::STRING_ONLOAD;
	getSystemState()->stage->AVM1AddScriptToExecute(script);
	
	isAVM1Loaded=true;
}
void MovieClip::initFrame()
{
	if (!needsActionScript3())
		return;
	/* Now the new legacy display objects are there, so we can also init their
	 * first frame (top-down) and call their constructors (bottom-up) */

	// work on a copy because initframe may alter the displaylist
	std::vector<_R<DisplayObject>> tmplist;
	cloneDisplayList(tmplist);
	auto it=tmplist.begin();
	for(;it!=tmplist.end();it++)
		(*it)->initFrame();

	/* Set last_FP to reflect the frame that we have initialized currently.
	 * This must be set before the constructor of this MovieClip is run,
	 * or it will call initFrame(). */
	state.last_FP=state.FP;

	/* call our own constructor, if necassary */
	if (!isConstructed())
	{
		DisplayObject::initFrame();
	}
	state.creatingframe=false;
}

void MovieClip::executeFrameScript()
{
	auto itbind = variablebindings.begin();
	while (itbind != variablebindings.end())
	{
		asAtom v = getVariableBindingValue(getSystemState()->getStringFromUniqueId((*itbind).first));
		(*itbind).second->UpdateVariableBinding(v);
		ASATOM_DECREF(v);
		itbind++;
	}
	if (!needsActionScript3())
		return;
	if (state.explicit_FP)
	{
		state.explicit_FP=false;
		// it seems that framescripts aren't executed after current frame was changed explicitely from script (only on swf files < version 10)
		if (loadedFrom->version < 10)
			return;
	}
	uint32_t f = frameScripts.count(state.FP) ? state.FP : UINT32_MAX;
	if (f != UINT32_MAX && !markedForLegacyDeletion)
	{
		if (lastFrameScriptExecuted != f)
		{
			lastFrameScriptExecuted = f;
			inExecuteFramescript = true;
			asAtom v=asAtomHandler::invalidAtom;
			this->incRef();
			asAtom obj = asAtomHandler::fromObjectNoPrimitive(this);
			asAtomHandler::callFunction(frameScripts[f],getInstanceWorker(),v,obj,nullptr,0,false);
			ASATOM_DECREF(v);
			this->decRef();
			inExecuteFramescript = false;
		}
	}
	Sprite::executeFrameScript();
}

void MovieClip::checkRatio(uint32_t ratio, bool inskipping)
{
	// according to http://wahlers.com.br/claus/blog/hacking-swf-2-placeobject-and-ratio/
	// if the ratio value is different from the previous ratio value for this MovieClip, this clip is resetted to frame 0
	if (ratio != 0 && ratio != lastratio && !state.stop_FP)
	{
		this->state.next_FP=0;
	}
	lastratio=ratio;
}

/* This is run in vm's thread context */
void DisplayObjectContainer::advanceFrame(bool implicit)
{
	if (needsActionScript3() || !implicit)
	{
		// elements of the dynamicDisplayList may be removed during advanceFrame() calls,
		// so we create a temporary list containing all elements
		std::vector<_R<DisplayObject>> tmplist;
		cloneDisplayList(tmplist);
		auto it=tmplist.begin();
		for(;it!=tmplist.end();it++)
			(*it)->advanceFrame(implicit);
	}
	else
		InteractiveObject::advanceFrame(implicit);
}		

/* Update state.last_FP. If enough frames
 * are available, set state.FP to state.next_FP.
 * This is run in vm's thread context.
 */
void MovieClip::advanceFrame(bool implicit)
{
	checkSound(state.next_FP);
	if (state.frameadvanced && state.explicit_FP)
	{
		// frame was advanced more than once in one EnterFrame event, so initFrame was not called
		// set last_FP to the FP set by previous advanceFrame
		state.last_FP=state.FP;
	}
	if (needsActionScript3())
		state.frameadvanced=false;
	state.creatingframe=true;
	/* A MovieClip can only have frames if
	 * 1a. It is a RootMovieClip
	 * 1b. or it is a DefineSpriteTag
	 * 2. and is exported as a subclass of MovieClip (see bindedTo)
	 */
	if(!this->is<RootMovieClip>() && (fromDefineSpriteTag==UINT32_MAX
	   || (!getClass()->isSubClass(Class<MovieClip>::getClass(getSystemState()))
		   && (needsActionScript3() || !getClass()->isSubClass(Class<AVM1MovieClip>::getClass(getSystemState()))))))
	{
		if (int(state.FP) >= state.last_FP) // no need to advance frame if we are moving backwards in the timline, as the timeline will be rebuild anyway
			DisplayObjectContainer::advanceFrame(true);
		declareFrame(implicit);
		return;
	}

	//If we have not yet loaded enough frames delay advancement
	if(state.next_FP>=(uint32_t)getFramesLoaded())
	{
		if(hasFinishedLoading())
		{
			if (getFramesLoaded() != 0)
				LOG(LOG_ERROR,"state.next_FP >= getFramesLoaded:"<< state.next_FP<<" "<<getFramesLoaded() <<" "<<toDebugString()<<" "<<getTagID());
			state.next_FP = state.FP;
		}
		else
			return;
	}

	if (state.next_FP != state.FP)
	{
		if (!inExecuteFramescript)
			lastFrameScriptExecuted=UINT32_MAX;
		state.FP=state.next_FP;
	}
	if(!state.stop_FP && getFramesLoaded()>0)
	{
		if (hasFinishedLoading())
			state.next_FP=imin(state.FP+1,getFramesLoaded()-1);
		else
			state.next_FP=state.FP+1;
		if(hasFinishedLoading() && state.FP == getFramesLoaded()-1)
			state.next_FP = 0;
	}
	// ensure the legacy objects of the current frame are created
	if (int(state.FP) >= state.last_FP) // no need to advance frame if we are moving backwards in the timeline, as the timeline will be rebuild anyway
		DisplayObjectContainer::advanceFrame(true);
	
	declareFrame(implicit);
	// setting state.frameadvanced ensures that the frame is not declared multiple times
	// if it was set by an actionscript command.
	state.frameadvanced = true;
	markedForLegacyDeletion=false;
}

void MovieClip::constructionComplete()
{
	Sprite::constructionComplete();

	AVM1HandleConstruction();
	/* If this object was 'new'ed from AS code, the first
	 * frame has not been initalized yet, so init the frame
	 * now */
	if(state.last_FP == -1)
	{
		advanceFrame(true);
		initFrame();
		if (getSystemState()->getSwfVersion()>= 10 && state.explicit_FP && needsActionScript3() && getParent())
		{
			// it seems that adobe executes framescripts in a very weird way whenever playhead ist explicitely changed (gotoandstop, nextframe etc.):
			// - first child of new frame is declared
			// - constructor of first child is called
			// - inside constructor super constructor is called
			// - this leads to calling the builtin constructor
			// - the framescript of the current frame is called 
			// - the rest of the constructor of the first child is executed
			getParent()->executeFrameScript();
		}
	}
}
void MovieClip::afterConstruction()
{
	if(this->isOnStage() && getSystemState()->getSwfVersion()>= 10 && frameScripts.count(0) && state.FP == 0)
	{
		// execute framescript of frame 0 after construction is completed
		// only if state.FP was not changed during construction and script was not already executed after declaration
		this->executeFrameScript();
	}
}

Frame *MovieClip::getCurrentFrame()
{
	if (state.FP >= frames.size())
	{
		LOG(LOG_ERROR,"MovieClip.getCurrentFrame invalid frame:"<<state.FP<<" "<<frames.size()<<" "<<this->toDebugString());
		throw RunTimeException("invalid current frame");
	}
	auto it = frames.begin();
	uint32_t i = 0;
	while (i < state.FP)
	{
		it++;
		i++;
	}
	return &(*it);
}

void AVM1Movie::sinit(Class_base* c)
{
	CLASS_SETUP(c, DisplayObject, _constructor, CLASS_SEALED);
}

ASFUNCTIONBODY_ATOM(AVM1Movie,_constructor)
{
	DisplayObject::_constructor(ret,wrk,obj,nullptr,0);
}

void Shader::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED);
}

ASFUNCTIONBODY_ATOM(Shader,_constructor)
{
	LOG(LOG_NOT_IMPLEMENTED, "Shader class is unimplemented.");
}

void BitmapDataChannel::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("ALPHA",nsNameAndKind(),asAtomHandler::fromUInt((uint32_t)BitmapDataChannel::ALPHA),CONSTANT_TRAIT);
	c->setVariableAtomByQName("BLUE",nsNameAndKind(),asAtomHandler::fromUInt((uint32_t)BitmapDataChannel::BLUE),CONSTANT_TRAIT);
	c->setVariableAtomByQName("GREEN",nsNameAndKind(),asAtomHandler::fromUInt((uint32_t)BitmapDataChannel::GREEN),CONSTANT_TRAIT);
	c->setVariableAtomByQName("RED",nsNameAndKind(),asAtomHandler::fromUInt((uint32_t)BitmapDataChannel::RED),CONSTANT_TRAIT);
}

unsigned int BitmapDataChannel::channelShift(uint32_t channelConstant)
{
	unsigned int shift;
	switch (channelConstant)
	{
		case BitmapDataChannel::ALPHA:
			shift = 24;
			break;
		case BitmapDataChannel::RED:
			shift = 16;
			break;
		case BitmapDataChannel::GREEN:
			shift = 8;
			break;
		case BitmapDataChannel::BLUE:
		default: // check
			shift = 0;
			break;
	}

	return shift;
}

void LineScaleMode::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("HORIZONTAL",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"horizontal"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NONE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"none"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NORMAL",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"normal"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("VERTICAL",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"vertical"),CONSTANT_TRAIT);
}

bool Stage3D::renderImpl(RenderContext &ctxt) const
{
	if (!visible || context3D.isNull())
		return false;
	return context3D->renderImpl(ctxt);
}

Stage3D::Stage3D(ASWorker* wrk, Class_base* c):EventDispatcher(wrk,c),x(0),y(0),visible(true)
{
	subtype = SUBTYPE_STAGE3D;
}

bool Stage3D::destruct()
{
	context3D.reset();
	return EventDispatcher::destruct();
}

void Stage3D::prepareShutdown()
{
	if (this->preparedforshutdown)
		return;
	EventDispatcher::prepareShutdown();
	if (context3D)
		context3D->prepareShutdown();
}

void Stage3D::sinit(Class_base *c)
{
	CLASS_SETUP(c, EventDispatcher, _constructor, CLASS_SEALED);
	c->setDeclaredMethodByQName("requestContext3D","",Class<IFunction>::getFunction(c->getSystemState(),requestContext3D),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("requestContext3DMatchingProfiles","",Class<IFunction>::getFunction(c->getSystemState(),requestContext3DMatchingProfiles),NORMAL_METHOD,true);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,x,Number);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,y,Number);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,visible,Boolean);
	REGISTER_GETTER_RESULTTYPE(c,context3D,Context3D);
}
ASFUNCTIONBODY_GETTER_SETTER(Stage3D,x)
ASFUNCTIONBODY_GETTER_SETTER(Stage3D,y)
ASFUNCTIONBODY_GETTER_SETTER(Stage3D,visible)
ASFUNCTIONBODY_GETTER(Stage3D,context3D)

ASFUNCTIONBODY_ATOM(Stage3D,_constructor)
{
	//Stage3D* th=asAtomHandler::as<Stage3D>(obj);
	EventDispatcher::_constructor(ret,wrk,obj,nullptr,0);
}
ASFUNCTIONBODY_ATOM(Stage3D,requestContext3D)
{
	Stage3D* th=asAtomHandler::as<Stage3D>(obj);
	tiny_string context3DRenderMode;
	tiny_string profile;
	ARG_CHECK(ARG_UNPACK(context3DRenderMode,"auto")(profile,"baseline"));
	
	th->context3D = _MR(Class<Context3D>::getInstanceS(wrk));
	th->context3D->driverInfo = wrk->getSystemState()->getEngineData()->driverInfoString;
	th->incRef();
	getVm(wrk->getSystemState())->addEvent(_MR(th),_MR(Class<Event>::getInstanceS(wrk,"context3DCreate")));
}
ASFUNCTIONBODY_ATOM(Stage3D,requestContext3DMatchingProfiles)
{
	//Stage3D* th=asAtomHandler::as<Stage3D>(obj);
	_NR<Vector> profiles;
	ARG_CHECK(ARG_UNPACK(profiles));
	LOG(LOG_NOT_IMPLEMENTED,"Stage3D.requestContext3DMatchingProfiles does nothing");
}

void ActionScriptVersion::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("ACTIONSCRIPT2",nsNameAndKind(),asAtomHandler::fromUInt((uint32_t)2),CONSTANT_TRAIT);
	c->setVariableAtomByQName("ACTIONSCRIPT3",nsNameAndKind(),asAtomHandler::fromUInt((uint32_t)3),CONSTANT_TRAIT);
}

void AVM1scriptToExecute::execute()
{
	std::map<uint32_t, asAtom> scopevariables;
	if (actions)
		ACTIONRECORD::executeActions(clip,avm1context,*actions, startactionpos,scopevariables);
	if (this->event_name_id != UINT32_MAX)
	{
		asAtom func=asAtomHandler::invalidAtom;
		multiname m(nullptr);
		m.name_type=multiname::NAME_STRING;
		m.isAttribute = false;
		m.name_s_id= this->event_name_id;
		clip->getVariableByMultiname(func,m,GET_VARIABLE_OPTION::NONE,clip->getInstanceWorker());
		if (asAtomHandler::is<AVM1Function>(func))
		{
			asAtom ret=asAtomHandler::invalidAtom;
			asAtom obj = asAtomHandler::fromObjectNoPrimitive(clip);
			asAtomHandler::as<AVM1Function>(func)->call(&ret,&obj,nullptr,0);
			asAtomHandler::as<AVM1Function>(func)->decRef();
		}
		clip->decRef(); // was increffed in AVM1AddScriptEvents
	}
}
