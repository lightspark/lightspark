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

#include "display_object/Shape.h"
#include "graphics/Graphics.h"
#include "parsing/tags.h"

using namespace std;
using namespace lightspark;

bool Shape::hasGraphics() const
{
	return graphics != nullptr && graphics->hasBounds();
}

Optional<Rect<Twips>> Shape::tryBoundsRect(bool visibleOnly)
{
	if (visibleOnly && !isVisible())
		return {};
	if (isTimelineCreated() && tag != nullptr)
		return tag->ShapeBounds;

	if (hasGraphics())
		return graphics->tryBoundsRect();
	return TokenContainer::tryBoundsRect(tokens);
}

bool Shape::hitTestShape
(
	const Vector2Twips& globalPoint,
	const Vector2Twips& localPoint,
	const HitTestFlags& flags
)
{
	auto _rect = boundsRect(false);
	if (!_rect.intersects(localPoint))
		return false;

	bool isDynamic = !isTimelineCreated() || tag == nullptr;
	if (isDynamic && hasGraphics())
		return graphics->hitTest(localPoint);

	return TokenContainer::hitTestShape
	(
		localPoint - _rect.min,
		tokens
	);
}

Shape::Shape
(
	SystemState* sys,
	DefineShapeTag* _tag,
	number_t _scaling,
	Optional<const tiny_string&> name
) : DisplayObject(Type::Shape, sys, name), TokenContainer
(
	this,
	_tag != nullptr ? _tag->tokens : nullptr,
	_scaling
), graphics(nullptr), tag(_tag)
{
}

Shape::~Shape()
{
	if (graphics != nullptr)
		delete graphics;
}

Graphics& Shape::getGraphics()
{
	if (graphics == nullptr)
		graphics = new Graphics(this);
	return *graphics;
}

uint32_t Shape::getTagID() const
{
	return tag != nullptr ? tag->getId() : UINT32_MAX;
}

void Shape::requestInvalidation
(
	InvalidateQueue* q,
	bool forceTextureRefresh
)
{
	if (hasGraphics())
	{
		requestInvalidationFilterParent(q);
		q->addToInvalidateQueue(this);
	}
	TokenContainer::requestInvalidation(q, forceTextureRefresh);
}

void Shape::refreshSurfaceState()
{
	if (graphics != nullptr)
		graphics->refreshSurfaceState();
}

IDrawable* Shape::invalidate(bool smoothing)
{
	auto smoothMode =
	(
		smoothing ?
		SMOOTH_MODE::SMOOTH_ANTIALIAS :
		SMOOTH_MODE::SMOOTH_NONE
	);

	if (hasGraphics())
		return graphics->invalidate(smoothMode);
	return TokenContainer::invalidate(smoothMode, false, *tokens);
}

string Shape::toDebugString() const
{
	auto ret = DisplayObject::toDebugString();
	if (hasGraphics())
		ret += " hasgraphics";
	return ret;
}
