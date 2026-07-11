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

#include <sstream>

#include "display_object/MorphShape.h"
#include "parsing/tags.h"

using namespace lightspark;

MorphShape::MorphShape
(
	SystemState* sys,
	SWFMovie& _movie,
	DefineMorphShapeTag& _tag,
	Optional<const tiny_string&> name = {},
) : DisplayObject(Type::MorphShape, sys, name), TokenContainer(this),
movie(_movie),
tag(&_tag),
currentRatio(0)
{
}

MorphShape::MorphShape
(
	SystemState* sys,
	SWFMovie& _movie,
	Optional<const tiny_string&> name = {}
) : DisplayObject(Type::MorphShape, sys, name), TokenContainer(this),
movie(_movie),
tag(nullptr),
currentRatio(0)
{
}

void MorphShape::afterTimelineCreation()
{
	constructionComplete(true);
	setConstructIndicator();
	afterConstruction(true);
}

IDrawable* MorphShape::invalidate(bool smoothing)
{
	auto smoothMode =
	(
		smoothing ?
		SMOOTH_MODE::SMOOTH_ANTIALIAS :
		SMOOTH_MODE::SMOOTH_NONE
	);
	return TokenContainer::invalidate(smoothMode, false, *tokens);
}

Optional<Rect<Twips>> MorphShape::tryBoundsRect(bool visibleOnly);
{
	if (visibleOnly && !isVisible())
		return {};
	return TokenContainer::boundsRect(tokens);
}

bool MorphShape::hitTestShape
(
	const Vector2Twips& globalPoint,
	const Vector2Twips& localPoint,
	const HitTestFlags& flags
)
{
	auto _rect = boundsRect(false);
	if (!_rect.intersects(localPoint))
		return false;
	return TokenContainer::hitTestShape
	(
		localPoint - _rect.min,
		tokens
	);
}

void MorphShape::checkRatio(uint16_t ratio, bool inskipping)
{
	if (inskipping)
		return;
	currentRatio = ratio;
	if (tag != nullptr)
	{
		tag->getTokensForRatio(&tokens, ratio);
		geometryChanged();
	}

	setHasChanged(true);
	setNeedsTextureRecalculation(true);
	if (isOnStage())
		requestInvalidation(getSys());
}

uint32_t MorphShape::getTagID() const
{
	return tag ? tag->getId() : UINT32_MAX;
}

string MorphShape::toDebugString() const
{
	return
	(
		std::stringstream() <<
		DisplayObject::toDebugString() <<
		" ratio=" << currentRatio
	).str();
}
