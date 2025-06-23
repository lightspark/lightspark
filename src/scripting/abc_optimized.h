/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2025  Ludger Krämer <dbluelle@onlinehome.de>

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

#ifndef SCRIPTING_ABC_OPTIMIZED_H
#define SCRIPTING_ABC_OPTIMIZED_H 1

#include "scripting/abcutils.h"
#include "scripting/abctypes.h"

#define REPLACELOCALRESULT(context,pos,ret) { \
	if (USUALLY_FALSE(context->exceptionthrown)) \
	{ \
		ASATOM_DECREF(ret); \
		return; \
	} \
	asAtom oldres = CONTEXT_GETLOCAL(context,pos);\
	if (asAtomHandler::isLocalNumber(ret)) \
		asAtomHandler::setNumber(CONTEXT_GETLOCAL(context,pos),context->worker,asAtomHandler::getLocalNumber(context,ret),pos);\
	else \
		asAtomHandler::set(CONTEXT_GETLOCAL(context,pos),ret);\
	ASATOM_DECREF(oldres);\
}

namespace lightspark
{
void abc_callFunctionBuiltinOneArgVoid_constant_constant(call_context* context);
void abc_callFunctionBuiltinOneArgVoid_local_constant(call_context* context);
void abc_callFunctionBuiltinOneArgVoid_constant_local(call_context* context);
void abc_callFunctionBuiltinOneArgVoid_local_local(call_context* context);
void abc_callFunctionBuiltinOneArg_constant_constant(call_context* context);
void abc_callFunctionBuiltinOneArg_local_constant(call_context* context);
void abc_callFunctionBuiltinOneArg_constant_local(call_context* context);
void abc_callFunctionBuiltinOneArg_local_local(call_context* context);
void abc_callFunctionBuiltinOneArg_constant_constant_localresult(call_context* context);
void abc_callFunctionBuiltinOneArg_local_constant_localresult(call_context* context);
void abc_callFunctionBuiltinOneArg_constant_local_localresult(call_context* context);
void abc_callFunctionBuiltinOneArg_local_local_localresult(call_context* context);
void abc_callFunctionBuiltinMultiArgsVoid_constant(call_context* context);
void abc_callFunctionBuiltinMultiArgsVoid_local(call_context* context);
void abc_callFunctionBuiltinMultiArgs_constant(call_context* context);
void abc_callFunctionBuiltinMultiArgs_local(call_context* context);
void abc_callFunctionBuiltinMultiArgs_constant_localResult(call_context* context);
void abc_callFunctionBuiltinMultiArgs_local_localResult(call_context* context);
void abc_callFunctionBuiltinNoArgs_constant(call_context* context);
void abc_callFunctionBuiltinNoArgs_local(call_context* context);
void abc_callFunctionBuiltinNoArgs_constant_localresult(call_context* context);
void abc_callFunctionBuiltinNoArgs_local_localresult(call_context* context);

void abc_negate_constant(call_context* context);
void abc_negate_local(call_context* context);
void abc_negate_constant_localresult(call_context* context);
void abc_negate_local_localresult(call_context* context);
void abc_negate_constant_setslotnocoerce(call_context* context);
void abc_negate_local_setslotnocoerce(call_context* context);

void abc_add_constant_constant(call_context* context);
void abc_add_local_constant(call_context* context);
void abc_add_constant_local(call_context* context);
void abc_add_local_local(call_context* context);
void abc_add_constant_constant_localresult(call_context* context);
void abc_add_local_constant_localresult(call_context* context);
void abc_add_constant_local_localresult(call_context* context);
void abc_add_local_local_localresult(call_context* context);
void abc_add_constant_constant_setslotnocoerce(call_context* context);
void abc_add_local_constant_setslotnocoerce(call_context* context);
void abc_add_constant_local_setslotnocoerce(call_context* context);
void abc_add_local_local_setslotnocoerce(call_context* context);

void abc_callFunctionSyntheticOneArgVoid_constant_constant(call_context* context);
void abc_callFunctionSyntheticOneArgVoid_local_constant(call_context* context);
void abc_callFunctionSyntheticOneArgVoid_constant_local(call_context* context);
void abc_callFunctionSyntheticOneArgVoid_local_local(call_context* context);
void abc_callFunctionSyntheticOneArg_constant_constant(call_context* context);
void abc_callFunctionSyntheticOneArg_local_constant(call_context* context);
void abc_callFunctionSyntheticOneArg_constant_local(call_context* context);
void abc_callFunctionSyntheticOneArg_local_local(call_context* context);
void abc_callFunctionSyntheticOneArg_constant_constant_localresult(call_context* context);
void abc_callFunctionSyntheticOneArg_local_constant_localresult(call_context* context);
void abc_callFunctionSyntheticOneArg_constant_local_localresult(call_context* context);
void abc_callFunctionSyntheticOneArg_local_local_localresult(call_context* context);
void abc_callFunctionSyntheticMultiArgsVoid_constant(call_context* context);
void abc_callFunctionSyntheticMultiArgsVoid_local(call_context* context);
void abc_callFunctionSyntheticMultiArgs_constant(call_context* context);
void abc_callFunctionSyntheticMultiArgs_local(call_context* context);
void abc_callFunctionSyntheticMultiArgs_constant_localResult(call_context* context);
void abc_callFunctionSyntheticMultiArgs_local_localResult(call_context* context);
void abc_callFunctionSyntheticMultiArgs(call_context* context);
void abc_callFunctionSyntheticMultiArgsVoid(call_context* context);

void abc_getslot_constant(call_context* context);
void abc_getslot_local(call_context* context);
void abc_getslot_constant_localresult(call_context* context);
void abc_getslot_local_localresult(call_context* context);
void abc_getslot_constant_setslotnocoerce(call_context* context);
void abc_getslot_local_setslotnocoerce(call_context* context);
void abc_getSlotFromScopeObject(call_context* context);
void abc_getSlotFromScopeObject_localresult(call_context* context);
}

#endif /* SCRIPTING_ABC_OPTIMIZED_H */
