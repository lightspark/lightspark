/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2013  Alessandro Pignotti (a.pignotti@sssup.it)
    Copyright (C) 2026  mr b0nk 500 (b0nk@b0nk.xyz)

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

#ifndef DISPLAY_OBJECT_MORPHSHAPE_H
#define DISPLAY_OBJECT_MORPHSHAPE_H 1

#include "backends/geometry.h"
#include "display_object/DisplayObject.h"
#include "graphics/TokenContainer.h"
#include "swftypes.h"
#include "utils/optional.h"

// Based on Ruffle's `display_object::MorphShape`.

namespace lightspark
{

class DefineMorphShapeTag;
class SWFMovie;

class MorphShape : public DisplayObject, public TokenContainer
{
private:
	SWFMovie& movie;
	DefineMorphShapeTag* tag;
	uint16_t currentRatio;
public:
	MorphShape
	(
		SystemState* sys,
		SWFMovie& _movie,
		DefineMorphShapeTag& _tag,
		Optional<const tiny_string&> name = {},
	);

	MorphShape
	(
		SystemState* sys,
		SWFMovie& _movie,
		Optional<const tiny_string&> name = {}
	);

	void afterLegacyCreation() override;
	void requestInvalidation
	(
		InvalidateQueue* q,
		bool forceTextureRefresh = false
	) override
	{
		TokenContainer::requestInvalidation(q, forceTextureRefresh);
	}

	IDrawable* invalidate(bool smoothing) override;
	Optional<Rect<Twips>> tryBoundsRect(bool visibleOnly) override;
	bool hitTestShape
	(
		const Vector2Twips& globalPoint,
		const Vector2Twips& localPoint,
		const HitTestFlags& flags
	) override;

	void checkRatio(uint16_t ratio, bool inskipping) override;
	uint32_t getTagID() const override;
	number_t getScaleFactor() const override { return scaling; }
	std::string toDebugString() const override;
};

}
#endif /* DISPLAY_OBJECT_MORPHSHAPE_H */
