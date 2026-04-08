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

#include "display_object/DisplayObject.h"
#include "display_object/Video.h"
#include "gc/context.h"
#include "scripting/avm1/activation.h"
#include "scripting/avm1/function.h"
#include "scripting/avm1/object/object.h"
#include "scripting/avm1/prop.h"
#include "scripting/avm1/prop_decl.h"
#include "scripting/avm1/runtime.h"
#include "scripting/avm1/toplevel/NetStream.h"
#include "scripting/avm1/toplevel/Video.h"
#include "tiny_string.h"
#include "utils/array.h"
#include "utils/optional.h"

using namespace lightspark;

// Based on both Ruffle's `avm1::globals::video` crate, and Gnash's
// implementation of `Video`.

AVM1Video::AVM1Video(AVM1Activation& act, GcPtr<Video> video) :
AVM1DisplayObject
(
	act,
	video,
	act.getPrototypes().video->proto
) {}

constexpr auto protoFlags =
(
	AVM1PropFlags::DontEnum |
	AVM1PropFlags::DontDelete |
	AVM1PropFlags::Version6
);

using AVM1Video;

static constexpr auto protoDecls =
{
	AVM1_FUNCTION_TYPE_PROTO(AVM1Video, attachVideo, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1Video, clear, protoFlags),
	AVM1_PROPERTY_TYPE_PROTO(AVM1Video, deblocking, Deblocking, protoFlags),
	AVM1_PROPERTY_TYPE_PROTO(AVM1Video, smoothing, Smoothing, protoFlags),
	AVM1_GETTER_TYPE_PROTO(AVM1Video, width, Width, protoFlags),
	AVM1_GETTER_TYPE_PROTO(AVM1Video, height, Height, protoFlags)
};

GcPtr<AVM1SystemClass> AVM1Video::createClass
(
	AVM1DeclContext& ctx,
	const GcPtr<AVM1Object>& superProto
)
{
	auto _class = ctx.makeEmptyClass(superProto);
	ctx.definePropsOn(_class->proto, protoDecls);
	return _class;
}

AVM1_FUNCTION_TYPE_BODY(AVM1Video, AVM1Video, attachVideo)
{
	NullableGcPtr<AVM1NetStream> source;
	AVM1_ARG_UNPACK(source);

	if (!source.isNull())
		_this->attachNetStream(source);
	else
	{
		LOG
		(
			LOG_ERROR,
			"Video.attachVideo(): "
			"`source` must be a `NetStream`"
		);
	}

	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_TYPE_BODY(AVM1Video, AVM1Video, clear)
{
	_this->clear();
	return AVM1Value::undefinedVal;
}

AVM1_GETTER_TYPE_BODY(AVM1Video, AVM1Video, Deblocking)
{
	return number_t(_this->getDeblockingFilter());
}

AVM1_SETTER_TYPE_BODY(AVM1Video, AVM1Video, Deblocking)
{
	_this->setDeblockingFilter(value.toUint32(act));
}

AVM1_GETTER_TYPE_BODY(AVM1Video, AVM1Video, Smoothing)
{
	return _this->getSmoothing();
}

AVM1_SETTER_TYPE_BODY(AVM1Video, AVM1Video, Smoothing)
{
	_this->setSmoothing(value.toBool(act.getSwfVersion()));
}

AVM1_GETTER_TYPE_BODY(AVM1Video, AVM1Video, Width)
{
	return number_t(_this->getWidth());
}

AVM1_GETTER_TYPE_BODY(AVM1Video, AVM1Video, Height)
{
	return number_t(_this->getHeight());
}
