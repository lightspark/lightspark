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
#include "backends/graphics.h"

namespace lightspark
{

class RootMovieClip;
class DisplayListTag;
class LoaderInfo;
class DisplayObjectContainer;
class InteractiveObject;

class DisplayObject: public EventDispatcher
{
//friend class DisplayObjectContainer;
friend class GraphicsContainer;
private:
	MATRIX Matrix;
	ACQUIRE_RELEASE_FLAG(useMatrix);
	number_t tx,ty;
	number_t rotation;
	number_t sx,sy;
	/**
	  	The object we are masking, if any
	*/
	DisplayObject* maskOf;
	void localToGlobal(number_t xin, number_t yin, number_t& xout, number_t& yout) const;
	void becomeMaskOf(DisplayObject* m);
	void setMask(DisplayObject* m);
protected:
	/**
	  	The object that masks us, if any
	*/
	DisplayObject* mask;
	mutable Spinlock spinlock;
	void computeDeviceBoundsForRect(number_t xmin, number_t xmax, number_t ymin, number_t ymax,
			uint32_t& outXMin, uint32_t& outYMin, uint32_t& outWidth, uint32_t& outHeight) const;
	void valFromMatrix();
	bool onStage;
	RootMovieClip* root;
	LoaderInfo* loaderInfo;
	int computeWidth();
	int computeHeight();
	bool isSimple() const;
	bool skipRender(bool maskEnabled) const;
	float alpha;
	bool visible;
	//Data to handle async rendering
	Shepherd shepherd;
	CachedSurface cachedSurface;
	void defaultRender(bool maskEnabled) const;
	DisplayObject(const DisplayObject& d);
	void renderPrologue() const;
	void renderEpilogue() const;
	void hitTestPrologue() const;
	void hitTestEpilogue() const;
public:
	int Depth;
	tiny_string name;
	UI16 CharacterId;
	CXFORMWITHALPHA ColorTransform;
	UI16 Ratio;
	UI16 ClipDepth;
	CLIPACTIONS ClipActions;
	DisplayObjectContainer* parent;
	DisplayObject();
	~DisplayObject();
	MATRIX getMatrix() const;
	virtual void invalidate();
	MATRIX getConcatenatedMatrix() const;
	virtual const std::vector<GeomToken>& getTokens()
	{
		throw RunTimeException("DisplayObject::getTokens");
	}
	virtual float getScaleFactor() const
	{
		throw RunTimeException("DisplayObject::getScaleFactor");
	}
	virtual void Render(bool maskEnabled)
	{
		throw RunTimeException("DisplayObject::Render");
	}
	virtual bool getBounds(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const
	{
		throw RunTimeException("DisplayObject::getBounds");
	}
	virtual InteractiveObject* hitTest(InteractiveObject* last, number_t x, number_t y)
	{
		throw RunTimeException("DisplayObject::hitTest");
	}
	//API to handle mask support in hit testing
	virtual bool isOpaque(number_t x, number_t y) const
	{
		throw RunTimeException("DisplayObject::isOpaque");
	}
	virtual void setRoot(RootMovieClip* root);
	virtual void setOnStage(bool staged);
	RootMovieClip* getRoot() { return root; }
	virtual Vector2 debugRender(FTFont* font, bool deep)
	{
		throw RunTimeException("DisplayObject::debugRender");
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
	ASFUNCTION(_setMask);
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
	ASFUNCTION(_setName);
	ASFUNCTION(_getParent);
	ASFUNCTION(_getRoot);
	ASFUNCTION(_getBlendMode);
	ASFUNCTION(_getScale9Grid);
	ASFUNCTION(_setRotation);
	ASFUNCTION(localToGlobal);
};

class InteractiveObject: public DisplayObject
{
protected:
	bool mouseEnabled;
public:
	InteractiveObject();
	virtual ~InteractiveObject();
	ASFUNCTION(_constructor);
	ASFUNCTION(_setMouseEnabled);
	ASFUNCTION(_getMouseEnabled);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
};

class DisplayObjectContainer: public InteractiveObject
{
private:
	void _addChildAt(DisplayObject* child, unsigned int index);
	bool _contains(DisplayObject* d);
protected:
	void invalidate();
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
	ASFUNCTION(getChildByName);
	ASFUNCTION(contains);
};

class GraphicsContainer;

class Graphics: public ASObject
{
friend class GraphicsContainer;
private:
	std::vector<GeomToken> tokens;
	//We need a list to preserve pointers
	std::list<FILLSTYLE> styles; 
	int curX, curY;
	GraphicsContainer *const owner;
	//TODO: Add spinlock
public:
	Graphics():owner(NULL)
	{
		throw RunTimeException("Cannot instantiate a Graphics object");
	}
	Graphics(GraphicsContainer* _o):curX(0),curY(0),owner(_o)
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
	bool getBounds(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const;
	std::vector<GeomToken> getGraphicsTokens() const;
	bool hitTest(number_t x, number_t y) const;
};

class GraphicsContainer
{
friend class Graphics;
protected:
	Graphics* graphics;
	DisplayObject* owner;
	GraphicsContainer(DisplayObject* _o):graphics(NULL),owner(_o){}
	void invalidateGraphics();
};

class Shape: public DisplayObject, public GraphicsContainer
{
protected:
	std::vector<GeomToken> cachedTokens;
	void renderImpl(bool maskEnabled, number_t t1, number_t t2, number_t t3, number_t t4) const;
public:
	Shape():GraphicsContainer(this){}
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
	ASFUNCTION(_getGraphics);
	bool getBounds(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const;
	void Render(bool maskEnabled);
	InteractiveObject* hitTest(InteractiveObject* last, number_t x, number_t y);
	bool isOpaque(number_t x, number_t y) const;
	void invalidate();
	const std::vector<GeomToken>& getTokens();
	float getScaleFactor() const;
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
friend class RootMovieClip;
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
	ASFUNCTION(_getLoaderURL);
	ASFUNCTION(_getURL);
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
	void jobFence();
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
	void Render(bool maskEnabled);
	bool getBounds(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const;
};

class Sprite: public DisplayObjectContainer, public GraphicsContainer
{
friend class DisplayObject;
private:
	bool boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const;
protected:
	void renderImpl(bool maskEnabled, number_t t1,number_t t2,number_t t3,number_t t4) const;
	InteractiveObject* hitTestImpl(number_t x, number_t y);
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
	void Render(bool maskEnabled);
	InteractiveObject* hitTest(InteractiveObject* last, number_t x, number_t y);
	void invalidate();
};

class MovieClip: public Sprite
{
private:
	uint32_t totalFrames;
	bool boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const;
protected:
	uint32_t framesLoaded;
	std::list<std::pair<PlaceInfo, DisplayObject*> > displayList;
	Frame* cur_frame;
	void bootstrap();
	std::vector<IFunction*> frameScripts;
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
	void setTotalFrames(uint32_t t);

	//DisplayObject interface
	void Render(bool maskEnabled);
	InteractiveObject* hitTest(InteractiveObject* last, number_t x, number_t y);
	void invalidate();
	void setRoot(RootMovieClip* r);
	void setOnStage(bool staged);
	
	/*! \brief Should be run with the default fragment/vertex program on
	* * \param font An FT font used for debug messages
	* * \param deep Flag to enable propagation of the debugRender to children */
	Vector2 debugRender(FTFont* font, bool deep);
	bool getBounds(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const;
	void check() const
	{
		assert_and_throw(frames.size()==framesLoaded);
	}
};

class Stage: public DisplayObjectContainer
{
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
	bool getBounds(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const;
};

};

#endif
