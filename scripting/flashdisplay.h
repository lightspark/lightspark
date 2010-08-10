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

#ifndef _FLASH_DISPLAY_H
#define _FLASH_DISPLAY_H

#include "compat.h"
#include <FTGL/ftgl.h>

#include "swftypes.h"
#include "flashevents.h"
#include "flashutils.h"
#include "thread_pool.h"
#include "backends/geometry.h"

namespace lightspark
{

class RootMovieClip;
class DisplayListTag;
class LoaderInfo;
class DisplayObjectContainer;

class DisplayObject: public EventDispatcher
{
friend class DisplayObjectContainer;
private:
	MATRIX Matrix;
	bool useMatrix;
	number_t tx,ty;
	number_t rotation;
	number_t sx,sy;
	bool onStage;

protected:
	MATRIX getMatrix() const;
	void valFromMatrix();
	RootMovieClip* root;
	LoaderInfo* loaderInfo;
	int computeWidth();
	int computeHeight();
	bool isSimple() const;
	float alpha;
	bool visible;
public:
	int Depth;
	UI16 CharacterId;
	CXFORMWITHALPHA ColorTransform;
	UI16 Ratio;
	UI16 ClipDepth;
	CLIPACTIONS ClipActions;
	DisplayObjectContainer* parent;
	DisplayObject();
	~DisplayObject();
	virtual void Render()
	{
		throw RunTimeException("DisplayObject::Render");
	}
	virtual void inputRender()
	{
		Render();
	}
	virtual bool getBounds(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const
	{
		throw RunTimeException("DisplayObject::getBounds");
		return false;
	}
	virtual void setRoot(RootMovieClip* root);
	virtual void setOnStage(bool staged);
	RootMovieClip* getRoot() { return root; }
	virtual Vector2 debugRender(FTFont* font, bool deep)
	{
		::abort();
		return Vector2(0,0);
	}
	void setMatrix(const MATRIX& m);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
	ASFUNCTION(_getVisible);
	ASFUNCTION(_setVisible);
	ASFUNCTION(_getStage);
	ASFUNCTION(_getX);
	ASFUNCTION(_setX);
	ASFUNCTION(_getY);
	ASFUNCTION(_setY);
	ASFUNCTION(_getMask);
	ASFUNCTION(_setAlpha);
	ASFUNCTION(_getAlpha);
	ASFUNCTION(_getScaleX);
	ASFUNCTION(_setScaleX);
	ASFUNCTION(_getScaleY);
	ASFUNCTION(_setScaleY);
	ASFUNCTION(_getLoaderInfo);
	ASFUNCTION(_getBounds);
	ASFUNCTION(_getWidth);
	ASFUNCTION(_setWidth);
	ASFUNCTION(_getHeight);
	ASFUNCTION(_setHeight);
	ASFUNCTION(_getRotation);
	ASFUNCTION(_getName);
	ASFUNCTION(_getParent);
	ASFUNCTION(_getRoot);
	ASFUNCTION(_getBlendMode);
	ASFUNCTION(_getScale9Grid);
	ASFUNCTION(_setRotation);
	ASFUNCTION(_setName);
	ASFUNCTION(localToGlobal);
};

class InteractiveObject: public DisplayObject
{
protected:
	float id;
	void RenderProloue();
	void RenderEpilogue();
public:
	InteractiveObject();
	virtual ~InteractiveObject();
	ASFUNCTION(_constructor);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	void setId(float i){id=i;}
};

class DisplayObjectContainer: public InteractiveObject
{
private:
	void _addChildAt(DisplayObject* child, unsigned int index);
	bool _contains(DisplayObject* d);
protected:
	//This is shared between RenderThread and VM
	std::list < DisplayObject* > dynamicDisplayList;
	//The lock should only be taken when doing write operations
	//As the RenderThread only reads, it's safe to read without the lock
	mutable Mutex mutexDisplayList;
	void setRoot(RootMovieClip* r);
	void setOnStage(bool staged);
public:
	void dumpDisplayList();
	void _removeChild(DisplayObject*);
	DisplayObjectContainer();
	virtual ~DisplayObjectContainer();
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
	ASFUNCTION(_getNumChildren);
	ASFUNCTION(addChild);
	ASFUNCTION(removeChild);
	ASFUNCTION(removeChildAt);
	ASFUNCTION(addChildAt);
	ASFUNCTION(getChildIndex);
	ASFUNCTION(getChildAt);
	ASFUNCTION(contains);
};

class Graphics: public ASObject
{
private:
	//builder and geometry are used by RenderThread and ABCVm
	mutable Mutex builderMutex;
	mutable ShapesBuilder builder;
	mutable Mutex geometryMutex;
	mutable bool validGeometry;
	mutable std::vector<GeomShape> geometry;
	//We need a list to preserve pointers
	std::list<FILLSTYLE> styles; 
	int curX, curY;
public:
	Graphics():builderMutex("builderMutex"),geometryMutex("geometryMutex"),validGeometry(false),curX(0),curY(0)
	{
	}
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
	ASFUNCTION(beginFill);
	ASFUNCTION(beginGradientFill);
	ASFUNCTION(endFill);
	ASFUNCTION(drawRect);
	ASFUNCTION(drawCircle);
	ASFUNCTION(moveTo);
	ASFUNCTION(lineTo);
	ASFUNCTION(clear);
	void Render();
	bool getBounds(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const;
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
	bool getBounds(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const;
	void Render();
	void inputRender();
};

class MorphShape: public DisplayObject
{
public:
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
	//bool getBounds(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const;
	//void Render();
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
	RootMovieClip* local_root;
	bool loading;
	bool loaded;
	DisplayObject* content;
	SOURCE source;
	tiny_string url;
	ByteArray* bytes;
	LoaderInfo* contentLoaderInfo;
	void execute();
	void threadAbort();
public:
	Loader():local_root(NULL),loading(false),loaded(false),content(NULL)
	{
	}
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
	ASFUNCTION(load);
	ASFUNCTION(loadBytes);
	ASFUNCTION(_getContentLoaderInfo);
	int getDepth() const
	{
		return 0;
	}
	void Render();
	bool getBounds(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const;
};

class Sprite: public DisplayObjectContainer
{
friend class DisplayObject;
private:
	bool boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const;
protected:
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
	bool getBounds(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const;
	void Render();
	void inputRender();
};

class MovieClip: public Sprite
{
//SERIOUS_TODO: add synchronization
friend class ParseThread;
protected:
	uint32_t framesLoaded;
	uint32_t totalFrames;
	std::list<std::pair<PlaceInfo, DisplayObject*> > displayList;
	Frame* cur_frame;
	void bootstrap();
public:
	std::vector<Frame> frames;
	RunState state;
	MovieClip();
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
	ASFUNCTION(swapDepths);
	ASFUNCTION(createEmptyMovieClip);
	ASFUNCTION(addFrameScript);
	ASFUNCTION(stop);
	ASFUNCTION(gotoAndStop);
	ASFUNCTION(nextFrame);
	ASFUNCTION(_getCurrentFrame);
	ASFUNCTION(_getTotalFrames);
	ASFUNCTION(_getFramesLoaded);

	virtual void addToFrame(DisplayListTag* r);

	void advanceFrame();
	uint32_t getFrameIdByLabel(const tiny_string& l) const;

	//DisplayObject interface
	void Render();
	void inputRender();
	void setRoot(RootMovieClip* r);
	
	/*! \brief Should be run with the default fragment/vertex program on
	* * \param font An FT font used for debug messages
	* * \param deep Flag to enable propagation of the debugRender to children */
	Vector2 debugRender(FTFont* font, bool deep);
	bool getBounds(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const;
	void check()
	{
		assert_and_throw(frames.size()==framesLoaded);
	}
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
	ASFUNCTION(_getScaleMode);
	ASFUNCTION(_setScaleMode);
};

class StageScaleMode: public ASObject
{
public:
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o)
	{
	}
};

class StageAlign: public ASObject
{
public:
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o)
	{
	}
};

class LineScaleMode: public ASObject
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
