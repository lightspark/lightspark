/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2011  Alessandro Pignotti (a.pignotti@sssup.it)

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
#include "backends/image.h"
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
REGISTER_CLASS_NAME(FrameLabel);
REGISTER_CLASS_NAME(Scene);

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

void LoaderInfo::finalize()
{
	EventDispatcher::finalize();
	sharedEvents.reset();
	loader.reset();
}

void LoaderInfo::setBytesLoaded(uint32_t b)
{
	if(b!=bytesLoaded)
	{
		SpinlockLocker l(spinlock);
		bytesLoaded=b;
		if(sys && sys->currentVm)
		{
			this->incRef();
			sys->currentVm->addEvent(_MR(this),_MR(Class<ProgressEvent>::getInstanceS(bytesLoaded,bytesTotal)));
		}
		if(loadStatus==INIT_SENT)
		{
			//The clip is also complete now
			this->incRef();
			sys->currentVm->addEvent(_MR(this),_MR(Class<Event>::getInstanceS("complete")));
			loadStatus=COMPLETE;
		}
	}
}

void LoaderInfo::sendInit()
{
	this->incRef();
	sys->currentVm->addEvent(_MR(this),_MR(Class<Event>::getInstanceS("init")));
	SpinlockLocker l(spinlock);
	assert(loadStatus==STARTED);
	loadStatus=INIT_SENT;
	if(bytesTotal && bytesLoaded==bytesTotal)
	{
		//The clip is also complete now
		this->incRef();
		sys->currentVm->addEvent(_MR(this),_MR(Class<Event>::getInstanceS("complete")));
		loadStatus=COMPLETE;
	}
}

ASFUNCTIONBODY(LoaderInfo,_constructor)
{
	LoaderInfo* th=static_cast<LoaderInfo*>(obj);
	EventDispatcher::_constructor(obj,NULL,0);
	th->sharedEvents=_MR(Class<EventDispatcher>::getInstanceS());
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
		return new Undefined;
	else
		return Loader::_getContent(th->loader.getPtr(),NULL,0);
}

ASFUNCTIONBODY(LoaderInfo,_getLoader)
{
	LoaderInfo* th=static_cast<LoaderInfo*>(obj);
	if(th->loader.isNull())
		return new Undefined;
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
	return Class<ApplicationDomain>::getInstanceS();
}

ASFUNCTIONBODY(Loader,_constructor)
{
	Loader* th=static_cast<Loader*>(obj);
	DisplayObjectContainer::_constructor(obj,NULL,0);
	th->incRef();
	th->contentLoaderInfo=_MR(Class<LoaderInfo>::getInstanceS(_MR(th)));
	//TODO: the parameters should only be set if the loaded clip uses AS3. See specs.
	_NR<ASObject> p=sys->getParameters();
	if(!p.isNull())
	{
		p->incRef();
		th->contentLoaderInfo->setVariableByQName("parameters","",p.getPtr());
	}
	return NULL;
}

ASFUNCTIONBODY(Loader,_getContent)
{
	Loader* th=static_cast<Loader*>(obj);
	SpinlockLocker l(th->localRootSpinlock);
	_NR<ASObject> ret=th->localRoot;
	if(ret.isNull())
		ret=_MR(new Undefined);

	ret->incRef();
	return ret.getPtr();
}

ASFUNCTIONBODY(Loader,_getContentLoaderInfo)
{
	Loader* th=static_cast<Loader*>(obj);
	th->contentLoaderInfo->incRef();
	return th->contentLoaderInfo.getPtr();
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
	assert_and_throw(args[0]->getPrototype() && 
			args[0]->getPrototype()->isSubClass(Class<ByteArray>::getClass()));
	args[0]->incRef();
	th->bytes=_MR(static_cast<ByteArray*>(args[0]));
	if(th->bytes->bytes)
	{
		th->loading=true;
		th->source=BYTES;
		//To be decreffed in jobFence
		th->incRef();
		sys->addJob(th);
	}
	return NULL;
}

void Loader::finalize()
{
	DisplayObjectContainer::finalize();
	localRoot.reset();
	contentLoaderInfo.reset();
	bytes.reset();
}

Loader::~Loader()
{
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
	//Use a local variable to store the new root, as the localRoot member may change
	_R<RootMovieClip> newRoot=_MR(RootMovieClip::getInstance(contentLoaderInfo.getPtr()));
	if(isOnStage())
		newRoot->setOnStage(true);
	if(source==URL)
	{
		//TODO: add security checks
		LOG(LOG_CALLS,_("Loader async execution ") << url);
		downloader=sys->downloadManager->download(url, false, contentLoaderInfo.getPtr());
		downloader->waitForData(); //Wait for some data, making sure our check for failure is working
		if(downloader->hasFailed()) //Check to see if the download failed for some reason
		{
			LOG(LOG_ERROR, "Loader::execute(): Download of URL failed: " << url);
			sys->currentVm->addEvent(contentLoaderInfo,_MR(Class<Event>::getInstanceS("ioError")));
			sys->downloadManager->destroy(downloader);
			return;
		}
		sys->currentVm->addEvent(contentLoaderInfo,_MR(Class<Event>::getInstanceS("open")));
		istream s(downloader);
		ParseThread* local_pt=new ParseThread(newRoot.getPtr(),s);
		local_pt->run();
		{
			//Acquire the lock to ensure consistency in threadAbort
			SpinlockLocker l(downloaderLock);
			sys->downloadManager->destroy(downloader);
			downloader=NULL;
		}
		if(local_pt->getFileType()==ParseThread::SWF)
		{
			SpinlockLocker l(localRootSpinlock);
			localRoot=newRoot;
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

		ParseThread* local_pt = new ParseThread(newRoot.getPtr(),s);
		local_pt->run();
		if(local_pt->getFileType()==ParseThread::SWF)
		{
			SpinlockLocker l(localRootSpinlock);
			localRoot=newRoot;
		}
		bytes->decRef();
		//Add a complete event for this object
		sys->currentVm->addEvent(contentLoaderInfo,_MR(Class<Event>::getInstanceS("complete")));
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
	SpinlockLocker l(localRootSpinlock);
	if(!loaded || skipRender(maskEnabled) || localRoot.isNull())
		return;

	renderPrologue();

	localRoot->Render(maskEnabled);

	renderEpilogue();
}

bool Loader::getBounds(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const
{
	SpinlockLocker l(localRootSpinlock);
	if(!localRoot.isNull() && localRoot->getBounds(xmin,xmax,ymin,ymax))
	{
		getMatrix().multiply2D(xmin,ymin,xmin,ymin);
		getMatrix().multiply2D(xmax,ymax,xmax,ymax);
		return true;
	}
	else
		return false;
}

void Loader::setOnStage(bool staged)
{
	DisplayObjectContainer::setOnStage(staged);
	SpinlockLocker l(localRootSpinlock);
	if(!localRoot.isNull())
		localRoot->setOnStage(staged);
}

Sprite::Sprite():TokenContainer(this),graphics(NULL)
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
	c->super=Class<DisplayObjectContainer>::getClass();
	c->max_level=c->super->max_level+1;
	c->setGetterByQName("graphics","",Class<IFunction>::getFunction(_getGraphics),true);
}

void Sprite::buildTraits(ASObject* o)
{
}

bool Sprite::boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const
{
	bool ret=false;
	{
		Locker l(mutexDisplayList);
		if(dynamicDisplayList.empty() && tokensEmpty())
			return false;

		//TODO: Check bounds calculation
		list<_R<DisplayObject>>::const_iterator it=dynamicDisplayList.begin();
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
	number_t txmin,txmax,tymin,tymax;
	if(TokenContainer::getBounds(txmin,txmax,tymin,tymax))
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

void Sprite::requestInvalidation()
{
	DisplayObjectContainer::requestInvalidation();
	TokenContainer::requestInvalidation();
}

void DisplayObject::renderPrologue() const
{
	if(!mask.isNull())
	{
		if(mask->parent.isNull())
			rt->pushMask(mask.getPtr(),MATRIX());
		else
			rt->pushMask(mask.getPtr(),mask->parent->getConcatenatedMatrix());
	}
}

void DisplayObject::renderEpilogue() const
{
	if(!mask.isNull())
		rt->popMask();
}

void Sprite::renderImpl(bool maskEnabled, number_t t1,number_t t2,number_t t3,number_t t4) const
{
	//Draw the dynamically added graphics, if any
	//Should clean only the bounds of the graphics
	if(!tokensEmpty())
		defaultRender(maskEnabled);

	{
		Locker l(mutexDisplayList);
		//Now draw also the display list
		list<_R<DisplayObject>>::const_iterator it=dynamicDisplayList.begin();
		for(;it!=dynamicDisplayList.end();++it)
			(*it)->Render(maskEnabled);
	}
}

void Sprite::Render(bool maskEnabled)
{
	if(skipRender(maskEnabled))
		return;

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
	if(!mask.isNull())
		sys->getInputThread()->pushMask(mask.getPtr(),mask->getConcatenatedMatrix().getInverted());
}

void DisplayObject::hitTestEpilogue() const
{
	if(!mask.isNull())
		sys->getInputThread()->popMask();
}

_NR<InteractiveObject> Sprite::hitTestImpl(number_t x, number_t y)
{
	_NR<InteractiveObject> ret = NullRef;
	{
		//Test objects added at runtime, in reverse order
		Locker l(mutexDisplayList);
		list<_R<DisplayObject>>::const_reverse_iterator j=dynamicDisplayList.rbegin();
		for(;j!=dynamicDisplayList.rend();++j)
		{
			number_t localX, localY;
			(*j)->getMatrix().getInverted().multiply2D(x,y,localX,localY);
			this->incRef();
			ret=(*j)->hitTest(_MR(this), localX,localY);
			if(!ret.isNull())
				break;
		}
	}

	if(ret==NULL && !tokensEmpty() && mouseEnabled)
	{
		//The coordinates are locals
		if(TokenContainer::hitTest(x,y))
		{
			this->incRef();
			ret=_MR(this);
			//Also test if the we are under the mask (if any)
			if(sys->getInputThread()->isMaskPresent())
			{
				number_t globalX, globalY;
				getConcatenatedMatrix().multiply2D(x,y,globalX,globalY);
				if(!sys->getInputThread()->isMasked(globalX, globalY))
					ret=NullRef;
			}
		}
	}
	return ret;
}

_NR<InteractiveObject> Sprite::hitTest(_NR<InteractiveObject>, number_t x, number_t y)
{
	//NOTE: in hitTest the stuff must be rendered in the opposite order of Rendering
	//TODO: TOLOCK
	//First of all filter using the BBOX
	number_t t1,t2,t3,t4;
	bool notEmpty=boundsRect(t1,t2,t3,t4);
	if(!notEmpty)
		return NullRef;
	if(x<t1 || x>t2 || y<t3 || y>t4)
		return NullRef;

	hitTestPrologue();

	_NR<InteractiveObject> ret=hitTestImpl(x, y);

	hitTestEpilogue();
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

void FrameLabel::sinit(Class_base* c)
{
	c->setConstructor(NULL);
	c->super=Class<ASObject>::getClass();
	c->max_level=c->super->max_level+1;
	c->setGetterByQName("frame","",Class<IFunction>::getFunction(_getFrame),true);
	c->setGetterByQName("name","",Class<IFunction>::getFunction(_getName),true);
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

void Scene::sinit(Class_base* c)
{
	c->setConstructor(NULL);
	c->super=Class<ASObject>::getClass();
	c->max_level=c->super->max_level+1;
	c->setGetterByQName("labels","",Class<IFunction>::getFunction(_getLabels),true);
	c->setGetterByQName("name","",Class<IFunction>::getFunction(_getName),true);
	c->setGetterByQName("numFrames","",Class<IFunction>::getFunction(_getNumFrames),true);
}

ASFUNCTIONBODY(Scene,_getLabels)
{
	Scene* th=static_cast<Scene*>(obj);
	Array* ret = Class<Array>::getInstanceS();
	ret->resize(th->labels.size());
	for(size_t i=0; i<th->labels.size(); ++i)
	{
		ret->set(i, Class<FrameLabel>::getInstanceS(th->labels[i]));
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

void Frame::execute(_R<DisplayObjectContainer> displayList)
{
	std::list<DisplayListTag*>::iterator it=blueprint.begin();
	for(;it!=blueprint.end();++it)
		(*it)->execute(displayList.getPtr());
}

void MovieClip::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->super=Class<Sprite>::getClass();
	c->max_level=c->super->max_level+1;
	c->setGetterByQName("currentFrame","",Class<IFunction>::getFunction(_getCurrentFrame),true);
	c->setGetterByQName("totalFrames","",Class<IFunction>::getFunction(_getTotalFrames),true);
	c->setGetterByQName("framesLoaded","",Class<IFunction>::getFunction(_getFramesLoaded),true);
	c->setGetterByQName("currentFrameLabel","",Class<IFunction>::getFunction(_getCurrentFrameLabel),true);
	c->setGetterByQName("currentLabel","",Class<IFunction>::getFunction(_getCurrentLabel),true);
	c->setGetterByQName("scenes","",Class<IFunction>::getFunction(_getScenes),true);
	c->setGetterByQName("currentScene","",Class<IFunction>::getFunction(_getCurrentScene),true);
	c->setMethodByQName("stop","",Class<IFunction>::getFunction(stop),true);
	c->setMethodByQName("gotoAndStop","",Class<IFunction>::getFunction(gotoAndStop),true);
	c->setMethodByQName("gotoAndPlay","",Class<IFunction>::getFunction(gotoAndPlay),true);
	c->setMethodByQName("nextFrame","",Class<IFunction>::getFunction(nextFrame),true);
	c->setMethodByQName("addFrameScript","",Class<IFunction>::getFunction(addFrameScript),true);
}

void MovieClip::buildTraits(ASObject* o)
{
}

MovieClip::MovieClip():constructed(false),totalFrames_unreliable(1),framesLoaded(0)
{
	frames.emplace_back();
	scenes.resize(1);
}

MovieClip::MovieClip(const MovieClip& r):constructed(false),frames(r.frames),
	totalFrames_unreliable(r.totalFrames_unreliable),framesLoaded(r.framesLoaded),
	frameScripts(r.frameScripts),scenes(r.scenes),
	state(r.state)
{
	assert(!r.isConstructed());
}

void MovieClip::finalize()
{
	Sprite::finalize();
	frames.clear();
	frameScripts.clear();
}

void MovieClip::setTotalFrames(uint32_t t)
{
	//For sprites, this is called after parsing
	//with the actual frame count
	//For the root movie, this is called before the parsing
	//with the frame count from the header
	totalFrames_unreliable=t;
}

/* This runs in vm's thread context,
 * but no locking is needed here as it only accesses the last frame.
 * See comment on the 'frames' member. */
void MovieClip::addToFrame(DisplayListTag* t)
{
	frames.back().blueprint.push_back(t);
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
	th->incRef();
	sys->currentVm->addEvent(NullRef,_MR(new FrameChangeEvent(th->state.FP+1,_MR(th))));
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
		ret->set(i, Class<Scene>::getInstanceS(th->scenes[i],numFrames));
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
	return new Null();
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
			if(th->scenes[i].labels[j].name.len() != 0)
				label = th->scenes[i].labels[j].name;
		}
	}

	if(label.len() == 0)
		return new Null();
	else
		return Class<ASString>::getInstanceS(label);
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
	if((!state.stop_FP || state.explicit_FP) && getFramesLoaded()!=0 && getPrototype()->isSubClass(Class<MovieClip>::getClass()))
	{
		//If we have not yet loaded enough frames delay advancement
		if(state.next_FP>=getFramesLoaded())
		{
			LOG(LOG_NOT_IMPLEMENTED,_("Not enough frames loaded"));
			return;
		}

		/* Go through the list of frames.
		 * If our next_FP is after our current,
		 * we construct all frames from current
		 * to next_FP.
		 * If our next_FP is before our current,
		 * we purge all objects on the 0th frame
		 * and then construct all frames from
		 * the 0th to the next_FP.
		 */
		bool purge;
		std::list<Frame>::iterator iter=frames.begin();
		for(uint32_t i=0;i<=state.next_FP;i++)
		{
			if(state.next_FP < state.FP || i > state.FP)
			{
				purge = (i==0);
				if(sys->currentVm)
				{
					this->incRef();
					_R<ConstructFrameEvent> ce(new ConstructFrameEvent(&(*iter), _MR(this), purge));
					sys->currentVm->addEvent(NullRef, ce);
				}
			}
			++iter;
		}

		bool frameChanging=(state.FP!=state.next_FP);
		state.FP=state.next_FP;
		if(!state.stop_FP && getFramesLoaded()>0)
			state.next_FP=imin(state.FP+1,getFramesLoaded()-1);
		state.explicit_FP=false;

		if(frameChanging && frameScripts.count(state.FP))
			getVm()->addEvent(NullRef, _MR(new FunctionEvent(frameScripts[state.FP])));

		/*
		 Frame& curFrame=frames[state.FP];
		//Set the object on stage
		if(isOnStage())
		{
			DisplayListType::const_iterator it=curFrame.displayList.begin();
			for(;it!=curFrame.displayList.end();it++)
				it->second->setOnStage(true);
		}

		//Invalidate the current frame if needed
		if(curFrame.isInvalid())
		{
			DisplayListType::const_iterator it=curFrame.displayList.begin();
			for(;it!=curFrame.displayList.end();it++)
				it->second->requestInvalidation();
			curFrame.setInvalid(false);
		}*/
	}

}

void MovieClip::bootstrap()
{
	assert_and_throw(getFramesLoaded()>0);
	if(sys->currentVm)
	{
		this->incRef();
		_R<ConstructFrameEvent> ce(new ConstructFrameEvent(&frames.front(), _MR(this), false));
		sys->currentVm->addEvent(NullRef, ce);
	}
}

void MovieClip::setParent(_NR<DisplayObjectContainer> p)
{
	if(!getParent().isNull() && !getParent()->getRoot().isNull())
		getParent()->getRoot()->unregisterChildClip(this);
	Sprite::setParent(p);
	if(!getParent().isNull() && !getParent()->getRoot().isNull())
		getParent()->getRoot()->registerChildClip(this);
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
		assert(scenes.size() == sceneNo-1);
		scenes.resize(sceneNo);
		scenes[sceneNo].name = name;
		scenes[sceneNo].startframe = startframe;
	}
}

/**
 * Find the scene to which the given frame belongs and
 * adds the frame label to that scene.
 * The labels of the scene will stay sorted by frame.
 */
void MovieClip::addFrameLabel(uint32_t frame, const tiny_string& label)
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

void MovieClip::constructionComplete()
{
	RELEASE_WRITE(constructed,true);
	//Execute the event registered for the first frame, if any
	if(sys->currentVm && frameScripts.count(0))
	{
		_R<FunctionEvent> funcEvent(new FunctionEvent(_MR(frameScripts[0])));
		getVm()->addEvent(NullRef, funcEvent);
	}

}

DisplayObject::DisplayObject():useMatrix(true),tx(0),ty(0),rotation(0),sx(1),sy(1),maskOf(NULL),parent(NULL),mask(NULL),onStage(false),
	loaderInfo(NULL),alpha(1.0),visible(true),invalidateQueueNext(NULL)
{
}

DisplayObject::DisplayObject(const DisplayObject& d):useMatrix(true),tx(d.tx),ty(d.ty),rotation(d.rotation),sx(d.sx),sy(d.sy),maskOf(NULL),
	parent(NULL),mask(NULL),onStage(false),loaderInfo(NULL),alpha(d.alpha),visible(d.visible),invalidateQueueNext(NULL)
{
}

void DisplayObject::finalize()
{
	EventDispatcher::finalize();
	maskOf.reset();
	parent.reset();
	mask.reset();
	loaderInfo.reset();
	invalidateQueueNext.reset();
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
	c->setGetterByQName("mouseX","",Class<IFunction>::getFunction(_getMouseX),true);
	c->setGetterByQName("mouseY","",Class<IFunction>::getFunction(_getMouseY),true);
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

void DisplayObject::becomeMaskOf(_NR<DisplayObject> m)
{
	maskOf=m;
/*	_NR<DisplayObject> tmp=maskOf;
	maskOf.reset();
	if(!tmp.isNull())
		tmp->setMask(NullRef);*/
}

void DisplayObject::setMask(_NR<DisplayObject> m)
{
	bool mustInvalidate=(mask!=m) && onStage;

	if(!mask.isNull())
	{
		//Remove previous mask
		mask->becomeMaskOf(NullRef);
	}

	mask=m;
	if(!mask.isNull())
	{
		//Use new mask
		this->incRef();
		mask->becomeMaskOf(_MR(this));
	}

	if(mustInvalidate && onStage)
		requestInvalidation();
}

MATRIX DisplayObject::getConcatenatedMatrix() const
{
	if(parent.isNull())
		return getMatrix();
	else
		return parent->getConcatenatedMatrix().multiplyMatrix(getMatrix());
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
	return visible==false || alpha==0.0 || (!maskEnabled && !maskOf.isNull());
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
	if(!mask.isNull())
		mask->requestInvalidation();
}

void DisplayObject::localToGlobal(number_t xin, number_t yin, number_t& xout, number_t& yout) const
{
	getMatrix().multiply2D(xin, yin, xout, yout);
	if(!parent.isNull())
		parent->localToGlobal(xout, yout, xout, yout);
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
			this->incRef();
			getVm()->addEvent(_MR(this),_MR(Class<Event>::getInstanceS("addedToStage")));
		}
		else if(onStage==false && hasEventListener("removedFromStage"))
		{
			this->incRef();
			getVm()->addEvent(_MR(this),_MR(Class<Event>::getInstanceS("removedFromStage")));
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
	if(th->mask.isNull())
		return new Null;

	th->mask->incRef();
	return th->mask.getPtr();
}

ASFUNCTIONBODY(DisplayObject,_setMask)
{
	DisplayObject* th=Class<DisplayObject>::cast(obj);
	assert_and_throw(argslen==1);
	if(args[0] && args[0]->getPrototype() && args[0]->getPrototype()->isSubClass(Class<DisplayObject>::getClass()))
	{
		//We received a valid mask object
		DisplayObject* newMask=Class<DisplayObject>::cast(args[0]);
		newMask->incRef();
		th->setMask(_MR(newMask));
	}
	else
		th->setMask(NullRef);

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
	if(th->loaderInfo.isNull())
		return new Undefined;

	th->loaderInfo->incRef();
	return th->loaderInfo.getPtr();
}

ASFUNCTIONBODY(DisplayObject,_getStage)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	_NR<Stage> ret=th->getStage();
	if(ret.isNull())
		return new Null;

	ret->incRef();
	return ret.getPtr();
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

void DisplayObject::setParent(_NR<DisplayObjectContainer> p)
{
	if(parent!=p)
	{
		parent=p;
		if(onStage)
			requestInvalidation();
	}
}

ASFUNCTIONBODY(DisplayObject,_getParent)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	if(th->parent.isNull())
		return new Undefined;

	th->parent->incRef();
	return th->parent.getPtr();
}

ASFUNCTIONBODY(DisplayObject,_getRoot)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	_NR<RootMovieClip> ret=th->getRoot();
	if(ret.isNull())
		return new Undefined;

	ret->incRef();
	return ret.getPtr();
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

_NR<RootMovieClip> DisplayObject::getRoot()
{
	if(parent.isNull())
		return NullRef;

	return parent->getRoot();
}

_NR<Stage> DisplayObject::getStage() const
{
	if(parent.isNull())
		return NullRef;

	return parent->getStage();
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

ASFUNCTIONBODY(DisplayObject,_getMouseX)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	number_t temp, outX;
	th->getConcatenatedMatrix().getInverted().multiply2D(sys->getInputThread()->getMouseX(), temp, outX, temp);
	return abstract_d(outX);
}

ASFUNCTIONBODY(DisplayObject,_getMouseY)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	number_t temp, outY;
	th->getConcatenatedMatrix().getInverted().multiply2D(temp, sys->getInputThread()->getMouseY(), temp, outY);
	return abstract_d(outY);
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

bool DisplayObjectContainer::hasLegacyChildAt(uint32_t depth)
{
	std::map<uint32_t,DisplayObject*>::iterator i = depthToLegacyChild.find(depth);
	return i != depthToLegacyChild.end();
}

void DisplayObjectContainer::deleteLegacyChildAt(uint32_t depth)
{
	if(!hasLegacyChildAt(depth))
	{
		LOG(LOG_ERROR,"deleteLegacyChildAt: no child at that depth");
		return;
	}
	DisplayObject* obj = depthToLegacyChild[depth];
	if(obj->name.len() > 0)
	{
		multiname objName;
		objName.name_type=multiname::NAME_STRING;
		objName.name_s=obj->name;
		objName.ns.push_back(nsNameAndKind("",NAMESPACE));
		deleteVariableByMultiname(objName);
	}

	obj->incRef();
	bool ret = _removeChild(_MR(obj));
	assert_and_throw(ret);

	depthToLegacyChild.erase(depth);
}

void DisplayObjectContainer::insertLegacyChildAt(uint32_t depth, DisplayObject* obj)
{
	if(hasLegacyChildAt(depth))
	{
		LOG(LOG_ERROR,"insertLegacyChildAt: there is already one child at that depth");
		return;
	}
	_addChildAt(_MR(obj),depth-1); /* depth is 1 based in SWF */
	if(obj->name.len() > 0)
	{
		obj->incRef();
		setVariableByQName(obj->name,"",obj);
	}

	depthToLegacyChild[depth] = obj;
}

void DisplayObjectContainer::transformLegacyChildAt(uint32_t depth, const MATRIX& mat)
{
	if(!hasLegacyChildAt(depth))
	{
		LOG(LOG_ERROR,"transformLegacyChildAt: no child at that depth");
		return;
	}
	depthToLegacyChild[depth]->setMatrix(mat);
}

void DisplayObjectContainer::purgeLegacyChildren()
{
	std::map<uint32_t,DisplayObject*>::iterator i = depthToLegacyChild.begin();
	while( i != depthToLegacyChild.end() )
	{
		deleteLegacyChildAt(i->first);
		i = depthToLegacyChild.begin();
	}
}

void DisplayObjectContainer::finalize()
{
	InteractiveObject::finalize();
	//Release every child
	dynamicDisplayList.clear();
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
	list<_R<DisplayObject> >::const_iterator it=dynamicDisplayList.begin();
	for(;it!=dynamicDisplayList.end();++it)
		cout << (*it)->getPrototype()->class_name << endl;
}

void DisplayObjectContainer::setOnStage(bool staged)
{
	if(staged!=onStage)
	{
		DisplayObject::setOnStage(staged);
		//Notify childern
		Locker l(mutexDisplayList);
		list<_R<DisplayObject>>::const_iterator it=dynamicDisplayList.begin();
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
	list<_R<DisplayObject>>::const_iterator it=dynamicDisplayList.begin();
	for(;it!=dynamicDisplayList.end();it++)
		(*it)->requestInvalidation();
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
	if(child->getParent()==NULL)
		return false;
	assert_and_throw(child->getParent()==this);

	{
		Locker l(mutexDisplayList);
		list<_R<DisplayObject>>::iterator it=find(dynamicDisplayList.begin(),dynamicDisplayList.end(),child);
		if(it==dynamicDisplayList.end())
			return false;
		dynamicDisplayList.erase(it);
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
	assert_and_throw(args[0] && args[0]->getPrototype() && 
		args[0]->getPrototype()->isSubClass(Class<DisplayObject>::getClass()));

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
		return new Null;
	}
	//Validate object type
	assert_and_throw(args[0]->getPrototype() &&
		args[0]->getPrototype()->isSubClass(Class<DisplayObject>::getClass()));

	int index=args[1]->toInt();

	//Cast to object
	args[0]->incRef();
	_R<DisplayObject> d=_MR(Class<DisplayObject>::cast(args[0]));
	assert_and_throw(index >= 0 && (size_t)index<=th->dynamicDisplayList.size());
	th->_addChildAt(d,index);

	//Notify the object
	sys->currentVm->addEvent(d,_MR(Class<Event>::getInstanceS("added")));

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
		return new Null;
	}
	//Validate object type
	assert_and_throw(args[0] && args[0]->getPrototype() && 
		args[0]->getPrototype()->isSubClass(Class<DisplayObject>::getClass()));

	//Cast to object
	args[0]->incRef();
	_R<DisplayObject> d=_MR(Class<DisplayObject>::cast(args[0]));
	th->_addChildAt(d,numeric_limits<unsigned int>::max());

	//Notify the object
	sys->currentVm->addEvent(d,_MR(Class<Event>::getInstanceS("added")));

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
		return new Null;
	}
	//Validate object type
	assert_and_throw(args[0] && args[0]->getPrototype() && 
		args[0]->getPrototype()->isSubClass(Class<DisplayObject>::getClass()));
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
	list<_R<DisplayObject>>::iterator it=th->dynamicDisplayList.begin();
	for(unsigned int i=0;i<index;i++)
		++it;

	(*it)->incRef();
	return (*it).getPtr();
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

	list<_R<DisplayObject>>::const_iterator it=th->dynamicDisplayList.begin();
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

void Shape::finalize()
{
	DisplayObject::finalize();
	graphics.reset();
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
	bool ret=TokenContainer::getBounds(xmin,xmax,ymin,ymax);
	if(ret)
	{
		getMatrix().multiply2D(xmin,ymin,xmin,ymin);
		getMatrix().multiply2D(xmax,ymax,xmax,ymax);
		return true;
	}
	return false;
}


bool Shape::isOpaque(number_t x, number_t y) const
{
	LOG(LOG_NOT_IMPLEMENTED,"Shape::isOpaque not really implemented");
	return false;
}

_NR<InteractiveObject> Shape::hitTest(_NR<InteractiveObject> last, number_t x, number_t y)
{
	//NOTE: in hitTest the stuff must be rendered in the opposite order of Rendering
	assert_and_throw(!sys->getInputThread()->isMaskPresent());
	//TODO: TOLOCK
	if(!mask.isNull())
		throw UnsupportedException("Support masks in Shape::hitTest");

	//The coordinates are already local
	if(TokenContainer::hitTest(x,y))
		return last;

	return NullRef;
}

void TokenContainer::renderImpl(bool maskEnabled, number_t t1, number_t t2, number_t t3, number_t t4) const
{
	if(!owner->isSimple())
		rt->glAcquireTempBuffer(t1,t2,t3,t4);

	owner->defaultRender(maskEnabled);

	if(!owner->isSimple())
		rt->glBlitTempBuffer(t1,t2,t3,t4);
}

void TokenContainer::Render(bool maskEnabled)
{
	if(tokens.empty())
		return;
	//If graphics is not yet initialized we have nothing to do
	if(owner->skipRender(maskEnabled))
		return;

	number_t t1,t2,t3,t4;
	bool ret=getBounds(t1,t2,t3,t4);
	if(!ret)
		return;

	owner->renderPrologue();

	renderImpl(maskEnabled,t1,t2,t3,t4);

	owner->renderEpilogue();
}

/*! \brief Generate a vector of shapes from a SHAPERECORD list
* * \param cur SHAPERECORD list head
* * \param shapes a vector to be populated with the shapes */

void TokenContainer::FromShaperecordListToShapeVector(const std::vector<SHAPERECORD>& shapeRecords,
													  std::vector<GeomToken>& tokens,
													  const std::list<FILLSTYLE>& fillStyles,
													  const Vector2& offset, int scaling)
{
	int startX=offset.x;
	int startY=offset.y;
	unsigned int color0=0;
	unsigned int color1=0;

	ShapesBuilder shapesBuilder;

	for(unsigned int i=0;i<shapeRecords.size();i++)
	{
		const SHAPERECORD* cur=&shapeRecords[i];
		if(cur->TypeFlag)
		{
			if(cur->StraightFlag)
			{
				Vector2 p1(startX,startY);
				startX+=cur->DeltaX * scaling;
				startY+=cur->DeltaY * scaling;
				Vector2 p2(startX,startY);

				if(color0)
					shapesBuilder.extendFilledOutlineForColor(color0,p1,p2);
				if(color1)
					shapesBuilder.extendFilledOutlineForColor(color1,p1,p2);
			}
			else
			{
				Vector2 p1(startX,startY);
				startX+=cur->ControlDeltaX * scaling;
				startY+=cur->ControlDeltaY * scaling;
				Vector2 p2(startX,startY);
				startX+=cur->AnchorDeltaX * scaling;
				startY+=cur->AnchorDeltaY * scaling;
				Vector2 p3(startX,startY);

				if(color0)
					shapesBuilder.extendFilledOutlineForColorCurve(color0,p1,p2,p3);
				if(color1)
					shapesBuilder.extendFilledOutlineForColorCurve(color1,p1,p2,p3);
			}
		}
		else
		{
			if(cur->StateMoveTo)
			{
				startX=cur->MoveDeltaX * scaling + offset.x;
				startY=cur->MoveDeltaY * scaling + offset.y;
			}
/*			if(cur->StateLineStyle)
			{
				cur_path->state.validStroke=true;
				cur_path->state.stroke=cur->LineStyle;
			}*/
			if(cur->StateFillStyle1)
			{
				color1=cur->FillStyle1;
			}
			if(cur->StateFillStyle0)
			{
				color0=cur->FillStyle0;
			}
		}
	}

	shapesBuilder.outputTokens(fillStyles, tokens);
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
	if(sys->scaleMode==SystemState::NO_SCALE)
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
	if(sys->scaleMode==SystemState::NO_SCALE)
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
	rt->requestResize(rt->windowWidth, rt->windowHeight);
	return NULL;
}

void TokenContainer::requestInvalidation()
{
	if(tokens.empty())
		return;
	owner->incRef();
	sys->addToInvalidateQueue(_MR(owner));
}

void TokenContainer::invalidate()
{
	uint32_t x,y,width,height;
	number_t bxmin,bxmax,bymin,bymax;
	if(getBounds(bxmin,bxmax,bymin,bymax)==false)
	{
		//No contents, nothing to do
		return;
	}

	owner->computeDeviceBoundsForRect(bxmin,bxmax,bymin,bymax,x,y,width,height);
	if(width==0 || height==0)
		return;
	CairoRenderer* r=new CairoRenderer(owner, owner->cachedSurface, tokens,
				owner->getConcatenatedMatrix(), x, y, width, height, scaling);
	sys->addJob(r);
}

bool TokenContainer::hitTest(number_t x, number_t y) const
{
	return CairoRenderer::hitTest(tokens, 1, x, y);
}

bool TokenContainer::getBounds(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const
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
	if(hasContent)
	{
		/* scale the bounding box coordinates and round them to a bigger integer box */
		#define roundDown(x) \
			copysign(floor(abs(x)), x)
		#define roundUp(x) \
			copysign(ceil(abs(x)), x)
		xmin = roundDown(xmin*scaling);
		xmax = roundUp(xmax*scaling);
		ymin = roundDown(ymin*scaling);
		ymax = roundUp(ymax*scaling);
		#undef roundDown
		#undef roundUp
	}
	return hasContent;

#undef VECTOR_BOUNDS
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

ASFUNCTIONBODY(Graphics,_constructor)
{
	return NULL;
}

ASFUNCTIONBODY(Graphics,clear)
{
	Graphics* th=static_cast<Graphics*>(obj);
	th->checkAndSetScaling();
	th->owner->tokens.clear();
	th->owner->owner->requestInvalidation();
	return NULL;
}

ASFUNCTIONBODY(Graphics,moveTo)
{
	Graphics* th=static_cast<Graphics*>(obj);
	th->checkAndSetScaling();
	assert_and_throw(argslen==2);

	th->curX=args[0]->toInt();
	th->curY=args[1]->toInt();

	th->owner->tokens.emplace_back(MOVE, Vector2(th->curX, th->curY));
	return NULL;
}

ASFUNCTIONBODY(Graphics,lineTo)
{
	Graphics* th=static_cast<Graphics*>(obj);
	assert_and_throw(argslen==2);
	th->checkAndSetScaling();

	int x=args[0]->toInt();
	int y=args[1]->toInt();

	th->owner->tokens.emplace_back(STRAIGHT, Vector2(x, y));
	th->owner->owner->requestInvalidation();

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

	th->owner->tokens.emplace_back(CURVE_QUADRATIC,
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
	th->checkAndSetScaling();

	int control1X=args[0]->toInt();
	int control1Y=args[1]->toInt();

	int control2X=args[2]->toInt();
	int control2Y=args[3]->toInt();

	int anchorX=args[4]->toInt();
	int anchorY=args[5]->toInt();

	th->owner->tokens.emplace_back(CURVE_CUBIC,
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
	th->owner->tokens.emplace_back(MOVE, Vector2(x+width, y+height-ellipseHeight));

	// D -> E
	th->owner->tokens.emplace_back(CURVE_CUBIC,
	                        Vector2(x+width, y+height-ellipseHeight+kappaH),
	                        Vector2(x+width-ellipseWidth+kappaW, y+height),
	                        Vector2(x+width-ellipseWidth, y+height));

	// E -> F
	th->owner->tokens.emplace_back(STRAIGHT, Vector2(x+ellipseWidth, y+height));

	// F -> G
	th->owner->tokens.emplace_back(CURVE_CUBIC,
	                        Vector2(x+ellipseWidth-kappaW, y+height),
	                        Vector2(x, y+height-kappaH),
	                        Vector2(x, y+height-ellipseHeight));

	// G -> H
	th->owner->tokens.emplace_back(STRAIGHT, Vector2(x, y+ellipseHeight));

	// H -> A
	th->owner->tokens.emplace_back(CURVE_CUBIC,
	                        Vector2(x, y+ellipseHeight-kappaH),
	                        Vector2(x+ellipseWidth-kappaW, y),
	                        Vector2(x+ellipseWidth, y));

	// A -> B
	th->owner->tokens.emplace_back(STRAIGHT, Vector2(x+width-ellipseWidth, y));

	// B -> C
	th->owner->tokens.emplace_back(CURVE_CUBIC,
	                        Vector2(x+width-ellipseWidth+kappaW, y),
	                        Vector2(x+width, y+kappaH),
	                        Vector2(x+width, y+ellipseHeight));

	// C -> D
	th->owner->tokens.emplace_back(STRAIGHT, Vector2(x+width, y+height-ellipseHeight));

	th->owner->owner->requestInvalidation();
	
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
	th->owner->tokens.emplace_back(MOVE, Vector2(x+radius, y));

	// bottom
	th->owner->tokens.emplace_back(CURVE_CUBIC,
	                        Vector2(x+radius, y+kappa ),
	                        Vector2(x+kappa , y+radius),
	                        Vector2(x       , y+radius));

	// left
	th->owner->tokens.emplace_back(CURVE_CUBIC,
	                        Vector2(x-kappa , y+radius),
	                        Vector2(x-radius, y+kappa ),
	                        Vector2(x-radius, y       ));

	// top
	th->owner->tokens.emplace_back(CURVE_CUBIC,
	                        Vector2(x-radius, y-kappa ),
	                        Vector2(x-kappa , y-radius),
	                        Vector2(x       , y-radius));

	// back to right
	th->owner->tokens.emplace_back(CURVE_CUBIC,
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
	th->checkAndSetScaling();

	int x=args[0]->toInt();
	int y=args[1]->toInt();
	int width=args[2]->toInt();
	int height=args[3]->toInt();

	const Vector2 a(x,y);
	const Vector2 b(x+width,y);
	const Vector2 c(x+width,y+height);
	const Vector2 d(x,y+height);

	th->owner->tokens.emplace_back(MOVE, a);
	th->owner->tokens.emplace_back(STRAIGHT, b);
	th->owner->tokens.emplace_back(STRAIGHT, c);
	th->owner->tokens.emplace_back(STRAIGHT, d);
	th->owner->tokens.emplace_back(STRAIGHT, a);
	th->owner->owner->requestInvalidation();
	
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
	
	LINESTYLE2 style(-1);
	style.Color = RGBA(color, alpha);
	style.Width = thickness;
	th->owner->tokens.emplace_back(SET_STROKE, style);
	return NULL;
}

ASFUNCTIONBODY(Graphics,beginGradientFill)
{
	Graphics* th=static_cast<Graphics*>(obj);
	assert_and_throw(argslen>=4);
	th->checkAndSetScaling();

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
	th->owner->tokens.emplace_back(SET_FILL, style);
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
	FILLSTYLE style(-1);
	style.FillStyleType = SOLID_FILL;
	style.Color         = RGBA(color, alpha);
	th->owner->tokens.emplace_back(SET_FILL, style);
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
	return size;
}

bool Bitmap::fromJPEG(uint8_t *inData, int len)
{
	assert(!data);
	uint8_t* rgbData = ImageDecoder::decodeJPEG(inData, len, &size.width, &size.height);
	data = CairoRenderer::convertBitmapToCairo(rgbData, size.width, size.height);
	delete[] rgbData;
	if(!data)
	{
		LOG(LOG_ERROR, "Error decoding jpeg");
		return false;
	}
	return true;
}

void SimpleButton::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->super=Class<InteractiveObject>::getClass();
	c->max_level=c->super->max_level+1;
	c->setGetterByQName("upState","",Class<IFunction>::getFunction(_getUpState),true);
	c->setSetterByQName("upState","",Class<IFunction>::getFunction(_setUpState),true);
	c->setGetterByQName("downState","",Class<IFunction>::getFunction(_getDownState),true);
	c->setSetterByQName("downState","",Class<IFunction>::getFunction(_setDownState),true);
	c->setGetterByQName("overState","",Class<IFunction>::getFunction(_getOverState),true);
	c->setSetterByQName("overState","",Class<IFunction>::getFunction(_setOverState),true);
	c->setGetterByQName("hitTestState","",Class<IFunction>::getFunction(_getHitTestState),true);
	c->setSetterByQName("hitTestState","",Class<IFunction>::getFunction(_setHitTestState),true);
	c->setGetterByQName("enabled","",Class<IFunction>::getFunction(_getEnabled),true);
	c->setSetterByQName("enabled","",Class<IFunction>::getFunction(_setEnabled),true);
}

void SimpleButton::buildTraits(ASObject* o)
{
}

_NR<InteractiveObject> SimpleButton::hitTest(_NR<InteractiveObject> last, number_t x, number_t y)
{
	_NR<InteractiveObject> ret = NullRef;
	if (hitTestState)
	{
		number_t localX, localY;
		hitTestState->getMatrix().getInverted().multiply2D(x,y,localX,localY);
		ret = hitTestState->hitTest(last, localX, localY);
	}
	return ret;
}

ASFUNCTIONBODY(SimpleButton,_constructor)
{
	InteractiveObject::_constructor(obj,NULL,0);
	SimpleButton* th=static_cast<SimpleButton*>(obj);
	th->upState = NULL;
	th->downState = NULL;
	th->hitTestState = NULL;
	th->overState = NULL;

	if (argslen >= 1)
		th->upState = static_cast<DisplayObject*>(args[0]);
	if (argslen >= 2)
		th->overState = static_cast<DisplayObject*>(args[1]);
	if (argslen >= 3)
		th->downState = static_cast<DisplayObject*>(args[2]);
	if (argslen == 4)
		th->hitTestState = static_cast<DisplayObject*>(args[3]);

	if (th->upState) {
		th->upState->setOnStage(true);
		th->upState->requestInvalidation();
	}
	return NULL;
}

ASFUNCTIONBODY(SimpleButton,_getUpState)
{
	SimpleButton* th=static_cast<SimpleButton*>(obj);
	if(th->upState==NULL)
		return new Null;

	th->upState->incRef();

	return th->upState;
}

ASFUNCTIONBODY(SimpleButton,_setUpState)
{
	assert_and_throw(argslen == 1);
	SimpleButton* th=static_cast<SimpleButton*>(obj);
	th->upState =Class<DisplayObject>::cast(args[0]);
	th->upState->setOnStage(true);
	th->upState->requestInvalidation();
	return NULL;
}

ASFUNCTIONBODY(SimpleButton,_getHitTestState)
{
	SimpleButton* th=static_cast<SimpleButton*>(obj);
	if(th->hitTestState==NULL)
		return new Null;

	th->hitTestState->incRef();

	return th->hitTestState;
}

ASFUNCTIONBODY(SimpleButton,_setHitTestState)
{
	assert_and_throw(argslen == 1);
	SimpleButton* th=static_cast<SimpleButton*>(obj);
	th->hitTestState =Class<DisplayObject>::cast(args[0]);
	return NULL;
}

ASFUNCTIONBODY(SimpleButton,_getOverState)
{
	SimpleButton* th=static_cast<SimpleButton*>(obj);
	if(th->overState==NULL)
		return new Null;

	th->overState->incRef();

	return th->overState;
}

ASFUNCTIONBODY(SimpleButton,_setOverState)
{
	assert_and_throw(argslen == 1);
	SimpleButton* th=static_cast<SimpleButton*>(obj);
	th->overState =Class<DisplayObject>::cast(args[0]);
	return NULL;
}

ASFUNCTIONBODY(SimpleButton,_getDownState)
{
	SimpleButton* th=static_cast<SimpleButton*>(obj);
	if(th->downState==NULL)
		return new Null;

	th->downState->incRef();

	return th->downState;
}

ASFUNCTIONBODY(SimpleButton,_setDownState)
{
	assert_and_throw(argslen == 1);
	SimpleButton* th=static_cast<SimpleButton*>(obj);
	th->downState =Class<DisplayObject>::cast(args[0]);
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

bool SimpleButton::getBounds(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const
{
	//FIXME: handle mouse down and over states too
	if (upState)
		return upState->getBounds(xmin, xmax, ymin, ymax);

	return false;
}

void SimpleButton::Render(bool maskEnabled)
{
	//FIXME: handle mouse down and over states too
	if (upState)
		upState->Render(maskEnabled);
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
