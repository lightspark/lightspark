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

#include "scripting/flash/display/Stage.h"
#include "scripting/flash/display/RootMovieClip.h"
#include "scripting/flash/display/Stage3D.h"
#include "scripting/flash/display/LoaderInfo.h"
#include "scripting/flash/display3d/flashdisplay3d.h"
#include "scripting/flash/geom/Rectangle.h"
#include "scripting/flash/media/flashmedia.h"
#include "scripting/flash/ui/keycodes.h"
#include "scripting/toplevel/AVM1Function.h"
#include "scripting/toplevel/Boolean.h"
#include "scripting/toplevel/Number.h"
#include "scripting/toplevel/UInteger.h"
#include "scripting/toplevel/Vector.h"
#include "scripting/class.h"
#include "scripting/abc.h"
#include "scripting/argconv.h"
#include "backends/rendering.h"
#include "backends/cachedsurface.h"
#include "backends/input.h"
#include "platforms/engineutils.h"
#include <algorithm>

using namespace std;
using namespace lightspark;

void Stage::sinit(Class_base* c)
{
	CLASS_SETUP(c, DisplayObjectContainer, _constructor, CLASS_SEALED);
	c->setDeclaredMethodByQName("allowFullScreen","",c->getSystemState()->getBuiltinFunction(_getAllowFullScreen,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("allowFullScreenInteractive","",c->getSystemState()->getBuiltinFunction(_getAllowFullScreenInteractive,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("colorCorrectionSupport","",c->getSystemState()->getBuiltinFunction(_getColorCorrectionSupport,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("fullScreenHeight","",c->getSystemState()->getBuiltinFunction(_getStageHeight,0,Class<UInteger>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("fullScreenWidth","",c->getSystemState()->getBuiltinFunction(_getStageWidth,0,Class<UInteger>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("stageWidth","",c->getSystemState()->getBuiltinFunction(_getStageWidth,0,Class<UInteger>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("stageWidth","",c->getSystemState()->getBuiltinFunction(_setStageWidth),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("stageHeight","",c->getSystemState()->getBuiltinFunction(_getStageHeight,0,Class<UInteger>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("stageHeight","",c->getSystemState()->getBuiltinFunction(_setStageHeight),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("width","",c->getSystemState()->getBuiltinFunction(_getStageWidth,0,Class<UInteger>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("height","",c->getSystemState()->getBuiltinFunction(_getStageHeight),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("scaleMode","",c->getSystemState()->getBuiltinFunction(_getScaleMode,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("scaleMode","",c->getSystemState()->getBuiltinFunction(_setScaleMode),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("loaderInfo","",c->getSystemState()->getBuiltinFunction(_getLoaderInfo,0,Class<LoaderInfo>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("stageVideos","",c->getSystemState()->getBuiltinFunction(_getStageVideos,0,Class<Vector>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("focus","",c->getSystemState()->getBuiltinFunction(_getFocus,0,Class<InteractiveObject>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("focus","",c->getSystemState()->getBuiltinFunction(_setFocus),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("frameRate","",c->getSystemState()->getBuiltinFunction(_getFrameRate,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("frameRate","",c->getSystemState()->getBuiltinFunction(_setFrameRate),SETTER_METHOD,true);

	// override the setter from DisplayObjectContainer
	c->setDeclaredMethodByQName("tabChildren","",c->getSystemState()->getBuiltinFunction(_setTabChildren),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("wmodeGPU","",c->getSystemState()->getBuiltinFunction(_getWmodeGPU,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("invalidate","",c->getSystemState()->getBuiltinFunction(_invalidate),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("color","",c->getSystemState()->getBuiltinFunction(_getColor,0,Class<UInteger>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("color","",c->getSystemState()->getBuiltinFunction(_setColor),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("isFocusInaccessible","",c->getSystemState()->getBuiltinFunction(_isFocusInaccessible,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	
	REGISTER_GETTER_SETTER_RESULTTYPE(c,align,ASString);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,colorCorrection,ASString);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,displayState,ASString);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,fullScreenSourceRect,Rectangle);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,showDefaultContextMenu,Boolean);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,quality,ASString);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,stageFocusRect,Boolean);
	REGISTER_GETTER_RESULTTYPE(c,allowsFullScreen,Boolean);
	REGISTER_GETTER_RESULTTYPE(c,stage3Ds,Vector);
	REGISTER_GETTER_RESULTTYPE(c,softKeyboardRect,Rectangle);
	REGISTER_GETTER_RESULTTYPE(c,contentsScaleFactor,Number);
	REGISTER_GETTER_RESULTTYPE(c,nativeWindow,NativeWindow);

	if(c->getSystemState()->flashMode==SystemState::AIR) 
	{
		c->setDeclaredMethodByQName("setAspectRatio","",c->getSystemState()->getBuiltinFunction(setAspectRatio),NORMAL_METHOD,true);
	}
}

ASFUNCTIONBODY_GETTER_SETTER_STRINGID_CB(Stage,align,onAlign)
ASFUNCTIONBODY_GETTER_SETTER_CB(Stage,colorCorrection,onColorCorrection)
ASFUNCTIONBODY_GETTER_SETTER_CB(Stage,displayState,onDisplayState)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(Stage,showDefaultContextMenu)
ASFUNCTIONBODY_GETTER_SETTER_CB(Stage,fullScreenSourceRect,onFullScreenSourceRect)
ASFUNCTIONBODY_GETTER_SETTER(Stage,quality)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(Stage,stageFocusRect)
ASFUNCTIONBODY_GETTER_NOT_IMPLEMENTED(Stage,allowsFullScreen)
ASFUNCTIONBODY_GETTER(Stage,stage3Ds)
ASFUNCTIONBODY_GETTER_NOT_IMPLEMENTED(Stage,softKeyboardRect)
ASFUNCTIONBODY_GETTER_NOT_IMPLEMENTED(Stage,contentsScaleFactor)
ASFUNCTIONBODY_GETTER(Stage,nativeWindow)
ASFUNCTIONBODY_ATOM(Stage,setAspectRatio)
{
	LOG(LOG_NOT_IMPLEMENTED,"flash.display.Stage.setAspectRatio is stubbed");
}

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

void Stage::onAlign(uint32_t /*oldValue*/)
{
	LOG(LOG_NOT_IMPLEMENTED, "Stage.align = " << getSystemState()->getStringFromUniqueId(align));
}

void Stage::onColorCorrection(const tiny_string& oldValue)
{
	if (colorCorrection != "default" && 
	    colorCorrection != "on" && 
	    colorCorrection != "off")
	{
		colorCorrection = oldValue;
		createError<ArgumentError>(this->getInstanceWorker(),kInvalidEnumError, "colorCorrection");
	}
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
	if (e->type == "keyDown")
	{
		KeyboardEvent* ev = e->as<KeyboardEvent>();
		uint32_t modifiers = ev->getModifiers() & (KMOD_LSHIFT | KMOD_RSHIFT |KMOD_LCTRL | KMOD_RCTRL | KMOD_LALT | KMOD_RALT);
		if (modifiers == KMOD_NONE)
		{
			switch (ev->getKeyCode())
			{
				case AS3KEYCODE_ESCAPE:
					if (getSystemState()->getEngineData()->inFullScreenMode())
						getSystemState()->getEngineData()->setDisplayState("normal",getSystemState());
					break;
				default:
					break;
			}
		}
	}
}

void Stage::eventListenerAdded(const tiny_string& eventName)
{
	if (eventName == "stageVideoAvailability")
	{
		// StageVideoAvailabilityEvent is dispatched directly after an eventListener is added
		// see https://www.adobe.com/devnet/flashplayer/articles/stage_video.html 
		this->incRef();
		getVm(getSystemState())->addEvent(_MR(this),_MR(Class<StageVideoAvailabilityEvent>::getInstanceS(getInstanceWorker())));
	}
}
bool Stage::renderStage3D()
{
	for (uint32_t i = 0; i < stage3Ds->size(); i++)
	{
		asAtom a=stage3Ds->at(i);
		if (!asAtomHandler::as<Stage3D>(a)->context3D.isNull()
				&& asAtomHandler::as<Stage3D>(a)->context3D->backBufferHeight != 0
				&& asAtomHandler::as<Stage3D>(a)->context3D->backBufferWidth != 0
				&& asAtomHandler::as<Stage3D>(a)->visible)
			return true;
	}
	return false;
}
void Stage::render(RenderContext &ctxt,const MATRIX* startmatrix)
{
	bool has3d = false;
	for (uint32_t i = 0; i < stage3Ds->size(); i++)
	{
		asAtom a=stage3Ds->at(i);
		if (asAtomHandler::as<Stage3D>(a)->renderImpl(ctxt))
			has3d = true;
	}
	if (has3d)
	{
		// setup opengl state for additional 2d rendering
		getSystemState()->getEngineData()->exec_glActiveTexture_GL_TEXTURE0(SAMPLEPOSITION::SAMPLEPOS_STANDARD);
		getSystemState()->getEngineData()->exec_glBlendFunc(BLEND_ONE,BLEND_ONE_MINUS_SRC_ALPHA);
		getSystemState()->getEngineData()->exec_glUseProgram(((RenderThread&)ctxt).gpu_program);
		getSystemState()->getEngineData()->exec_glViewport(0,0,getSystemState()->getRenderThread()->windowWidth,getSystemState()->getRenderThread()->windowHeight);

		((GLRenderContext&)ctxt).lsglLoadIdentity();
		((GLRenderContext&)ctxt).setMatrixUniform(GLRenderContext::LSGL_MODELVIEW);
	}
	this->getCachedSurface()->Render(getSystemState(),ctxt,startmatrix);
}
bool Stage::destruct()
{
	cleanupDeadHiddenObjects();
	cleanupRemovedDisplayObjects();
	focus.reset();
	root.reset();
	stage3Ds.reset();
	nativeWindow.reset();
	forEachHiddenObject([&](DisplayObject* obj)
	{
		obj->decRef();
	}, true);
	hiddenobjects.clear();
	fullScreenSourceRect.reset();
	softKeyboardRect.reset();
	
	avm1KeyboardListeners.clear();
	avm1MouseListeners.clear();
	avm1EventListeners.clear();
	avm1ResizeListeners.clear();

	return DisplayObjectContainer::destruct();
}

void Stage::finalize()
{
	focus.reset();
	root.reset();
	stage3Ds.reset();
	nativeWindow.reset();
	forEachHiddenObject([&](DisplayObject* obj)
	{
		obj->decRef();
	}, true);
	hiddenobjects.clear();
	fullScreenSourceRect.reset();
	softKeyboardRect.reset();
	
	avm1KeyboardListeners.clear();
	avm1MouseListeners.clear();
	avm1EventListeners.clear();
	avm1ResizeListeners.clear();

	DisplayObjectContainer::finalize();
}

bool Stage::countCylicMemberReferences(garbagecollectorstate& gcstate)
{
	if (gcstate.checkAncestors(this))
		return false;
	bool ret = DisplayObjectContainer::countCylicMemberReferences(gcstate);
	for (auto it = hiddenobjects.begin(); it != hiddenobjects.end(); it++)
		ret = (*it)->countAllCylicMemberReferences(gcstate) || ret;
	for (auto it = removedDisplayObjects.begin(); it != removedDisplayObjects.end(); it++)
		ret = (*it)->countAllCylicMemberReferences(gcstate) || ret;
	return ret;
}

void Stage::prepareShutdown()
{
	if (this->preparedforshutdown)
		return;
	DisplayObjectContainer::prepareShutdown();
	if (fullScreenSourceRect)
		fullScreenSourceRect->prepareShutdown();
	if (stage3Ds)
		stage3Ds->prepareShutdown();
	if (softKeyboardRect)
		softKeyboardRect->prepareShutdown();
	if (nativeWindow)
		nativeWindow->prepareShutdown();
	if (focus)
		focus->prepareShutdown();
	if (root)
		root->prepareShutdown();
	forEachHiddenObject([&](DisplayObject* obj)
	{
		obj->prepareShutdown();
	}, true);
	avm1KeyboardListeners.clear();
	avm1MouseListeners.clear();
	avm1EventListeners.clear();
	avm1ResizeListeners.clear();
}

Stage::Stage(ASWorker* wrk, Class_base* c):DisplayObjectContainer(wrk,c)
	,avm1DisplayObjectFirst(nullptr),avm1DisplayObjectLast(nullptr),hasAVM1Clips(false),invalidated(true)
	,align(c->getSystemState()->getUniqueStringId("TL")), colorCorrection("default"),displayState("normal"),showDefaultContextMenu(true),quality("high")
	,stageFocusRect(false),allowsFullScreen(false),contentsScaleFactor(1.0)
{
	subtype = SUBTYPE_STAGE;
	RELEASE_WRITE(this->invalidated,false);
	onStage = true;
	asAtom v=asAtomHandler::invalidAtom;
	RootMovieClip* root = wrk->rootClip.getPtr();
	Template<Vector>::getInstanceS(wrk,v,root,Class<Stage3D>::getClass(getSystemState()),NullRef);
	stage3Ds = _R<Vector>(asAtomHandler::as<Vector>(v));
	stage3Ds->setRefConstant();
	// according to specs, Desktop computers usually have 4 Stage3D objects available
	ASObject* o = Class<Stage3D>::getInstanceS(wrk);
	o->setRefConstant();
	v =asAtomHandler::fromObject(o);
	stage3Ds->append(v);

	o = Class<Stage3D>::getInstanceS(wrk);
	o->setRefConstant();
	v =asAtomHandler::fromObject(o);
	stage3Ds->append(v);

	o = Class<Stage3D>::getInstanceS(wrk);
	o->setRefConstant();
	v =asAtomHandler::fromObject(o);
	stage3Ds->append(v);

	o = Class<Stage3D>::getInstanceS(wrk);
	o->setRefConstant();
	v =asAtomHandler::fromObject(o);
	stage3Ds->append(v);

	softKeyboardRect = _R<Rectangle>(Class<Rectangle>::getInstanceS(wrk));
	if (wrk->getSystemState()->flashMode == SystemState::AIR)
	{
		nativeWindow = _MR(Class<NativeWindow>::getInstanceSNoArgs(wrk));
		nativeWindow->setRefConstant();
	}
}

_NR<Stage> Stage::getStage()
{
	this->incRef();
	return _MR(this);
}

ASFUNCTIONBODY_ATOM(Stage,_constructor)
{
}

_NR<DisplayObject> Stage::hitTestImpl(const Vector2f& globalPoint, const Vector2f& localPoint, HIT_TYPE type,bool interactiveObjectsOnly)
{
	_NR<DisplayObject> ret;
	ret = DisplayObjectContainer::hitTestImpl(globalPoint, localPoint, type, interactiveObjectsOnly);
	if(!ret)
	{
		/* If nothing else is hit, we hit the stage */
		this->incRef();
		ret = _MNR(this);
	}
	return ret;
}

_NR<RootMovieClip> Stage::getRoot()
{
	return root;
}

void Stage::setRoot(_NR<RootMovieClip> _root)
{
	root = _root;
}

uint32_t Stage::internalGetWidth() const
{
	uint32_t width;
	if (this->fullScreenSourceRect)
		width=this->fullScreenSourceRect->width;
	else if(getSystemState()->scaleMode==SystemState::NO_SCALE)
		width=getSystemState()->getRenderThread()->windowWidth;
	else
	{
		RECT size=getSystemState()->mainClip->getFrameSize();
		width=(size.Xmax-size.Xmin)/20;
	}
	return width;
}

uint32_t Stage::internalGetHeight() const
{
	uint32_t height;
	if (this->fullScreenSourceRect)
		height=this->fullScreenSourceRect->height;
	else if(getSystemState()->scaleMode==SystemState::NO_SCALE)
		height=getSystemState()->getRenderThread()->windowHeight;
	else
	{
		RECT size=getSystemState()->mainClip->getFrameSize();
		height=(size.Ymax-size.Ymin)/20;
	}
	return height;
}

ASFUNCTIONBODY_ATOM(Stage,_getStageWidth)
{
	asAtomHandler::setUInt(ret,wrk,wrk->getSystemState()->stage->internalGetWidth());
}

ASFUNCTIONBODY_ATOM(Stage,_setStageWidth)
{
	//Stage* th=asAtomHandler::as<Stage>(obj);
	LOG(LOG_NOT_IMPLEMENTED,"Stage.stageWidth setter");
}

ASFUNCTIONBODY_ATOM(Stage,_getStageHeight)
{
	asAtomHandler::setUInt(ret,wrk,wrk->getSystemState()->stage->internalGetHeight());
}

ASFUNCTIONBODY_ATOM(Stage,_setStageHeight)
{
	//Stage* th=asAtomHandler::as<Stage>(obj);
	LOG(LOG_NOT_IMPLEMENTED,"Stage.stageHeight setter");
}

ASFUNCTIONBODY_ATOM(Stage,_getLoaderInfo)
{
	asAtom a = asAtomHandler::fromObject(wrk->getSystemState()->mainClip);
	RootMovieClip::_getLoaderInfo(ret,wrk,a,nullptr,0);
}

ASFUNCTIONBODY_ATOM(Stage,_getScaleMode)
{
	//Stage* th=asAtomHandler::as<Stage>(obj);
	switch(wrk->getSystemState()->scaleMode)
	{
		case SystemState::EXACT_FIT:
			ret = asAtomHandler::fromString(wrk->getSystemState(),"exactFit");
			return;
		case SystemState::SHOW_ALL:
			ret = asAtomHandler::fromString(wrk->getSystemState(),"showAll");
			return;
		case SystemState::NO_BORDER:
			ret = asAtomHandler::fromString(wrk->getSystemState(),"noBorder");
			return;
		case SystemState::NO_SCALE:
			ret = asAtomHandler::fromString(wrk->getSystemState(),"noScale");
			return;
	}
}

ASFUNCTIONBODY_ATOM(Stage,_setScaleMode)
{
	Stage* th=asAtomHandler::as<Stage>(obj);
	const tiny_string& arg0=asAtomHandler::toString(args[0],wrk);
	SystemState::SCALE_MODE oldScaleMode = wrk->getSystemState()->scaleMode;
	if(arg0=="exactFit")
		wrk->getSystemState()->scaleMode=SystemState::EXACT_FIT;
	else if(arg0=="showAll")
		wrk->getSystemState()->scaleMode=SystemState::SHOW_ALL;
	else if(arg0=="noBorder")
		wrk->getSystemState()->scaleMode=SystemState::NO_BORDER;
	else if(arg0=="noScale")
		wrk->getSystemState()->scaleMode=SystemState::NO_SCALE;
	if (oldScaleMode != wrk->getSystemState()->scaleMode && th->fullScreenSourceRect.isNull())
	{
		RenderThread* rt=wrk->getSystemState()->getRenderThread();
		rt->requestResize(UINT32_MAX, UINT32_MAX, true);
	}
}

ASFUNCTIONBODY_ATOM(Stage,_getStageVideos)
{
	LOG(LOG_NOT_IMPLEMENTED, "Accelerated rendering through StageVideo not implemented, SWF should fall back to Video");
	RootMovieClip* root = wrk->rootClip.getPtr();
	Template<Vector>::getInstanceS(wrk,ret,root,Class<StageVideo>::getClass(wrk->getSystemState()),NullRef);
}

ASFUNCTIONBODY_ATOM(Stage,_isFocusInaccessible)
{
	LOG(LOG_NOT_IMPLEMENTED,"Stage.isFocusInaccessible always returns false");
	ret = asAtomHandler::falseAtom;
}

_NR<InteractiveObject> Stage::getFocusTarget()
{
	Locker l(focusSpinlock);
	if (focus.isNull() || !focus->isOnStage() || !focus->isVisible())
	{
		incRef();
		return _MNR(this);
	}
	else
	{
		return focus;
	}
}

void Stage::setFocusTarget(_NR<InteractiveObject> f)
{
	Locker l(focusSpinlock);
	if (focus)
	{
		focus->lostFocus();
		getVm(getSystemState())->addEvent(_MR(focus),_MR(Class<FocusEvent>::getInstanceS(getInstanceWorker(),"focusOut")));
	}
	focus = f;
	if (focus)
	{
		focus->gotFocus();
		getVm(getSystemState())->addEvent(_MR(focus),_MR(Class<FocusEvent>::getInstanceS(getInstanceWorker(),"focusIn")));
	}
}

void Stage::checkResetFocusTarget(InteractiveObject* removedtarget)
{
	Locker l(focusSpinlock);
	if (focus.getPtr() == removedtarget)
		focus=NullRef;
}

void Stage::addHiddenObject(DisplayObject* o)
{
	if (!o->getInstanceWorker()->isPrimordial)
		return;
	auto it = hiddenobjects.find(o);
	if (it != hiddenobjects.end())
		return;
	// don't add hidden object if any ancestor was already added as one
	DisplayObject* p=o->getParent();
	while (p)
	{
		auto itp = hiddenobjects.find(p);
		if (itp != hiddenobjects.end())
			return;
		p=p->getParent();
	}
	o->incRef();
	hiddenobjects.insert(o);
}

void Stage::removeHiddenObject(DisplayObject* o)
{
	auto it = hiddenobjects.find(o);
	if (it != hiddenobjects.end())
	{
		hiddenobjects.erase(it);
		o->decRef();
	}
}

void Stage::forEachHiddenObject(std::function<void(DisplayObject*)> callback, bool allowInvalid)
{
	unordered_set<DisplayObject*> tmp = hiddenobjects; // work on copy as hidden object list may be altered during calls
	for (auto it : tmp)
	{
		if (allowInvalid || it->getParent() == nullptr)
			callback(it);
	}
}

void Stage::cleanupDeadHiddenObjects()
{
	auto it = hiddenobjects.begin();
	while (it != hiddenobjects.end())
	{
		DisplayObject* clip = *it;
		// NOTE: Objects that are removed by ActionScript are only
		//       removed from the hidden object list if this is the last reference (so they are not reachable by code anymore).
		if (clip->placedByActionScript && clip->getParent() == nullptr)
		{
			if (clip->isLastRef())
			{
				clip->decRef();
				it = hiddenobjects.erase(it);
			}
			else
			{
				if (clip->handleGarbageCollection())
				{
					it = hiddenobjects.erase(it);
				}
				else
				{
					clip->incRef();
					++it;
				}
			}
		}
		else if (clip->getParent() != nullptr)
		{
			clip->decRef();
			it = hiddenobjects.erase(it);
		}
		else
			++it;
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
	Locker l(DisplayObjectRemovedMutex);
	auto it = removedDisplayObjects.begin();
	while (it != removedDisplayObjects.end())
	{
		(*it)->removeStoredMember();
		it = removedDisplayObjects.erase(it);
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

void Stage::enterFrame(bool implicit)
{
	forEachHiddenObject([&](DisplayObject* obj)
	{
		obj->advanceFrame(implicit);
	});
	std::vector<_R<DisplayObject>> list;
	cloneDisplayList(list);
	for (auto child : list)
		child->enterFrame(implicit);
	executeAVM1Scripts(implicit);
}

void Stage::advanceFrame(bool implicit)
{
	if (getSystemState()->mainClip->usesActionScript3)
	{
		forEachHiddenObject([&](DisplayObject* obj)
		{
			obj->advanceFrame(implicit);
		});
		DisplayObjectContainer::advanceFrame(implicit);
	}
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
			if ((*itscr).isEventScript)
				(*itscr).clip->AVM1EventScriptsAdded=false;
			if ((*itscr).clip->isOnStage())
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
		if (e->type =="keyDown")
		{
			getSystemState()->getInputThread()->setLastKeyDown(e->as<KeyboardEvent>());
		}
		else if (e->type =="keyUp")
		{
			getSystemState()->getInputThread()->setLastKeyUp(e->as<KeyboardEvent>());
		}
		avm1listenerMutex.lock();
		vector<ASObject*> tmplisteners = avm1KeyboardListeners;
		for (auto it = tmplisteners.begin(); it != tmplisteners.end(); it++)
			(*it)->incRef();
		avm1listenerMutex.unlock();
		// eventhandlers may change the listener list, so we work on a copy
		bool handled = false;
		auto it = tmplisteners.rbegin();
		while (it != tmplisteners.rend())
		{
			if (!handled && (*it)->AVM1HandleKeyboardEvent(e->as<KeyboardEvent>()))
				handled=true;
			(*it)->decRef();
			it++;
		}
	}
	else if (e->is<MouseEvent>())
	{
		avm1listenerMutex.lock();
		vector<ASObject*> tmplisteners = avm1MouseListeners;
		for (auto it = tmplisteners.begin(); it != tmplisteners.end(); it++)
			(*it)->incRef();
		avm1listenerMutex.unlock();
		// eventhandlers may change the listener list, so we work on a copy
		auto it = tmplisteners.rbegin();
		while (it != tmplisteners.rend())
		{
			(*it)->AVM1HandleMouseEvent(dispatcher, e->as<MouseEvent>());
			(*it)->decRef();
			it++;
		}
	}
	else
	{
		avm1listenerMutex.lock();
		vector<ASObject*> tmplisteners = avm1EventListeners;
		avm1listenerMutex.unlock();
		// eventhandlers may change the listener list, so we work on a copy
		auto it = tmplisteners.rbegin();
		while (it != tmplisteners.rend())
		{
			(*it)->incRef();
			(*it)->AVM1HandleEvent(dispatcher, e);
			(*it)->decRef();
			it++;
		}
		if (!avm1ResizeListeners.empty() && dispatcher==this && e->type=="resize")
		{
			avm1listenerMutex.lock();
			vector<ASObject*> tmplisteners = avm1ResizeListeners;
			for (auto it = tmplisteners.begin(); it != tmplisteners.end(); it++)
				(*it)->incRef();
			avm1listenerMutex.unlock();
			// eventhandlers may change the listener list, so we work on a copy
			auto it = tmplisteners.rbegin();
			while (it != tmplisteners.rend())
			{
				asAtom func=asAtomHandler::invalidAtom;
				multiname m(nullptr);
				m.name_type=multiname::NAME_STRING;
				m.isAttribute = false;
				m.name_s_id=getSystemState()->getUniqueStringId("onResize");
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

void Stage::AVM1AddKeyboardListener(ASObject *o)
{
	Locker l(avm1listenerMutex);
	for (auto it = avm1KeyboardListeners.begin(); it != avm1KeyboardListeners.end(); it++)
	{
		if ((*it) == o)
			return;
	}
	avm1KeyboardListeners.push_back(o);
}

void Stage::AVM1RemoveKeyboardListener(ASObject *o)
{
	Locker l(avm1listenerMutex);
	for (auto it = avm1KeyboardListeners.begin(); it != avm1KeyboardListeners.end(); it++)
	{
		if ((*it) == o)
		{
			avm1KeyboardListeners.erase(it);
			break;
		}
	}
}
void Stage::AVM1AddMouseListener(ASObject *o)
{
	Locker l(avm1listenerMutex);
	auto it = std::find_if(avm1MouseListeners.begin(), avm1MouseListeners.end(), [&](ASObject* obj)
	{
		if (obj == o)
			return true;
		if (o->is<DisplayObject>() && obj->is<DisplayObject>())
		{
			DisplayObject* dispA = o->as<DisplayObject>();
			DisplayObject* dispB = obj->as<DisplayObject>();

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
	if (it != avm1MouseListeners.end() && (*it) == o)
		return;
	if (it != avm1MouseListeners.end())
		avm1MouseListeners.insert(it, o);
	else
		avm1MouseListeners.push_back(o);
}

void Stage::AVM1RemoveMouseListener(ASObject *o)
{
	Locker l(avm1listenerMutex);
	for (auto it = avm1MouseListeners.begin(); it != avm1MouseListeners.end(); it++)
	{
		if ((*it) == o)
		{
			avm1MouseListeners.erase(it);
			break;
		}
	}
}
void Stage::AVM1AddEventListener(ASObject *o)
{
	Locker l(avm1listenerMutex);
	for (auto it = avm1EventListeners.begin(); it != avm1EventListeners.end(); it++)
	{
		if ((*it) == o)
			return;
	}
	avm1EventListeners.push_back(o);
}
void Stage::AVM1RemoveEventListener(ASObject *o)
{
	Locker l(avm1listenerMutex);
	for (auto it = avm1EventListeners.begin(); it != avm1EventListeners.end(); it++)
	{
		if ((*it) == o)
		{
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

ASFUNCTIONBODY_ATOM(Stage,_getFocus)
{
	Stage* th=asAtomHandler::as<Stage>(obj);
	_NR<InteractiveObject> focus = th->getFocusTarget();
	if (focus.isNull())
	{
		return;
	}
	else
	{
		focus->incRef();
		ret = asAtomHandler::fromObject(focus.getPtr());
	}
}

ASFUNCTIONBODY_ATOM(Stage,_setFocus)
{
	Stage* th=asAtomHandler::as<Stage>(obj);
	_NR<InteractiveObject> focus;
	ARG_CHECK(ARG_UNPACK(focus));
	th->setFocusTarget(focus);
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
	_NR<RootMovieClip> root = th->getRoot();
	if (root.isNull())
		asAtomHandler::setNumber(ret,wrk,wrk->getSystemState()->mainClip->getFrameRate());
	else
		asAtomHandler::setNumber(ret,wrk,root->getFrameRate());
}

ASFUNCTIONBODY_ATOM(Stage,_setFrameRate)
{
	Stage* th=asAtomHandler::as<Stage>(obj);
	number_t frameRate;
	ARG_CHECK(ARG_UNPACK(frameRate));
	_NR<RootMovieClip> root = th->getRoot();
	if (!root.isNull())
		root->setFrameRate(frameRate);
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
	_NR<RootMovieClip> root = th->getRoot();
	if (!root.isNull())
		rgb = root->getBackground();
	asAtomHandler::setUInt(ret,wrk,rgb.toUInt());
}

ASFUNCTIONBODY_ATOM(Stage,_setColor)
{
	Stage* th=asAtomHandler::as<Stage>(obj);
	uint32_t color;
	ARG_CHECK(ARG_UNPACK(color));
	RGB rgb(color);
	_NR<RootMovieClip> root = th->getRoot();
	if (!root.isNull())
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
