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

#ifndef SCRIPTING_FLASH_DISPLAY_STAGE_H
#define SCRIPTING_FLASH_DISPLAY_STAGE_H 1

#include "scripting/flash/display/flashdisplay.h"

namespace lightspark
{
class RootMovieClip;
class ASWorker;

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
	_NR<DisplayObject> hitTestImpl(const Vector2f& globalPoint, const Vector2f& localPoint, HIT_TYPE type,bool interactiveObjectsOnly) override;
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
	ASFUNCTION_ATOM(setAspectRatio);
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

}

#endif /* SCRIPTING_FLASH_DISPLAY_STAGE_H */
