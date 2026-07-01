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

#ifndef SCRIPTING_AVM1_TOPLEVEL_NETSTREAM_H
#define SCRIPTING_AVM1_TOPLEVEL_NETSTREAM_H 1

#include <utility>

#include "gc/ptr.h"
#include "interfaces/backends/net_stream.h"
#include "scripting/avm1/function.h"
#include "scripting/avm1/object/object.h"
#include "tiny_string.h"
#include "utils/optional.h"

// Based on Ruffle's `avm1::globals::netstream` crate.

namespace lightspark
{

class AMFValue;
class AVM1Activation;
class AVM1Array;
class AVM1DeclContext;
class NetStream;
template<typename T, size_t N = size_t(-1)>
class Span;
class SystemState;

class AVM1NetStream : public AVM1Object, public INetStreamObject
{
private:
	GcPtr<NetStream> stream;

	using INetStreamObject::StatusEventValues;
	void triggerStatusEvent
	(
		SystemState* sys,
		const StatusEventValues& values
	) override;

	void handleScriptData
	(
		SystemState* sys,
		const tiny_string& varName,
		const AMFValue& varValue
	) override;
public:
	AVM1NetStream(AVM1Activation& act);
	static GcPtr<AVM1SystemClass> createClass
	(
		AVM1DeclContext& ctx,
		const GcPtr<AVM1Object>& superProto
	);

	AVM1_FUNCTION_DECL(ctor);
	AVM1_GETTER_TYPE_DECL(AVM1NetStream, BufferLength);
	AVM1_PROPERTY_TYPE_DECL(AVM1NetStream, BufferTime);
	AVM1_GETTER_TYPE_DECL(AVM1NetStream, BytesLoaded);
	AVM1_GETTER_TYPE_DECL(AVM1NetStream, BytesTotal);
	AVM1_GETTER_TYPE_DECL(AVM1NetStream, Time);
	AVM1_FUNCTION_TYPE_DECL(AVM1NetStream, play);
	AVM1_FUNCTION_TYPE_DECL(AVM1NetStream, pause);
	AVM1_FUNCTION_TYPE_DECL(AVM1NetStream, seek);
};

}
#endif /* SCRIPTING_AVM1_TOPLEVEL_NETSTREAM_H */
