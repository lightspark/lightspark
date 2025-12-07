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

#ifndef SCRIPTING_AVM1_TOPLEVEL_NETCONNECTION_H
#define SCRIPTING_AVM1_TOPLEVEL_NETCONNECTION_H 1

#include "backends/net_connection.h"
#include "gc/ptr.h"
#include "scripting/avm1/function.h"
#include "scripting/avm1/object/object.h"
#include "tiny_string.h"
#include "utils/optional.h"

// Based on Ruffle's `avm1::globals::netconnection` crate.

namespace lightspark
{

class AMFValue;
class AVM1Activation;
class AVM1Array;
class AVM1DeclContext;
template<typename T, size_t N = size_t(-1)>
class Span;
class SystemState;

class AVM1NetConnection : public AVM1Object, public NetConnection
{
	// NOTE: This is needed because `NetConnection` derives from
	// `IGcResource`.
	GC_RESOURCE_IFACE_WRAPPER(AVM1Object);
public:
	void sendStatus
	(
		SystemState* sys,
		const Optional<tiny_string>& code = {}
	) override;

	void sendCallback
	(
		SystemState* sys,
		bool isStatus,
		const AMFValue& message
	) override;

	AVM1NetConnection(AVM1Activation& act) : AVM1Object(act) {}
	static GcPtr<AVM1SystemClass> createClass
	(
		AVM1DeclContext& ctx,
		const GcPtr<AVM1Object>& superProto
	);

	AVM1_FUNCTION_DECL(ctor);
	AVM1_GETTER_TYPE_DECL(AVM1NetConnection, IsConnected);
	AVM1_GETTER_TYPE_DECL(AVM1NetConnection, Protocol);
	AVM1_GETTER_TYPE_DECL(AVM1NetConnection, URI);
	AVM1_FUNCTION_TYPE_DECL(AVM1NetConnection, addHeader);
	AVM1_FUNCTION_TYPE_DECL(AVM1NetConnection, call);
	AVM1_FUNCTION_TYPE_DECL(AVM1NetConnection, close);
	AVM1_FUNCTION_TYPE_DECL(AVM1NetConnection, connect);
};

}
#endif /* SCRIPTING_AVM1_TOPLEVEL_NETCONNECTION_H */
