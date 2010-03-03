/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009  Alessandro Pignotti (a.pignotti@sssup.it)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
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
#include "streams.h"
#include "compat.h"
#include "class.h"

#include <GL/glew.h>
#include <fstream>
#include <limits>
using namespace std;
using namespace lightspark;

extern TLSDATA SystemState* sys;
extern TLSDATA RenderThread* rt;
extern TLSDATA ParseThread* pt;

REGISTER_CLASS_NAME(LoaderInfo);
REGISTER_CLASS_NAME(MovieClip);
REGISTER_CLASS_NAME(DisplayObject);
REGISTER_CLASS_NAME(DisplayObjectContainer);
REGISTER_CLASS_NAME(Sprite);
REGISTER_CLASS_NAME(Loader);
REGISTER_CLASS_NAME(Shape);
REGISTER_CLASS_NAME(Stage);
REGISTER_CLASS_NAME(Graphics);
REGISTER_CLASS_NAME(LineScaleMode);
REGISTER_CLASS_NAME(StageScaleMode);
REGISTER_CLASS_NAME(StageAlign);
REGISTER_CLASS_NAME(Bitmap);

void LoaderInfo::sinit(Class_base* c)
{
	assert(c->constructor==NULL);
	c->constructor=new Function(_constructor);
	c->super=Class<EventDispatcher>::getClass();
	c->max_level=c->super->max_level+1;
}

void LoaderInfo::buildTraits(ASObject* o)
{
	o->setGetterByQName("loaderURL","",new Function(_getLoaderUrl));
	o->setGetterByQName("url","",new Function(_getUrl));
	o->setGetterByQName("bytesLoaded","",new Function(_getBytesLoaded));
	o->setGetterByQName("bytesTotal","",new Function(_getBytesTotal));
	o->setGetterByQName("applicationDomain","",new Function(_getApplicationDomain));
	o->setGetterByQName("sharedEvents","",new Function(_getSharedEvents));
}

ASFUNCTIONBODY(LoaderInfo,_constructor)
{
	LoaderInfo* th=static_cast<LoaderInfo*>(obj->implementation);
	EventDispatcher::_constructor(obj,NULL,0);
	th->sharedEvents=Class<EventDispatcher>::getInstanceS(true);
	return NULL;
}

ASFUNCTIONBODY(LoaderInfo,_getLoaderUrl)
{
	LoaderInfo* th=static_cast<LoaderInfo*>(obj->implementation);
	return Class<ASString>::getInstanceS(true,th->loaderURL)->obj;
}

ASFUNCTIONBODY(LoaderInfo,_getSharedEvents)
{
	LoaderInfo* th=static_cast<LoaderInfo*>(obj->implementation);
	th->sharedEvents->obj->incRef();
	return th->sharedEvents->obj;
}

ASFUNCTIONBODY(LoaderInfo,_getUrl)
{
	LoaderInfo* th=static_cast<LoaderInfo*>(obj->implementation);
	return Class<ASString>::getInstanceS(true,th->url)->obj;
}

ASFUNCTIONBODY(LoaderInfo,_getBytesLoaded)
{
	LoaderInfo* th=static_cast<LoaderInfo*>(obj->implementation);
	return abstract_i(th->bytesLoaded);
}

ASFUNCTIONBODY(LoaderInfo,_getBytesTotal)
{
	LoaderInfo* th=static_cast<LoaderInfo*>(obj->implementation);
	return abstract_i(th->bytesTotal);
}

ASFUNCTIONBODY(LoaderInfo,_getApplicationDomain)
{
	return Class<ApplicationDomain>::getInstanceS(true)->obj;
}

ASFUNCTIONBODY(Loader,_constructor)
{
	Loader* th=static_cast<Loader*>(obj->implementation);
	th->contentLoaderInfo=Class<LoaderInfo>::getInstanceS(true);
	return NULL;
}

ASFUNCTIONBODY(Loader,_getContentLoaderInfo)
{
	Loader* th=static_cast<Loader*>(obj->implementation);
	th->contentLoaderInfo->obj->incRef();
	return th->contentLoaderInfo->obj;
}

ASFUNCTIONBODY(Loader,load)
{
	Loader* th=static_cast<Loader*>(obj->implementation);
/*	if(th->loading)
		return NULL;
	th->loading=true;*/
	abort();
/*	if(args->at(0)->getClassName()!="URLRequest")
	{
		LOG(ERROR,"ArgumentError");
		abort();
	}*/
	URLRequest* r=static_cast<URLRequest*>(args[0]->implementation);
	th->url=r->url;
	th->source=URL;
	sys->cur_thread_pool->addJob(th);
	return NULL;
}

ASFUNCTIONBODY(Loader,loadBytes)
{
	Loader* th=static_cast<Loader*>(obj->implementation);
	if(th->loading)
		return NULL;
	//Find the actual ByteArray object
	assert(argslen>=1);
	assert(args[0]->prototype->isSubClass(Class<ByteArray>::getClass()));
	th->bytes=static_cast<ByteArray*>(args[0]->implementation);
	if(th->bytes->bytes)
	{
		th->loading=true;
		th->source=BYTES;
		sys->cur_thread_pool->addJob(th);
	}
	return NULL;
}

void Loader::sinit(Class_base* c)
{
	assert(c->constructor==NULL);
	c->constructor=new Function(_constructor);
	c->super=Class<DisplayObjectContainer>::getClass();
	c->max_level=c->super->max_level+1;
}

void Loader::buildTraits(ASObject* o)
{
	o->setGetterByQName("contentLoaderInfo","",new Function(_getContentLoaderInfo));
	o->setVariableByQName("loadBytes","",new Function(loadBytes));
//	obj->setVariableByQName("load","",new Function(load));
}

void Loader::execute()
{
	LOG(LOG_NOT_IMPLEMENTED,"Loader async execution " << url);
	if(source==URL)
	{
		abort();
		/*local_root=new RootMovieClip;
		zlib_file_filter zf;
		zf.open(url.raw_buf(),ios_base::in);
		istream s(&zf);

		ParseThread local_pt(sys,local_root,s);
		local_pt.wait();*/
	}
	else if(source==BYTES)
	{
		//Implement loadBytes, now just dump
		assert(bytes->bytes);

		//We only support swf files now
		assert(memcmp(bytes->bytes,"CWS",3)==0);

		//The loaderInfo of the content is out contentLoaderInfo
		contentLoaderInfo->obj->incRef();
		local_root=new RootMovieClip(contentLoaderInfo);
		zlib_bytes_filter zf(bytes->bytes,bytes->len);
		istream s(&zf);

		ParseThread local_pt(sys,local_root,s);
		local_pt.wait();
		//HACK: advance to first frame, so that scripts get executed
		//We shold understand how to deliver frame events to movieclips not in the display list
		local_root->advanceFrame();
		content=local_root;
	}
	loaded=true;
	//Add a complete event for this object
	sys->currentVm->addEvent(contentLoaderInfo,Class<Event>::getInstanceS(true,"complete"));
}

void Loader::Render()
{
	if(!loaded)
		return;

	local_root->Render();
}

Sprite::Sprite():graphics(NULL)
{
}

void Sprite::sinit(Class_base* c)
{
	assert(c->constructor==NULL);
	c->constructor=new Function(_constructor);
	c->super=Class<DisplayObjectContainer>::getClass();
	c->max_level=c->super->max_level+1;
}

void Sprite::buildTraits(ASObject* o)
{
	o->setGetterByQName("graphics","",new Function(_getGraphics));
}

void Sprite::getBounds(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax)
{
	//Iterate over the displaylist of the current frame
	sem_wait(&sem_displayList);
	if(dynamicDisplayList.empty() && graphics==NULL)
	{
		sem_post(&sem_displayList);
		xmin=0;
		xmax=0;
		ymin=0;
		ymax=0;
		return;
	}

	//TODO: Check bounds calculation
	list<IDisplayListElem*>::iterator it=dynamicDisplayList.begin();
	for(;it!=dynamicDisplayList.end();it++)
	{
		number_t txmin,txmax,tymin,tymax;
		(*it)->getBounds(txmin,txmax,tymin,tymax);
		xmin = min(xmin,txmin);
		xmax = max(xmax,txmax);
		ymin = min(ymin,txmin);
		ymax = max(ymax,tymax);
	}
	sem_post(&sem_displayList);
	if(graphics)
	{
		number_t txmin,txmax,tymin,tymax;
		graphics->getBounds(txmin,txmax,tymin,tymax);
		xmin = min(xmin,txmin);
		xmax = max(xmax,txmax);
		ymin = min(ymin,txmin);
		ymax = max(ymax,tymax);
	}
	//TODO: take rotation into account
	Matrix.multiply2D(xmin,ymin,xmin,ymin);
	Matrix.multiply2D(xmax,ymax,xmax,ymax);
}

void Sprite::Render()
{
	assert(obj && obj->prototype);

	rt->glAcquireFramebuffer();

	float matrix[16];
	Matrix.get4DMatrix(matrix);
	glPushMatrix();
	glMultMatrixf(matrix);

	//Draw the dynamically added graphics, if any
	if(graphics)
		graphics->Render();


/*	FILLSTYLE::fixedColor(0,0,0);
	glBegin(GL_QUADS);
		glVertex2i(0,0);
		glVertex2i(75,0);
		glVertex2i(75,75);
		glVertex2i(0,75);
	glEnd();*/

	rt->glBlitFramebuffer();
	glPopMatrix();

	glPushMatrix();
	glMultMatrixf(matrix);

	sem_wait(&sem_displayList);
	//Now draw also the display list
	list<IDisplayListElem*>::iterator it=dynamicDisplayList.begin();
	for(;it!=dynamicDisplayList.end();it++)
		(*it)->Render();
	sem_post(&sem_displayList);

	glPopMatrix();
}

ASFUNCTIONBODY(Sprite,_constructor)
{
	//Sprite* th=static_cast<Sprite*>(obj->implementation);
	DisplayObjectContainer::_constructor(obj,NULL,0);

	return NULL;
}

ASFUNCTIONBODY(Sprite,_getGraphics)
{
	Sprite* th=static_cast<Sprite*>(obj->implementation);
	//Probably graphics is not used often, so create it here
	if(th->graphics==NULL)
		th->graphics=Class<Graphics>::getInstanceS(true);

	th->graphics->obj->incRef();
	return th->graphics->obj;
}

void MovieClip::sinit(Class_base* c)
{
	assert(c->constructor==NULL);
	c->constructor=new Function(_constructor);
	c->super=Class<Sprite>::getClass();
	c->max_level=c->super->max_level+1;
}

void MovieClip::buildTraits(ASObject* o)
{
	o->setGetterByQName("currentFrame","",new Function(_getCurrentFrame));
	o->setGetterByQName("totalFrames","",new Function(_getTotalFrames));
	o->setGetterByQName("framesLoaded","",new Function(_getFramesLoaded));
	o->setVariableByQName("stop","",new Function(stop));
	o->setVariableByQName("nextFrame","",new Function(nextFrame));
}

MovieClip::MovieClip():framesLoaded(1),totalFrames(1),cur_frame(NULL)
{
	//It's ok to initialize here framesLoaded=1, as it is valid and empty
	//RooMovieClip() will reset it, as stuff loaded dynamically needs frames to be committed
	frames.push_back(Frame());
	cur_frame=&frames.back();
}

void MovieClip::addToFrame(DisplayListTag* t)
{
	cur_frame->blueprint.push_back(t);
}

ASFUNCTIONBODY(MovieClip,addFrameScript)
{
	abort();
/*	MovieClip* th=static_cast<MovieClip*>(obj->implementation);
	if(args->size()%2)
	{
		LOG(LOG_ERROR,"Invalid arguments to addFrameScript");
		abort();
	}
	for(int i=0;i<args->size();i+=2)
	{
		int f=args->at(i+0)->toInt();
		IFunction* g=args->at(i+1)->toFunction();

		//Should wait for frames to be received
		if(f>=framesLoaded)
		{
			LOG(LOG_ERROR,"Invalid frame number passed to addFrameScript");
			abort();
		}

		th->frames[f].setScript(g);
	}*/
	return NULL;
}

ASFUNCTIONBODY(MovieClip,createEmptyMovieClip)
{
	LOG(LOG_NOT_IMPLEMENTED,"createEmptyMovieClip");
	return new Undefined;
/*	MovieClip* th=static_cast<MovieClip*>(obj);
	if(th==NULL)
		LOG(ERROR,"Not a valid MovieClip");

	LOG(CALLS,"Called createEmptyMovieClip: " << args->args[0]->toString() << " " << args->args[1]->toString());
	MovieClip* ret=new MovieClip();

	IDisplayListElem* t=new ASObjectWrapper(ret,args->args[1]->toInt());
	list<IDisplayListElem*>::iterator it=lower_bound(th->dynamicDisplayList.begin(),th->dynamicDisplayList.end(),t->getDepth(),list_orderer);
	th->dynamicDisplayList.insert(it,t);

	th->setVariableByName(args->args[0]->toString(),ret);
	return ret;*/
}

ASFUNCTIONBODY(MovieClip,swapDepths)
{
	LOG(LOG_NOT_IMPLEMENTED,"Called swapDepths");
	return NULL;
}

ASFUNCTIONBODY(MovieClip,stop)
{
	MovieClip* th=static_cast<MovieClip*>(obj->implementation);
	th->state.stop_FP=true;
	return NULL;
}

ASFUNCTIONBODY(MovieClip,nextFrame)
{
	MovieClip* th=static_cast<MovieClip*>(obj->implementation);
	assert(th->state.FP<th->state.max_FP);
	sys->currentVm->addEvent(NULL,new FrameChangeEvent(th->state.FP+1,th));
	return NULL;
}

ASFUNCTIONBODY(MovieClip,_getFramesLoaded)
{
	MovieClip* th=static_cast<MovieClip*>(obj->implementation);
	//currentFrame is 1-based
	return new Integer(th->framesLoaded);
}

ASFUNCTIONBODY(MovieClip,_getTotalFrames)
{
	MovieClip* th=static_cast<MovieClip*>(obj->implementation);
	//currentFrame is 1-based
	return new Integer(th->totalFrames);
}

ASFUNCTIONBODY(MovieClip,_getCurrentFrame)
{
	MovieClip* th=static_cast<MovieClip*>(obj->implementation);
	//currentFrame is 1-based
	return new Integer(th->state.FP+1);
}

ASFUNCTIONBODY(MovieClip,_constructor)
{
	//MovieClip* th=static_cast<MovieClip*>(obj->implementation);
	Sprite::_constructor(obj,NULL,0);
/*	th->setVariableByQName("swapDepths","",new Function(swapDepths));
	th->setVariableByQName("createEmptyMovieClip","",new Function(createEmptyMovieClip));
	th->setVariableByQName("addFrameScript","",new Function(addFrameScript));*/
	return NULL;
}

void MovieClip::advanceFrame()
{
	if(!state.stop_FP || state.explicit_FP /*&& (class_name=="MovieClip")*/)
	{
		//Before assigning the next_FP we initialize the frame
		frames[state.next_FP].init(this,displayList);
		state.FP=state.next_FP;
		if(!state.stop_FP)
			state.next_FP=min(state.FP+1,framesLoaded-1);
		state.explicit_FP=false;
	}

}

void MovieClip::Render()
{
	LOG(LOG_TRACE,"Render MovieClip");

	assert(state.FP<framesLoaded);

	advanceFrame();
	if(!state.stop_FP)
		frames[state.FP].runScript();

	//Set the id in the secondary color
	glPushAttrib(GL_CURRENT_BIT);
	glSecondaryColor3f(id,0,0);

	//Apply local transformation
	glPushMatrix();
	//glTranslatef(_x,_y,0);
	//glRotatef(rotation,0,0,1);
	frames[state.FP].Render();

	//Render objects added at runtime
	sem_wait(&sem_displayList);
	list<IDisplayListElem*>::iterator j=dynamicDisplayList.begin();
	for(;j!=dynamicDisplayList.end();j++)
		(*j)->Render();
	sem_post(&sem_displayList);

	glPopMatrix();
	glPopAttrib();

	LOG(LOG_TRACE,"End Render MovieClip");
}

void MovieClip::getBounds(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax)
{
	//Iterate over the displaylist of the current frame
	sem_wait(&sem_displayList);
	if(dynamicDisplayList.empty() && (framesLoaded==0 || frames[state.FP].displayList.size()==0))
	{
		xmin=0;
		xmax=0;
		ymin=0;
		ymax=0;
		sem_post(&sem_displayList);
		return;
	}
	//TODO: add dynamic dysplay list
	std::list<std::pair<PlaceInfo, IDisplayListElem*> >::iterator it=frames[state.FP].displayList.begin();
	assert(frames[state.FP].displayList.size()==1);
	sem_post(&sem_displayList);
	it->second->getBounds(xmin,xmax,ymin,ymax);
	//TODO: take rotation into account
	Matrix.multiply2D(xmin,ymin,xmin,ymin);
	Matrix.multiply2D(xmax,ymax,xmax,ymax);
}

DisplayObject::DisplayObject():width(0),height(0),loaderInfo(NULL)
{
}

void DisplayObject::sinit(Class_base* c)
{
	assert(c->constructor==NULL);
	c->constructor=new Function(_constructor);
	c->super=Class<EventDispatcher>::getClass();
	c->max_level=c->super->max_level+1;
}

void DisplayObject::buildTraits(ASObject* o)
{
	o->setGetterByQName("loaderInfo","",new Function(_getLoaderInfo));
	o->setGetterByQName("width","",new Function(_getWidth));
	o->setSetterByQName("width","",new Function(_setWidth));
	o->setGetterByQName("scaleX","",new Function(_getScaleX));
	o->setSetterByQName("scaleX","",new Function(undefinedFunction));
	o->setGetterByQName("scaleY","",new Function(_getScaleY));
	o->setSetterByQName("scaleY","",new Function(undefinedFunction));
	o->setGetterByQName("x","",new Function(_getX));
	o->setSetterByQName("x","",new Function(_setX));
	o->setGetterByQName("y","",new Function(_getY));
	o->setSetterByQName("y","",new Function(_setY));
	o->setGetterByQName("height","",new Function(_getHeight));
	o->setSetterByQName("height","",new Function(undefinedFunction));
	o->setGetterByQName("visible","",new Function(_getVisible));
	o->setSetterByQName("visible","",new Function(undefinedFunction));
	o->setGetterByQName("rotation","",new Function(_getRotation));
	o->setSetterByQName("rotation","",new Function(undefinedFunction));
	o->setGetterByQName("name","",new Function(_getName));
	o->setGetterByQName("parent","",new Function(_getParent));
	o->setGetterByQName("root","",new Function(_getRoot));
	o->setGetterByQName("blendMode","",new Function(_getBlendMode));
	o->setSetterByQName("blendMode","",new Function(undefinedFunction));
	o->setGetterByQName("scale9Grid","",new Function(_getScale9Grid));
	o->setSetterByQName("scale9Grid","",new Function(undefinedFunction));
	o->setGetterByQName("stage","",new Function(_getStage));
	o->setVariableByQName("getBounds","",new Function(_getBounds));
	o->setVariableByQName("localToGlobal","",new Function(localToGlobal));
	o->setSetterByQName("name","",new Function(_setName));
	o->setGetterByQName("mask","",new Function(_getMask));
	o->setSetterByQName("mask","",new Function(undefinedFunction));
	o->setGetterByQName("alpha","",new Function(_getAlpha));
	o->setSetterByQName("alpha","",new Function(undefinedFunction));
	o->setGetterByQName("cacheAsBitmap","",new Function(undefinedFunction));
	o->setSetterByQName("cacheAsBitmap","",new Function(undefinedFunction));
	o->setGetterByQName("opaqueBackground","",new Function(undefinedFunction));
	o->setSetterByQName("opaqueBackground","",new Function(undefinedFunction));

	//This is from InteractiveObject
	o->setSetterByQName("mouseEnabled","",new Function(undefinedFunction));
}

ASFUNCTIONBODY(DisplayObject,_getAlpha)
{
	//DisplayObject* th=static_cast<DisplayObject*>(obj->implementation);
	return abstract_d(1);
}

ASFUNCTIONBODY(DisplayObject,_getMask)
{
	//DisplayObject* th=static_cast<DisplayObject*>(obj->implementation);
	return new Null;
}

ASFUNCTIONBODY(DisplayObject,_getScaleX)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj->implementation);
	return abstract_d(th->Matrix.ScaleX);
}

ASFUNCTIONBODY(DisplayObject,_getScaleY)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj->implementation);
	return abstract_d(th->Matrix.ScaleY);
}

ASFUNCTIONBODY(DisplayObject,_getX)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj->implementation);
	return abstract_d(th->Matrix.TranslateX);
}

ASFUNCTIONBODY(DisplayObject,_setX)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj->implementation);
	assert(args && argslen==1);
	th->Matrix.TranslateX=args[0]->toInt();
	return NULL;
}

ASFUNCTIONBODY(DisplayObject,_getY)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj->implementation);
	return abstract_d(th->Matrix.TranslateY);
}

ASFUNCTIONBODY(DisplayObject,_setY)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj->implementation);
	assert(args && argslen==1);
	th->Matrix.TranslateY=args[0]->toInt();
	return NULL;
}

ASFUNCTIONBODY(DisplayObject,_getBounds)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj->implementation);
	assert(argslen==1);

	Rectangle* ret=Class<Rectangle>::getInstanceS(true);
	number_t x1,x2,y1,y2;
	th->getBounds(x1,x2,y1,y2);

	//Bounds are in the form [XY]{min,max}
	//convert it to rect (x,y,width,height) representation
	ret->x=x1;
	ret->width=x2-x1;
	ret->y=y1;
	ret->height=y2-y1;
	return ret->obj;
}

ASFUNCTIONBODY(DisplayObject,_constructor)
{
	//DisplayObject* th=static_cast<DisplayObject*>(obj->implementation);
	EventDispatcher::_constructor(obj,NULL,0);

	return NULL;
}

ASFUNCTIONBODY(DisplayObject,_getLoaderInfo)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj->implementation);
	if(th->loaderInfo)
	{
		th->loaderInfo->obj->incRef();
		return th->loaderInfo->obj;
	}
	else
		return new Undefined;
}

ASFUNCTIONBODY(DisplayObject,_getStage)
{
	assert(sys->stage);
	sys->stage->obj->incRef();
	return sys->stage->obj;
}

ASFUNCTIONBODY(DisplayObject,_getScale9Grid)
{
	//DisplayObject* th=static_cast<DisplayObject*>(obj->implementation);
	return new Undefined;
}

ASFUNCTIONBODY(DisplayObject,_getBlendMode)
{
	//DisplayObject* th=static_cast<DisplayObject*>(obj->implementation);
	return new Undefined;
}

ASFUNCTIONBODY(DisplayObject,localToGlobal)
{
	//DisplayObject* th=static_cast<DisplayObject*>(obj->implementation);
	return new Undefined;
}

ASFUNCTIONBODY(DisplayObject,_setRotation)
{
	//DisplayObject* th=static_cast<DisplayObject*>(obj->implementation);
	abort();
	//th->rotation=args->at(0)->toNumber();
	return NULL;
}

ASFUNCTIONBODY(DisplayObject,_setWidth)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj->implementation);
	th->width=args[0]->toInt();
	LOG(LOG_NOT_IMPLEMENTED,"Setting width not really supported on type " << ((obj->prototype)?(obj->prototype->class_name):""));
	return NULL;
}

ASFUNCTIONBODY(DisplayObject,_setName)
{
	//DisplayObject* th=static_cast<DisplayObject*>(obj->implementation);
	return NULL;
}

ASFUNCTIONBODY(DisplayObject,_getName)
{
	//DisplayObject* th=static_cast<DisplayObject*>(obj->implementation);
	return new Undefined;
}

ASFUNCTIONBODY(DisplayObject,_getParent)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj->implementation);
	if(th->parent==NULL)
		return new Undefined;

	th->parent->obj->incRef();
	return th->parent->obj;
}

ASFUNCTIONBODY(DisplayObject,_getRoot)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj->implementation);
	if(th->root)
	{
		th->root->obj->incRef();
		return th->root->obj;
	}
	else
		return new Undefined;
}

ASFUNCTIONBODY(DisplayObject,_getRotation)
{
	//DisplayObject* th=static_cast<DisplayObject*>(obj->implementation);
	return new Undefined;
}

ASFUNCTIONBODY(DisplayObject,_getVisible)
{
	//DisplayObject* th=static_cast<DisplayObject*>(obj->implementation);
	return abstract_b(true);
}

ASFUNCTIONBODY(DisplayObject,_getWidth)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj->implementation);
	//If the width has been explicity set, return that
	if(th->width!=0)
		return abstract_b(th->width);

	number_t x1,x2,y1,y2;
	th->getBounds(x1,x2,y1,y2);

	return abstract_i(x2-x1);
}

ASFUNCTIONBODY(DisplayObject,_getHeight)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj->implementation);
	//If the  height has been explicity set, return that
	if(th->height!=0)
		return abstract_b(th->height);

	number_t x1,x2,y1,y2;
	th->getBounds(x1,x2,y1,y2);

	return abstract_i(y2-y1);
}

void DisplayObjectContainer::sinit(Class_base* c)
{
	assert(c->constructor==NULL);
	c->constructor=new Function(_constructor);
	c->super=Class<DisplayObject>::getClass();
	c->max_level=c->super->max_level+1;
}

void DisplayObjectContainer::buildTraits(ASObject* o)
{
	o->setGetterByQName("numChildren","",new Function(_getNumChildren));
	o->setVariableByQName("getChildIndex","",new Function(getChildIndex));
	o->setVariableByQName("getChildAt","",new Function(getChildAt));
	o->setVariableByQName("addChild","",new Function(addChild));
	o->setVariableByQName("removeChild","",new Function(removeChild));
	o->setVariableByQName("addChildAt","",new Function(addChildAt));
	o->setVariableByQName("contains","",new Function(contains));
	o->setSetterByQName("mouseChildren","",new Function(undefinedFunction));
}

DisplayObjectContainer::DisplayObjectContainer()
{
	sem_init(&sem_displayList,0,1);
}

//This must be called fromt VM context
void DisplayObjectContainer::setRoot(RootMovieClip* r)
{
	if(r!=root)
	{
		DisplayObject::setRoot(r);
		list<IDisplayListElem*>::const_iterator it=dynamicDisplayList.begin();
		for(;it!=dynamicDisplayList.end();it++)
			(*it)->setRoot(r);
	}
}

ASFUNCTIONBODY(DisplayObjectContainer,_constructor)
{
	DisplayObject::_constructor(obj,NULL,0);
	return NULL;
}

ASFUNCTIONBODY(DisplayObjectContainer,_getNumChildren)
{
	DisplayObjectContainer* th=static_cast<DisplayObjectContainer*>(obj->implementation);
	return abstract_i(th->dynamicDisplayList.size());
}

void DisplayObjectContainer::_addChildAt(DisplayObject* child, unsigned int index)
{
	//If the child has no parent, set this container to parent
	//If there is a previous parent, purge the child from his list
	if(child->parent)
	{
		//Child already in this container
		if(child->parent==this)
			return;
		else
			child->parent->_removeChild(child);
	}
	child->parent=this;

	sem_wait(&sem_displayList);
	//Set the root of the movie to this container
	assert(child->root==NULL);
	child->setRoot(root);

	//We insert the object in the back of the list
	if(index==numeric_limits<unsigned int>::max())
		dynamicDisplayList.push_back(child);
	else
	{
		assert(index<=dynamicDisplayList.size());
		list<IDisplayListElem*>::iterator it=dynamicDisplayList.begin();
		for(unsigned int i=0;i<index;i++)
			it++;
		dynamicDisplayList.insert(it,child);
	}
	sem_post(&sem_displayList);
}

void DisplayObjectContainer::_removeChild(IDisplayListElem* child)
{
	assert(child->parent==this);
	assert(child->root==root);

	sem_wait(&sem_displayList);
	list<IDisplayListElem*>::iterator it=find(dynamicDisplayList.begin(),dynamicDisplayList.end(),child);
	assert(it!=dynamicDisplayList.end());
	dynamicDisplayList.erase(it);
	sem_post(&sem_displayList);
	child->parent=NULL;
}

bool DisplayObjectContainer::_contains(DisplayObject* d)
{
	if(d==this)
		return true;

	list<IDisplayListElem*>::const_iterator it=dynamicDisplayList.begin();
	for(;it!=dynamicDisplayList.end();it++)
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
	DisplayObjectContainer* th=static_cast<DisplayObjectContainer*>(obj->implementation);
	assert(argslen==1);
	//Validate object type
	assert(args[0]->prototype->isSubClass(Class<DisplayObject>::getClass()));

	//Cast to object
	DisplayObject* d=static_cast<DisplayObject*>(args[0]->implementation);

	bool ret=th->_contains(d);
	return abstract_i(ret);
}

//Only from VM context
ASFUNCTIONBODY(DisplayObjectContainer,addChildAt)
{
	DisplayObjectContainer* th=static_cast<DisplayObjectContainer*>(obj->implementation);
	assert(argslen==2);
	//Validate object type
	assert(args[0]->prototype->isSubClass(Class<DisplayObject>::getClass()));
	args[0]->incRef();

	int index=args[1]->toInt();

	//Cast to object
	DisplayObject* d=Class<DisplayObject>::cast(args[0]->implementation);
	th->_addChildAt(d,index);

	//Notify the object
	d->obj->incRef();
	sys->currentVm->addEvent(d,Class<Event>::getInstanceS(true,"added",d->obj));

	return d->obj;
}

ASFUNCTIONBODY(DisplayObjectContainer,addChild)
{
	DisplayObjectContainer* th=static_cast<DisplayObjectContainer*>(obj->implementation);
	assert(argslen==1);
	//Validate object type
	assert(args[0]->prototype->isSubClass(Class<DisplayObject>::getClass()));
	args[0]->incRef();

	//Cast to object
	DisplayObject* d=Class<DisplayObject>::cast(args[0]->implementation);
	th->_addChildAt(d,numeric_limits<unsigned int>::max());

	//Notify the object
	d->obj->incRef();
	sys->currentVm->addEvent(d,Class<Event>::getInstanceS(true,"added",d->obj));

	return d->obj;
}

//Only from VM context
ASFUNCTIONBODY(DisplayObjectContainer,removeChild)
{
	DisplayObjectContainer* th=static_cast<DisplayObjectContainer*>(obj->implementation);
	assert(argslen==1);
	//Validate object type
	assert(args[0]->prototype->isSubClass(Class<DisplayObject>::getClass()));
	//Cast to object
	DisplayObject* d=Class<DisplayObject>::cast(args[0]->implementation);

	th->_removeChild(d);

	//As we return the child we don't decRef it
	return d->obj;
}

//Only from VM context
ASFUNCTIONBODY(DisplayObjectContainer,getChildAt)
{
	DisplayObjectContainer* th=static_cast<DisplayObjectContainer*>(obj->implementation);
	assert(argslen==1);
	unsigned int index=args[0]->toInt();
	assert(index<th->dynamicDisplayList.size());
	list<IDisplayListElem*>::iterator it=th->dynamicDisplayList.begin();
	for(unsigned int i=0;i<index;i++)
		it++;

	(*it)->obj->incRef();
	return (*it)->obj;
}

//Only from VM context
ASFUNCTIONBODY(DisplayObjectContainer,getChildIndex)
{
	DisplayObjectContainer* th=static_cast<DisplayObjectContainer*>(obj->implementation);
	assert(argslen==1);
	//Validate object type
	assert(args[0]->prototype->isSubClass(Class<DisplayObject>::getClass()));

	//Cast to object
	DisplayObject* d=static_cast<DisplayObject*>(args[0]->implementation);

	list<IDisplayListElem*>::const_iterator it=th->dynamicDisplayList.begin();
	int ret=0;
	do
	{
		if(*it==d)
			break;
		
		ret++;
		it++;
		assert(it!=th->dynamicDisplayList.end());
	}
	while(1);
	return abstract_i(ret);
}

void Shape::sinit(Class_base* c)
{
	assert(c->constructor==NULL);
	c->constructor=new Function(_constructor);
	c->super=Class<DisplayObject>::getClass();
	c->max_level=c->super->max_level+1;
}

void Shape::buildTraits(ASObject* o)
{
	o->setGetterByQName("graphics","",new Function(_getGraphics));
}

void Shape::Render()
{
	//If graphics is not yet initialized we have nothing to do
	if(graphics==NULL)
		return;

	rt->glAcquireFramebuffer();

	float matrix[16];
	Matrix.get4DMatrix(matrix);
	glPushMatrix();
	glMultMatrixf(matrix);

	graphics->Render();

	rt->glBlitFramebuffer();
	glPopMatrix();
}

ASFUNCTIONBODY(Shape,_constructor)
{
	DisplayObject::_constructor(obj,NULL,0);
	return NULL;
}

ASFUNCTIONBODY(Shape,_getGraphics)
{
	Shape* th=static_cast<Shape*>(obj->implementation);
	//Probably graphics is not used often, so create it here
	if(th->graphics==NULL)
		th->graphics=Class<Graphics>::getInstanceS(true);

	th->graphics->obj->incRef();
	return th->graphics->obj;
}

void Stage::sinit(Class_base* c)
{
	assert(c->constructor==NULL);
	c->constructor=new Function(_constructor);
	c->super=Class<DisplayObjectContainer>::getClass();
	c->max_level=c->super->max_level+1;
}

void Stage::buildTraits(ASObject* o)
{
	o->setGetterByQName("stageWidth","",new Function(_getStageWidth));
	o->setGetterByQName("stageHeight","",new Function(_getStageHeight));
}

Stage::Stage()
{
}

ASFUNCTIONBODY(Stage,_constructor)
{
	return NULL;
}

ASFUNCTIONBODY(Stage,_getStageWidth)
{
	//Stage* th=static_cast<Stage*>(obj->implementation);
	RECT size=sys->getFrameSize();
	int width=size.Xmax/20;
	return abstract_d(width);
}

ASFUNCTIONBODY(Stage,_getStageHeight)
{
	//Stage* th=static_cast<Stage*>(obj->implementation);
	RECT size=sys->getFrameSize();
	int height=size.Ymax/20;
	return abstract_d(height);
}

void Graphics::sinit(Class_base* c)
{
	assert(c->constructor==NULL);
	c->constructor=new Function(_constructor);
}

void Graphics::buildTraits(ASObject* o)
{
	o->setVariableByQName("clear","",new Function(clear));
	o->setVariableByQName("drawRect","",new Function(drawRect));
	o->setVariableByQName("drawCircle","",new Function(drawCircle));
	o->setVariableByQName("moveTo","",new Function(moveTo));
	o->setVariableByQName("lineTo","",new Function(lineTo));
	o->setVariableByQName("beginFill","",new Function(beginFill));
	o->setVariableByQName("beginGradientFill","",new Function(beginGradientFill));
	o->setVariableByQName("endFill","",new Function(endFill));
}

void Graphics::getBounds(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax)
{
	if(geometry.size()==0)
	{
		xmin=0;
		xmax=0;
		ymin=0;
		ymax=0;
	}
	else
	{
		//Initialize values to the first available
		assert(geometry[0].outline.size()>0);
		xmin=xmax=geometry[0].outline[0].x;
		ymin=ymax=geometry[0].outline[0].y;

		for(unsigned int i=0;i<geometry.size();i++)
		{
			for(unsigned int j=0;j<geometry[i].outline.size();j++)
			{
				const Vector2& v=geometry[i].outline[j];
				if(v.x<xmin)
					xmin=v.x;
				else if(v.x>xmax)
					xmax=v.x;

				if(v.y<ymin)
					ymin=v.y;
				else if(v.y>ymax)
					ymax=v.y;
			}
		}
	}
}

ASFUNCTIONBODY(Graphics,_constructor)
{
	return NULL;
}

ASFUNCTIONBODY(Graphics,clear)
{
	Graphics* th=static_cast<Graphics*>(obj->implementation);
	sem_wait(&th->geometry_mutex);
	th->geometry.clear();
	sem_post(&th->geometry_mutex);
	th->tmpShape=GeomShape();
	th->styles.clear();
	return NULL;
}

ASFUNCTIONBODY(Graphics,moveTo)
{
	Graphics* th=static_cast<Graphics*>(obj->implementation);
	assert(argslen==2);

	//As we are moving, first of all flush the shape
	th->flushShape(true);

	th->curX=args[0]->toInt();
	th->curY=args[1]->toInt();
	return NULL;
}

ASFUNCTIONBODY(Graphics,lineTo)
{
	Graphics* th=static_cast<Graphics*>(obj->implementation);
	assert(argslen==2);

	int x=args[0]->toInt();
	int y=args[1]->toInt();

	//If this is the first line, add also the starting point
	if(th->tmpShape.outline.size()==0)
		th->tmpShape.outline.push_back(Vector2(th->curX,th->curY));

	th->tmpShape.outline.push_back(Vector2(x,y));

	return NULL;
}

void Graphics::flushShape(bool keepStyle)
{
	if(!tmpShape.outline.empty())
	{
		if(tmpShape.color)
		{
			assert(tmpShape.color==1);
			tmpShape.BuildFromEdges(&styles.back());
		}
		sem_wait(&geometry_mutex);
		int oldcolor=tmpShape.color;
		geometry.push_back(tmpShape);
		sem_post(&geometry_mutex);
		tmpShape=GeomShape();
		if(keepStyle)
			tmpShape.color=oldcolor;
	}
}

ASFUNCTIONBODY(Graphics,drawCircle)
{
	Graphics* th=static_cast<Graphics*>(obj->implementation);
	assert(argslen==3);

	double x=args[0]->toNumber();
	double y=args[1]->toNumber();
	double radius=args[2]->toNumber();

	th->flushShape(true);

	//Well, right now let's build a square anyway
	th->tmpShape.outline.push_back(Vector2(x-radius,y-radius));
	th->tmpShape.outline.push_back(Vector2(x+radius,y-radius));
	th->tmpShape.outline.push_back(Vector2(x+radius,y+radius));
	th->tmpShape.outline.push_back(Vector2(x-radius,y+radius));
	th->tmpShape.outline.push_back(Vector2(x-radius,y-radius));

	th->flushShape(true);

	return NULL;
}

ASFUNCTIONBODY(Graphics,drawRect)
{
	Graphics* th=static_cast<Graphics*>(obj->implementation);
	assert(argslen==4);

	int x=args[0]->toInt();
	int y=args[1]->toInt();
	int width=args[2]->toInt();
	int height=args[3]->toInt();

	th->flushShape(true);

	if(width==0)
	{
		th->tmpShape.outline.push_back(Vector2(x,y));
		th->tmpShape.outline.push_back(Vector2(x,y+height));
	}
	else if(height==0)
	{
		th->tmpShape.outline.push_back(Vector2(x,y));
		th->tmpShape.outline.push_back(Vector2(x+width,y));
	}
	else
	{
		//Build a shape and add it to the geometry vector
		th->tmpShape.outline.push_back(Vector2(x,y));
		th->tmpShape.outline.push_back(Vector2(x+width,y));
		th->tmpShape.outline.push_back(Vector2(x+width,y+height));
		th->tmpShape.outline.push_back(Vector2(x,y+height));
		th->tmpShape.outline.push_back(Vector2(x,y));
	}

	th->flushShape(true);

	return NULL;
}

ASFUNCTIONBODY(Graphics,beginGradientFill)
{
	Graphics* th=static_cast<Graphics*>(obj->implementation);
	th->styles.push_back(FILLSTYLE());
	th->styles.back().FillStyleType=0x00;
	uint32_t color=0;
	uint8_t alpha=255;
	if(argslen>=2) //Colors
	{
		assert(args[1]->getObjectType()==T_ARRAY);
		Array* ar=Class<Array>::cast(args[1]->implementation);
		assert(ar->size()>=1);
		color=ar->at(0)->toUInt();
		//cout << "Color " << hex << args->at(0)->toInt() << dec << endl;
	}
	th->styles.back().Color=RGBA(color&0xff,(color>>8)&0xff,(color>>16)&0xff,alpha);
	th->tmpShape.color=1;
	return NULL;
}

ASFUNCTIONBODY(Graphics,beginFill)
{
	Graphics* th=static_cast<Graphics*>(obj->implementation);
	th->styles.push_back(FILLSTYLE());
	th->styles.back().FillStyleType=0x00;
	uint32_t color=0;
	uint8_t alpha=255;
	if(argslen>=1)
	{
		color=args[0]->toUInt();
		//cout << "Color " << hex << args->at(0)->toInt() << dec << endl;
	}
	if(argslen>=2)
	{
		alpha=(uint8_t(args[1]->toNumber()*0xff));
		//cout << "Alpha " << args->at(1)->toNumber() << endl;
	}
	th->styles.back().Color=RGBA((color>>16)&0xff,(color>>8)&0xff,color&0xff,alpha);
	th->tmpShape.color=1;
	return NULL;
}

ASFUNCTIONBODY(Graphics,endFill)
{
	Graphics* th=static_cast<Graphics*>(obj->implementation);
	assert(th->tmpShape.color);
	if(!th->tmpShape.outline.empty())
	{
		if(th->tmpShape.outline.front()!=th->tmpShape.outline.back())
			th->tmpShape.outline.push_back(th->tmpShape.outline.front());
	}
	th->flushShape(false);
	return NULL;
}

void Graphics::Render()
{
	//Should probably flush the shape
	sem_wait(&geometry_mutex);

	for(unsigned int i=0;i<geometry.size();i++)
		geometry[i].Render();

/*	if(geometry.size()==1)
	{
		FILLSTYLE::fixedColor(0,0,0);
		glBegin(GL_QUADS);
			glVertex2i(0,0);
			glVertex2i(75,0);
			glVertex2i(75,75);
			glVertex2i(0,75);
		glEnd();
	}*/
	sem_post(&geometry_mutex);
}

void LineScaleMode::sinit(Class_base* c)
{
	c->setVariableByQName("HORIZONTAL","",Class<ASString>::getInstanceS(true,"horizontal")->obj);
	c->setVariableByQName("NONE","",Class<ASString>::getInstanceS(true,"none")->obj);
	c->setVariableByQName("NORMAL","",Class<ASString>::getInstanceS(true,"normal")->obj);
	c->setVariableByQName("VERTICAL","",Class<ASString>::getInstanceS(true,"vertical")->obj);
}

void StageScaleMode::sinit(Class_base* c)
{
	c->setVariableByQName("EXACT_FIT","",Class<ASString>::getInstanceS(true,"exactFit")->obj);
	c->setVariableByQName("NO_BORDER","",Class<ASString>::getInstanceS(true,"noBorder")->obj);
	c->setVariableByQName("NO_SCALE","",Class<ASString>::getInstanceS(true,"noScale")->obj);
	c->setVariableByQName("SHOW_ALL","",Class<ASString>::getInstanceS(true,"showAll")->obj);
}

void StageAlign::sinit(Class_base* c)
{
	c->setVariableByQName("TOP_LEFT","",Class<ASString>::getInstanceS(true,"TL")->obj);
}

void Bitmap::sinit(Class_base* c)
{
	assert(c->constructor==NULL);
//	c->constructor=new Function(_constructor);
	c->super=Class<DisplayObject>::getClass();
	c->max_level=c->super->max_level+1;
}
