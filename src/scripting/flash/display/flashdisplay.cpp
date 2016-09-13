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
#include "scripting/argconv.h"
#include "scripting/toplevel/Vector.h"

#define FRAME_NOT_FOUND 0xffffffff //Used by getFrameIdBy*

using namespace std;
using namespace lightspark;

std::ostream& lightspark::operator<<(std::ostream& s, const DisplayObject& r)
{
	s << "[" << r.getClass()->class_name << "]";
	if(!r.name.empty())
		s << " name: " << r.name;
	return s;
}

LoaderInfo::LoaderInfo(Class_base* c):EventDispatcher(c),applicationDomain(NullRef),securityDomain(NullRef),
	contentType("application/x-shockwave-flash"),
	bytesLoaded(0),bytesTotal(0),sharedEvents(NullRef),
	loader(NullRef),bytesData(NullRef),loadStatus(STARTED),actionScriptVersion(3),swfVersion(0),
	childAllowsParent(true),uncaughtErrorEvents(NullRef),parentAllowsChild(true),frameRate(0)
{
	sharedEvents=_MR(Class<EventDispatcher>::getInstanceS(c->getSystemState()));
	parameters = _MR(Class<ASObject>::getInstanceS(c->getSystemState()));
	uncaughtErrorEvents = _MR(Class<UncaughtErrorEvents>::getInstanceS(c->getSystemState()));
	LOG(LOG_NOT_IMPLEMENTED,"LoaderInfo: childAllowsParent and parentAllowsChild always return true");
}

LoaderInfo::LoaderInfo(Class_base* c, _R<Loader> l):EventDispatcher(c),applicationDomain(NullRef),securityDomain(NullRef),
	contentType("application/x-shockwave-flash"),
	bytesLoaded(0),bytesTotal(0),sharedEvents(NullRef),
	loader(l),bytesData(NullRef),loadStatus(STARTED),actionScriptVersion(3),swfVersion(0),
	childAllowsParent(true),uncaughtErrorEvents(NullRef),parentAllowsChild(true),frameRate(0)
{
	sharedEvents=_MR(Class<EventDispatcher>::getInstanceS(c->getSystemState()));
	parameters = _MR(Class<ASObject>::getInstanceS(c->getSystemState()));
	uncaughtErrorEvents = _MR(Class<UncaughtErrorEvents>::getInstanceS(c->getSystemState()));
	LOG(LOG_NOT_IMPLEMENTED,"LoaderInfo: childAllowsParent and parentAllowsChild always return true");
}

void LoaderInfo::sinit(Class_base* c)
{
	CLASS_SETUP(c, EventDispatcher, _constructor, CLASS_SEALED);
	c->isReusable = true;
	c->setDeclaredMethodByQName("loaderURL","",Class<IFunction>::getFunction(c->getSystemState(),_getLoaderURL),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("loader","",Class<IFunction>::getFunction(c->getSystemState(),_getLoader),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("content","",Class<IFunction>::getFunction(c->getSystemState(),_getContent),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("url","",Class<IFunction>::getFunction(c->getSystemState(),_getURL),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("bytesLoaded","",Class<IFunction>::getFunction(c->getSystemState(),_getBytesLoaded),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("bytesTotal","",Class<IFunction>::getFunction(c->getSystemState(),_getBytesTotal),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("bytes","",Class<IFunction>::getFunction(c->getSystemState(),_getBytes),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("applicationDomain","",Class<IFunction>::getFunction(c->getSystemState(),_getApplicationDomain),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("sharedEvents","",Class<IFunction>::getFunction(c->getSystemState(),_getSharedEvents),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("width","",Class<IFunction>::getFunction(c->getSystemState(),_getWidth),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("height","",Class<IFunction>::getFunction(c->getSystemState(),_getHeight),GETTER_METHOD,true);
	REGISTER_GETTER(c,parameters);
	REGISTER_GETTER(c,actionScriptVersion);
	REGISTER_GETTER(c,swfVersion);
	REGISTER_GETTER(c,childAllowsParent);
	REGISTER_GETTER(c,contentType);
	REGISTER_GETTER(c,uncaughtErrorEvents);
	REGISTER_GETTER(c,parentAllowsChild);
	REGISTER_GETTER(c,frameRate);
}

ASFUNCTIONBODY_GETTER(LoaderInfo,parameters);
ASFUNCTIONBODY_GETTER(LoaderInfo,actionScriptVersion);
ASFUNCTIONBODY_GETTER(LoaderInfo,childAllowsParent);
ASFUNCTIONBODY_GETTER(LoaderInfo,contentType);
ASFUNCTIONBODY_GETTER(LoaderInfo,swfVersion);
ASFUNCTIONBODY_GETTER(LoaderInfo,uncaughtErrorEvents);
ASFUNCTIONBODY_GETTER(LoaderInfo,parentAllowsChild);
ASFUNCTIONBODY_GETTER(LoaderInfo,frameRate);

void LoaderInfo::buildTraits(ASObject* o)
{
}

bool LoaderInfo::destruct()
{
	sharedEvents.reset();
	loader.reset();
	applicationDomain.reset();
	securityDomain.reset();
	waitedObject.reset();
	bytesData.reset();
	contentType = "application/x-shockwave-flash";
	bytesLoaded = 0;
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
	return EventDispatcher::destruct();
}

void LoaderInfo::resetState()
{
	SpinlockLocker l(spinlock);
	bytesLoaded=0;
	bytesTotal=0;
	if(!bytesData.isNull())
		bytesData->setLength(0);
	loadStatus=STARTED;
}

void LoaderInfo::setBytesLoaded(uint32_t b)
{
	if(b!=bytesLoaded)
	{
		SpinlockLocker l(spinlock);
		bytesLoaded=b;
		if(getVm(getSystemState()))
		{
			this->incRef();
			getVm(getSystemState())->addEvent(_MR(this),_MR(Class<ProgressEvent>::getInstanceS(getSystemState(),bytesLoaded,bytesTotal)));
		}
		if(loadStatus==INIT_SENT)
		{
			//The clip is also complete now
			if(getVm(getSystemState()))
			{
				this->incRef();
				getVm(getSystemState())->addEvent(_MR(this),_MR(Class<Event>::getInstanceS(getSystemState(),"complete")));
			}
			loadStatus=COMPLETE;
		}
	}
}

void LoaderInfo::sendInit()
{
	this->incRef();
	getVm(getSystemState())->addEvent(_MR(this),_MR(Class<Event>::getInstanceS(getSystemState(),"init")));
	assert(loadStatus==STARTED);
	loadStatus=INIT_SENT;
	if(bytesTotal && bytesLoaded==bytesTotal)
	{
		//The clip is also complete now
		this->incRef();
		getVm(getSystemState())->addEvent(_MR(this),_MR(Class<Event>::getInstanceS(getSystemState(),"complete")));
		loadStatus=COMPLETE;
	}
}

void LoaderInfo::setWaitedObject(_NR<DisplayObject> w)
{
	SpinlockLocker l(spinlock);
	waitedObject = w;
}

void LoaderInfo::objectHasLoaded(_R<DisplayObject> obj)
{
	SpinlockLocker l(spinlock);
	if(waitedObject != obj)
		return;
	if(!loader.isNull() && obj==waitedObject)
		loader->setContent(obj);
	sendInit();
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
		parameters = _MR(Class<ASObject>::getInstanceS(getSystemState()));
		SystemState::parseParametersFromURLIntoObject(url, parameters);
	}
}

ASFUNCTIONBODY(LoaderInfo,_constructor)
{
	//LoaderInfo* th=static_cast<LoaderInfo*>(obj);
	EventDispatcher::_constructor(obj,NULL,0);
	return NULL;
}

ASFUNCTIONBODY(LoaderInfo,_getLoaderURL)
{
	LoaderInfo* th=static_cast<LoaderInfo*>(obj);
	return abstract_s(obj->getSystemState(),th->loaderURL);
}

ASFUNCTIONBODY(LoaderInfo,_getContent)
{
	//Use Loader::getContent
	LoaderInfo* th=static_cast<LoaderInfo*>(obj);
	if(th->loader.isNull())
		return obj->getSystemState()->getUndefinedRef();
	else
		return Loader::_getContent(th->loader.getPtr(),NULL,0);
}

ASFUNCTIONBODY(LoaderInfo,_getLoader)
{
	LoaderInfo* th=static_cast<LoaderInfo*>(obj);
	if(th->loader.isNull())
		return obj->getSystemState()->getUndefinedRef();
	else
	{
		th->loader->incRef();
		return th->loader.getPtr();
	}
}

ASFUNCTIONBODY(LoaderInfo,_getSharedEvents)
{
	LoaderInfo* th=static_cast<LoaderInfo*>(obj);
	th->sharedEvents->incRef();
	return th->sharedEvents.getPtr();
}

ASFUNCTIONBODY(LoaderInfo,_getURL)
{
	LoaderInfo* th=static_cast<LoaderInfo*>(obj);
	return abstract_s(obj->getSystemState(),th->url);
}

ASFUNCTIONBODY(LoaderInfo,_getBytesLoaded)
{
	LoaderInfo* th=static_cast<LoaderInfo*>(obj);
	return abstract_i(obj->getSystemState(),th->bytesLoaded);
}

ASFUNCTIONBODY(LoaderInfo,_getBytesTotal)
{
	LoaderInfo* th=static_cast<LoaderInfo*>(obj);
	return abstract_i(obj->getSystemState(),th->bytesTotal);
}

ASFUNCTIONBODY(LoaderInfo,_getBytes)
{
	LoaderInfo* th=static_cast<LoaderInfo*>(obj);
	if (th->bytesData.isNull())
		th->bytesData = _NR<ByteArray>(Class<ByteArray>::getInstanceS(obj->getSystemState()));
	if (!th->loader->getContent().isNull())
		th->bytesData->writeObject(th->loader->getContent().getPtr());

	return th->bytesData.getPtr();
}

ASFUNCTIONBODY(LoaderInfo,_getApplicationDomain)
{
	LoaderInfo* th=static_cast<LoaderInfo*>(obj);
	if(th->applicationDomain.isNull())
		return obj->getSystemState()->getNullRef();

	th->applicationDomain->incRef();
	return th->applicationDomain.getPtr();
}

ASFUNCTIONBODY(LoaderInfo,_getWidth)
{
	LoaderInfo* th=static_cast<LoaderInfo*>(obj);
	_NR<Loader> l = th->loader;
	if(l.isNull())
		return abstract_d(obj->getSystemState(),0);
	_NR<DisplayObject> o=l->getContent();
	if (o.isNull())
		return abstract_d(obj->getSystemState(),0);

	return abstract_d(obj->getSystemState(),o->getNominalWidth());
}

ASFUNCTIONBODY(LoaderInfo,_getHeight)
{
	LoaderInfo* th=static_cast<LoaderInfo*>(obj);
	_NR<Loader> l = th->loader;
	if(l.isNull())
		return abstract_d(obj->getSystemState(),0);
	_NR<DisplayObject> o=l->getContent();
	if (o.isNull())
		return abstract_d(obj->getSystemState(),0);

	return abstract_d(obj->getSystemState(),o->getNominalHeight());
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
		_R<MemoryStreamCache> cache(_MR(new MemoryStreamCache));
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
			loaderInfo->incRef();
			getVm(loader->getSystemState())->addEvent(loaderInfo,_MR(Class<IOErrorEvent>::getInstanceS(loader->getSystemState())));
			loader->incRef();
			getVm(loader->getSystemState())->addEvent(loader,_MR(Class<IOErrorEvent>::getInstanceS(loader->getSystemState())));
			delete sbuf;
			// downloader will be deleted in jobFence
			return;
		}
		loaderInfo->incRef();
		getVm(loader->getSystemState())->addEvent(loaderInfo,_MR(Class<Event>::getInstanceS(loader->getSystemState(),"open")));
	}
	else if(source==BYTES)
	{
		assert_and_throw(bytes->bytes);

		loaderInfo->incRef();
		getVm(loader->getSystemState())->addEvent(loaderInfo,_MR(Class<Event>::getInstanceS(loader->getSystemState(),"open")));
		loaderInfo->setBytesTotal(bytes->getLength());
		loaderInfo->setBytesLoaded(bytes->getLength());

		sbuf = new bytes_buf(bytes->bytes,bytes->getLength());
	}

	istream s(sbuf);
	ParseThread local_pt(s,loaderInfo->applicationDomain,loaderInfo->securityDomain,loader.getPtr(),url.getParsedURL());
	local_pt.execute();

	// Delete the bytes container (cache reader or bytes_buf)
	delete sbuf;
	sbuf = NULL;
	if (source==URL) {
		//Acquire the lock to ensure consistency in threadAbort
		SpinlockLocker l(downloaderLock);
		if(downloader)
			loaderInfo->getSystemState()->downloadManager->destroy(downloader);
		downloader=NULL;
	}

	bytes.reset();

	_NR<DisplayObject> obj=local_pt.getParsedObject();
	if(obj.isNull())
	{
		// The stream did not contain RootMovieClip or Bitmap
		if(!threadAborting)
		{
			loaderInfo->incRef();
			getVm(loader->getSystemState())->addEvent(loaderInfo,_MR(Class<IOErrorEvent>::getInstanceS(loader->getSystemState())));
		}
		return;
	}
}

ASFUNCTIONBODY(Loader,_constructor)
{
	Loader* th=static_cast<Loader*>(obj);
	DisplayObjectContainer::_constructor(obj,NULL,0);
	th->contentLoaderInfo->setLoaderURL(obj->getSystemState()->mainClip->getOrigin().getParsedURL());
	th->uncaughtErrorEvents = _MR(Class<UncaughtErrorEvents>::getInstanceS(obj->getSystemState()));
	return NULL;
}

ASFUNCTIONBODY(Loader,_getContent)
{
	Loader* th=static_cast<Loader*>(obj);
	SpinlockLocker l(th->spinlock);
	_NR<ASObject> ret=th->content;
	if(ret.isNull())
		ret=_MR(obj->getSystemState()->getUndefinedRef());

	ret->incRef();
	return ret.getPtr();
}

ASFUNCTIONBODY(Loader,_getContentLoaderInfo)
{
	Loader* th=static_cast<Loader*>(obj);
	th->contentLoaderInfo->incRef();
	return th->contentLoaderInfo.getPtr();
}

ASFUNCTIONBODY(Loader,close)
{
	Loader* th=static_cast<Loader*>(obj);
 	SpinlockLocker l(th->spinlock);
	for (auto j=th->jobs.begin(); j!=th->jobs.end(); j++)
		(*j)->threadAbort();

	return NULL;
}

ASFUNCTIONBODY(Loader,load)
{
	Loader* th=static_cast<Loader*>(obj);

	th->unload();
	_NR<URLRequest> r;
	_NR<LoaderContext> context;
	ARG_UNPACK (r)(context, NullRef);
	th->url=r->getRequestURL();
	th->contentLoaderInfo->setURL(th->url.getParsedURL());
	th->contentLoaderInfo->resetState();
	//Check if a security domain has been manually set
	_NR<SecurityDomain> secDomain;
	_NR<SecurityDomain> curSecDomain=ABCVm::getCurrentSecurityDomain(getVm(th->getSystemState())->currentCallContext);
	if(!context.isNull())
	{
		if (!context->securityDomain.isNull())
		{
			//The passed domain must be the current one. See Loader::load specs.
			if(context->securityDomain!=curSecDomain)
				throw Class<SecurityError>::getInstanceS(obj->getSystemState(),"SecurityError: securityDomain must be current one");
			secDomain=curSecDomain;
		}

		bool sameDomain = (secDomain == curSecDomain);
		th->allowCodeImport = !sameDomain || context->getAllowCodeImport();

		if (!context->parameters.isNull())
			th->contentLoaderInfo->setParameters(context->parameters);
	}
	//Default is to create a child ApplicationDomain if the file is in the same security context
	//otherwise create a child of the system domain. If the security domain is different
	//the passed applicationDomain is ignored
	_R<RootMovieClip> currentRoot=getVm(th->getSystemState())->currentCallContext->context->root;
	// empty origin is possible if swf is loaded by loadBytes()
	if(currentRoot->getOrigin().isEmpty() || currentRoot->getOrigin().getHostname()==th->url.getHostname() || !secDomain.isNull())
	{
		//Same domain
		_NR<ApplicationDomain> parentDomain = currentRoot->applicationDomain;
		//Support for LoaderContext
		if(context.isNull() || context->applicationDomain.isNull())
			th->contentLoaderInfo->applicationDomain = _MR(Class<ApplicationDomain>::getInstanceS(obj->getSystemState(),parentDomain));
		else
			th->contentLoaderInfo->applicationDomain = context->applicationDomain;
		th->contentLoaderInfo->securityDomain = curSecDomain;
	}
	else
	{
		//Different domain
		_NR<ApplicationDomain> parentDomain =  obj->getSystemState()->systemDomain;
		th->contentLoaderInfo->applicationDomain = _MR(Class<ApplicationDomain>::getInstanceS(obj->getSystemState(),parentDomain));
		th->contentLoaderInfo->securityDomain = _MR(Class<SecurityDomain>::getInstanceS(obj->getSystemState()));
	}

	if(!th->url.isValid())
	{
		//Notify an error during loading
		th->incRef();
		obj->getSystemState()->currentVm->addEvent(_MR(th),_MR(Class<IOErrorEvent>::getInstanceS(obj->getSystemState())));
		return NULL;
	}

	SecurityManager::checkURLStaticAndThrow(th->url, ~(SecurityManager::LOCAL_WITH_FILE),
		SecurityManager::LOCAL_WITH_FILE | SecurityManager::LOCAL_TRUSTED, true);

	if (!context.isNull() && context->getCheckPolicyFile())
	{
		//TODO: this should be async as it could block if invoked from ExternalInterface
		SecurityManager::EVALUATIONRESULT evaluationResult;
		evaluationResult = obj->getSystemState()->securityManager->evaluatePoliciesURL(th->url, true);
		if(evaluationResult == SecurityManager::NA_CROSSDOMAIN_POLICY)
		{
			// should this dispatch SecurityErrorEvent instead of throwing?
			throw Class<SecurityError>::getInstanceS(obj->getSystemState(),
				"SecurityError: connection to domain not allowed by securityManager");
		}
	}

	th->incRef();
	r->incRef();
	LoaderThread *thread=new LoaderThread(_MR(r), _MR(th));

	SpinlockLocker l(th->spinlock);
	th->jobs.push_back(thread);
	obj->getSystemState()->addJob(thread);

	return NULL;
}

ASFUNCTIONBODY(Loader,loadBytes)
{
	Loader* th=static_cast<Loader*>(obj);

	th->unload();

	_NR<ByteArray> bytes;
	_NR<LoaderContext> context;
	ARG_UNPACK (bytes)(context, NullRef);

	_NR<ApplicationDomain> parentDomain = ABCVm::getCurrentApplicationDomain(getVm(th->getSystemState())->currentCallContext);
	if(context.isNull() || context->applicationDomain.isNull())
		th->contentLoaderInfo->applicationDomain = _MR(Class<ApplicationDomain>::getInstanceS(obj->getSystemState(),parentDomain));
	else
		th->contentLoaderInfo->applicationDomain = context->applicationDomain;
	//Always loaded in the current security domain
	_NR<SecurityDomain> curSecDomain=ABCVm::getCurrentSecurityDomain(getVm(th->getSystemState())->currentCallContext);
	th->contentLoaderInfo->securityDomain = curSecDomain;

	th->allowCodeImport = context.isNull() || context->getAllowCodeImport();

	if (!context.isNull() && !context->parameters.isNull())
		th->contentLoaderInfo->setParameters(context->parameters);

	if(bytes->getLength()!=0)
	{
		th->incRef();
		LoaderThread *thread=new LoaderThread(_MR(bytes), _MR(th));
		SpinlockLocker l(th->spinlock);
		th->jobs.push_back(thread);
		obj->getSystemState()->addJob(thread);
	}
	else
		LOG(LOG_INFO, "Empty ByteArray passed to Loader.loadBytes");
	return NULL;
}

ASFUNCTIONBODY(Loader,_unload)
{
	Loader* th=static_cast<Loader*>(obj);
	th->unload();
	return NULL;
}
ASFUNCTIONBODY(Loader,_unloadAndStop)
{
	Loader* th=static_cast<Loader*>(obj);
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

	return NULL;
}

void Loader::unload()
{
	_NR<DisplayObject> content_copy = NullRef;
	{
		SpinlockLocker l(spinlock);
		for (auto j=jobs.begin(); j!=jobs.end(); j++)
			(*j)->threadAbort();

		content_copy=content;
		content.reset();
	}
	
	if(loaded)
	{
		contentLoaderInfo->incRef();
		getVm(getSystemState())->addEvent(contentLoaderInfo,_MR(Class<Event>::getInstanceS(getSystemState(),"unload")));
		loaded=false;
	}

	// removeChild may execute AS code, release the lock before
	// calling
	if(content_copy)
		_removeChild(content_copy);

	contentLoaderInfo->resetState();
}

void Loader::finalize()
{
	DisplayObjectContainer::finalize();
	content.reset();
	contentLoaderInfo.reset();
}

Loader::Loader(Class_base* c):DisplayObjectContainer(c),content(NullRef),contentLoaderInfo(NullRef),loaded(false), allowCodeImport(true),uncaughtErrorEvents(NullRef)
{
	incRef();
	contentLoaderInfo=_MR(Class<LoaderInfo>::getInstanceS(c->getSystemState(),_MR(this)));
}

Loader::~Loader()
{
}

void Loader::sinit(Class_base* c)
{
	CLASS_SETUP(c, DisplayObjectContainer, _constructor, CLASS_SEALED);
	c->setDeclaredMethodByQName("contentLoaderInfo","",Class<IFunction>::getFunction(c->getSystemState(),_getContentLoaderInfo),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("content","",Class<IFunction>::getFunction(c->getSystemState(),_getContent),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("close","",Class<IFunction>::getFunction(c->getSystemState(),close),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("loadBytes","",Class<IFunction>::getFunction(c->getSystemState(),loadBytes),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("load","",Class<IFunction>::getFunction(c->getSystemState(),load),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("unload","",Class<IFunction>::getFunction(c->getSystemState(),_unload),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("unloadAndStop","",Class<IFunction>::getFunction(c->getSystemState(),_unloadAndStop),NORMAL_METHOD,true);
	REGISTER_GETTER(c,uncaughtErrorEvents);
}

ASFUNCTIONBODY_GETTER(Loader,uncaughtErrorEvents);

void Loader::threadFinished(IThreadJob* finishedJob)
{
	SpinlockLocker l(spinlock);
	jobs.remove(finishedJob);
	delete finishedJob;
}

void Loader::buildTraits(ASObject* o)
{
}

void Loader::setContent(_R<DisplayObject> o)
{
	{
		Locker l(mutexDisplayList);
		dynamicDisplayList.clear();
	}

	{
		SpinlockLocker l(spinlock);
		content=o;
		content->isLoadedRoot = true;
		loaded=true;
	}

	// _addChild may cause AS code to run, release locks beforehand.
	_addChildAt(o, 0);
}

Sprite::Sprite(Class_base* c):DisplayObjectContainer(c),TokenContainer(this),graphics(NullRef),buttonMode(false),useHandCursor(false)
{
}

bool Sprite::destruct()
{
	graphics.reset();
	hitArea.reset();
	hitTarget.reset();
	buttonMode = false;
	useHandCursor = false;
	return DisplayObjectContainer::destruct();
}

void Sprite::sinit(Class_base* c)
{
	CLASS_SETUP(c, DisplayObjectContainer, _constructor, CLASS_SEALED);
	c->isReusable = true;
	c->setDeclaredMethodByQName("graphics","",Class<IFunction>::getFunction(c->getSystemState(),_getGraphics),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("startDrag","",Class<IFunction>::getFunction(c->getSystemState(),_startDrag),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("stopDrag","",Class<IFunction>::getFunction(c->getSystemState(),_stopDrag),NORMAL_METHOD,true);
	REGISTER_GETTER_SETTER(c, buttonMode);
	REGISTER_GETTER_SETTER(c, hitArea);
	REGISTER_GETTER_SETTER(c, useHandCursor);
}

ASFUNCTIONBODY_GETTER_SETTER(Sprite, buttonMode);
ASFUNCTIONBODY_GETTER_SETTER(Sprite, useHandCursor);

void Sprite::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(Sprite,_startDrag)
{
	Sprite* th=Class<Sprite>::cast(obj);
	bool lockCenter = false;
	const RECT* bounds = NULL;
	if(argslen > 0)
		lockCenter = ArgumentConversion<bool>::toConcrete(args[0]);
	if(argslen > 1)
	{
		Rectangle* rect = Class<Rectangle>::cast(args[1]);
		if(!rect)
			throw Class<ArgumentError>::getInstanceS(obj->getSystemState(),"Wrong type");
		bounds = new RECT(rect->getRect());
	}

	Vector2f offset;
	if(!lockCenter)
	{
		offset = -th->getParent()->getLocalMousePos();
		offset += th->getXY();
	}

	th->incRef();
	obj->getSystemState()->getInputThread()->startDrag(_MR(th), bounds, offset);
	return NULL;
}

ASFUNCTIONBODY(Sprite,_stopDrag)
{
	Sprite* th=Class<Sprite>::cast(obj);
	obj->getSystemState()->getInputThread()->stopDrag(th);
	return NULL;
}

ASFUNCTIONBODY_GETTER(Sprite, hitArea);

ASFUNCTIONBODY(Sprite,_setter_hitArea)
{
	Sprite* th=Class<Sprite>::cast(obj);
	_NR<Sprite> value;
	ARG_UNPACK(value);

	if (!th->hitArea.isNull())
		th->hitArea->hitTarget.reset();

	th->hitArea = value;
	if (!th->hitArea.isNull())
	{
		th->incRef();
		th->hitArea->hitTarget = _MNR(th);
	}

	return NULL;
}

bool DisplayObjectContainer::boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const
{
	bool ret = false;

	if(dynamicDisplayList.empty())
		return false;

	Locker l(mutexDisplayList);
	list<_R<DisplayObject>>::const_iterator it=dynamicDisplayList.begin();
	for(;it!=dynamicDisplayList.end();++it)
	{
		number_t txmin,txmax,tymin,tymax;
		if((*it)->getBounds(txmin,txmax,tymin,tymax,(*it)->getMatrix()))
		{
			if(ret==true)
			{
				xmin = imin(xmin,txmin);
				xmax = imax(xmax,txmax);
				ymin = imin(ymin,tymin);
				ymax = imax(ymax,tymax);
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
	return ret;
}

bool Sprite::boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const
{
	bool ret;
	ret = DisplayObjectContainer::boundsRect(xmin,xmax,ymin,ymax);
	number_t txmin,txmax,tymin,tymax;
	if(TokenContainer::boundsRect(txmin,txmax,tymin,tymax))
	{
		if(ret==true)
		{
			xmin = imin(xmin,txmin);
			xmax = imax(xmax,txmax);
			ymin = imin(ymin,tymin);
			ymax = imax(ymax,tymax);
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

void Sprite::requestInvalidation(InvalidateQueue* q)
{
	DisplayObjectContainer::requestInvalidation(q);
	TokenContainer::requestInvalidation(q);
}

void DisplayObjectContainer::renderImpl(RenderContext& ctxt) const
{
	Locker l(mutexDisplayList);
	//Now draw also the display list
	list<_R<DisplayObject>>::const_iterator it=dynamicDisplayList.begin();
	for(;it!=dynamicDisplayList.end();++it)
	{
		//Skip the drawing of masks
		if((*it)->isMask())
			continue;
		(*it)->Render(ctxt);
	}
}

void Sprite::renderImpl(RenderContext& ctxt) const
{
	//Draw the dynamically added graphics, if any
	if(!tokensEmpty())
		defaultRender(ctxt);

	DisplayObjectContainer::renderImpl(ctxt);
}

/*
Subclasses of DisplayObjectContainer must still check
isHittable() to see if they should send out events.
*/
_NR<DisplayObject> DisplayObjectContainer::hitTestImpl(_NR<DisplayObject> last, number_t x, number_t y, DisplayObject::HIT_TYPE type)
{
	_NR<DisplayObject> ret = NullRef;
	//Test objects added at runtime, in reverse order
	Locker l(mutexDisplayList);
	list<_R<DisplayObject>>::const_reverse_iterator j=dynamicDisplayList.rbegin();
	for(;j!=dynamicDisplayList.rend();++j)
	{
		//Don't check masks
		if((*j)->isMask())
			continue;

		if(!(*j)->getMatrix().isInvertible())
			continue; /* The object is shrunk to zero size */

		number_t localX, localY;
		(*j)->getMatrix().getInverted().multiply2D(x,y,localX,localY);
		this->incRef();
		ret=(*j)->hitTest(_MR(this), localX,localY, type);
		if(!ret.isNull())
			break;
	}
	/* When mouseChildren is false, we should get all events of our children */
	if(ret && !mouseChildren)
	{
		this->incRef();
		ret = _MNR(this);
	}
	return ret;
}

_NR<DisplayObject> Sprite::hitTestImpl(_NR<DisplayObject>, number_t x, number_t y, DisplayObject::HIT_TYPE type)
{
	//Did we hit a children?
	_NR<DisplayObject> ret = NullRef;
	this->incRef();
	ret = DisplayObjectContainer::hitTestImpl(_MR(this),x,y, type);

	if (ret.isNull() && hitArea.isNull())
	{
		//The coordinates are locals
		this->incRef();
		ret = TokenContainer::hitTestImpl(_MR(this),x,y, type);

		if (!ret.isNull())  //Did we hit the sprite?
		{
			if (!hitTarget.isNull())
			{
				//Another Sprite has registered us
				//as its hitArea -> relay the hit
				if (hitTarget->isHittable(type))
					ret = hitTarget;
				else
					ret.reset();
			}
			else if (!isHittable(type))
			{
				//Hit ignored due to a disabled HIT_TYPE
				ret.reset();
			}
		}
	}

	return ret;
}

ASFUNCTIONBODY(Sprite,_constructor)
{
	//Sprite* th=Class<Sprite>::cast(obj);
	DisplayObjectContainer::_constructor(obj,NULL,0);
	return NULL;
}

ASFUNCTIONBODY(Sprite,_getGraphics)
{
	Sprite* th=static_cast<Sprite*>(obj);
	//Probably graphics is not used often, so create it here
	if(th->graphics.isNull())
		th->graphics=_MR(Class<Graphics>::getInstanceS(obj->getSystemState(),th));

	th->graphics->incRef();
	return th->graphics.getPtr();
}

FrameLabel::FrameLabel(Class_base* c):ASObject(c)
{
}

FrameLabel::FrameLabel(Class_base* c, const FrameLabel_data& data):ASObject(c),FrameLabel_data(data)
{
}

void FrameLabel::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setDeclaredMethodByQName("frame","",Class<IFunction>::getFunction(c->getSystemState(),_getFrame),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("name","",Class<IFunction>::getFunction(c->getSystemState(),_getName),GETTER_METHOD,true);
}

void FrameLabel::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(FrameLabel,_getFrame)
{
	FrameLabel* th=static_cast<FrameLabel*>(obj);
	return abstract_i(obj->getSystemState(),th->frame);
}

ASFUNCTIONBODY(FrameLabel,_getName)
{
	FrameLabel* th=static_cast<FrameLabel*>(obj);
	return abstract_s(obj->getSystemState(),th->name);
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

Scene::Scene(Class_base* c):ASObject(c)
{
}

Scene::Scene(Class_base* c, const Scene_data& data, uint32_t _numFrames):ASObject(c),Scene_data(data),numFrames(_numFrames)
{
}

void Scene::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setDeclaredMethodByQName("labels","",Class<IFunction>::getFunction(c->getSystemState(),_getLabels),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("name","",Class<IFunction>::getFunction(c->getSystemState(),_getName),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("numFrames","",Class<IFunction>::getFunction(c->getSystemState(),_getNumFrames),GETTER_METHOD,true);
}

ASFUNCTIONBODY(Scene,_getLabels)
{
	Scene* th=static_cast<Scene*>(obj);
	Array* ret = Class<Array>::getInstanceSNoArgs(obj->getSystemState());
	ret->resize(th->labels.size());
	for(size_t i=0; i<th->labels.size(); ++i)
	{
		ret->set(i, _MR(Class<FrameLabel>::getInstanceS(obj->getSystemState(),th->labels[i])));
	}
	return ret;
}

ASFUNCTIONBODY(Scene,_getName)
{
	Scene* th=static_cast<Scene*>(obj);
	return abstract_s(obj->getSystemState(),th->name);
}

ASFUNCTIONBODY(Scene,_getNumFrames)
{
	Scene* th=static_cast<Scene*>(obj);
	return abstract_i(obj->getSystemState(),th->numFrames);
}

void Frame::destroyTags()
{
	auto it=blueprint.begin();
	for(;it!=blueprint.end();++it)
		delete (*it);
}

void Frame::execute(_R<DisplayObjectContainer> displayList)
{
	auto it=blueprint.begin();
	for(;it!=blueprint.end();++it)
		(*it)->execute(displayList.getPtr());
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
void FrameContainer::addToFrame(const DisplayListTag* t)
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
	c->setDeclaredMethodByQName("currentFrame","",Class<IFunction>::getFunction(c->getSystemState(),_getCurrentFrame),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("totalFrames","",Class<IFunction>::getFunction(c->getSystemState(),_getTotalFrames),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("framesLoaded","",Class<IFunction>::getFunction(c->getSystemState(),_getFramesLoaded),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("currentFrameLabel","",Class<IFunction>::getFunction(c->getSystemState(),_getCurrentFrameLabel),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("currentLabel","",Class<IFunction>::getFunction(c->getSystemState(),_getCurrentLabel),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("currentLabels","",Class<IFunction>::getFunction(c->getSystemState(),_getCurrentLabels),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("scenes","",Class<IFunction>::getFunction(c->getSystemState(),_getScenes),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("currentScene","",Class<IFunction>::getFunction(c->getSystemState(),_getCurrentScene),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("stop","",Class<IFunction>::getFunction(c->getSystemState(),stop),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("play","",Class<IFunction>::getFunction(c->getSystemState(),play),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("gotoAndStop","",Class<IFunction>::getFunction(c->getSystemState(),gotoAndStop),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("gotoAndPlay","",Class<IFunction>::getFunction(c->getSystemState(),gotoAndPlay),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("prevFrame","",Class<IFunction>::getFunction(c->getSystemState(),prevFrame),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("nextFrame","",Class<IFunction>::getFunction(c->getSystemState(),nextFrame),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("addFrameScript","",Class<IFunction>::getFunction(c->getSystemState(),addFrameScript),NORMAL_METHOD,true);
	REGISTER_GETTER_SETTER(c, enabled);
}

ASFUNCTIONBODY_GETTER_SETTER(MovieClip, enabled);

void MovieClip::buildTraits(ASObject* o)
{
}

MovieClip::MovieClip(Class_base* c):Sprite(c),fromDefineSpriteTag(false),totalFrames_unreliable(1),enabled(true)
{
}

MovieClip::MovieClip(Class_base* c, const FrameContainer& f, bool defineSpriteTag):Sprite(c),FrameContainer(f),fromDefineSpriteTag(defineSpriteTag),totalFrames_unreliable(frames.size()),enabled(true)
{
	//For sprites totalFrames_unreliable is the actual frame count
	//For the root movie, it's the frame count from the header
}

bool MovieClip::destruct()
{
	frames.clear();
	frameScripts.clear();
	fromDefineSpriteTag = false;
	totalFrames_unreliable = 1;
	enabled = true;
	return Sprite::destruct();
}

/* Returns a Scene_data pointer for a scene called sceneName, or for
 * the current scene if sceneName is empty. Returns NULL, if not found.
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

	return NULL;  //Not found!
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
				if(scenes[i].labels[j].name == label)
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
				if(scene->labels[j].name == label)
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

ASFUNCTIONBODY(MovieClip,addFrameScript)
{
	MovieClip* th=Class<MovieClip>::cast(obj);
	assert_and_throw(argslen>=2 && argslen%2==0);

	for(uint32_t i=0;i<argslen;i+=2)
	{
		uint32_t frame=args[i]->toInt();

		if(args[i+1]->getObjectType()!=T_FUNCTION)
		{
			LOG(LOG_ERROR,_("Not a function"));
			return NULL;
		}
		IFunction* f=static_cast<IFunction*>(args[i+1]);
		f->incRef();
		th->frameScripts[frame]=_MNR(f);
	}
	
	return NULL;
}

ASFUNCTIONBODY(MovieClip,swapDepths)
{
	LOG(LOG_NOT_IMPLEMENTED,_("Called swapDepths"));
	return NULL;
}

ASFUNCTIONBODY(MovieClip,stop)
{
	MovieClip* th=static_cast<MovieClip*>(obj);
	th->state.stop_FP=true;
	th->state.next_FP=th->state.FP;
	return NULL;
}

ASFUNCTIONBODY(MovieClip,play)
{
	MovieClip* th=static_cast<MovieClip*>(obj);
	th->state.stop_FP=false;
	return NULL;
}

ASObject* MovieClip::gotoAnd(ASObject* const* args, const unsigned int argslen, bool stop)
{
	uint32_t next_FP;
	tiny_string sceneName;
	assert_and_throw(argslen==1 || argslen==2);
	if(argslen==2)
	{
		sceneName = args[1]->toString();
	}
	if(args[0]->getObjectType()==T_STRING)
	{
		uint32_t dest=getFrameIdByLabel(args[0]->toString(), sceneName);
		if(dest==FRAME_NOT_FOUND)
		{
			LOG(LOG_ERROR, (stop ? "gotoAndStop: label not found:" : "gotoAndPlay: label not found:") <<args[0]->toString());
			throwError<ArgumentError>(kInvalidArgumentError,args[0]->toString());
		}

		next_FP = dest;
	}
	else
	{
		uint32_t inFrameNo = args[0]->toInt();
		if(inFrameNo == 0)
			return NULL; /*this behavior was observed by testing */

		next_FP = getFrameIdByNumber(inFrameNo-1, sceneName);
		if(next_FP > getFramesLoaded())
		{
			LOG(LOG_ERROR, next_FP << "= next_FP >= state.max_FP = " << getFramesLoaded());
			/* spec says we should throw an error, but then YT breaks */
			//throwError<ArgumentError>(kInvalidArgumentError,stop ? "gotoAndStop: frame not found" : "gotoAndPlay: frame not found");
			next_FP = getFramesLoaded()-1;
		}
	}

	state.next_FP = next_FP;
	state.explicit_FP = true;
	state.stop_FP = stop;
	return NULL;
}

ASFUNCTIONBODY(MovieClip,gotoAndStop)
{
	MovieClip* th=static_cast<MovieClip*>(obj);
	return th->gotoAnd(args,argslen,true);
}

ASFUNCTIONBODY(MovieClip,gotoAndPlay)
{
	MovieClip* th=static_cast<MovieClip*>(obj);
	return th->gotoAnd(args,argslen,false);
}

ASFUNCTIONBODY(MovieClip,nextFrame)
{
	MovieClip* th=static_cast<MovieClip*>(obj);
	assert_and_throw(th->state.FP<th->getFramesLoaded());
	th->state.next_FP = th->state.FP+1;
	th->state.explicit_FP=true;
	if (!th->getParent())
	{
		th->advanceFrame();
		th->initFrame();
	}
	return NULL;
}

ASFUNCTIONBODY(MovieClip,prevFrame)
{
	MovieClip* th=static_cast<MovieClip*>(obj);
	assert_and_throw(th->state.FP<th->getFramesLoaded());
	th->state.next_FP = th->state.FP-1;
	th->state.explicit_FP=true;
	if (!th->getParent())
	{
		th->advanceFrame();
		th->initFrame();
	}
	return NULL;
}

ASFUNCTIONBODY(MovieClip,_getFramesLoaded)
{
	MovieClip* th=static_cast<MovieClip*>(obj);
	return abstract_i(obj->getSystemState(),th->getFramesLoaded());
}

ASFUNCTIONBODY(MovieClip,_getTotalFrames)
{
	MovieClip* th=static_cast<MovieClip*>(obj);
	return abstract_i(obj->getSystemState(),th->totalFrames_unreliable);
}

ASFUNCTIONBODY(MovieClip,_getScenes)
{
	MovieClip* th=static_cast<MovieClip*>(obj);
	Array* ret = Class<Array>::getInstanceSNoArgs(obj->getSystemState());
	ret->resize(th->scenes.size());
	uint32_t numFrames;
	for(size_t i=0; i<th->scenes.size(); ++i)
	{
		if(i == th->scenes.size()-1)
			numFrames = th->totalFrames_unreliable - th->scenes[i].startframe;
		else
			numFrames = th->scenes[i].startframe - th->scenes[i+1].startframe;
		ret->set(i, _MR(Class<Scene>::getInstanceS(obj->getSystemState(),th->scenes[i],numFrames)));
	}
	return ret;
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

ASFUNCTIONBODY(MovieClip,_getCurrentScene)
{
	MovieClip* th=static_cast<MovieClip*>(obj);
	uint32_t numFrames;
	uint32_t curScene = th->getCurrentScene();
	if(curScene == th->scenes.size()-1)
		numFrames = th->totalFrames_unreliable - th->scenes[curScene].startframe;
	else
		numFrames = th->scenes[curScene].startframe - th->scenes[curScene+1].startframe;

	return Class<Scene>::getInstanceS(obj->getSystemState(),th->scenes[curScene],numFrames);
}

ASFUNCTIONBODY(MovieClip,_getCurrentFrame)
{
	MovieClip* th=static_cast<MovieClip*>(obj);
	//currentFrame is 1-based and relative to current scene
	if (th->state.explicit_FP)
		// if frame is set explicitly, the currentframe property should be set to next_FP, even if it is not displayed yet
		return abstract_i(obj->getSystemState(),th->state.next_FP+1 - th->scenes[th->getCurrentScene()].startframe);
	return abstract_i(obj->getSystemState(),th->state.FP+1 - th->scenes[th->getCurrentScene()].startframe);
}

ASFUNCTIONBODY(MovieClip,_getCurrentFrameLabel)
{
	MovieClip* th=static_cast<MovieClip*>(obj);
	for(size_t i=0;i<th->scenes.size();++i)
	{
		for(size_t j=0;j<th->scenes[i].labels.size();++j)
			if(th->scenes[i].labels[j].frame == th->state.FP)
				return abstract_s(obj->getSystemState(),th->scenes[i].labels[j].name);
	}
	return obj->getSystemState()->getNullRef();
}

ASFUNCTIONBODY(MovieClip,_getCurrentLabel)
{
	MovieClip* th=static_cast<MovieClip*>(obj);
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
		return obj->getSystemState()->getNullRef();
	else
		return abstract_s(obj->getSystemState(),label);
}

ASFUNCTIONBODY(MovieClip,_getCurrentLabels)
{
	MovieClip* th=static_cast<MovieClip*>(obj);
	Scene_data& sc = th->scenes[th->getCurrentScene()];

	Array* ret = Class<Array>::getInstanceSNoArgs(obj->getSystemState());
	ret->resize(sc.labels.size());
	for(size_t i=0; i<sc.labels.size(); ++i)
	{
		ret->set(i, _MR(Class<FrameLabel>::getInstanceS(obj->getSystemState(),sc.labels[i])));
	}
	return ret;
}

ASFUNCTIONBODY(MovieClip,_constructor)
{
	Sprite::_constructor(obj,NULL,0);
/*	th->setVariableByQName("swapDepths","",Class<IFunction>::getFunction(c->getSystemState(),swapDepths));
	th->setVariableByQName("createEmptyMovieClip","",Class<IFunction>::getFunction(c->getSystemState(),createEmptyMovieClip));*/
	return NULL;
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

void DisplayObjectContainer::sinit(Class_base* c)
{
	CLASS_SETUP(c, InteractiveObject, _constructor, CLASS_SEALED);
	c->isReusable = true;
	c->setDeclaredMethodByQName("numChildren","",Class<IFunction>::getFunction(c->getSystemState(),_getNumChildren),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("getChildIndex","",Class<IFunction>::getFunction(c->getSystemState(),_getChildIndex),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setChildIndex","",Class<IFunction>::getFunction(c->getSystemState(),_setChildIndex),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getChildAt","",Class<IFunction>::getFunction(c->getSystemState(),getChildAt),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getChildByName","",Class<IFunction>::getFunction(c->getSystemState(),getChildByName),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("addChild","",Class<IFunction>::getFunction(c->getSystemState(),addChild),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("removeChild","",Class<IFunction>::getFunction(c->getSystemState(),removeChild),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("removeChildAt","",Class<IFunction>::getFunction(c->getSystemState(),removeChildAt),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("addChildAt","",Class<IFunction>::getFunction(c->getSystemState(),addChildAt),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("swapChildren","",Class<IFunction>::getFunction(c->getSystemState(),swapChildren),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("contains","",Class<IFunction>::getFunction(c->getSystemState(),contains),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("mouseChildren","",Class<IFunction>::getFunction(c->getSystemState(),_setMouseChildren),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("mouseChildren","",Class<IFunction>::getFunction(c->getSystemState(),_getMouseChildren),GETTER_METHOD,true);
	REGISTER_GETTER_SETTER(c, tabChildren);
}

ASFUNCTIONBODY_GETTER_SETTER(DisplayObjectContainer, tabChildren);

void DisplayObjectContainer::buildTraits(ASObject* o)
{
}

DisplayObjectContainer::DisplayObjectContainer(Class_base* c):InteractiveObject(c),mouseChildren(true),tabChildren(true)
{
}

bool DisplayObjectContainer::hasLegacyChildAt(uint32_t depth)
{
	auto i = depthToLegacyChild.left.find(depth);
	return i != depthToLegacyChild.left.end();
}

void DisplayObjectContainer::deleteLegacyChildAt(uint32_t depth)
{
	if(!hasLegacyChildAt(depth))
	{
		LOG(LOG_ERROR,"deleteLegacyChildAt: no child at that depth");
		return;
	}
	DisplayObject* obj = depthToLegacyChild.left.at(depth);
	if(!obj->name.empty())
	{
		//The variable is not deleted, but just set to null
		//This is a tested behavior
		multiname objName(NULL);
		objName.name_type=multiname::NAME_STRING;
		objName.name_s_id=getSystemState()->getUniqueStringId(obj->name);
		objName.ns.emplace_back(getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
		setVariableByMultiname(objName,getSystemState()->getNullRef(), ASObject::CONST_NOT_ALLOWED);
	}

	obj->incRef();
	//this also removes it from depthToLegacyChild
	bool ret = _removeChild(_MR(obj));
	assert_and_throw(ret);
}

void DisplayObjectContainer::insertLegacyChildAt(uint32_t depth, DisplayObject* obj)
{
	if(hasLegacyChildAt(depth))
	{
		LOG(LOG_ERROR,"insertLegacyChildAt: there is already one child at that depth");
		return;
	}
	_addChildAt(_MR(obj),depth-1); /* depth is 1 based in SWF */
	if(!obj->name.empty())
	{
		obj->incRef();
		multiname objName(NULL);
		objName.name_type=multiname::NAME_STRING;
		objName.name_s_id=getSystemState()->getUniqueStringId(obj->name);
		objName.ns.emplace_back(getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
		setVariableByMultiname(objName,obj,ASObject::CONST_NOT_ALLOWED);
	}

	depthToLegacyChild.insert(boost::bimap<uint32_t,DisplayObject*>::value_type(depth,obj));
}

void DisplayObjectContainer::transformLegacyChildAt(uint32_t depth, const MATRIX& mat)
{
	if(!hasLegacyChildAt(depth))
	{
		LOG(LOG_ERROR,"transformLegacyChildAt: no child at that depth");
		return;
	}
	depthToLegacyChild.left.at(depth)->setLegacyMatrix(mat);
}

void DisplayObjectContainer::purgeLegacyChildren()
{
	auto i = depthToLegacyChild.begin();
	while( i != depthToLegacyChild.end() )
	{
		deleteLegacyChildAt(i->left);
		i = depthToLegacyChild.begin();
	}
}

bool DisplayObjectContainer::destruct()
{
	//Release every child
	dynamicDisplayList.clear();
	mouseChildren = true;
	tabChildren = true;
	return InteractiveObject::destruct();
}

InteractiveObject::InteractiveObject(Class_base* c):DisplayObject(c),mouseEnabled(true),doubleClickEnabled(false),accessibilityImplementation(NullRef),contextMenu(NullRef),tabEnabled(false),tabIndex(-1)
{
}

InteractiveObject::~InteractiveObject()
{
	if(getSystemState()->getInputThread())
		getSystemState()->getInputThread()->removeListener(this);
}

ASFUNCTIONBODY(InteractiveObject,_constructor)
{
	InteractiveObject* th=static_cast<InteractiveObject*>(obj);
	EventDispatcher::_constructor(obj,NULL,0);
	//Object registered very early are not supported this way (Stage for example)
	if(obj->getSystemState()->getInputThread())
		obj->getSystemState()->getInputThread()->addListener(th);

	return NULL;
}

ASFUNCTIONBODY(InteractiveObject,_setMouseEnabled)
{
	InteractiveObject* th=static_cast<InteractiveObject*>(obj);
	assert_and_throw(argslen==1);
	th->mouseEnabled=Boolean_concrete(args[0]);
	return NULL;
}

ASFUNCTIONBODY(InteractiveObject,_getMouseEnabled)
{
	InteractiveObject* th=static_cast<InteractiveObject*>(obj);
	return abstract_b(obj->getSystemState(),th->mouseEnabled);
}

ASFUNCTIONBODY(InteractiveObject,_setDoubleClickEnabled)
{
	InteractiveObject* th=static_cast<InteractiveObject*>(obj);
	assert_and_throw(argslen==1);
	th->doubleClickEnabled=Boolean_concrete(args[0]);
	return NULL;
}

ASFUNCTIONBODY(InteractiveObject,_getDoubleClickEnabled)
{
	InteractiveObject* th=static_cast<InteractiveObject*>(obj);
	return abstract_b(obj->getSystemState(),th->doubleClickEnabled);
}

bool InteractiveObject::destruct()
{
	contextMenu.reset();
	mouseEnabled = true;
	doubleClickEnabled =false;
	accessibilityImplementation.reset();
	tabEnabled = false;
	tabIndex = -1;
	return DisplayObject::destruct();
}

void InteractiveObject::buildTraits(ASObject* o)
{
}

void InteractiveObject::sinit(Class_base* c)
{
	CLASS_SETUP(c, DisplayObject, _constructor, CLASS_SEALED);
	c->isReusable = true;
	c->setDeclaredMethodByQName("mouseEnabled","",Class<IFunction>::getFunction(c->getSystemState(),_setMouseEnabled),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("mouseEnabled","",Class<IFunction>::getFunction(c->getSystemState(),_getMouseEnabled),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("doubleClickEnabled","",Class<IFunction>::getFunction(c->getSystemState(),_setDoubleClickEnabled),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("doubleClickEnabled","",Class<IFunction>::getFunction(c->getSystemState(),_getDoubleClickEnabled),GETTER_METHOD,true);
	REGISTER_GETTER_SETTER(c, accessibilityImplementation);
	REGISTER_GETTER_SETTER(c, contextMenu);
	REGISTER_GETTER_SETTER(c, tabEnabled);
	REGISTER_GETTER_SETTER(c, tabIndex);
	REGISTER_GETTER_SETTER(c, focusRect);
}

ASFUNCTIONBODY_GETTER_SETTER(InteractiveObject, accessibilityImplementation);
ASFUNCTIONBODY_GETTER_SETTER(InteractiveObject, contextMenu);
ASFUNCTIONBODY_GETTER_SETTER(InteractiveObject, tabEnabled);
ASFUNCTIONBODY_GETTER_SETTER(InteractiveObject, tabIndex);
ASFUNCTIONBODY_GETTER_SETTER(InteractiveObject, focusRect); // stub

void DisplayObjectContainer::dumpDisplayList(unsigned int level)
{
	tiny_string indent(std::string(2*level, ' '));
	list<_R<DisplayObject> >::const_iterator it=dynamicDisplayList.begin();
	for(;it!=dynamicDisplayList.end();++it)
	{
		Vector2f pos = (*it)->getXY();
		LOG(LOG_INFO, indent << (*it)->getClass()->class_name <<
		    " (" << pos.x << "," << pos.y << ") " <<
		    (*it)->getNominalWidth() << "x" << (*it)->getNominalHeight() << " " <<
		    ((*it)->isVisible() ? "v" : "") <<
		    ((*it)->isMask() ? "m" : "") << " " <<
		    "a=" << (*it)->clippedAlpha() << " " <<
		    it->getPtr());

		if ((*it)->is<DisplayObjectContainer>())
		{
			(*it)->as<DisplayObjectContainer>()->dumpDisplayList(level+1);
		}
	}
}

void DisplayObjectContainer::setOnStage(bool staged)
{
	if(staged!=onStage)
	{
		DisplayObject::setOnStage(staged);
		//Notify childern
		//Make a copy of display list, and release the mutex
		//before calling setOnStage
		list<_R<DisplayObject>> displayListCopy;
		{
			Locker l(mutexDisplayList);
			displayListCopy.assign(dynamicDisplayList.begin(),
					       dynamicDisplayList.end());
		}
		list<_R<DisplayObject>>::const_iterator it=displayListCopy.begin();
		for(;it!=displayListCopy.end();++it)
			(*it)->setOnStage(staged);
	}
}

ASFUNCTIONBODY(DisplayObjectContainer,_constructor)
{
	InteractiveObject::_constructor(obj,NULL,0);
	return NULL;
}

ASFUNCTIONBODY(DisplayObjectContainer,_getNumChildren)
{
	DisplayObjectContainer* th=static_cast<DisplayObjectContainer*>(obj);
	return abstract_i(obj->getSystemState(),th->dynamicDisplayList.size());
}

ASFUNCTIONBODY(DisplayObjectContainer,_getMouseChildren)
{
	DisplayObjectContainer* th=static_cast<DisplayObjectContainer*>(obj);
	return abstract_b(obj->getSystemState(),th->mouseChildren);
}

ASFUNCTIONBODY(DisplayObjectContainer,_setMouseChildren)
{
	DisplayObjectContainer* th=static_cast<DisplayObjectContainer*>(obj);
	assert_and_throw(argslen==1);
	th->mouseChildren=Boolean_concrete(args[0]);
	return NULL;
}

void DisplayObjectContainer::requestInvalidation(InvalidateQueue* q)
{
	DisplayObject::requestInvalidation(q);
	Locker l(mutexDisplayList);
	list<_R<DisplayObject>>::const_iterator it=dynamicDisplayList.begin();
	for(;it!=dynamicDisplayList.end();++it)
		(*it)->requestInvalidation(q);
}

void DisplayObjectContainer::_addChildAt(_R<DisplayObject> child, unsigned int index)
{
	//If the child has no parent, set this container to parent
	//If there is a previous parent, purge the child from his list
	if(!child->getParent().isNull())
	{
		//Child already in this container
		if(child->getParent()==this)
			return;
		else
			child->getParent()->_removeChild(child);
	}
	this->incRef();
	child->setParent(_MR(this));
	{
		Locker l(mutexDisplayList);
		//We insert the object in the back of the list
		if(index >= dynamicDisplayList.size())
			dynamicDisplayList.push_back(child);
		else
		{
			list<_R<DisplayObject>>::iterator it=dynamicDisplayList.begin();
			for(unsigned int i=0;i<index;i++)
				++it;
			dynamicDisplayList.insert(it,child);
		}
	}
	child->setOnStage(onStage);
}

bool DisplayObjectContainer::_removeChild(_R<DisplayObject> child)
{
	if(!child->getParent() || child->getParent()!=this)
		return false;

	{
		Locker l(mutexDisplayList);
		list<_R<DisplayObject>>::iterator it=find(dynamicDisplayList.begin(),dynamicDisplayList.end(),child);
		if(it==dynamicDisplayList.end())
			return false;
		dynamicDisplayList.erase(it);

		//Erase this from the legacy child map (if it is in there)
		depthToLegacyChild.right.erase(child.getPtr());
	}
	child->setOnStage(false);
	child->setParent(NullRef);
	return true;
}

bool DisplayObjectContainer::_contains(_R<DisplayObject> d)
{
	if(d==this)
		return true;

	list<_R<DisplayObject>>::const_iterator it=dynamicDisplayList.begin();
	for(;it!=dynamicDisplayList.end();++it)
	{
		if(*it==d)
			return true;
		DisplayObjectContainer* c=dynamic_cast<DisplayObjectContainer*>((*it).getPtr());
		if(c && c->_contains(d))
			return true;
	}
	return false;
}

ASFUNCTIONBODY(DisplayObjectContainer,contains)
{
	DisplayObjectContainer* th=static_cast<DisplayObjectContainer*>(obj);
	assert_and_throw(argslen==1);
	if(args[0]->getObjectType() == T_CLASS)
		return abstract_b(obj->getSystemState(),false);
	if (!args[0]->getClass())
		return abstract_b(obj->getSystemState(),false);
	if (!args[0]->getClass()->isSubClass(Class<DisplayObject>::getClass(obj->getSystemState())))
		return abstract_b(obj->getSystemState(),false);

	//Cast to object
	DisplayObject* d=static_cast<DisplayObject*>(args[0]);
	d->incRef();
	bool ret=th->_contains(_MR(d));
	return abstract_b(obj->getSystemState(),ret);
}

//Only from VM context
ASFUNCTIONBODY(DisplayObjectContainer,addChildAt)
{
	DisplayObjectContainer* th=static_cast<DisplayObjectContainer*>(obj);
	assert_and_throw(argslen==2);
	if(args[0]->getObjectType() == T_CLASS)
	{
		return obj->getSystemState()->getNullRef();
	}
	//Validate object type
	assert_and_throw(args[0]->getClass() &&
		args[0]->getClass()->isSubClass(Class<DisplayObject>::getClass(obj->getSystemState())));

	int index=args[1]->toInt();

	//Cast to object
	args[0]->incRef();
	_R<DisplayObject> d=_MR(Class<DisplayObject>::cast(args[0]));
	assert_and_throw(index >= 0 && (size_t)index<=th->dynamicDisplayList.size());
	th->_addChildAt(d,index);

	//Notify the object
	d->incRef();
	getVm(obj->getSystemState())->addEvent(d,_MR(Class<Event>::getInstanceS(obj->getSystemState(),"added")));

	//incRef again as the value is getting returned
	d->incRef();
	return d.getPtr();
}

ASFUNCTIONBODY(DisplayObjectContainer,addChild)
{
	DisplayObjectContainer* th=static_cast<DisplayObjectContainer*>(obj);
	assert_and_throw(argslen==1);
	if(args[0]->getObjectType() == T_CLASS)
	{
		return obj->getSystemState()->getNullRef();
	}
	//Validate object type
	assert_and_throw(args[0] && args[0]->getClass() && 
		args[0]->getClass()->isSubClass(Class<DisplayObject>::getClass(obj->getSystemState())));

	//Cast to object
	args[0]->incRef();
	_R<DisplayObject> d=_MR(Class<DisplayObject>::cast(args[0]));
	th->_addChildAt(d,numeric_limits<unsigned int>::max());

	//Notify the object
	d->incRef();
	getVm(obj->getSystemState())->addEvent(d,_MR(Class<Event>::getInstanceS(obj->getSystemState(),"added")));

	d->incRef();
	return d.getPtr();
}

//Only from VM context
ASFUNCTIONBODY(DisplayObjectContainer,removeChild)
{
	DisplayObjectContainer* th=static_cast<DisplayObjectContainer*>(obj);
	assert_and_throw(argslen==1);
	if(args[0]->getObjectType() == T_CLASS ||
	   args[0]->getObjectType() == T_UNDEFINED ||
	   args[0]->getObjectType() == T_NULL)
	{
		return obj->getSystemState()->getNullRef();
	}
	//Validate object type
	assert_and_throw(args[0] && args[0]->getClass() && 
		args[0]->getClass()->isSubClass(Class<DisplayObject>::getClass(obj->getSystemState())));
	//Cast to object
	DisplayObject* d=Class<DisplayObject>::cast(args[0]);
	d->incRef();
	if(!th->_removeChild(_MR(d)))
		throw Class<ArgumentError>::getInstanceS(obj->getSystemState(),"removeChild: child not in list", 2025);

	//As we return the child we have to incRef it
	d->incRef();
	return d;
}

//Only from VM context
ASFUNCTIONBODY(DisplayObjectContainer,removeChildAt)
{
	DisplayObjectContainer* th=static_cast<DisplayObjectContainer*>(obj);
	assert_and_throw(argslen==1);
	//Validate object type
	int32_t index=args[0]->toInt();

	DisplayObject* child;
	{
		Locker l(th->mutexDisplayList);
		if(index>=int(th->dynamicDisplayList.size()) || index<0)
			throw Class<RangeError>::getInstanceS(obj->getSystemState(),"removeChildAt: invalid index", 2025);
		list<_R<DisplayObject>>::iterator it=th->dynamicDisplayList.begin();
		for(int32_t i=0;i<index;i++)
			++it;
		child=(*it).getPtr();
		//incRef before the refrence is destroyed
		child->incRef();
		th->dynamicDisplayList.erase(it);
	}
	child->setOnStage(false);
	child->setParent(NullRef);

	//As we return the child we don't decRef it
	return child;
}

ASFUNCTIONBODY(DisplayObjectContainer,_setChildIndex)
{
	DisplayObjectContainer* th=static_cast<DisplayObjectContainer*>(obj);
	assert_and_throw(argslen==2);

	//Validate object type
	assert_and_throw(args[0] && args[0]->getClass() &&
		args[0]->getClass()->isSubClass(Class<DisplayObject>::getClass(obj->getSystemState())));
	args[0]->incRef();
	_R<DisplayObject> child = _MR(Class<DisplayObject>::cast(args[0]));

	int index=args[1]->toInt();
	int curIndex = th->getChildIndex(child);

	if(curIndex == index)
		return NULL;

	Locker l(th->mutexDisplayList);
	th->dynamicDisplayList.remove(child); //remove from old position

	list<_R<DisplayObject>>::iterator it=th->dynamicDisplayList.begin();
	int i = 0;
	for(;it != th->dynamicDisplayList.end(); ++it)
		if(i++ == index)
		{
			th->dynamicDisplayList.insert(it, child);
			return NULL;
		}

	th->dynamicDisplayList.push_back(child);
	return NULL;
}

ASFUNCTIONBODY(DisplayObjectContainer,swapChildren)
{
	DisplayObjectContainer* th=static_cast<DisplayObjectContainer*>(obj);
	assert_and_throw(argslen==2);
	
	//Validate object type
	assert_and_throw(args[0] && args[0]->getClass() && 
		args[0]->getClass()->isSubClass(Class<DisplayObject>::getClass(obj->getSystemState())));
	assert_and_throw(args[1] && args[1]->getClass() && 
		args[1]->getClass()->isSubClass(Class<DisplayObject>::getClass(obj->getSystemState())));

	if (args[0] == args[1])
	{
		// Must return, otherwise crashes trying to erase the
		// same object twice
		return NULL;
	}

	//Cast to object
	args[0]->incRef();
	_R<DisplayObject> child1=_MR(Class<DisplayObject>::cast(args[0]));
	args[1]->incRef();
	_R<DisplayObject> child2=_MR(Class<DisplayObject>::cast(args[1]));

	{
		Locker l(th->mutexDisplayList);
		std::list<_R<DisplayObject>>::iterator it1=find(th->dynamicDisplayList.begin(),th->dynamicDisplayList.end(),child1);
		std::list<_R<DisplayObject>>::iterator it2=find(th->dynamicDisplayList.begin(),th->dynamicDisplayList.end(),child2);
		if(it1==th->dynamicDisplayList.end() || it2==th->dynamicDisplayList.end())
			throw Class<ArgumentError>::getInstanceS(obj->getSystemState(),"Argument is not child of this object", 2025);

		th->dynamicDisplayList.insert(it1, child2);
		th->dynamicDisplayList.insert(it2, child1);
		th->dynamicDisplayList.erase(it1);
		th->dynamicDisplayList.erase(it2);
	}
	
	return NULL;
}

//Only from VM context
ASFUNCTIONBODY(DisplayObjectContainer,getChildByName)
{
	DisplayObjectContainer* th=static_cast<DisplayObjectContainer*>(obj);
	assert_and_throw(argslen==1);
	const tiny_string& wantedName=args[0]->toString();
	list<_R<DisplayObject>>::iterator it=th->dynamicDisplayList.begin();
	ASObject* ret=NULL;
	for(;it!=th->dynamicDisplayList.end();++it)
	{
		if((*it)->name==wantedName)
		{
			ret=(*it).getPtr();
			break;
		}
	}
	if(ret)
		ret->incRef();
	else
		ret=obj->getSystemState()->getUndefinedRef();
	return ret;
}

//Only from VM context
ASFUNCTIONBODY(DisplayObjectContainer,getChildAt)
{
	DisplayObjectContainer* th=static_cast<DisplayObjectContainer*>(obj);
	assert_and_throw(argslen==1);
	unsigned int index=args[0]->toInt();
	if(index>=th->dynamicDisplayList.size())
		throw Class<RangeError>::getInstanceS(obj->getSystemState(),"getChildAt: invalid index", 2025);
	list<_R<DisplayObject>>::iterator it=th->dynamicDisplayList.begin();
	for(unsigned int i=0;i<index;i++)
		++it;

	(*it)->incRef();
	return (*it).getPtr();
}

int DisplayObjectContainer::getChildIndex(_R<DisplayObject> child)
{
	list<_R<DisplayObject>>::const_iterator it = dynamicDisplayList.begin();
	int ret = 0;
	do
	{
		if(*it == child)
			break;
		ret++;
		++it;
		if(it == dynamicDisplayList.end())
			throw Class<ArgumentError>::getInstanceS(getSystemState(),"getChildIndex: child not in list", 2025);
	}
	while(1);
	return ret;
}

//Only from VM context
ASFUNCTIONBODY(DisplayObjectContainer,_getChildIndex)
{
	DisplayObjectContainer* th=static_cast<DisplayObjectContainer*>(obj);
	assert_and_throw(argslen==1);
	//Validate object type
	assert_and_throw(args[0]->getClass()->isSubClass(Class<DisplayObject>::getClass(obj->getSystemState())));

	//Cast to object
	_R<DisplayObject> d= _MR(static_cast<DisplayObject*>(args[0]));
	d->incRef();

	return abstract_i(obj->getSystemState(),th->getChildIndex(d));
}

Shape::Shape(Class_base* c):DisplayObject(c),TokenContainer(this),graphics(NullRef)
{
}

Shape::Shape(Class_base* c, const tokensVector& tokens, float scaling):
	DisplayObject(c),TokenContainer(this, tokens, scaling),graphics(NullRef)
{
}

void Shape::finalize()
{
	DisplayObject::finalize();
	graphics.reset();
}

void Shape::sinit(Class_base* c)
{
	CLASS_SETUP(c, DisplayObject, _constructor, CLASS_SEALED);
	c->setDeclaredMethodByQName("graphics","",Class<IFunction>::getFunction(c->getSystemState(),_getGraphics),GETTER_METHOD,true);
}

void Shape::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(Shape,_constructor)
{
	DisplayObject::_constructor(obj,NULL,0);
	return NULL;
}

ASFUNCTIONBODY(Shape,_getGraphics)
{
	Shape* th=static_cast<Shape*>(obj);
	if(th->graphics.isNull())
		th->graphics=_MR(Class<Graphics>::getInstanceS(obj->getSystemState(),th));
	th->graphics->incRef();
	return th->graphics.getPtr();
}

void MorphShape::sinit(Class_base* c)
{
	// FIXME: should use _constructorNotInstantiatable but then
	// DefineMorphShapeTag::instance breaks
	CLASS_SETUP_NO_CONSTRUCTOR(c, DisplayObject, CLASS_SEALED | CLASS_FINAL);
}

void MorphShape::buildTraits(ASObject* o)
{
	//No traits
}

bool MorphShape::boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const
{
	LOG(LOG_NOT_IMPLEMENTED, "MorphShape::boundsRect is a stub");
	return false;
}

_NR<DisplayObject> MorphShape::hitTestImpl(_NR<DisplayObject> last, number_t x, number_t y, HIT_TYPE type)
{
	return NullRef;
}

void Stage::sinit(Class_base* c)
{
	CLASS_SETUP(c, DisplayObjectContainer, _constructor, CLASS_SEALED);
	c->setDeclaredMethodByQName("allowFullScreen","",Class<IFunction>::getFunction(c->getSystemState(),_getAllowFullScreen),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("allowFullScreenInteractive","",Class<IFunction>::getFunction(c->getSystemState(),_getAllowFullScreenInteractive),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("colorCorrectionSupport","",Class<IFunction>::getFunction(c->getSystemState(),_getColorCorrectionSupport),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("fullScreenHeight","",Class<IFunction>::getFunction(c->getSystemState(),_getStageHeight),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("fullScreenWidth","",Class<IFunction>::getFunction(c->getSystemState(),_getStageWidth),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("stageWidth","",Class<IFunction>::getFunction(c->getSystemState(),_getStageWidth),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("stageWidth","",Class<IFunction>::getFunction(c->getSystemState(),undefinedFunction),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("stageHeight","",Class<IFunction>::getFunction(c->getSystemState(),_getStageHeight),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("stageHeight","",Class<IFunction>::getFunction(c->getSystemState(),undefinedFunction),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("width","",Class<IFunction>::getFunction(c->getSystemState(),_getStageWidth),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("height","",Class<IFunction>::getFunction(c->getSystemState(),_getStageHeight),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("scaleMode","",Class<IFunction>::getFunction(c->getSystemState(),_getScaleMode),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("scaleMode","",Class<IFunction>::getFunction(c->getSystemState(),_setScaleMode),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("loaderInfo","",Class<IFunction>::getFunction(c->getSystemState(),_getLoaderInfo),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("stageVideos","",Class<IFunction>::getFunction(c->getSystemState(),_getStageVideos),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("focus","",Class<IFunction>::getFunction(c->getSystemState(),_getFocus),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("focus","",Class<IFunction>::getFunction(c->getSystemState(),_setFocus),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("frameRate","",Class<IFunction>::getFunction(c->getSystemState(),_getFrameRate),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("frameRate","",Class<IFunction>::getFunction(c->getSystemState(),_setFrameRate),SETTER_METHOD,true);
	// override the setter from DisplayObjectContainer
	c->setDeclaredMethodByQName("tabChildren","",Class<IFunction>::getFunction(c->getSystemState(),_setTabChildren),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("wmodeGPU","",Class<IFunction>::getFunction(c->getSystemState(),_getWmodeGPU),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("invalidate","",Class<IFunction>::getFunction(c->getSystemState(),_invalidate),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("color","",Class<IFunction>::getFunction(c->getSystemState(),_getColor),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("color","",Class<IFunction>::getFunction(c->getSystemState(),_setColor),SETTER_METHOD,true);
	REGISTER_GETTER_SETTER(c,align);
	REGISTER_GETTER_SETTER(c,colorCorrection);
	REGISTER_GETTER_SETTER(c,displayState);
	REGISTER_GETTER_SETTER(c,fullScreenSourceRect);
	REGISTER_GETTER_SETTER(c,showDefaultContextMenu);
	REGISTER_GETTER_SETTER(c,quality);
	REGISTER_GETTER_SETTER(c,stageFocusRect);
	REGISTER_GETTER(c,allowsFullScreen);
}

ASFUNCTIONBODY_GETTER_SETTER_CB(Stage,align,onAlign);
ASFUNCTIONBODY_GETTER_SETTER_CB(Stage,colorCorrection,onColorCorrection);
ASFUNCTIONBODY_GETTER_SETTER_CB(Stage,displayState,onDisplayState);
ASFUNCTIONBODY_GETTER_SETTER(Stage,showDefaultContextMenu);  // stub
ASFUNCTIONBODY_GETTER_SETTER_CB(Stage,fullScreenSourceRect,onFullScreenSourceRect);
ASFUNCTIONBODY_GETTER_SETTER(Stage,quality);
ASFUNCTIONBODY_GETTER_SETTER(Stage,stageFocusRect);  // stub
ASFUNCTIONBODY_GETTER_NOT_IMPLEMENTED(Stage,allowsFullScreen);  // stub

void Stage::onDisplayState(const tiny_string&)
{
	if (displayState != "normal")
		LOG(LOG_NOT_IMPLEMENTED,"Stage.displayState = " << displayState);
	displayState = "normal"; // until fullscreen support is implemented
}

void Stage::onAlign(const tiny_string& /*oldValue*/)
{
	LOG(LOG_NOT_IMPLEMENTED, "Stage.align = " << align);
}

void Stage::onColorCorrection(const tiny_string& oldValue)
{
	if (colorCorrection != "default" && 
	    colorCorrection != "on" && 
	    colorCorrection != "off")
	{
		colorCorrection = oldValue;
		throwError<ArgumentError>(kInvalidEnumError, "colorCorrection");
	}
}

void Stage::onFullScreenSourceRect(_NR<Rectangle> /*oldValue*/)
{
	LOG(LOG_NOT_IMPLEMENTED, "Stage.fullScreenSourceRect");
	fullScreenSourceRect.reset();
}

void Stage::eventListenerAdded(const tiny_string& eventName)
{
	if (eventName == "stageVideoAvailability")
	{
		// StageVideoAvailabilityEvent is dispatched directly after an eventListener is added added
		// see https://www.adobe.com/devnet/flashplayer/articles/stage_video.html 
		this->incRef();
		getVm(getSystemState())->addEvent(_MR(this),_MR(Class<StageVideoAvailabilityEvent>::getInstanceS(getSystemState())));
	}
}

void Stage::buildTraits(ASObject* o)
{
}

Stage::Stage(Class_base* c):
	DisplayObjectContainer(c), colorCorrection("default"),
	showDefaultContextMenu(true),quality("high"),stageFocusRect(false),allowsFullScreen(false)
{
	onStage = true;
}

_NR<Stage> Stage::getStage()
{
	this->incRef();
	return _MR(this);
}

ASFUNCTIONBODY(Stage,_constructor)
{
	return NULL;
}

_NR<DisplayObject> Stage::hitTestImpl(_NR<DisplayObject> last, number_t x, number_t y, DisplayObject::HIT_TYPE type)
{
	_NR<DisplayObject> ret;
	ret = DisplayObjectContainer::hitTestImpl(last, x, y, type);
	if(!ret)
	{
		/* If nothing else is hit, we hit the stage */
		this->incRef();
		ret = _MNR(this);
	}
	return ret;
}

uint32_t Stage::internalGetWidth() const
{
	uint32_t width;
	if(getSystemState()->scaleMode==SystemState::NO_SCALE)
		width=getSystemState()->getRenderThread()->windowWidth;
	else
	{
		RECT size=getSystemState()->mainClip->getFrameSize();
		width=size.Xmax/20;
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
		height=size.Ymax/20;
	}
	return height;
}

ASFUNCTIONBODY(Stage,_getStageWidth)
{
	Stage* th=static_cast<Stage*>(obj);
	return abstract_d(obj->getSystemState(),th->internalGetWidth());
}

ASFUNCTIONBODY(Stage,_getStageHeight)
{
	Stage* th=static_cast<Stage*>(obj);
	return abstract_d(obj->getSystemState(),th->internalGetHeight());
}

ASFUNCTIONBODY(Stage,_getLoaderInfo)
{
	return RootMovieClip::_getLoaderInfo(obj->getSystemState()->mainClip,NULL,0);
}

ASFUNCTIONBODY(Stage,_getScaleMode)
{
	//Stage* th=static_cast<Stage*>(obj);
	switch(obj->getSystemState()->scaleMode)
	{
		case SystemState::EXACT_FIT:
			return abstract_s(obj->getSystemState(),"exactFit");
		case SystemState::SHOW_ALL:
			return abstract_s(obj->getSystemState(),"showAll");
		case SystemState::NO_BORDER:
			return abstract_s(obj->getSystemState(),"noBorder");
		case SystemState::NO_SCALE:
			return abstract_s(obj->getSystemState(),"noScale");
	}
	return NULL;
}

ASFUNCTIONBODY(Stage,_setScaleMode)
{
	//Stage* th=static_cast<Stage*>(obj);
	const tiny_string& arg0=args[0]->toString();
	if(arg0=="exactFit")
		obj->getSystemState()->scaleMode=SystemState::EXACT_FIT;
	else if(arg0=="showAll")
		obj->getSystemState()->scaleMode=SystemState::SHOW_ALL;
	else if(arg0=="noBorder")
		obj->getSystemState()->scaleMode=SystemState::NO_BORDER;
	else if(arg0=="noScale")
		obj->getSystemState()->scaleMode=SystemState::NO_SCALE;

	RenderThread* rt=obj->getSystemState()->getRenderThread();
	rt->requestResize(rt->windowWidth, rt->windowHeight, true);
	return NULL;
}

ASFUNCTIONBODY(Stage,_getStageVideos)
{
	LOG(LOG_NOT_IMPLEMENTED, "Accelerated rendering through StageVideo not implemented, SWF should fall back to Video");
	return Template<Vector>::getInstanceS(obj->getSystemState(),Class<StageVideo>::getClass(obj->getSystemState()),NullRef);
}

_NR<InteractiveObject> Stage::getFocusTarget()
{
	SpinlockLocker l(focusSpinlock);
	if (focus.isNull())
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
	SpinlockLocker l(focusSpinlock);
	focus = f;
}

ASFUNCTIONBODY(Stage,_getFocus)
{
	Stage* th=static_cast<Stage*>(obj);
	_NR<InteractiveObject> focus = th->getFocusTarget();
	if (focus.isNull())
	{
		return NULL;
	}
	else
	{
		focus->incRef();
		return focus.getPtr();
	}
}

ASFUNCTIONBODY(Stage,_setFocus)
{
	Stage* th=static_cast<Stage*>(obj);
	_NR<InteractiveObject> focus;
	ARG_UNPACK(focus);
	th->setFocusTarget(focus);
	return NULL;
}

ASFUNCTIONBODY(Stage,_setTabChildren)
{
	// The specs says that Stage.tabChildren should throw
	// IllegalOperationError, but testing shows that instead of
	// throwing this simply ignores the value.
	return NULL;
}

ASFUNCTIONBODY(Stage,_getFrameRate)
{
	Stage* th=obj->as<Stage>();
	_NR<RootMovieClip> root = th->getRoot();
	if (root.isNull())
		return abstract_d(obj->getSystemState(),obj->getSystemState()->mainClip->getFrameRate());
	else
		return abstract_d(obj->getSystemState(),root->getFrameRate());
}

ASFUNCTIONBODY(Stage,_setFrameRate)
{
	Stage* th=obj->as<Stage>();
	number_t frameRate;
	ARG_UNPACK(frameRate);
	_NR<RootMovieClip> root = th->getRoot();
	if (!root.isNull())
		root->setFrameRate(frameRate);
	return NULL;
}

ASFUNCTIONBODY(Stage,_getAllowFullScreen)
{
	return abstract_b(obj->getSystemState(),false); // until fullscreen support is implemented
}

ASFUNCTIONBODY(Stage,_getAllowFullScreenInteractive)
{
	return abstract_b(obj->getSystemState(),false);
}

ASFUNCTIONBODY(Stage,_getColorCorrectionSupport)
{
	return abstract_b(obj->getSystemState(),false); // until color correction is implemented
}

ASFUNCTIONBODY(Stage,_getWmodeGPU)
{
	return abstract_b(obj->getSystemState(),false);
}
ASFUNCTIONBODY(Stage,_invalidate)
{
	LOG(LOG_NOT_IMPLEMENTED,"invalidate not implemented yet");
	// TODO this crashes lightspark
	//Stage* th=obj->as<Stage>();
	//_R<FlushInvalidationQueueEvent> event=_MR(new (obj->getSystemState()->unaccountedMemory) FlushInvalidationQueueEvent());
	//getVm()->addEvent(_MR(th),event);
	return NULL;
}
ASFUNCTIONBODY(Stage,_getColor)
{
	Stage* th=static_cast<Stage*>(obj);
	RGB rgb;
	_NR<RootMovieClip> root = th->getRoot();
	if (!root.isNull())
		rgb = root->getBackground();
	return abstract_ui(obj->getSystemState(),rgb.toUInt());
}

ASFUNCTIONBODY(Stage,_setColor)
{
	Stage* th=static_cast<Stage*>(obj);
	uint32_t color;
	ARG_UNPACK(color);
	RGB rgb(color);
	_NR<RootMovieClip> root = th->getRoot();
	if (!root.isNull())
		root->setBackground(rgb);
	return NULL;
}


void StageScaleMode::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableByQName("EXACT_FIT","",abstract_s(c->getSystemState(),"exactFit"),CONSTANT_TRAIT);
	c->setVariableByQName("NO_BORDER","",abstract_s(c->getSystemState(),"noBorder"),CONSTANT_TRAIT);
	c->setVariableByQName("NO_SCALE","",abstract_s(c->getSystemState(),"noScale"),CONSTANT_TRAIT);
	c->setVariableByQName("SHOW_ALL","",abstract_s(c->getSystemState(),"showAll"),CONSTANT_TRAIT);
}

void StageAlign::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableByQName("BOTTOM","",abstract_s(c->getSystemState(),"B"),CONSTANT_TRAIT);
	c->setVariableByQName("BOTTOM_LEFT","",abstract_s(c->getSystemState(),"BL"),CONSTANT_TRAIT);
	c->setVariableByQName("BOTTOM_RIGHT","",abstract_s(c->getSystemState(),"BR"),CONSTANT_TRAIT);
	c->setVariableByQName("LEFT","",abstract_s(c->getSystemState(),"L"),CONSTANT_TRAIT);
	c->setVariableByQName("RIGHT","",abstract_s(c->getSystemState(),"R"),CONSTANT_TRAIT);
	c->setVariableByQName("TOP","",abstract_s(c->getSystemState(),"T"),CONSTANT_TRAIT);
	c->setVariableByQName("TOP_LEFT","",abstract_s(c->getSystemState(),"TL"),CONSTANT_TRAIT);
	c->setVariableByQName("TOP_RIGHT","",abstract_s(c->getSystemState(),"TR"),CONSTANT_TRAIT);
}

void StageQuality::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableByQName("BEST","",abstract_s(c->getSystemState(),"best"),CONSTANT_TRAIT);
	c->setVariableByQName("HIGH","",abstract_s(c->getSystemState(),"high"),CONSTANT_TRAIT);
	c->setVariableByQName("LOW","",abstract_s(c->getSystemState(),"low"),CONSTANT_TRAIT);
	c->setVariableByQName("MEDIUM","",abstract_s(c->getSystemState(),"medium"),CONSTANT_TRAIT);
}

void StageDisplayState::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableByQName("FULL_SCREEN","",abstract_s(c->getSystemState(),"fullScreen"),CONSTANT_TRAIT);
	c->setVariableByQName("FULL_SCREEN_INTERACTIVE","",abstract_s(c->getSystemState(),"fullScreenInteractive"),CONSTANT_TRAIT);
	c->setVariableByQName("NORMAL","",abstract_s(c->getSystemState(),"normal"),CONSTANT_TRAIT);
}

Bitmap::Bitmap(Class_base* c, _NR<LoaderInfo> li, std::istream *s, FILE_TYPE type):
	DisplayObject(c),TokenContainer(this),smoothing(false)
{
	if(li)
	{
		loaderInfo = li;
		this->incRef();
		loaderInfo->setWaitedObject(_MR(this));
	}

	bitmapData = _MR(Class<BitmapData>::getInstanceS(c->getSystemState()));
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
			LOG(LOG_NOT_IMPLEMENTED, _("GIFs are not yet supported"));
			break;
		default:
			LOG(LOG_ERROR,_("Unsupported image type"));
			break;
	}
	Bitmap::updatedData();
}

Bitmap::Bitmap(Class_base* c, _R<BitmapData> data) : DisplayObject(c),TokenContainer(this),smoothing(false)
{
	bitmapData = data;
	bitmapData->addUser(this);
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
	smoothing = false;
	return DisplayObject::destruct();
}

void Bitmap::sinit(Class_base* c)
{
	CLASS_SETUP(c, DisplayObject, _constructor, CLASS_SEALED);
	c->isReusable = true;
	REGISTER_GETTER_SETTER(c,bitmapData);
	REGISTER_GETTER_SETTER(c,smoothing);
	REGISTER_GETTER_SETTER(c,pixelSnapping);

}

ASFUNCTIONBODY(Bitmap,_constructor)
{
	tiny_string _pixelSnapping;
	_NR<BitmapData> _bitmapData;
	Bitmap* th = obj->as<Bitmap>();
	ARG_UNPACK(_bitmapData, NullRef)(_pixelSnapping, "auto")(th->smoothing, false);

	DisplayObject::_constructor(obj,NULL,0);

	if(_pixelSnapping!="auto")
		LOG(LOG_NOT_IMPLEMENTED, "Bitmap constructor doesn't support pixelSnapping:"<<_pixelSnapping);
	th->pixelSnapping = _pixelSnapping;

	if(!_bitmapData.isNull())
	{
		th->bitmapData=_bitmapData;
		th->bitmapData->addUser(th);
		th->updatedData();
	}

	return NULL;
}

void Bitmap::onBitmapData(_NR<BitmapData> old)
{
	if(!old.isNull())
		old->removeUser(this);
	if(!bitmapData.isNull())
		bitmapData->addUser(this);
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

ASFUNCTIONBODY_GETTER_SETTER_CB(Bitmap,bitmapData,onBitmapData);
ASFUNCTIONBODY_GETTER_SETTER_CB(Bitmap,smoothing,onSmoothingChanged);
ASFUNCTIONBODY_GETTER_SETTER_CB(Bitmap,pixelSnapping,onPixelSnappingChanged);

void Bitmap::updatedData()
{
	tokens.clear();

	if(bitmapData.isNull() || bitmapData->getBitmapContainer().isNull())
		return;

	FILLSTYLE style(0xff);
	if (smoothing)
		style.FillStyleType=CLIPPED_BITMAP;
	else
		style.FillStyleType=NON_SMOOTHED_CLIPPED_BITMAP;
	style.bitmap=bitmapData->getBitmapContainer();
	tokens.emplace_back(GeomToken(SET_FILL, style));
	tokens.emplace_back(GeomToken(MOVE, Vector2(0, 0)));
	tokens.emplace_back(GeomToken(STRAIGHT, Vector2(0, style.bitmap->getHeight())));
	tokens.emplace_back(GeomToken(STRAIGHT, Vector2(style.bitmap->getWidth(), style.bitmap->getHeight())));
	tokens.emplace_back(GeomToken(STRAIGHT, Vector2(style.bitmap->getWidth(), 0)));
	tokens.emplace_back(GeomToken(STRAIGHT, Vector2(0, 0)));
	if(onStage)
		requestInvalidation(getSystemState());
}
bool Bitmap::boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const
{
	return TokenContainer::boundsRect(xmin,xmax,ymin,ymax);
}

_NR<DisplayObject> Bitmap::hitTestImpl(_NR<DisplayObject> last, number_t x, number_t y, DisplayObject::HIT_TYPE type)
{
	//Simple check inside the area, opacity data should not be considered
	//NOTE: on the X axis the 0th line must be ignored, while the one past the width is valid
	//NOTE: on the Y asix the 0th line is valid, while the one past the width is not
	//NOTE: This is tested behaviour!
	if(!bitmapData.isNull() && x > 0 && x <= bitmapData->getWidth() && y >=0 && y < bitmapData->getHeight())
		return last;
	return NullRef;
}

IntSize Bitmap::getBitmapSize() const
{
	if(bitmapData.isNull())
		return IntSize(0, 0);
	else
		return IntSize(bitmapData->getWidth(), bitmapData->getHeight());
}

void SimpleButton::sinit(Class_base* c)
{
	CLASS_SETUP(c, InteractiveObject, _constructor, CLASS_SEALED);
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

void SimpleButton::buildTraits(ASObject* o)
{
}

_NR<DisplayObject> SimpleButton::hitTestImpl(_NR<DisplayObject> last, number_t x, number_t y, DisplayObject::HIT_TYPE type)
{
	_NR<DisplayObject> ret = NullRef;
	if(hitTestState)
	{
		if(hitTestState->getMatrix().isInvertible())
		{
			number_t localX, localY;
			hitTestState->getMatrix().getInverted().multiply2D(x,y,localX,localY);
			this->incRef();
			ret = hitTestState->hitTest(_MR(this), localX, localY, type);
		}
	}
	/* mouseDown events, for example, are never dispatched to the hitTestState,
	 * but directly to this button (and with event.target = this). This has been
	 * tested with the official flash player. It cannot work otherwise, as
	 * hitTestState->parent == NULL. (This has also been verified)
	 */
	if(ret)
	{
		if(!isHittable(type))
			return NullRef;
			
		this->incRef();
		ret = _MR(this);
	}
	return ret;
}

void SimpleButton::defaultEventBehavior(_R<Event> e)
{
	if(e->type == "mouseDown")
	{
		currentState = DOWN;
		reflectState();
	}
	else if(e->type == "mouseUp")
	{
		currentState = UP;
		reflectState();
	}
	else if(e->type == "mouseOver")
	{
		currentState = OVER;
		reflectState();
	}
	else if(e->type == "mouseOut")
	{
		currentState = UP;
		reflectState();
	}
}

SimpleButton::SimpleButton(Class_base* c, DisplayObject *dS, DisplayObject *hTS,
				DisplayObject *oS, DisplayObject *uS)
	: DisplayObjectContainer(c), downState(dS), hitTestState(hTS), overState(oS), upState(uS),
	  currentState(UP)
{
	/* When called from DefineButton2Tag::instance, they are not constructed yet
	 * TODO: construct them here for once, or each time they become visible?
	 */
	if(dS) dS->initFrame();
	if(hTS) hTS->initFrame();
	if(oS) oS->initFrame();
	if(uS) uS->initFrame();

	tabEnabled = true;
}

void SimpleButton::finalize()
{
	DisplayObjectContainer::finalize();
	downState.reset();
	hitTestState.reset();
	overState.reset();
	upState.reset();
}

ASFUNCTIONBODY(SimpleButton,_constructor)
{
	/* This _must_ not call the DisplayObjectContainer
	 * see note at the class declaration.
	 */
	InteractiveObject::_constructor(obj,NULL,0);
	SimpleButton* th=static_cast<SimpleButton*>(obj);
	_NR<DisplayObject> upState;
	_NR<DisplayObject> overState;
	_NR<DisplayObject> downState;
	_NR<DisplayObject> hitTestState;
	ARG_UNPACK(upState, NullRef)(overState, NullRef)(downState, NullRef)(hitTestState, NullRef);

	if (!upState.isNull())
		th->upState = upState;
	if (!overState.isNull())
		th->overState = overState;
	if (!downState.isNull())
		th->downState = downState;
	if (!hitTestState.isNull())
		th->hitTestState = hitTestState;

	th->reflectState();

	return NULL;
}

void SimpleButton::reflectState()
{
	assert(dynamicDisplayList.empty() || dynamicDisplayList.size() == 1);
	if(!dynamicDisplayList.empty())
		_removeChild(dynamicDisplayList.front());

	if(currentState == UP && !upState.isNull())
		_addChildAt(upState,0);
	else if(currentState == DOWN && !downState.isNull())
		_addChildAt(downState,0);
	else if(currentState == OVER && !overState.isNull())
		_addChildAt(overState,0);
}

ASFUNCTIONBODY(SimpleButton,_getUpState)
{
	SimpleButton* th=static_cast<SimpleButton*>(obj);
	if(!th->upState)
		return obj->getSystemState()->getNullRef();

	th->upState->incRef();
	return th->upState.getPtr();
}

ASFUNCTIONBODY(SimpleButton,_setUpState)
{
	assert_and_throw(argslen == 1);
	SimpleButton* th=static_cast<SimpleButton*>(obj);
	th->upState = _MNR(Class<DisplayObject>::cast(args[0]));
	th->upState->incRef();
	th->reflectState();
	return NULL;
}

ASFUNCTIONBODY(SimpleButton,_getHitTestState)
{
	SimpleButton* th=static_cast<SimpleButton*>(obj);
	if(!th->hitTestState)
		return obj->getSystemState()->getNullRef();

	th->hitTestState->incRef();
	return th->hitTestState.getPtr();
}

ASFUNCTIONBODY(SimpleButton,_setHitTestState)
{
	assert_and_throw(argslen == 1);
	SimpleButton* th=static_cast<SimpleButton*>(obj);
	th->hitTestState = _MNR(Class<DisplayObject>::cast(args[0]));
	th->hitTestState->incRef();
	return NULL;
}

ASFUNCTIONBODY(SimpleButton,_getOverState)
{
	SimpleButton* th=static_cast<SimpleButton*>(obj);
	if(!th->overState)
		return obj->getSystemState()->getNullRef();

	th->overState->incRef();
	return th->overState.getPtr();
}

ASFUNCTIONBODY(SimpleButton,_setOverState)
{
	assert_and_throw(argslen == 1);
	SimpleButton* th=static_cast<SimpleButton*>(obj);
	th->overState = _MNR(Class<DisplayObject>::cast(args[0]));
	th->overState->incRef();
	th->reflectState();
	return NULL;
}

ASFUNCTIONBODY(SimpleButton,_getDownState)
{
	SimpleButton* th=static_cast<SimpleButton*>(obj);
	if(!th->downState)
		return obj->getSystemState()->getNullRef();

	th->downState->incRef();
	return th->downState.getPtr();
}

ASFUNCTIONBODY(SimpleButton,_setDownState)
{
	assert_and_throw(argslen == 1);
	SimpleButton* th=static_cast<SimpleButton*>(obj);
	th->downState = _MNR(Class<DisplayObject>::cast(args[0]));
	th->downState->incRef();
	th->reflectState();
	return NULL;
}

ASFUNCTIONBODY(SimpleButton,_setEnabled)
{
	SimpleButton* th=static_cast<SimpleButton*>(obj);
	assert_and_throw(argslen==1);
	th->enabled=Boolean_concrete(args[0]);
	return NULL;
}

ASFUNCTIONBODY(SimpleButton,_getEnabled)
{
	SimpleButton* th=static_cast<SimpleButton*>(obj);
	return abstract_b(obj->getSystemState(),th->enabled);
}

ASFUNCTIONBODY(SimpleButton,_setUseHandCursor)
{
	SimpleButton* th=static_cast<SimpleButton*>(obj);
	assert_and_throw(argslen==1);
	th->useHandCursor=Boolean_concrete(args[0]);
	return NULL;
}

ASFUNCTIONBODY(SimpleButton,_getUseHandCursor)
{
	SimpleButton* th=static_cast<SimpleButton*>(obj);
	return abstract_b(obj->getSystemState(),th->useHandCursor);
}

void GradientType::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableByQName("LINEAR","",abstract_s(c->getSystemState(),"linear"),CONSTANT_TRAIT);
	c->setVariableByQName("RADIAL","",abstract_s(c->getSystemState(),"radial"),CONSTANT_TRAIT);
}

void BlendMode::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableByQName("ADD","",abstract_s(c->getSystemState(),"add"),CONSTANT_TRAIT);
	c->setVariableByQName("ALPHA","",abstract_s(c->getSystemState(),"alpha"),CONSTANT_TRAIT);
	c->setVariableByQName("DARKEN","",abstract_s(c->getSystemState(),"darken"),CONSTANT_TRAIT);
	c->setVariableByQName("DIFFERENCE","",abstract_s(c->getSystemState(),"difference"),CONSTANT_TRAIT);
	c->setVariableByQName("ERASE","",abstract_s(c->getSystemState(),"erase"),CONSTANT_TRAIT);
	c->setVariableByQName("HARDLIGHT","",abstract_s(c->getSystemState(),"hardlight"),CONSTANT_TRAIT);
	c->setVariableByQName("INVERT","",abstract_s(c->getSystemState(),"invert"),CONSTANT_TRAIT);
	c->setVariableByQName("LAYER","",abstract_s(c->getSystemState(),"layer"),CONSTANT_TRAIT);
	c->setVariableByQName("LIGHTEN","",abstract_s(c->getSystemState(),"lighten"),CONSTANT_TRAIT);
	c->setVariableByQName("MULTIPLY","",abstract_s(c->getSystemState(),"multiply"),CONSTANT_TRAIT);
	c->setVariableByQName("NORMAL","",abstract_s(c->getSystemState(),"normal"),CONSTANT_TRAIT);
	c->setVariableByQName("OVERLAY","",abstract_s(c->getSystemState(),"overlay"),CONSTANT_TRAIT);
	c->setVariableByQName("SCREEN","",abstract_s(c->getSystemState(),"screen"),CONSTANT_TRAIT);
	c->setVariableByQName("SUBTRACT","",abstract_s(c->getSystemState(),"subtract"),CONSTANT_TRAIT);
}

void SpreadMethod::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableByQName("PAD","",abstract_s(c->getSystemState(),"pad"),CONSTANT_TRAIT);
	c->setVariableByQName("REFLECT","",abstract_s(c->getSystemState(),"reflect"),CONSTANT_TRAIT);
	c->setVariableByQName("REPEAT","",abstract_s(c->getSystemState(),"repeat"),CONSTANT_TRAIT);
}

void InterpolationMethod::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableByQName("RGB","",abstract_s(c->getSystemState(),"rgb"),CONSTANT_TRAIT);
	c->setVariableByQName("LINEAR_RGB","",abstract_s(c->getSystemState(),"linearRGB"),CONSTANT_TRAIT);
}

void GraphicsPathCommand::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableByQName("CUBIC_CURVE_TO","",abstract_i(c->getSystemState(),6),CONSTANT_TRAIT);
	c->setVariableByQName("CURVE_TO","",abstract_i(c->getSystemState(),3),CONSTANT_TRAIT);
	c->setVariableByQName("LINE_TO","",abstract_i(c->getSystemState(),2),CONSTANT_TRAIT);
	c->setVariableByQName("MOVE_TO","",abstract_i(c->getSystemState(),1),CONSTANT_TRAIT);
	c->setVariableByQName("NO_OP","",abstract_i(c->getSystemState(),0),CONSTANT_TRAIT);
	c->setVariableByQName("WIDE_LINE_TO","",abstract_i(c->getSystemState(),5),CONSTANT_TRAIT);
	c->setVariableByQName("WIDE_MOVE_TO","",abstract_i(c->getSystemState(),4),CONSTANT_TRAIT);
}

void GraphicsPathWinding::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableByQName("EVEN_ODD","",abstract_s(c->getSystemState(),"evenOdd"),CONSTANT_TRAIT);
	c->setVariableByQName("NON_ZERO","",abstract_s(c->getSystemState(),"nonZero"),CONSTANT_TRAIT);
}

void PixelSnapping::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableByQName("ALWAYS","",abstract_s(c->getSystemState(),"always"),CONSTANT_TRAIT);
	c->setVariableByQName("AUTO","",abstract_s(c->getSystemState(),"auto"),CONSTANT_TRAIT);
	c->setVariableByQName("NEVER","",abstract_s(c->getSystemState(),"never"),CONSTANT_TRAIT);

}

/* Go through the hierarchy and add all
 * legacy objects which are new in the current
 * frame top-down. At the same time, call their
 * constructors in reverse order (bottom-up).
 * This is called in vm's thread context */
void DisplayObjectContainer::initFrame()
{
	/* init the frames and call constructors of our children first */
	auto it=dynamicDisplayList.begin();
	for(;it!=dynamicDisplayList.end();it++)
		(*it)->initFrame();
	/* call our own constructor, if necassary */
	DisplayObject::initFrame();
}

/* Go through the hierarchy and add all
 * legacy objects which are new in the current
 * frame top-down. At the same time, call their
 * constructors in reverse order (bottom-up).
 * This is called in vm's thread context */
void MovieClip::initFrame()
{
	/* Go through the list of frames.
	 * If our next_FP is after our current,
	 * we construct all frames from current
	 * to next_FP.
	 * If our next_FP is before our current,
	 * we purge all objects on the 0th frame
	 * and then construct all frames from
	 * the 0th to the next_FP.
	 * TODO: do not purge legacy objects that were also there at state.FP,
	 * we saw that their constructor is not run again.
	 * We also will run the constructor on objects that got placed and deleted
	 * before state.FP (which may get us an segfault).
	 *
	 */
	if((int)state.FP < state.last_FP)
		purgeLegacyChildren();

	//Declared traits must exists before legacy objects are added
	if (getClass())
		getClass()->setupDeclaredTraits(this);

	if(getFramesLoaded())
	{
		std::list<Frame>::iterator iter=frames.begin();
		for(uint32_t i=0;i<=state.FP;i++)
		{
			if((int)state.FP < state.last_FP || (int)i > state.last_FP)
			{
				this->incRef(); //TODO kill ref from execute's declaration
				iter->execute(_MR(this));
			}
			++iter;
		}
	}

	/* Now the new legacy display objects are there, so we can also init their
	 * first frame (top-down) and call their constructors (bottom-up) */
	auto it=dynamicDisplayList.begin();
	for(;it!=dynamicDisplayList.end();it++)
		(*it)->initFrame();

	/* Set last_FP to reflect the frame that we have initialized currently.
	 * This must be set before the constructor of this MovieClip is run,
	 * or it will call initFrame(). */
	bool newFrame = (int)state.FP != state.last_FP;
	state.last_FP=state.FP;

	/* call our own constructor, if necassary */
	DisplayObject::initFrame();

	/* Run framescripts if this is a new frame. We do it at the end because our constructor
	 * may just have registered one. */
	//TODO: check order: child or parent first?
	if(newFrame && frameScripts.count(state.FP))
	{
		ASObject *v=frameScripts[state.FP]->call(NULL,NULL,0);
		if(v)
			v->decRef();
	}

}

/* This is run in vm's thread context */
void DisplayObjectContainer::advanceFrame()
{
	list<_R<DisplayObject>>::const_iterator it=dynamicDisplayList.begin();
	for(;it!=dynamicDisplayList.end();++it)
		(*it)->advanceFrame();
}

/* Update state.last_FP. If enough frames
 * are available, set state.FP to state.next_FP.
 * This is run in vm's thread context.
 */
void MovieClip::advanceFrame()
{
	//TODO check order: child or parent first?
	DisplayObjectContainer::advanceFrame();

	/* A MovieClip can only have frames if
	 * 1a. It is a RootMovieClip
	 * 1b. or it is a DefineSpriteTag
	 * 2. and is exported as a subclass of MovieClip (see bindedTo)
	 */
	if((!dynamic_cast<RootMovieClip*>(this) && !fromDefineSpriteTag)
	   || !getClass()->isSubClass(Class<MovieClip>::getClass(getSystemState())))
		return;

	//If we have not yet loaded enough frames delay advancement
	if(state.next_FP>=(uint32_t)getFramesLoaded())
	{
		if(hasFinishedLoading())
		{
			LOG(LOG_ERROR,_("state.next_FP >= getFramesLoaded"));
			state.next_FP = state.FP;
		}
		return;
	}

	state.FP=state.next_FP;
	state.explicit_FP=false;
	if(!state.stop_FP && getFramesLoaded()>0)
	{
		state.next_FP=imin(state.FP+1,getFramesLoaded()-1);
		if(hasFinishedLoading() && state.FP == getFramesLoaded()-1)
			state.next_FP = 0;
	}
}

void MovieClip::constructionComplete()
{
	DisplayObject::constructionComplete();

	/* If this object was 'new'ed from AS code, the first
	 * frame has not been initalized yet, so init the frame
	 * now */
	if(state.last_FP == -1)
		initFrame();
}

void AVM1Movie::sinit(Class_base* c)
{
	CLASS_SETUP(c, DisplayObject, _constructor, CLASS_SEALED);
}

void AVM1Movie::buildTraits(ASObject* o)
{
	//No traits
}

ASFUNCTIONBODY(AVM1Movie,_constructor)
{
	DisplayObject::_constructor(obj,NULL,0);
	return NULL;
}

void Shader::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED);
}

ASFUNCTIONBODY(Shader,_constructor)
{
	LOG(LOG_NOT_IMPLEMENTED, _("Shader class is unimplemented."));
	return NULL;
}

void BitmapDataChannel::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableByQName("ALPHA","",abstract_ui(c->getSystemState(),8),CONSTANT_TRAIT);
	c->setVariableByQName("BLUE","",abstract_ui(c->getSystemState(),4),CONSTANT_TRAIT);
	c->setVariableByQName("GREEN","",abstract_ui(c->getSystemState(),2),CONSTANT_TRAIT);
	c->setVariableByQName("RED","",abstract_ui(c->getSystemState(),1),CONSTANT_TRAIT);
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
	c->setVariableByQName("HORIZONTAL","",abstract_s(c->getSystemState(),"horizontal"),CONSTANT_TRAIT);
	c->setVariableByQName("NONE","",abstract_s(c->getSystemState(),"none"),CONSTANT_TRAIT);
	c->setVariableByQName("NORMAL","",abstract_s(c->getSystemState(),"normal"),CONSTANT_TRAIT);
	c->setVariableByQName("VERTICAL","",abstract_s(c->getSystemState(),"vertical"),CONSTANT_TRAIT);
}
