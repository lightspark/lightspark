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

#ifndef SCRIPTING_AVM1_TOPLEVEL_STRING_H
#define SCRIPTING_AVM1_TOPLEVEL_STRING_H 1

#include "gc/ptr.h"
#include "scripting/avm1/function.h"
#include "scripting/avm1/object/object.h"
#include "tiny_string.h"

// Based on Ruffle's `avm1::globals::string` crate.

namespace lightspark
{

class AVM1Activation;
class AVM1DeclContext;
class AVM1SystemClass;

class AVM1String : public AVM1Object
{
private:
	tiny_string str;
public:
	AVM1String
	(
		const GcContext& ctx,
		GcPtr<AVM1Object> proto,
		const tiny_string& _str = ""
	);

	static GcPtr<AVM1SystemClass> createClass
	(
		AVM1DeclContext& ctx,
		const GcPtr<AVM1Object>& superProto
	);

	// Implements the `String` constructor.
	AVM1_FUNCTION_DECL(ctor);

	// Implements `String()`.
	AVM1_FUNCTION_DECL(string);

	AVM1_FUNCTION_TYPE_DECL(AVM1String, valueOf);
	AVM1_FUNCTION_TYPE_DECL(AVM1String, toString);
	AVM1_FUNCTION_DECL(toUpperCase);
	AVM1_FUNCTION_DECL(toLowerCase);
	AVM1_FUNCTION_DECL(charAt);
	AVM1_FUNCTION_DECL(charCodeAt);
	AVM1_FUNCTION_DECL(concat);
	AVM1_FUNCTION_DECL(indexOf);
	AVM1_FUNCTION_DECL(lastIndexOf);
	AVM1_FUNCTION_DECL(slice);
	AVM1_FUNCTION_DECL(substring);
	AVM1_FUNCTION_DECL(split);
	AVM1_FUNCTION_DECL(substr);

	AVM1_FUNCTION_DECL(fromCharCode);
};

}
#endif /* SCRIPTING_AVM1_TOPLEVEL_STRING_H */
