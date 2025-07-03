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

using namespace std;
using namespace lightspark;

void lightspark::abc_bitnot_constant(call_context* context)
{
	LOG_CALL( "bitnot_c");
	asAtom res = *context->exec_pos->arg1_constant;
	asAtomHandler::bitnot(res,context->worker);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void lightspark::abc_bitnot_local(call_context* context)
{
	LOG_CALL( "bitnot_l");
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	asAtomHandler::bitnot(res,context->worker);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void lightspark::abc_bitnot_constant_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "bitnot_cl");
	asAtom res = *context->exec_pos->arg1_constant;
	asAtomHandler::bitnot(res,context->worker);
	REPLACELOCALRESULT(context,instrptr->local3.pos,res);
	++(context->exec_pos);
}
void lightspark::abc_bitnot_local_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "bitnot_ll");
	asAtom res = CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	asAtomHandler::bitnot(res,context->worker);
	REPLACELOCALRESULT(context,instrptr->local3.pos,res);
	++(context->exec_pos);
}
void lightspark::abc_bitnot_constant_setslotnocoerce(call_context* context)
{
	asAtom res = *context->exec_pos->arg1_constant;
	asAtomHandler::bitnot(res,context->worker);
	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	LOG_CALL("bitnot_cs " << t << " "<< asAtomHandler::toDebugString(res) << " "<< asAtomHandler::toDebugString(obj));
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res,context->worker);
	++(context->exec_pos);
}
void lightspark::abc_bitnot_local_setslotnocoerce(call_context* context)
{
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	asAtomHandler::bitnot(res,context->worker);
	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	LOG_CALL("bitnot_ls " << t << " "<< asAtomHandler::toDebugString(res) << " "<< asAtomHandler::toDebugString(obj));
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res,context->worker);
	++(context->exec_pos);
}

