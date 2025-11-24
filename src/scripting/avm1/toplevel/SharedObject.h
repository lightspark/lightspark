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

#ifndef SCRIPTING_AVM1_TOPLEVEL_SHAREDOBJECT_H
#define SCRIPTING_AVM1_TOPLEVEL_SHAREDOBJECT_H 1

#include "gc/ptr.h"
#include "scripting/avm1/function.h"
#include "scripting/avm1/object/object.h"
#include "tiny_string.h"

// Based on Ruffle's `avm1::globals::shared_object` crate.

namespace lightspark
{

class Amf0Serializer;
class AVM1Activation;
class AVM1Array;
class AVM1DeclContext;
class LocalSharedObject;

class AVM1SharedObject : public AVM1Object
{
private:
	tiny_string name;

	// Serializes a `Value` into an AMF0 value.
	static void serializeValue
	(
		AVM1Activation& act,
		const tiny_string& name,
		const AVM1Value& value,
		Amf0Serializer& serializer
	);

	// Serializes an `Object`, and it's children into an AMF0 object.
	static void serialize
	(
		AVM1Activation& act,
		const GcPtr<AVM1Object>& obj,
		Amf0Serializer& serializer
	);

	static LocalSharedObject makeLSO
	(
		AVM1Activation& act,
		const tiny_string& name,
		const GcPtr<AVM1Object>& obj
	);

	bool flushData(AVM1Activation& act) const;
public:
	AVM1SharedObject(AVM1Activation& act);
	static GcPtr<AVM1SystemClass> createClass
	(
		AVM1DeclContext& ctx,
		const GcPtr<AVM1Object>& superProto
	);

	AVM1_FUNCTION_DECL(ctor);
	AVM1_FUNCTION_DECL(clear);
	AVM1_FUNCTION_TYPE_DECL(AVM1SharedObject, close);
	AVM1_FUNCTION_TYPE_DECL(AVM1SharedObject, connect);
	AVM1_FUNCTION_TYPE_DECL(AVM1SharedObject, flush);
	AVM1_FUNCTION_TYPE_DECL(AVM1SharedObject, getSize);
	AVM1_FUNCTION_TYPE_DECL(AVM1SharedObject, send);
	AVM1_FUNCTION_TYPE_DECL(AVM1SharedObject, setFps);
	AVM1_FUNCTION_TYPE_DECL(AVM1SharedObject, onStatus);
	AVM1_FUNCTION_TYPE_DECL(AVM1SharedObject, onSync);

	AVM1_FUNCTION_TYPE_DECL(AVM1SharedObject, deleteAll);
	AVM1_FUNCTION_TYPE_DECL(AVM1SharedObject, getDiskUsage);
	AVM1_FUNCTION_DECL(getLocal);
	AVM1_FUNCTION_DECL(getRemote);
};

}
#endif /* SCRIPTING_AVM1_TOPLEVEL_SHAREDOBJECT_H */
