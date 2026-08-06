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
#include "display_object/Button.h"
#include "display_object/Stage.h"
#include "parsing/tags.h"

using namespace lightspark;

void Button::handleMouseCursor(bool rollOver)
{
	hasMouse = rollOver;
	getSys()->setMouseHandCursor(hasMouse && useHandCursor);
}

bool Button::hitTestShape
(
	const Vector2Twips& globalPoint,
	const Vector2Twips& localPoint,
	const HitTestFlags& flags
)
{
	Locker l(mutexDisplayList);
	for (auto& child : dynamicDisplayList)
	{
		if (child.hitTestShape(globalPoint, flags))
			return true;
	}
	return false;
}

bool Button::filterEvent(const ClipEvent& ev)
{
	// NOTE: An invisible `Button` can still run a `rollOut`, or
	// `releaseOutside` event.
	// Normally, a disabled `Button` won't run events (with the exception
	// of `keyPress`), but it's state can still change. This is tested by
	// Ruffle's `avm1/mouse_events_visible_enabled` test.
	if (!isVisible() && state == State::Up)
		return false;

	// NOTE: `keyPress` events aren't handled, if the `Button` is inside
	// another `Button`.
	return
	(
		!ev.is<KeyPressEvent>() ||
		getParent() == nullptr ||
		!getParent()->is<Button>()
	);
}

bool Button::handleEvent(const ClipEvent& ev)
{
	bool isEnabled = getEnabled();
	bool handled = false;

	State newState;
	ButtonCond cond;
	Optional<ButtonSound> sound;
	std::tie(newState, cond, sound) = ev.visit(makeVisitor
	(
		[](const DragEvent& drag)
		{
			return std::make_tuple
			(
				drag.out ? State::Over : State::Down
				drag.out ?
				ButtonCond::OverDownToOutDown :
				ButtonCond::OutDownToOverDown,
				{}
			);
		},
		[&](const RollEvent& roll)
		{
			return roll.out ? std::make_tuple
			(
				State::Up,
				ButtonCond::OverUpToIdle,
				overToUpSound
			) : std::make_tuple
			(
				State::Over,
				ButtonCond::IdleToOverUp,
				upToOverSound
			);
		},
		[&](const ButtonPressEvent& press)
		{
			if (press.button != MouseButton::Left)
				return std::make_pair(-1, nullptr);
			return std::make_tuple
			(
				State::Down,
				ButtonCond::OverUpToOverDown,
				overToDownSound
			);
		},
		[&](const ButtonReleaseEvent& release)
		{
			if (release.button != MouseButton::Left)
				return std::make_tuple(-1, -1, {});
			return release.over ? std::make_tuple
			(
				State::Over,
				ButtonCond::OverDownToOverUp,
				downToOverSound
			) : std::make_tuple
			(
				State::Up,
				ButtonCond::OverUpToIdle,
				overToUpSound
			);
		},
		[&](const KeyPressEvent& keyPress)
		{
			handled = runActions(ButtonCond::fromKeyCode
			(
				keyPress.keyCode
			));
			return std::make_tuple(-1, -1, {});
		},
		// NOTE: `key{Up,Down}` might run event handlers.
		[&](const KeyEvent& key)
		{
			return std::make_tuple(state, -1, {});
		}
	));

	if (newState == -1)
		return handled;

	bool updateState;
	std::tie(updateState, newState) = [&]
	{
		if (!isEnabled)
		{
			// Remove the current `mouse{Over,Down}` objects.
			// This is needed to make sure the `Button` will run it's
			// events, when enabled.
			auto inputThread = getSys()->getInputThread();
			if (this == inputThread->getMouseOverObj())
				inputThread->setMouseOverObj(nullptr);
			if (this == inputThread->getMouseDownObj())
				inputThread->setMouseDownObj(nullptr);
			return std::make_pair
			(
				newState != State::Over,
				State::Up
			);
		}

		if (cond != -1)
			runActions(cond);
		if (sound.hasValue())
			AudioManager::handleSoundEvent(this, *sound);

		auto ret = std::make_pair(state != newState, newState);
		auto methodName = ev.getMethodName();
		if (!shouldRunEventHandlers(ev) || methodName.empty())
			return ret;
		// Queue ActionScript defined handlers after the SWF defined
		// handlers. (e.g. `clip.onPress = func;`).
		getSys()->queueActionBack
		(
			*this,
			MethodAction(tryToAVM1Object(), methodName),
			false
		);
		return ret;
	}();

	if (updateState)
		setState(newState);
	return false;
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

Optional<Rect<Twips>> Button::tryBoundsRect(bool visibleOnly)
{
	return {};
}

Button::Button
(
	SystemState* sys,
	SWFMovie& _movie,
	DefineButtonTag* _tag,
	Optional<const tiny_string&> name
) : InteractiveObject(sys, _movie, name), DisplayObjectContainer(movie),
movie(_movie),
tag(_tag),
state(State::Up),
initialized(false)
{
	if (tag == nullptr || tag->sounds == nullptr)
		return;

	auto trySetButtonSound = [&](uint16_t id, const SOUNDINFO& info)
	{
		if (!id)
			return {};
		return ButtonSound(id, info);
	};

	#define TRY_SET_BUTTONSOUND(num, name) trySetButtonSound \
	( \
		tag->sounds->SoundID##num##_##name, \
		tag->sounds->SoundInfo##num##_##name \
	)

	upToOverSound = TRY_SET_BUTTONSOUND(0, OverUpToIdle);
	overToDownSound = TRY_SET_BUTTONSOUND(1, IdleToOverUp);
	downToOverSound = TRY_SET_BUTTONSOUND(2, OverUpToOverDown);
	overToUpSound = TRY_SET_BUTTONSOUND(3, OverDownToOverUp);

	#undef TRY_SET_BUTTONSOUND
}

void Button::requestInvalidation
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

void Button::avm1Unload()
{
	forEachChild([](auto& child)
	{
		child.avm1Unload();
	});

	dropFocus();

	if (getMaskee() != nullptr)
		getMaskee()->setMasker(nullptr);
	if (getMasker() != nullptr)
		getMasker()->setMaskee(nullptr);

	// Don't unregister variable bindings.
	setRemovedByAVM1(true);
}

void Button::afterCreation
(
	_NGC<AVM1Object> initObj,
	const ObjectCreator& createdBy,
	bool runFrame
)
{
	auto obj = tryToAVM1Object();
	if (obj.isNull() || initialized)
		return;

	setState(State::Up);
	initialized = true;

	std::vector<std::pair<int32_t, DisplayObjectRef>> newChildren;
	for (const auto& record : tag->Characters)
	{
		if (!record.ButtonStateHitTest)
			continue;

		auto child = movie.createInstanceById(record.CharacterID);
		if (child == nullptr)
			continue;

		child->setMatrix(record.PlaceMatrix);
		child->setParent(this);
		child->setDepth(record.PlaceDepth);
		newChildren.emplace_back
		(
			record.PlaceDepth,
			*child
		);
	}

	for (const auto& pair : children)
	{
		auto depth = pair.first;
		auto& child = pair.second;
		child.afterCreation(nullptr, ObjectCreator::Movie, false);
		hitArea.insert(pair);
		hitBounds = hitBounds._union(child.getLocalBounds());
	}
}

uint32_t Button::getTagID() const
{
	return tag != nullptr ? tag->getId() : UINT32_MAX;
}

void Button::setState(const State& _state)
{
	auto hasState = []
	(
		const BUTTONRECORD& record,
		const State& _state
	)
	{
		switch (_state)
		{
			case State::Up: return record.ButtonStateUp;
			case State::Down: return record.ButtonStateDown;
			case State::Over: return record.ButtonStateOver;
			default: return false;
		}
	};

	std::set<int32_t> removedDepths;
	forEachChild([&](const auto& child)
	{
		removedDepths.emplace(child.getDepth());
	});

	std::vector<std::pair<int32_t, DisplayObjectRef>> children;
	for (const auto& record : tag->Characters)
	{
		if (!hasState(record, _state))
			continue;

		// The state contains this depth, so don't remove it.
		removedDepths.erase(record.PlaceDepth);

		auto child = [&]
		{
			auto child = tryGetChildAt(record.PlaceDepth);
			auto id = record.CharacterID;
			// Reuse an existing child.
			if (child != nullptr && child->getTagID() == id)
				return child;

			// Create a new child instance.
			child = movie.createInstanceById(id);
			if (child == nullptr)
				return nullptr;

			// New child that previously didn't exist, create it.
			child->setParent(this);
			child->setDepth(record.PlaceDepth);
			children.emplace_back
			(
				record.PlaceDepth,
				*child
			);
			return child;
		}();

		if (child == nullptr)
			continue;

		// Set the child's transform (and modify the previous child, if
		// it already existed).
		child->setMatrix(record.PlaceMatrix);
		child->setColorTransform(record.ColorTransform);
		child->setFilters(record.FilterList);
	}

	// Remove any children that no longer exist in this state.
	for (auto depth : removedDepths)
	{
		auto child = tryGetChildAt(depth);
		if (child != nullptr)
			removeChild(*child);
	}

	for (const auto& pair : children)
	{
		auto depth = pair.first;
		auto& child = pair.second;
		// Initialize the new child.
		child.afterCreation(nullptr, ObjectCreator::Movie, false);
		auto clip = child.as<MovieClip>();
		if (clip != nullptr)
			clip->AVM1runFrame();
		auto removedChild = replaceChildAt(depth, child);
		handleAddedEvent(*this, child, false);

		if (removedChild != nullptr)
			handleRemovedEvent(removedChild);
	}
	markAsChanged();
}

bool Button::getBoolProp(const tiny_string& name, bool _default) const
{
	return getAVM1BoolProp(name, [&](SystemState* sys)
	{
		return _default;
	});
}
