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

#include <list>

#include "backends/cachedsurface.h"
#include "display_object/RootMovieClip.h"
#include "display_object/SimpleButton.h"
#include "display_object/Stage.h"
#include "parsing/tags.h"

using namespace lightspark;

void SimpleButton::handleMouseCursor(bool rollOver)
{
	hasMouse = rollOver;
	getSys()->setMouseHandCursor(hasMouse && useHandCursor);
}

// Based on Ruffle's `AVM2Button::hit_test_shape()`.
bool SimpleButton::hitTestShape
(
	const Vector2Twips& globalPoint,
	const Vector2Twips& localPoint,
	const HitTestFlags& flags
)
{
	if (flags & HitTestFlags::SkipInvisible && !isVisible())
		return false;

	auto child = getStateObject(currentState);
	if (child == nullptr)
		return false;

	if (child->getParent() == nullptr && !getMatrix().isInvertible())
		return false;

	auto _globalPoint =
	(
		child->getParent() == nullptr ?
		// `hitArea` isn't actually a child, so use `localPoint` instead.
		localPoint :
		globalPoint
	);
	return child->hitTestShape(_globalPoint, flags);
}

// Based on Ruffle's `AVM2Button::propagate_to_children()`.
bool SimpleButton::propagateEventToChildren(const ClipEvent& ev)
{
	if (!ev.isPropagating())
		return false;

	auto child = stateChild[currentState];
	if (child == nullptr)
		return false;

	auto intrObj = child->as<InteractiveObject>();
	return intrObj != nullptr && intrObj->handleClipEvent(ev);
}

// Based on Ruffle's `AVM2Button::event_dispatch()`.
bool SimpleButton::handleEvent(const ClipEvent& ev)
{
	auto pair = ev.visit(makeVisitor
	(
		[](const DragEvent& drag)
		{
			return std::make_pair
			(
				drag.over ? STATE_OVER : STATE_DOWN,
				nullptr
			);
		},
		[&](const RollEvent& roll)
		{
			return roll.out ? std::make_pair
			(
				STATE_UP,
				soundchannel_OverUpToIdle
			) : std::make_pair
			(
				STATE_OVER,
				soundchannel_IdleToOverUp
			);
		},
		[&](const ButtonPressEvent& press)
		{
			if (press.button != MouseButton::Left)
				return std::make_pair(-1, nullptr);
			return std::make_pair
			(
				STATE_DOWN,
				soundchannel_OverUpToOverDown
			);
		},
		[&](const ButtonReleaseEvent& release)
		{
			if (release.button != MouseButton::Left)
				return std::make_pair(-1, nullptr);
			return release.over ? std::make_pair
			(
				STATE_UP,
				soundchannel_OverUpToIdle
			) : std::make_pair
			(
				STATE_OVER,
				soundchannel_OverDownToOverUp
			);
		},
		[&](const MouseUpEvent& mouseUp)
		{
			bool isValid =
			(
				mouseUp.button == MouseButton::Left &&
				!mouseUp.out
			);

			if (!isValid)
				return std::make_pair(-1, nullptr);
			return std::make_pair
			(
				STATE_UP,
				soundchannel_OverUpToIdle
			);
		}
	));

	if (pair.first == -1)
		return false;

	if (pair.second != nullptr)
		pair.second->play();

	if (currentState != pair.first)
		setState(pair.first);
	return true;
}

Rect<Twips> boundsRectWithTransformImpl(const MATRIX& mtx)
{
	auto _bounds = mtx * boundsRect(false);
	auto child = getStateObject(currentState);
	if (child == nullptr)
		return _bounds;
	return _bounds._union(child->boundsRectWithTransform
	(
		mtx *
		child->getMatrix()
	));
}

Optional<Rect<Twips>> SimpleButton::tryBoundsRect(bool visibleOnly)
{
	return {};
}

SimpleButton::SimpleButton
(
	SystemState* sys,
	SWFMovie& _movie,
	DefineButtonTag* _tag,
	Optional<const tiny_string&> name
) : InteractiveObject(sys, _movie, name),
stateChild({ nullptr }),
tag(_tag),
statesDirty(true),
currentState(STATE_OUT),
oldState(STATE_OUT),
enabled(true),
useHandCursor(true),
hasMouse(false),
{
	tabEnabled = true;
	if (tag == nullptr || tag->sounds == nullptr)
		return;

	auto trySetButtonSound = [&]
	(
		uint16_t id,
		const SOUNDINFO& info,
		auto& channel,
		const char* name
	)
	{
		if (!id)
			return;
		auto sound = dynamic_cast<DefineSoundTag*>(movie.dictionaryLookup(id));
		if (sound != nullptr)
		{
			channel = sound->createSoundChannel(&info);
			return;
		}

		LOG
		(
			LOG_ERROR,
			"ButtonSound not found for " << name << ':' <<
			id << " on button " << tag->getId()
		);
	};

	#define TRY_SET_BUTTONSOUND(num, name) trySetButtonSound \
	( \
		tag->sounds->SoundID##num##_##name, \
		tag->sounds->SoundInfo##num##_##name, \
		soundchannel_##name, \
		#name \
	)

	TRY_SET_BUTTONSOUND(0, OverUpToIdle);
	TRY_SET_BUTTONSOUND(1, IdleToOverUp);
	TRY_SET_BUTTONSOUND(2, OverUpToOverDown);
	TRY_SET_BUTTONSOUND(3, OverDownToOverUp);

	#undef TRY_SET_BUTTONSOUND
}

void SimpleButton::requestInvalidation
(
	InvalidateQueue* q,
	bool forceTextureRefresh
)
{
	requestInvalidationFilterParent(q);
	InteractiveObject::requestInvalidation
	(
		q,
		forceTextureRefresh
	);

	setHasChanged(true);
	q->addToInvalidateQueue(this);
}

uint32_t SimpleButton::getTagID() const
{
	return tag != nullptr ? tag->getId() : UINT32_MAX;
}

// Based on Ruffle's `AVM2Button::set_state_child()`.
void SimpleButton::setStateObject
(
	const BUTTONSTATE& state,
	DisplayObject* child
)
{
	bool childWasOnStage = child != nullptr && child->isOnStage();
	auto oldStateChild = stateChild[state];
	bool isCurState = state == currentState;

	stateChild[state] = child;
	if (child != nullptr)
	{
		auto parent = child->getParent();
		if (parent == nullptr)
			goto trySetParent;

		auto _container = parent->as<DisplayObjectContainer>();
		if (_container != nullptr)
			_container->removeChild(child);
	trySetParent:
		if (isCurState)
			child->setParent(this);
	}

	if (oldStateChild != nullptr)
		oldStateChild->setParent(nullptr);

	if (!isCurState)
		return;

	if (child != nullptr)
	{
		DisplayObjectContainer::handleAddedEvent
		(
			*this,
			child,
			childWasOnStage
		);
	}

	if (oldStateChild != nullptr)
		DisplayObjectContainer::handleRemovedEvent(oldStateChild);

	if (child != nullptr)
	{
		getSys()->handleBroadcastEvent("frameConstructed");
		child->executeFrameScript();
		getSys()->handleBroadcastEvent("exitFrame");
	}
}
