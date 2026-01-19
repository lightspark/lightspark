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
#include "scripting/abc_optimized_add.h"

using namespace std;
using namespace lightspark;

void lightspark::abc_add_constant_constant(call_context* context)
{
	LOG_CALL("add_cc");
	asAtom res = *context->exec_pos->arg1_constant;
	asAtomHandler::add(res,*context->exec_pos->arg2_constant,context->worker,context->exec_pos->local3.flags & ABC_OP_FORCEINT);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void lightspark::abc_add_local_constant(call_context* context)
{
	LOG_CALL("add_lc");
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	ASObject* o = asAtomHandler::getObject(res);
	if (o)
		o->incRef(); // ensure old value is not replaced
	if (asAtomHandler::add(res,*context->exec_pos->arg2_constant,context->worker,context->exec_pos->local3.flags & ABC_OP_FORCEINT))
	{
		if (o)
			o->decRef();
	}
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void lightspark::abc_add_constant_local(call_context* context)
{
	LOG_CALL("add_cl");
	asAtom res = *context->exec_pos->arg1_constant;
	asAtomHandler::add(res,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->worker,context->exec_pos->local3.flags & ABC_OP_FORCEINT);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void lightspark::abc_add_local_local(call_context* context)
{
	LOG_CALL("add_ll");
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	ASObject* o = asAtomHandler::getObject(res);
	if (o)
		o->incRef(); // ensure old value is not replaced
	if (asAtomHandler::add(res,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->worker,context->exec_pos->local3.flags & ABC_OP_FORCEINT))
	{
		if (o)
			o->decRef();
	}
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void lightspark::abc_add_constant_constant_localresult(call_context* context)
{
	LOG_CALL("add_ccl");
	asAtomHandler::addreplace(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),context->worker,*context->exec_pos->arg1_constant,*context->exec_pos->arg2_constant,context->exec_pos->local3.flags & ABC_OP_FORCEINT);
	++(context->exec_pos);
}
void lightspark::abc_add_local_constant_localresult(call_context* context)
{
	LOG_CALL("add_lcl");
	asAtomHandler::addreplace(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),*context->exec_pos->arg2_constant,context->exec_pos->local3.flags & ABC_OP_FORCEINT);
	++(context->exec_pos);
}
void lightspark::abc_add_constant_local_localresult(call_context* context)
{
	LOG_CALL("add_cll");
	asAtomHandler::addreplace(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),context->worker,*context->exec_pos->arg1_constant,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->exec_pos->local3.flags & ABC_OP_FORCEINT);
	++(context->exec_pos);
}

void lightspark::abc_add_local_local_localresult(call_context* context)
{
	LOG_CALL("add_lll "<<context->exec_pos->local_pos1<<"/"<<context->exec_pos->local_pos2<<"/"<<context->exec_pos->local3.pos);
	asAtomHandler::addreplace(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->exec_pos->local3.flags & ABC_OP_FORCEINT);
	++(context->exec_pos);
}
void lightspark::abc_add_constant_constant_setslotnocoerce(call_context* context)
{
	LOG_CALL("add_ccs");
	asAtom res = asAtomHandler::invalidAtom;
	asAtomHandler::addreplace(res,context->worker,*context->exec_pos->arg1_constant,*context->exec_pos->arg2_constant,context->exec_pos->local3.flags & ABC_OP_FORCEINT);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}
void lightspark::abc_add_local_constant_setslotnocoerce(call_context* context)
{
	LOG_CALL("add_lcs");
	asAtom res = asAtomHandler::invalidAtom;
	asAtomHandler::addreplace(res,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),*context->exec_pos->arg2_constant,context->exec_pos->local3.flags & ABC_OP_FORCEINT);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}
void lightspark::abc_add_constant_local_setslotnocoerce(call_context* context)
{
	LOG_CALL("add_cls");
	asAtom res = asAtomHandler::invalidAtom;
	asAtomHandler::addreplace(res,context->worker,*context->exec_pos->arg1_constant,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->exec_pos->local3.flags & ABC_OP_FORCEINT);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}
void lightspark::abc_add_local_local_setslotnocoerce(call_context* context)
{
	LOG_CALL("add_lls");
	asAtom res = asAtomHandler::invalidAtom;
	asAtomHandler::addreplace(res,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->exec_pos->local3.flags & ABC_OP_FORCEINT);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}
