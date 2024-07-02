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

#include "backends/security.h"
#include "parsing/streams.h"
#include "scripting/abc.h"
#include "scripting/argconv.h"
#include "scripting/class.h"
#include "scripting/flash/display/Loader.h"
#include "scripting/flash/display/LoaderInfo.h"
#include "scripting/flash/display/RootMovieClip.h"
#include "scripting/flash/net/flashnet.h"

using namespace std;
using namespace lightspark;

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
	DisplayObject* res = obj.getPtr();
	if (loader.getPtr() && res && (!res->is<RootMovieClip>() || res->as<RootMovieClip>()->hasFinishedLoading()))
	{
		if (res != loader->getSystemState()->mainClip)
		{
			res->incRef();
			getVm(loader->getSystemState())->addBufferEvent(NullRef,_MR(new (loader->getSystemState()->unaccountedMemory) SetLoaderContentEvent(_MR(res), loader)));
			getVm(loader->getSystemState())->addEvent(NullRef, _MR(new (loader->getSystemState()->unaccountedMemory) FlushEventBufferEvent(false,true)));
		}
	}
}

ASFUNCTIONBODY_ATOM(Loader,_constructor)
{
	Loader* th=asAtomHandler::as<Loader>(obj);
	DisplayObjectContainer::_constructor(ret,wrk,obj,nullptr,0);
	if (!th->contentLoaderInfo)
	{
		th->contentLoaderInfo=Class<LoaderInfo>::getInstanceS(wrk,th);
		th->contentLoaderInfo->addStoredMember();
	}
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
	ret = asAtomHandler::fromObject(th->contentLoaderInfo);
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
void Loader::loadIntern(URLRequest* r, LoaderContext* context, DisplayObject* _avm1target)
{
	if (_avm1target != nullptr)
	{
		_avm1target->incRef();
		this->avm1target=_MR(_avm1target);
	}
	if (!contentLoaderInfo)
	{
		contentLoaderInfo=Class<LoaderInfo>::getInstanceS(getInstanceWorker(),this);
		contentLoaderInfo->addStoredMember();
	}
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
	RootMovieClip* currentRoot=this->loadedFrom ? this->loadedFrom : getInstanceWorker()->rootClip.getPtr();
	// empty origin is possible if swf is loaded by loadBytes()
	if(currentRoot->getOrigin().isEmpty() || currentRoot->getOrigin().getHostname()==this->url.getHostname() || secDomain)
	{
		//Same domain
		_NR<ApplicationDomain> parentDomain;
		if (getInstanceWorker()->currentCallContext)
			parentDomain = ABCVm::getCurrentApplicationDomain(getInstanceWorker()->currentCallContext);
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
	if (getInstanceWorker()->currentCallContext && getInstanceWorker()->currentCallContext->exceptionthrown)
		return;

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
		if (getVm(getSystemState())->addEvent(getContentLoaderInfo(),_MR(ev)))
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
	if (contentLoaderInfo)
		contentLoaderInfo->removeStoredMember();
	contentLoaderInfo=nullptr;
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
	if (contentLoaderInfo)
		contentLoaderInfo->removeStoredMember();
	contentLoaderInfo=nullptr;
	avm1target.reset();
	uncaughtErrorEvents.reset();
	return DisplayObjectContainer::destruct();
}

bool Loader::countCylicMemberReferences(garbagecollectorstate& gcstate)
{
	if (gcstate.checkAncestors(this))
		return false;
	bool ret = DisplayObjectContainer::countCylicMemberReferences(gcstate);
	if (contentLoaderInfo)
		ret = contentLoaderInfo->countAllCylicMemberReferences(gcstate) || ret;
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
Loader::Loader(ASWorker* wrk, Class_base* c):DisplayObjectContainer(wrk,c),content(nullptr),contentLoaderInfo(nullptr),loaded(false), allowCodeImport(true),uncaughtErrorEvents(NullRef)
{
	subtype=SUBTYPE_LOADER;
	contentLoaderInfo=Class<LoaderInfo>::getInstanceS(wrk,this);
	contentLoaderInfo->addStoredMember();
}

Loader::~Loader()
{
}

void Loader::sinit(Class_base* c)
{
	CLASS_SETUP(c, DisplayObjectContainer, _constructor, CLASS_SEALED);
	c->isReusable=true;
	c->setDeclaredMethodByQName("contentLoaderInfo","",c->getSystemState()->getBuiltinFunction(_getContentLoaderInfo,0,Class<LoaderInfo>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("content","",c->getSystemState()->getBuiltinFunction(_getContent,0,Class<DisplayObject>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("close","",c->getSystemState()->getBuiltinFunction(close),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("loadBytes","",c->getSystemState()->getBuiltinFunction(loadBytes),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("load","",c->getSystemState()->getBuiltinFunction(load),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("unload","",c->getSystemState()->getBuiltinFunction(_unload),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("unloadAndStop","",c->getSystemState()->getBuiltinFunction(_unloadAndStop),NORMAL_METHOD,true);
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
	{
		Locker l(mutexDisplayList);
		clearDisplayList();
	}

	{
		Locker l(spinlock);
		if (o->is<RootMovieClip>() && o != getSystemState()->mainClip && !o->as<RootMovieClip>()->usesActionScript3)
		{
			AVM1Movie* m = Class<AVM1Movie>::getInstanceS(getInstanceWorker());
			o->incRef();
			m->_addChildAt(o,0);
			m->addStoredMember();
			content = m;
		}
		else
		{
			o->incRef();
			o->addStoredMember();
			content=o;
		}
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
		content->incRef();
		_addChildAt(content, 0);
	}
	if (!o->loaderInfo.isNull())
		o->loaderInfo->setComplete();
}

_NR<LoaderInfo> Loader::getContentLoaderInfo() 
{
	contentLoaderInfo->incRef();
	return _MNR(contentLoaderInfo);
}
