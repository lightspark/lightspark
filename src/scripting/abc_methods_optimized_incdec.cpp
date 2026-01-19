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
#include "scripting/abc_optimized_incdec.h"

using namespace std;
using namespace lightspark;

void lightspark::abc_increment_constant(call_context* context)
{
	asAtom res = *context->exec_pos->arg1_constant;
	LOG_CALL("increment_c "<<asAtomHandler::toDebugString(res));
	if (!asAtomHandler::increment(res,context->worker,false))
		ASATOM_INCREF(res);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void lightspark::abc_increment_local(call_context* context)
{
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	LOG_CALL("increment_l "<<context->exec_pos->local_pos1<<" "<<asAtomHandler::toDebugString(res));
	if (!asAtomHandler::increment(res,context->worker,false))
		ASATOM_INCREF(res);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void lightspark::abc_increment_constant_localresult(call_context* context)
{
	asAtom res = *context->exec_pos->arg1_constant;
	LOG_CALL("increment_cl "<<context->exec_pos->local3.pos<<" "<<asAtomHandler::toDebugString(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos))<<" "<<asAtomHandler::toDebugString(res));
	if (context->exec_pos->local_pos1 != context->exec_pos->local3.pos)
	{
		ASATOM_DECREF(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos));
		if (asAtomHandler::isNumber(res))
			asAtomHandler::setNumber(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),asAtomHandler::toNumber(res));
		else
			asAtomHandler::set(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),res);
	}
	else
		asAtomHandler::set(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),res);
	asAtomHandler::increment(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),context->worker,true);
	++(context->exec_pos);
}

void lightspark::abc_increment_local_localresult(call_context* context)
{
	LOG_CALL("increment_ll "<<context->exec_pos->local_pos1<<" "<<context->exec_pos->local3.pos<<" "<<asAtomHandler::toDebugString(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos))<<" "<<asAtomHandler::toDebugString(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1)));
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	if (context->exec_pos->local_pos1 != context->exec_pos->local3.pos)
	{
		ASATOM_DECREF(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos));
		if (asAtomHandler::isNumber(res))
			asAtomHandler::setNumber(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),asAtomHandler::toNumber(res));
		else
			asAtomHandler::set(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),res);
	}
	else
		asAtomHandler::set(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),res);
	asAtomHandler::increment(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),context->worker,true);
	++(context->exec_pos);
}
void lightspark::abc_decrement_constant(call_context* context)
{
	asAtom res = *context->exec_pos->arg1_constant;
	LOG_CALL("decrement_c "<<asAtomHandler::toDebugString(res));
	if (!asAtomHandler::decrement(res,context->worker,false))
		ASATOM_INCREF(res);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void lightspark::abc_decrement_local(call_context* context)
{
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	LOG_CALL("decrement_l "<<context->exec_pos->local_pos1<<" "<<asAtomHandler::toDebugString(res));
	if (!asAtomHandler::decrement(res,context->worker,false))
		ASATOM_INCREF(res);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void lightspark::abc_decrement_constant_localresult(call_context* context)
{
	asAtom res = *context->exec_pos->arg1_constant;
	LOG_CALL("decrement_cl "<<context->exec_pos->local3.pos<<" "<<asAtomHandler::toDebugString(res));
	if (context->exec_pos->local_pos1 != context->exec_pos->local3.pos)
	{
		ASATOM_DECREF(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos));
		if (asAtomHandler::isNumber(res))
			asAtomHandler::setNumber(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),asAtomHandler::toNumber(res));
		else
			asAtomHandler::set(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),res);
	}
	else
		asAtomHandler::set(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),res);
	asAtomHandler::decrement(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),context->worker,true);
	++(context->exec_pos);
}
void lightspark::abc_decrement_local_localresult(call_context* context)
{
	LOG_CALL("decrement_ll "<<context->exec_pos->local_pos1<<" "<<context->exec_pos->local3.pos<<" "<<asAtomHandler::toDebugString(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos)));
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	if (context->exec_pos->local_pos1 != context->exec_pos->local3.pos)
	{
		ASATOM_DECREF(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos));
		if (asAtomHandler::isNumber(res))
			asAtomHandler::setNumber(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),asAtomHandler::toNumber(res));
		else
			asAtomHandler::set(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),res);
	}
	else
		asAtomHandler::set(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),res);
	asAtomHandler::decrement(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),context->worker,true);
	++(context->exec_pos);
}
void lightspark::abc_increment_i_local(call_context* context)
{
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	LOG_CALL("increment_i_l "<<context->exec_pos->local_pos1<<" "<<asAtomHandler::toDebugString(res));
	asAtomHandler::increment_i(res);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void lightspark::abc_increment_i_local_localresult(call_context* context)
{
	LOG_CALL("increment_i_ll "<<context->exec_pos->local_pos1<<" "<<context->exec_pos->local3.pos<<" "<<asAtomHandler::toDebugString(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1)));
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	asAtom oldres = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	asAtomHandler::set(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),res);
	asAtomHandler::increment_i(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos));
	ASATOM_DECREF(oldres);
	++(context->exec_pos);
}
void lightspark::abc_increment_i_local_setslotnocoerce(call_context* context)
{
	LOG_CALL("increment_i_ls "<<context->exec_pos->local_pos1<<" "<<context->exec_pos->local3.pos<<" "<<asAtomHandler::toDebugString(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1)));
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	asAtomHandler::increment_i(res);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}
void lightspark::abc_decrement_i_local(call_context* context)
{
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	LOG_CALL("decrement_i_l "<<context->exec_pos->local_pos1<<" "<<asAtomHandler::toDebugString(res));
	asAtomHandler::decrement_i(res);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void lightspark::abc_decrement_i_local_localresult(call_context* context)
{
	LOG_CALL("decrement_i_ll "<<context->exec_pos->local_pos1<<" "<<context->exec_pos->local3.pos<<" "<<asAtomHandler::toDebugString(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos)));
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	asAtom oldres = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	asAtomHandler::set(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),res);
	asAtomHandler::decrement_i(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos));
	ASATOM_DECREF(oldres);
	++(context->exec_pos);
}
void lightspark::abc_decrement_i_local_setslotnocoerce(call_context* context)
{
	LOG_CALL("decrement_i_ls "<<context->exec_pos->local_pos1<<" "<<context->exec_pos->local3.pos<<" "<<asAtomHandler::toDebugString(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1)));
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	asAtomHandler::decrement_i(res);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}
void lightspark::abc_inclocal_i_optimized(call_context* context)
{
	int32_t t = context->exec_pos->arg1_uint;
	LOG_CALL( "incLocal_i_o " << t << " "<< context->exec_pos->arg2_int);
	asAtomHandler::increment_i(CONTEXT_GETLOCAL(context,t),context->exec_pos->arg2_int);
	++context->exec_pos;
}
void lightspark::abc_declocal_i_optimized(call_context* context)
{
	int32_t t = context->exec_pos->arg1_uint;
	LOG_CALL( "decLocal_i_o " << t << " "<< context->exec_pos->arg2_int);
	asAtomHandler::decrement_i(CONTEXT_GETLOCAL(context,t),context->exec_pos->arg2_int);
	++context->exec_pos;
}
void lightspark::abc_inclocal_i_postfix(call_context* context)
{
	int32_t t = context->exec_pos->arg1_uint;
	asAtom& res = CONTEXT_GETLOCAL(context,t);
	asAtom oldres = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	if (USUALLY_TRUE(
			asAtomHandler::isInteger(res)
			&& !asAtomHandler::isObject(oldres)))
	{
		// fast path for common case that argument is int and the result doesn't overflow
		LOG_CALL( "incLocal_i_postfix_fast " << t <<" "<<context->exec_pos->local3.pos);
		asAtomHandler::set(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),res);
		asAtomHandler::setInt(res,asAtomHandler::getInt(res)+1);
	}
	else
	{
		LOG_CALL( "incLocal_i_postfix " << t <<" "<<context->exec_pos->local3.pos);
		REPLACELOCALRESULT(context,context->exec_pos->local3.pos,res);
		asAtomHandler::increment_i(CONTEXT_GETLOCAL(context,t));
	}
	++context->exec_pos;
}
void lightspark::abc_declocal_i_postfix(call_context* context)
{
	int32_t t = context->exec_pos->arg1_uint;
	asAtom& res = CONTEXT_GETLOCAL(context,t);
	asAtom oldres = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	if (USUALLY_TRUE(
			asAtomHandler::isInteger(res)
			&& !asAtomHandler::isObject(oldres)))
	{
		// fast path for common case that argument is ints and the result doesn't overflow
		LOG_CALL( "decLocal_i_postfix_fast " << t <<" "<<context->exec_pos->local3.pos);
		asAtomHandler::set(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),res);
		asAtomHandler::setInt(res,asAtomHandler::getInt(res)-1);
	}
	else
	{
		LOG_CALL( "decLocal_i_postfix " << t <<" "<<context->exec_pos->local3.pos);
		REPLACELOCALRESULT(context,context->exec_pos->local3.pos,res);
		asAtomHandler::decrement_i(CONTEXT_GETLOCAL(context,t));
	}
	++context->exec_pos;
}
