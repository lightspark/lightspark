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

#include "scripting/abc.h"
#include "scripting/abc_optimized.h"
#include "scripting/abc_optimized_functionbuiltin.h"
#include "scripting/class.h"
#include "scripting/toplevel/toplevel.h"
#include "scripting/toplevel/Error.h"
using namespace std;
using namespace lightspark;

void lightspark::abc_callFunctionBuiltinOneArgVoid_constant_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = *instrptr->arg1_constant;
	asAtom value = *instrptr->arg2_constant;
	asAtom func = asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj3);
	LOG_CALL("callFunctionBuiltinOneArgVoid_cc " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(value));
	asAtom ret = asAtomHandler::invalidAtom;
	if(asAtomHandler::is<Null>(obj))
	{
		createError<TypeError>(context->worker,kConvertNullToObjectError);
		return;
	}
	if (asAtomHandler::is<Undefined>(obj))
	{
		createError<TypeError>(context->worker,kConvertUndefinedToObjectError);
		return;
	}
	asAtomHandler::getObjectNoCheck(func)->as<Function>()->call(ret,context->worker, obj, &value, 1);
	ASATOM_DECREF(ret);
	if (instrptr->cacheobj3->as<IFunction>()->clonedFrom)
		instrptr->cacheobj3->decRef();
	++(context->exec_pos);
}
void lightspark::abc_callFunctionBuiltinOneArgVoid_local_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	asAtom value = *instrptr->arg2_constant;
	asAtom func = asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj3);
	LOG_CALL("callFunctionBuiltinOneArgVoid_lc " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(value));
	asAtom ret = asAtomHandler::invalidAtom;
	if(asAtomHandler::is<Null>(obj))
	{
		createError<TypeError>(context->worker,kConvertNullToObjectError);
		return;
	}
	if (asAtomHandler::is<Undefined>(obj))
	{
		createError<TypeError>(context->worker,kConvertUndefinedToObjectError);
		return;
	}
	asAtomHandler::getObjectNoCheck(func)->as<Function>()->call(ret,context->worker, obj, &value, 1);
	ASATOM_DECREF(ret);
	if (instrptr->cacheobj3->as<IFunction>()->clonedFrom)
		instrptr->cacheobj3->decRef();
	++(context->exec_pos);
}
void lightspark::abc_callFunctionBuiltinOneArgVoid_constant_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = *instrptr->arg1_constant;
	asAtom value = CONTEXT_GETLOCAL(context,instrptr->local_pos2);
	asAtom func = asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj3);
	LOG_CALL("callFunctionOneBuiltinArgVoid_cl " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(value));
	asAtom ret = asAtomHandler::invalidAtom;
	if(asAtomHandler::is<Null>(obj))
	{
		createError<TypeError>(context->worker,kConvertNullToObjectError);
		return;
	}
	if (asAtomHandler::is<Undefined>(obj))
	{
		createError<TypeError>(context->worker,kConvertUndefinedToObjectError);
		return;
	}
	asAtomHandler::getObjectNoCheck(func)->as<Function>()->call(ret,context->worker, obj, &value, 1);
	ASATOM_DECREF(ret);
	if (instrptr->cacheobj3->as<IFunction>()->clonedFrom)
		instrptr->cacheobj3->decRef();
	++(context->exec_pos);
}
void lightspark::abc_callFunctionBuiltinOneArgVoid_local_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	asAtom value = CONTEXT_GETLOCAL(context,instrptr->local_pos2);
	asAtom func = asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj3);
	LOG_CALL("callFunctionBuiltinOneArgVoid_ll " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(value));
	asAtom ret = asAtomHandler::invalidAtom;
	if(asAtomHandler::is<Null>(obj))
	{
		createError<TypeError>(context->worker,kConvertNullToObjectError);
		return;
	}
	if (asAtomHandler::is<Undefined>(obj))
	{
		createError<TypeError>(context->worker,kConvertUndefinedToObjectError);
		return;
	}
	asAtomHandler::getObjectNoCheck(func)->as<Function>()->call(ret,context->worker, obj, &value, 1);
	ASATOM_DECREF(ret);
	if (instrptr->cacheobj3->as<IFunction>()->clonedFrom)
		instrptr->cacheobj3->decRef();
	++(context->exec_pos);
}


void lightspark::abc_callFunctionBuiltinOneArg_constant_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = *instrptr->arg1_constant;
	asAtom value = *instrptr->arg2_constant;
	bool fromglobal = (++context->exec_pos)->local2.flags & ABC_OP_FROMGLOBAL;
	asAtom func = fromglobal ? context->exec_pos->cachedvar3->var : asAtomHandler::fromObjectNoPrimitive(context->exec_pos->cacheobj3);
	LOG_CALL("callFunctionBuiltinOneArg_cc " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(value));
	asAtom ret = asAtomHandler::invalidAtom;
	asAtomHandler::getObjectNoCheck(func)->as<Function>()->call(ret,context->worker, obj, &value, 1);
	RUNTIME_STACK_PUSH(context,ret);
	if (!fromglobal && context->exec_pos->cacheobj3->as<IFunction>()->clonedFrom)
		context->exec_pos->cacheobj3->decRef();
	++(context->exec_pos);
}
void lightspark::abc_callFunctionBuiltinOneArg_local_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	asAtom value = *instrptr->arg2_constant;
	bool fromglobal = (++context->exec_pos)->local2.flags & ABC_OP_FROMGLOBAL;
	asAtom func = fromglobal ? context->exec_pos->cachedvar3->var : asAtomHandler::fromObjectNoPrimitive(context->exec_pos->cacheobj3);
	LOG_CALL("callFunctionBuiltinOneArg_lc " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(value));
	asAtom ret = asAtomHandler::invalidAtom;
	if(asAtomHandler::is<Null>(obj))
	{
		createError<TypeError>(context->worker,kConvertNullToObjectError);
		return;
	}
	if (asAtomHandler::is<Undefined>(obj))
	{
		createError<TypeError>(context->worker,kConvertUndefinedToObjectError);
		return;
	}
	asAtomHandler::getObjectNoCheck(func)->as<Function>()->call(ret,context->worker, obj, &value, 1);
	RUNTIME_STACK_PUSH(context,ret);
	if (!fromglobal && context->exec_pos->cacheobj3->as<IFunction>()->clonedFrom)
		context->exec_pos->cacheobj3->decRef();
	++(context->exec_pos);
}
void lightspark::abc_callFunctionBuiltinOneArg_constant_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = *instrptr->arg1_constant;
	asAtom value = CONTEXT_GETLOCAL(context,instrptr->local_pos2);
	bool fromglobal = (++context->exec_pos)->local2.flags & ABC_OP_FROMGLOBAL;
	asAtom func = fromglobal ? context->exec_pos->cachedvar3->var : asAtomHandler::fromObjectNoPrimitive(context->exec_pos->cacheobj3);
	LOG_CALL("callFunctionOneBuiltinArg_cl " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(value));
	asAtom ret = asAtomHandler::invalidAtom;
	asAtomHandler::getObjectNoCheck(func)->as<Function>()->call(ret,context->worker, obj, &value, 1);
	RUNTIME_STACK_PUSH(context,ret);
	if (!fromglobal && context->exec_pos->cacheobj3->as<IFunction>()->clonedFrom)
		context->exec_pos->cacheobj3->decRef();
	++(context->exec_pos);
}
void lightspark::abc_callFunctionBuiltinOneArg_local_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	asAtom value = CONTEXT_GETLOCAL(context,instrptr->local_pos2);
	bool fromglobal = (++context->exec_pos)->local2.flags & ABC_OP_FROMGLOBAL;
	asAtom func = fromglobal ? context->exec_pos->cachedvar3->var : asAtomHandler::fromObjectNoPrimitive(context->exec_pos->cacheobj3);
	LOG_CALL("callFunctionOneBuiltinArg_ll " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(value));
	asAtom ret = asAtomHandler::invalidAtom;
	asAtomHandler::getObjectNoCheck(func)->as<Function>()->call(ret,context->worker, obj, &value, 1);
	RUNTIME_STACK_PUSH(context,ret);
	if (!fromglobal && context->exec_pos->cacheobj3->as<IFunction>()->clonedFrom)
		context->exec_pos->cacheobj3->decRef();
	++(context->exec_pos);
}
void lightspark::abc_callFunctionBuiltinOneArg_constant_constant_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = *instrptr->arg1_constant;
	asAtom value = *instrptr->arg2_constant;
	bool fromglobal = (++context->exec_pos)->local2.flags & ABC_OP_FROMGLOBAL;
	asAtom func = fromglobal ? context->exec_pos->cachedvar3->var : asAtomHandler::fromObjectNoPrimitive(context->exec_pos->cacheobj3);
	LOG_CALL("callFunctionBuiltinOneArg_ccl " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(value));
	asAtom oldres = CONTEXT_GETLOCAL(context,instrptr->local3.pos);
	asAtomHandler::getObjectNoCheck(func)->as<Function>()->call(CONTEXT_GETLOCAL(context,instrptr->local3.pos),context->worker, obj, &value, 1,instrptr->local3.pos);
	ASATOM_DECREF(oldres);
	if (!fromglobal && context->exec_pos->cacheobj3->as<IFunction>()->clonedFrom)
		context->exec_pos->cacheobj3->decRef();
	++(context->exec_pos);
}
void lightspark::abc_callFunctionBuiltinOneArg_local_constant_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	asAtom value = *instrptr->arg2_constant;
	bool fromglobal = (++context->exec_pos)->local2.flags & ABC_OP_FROMGLOBAL;
	asAtom func = fromglobal ? context->exec_pos->cachedvar3->var : asAtomHandler::fromObjectNoPrimitive(context->exec_pos->cacheobj3);
	LOG_CALL("callFunctionBuiltinOneArg_lcl " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(value));
	asAtom oldres = CONTEXT_GETLOCAL(context,instrptr->local3.pos);
	asAtomHandler::getObjectNoCheck(func)->as<Function>()->call(CONTEXT_GETLOCAL(context,instrptr->local3.pos),context->worker, obj, &value, 1,instrptr->local3.pos);
	ASATOM_DECREF(oldres);
	if (!fromglobal && context->exec_pos->cacheobj3->as<IFunction>()->clonedFrom)
		context->exec_pos->cacheobj3->decRef();
	++(context->exec_pos);
}
void lightspark::abc_callFunctionBuiltinOneArg_constant_local_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = *instrptr->arg1_constant;
	asAtom value = CONTEXT_GETLOCAL(context,instrptr->local_pos2);
	bool fromglobal = (++context->exec_pos)->local2.flags & ABC_OP_FROMGLOBAL;
	asAtom func = fromglobal ? context->exec_pos->cachedvar3->var : asAtomHandler::fromObjectNoPrimitive(context->exec_pos->cacheobj3);
	LOG_CALL("callFunctionOneBuiltinArg_cll " << instrptr->local_pos2<<"/"<<instrptr->local3.pos<<" "<<asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(value));
	asAtom oldres = CONTEXT_GETLOCAL(context,instrptr->local3.pos);
	asAtomHandler::getObjectNoCheck(func)->as<Function>()->call(CONTEXT_GETLOCAL(context,instrptr->local3.pos),context->worker, obj, &value, 1,instrptr->local3.pos);
	ASATOM_DECREF(oldres);
	if (!fromglobal && context->exec_pos->cacheobj3->as<IFunction>()->clonedFrom)
		context->exec_pos->cacheobj3->decRef();
	++(context->exec_pos);
}
void lightspark::abc_callFunctionBuiltinOneArg_local_local_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	asAtom value = CONTEXT_GETLOCAL(context,instrptr->local_pos2);
	bool fromglobal = (++context->exec_pos)->local2.flags & ABC_OP_FROMGLOBAL;
	asAtom func = fromglobal ? context->exec_pos->cachedvar3->var : asAtomHandler::fromObjectNoPrimitive(context->exec_pos->cacheobj3);
	LOG_CALL("callFunctionOneBuiltinArg_lll " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(value));
	asAtom oldres = CONTEXT_GETLOCAL(context,instrptr->local3.pos);
	asAtomHandler::getObjectNoCheck(func)->as<Function>()->call(CONTEXT_GETLOCAL(context,instrptr->local3.pos),context->worker, obj, &value, 1,instrptr->local3.pos);
	ASATOM_DECREF(oldres);
	if (!fromglobal && context->exec_pos->cacheobj3->as<IFunction>()->clonedFrom)
		context->exec_pos->cacheobj3->decRef();
	++(context->exec_pos);
}

void lightspark::abc_callFunctionBuiltinMultiArgsVoid_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = *instrptr->arg1_constant;
	uint32_t argcount = instrptr->local2.pos;
	bool fromglobal = instrptr->local2.flags & ABC_OP_FROMGLOBAL;
	asAtom func = fromglobal ? instrptr->cachedvar3->var : asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj3);
	LOG_CALL("callFunctionBuiltinMultiArgsVoid_c " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" "<<argcount);
	asAtom* args = LS_STACKALLOC(asAtom,argcount);
	for (uint32_t i = argcount; i > 0 ; i--)
	{
		(++(context->exec_pos));
		if (context->exec_pos->arg2_uint == OPERANDTYPES::OP_LOCAL || context->exec_pos->arg2_uint == OPERANDTYPES::OP_CACHED_SLOT)
			args[i-1] = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
		else
			args[i-1] = *context->exec_pos->arg1_constant;
	}
	asAtom ret=asAtomHandler::invalidAtom;
	asAtomHandler::getObjectNoCheck(func)->as<Function>()->call(ret,context->worker,obj, args, argcount);
	ASATOM_DECREF(ret);
	if (!fromglobal && instrptr->cacheobj3->as<IFunction>()->clonedFrom)
		instrptr->cacheobj3->decRef();
	++(context->exec_pos);
}

void lightspark::abc_callFunctionBuiltinMultiArgsVoid_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	uint32_t argcount = instrptr->local2.pos;
	bool fromglobal = instrptr->local2.flags & ABC_OP_FROMGLOBAL;
	asAtom func = fromglobal ? instrptr->cachedvar3->var : asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj3);
	LOG_CALL("callFunctionBuiltinMultiArgsVoid_l " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" "<<argcount);
	asAtom* args = LS_STACKALLOC(asAtom,argcount);
	for (uint32_t i = argcount; i > 0 ; i--)
	{
		(++(context->exec_pos));
		if (context->exec_pos->arg2_uint == OPERANDTYPES::OP_LOCAL || context->exec_pos->arg2_uint == OPERANDTYPES::OP_CACHED_SLOT)
			args[i-1] = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
		else
			args[i-1] = *context->exec_pos->arg1_constant;
	}
	asAtom ret=asAtomHandler::invalidAtom;
	asAtomHandler::getObjectNoCheck(func)->as<Function>()->call(ret,context->worker,obj, args, argcount);
	ASATOM_DECREF(ret);
	if (!fromglobal && instrptr->cacheobj3->as<IFunction>()->clonedFrom)
		instrptr->cacheobj3->decRef();
	++(context->exec_pos);
}
void lightspark::abc_callFunctionBuiltinMultiArgs_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = *instrptr->arg1_constant;
	uint32_t argcount = instrptr->local2.pos;
	bool fromglobal = instrptr->local2.flags & ABC_OP_FROMGLOBAL;
	asAtom func = fromglobal ? instrptr->cachedvar3->var : asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj3);
	LOG_CALL("callFunctionBuiltinMultiArgs_c " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" "<<argcount);
	asAtom* args = LS_STACKALLOC(asAtom,argcount);
	for (uint32_t i = argcount; i > 0 ; i--)
	{
		(++(context->exec_pos));
		if (context->exec_pos->arg2_uint == OPERANDTYPES::OP_LOCAL || context->exec_pos->arg2_uint == OPERANDTYPES::OP_CACHED_SLOT)
			args[i-1] = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
		else
			args[i-1] = *context->exec_pos->arg1_constant;
	}
	asAtom ret = asAtomHandler::invalidAtom;
	asAtomHandler::getObjectNoCheck(func)->as<Function>()->call(ret,context->worker,obj, args, argcount);
	RUNTIME_STACK_PUSH(context,ret);
	if (!fromglobal &&instrptr->cacheobj3->as<IFunction>()->clonedFrom)
		instrptr->cacheobj3->decRef();
	++(context->exec_pos);
}
void lightspark::abc_callFunctionBuiltinMultiArgs_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	uint32_t argcount = instrptr->local2.pos;
	bool fromglobal = instrptr->local2.flags & ABC_OP_FROMGLOBAL;
	asAtom func = fromglobal ? instrptr->cachedvar3->var : asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj3);
	LOG_CALL("callFunctionBuiltinMultiArgs_l " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" "<<argcount);
	asAtom* args = LS_STACKALLOC(asAtom,argcount);
	for (uint32_t i = argcount; i > 0 ; i--)
	{
		(++(context->exec_pos));
		if (context->exec_pos->arg2_uint == OPERANDTYPES::OP_LOCAL || context->exec_pos->arg2_uint == OPERANDTYPES::OP_CACHED_SLOT)
			args[i-1] = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
		else
			args[i-1] = *context->exec_pos->arg1_constant;
	}
	asAtom ret = asAtomHandler::invalidAtom;
	asAtomHandler::getObjectNoCheck(func)->as<Function>()->call(ret,context->worker,obj, args, argcount);
	RUNTIME_STACK_PUSH(context,ret);
	if (!fromglobal && instrptr->cacheobj3->as<IFunction>()->clonedFrom)
		instrptr->cacheobj3->decRef();
	++(context->exec_pos);
}
void lightspark::abc_callFunctionBuiltinMultiArgs_constant_localResult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = *instrptr->arg1_constant;
	uint32_t argcount = instrptr->local2.pos;
	bool fromglobal = instrptr->local2.flags & ABC_OP_FROMGLOBAL;
	asAtom func = fromglobal ? instrptr->cachedvar3->var : asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj3);
	LOG_CALL("callFunctionBuiltinMultiArgs_c_lr " << asAtomHandler::toDebugString(func) << ' ' << asAtomHandler::toDebugString(obj)<<" "<<argcount);
	asAtom* args = LS_STACKALLOC(asAtom,argcount);
	for (uint32_t i = argcount; i > 0 ; i--)
	{
		(++(context->exec_pos));
		if (context->exec_pos->arg2_uint == OPERANDTYPES::OP_LOCAL || context->exec_pos->arg2_uint == OPERANDTYPES::OP_CACHED_SLOT)
			args[i-1] = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
		else
			args[i-1] = *context->exec_pos->arg1_constant;
	}
	asAtom ret = asAtomHandler::invalidAtom;
	asAtomHandler::getObjectNoCheck(func)->as<Function>()->call(ret,context->worker,obj, args, argcount,context->exec_pos->local3.pos);
	REPLACELOCALRESULT(context,context->exec_pos->local3.pos,ret);
	if (!fromglobal && instrptr->cacheobj3->as<IFunction>()->clonedFrom)
		instrptr->cacheobj3->decRef();
	++(context->exec_pos);
}
void lightspark::abc_callFunctionBuiltinMultiArgs_local_localResult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	uint32_t argcount = instrptr->local2.pos;
	bool fromglobal = instrptr->local2.flags & ABC_OP_FROMGLOBAL;
	asAtom func = fromglobal ? instrptr->cachedvar3->var : asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj3);
	LOG_CALL("callFunctionBuiltinMultiArgs_l_lr " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" "<<argcount);
	asAtom* args = LS_STACKALLOC(asAtom,argcount);
	for (uint32_t i = argcount; i > 0 ; i--)
	{
		(++(context->exec_pos));
		if (context->exec_pos->arg2_uint == OPERANDTYPES::OP_LOCAL || context->exec_pos->arg2_uint == OPERANDTYPES::OP_CACHED_SLOT)
			args[i-1] = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
		else
			args[i-1] = *context->exec_pos->arg1_constant;
	}
	asAtom ret = asAtomHandler::invalidAtom;
	asAtomHandler::getObjectNoCheck(func)->as<Function>()->call(ret,context->worker,obj, args, argcount,context->exec_pos->local3.pos);
	REPLACELOCALRESULT(context,context->exec_pos->local3.pos,ret);
	if (instrptr->cacheobj3->as<IFunction>()->clonedFrom)
		instrptr->cacheobj3->decRef();
	++(context->exec_pos);
}
void lightspark::abc_callFunctionBuiltinNoArgs_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = *instrptr->arg1_constant;
	asAtom func = asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj2);
	asAtom ret = asAtomHandler::invalidAtom;
	LOG_CALL("callFunctionBuiltinNoArgs_c " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << " on " << asAtomHandler::toDebugString(obj));
	if(asAtomHandler::is<Null>(obj))
	{
		createError<TypeError>(context->worker,kConvertNullToObjectError);
		return;
	}
	if (asAtomHandler::is<Undefined>(obj))
	{
		createError<TypeError>(context->worker,kConvertUndefinedToObjectError);
		return;
	}
	asAtomHandler::getObjectNoCheck(func)->as<Function>()->call(ret,context->worker, obj, nullptr, 0);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void lightspark::abc_callFunctionBuiltinNoArgs_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	asAtom func = asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj2);
	asAtom ret = asAtomHandler::invalidAtom;
	LOG_CALL("callFunctionBuiltinNoArgs_l " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << " on " << asAtomHandler::toDebugString(obj));
	if(asAtomHandler::is<Null>(obj))
	{
		createError<TypeError>(context->worker,kConvertNullToObjectError);
		return;
	}
	if (asAtomHandler::is<Undefined>(obj))
	{
		createError<TypeError>(context->worker,kConvertUndefinedToObjectError);
		return;
	}
	asAtomHandler::getObjectNoCheck(func)->as<Function>()->call(ret,context->worker, obj, nullptr, 0);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void lightspark::abc_callFunctionBuiltinNoArgs_constant_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = *instrptr->arg1_constant;
	asAtom func = asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj2);
	LOG_CALL("callFunctionBuiltinNoArgs_cl " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << " on " << asAtomHandler::toDebugString(obj));
	if(asAtomHandler::is<Null>(obj))
	{
		createError<TypeError>(context->worker,kConvertNullToObjectError);
		return;
	}
	if (asAtomHandler::is<Undefined>(obj))
	{
		createError<TypeError>(context->worker,kConvertUndefinedToObjectError);
		return;
	}
	asAtom oldres = CONTEXT_GETLOCAL(context,instrptr->local3.pos);
	asAtomHandler::getObjectNoCheck(func)->as<Function>()->call(CONTEXT_GETLOCAL(context,instrptr->local3.pos),context->worker, obj, nullptr, 0,instrptr->local3.pos);
	ASATOM_DECREF(oldres);
	++(context->exec_pos);
}
void lightspark::abc_callFunctionBuiltinNoArgs_local_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	asAtom func = asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj2);
	LOG_CALL("callFunctionBuiltinNoArgs_ll " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << " on " << asAtomHandler::toDebugString(obj));
	if(asAtomHandler::is<Null>(obj))
	{
		createError<TypeError>(context->worker,kConvertNullToObjectError);
		return;
	}
	if (asAtomHandler::is<Undefined>(obj))
	{
		createError<TypeError>(context->worker,kConvertUndefinedToObjectError);
		return;
	}
	asAtom oldres = CONTEXT_GETLOCAL(context,instrptr->local3.pos);
	asAtomHandler::getObjectNoCheck(func)->as<Function>()->call(CONTEXT_GETLOCAL(context,instrptr->local3.pos),context->worker, obj, nullptr, 0,instrptr->local3.pos);
	ASATOM_DECREF(oldres);
	++(context->exec_pos);
}
