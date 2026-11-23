/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2013  Alessandro Pignotti (a.pignotti@sssup.it)
    Copyright (C) 2023, 2026  mr b0nk 500 (b0nk@b0nk.xyz)

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

#ifndef DISPLAY_OBJECT_STATICTEXT_H
#define DISPLAY_OBJECT_STATICTEXT_H 1

#include "compat.h"
#include "display_object/DisplayObject.h"
#include "graphics/TokenContainer.h"
#include "swf.h"
#include "swftypes.h"
#include "tiny_string.h"
#include "utils/optional.h"

namespace lightspark
{

class DefineTextTag;
class SWFMovie;

class StaticText : public DisplayObject, public TokenContainer
{
private:
	ASFUNCTION_ATOM(_getText);

	SWFMovie& movie
	DefineTextTag* tag;
public:
	StaticText
	(
		SystemState* sys,
		SWFMovie& _movie,
		tokensVector* tokens = nullptr,
		DefineTextTag* _tag = nullptr,
		Optional<const tiny_string&> name = {}
	);

	StaticText
	(
		SystemState* sys,
		SWFMovie& _movie,
		Optional<const tiny_string&> name = {}
	) : StaticText(sys, _movie, nullptr, nullptr, name) {}

	Optional<Rect<Twips>> tryBoundsRect(bool visibleOnly) override;
	bool hitTestShape
	(
		const Vector2Twips& globalPoint,
		const Vector2Twips& localPoint,
		const HitTestFlags& flags
	) override;

	void requestInvalidation
	(
		InvalidateQueue* q,
		bool forceTextureRefresh = false
	) override
	{
		TokenContainer::requestInvalidation
		(
			q,
			forceTextureRefresh
		);
	}

	IDrawable* invalidate(bool smoothing) override;
	uint32_t getTagID() const override;
	number_t getScaleFactor() const override { return scaling; }
};

}
#endif /* DISPLAY_OBJECT_STATICTEXT_H */
