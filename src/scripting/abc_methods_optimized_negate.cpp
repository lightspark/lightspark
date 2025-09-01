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
#include "scripting/abc_optimized_negate.h"

using namespace std;
using namespace lightspark;

void lightspark::abc_negate_constant(call_context* context)
{
	LOG_CALL( "negate_c");
	asAtom res = *context->exec_pos->arg1_constant;
	if (asAtomHandler::isInteger(res) && asAtomHandler::toInt(res) != 0)
	{
		int32_t ret=-(asAtomHandler::toInt(res));
		asAtomHandler::setInt(res,context->worker, ret);
	}
	else
	{
		number_t ret=-(asAtomHandler::getNumber(context->worker,res));
		res = asAtomHandler::fromNumber(context->worker,ret,false);
	}
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void lightspark::abc_negate_local(call_context* context)
{
	LOG_CALL( "negate_l");
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	if (asAtomHandler::isInteger(res) && asAtomHandler::toInt(res) != 0)
	{
		int32_t ret=-(asAtomHandler::toInt(res));
		asAtomHandler::setInt(res,context->worker, ret);
	}
	else
	{
		number_t ret=-(asAtomHandler::getNumber(context->worker,res));
		res = asAtomHandler::fromNumber(context->worker,ret,false);
	}
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void lightspark::abc_negate_constant_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "negate_cl");
	asAtom res = *context->exec_pos->arg1_constant;
	if (asAtomHandler::isInteger(res) && asAtomHandler::toInt(res) != 0)
	{
		int32_t ret=-(asAtomHandler::toInt(res));
		asAtomHandler::setInt(res,context->worker, ret);
		REPLACELOCALRESULT(context,instrptr->local3.pos,res);
	}
	else
	{
		asAtom oldres = CONTEXT_GETLOCAL(context,instrptr->local3.pos);
		number_t ret=-(asAtomHandler::getNumber(context->worker,res));
		asAtomHandler::setNumber(CONTEXT_GETLOCAL(context,instrptr->local3.pos),context->worker,ret,instrptr->local3.pos);
		ASATOM_DECREF(oldres);
	}
	++(context->exec_pos);
}
void lightspark::abc_negate_local_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom res = CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	LOG_CALL( "negate_ll "<<instrptr->local_pos1<<" "<<instrptr->local3.pos<<" "<<asAtomHandler::toDebugString(res));
	if (asAtomHandler::isInteger(res) && asAtomHandler::toInt(res) != 0)
	{
		int32_t ret=-(asAtomHandler::toInt(res));
		asAtomHandler::setInt(res,context->worker, ret);
		REPLACELOCALRESULT(context,instrptr->local3.pos,res);
	}
	else
	{
		asAtom oldres = CONTEXT_GETLOCAL(context,instrptr->local3.pos);
		number_t ret=-(asAtomHandler::getNumber(context->worker,res));
		asAtomHandler::setNumber(CONTEXT_GETLOCAL(context,instrptr->local3.pos),context->worker,ret,instrptr->local3.pos);
		ASATOM_DECREF(oldres);
	}
	++(context->exec_pos);
}
void lightspark::abc_negate_constant_setslotnocoerce(call_context* context)
{
	asAtom res = *context->exec_pos->arg1_constant;
	if (asAtomHandler::isInteger(res) && asAtomHandler::toInt(res) != 0)
	{
		int32_t ret=-(asAtomHandler::toInt(res));
		asAtomHandler::setInt(res,context->worker, ret);
	}
	else
	{
		number_t ret=-(asAtomHandler::getNumber(context->worker,res));
		res = asAtomHandler::fromNumber(context->worker,ret,false);
	}
	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	LOG_CALL("negate_cs " << t << " "<< asAtomHandler::toDebugString(res) << " "<< asAtomHandler::toDebugString(obj));
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res,context->worker);
	++(context->exec_pos);
}
void lightspark::abc_negate_local_setslotnocoerce(call_context* context)
{
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	if (asAtomHandler::isInteger(res) && asAtomHandler::toInt(res) != 0)
	{
		int32_t ret=-(asAtomHandler::toInt(res));
		asAtomHandler::setInt(res,context->worker, ret);
	}
	else
	{
		number_t ret=-(asAtomHandler::getNumber(context->worker,res));
		res = asAtomHandler::fromNumber(context->worker,ret,false);
	}
	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	LOG_CALL("negate_ls " << t << " "<< asAtomHandler::toDebugString(res) << " "<< asAtomHandler::toDebugString(obj));
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res,context->worker);
	++(context->exec_pos);
}

