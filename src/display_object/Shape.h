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

#ifndef DISPLAY_OBJECT_SHAPE_H
#define DISPLAY_OBJECT_SHAPE_H 1

#include "backends/geometry.h"
#include "display_object/DisplayObject.h"
#include "graphics/TokenContainer.h"
#include "utils/optional.h"

// Based on Ruffle's `display_object::Graphic`.

namespace lightspark
{

class tiny_string;
class DefineShapeTag;
class Graphics;

class Shape : public DisplayObject, public TokenContainer
{
private:
	Graphics* graphics;
	DefineShapeTag* tag;

	bool hasGraphics() const;
public:
	Optional<Rect<Twips>> tryBoundsRect(bool visibleOnly) override;
	bool hitTestShape
	(
		const Vector2Twips& globalPoint,
		const Vector2Twips& localPoint,
		const HitTestFlags& flags
	) override;

	Shape
	(
		SystemState* sys,
		DefineShapeTag* _tag,
		number_t _scaling,
		Optional<const tiny_string&> name = {}
	);

	Shape
	(
		SystemState* sys,
		Optional<const tiny_string&> name = {}
	) : Shape(sys, nullptr, 1, name) {}

	~Shape();

	Graphics* tryGetGraphics() const { return graphics; }
	Graphics& getGraphics();
	uint32_t getTagID() const override;
	number_t getScaleFactor() const override { return scaling; }

	void requestInvalidation(InvalidateQueue* q, bool forceTextureRefresh=false) override;
	void refreshSurfaceState() override;
	IDrawable* invalidate(bool smoothing) override;
	std::string toDebugString() const override;
};

}

#endif /* DISPLAY_OBJECT_SHAPE_H */
