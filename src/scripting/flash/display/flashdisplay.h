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

#include "compat.h"

#include "forwards/threading.h"
#include "forwards/backends/netutils.h"
#include "interfaces/backends/netutils.h"
#include "swftypes.h"
#include "scripting/flash/events/flashevents.h"
#include "backends/netutils.h"
#include "backends/graphics.h"
#include "scripting/flash/display/DisplayObject.h"
#include "scripting/flash/display/Graphics.h"
#include "scripting/flash/display/TokenContainer.h"
#include "scripting/flash/display/NativeWindow.h"
#include "abcutils.h"
#include <unordered_set>

namespace lightspark
{

class RootMovieClip;
class DisplayListTag;
class DefineShapeTag;
class AVM1ActionTag;
class AVM1InitActionTag;
class DefineButtonTag;
class InteractiveObject;
class Downloader;
class RenderContext;
class SoundChannel;
class SoundTransform;
class ApplicationDomain;
class SecurityDomain;
class BitmapData;
class Matrix;
class Vector;
class Graphics;
class Rectangle;
class Class_inherit;
class Point;
class NativeMenuItem;
class AVM1MovieClipLoader;
class Context3D;
class AccessibilityImplementation;

class InteractiveObject: public DisplayObject
{
protected:
	bool mouseEnabled;
	bool doubleClickEnabled;
	void setOnStage(bool staged, bool force, bool inskipping=false) override;
	void onContextMenu(_NR<ASObject> oldValue);
	~InteractiveObject();
public:
	bool isHittable(DisplayObject::HIT_TYPE type)
	{
		if(type == DisplayObject::MOUSE_CLICK_HIT)
			return mouseEnabled;
		else if(type == DisplayObject::DOUBLE_CLICK_HIT)
			return doubleClickEnabled && mouseEnabled;
		else
			return true;
	}
	InteractiveObject(ASWorker* wrk,Class_base* c);
	ASPROPERTY_GETTER_SETTER(_NR<AccessibilityImplementation>,accessibilityImplementation);
	ASPROPERTY_GETTER_SETTER(_NR<ASObject>,contextMenu); // adobe seems to allow DisplayObjects, too
	ASPROPERTY_GETTER_SETTER(bool,tabEnabled);
	ASPROPERTY_GETTER_SETTER(int32_t,tabIndex);
	ASPROPERTY_GETTER_SETTER(_NR<ASObject>,focusRect);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(_setMouseEnabled);
	ASFUNCTION_ATOM(_getMouseEnabled);
	ASFUNCTION_ATOM(_setDoubleClickEnabled);
	ASFUNCTION_ATOM(_getDoubleClickEnabled);
	bool destruct() override;
	void finalize() override;
	void prepareShutdown() override;
	bool countCylicMemberReferences(garbagecollectorstate& gcstate) override;
	static void sinit(Class_base* c);
	virtual void lostFocus() {}
	virtual void gotFocus() {} 
	virtual void textInputChanged(const tiny_string& newtext) {} 
	void setMouseEnabled(bool enabled) { mouseEnabled = enabled; }
	void defaultEventBehavior(_R<Event> e) override;
	// returns the owner of the contextmenu
	_NR<InteractiveObject> getCurrentContextMenuItems(std::vector<_R<NativeMenuItem> > &items);
};

class DisplayObjectContainer: public InteractiveObject
{
private:
	bool mouseChildren;
	map<int32_t,DisplayObject*> mapDepthToLegacyChild;
	unordered_map<DisplayObject*,int32_t> mapLegacyChildToDepth;
	set<int32_t> legacyChildrenMarkedForDeletion;
	map<int32_t,DisplayObject*> mapFrameDepthToLegacyChildRemembered;
	bool _contains(DisplayObject* child);
	void getObjectsFromPoint(Point* point, Array* ar);
	number_t boundsrectXmin;
	number_t boundsrectYmin;
	number_t boundsrectXmax;
	number_t boundsrectYmax;
	bool boundsRectDirty;
	number_t boundsrectVisibleXmin;
	number_t boundsrectVisibleYmin;
	number_t boundsrectVisibleXmax;
	number_t boundsrectVisibleYmax;
	bool boundsRectVisibleDirty;
	void umarkLegacyChild(DisplayObject* child);
protected:
	std::vector < DisplayObject* > dynamicDisplayList;
	void clearDisplayList();
	//The lock should only be taken when doing write operations
	//As the RenderThread only reads, it's safe to read without the lock
	mutable Mutex mutexDisplayList;
	void setOnStage(bool staged, bool force, bool inskipping=false) override;
	_NR<DisplayObject> hitTestImpl(const Vector2f& globalPoint, const Vector2f& localPoint, DisplayObject::HIT_TYPE type,bool interactiveObjectsOnly) override;
	bool boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax, bool visibleOnly) override;
	virtual void resetToStart() {}
	ASPROPERTY_GETTER_SETTER(bool, tabChildren);
	void LegacyChildEraseDeletionMarked();
	void rememberLastFrameChildren();
	void clearLastFrameChildren();
public:
	bool boundsRectWithoutChildren(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax, bool visibleOnly) override
	{
		return false;
	}
	DisplayObject* getLastFrameChildAtDepth(int depth);
	void fillGraphicsData(Vector* v, bool recursive) override;
	bool LegacyChildRemoveDeletionMark(int32_t depth);
	void requestInvalidation(InvalidateQueue* q, bool forceTextureRefresh=false) override;
	void requestInvalidationIncludingChildren(InvalidateQueue* q) override;
	IDrawable* invalidate(bool smoothing) override;
	void invalidateForRenderToBitmap(RenderDisplayObjectToBitmapContainer* container) override;
	
	void _addChildAt(DisplayObject* child, unsigned int index, bool inskipping=false);
	void dumpDisplayList(unsigned int level=0);
	void handleRemovedEvent(DisplayObject* child, bool keepOnStage = false, bool inskipping = false);
	bool _removeChild(DisplayObject* child, bool direct=false, bool inskipping=false, bool keeponstage=false);
	void _removeAllChildren();
	void removeAVM1Listeners() override;
	int getChildIndex(DisplayObject* child);
	DisplayObjectContainer(ASWorker* wrk,Class_base* c);
	void markAsChanged() override;
	inline void markBoundsRectDirty() { boundsRectDirty=true; boundsRectVisibleDirty=true; }
	void markBoundsRectDirtyChildren();
	bool destruct() override;
	void finalize() override;
	void prepareShutdown() override;
	bool countCylicMemberReferences(garbagecollectorstate& gcstate) override;
	void cloneDisplayList(std::vector<_R<DisplayObject>>& displayListCopy);
	bool isEmpty() const { return dynamicDisplayList.empty(); }
	bool hasLegacyChildAt(int32_t depth);
	// this does not test if a DisplayObject exists at the provided depth
	DisplayObject* getLegacyChildAt(int32_t depth);
	void setupClipActionsAt(int32_t depth, const CLIPACTIONS& actions);
	void checkRatioForLegacyChildAt(int32_t depth, uint32_t ratio, bool inskipping);
	void checkColorTransformForLegacyChildAt(int32_t depth, const CXFORMWITHALPHA& colortransform);
	void deleteLegacyChildAt(int32_t depth, bool inskipping);
	void insertLegacyChildAt(int32_t depth, DisplayObject* obj, bool inskipping=false, bool fromtag=true);
	DisplayObject* findLegacyChildByTagID(uint32_t tagid);
	int findLegacyChildDepth(DisplayObject* obj);
	void transformLegacyChildAt(int32_t depth, const MATRIX& mat);
	uint32_t getMaxLegacyChildDepth();
	void purgeLegacyChildren();
	void checkClipDepth();
	void enterFrame(bool implicit) override;
	void advanceFrame(bool implicit) override;
	void declareFrame(bool implicit) override;
	void initFrame() override;
	void executeFrameScript() override;
	void afterLegacyInsert() override;
	void afterLegacyDelete(bool inskipping) override;
	multiname* setVariableByMultiname(multiname& name, asAtom& o, CONST_ALLOWED_FLAG allowConst, bool* alreadyset, ASWorker* wrk) override;
	bool deleteVariableByMultiname(const multiname& name, ASWorker* wrk) override;
	inline bool deleteVariableByMultinameWithoutRemovingChild(const multiname& name, ASWorker* wrk)
	{
		return InteractiveObject::deleteVariableByMultiname(name,wrk);
	}
	
	static void sinit(Class_base* c);
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

class Sprite: public DisplayObjectContainer, public TokenContainer
{
friend class DisplayObject;
private:
	_NR<Graphics> graphics;
	//hitTarget is non-null if another Sprite has registered this
	//Sprite as its hitArea. Hits will be relayed to hitTarget.
	_NR<Sprite> hitTarget;
	_NR<SoundChannel> sound;
	_NR<SoundTransform> soundtransform;
	uint32_t soundstartframe;
	bool streamingsound;
	bool hasMouse;
	void afterSetUseHandCursor(bool oldValue);
protected:
	bool boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax, bool visibleOnly) override;
	_NR<DisplayObject> hitTestImpl(const Vector2f& globalPoint, const Vector2f& localPoint, DisplayObject::HIT_TYPE type,bool interactiveObjectsOnly) override;
	void resetToStart() override;
	void checkSound(uint32_t frame);// start sound streaming if it is not already playing
	void stopSound();
	void markSoundFinished();
public:
	bool boundsRectWithoutChildren(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax, bool visibleOnly) override
	{
		if (visibleOnly && !this->isVisible())
			return false;
		return TokenContainer::boundsRect(xmin,xmax,ymin,ymax);
	}
	void fillGraphicsData(Vector* v, bool recursive) override;
	bool dragged;
	Sprite(ASWorker* wrk,Class_base* c);
	void setSound(SoundChannel* s, bool forstreaming);
	SoundChannel* getSoundChannel() const;
	void appendSound(unsigned char* buf, int len, uint32_t frame);
	void setSoundStartFrame(uint32_t frame) { soundstartframe=frame; }
	bool destruct() override;
	void finalize() override;
	void prepareShutdown() override;
	void startDrawJob() override;
	void endDrawJob() override;
	static void sinit(Class_base* c);
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
	IDrawable* invalidate(bool smoothing) override;
	void requestInvalidation(InvalidateQueue* q, bool forceTextureRefresh=false) override;
	Graphics* getGraphics();
	void handleMouseCursor(bool rollover) override;
	bool allowAsMask() const override { return !isEmpty() || !graphics.isNull(); }
	float getScaleFactor() const override { return this->scaling; }
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
	FrameLabel(ASWorker* wrk,Class_base* c);
	FrameLabel(ASWorker* wrk,Class_base* c, const FrameLabel_data& data);
	static void sinit(Class_base* c);
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
	Scene(ASWorker* wrk,Class_base* c);
	Scene(ASWorker* wrk,Class_base* c, const Scene_data& data, uint32_t _numFrames);
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
	AVM1context avm1context;
public:
	inline AVM1context* getAVM1Context() { return &avm1context; }
	std::list<DisplayListTag*> blueprint;
	void execute(DisplayObjectContainer* displayList, bool inskipping, std::vector<_R<DisplayObject>>& removedFrameScripts);
	void AVM1executeActions(MovieClip* clip);
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
	void setFramesLoaded(uint32_t fl) { framesLoaded = fl; }
	FrameContainer();
	FrameContainer(const FrameContainer& f);
private:
	//No need for any lock, just make sure accesses are atomic
	ATOMIC_INT32(framesLoaded);
	AVM1context avm1context;
public:
	void addFrameLabel(uint32_t frame, const tiny_string& label);
	uint32_t getFramesLoaded() { return framesLoaded; }
	void setAvm1InitAction(AVM1InitActionTag* t);
	inline AVM1context* getAVM1Context() { return &avm1context; }
};

class MovieClip: public Sprite, public FrameContainer
{
friend class Stage;
private:
	uint32_t getCurrentScene() const;
	const Scene_data *getScene(const tiny_string &sceneName) const;
	uint32_t getFrameIdByNumber(uint32_t i, const tiny_string& sceneName) const;
	std::map<uint32_t,asAtom > frameScripts;
	std::vector<_R<DisplayObject>> removedFrameScripts;
	uint32_t fromDefineSpriteTag;
	uint32_t lastFrameScriptExecuted;
	uint32_t lastratio;
	bool initializingFrame;
	bool inExecuteFramescript;
	bool inAVM1Attachment;
	bool isAVM1Loaded;
	bool AVM1EventScriptsAdded;
	void runGoto(bool newFrame);
protected:
	const CLIPACTIONS* actions;
	/* This is read from the SWF header. It's only purpose is for flash.display.MovieClip.totalFrames */
	uint32_t totalFrames_unreliable;
	ASPROPERTY_GETTER_SETTER(bool, enabled);
public:
	uint32_t getFrameIdByLabel(const tiny_string& l, const tiny_string& sceneName) const;
	void constructionComplete(bool _explicit = false) override;
	void beforeConstruction(bool _explicit = false) override;
	void afterConstruction(bool _explicit = false) override;
	RunState state;
	_NR<AVM1MovieClipLoader> avm1loader;
	Frame* getCurrentFrame();
	MovieClip(ASWorker* wrk,Class_base* c);
	MovieClip(ASWorker* wrk,Class_base* c, const FrameContainer& f, uint32_t defineSpriteTagID);
	bool destruct() override;
	void finalize() override;
	void prepareShutdown() override;
	bool countCylicMemberReferences(garbagecollectorstate& gcstate) override;
	void setPlaying();
	void setStopped();
	void gotoAnd(asAtom *args, const unsigned int argslen, bool stop);
	static void sinit(Class_base* c);
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

	void enterFrame(bool implicit) override;
	void advanceFrame(bool implicit) override;
	void declareFrame(bool implicit) override;
	void initFrame() override;
	void executeFrameScript() override;
	void checkRatio(uint32_t ratio, bool inskipping) override;

	void afterLegacyInsert() override;
	void afterLegacyDelete(bool inskipping) override;

	void addScene(uint32_t sceneNo, uint32_t startframe, const tiny_string& name);
	uint32_t getTagID() const override { return fromDefineSpriteTag; }
	void setupActions(const CLIPACTIONS& clipactions);

	bool AVM1HandleKeyboardEvent(KeyboardEvent *e) override;
	bool AVM1HandleMouseEvent(EventDispatcher* dispatcher, MouseEvent *e) override;
	void AVM1HandleEvent(EventDispatcher* dispatcher, Event* e) override;
	void AVM1AfterAdvance() override;
	
	void AVM1gotoFrameLabel(const tiny_string &label, bool stop, bool switchplaystate);
	void AVM1gotoFrame(int frame, bool stop, bool switchplaystate, bool advanceFrame=true);
	static void AVM1SetupMethods(Class_base* c);
	void AVM1ExecuteFrameActionsFromLabel(const tiny_string &label);
	void AVM1ExecuteFrameActions(uint32_t frame);
	void AVM1AddScriptEvents();
	void AVM1HandleConstruction();
	bool getAVM1Loaded() const { return isAVM1Loaded; }
	MovieClip* AVM1CloneSprite(asAtom target, uint32_t Depth, ASObject* initobj);

	ASFUNCTION_ATOM(AVM1AttachMovie);
	ASFUNCTION_ATOM(AVM1CreateEmptyMovieClip);
	ASFUNCTION_ATOM(AVM1RemoveMovieClip);
	ASFUNCTION_ATOM(AVM1DuplicateMovieClip);
	ASFUNCTION_ATOM(AVM1Clear);
	ASFUNCTION_ATOM(AVM1MoveTo);
	ASFUNCTION_ATOM(AVM1LineTo);
	ASFUNCTION_ATOM(AVM1LineStyle);
	ASFUNCTION_ATOM(AVM1BeginFill);
	ASFUNCTION_ATOM(AVM1BeginGradientFill);
	ASFUNCTION_ATOM(AVM1EndFill);
	ASFUNCTION_ATOM(AVM1GetNextHighestDepth);
	ASFUNCTION_ATOM(AVM1AttachBitmap);
	ASFUNCTION_ATOM(AVM1getInstanceAtDepth);
	ASFUNCTION_ATOM(AVM1getSWFVersion);
	ASFUNCTION_ATOM(AVM1LoadMovie);
	ASFUNCTION_ATOM(AVM1UnloadMovie);
	ASFUNCTION_ATOM(AVM1CreateTextField);
	
	std::string toDebugString() const override;
};

struct AVM1scriptToExecute
{
	const std::vector<uint8_t>* actions;
	uint32_t startactionpos;
	AVM1context* avm1context;
	uint32_t event_name_id;
	MovieClip* clip;
	bool isEventScript;
	void execute();
};
class Stage: public DisplayObjectContainer
{
public:
	uint32_t internalGetHeight() const;
	uint32_t internalGetWidth() const;
private:
	Mutex avm1listenerMutex;
	Mutex avm1DisplayObjectMutex;
	Mutex avm1ScriptMutex;
	void onColorCorrection(const tiny_string&);
	void onFullScreenSourceRect(_NR<Rectangle>);
	// Keyboard focus object is accessed from the VM thread (AS
	// code) and the input thread and is protected focusSpinlock
	Mutex focusSpinlock;
	_NR<InteractiveObject> focus;
	_NR<RootMovieClip> root;
	// list of objects that are not added to stage, but need to be handled when first frame is executed
	// currently used when Loader contents are added and the Loader is not on stage
	// or a MovieClip is not on stage but set to "play" from AS3 code
	unordered_set<DisplayObject*> hiddenobjects;
	vector<ASObject*> avm1KeyboardListeners;
	vector<ASObject*> avm1MouseListeners;
	vector<ASObject*> avm1EventListeners;
	vector<ASObject*> avm1ResizeListeners;
	// double linked list of AVM1 MovieClips currently on Stage that have scripts to execute
	// this is needed to execute the scripts in the correct order
	DisplayObject* avm1DisplayObjectFirst;
	DisplayObject* avm1DisplayObjectLast;
	std::list<AVM1scriptToExecute> avm1scriptstoexecute;
	bool hasAVM1Clips;
	void executeAVM1Scripts(bool implicit);
	Mutex DisplayObjectRemovedMutex;
	unordered_set<DisplayObject*> removedDisplayObjects;
protected:
	virtual void eventListenerAdded(const tiny_string& eventName) override;
public:
	void render(RenderContext& ctxt, const MATRIX* startmatrix);
	bool destruct() override;
	void prepareShutdown() override;
	void defaultEventBehavior(_R<Event> e) override;
	ACQUIRE_RELEASE_FLAG(invalidated);
	void onAlign(uint32_t);
	void forceInvalidation();
	bool renderStage3D();
	void onDisplayState(const tiny_string& old_value);
	void AVM1RootClipAdded() { hasAVM1Clips = true; }
	_NR<DisplayObject> hitTestImpl(const Vector2f& globalPoint, const Vector2f& localPoint, DisplayObject::HIT_TYPE type,bool interactiveObjectsOnly) override;
	void setOnStage(bool staged, bool force,bool inskipping=false) override { assert(false); /* we are the stage */}
	_NR<RootMovieClip> getRoot() override;
	void setRoot(_NR<RootMovieClip> _root);
	Stage(ASWorker* wrk,Class_base* c);
	static void sinit(Class_base* c);
	_NR<Stage> getStage() override;
	_NR<InteractiveObject> getFocusTarget();
	void setFocusTarget(_NR<InteractiveObject> focus);
	void checkResetFocusTarget(InteractiveObject* removedtarget);
	void addHiddenObject(DisplayObject* o);
	void removeHiddenObject(DisplayObject* o);
	void forEachHiddenObject(std::function<void(DisplayObject*)> callback, bool allowInvalid = false);
	void cleanupDeadHiddenObjects();
	void prepareForRemoval(DisplayObject* d);
	void cleanupRemovedDisplayObjects();
	void enterFrame(bool implicit) override;
	void advanceFrame(bool implicit) override;
	void initFrame() override;
	void executeFrameScript() override;
	void finalize() override;
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
	ASFUNCTION_ATOM(_isFocusInaccessible);
	ASPROPERTY_GETTER_SETTER(uint32_t,align);
	ASPROPERTY_GETTER_SETTER(tiny_string,colorCorrection);
	ASPROPERTY_GETTER_SETTER(tiny_string,displayState);
	ASPROPERTY_GETTER_SETTER(_NR<Rectangle>,fullScreenSourceRect);
	ASPROPERTY_GETTER_SETTER(bool,showDefaultContextMenu);
	ASPROPERTY_GETTER_SETTER(tiny_string,quality);
	ASPROPERTY_GETTER_SETTER(bool,stageFocusRect);
	ASPROPERTY_GETTER(bool,allowsFullScreen);
	ASPROPERTY_GETTER(_NR<Vector>, stage3Ds);
	ASPROPERTY_GETTER(_NR<Rectangle>, softKeyboardRect);
	ASPROPERTY_GETTER(number_t,contentsScaleFactor);
	ASPROPERTY_GETTER(_NR<NativeWindow>,nativeWindow);
	
	void AVM1HandleEvent(EventDispatcher *dispatcher, Event* e) override;
	void AVM1AddKeyboardListener(ASObject* o);
	void AVM1RemoveKeyboardListener(ASObject *o);
	void AVM1AddMouseListener(ASObject *o);
	void AVM1RemoveMouseListener(ASObject* o);
	void AVM1AddEventListener(ASObject *o);
	void AVM1RemoveEventListener(ASObject *o);
	void AVM1AddResizeListener(ASObject *o);
	bool AVM1RemoveResizeListener(ASObject *o);
	void AVM1AddDisplayObject(DisplayObject* dobj);
	void AVM1RemoveDisplayObject(DisplayObject* dobj);
	void AVM1AddScriptToExecute(AVM1scriptToExecute& script);
};

class StageScaleMode: public ASObject
{
public:
	StageScaleMode(ASWorker* wrk, Class_base* c):ASObject(wrk,c){}
	static void sinit(Class_base* c);
};

class StageAlign: public ASObject
{
public:
	StageAlign(ASWorker* wrk, Class_base* c):ASObject(wrk,c){}
	static void sinit(Class_base* c);
};

class StageQuality: public ASObject
{
public:
	StageQuality(ASWorker* wrk, Class_base* c):ASObject(wrk,c){}
	static void sinit(Class_base* c);
};

class StageDisplayState: public ASObject
{
public:
	StageDisplayState(ASWorker* wrk, Class_base* c):ASObject(wrk,c){}
	static void sinit(Class_base* c);
};


class LineScaleMode: public ASObject
{
public:
	LineScaleMode(ASWorker* wrk, Class_base* c):ASObject(wrk,c){}
	static void sinit(Class_base* c);
};

class BlendMode: public ASObject
{
public:
	BlendMode(ASWorker* wrk, Class_base* c):ASObject(wrk,c){}
	static void sinit(Class_base* c);
};

class GradientType: public ASObject
{
public:
	GradientType(ASWorker* wrk, Class_base* c):ASObject(wrk,c){}
	static void sinit(Class_base* c);
};

class InterpolationMethod: public ASObject
{
public:
	InterpolationMethod(ASWorker* wrk, Class_base* c):ASObject(wrk,c){}
	static void sinit(Class_base* c);
};

class SpreadMethod: public ASObject
{
public:
	SpreadMethod(ASWorker* wrk, Class_base* c):ASObject(wrk,c){}
	static void sinit(Class_base* c);
};

class GraphicsPathCommand: public ASObject
{
public:
	GraphicsPathCommand(ASWorker* wrk, Class_base* c):ASObject(wrk,c){}
	static void sinit(Class_base* c);
};

class GraphicsPathWinding: public ASObject
{
public:
	GraphicsPathWinding(ASWorker* wrk, Class_base* c):ASObject(wrk,c){}
	static void sinit(Class_base* c);

};

class PixelSnapping: public ASObject
{
public:
	PixelSnapping(ASWorker* wrk, Class_base* c):ASObject(wrk,c){}
	static void sinit(Class_base* c);
};

class CapsStyle: public ASObject
{
public:
	CapsStyle(ASWorker* wrk, Class_base* c):ASObject(wrk,c){}
	static void sinit(Class_base* c);
};
class JointStyle: public ASObject
{
public:
	JointStyle(ASWorker* wrk, Class_base* c):ASObject(wrk,c){}
	static void sinit(Class_base* c);
};

// AVM1Movie is derived from DisplayObjectContainer, but to ActionScript it behaves as a DisplayObject (similar to how we handle SimpleButtons)
// Its only child is the RootMovieClip of the loaded AVM1 swf file
class AVM1Movie: public DisplayObjectContainer
{
public:
	AVM1Movie(ASWorker* wrk, Class_base* c):DisplayObjectContainer(wrk,c){}
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(_constructor);
};

class Shader : public ASObject
{
public:
	Shader(ASWorker* wrk, Class_base* c):ASObject(wrk,c){}
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(_constructor);
};

class BitmapDataChannel : public ASObject
{
public:
	enum {RED=1, GREEN=2, BLUE=4, ALPHA=8};
	BitmapDataChannel(ASWorker* wrk, Class_base* c):ASObject(wrk,c){}
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(_constructor);
	static unsigned int channelShift(uint32_t channelConstant);
};

class ActionScriptVersion: public ASObject
{
public:
	ActionScriptVersion(ASWorker* wrk, Class_base* c):ASObject(wrk,c){}
	static void sinit(Class_base* c);
};

}

#endif /* SCRIPTING_FLASH_DISPLAY_FLASHDISPLAY_H */
