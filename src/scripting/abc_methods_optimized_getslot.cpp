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
#include "scripting/abc_optimized_getslot.h"
#include "scripting/class.h"
#include "scripting/toplevel/Error.h"

using namespace std;
using namespace lightspark;

void lightspark::abc_getslot_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos++;
	uint32_t t = instrptr->arg2_uint;
	asAtom* pval = instrptr->arg1_constant;
	asAtom ret=asAtomHandler::getObjectNoCheck(*pval)->getSlotNoCheck(t);
	LOG_CALL("getSlot_c " << t << " " << asAtomHandler::toDebugString(ret));
	ASATOM_INCREF(ret);
	RUNTIME_STACK_PUSH(context,ret);
}
void lightspark::abc_getslot_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos++;
	uint32_t t = instrptr->arg2_uint;
	ASObject* obj = asAtomHandler::getObject(CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	if (!obj)
	{
		createError<TypeError>(context->worker,kConvertNullToObjectError);
		return;
	}
	asAtom res = obj->getSlotNoCheck(t);
	LOG_CALL("getSlot_l " << t << " " << asAtomHandler::toDebugString(res));
	ASATOM_INCREF(res);
	RUNTIME_STACK_PUSH(context,res);
}
void lightspark::abc_getslot_constant_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos++;
	uint32_t t = instrptr->arg2_uint;
	asAtom res = asAtomHandler::getObject(*instrptr->arg1_constant)->getSlotNoCheck(t);
	asAtom o = CONTEXT_GETLOCAL(context,instrptr->local3.pos);
	if (o.uintval != res.uintval)
	{
		if (asAtomHandler::isNumber(res))
		{
			ASATOM_DECREF(o);
			asAtomHandler::setNumber(CONTEXT_GETLOCAL(context,instrptr->local3.pos),asAtomHandler::getNumber(context->worker,res));
		}
		else
		{
			ASATOM_INCREF(res);
			REPLACELOCALRESULT(context,instrptr->local3.pos,res);
		}
	}
	LOG_CALL("getSlot_cl " << t << " " << asAtomHandler::toDebugString(res)<<" "<<asAtomHandler::toDebugString(*instrptr->arg1_constant));
}
void lightspark::abc_getslot_local_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos++;
	uint32_t t = instrptr->arg2_uint;
	if (!asAtomHandler::isObject(CONTEXT_GETLOCAL(context,instrptr->local_pos1)))
	{
		createError<TypeError>(context->worker,kConvertNullToObjectError);
		return;
	}
	ASObject* obj = asAtomHandler::getObjectNoCheck(CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	asAtom res = obj->getSlotNoCheck(t);
	asAtom o = CONTEXT_GETLOCAL(context,instrptr->local3.pos);
	if (o.uintval != res.uintval)
	{
		if (asAtomHandler::isNumberPtr(res))
		{
			ASATOM_DECREF(o);
			asAtomHandler::setNumber(CONTEXT_GETLOCAL(context,instrptr->local3.pos),asAtomHandler::getNumber(context->worker,res));
		}
		else
		{
			ASATOM_INCREF(res);
			REPLACELOCALRESULT(context,instrptr->local3.pos,res);
		}
	}
	LOG_CALL("getSlot_ll " << t << " " <<instrptr->local_pos1<<":"<< asAtomHandler::toDebugString(CONTEXT_GETLOCAL(context,instrptr->local_pos1))<<" "<< asAtomHandler::toDebugString(CONTEXT_GETLOCAL(context,instrptr->local3.pos)));
}
void lightspark::abc_getslot_constant_setslotnocoerce(call_context* context)
{
	uint32_t tget = context->exec_pos->arg2_uint;
	variable* var = asAtomHandler::getObject(*context->exec_pos->arg1_constant)->getSlotVar(tget+1);
	LOG_CALL("getSlot_cs " << tget << " " << asAtomHandler::toDebugString(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos)));

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotFromVariable(t,var);
	++(context->exec_pos);
}
void lightspark::abc_getslot_local_setslotnocoerce(call_context* context)
{
	uint32_t tget = context->exec_pos->arg2_uint;
	if (!asAtomHandler::isObject(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1)))
	{
		createError<TypeError>(context->worker,kConvertNullToObjectError);
		return;
	}
	variable* var = asAtomHandler::getObjectNoCheck(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1))->getSlotVar(tget+1);
	LOG_CALL("getSlot_ls " << tget << " " <<context->exec_pos->local_pos1<<":"<< asAtomHandler::toDebugString(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1))<<" "<< asAtomHandler::toDebugString(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos)));

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotFromVariable(t,var);
	++(context->exec_pos);
}
void lightspark::abc_getSlotFromScopeObject(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos++;
	uint32_t t = instrptr->arg1_uint;
	uint32_t tslot = instrptr->arg2_uint;
	assert_and_throw(context->curr_scope_stack > (uint32_t)t);
	ASObject* obj = asAtomHandler::getObjectNoCheck(context->scope_stack[t]);
	asAtom res = obj->getSlotNoCheck(tslot);
	LOG_CALL("getSlotFromScopeObject " << t << " " << tslot<<" "<< asAtomHandler::toDebugString(res) << " "<< obj->toDebugString());
	ASATOM_INCREF(res);
	RUNTIME_STACK_PUSH(context,res);
}
void lightspark::abc_getSlotFromScopeObject_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos++;
	uint32_t t = instrptr->arg1_uint;
	uint32_t tslot = instrptr->arg2_uint;
	assert_and_throw(context->curr_scope_stack > (uint32_t)t);
	ASObject* obj = asAtomHandler::getObjectNoCheck(context->scope_stack[t]);
	asAtom res = obj->getSlotNoCheck(tslot);
	LOG_CALL("getSlotFromScopeObject_l " << t << " " << tslot<<" "<< asAtomHandler::toDebugString(res) << " "<< obj->toDebugString()<<" "<<instrptr->local3.pos);
	if (res.uintval != CONTEXT_GETLOCAL(context,instrptr->local3.pos).uintval)
	{
		ASATOM_INCREF(res);
		REPLACELOCALRESULT(context,instrptr->local3.pos,res);
	}
}
