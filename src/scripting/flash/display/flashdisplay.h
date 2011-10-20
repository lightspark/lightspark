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

#include <boost/bimap.hpp>
#include "compat.h"

#include "swftypes.h"
#include "flash/events/flashevents.h"
#include "thread_pool.h"
#include "flash/utils/flashutils.h"
#include "backends/geometry.h"
#include "backends/graphics.h"
#include "backends/netutils.h"

namespace lightspark
{

class Stage;
class RootMovieClip;
class DisplayListTag;
class LoaderInfo;
class DisplayObjectContainer;
class InteractiveObject;
class Downloader;
class AccessibilityProperties;

class DisplayObject: public EventDispatcher
{
friend class TokenContainer;
friend std::ostream& operator<<(std::ostream& s, const DisplayObject& r);
public:
	enum HIT_TYPE { GENERIC_HIT, DOUBLE_CLICK };
private:
	ASPROPERTY_GETTER_SETTER(_NR<AccessibilityProperties>,accessibilityProperties);
	static ATOMIC_INT32(instanceCount);
	MATRIX Matrix;
	ACQUIRE_RELEASE_FLAG(useMatrix);
	number_t tx,ty;
	number_t rotation;
	number_t sx,sy;
	/**
	  	The object we are masking, if any
	*/
	_NR<DisplayObject> maskOf;
	void becomeMaskOf(_NR<DisplayObject> m);
	void setMask(_NR<DisplayObject> m);
	_NR<DisplayObjectContainer> parent;
protected:
	~DisplayObject();
	/**
	  	The object that masks us, if any
	*/
	_NR<DisplayObject> mask;
	mutable Spinlock spinlock;
	void computeDeviceBoundsForRect(number_t xmin, number_t xmax, number_t ymin, number_t ymax,
			int32_t& outXMin, int32_t& outYMin, uint32_t& outWidth, uint32_t& outHeight) const;
	void valFromMatrix();
	bool onStage;
	_NR<LoaderInfo> loaderInfo;
	number_t computeWidth();
	number_t computeHeight();
	bool isSimple() const;
	bool skipRender(bool maskEnabled) const;
	float alpha;
	bool visible;
	/* cachedSurface may only be read/written from within the render thread */
	CachedSurface cachedSurface;

	void defaultRender(bool maskEnabled) const;
	DisplayObject(const DisplayObject& d);
	void renderPrologue() const;
	void renderEpilogue() const;
	void hitTestPrologue() const;
	void hitTestEpilogue() const;
	virtual bool boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const
	{
		throw RunTimeException("DisplayObject::boundsRect: Derived class must implement this!");
	}
	virtual void renderImpl(bool maskEnabled, number_t t1,number_t t2,number_t t3,number_t t4) const
	{
		throw RunTimeException("DisplayObject::renderImpl: Derived class must implement this!");
	}
	virtual _NR<InteractiveObject> hitTestImpl(_NR<InteractiveObject> last, number_t x, number_t y, HIT_TYPE type)
	{
		throw RunTimeException("DisplayObject::hitTestImpl: Derived class must implement this!");
	}
public:
	tiny_string name;
	UI16_SWF CharacterId;
	CXFORMWITHALPHA ColorTransform;
	UI16_SWF Ratio;
	UI16_SWF ClipDepth;
	CLIPACTIONS ClipActions;
	_NR<DisplayObjectContainer> getParent() const { return parent; }
	void setParent(_NR<DisplayObjectContainer> p);
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
	void localToGlobal(number_t xin, number_t yin, number_t& xout, number_t& yout) const;
	void globalToLocal(number_t xin, number_t yin, number_t& xout, number_t& yout) const;
	float getConcatenatedAlpha() const;
	virtual float getScaleFactor() const
	{
		throw RunTimeException("DisplayObject::getScaleFactor");
	}
	void Render(bool maskEnabled);
	bool getBounds(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const;
	_NR<InteractiveObject> hitTest(_NR<InteractiveObject> last, number_t x, number_t y, HIT_TYPE type);
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
	virtual void advanceFrame() {}
	virtual void initFrame();
	Vector2f getLocalMousePos();
	Vector2f getXY();
	void setX(number_t x);
	void setY(number_t y);
	// Nominal width and heigt are the size before scaling and rotation
	number_t getNominalWidth();
	number_t getNominalHeight();
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
	ASFUNCTION(globalToLocal);
};

class InteractiveObject: public DisplayObject
{
protected:
	bool mouseEnabled;
	bool doubleClickEnabled;
	bool isHittable(DisplayObject::HIT_TYPE type)
	{
		if(type == DisplayObject::DOUBLE_CLICK)
			return doubleClickEnabled && mouseEnabled;
		return mouseEnabled;
	}
	~InteractiveObject();
public:
	InteractiveObject();
	ASFUNCTION(_constructor);
	ASFUNCTION(_setMouseEnabled);
	ASFUNCTION(_getMouseEnabled);
	ASFUNCTION(_setDoubleClickEnabled);
	ASFUNCTION(_getDoubleClickEnabled);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
};

class DisplayObjectContainer: public InteractiveObject
{
private:
	boost::bimap<uint32_t,DisplayObject*> depthToLegacyChild;
	bool _contains(_R<DisplayObject> child);
	bool mouseChildren;
protected:
	void requestInvalidation();
	//This is shared between RenderThread and VM
	std::list < _R<DisplayObject> > dynamicDisplayList;
	//The lock should only be taken when doing write operations
	//As the RenderThread only reads, it's safe to read without the lock
	mutable Mutex mutexDisplayList;
	void setOnStage(bool staged);
	_NR<InteractiveObject> hitTestImpl(_NR<InteractiveObject> last, number_t x, number_t y, DisplayObject::HIT_TYPE type);
	bool boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const;
	void renderImpl(bool maskEnabled, number_t t1,number_t t2,number_t t3,number_t t4) const;
public:
	void _addChildAt(_R<DisplayObject> child, unsigned int index);
	void dumpDisplayList();
	bool _removeChild(_R<DisplayObject> child);
	int getChildIndex(_R<DisplayObject> child);
	DisplayObjectContainer();
	void finalize();
	bool hasLegacyChildAt(uint32_t depth);
	void deleteLegacyChildAt(uint32_t depth);
	void insertLegacyChildAt(uint32_t depth, DisplayObject* obj);
	void transformLegacyChildAt(uint32_t depth, const MATRIX& mat);
	void purgeLegacyChildren();
	void advanceFrame();
	void initFrame();
	bool isOpaque(number_t x, number_t y) const;
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
	ASFUNCTION(_getNumChildren);
	ASFUNCTION(addChild);
	ASFUNCTION(removeChild);
	ASFUNCTION(removeChildAt);
	ASFUNCTION(addChildAt);
	ASFUNCTION(_getChildIndex);
	ASFUNCTION(_setChildIndex);
	ASFUNCTION(getChildAt);
	ASFUNCTION(getChildByName);
	ASFUNCTION(contains);
	ASFUNCTION(_getMouseChildren);
	ASFUNCTION(_setMouseChildren);
	ASFUNCTION(swapChildren);
};

/* This is really ugly, but the parent of the current
 * active state (e.g. upState) is set to the owning SimpleButton,
 * which is not a DisplayObjectContainer per spec.
 * We let it derive from DisplayObjectContainer, but
 * call only the InteractiveObject::_constructor
 * to make it look like an InteractiveObject to AS.
 */
class SimpleButton: public DisplayObjectContainer
{
private:
	_NR<DisplayObject> downState;
	_NR<DisplayObject> hitTestState;
	_NR<DisplayObject> overState;
	_NR<DisplayObject> upState;
	bool enabled;
	bool useHandCursor;
	enum
	{
		UP,
		OVER,
		DOWN
	} currentState;
	void reflectState();
	_NR<InteractiveObject> hitTestImpl(_NR<InteractiveObject> last, number_t x, number_t y, DisplayObject::HIT_TYPE type);
	/* This is called by when an event is dispatched */
	void defaultEventBehavior(_R<Event> e);
public:
	SimpleButton(DisplayObject *dS = NULL, DisplayObject *hTS = NULL,
				 DisplayObject *oS = NULL, DisplayObject *uS = NULL);
	void finalize();
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
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

class TokenContainer
{
	friend class Graphics;
public:
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
	static void FromShaperecordListToShapeVector(const std::vector<SHAPERECORD>& shapeRecords,
					 std::vector<GeomToken>& tokens, const std::list<FILLSTYLE>& fillStyles,
					 const Vector2& offset = Vector2(), int scaling = 1);
protected:
	TokenContainer(DisplayObject* _o) : owner(_o), scaling(1.0f) {}
	TokenContainer(DisplayObject* _o, const std::vector<GeomToken>& _tokens, float _scaling)
		: owner(_o), scaling(_scaling), tokens(_tokens) {}

	void invalidate();
	void requestInvalidation();
	bool boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const;
	_NR<InteractiveObject> hitTestImpl(_NR<InteractiveObject> last, number_t x, number_t y, DisplayObject::HIT_TYPE type) const;
	void renderImpl(bool maskEnabled, number_t t1, number_t t2, number_t t3, number_t t4) const;
	bool tokensEmpty() const { return tokens.empty(); }
	bool isOpaqueImpl(number_t x, number_t y) const;
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
	bool boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const
		{ return TokenContainer::boundsRect(xmin,xmax,ymin,ymax); }
	void renderImpl(bool maskEnabled, number_t t1, number_t t2, number_t t3, number_t t4) const
		{ TokenContainer::renderImpl(maskEnabled,t1,t2,t3,t4); }
	_NR<InteractiveObject> hitTestImpl(_NR<InteractiveObject> last, number_t x, number_t y, DisplayObject::HIT_TYPE type)
		{ return TokenContainer::hitTestImpl(last,x,y, type); }
public:
	Shape():TokenContainer(this), graphics(NULL) {}
	Shape(const std::vector<GeomToken>& tokens, float scaling)
		: TokenContainer(this, tokens, scaling), graphics(NULL) {}
	void finalize();
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
	ASFUNCTION(_getGraphics);
	bool isOpaque(number_t x, number_t y) const;
	void requestInvalidation() { TokenContainer::requestInvalidation(); }
	void invalidate() { TokenContainer::invalidate(); }
};

class MorphShape: public DisplayObject
{
protected:
	bool boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const;
	virtual _NR<InteractiveObject> hitTestImpl(_NR<InteractiveObject> last, number_t x, number_t y, HIT_TYPE type);
public:
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
};

class Loader;

class LoaderInfo: public EventDispatcher, public ILoadable
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
	ASFUNCTION(_getWidth);
	ASFUNCTION(_getHeight);
	void sendInit();
	//ILoadable interface
	void setBytesTotal(uint32_t b)
	{
		bytesTotal=b;
	}
	void setBytesLoaded(uint32_t b);
};

class Loader: public IThreadJob, public DisplayObjectContainer
{
private:
	enum SOURCE { URL, BYTES };
	mutable Spinlock contentSpinlock;
	_NR<DisplayObject> content;
	bool loading;
	bool loaded;
	SOURCE source;
	URLInfo url;
	std::vector<uint8_t> postData;
	_NR<ByteArray> bytes;
	_NR<LoaderInfo> contentLoaderInfo;
	Spinlock downloaderLock;
	Downloader* downloader;
	void execute();
	void threadAbort();
	void jobFence();
public:
	Loader():content(NullRef),loading(false),loaded(false),bytes(NullRef),contentLoaderInfo(NullRef),downloader(NULL)
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
	void setContent(_R<DisplayObject> o);
	_NR<DisplayObject> getContent() { return content; }
	_R<LoaderInfo> getContentLoaderInfo() { return contentLoaderInfo; }
};

class Sprite: public DisplayObjectContainer, public TokenContainer
{
friend class DisplayObject;
private:
	_NR<Graphics> graphics;
protected:
	bool boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const;
	void renderImpl(bool maskEnabled, number_t t1,number_t t2,number_t t3,number_t t4) const;
	_NR<InteractiveObject> hitTestImpl(_NR<InteractiveObject> last, number_t x, number_t y, DisplayObject::HIT_TYPE type);
public:
	Sprite();
	void finalize();
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
	ASFUNCTION(_getGraphics);
	ASFUNCTION(_startDrag);
	ASFUNCTION(_stopDrag);
	int getDepth() const
	{
		return 0;
	}
	void invalidate() { TokenContainer::invalidate(); }
	void requestInvalidation();
	bool isOpaque(number_t x, number_t y) const;
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
	static void buildTraits(ASObject* o);
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
	std::list<_R<DisplayListTag>> blueprint;
	void execute(_R<DisplayObjectContainer> displayList);
};

class MovieClip: public Sprite
{
friend class ParserThread;
private:
	uint32_t getCurrentScene();
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
	void constructionComplete();
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
	/*
	 * returns true if all frames of this MovieClip are loaded
	 * this is overwritten in RootMovieClip, because that one is
	 * executed while loading
	 */
	virtual bool hasFinishedLoading() { return true; }
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
	ASFUNCTION(_getCurrentLabels);
	ASFUNCTION(_getTotalFrames);
	ASFUNCTION(_getFramesLoaded);
	ASFUNCTION(_getScenes);
	ASFUNCTION(_getCurrentScene);

	virtual void addToFrame(_R<DisplayListTag> r);

	void advanceFrame();
	void initFrame();
	uint32_t getFrameIdByLabel(const tiny_string& l) const;
	void setTotalFrames(uint32_t t);

	void check() const
	{
		assert_and_throw(frames.size()==framesLoaded);
	}
	void addScene(uint32_t sceneNo, uint32_t startframe, const tiny_string& name);
	void addFrameLabel(uint32_t frame, const tiny_string& label);
};

class Stage: public DisplayObjectContainer
{
private:
	uint32_t internalGetHeight() const;
	uint32_t internalGetWidth() const;
	void onDisplayState(const tiny_string&);
public:
	_NR<InteractiveObject> hitTestImpl(_NR<InteractiveObject> last, number_t x, number_t y, DisplayObject::HIT_TYPE type);
	void setOnStage(bool staged) { assert(false); /* we are the stage */}
	Stage();
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
	ASFUNCTION(_getStageWidth);
	ASFUNCTION(_getStageHeight);
	ASFUNCTION(_getScaleMode);
	ASFUNCTION(_setScaleMode);
	ASFUNCTION(_getLoaderInfo);
	ASPROPERTY_GETTER_SETTER(tiny_string,displayState);
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

class Bitmap: public DisplayObject, public TokenContainer
{
friend class CairoTokenRenderer;
protected:
	bool fromRGB(uint8_t* rgb, uint32_t width, uint32_t height, bool hasAlpha);
	bool fromJPEG( uint8_t* data, int len);
	bool fromJPEG(std::istream& s);
	IntSize size;
	/* the bitmaps data in cairo's internal representation */
	uint8_t* data;
	void renderImpl(bool maskEnabled, number_t t1, number_t t2, number_t t3, number_t t4) const
		{ TokenContainer::renderImpl(maskEnabled,t1,t2,t3,t4); }
public:
	Bitmap() : TokenContainer(this), size(0,0), data(NULL) {}
	Bitmap(std::istream *s, FILE_TYPE type=FT_UNKNOWN);
	static void sinit(Class_base* c);
	bool boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const;
	_NR<InteractiveObject> hitTestImpl(_NR<InteractiveObject> last, number_t x, number_t y, DisplayObject::HIT_TYPE type);
	virtual IntSize getBitmapSize() const;
	void requestInvalidation() { TokenContainer::requestInvalidation(); }
	void invalidate() { TokenContainer::invalidate(); }
};

class AVM1Movie: public DisplayObject
{
public:
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
};

class Shader : public ASObject
{
public:
	static void sinit(Class_base* c);
	ASFUNCTION(_constructor);
};

};

#endif
