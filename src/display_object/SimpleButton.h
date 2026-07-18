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

#ifndef DISPLAY_OBJECT_SIMPLEBUTTON_H
#define DISPLAY_OBJECT_SIMPLEBUTTON_H 1

#include <cstdint>
#include <utility>
#include <vector>

#include "display_object/InteractiveObject.h"
#include "swftypes.h"
#include "tiny_string.h"
#include "utils/optional.h"

namespace lightspark
{

class ClipEvent;
class SWFMovie;

class SimpleButton : public InteractiveObject
{
public:
	enum BUTTONSTATE
	{
		STATE_UP,
		STATE_OVER,
		STATE_DOWN,
		STATE_OUT
	};
private:
	SWFMovie& movie;
	DisplayObject* stateChild[4];
	_NR<SoundChannel> soundchannel_OverUpToIdle;
	_NR<SoundChannel> soundchannel_IdleToOverUp;
	_NR<SoundChannel> soundchannel_OverUpToOverDown;
	_NR<SoundChannel> soundchannel_OverDownToOverUp;

	Rect<Twips> boundsRectWithTransformImpl(const MATRIX& mtx) override;
protected:
	DefineButtonTag* tag;
	bool statesDirty;
	BUTTONSTATE currentState;
	BUTTONSTATE oldState;
	bool enabled;
	bool useHandCursor;
	bool hasMouse;
public:
	SimpleButton
	(
		SystemState* sys,
		SWFMovie& _movie,
		DefineButtonTag* _tag = nullptr,
		Optional<const tiny_string&> name = {}
	);

	SimpleButton
	(
		SystemState* sys,
		SWFMovie& _movie,
		Optional<const tiny_string&> name = {}
	) : SimpleButton(sys, _movie, nullptr, name) {}

	Optional<Rect<Twips>> tryBoundsRect(bool visibleOnly) override;
	bool hitTestShape
	(
		const Vector2Twips& globalPoint,
		const Vector2Twips& localPoint,
		const HitTestFlags& flags
	) override;

	bool propagateEventToChildren(const ClipEvent& ev) override;
	bool handleEvent(const ClipEvent& ev) override;

	void requestInvalidation
	(
		InvalidateQueue* q,
		bool forceTextureRefresh = false
	) override;

	uint32_t getTagID() const override;
	DisplayObject* getStateObject(const BUTTONSTATE& state)
	{
		return stateChild[state];
	}

	void setStateObject(const BUTTONSTATE& state, DisplayObject* obj);
	bool getEnabled() const { return enabled; }
	void setEnabled(bool _enabled) { enabled = _enabled; }
	bool getUseHandCursor() const { return useHandCursor; }
	void setUseHandCursor(bool flag) { useHandCursor = flag; }
	void handleMouseCursor(bool rollOver) override;
	bool allowAsMask() const override
	{
		return !isEmpty();
	}

	BUTTONSTATE getCurrentState() const { return currentState; }
};

}
#endif /* DISPLAY_OBJECT_SIMPLEBUTTON_H */
