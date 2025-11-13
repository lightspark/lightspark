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

#include <initializer_list>

#include "display_object/DisplayObject.h"
#include "gc/context.h"
#include "scripting/avm1/activation.h"
#include "scripting/avm1/function.h"
#include "scripting/avm1/prop.h"
#include "scripting/avm1/prop_decl.h"
#include "scripting/avm1/runtime.h"
#include "swftypes.h"
#include "tiny_string.h"
#include "utils/array.h"

using namespace lightspark;

// Based on Ruffle's `avm1::globals::color` crate.

constexpr auto protoFlags =
(
	AVM1PropFlags::DontEnum |
	AVM1PropFlags::DontDelete |
	AVM1PropFlags::ReadOnly
);

using AVM1Color;

#define AVM1_PROPERTY_FUNC_PROTO(name, ...) \
	AVM1Decl("get"#name, AVM1_GETTER_FUNC(name), ##__VA_ARGS__), \
	AVM1Decl("set"#name, AVM1_SETTER_FUNC(name), ##__VA_ARGS__)

static constexpr auto protoDecls =
{
	AVM1_PROPERTY_FUNC_PROTO(RGB, protoFlags),
	AVM1_PROPERTY_FUNC_PROTO(Transform, protoFlags)
};

#undef AVM1_PROP_FUNC_PROTO

GcPtr<AVM1SystemClass> AVM1Color::createClass
(
	AVM1DeclContext& ctx,
	const GcPtr<AVM1Object>& superProto
)
{
	auto _class = ctx.makeClass(ctor, superProto);
	ctx.definePropsOn(_class->proto, protoDecls);
	return _class;
}

AVM1_FUNCTION_BODY(AVM1Color, ctor)
{
	// Set the undocumented `target` property.
	_this->defineValue
	(
		"target",
		// The target `DisplayObject` that this `Color` will modify.
		!args.empty() ? args[0] : AVM1Value::undefinedVal,
		protoFlags
	);

	return AVM1Value::undefinedVal;
}

NullableGcPtr<DisplayObject> AVM1Color::getTarget(AVM1_GETTER_ARGS)
{
	// NOTE: The target path is resolved based on the active `tellTarget()`
	// clip of the stack frame. This means that calls to the same `Color`
	// object might set the colour of a different clip, depending on
	// which timeline it was called from!
	auto target = _this->getProp(act, "target");

	// Don't do anything, if `target` is `undefined`, or empty.
	if (target.is<UndefinedVal>())
		return NullGc;
	return act.resolveTargetClip
	(
		act.getTargetOrRootClip(),
		target,
		false
	);
}

AVM1_GETTER_BODY(AVM1Color, RGB)
{
	auto target = getTarget(act, _this);
	if (target.isNull())
		return AVM1Value::undefinedVal;

	auto cxform = target->getColorTransform();

	auto redOffset = int32_t(cxform.RedAddTerm) << 16;
	auto greenOffset = int32_t(cxform.GreenAddTerm) << 8;
	auto blueOffset = int32_t(cxform.BlueAddTerm);

	return number_t(redOffset | greenOffset | blueOffset);
}

AVM1_GETTER_BODY(AVM1Color, Transform)
{
	auto target = getTarget(act, _this);
	if (target.isNull())
		return AVM1Value::undefinedVal;

	auto cxform = target->getColorTransform();
	auto ret = NEW_GC_PTR(act.getGcCtx(), AVM1Object
	(
		act.getGcCtx(),
		act.getPrototypes().object->proto
	));

	ret->setProp(act, "ra", number_t(cxform.RedMultTerm) * 100);
	ret->setProp(act, "ga", number_t(cxform.GreenMultTerm) * 100);
	ret->setProp(act, "ba", number_t(cxform.BlueMultTerm) * 100);
	ret->setProp(act, "aa", number_t(cxform.AlphaMultTerm) * 100);

	ret->setProp(act, "rb", number_t(cxform.RedAddTerm));
	ret->setProp(act, "gb", number_t(cxform.GreenAddTerm));
	ret->setProp(act, "bb", number_t(cxform.BlueAddTerm));
	ret->setProp(act, "ab", number_t(cxform.AlphaAddTerm));

	return ret;
}

AVM1_SETTER_BODY(AVM1Color, RGB)
{
	auto target = getTarget(act, _this);
	if (target.isNull())
		return;

	target->setTransformedByActionScript(true);
	target->setHasChanged(true);
	target->requestInvalidation(act.getSys());

	auto colorTransform = target->getColorTransform();
	uint32_t rgb = value.toInt32(act);

	colorTransform.RedMultTerm = 0;
	colorTransform.GreenMultTerm = 0;
	colorTransform.BlueMultTerm = 0;

	colorTransform.RedAddTerm = uint8_t(rgb >> 16);
	colorTransform.GreenAddTerm = uint8_t(rgb >> 8);
	colorTransform.BlueAddTerm = uint8_t(rgb);
	target->setColorTransform(colorTransform);
}

AVM1_SETTER_BODY(AVM1Color, Transform)
{
	auto target = getTarget(act, _this);
	if (target.isNull())
		return;

	target->setTransformedByActionScript(true);
	target->setHasChanged(true);
	target->requestInvalidation(act.getSys());

	auto cxform = target->getColorTransform();
	auto transform = value.toObject(act);

	auto setMultTerm = [&](const tiny_string& name FIXED8& out)
	{
		// NOTE: The arguments are only set, if the proerty exists on the
		// object itself (excluding the prototype).
		if (!transform->hasOwnProp(act, name))
			return;
		out = transform->getProp
		(
			act,
			name
		).toInt16(act) * 256 / 100;
	};

	auto setAddTerm = [&](const tiny_string& name int16_t& out)
	{
		// NOTE: The arguments are only set, if the proerty exists on the
		// object itself (excluding the prototype).
		if (!transform->hasOwnProp(act, name))
			return;
		out = transform->getProp(act, name).toInt16(act);
	};

	setMultTerm("ra", cxform.RedMultTerm);
	setMultTerm("ga", cxform.GreenMultTerm);
	setMultTerm("ba", cxform.BlueMultTerm);
	setMultTerm("aa", cxform.AlphaMultTerm);

	setAddTerm("rb", cxform.RedAddTerm);
	setAddTerm("gb", cxform.GreenAddTerm);
	setAddTerm("bb", cxform.BlueAddTerm);
	setAddTerm("ab", cxform.AlphaAddTerm);
	target->setColorTransform(cxform);
}
