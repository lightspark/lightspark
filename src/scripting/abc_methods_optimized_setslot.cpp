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
#include "scripting/abc_optimized_setslot.h"

using namespace std;
using namespace lightspark;

void lightspark::abc_setslot_constant_constant(call_context* context)
{
	asAtom v1 = *context->exec_pos->arg1_constant;
	asAtom v2 = *context->exec_pos->arg2_constant;
	uint32_t t = context->exec_pos->arg3_uint;
	LOG_CALL("setSlot_cc " << t << " "<< asAtomHandler::toDebugString(v2) << " "<< asAtomHandler::toDebugString(v1));
	if (asAtomHandler::getObject(v1)->setSlot(context->worker,t,v2)==TTRUE)
		ASATOM_INCREF(v2);
	++(context->exec_pos);
}
void lightspark::abc_setslot_local_constant(call_context* context)
{
	asAtom v1 = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	asAtom v2 = *context->exec_pos->arg2_constant;
	uint32_t t = context->exec_pos->arg3_uint;
	LOG_CALL("setSlot_lc " << t << " "<< asAtomHandler::toDebugString(v2) << " "<< asAtomHandler::toDebugString(v1));
	if (asAtomHandler::getObject(v1)->setSlot(context->worker,t,v2)==TTRUE)
		ASATOM_INCREF(v2);
	++(context->exec_pos);
}
void lightspark::abc_setslot_constant_local(call_context* context)
{
	asAtom v1 = *context->exec_pos->arg1_constant;
	asAtom v2 = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2);
	uint32_t t = context->exec_pos->arg3_uint;
	LOG_CALL("setSlot_cl " << t << " "<< asAtomHandler::toDebugString(v2) << " "<< asAtomHandler::toDebugString(v1));
	if (asAtomHandler::getObject(v1)->setSlot(context->worker,t,v2)==TTRUE)
		ASATOM_INCREF(v2);
	++(context->exec_pos);
}
void lightspark::abc_setslot_local_local(call_context* context)
{
	asAtom v1 = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	asAtom v2 = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2);
	uint32_t t = context->exec_pos->arg3_uint;
	LOG_CALL("setSlot_ll " << t << " "<< asAtomHandler::toDebugString(v2) << " "<< asAtomHandler::toDebugString(v1));
	if (asAtomHandler::getObject(v1)->setSlot(context->worker,t,v2)==TTRUE)
		ASATOM_INCREF(v2);
	++(context->exec_pos);
}
void lightspark::abc_setslotNoCoerce_constant_constant(call_context* context)
{
	asAtom v1 = *context->exec_pos->arg1_constant;
	asAtom v2 = *context->exec_pos->arg2_constant;
	uint32_t t = context->exec_pos->arg3_uint;
	LOG_CALL("setSlotNoCoerce_cc " << t << " "<< asAtomHandler::toDebugString(v2) << " "<< asAtomHandler::toDebugString(v1));
	asAtomHandler::getObjectNoCheck(v1)->setSlotNoCoerce(t,v2,context->worker);
	++(context->exec_pos);
}
void lightspark::abc_setslotNoCoerce_local_constant(call_context* context)
{
	asAtom v1 = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	asAtom v2 = *context->exec_pos->arg2_constant;
	uint32_t t = context->exec_pos->arg3_uint;
	LOG_CALL("setSlotNoCoerce_lc " << t << " "<< asAtomHandler::toDebugString(v2) << " "<< asAtomHandler::toDebugString(v1));
	asAtomHandler::getObjectNoCheck(v1)->setSlotNoCoerce(t,v2,context->worker);
	++(context->exec_pos);
}
void lightspark::abc_setslotNoCoerce_constant_local(call_context* context)
{
	asAtom v1 = *context->exec_pos->arg1_constant;
	asAtom v2 = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2);
	uint32_t t = context->exec_pos->arg3_uint;
	LOG_CALL("setSlotNoCoerce_cl " << t << " "<< asAtomHandler::toDebugString(v2) << " "<< asAtomHandler::toDebugString(v1));
	asAtomHandler::getObjectNoCheck(v1)->setSlotNoCoerce(t,v2,context->worker);
	++(context->exec_pos);
}
void lightspark::abc_setslotNoCoerce_local_local(call_context* context)
{
	asAtom v1 = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	asAtom v2 = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2);
	uint32_t t = context->exec_pos->arg3_uint;
	LOG_CALL("setSlotNoCoerce_ll " << t << " "<< asAtomHandler::toDebugString(v2) << " "<< asAtomHandler::toDebugString(v1));
	asAtomHandler::getObjectNoCheck(v1)->setSlotNoCoerce(t,v2,context->worker);
	++(context->exec_pos);
}
