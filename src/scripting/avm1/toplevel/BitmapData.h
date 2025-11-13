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

#ifndef SCRIPTING_AVM1_TOPLEVEL_BITMAPDATA_H
#define SCRIPTING_AVM1_TOPLEVEL_BITMAPDATA_H 1

#include "gc/ptr.h"
#include "scripting/avm1/function.h"
#include "scripting/avm1/object.h"

// Based on Ruffle's `avm1::globals::bitmap_data` crate.

#define BITMAP_DATA_GETTER_DECL(name) \
	AVM1_GETTER_TYPE_DECL(AVM1BitmapData, name)
#define BITMAP_DATA_GETTER_BODY(name) \
	AVM1_GETTER_TYPE_BODY(AVM1BitmapData, AVM1BitmapData, name)

#define BITMAP_DATA_FUNCTION_DECL(name) \
	AVM1_FUNCTION_TYPE_DECL(AVM1BitmapData, name)
#define BITMAP_DATA_FUNCTION_BODY(name) \
	AVM1_FUNCTION_TYPE_BODY(AVM1BitmapData, AVM1BitmapData, name)

namespace lightspark
{

class AVM1Activation;
class AVM1Context;
class AVM1DeclContext;
class AVM1SystemClass;
class AVM1Value;
class BitmapData;
class DisplayObject;
class GcContext;
template<typename T>
class Optional;

class AVM1BitmapData : public AVM1Object
{
private:
	GcPtr<BitmapData> data;
public:
	AVM1BitmapData
	(
		GcContext& ctx,
		const Optional<AVM1Value>& proto
		const GcPtr<BitmapData>& _data
	) : AVM1Object(ctx, proto), data(_data) {}

	GcPtr<BitmapData>& getData() { return data; }
	const GcPtr<BitmapData>& getData() const { return data; }
	NullableGcPtr<BitmapContainer>& getContainer();
	const NullableGcPtr<BitmapContainer>& getContainer() const;
	bool isDisposed() const;

	static GcPtr<AVM1SystemClass> createClass
	(
		AVM1DeclContext& ctx,
		const GcPtr<AVM1Object>& superProto
	);

	AVM1_FUNCTION_DECL(ctor);
	BITMAP_DATA_GETTER_DECL(Height);
	BITMAP_DATA_GETTER_DECL(Width);
	BITMAP_DATA_GETTER_DECL(Transparent);
	BITMAP_DATA_GETTER_DECL(Rectangle);
	BITMAP_DATA_FUNCTION_DECL(getPixel);
	BITMAP_DATA_FUNCTION_DECL(getPixel32);
	BITMAP_DATA_FUNCTION_DECL(setPixel);
	BITMAP_DATA_FUNCTION_DECL(setPixel32);
	AVM1_FUNCTION_DECL(copyChannel);
	AVM1_FUNCTION_DECL(fillRect);
	BITMAP_DATA_FUNCTION_DECL(clone);
	BITMAP_DATA_FUNCTION_DECL(dispose);
	BITMAP_DATA_FUNCTION_DECL(floodFill);
	AVM1_FUNCTION_DECL(noise);
	BITMAP_DATA_FUNCTION_DECL(colorTransform);
	BITMAP_DATA_FUNCTION_DECL(getColorBoundsRect);
	BITMAP_DATA_FUNCTION_DECL(perlinNoise);
	BITMAP_DATA_FUNCTION_DECL(applyFilter);
	BITMAP_DATA_FUNCTION_DECL(draw);
	BITMAP_DATA_FUNCTION_DECL(hitTest);
	BITMAP_DATA_FUNCTION_DECL(generateFilterRect);
	BITMAP_DATA_FUNCTION_DECL(copyPixels);
	BITMAP_DATA_FUNCTION_DECL(merge);
	BITMAP_DATA_FUNCTION_DECL(paletteMap);
	BITMAP_DATA_FUNCTION_DECL(pixelDissolve);
	BITMAP_DATA_FUNCTION_DECL(scroll);
	BITMAP_DATA_FUNCTION_DECL(threshold);
	AVM1_FUNCTION_DECL(compare);
	BITMAP_DATA_FUNCTION_DECL(loadBitmap);
};

}
#endif /* SCRIPTING_AVM1_TOPLEVEL_BITMAPDATA_H */
