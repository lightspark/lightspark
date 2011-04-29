/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009,2010  Alessandro Pignotti (a.pignotti@sssup.it)

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
#include <algorithm>

#include "abc.h"
#include "flashdisplay.h"
#include "swf.h"
#include "flashgeom.h"
#include "flashnet.h"
#include "flashsystem.h"
#include "parsing/streams.h"
#include "compat.h"
#include "class.h"
#include "backends/rendering.h"
#include "backends/geometry.h"
#include "compat.h"

#include <GL/glew.h>
#include <fstream>
#include <limits>
#include <cmath>

using namespace std;
using namespace lightspark;

SET_NAMESPACE("flash.display");

REGISTER_CLASS_NAME(LoaderInfo);
REGISTER_CLASS_NAME(MovieClip);
REGISTER_CLASS_NAME(DisplayObject);
REGISTER_CLASS_NAME(InteractiveObject);
REGISTER_CLASS_NAME(DisplayObjectContainer);
REGISTER_CLASS_NAME(Sprite);
REGISTER_CLASS_NAME(Loader);
REGISTER_CLASS_NAME(Shape);
REGISTER_CLASS_NAME(MorphShape);
REGISTER_CLASS_NAME(Stage);
REGISTER_CLASS_NAME(Graphics);
REGISTER_CLASS_NAME(LineScaleMode);
REGISTER_CLASS_NAME(StageScaleMode);
REGISTER_CLASS_NAME(StageAlign);
REGISTER_CLASS_NAME(StageQuality);
REGISTER_CLASS_NAME(StageDisplayState);
REGISTER_CLASS_NAME(GradientType);
REGISTER_CLASS_NAME(BlendMode);
REGISTER_CLASS_NAME(SpreadMethod);
REGISTER_CLASS_NAME(InterpolationMethod);
REGISTER_CLASS_NAME(Bitmap);
REGISTER_CLASS_NAME(SimpleButton);

void LoaderInfo::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->super=Class<EventDispatcher>::getClass();
	c->max_level=c->super->max_level+1;
	c->setGetterByQName("loaderURL","",Class<IFunction>::getFunction(_getLoaderURL),true);
	c->setGetterByQName("loader","",Class<IFunction>::getFunction(_getLoader),true);
	c->setGetterByQName("content","",Class<IFunction>::getFunction(_getContent),true);
	c->setGetterByQName("url","",Class<IFunction>::getFunction(_getURL),true);
	c->setGetterByQName("bytesLoaded","",Class<IFunction>::getFunction(_getBytesLoaded),true);
	c->setGetterByQName("bytesTotal","",Class<IFunction>::getFunction(_getBytesTotal),true);
	c->setGetterByQName("applicationDomain","",Class<IFunction>::getFunction(_getApplicationDomain),true);
	c->setGetterByQName("sharedEvents","",Class<IFunction>::getFunction(_getSharedEvents),true);
}

void LoaderInfo::buildTraits(ASObject* o)
{
}

void LoaderInfo::setBytesLoaded(uint32_t b)
{
	if(b!=bytesLoaded)
	{
		SpinlockLocker l(spinlock);
		bytesLoaded=b;
		if(sys && sys->currentVm)
			sys->currentVm->addEvent(this,Class<ProgressEvent>::getInstanceS(bytesLoaded,bytesTotal));
		if(loadStatus==INIT_SENT)
		{
			//The clip is also complete now
			Event* e=Class<Event>::getInstanceS("complete");
			sys->currentVm->addEvent(this,e);
			e->decRef();
			loadStatus=COMPLETE;
		}
	}
}

void LoaderInfo::sendInit()
{
	Event* e=Class<Event>::getInstanceS("init");
	sys->currentVm->addEvent(this,e);
	e->decRef();
	SpinlockLocker l(spinlock);
	assert(loadStatus==STARTED);
	loadStatus=INIT_SENT;
	if(bytesTotal && bytesLoaded==bytesTotal)
	{
		//The clip is also complete now
		e=Class<Event>::getInstanceS("complete");
		sys->currentVm->addEvent(this,e);
		e->decRef();
		loadStatus=COMPLETE;
	}
}

ASFUNCTIONBODY(LoaderInfo,_constructor)
{
	LoaderInfo* th=static_cast<LoaderInfo*>(obj);
	EventDispatcher::_constructor(obj,NULL,0);
	th->sharedEvents=Class<EventDispatcher>::getInstanceS();
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
	if(th->loader)
		return Loader::_getContent(th->loader,NULL,0);
	else
		return new Undefined;
}

ASFUNCTIONBODY(LoaderInfo,_getLoader)
{
	LoaderInfo* th=static_cast<LoaderInfo*>(obj);
	if(th->loader)
	{
		th->loader->incRef();
		return th->loader;
	}
	else
		return new Undefined;
}

ASFUNCTIONBODY(LoaderInfo,_getSharedEvents)
{
	LoaderInfo* th=static_cast<LoaderInfo*>(obj);
	th->sharedEvents->incRef();
	return th->sharedEvents;
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
	return Class<ApplicationDomain>::getInstanceS();
}

ASFUNCTIONBODY(Loader,_constructor)
{
	Loader* th=static_cast<Loader*>(obj);
	DisplayObjectContainer::_constructor(obj,NULL,0);
	th->contentLoaderInfo=Class<LoaderInfo>::getInstanceS(th);
	return NULL;
}

ASFUNCTIONBODY(Loader,_getContent)
{
	Loader* th=static_cast<Loader*>(obj);
	if(th->local_root)
	{
		th->local_root->incRef();
		return th->local_root;
	}
	else
		return new Undefined;
}

ASFUNCTIONBODY(Loader,_getContentLoaderInfo)
{
	Loader* th=static_cast<Loader*>(obj);
	th->contentLoaderInfo->incRef();
	return th->contentLoaderInfo;
}

ASFUNCTIONBODY(Loader,load)
{
	Loader* th=static_cast<Loader*>(obj);
	if(th->loading)
		return NULL;
	th->loading=true;
	assert_and_throw(argslen > 0 && args[0] && argslen <= 2);
	assert_and_throw(args[0]->getPrototype()->isSubClass(Class<URLRequest>::getClass()));
	URLRequest* r=static_cast<URLRequest*>(args[0]);
	assert_and_throw(r->method==URLRequest::GET);
	th->url=r->url;
	th->source=URL;
	//To be decreffed in jobFence
	th->incRef();
	sys->addJob(th);
	return NULL;
}

ASFUNCTIONBODY(Loader,loadBytes)
{
	Loader* th=static_cast<Loader*>(obj);
	if(th->loading)
		return NULL;
	//Find the actual ByteArray object
	assert_and_throw(argslen>=1);
	assert_and_throw(args[0]->getPrototype()->isSubClass(Class<ByteArray>::getClass()));
	th->bytes=static_cast<ByteArray*>(args[0]);
	if(th->bytes->bytes)
	{
		th->bytes->incRef();
		th->loading=true;
		th->source=BYTES;
		//To be decreffed in jobFence
		th->incRef();
		sys->addJob(th);
	}
	return NULL;
}

Loader::~Loader()
{
	if(local_root && !sys->finalizingDestruction)
		local_root->decRef();
}

void Loader::jobFence()
{
	decRef();
}

void Loader::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->super=Class<DisplayObjectContainer>::getClass();
	c->max_level=c->super->max_level+1;
	c->setGetterByQName("contentLoaderInfo","",Class<IFunction>::getFunction(_getContentLoaderInfo),true);
	c->setGetterByQName("content","",Class<IFunction>::getFunction(_getContent),true);
	c->setMethodByQName("loadBytes","",Class<IFunction>::getFunction(loadBytes),true);
	c->setMethodByQName("load","",Class<IFunction>::getFunction(load),true);
}

void Loader::buildTraits(ASObject* o)
{
}

void Loader::execute()
{
	assert(source==URL || source==BYTES);
	//The loaderInfo of the content is our contentLoaderInfo
	contentLoaderInfo->incRef();
	local_root=RootMovieClip::getInstance(contentLoaderInfo);
	if(source==URL)
	{
		//TODO: add security checks
		LOG(LOG_CALLS,_("Loader async execution ") << url);
		downloader=sys->downloadManager->download(url, false, contentLoaderInfo);
		downloader->waitForData(); //Wait for some data, making sure our check for failure is working
		if(downloader->hasFailed()) //Check to see if the download failed for some reason
		{
			LOG(LOG_ERROR, "Loader::execute(): Download of URL failed: " << url);
			sys->currentVm->addEvent(contentLoaderInfo,Class<Event>::getInstanceS("ioError"));
			sys->downloadManager->destroy(downloader);
			return;
		}
		sys->currentVm->addEvent(contentLoaderInfo,Class<Event>::getInstanceS("open"));
		istream s(downloader);
		ParseThread* local_pt=new ParseThread(local_root,s);
		local_pt->run();
		{
			//Acquire the lock to ensure consistency in threadAbort
			SpinlockLocker l(downloaderLock);
			sys->downloadManager->destroy(downloader);
			downloader=NULL;
		}
		//complete event is dispatched when the LoaderInfo has sent init and bytesTotal==bytesLoaded
	}
	else if(source==BYTES)
	{
		//TODO: set bytesLoaded and bytesTotal
		assert_and_throw(bytes->bytes);

		//We only support swf files now
		assert_and_throw(memcmp(bytes->bytes,"CWS",3)==0);

		bytes_buf bb(bytes->bytes,bytes->len);
		istream s(&bb);

		ParseThread* local_pt = new ParseThread(local_root,s);
		local_pt->run();
		bytes->decRef();
		//Add a complete event for this object
		sys->currentVm->addEvent(contentLoaderInfo,Class<Event>::getInstanceS("complete"));
	}
	loaded=true;
}

void Loader::threadAbort()
{
	if(source==URL)
	{
		//We have to stop the downloader
		SpinlockLocker l(downloaderLock);
		if(downloader != NULL)
			downloader->stop();
	}
}

void Loader::Render(bool maskEnabled)
{
	if(!loaded || skipRender(maskEnabled))
		return;

	renderPrologue();

	local_root->Render(maskEnabled);

	renderEpilogue();
}

bool Loader::getBounds(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const
{
	if(local_root && local_root->getBounds(xmin,xmax,ymin,ymax))
	{
		getMatrix().multiply2D(xmin,ymin,xmin,ymin);
		getMatrix().multiply2D(xmax,ymax,xmax,ymax);
		return true;
	}
	else
		return false;
}

Sprite::Sprite():GraphicsContainer(this),constructed(false)
{
}

Sprite::Sprite(const Sprite& r):GraphicsContainer(this),constructed(false)
{
	assert(!r.isConstructed());
}

void Sprite::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->super=Class<DisplayObjectContainer>::getClass();
	c->max_level=c->super->max_level+1;
	c->setGetterByQName("graphics","",Class<IFunction>::getFunction(_getGraphics),true);
}

void GraphicsContainer::invalidateGraphics()
{
	assert(graphics);
	if(!owner->isOnStage())
		return;
	uint32_t x,y,width,height;
	number_t bxmin,bxmax,bymin,bymax;
	if(graphics->getBounds(bxmin,bxmax,bymin,bymax)==false)
	{
		//No contents, nothing to do
		return;
	}
	owner->computeDeviceBoundsForRect(bxmin,bxmax,bymin,bymax,x,y,width,height);
	if(width==0 || height==0)
		return;
	CairoRenderer* r=new CairoRenderer(&owner->shepherd, owner->cachedSurface, graphics->tokens,
				owner->getConcatenatedMatrix(), x, y, width, height, 1);
	sys->addJob(r);
}

void Sprite::buildTraits(ASObject* o)
{
}

bool Sprite::boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const
{
	bool ret=false;
	{
		Locker l(mutexDisplayList);
		if(dynamicDisplayList.empty() && graphics==NULL)
			return false;

		//TODO: Check bounds calculation
		list<DisplayObject*>::const_iterator it=dynamicDisplayList.begin();
		for(;it!=dynamicDisplayList.end();++it)
		{
			number_t txmin,txmax,tymin,tymax;
			if((*it)->getBounds(txmin,txmax,tymin,tymax))
			{
				if(ret==true)
				{
					xmin = imin(xmin,txmin);
					xmax = imax(xmax,txmax);
					ymin = imin(ymin,txmin);
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
	}
	if(graphics)
	{
		number_t txmin,txmax,tymin,tymax;
		if(graphics->getBounds(txmin,txmax,tymin,tymax))
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
	}
	return ret;
}

bool Sprite::getBounds(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const
{
	bool ret=boundsRect(xmin,xmax,ymin,ymax);
	if(ret)
	{
		//TODO: take rotation into account
		getMatrix().multiply2D(xmin,ymin,xmin,ymin);
		getMatrix().multiply2D(xmax,ymax,xmax,ymax);
	}
	return ret;
}

void Sprite::invalidate()
{
	assert(graphics);
	invalidateGraphics();
}

void Sprite::requestInvalidation()
{
	DisplayObjectContainer::requestInvalidation();
	if(graphics)
		sys->addToInvalidateQueue(this);
}

void DisplayObject::renderPrologue() const
{
	if(mask)
	{
		if(mask->parent)
			rt->pushMask(mask,mask->parent->getConcatenatedMatrix());
		else
			rt->pushMask(mask,MATRIX());
	}
}

void DisplayObject::renderEpilogue() const
{
	if(mask)
		rt->popMask();
}

void Sprite::renderImpl(bool maskEnabled, number_t t1,number_t t2,number_t t3,number_t t4) const
{
	//Draw the dynamically added graphics, if any
	if(graphics)
	{
		//Should clean only the bounds of the graphics
		if(!isSimple())
			rt->glAcquireTempBuffer(t1,t2,t3,t4);
		defaultRender(maskEnabled);
		if(!isSimple())
			rt->glBlitTempBuffer(t1,t2,t3,t4);
	}
	
	{
		Locker l(mutexDisplayList);
		//Now draw also the display list
		list<DisplayObject*>::const_iterator it=dynamicDisplayList.begin();
		for(;it!=dynamicDisplayList.end();++it)
			(*it)->Render(maskEnabled);
	}
}

void Sprite::Render(bool maskEnabled)
{
	if(skipRender(maskEnabled))
		return;

	//TODO: TOLOCK
	number_t t1,t2,t3,t4;
	bool notEmpty=boundsRect(t1,t2,t3,t4);
	if(!notEmpty)
		return;

	renderPrologue();

	renderImpl(maskEnabled,t1,t2,t3,t4);

	renderEpilogue();
}

void DisplayObject::hitTestPrologue() const
{
	if(mask)
		sys->getInputThread()->pushMask(mask,mask->getConcatenatedMatrix().getInverted());
}

void DisplayObject::hitTestEpilogue() const
{
	if(mask)
		sys->getInputThread()->popMask();
}

InteractiveObject* Sprite::hitTestImpl(number_t x, number_t y)
{
	InteractiveObject* ret=NULL;
	{
		//Test objects added at runtime, in reverse order
		Locker l(mutexDisplayList);
		list<DisplayObject*>::const_reverse_iterator j=dynamicDisplayList.rbegin();
		for(;j!=dynamicDisplayList.rend();++j)
		{
			number_t localX, localY;
			(*j)->getMatrix().getInverted().multiply2D(x,y,localX,localY);
			ret=(*j)->hitTest(this, localX,localY);
			if(ret)
				break;
		}
	}

	if(ret==NULL && graphics && mouseEnabled)
	{
		//The coordinates are locals
		if(graphics->hitTest(x,y))
		{
			ret=this;
			//Also test if the we are under the mask (if any)
			if(sys->getInputThread()->isMaskPresent())
			{
				number_t globalX, globalY;
				getConcatenatedMatrix().multiply2D(x,y,globalX,globalY);
				if(!sys->getInputThread()->isMasked(globalX, globalY))
					ret=NULL;
			}
		}
	}
	return ret;
}

InteractiveObject* Sprite::hitTest(InteractiveObject*, number_t x, number_t y)
{
	//NOTE: in hitTest the stuff must be rendered in the opposite order of Rendering
	//TODO: TOLOCK
	//First of all filter using the BBOX
	number_t t1,t2,t3,t4;
	bool notEmpty=boundsRect(t1,t2,t3,t4);
	if(!notEmpty)
		return NULL;
	if(x<t1 || x>t2 || y<t3 || y>t4)
		return NULL;

	hitTestPrologue();

	InteractiveObject* ret=hitTestImpl(x, y);

	hitTestEpilogue();
	return ret;
}

ASFUNCTIONBODY(Sprite,_constructor)
{
	Sprite* th=Class<Sprite>::cast(obj);
	DisplayObjectContainer::_constructor(obj,NULL,0);
	th->setConstructed();

	return NULL;
}

ASFUNCTIONBODY(Sprite,_getGraphics)
{
	Sprite* th=static_cast<Sprite*>(obj);
	//Probably graphics is not used often, so create it here
	if(th->graphics==NULL)
		th->graphics=Class<Graphics>::getInstanceS(th);

	th->graphics->incRef();
	return th->graphics;
}

void MovieClip::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->super=Class<Sprite>::getClass();
	c->max_level=c->super->max_level+1;
	c->setGetterByQName("currentFrame","",Class<IFunction>::getFunction(_getCurrentFrame),true);
	c->setGetterByQName("totalFrames","",Class<IFunction>::getFunction(_getTotalFrames),true);
	c->setGetterByQName("framesLoaded","",Class<IFunction>::getFunction(_getFramesLoaded),true);
	c->setMethodByQName("stop","",Class<IFunction>::getFunction(stop),true);
	c->setMethodByQName("gotoAndStop","",Class<IFunction>::getFunction(gotoAndStop),true);
	c->setMethodByQName("nextFrame","",Class<IFunction>::getFunction(nextFrame),true);
	c->setMethodByQName("addFrameScript","",Class<IFunction>::getFunction(addFrameScript),true);
}

void MovieClip::buildTraits(ASObject* o)
{
}

MovieClip::MovieClip():totalFrames(1),framesLoaded(1),cur_frame(NULL)
{
	//It's ok to initialize here framesLoaded=1, as it is valid and empty
	//RooMovieClip() will reset it, as stuff loaded dynamically needs frames to be committed
	frames.emplace_back();
	cur_frame=&frames.back();
	frameScripts.resize(totalFrames,NULL);
}

void MovieClip::setTotalFrames(uint32_t t)
{
	assert(totalFrames==1);
	totalFrames=t;
	frameScripts.resize(totalFrames,NULL);
}

void MovieClip::addToFrame(DisplayListTag* t)
{
	cur_frame->blueprint.push_back(t);
}

uint32_t MovieClip::getFrameIdByLabel(const tiny_string& l) const
{
	for(uint32_t i=0;i<framesLoaded;i++)
	{
		if(frames[i].Label==l)
			return i;
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
		if(frame>=th->totalFrames)
			return NULL;
		if(args[i+1]->getObjectType()!=T_FUNCTION)
		{
			LOG(LOG_ERROR,_("Not a function"));
			return NULL;
		}
		IFunction* f=static_cast<IFunction*>(args[i+1]);
		f->incRef();
		assert(th->frameScripts.size()==th->totalFrames);
		th->frameScripts[frame]=f;
	}
	
	return NULL;
}

ASFUNCTIONBODY(MovieClip,createEmptyMovieClip)
{
	LOG(LOG_NOT_IMPLEMENTED,_("createEmptyMovieClip"));
	return new Undefined;
/*	MovieClip* th=static_cast<MovieClip*>(obj);
	if(th==NULL)
		LOG(ERROR,_("Not a valid MovieClip"));

	LOG(CALLS,_("Called createEmptyMovieClip: ") << args->args[0]->toString() << _(" ") << args->args[1]->toString());
	MovieClip* ret=new MovieClip();

	DisplayObject* t=new ASObjectWrapper(ret,args->args[1]->toInt());
	list<DisplayObject*>::iterator it=lower_bound(th->dynamicDisplayList.begin(),th->dynamicDisplayList.end(),t->getDepth(),list_orderer);
	th->dynamicDisplayList.insert(it,t);

	th->setVariableByName(args->args[0]->toString(),ret);
	return ret;*/
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

ASFUNCTIONBODY(MovieClip,gotoAndStop)
{
	MovieClip* th=static_cast<MovieClip*>(obj);
	assert_and_throw(argslen==1);
	if(args[0]->getObjectType()==T_STRING)
	{
		uint32_t dest=th->getFrameIdByLabel(args[0]->toString());
		if(dest==0xffffffff)
			throw RunTimeException("MovieClip::gotoAndStop frame does not exists");
		th->state.next_FP=dest;
	}
	else
		th->state.next_FP=args[0]->toInt();

	//TODO: check, should wrap around?
	th->state.next_FP%=th->state.max_FP;
	th->state.explicit_FP=true;
	th->state.stop_FP=true;
	return NULL;
}

ASFUNCTIONBODY(MovieClip,nextFrame)
{
	MovieClip* th=static_cast<MovieClip*>(obj);
	assert_and_throw(th->state.FP<th->state.max_FP);
	sys->currentVm->addEvent(NULL,new FrameChangeEvent(th->state.FP+1,th));
	return NULL;
}

ASFUNCTIONBODY(MovieClip,_getFramesLoaded)
{
	MovieClip* th=static_cast<MovieClip*>(obj);
	//currentFrame is 1-based
	return abstract_i(th->framesLoaded);
}

ASFUNCTIONBODY(MovieClip,_getTotalFrames)
{
	MovieClip* th=static_cast<MovieClip*>(obj);
	//currentFrame is 1-based
	return abstract_i(th->totalFrames);
}

ASFUNCTIONBODY(MovieClip,_getCurrentFrame)
{
	MovieClip* th=static_cast<MovieClip*>(obj);
	//currentFrame is 1-based
	return abstract_i(th->state.FP+1);
}

ASFUNCTIONBODY(MovieClip,_constructor)
{
	//MovieClip* th=Class<MovieClip>::cast(obj);
	Sprite::_constructor(obj,NULL,0);
/*	th->setVariableByQName("swapDepths","",Class<IFunction>::getFunction(swapDepths));
	th->setVariableByQName("createEmptyMovieClip","",Class<IFunction>::getFunction(createEmptyMovieClip));*/
	return NULL;
}

void MovieClip::advanceFrame()
{
	if(!isConstructed()) //The constructor has not been run yet for this clip
		return;
	if((!state.stop_FP || state.explicit_FP) && totalFrames!=0 && getPrototype()->isSubClass(Class<MovieClip>::getClass()))
	{
		//If we have not yet loaded enough frames delay advancement
		if(state.next_FP>=framesLoaded)
		{
			LOG(LOG_NOT_IMPLEMENTED,_("Not enough frames loaded"));
			return;
		}
		//Before assigning the next_FP we initialize the frame
		//Should initialize all the frames from the current to the next
		for(uint32_t i=(state.FP+1);i<=state.next_FP;i++)
			frames[i].init(this,frames[i-1].displayList);

		//Before actually changing the frame verify that it's constructed
		//If it's not delay the advancement
		if(!frames[state.next_FP].isConstructed())
			return;

		bool frameChanging=(state.FP!=state.next_FP);
		state.FP=state.next_FP;
		if(!state.stop_FP && framesLoaded>0)
			state.next_FP=imin(state.FP+1,framesLoaded-1);
		state.explicit_FP=false;
		assert(state.FP<frameScripts.size());
		if(frameChanging && frameScripts[state.FP])
		{
			FunctionEvent* funcEvent = new FunctionEvent(frameScripts[state.FP]);
			getVm()->addEvent(NULL, funcEvent);
			funcEvent->decRef();
		}

		Frame& curFrame=frames[state.FP];

		//Set the object on stage
		if(isOnStage())
		{
			list<std::pair<PlaceInfo, DisplayObject*> >::const_iterator it=curFrame.displayList.begin();
			for(;it!=curFrame.displayList.end();it++)
				it->second->setOnStage(true);
		}

		//Invalidate the current frame if needed
		if(curFrame.isInvalid())
		{
			list<std::pair<PlaceInfo, DisplayObject*> >::const_iterator it=curFrame.displayList.begin();
			for(;it!=curFrame.displayList.end();it++)
				it->second->requestInvalidation();
			curFrame.setInvalid(false);
		}
	}

}

void MovieClip::requestInvalidation()
{
	Sprite::requestInvalidation();
	//Mark all frames as not valid
	for(uint32_t i=0;i<frames.size();i++)
		frames[i].setInvalid(true);

	if(framesLoaded)
	{
		assert(state.FP<framesLoaded);
		//Actually invalidate the current frame
		Frame& curFrame=frames[state.FP];
		list<std::pair<PlaceInfo, DisplayObject*> >::const_iterator it=curFrame.displayList.begin();
		for(;it!=curFrame.displayList.end();it++)
			it->second->requestInvalidation();
		curFrame.setInvalid(false);
	}
}

void MovieClip::setOnStage(bool staged)
{
	if(staged!=onStage)
	{
		DisplayObjectContainer::setOnStage(staged);
		//Now notify all the objects in all frames
		for(uint32_t i=0;i<frames.size();i++)
		{
			list<std::pair<PlaceInfo, DisplayObject*> >::const_iterator it=frames[i].displayList.begin();
			for(;it!=frames[i].displayList.end();it++)
				it->second->setOnStage(staged);
		}
	}
}

void MovieClip::setRoot(RootMovieClip* r)
{
	if(r==root)
		return;
	if(root)
		root->unregisterChildClip(this);
	DisplayObjectContainer::setRoot(r);
	//Now notify all the objects in all frames
	for(uint32_t i=0;i<frames.size();i++)
	{
		list<std::pair<PlaceInfo, DisplayObject*> >::const_iterator it=frames[i].displayList.begin();
		for(;it!=frames[i].displayList.end();it++)
			it->second->setRoot(root);
	}
	if(root)
		root->registerChildClip(this);
}

void MovieClip::bootstrap()
{
	if(totalFrames==0)
		return;
	assert_and_throw(framesLoaded>0);
	assert_and_throw(frames.size()>=1);
	frames[0].init(this,list<pair<PlaceInfo,DisplayObject*> >());
}

void MovieClip::Render(bool maskEnabled)
{
	if(!isConstructed()) //The constructor has not been run yet for this clip
		return;
	if(skipRender(maskEnabled))
		return;

	number_t t1,t2,t3,t4;
	bool notEmpty=boundsRect(t1,t2,t3,t4);
	if(!notEmpty)
		return;

	renderPrologue();

	Sprite::renderImpl(maskEnabled,t1,t2,t3,t4);

	if(framesLoaded)
	{
		//Save current frame, this may change during rendering
		uint32_t curFP=state.FP;
		assert_and_throw(curFP<framesLoaded);
		frames[curFP].Render(maskEnabled);
	}

	renderEpilogue();
}

InteractiveObject* MovieClip::hitTest(InteractiveObject*, number_t x, number_t y)
{
	//NOTE: in hitTest the stuff must be tested in the opposite order of Rendering

	//TODO: TOLOCK
	//First of all firter using the BBOX
	number_t t1,t2,t3,t4;
	bool notEmpty=boundsRect(t1,t2,t3,t4);
	if(!notEmpty)
		return NULL;
	if(x<t1 || x>t2 || y<t3 || y>t4)
		return NULL;

	hitTestPrologue();

	InteractiveObject* ret=NULL;
	if(framesLoaded)
	{
		uint32_t curFP=state.FP;
		assert_and_throw(curFP<framesLoaded);
		list<pair<PlaceInfo, DisplayObject*> >::const_iterator it=frames[curFP].displayList.begin();
		for(;it!=frames[curFP].displayList.end();++it)
		{
			number_t localX, localY;
			it->second->getMatrix().getInverted().multiply2D(x,y,localX,localY);
			ret=it->second->hitTest(this, localX,localY);
			if(ret)
				break;
		}
	}

	if(ret==NULL)
		ret=Sprite::hitTestImpl(x, y);

	hitTestEpilogue();
	return ret;
}

Vector2 MovieClip::debugRender(FTFont* font, bool deep)
{
	Vector2 ret(0,0);
	if(!deep)
	{
		glColor3f(0.8,0,0);
		font->Render("MovieClip",-1,FTPoint(0,50));
		glBegin(GL_LINE_LOOP);
			glVertex2i(0,0);
			glVertex2i(100,0);
			glVertex2i(100,100);
			glVertex2i(0,100);
		glEnd();
		ret+=Vector2(100,100);
	}
	else
	{
		MatrixApplier ma;
		if(framesLoaded)
		{
			assert_and_throw(state.FP<framesLoaded);
			int curFP=state.FP;
			list<pair<PlaceInfo, DisplayObject*> >::const_iterator it=frames[curFP].displayList.begin();
	
			for(;it!=frames[curFP].displayList.end();++it)
			{
				Vector2 off=it->second->debugRender(font, false);
				glTranslatef(off.x,0,0);
				ret.x+=off.x;
				if(ret.x*20>sys->getFrameSize().Xmax)
				{
					glTranslatef(-ret.x,off.y,0);
					ret.x=0;
				}
			}
		}

		{
			Locker l(mutexDisplayList);
			/*list<DisplayObject*>::iterator j=dynamicDisplayList.begin();
			for(;j!=dynamicDisplayList.end();++j)
				(*j)->Render();*/
			assert_and_throw(dynamicDisplayList.empty());
		}

		ma.unapply();
	}
	
	return ret;
}

bool MovieClip::boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const
{
	bool valid=Sprite::boundsRect(xmin,xmax,ymin,ymax);
	
	if(framesLoaded==0) //We end here
		return valid;

	//Iterate over the displaylist of the current frame
	uint32_t curFP=state.FP;
	std::list<std::pair<PlaceInfo, DisplayObject*> >::const_iterator it=frames[curFP].displayList.begin();
	
	//Update bounds for all the elements
	for(;it!=frames[curFP].displayList.end();++it)
	{
		number_t t1,t2,t3,t4;
		if(it->second->getBounds(t1,t2,t3,t4))
		{
			if(valid==false)
			{
				xmin=t1;
				xmax=t2;
				ymin=t3;
				ymax=t4;
				valid=true;
				//Now values are valid
				continue;
			}
			else
			{
				xmin=imin(xmin,t1);
				xmax=imax(xmax,t2);
				ymin=imin(ymin,t3);
				ymax=imax(ymax,t4);
			}
			break;
		}
	}
	return valid;
}

bool MovieClip::getBounds(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const
{
	bool ret=boundsRect(xmin,xmax,ymin,ymax);
	if(ret)
	{
		//TODO: take rotation into account
		getMatrix().multiply2D(xmin,ymin,xmin,ymin);
		getMatrix().multiply2D(xmax,ymax,xmax,ymax);
	}
	return ret;
}

DisplayObject::DisplayObject():useMatrix(true),tx(0),ty(0),rotation(0),sx(1),sy(1),maskOf(NULL),parent(NULL),mask(NULL),onStage(false),
	root(NULL),loaderInfo(NULL),alpha(1.0),visible(true),invalidateQueueNext(NULL)
{
}

DisplayObject::DisplayObject(const DisplayObject& d):useMatrix(true),tx(d.tx),ty(d.ty),rotation(d.rotation),sx(d.sx),sy(d.sy),maskOf(NULL),
	parent(NULL),mask(NULL),onStage(false),root(NULL),loaderInfo(NULL),alpha(d.alpha),visible(d.visible),invalidateQueueNext(NULL)
{
}

DisplayObject::~DisplayObject()
{
	if(loaderInfo && !sys->finalizingDestruction)
		loaderInfo->decRef();
}

void DisplayObject::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->super=Class<EventDispatcher>::getClass();
	c->max_level=c->super->max_level+1;
	c->setGetterByQName("loaderInfo","",Class<IFunction>::getFunction(_getLoaderInfo),true);
	c->setGetterByQName("width","",Class<IFunction>::getFunction(_getWidth),true);
	c->setSetterByQName("width","",Class<IFunction>::getFunction(_setWidth),true);
	c->setGetterByQName("scaleX","",Class<IFunction>::getFunction(_getScaleX),true);
	c->setSetterByQName("scaleX","",Class<IFunction>::getFunction(_setScaleX),true);
	c->setGetterByQName("scaleY","",Class<IFunction>::getFunction(_getScaleY),true);
	c->setSetterByQName("scaleY","",Class<IFunction>::getFunction(_setScaleY),true);
	c->setGetterByQName("x","",Class<IFunction>::getFunction(_getX),true);
	c->setSetterByQName("x","",Class<IFunction>::getFunction(_setX),true);
	c->setGetterByQName("y","",Class<IFunction>::getFunction(_getY),true);
	c->setSetterByQName("y","",Class<IFunction>::getFunction(_setY),true);
	c->setGetterByQName("height","",Class<IFunction>::getFunction(_getHeight),true);
	c->setSetterByQName("height","",Class<IFunction>::getFunction(_setHeight),true);
	c->setGetterByQName("visible","",Class<IFunction>::getFunction(_getVisible),true);
	c->setSetterByQName("visible","",Class<IFunction>::getFunction(_setVisible),true);
	c->setGetterByQName("rotation","",Class<IFunction>::getFunction(_getRotation),true);
	c->setSetterByQName("rotation","",Class<IFunction>::getFunction(_setRotation),true);
	c->setGetterByQName("name","",Class<IFunction>::getFunction(_getName),true);
	c->setSetterByQName("name","",Class<IFunction>::getFunction(_setName),true);
	c->setGetterByQName("parent","",Class<IFunction>::getFunction(_getParent),true);
	c->setGetterByQName("root","",Class<IFunction>::getFunction(_getRoot),true);
	c->setGetterByQName("blendMode","",Class<IFunction>::getFunction(_getBlendMode),true);
	c->setSetterByQName("blendMode","",Class<IFunction>::getFunction(undefinedFunction),true);
	c->setGetterByQName("scale9Grid","",Class<IFunction>::getFunction(_getScale9Grid),true);
	c->setSetterByQName("scale9Grid","",Class<IFunction>::getFunction(undefinedFunction),true);
	c->setGetterByQName("stage","",Class<IFunction>::getFunction(_getStage),true);
	c->setGetterByQName("mask","",Class<IFunction>::getFunction(_getMask),true);
	c->setSetterByQName("mask","",Class<IFunction>::getFunction(_setMask),true);
	c->setGetterByQName("alpha","",Class<IFunction>::getFunction(_getAlpha),true);
	c->setSetterByQName("alpha","",Class<IFunction>::getFunction(_setAlpha),true);
	c->setGetterByQName("cacheAsBitmap","",Class<IFunction>::getFunction(undefinedFunction),true);
	c->setSetterByQName("cacheAsBitmap","",Class<IFunction>::getFunction(undefinedFunction),true);
	c->setGetterByQName("opaqueBackground","",Class<IFunction>::getFunction(undefinedFunction),true);
	c->setSetterByQName("opaqueBackground","",Class<IFunction>::getFunction(undefinedFunction),true);
	c->setMethodByQName("getBounds","",Class<IFunction>::getFunction(_getBounds),true);
	c->setMethodByQName("localToGlobal","",Class<IFunction>::getFunction(localToGlobal),true);
}

void DisplayObject::buildTraits(ASObject* o)
{
}

void DisplayObject::setMatrix(const lightspark::MATRIX& m)
{
	if(ACQUIRE_READ(useMatrix))
	{
		bool mustInvalidate=false;
		{
			SpinlockLocker locker(spinlock);
			if(Matrix!=m)
			{
				Matrix=m;
				mustInvalidate=true;
			}
		}
		if(mustInvalidate && onStage)
			requestInvalidation();
	}
}

void DisplayObject::becomeMaskOf(DisplayObject* m)
{
	assert_and_throw(mask==NULL);
	if(m)
		m->incRef();
	DisplayObject* tmp=maskOf;
	maskOf=m;
	if(tmp)
	{
		//We are changing owner
		tmp->setMask(NULL);
		tmp->decRef();
	}
}

void DisplayObject::setMask(DisplayObject* m)
{
	bool mustInvalidate=(mask!=m) && onStage;
	if(m)
		m->incRef();
	DisplayObject* tmp=mask;
	mask=m;
	if(tmp)
	{
		//Drop the previous mask
		tmp->becomeMaskOf(NULL);
		tmp->decRef();
	}
	if(mustInvalidate && onStage)
		requestInvalidation();
}

MATRIX DisplayObject::getConcatenatedMatrix() const
{
	if(parent)
		return parent->getConcatenatedMatrix().multiplyMatrix(getMatrix());
	else
		return getMatrix();
}

MATRIX DisplayObject::getMatrix() const
{
	MATRIX ret;
	if(ACQUIRE_READ(useMatrix))
	{
		SpinlockLocker locker(spinlock);
		ret=Matrix;
	}
	else
	{
		ret.TranslateX=tx;
		ret.TranslateY=ty;
		ret.ScaleX=sx*cos(rotation*M_PI/180);
		ret.RotateSkew1=-sx*sin(rotation*M_PI/180);
		ret.RotateSkew0=sy*sin(rotation*M_PI/180);
		ret.ScaleY=sy*cos(rotation*M_PI/180);
	}
	return ret;
}

void DisplayObject::valFromMatrix()
{
	assert(useMatrix);
	SpinlockLocker locker(spinlock);
	tx=Matrix.TranslateX;
	ty=Matrix.TranslateY;
	sx=Matrix.ScaleX;
	sy=Matrix.ScaleY;
}

bool DisplayObject::isSimple() const
{
	//TODO: Check filters
	return alpha==1.0;
}

bool DisplayObject::skipRender(bool maskEnabled) const
{
	return visible==false || alpha==0.0 || (!maskEnabled && maskOf!=NULL);
}

void DisplayObject::defaultRender(bool maskEnabled) const
{
	//TODO: TOLOCK
	if(!cachedSurface.tex.isValid())
		return;
	float enableMaskLookup=0.0f;
	//If the maskEnabled is already set we are the mask!
	if(!maskEnabled && rt->isMaskPresent())
	{
		rt->renderMaskToTmpBuffer();
		enableMaskLookup=1.0f;
		glColor4f(1,0,0,0);
		glBegin(GL_QUADS);
			glVertex2i(-1000,-1000);
			glVertex2i(1000,-1000);
			glVertex2i(1000,1000);
			glVertex2i(-1000,1000);
		glEnd();
	}
	glPushMatrix();
	glLoadIdentity();
	glColor4f(enableMaskLookup,0,1,0);
	rt->renderTextured(cachedSurface.tex, cachedSurface.xOffset, cachedSurface.yOffset, cachedSurface.tex.width, cachedSurface.tex.height);
	glPopMatrix();
}

void DisplayObject::computeDeviceBoundsForRect(number_t xmin, number_t xmax, number_t ymin, number_t ymax,
		uint32_t& outXMin, uint32_t& outYMin, uint32_t& outWidth, uint32_t& outHeight) const
{
	//As the transformation is arbitrary we have to check all the four vertices
	number_t coords[8];
	localToGlobal(xmin,ymin,coords[0],coords[1]);
	localToGlobal(xmin,ymax,coords[2],coords[3]);
	localToGlobal(xmax,ymax,coords[4],coords[5]);
	localToGlobal(xmax,ymin,coords[6],coords[7]);
	//Now find out the minimum and maximum that represent the complete bounding rect
	number_t minx=coords[6];
	number_t maxx=coords[6];
	number_t miny=coords[7];
	number_t maxy=coords[7];
	for(int i=0;i<6;i+=2)
	{
		if(coords[i]<minx)
			minx=coords[i];
		else if(coords[i]>maxx)
			maxx=coords[i];
		if(coords[i+1]<miny)
			miny=coords[i+1];
		else if(coords[i+1]>maxy)
			maxy=coords[i+1];
	}
	outXMin=minx;
	outYMin=miny;
	outWidth=ceil(maxx-minx);
	outHeight=ceil(maxy-miny);
}

void DisplayObject::invalidate()
{
	//Not supposed to be called
	throw RunTimeException("DisplayObject::invalidate");
}

void DisplayObject::requestInvalidation()
{
	//Let's invalidate also the mask
	if(mask)
		mask->requestInvalidation();
}

void DisplayObject::localToGlobal(number_t xin, number_t yin, number_t& xout, number_t& yout) const
{
	getMatrix().multiply2D(xin, yin, xout, yout);
	if(parent)
		parent->localToGlobal(xout, yout, xout, yout);
}

void DisplayObject::setRoot(RootMovieClip* r)
{
	root=r;
}

void DisplayObject::setOnStage(bool staged)
{
	//TODO: When removing from stage released the cachedTex
	if(staged!=onStage)
	{
		//Our stage condition changed, send event
		onStage=staged;
		if(staged==true)
			requestInvalidation();
		if(getVm()==NULL)
			return;
		if(onStage==true && hasEventListener("addedToStage"))
		{
			Event* e=Class<Event>::getInstanceS("addedToStage");
			getVm()->addEvent(this,e);
			e->decRef();
		}
		else if(onStage==false && hasEventListener("removedFromStage"))
		{
			Event* e=Class<Event>::getInstanceS("removedFromStage");
			getVm()->addEvent(this,e);
			e->decRef();
		}
	}
}

ASFUNCTIONBODY(DisplayObject,_setAlpha)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	assert_and_throw(argslen==1);
	number_t val=args[0]->toNumber();
	//Clip val
	val=dmax(0,val);
	val=dmin(val,1);
	th->alpha=val;
	return NULL;
}

ASFUNCTIONBODY(DisplayObject,_getAlpha)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	return abstract_d(th->alpha);
}

ASFUNCTIONBODY(DisplayObject,_getMask)
{
	DisplayObject* th=Class<DisplayObject>::cast(obj);
	if(th->mask==NULL)
		return new Null;

	th->mask->incRef();
	return th->mask;
}

ASFUNCTIONBODY(DisplayObject,_setMask)
{
	DisplayObject* th=Class<DisplayObject>::cast(obj);
	assert_and_throw(argslen==1);
	if(args[0] && args[0]->getPrototype() && args[0]->getPrototype()->isSubClass(Class<DisplayObject>::getClass()))
	{
		//We received a valid mask object
		DisplayObject* newMask=Class<DisplayObject>::cast(args[0]);
		newMask->becomeMaskOf(th);
		th->setMask(newMask);
	}
	else
		th->setMask(NULL);

	return NULL;
}

ASFUNCTIONBODY(DisplayObject,_getScaleX)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	if(ACQUIRE_READ(th->useMatrix))
		return abstract_d(th->Matrix.ScaleX);
	else
		return abstract_d(th->sx);
}

ASFUNCTIONBODY(DisplayObject,_setScaleX)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	assert_and_throw(argslen==1);
	number_t val=args[0]->toNumber();
	if(ACQUIRE_READ(th->useMatrix))
	{
		th->valFromMatrix();
		RELEASE_WRITE(th->useMatrix,false);
	}
	th->sx=val;
	if(th->onStage)
		th->requestInvalidation();
	return NULL;
}

ASFUNCTIONBODY(DisplayObject,_getScaleY)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	if(ACQUIRE_READ(th->useMatrix))
		return abstract_d(th->Matrix.ScaleY);
	else
		return abstract_d(th->sy);
}

ASFUNCTIONBODY(DisplayObject,_setScaleY)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	assert_and_throw(argslen==1);
	number_t val=args[0]->toNumber();
	if(ACQUIRE_READ(th->useMatrix))
	{
		th->valFromMatrix();
		RELEASE_WRITE(th->useMatrix,false);
	}
	th->sy=val;
	if(th->onStage)
		th->requestInvalidation();
	return NULL;
}

ASFUNCTIONBODY(DisplayObject,_getX)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	if(ACQUIRE_READ(th->useMatrix))
		return abstract_d(th->Matrix.TranslateX);
	else
		return abstract_d(th->tx);
}

ASFUNCTIONBODY(DisplayObject,_setX)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	assert_and_throw(argslen==1);
	number_t val=args[0]->toNumber();
	if(ACQUIRE_READ(th->useMatrix))
	{
		th->valFromMatrix();
		RELEASE_WRITE(th->useMatrix,false);
	}
	th->tx=val;
	if(th->onStage)
		th->requestInvalidation();
	return NULL;
}

ASFUNCTIONBODY(DisplayObject,_getY)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	if(ACQUIRE_READ(th->useMatrix))
		return abstract_d(th->Matrix.TranslateY);
	else
		return abstract_d(th->ty);
}

ASFUNCTIONBODY(DisplayObject,_setY)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	assert_and_throw(argslen==1);
	number_t val=args[0]->toNumber();
	if(ACQUIRE_READ(th->useMatrix))
	{
		th->valFromMatrix();
		RELEASE_WRITE(th->useMatrix,false);
	}
	th->ty=val;
	if(th->onStage)
		th->requestInvalidation();
	return NULL;
}

ASFUNCTIONBODY(DisplayObject,_getBounds)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	assert_and_throw(argslen==1);

	Rectangle* ret=Class<Rectangle>::getInstanceS();
	number_t x1,x2,y1,y2;
	bool r=th->getBounds(x1,x2,y1,y2);
	if(r)
	{
		//Bounds are in the form [XY]{min,max}
		//convert it to rect (x,y,width,height) representation
		ret->x=x1;
		ret->width=x2-x1;
		ret->y=y1;
		ret->height=y2-y1;
	}
	else
	{
		ret->x=0;
		ret->width=0;
		ret->y=0;
		ret->height=0;
	}
	return ret;
}

ASFUNCTIONBODY(DisplayObject,_constructor)
{
	//DisplayObject* th=static_cast<DisplayObject*>(obj->implementation);
	EventDispatcher::_constructor(obj,NULL,0);

	return NULL;
}

ASFUNCTIONBODY(DisplayObject,_getLoaderInfo)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	if(th->loaderInfo)
	{
		th->loaderInfo->incRef();
		return th->loaderInfo;
	}
	else
		return new Undefined;
}

ASFUNCTIONBODY(DisplayObject,_getStage)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	if(th->onStage)
	{
		assert(sys->stage);
		sys->stage->incRef();
		return sys->stage;
	}
	else
		return new Null;
}

ASFUNCTIONBODY(DisplayObject,_getScale9Grid)
{
	//DisplayObject* th=static_cast<DisplayObject*>(obj);
	return new Undefined;
}

ASFUNCTIONBODY(DisplayObject,_getBlendMode)
{
	//DisplayObject* th=static_cast<DisplayObject*>(obj);
	return new Undefined;
}

ASFUNCTIONBODY(DisplayObject,localToGlobal)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	assert_and_throw(argslen == 1);

	Point* pt=static_cast<Point*>(args[0]);

	number_t tempx, tempy;

	th->getMatrix().multiply2D(pt->getX(), pt->getY(), tempx, tempy);

	return Class<Point>::getInstanceS(tempx, tempy);
}

ASFUNCTIONBODY(DisplayObject,_setRotation)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	assert_and_throw(argslen==1);
	number_t val=args[0]->toNumber();
	if(ACQUIRE_READ(th->useMatrix))
	{
		th->valFromMatrix();
		RELEASE_WRITE(th->useMatrix,false);
	}
	th->rotation=val;
	if(th->onStage)
		th->requestInvalidation();
	return NULL;
}

ASFUNCTIONBODY(DisplayObject,_setName)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	assert_and_throw(argslen==1);
	th->name=args[0]->toString();
	return NULL;
}

ASFUNCTIONBODY(DisplayObject,_getName)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	return Class<ASString>::getInstanceS(th->name);
}

void DisplayObject::setParent(DisplayObjectContainer* p)
{
	if(parent!=p)
	{
		parent=p;
		requestInvalidation();
	}
}

ASFUNCTIONBODY(DisplayObject,_getParent)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	if(th->parent==NULL)
		return new Undefined;

	th->parent->incRef();
	return th->parent;
}

ASFUNCTIONBODY(DisplayObject,_getRoot)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	if(th->root)
	{
		th->root->incRef();
		return th->root;
	}
	else
		return new Undefined;
}

ASFUNCTIONBODY(DisplayObject,_getRotation)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	//There is no easy way to get rotation from matrix, let's ignore the matrix
	if(ACQUIRE_READ(th->useMatrix))
	{
		th->valFromMatrix();
		RELEASE_WRITE(th->useMatrix,false);
	}
	return abstract_d(th->rotation);
}

ASFUNCTIONBODY(DisplayObject,_setVisible)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	assert_and_throw(argslen==1);
	th->visible=Boolean_concrete(args[0]);
	return NULL;
}

ASFUNCTIONBODY(DisplayObject,_getVisible)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	return abstract_b(th->visible);
}

int DisplayObject::computeHeight()
{
	number_t x1,x2,y1,y2;
	bool ret=getBounds(x1,x2,y1,y2);

	return (ret)?(y2-y1):0;
}

int DisplayObject::computeWidth()
{
	number_t x1,x2,y1,y2;
	bool ret=getBounds(x1,x2,y1,y2);

	return (ret)?(x2-x1):0;
}

ASFUNCTIONBODY(DisplayObject,_getWidth)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	int ret=th->computeWidth();
	return abstract_i(ret);
}

ASFUNCTIONBODY(DisplayObject,_setWidth)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	int newwidth=args[0]->toInt();
	//Should actually scale the object
	int computed=th->computeWidth();
	if(computed==0) //Cannot scale, nothing to do (See Reference)
		return NULL;
	
	if(computed!=newwidth) //If the width is changing, calculate new scale
	{
		number_t newscale=newwidth;
		newscale/=computed;
		if(ACQUIRE_READ(th->useMatrix))
		{
			th->valFromMatrix();
			RELEASE_WRITE(th->useMatrix,false);
		}
		th->sx=newscale;
	}
	if(th->onStage)
		th->requestInvalidation();
	return NULL;
}

ASFUNCTIONBODY(DisplayObject,_getHeight)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	int ret=th->computeHeight();;
	return abstract_i(ret);
}

ASFUNCTIONBODY(DisplayObject,_setHeight)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	int newheight=args[0]->toInt();
	//Should actually scale the object
	int computed=th->computeHeight();
	if(computed==0) //Cannot scale, nothing to do (See Reference)
		return NULL;
	
	if(computed!=newheight) //If the height is changing, calculate new scale
	{
		number_t newscale=newheight;
		newscale/=computed;
		if(ACQUIRE_READ(th->useMatrix))
		{
			th->valFromMatrix();
			RELEASE_WRITE(th->useMatrix,false);
		}
		th->sy=newscale;
	}
	if(th->onStage)
		th->requestInvalidation();
	return NULL;
}

void DisplayObjectContainer::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->super=Class<InteractiveObject>::getClass();
	c->max_level=c->super->max_level+1;
	c->setGetterByQName("numChildren","",Class<IFunction>::getFunction(_getNumChildren),true);
	c->setMethodByQName("getChildIndex","",Class<IFunction>::getFunction(getChildIndex),true);
	c->setMethodByQName("getChildAt","",Class<IFunction>::getFunction(getChildAt),true);
	c->setMethodByQName("getChildByName","",Class<IFunction>::getFunction(getChildByName),true);
	c->setMethodByQName("addChild","",Class<IFunction>::getFunction(addChild),true);
	c->setMethodByQName("removeChild","",Class<IFunction>::getFunction(removeChild),true);
	c->setMethodByQName("removeChildAt","",Class<IFunction>::getFunction(removeChildAt),true);
	c->setMethodByQName("addChildAt","",Class<IFunction>::getFunction(addChildAt),true);
	c->setMethodByQName("contains","",Class<IFunction>::getFunction(contains),true);
	//c->setSetterByQName("mouseChildren","",Class<IFunction>::getFunction(undefinedFunction));
	//c->setGetterByQName("mouseChildren","",Class<IFunction>::getFunction(undefinedFunction));
}

void DisplayObjectContainer::buildTraits(ASObject* o)
{
}

DisplayObjectContainer::DisplayObjectContainer():mutexDisplayList("mutexDisplayList")
{
}

DisplayObjectContainer::~DisplayObjectContainer()
{
	//Release every child
	if(!sys->finalizingDestruction)
	{
		list<DisplayObject*>::iterator it=dynamicDisplayList.begin();
		for(;it!=dynamicDisplayList.end();++it)
			(*it)->decRef();
	}
}

InteractiveObject::InteractiveObject():mouseEnabled(true)
{
}

InteractiveObject::~InteractiveObject()
{
	if(sys && sys->getInputThread())
		sys->getInputThread()->removeListener(this);
}

ASFUNCTIONBODY(InteractiveObject,_constructor)
{
	InteractiveObject* th=static_cast<InteractiveObject*>(obj);
	EventDispatcher::_constructor(obj,NULL,0);
	//Object registered very early are not supported this way (Stage for example)
	if(sys && sys->getInputThread())
		sys->getInputThread()->addListener(th);
	
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

void InteractiveObject::buildTraits(ASObject* o)
{
}

void InteractiveObject::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->super=Class<DisplayObject>::getClass();
	c->max_level=c->super->max_level+1;
	c->setSetterByQName("mouseEnabled","",Class<IFunction>::getFunction(_setMouseEnabled),true);
	c->setGetterByQName("mouseEnabled","",Class<IFunction>::getFunction(_getMouseEnabled),true);
}

void DisplayObjectContainer::dumpDisplayList()
{
	cout << "Size: " << dynamicDisplayList.size() << endl;
	list<DisplayObject*>::const_iterator it=dynamicDisplayList.begin();
	for(;it!=dynamicDisplayList.end();++it)
	{
		if(*it)
			cout << (*it)->getPrototype()->class_name << endl;
		else
			cout << "UNKNOWN" << endl;
	}
}

//This must be called fromt VM context
void DisplayObjectContainer::setRoot(RootMovieClip* r)
{
	if(r!=root)
	{
		DisplayObject::setRoot(r);
		list<DisplayObject*>::const_iterator it=dynamicDisplayList.begin();
		for(;it!=dynamicDisplayList.end();++it)
			(*it)->setRoot(r);
	}
}

void DisplayObjectContainer::setOnStage(bool staged)
{
	if(staged!=onStage)
	{
		DisplayObject::setOnStage(staged);
		//Notify childern
		Locker l(mutexDisplayList);
		list<DisplayObject*>::const_iterator it=dynamicDisplayList.begin();
		for(;it!=dynamicDisplayList.end();++it)
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

void DisplayObjectContainer::requestInvalidation()
{
	DisplayObject::requestInvalidation();
	Locker l(mutexDisplayList);
	list<DisplayObject*>::const_iterator it=dynamicDisplayList.begin();
	for(;it!=dynamicDisplayList.end();it++)
		(*it)->requestInvalidation();
}

void DisplayObjectContainer::_addChildAt(DisplayObject* child, unsigned int index)
{
	//If the child has no parent, set this container to parent
	//If there is a previous parent, purge the child from his list
	if(child->getParent())
	{
		//Child already in this container
		if(child->getParent()==this)
			return;
		else
			child->getParent()->_removeChild(child);
	}
	child->setParent(this);
	incRef();

	//Set the root of the movie to this container
	child->setRoot(root);

	{
		Locker l(mutexDisplayList);
		//We insert the object in the back of the list
		if(index==numeric_limits<unsigned int>::max())
			dynamicDisplayList.push_back(child);
		else
		{
			assert_and_throw(index<=dynamicDisplayList.size());
			list<DisplayObject*>::iterator it=dynamicDisplayList.begin();
			for(unsigned int i=0;i<index;i++)
				++it;
			dynamicDisplayList.insert(it,child);
		}
		//We acquire a reference to the child
		child->incRef();
	}
	child->setOnStage(onStage);
}

bool DisplayObjectContainer::_removeChild(DisplayObject* child)
{
	if(child->getParent()==NULL)
		return false;
	assert_and_throw(child->getParent()==this);
	assert_and_throw(child->getRoot()==root);

	{
		Locker l(mutexDisplayList);
		list<DisplayObject*>::iterator it=find(dynamicDisplayList.begin(),dynamicDisplayList.end(),child);
		if(it==dynamicDisplayList.end())
			return false;
		dynamicDisplayList.erase(it);
	}
	//Set the root of the movie to NULL
	child->setRoot(NULL);
	//We can release the reference to the child
	child->getParent()->decRef();
	child->setParent(NULL);
	child->setOnStage(false);
	child->decRef();
	return true;
}

bool DisplayObjectContainer::_contains(DisplayObject* d)
{
	if(d==this)
		return true;

	list<DisplayObject*>::const_iterator it=dynamicDisplayList.begin();
	for(;it!=dynamicDisplayList.end();++it)
	{
		if(*it==d)
			return true;
		DisplayObjectContainer* c=dynamic_cast<DisplayObjectContainer*>(*it);
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
	assert_and_throw(args[0] && args[0]->getPrototype() && 
		args[0]->getPrototype()->isSubClass(Class<DisplayObject>::getClass()));

	//Cast to object
	DisplayObject* d=static_cast<DisplayObject*>(args[0]);

	bool ret=th->_contains(d);
	return abstract_b(ret);
}

//Only from VM context
ASFUNCTIONBODY(DisplayObjectContainer,addChildAt)
{
	DisplayObjectContainer* th=static_cast<DisplayObjectContainer*>(obj);
	assert_and_throw(argslen==2);
	if(args[0]->getObjectType() == T_CLASS)
	{
		return new Null;
	}
	//Validate object type
	assert_and_throw(args[0] && args[0]->getPrototype() && 
		args[0]->getPrototype()->isSubClass(Class<DisplayObject>::getClass()));

	int index=args[1]->toInt();

	//Cast to object
	DisplayObject* d=Class<DisplayObject>::cast(args[0]);
	th->_addChildAt(d,index);

	//Notify the object
	sys->currentVm->addEvent(d,Class<Event>::getInstanceS("added"));

	//incRef again as the value is getting returned
	d->incRef();
	return d;
}

ASFUNCTIONBODY(DisplayObjectContainer,addChild)
{
	DisplayObjectContainer* th=static_cast<DisplayObjectContainer*>(obj);
	assert_and_throw(argslen==1);
	if(args[0]->getObjectType() == T_CLASS)
	{
		return new Null;
	}
	//Validate object type
	assert_and_throw(args[0] && args[0]->getPrototype() && 
		args[0]->getPrototype()->isSubClass(Class<DisplayObject>::getClass()));

	//Cast to object
	DisplayObject* d=Class<DisplayObject>::cast(args[0]);
	th->_addChildAt(d,numeric_limits<unsigned int>::max());

	//Notify the object
	sys->currentVm->addEvent(d,Class<Event>::getInstanceS("added"));

	d->incRef();
	return d;
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
		return new Null;
	}
	//Validate object type
	assert_and_throw(args[0] && args[0]->getPrototype() && 
		args[0]->getPrototype()->isSubClass(Class<DisplayObject>::getClass()));
	//Cast to object
	DisplayObject* d=Class<DisplayObject>::cast(args[0]);

	if(!th->_removeChild(d))
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
		list<DisplayObject*>::iterator it=th->dynamicDisplayList.begin();
		for(int32_t i=0;i<index;i++)
			++it;
		child=*it;
		th->dynamicDisplayList.erase(it);
	}
	//We can release the reference to the child
	child->getParent()->decRef();
	child->setParent(NULL);
	child->setOnStage(false);

	//As we return the child we don't decRef it
	return child;
}

//Only from VM context
ASFUNCTIONBODY(DisplayObjectContainer,getChildByName)
{
	DisplayObjectContainer* th=static_cast<DisplayObjectContainer*>(obj);
	assert_and_throw(argslen==1);
	const tiny_string& wantedName=args[0]->toString();
	list<DisplayObject*>::iterator it=th->dynamicDisplayList.begin();
	ASObject* ret=NULL;
	for(;it!=th->dynamicDisplayList.end();++it)
	{
		if((*it)->name==wantedName)
		{
			ret=*it;
			break;
		}
	}
	if(ret)
		ret->incRef();
	else
		ret=new Undefined;
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
	list<DisplayObject*>::iterator it=th->dynamicDisplayList.begin();
	for(unsigned int i=0;i<index;i++)
		++it;

	(*it)->incRef();
	return *it;
}

//Only from VM context
ASFUNCTIONBODY(DisplayObjectContainer,getChildIndex)
{
	DisplayObjectContainer* th=static_cast<DisplayObjectContainer*>(obj);
	assert_and_throw(argslen==1);
	//Validate object type
	assert_and_throw(args[0]->getPrototype()->isSubClass(Class<DisplayObject>::getClass()));

	//Cast to object
	DisplayObject* d=static_cast<DisplayObject*>(args[0]);

	list<DisplayObject*>::const_iterator it=th->dynamicDisplayList.begin();
	int ret=0;
	do
	{
		if(*it==d)
			break;
		
		ret++;
		++it;
		if(it==th->dynamicDisplayList.end())   
			throw Class<ArgumentError>::getInstanceS("getChildIndex: child not in list");
	}
	while(1);
	return abstract_i(ret);
}

void Shape::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->super=Class<DisplayObject>::getClass();
	c->max_level=c->super->max_level+1;
	c->setGetterByQName("graphics","",Class<IFunction>::getFunction(_getGraphics),true);
}

void Shape::buildTraits(ASObject* o)
{
}

bool Shape::getBounds(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const
{
	if(graphics)
	{
		bool ret=graphics->getBounds(xmin,xmax,ymin,ymax);
		if(ret)
		{
			getMatrix().multiply2D(xmin,ymin,xmin,ymin);
			getMatrix().multiply2D(xmax,ymax,xmax,ymax);
			return true;
		}
	}
	return false;
}


bool Shape::isOpaque(number_t x, number_t y) const
{
	LOG(LOG_NOT_IMPLEMENTED,"Shape::isOpaque not really implemented");
	return false;
}

InteractiveObject* Shape::hitTest(InteractiveObject* last, number_t x, number_t y)
{
	//NOTE: in hitTest the stuff must be rendered in the opposite order of Rendering
	assert_and_throw(!sys->getInputThread()->isMaskPresent());
	//TODO: TOLOCK
	if(mask)
	{
		LOG(LOG_NOT_IMPLEMENTED,"Support masks in Shape::hitTest");
		::abort();
	}

	if(graphics)
	{
		//The coordinates are already local
		if(graphics->hitTest(x,y))
			return last;
	}

	return NULL;
}

void Shape::renderImpl(bool maskEnabled, number_t t1, number_t t2, number_t t3, number_t t4) const
{
	renderPrologue();

	if(!isSimple())
		rt->glAcquireTempBuffer(t1,t2,t3,t4);

	defaultRender(maskEnabled);

	if(!isSimple())
		rt->glBlitTempBuffer(t1,t2,t3,t4);
	renderEpilogue();
}

void Shape::Render(bool maskEnabled)
{
	//If graphics is not yet initialized we have nothing to do
	if(graphics==NULL || skipRender(maskEnabled))
		return;

	number_t t1,t2,t3,t4;
	bool ret=graphics->getBounds(t1,t2,t3,t4);
	if(!ret)
		return;
	Shape::renderImpl(maskEnabled, t1, t2, t3, t4);
}

const vector<GeomToken>& Shape::getTokens()
{
	return cachedTokens;
}

float Shape::getScaleFactor() const
{
	return 1;
}

void Shape::invalidate()
{
	assert(graphics);
	invalidateGraphics();
	cachedTokens=graphics->getGraphicsTokens();
}

void Shape::requestInvalidation()
{
	if(graphics)
		sys->addToInvalidateQueue(this);
}

ASFUNCTIONBODY(Shape,_constructor)
{
	DisplayObject::_constructor(obj,NULL,0);
	return NULL;
}

ASFUNCTIONBODY(Shape,_getGraphics)
{
	Shape* th=static_cast<Shape*>(obj);
	//Probably graphics is not used often, so create it here
	if(th->graphics==NULL)
		th->graphics=Class<Graphics>::getInstanceS(th);

	th->graphics->incRef();
	return th->graphics;
}

void MorphShape::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->super=Class<DisplayObject>::getClass();
	c->max_level=c->super->max_level+1;
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

void Stage::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->super=Class<DisplayObjectContainer>::getClass();
	c->max_level=c->super->max_level+1;
	c->setGetterByQName("stageWidth","",Class<IFunction>::getFunction(_getStageWidth),true);
	c->setGetterByQName("stageHeight","",Class<IFunction>::getFunction(_getStageHeight),true);
	c->setGetterByQName("width","",Class<IFunction>::getFunction(_getStageWidth),true);
	c->setGetterByQName("height","",Class<IFunction>::getFunction(_getStageHeight),true);
	c->setGetterByQName("scaleMode","",Class<IFunction>::getFunction(_getScaleMode),true);
	c->setSetterByQName("scaleMode","",Class<IFunction>::getFunction(_setScaleMode),true);
	c->setGetterByQName("loaderInfo","",Class<IFunction>::getFunction(_getLoaderInfo),true);
}

void Stage::buildTraits(ASObject* o)
{
}

Stage::Stage()
{
}

ASFUNCTIONBODY(Stage,_constructor)
{
	return NULL;
}

uint32_t Stage::internalGetWidth() const
{
	uint32_t width;
	if(sys->scaleMode==SystemState::NO_SCALE && sys->getRenderThread())
		width=sys->getRenderThread()->windowWidth;
	else
	{
		RECT size=sys->getFrameSize();
		width=size.Xmax/20;
	}
	return width;
}

uint32_t Stage::internalGetHeight() const
{
	uint32_t height;
	if(sys->scaleMode==SystemState::NO_SCALE && sys->getRenderThread())
		height=sys->getRenderThread()->windowHeight;
	else
	{
		RECT size=sys->getFrameSize();
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
	return SystemState::_getLoaderInfo(sys,NULL,0);
}

ASFUNCTIONBODY(Stage,_getScaleMode)
{
	//Stage* th=static_cast<Stage*>(obj);
	switch(sys->scaleMode)
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
		sys->scaleMode=SystemState::EXACT_FIT;
	else if(arg0=="showAll")	
		sys->scaleMode=SystemState::SHOW_ALL;
	else if(arg0=="noBorder")	
		sys->scaleMode=SystemState::NO_BORDER;
	else if(arg0=="noScale")	
		sys->scaleMode=SystemState::NO_SCALE;
	
	RenderThread* rt=sys->getRenderThread();
	if(rt)
		rt->requestResize(rt->windowWidth, rt->windowHeight);
	return NULL;
}

void Graphics::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setMethodByQName("clear","",Class<IFunction>::getFunction(clear),true);
	c->setMethodByQName("drawRect","",Class<IFunction>::getFunction(drawRect),true);
	c->setMethodByQName("drawRoundRect","",Class<IFunction>::getFunction(drawRoundRect),true);
	c->setMethodByQName("drawCircle","",Class<IFunction>::getFunction(drawCircle),true);
	c->setMethodByQName("moveTo","",Class<IFunction>::getFunction(moveTo),true);
	c->setMethodByQName("curveTo","",Class<IFunction>::getFunction(curveTo),true);
	c->setMethodByQName("cubicCurveTo","",Class<IFunction>::getFunction(cubicCurveTo),true);
	c->setMethodByQName("lineTo","",Class<IFunction>::getFunction(lineTo),true);
	c->setMethodByQName("lineStyle","",Class<IFunction>::getFunction(lineStyle),true);
	c->setMethodByQName("beginFill","",Class<IFunction>::getFunction(beginFill),true);
	c->setMethodByQName("beginGradientFill","",Class<IFunction>::getFunction(beginGradientFill),true);
	c->setMethodByQName("endFill","",Class<IFunction>::getFunction(endFill),true);
}

void Graphics::buildTraits(ASObject* o)
{
}

vector<GeomToken> Graphics::getGraphicsTokens() const
{
	return tokens;
}

bool Graphics::hitTest(number_t x, number_t y) const
{
	return CairoRenderer::hitTest(tokens, 1, x, y);
}

bool Graphics::getBounds(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const
{

	#define VECTOR_BOUNDS(v) \
		xmin=dmin(v.x-strokeWidth,xmin); \
		xmax=dmax(v.x+strokeWidth,xmax); \
		ymin=dmin(v.y-strokeWidth,ymin); \
		ymax=dmax(v.y+strokeWidth,ymax);

	if(tokens.size()==0)
		return false;

	xmin = numeric_limits<double>::infinity();
	ymin = numeric_limits<double>::infinity();
	xmax = -numeric_limits<double>::infinity();
	ymax = -numeric_limits<double>::infinity();

	bool hasContent = false;
	double strokeWidth = 0;

	for(unsigned int i=0;i<tokens.size();i++)
	{
		switch(tokens[i].type)
		{
			case CURVE_CUBIC:
			{
				VECTOR_BOUNDS(tokens[i].p3);
				// fall through
			}
			case CURVE_QUADRATIC:
			{
				VECTOR_BOUNDS(tokens[i].p2);
				// fall through
			}
			case STRAIGHT:
			{
				hasContent = true;
				// fall through
			}
			case MOVE:
			{
				VECTOR_BOUNDS(tokens[i].p1);
				break;
			}
			case CLEAR_FILL:
			case CLEAR_STROKE:
			case SET_FILL:
				break;
			case SET_STROKE:
				strokeWidth = (double)(tokens[i].lineStyle.Width / 20.0);
				break;
		}
	}
	return hasContent;

#undef VECTOR_BOUNDS
}

ASFUNCTIONBODY(Graphics,_constructor)
{
	return NULL;
}

ASFUNCTIONBODY(Graphics,clear)
{
	Graphics* th=static_cast<Graphics*>(obj);
	th->tokens.clear();
	return NULL;
}

ASFUNCTIONBODY(Graphics,moveTo)
{
	Graphics* th=static_cast<Graphics*>(obj);
	assert_and_throw(argslen==2);

	th->curX=args[0]->toInt();
	th->curY=args[1]->toInt();

	th->tokens.emplace_back(MOVE, Vector2(th->curX, th->curY));
	return NULL;
}

ASFUNCTIONBODY(Graphics,lineTo)
{
	Graphics* th=static_cast<Graphics*>(obj);
	assert_and_throw(argslen==2);

	int x=args[0]->toInt();
	int y=args[1]->toInt();

	th->tokens.emplace_back(STRAIGHT, Vector2(x, y));
	th->owner->owner->requestInvalidation();

	th->curX=x;
	th->curY=y;
	return NULL;
}

ASFUNCTIONBODY(Graphics,curveTo)
{
	Graphics* th=static_cast<Graphics*>(obj);
	assert_and_throw(argslen==4);

	int controlX=args[0]->toInt();
	int controlY=args[1]->toInt();

	int anchorX=args[2]->toInt();
	int anchorY=args[3]->toInt();

	th->tokens.emplace_back(CURVE_QUADRATIC,
	                        Vector2(controlX, controlY),
	                        Vector2(anchorX, anchorY));
	th->owner->owner->requestInvalidation();

	th->curX=anchorX;
	th->curY=anchorY;
	return NULL;
}

ASFUNCTIONBODY(Graphics,cubicCurveTo)
{
	Graphics* th=static_cast<Graphics*>(obj);
	assert_and_throw(argslen==6);

	int control1X=args[0]->toInt();
	int control1Y=args[1]->toInt();

	int control2X=args[2]->toInt();
	int control2Y=args[3]->toInt();

	int anchorX=args[4]->toInt();
	int anchorY=args[5]->toInt();

	th->tokens.emplace_back(CURVE_CUBIC,
	                        Vector2(control1X, control1Y),
	                        Vector2(control2X, control2Y),
	                        Vector2(anchorX, anchorY));
	th->owner->owner->requestInvalidation();

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
	th->tokens.emplace_back(MOVE, Vector2(x+width, y+height-ellipseHeight));

	// D -> E
	th->tokens.emplace_back(CURVE_CUBIC,
	                        Vector2(x+width, y+height-ellipseHeight+kappaH),
	                        Vector2(x+width-ellipseWidth+kappaW, y+height),
	                        Vector2(x+width-ellipseWidth, y+height));

	// E -> F
	th->tokens.emplace_back(STRAIGHT, Vector2(x+ellipseWidth, y+height));

	// F -> G
	th->tokens.emplace_back(CURVE_CUBIC,
	                        Vector2(x+ellipseWidth-kappaW, y+height),
	                        Vector2(x, y+height-kappaH),
	                        Vector2(x, y+height-ellipseHeight));

	// G -> H
	th->tokens.emplace_back(STRAIGHT, Vector2(x, y+ellipseHeight));

	// H -> A
	th->tokens.emplace_back(CURVE_CUBIC,
	                        Vector2(x, y+ellipseHeight-kappaH),
	                        Vector2(x+ellipseWidth-kappaW, y),
	                        Vector2(x+ellipseWidth, y));

	// A -> B
	th->tokens.emplace_back(STRAIGHT, Vector2(x+width-ellipseWidth, y));

	// B -> C
	th->tokens.emplace_back(CURVE_CUBIC,
	                        Vector2(x+width-ellipseWidth+kappaW, y),
	                        Vector2(x+width, y+kappaH),
	                        Vector2(x+width, y+ellipseHeight));

	// C -> D
	th->tokens.emplace_back(STRAIGHT, Vector2(x+width, y+height-ellipseHeight));

	th->owner->owner->requestInvalidation();
	
	return NULL;
}

ASFUNCTIONBODY(Graphics,drawCircle)
{
	Graphics* th=static_cast<Graphics*>(obj);
	assert_and_throw(argslen==3);

	double x=args[0]->toNumber();
	double y=args[1]->toNumber();
	double radius=args[2]->toNumber();

	double kappa = KAPPA*radius;

	// right
	th->tokens.emplace_back(MOVE, Vector2(x+radius, y));

	// bottom
	th->tokens.emplace_back(CURVE_CUBIC,
	                        Vector2(x+radius, y+kappa ),
	                        Vector2(x+kappa , y+radius),
	                        Vector2(x       , y+radius));

	// left
	th->tokens.emplace_back(CURVE_CUBIC,
	                        Vector2(x-kappa , y+radius),
	                        Vector2(x-radius, y+kappa ),
	                        Vector2(x-radius, y       ));

	// top
	th->tokens.emplace_back(CURVE_CUBIC,
	                        Vector2(x-radius, y-kappa ),
	                        Vector2(x-kappa , y-radius),
	                        Vector2(x       , y-radius));

	// back to right
	th->tokens.emplace_back(CURVE_CUBIC,
	                        Vector2(x+kappa , y-radius),
	                        Vector2(x+radius, y-kappa ),
	                        Vector2(x+radius, y       ));

	th->owner->owner->requestInvalidation();
	
	return NULL;
}

ASFUNCTIONBODY(Graphics,drawRect)
{
	Graphics* th=static_cast<Graphics*>(obj);
	assert_and_throw(argslen==4);

	int x=args[0]->toInt();
	int y=args[1]->toInt();
	int width=args[2]->toInt();
	int height=args[3]->toInt();

	const Vector2 a(x,y);
	const Vector2 b(x+width,y);
	const Vector2 c(x+width,y+height);
	const Vector2 d(x,y+height);

	th->tokens.emplace_back(MOVE, a);
	th->tokens.emplace_back(STRAIGHT, b);
	th->tokens.emplace_back(STRAIGHT, c);
	th->tokens.emplace_back(STRAIGHT, d);
	th->tokens.emplace_back(STRAIGHT, a);
	th->owner->owner->requestInvalidation();
	
	return NULL;
}

ASFUNCTIONBODY(Graphics,lineStyle)
{
	Graphics* th=static_cast<Graphics*>(obj);
	if (argslen == 0)
	{
		th->tokens.emplace_back(CLEAR_STROKE);
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
	
	LINESTYLE2 style(-1);
	style.Color = RGBA(color, alpha);
	style.Width = thickness;
	th->tokens.emplace_back(SET_STROKE, style);
	return NULL;
}

ASFUNCTIONBODY(Graphics,beginGradientFill)
{
	Graphics* th=static_cast<Graphics*>(obj);
	assert_and_throw(argslen>=4);

	FILLSTYLE style(-1);

	assert_and_throw(args[1]->getObjectType()==T_ARRAY);
	Array* colors=Class<Array>::cast(args[1]);

	assert_and_throw(args[2]->getObjectType()==T_ARRAY);
	Array* alphas=Class<Array>::cast(args[2]);

	assert_and_throw(args[3]->getObjectType()==T_ARRAY);
	Array* ratios=Class<Array>::cast(args[3]);

	int NumGradient = colors->size();
	if (NumGradient != alphas->size() || NumGradient != ratios->size())
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
	GRADIENT grad(-1);
	grad.NumGradient = NumGradient;
	for(int i = 0; i < NumGradient; i ++)
	{
		GRADRECORD record(-1);
		record.Color = RGBA(colors->at(i)->toUInt(), (int)alphas->at(i)->toNumber()*255);
		record.Ratio = UI8(ratios->at(i)->toUInt());
		grad.GradientRecords.push_back(record);
	}

	if(argslen > 4 && args[4]->getPrototype()==Class<Matrix>::getClass())
	{
		style.Matrix = static_cast<Matrix*>(args[4])->getMATRIX();
		//Conversion from twips to pixels
		style.Matrix.ScaleX /= 20.0;
		style.Matrix.RotateSkew0 /= 20.0;
		style.Matrix.RotateSkew1 /= 20.0;
		style.Matrix.ScaleY /= 20.0;
		//Traslations are ok, that is applied already in the pixel space
	}
	else
	{
		style.Matrix.ScaleX = 100.0/16384.0;
		style.Matrix.ScaleY = 100.0/16384.0;
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

	if(argslen > 6)
	{
		const tiny_string& interp=args[6]->toString();
		if (interp == "rgb")
			grad.InterpolationMode = 0;
		else if (interp == "linearRGB")
			grad.InterpolationMode = 1;
	}

	style.Gradient = grad;
	th->tokens.emplace_back(SET_FILL, style);
	return NULL;
}

ASFUNCTIONBODY(Graphics,beginFill)
{
	Graphics* th=static_cast<Graphics*>(obj);
	uint32_t color=0;
	uint8_t alpha=255;
	if(argslen>=1)
		color=args[0]->toUInt();
	if(argslen>=2)
		alpha=(uint8_t(args[1]->toNumber()*0xff));
	FILLSTYLE style(-1);
	style.FillStyleType = SOLID_FILL;
	style.Color         = RGBA(color, alpha);
	th->tokens.emplace_back(SET_FILL, style);
	return NULL;
}

ASFUNCTIONBODY(Graphics,endFill)
{
	Graphics* th=static_cast<Graphics*>(obj);
	th->tokens.emplace_back(CLEAR_FILL);
	return NULL;
}

void LineScaleMode::sinit(Class_base* c)
{
	c->setConstructor(NULL);
	c->setVariableByQName("HORIZONTAL","",Class<ASString>::getInstanceS("horizontal"));
	c->setVariableByQName("NONE","",Class<ASString>::getInstanceS("none"));
	c->setVariableByQName("NORMAL","",Class<ASString>::getInstanceS("normal"));
	c->setVariableByQName("VERTICAL","",Class<ASString>::getInstanceS("vertical"));
}

void StageScaleMode::sinit(Class_base* c)
{
	c->setConstructor(NULL);
	c->setVariableByQName("EXACT_FIT","",Class<ASString>::getInstanceS("exactFit"));
	c->setVariableByQName("NO_BORDER","",Class<ASString>::getInstanceS("noBorder"));
	c->setVariableByQName("NO_SCALE","",Class<ASString>::getInstanceS("noScale"));
	c->setVariableByQName("SHOW_ALL","",Class<ASString>::getInstanceS("showAll"));
}

void StageAlign::sinit(Class_base* c)
{
	c->setConstructor(NULL);
	c->setVariableByQName("TOP_LEFT","",Class<ASString>::getInstanceS("TL"));
}

void StageQuality::sinit(Class_base* c)
{
	c->setConstructor(NULL);
	c->setVariableByQName("BEST","",Class<ASString>::getInstanceS("best"));
	c->setVariableByQName("HIGH","",Class<ASString>::getInstanceS("high"));
	c->setVariableByQName("LOW","",Class<ASString>::getInstanceS("low"));
	c->setVariableByQName("MEDIUM","",Class<ASString>::getInstanceS("medium"));
}

void StageDisplayState::sinit(Class_base* c)
{
	c->setConstructor(NULL);
	c->setVariableByQName("FULL_SCREEN","",Class<ASString>::getInstanceS("fullScreen"));
	c->setVariableByQName("NORMAL","",Class<ASString>::getInstanceS("normal"));
}

void Bitmap::sinit(Class_base* c)
{
//	c->constructor=Class<IFunction>::getFunction(_constructor);
	c->setConstructor(NULL);
	c->super=Class<DisplayObject>::getClass();
	c->max_level=c->super->max_level+1;
}

bool Bitmap::getBounds(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const
{
	return false;
}

IntSize Bitmap::getBitmapSize() const
{
	return IntSize(100,100);
}

void SimpleButton::sinit(Class_base* c)
{
//	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setConstructor(NULL);
	c->super=Class<InteractiveObject>::getClass();
	c->max_level=c->super->max_level+1;
}

void GradientType::sinit(Class_base* c)
{
	c->setConstructor(NULL);
	c->setVariableByQName("LINEAR","",Class<ASString>::getInstanceS("linear"));
	c->setVariableByQName("RADIAL","",Class<ASString>::getInstanceS("radial"));
}

void BlendMode::sinit(Class_base* c)
{
	c->setConstructor(NULL);
	c->setVariableByQName("ADD","",Class<ASString>::getInstanceS("add"));
	c->setVariableByQName("ALPHA","",Class<ASString>::getInstanceS("alpha"));
	c->setVariableByQName("DARKEN","",Class<ASString>::getInstanceS("darken"));
	c->setVariableByQName("DIFFERENCE","",Class<ASString>::getInstanceS("difference"));
	c->setVariableByQName("ERASE","",Class<ASString>::getInstanceS("erase"));
	c->setVariableByQName("HARDLIGHT","",Class<ASString>::getInstanceS("hardlight"));
	c->setVariableByQName("INVERT","",Class<ASString>::getInstanceS("invert"));
	c->setVariableByQName("LAYER","",Class<ASString>::getInstanceS("layer"));
	c->setVariableByQName("LIGHTEN","",Class<ASString>::getInstanceS("lighten"));
	c->setVariableByQName("MULTIPLY","",Class<ASString>::getInstanceS("multiply"));
	c->setVariableByQName("NORMAL","",Class<ASString>::getInstanceS("normal"));
	c->setVariableByQName("OVERLAY","",Class<ASString>::getInstanceS("overlay"));
	c->setVariableByQName("SCREEN","",Class<ASString>::getInstanceS("screen"));
	c->setVariableByQName("SUBSTRACT","",Class<ASString>::getInstanceS("substract"));
}

void SpreadMethod::sinit(Class_base* c)
{
	c->setConstructor(NULL);
	c->setVariableByQName("PAD","",Class<ASString>::getInstanceS("pad"));
	c->setVariableByQName("REFLECT","",Class<ASString>::getInstanceS("reflect"));
	c->setVariableByQName("REPEAT","",Class<ASString>::getInstanceS("repeat"));
}

void InterpolationMethod::sinit(Class_base* c)
{
	c->setConstructor(NULL);
	c->setVariableByQName("RGB","",Class<ASString>::getInstanceS("rgb"));
	c->setVariableByQName("LINEAR_RGB","",Class<ASString>::getInstanceS("linearRGB"));
}

void SimpleButton::buildTraits(ASObject* o)
{
}

bool SimpleButton::getBounds(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const
{
	return false;
}

