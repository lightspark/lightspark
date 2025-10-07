/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2025  mr b0nk 500 (b0nk@b0nk.xyz)

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

#include "scripting/avm1/activation.h"
#include "scripting/avm1/object.h"
#include "scripting/avm1/display_object.h"
#include "scripting/avm1/movieclip_ref.h"
#include "tiny_string.h"
#include "utils/optional.h"

using namespace lightspark;

// Based on Ruffle's `avm1::object_reference::MovieClipReference`.

AVM1MovieClipPath::AVM1MovieClipPath(const tiny_string& path) :
level(0),
fullPath(path)
{
	// Break up the path.
	auto parts = path.split('.');

	if (parts.empty())
		return;

	// Parse the level id, to support multi file movies.
	auto it = parts.begin();
	if (it->startsWith("_level"))
		level = it->stripPrefix("_level").tryToNumber<int32_t>().valueOr(level);
	pathSegments = SegmentsType(++it, parts.end());
}

AVM1MovieClipRef::AVM1MovieClipRef
(
	AVM1Activation& activation,
	const _R<AVM1DisplayObject>& dispObj
)
{
	if (dispObj->is<MovieClip>())
	{
		path = AVM1MovieClipPath(dispObj->getPath());
		cachedDispObj = dispObj;
	}
	else if (activation.getSwfVersion() <= 5)
	{
		cachedDispObj = handleSWF5Refs(activation, dispObj);
		path = AVM1MovieClipPath(cachedDispObj->getPath());
	}
	else
		throw RunTimeException("Invalid MovieClipRef");
}

_R<AVM1DisplayObject> AVM1MovieClipRef::handleSWF5Refs
(
	AVM1Activation& activation,
	const _R<AVM1DisplayObject>& dispObj
)
{
	if (activation.getSwfVersion() > 5)
		return dispObj;

	// NOTE: In SWF 5, paths resolve to the first `MovieClip` parent, if
	// the target isn't a `MovieClip`.
	auto obj = dispObj.getPtr();
	for (; obj != nullptr && !obj->is<MovieClip>(); obj->getAVM1Parent());
	if (obj == nullptr)
	{
		throw RunTimeException
		(
			"AVM1MovieClipRef::handleSWF5Refs(): Somehow got an object "
			"that has no `MovieClip` in the chain."
		);
	}

	return _MR(obj);
}

Optional<AVM1MovieClipRef::ResolveRefType> AVM1MovieClipRef::resolveRef
(
	AVM1Activation& activation
) const
{
	// Check if we've got a cache that we can use.
	if (!cachedDispObj.isNull() && !cachedDispObj->isAVM1Removed())
	{
		// NOTE: There's a bug here, but this *is* how it works in Flash
		// Player:
		// If we're using the cached `DisplayObject`, we return it's path,
		// which can be changed by setting `_name`.
		// However, if we remove, and recreate the clip, the stored path
		// (the original path) will differ from the path of the cached
		// object (the modified path).
		// Basically, changes to `_name` are reverted after the clip is
		// recreated.
		return std::make_pair(true, handleSWF5Refs(activation, cachedDispObj));
	}

	// We missed the cache, fallback to the slow path.
	cachedDispObj = NullRef;

	// The clip can't be used (at all).
	// Here, we manually parse the path, in order to find the target
	// `DisplayObject`.
	// This is different to how paths are resolved in `AVM1Activation`
	// in 2 ways:
	// 1. We only handle slash paths to `DisplayObject`s. Other path
	// types, and variable paths *aren't* valid here.
	// 2. We only interact with the display list, not scopes. If you
	// shadow a `DisplayObject` in a path, it still has to resolve to
	// the correct object. e.g.:
	// ```actionscript
	// var _level0 = 123;
	// trace(this.child);
	// ```
	// Should correctly find the child. Since `this` == `_level0.child`,
	// we don't want to try, and find `123.child`!

	// Get the level.
	auto start = _MNR(activation.getOrCreateLevel(path.getLevel()));

	// Keep traversing the path, to find the target `DisplayObject`.
	for (const auto& part : path.getPathSegments())
	{
		if (start.isNull() && !start->is<DisplayObjectContainer>())
			continue;
		auto con = start->as<DisplayObjectContainer>();
		start = con->getChildByName(part, activation.isCaseSensitive());
	}

	if (start.isNull())
		return {};
	
	return std::make_tuple(false, handleSWF5Refs(activation, start));
}

_NR<AVM1Object> AVM1MovieClipRef::toObject(AVM1Activation& activation) const
{
	return resolveRef(activation).transformOr
	(
		NullRef,
		[](const ResolveRefType& pair)
		{
			return _MNR(pair.second->as<AVM1Object>());
		}
	);
}

tiny_string AVM1MovieClipRef::toString(AVM1Activation& activation) const
{
	return resolveRef(activation).transformOr
	(
		// Couldn't resolve the reference.
		tiny_string(),
		[](const ResolveRefType& pair)
		{
			// Found the reference, cached. We sadly can't reuse `path`.
			// It'd be faster if we could.
			// But if the clip's been renamed since being created, then
			// `dispObj->getPath() != path`.
			if (pair.first)
				return pair.second->getPath();
			// Found the reference, uncached, which means our cached path
			// must be correct.
			return path.getFullPath();
		}
	);
}
