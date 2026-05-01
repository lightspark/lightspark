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

#include "gc/context.h"
#include "scripting/avm1/activation.h"
#include "scripting/avm1/prop_decl.h"
#include "scripting/avm1/toplevel/Camera.h"
#include "swf.h"

using namespace lightspark;

// Based on Ruffle's `avm1::globals::camera` crate.

AVM1Camera::AVM1Camera(AVM1Activation& act) : AVM1Object
(
	act,
	act.getPrototypes().camera->proto
) {}

constexpr auto protoFlags =
(
	AVM1PropFlags::DontDelete |
	AVM1PropFlags::ReadOnly |
	AVM1PropFlags::Version6
);

using AVM1Camera;

static constexpr auto protoDecls =
{
	AVM1_FUNCTION_TYPE_PROTO(AVM1Camera, setLoopback, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1Camera, setMode, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1Camera, setMotionLevel, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1Camera, setQuality, protoFlags),
	AVM1_GETTER_TYPE_PROTO(AVM1Camera, activityLevel, ActivityLevel, protoFlags),
	AVM1_GETTER_TYPE_PROTO(AVM1Camera, bandwidth, Bandwidth, protoFlags),
	AVM1_GETTER_TYPE_PROTO(AVM1Camera, currentFps, CurrentFps, protoFlags),
	AVM1_GETTER_TYPE_PROTO(AVM1Camera, fps, Fps, protoFlags),
	AVM1_GETTER_TYPE_PROTO(AVM1Camera, height, Height, protoFlags),
	AVM1_GETTER_TYPE_PROTO(AVM1Camera, index, Index, protoFlags),
	AVM1_GETTER_TYPE_PROTO(AVM1Camera, keyFrameInterval, KeyFrameInterval, protoFlags),
	AVM1_GETTER_TYPE_PROTO(AVM1Camera, loopback, Loopback, protoFlags),
	AVM1_GETTER_TYPE_PROTO(AVM1Camera, motionLevel, MotionLevel, protoFlags),
	AVM1_GETTER_TYPE_PROTO(AVM1Camera, motionTimeout, MotionTimeout, protoFlags),
	AVM1_GETTER_TYPE_PROTO(AVM1Camera, muted, Muted, protoFlags),
	AVM1_GETTER_TYPE_PROTO(AVM1Camera, name, Name, protoFlags),
	AVM1_GETTER_TYPE_PROTO(AVM1Camera, quality, Quality, protoFlags),
	AVM1_GETTER_TYPE_PROTO(AVM1Camera, width, Width, protoFlags)
};

static constexpr auto objDecls =
{
	AVM1_FUNCTION_PROTO(get),
	AVM1_GETTER_PROTO(names, Names)
};

GcPtr<AVM1SystemClass> AVM1Camera::createClass
(
	AVM1DeclContext& ctx,
	const GcPtr<AVM1Object>& superProto
)
{
	auto _class = ctx.makeClass(ctor, superProto);

	ctx.definePropsOn(_class->proto, protoDecls);
	ctx.definePropsOn(_class->ctor, objDecls);
	return _class;
}

AVM1_FUNCTION_BODY(AVM1Camera, ctor)
{
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_TYPE_BODY(AVM1Camera, AVM1Camera, setLoopback)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `Camera.setLoopback()` is a stub.");
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_TYPE_BODY(AVM1Camera, AVM1Camera, setMode)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `Camera.setMode()` is a stub.");
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_TYPE_BODY(AVM1Camera, AVM1Camera, setMotionLevel)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `Camera.setMotionLevel()` is a stub.");
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_TYPE_BODY(AVM1Camera, AVM1Camera, setQuality)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `Camera.setQuality()` is a stub.");
	return AVM1Value::undefinedVal;
}

AVM1_GETTER_TYPE_BODY(AVM1Camera, AVM1Camera, ActivityLevel)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `Camera.activityLevel` is a stub.");
	return -1;
}

AVM1_GETTER_TYPE_BODY(AVM1Camera, AVM1Camera, Bandwidth)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `Camera.bandwidth` is a stub.");
	return 0;
}

AVM1_GETTER_TYPE_BODY(AVM1Camera, AVM1Camera, CurrentFps)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `Camera.currentFps` is a stub.");
	return 15;
}

AVM1_GETTER_TYPE_BODY(AVM1Camera, AVM1Camera, Fps)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `Camera.fps` is a stub.");
	return 15;
}

AVM1_GETTER_TYPE_BODY(AVM1Camera, AVM1Camera, Height)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `Camera.height` is a stub.");
	return 0;
}

AVM1_GETTER_TYPE_BODY(AVM1Camera, AVM1Camera, Index)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `Camera.index` is a stub.");
	return 0;
}

AVM1_GETTER_TYPE_BODY(AVM1Camera, AVM1Camera, KeyFrameInterval)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `Camera.keyFrameInterval` is a stub.");
	return 15;
}

AVM1_GETTER_TYPE_BODY(AVM1Camera, AVM1Camera, Loopback)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `Camera.loopback` is a stub.");
	return false;
}

AVM1_GETTER_TYPE_BODY(AVM1Camera, AVM1Camera, MotionLevel)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `Camera.motionLevel` is a stub.");
	return 50;
}

AVM1_GETTER_TYPE_BODY(AVM1Camera, AVM1Camera, MotionTimeout)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `Camera.motionTimeout` is a stub.");
	return 2000;
}

AVM1_GETTER_TYPE_BODY(AVM1Camera, AVM1Camera, Muted)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `Camera.muted` is a stub.");
	return false;
}

AVM1_GETTER_TYPE_BODY(AVM1Camera, AVM1Camera, Name)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `Camera.name` is a stub.");
	return "";
}

AVM1_GETTER_TYPE_BODY(AVM1Camera, AVM1Camera, Quality)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `Camera.quality` is a stub.");
	return 0;
}

AVM1_GETTER_TYPE_BODY(AVM1Camera, AVM1Camera, Width)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `Camera.width` is a stub.");
	return 0;
}

AVM1_FUNCTION_BODY(AVM1Camera, get)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `Camera.get()` is a stub.");
	return AVM1Value::nullVal;
}

AVM1_GETTER_BODY(AVM1Camera, Names)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `Camera.names` is a stub.");
	return AVM1Value::nullVal;
}
