/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2013  Alessandro Pignotti (a.pignotti@sssup.it)
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

#include <algorithm>
#include <list>

#include "asobject.h"
#include "backends/rendering_context.h"
#include "display_object/RootMovieClip.h"
#include "display_object/Stage.h"
#include "events.h"
#include "gc/context.h"
#include "scripting/avm1/value.h"
#include "scripting/avm1/object/object.h"
#include "scripting/flash/display/NativeWindow.h"
#include "scripting/flash/display/Stage3D.h"
#include "scripting/flash/display3d/flashdisplay3d.h"
#include "scripting/flash/events/flashevents.h"
#include "scripting/flash/ui/NativeMenuItem.h"
#include "tiny_string.h"

using namespace lightspark;

void Stage::onDisplayState(const tiny_string& old_value)
{
	if (old_value == displayState)
		return;
	tiny_string s = displayState.lowercase();

	// AVM1 allows case insensitive values, so we correct them here
	if (s=="normal")
		displayState="normal";
	if (s=="fullscreen")
		displayState="fullScreen";
	if (s=="fullscreeninteractive")
		displayState="fullScreenInteractive";

	if (displayState != "normal" && displayState != "fullScreen" && displayState != "fullScreenInteractive")
	{
		LOG(LOG_ERROR,"invalid value for DisplayState");
		return;
	}
	if (!getSystemState()->allowFullscreen && displayState == "fullScreen")
	{
		if (needsActionScript3())
			createError<SecurityError>(getInstanceWorker(),kInvalidParamError);
		return;
	}
	if (!getSystemState()->allowFullscreenInteractive && displayState == "fullScreenInteractive")
	{
		if (needsActionScript3())
			createError<SecurityError>(getInstanceWorker(),kInvalidParamError);
		return;
	}
	LOG(LOG_NOT_IMPLEMENTED,"setting display state does not check for the security sandbox!");
	getSystemState()->getEngineData()->setDisplayState(displayState,getSystemState());
}

void Stage::onFullScreenSourceRect(_NR<Rectangle> oldValue)
{
	if ((this->fullScreenSourceRect.isNull() && !oldValue.isNull()) ||
		(!this->fullScreenSourceRect.isNull() && oldValue.isNull()) ||
		(!this->fullScreenSourceRect.isNull() && !oldValue.isNull() &&
		 (this->fullScreenSourceRect->x != oldValue->x ||
		  this->fullScreenSourceRect->y != oldValue->y ||
		  this->fullScreenSourceRect->width != oldValue->width ||
		  this->fullScreenSourceRect->height != oldValue->height)))
		getSystemState()->getRenderThread()->requestResize(UINT32_MAX,UINT32_MAX,true);
	
}

void Stage::defaultEventBehavior(_R<Event> e)
{
}

bool Stage::renderStage3D()
{
	for (auto stage3D : stage3Ds)
	{
		auto ctx3D = stage3D->context3D;
		bool canRender =
		(
			!ctx3D.isNull() &&
			ctx3D->backBufferHeight &&
			ctx3D->backBufferWidth &&
			ctx3D->visible
		);

		if (canRender)
			return true;
	}
	return false;
}

void Stage::render(RenderContext& ctx, const MATRIX* startMatrix)
{
	bool has3d = false;
	for (auto stage3D : stage3Ds)
		has3d |= stage3D->renderImpl(ctx);

	if (!has3d)
	{
		getCachedSurface()->Render(getSys(), ctx, startMatrix);
		return;
	}

	auto glCtx = static_cast<GLRenderContext&>(ctx);
	auto glRenderThread = static_cast<RenderThread&>(glCtx);
	auto renderThread = getSys()->getRenderThread();
	auto engineData = getSys()->getEngineData();
	Vector2 windowSize
	(
		renderThread->windowWidth,
		renderThread->windowHeight
	);

	// setup opengl state for additional 2d rendering
	engineData->exec_glActiveTexture_GL_TEXTURE0(SAMPLEPOSITION::SAMPLEPOS_STANDARD);
	engineData->exec_glBlendFunc(BLEND_ONE, BLEND_ONE_MINUS_SRC_ALPHA);
	engineData->exec_glUseProgram(glRenderThread.gpu_program);
	engineData->exec_glViewport(0, 0, windowSize.x, windowSize.y);

	glCtx.lsglLoadIdentity();
	glCtx.setMatrixUniform(GLRenderContext::LSGL_MODELVIEW);

	getCachedSurface()->Render(getSys(), ctx, startMatrix);
}

void Stage::AVM1RemoveAllListeners()
{
	avm1FocusListeners.clear();
	avm1KeyboardListeners.clear();
	avm1MouseListeners.clear();
	avm1EventListeners.clear();
	avm1ResizeListeners.clear();
}

Stage::Stage
(
	SystemState* sys,
	SWFMovie& _movie,
	Optional<const tiny_string&> name
) :
InteractiveObject(Type::Stage, sys, name),
movie(_movie),
loaderInfo(nullptr),
focus(nullptr),
avm1Focus(nullptr),
root(nullptr),
avm1DisplayObjectFirst(nullptr),
avm1DisplayObjectLast(nullptr),
hasAVM1Clips(false),
scaleMode(StageScale::ShowAll),
invalidated(false),
align(0),
displayState(StageDisplayState::Normal),
showDefaultContextMenu(true),
quality(Quality::High),
stageFocusRect(false),
allowsFullScreen(false),
contentsScaleFactor(1)
{
	auto wrk = sys->getInstanceWorker();

	// according to specs, Desktop computers usually have 4 Stage3D objects available
	stage3Ds.reserve(4);

	for (size_t i = 0; i < 4; ++i)
	{
		auto stage3D = Class<Stage3D>::getInstanceS(wrk);
		stage3D->setRefConstant();
		stage3Ds.emplace_back(stage3D);
	}

	if (sys->flashMode != SystemState::AIR)
		return;

	nativeWindow = _MR(Class<NativeWindow>::getInstanceSNoArgs(wrk));
	nativeWindow->setRefConstant();
}

Vector2 Stage::getStageSize() const
{
	if (scaleMode != StageScale::NoScale)
		return movie.getSize().toPx();

	auto renderThread = getSys()->getRenderThread();
	return Vector2
	(
		renderThread->windowWidth,
		renderThread->windowHeight
	);
}

void Stage::setScaleMode(const StageScale& mode)
{
	auto oldScaleMode = scaleMode;
	scaleMode = mode;
	if (oldScaleMode == scaleMode || fullScreenSourceRect.isValid())
		return;

	auto rt = getSys()->getRenderThread();
	rt->requestResize(UINT32_MAX, UINT32_MAX, true);
}

InteractiveObject& Stage::getFocusTarget()
{
	Locker l(focusSpinlock);
	return
	(
		focus != nullptr &&
		focus->isOnStage() &&
		focus->isVisible()
	) ? *focus : *this;
}

InteractiveObject* Stage::getAVM1FocusTarget()
{
	Locker l(focusSpinlock);
	return avm1Focus;
}

void Stage::setTabFocusTarget(bool next)
{
	auto focusTarget = getFocusTarget();
	auto startPos; focusTarget.localToGlobal(Vector2Twips());
	std::map<int32_t, DisplayObject&> distanceMap;
	bool hasTabIndices = false;
	fillTabStopsAutomatic(distanceMap, hasTabIndices);
	DisplayObject* newfocustarget=nullptr;


	if (hasTabIndices)
	{
		std::map<int32_t,DisplayObject*> tabindexmap;
		fillTabStopsByTabIndex(tabindexmap);
		int32_t currenttabindex=-1;
		if (currentfocustarget && currentfocustarget->is<InteractiveObject>())
			currenttabindex=currentfocustarget->as<InteractiveObject>()->tabIndex;

		auto it = tabindexmap.find(currenttabindex);
		if (it==tabindexmap.end())
			it=tabindexmap.begin();
		else
		{
			if (next)
			{
				it++;
				if (it==tabindexmap.end())
					it=tabindexmap.begin();
			}
			else
			{
				if (it==tabindexmap.begin())
					it=tabindexmap.end();
				it--;
			}
		}
		newfocustarget = it->second;
	}
	else if (!distancemap.empty())
	{
		auto it = distancemap.find(startposition.y*internalGetWidth()+startposition.x);
		if (it==distancemap.end())
			it=distancemap.begin();
		else
		{
			if (next)
			{
				it++;
				if (it==distancemap.end())
					it=distancemap.begin();
			}
			else
			{
				if (it==distancemap.begin())
					it=distancemap.end();
				it--;
			}
		}
		newfocustarget = it->second;
	}

	bool handleFocusEvent =
	(
		newFocusTarget != nullptr &&
		!newFocusTarget->is<InteractiveObject>() &&
		newFocusTarget->toASObject().isNull()
	);

	if (!handleFocusEvent)
		return; 

	auto obj = newFocusTarget->toASObject();

	// it seems that focus events are executed directly
	getVm(getSys())->tryAddEvent(obj, _MR(Class<FocusEvent>::getInstanceS
	(
		obj->getInstanceWorker(),
		"keyFocusChange",
		true,
		&focusTarget,
		AS3KEYCODE_TAB,
		!next
	)));

	setFocusTarget(*newFocusTarget->as<InteractiveObject>(), false);
}

bool Stage::setFocusTarget
(
	InteractiveObject* obj,
	bool setFromMouse,
	bool preventAVM1Events = false
)
{
	auto wrk = getSys()->worker;
	Locker l(focusSpinlock);

	bool ret = true;
	if (obj != nullptr && !obj->isFocusable(setFromMouse))
	{
		obj = nullptr;
		ret = false;
	}

	auto oldFocus = focus;
	focus = obj;

	auto addFocusEvent = [&]
	(
		const tiny_string& name
		InteractiveObject* focusRel,
	)
	{
		if (focusRel == nullptr)
			return;

		auto _obj = focusRel.toASObject();
		if (_obj.isNull())
			return;

		auto vm = getVm(getSys());
		// it seems that focus events are executed directly
		vm->tryAddEvent(_obj, _MR(Class<FocusEvent>::getInstanceS
		(
			wrk,
			name,
			false,
			focusRel
		)));
	};

	if (oldFocus != nullptr)
	{
		oldFocus->lostFocus();
		addFocusEvent("focusOut",
		(
			obj != nullptr &&
			obj != this &&
			obj->is<RootMovieClip>()
		) ? obj : nullptr);
	}

	if (focus != nullptr)
	{
		focus->gotFocus();
		addFocusEvent("focusIn",
		(
			oldFocus != nullptr &&
			oldFocus != this &&
		) ? oldFocus : nullptr);
	}

	l.release();

	[&]
	{
		if (focus == avm1Focus && obj != nullptr)
			return;
		bool _setFocus = obj == nullptr ||
		(
			focus == nullptr && ret
		) ||
		(
			focus != nullptr &&
			focus->isFocusable(setFromMouse)
		);

		bool isValid =
		(
			avm1Focus != nullptr &&
			avm1Focus->is<TextField>()
		) || _setFocus;
		if (!isValid)
			return;

		auto oldAVM1Focus = avm1Focus;
		avm1Focus = focus;
		if (preventAVM1Events)
			return;

		auto rootClip = getRootClip();
		if (rootClip == nullptr)
			return;


		auto oldFocusVal =
		(
			oldAVM1Focus != nullptr ?
			oldAVM1Focus->toAVM1ValueOrUndef() :
			AVM1Value::nullVal
		);

		auto newFocusVal =
		(
			_setFocus &&
			avm1Focus != nullptr
		) ? avm1Focus->toAVM1ValueOrUndef() : AVM1Value::nullVal;

		AVM1Context::notifySystemListeners
		(
			rootClip,
			"Selection",
			"onSetFocus",
			{ oldFocusVal, newFocusVal }
		);
	}();

	if (oldFocus != nullptr)
		oldFocus->requestInvalidation(getSys());
	if (focus != nullptr)
		focus->requestInvalidation(getSys());

	l.acquire();
	return ret;
}

void Stage::checkResetFocusTarget(InteractiveObject* removedtarget)
{
	Locker l(focusSpinlock);
	if (focus == removedtarget && !getSys()->isShuttingDown())
		setFocusTarget(nullptr, false);
}

void Stage::addHiddenObject(DisplayObject& obj)
{
	auto isHiddenObj = [&hiddenObjects](DisplayObject& obj)
	{
		return std::find_if
		(
			hiddenObjects.begin(),
			hiddenObjects.end(),
			[&](const auto& _obj) { return &_obj == &obj; }
		) != hiddenObjects.end();
	};

	auto _obj = obj.toASObject();
	if (_obj.isNull() || !_obj->getInstanceWorker()->isPrimordial)
		return;

	if (isHiddenObj(obj))
		return;
	assert(&obj != this);

	// don't add hidden object if any ancestor was already added as one
	for (auto p = obj.getParent(); p != nullptr; p = p->getParent())
	{
		if (isHiddenObj(*p))
			return;
	}

	hiddenObjects.emplace_back(obj);
}

void Stage::removeHiddenObject(DisplayObject& obj)
{
	auto it = std::find_if
	(
		hiddenObjects.begin(),
		hiddenObjects.end(),
		[&](const auto& _obj) { return &_obj == &obj; }
	);

	if (it != hiddenObjects.end())
		hiddenObjects.erase(it);
}

void Stage::forEachHiddenObject(std::function<void(DisplayObject*)> callback, bool allowInvalid)
{
	auto& list = hiddenObjects;
	// NOTE: A range for loop can't be used here because a clip can be
	// removed from the list during the callback.
	for (auto it = list.begin(); it != list.end(); ++it)
	{
		auto& clip = it->get();
		if (!allowInvalid && clip.getParent() != nullptr)
			continue;

		// NOTE: Because the clip can be removed from the list during the
		// callback, we need to get the previous iterator for later.
		auto prevIt = std::prev(it);
		callback(&clip);

		if (std::next(prevIt) != it)
		{
			// The clip was removed from the list, so use the previous
			// iterator.
			it = prevIt;
		}
	}
}

void Stage::cleanupDeadHiddenObjects()
{
	auto it = hiddenObjects.begin();
	while (it != hiddenObjects.end())
	{
		if (it->get().getParent() != nullptr)
			it = hiddenObjects.erase(it);
		else
			++it;
	}
}

void Stage::prepareForRemoval(DisplayObject& d)
{
	Locker l(DisplayObjectRemovedMutex);
	removedDisplayObjects.emplace(d);
}

void Stage::cleanupRemovedDisplayObjects()
{
	if (getVm(getSys()) != nullptr)
		getVm(getSys())->clearDeletableObjects();

	if (getSys()->isShuttingDown())
		return;

	Locker l(DisplayObjectRemovedMutex);
	auto& list = removedDisplayObjects;
	for (auto it = list.begin(); it != list.end(); it = list.erase(it))
	{
		auto& obj = it->get();
		// DisplayObjects with broadcast listeners are not destroyed, only hidden
		if (!obj.hasBroadcastListeners())
			continue;
		auto clip = obj.as<MovieClip>();
		if (clip != nullptr && clip->state.advancedByTick)
		{
			// ensure the hidden MovieClip is not advanced on the next tick
			clip->state.next_FP = clip->state.FP;
		}
		addHiddenObject(obj);
	}
}

void Stage::AVM1AddDisplayObject(DisplayObject& obj)
{
	if (!hasAVM1Clips || obj.isAS3())
		return;

	Locker l(avm1DisplayObjectMutex);
	bool canAdd =
	(
		obj.avm1PrevDisplayObject == nullptr &&
		obj.avm1NextDisplayObject == nullptr &&
		avm1DisplayObjectFirst != &obj
	);

	if (!canAdd)
		return;

	if (avm1DisplayObjectFirst == nullptr)
	{
		avm1DisplayObjectFirst = &obj;
		avm1DisplayObjectLast = &dobj;
		return;
	}

	obj.avm1NextDisplayObject = avm1DisplayObjectFirst;
	avm1DisplayObjectFirst->avm1PrevDisplayObject = &obj;
	avm1DisplayObjectFirst = &obj;
}

void Stage::AVM1RemoveDisplayObject(DisplayObject& obj)
{
	if (!hasAVM1Clips)
		return;

	Locker l(avm1DisplayObjectMutex);
	bool canRemove =
	(
		obj.avm1PrevDisplayObject != nullptr ||
		dobj.avm1NextDisplayObject != nullptr
	);

	if (!canRemove)
		return;

	if (obj.avm1PrevDisplayObject != nullptr)
		obj.avm1PrevDisplayObject->avm1NextDisplayObject = obj.avm1NextDisplayObject;
	else
	{
		avm1DisplayObjectFirst = dobj.avm1NextDisplayObject;
		avm1DisplayObjectFirst->avm1PrevDisplayObject = nullptr;
	}

	if (obj.avm1NextDisplayObject != nullptr)
		obj.avm1NextDisplayObject->avm1PrevDisplayObject = dobj.avm1PrevDisplayObject;
	else
	{
		avm1DisplayObjectLast = obj.avm1PrevDisplayObject;
		obj.avm1PrevDisplayObject->avm1NextDisplayObject = nullptr;
	}

	obj.avm1PrevDisplayObject = nullptr;
	obj.avm1NextDisplayObject = nullptr;
}

void Stage::AVM1AddScriptToExecute(AVM1scriptToExecute& script)
{
	assert(!script.clip->isAS3());
	Locker l(avm1ScriptMutex);
	avm1ScriptsToExecute.push_back(script);
}

void Stage::AVM1SetLevelRoot(int32_t level, RootMovieClip& root)
{
	addChildAt(root, level);
}

void Stage::AVM1removeLevelRoot(int32_t level)
{
	deleteChildAt(level);
}

RootMovieClip* Stage::AVM1getLevelRoot(int32_t level)
{
	auto root = tryGetChildAt(level);
	return root != nullptr ? root->as<RootMovieClip>() : nullptr;
}

void Stage::enterFrame(bool implicit)
{
	auto list = cloneDisplayList();
	for (auto& child : list)
		child->enterFrame(implicit);
	executeAVM1Scripts(implicit);
}

void Stage::advanceFrame(bool implicit)
{
	auto root = getSys()->mainClip;
	if (!root->getFramesLoaded())
		return;
	if (root->getParent() == nullptr)
		insertChildAt(0, *root, false, false);

	forEachHiddenObject([&](DisplayObject* obj)
	{
		obj->advanceFrame(implicit);
	});

	DisplayObjectContainer::advanceFrame(implicit);
	executeAVM1Scripts(implicit);
}

void Stage::executeAVM1Scripts(bool implicit)
{
	if (!hasAVM1Clips)
		return;

	// scripts on AVM1 clips are executed in order of instantiation
	avm1DisplayObjectMutex.lock();
	auto obj = avm1DisplayObjectFirst;
	avm1DisplayObjectMutex.unlock();

	DisplayObject* prev = nullptr;
	DisplayObject* next = nullptr;

	for (; obj != nullptr; obj = next)
	{
		if (!obj->isAS3() && obj->isCreatedByTimeline())
			obj->advanceFrame(implicit);

		Locker l(avm1DisplayObjectMutex);
		bool isRemoved =
		(
			obj->avm1NextDisplayObject == nullptr &&
			obj->avm1PrevDisplayObject == nullptr
		);

		if (!isRemoved)
		{
			next = obj->avm1NextDisplayObject;
			prev = obj;
			continue;
		}

		// clip was removed from list during frame advance
		next =
		(
			prev != nullptr ?
			prev->avm1NextDisplayObject :
			obj != avm1DisplayObjectFirst ?
			avm1DisplayObjectFirst :
			nullptr
		);
	}

	avm1ScriptMutex.lock();
	auto& list = avm1ScriptsToExecute;
	for (auto it = list.begin(); it != list.end(); it = list.erase(it))
	{
		if (!it->clip->isMarkedForTimelineDeletion())
			it->execute();
	}
	avm1ScriptMutex.unlock();

	avm1DisplayObjectMutex.lock();
	obj = avm1DisplayObjectFirst;
	for (; obj != nullptr; obj = obj->avm1NextDisplayObject)
	{
		if (obj->isCreatedByTimeline())
			obj->AVM1AfterAdvance();
	}
	avm1DisplayObjectMutex.unlock();
	AVM1AfterAdvance();
}

void Stage::initFrame()
{
	forEachHiddenObject([&](DisplayObject* obj)
	{
		if (obj->isConstructed())
			obj->initFrame();
	});
	DisplayObjectContainer::initFrame();
}

void Stage::executeFrameScript()
{
	forEachHiddenObject([&](DisplayObject* obj)
	{
		obj->executeFrameScript();
	});
	DisplayObjectContainer::executeFrameScript();
}

void Stage::AVM1HandleEvent(EventDispatcher* dispatcher, Event* e)
{
	if (e->is<KeyboardEvent>())
	{
		avm1listenerMutex.lock();
		vector<asAtom> tmplisteners = avm1KeyboardListeners;
		for (auto it = tmplisteners.begin(); it != tmplisteners.end(); it++)
			ASATOM_ADDSTOREDMEMBER(*it);
		avm1listenerMutex.unlock();

		if (e->type =="keyDown")
		{
			getSystemState()->getInputThread()->setLastKeyDown(e->as<KeyboardEvent>());
			for (auto it = tmplisteners.begin(); it != tmplisteners.end(); it++)
			{
				ASObject* o = asAtomHandler::getObject(*it);
				if (o)
				{
					o->AVM1HandleKeyboardEvent(e->as<KeyboardEvent>());
					o->removeStoredMember();
				}
			}
			bool handled = false;
			auto it = tmplisteners.begin();
			while (it != tmplisteners.end())
			{
				ASObject* o = asAtomHandler::getObject(*it);
				if (o)
				{
					if (!handled && o->AVM1HandleKeyPressedEvent(e->as<KeyboardEvent>()))
						handled = true;
				}
				it++;
			}
			if (!e->defaultPrevented && !handled)
			{
				uint32_t modifiers = e->as<KeyboardEvent>()->getModifiers() & (KMOD_LSHIFT | KMOD_RSHIFT |KMOD_LCTRL | KMOD_RCTRL | KMOD_LALT | KMOD_RALT);
				if (modifiers == KMOD_NONE)
				{
					switch (e->as<KeyboardEvent>()->getKeyCode())
					{
						case AS3KEYCODE_ESCAPE:
							if (getSystemState()->getEngineData()->inFullScreenMode())
								getSystemState()->getEngineData()->setDisplayState("normal",getSystemState());
							break;
						case AS3KEYCODE_TAB:
							setTabFocusTarget(true);
							break;
						case AS3KEYCODE_ENTER:
						{
							auto dispobj=getAVM1FocusTarget();
							if (dispobj)
								dispobj->AVM1HandlePressedEvent(dispobj,false);
							break;
						}
						case AS3KEYCODE_SPACE:
						{
							auto dispobj=getAVM1FocusTarget();
							if (dispobj)
								dispobj->AVM1HandlePressedEvent(dispobj,true);
							break;
						}
						default:
							break;
					}
				}
				if (modifiers & KMOD_SHIFT)
				{
					switch (e->as<KeyboardEvent>()->getKeyCode())
					{
						case AS3KEYCODE_TAB:
							setTabFocusTarget(false);
							break;
						default:
							break;
					}
				}
			}
		}
		else if (e->type =="keyUp")
		{
			getSystemState()->getInputThread()->setLastKeyUp(e->as<KeyboardEvent>());
			if (!e->defaultPrevented)
			{
				uint32_t modifiers = e->as<KeyboardEvent>()->getModifiers() & (KMOD_LSHIFT | KMOD_RSHIFT |KMOD_LCTRL | KMOD_RCTRL | KMOD_LALT | KMOD_RALT);
				if (modifiers == KMOD_NONE)
				{
					switch (e->as<KeyboardEvent>()->getKeyCode())
					{
						case AS3KEYCODE_ENTER:
						{
							auto dispobj=getAVM1FocusTarget();
							if (dispobj)
								dispobj->AVM1HandleReleasedEvent(dispobj,false);
							break;
						}
						case AS3KEYCODE_SPACE:
						{
							auto dispobj=getAVM1FocusTarget();
							if (dispobj)
								dispobj->AVM1HandleReleasedEvent(dispobj,true);
							break;
						}
						default:
							break;
					}
				}
			}

			auto it = tmplisteners.begin();
			while (it != tmplisteners.end())
			{
				ASObject* o = asAtomHandler::getObject(*it);
				if (o)
				{
					o->AVM1HandleKeyboardEvent(e->as<KeyboardEvent>());
					o->removeStoredMember();
				}
				it++;
			}
		}
	}
	else if (e->is<MouseEvent>())
	{
		avm1listenerMutex.lock();
		// eventhandlers may change the listener list, so we work on a copy
		vector<asAtom> tmplisteners = avm1MouseListeners;
		for (auto it = avm1MouseListeners.begin(); it != avm1MouseListeners.end(); it++)
		{
			ASATOM_ADDSTOREDMEMBER(*it);
		}
		avm1listenerMutex.unlock();
		if (e->type=="mouseDown")
		{
			// AVM1 mouseDown events trigger multiple handlers that have to be handled in the correct order:
			// - onMouseDown
			// - onSetFocus
			// - onPressed
			for (auto it = tmplisteners.rbegin(); it != tmplisteners.rend(); it++)
			{
				ASObject* o = asAtomHandler::getObject(*it);
				if (o)
					o->AVM1HandleMouseEvent(dispatcher, e->as<MouseEvent>());
			}
			if (dispatcher->is<InteractiveObject>())
				setFocusTarget(asAtomHandler::fromObject(dispatcher),true);
			for (auto it = tmplisteners.rbegin(); it != tmplisteners.rend(); it++)
			{
				ASObject* o = asAtomHandler::getObject(*it);
				if (o)
				{
					if (!asAtomHandler::is<DisplayObject>(*it)
						|| asAtomHandler::as<DisplayObject>(*it)->AVM1isHitByMouseEvent(dispatcher,e->as<MouseEvent>())
						)
						o->AVM1HandlePressedEvent(dispatcher,true);
					o->removeStoredMember();
				}
			}
		}
		else
		{
			auto it = tmplisteners.begin();
			while (it != tmplisteners.end())
			{
				ASObject* o = asAtomHandler::getObject(*it);
				if (o)
				{
					o->AVM1HandleMouseEvent(dispatcher, e->as<MouseEvent>());
					o->removeStoredMember();
				}
				it++;
			}
		}
	}
	else
	{
		avm1listenerMutex.lock();
		vector<pair<ASObject*,uint32_t>> tmplisteners = avm1EventListeners;
		for (auto it = tmplisteners.begin(); it != tmplisteners.end(); it++)
		{
			it->first->incRef();
		}
		avm1listenerMutex.unlock();
		// eventhandlers may change the listener list, so we work on a copy
		auto it = tmplisteners.rbegin();
		while (it != tmplisteners.rend())
		{
			ASObject* o = it->first;
			o->AVM1HandleEvent(dispatcher, e);
			o->decRef();
			++it;
		}
		if (!avm1ResizeListeners.empty() && dispatcher==this && e->type=="resize")
		{
			avm1listenerMutex.lock();
			vector<ASObject*> tmplisteners = avm1ResizeListeners;
			for (auto it = tmplisteners.begin(); it != tmplisteners.end(); it++)
			{
				(*it)->incRef();
			}
			avm1listenerMutex.unlock();
			// eventhandlers may change the listener list, so we work on a copy
			auto it = tmplisteners.rbegin();
			while (it != tmplisteners.rend())
			{
				asAtom func=asAtomHandler::invalidAtom;
				multiname m(nullptr);
				m.name_type=multiname::NAME_STRING;
				m.isAttribute = false;
				m.name_s_id=BUILTIN_STRINGS::STRING_ONRESIZE;
				(*it)->getVariableByMultiname(func,m,GET_VARIABLE_OPTION::NONE,getInstanceWorker());
				if (asAtomHandler::is<AVM1Function>(func))
				{
					asAtom ret=asAtomHandler::invalidAtom;
					asAtom obj = asAtomHandler::fromObject(this);
					asAtomHandler::as<AVM1Function>(func)->call(&ret,&obj,nullptr,0);
					asAtomHandler::as<AVM1Function>(func)->decRef();
				}
				(*it)->decRef();
				it++;
			}
		}
	}
}

bool Stage::addAVM1KeyListener(DisplayObject& obj)
{
	Locker l(avm1ListenerMutex);
	auto it = std::find_if
	(
		avm1KeyListeners.begin(),
		avm1KeyListeners.end(),
		[&](const auto& _obj) { return &obj == &_obj.get(); }
	);
	if (it != avm1KeyListeners.end())
		return false;

	avm1KeyListeners.emplace_back(obj);
	return true;
}

bool Stage::removeAVM1KeyListener(DisplayObject& obj)
{
	Locker l(avm1ListenerMutex);
	auto it = std::find_if
	(
		avm1KeyListeners.begin(),
		avm1KeyListeners.end(),
		[&](const auto& _obj) { return &obj == &_obj.get(); }
	);
	if (it == avm1KeyListeners.end())
		return false;
	avm1KeyListeners.erase(it);
	return true;
}

std::vector<DisplayObjectRef> Stage::getAVM1KeyListeners()
{
	Locker l(avm1ListenerMutex);
	auto listeners = avm1KeyListeners;
	return listeners;
}

bool Stage::AVM1AddMouseListener(DisplayObject& obj)
{
	auto findFunc = [&](const DisplayObject& _obj)
	{
		if (&obj == &_obj)
			return true;

		auto commonParent = obj.findCommonAncestor(_obj);
		if (commonParent == nullptr)
			return false;

		auto depthA = obj.findParentDepth(commonParent);
		auto depthB = _obj.findParentDepth(commonParent);
		auto parentA = obj.getAncestor(depthB >= 0 ? depthA - 1 : 0);
		auto parentB = _obj.getAncestor(depthA >= 0 ? depthB - 1 : 0);
		if (parentA == nullptr && parentB == nullptr)
			return false;
		return parentA->getDepth() < parentB->getDepth();
	};

	Locker l(avm1ListenerMutex);
	auto it = std::find_if
	(
		avm1MouseListeners.begin(),
		avm1MouseListeners.end(),
		findFunc
	);

	if (it == avm1MoustListeners.end())
	{
		avm1MouseListeners.emplace_back(obj);
		return true;
	}

	if (&obj == &it->get())
		return false;
	avm1MouseListeners.insert(it, obj);
	return true;
}

bool Stage::removeAVM1MouseListener(DisplayObject& obj)
{
	Locker l(avm1ListenerMutex);
	auto it = std::find_if
	(
		avm1MouseListeners.begin(),
		avm1MouseListeners.end(),
		[&](const auto& _obj) { return &obj == &_obj.get(); }
	);
	if (it == avm1MouseListeners.end())
		return false;
	avm1MouseListeners.erase(it);
	return true;
}

std::vector<DisplayObjectRef> Stage::getAVM1MouseListeners()
{
	Locker l(avm1ListenerMutex);
	auto listeners = avm1MouseListeners;
	return listeners;
}

void Stage::addAVM1EventListener(_GC<AVM1Object> obj)
{
	Locker l(avm1ListenerMutex);
	auto it = std::find_if
	(
		avm1EventListeners.begin(),
		avm1EventListeners.end(),
		[&](const auto& pair) { return pair.first == obj; }
	);

	if (it == avm1EventListeners.end())
	{
		++it->second;
		return;
	}

	avm1EventListeners.emplace_back(obj, 1);
}
void Stage::removeAVM1EventListener(_GC<AVM1Object> obj)
{
	Locker l(avm1ListenerMutex);
	auto it = std::find_if
	(
		avm1EventListeners.begin(),
		avm1EventListeners.end(),
		[&](const auto& pair) { return pair.first == obj; }
	);

	if (it == avm1EventListeners.end())
		return;

	assert(it->second);
	if (!--it->second)
		avm1EventListeners.erase(it);
}

void Stage::addAVM1EventListener(_GC<AVM1Object> obj)
{
	Locker l(avm1ListenerMutex);
	auto it = std::find
	(
		avm1ResizeListeners.begin(),
		avm1ResizeListeners.end(),
		obj
	);

	if (it == avm1ResizeListeners.end())
		avm1ResizeListeners.emplace_back(obj);
}

bool Stage::removeAVM1EventListener(_GC<AVM1Object> obj)
{
	Locker l(avm1ListenerMutex);
	auto it = std::find
	(
		avm1ResizeListeners.begin(),
		avm1ResizeListeners.end(),
		obj
	);

	if (it == avm1ResizeListeners.end())
		return false;

	avm1ResizeListeners.erase(it);
	// it's not mentioned in the specs but I assume we return true if we found the listener object
	return true;
}

void Stage::forceInvalidation()
{
	RELEASE_WRITE(invalidated, true);

	auto event = _MR(new
	(
		getSys()->unaccountedMemory
	) FlushInvalidationQueueEvent());

	getVm(getSys())->addEvent(NullRef, event);
}
