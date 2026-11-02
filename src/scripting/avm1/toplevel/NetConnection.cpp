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

#include "gc/context.h"
#include "parsing/amf0_generator.h"
#include "scripting/avm1/activation.h"
#include "scripting/avm1/amf.h"
#include "scripting/avm1/prop.h"
#include "scripting/avm1/prop_decl.h"
#include "scripting/avm1/toplevel/NetConnection.h"
#include "swf.h"
#include "tiny_string.h"
#include "utils/array.h"
#include "utils/span.h"

using namespace lightspark;

// Based on Ruffle's `avm1::globals::netconnection` crate.

void AVM1NetConnection::sendStatus
(
	SystemState* sys,
	const Optional<tiny_string>& code
)
{
	const auto& ctx = *sys->getAVM1Ctx();
	auto rootClip = sys->stage->getRoot();
	if (rootClip.isNull())
	{
		LOG
		(
			LOG_ERROR,
			"Ignoring `NetConnection.onStatus()` because there's no "
			"root clip."
		);
		return;
	}

	AVM1Activation act(ctx, "NetConnection.onStatus()", rootClip);

	using ArgsVec = std::vector<AVM1Value>;
	auto args = code.transformOr(ArgsVec {}, [&](const auto& code)
	{
		auto ctor = act.getPrototypes().object->ctor;
		auto event = ctor->construct(act, {}).toObject(act);
		event->setProp(act, "code", )
		event->setProp(act, "level", status);
		return { event };
	});

	(void)callMethod
	(
		act,
		"onStatus",
		args,
		AVM1ExecutionReason::Special
	);
}

void AVM1NetConnection::sendCallback
(
	SystemState* sys,
	bool isStatus,
	const AMFValue& message
)
{
	const auto& ctx = *sys->getAVM1Ctx();
	auto rootClip = sys->stage->getRoot();
	if (rootClip.isNull())
	{
		LOG
		(
			LOG_ERROR,
			"Ignoring `NetConnection` response because there's no root "
			"clip."
		);
		return;
	}

	AVM1Activation act(ctx, "`NetConnection` response", rootClip);
	(void)callMethod
	(
		act,
		isStatus ? "onStatus" : "onResult",
		{ deserializeAVM1Value(act, message) },
		AVM1ExecutionReason::Special
	);
}

constexpr auto protoFlags =
(
	AVM1PropFlags::DontEnum |
	AVM1PropFlags::DontDelete
);

using AVM1NetConnection;

static constexpr auto protoDecls =
{
	AVM1_GETTER_TYPE_PROTO(AVM1NetConnection, isConnected, IsConnected),
	AVM1_GETTER_TYPE_PROTO(AVM1NetConnection, protocol, Protocol),
	AVM1_GETTER_TYPE_PROTO(AVM1NetConnection, uri, URI),
	AVM1_FUNCTION_TYPE_PROTO(AVM1NetConnection, addHeader, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1NetConnection, call, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1NetConnection, close, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1NetConnection, connect, protoFlags)
};

GcPtr<AVM1SystemClass> AVM1NetConnection::createClass
(
	AVM1DeclContext& ctx,
	const GcPtr<AVM1Object>& superProto
)
{
	auto _class = ctx.makeClass(ctor, superProto);

	ctx.definePropsOn(_class->proto, protoDecls);
	return _class;
}

AVM1_FUNCTION_BODY(AVM1NetConnection, ctor)
{
	_this.constCast() = NEW_GC_PTR
	(
		act.getGcCtx(),
		AVM1NetConnection(act)
	);

	return AVM1Value::undefinedVal;
}

AVM1_GETTER_TYPE_BODY(AVM1NetConnection, AVM1NetConnection, IsConnected)
{
	return act.getSys()->netConnectionManager->isConnected(_this);
}

AVM1_GETTER_TYPE_BODY(AVM1NetConnection, AVM1NetConnection, Protocol)
{
	auto netConnectionMgr = act.getSys()->netConnectionManager;
	const auto& undefVal = AVM1Value::undefinedVal;
	return netConnectionMgr->tryGetProtocol(_this).valueOr(undefVal);
}

AVM1_GETTER_TYPE_BODY(AVM1NetConnection, AVM1NetConnection, URI)
{
	auto netConnectionMgr = act.getSys()->netConnectionManager;
	const auto& undefVal = AVM1Value::undefinedVal;
	return netConnectionMgr->tryGetURI(_this).valueOr(undefVal);
}

AVM1_FUNCTION_TYPE_BODY(AVM1NetConnection, AVM1NetConnection, addHeader)
{
	auto netConnectionMgr = act.getSys()->netConnectionManager;
	AVM1_ARG_UNPACK_NAMED(unpacker);

	tiny_string name;
	bool mustUnderstand;
	unpacker(name)(mustUnderstand, true);

	const auto& nullVal = AVM1Value::nullVal;
	netConnectionMgr->setHeader
	(
		_this,
		name,
		mustUnderstand,
		serializeAVM1Value(act, unpacker.unpackOr(nullVal))
	);
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_TYPE_BODY(AVM1NetConnection, AVM1NetConnection, call)
{
	auto netConnectionMgr = act.getSys()->netConnectionManager;
	AVM1_ARG_UNPACK_NAMED(unpacker);

	tiny_string command;
	unpacker(command);

	if (!netConnectionMgr->hasConnection(_this))
		return AVM1Value::undefinedVal;

	NullableGcPtr<AVM1Object> responder;
	unpacker(responder);

	auto argSpan = makeSpan(args).trySubSpan(2);
	std::vector<AMFValue> _args;
	_args.reserve(argSpan.size());

	for (const auto& arg : argSpan)
		_args.push_back(serializeAVM1Value(act, arg));

	_this->sendAVM1(command, AMFStrictArray(_args), responder);
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_TYPE_BODY(AVM1NetConnection, AVM1NetConnection, close)
{
	auto netConnectionMgr = act.getSys()->netConnectionManager;
	netConnectionMgr->close(_this, true);
	_this->disconnect(act);
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_TYPE_BODY(AVM1NetConnection, AVM1NetConnection, connect)
{
	auto netConnectionMgr = act.getSys()->netConnectionManager;
	if (args.empty() || args[0].isNullOrUndefined())
	{
		// Local connection.
		netConnectionMgr->connect(_this);
		return AVM1Value::undefinedVal;
	}

	// Remote connection.
	netConnectionMgr->connect(_this, args[0].toString(act));
	return AVM1Value::undefinedVal;
}
