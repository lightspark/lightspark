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
#include "scripting/abc_optimized_getscopeobject.h"

using namespace std;
using namespace lightspark;

void lightspark::abc_getfuncscopeobject(call_context* context)
{
	uint32_t t = context->exec_pos->arg2_uint;
	assert(context->parent_scope_stack->scope.size() > t);
	LOG_CALL("getfuncscopeobject "<<t<<" "<<asAtomHandler::toDebugString(context->parent_scope_stack->scope[t].object));
	asAtom res =context->parent_scope_stack->scope[t].object;
	ASATOM_INCREF(res);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void lightspark::abc_getfuncscopeobject_localresult(call_context* context)
{
	uint32_t t = context->exec_pos->arg2_uint;
	assert(context->parent_scope_stack->scope.size() > t);
	LOG_CALL("getfuncscopeobject_l "<<t<<" "<<asAtomHandler::toDebugString(context->parent_scope_stack->scope[t].object));
	asAtom res =context->parent_scope_stack->scope[t].object;
	ASATOM_INCREF(res);
	REPLACELOCALRESULT(context,context->exec_pos->local3.pos,res);
	++(context->exec_pos);
}
void lightspark::abc_getscopeobject_localresult(call_context* context)
{
	uint32_t t = context->exec_pos->arg2_uint;
	assert_and_throw(context->curr_scope_stack > (uint32_t)t);
	asAtom ret=context->scope_stack[t];
	if (ret.uintval != CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos).uintval)
	{
		ASATOM_INCREF(ret);
		REPLACELOCALRESULT(context,context->exec_pos->local3.pos,ret);
	}
	LOG_CALL("getScopeObject_l " << asAtomHandler::toDebugString(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos))<<" "<<t<<" "<<context->exec_pos->local3.pos);
	++(context->exec_pos);
}

