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

#include "backends/geometry.h"
#include "display_object/StaticText.h"
#include "parsing/tags.h"

using namespace lightspark;

IDrawable* StaticText::invalidate(bool smoothing)
{
	auto smoothMode =
	(
		smoothing ?
		SMOOTH_MODE::SMOOTH_SUBPIXEL :
		SMOOTH_MODE::SMOOTH_NONE
	);
	return TokenContainer::invalidate(smoothMode, false, *tokens);
}

uint32_t StaticText::getTagID() const
{
	return tag != nullptr ? tag->getId() : UINT32_MAX;
}

Optional<Rect<Twips>> StaticText::tryBoundsRect(bool visibleOnly)
{
	if (visibleOnly && !isVisible())
		return {};
	if (tag == nullptr)
		return {};
	return tag->TextBounds;
}

bool StaticText::hitTestShape
(
	const Vector2Twips& globalPoint,
	const Vector2Twips& localPoint,
	const HitTestFlags& flags
)
{
	if (flags & HitTestFlags::SkipInvisible && !isVisible())
		return false;

	if (tag != nullptr && tag->UseFlashType)
	{
		// when using advanced text rendering, hittesting is done on the transformed bounds rect
		return getWorldBounds().intersects(globalPoint);
	}

	auto _bounds = tryBoundsRect(false);
	if (!_bounds.hasValue() || !_bounds.intersects(localPoint))
		return false;
	return tokens->hitTest
	(
		getSys(),
		localPoint - _bounds->min,
		scaling
	);
}

StaticText::StaticText
(
	SystemState* sys,
	SWFMovie& _movie,
	tokensVector* tokens = nullptr,
	DefineTextTag* _tag = nullptr,
	Optional<const tiny_string&> name = {}
) : DisplayObject(sys, name), TokenContainer
(
	this,
	tokens,
	1 / 1024.0 / 20.0
),
movie(_movie),
tag(_tag)
{
}
