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

#ifndef SCRIPTING_AVM1_TOPLEVEL_LOCALCONNECTION_H
#define SCRIPTING_AVM1_TOPLEVEL_LOCALCONNECTION_H 1

#include "gc/ptr.h"
#include "scripting/avm1/function.h"
#include "scripting/avm1/object/object.h"
#include "tiny_string.h"
#include "utils/optional.h"

// Based on Ruffle's `avm1::globals::local_connection` crate.

namespace lightspark
{

class AMFValue;
class AVM1Activation;
class AVM1Array;
class AVM1DeclContext;
template<typename T, size_t N = size_t(-1)>
class Span;

class AVM1LocalConnection : public AVM1Object
{
private:
	Optional<tiny_string> connectionName;

	bool connect(AVM1Activation& act, const tiny_string& name);
	void disconnect(AVM1Activation& act);
public:
	static void sendStatus
	(
		AVM1Context& ctx,
		const GcPtr<AVM1Object>& _this,
		const tiny_string& status
	);

	static void runMethod
	(
		AVM1Activation& act,
		const GcPtr<AVM1Object>& _this,
		const tiny_string& methodName,
		const Span<AMFValue>& args
	);

	bool isConnected() const { return status == Status::Connected; }

	AVM1LocalConnection(AVM1Activation& act);
	static GcPtr<AVM1SystemClass> createClass
	(
		AVM1DeclContext& ctx,
		const GcPtr<AVM1Object>& superProto
	);

	AVM1_FUNCTION_DECL(ctor);
	AVM1_FUNCTION_DECL(domain);
	AVM1_FUNCTION_DECL(connect);
	AVM1_FUNCTION_TYPE_DECL(AVM1LocalConnection, close);
	AVM1_FUNCTION_DECL(send);
};

}
#endif /* SCRIPTING_AVM1_TOPLEVEL_LOCALCONNECTION_H */
