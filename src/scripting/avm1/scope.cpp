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

#include "display_object/DisplayObject.h"
#include "gc/context.h"
#include "scripting/avm1/activation.h"
#include "scripting/avm1/callable_value.h"
#include "scripting/avm1/object/display_object.h"
#include "scripting/avm1/object/object.h"
#include "scripting/avm1/prop.h"
#include "scripting/avm1/scope.h"
#include "scripting/avm1/value.h"
#include "scripting/avm1/toplevel/toplevel.h"
#include "swf.h"
#include "tiny_string.h"
#include "utils/impl.h"
#include "utils/optional.h"

using namespace lightspark;

// Based on Ruffle's `avm1::scope::Scope`.

AVM1Scope::AVM1Scope
(
	const GcContext& ctx,
	const GcPtr<AVM1Scope>& parent,
	const AVM1ScopeClass& type,
	const GcPtr<AVM1Object>& _values
) : GcResource(ctx), parent(_parent), _class(type), values(_values)
{
}

AVM1Scope
(
	const GcPtr<AVM1Scope>& parent,
	const AVM1ScopeClass& type,
	const GcPtr<AVM1Object>& values
) : AVM1Scope(values->getGcCtx(), parent, type, values)
{
}

AVM1Scope::AVM1Scope(const GcPtr<AVM1Global>& globals) : AVM1Scope
(
	// `parent`
	nullptr,
	// `_class`
	AVM1ScopeClass::Global,
	// `values`
	globals
)
{
}

AVM1Scope::AVM1Scope
(
	const GcPtr<AVM1Scope> parent,
	const GcPtr<AVM1DisplayObject>& clip
) : AVM1Scope(parent, AVM1ScopeClass::Target, clip)
{
}

AVM1Scope::AVM1Scope
(
	const GcContext& ctx,
	const GcPtr<AVM1Scope>& parent
) : AVM1Scope(ctx, parent, AVM1ScopeClass::Local, NEW_GC_PTR(ctx, AVM1Object(ctx)))
{
}

void AVM1Scope::setTargetScope(const GcPtr<AVM1DisplayObject>& clip)
{
	if (isTargetScope())
		values = clip;
	else if (!parent.isNull())
		parent->setTargetScope(clip);
}

Impl<AVM1CallableValue> AVM1Scope::resolveRecursive
(
	AVM1Activation& act,
	const tiny_string& name,
	bool isTopLevel
) const
{
	if (values->hasProp(act, name))
	{
		return AVM1Callable
		(
			values,
			values->getNonSlashPathProp(act, name)
		);
	}

	if (parent.isNull())
		return AVM1UnCallable(AVM1Value::undefinedVal);

	auto ret = parent->getVar(act, name);

	if (ret->isCallable() || !ret->getValue().is<UndefinedVal>())
		return ret;

	auto clip = asAtomHandler::as<DisplayObject>(values);
	if (!isTopLevel || ret->isCallable())
		return ret;
	if (!ret->getValue().is<UndefinedVal>() || values->hasOwnProp(act, name))
		return ret;

	// If we failed to find the value in the scope chain, but would've
	// resolved on `values`, if it were on stage, then try looking for
	// it in the root clip instead.
	auto rootObj = act.getRootObject().toObject(act);
	return AVM1Callable(rootObj, rootObj->getNonSlashPathProp(act, name));
}

void AVM1Scope::setVar
(
	AVM1Activation& act,
	const tiny_string& name,
	const AVM1Value& value
)
{
	bool removed = values->tryAs
	<
		DisplayObject
	>().transformOr(false, [&](const auto& obj)
	{
		return obj.isAVM1Removed();
	});

	if (!removed && (isTargetScope() || values->hasProp(act, name)))
	{
		// Found the variable on this object, overwrite it.
		// Or, we've hit the currently running clip, so create it here.
		return values->setProp(act, name, value);
	}
	else if (!parent.isNull())
	{
		// Couldn't find the variable, traverse the scope chain.
		return parent->setVar(act, name, value);
	}

	// This probably shouldn't happen, since all AVM1 code runs in
	// reference to a `MovieClip`, so we should always have a `MovieClip`
	// scope. Define it on the top level scope.
	LOG(LOG_ERROR, "AVM1Scope::setVar: No top level `MovieClip` scope.");
	return values->setProp(act, name, value);
}

void AVM1Scope::defineLocal
(
	AVM1Activation& act,
	const tiny_string& name,
	const AVM1Value& value
)
{
	if (!isWithScope() || parent.isNull())
		return values->setProp(act, name, value);
	// When defining a local in a `with` scope, we also need to check if
	// that local exists on the `with` target. If it does, the variable
	// of the target should be modified. If not, the property should be
	// defined in the first non-`with` parent scope.

	// Does this variable already exist on the target?
	if (values->hasOwnProp(act, name))
		return values->setProp(act, name, value);

	// If not, go up the scope chain.
	return parent->defineLocal(act, name, value);
}

void AVM1Scope::forceDefineLocal(const tiny_string& name, const AVM1Value& value)
{
	values->defineValue(name, value);
}

#ifdef USE_STRING_ID
void AVM1Scope::forceDefineLocal(uint32_t nameID, const AVM1Value& value)
{
	values->defineValue(nameID, value);
}
#endif

bool AVM1Scope::deleteVar(AVM1Activation& act, const tiny_string& name)
{
	if (values->hasProp(act, name))
		return values->deleteProp(act, name);
	else if (!parent.isNull())
		return parent->deleteVar(act, name);
	return false;
}
