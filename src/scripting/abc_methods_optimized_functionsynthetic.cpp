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
#include "scripting/toplevel/toplevel.h"

using namespace std;
using namespace lightspark;

void lightspark::abc_callFunctionSyntheticOneArgVoid_constant_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = *instrptr->arg1_constant;
	asAtom value = *instrptr->arg2_constant;
	bool fromglobal = (++context->exec_pos)->local2.flags & ABC_OP_FROMGLOBAL;
	asAtom func = fromglobal ? context->exec_pos->cachedvar3->var : asAtomHandler::fromObjectNoPrimitive(context->exec_pos->cacheobj3);
	LOG_CALL("callFunctionSyntheticOneArgVoid_cc " << asAtomHandler::toDebugString(func) << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(value));
	asAtom ret = asAtomHandler::invalidAtom;
	asAtomHandler::getObjectNoCheck(func)->as<SyntheticFunction>()->call(context->worker,ret,obj, &value, 1,false,(instrptr->local3.flags&ABC_OP_COERCED)==0);
	ASATOM_DECREF(ret);
	if (!fromglobal && context->exec_pos->cacheobj3->as<IFunction>()->clonedFrom)
		context->exec_pos->cacheobj3->decRef();
	++(context->exec_pos);
}
void lightspark::abc_callFunctionSyntheticOneArgVoid_local_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	asAtom value = *instrptr->arg2_constant;
	bool fromglobal = (++context->exec_pos)->local2.flags & ABC_OP_FROMGLOBAL;
	asAtom func = fromglobal ? context->exec_pos->cachedvar3->var : asAtomHandler::fromObjectNoPrimitive(context->exec_pos->cacheobj3);
	LOG_CALL("callFunctionSyntheticOneArgVoid_lc " << asAtomHandler::toDebugString(func) << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(value));
	ASATOM_INCREF(obj); // ensure that obj stays valid during call
	asAtom ret = asAtomHandler::invalidAtom;
	asAtomHandler::getObjectNoCheck(func)->as<SyntheticFunction>()->call(context->worker,ret,obj, &value, 1,false,(instrptr->local3.flags&ABC_OP_COERCED)==0);
	ASATOM_DECREF(obj);
	ASATOM_DECREF(ret);
	if (!fromglobal && context->exec_pos->cacheobj3->as<IFunction>()->clonedFrom)
		context->exec_pos->cacheobj3->decRef();
	++(context->exec_pos);
}
void lightspark::abc_callFunctionSyntheticOneArgVoid_constant_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = *instrptr->arg1_constant;
	asAtom value = CONTEXT_GETLOCAL(context,instrptr->local_pos2);
	bool fromglobal = (++context->exec_pos)->local2.flags & ABC_OP_FROMGLOBAL;
	asAtom func = fromglobal ? context->exec_pos->cachedvar3->var : asAtomHandler::fromObjectNoPrimitive(context->exec_pos->cacheobj3);
	LOG_CALL("callFunctionSyntheticOneArgVoid_cl " << asAtomHandler::toDebugString(func) << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(value));
	ASATOM_INCREF(value);
	asAtom ret = asAtomHandler::invalidAtom;
	asAtomHandler::getObjectNoCheck(func)->as<SyntheticFunction>()->call(context->worker,ret,obj, &value, 1,false,(instrptr->local3.flags&ABC_OP_COERCED)==0);
	ASATOM_DECREF(ret);
	if (!fromglobal && context->exec_pos->cacheobj3->as<IFunction>()->clonedFrom)
		context->exec_pos->cacheobj3->decRef();
	++(context->exec_pos);
}
void lightspark::abc_callFunctionSyntheticOneArgVoid_local_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	asAtom value = CONTEXT_GETLOCAL(context,instrptr->local_pos2);
	bool fromglobal = (++context->exec_pos)->local2.flags & ABC_OP_FROMGLOBAL;
	asAtom func = fromglobal ? context->exec_pos->cachedvar3->var : asAtomHandler::fromObjectNoPrimitive(context->exec_pos->cacheobj3);
	LOG_CALL("callFunctionSyntheticOneArgVoid_ll " << asAtomHandler::toDebugString(func) << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(value));
	ASATOM_INCREF(obj); // ensure that obj stays valid during call
	ASATOM_INCREF(value);
	asAtom ret = asAtomHandler::invalidAtom;
	asAtomHandler::getObjectNoCheck(func)->as<SyntheticFunction>()->call(context->worker,ret,obj, &value, 1,false,(instrptr->local3.flags&ABC_OP_COERCED)==0);
	ASATOM_DECREF(obj);
	ASATOM_DECREF(ret);
	if (!fromglobal && context->exec_pos->cacheobj3->as<IFunction>()->clonedFrom)
		context->exec_pos->cacheobj3->decRef();
	++(context->exec_pos);
}
void lightspark::abc_callFunctionSyntheticOneArg_constant_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = *instrptr->arg1_constant;
	asAtom value = *instrptr->arg2_constant;
	bool fromglobal = (++context->exec_pos)->local2.flags & ABC_OP_FROMGLOBAL;
	asAtom func = fromglobal ? context->exec_pos->cachedvar3->var : asAtomHandler::fromObjectNoPrimitive(context->exec_pos->cacheobj3);
	LOG_CALL("callFunctionSyntheticOneArg_cc " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(value));
	asAtom ret = asAtomHandler::invalidAtom;
	asAtomHandler::getObjectNoCheck(func)->as<SyntheticFunction>()->call(context->worker,ret,obj, &value, 1,false,(instrptr->local3.flags&ABC_OP_COERCED)==0);
	RUNTIME_STACK_PUSH(context,ret);
	if (!fromglobal && context->exec_pos->cacheobj3->as<IFunction>()->clonedFrom)
		context->exec_pos->cacheobj3->decRef();
	++(context->exec_pos);
}
void lightspark::abc_callFunctionSyntheticOneArg_local_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	asAtom value = *instrptr->arg2_constant;
	bool fromglobal = (++context->exec_pos)->local2.flags & ABC_OP_FROMGLOBAL;
	asAtom func = fromglobal ? context->exec_pos->cachedvar3->var : asAtomHandler::fromObjectNoPrimitive(context->exec_pos->cacheobj3);
	LOG_CALL("callFunctionSyntheticOneArg_lc " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(value));
	ASATOM_INCREF(obj); // ensure that obj stays valid during call
	asAtom ret = asAtomHandler::invalidAtom;
	asAtomHandler::getObjectNoCheck(func)->as<SyntheticFunction>()->call(context->worker,ret,obj, &value, 1,false,(instrptr->local3.flags&ABC_OP_COERCED)==0);
	ASATOM_DECREF(obj);
	RUNTIME_STACK_PUSH(context,ret);
	if (!fromglobal && context->exec_pos->cacheobj3->as<IFunction>()->clonedFrom)
		context->exec_pos->cacheobj3->decRef();
	++(context->exec_pos);
}
void lightspark::abc_callFunctionSyntheticOneArg_constant_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = *instrptr->arg1_constant;
	asAtom value = CONTEXT_GETLOCAL(context,instrptr->local_pos2);
	bool fromglobal = (++context->exec_pos)->local2.flags & ABC_OP_FROMGLOBAL;
	asAtom func = fromglobal ? context->exec_pos->cachedvar3->var : asAtomHandler::fromObjectNoPrimitive(context->exec_pos->cacheobj3);
	LOG_CALL("callFunctionSyntheticOneArg_cl " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(value));
	ASATOM_INCREF(value);
	asAtom ret = asAtomHandler::invalidAtom;
	asAtomHandler::getObjectNoCheck(func)->as<SyntheticFunction>()->call(context->worker,ret,obj, &value, 1,false,(instrptr->local3.flags&ABC_OP_COERCED)==0);
	RUNTIME_STACK_PUSH(context,ret);
	if (!fromglobal && context->exec_pos->cacheobj3->as<IFunction>()->clonedFrom)
		context->exec_pos->cacheobj3->decRef();
	++(context->exec_pos);
}
void lightspark::abc_callFunctionSyntheticOneArg_local_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	asAtom value = CONTEXT_GETLOCAL(context,instrptr->local_pos2);
	bool fromglobal = (++context->exec_pos)->local2.flags & ABC_OP_FROMGLOBAL;
	asAtom func = fromglobal ? context->exec_pos->cachedvar3->var : asAtomHandler::fromObjectNoPrimitive(context->exec_pos->cacheobj3);
	LOG_CALL("callFunctionSyntheticOneArg_ll " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(value));
	ASATOM_INCREF(obj); // ensure that obj stays valid during call
	ASATOM_INCREF(value);
	asAtom ret = asAtomHandler::invalidAtom;
	asAtomHandler::getObjectNoCheck(func)->as<SyntheticFunction>()->call(context->worker,ret,obj, &value, 1,false,(instrptr->local3.flags&ABC_OP_COERCED)==0);
	ASATOM_DECREF(obj);
	RUNTIME_STACK_PUSH(context,ret);
	if (!fromglobal && context->exec_pos->cacheobj3->as<IFunction>()->clonedFrom)
		context->exec_pos->cacheobj3->decRef();
	++(context->exec_pos);
}
void lightspark::abc_callFunctionSyntheticOneArg_constant_constant_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = *instrptr->arg1_constant;
	asAtom value = *instrptr->arg2_constant;
	bool fromglobal = (++context->exec_pos)->local2.flags & ABC_OP_FROMGLOBAL;
	asAtom func = fromglobal ? context->exec_pos->cachedvar3->var : asAtomHandler::fromObjectNoPrimitive(context->exec_pos->cacheobj3);
	LOG_CALL("callFunctionSyntheticOneArg_ccl " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(value));
	asAtom oldres = CONTEXT_GETLOCAL(context,instrptr->local3.pos);
	asAtomHandler::getObjectNoCheck(func)->as<SyntheticFunction>()->call(context->worker,CONTEXT_GETLOCAL(context,instrptr->local3.pos),obj, &value, 1,false,(instrptr->local3.flags&ABC_OP_COERCED)==0,instrptr->local3.pos);
	ASATOM_DECREF(oldres);
	if (!fromglobal && context->exec_pos->cacheobj3->as<IFunction>()->clonedFrom)
		context->exec_pos->cacheobj3->decRef();
	++(context->exec_pos);
}
void lightspark::abc_callFunctionSyntheticOneArg_local_constant_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	asAtom value = *instrptr->arg2_constant;
	bool fromglobal = (++context->exec_pos)->local2.flags & ABC_OP_FROMGLOBAL;
	asAtom func = fromglobal ? context->exec_pos->cachedvar3->var : asAtomHandler::fromObjectNoPrimitive(context->exec_pos->cacheobj3);
	LOG_CALL("callFunctionSyntheticOneArg_lcl " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(value));
	ASATOM_INCREF(obj); // ensure that obj stays valid during call
	asAtom oldres = CONTEXT_GETLOCAL(context,instrptr->local3.pos);
	asAtomHandler::getObjectNoCheck(func)->as<SyntheticFunction>()->call(context->worker,CONTEXT_GETLOCAL(context,instrptr->local3.pos),obj, &value, 1,false,(instrptr->local3.flags&ABC_OP_COERCED)==0,instrptr->local3.pos);
	ASATOM_DECREF(obj);
	ASATOM_DECREF(oldres);
	if (!fromglobal && context->exec_pos->cacheobj3->as<IFunction>()->clonedFrom)
		context->exec_pos->cacheobj3->decRef();
	++(context->exec_pos);
}
void lightspark::abc_callFunctionSyntheticOneArg_constant_local_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = *instrptr->arg1_constant;
	asAtom value = CONTEXT_GETLOCAL(context,instrptr->local_pos2);
	bool fromglobal = (++context->exec_pos)->local2.flags & ABC_OP_FROMGLOBAL;
	asAtom func = fromglobal ? context->exec_pos->cachedvar3->var : asAtomHandler::fromObjectNoPrimitive(context->exec_pos->cacheobj3);
	LOG_CALL("callFunctionSyntheticOneArg_cll " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(value));
	ASATOM_INCREF(value);
	asAtom oldres = CONTEXT_GETLOCAL(context,instrptr->local3.pos);
	asAtomHandler::getObjectNoCheck(func)->as<SyntheticFunction>()->call(context->worker,CONTEXT_GETLOCAL(context,instrptr->local3.pos),obj, &value, 1,false,(instrptr->local3.flags&ABC_OP_COERCED)==0,instrptr->local3.pos);
	ASATOM_DECREF(oldres);
	if (!fromglobal && context->exec_pos->cacheobj3->as<IFunction>()->clonedFrom)
		context->exec_pos->cacheobj3->decRef();
	++(context->exec_pos);
}
void lightspark::abc_callFunctionSyntheticOneArg_local_local_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	asAtom value = CONTEXT_GETLOCAL(context,instrptr->local_pos2);
	bool fromglobal = (++context->exec_pos)->local2.flags & ABC_OP_FROMGLOBAL;
	asAtom func = fromglobal ? context->exec_pos->cachedvar3->var : asAtomHandler::fromObjectNoPrimitive(context->exec_pos->cacheobj3);
	LOG_CALL("callFunctionSyntheticOneArg_lll " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(value)<<" " <<asAtomHandler::toDebugString(func));
	ASATOM_INCREF(obj); // ensure that obj stays valid during call
	ASATOM_INCREF(value);
	asAtom oldres = CONTEXT_GETLOCAL(context,instrptr->local3.pos);
	asAtomHandler::getObjectNoCheck(func)->as<SyntheticFunction>()->call(context->worker,CONTEXT_GETLOCAL(context,instrptr->local3.pos),obj, &value, 1,false,(instrptr->local3.flags&ABC_OP_COERCED)==0,instrptr->local3.pos);
	ASATOM_DECREF(obj);
	ASATOM_DECREF(oldres);
	if (!fromglobal && context->exec_pos->cacheobj3->as<IFunction>()->clonedFrom)
		context->exec_pos->cacheobj3->decRef();
	++(context->exec_pos);
}



void lightspark::abc_callFunctionSyntheticMultiArgsVoid(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t argcount = instrptr->local2.pos;
	SyntheticFunction* func = instrptr->cacheobj3->as<SyntheticFunction>();
	LOG_CALL("callFunctionSyntheticMultiArgsVoid " << func->getSystemState()->getStringFromUniqueId(func->functionname) << ' ' <<argcount);
	RUNTIME_STACK_POP_N_CREATE(context,argcount+1,args);
	asAtom ret = asAtomHandler::invalidAtom;
	func->call(context->worker,ret,*args, args+1, argcount,false,(instrptr->local2.flags&ABC_OP_COERCED)==0);
	ASATOM_DECREF(ret);
	ASATOM_DECREF_POINTER(args);
	if (instrptr->cacheobj3->as<IFunction>()->clonedFrom)
		instrptr->cacheobj3->decRef();
	++(context->exec_pos);
}
void lightspark::abc_callFunctionSyntheticMultiArgsVoid_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = *instrptr->arg1_constant;
	uint32_t argcount = instrptr->local2.pos;
	bool fromglobal = instrptr->local2.flags & ABC_OP_FROMGLOBAL;
	asAtom func = fromglobal ? instrptr->cachedvar3->var : asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj3);
	LOG_CALL("callFunctionSyntheticMultiArgsVoid_c " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" "<<argcount);
	asAtom* args = g_newa(asAtom,argcount);
	for (uint32_t i = argcount; i > 0 ; i--)
	{
		(++(context->exec_pos));
		if (context->exec_pos->arg2_uint == OPERANDTYPES::OP_LOCAL || context->exec_pos->arg2_uint == OPERANDTYPES::OP_CACHED_SLOT)
		{
			args[i-1] = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
			ASATOM_INCREF(args[i-1]);
		}
		else
			args[i-1] = *context->exec_pos->arg1_constant;
	}
	asAtom ret = asAtomHandler::invalidAtom;
	asAtomHandler::getObjectNoCheck(func)->as<SyntheticFunction>()->call(context->worker,ret,obj, args, argcount,false,(instrptr->local2.flags&ABC_OP_COERCED)==0);
	ASATOM_DECREF(ret);
	if (!fromglobal && instrptr->cacheobj3->as<IFunction>()->clonedFrom)
		instrptr->cacheobj3->decRef();
	++(context->exec_pos);
}
void lightspark::abc_callFunctionSyntheticMultiArgsVoid_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	uint32_t argcount = instrptr->local2.pos;
	bool fromglobal = instrptr->local2.flags & ABC_OP_FROMGLOBAL;
	asAtom func = fromglobal ? instrptr->cachedvar3->var : asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj3);
	LOG_CALL("callFunctionSyntheticMultiArgsVoid_l " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" "<<argcount);
	asAtom* args = g_newa(asAtom,argcount);
	for (uint32_t i = argcount; i > 0 ; i--)
	{
		(++(context->exec_pos));
		if (context->exec_pos->arg2_uint == OPERANDTYPES::OP_LOCAL || context->exec_pos->arg2_uint == OPERANDTYPES::OP_CACHED_SLOT)
		{
			args[i-1] = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
			ASATOM_INCREF(args[i-1]);
		}
		else
			args[i-1] = *context->exec_pos->arg1_constant;
	}
	asAtom ret = asAtomHandler::invalidAtom;
	asAtomHandler::getObjectNoCheck(func)->as<SyntheticFunction>()->call(context->worker,ret,obj, args, argcount,false,(instrptr->local2.flags&ABC_OP_COERCED)==0);
	ASATOM_DECREF(ret);
	if (!fromglobal && instrptr->cacheobj3->as<IFunction>()->clonedFrom)
		instrptr->cacheobj3->decRef();
	++(context->exec_pos);
}
void lightspark::abc_callFunctionSyntheticMultiArgs(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t argcount = instrptr->local2.pos;
	bool fromglobal = instrptr->local2.flags & ABC_OP_FROMGLOBAL;
	asAtom func = fromglobal ? instrptr->cachedvar3->var : asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj3);
	LOG_CALL("callFunctionSyntheticMultiArgs " << asAtomHandler::toDebugString(func) << ' ' <<argcount);
	RUNTIME_STACK_POP_N_CREATE(context,argcount+1,args);
	asAtom ret = asAtomHandler::invalidAtom;
	asAtomHandler::as<SyntheticFunction>(func)->call(context->worker,ret,*args, args+1, argcount,false,(instrptr->local2.flags&ABC_OP_COERCED)==0);
	ASATOM_DECREF_POINTER(args);
	RUNTIME_STACK_PUSH(context,ret);
	if (!fromglobal && instrptr->cacheobj3->as<IFunction>()->clonedFrom)
		instrptr->cacheobj3->decRef();
	++(context->exec_pos);
}
void lightspark::abc_callFunctionSyntheticMultiArgs_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = *instrptr->arg1_constant;
	uint32_t argcount = instrptr->local2.pos;
	bool fromglobal = instrptr->local2.flags & ABC_OP_FROMGLOBAL;
	asAtom func = fromglobal ? instrptr->cachedvar3->var : asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj3);
	LOG_CALL("callFunctionSyntheticMultiArgs_c " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" "<<argcount);
	asAtom* args = g_newa(asAtom,argcount);
	for (uint32_t i = argcount; i > 0 ; i--)
	{
		(++(context->exec_pos));
		if (context->exec_pos->arg2_uint == OPERANDTYPES::OP_LOCAL || context->exec_pos->arg2_uint == OPERANDTYPES::OP_CACHED_SLOT)
		{
			args[i-1] = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
			ASATOM_INCREF(args[i-1]);
		}
		else
			args[i-1] = *context->exec_pos->arg1_constant;
	}
	asAtom ret = asAtomHandler::invalidAtom;
	asAtomHandler::getObjectNoCheck(func)->as<SyntheticFunction>()->call(context->worker,ret,obj, args, argcount,false,(instrptr->local2.flags&ABC_OP_COERCED)==0);
	RUNTIME_STACK_PUSH(context,ret);
	if (!fromglobal && instrptr->cacheobj3->as<IFunction>()->clonedFrom)
		instrptr->cacheobj3->decRef();
	++(context->exec_pos);
}
void lightspark::abc_callFunctionSyntheticMultiArgs_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	uint32_t argcount = instrptr->local2.pos;
	bool fromglobal = instrptr->local2.flags & ABC_OP_FROMGLOBAL;
	asAtom func = fromglobal ? instrptr->cachedvar3->var : asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj3);
	LOG_CALL("callFunctionSyntheticMultiArgs_l " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" "<<argcount);
	asAtom* args = g_newa(asAtom,argcount);
	for (uint32_t i = argcount; i > 0 ; i--)
	{
		(++(context->exec_pos));
		if (context->exec_pos->arg2_uint == OPERANDTYPES::OP_LOCAL || context->exec_pos->arg2_uint == OPERANDTYPES::OP_CACHED_SLOT)
		{
			args[i-1] = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
			ASATOM_INCREF(args[i-1]);
		}
		else
			args[i-1] = *context->exec_pos->arg1_constant;
	}
	asAtom ret = asAtomHandler::invalidAtom;
	asAtomHandler::getObjectNoCheck(func)->as<SyntheticFunction>()->call(context->worker,ret,obj, args, argcount,false,(instrptr->local2.flags&ABC_OP_COERCED)==0);
	RUNTIME_STACK_PUSH(context,ret);
	if (!fromglobal && instrptr->cacheobj3->as<IFunction>()->clonedFrom)
		instrptr->cacheobj3->decRef();
	++(context->exec_pos);
}
void lightspark::abc_callFunctionSyntheticMultiArgs_constant_localResult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = *instrptr->arg1_constant;
	uint32_t argcount = instrptr->local2.pos;
	bool fromglobal = instrptr->local2.flags & ABC_OP_FROMGLOBAL;
	asAtom func = fromglobal ? instrptr->cachedvar3->var : asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj3);
	LOG_CALL("callFunctionSyntheticMultiArgs_c_lr " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" "<<argcount);
	asAtom* args = g_newa(asAtom,argcount);
	for (uint32_t i = argcount; i > 0 ; i--)
	{
		(++(context->exec_pos));
		if (context->exec_pos->arg2_uint == OPERANDTYPES::OP_LOCAL || context->exec_pos->arg2_uint == OPERANDTYPES::OP_CACHED_SLOT)
		{
			args[i-1] = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
			ASATOM_INCREF(args[i-1]);
		}
		else
			args[i-1] = *context->exec_pos->arg1_constant;
	}
	// local result pos is stored in the context->exec_pos of the last argument
	asAtom oldres = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	asAtomHandler::getObjectNoCheck(func)->as<SyntheticFunction>()->call(context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),obj, args, argcount,false,(instrptr->local2.flags&ABC_OP_COERCED)==0,context->exec_pos->local3.pos);
	ASATOM_DECREF(oldres);
	if (!fromglobal && instrptr->cacheobj3->as<IFunction>()->clonedFrom)
		instrptr->cacheobj3->decRef();
	++(context->exec_pos);
}
void lightspark::abc_callFunctionSyntheticMultiArgs_local_localResult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	uint32_t argcount = instrptr->local2.pos;
	bool fromglobal = instrptr->local2.flags & ABC_OP_FROMGLOBAL;
	asAtom func = fromglobal ? instrptr->cachedvar3->var : asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj3);
	LOG_CALL("callFunctionSyntheticMultiArgs_l_lr " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" "<<argcount);
	asAtom* args = g_newa(asAtom,argcount);
	for (uint32_t i = argcount; i > 0 ; i--)
	{
		(++(context->exec_pos));
		if (context->exec_pos->arg2_uint == OPERANDTYPES::OP_LOCAL || context->exec_pos->arg2_uint == OPERANDTYPES::OP_CACHED_SLOT)
		{
			args[i-1] = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
			ASATOM_INCREF(args[i-1]);
		}
		else
			args[i-1] = *context->exec_pos->arg1_constant;
	}
	// local result pos is stored in the context->exec_pos of the last argument
	asAtom oldres = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	asAtomHandler::getObjectNoCheck(func)->as<SyntheticFunction>()->call(context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),obj, args, argcount,false,(instrptr->local2.flags&ABC_OP_COERCED)==0,context->exec_pos->local3.pos);
	ASATOM_DECREF(oldres);
	if (!fromglobal && instrptr->cacheobj3->as<IFunction>()->clonedFrom)
		instrptr->cacheobj3->decRef();
	++(context->exec_pos);
}
