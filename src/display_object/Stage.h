/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2012-2013  Alessandro Pignotti (a.pignotti@sssup.it)
    Copyright (C) 2023-2024, 2026  mr b0nk 500 (b0nk@b0nk.xyz)

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

#ifndef DISPLAY_OBJECT_STAGE_H
#define DISPLAY_OBJECT_STAGE_H 1

#include <cstddef>
#include <cstdint>
#include <vector>

#include "backends/graphics.h"
#include "display_object/DisplayObjectContainer.h"
#include "display_object/InteractiveObject.h"
#include "gc/ptr.h"
#include "smartrefs.h"
#include "swftypes.h"
#include "utils/optional.h"
#include "utils/span.h"

// Based on Ruffle's `display_object::Stage`.

namespace lightspark
{

class tiny_string;
class ASObject;
class AVM1Array;
class AVM1Object;
class AVM1Value;
class Event;
class LoaderInfo;
class NativeWindow;
class Stage3D;
class SWFMovie;
class RenderContext;
class RootMovieClip;

enum class StageDisplayState
{
	Normal,
	FullScreen,
	FullScreenInteractive,
};

enum class StageAlign : uint8_t
{
	Top = 1 << 0,
	Bottom = 1 << 1,
	Left = 1 << 2,
	Right = 1 << 3,
};

enum class StageScale
{
	ExactFit,
	NoBorder,
	NoScale,
	ShowAll,
};

enum class Quality
{
	Low,
	Medium,
	High,
	Best,
	High8x8,
	High8x8Linear,
	High16x16,
	High16x16Linear,
};

class Stage : public InteractiveObject, public DisplayObjectContainer
{
public:
	Vector2 getStageSize() const;
	uint32_t internalGetHeight() const { return getStageSize().y; }
	uint32_t internalGetWidth() const { return getStageSize().x; }
private:
	SWFMovie& movie;
	LoaderInfo* loaderInfo;

	Mutex avm1ListenerMutex;
	Mutex avm1DisplayObjectMutex;
	Mutex avm1ScriptMutex;
	// Keyboard focus object is accessed from the VM thread (AS
	// code) and the input thread and is protected focusSpinlock
	Mutex focusSpinlock;
	InteractiveObject* focus;
	InteractiveObject* avm1Focus;
	RootMovieClip* root;
	std::vector<AVM1Value> avm1KeyboardListeners;
	std::vector<AVM1Value> avm1MouseListeners;
	std::vector<std::pair<_GC<AVM1Object>, uint32_t>> avm1EventListeners;
	std::vector<_GC<AVM1Object>> avm1ResizeListeners;
	std::vector<AVM1Value> avm1FocusListeners;
	// double linked list of AVM1 MovieClips currently on Stage that have scripts to execute
	// this is needed to execute the scripts in the correct order
	DisplayObject* avm1DisplayObjectFirst;
	DisplayObject* avm1DisplayObjectLast;
	std::list<AVM1scriptToExecute> avm1ScriptsToExecute;
	bool hasAVM1Clips;
	void executeAVM1Scripts(bool implicit);
	Mutex DisplayObjectRemovedMutex;
	std::unordered_set<DisplayObject&> removedDisplayObjects;

	Optional<RGB> backgroundColor;
	StageScale scaleMode;
	ACQUIRE_RELEASE_FLAG(invalidated);
	StageAlign align;
	Optional<bool> colorCorrection;
	StageDisplayState displayState;
	RectF fullScreenSourceRect;
	bool showDefaultContextMenu;
	Quality quality;
	bool stageFocusRect;
	bool allowsFullScreen;
	std::vector<_R<Stage3D>> stage3Ds;
	RectF softKeyboardRect;
	number_t contentsScaleFactor;
	_NR<NativeWindow> nativeWindow;
public:
	Stage
	(
		SystemState* sys,
		SWFMovie& _movie,
		Optional<const tiny_string&> name = {}
	);

	void render(RenderContext& ctx, const MATRIX* startMatrix);
	void defaultEventBehavior(_R<Event> e) override {}
	void forceInvalidation();
	bool renderStage3D();
	void AVM1RootClipAdded() { hasAVM1Clips = true; }
	bool isFocusable(bool fromMouse) override { return false; }
	InteractiveObject& getFocusTarget();
	InteractiveObject* getAVM1FocusTarget();
	void setTabFocusTarget(bool next);
	bool setFocusTarget
	(
		InteractiveObject* obj,
		bool setFromMouse,
		bool preventAVM1Events = false
	);

	void checkResetFocusTarget(InteractiveObject& removedtarget);
	void addHiddenObject(DisplayObject& o);
	void removeHiddenObject(DisplayObject& o);
	void forEachHiddenObject
	(
		std::function<void(DisplayObject*)> callback,
		bool allowInvalid = false
	);

	void cleanupDeadHiddenObjects();
	void prepareForRemoval(DisplayObject& d);
	void cleanupRemovedDisplayObjects();
	void enterFrame(bool implicit) override;
	void advanceFrame(bool implicit) override;
	void initFrame() override;
	void executeFrameScript() override;
	const SWFMovie& getMovie() const override { return movie; }
	LoaderInfo* getLoaderInfo() const override { return loaderInfo; }
	void setLoaderInfo(LoaderInfo* info) { loaderInfo = info; }

	bool isFullscreen() const;

	Optional<const RGB&> getBackgroundColor() const
	{
		return backgroundColor.asRef();
	}

	void setBackgroundColor(const Optional<RGB>& color)
	{
		backgroundColor = color;
	}

	const StageScale& getScaleMode() const { return scaleMode; }
	void setScaleMode(const StageScale& mode);
	bool getInvalidated() const { return ACQUIRE_READ(invalidated); }
	void setInvalidated(bool flag)
	{
		RELEASE_WRITE(invalidated, flag);
	}

	const StageAlign& getAlign() const { return align; }
	void setAlign(const StageAlign& _align);
	Optional<bool> getColorCorrection() const
	{
		return colorCorrection;
	}

	void setColorCorrection(const Optional<bool> flag)
	{
		colorCorrection = flag;
	}

	const StageDisplayState& getDisplayState() const
	{
		return displayState;
	}

	void toggleDisplayState();
	void setDisplayState(const StageDisplayState& state);
	const RectF& getFullScreenSourceRect() const
	{
		return fullScreenSourceRect;
	}

	void setFullScreenSourceRect(const RectF& rect)
	{
		fullScreenSourceRect = rect;
	}

	bool getShowDefaultContextMenu() const
	{
		return showDefaultContextMenu;
	}

	void setShowDefaultContextMenu(bool flag)
	{
		showDefaultContextMenu = flag;
	}

	const Quality& getQuality() const { return quality; }
	void setQuality(const Quality& _quality);
	bool getStageFocusRect() const { return stageFocusRect; }
	void setStageFocusRect(bool flag) { stageFocusRect = flag; }
	bool getAllowsFullScreen() const { return allowsFullScreen; }
	void setAllowsFullScreen(bool flag) { allowsFullScreen = flag; }
	Span<_R<Stage3D>> getStage3Ds() const
	{
		return makeSpan(stage3Ds);
	}

	const RectF& getSoftKeyboardRect() const
	{
		return softKeyboardRect;
	}

	void setSoftKeyboardRect(const RectF& rect)
	{
		softKeyboardRect = rect;
	}

	number_t getContentsScaleFactor() const
	{
		return contentsScaleFactor;
	}

	void setContentsScaleFactor(number_t scale)
	{
		contentsScaleFactor = scale;
	}

	_NR<NativeWindow> getNativeWindow() const { return nativeWindow; }
	void setNativeWindow(_NR<NativeWindow> window)
	{
		nativeWindow = window;
	}
	
	void AVM1HandleEvent(EventDispatcher *dispatcher, Event* e) override;
	bool AVM1AddKeyboardListener(asAtom listener);
	bool AVM1RemoveKeyboardListener(asAtom listener);
	void AVM1GetKeyboardListeners(AVM1Array* res);
	bool AVM1AddMouseListener(asAtom listener);
	bool AVM1RemoveMouseListener(asAtom listener);
	void AVM1GetMouseListeners(AVM1Array* res);
	void AVM1AddEventListener(ASObject *o);
	void AVM1RemoveEventListener(ASObject *o);
	void AVM1AddResizeListener(ASObject *o);
	bool AVM1RemoveResizeListener(ASObject *o);
	void AVM1AddFocusListener(asAtom listener);
	bool AVM1RemoveFocusListener(asAtom listener);
	void AVM1AddDisplayObject(DisplayObject* dobj);
	void AVM1RemoveDisplayObject(DisplayObject* dobj);
	void AVM1AddScriptToExecute(AVM1scriptToExecute& script);
	void AVM1SetLevelRoot(int level, RootMovieClip* root);
	void AVM1removeLevelRoot(int level);
	RootMovieClip* AVM1getLevelRoot(int level);
	void AVM1RemoveAllListeners();
};

}
#endif /* DISPLAY_OBJECT_STAGE_H */
