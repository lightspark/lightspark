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

#ifndef SCRIPTING_AVM1_PROP_DECL_H
#define SCRIPTING_AVM1_PROP_DECL_H 1

#include <initializer_list>

#include "gc/ptr.h"
#include "scripting/avm1/function.h"
#include "scripting/avm1/prop.h"
#include "scripting/avm1/value.h"

// Based on Ruffle's `avm1::property_decl` crate.

namespace lightspark
{

class GcContext;
class AVM1Decl;
class AVM1Object;
struct AVM1SystemClass;

class AVM1DeclContext
{
	using SysClass = AVM1SystemClass;
private:
	GcContext& gcCtx;
	GcPtr<AVM1Object> objProto;
	GcPtr<AVM1Object> funcProto;
public:
	AVM1DeclContext(GcContext& ctx);

	GcContext& getGcCtx() { return gcCtx; }
	const GcContext& getGcCtx() const { return gcCtx; }
	const GcPtr<AVM1Object>& getObjectProto() const { return objProto; }
	const GcPtr<AVM1Object>& getFuncProto() const { return funcProto; }

	void definePropsOn
	(
		const GcPtr<AVM1Object>& _this,
		const std::initializer_list<AVM1Decl>& decls
	);

	GcPtr<SysClass> makeEmptyClass
	(
		const GcPtr<AVM1Object>& superProto
	) const;

	// Creates a class with a "normal" constructor. This should be used
	// for classes that have their constructor implemented in bytecode in
	// Flash Player's `playerglobals.swf`.
	GcPtr<SysClass> makeClass
	(
		AVM1NativeFunc func,
		const GcPtr<AVM1Object>& superProto
	) const;

	// Creates a class with a "special" constructor. This should be used
	// for classes with a native constructor in Flash Player's
	// `playerglobals.swf`.
	GcPtr<SysClass> makeNativeClass
	(
		AVM1NativeFunc ctor,
		AVM1NativeFunc func,
		const GcPtr<AVM1Object>& superProto
	) const;

	GcPtr<SysClass> makeNativeClassWithProto
	(
		AVM1NativeFunc ctor,
		AVM1NativeFunc func,
		const GcPtr<AVM1Object>& proto
	) const;
};

struct AVM1SystemClass
{
	GcPtr<AVM1Object> proto;
	GcPtr<AVM1Object> ctor;
};

// The declaration of a property, method, or simple field, that can be
// defined on an `Object`.
class AVM1Decl
{
public:
	// All possible types of an `AVM1Decl`.
	enum class Type
	{
		// Declares a property with a getter, and an optional setter.
		Prop,
		// Declares a native function.
		//
		// This is intended for use with defining native object prototypes.
		// Notably, this creates a function object without an explicit
		// `prototype`, which is only possible when defining native
		// functions.
		Method,
		// Declares a native function, with a `prototype`.
		// It's recommended to use `Method` when defining native functions.
		Function,
		// Declares a static string value.
		String,
		// Declares a static `bool` value.
		Bool,
		// Declares a static `int` value.
		Int,
		// Declares a static `float` value.
		Float,
	};

	struct Prop
	{
		AVM1NativeFunc getter;
		AVM1NativeFunc setter;
	};
private:
	Type type;
	const char* name;
	union
	{
		Prop prop;
		AVM1NativeFunc func;
		const char* str;
		bool _bool;
		int32_t _int;
		number_t _float;
	};

	AVM1PropFlags flags;
public:
	AVM1Decl
	(
		const char* _name,
		AVM1NativeFunc getter,
		AVM1NativeFunc setter,
		const AVM1PropFlags& _flags = AVM1PropFlags(0)
	) :
	type(Type::Prop),
	name(_name),
	prop({ getter, setter }),
	flags(_flags) {}

	AVM1Decl
	(
		const char* _name,
		AVM1NativeFunc _func,
		const AVM1PropFlags& _flags = AVM1PropFlags(0),
		bool isMethod = true
	) :
	type(isMethod ? Type::Method : Type::Function),
	name(_name),
	func(_func),
	flags(_flags) {}

	AVM1Decl
	(
		const char* _name,
		const char* _str,
		const AVM1PropFlags& _flags = AVM1PropFlags(0)
	) : type(Type::String), name(_name), str(_str), flags(_flags) {}

	AVM1Decl
	(
		const char* _name,
		bool boolVal,
		const AVM1PropFlags& _flags = AVM1PropFlags(0)
	) : type(Type::Bool), name(_name), _bool(boolVal), flags(_flags) {}

	AVM1Decl
	(
		const char* _name,
		int32_t intVal,
		const AVM1PropFlags& _flags = AVM1PropFlags(0)
	) : type(Type::Int), name(_name), _int(intVal), flags(_flags) {}

	AVM1Decl
	(
		const char* _name,
		number_t floatVal,
		const AVM1PropFlags& _flags = AVM1PropFlags(0)
	) : type(Type::Float), name(_name), _float(floatVal), flags(_flags) {}

	// Defines the field represented by this declaration on an `Object`.
	// Returns the value defined on the `Object`, or `undefined`, if this
	// declaration defined a property.
	AVM1Value definePropOn
	(
		GcContext& ctx,
		const GcPtr<AVM1Object>& _this,
		const GcPtr<AVM1Object>& funcProto
	) const;
};

}
#endif /* SCRIPTING_AVM1_PROP_DECL_H */
