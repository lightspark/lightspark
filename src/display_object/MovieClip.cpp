/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2013  Alessandro Pignotti (a.pignotti@sssup.it)
    Copyright (C) 2023-2026  mr b0nk 500 (b0nk@b0nk.xyz)

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
#include <array>
#include <list>

#include "display_object/MovieClip.h"
#include "gc/context.h"
#include "gc/ptr.h"
#include "tiny_string.h"
#include "utils/optional.h"

#include "scripting/flash/display/flashdisplay.h"
#include "scripting/flash/display/FrameContainer.h"
#include "scripting/flash/display/RootMovieClip.h"
#include "parsing/tags.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include "scripting/avm1/avm1display.h"
#include "scripting/avm1/avm1text.h"
#include "scripting/flash/display/Loader.h"
#include "scripting/flash/geom/flashgeom.h"
#include "scripting/flash/geom/Point.h"
#include "scripting/flash/geom/Rectangle.h"
#include "scripting/toplevel/AVM1Function.h"
#include "scripting/toplevel/Array.h"
#include "scripting/toplevel/Integer.h"
#include "scripting/flash/ui/keycodes.h"

using namespace lightspark;

#define AVM_MAX_DEPTH 2130706428

MovieClip::MovieClip
(
	SystemState* sys,
	SWFMovie& _movie,
	FrameContainer* f,
	DefineSpriteTag* _tag,
	LoaderInfo* _loaderInfo,
	Optional<const tiny_string&> name
) :
InteractiveObject(Type::MovieClip, sys, name),
TokenContainer(this),
movie(_movie),
tag(_tag),
loaderInfo(_loaderInfo),
hitTarget(nullptr),
hitArea(nullptr),
dropTarget(nullptr),
graphics(nullptr),
soundStream(nullptr),
soundStartFrame(-1),
streamingSound(false),
clipFlags(0),
totalFrames(f != nullptr ? f->getFramesLoaded() : 1),
lastFrameScriptExecuted(UINT32_MAX),
lastRatio(0),
buttonMode(false),
avm2Enabled(true),
avm2UseHandCursor(true),
frameContainer(f),
actions(nullptr)
{
}

void MovieClip::refreshSurfaceState()
{
	if (graphics != nullptr)
		graphics->refreshSurfaceState();
}

IDrawable* MovieClip::invalidate(bool smoothing)
{
	auto trySetupChildrenList = [&](IDrawable* drawable)
	{
		if (drawable != nullptr)
			return drawable;

		Locker l(mutexDisplayList);
		drawable->getState()->setupChildrenList(dynamicDisplayList);
		return drawable;
	};

	auto ret = trySetupChildrenList(getFilterDrawable(smoothing));
	if (ret != nullptr)
		return ret;

	if (graphics == nullptr || !graphics->hasBounds())
		return DisplayObjectContainer::invalidate(smoothing);
	return trySetupChildrenList(graphics->invalidate
	(
		smoothing ?
		SMOOTH_MODE::SMOOTH_ANTIALIAS :
		SMOOTH_MODE::SMOOTH_NONE
	));
}

Optional<Rect<Twips>> MovieClip::tryBoundsRect(bool visibleOnly)
{
	if (visibleOnly && !isVisible())
		return {};

	auto ret = DisplayObjectContainer::tryBoundsRect(visibleOnly);
	if (graphics == nullptr || !graphics->hasBounds())
		return ret;

	if (!ret.hasValue())
		return graphics->boundsRect();

	return graphics->boundsRect().andThen([&](const auto& rect)
	{
		return makeOptional(ret->_union(rect));
	});
}

Optional<Rect<Twips>> MovieClip::tryBoundsRectWithoutChildren(bool visibleOnly)
{
	if (visibleOnly && !isVisible())
		return {};
	if (graphics == nullptr || !graphics->hasBounds())
		return {}
	return graphics->boundsRect();
}

void MovieClip::requestInvalidation(InvalidateQueue* q, bool forceTextureRefresh)
{
	if (graphics != nullptr && graphics->hasBounds())
	{
		requestInvalidationFilterParent(q);
		q->addToInvalidateQueue(this);
	}

	DisplayObjectContainer::requestInvalidation
	(
		q,
		forceTextureRefresh
	);
}

bool MovieClip::hitTestShapeChildren
(
	const Vector2Twips& point,
	const HitTestFlags& flags
)
{
	using SkipInvis = HitTestFlags::SkipInvisible;
	using SkipMask = HitTestFlags::SkipMask;

	Locker l(mutexDisplayList);
	ssize_t clipDepth = 0;
	for (auto& child : dynamicDisplayList)
	{
		bool pointInChild =
		(
			child.getClipDepth() <= 0 &&
			child.getDepth() >= clipDepth &&
			child.hitTestShape(point, flags)
		);

		if (pointInChild)
			return true;

		if (child.getClipDepth() <= 0)
			continue;

		clipDepth = !child.hitTestShape
		(
			point,
			SkipMask | SkipInvis
		) ? child.getClipDepth() ? 0;
	}
	return false;
}

bool MovieClip::hitTestShape
(
	const Vector2Twips& globalPoint,
	const Vector2Twips& localPoint,
	const HitTestFlags& flags
)
{
	using SkipInvis = HitTestFlags::SkipInvisible;
	using SkipChildren = HitTestFlags::SkipChildren;
	using SkipMask = HitTestFlags::SkipMask;

	const bool skipInvis = flags & SkipInvis;
	const bool skipChildren = flags & SkipChildren;
	const bool skipMask = flags & SkipMask;
	if (skipInvis && !isVisible() && getMaskee() == nullptr)
		return false;
	if (skipMask && getMaskee() != nullptr)
		return false;
	if (!boundsRectWithoutChildren(false).contains(localPoint))
		return false;

	auto localMtx = globalToLocalMatrix();
	if (!localMtx.isValid())
		return false;

	bool pointNotInMask =
	(
		getMasker() != nullptr &&
		!getMasker()->hitTestShape(globalPoint, SkipInvis)
	);

	if (pointNotInMask)
		return false;

	if (!skipChildren && hitTestShapeChildren(globalPoint, flags))
		return true;

	if (hitArea != nullptr)
		return hitArea->hitTestShape(globalPoint, flags);

	return graphics != nullptr && graphics->hitTest
	(
		localPoint,
		localMtx
	);
}

void MovieClip::resetToStart()
{
	if (soundStream != nullptr && getTagID() != UINT32_MAX)
		soundStream->reset();
}

void MovieClip::setSound(SoundInstance* sound, bool forStreaming)
{
	soundStream = sound;
	streamingSound = forStreaming;

	if (soundStream != nullptr)
		soundStream->transform = soundTransform;
}

void MovieClip::appendSound(Span<uint8_t> data, uint32_t frame)
{
	if (soundStream != nullptr)
		soundStream->appendStreamBlock(data);
	if (soundStartFrame == size_t(-1))
		soundStartFrame = frame;
}

void MovieClip::checkSound(uint32_t frame)
{
	if (!streamingSound || soundStartFrame != frame)
		return;

	if (soundStream != nullptr)
		soundStream->play();
}

void MovieClip::stopSound()
{
	if (soundStream != nullptr)
		soundStream->stop();
}

void MovieClip::markSoundFinished()
{
	if (soundStream != nullptr)
		soundStream->markFinished();
}

Graphics& MovieClip::getGraphics()
{

	// `graphics` probably isn't used that often, so create it here.
	if (graphics == nullptr)
		graphics = new Graphics(this);
	return *graphics;
}

void MovieClip::handleMouseCursor(bool rollover)
{
	hasMouse = !rollover;
	sys->setMouseCursor
	(
		rollover &&
		buttonMode &&
		avm2UseHandCursor
	);
}

void MovieClip::addFrameScript(uint16_t frame, _NR<ASObject> func)
{
	auto it = frameScripts.find(frame);
	if (it != frameScripts.end())
		it->second->removeStoredMember();

	if (func.isNull())
	{
		frameScripts.erase(frame);
		return;
	}

	func->incRef();
	func->addStoredMember();
	frameScripts.insert(frame, func);
}

void MovieClip::setPlaying()
{
	if (!state.stop_FP)
		return;

	state.stop_FP = false;
	if (!isAS3() && state.next_FP == state.FP)
	{
		if (state.FP == getFramesLoaded() - 1)
			state.next_FP = 0;
		else
			state.next_FP++;
	}

	if (isOnStage() && isAS3())
		advanceFrame(true);
	else
		sys->stage->addHiddenObject(this);
}
void MovieClip::setStopped()
{
	if (state.stop_FP)
		return;

	state.stop_FP = true;
	// if we reset state.next_FP when it was set explicitly, we get into an infinite loop (see ruffle test avm2/goto_framescript_queued)
	if (!state.explicit_FP)
		state.next_FP = state.FP;
}

void MovieClip::gotoFrame(uint16_t frame, bool stop)
{
	bool newFrame = state.FP != next_FP;

	if (!stop && !newFrame && !isAS3())
	{
		// for AVM1 gotoandplay if we are not switching to a new frame we just act like a normal "play"
		setPlaying();
		return;
	}

	state.next_FP = frame;
	state.explicit_FP = true;
	state.stop_FP = stop;

	if (newFrame && isAS3() && isExecutingFrameScript())
		state.gotoQueued = true;
	else if (newFrame)
		runGoto(true);
	else if (isAS3())
	{
		state.gotoQueued = false;
		sys->runInnerGotoFrame(this);
	}
}

void MovieClip::runGoto(bool newFrame)
{
	if (!isAS3())
	{
		advanceFrame(false);
		return;
	}

	if (!newFrame)
	{
		if (!state.creatingframe)
			lastFrameScriptExecuted = UINT32_MAX;
		setSkipFrame(false);
		advanceFrame(false);
	}

	sys->runInnerGotoFrame(this, removedFrameScripts);
}

void MovieClip::AVM1gotoFrameLabel
(
	const tiny_string& label,
	bool stop,
	bool switchPlayState
)
{
	auto dest = frameContainer->getFrameIdByLabel(label, "", state.FP);
	if (dest == FRAME_NOT_FOUND)
	{
		LOG
		(
			LOG_ERROR,
			"AVM1gotoFrameLabel: "
			"Couldn't find label " << label << " for clip: " <<
			toDebugString()
		);
		return;
	}
	AVM1gotoFrame(dest, stop, switchPlayState, true);
}

void MovieClip::AVM1gotoFrame
(
	uint16_t frame,
	bool stop,
	bool switchPlayState,
	bool advanceFrame = true
)
{
	state.next_FP = frame;
	state.explicit_FP = true;
	bool newframe = state.FP != frame;
	if (switchPlayState)
		state.stop_FP = stop;
	if (advanceFrame)
		runGoto(newframe);
}

void MovieClip::nextFrame()
{
	bool newFrame =
	(
		!hasFinishedLoading() ||
		state.FP != getFramesLoaded() - 1
	);

	state.next_FP = !newFrame ? state.FP : state.FP + 1;
	state.explicit_FP = true;
	state.stop_FP = true;

	runGoto(newFrame);
}

void MovieClip::prevFrame()
{
	bool newFrame = state.FP != 0;

	state.next_FP = !newFrame ? state.FP : state.FP - 1;
	state.explicit_FP = true;
	state.stop_FP = true;

	runGoto(newFrame);
}

void MovieClip::nextScene()
{
	if (hasFinishedLoading() && state.FP == getFramesLoaded() - 1)
		return;

	bool newFrame =
	(
		!hasFinishedLoading() ||
		state.FP != getFramesLoaded() - 1
	);

	state.next_FP = frameContainer->nextScene(state.FP);
	state.explicit_FP = true;
	state.stop_FP = false;

	runGoto(newFrame);
}

void MovieClip::prevScene()
{
	if (hasFinishedLoading() && !state.FP)
		return;

	bool newFrame = state.FP != 0;
	state.next_FP = frameContainer->prevScene(state.FP);
	state.explicit_FP = true;
	state.stop_FP = false;

	runGoto(newFrame);
}

uint16_t MovieClip::getFramesLoaded() const
{
	if (frameContainer == nullptr)
		return 1;
	return std::min(frameContainer->getFramesLoaded(), 1);
}

Span<const Scene> MovieClip::getScenes() const
{
	if (frameContainer == nullptr)
		return {};
	return frameContainer->getScenes(totalFrames);
}

Scene* MovieClip::getCurrentScene() const
{
	if (frameContainer == nullptr)
		return nullptr;
	return frameContainer->getCurrentScene(totalFrames);
}

Optional<tiny_string> MovieClip::getCurrentFrameLabel() const
{
	if (frameContainer == nullptr)
		return {};
	return frameContainer->getCurrentFrameLabel(state.FP);
}

Optional<tiny_string> MovieClip::getCurrentLabel() const
{
	if (frameContainer == nullptr)
		return {};
	return frameContainer->getCurrentLabel(state.FP);
}

std::vector<tiny_string> MovieClip::getCurrentLabels() const
{
	if (frameContainer == nullptr)
		return {};
	return frameContainer->getCurrentLabels(state.FP);
}

uint32_t MovieClip::getTagID() const
{
	return tag != nullptr ? tag->getId() ? UINT32_MAX;
}

size_t MovieClip::getTagSize() const
{
	return tag != nullptr ? tag->Header.getLength() : 0;
}

size_t MovieClip::getBytesTotal() const
{
	// Return the uncompressed SWF size for loaded SWFs.
	if (isRoot())
		return movie.getSize();
	// Otherwise, return the size of the clip's `DefineSprite` tag.
	return getTagSize();
}

size_t MovieClip::getBytesTotalCompressed() const
{
	auto compresseMovieSize = movie.getCompressedSize();
	if (isRoot())
		return compressedMovieSize;

	number_t movieSize = movie.getSize();
	number_t clipSize = getTagSize();
	return std::floor(clipSize * compressedMovieSize / movieSize);
}

size_t MovieClip::getBytesLoaded() const
{
	return movie.getBytesLoaded();
}

size_t MovieClip::getBytesLoadedCompressed() const
{
	if (!getBytesTotal())
		return 0;

	number_t bytesLoaded = getBytesLoaded();
	number_t compressedBytesTotal = getBytesTotalCompressed();
	number_t bytesTotal = getBytesTotal();
	return bytesLoaded * compressedBytesTotal / bytesTotal;
}

void MovieClip::afterTimelineCreation()
{
	lastRatio = ratio;
	InteractiveObject::afterTimelineCreation();
}

void MovieClip::afterTimelineDeletion(bool inskipping)
{
	sys->stage->AVM1RemoveMouseListener(this);
	sys->stage->AVM1RemoveKeyboardListener(this);
	InteractiveObject::afterTimelineDeletion(inskipping);
}

bool MovieClip::AVM1HandleKeyboardEvent(KeyboardEvent *e)
{
	if (getSystemState()->stage->getAVM1FocusTarget() != this)
		return false;
	if (!e->defaultPrevented
		&& (e->getKeyCode() == AS3KEYCODE_TAB)
		&& (e->getKeyCode() == AS3KEYCODE_ESCAPE)
		)
		return false;
	if (this->actions)
	{
		for (auto it = actions->ClipActionRecords.begin(); it != actions->ClipActionRecords.end(); it++)
		{
			if(it->EventFlags.ClipEventKeyDown)
				ACTIONRECORD::executeActions(this,this->AVM1getCurrentFrameContext(),it->actions,it->startactionpos);
		}
	}
	Sprite::AVM1HandleKeyboardEvent(e);
	return false;
}

bool MovieClip::AVM1HandleKeyPressedEvent(KeyboardEvent *e)
{
	if (!e->defaultPrevented)
	{
		switch (e->getKeyCode())
		{
			case AS3KEYCODE_TAB :
				return false;
			case AS3KEYCODE_ESCAPE:
				return true;
			case AS3KEYCODE_ENTER:
				return this->isFocusable(false);
			default:
				break;
		}
	}
	if (this->actions)
	{
		for (auto it = actions->ClipActionRecords.begin(); it != actions->ClipActionRecords.end(); it++)
		{
			bool exec= false;
			if (it->EventFlags.ClipEventKeyPress)
			{
				switch (it->KeyCode)
				{
					case 1:
						exec = it->KeyCode == AS3KeyCode::AS3KEYCODE_LEFT;
						break;
					case 2:
						exec = it->KeyCode == AS3KeyCode::AS3KEYCODE_RIGHT;
						break;
					case 3:
						exec = it->KeyCode == AS3KeyCode::AS3KEYCODE_HOME;
						break;
					case 4:
						exec = it->KeyCode == AS3KeyCode::AS3KEYCODE_END;
						break;
					case 5:
						exec = it->KeyCode == AS3KeyCode::AS3KEYCODE_INSERT;
						break;
					case 6:
						exec = it->KeyCode == AS3KeyCode::AS3KEYCODE_DELETE;
						break;
					case 8:
						exec = it->KeyCode == AS3KeyCode::AS3KEYCODE_BACKSPACE;
						break;
					case 13:
						exec = it->KeyCode == AS3KeyCode::AS3KEYCODE_ENTER;
						break;
					case 14:
						exec = it->KeyCode == AS3KeyCode::AS3KEYCODE_UP;
						break;
					case 15:
						exec = it->KeyCode == AS3KeyCode::AS3KEYCODE_DOWN;
						break;
					case 16:
						exec = it->KeyCode == AS3KeyCode::AS3KEYCODE_PAGE_UP;
						break;
					case 17:
						exec = it->KeyCode == AS3KeyCode::AS3KEYCODE_PAGE_DOWN;
						break;
					case 18:
						exec = it->KeyCode == AS3KeyCode::AS3KEYCODE_TAB;
						break;
					case 19:
						exec = it->KeyCode == AS3KeyCode::AS3KEYCODE_ESCAPE;
						break;
					default:
						exec = it->KeyCode == e->getCharCode();
						break;
				}
			}
			if (exec)
			{
				ACTIONRECORD::executeActions(this,this->AVM1getCurrentFrameContext(),it->actions,it->startactionpos);
				return true;
			}
		}
	}
	return false;
}

bool MovieClip::AVM1HandleMouseEvent(EventDispatcher *dispatcher, MouseEvent *e)
{
	if (!this->isOnStage() || !this->enabled)
		return false;
	if (dispatcher->is<DisplayObject>())
	{
		DisplayObject* dispobj=nullptr;
		if (dispatcher == this)
			dispobj=this;
		else
		{
			number_t x,y,xg,yg;
			// TODO: Add overloads for Vector2f.
			dispatcher->as<DisplayObject>()->localToGlobal(e->localX*TWIPS_FACTOR,e->localY*TWIPS_FACTOR,xg,yg);
			this->globalToLocal(xg,yg,x,y);
			_NR<DisplayObject> d =hitTest(Vector2f(xg,yg), Vector2f(x,y), MOUSE_CLICK_HIT,true);
			dispobj = d.getPtr();
		}
		if (actions)
		{
			for (auto it = actions->ClipActionRecords.begin(); it != actions->ClipActionRecords.end(); it++)
			{
				// according to https://docstore.mik.ua/orelly/web2/action/ch10_09.htm
				// mouseUp/mouseDown/mouseMove events are sent to all MovieClips on the Stage
				if( (e->type == "mouseDown" && it->EventFlags.ClipEventMouseDown)
					|| (e->type == "mouseUp" && it->EventFlags.ClipEventMouseUp)
					|| (e->type == "mouseMove" && it->EventFlags.ClipEventMouseMove)
					)
				{
					ACTIONRECORD::executeActions(this,this->AVM1getCurrentFrameContext(),it->actions,it->startactionpos);
				}
				if (this->dragged && it->EventFlags.ClipEventRelease && e->type == "mouseUp")
				{
					ACTIONRECORD::executeActions(this,this->AVM1getCurrentFrameContext(),it->actions,it->startactionpos);
				}
				else if( dispobj &&
					((e->type == "mouseUp" && it->EventFlags.ClipEventRelease && !this->dragged)
					|| (e->type == "mouseDown" && it->EventFlags.ClipEventPress)
					|| (e->type == "rollOver" && it->EventFlags.ClipEventRollOver)
					|| (e->type == "rollOut" && it->EventFlags.ClipEventRollOut)
					|| (e->type == "releaseOutside" && it->EventFlags.ClipEventReleaseOutside)
					))
				{
					ACTIONRECORD::executeActions(this,this->AVM1getCurrentFrameContext(),it->actions,it->startactionpos);
				}
			}
		}

		if (getSystemState()->stage->getAVM1FocusTarget() == this
			&& dispatcher != this
			&& (e->type == "mouseMove" || e->type == "mouseOver")
			)
			getSystemState()->stage->setFocusTarget(asAtomHandler::nullAtom,false,true);

		AVM1HandleMouseEventStandard(dispobj,e);
	}
	return false;
}

void MovieClip::AVM1HandleEvent(EventDispatcher *dispatcher, Event* e)
{
	if (this->actions && hasAVM1LoadEvent)
	{
		DisplayObject* p = this;
		while (p && p != dispatcher)
			p = p->getParent();
		if (p == dispatcher)
		{
			for (auto it = actions->ClipActionRecords.begin(); it != actions->ClipActionRecords.end(); it++)
			{
				if (it->EventFlags.ClipEventLoad && e->type == "complete")
				{
					ACTIONRECORD::executeActions(this,this->AVM1getCurrentFrameContext(),it->actions,it->startactionpos);
					AVM1removeOneEventListener();
				}
			}
		}
	}
	if (this->loaderInfo && dispatcher == this->loaderInfo && this->loaderInfo->getContent()==this)
	{
		if (this->actions)
		{
			for (auto it = actions->ClipActionRecords.begin(); it != actions->ClipActionRecords.end(); it++)
			{
				if (it->EventFlags.ClipEventUnload && e->type == "unload")
				{
					ACTIONRECORD::executeActions(this,this->AVM1getCurrentFrameContext(),it->actions,it->startactionpos);
				}
			}
		}
		if (e->type == "unload")
		{
			if (!this->is<DisplayObject>() || this->is<MovieClip>())
			{
				asAtom func=asAtomHandler::invalidAtom;
				multiname m(nullptr);
				m.name_type=multiname::NAME_STRING;
				m.isAttribute = false;
				asAtom ret=asAtomHandler::invalidAtom;
				asAtom obj = asAtomHandler::fromObject(this);
				ASWorker* wrk = getInstanceWorker();
				m.name_s_id=BUILTIN_STRINGS::STRING_ONUNLOAD;
				AVM1getVariableByMultiname(func,m,GET_VARIABLE_OPTION::NONE,wrk);
				if (asAtomHandler::is<AVM1Function>(func))
					asAtomHandler::as<AVM1Function>(func)->call(&ret,&obj,nullptr,0);
				ASATOM_DECREF(func);
			}
		}
	}
}

void MovieClip::AVM1AfterAdvance()
{
	if (!state.frameadvanced)
		return;
	state.frameadvanced=false;
	state.last_FP=state.FP;
	state.explicit_FP=false;
}

void MovieClip::setupActions(const CLIPACTIONS& clipActions)
{
	actions = &clipActions;

	bool hasMouseEvents =
	(
		clipActions.AllEventFlags.ClipEventMouseDown ||
		clipActions.AllEventFlags.ClipEventMouseMove ||
		clipActions.AllEventFlags.ClipEventRollOver ||
		clipActions.AllEventFlags.ClipEventRollOut ||
		clipActions.AllEventFlags.ClipEventPress ||
		clipActions.AllEventFlags.ClipEventMouseUp
	);

	bool hasKeyEvents =
	(
		clipActions.AllEventFlags.ClipEventKeyDown ||
		clipActions.AllEventFlags.ClipEventKeyUp ||
		clipActions.AllEventFlags.ClipEventKeyPress
	);

	bool hasLoadEvents =
	(
		clipActions.AllEventFlags.ClipEventLoad ||
		clipActions.AllEventFlags.ClipEventUnload
	);

	if (hasMouseEvents)
	{
		setMouseEnabled(true);
		AVM1addOneMouseEventListener();
	}

	if (hasKeyEvents)
		AVM1addOneKeyboardEventListener();

	if (hasLoadEvents)
	{
		setHasAVM1LoadEvent(true);
		AVM1addOneEventListener();
	}

	if (clipActions.AllEventFlags.ClipEventEnterFrame)
		AVM1addOneEventListener();
}

void MovieClip::forEachFrameAction()
(
	uint16_t frame,
	std::function<void(Span<uint8_t>)> func
)
{
	if (frameContainer == nullptr)
		return;
	auto frameActions = frameContainer->getFrameActions(frame);
	for (const auto& _actions : frameActions)
		func(makeSpan(_actions));
}

string MovieClip::toDebugString() const
{
	std::string res = Sprite::toDebugString();
	res += " state=";
	char buf[100];
	sprintf(buf,"%d/%u/%u%s",state.last_FP,state.FP,state.next_FP,state.stop_FP?" stopped":"");
	res += buf;
	return res;
}

void MovieClip::AVM1unloadMovie
{
	avm1Unload();
	moveToUnloadedState();
}

void MovieClip::AVM1HandleConstruction(bool forInitAction)
{
	if (needsActionScript3() || inAVM1Attachment || constructorCallComplete)
		return;
	setConstructIndicator();
	if (!forInitAction)
		getSystemState()->stage->AVM1AddDisplayObject(this);
	setConstructorCallComplete();
	AVM1Function* constr = this->getInstanceWorker()->AVM1getClassConstructor(this);
	if (constr)
	{
		asAtom pr = constr->getprop_prototypeAtom();
		if (!asAtomHandler::isObject(pr))
			pr = constr->prototype;
		setprop_prototype(pr, BUILTIN_STRINGS::STRING_PROTO);
		asAtom ret = asAtomHandler::invalidAtom;
		asAtom obj = asAtomHandler::fromObjectNoPrimitive(this);
		constr->call(&ret,&obj,nullptr,0);
		AVM1registerPrototypeListeners();
	}
	else
	{
		asAtom proto = asAtomHandler::fromObject(this->getClass()->prototype->getObj());
		setprop_prototype(proto,BUILTIN_STRINGS::STRING_PROTO);
	}

	if (this != getSystemState()->mainClip)
		afterConstruction();
}

/* Go through the hierarchy and add all
 * legacy objects which are new in the current
 * frame top-down. At the same time, call their
 * constructors in reverse order (bottom-up).
 * This is called in vm's thread context */
void MovieClip::declareFrame(bool implicit)
{
	if (state.frameadvanced && (implicit || isAS3()))
		return;
	/* Go through the list of frames.
	 * If our next_FP is after our current,
	 * we construct all frames from current
	 * to next_FP.
	 * If our next_FP is before our current,
	 * we purge all objects on the 0th frame
	 * and then construct all frames from
	 * the 0th to the next_FP.
	 * We also will run the constructor on objects that got placed and deleted
	 * before state.FP (which may get us an segfault).
	 *
	 */
	if ((int)state.FP < state.last_FP)
	{
		purgeLegacyChildren(implicit);
		resetToStart();
	}

	auto obj = toASObject();
	//Declared traits must exists before legacy objects are added
	if (!obj.isNull() && obj->getClass() != nullptr)
		obj->getClass()->setupDeclaredTraits(obj.getPtr());
	bool wasInitialized = isInitialized;
	if (!isAS3() && !state.frameadvanced && !isInitialized())
		AVM1runFrame();

	bool newFrame = (int)state.FP != state.last_FP;
	if (newFrame)
		framecontainer->declareFrame(this);
	if (!isAS3() && !state.frameadvanced && wasInitialized)
		AVM1runFrame();
	if (newFrame)
		state.frameadvanced = true;

	// remove all legacy objects that have not been handled in the PlaceObject/RemoveObject tags
	LegacyChildEraseDeletionMarked();
	if (!isAS3())
		return;
	DisplayObjectContainer::declareFrame(implicit);
}

void MovieClip::AVM1runFrame()
{
	if (isAS3())
		return;

	if (!isInitialized())
	{
		handleEvent(ClipEvent::Load);
		setInitialized(true);
	}
	else
		handleEvent(ClipEvent::EnterFrame);

	if (!state.stop_FP)
		advanceFrame(true);
}

void MovieClip::initFrame()
{
	if (!isAS3())
		return;
	if (!isInitialized() && !state.stop_FP && state.last_FP == -1)
	{
		// ensure frame is advanced in first tick after construction
		state.next_FP = 1;
	}
	/* Set last_FP to reflect the frame that we have initialized currently.
	 * This must be set before the constructor of this MovieClip is run,
	 * or it will call initFrame(). */
	state.last_FP = state.FP;

	/* call our own constructor, if necassary */
	Sprite::initFrame();
	state.creatingframe = false;
}

void MovieClip::executeFrameScript()
{
	if (!isAS3() || toASObject().isNull())
		return;
	state.explicit_FP = false;
	uint32_t f = frameScripts.count(state.FP) ? state.FP : UINT32_MAX;
	bool runFrameScript = 
	(
		f != UINT32_MAX &&
		!isMarkedForTimelineDeletion() &&
		!isExecutingFrameScript() &&
		lastFrameScriptExecuted != f &&
		frameScripts.find(f) != frameScripts.end()
	);

	if (runFrameScript)
	{
		lastFrameScriptExecuted = f;
		setExecutingFrameScript(true);
		auto root = getAVM2Root();
		root->executingFrameScriptCount++;

		auto obj = toASObject();
		auto frameScript = frameScripts[f];
		auto v = asAtomHandler::invalidAtom;
		try
		{
			asAtomHandler::callFunction
			(
				asAtomHandler::fromObject(frameScript.getPtr()),
				obj->getInstanceWorker(),
				v,
				toASAtomOrNull(),
				nullptr,
				0,
				false
			);
		}
		catch(ASObject*& e)
		{
			setStopped();
			ASATOM_DECREF(v);
			root->executingFrameScriptCount--;
			inExecuteFramescript = false;
			throw;
		}
		ASATOM_DECREF(v);
		root->executingFrameScriptCount--;
		inExecuteFramescript = false;
	}

	if (state.gotoQueued)
	{
		state.gotoQueued=false;
		runGoto(true);
	}
	DisplayObjectContainer::executeFrameScript();
}

void MovieClip::checkRatio(uint32_t ratio, bool inskipping)
{
	// according to http://wahlers.com.br/claus/blog/hacking-swf-2-placeobject-and-ratio/
	// if the ratio value is different from the previous ratio value for this MovieClip, this clip is resetted to frame 0
	if (ratio != 0 && ratio != lastratio && !state.stop_FP)
		state.next_FP = 0;
	lastRatio = ratio;
}


void MovieClip::enterFrame(bool implicit)
{
	std::vector<DisplayObject&> list;
	cloneDisplayList(list);
	for (auto it = list.rbegin(); it != list.rend(); ++it)
	{
		auto child = *it;
		child->setSkipFrame
		(
			getSkipFrame() ||
			child->getSkipFrame()
		);
		child->enterFrame(implicit);
	}

	if (skipFrame)
	{
		skipFrame = false;
		return;
	}

	if (isAS3() && !state.stop_FP)
	{
		state.inEnterFrame = true;
		advanceFrame(implicit);
		state.inEnterFrame = false;
	}
}

/* Update state.last_FP. If enough frames
 * are available, set state.FP to state.next_FP.
 * This is run in vm's thread context.
 */
void MovieClip::advanceFrame(bool implicit)
{
	bool hasAdvanced =
	(
		implicit &&
		!sys->mainClip->isAS3() &&
		state.frameadvanced &&
		state.last_FP == -1
	);

	if (hasAdvanced)
		return; // frame was already advanced after construction
	if (framecontainer == nullptr)
		framecontainer = new FrameContainer();
	if (state.last_FP != -1)
		state.advancedByTick = true;
	checkSound(state.next_FP);
	if (state.frameadvanced)
	{
		// frame was advanced more than once in one EnterFrame event, so initFrame was not called
		// set last_FP to the FP set by previous advanceFrame
		state.last_FP = state.FP;
	}
	if (isAS3() || sys->mainClip->isAS3())
		state.frameadvanced = false;
	state.creatingframe=true;
	/* A MovieClip can only have frames if
	 * 1a. It is a RootMovieClip
	 * 1b. or it is a DefineSpriteTag
	 * 2. and is exported as a subclass of MovieClip (see bindedTo)
	 */
	bool canHaveFrames = [&]
	{
		if (!isRoot() && tag == nullptr)
			return false;
		if (!isAS3())
			return true;
		auto obj = toASObject();
		if (obj.isNull())
			return false;

		auto movieClipClass = Class<ASMovieClip>::getClass(sys);
		return obj->getClass()->isSubClass(movieClipClass);
	}();

	if (!canHaveFrames)
	{
		if (int(state.FP) >= state.last_FP && !state.inEnterFrame && implicit) // no need to advance frame if we are moving backwards in the timline, as the timeline will be rebuild anyway
			DisplayObjectContainer::advanceFrame(true);
		declareFrame(implicit);
		return;
	}

	//If we have not yet loaded enough frames delay advancement
	if (state.next_FP >= getFramesLoaded())
	{
		if (!hasFinishedLoading())
			return;

		if (framecontainer->getFramesLoaded() > 1)
			LOG(LOG_ERROR,"state.next_FP >= getFramesLoaded:"<< state.next_FP<<" "<<framecontainer->getFramesLoaded() <<" "<<toDebugString()<<" "<<getTagID());
		state.next_FP = state.FP;
	}

	if (state.next_FP != state.FP)
	{
		if (!inExecuteFramescript)
			lastFrameScriptExecuted = UINT32_MAX;
		state.FP = state.next_FP;
	}

	if (implicit && !state.stop_FP && getFramesLoaded() > 0)
	{
		state.next_FP =
		(
			hasFinishedLoading() ?
			state.FP == getFramesLoaded() - 1 ? 0 :
			imin(state.FP + 1, getFramesLoaded() - 1) :
			state.FP + 1
		);
	}
	// ensure the legacy objects of the current frame are created
	if (int(state.FP) >= state.last_FP && !state.inEnterFrame && implicit) // no need to advance frame if we are moving backwards in the timeline, as the timeline will be rebuild anyway
		DisplayObjectContainer::advanceFrame(true);

	declareFrame(implicit);
	// setting state.frameadvanced ensures that the frame is not declared multiple times
	// if it was set by an actionscript command.
	state.frameadvanced = true;
	setMarkedForTimelineDeletion(false);
}

void MovieClip::constructionComplete(bool _explicit, bool forInitAction)
{
	Sprite::constructionComplete(_explicit,forInitAction);

	/* If this object was 'new'ed from AS code, the first
	 * frame has not been initalized yet, so init the frame
	 * now */
	if (!framecontainer)
		framecontainer=new FrameContainer();
	if(state.last_FP == -1
		&& !needsActionScript3()
		&& !forInitAction)
	{
		if (!framecontainer)
			framecontainer=new FrameContainer();
		advanceFrame(true);
		if (getSystemState()->getFramePhase() != FramePhase::ADVANCE_FRAME)
			initFrame();
	}
	AVM1HandleConstruction(forInitAction);
}

void MovieClip::beforeConstruction(bool _explicit)
{
	if(state.last_FP == -1 && needsActionScript3())
	{
		if (!framecontainer)
			framecontainer=new FrameContainer();
		advanceFrame(true);
	}
	Sprite::beforeConstruction(_explicit);
}

void MovieClip::afterConstruction(bool _explicit)
{
	if (needsActionScript3())
		setConstructorCallComplete();
	Sprite::afterConstruction(_explicit);
}

uint32_t MovieClip::getFrameCount()
{
	if (frameContainer == nullptr)
		return 0;
	return frameContainer->getFramesSize();
}
