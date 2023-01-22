/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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
#include "compat.h"
#include "exceptions.h"
#include "scripting/abcutils.h"
#include "scripting/class.h"
#include "scripting/toplevel/ASString.h"
#include "scripting/toplevel/RegExp.h"
#include "scripting/toplevel/Vector.h"
#include "parsing/streams.h"
#include <string>
#include <sstream>

using namespace std;
using namespace lightspark;

FORCE_INLINE void replacelocalresult(call_context* context,uint16_t pos,asAtom& ret)
{
	if (USUALLY_FALSE(context->exceptionthrown))
		return;
	asAtom oldres = CONTEXT_GETLOCAL(context,pos);
	asAtomHandler::set(CONTEXT_GETLOCAL(context,pos),ret);
	ASATOM_DECREF(oldres);
}
void ABCVm::abc_ifnlt_constant_constant(call_context* context)
{
	bool cond=!(asAtomHandler::isLess(*context->exec_pos->arg1_constant,context->worker,*context->exec_pos->arg2_constant) == TTRUE);
	LOG_CALL("ifNLT_cc (" << ((cond)?"taken)":"not taken)"));
	if(cond)
		context->exec_pos += context->exec_pos->arg3_int;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifnlt_local_constant(call_context* context)
{
	bool cond=!(asAtomHandler::isLess(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),context->worker,*context->exec_pos->arg2_constant) == TTRUE);
	LOG_CALL("ifNLT_lc (" << ((cond)?"taken)":"not taken)"));
	if(cond)
		context->exec_pos += context->exec_pos->arg3_int;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifnlt_constant_local(call_context* context)
{
	bool cond=!(asAtomHandler::isLess(*context->exec_pos->arg1_constant,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2)) == TTRUE);
	LOG_CALL("ifNLT_cl (" << ((cond)?"taken)":"not taken)"));
	if(cond)
		context->exec_pos += context->exec_pos->arg3_int;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifnlt_local_local(call_context* context)
{
	bool cond=!(asAtomHandler::isLess(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2)) == TTRUE);
	LOG_CALL("ifNLT_ll (" << ((cond)?"taken)":"not taken)"));
	if(cond)
		context->exec_pos += context->exec_pos->arg3_int;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifnge_constant_constant(call_context* context)
{
	bool cond=!(asAtomHandler::isLess(*context->exec_pos->arg1_constant,context->worker,*context->exec_pos->arg2_constant) == TFALSE);
	LOG_CALL("ifNGE_cc (" << ((cond)?"taken)":"not taken)"));
	if(cond)
		context->exec_pos += context->exec_pos->arg3_int;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifnge_local_constant(call_context* context)
{
	bool cond=!(asAtomHandler::isLess(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),context->worker,*context->exec_pos->arg2_constant) == TFALSE);
	LOG_CALL("ifNGE_lc (" << ((cond)?"taken)":"not taken)"));
	if(cond)
		context->exec_pos += context->exec_pos->arg3_int;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifnge_constant_local(call_context* context)
{
	bool cond=!(asAtomHandler::isLess(*context->exec_pos->arg1_constant,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2)) == TFALSE);
	LOG_CALL("ifNGE_cl (" << ((cond)?"taken)":"not taken)"));
	if(cond)
		context->exec_pos += context->exec_pos->arg3_int;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifnge_local_local(call_context* context)
{
	bool cond=!(asAtomHandler::isLess(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2)) == TFALSE);
	LOG_CALL("ifNGE_ll (" << ((cond)?"taken)":"not taken)")<<" "<<asAtomHandler::toDebugString(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1))<<" "<<asAtomHandler::toDebugString(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2)));
	if(cond)
		context->exec_pos += context->exec_pos->arg3_int;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifnle_constant_constant(call_context* context)
{
	bool cond=!(asAtomHandler::isLess(*context->exec_pos->arg2_constant,context->worker,*context->exec_pos->arg1_constant) == TFALSE);
	LOG_CALL("ifNLE_cc (" << ((cond)?"taken)":"not taken)"));
	if(cond)
		context->exec_pos += context->exec_pos->arg3_int;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifnle_local_constant(call_context* context)
{
	bool cond=!(asAtomHandler::isLess(*context->exec_pos->arg2_constant,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1)) == TFALSE);
	LOG_CALL("ifNLE_lc (" << ((cond)?"taken)":"not taken)"));
	if(cond)
		context->exec_pos += context->exec_pos->arg3_int;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifnle_constant_local(call_context* context)
{
	bool cond=!(asAtomHandler::isLess(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->worker,*context->exec_pos->arg1_constant) == TFALSE);
	LOG_CALL("ifNLE_cl (" << ((cond)?"taken)":"not taken)"));
	if(cond)
		context->exec_pos += context->exec_pos->arg3_int;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifnle_local_local(call_context* context)
{
	bool cond=!(asAtomHandler::isLess(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1)) == TFALSE);
	LOG_CALL("ifNLE_ll (" << ((cond)?"taken)":"not taken)")<<" "<<asAtomHandler::toDebugString(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1))<<" "<<asAtomHandler::toDebugString(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2)));
	if(cond)
		context->exec_pos += context->exec_pos->arg3_int;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifngt_constant_constant(call_context* context)
{
	bool cond=!(asAtomHandler::isLess(*context->exec_pos->arg2_constant,context->worker,*context->exec_pos->arg1_constant) == TTRUE);
	LOG_CALL("ifNGT_cc (" << ((cond)?"taken)":"not taken)"));
	if(cond)
		context->exec_pos += context->exec_pos->arg3_int;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifngt_local_constant(call_context* context)
{
	bool cond=!(asAtomHandler::isLess(*context->exec_pos->arg2_constant,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1)) == TTRUE);
	LOG_CALL("ifNGT_lc (" << ((cond)?"taken)":"not taken)"));
	if(cond)
		context->exec_pos += context->exec_pos->arg3_int;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifngt_constant_local(call_context* context)
{
	bool cond=!(asAtomHandler::isLess(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->worker,*context->exec_pos->arg1_constant) == TTRUE);
	LOG_CALL("ifNGT_cl (" << ((cond)?"taken)":"not taken)"));
	if(cond)
		context->exec_pos += context->exec_pos->arg3_int;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifngt_local_local(call_context* context)
{
	bool cond=!(asAtomHandler::isLess(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1)) == TTRUE);
	LOG_CALL("ifNGT_ll (" << ((cond)?"taken)":"not taken)")<<" "<<asAtomHandler::toDebugString(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1))<<" "<<asAtomHandler::toDebugString(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2)));
	if(cond)
		context->exec_pos += context->exec_pos->arg3_int;
	else
		++(context->exec_pos);
}
void ABCVm::abc_iftrue_constant(call_context* context)
{
	bool cond=asAtomHandler::Boolean_concrete(*context->exec_pos->arg1_constant);
	LOG_CALL("ifTrue_c (" << ((cond)?"taken)":"not taken)"));
	if(cond)
		context->exec_pos += context->exec_pos->arg3_int;
	else
		++(context->exec_pos);
}
void ABCVm::abc_iftrue_local(call_context* context)
{
	bool cond=asAtomHandler::Boolean_concrete(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1));
	LOG_CALL("ifTrue_l (" << ((cond)?"taken)":"not taken)"));
	if(cond)
		context->exec_pos += context->exec_pos->arg3_int;
	else
		++(context->exec_pos);
}
void ABCVm::abc_iftrue_dup_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	bool cond=asAtomHandler::Boolean_concrete(*instrptr->arg1_constant);
	LOG_CALL("ifTrue_dup_c (" << ((cond)?"taken)":"not taken)"));
	if(cond)
	{
		context->exec_pos += context->exec_pos->arg3_int;
		RUNTIME_STACK_PUSH(context,*instrptr->arg1_constant);
	}
	else
		++(context->exec_pos);
}
void ABCVm::abc_iftrue_dup_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	bool cond=asAtomHandler::Boolean_concrete(CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	LOG_CALL("ifTrue_dup_l (" << ((cond)?"taken)":"not taken)"));
	if(cond)
	{
		context->exec_pos += context->exec_pos->arg3_int;
		ASATOM_INCREF(CONTEXT_GETLOCAL(context,instrptr->local_pos1));
		RUNTIME_STACK_PUSH(context,CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	}
	else
		++(context->exec_pos);
}
void ABCVm::abc_iffalse_constant(call_context* context)
{
	bool cond=!asAtomHandler::Boolean_concrete(*context->exec_pos->arg1_constant);
	LOG_CALL("ifFalse_c (" << ((cond)?"taken)":"not taken)"));
	if(cond)
		context->exec_pos += context->exec_pos->arg3_int;
	else
		++(context->exec_pos);
}
void ABCVm::abc_iffalse_local(call_context* context)
{
	bool cond=!asAtomHandler::Boolean_concrete(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1));
	LOG_CALL("ifFalse_l (" << ((cond)?"taken)":"not taken)"));
	if(cond)
		context->exec_pos += context->exec_pos->arg3_int;
	else
		++(context->exec_pos);
}
void ABCVm::abc_iffalse_dup_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	bool cond=!asAtomHandler::Boolean_concrete(*instrptr->arg1_constant);
	LOG_CALL("ifFalse_dup_c (" << ((cond)?"taken)":"not taken)"));
	if(cond)
	{
		context->exec_pos += context->exec_pos->arg3_int;
		RUNTIME_STACK_PUSH(context,*instrptr->arg1_constant);
	}
	else
		++(context->exec_pos);
}
void ABCVm::abc_iffalse_dup_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	bool cond=!asAtomHandler::Boolean_concrete(CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	LOG_CALL("ifFalse_dup_l (" << ((cond)?"taken)":"not taken)"));
	if(cond)
	{
		context->exec_pos += context->exec_pos->arg3_int;
		ASATOM_INCREF(CONTEXT_GETLOCAL(context,instrptr->local_pos1));
		RUNTIME_STACK_PUSH(context,CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	}
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifeq_constant_constant(call_context* context)
{
	bool cond=asAtomHandler::isEqual(*context->exec_pos->arg1_constant,context->worker,*context->exec_pos->arg2_constant);
	LOG_CALL("ifEq_cc (" << ((cond)?"taken)":"not taken)"));
	if(cond)
		context->exec_pos += context->exec_pos->arg3_int;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifeq_local_constant(call_context* context)
{
	bool cond=asAtomHandler::isEqual(*context->exec_pos->arg2_constant,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1));
	LOG_CALL("ifEq_lc (" << ((cond)?"taken)":"not taken)"));
	if(cond)
		context->exec_pos += context->exec_pos->arg3_int;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifeq_constant_local(call_context* context)
{
	bool cond=asAtomHandler::isEqual(*context->exec_pos->arg1_constant,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2));
	LOG_CALL("ifEq_cl (" << ((cond)?"taken)":"not taken)"));
	if(cond)
		context->exec_pos += context->exec_pos->arg3_int;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifeq_local_local(call_context* context)
{
	bool cond=asAtomHandler::isEqual(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2));
	LOG_CALL("ifEq_ll (" << ((cond)?"taken)":"not taken)"));
	if(cond)
		context->exec_pos += context->exec_pos->arg3_int;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifne_constant_constant(call_context* context)
{
	bool cond=!asAtomHandler::isEqual(*context->exec_pos->arg1_constant,context->worker,*context->exec_pos->arg2_constant);
	LOG_CALL("ifNE_cc (" << ((cond)?"taken)":"not taken)"));
	if(cond)
		context->exec_pos += context->exec_pos->arg3_int;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifne_local_constant(call_context* context)
{
	bool cond=!asAtomHandler::isEqual(*context->exec_pos->arg2_constant,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1));
	LOG_CALL("ifNE_lc (" << ((cond)?"taken)":"not taken)"));
	if(cond)
		context->exec_pos += context->exec_pos->arg3_int;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifne_constant_local(call_context* context)
{
	bool cond=!asAtomHandler::isEqual(*context->exec_pos->arg1_constant,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2));
	LOG_CALL("ifNE_cl (" << ((cond)?"taken)":"not taken)"));
	if(cond)
		context->exec_pos += context->exec_pos->arg3_int;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifne_local_local(call_context* context)
{
	bool cond=!asAtomHandler::isEqual(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2));
	LOG_CALL("ifNE_ll (" << ((cond)?"taken)":"not taken)"));
	if(cond)
		context->exec_pos += context->exec_pos->arg3_int;
	else
		++(context->exec_pos);
}

void ABCVm::abc_iflt_constant_constant(call_context* context)
{
	bool cond=asAtomHandler::isLess(*context->exec_pos->arg1_constant,context->worker,*context->exec_pos->arg2_constant) == TTRUE;
	LOG_CALL("ifLT_cc (" << ((cond)?"taken)":"not taken)"));
	if(cond)
		context->exec_pos += context->exec_pos->arg3_int;
	else
		++(context->exec_pos);
}
void ABCVm::abc_iflt_local_constant(call_context* context)
{
	bool cond=asAtomHandler::isLess(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),context->worker,*context->exec_pos->arg2_constant) == TTRUE;
	LOG_CALL("ifLT_lc (" << ((cond)?"taken)":"not taken)"));
	if(cond)
		context->exec_pos += context->exec_pos->arg3_int;
	else
		++(context->exec_pos);
}
void ABCVm::abc_iflt_constant_local(call_context* context)
{
	bool cond=asAtomHandler::isLess(*context->exec_pos->arg1_constant,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2)) == TTRUE;
	LOG_CALL("ifLT_cl (" << ((cond)?"taken)":"not taken)"));
	if(cond)
		context->exec_pos += context->exec_pos->arg3_int;
	else
		++(context->exec_pos);
}
void ABCVm::abc_iflt_local_local(call_context* context)
{
	bool cond=asAtomHandler::isLess(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2)) == TTRUE;
	LOG_CALL("ifLT_ll (" << ((cond)?"taken)":"not taken)"));
	if(cond)
		context->exec_pos += context->exec_pos->arg3_int;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifle_constant_constant(call_context* context)
{
	bool cond=asAtomHandler::isLess(*context->exec_pos->arg2_constant,context->worker,*context->exec_pos->arg1_constant) == TFALSE;
	LOG_CALL("ifLE_cc (" << ((cond)?"taken)":"not taken)"));
	if(cond)
		context->exec_pos += context->exec_pos->arg3_int;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifle_local_constant(call_context* context)
{
	bool cond=asAtomHandler::isLess(*context->exec_pos->arg2_constant,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1)) == TFALSE;
	LOG_CALL("ifLE_lc (" << ((cond)?"taken)":"not taken)"));
	if(cond)
		context->exec_pos += context->exec_pos->arg3_int;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifle_constant_local(call_context* context)
{
	bool cond=asAtomHandler::isLess(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->worker,*context->exec_pos->arg1_constant) == TFALSE;
	LOG_CALL("ifLE_cl (" << ((cond)?"taken)":"not taken)"));
	if(cond)
		context->exec_pos += context->exec_pos->arg3_int;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifle_local_local(call_context* context)
{
	bool cond=asAtomHandler::isLess(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1)) == TFALSE;
	LOG_CALL("ifLE_ll (" << ((cond)?"taken)":"not taken)"));
	if(cond)
		context->exec_pos += context->exec_pos->arg3_int;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifgt_constant_constant(call_context* context)
{
	bool cond=asAtomHandler::isLess(*context->exec_pos->arg2_constant,context->worker,*context->exec_pos->arg1_constant) == TTRUE;
	LOG_CALL("ifGT_cc (" << ((cond)?"taken)":"not taken)"));
	if(cond)
		context->exec_pos += context->exec_pos->arg3_int;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifgt_local_constant(call_context* context)
{
	bool cond=asAtomHandler::isLess(*context->exec_pos->arg2_constant,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1)) == TTRUE;
	LOG_CALL("ifGT_lc (" << ((cond)?"taken)":"not taken)"));
	if(cond)
		context->exec_pos += context->exec_pos->arg3_int;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifgt_constant_local(call_context* context)
{
	bool cond=asAtomHandler::isLess(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->worker,*context->exec_pos->arg1_constant) == TTRUE;
	LOG_CALL("ifGT_cl (" << ((cond)?"taken)":"not taken)"));
	if(cond)
		context->exec_pos += context->exec_pos->arg3_int;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifgt_local_local(call_context* context)
{
	bool cond=asAtomHandler::isLess(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1)) == TTRUE;
	LOG_CALL("ifGT_ll (" << ((cond)?"taken)":"not taken)"));
	if(cond)
		context->exec_pos += context->exec_pos->arg3_int;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifge_constant_constant(call_context* context)
{
	bool cond=asAtomHandler::isLess(*context->exec_pos->arg1_constant,context->worker,*context->exec_pos->arg2_constant) == TFALSE;
	LOG_CALL("ifGE_cc (" << ((cond)?"taken)":"not taken)"));
	if(cond)
		context->exec_pos += context->exec_pos->arg3_int;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifge_local_constant(call_context* context)
{
	bool cond=asAtomHandler::isLess(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),context->worker,*context->exec_pos->arg2_constant) == TFALSE;
	LOG_CALL("ifGE_lc (" << ((cond)?"taken)":"not taken)"));
	if(cond)
		context->exec_pos += context->exec_pos->arg3_int;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifge_constant_local(call_context* context)
{
	bool cond=asAtomHandler::isLess(*context->exec_pos->arg1_constant,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2)) == TFALSE;
	LOG_CALL("ifGE_cl (" << ((cond)?"taken)":"not taken)"));
	if(cond)
		context->exec_pos += context->exec_pos->arg3_int;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifge_local_local(call_context* context)
{
	bool cond=asAtomHandler::isLess(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2)) == TFALSE;
	LOG_CALL("ifGE_ll (" << ((cond)?"taken)":"not taken)"));
	if(cond)
		context->exec_pos += context->exec_pos->arg3_int;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifstricteq_constant_constant(call_context* context)
{
	bool cond=asAtomHandler::isEqualStrict(*context->exec_pos->arg1_constant,context->worker,*context->exec_pos->arg2_constant);
	LOG_CALL("ifstricteq_cc (" << ((cond)?"taken)":"not taken)"));
	if(cond)
		context->exec_pos += context->exec_pos->arg3_int;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifstricteq_local_constant(call_context* context)
{
	bool cond=asAtomHandler::isEqualStrict(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),context->worker,*context->exec_pos->arg2_constant);
	LOG_CALL("ifstricteq_lc (" << ((cond)?"taken)":"not taken)"));
	if(cond)
		context->exec_pos += context->exec_pos->arg3_int;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifstricteq_constant_local(call_context* context)
{
	bool cond=asAtomHandler::isEqualStrict(*context->exec_pos->arg1_constant,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2));
	LOG_CALL("ifstricteq_cl (" << ((cond)?"taken)":"not taken)"));
	if(cond)
		context->exec_pos += context->exec_pos->arg3_int;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifstricteq_local_local(call_context* context)
{
	LOG_CALL("ifstricteq_ll (" << asAtomHandler::toDebugString(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1))<<" "<<asAtomHandler::toDebugString(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2)));
	bool cond=asAtomHandler::isEqualStrict(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2));
	LOG_CALL("ifstricteq_ll (" << ((cond)?"taken)":"not taken)"));
	if(cond)
		context->exec_pos += context->exec_pos->arg3_int;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifstrictne_constant_constant(call_context* context)
{
	bool cond=!asAtomHandler::isEqualStrict(*context->exec_pos->arg1_constant,context->worker,*context->exec_pos->arg2_constant);
	LOG_CALL("ifstrictne_cc (" << ((cond)?"taken)":"not taken)"));
	if(cond)
		context->exec_pos += context->exec_pos->arg3_int;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifstrictne_local_constant(call_context* context)
{
	bool cond=!asAtomHandler::isEqualStrict(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),context->worker,*context->exec_pos->arg2_constant);
	LOG_CALL("ifstrictne_lc (" << ((cond)?"taken)":"not taken)"));
	if(cond)
		context->exec_pos += context->exec_pos->arg3_int;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifstrictne_constant_local(call_context* context)
{
	bool cond=!asAtomHandler::isEqualStrict(*context->exec_pos->arg1_constant,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2));
	LOG_CALL("ifstrictne_cl (" << ((cond)?"taken)":"not taken)"));
	if(cond)
		context->exec_pos += context->exec_pos->arg3_int;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifstrictne_local_local(call_context* context)
{
	bool cond=!asAtomHandler::isEqualStrict(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2));
	LOG_CALL("ifstrictne_ll (" << ((cond)?"taken)":"not taken)"));
	if(cond)
		context->exec_pos += context->exec_pos->arg3_int;
	else
		++(context->exec_pos);
}
void ABCVm::abc_pushcachedconstant(call_context* context)
{
	uint32_t t = context->exec_pos->arg3_uint;
	assert(t <= (uint32_t)context->mi->context->atomsCachedMaxID);
	asAtom a = context->mi->context->constantAtoms_cached[t];
	LOG_CALL("pushcachedconstant "<<t<<" "<<asAtomHandler::toDebugString(a));
	ASATOM_INCREF(a);
	RUNTIME_STACK_PUSH(context,a);
	++(context->exec_pos);
}
void ABCVm::abc_pushcachedslot(call_context* context)
{
	uint32_t t = context->exec_pos->arg3_uint;
	assert(t < context->mi->body->localconstantslots.size());
	asAtom a = *(context->localslots[context->mi->body->getReturnValuePos()+1+context->mi->body->localresultcount+t]);
	LOG_CALL("pushcachedslot "<<t<<" "<<asAtomHandler::toDebugString(a));
	ASATOM_INCREF(a);
	RUNTIME_STACK_PUSH(context,a);
	++(context->exec_pos);
}
void ABCVm::abc_getlexfromslot(call_context* context)
{
	uint32_t t = context->exec_pos->arg1_uint;

	ASObject* s = asAtomHandler::toObject(*context->locals,context->worker);
	asAtom a = s->getSlot(t);
	LOG_CALL("getlexfromslot "<<s->toDebugString()<<" "<<t);
	ASATOM_INCREF(a);
	RUNTIME_STACK_PUSH(context,a);
	++(context->exec_pos);
}
void ABCVm::abc_getlexfromslot_localresult(call_context* context)
{
	uint32_t t = context->exec_pos->arg1_uint;

	ASObject* s = asAtomHandler::toObject(*context->locals,context->worker);
	asAtom a = s->getSlot(t);
	LOG_CALL("getlexfromslot_l "<<s->toDebugString()<<" "<<t);
	ASATOM_INCREF(a);
	replacelocalresult(context,context->exec_pos->local3.pos,a);
	++(context->exec_pos);
}
void ABCVm::abc_pushScope_constant(call_context* context)
{
	//pushscope
	asAtom* t = context->exec_pos->arg1_constant;
	LOG_CALL( "pushScope_c " << asAtomHandler::toDebugString(*t) );
	assert_and_throw(context->curr_scope_stack < context->mi->body->max_scope_depth);
	if (asAtomHandler::isObject(*t))
	{
		asAtomHandler::getObjectNoCheck(*t)->incRef();
		asAtomHandler::getObjectNoCheck(*t)->addStoredMember();
	}
	context->scope_stack[context->curr_scope_stack] = *t;
	context->scope_stack_dynamic[context->curr_scope_stack] = false;
	context->curr_scope_stack++;
	++(context->exec_pos);
}
void ABCVm::abc_pushScope_local(call_context* context)
{
	//pushscope
	asAtom* t = &CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	LOG_CALL( "pushScope_l " << asAtomHandler::toDebugString(*t) );
	assert_and_throw(context->curr_scope_stack < context->mi->body->max_scope_depth);
	if (asAtomHandler::isObject(*t))
	{
		asAtomHandler::getObjectNoCheck(*t)->incRef();
		asAtomHandler::getObjectNoCheck(*t)->addStoredMember();
	}
	context->scope_stack[context->curr_scope_stack] = *t;
	context->scope_stack_dynamic[context->curr_scope_stack] = false;
	context->curr_scope_stack++;
	++(context->exec_pos);
}
void ABCVm::abc_li8_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "li8_c");
	asAtom ret=asAtomHandler::invalidAtom;
	ApplicationDomain::loadIntN<uint8_t>(context->mi->context->root->applicationDomain.getPtr(),ret,*instrptr->arg1_constant);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_li8_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "li8_l");
	asAtom ret=asAtomHandler::invalidAtom;
	ApplicationDomain::loadIntN<uint8_t>(context->mi->context->root->applicationDomain.getPtr(),ret,CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_li8_constant_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "li8_cl");
	asAtom ret=asAtomHandler::invalidAtom;
	ApplicationDomain::loadIntN<uint8_t>(context->mi->context->root->applicationDomain.getPtr(),ret,*instrptr->arg1_constant);
	replacelocalresult(context,instrptr->local3.pos,ret);
	++(context->exec_pos);
}
void ABCVm::abc_li8_local_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "li8_ll");
	asAtom oldres = CONTEXT_GETLOCAL(context,instrptr->local3.pos);
	uint32_t addr=asAtomHandler::getUInt(CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	ByteArray* dm = context->mi->context->root->applicationDomain->currentDomainMemory;
	if(USUALLY_FALSE(dm->getLength() <= addr))
	{
		createError<RangeError>(context->worker,kInvalidRangeError);
		return;
	}
	(CONTEXT_GETLOCAL(context,instrptr->local3.pos).uintval=(*(dm->getBufferNoCheck()+addr))<<3|ATOM_INTEGER);
	ASATOM_DECREF(oldres);
	++(context->exec_pos);
}
void ABCVm::abc_li8_constant_setslotnocoerce(call_context* context)
{
	asAtom ret=asAtomHandler::invalidAtom;
	ApplicationDomain::loadIntN<uint8_t>(context->mi->context->root->applicationDomain.getPtr(),ret,*context->exec_pos->arg1_constant);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	LOG_CALL("li8_cs " << t << " "<< asAtomHandler::toDebugString(ret) << " "<< asAtomHandler::toDebugString(obj));
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,ret);
	++(context->exec_pos);
}
void ABCVm::abc_li8_local_setslotnocoerce(call_context* context)
{
	asAtom ret=asAtomHandler::invalidAtom;
	ApplicationDomain::loadIntN<uint8_t>(context->mi->context->root->applicationDomain.getPtr(),ret,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1));

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	LOG_CALL("li8_ls " << t << " "<< asAtomHandler::toDebugString(ret) << " "<< asAtomHandler::toDebugString(obj));
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,ret);
	++(context->exec_pos);
}

void ABCVm::abc_li16_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "li16_c");
	asAtom ret=asAtomHandler::invalidAtom;
	ApplicationDomain::loadIntN<uint16_t>(context->mi->context->root->applicationDomain.getPtr(),ret,*instrptr->arg1_constant);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_li16_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "li16_l");
	asAtom ret=asAtomHandler::invalidAtom;
	ApplicationDomain::loadIntN<uint16_t>(context->mi->context->root->applicationDomain.getPtr(),ret,CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_li16_constant_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "li16_cl");
	asAtom ret=asAtomHandler::invalidAtom;
	ApplicationDomain::loadIntN<uint16_t>(context->mi->context->root->applicationDomain.getPtr(),ret,*instrptr->arg1_constant);
	replacelocalresult(context,instrptr->local3.pos,ret);
	++(context->exec_pos);
}
void ABCVm::abc_li16_local_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "li16_ll");
	asAtom ret=asAtomHandler::invalidAtom;
	ApplicationDomain::loadIntN<uint16_t>(context->mi->context->root->applicationDomain.getPtr(),ret,CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	replacelocalresult(context,instrptr->local3.pos,ret);
	++(context->exec_pos);
}
void ABCVm::abc_li16_constant_setslotnocoerce(call_context* context)
{
	asAtom ret=asAtomHandler::invalidAtom;
	ApplicationDomain::loadIntN<uint16_t>(context->mi->context->root->applicationDomain.getPtr(),ret,*context->exec_pos->arg1_constant);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	LOG_CALL("li16_cs " << t << " "<< asAtomHandler::toDebugString(ret) << " "<< asAtomHandler::toDebugString(obj));
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,ret);
	++(context->exec_pos);
}
void ABCVm::abc_li16_local_setslotnocoerce(call_context* context)
{
	asAtom ret=asAtomHandler::invalidAtom;
	ApplicationDomain::loadIntN<uint16_t>(context->mi->context->root->applicationDomain.getPtr(),ret,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1));

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	LOG_CALL("li16_ls " << t << " "<< asAtomHandler::toDebugString(ret) << " "<< asAtomHandler::toDebugString(obj));
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,ret);
	++(context->exec_pos);
}
void ABCVm::abc_li32_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "li32_c");
	asAtom ret=asAtomHandler::invalidAtom;
	ApplicationDomain::loadIntN<int32_t>(context->mi->context->root->applicationDomain.getPtr(),ret,*instrptr->arg1_constant);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_li32_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "li32_l");
	asAtom ret=asAtomHandler::invalidAtom;
	ApplicationDomain::loadIntN<int32_t>(context->mi->context->root->applicationDomain.getPtr(),ret,CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_li32_constant_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "li32_cl");
	asAtom ret=asAtomHandler::invalidAtom;
	ApplicationDomain::loadIntN<int32_t>(context->mi->context->root->applicationDomain.getPtr(),ret,*instrptr->arg1_constant);
	replacelocalresult(context,instrptr->local3.pos,ret);
	++(context->exec_pos);
}
void ABCVm::abc_li32_local_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "li32_ll");
	asAtom ret=asAtomHandler::invalidAtom;
	ApplicationDomain::loadIntN<int32_t>(context->mi->context->root->applicationDomain.getPtr(),ret,CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	replacelocalresult(context,instrptr->local3.pos,ret);
	++(context->exec_pos);
}
void ABCVm::abc_li32_constant_setslotnocoerce(call_context* context)
{
	asAtom ret=asAtomHandler::invalidAtom;
	ApplicationDomain::loadIntN<int32_t>(context->mi->context->root->applicationDomain.getPtr(),ret,*context->exec_pos->arg1_constant);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	LOG_CALL("li32_cs " << t << " "<< asAtomHandler::toDebugString(ret) << " "<< asAtomHandler::toDebugString(obj));
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,ret);
	++(context->exec_pos);
}
void ABCVm::abc_li32_local_setslotnocoerce(call_context* context)
{
	asAtom ret=asAtomHandler::invalidAtom;
	ApplicationDomain::loadIntN<int32_t>(context->mi->context->root->applicationDomain.getPtr(),ret,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1));

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	LOG_CALL("li32_ls " << t << " "<< asAtomHandler::toDebugString(ret) << " "<< asAtomHandler::toDebugString(obj));
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,ret);
	++(context->exec_pos);
}

void ABCVm::abc_lf32_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "lf32_c");
	asAtom ret=asAtomHandler::invalidAtom;
	ApplicationDomain::loadFloat(context->mi->context->root->applicationDomain.getPtr(),ret,*instrptr->arg1_constant);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_lf32_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "lf32_l");
	asAtom ret=asAtomHandler::invalidAtom;
	ApplicationDomain::loadFloat(context->mi->context->root->applicationDomain.getPtr(),ret,CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_lf32_constant_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "lf32_cl");
	ApplicationDomain::loadFloat(context->mi->context->root->applicationDomain.getPtr(),CONTEXT_GETLOCAL(context,instrptr->local3.pos),*instrptr->arg1_constant);
	++(context->exec_pos);
}
void ABCVm::abc_lf32_local_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "lf32_ll");
	ApplicationDomain::loadFloat(context->mi->context->root->applicationDomain.getPtr(),CONTEXT_GETLOCAL(context,instrptr->local3.pos),CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	++(context->exec_pos);
}
void ABCVm::abc_lf32_constant_setslotnocoerce(call_context* context)
{
	asAtom ret=asAtomHandler::invalidAtom;
	ApplicationDomain::loadFloat(context->mi->context->root->applicationDomain.getPtr(),ret,*context->exec_pos->arg1_constant);
	
	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	LOG_CALL("lf32_cs " << t << " "<< asAtomHandler::toDebugString(ret) << " "<< asAtomHandler::toDebugString(obj));
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,ret);
	++(context->exec_pos);
}
void ABCVm::abc_lf32_local_setslotnocoerce(call_context* context)
{
	asAtom ret=asAtomHandler::invalidAtom;
	ApplicationDomain::loadFloat(context->mi->context->root->applicationDomain.getPtr(),ret,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1));
	
	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	LOG_CALL("lf32_ls " << t << " "<< asAtomHandler::toDebugString(ret) << " "<< asAtomHandler::toDebugString(obj));
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,ret);
	++(context->exec_pos);
}
void ABCVm::abc_lf64_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "lf64_c");
	asAtom ret=asAtomHandler::invalidAtom;
	ApplicationDomain::loadDouble(context->mi->context->root->applicationDomain.getPtr(),ret,*instrptr->arg1_constant);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_lf64_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "lf64_l");
	asAtom ret=asAtomHandler::invalidAtom;
	ApplicationDomain::loadDouble(context->mi->context->root->applicationDomain.getPtr(),ret,CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_lf64_constant_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "lf64_cl");
	asAtom ret=asAtomHandler::invalidAtom;
	ApplicationDomain::loadDouble(context->mi->context->root->applicationDomain.getPtr(),ret,*instrptr->arg1_constant);
	replacelocalresult(context,instrptr->local3.pos,ret);
	++(context->exec_pos);
}
void ABCVm::abc_lf64_local_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "lf64_ll");
	asAtom ret=asAtomHandler::invalidAtom;
	ApplicationDomain::loadDouble(context->mi->context->root->applicationDomain.getPtr(),ret,CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	replacelocalresult(context,instrptr->local3.pos,ret);
	++(context->exec_pos);
}
void ABCVm::abc_lf64_constant_setslotnocoerce(call_context* context)
{
	asAtom ret=asAtomHandler::invalidAtom;
	ApplicationDomain::loadDouble(context->mi->context->root->applicationDomain.getPtr(),ret,*context->exec_pos->arg1_constant);
	
	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	LOG_CALL("lf64_cs " << t << " "<< asAtomHandler::toDebugString(ret) << " "<< asAtomHandler::toDebugString(obj));
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,ret);
	++(context->exec_pos);
}
void ABCVm::abc_lf64_local_setslotnocoerce(call_context* context)
{
	asAtom ret=asAtomHandler::invalidAtom;
	ApplicationDomain::loadDouble(context->mi->context->root->applicationDomain.getPtr(),ret,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1));
	
	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	LOG_CALL("lf64_ls " << t << " "<< asAtomHandler::toDebugString(ret) << " "<< asAtomHandler::toDebugString(obj));
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,ret);
	++(context->exec_pos);
}
void ABCVm::abc_si8_constant_constant(call_context* context)
{
	LOG_CALL( "si8_cc");
	preloadedcodedata* instrptr = context->exec_pos;
	ApplicationDomain::storeIntN<uint8_t>(context->mi->context->root->applicationDomain.getPtr(),*instrptr->arg2_constant,*instrptr->arg1_constant);
	++(context->exec_pos);
}
void ABCVm::abc_si8_local_constant(call_context* context)
{
	LOG_CALL( "si8_lc");
	preloadedcodedata* instrptr = context->exec_pos;
	ApplicationDomain::storeIntN<uint8_t>(context->mi->context->root->applicationDomain.getPtr(),*instrptr->arg2_constant,CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	++(context->exec_pos);
}
void ABCVm::abc_si8_constant_local(call_context* context)
{
	LOG_CALL( "si8_cl");
	preloadedcodedata* instrptr = context->exec_pos;
	ApplicationDomain::storeIntN<uint8_t>(context->mi->context->root->applicationDomain.getPtr(),CONTEXT_GETLOCAL(context,instrptr->local_pos2),*instrptr->arg1_constant);
	++(context->exec_pos);
}
void ABCVm::abc_si8_local_local(call_context* context)
{
	LOG_CALL( "si8_ll");
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t addr=asAtomHandler::getUInt(CONTEXT_GETLOCAL(context,instrptr->local_pos2));
	int32_t val=asAtomHandler::getInt(CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	ByteArray* dm = context->mi->context->root->applicationDomain->currentDomainMemory;
	if(USUALLY_FALSE(dm->getLength() <= addr))
	{
		createError<RangeError>(context->worker,kInvalidRangeError);
		return;
	}
	*(dm->getBufferNoCheck()+addr)=val;

	++(context->exec_pos);
}
void ABCVm::abc_si16_constant_constant(call_context* context)
{
	LOG_CALL( "si16_cc");
	preloadedcodedata* instrptr = context->exec_pos;
	ApplicationDomain::storeIntN<uint16_t>(context->mi->context->root->applicationDomain.getPtr(),*instrptr->arg2_constant,*instrptr->arg1_constant);
	++(context->exec_pos);
}
void ABCVm::abc_si16_local_constant(call_context* context)
{
	LOG_CALL( "si16_lc");
	preloadedcodedata* instrptr = context->exec_pos;
	ApplicationDomain::storeIntN<uint16_t>(context->mi->context->root->applicationDomain.getPtr(),*instrptr->arg2_constant,CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	++(context->exec_pos);
}
void ABCVm::abc_si16_constant_local(call_context* context)
{
	LOG_CALL( "si16_cl");
	preloadedcodedata* instrptr = context->exec_pos;
	ApplicationDomain::storeIntN<uint16_t>(context->mi->context->root->applicationDomain.getPtr(),CONTEXT_GETLOCAL(context,instrptr->local_pos2),*instrptr->arg1_constant);
	++(context->exec_pos);
}
void ABCVm::abc_si16_local_local(call_context* context)
{
	LOG_CALL( "si16_ll");
	preloadedcodedata* instrptr = context->exec_pos;
	ApplicationDomain::storeIntN<uint16_t>(context->mi->context->root->applicationDomain.getPtr(),CONTEXT_GETLOCAL(context,instrptr->local_pos2),CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	++(context->exec_pos);
}
void ABCVm::abc_si32_constant_constant(call_context* context)
{
	LOG_CALL( "si32_cc");
	preloadedcodedata* instrptr = context->exec_pos;
	ApplicationDomain::storeIntN<uint32_t>(context->mi->context->root->applicationDomain.getPtr(),*instrptr->arg2_constant,*instrptr->arg1_constant);
	++(context->exec_pos);
}
void ABCVm::abc_si32_local_constant(call_context* context)
{
	LOG_CALL( "si32_lc");
	preloadedcodedata* instrptr = context->exec_pos;
	ApplicationDomain::storeIntN<uint32_t>(context->mi->context->root->applicationDomain.getPtr(),*instrptr->arg2_constant,CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	++(context->exec_pos);
}
void ABCVm::abc_si32_constant_local(call_context* context)
{
	LOG_CALL( "si32_cl");
	preloadedcodedata* instrptr = context->exec_pos;
	ApplicationDomain::storeIntN<uint32_t>(context->mi->context->root->applicationDomain.getPtr(),CONTEXT_GETLOCAL(context,instrptr->local_pos2),*instrptr->arg1_constant);
	++(context->exec_pos);
}
void ABCVm::abc_si32_local_local(call_context* context)
{
	LOG_CALL( "si32_ll");
	preloadedcodedata* instrptr = context->exec_pos;
	ApplicationDomain::storeIntN<uint32_t>(context->mi->context->root->applicationDomain.getPtr(),CONTEXT_GETLOCAL(context,instrptr->local_pos2),CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	++(context->exec_pos);
}
void ABCVm::abc_sf32_constant_constant(call_context* context)
{
	LOG_CALL( "sf32_cc");
	preloadedcodedata* instrptr = context->exec_pos;
	ApplicationDomain::storeFloat(context->mi->context->root->applicationDomain.getPtr(),*instrptr->arg2_constant,*instrptr->arg1_constant);
	++(context->exec_pos);
}
void ABCVm::abc_sf32_local_constant(call_context* context)
{
	LOG_CALL( "sf32_lc");
	preloadedcodedata* instrptr = context->exec_pos;
	ApplicationDomain::storeFloat(context->mi->context->root->applicationDomain.getPtr(),*instrptr->arg2_constant,CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	++(context->exec_pos);
}
void ABCVm::abc_sf32_constant_local(call_context* context)
{
	LOG_CALL( "sf32_cl");
	preloadedcodedata* instrptr = context->exec_pos;
	ApplicationDomain::storeFloat(context->mi->context->root->applicationDomain.getPtr(),CONTEXT_GETLOCAL(context,instrptr->local_pos2),*instrptr->arg1_constant);
	++(context->exec_pos);
}
void ABCVm::abc_sf32_local_local(call_context* context)
{
	LOG_CALL( "sf32_ll");
	preloadedcodedata* instrptr = context->exec_pos;
	ApplicationDomain::storeFloat(context->mi->context->root->applicationDomain.getPtr(),CONTEXT_GETLOCAL(context,instrptr->local_pos2),CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	++(context->exec_pos);
}
void ABCVm::abc_sf64_constant_constant(call_context* context)
{
	LOG_CALL( "sf64_cc");
	preloadedcodedata* instrptr = context->exec_pos;
	ApplicationDomain::storeDouble(context->mi->context->root->applicationDomain.getPtr(),*instrptr->arg2_constant,*instrptr->arg1_constant);
	++(context->exec_pos);
}
void ABCVm::abc_sf64_local_constant(call_context* context)
{
	LOG_CALL( "sf64_lc");
	preloadedcodedata* instrptr = context->exec_pos;
	ApplicationDomain::storeDouble(context->mi->context->root->applicationDomain.getPtr(),*instrptr->arg2_constant,CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	++(context->exec_pos);
}
void ABCVm::abc_sf64_constant_local(call_context* context)
{
	LOG_CALL( "sf64_cl");
	preloadedcodedata* instrptr = context->exec_pos;
	ApplicationDomain::storeDouble(context->mi->context->root->applicationDomain.getPtr(),CONTEXT_GETLOCAL(context,instrptr->local_pos2),*instrptr->arg1_constant);
	++(context->exec_pos);
}
void ABCVm::abc_sf64_local_local(call_context* context)
{
	LOG_CALL( "sf64_ll");
	preloadedcodedata* instrptr = context->exec_pos;
	ApplicationDomain::storeDouble(context->mi->context->root->applicationDomain.getPtr(),CONTEXT_GETLOCAL(context,instrptr->local_pos2),CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	++(context->exec_pos);
}
void ABCVm::construct_noargs_intern(call_context* context,asAtom& ret,asAtom& obj)
{
	LOG_CALL("Constructing");

	switch(asAtomHandler::getObjectType(obj))
	{
		case T_CLASS:
		{
			Class_base* o_class=asAtomHandler::as<Class_base>(obj);
			o_class->getInstance(context->worker,ret,true,nullptr,0);
			break;
		}
		case T_FUNCTION:
		{
			constructFunction(ret,context, obj, nullptr,0);
			break;
		}

		default:
		{
			createError<TypeError>(context->worker,kConstructOfNonFunctionError);
		}
	}
	if (asAtomHandler::isObject(ret))
		asAtomHandler::getObject(ret)->setConstructorCallComplete();
	LOG_CALL("End of construct_noargs " << asAtomHandler::toDebugString(ret));
}
void ABCVm::abc_construct_constant(call_context* context)
{
	asAtom obj= *context->exec_pos->arg1_constant;
	LOG_CALL( "construct_noargs_c");
	asAtom ret=asAtomHandler::invalidAtom;
	construct_noargs_intern(context,ret,obj);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_construct_local(call_context* context)
{
	asAtom obj= CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	LOG_CALL( "construct_noargs_l");
	asAtom ret=asAtomHandler::invalidAtom;
	construct_noargs_intern(context,ret,obj);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_construct_constant_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj= *context->exec_pos->arg1_constant;
	LOG_CALL( "construct_noargs_c_lr");
	asAtom res=asAtomHandler::invalidAtom;
	construct_noargs_intern(context,res,obj);
	replacelocalresult(context,instrptr->local3.pos,res);
	++(context->exec_pos);
}
void ABCVm::abc_construct_local_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj= CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	LOG_CALL( "construct_noargs_l_lr ");
	asAtom res=asAtomHandler::invalidAtom;
	construct_noargs_intern(context,res,obj);
	replacelocalresult(context,instrptr->local3.pos,res);
	++(context->exec_pos);
}
FORCE_INLINE void callprop_intern(call_context* context,asAtom& ret,asAtom& obj,asAtom* args, uint32_t argsnum,multiname* name,preloadedcodedata* cacheptr,bool refcounted, bool needreturn, bool coercearguments)
{
	assert(context->worker==getWorker());
	if ((cacheptr->local2.flags&ABC_OP_CACHED) == ABC_OP_CACHED)
	{
		if (asAtomHandler::isObject(obj) &&
				((asAtomHandler::is<Class_base>(obj) && asAtomHandler::getObjectNoCheck(obj) == cacheptr->cacheobj1)
				|| asAtomHandler::getObjectNoCheck(obj)->getClass() == cacheptr->cacheobj1))
		{
			asAtom o = asAtomHandler::fromObjectNoPrimitive(cacheptr->cacheobj3);
			LOG_CALL( "callProperty from cache:"<<*name<<" "<<asAtomHandler::toDebugString(obj)<<" "<<asAtomHandler::toDebugString(o)<<" "<<coercearguments);
			if(asAtomHandler::is<IFunction>(o))
				asAtomHandler::callFunction(o,context->worker,ret,obj,args,argsnum,refcounted,needreturn && coercearguments,coercearguments);
			else if(asAtomHandler::is<Class_base>(o))
			{
				asAtomHandler::as<Class_base>(o)->generator(context->worker,ret,args,argsnum);
				if (refcounted)
				{
					for(uint32_t i=0;i<argsnum;i++)
						ASATOM_DECREF(args[i]);
					ASATOM_DECREF(obj);
				}
			}
			else if(asAtomHandler::is<RegExp>(o))
				RegExp::exec(ret,context->worker,o,args,argsnum);
			else
			{
				LOG(LOG_ERROR,"trying to call an object as a function:"<<asAtomHandler::toDebugString(o) <<" on "<<asAtomHandler::toDebugString(obj));
				createError<TypeError>(context->worker,kCallOfNonFunctionError, "Object");
			}
			if (needreturn && asAtomHandler::isInvalid(ret))
				ret = asAtomHandler::undefinedAtom;
			LOG_CALL("End of calling cached property "<<*name<<" "<<asAtomHandler::toDebugString(ret));
			return;
		}
		else
		{
			if (cacheptr->cacheobj3 && cacheptr->cacheobj3->is<Function>() && cacheptr->cacheobj3->as<IFunction>()->clonedFrom)
				cacheptr->cacheobj3->decRef();
			cacheptr->local2.flags |= ABC_OP_NOTCACHEABLE;
			cacheptr->local2.flags &= ~ABC_OP_CACHED;
		}
	}
	if(asAtomHandler::is<Null>(obj))
	{
		LOG(LOG_ERROR,"trying to call property on null:"<<*name);
		createError<TypeError>(context->worker,kConvertNullToObjectError);
		return;
		
	}
	if (asAtomHandler::is<Undefined>(obj))
	{
		LOG(LOG_ERROR,"trying to call property on undefined2:"<<*name);
		createError<TypeError>(context->worker,kConvertUndefinedToObjectError);
		return;
	}
	ASObject* pobj = asAtomHandler::getObject(obj);
	asAtom o=asAtomHandler::invalidAtom;
	bool canCache = false;
	if (!pobj)
	{
		// fast path for primitives to avoid creation of ASObjects
		asAtomHandler::getVariableByMultiname(obj,o,context->sys,*name,context->worker);
		canCache = asAtomHandler::isValid(o);
	}
	if(asAtomHandler::isInvalid(o))
	{
		pobj = asAtomHandler::toObject(obj,context->worker);
		//We should skip the special implementation of get
		GET_VARIABLE_RESULT varres = pobj->getVariableByMultiname(o,*name, GET_VARIABLE_OPTION(SKIP_IMPL),context->worker);
		canCache = varres & GET_VARIABLE_RESULT::GETVAR_CACHEABLE;
	}
	name->resetNameIfObject();
	if(asAtomHandler::isInvalid(o) && asAtomHandler::is<Class_base>(obj))
	{
		// check super classes
		_NR<Class_base> tmpcls = asAtomHandler::as<Class_base>(obj)->super;
		while (tmpcls && !tmpcls.isNull())
		{
			tmpcls->getVariableByMultiname(o,*name, GET_VARIABLE_OPTION(SKIP_IMPL),context->worker);
			if(asAtomHandler::isValid(o))
			{
				canCache = true;
				break;
			}
			tmpcls = tmpcls->super;
		}
	}
	if(asAtomHandler::isValid(o) && !asAtomHandler::is<Proxy>(obj))
	{
		if(asAtomHandler::is<IFunction>(o))
		{
			if (canCache
					&& (cacheptr->local2.flags & ABC_OP_NOTCACHEABLE)==0
					&& asAtomHandler::canCacheMethod(obj,name)
					&& asAtomHandler::isObject(o)
					&& !asAtomHandler::as<IFunction>(o)->clonedFrom
					&& ((asAtomHandler::is<Class_base>(obj) && asAtomHandler::as<IFunction>(o)->inClass == asAtomHandler::as<Class_base>(obj)) 
						|| (asAtomHandler::as<IFunction>(o)->inClass && asAtomHandler::getClass(obj,context->sys)->isSubClass(asAtomHandler::as<IFunction>(o)->inClass))))
			{
				// cache method if multiname is static and it is a method of a sealed class
				cacheptr->local2.flags |= ABC_OP_CACHED;
				if (argsnum==2 && asAtomHandler::is<SyntheticFunction>(o) && cacheptr->cacheobj1 && cacheptr->cacheobj3) // special case 2 parameters with known parameter types: check if coercion can be skipped
				{
					SyntheticFunction* f = asAtomHandler::as<SyntheticFunction>(o);
					if (!f->getMethodInfo()->returnType)
						f->checkParamTypes();
					if (f->getMethodInfo()->paramTypes.size()==2 &&
						f->canSkipCoercion(0,(Class_base*)cacheptr->cacheobj1) &&
						f->canSkipCoercion(1,(Class_base*)cacheptr->cacheobj3))
					{
						cacheptr->local2.flags |= ABC_OP_COERCED;
					}
				}
				cacheptr->cacheobj1 = asAtomHandler::getClass(obj,context->sys);
				cacheptr->cacheobj3 = asAtomHandler::getObjectNoCheck(o);
				LOG_CALL("caching callproperty:"<<*name<<" "<<cacheptr->cacheobj1->toDebugString()<<" "<<cacheptr->cacheobj3->toDebugString());
			}
			else
			{
				cacheptr->local2.flags |= ABC_OP_NOTCACHEABLE;
				cacheptr->local2.flags &= ~ABC_OP_CACHED;
			}
			asAtom closure = asAtomHandler::getClosureAtom(o,obj);
			if (refcounted && closure.uintval != obj.uintval && !(cacheptr->local2.flags & ABC_OP_CACHED) && asAtomHandler::as<IFunction>(o)->clonedFrom)
				ASATOM_INCREF(closure);
			asAtomHandler::callFunction(o,context->worker,ret,closure,args,argsnum,refcounted,needreturn && coercearguments,coercearguments);
			if (needreturn && asAtomHandler::isInvalid(ret))
				ret = asAtomHandler::undefinedAtom;
		}
		else if(asAtomHandler::is<Class_base>(o))
		{
			asAtomHandler::as<Class_base>(o)->generator(context->worker,ret,args,argsnum);
			if (refcounted)
			{
				for(uint32_t i=0;i<argsnum;i++)
					ASATOM_DECREF(args[i]);
				ASATOM_DECREF(obj);
			}
		}
		else if(asAtomHandler::is<RegExp>(o))
			RegExp::exec(ret,context->worker,o,args,argsnum);
		else
		{
			LOG(LOG_ERROR,"trying to call an object as a function:"<<asAtomHandler::toDebugString(o) <<" on "<<asAtomHandler::toDebugString(obj));
			createError<TypeError>(context->worker,kCallOfNonFunctionError, "Object");
			return;
		}
	}
	else
	{
		//If the object is a Proxy subclass, try to invoke callProperty
		if(asAtomHandler::is<Proxy>(obj))
		{
			//Check if there is a custom caller defined, skipping implementation to avoid recursive calls
			multiname callPropertyName(nullptr);
			callPropertyName.name_type=multiname::NAME_STRING;
			callPropertyName.name_s_id=context->sys->getUniqueStringId("callProperty");
			callPropertyName.ns.emplace_back(context->sys,flash_proxy,NAMESPACE);
			asAtom oproxy=asAtomHandler::invalidAtom;
			pobj->getVariableByMultiname(oproxy,callPropertyName,SKIP_IMPL,context->worker);
			if(asAtomHandler::isValid(oproxy))
			{
				assert_and_throw(asAtomHandler::isFunction(oproxy));
				if(asAtomHandler::isValid(o))
				{
					if(asAtomHandler::is<IFunction>(o))
						asAtomHandler::callFunction(o,context->worker,ret,obj,args,argsnum,refcounted,needreturn && coercearguments,coercearguments);
					else if(asAtomHandler::is<Class_base>(o))
					{
						asAtomHandler::as<Class_base>(o)->generator(context->worker,ret,args,argsnum);
						if (refcounted)
						{
							for(uint32_t i=0;i<argsnum;i++)
								ASATOM_DECREF(args[i]);
							ASATOM_DECREF(obj);
						}
					}
					else if(asAtomHandler::is<RegExp>(o))
						RegExp::exec(ret,context->worker,o,args,argsnum);
					else
					{
						LOG(LOG_ERROR,"trying to call an object as a function:"<<asAtomHandler::toDebugString(o) <<" on "<<asAtomHandler::toDebugString(obj));
						createError<TypeError>(context->worker,kCallOfNonFunctionError, "Object");
						return;
					}
				}
				else
				{
					//Create a new array
					asAtom* proxyArgs=g_newa(asAtom,argsnum+1);
					ASObject* namearg = abstract_s(context->worker,name->normalizedName(context->sys));
					namearg->setProxyProperty(*name);
					proxyArgs[0]=asAtomHandler::fromObject(namearg);
					for(uint32_t i=0;i<argsnum;i++)
						proxyArgs[i+1]=args[i];

					//We now suppress special handling
					LOG_CALL("Proxy::callProperty");
					ASATOM_INCREF(oproxy);
					asAtomHandler::callFunction(oproxy,context->worker,ret,obj,proxyArgs,argsnum+1,true,needreturn && coercearguments,coercearguments);
					ASATOM_DECREF(oproxy);
				}
				LOG_CALL("End of calling proxy custom caller " << *name);
				ASATOM_DECREF(oproxy);
			}
			else if(asAtomHandler::isValid(o))
			{
				if(asAtomHandler::is<IFunction>(o))
					asAtomHandler::callFunction(o,context->worker,ret,obj,args,argsnum,refcounted,needreturn && coercearguments,coercearguments);
				else if(asAtomHandler::is<Class_base>(o))
				{
					asAtomHandler::as<Class_base>(o)->generator(context->worker,ret,args,argsnum);
					if (refcounted)
					{
						for(uint32_t i=0;i<argsnum;i++)
							ASATOM_DECREF(args[i]);
						ASATOM_DECREF(obj);
					}
				}
				else if(asAtomHandler::is<RegExp>(o))
					RegExp::exec(ret,context->worker,o,args,argsnum);
				else
				{
					LOG(LOG_ERROR,"trying to call an object as a function:"<<asAtomHandler::toDebugString(o) <<" on "<<asAtomHandler::toDebugString(obj));
					createError<TypeError>(context->worker,kCallOfNonFunctionError, "Object");
					return;
				}
				LOG_CALL("End of calling proxy " << *name);
			}
		}
		if (asAtomHandler::isInvalid(ret))
		{
			if (pobj->hasPropertyByMultiname(*name,true,true,context->worker))
			{
				tiny_string clsname = pobj->getClass()->getQualifiedClassName();
				createError<ReferenceError>(context->worker,kWriteOnlyError, name->normalizedName(context->sys), clsname);
			}
			if (pobj->getClass() && pobj->getClass()->isSealed)
			{
				tiny_string clsname = pobj->getClass()->getQualifiedClassName();
				createError<ReferenceError>(context->worker,kReadSealedError, name->normalizedName(context->sys), clsname);
			}
			if (asAtomHandler::is<Class_base>(obj))
			{
				tiny_string clsname = asAtomHandler::as<Class_base>(obj)->class_name.getQualifiedName(context->sys);
				createError<TypeError>(context->worker,kCallOfNonFunctionError, name->qualifiedString(context->sys), clsname);
			}
			else
			{
				tiny_string clsname = pobj->getClassName();
				createError<TypeError>(context->worker,kCallNotFoundError, name->qualifiedString(context->sys), clsname);
			}
			asAtomHandler::setUndefined(ret);
		}
	}

	ASATOM_DECREF(o);
	LOG_CALL("End of calling " << *name<<" "<<asAtomHandler::toDebugString(ret));
}
void ABCVm::abc_callpropertyStaticName(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t argcount = context->exec_pos->local2.pos;
	multiname* name=(++context->exec_pos)->cachedmultiname2;
	LOG_CALL( "callProperty_staticname " << *name<<" "<<argcount);
	RUNTIME_STACK_POP_N_CREATE(context,argcount+1,args);
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern(context,ret,*args,args+1,argcount,name,instrptr,true,true,(instrptr->local2.flags&ABC_OP_COERCED)==0);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_callpropertyStaticName_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));

	RUNTIME_STACK_POP_CREATE(context,args);
	multiname* name=instrptr->cachedmultiname2;

	LOG_CALL( "callProperty_l " << *name);
	RUNTIME_STACK_POP_CREATE(context,obj);
	asAtom res=asAtomHandler::invalidAtom;
	callprop_intern(context,res,*obj,args,1,name,context->exec_pos,true,true,(instrptr->local2.flags&ABC_OP_COERCED)==0);
	replacelocalresult(context,instrptr->local3.pos,res);
	++(context->exec_pos);
}
void ABCVm::abc_callpropertyStaticNameCached(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t argcount = instrptr->local2.pos;
	asAtom* args = g_newa(asAtom,argcount);
	multiname* name=(context->exec_pos+1)->cachedmultiname3;
	LOG_CALL( "callProperty_staticnameCached " << *name<<" "<<argcount);
	for (uint32_t i = argcount; i > 0 ; i--)
	{
		(++(context->exec_pos));
		if (context->exec_pos->arg2_uint == OPERANDTYPES::OP_LOCAL || context->exec_pos->arg2_uint == OPERANDTYPES::OP_CACHED_SLOT)
			args[i-1] = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
		else
			args[i-1] = *context->exec_pos->arg1_constant;
	}
	RUNTIME_STACK_POP_CREATE(context,obj);
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern(context,ret,*obj,args,argcount,name,instrptr,false,true,(instrptr->local2.flags&ABC_OP_COERCED)==0);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_callpropertyStaticNameCached_localResult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t argcount = instrptr->local2.pos;
	asAtom* args = g_newa(asAtom,argcount);
	multiname* name=(context->exec_pos+1)->cachedmultiname3;
	LOG_CALL( "callProperty_staticnameCached_lr " << *name<<" "<<argcount);
	for (uint32_t i = argcount; i > 0 ; i--)
	{
		(++(context->exec_pos));
		if (context->exec_pos->arg2_uint == OPERANDTYPES::OP_LOCAL || context->exec_pos->arg2_uint == OPERANDTYPES::OP_CACHED_SLOT)
			args[i-1] = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
		else
			args[i-1] = *context->exec_pos->arg1_constant;
	}
	RUNTIME_STACK_POP_CREATE(context,obj);
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern(context,ret,*obj,args,argcount,name,instrptr,false,true,(instrptr->local2.flags&ABC_OP_COERCED)==0);
	replacelocalresult(context,context->exec_pos->local3.pos,ret);
	++(context->exec_pos);
}
void ABCVm::abc_callpropertyStaticNameCached_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t argcount = instrptr->local2.pos;
	asAtom* args = g_newa(asAtom,argcount);
	assert(argcount>1);
	multiname* name=(context->exec_pos+1)->cachedmultiname3;
	LOG_CALL( "callProperty_staticnameCached_c " << *name<<" "<<argcount);
	for (uint32_t i = argcount; i > 0 ; i--)
	{
		(++(context->exec_pos));
		if (context->exec_pos->arg2_uint == OPERANDTYPES::OP_LOCAL || context->exec_pos->arg2_uint == OPERANDTYPES::OP_CACHED_SLOT)
			args[i-1] = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
		else
			args[i-1] = *context->exec_pos->arg1_constant;
	}
	asAtom obj = *(++context->exec_pos)->arg2_constant;
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern(context,ret,obj,args,argcount,name,instrptr,false,true,(instrptr->local2.flags&ABC_OP_COERCED)==0);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_callpropertyStaticNameCached_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t argcount = instrptr->local2.pos;
	asAtom* args = g_newa(asAtom,argcount);
	assert(argcount>1);
	multiname* name=(context->exec_pos+1)->cachedmultiname3;
	LOG_CALL( "callProperty_staticnameCached_l " << *name<<" "<<argcount);
	for (uint32_t i = argcount; i > 0 ; i--)
	{
		(++(context->exec_pos));
		if (context->exec_pos->arg2_uint == OPERANDTYPES::OP_LOCAL || context->exec_pos->arg2_uint == OPERANDTYPES::OP_CACHED_SLOT)
			args[i-1] = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
		else
			args[i-1] = *context->exec_pos->arg1_constant;
	}
	asAtom obj = CONTEXT_GETLOCAL(context,(++context->exec_pos)->local_pos2);
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern(context,ret,obj,args,argcount,name,instrptr,false,true,(instrptr->local2.flags&ABC_OP_COERCED)==0);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_callpropertyStaticNameCached_constant_localResult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t argcount = instrptr->local2.pos;
	asAtom* args = g_newa(asAtom,argcount);
	assert(argcount>1);
	multiname* name=(context->exec_pos+1)->cachedmultiname3;
	LOG_CALL( "callProperty_staticnameCached_cl " << *name<<" "<<argcount);
	for (uint32_t i = argcount; i > 0 ; i--)
	{
		(++(context->exec_pos));
		if (context->exec_pos->arg2_uint == OPERANDTYPES::OP_LOCAL || context->exec_pos->arg2_uint == OPERANDTYPES::OP_CACHED_SLOT)
			args[i-1] = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
		else
			args[i-1] = *context->exec_pos->arg1_constant;
	}
	asAtom obj = *(++context->exec_pos)->arg2_constant;
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern(context,ret,obj,args,argcount,name,instrptr,false,true,(instrptr->local2.flags&ABC_OP_COERCED)==0);
	replacelocalresult(context,context->exec_pos->local3.pos,ret);
	++(context->exec_pos);
}
void ABCVm::abc_callpropertyStaticNameCached_local_localResult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t argcount = instrptr->local2.pos;
	asAtom* args = g_newa(asAtom,argcount);
	assert(argcount>1);
	multiname* name=(context->exec_pos+1)->cachedmultiname3;
	LOG_CALL( "callProperty_staticnameCached_ll " << *name<<" "<<argcount);
	for (uint32_t i = argcount; i > 0 ; i--)
	{
		(++(context->exec_pos));
		if (context->exec_pos->arg2_uint == OPERANDTYPES::OP_LOCAL || context->exec_pos->arg2_uint == OPERANDTYPES::OP_CACHED_SLOT)
			args[i-1] = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
		else
			args[i-1] = *context->exec_pos->arg1_constant;
	}
	asAtom obj = CONTEXT_GETLOCAL(context,(++context->exec_pos)->local_pos2);
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern(context,ret,obj,args,argcount,name,instrptr,false,true,(instrptr->local2.flags&ABC_OP_COERCED)==0);
	replacelocalresult(context,context->exec_pos->local3.pos,ret);
	++(context->exec_pos);
}
void ABCVm::abc_callpropertyStaticName_constant_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	multiname* name=(++(context->exec_pos))->cachedmultiname2;
	bool coerce = ((++(context->exec_pos))->local2.flags&ABC_OP_COERCED)==0;

	asAtom args=*instrptr->arg2_constant;
	asAtom obj= *instrptr->arg1_constant;
	LOG_CALL( "callProperty_cc " << *name);
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern(context,ret,obj,&args,1,name,context->exec_pos,false,true,coerce);
	RUNTIME_STACK_PUSH(context,ret);

	++(context->exec_pos);
}
void ABCVm::abc_callpropertyStaticName_local_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	multiname* name=(++(context->exec_pos))->cachedmultiname2;
	bool coerce = ((++(context->exec_pos))->local2.flags&ABC_OP_COERCED)==0;

	asAtom args=*instrptr->arg2_constant;

	asAtom obj= CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	LOG_CALL( "callProperty_lc " << *name);
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern(context,ret,obj,&args,1,name,context->exec_pos,false,true,coerce);
	RUNTIME_STACK_PUSH(context,ret);

	++(context->exec_pos);
}
void ABCVm::abc_callpropertyStaticName_constant_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	multiname* name=(++(context->exec_pos))->cachedmultiname2;
	bool coerce = ((++(context->exec_pos))->local2.flags&ABC_OP_COERCED)==0;

	asAtom* args=&CONTEXT_GETLOCAL(context,instrptr->local_pos2);

	asAtom obj= *instrptr->arg1_constant;
	LOG_CALL( "callProperty_cl " << *name);
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern(context,ret,obj,args,1,name,context->exec_pos,false,true,coerce);
	RUNTIME_STACK_PUSH(context,ret);

	++(context->exec_pos);
}
void ABCVm::abc_callpropertyStaticName_local_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	multiname* name=(++(context->exec_pos))->cachedmultiname2;
	bool coerce = ((++(context->exec_pos))->local2.flags&ABC_OP_COERCED)==0;

	asAtom* args=&CONTEXT_GETLOCAL(context,instrptr->local_pos2);

	asAtom obj= CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	LOG_CALL( "callProperty_ll " << *name<<" "<<instrptr->local_pos1<<" "<<instrptr->local_pos2<<" "<<asAtomHandler::toDebugString(obj)<<" "<<asAtomHandler::toDebugString(*args));
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern(context,ret,obj,args,1,name,context->exec_pos,false,true,coerce);
	RUNTIME_STACK_PUSH(context,ret);

	++(context->exec_pos);
}
void ABCVm::abc_callpropertyStaticName_constant_constant_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	multiname* name=(++(context->exec_pos))->cachedmultiname2;
	bool coerce = ((++(context->exec_pos))->local2.flags&ABC_OP_COERCED)==0;

	asAtom args=*instrptr->arg2_constant;

	asAtom obj= *instrptr->arg1_constant;
	LOG_CALL( "callProperty_ccl " << *name);
	asAtom res=asAtomHandler::invalidAtom;
	callprop_intern(context,res,obj,&args,1,name,context->exec_pos,false,true,coerce);
	replacelocalresult(context,instrptr->local3.pos,res);
	++(context->exec_pos);
}
void ABCVm::abc_callpropertyStaticName_local_constant_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	multiname* name=(++(context->exec_pos))->cachedmultiname2;
	bool coerce = ((++(context->exec_pos))->local2.flags&ABC_OP_COERCED)==0;

	asAtom args=*instrptr->arg2_constant;

	asAtom obj= CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	LOG_CALL( "callProperty_lcl " << *name);
	asAtom res=asAtomHandler::invalidAtom;
	callprop_intern(context,res,obj,&args,1,name,context->exec_pos,false,true,coerce);
	replacelocalresult(context,instrptr->local3.pos,res);
	++(context->exec_pos);
}
void ABCVm::abc_callpropertyStaticName_constant_local_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	multiname* name=(++(context->exec_pos))->cachedmultiname2;
	bool coerce = ((++(context->exec_pos))->local2.flags&ABC_OP_COERCED)==0;

	asAtom* args=&CONTEXT_GETLOCAL(context,instrptr->local_pos2);

	asAtom obj= *instrptr->arg1_constant;
	LOG_CALL( "callProperty_cll " << *name);
	asAtom res=asAtomHandler::invalidAtom;
	callprop_intern(context,res,obj,args,1,name,context->exec_pos,false,true,coerce);
	replacelocalresult(context,instrptr->local3.pos,res);
	++(context->exec_pos);
}
void ABCVm::abc_callpropertyStaticName_local_local_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	multiname* name=(++(context->exec_pos))->cachedmultiname2;
	bool coerce = ((++(context->exec_pos))->local2.flags&ABC_OP_COERCED)==0;

	asAtom* args=&CONTEXT_GETLOCAL(context,instrptr->local_pos2);

	asAtom obj= CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	LOG_CALL( "callProperty_lll " << *name);
	asAtom res=asAtomHandler::invalidAtom;
	callprop_intern(context,res,obj,args,1,name,context->exec_pos,false,true,coerce);
	replacelocalresult(context,instrptr->local3.pos,res);
	++(context->exec_pos);
}
void ABCVm::abc_callpropertyStaticName_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	multiname* name=context->exec_pos->cachedmultiname2;
	(++(context->exec_pos));


	asAtom obj= *instrptr->arg1_constant;
	LOG_CALL( "callProperty_noargs_c " << *name);
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern(context,ret,obj,nullptr,0,name,context->exec_pos,false,true,false);
	RUNTIME_STACK_PUSH(context,ret);

	++(context->exec_pos);
}
void ABCVm::abc_callpropertyStaticName_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	multiname* name=context->exec_pos->cachedmultiname2;
	(++(context->exec_pos));


	asAtom obj= CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	LOG_CALL( "callProperty_noargs_l " << *name);
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern(context,ret,obj,nullptr,0,name,context->exec_pos,false,true,false);
	RUNTIME_STACK_PUSH(context,ret);

	++(context->exec_pos);
}
void ABCVm::abc_callpropertyStaticName_constant_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	multiname* name=context->exec_pos->cachedmultiname2;
	(++(context->exec_pos));


	asAtom obj= *instrptr->arg1_constant;
	LOG_CALL( "callProperty_noargs_c_lr " << *name);
	asAtom res=asAtomHandler::invalidAtom;
	callprop_intern(context,res,obj,nullptr,0,name,context->exec_pos,false,true,false);
	replacelocalresult(context,instrptr->local3.pos,res);

	++(context->exec_pos);
}
void ABCVm::abc_callpropertyStaticName_local_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	multiname* name=context->exec_pos->cachedmultiname2;
	(++(context->exec_pos));

	asAtom obj= CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	LOG_CALL( "callProperty_noargs_l_lr " << *name);
	asAtom res=asAtomHandler::invalidAtom;
	callprop_intern(context,res,obj,nullptr,0,name,context->exec_pos,false,true,false);
	replacelocalresult(context,instrptr->local3.pos,res);

	++(context->exec_pos);
}
void ABCVm::abc_returnvalue_constant(call_context* context)
{
	asAtomHandler::set(context->locals[context->mi->body->getReturnValuePos()],*context->exec_pos->arg1_constant);
	LOG_CALL("returnValue_c " << asAtomHandler::toDebugString(context->locals[context->mi->body->getReturnValuePos()]));
	++(context->exec_pos);
}
void ABCVm::abc_returnvalue_local(call_context* context)
{
	asAtomHandler::set(context->locals[context->mi->body->getReturnValuePos()],CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1));
	LOG_CALL("returnValue_l " << asAtomHandler::toDebugString(context->locals[context->mi->body->getReturnValuePos()]));
	ASATOM_INCREF(context->locals[context->mi->body->getReturnValuePos()]);
	++(context->exec_pos);
}
void ABCVm::abc_constructsuper_constant(call_context* context)
{
	LOG_CALL( "constructSuper_c ");
	asAtom obj=*context->exec_pos->arg1_constant;
	context->inClass->super->handleConstruction(obj,nullptr, 0, false);
	LOG_CALL("End super construct "<<asAtomHandler::toDebugString(obj));
	++(context->exec_pos);
}
void ABCVm::abc_constructsuper_local(call_context* context)
{
	LOG_CALL( "constructSuper_l ");
	asAtom obj= CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	context->inClass->super->handleConstruction(obj,nullptr, 0, false);
	LOG_CALL("End super construct "<<asAtomHandler::toDebugString(obj));
	++(context->exec_pos);
}
void ABCVm::constructpropnoargs_intern(call_context* context,asAtom& ret,asAtom& obj,multiname* name, ASObject* constructor)
{
	asAtom o=asAtomHandler::invalidAtom;
	if (constructor)
		o = asAtomHandler::fromObjectNoPrimitive(constructor);
	else
		asAtomHandler::toObject(obj,context->worker)->getVariableByMultiname(o,*name,GET_VARIABLE_OPTION::NONE, context->worker);

	if(asAtomHandler::isInvalid(o))
	{
		if (asAtomHandler::is<Undefined>(obj))
			createError<TypeError>(context->worker,kConvertUndefinedToObjectError);
		else if (asAtomHandler::isPrimitive(obj))
			createError<TypeError>(context->worker,kConstructOfNonFunctionError);
		else
			createError<ReferenceError>(context->worker,kUndefinedVarError, name->normalizedNameUnresolved(context->sys));
		ASATOM_DECREF(obj);
		return;
	}
	try
	{
		if(asAtomHandler::isClass(o))
		{
			Class_base* o_class=asAtomHandler::as<Class_base>(o);
			o_class->getInstance(context->worker,ret,true,nullptr,0);
		}
		else if(asAtomHandler::isFunction(o))
		{
			constructFunction(ret,context, o, nullptr, 0);
		}
		else if (asAtomHandler::isTemplate(o))
		{
			createError<TypeError>(context->worker,kConstructOfNonFunctionError);
			return;
		}
		else
		{
			createError<TypeError>(context->worker,kNotConstructorError);
			return;
		}
	}
	catch(ASObject* exc)
	{
		LOG_CALL("Exception during object construction. Returning Undefined");
		//Handle eventual exceptions from the constructor, to fix the stack
		RUNTIME_STACK_PUSH(context,obj);
		throw;
	}
	ASATOM_DECREF(o);
	if (asAtomHandler::isObject(ret))
		asAtomHandler::getObjectNoCheck(ret)->setConstructorCallComplete();
	LOG_CALL("End of constructing " << asAtomHandler::toDebugString(ret));
}
void ABCVm::abc_constructpropStaticName_constant(call_context* context)
{
	multiname* name=context->exec_pos->cachedmultiname2;
	asAtom obj= *context->exec_pos->arg1_constant;
	LOG_CALL( "constructprop_noargs_c "<<*name);
	++(context->exec_pos);
	asAtom ret=asAtomHandler::invalidAtom;
	constructpropnoargs_intern(context,ret,obj,name,context->exec_pos->cacheobj2);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_constructpropStaticName_local(call_context* context)
{
	multiname* name=context->exec_pos->cachedmultiname2;
	asAtom obj= CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	++(context->exec_pos);
	LOG_CALL( "constructprop_noargs_l " << *name);
	asAtom ret=asAtomHandler::invalidAtom;
	constructpropnoargs_intern(context,ret,obj,name,nullptr);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_constructpropStaticName_constant_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	multiname* name=context->exec_pos->cachedmultiname2;
	asAtom obj= *context->exec_pos->arg1_constant;
	++(context->exec_pos);
	LOG_CALL( "constructprop_noargs_c_lr " << *name);
	asAtom res=asAtomHandler::invalidAtom;
	constructpropnoargs_intern(context,res,obj,name,context->exec_pos->cacheobj2);
	replacelocalresult(context,instrptr->local3.pos,res);
	++(context->exec_pos);
}
void ABCVm::abc_constructpropStaticName_local_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	multiname* name=context->exec_pos->cachedmultiname2;
	asAtom obj= CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	++(context->exec_pos);
	LOG_CALL( "constructprop_noargs_l_lr " << *name);
	asAtom res=asAtomHandler::invalidAtom;
	constructpropnoargs_intern(context,res,obj,name,nullptr);
	replacelocalresult(context,instrptr->local3.pos,res);
	++(context->exec_pos);
}
void ABCVm::abc_callpropvoidStaticName(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t argcount = instrptr->local2.pos;
	multiname* name=(++context->exec_pos)->cachedmultiname2;
	LOG_CALL( "callPropvoid_staticname " << *name<<" "<<argcount);
	RUNTIME_STACK_POP_N_CREATE(context,argcount+1,args);
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern(context,ret,*args,args+1,argcount,name,instrptr,true,false,(instrptr->local2.flags&ABC_OP_COERCED)==0);
	ASATOM_DECREF(ret);
	++(context->exec_pos);
}
void ABCVm::abc_callpropvoidStaticNameCached(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t argcount = instrptr->local2.pos;
	asAtom* args = g_newa(asAtom,argcount);
	multiname* name=(context->exec_pos+1)->cachedmultiname3;
	LOG_CALL( "callPropvoid_staticnameCached " << *name<<" "<<argcount);
	for (uint32_t i = argcount; i > 0 ; i--)
	{
		(++(context->exec_pos));
		if (context->exec_pos->arg2_uint == OPERANDTYPES::OP_LOCAL || context->exec_pos->arg2_uint == OPERANDTYPES::OP_CACHED_SLOT)
			args[i-1] = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
		else
			args[i-1] = *context->exec_pos->arg1_constant;
	}
	RUNTIME_STACK_POP_CREATE(context,obj);
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern(context,ret,*obj,args,argcount,name,instrptr,false,false,(instrptr->local2.flags&ABC_OP_COERCED)==0);
	ASATOM_DECREF(ret);
	ASATOM_DECREF_POINTER(obj);
	++(context->exec_pos);
}
void ABCVm::abc_callpropvoidStaticNameCached_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t argcount = instrptr->local2.pos;
	asAtom* args = g_newa(asAtom,argcount);
	multiname* name=(context->exec_pos+1)->cachedmultiname3;
	LOG_CALL( "callPropvoid_staticnameCached_c " << *name<<" "<<argcount);
	for (uint32_t i = argcount; i > 0 ; i--)
	{
		(++(context->exec_pos));
		if (context->exec_pos->arg2_uint == OPERANDTYPES::OP_LOCAL || context->exec_pos->arg2_uint == OPERANDTYPES::OP_CACHED_SLOT)
			args[i-1] = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
		else
			args[i-1] = *context->exec_pos->arg1_constant;
	}
	asAtom obj = *(++context->exec_pos)->arg2_constant;
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern(context,ret,obj,args,argcount,name,instrptr,false,false,(instrptr->local2.flags&ABC_OP_COERCED)==0);
	ASATOM_DECREF(ret);
	++(context->exec_pos);
}
void ABCVm::abc_callpropvoidStaticNameCached_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t argcount = instrptr->local2.pos;
	asAtom* args = g_newa(asAtom,argcount);
	multiname* name=(context->exec_pos+1)->cachedmultiname3;
	LOG_CALL( "callPropvoid_staticnameCached_l " << *name<<" "<<argcount);
	for (uint32_t i = argcount; i > 0 ; i--)
	{
		(++(context->exec_pos));
		if (context->exec_pos->arg2_uint == OPERANDTYPES::OP_LOCAL || context->exec_pos->arg2_uint == OPERANDTYPES::OP_CACHED_SLOT)
			args[i-1] = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
		else
			args[i-1] = *context->exec_pos->arg1_constant;
	}
	asAtom obj = CONTEXT_GETLOCAL(context,(++context->exec_pos)->local_pos2);
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern(context,ret,obj,args,argcount,name,instrptr,false,false,(instrptr->local2.flags&ABC_OP_COERCED)==0);
	ASATOM_DECREF(ret);
	++(context->exec_pos);
}
void ABCVm::abc_callpropvoidStaticName_constant_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));

	asAtom args=*instrptr->arg2_constant;
	multiname* name=context->exec_pos->cachedmultiname2;
	(++(context->exec_pos));

	asAtom obj= *instrptr->arg1_constant;
	LOG_CALL( "callPropvoidStaticName_cc " << *name);
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern(context,ret,obj,&args,1,name,context->exec_pos,false,false,(context->exec_pos->local2.flags&ABC_OP_COERCED)==0);
	ASATOM_DECREF(ret);

	++(context->exec_pos);
}
void ABCVm::abc_callpropvoidStaticName_local_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));

	asAtom args=*instrptr->arg2_constant;
	multiname* name=context->exec_pos->cachedmultiname2;
	(++(context->exec_pos));

	asAtom obj= CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	LOG_CALL( "callPropvoidStaticName_lc " << *name);
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern(context,ret,obj,&args,1,name,context->exec_pos,false,false,(context->exec_pos->local2.flags&ABC_OP_COERCED)==0);
	ASATOM_DECREF(ret);

	++(context->exec_pos);
}
void ABCVm::abc_callpropvoidStaticName_constant_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));

	asAtom* args=&CONTEXT_GETLOCAL(context,instrptr->local_pos2);
	multiname* name=context->exec_pos->cachedmultiname2;
	(++(context->exec_pos));

	asAtom obj= *instrptr->arg1_constant;
	LOG_CALL( "callPropvoidStaticName_cl " << *name);
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern(context,ret,obj,args,1,name,context->exec_pos,false,false,(context->exec_pos->local2.flags&ABC_OP_COERCED)==0);
	ASATOM_DECREF(ret);
	++(context->exec_pos);
}
void ABCVm::abc_callpropvoidStaticName_local_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));

	asAtom* args=&CONTEXT_GETLOCAL(context,instrptr->local_pos2);
	multiname* name=context->exec_pos->cachedmultiname2;
	(++(context->exec_pos));

	asAtom obj= CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	LOG_CALL( "callPropvoidStaticName_ll " << *name);
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern(context,ret,obj,args,1,name,context->exec_pos,false,false,(context->exec_pos->local2.flags&ABC_OP_COERCED)==0);
	ASATOM_DECREF(ret);

	++(context->exec_pos);
}
void ABCVm::abc_callpropvoidStaticName_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	multiname* name=context->exec_pos->cachedmultiname2;
	(++(context->exec_pos));

	asAtom obj= *instrptr->arg1_constant;
	LOG_CALL( "callPropvoidStaticName_noargs_c " << *name);
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern(context,ret,obj,nullptr,0,name,context->exec_pos,false,false,false);
	ASATOM_DECREF(ret);

	++(context->exec_pos);
}
void ABCVm::abc_callpropvoidStaticName_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	multiname* name=context->exec_pos->cachedmultiname2;
	(++(context->exec_pos));

	asAtom obj= CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	LOG_CALL( "callPropvoid_noargs_l " << *name<<" "<<asAtomHandler::toDebugString(obj));
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern(context,ret,obj,nullptr,0,name,context->exec_pos,false,false,false);
	ASATOM_DECREF(ret);

	++(context->exec_pos);
}
void ABCVm::abc_getlex_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	assert(instrptr->local3.pos > 0);
	if ((instrptr->local3.flags&ABC_OP_CACHED) == ABC_OP_CACHED)
	{
		asAtom oldres = CONTEXT_GETLOCAL(context,instrptr->local3.pos);
		asAtomHandler::setFunction(CONTEXT_GETLOCAL(context,instrptr->local3.pos),instrptr->cacheobj1,nullptr,context->worker);//,instrptr->cacheobj2);
		ASATOM_INCREF(CONTEXT_GETLOCAL(context,instrptr->local3.pos));
		ASATOM_DECREF(oldres);
		LOG_CALL( "getLex_l from cache: " <<  instrptr->cacheobj1->toDebugString());
	}
	else if (getLex_multiname(context,instrptr->cachedmultiname2,instrptr->local3.pos))
	{
		// put object in cache
		instrptr->local3.flags = ABC_OP_CACHED;
		instrptr->cacheobj1 = asAtomHandler::getObject(CONTEXT_GETLOCAL(context,instrptr->local3.pos));
		instrptr->cacheobj2 = asAtomHandler::getClosure(CONTEXT_GETLOCAL(context,instrptr->local3.pos));
	}
	++(context->exec_pos);
}
void ABCVm::abc_setPropertyStaticName(call_context* context)
{
	multiname* name=context->exec_pos->cachedmultiname2;
	RUNTIME_STACK_POP_CREATE(context,value);
	RUNTIME_STACK_POP_CREATE(context,obj);

	LOG_CALL("setProperty_s " << *name << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));
	if(asAtomHandler::isNull(*obj))
	{
		LOG(LOG_ERROR,"calling setProperty on null:" << *name << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));
		ASATOM_DECREF_POINTER(value);
		ASATOM_DECREF_POINTER(obj);
		createError<TypeError>(context->worker,kConvertNullToObjectError);
		return;
	}
	if (asAtomHandler::isUndefined(*obj))
	{
		LOG(LOG_ERROR,"calling setProperty on undefined:" << *name << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));
		ASATOM_DECREF_POINTER(value);
		ASATOM_DECREF_POINTER(obj);
		createError<TypeError>(context->worker,kConvertUndefinedToObjectError);
		return;
	}
	ASObject* o = asAtomHandler::toObject(*obj,context->worker);
	bool alreadyset=false;
	multiname* simplesettername = nullptr;
	if (context->exec_pos->local3.pos == 0x68)//initproperty
		simplesettername =o->setVariableByMultiname(*name,*value,ASObject::CONST_ALLOWED,&alreadyset,context->worker);
	else//Do not allow to set contant traits
		simplesettername =o->setVariableByMultiname(*name,*value,ASObject::CONST_NOT_ALLOWED,&alreadyset,context->worker);
	if (simplesettername)
		context->exec_pos->cachedmultiname2 = simplesettername;
	if (alreadyset)
		ASATOM_DECREF_POINTER(value);
	ASATOM_DECREF_POINTER(obj);
	++(context->exec_pos);
}
void ABCVm::abc_setPropertyStaticName_constant_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));
	multiname* name=context->exec_pos->cachedmultiname2;
	asAtom* obj = instrptr->arg1_constant;
	asAtom* value = instrptr->arg2_constant;

	LOG_CALL("setProperty_scc " << *name << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));

	if(asAtomHandler::isNull(*obj))
	{
		LOG(LOG_ERROR,"calling setProperty on null:" << *name << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));
		createError<TypeError>(context->worker,kConvertNullToObjectError);
		return;
	}
	if (asAtomHandler::isUndefined(*obj))
	{
		LOG(LOG_ERROR,"calling setProperty on undefined:" << *name << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));
		createError<TypeError>(context->worker,kConvertUndefinedToObjectError);
		return;
	}

	ASObject* o = asAtomHandler::toObject(*obj,context->worker);
	multiname* simplesettername = nullptr;
	if (context->exec_pos->local3.pos == 0x68)//initproperty
		simplesettername =o->setVariableByMultiname(*name,*value,ASObject::CONST_ALLOWED,nullptr,context->worker);
	else//Do not allow to set contant traits
		simplesettername =o->setVariableByMultiname(*name,*value,ASObject::CONST_NOT_ALLOWED,nullptr,context->worker);
	if (simplesettername)
		context->exec_pos->cachedmultiname2 = simplesettername;
	++(context->exec_pos);
}
void ABCVm::abc_setPropertyStaticName_local_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));
	multiname* name=context->exec_pos->cachedmultiname2;
	asAtom* obj = &CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	asAtom* value = instrptr->arg2_constant;

	LOG_CALL("setProperty_slc " << *name << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value)<<" "<<instrptr->local_pos1);

	if(asAtomHandler::isNull(*obj))
	{
		LOG(LOG_ERROR,"calling setProperty on null:" << *name << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));
		createError<TypeError>(context->worker,kConvertNullToObjectError);
		return;
	}
	if (asAtomHandler::isUndefined(*obj))
	{
		LOG(LOG_ERROR,"calling setProperty on undefined:" << *name << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));
		createError<TypeError>(context->worker,kConvertUndefinedToObjectError);
		return;
	}
	ASObject* o = asAtomHandler::toObject(*obj,context->worker);
	o->incRef(); // this is neccessary for reference counting in case of exception thrown in setVariableByMultiname
	multiname* simplesettername = nullptr;
	if (context->exec_pos->local3.pos == 0x68)//initproperty
		simplesettername =o->setVariableByMultiname(*name,*value,ASObject::CONST_ALLOWED,nullptr,context->worker);
	else//Do not allow to set contant traits
		simplesettername =o->setVariableByMultiname(*name,*value,ASObject::CONST_NOT_ALLOWED,nullptr,context->worker);
	if (simplesettername)
		context->exec_pos->cachedmultiname2 = simplesettername;
	o->decRef(); // this is neccessary for reference counting in case of exception thrown in setVariableByMultiname
	++(context->exec_pos);
}
void ABCVm::abc_setPropertyStaticName_constant_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));
	multiname* name=context->exec_pos->cachedmultiname2;
	asAtom* obj = instrptr->arg1_constant;
	asAtom* value = &CONTEXT_GETLOCAL(context,instrptr->local_pos2);

	LOG_CALL("setProperty_scl " << *name << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));

	if(asAtomHandler::isNull(*obj))
	{
		LOG(LOG_ERROR,"calling setProperty on null:" << *name << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));
		createError<TypeError>(context->worker,kConvertNullToObjectError);
		return;
	}
	if (asAtomHandler::isUndefined(*obj))
	{
		LOG(LOG_ERROR,"calling setProperty on undefined:" << *name << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));
		createError<TypeError>(context->worker,kConvertUndefinedToObjectError);
		return;
	}
	ASObject* o = asAtomHandler::toObject(*obj,context->worker);
	ASATOM_INCREF_POINTER(value);
	bool alreadyset=false;
	multiname* simplesettername = nullptr;
	if (context->exec_pos->local3.pos == 0x68)//initproperty
		simplesettername =o->setVariableByMultiname(*name,*value,ASObject::CONST_ALLOWED,&alreadyset,context->worker);
	else//Do not allow to set contant traits
		simplesettername =o->setVariableByMultiname(*name,*value,ASObject::CONST_NOT_ALLOWED,&alreadyset,context->worker);
	if (simplesettername)
		context->exec_pos->cachedmultiname2 = simplesettername;
	if (alreadyset)
		ASATOM_DECREF_POINTER(value);
	++(context->exec_pos);
}
void ABCVm::abc_setPropertyStaticName_local_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));
	multiname* name=context->exec_pos->cachedmultiname2;
	asAtom* obj = &CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	asAtom* value = &CONTEXT_GETLOCAL(context,instrptr->local_pos2);

	LOG_CALL("setProperty_sll " << *name << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));

	if(asAtomHandler::isNull(*obj))
	{
		LOG(LOG_ERROR,"calling setProperty on null:" << *name << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));
		createError<TypeError>(context->worker,kConvertNullToObjectError);
		return;
	}
	if (asAtomHandler::isUndefined(*obj))
	{
		LOG(LOG_ERROR,"calling setProperty on undefined:" << *name << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));
		createError<TypeError>(context->worker,kConvertUndefinedToObjectError);
		return;
	}
	ASObject* o = asAtomHandler::toObject(*obj,context->worker);
	o->incRef(); // this is neccessary for reference counting in case of exception thrown in setVariableByMultiname
	ASATOM_INCREF_POINTER(value);
	bool alreadyset=false;
	multiname* simplesettername = nullptr;
	if (context->exec_pos->local3.pos == 0x68)//initproperty
		simplesettername =o->setVariableByMultiname(*name,*value,ASObject::CONST_ALLOWED,&alreadyset,context->worker);
	else//Do not allow to set contant traits
		simplesettername =o->setVariableByMultiname(*name,*value,ASObject::CONST_NOT_ALLOWED,&alreadyset,context->worker);
	if (simplesettername)
		context->exec_pos->cachedmultiname2 = simplesettername;
	if (alreadyset)
		ASATOM_DECREF_POINTER(value);
	o->decRef(); // this is neccessary for reference counting in case of exception thrown in setVariableByMultiname
	++(context->exec_pos);
}
void ABCVm::abc_setPropertyInteger(call_context* context)
{
	RUNTIME_STACK_POP_CREATE(context,value);
	RUNTIME_STACK_POP_CREATE(context,idx);
	int index = asAtomHandler::getInt(*idx);
	RUNTIME_STACK_POP_CREATE(context,obj);

	LOG_CALL("setPropertyInteger " << index << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));

	if(asAtomHandler::isNull(*obj))
	{
		LOG(LOG_ERROR,"calling setProperty on null:" << index << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));
		ASATOM_DECREF_POINTER(value);
		ASATOM_DECREF_POINTER(obj);
		createError<TypeError>(context->worker,kConvertNullToObjectError);
		return;
	}
	if (asAtomHandler::isUndefined(*obj))
	{
		LOG(LOG_ERROR,"calling setProperty on undefined:" << index << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));
		ASATOM_DECREF_POINTER(value);
		ASATOM_DECREF_POINTER(obj);
		createError<TypeError>(context->worker,kConvertUndefinedToObjectError);
		return;
	}

	ASObject* o = asAtomHandler::getObject(*obj);
	if (!o)
		o = asAtomHandler::toObject(*obj,context->worker);
	bool alreadyset=false;
	if (context->exec_pos->local3.pos == 0x68)//initproperty
		o->setVariableByInteger(index,*value,ASObject::CONST_ALLOWED,&alreadyset,context->worker);
	else//Do not allow to set contant traits
		o->setVariableByInteger(index,*value,ASObject::CONST_NOT_ALLOWED,&alreadyset,context->worker);
	if (alreadyset)
		ASATOM_DECREF_POINTER(value);
	ASATOM_DECREF_POINTER(obj);
	++(context->exec_pos);
}
void ABCVm::abc_setPropertyInteger_constant_constant_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));
	asAtom* obj = instrptr->arg3_constant;
	int index = asAtomHandler::getInt(*instrptr->arg1_constant);
	asAtom* value = instrptr->arg2_constant;

	LOG_CALL("setProperty_i_ccc " << index << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));

	if(asAtomHandler::isNull(*obj))
	{
		LOG(LOG_ERROR,"calling setProperty on null:" << index << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));
		createError<TypeError>(context->worker,kConvertNullToObjectError);
		return;
	}
	if (asAtomHandler::isUndefined(*obj))
	{
		LOG(LOG_ERROR,"calling setProperty on undefined:" << index << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));
		createError<TypeError>(context->worker,kConvertUndefinedToObjectError);
		return;
	}

	ASObject* o = asAtomHandler::getObject(*obj);
	if (!o)
		o = asAtomHandler::toObject(*obj,context->worker);
	ASATOM_INCREF_POINTER(value);
	bool alreadyset=false;
	if (context->exec_pos->local3.pos == 0x68)//initproperty
		o->setVariableByInteger(index,*value,ASObject::CONST_ALLOWED,&alreadyset,context->worker);
	else//Do not allow to set contant traits
		o->setVariableByInteger(index,*value,ASObject::CONST_NOT_ALLOWED,&alreadyset,context->worker);
	if (alreadyset)
		ASATOM_DECREF_POINTER(value);
	++(context->exec_pos);
}
void ABCVm::abc_setPropertyInteger_constant_local_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));
	asAtom* obj = instrptr->arg3_constant;
	int index = asAtomHandler::getInt(CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	asAtom* value = instrptr->arg2_constant;

	LOG_CALL("setProperty_i_clc " << index << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));

	if(asAtomHandler::isNull(*obj))
	{
		LOG(LOG_ERROR,"calling setProperty on null:" << index << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));
		createError<TypeError>(context->worker,kConvertNullToObjectError);
		return;
	}
	if (asAtomHandler::isUndefined(*obj))
	{
		LOG(LOG_ERROR,"calling setProperty on undefined:" << index << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));
		createError<TypeError>(context->worker,kConvertUndefinedToObjectError);
		return;
	}

	ASObject* o = asAtomHandler::getObject(*obj);
	if (!o)
		o = asAtomHandler::toObject(*obj,context->worker);
	ASATOM_INCREF_POINTER(value);
	bool alreadyset=false;
	if (context->exec_pos->local3.pos == 0x68)//initproperty
		o->setVariableByInteger(index,*value,ASObject::CONST_ALLOWED,&alreadyset,context->worker);
	else//Do not allow to set contant traits
		o->setVariableByInteger(index,*value,ASObject::CONST_NOT_ALLOWED,&alreadyset,context->worker);
	if (alreadyset)
		ASATOM_DECREF_POINTER(value);
	++(context->exec_pos);
}
void ABCVm::abc_setPropertyInteger_constant_constant_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));
	asAtom* obj = instrptr->arg3_constant;
	int index = asAtomHandler::getInt(*instrptr->arg1_constant);
	asAtom* value = &CONTEXT_GETLOCAL(context,instrptr->local_pos2);

	LOG_CALL("setProperty_i_ccl " << index << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));

	if(asAtomHandler::isNull(*obj))
	{
		LOG(LOG_ERROR,"calling setProperty on null:" << index << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));
		createError<TypeError>(context->worker,kConvertNullToObjectError);
		return;
	}
	if (asAtomHandler::isUndefined(*obj))
	{
		LOG(LOG_ERROR,"calling setProperty on undefined:" << index << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));
		createError<TypeError>(context->worker,kConvertUndefinedToObjectError);
		return;
	}

	ASObject* o = asAtomHandler::getObject(*obj);
	if (!o)
		o = asAtomHandler::toObject(*obj,context->worker);
	ASATOM_INCREF_POINTER(value);
	bool alreadyset=false;
	if (context->exec_pos->local3.pos == 0x68)//initproperty
		o->setVariableByInteger(index,*value,ASObject::CONST_ALLOWED,&alreadyset,context->worker);
	else//Do not allow to set contant traits
		o->setVariableByInteger(index,*value,ASObject::CONST_NOT_ALLOWED,&alreadyset,context->worker);
	if (alreadyset)
		ASATOM_DECREF_POINTER(value);
	++(context->exec_pos);
}
void ABCVm::abc_setPropertyInteger_constant_local_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));
	asAtom* obj = instrptr->arg3_constant;
	int index = asAtomHandler::getInt(CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	asAtom* value = &CONTEXT_GETLOCAL(context,instrptr->local_pos2);

	LOG_CALL("setProperty_i_cll " << index << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));

	if(asAtomHandler::isNull(*obj))
	{
		LOG(LOG_ERROR,"calling setProperty on null:" << index << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));
		createError<TypeError>(context->worker,kConvertNullToObjectError);
		return;
	}
	if (asAtomHandler::isUndefined(*obj))
	{
		LOG(LOG_ERROR,"calling setProperty on undefined:" << index << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));
		createError<TypeError>(context->worker,kConvertUndefinedToObjectError);
		return;
	}

	ASATOM_INCREF_POINTER(value);
	ASObject* o = asAtomHandler::getObject(*obj);
	if (!o)
		o = asAtomHandler::toObject(*obj,context->worker);
	bool alreadyset=false;
	if (context->exec_pos->local3.pos == 0x68)//initproperty
		o->setVariableByInteger(index,*value,ASObject::CONST_ALLOWED,&alreadyset,context->worker);
	else//Do not allow to set contant traits
		o->setVariableByInteger(index,*value,ASObject::CONST_NOT_ALLOWED,&alreadyset,context->worker);
	if (alreadyset)
		ASATOM_DECREF_POINTER(value);
	++(context->exec_pos);
}
void ABCVm::abc_setPropertyInteger_local_constant_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));
	asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local3.pos);
	int index = asAtomHandler::getInt(*instrptr->arg1_constant);
	asAtom* value = instrptr->arg2_constant;

	LOG_CALL("setProperty_i_lcc " << index << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));

	if(asAtomHandler::isNull(obj))
	{
		LOG(LOG_ERROR,"calling setProperty on null:" << index << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));
		createError<TypeError>(context->worker,kConvertNullToObjectError);
		return;
	}
	if (asAtomHandler::isUndefined(obj))
	{
		LOG(LOG_ERROR,"calling setProperty on undefined:" << index << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));
		createError<TypeError>(context->worker,kConvertUndefinedToObjectError);
		return;
	}

	ASObject* o = asAtomHandler::getObject(obj);
	if (!o)
		o = asAtomHandler::toObject(obj,context->worker);
	ASATOM_INCREF_POINTER(value);
	bool alreadyset=false;
	if (context->exec_pos->local3.pos == 0x68)//initproperty
		o->setVariableByInteger(index,*value,ASObject::CONST_ALLOWED,&alreadyset,context->worker);
	else//Do not allow to set contant traits
		o->setVariableByInteger(index,*value,ASObject::CONST_NOT_ALLOWED,&alreadyset,context->worker);
	if (alreadyset)
		ASATOM_DECREF_POINTER(value);
	++(context->exec_pos);
}
void ABCVm::abc_setPropertyInteger_local_local_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));
	asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local3.pos);
	int index = asAtomHandler::getInt(CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	asAtom* value = instrptr->arg2_constant;

	LOG_CALL("setProperty_i_llc " << index << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));

	if(asAtomHandler::isNull(obj))
	{
		LOG(LOG_ERROR,"calling setProperty on null:" << index << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));
		createError<TypeError>(context->worker,kConvertNullToObjectError);
		return;
	}
	if (asAtomHandler::isUndefined(obj))
	{
		LOG(LOG_ERROR,"calling setProperty on undefined:" << index << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));
		createError<TypeError>(context->worker,kConvertUndefinedToObjectError);
		return;
	}

	ASObject* o = asAtomHandler::getObject(obj);
	if (!o)
		o = asAtomHandler::toObject(obj,context->worker);
	ASATOM_INCREF_POINTER(value);
	bool alreadyset=false;
	if (context->exec_pos->local3.pos == 0x68)//initproperty
		o->setVariableByInteger(index,*value,ASObject::CONST_ALLOWED,&alreadyset,context->worker);
	else//Do not allow to set contant traits
		o->setVariableByInteger(index,*value,ASObject::CONST_NOT_ALLOWED,&alreadyset,context->worker);
	if (alreadyset)
		ASATOM_DECREF_POINTER(value);
	++(context->exec_pos);
}
void ABCVm::abc_setPropertyInteger_local_constant_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));
	asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local3.pos);
	int index = asAtomHandler::getInt(*instrptr->arg1_constant);
	asAtom* value = &CONTEXT_GETLOCAL(context,instrptr->local_pos2);

	LOG_CALL("setProperty_i_lcl " << index << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));

	if(asAtomHandler::isNull(obj))
	{
		LOG(LOG_ERROR,"calling setProperty on null:" << index << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));
		createError<TypeError>(context->worker,kConvertNullToObjectError);
		return;
	}
	if (asAtomHandler::isUndefined(obj))
	{
		LOG(LOG_ERROR,"calling setProperty on undefined:" << index << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));
		createError<TypeError>(context->worker,kConvertUndefinedToObjectError);
		return;
	}

	ASATOM_INCREF_POINTER(value);
	ASObject* o = asAtomHandler::getObject(obj);
	if (!o)
		o = asAtomHandler::toObject(obj,context->worker);
	bool alreadyset=false;
	if (context->exec_pos->local3.pos == 0x68)//initproperty
		o->setVariableByInteger(index,*value,ASObject::CONST_ALLOWED,&alreadyset,context->worker);
	else//Do not allow to set contant traits
		o->setVariableByInteger(index,*value,ASObject::CONST_NOT_ALLOWED,&alreadyset,context->worker);
	if (alreadyset)
		ASATOM_DECREF_POINTER(value);
	++(context->exec_pos);
}
void ABCVm::abc_setPropertyInteger_local_local_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));
	asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local3.pos);
	int index = asAtomHandler::getInt(CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	asAtom* value = &CONTEXT_GETLOCAL(context,instrptr->local_pos2);

	LOG_CALL("setProperty_i_lll " << index << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value)<<" "<<instrptr->local_pos1);

	if(asAtomHandler::isNull(obj))
	{
		LOG(LOG_ERROR,"calling setProperty on null:" << index << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));
		createError<TypeError>(context->worker,kConvertNullToObjectError);
		return;
	}
	if (asAtomHandler::isUndefined(obj))
	{
		LOG(LOG_ERROR,"calling setProperty on undefined:" << index << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));
		createError<TypeError>(context->worker,kConvertUndefinedToObjectError);
		return;
	}

	ASATOM_INCREF_POINTER(value);
	ASObject* o = asAtomHandler::getObject(obj);
	if (!o)
		o = asAtomHandler::toObject(obj,context->worker);
	bool alreadyset=false;
	if (context->exec_pos->local3.pos == 0x68)//initproperty
		o->setVariableByInteger(index,*value,ASObject::CONST_ALLOWED,&alreadyset,context->worker);
	else//Do not allow to set contant traits
		o->setVariableByInteger(index,*value,ASObject::CONST_NOT_ALLOWED,&alreadyset,context->worker);
	if (alreadyset)
		ASATOM_DECREF_POINTER(value);
	++(context->exec_pos);
}


void ABCVm::abc_setPropertyIntegerVector_constant_constant_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));
	asAtom* obj = instrptr->arg3_constant;
	int index = asAtomHandler::getInt(*instrptr->arg1_constant);
	asAtom* value = instrptr->arg2_constant;

	LOG_CALL("setProperty_iv_ccc " << index << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));

	if(asAtomHandler::isNull(*obj))
	{
		LOG(LOG_ERROR,"calling setProperty on null:" << index << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));
		ASATOM_DECREF_POINTER(value);
		createError<TypeError>(context->worker,kConvertNullToObjectError);
		return;
	}
	if (asAtomHandler::isUndefined(*obj))
	{
		LOG(LOG_ERROR,"calling setProperty on undefined:" << index << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));
		ASATOM_DECREF_POINTER(value);
		createError<TypeError>(context->worker,kConvertUndefinedToObjectError);
		return;
	}

	Vector* o = asAtomHandler::as<Vector>(*obj);
	bool alreadyset=false;
	o->setVariableByIntegerNoCoerce(index,*value,&alreadyset,context->worker);
	if (alreadyset)
		ASATOM_DECREF_POINTER(value);
	++(context->exec_pos);
}
void ABCVm::abc_setPropertyIntegerVector_constant_local_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));
	asAtom* obj = instrptr->arg3_constant;
	int index = asAtomHandler::getInt(CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	asAtom* value = instrptr->arg2_constant;

	LOG_CALL("setProperty_iv_clc " << index << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));

	if(asAtomHandler::isNull(*obj))
	{
		LOG(LOG_ERROR,"calling setProperty on null:" << index << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));
		ASATOM_DECREF_POINTER(value);
		createError<TypeError>(context->worker,kConvertNullToObjectError);
		return;
	}
	if (asAtomHandler::isUndefined(*obj))
	{
		LOG(LOG_ERROR,"calling setProperty on undefined:" << index << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));
		ASATOM_DECREF_POINTER(value);
		createError<TypeError>(context->worker,kConvertUndefinedToObjectError);
		return;
	}

	Vector* o = asAtomHandler::as<Vector>(*obj);
	bool alreadyset=false;
	o->setVariableByIntegerNoCoerce(index,*value,&alreadyset,context->worker);
	if (alreadyset)
		ASATOM_DECREF_POINTER(value);
	++(context->exec_pos);
}
void ABCVm::abc_setPropertyIntegerVector_constant_constant_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));
	asAtom* obj = instrptr->arg3_constant;
	int index = asAtomHandler::getInt(*instrptr->arg1_constant);
	asAtom* value = &CONTEXT_GETLOCAL(context,instrptr->local_pos2);

	LOG_CALL("setProperty_iv_ccl " << index << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));

	if(asAtomHandler::isNull(*obj))
	{
		LOG(LOG_ERROR,"calling setProperty on null:" << index << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));
		ASATOM_DECREF_POINTER(value);
		createError<TypeError>(context->worker,kConvertNullToObjectError);
		return;
	}
	if (asAtomHandler::isUndefined(*obj))
	{
		LOG(LOG_ERROR,"calling setProperty on undefined:" << index << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));
		ASATOM_DECREF_POINTER(value);
		createError<TypeError>(context->worker,kConvertUndefinedToObjectError);
		return;
	}

	Vector* o = asAtomHandler::as<Vector>(*obj);
	bool alreadyset=false;
	o->setVariableByIntegerNoCoerce(index,*value,&alreadyset,context->worker);
	if (alreadyset)
		ASATOM_DECREF_POINTER(value);
	++(context->exec_pos);
}
void ABCVm::abc_setPropertyIntegerVector_constant_local_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));
	asAtom* obj = instrptr->arg3_constant;
	int index = asAtomHandler::getInt(CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	asAtom* value = &CONTEXT_GETLOCAL(context,instrptr->local_pos2);

	LOG_CALL("setProperty_iv_cll " << index << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));

	if(asAtomHandler::isNull(*obj))
	{
		LOG(LOG_ERROR,"calling setProperty on null:" << index << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));
		createError<TypeError>(context->worker,kConvertNullToObjectError);
		return;
	}
	if (asAtomHandler::isUndefined(*obj))
	{
		LOG(LOG_ERROR,"calling setProperty on undefined:" << index << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));
		createError<TypeError>(context->worker,kConvertUndefinedToObjectError);
		return;
	}

	ASATOM_INCREF_POINTER(value);
	Vector* o = asAtomHandler::as<Vector>(*obj);
	bool alreadyset=false;
	o->setVariableByIntegerNoCoerce(index,*value,&alreadyset,context->worker);
	if (alreadyset)
		ASATOM_DECREF_POINTER(value);
	++(context->exec_pos);
}
void ABCVm::abc_setPropertyIntegerVector_local_constant_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));
	asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local3.pos);
	int index = asAtomHandler::getInt(*instrptr->arg1_constant);
	asAtom* value = instrptr->arg2_constant;

	LOG_CALL("setProperty_iv_lcc " << index << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));

	if(asAtomHandler::isNull(obj))
	{
		LOG(LOG_ERROR,"calling setProperty on null:" << index << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));
		ASATOM_DECREF_POINTER(value);
		createError<TypeError>(context->worker,kConvertNullToObjectError);
		return;
	}
	if (asAtomHandler::isUndefined(obj))
	{
		LOG(LOG_ERROR,"calling setProperty on undefined:" << index << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));
		ASATOM_DECREF_POINTER(value);
		createError<TypeError>(context->worker,kConvertUndefinedToObjectError);
		return;
	}

	Vector* o = asAtomHandler::as<Vector>(obj);
	bool alreadyset=false;
	o->setVariableByIntegerNoCoerce(index,*value,&alreadyset,context->worker);
	if (alreadyset)
		ASATOM_DECREF_POINTER(value);
	++(context->exec_pos);
}
void ABCVm::abc_setPropertyIntegerVector_local_local_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));
	asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local3.pos);
	int index = asAtomHandler::getInt(CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	asAtom* value = instrptr->arg2_constant;

	LOG_CALL("setProperty_iv_llc " << index << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));

	if(asAtomHandler::isNull(obj))
	{
		LOG(LOG_ERROR,"calling setProperty on null:" << index << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));
		ASATOM_DECREF_POINTER(value);
		createError<TypeError>(context->worker,kConvertNullToObjectError);
		return;
	}
	if (asAtomHandler::isUndefined(obj))
	{
		LOG(LOG_ERROR,"calling setProperty on undefined:" << index << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));
		ASATOM_DECREF_POINTER(value);
		createError<TypeError>(context->worker,kConvertUndefinedToObjectError);
		return;
	}

	Vector* o = asAtomHandler::as<Vector>(obj);
	bool alreadyset=false;
	o->setVariableByIntegerNoCoerce(index,*value,&alreadyset,context->worker);
	if (alreadyset)
		ASATOM_DECREF_POINTER(value);
	++(context->exec_pos);
}
void ABCVm::abc_setPropertyIntegerVector_local_constant_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));
	asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local3.pos);
	int index = asAtomHandler::getInt(*instrptr->arg1_constant);
	asAtom* value = &CONTEXT_GETLOCAL(context,instrptr->local_pos2);

	LOG_CALL("setProperty_iv_lcl " << index << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));

	if(asAtomHandler::isNull(obj))
	{
		LOG(LOG_ERROR,"calling setProperty on null:" << index << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));
		createError<TypeError>(context->worker,kConvertNullToObjectError);
		return;
	}
	if (asAtomHandler::isUndefined(obj))
	{
		LOG(LOG_ERROR,"calling setProperty on undefined:" << index << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));
		createError<TypeError>(context->worker,kConvertUndefinedToObjectError);
		return;
	}

	ASATOM_INCREF_POINTER(value);
	Vector* o = asAtomHandler::as<Vector>(obj);
	bool alreadyset=false;
	o->setVariableByIntegerNoCoerce(index,*value,&alreadyset,context->worker);
	if (alreadyset)
		ASATOM_DECREF_POINTER(value);
	++(context->exec_pos);
}
void ABCVm::abc_setPropertyIntegerVector_local_local_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));
	asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local3.pos);
	int index = asAtomHandler::getInt(CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	asAtom* value = &CONTEXT_GETLOCAL(context,instrptr->local_pos2);

	LOG_CALL("setProperty_iv_lll " << index << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value)<<" "<<instrptr->local_pos1);

	if(asAtomHandler::isNull(obj))
	{
		LOG(LOG_ERROR,"calling setProperty on null:" << index << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));
		createError<TypeError>(context->worker,kConvertNullToObjectError);
		return;
	}
	if (asAtomHandler::isUndefined(obj))
	{
		LOG(LOG_ERROR,"calling setProperty on undefined:" << index << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));
		createError<TypeError>(context->worker,kConvertUndefinedToObjectError);
		return;
	}

	ASATOM_INCREF_POINTER(value);
	Vector* o = asAtomHandler::as<Vector>(obj);
	bool alreadyset=false;
	o->setVariableByIntegerNoCoerce(index,*value,&alreadyset,context->worker);
	if (alreadyset)
		ASATOM_DECREF_POINTER(value);
	++(context->exec_pos);
}

void ABCVm::abc_setlocal_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t i = ((context->exec_pos)++)->arg3_uint;

	asAtom* obj = instrptr->arg1_constant;
	LOG_CALL( "setLocal_c n " << i << ": " << asAtomHandler::toDebugString(*obj) );
	assert(i <= uint32_t(context->mi->body->getReturnValuePos()+1+context->mi->body->localresultcount));
	if ((int)i != context->argarrayposition || asAtomHandler::isArray(*obj))
	{
		ASATOM_DECREF(context->locals[i]);
		context->locals[i]=*instrptr->arg1_constant;
	}
}
void ABCVm::abc_setlocal_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t i = ((context->exec_pos)++)->arg3_uint;

	asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	LOG_CALL( "setLocal_l n " << i << ": " << asAtomHandler::toDebugString(obj) <<" "<<instrptr->local_pos1);
	assert(i <= uint32_t(context->mi->body->getReturnValuePos()+1+context->mi->body->localresultcount));
	if ((int)i != context->argarrayposition || asAtomHandler::isArray(obj))
	{
		ASATOM_INCREF(obj);
		ASATOM_DECREF(context->locals[i]);
		context->locals[i]=obj;
	}
}
void ABCVm::abc_getscopeobject_localresult(call_context* context)
{
	uint32_t t = context->exec_pos->arg2_uint;
	assert_and_throw(context->curr_scope_stack > (uint32_t)t);
	asAtom ret=context->scope_stack[t];
	if (ret.uintval != CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos).uintval)
	{
		ASATOM_INCREF(ret);
		replacelocalresult(context,context->exec_pos->local3.pos,ret);
	}
	LOG_CALL("getScopeObject_l " << asAtomHandler::toDebugString(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos))<<" "<<t<<" "<<context->exec_pos->local3.pos);
	++(context->exec_pos);
}
void ABCVm::abc_getProperty_constant_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t t = (++(context->exec_pos))->arg3_uint;
	multiname* name=context->mi->context->getMultinameImpl(*instrptr->arg2_constant,nullptr,t,false);
	ASObject* obj= asAtomHandler::toObject(*instrptr->arg1_constant,context->worker);
	LOG_CALL( "getProperty_cc " << *name << ' ' << obj->toDebugString() << ' '<<obj->isInitialized());
	asAtom prop=asAtomHandler::invalidAtom;
	obj->getVariableByMultiname(prop,*name,GET_VARIABLE_OPTION::NONE, context->worker);
	if (checkPropertyException(obj,name,prop))
		return;
	RUNTIME_STACK_PUSH(context,prop);
	++(context->exec_pos);
}
void ABCVm::abc_getProperty_local_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t t = (++(context->exec_pos))->arg3_uint;
	multiname* name=context->mi->context->getMultinameImpl(*instrptr->arg2_constant,nullptr,t,false);
	ASObject* obj= asAtomHandler::toObject(CONTEXT_GETLOCAL(context,instrptr->local_pos1),context->worker);
	LOG_CALL( "getProperty_lc " << *name << ' ' << obj->toDebugString() << ' '<<obj->isInitialized());
	asAtom prop=asAtomHandler::invalidAtom;
	obj->getVariableByMultiname(prop,*name,GET_VARIABLE_OPTION::NONE, context->worker);
	if (checkPropertyException(obj,name,prop))
		return;
	RUNTIME_STACK_PUSH(context,prop);
	++(context->exec_pos);
}
void ABCVm::abc_getProperty_constant_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t t = (++(context->exec_pos))->arg3_uint;
	multiname* name=context->mi->context->getMultinameImpl(CONTEXT_GETLOCAL(context,instrptr->local_pos2),nullptr,t,false);
	ASObject* obj= asAtomHandler::toObject(*instrptr->arg1_constant,context->worker);
	LOG_CALL( "getProperty_cl " << *name << ' ' << obj->toDebugString() << ' '<<obj->isInitialized());
	asAtom prop=asAtomHandler::invalidAtom;
	obj->getVariableByMultiname(prop,*name,GET_VARIABLE_OPTION::NONE, context->worker);
	if (checkPropertyException(obj,name,prop))
		return;
	RUNTIME_STACK_PUSH(context,prop);
	++(context->exec_pos);
}
void ABCVm::abc_getProperty_local_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t t = (++(context->exec_pos))->arg3_uint;
	multiname* name=context->mi->context->getMultinameImpl(CONTEXT_GETLOCAL(context,instrptr->local_pos2),nullptr,t,false);
	ASObject* obj= asAtomHandler::toObject(CONTEXT_GETLOCAL(context,instrptr->local_pos1),context->worker);
	LOG_CALL( "getProperty_ll " << *name <<"("<<instrptr->local_pos2<<")"<< ' ' << obj->toDebugString() <<"("<<instrptr->local_pos1<<")"<< ' '<<obj->isInitialized());
	asAtom prop=asAtomHandler::invalidAtom;
	obj->getVariableByMultiname(prop,*name,GET_VARIABLE_OPTION::NONE, context->worker);
	if (checkPropertyException(obj,name,prop))
		return;
	RUNTIME_STACK_PUSH(context,prop);
	++(context->exec_pos);
}
void ABCVm::abc_getProperty_constant_constant_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t t = (++(context->exec_pos))->arg3_uint;
	multiname* name=context->mi->context->getMultinameImpl(*instrptr->arg2_constant,nullptr,t,false);
	ASObject* obj= asAtomHandler::toObject(*instrptr->arg1_constant,context->worker);
	LOG_CALL( "getProperty_ccl " << *name << ' ' << obj->toDebugString() << ' '<<obj->isInitialized());
	asAtom prop=asAtomHandler::invalidAtom;
	obj->getVariableByMultiname(prop,*name,GET_VARIABLE_OPTION::NONE,context->worker);
	if (checkPropertyException(obj,name,prop))
		return;
	replacelocalresult(context,instrptr->local3.pos,prop);
	++(context->exec_pos);
}
void ABCVm::abc_getProperty_local_constant_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t t = (++(context->exec_pos))->arg3_uint;
	asAtom prop=asAtomHandler::invalidAtom;
	if (asAtomHandler::isInteger(*instrptr->arg2_constant) && asAtomHandler::isObject(CONTEXT_GETLOCAL(context,instrptr->local_pos1)))
	{
		int n = asAtomHandler::toInt(*instrptr->arg2_constant);
		LOG_CALL( "getProperty_lcl int " << n << ' ' << asAtomHandler::toDebugString(CONTEXT_GETLOCAL(context,instrptr->local_pos1)));
		ASObject* obj= asAtomHandler::getObjectNoCheck(CONTEXT_GETLOCAL(context,instrptr->local_pos1));
		obj->getVariableByInteger(prop,n,GET_VARIABLE_OPTION::NONE,context->worker);
		if (checkPropertyExceptionInteger(obj,n,prop))
			return;
	}
	else
	{
		multiname* name=context->mi->context->getMultinameImpl(*instrptr->arg2_constant,nullptr,t,false);
		ASObject* obj= asAtomHandler::toObject(CONTEXT_GETLOCAL(context,instrptr->local_pos1),context->worker);
		LOG_CALL( "getProperty_lcl " << *name << ' ' << obj->toDebugString() << ' '<<obj->isInitialized());
		obj->getVariableByMultiname(prop,*name,GET_VARIABLE_OPTION::NONE,context->worker);
		if (checkPropertyException(obj,name,prop))
			return;
	}
	replacelocalresult(context,instrptr->local3.pos,prop);
	++(context->exec_pos);
}
void ABCVm::abc_getProperty_constant_local_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t t = (++(context->exec_pos))->arg3_uint;
	multiname* name=context->mi->context->getMultinameImpl(CONTEXT_GETLOCAL(context,instrptr->local_pos2),nullptr,t,false);
	ASObject* obj= asAtomHandler::toObject(*instrptr->arg1_constant,context->worker,true);
	LOG_CALL( "getProperty_cll " << *name << ' ' << obj->toDebugString() << ' '<<obj->isInitialized());
	asAtom prop=asAtomHandler::invalidAtom;
	obj->getVariableByMultiname(prop,*name,GET_VARIABLE_OPTION::NONE,context->worker);
	if (checkPropertyException(obj,name,prop))
		return;
	replacelocalresult(context,instrptr->local3.pos,prop);
	++(context->exec_pos);
}
void ABCVm::abc_getProperty_local_local_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t t = (++(context->exec_pos))->arg3_uint;
	asAtom prop=asAtomHandler::invalidAtom;
	if (asAtomHandler::isInteger(CONTEXT_GETLOCAL(context,instrptr->local_pos2)) && asAtomHandler::isObject(CONTEXT_GETLOCAL(context,instrptr->local_pos1)))
	{
		int n = asAtomHandler::toInt(CONTEXT_GETLOCAL(context,instrptr->local_pos2));
		LOG_CALL( "getProperty_lll int " << n << ' ' << asAtomHandler::toDebugString(CONTEXT_GETLOCAL(context,instrptr->local_pos1)));
		ASObject* obj= asAtomHandler::getObjectNoCheck(CONTEXT_GETLOCAL(context,instrptr->local_pos1));
		obj->getVariableByInteger(prop,n,GET_VARIABLE_OPTION::NONE,context->worker);
		if (checkPropertyExceptionInteger(obj,n,prop))
			return;
	}
	else
	{
		multiname* name=context->mi->context->getMultinameImpl(CONTEXT_GETLOCAL(context,instrptr->local_pos2),nullptr,t,false);
		ASObject* obj= asAtomHandler::toObject(CONTEXT_GETLOCAL(context,instrptr->local_pos1),context->worker);
		LOG_CALL( "getProperty_lll " << *name << ' ' << obj->toDebugString() << ' '<<obj->isInitialized());
		obj->getVariableByMultiname(prop,*name,GET_VARIABLE_OPTION::NONE,context->worker);
		if (checkPropertyException(obj,name,prop))
			return;
	}
	replacelocalresult(context,instrptr->local3.pos,prop);
	++(context->exec_pos);
}
void ABCVm::abc_getPropertyInteger(call_context* context)
{
	RUNTIME_STACK_POP_CREATE(context,arg1);
	RUNTIME_STACK_POP_CREATE_ASOBJECT(context,obj);
	int index=asAtomHandler::toInt(*arg1);
	LOG_CALL( "getPropertyInteger " << index << ' ' << obj->toDebugString() << ' '<<obj->isInitialized());
	asAtom prop=asAtomHandler::invalidAtom;
	if (obj->is<Vector>())
	{
		obj->as<Vector>()->getVariableByIntegerDirect(prop,index,context->worker);
		ASATOM_INCREF(prop);
	}
	else
		obj->getVariableByInteger(prop,index,GET_VARIABLE_OPTION::NONE,context->worker);
	if (checkPropertyExceptionInteger(obj,index,prop))
		return;
	RUNTIME_STACK_PUSH(context,prop);
	++(context->exec_pos);
}
void ABCVm::abc_getPropertyInteger_constant_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	int index=asAtomHandler::toInt(*instrptr->arg2_constant);
	ASObject* obj= asAtomHandler::getObject(*instrptr->arg1_constant);
	if (!obj)
		obj= asAtomHandler::toObject(*instrptr->arg1_constant,context->worker);
	LOG_CALL( "getPropertyInteger_cc " << index << ' ' << obj->toDebugString() << ' '<<obj->isInitialized());
	asAtom prop=asAtomHandler::invalidAtom;
	if (obj->is<Vector>())
	{
		obj->as<Vector>()->getVariableByIntegerDirect(prop,index,context->worker);
		ASATOM_INCREF(prop);
	}
	else
		obj->getVariableByInteger(prop,index,GET_VARIABLE_OPTION::NONE,context->worker);
	if (checkPropertyExceptionInteger(obj,index,prop))
		return;
	RUNTIME_STACK_PUSH(context,prop);
	++(context->exec_pos);
}
void ABCVm::abc_getPropertyInteger_local_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	int index=asAtomHandler::toInt(*instrptr->arg2_constant);
	ASObject* obj= asAtomHandler::getObject(CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	if (!obj)
		obj= asAtomHandler::toObject(CONTEXT_GETLOCAL(context,instrptr->local_pos1),context->worker);
	LOG_CALL( "getPropertyInteger_lc " << index << ' ' << obj->toDebugString() << ' '<<obj->isInitialized());
	asAtom prop=asAtomHandler::invalidAtom;
	if (obj->is<Vector>())
	{
		obj->as<Vector>()->getVariableByIntegerDirect(prop,index,context->worker);
		ASATOM_INCREF(prop);
	}
	else
		obj->getVariableByInteger(prop,index,GET_VARIABLE_OPTION::NONE,context->worker);
	if (checkPropertyExceptionInteger(obj,index,prop))
		return;
	RUNTIME_STACK_PUSH(context,prop);
	++(context->exec_pos);
}
void ABCVm::abc_getPropertyInteger_constant_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	int index=asAtomHandler::toInt(CONTEXT_GETLOCAL(context,instrptr->local_pos2));
	ASObject* obj= asAtomHandler::getObject(*instrptr->arg1_constant);
	if (!obj)
		obj= asAtomHandler::toObject(*instrptr->arg1_constant,context->worker);
	LOG_CALL( "getPropertyInteger_cl " << index << ' ' << obj->toDebugString() << ' '<<obj->isInitialized());
	asAtom prop=asAtomHandler::invalidAtom;
	if (obj->is<Vector>())
	{
		obj->as<Vector>()->getVariableByIntegerDirect(prop,index,context->worker);
		ASATOM_INCREF(prop);
	}
	else
		obj->getVariableByInteger(prop,index,GET_VARIABLE_OPTION::NONE,context->worker);
	if (checkPropertyExceptionInteger(obj,index,prop))
		return;
	RUNTIME_STACK_PUSH(context,prop);
	++(context->exec_pos);
}
void ABCVm::abc_getPropertyInteger_local_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	int index=asAtomHandler::toInt(CONTEXT_GETLOCAL(context,instrptr->local_pos2));
	ASObject* obj = nullptr;
	if (asAtomHandler::isObject(CONTEXT_GETLOCAL(context,instrptr->local_pos1)))
		obj = asAtomHandler::getObjectNoCheck(CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	else
		obj= asAtomHandler::toObject(CONTEXT_GETLOCAL(context,instrptr->local_pos1),context->worker);
	LOG_CALL( "getPropertyInteger_ll " << index <<"("<<instrptr->local_pos2<<")"<< ' ' << obj->toDebugString() <<"("<<instrptr->local_pos1<<")"<< ' '<<obj->isInitialized());
	asAtom prop=asAtomHandler::invalidAtom;
	if (obj->is<Vector>())
	{
		obj->as<Vector>()->getVariableByIntegerDirect(prop,index,context->worker);
		ASATOM_INCREF(prop);
	}
	else
		obj->getVariableByInteger(prop,index,GET_VARIABLE_OPTION::NONE,context->worker);
	if (checkPropertyExceptionInteger(obj,index,prop))
		return;
	RUNTIME_STACK_PUSH(context,prop);
	++(context->exec_pos);
}
void ABCVm::abc_getPropertyInteger_constant_constant_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	int index=asAtomHandler::toInt(*instrptr->arg2_constant);
	ASObject* obj= asAtomHandler::getObject(*instrptr->arg1_constant);
	if (!obj)
		obj= asAtomHandler::toObject(*instrptr->arg1_constant,context->worker);
	LOG_CALL( "getPropertyInteger_ccl " << index << ' ' << obj->toDebugString() << ' '<<obj->isInitialized());
	asAtom prop=asAtomHandler::invalidAtom;
	obj->getVariableByInteger(prop,index,GET_VARIABLE_OPTION::NONE,context->worker);
	if (checkPropertyExceptionInteger(obj,index,prop))
		return;
	ASATOM_INCREF(prop);
	replacelocalresult(context,instrptr->local3.pos,prop);
	++(context->exec_pos);
}
void ABCVm::abc_getPropertyInteger_local_constant_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	int index=asAtomHandler::toInt(*instrptr->arg2_constant);
	ASObject* obj= asAtomHandler::getObject(CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	if (!obj)
		obj= asAtomHandler::toObject(CONTEXT_GETLOCAL(context,instrptr->local_pos1),context->worker);
	LOG_CALL( "getPropertyInteger_lcl " << index << ' ' << obj->toDebugString() << ' '<<obj->isInitialized());
	asAtom prop=asAtomHandler::invalidAtom;
	if (obj->is<Vector>())
	{
		obj->as<Vector>()->getVariableByIntegerDirect(prop,index,context->worker);
		ASATOM_INCREF(prop);
	}
	else
		obj->getVariableByInteger(prop,index,GET_VARIABLE_OPTION::NONE,context->worker);
	if (checkPropertyExceptionInteger(obj,index,prop))
		return;
	replacelocalresult(context,instrptr->local3.pos,prop);
	++(context->exec_pos);
}
void ABCVm::abc_getPropertyInteger_constant_local_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	int index=asAtomHandler::toInt(CONTEXT_GETLOCAL(context,instrptr->local_pos2));
	ASObject* obj= asAtomHandler::getObject(*instrptr->arg1_constant);
	if (!obj)
		obj= asAtomHandler::toObject(*instrptr->arg1_constant,context->worker,true);
	LOG_CALL( "getPropertyInteger_cll " << index << ' ' << obj->toDebugString() << ' '<<obj->isInitialized());
	asAtom prop=asAtomHandler::invalidAtom;
	if (obj->is<Vector>())
	{
		obj->as<Vector>()->getVariableByIntegerDirect(prop,index,context->worker);
		ASATOM_INCREF(prop);
	}
	else
		obj->getVariableByInteger(prop,index,GET_VARIABLE_OPTION::NONE,context->worker);
	if (checkPropertyExceptionInteger(obj,index,prop))
		return;
	replacelocalresult(context,instrptr->local3.pos,prop);
	++(context->exec_pos);
}
void ABCVm::abc_getPropertyInteger_local_local_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	int index=asAtomHandler::toInt(CONTEXT_GETLOCAL(context,instrptr->local_pos2));
	ASObject* obj= asAtomHandler::getObject(CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	if (!obj)
		obj= asAtomHandler::toObject(CONTEXT_GETLOCAL(context,instrptr->local_pos1),context->worker);
	LOG_CALL( "getPropertyInteger_lll " << index << ' ' << obj->toDebugString() << ' '<<obj->isInitialized());
	asAtom prop=asAtomHandler::invalidAtom;
	if (obj->is<Vector>())
	{
		obj->as<Vector>()->getVariableByIntegerDirect(prop,index,context->worker);
		ASATOM_INCREF(prop);
	}
	else
		obj->getVariableByInteger(prop,index,GET_VARIABLE_OPTION::NONE,context->worker);
	if (checkPropertyExceptionInteger(obj,index,prop))
		return;
	replacelocalresult(context,instrptr->local3.pos,prop);
	++(context->exec_pos);
}
void ABCVm::abc_getPropertyStaticName_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	multiname* name=instrptr->cachedmultiname2;

	ASObject* obj= asAtomHandler::toObject(*instrptr->arg1_constant,context->worker);
	LOG_CALL( "getProperty_sc " << *name << ' ' << obj->toDebugString() << ' '<<obj->isInitialized());
	asAtom prop=asAtomHandler::invalidAtom;
	if(asAtomHandler::isInvalid(prop))
	{
		bool isgetter = obj->getVariableByMultiname(prop,*name,GET_VARIABLE_OPTION::DONT_CALL_GETTER,context->worker) & GET_VARIABLE_RESULT::GETVAR_ISGETTER;
		if (isgetter)
		{
			//Call the getter
			LOG_CALL("Calling the getter for " << *name << " on " << obj->toDebugString());
			assert(asAtomHandler::isFunction(prop));
			IFunction* f = asAtomHandler::as<IFunction>(prop);
			ASObject* closure = asAtomHandler::getClosure(prop);
			prop = asAtom();
			multiname* simplegetter = f->callGetter(prop,closure ? closure : obj,context->worker);
			if (simplegetter)
			{
				LOG_CALL("is simple getter " << *simplegetter);
				instrptr->cachedmultiname2 = simplegetter;
			}
			LOG_CALL("End of getter"<< ' ' << f->toDebugString()<<" result:"<<asAtomHandler::toDebugString(prop));
		}
	}
	if(checkPropertyException(obj,name,prop))
		return;
	RUNTIME_STACK_PUSH(context,prop);
	++(context->exec_pos);
}
void ABCVm::abc_getPropertyStaticName_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom prop=asAtomHandler::invalidAtom;
	multiname* name=instrptr->cachedmultiname2;

	if (name->name_type == multiname::NAME_INT
			&& asAtomHandler::is<Array>(CONTEXT_GETLOCAL(context,instrptr->local_pos1))
			&& name->name_i > 0
			&& (uint32_t)name->name_i < asAtomHandler::as<Array>(CONTEXT_GETLOCAL(context,instrptr->local_pos1))->currentsize)
	{
		LOG_CALL( "getProperty_sl " << name->name_i << ' ' << asAtomHandler::toDebugString(CONTEXT_GETLOCAL(context,instrptr->local_pos1)));
		asAtomHandler::as<Array>(CONTEXT_GETLOCAL(context,instrptr->local_pos1))->at_nocheck(prop,name->name_i);
		ASATOM_INCREF(prop);
	}
	else
	{
		ASObject* obj= asAtomHandler::toObject(CONTEXT_GETLOCAL(context,instrptr->local_pos1),context->worker);
		LOG_CALL( "getProperty_sl " << *name << ' ' << obj->toDebugString() << ' '<<obj->isInitialized());
		if(asAtomHandler::isInvalid(prop))
		{
			bool isgetter = obj->getVariableByMultiname(prop,*name,GET_VARIABLE_OPTION::DONT_CALL_GETTER,context->worker) & GET_VARIABLE_RESULT::GETVAR_ISGETTER;
			if (isgetter)
			{
				//Call the getter
				LOG_CALL("Calling the getter for " << *name << " on " << obj->toDebugString());
				assert(asAtomHandler::isFunction(prop));
				IFunction* f = asAtomHandler::as<IFunction>(prop);
				ASObject* closure = asAtomHandler::getClosure(prop);
				prop = asAtom();
				multiname* simplegetter = f->callGetter(prop,closure ? closure : obj,context->worker);
				if (simplegetter)
				{
					LOG_CALL("is simple getter " << *simplegetter);
					instrptr->cachedmultiname2 = simplegetter;
				}
				LOG_CALL("End of getter"<< ' ' << f->toDebugString()<<" result:"<<asAtomHandler::toDebugString(prop));
			}
		}
		if(checkPropertyException(obj,name,prop))
			return;
	}
	RUNTIME_STACK_PUSH(context,prop);
	++(context->exec_pos);
}
void ABCVm::abc_getPropertyStaticName_constant_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	multiname* name=instrptr->cachedmultiname2;

	ASObject* obj= asAtomHandler::toObject(*instrptr->arg1_constant,context->worker,true);
	LOG_CALL( "getProperty_scl " << *name << ' ' << obj->toDebugString() << ' '<<obj->isInitialized());
	asAtom prop=asAtomHandler::invalidAtom;
	if(asAtomHandler::isInvalid(prop))
	{
		GET_VARIABLE_RESULT getvarres = obj->getVariableByMultiname(prop,*name,GET_VARIABLE_OPTION::DONT_CALL_GETTER,context->worker);
		bool isgetter = getvarres & GET_VARIABLE_RESULT::GETVAR_ISGETTER;
		if (isgetter)
		{
			//Call the getter
			LOG_CALL("Calling the getter for " << *name << " on " << obj->toDebugString());
			assert(asAtomHandler::isFunction(prop));
			IFunction* f = asAtomHandler::as<IFunction>(prop);
			ASObject* closure = asAtomHandler::getClosure(prop);
			prop = asAtom();
			multiname* simplegetter = f->callGetter(prop,closure ? closure : obj,context->worker);
			if (simplegetter)
			{
				LOG_CALL("is simple getter " << *simplegetter);
				instrptr->cachedmultiname2 = simplegetter;
			}
			LOG_CALL("End of getter"<< ' ' << f->toDebugString()<<" result:"<<asAtomHandler::toDebugString(prop));
		}
	}
	if(checkPropertyException(obj,name,prop))
		return;
	replacelocalresult(context,instrptr->local3.pos,prop);
	++(context->exec_pos);
}
void ABCVm::abc_getPropertyStaticName_local_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	multiname* name=instrptr->cachedmultiname2;

	if (name->name_type == multiname::NAME_INT
			&& asAtomHandler::is<Array>(CONTEXT_GETLOCAL(context,instrptr->local_pos1))
			&& name->name_i > 0
			&& (uint32_t)name->name_i < asAtomHandler::as<Array>(CONTEXT_GETLOCAL(context,instrptr->local_pos1))->currentsize)
	{
		LOG_CALL( "getProperty_slli " << name->name_i << ' ' << asAtomHandler::toDebugString(CONTEXT_GETLOCAL(context,instrptr->local_pos1)));
		asAtom oldres = CONTEXT_GETLOCAL(context,instrptr->local3.pos);
		asAtomHandler::as<Array>(CONTEXT_GETLOCAL(context,instrptr->local_pos1))->at_nocheck(CONTEXT_GETLOCAL(context,instrptr->local3.pos),name->name_i);
		ASATOM_INCREF(CONTEXT_GETLOCAL(context,instrptr->local3.pos));
		ASATOM_DECREF(oldres);
	}
	else
	{
		ASObject* obj= asAtomHandler::toObject(CONTEXT_GETLOCAL(context,instrptr->local_pos1),context->worker);
		asAtom prop=asAtomHandler::invalidAtom;
// TODO caching doesn't work until we find a way to detect if obj is a reused object
// that has been destroyed since the last call
//		if (((context->exec_pos->data&ABC_OP_NOTCACHEABLE) == 0)
//				&& ((obj->is<Class_base>() && obj->as<Class_base>()->isSealed) ||
//					(obj->getClass()
//					 && (obj->getClass()->isSealed
//						 || (obj->getClass() == Class<Array>::getRef(obj->getSystemState()).getPtr())))))
//		{
//			variable* v = obj->findVariableByMultiname(*name,obj->getClass());
//			if (v)
//			{
//				context->exec_pos->data |= ABC_OP_CACHED;
//				context->exec_pos->cachedvar2=v;
//				context->exec_pos->cacheobj1=obj;
//				if (asAtomHandler::isValid(v->getter))
//				{
//					asAtomHandler::set(prop,v->getter);
//					//Call the getter
//					LOG_CALL("Calling the getter for " << *name << " on " << obj->toDebugString());
//					assert(asAtomHandler::isFunction(prop));
//					IFunction* f = asAtomHandler::as<IFunction>(prop);
//					ASObject* closure = asAtomHandler::getClosure(prop);
//					f->callGetter(prop,closure ? closure : obj);
//					LOG_CALL("End of getter"<< ' ' << f->toDebugString()<<" result:"<<asAtomHandler::toDebugString(prop));
//				}
//				else
//				{
//					asAtomHandler::set(prop,v->var);
//					ASATOM_INCREF(prop);
//				}
//			}
//		}
		if(asAtomHandler::isInvalid(prop))
		{
			GET_VARIABLE_RESULT getvarres = obj->getVariableByMultiname(prop,*name,GET_VARIABLE_OPTION::DONT_CALL_GETTER,context->worker);
			bool isgetter = getvarres & GET_VARIABLE_RESULT::GETVAR_ISGETTER;
			if (isgetter)
			{
				//Call the getter
				LOG_CALL("Calling the getter for " << *name << " on " << obj->toDebugString());
				assert(asAtomHandler::isFunction(prop));
				IFunction* f = asAtomHandler::as<IFunction>(prop);
				ASObject* closure = asAtomHandler::getClosure(prop);
				prop = asAtom();
				multiname* simplegetter = f->callGetter(prop,closure ? closure : obj,context->worker);
				if (simplegetter)
				{
					LOG_CALL("is simple getter " << *simplegetter);
					instrptr->cachedmultiname2 = simplegetter;
				}
				LOG_CALL("End of getter"<< ' ' << f->toDebugString()<<" result:"<<asAtomHandler::toDebugString(prop));
			}
			else
			{
				LOG_CALL("getProperty_sll " << *name << ' ' << obj->toDebugString()<<" "<<instrptr->local3.pos<<" "<<asAtomHandler::toDebugString(prop));
			}

		}
		if(checkPropertyException(obj,name,prop))
			return;
		replacelocalresult(context,instrptr->local3.pos,prop);
	}
	++(context->exec_pos);
}
void ABCVm::abc_getPropertyStaticName_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	multiname* name=instrptr->cachedmultiname2;

	RUNTIME_STACK_POP_CREATE_ASOBJECT(context,obj);
	LOG_CALL( "getProperty_slr " << *name << ' ' << obj->toDebugString() << ' '<<obj->isInitialized()<<" "<<instrptr->local3.pos);
	asAtom prop=asAtomHandler::invalidAtom;
	if(asAtomHandler::isInvalid(prop))
	{
		GET_VARIABLE_RESULT getvarres = obj->getVariableByMultiname(prop,*name,GET_VARIABLE_OPTION::DONT_CALL_GETTER,context->worker);
		bool isgetter = getvarres & GET_VARIABLE_RESULT::GETVAR_ISGETTER;
		if (isgetter)
		{
			//Call the getter
			LOG_CALL("Calling the getter for " << *name << " on " << obj->toDebugString());
			assert(asAtomHandler::isFunction(prop));
			IFunction* f = asAtomHandler::as<IFunction>(prop);
			ASObject* closure = asAtomHandler::getClosure(prop);
			prop = asAtom();
			multiname* simplegetter = f->callGetter(prop,closure ? closure : obj,context->worker);
			if (simplegetter)
			{
				LOG_CALL("is simple getter " << *simplegetter);
				instrptr->cachedmultiname2 = simplegetter;
			}
			LOG_CALL("End of getter"<< ' ' << f->toDebugString()<<" result:"<<asAtomHandler::toDebugString(prop));
		}
	}
	if(checkPropertyException(obj,name,prop))
		return;
	replacelocalresult(context,instrptr->local3.pos,prop);
	obj->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_callFunctionNoArgs_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = *instrptr->arg1_constant;
	asAtom func = asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj2);
	asAtom ret;
	LOG_CALL("callFunctionNoArgs_c " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << " on " << asAtomHandler::toDebugString(obj));
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
	asAtomHandler::callFunction(func,context->worker,ret,obj,nullptr,0,false,false);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_callFunctionNoArgs_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	asAtom func = asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj2);
	asAtom ret;
	LOG_CALL("callFunctionNoArgs_l " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << " on " << asAtomHandler::toDebugString(obj));
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
	asAtomHandler::callFunction(func,context->worker,ret,obj,nullptr,0,false,false);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_callFunctionNoArgs_constant_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = *instrptr->arg1_constant;
	asAtom func = asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj2);
	LOG_CALL("callFunctionNoArgs_cl " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << " on " << asAtomHandler::toDebugString(obj));
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
	asAtomHandler::callFunction(func,context->worker,CONTEXT_GETLOCAL(context,instrptr->local3.pos),obj,nullptr,0,false,false);
	ASATOM_DECREF(oldres);
	++(context->exec_pos);
}
void ABCVm::abc_callFunctionNoArgs_local_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	asAtom func = asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj2);
	LOG_CALL("callFunctionNoArgs_ll " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << " on " << asAtomHandler::toDebugString(obj));
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
	asAtomHandler::callFunction(func,context->worker,CONTEXT_GETLOCAL(context,instrptr->local3.pos),obj,nullptr,0,false,false);
	ASATOM_DECREF(oldres);
	++(context->exec_pos);
}
void ABCVm::abc_callFunctionNoArgs_constant_setslotnocoerce(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = *instrptr->arg1_constant;
	asAtom func = asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj2);
	LOG_CALL("callFunctionNoArgs_cs " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << " on " << asAtomHandler::toDebugString(obj));
	asAtom res = asAtomHandler::invalidAtom;
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
	asAtomHandler::callFunction(func,context->worker,res,obj,nullptr,0,false,false);
	asAtom objslot = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(objslot)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}
void ABCVm::abc_callFunctionNoArgs_local_setslotnocoerce(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	asAtom func = asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj2);
	LOG_CALL("callFunctionNoArgs_ls " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << " on " << asAtomHandler::toDebugString(obj));
	asAtom res = asAtomHandler::invalidAtom;
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
	asAtomHandler::callFunction(func,context->worker,res,obj,nullptr,0,false,false);
	asAtom objslot = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(objslot)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}
void ABCVm::abc_callFunctionNoArgsVoid_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = *instrptr->arg1_constant;
	asAtom func = asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj2);
	LOG_CALL("callFunctionNoArgsVoid_c " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << " on " << asAtomHandler::toDebugString(obj));
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
	asAtomHandler::callFunction(func,context->worker,ret,obj,nullptr,0,false,false,false);
	ASATOM_DECREF(ret);
	++(context->exec_pos);
}
void ABCVm::abc_callFunctionNoArgsVoid_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	asAtom func = asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj2);
	LOG_CALL("callFunctionNoArgsVoid_l " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << " on " << asAtomHandler::toDebugString(obj));
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
	asAtomHandler::callFunction(func,context->worker,ret,obj,nullptr,0,false,false,false);
	ASATOM_DECREF(ret);
	++(context->exec_pos);
}
void ABCVm::abc_callFunctionSyntheticOneArgVoid_constant_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = *instrptr->arg1_constant;
	asAtom* value = instrptr->arg2_constant;
	asAtom func = asAtomHandler::fromObjectNoPrimitive((++context->exec_pos)->cacheobj3);
	LOG_CALL("callFunctionSyntheticOneArgVoid_cc " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));
	asAtom ret = asAtomHandler::invalidAtom;
	asAtomHandler::getObjectNoCheck(func)->as<SyntheticFunction>()->call(context->worker,ret,obj, value, 1,false,(instrptr->local3.flags&ABC_OP_COERCED)==0);
	ASATOM_DECREF(ret);
	if (context->exec_pos->cacheobj3->as<IFunction>()->clonedFrom)
		context->exec_pos->cacheobj3->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_callFunctionSyntheticOneArgVoid_local_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	asAtom* value = instrptr->arg2_constant;
	asAtom func = asAtomHandler::fromObjectNoPrimitive((++context->exec_pos)->cacheobj3);
	LOG_CALL("callFunctionSyntheticOneArgVoid_lc " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));
	ASATOM_INCREF(obj); // ensure that obj stays valid during call
	asAtom ret = asAtomHandler::invalidAtom;
	asAtomHandler::getObjectNoCheck(func)->as<SyntheticFunction>()->call(context->worker,ret,obj, value, 1,false,(instrptr->local3.flags&ABC_OP_COERCED)==0);
	ASATOM_DECREF(obj);
	ASATOM_DECREF(ret);
	if (context->exec_pos->cacheobj3->as<IFunction>()->clonedFrom)
		context->exec_pos->cacheobj3->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_callFunctionSyntheticOneArgVoid_constant_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = *instrptr->arg1_constant;
	asAtom* value = &CONTEXT_GETLOCAL(context,instrptr->local_pos2);
	asAtom func = asAtomHandler::fromObjectNoPrimitive((++context->exec_pos)->cacheobj3);
	LOG_CALL("callFunctionSyntheticOneArgVoid_cl " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));
	ASATOM_INCREF_POINTER(value);
	asAtom ret = asAtomHandler::invalidAtom;
	asAtomHandler::getObjectNoCheck(func)->as<SyntheticFunction>()->call(context->worker,ret,obj, value, 1,false,(instrptr->local3.flags&ABC_OP_COERCED)==0);
	ASATOM_DECREF(ret);
	if (context->exec_pos->cacheobj3->as<IFunction>()->clonedFrom)
		context->exec_pos->cacheobj3->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_callFunctionSyntheticOneArgVoid_local_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	asAtom* value = &CONTEXT_GETLOCAL(context,instrptr->local_pos2);
	asAtom func = asAtomHandler::fromObjectNoPrimitive((++context->exec_pos)->cacheobj3);
	LOG_CALL("callFunctionSyntheticOneArgVoid_ll " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));
	ASATOM_INCREF(obj); // ensure that obj stays valid during call
	ASATOM_INCREF_POINTER(value);
	asAtom ret = asAtomHandler::invalidAtom;
	asAtomHandler::getObjectNoCheck(func)->as<SyntheticFunction>()->call(context->worker,ret,obj, value, 1,false,(instrptr->local3.flags&ABC_OP_COERCED)==0);
	ASATOM_DECREF(obj);
	ASATOM_DECREF(ret);
	if (context->exec_pos->cacheobj3->as<IFunction>()->clonedFrom)
		context->exec_pos->cacheobj3->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_callFunctionBuiltinOneArgVoid_constant_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = *instrptr->arg1_constant;
	asAtom* value = instrptr->arg2_constant;
	asAtom func = asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj3);
	LOG_CALL("callFunctionBuiltinOneArgVoid_cc " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));
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
	asAtomHandler::getObjectNoCheck(func)->as<Function>()->call(ret,context->worker, obj, value, 1);
	ASATOM_DECREF(ret);
	if (instrptr->cacheobj3->as<IFunction>()->clonedFrom)
		instrptr->cacheobj3->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_callFunctionBuiltinOneArgVoid_local_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	asAtom* value = instrptr->arg2_constant;
	asAtom func = asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj3);
	LOG_CALL("callFunctionBuiltinOneArgVoid_lc " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));
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
	asAtomHandler::getObjectNoCheck(func)->as<Function>()->call(ret,context->worker, obj, value, 1);
	ASATOM_DECREF(ret);
	if (instrptr->cacheobj3->as<IFunction>()->clonedFrom)
		instrptr->cacheobj3->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_callFunctionBuiltinOneArgVoid_constant_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = *instrptr->arg1_constant;
	asAtom* value = &CONTEXT_GETLOCAL(context,instrptr->local_pos2);
	asAtom func = asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj3);
	LOG_CALL("callFunctionOneBuiltinArgVoid_cl " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));
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
	asAtomHandler::getObjectNoCheck(func)->as<Function>()->call(ret,context->worker, obj, value, 1);
	ASATOM_DECREF(ret);
	if (instrptr->cacheobj3->as<IFunction>()->clonedFrom)
		instrptr->cacheobj3->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_callFunctionBuiltinOneArgVoid_local_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	asAtom* value = &CONTEXT_GETLOCAL(context,instrptr->local_pos2);
	asAtom func = asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj3);
	LOG_CALL("callFunctionBuiltinOneArgVoid_ll " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));
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
	asAtomHandler::getObjectNoCheck(func)->as<Function>()->call(ret,context->worker, obj, value, 1);
	ASATOM_DECREF(ret);
	if (instrptr->cacheobj3->as<IFunction>()->clonedFrom)
		instrptr->cacheobj3->decRef();
	++(context->exec_pos);
}

void ABCVm::abc_callFunctionSyntheticOneArg_constant_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = *instrptr->arg1_constant;
	asAtom* value = instrptr->arg2_constant;
	asAtom func = asAtomHandler::fromObjectNoPrimitive((++context->exec_pos)->cacheobj3);
	LOG_CALL("callFunctionSyntheticOneArg_cc " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));
	asAtom ret = asAtomHandler::invalidAtom;
	asAtomHandler::getObjectNoCheck(func)->as<SyntheticFunction>()->call(context->worker,ret,obj, value, 1,false,(instrptr->local3.flags&ABC_OP_COERCED)==0);
	RUNTIME_STACK_PUSH(context,ret);
	if (context->exec_pos->cacheobj3->as<IFunction>()->clonedFrom)
		context->exec_pos->cacheobj3->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_callFunctionSyntheticOneArg_local_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	asAtom* value = instrptr->arg2_constant;
	asAtom func = asAtomHandler::fromObjectNoPrimitive((++context->exec_pos)->cacheobj3);
	LOG_CALL("callFunctionSyntheticOneArg_lc " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));
	ASATOM_INCREF(obj); // ensure that obj stays valid during call
	asAtom ret = asAtomHandler::invalidAtom;
	asAtomHandler::getObjectNoCheck(func)->as<SyntheticFunction>()->call(context->worker,ret,obj, value, 1,false,(instrptr->local3.flags&ABC_OP_COERCED)==0);
	ASATOM_DECREF(obj);
	RUNTIME_STACK_PUSH(context,ret);
	if (context->exec_pos->cacheobj3->as<IFunction>()->clonedFrom)
		context->exec_pos->cacheobj3->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_callFunctionSyntheticOneArg_constant_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = *instrptr->arg1_constant;
	asAtom* value = &CONTEXT_GETLOCAL(context,instrptr->local_pos2);
	asAtom func = asAtomHandler::fromObjectNoPrimitive((++context->exec_pos)->cacheobj3);
	LOG_CALL("callFunctionSyntheticOneArg_cl " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));
	ASATOM_INCREF_POINTER(value);
	asAtom ret = asAtomHandler::invalidAtom;
	asAtomHandler::getObjectNoCheck(func)->as<SyntheticFunction>()->call(context->worker,ret,obj, value, 1,false,(instrptr->local3.flags&ABC_OP_COERCED)==0);
	RUNTIME_STACK_PUSH(context,ret);
	if (context->exec_pos->cacheobj3->as<IFunction>()->clonedFrom)
		context->exec_pos->cacheobj3->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_callFunctionSyntheticOneArg_local_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	asAtom* value = &CONTEXT_GETLOCAL(context,instrptr->local_pos2);
	asAtom func = asAtomHandler::fromObjectNoPrimitive((++context->exec_pos)->cacheobj3);
	LOG_CALL("callFunctionSyntheticOneArg_ll " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));
	ASATOM_INCREF(obj); // ensure that obj stays valid during call
	ASATOM_INCREF_POINTER(value);
	asAtom ret = asAtomHandler::invalidAtom;
	asAtomHandler::getObjectNoCheck(func)->as<SyntheticFunction>()->call(context->worker,ret,obj, value, 1,false,(instrptr->local3.flags&ABC_OP_COERCED)==0);
	ASATOM_DECREF(obj);
	RUNTIME_STACK_PUSH(context,ret);
	if (context->exec_pos->cacheobj3->as<IFunction>()->clonedFrom)
		context->exec_pos->cacheobj3->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_callFunctionSyntheticOneArg_constant_constant_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = *instrptr->arg1_constant;
	asAtom* value = instrptr->arg2_constant;
	asAtom func = asAtomHandler::fromObjectNoPrimitive((++context->exec_pos)->cacheobj3);
	LOG_CALL("callFunctionSyntheticOneArg_ccl " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));
	asAtom oldres = CONTEXT_GETLOCAL(context,instrptr->local3.pos);
	asAtomHandler::getObjectNoCheck(func)->as<SyntheticFunction>()->call(context->worker,CONTEXT_GETLOCAL(context,instrptr->local3.pos),obj, value, 1,false,(instrptr->local3.flags&ABC_OP_COERCED)==0);
	ASATOM_DECREF(oldres);
	if (context->exec_pos->cacheobj3->as<IFunction>()->clonedFrom)
		context->exec_pos->cacheobj3->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_callFunctionSyntheticOneArg_local_constant_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	asAtom* value = instrptr->arg2_constant;
	asAtom func = asAtomHandler::fromObjectNoPrimitive((++context->exec_pos)->cacheobj3);
	LOG_CALL("callFunctionSyntheticOneArg_lcl " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));
	ASATOM_INCREF(obj); // ensure that obj stays valid during call
	asAtom oldres = CONTEXT_GETLOCAL(context,instrptr->local3.pos);
	asAtomHandler::getObjectNoCheck(func)->as<SyntheticFunction>()->call(context->worker,CONTEXT_GETLOCAL(context,instrptr->local3.pos),obj, value, 1,false,(instrptr->local3.flags&ABC_OP_COERCED)==0);
	ASATOM_DECREF(obj);
	ASATOM_DECREF(oldres);
	if (context->exec_pos->cacheobj3->as<IFunction>()->clonedFrom)
		context->exec_pos->cacheobj3->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_callFunctionSyntheticOneArg_constant_local_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = *instrptr->arg1_constant;
	asAtom* value = &CONTEXT_GETLOCAL(context,instrptr->local_pos2);
	asAtom func = asAtomHandler::fromObjectNoPrimitive((++context->exec_pos)->cacheobj3);
	LOG_CALL("callFunctionSyntheticOneArg_cll " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));
	ASATOM_INCREF_POINTER(value);
	asAtom oldres = CONTEXT_GETLOCAL(context,instrptr->local3.pos);
	asAtomHandler::getObjectNoCheck(func)->as<SyntheticFunction>()->call(context->worker,CONTEXT_GETLOCAL(context,instrptr->local3.pos),obj, value, 1,false,(instrptr->local3.flags&ABC_OP_COERCED)==0);
	ASATOM_DECREF(oldres);
	if (context->exec_pos->cacheobj3->as<IFunction>()->clonedFrom)
		context->exec_pos->cacheobj3->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_callFunctionSyntheticOneArg_local_local_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	asAtom* value = &CONTEXT_GETLOCAL(context,instrptr->local_pos2);
	asAtom func = asAtomHandler::fromObjectNoPrimitive((++context->exec_pos)->cacheobj3);
	LOG_CALL("callFunctionSyntheticOneArg_lll " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));
	ASATOM_INCREF(obj); // ensure that obj stays valid during call
	ASATOM_INCREF_POINTER(value);
	asAtom oldres = CONTEXT_GETLOCAL(context,instrptr->local3.pos);
	asAtomHandler::getObjectNoCheck(func)->as<SyntheticFunction>()->call(context->worker,CONTEXT_GETLOCAL(context,instrptr->local3.pos),obj, value, 1,false,(instrptr->local3.flags&ABC_OP_COERCED)==0);
	ASATOM_DECREF(obj);
	ASATOM_DECREF(oldres);
	if (context->exec_pos->cacheobj3->as<IFunction>()->clonedFrom)
		context->exec_pos->cacheobj3->decRef();
	++(context->exec_pos);
}

void ABCVm::abc_callFunctionBuiltinOneArg_constant_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = *instrptr->arg1_constant;
	asAtom* value = instrptr->arg2_constant;
	asAtom func = asAtomHandler::fromObjectNoPrimitive((++context->exec_pos)->cacheobj3);
	LOG_CALL("callFunctionBuiltinOneArg_cc " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));
	asAtom ret = asAtomHandler::invalidAtom;
	asAtomHandler::getObjectNoCheck(func)->as<Function>()->call(ret,context->worker, obj, value, 1);
	RUNTIME_STACK_PUSH(context,ret);
	if (context->exec_pos->cacheobj3->as<IFunction>()->clonedFrom)
		context->exec_pos->cacheobj3->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_callFunctionBuiltinOneArg_local_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	asAtom* value = instrptr->arg2_constant;
	asAtom func = asAtomHandler::fromObjectNoPrimitive((++context->exec_pos)->cacheobj3);
	LOG_CALL("callFunctionBuiltinOneArg_lc " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));
	asAtom ret = asAtomHandler::invalidAtom;
	asAtomHandler::getObjectNoCheck(func)->as<Function>()->call(ret,context->worker, obj, value, 1);
	RUNTIME_STACK_PUSH(context,ret);
	if (context->exec_pos->cacheobj3->as<IFunction>()->clonedFrom)
		context->exec_pos->cacheobj3->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_callFunctionBuiltinOneArg_constant_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = *instrptr->arg1_constant;
	asAtom* value = &CONTEXT_GETLOCAL(context,instrptr->local_pos2);
	asAtom func = asAtomHandler::fromObjectNoPrimitive((++context->exec_pos)->cacheobj3);
	LOG_CALL("callFunctionOneBuiltinArg_cl " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));
	asAtom ret = asAtomHandler::invalidAtom;
	asAtomHandler::getObjectNoCheck(func)->as<Function>()->call(ret,context->worker, obj, value, 1);
	RUNTIME_STACK_PUSH(context,ret);
	if (context->exec_pos->cacheobj3->as<IFunction>()->clonedFrom)
		context->exec_pos->cacheobj3->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_callFunctionBuiltinOneArg_local_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	asAtom* value = &CONTEXT_GETLOCAL(context,instrptr->local_pos2);
	asAtom func = asAtomHandler::fromObjectNoPrimitive((++context->exec_pos)->cacheobj3);
	LOG_CALL("callFunctionOneBuiltinArg_ll " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));
	asAtom ret = asAtomHandler::invalidAtom;
	asAtomHandler::getObjectNoCheck(func)->as<Function>()->call(ret,context->worker, obj, value, 1);
	RUNTIME_STACK_PUSH(context,ret);
	if (context->exec_pos->cacheobj3->as<IFunction>()->clonedFrom)
		context->exec_pos->cacheobj3->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_callFunctionBuiltinOneArg_constant_constant_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = *instrptr->arg1_constant;
	asAtom* value = instrptr->arg2_constant;
	asAtom func = asAtomHandler::fromObjectNoPrimitive((++context->exec_pos)->cacheobj3);
	LOG_CALL("callFunctionBuiltinOneArg_ccl " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));
	asAtom oldres = CONTEXT_GETLOCAL(context,instrptr->local3.pos);
	asAtomHandler::getObjectNoCheck(func)->as<Function>()->call(CONTEXT_GETLOCAL(context,instrptr->local3.pos),context->worker, obj, value, 1);
	ASATOM_DECREF(oldres);
	if (context->exec_pos->cacheobj3->as<IFunction>()->clonedFrom)
		context->exec_pos->cacheobj3->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_callFunctionBuiltinOneArg_local_constant_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	asAtom* value = instrptr->arg2_constant;
	asAtom func = asAtomHandler::fromObjectNoPrimitive((++context->exec_pos)->cacheobj3);
	LOG_CALL("callFunctionBuiltinOneArg_lcl " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));
	asAtom oldres = CONTEXT_GETLOCAL(context,instrptr->local3.pos);
	asAtomHandler::getObjectNoCheck(func)->as<Function>()->call(CONTEXT_GETLOCAL(context,instrptr->local3.pos),context->worker, obj, value, 1);
	ASATOM_DECREF(oldres);
	if (context->exec_pos->cacheobj3->as<IFunction>()->clonedFrom)
		context->exec_pos->cacheobj3->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_callFunctionBuiltinOneArg_constant_local_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = *instrptr->arg1_constant;
	asAtom* value = &CONTEXT_GETLOCAL(context,instrptr->local_pos2);
	asAtom func = asAtomHandler::fromObjectNoPrimitive((++context->exec_pos)->cacheobj3);
	LOG_CALL("callFunctionOneBuiltinArg_cll " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));
	asAtom oldres = CONTEXT_GETLOCAL(context,instrptr->local3.pos);
	asAtomHandler::getObjectNoCheck(func)->as<Function>()->call(CONTEXT_GETLOCAL(context,instrptr->local3.pos),context->worker, obj, value, 1);
	ASATOM_DECREF(oldres);
	if (context->exec_pos->cacheobj3->as<IFunction>()->clonedFrom)
		context->exec_pos->cacheobj3->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_callFunctionBuiltinOneArg_local_local_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	asAtom* value = &CONTEXT_GETLOCAL(context,instrptr->local_pos2);
	asAtom func = asAtomHandler::fromObjectNoPrimitive((++context->exec_pos)->cacheobj3);
	LOG_CALL("callFunctionOneBuiltinArg_lll " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));
	asAtom oldres = CONTEXT_GETLOCAL(context,instrptr->local3.pos);
	asAtomHandler::getObjectNoCheck(func)->as<Function>()->call(CONTEXT_GETLOCAL(context,instrptr->local3.pos),context->worker, obj, value, 1);
	ASATOM_DECREF(oldres);
	if (context->exec_pos->cacheobj3->as<IFunction>()->clonedFrom)
		context->exec_pos->cacheobj3->decRef();
	++(context->exec_pos);
}



void ABCVm::abc_callFunctionSyntheticMultiArgsVoid(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t argcount = instrptr->local2.pos;
	SyntheticFunction* func = instrptr->cacheobj3->as<SyntheticFunction>();
	LOG_CALL("callFunctionSyntheticMultiArgsVoid " << func->getSystemState()->getStringFromUniqueId(func->functionname) << ' ' <<argcount);
	RUNTIME_STACK_POP_N_CREATE(context,argcount+1,args);
	asAtom ret;
	func->call(context->worker,ret,*args, args+1, argcount,false,(instrptr->local2.flags&ABC_OP_COERCED)==0);
	ASATOM_DECREF(ret);
	ASATOM_DECREF_POINTER(args);
	if (instrptr->cacheobj3->as<IFunction>()->clonedFrom)
		instrptr->cacheobj3->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_callFunctionSyntheticMultiArgsVoid_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = *instrptr->arg1_constant;
	uint32_t argcount = instrptr->local2.pos;
	asAtom func = asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj3);
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
	if (instrptr->cacheobj3->as<IFunction>()->clonedFrom)
		instrptr->cacheobj3->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_callFunctionSyntheticMultiArgsVoid_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	uint32_t argcount = instrptr->local2.pos;
	asAtom func = asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj3);
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
	if (instrptr->cacheobj3->as<IFunction>()->clonedFrom)
		instrptr->cacheobj3->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_callFunctionSyntheticMultiArgs(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t argcount = instrptr->local2.pos;
	SyntheticFunction* func = instrptr->cacheobj3->as<SyntheticFunction>();
	LOG_CALL("callFunctionSyntheticMultiArgs " << func->getSystemState()->getStringFromUniqueId(func->functionname) << ' ' <<argcount);
	RUNTIME_STACK_POP_N_CREATE(context,argcount+1,args);
	asAtom ret;
	func->call(context->worker,ret,*args, args+1, argcount,false,(instrptr->local2.flags&ABC_OP_COERCED)==0);
	ASATOM_DECREF_POINTER(args);
	RUNTIME_STACK_PUSH(context,ret);
	if (instrptr->cacheobj3->as<IFunction>()->clonedFrom)
		instrptr->cacheobj3->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_callFunctionSyntheticMultiArgs_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = *instrptr->arg1_constant;
	uint32_t argcount = instrptr->local2.pos;
	asAtom func = asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj3);
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
	asAtom ret;
	asAtomHandler::getObjectNoCheck(func)->as<SyntheticFunction>()->call(context->worker,ret,obj, args, argcount,false,(instrptr->local2.flags&ABC_OP_COERCED)==0);
	RUNTIME_STACK_PUSH(context,ret);
	if (instrptr->cacheobj3->as<IFunction>()->clonedFrom)
		instrptr->cacheobj3->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_callFunctionSyntheticMultiArgs_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	uint32_t argcount = instrptr->local2.pos;
	asAtom func = asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj3);
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
	asAtom ret;
	asAtomHandler::getObjectNoCheck(func)->as<SyntheticFunction>()->call(context->worker,ret,obj, args, argcount,false,(instrptr->local2.flags&ABC_OP_COERCED)==0);
	RUNTIME_STACK_PUSH(context,ret);
	if (instrptr->cacheobj3->as<IFunction>()->clonedFrom)
		instrptr->cacheobj3->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_callFunctionSyntheticMultiArgs_constant_localResult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = *instrptr->arg1_constant;
	uint32_t argcount = instrptr->local2.pos;
	asAtom func = asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj3);
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
	asAtomHandler::getObjectNoCheck(func)->as<SyntheticFunction>()->call(context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),obj, args, argcount,false,(instrptr->local2.flags&ABC_OP_COERCED)==0);
	ASATOM_DECREF(oldres);
	if (instrptr->cacheobj3->as<IFunction>()->clonedFrom)
		instrptr->cacheobj3->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_callFunctionSyntheticMultiArgs_local_localResult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	uint32_t argcount = instrptr->local2.pos;
	asAtom func = asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj3);
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
	asAtomHandler::getObjectNoCheck(func)->as<SyntheticFunction>()->call(context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),obj, args, argcount,false,(instrptr->local2.flags&ABC_OP_COERCED)==0);
	ASATOM_DECREF(oldres);
	if (instrptr->cacheobj3->as<IFunction>()->clonedFrom)
		instrptr->cacheobj3->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_callFunctionBuiltinMultiArgsVoid_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = *instrptr->arg1_constant;
	uint32_t argcount = instrptr->local2.pos;
	asAtom func = asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj3);
	LOG_CALL("callFunctionBuiltinMultiArgsVoid_c " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" "<<argcount);
	asAtom* args = g_newa(asAtom,argcount);
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
	if (instrptr->cacheobj3->as<IFunction>()->clonedFrom)
		instrptr->cacheobj3->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_callFunctionBuiltinMultiArgsVoid_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	uint32_t argcount = instrptr->local2.pos;
	asAtom func = asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj3);
	LOG_CALL("callFunctionBuiltinMultiArgsVoid_l " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" "<<argcount);
	asAtom* args = g_newa(asAtom,argcount);
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
	if (instrptr->cacheobj3->as<IFunction>()->clonedFrom)
		instrptr->cacheobj3->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_callFunctionBuiltinMultiArgs_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = *instrptr->arg1_constant;
	uint32_t argcount = instrptr->local2.pos;
	asAtom func = asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj3);
	LOG_CALL("callFunctionBuiltinMultiArgs_c " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" "<<argcount);
	asAtom* args = g_newa(asAtom,argcount);
	for (uint32_t i = argcount; i > 0 ; i--)
	{
		(++(context->exec_pos));
		if (context->exec_pos->arg2_uint == OPERANDTYPES::OP_LOCAL || context->exec_pos->arg2_uint == OPERANDTYPES::OP_CACHED_SLOT)
			args[i-1] = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
		else
			args[i-1] = *context->exec_pos->arg1_constant;
	}
	asAtom ret;
	asAtomHandler::getObjectNoCheck(func)->as<Function>()->call(ret,context->worker,obj, args, argcount);
	RUNTIME_STACK_PUSH(context,ret);
	if (instrptr->cacheobj3->as<IFunction>()->clonedFrom)
		instrptr->cacheobj3->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_callFunctionBuiltinMultiArgs_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	uint32_t argcount = instrptr->local2.pos;
	asAtom func = asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj3);
	LOG_CALL("callFunctionBuiltinMultiArgs_l " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" "<<argcount);
	asAtom* args = g_newa(asAtom,argcount);
	for (uint32_t i = argcount; i > 0 ; i--)
	{
		(++(context->exec_pos));
		if (context->exec_pos->arg2_uint == OPERANDTYPES::OP_LOCAL || context->exec_pos->arg2_uint == OPERANDTYPES::OP_CACHED_SLOT)
			args[i-1] = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
		else
			args[i-1] = *context->exec_pos->arg1_constant;
	}
	asAtom ret;
	asAtomHandler::getObjectNoCheck(func)->as<Function>()->call(ret,context->worker,obj, args, argcount);
	RUNTIME_STACK_PUSH(context,ret);
	if (instrptr->cacheobj3->as<IFunction>()->clonedFrom)
		instrptr->cacheobj3->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_callFunctionBuiltinMultiArgs_constant_localResult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = *instrptr->arg1_constant;
	uint32_t argcount = instrptr->local2.pos;
	asAtom func = asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj3);
	LOG_CALL("callFunctionBuiltinMultiArgs_c_lr " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" "<<argcount);
	asAtom* args = g_newa(asAtom,argcount);
	for (uint32_t i = argcount; i > 0 ; i--)
	{
		(++(context->exec_pos));
		if (context->exec_pos->arg2_uint == OPERANDTYPES::OP_LOCAL || context->exec_pos->arg2_uint == OPERANDTYPES::OP_CACHED_SLOT)
			args[i-1] = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
		else
			args[i-1] = *context->exec_pos->arg1_constant;
	}
	asAtom ret;
	asAtomHandler::getObjectNoCheck(func)->as<Function>()->call(ret,context->worker,obj, args, argcount);
	replacelocalresult(context,context->exec_pos->local3.pos,ret);
	if (instrptr->cacheobj3->as<IFunction>()->clonedFrom)
		instrptr->cacheobj3->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_callFunctionBuiltinMultiArgs_local_localResult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	uint32_t argcount = instrptr->local2.pos;
	asAtom func = asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj3);
	LOG_CALL("callFunctionBuiltinMultiArgs_l_lr " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" "<<argcount);
	asAtom* args = g_newa(asAtom,argcount);
	for (uint32_t i = argcount; i > 0 ; i--)
	{
		(++(context->exec_pos));
		if (context->exec_pos->arg2_uint == OPERANDTYPES::OP_LOCAL || context->exec_pos->arg2_uint == OPERANDTYPES::OP_CACHED_SLOT)
			args[i-1] = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
		else
			args[i-1] = *context->exec_pos->arg1_constant;
	}
	asAtom ret;
	asAtomHandler::getObjectNoCheck(func)->as<Function>()->call(ret,context->worker,obj, args, argcount);
	replacelocalresult(context,context->exec_pos->local3.pos,ret);
	if (instrptr->cacheobj3->as<IFunction>()->clonedFrom)
		instrptr->cacheobj3->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_getslot_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos++;
	uint32_t t = instrptr->arg2_uint;
	asAtom* pval = instrptr->arg1_constant;
	asAtom ret=asAtomHandler::getObject(*pval)->getSlotNoCheck(t);
	LOG_CALL("getSlot_c " << t << " " << asAtomHandler::toDebugString(ret));
	ASATOM_INCREF(ret);
	RUNTIME_STACK_PUSH(context,ret);
}
void ABCVm::abc_getslot_local(call_context* context)
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
void ABCVm::abc_getslot_constant_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos++;
	uint32_t t = instrptr->arg2_uint;
	asAtom res = asAtomHandler::getObject(*instrptr->arg1_constant)->getSlotNoCheck(t);
	ASATOM_INCREF(res);
	replacelocalresult(context,instrptr->local3.pos,res);
	LOG_CALL("getSlot_cl " << t << " " << asAtomHandler::toDebugString(res));
}
void ABCVm::abc_getslot_local_localresult(call_context* context)
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
		ASATOM_INCREF(res);
		replacelocalresult(context,instrptr->local3.pos,res);
	}
	LOG_CALL("getSlot_ll " << t << " " <<instrptr->local_pos1<<":"<< asAtomHandler::toDebugString(CONTEXT_GETLOCAL(context,instrptr->local_pos1))<<" "<< asAtomHandler::toDebugString(CONTEXT_GETLOCAL(context,instrptr->local3.pos)));
}
void ABCVm::abc_getslot_constant_setslotnocoerce(call_context* context)
{
	uint32_t tget = context->exec_pos->arg2_uint;
	asAtom res = asAtomHandler::getObject(*context->exec_pos->arg1_constant)->getSlotNoCheck(tget);
	LOG_CALL("getSlot_cs " << tget << " " << asAtomHandler::toDebugString(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos)));

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}
void ABCVm::abc_getslot_local_setslotnocoerce(call_context* context)
{
	uint32_t tget = context->exec_pos->arg2_uint;
	if (!asAtomHandler::isObject(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1)))
	{
		createError<TypeError>(context->worker,kConvertNullToObjectError);
		return;
	}
	ASObject* objget = asAtomHandler::getObjectNoCheck(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1));
	asAtom res = objget->getSlotNoCheck(tget);
	LOG_CALL("getSlot_ls " << tget << " " <<context->exec_pos->local_pos1<<":"<< asAtomHandler::toDebugString(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1))<<" "<< asAtomHandler::toDebugString(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos)));

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}
void ABCVm::abc_setslot_constant_constant(call_context* context)
{
	asAtom v1 = *context->exec_pos->arg1_constant;
	asAtom v2 = *context->exec_pos->arg2_constant;
	uint32_t t = context->exec_pos->arg3_uint;
	LOG_CALL("setSlot_cc " << t << " "<< asAtomHandler::toDebugString(v2) << " "<< asAtomHandler::toDebugString(v1));
	if (asAtomHandler::getObject(v1)->setSlot(context->worker,t,v2))
		ASATOM_INCREF(v2);
	++(context->exec_pos);
}
void ABCVm::abc_setslot_local_constant(call_context* context)
{
	asAtom v1 = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	asAtom v2 = *context->exec_pos->arg2_constant;
	uint32_t t = context->exec_pos->arg3_uint;
	LOG_CALL("setSlot_lc " << t << " "<< asAtomHandler::toDebugString(v2) << " "<< asAtomHandler::toDebugString(v1));
	if (asAtomHandler::getObject(v1)->setSlot(context->worker,t,v2))
		ASATOM_INCREF(v2);
	++(context->exec_pos);
}
void ABCVm::abc_setslot_constant_local(call_context* context)
{
	asAtom v1 = *context->exec_pos->arg1_constant;
	asAtom v2 = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2);
	uint32_t t = context->exec_pos->arg3_uint;
	LOG_CALL("setSlot_cl " << t << " "<< asAtomHandler::toDebugString(v2) << " "<< asAtomHandler::toDebugString(v1));
	if (asAtomHandler::getObject(v1)->setSlot(context->worker,t,v2))
		ASATOM_INCREF(v2);
	++(context->exec_pos);
}
void ABCVm::abc_setslot_local_local(call_context* context)
{
	asAtom v1 = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	asAtom v2 = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2);
	uint32_t t = context->exec_pos->arg3_uint;
	LOG_CALL("setSlot_ll " << t << " "<< asAtomHandler::toDebugString(v2) << " "<< asAtomHandler::toDebugString(v1));
	if (asAtomHandler::getObject(v1)->setSlot(context->worker,t,v2))
		ASATOM_INCREF(v2);
	++(context->exec_pos);
}
void ABCVm::abc_setslotNoCoerce_constant_constant(call_context* context)
{
	asAtom v1 = *context->exec_pos->arg1_constant;
	asAtom v2 = *context->exec_pos->arg2_constant;
	uint32_t t = context->exec_pos->arg3_uint;
	LOG_CALL("setSlotNoCoerce_cc " << t << " "<< asAtomHandler::toDebugString(v2) << " "<< asAtomHandler::toDebugString(v1));
	asAtomHandler::getObjectNoCheck(v1)->setSlotNoCoerce(t,v2);
	++(context->exec_pos);
}
void ABCVm::abc_setslotNoCoerce_local_constant(call_context* context)
{
	asAtom v1 = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	asAtom v2 = *context->exec_pos->arg2_constant;
	uint32_t t = context->exec_pos->arg3_uint;
	LOG_CALL("setSlotNoCoerce_lc " << t << " "<< asAtomHandler::toDebugString(v2) << " "<< asAtomHandler::toDebugString(v1));
	asAtomHandler::getObjectNoCheck(v1)->setSlotNoCoerce(t,v2);
	++(context->exec_pos);
}
void ABCVm::abc_setslotNoCoerce_constant_local(call_context* context)
{
	asAtom v1 = *context->exec_pos->arg1_constant;
	asAtom v2 = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2);
	uint32_t t = context->exec_pos->arg3_uint;
	LOG_CALL("setSlotNoCoerce_cl " << t << " "<< asAtomHandler::toDebugString(v2) << " "<< asAtomHandler::toDebugString(v1));
	asAtomHandler::getObjectNoCheck(v1)->setSlotNoCoerce(t,v2);
	++(context->exec_pos);
}
void ABCVm::abc_setslotNoCoerce_local_local(call_context* context)
{
	asAtom v1 = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	asAtom v2 = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2);
	uint32_t t = context->exec_pos->arg3_uint;
	LOG_CALL("setSlotNoCoerce_ll " << t << " "<< asAtomHandler::toDebugString(v2) << " "<< asAtomHandler::toDebugString(v1));
	asAtomHandler::getObjectNoCheck(v1)->setSlotNoCoerce(t,v2);
	++(context->exec_pos);
}
void ABCVm::abc_convert_i_constant(call_context* context)
{
	LOG_CALL("convert_i_c");
	asAtom res = *context->exec_pos->arg1_constant;
	if(!asAtomHandler::isInteger(res))
	{
		int32_t v= asAtomHandler::toIntStrict(res);
		asAtomHandler::setInt(res,context->worker,v);
	}
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_convert_i_local(call_context* context)
{
	LOG_CALL("convert_i_l:"<<asAtomHandler::toDebugString(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1)));
	asAtom res =CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	if(!asAtomHandler::isInteger(res))
	{
		int32_t v= asAtomHandler::toIntStrict(res);
		asAtomHandler::setInt(res,context->worker,v);
	}
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_convert_i_constant_localresult(call_context* context)
{
	LOG_CALL("convert_i_cl");
	asAtom res = *context->exec_pos->arg1_constant;
	if(!asAtomHandler::isInteger(res))
	{
		int32_t v= asAtomHandler::toIntStrict(res);
		asAtomHandler::setInt(res,context->worker,v);
	}
	replacelocalresult(context,context->exec_pos->local3.pos,res);
	++(context->exec_pos);
}
void ABCVm::abc_convert_i_local_localresult(call_context* context)
{
	LOG_CALL("convert_i_ll");
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	if(!asAtomHandler::isInteger(res))
	{
		int32_t v= asAtomHandler::toIntStrict(res);
		asAtomHandler::setInt(res,context->worker,v);
	}
	replacelocalresult(context,context->exec_pos->local3.pos,res);
	++(context->exec_pos);
}
void ABCVm::abc_convert_i_constant_setslotnocoerce(call_context* context)
{
	LOG_CALL("convert_i_cs");
	asAtom res = *context->exec_pos->arg1_constant;
	if(!asAtomHandler::isInteger(res)) {
		int32_t v= asAtomHandler::toIntStrict(res);
		asAtomHandler::setInt(res,context->worker,v);
	}
	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}
void ABCVm::abc_convert_i_local_setslotnocoerce(call_context* context)
{
	LOG_CALL("convert_i_ls");
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	if(!asAtomHandler::isInteger(res)) {
		int32_t v= asAtomHandler::toIntStrict(res);
		asAtomHandler::setInt(res,context->worker,v);
	}
	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}

void ABCVm::abc_convert_u_constant(call_context* context)
{
	LOG_CALL("convert_u_c");
	asAtom res = *context->exec_pos->arg1_constant;
	if(!asAtomHandler::isUInteger(res))
	{
		int32_t v= asAtomHandler::toUInt(res);
		asAtomHandler::setUInt(res,context->worker,v);
	}
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_convert_u_local(call_context* context)
{
	LOG_CALL("convert_u_l:"<<asAtomHandler::toDebugString(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1)));
	asAtom res =CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	if(!asAtomHandler::isUInteger(res))
	{
		int32_t v= asAtomHandler::toUInt(res);
		asAtomHandler::setUInt(res,context->worker,v);
	}
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_convert_u_constant_localresult(call_context* context)
{
	LOG_CALL("convert_u_cl");
	asAtom res = *context->exec_pos->arg1_constant;
	if(!asAtomHandler::isUInteger(res))
	{
		int32_t v= asAtomHandler::toUInt(res);
		asAtomHandler::setUInt(res,context->worker,v);
	}
	replacelocalresult(context,context->exec_pos->local3.pos,res);
	++(context->exec_pos);
}
void ABCVm::abc_convert_u_local_localresult(call_context* context)
{
	LOG_CALL("convert_u_ll");
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	if(!asAtomHandler::isUInteger(res))
	{
		int32_t v= asAtomHandler::toUInt(res);
		asAtomHandler::setUInt(res,context->worker,v);
	}
	replacelocalresult(context,context->exec_pos->local3.pos,res);
	++(context->exec_pos);
}
void ABCVm::abc_convert_u_constant_setslotnocoerce(call_context* context)
{
	LOG_CALL("convert_u_cs");
	asAtom res = *context->exec_pos->arg1_constant;
	if(!asAtomHandler::isUInteger(res)) {
		int32_t v= asAtomHandler::toUInt(res);
		asAtomHandler::setUInt(res,context->worker,v);
	}
	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}
void ABCVm::abc_convert_u_local_setslotnocoerce(call_context* context)
{
	LOG_CALL("convert_u_ls");
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	if(!asAtomHandler::isUInteger(res)) {
		int32_t v= asAtomHandler::toUInt(res);
		asAtomHandler::setUInt(res,context->worker,v);
	}
	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}
void ABCVm::abc_convert_d_constant(call_context* context)
{
	LOG_CALL("convert_d_c");
	asAtom res = *context->exec_pos->arg1_constant;
	if(!asAtomHandler::isNumeric(res))
		asAtomHandler::setNumber(res,context->worker,asAtomHandler::toNumber(res));
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_convert_d_local(call_context* context)
{
	LOG_CALL("convert_d_l:"<<asAtomHandler::toDebugString(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1)));
	asAtom res =CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	if(!asAtomHandler::isNumeric(res))
		asAtomHandler::setNumber(res,context->worker,asAtomHandler::toNumber(res));
	else
		ASATOM_INCREF(res);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_convert_d_constant_localresult(call_context* context)
{
	LOG_CALL("convert_d_cl");
	asAtom res = *context->exec_pos->arg1_constant;
	if(!asAtomHandler::isNumeric(res))
		asAtomHandler::setNumber(res,context->worker,asAtomHandler::toNumber(res));
	else
		ASATOM_INCREF(res);
	replacelocalresult(context,context->exec_pos->local3.pos,res);
	++(context->exec_pos);
}
void ABCVm::abc_convert_d_local_localresult(call_context* context)
{
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	LOG_CALL("convert_d_ll "<<asAtomHandler::toDebugString(res)<<" "<<asAtomHandler::getObject(res)<<" "<< context->exec_pos->local_pos1<<" "<<(context->exec_pos->local3.pos));
	if(!asAtomHandler::isNumeric(res))
		asAtomHandler::setNumber(res,context->worker,asAtomHandler::toNumber(res));
	else
		ASATOM_INCREF(res);
	replacelocalresult(context,context->exec_pos->local3.pos,res);
	++(context->exec_pos);
}
void ABCVm::abc_convert_d_constant_setslotnocoerce(call_context* context)
{
	LOG_CALL("convert_d_cs");
	asAtom res = *context->exec_pos->arg1_constant;
	if(!asAtomHandler::isNumeric(res))
		asAtomHandler::setNumber(res,context->worker,asAtomHandler::toNumber(res));
	else
		ASATOM_INCREF(res);
	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}
void ABCVm::abc_convert_d_local_setslotnocoerce(call_context* context)
{
	LOG_CALL("convert_d_ls");
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	if(!asAtomHandler::isNumeric(res))
		asAtomHandler::setNumber(res,context->worker,asAtomHandler::toNumber(res));
	else
		ASATOM_INCREF(res);
	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}
void ABCVm::abc_convert_b_constant(call_context* context)
{
	LOG_CALL("convert_b_c");
	asAtom res = *context->exec_pos->arg1_constant;
	if(!asAtomHandler::isBool(res))
		asAtomHandler::convert_b(res,false);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_convert_b_local(call_context* context)
{
	LOG_CALL("convert_b_l:"<<asAtomHandler::toDebugString(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1)));
	asAtom res =CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	if(!asAtomHandler::isBool(res))
		asAtomHandler::convert_b(res,false);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_convert_b_constant_localresult(call_context* context)
{
	LOG_CALL("convert_b_cl");
	asAtom res = *context->exec_pos->arg1_constant;
	if(!asAtomHandler::isBool(res))
		asAtomHandler::convert_b(res,false);
	replacelocalresult(context,context->exec_pos->local3.pos,res);
	++(context->exec_pos);
}
void ABCVm::abc_convert_b_local_localresult(call_context* context)
{
	LOG_CALL("convert_b_ll");
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	if(!asAtomHandler::isBool(res))
		asAtomHandler::convert_b(res,false);
	replacelocalresult(context,context->exec_pos->local3.pos,res);
	++(context->exec_pos);
}
void ABCVm::abc_convert_b_constant_setslotnocoerce(call_context* context)
{
	LOG_CALL("convert_b_cs");
	asAtom res = *context->exec_pos->arg1_constant;
	if(!asAtomHandler::isBool(res))
		asAtomHandler::convert_b(res,false);
	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}
void ABCVm::abc_convert_b_local_setslotnocoerce(call_context* context)
{
	LOG_CALL("convert_b_ls");
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	if(!asAtomHandler::isBool(res))
		asAtomHandler::convert_b(res,false);
	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}

void ABCVm::abc_astypelate_constant_constant(call_context* context)
{
	LOG_CALL("astypelate_cc");
	asAtom ret = asAtomHandler::asTypelate(*context->exec_pos->arg1_constant,*context->exec_pos->arg2_constant,context->worker);
	ASATOM_INCREF(ret);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_astypelate_local_constant(call_context* context)
{
	LOG_CALL("astypelate_lc");
	asAtom ret = asAtomHandler::asTypelate(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),*context->exec_pos->arg2_constant,context->worker);
	ASATOM_INCREF(ret);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_astypelate_constant_local(call_context* context)
{
	LOG_CALL("astypelate_cl");
	asAtom ret = asAtomHandler::asTypelate(*context->exec_pos->arg1_constant,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->worker);
	ASATOM_INCREF(ret);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_astypelate_local_local(call_context* context)
{
	LOG_CALL("astypelate_ll");
	asAtom ret = asAtomHandler::asTypelate(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->worker);
	ASATOM_INCREF(ret);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_astypelate_constant_constant_localresult(call_context* context)
{
	LOG_CALL("astypelate_ccl");
	asAtom ret = asAtomHandler::asTypelate(*context->exec_pos->arg1_constant,*context->exec_pos->arg2_constant,context->worker);
	ASATOM_INCREF(ret);
	replacelocalresult(context,context->exec_pos->local3.pos,ret);
	++(context->exec_pos);
}
void ABCVm::abc_astypelate_local_constant_localresult(call_context* context)
{
	LOG_CALL("astypelate_lcl");
	asAtom ret = asAtomHandler::asTypelate(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),*context->exec_pos->arg2_constant,context->worker);
	ASATOM_INCREF(ret);
	replacelocalresult(context,context->exec_pos->local3.pos,ret);
	++(context->exec_pos);
}
void ABCVm::abc_astypelate_constant_local_localresult(call_context* context)
{
	LOG_CALL("astypelate_cll");
	asAtom ret = asAtomHandler::asTypelate(*context->exec_pos->arg1_constant,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->worker);
	ASATOM_INCREF(ret);
	replacelocalresult(context,context->exec_pos->local3.pos,ret);
	++(context->exec_pos);
}
void ABCVm::abc_astypelate_local_local_localresult(call_context* context)
{
	LOG_CALL("astypelate_lll");
	asAtom ret = asAtomHandler::asTypelate(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->worker);
	ASATOM_INCREF(ret);
	replacelocalresult(context,context->exec_pos->local3.pos,ret);
	++(context->exec_pos);
}
void ABCVm::abc_astypelate_constant_constant_setslotnocoerce(call_context* context)
{
	LOG_CALL("astypelate_ccs");
	asAtom res = asAtomHandler::asTypelate(*context->exec_pos->arg1_constant,*context->exec_pos->arg2_constant,context->worker);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}
void ABCVm::abc_astypelate_local_constant_setslotnocoerce(call_context* context)
{
	LOG_CALL("astypelate_lcs");
	asAtom res = asAtomHandler::asTypelate(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),*context->exec_pos->arg2_constant,context->worker);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}
void ABCVm::abc_astypelate_constant_local_setslotnocoerce(call_context* context)
{
	LOG_CALL("astypelate_cls");
	asAtom res = asAtomHandler::asTypelate(*context->exec_pos->arg1_constant,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->worker);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}
void ABCVm::abc_astypelate_local_local_setslotnocoerce(call_context* context)
{
	LOG_CALL("astypelate_lls");
	asAtom res = asAtomHandler::asTypelate(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->worker);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}

void ABCVm::abc_increment_local(call_context* context)
{
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	LOG_CALL("increment_l "<<context->exec_pos->local_pos1<<" "<<asAtomHandler::toDebugString(res));
	asAtomHandler::increment(res,context->worker);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_increment_local_localresult(call_context* context)
{
	LOG_CALL("increment_ll "<<context->exec_pos->local_pos1<<" "<<context->exec_pos->local3.pos<<" "<<asAtomHandler::toDebugString(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos)));
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	if (context->exec_pos->local3.pos != context->exec_pos->local_pos1)
		ASATOM_DECREF(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos));
	asAtomHandler::set(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),res);
	asAtomHandler::increment(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),context->worker);
	++(context->exec_pos);
}
void ABCVm::abc_decrement_local(call_context* context)
{
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	LOG_CALL("decrement_l "<<context->exec_pos->local_pos1<<" "<<asAtomHandler::toDebugString(res));
	asAtomHandler::decrement(res,context->worker);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_decrement_local_localresult(call_context* context)
{
	LOG_CALL("decrement_ll "<<context->exec_pos->local_pos1<<" "<<context->exec_pos->local3.pos<<" "<<asAtomHandler::toDebugString(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos)));
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	if (context->exec_pos->local3.pos != context->exec_pos->local_pos1)
		ASATOM_DECREF(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos));
	asAtomHandler::set(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),res);
	asAtomHandler::decrement(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),context->worker);
	++(context->exec_pos);
}
void ABCVm::abc_typeof_constant(call_context* context)
{
	LOG_CALL("typeof_c");
	asAtom res = asAtomHandler::typeOf(*context->exec_pos->arg1_constant);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_typeof_local(call_context* context)
{
	LOG_CALL("typeof_l "<<asAtomHandler::toDebugString(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1)));
	asAtom res = asAtomHandler::typeOf(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1));
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_typeof_constant_localresult(call_context* context)
{
	LOG_CALL("typeof_cl");
	asAtom res = asAtomHandler::typeOf(*context->exec_pos->arg1_constant);
	replacelocalresult(context,context->exec_pos->local3.pos,res);
	++(context->exec_pos);
}
void ABCVm::abc_typeof_local_localresult(call_context* context)
{
	LOG_CALL("typeof_ll");
	asAtom res = asAtomHandler::typeOf(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1));
	replacelocalresult(context,context->exec_pos->local3.pos,res);
	++(context->exec_pos);
}void ABCVm::abc_not_constant(call_context* context)
{
	LOG_CALL("not_c");
	asAtom res = asAtomHandler::fromBool(!asAtomHandler::Boolean_concrete(*context->exec_pos->arg1_constant));
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_not_local(call_context* context)
{
	LOG_CALL("not_l "<<asAtomHandler::toDebugString(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1)));
	asAtom res = asAtomHandler::fromBool(!asAtomHandler::Boolean_concrete(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1)));
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_not_constant_localresult(call_context* context)
{
	LOG_CALL("not_cl");
	bool res = !asAtomHandler::Boolean_concrete(*context->exec_pos->arg1_constant);
	ASATOM_DECREF(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos));
	asAtomHandler::setBool(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),res);
	++(context->exec_pos);
}
void ABCVm::abc_not_local_localresult(call_context* context)
{
	LOG_CALL("not_ll");
	bool res = !asAtomHandler::Boolean_concrete(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1));
	ASATOM_DECREF(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos));
	asAtomHandler::setBool(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),res);
	++(context->exec_pos);
}
void ABCVm::abc_add_constant_constant(call_context* context)
{
	LOG_CALL("add_cc");
	asAtom res = *context->exec_pos->arg1_constant;
	asAtomHandler::add(res,*context->exec_pos->arg2_constant,context->worker,context->exec_pos->local3.flags & ABC_OP_FORCEINT);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_add_local_constant(call_context* context)
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
void ABCVm::abc_add_constant_local(call_context* context)
{
	LOG_CALL("add_cl");
	asAtom res = *context->exec_pos->arg1_constant;
	asAtomHandler::add(res,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->worker,context->exec_pos->local3.flags & ABC_OP_FORCEINT);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_add_local_local(call_context* context)
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
void ABCVm::abc_add_constant_constant_localresult(call_context* context)
{
	LOG_CALL("add_ccl");
	asAtomHandler::addreplace(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),context->worker,*context->exec_pos->arg1_constant,*context->exec_pos->arg2_constant,context->exec_pos->local3.flags & ABC_OP_FORCEINT);
	++(context->exec_pos);
}
void ABCVm::abc_add_local_constant_localresult(call_context* context)
{
	LOG_CALL("add_lcl");
	asAtomHandler::addreplace(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),*context->exec_pos->arg2_constant,context->exec_pos->local3.flags & ABC_OP_FORCEINT);
	++(context->exec_pos);
}
void ABCVm::abc_add_constant_local_localresult(call_context* context)
{
	LOG_CALL("add_cll");
	asAtomHandler::addreplace(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),context->worker,*context->exec_pos->arg1_constant,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->exec_pos->local3.flags & ABC_OP_FORCEINT);
	++(context->exec_pos);
}
void ABCVm::abc_add_local_local_localresult(call_context* context)
{
	LOG_CALL("add_lll");
	asAtomHandler::addreplace(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->exec_pos->local3.flags & ABC_OP_FORCEINT);
	++(context->exec_pos);
}
void ABCVm::abc_add_constant_constant_setslotnocoerce(call_context* context)
{
	LOG_CALL("add_ccs");
	asAtom res = asAtomHandler::invalidAtom;
	asAtomHandler::addreplace(res,context->worker,*context->exec_pos->arg1_constant,*context->exec_pos->arg2_constant,context->exec_pos->local3.flags & ABC_OP_FORCEINT);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}
void ABCVm::abc_add_local_constant_setslotnocoerce(call_context* context)
{
	LOG_CALL("add_lcs");
	asAtom res = asAtomHandler::invalidAtom;
	asAtomHandler::addreplace(res,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),*context->exec_pos->arg2_constant,context->exec_pos->local3.flags & ABC_OP_FORCEINT);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}
void ABCVm::abc_add_constant_local_setslotnocoerce(call_context* context)
{
	LOG_CALL("add_cls");
	asAtom res = asAtomHandler::invalidAtom;
	asAtomHandler::addreplace(res,context->worker,*context->exec_pos->arg1_constant,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->exec_pos->local3.flags & ABC_OP_FORCEINT);
	
	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}
void ABCVm::abc_add_local_local_setslotnocoerce(call_context* context)
{
	LOG_CALL("add_lls");
	asAtom res = asAtomHandler::invalidAtom;
	asAtomHandler::addreplace(res,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->exec_pos->local3.flags & ABC_OP_FORCEINT);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}
void ABCVm::abc_subtract_constant_constant(call_context* context)
{
	LOG_CALL("subtract_cc");
	asAtom res = *context->exec_pos->arg1_constant;
	asAtomHandler::subtract(res,context->worker,*context->exec_pos->arg2_constant,context->exec_pos->local3.flags & ABC_OP_FORCEINT);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_subtract_local_constant(call_context* context)
{
	LOG_CALL("subtract_lc");
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	asAtomHandler::subtract(res,context->worker,*context->exec_pos->arg2_constant,context->exec_pos->local3.flags & ABC_OP_FORCEINT);

	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_subtract_constant_local(call_context* context)
{
	LOG_CALL("subtract_cl");
	asAtom res = *context->exec_pos->arg1_constant;
	asAtomHandler::subtract(res,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->exec_pos->local3.flags & ABC_OP_FORCEINT);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_subtract_local_local(call_context* context)
{
	LOG_CALL("subtract_ll");
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	asAtomHandler::subtract(res,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->exec_pos->local3.flags & ABC_OP_FORCEINT);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_subtract_constant_constant_localresult(call_context* context)
{
	LOG_CALL("subtract_ccl");
	asAtomHandler::subtractreplace(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),context->worker,*context->exec_pos->arg1_constant,*context->exec_pos->arg2_constant,context->exec_pos->local3.flags & ABC_OP_FORCEINT);
	++(context->exec_pos);
}
void ABCVm::abc_subtract_local_constant_localresult(call_context* context)
{
	LOG_CALL("subtract_lcl");
	asAtomHandler::subtractreplace(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),*context->exec_pos->arg2_constant,context->exec_pos->local3.flags & ABC_OP_FORCEINT);
	++(context->exec_pos);
}
void ABCVm::abc_subtract_constant_local_localresult(call_context* context)
{
	LOG_CALL("subtract_cll");
	asAtomHandler::subtractreplace(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),context->worker,*context->exec_pos->arg1_constant,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->exec_pos->local3.flags & ABC_OP_FORCEINT);
	++(context->exec_pos);
}
void ABCVm::abc_subtract_local_local_localresult(call_context* context)
{
	LOG_CALL("subtract_lll");
	asAtomHandler::subtractreplace(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->exec_pos->local3.flags & ABC_OP_FORCEINT);
	++(context->exec_pos);
}
void ABCVm::abc_subtract_constant_constant_setslotnocoerce(call_context* context)
{
	LOG_CALL("subtract_ccs");
	asAtom res = asAtomHandler::invalidAtom;
	asAtomHandler::subtractreplace(res,context->worker,*context->exec_pos->arg1_constant,*context->exec_pos->arg2_constant,context->exec_pos->local3.flags & ABC_OP_FORCEINT);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}
void ABCVm::abc_subtract_local_constant_setslotnocoerce(call_context* context)
{
	LOG_CALL("subtract_lcs");
	asAtom res = asAtomHandler::invalidAtom;
	asAtomHandler::subtractreplace(res,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),*context->exec_pos->arg2_constant,context->exec_pos->local3.flags & ABC_OP_FORCEINT);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}
void ABCVm::abc_subtract_constant_local_setslotnocoerce(call_context* context)
{
	LOG_CALL("subtract_cls");
	asAtom res = asAtomHandler::invalidAtom;
	asAtomHandler::subtractreplace(res,context->worker,*context->exec_pos->arg1_constant,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->exec_pos->local3.flags & ABC_OP_FORCEINT);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}
void ABCVm::abc_subtract_local_local_setslotnocoerce(call_context* context)
{
	LOG_CALL("subtract_lls");
	asAtom res = asAtomHandler::invalidAtom;
	asAtomHandler::subtractreplace(res,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->exec_pos->local3.flags & ABC_OP_FORCEINT);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}

void ABCVm::abc_multiply_constant_constant(call_context* context)
{
	LOG_CALL("multiply_cc");
	asAtom res = *context->exec_pos->arg1_constant;
	asAtomHandler::multiply(res,context->worker,*context->exec_pos->arg2_constant,context->exec_pos->local3.flags & ABC_OP_FORCEINT);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_multiply_local_constant(call_context* context)
{
	LOG_CALL("multiply_lc");
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	asAtomHandler::multiply(res,context->worker,*context->exec_pos->arg2_constant,context->exec_pos->local3.flags & ABC_OP_FORCEINT);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_multiply_constant_local(call_context* context)
{
	LOG_CALL("multiply_cl");
	asAtom res = *context->exec_pos->arg1_constant;
	asAtomHandler::multiply(res,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->exec_pos->local3.flags & ABC_OP_FORCEINT);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_multiply_local_local(call_context* context)
{
	LOG_CALL("multiply_ll");
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	asAtomHandler::multiply(res,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->exec_pos->local3.flags & ABC_OP_FORCEINT);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_multiply_constant_constant_localresult(call_context* context)
{
	LOG_CALL("multiply_ccl");
	asAtomHandler::multiplyreplace(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),context->worker,*context->exec_pos->arg1_constant,*context->exec_pos->arg2_constant,context->exec_pos->local3.flags & ABC_OP_FORCEINT);
	++(context->exec_pos);
}
void ABCVm::abc_multiply_local_constant_localresult(call_context* context)
{
	LOG_CALL("multiply_lcl");
	asAtomHandler::multiplyreplace(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),*context->exec_pos->arg2_constant,context->exec_pos->local3.flags & ABC_OP_FORCEINT);
	++(context->exec_pos);
}
void ABCVm::abc_multiply_constant_local_localresult(call_context* context)
{
	LOG_CALL("multiply_cll");
	asAtomHandler::multiplyreplace(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),context->worker,*context->exec_pos->arg1_constant,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->exec_pos->local3.flags & ABC_OP_FORCEINT);
	++(context->exec_pos);
}
void ABCVm::abc_multiply_local_local_localresult(call_context* context)
{
	LOG_CALL("multiply_lll");
	asAtomHandler::multiplyreplace(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->exec_pos->local3.flags & ABC_OP_FORCEINT);
	++(context->exec_pos);
}
void ABCVm::abc_multiply_constant_constant_setslotnocoerce(call_context* context)
{
	LOG_CALL("multiply_ccs");
	asAtom res = asAtomHandler::invalidAtom;
	asAtomHandler::multiplyreplace(res,context->worker,*context->exec_pos->arg1_constant,*context->exec_pos->arg2_constant,context->exec_pos->local3.flags & ABC_OP_FORCEINT);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}
void ABCVm::abc_multiply_local_constant_setslotnocoerce(call_context* context)
{
	LOG_CALL("multiply_lcs");
	asAtom res = asAtomHandler::invalidAtom;
	asAtomHandler::multiplyreplace(res,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),*context->exec_pos->arg2_constant,context->exec_pos->local3.flags & ABC_OP_FORCEINT);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}
void ABCVm::abc_multiply_constant_local_setslotnocoerce(call_context* context)
{
	LOG_CALL("multiply_cls");
	asAtom res = asAtomHandler::invalidAtom;
	asAtomHandler::multiplyreplace(res,context->worker,*context->exec_pos->arg1_constant,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->exec_pos->local3.flags & ABC_OP_FORCEINT);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}
void ABCVm::abc_multiply_local_local_setslotnocoerce(call_context* context)
{
	LOG_CALL("multiply_lls");
	asAtom res = asAtomHandler::invalidAtom;
	asAtomHandler::multiplyreplace(res,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->exec_pos->local3.flags & ABC_OP_FORCEINT);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}

void ABCVm::abc_divide_constant_constant(call_context* context)
{
	LOG_CALL("divide_cc");
	asAtom res = *context->exec_pos->arg1_constant;
	asAtomHandler::divide(res,context->worker,*context->exec_pos->arg2_constant,context->exec_pos->local3.flags & ABC_OP_FORCEINT);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_divide_local_constant(call_context* context)
{
	LOG_CALL("divide_lc");
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	asAtomHandler::divide(res,context->worker,*context->exec_pos->arg2_constant,context->exec_pos->local3.flags & ABC_OP_FORCEINT);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_divide_constant_local(call_context* context)
{
	LOG_CALL("divide_cl");
	asAtom res = *context->exec_pos->arg1_constant;
	asAtomHandler::divide(res,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->exec_pos->local3.flags & ABC_OP_FORCEINT);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_divide_local_local(call_context* context)
{
	LOG_CALL("divide_ll");
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	asAtomHandler::divide(res,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->exec_pos->local3.flags & ABC_OP_FORCEINT);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_divide_constant_constant_localresult(call_context* context)
{
	LOG_CALL("divide_ccl");
	asAtomHandler::dividereplace(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),context->worker,*context->exec_pos->arg1_constant,*context->exec_pos->arg2_constant,context->exec_pos->local3.flags & ABC_OP_FORCEINT);
	++(context->exec_pos);
}
void ABCVm::abc_divide_local_constant_localresult(call_context* context)
{
	LOG_CALL("divide_lcl");
	asAtomHandler::dividereplace(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),*context->exec_pos->arg2_constant,context->exec_pos->local3.flags & ABC_OP_FORCEINT);
	++(context->exec_pos);
}
void ABCVm::abc_divide_constant_local_localresult(call_context* context)
{
	LOG_CALL("divide_cll");
	asAtomHandler::dividereplace(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),context->worker,*context->exec_pos->arg1_constant,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->exec_pos->local3.flags & ABC_OP_FORCEINT);
	++(context->exec_pos);
}
void ABCVm::abc_divide_local_local_localresult(call_context* context)
{
	LOG_CALL("divide_lll");
	asAtomHandler::dividereplace(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->exec_pos->local3.flags & ABC_OP_FORCEINT);
	++(context->exec_pos);
}
void ABCVm::abc_divide_constant_constant_setslotnocoerce(call_context* context)
{
	LOG_CALL("divide_ccs");
	asAtom res = asAtomHandler::invalidAtom;
	asAtomHandler::dividereplace(res,context->worker,*context->exec_pos->arg1_constant,*context->exec_pos->arg2_constant,context->exec_pos->local3.flags & ABC_OP_FORCEINT);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}
void ABCVm::abc_divide_local_constant_setslotnocoerce(call_context* context)
{
	LOG_CALL("divide_lcs");
	asAtom res = asAtomHandler::invalidAtom;
	asAtomHandler::dividereplace(res,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),*context->exec_pos->arg2_constant,context->exec_pos->local3.flags & ABC_OP_FORCEINT);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}
void ABCVm::abc_divide_constant_local_setslotnocoerce(call_context* context)
{
	LOG_CALL("divide_cls");
	asAtom res = asAtomHandler::invalidAtom;
	asAtomHandler::dividereplace(res,context->worker,*context->exec_pos->arg1_constant,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->exec_pos->local3.flags & ABC_OP_FORCEINT);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}
void ABCVm::abc_divide_local_local_setslotnocoerce(call_context* context)
{
	LOG_CALL("divide_lls");
	asAtom res = asAtomHandler::invalidAtom;
	asAtomHandler::dividereplace(res,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->exec_pos->local3.flags & ABC_OP_FORCEINT);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}
void ABCVm::abc_modulo_constant_constant(call_context* context)
{
	asAtom res = *context->exec_pos->arg1_constant;
	asAtomHandler::modulo(res,context->worker,*context->exec_pos->arg2_constant,context->exec_pos->local3.flags & ABC_OP_FORCEINT);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_modulo_local_constant(call_context* context)
{
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	asAtomHandler::modulo(res,context->worker,*context->exec_pos->arg2_constant,context->exec_pos->local3.flags & ABC_OP_FORCEINT);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_modulo_constant_local(call_context* context)
{
	asAtom res = *context->exec_pos->arg1_constant;
	asAtomHandler::modulo(res,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->exec_pos->local3.flags & ABC_OP_FORCEINT);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_modulo_local_local(call_context* context)
{
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	asAtomHandler::modulo(res,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->exec_pos->local3.flags & ABC_OP_FORCEINT);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_modulo_constant_constant_localresult(call_context* context)
{
	LOG_CALL("modulo_ccl");
	asAtomHandler::moduloreplace(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),context->worker,*context->exec_pos->arg1_constant,*context->exec_pos->arg2_constant,context->exec_pos->local3.flags & ABC_OP_FORCEINT);
	++(context->exec_pos);
}
void ABCVm::abc_modulo_local_constant_localresult(call_context* context)
{
	LOG_CALL("modulo_lcl");
	asAtomHandler::moduloreplace(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),*context->exec_pos->arg2_constant,context->exec_pos->local3.flags & ABC_OP_FORCEINT);
	++(context->exec_pos);
}
void ABCVm::abc_modulo_constant_local_localresult(call_context* context)
{
	LOG_CALL("modulo_cll");
	asAtomHandler::moduloreplace(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),context->worker,*context->exec_pos->arg1_constant,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->exec_pos->local3.flags & ABC_OP_FORCEINT);
	++(context->exec_pos);
}
void ABCVm::abc_modulo_local_local_localresult(call_context* context)
{
	LOG_CALL("modulo_lll");
	asAtomHandler::moduloreplace(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->exec_pos->local3.flags & ABC_OP_FORCEINT);
	++(context->exec_pos);
}
void ABCVm::abc_modulo_constant_constant_setslotnocoerce(call_context* context)
{
	LOG_CALL("modulo_ccs");
	asAtom res = asAtomHandler::invalidAtom;
	asAtomHandler::moduloreplace(res,context->worker,*context->exec_pos->arg1_constant,*context->exec_pos->arg2_constant,context->exec_pos->local3.flags & ABC_OP_FORCEINT);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}
void ABCVm::abc_modulo_local_constant_setslotnocoerce(call_context* context)
{
	LOG_CALL("modulo_lcs");
	asAtom res = asAtomHandler::invalidAtom;
	asAtomHandler::moduloreplace(res,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),*context->exec_pos->arg2_constant,context->exec_pos->local3.flags & ABC_OP_FORCEINT);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}
void ABCVm::abc_modulo_constant_local_setslotnocoerce(call_context* context)
{
	LOG_CALL("modulo_cls");
	asAtom res = asAtomHandler::invalidAtom;
	asAtomHandler::moduloreplace(res,context->worker,*context->exec_pos->arg1_constant,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->exec_pos->local3.flags & ABC_OP_FORCEINT);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}
void ABCVm::abc_modulo_local_local_setslotnocoerce(call_context* context)
{
	LOG_CALL("modulo_lls");
	asAtom res = asAtomHandler::invalidAtom;
	asAtomHandler::moduloreplace(res,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->exec_pos->local3.flags & ABC_OP_FORCEINT);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}
void ABCVm::abc_lshift_constant_constant(call_context* context)
{
	int32_t i2=context->exec_pos->arg1_int;
	uint32_t i1=(context->exec_pos->arg2_uint)&0x1f;
	LOG_CALL("lShift_cc "<<std::hex<<i2<<"<<"<<i1<<std::dec);
	asAtom res;
	asAtomHandler::setInt(res,context->worker,i2<<i1);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_lshift_local_constant(call_context* context)
{
	int32_t i2=asAtomHandler::toInt(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1));
	uint32_t i1=(context->exec_pos->arg2_uint)&0x1f;
	LOG_CALL("lShift_lc "<<std::hex<<i2<<"<<"<<i1<<std::dec);
	asAtom res;
	asAtomHandler::setInt(res,context->worker,i2<<i1);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_lshift_constant_local(call_context* context)
{
	int32_t i2=context->exec_pos->arg1_int;
	uint32_t i1=(asAtomHandler::toInt(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2)))&0x1f;
	LOG_CALL("lShift_cl "<<std::hex<<i2<<"<<"<<i1<<std::dec);
	asAtom res;
	asAtomHandler::setInt(res,context->worker,i2<<i1);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_lshift_local_local(call_context* context)
{
	int32_t i2=asAtomHandler::toInt(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1));
	uint32_t i1=(asAtomHandler::toInt(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2)))&0x1f;
	LOG_CALL("lShift_ll "<<std::hex<<i2<<"<<"<<i1<<std::dec);
	asAtom res;
	asAtomHandler::setInt(res,context->worker,i2<<i1);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_lshift_constant_constant_localresult(call_context* context)
{
	int32_t i2=context->exec_pos->arg1_int;
	uint32_t i1=(context->exec_pos->arg2_uint)&0x1f;
	LOG_CALL("lShift_ccl "<<std::hex<<i2<<"<<"<<i1<<std::dec);
	asAtom res;
	asAtomHandler::setInt(res,context->worker,i2<<i1);
	replacelocalresult(context,context->exec_pos->local3.pos,res);
	++(context->exec_pos);
}
void ABCVm::abc_lshift_local_constant_localresult(call_context* context)
{
	int32_t i2=asAtomHandler::toInt(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1));
	uint32_t i1=(context->exec_pos->arg2_uint)&0x1f;
	LOG_CALL("lShift_lcl "<<std::hex<<i2<<"<<"<<i1<<std::dec);
	asAtom res;
	asAtomHandler::setInt(res,context->worker,i2<<i1);
	replacelocalresult(context,context->exec_pos->local3.pos,res);
	++(context->exec_pos);
}
void ABCVm::abc_lshift_constant_local_localresult(call_context* context)
{
	int32_t i2=context->exec_pos->arg1_int;
	uint32_t i1=(asAtomHandler::toInt(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2)))&0x1f;
	LOG_CALL("lShift_cll "<<std::hex<<i2<<"<<"<<i1<<std::dec);
	asAtom res;
	asAtomHandler::setInt(res,context->worker,i2<<i1);
	replacelocalresult(context,context->exec_pos->local3.pos,res);
	++(context->exec_pos);
}
void ABCVm::abc_lshift_local_local_localresult(call_context* context)
{
	int32_t i2=asAtomHandler::toInt(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1));
	uint32_t i1=(asAtomHandler::toInt(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2)))&0x1f;
	LOG_CALL("lShift_lll "<<std::hex<<i2<<"<<"<<i1<<std::dec);
	asAtom res;
	asAtomHandler::setInt(res,context->worker,i2<<i1);
	replacelocalresult(context,context->exec_pos->local3.pos,res);
	++(context->exec_pos);
}
void ABCVm::abc_lshift_constant_constant_setslotnocoerce(call_context* context)
{
	int32_t i2=context->exec_pos->arg1_int;
	uint32_t i1=(context->exec_pos->arg2_uint)&0x1f;
	LOG_CALL("lShift_ccs "<<std::hex<<i2<<"<<"<<i1<<std::dec);
	asAtom res;
	asAtomHandler::setInt(res,context->worker,i2<<i1);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}
void ABCVm::abc_lshift_local_constant_setslotnocoerce(call_context* context)
{
	int32_t i2=asAtomHandler::toInt(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1));
	uint32_t i1=(context->exec_pos->arg2_uint)&0x1f;
	LOG_CALL("lShift_lcs "<<std::hex<<i2<<"<<"<<i1<<std::dec);
	asAtom res;
	asAtomHandler::setInt(res,context->worker,i2<<i1);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}
void ABCVm::abc_lshift_constant_local_setslotnocoerce(call_context* context)
{
	int32_t i2=context->exec_pos->arg1_int;
	uint32_t i1=(asAtomHandler::toInt(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2)))&0x1f;
	LOG_CALL("lShift_cls "<<std::hex<<i2<<"<<"<<i1<<std::dec);
	asAtom res;
	asAtomHandler::setInt(res,context->worker,i2<<i1);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}
void ABCVm::abc_lshift_local_local_setslotnocoerce(call_context* context)
{
	int32_t i2=asAtomHandler::toInt(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1));
	uint32_t i1=(asAtomHandler::toInt(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2)))&0x1f;
	LOG_CALL("lShift_lls "<<std::hex<<i2<<"<<"<<i1<<std::dec);
	asAtom res;
	asAtomHandler::setInt(res,context->worker,i2<<i1);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}


void ABCVm::abc_rshift_constant_constant(call_context* context)
{
	LOG_CALL("rShift_cc ");
	asAtom res = *context->exec_pos->arg1_constant;
	asAtomHandler::rshift(res,context->worker,*context->exec_pos->arg2_constant);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_rshift_local_constant(call_context* context)
{
	LOG_CALL("rShift_lc ");
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	asAtomHandler::rshift(res,context->worker,*context->exec_pos->arg2_constant);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_rshift_constant_local(call_context* context)
{
	LOG_CALL("rShift_cl ");
	asAtom res = *context->exec_pos->arg1_constant;
	asAtomHandler::rshift(res,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2));
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_rshift_local_local(call_context* context)
{
	LOG_CALL("rShift_ll ");
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	asAtomHandler::rshift(res,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2));
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_rshift_constant_constant_localresult(call_context* context)
{
	LOG_CALL("rShift_ccl ");
	asAtom res = *context->exec_pos->arg1_constant;
	asAtomHandler::rshift(res,context->worker,*context->exec_pos->arg2_constant);
	replacelocalresult(context,context->exec_pos->local3.pos,res);
	++(context->exec_pos);
}
void ABCVm::abc_rshift_local_constant_localresult(call_context* context)
{
	LOG_CALL("rShift_lcl ");
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	asAtomHandler::rshift(res,context->worker,*context->exec_pos->arg2_constant);
	replacelocalresult(context,context->exec_pos->local3.pos,res);
	++(context->exec_pos);
}
void ABCVm::abc_rshift_constant_local_localresult(call_context* context)
{
	LOG_CALL("rShift_cll ");
	asAtom res = *context->exec_pos->arg1_constant;
	asAtomHandler::rshift(res,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2));
	replacelocalresult(context,context->exec_pos->local3.pos,res);
	++(context->exec_pos);
}
void ABCVm::abc_rshift_local_local_localresult(call_context* context)
{
	LOG_CALL("rShift_lll ");
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	asAtomHandler::rshift(res,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2));
	replacelocalresult(context,context->exec_pos->local3.pos,res);
	++(context->exec_pos);
}
void ABCVm::abc_rshift_constant_constant_setslotnocoerce(call_context* context)
{
	LOG_CALL("rShift_ccs ");
	asAtom res = *context->exec_pos->arg1_constant;
	asAtomHandler::rshift(res,context->worker,*context->exec_pos->arg2_constant);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}
void ABCVm::abc_rshift_local_constant_setslotnocoerce(call_context* context)
{
	LOG_CALL("rShift_lcs ");
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	asAtomHandler::rshift(res,context->worker,*context->exec_pos->arg2_constant);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}
void ABCVm::abc_rshift_constant_local_setslotnocoerce(call_context* context)
{
	LOG_CALL("rShift_cls ");
	asAtom res = *context->exec_pos->arg1_constant;
	asAtomHandler::rshift(res,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2));

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}
void ABCVm::abc_rshift_local_local_setslotnocoerce(call_context* context)
{
	LOG_CALL("rShift_lls ");
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	asAtomHandler::rshift(res,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2));

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}

void ABCVm::abc_urshift_constant_constant(call_context* context)
{
	uint32_t i2=context->exec_pos->arg1_uint;
	uint32_t i1=(context->exec_pos->arg2_uint)&0x1f;
	LOG_CALL("urShift_cc "<<std::hex<<i2<<">>"<<i1<<std::dec);
	asAtom res;
	asAtomHandler::setInt(res,context->worker,i2>>i1);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_urshift_local_constant(call_context* context)
{
	uint32_t i2=asAtomHandler::toUInt(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1));
	uint32_t i1=(context->exec_pos->arg2_uint)&0x1f;
	LOG_CALL("urShift_lc "<<std::hex<<i2<<">>"<<i1<<std::dec);
	asAtom res;
	asAtomHandler::setInt(res,context->worker,i2>>i1);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_urshift_constant_local(call_context* context)
{
	uint32_t i2=(context->exec_pos->arg1_uint);
	uint32_t i1=asAtomHandler::toUInt(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2))&0x1f;
	LOG_CALL("urShift_cl "<<std::hex<<i2<<">>"<<i1<<std::dec);
	asAtom res;
	asAtomHandler::setInt(res,context->worker,i2>>i1);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_urshift_local_local(call_context* context)
{
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	asAtomHandler::urshift(res,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2));
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_urshift_constant_constant_localresult(call_context* context)
{
	uint32_t i2=context->exec_pos->arg1_uint;
	uint32_t i1=(context->exec_pos->arg2_uint)&0x1f;
	LOG_CALL("urShift_ccl "<<std::hex<<i2<<">>"<<i1<<std::dec);
	asAtom res;
	asAtomHandler::setInt(res,context->worker,i2>>i1);
	replacelocalresult(context,context->exec_pos->local3.pos,res);
	++(context->exec_pos);
}
void ABCVm::abc_urshift_local_constant_localresult(call_context* context)
{
	uint32_t i2=asAtomHandler::toUInt(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1));
	uint32_t i1=(context->exec_pos->arg2_uint)&0x1f;
	LOG_CALL("urShift_lcl "<<std::hex<<i2<<">>"<<i1<<std::dec);
	asAtom res;
	asAtomHandler::setInt(res,context->worker,i2>>i1);
	replacelocalresult(context,context->exec_pos->local3.pos,res);
	++(context->exec_pos);
}
void ABCVm::abc_urshift_constant_local_localresult(call_context* context)
{
	uint32_t i2=(context->exec_pos->arg1_uint);
	uint32_t i1=asAtomHandler::toUInt(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2))&0x1f;
	LOG_CALL("urShift_cll "<<std::hex<<i2<<">>"<<i1<<std::dec);
	asAtom res;
	asAtomHandler::setInt(res,context->worker,i2>>i1);
	replacelocalresult(context,context->exec_pos->local3.pos,res);
	++(context->exec_pos);
}
void ABCVm::abc_urshift_local_local_localresult(call_context* context)
{
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	asAtomHandler::urshift(res,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2));
	replacelocalresult(context,context->exec_pos->local3.pos,res);
	++(context->exec_pos);
}
void ABCVm::abc_urshift_constant_constant_setslotnocoerce(call_context* context)
{
	uint32_t i2=context->exec_pos->arg1_uint;
	uint32_t i1=(context->exec_pos->arg2_uint)&0x1f;
	LOG_CALL("urShift_ccs "<<std::hex<<i2<<">>"<<i1<<std::dec);
	asAtom res;
	asAtomHandler::setInt(res,context->worker,i2>>i1);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}
void ABCVm::abc_urshift_local_constant_setslotnocoerce(call_context* context)
{
	uint32_t i2=asAtomHandler::toUInt(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1));
	uint32_t i1=(context->exec_pos->arg2_uint)&0x1f;
	LOG_CALL("urShift_lcs "<<std::hex<<i2<<">>"<<i1<<std::dec);
	asAtom res;
	asAtomHandler::setInt(res,context->worker,i2>>i1);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}
void ABCVm::abc_urshift_constant_local_setslotnocoerce(call_context* context)
{
	uint32_t i2=(context->exec_pos->arg1_uint);
	uint32_t i1=asAtomHandler::toUInt(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2))&0x1f;
	LOG_CALL("urShift_cls "<<std::hex<<i2<<">>"<<i1<<std::dec);
	asAtom res;
	asAtomHandler::setInt(res,context->worker,i2>>i1);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}
void ABCVm::abc_urshift_local_local_setslotnocoerce(call_context* context)
{
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	asAtomHandler::urshift(res,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2));

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}

void ABCVm::abc_bitand_constant_constant(call_context* context)
{
	int32_t i1=context->exec_pos->arg2_int;
	int32_t i2=context->exec_pos->arg1_int;
	LOG_CALL("bitAnd_cc " << std::hex << i1 << '&' << i2 << std::dec);
	asAtom res;
	asAtomHandler::setInt(res,context->worker,i1&i2);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_bitand_local_constant(call_context* context)
{
	int32_t i1=context->exec_pos->arg2_int;
	int32_t i2=asAtomHandler::toInt(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1));
	LOG_CALL("bitAnd_lc " << std::hex << i1 << '&' << i2 << std::dec);
	asAtom res;
	asAtomHandler::setInt(res,context->worker,i1&i2);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_bitand_constant_local(call_context* context)
{
	int32_t i1=asAtomHandler::toInt(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2));
	int32_t i2=context->exec_pos->arg1_int;
	LOG_CALL("bitAnd_cl " << std::hex << i1 << '&' << i2 << std::dec);
	asAtom res;
	asAtomHandler::setInt(res,context->worker,i1&i2);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_bitand_local_local(call_context* context)
{
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	asAtomHandler::bit_and(res,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2));
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_bitand_constant_constant_localresult(call_context* context)
{
	int32_t i1=context->exec_pos->arg2_int;
	int32_t i2=context->exec_pos->arg1_int;
	LOG_CALL("bitAnd_ccl " << std::hex << i1 << '&' << i2 << std::dec);
	asAtom res;
	asAtomHandler::setInt(res,context->worker,i1&i2);
	replacelocalresult(context,context->exec_pos->local3.pos,res);
	++(context->exec_pos);
}
void ABCVm::abc_bitand_local_constant_localresult(call_context* context)
{
	int32_t i1=context->exec_pos->arg2_int;
	int32_t i2=asAtomHandler::toInt(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1));
	LOG_CALL("bitAnd_lcl " << std::hex << i1 << '&' << i2 << std::dec);
	asAtom res;
	asAtomHandler::setInt(res,context->worker,i1&i2);
	replacelocalresult(context,context->exec_pos->local3.pos,res);
	++(context->exec_pos);
}
void ABCVm::abc_bitand_constant_local_localresult(call_context* context)
{
	int32_t i1=asAtomHandler::toInt(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2));
	int32_t i2=context->exec_pos->arg1_int;
	LOG_CALL("bitAnd_cll " << std::hex << i1 << '&' << i2 << std::dec);
	asAtom res;
	asAtomHandler::setInt(res,context->worker,i1&i2);
	replacelocalresult(context,context->exec_pos->local3.pos,res);
	++(context->exec_pos);
}
void ABCVm::abc_bitand_local_local_localresult(call_context* context)
{
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	asAtomHandler::bit_and(res,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2));
	replacelocalresult(context,context->exec_pos->local3.pos,res);
	++(context->exec_pos);
}
void ABCVm::abc_bitand_constant_constant_setslotnocoerce(call_context* context)
{
	int32_t i1=context->exec_pos->arg2_int;
	int32_t i2=context->exec_pos->arg1_int;
	LOG_CALL("bitAnd_ccs " << std::hex << i1 << '&' << i2 << std::dec);
	asAtom res;
	asAtomHandler::setInt(res,context->worker,i1&i2);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}
void ABCVm::abc_bitand_local_constant_setslotnocoerce(call_context* context)
{
	int32_t i1=context->exec_pos->arg2_int;
	int32_t i2=asAtomHandler::toInt(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1));
	LOG_CALL("bitAnd_lcs " << std::hex << i1 << '&' << i2 << std::dec);
	asAtom res;
	asAtomHandler::setInt(res,context->worker,i1&i2);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}
void ABCVm::abc_bitand_constant_local_setslotnocoerce(call_context* context)
{
	int32_t i1=asAtomHandler::toInt(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2));
	int32_t i2=context->exec_pos->arg1_int;
	LOG_CALL("bitAnd_cls " << std::hex << i1 << '&' << i2 << std::dec);
	asAtom res;
	asAtomHandler::setInt(res,context->worker,i1&i2);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}
void ABCVm::abc_bitand_local_local_setslotnocoerce(call_context* context)
{
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	asAtomHandler::bit_and(res,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2));

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}
void ABCVm::abc_bitor_constant_constant(call_context* context)
{
	int32_t i1=context->exec_pos->arg2_int;
	int32_t i2=context->exec_pos->arg1_int;
	LOG_CALL("bitor_cc " << std::hex << i1 << '|' << i2 << std::dec);
	asAtom res;
	asAtomHandler::setInt(res,context->worker,i1|i2);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_bitor_local_constant(call_context* context)
{
	int32_t i1=context->exec_pos->arg2_int;
	int32_t i2=asAtomHandler::toInt(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1));
	LOG_CALL("bitor_lc " << std::hex << i1 << '|' << i2 << std::dec);
	asAtom res;
	asAtomHandler::setInt(res,context->worker,i1|i2);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_bitor_constant_local(call_context* context)
{
	int32_t i1=asAtomHandler::toInt(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2));
	int32_t i2=context->exec_pos->arg1_int;
	LOG_CALL("bitor_cl " << std::hex << i1 << '|' << i2 << std::dec);
	asAtom res;
	asAtomHandler::setInt(res,context->worker,i1|i2);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_bitor_local_local(call_context* context)
{
	int32_t i1=asAtomHandler::toInt(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2));
	int32_t i2=asAtomHandler::toInt(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1));
	LOG_CALL("bitor_cl " << std::hex << i1 << '|' << i2 << std::dec);
	asAtom res;
	asAtomHandler::setInt(res,context->worker,i1|i2);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_bitor_constant_constant_localresult(call_context* context)
{
	int32_t i1=context->exec_pos->arg2_int;
	int32_t i2=context->exec_pos->arg1_int;
	LOG_CALL("bitor_ccl " << std::hex << i1 << '|' << i2 << std::dec);
	asAtom res;
	asAtomHandler::setInt(res,context->worker,i1|i2);
	replacelocalresult(context,context->exec_pos->local3.pos,res);
	++(context->exec_pos);
}
void ABCVm::abc_bitor_local_constant_localresult(call_context* context)
{
	int32_t i1=context->exec_pos->arg2_int;
	int32_t i2=asAtomHandler::toInt(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1));
	LOG_CALL("bitor_lcl " << std::hex << i1 << '|' << i2 << std::dec);
	asAtom res;
	asAtomHandler::setInt(res,context->worker,i1|i2);
	replacelocalresult(context,context->exec_pos->local3.pos,res);
	++(context->exec_pos);
}
void ABCVm::abc_bitor_constant_local_localresult(call_context* context)
{
	int32_t i1=asAtomHandler::toInt(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2));
	int32_t i2=context->exec_pos->arg1_int;
	LOG_CALL("bitor_cll " << std::hex << i1 << '|' << i2 << std::dec);
	asAtom res;
	asAtomHandler::setInt(res,context->worker,i1|i2);
	replacelocalresult(context,context->exec_pos->local3.pos,res);
	++(context->exec_pos);
}
void ABCVm::abc_bitor_local_local_localresult(call_context* context)
{
	int32_t i1=asAtomHandler::toInt(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2));
	int32_t i2=asAtomHandler::toInt(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1));
	LOG_CALL("bitor_lll " << std::hex << i1 << '|' << i2 << std::dec);
	asAtom res;
	asAtomHandler::setInt(res,context->worker,i1|i2);
	replacelocalresult(context,context->exec_pos->local3.pos,res);
	++(context->exec_pos);
}
void ABCVm::abc_bitor_constant_constant_setslotnocoerce(call_context* context)
{
	int32_t i1=context->exec_pos->arg2_int;
	int32_t i2=context->exec_pos->arg1_int;
	LOG_CALL("bitor_ccs " << std::hex << i1 << '|' << i2 << std::dec);
	asAtom res;
	asAtomHandler::setInt(res,context->worker,i1|i2);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}
void ABCVm::abc_bitor_local_constant_setslotnocoerce(call_context* context)
{
	int32_t i1=context->exec_pos->arg2_int;
	int32_t i2=asAtomHandler::toInt(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1));
	LOG_CALL("bitor_lcs " << std::hex << i1 << '|' << i2 << std::dec);
	asAtom res;
	asAtomHandler::setInt(res,context->worker,i1|i2);
	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);

	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}
void ABCVm::abc_bitor_constant_local_setslotnocoerce(call_context* context)
{
	int32_t i1=asAtomHandler::toInt(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2));
	int32_t i2=context->exec_pos->arg1_int;
	LOG_CALL("bitor_cls " << std::hex << i1 << '|' << i2 << std::dec);
	asAtom res;
	asAtomHandler::setInt(res,context->worker,i1|i2);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}
void ABCVm::abc_bitor_local_local_setslotnocoerce(call_context* context)
{
	int32_t i1=asAtomHandler::toInt(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2));
	int32_t i2=asAtomHandler::toInt(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1));
	LOG_CALL("bitor_lls " << std::hex << i1 << '|' << i2 << std::dec);
	asAtom res;
	asAtomHandler::setInt(res,context->worker,i1|i2);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}

void ABCVm::abc_bitxor_constant_constant(call_context* context)
{
	int32_t i1=context->exec_pos->arg2_int;
	int32_t i2=context->exec_pos->arg1_int;
	LOG_CALL("bitXor_cc " << std::hex << i1 << '^' << i2 << std::dec);
	asAtom res;
	asAtomHandler::setInt(res,context->worker,i1^i2);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_bitxor_local_constant(call_context* context)
{
	int32_t i1=context->exec_pos->arg2_int;
	int32_t i2=asAtomHandler::toInt(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1));
	LOG_CALL("bitXor_lc " << std::hex << i1 << '^' << i2 << std::dec);
	asAtom res;
	asAtomHandler::setInt(res,context->worker,i1^i2);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_bitxor_constant_local(call_context* context)
{
	int32_t i1=asAtomHandler::toInt(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2));
	int32_t i2=context->exec_pos->arg1_int;
	LOG_CALL("bitXor_cl " << std::hex << i1 << '^' << i2 << std::dec);
	asAtom res;
	asAtomHandler::setInt(res,context->worker,i1^i2);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_bitxor_local_local(call_context* context)
{
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	asAtomHandler::bit_xor(res,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2));
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_bitxor_constant_constant_localresult(call_context* context)
{
	int32_t i1=context->exec_pos->arg2_int;
	int32_t i2=context->exec_pos->arg1_int;
	LOG_CALL("bitXor_ccl " << std::hex << i1 << '^' << i2 << std::dec);
	asAtom res;
	asAtomHandler::setInt(res,context->worker,i1^i2);
	replacelocalresult(context,context->exec_pos->local3.pos,res);
	++(context->exec_pos);
}
void ABCVm::abc_bitxor_local_constant_localresult(call_context* context)
{
	int32_t i1=context->exec_pos->arg2_int;
	int32_t i2=asAtomHandler::toInt(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1));
	LOG_CALL("bitXor_lcl " << std::hex << i1 << '^' << i2 << std::dec);
	ASATOM_DECREF(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos));
	asAtomHandler::setInt(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),context->worker,i1^i2);
	++(context->exec_pos);
}
void ABCVm::abc_bitxor_constant_local_localresult(call_context* context)
{
	int32_t i1=asAtomHandler::toInt(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2));
	int32_t i2=context->exec_pos->arg1_int;
	ASATOM_DECREF(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos));
	asAtomHandler::setInt(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),context->worker,i1^i2);
	++(context->exec_pos);
}
void ABCVm::abc_bitxor_local_local_localresult(call_context* context)
{
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	asAtomHandler::bit_xor(res,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2));
	replacelocalresult(context,context->exec_pos->local3.pos,res);
	++(context->exec_pos);
}
void ABCVm::abc_bitxor_constant_constant_setslotnocoerce(call_context *context)
{
	int32_t i1=context->exec_pos->arg2_int;
	int32_t i2=context->exec_pos->arg1_int;
	LOG_CALL("bitXor_ccs " << std::hex << i1 << '^' << i2 << std::dec);
	asAtom res = asAtomHandler::invalidAtom;
	asAtomHandler::setInt(res,context->worker,i1^i2);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}
void ABCVm::abc_bitxor_local_constant_setslotnocoerce(call_context* context)
{
	int32_t i1=context->exec_pos->arg2_int;
	int32_t i2=asAtomHandler::toInt(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1));
	LOG_CALL("bitXor_lcs " << std::hex << i1 << '^' << i2 << std::dec);
	asAtom res = asAtomHandler::invalidAtom;
	asAtomHandler::setInt(res,context->worker,i1^i2);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}
void ABCVm::abc_bitxor_constant_local_setslotnocoerce(call_context* context)
{
	int32_t i1=asAtomHandler::toInt(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2));
	int32_t i2=context->exec_pos->arg1_int;
	LOG_CALL("bitXor_cls " << std::hex << i1 << '^' << i2 << std::dec);
	asAtom res = asAtomHandler::invalidAtom;
	asAtomHandler::setInt(res,context->worker,i1^i2);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}
void ABCVm::abc_bitxor_local_local_setslotnocoerce(call_context *context)
{
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	asAtomHandler::bit_xor(res,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2));

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}

void ABCVm::abc_equals_constant_constant(call_context* context)
{
	//equals

	bool ret=(asAtomHandler::isEqual(*context->exec_pos->arg2_constant,context->worker,*context->exec_pos->arg1_constant));
	LOG_CALL("equals_cc "<<ret);

	RUNTIME_STACK_PUSH(context,asAtomHandler::fromBool(ret));
	++(context->exec_pos);
}
void ABCVm::abc_equals_local_constant(call_context* context)
{
	//equals

	bool ret=(asAtomHandler::isEqual(*context->exec_pos->arg2_constant,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1)));
	LOG_CALL("equals_lc "<<ret);

	RUNTIME_STACK_PUSH(context,asAtomHandler::fromBool(ret));
	++(context->exec_pos);
}
void ABCVm::abc_equals_constant_local(call_context* context)
{
	//equals

	bool ret=(asAtomHandler::isEqual(*context->exec_pos->arg1_constant,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2)));
	LOG_CALL("equals_cl "<<ret);

	RUNTIME_STACK_PUSH(context,asAtomHandler::fromBool(ret));
	++(context->exec_pos);
}
void ABCVm::abc_equals_local_local(call_context* context)
{
	//equals

	bool ret=(asAtomHandler::isEqual(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1)));
	LOG_CALL("equals_ll "<<ret);

	RUNTIME_STACK_PUSH(context,asAtomHandler::fromBool(ret));
	++(context->exec_pos);
}
void ABCVm::abc_equals_constant_constant_localresult(call_context* context)
{
	//equals

	bool ret=(asAtomHandler::isEqual(*context->exec_pos->arg2_constant,context->worker,*context->exec_pos->arg1_constant));
	LOG_CALL("equals_ccl "<<ret);

	ASATOM_DECREF(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos));
	asAtomHandler::setBool(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),ret);
	++(context->exec_pos);
}
void ABCVm::abc_equals_local_constant_localresult(call_context* context)
{
	//equals

	bool ret=(asAtomHandler::isEqual(*context->exec_pos->arg2_constant,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1)));
	LOG_CALL("equals_lcl "<<ret);

	ASATOM_DECREF(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos));
	asAtomHandler::setBool(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),ret);
	++(context->exec_pos);
}
void ABCVm::abc_equals_constant_local_localresult(call_context* context)
{
	//equals

	bool ret=(asAtomHandler::isEqual(*context->exec_pos->arg1_constant,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2)));
	LOG_CALL("equals_cll "<<ret);

	ASATOM_DECREF(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos));
	asAtomHandler::setBool(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),ret);
	++(context->exec_pos);
}
void ABCVm::abc_equals_local_local_localresult(call_context* context)
{
	//equals

	bool ret=(asAtomHandler::isEqual(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1)));
	LOG_CALL("equals_lll "<<ret);

	ASATOM_DECREF(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos));
	asAtomHandler::setBool(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),ret);
	++(context->exec_pos);
}
void ABCVm::abc_lessthan_constant_constant(call_context* context)
{
	bool ret=(asAtomHandler::isLess(*context->exec_pos->arg1_constant,context->worker,*context->exec_pos->arg2_constant)==TTRUE);
	LOG_CALL("lessthan_cc "<<ret);

	RUNTIME_STACK_PUSH(context,asAtomHandler::fromBool(ret));
	++(context->exec_pos);
}
void ABCVm::abc_lessthan_local_constant(call_context* context)
{
	bool ret=(asAtomHandler::isLess(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),context->worker,*context->exec_pos->arg2_constant)==TTRUE);
	LOG_CALL("lessthan_lc "<<ret);

	RUNTIME_STACK_PUSH(context,asAtomHandler::fromBool(ret));
	++(context->exec_pos);
}
void ABCVm::abc_lessthan_constant_local(call_context* context)
{
	bool ret=(asAtomHandler::isLess(*context->exec_pos->arg1_constant,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2))==TTRUE);
	LOG_CALL("lessthan_cl "<<ret);

	RUNTIME_STACK_PUSH(context,asAtomHandler::fromBool(ret));
	++(context->exec_pos);
}
void ABCVm::abc_lessthan_local_local(call_context* context)
{
	bool ret=(asAtomHandler::isLess(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2))==TTRUE);
	LOG_CALL("lessthan_ll "<<ret);

	RUNTIME_STACK_PUSH(context,asAtomHandler::fromBool(ret));
	++(context->exec_pos);
}
void ABCVm::abc_lessthan_constant_constant_localresult(call_context* context)
{
	bool ret=(asAtomHandler::isLess(*context->exec_pos->arg1_constant,context->worker,*context->exec_pos->arg2_constant)==TTRUE);
	LOG_CALL("lessthan_ccl "<<ret);

	ASATOM_DECREF(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos));
	asAtomHandler::setBool(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),ret);
	++(context->exec_pos);
}
void ABCVm::abc_lessthan_local_constant_localresult(call_context* context)
{
	bool ret=(asAtomHandler::isLess(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),context->worker,*context->exec_pos->arg2_constant)==TTRUE);
	LOG_CALL("lessthan_lcl "<<ret);

	ASATOM_DECREF(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos));
	asAtomHandler::setBool(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),ret);
	++(context->exec_pos);
}
void ABCVm::abc_lessthan_constant_local_localresult(call_context* context)
{
	bool ret=(asAtomHandler::isLess(*context->exec_pos->arg1_constant,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2))==TTRUE);
	LOG_CALL("lessthan_cll "<<ret);

	ASATOM_DECREF(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos));
	asAtomHandler::setBool(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),ret);
	++(context->exec_pos);
}
void ABCVm::abc_lessthan_local_local_localresult(call_context* context)
{
	bool ret=(asAtomHandler::isLess(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2))==TTRUE);
	LOG_CALL("lessthan_lll "<<ret);

	ASATOM_DECREF(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos));
	asAtomHandler::setBool(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),ret);
	++(context->exec_pos);
}
void ABCVm::abc_lessequals_constant_constant(call_context* context)
{
	bool ret=(asAtomHandler::isLess(*context->exec_pos->arg2_constant,context->worker,*context->exec_pos->arg1_constant)==TFALSE);
	LOG_CALL("lessequals_cc "<<ret);

	RUNTIME_STACK_PUSH(context,asAtomHandler::fromBool(ret));
	++(context->exec_pos);
}
void ABCVm::abc_lessequals_local_constant(call_context* context)
{
	bool ret=(asAtomHandler::isLess(*context->exec_pos->arg2_constant,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1))==TFALSE);
	LOG_CALL("lessequals_lc "<<ret);

	RUNTIME_STACK_PUSH(context,asAtomHandler::fromBool(ret));
	++(context->exec_pos);
}
void ABCVm::abc_lessequals_constant_local(call_context* context)
{
	bool ret=(asAtomHandler::isLess(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->worker,*context->exec_pos->arg1_constant)==TFALSE);
	LOG_CALL("lessequals_cl "<<ret);

	RUNTIME_STACK_PUSH(context,asAtomHandler::fromBool(ret));
	++(context->exec_pos);
}
void ABCVm::abc_lessequals_local_local(call_context* context)
{
	bool ret=(asAtomHandler::isLess(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1))==TFALSE);
	LOG_CALL("lessequals_ll "<<ret);

	RUNTIME_STACK_PUSH(context,asAtomHandler::fromBool(ret));
	++(context->exec_pos);
}
void ABCVm::abc_lessequals_constant_constant_localresult(call_context* context)
{
	bool ret=(asAtomHandler::isLess(*context->exec_pos->arg2_constant,context->worker,*context->exec_pos->arg1_constant)==TFALSE);
	LOG_CALL("lessequals_ccl "<<ret);

	ASATOM_DECREF(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos));
	asAtomHandler::setBool(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),ret);
	++(context->exec_pos);
}
void ABCVm::abc_lessequals_local_constant_localresult(call_context* context)
{
	bool ret=(asAtomHandler::isLess(*context->exec_pos->arg2_constant,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1))==TFALSE);
	LOG_CALL("lessequals_lcl "<<ret);

	ASATOM_DECREF(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos));
	asAtomHandler::setBool(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),ret);
	++(context->exec_pos);
}
void ABCVm::abc_lessequals_constant_local_localresult(call_context* context)
{
	bool ret=(asAtomHandler::isLess(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->worker,*context->exec_pos->arg1_constant)==TFALSE);
	LOG_CALL("lessequals_cll "<<ret);

	ASATOM_DECREF(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos));
	asAtomHandler::setBool(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),ret);
	++(context->exec_pos);
}
void ABCVm::abc_lessequals_local_local_localresult(call_context* context)
{
	bool ret=(asAtomHandler::isLess(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1))==TFALSE);
	LOG_CALL("lessequals_lll "<<ret);

	ASATOM_DECREF(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos));
	asAtomHandler::setBool(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),ret);
	++(context->exec_pos);
}
void ABCVm::abc_greaterthan_constant_constant(call_context* context)
{
	bool ret=(asAtomHandler::isLess(*context->exec_pos->arg2_constant,context->worker,*context->exec_pos->arg1_constant)==TTRUE);
	LOG_CALL("greaterThan_cc "<<ret);

	RUNTIME_STACK_PUSH(context,asAtomHandler::fromBool(ret));
	++(context->exec_pos);
}
void ABCVm::abc_greaterthan_local_constant(call_context* context)
{
	bool ret=(asAtomHandler::isLess(*context->exec_pos->arg2_constant,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1))==TTRUE);
	LOG_CALL("greaterThan_lc "<<ret);

	RUNTIME_STACK_PUSH(context,asAtomHandler::fromBool(ret));
	++(context->exec_pos);
}
void ABCVm::abc_greaterthan_constant_local(call_context* context)
{
	bool ret=(asAtomHandler::isLess(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->worker,*context->exec_pos->arg1_constant)==TTRUE);
	LOG_CALL("greaterThan_cl "<<ret);

	RUNTIME_STACK_PUSH(context,asAtomHandler::fromBool(ret));
	++(context->exec_pos);
}
void ABCVm::abc_greaterthan_local_local(call_context* context)
{
	bool ret=(asAtomHandler::isLess(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1))==TTRUE);
	LOG_CALL("greaterThan_ll "<<ret);

	RUNTIME_STACK_PUSH(context,asAtomHandler::fromBool(ret));
	++(context->exec_pos);
}
void ABCVm::abc_greaterthan_constant_constant_localresult(call_context* context)
{
	bool ret=(asAtomHandler::isLess(*context->exec_pos->arg2_constant,context->worker,*context->exec_pos->arg1_constant)==TTRUE);
	LOG_CALL("greaterThan_ccl "<<ret);

	ASATOM_DECREF(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos));
	asAtomHandler::setBool(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),ret);
	++(context->exec_pos);
}
void ABCVm::abc_greaterthan_local_constant_localresult(call_context* context)
{
	bool ret=(asAtomHandler::isLess(*context->exec_pos->arg2_constant,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1))==TTRUE);
	LOG_CALL("greaterThan_lcl "<<ret);

	ASATOM_DECREF(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos));
	asAtomHandler::setBool(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),ret);
	++(context->exec_pos);
}
void ABCVm::abc_greaterthan_constant_local_localresult(call_context* context)
{
	bool ret=(asAtomHandler::isLess(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->worker,*context->exec_pos->arg1_constant)==TTRUE);
	LOG_CALL("greaterThan_cll "<<ret);

	ASATOM_DECREF(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos));
	asAtomHandler::setBool(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),ret);
	++(context->exec_pos);
}
void ABCVm::abc_greaterthan_local_local_localresult(call_context* context)
{
	bool ret=(asAtomHandler::isLess(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1))==TTRUE);
	LOG_CALL("greaterThan_lll "<<ret);

	ASATOM_DECREF(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos));
	asAtomHandler::setBool(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),ret);
	++(context->exec_pos);
}
void ABCVm::abc_greaterequals_constant_constant(call_context* context)
{
	bool ret=(asAtomHandler::isLess(*context->exec_pos->arg1_constant,context->worker,*context->exec_pos->arg2_constant)==TFALSE);
	LOG_CALL("greaterequals_cc "<<ret);

	RUNTIME_STACK_PUSH(context,asAtomHandler::fromBool(ret));
	++(context->exec_pos);
}
void ABCVm::abc_greaterequals_local_constant(call_context* context)
{
	bool ret=(asAtomHandler::isLess(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),context->worker,*context->exec_pos->arg2_constant)==TFALSE);
	LOG_CALL("greaterequals_lc "<<ret);

	RUNTIME_STACK_PUSH(context,asAtomHandler::fromBool(ret));
	++(context->exec_pos);
}
void ABCVm::abc_greaterequals_constant_local(call_context* context)
{
	bool ret=(asAtomHandler::isLess(*context->exec_pos->arg1_constant,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2))==TFALSE);
	LOG_CALL("greaterequals_cl "<<ret);

	RUNTIME_STACK_PUSH(context,asAtomHandler::fromBool(ret));
	++(context->exec_pos);
}
void ABCVm::abc_greaterequals_local_local(call_context* context)
{
	bool ret=(asAtomHandler::isLess(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2))==TFALSE);
	LOG_CALL("greaterequals_ll "<<ret);

	RUNTIME_STACK_PUSH(context,asAtomHandler::fromBool(ret));
	++(context->exec_pos);
}
void ABCVm::abc_greaterequals_constant_constant_localresult(call_context* context)
{
	bool ret=(asAtomHandler::isLess(*context->exec_pos->arg1_constant,context->worker,*context->exec_pos->arg2_constant)==TFALSE);
	LOG_CALL("greaterequals_ccl "<<ret);

	ASATOM_DECREF(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos));
	asAtomHandler::setBool(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),ret);
	++(context->exec_pos);
}
void ABCVm::abc_greaterequals_local_constant_localresult(call_context* context)
{
	bool ret=(asAtomHandler::isLess(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),context->worker,*context->exec_pos->arg2_constant)==TFALSE);
	LOG_CALL("greaterequals_lcl "<<ret);

	ASATOM_DECREF(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos));
	asAtomHandler::setBool(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),ret);
	++(context->exec_pos);
}
void ABCVm::abc_greaterequals_constant_local_localresult(call_context* context)
{
	bool ret=(asAtomHandler::isLess(*context->exec_pos->arg1_constant,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2))==TFALSE);
	LOG_CALL("greaterequals_cll "<<ret);

	ASATOM_DECREF(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos));
	asAtomHandler::setBool(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),ret);
	++(context->exec_pos);
}
void ABCVm::abc_greaterequals_local_local_localresult(call_context* context)
{
	bool ret=(asAtomHandler::isLess(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2))==TFALSE);
	LOG_CALL("greaterequals_lll "<<ret<<" "<<asAtomHandler::toDebugString(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1))<<" "<<asAtomHandler::toDebugString(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2)));

	ASATOM_DECREF(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos));
	asAtomHandler::setBool(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),ret);
	++(context->exec_pos);
}

void ABCVm::abc_istypelate_constant_constant(call_context* context)
{
	bool ret=asAtomHandler::isTypelate(*context->exec_pos->arg1_constant,*context->exec_pos->arg2_constant);
	LOG_CALL("istypelate_cc "<<ret);

	RUNTIME_STACK_PUSH(context,asAtomHandler::fromBool(ret));
	++(context->exec_pos);
}
void ABCVm::abc_istypelate_local_constant(call_context* context)
{
	bool ret=asAtomHandler::isTypelate(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),*context->exec_pos->arg2_constant);
	LOG_CALL("istypelate_lc "<<ret);

	RUNTIME_STACK_PUSH(context,asAtomHandler::fromBool(ret));
	++(context->exec_pos);
}
void ABCVm::abc_istypelate_constant_local(call_context* context)
{
	bool ret=asAtomHandler::isTypelate(*context->exec_pos->arg1_constant,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2));
	LOG_CALL("istypelate_cl "<<ret);

	RUNTIME_STACK_PUSH(context,asAtomHandler::fromBool(ret));
	++(context->exec_pos);
}
void ABCVm::abc_istypelate_local_local(call_context* context)
{
	bool ret=asAtomHandler::isTypelate(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2));
	LOG_CALL("istypelate_ll "<<ret);

	RUNTIME_STACK_PUSH(context,asAtomHandler::fromBool(ret));
	++(context->exec_pos);
}
void ABCVm::abc_istypelate_constant_constant_localresult(call_context* context)
{
	bool ret=asAtomHandler::isTypelate(*context->exec_pos->arg1_constant,*context->exec_pos->arg2_constant);
	LOG_CALL("istypelate_ccl "<<ret);

	ASATOM_DECREF(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos));
	asAtomHandler::setBool(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),ret);
	++(context->exec_pos);
}
void ABCVm::abc_istypelate_local_constant_localresult(call_context* context)
{
	bool ret=asAtomHandler::isTypelate(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),*context->exec_pos->arg2_constant);
	LOG_CALL("istypelate_lcl "<<ret);

	ASATOM_DECREF(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos));
	asAtomHandler::setBool(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),ret);
	++(context->exec_pos);
}
void ABCVm::abc_istypelate_constant_local_localresult(call_context* context)
{
	bool ret=asAtomHandler::isTypelate(*context->exec_pos->arg1_constant,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2));
	LOG_CALL("istypelate_cll "<<ret);

	ASATOM_DECREF(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos));
	asAtomHandler::setBool(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),ret);
	++(context->exec_pos);
}
void ABCVm::abc_istypelate_local_local_localresult(call_context* context)
{
	bool ret=asAtomHandler::isTypelate(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2));
	LOG_CALL("istypelate_lll "<<ret<<" "<<asAtomHandler::toDebugString(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1))<<" "<<asAtomHandler::toDebugString(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2)));

	ASATOM_DECREF(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos));
	asAtomHandler::setBool(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),ret);
	++(context->exec_pos);
}

void ABCVm::abc_instanceof_constant_constant(call_context* context)
{
	bool ret = instanceOf(asAtomHandler::toObject(*context->exec_pos->arg1_constant,context->worker),asAtomHandler::toObject(*context->exec_pos->arg2_constant,context->worker));
	LOG_CALL("instanceof_cc "<<ret);

	RUNTIME_STACK_PUSH(context,asAtomHandler::fromBool(ret));
	++(context->exec_pos);
}
void ABCVm::abc_instanceof_local_constant(call_context* context)
{
	bool ret = instanceOf(asAtomHandler::toObject(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),context->worker),asAtomHandler::toObject(*context->exec_pos->arg2_constant,context->worker));
	LOG_CALL("instanceof_lc "<<ret);

	RUNTIME_STACK_PUSH(context,asAtomHandler::fromBool(ret));
	++(context->exec_pos);
}
void ABCVm::abc_instanceof_constant_local(call_context* context)
{
	bool ret = instanceOf(asAtomHandler::toObject(*context->exec_pos->arg1_constant,context->worker),asAtomHandler::toObject(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->worker));
	LOG_CALL("instanceof_cl "<<ret);

	RUNTIME_STACK_PUSH(context,asAtomHandler::fromBool(ret));
	++(context->exec_pos);
}
void ABCVm::abc_instanceof_local_local(call_context* context)
{
	bool ret = instanceOf(asAtomHandler::toObject(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),context->worker),asAtomHandler::toObject(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->worker));
	LOG_CALL("instanceof_ll "<<ret);

	RUNTIME_STACK_PUSH(context,asAtomHandler::fromBool(ret));
	++(context->exec_pos);
}
void ABCVm::abc_instanceof_constant_constant_localresult(call_context* context)
{
	bool ret = instanceOf(asAtomHandler::toObject(*context->exec_pos->arg1_constant,context->worker),asAtomHandler::toObject(*context->exec_pos->arg2_constant,context->worker));
	LOG_CALL("instanceof_ccl "<<ret);

	ASATOM_DECREF(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos));
	asAtomHandler::setBool(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),ret);
	++(context->exec_pos);
}
void ABCVm::abc_instanceof_local_constant_localresult(call_context* context)
{
	bool ret = instanceOf(asAtomHandler::toObject(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),context->worker),asAtomHandler::toObject(*context->exec_pos->arg2_constant,context->worker));
	LOG_CALL("instanceof_lcl "<<ret);

	ASATOM_DECREF(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos));
	asAtomHandler::setBool(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),ret);
	++(context->exec_pos);
}
void ABCVm::abc_instanceof_constant_local_localresult(call_context* context)
{
	bool ret = instanceOf(asAtomHandler::toObject(*context->exec_pos->arg1_constant,context->worker),asAtomHandler::toObject(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->worker));
	LOG_CALL("instanceof_cll "<<ret);

	ASATOM_DECREF(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos));
	asAtomHandler::setBool(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),ret);
	++(context->exec_pos);
}
void ABCVm::abc_instanceof_local_local_localresult(call_context* context)
{
	bool ret = instanceOf(asAtomHandler::toObject(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),context->worker),asAtomHandler::toObject(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->worker));
	LOG_CALL("instanceof_lll "<<ret<<" "<<asAtomHandler::toDebugString(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1))<<" "<<asAtomHandler::toDebugString(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2)));

	ASATOM_DECREF(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos));
	asAtomHandler::setBool(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),ret);
	++(context->exec_pos);
}

void ABCVm::abc_increment_i_local(call_context* context)
{
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	LOG_CALL("increment_i_l "<<context->exec_pos->local_pos1<<" "<<asAtomHandler::toDebugString(res));
	asAtomHandler::increment_i(res,context->worker);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_increment_i_local_localresult(call_context* context)
{
	LOG_CALL("increment_i_ll "<<context->exec_pos->local_pos1<<" "<<context->exec_pos->local3.pos<<" "<<asAtomHandler::toDebugString(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1)));
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	asAtom oldres = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	asAtomHandler::set(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),res);
	asAtomHandler::increment_i(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),context->worker);
	ASATOM_DECREF(oldres);
	++(context->exec_pos);
}
void ABCVm::abc_increment_i_local_setslotnocoerce(call_context* context)
{
	LOG_CALL("increment_i_ls "<<context->exec_pos->local_pos1<<" "<<context->exec_pos->local3.pos<<" "<<asAtomHandler::toDebugString(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1)));
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	asAtomHandler::increment_i(res,context->worker);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}
void ABCVm::abc_decrement_i_local(call_context* context)
{
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	LOG_CALL("decrement_i_l "<<context->exec_pos->local_pos1<<" "<<asAtomHandler::toDebugString(res));
	asAtomHandler::decrement_i(res,context->worker);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_decrement_i_local_localresult(call_context* context)
{
	LOG_CALL("decrement_i_ll "<<context->exec_pos->local_pos1<<" "<<context->exec_pos->local3.pos<<" "<<asAtomHandler::toDebugString(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos)));
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	asAtom oldres = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	asAtomHandler::set(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),res);
	asAtomHandler::decrement_i(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),context->worker);
	ASATOM_DECREF(oldres);
	++(context->exec_pos);
}
void ABCVm::abc_decrement_i_local_setslotnocoerce(call_context* context)
{
	LOG_CALL("decrement_i_ls "<<context->exec_pos->local_pos1<<" "<<context->exec_pos->local3.pos<<" "<<asAtomHandler::toDebugString(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1)));
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	asAtomHandler::decrement_i(res,context->worker);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}
void ABCVm::abc_add_i_constant_constant(call_context* context)
{
	asAtom res = asAtomHandler::invalidAtom;
	int64_t num1=context->exec_pos->arg1_int;
	int64_t num2=context->exec_pos->arg2_int;
	LOG_CALL("add_i_cc " << num1 << '+' << num2);
	int64_t ret = num1+num2;
	if (ret >= INT32_MAX || ret <= INT32_MIN)
		asAtomHandler::setNumber(res,context->worker,ret);
	else
		asAtomHandler::setInt(res,context->worker,ret);

	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_add_i_local_constant(call_context* context)
{
	asAtom res = asAtomHandler::invalidAtom;
	int64_t num1=asAtomHandler::toInt64(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1));
	int64_t num2=context->exec_pos->arg2_int;
	LOG_CALL("add_i_lc " << num1 << '+' << num2);
	int64_t ret = num1+num2;
	if (ret >= INT32_MAX || ret <= INT32_MIN)
		asAtomHandler::setNumber(res,context->worker,ret);
	else
		asAtomHandler::setInt(res,context->worker,ret);

	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_add_i_constant_local(call_context* context)
{
	asAtom res = asAtomHandler::invalidAtom;
	int64_t num1=context->exec_pos->arg1_int;
	int64_t num2=asAtomHandler::toInt64(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2));
	LOG_CALL("add_i_lc " << num1 << '+' << num2);
	int64_t ret = num1+num2;
	if (ret >= INT32_MAX || ret <= INT32_MIN)
		asAtomHandler::setNumber(res,context->worker,ret);
	else
		asAtomHandler::setInt(res,context->worker,ret);

	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_add_i_local_local(call_context* context)
{
	LOG_CALL("add_i_ll");
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	asAtomHandler::add_i(res,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2));
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_add_i_constant_constant_localresult(call_context* context)
{
	asAtom res;
	int64_t num1=context->exec_pos->arg1_int;
	int64_t num2=context->exec_pos->arg2_int;
	LOG_CALL("add_i_ccl " << num1 << '+' << num2);
	int64_t ret = num1+num2;
	if (context->exec_pos->local3.flags & ABC_OP_FORCEINT || (ret < INT32_MAX && ret > INT32_MIN))
		asAtomHandler::setInt(res,context->worker,(int32_t)ret);
	else
		asAtomHandler::setNumber(res,context->worker,ret);
	replacelocalresult(context,context->exec_pos->local3.pos,res);
	++(context->exec_pos);
}
void ABCVm::abc_add_i_local_constant_localresult(call_context* context)
{
	asAtom arg1 = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	asAtom oldres = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	if (USUALLY_TRUE(
#ifdef LIGHTSPARK_64
			((arg1.uintval & 0xc000000000000007) ==ATOM_INTEGER)
#else
			((context->exec_pos->arg2_int & 0xc0000007) ==ATOM_INTEGER ) && ((arg1.uintval & 0xc0000007) ==ATOM_INTEGER )
#endif
			&& !asAtomHandler::isObject(oldres)))
	{
		// fast path for common case that both arguments are ints and the result doesn't overflow
		asAtom res = asAtomHandler::fromInt(context->exec_pos->arg2_int);
		LOG_CALL("add_i_lcl_fast");
		res.intval += arg1.intval-ATOM_INTEGER;
		asAtomHandler::set(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),res);
	}
	else
	{
		asAtom res;
		int64_t num1=asAtomHandler::toInt64(arg1);
		int64_t num2=context->exec_pos->arg2_int;
		LOG_CALL("add_i_lcl " << num1 << '+' << num2<<" "<<hex<<context->exec_pos->local3.flags);
		int64_t ret = num1+num2;
		if (context->exec_pos->local3.flags & ABC_OP_FORCEINT || (ret < INT32_MAX && ret > INT32_MIN))
			asAtomHandler::setInt(res,context->worker,(int32_t)ret);
		else
			asAtomHandler::setNumber(res,context->worker,ret);
		replacelocalresult(context,context->exec_pos->local3.pos,res);
	}
	++(context->exec_pos);
}
void ABCVm::abc_add_i_constant_local_localresult(call_context* context)
{
	asAtom arg2 = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2);
	asAtom oldres = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	if (USUALLY_TRUE(
#ifdef LIGHTSPARK_64
			((arg2.uintval & 0xc000000000000007) ==ATOM_INTEGER)
#else
			((context->exec_pos->arg1_int & 0xc0000007) ==ATOM_INTEGER ) && ((arg2.uintval & 0xc0000007) ==ATOM_INTEGER )
#endif
			&& !asAtomHandler::isObject(oldres)))
	{
		// fast path for common case that both arguments are ints and the result doesn't overflow
		asAtom res = asAtomHandler::fromInt(context->exec_pos->arg1_int);
		LOG_CALL("add_i_cll_fast");
		res.intval += arg2.intval-ATOM_INTEGER;
		asAtomHandler::set(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),res);
	}
	else
	{
		asAtom res;
		int64_t num1=context->exec_pos->arg1_int;
		int64_t num2=asAtomHandler::toInt64(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2));
		LOG_CALL("add_i_cll " << num1 << '+' << num2);
		int64_t ret = num1+num2;
		if (context->exec_pos->local3.flags & ABC_OP_FORCEINT || (ret < INT32_MAX && ret > INT32_MIN))
			asAtomHandler::setInt(res,context->worker,(int32_t)ret);
		else
			asAtomHandler::setNumber(res,context->worker,ret);
		replacelocalresult(context,context->exec_pos->local3.pos,res);
	}
	++(context->exec_pos);
}
void ABCVm::abc_add_i_local_local_localresult(call_context* context)
{
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	asAtom arg2 = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2);
	asAtom oldres = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	if (USUALLY_TRUE(
#ifdef LIGHTSPARK_64
			(((res.uintval | arg2.uintval) & 0xc000000000000007) ==ATOM_INTEGER)
#else
			(((res.uintval | arg2.uintval) & 0xc0000007) ==ATOM_INTEGER)
#endif
			&& !asAtomHandler::isObject(oldres)))
	{
		// fast path for common case that both arguments are ints and the result doesn't overflow
		LOG_CALL("add_i_lll_fast");
		res.intval += arg2.intval-ATOM_INTEGER;
		asAtomHandler::set(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),res);
	}
	else
	{
		LOG_CALL("add_i_lll");
		asAtomHandler::add_i(res,context->worker,arg2);
		replacelocalresult(context,context->exec_pos->local3.pos,res);
	}
	++(context->exec_pos);
}
void ABCVm::abc_add_i_constant_constant_setslotnocoerce(call_context* context)
{
	asAtom res;
	int64_t num1=context->exec_pos->arg1_int;
	int64_t num2=context->exec_pos->arg2_int;
	int64_t ret = num1+num2;
	if (context->exec_pos->local3.flags & ABC_OP_FORCEINT || (ret < INT32_MAX && ret > INT32_MIN))
		asAtomHandler::setInt(res,context->worker,(int32_t)ret);
	else
		asAtomHandler::setNumber(res,context->worker,ret);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	LOG_CALL("add_i_ccs " << t << " "<< asAtomHandler::toDebugString(res) << " "<< asAtomHandler::toDebugString(obj));
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}
void ABCVm::abc_add_i_local_constant_setslotnocoerce(call_context* context)
{
	asAtom res;
	int64_t num1=asAtomHandler::toInt64(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1));
	int64_t num2=context->exec_pos->arg2_int;
	int64_t ret = num1+num2;
	if (context->exec_pos->local3.flags & ABC_OP_FORCEINT || (ret < INT32_MAX && ret > INT32_MIN))
		asAtomHandler::setInt(res,context->worker,(int32_t)ret);
	else
		asAtomHandler::setNumber(res,context->worker,ret);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	LOG_CALL("add_i_lcs " << t << " "<< asAtomHandler::toDebugString(res) << " "<< asAtomHandler::toDebugString(obj));
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}
void ABCVm::abc_add_i_constant_local_setslotnocoerce(call_context* context)
{
	asAtom res;
	int64_t num1=context->exec_pos->arg1_int;
	int64_t num2=asAtomHandler::toInt64(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2));
	int64_t ret = num1+num2;
	if (context->exec_pos->local3.flags & ABC_OP_FORCEINT || (ret < INT32_MAX && ret > INT32_MIN))
		asAtomHandler::setInt(res,context->worker,(int32_t)ret);
	else
		asAtomHandler::setNumber(res,context->worker,ret);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	LOG_CALL("add_i_cls " << t << " "<< asAtomHandler::toDebugString(res) << " "<< asAtomHandler::toDebugString(obj));
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}
void ABCVm::abc_add_i_local_local_setslotnocoerce(call_context* context)
{
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	asAtomHandler::add_i(res,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2));

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	LOG_CALL("add_i_lls " << t << " "<< asAtomHandler::toDebugString(res) << " "<< asAtomHandler::toDebugString(obj));
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}

void ABCVm::abc_subtract_i_constant_constant(call_context* context)
{
	asAtom res = asAtomHandler::invalidAtom;
	int64_t num1=context->exec_pos->arg1_int;
	int64_t num2=context->exec_pos->arg2_int;
	LOG_CALL("subtract_i_cc " << num1 << '-' << num2);
	int64_t ret = num1-num2;
	asAtomHandler::setInt(res,context->worker,(int32_t)ret);

	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_subtract_i_local_constant(call_context* context)
{
	asAtom res = asAtomHandler::invalidAtom;
	int64_t num1=asAtomHandler::toInt64(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1));
	int64_t num2=context->exec_pos->arg2_int;
	LOG_CALL("subtract_i_lc " << num1 << '-' << num2);
	int64_t ret = num1-num2;
	asAtomHandler::setInt(res,context->worker,(int32_t)ret);

	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_subtract_i_constant_local(call_context* context)
{
	asAtom res = asAtomHandler::invalidAtom;
	int64_t num1=context->exec_pos->arg1_int;
	int64_t num2=asAtomHandler::toInt64(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2));
	LOG_CALL("subtract_i_lc " << num1 << '-' << num2);
	int64_t ret = num1-num2;
	asAtomHandler::setInt(res,context->worker,(int32_t)ret);

	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_subtract_i_local_local(call_context* context)
{
	LOG_CALL("subtract_i_ll");
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	asAtomHandler::subtract_i(res,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2));
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_subtract_i_constant_constant_localresult(call_context* context)
{
	asAtom res;
	int64_t num1=context->exec_pos->arg1_int;
	int64_t num2=context->exec_pos->arg2_int;
	LOG_CALL("subtract_i_ccl " << num1 << '-' << num2);
	int64_t ret = num1-num2;
	asAtomHandler::setInt(res,context->worker,(int32_t)ret);

	replacelocalresult(context,context->exec_pos->local3.pos,res);
	++(context->exec_pos);
}
void ABCVm::abc_subtract_i_local_constant_localresult(call_context* context)
{
	asAtom res;
	int64_t num1=asAtomHandler::toInt64(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1));
	int64_t num2=context->exec_pos->arg2_int;
	LOG_CALL("subtract_i_lcl " << num1 << '-' << num2<<" "<<hex<<context->exec_pos->local3.flags);
	int64_t ret = num1-num2;
	asAtomHandler::setInt(res,context->worker,(int32_t)ret);

	replacelocalresult(context,context->exec_pos->local3.pos,res);
	++(context->exec_pos);
}
void ABCVm::abc_subtract_i_constant_local_localresult(call_context* context)
{
	asAtom res;
	int64_t num1=context->exec_pos->arg1_int;
	int64_t num2=asAtomHandler::toInt64(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2));
	LOG_CALL("subtract_i_cll " << num1 << '-' << num2);
	int64_t ret = num1-num2;
	asAtomHandler::setInt(res,context->worker,(int32_t)ret);

	replacelocalresult(context,context->exec_pos->local3.pos,res);
	++(context->exec_pos);
}
void ABCVm::abc_subtract_i_local_local_localresult(call_context* context)
{
	LOG_CALL("subtract_i_lll");
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	asAtomHandler::subtract_i(res,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2));
	replacelocalresult(context,context->exec_pos->local3.pos,res);
	++(context->exec_pos);
}
void ABCVm::abc_subtract_i_constant_constant_setslotnocoerce(call_context* context)
{
	asAtom res;
	int64_t num1=context->exec_pos->arg1_int;
	int64_t num2=context->exec_pos->arg2_int;
	int64_t ret = num1-num2;
	asAtomHandler::setInt(res,context->worker,(int32_t)ret);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	LOG_CALL("subtract_i_ccs " << t << " "<< asAtomHandler::toDebugString(res) << " "<< asAtomHandler::toDebugString(obj));
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}
void ABCVm::abc_subtract_i_local_constant_setslotnocoerce(call_context* context)
{
	asAtom res;
	int64_t num1=asAtomHandler::toInt64(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1));
	int64_t num2=context->exec_pos->arg2_int;
	int64_t ret = num1-num2;
	asAtomHandler::setInt(res,context->worker,(int32_t)ret);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	LOG_CALL("subtract_i_lcs " << t << " "<< asAtomHandler::toDebugString(res) << " "<< asAtomHandler::toDebugString(obj));
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}
void ABCVm::abc_subtract_i_constant_local_setslotnocoerce(call_context* context)
{
	asAtom res;
	int64_t num1=context->exec_pos->arg1_int;
	int64_t num2=asAtomHandler::toInt64(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2));
	int64_t ret = num1-num2;
	asAtomHandler::setInt(res,context->worker,(int32_t)ret);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	LOG_CALL("subtract_i_cls " << t << " "<< asAtomHandler::toDebugString(res) << " "<< asAtomHandler::toDebugString(obj));
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}
void ABCVm::abc_subtract_i_local_local_setslotnocoerce(call_context* context)
{
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	asAtomHandler::subtract_i(res,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2));

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	LOG_CALL("subtract_i_lls " << t << " "<< asAtomHandler::toDebugString(res) << " "<< asAtomHandler::toDebugString(obj));
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}

void ABCVm::abc_multiply_i_constant_constant(call_context* context)
{
	asAtom res = asAtomHandler::invalidAtom;
	int64_t num1=context->exec_pos->arg1_int;
	int64_t num2=context->exec_pos->arg2_int;
	LOG_CALL("multiply_i_cc " << num1 << '*' << num2);
	int64_t ret = num1*num2;
	asAtomHandler::setInt(res,context->worker,(int32_t)ret);

	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_multiply_i_local_constant(call_context* context)
{
	asAtom res = asAtomHandler::invalidAtom;
	int64_t num1=asAtomHandler::toInt64(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1));
	int64_t num2=context->exec_pos->arg2_int;
	LOG_CALL("multiply_i_lc " << num1 << '*' << num2);
	int64_t ret = num1*num2;
	asAtomHandler::setInt(res,context->worker,(int32_t)ret);

	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_multiply_i_constant_local(call_context* context)
{
	asAtom res = asAtomHandler::invalidAtom;
	int64_t num1=context->exec_pos->arg1_int;
	int64_t num2=asAtomHandler::toInt64(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2));
	LOG_CALL("multiply_i_lc " << num1 << '*' << num2);
	int64_t ret = num1*num2;
	asAtomHandler::setInt(res,context->worker,(int32_t)ret);

	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_multiply_i_local_local(call_context* context)
{
	LOG_CALL("multiply_i_ll");
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	asAtomHandler::multiply_i(res,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2));
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_multiply_i_constant_constant_localresult(call_context* context)
{
	asAtom res;
	int64_t num1=context->exec_pos->arg1_int;
	int64_t num2=context->exec_pos->arg2_int;
	LOG_CALL("multiply_i_ccl " << num1 << '*' << num2);
	int64_t ret = num1*num2;
	asAtomHandler::setInt(res,context->worker,(int32_t)ret);

	replacelocalresult(context,context->exec_pos->local3.pos,res);
	++(context->exec_pos);
}
void ABCVm::abc_multiply_i_local_constant_localresult(call_context* context)
{
	asAtom res;
	int64_t num1=asAtomHandler::toInt64(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1));
	int64_t num2=context->exec_pos->arg2_int;
	LOG_CALL("multiply_i_lcl " << num1 << '*' << num2<<" "<<hex<<context->exec_pos->local3.flags);
	int64_t ret = num1*num2;
	asAtomHandler::setInt(res,context->worker,(int32_t)ret);
	replacelocalresult(context,context->exec_pos->local3.pos,res);
	++(context->exec_pos);
}
void ABCVm::abc_multiply_i_constant_local_localresult(call_context* context)
{
	asAtom res;
	int64_t num1=context->exec_pos->arg1_int;
	int64_t num2=asAtomHandler::toInt64(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2));
	LOG_CALL("multiply_i_cll " << num1 << '*' << num2);
	int64_t ret = num1*num2;
	asAtomHandler::setInt(res,context->worker,(int32_t)ret);
	replacelocalresult(context,context->exec_pos->local3.pos,res);
	++(context->exec_pos);
}
void ABCVm::abc_multiply_i_local_local_localresult(call_context* context)
{
	LOG_CALL("multiply_i_lll");
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	asAtomHandler::multiply_i(res,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2));
	replacelocalresult(context,context->exec_pos->local3.pos,res);
	++(context->exec_pos);
}
void ABCVm::abc_multiply_i_constant_constant_setslotnocoerce(call_context* context)
{
	asAtom res;
	int64_t num1=context->exec_pos->arg1_int;
	int64_t num2=context->exec_pos->arg2_int;
	int64_t ret = num1*num2;
	asAtomHandler::setInt(res,context->worker,(int32_t)ret);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	LOG_CALL("multiply_i_ccs " << t << " "<< asAtomHandler::toDebugString(res) << " "<< asAtomHandler::toDebugString(obj));
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}
void ABCVm::abc_multiply_i_local_constant_setslotnocoerce(call_context* context)
{
	asAtom res;
	int64_t num1=asAtomHandler::toInt64(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1));
	int64_t num2=context->exec_pos->arg2_int;
	int64_t ret = num1*num2;
	asAtomHandler::setInt(res,context->worker,(int32_t)ret);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	LOG_CALL("multiply_i_lcs " << t << " "<< asAtomHandler::toDebugString(res) << " "<< asAtomHandler::toDebugString(obj));
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}
void ABCVm::abc_multiply_i_constant_local_setslotnocoerce(call_context* context)
{
	asAtom res;
	int64_t num1=context->exec_pos->arg1_int;
	int64_t num2=asAtomHandler::toInt64(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2));
	int64_t ret = num1*num2;
	asAtomHandler::setInt(res,context->worker,(int32_t)ret);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	LOG_CALL("multiply_i_cls " << t << " "<< asAtomHandler::toDebugString(res) << " "<< asAtomHandler::toDebugString(obj));
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}
void ABCVm::abc_multiply_i_local_local_setslotnocoerce(call_context* context)
{
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	asAtomHandler::multiply_i(res,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2));

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	LOG_CALL("multiply_i_lls " << t << " "<< asAtomHandler::toDebugString(res) << " "<< asAtomHandler::toDebugString(obj));
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}


void ABCVm::abc_inclocal_i_optimized(call_context* context)
{
	int32_t t = context->exec_pos->arg1_uint;
	LOG_CALL( "incLocal_i_o " << t << " "<< context->exec_pos->arg2_int);
	asAtomHandler::increment_i(CONTEXT_GETLOCAL(context,t),context->worker,context->exec_pos->arg2_int);
	++context->exec_pos;
}
void ABCVm::abc_declocal_i_optimized(call_context* context)
{
	int32_t t = context->exec_pos->arg1_uint;
	LOG_CALL( "decLocal_i_o " << t << " "<< context->exec_pos->arg2_int);
	asAtomHandler::decrement_i(CONTEXT_GETLOCAL(context,t),context->worker,context->exec_pos->arg2_int);
	++context->exec_pos;
}
void ABCVm::abc_inclocal_i_postfix(call_context* context)
{
	int32_t t = context->exec_pos->arg1_uint;
	asAtom& res = CONTEXT_GETLOCAL(context,t);
	asAtom oldres = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	if (USUALLY_TRUE(
#ifdef LIGHTSPARK_64
			((res.uintval & 0xc000000000000007) ==ATOM_INTEGER)
#else
			((res.uintval & 0xc0000007) ==ATOM_INTEGER )
#endif
			&& !asAtomHandler::isObject(oldres)))
	{
		// fast path for common case that argument is ints and the result doesn't overflow
		LOG_CALL( "incLocal_i_postfix_fast " << t <<" "<<context->exec_pos->local3.pos);
		asAtomHandler::set(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),res);
		res.intval += 8;
	}
	else
	{
		LOG_CALL( "incLocal_i_postfix " << t <<" "<<context->exec_pos->local3.pos);
		replacelocalresult(context,context->exec_pos->local3.pos,res);
		asAtomHandler::increment_i(CONTEXT_GETLOCAL(context,t),context->worker);
	}
	++context->exec_pos;
}
void ABCVm::abc_declocal_i_postfix(call_context* context)
{
	int32_t t = context->exec_pos->arg1_uint;
	asAtom& res = CONTEXT_GETLOCAL(context,t);
	asAtom oldres = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	if (USUALLY_TRUE(
#ifdef LIGHTSPARK_64
			((res.uintval & 0xc000000000000007) ==ATOM_INTEGER)
#else
			((res.uintval & 0xc0000007) ==ATOM_INTEGER )
#endif
			&& !asAtomHandler::isObject(oldres)))
	{
		// fast path for common case that argument is ints and the result doesn't overflow
		LOG_CALL( "decLocal_i_postfix_fast " << t <<" "<<context->exec_pos->local3.pos);
		asAtomHandler::set(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),res);
		res.intval -= 8;
	}
	else
	{
		LOG_CALL( "decLocal_i_postfix " << t <<" "<<context->exec_pos->local3.pos);
		replacelocalresult(context,context->exec_pos->local3.pos,res);
		asAtomHandler::decrement_i(CONTEXT_GETLOCAL(context,t),context->worker);
	}
	++context->exec_pos;
}

void ABCVm::abc_dup_local(call_context* context)
{
	LOG_CALL("dup_l "<<context->exec_pos->local_pos1<<" "<<(context->exec_pos->local3.pos));
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	ASATOM_INCREF(res);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_dup_local_localresult(call_context* context)
{
	LOG_CALL("dup_ll "<<context->exec_pos->local_pos1<<" "<<(context->exec_pos->local3.pos));
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	ASATOM_INCREF(res);
	replacelocalresult(context,context->exec_pos->local3.pos,res);
	++(context->exec_pos);
}
void ABCVm::abc_dup_local_setslotnocoerce(call_context* context)
{
	LOG_CALL("dup_ls "<<context->exec_pos->local_pos1<<" "<<(context->exec_pos->local3.pos));
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	ASATOM_INCREF(res);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res);
	++(context->exec_pos);
}

void ABCVm::abc_dup_increment_local_localresult(call_context* context)
{
	LOG_CALL("dup_increment_ll "<<context->exec_pos->local_pos1<<" "<<context->exec_pos->local_pos2<<" "<<context->exec_pos->local3.pos<<" "<<asAtomHandler::toDebugString(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1)));
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	replacelocalresult(context,context->exec_pos->local_pos2,res);
	asAtomHandler::increment(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->worker);
	replacelocalresult(context,context->exec_pos->local3.pos,res);
	++(context->exec_pos);
}
void ABCVm::abc_dup_decrement_local_localresult(call_context* context)
{
	LOG_CALL("dup_decrement_ll "<<context->exec_pos->local_pos1<<" "<<context->exec_pos->local_pos2<<" "<<context->exec_pos->local3.pos<<" "<<asAtomHandler::toDebugString(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1)));
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	replacelocalresult(context,context->exec_pos->local_pos2,res);
	asAtomHandler::decrement(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->worker);
	replacelocalresult(context,context->exec_pos->local3.pos,res);
	++(context->exec_pos);
}
void ABCVm::abc_dup_increment_i_local_localresult(call_context* context)
{
	LOG_CALL("dup_increment_i_ll "<<context->exec_pos->local_pos1<<" "<<context->exec_pos->local_pos2<<" "<<context->exec_pos->local3.pos<<" "<<asAtomHandler::toDebugString(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1)));
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	replacelocalresult(context,context->exec_pos->local_pos2,res);
	asAtomHandler::increment_i(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->worker);
	replacelocalresult(context,context->exec_pos->local3.pos,res);
	++(context->exec_pos);
}
void ABCVm::abc_dup_decrement_i_local_localresult(call_context* context)
{
	LOG_CALL("dup_decrement_i_ll "<<context->exec_pos->local_pos1<<" "<<context->exec_pos->local_pos2<<" "<<context->exec_pos->local3.pos<<" "<<asAtomHandler::toDebugString(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1)));
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	replacelocalresult(context,context->exec_pos->local_pos2,res);
	asAtomHandler::decrement_i(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->worker);
	replacelocalresult(context,context->exec_pos->local3.pos,res);
	++(context->exec_pos);
}
void ABCVm::abc_getfuncscopeobject(call_context* context)
{
	uint32_t t = context->exec_pos->arg2_uint;
	assert(context->parent_scope_stack->scope.size() > t);
	LOG_CALL("getfuncscopeobject "<<t<<" "<<asAtomHandler::toDebugString(context->parent_scope_stack->scope[t].object));
	asAtom res =context->parent_scope_stack->scope[t].object;
	ASATOM_INCREF(res);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_getfuncscopeobject_localresult(call_context* context)
{
	uint32_t t = context->exec_pos->arg2_uint;
	assert(context->parent_scope_stack->scope.size() > t);
	LOG_CALL("getfuncscopeobject_l "<<t<<" "<<asAtomHandler::toDebugString(context->parent_scope_stack->scope[t].object));
	asAtom res =context->parent_scope_stack->scope[t].object;
	ASATOM_INCREF(res);
	replacelocalresult(context,context->exec_pos->local3.pos,res);
	++(context->exec_pos);
}
void ABCVm::abc_lookupswitch_constant(call_context* context)
{
	preloadedcodedata* here=context->exec_pos; //Base for the jumps is the instruction itself for the switch
	asAtom index_obj = *here->arg1_constant;
	int32_t t = (++(context->exec_pos))->arg3_int;
	preloadedcodedata* defaultdest=here+t;
	LOG_CALL("lookupswitch_c " << t);
	uint32_t count = (++(context->exec_pos))->arg3_uint;

	assert_and_throw(asAtomHandler::isNumeric(index_obj));
	unsigned int index=asAtomHandler::toUInt(index_obj);
	ASATOM_DECREF(index_obj);

	preloadedcodedata* dest=defaultdest;
	if(index<=count)
		dest=here+(context->exec_pos+index+1)->arg3_int;

	context->exec_pos = dest;
}
void ABCVm::abc_lookupswitch_local(call_context* context)
{
	preloadedcodedata* here=context->exec_pos; //Base for the jumps is the instruction itself for the switch
	asAtom index_obj = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	int32_t t = (++(context->exec_pos))->arg3_int;
	preloadedcodedata* defaultdest=here+t;
	LOG_CALL("lookupswitch_l " << t<<" "<<asAtomHandler::toDebugString(index_obj));
	uint32_t count = (++(context->exec_pos))->arg3_uint;

	assert_and_throw(asAtomHandler::isNumeric(index_obj));
	unsigned int index=asAtomHandler::toUInt(index_obj);
	ASATOM_DECREF(index_obj);

	preloadedcodedata* dest=defaultdest;
	if(index<=count)
		dest=here+(context->exec_pos+index+1)->arg3_int;

	context->exec_pos = dest;
}

void callFunctionClassRegexp(call_context* context, asAtom& f, asAtom& obj, asAtom& ret)
{
	obj = asAtomHandler::getClosureAtom(f,obj);
	uint32_t argcount = context->exec_pos->local3.flags;
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
	if(asAtomHandler::is<IFunction>(f))
		asAtomHandler::callFunction(f,context->worker,ret,obj,args,argcount,false);
	else if(asAtomHandler::is<Class_base>(f))
	{
		Class_base* c=asAtomHandler::as<Class_base>(f);
		c->generator(context->worker,ret,args,argcount);
	}
	else if(asAtomHandler::is<RegExp>(f))
	{
		RegExp::exec(ret,context->worker,f,args,argcount);
	}
	else
	{
		LOG(LOG_ERROR,"trying to call an object as a function:"<<asAtomHandler::toDebugString(f) <<" on "<<asAtomHandler::toDebugString(obj));
		createError<TypeError>(context->worker,kCallOfNonFunctionError, "Object");
	}
}

void ABCVm::abc_callvoid_constant_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom func = *instrptr->arg1_constant;
	asAtom obj = *instrptr->arg2_constant;
	LOG_CALL("callvoid_cc " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(asAtomHandler::getClosureAtom(func,obj))<<" "<<context->exec_pos->local3.flags);
	asAtom ret;
	callFunctionClassRegexp(context, func, obj,ret);
	++(context->exec_pos);
}
void ABCVm::abc_callvoid_local_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom func = CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	asAtom obj = *instrptr->arg2_constant;
	LOG_CALL("callvoid_lc " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(asAtomHandler::getClosureAtom(func,obj))<<" "<<context->exec_pos->local3.flags);
	asAtom ret;
	callFunctionClassRegexp(context, func, obj,ret);
	++(context->exec_pos);
}

void ABCVm::abc_callvoid_constant_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom func = *instrptr->arg1_constant;
	asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local_pos2);
	LOG_CALL("callvoid_cl " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(asAtomHandler::getClosureAtom(func,obj))<<" "<<context->exec_pos->local3.flags);
	asAtom ret;
	callFunctionClassRegexp(context, func, obj,ret);
	++(context->exec_pos);
}
void ABCVm::abc_callvoid_local_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom func = CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local_pos2);
	LOG_CALL("callvoid_ll " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(asAtomHandler::getClosureAtom(func,obj))<<" "<<context->exec_pos->local3.flags);
	asAtom ret;
	callFunctionClassRegexp(context, func, obj,ret);
	++(context->exec_pos);
}

void ABCVm::abc_call_constant_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom func = *instrptr->arg1_constant;
	asAtom obj = *instrptr->arg2_constant;
	LOG_CALL("call_cc " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(asAtomHandler::getClosureAtom(func,obj))<<" "<<context->exec_pos->local3.flags);
	asAtom ret;
	callFunctionClassRegexp(context, func, obj,ret);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_call_local_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom func = CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	asAtom obj = *instrptr->arg2_constant;
	LOG_CALL("call_lc " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(asAtomHandler::getClosureAtom(func,obj))<<" "<<context->exec_pos->local3.flags);
	asAtom ret;
	callFunctionClassRegexp(context, func, obj,ret);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_call_constant_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom func = *instrptr->arg1_constant;
	asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local_pos2);
	LOG_CALL("call_cl " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(asAtomHandler::getClosureAtom(func,obj))<<" "<<context->exec_pos->local3.flags);
	asAtom ret;
	callFunctionClassRegexp(context, func, obj,ret);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_call_local_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom func = CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local_pos2);
	LOG_CALL("call_ll " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(asAtomHandler::getClosureAtom(func,obj))<<" "<<context->exec_pos->local3.flags);
	asAtom ret;
	callFunctionClassRegexp(context, func, obj,ret);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_call_constant_constant_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom func = *instrptr->arg1_constant;
	asAtom obj = *instrptr->arg2_constant;
	LOG_CALL("call_ccl " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(asAtomHandler::getClosureAtom(func,obj))<<" "<<context->exec_pos->local3.flags);
	asAtom ret;
	callFunctionClassRegexp(context, func, obj,ret);
	replacelocalresult(context,instrptr->local3.pos,ret);
	++(context->exec_pos);
}
void ABCVm::abc_call_local_constant_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom func = CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	asAtom obj = *instrptr->arg2_constant;
	LOG_CALL("call_lcl " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(asAtomHandler::getClosureAtom(func,obj))<<" "<<context->exec_pos->local3.flags);
	asAtom ret;
	callFunctionClassRegexp(context, func, obj,ret);
	replacelocalresult(context,instrptr->local3.pos,ret);
	++(context->exec_pos);
}
void ABCVm::abc_call_constant_local_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom func = *instrptr->arg1_constant;
	asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local_pos2);
	LOG_CALL("call_cll " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(asAtomHandler::getClosureAtom(func,obj))<<" "<<context->exec_pos->local3.flags);
	asAtom ret;
	callFunctionClassRegexp(context, func, obj,ret);
	replacelocalresult(context,instrptr->local3.pos,ret);
	++(context->exec_pos);
}
void ABCVm::abc_call_local_local_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom func = CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local_pos2);
	LOG_CALL("call_lll " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(asAtomHandler::getClosureAtom(func,obj))<<" "<<context->exec_pos->local3.flags);
	asAtom ret;
	callFunctionClassRegexp(context, func, obj,ret);
	replacelocalresult(context,instrptr->local3.pos,ret);
	++(context->exec_pos);
}
void ABCVm::abc_coerce_constant(call_context* context)
{
	LOG_CALL("coerce_c");
	asAtom res = *context->exec_pos->arg1_constant;
	multiname* mn = context->exec_pos->cachedmultiname2;
	const Type* type = mn->cachedType != nullptr ? mn->cachedType : Type::getTypeFromMultiname(mn, context->mi->context);
	if (type == nullptr)
	{
		LOG(LOG_ERROR,"coerce: type not found:"<< *mn);
		createError<TypeError>(context->worker,kClassNotFoundError,mn->qualifiedString(getSys()));
		return;
	}
	if (!type->coerce(context->worker,res))
		ASATOM_INCREF(res);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_coerce_local(call_context* context)
{
	LOG_CALL("coerce_l:"<<asAtomHandler::toDebugString(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1)));
	asAtom res =CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	multiname* mn = context->exec_pos->cachedmultiname2;
	const Type* type = mn->cachedType != nullptr ? mn->cachedType : Type::getTypeFromMultiname(mn, context->mi->context);
	if (type == nullptr)
	{
		LOG(LOG_ERROR,"coerce: type not found:"<< *mn);
		createError<TypeError>(context->worker,kClassNotFoundError,mn->qualifiedString(getSys()));
		return;
	}
	if (!type->coerce(context->worker,res))
		ASATOM_INCREF(res);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_coerce_constant_localresult(call_context* context)
{
	LOG_CALL("coerce_cl");
	asAtom res = *context->exec_pos->arg1_constant;
	multiname* mn = context->exec_pos->cachedmultiname2;
	const Type* type = mn->cachedType != nullptr ? mn->cachedType : Type::getTypeFromMultiname(mn, context->mi->context);
	if (type == nullptr)
	{
		LOG(LOG_ERROR,"coerce: type not found:"<< *mn);
		createError<TypeError>(context->worker,kClassNotFoundError,mn->qualifiedString(getSys()));
		return;
	}
	if (!type->coerce(context->worker,res))
		ASATOM_INCREF(res);
	replacelocalresult(context,context->exec_pos->local3.pos,res);
	++(context->exec_pos);
}
void ABCVm::abc_coerce_local_localresult(call_context* context)
{
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	multiname* mn = context->exec_pos->cachedmultiname2;
	LOG_CALL("coerce_ll:"<<*mn<<" "<<asAtomHandler::toDebugString(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1)));
	const Type* type = mn->cachedType != nullptr ? mn->cachedType : Type::getTypeFromMultiname(mn, context->mi->context);
	if (type == nullptr)
	{
		LOG(LOG_ERROR,"coerce: type not found:"<< *mn);
		createError<TypeError>(context->worker,kClassNotFoundError,mn->qualifiedString(getSys()));
		return;
	}
	
	if (!type->coerce(context->worker,res))
		ASATOM_INCREF(res);
	replacelocalresult(context,context->exec_pos->local3.pos,res);
	++(context->exec_pos);
}

void ABCVm::abc_sxi1_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "sxi1_c");
	int32_t res=asAtomHandler::toUInt(*instrptr->arg1_constant)&0x1 ? -1 : 0;
	ASATOM_DECREF(*instrptr->arg1_constant);
	asAtom ret=asAtomHandler::invalidAtom;
	asAtomHandler::setInt(ret,context->worker,res);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_sxi1_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "sxi1_l");
	int32_t res=asAtomHandler::toUInt(CONTEXT_GETLOCAL(context,instrptr->local_pos1))&0x1 ? -1 : 0;
	ASATOM_DECREF(CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	asAtom ret=asAtomHandler::invalidAtom;
	asAtomHandler::setInt(ret,context->worker,res);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_sxi1_constant_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "sxi1_cl");
	int32_t res=asAtomHandler::toUInt(*instrptr->arg1_constant)&0x1 ? -1 : 0;
	ASATOM_DECREF(*instrptr->arg1_constant);
	asAtom ret=asAtomHandler::invalidAtom;
	asAtomHandler::setInt(ret,context->worker,res);
	replacelocalresult(context,instrptr->local3.pos,ret);
	++(context->exec_pos);
}
void ABCVm::abc_sxi1_local_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "sxi1_ll");
	int32_t res=asAtomHandler::toUInt(CONTEXT_GETLOCAL(context,instrptr->local_pos1))&0x1 ? -1 : 0;
	ASATOM_DECREF(CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	asAtom ret=asAtomHandler::invalidAtom;
	asAtomHandler::setInt(ret,context->worker,res);
	replacelocalresult(context,instrptr->local3.pos,ret);
	++(context->exec_pos);
}

void ABCVm::abc_sxi8_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "sxi8_c");
	int32_t res=(int8_t)asAtomHandler::toUInt(*instrptr->arg1_constant);
	ASATOM_DECREF(*instrptr->arg1_constant);
	asAtom ret=asAtomHandler::invalidAtom;
	asAtomHandler::setInt(ret,context->worker,res);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_sxi8_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "sxi8_l");
	int32_t res=(int8_t)asAtomHandler::toUInt(CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	ASATOM_DECREF(CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	asAtom ret=asAtomHandler::invalidAtom;
	asAtomHandler::setInt(ret,context->worker,res);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_sxi8_constant_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "sxi8_cl");
	int32_t res=asAtomHandler::toUInt(*instrptr->arg1_constant)&0x1 ? -1 : 0;
	ASATOM_DECREF(*instrptr->arg1_constant);
	asAtom ret=asAtomHandler::invalidAtom;
	asAtomHandler::setInt(ret,context->worker,res);
	replacelocalresult(context,instrptr->local3.pos,ret);
	++(context->exec_pos);
}
void ABCVm::abc_sxi8_local_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "sxi8_ll");
	int32_t res=(int8_t)asAtomHandler::toUInt(CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	ASATOM_DECREF(CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	asAtom ret=asAtomHandler::invalidAtom;
	asAtomHandler::setInt(ret,context->worker,res);
	replacelocalresult(context,instrptr->local3.pos,ret);
	++(context->exec_pos);
}

void ABCVm::abc_sxi16_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "sxi16_c");
	int32_t res=(int8_t)asAtomHandler::toUInt(*instrptr->arg1_constant);
	ASATOM_DECREF(*instrptr->arg1_constant);
	asAtom ret=asAtomHandler::invalidAtom;
	asAtomHandler::setInt(ret,context->worker,res);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_sxi16_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "sxi16_l");
	int32_t res=(int16_t)asAtomHandler::toUInt(CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	ASATOM_DECREF(CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	asAtom ret=asAtomHandler::invalidAtom;
	asAtomHandler::setInt(ret,context->worker,res);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_sxi16_constant_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "sxi16_cl");
	int32_t res=(int16_t)asAtomHandler::toUInt(*instrptr->arg1_constant);
	ASATOM_DECREF(*instrptr->arg1_constant);
	asAtom ret=asAtomHandler::invalidAtom;
	asAtomHandler::setInt(ret,context->worker,res);
	replacelocalresult(context,instrptr->local3.pos,ret);
	++(context->exec_pos);
}
void ABCVm::abc_sxi16_local_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "sxi16_ll");
	int32_t res=(int16_t)asAtomHandler::toUInt(CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	ASATOM_DECREF(CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	asAtom ret=asAtomHandler::invalidAtom;
	asAtomHandler::setInt(ret,context->worker,res);
	replacelocalresult(context,instrptr->local3.pos,ret);
	++(context->exec_pos);
}
