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
#include "scripting/avm1/toplevel/Microphone.h"
#include "swf.h"

using namespace lightspark;

// Based on Ruffle's `avm1::globals::microphone` crate.

AVM1Microphone::AVM1Microphone(AVM1Activation& act) : AVM1Object
(
	act,
	act.getPrototypes().microphone->proto
) {}

constexpr auto protoFlags =
(
	AVM1PropFlags::DontDelete |
	AVM1PropFlags::ReadOnly |
	AVM1PropFlags::Version6
);

constexpr auto protoFlagsV10 =
(
	AVM1PropFlags::DontDelete |
	AVM1PropFlags::ReadOnly |
	AVM1PropFlags::Version10
);

using AVM1Microphone;

static constexpr auto protoDecls =
{
	AVM1_FUNCTION_TYPE_PROTO(AVM1Microphone, setSilenceLevel, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1Microphone, setCodec, protoFlagsV10),
	AVM1_FUNCTION_TYPE_PROTO(AVM1Microphone, setEncodeQuality, protoFlagsV10),
	AVM1_FUNCTION_TYPE_PROTO(AVM1Microphone, setFramesPerPacket, protoFlagsV10),
	AVM1_FUNCTION_TYPE_PROTO(AVM1Microphone, setRate;
	AVM1_FUNCTION_TYPE_PROTO(AVM1Microphone, setGain, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1Microphone, setUseEchoSuppression, protoFlags),
	AVM1_GETTER_TYPE_PROTO(AVM1Microphone, activityLevel, ActivityLevel, protoFlags),
	AVM1_GETTER_TYPE_PROTO(AVM1Microphone, codec, Codec, protoFlagsV10),
	AVM1_GETTER_TYPE_PROTO(AVM1Microphone, encodeQuality, EncodeQuality, protoFlagsV10),
	AVM1_GETTER_TYPE_PROTO(AVM1Microphone, framesPerPacket, FramesPerPacket, protoFlagsV10),
	AVM1_GETTER_TYPE_PROTO(AVM1Microphone, gain, Gain, protoFlags),
	AVM1_GETTER_TYPE_PROTO(AVM1Microphone, index, Index, protoFlags),
	AVM1_GETTER_TYPE_PROTO(AVM1Microphone, name, Name, protoFlags),
	AVM1_GETTER_TYPE_PROTO(AVM1Microphone, rate, Rate, protoFlags),
	AVM1_GETTER_TYPE_PROTO(AVM1Microphone, silenceLevel, SilenceLevel, protoFlags),
	AVM1_GETTER_TYPE_PROTO(AVM1Microphone, silenceTimeout, SilenceTimeout, protoFlags),
	AVM1_GETTER_TYPE_PROTO(AVM1Microphone, useEchoSuppression, UseEchoSuppression, protoFlags)
};

static constexpr auto objDecls =
{
	AVM1_FUNCTION_PROTO(get),
	AVM1_GETTER_PROTO(names, Names)
};

GcPtr<AVM1SystemClass> AVM1Microphone::createClass
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

AVM1_FUNCTION_BODY(AVM1Microphone, ctor)
{
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_TYPE_BODY(AVM1Microphone, AVM1Microphone, setSilenceLevel)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `Microphone.setSilenceLevel()` is a stub.");
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_TYPE_BODY(AVM1Microphone, AVM1Microphone, setCodec)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `Microphone.setCodec()` is a stub.");
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_TYPE_BODY(AVM1Microphone, AVM1Microphone, setEncodeQuality)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `Microphone.setEncodeQuality()` is a stub.");
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_TYPE_BODY(AVM1Microphone, AVM1Microphone, setFramesPerPacket)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `Microphone.setFramesPerPacket()` is a stub.");
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_TYPE_BODY(AVM1Microphone, AVM1Microphone, setRate)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `Microphone.setRate()` is a stub.");
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_TYPE_BODY(AVM1Microphone, AVM1Microphone, setGain)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `Microphone.setGain()` is a stub.");
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_TYPE_BODY(AVM1Microphone, AVM1Microphone, setUseEchoSuppression)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `Microphone.setUseEchoSuppression()` is a stub.");
	return AVM1Value::undefinedVal;
}

AVM1_GETTER_TYPE_BODY(AVM1Microphone, AVM1Microphone, ActivityLevel)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `Microphone.activityLevel` is a stub.");
	return -1;
}

AVM1_GETTER_TYPE_BODY(AVM1Microphone, AVM1Microphone, Codec)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `Microphone.codec` is a stub.");
	return "NELLYMOSER";
}

AVM1_GETTER_TYPE_BODY(AVM1Microphone, AVM1Microphone, EncodeQuality)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `Microphone.encodeQuality` is a stub.");
	return 0;
}

AVM1_GETTER_TYPE_BODY(AVM1Microphone, AVM1Microphone, FramesPerPacket)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `Microphone.framesPerPacket` is a stub.");
	return 2;
}

AVM1_GETTER_TYPE_BODY(AVM1Microphone, AVM1Microphone, Gain)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `Microphone.gain` is a stub.");
	return 50;
}

AVM1_GETTER_TYPE_BODY(AVM1Microphone, AVM1Microphone, Index)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `Microphone.index` is a stub.");
	return 0;
}

AVM1_GETTER_TYPE_BODY(AVM1Microphone, AVM1Microphone, Muted)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `Microphone.muted` is a stub.");
	return false;
}

AVM1_GETTER_TYPE_BODY(AVM1Microphone, AVM1Microphone, Name)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `Microphone.name`'s getter is a stub.");
	return "";
}

AVM1_GETTER_TYPE_BODY(AVM1Microphone, AVM1Microphone, Rate)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `Microphone.rate` is a stub.");
	return 8;
}

AVM1_GETTER_TYPE_BODY(AVM1Microphone, AVM1Microphone, SilenceLevel)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `Microphone.silenceLevel` is a stub.");
	return 10;
}

AVM1_GETTER_TYPE_BODY(AVM1Microphone, AVM1Microphone, SilenceTimeout)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `Microphone.silenceTimeout` is a stub.");
	return 2000;
}

AVM1_GETTER_TYPE_BODY(AVM1Microphone, AVM1Microphone, UseEchoSuppression)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `Microphone.useEchoSuppression` is a stub.");
	return false;
}

AVM1_FUNCTION_BODY(AVM1Microphone, get)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `Microphone.get()` is a stub.");
	return AVM1Value::nullVal;
}
AVM1_GETTER_BODY(AVM1Microphone, Names)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `Microphone.names` is a stub.");
	return AVM1Value::nullVal;
}
