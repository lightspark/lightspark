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

#include "asobject.h"
#include "scripting/avm1/scope.h"
#include "scripting/class.h"
#include "scripting/flash/display/RootMovieClip.h"
#include "scripting/flash/system/flashsystem.h"
#include "scripting/toplevel/Global.h"
#include "swf.h"
#include "swftypes.h"
#include "utils/optional.h"

using namespace lightspark;

// Based on Ruffle's `avm1::scope::Scope`.

AVM1Scope::AVM1Scope(Global* globals) : AVM1Scope
(
	// `parent`
	NullRef,
	// `_class`
	AVM1ScopeClass::Global,
	// `values`
	asAtomHandler::fromObjectNoPrimitive(globals)
)
{
}

AVM1Scope::AVM1Scope
(
	const _R<AVM1Scope>& parent,
	DisplayObject* clip
) : AVM1Scope(parent, AVM1ScopeClass::Target, asAtomHandler::fromObjectNoPrimitive(clip))
{
}

AVM1Scope::AVM1Scope
(
	const _R<AVM1Scope>& parent,
	ASWorker* wrk
) : AVM1Scope(parent, AVM1ScopeClass::Local, asAtomHandler::fromObjectNoPrimitive(new_asobject(wrk)),true)
{
}

void AVM1Scope::setLocals(asAtom newLocals)
{
	values = newLocals;
}

void AVM1Scope::setTargetScope(DisplayObject* clip)
{
	if (isTargetScope())
		setLocals(asAtomHandler::fromObjectNoPrimitive(clip));
	else if (!parent.isNull())
		parent->setTargetScope(clip);
}

asAtom AVM1Scope::resolveRecursiveByMultiname
(
	DisplayObject* baseClip,
	const multiname& name,
	const GET_VARIABLE_OPTION& options,
	ASWorker* wrk,
	bool isTopLevel
)
{
	asAtom ret = asAtomHandler::invalidAtom;
	if (asAtomHandler::hasPropertyByMultiname(values,name, true, true, wrk))
	{
		asAtomHandler::AVM1getVariableByMultiname(values,ret, name, options, wrk, false);
		return ret;
	}

	if (parent.isNull())
		return ret;

	ret = parent->getVariableByMultiname(baseClip, name, options, wrk);

	if (asAtomHandler::isValid(ret) || !asAtomHandler::is<DisplayObject>(values))
		return ret;

	auto clip = asAtomHandler::as<DisplayObject>(values);
	if (!isTopLevel || (clip->isOnStage() && !clip->hasPropertyByMultiname(name, true, true, wrk)))
		return ret;

	// If we failed to find the value in the scope chain, but would've
	// resolved on `values`, if it were on stage, then try looking for
	// it in the root clip instead.
	auto root = baseClip->AVM1getRoot();
	ret = asAtomHandler::invalidAtom;
	root->AVM1getVariableByMultiname(ret, name, options, wrk, false);
	return ret;
}

bool AVM1Scope::setVariableByMultiname
(
	multiname& name,
	asAtom& value,
	const CONST_ALLOWED_FLAG& allowConst,
	ASWorker* wrk
)
{
	bool removed =
	(
		asAtomHandler::is<DisplayObject>(values) &&
		!asAtomHandler::as<DisplayObject>(values)->isOnStage()
	);

	if (!removed && asAtomHandler::isValid(values) && (isTargetScope() || asAtomHandler::hasPropertyByMultiname(values,name, true, true, wrk)))
	{
		// Found the variable on this object, overwrite it.
		// Or, we've hit the currently running clip, so create it here.
		return asAtomHandler::AVM1setVariableByMultiname(values,name, value, allowConst, wrk);
	}
	else if (!parent.isNull())
	{
		// Couldn't find the variable, traverse the scope chain.
		return parent->setVariableByMultiname(name, value, allowConst, wrk);
	}
	return asAtomHandler::AVM1setVariableByMultiname(values,name, value, allowConst, wrk);
}

bool AVM1Scope::defineLocalByMultiname
(
	multiname& name,
	asAtom& value,
	const CONST_ALLOWED_FLAG& allowConst,
	ASWorker* wrk
)
{
	ASObject* v = asAtomHandler::getObject(values);
	if (!isWithScope() || !parent.isNull())
		return v && v->AVM1setVariableByMultiname(name, value, allowConst, wrk);
	// When defining a local in a `with` scope, we also need to check if
	// that local exists on the `with` target. If it does, the variable
	// of the target should be modified. If not, the property should be
	// defined in the first non-`with` parent scope.

	// Does this variable already exist on the target?
	if (v && v->hasPropertyByMultiname(name, true, true, wrk))
		return v->AVM1setVariableByMultiname(name, value, allowConst, wrk);

	// If not, go up the scope chain.
	return parent->defineLocalByMultiname(name, value, allowConst, wrk);
}

bool AVM1Scope::forceDefineLocalByMultiname
(
	multiname& name,
	asAtom& value,
	const CONST_ALLOWED_FLAG& allowConst,
	ASWorker* wrk
)
{
	ASObject* v = asAtomHandler::getObject(values);
	bool alreadySet = false;
	if (v)
		v->setVariableByMultiname(name, value, allowConst, &alreadySet, wrk);
	return alreadySet;
}

bool AVM1Scope::forceDefineLocal
(
	const tiny_string& name,
	asAtom& value,
	const CONST_ALLOWED_FLAG& allowConst,
	ASWorker* wrk
)
{
	return forceDefineLocal
	(
		wrk->getSystemState()->getUniqueStringId(name, true),
		value,
		allowConst,
		wrk
	);
}

bool AVM1Scope::forceDefineLocal
(
	uint32_t nameID,
	asAtom& value,
	const CONST_ALLOWED_FLAG& allowConst,
	ASWorker* wrk
)
{
	multiname m(nullptr);
	m.name_type = multiname::NAME_STRING;
	m.name_s_id = nameID;
	return forceDefineLocalByMultiname(m, value, allowConst, wrk);
}

void AVM1Scope::clearScope()
{
	assert(storedmembercount);
	--this->storedmembercount;
	// resetting the local value is needed to avoid dangling references
	ASObject* v = getLocalsPtr();
	if (v)
		v->removeStoredMember();
}

bool AVM1Scope::deleteVariableByMultiname(const multiname& name, ASWorker* wrk)
{
	ASObject* v = asAtomHandler::getObject(values);
	if (v && v->hasPropertyByMultiname(name, true, true, wrk))
		return v->deleteVariableByMultiname(name, wrk);
	else if (!parent.isNull())
		return parent->deleteVariableByMultiname(name, wrk);
	return false;
}

bool AVM1Scope::countAllCyclicMemberReferences(garbagecollectorstate& gcstate)
{
	if (gcstate.stopped || this->getRefCount() != (int)this->storedmembercount)
		return false;
	bool ret = false;
	ASObject* v = asAtomHandler::getObject(values);
	if (v)
		ret = v->countAllCylicMemberReferences(gcstate);
	return ret;
}

void AVM1Scope::prepareShutdown()
{
	ASObject* v = asAtomHandler::getObject(values);
	if (v)
		v->prepareShutdown();
	if (!parent.isNull())
		parent->prepareShutdown();
}
