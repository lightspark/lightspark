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

#ifndef _FLASH_DISPLAY_H
#define _FLASH_DISPLAY_H

#include "swftypes.h"
#include "flashevents.h"
#include "flashutils.h"
#include "thread_pool.h"
#include "geometry.h"

namespace lightspark
{

class RootMovieClip;
class DisplayListTag;
class LoaderInfo;
class DisplayObjectContainer;

class IDisplayListElem: public EventDispatcher
{
public:
	int Depth;
	UI16 CharacterId;
	CXFORMWITHALPHA ColorTransform;
	UI16 Ratio;
	UI16 ClipDepth;
	CLIPACTIONS ClipActions;
	RootMovieClip* root;
	MATRIX Matrix;
	MATRIX* origMatrix;
	DisplayObjectContainer* parent;

	IDisplayListElem():root(NULL),origMatrix(NULL),parent(NULL){}
	virtual void Render()=0;
	virtual void getBounds(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax)=0;
};

class DisplayObject: public IDisplayListElem
{
friend class DisplayObjectContainer;
protected:
	intptr_t width;
	intptr_t height;
	number_t rotation;
	LoaderInfo* loaderInfo;
public:
	DisplayObject();
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
	ASFUNCTION(_getVisible);
	ASFUNCTION(_getStage);
	ASFUNCTION(_getX);
	ASFUNCTION(_setX);
	ASFUNCTION(_getY);
	ASFUNCTION(_setY);
	ASFUNCTION(_getMask);
	ASFUNCTION(_getAlpha);
	ASFUNCTION(_getScaleX);
	ASFUNCTION(_getScaleY);
	ASFUNCTION(_getLoaderInfo);
	ASFUNCTION(_getWidth);
	ASFUNCTION(_getBounds);
	ASFUNCTION(_setWidth);
	ASFUNCTION(_getHeight);
	ASFUNCTION(_getRotation);
	ASFUNCTION(_getName);
	ASFUNCTION(_getParent);
	ASFUNCTION(_getRoot);
	ASFUNCTION(_getBlendMode);
	ASFUNCTION(_getScale9Grid);
	ASFUNCTION(_setRotation);
	ASFUNCTION(_setName);
	ASFUNCTION(localToGlobal);
	void Render()
	{
		abort();
	}
	void getBounds(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax)
	{
		abort();
	}
};

class DisplayObjectContainer: public DisplayObject
{
private:
	void _addChildAt(DisplayObject* child, int index);
	bool _contains(DisplayObject* d);
protected:
	std::list < IDisplayListElem* > dynamicDisplayList;
public:
	void _removeChild(IDisplayListElem*);
	DisplayObjectContainer();
	void getBounds(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax)
	{
		abort();
	}
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
	ASFUNCTION(_getNumChildren);
	ASFUNCTION(addChild);
	ASFUNCTION(removeChild);
	ASFUNCTION(addChildAt);
	ASFUNCTION(getChildIndex);
	ASFUNCTION(getChildAt);
	ASFUNCTION(contains);
};

class Graphics: public IInterface
{
private:
	//As geometry is used by RenderThread but modified by ABCVm we have to mutex a bit
	sem_t geometry_mutex;
	std::vector<GeomShape> geometry;
	GeomShape tmpShape;
	int curX, curY;
	void flushShape();
public:
	Graphics():curX(0),curY(0)
	{
		sem_init(&geometry_mutex,0,1);
	}
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
	ASFUNCTION(beginFill);
	ASFUNCTION(drawRect);
	ASFUNCTION(moveTo);
	ASFUNCTION(lineTo);
	ASFUNCTION(clear);
	void Render();
	void getBounds(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax);
};

class Shape: public DisplayObject
{
private:
	Graphics* graphics;
public:
	Shape():graphics(NULL){}
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
	ASFUNCTION(_getGraphics);
	void getBounds(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax)
	{
		if(graphics)
			graphics->getBounds(xmin,xmax,ymin,ymax);
		else
		{
			xmin=0;
			xmax=0;
			ymin=0;
			ymax=0;
		}
	}
	void Render();
};

class LoaderInfo: public EventDispatcher
{
friend class SystemState;
private:
	uint32_t bytesLoaded;
	uint32_t bytesTotal;
	tiny_string url;
	tiny_string loaderURL;
	EventDispatcher* sharedEvents;
public:
	LoaderInfo():bytesLoaded(100),bytesTotal(100)
	{
	}
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
	ASFUNCTION(addEventListener);
	ASFUNCTION(_getLoaderUrl);
	ASFUNCTION(_getUrl);
	ASFUNCTION(_getBytesLoaded);
	ASFUNCTION(_getBytesTotal);
	ASFUNCTION(_getApplicationDomain);
	ASFUNCTION(_getSharedEvents);
};

class Loader: public IThreadJob, public DisplayObjectContainer
{
private:
	enum SOURCE { URL, BYTES };
	SOURCE source;
	tiny_string url;
	ByteArray* bytes;
	bool loading;
	bool loaded;
	RootMovieClip* local_root;
	LoaderInfo* contentLoaderInfo;
	DisplayObject* content;
public:
	Loader():loading(false),local_root(NULL),loaded(false),content(NULL)
	{
	}
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
	ASFUNCTION(load);
	ASFUNCTION(loadBytes);
	ASFUNCTION(_getContentLoaderInfo);
	void execute();
	int getDepth() const
	{
		return 0;
	}
	void Render();
};

class Sprite: public DisplayObjectContainer
{
friend class DisplayObject;
private:
	Graphics* graphics;
public:
	Sprite();
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
	ASFUNCTION(_getGraphics);
	int getDepth() const
	{
		return 0;
	}
	void getBounds(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax);
	void Render();
};

class MovieClip: public Sprite
{
//SERIOUS_TODO: add syncrhonization
friend class ParseThread;
protected:
	uint32_t framesLoaded;
	uint32_t totalFrames;
	std::list<std::pair<PlaceInfo, IDisplayListElem*> > displayList;
	Frame cur_frame;
public:
	std::vector<Frame> frames;
	RunState state;
public:
	MovieClip();
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
	ASFUNCTION(swapDepths);
	ASFUNCTION(createEmptyMovieClip);
	ASFUNCTION(addFrameScript);
	ASFUNCTION(stop);
	ASFUNCTION(nextFrame);
	ASFUNCTION(_getCurrentFrame);
	ASFUNCTION(_getTotalFrames);
	ASFUNCTION(_getFramesLoaded);

	virtual void addToFrame(DisplayListTag* r);

	void advanceFrame();

	//IDisplayListElem interface
	void Render();
	void getBounds(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax);
};

class Stage: public DisplayObjectContainer
{
private:
	//Taken directly from SystemState
	//uintptr_t width;
	//uintptr_t height;
public:
	Stage();
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
	ASFUNCTION(_getStageWidth);
	ASFUNCTION(_getStageHeight);
};

class StageScaleMode: public IInterface
{
public:
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o)
	{
	}
};

class StageAlign: public IInterface
{
public:
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o)
	{
	}
};

class LineScaleMode: public IInterface
{
public:
	static void sinit(Class_base* c);
};

class Bitmap: public DisplayObject
{
public:
	static void sinit(Class_base* c);
};

};

#endif
