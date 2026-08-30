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

#ifndef DISPLAY_OBJECT_BUTTON_H
#define DISPLAY_OBJECT_BUTTON_H 1

#include <cstdint>
#include <map>
#include <utility>
#include <vector>

#include "display_object/DisplayObjectContainer.h"
#include "display_object/InteractiveObject.h"
#include "swftypes.h"
#include "tiny_string.h"
#include "utils/optional.h"

// Based on Ruffle's `display_object::Avm1Button`.

namespace lightspark
{

class DefineButtonTag;
class ClipEvent;
class SWFMovie;

class Button : public InteractiveObject, public DisplayObjectContainer
{
	template<typename T>
	using RefWrapper = std::reference_wrapper<T>;
	using DisplayObjectRef = RefWrapper<DisplayObject>;
public:
	enum class State
	{
		Up,
		Over,
		Down,
	};
private:
	SWFMovie& movie;

	Optional<ButtonSound> upToOverSound;
	Optional<ButtonSound> overToDownSound;
	Optional<ButtonSound> downToOverSound;
	Optional<ButtonSound> overToUpSound;

	DefineButtonTag* tag;
	State state;

	std::map<int32_t, DisplayObjectRef> hitArea;
	Rect<Twips> hitBounds;
	std::vector<AVM1VarBinding> varBindings;
	bool initialized;
public:
	Button
	(
		SystemState* sys,
		SWFMovie& _movie,
		DefineButtonTag* _tag = nullptr,
		Optional<const tiny_string&> name = {}
	);

	Button
	(
		SystemState* sys,
		SWFMovie& _movie,
		Optional<const tiny_string&> name = {}
	) : Button(sys, _movie, nullptr, name) {}

	Optional<Rect<Twips>> tryBoundsRect(bool visibleOnly) override;
	bool hitTestShape
	(
		const Vector2Twips& globalPoint,
		const Vector2Twips& localPoint,
		const HitTestFlags& flags
	) override;

	bool filterEvent(const ClipEvent& ev) override;
	bool handleEvent(const ClipEvent& ev) override;

	void requestInvalidation
	(
		InvalidateQueue* q,
		bool forceTextureRefresh = false
	) override;

	void avm1Unload() override;
	void afterCreation
	(
		_NGC<AVM1Object> initObj,
		const ObjectCreator& createdBy,
		bool runFrame
	) override;

	void handleMouseCursor(bool rollOver) override;
	bool allowAsMask() const override
	{
		return !isEmpty();
	}

	Span<const AVM1VarBinding> getAVM1VarBindings() const override
	{
		return makeSpan(varBindings);
	}

	Span<AVM1VarBinding> getAVM1VarBindings() override
	{
		return makeSpan(varBindings);
	}

	uint32_t getTagID() const override;
	const SWFMovie& getMovie() const override { return movie; }
	const State& getState() const { return state; }
	void setState(const State& _state);
	bool getBoolProp(const tiny_string& name, bool _default) const;
	bool getEnabled() const { return getBoolProp("enabled", true); }
	bool getUseHandCursor() const
	{
		return getBoolProp("useHandCursor", true);
	}
};

}
#endif /* DISPLAY_OBJECT_BUTTON_H */
