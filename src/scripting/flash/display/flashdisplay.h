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
#include "DisplayObject.h"

namespace lightspark
{

class RootMovieClip;
class DisplayListTag;
class InteractiveObject;
class Downloader;
class RenderContext;
class ApplicationDomain;

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
	InteractiveObject(Class_base* c);
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
	void requestInvalidation(InvalidateQueue* q);
	//This is shared between RenderThread and VM
	std::list < _R<DisplayObject> > dynamicDisplayList;
	//The lock should only be taken when doing write operations
	//As the RenderThread only reads, it's safe to read without the lock
	mutable Mutex mutexDisplayList;
	void setOnStage(bool staged);
	_NR<InteractiveObject> hitTestImpl(_NR<InteractiveObject> last, number_t x, number_t y, DisplayObject::HIT_TYPE type);
	bool boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const;
	void renderImpl(RenderContext& ctxt, bool maskEnabled, number_t t1,number_t t2,number_t t3,number_t t4) const;
public:
	void _addChildAt(_R<DisplayObject> child, unsigned int index);
	void dumpDisplayList();
	bool _removeChild(_R<DisplayObject> child);
	int getChildIndex(_R<DisplayObject> child);
	DisplayObjectContainer(Class_base* c);
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
	SimpleButton(Class_base* c, DisplayObject *dS = NULL, DisplayObject *hTS = NULL,
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
					 tokensVector& tokens, const std::list<FILLSTYLE>& fillStyles,
					 const Vector2& offset = Vector2(), int scaling = 1);
	void getTextureSize(int *width, int *height) const;
protected:
	TokenContainer(DisplayObject* _o);
	TokenContainer(DisplayObject* _o, const tokensVector& _tokens, float _scaling);

	void invalidate();
	void requestInvalidation(InvalidateQueue* q);
	bool boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const;
	_NR<InteractiveObject> hitTestImpl(_NR<InteractiveObject> last, number_t x, number_t y, DisplayObject::HIT_TYPE type) const;
	void renderImpl(RenderContext& ctxt, bool maskEnabled, number_t t1, number_t t2, number_t t3, number_t t4) const;
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
	static void solveVertexMapping(double x1, double y1,
				       double x2, double y2,
				       double x3, double y3,
				       double u1, double u2, double u3,
				       double c[3]);
public:
	Graphics(Class_base* c):ASObject(c),owner(NULL)
	{
		throw RunTimeException("Cannot instantiate a Graphics object");
	}
	Graphics(Class_base* c, TokenContainer* _o)
		: ASObject(c),curX(0),curY(0),owner(_o) {}
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
	ASFUNCTION(lineStyle);
	ASFUNCTION(beginFill);
	ASFUNCTION(beginGradientFill);
	ASFUNCTION(beginBitmapFill);
	ASFUNCTION(endFill);
	ASFUNCTION(drawRect);
	ASFUNCTION(drawRoundRect);
	ASFUNCTION(drawCircle);
	ASFUNCTION(drawTriangles);
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
	void renderImpl(RenderContext& ctxt, bool maskEnabled, number_t t1, number_t t2, number_t t3, number_t t4) const
		{ TokenContainer::renderImpl(ctxt, maskEnabled,t1,t2,t3,t4); }
	_NR<InteractiveObject> hitTestImpl(_NR<InteractiveObject> last, number_t x, number_t y, DisplayObject::HIT_TYPE type)
		{ return TokenContainer::hitTestImpl(last,x,y, type); }
public:
	Shape(Class_base* c);
	Shape(Class_base* c, const tokensVector& tokens, float scaling);
	void finalize();
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
	ASFUNCTION(_getGraphics);
	bool isOpaque(number_t x, number_t y) const;
	void requestInvalidation(InvalidateQueue* q) { TokenContainer::requestInvalidation(q); }
	void invalidate() { TokenContainer::invalidate(); }
};

class MorphShape: public DisplayObject
{
protected:
	bool boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const;
	virtual _NR<InteractiveObject> hitTestImpl(_NR<InteractiveObject> last, number_t x, number_t y, HIT_TYPE type);
public:
	MorphShape(Class_base* c):DisplayObject(c){}
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
};

class Loader;

class LoaderInfo: public EventDispatcher, public ILoadable
{
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
	ASPROPERTY_GETTER(_NR<ASObject>,parameters);
	ASPROPERTY_GETTER(uint32_t,actionScriptVersion);
	LoaderInfo(Class_base* c);
	LoaderInfo(Class_base* c, _R<Loader> l);
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
	_NR<ApplicationDomain> applicationDomain;
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
	void setURL(const tiny_string& _url) { url=_url; }
	void setLoaderURL(const tiny_string& _url) { loaderURL=_url; }
	void resetState();
};

class URLRequest;

class LoaderThread : public DownloaderThreadBase
{
private:
	enum SOURCE { URL, BYTES };
	_NR<ByteArray> bytes;
	_R<Loader> loader;
	_NR<LoaderInfo> loaderInfo;
	SOURCE source;
	void execute();
public:
	LoaderThread(_R<URLRequest> request, _R<Loader> loader);
	LoaderThread(_R<ByteArray> bytes, _R<Loader> loader);
};

class Loader: public DisplayObjectContainer, public IDownloaderThreadListener
{
private:
	mutable Spinlock spinlock;
	_NR<DisplayObject> content;
	IThreadJob *job;
	bool loaded;
	URLInfo url;
	_NR<LoaderInfo> contentLoaderInfo;
	void unload();
public:
	Loader(Class_base* c);
	~Loader();
	void finalize();
	void threadFinished(IThreadJob* job);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
	ASFUNCTION(close);
	ASFUNCTION(load);
	ASFUNCTION(loadBytes);
	ASFUNCTION(_unload);
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
	void renderImpl(RenderContext& ctxt, bool maskEnabled, number_t t1,number_t t2,number_t t3,number_t t4) const;
	_NR<InteractiveObject> hitTestImpl(_NR<InteractiveObject> last, number_t x, number_t y, DisplayObject::HIT_TYPE type);
public:
	Sprite(Class_base* c);
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
	void requestInvalidation(InvalidateQueue* q);
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
	FrameLabel(Class_base* c);
	FrameLabel(Class_base* c, const FrameLabel_data& data);
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
	Scene(Class_base* c);
	Scene(Class_base* c, const Scene_data& data, uint32_t _numFrames);
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
	MovieClip(Class_base* c);
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
	Stage(Class_base* c);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	_NR<Stage> getStage();
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
	StageScaleMode(Class_base* c):ASObject(c){}
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o)
	{
	}
};

class StageAlign: public ASObject
{
public:
	StageAlign(Class_base* c):ASObject(c){}
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o)
	{
	}
};

class StageQuality: public ASObject
{
public:
	StageQuality(Class_base* c):ASObject(c){}
	static void sinit(Class_base* c);
};

class StageDisplayState: public ASObject
{
public:
	StageDisplayState(Class_base* c):ASObject(c){}
	static void sinit(Class_base* c);
};

class LineScaleMode: public ASObject
{
public:
	LineScaleMode(Class_base* c):ASObject(c){}
	static void sinit(Class_base* c);
};

class BlendMode: public ASObject
{
public:
	BlendMode(Class_base* c):ASObject(c){}
	static void sinit(Class_base* c);
};

class GradientType: public ASObject
{
public:
	GradientType(Class_base* c):ASObject(c){}
	static void sinit(Class_base* c);
};

class InterpolationMethod: public ASObject
{
public:
	InterpolationMethod(Class_base* c):ASObject(c){}
	static void sinit(Class_base* c);
};

class SpreadMethod: public ASObject
{
public:
	SpreadMethod(Class_base* c):ASObject(c){}
	static void sinit(Class_base* c);
};

class IntSize
{
public:
	uint32_t width;
	uint32_t height;
	IntSize(uint32_t w, uint32_t h):width(h),height(h){}
};

class BitmapData: public ASObject, public IBitmapDrawable
{
protected:
	size_t stride;
	size_t dataSize;
	bool disposed;
	uint32_t getPixelPriv(uint32_t x, uint32_t y);
	void setPixelPriv(uint32_t x, uint32_t y, uint32_t color, bool setAlpha);
	void copyFrom(BitmapData *source);
public:
	BitmapData(Class_base* c);
	static void sinit(Class_base* c);
	~BitmapData();
	/* the bitmaps data in premultiplied, native-endian 32 bit
	 * ARGB format. stride is the number of bytes per row, may be
	 * larger than width. dataSize is the total allocated size of
	 * data (=stride*height) */
	std::vector<uint8_t, reporter_allocator<uint8_t>> data;
	uint8_t* getData() { return &data[0]; }
	ASPROPERTY_GETTER(int32_t, width);
	ASPROPERTY_GETTER(int32_t, height);
	ASPROPERTY_GETTER(bool, transparent);
	ASFUNCTION(_constructor);
	ASFUNCTION(dispose);
	ASFUNCTION(draw);
	ASFUNCTION(getPixel);
	ASFUNCTION(getPixel32);
	ASFUNCTION(setPixel);
	ASFUNCTION(setPixel32);
	ASFUNCTION(getRect);
	ASFUNCTION(copyPixels);
	ASFUNCTION(fillRect);
	ASFUNCTION(generateFilterRect);
	bool fromRGB(uint8_t* rgb, uint32_t width, uint32_t height, bool hasAlpha);
	bool fromJPEG(uint8_t* data, int len);
	bool fromJPEG(std::istream& s);
	bool fromPNG(std::istream& s);
	int getWidth() const { return width; }
	int getHeight() const { return height; }
};

class Bitmap: public DisplayObject, public TokenContainer
{
friend class CairoTokenRenderer;
private:
	void onBitmapData(_NR<BitmapData>);
protected:
	void renderImpl(RenderContext& ctxt, bool maskEnabled, number_t t1, number_t t2, number_t t3, number_t t4) const
		{ TokenContainer::renderImpl(ctxt, maskEnabled,t1,t2,t3,t4); }
public:
	ASPROPERTY_GETTER_SETTER(_NR<BitmapData>,bitmapData);
	/* Call this after updating any member of 'data' */
	void updatedData();
	Bitmap(Class_base* c, std::istream *s = NULL, FILE_TYPE type=FT_UNKNOWN);
	Bitmap(Class_base* c, _R<BitmapData> data);
	static void sinit(Class_base* c);
	ASFUNCTION(_constructor);
	bool boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const;
	_NR<InteractiveObject> hitTestImpl(_NR<InteractiveObject> last, number_t x, number_t y, DisplayObject::HIT_TYPE type);
	virtual IntSize getBitmapSize() const;
	void requestInvalidation(InvalidateQueue* q) { TokenContainer::requestInvalidation(q); }
	void invalidate() { TokenContainer::invalidate(); }
};

class AVM1Movie: public DisplayObject
{
public:
	AVM1Movie(Class_base* c):DisplayObject(c){}
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
};

class Shader : public ASObject
{
public:
	Shader(Class_base* c):ASObject(c){}
	static void sinit(Class_base* c);
	ASFUNCTION(_constructor);
};

};

#endif
