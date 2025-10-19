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

#include "gc/context.h"
#include "scripting/avm1/function.h"
#include "scripting/avm1/object.h"
#include "scripting/avm1/prop_decl.h"

using namespace lightspark;

// Based on Ruffle's `avm1::property_decl` crate.

AVM1DeclContext::AVM1DeclContext(GcContext& ctx) :
gcCtx(ctx),
objProto(NEW_GC_PTR(ctx, AVM1Object(ctx))),
funcProto(NEW_GC_PTR(ctx, AVM1Object(ctx, objProto)))
{
}

void AVM1DeclContext::definePropsOn
(
	const GcPtr<AVM1Object>& _this,
	const std::initializer_list<AVM1Decl>& decls
)
{
	for (const auto& decl : decls)
		decl.definePropOn(gcCtx, _this, funcProto);
}

GcPtr<AVM1SystemClass> AVM1DeclContext::makeEmptyClass
(
	const GcPtr<AVM1Object>& superProto
) const
{
	auto proto = NEW_GC_PTR(gcCtx, AVM1Object(gcCtx, superProto));
	return NEW_GC_PTR(gcCtx, AVM1SystemClass
	{
		proto,
		// `ctor`.
		NEW_GC_PTR(gcCtx, AVM1FunctionObject(gcCtx, funcProto, proto))
	});
}

GcPtr<AVM1SystemClass> AVM1DeclContext::makeClass
(
	AVM1NativeFunc func,
	const GcPtr<AVM1Object>& superProto
) const
{
	auto proto = NEW_GC_PTR(gcCtx, AVM1Object(gcCtx, superProto));
	return NEW_GC_PTR(gcCtx, AVM1SystemClass
	{
		proto,
		// `ctor`.
		NEW_GC_PTR(gcCtx, AVM1FunctionObject
		(
			gcCtx,
			func,
			funcProto,
			proto
		))
	});
}

GcPtr<AVM1SystemClass> AVM1DeclContext::makeNativeClass
(
	AVM1NativeFunc ctor,
	AVM1NativeFunc func,
	const GcPtr<AVM1Object>& superProto
) const
{
	return makeNativeClassWithProto
	(
		ctor,
		func,
		NEW_GC_PTR(gcCtx, AVM1Object(gcCtx, superProto))
	);
}

GcPtr<AVM1SystemClass> AVM1DeclContext::makeNativeClassWithProto
(
	AVM1NativeFunc ctor,
	AVM1NativeFunc func,
	const GcPtr<AVM1Object>& proto
) const
{
	return NEW_GC_PTR(gcCtx, AVM1SystemClass
	{
		proto,
		// `ctor`.
		NEW_GC_PTR(gcCtx, AVM1FunctionObject
		(
			gcCtx,
			ctor,
			func,
			funcProto,
			proto
		))
	});
}

AVM1Value AVM1Decl::definePropOn
(
	GcContext& ctx,
	const GcPtr<AVM1Object>& _this,
	const GcPtr<AVM1Object>& funcProto
) const
{
	AVM1Value value;

	switch (type)
	{
		case Type::Prop:
		{
			bool hasSetter = prop.setter != nullptr;
			_this->addProp
			(
				name,
				NEW_GC_PTR(ctx, AVM1FunctionObject
				(
					ctx,
					prop.getter,
					funcProto,
					funcProto
				)),
				hasSetter ? NEW_GC_PTR(ctx, AVM1FunctionObject
				(
					ctx,
					prop.setter,
					funcProto,
					funcProto
				)) : NullGc,
				flags
			);
			return AVM1Value::undefinedVal;
			break;
		}
		case Type::Method:
		{
			value = NEW_GC_PTR(ctx, AVM1FunctionObject
			(
				ctx,
				func,
				nullptr,
				funcProto
			));
			break;
		}
		case Type::Function:
		{
			value = NEW_GC_PTR(ctx, AVM1FunctionObject
			(
				ctx,
				func,
				funcProto
				funcProto
			));
			break;
		}
		case Type::String: value = str; break;
		case Type::Bool: value = _bool; break;
		case Type::Int: value = number_t(_int); break;
		case Type::Float: value = _float; break;
		default: assert(false); break;
	}

	_this->defineValue(name, value, flags);
	return value;
}
