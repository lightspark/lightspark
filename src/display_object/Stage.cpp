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

bool Stage::setFocusTarget(const asAtom newfocus, bool setFromMouse, bool preventAVM1Events)
{
	Locker l(focusSpinlock);
	bool isundefined = asAtomHandler::isUndefined(newfocus);
	InteractiveObject* f = asAtomHandler::is<InteractiveObject>(newfocus) ? asAtomHandler::as<InteractiveObject>(newfocus) : nullptr;
	if (f==focus && !isundefined)
		return newfocus.uintval != asAtomHandler::falseAtom.uintval;
	bool ret = true;
	if (f && !f->isFocusable(setFromMouse))
	{
		f=nullptr;
		ret = false;
	}
	InteractiveObject* oldfocus = focus;
	focus = f;
	if (oldfocus)
	{
		oldfocus->lostFocus();
		_NR<InteractiveObject> focusrel;
		if (f && f!=this && !f->is<RootMovieClip>())
		{
			f->incRef();
			focusrel=_MR(f);
		}

		auto e = _MR(Class<FocusEvent>::getInstanceS(getInstanceWorker(),"focusOut",false,focusrel));
		// it seems that focus events are executed directly
		if(isVmThread())
			ABCVm::publicHandleEvent(oldfocus,e);
		else
			getVm(getSystemState())->addEvent(_MR(oldfocus),e);
	}
	if (focus)
	{
		focus->incRef();
		focus->addStoredMember();
		focus->gotFocus();
		_NR<InteractiveObject> focusrel;
		if (oldfocus && oldfocus!=this)
		{
			oldfocus->incRef();
			focusrel=_MR(oldfocus);
		}
		auto e = _MR(Class<FocusEvent>::getInstanceS(getInstanceWorker(),"focusIn",false,focusrel));
		// it seems that focus events are executed directly
		if(isVmThread())
			ABCVm::publicHandleEvent(focus,e);
		else
		{
			focus->incRef();
			getVm(getSystemState())->addEvent(_MR(focus),e);
		}
	}
	l.release();

	if (focus != avm1focus || isundefined)
	{
		bool setfocus = isundefined
						|| (!focus && ret)
						|| (focus && focus->isFocusable(setFromMouse));
		InteractiveObject* avm1oldfocus = avm1focus;
		if ((avm1oldfocus && avm1oldfocus->is<TextField>())
			|| setfocus)
		{
			avm1focus = focus;
			if (avm1focus)
			{
				avm1focus->incRef();
				avm1focus->addStoredMember();
			}
			if (!preventAVM1Events)
			{
				avm1listenerMutex.lock();
				vector<asAtom> tmplisteners = avm1FocusListeners;
				for (auto it = tmplisteners.begin(); it != tmplisteners.end(); it++)
				{
					ASATOM_INCREF(*it);
				}
				avm1listenerMutex.unlock();
				for (auto it = tmplisteners.begin(); it != tmplisteners.end(); it++)
				{
					ASObject* o = asAtomHandler::getObject(*it);
					if (o)
					{
						o->AVM1HandleSetFocusEvent(
							setfocus ? focus : nullptr
							,avm1oldfocus);
						o->decRef();
					}
				}
			}
			if (avm1oldfocus)
				avm1oldfocus->removeStoredMember();
		}
	}
	if (oldfocus)
		oldfocus->requestInvalidation(getSystemState());
	if (focus)
		focus->requestInvalidation(getSystemState());
	l.acquire();
	if (oldfocus)
		oldfocus->removeStoredMember();
	return ret;
}

void Stage::checkResetFocusTarget(InteractiveObject* removedtarget)
{
	Locker l(focusSpinlock);
	if (focus == removedtarget && !getSystemState()->isShuttingDown())
	{
		setFocusTarget(asAtomHandler::nullAtom,false);
	}
}
void Stage::addHiddenObject(DisplayObject* o)
{
	if (!o->getInstanceWorker()->isPrimordial)
		return;
	if (o->hiddenPrevDisplayObject || o->hiddenNextDisplayObject || this->hiddenNextDisplayObject == o)
		return;
	assert(o!=this);
	// don't add hidden object if any ancestor was already added as one
	DisplayObject* p=o->getParent();
	while (p && p != this)
	{
		if (p->hiddenPrevDisplayObject || p->hiddenNextDisplayObject)
			return;
		p=p->getParent();
	}
	if (this->hiddenNextDisplayObject==this)
	{
		this->hiddenNextDisplayObject=o;
		o->hiddenPrevDisplayObject=this;
		o->hiddenNextDisplayObject=this;
	}
	else
	{
		o->hiddenNextDisplayObject=this->hiddenNextDisplayObject;
		o->hiddenPrevDisplayObject=this;
		this->hiddenNextDisplayObject->hiddenPrevDisplayObject=o;
		this->hiddenNextDisplayObject=o;
	}
}

void Stage::removeHiddenObject(DisplayObject* o)
{
	if (!o->hiddenPrevDisplayObject || !o->hiddenNextDisplayObject || o==this)
		return;
	if (o->hiddenPrevDisplayObject !=this)
		o->hiddenPrevDisplayObject->hiddenNextDisplayObject=o->hiddenNextDisplayObject;
	else
	{
		this->hiddenNextDisplayObject=o->hiddenNextDisplayObject;
		this->hiddenNextDisplayObject->hiddenPrevDisplayObject=this;
	}
	if (o->hiddenNextDisplayObject!=this)
		o->hiddenNextDisplayObject->hiddenPrevDisplayObject=o->hiddenPrevDisplayObject;
	else
		o->hiddenPrevDisplayObject->hiddenNextDisplayObject=this;
	o->hiddenPrevDisplayObject=nullptr;
	o->hiddenNextDisplayObject=nullptr;
}

void Stage::forEachHiddenObject(std::function<void(DisplayObject*)> callback, bool allowInvalid)
{
	DisplayObject* clip = this->hiddenNextDisplayObject;
	while (clip && clip != this)
	{
		DisplayObject* nextclip = clip->hiddenNextDisplayObject;
		if ((allowInvalid || clip->getParent() == nullptr))
		{
			clip->incRef(); // clip may be destroyed inside callback, we have to delay that until callback is done
			callback(clip);
			clip->decRef();
		}
		clip = nextclip;
	}
}

void Stage::cleanupDeadHiddenObjects()
{
	DisplayObject* clip = this->hiddenNextDisplayObject;
	while (clip && clip != this)
	{
		DisplayObject* nextclip = clip->hiddenNextDisplayObject;
		if (clip->getParent() != nullptr)
			this->removeHiddenObject(clip);
		clip = nextclip;
	}
}

void Stage::prepareForRemoval(DisplayObject* d)
{
	Locker l(DisplayObjectRemovedMutex);
	if (!removedDisplayObjects.insert(d).second)
		d->removeStoredMember();
}

void Stage::cleanupRemovedDisplayObjects()
{
	if (getVm(getSystemState()))
		getVm(getSystemState())->clearDeletableObjects();
	Locker l(DisplayObjectRemovedMutex);
	auto it = removedDisplayObjects.begin();
	while (it != removedDisplayObjects.end())
	{
		DisplayObject* o = *it;
		it = removedDisplayObjects.erase(it);
		if (!getSystemState()->isShuttingDown())
		{
			if (o->hasBroadcastListeners()) // DisplayObjects with broadcast listeners are not destroyed, only hidden
			{
				if (o->is<MovieClip>() && !o->as<MovieClip>()->state.advancedByTick)
				{
					// ensure the hidden MovieClip is not advanced on the next tick
					o->as<MovieClip>()->state.next_FP = o->as<MovieClip>()->state.FP;
				}
				addHiddenObject(o);
			}
		}
		o->removeStoredMember();
	}
}

void Stage::AVM1AddDisplayObject(DisplayObject* dobj)
{
	if (!hasAVM1Clips || dobj->needsActionScript3())
		return;
	Locker l(avm1DisplayObjectMutex);
	if (dobj->avm1PrevDisplayObject || dobj->avm1NextDisplayObject || this->avm1DisplayObjectFirst == dobj)
		return;
	if (!this->avm1DisplayObjectFirst)
	{
		this->avm1DisplayObjectFirst=dobj;
		this->avm1DisplayObjectLast=dobj;
	}
	else
	{
		dobj->avm1NextDisplayObject=this->avm1DisplayObjectFirst;
		this->avm1DisplayObjectFirst->avm1PrevDisplayObject=dobj;
		this->avm1DisplayObjectFirst=dobj;
	}
}

void Stage::AVM1RemoveDisplayObject(DisplayObject* dobj)
{
	if (!hasAVM1Clips)
		return;
	Locker l(avm1DisplayObjectMutex);
	if (!dobj->avm1PrevDisplayObject && !dobj->avm1NextDisplayObject)
		return;
	if (dobj->avm1PrevDisplayObject)
		dobj->avm1PrevDisplayObject->avm1NextDisplayObject=dobj->avm1NextDisplayObject;
	else
	{
		this->avm1DisplayObjectFirst=dobj->avm1NextDisplayObject;
		this->avm1DisplayObjectFirst->avm1PrevDisplayObject=nullptr;
	}
	if (dobj->avm1NextDisplayObject)
		dobj->avm1NextDisplayObject->avm1PrevDisplayObject=dobj->avm1PrevDisplayObject;
	else
	{
		this->avm1DisplayObjectLast=dobj->avm1PrevDisplayObject;
		dobj->avm1PrevDisplayObject->avm1NextDisplayObject=nullptr;
	}
	dobj->avm1PrevDisplayObject=nullptr;
	dobj->avm1NextDisplayObject=nullptr;
}

void Stage::AVM1AddScriptToExecute(AVM1scriptToExecute& script)
{
	assert(!script.clip->needsActionScript3());
	Locker l(avm1ScriptMutex);
	avm1scriptstoexecute.push_back(script);
}

void Stage::AVM1SetLevelRoot(int level, RootMovieClip* root)
{
	avm1MapLevelToRoot[level]=root;
}

void Stage::AVM1removeLevelRoot(int level)
{
	avm1MapLevelToRoot.erase(level);
}

RootMovieClip* Stage::AVM1getLevelRoot(int level)
{
	auto it = avm1MapLevelToRoot.find(level);
	if (it != avm1MapLevelToRoot.end())
		return it->second;
	return nullptr;
}

void Stage::enterFrame(bool implicit)
{
	std::vector<_R<DisplayObject>> list;
	cloneDisplayList(list);
	for (auto child : list)
		child->enterFrame(implicit);
	executeAVM1Scripts(implicit);
}

void Stage::advanceFrame(bool implicit)
{
	if (getSystemState()->mainClip->getFramesLoaded()==0)
		return;
	if (!getSystemState()->mainClip->getParent())
	{
		getSystemState()->mainClip->incRef();
		insertLegacyChildAt(-16384, getSystemState()->mainClip, false, false);
	}
	forEachHiddenObject([&](DisplayObject* obj)
	{
		obj->advanceFrame(implicit);
	});
	DisplayObjectContainer::advanceFrame(implicit);
	executeAVM1Scripts(implicit);
}
void Stage::executeAVM1Scripts(bool implicit)
{
	if (hasAVM1Clips)
	{
		// scripts on AVM1 clips are executed in order of instantiation
		avm1DisplayObjectMutex.lock();
		DisplayObject* dobj = avm1DisplayObjectFirst;
		avm1DisplayObjectMutex.unlock();
		DisplayObject* prevdobj = nullptr;
		DisplayObject* nextdobj = nullptr;
		while (dobj)
		{
			dobj->incRef();
			if (!dobj->needsActionScript3() && dobj->isConstructed())
				dobj->advanceFrame(implicit);
			avm1DisplayObjectMutex.lock();
			if (!dobj->avm1NextDisplayObject && !dobj->avm1PrevDisplayObject) // clip was removed from list during frame advance
			{
				if (prevdobj)
					nextdobj = prevdobj->avm1NextDisplayObject;
				else if (dobj != avm1DisplayObjectFirst)
					nextdobj = avm1DisplayObjectFirst;
				else
					nextdobj = nullptr;
			}
			else 
			{
				nextdobj = dobj->avm1NextDisplayObject;
				prevdobj = dobj;
			}
			avm1DisplayObjectMutex.unlock();
			dobj->decRef();
			dobj = nextdobj;
		}
		avm1ScriptMutex.lock();
		auto itscr = avm1scriptstoexecute.begin();
		while (itscr != avm1scriptstoexecute.end())
		{
			if (!(*itscr).clip->markedForLegacyDeletion)
				(*itscr).execute();
			else
				(*itscr).clip->decRef(); // was increffed in AVM1AddScriptEvents
			itscr = avm1scriptstoexecute.erase(itscr);
		}
		avm1ScriptMutex.unlock();
		
		avm1DisplayObjectMutex.lock();
		dobj = avm1DisplayObjectFirst;
		while (dobj)
		{
			if (dobj->isConstructed())
				dobj->AVM1AfterAdvance();
			dobj = dobj->avm1NextDisplayObject;
		}
		avm1DisplayObjectMutex.unlock();
		AVM1AfterAdvance();
	}
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

bool Stage::AVM1AddKeyboardListener(asAtom listener)
{
	Locker l(avm1listenerMutex);
	for (auto it = avm1KeyboardListeners.begin(); it != avm1KeyboardListeners.end(); it++)
	{
		if ((*it).uintval == listener.uintval)
			return false;
	}
	ASATOM_ADDSTOREDMEMBER(listener);
	avm1KeyboardListeners.push_back(listener);
	return true;
}

bool Stage::AVM1RemoveKeyboardListener(asAtom listener)
{
	Locker l(avm1listenerMutex);
	for (auto it = avm1KeyboardListeners.begin(); it != avm1KeyboardListeners.end(); it++)
	{
		if ((*it).uintval == listener.uintval)
		{
			avm1KeyboardListeners.erase(it);
			ASATOM_REMOVESTOREDMEMBER(listener);
			return true;
		}
	}
	return true;
}

void Stage::AVM1GetKeyboardListeners(AVM1Array* res)
{
	Locker l(avm1listenerMutex);
	res->resize(avm1KeyboardListeners.size());
	for (uint32_t i = 0; i < avm1KeyboardListeners.size();i++)
	{
		asAtom l = avm1KeyboardListeners.at(i);
		res->set(i,l,false);
	}
}
bool Stage::AVM1AddMouseListener(asAtom listener)
{
	Locker l(avm1listenerMutex);
	auto it = std::find_if(avm1MouseListeners.begin(), avm1MouseListeners.end(), [&](asAtom obj)
	{
		if (obj.uintval == listener.uintval)
			return true;
		ASObject* o = asAtomHandler::getObject(listener);
		if (o && o->is<DisplayObject>() && asAtomHandler::is<DisplayObject>(obj))
		{
			DisplayObject* dispA = o->as<DisplayObject>();
			DisplayObject* dispB = asAtomHandler::as<DisplayObject>(obj);

			if (dispA != nullptr && dispB != nullptr)
			{
				auto commonAncestor = dispA->findCommonAncestor(dispB);
				int parentDepthA = dispA->findParentDepth(commonAncestor);
				int parentDepthB = dispB->findParentDepth(commonAncestor);

				if (commonAncestor != nullptr)
				{
					int depthA = 16384 + commonAncestor->findLegacyChildDepth(dispA->getAncestor(parentDepthB < 0 ? 0 : parentDepthA-1));
					int depthB = 16384 + commonAncestor->findLegacyChildDepth(dispB->getAncestor(parentDepthA < 0 ? 0 : parentDepthB-1));
					return depthA < depthB;
				}
			}
		}
		return false;
	});
	if (it != avm1MouseListeners.end() && (*it).uintval == listener.uintval)
		return true;
	if (it != avm1MouseListeners.end())
		avm1MouseListeners.insert(it, listener);
	else
		avm1MouseListeners.push_back(listener);
	ASATOM_ADDSTOREDMEMBER(listener);
	return true;
}

bool Stage::AVM1RemoveMouseListener(asAtom listener)
{
	Locker l(avm1listenerMutex);
	for (auto it = avm1MouseListeners.begin(); it != avm1MouseListeners.end(); it++)
	{
		if ((*it).uintval == listener.uintval)
		{
			ASATOM_REMOVESTOREDMEMBER(listener);
			avm1MouseListeners.erase(it);
			return true;
		}
	}
	return false;
}

void Stage::AVM1GetMouseListeners(AVM1Array* res)
{
	Locker l(avm1listenerMutex);
	res->resize(avm1MouseListeners.size());
	for (uint32_t i = 0; i < avm1MouseListeners.size();i++)
	{
		asAtom listener = avm1MouseListeners.at(i);
		res->set(i,listener,false);
	}
}

void Stage::AVM1AddEventListener(ASObject *o)
{
	Locker l(avm1listenerMutex);
	o->incRef();
	o->addStoredMember();
	for (auto it = avm1EventListeners.begin(); it != avm1EventListeners.end(); it++)
	{
		if ((*it).first == o)
		{
			++(*it).second;
			return;
		}
	}
	avm1EventListeners.push_back(make_pair(o,1));
}
void Stage::AVM1RemoveEventListener(ASObject *o)
{
	Locker l(avm1listenerMutex);
	for (auto it = avm1EventListeners.begin(); it != avm1EventListeners.end(); it++)
	{
		if ((*it).first == o)
		{
			assert ((*it).second);
			--(*it).second;
			o->removeStoredMember();
			if ((*it).second==0)
				avm1EventListeners.erase(it);
			break;
		}
	}
}

void Stage::AVM1AddResizeListener(ASObject *o)
{
	Locker l(avm1listenerMutex);
	for (auto it = avm1ResizeListeners.begin(); it != avm1ResizeListeners.end(); it++)
	{
		if ((*it) == o)
			return;
	}
	avm1ResizeListeners.push_back(o);
}

bool Stage::AVM1RemoveResizeListener(ASObject *o)
{
	Locker l(avm1listenerMutex);
	for (auto it = avm1ResizeListeners.begin(); it != avm1ResizeListeners.end(); it++)
	{
		if ((*it) == o)
		{
			avm1ResizeListeners.erase(it);
			o->decRef();
			// it's not mentioned in the specs but I assume we return true if we found the listener object
			return true;
		}
	}
	return false;
}
void Stage::AVM1AddFocusListener(asAtom listener)
{
	Locker l(avm1listenerMutex);
	for (auto it = avm1FocusListeners.begin(); it != avm1FocusListeners.end(); it++)
	{
		if ((*it).uintval == listener.uintval)
			return;
	}
	ASATOM_ADDSTOREDMEMBER(listener);
	avm1FocusListeners.push_back(listener);
}

bool Stage::AVM1RemoveFocusListener(asAtom listener)
{
	Locker l(avm1listenerMutex);
	for (auto it = avm1FocusListeners.begin(); it != avm1FocusListeners.end(); it++)
	{
		if ((*it).uintval == listener.uintval)
		{
			avm1FocusListeners.erase(it);
			ASATOM_REMOVESTOREDMEMBER(listener);
			return true;
		}
	}
	return false;
}
ASFUNCTIONBODY_ATOM(Stage,_getFocus)
{
	Stage* th=asAtomHandler::as<Stage>(obj);
	InteractiveObject* focus = th->getFocusTarget();
	if (!focus || focus==th || focus->is<RootMovieClip>())
	{
		ret = asAtomHandler::nullAtom;
		return;
	}
	else
	{
		focus->incRef();
		ret = asAtomHandler::fromObject(focus);
	}
}

ASFUNCTIONBODY_ATOM(Stage,_setFocus)
{
	Stage* th=asAtomHandler::as<Stage>(obj);
	_NR<InteractiveObject> focus;
	ARG_CHECK(ARG_UNPACK(focus));
	th->setFocusTarget(asAtomHandler::fromObject(focus.getPtr()),false);
}

ASFUNCTIONBODY_ATOM(Stage,_setTabChildren)
{
	// The specs says that Stage.tabChildren should throw
	// IllegalOperationError, but testing shows that instead of
	// throwing this simply ignores the value.
}

ASFUNCTIONBODY_ATOM(Stage,_getFrameRate)
{
	Stage* th=asAtomHandler::as<Stage>(obj);
	RootMovieClip* root = th->getRoot();
	if (!root)
		asAtomHandler::setNumber(ret, wrk->getSystemState()->mainClip->applicationDomain->getFrameRate());
	else
		asAtomHandler::setNumber(ret, root->applicationDomain->getFrameRate());
}

ASFUNCTIONBODY_ATOM(Stage,_setFrameRate)
{
	Stage* th=asAtomHandler::as<Stage>(obj);
	number_t frameRate;
	ARG_CHECK(ARG_UNPACK(frameRate));
	RootMovieClip* root = th->getRoot();
	if (root)
		root->applicationDomain->setFrameRate(frameRate);
}

ASFUNCTIONBODY_ATOM(Stage,_getAllowFullScreen)
{
	asAtomHandler::setBool(ret,wrk->getSystemState()->allowFullscreen);
}

ASFUNCTIONBODY_ATOM(Stage,_getAllowFullScreenInteractive)
{
	asAtomHandler::setBool(ret,wrk->getSystemState()->allowFullscreenInteractive);
}

ASFUNCTIONBODY_ATOM(Stage,_getColorCorrectionSupport)
{
	asAtomHandler::setBool(ret,false); // until color correction is implemented
}

ASFUNCTIONBODY_ATOM(Stage,_getWmodeGPU)
{
	asAtomHandler::setBool(ret,false);
}
ASFUNCTIONBODY_ATOM(Stage,_invalidate)
{
	Stage* th=asAtomHandler::as<Stage>(obj);
	th->forceInvalidation();
}
void Stage::forceInvalidation()
{
	RELEASE_WRITE(this->invalidated,true);
	_R<FlushInvalidationQueueEvent> event=_MR(new (getSystemState()->unaccountedMemory) FlushInvalidationQueueEvent());
	getVm(getSystemState())->addEvent(NullRef,event);
}
ASFUNCTIONBODY_ATOM(Stage,_getColor)
{
	Stage* th=asAtomHandler::as<Stage>(obj);
	RGB rgb;
	RootMovieClip* root = th->getRoot();
	if (root)
		rgb = root->getBackground();
	asAtomHandler::setUInt(ret,rgb.toUInt());
}

ASFUNCTIONBODY_ATOM(Stage,_setColor)
{
	Stage* th=asAtomHandler::as<Stage>(obj);
	uint32_t color;
	ARG_CHECK(ARG_UNPACK(color));
	RGB rgb(color);
	RootMovieClip* root = th->getRoot();
	if (root)
		root->setBackground(rgb);
}


void StageScaleMode::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("EXACT_FIT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"exactFit"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NO_BORDER",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"noBorder"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NO_SCALE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"noScale"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("SHOW_ALL",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"showAll"),CONSTANT_TRAIT);
}

void StageAlign::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("BOTTOM",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"B"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("BOTTOM_LEFT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"BL"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("BOTTOM_RIGHT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"BR"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("LEFT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"L"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("RIGHT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"R"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("TOP",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"T"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("TOP_LEFT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"TL"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("TOP_RIGHT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"TR"),CONSTANT_TRAIT);
}

void StageQuality::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("BEST",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"best"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("HIGH",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"high"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("LOW",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"low"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("MEDIUM",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"medium"),CONSTANT_TRAIT);

	c->setVariableAtomByQName("HIGH_16X16",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"16x16"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("HIGH_16X16_LINEAR",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"16x16linear"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("HIGH_8X8",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"8x8"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("HIGH_8X8_LINEAR",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"8x8linear"),CONSTANT_TRAIT);
}

void StageDisplayState::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("FULL_SCREEN",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"fullScreen"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("FULL_SCREEN_INTERACTIVE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"fullScreenInteractive"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NORMAL",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"normal"),CONSTANT_TRAIT);
}
