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
#include "flash/geom/flashgeom.h"
#include "flash/system/flashsystem.h"
#include "parsing/streams.h"
#include "compat.h"
#include "class.h"
#include "backends/rendering.h"
#include "backends/geometry.h"
#include "backends/image.h"
#include "backends/glmatrices.h"
#include "compat.h"
#include "flash/accessibility/flashaccessibility.h"
#include "argconv.h"

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
REGISTER_CLASS_NAME(AVM1Movie);
REGISTER_CLASS_NAME(Shader);

ATOMIC_INT32(DisplayObject::instanceCount);

std::ostream& lightspark::operator<<(std::ostream& s, const DisplayObject& r)
{
	s << "[" << r.getClass()->class_name << "]";
	if(!r.name.empty())
		s << " name: " << r.name;
	return s;
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
}

ASFUNCTIONBODY_GETTER(LoaderInfo,parameters);

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
		th->contentLoaderInfo->setVariableByQName("parameters","",p.getPtr(),DECLARED_TRAIT);
	}
	return NULL;
}

ASFUNCTIONBODY(Loader,_getContent)
{
	Loader* th=static_cast<Loader*>(obj);
	SpinlockLocker l(th->contentSpinlock);
	_NR<ASObject> ret=th->content;
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
	URLRequest* r=Class<URLRequest>::dyncast(args[0]);
	if(r==NULL)
		throw Class<ArgumentError>::getInstanceS("Wrong argument in Loader::load");
	th->url=r->getRequestURL();
	r->getPostData(th->postData);
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
	assert_and_throw(args[0]->getClass() && 
			args[0]->getClass()->isSubClass(Class<ByteArray>::getClass()));
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
	content.reset();
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
	c->setSuper(Class<DisplayObjectContainer>::getRef());
	c->setDeclaredMethodByQName("contentLoaderInfo","",Class<IFunction>::getFunction(_getContentLoaderInfo),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("content","",Class<IFunction>::getFunction(_getContent),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("loadBytes","",Class<IFunction>::getFunction(loadBytes),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("load","",Class<IFunction>::getFunction(load),NORMAL_METHOD,true);
}

void Loader::buildTraits(ASObject* o)
{
}

void Loader::execute()
{
	assert(source==URL || source==BYTES);

	if(source==URL)
	{
		//TODO: add security checks
		LOG(LOG_CALLS,_("Loader async execution ") << url);
		assert_and_throw(postData.empty());
		downloader=sys->downloadManager->download(url, false, contentLoaderInfo.getPtr());
		downloader->waitForData(); //Wait for some data, making sure our check for failure is working
		if(downloader->hasFailed()) //Check to see if the download failed for some reason
		{
			LOG(LOG_ERROR, "Loader::execute(): Download of URL failed: " << url);
			sys->currentVm->addEvent(contentLoaderInfo,_MR(Class<IOErrorEvent>::getInstanceS()));
			sys->downloadManager->destroy(downloader);
			return;
		}
		sys->currentVm->addEvent(contentLoaderInfo,_MR(Class<Event>::getInstanceS("open")));
		istream s(downloader);
		ParseThread local_pt(s,this,url.getParsedURL());
		local_pt.run();
		{
			//Acquire the lock to ensure consistency in threadAbort
			SpinlockLocker l(downloaderLock);
			sys->downloadManager->destroy(downloader);
			downloader=NULL;
		}

		_NR<DisplayObject> obj=local_pt.getParsedObject();
		if(obj.isNull())
		{
			// The stream did not contain RootMovieClip or Bitmap
			sys->currentVm->addEvent(contentLoaderInfo,_MR(Class<IOErrorEvent>::getInstanceS()));
			return;
		}

		// Wait until the object is constructed before adding
		// to the Loader
		while (!obj->isConstructed() && !aborting)
			compat_msleep(100);

		if(aborting)
			return;

		setContent(obj);
		contentLoaderInfo->sendInit();
	}
	else if(source==BYTES)
	{
		//TODO: set bytesLoaded and bytesTotal
		assert_and_throw(bytes->bytes);

		//We only support swf files now
		assert_and_throw(memcmp(bytes->bytes,"CWS",3)==0);

		bytes_buf bb(bytes->bytes,bytes->len);
		istream s(&bb);

		ParseThread local_pt(s,this);
		local_pt.run();
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

void Loader::setContent(_R<DisplayObject> o)
{
	{
		Locker l(mutexDisplayList);
		dynamicDisplayList.clear();
	}

	{
		SpinlockLocker l(contentSpinlock);
		content=o;
	}

	// _addChild may cause AS code to run, release locks beforehand.
	_addChildAt(o, 0);
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
	c->setSuper(Class<DisplayObjectContainer>::getRef());
	c->setDeclaredMethodByQName("graphics","",Class<IFunction>::getFunction(_getGraphics),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("startDrag","",Class<IFunction>::getFunction(_startDrag),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("stopDrag","",Class<IFunction>::getFunction(_stopDrag),NORMAL_METHOD,true);
}

void Sprite::buildTraits(ASObject* o)
{
}

Vector2f DisplayObject::getXY()
{
	Vector2f ret;
	if(ACQUIRE_READ(useMatrix))
	{
		ret.x = getMatrix().TranslateX;
		ret.y = getMatrix().TranslateY;
	}
	else
	{
		ret.x = tx;
		ret.y = ty;
	}
	return ret;
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
			throw ArgumentError("Wrong type");
		bounds = new RECT(rect->getRect());
	}

	Vector2f offset;
	if(!lockCenter)
	{
		offset = -th->getParent()->getLocalMousePos();
		offset += th->getXY();
	}

	th->incRef();
	sys->getInputThread()->startDrag(_MR(th), bounds, offset);
	return NULL;
}

ASFUNCTIONBODY(Sprite,_stopDrag)
{
	Sprite* th=Class<Sprite>::cast(obj);
	sys->getInputThread()->stopDrag(th);
	return NULL;
}

bool DisplayObjectContainer::boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const
{
	bool ret = false;

	if(dynamicDisplayList.empty())
		return false;

	Locker l(mutexDisplayList);
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

bool DisplayObject::getBounds(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const
{
	if(!isConstructed())
		return false;

	bool ret=boundsRect(xmin,xmax,ymin,ymax);
	if(ret)
	{
		//TODO: take rotation into account
		getMatrix().multiply2D(xmin,ymin,xmin,ymin);
		getMatrix().multiply2D(xmax,ymax,xmax,ymax);
	}
	return ret;
}

number_t DisplayObject::getNominalWidth()
{
	number_t xmin, xmax, ymin, ymax;

	if(!isConstructed())
		return 0;

	bool ret=boundsRect(xmin,xmax,ymin,ymax);
	return ret?(xmax-xmin):0;
}

number_t DisplayObject::getNominalHeight()
{
	number_t xmin, xmax, ymin, ymax;

	if(!isConstructed())
		return 0;

	bool ret=boundsRect(xmin,xmax,ymin,ymax);
	return ret?(ymax-ymin):0;
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

void DisplayObjectContainer::renderImpl(bool maskEnabled, number_t t1,number_t t2,number_t t3,number_t t4) const
{
	Locker l(mutexDisplayList);
	//Now draw also the display list
	list<_R<DisplayObject>>::const_iterator it=dynamicDisplayList.begin();
	for(;it!=dynamicDisplayList.end();++it)
		(*it)->Render(maskEnabled);
}

void Sprite::renderImpl(bool maskEnabled, number_t t1,number_t t2,number_t t3,number_t t4) const
{
	//Draw the dynamically added graphics, if any
	//Should clean only the bounds of the graphics
	if(!tokensEmpty())
		defaultRender(maskEnabled);

	DisplayObjectContainer::renderImpl(maskEnabled, t1, t2, t3, t4);
}

void DisplayObject::Render(bool maskEnabled)
{
	if(!isConstructed() || skipRender(maskEnabled))
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
	auto it=blueprint.begin();
	for(;it!=blueprint.end();++it)
		(*it)->execute(displayList.getPtr());
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

MovieClip::MovieClip():totalFrames_unreliable(1),framesLoaded(0)
{
	frames.emplace_back(Frame());
	scenes.resize(1);
}

MovieClip::MovieClip(const MovieClip& r):frames(r.frames),
	totalFrames_unreliable(r.totalFrames_unreliable),framesLoaded(r.framesLoaded),
	frameScripts(r.frameScripts),scenes(r.scenes),
	state(r.state)
{
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
void MovieClip::addToFrame(_R<DisplayListTag> t)
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
			if(!th->scenes[i].labels[j].name.empty())
				label = th->scenes[i].labels[j].name;
		}
	}

	if(label.empty())
		return new Null();
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
		ret->set(i, Class<FrameLabel>::getInstanceS(sc.labels[i]));
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

DisplayObject::DisplayObject():useMatrix(true),tx(0),ty(0),rotation(0),sx(1),sy(1),maskOf(NULL),parent(NULL),mask(NULL),onStage(false),
	loaderInfo(NULL),alpha(1.0),visible(true),invalidateQueueNext(NULL)
{
	name = tiny_string("instance") + Integer::toString(ATOMIC_INCREMENT(instanceCount));
}

DisplayObject::DisplayObject(const DisplayObject& d):useMatrix(true),tx(d.tx),ty(d.ty),rotation(d.rotation),sx(d.sx),sy(d.sy),maskOf(NULL),
	parent(NULL),mask(NULL),onStage(false),loaderInfo(NULL),alpha(d.alpha),visible(d.visible),name(d.name),invalidateQueueNext(NULL)
{
	assert(!d.isConstructed());
}

DisplayObject::~DisplayObject() {}

void DisplayObject::finalize()
{
	EventDispatcher::finalize();
	maskOf.reset();
	parent.reset();
	mask.reset();
	loaderInfo.reset();
	invalidateQueueNext.reset();
	accessibilityProperties.reset();
}

void DisplayObject::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<EventDispatcher>::getRef());
	c->setDeclaredMethodByQName("loaderInfo","",Class<IFunction>::getFunction(_getLoaderInfo),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("width","",Class<IFunction>::getFunction(_getWidth),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("width","",Class<IFunction>::getFunction(_setWidth),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("scaleX","",Class<IFunction>::getFunction(_getScaleX),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("scaleX","",Class<IFunction>::getFunction(_setScaleX),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("scaleY","",Class<IFunction>::getFunction(_getScaleY),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("scaleY","",Class<IFunction>::getFunction(_setScaleY),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("x","",Class<IFunction>::getFunction(_getX),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("x","",Class<IFunction>::getFunction(_setX),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("y","",Class<IFunction>::getFunction(_getY),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("y","",Class<IFunction>::getFunction(_setY),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("height","",Class<IFunction>::getFunction(_getHeight),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("height","",Class<IFunction>::getFunction(_setHeight),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("visible","",Class<IFunction>::getFunction(_getVisible),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("visible","",Class<IFunction>::getFunction(_setVisible),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("rotation","",Class<IFunction>::getFunction(_getRotation),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("rotation","",Class<IFunction>::getFunction(_setRotation),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("name","",Class<IFunction>::getFunction(_getName),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("name","",Class<IFunction>::getFunction(_setName),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("parent","",Class<IFunction>::getFunction(_getParent),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("root","",Class<IFunction>::getFunction(_getRoot),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("blendMode","",Class<IFunction>::getFunction(_getBlendMode),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("blendMode","",Class<IFunction>::getFunction(undefinedFunction),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("scale9Grid","",Class<IFunction>::getFunction(_getScale9Grid),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("scale9Grid","",Class<IFunction>::getFunction(undefinedFunction),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("stage","",Class<IFunction>::getFunction(_getStage),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("mask","",Class<IFunction>::getFunction(_getMask),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("mask","",Class<IFunction>::getFunction(_setMask),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("alpha","",Class<IFunction>::getFunction(_getAlpha),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("alpha","",Class<IFunction>::getFunction(_setAlpha),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("cacheAsBitmap","",Class<IFunction>::getFunction(undefinedFunction),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("cacheAsBitmap","",Class<IFunction>::getFunction(undefinedFunction),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("opaqueBackground","",Class<IFunction>::getFunction(undefinedFunction),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("opaqueBackground","",Class<IFunction>::getFunction(undefinedFunction),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("getBounds","",Class<IFunction>::getFunction(_getBounds),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("mouseX","",Class<IFunction>::getFunction(_getMouseX),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("mouseY","",Class<IFunction>::getFunction(_getMouseY),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("localToGlobal","",Class<IFunction>::getFunction(localToGlobal),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("globalToLocal","",Class<IFunction>::getFunction(globalToLocal),NORMAL_METHOD,true);
	REGISTER_GETTER_SETTER(c,accessibilityProperties);
}

ASFUNCTIONBODY_GETTER_SETTER(DisplayObject,accessibilityProperties);

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

float DisplayObject::getConcatenatedAlpha() const
{
	if(parent.isNull())
		return alpha;
	else
		return parent->getConcatenatedAlpha()*alpha;
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
	if(Matrix.RotateSkew0 || Matrix.RotateSkew1)
		LOG(LOG_ERROR,"valFromMatrix may has dropped rotation!");
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
	/* cachedSurface is only modified from within the render thread
	 * so we need no locking here */
	if(!cachedSurface.tex.isValid())
		return;
	float enableMaskLookup=0.0f;
	//If the maskEnabled is already set we are the mask!
	if(!maskEnabled && rt->isMaskPresent())
	{
		rt->renderMaskToTmpBuffer();
		enableMaskLookup=1.0f;
	}
	lsglPushMatrix();
	lsglLoadIdentity();
	rt->setMatrixUniform(LSGL_MODELVIEW);
	glUniform1f(rt->maskUniform, enableMaskLookup);
	glUniform1f(rt->yuvUniform, 0);
	glUniform1f(rt->alphaUniform, cachedSurface.alpha);
	rt->renderTextured(cachedSurface.tex, cachedSurface.xOffset, cachedSurface.yOffset, cachedSurface.tex.width, cachedSurface.tex.height);
	lsglPopMatrix();
	rt->setMatrixUniform(LSGL_MODELVIEW);
}

void DisplayObject::computeDeviceBoundsForRect(number_t xmin, number_t xmax, number_t ymin, number_t ymax,
		int32_t& outXMin, int32_t& outYMin, uint32_t& outWidth, uint32_t& outHeight) const
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
//TODO: Fix precision issues, Adobe seems to do the matrix mult with twips and rounds the results, 
//this way they have less pb with precision.
void DisplayObject::localToGlobal(number_t xin, number_t yin, number_t& xout, number_t& yout) const
{
	getMatrix().multiply2D(xin, yin, xout, yout);
	if(!parent.isNull())
		parent->localToGlobal(xout, yout, xout, yout);
}
//TODO: Fix precision issues
void DisplayObject::globalToLocal(number_t xin, number_t yin, number_t& xout, number_t& yout) const
{
	getConcatenatedMatrix().getInverted().multiply2D(xin, yin, xout, yout);
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
		/*NOTE: By tests we can assert that added/addedToStage is dispatched
		  immediately when addChild is called. On the other hand setOnStage may
		  be also called outside of the VM thread (for example by Loader::execute)
		  so we have to check isVmThread and act accordingly. If in the future
		  asynchronous uses of setOnStage are removed the code can be simplified
		  by removing the !isVmThread case.
		*/
		if(onStage==true && isConstructed())
		{
			this->incRef();
			_R<Event> e=_MR(Class<Event>::getInstanceS("addedToStage"));
			if(isVmThread)
				ABCVm::publicHandleEvent(_MR(this),e);
			else
				getVm()->addEvent(_MR(this),e);
		}
		else if(onStage==false)
		{
			this->incRef();
			_R<Event> e=_MR(Class<Event>::getInstanceS("removedFromStage"));
			if(isVmThread)
				ABCVm::publicHandleEvent(_MR(this),e);
			else
				getVm()->addEvent(_MR(this),e);
		}
	}
}

ASFUNCTIONBODY(DisplayObject,_setAlpha)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	number_t val;
	ARG_UNPACK (val);
	//Clip val
	val=dmax(0,val);
	val=dmin(val,1);
	th->alpha=val;
	if(th->onStage)
		th->requestInvalidation();
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
	if(args[0] && args[0]->getClass() && args[0]->getClass()->isSubClass(Class<DisplayObject>::getClass()))
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

void DisplayObject::setX(number_t val)
{
	if(ACQUIRE_READ(useMatrix))
	{
		valFromMatrix();
		RELEASE_WRITE(useMatrix,false);
	}
	tx=val;
	if(onStage)
		requestInvalidation();
}

void DisplayObject::setY(number_t val)
{
	if(ACQUIRE_READ(useMatrix))
	{
		valFromMatrix();
		RELEASE_WRITE(useMatrix,false);
	}
	ty=val;
	if(onStage)
		requestInvalidation();
}

ASFUNCTIONBODY(DisplayObject,_setX)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	assert_and_throw(argslen==1);
	number_t val=args[0]->toNumber();
	th->setX(val);
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
	th->setY(val);
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

	/* According to tests returning root.loaderInfo is the correct
	 * behaviour, even though the documentation states that only
	 * the main class should have non-null loaderInfo. */
	_NR<RootMovieClip> r=th->getRoot();
	if(r.isNull() || r->loaderInfo.isNull())
		return new Undefined;
	
	r->loaderInfo->incRef();
	return r->loaderInfo.getPtr();
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

	th->localToGlobal(pt->getX(), pt->getY(), tempx, tempy);

	return Class<Point>::getInstanceS(tempx, tempy);
}

ASFUNCTIONBODY(DisplayObject,globalToLocal)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	assert_and_throw(argslen == 1);

	Point* pt=static_cast<Point*>(args[0]);

	number_t tempx, tempy;

	th->globalToLocal(pt->getX(), pt->getY(), tempx, tempy);

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

number_t DisplayObject::computeHeight()
{
	number_t x1,x2,y1,y2;
	bool ret=getBounds(x1,x2,y1,y2);

	return (ret)?(y2-y1):0;
}

number_t DisplayObject::computeWidth()
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
	return abstract_d(th->computeWidth());
}

ASFUNCTIONBODY(DisplayObject,_setWidth)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	number_t newwidth=args[0]->toNumber();

	number_t xmin,xmax,y1,y2;
	if(!th->boundsRect(xmin,xmax,y1,y2))
		return NULL;

	number_t width=xmax-xmin;
	if(width==0) //Cannot scale, nothing to do (See Reference)
		return NULL;
	
	if(width*th->sx!=newwidth) //If the width is changing, calculate new scale
	{
		if(ACQUIRE_READ(th->useMatrix))
		{
			th->valFromMatrix();
			RELEASE_WRITE(th->useMatrix,false);
		}
		th->sx = newwidth/width;
		if(th->onStage)
			th->requestInvalidation();
	}
	return NULL;
}

ASFUNCTIONBODY(DisplayObject,_getHeight)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	return abstract_d(th->computeHeight());
}

ASFUNCTIONBODY(DisplayObject,_setHeight)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	number_t newheight=args[0]->toNumber();

	number_t x1,x2,ymin,ymax;
	if(!th->boundsRect(x1,x2,ymin,ymax))
		return NULL;

	number_t height=ymax-ymin;
	if(height==0) //Cannot scale, nothing to do (See Reference)
		return NULL;
	
	if(height*th->sy!=newheight) //If the height is changing, calculate new scale
	{
		if(ACQUIRE_READ(th->useMatrix))
		{
			th->valFromMatrix();
			RELEASE_WRITE(th->useMatrix,false);
		}
		th->sy=newheight/height;
		if(th->onStage)
			th->requestInvalidation();
	}
	return NULL;
}

Vector2f DisplayObject::getLocalMousePos()
{
	return getConcatenatedMatrix().getInverted().multiply2D(sys->getInputThread()->getMousePos());
}

ASFUNCTIONBODY(DisplayObject,_getMouseX)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	return abstract_d(th->getLocalMousePos().x);
}

ASFUNCTIONBODY(DisplayObject,_getMouseY)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	return abstract_d(th->getLocalMousePos().y);
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

DisplayObjectContainer::DisplayObjectContainer():mouseChildren(true),mutexDisplayList("mutexDisplayList")
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
		multiname objName;
		objName.name_type=multiname::NAME_STRING;
		objName.name_s=obj->name;
		objName.ns.push_back(nsNameAndKind("",NAMESPACE));
		deleteVariableByMultiname(objName);
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
		multiname objName;
		objName.name_type=multiname::NAME_STRING;
		objName.name_s=obj->name;
		objName.ns.push_back(nsNameAndKind("",NAMESPACE));
		// If this function is called by PlaceObject tag
		// before the properties are initialized, we need to
		// initialize the property here to make sure that it
		// will be a declared property and not a dynamic one.
		if(hasPropertyByMultiname(objName,true))
			setVariableByMultiname(objName,obj);
		else
			setVariableByQName(objName.name_s,objName.ns[0],obj,DECLARED_TRAIT);
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
	depthToLegacyChild.left.at(depth)->setMatrix(mat);
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

InteractiveObject::InteractiveObject():mouseEnabled(true),doubleClickEnabled(false)
{
}

InteractiveObject::~InteractiveObject()
{
	if(sys && sys->getInputThread())
		sys->getInputThread()->removeListener(this);
}

_NR<InteractiveObject> DisplayObject::hitTest(_NR<InteractiveObject> last, number_t x, number_t y, HIT_TYPE type)
{
	/*number_t t1,t2,t3,t4;
	bool notEmpty=boundsRect(t1,t2,t3,t4);
	if(!notEmpty)
		return NullRef;
	if(x<t1 || x>t2 || y<t3 || y>t4)
		return NullRef;
	 */
	if(!visible || !isConstructed())
		return NullRef;

	hitTestPrologue();
	_NR<InteractiveObject> ret = hitTestImpl(last, x,y, type);
	hitTestEpilogue();
	return ret;
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
	cout << "Size: " << dynamicDisplayList.size() << endl;
	list<_R<DisplayObject> >::const_iterator it=dynamicDisplayList.begin();
	for(;it!=dynamicDisplayList.end();++it)
		cout << (*it)->getClass()->class_name << endl;
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
	if(!child->getParent())
		return false;
	assert_and_throw(child->getParent()==this);

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
		return new Null;
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
	assert_and_throw(args[0] && args[0]->getClass() && 
		args[0]->getClass()->isSubClass(Class<DisplayObject>::getClass()));

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

bool Shape::isOpaque(number_t x, number_t y) const
{
	return TokenContainer::isOpaqueImpl(x, y);
}

bool Sprite::isOpaque(number_t x, number_t y) const
{
	return (TokenContainer::isOpaqueImpl(x, y)) || (DisplayObjectContainer::isOpaque(x,y));
}

bool DisplayObjectContainer::isOpaque(number_t x, number_t y) const
{
	list<_R<DisplayObject>>::const_iterator it=dynamicDisplayList.begin();
	number_t lx, ly;
	for(;it!=dynamicDisplayList.end();++it)
	{
		//x y are local coordinates of the container, should be local coord of *it
		((*it)->getMatrix()).getInverted().multiply2D(x,y,lx,ly);
		if(((*it)->isOpaque(lx,ly)))
		{			
			return true;		
		}
	}
	return false;
}

void TokenContainer::renderImpl(bool maskEnabled, number_t t1, number_t t2, number_t t3, number_t t4) const
{
	//if(!owner->isSimple())
	//	rt->acquireTempBuffer(t1,t2,t3,t4);

	owner->defaultRender(maskEnabled);

	//if(!owner->isSimple())
	//	rt->blitTempBuffer(t1,t2,t3,t4);
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

Stage::Stage()
{
	onStage = true;
}

ASFUNCTIONBODY(Stage,_constructor)
{
	return NULL;
}

_NR<InteractiveObject> Stage::hitTestImpl(_NR<InteractiveObject> last, number_t x, number_t y, DisplayObject::HIT_TYPE type)
{
	_NR<InteractiveObject> ret;
	ret = DisplayObjectContainer::hitTestImpl(last, x, y, type);
	if(!ret && isHittable(type))
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
	int32_t x,y;
	uint32_t width,height;
	number_t bxmin,bxmax,bymin,bymax;
	if(boundsRect(bxmin,bxmax,bymin,bymax)==false)
	{
		//No contents, nothing to do
		return;
	}

	owner->computeDeviceBoundsForRect(bxmin,bxmax,bymin,bymax,x,y,width,height);
	if(width==0 || height==0)
		return;
	CairoRenderer* r=new CairoTokenRenderer(owner, owner->cachedSurface, tokens,
				owner->getConcatenatedMatrix(), x, y, width, height, scaling,
				owner->getConcatenatedAlpha());
	sys->addJob(r);
}

bool TokenContainer::isOpaqueImpl(number_t x, number_t y) const
{
	return CairoTokenRenderer::isOpaque(tokens, scaling, x, y);
}

_NR<InteractiveObject> TokenContainer::hitTestImpl(_NR<InteractiveObject> last, number_t x, number_t y, DisplayObject::HIT_TYPE type) const
{
	//TODO: test against the CachedSurface
	if(CairoTokenRenderer::hitTest(tokens, scaling, x, y))
	{
		if(sys->getInputThread()->isMaskPresent())
		{
			number_t globalX, globalY;
			owner->getConcatenatedMatrix().multiply2D(x,y,globalX,globalY);
			if(!sys->getInputThread()->isMasked(globalX, globalY))//You must be under the mask to be hit
				return NullRef;
		}
		return last;
	}
	return NullRef;
}

bool TokenContainer::boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const
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
	c->setDeclaredMethodByQName("clear","",Class<IFunction>::getFunction(clear),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("drawRect","",Class<IFunction>::getFunction(drawRect),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("drawRoundRect","",Class<IFunction>::getFunction(drawRoundRect),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("drawCircle","",Class<IFunction>::getFunction(drawCircle),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("moveTo","",Class<IFunction>::getFunction(moveTo),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("curveTo","",Class<IFunction>::getFunction(curveTo),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("cubicCurveTo","",Class<IFunction>::getFunction(cubicCurveTo),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("lineTo","",Class<IFunction>::getFunction(lineTo),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("lineStyle","",Class<IFunction>::getFunction(lineStyle),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("beginFill","",Class<IFunction>::getFunction(beginFill),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("beginGradientFill","",Class<IFunction>::getFunction(beginGradientFill),NORMAL_METHOD,true);
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

	th->owner->tokens.emplace_back(GeomToken(CURVE_QUADRATIC,
	                        Vector2(controlX, controlY),
	                        Vector2(anchorX, anchorY)));
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

	th->owner->tokens.emplace_back(GeomToken(CURVE_CUBIC,
	                        Vector2(control1X, control1Y),
	                        Vector2(control2X, control2Y),
	                        Vector2(anchorX, anchorY)));
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

	th->owner->tokens.emplace_back(GeomToken(MOVE, a));
	th->owner->tokens.emplace_back(GeomToken(STRAIGHT, b));
	th->owner->tokens.emplace_back(GeomToken(STRAIGHT, c));
	th->owner->tokens.emplace_back(GeomToken(STRAIGHT, d));
	th->owner->tokens.emplace_back(GeomToken(STRAIGHT, a));
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
	th->owner->tokens.emplace_back(GeomToken(SET_STROKE, style));
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

	//assert_and_throw(args[3]->getObjectType()==T_ARRAY);
	//Work around for bug in YouTube player of July 13 2011
	if(args[3]->getObjectType()==T_UNDEFINED)
		return NULL;
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

	if(argslen > 4 && args[4]->getClass()==Class<Matrix>::getClass())
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
	FILLSTYLE style(-1);
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

Bitmap::Bitmap(std::istream *s, FILE_TYPE type) : TokenContainer(this), size(0,0), data(NULL)
{
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
			fromJPEG(*s);
			break;
		case FT_PNG:
		case FT_GIF:
			LOG(LOG_NOT_IMPLEMENTED, _("PNGs and GIFs are not yet supported"));
			break;
		default:
			LOG(LOG_ERROR,_("Unsupported image type"));
			break;
	}
}

void Bitmap::sinit(Class_base* c)
{
//	c->constructor=Class<IFunction>::getFunction(_constructor);
	c->setConstructor(NULL);
	c->setSuper(Class<DisplayObject>::getRef());
}

bool Bitmap::boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const
{
	if(!data)
		return false;

	return TokenContainer::boundsRect(xmin,xmax,ymin,ymax);
}

_NR<InteractiveObject> Bitmap::hitTestImpl(_NR<InteractiveObject> last, number_t x, number_t y, DisplayObject::HIT_TYPE type)
{
	return NullRef;
}

IntSize Bitmap::getBitmapSize() const
{
	return size;
}

bool Bitmap::fromRGB(uint8_t* rgb, uint32_t width, uint32_t height, bool hasAlpha)
{
	size.width = width;
	size.height = height;
	if(hasAlpha)
		data = CairoRenderer::convertBitmapWithAlphaToCairo(rgb, width, height);
	else
		data = CairoRenderer::convertBitmapToCairo(rgb, width, height);
	delete[] rgb;
	if(!data)
	{
		LOG(LOG_ERROR, "Error decoding image");
		return false;
	}

	FILLSTYLE style(-1);
	style.FillStyleType=CLIPPED_BITMAP;
	style.bitmap=this;
	tokens.clear();
	tokens.emplace_back(GeomToken(SET_FILL, style));
	tokens.emplace_back(GeomToken(MOVE, Vector2(0, 0)));
	tokens.emplace_back(GeomToken(STRAIGHT, Vector2(0, size.height)));
	tokens.emplace_back(GeomToken(STRAIGHT, Vector2(size.width, size.height)));
	tokens.emplace_back(GeomToken(STRAIGHT, Vector2(size.width, 0)));
	tokens.emplace_back(GeomToken(STRAIGHT, Vector2(0, 0)));
	requestInvalidation();

	return true;
}

bool Bitmap::fromJPEG(uint8_t *inData, int len)
{
	assert(!data);
	uint8_t *rgb=ImageDecoder::decodeJPEG(inData, len, &size.width, &size.height);
	return fromRGB(rgb, size.width, size.height, false);
}

bool Bitmap::fromJPEG(std::istream &s)
{
	assert(!data);
	uint8_t *rgb=ImageDecoder::decodeJPEG(s, &size.width, &size.height);
	return fromRGB(rgb, size.width, size.height, false);
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
		number_t localX, localY;
		hitTestState->getMatrix().getInverted().multiply2D(x,y,localX,localY);
		this->incRef();
		ret = hitTestState->hitTest(_MR(this), localX, localY, type);
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

SimpleButton::SimpleButton(DisplayObject *dS, DisplayObject *hTS,
						   DisplayObject *oS, DisplayObject *uS)
	: downState(dS), hitTestState(hTS), overState(oS), upState(uS),
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

	if (argslen >= 1)
	{
		th->upState = _MNR(static_cast<DisplayObject*>(args[0]));
		th->upState->incRef();
	}
	if (argslen >= 2)
	{
		th->overState = _MNR(static_cast<DisplayObject*>(args[1]));
		th->overState->incRef();
	}
	if (argslen >= 3)
	{
		th->downState = _MNR(static_cast<DisplayObject*>(args[2]));
		th->downState->incRef();
	}
	if (argslen == 4)
	{
		th->hitTestState = _MNR(static_cast<DisplayObject*>(args[3]));
		th->hitTestState->incRef();
	}

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
		return new Null;

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
		return new Null;

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
		return new Null;

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
		return new Null;

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

/* Display objects have no children in general,
 * so we skip to calling the constructor, if necessary.
 * This is called in vm's thread context */
void DisplayObject::initFrame()
{
	if(!isConstructed() && getClass())
	{
		getClass()->handleConstruction(this,NULL,0,true);

		if(!onStage)
			return;

		/* addChild has already been called for this object,
		 * but addedToStage is delayed until after construction.
		 * This is from "Order of Operations".
		 */
		/* TODO: also dispatch event "added" */
		/* TODO: should we directly call handleEventPublic? */
		this->incRef();
		getVm()->addEvent(_MR(this),_MR(Class<Event>::getInstanceS("addedToStage")));
	}
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

	if(framesLoaded)
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
		frameScripts[state.FP]->call(NULL,NULL,0);

}

/* This is run in vm's thread context */
void DisplayObjectContainer::advanceFrame()
{
	list<_R<DisplayObject>>::const_iterator it=dynamicDisplayList.begin();
	for(;it!=dynamicDisplayList.end();it++)
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
