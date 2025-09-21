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
#include "scripting/flash/errors/flasherrors.h"
#include "scripting/flash/system/ApplicationDomain.h"
#include "scripting/toplevel/toplevel.h"
#include "scripting/toplevel/ASString.h"
#include "scripting/toplevel/RegExp.h"
#include "parsing/streams.h"
#include <string>
#include <sstream>

using namespace std;
using namespace lightspark;


FORCE_INLINE void replacelocalresult(call_context* context,uint16_t pos,asAtom& ret)
{
	if (USUALLY_FALSE(context->exceptionthrown))
	{
		ASATOM_DECREF(ret);
		return;
	}
	asAtom oldres = CONTEXT_GETLOCAL(context,pos);
	if (asAtomHandler::isLocalNumber(ret))
		asAtomHandler::setNumber(CONTEXT_GETLOCAL(context,pos),context->worker,asAtomHandler::getLocalNumber(context,ret),pos);
	else
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
	LOG_CALL("ifNLT_ll (" << ((cond)?"taken)":"not taken)") << " "<< context->exec_pos->local_pos1<<"/"<<asAtomHandler::toDebugString(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1))<< " "<< context->exec_pos->local_pos2<<"/"<<asAtomHandler::toDebugString(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2)));
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
	int32_t num = context->exec_pos->arg2_int;
	asAtom o = num ==-1 ? *context->locals : (context->function->func_scope->scope.rbegin()+num)->object;
	ASObject* s = asAtomHandler::toObject(o,context->worker);
	asAtom a = s->getSlot(t);
	LOG_CALL("getlexfromslot "<<s->toDebugString()<<" "<<t);
	ASATOM_INCREF(a);
	RUNTIME_STACK_PUSH(context,a);
	++(context->exec_pos);
}

void ABCVm::abc_getlexfromslot_localresult(call_context* context)
{
	uint32_t t = context->exec_pos->arg1_uint;
	int32_t num = context->exec_pos->arg2_int;
	asAtom o = num ==-1 ? *context->locals : (context->function->func_scope->scope.rbegin()+num)->object;
	ASObject* s = asAtomHandler::toObject(o,context->worker);
	asAtom a = s->getSlot(t,context->exec_pos->local3.pos);
	LOG_CALL("getlexfromslot_l "<<s->toDebugString()<<" "<<t<<" "<<asAtomHandler::toDebugString(a));
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
	ApplicationDomain::loadIntN<uint8_t>(context->mi->context->applicationDomain,ret,*instrptr->arg1_constant);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_li8_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "li8_l");
	asAtom ret=asAtomHandler::invalidAtom;
	ApplicationDomain::loadIntN<uint8_t>(context->mi->context->applicationDomain,ret,CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_li8_constant_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "li8_cl");
	asAtom ret=asAtomHandler::invalidAtom;
	ApplicationDomain::loadIntN<uint8_t>(context->mi->context->applicationDomain,ret,*instrptr->arg1_constant);
	replacelocalresult(context,instrptr->local3.pos,ret);
	++(context->exec_pos);
}
void ABCVm::abc_li8_local_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "li8_ll");
	asAtom oldres = CONTEXT_GETLOCAL(context,instrptr->local3.pos);
	uint32_t addr=asAtomHandler::getUInt(CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	ByteArray* dm = context->mi->context->applicationDomain->currentDomainMemory;
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
	ApplicationDomain::loadIntN<uint8_t>(context->mi->context->applicationDomain,ret,*context->exec_pos->arg1_constant);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	LOG_CALL("li8_cs " << t << " "<< asAtomHandler::toDebugString(ret) << " "<< asAtomHandler::toDebugString(obj));
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,ret,context->worker);
	++(context->exec_pos);
}
void ABCVm::abc_li8_local_setslotnocoerce(call_context* context)
{
	asAtom ret=asAtomHandler::invalidAtom;
	ApplicationDomain::loadIntN<uint8_t>(context->mi->context->applicationDomain,ret,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1));

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	LOG_CALL("li8_ls " << t << " "<< asAtomHandler::toDebugString(ret) << " "<< asAtomHandler::toDebugString(obj));
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,ret,context->worker);
	++(context->exec_pos);
}

void ABCVm::abc_li16_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "li16_c");
	asAtom ret=asAtomHandler::invalidAtom;
	ApplicationDomain::loadIntN<uint16_t>(context->mi->context->applicationDomain,ret,*instrptr->arg1_constant);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_li16_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "li16_l");
	asAtom ret=asAtomHandler::invalidAtom;
	ApplicationDomain::loadIntN<uint16_t>(context->mi->context->applicationDomain,ret,CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_li16_constant_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "li16_cl");
	asAtom ret=asAtomHandler::invalidAtom;
	ApplicationDomain::loadIntN<uint16_t>(context->mi->context->applicationDomain,ret,*instrptr->arg1_constant);
	replacelocalresult(context,instrptr->local3.pos,ret);
	++(context->exec_pos);
}
void ABCVm::abc_li16_local_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "li16_ll");
	asAtom ret=asAtomHandler::invalidAtom;
	ApplicationDomain::loadIntN<uint16_t>(context->mi->context->applicationDomain,ret,CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	replacelocalresult(context,instrptr->local3.pos,ret);
	++(context->exec_pos);
}
void ABCVm::abc_li16_constant_setslotnocoerce(call_context* context)
{
	asAtom ret=asAtomHandler::invalidAtom;
	ApplicationDomain::loadIntN<uint16_t>(context->mi->context->applicationDomain,ret,*context->exec_pos->arg1_constant);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	LOG_CALL("li16_cs " << t << " "<< asAtomHandler::toDebugString(ret) << " "<< asAtomHandler::toDebugString(obj));
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,ret,context->worker);
	++(context->exec_pos);
}
void ABCVm::abc_li16_local_setslotnocoerce(call_context* context)
{
	asAtom ret=asAtomHandler::invalidAtom;
	ApplicationDomain::loadIntN<uint16_t>(context->mi->context->applicationDomain,ret,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1));

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	LOG_CALL("li16_ls " << t << " "<< asAtomHandler::toDebugString(ret) << " "<< asAtomHandler::toDebugString(obj));
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,ret,context->worker);
	++(context->exec_pos);
}
void ABCVm::abc_li32_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "li32_c");
	asAtom ret=asAtomHandler::invalidAtom;
	ApplicationDomain::loadIntN<int32_t>(context->mi->context->applicationDomain,ret,*instrptr->arg1_constant);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_li32_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "li32_l");
	asAtom ret=asAtomHandler::invalidAtom;
	ApplicationDomain::loadIntN<int32_t>(context->mi->context->applicationDomain,ret,CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_li32_constant_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "li32_cl");
	asAtom ret=asAtomHandler::invalidAtom;
	ApplicationDomain::loadIntN<int32_t>(context->mi->context->applicationDomain,ret,*instrptr->arg1_constant);
	replacelocalresult(context,instrptr->local3.pos,ret);
	++(context->exec_pos);
}
void ABCVm::abc_li32_local_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "li32_ll");
	asAtom ret=asAtomHandler::invalidAtom;
	ApplicationDomain::loadIntN<int32_t>(context->mi->context->applicationDomain,ret,CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	replacelocalresult(context,instrptr->local3.pos,ret);
	++(context->exec_pos);
}
void ABCVm::abc_li32_constant_setslotnocoerce(call_context* context)
{
	asAtom ret=asAtomHandler::invalidAtom;
	ApplicationDomain::loadIntN<int32_t>(context->mi->context->applicationDomain,ret,*context->exec_pos->arg1_constant);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	LOG_CALL("li32_cs " << t << " "<< asAtomHandler::toDebugString(ret) << " "<< asAtomHandler::toDebugString(obj));
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,ret,context->worker);
	++(context->exec_pos);
}
void ABCVm::abc_li32_local_setslotnocoerce(call_context* context)
{
	asAtom ret=asAtomHandler::invalidAtom;
	ApplicationDomain::loadIntN<int32_t>(context->mi->context->applicationDomain,ret,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1));

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	LOG_CALL("li32_ls " << t << " "<< asAtomHandler::toDebugString(ret) << " "<< asAtomHandler::toDebugString(obj));
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,ret,context->worker);
	++(context->exec_pos);
}

void ABCVm::abc_lf32_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "lf32_c");
	asAtom ret=asAtomHandler::invalidAtom;
	ApplicationDomain::loadFloat(context->mi->context->applicationDomain,ret,*instrptr->arg1_constant);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_lf32_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "lf32_l");
	asAtom ret=asAtomHandler::invalidAtom;
	ApplicationDomain::loadFloat(context->mi->context->applicationDomain,ret,CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_lf32_constant_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "lf32_cl");
	ApplicationDomain::loadFloat(context->mi->context->applicationDomain,CONTEXT_GETLOCAL(context,instrptr->local3.pos),*instrptr->arg1_constant,instrptr->local3.pos);
	++(context->exec_pos);
}
void ABCVm::abc_lf32_local_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "lf32_ll");
	ApplicationDomain::loadFloat(context->mi->context->applicationDomain,CONTEXT_GETLOCAL(context,instrptr->local3.pos),CONTEXT_GETLOCAL(context,instrptr->local_pos1),instrptr->local3.pos);
	++(context->exec_pos);
}
void ABCVm::abc_lf32_constant_setslotnocoerce(call_context* context)
{
	asAtom ret=asAtomHandler::invalidAtom;
	ApplicationDomain::loadFloat(context->mi->context->applicationDomain,ret,*context->exec_pos->arg1_constant);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	LOG_CALL("lf32_cs " << t << " "<< asAtomHandler::toDebugString(ret) << " "<< asAtomHandler::toDebugString(obj));
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,ret,context->worker);
	++(context->exec_pos);
}
void ABCVm::abc_lf32_local_setslotnocoerce(call_context* context)
{
	asAtom ret=asAtomHandler::invalidAtom;
	ApplicationDomain::loadFloat(context->mi->context->applicationDomain,ret,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1));

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	LOG_CALL("lf32_ls " << t << " "<< asAtomHandler::toDebugString(ret) << " "<< asAtomHandler::toDebugString(obj));
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,ret,context->worker);
	++(context->exec_pos);
}
void ABCVm::abc_lf64_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "lf64_c");
	asAtom ret=asAtomHandler::invalidAtom;
	ApplicationDomain::loadDouble(context->mi->context->applicationDomain,ret,*instrptr->arg1_constant);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_lf64_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "lf64_l");
	asAtom ret=asAtomHandler::invalidAtom;
	ApplicationDomain::loadDouble(context->mi->context->applicationDomain,ret,CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_lf64_constant_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "lf64_cl");
	asAtom ret=asAtomHandler::invalidAtom;
	ApplicationDomain::loadDouble(context->mi->context->applicationDomain,ret,*instrptr->arg1_constant,instrptr->local3.pos);
	replacelocalresult(context,instrptr->local3.pos,ret);
	++(context->exec_pos);
}
void ABCVm::abc_lf64_local_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "lf64_ll");
	asAtom ret=asAtomHandler::invalidAtom;
	ApplicationDomain::loadDouble(context->mi->context->applicationDomain,ret,CONTEXT_GETLOCAL(context,instrptr->local_pos1),instrptr->local3.pos);
	replacelocalresult(context,instrptr->local3.pos,ret);
	++(context->exec_pos);
}
void ABCVm::abc_lf64_constant_setslotnocoerce(call_context* context)
{
	asAtom ret=asAtomHandler::invalidAtom;
	ApplicationDomain::loadDouble(context->mi->context->applicationDomain,ret,*context->exec_pos->arg1_constant);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	LOG_CALL("lf64_cs " << t << " "<< asAtomHandler::toDebugString(ret) << " "<< asAtomHandler::toDebugString(obj));
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,ret,context->worker);
	++(context->exec_pos);
}
void ABCVm::abc_lf64_local_setslotnocoerce(call_context* context)
{
	asAtom ret=asAtomHandler::invalidAtom;
	ApplicationDomain::loadDouble(context->mi->context->applicationDomain,ret,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1));

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	LOG_CALL("lf64_ls " << t << " "<< asAtomHandler::toDebugString(ret) << " "<< asAtomHandler::toDebugString(obj));
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,ret,context->worker);
	++(context->exec_pos);
}
void ABCVm::abc_si8_constant_constant(call_context* context)
{
	LOG_CALL( "si8_cc");
	preloadedcodedata* instrptr = context->exec_pos;
	ApplicationDomain::storeIntN<uint8_t>(context->mi->context->applicationDomain,*instrptr->arg2_constant,*instrptr->arg1_constant);
	++(context->exec_pos);
}
void ABCVm::abc_si8_local_constant(call_context* context)
{
	LOG_CALL( "si8_lc");
	preloadedcodedata* instrptr = context->exec_pos;
	ApplicationDomain::storeIntN<uint8_t>(context->mi->context->applicationDomain,*instrptr->arg2_constant,CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	++(context->exec_pos);
}
void ABCVm::abc_si8_constant_local(call_context* context)
{
	LOG_CALL( "si8_cl");
	preloadedcodedata* instrptr = context->exec_pos;
	ApplicationDomain::storeIntN<uint8_t>(context->mi->context->applicationDomain,CONTEXT_GETLOCAL(context,instrptr->local_pos2),*instrptr->arg1_constant);
	++(context->exec_pos);
}
void ABCVm::abc_si8_local_local(call_context* context)
{
	LOG_CALL( "si8_ll");
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t addr=asAtomHandler::getUInt(CONTEXT_GETLOCAL(context,instrptr->local_pos2));
	int32_t val=asAtomHandler::getInt(CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	ByteArray* dm = context->mi->context->applicationDomain->currentDomainMemory;
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
	ApplicationDomain::storeIntN<uint16_t>(context->mi->context->applicationDomain,*instrptr->arg2_constant,*instrptr->arg1_constant);
	++(context->exec_pos);
}
void ABCVm::abc_si16_local_constant(call_context* context)
{
	LOG_CALL( "si16_lc");
	preloadedcodedata* instrptr = context->exec_pos;
	ApplicationDomain::storeIntN<uint16_t>(context->mi->context->applicationDomain,*instrptr->arg2_constant,CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	++(context->exec_pos);
}
void ABCVm::abc_si16_constant_local(call_context* context)
{
	LOG_CALL( "si16_cl");
	preloadedcodedata* instrptr = context->exec_pos;
	ApplicationDomain::storeIntN<uint16_t>(context->mi->context->applicationDomain,CONTEXT_GETLOCAL(context,instrptr->local_pos2),*instrptr->arg1_constant);
	++(context->exec_pos);
}
void ABCVm::abc_si16_local_local(call_context* context)
{
	LOG_CALL( "si16_ll");
	preloadedcodedata* instrptr = context->exec_pos;
	ApplicationDomain::storeIntN<uint16_t>(context->mi->context->applicationDomain,CONTEXT_GETLOCAL(context,instrptr->local_pos2),CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	++(context->exec_pos);
}
void ABCVm::abc_si32_constant_constant(call_context* context)
{
	LOG_CALL( "si32_cc");
	preloadedcodedata* instrptr = context->exec_pos;
	ApplicationDomain::storeIntN<uint32_t>(context->mi->context->applicationDomain,*instrptr->arg2_constant,*instrptr->arg1_constant);
	++(context->exec_pos);
}
void ABCVm::abc_si32_local_constant(call_context* context)
{
	LOG_CALL( "si32_lc");
	preloadedcodedata* instrptr = context->exec_pos;
	ApplicationDomain::storeIntN<uint32_t>(context->mi->context->applicationDomain,*instrptr->arg2_constant,CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	++(context->exec_pos);
}
void ABCVm::abc_si32_constant_local(call_context* context)
{
	LOG_CALL( "si32_cl");
	preloadedcodedata* instrptr = context->exec_pos;
	ApplicationDomain::storeIntN<uint32_t>(context->mi->context->applicationDomain,CONTEXT_GETLOCAL(context,instrptr->local_pos2),*instrptr->arg1_constant);
	++(context->exec_pos);
}
void ABCVm::abc_si32_local_local(call_context* context)
{
	LOG_CALL( "si32_ll");
	preloadedcodedata* instrptr = context->exec_pos;
	ApplicationDomain::storeIntN<uint32_t>(context->mi->context->applicationDomain,CONTEXT_GETLOCAL(context,instrptr->local_pos2),CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	++(context->exec_pos);
}
void ABCVm::abc_sf32_constant_constant(call_context* context)
{
	LOG_CALL( "sf32_cc");
	preloadedcodedata* instrptr = context->exec_pos;
	ApplicationDomain::storeFloat(context->mi->context->applicationDomain,*instrptr->arg2_constant,*instrptr->arg1_constant);
	++(context->exec_pos);
}
void ABCVm::abc_sf32_local_constant(call_context* context)
{
	LOG_CALL( "sf32_lc");
	preloadedcodedata* instrptr = context->exec_pos;
	ApplicationDomain::storeFloat(context->mi->context->applicationDomain,*instrptr->arg2_constant,CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	++(context->exec_pos);
}
void ABCVm::abc_sf32_constant_local(call_context* context)
{
	LOG_CALL( "sf32_cl");
	preloadedcodedata* instrptr = context->exec_pos;
	ApplicationDomain::storeFloat(context->mi->context->applicationDomain,CONTEXT_GETLOCAL(context,instrptr->local_pos2),*instrptr->arg1_constant);
	++(context->exec_pos);
}
void ABCVm::abc_sf32_local_local(call_context* context)
{
	LOG_CALL( "sf32_ll");
	preloadedcodedata* instrptr = context->exec_pos;
	ApplicationDomain::storeFloat(context->mi->context->applicationDomain,CONTEXT_GETLOCAL(context,instrptr->local_pos2),CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	++(context->exec_pos);
}
void ABCVm::abc_sf64_constant_constant(call_context* context)
{
	LOG_CALL( "sf64_cc");
	preloadedcodedata* instrptr = context->exec_pos;
	ApplicationDomain::storeDouble(context->mi->context->applicationDomain,*instrptr->arg2_constant,*instrptr->arg1_constant);
	++(context->exec_pos);
}
void ABCVm::abc_sf64_local_constant(call_context* context)
{
	LOG_CALL( "sf64_lc");
	preloadedcodedata* instrptr = context->exec_pos;
	ApplicationDomain::storeDouble(context->mi->context->applicationDomain,*instrptr->arg2_constant,CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	++(context->exec_pos);
}
void ABCVm::abc_sf64_constant_local(call_context* context)
{
	LOG_CALL( "sf64_cl");
	preloadedcodedata* instrptr = context->exec_pos;
	ApplicationDomain::storeDouble(context->mi->context->applicationDomain,CONTEXT_GETLOCAL(context,instrptr->local_pos2),*instrptr->arg1_constant);
	++(context->exec_pos);
}
void ABCVm::abc_sf64_local_local(call_context* context)
{
	LOG_CALL( "sf64_ll");
	preloadedcodedata* instrptr = context->exec_pos;
	ApplicationDomain::storeDouble(context->mi->context->applicationDomain,CONTEXT_GETLOCAL(context,instrptr->local_pos2),CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	++(context->exec_pos);
}
void ABCVm::construct_noargs_intern(call_context* context,asAtom& ret,asAtom& obj)
{
	context->explicitConstruction = true;
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
			context->explicitConstruction = false;
			createError<TypeError>(context->worker,kConstructOfNonFunctionError);
		}
	}
	if (asAtomHandler::isObject(ret))
		asAtomHandler::getObject(ret)->setConstructorCallComplete();
	LOG_CALL("End of construct_noargs " << asAtomHandler::toDebugString(ret));
	context->explicitConstruction = false;
}
void ABCVm::abc_construct_constant(call_context* context)
{
	asAtom obj= *context->exec_pos->arg1_constant;
	LOG_CALL( "construct_noargs_c "<<asAtomHandler::toDebugString(obj));
	asAtom ret=asAtomHandler::invalidAtom;
	construct_noargs_intern(context,ret,obj);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_construct_local(call_context* context)
{
	asAtom obj= CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	LOG_CALL( "construct_noargs_l "<<asAtomHandler::toDebugString(obj));
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
void ABCVm::abc_returnvalue_constant(call_context* context)
{
	asAtom v = *context->exec_pos->arg1_constant;
	if (asAtomHandler::isLocalNumber(v))
		asAtomHandler::setNumber(v,context->worker,asAtomHandler::getLocalNumber(context,v),context->mi->body->getReturnValuePos());
	else
		asAtomHandler::set(context->locals[context->mi->body->getReturnValuePos()],v);
	LOG_CALL("returnValue_c " << asAtomHandler::toDebugString(context->locals[context->mi->body->getReturnValuePos()]));
	++(context->exec_pos);
}
void ABCVm::abc_returnvalue_local(call_context* context)
{
	asAtom v = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	if (asAtomHandler::isLocalNumber(v))
		asAtomHandler::setNumber(context->locals[context->mi->body->getReturnValuePos()],context->worker,asAtomHandler::getLocalNumber(context,v),context->mi->body->getReturnValuePos());
	else
		asAtomHandler::set(context->locals[context->mi->body->getReturnValuePos()],v);
	LOG_CALL("returnValue_l " << asAtomHandler::toDebugString(context->locals[context->mi->body->getReturnValuePos()]));
	ASATOM_INCREF(context->locals[context->mi->body->getReturnValuePos()]);
	++(context->exec_pos);
}
void ABCVm::abc_constructsuper_constant(call_context* context)
{
	LOG_CALL( "constructSuper_c ");
	asAtom obj=*context->exec_pos->arg1_constant;
	context->function->inClass->super->handleConstruction(obj,nullptr, 0, false, context->explicitConstruction);
	LOG_CALL("End super construct "<<asAtomHandler::toDebugString(obj));
	++(context->exec_pos);
}
void ABCVm::abc_constructsuper_local(call_context* context)
{
	LOG_CALL( "constructSuper_l ");
	asAtom obj= CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	context->function->inClass->super->handleConstruction(obj,nullptr, 0, false, context->explicitConstruction);
	LOG_CALL("End super construct "<<asAtomHandler::toDebugString(obj));
	++(context->exec_pos);
}
void ABCVm::constructpropnoargs_intern(call_context* context,asAtom& ret,asAtom& obj,multiname* name, ASObject* constructor)
{
	context->explicitConstruction = true;
	asAtom o=asAtomHandler::invalidAtom;
	if (constructor)
		o = asAtomHandler::fromObjectNoPrimitive(constructor);
	else
		asAtomHandler::toObject(obj,context->worker)->getVariableByMultiname(o,*name,GET_VARIABLE_OPTION::NONE, context->worker);

	if(asAtomHandler::isInvalid(o))
	{
		context->explicitConstruction = false;
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
			context->explicitConstruction = false;
			createError<TypeError>(context->worker,kConstructOfNonFunctionError);
			ASATOM_DECREF(o);
			return;
		}
		else
		{
			context->explicitConstruction = false;
			createError<TypeError>(context->worker,kNotConstructorError);
			ASATOM_DECREF(o);
			return;
		}
	}
	catch(ASObject* exc)
	{
		context->explicitConstruction = false;
		LOG_CALL("Exception during object construction. Returning Undefined");
		//Handle eventual exceptions from the constructor, to fix the stack
		RUNTIME_STACK_PUSH(context,obj);
		throw;
	}
	context->explicitConstruction = false;
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
void ABCVm::abc_getlex_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	assert(instrptr->local3.pos > 0);
	if ((instrptr->local3.flags&ABC_OP_CACHED) == ABC_OP_CACHED)
	{
		asAtom oldres = CONTEXT_GETLOCAL(context,instrptr->local3.pos);
		asAtomHandler::setFunction(CONTEXT_GETLOCAL(context,instrptr->local3.pos),instrptr->cacheobj1,asAtomHandler::invalidAtom,context->worker);
		ASATOM_INCREF(CONTEXT_GETLOCAL(context,instrptr->local3.pos));
		ASATOM_DECREF(oldres);
		LOG_CALL( "getLex_l from cache: " <<  instrptr->cacheobj1->toDebugString());
	}
	else if (getLex_multiname(context,instrptr->cachedmultiname2,instrptr->local3.pos))
	{
		// put object in cache
		instrptr->local3.flags = ABC_OP_CACHED;
		instrptr->cacheobj1 = asAtomHandler::getObject(CONTEXT_GETLOCAL(context,instrptr->local3.pos));
		instrptr->cacheobj2 = asAtomHandler::getObject(asAtomHandler::getClosureAtom(CONTEXT_GETLOCAL(context,instrptr->local3.pos),asAtomHandler::invalidAtom));
	}
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
		if (asAtomHandler::isLocalNumber(*obj))
		{
			ASATOM_DECREF(context->locals[i]);
			asAtomHandler::setNumber(context->locals[i],context->worker,asAtomHandler::getLocalNumber(context,*obj),i);
		}
		else
		{
			ASATOM_DECREF(context->locals[i]);
			context->locals[i]=*instrptr->arg1_constant;
		}
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
		if (asAtomHandler::isLocalNumber(obj))
		{
			ASATOM_DECREF(context->locals[i]);
			asAtomHandler::setNumber(context->locals[i],context->worker,asAtomHandler::getLocalNumber(context,obj),i);
		}
		else
		{
			ASATOM_INCREF(obj);
			ASATOM_DECREF(context->locals[i]);
			context->locals[i]=obj;
		}
	}
}
void ABCVm::abc_callFunctionNoArgs_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = *instrptr->arg1_constant;
	asAtom func = asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj2);
	asAtom ret = asAtomHandler::invalidAtom;
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
	asAtom ret = asAtomHandler::invalidAtom;
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
	asAtomHandler::callFunction(func,context->worker,CONTEXT_GETLOCAL(context,instrptr->local3.pos),obj,nullptr,0,false,false,true,false,instrptr->local3.pos);
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
	asAtomHandler::getObjectNoCheck(objslot)->setSlotNoCoerce(t,res,context->worker);
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
	asAtomHandler::getObjectNoCheck(objslot)->setSlotNoCoerce(t,res,context->worker);
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
void ABCVm::abc_convert_i_constant(call_context* context)
{
	LOG_CALL("convert_i_c");
	asAtom res = *context->exec_pos->arg1_constant;
	if(!asAtomHandler::isInteger(res))
	{
		int32_t v= asAtomHandler::toIntStrict(context->worker,res);
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
		int32_t v= asAtomHandler::toIntStrict(context->worker,res);
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
		int32_t v= asAtomHandler::toIntStrict(context->worker,res);
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
		int32_t v= asAtomHandler::toIntStrict(context->worker,res);
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
		int32_t v= asAtomHandler::toIntStrict(context->worker,res);
		asAtomHandler::setInt(res,context->worker,v);
	}
	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res,context->worker);
	++(context->exec_pos);
}
void ABCVm::abc_convert_i_local_setslotnocoerce(call_context* context)
{
	LOG_CALL("convert_i_ls");
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	if(!asAtomHandler::isInteger(res)) {
		int32_t v= asAtomHandler::toIntStrict(context->worker,res);
		asAtomHandler::setInt(res,context->worker,v);
	}
	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res,context->worker);
	++(context->exec_pos);
}

void ABCVm::abc_convert_u_constant(call_context* context)
{
	LOG_CALL("convert_u_c");
	asAtom res = *context->exec_pos->arg1_constant;
	if(!asAtomHandler::isUInteger(res))
	{
		int32_t v= asAtomHandler::getUInt(context->worker,res);
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
		int32_t v= asAtomHandler::getUInt(context->worker,res);
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
		int32_t v= asAtomHandler::getUInt(context->worker,res);
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
		int32_t v= asAtomHandler::getUInt(context->worker,res);
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
		int32_t v= asAtomHandler::getUInt(context->worker,res);
		asAtomHandler::setUInt(res,context->worker,v);
	}
	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res,context->worker);
	++(context->exec_pos);
}
void ABCVm::abc_convert_u_local_setslotnocoerce(call_context* context)
{
	LOG_CALL("convert_u_ls");
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	if(!asAtomHandler::isUInteger(res)) {
		int32_t v= asAtomHandler::getUInt(context->worker,res);
		asAtomHandler::setUInt(res,context->worker,v);
	}
	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res,context->worker);
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
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res,context->worker);
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
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res,context->worker);
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
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res,context->worker);
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
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res,context->worker);
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
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res,context->worker);
	++(context->exec_pos);
}
void ABCVm::abc_astypelate_local_constant_setslotnocoerce(call_context* context)
{
	LOG_CALL("astypelate_lcs");
	asAtom res = asAtomHandler::asTypelate(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),*context->exec_pos->arg2_constant,context->worker);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res,context->worker);
	++(context->exec_pos);
}
void ABCVm::abc_astypelate_constant_local_setslotnocoerce(call_context* context)
{
	LOG_CALL("astypelate_cls");
	asAtom res = asAtomHandler::asTypelate(*context->exec_pos->arg1_constant,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->worker);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res,context->worker);
	++(context->exec_pos);
}
void ABCVm::abc_astypelate_local_local_setslotnocoerce(call_context* context)
{
	LOG_CALL("astypelate_lls");
	asAtom res = asAtomHandler::asTypelate(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->worker);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res,context->worker);
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

	asAtomHandler::subtractreplace(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),context->worker,*context->exec_pos->arg1_constant,*context->exec_pos->arg2_constant,context->exec_pos->local3.flags & ABC_OP_FORCEINT,context->exec_pos->local3.pos);
	++(context->exec_pos);
}
void ABCVm::abc_subtract_local_constant_localresult(call_context* context)
{
	LOG_CALL("subtract_lcl");
	asAtomHandler::subtractreplace(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),*context->exec_pos->arg2_constant,context->exec_pos->local3.flags & ABC_OP_FORCEINT,context->exec_pos->local3.pos);
	++(context->exec_pos);
}
void ABCVm::abc_subtract_constant_local_localresult(call_context* context)
{
	LOG_CALL("subtract_cll");
	asAtomHandler::subtractreplace(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),context->worker,*context->exec_pos->arg1_constant,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->exec_pos->local3.flags & ABC_OP_FORCEINT,context->exec_pos->local3.pos);
	++(context->exec_pos);
}
void ABCVm::abc_subtract_local_local_localresult(call_context* context)
{
	LOG_CALL("subtract_lll "<<context->exec_pos->local_pos1<<"/"<<context->exec_pos->local_pos2<<"/"<<context->exec_pos->local3.pos);
	asAtomHandler::subtractreplace(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->exec_pos->local3.flags & ABC_OP_FORCEINT,context->exec_pos->local3.pos);
	++(context->exec_pos);
}
void ABCVm::abc_subtract_constant_constant_setslotnocoerce(call_context* context)
{
	LOG_CALL("subtract_ccs");
	asAtom res = asAtomHandler::invalidAtom;
	asAtomHandler::subtractreplace(res,context->worker,*context->exec_pos->arg1_constant,*context->exec_pos->arg2_constant,context->exec_pos->local3.flags & ABC_OP_FORCEINT,context->exec_pos->local3.pos);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res,context->worker);
	++(context->exec_pos);
}
void ABCVm::abc_subtract_local_constant_setslotnocoerce(call_context* context)
{
	LOG_CALL("subtract_lcs");
	asAtom res = asAtomHandler::invalidAtom;
	asAtomHandler::subtractreplace(res,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),*context->exec_pos->arg2_constant,context->exec_pos->local3.flags & ABC_OP_FORCEINT,context->exec_pos->local3.pos);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res,context->worker);
	++(context->exec_pos);
}
void ABCVm::abc_subtract_constant_local_setslotnocoerce(call_context* context)
{
	LOG_CALL("subtract_cls");
	asAtom res = asAtomHandler::invalidAtom;
	asAtomHandler::subtractreplace(res,context->worker,*context->exec_pos->arg1_constant,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->exec_pos->local3.flags & ABC_OP_FORCEINT,context->exec_pos->local3.pos);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res,context->worker);
	++(context->exec_pos);
}
void ABCVm::abc_subtract_local_local_setslotnocoerce(call_context* context)
{
	LOG_CALL("subtract_lls");
	asAtom res = asAtomHandler::invalidAtom;
	asAtomHandler::subtractreplace(res,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->exec_pos->local3.flags & ABC_OP_FORCEINT,context->exec_pos->local3.pos);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res,context->worker);
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
	asAtomHandler::multiplyreplace(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),context->worker,*context->exec_pos->arg1_constant,*context->exec_pos->arg2_constant,context->exec_pos->local3.flags & ABC_OP_FORCEINT,context->exec_pos->local3.pos);
	++(context->exec_pos);
}
void ABCVm::abc_multiply_local_constant_localresult(call_context* context)
{
	LOG_CALL("multiply_lcl");
	asAtomHandler::multiplyreplace(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),*context->exec_pos->arg2_constant,context->exec_pos->local3.flags & ABC_OP_FORCEINT,context->exec_pos->local3.pos);
	++(context->exec_pos);
}
void ABCVm::abc_multiply_constant_local_localresult(call_context* context)
{
	LOG_CALL("multiply_cll");
	asAtomHandler::multiplyreplace(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),context->worker,*context->exec_pos->arg1_constant,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->exec_pos->local3.flags & ABC_OP_FORCEINT,context->exec_pos->local3.pos);
	++(context->exec_pos);
}
void ABCVm::abc_multiply_local_local_localresult(call_context* context)
{
	LOG_CALL("multiply_lll "<<context->exec_pos->local_pos1<<"/"<<context->exec_pos->local_pos2<<"/"<<context->exec_pos->local3.pos);
	asAtomHandler::multiplyreplace(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->exec_pos->local3.flags & ABC_OP_FORCEINT,context->exec_pos->local3.pos);
	++(context->exec_pos);
}
void ABCVm::abc_multiply_constant_constant_setslotnocoerce(call_context* context)
{
	LOG_CALL("multiply_ccs");
	asAtom res = asAtomHandler::invalidAtom;
	asAtomHandler::multiplyreplace(res,context->worker,*context->exec_pos->arg1_constant,*context->exec_pos->arg2_constant,context->exec_pos->local3.flags & ABC_OP_FORCEINT,context->exec_pos->local3.pos);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res,context->worker);
	++(context->exec_pos);
}
void ABCVm::abc_multiply_local_constant_setslotnocoerce(call_context* context)
{
	LOG_CALL("multiply_lcs");
	asAtom res = asAtomHandler::invalidAtom;
	asAtomHandler::multiplyreplace(res,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),*context->exec_pos->arg2_constant,context->exec_pos->local3.flags & ABC_OP_FORCEINT,context->exec_pos->local3.pos);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res,context->worker);
	++(context->exec_pos);
}
void ABCVm::abc_multiply_constant_local_setslotnocoerce(call_context* context)
{
	LOG_CALL("multiply_cls");
	asAtom res = asAtomHandler::invalidAtom;
	asAtomHandler::multiplyreplace(res,context->worker,*context->exec_pos->arg1_constant,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->exec_pos->local3.flags & ABC_OP_FORCEINT,context->exec_pos->local3.pos);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res,context->worker);
	++(context->exec_pos);
}
void ABCVm::abc_multiply_local_local_setslotnocoerce(call_context* context)
{
	LOG_CALL("multiply_lls");
	asAtom res = asAtomHandler::invalidAtom;
	asAtomHandler::multiplyreplace(res,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->exec_pos->local3.flags & ABC_OP_FORCEINT,context->exec_pos->local3.pos);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res,context->worker);
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
	asAtomHandler::dividereplace(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),context->worker,*context->exec_pos->arg1_constant,*context->exec_pos->arg2_constant,context->exec_pos->local3.flags & ABC_OP_FORCEINT,context->exec_pos->local3.pos);
	++(context->exec_pos);
}
void ABCVm::abc_divide_local_constant_localresult(call_context* context)
{
	LOG_CALL("divide_lcl");
	asAtomHandler::dividereplace(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),*context->exec_pos->arg2_constant,context->exec_pos->local3.flags & ABC_OP_FORCEINT,context->exec_pos->local3.pos);
	++(context->exec_pos);
}
void ABCVm::abc_divide_constant_local_localresult(call_context* context)
{
	LOG_CALL("divide_cll");
	asAtomHandler::dividereplace(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),context->worker,*context->exec_pos->arg1_constant,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->exec_pos->local3.flags & ABC_OP_FORCEINT,context->exec_pos->local3.pos);
	++(context->exec_pos);
}
void ABCVm::abc_divide_local_local_localresult(call_context* context)
{
	LOG_CALL("divide_lll");
	asAtomHandler::dividereplace(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->exec_pos->local3.flags & ABC_OP_FORCEINT,context->exec_pos->local3.pos);
	++(context->exec_pos);
}
void ABCVm::abc_divide_constant_constant_setslotnocoerce(call_context* context)
{
	LOG_CALL("divide_ccs");
	asAtom res = asAtomHandler::invalidAtom;
	asAtomHandler::dividereplace(res,context->worker,*context->exec_pos->arg1_constant,*context->exec_pos->arg2_constant,context->exec_pos->local3.flags & ABC_OP_FORCEINT,context->exec_pos->local3.pos);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res,context->worker);
	++(context->exec_pos);
}
void ABCVm::abc_divide_local_constant_setslotnocoerce(call_context* context)
{
	LOG_CALL("divide_lcs");
	asAtom res = asAtomHandler::invalidAtom;
	asAtomHandler::dividereplace(res,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),*context->exec_pos->arg2_constant,context->exec_pos->local3.flags & ABC_OP_FORCEINT,context->exec_pos->local3.pos);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res,context->worker);
	++(context->exec_pos);
}
void ABCVm::abc_divide_constant_local_setslotnocoerce(call_context* context)
{
	LOG_CALL("divide_cls");
	asAtom res = asAtomHandler::invalidAtom;
	asAtomHandler::dividereplace(res,context->worker,*context->exec_pos->arg1_constant,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->exec_pos->local3.flags & ABC_OP_FORCEINT,context->exec_pos->local3.pos);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res,context->worker);
	++(context->exec_pos);
}
void ABCVm::abc_divide_local_local_setslotnocoerce(call_context* context)
{
	LOG_CALL("divide_lls");
	asAtom res = asAtomHandler::invalidAtom;
	asAtomHandler::dividereplace(res,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->exec_pos->local3.flags & ABC_OP_FORCEINT,context->exec_pos->local3.pos);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res,context->worker);
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
	asAtomHandler::moduloreplace(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),context->worker,*context->exec_pos->arg1_constant,*context->exec_pos->arg2_constant,context->exec_pos->local3.flags & ABC_OP_FORCEINT,context->exec_pos->local3.pos);
	++(context->exec_pos);
}
void ABCVm::abc_modulo_local_constant_localresult(call_context* context)
{
	LOG_CALL("modulo_lcl");
	asAtomHandler::moduloreplace(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),*context->exec_pos->arg2_constant,context->exec_pos->local3.flags & ABC_OP_FORCEINT,context->exec_pos->local3.pos);
	++(context->exec_pos);
}
void ABCVm::abc_modulo_constant_local_localresult(call_context* context)
{
	LOG_CALL("modulo_cll");
	asAtomHandler::moduloreplace(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),context->worker,*context->exec_pos->arg1_constant,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->exec_pos->local3.flags & ABC_OP_FORCEINT,context->exec_pos->local3.pos);
	++(context->exec_pos);
}
void ABCVm::abc_modulo_local_local_localresult(call_context* context)
{
	LOG_CALL("modulo_lll");
	asAtomHandler::moduloreplace(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->exec_pos->local3.flags & ABC_OP_FORCEINT,context->exec_pos->local3.pos);
	++(context->exec_pos);
}
void ABCVm::abc_modulo_constant_constant_setslotnocoerce(call_context* context)
{
	LOG_CALL("modulo_ccs");
	asAtom res = asAtomHandler::invalidAtom;
	asAtomHandler::moduloreplace(res,context->worker,*context->exec_pos->arg1_constant,*context->exec_pos->arg2_constant,context->exec_pos->local3.flags & ABC_OP_FORCEINT,context->exec_pos->local3.pos);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res,context->worker);
	++(context->exec_pos);
}
void ABCVm::abc_modulo_local_constant_setslotnocoerce(call_context* context)
{
	LOG_CALL("modulo_lcs");
	asAtom res = asAtomHandler::invalidAtom;
	asAtomHandler::moduloreplace(res,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),*context->exec_pos->arg2_constant,context->exec_pos->local3.flags & ABC_OP_FORCEINT,context->exec_pos->local3.pos);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res,context->worker);
	++(context->exec_pos);
}
void ABCVm::abc_modulo_constant_local_setslotnocoerce(call_context* context)
{
	LOG_CALL("modulo_cls");
	asAtom res = asAtomHandler::invalidAtom;
	asAtomHandler::moduloreplace(res,context->worker,*context->exec_pos->arg1_constant,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->exec_pos->local3.flags & ABC_OP_FORCEINT,context->exec_pos->local3.pos);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res,context->worker);
	++(context->exec_pos);
}
void ABCVm::abc_modulo_local_local_setslotnocoerce(call_context* context)
{
	LOG_CALL("modulo_lls");
	asAtom res = asAtomHandler::invalidAtom;
	asAtomHandler::moduloreplace(res,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->exec_pos->local3.flags & ABC_OP_FORCEINT,context->exec_pos->local3.pos);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res,context->worker);
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
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res,context->worker);
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
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res,context->worker);
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
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res,context->worker);
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
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res,context->worker);
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
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res,context->worker);
	++(context->exec_pos);
}
void ABCVm::abc_rshift_local_constant_setslotnocoerce(call_context* context)
{
	LOG_CALL("rShift_lcs ");
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	asAtomHandler::rshift(res,context->worker,*context->exec_pos->arg2_constant);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res,context->worker);
	++(context->exec_pos);
}
void ABCVm::abc_rshift_constant_local_setslotnocoerce(call_context* context)
{
	LOG_CALL("rShift_cls ");
	asAtom res = *context->exec_pos->arg1_constant;
	asAtomHandler::rshift(res,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2));

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res,context->worker);
	++(context->exec_pos);
}
void ABCVm::abc_rshift_local_local_setslotnocoerce(call_context* context)
{
	LOG_CALL("rShift_lls ");
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	asAtomHandler::rshift(res,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2));

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res,context->worker);
	++(context->exec_pos);
}

void ABCVm::abc_urshift_constant_constant(call_context* context)
{
	uint32_t i2=context->exec_pos->arg1_uint;
	uint32_t i1=(context->exec_pos->arg2_uint)&0x1f;
	LOG_CALL("urShift_cc "<<std::hex<<i2<<">>"<<i1<<std::dec);
	asAtom res;
	asAtomHandler::setUInt(res,context->worker,i2>>i1);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_urshift_local_constant(call_context* context)
{
	uint32_t i2=asAtomHandler::getUInt(context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1));
	uint32_t i1=(context->exec_pos->arg2_uint)&0x1f;
	LOG_CALL("urShift_lc "<<std::hex<<i2<<">>"<<i1<<std::dec);
	asAtom res;
	asAtomHandler::setUInt(res,context->worker,i2>>i1);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_urshift_constant_local(call_context* context)
{
	uint32_t i2=(context->exec_pos->arg1_uint);
	uint32_t i1=asAtomHandler::getUInt(context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2))&0x1f;
	LOG_CALL("urShift_cl "<<std::hex<<i2<<">>"<<i1<<std::dec);
	asAtom res;
	asAtomHandler::setUInt(res,context->worker,i2>>i1);
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
	asAtomHandler::setUInt(res,context->worker,i2>>i1);
	replacelocalresult(context,context->exec_pos->local3.pos,res);
	++(context->exec_pos);
}
void ABCVm::abc_urshift_local_constant_localresult(call_context* context)
{
	uint32_t i2=asAtomHandler::getUInt(context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1));
	uint32_t i1=(context->exec_pos->arg2_uint)&0x1f;
	LOG_CALL("urShift_lcl "<<std::hex<<i2<<">>"<<i1<<std::dec);
	asAtom res;
	asAtomHandler::setUInt(res,context->worker,i2>>i1);
	replacelocalresult(context,context->exec_pos->local3.pos,res);
	++(context->exec_pos);
}
void ABCVm::abc_urshift_constant_local_localresult(call_context* context)
{
	uint32_t i2=(context->exec_pos->arg1_uint);
	uint32_t i1=asAtomHandler::getUInt(context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2))&0x1f;
	LOG_CALL("urShift_cll "<<std::hex<<i2<<">>"<<i1<<std::dec);
	asAtom res;
	asAtomHandler::setUInt(res,context->worker,i2>>i1);
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
	asAtomHandler::setUInt(res,context->worker,i2>>i1);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res,context->worker);
	++(context->exec_pos);
}
void ABCVm::abc_urshift_local_constant_setslotnocoerce(call_context* context)
{
	uint32_t i2=asAtomHandler::getUInt(context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1));
	uint32_t i1=(context->exec_pos->arg2_uint)&0x1f;
	LOG_CALL("urShift_lcs "<<std::hex<<i2<<">>"<<i1<<std::dec);
	asAtom res;
	asAtomHandler::setUInt(res,context->worker,i2>>i1);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res,context->worker);
	++(context->exec_pos);
}
void ABCVm::abc_urshift_constant_local_setslotnocoerce(call_context* context)
{
	uint32_t i2=(context->exec_pos->arg1_uint);
	uint32_t i1=asAtomHandler::getUInt(context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2))&0x1f;
	LOG_CALL("urShift_cls "<<std::hex<<i2<<">>"<<i1<<std::dec);
	asAtom res;
	asAtomHandler::setUInt(res,context->worker,i2>>i1);

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res,context->worker);
	++(context->exec_pos);
}
void ABCVm::abc_urshift_local_local_setslotnocoerce(call_context* context)
{
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	asAtomHandler::urshift(res,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2));

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res,context->worker);
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
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res,context->worker);
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
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res,context->worker);
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
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res,context->worker);
	++(context->exec_pos);
}
void ABCVm::abc_bitand_local_local_setslotnocoerce(call_context* context)
{
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	asAtomHandler::bit_and(res,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2));

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res,context->worker);
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
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res,context->worker);
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
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res,context->worker);
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
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res,context->worker);
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
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res,context->worker);
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
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res,context->worker);
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
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res,context->worker);
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
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res,context->worker);
	++(context->exec_pos);
}
void ABCVm::abc_bitxor_local_local_setslotnocoerce(call_context *context)
{
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	asAtomHandler::bit_xor(res,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2));

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res,context->worker);
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
	bool ret = instanceOf(*context->exec_pos->arg1_constant,*context->exec_pos->arg2_constant);
	LOG_CALL("instanceof_cc "<<ret);

	RUNTIME_STACK_PUSH(context,asAtomHandler::fromBool(ret));
	++(context->exec_pos);
}
void ABCVm::abc_instanceof_local_constant(call_context* context)
{
	bool ret = instanceOf(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),*context->exec_pos->arg2_constant);
	LOG_CALL("instanceof_lc "<<ret);

	RUNTIME_STACK_PUSH(context,asAtomHandler::fromBool(ret));
	++(context->exec_pos);
}
void ABCVm::abc_instanceof_constant_local(call_context* context)
{
	bool ret = instanceOf(*context->exec_pos->arg1_constant,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2));
	LOG_CALL("instanceof_cl "<<ret);

	RUNTIME_STACK_PUSH(context,asAtomHandler::fromBool(ret));
	++(context->exec_pos);
}
void ABCVm::abc_instanceof_local_local(call_context* context)
{
	bool ret = instanceOf(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2));
	LOG_CALL("instanceof_ll "<<ret);

	RUNTIME_STACK_PUSH(context,asAtomHandler::fromBool(ret));
	++(context->exec_pos);
}
void ABCVm::abc_instanceof_constant_constant_localresult(call_context* context)
{
	bool ret = instanceOf(*context->exec_pos->arg1_constant,*context->exec_pos->arg2_constant);
	LOG_CALL("instanceof_ccl "<<ret);

	ASATOM_DECREF(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos));
	asAtomHandler::setBool(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),ret);
	++(context->exec_pos);
}
void ABCVm::abc_instanceof_local_constant_localresult(call_context* context)
{
	bool ret = instanceOf(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),*context->exec_pos->arg2_constant);
	LOG_CALL("instanceof_lcl "<<ret);

	ASATOM_DECREF(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos));
	asAtomHandler::setBool(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),ret);
	++(context->exec_pos);
}
void ABCVm::abc_instanceof_constant_local_localresult(call_context* context)
{
	bool ret = instanceOf(*context->exec_pos->arg1_constant,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2));
	LOG_CALL("instanceof_cll "<<ret);

	ASATOM_DECREF(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos));
	asAtomHandler::setBool(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),ret);
	++(context->exec_pos);
}
void ABCVm::abc_instanceof_local_local_localresult(call_context* context)
{
	bool ret = instanceOf(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2));
	LOG_CALL("instanceof_lll "<<ret<<" "<<asAtomHandler::toDebugString(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1))<<" "<<asAtomHandler::toDebugString(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2)));

	ASATOM_DECREF(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos));
	asAtomHandler::setBool(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),ret);
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
	if (USUALLY_TRUE((context->exec_pos->local3.flags & ABC_OP_FORCEINT)|| (
#ifdef LIGHTSPARK_64
			((arg1.uintval & 0xc000000000000007) ==ATOM_INTEGER)
#else
			((context->exec_pos->arg2_int & 0xc0000007) ==ATOM_INTEGER ) && ((arg1.uintval & 0xc0000007) ==ATOM_INTEGER )
#endif
			&& !asAtomHandler::isObject(oldres))))
	{
		// fast path for common case that both arguments are ints and the result doesn't overflow
		int64_t ret = context->exec_pos->arg2_int + asAtomHandler::getInt(arg1);
		LOG_CALL("add_i_cll_fast "<<asAtomHandler::toDebugString(arg1)<<"+"<<context->exec_pos->arg2_int<<" = "<<ret);
		asAtomHandler::set(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),asAtomHandler::fromInt((int32_t)ret));
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
	if (USUALLY_TRUE((context->exec_pos->local3.flags & ABC_OP_FORCEINT)|| (
#ifdef LIGHTSPARK_64
			((arg2.uintval & 0xc000000000000007) ==ATOM_INTEGER)
#else
			((context->exec_pos->arg1_int & 0xc0000007) ==ATOM_INTEGER ) && ((arg2.uintval & 0xc0000007) ==ATOM_INTEGER )
#endif
			&& !asAtomHandler::isObject(oldres))))
	{
		// fast path for common case that both arguments are ints and the result doesn't overflow
		int64_t ret = context->exec_pos->arg1_int+asAtomHandler::getInt(arg2);
		LOG_CALL("add_i_cll_fast "<<context->exec_pos->arg1_int<<"+"<<asAtomHandler::toDebugString(arg2)<<" = "<<ret);
		asAtomHandler::set(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),asAtomHandler::fromInt((int32_t)ret));
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
	if (USUALLY_TRUE((context->exec_pos->local3.flags & ABC_OP_FORCEINT)|| (
#ifdef LIGHTSPARK_64
			(((res.uintval | arg2.uintval) & 0xc000000000000007) ==ATOM_INTEGER)
#else
			(((res.uintval | arg2.uintval) & 0xc0000007) ==ATOM_INTEGER)
#endif
			&& !asAtomHandler::isObject(oldres))))
	{
		// fast path for common case that both arguments are ints and the result doesn't overflow
		int64_t ret = asAtomHandler::getInt(res)+asAtomHandler::getInt(arg2);
		LOG_CALL("add_i_lll_fast "<<asAtomHandler::toDebugString(res)<<"+"<<asAtomHandler::toDebugString(arg2)<<" = "<<ret);
		asAtomHandler::set(CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos),asAtomHandler::fromInt((int32_t)ret));
	}
	else
	{
		LOG_CALL("add_i_lll ");
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
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res,context->worker);
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
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res,context->worker);
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
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res,context->worker);
	++(context->exec_pos);
}
void ABCVm::abc_add_i_local_local_setslotnocoerce(call_context* context)
{
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	asAtomHandler::add_i(res,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2));

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	LOG_CALL("add_i_lls " << t << " "<< asAtomHandler::toDebugString(res) << " "<< asAtomHandler::toDebugString(obj));
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res,context->worker);
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
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res,context->worker);
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
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res,context->worker);
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
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res,context->worker);
	++(context->exec_pos);
}
void ABCVm::abc_subtract_i_local_local_setslotnocoerce(call_context* context)
{
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	asAtomHandler::subtract_i(res,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2));

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	LOG_CALL("subtract_i_lls " << t << " "<< asAtomHandler::toDebugString(res) << " "<< asAtomHandler::toDebugString(obj));
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res,context->worker);
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
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res,context->worker);
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
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res,context->worker);
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
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res,context->worker);
	++(context->exec_pos);
}
void ABCVm::abc_multiply_i_local_local_setslotnocoerce(call_context* context)
{
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	asAtomHandler::multiply_i(res,context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2));

	asAtom obj = CONTEXT_GETLOCAL(context,context->exec_pos->local3.pos);
	uint32_t t = context->exec_pos->local3.flags & ~ABC_OP_BITMASK_USED;
	LOG_CALL("multiply_i_lls " << t << " "<< asAtomHandler::toDebugString(res) << " "<< asAtomHandler::toDebugString(obj));
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res,context->worker);
	++(context->exec_pos);
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
	asAtomHandler::getObjectNoCheck(obj)->setSlotNoCoerce(t,res,context->worker);
	++(context->exec_pos);
}

void ABCVm::abc_dup_increment_local_localresult(call_context* context)
{
	LOG_CALL("dup_increment_ll "<<context->exec_pos->local_pos1<<" "<<context->exec_pos->local_pos2<<" "<<context->exec_pos->local3.pos<<" "<<asAtomHandler::toDebugString(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1)));
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	ASATOM_INCREF(res);
	replacelocalresult(context,context->exec_pos->local_pos2,res);
	asAtomHandler::increment(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->worker,false);
	replacelocalresult(context,context->exec_pos->local3.pos,res);
	++(context->exec_pos);
}
void ABCVm::abc_dup_decrement_local_localresult(call_context* context)
{
	LOG_CALL("dup_decrement_ll "<<context->exec_pos->local_pos1<<" "<<context->exec_pos->local_pos2<<" "<<context->exec_pos->local3.pos<<" "<<asAtomHandler::toDebugString(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1)));
	asAtom res = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	ASATOM_INCREF(res);
	replacelocalresult(context,context->exec_pos->local_pos2,res);
	asAtomHandler::decrement(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2),context->worker,false);
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
void ABCVm::abc_lookupswitch_constant(call_context* context)
{
	preloadedcodedata* here=context->exec_pos; //Base for the jumps is the instruction itself for the switch
	asAtom index_obj = *here->arg1_constant;
	int32_t t = (++(context->exec_pos))->arg3_int;
	preloadedcodedata* defaultdest=here+t;
	LOG_CALL("lookupswitch_c " << t);
	uint32_t count = (++(context->exec_pos))->arg3_uint;

	assert_and_throw(asAtomHandler::isNumeric(index_obj));
	unsigned int index=asAtomHandler::getUInt(context->worker,index_obj);
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
	unsigned int index=asAtomHandler::getUInt(context->worker,index_obj);
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
	asAtom* args = LS_STACKALLOC(asAtom,argcount);
	for (uint32_t i = argcount; i > 0 ; i--)
	{
		(++(context->exec_pos));
		if (context->exec_pos->arg2_uint == OPERANDTYPES::OP_LOCAL || context->exec_pos->arg2_uint == OPERANDTYPES::OP_CACHED_SLOT)
			args[i-1] = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
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
	LOG_CALL("callvoid_cc " << asAtomHandler::toDebugString(func)<<" "<<context->exec_pos->local3.flags);
	asAtom ret = asAtomHandler::invalidAtom;
	callFunctionClassRegexp(context, func, obj,ret);
	ASATOM_DECREF(ret);
	++(context->exec_pos);
}
void ABCVm::abc_callvoid_local_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom func = CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	asAtom obj = *instrptr->arg2_constant;
	LOG_CALL("callvoid_lc " << asAtomHandler::toDebugString(func)<<" "<<context->exec_pos->local3.flags);
	asAtom ret = asAtomHandler::invalidAtom;
	callFunctionClassRegexp(context, func, obj,ret);
	ASATOM_DECREF(ret);
	++(context->exec_pos);
}

void ABCVm::abc_callvoid_constant_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom func = *instrptr->arg1_constant;
	asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local_pos2);
	LOG_CALL("callvoid_cl " << asAtomHandler::toDebugString(func)<<" "<<context->exec_pos->local3.flags);
	asAtom ret = asAtomHandler::invalidAtom;
	callFunctionClassRegexp(context, func, obj,ret);
	ASATOM_DECREF(ret);
	++(context->exec_pos);
}
void ABCVm::abc_callvoid_local_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom func = CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local_pos2);
	LOG_CALL("callvoid_ll " << asAtomHandler::toDebugString(func)<<" "<<context->exec_pos->local3.flags);
	asAtom ret = asAtomHandler::invalidAtom;
	callFunctionClassRegexp(context, func, obj,ret);
	ASATOM_DECREF(ret);
	++(context->exec_pos);
}

void ABCVm::abc_call_constant_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom func = *instrptr->arg1_constant;
	asAtom obj = *instrptr->arg2_constant;
	LOG_CALL("call_cc " << asAtomHandler::toDebugString(func)<<" "<<context->exec_pos->local3.flags);
	asAtom ret = asAtomHandler::invalidAtom;
	callFunctionClassRegexp(context, func, obj,ret);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_call_local_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom func = CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	asAtom obj = *instrptr->arg2_constant;
	LOG_CALL("call_lc " << asAtomHandler::toDebugString(func)<<" "<<context->exec_pos->local3.flags);
	asAtom ret = asAtomHandler::invalidAtom;
	callFunctionClassRegexp(context, func, obj,ret);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_call_constant_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom func = *instrptr->arg1_constant;
	asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local_pos2);
	LOG_CALL("call_cl " << asAtomHandler::toDebugString(func)<<" "<<context->exec_pos->local3.flags);
	asAtom ret = asAtomHandler::invalidAtom;
	callFunctionClassRegexp(context, func, obj,ret);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_call_local_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom func = CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local_pos2);
	LOG_CALL("call_ll " << asAtomHandler::toDebugString(func)<<" "<<context->exec_pos->local3.flags);
	asAtom ret = asAtomHandler::invalidAtom;
	callFunctionClassRegexp(context, func, obj,ret);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_call_constant_constant_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom func = *instrptr->arg1_constant;
	asAtom obj = *instrptr->arg2_constant;
	LOG_CALL("call_ccl " << asAtomHandler::toDebugString(func)<<" "<<context->exec_pos->local3.flags);
	asAtom ret = asAtomHandler::invalidAtom;
	callFunctionClassRegexp(context, func, obj,ret);
	replacelocalresult(context,instrptr->local3.pos,ret);
	++(context->exec_pos);
}
void ABCVm::abc_call_local_constant_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom func = CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	asAtom obj = *instrptr->arg2_constant;
	LOG_CALL("call_lcl " << asAtomHandler::toDebugString(func)<<" "<<context->exec_pos->local3.flags);
	asAtom ret = asAtomHandler::invalidAtom;
	callFunctionClassRegexp(context, func, obj,ret);
	replacelocalresult(context,instrptr->local3.pos,ret);
	++(context->exec_pos);
}
void ABCVm::abc_call_constant_local_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom func = *instrptr->arg1_constant;
	asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local_pos2);
	LOG_CALL("call_cll " << asAtomHandler::toDebugString(func)<<" "<<context->exec_pos->local3.flags);
	asAtom ret = asAtomHandler::invalidAtom;
	callFunctionClassRegexp(context, func, obj,ret);
	replacelocalresult(context,instrptr->local3.pos,ret);
	++(context->exec_pos);
}
void ABCVm::abc_call_local_local_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom func = CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local_pos2);
	LOG_CALL("call_lll " << asAtomHandler::toDebugString(func)<<" "<<context->exec_pos->local3.flags);
	asAtom ret = asAtomHandler::invalidAtom;
	callFunctionClassRegexp(context, func, obj,ret);
	replacelocalresult(context,instrptr->local3.pos,ret);
	++(context->exec_pos);
}
void ABCVm::abc_coerce_constant(call_context* context)
{
	LOG_CALL("coerce_c");
	asAtom res = *context->exec_pos->arg1_constant;
	multiname* mn = context->exec_pos->cachedmultiname2;
	Type* type = mn->cachedType != nullptr ? mn->cachedType : Type::getTypeFromMultiname(mn, context->mi->context);
	if (type == nullptr)
	{
		LOG(LOG_ERROR,"coerce: type not found:"<< *mn);
		createError<TypeError>(context->worker,kClassNotFoundError,mn->qualifiedString(context->sys));
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
	Type* type = mn->cachedType != nullptr ? mn->cachedType : Type::getTypeFromMultiname(mn, context->mi->context);
	if (type == nullptr)
	{
		LOG(LOG_ERROR,"coerce: type not found:"<< *mn);
		createError<TypeError>(context->worker,kClassNotFoundError,mn->qualifiedString(context->sys));
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
	Type* type = mn->cachedType != nullptr ? mn->cachedType : Type::getTypeFromMultiname(mn, context->mi->context);
	if (type == nullptr)
	{
		LOG(LOG_ERROR,"coerce: type not found:"<< *mn);
		createError<TypeError>(context->worker,kClassNotFoundError,mn->qualifiedString(context->sys));
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
	Type* type = mn->cachedType != nullptr ? mn->cachedType : Type::getTypeFromMultiname(mn, context->mi->context);
	if (type == nullptr)
	{
		LOG(LOG_ERROR,"coerce: type not found:"<< *mn);
		createError<TypeError>(context->worker,kClassNotFoundError,mn->qualifiedString(context->sys));
		return;
	}

	bool coerced = !type->coerce(context->worker,res);
	if (context->exec_pos->local_pos1!=context->exec_pos->local3.pos)
	{
		if (coerced)
			ASATOM_INCREF(res);
		replacelocalresult(context,context->exec_pos->local3.pos,res);
	}
	++(context->exec_pos);
}

void ABCVm::abc_sxi1_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "sxi1_c");
	int32_t res=asAtomHandler::getUInt(context->worker,*instrptr->arg1_constant)&0x1 ? -1 : 0;
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
	int32_t res=asAtomHandler::getUInt(context->worker,CONTEXT_GETLOCAL(context,instrptr->local_pos1))&0x1 ? -1 : 0;
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
	int32_t res=asAtomHandler::getUInt(context->worker,*instrptr->arg1_constant)&0x1 ? -1 : 0;
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
	int32_t res=asAtomHandler::getUInt(context->worker,CONTEXT_GETLOCAL(context,instrptr->local_pos1))&0x1 ? -1 : 0;
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
	int32_t res=(int8_t)asAtomHandler::getUInt(context->worker,*instrptr->arg1_constant);
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
	int32_t res=(int8_t)asAtomHandler::getUInt(context->worker,CONTEXT_GETLOCAL(context,instrptr->local_pos1));
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
	int32_t res=(int8_t)asAtomHandler::getUInt(context->worker,*instrptr->arg1_constant);
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
	int32_t res=(int8_t)asAtomHandler::getUInt(context->worker,CONTEXT_GETLOCAL(context,instrptr->local_pos1));
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
	int32_t res=(int8_t)asAtomHandler::getUInt(context->worker,*instrptr->arg1_constant);
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
	int32_t res=(int16_t)asAtomHandler::getUInt(context->worker,CONTEXT_GETLOCAL(context,instrptr->local_pos1));
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
	int32_t res=(int16_t)asAtomHandler::getUInt(context->worker,*instrptr->arg1_constant);
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
	int32_t res=(int16_t)asAtomHandler::getUInt(context->worker,CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	ASATOM_DECREF(CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	asAtom ret=asAtomHandler::invalidAtom;
	asAtomHandler::setInt(ret,context->worker,res);
	replacelocalresult(context,instrptr->local3.pos,ret);
	++(context->exec_pos);
}
void ABCVm::abc_nextvalue_constant_constant(call_context* context)
{
	LOG_CALL("nextvalue_cc");
	asAtom ret=asAtomHandler::invalidAtom;
	asAtomHandler::toObject(*context->exec_pos->arg1_constant,context->worker)->nextValue(ret,asAtomHandler::getUInt(context->worker,*context->exec_pos->arg2_constant));
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_nextvalue_local_constant(call_context* context)
{
	LOG_CALL("nextvalue_lc");
	asAtom ret=asAtomHandler::invalidAtom;
	asAtomHandler::toObject(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),context->worker)->nextValue(ret,asAtomHandler::getUInt(context->worker,*context->exec_pos->arg2_constant));
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_nextvalue_constant_local(call_context* context)
{
	LOG_CALL("nextvalue_cl");
	asAtom ret=asAtomHandler::invalidAtom;
	asAtomHandler::toObject(*context->exec_pos->arg1_constant,context->worker)->nextValue(ret,asAtomHandler::getUInt(context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2)));
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_nextvalue_local_local(call_context* context)
{
	LOG_CALL("nextvalue_ll");
	asAtom ret=asAtomHandler::invalidAtom;
	asAtomHandler::toObject(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),context->worker)->nextValue(ret,asAtomHandler::getUInt(context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2)));
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_nextvalue_constant_constant_localresult(call_context* context)
{
	LOG_CALL("nextvalue_ccl");
	asAtom ret=asAtomHandler::invalidAtom;
	asAtomHandler::toObject(*context->exec_pos->arg1_constant,context->worker)->nextValue(ret,asAtomHandler::getUInt(context->worker,*context->exec_pos->arg2_constant));
	replacelocalresult(context,context->exec_pos->local3.pos,ret);
	++(context->exec_pos);
}
void ABCVm::abc_nextvalue_local_constant_localresult(call_context* context)
{
	LOG_CALL("nextvalue_lcl");
	asAtom ret=asAtomHandler::invalidAtom;
	asAtomHandler::toObject(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),context->worker)->nextValue(ret,asAtomHandler::getUInt(context->worker,*context->exec_pos->arg2_constant));
	replacelocalresult(context,context->exec_pos->local3.pos,ret);
	++(context->exec_pos);
}
void ABCVm::abc_nextvalue_constant_local_localresult(call_context* context)
{
	LOG_CALL("nextvalue_cll");
	asAtom ret=asAtomHandler::invalidAtom;
	asAtomHandler::toObject(*context->exec_pos->arg1_constant,context->worker)->nextValue(ret,asAtomHandler::getUInt(context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2)));
	replacelocalresult(context,context->exec_pos->local3.pos,ret);
	++(context->exec_pos);
}
void ABCVm::abc_nextvalue_local_local_localresult(call_context* context)
{
	LOG_CALL("nextvalue_lll");
	asAtom ret=asAtomHandler::invalidAtom;
	asAtomHandler::toObject(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),context->worker)->nextValue(ret,asAtomHandler::getUInt(context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2)));
	replacelocalresult(context,context->exec_pos->local3.pos,ret);
	++(context->exec_pos);
}
void ABCVm::abc_hasnext2_localresult(call_context* context)
{
	LOG_CALL( "hasnext2_l");
	uint32_t t = context->exec_pos->arg1_uint;
	uint32_t t2 = context->exec_pos->arg2_uint;
	asAtom ret=asAtomHandler::invalidAtom;
	asAtomHandler::setBool(ret,hasNext2(context,t,t2));
	replacelocalresult(context,context->exec_pos->local3.pos,ret);
	++(context->exec_pos);
}
void ABCVm::abc_hasnext2_iftrue(call_context* context)
{
	LOG_CALL( "hasnext2_iftrue");
	uint32_t t = context->exec_pos->arg1_uint;
	uint32_t t2 = context->exec_pos->arg2_uint;
	if (hasNext2(context,t,t2))
		context->exec_pos += context->exec_pos->arg3_int;
	else
		++(context->exec_pos);
}

void ABCVm::abc_nextname_constant_constant(call_context* context)
{
	LOG_CALL("nextname_cc");
	asAtom ret=asAtomHandler::invalidAtom;
	asAtomHandler::toObject(*context->exec_pos->arg1_constant,context->worker)->nextName(ret,asAtomHandler::getUInt(context->worker,*context->exec_pos->arg2_constant));
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_nextname_local_constant(call_context* context)
{
	LOG_CALL("nextname_lc");
	asAtom ret=asAtomHandler::invalidAtom;
	asAtomHandler::toObject(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),context->worker)->nextName(ret,asAtomHandler::getUInt(context->worker,*context->exec_pos->arg2_constant));
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_nextname_constant_local(call_context* context)
{
	LOG_CALL("nextname_cl");
	asAtom ret=asAtomHandler::invalidAtom;
	asAtomHandler::toObject(*context->exec_pos->arg1_constant,context->worker)->nextName(ret,asAtomHandler::getUInt(context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2)));
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_nextname_local_local(call_context* context)
{
	LOG_CALL("nextname_ll");
	asAtom ret=asAtomHandler::invalidAtom;
	asAtomHandler::toObject(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),context->worker)->nextName(ret,asAtomHandler::getUInt(context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2)));
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_nextname_constant_constant_localresult(call_context* context)
{
	LOG_CALL("nextname_ccl");
	asAtom ret=asAtomHandler::invalidAtom;
	asAtomHandler::toObject(*context->exec_pos->arg1_constant,context->worker)->nextName(ret,asAtomHandler::getUInt(context->worker,*context->exec_pos->arg2_constant));
	replacelocalresult(context,context->exec_pos->local3.pos,ret);
	++(context->exec_pos);
}
void ABCVm::abc_nextname_local_constant_localresult(call_context* context)
{
	LOG_CALL("nextname_lcl");
	asAtom ret=asAtomHandler::invalidAtom;
	asAtomHandler::toObject(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),context->worker)->nextName(ret,asAtomHandler::getUInt(context->worker,*context->exec_pos->arg2_constant));
	replacelocalresult(context,context->exec_pos->local3.pos,ret);
	++(context->exec_pos);
}
void ABCVm::abc_nextname_constant_local_localresult(call_context* context)
{
	LOG_CALL("nextname_cll");
	asAtom ret=asAtomHandler::invalidAtom;
	asAtomHandler::toObject(*context->exec_pos->arg1_constant,context->worker)->nextName(ret,asAtomHandler::getUInt(context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2)));
	replacelocalresult(context,context->exec_pos->local3.pos,ret);
	++(context->exec_pos);
}
void ABCVm::abc_nextname_local_local_localresult(call_context* context)
{
	LOG_CALL("nextname_lll");
	asAtom ret=asAtomHandler::invalidAtom;
	asAtomHandler::toObject(CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1),context->worker)->nextName(ret,asAtomHandler::getUInt(context->worker,CONTEXT_GETLOCAL(context,context->exec_pos->local_pos2)));
	replacelocalresult(context,context->exec_pos->local3.pos,ret);
	++(context->exec_pos);
}
void ABCVm::abc_newobject_noargs_localresult(call_context* context)
{
	context->explicitConstruction = true;
	asAtom ret=asAtomHandler::fromObjectNoPrimitive(new_asobject(context->worker));
	LOG_CALL("newObject_noargs_l " << asAtomHandler::toDebugString(ret));
	replacelocalresult(context,context->exec_pos->local3.pos,ret);
	context->explicitConstruction = false;
	++(context->exec_pos);
}
void ABCVm::constructpropMultiArgs_intern(call_context* context,asAtom& ret,asAtom& obj)
{
	context->explicitConstruction = true;

	uint32_t argcount = context->exec_pos->local2.pos;
	asAtom* args = LS_STACKALLOC(asAtom,argcount);
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

	++(context->exec_pos);
	ASObject* constructor = context->exec_pos->cacheobj1;
	multiname* name = context->exec_pos->cachedmultiname2;
	asAtom o=asAtomHandler::invalidAtom;
	if (constructor)
		o = asAtomHandler::fromObjectNoPrimitive(constructor);
	else
		asAtomHandler::toObject(obj,context->worker)->getVariableByMultiname(o,*name,GET_VARIABLE_OPTION::NONE, context->worker);

	if(asAtomHandler::isInvalid(o))
	{
		context->explicitConstruction = false;
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
			o_class->getInstance(context->worker,ret,true,args,argcount);
		}
		else if(asAtomHandler::isFunction(o))
		{
			constructFunction(ret,context, o,args,argcount);
		}
		else if (asAtomHandler::isTemplate(o))
		{
			context->explicitConstruction = false;
			createError<TypeError>(context->worker,kConstructOfNonFunctionError);
			return;
		}
		else
		{
			context->explicitConstruction = false;
			createError<TypeError>(context->worker,kNotConstructorError);
			return;
		}
	}
	catch(ASObject* exc)
	{
		context->explicitConstruction = false;
		LOG_CALL("Exception during object construction. Returning Undefined");
		//Handle eventual exceptions from the constructor, to fix the stack
		RUNTIME_STACK_PUSH(context,obj);
		throw;
	}
	context->explicitConstruction = false;
	ASATOM_DECREF(o);
	if (asAtomHandler::isObject(ret))
		asAtomHandler::getObjectNoCheck(ret)->setConstructorCallComplete();
	LOG_CALL("End of constructing " << *name<<" "<<asAtomHandler::toDebugString(obj)<<" result:"<<asAtomHandler::toDebugString(ret)<<(constructor ? " with constructor" : ""));
}
void ABCVm::abc_constructpropMultiArgs_constant(call_context* context)
{
	asAtom obj= *context->exec_pos->arg1_constant;
	LOG_CALL( "constructprop_MultiArgs_c");
	asAtom ret=asAtomHandler::invalidAtom;
	constructpropMultiArgs_intern(context,ret,obj);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_constructpropMultiArgs_local(call_context* context)
{
	asAtom obj= CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	LOG_CALL( "constructprop_MultiArgs_l");
	asAtom ret=asAtomHandler::invalidAtom;
	constructpropMultiArgs_intern(context,ret,obj);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_constructpropMultiArgs_constant_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj= *context->exec_pos->arg1_constant;
	LOG_CALL( "constructprop_MultiArgs_c_lr");
	asAtom res=asAtomHandler::invalidAtom;
	constructpropMultiArgs_intern(context,res,obj);
	replacelocalresult(context,instrptr->local3.pos,res);
	++(context->exec_pos);
}
void ABCVm::abc_constructpropMultiArgs_local_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj= CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
	LOG_CALL( "constructprop_MultiArgs_l_lr ");
	asAtom res=asAtomHandler::invalidAtom;
	constructpropMultiArgs_intern(context,res,obj);
	replacelocalresult(context,instrptr->local3.pos,res);
	++(context->exec_pos);
}
