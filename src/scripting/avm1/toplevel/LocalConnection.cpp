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
#include "scripting/avm1/toplevel/LocalConnection.h"
#include "swf.h"
#include "tiny_string.h"
#include "utils/array.h"
#include "utils/span.h"

using namespace lightspark;

// Based on Ruffle's `avm1::globals::local_connection` crate.

AVM1LocalConnection::AVM1LocalConnection(AVM1Activation& act) : AVM1Object
(
	act,
	act.getPrototypes().localConnection->proto
) {}

bool AVM1LocalConnection::connect(AVM1Activation& act, const tiny_string& name)
{
	if (isConnected())
		return false;

	auto sys = act.getSys();
	connectionName = sys->localConnectionManager->connect
	(
		sys->getRootMovie()->getURL(),
		this,
		name
	);

	return connectionName.hasValue();
}

void AVM1LocalConnection::disconnect(AVM1Activation& act)
{
	if (!connectionName.hasValue())
		return;
	act.getSys()->localConnectionManager->disconnect(*connectionName);
	connectionName.reset();
}

void AVM1LocalConnection::sendStatus
(
	AVM1Context& ctx,
	const GcPtr<AVM1Object>& _this,
	const tiny_string& status
)
{
	auto rootClip = ctx.getSys()->stage->getRoot();
	if (rootClip.isNull())
	{
		LOG
		(
			LOG_ERROR,
			"Ignoring `LocalConnection.onStatus()` because there's no "
			"root clip."
		);
		return;
	}

	AVM1Activation act(ctx, "LocalConnection.onStatus()", rootClip);

	auto ctor = act.getPrototypes().object->ctor;
	auto event = ctor->construct(act, {}).toObject(act);
	event->setProp(act, "level", status);

	(void)_this->callMethod
	(
		act,
		"onStatus",
		{ event },
		AVM1ExecutionReason::Special
	);
}

void AVM1LocalConnection::runMethod
(
	AVM1Activation& act,
	const GcPtr<AVM1Object>& _this,
	const tiny_string& methodName,
	const Span<AMFValue>& args
)
{
	auto rootClip = ctx.getSys()->stage->getRoot();
	if (rootClip.isNull())
	{
		LOG
		(
			LOG_ERROR,
			"Ignoring `LocalConnection` callback because there's no root "
			"clip."
		);
		return;
	}

	AVM1Activation act(ctx, "`LocalConnection` method call", rootClip);

	std::vector<AVM1Value> _args;
	_args.reserve(args.size());

	for (const auto& arg : args)
		_args.push_back(deserializeAVM1Value(act, arg));

	(void)_this->callMethod
	(
		act,
		methodName,
		_args,
		AVM1ExecutionReason::Special
	);
}

constexpr auto protoFlags =
(
	AVM1PropFlags::DontEnum |
	AVM1PropFlags::DontDelete
);

using AVM1LocalConnection;

static constexpr auto protoDecls =
{
	AVM1_FUNCTION_PROTO(domain, protoFlags),
	AVM1_FUNCTION_PROTO(connect, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1LocalConnection, close, protoFlags),
	AVM1_FUNCTION_PROTO(send, protoFlags)
};

GcPtr<AVM1SystemClass> AVM1LocalConnection::createClass
(
	AVM1DeclContext& ctx,
	const GcPtr<AVM1Object>& superProto
)
{
	auto _class = ctx.makeNativeClass(ctor, nullptr, superProto);

	ctx.definePropsOn(_class->proto, protoDecls);
	return _class;
}

AVM1_FUNCTION_BODY(AVM1LocalConnection, ctor)
{
	new (_this.getPtr()) AVM1LocalConnection(act);
	return _this;
}

AVM1_FUNCTION_BODY(AVM1LocalConnection, domain)
{
	return LocalConnectionManager::getDomain
	(
		act.getBaseClip()->getMovie()->getURL()
	);
}

AVM1_FUNCTION_BODY(AVM1LocalConnection, connect)
{
	tiny_string connectionName;

	// NOTE: `AVM1_ARG_UNPACK_STRICT` is used here because Flash Player
	// doesn't do a `String` conversion on `connectionName`, instead, it
	// bails if the value isn't a `String`, returning `false`.
	AVM1_ARG_CHECK_RET(AVM1_ARG_UNPACK_STRICT(connectionName), false);

	if (connectionName.empty() || connectionName.contains(':'))
		return false;

	auto localConnection = _this->as<AVM1LocalConnection>();
	if (localConnection.isNull())
		return AVM1Value::undefinedVal;
	return localConnection->connect(act, connectionName);
}

AVM1_FUNCTION_TYPE_BODY(AVM1LocalConnection, AVM1LocalConnection, close)
{
	_this->disconnect(act);
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_BODY(AVM1LocalConnection, send)
{
	AVM1_ARG_UNPACK_NAMED(unpacker);
	tiny_string connectionName;
	tiny_string methodName;

	// NOTE: `withStrict()` is used here because Flash Player doesn't
	// do a `String` conversion on `{connection,method}Name`, instead,
	// it  bails if either value isn't a `String`, returning `false`.
	AVM1_ARG_CHECK_RET(unpacker.withStrict().unpackArgs
	(
		connectionName,
		methodName
	), false);

	if (connectionName.empty() || methodName.empty())
		return false;

	bool isReservedMethod =
	(
		methodName == "send" ||
		methodName == "connect" ||
		methodName == "close" ||
		methodName == "allowDomain" ||
		methodName == "allowInsecureDomain" ||
		methodName == "domain"
	);

	if (isReservedMethod)
		return false;

	std::vector<AMFValue> amfArgs;
	amfArgs.reserve(unpacker.getSize());

	for (const auto& arg : unpacker.getArgs())
		amfArgs.push_back(serializeAVM1Value(act, arg));

	auto sys = act.getSys();
	sys->localConnectionManager->send
	(
		sys->getRootMovie()->getURL(),
		_this,
		connectionName,
		methodName,
		amfArgs
	);

	return true;
}
