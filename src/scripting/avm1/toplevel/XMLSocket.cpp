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

#include "backends/socket.h"
#include "backends/urlutils.h"
#include "gc/context.h"
#include "scripting/avm1/activation.h"
#include "scripting/avm1/prop.h"
#include "scripting/avm1/prop_decl.h"
#include "scripting/avm1/toplevel/XMLSocket.h"
#include "swf.h"
#include "tiny_string.h"
#include "utils/array.h"
#include "utils/span.h"
#include "utils/timespec.h"

using namespace lightspark;

// Based on Ruffle's `avm1::globals::xml_socket` crate.

AVM1XMLSocket::AVM1XMLSocket(AVM1Activation& act) :
AVM1Object(act),
// NOTE: The default timeout of an `XMLSocket` is 20 seconds.
stream(NEW_GC_PTR(act.getGcCtx(), Socket(this, TimeSpec::fromSec(20))))
{
}

void AVM1XMLSocket::handleConnection
(
	SystemState* sys,
	const ConnectionState& state
)
{
	(void)callMethod
	(
		AVM1Activation(*sys->getAVM1Context(), "XMLSocket"),
		"onConnect",
		{ state == ConnectionState::Connected },
		AVM1ExecutionReason::Special
	);
}

void AVM1XMLSocket::handleData
(
	SystemState* sys,
	const Span<uint8_t>& data
)
{
	auto pos = data.find('\0');
	if (pos == dynExtent)
		return;

	auto buffer = socket->getReadBuffer();
	socket->setReadBuffer(data.getFirst(pos));

	AVM1Activation act(*sys->getAVM1Context(), "XMLSocket");

	auto _data = data.subSpan(pos);
	for (; !_data.empty(); _data = _data.subSpan(pos))
	{
		// Remove the null byte.
		_data = _data.getFirst(1);

		// Create a message from the buffer.
		tiny_string msg(buffer);

		// Call the event handler.
		(void)callMethod
		(
			act,
			"onData",
			{ msg },
			AVM1ExecutionReason::Special
		);

		// Check if we have another null byte.
		pos = _data.find('\0');
		if (pos == dynExtent)
		{
			// No more messages in the payload, exit the loop.
			break;
		}

		auto bufferSpan = _data.getFirst(pos);
		// NOTE: Because the data in `Socket::readBuffer` was already
		// consumed, we don't need to access it again.
		buffer.assign(bufferSpan.begin(), bufferSpan.end());
	}

	// Check if we have any leftover bytes.
	if (_data.empty())
		return;

	// We have leftover bytes, append them to the read buffer, which
	// will be used when the next packet is received.
	socket->appendReadBuffer(_data);
}

void AVM1XMLSocket::handleClose(SystemState* sys)
{
	(void)callMethod
	(
		AVM1Activation(*sys->getAVM1Context(), "XMLSocket"),
		"onClose",
		{},
		AVM1ExecutionReason::Special
	);
}

constexpr auto protoFlags =
(
	AVM1PropFlags::DontEnum |
	AVM1PropFlags::DontDelete
);

using AVM1XMLSocket;

static constexpr auto protoDecls =
{
	AVM1_GETTER_TYPE_PROTO(AVM1XMLSocket, BufferLength),
	AVM1_PROPERTY_TYPE_PROTO(AVM1XMLSocket, timeout, Timeout),
	AVM1_FUNCTION_TYPE_PROTO(AVM1XMLSocket, close),
	AVM1_FUNCTION_TYPE_PROTO(AVM1XMLSocket, connect),
	AVM1_FUNCTION_TYPE_PROTO(AVM1XMLSocket, send),
	AVM1_FUNCTION_TYPE_PROTO(AVM1XMLSocket, onConnect, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1XMLSocket, onClose, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1XMLSocket, onData, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1XMLSocket, onXML, protoFlags)
};

GcPtr<AVM1SystemClass> AVM1XMLSocket::createClass
(
	AVM1DeclContext& ctx,
	const GcPtr<AVM1Object>& superProto
)
{
	auto _class = ctx.makeClass(ctor, superProto);

	ctx.definePropsOn(_class->proto, protoDecls);
	return _class;
}

AVM1_FUNCTION_BODY(AVM1XMLSocket, ctor)
{
	_this.constCast() = NEW_GC_PTR
	(
		act.getGcCtx(),
		AVM1XMLSocket(act)
	);

	return AVM1Value::undefinedVal;
}

AVM1_GETTER_TYPE_BODY(AVM1XMLSocket, AVM1XMLSocket, Timeout)
{
	return number_t(_this->socket->getTimeout().toMs());
}

AVM1_SETTER_TYPE_BODY(AVM1XMLSocket, AVM1XMLSocket, Timeout)
{
	_this->socket->setTimeout(TimeSpec::fromMs(value.toNumber(act)));
}

AVM1_FUNCTION_TYPE_DECL(AVM1XMLSocket, close)
{
	_this->socket->close(act.getSys());
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_TYPE_DECL(AVM1XMLSocket, connect)
{
	AVM1_ARG_UNPACK_NAMED(unpacker);

	auto host = unpacker.tryUnpack().orElse([&]
	{
		URLInfo url(act.getBaseClip()->getMovie()->getURL());
		if (!url.isValid())
			return AVM1Value::undefinedVal;

		if (url.getProtocol() == "file")
			return "localhost";
		else if (url.isDomain())
			return url.getHostname();
		// No domain?
		return "localhost";
	})->toString(act);

	uint16_t port;
	unpacker(port);

	return _this->socket->connect(act.getSys(), host, port);
}

AVM1_FUNCTION_TYPE_DECL(AVM1XMLSocket, send)
{
	tiny_string data;
	AVM1_ARG_UNPACK(data);

	// NOTE: `tiny_string` always contains a null byte at the end.
	_this->socket->send(act.getSys(), data);
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_TYPE_DECL(AVM1XMLSocket, onConnect)
{
	// NOTE: The default implementation of `onConnect()` is a no-op.
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_TYPE_DECL(AVM1XMLSocket, onClose)
{
	// NOTE: The default implementation of `onClose()` is a no-op.
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_TYPE_DECL(AVM1XMLSocket, onData)
{
	AVM1Value xmlVal;
	try
	{
		auto xmlCtor = act.getPrototypes().xml->ctor;
		xmlVal = xmlCtor->construct(act, args);
	}
	catch (...)
	{
		LOG
		(
			LOG_ERROR,
			"`XMLSocket.onData()`: Received an invalid `XML` object. "
			"Ignoring this message."
		);
		return AVM1Value::undefinedVal;
	}

	// NOTE: The `onXML()` call is here, rather than in the `try` block
	// because we want to propagate any exceptions it throws, up the call
	// stack.
	(void)_this->callMethod
	(
		act,
		"onXML",
		{ xmlVal },
		AVM1ExecutionReason::Special
	);
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_TYPE_DECL(AVM1XMLSocket, onXML)
{
	// NOTE: The default implementation of `onXML()` is a no-op.
	return AVM1Value::undefinedVal;
}
