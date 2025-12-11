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

#ifndef SCRIPTING_AVM1_TOPLEVEL_XMLSOCKET_H
#define SCRIPTING_AVM1_TOPLEVEL_XMLSOCKET_H 1

#include "gc/ptr.h"
#include "interfaces/backends/socket.h"
#include "scripting/avm1/function.h"
#include "scripting/avm1/object/object.h"
#include "utils/span.h"

// Based on Ruffle's `avm1::globals::xml_socket` crate.

namespace lightspark
{

class AVM1Activation;
class AVM1DeclContext;
class AVM1SystemClass;
enum ConnectionState;
class Socket;
class SystemState;

class AVM1XMLSocket : public AVM1Object, public ISocketObject
{
private:
	GcPtr<Socket> socket;

	void handleConnection
	(
		SystemState* sys,
		const ConnectionState& state
	) override;

	void handleData
	(
		SystemState* sys,
		const Span<uint8_t>& data
	) override;

	void handleClose(SystemState* sys) override;
public:
	AVM1XMLSocket(AVM1Activation& act);

	static GcPtr<AVM1SystemClass> createClass
	(
		AVM1DeclContext& ctx,
		const GcPtr<AVM1Object>& superProto
	);

	AVM1_FUNCTION_DECL(ctor);
	AVM1_PROPERTY_TYPE_DECL(AVM1XMLSocket, Timeout);
	AVM1_FUNCTION_TYPE_DECL(AVM1XMLSocket, close);
	AVM1_FUNCTION_TYPE_DECL(AVM1XMLSocket, connect);
	AVM1_FUNCTION_TYPE_DECL(AVM1XMLSocket, send);
	AVM1_FUNCTION_TYPE_DECL(AVM1XMLSocket, onConnect);
	AVM1_FUNCTION_TYPE_DECL(AVM1XMLSocket, onClose);
	AVM1_FUNCTION_TYPE_DECL(AVM1XMLSocket, onData);
	AVM1_FUNCTION_TYPE_DECL(AVM1XMLSocket, onXML);
};

}
#endif /* SCRIPTING_AVM1_TOPLEVEL_XMLSOCKET_H */
