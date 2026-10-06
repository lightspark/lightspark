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

#include "backends/net_stream.h"
#include "gc/context.h"
#include "parsing/amf0_generator.h"
#include "scripting/avm1/activation.h"
#include "scripting/avm1/amf.h"
#include "scripting/avm1/prop.h"
#include "scripting/avm1/prop_decl.h"
#include "scripting/avm1/toplevel/NetStream.h"
#include "swf.h"
#include "tiny_string.h"
#include "utils/array.h"
#include "utils/span.h"
#include "utils/timespec.h"

using namespace lightspark;

// Based on Ruffle's `avm1::globals::netstream` crate.

AVM1NetStream::AVM1NetStream(AVM1Activation& act) :
AVM1Object(act),
stream(NEW_GC_PTR(act.getGcCtx(), NetStream(this)))
{
}

void AVM1NetStream::triggerStatusEvent
(
	SystemState* sys,
	const StatusEventValues& values
)
{
	const auto& ctx = *sys->getAVM1Ctx();
	AVM1Activation act
	(
		ctx,
		"NetStream.onStatus()",
		sys->stage->getRoot()
	);

	auto infoObj = NEW_GC_PTR(act.getGcCtx(), AVM1Object(act));
	for (const auto& pair : values)
		infoObj->setProp(act, pair.first, pair.second);

	try
	{
		(void)callMethod
		(
			act,
			"onStatus",
			{ infoObj },
			AVM1ExecutionReason::Special
		);
	}
	catch (std::exception& e)
	{
		LOG
		(
			LOG_ERROR,
			"`AVM1NetStream::triggerStatusEvent()`: Exception thrown "
			"while handling an `onStatus()` event from a `NetStream`. "
			"Reason: " << e.what()
		);
	}
}

void AVM1NetStream::handleScriptData
(
	SystemState* sys,
	const tiny_string& varName,
	const AMFValue& varValue
)
{
	const auto& ctx = *sys->getAVM1Ctx();
	AVM1Activation act
	(
		ctx,
		tiny_string("FLV: ") + varName,
		sys->stage->getRoot()
	);

	try
	{
		(void)callMethod
		(
			act,
			varName,
			{ deserializeAVM1Value(act, varValue) }
			AVM1ExecutionReason::Special
		);
	}
	catch (std::exception& e)
	{
		LOG
		(
			LOG_ERROR,
			"`AVM1NetStream::handleScriptData()`: Exception thrown "
			"while running a script data handler from a `NetStream`. "
			"Reason: " << e.what()
		);
	}
}

constexpr auto protoFlags =
(
	AVM1PropFlags::DontEnum |
	AVM1PropFlags::DontDelete
);

using AVM1NetStream;

static constexpr auto protoDecls =
{
	AVM1_GETTER_TYPE_PROTO(AVM1NetStream, BufferLength),
	AVM1_PROPERTY_TYPE_PROTO(AVM1NetStream, BufferTime),
	AVM1_GETTER_TYPE_PROTO(AVM1NetStream, BytesLoaded),
	AVM1_GETTER_TYPE_PROTO(AVM1NetStream, BytesTotal),
	AVM1_GETTER_TYPE_PROTO(AVM1NetStream, Time),
	AVM1_FUNCTION_TYPE_PROTO(AVM1NetStream, play, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1NetStream, pause, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1NetStream, seek, protoFlags),
	AVM1_SETTER_FUNC_TYPE_PROTO(AVM1NetStream, BufferTime, protoFlags)
};

GcPtr<AVM1SystemClass> AVM1NetStream::createClass
(
	AVM1DeclContext& ctx,
	const GcPtr<AVM1Object>& superProto
)
{
	auto _class = ctx.makeClass(ctor, superProto);

	ctx.definePropsOn(_class->proto, protoDecls);
	return _class;
}

AVM1_FUNCTION_BODY(AVM1NetStream, ctor)
{
	_this.constCast() = NEW_GC_PTR
	(
		act.getGcCtx(),
		AVM1NetStream(act)
	);

	return AVM1Value::undefinedVal;
}

AVM1_GETTER_TYPE_BODY(AVM1NetStream, AVM1NetStream, BufferLength)
{
	return number_t(_this->stream->getBufferLength());
}

AVM1_GETTER_TYPE_BODY(AVM1NetStream, AVM1NetStream, BufferTime)
{
	return number_t(_this->stream->getBufferTime().toMs());
}

AVM1_GETTER_TYPE_BODY(AVM1NetStream, AVM1NetStream, BytesLoaded)
{
	return number_t(_this->stream->getBytesLoaded());
}

AVM1_GETTER_TYPE_BODY(AVM1NetStream, AVM1NetStream, BytesTotal)
{
	return number_t(_this->stream->getBytesTotal());
}

AVM1_GETTER_TYPE_BODY(AVM1NetStream, AVM1NetStream, Time)
{
	return number_t(_this->stream->getTime().toMs());
}

AVM1_FUNCTION_TYPE_BODY(AVM1NetStream, AVM1NetStream, play)
{
	tiny_string name;
	AVM1_ARG_UNPACK(name);

	_this->stream->play(act.getSys(), name);
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_TYPE_BODY(AVM1NetStream, AVM1NetStream, pause)
{
	if (args.empty() || args[0].is<UndefinedVal>())
		_this->stream->togglePaused(act.getSys());
	else if (args[0].toBool(act.getSwfVersion()))
		_this->stream->pause(act.getSys(), true);
	else
		_this->stream->resume(act.getSys());
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_TYPE_BODY(AVM1NetStream, AVM1NetStream, seek)
{
	number_t offset;
	AVM1_ARG_UNPACK(offset);

	_this->stream->seek
	(
		act.getSys(),
		TimeSpec::fromFloat(offset),
		false
	);
	return AVM1Value::undefinedVal;
}

AVM1_SETTER_TYPE_BODY(AVM1NetStream, AVM1NetStream, BufferTime)
{
	auto bufferTime = TimeSpec::fromFloat(value.toNumber(act));
	_this->stream->setBufferTime(bufferTime);
	return AVM1Value::undefinedVal;
}
