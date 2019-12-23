/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef SCRIPTING_FLASH_DISPLAY_FLASHDISPLAY_H
#define SCRIPTING_FLASH_DISPLAY_FLASHDISPLAY_H 1

#ifndef BOOST_BIMAP_DISABLE_SERIALIZATION
#include <boost/serialization/split_member.hpp>
#endif

#include <boost/bimap.hpp>
#include "compat.h"

#include "swftypes.h"
#include "scripting/flash/events/flashevents.h"
#include "thread_pool.h"
#include "scripting/flash/utils/flashutils.h"
#include "backends/graphics.h"
#include "backends/netutils.h"
#include "scripting/flash/display/DisplayObject.h"
#include "scripting/flash/display/TokenContainer.h"
#include "scripting/flash/display3d/flashdisplay3d.h"
#include "scripting/flash/ui/ContextMenu.h"
#include "scripting/flash/accessibility/flashaccessibility.h"
#include "scripting/toplevel/toplevel.h"

namespace lightspark
{

class RootMovieClip;
class DisplayListTag;
class AVM1ActionTag;
class AVM1InitActionTag;
class DefineButtonTag;
class InteractiveObject;
class Downloader;
class RenderContext;
class RenderContext;
class ApplicationDomain;
class SecurityDomain;
class BitmapData;
class Matrix;
class Vector;
class Graphics;
class Rectangle;
class Class_inherit;
class Point;

class InteractiveObject: public DisplayObject
{
protected:
	bool mouseEnabled;
	bool doubleClickEnabled;
	bool isHittable(DisplayObject::HIT_TYPE type)
	{
		if(type == DisplayObject::MOUSE_CLICK)
			return mouseEnabled;
		else if(type == DisplayObject::DOUBLE_CLICK)
			return doubleClickEnabled && mouseEnabled;
		else
			return true;
	}
	~InteractiveObject();
public:
	InteractiveObject(Class_base* c);
	ASPROPERTY_GETTER_SETTER(_NR<AccessibilityImplementation>,accessibilityImplementation);
	ASPROPERTY_GETTER_SETTER(_NR<ContextMenu>,contextMenu); // TOOD: should be NativeMenu
	ASPROPERTY_GETTER_SETTER(bool,tabEnabled);
	ASPROPERTY_GETTER_SETTER(int32_t,tabIndex);
	ASPROPERTY_GETTER_SETTER(_NR<ASObject>,focusRect);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(_setMouseEnabled);
	ASFUNCTION_ATOM(_getMouseEnabled);
	ASFUNCTION_ATOM(_setDoubleClickEnabled);
	ASFUNCTION_ATOM(_getDoubleClickEnabled);
	bool destruct() override;
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	virtual void lostFocus() {}
	virtual void gotFocus() {} 
	virtual void textInputChanged(const tiny_string& newtext) {} 
	void setMouseEnabled(bool enabled) { mouseEnabled = enabled; }
};

class DisplayObjectContainer: public InteractiveObject
{
private:
	bool mouseChildren;
	boost::bimap<int32_t,DisplayObject*> depthToLegacyChild;
	map<int32_t,DisplayObject*> namedRemovedLegacyChildren;
	set<int32_t> legacyChildrenMarkedForDeletion;
	bool _contains(_R<DisplayObject> child);
	void getObjectsFromPoint(Point* point, Array* ar);
protected:
	//This is shared between RenderThread and VM
	std::vector < _R<DisplayObject> > dynamicDisplayList;
	//The lock should only be taken when doing write operations
	//As the RenderThread only reads, it's safe to read without the lock
	mutable Mutex mutexDisplayList;
	void setOnStage(bool staged, bool force = false) override;
	_NR<DisplayObject> hitTestImpl(_NR<DisplayObject> last, number_t x, number_t y, DisplayObject::HIT_TYPE type,bool interactiveObjectsOnly) override;
	bool boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const override;
	bool renderImpl(RenderContext& ctxt) const override;
	virtual void resetToStart() {}
	ASPROPERTY_GETTER_SETTER(bool, tabChildren);
	void LegacyChildEraseDeletionMarked();
public:
	DisplayObject* findRemovedLegacyChild(uint32_t name);
	void LegacyChildRemoveDeletionMark(int32_t depth);
	void requestInvalidation(InvalidateQueue* q) override;
	void _addChildAt(_R<DisplayObject> child, unsigned int index);
	void dumpDisplayList(unsigned int level=0);
	bool _removeChild(DisplayObject* child);
	int getChildIndex(_R<DisplayObject> child);
	DisplayObjectContainer(Class_base* c);
	bool destruct() override;
	void resetLegacyState() override;
	bool hasLegacyChildAt(int32_t depth);
	// this does not test if a DisplayObject exists at the provided depth
	DisplayObject* getLegacyChildAt(int32_t depth);
	void setupClipActionsAt(int32_t depth, const CLIPACTIONS& actions);
	void checkRatioForLegacyChildAt(int32_t depth, uint32_t ratio);
	void checkColorTransformForLegacyChildAt(int32_t depth, const CXFORMWITHALPHA& colortransform);
	void deleteLegacyChildAt(int32_t depth);
	void insertLegacyChildAt(int32_t depth, DisplayObject* obj);
	int findLegacyChildDepth(DisplayObject* obj);
	void transformLegacyChildAt(int32_t depth, const MATRIX& mat);
	uint32_t getMaxLegacyChildDepth();
	void purgeLegacyChildren();
	void checkClipDepth();
	void advanceFrame() override;
	void declareFrame() override;
	void initFrame() override;
	void executeFrameScript() override;
	multiname* setVariableByMultiname(const multiname& name, asAtom& o, CONST_ALLOWED_FLAG allowConst, bool* alreadyset=nullptr) override;
	bool deleteVariableByMultiname(const multiname& name) override;
	
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(_getNumChildren);
	ASFUNCTION_ATOM(addChild);
	ASFUNCTION_ATOM(removeChild);
	ASFUNCTION_ATOM(removeChildAt);
	ASFUNCTION_ATOM(removeChildren);
	ASFUNCTION_ATOM(addChildAt);
	ASFUNCTION_ATOM(_getChildIndex);
	ASFUNCTION_ATOM(_setChildIndex);
	ASFUNCTION_ATOM(getChildAt);
	ASFUNCTION_ATOM(getChildByName);
	ASFUNCTION_ATOM(getObjectsUnderPoint);
	ASFUNCTION_ATOM(contains);
	ASFUNCTION_ATOM(_getMouseChildren);
	ASFUNCTION_ATOM(_setMouseChildren);
	ASFUNCTION_ATOM(swapChildren);
	ASFUNCTION_ATOM(swapChildrenAt);
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
	DefineButtonTag* buttontag;
	enum BUTTONSTATE
	{
		UP,
		OVER,
		DOWN,
		STATE_OUT
	};
	BUTTONSTATE currentState;
	bool enabled;
	bool useHandCursor;
	void reflectState();
	_NR<DisplayObject> hitTestImpl(_NR<DisplayObject> last, number_t x, number_t y, DisplayObject::HIT_TYPE type,bool interactiveObjectsOnly) override;
	/* This is called by when an event is dispatched */
	void defaultEventBehavior(_R<Event> e) override;
public:
	SimpleButton(Class_base* c, DisplayObject *dS = nullptr, DisplayObject *hTS = nullptr,
				 DisplayObject *oS = nullptr, DisplayObject *uS = nullptr, DefineButtonTag* tag = nullptr);
	void finalize() override;
	IDrawable* invalidate(DisplayObject* target, const MATRIX& initialMatrix,bool smoothing) override;
	void requestInvalidation(InvalidateQueue* q) override;
	uint32_t getTagID() const override;

	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(_getUpState);
	ASFUNCTION_ATOM(_setUpState);
	ASFUNCTION_ATOM(_getDownState);
	ASFUNCTION_ATOM(_setDownState);
	ASFUNCTION_ATOM(_getOverState);
	ASFUNCTION_ATOM(_setOverState);
	ASFUNCTION_ATOM(_getHitTestState);
	ASFUNCTION_ATOM(_setHitTestState);
	ASFUNCTION_ATOM(_getEnabled);
	ASFUNCTION_ATOM(_setEnabled);
	ASFUNCTION_ATOM(_getUseHandCursor);
	ASFUNCTION_ATOM(_setUseHandCursor);

	void afterLegacyInsert() override;
	void afterLegacyDelete(DisplayObjectContainer* par) override;
	bool AVM1HandleKeyboardEvent(KeyboardEvent* e) override;
	bool AVM1HandleMouseEvent(EventDispatcher* dispatcher, MouseEvent *e) override;
};

class Shape: public DisplayObject, public TokenContainer
{
protected:
	_NR<Graphics> graphics;
	bool boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const override
		{ return TokenContainer::boundsRect(xmin,xmax,ymin,ymax); }
	bool renderImpl(RenderContext& ctxt) const override
		{ return TokenContainer::renderImpl(ctxt); }
	_NR<DisplayObject> hitTestImpl(_NR<DisplayObject> last, number_t x, number_t y, DisplayObject::HIT_TYPE type,bool interactiveObjectsOnly) override
		{
			if (interactiveObjectsOnly)
				this->incRef();
			return TokenContainer::hitTestImpl(interactiveObjectsOnly ? _NR<DisplayObject>(this) : last,x,y, type); 
		}
public:
	Shape(Class_base* c);
	Shape(Class_base* c, const tokensVector& tokens, float scaling);
	void finalize() override;
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(_getGraphics);
	void requestInvalidation(InvalidateQueue* q) override { TokenContainer::requestInvalidation(q); }
	IDrawable* invalidate(DisplayObject* target, const MATRIX& initialMatrix,bool smoothing) override
	{ return TokenContainer::invalidate(target, initialMatrix,smoothing); }
};

class DefineMorphShapeTag;
class MorphShape: public DisplayObject, public TokenContainer
{
private:
	DefineMorphShapeTag* morphshapetag;
protected:
	bool boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const override
		{ return TokenContainer::boundsRect(xmin,xmax,ymin,ymax); }
	bool renderImpl(RenderContext& ctxt) const override
		{ return TokenContainer::renderImpl(ctxt); }
	_NR<DisplayObject> hitTestImpl(_NR<DisplayObject> last, number_t x, number_t y, DisplayObject::HIT_TYPE type,bool interactiveObjectsOnly) override
		{
			if (interactiveObjectsOnly)
				this->incRef();
			return TokenContainer::hitTestImpl(interactiveObjectsOnly ? _NR<DisplayObject>(this) : last,x,y, type); 
		}
public:
	MorphShape(Class_base* c);
	MorphShape(Class_base* c, DefineMorphShapeTag* _morphshapetag);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	void requestInvalidation(InvalidateQueue* q) override { TokenContainer::requestInvalidation(q); }
	IDrawable* invalidate(DisplayObject* target, const MATRIX& initialMatrix,bool smoothing) override
	{ return TokenContainer::invalidate(target, initialMatrix,smoothing); }
	void checkRatio(uint32_t ratio) override;
};

class Loader;

class LoaderInfo: public EventDispatcher, public ILoadable
{
public:
	_NR<ApplicationDomain> applicationDomain;
	_NR<SecurityDomain> securityDomain;
	ASPROPERTY_GETTER(_NR<ASObject>,parameters);
	ASPROPERTY_GETTER(tiny_string, contentType);
private:
	uint32_t bytesLoaded;
	uint32_t bytesTotal;
	tiny_string url;
	tiny_string loaderURL;
	_NR<EventDispatcher> sharedEvents;
	_NR<Loader> loader;
	_NR<ByteArray> bytesData;
	/*
	 * waitedObject is the object we are supposed to wait,
	 * it's necessary when multiple loads are invoked on
	 * the same Loader. Since the construction may complete
	 * after the second load is used we need to know what is
	 * the last object that will notify this LoaderInfo about
	 * completion
	 */
	_NR<DisplayObject> waitedObject;
	_NR<ProgressEvent> progressEvent;
	Spinlock spinlock;
	enum LOAD_STATUS { STARTED=0, INIT_SENT, COMPLETE };
	LOAD_STATUS loadStatus;
	/*
	 * sendInit should be called with the spinlock held
	 */
	void sendInit();
public:
	ASPROPERTY_GETTER(uint32_t,actionScriptVersion);
	ASPROPERTY_GETTER(uint32_t,swfVersion);
	ASPROPERTY_GETTER(bool, childAllowsParent);
	ASPROPERTY_GETTER(_NR<UncaughtErrorEvents>,uncaughtErrorEvents);
	ASPROPERTY_GETTER(bool,parentAllowsChild);
	ASPROPERTY_GETTER(number_t,frameRate);
	LoaderInfo(Class_base* c);
	LoaderInfo(Class_base* c, _R<Loader> l);
	bool destruct();
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(_getLoaderURL);
	ASFUNCTION_ATOM(_getURL);
	ASFUNCTION_ATOM(_getBytesLoaded);
	ASFUNCTION_ATOM(_getBytesTotal);
	ASFUNCTION_ATOM(_getBytes);
	ASFUNCTION_ATOM(_getApplicationDomain);
	ASFUNCTION_ATOM(_getLoader);
	ASFUNCTION_ATOM(_getContent);
	ASFUNCTION_ATOM(_getSharedEvents);
	ASFUNCTION_ATOM(_getWidth);
	ASFUNCTION_ATOM(_getHeight);
	void objectHasLoaded(_R<DisplayObject> obj);
	void setWaitedObject(_NR<DisplayObject> w);
	//ILoadable interface
	void setBytesTotal(uint32_t b)
	{
		bytesTotal=b;
	}
	void setBytesLoaded(uint32_t b);
	uint32_t getBytesLoaded() { return bytesLoaded; }
	uint32_t getBytesTotal() { return bytesTotal; }
	void setURL(const tiny_string& _url, bool setParameters=true);
	void setLoaderURL(const tiny_string& _url) { loaderURL=_url; }
	void setParameters(_NR<ASObject> p) { parameters = p; }
	void resetState();
	void setFrameRate(number_t f) { frameRate=f; }
	void setComplete();
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
	// There can be multiple jobs, one active and aborted ones
	// that have not yet terminated
	std::list<IThreadJob *> jobs;
	URLInfo url;
	_NR<LoaderInfo> contentLoaderInfo;
	void unload();
	bool loaded;
	bool allowCodeImport;
protected:
	_NR<DisplayObject> avm1target;
	void loadIntern(URLRequest* r, LoaderContext* context);
public:
	Loader(Class_base* c);
	~Loader();
	void finalize();
	void threadFinished(IThreadJob* job);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(close);
	ASFUNCTION_ATOM(load);
	ASFUNCTION_ATOM(loadBytes);
	ASFUNCTION_ATOM(_unload);
	ASFUNCTION_ATOM(_unloadAndStop);
	ASFUNCTION_ATOM(_getContentLoaderInfo);
	ASFUNCTION_ATOM(_getContent);
	ASPROPERTY_GETTER(_NR<UncaughtErrorEvents>,uncaughtErrorEvents);
	int getDepth() const
	{
		return 0;
	}
	void setContent(_R<DisplayObject> o);
	_NR<DisplayObject> getContent() { return content; }
	_R<LoaderInfo> getContentLoaderInfo() { return contentLoaderInfo; }
	bool allowLoadingSWF() { return allowCodeImport; }
};

class Sprite: public DisplayObjectContainer, public TokenContainer
{
friend class DisplayObject;
private:
	_NR<Graphics> graphics;
	//hitTarget is non-null if another Sprite has registered this
	//Sprite as its hitArea. Hits will be relayed to hitTarget.
	_NR<Sprite> hitTarget;
	_NR<SoundChannel> sound;
protected:
	bool boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const override;
	bool renderImpl(RenderContext& ctxt) const override;
	_NR<DisplayObject> hitTestImpl(_NR<DisplayObject> last, number_t x, number_t y, DisplayObject::HIT_TYPE type,bool interactiveObjectsOnly) override;
	void resetToStart() override;
	void checkSound();// start sound streaming if it is not already playing
	void markSoundFinished();
public:
	bool dragged;
	Sprite(Class_base* c);
	void setSound(SoundChannel* s);
	void appendSound(unsigned char* buf, int len);
	bool destruct() override;
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(_getGraphics);
	ASFUNCTION_ATOM(_startDrag);
	ASFUNCTION_ATOM(_stopDrag);
	ASPROPERTY_GETTER_SETTER(bool, buttonMode);
	ASPROPERTY_GETTER_SETTER(_NR<Sprite>, hitArea);
	ASPROPERTY_GETTER_SETTER(bool, useHandCursor);
	ASFUNCTION_ATOM(getSoundTransform);
	ASFUNCTION_ATOM(setSoundTransform);
	int getDepth() const
	{
		return 0;
	}
	IDrawable* invalidate(DisplayObject* target, const MATRIX& initialMatrix,bool smoothing) override
	{ return TokenContainer::invalidate(target, initialMatrix,smoothing); }
	void requestInvalidation(InvalidateQueue* q) override;
	_NR<Graphics> getGraphics();
};

struct FrameLabel_data
{
	FrameLabel_data() : frame(0) {}
	FrameLabel_data(uint32_t _frame, tiny_string _name) : name(_name),frame(_frame){}
	tiny_string name;
	uint32_t frame;
};

class FrameLabel: public ASObject, public FrameLabel_data
{
public:
	FrameLabel(Class_base* c);
	FrameLabel(Class_base* c, const FrameLabel_data& data);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	ASFUNCTION_ATOM(_getFrame);
	ASFUNCTION_ATOM(_getName);
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
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(_getLabels);
	ASFUNCTION_ATOM(_getName);
	ASFUNCTION_ATOM(_getNumFrames);
};

class Frame
{
friend class FrameContainer;
private:
	std::list<AVM1ActionTag*> avm1actions;
	std::list<AVM1InitActionTag*> avm1initactions;
	AVM1context avm1context;
public:
	inline AVM1context* getAVM1Context() { return &avm1context; }
	std::list<DisplayListTag*> blueprint;
	void execute(DisplayObjectContainer* displayList);
	void AVM1executeActions(MovieClip* clip, bool avm1initactionsdone);
	/**
	 * destroyTags must be called only by the tag destructor, not by
	 * the objects that are instance of tags
	 */
	void destroyTags();
};

class FrameContainer
{
protected:
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
	std::list<Frame> frames;
	std::vector<Scene_data> scenes;
	void addToFrame(DisplayListTag *r);
	void addAvm1ActionToFrame(AVM1ActionTag* t);
	void addAvm1InitActionToFrame(AVM1InitActionTag* t);
	void setFramesLoaded(uint32_t fl) { framesLoaded = fl; }
	FrameContainer();
	FrameContainer(const FrameContainer& f);
private:
	//No need for any lock, just make sure accesses are atomic
	ATOMIC_INT32(framesLoaded);
public:
	void addFrameLabel(uint32_t frame, const tiny_string& label);
	uint32_t getFramesLoaded() { return framesLoaded; }
};

class MovieClip: public Sprite, public FrameContainer
{
private:
	uint32_t getCurrentScene() const;
	const Scene_data *getScene(const tiny_string &sceneName) const;
	uint32_t getFrameIdByNumber(uint32_t i, const tiny_string& sceneName) const;
	uint32_t getFrameIdByLabel(const tiny_string& l, const tiny_string& sceneName) const;
	std::map<uint32_t,asAtom > frameScripts;
	uint32_t fromDefineSpriteTag;
	uint32_t frameScriptToExecute;
	bool avm1initactionsdone;

	std::set<uint32_t> frameinitactionsdone;

	CLIPACTIONS actions;
	std::list<Frame>::iterator currentframeIterator;
protected:
	/* This is read from the SWF header. It's only purpose is for flash.display.MovieClip.totalFrames */
	uint32_t totalFrames_unreliable;
	ASPROPERTY_GETTER_SETTER(bool, enabled);
public:
	void constructionComplete() override;
	void afterConstruction() override;
	void resetLegacyState() override;
	RunState state;
	Frame* getCurrentFrame();
	MovieClip(Class_base* c);
	MovieClip(Class_base* c, const FrameContainer& f, uint32_t defineSpriteTagID);
	bool destruct() override;
	void gotoAnd(asAtom *args, const unsigned int argslen, bool stop);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	/*
	 * returns true if all frames of this MovieClip are loaded
	 * this is overwritten in RootMovieClip, because that one is
	 * executed while loading
	 */
	virtual bool hasFinishedLoading() { return true; }
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(swapDepths);
	ASFUNCTION_ATOM(addFrameScript);
	ASFUNCTION_ATOM(stop);
	ASFUNCTION_ATOM(play);
	ASFUNCTION_ATOM(gotoAndStop);
	ASFUNCTION_ATOM(gotoAndPlay);
	ASFUNCTION_ATOM(prevFrame);
	ASFUNCTION_ATOM(nextFrame);
	ASFUNCTION_ATOM(_getCurrentFrame);
	ASFUNCTION_ATOM(_getCurrentFrameLabel);
	ASFUNCTION_ATOM(_getCurrentLabel);
	ASFUNCTION_ATOM(_getCurrentLabels);
	ASFUNCTION_ATOM(_getTotalFrames);
	ASFUNCTION_ATOM(_getFramesLoaded);
	ASFUNCTION_ATOM(_getScenes);
	ASFUNCTION_ATOM(_getCurrentScene);

	void advanceFrame() override;
	void declareFrame() override;
	void initFrame() override;
	void executeFrameScript() override;

	void afterLegacyInsert() override;
	void afterLegacyDelete(DisplayObjectContainer* par) override;

	void addScene(uint32_t sceneNo, uint32_t startframe, const tiny_string& name);
	uint32_t getTagID() const override { return fromDefineSpriteTag; }
	void setupActions(const CLIPACTIONS& clipactions);

	bool AVM1HandleKeyboardEvent(KeyboardEvent *e) override;
	bool AVM1HandleMouseEvent(EventDispatcher* dispatcher, MouseEvent *e) override;
	void AVM1HandleEvent(EventDispatcher* dispatcher, Event* e) override;
	
	void AVM1gotoFrameLabel(const tiny_string &label);
	void AVM1gotoFrame(int frame, bool stop, bool switchplaystate=false);
	static void AVM1SetupMethods(Class_base* c);
	void AVM1ExecuteFrameActionsFromLabel(const tiny_string &label);
	void AVM1ExecuteFrameActions(uint32_t frame);
	ASFUNCTION_ATOM(AVM1AttachMovie);
	ASFUNCTION_ATOM(AVM1CreateEmptyMovieClip);
	ASFUNCTION_ATOM(AVM1RemoveMovieClip);
	ASFUNCTION_ATOM(AVM1Clear);
	ASFUNCTION_ATOM(AVM1MoveTo);
	ASFUNCTION_ATOM(AVM1LineTo);
	ASFUNCTION_ATOM(AVM1LineStyle);
	ASFUNCTION_ATOM(AVM1BeginFill);
	ASFUNCTION_ATOM(AVM1EndFill);
	ASFUNCTION_ATOM(AVM1GetNextHighestDepth);
	ASFUNCTION_ATOM(AVM1AttachBitmap);
	ASFUNCTION_ATOM(AVM1getInstanceAtDepth);
	ASFUNCTION_ATOM(AVM1getSWFVersion);
};

class Stage: public DisplayObjectContainer
{
public:
	uint32_t internalGetHeight() const;
	uint32_t internalGetWidth() const;
private:
	void onAlign(const tiny_string&);
	void onColorCorrection(const tiny_string&);
	void onFullScreenSourceRect(_NR<Rectangle>);
	// Keyboard focus object is accessed from the VM thread (AS
	// code) and the input thread and is protected focusSpinlock
	Spinlock focusSpinlock;
	_NR<InteractiveObject> focus;
	_NR<RootMovieClip> root;
	// list of objects that are not added to stage, but need to be handled when first frame is executed
	// currently only used when Loader contents are added and the Loader is not on stage
	list<_R<DisplayObject>> hiddenobjects;
	vector<_R<ASObject>> avm1KeyboardListeners;
	vector<_R<ASObject>> avm1MouseListeners;
	vector<_R<ASObject>> avm1EventListeners;
protected:
	virtual void eventListenerAdded(const tiny_string& eventName) override;
	bool renderImpl(RenderContext& ctxt) const override;
public:
	void onDisplayState(const tiny_string&);
	_NR<DisplayObject> hitTestImpl(_NR<DisplayObject> last, number_t x, number_t y, DisplayObject::HIT_TYPE type,bool interactiveObjectsOnly) override;
	void setOnStage(bool staged, bool force = false) override { assert(false); /* we are the stage */}
	_NR<RootMovieClip> getRoot() override;
	void setRoot(_NR<RootMovieClip> _root);
	Stage(Class_base* c);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	_NR<Stage> getStage() override;
	_NR<InteractiveObject> getFocusTarget();
	void setFocusTarget(_NR<InteractiveObject> focus);
	void addHiddenObject(_R<DisplayObject> o) { hiddenobjects.push_back(o);}
	void initFrame() override;
	void executeFrameScript() override;
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(_getAllowFullScreen);
	ASFUNCTION_ATOM(_getAllowFullScreenInteractive);
	ASFUNCTION_ATOM(_getColorCorrectionSupport);
	ASFUNCTION_ATOM(_getStageWidth);
	ASFUNCTION_ATOM(_setStageWidth);
	ASFUNCTION_ATOM(_getStageHeight);
	ASFUNCTION_ATOM(_setStageHeight);
	ASFUNCTION_ATOM(_getScaleMode);
	ASFUNCTION_ATOM(_setScaleMode);
	ASFUNCTION_ATOM(_getLoaderInfo);
	ASFUNCTION_ATOM(_getStageVideos);
	ASFUNCTION_ATOM(_getFocus);
	ASFUNCTION_ATOM(_setFocus);
	ASFUNCTION_ATOM(_setTabChildren);
	ASFUNCTION_ATOM(_getFrameRate);
	ASFUNCTION_ATOM(_setFrameRate);
	ASFUNCTION_ATOM(_getWmodeGPU);
	ASFUNCTION_ATOM(_invalidate);
	ASFUNCTION_ATOM(_getColor);
	ASFUNCTION_ATOM(_setColor);
	ASPROPERTY_GETTER_SETTER(tiny_string,align);
	ASPROPERTY_GETTER_SETTER(tiny_string,colorCorrection);
	ASPROPERTY_GETTER_SETTER(tiny_string,displayState);
	ASPROPERTY_GETTER_SETTER(_NR<Rectangle>,fullScreenSourceRect);
	ASPROPERTY_GETTER_SETTER(bool,showDefaultContextMenu);
	ASPROPERTY_GETTER_SETTER(tiny_string,quality);
	ASPROPERTY_GETTER_SETTER(bool,stageFocusRect);
	ASPROPERTY_GETTER(bool,allowsFullScreen);
	ASPROPERTY_GETTER(_NR<Vector>, stage3Ds);
	
	void AVM1HandleEvent(EventDispatcher *dispatcher, Event* e) override;
	void AVM1AddKeyboardListener(ASObject* o);
	void AVM1RemoveKeyboardListener(ASObject *o);
	void AVM1AddMouseListener(ASObject *o);
	void AVM1RemoveMouseListener(ASObject* o);
	void AVM1AddEventListener(ASObject *o);
	void AVM1RemoveEventListener(ASObject *o);
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

class Stage3D: public EventDispatcher
{
friend class Stage;
protected:
	bool renderImpl(RenderContext &ctxt) const;
public:
	Stage3D(Class_base* c):EventDispatcher(c),x(0),y(0),visible(true){ subtype = SUBTYPE_STAGE3D; }
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(requestContext3D);
	ASFUNCTION_ATOM(requestContext3DMatchingProfiles);
	ASPROPERTY_GETTER_SETTER(number_t,x);
	ASPROPERTY_GETTER_SETTER(number_t,y);
	ASPROPERTY_GETTER_SETTER(bool,visible);
	ASPROPERTY_GETTER(_NR<Context3D>,context3D);
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

class GraphicsPathCommand: public ASObject
{
public:
	enum {NO_OP=0, MOVE_TO, LINE_TO, CURVE_TO, WIDE_MOVE_TO, WIDE_LINE_TO, CUBIC_CURVE_TO};
	GraphicsPathCommand(Class_base* c):ASObject(c){}
	static void sinit(Class_base* c);
};

class GraphicsPathWinding: public ASObject
{
public:
	GraphicsPathWinding(Class_base* c):ASObject(c){}
	static void sinit(Class_base* c);

};

class IntSize
{
public:
	uint32_t width;
	uint32_t height;
	IntSize(uint32_t w, uint32_t h):width(h),height(h){}
};

class PixelSnapping: public ASObject
{
public:
	PixelSnapping(Class_base* c):ASObject(c){}
	static void sinit(Class_base* c);
};

class CapsStyle: public ASObject
{
public:
	CapsStyle(Class_base* c):ASObject(c){}
	static void sinit(Class_base* c);
};
class JointStyle: public ASObject
{
public:
	JointStyle(Class_base* c):ASObject(c){}
	static void sinit(Class_base* c);
};

class Bitmap: public DisplayObject, public TokenContainer
{
friend class CairoTokenRenderer;
private:
	void onBitmapData(_NR<BitmapData>);
	void onSmoothingChanged(bool);
	void onPixelSnappingChanged(tiny_string snapping);
protected:
	bool renderImpl(RenderContext& ctxt) const override
		{ return TokenContainer::renderImpl(ctxt); }
public:
	ASPROPERTY_GETTER_SETTER(_NR<BitmapData>,bitmapData);
	ASPROPERTY_GETTER_SETTER(bool, smoothing);
	ASPROPERTY_GETTER_SETTER(tiny_string,pixelSnapping);
	/* Call this after updating any member of 'data' */
	void updatedData();
	Bitmap(Class_base* c, _NR<LoaderInfo> li=NullRef, std::istream *s = NULL, FILE_TYPE type=FT_UNKNOWN);
	Bitmap(Class_base* c, _R<BitmapData> data);
	~Bitmap();
	bool destruct() override;
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(_constructor);
	bool boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const override;
	_NR<DisplayObject> hitTestImpl(_NR<DisplayObject> last, number_t x, number_t y, DisplayObject::HIT_TYPE type,bool interactiveObjectsOnly) override;
	virtual IntSize getBitmapSize() const;
	void requestInvalidation(InvalidateQueue* q) override { TokenContainer::requestInvalidation(q); }
	IDrawable* invalidate(DisplayObject* target, const MATRIX& initialMatrix,bool smoothing) override
	{ return TokenContainer::invalidate(target, initialMatrix,smoothing); }
};

class AVM1Movie: public DisplayObject
{
public:
	AVM1Movie(Class_base* c):DisplayObject(c){}
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	ASFUNCTION_ATOM(_constructor);
};

class Shader : public ASObject
{
public:
	Shader(Class_base* c):ASObject(c){}
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(_constructor);
};

class BitmapDataChannel : public ASObject
{
public:
	enum {RED=1, GREEN=2, BLUE=4, ALPHA=8};
	BitmapDataChannel(Class_base* c):ASObject(c){}
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(_constructor);
	static unsigned int channelShift(uint32_t channelConstant);
};

class ActionScriptVersion: public ASObject
{
public:
	ActionScriptVersion(Class_base* c):ASObject(c){}
	static void sinit(Class_base* c);
};

}

#endif /* SCRIPTING_FLASH_DISPLAY_FLASHDISPLAY_H */
