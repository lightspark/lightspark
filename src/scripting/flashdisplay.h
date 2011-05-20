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

class Stage;
class RootMovieClip;
class DisplayListTag;
class LoaderInfo;
class DisplayObjectContainer;
class InteractiveObject;
class Downloader;

class DisplayObject: public EventDispatcher
{
friend class TokenContainer;
private:
	MATRIX Matrix;
	ACQUIRE_RELEASE_FLAG(useMatrix);
	number_t tx,ty;
	number_t rotation;
	number_t sx,sy;
	/**
	  	The object we are masking, if any
	*/
	_NR<DisplayObject> maskOf;
	void localToGlobal(number_t xin, number_t yin, number_t& xout, number_t& yout) const;
	void becomeMaskOf(_NR<DisplayObject> m);
	void setMask(_NR<DisplayObject> m);
	_NR<DisplayObjectContainer> parent;
protected:
	/**
	  	The object that masks us, if any
	*/
	_NR<DisplayObject> mask;
	mutable Spinlock spinlock;
	void computeDeviceBoundsForRect(number_t xmin, number_t xmax, number_t ymin, number_t ymax,
			uint32_t& outXMin, uint32_t& outYMin, uint32_t& outWidth, uint32_t& outHeight) const;
	void valFromMatrix();
	bool onStage;
	_NR<LoaderInfo> loaderInfo;
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
	tiny_string name;
	UI16_SWF CharacterId;
	CXFORMWITHALPHA ColorTransform;
	UI16_SWF Ratio;
	UI16_SWF ClipDepth;
	CLIPACTIONS ClipActions;
	_NR<DisplayObjectContainer> getParent() const { return parent; }
	virtual void setParent(_NR<DisplayObjectContainer> p);
	/*
	   Used to link DisplayObjects the invalidation queue
	*/
	_NR<DisplayObject> invalidateQueueNext;
	DisplayObject();
	void finalize();
	MATRIX getMatrix() const;
	virtual void invalidate();
	virtual void requestInvalidation();
	MATRIX getConcatenatedMatrix() const;
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
	virtual _NR<InteractiveObject> hitTest(_NR<InteractiveObject> last, number_t x, number_t y)
	{
		throw RunTimeException("DisplayObject::hitTest");
	}
	//API to handle mask support in hit testing
	virtual bool isOpaque(number_t x, number_t y) const
	{
		throw RunTimeException("DisplayObject::isOpaque");
	}
	virtual void setOnStage(bool staged);
	bool isOnStage() const { return onStage; }
	virtual _NR<RootMovieClip> getRoot();
	virtual _NR<Stage> getStage() const;
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
	ASFUNCTION(_getMouseX);
	ASFUNCTION(_getMouseY);
	ASFUNCTION(localToGlobal);
};

class InteractiveObject: public DisplayObject
{
protected:
	bool mouseEnabled;
public:
	InteractiveObject();
	~InteractiveObject();
	ASFUNCTION(_constructor);
	ASFUNCTION(_setMouseEnabled);
	ASFUNCTION(_getMouseEnabled);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
};

class SimpleButton: public InteractiveObject
{
private:
	DisplayObject *downState;
	DisplayObject *hitTestState;
	DisplayObject *overState;
	DisplayObject *upState;
	bool enabled;
	bool useHandCursor;
public:
	SimpleButton(){}
	void Render(bool maskEnabled);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	bool getBounds(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const;
	_NR<InteractiveObject> hitTest(_NR<InteractiveObject> last, number_t x, number_t y);
	ASFUNCTION(_constructor);
	ASFUNCTION(_getUpState);
	ASFUNCTION(_setUpState);
	ASFUNCTION(_getDownState);
	ASFUNCTION(_setDownState);
	ASFUNCTION(_getOverState);
	ASFUNCTION(_setOverState);
	ASFUNCTION(_getHitTestState);
	ASFUNCTION(_setHitTestState);
	ASFUNCTION(_getEnabled);
	ASFUNCTION(_setEnabled);
	ASFUNCTION(_getUseHandCursor);
	ASFUNCTION(_setUseHandCursor);
};

class DisplayObjectContainer: public InteractiveObject
{
private:
	std::map<uint32_t,DisplayObject*> depthToLegacyChild;
	bool _contains(_R<DisplayObject> child);
protected:
	void requestInvalidation();
	//This is shared between RenderThread and VM
	std::list < _R<DisplayObject> > dynamicDisplayList;
	//The lock should only be taken when doing write operations
	//As the RenderThread only reads, it's safe to read without the lock
	mutable Mutex mutexDisplayList;
	void setOnStage(bool staged);
public:
	void _addChildAt(_R<DisplayObject> child, unsigned int index);
	void dumpDisplayList();
	bool _removeChild(_R<DisplayObject> child);
	DisplayObjectContainer();
	void finalize();
	bool hasLegacyChildAt(uint32_t depth);
	void deleteLegacyChildAt(uint32_t depth);
	void insertLegacyChildAt(uint32_t depth, DisplayObject* obj);
	void transformLegacyChildAt(uint32_t depth, const MATRIX& mat);
	void purgeLegacyChildren();
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

class TokenContainer
{
	friend class Graphics;
private:
	DisplayObject* owner;
	/* multiply shapes' coordinates by this
	 * value to get pixel.
	 * DefineShapeTags set a scaling of 1/20,
	 * DefineTextTags set a scaling of 1/1024/20.
	 * If any drawing function is called and
	 * scaling is not 1.0f,
	 * the tokens are cleared and scaling is set
	 * to 1.0f.
	 */
	float scaling;
	std::vector<GeomToken> tokens;
protected:
	TokenContainer(DisplayObject* _o) : owner(_o), scaling(1.0f) {}
	TokenContainer(DisplayObject* _o, const std::vector<GeomToken>& _tokens, float _scaling)
		: owner(_o), scaling(_scaling), tokens(_tokens) {}

	void invalidate();
	void requestInvalidation();
	bool getBounds(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const;
	bool hitTest(number_t x, number_t y) const;
	void renderImpl(bool maskEnabled, number_t t1, number_t t2, number_t t3, number_t t4) const;
	void Render(bool maskEnabled);
	bool tokensEmpty() const { return tokens.empty(); }
public:
	static void FromShaperecordListToShapeVector(const std::vector<SHAPERECORD>& shapeRecords,
												 std::vector<GeomToken>& tokens, const std::list<FILLSTYLE>& fillStyles,
												 const Vector2& offset = Vector2(), int scaling = 1);
};

/* This objects paints to its owners tokens */
class Graphics: public ASObject
{
private:
	int curX, curY;
	TokenContainer *const owner;
	//TODO: Add spinlock
	void checkAndSetScaling()
	{
		if(owner->scaling != 1.0f)
		{
			owner->scaling = 1.0f;
			owner->tokens.clear();
			assert(curX == 0 && curY == 0);
		}
	}
public:
	Graphics():owner(NULL)
	{
		throw RunTimeException("Cannot instantiate a Graphics object");
	}
	Graphics(TokenContainer* _o)
		: curX(0),curY(0),owner(_o) {}
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
	ASFUNCTION(lineStyle);
	ASFUNCTION(beginFill);
	ASFUNCTION(beginGradientFill);
	ASFUNCTION(endFill);
	ASFUNCTION(drawRect);
	ASFUNCTION(drawRoundRect);
	ASFUNCTION(drawCircle);
	ASFUNCTION(moveTo);
	ASFUNCTION(lineTo);
	ASFUNCTION(curveTo);
	ASFUNCTION(cubicCurveTo);
	ASFUNCTION(clear);
};


class Shape: public DisplayObject, public TokenContainer
{
protected:
	_NR<Graphics> graphics;
public:
	Shape():TokenContainer(this), graphics(NULL) {}
	Shape(const std::vector<GeomToken>& tokens, float scaling)
		: TokenContainer(this, tokens, scaling), graphics(NULL) {}
	void finalize();
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
	ASFUNCTION(_getGraphics);
	bool getBounds(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const;
	void Render(bool maskEnabled) { TokenContainer::Render(maskEnabled); }
	_NR<InteractiveObject> hitTest(_NR<InteractiveObject> last, number_t x, number_t y);
	bool isOpaque(number_t x, number_t y) const;
	void requestInvalidation() { TokenContainer::requestInvalidation(); }
	void invalidate() { TokenContainer::invalidate(); }
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

class Loader;

class LoaderInfo: public EventDispatcher
{
friend class RootMovieClip;
private:
	uint32_t bytesLoaded;
	uint32_t bytesTotal;
	tiny_string url;
	tiny_string loaderURL;
	_NR<EventDispatcher> sharedEvents;
	_NR<Loader> loader;
	enum LOAD_STATUS { STARTED=0, INIT_SENT, COMPLETE };
	LOAD_STATUS loadStatus;
	Spinlock spinlock;
public:
	LoaderInfo():bytesLoaded(0),bytesTotal(0),sharedEvents(NullRef),loader(NullRef),loadStatus(STARTED) {}
	LoaderInfo(_R<Loader> l):bytesLoaded(0),bytesTotal(0),sharedEvents(NullRef),loader(l),loadStatus(STARTED) {}
	void finalize();
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
	ASFUNCTION(addEventListener);
	ASFUNCTION(_getLoaderURL);
	ASFUNCTION(_getURL);
	ASFUNCTION(_getBytesLoaded);
	ASFUNCTION(_getBytesTotal);
	ASFUNCTION(_getApplicationDomain);
	ASFUNCTION(_getLoader);
	ASFUNCTION(_getContent);
	ASFUNCTION(_getSharedEvents);
	void setBytesTotal(uint32_t b)
	{
		bytesTotal=b;
	}
	void setBytesLoaded(uint32_t b);
	void sendInit();
};

class Loader: public IThreadJob, public DisplayObjectContainer
{
private:
	enum SOURCE { URL, BYTES };
	mutable Spinlock localRootSpinlock;
	_NR<RootMovieClip> localRoot;
	bool loading;
	bool loaded;
	SOURCE source;
	tiny_string url;
	_NR<ByteArray> bytes;
	_NR<LoaderInfo> contentLoaderInfo;
	Spinlock downloaderLock;
	Downloader* downloader;
	void execute();
	void threadAbort();
	void jobFence();
public:
	Loader():localRoot(NullRef),loading(false),loaded(false),bytes(NullRef),contentLoaderInfo(NullRef),downloader(NULL)
	{
	}
	~Loader();
	void finalize();
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
	ASFUNCTION(load);
	ASFUNCTION(loadBytes);
	ASFUNCTION(_getContentLoaderInfo);
	ASFUNCTION(_getContent);
	int getDepth() const
	{
		return 0;
	}
	void Render(bool maskEnabled);
	bool getBounds(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const;
	void setOnStage(bool staged);
};

class Sprite: public DisplayObjectContainer, public TokenContainer
{
friend class DisplayObject;
private:
	_NR<Graphics> graphics;
protected:
	bool boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const;
	void renderImpl(bool maskEnabled, number_t t1,number_t t2,number_t t3,number_t t4) const;
	_NR<InteractiveObject> hitTestImpl(number_t x, number_t y);
public:
	Sprite();
	void finalize();
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
	_NR<InteractiveObject> hitTest(_NR<InteractiveObject> last, number_t x, number_t y);
	void invalidate() { TokenContainer::invalidate(); }
	void requestInvalidation();
};

struct FrameLabel_data
{
	FrameLabel_data() : frame(0) {}
	FrameLabel_data(uint32_t _frame, tiny_string _name) : frame(_frame), name(_name) {}
	uint32_t frame;
	tiny_string name;
};

class FrameLabel: public ASObject, public FrameLabel_data
{
public:
	FrameLabel() {}
	FrameLabel(const FrameLabel_data& data) : FrameLabel_data(data) {}
	static void sinit(Class_base* c);
	ASFUNCTION(_getFrame);
	ASFUNCTION(_getName);
};

struct Scene_data
{
	Scene_data() : startframe(0) {}
	//this vector is sorted with respect to frame
	std::vector<FrameLabel_data> labels;
	tiny_string name;
	uint32_t startframe;
	void addFrameLabel(uint32_t frame, const tiny_string& label);
};

class Scene: public ASObject, public Scene_data
{
	uint32_t numFrames;
public:
	Scene() {}
	Scene(const Scene_data& data, uint32_t _numFrames) : Scene_data(data), numFrames(_numFrames) {}
	static void sinit(Class_base* c);
	ASFUNCTION(_constructor);
	ASFUNCTION(_getLabels);
	ASFUNCTION(_getName);
	ASFUNCTION(_getNumFrames);
};

class Frame
{
public:
	std::list<DisplayListTag*> blueprint;
	void execute(_R<DisplayObjectContainer> displayList);
};

class MovieClip: public Sprite
{
friend class ParserThread;
private:
	uint32_t getCurrentScene();
	ACQUIRE_RELEASE_FLAG(constructed);
	bool isConstructed() const { return ACQUIRE_READ(constructed); }
	/* This list is accessed by both the vm thread and the parsing thread,
	 * but the parsing thread only accesses frames.back(), while
	 * the vm thread only accesses the frames before that frame (until
	 * the parsing finished; then it can also access the last frame).
	 * To make that easier for the vm thread, the member framesLoaded keep
	 * track of how many frames the vm may access. Access to framesLoaded
	 * is guarded by a spinlock.
	 * For non-RootMovieClips, the parser fills the frames member before
	 * handing the object to the vm, so there is no issue here.
	 * RootMovieClips use the new_frame semaphore to wait
	 * for a finished frame from the parser.
	 * It cannot be implemented as std::vector, because then reallocation
	 * would break concurrent access.
	 */
protected:
	std::list<Frame> frames;
	/* This is read from the SWF header. It's only purpose is for flash.display.MovieClip.totalFrames */
	uint32_t totalFrames_unreliable;
	uint32_t getFramesLoaded() { SpinlockLocker l(framesLoadedLock); return framesLoaded; }
	void setFramesLoaded(uint32_t fl) { SpinlockLocker l(framesLoadedLock); framesLoaded = fl; }
	void bootstrap();
private:
	Spinlock framesLoadedLock;
	uint32_t framesLoaded;
	std::map<uint32_t,_NR<IFunction> > frameScripts;
	std::vector<Scene_data> scenes;
public:
	RunState state;
	MovieClip();
	MovieClip(const MovieClip& r);
	void finalize();
	ASObject* gotoAnd(ASObject* const* args, const unsigned int argslen, bool stop);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
	ASFUNCTION(swapDepths);
	ASFUNCTION(createEmptyMovieClip);
	ASFUNCTION(addFrameScript);
	ASFUNCTION(stop);
	ASFUNCTION(gotoAndStop);
	ASFUNCTION(gotoAndPlay);
	ASFUNCTION(nextFrame);
	ASFUNCTION(_getCurrentFrame);
	ASFUNCTION(_getCurrentFrameLabel);
	ASFUNCTION(_getCurrentLabel);
	ASFUNCTION(_getTotalFrames);
	ASFUNCTION(_getFramesLoaded);
	ASFUNCTION(_getScenes);
	ASFUNCTION(_getCurrentScene);

	virtual void addToFrame(DisplayListTag* r);

	void advanceFrame();
	uint32_t getFrameIdByLabel(const tiny_string& l) const;
	void setTotalFrames(uint32_t t);

	//DisplayObject interface
	void setParent(_NR<DisplayObjectContainer> p);

	void check() const
	{
		assert_and_throw(frames.size()==framesLoaded);
	}
	void addScene(uint32_t sceneNo, uint32_t startframe, const tiny_string& name);
	void addFrameLabel(uint32_t frame, const tiny_string& label);

	void constructionComplete();
};

class Stage: public DisplayObjectContainer
{
private:
	uint32_t internalGetHeight() const;
	uint32_t internalGetWidth() const;
public:
	Stage();
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
	ASFUNCTION(_getStageWidth);
	ASFUNCTION(_getStageHeight);
	ASFUNCTION(_getScaleMode);
	ASFUNCTION(_setScaleMode);
	ASFUNCTION(_getLoaderInfo);
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

class StageQuality: public ASObject
{
public:
	static void sinit(Class_base* c);
};

class StageDisplayState: public ASObject
{
public:
	static void sinit(Class_base* c);
};

class LineScaleMode: public ASObject
{
public:
	static void sinit(Class_base* c);
};

class BlendMode: public ASObject
{
public:
	static void sinit(Class_base* c);
};

class GradientType: public ASObject
{
public:
	static void sinit(Class_base* c);
};

class InterpolationMethod: public ASObject
{
public:
	static void sinit(Class_base* c);
};

class SpreadMethod: public ASObject
{
public:
	static void sinit(Class_base* c);
};

class IntSize
{
public:
	uint32_t width;
	uint32_t height;
	IntSize(uint32_t w, uint32_t h):width(h),height(h){}
};

class Bitmap: public DisplayObject
{
friend class CairoRenderer;
protected:
	bool fromJPEG( uint8_t* data, int len);
	IntSize size;
	/* the bitmaps data in cairo's internal representation */
	uint8_t* data;
public:
	Bitmap() : size(0,0), data(NULL) {}
	static void sinit(Class_base* c);
	bool getBounds(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const;
	virtual IntSize getBitmapSize() const;
};

};

#endif
