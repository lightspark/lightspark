/**************************************************************************
    Lightspark, a free flash player implementation

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

#ifndef DISPLAY_OBJECT_DISPLAYOBJECT_H
#define DISPLAY_OBJECT_DISPLAYOBJECT_H 1

#include <cstdint>

#include "backends/geometry.h"
#include "smartrefs.h"
#include "swftypes.h"
#include "tiny_string.h"

namespace lightspark
{

class Matrix3D;
template<typename T>
class Optional;

class DisplayObject
{
private:
	DisplayObject* parent;

	// The frame that this clip was placed on.
	uint16_t placeFrame;
	uint16_t depth;
	uint16_t ratio;

	tiny_string name;

	MATRIX matrix;
	number_t tz;
	number_t rotation;
	Vector2f scale;
	number_t scaleZ;
	number_t skew;
	AS_BLENDMODE blendMode;

	// If `true`, this `DisplayObject` is the root of a loaded file
	// (either an SWF, or an image).
	bool isLoadedRoot;
	bool inInitFrame;
	bool filterListHasChanged;
	bool hasMatrix3D;
	// The number of `DisplayObject`s that use us as a mask.
	uint32_t maskCount;

	number_t maxFilterBorder;

	/* cachedSurface may only be read/written from within the render thread
	 * It is the cached version of the object for fast draw on the Stage
	 */
	_R<CachedSurface> cachedSurface;
	bool useLegacyMatrix;
	bool needsTextureRecalculation;
	bool textureRecalculationSkippable;

	RectF scalingGrid;
	MATRIX currentRenderMatrix;

	// Whether this `DisplayObject` has been removed from the display
	// list.
	// This is needed in AVM1 to remove any queued actions from removed
	// `MovieClip`s.
	bool removedByAVM1;
	bool visible;
	bool transformedByActionScript;
	bool placedByActionScript;
	// If set, skip the next call to `enterFrame()`.
	bool skipFrame;
	// this is reset after the drawjob is done to ensure a changed DisplayObject is only rendered once
	bool hasChanged;
	// this is set to true for DisplayObjects that are placed from a tag
	bool legacy;
public:
	void setMatrix(const MATRIX& m);
	void setMatrix3D(const Matrix3D* m);
};

}
#endif /* DISPLAY_OBJECT_DISPLAYOBJECT_H */
