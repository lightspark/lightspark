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

#include <initializer_list>
#include <sstream>

#include "backends/graphics.h"
#include "display_object/DisplayObject.h"
#include "gc/context.h"
#include "scripting/avm1/activation.h"
#include "scripting/avm1/function.h"
#include "scripting/avm1/movieclip_ref.h"
#include "scripting/avm1/object/array.h"
#include "scripting/avm1/object/object.h"
#include "scripting/avm1/prop.h"
#include "scripting/avm1/prop_decl.h"
#include "scripting/avm1/runtime.h"
#include "scripting/avm1/toplevel/ColorTransform.h"
#include "scripting/avm1/toplevel/Matrix.h"
#include "tiny_string.h"
#include "utils/array.h"
#include "utils/optional.h"

using namespace lightspark;

// Based on Ruffle's `avm1::globals::transform` crate.

AVM1Transform::AVM1Transform
(
	AVM1Activation& act,
	const NullableGcPtr<AVM1MovieClipRef>& _clip
) : AVM1Object
(
	act,
	act.getPrototypes().transform->proto
), clip(_clip) {}

using AVM1Transform;

#define TRANSFORM_GETTER(func) AVM1_TYPE_FUNC(AVM1Transform, \
[](AVM1_FUNCTION_TYPE_ARGS(AVM1Transform)) \
{ \
	auto clip = _this->getClip(); \
	if (clip.isNull()) \
		return AVM1Value::undefinedVal; \
	return get#func(act, _this, clip); \
})

#define TRANSFORM_SETTER(func) AVM1_TYPE_FUNC(AVM1Transform, \
[](AVM1_FUNCTION_TYPE_ARGS(AVM1Transform)) \
{ \
	auto clip = _this->getClip(); \
	if (!clip.isNull() && !args.empty()) \
		set#func(act, _this, args[0], clip); \
	return AVM1Value::undefinedVal; \
})

#define TRANSFORM_PROP_PROTO(name, func) AVM1Decl \
( \
	#name, \
	TRANSFORM_GETTER(func), \
	TRANSFORM_SETTER(func), \
	AVM1PropFlags::Version8 \
)

#define TRANSFORM_GETTER_BODY(name) AVM1_GETTER_TYPE_BODY \
( \
	AVM1Transform, \
	AVM1Transform, \
	name, \
	GcPtr<DisplayObject> clip \
)

#define TRANSFORM_SETTER_BODY(name) AVM1_SETTER_TYPE_BODY \
( \
	AVM1Transform, \
	AVM1Transform, \
	name, \
	GcPtr<DisplayObject> clip \
)

static constexpr auto protoDecls =
{
	TRANSFORM_PROP_PROTO(matrix, Matrix),
	TRANSFORM_PROP_PROTO(concatenatedMatrix, ConcatenatedMatrix),
	TRANSFORM_PROP_PROTO(colorTransform, ColorTransform),
	TRANSFORM_PROP_PROTO(concatenatedColorTransform, ConcatenatedColorTransform), 
	TRANSFORM_PROP_PROTO(pixelBounds, PixelBounds),
};

GcPtr<AVM1SystemClass> AVM1Transform::createClass
(
	AVM1DeclContext& ctx,
	const GcPtr<AVM1Object>& superProto
)
{
	auto _class = ctx.makeNativeClass(ctor, nullptr, superProto);
	ctx.definePropsOn(_class->proto, protoDecls);
	return _class;
}

AVM1_FUNCTION_BODY(AVM1Transform, ctor)
{
	// NOTE: `Transform`'s constructor only allows exactly 1 argument.
	if (args.size() != 1 || args[0].isPrimitive())
		return AVM1Value::undefinedVal;

	return _this.constCast() = NEW_GC_PTR(act.getGcCtx(), AVM1Transform
	(
		act,
		args[0].tryAs<AVM1MovieClipRef>(act)
	));
}

TRANSFORM_GETTER_BODY(Matrix)
{
	return AVM1Matrix::toValue(act, clip->getMatrix());
}

TRANSFORM_SETTER_BODY(Matrix)
{
	static constexpr auto mtxProps =
	{
		"a",
		"b",
		"c",
		"d",
		"tx",
		"ty",
	};

	auto obj = value.toObject(act);

	// NOTE: Assignments only occur on `Object`s with `Matrix` properties
	// (`[a-d]`, `t[xy]`).
	bool isMtx = std::all_of
	(
		mtxProps.begin(),
		mtxProps.end(),
		[&](const char* str) { return obj.hasOwnProp(act, str); }
	);

	if (isMtx)
	{
		clip->setMatrix(AVM1Matrix::objToMatrix(act, obj));
		clip->transformedByActionScript = true;
	}
}

TRANSFORM_GETTER_BODY(ConcatenatedMatrix)
{
	// TODO: Testing done by the Ruffle team has shown that
	// `concatenatedMatrix` *doesn't* include the `scrollRect` of the
	// clip itself, but *does* include the `scrollRect` of it's  parents.
	return AVM1Matrix::fromMATRIX
	(
		act,
		clip->getConcatenatedMatrix(true, false)
	);
}

TRANSFORM_GETTER_BODY(ColorTransform)
{
	return NEW_GC_PTR(act.getGcCtx(), AVM1ColorTransform
	(
		act,
		clip->colorTransform
	));
}

TRANSFORM_SETTER_BODY(ColorTransform)
{
	auto ct = value.toObject(act)->as<AVM1ColorTransform>(act);
	if (!ct.isNull())
	{
		clip->colorTransform = *ct;
		clip->transformedByActionScript = true;
	}
}

TRANSFORM_GETTER_BODY(ConcatenatedColorTransform)
{
	auto ct = clip->colorTransform;	
	for (auto node = clip->getAVM1Parent(); !node.isNull(); node = node->getParent())
		ct = node->colorTransform.multiplyTransform(ct);

	return NEW_GC_PTR(act.getGcCtx(), AVM1ColorTransform(act, ct));
}

TRANSFORM_GETTER_BODY(PixelBounds)
{
	auto stageBounds = clip->getStageBounds();

	// NOTE: If the stage bounds are invalid, then `pixelBounds` returns
	// an all-zero `Rectangle`.
	return AVM1MovieClip::makeRect
	(
		act,
		stageBounds.isValid() ? stageBounds : RectF {}
	);
}

#undef TRANSFORM_SETTER_BODY
#undef TRANSFORM_GETTER_BODY
#undef TRANSFORM_PROP_PROTO
#undef TRANSFORM_SETTER
#undef TRANSFORM_GETTER
