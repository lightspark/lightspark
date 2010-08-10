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

#include <GL/glew.h>
#include <fstream>
#include <limits>
#include <cmath>
using namespace std;
using namespace lightspark;

extern TLSDATA SystemState* sys;
extern TLSDATA RenderThread* rt;

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
REGISTER_CLASS_NAME(Bitmap);

void LoaderInfo::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->super=Class<EventDispatcher>::getClass();
	c->max_level=c->super->max_level+1;
	c->setGetterByQName("loaderURL","",Class<IFunction>::getFunction(_getLoaderUrl));
	c->setGetterByQName("url","",Class<IFunction>::getFunction(_getUrl));
	c->setGetterByQName("bytesLoaded","",Class<IFunction>::getFunction(_getBytesLoaded));
	c->setGetterByQName("bytesTotal","",Class<IFunction>::getFunction(_getBytesTotal));
	c->setGetterByQName("applicationDomain","",Class<IFunction>::getFunction(_getApplicationDomain));
	c->setGetterByQName("sharedEvents","",Class<IFunction>::getFunction(_getSharedEvents));
}

void LoaderInfo::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(LoaderInfo,_constructor)
{
	LoaderInfo* th=static_cast<LoaderInfo*>(obj);
	EventDispatcher::_constructor(obj,NULL,0);
	th->sharedEvents=Class<EventDispatcher>::getInstanceS();
	return NULL;
}

ASFUNCTIONBODY(LoaderInfo,_getLoaderUrl)
{
	LoaderInfo* th=static_cast<LoaderInfo*>(obj);
	return Class<ASString>::getInstanceS(th->loaderURL);
}

ASFUNCTIONBODY(LoaderInfo,_getSharedEvents)
{
	LoaderInfo* th=static_cast<LoaderInfo*>(obj);
	th->sharedEvents->incRef();
	return th->sharedEvents;
}

ASFUNCTIONBODY(LoaderInfo,_getUrl)
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
	th->contentLoaderInfo=Class<LoaderInfo>::getInstanceS();
	return NULL;
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
/*	if(th->loading)
		return NULL;
	th->loading=true;*/
	throw UnsupportedException("Loader::load");
/*	if(args->at(0)->getClassName()!="URLRequest")
	{
		LOG(ERROR,"ArgumentError");
		abort();
	}*/
	URLRequest* r=static_cast<URLRequest*>(args[0]);
	th->url=r->url;
	th->source=URL;
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
		th->loading=true;
		th->source=BYTES;
		th->incRef();
		sys->addJob(th);
	}
	return NULL;
}

void Loader::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->super=Class<DisplayObjectContainer>::getClass();
	c->max_level=c->super->max_level+1;
	c->setGetterByQName("contentLoaderInfo","",Class<IFunction>::getFunction(_getContentLoaderInfo));
	c->setVariableByQName("loadBytes","",Class<IFunction>::getFunction(loadBytes));
//	c->setVariableByQName("load","",Class<IFunction>::getFunction(load));
}

void Loader::buildTraits(ASObject* o)
{
}

void Loader::execute()
{
	LOG(LOG_NOT_IMPLEMENTED,"Loader async execution " << url);
	if(source==URL)
	{
		threadAbort();
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
		assert_and_throw(bytes->bytes);

		//We only support swf files now
		assert_and_throw(memcmp(bytes->bytes,"CWS",3)==0);

		//The loaderInfo of the content is out contentLoaderInfo
		contentLoaderInfo->incRef();
		local_root=new RootMovieClip(contentLoaderInfo);
		zlib_bytes_filter zf(bytes->bytes,bytes->len);
		istream s(&zf);

		ParseThread* local_pt = new ParseThread(local_root,s);
		local_pt->run();
		content=local_root;
	}
	loaded=true;
	//Add a complete event for this object
	sys->currentVm->addEvent(contentLoaderInfo,Class<Event>::getInstanceS("complete"));
}

void Loader::threadAbort()
{
	//TODO: implement
	throw UnsupportedException("Loader::threadAbort");
}

void Loader::Render()
{
	if(!loaded)
		return;

	if(alpha==0.0)
		return;
	if(!visible)
		return;

	MatrixApplier ma(getMatrix());
	local_root->Render();
	ma.unapply();
}

bool Loader::getBounds(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const
{
	if(content)
	{
		if(content->getBounds(xmin,xmax,ymin,ymax))
		{
			getMatrix().multiply2D(xmin,ymin,xmin,ymin);
			getMatrix().multiply2D(xmax,ymax,xmax,ymax);
			return true;
		}
		else
			return false;
	}
	else
		return false;
}

Sprite::Sprite():graphics(NULL)
{
}

void Sprite::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->super=Class<DisplayObjectContainer>::getClass();
	c->max_level=c->super->max_level+1;
	c->setGetterByQName("graphics","",Class<IFunction>::getFunction(_getGraphics));
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
		for(;it!=dynamicDisplayList.end();it++)
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
				}
				ret=true;
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
				ymin = imin(ymin,txmin);
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

void Sprite::Render()
{
	number_t t1,t2,t3,t4;
	bool notEmpty=boundsRect(t1,t2,t3,t4);
	if(!notEmpty)
		return;

	if(alpha==0.0)
		return;
	if(!visible)
		return;

	MatrixApplier ma(getMatrix());

	//Draw the dynamically added graphics, if any
	if(graphics)
	{
		//Should clean only the bounds of the graphics
		if(!isSimple())
			rt->glAcquireTempBuffer(t1,t2,t3,t4);
		graphics->Render();
		if(!isSimple())
			rt->glBlitTempBuffer(t1,t2,t3,t4);
	}
	
	{
		Locker l(mutexDisplayList);
		//Now draw also the display list
		list<DisplayObject*>::iterator it=dynamicDisplayList.begin();
		for(;it!=dynamicDisplayList.end();it++)
			(*it)->Render();
	}
	ma.unapply();
}

void Sprite::inputRender()
{
	if(alpha==0.0)
		return;
	if(!visible)
		return;
	InteractiveObject::RenderProloue();

	MatrixApplier ma(getMatrix());

	if(graphics)
		graphics->Render();

	{
		Locker l(mutexDisplayList);
		//Now draw also the display list
		list<DisplayObject*>::iterator it=dynamicDisplayList.begin();
		for(;it!=dynamicDisplayList.end();it++)
			(*it)->inputRender();
	}

	ma.unapply();
	
	InteractiveObject::RenderEpilogue();
}

ASFUNCTIONBODY(Sprite,_constructor)
{
	//Sprite* th=static_cast<Sprite*>(obj->implementation);
	DisplayObjectContainer::_constructor(obj,NULL,0);

	return NULL;
}

ASFUNCTIONBODY(Sprite,_getGraphics)
{
	Sprite* th=static_cast<Sprite*>(obj);
	//Probably graphics is not used often, so create it here
	if(th->graphics==NULL)
		th->graphics=Class<Graphics>::getInstanceS();

	th->graphics->incRef();
	return th->graphics;
}

void MovieClip::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->super=Class<Sprite>::getClass();
	c->max_level=c->super->max_level+1;
	c->setGetterByQName("currentFrame","",Class<IFunction>::getFunction(_getCurrentFrame));
	c->setGetterByQName("totalFrames","",Class<IFunction>::getFunction(_getTotalFrames));
	c->setGetterByQName("framesLoaded","",Class<IFunction>::getFunction(_getFramesLoaded));
	c->setVariableByQName("stop","",Class<IFunction>::getFunction(stop));
	c->setVariableByQName("gotoAndStop","",Class<IFunction>::getFunction(gotoAndStop));
	c->setVariableByQName("nextFrame","",Class<IFunction>::getFunction(nextFrame));
}

void MovieClip::buildTraits(ASObject* o)
{
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
	throw UnsupportedException("MovieClip::addFrameScript");
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

	DisplayObject* t=new ASObjectWrapper(ret,args->args[1]->toInt());
	list<DisplayObject*>::iterator it=lower_bound(th->dynamicDisplayList.begin(),th->dynamicDisplayList.end(),t->getDepth(),list_orderer);
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
	//MovieClip* th=static_cast<MovieClip*>(obj->implementation);
	Sprite::_constructor(obj,NULL,0);
/*	th->setVariableByQName("swapDepths","",Class<IFunction>::getFunction(swapDepths));
	th->setVariableByQName("createEmptyMovieClip","",Class<IFunction>::getFunction(createEmptyMovieClip));
	th->setVariableByQName("addFrameScript","",Class<IFunction>::getFunction(addFrameScript));*/
	return NULL;
}

void MovieClip::advanceFrame()
{
	if((!state.stop_FP || state.explicit_FP) && totalFrames!=0 && getPrototype()->isSubClass(Class<MovieClip>::getClass()))
	{
		//If we have not yet loaded enough frames delay advancement
		if(state.next_FP>=framesLoaded)
		{
			LOG(LOG_NOT_IMPLEMENTED,"Not enough frames loaded");
			return;
		}
		//Before assigning the next_FP we initialize the frame
		//Should initialize all the frames from the current to the next
		for(uint32_t i=(state.FP+1);i<=state.next_FP;i++)
			frames[i].init(this,displayList);
		state.FP=state.next_FP;
		if(!state.stop_FP && framesLoaded>0)
			state.next_FP=imin(state.FP+1,framesLoaded-1);
		state.explicit_FP=false;
	}

}

void MovieClip::setRoot(RootMovieClip* r)
{
	if(r==root)
		return;
	if(root)
		root->unregisterChildClip(this);
	DisplayObjectContainer::setRoot(r);
	if(root)
		root->registerChildClip(this);
}

void MovieClip::bootstrap()
{
	if(totalFrames==0)
		return;
	assert_and_throw(framesLoaded>0);
	assert_and_throw(frames.size()>=1);
	frames[0].init(this,displayList);
}

void MovieClip::Render()
{
	if(alpha==0.0)
		return;
	if(!visible)
		return;
	assert_and_throw(graphics==NULL);

	MatrixApplier ma(getMatrix());
	//Save current frame, this may change during rendering
	uint32_t curFP=state.FP;

	if(framesLoaded)
	{
		assert_and_throw(curFP<framesLoaded);

		if(!state.stop_FP)
			frames[curFP].runScript();

		frames[curFP].Render();
	}

	{
		//Render objects added at runtime
		Locker l(mutexDisplayList);
		list<DisplayObject*>::iterator j=dynamicDisplayList.begin();
		for(;j!=dynamicDisplayList.end();j++)
			(*j)->Render();
	}

	ma.unapply();
}

void MovieClip::inputRender()
{
	if(alpha==0.0)
		return;
	if(!visible)
		return;
	InteractiveObject::RenderProloue();

	assert_and_throw(graphics==NULL);

	MatrixApplier ma(getMatrix());
	//Save current frame, this may change during rendering
	uint32_t curFP=state.FP;

	if(framesLoaded)
	{
		assert_and_throw(curFP<framesLoaded);

		if(!state.stop_FP)
			frames[curFP].runScript();

		frames[curFP].inputRender();
	}

	{
		//Render objects added at runtime
		Locker l(mutexDisplayList);
		list<DisplayObject*>::iterator j=dynamicDisplayList.begin();
		for(;j!=dynamicDisplayList.end();j++)
			(*j)->inputRender();
	}

	ma.unapply();
	InteractiveObject::RenderEpilogue();
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
	
			for(;it!=frames[curFP].displayList.end();it++)
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
			for(;j!=dynamicDisplayList.end();j++)
				(*j)->Render();*/
			assert_and_throw(dynamicDisplayList.empty());
		}

		ma.unapply();
	}
	
	return ret;
}

bool MovieClip::getBounds(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const
{
	bool valid=false;
	{
		Locker l(mutexDisplayList);
		
		list<DisplayObject*>::const_iterator dynit=dynamicDisplayList.begin();
		for(;dynit!=dynamicDisplayList.end();dynit++)
		{
			number_t t1,t2,t3,t4;
			if((*dynit)->getBounds(t1,t2,t3,t4))
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
	}
	
	if(framesLoaded==0) //We end here
		return valid;

	//Iterate over the displaylist of the current frame
	uint32_t curFP=state.FP;
	std::list<std::pair<PlaceInfo, DisplayObject*> >::const_iterator it=frames[curFP].displayList.begin();
	
	//Update bounds for all the elements
	for(;it!=frames[curFP].displayList.end();it++)
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
	
	if(valid)
	{
		//TODO: take rotation into account
		getMatrix().multiply2D(xmin,ymin,xmin,ymin);
		getMatrix().multiply2D(xmax,ymax,xmax,ymax);
		return true;
	}
	return false;
}

DisplayObject::DisplayObject():useMatrix(true),tx(0),ty(0),rotation(0),sx(1),sy(1),onStage(false),root(NULL),loaderInfo(NULL),
	alpha(1.0),visible(true),parent(NULL)
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
	c->setGetterByQName("loaderInfo","",Class<IFunction>::getFunction(_getLoaderInfo));
	c->setGetterByQName("width","",Class<IFunction>::getFunction(_getWidth));
	c->setSetterByQName("width","",Class<IFunction>::getFunction(_setWidth));
	c->setGetterByQName("scaleX","",Class<IFunction>::getFunction(_getScaleX));
	c->setSetterByQName("scaleX","",Class<IFunction>::getFunction(_setScaleX));
	c->setGetterByQName("scaleY","",Class<IFunction>::getFunction(_getScaleY));
	c->setSetterByQName("scaleY","",Class<IFunction>::getFunction(_setScaleY));
	c->setGetterByQName("x","",Class<IFunction>::getFunction(_getX));
	c->setSetterByQName("x","",Class<IFunction>::getFunction(_setX));
	c->setGetterByQName("y","",Class<IFunction>::getFunction(_getY));
	c->setSetterByQName("y","",Class<IFunction>::getFunction(_setY));
	c->setGetterByQName("height","",Class<IFunction>::getFunction(_getHeight));
	c->setSetterByQName("height","",Class<IFunction>::getFunction(_setHeight));
	c->setGetterByQName("visible","",Class<IFunction>::getFunction(_getVisible));
	c->setSetterByQName("visible","",Class<IFunction>::getFunction(_setVisible));
	c->setGetterByQName("rotation","",Class<IFunction>::getFunction(_getRotation));
	c->setSetterByQName("rotation","",Class<IFunction>::getFunction(_setRotation));
	c->setGetterByQName("name","",Class<IFunction>::getFunction(_getName));
	c->setGetterByQName("parent","",Class<IFunction>::getFunction(_getParent));
	c->setGetterByQName("root","",Class<IFunction>::getFunction(_getRoot));
	c->setGetterByQName("blendMode","",Class<IFunction>::getFunction(_getBlendMode));
	c->setSetterByQName("blendMode","",Class<IFunction>::getFunction(undefinedFunction));
	c->setGetterByQName("scale9Grid","",Class<IFunction>::getFunction(_getScale9Grid));
	c->setSetterByQName("scale9Grid","",Class<IFunction>::getFunction(undefinedFunction));
	c->setGetterByQName("stage","",Class<IFunction>::getFunction(_getStage));
	c->setVariableByQName("getBounds","",Class<IFunction>::getFunction(_getBounds));
	c->setVariableByQName("localToGlobal","",Class<IFunction>::getFunction(localToGlobal));
	c->setSetterByQName("name","",Class<IFunction>::getFunction(_setName));
	c->setGetterByQName("mask","",Class<IFunction>::getFunction(_getMask));
	c->setSetterByQName("mask","",Class<IFunction>::getFunction(undefinedFunction));
	c->setGetterByQName("alpha","",Class<IFunction>::getFunction(_getAlpha));
	c->setSetterByQName("alpha","",Class<IFunction>::getFunction(_setAlpha));
	c->setGetterByQName("cacheAsBitmap","",Class<IFunction>::getFunction(undefinedFunction));
	c->setSetterByQName("cacheAsBitmap","",Class<IFunction>::getFunction(undefinedFunction));
	c->setGetterByQName("opaqueBackground","",Class<IFunction>::getFunction(undefinedFunction));
	c->setSetterByQName("opaqueBackground","",Class<IFunction>::getFunction(undefinedFunction));
}

void DisplayObject::buildTraits(ASObject* o)
{
}

void DisplayObject::setMatrix(const lightspark::MATRIX& m)
{
	Matrix=m;
}

MATRIX DisplayObject::getMatrix() const
{
	MATRIX ret;
	if(useMatrix)
		ret=Matrix;
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

void DisplayObject::setRoot(RootMovieClip* r)
{
	if(root!=r)
	{
		assert_and_throw(root==NULL);
		root=r;
	}
}

void DisplayObject::setOnStage(bool staged)
{
	if(staged!=onStage)
	{
		//Our stage condition changed, send event
		onStage=staged;
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
	//DisplayObject* th=static_cast<DisplayObject*>(obj->implementation);
	return new Null;
}

ASFUNCTIONBODY(DisplayObject,_getScaleX)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	if(th->useMatrix)
		return abstract_d(th->Matrix.ScaleX);
	else
		return abstract_d(th->sx);
}

ASFUNCTIONBODY(DisplayObject,_setScaleX)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	assert_and_throw(argslen==1);
	number_t val=args[0]->toNumber();
	if(th->useMatrix)
	{
		th->valFromMatrix();
		th->useMatrix=false;
	}
	th->sx=val;
	return NULL;
}

ASFUNCTIONBODY(DisplayObject,_getScaleY)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	if(th->useMatrix)
		return abstract_d(th->Matrix.ScaleY);
	else
		return abstract_d(th->sy);
}

ASFUNCTIONBODY(DisplayObject,_setScaleY)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	assert_and_throw(argslen==1);
	number_t val=args[0]->toNumber();
	if(th->useMatrix)
	{
		th->valFromMatrix();
		th->useMatrix=false;
	}
	th->sy=val;
	return NULL;
}

ASFUNCTIONBODY(DisplayObject,_getX)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	if(th->useMatrix)
		return abstract_d(th->Matrix.TranslateX);
	else
		return abstract_d(th->tx);
}

ASFUNCTIONBODY(DisplayObject,_setX)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	assert_and_throw(argslen==1);
	number_t val=args[0]->toNumber();
	if(th->useMatrix)
	{
		th->valFromMatrix();
		th->useMatrix=false;
	}
	th->tx=val;
	return NULL;
}

ASFUNCTIONBODY(DisplayObject,_getY)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	if(th->useMatrix)
		return abstract_d(th->Matrix.TranslateY);
	else
		return abstract_d(th->ty);
}

ASFUNCTIONBODY(DisplayObject,_setY)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	assert_and_throw(argslen==1);
	number_t val=args[0]->toNumber();
	if(th->useMatrix)
	{
		th->valFromMatrix();
		th->useMatrix=false;
	}
	th->ty=val;
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
	assert(sys->stage);
	sys->stage->incRef();
	return sys->stage;
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
	//DisplayObject* th=static_cast<DisplayObject*>(obj);
	return new Undefined;
}

ASFUNCTIONBODY(DisplayObject,_setRotation)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	assert_and_throw(argslen==1);
	number_t val=args[0]->toNumber();
	if(th->useMatrix)
	{
		th->valFromMatrix();
		th->useMatrix=false;
	}
	th->rotation=val;
	return NULL;
}

ASFUNCTIONBODY(DisplayObject,_setName)
{
	//DisplayObject* th=static_cast<DisplayObject*>(obj);
	return NULL;
}

ASFUNCTIONBODY(DisplayObject,_getName)
{
	//DisplayObject* th=static_cast<DisplayObject*>(obj);
	return new Undefined;
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
	if(th->useMatrix)
	{
		th->valFromMatrix();
		th->useMatrix=false;
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
		if(th->useMatrix)
		{
			th->valFromMatrix();
			th->useMatrix=false;
		}
		th->sx=newscale;
	}
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
		if(th->useMatrix)
		{
			th->valFromMatrix();
			th->useMatrix=false;
		}
		th->sy=newscale;
	}
	return NULL;
}

void DisplayObjectContainer::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->super=Class<InteractiveObject>::getClass();
	c->max_level=c->super->max_level+1;
	c->setGetterByQName("numChildren","",Class<IFunction>::getFunction(_getNumChildren));
	c->setVariableByQName("getChildIndex","",Class<IFunction>::getFunction(getChildIndex));
	c->setVariableByQName("getChildAt","",Class<IFunction>::getFunction(getChildAt));
	c->setVariableByQName("addChild","",Class<IFunction>::getFunction(addChild));
	c->setVariableByQName("removeChild","",Class<IFunction>::getFunction(removeChild));
	c->setVariableByQName("removeChildAt","",Class<IFunction>::getFunction(removeChildAt));
	c->setVariableByQName("addChildAt","",Class<IFunction>::getFunction(addChildAt));
	c->setVariableByQName("contains","",Class<IFunction>::getFunction(contains));
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
		for(;it!=dynamicDisplayList.end();it++)
			(*it)->decRef();
	}
}

InteractiveObject::InteractiveObject():id(0)
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
	assert_and_throw(th->id==0);
	//Object registered very early are not supported this way (Stage for example)
	if(sys && sys->getInputThread())
		sys->getInputThread()->addListener(th);
	
	return NULL;
}

void InteractiveObject::buildTraits(ASObject* o)
{
}

void InteractiveObject::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->super=Class<DisplayObject>::getClass();
	c->max_level=c->super->max_level+1;
	//c->setSetterByQName("mouseEnabled","",Class<IFunction>::getFunction(undefinedFunction));
	//c->setGetterByQName("mouseEnabled","",Class<IFunction>::getFunction(undefinedFunction));
}

void InteractiveObject::RenderProloue()
{
	rt->pushId();
	rt->currentId=id;
	FILLSTYLE::fixedColor(id,id,id);
}

void InteractiveObject::RenderEpilogue()
{
	rt->popId();
}

void DisplayObjectContainer::dumpDisplayList()
{
	cout << "Size: " << dynamicDisplayList.size() << endl;
	list<DisplayObject*>::const_iterator it=dynamicDisplayList.begin();
	for(;it!=dynamicDisplayList.end();it++)
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
		for(;it!=dynamicDisplayList.end();it++)
			(*it)->setRoot(r);
	}
}

void DisplayObjectContainer::setOnStage(bool staged)
{
	if(staged!=onStage)
	{
		DisplayObject::setOnStage(staged);
		//Notify childern
		list<DisplayObject*>::const_iterator it=dynamicDisplayList.begin();
		for(;it!=dynamicDisplayList.end();it++)
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
				it++;
			dynamicDisplayList.insert(it,child);
			//We acquire a reference to the child
			child->incRef();
		}
	}
	child->setOnStage(onStage);
}

void DisplayObjectContainer::_removeChild(DisplayObject* child)
{
	if(child->parent==NULL)
		return; //Should throw an ArgumentError
	assert_and_throw(child->parent==this);
	assert_and_throw(child->getRoot()==root);

	Locker l(mutexDisplayList);
	{
		list<DisplayObject*>::iterator it=find(dynamicDisplayList.begin(),dynamicDisplayList.end(),child);
		assert_and_throw(it!=dynamicDisplayList.end());
		dynamicDisplayList.erase(it);
	}
	//We can release the reference to the child
	child->parent=NULL;
	child->setOnStage(false);
	child->decRef();
}

bool DisplayObjectContainer::_contains(DisplayObject* d)
{
	if(d==this)
		return true;

	list<DisplayObject*>::const_iterator it=dynamicDisplayList.begin();
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
	DisplayObjectContainer* th=static_cast<DisplayObjectContainer*>(obj);
	assert_and_throw(argslen==1);
	//Validate object type
	assert_and_throw(args[0]->getPrototype()->isSubClass(Class<DisplayObject>::getClass()));

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
	//Validate object type
	assert_and_throw(args[0] && args[0]->getPrototype() && 
		args[0]->getPrototype()->isSubClass(Class<DisplayObject>::getClass()));
	args[0]->incRef();

	int index=args[1]->toInt();

	//Cast to object
	DisplayObject* d=Class<DisplayObject>::cast(args[0]);
	th->_addChildAt(d,index);

	//Notify the object
	d->incRef();
	sys->currentVm->addEvent(d,Class<Event>::getInstanceS("added"));

	return d;
}

ASFUNCTIONBODY(DisplayObjectContainer,addChild)
{
	DisplayObjectContainer* th=static_cast<DisplayObjectContainer*>(obj);
	assert_and_throw(argslen==1);
	//Validate object type
	assert_and_throw(args[0] && args[0]->getPrototype() && 
		args[0]->getPrototype()->isSubClass(Class<DisplayObject>::getClass()));
	args[0]->incRef();

	//Cast to object
	DisplayObject* d=Class<DisplayObject>::cast(args[0]);
	th->_addChildAt(d,numeric_limits<unsigned int>::max());

	//Notify the object
	d->incRef();
	sys->currentVm->addEvent(d,Class<Event>::getInstanceS("added"));

	return d;
}

//Only from VM context
ASFUNCTIONBODY(DisplayObjectContainer,removeChild)
{
	DisplayObjectContainer* th=static_cast<DisplayObjectContainer*>(obj);
	assert_and_throw(argslen==1);
	//Validate object type
	assert_and_throw(args[0]->getPrototype()->isSubClass(Class<DisplayObject>::getClass()));
	//Cast to object
	DisplayObject* d=Class<DisplayObject>::cast(args[0]);

	th->_removeChild(d);

	//As we return the child we don't decRef it again
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
			return NULL;
		list<DisplayObject*>::iterator it=th->dynamicDisplayList.begin();
		for(int32_t i=0;i<index;i++)
			it++;
		child=*it;
		th->dynamicDisplayList.erase(it);
	}
	//We can release the reference to the child
	child->parent=NULL;
	child->setOnStage(false);

	//As we return the child we don't decRef it
	return child;
}

//Only from VM context
ASFUNCTIONBODY(DisplayObjectContainer,getChildAt)
{
	DisplayObjectContainer* th=static_cast<DisplayObjectContainer*>(obj);
	assert_and_throw(argslen==1);
	unsigned int index=args[0]->toInt();
	assert_and_throw(index<th->dynamicDisplayList.size());
	list<DisplayObject*>::iterator it=th->dynamicDisplayList.begin();
	for(unsigned int i=0;i<index;i++)
		it++;

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
		it++;
		assert_and_throw(it!=th->dynamicDisplayList.end());
	}
	while(1);
	return abstract_i(ret);
}

void Shape::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->super=Class<DisplayObject>::getClass();
	c->max_level=c->super->max_level+1;
	c->setGetterByQName("graphics","",Class<IFunction>::getFunction(_getGraphics));
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

void Shape::Render()
{
	//If graphics is not yet initialized we have nothing to do
	if(graphics==NULL)
		return;

	if(alpha==0.0)
		return;
	if(!visible)
		return;

	number_t t1,t2,t3,t4;
	bool ret=graphics->getBounds(t1,t2,t3,t4);

	if(!ret)
		return;

	MatrixApplier ma(getMatrix());

	if(!isSimple())
		rt->glAcquireTempBuffer(t1,t2,t3,t4);

	graphics->Render();

	if(!isSimple())
		rt->glBlitTempBuffer(t1,t2,t3,t4);
	
	ma.unapply();
}

void Shape::inputRender()
{
	//If graphics is not yet initialized we have nothing to do
	if(graphics==NULL)
		return;

	if(alpha==0.0)
		return;
	if(!visible)
		return;

	MatrixApplier ma(getMatrix());

	graphics->Render();
	ma.unapply();
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
		th->graphics=Class<Graphics>::getInstanceS();

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
	c->setGetterByQName("stageWidth","",Class<IFunction>::getFunction(_getStageWidth));
	c->setGetterByQName("stageHeight","",Class<IFunction>::getFunction(_getStageHeight));
	c->setGetterByQName("scaleMode","",Class<IFunction>::getFunction(_getScaleMode));
	c->setSetterByQName("scaleMode","",Class<IFunction>::getFunction(_setScaleMode));
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

ASFUNCTIONBODY(Stage,_getStageWidth)
{
	//Stage* th=static_cast<Stage*>(obj);
	uint32_t width;
	if(sys->scaleMode==SystemState::NO_SCALE && sys->getRenderThread())
		width=sys->getRenderThread()->windowWidth;
	else
	{
		RECT size=sys->getFrameSize();
		width=size.Xmax/20;
	}
	return abstract_d(width);
}

ASFUNCTIONBODY(Stage,_getStageHeight)
{
	//Stage* th=static_cast<Stage*>(obj);
	uint32_t height;
	if(sys->scaleMode==SystemState::NO_SCALE && sys->getRenderThread())
		height=sys->getRenderThread()->windowHeight;
	else
	{
		RECT size=sys->getFrameSize();
		height=size.Ymax/20;
	}
	return abstract_d(height);
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
	c->setVariableByQName("clear","",Class<IFunction>::getFunction(clear));
	c->setVariableByQName("drawRect","",Class<IFunction>::getFunction(drawRect));
	c->setVariableByQName("drawCircle","",Class<IFunction>::getFunction(drawCircle));
	c->setVariableByQName("moveTo","",Class<IFunction>::getFunction(moveTo));
	c->setVariableByQName("lineTo","",Class<IFunction>::getFunction(lineTo));
	c->setVariableByQName("beginFill","",Class<IFunction>::getFunction(beginFill));
	c->setVariableByQName("beginGradientFill","",Class<IFunction>::getFunction(beginGradientFill));
	c->setVariableByQName("endFill","",Class<IFunction>::getFunction(endFill));
}

void Graphics::buildTraits(ASObject* o)
{
}

bool Graphics::getBounds(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const
{
	Locker locker2(geometryMutex);
	//If the geometry has been modified we have to generate it again
	if(!validGeometry)
	{
		validGeometry=true;
		Locker locker(builderMutex);
		geometry.clear();
		builder.outputShapes(geometry);
		for(unsigned int i=0;i<geometry.size();i++)
			geometry[i].BuildFromEdges(&styles);
	}
	if(geometry.size()==0)
		return false;

	//Initialize values to the first available
	bool initialized=false;
	for(unsigned int i=0;i<geometry.size();i++)
	{
		for(unsigned int j=0;j<geometry[i].outlines.size();j++)
		{
			for(unsigned int k=0;k<geometry[i].outlines[j].size();k++)
			{
				const Vector2& v=geometry[i].outlines[j][k];
				if(initialized)
				{
					xmin=imin(v.x,xmin);
					xmax=imax(v.x,xmax);
					ymin=imin(v.y,ymin);
					ymax=imax(v.y,ymax);
				}
				else
				{
					xmin=v.x;
					xmax=v.x;
					ymin=v.y;
					ymax=v.y;
					initialized=true;
				}
			}
		}
	}
	return initialized;
}

ASFUNCTIONBODY(Graphics,_constructor)
{
	return NULL;
}

ASFUNCTIONBODY(Graphics,clear)
{
	Graphics* th=static_cast<Graphics*>(obj);
	{
		Locker locker(th->builderMutex);
		th->builder.clear();
		th->validGeometry=false;
	}
	th->styles.clear();
	return NULL;
}

ASFUNCTIONBODY(Graphics,moveTo)
{
	Graphics* th=static_cast<Graphics*>(obj);
	assert_and_throw(argslen==2);

	th->curX=args[0]->toInt();
	th->curY=args[1]->toInt();
	return NULL;
}

ASFUNCTIONBODY(Graphics,lineTo)
{
	Graphics* th=static_cast<Graphics*>(obj);
	assert_and_throw(argslen==2);

	int x=args[0]->toInt();
	int y=args[1]->toInt();

	//TODO: support line styles to avoid this
	if(th->styles.size())
	{
		Locker locker(th->builderMutex);
		th->builder.extendOutlineForColor(th->styles.size(),Vector2(th->curX,th->curY),Vector2(x,y));
		th->validGeometry=false;
	}

	th->curX=x;
	th->curY=y;
	return NULL;
}

ASFUNCTIONBODY(Graphics,drawCircle)
{
	Graphics* th=static_cast<Graphics*>(obj);
	assert_and_throw(argslen==3);

	double x=args[0]->toNumber();
	double y=args[1]->toNumber();
	double radius=args[2]->toNumber();

	//Well, right now let's build a square anyway
	const Vector2 a(x-radius,y-radius);
	const Vector2 b(x+radius,y-radius);
	const Vector2 c(x+radius,y+radius);
	const Vector2 d(x-radius,y+radius);

	//TODO: support line styles to avoid this
	if(th->styles.size())
	{
		Locker locker(th->builderMutex);
		th->builder.extendOutlineForColor(th->styles.size(),a,b);
		th->builder.extendOutlineForColor(th->styles.size(),b,c);
		th->builder.extendOutlineForColor(th->styles.size(),c,d);
		th->builder.extendOutlineForColor(th->styles.size(),d,a);
		th->validGeometry=false;
	}
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

	//TODO: support line styles to avoid this
	if(th->styles.size())
	{
		Locker locker(th->builderMutex);
		th->builder.extendOutlineForColor(th->styles.size(),a,b);
		th->builder.extendOutlineForColor(th->styles.size(),b,c);
		th->builder.extendOutlineForColor(th->styles.size(),c,d);
		th->builder.extendOutlineForColor(th->styles.size(),d,a);
		th->validGeometry=false;
	}
	return NULL;
}

ASFUNCTIONBODY(Graphics,beginGradientFill)
{
	Graphics* th=static_cast<Graphics*>(obj);
	th->styles.push_back(FILLSTYLE());
	th->styles.back().FillStyleType=0x00;
	uint32_t color=0;
	uint8_t alpha=255;
	if(argslen>=2) //Colors
	{
		assert_and_throw(args[1]->getObjectType()==T_ARRAY);
		Array* ar=Class<Array>::cast(args[1]);
		assert_and_throw(ar->size()>=1);
		color=ar->at(0)->toUInt();
	}
	th->styles.back().Color=RGBA(color&0xff,(color>>8)&0xff,(color>>16)&0xff,alpha);
	return NULL;
}

ASFUNCTIONBODY(Graphics,beginFill)
{
	Graphics* th=static_cast<Graphics*>(obj);
	th->styles.push_back(FILLSTYLE());
	th->styles.back().FillStyleType=0x00;
	uint32_t color=0;
	uint8_t alpha=255;
	if(argslen>=1)
		color=args[0]->toUInt();
	if(argslen>=2)
		alpha=(uint8_t(args[1]->toNumber()*0xff));
	th->styles.back().Color=RGBA((color>>16)&0xff,(color>>8)&0xff,color&0xff,alpha);
	return NULL;
}

ASFUNCTIONBODY(Graphics,endFill)
{
//	Graphics* th=static_cast<Graphics*>(obj);
	//TODO: close the path if open
	return NULL;
}

void Graphics::Render()
{
	Locker locker2(geometryMutex);
	//If the geometry has been modified we have to generate it again
	if(!validGeometry)
	{
		validGeometry=true;
		Locker locker(builderMutex);
		cout << "Generating geometry" << endl;
		geometry.clear();
		builder.outputShapes(geometry);
		for(unsigned int i=0;i<geometry.size();i++)
			geometry[i].BuildFromEdges(&styles);
	}

	for(unsigned int i=0;i<geometry.size();i++)
		geometry[i].Render();
}

void LineScaleMode::sinit(Class_base* c)
{
	c->setVariableByQName("HORIZONTAL","",Class<ASString>::getInstanceS("horizontal"));
	c->setVariableByQName("NONE","",Class<ASString>::getInstanceS("none"));
	c->setVariableByQName("NORMAL","",Class<ASString>::getInstanceS("normal"));
	c->setVariableByQName("VERTICAL","",Class<ASString>::getInstanceS("vertical"));
}

void StageScaleMode::sinit(Class_base* c)
{
	c->setVariableByQName("EXACT_FIT","",Class<ASString>::getInstanceS("exactFit"));
	c->setVariableByQName("NO_BORDER","",Class<ASString>::getInstanceS("noBorder"));
	c->setVariableByQName("NO_SCALE","",Class<ASString>::getInstanceS("noScale"));
	c->setVariableByQName("SHOW_ALL","",Class<ASString>::getInstanceS("showAll"));
}

void StageAlign::sinit(Class_base* c)
{
	c->setVariableByQName("TOP_LEFT","",Class<ASString>::getInstanceS("TL"));
}

void Bitmap::sinit(Class_base* c)
{
//	c->constructor=Class<IFunction>::getFunction(_constructor);
	c->setConstructor(NULL);
	c->super=Class<DisplayObject>::getClass();
	c->max_level=c->super->max_level+1;
}
