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

#ifndef SCRIPTING_AVM1_MOVIECLIP_REF_H
#define SCRIPTING_AVM1_MOVIECLIP_REF_H 1

#include <list>
#include <utility>

#include "gc/resource.h"
#include "gc/ptr.h"
#include "tiny_string.h"

// Based on Ruffle's `avm1::object_reference::MovieClipReference`.

namespace lightspark
{

class AVM1Activation;
class AVM1DisplayObject;
class AVM1Object;
class DisplayObject;
class GcContext;
template<typename T>
class Optional;

class AVM1MovieClipPath
{
public:
	using SegmentsType = std::list<tiny_string>;
private:
	// The level that this path starts at.
	int32_t level;

	// The elements of the path.
	SegmentsType pathSegments;

	// The unparsed, original path.
	// This is used to make string conversions faster, by not having to
	// rebuild the path.
	tiny_string fullPath;
public:
	// Construct an `AVM1MovieClipPath` from a clip path string.
	AVM1MovieClipPath(const tiny_string& path);

	int32_t getLevel() const { return level; }
	const SegmentsType& getPathSegments() const { return pathSegments; }
	const tiny_string& getFullPath() const { return fullPath; }
};

// Represents a reference to a `MovieClip` in AVM1.
// This consists of a string path, which resolves to to a target value,
// when used.
// This also handles caching, in order to maintain performance.
class AVM1MovieClipRef : public GcResource
{
private:
	// The path to the target clip.
	MovieClipPath path;

	// A weak reference to the target `DisplayObject` that `path` points
	// to.
	// This is used for fast path resolving when possible, as well as for
	// regenerating `path` (in the case that the target object was
	// renamed).
	// If this is `NullGc`, then we've previously missed the cache, due
	// to the target object being removed, and recreated, causing us to
	// fallback to the slow path resolution.
	WeakGcPtr<AVM1DisplayObject> cachedDispObj;
public:
	AVM1MovieClipRef
	(
		AVM1Activation& activation,
		const GcPtr<AVM1DisplayObject>& dispObj
	);

	// Handle the logic of SWF 5 `DisplayObject`s.
	static GcPtr<ADisplayObject> handleSWF5Refs
	(
		AVM1Activation& activation,
		const GcPtr<DisplayObject>& dispObj
	);

	using ResolveRefType = Optional<std::pair
	<
		bool,
		GcPtr<DisplayObject>
	>>;
	// Resolve the clip reference.
	// `pair.first` indicates if the path was cached, or not.
	ResolveRefType resolveRef(AVM1Activation& activation) const;

	// Convert this reference into an `Object`.
	NullableGcPtr<AVM1Object> toObject(AVM1Activation& activation) const;

	// Convert this reference into a `String`.
	tiny_string toString(AVM1Activation& activation) const;

	// Get the path used for this reference.
	const tiny_string& getPath() const { return path.getFullPath(); }
};

}
#endif /* SCRIPTING_AVM1_MOVIECLIP_REF_H */
