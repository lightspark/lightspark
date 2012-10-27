/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2012  Alessandro Pignotti (a.pignotti@sssup.it)

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
#include "scripting/flash/display/flashdisplay.h"
#include "swf.h"
#include "scripting/flash/geom/flashgeom.h"
#include "scripting/flash/system/flashsystem.h"
#include "parsing/streams.h"
#include "compat.h"
#include "scripting/class.h"
#include "backends/rendering.h"
#include "backends/geometry.h"
#include "scripting/flash/accessibility/flashaccessibility.h"
#include "scripting/flash/display/BitmapData.h"
#include "scripting/argconv.h"
#include "scripting/toplevel/Vector.h"
#include "backends/security.h"

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
	loader(NullRef),loadStatus(STARTED),actionScriptVersion(3),childAllowsParent(true)
{
}

LoaderInfo::LoaderInfo(Class_base* c, _R<Loader> l):EventDispatcher(c),applicationDomain(NullRef),securityDomain(NullRef),
	contentType("application/x-shockwave-flash"),
	bytesLoaded(0),bytesTotal(0),sharedEvents(NullRef),
	loader(l),loadStatus(STARTED),actionScriptVersion(3),childAllowsParent(true)
{
}

void LoaderInfo::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<EventDispatcher>::getRef());
	c->setDeclaredMethodByQName("loaderURL","",Class<IFunction>::getFunction(_getLoaderURL),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("loader","",Class<IFunction>::getFunction(_getLoader),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("content","",Class<IFunction>::getFunction(_getContent),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("url","",Class<IFunction>::getFunction(_getURL),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("bytesLoaded","",Class<IFunction>::getFunction(_getBytesLoaded),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("bytesTotal","",Class<IFunction>::getFunction(_getBytesTotal),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("applicationDomain","",Class<IFunction>::getFunction(_getApplicationDomain),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("sharedEvents","",Class<IFunction>::getFunction(_getSharedEvents),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("width","",Class<IFunction>::getFunction(_getWidth),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("height","",Class<IFunction>::getFunction(_getHeight),GETTER_METHOD,true);
	REGISTER_GETTER(c,parameters);
	REGISTER_GETTER(c,actionScriptVersion);
	REGISTER_GETTER(c,childAllowsParent);
	REGISTER_GETTER(c,contentType);
}

ASFUNCTIONBODY_GETTER(LoaderInfo,parameters);
ASFUNCTIONBODY_GETTER(LoaderInfo,actionScriptVersion);
ASFUNCTIONBODY_GETTER(LoaderInfo,childAllowsParent);
ASFUNCTIONBODY_GETTER(LoaderInfo,contentType);

void LoaderInfo::buildTraits(ASObject* o)
{
}

void LoaderInfo::finalize()
{
	EventDispatcher::finalize();
	sharedEvents.reset();
	loader.reset();
	applicationDomain.reset();
	securityDomain.reset();
	waitedObject.reset();
}

void LoaderInfo::resetState()
{
	SpinlockLocker l(spinlock);
	bytesLoaded=0;
	bytesTotal=0;
	loadStatus=STARTED;
}

void LoaderInfo::setBytesLoaded(uint32_t b)
{
	if(b!=bytesLoaded)
	{
		SpinlockLocker l(spinlock);
		bytesLoaded=b;
		if(isVmThread())
		{
			this->incRef();
			getVm()->addEvent(_MR(this),_MR(Class<ProgressEvent>::getInstanceS(bytesLoaded,bytesTotal)));
		}
		if(loadStatus==INIT_SENT)
		{
			//The clip is also complete now
			this->incRef();
			getVm()->addEvent(_MR(this),_MR(Class<Event>::getInstanceS("complete")));
			loadStatus=COMPLETE;
		}
	}
}

void LoaderInfo::sendInit()
{
	this->incRef();
	getVm()->addEvent(_MR(this),_MR(Class<Event>::getInstanceS("init")));
	assert(loadStatus==STARTED);
	loadStatus=INIT_SENT;
	if(bytesTotal && bytesLoaded==bytesTotal)
	{
		//The clip is also complete now
		this->incRef();
		getVm()->addEvent(_MR(this),_MR(Class<Event>::getInstanceS("complete")));
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
		parameters = _MR(Class<ASObject>::getInstanceS());
		SystemState::parseParametersFromURLIntoObject(url, parameters);
	}
}

ASFUNCTIONBODY(LoaderInfo,_constructor)
{
	LoaderInfo* th=static_cast<LoaderInfo*>(obj);
	EventDispatcher::_constructor(obj,NULL,0);
	th->sharedEvents=_MR(Class<EventDispatcher>::getInstanceS());
	th->parameters = _MR(Class<ASObject>::getInstanceS());
	return NULL;
}

ASFUNCTIONBODY(LoaderInfo,_getLoaderURL)
{
	LoaderInfo* th=static_cast<LoaderInfo*>(obj);
	return Class<ASString>::getInstanceS(th->loaderURL);
}

ASFUNCTIONBODY(LoaderInfo,_getContent)
{
	//Use Loader::getContent
	LoaderInfo* th=static_cast<LoaderInfo*>(obj);
	if(th->loader.isNull())
		return getSys()->getUndefinedRef();
	else
		return Loader::_getContent(th->loader.getPtr(),NULL,0);
}

ASFUNCTIONBODY(LoaderInfo,_getLoader)
{
	LoaderInfo* th=static_cast<LoaderInfo*>(obj);
	if(th->loader.isNull())
		return getSys()->getUndefinedRef();
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
	return Class<ASString>::getInstanceS(th->url);
}

ASFUNCTIONBODY(LoaderInfo,_getBytesLoaded)
{
	LoaderInfo* th=static_cast<LoaderInfo*>(obj);
	return abstract_i(th->bytesLoaded);
}

ASFUNCTIONBODY(LoaderInfo,_getBytesTotal)
{
	LoaderInfo* th=static_cast<LoaderInfo*>(obj);
	return abstract_i(th->bytesTotal);
}

ASFUNCTIONBODY(LoaderInfo,_getApplicationDomain)
{
	LoaderInfo* th=static_cast<LoaderInfo*>(obj);
	if(th->applicationDomain.isNull())
		return getSys()->getNullRef();

	th->applicationDomain->incRef();
	return th->applicationDomain.getPtr();
}

ASFUNCTIONBODY(LoaderInfo,_getWidth)
{
	LoaderInfo* th=static_cast<LoaderInfo*>(obj);
	_NR<Loader> l = th->loader;
	if(l.isNull())
		return abstract_d(0);
	_NR<DisplayObject> o=l->getContent();
	if (o.isNull())
		return abstract_d(0);

	return abstract_d(o->getNominalWidth());
}

ASFUNCTIONBODY(LoaderInfo,_getHeight)
{
	LoaderInfo* th=static_cast<LoaderInfo*>(obj);
	_NR<Loader> l = th->loader;
	if(l.isNull())
		return abstract_d(0);
	_NR<DisplayObject> o=l->getContent();
	if (o.isNull())
		return abstract_d(0);

	return abstract_d(o->getNominalHeight());
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
		if(!createDownloader(false, loaderInfo, loaderInfo.getPtr(), false))
			return;

		downloader->waitForData(); //Wait for some data, making sure our check for failure is working
		if(downloader->hasFailed()) //Check to see if the download failed for some reason
		{
			LOG(LOG_ERROR, "Loader::execute(): Download of URL failed: " << url);
			getVm()->addEvent(loaderInfo,_MR(Class<IOErrorEvent>::getInstanceS()));
			// downloader will be deleted in jobFence
			return;
		}
		getVm()->addEvent(loaderInfo,_MR(Class<Event>::getInstanceS("open")));
		sbuf=downloader;
	}
	else if(source==BYTES)
	{
		assert_and_throw(bytes->bytes);

		loaderInfo->setBytesTotal(bytes->getLength());
		loaderInfo->setBytesLoaded(bytes->getLength());

		bytes_buf bb(bytes->bytes,bytes->getLength());
		sbuf=&bb;
	}

	istream s(sbuf);
	ParseThread local_pt(s,loaderInfo->applicationDomain,loaderInfo->securityDomain,loader.getPtr(),url.getParsedURL());
	local_pt.execute();

	{
		//Acquire the lock to ensure consistency in threadAbort
		SpinlockLocker l(downloaderLock);
		if(downloader)
			getSys()->downloadManager->destroy(downloader);
		downloader=NULL;
	}
	bytes.reset();

	_NR<DisplayObject> obj=local_pt.getParsedObject();
	if(obj.isNull())
	{
		// The stream did not contain RootMovieClip or Bitmap
		if(!threadAborting)
			getVm()->addEvent(loaderInfo,_MR(Class<IOErrorEvent>::getInstanceS()));
		return;
	}
}

ASFUNCTIONBODY(Loader,_constructor)
{
	Loader* th=static_cast<Loader*>(obj);
	DisplayObjectContainer::_constructor(obj,NULL,0);
	th->contentLoaderInfo->setLoaderURL(getSys()->mainClip->getOrigin().getParsedURL());
	return NULL;
}

ASFUNCTIONBODY(Loader,_getContent)
{
	Loader* th=static_cast<Loader*>(obj);
	SpinlockLocker l(th->spinlock);
	_NR<ASObject> ret=th->content;
	if(ret.isNull())
		ret=_MR(getSys()->getUndefinedRef());

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
	if(th->job)
		th->job->threadAbort();

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
	_NR<SecurityDomain> curSecDomain=ABCVm::getCurrentSecurityDomain(getVm()->currentCallContext);
	if(!context.isNull() && !context->securityDomain.isNull())
	{
		//The passed domain must be the current one. See Loader::load specs.
		if(context->securityDomain!=curSecDomain)
			throw Class<SecurityError>::getInstanceS("SecurityError: securityDomain must be current one");
		secDomain=curSecDomain;
	}
	//Default is to create a child ApplicationDomain if the file is in the same security context
	//otherwise create a child of the system domain. If the security domain is different
	//the passed applicationDomain is ignored
	_R<RootMovieClip> currentRoot=getVm()->currentCallContext->context->root;
	if(currentRoot->getOrigin().getHostname()==th->url.getHostname() || !secDomain.isNull())
	{
		//Same domain
		_NR<ApplicationDomain> parentDomain = currentRoot->applicationDomain;
		//Support for LoaderContext
		if(context.isNull() || context->applicationDomain.isNull())
			th->contentLoaderInfo->applicationDomain = _MR(Class<ApplicationDomain>::getInstanceS(parentDomain));
		else
			th->contentLoaderInfo->applicationDomain = context->applicationDomain;
		th->contentLoaderInfo->securityDomain = curSecDomain;
	}
	else
	{
		//Different domain
		_NR<ApplicationDomain> parentDomain =  getSys()->systemDomain;
		th->contentLoaderInfo->applicationDomain = _MR(Class<ApplicationDomain>::getInstanceS(parentDomain));
		th->contentLoaderInfo->securityDomain = _MR(Class<SecurityDomain>::getInstanceS());
	}

	if(!th->url.isValid())
	{
		//Notify an error during loading
		th->incRef();
		getSys()->currentVm->addEvent(_MR(th),_MR(Class<IOErrorEvent>::getInstanceS()));
		return NULL;
	}

	SecurityManager::checkURLStaticAndThrow(th->url, ~(SecurityManager::LOCAL_WITH_FILE),
		SecurityManager::LOCAL_WITH_FILE | SecurityManager::LOCAL_TRUSTED, true);

	th->incRef();
	r->incRef();
	LoaderThread *thread=new LoaderThread(_MR(r), _MR(th));
	getSys()->addJob(thread);
	th->job=thread;
	return NULL;
}

ASFUNCTIONBODY(Loader,loadBytes)
{
	Loader* th=static_cast<Loader*>(obj);

	th->unload();

	_NR<ByteArray> bytes;
	_NR<LoaderContext> context;
	ARG_UNPACK (bytes)(context, NullRef);

	_NR<ApplicationDomain> parentDomain = ABCVm::getCurrentApplicationDomain(getVm()->currentCallContext);
	if(context.isNull() || context->applicationDomain.isNull())
		th->contentLoaderInfo->applicationDomain = _MR(Class<ApplicationDomain>::getInstanceS(parentDomain));
	else
		th->contentLoaderInfo->applicationDomain = context->applicationDomain;
	//Always loaded in the current security domain
	_NR<SecurityDomain> curSecDomain=ABCVm::getCurrentSecurityDomain(getVm()->currentCallContext);
	th->contentLoaderInfo->securityDomain = curSecDomain;

	if(bytes->getLength()!=0)
	{
		th->incRef();
		LoaderThread *thread=new LoaderThread(_MR(bytes), _MR(th));
		getSys()->addJob(thread);
		th->job=thread;
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

void Loader::unload()
{
	_NR<DisplayObject> content_copy = NullRef;
	{
		SpinlockLocker l(spinlock);
		if(job)
			job->threadAbort();

		content_copy=content;
		content.reset();
	}
	
	if(loaded)
	{
		getVm()->addEvent(contentLoaderInfo,_MR(Class<Event>::getInstanceS("unload")));
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

Loader::Loader(Class_base* c):DisplayObjectContainer(c),content(NullRef),job(NULL),contentLoaderInfo(NullRef),loaded(false)
{
	incRef();
	contentLoaderInfo=_MR(Class<LoaderInfo>::getInstanceS(_MR(this)));
}

Loader::~Loader()
{
}

void Loader::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<DisplayObjectContainer>::getRef());
	c->setDeclaredMethodByQName("contentLoaderInfo","",Class<IFunction>::getFunction(_getContentLoaderInfo),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("content","",Class<IFunction>::getFunction(_getContent),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("close","",Class<IFunction>::getFunction(close),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("loadBytes","",Class<IFunction>::getFunction(loadBytes),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("load","",Class<IFunction>::getFunction(load),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("unload","",Class<IFunction>::getFunction(_unload),NORMAL_METHOD,true);
}

void Loader::threadFinished(IThreadJob* finishedJob)
{
	// If this is the current job, we are done. If these are not
	// equal, finishedJob is a job that was cancelled when load()
	// was called again, and we have to still wait for the correct
	// job.
	SpinlockLocker l(spinlock);
	if(finishedJob==job)
		job=NULL;

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
		loaded=true;
	}

	// _addChild may cause AS code to run, release locks beforehand.
	_addChildAt(o, 0);
}

Sprite::Sprite(Class_base* c):DisplayObjectContainer(c),TokenContainer(this),graphics(NullRef)
{
}

void Sprite::finalize()
{
	DisplayObjectContainer::finalize();
	graphics.reset();
}

void Sprite::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<DisplayObjectContainer>::getRef());
	c->setDeclaredMethodByQName("graphics","",Class<IFunction>::getFunction(_getGraphics),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("startDrag","",Class<IFunction>::getFunction(_startDrag),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("stopDrag","",Class<IFunction>::getFunction(_stopDrag),NORMAL_METHOD,true);
}

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
			throw Class<ArgumentError>::getInstanceS("Wrong type");
		bounds = new RECT(rect->getRect());
	}

	Vector2f offset;
	if(!lockCenter)
	{
		offset = -th->getParent()->getLocalMousePos();
		offset += th->getXY();
	}

	th->incRef();
	getSys()->getInputThread()->startDrag(_MR(th), bounds, offset);
	return NULL;
}

ASFUNCTIONBODY(Sprite,_stopDrag)
{
	Sprite* th=Class<Sprite>::cast(obj);
	getSys()->getInputThread()->stopDrag(th);
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
_NR<InteractiveObject> DisplayObjectContainer::hitTestImpl(_NR<InteractiveObject> last, number_t x, number_t y, DisplayObject::HIT_TYPE type)
{
	_NR<InteractiveObject> ret = NullRef;
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

_NR<InteractiveObject> Sprite::hitTestImpl(_NR<InteractiveObject>, number_t x, number_t y, DisplayObject::HIT_TYPE type)
{
	_NR<InteractiveObject> ret = NullRef;
	this->incRef();
	ret = DisplayObjectContainer::hitTestImpl(_MR(this),x,y, type);
	if(!ret && isHittable(type))
	{
		//The coordinates are locals
		this->incRef();
		return TokenContainer::hitTestImpl(_MR(this),x,y, type);
	}
	else
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
		th->graphics=_MR(Class<Graphics>::getInstanceS(th));

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
	c->setConstructor(NULL);
	c->setSuper(Class<ASObject>::getRef());
	c->setDeclaredMethodByQName("frame","",Class<IFunction>::getFunction(_getFrame),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("name","",Class<IFunction>::getFunction(_getName),GETTER_METHOD,true);
}

void FrameLabel::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(FrameLabel,_getFrame)
{
	FrameLabel* th=static_cast<FrameLabel*>(obj);
	return abstract_i(th->frame);
}

ASFUNCTIONBODY(FrameLabel,_getName)
{
	FrameLabel* th=static_cast<FrameLabel*>(obj);
	return Class<ASString>::getInstanceS(th->name);
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
			assert_and_throw(fl.name == label);
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
	c->setConstructor(NULL);
	c->setSuper(Class<ASObject>::getRef());
	c->setDeclaredMethodByQName("labels","",Class<IFunction>::getFunction(_getLabels),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("name","",Class<IFunction>::getFunction(_getName),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("numFrames","",Class<IFunction>::getFunction(_getNumFrames),GETTER_METHOD,true);
}

ASFUNCTIONBODY(Scene,_getLabels)
{
	Scene* th=static_cast<Scene*>(obj);
	Array* ret = Class<Array>::getInstanceS();
	ret->resize(th->labels.size());
	for(size_t i=0; i<th->labels.size(); ++i)
	{
		ret->set(i, _MR(Class<FrameLabel>::getInstanceS(th->labels[i])));
	}
	return ret;
}

ASFUNCTIONBODY(Scene,_getName)
{
	Scene* th=static_cast<Scene*>(obj);
	return Class<ASString>::getInstanceS(th->name);
}

ASFUNCTIONBODY(Scene,_getNumFrames)
{
	Scene* th=static_cast<Scene*>(obj);
	return abstract_i(th->numFrames);
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

void Frame::bindClasses(RootMovieClip *root)
{
	ABCVm *vm = getVm();
	auto it=classesToBeBound.begin();
	for(;it!=classesToBeBound.end();++it)
		vm->buildClassAndBindTag(it->first.raw_buf(), it->second);

	classesToBeBound.clear();
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
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<Sprite>::getRef());
	c->setDeclaredMethodByQName("currentFrame","",Class<IFunction>::getFunction(_getCurrentFrame),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("totalFrames","",Class<IFunction>::getFunction(_getTotalFrames),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("framesLoaded","",Class<IFunction>::getFunction(_getFramesLoaded),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("currentFrameLabel","",Class<IFunction>::getFunction(_getCurrentFrameLabel),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("currentLabel","",Class<IFunction>::getFunction(_getCurrentLabel),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("currentLabels","",Class<IFunction>::getFunction(_getCurrentLabels),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("scenes","",Class<IFunction>::getFunction(_getScenes),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("currentScene","",Class<IFunction>::getFunction(_getCurrentScene),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("stop","",Class<IFunction>::getFunction(stop),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("gotoAndStop","",Class<IFunction>::getFunction(gotoAndStop),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("gotoAndPlay","",Class<IFunction>::getFunction(gotoAndPlay),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("nextFrame","",Class<IFunction>::getFunction(nextFrame),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("addFrameScript","",Class<IFunction>::getFunction(addFrameScript),NORMAL_METHOD,true);
}

void MovieClip::buildTraits(ASObject* o)
{
}

MovieClip::MovieClip(Class_base* c):Sprite(c),totalFrames_unreliable(1)
{
}

MovieClip::MovieClip(Class_base* c, const FrameContainer& f):Sprite(c),FrameContainer(f),totalFrames_unreliable(frames.size())
{
	//For sprites totalFrames_unreliable is the actual frame count
	//For the root movie, it's the frame count from the header
}

void MovieClip::finalize()
{
	Sprite::finalize();
	frames.clear();
	frameScripts.clear();
}

uint32_t MovieClip::getFrameIdByLabel(const tiny_string& l) const
{
	for(size_t i=0;i<scenes.size();++i)
	{
		for(size_t j=0;j<scenes[i].labels.size();++j)
			if(scenes[i].labels[j].name == l)
				return scenes[i].labels[j].frame;
	}
	return 0xffffffff;
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
	return NULL;
}

ASObject* MovieClip::gotoAnd(ASObject* const* args, const unsigned int argslen, bool stop)
{
	uint32_t next_FP;
	assert_and_throw(argslen==1 || argslen==2);
	if(argslen==2)
	{
		LOG(LOG_NOT_IMPLEMENTED,"MovieClip.gotoAndStop/Play with two args is not supported yet");
		return NULL;
	}
	if(args[0]->getObjectType()==T_STRING)
	{
		uint32_t dest=getFrameIdByLabel(args[0]->toString());
		if(dest==0xffffffff)
			throw Class<ArgumentError>::getInstanceS("gotoAndPlay/Stop: label not found");

		next_FP = dest;
	}
	else
	{
		uint32_t inFrameNo = args[0]->toInt();
		if(inFrameNo == 0)
			return NULL; /*this behavior was observed by testing */
		/* "the current scene determines the global frame number" */
		next_FP = scenes[getCurrentScene()].startframe + inFrameNo - 1;
		if(next_FP >= getFramesLoaded())
		{
			LOG(LOG_ERROR, next_FP << "= next_FP >= state.max_FP = " << getFramesLoaded());
			/* spec says we should throw an error, but then YT breaks */
			//throw Class<ArgumentError>::getInstanceS("gotoAndPlay/Stop: frame not found");
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
	return NULL;
}

ASFUNCTIONBODY(MovieClip,_getFramesLoaded)
{
	MovieClip* th=static_cast<MovieClip*>(obj);
	return abstract_i(th->getFramesLoaded());
}

ASFUNCTIONBODY(MovieClip,_getTotalFrames)
{
	MovieClip* th=static_cast<MovieClip*>(obj);
	return abstract_i(th->totalFrames_unreliable);
}

ASFUNCTIONBODY(MovieClip,_getScenes)
{
	MovieClip* th=static_cast<MovieClip*>(obj);
	Array* ret = Class<Array>::getInstanceS();
	ret->resize(th->scenes.size());
	uint32_t numFrames;
	for(size_t i=0; i<th->scenes.size(); ++i)
	{
		if(i == th->scenes.size()-1)
			numFrames = th->totalFrames_unreliable - th->scenes[i].startframe;
		else
			numFrames = th->scenes[i].startframe - th->scenes[i+1].startframe;
		ret->set(i, _MR(Class<Scene>::getInstanceS(th->scenes[i],numFrames)));
	}
	return ret;
}

uint32_t MovieClip::getCurrentScene()
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

	return Class<Scene>::getInstanceS(th->scenes[curScene],numFrames);
}

ASFUNCTIONBODY(MovieClip,_getCurrentFrame)
{
	MovieClip* th=static_cast<MovieClip*>(obj);
	//currentFrame is 1-based and relative to current scene
	return abstract_i(th->state.FP+1 - th->scenes[th->getCurrentScene()].startframe);
}

ASFUNCTIONBODY(MovieClip,_getCurrentFrameLabel)
{
	MovieClip* th=static_cast<MovieClip*>(obj);
	for(size_t i=0;i<th->scenes.size();++i)
	{
		for(size_t j=0;j<th->scenes[i].labels.size();++j)
			if(th->scenes[i].labels[j].frame == th->state.FP)
				return Class<ASString>::getInstanceS(th->scenes[i].labels[j].name);
	}
	return getSys()->getNullRef();
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
		return getSys()->getNullRef();
	else
		return Class<ASString>::getInstanceS(label);
}

ASFUNCTIONBODY(MovieClip,_getCurrentLabels)
{
	MovieClip* th=static_cast<MovieClip*>(obj);
	Scene_data& sc = th->scenes[th->getCurrentScene()];

	Array* ret = Class<Array>::getInstanceS();
	ret->resize(sc.labels.size());
	for(size_t i=0; i<sc.labels.size(); ++i)
	{
		ret->set(i, _MR(Class<FrameLabel>::getInstanceS(sc.labels[i])));
	}
	return ret;
}

ASFUNCTIONBODY(MovieClip,_constructor)
{
	Sprite::_constructor(obj,NULL,0);
/*	th->setVariableByQName("swapDepths","",Class<IFunction>::getFunction(swapDepths));
	th->setVariableByQName("createEmptyMovieClip","",Class<IFunction>::getFunction(createEmptyMovieClip));*/
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
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<InteractiveObject>::getRef());
	c->setDeclaredMethodByQName("numChildren","",Class<IFunction>::getFunction(_getNumChildren),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("getChildIndex","",Class<IFunction>::getFunction(_getChildIndex),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setChildIndex","",Class<IFunction>::getFunction(_setChildIndex),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getChildAt","",Class<IFunction>::getFunction(getChildAt),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getChildByName","",Class<IFunction>::getFunction(getChildByName),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("addChild","",Class<IFunction>::getFunction(addChild),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("removeChild","",Class<IFunction>::getFunction(removeChild),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("removeChildAt","",Class<IFunction>::getFunction(removeChildAt),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("addChildAt","",Class<IFunction>::getFunction(addChildAt),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("swapChildren","",Class<IFunction>::getFunction(swapChildren),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("contains","",Class<IFunction>::getFunction(contains),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("mouseChildren","",Class<IFunction>::getFunction(_setMouseChildren),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("mouseChildren","",Class<IFunction>::getFunction(_getMouseChildren),GETTER_METHOD,true);
}

void DisplayObjectContainer::buildTraits(ASObject* o)
{
}

DisplayObjectContainer::DisplayObjectContainer(Class_base* c):InteractiveObject(c),mouseChildren(true)
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
		objName.name_s_id=getSys()->getUniqueStringId(obj->name);
		objName.ns.push_back(nsNameAndKind("",NAMESPACE));
		setVariableByMultiname(objName,getSys()->getNullRef(), ASObject::CONST_NOT_ALLOWED);
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
		objName.name_s_id=getSys()->getUniqueStringId(obj->name);
		objName.ns.push_back(nsNameAndKind("",NAMESPACE));
		//TODO: discuss the following comment with aajanki
		// If this function is called by PlaceObject tag
		// before the properties are initialized, we need to
		// initialize the property here to make sure that it
		// will be a declared property and not a dynamic one.
		if(hasPropertyByMultiname(objName,true,false))
			setVariableByMultiname(objName,obj,ASObject::CONST_NOT_ALLOWED);
		else
			setVariableByQName(objName.name_s_id,objName.ns[0],obj,DYNAMIC_TRAIT);
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

void DisplayObjectContainer::finalize()
{
	InteractiveObject::finalize();
	//Release every child
	dynamicDisplayList.clear();
}

InteractiveObject::InteractiveObject(Class_base* c):DisplayObject(c),mouseEnabled(true),doubleClickEnabled(false)
{
}

InteractiveObject::~InteractiveObject()
{
	if(getSys()->getInputThread())
		getSys()->getInputThread()->removeListener(this);
}

ASFUNCTIONBODY(InteractiveObject,_constructor)
{
	InteractiveObject* th=static_cast<InteractiveObject*>(obj);
	EventDispatcher::_constructor(obj,NULL,0);
	//Object registered very early are not supported this way (Stage for example)
	if(getSys()->getInputThread())
		getSys()->getInputThread()->addListener(th);

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
	return abstract_b(th->mouseEnabled);
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
	return abstract_b(th->doubleClickEnabled);
}

void InteractiveObject::buildTraits(ASObject* o)
{
}

void InteractiveObject::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<DisplayObject>::getRef());
	c->setDeclaredMethodByQName("mouseEnabled","",Class<IFunction>::getFunction(_setMouseEnabled),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("mouseEnabled","",Class<IFunction>::getFunction(_getMouseEnabled),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("doubleClickEnabled","",Class<IFunction>::getFunction(_setDoubleClickEnabled),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("doubleClickEnabled","",Class<IFunction>::getFunction(_getDoubleClickEnabled),GETTER_METHOD,true);
}

void DisplayObjectContainer::dumpDisplayList()
{
	LOG(LOG_INFO, "Size: " << dynamicDisplayList.size());
	list<_R<DisplayObject> >::const_iterator it=dynamicDisplayList.begin();
	for(;it!=dynamicDisplayList.end();++it)
		LOG(LOG_INFO, (*it)->getClass()->class_name);
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
	return abstract_i(th->dynamicDisplayList.size());
}

ASFUNCTIONBODY(DisplayObjectContainer,_getMouseChildren)
{
	DisplayObjectContainer* th=static_cast<DisplayObjectContainer*>(obj);
	return abstract_b(th->mouseChildren);
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
	{
		return abstract_b(false);
	}
	//Validate object type
	assert_and_throw(args[0] && args[0]->getClass() && 
		args[0]->getClass()->isSubClass(Class<DisplayObject>::getClass()));

	//Cast to object
	DisplayObject* d=static_cast<DisplayObject*>(args[0]);
	d->incRef();
	bool ret=th->_contains(_MR(d));
	return abstract_b(ret);
}

//Only from VM context
ASFUNCTIONBODY(DisplayObjectContainer,addChildAt)
{
	DisplayObjectContainer* th=static_cast<DisplayObjectContainer*>(obj);
	assert_and_throw(argslen==2);
	if(args[0]->getObjectType() == T_CLASS)
	{
		return getSys()->getNullRef();
	}
	//Validate object type
	assert_and_throw(args[0]->getClass() &&
		args[0]->getClass()->isSubClass(Class<DisplayObject>::getClass()));

	int index=args[1]->toInt();

	//Cast to object
	args[0]->incRef();
	_R<DisplayObject> d=_MR(Class<DisplayObject>::cast(args[0]));
	assert_and_throw(index >= 0 && (size_t)index<=th->dynamicDisplayList.size());
	th->_addChildAt(d,index);

	//Notify the object
	getVm()->addEvent(d,_MR(Class<Event>::getInstanceS("added")));

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
		return getSys()->getNullRef();
	}
	//Validate object type
	assert_and_throw(args[0] && args[0]->getClass() && 
		args[0]->getClass()->isSubClass(Class<DisplayObject>::getClass()));

	//Cast to object
	args[0]->incRef();
	_R<DisplayObject> d=_MR(Class<DisplayObject>::cast(args[0]));
	th->_addChildAt(d,numeric_limits<unsigned int>::max());

	//Notify the object
	getVm()->addEvent(d,_MR(Class<Event>::getInstanceS("added")));

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
		return getSys()->getNullRef();
	}
	//Validate object type
	assert_and_throw(args[0] && args[0]->getClass() && 
		args[0]->getClass()->isSubClass(Class<DisplayObject>::getClass()));
	//Cast to object
	DisplayObject* d=Class<DisplayObject>::cast(args[0]);
	d->incRef();
	if(!th->_removeChild(_MR(d)))
		throw Class<ArgumentError>::getInstanceS("removeChild: child not in list");

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
			throw Class<RangeError>::getInstanceS("removeChildAt: invalid index");
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
		args[0]->getClass()->isSubClass(Class<DisplayObject>::getClass()));
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
		args[0]->getClass()->isSubClass(Class<DisplayObject>::getClass()));
	assert_and_throw(args[1] && args[1]->getClass() && 
		args[1]->getClass()->isSubClass(Class<DisplayObject>::getClass()));

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
			throw Class<ArgumentError>::getInstanceS("Argument is not child of this object");

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
		ret=getSys()->getUndefinedRef();
	return ret;
}

//Only from VM context
ASFUNCTIONBODY(DisplayObjectContainer,getChildAt)
{
	DisplayObjectContainer* th=static_cast<DisplayObjectContainer*>(obj);
	assert_and_throw(argslen==1);
	unsigned int index=args[0]->toInt();
	if(index>=th->dynamicDisplayList.size())
		throw Class<RangeError>::getInstanceS("getChildAt: invalid index");
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
			throw Class<ArgumentError>::getInstanceS("getChildIndex: child not in list");
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
	assert_and_throw(args[0]->getClass()->isSubClass(Class<DisplayObject>::getClass()));

	//Cast to object
	_R<DisplayObject> d= _MR(static_cast<DisplayObject*>(args[0]));
	d->incRef();

	return abstract_i(th->getChildIndex(d));
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
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<DisplayObject>::getRef());
	c->setDeclaredMethodByQName("graphics","",Class<IFunction>::getFunction(_getGraphics),GETTER_METHOD,true);
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
		th->graphics=_MR(Class<Graphics>::getInstanceS(th));
	th->graphics->incRef();
	return th->graphics.getPtr();
}

void MorphShape::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<DisplayObject>::getRef());
}

void MorphShape::buildTraits(ASObject* o)
{
	//No traits
}

ASFUNCTIONBODY(MorphShape,_constructor)
{
	DisplayObject::_constructor(obj,NULL,0);
	return NULL;
}

bool MorphShape::boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const
{
	LOG(LOG_NOT_IMPLEMENTED, "MorphShape::boundsRect is a stub");
	return false;
}

_NR<InteractiveObject> MorphShape::hitTestImpl(_NR<InteractiveObject> last, number_t x, number_t y, HIT_TYPE type)
{
	return NullRef;
}

void Stage::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<DisplayObjectContainer>::getRef());
	c->setDeclaredMethodByQName("stageWidth","",Class<IFunction>::getFunction(_getStageWidth),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("stageHeight","",Class<IFunction>::getFunction(_getStageHeight),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("width","",Class<IFunction>::getFunction(_getStageWidth),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("height","",Class<IFunction>::getFunction(_getStageHeight),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("scaleMode","",Class<IFunction>::getFunction(_getScaleMode),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("scaleMode","",Class<IFunction>::getFunction(_setScaleMode),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("loaderInfo","",Class<IFunction>::getFunction(_getLoaderInfo),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("stageVideos","",Class<IFunction>::getFunction(_getStageVideos),GETTER_METHOD,true);
	REGISTER_GETTER_SETTER(c,displayState);
}

void Stage::onDisplayState(const tiny_string&)
{
	LOG(LOG_NOT_IMPLEMENTED,"Stage.displayState = " << displayState);
}

ASFUNCTIONBODY_GETTER_SETTER_CB(Stage,displayState,onDisplayState);

void Stage::buildTraits(ASObject* o)
{
}

Stage::Stage(Class_base* c):DisplayObjectContainer(c)
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

_NR<InteractiveObject> Stage::hitTestImpl(_NR<InteractiveObject> last, number_t x, number_t y, DisplayObject::HIT_TYPE type)
{
	_NR<InteractiveObject> ret;
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
	if(getSys()->scaleMode==SystemState::NO_SCALE)
		width=getSys()->getRenderThread()->windowWidth;
	else
	{
		RECT size=getSys()->mainClip->getFrameSize();
		width=size.Xmax/20;
	}
	return width;
}

uint32_t Stage::internalGetHeight() const
{
	uint32_t height;
	if(getSys()->scaleMode==SystemState::NO_SCALE)
		height=getSys()->getRenderThread()->windowHeight;
	else
	{
		RECT size=getSys()->mainClip->getFrameSize();
		height=size.Ymax/20;
	}
	return height;
}

ASFUNCTIONBODY(Stage,_getStageWidth)
{
	Stage* th=static_cast<Stage*>(obj);
	return abstract_d(th->internalGetWidth());
}

ASFUNCTIONBODY(Stage,_getStageHeight)
{
	Stage* th=static_cast<Stage*>(obj);
	return abstract_d(th->internalGetHeight());
}

ASFUNCTIONBODY(Stage,_getLoaderInfo)
{
	return RootMovieClip::_getLoaderInfo(getSys()->mainClip,NULL,0);
}

ASFUNCTIONBODY(Stage,_getScaleMode)
{
	//Stage* th=static_cast<Stage*>(obj);
	switch(getSys()->scaleMode)
	{
		case SystemState::EXACT_FIT:
			return Class<ASString>::getInstanceS("exactFit");
		case SystemState::SHOW_ALL:
			return Class<ASString>::getInstanceS("showAll");
		case SystemState::NO_BORDER:
			return Class<ASString>::getInstanceS("noBorder");
		case SystemState::NO_SCALE:
			return Class<ASString>::getInstanceS("noScale");
	}
	return NULL;
}

ASFUNCTIONBODY(Stage,_setScaleMode)
{
	//Stage* th=static_cast<Stage*>(obj);
	const tiny_string& arg0=args[0]->toString();
	if(arg0=="exactFit")
		getSys()->scaleMode=SystemState::EXACT_FIT;
	else if(arg0=="showAll")
		getSys()->scaleMode=SystemState::SHOW_ALL;
	else if(arg0=="noBorder")
		getSys()->scaleMode=SystemState::NO_BORDER;
	else if(arg0=="noScale")
		getSys()->scaleMode=SystemState::NO_SCALE;

	RenderThread* rt=getSys()->getRenderThread();
	rt->requestResize(rt->windowWidth, rt->windowHeight, true);
	return NULL;
}

ASFUNCTIONBODY(Stage,_getStageVideos)
{
	LOG(LOG_NOT_IMPLEMENTED, "Accelerated rendering through StageVideo not implemented, SWF should fall back to Video");
	return Class<Vector>::getInstanceS(Class<StageVideo>::getClass());
}

void Graphics::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setDeclaredMethodByQName("clear","",Class<IFunction>::getFunction(clear),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("drawRect","",Class<IFunction>::getFunction(drawRect),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("drawRoundRect","",Class<IFunction>::getFunction(drawRoundRect),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("drawCircle","",Class<IFunction>::getFunction(drawCircle),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("drawTriangles","",Class<IFunction>::getFunction(drawTriangles),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("moveTo","",Class<IFunction>::getFunction(moveTo),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("curveTo","",Class<IFunction>::getFunction(curveTo),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("cubicCurveTo","",Class<IFunction>::getFunction(cubicCurveTo),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("lineTo","",Class<IFunction>::getFunction(lineTo),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("lineStyle","",Class<IFunction>::getFunction(lineStyle),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("beginFill","",Class<IFunction>::getFunction(beginFill),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("beginGradientFill","",Class<IFunction>::getFunction(beginGradientFill),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("beginBitmapFill","",Class<IFunction>::getFunction(beginBitmapFill),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("endFill","",Class<IFunction>::getFunction(endFill),NORMAL_METHOD,true);
}

void Graphics::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(Graphics,_constructor)
{
	return NULL;
}

ASFUNCTIONBODY(Graphics,clear)
{
	Graphics* th=static_cast<Graphics*>(obj);
	th->checkAndSetScaling();
	th->owner->tokens.clear();
	th->owner->owner->requestInvalidation(getSys());
	return NULL;
}

ASFUNCTIONBODY(Graphics,moveTo)
{
	Graphics* th=static_cast<Graphics*>(obj);
	th->checkAndSetScaling();
	assert_and_throw(argslen==2);

	th->curX=args[0]->toInt();
	th->curY=args[1]->toInt();

	th->owner->tokens.emplace_back(GeomToken(MOVE, Vector2(th->curX, th->curY)));
	return NULL;
}

ASFUNCTIONBODY(Graphics,lineTo)
{
	Graphics* th=static_cast<Graphics*>(obj);
	assert_and_throw(argslen==2);
	th->checkAndSetScaling();

	int x=args[0]->toInt();
	int y=args[1]->toInt();

	th->owner->tokens.emplace_back(GeomToken(STRAIGHT, Vector2(x, y)));
	th->owner->owner->requestInvalidation(getSys());

	th->curX=x;
	th->curY=y;
	return NULL;
}

ASFUNCTIONBODY(Graphics,curveTo)
{
	Graphics* th=static_cast<Graphics*>(obj);
	assert_and_throw(argslen==4);
	th->checkAndSetScaling();

	int controlX=args[0]->toInt();
	int controlY=args[1]->toInt();

	int anchorX=args[2]->toInt();
	int anchorY=args[3]->toInt();

	th->owner->tokens.emplace_back(GeomToken(CURVE_QUADRATIC,
	                        Vector2(controlX, controlY),
	                        Vector2(anchorX, anchorY)));
	th->owner->owner->requestInvalidation(getSys());

	th->curX=anchorX;
	th->curY=anchorY;
	return NULL;
}

ASFUNCTIONBODY(Graphics,cubicCurveTo)
{
	Graphics* th=static_cast<Graphics*>(obj);
	assert_and_throw(argslen==6);
	th->checkAndSetScaling();

	int control1X=args[0]->toInt();
	int control1Y=args[1]->toInt();

	int control2X=args[2]->toInt();
	int control2Y=args[3]->toInt();

	int anchorX=args[4]->toInt();
	int anchorY=args[5]->toInt();

	th->owner->tokens.emplace_back(GeomToken(CURVE_CUBIC,
	                        Vector2(control1X, control1Y),
	                        Vector2(control2X, control2Y),
	                        Vector2(anchorX, anchorY)));
	th->owner->owner->requestInvalidation(getSys());

	th->curX=anchorX;
	th->curY=anchorY;
	return NULL;
}

/* KAPPA = 4 * (sqrt2 - 1) / 3
 * This value was found in a Python prompt:
 *
 * >>> 4.0 * (2**0.5 - 1) / 3.0
 *
 * Source: http://whizkidtech.redprince.net/bezier/circle/
 */
const double KAPPA = 0.55228474983079356;

ASFUNCTIONBODY(Graphics,drawRoundRect)
{
	Graphics* th=static_cast<Graphics*>(obj);
	assert_and_throw(argslen==5 || argslen==6);
	th->checkAndSetScaling();

	double x=args[0]->toNumber();
	double y=args[1]->toNumber();
	double width=args[2]->toNumber();
	double height=args[3]->toNumber();
	double ellipseWidth=args[4]->toNumber();
	double ellipseHeight;
	if (argslen == 6)
		ellipseHeight=args[5]->toNumber();

	if (argslen == 5 || std::isnan(ellipseHeight))
		ellipseHeight=ellipseWidth;

	ellipseHeight /= 2;
	ellipseWidth  /= 2;

	double kappaW = KAPPA * ellipseWidth;
	double kappaH = KAPPA * ellipseHeight;

	/*
	 *    A-----B
	 *   /       \
	 *  H         C
	 *  |         |
	 *  G         D
	 *   \       /
	 *    F-----E
	 * 
	 * Flash starts and stops the pen at 'D', so we will too.
	 */

	// D
	th->owner->tokens.emplace_back(GeomToken(MOVE, Vector2(x+width, y+height-ellipseHeight)));

	// D -> E
	th->owner->tokens.emplace_back(GeomToken(CURVE_CUBIC,
	                        Vector2(x+width, y+height-ellipseHeight+kappaH),
	                        Vector2(x+width-ellipseWidth+kappaW, y+height),
	                        Vector2(x+width-ellipseWidth, y+height)));

	// E -> F
	th->owner->tokens.emplace_back(GeomToken(STRAIGHT, Vector2(x+ellipseWidth, y+height)));

	// F -> G
	th->owner->tokens.emplace_back(GeomToken(CURVE_CUBIC,
	                        Vector2(x+ellipseWidth-kappaW, y+height),
	                        Vector2(x, y+height-kappaH),
	                        Vector2(x, y+height-ellipseHeight)));

	// G -> H
	th->owner->tokens.emplace_back(GeomToken(STRAIGHT, Vector2(x, y+ellipseHeight)));

	// H -> A
	th->owner->tokens.emplace_back(GeomToken(CURVE_CUBIC,
	                        Vector2(x, y+ellipseHeight-kappaH),
	                        Vector2(x+ellipseWidth-kappaW, y),
	                        Vector2(x+ellipseWidth, y)));

	// A -> B
	th->owner->tokens.emplace_back(GeomToken(STRAIGHT, Vector2(x+width-ellipseWidth, y)));

	// B -> C
	th->owner->tokens.emplace_back(GeomToken(CURVE_CUBIC,
	                        Vector2(x+width-ellipseWidth+kappaW, y),
	                        Vector2(x+width, y+kappaH),
	                        Vector2(x+width, y+ellipseHeight)));

	// C -> D
	th->owner->tokens.emplace_back(GeomToken(STRAIGHT, Vector2(x+width, y+height-ellipseHeight)));

	th->owner->owner->requestInvalidation(getSys());
	
	return NULL;
}

ASFUNCTIONBODY(Graphics,drawCircle)
{
	Graphics* th=static_cast<Graphics*>(obj);
	assert_and_throw(argslen==3);
	th->checkAndSetScaling();

	double x=args[0]->toNumber();
	double y=args[1]->toNumber();
	double radius=args[2]->toNumber();

	double kappa = KAPPA*radius;

	// right
	th->owner->tokens.emplace_back(GeomToken(MOVE, Vector2(x+radius, y)));

	// bottom
	th->owner->tokens.emplace_back(GeomToken(CURVE_CUBIC,
	                        Vector2(x+radius, y+kappa ),
	                        Vector2(x+kappa , y+radius),
	                        Vector2(x       , y+radius)));

	// left
	th->owner->tokens.emplace_back(GeomToken(CURVE_CUBIC,
	                        Vector2(x-kappa , y+radius),
	                        Vector2(x-radius, y+kappa ),
	                        Vector2(x-radius, y       )));

	// top
	th->owner->tokens.emplace_back(GeomToken(CURVE_CUBIC,
	                        Vector2(x-radius, y-kappa ),
	                        Vector2(x-kappa , y-radius),
	                        Vector2(x       , y-radius)));

	// back to right
	th->owner->tokens.emplace_back(GeomToken(CURVE_CUBIC,
	                        Vector2(x+kappa , y-radius),
	                        Vector2(x+radius, y-kappa ),
	                        Vector2(x+radius, y       )));

	th->owner->owner->requestInvalidation(getSys());
	
	return NULL;
}

ASFUNCTIONBODY(Graphics,drawRect)
{
	Graphics* th=static_cast<Graphics*>(obj);
	assert_and_throw(argslen==4);
	th->checkAndSetScaling();

	int x=args[0]->toInt();
	int y=args[1]->toInt();
	int width=args[2]->toInt();
	int height=args[3]->toInt();

	const Vector2 a(x,y);
	const Vector2 b(x+width,y);
	const Vector2 c(x+width,y+height);
	const Vector2 d(x,y+height);

	th->owner->tokens.emplace_back(GeomToken(MOVE, a));
	th->owner->tokens.emplace_back(GeomToken(STRAIGHT, b));
	th->owner->tokens.emplace_back(GeomToken(STRAIGHT, c));
	th->owner->tokens.emplace_back(GeomToken(STRAIGHT, d));
	th->owner->tokens.emplace_back(GeomToken(STRAIGHT, a));
	th->owner->owner->requestInvalidation(getSys());
	
	return NULL;
}

/* Solve for c in the matrix equation
 *
 * [ 1 x1 y1 ] [ c[0] ]   [ u1 ]
 * [ 1 x2 y2 ] [ c[1] ] = [ u2 ]
 * [ 1 x3 y3 ] [ c[2] ]   [ u3 ]
 *
 * The result will be put in the output parameter c.
 */
void Graphics::solveVertexMapping(double x1, double y1,
				  double x2, double y2,
				  double x3, double y3,
				  double u1, double u2, double u3,
				  double c[3])
{
	double eps = 1e-15;
	double det = fabs(x2*y3 + x1*y2 + y1*x3 - y2*x3 - x1*y3 - y1*x2);

	if (det < eps)
	{
		// Degenerate matrix
		c[0] = c[1] = c[2] = 0;
		return;
	}

	// Symbolic solution of the equation by Gaussian elimination
	if (fabs(x1-x2) < eps)
	{
		c[2] = (u2-u1)/(y2-y1);
		c[1] = (u3 - u1 - (y3-y1)*c[2])/(x3-x1);
		c[0] = u1 - x1*c[1] - y1*c[2];
	}
	else
	{
		c[2] = ((x2-x1)*(u3-u1) - (x3-x1)*(u2-u1))/((y3-y1)*(x2-x1) - (x3-x1)*(y2-y1));
		c[1] = (u2 - u1 - (y2-y1)*c[2])/(x2-x1);
		c[0] = u1 - x1*c[1] - y1*c[2];
	}
}

ASFUNCTIONBODY(Graphics,drawTriangles)
{
	Graphics* th=static_cast<Graphics*>(obj);
	_NR<Vector> vertices;
	_NR<Vector> indices;
	_NR<Vector> uvtData;
	tiny_string culling;
	ARG_UNPACK (vertices) (indices, NullRef) (uvtData, NullRef) (culling, "none");

	if (culling != "none")
		LOG(LOG_NOT_IMPLEMENTED, "Graphics.drawTriangles doesn't support culling");

	// Validate the parameters
	if ((indices.isNull() && (vertices->size() % 6 != 0)) || 
	    (!indices.isNull() && (indices->size() % 3 != 0)))
	{
		throw Class<ArgumentError>::getInstanceS("Error #2004");
	}

	unsigned int numvertices=vertices->size()/2;
	unsigned int numtriangles;
	bool has_uvt=false;
	int uvtElemSize=2;
	int texturewidth=0;
	int textureheight=0;

	if (indices.isNull())
		numtriangles=numvertices/3;
	else
		numtriangles=indices->size()/3;

	if (!uvtData.isNull())
	{
		if (uvtData->size()==2*numvertices)
		{
			has_uvt=true;
			uvtElemSize=2; /* (u, v) */
		}
		else if (uvtData->size()==3*numvertices)
		{
			has_uvt=true;
			uvtElemSize=3; /* (u, v, t), t is ignored */
			LOG(LOG_NOT_IMPLEMENTED, "Graphics.drawTriangles doesn't support t in uvtData parameter");
		}
		else
		{
			throw Class<ArgumentError>::getInstanceS("Error #2004");
		}

		th->owner->getTextureSize(&texturewidth, &textureheight);
	}

	// According to testing, drawTriangles first fills the current
	// path and creates a new path, but keeps the source.
	th->owner->tokens.emplace_back(FILL_KEEP_SOURCE);

	if (has_uvt && (texturewidth==0 || textureheight==0))
		return NULL;

	// Construct the triangles
	for (unsigned int i=0; i<numtriangles; i++)
	{
		double x[3], y[3], u[3]={0}, v[3]={0};
		for (unsigned int j=0; j<3; j++)
		{
			unsigned int vertex;
			if (indices.isNull())
				vertex=3*i+j;
			else
				vertex=indices->at(3*i+j)->toInt();

			x[j]=vertices->at(2*vertex)->toNumber();
			y[j]=vertices->at(2*vertex+1)->toNumber();

			if (has_uvt)
			{
				u[j]=uvtData->at(vertex*uvtElemSize)->toNumber()*texturewidth;
				v[j]=uvtData->at(vertex*uvtElemSize+1)->toNumber()*textureheight;
			}
		}
		
		Vector2 a(x[0], y[0]);
		Vector2 b(x[1], y[1]);
		Vector2 c(x[2], y[2]);

		th->owner->tokens.emplace_back(GeomToken(MOVE, a));
		th->owner->tokens.emplace_back(GeomToken(STRAIGHT, b));
		th->owner->tokens.emplace_back(GeomToken(STRAIGHT, c));
		th->owner->tokens.emplace_back(GeomToken(STRAIGHT, a));

		if (has_uvt)
		{
			double t[6];

			// Use the known (x, y) and (u, v)
			// correspondences to compute a transformation
			// t from (x, y) space into (u, v) space
			// (cairo needs the mapping in this
			// direction).
			//
			// u = t[0] + t[1]*x + t[2]*y
			// v = t[3] + t[4]*x + t[5]*y
			//
			// u and v parts can be solved separately.
			th->solveVertexMapping(x[0], y[0], x[1], y[1], x[2], y[2],
					       u[0], u[1], u[2], t);
			th->solveVertexMapping(x[0], y[0], x[1], y[1], x[2], y[2],
					       v[0], v[1], v[2], &t[3]);

			MATRIX m(t[1], t[5], t[4], t[2], t[0], t[3]);
			th->owner->tokens.emplace_back(GeomToken(FILL_TRANSFORM_TEXTURE, m));
		}
	}
	
	th->owner->owner->requestInvalidation(getSys());

	return NULL;
}

ASFUNCTIONBODY(Graphics,lineStyle)
{
	Graphics* th=static_cast<Graphics*>(obj);
	th->checkAndSetScaling();

	if (argslen == 0)
	{
		th->owner->tokens.emplace_back(CLEAR_STROKE);
		return NULL;
	}
	uint32_t color = 0;
	uint8_t alpha = 255;
	UI16_SWF thickness = UI16_SWF(args[0]->toNumber() * 20);
	if (argslen >= 2)
		color = args[1]->toUInt();
	if (argslen >= 3)
		alpha = uint8_t(args[1]->toNumber() * 255);

	// TODO: pixel hinting, scaling, caps, miter, joints
	
	LINESTYLE2 style(0xff);
	style.Color = RGBA(color, alpha);
	style.Width = thickness;
	th->owner->tokens.emplace_back(GeomToken(SET_STROKE, style));
	return NULL;
}

ASFUNCTIONBODY(Graphics,beginGradientFill)
{
	Graphics* th=static_cast<Graphics*>(obj);
	assert_and_throw(argslen>=4);
	th->checkAndSetScaling();

	FILLSTYLE style(0xff);

	assert_and_throw(args[1]->getObjectType()==T_ARRAY);
	Array* colors=Class<Array>::cast(args[1]);

	assert_and_throw(args[2]->getObjectType()==T_ARRAY);
	Array* alphas=Class<Array>::cast(args[2]);

	//assert_and_throw(args[3]->getObjectType()==T_ARRAY);
	//Work around for bug in YouTube player of July 13 2011
	if(args[3]->getObjectType()==T_UNDEFINED)
		return NULL;
	Array* ratios=Class<Array>::cast(args[3]);

	int NumGradient = colors->size();
	if (NumGradient != (int)alphas->size() || NumGradient != (int)ratios->size())
		return NULL;

	if (NumGradient < 1 || NumGradient > 15)
		return NULL;

	const tiny_string& type=args[0]->toString();

	if(type == "linear")
		style.FillStyleType=LINEAR_GRADIENT;
	else if(type == "radial")
		style.FillStyleType=RADIAL_GRADIENT;
	else
		return NULL;

	// Don't support FOCALGRADIENT for now.
	GRADIENT grad(0xff);
	for(int i = 0; i < NumGradient; i ++)
	{
		GRADRECORD record(0xff);
		record.Color = RGBA(colors->at(i)->toUInt(), (int)alphas->at(i)->toNumber()*255);
		record.Ratio = UI8(ratios->at(i)->toUInt());
		grad.GradientRecords.push_back(record);
	}

	if(argslen > 4 && args[4]->getClass()==Class<Matrix>::getClass())
	{
		style.Matrix = static_cast<Matrix*>(args[4])->getMATRIX();
		//Conversion from twips to pixels
		cairo_matrix_scale(&style.Matrix, 1.0f/20.0f, 1.0f/20.0f);
	}
	else
	{
		cairo_matrix_scale(&style.Matrix, 100.0/16384.0, 100.0/16384.0);
	}

	if(argslen > 5)
	{
		const tiny_string& spread=args[5]->toString();
		if (spread == "pad")
			grad.SpreadMode = 0;
		else if (spread == "reflect")
			grad.SpreadMode = 1;
		else if (spread == "repeat")
			grad.SpreadMode = 2;
	}
	else
	{
		//default is pad
		grad.SpreadMode = 0;
	}


	if(argslen > 6)
	{
		const tiny_string& interp=args[6]->toString();
		if (interp == "rgb")
			grad.InterpolationMode = 0;
		else if (interp == "linearRGB")
			grad.InterpolationMode = 1;
	}
	else
	{
		//default is rgb
		grad.InterpolationMode = 0;
	}

	style.Gradient = grad;
	th->owner->tokens.emplace_back(GeomToken(SET_FILL, style));
	return NULL;
}

ASFUNCTIONBODY(Graphics,beginBitmapFill)
{
	Graphics* th = obj->as<Graphics>();
	_NR<BitmapData> bitmap;
	_NR<Matrix> matrix;
	bool repeat, smooth;
	ARG_UNPACK (bitmap) (matrix, NullRef) (repeat, true) (smooth, false);

	if(bitmap.isNull())
		return NULL;

	th->checkAndSetScaling();
	FILLSTYLE style(0xff);
	if(repeat && smooth)
		style.FillStyleType = REPEATING_BITMAP;
	else if(repeat && !smooth)
		style.FillStyleType = NON_SMOOTHED_REPEATING_BITMAP;
	else if(!repeat && smooth)
		style.FillStyleType = CLIPPED_BITMAP;
	else
		style.FillStyleType = NON_SMOOTHED_CLIPPED_BITMAP;

	if(!matrix.isNull())
		style.Matrix = matrix->getMATRIX();

	style.bitmap = *static_cast<BitmapContainer*>((bitmap.getPtr()));
	th->owner->tokens.emplace_back(GeomToken(SET_FILL, style));
	return NULL;
}

ASFUNCTIONBODY(Graphics,beginFill)
{
	Graphics* th=static_cast<Graphics*>(obj);
	th->checkAndSetScaling();
	uint32_t color=0;
	uint8_t alpha=255;
	if(argslen>=1)
		color=args[0]->toUInt();
	if(argslen>=2)
		alpha=(uint8_t(args[1]->toNumber()*0xff));
	FILLSTYLE style(0xff);
	style.FillStyleType = SOLID_FILL;
	style.Color         = RGBA(color, alpha);
	th->owner->tokens.emplace_back(GeomToken(SET_FILL, style));
	return NULL;
}

ASFUNCTIONBODY(Graphics,endFill)
{
	Graphics* th=static_cast<Graphics*>(obj);
	th->checkAndSetScaling();
	th->owner->tokens.emplace_back(CLEAR_FILL);
	return NULL;
}

void LineScaleMode::sinit(Class_base* c)
{
	c->setConstructor(NULL);
	c->setVariableByQName("HORIZONTAL","",Class<ASString>::getInstanceS("horizontal"),DECLARED_TRAIT);
	c->setVariableByQName("NONE","",Class<ASString>::getInstanceS("none"),DECLARED_TRAIT);
	c->setVariableByQName("NORMAL","",Class<ASString>::getInstanceS("normal"),DECLARED_TRAIT);
	c->setVariableByQName("VERTICAL","",Class<ASString>::getInstanceS("vertical"),DECLARED_TRAIT);
}

void StageScaleMode::sinit(Class_base* c)
{
	c->setConstructor(NULL);
	c->setVariableByQName("EXACT_FIT","",Class<ASString>::getInstanceS("exactFit"),DECLARED_TRAIT);
	c->setVariableByQName("NO_BORDER","",Class<ASString>::getInstanceS("noBorder"),DECLARED_TRAIT);
	c->setVariableByQName("NO_SCALE","",Class<ASString>::getInstanceS("noScale"),DECLARED_TRAIT);
	c->setVariableByQName("SHOW_ALL","",Class<ASString>::getInstanceS("showAll"),DECLARED_TRAIT);
}

void StageAlign::sinit(Class_base* c)
{
	c->setConstructor(NULL);
	c->setVariableByQName("TOP_LEFT","",Class<ASString>::getInstanceS("TL"),DECLARED_TRAIT);
}

void StageQuality::sinit(Class_base* c)
{
	c->setConstructor(NULL);
	c->setVariableByQName("BEST","",Class<ASString>::getInstanceS("best"),DECLARED_TRAIT);
	c->setVariableByQName("HIGH","",Class<ASString>::getInstanceS("high"),DECLARED_TRAIT);
	c->setVariableByQName("LOW","",Class<ASString>::getInstanceS("low"),DECLARED_TRAIT);
	c->setVariableByQName("MEDIUM","",Class<ASString>::getInstanceS("medium"),DECLARED_TRAIT);
}

void StageDisplayState::sinit(Class_base* c)
{
	c->setConstructor(NULL);
	c->setVariableByQName("FULL_SCREEN","",Class<ASString>::getInstanceS("fullScreen"),DECLARED_TRAIT);
	c->setVariableByQName("NORMAL","",Class<ASString>::getInstanceS("normal"),DECLARED_TRAIT);
}

Bitmap::Bitmap(Class_base* c, _NR<LoaderInfo> li, std::istream *s, FILE_TYPE type):
	DisplayObject(c),TokenContainer(this)
{
	if(li)
	{
		loaderInfo = li;
		this->incRef();
		loaderInfo->setWaitedObject(_MR(this));
	}

	bitmapData = _MR(Class<BitmapData>::getInstanceS());
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
			bitmapData->fromJPEG(*s);
			break;
		case FT_PNG:
			bitmapData->fromPNG(*s);
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

Bitmap::Bitmap(Class_base* c, _R<BitmapData> data) : DisplayObject(c),TokenContainer(this)
{
	bitmapData = data;
	bitmapData->addUser(this);
	Bitmap::updatedData();
}

Bitmap::~Bitmap()
{
	finalize();
}

void Bitmap::finalize()
{
	if(!bitmapData.isNull())
		bitmapData->removeUser(this);
	bitmapData.reset();
	DisplayObject::finalize();
}

void Bitmap::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<DisplayObject>::getRef());
	REGISTER_GETTER_SETTER(c,bitmapData);
}

ASFUNCTIONBODY(Bitmap,_constructor)
{
	tiny_string _pixelSnapping;
	bool _smoothing;
	_NR<BitmapData> _bitmapData;
	Bitmap* th = obj->as<Bitmap>();
	ARG_UNPACK(_bitmapData, NullRef)(_pixelSnapping, "auto")(_smoothing, false);

	DisplayObject::_constructor(obj,NULL,0);

	if(_pixelSnapping!="auto")
		LOG(LOG_NOT_IMPLEMENTED, "Bitmap constructor doesn't support pixelSnapping");
	if(_smoothing)
		LOG(LOG_NOT_IMPLEMENTED, "Bitmap constructor doesn't support smoothing");

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

ASFUNCTIONBODY_GETTER_SETTER_CB(Bitmap,bitmapData,onBitmapData);

void Bitmap::updatedData()
{
	tokens.clear();

	if(bitmapData.isNull())
		return;

	FILLSTYLE style(0xff);
	style.FillStyleType=CLIPPED_BITMAP;
	style.bitmap=*static_cast<BitmapContainer*>(bitmapData.getPtr());
	tokens.emplace_back(GeomToken(SET_FILL, style));
	tokens.emplace_back(GeomToken(MOVE, Vector2(0, 0)));
	tokens.emplace_back(GeomToken(STRAIGHT, Vector2(0, bitmapData->getHeight())));
	tokens.emplace_back(GeomToken(STRAIGHT, Vector2(bitmapData->getWidth(), bitmapData->getHeight())));
	tokens.emplace_back(GeomToken(STRAIGHT, Vector2(bitmapData->getWidth(), 0)));
	tokens.emplace_back(GeomToken(STRAIGHT, Vector2(0, 0)));
	if(onStage)
		requestInvalidation(getSys());
}
bool Bitmap::boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const
{
	return TokenContainer::boundsRect(xmin,xmax,ymin,ymax);
}

_NR<InteractiveObject> Bitmap::hitTestImpl(_NR<InteractiveObject> last, number_t x, number_t y, DisplayObject::HIT_TYPE type)
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
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<InteractiveObject>::getRef());
	c->setDeclaredMethodByQName("upState","",Class<IFunction>::getFunction(_getUpState),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("upState","",Class<IFunction>::getFunction(_setUpState),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("downState","",Class<IFunction>::getFunction(_getDownState),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("downState","",Class<IFunction>::getFunction(_setDownState),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("overState","",Class<IFunction>::getFunction(_getOverState),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("overState","",Class<IFunction>::getFunction(_setOverState),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("hitTestState","",Class<IFunction>::getFunction(_getHitTestState),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("hitTestState","",Class<IFunction>::getFunction(_setHitTestState),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("enabled","",Class<IFunction>::getFunction(_getEnabled),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("enabled","",Class<IFunction>::getFunction(_setEnabled),SETTER_METHOD,true);
}

void SimpleButton::buildTraits(ASObject* o)
{
}

_NR<InteractiveObject> SimpleButton::hitTestImpl(_NR<InteractiveObject> last, number_t x, number_t y, DisplayObject::HIT_TYPE type)
{
	_NR<InteractiveObject> ret = NullRef;
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
		return getSys()->getNullRef();

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
		return getSys()->getNullRef();

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
		return getSys()->getNullRef();

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
		return getSys()->getNullRef();

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
	return abstract_b(th->enabled);
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
	return abstract_b(th->useHandCursor);
}

void GradientType::sinit(Class_base* c)
{
	c->setConstructor(NULL);
	c->setVariableByQName("LINEAR","",Class<ASString>::getInstanceS("linear"),DECLARED_TRAIT);
	c->setVariableByQName("RADIAL","",Class<ASString>::getInstanceS("radial"),DECLARED_TRAIT);
}

void BlendMode::sinit(Class_base* c)
{
	c->setConstructor(NULL);
	c->setVariableByQName("ADD","",Class<ASString>::getInstanceS("add"),DECLARED_TRAIT);
	c->setVariableByQName("ALPHA","",Class<ASString>::getInstanceS("alpha"),DECLARED_TRAIT);
	c->setVariableByQName("DARKEN","",Class<ASString>::getInstanceS("darken"),DECLARED_TRAIT);
	c->setVariableByQName("DIFFERENCE","",Class<ASString>::getInstanceS("difference"),DECLARED_TRAIT);
	c->setVariableByQName("ERASE","",Class<ASString>::getInstanceS("erase"),DECLARED_TRAIT);
	c->setVariableByQName("HARDLIGHT","",Class<ASString>::getInstanceS("hardlight"),DECLARED_TRAIT);
	c->setVariableByQName("INVERT","",Class<ASString>::getInstanceS("invert"),DECLARED_TRAIT);
	c->setVariableByQName("LAYER","",Class<ASString>::getInstanceS("layer"),DECLARED_TRAIT);
	c->setVariableByQName("LIGHTEN","",Class<ASString>::getInstanceS("lighten"),DECLARED_TRAIT);
	c->setVariableByQName("MULTIPLY","",Class<ASString>::getInstanceS("multiply"),DECLARED_TRAIT);
	c->setVariableByQName("NORMAL","",Class<ASString>::getInstanceS("normal"),DECLARED_TRAIT);
	c->setVariableByQName("OVERLAY","",Class<ASString>::getInstanceS("overlay"),DECLARED_TRAIT);
	c->setVariableByQName("SCREEN","",Class<ASString>::getInstanceS("screen"),DECLARED_TRAIT);
	c->setVariableByQName("SUBSTRACT","",Class<ASString>::getInstanceS("substract"),DECLARED_TRAIT);
}

void SpreadMethod::sinit(Class_base* c)
{
	c->setConstructor(NULL);
	c->setVariableByQName("PAD","",Class<ASString>::getInstanceS("pad"),DECLARED_TRAIT);
	c->setVariableByQName("REFLECT","",Class<ASString>::getInstanceS("reflect"),DECLARED_TRAIT);
	c->setVariableByQName("REPEAT","",Class<ASString>::getInstanceS("repeat"),DECLARED_TRAIT);
}

void InterpolationMethod::sinit(Class_base* c)
{
	c->setConstructor(NULL);
	c->setVariableByQName("RGB","",Class<ASString>::getInstanceS("rgb"),DECLARED_TRAIT);
	c->setVariableByQName("LINEAR_RGB","",Class<ASString>::getInstanceS("linearRGB"),DECLARED_TRAIT);
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
	if((!dynamic_cast<RootMovieClip*>(this) && !dynamic_cast<DefineSpriteTag*>(this))
		|| !getClass()->isSubClass(Class<MovieClip>::getClass()))
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
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<DisplayObject>::getRef());
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
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<ASObject>::getRef());
}

ASFUNCTIONBODY(Shader,_constructor)
{
	LOG(LOG_NOT_IMPLEMENTED, _("Shader class is unimplemented."));
	return NULL;
}
