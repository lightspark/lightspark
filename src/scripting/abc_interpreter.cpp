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
#include "parsing/streams.h"
#include <string>
#include <sstream>

using namespace std;
using namespace lightspark;

uint64_t ABCVm::profilingCheckpoint(uint64_t& startTime)
{
	uint64_t cur=compat_get_thread_cputime_us();
	uint64_t ret=cur-startTime;
	startTime=cur;
	return ret;
}

void checkPropertyException(ASObject* obj,multiname* name, asAtom& prop)
{
	if (obj->getClass() && obj->getClass()->findBorrowedSettable(*name))
		throwError<ReferenceError>(kWriteOnlyError, name->normalizedNameUnresolved(obj->getSystemState()), obj->getClassName());
	if (obj->getClass() && obj->getClass()->isSealed)
		throwError<ReferenceError>(kReadSealedError, name->normalizedNameUnresolved(obj->getSystemState()), obj->getClass()->getQualifiedClassName());
	if (name->isEmpty() || !name->hasEmptyNS)
		throwError<ReferenceError>(kReadSealedErrorNs, name->normalizedNameUnresolved(obj->getSystemState()), obj->getClassName());
	if (obj->is<Undefined>())
		throwError<TypeError>(kConvertUndefinedToObjectError);
	if (Log::getLevel() >= LOG_NOT_IMPLEMENTED && (obj->getClass() && !obj->getClass()->isSealed))
		LOG(LOG_NOT_IMPLEMENTED,"getProperty: " << name->normalizedNameUnresolved(obj->getSystemState()) << " not found on " << obj->toDebugString() << " "<<obj->getClassName());
	prop = asAtom::undefinedAtom;
}

void ABCVm::executeFunction(call_context* context)
{
#ifdef PROFILING_SUPPORT
	if(mi->profTime.empty())
		mi->profTime.resize(code_len,0);
	uint64_t startTime=compat_get_thread_cputime_us();
#define PROF_ACCOUNT_TIME(a, b)  do{a+=b;}while(0)
#define PROF_IGNORE_TIME(a) do{ a; } while(0)
#else
#define PROF_ACCOUNT_TIME(a, b) do{ ; }while(0)
#define PROF_IGNORE_TIME(a) do{ ; } while(0)
#endif

	//Each case block builds the correct parameters for the interpreter function and call it
	while(!context->returning)
	{
#ifdef PROFILING_SUPPORT
		uint32_t instructionPointer=code.tellg();
#endif
		//LOG(LOG_INFO,"opcode:"<<(context->stackp-context->stack)<<" "<< hex<<(int)((context->exec_pos->data)&0x1ff));

		// context->exec_pos points to the current instruction, every abc_function has to make sure
		// it points to the next valid instruction after execution
		abcfunctions[(context->exec_pos->data)&0x1ff](context);

		PROF_ACCOUNT_TIME(mi->profTime[instructionPointer],profilingCheckpoint(startTime));
	}
	return;

#undef PROF_ACCOUNT_TIME
#undef PROF_IGNORE_TIME
}

ABCVm::abc_function ABCVm::abcfunctions[]={
	abc_invalidinstruction, // 0x00
	abc_bkpt,
	abc_nop,
	abc_throw,
	abc_getSuper,
	abc_setSuper,
	abc_dxns,
	abc_dxnslate,
	abc_kill,
	abc_label,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_ifnlt, //0x0c
	abc_ifnle,
	abc_ifngt,
	abc_ifnge,

	abc_jump, // 0x10
	abc_iftrue,
	abc_iffalse,
	abc_ifeq,
	abc_ifne,
	abc_iflt,
	abc_ifle,
	abc_ifgt,
	abc_ifge,
	abc_ifstricteq,
	abc_ifstrictne,
	abc_lookupswitch,
	abc_pushwith,
	abc_popscope,
	abc_nextname,
	abc_hasnext,

	abc_pushnull,// 0x20
	abc_pushundefined,
	abc_invalidinstruction,
	abc_nextvalue,
	abc_pushbyte,
	abc_pushshort,
	abc_pushtrue,
	abc_pushfalse,
	abc_pushnan,
	abc_pop,
	abc_dup,
	abc_swap,
	abc_pushstring,
	abc_pushint,
	abc_pushuint,
	abc_pushdouble,

	abc_pushScope, // 0x30
	abc_pushnamespace,
	abc_hasnext2,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_li8,
	abc_li16,
	abc_li32,
	abc_lf32,
	abc_lf64,
	abc_si8,
	abc_si16,
	abc_si32,
	abc_sf32,
	abc_sf64,
	abc_invalidinstruction,

	abc_newfunction,// 0x40
	abc_call,
	abc_construct,
	abc_callMethod,
	abc_callstatic,
	abc_callsuper,
	abc_callproperty,
	abc_returnvoid,
	abc_returnvalue,
	abc_constructsuper,
	abc_constructprop,
	abc_invalidinstruction,
	abc_callproplex,
	abc_invalidinstruction,
	abc_callsupervoid,
	abc_callpropvoid,

	abc_sxi1,// 0x50
	abc_sxi8,
	abc_sxi16,
	abc_constructgenerictype,
	abc_invalidinstruction,
	abc_newobject,
	abc_newarray,
	abc_newactivation,
	abc_newclass,
	abc_getdescendants,
	abc_newcatch,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_findpropstrict,
	abc_findproperty,
	abc_finddef,

	abc_getlex,// 0x60
	abc_setproperty,
	abc_getlocal,
	abc_setlocal,
	abc_getglobalscope,
	abc_getscopeobject,
	abc_getProperty,
	abc_invalidinstruction,
	abc_initproperty,
	abc_invalidinstruction,
	abc_deleteproperty,
	abc_invalidinstruction,
	abc_getslot,
	abc_setslot,
	abc_getglobalSlot,
	abc_setglobalSlot,

	abc_convert_s,// 0x70
	abc_esc_xelem,
	abc_esc_xattr,
	abc_convert_i,
	abc_convert_u,
	abc_convert_d,
	abc_convert_b,
	abc_convert_o,
	abc_checkfilter,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,

	abc_coerce, // 0x80
	abc_invalidinstruction,
	abc_coerce_a,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_coerce_s,
	abc_astype,
	abc_astypelate,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,

	abc_negate, //0x90
	abc_increment,
	abc_inclocal,
	abc_decrement,
	abc_declocal,
	abc_typeof,
	abc_not,
	abc_bitnot,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,

	abc_add, //0xa0
	abc_subtract,
	abc_multiply,
	abc_divide,
	abc_modulo,
	abc_lshift,
	abc_rshift,
	abc_urshift,
	abc_bitand,
	abc_bitor,
	abc_bitxor,
	abc_equals,
	abc_strictequals,
	abc_lessthan,
	abc_lessequals,
	abc_greaterthan,

	abc_greaterequals,// 0xb0
	abc_instanceof,
	abc_istype,
	abc_istypelate,
	abc_in,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,

	abc_increment_i, // 0xc0
	abc_decrement_i,
	abc_inclocal_i,
	abc_declocal_i,
	abc_negate_i,
	abc_add_i,
	abc_subtract_i,
	abc_multiply_i,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,

	abc_getlocal_0, // 0xd0
	abc_getlocal_1,
	abc_getlocal_2,
	abc_getlocal_3,
	abc_setlocal_0,
	abc_setlocal_1,
	abc_setlocal_2,
	abc_setlocal_3,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,

	abc_invalidinstruction, // 0xe0
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_debug,

	abc_debugline,// 0xf0
	abc_debugfile,
	abc_bkptline,
	abc_timestamp,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,

	// instructions for optimized opcodes (indicated by 0x01xx)
	abc_increment_local, // 0x100 ABC_OP_OPTIMZED_INCREMENT
	abc_increment_local_localresult,
	abc_decrement_local, // 0x102 ABC_OP_OPTIMZED_DECREMENT
	abc_decrement_local_localresult,
	abc_pushScope_constant, // 0x104 ABC_OP_OPTIMZED_PUSHSCOPE
	abc_pushScope_local,
	abc_getPropertyStaticName_constant, // 0x106 ABC_OP_OPTIMZED_GETPROPERTY_STATICNAME
	abc_getPropertyStaticName_local,
	abc_getPropertyStaticName_constant_localresult,
	abc_getPropertyStaticName_local_localresult,
	abc_getlex_localresult, // 0x10a ABC_OP_OPTIMZED_GETLEX
	abc_callpropertyStaticName_localresult, // 0x10b ABC_OP_OPTIMZED_CALLPROPERTY_STATICNAME_LOCALRESULT
	abc_ifnlt, //0x10c  jump data may have the 0x100 bit set, so we also add the jump opcodes here
	abc_ifnle,
	abc_ifngt,
	abc_ifnge,

	abc_jump, // 0x110
	abc_iftrue,
	abc_iffalse,
	abc_ifeq,
	abc_ifne,
	abc_iflt,
	abc_ifle,
	abc_ifgt,
	abc_ifge,
	abc_ifstricteq,
	abc_ifstrictne,
	abc_lookupswitch,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,

	// optimized arithmetical operations for all possible combinations of operand1/operand2/result
	abc_multiply_constant_constant, // 0x120 ABC_OP_OPTIMZED_MULTIPLY
	abc_multiply_local_constant,
	abc_multiply_constant_local,
	abc_multiply_local_local,
	abc_multiply_constant_constant_localresult,
	abc_multiply_local_constant_localresult,
	abc_multiply_constant_local_localresult,
	abc_multiply_local_local_localresult,
	abc_bitor_constant_constant, // 0x128 ABC_OP_OPTIMZED_BITOR
	abc_bitor_local_constant,
	abc_bitor_constant_local,
	abc_bitor_local_local,
	abc_bitor_constant_constant_localresult,
	abc_bitor_local_constant_localresult,
	abc_bitor_constant_local_localresult,
	abc_bitor_local_local_localresult,

	abc_bitxor_constant_constant, // 0x130 ABC_OP_OPTIMZED_BITXOR
	abc_bitxor_local_constant,
	abc_bitxor_constant_local,
	abc_bitxor_local_local,
	abc_bitxor_constant_constant_localresult,
	abc_bitxor_local_constant_localresult,
	abc_bitxor_constant_local_localresult,
	abc_bitxor_local_local_localresult,
	abc_subtract_constant_constant, // 0x138 ABC_OP_OPTIMZED_SUBTRACT
	abc_subtract_local_constant,
	abc_subtract_constant_local,
	abc_subtract_local_local,
	abc_subtract_constant_constant_localresult,
	abc_subtract_local_constant_localresult,
	abc_subtract_constant_local_localresult,
	abc_subtract_local_local_localresult,

	abc_add_constant_constant, // 0x140 ABC_OP_OPTIMZED_ADD
	abc_add_local_constant,
	abc_add_constant_local,
	abc_add_local_local,
	abc_add_constant_constant_localresult,
	abc_add_local_constant_localresult,
	abc_add_constant_local_localresult,
	abc_add_local_local_localresult,
	abc_divide_constant_constant, // 0x148 ABC_OP_OPTIMZED_DIVIDE
	abc_divide_local_constant,
	abc_divide_constant_local,
	abc_divide_local_local,
	abc_divide_constant_constant_localresult,
	abc_divide_local_constant_localresult,
	abc_divide_constant_local_localresult,
	abc_divide_local_local_localresult,

	abc_modulo_constant_constant, // 0x150 ABC_OP_OPTIMZED_MODULO
	abc_modulo_local_constant,
	abc_modulo_constant_local,
	abc_modulo_local_local,
	abc_modulo_constant_constant_localresult,
	abc_modulo_local_constant_localresult,
	abc_modulo_constant_local_localresult,
	abc_modulo_local_local_localresult,
	abc_lshift_constant_constant, // 0x158 ABC_OP_OPTIMZED_LSHIFT
	abc_lshift_local_constant,
	abc_lshift_constant_local,
	abc_lshift_local_local,
	abc_lshift_constant_constant_localresult,
	abc_lshift_local_constant_localresult,
	abc_lshift_constant_local_localresult,
	abc_lshift_local_local_localresult,

	abc_rshift_constant_constant, // 0x160 ABC_OP_OPTIMZED_RSHIFT
	abc_rshift_local_constant,
	abc_rshift_constant_local,
	abc_rshift_local_local,
	abc_rshift_constant_constant_localresult,
	abc_rshift_local_constant_localresult,
	abc_rshift_constant_local_localresult,
	abc_rshift_local_local_localresult,
	abc_urshift_constant_constant, // 0x168 ABC_OP_OPTIMZED_URSHIFT
	abc_urshift_local_constant,
	abc_urshift_constant_local,
	abc_urshift_local_local,
	abc_urshift_constant_constant_localresult,
	abc_urshift_local_constant_localresult,
	abc_urshift_constant_local_localresult,
	abc_urshift_local_local_localresult,

	abc_bitand_constant_constant, // 0x170 ABC_OP_OPTIMZED_BITAND
	abc_bitand_local_constant,
	abc_bitand_constant_local,
	abc_bitand_local_local,
	abc_bitand_constant_constant_localresult,
	abc_bitand_local_constant_localresult,
	abc_bitand_constant_local_localresult,
	abc_bitand_local_local_localresult,
	abc_getProperty_constant_constant, // 0x178 ABC_OP_OPTIMZED_GETPROPERTY
	abc_getProperty_local_constant,
	abc_getProperty_constant_local,
	abc_getProperty_local_local,
	abc_getProperty_constant_constant_localresult,
	abc_getProperty_local_constant_localresult,
	abc_getProperty_constant_local_localresult,
	abc_getProperty_local_local_localresult,
	abc_ifeq_constant_constant, // 0x180 ABC_OP_OPTIMZED_IFEQ
	abc_ifeq_local_constant,
	abc_ifeq_constant_local,
	abc_ifeq_local_local,
	abc_ifne_constant_constant, // 0x184 ABC_OP_OPTIMZED_IFNE
	abc_ifne_local_constant,
	abc_ifne_constant_local,
	abc_ifne_local_local,
	abc_iflt_constant_constant, // 0x188 ABC_OP_OPTIMZED_IFLT
	abc_iflt_local_constant,
	abc_iflt_constant_local,
	abc_iflt_local_local,
	abc_ifle_constant_constant, // 0x18c ABC_OP_OPTIMZED_IFLE
	abc_ifle_local_constant,
	abc_ifle_constant_local,
	abc_ifle_local_local,
	abc_ifgt_constant_constant, // 0x190 ABC_OP_OPTIMZED_IFGT
	abc_ifgt_local_constant,
	abc_ifgt_constant_local,
	abc_ifgt_local_local,
	abc_ifge_constant_constant, // 0x194 ABC_OP_OPTIMZED_IFGE
	abc_ifge_local_constant,
	abc_ifge_constant_local,
	abc_ifge_local_local,
	abc_ifstricteq_constant_constant, // 0x198 ABC_OP_OPTIMZED_IFSTRICTEQ
	abc_ifstricteq_local_constant,
	abc_ifstricteq_constant_local,
	abc_ifstricteq_local_local,
	abc_ifstrictne_constant_constant, // 0x19c ABC_OP_OPTIMZED_IFSTRICTNE
	abc_ifstrictne_local_constant,
	abc_ifstrictne_constant_local,
	abc_ifstrictne_local_local,

	abc_callpropertyStaticName_constant_constant,// 0x1a0 ABC_OP_OPTIMZED_CALLPROPERTY_STATICNAME
	abc_callpropertyStaticName_local_constant,
	abc_callpropertyStaticName_constant_local,
	abc_callpropertyStaticName_local_local,
	abc_callpropertyStaticName_constant_constant_localresult,
	abc_callpropertyStaticName_local_constant_localresult,
	abc_callpropertyStaticName_constant_local_localresult,
	abc_callpropertyStaticName_local_local_localresult,
	abc_greaterthan_constant_constant,// 0x1a8 ABC_OP_OPTIMZED_GREATERTHAN
	abc_greaterthan_local_constant,
	abc_greaterthan_constant_local,
	abc_greaterthan_local_local,
	abc_greaterthan_constant_constant_localresult,
	abc_greaterthan_local_constant_localresult,
	abc_greaterthan_constant_local_localresult,
	abc_greaterthan_local_local_localresult,

	abc_iftrue_constant,// 0x1b0 ABC_OP_OPTIMZED_IFTRUE
	abc_iftrue_local,
	abc_iffalse_constant,// 0x1b2 ABC_OP_OPTIMZED_IFFALSE
	abc_iffalse_local,
	abc_convert_d_constant,// 0x1b4 ABC_OP_OPTIMZED_CONVERTD
	abc_convert_d_local,
	abc_convert_d_constant_localresult,
	abc_convert_d_local_localresult,
	abc_returnvalue_constant,// 0x1b8 ABC_OP_OPTIMZED_RETURNVALUE
	abc_returnvalue_local,
	
	abc_invalidinstruction
};

void ABCVm::abc_bkpt(call_context* context)
{
	//bkpt
	LOG_CALL( _("bkpt") );
	++(context->exec_pos);
}
void ABCVm::abc_nop(call_context* context)
{
	//nop
	++(context->exec_pos);
}
void ABCVm::abc_throw(call_context* context)
{
	//throw
	_throw(context);
}
void ABCVm::abc_getSuper(call_context* context)
{
	//getsuper
	uint32_t t = (++(context->exec_pos))->data;
	getSuper(context,t);
	++(context->exec_pos);
}
void ABCVm::abc_setSuper(call_context* context)
{
	//setsuper
	uint32_t t = (++(context->exec_pos))->data;
	setSuper(context,t);
	++(context->exec_pos);
}
void ABCVm::abc_dxns(call_context* context)
{
	//dxns
	uint32_t t = (++(context->exec_pos))->data;
	dxns(context,t);
	++(context->exec_pos);
}
void ABCVm::abc_dxnslate(call_context* context)
{
	//dxnslate
	RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v,context->mi->context->root->getSystemState());
	dxnslate(context, v);
	++(context->exec_pos);
}
void ABCVm::abc_kill(call_context* context)
{
	//kill
	uint32_t t = (++(context->exec_pos))->data;
	LOG_CALL( "kill " << t);
	ASATOM_DECREF(context->locals[t]);
	context->locals[t]=asAtom::undefinedAtom;
	++(context->exec_pos);
}
void ABCVm::abc_label(call_context* context)
{
	//label
	LOG_CALL("label");
	++(context->exec_pos);
}
void ABCVm::abc_ifnlt(call_context* context)
{
	//ifnlt
	int32_t t = (*context->exec_pos).jumpdata.jump;
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POP_CREATE(context,v2);
	bool cond=!(v2->isLess(context->mi->context->root->getSystemState(),*v1) == TTRUE);
	ASATOM_DECREF_POINTER(v2);
	ASATOM_DECREF_POINTER(v1);
	LOG_CALL(_("ifNLT (") << ((cond)?_("taken)"):_("not taken)")));

	if(cond)
		context->exec_pos += t+1;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifnle(call_context* context)
{
	//ifnle
	int32_t t = (*context->exec_pos).jumpdata.jump;
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POP_CREATE(context,v2);
	bool cond=!(v1->isLess(context->mi->context->root->getSystemState(),*v2) == TFALSE);
	ASATOM_DECREF_POINTER(v2);
	ASATOM_DECREF_POINTER(v1);
	LOG_CALL(_("ifNLE (") << ((cond)?_("taken)"):_("not taken)")));

	if(cond)
		context->exec_pos += t+1;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifngt(call_context* context)
{
	//ifngt
	int32_t t = (*context->exec_pos).jumpdata.jump;
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POP_CREATE(context,v2);
	bool cond=!(v1->isLess(context->mi->context->root->getSystemState(),*v2) == TTRUE);
	ASATOM_DECREF_POINTER(v2);
	ASATOM_DECREF_POINTER(v1);
	LOG_CALL(_("ifNGT (") << ((cond)?_("taken)"):_("not taken)")));

	if(cond)
		context->exec_pos += t+1;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifnge(call_context* context)
{
	//ifnge
	int32_t t = (*context->exec_pos).jumpdata.jump;
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POP_CREATE(context,v2);
	bool cond=!(v2->isLess(context->mi->context->root->getSystemState(),*v1) == TFALSE);
	ASATOM_DECREF_POINTER(v2);
	ASATOM_DECREF_POINTER(v1);
	LOG_CALL(_("ifNGE (") << ((cond)?_("taken)"):_("not taken)")));

	if(cond)
		context->exec_pos += t+1;
	else
		++(context->exec_pos);
}
void ABCVm::abc_jump(call_context* context)
{
	//jump
	int32_t t = (*context->exec_pos).jumpdata.jump;

	LOG_CALL("jump:"<<t);
	context->exec_pos += t+1;
}
void ABCVm::abc_iftrue(call_context* context)
{
	//iftrue
	int32_t t = (*context->exec_pos).jumpdata.jump;

	RUNTIME_STACK_POP_CREATE(context,v1);
	bool cond=v1->Boolean_concrete();
	LOG_CALL(_("ifTrue (") << ((cond)?_("taken)"):_("not taken)")));
	ASATOM_DECREF_POINTER(v1);
	if(cond)
		context->exec_pos += t+1;
	else
		++(context->exec_pos);
}
void ABCVm::abc_iftrue_constant(call_context* context)
{
	//ifeq
	int32_t t = (*context->exec_pos).jumpdata.jump;
	bool cond=context->exec_pos->arg1_constant->Boolean_concrete();
	LOG_CALL(_("ifTrue_c (") << ((cond)?_("taken)"):_("not taken)")));
	if(cond)
		context->exec_pos += t+1;
	else
		++(context->exec_pos);
}
void ABCVm::abc_iftrue_local(call_context* context)
{
	//ifeq
	int32_t t = (*context->exec_pos).jumpdata.jump;
	bool cond=context->locals[context->exec_pos->local_pos1].Boolean_concrete();
	LOG_CALL(_("ifTrue_l (") << ((cond)?_("taken)"):_("not taken)")));
	if(cond)
		context->exec_pos += t+1;
	else
		++(context->exec_pos);
}
void ABCVm::abc_iffalse(call_context* context)
{
	//iffalse
	int32_t t = (*context->exec_pos).jumpdata.jump;

	RUNTIME_STACK_POP_CREATE(context,v1);
	bool cond=!v1->Boolean_concrete();
	LOG_CALL(_("ifFalse (") << ((cond)?_("taken"):_("not taken")) << ')');
	ASATOM_DECREF_POINTER(v1);
	if(cond)
		context->exec_pos += t+1;
	else
		++(context->exec_pos);
}
void ABCVm::abc_iffalse_constant(call_context* context)
{
	//ifeq
	int32_t t = (*context->exec_pos).jumpdata.jump;
	bool cond=!context->exec_pos->arg1_constant->Boolean_concrete();
	LOG_CALL(_("ifFalse_c (") << ((cond)?_("taken)"):_("not taken)")));
	if(cond)
		context->exec_pos += t+1;
	else
		++(context->exec_pos);
}
void ABCVm::abc_iffalse_local(call_context* context)
{
	//ifeq
	int32_t t = (*context->exec_pos).jumpdata.jump;
	bool cond=!context->locals[context->exec_pos->local_pos1].Boolean_concrete();
	LOG_CALL(_("ifFalse_l (") << ((cond)?_("taken)"):_("not taken)")));
	if(cond)
		context->exec_pos += t+1;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifeq(call_context* context)
{
	//ifeq
	int32_t t = (*context->exec_pos).jumpdata.jump;

	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POP_CREATE(context,v2);
	bool cond=(v1->isEqual(context->mi->context->root->getSystemState(),*v2));
	LOG_CALL(_("ifEq (") << ((cond)?_("taken)"):_("not taken)")));
	ASATOM_DECREF_POINTER(v2);
	ASATOM_DECREF_POINTER(v1);
	if(cond)
		context->exec_pos += t+1;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifeq_constant_constant(call_context* context)
{
	//ifeq
	int32_t t = (*context->exec_pos).jumpdata.jump;
	bool cond=context->exec_pos->arg1_constant->isEqual(context->mi->context->root->getSystemState(),*context->exec_pos->arg2_constant) == TTRUE;
	LOG_CALL(_("ifEq_cc (") << ((cond)?_("taken)"):_("not taken)")));
	if(cond)
		context->exec_pos += t+1;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifeq_local_constant(call_context* context)
{
	//ifeq
	int32_t t = (*context->exec_pos).jumpdata.jump;
	bool cond=context->locals[context->exec_pos->local_pos1].isEqual(context->mi->context->root->getSystemState(),*context->exec_pos->arg2_constant) == TTRUE;
	LOG_CALL(_("ifEq_lc (") << ((cond)?_("taken)"):_("not taken)")));
	if(cond)
		context->exec_pos += t+1;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifeq_constant_local(call_context* context)
{
	//ifeq
	int32_t t = (*context->exec_pos).jumpdata.jump;
	bool cond=context->exec_pos->arg1_constant->isEqual(context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos2]) == TTRUE;
	LOG_CALL(_("ifEq_cl (") << ((cond)?_("taken)"):_("not taken)")));
	if(cond)
		context->exec_pos += t+1;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifeq_local_local(call_context* context)
{
	//ifeq
	int32_t t = (*context->exec_pos).jumpdata.jump;
	bool cond=context->locals[context->exec_pos->local_pos1].isEqual(context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos2]) == TTRUE;
	LOG_CALL(_("ifEq_ll (") << ((cond)?_("taken)"):_("not taken)")));
	if(cond)
		context->exec_pos += t+1;
	else
		++(context->exec_pos);
}

void ABCVm::abc_ifne(call_context* context)
{
	//ifne
	int32_t t = (*context->exec_pos).jumpdata.jump;

	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POP_CREATE(context,v2);
	bool cond=!(v1->isEqual(context->mi->context->root->getSystemState(),*v2));
	LOG_CALL(_("ifNE (") << ((cond)?_("taken)"):_("not taken)")));
	ASATOM_DECREF_POINTER(v2);
	ASATOM_DECREF_POINTER(v1);
	if(cond)
		context->exec_pos += t+1;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifne_constant_constant(call_context* context)
{
	//ifne
	int32_t t = (*context->exec_pos).jumpdata.jump;
	bool cond=!context->exec_pos->arg1_constant->isEqual(context->mi->context->root->getSystemState(),*context->exec_pos->arg2_constant) == TTRUE;
	LOG_CALL(_("ifNE_cc (") << ((cond)?_("taken)"):_("not taken)")));
	if(cond)
		context->exec_pos += t+1;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifne_local_constant(call_context* context)
{
	//ifne
	int32_t t = (*context->exec_pos).jumpdata.jump;
	bool cond=!context->locals[context->exec_pos->local_pos1].isEqual(context->mi->context->root->getSystemState(),*context->exec_pos->arg2_constant) == TTRUE;
	LOG_CALL(_("ifNE_lc (") << ((cond)?_("taken)"):_("not taken)")));
	if(cond)
		context->exec_pos += t+1;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifne_constant_local(call_context* context)
{
	//ifne
	int32_t t = (*context->exec_pos).jumpdata.jump;
	bool cond=!context->exec_pos->arg1_constant->isEqual(context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos2]) == TTRUE;
	LOG_CALL(_("ifNE_cl (") << ((cond)?_("taken)"):_("not taken)")));
	if(cond)
		context->exec_pos += t+1;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifne_local_local(call_context* context)
{
	//ifne
	int32_t t = (*context->exec_pos).jumpdata.jump;
	bool cond=!context->locals[context->exec_pos->local_pos1].isEqual(context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos2]) == TTRUE;
	LOG_CALL(_("ifNE_ll (") << ((cond)?_("taken)"):_("not taken)")));
	if(cond)
		context->exec_pos += t+1;
	else
		++(context->exec_pos);
}
void ABCVm::abc_iflt(call_context* context)
{
	//iflt
	int32_t t = (*context->exec_pos).jumpdata.jump;

	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POP_CREATE(context,v2);
	bool cond=v2->isLess(context->mi->context->root->getSystemState(),*v1) == TTRUE;
	ASATOM_DECREF_POINTER(v2);
	ASATOM_DECREF_POINTER(v1);
	LOG_CALL(_("ifLT (") << ((cond)?_("taken)"):_("not taken)")));

	if(cond)
		context->exec_pos += t+1;
	else
		++(context->exec_pos);
}
void ABCVm::abc_iflt_constant_constant(call_context* context)
{
	//iflt
	int32_t t = (*context->exec_pos).jumpdata.jump;
	bool cond=context->exec_pos->arg1_constant->isLess(context->mi->context->root->getSystemState(),*context->exec_pos->arg2_constant) == TTRUE;
	LOG_CALL(_("ifLT_cc (") << ((cond)?_("taken)"):_("not taken)")));
	if(cond)
		context->exec_pos += t+1;
	else
		++(context->exec_pos);
}
void ABCVm::abc_iflt_local_constant(call_context* context)
{
	//iflt
	int32_t t = (*context->exec_pos).jumpdata.jump;
	bool cond=context->locals[context->exec_pos->local_pos1].isLess(context->mi->context->root->getSystemState(),*context->exec_pos->arg2_constant) == TTRUE;
	LOG_CALL(_("ifLT_lc (") << ((cond)?_("taken)"):_("not taken)")));
	if(cond)
		context->exec_pos += t+1;
	else
		++(context->exec_pos);
}
void ABCVm::abc_iflt_constant_local(call_context* context)
{
	//iflt
	int32_t t = (*context->exec_pos).jumpdata.jump;
	bool cond=context->exec_pos->arg1_constant->isLess(context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos2]) == TTRUE;
	LOG_CALL(_("ifLT_cl (") << ((cond)?_("taken)"):_("not taken)")));
	if(cond)
		context->exec_pos += t+1;
	else
		++(context->exec_pos);
}
void ABCVm::abc_iflt_local_local(call_context* context)
{
	//iflt
	int32_t t = (*context->exec_pos).jumpdata.jump;
	bool cond=context->locals[context->exec_pos->local_pos1].isLess(context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos2]) == TTRUE;
	LOG_CALL(_("ifLT_ll (") << ((cond)?_("taken)"):_("not taken)")));
	if(cond)
		context->exec_pos += t+1;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifle(call_context* context)
{
	//ifle
	int32_t t = (*context->exec_pos).jumpdata.jump;

	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POP_CREATE(context,v2);
	bool cond=v1->isLess(context->mi->context->root->getSystemState(),*v2) == TFALSE;
	ASATOM_DECREF_POINTER(v2);
	ASATOM_DECREF_POINTER(v1);
	LOG_CALL(_("ifLE (") << ((cond)?_("taken)"):_("not taken)")));

	if(cond)
		context->exec_pos += t+1;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifle_constant_constant(call_context* context)
{
	//ifle
	int32_t t = (*context->exec_pos).jumpdata.jump;
	bool cond=context->exec_pos->arg2_constant->isLess(context->mi->context->root->getSystemState(),*context->exec_pos->arg1_constant) == TFALSE;
	LOG_CALL(_("ifLE_cc (") << ((cond)?_("taken)"):_("not taken)")));
	if(cond)
		context->exec_pos += t+1;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifle_local_constant(call_context* context)
{
	//ifle
	int32_t t = (*context->exec_pos).jumpdata.jump;
	bool cond=context->exec_pos->arg2_constant->isLess(context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos1]) == TFALSE;
	LOG_CALL(_("ifLE_lc (") << ((cond)?_("taken)"):_("not taken)")));
	if(cond)
		context->exec_pos += t+1;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifle_constant_local(call_context* context)
{
	//ifle
	int32_t t = (*context->exec_pos).jumpdata.jump;
	bool cond=context->locals[context->exec_pos->local_pos2].isLess(context->mi->context->root->getSystemState(),*context->exec_pos->arg1_constant) == TFALSE;
	LOG_CALL(_("ifLE_cl (") << ((cond)?_("taken)"):_("not taken)")));
	if(cond)
		context->exec_pos += t+1;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifle_local_local(call_context* context)
{
	//ifle
	int32_t t = (*context->exec_pos).jumpdata.jump;
	bool cond=context->locals[context->exec_pos->local_pos2].isLess(context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos1]) == TFALSE;
	LOG_CALL(_("ifLE_ll (") << ((cond)?_("taken)"):_("not taken)")));
	if(cond)
		context->exec_pos += t+1;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifgt(call_context* context)
{
	//ifgt
	int32_t t = (*context->exec_pos).jumpdata.jump;

	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POP_CREATE(context,v2);
	bool cond=v1->isLess(context->mi->context->root->getSystemState(),*v2) == TTRUE;
	ASATOM_DECREF_POINTER(v2);
	ASATOM_DECREF_POINTER(v1);
	LOG_CALL(_("ifGT (") << ((cond)?_("taken)"):_("not taken)")));

	if(cond)
		context->exec_pos += t+1;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifgt_constant_constant(call_context* context)
{
	//ifgt
	int32_t t = (*context->exec_pos).jumpdata.jump;
	bool cond=context->exec_pos->arg2_constant->isLess(context->mi->context->root->getSystemState(),*context->exec_pos->arg1_constant) == TTRUE;
	LOG_CALL(_("ifGT_cc (") << ((cond)?_("taken)"):_("not taken)")));
	if(cond)
		context->exec_pos += t+1;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifgt_local_constant(call_context* context)
{
	//ifgt
	int32_t t = (*context->exec_pos).jumpdata.jump;
	bool cond=context->exec_pos->arg2_constant->isLess(context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos1]) == TTRUE;
	LOG_CALL(_("ifGT_lc (") << ((cond)?_("taken)"):_("not taken)")));
	if(cond)
		context->exec_pos += t+1;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifgt_constant_local(call_context* context)
{
	//ifgt
	int32_t t = (*context->exec_pos).jumpdata.jump;
	bool cond=context->locals[context->exec_pos->local_pos2].isLess(context->mi->context->root->getSystemState(),*context->exec_pos->arg1_constant) == TTRUE;
	LOG_CALL(_("ifGT_cl (") << ((cond)?_("taken)"):_("not taken)")));
	if(cond)
		context->exec_pos += t+1;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifgt_local_local(call_context* context)
{
	//ifgt
	int32_t t = (*context->exec_pos).jumpdata.jump;
	bool cond=context->locals[context->exec_pos->local_pos2].isLess(context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos1]) == TTRUE;
	LOG_CALL(_("ifGT_ll (") << ((cond)?_("taken)"):_("not taken)")));
	if(cond)
		context->exec_pos += t+1;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifge(call_context* context)
{
	//ifge
	int32_t t = (*context->exec_pos).jumpdata.jump;

	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POP_CREATE(context,v2);
	bool cond=v2->isLess(context->mi->context->root->getSystemState(),*v1) == TFALSE;
	ASATOM_DECREF_POINTER(v2);
	ASATOM_DECREF_POINTER(v1);
	LOG_CALL(_("ifGE (") << ((cond)?_("taken)"):_("not taken)")));

	if(cond)
		context->exec_pos += t+1;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifge_constant_constant(call_context* context)
{
	//ifge
	int32_t t = (*context->exec_pos).jumpdata.jump;
	bool cond=context->exec_pos->arg1_constant->isLess(context->mi->context->root->getSystemState(),*context->exec_pos->arg2_constant) == TFALSE;
	LOG_CALL(_("ifGE_cc (") << ((cond)?_("taken)"):_("not taken)")));
	if(cond)
		context->exec_pos += t+1;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifge_local_constant(call_context* context)
{
	//ifge
	int32_t t = (*context->exec_pos).jumpdata.jump;
	bool cond=context->locals[context->exec_pos->local_pos1].isLess(context->mi->context->root->getSystemState(),*context->exec_pos->arg2_constant) == TFALSE;
	LOG_CALL(_("ifGE_lc (") << ((cond)?_("taken)"):_("not taken)")));
	if(cond)
		context->exec_pos += t+1;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifge_constant_local(call_context* context)
{
	//ifge
	int32_t t = (*context->exec_pos).jumpdata.jump;
	bool cond=context->exec_pos->arg1_constant->isLess(context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos2]) == TFALSE;
	LOG_CALL(_("ifGE_cl (") << ((cond)?_("taken)"):_("not taken)")));
	if(cond)
		context->exec_pos += t+1;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifge_local_local(call_context* context)
{
	//ifge
	int32_t t = (*context->exec_pos).jumpdata.jump;
	bool cond=context->locals[context->exec_pos->local_pos1].isLess(context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos2]) == TFALSE;
	LOG_CALL(_("ifGE_ll (") << ((cond)?_("taken)"):_("not taken)")));
	if(cond)
		context->exec_pos += t+1;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifstricteq(call_context* context)
{
	//ifstricteq
	int32_t t = (*context->exec_pos).jumpdata.jump;

	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POP_CREATE(context,v2);

	bool cond=v1->isEqualStrict(context->mi->context->root->getSystemState(),*v2);
	LOG_CALL(_("ifStrictEq ")<<cond);
	ASATOM_DECREF_POINTER(v1);
	ASATOM_DECREF_POINTER(v2);
	if(cond)
		context->exec_pos += t+1;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifstricteq_constant_constant(call_context* context)
{
	//ifstricteq
	int32_t t = (*context->exec_pos).jumpdata.jump;
	bool cond=context->exec_pos->arg1_constant->isEqualStrict(context->mi->context->root->getSystemState(),*context->exec_pos->arg2_constant);
	LOG_CALL(_("ifstricteq_cc (") << ((cond)?_("taken)"):_("not taken)")));
	if(cond)
		context->exec_pos += t+1;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifstricteq_local_constant(call_context* context)
{
	//ifstricteq
	int32_t t = (*context->exec_pos).jumpdata.jump;
	bool cond=context->locals[context->exec_pos->local_pos1].isEqualStrict(context->mi->context->root->getSystemState(),*context->exec_pos->arg2_constant);
	LOG_CALL(_("ifstricteq_lc (") << ((cond)?_("taken)"):_("not taken)")));
	if(cond)
		context->exec_pos += t+1;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifstricteq_constant_local(call_context* context)
{
	//ifstricteq
	int32_t t = (*context->exec_pos).jumpdata.jump;
	bool cond=context->exec_pos->arg1_constant->isEqualStrict(context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos2]);
	LOG_CALL(_("ifstricteq_cl (") << ((cond)?_("taken)"):_("not taken)")));
	if(cond)
		context->exec_pos += t+1;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifstricteq_local_local(call_context* context)
{
	//ifstricteq
	int32_t t = (*context->exec_pos).jumpdata.jump;
	bool cond=context->locals[context->exec_pos->local_pos1].isEqualStrict(context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos2]);
	LOG_CALL(_("ifstricteq_ll (") << ((cond)?_("taken)"):_("not taken)")));
	if(cond)
		context->exec_pos += t+1;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifstrictne(call_context* context)
{
	//ifstrictne
	int32_t t = (*context->exec_pos).jumpdata.jump;

	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POP_CREATE(context,v2);

	bool cond=!v1->isEqualStrict(context->mi->context->root->getSystemState(),*v2);
	LOG_CALL(_("ifStrictNE ")<<cond <<" "<<v1->toDebugString()<<" "<<v2->toDebugString());
	ASATOM_DECREF_POINTER(v1);
	ASATOM_DECREF_POINTER(v2);
	if(cond)
		context->exec_pos += t+1;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifstrictne_constant_constant(call_context* context)
{
	//ifstrictne
	int32_t t = (*context->exec_pos).jumpdata.jump;
	bool cond=!context->exec_pos->arg1_constant->isEqualStrict(context->mi->context->root->getSystemState(),*context->exec_pos->arg2_constant);
	LOG_CALL(_("ifstrictne_cc (") << ((cond)?_("taken)"):_("not taken)")));
	if(cond)
		context->exec_pos += t+1;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifstrictne_local_constant(call_context* context)
{
	//ifstrictne
	int32_t t = (*context->exec_pos).jumpdata.jump;
	bool cond=!context->locals[context->exec_pos->local_pos1].isEqualStrict(context->mi->context->root->getSystemState(),*context->exec_pos->arg2_constant);
	LOG_CALL(_("ifstrictne_lc (") << ((cond)?_("taken)"):_("not taken)")));
	if(cond)
		context->exec_pos += t+1;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifstrictne_constant_local(call_context* context)
{
	//ifstrictne
	int32_t t = (*context->exec_pos).jumpdata.jump;
	bool cond=!context->exec_pos->arg1_constant->isEqualStrict(context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos2]);
	LOG_CALL(_("ifstrictne_cl (") << ((cond)?_("taken)"):_("not taken)")));
	if(cond)
		context->exec_pos += t+1;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifstrictne_local_local(call_context* context)
{
	//ifstrictne
	int32_t t = (*context->exec_pos).jumpdata.jump;
	bool cond=!context->locals[context->exec_pos->local_pos1].isEqualStrict(context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos2]);
	LOG_CALL(_("ifstrictne_ll (") << ((cond)?_("taken)"):_("not taken)")));
	if(cond)
		context->exec_pos += t+1;
	else
		++(context->exec_pos);
}
void ABCVm::abc_lookupswitch(call_context* context)
{
	//lookupswitch
	preloadedcodedata* here=context->exec_pos; //Base for the jumps is the instruction itself for the switch
	int32_t t = (++(context->exec_pos))->idata;
	preloadedcodedata* defaultdest=here+t;
	LOG_CALL(_("Switch default dest ") << t);
	uint32_t count = (++(context->exec_pos))->data;
	preloadedcodedata* offsets=g_newa(preloadedcodedata, count+1);
	for(unsigned int i=0;i<count+1;i++)
	{
		offsets[i] = *(++(context->exec_pos));
		LOG_CALL(_("Switch dest ") << i << ' ' << offsets[i].idata);
	}

	RUNTIME_STACK_POP_CREATE(context,index_obj);
	assert_and_throw(index_obj->isNumeric());
	unsigned int index=index_obj->toUInt();
	ASATOM_DECREF_POINTER(index_obj);

	preloadedcodedata* dest=defaultdest;
	if(index<=count)
		dest=here+offsets[index].idata;

	context->exec_pos = dest;
}
void ABCVm::abc_pushwith(call_context* context)
{
	//pushwith
	pushWith(context);
	++(context->exec_pos);
}
void ABCVm::abc_popscope(call_context* context)
{
	//popscope
	popScope(context);
	++(context->exec_pos);
}
void ABCVm::abc_nextname(call_context* context)
{
	//nextname
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	LOG_CALL("nextName");
	if(v1->type!=T_UINTEGER)
		throw UnsupportedException("Type mismatch in nextName");

	asAtom ret;
	pval->toObject(context->mi->context->root->getSystemState())->nextName(ret,v1->toUInt());
	ASATOM_DECREF_POINTER(pval);
	ASATOM_DECREF_POINTER(v1);
	*pval = ret;
	++(context->exec_pos);
}
void ABCVm::abc_hasnext(call_context* context)
{
	//hasnext
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	LOG_CALL("hasNext " << v1->toDebugString() << ' ' << pval->toDebugString());

	uint32_t curIndex=pval->toUInt();

	uint32_t newIndex=v1->toObject(context->mi->context->root->getSystemState())->nextNameIndex(curIndex);
	ASATOM_DECREF_POINTER(pval);
	ASATOM_DECREF_POINTER(v1);
	pval->setInt(newIndex);
	++(context->exec_pos);
}
void ABCVm::abc_pushnull(call_context* context)
{
	//pushnull
	LOG_CALL("pushnull");
	RUNTIME_STACK_PUSH(context,asAtom::nullAtom);
	++(context->exec_pos);
}
void ABCVm::abc_pushundefined(call_context* context)
{
	LOG_CALL("pushundefined");
	RUNTIME_STACK_PUSH(context,asAtom::undefinedAtom);
	++(context->exec_pos);
}
void ABCVm::abc_nextvalue(call_context* context)
{
	//nextvalue
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	LOG_CALL("nextvalue:"<<v1->toDebugString()<<" "<< pval->toDebugString());
	if(v1->type!=T_UINTEGER)
		throw UnsupportedException("Type mismatch in nextValue");

	asAtom ret;
	pval->toObject(context->mi->context->root->getSystemState())->nextValue(ret,v1->toUInt());
	ASATOM_DECREF_POINTER(pval);
	ASATOM_DECREF_POINTER(v1);
	*pval=ret;
	++(context->exec_pos);
}
void ABCVm::abc_pushbyte(call_context* context)
{
	//pushbyte
	int32_t t = (++(context->exec_pos))->idata;
	LOG_CALL("pushbyte "<<(int)t);
	RUNTIME_STACK_PUSH(context,asAtom(t));
	++(context->exec_pos);
}
void ABCVm::abc_pushshort(call_context* context)
{
	//pushshort
	// specs say pushshort is a u30, but it's really a u32
	// see https://bugs.adobe.com/jira/browse/ASC-4181
	int32_t t = (++(context->exec_pos))->idata;
	LOG_CALL("pushshort "<<t);

	RUNTIME_STACK_PUSH(context,asAtom(t));
	++(context->exec_pos);
}
void ABCVm::abc_pushtrue(call_context* context)
{
	//pushtrue
	LOG_CALL("pushtrue");
	RUNTIME_STACK_PUSH(context,asAtom::trueAtom);
	++(context->exec_pos);
}
void ABCVm::abc_pushfalse(call_context* context)
{
	//pushfalse
	LOG_CALL("pushfalse");
	RUNTIME_STACK_PUSH(context,asAtom::falseAtom);
	++(context->exec_pos);
}
void ABCVm::abc_pushnan(call_context* context)
{
	//pushnan
	LOG_CALL("pushNaN");
	RUNTIME_STACK_PUSH(context,asAtom(Number::NaN));
	++(context->exec_pos);
}
void ABCVm::abc_pop(call_context* context)
{
	//pop
	RUNTIME_STACK_POP_CREATE(context,o);
	LOG_CALL("pop "<<o->toDebugString());
	ASATOM_DECREF_POINTER(o);
	++(context->exec_pos);
}
void ABCVm::abc_dup(call_context* context)
{
	//dup
	dup();
	RUNTIME_STACK_PEEK_CREATE(context,o);
	ASATOM_INCREF_POINTER(o);
	RUNTIME_STACK_PUSH(context,*o);
	++(context->exec_pos);
}
void ABCVm::abc_swap(call_context* context)
{
	//swap
	swap();
	asAtom v1;
	RUNTIME_STACK_POP(context,v1);
	asAtom v2;
	RUNTIME_STACK_POP(context,v2);

	RUNTIME_STACK_PUSH(context,v1);
	RUNTIME_STACK_PUSH(context,v2);
	++(context->exec_pos);
}
void ABCVm::abc_pushstring(call_context* context)
{
	//pushstring
	uint32_t t = (++(context->exec_pos))->data;
	LOG_CALL( _("pushString ") << context->mi->context->root->getSystemState()->getStringFromUniqueId(context->mi->context->getString(t)) );
	RUNTIME_STACK_PUSH(context,asAtom::fromStringID(context->mi->context->getString(t)));
	++(context->exec_pos);
}
void ABCVm::abc_pushint(call_context* context)
{
	//pushint
	uint32_t t = (++(context->exec_pos))->data;
	s32 val=context->mi->context->constant_pool.integer[t];
	LOG_CALL( "pushInt " << val);

	RUNTIME_STACK_PUSH(context,asAtom(val));
	++(context->exec_pos);
}
void ABCVm::abc_pushuint(call_context* context)
{
	//pushuint
	uint32_t t = (++(context->exec_pos))->data;
	u32 val=context->mi->context->constant_pool.uinteger[t];
	LOG_CALL( "pushUInt " << val);

	RUNTIME_STACK_PUSH(context,asAtom(val));
	++(context->exec_pos);
}
void ABCVm::abc_pushdouble(call_context* context)
{
	//pushdouble
	uint32_t t = (++(context->exec_pos))->data;
	d64 val=context->mi->context->constant_pool.doubles[t];
	LOG_CALL( "pushDouble " << val);

	RUNTIME_STACK_PUSH(context,asAtom(val));
	++(context->exec_pos);
}
void ABCVm::abc_pushScope(call_context* context)
{
	//pushscope
	pushScope(context);
	++(context->exec_pos);
}
void ABCVm::abc_pushScope_constant(call_context* context)
{
	//pushscope
	asAtom* t = context->exec_pos->arg1_constant;
	LOG_CALL( _("pushScope_c ") << t->toDebugString() );
	assert_and_throw(context->curr_scope_stack < context->max_scope_stack);
	ASATOM_INCREF_POINTER(t);
	context->scope_stack[context->curr_scope_stack] = *t;
	context->scope_stack_dynamic[context->curr_scope_stack] = false;
	context->curr_scope_stack++;
	++(context->exec_pos);
}
void ABCVm::abc_pushScope_local(call_context* context)
{
	//pushscope
	asAtom* t = &context->locals[context->exec_pos->local_pos1];
	LOG_CALL( _("pushScope_l ") << t->toDebugString() );
	assert_and_throw(context->curr_scope_stack < context->max_scope_stack);
	ASATOM_INCREF_POINTER(t);
	context->scope_stack[context->curr_scope_stack] = *t;
	context->scope_stack_dynamic[context->curr_scope_stack] = false;
	context->curr_scope_stack++;
	++(context->exec_pos);
}
void ABCVm::abc_pushnamespace(call_context* context)
{
	//pushnamespace
	uint32_t t = (++(context->exec_pos))->data;
	RUNTIME_STACK_PUSH(context,asAtom::fromObject(pushNamespace(context, t) ));
	++(context->exec_pos);
}
void ABCVm::abc_hasnext2(call_context* context)
{
	//hasnext2
	uint32_t t = (++(context->exec_pos))->data;
	uint32_t t2 = (++(context->exec_pos))->data;

	bool ret=hasNext2(context,t,t2);
	RUNTIME_STACK_PUSH(context,asAtom(ret));
	++(context->exec_pos);
}
//Alchemy opcodes
void ABCVm::abc_li8(call_context* context)
{
	//li8
	LOG_CALL( "li8");
	loadIntN<uint8_t>(context);
	++(context->exec_pos);
}
void ABCVm::abc_li16(call_context* context)
{
	//li16
	LOG_CALL( "li16");
	loadIntN<uint16_t>(context);
	++(context->exec_pos);
}
void ABCVm::abc_li32(call_context* context)
{
	//li32
	LOG_CALL( "li32");
	loadIntN<int32_t>(context);
	++(context->exec_pos);
}
void ABCVm::abc_lf32(call_context* context)
{
	//lf32
	LOG_CALL( "lf32");
	loadFloat(context);
	++(context->exec_pos);
}
void ABCVm::abc_lf64(call_context* context)
{
	//lf64
	LOG_CALL( "lf64");
	loadDouble(context);
	++(context->exec_pos);
}
void ABCVm::abc_si8(call_context* context)
{
	//si8
	LOG_CALL( "si8");
	storeIntN<uint8_t>(context);
	++(context->exec_pos);
}
void ABCVm::abc_si16(call_context* context)
{
	//si16
	LOG_CALL( "si16");
	storeIntN<uint16_t>(context);
	++(context->exec_pos);
}
void ABCVm::abc_si32(call_context* context)
{
	//si32
	LOG_CALL( "si32");
	storeIntN<uint32_t>(context);
	++(context->exec_pos);
}
void ABCVm::abc_sf32(call_context* context)
{
	//sf32
	LOG_CALL( "sf32");
	storeFloat(context);
	++(context->exec_pos);
}
void ABCVm::abc_sf64(call_context* context)
{
	//sf64
	LOG_CALL( "sf64");
	storeDouble(context);
	++(context->exec_pos);
}
void ABCVm::abc_newfunction(call_context* context)
{
	//newfunction
	uint32_t t = (++(context->exec_pos))->data;
	RUNTIME_STACK_PUSH(context,asAtom::fromObject(newFunction(context,t)));
	++(context->exec_pos);
}
void ABCVm::abc_call(call_context* context)
{
	//call
	uint32_t t = (++(context->exec_pos))->data;
	method_info* called_mi=NULL;
	call(context,t,&called_mi);
	++(context->exec_pos);
}
void ABCVm::abc_construct(call_context* context)
{
	//construct
	uint32_t t = (++(context->exec_pos))->data;
	construct(context,t);
	++(context->exec_pos);
}
void ABCVm::abc_callMethod(call_context* context)
{
	// callmethod
	uint32_t t = (++(context->exec_pos))->data;
	uint32_t t2 = (++(context->exec_pos))->data;
	callMethod(context,t,t2);
	++(context->exec_pos);
}
void ABCVm::abc_callstatic(call_context* context)
{
	//callstatic
	uint32_t t = (++(context->exec_pos))->data;
	uint32_t t2 = (++(context->exec_pos))->data;
	callStatic(context,t,t2,NULL,true);
	++(context->exec_pos);
}
void ABCVm::abc_callsuper(call_context* context)
{
	//callsuper
	uint32_t t = (++(context->exec_pos))->data;
	uint32_t t2 = (++(context->exec_pos))->data;
	callSuper(context,t,t2,NULL,true);
	++(context->exec_pos);
}
void ABCVm::abc_callproperty(call_context* context)
{
	//callproperty
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t t = (++(context->exec_pos))->data;
	uint32_t t2 = (++(context->exec_pos))->data;
	callPropIntern(context,t,t2,true,false,instrptr);
	++(context->exec_pos);
}

void callpropOneArg(call_context* context,asAtom& ret,asAtom& obj,asAtom* args,multiname* name,preloadedcodedata* cacheptr)
{
	if ((cacheptr->data&ABC_OP_CACHED) == ABC_OP_CACHED)
	{
		if (obj.getObject() && 
				((obj.is<Class_base>() && obj.getObject() == cacheptr->cacheobj1)
				|| obj.getObject()->getClass() == cacheptr->cacheobj1))
		{
			asAtom o = asAtom::fromObject(cacheptr->cacheobj3);
			ASATOM_INCREF(o);
			LOG_CALL( "callProperty from cache:"<<*name<<" "<<obj.toDebugString()<<" "<<o.toDebugString());
			if(o.is<IFunction>())
				o.callFunction(ret,obj,args,1,false);
			else if(o.is<Class_base>())
				o.as<Class_base>()->generator(ret,args,1);
			else if(o.is<RegExp>())
				RegExp::exec(ret,context->mi->context->root->getSystemState(),o,args,1);
			else
			{
				LOG(LOG_ERROR,"trying to call an object as a function:"<<o.toDebugString() <<" on "<<obj.toDebugString());
				throwError<TypeError>(kCallOfNonFunctionError, "Object");
			}
			LOG_CALL("End of calling cached property "<<*name);
			return;
		}
		else
			cacheptr->data = ABC_OP_NOTCACHEABLE;
	}
	if(obj.is<Null>())
	{
		LOG(LOG_ERROR,"trying to call property on null:"<<*name);
		throwError<TypeError>(kConvertNullToObjectError);
	}
	if (obj.is<Undefined>())
	{
		LOG(LOG_ERROR,"trying to call property on undefined:"<<*name);
		throwError<TypeError>(kConvertUndefinedToObjectError);
	}
	ASObject* pobj = obj.getObject();
	asAtom o;
	bool canCache = false;
	if (!pobj)
	{
		// fast path for primitives to avoid creation of ASObjects
		obj.getVariableByMultiname(o,context->mi->context->root->getSystemState(),*name);
		canCache = o.type != T_INVALID;
	}
	if(o.type == T_INVALID)
	{
		pobj = obj.toObject(context->mi->context->root->getSystemState());
		//We should skip the special implementation of get
		canCache = pobj->getVariableByMultiname(o,*name, ASObject::SKIP_IMPL) & GET_VARIABLE_RESULT::GETVAR_CACHEABLE;
	}
	name->resetNameIfObject();
	if(o.type == T_INVALID && obj.is<Class_base>())
	{
		// check super classes
		_NR<Class_base> tmpcls = obj.as<Class_base>()->super;
		while (tmpcls && !tmpcls.isNull())
		{
			tmpcls->getVariableByMultiname(o,*name, ASObject::SKIP_IMPL);
			if(o.type != T_INVALID)
			{
				canCache = true;
				break;
			}
			tmpcls = tmpcls->super;
		}
	}
	if(o.type != T_INVALID && !obj.is<Proxy>())
	{
		if(o.is<IFunction>())
		{
			if (canCache 
					&& (cacheptr->data & ABC_OP_NOTCACHEABLE)==0 
					&& obj.canCacheMethod(name) 
					&& o.getObject() 
					&& (obj.is<Class_base>() || o.as<IFunction>()->inClass == obj.getClass(context->mi->context->root->getSystemState())))
			{
				// cache method if multiname is static and it is a method of a sealed class
				cacheptr->data |= ABC_OP_CACHED;
				cacheptr->cacheobj1 = obj.getClass(context->mi->context->root->getSystemState());
				cacheptr->cacheobj3 = o.getObject();
				LOG_CALL("caching callproperty:"<<*name<<" "<<cacheptr->cacheobj1->toDebugString()<<" "<<cacheptr->cacheobj3->toDebugString());
			}
			else
				cacheptr->data = ABC_OP_NOTCACHEABLE;
			o.callFunction(ret,obj,args,1,false);
		}
		else if(o.is<Class_base>())
			o.as<Class_base>()->generator(ret,args,1);
		else if(o.is<RegExp>())
			RegExp::exec(ret,context->mi->context->root->getSystemState(),o,args,1);
		else
		{
			LOG(LOG_ERROR,"trying to call an object as a function:"<<o.toDebugString() <<" on "<<obj.toDebugString());
			throwError<TypeError>(kCallOfNonFunctionError, "Object");
		}
	}
	else
	{
		//If the object is a Proxy subclass, try to invoke callProperty
		if(obj.is<Proxy>())
		{
			//Check if there is a custom caller defined, skipping implementation to avoid recursive calls
			multiname callPropertyName(NULL);
			callPropertyName.name_type=multiname::NAME_STRING;
			callPropertyName.name_s_id=context->mi->context->root->getSystemState()->getUniqueStringId("callProperty");
			callPropertyName.ns.emplace_back(context->mi->context->root->getSystemState(),flash_proxy,NAMESPACE);
			asAtom oproxy;
			pobj->getVariableByMultiname(oproxy,callPropertyName,ASObject::SKIP_IMPL);
			if(oproxy.type != T_INVALID)
			{
				assert_and_throw(oproxy.type==T_FUNCTION);
				if(o.type != T_INVALID)
				{
					if(o.is<IFunction>())
						o.callFunction(ret,obj,args,1,false);
					else if(o.is<Class_base>())
						o.as<Class_base>()->generator(ret,args,1);
					else if(o.is<RegExp>())
						RegExp::exec(ret,context->mi->context->root->getSystemState(),o,args,1);
					else
					{
						LOG(LOG_ERROR,"trying to call an object as a function:"<<o.toDebugString() <<" on "<<obj.toDebugString());
						throwError<TypeError>(kCallOfNonFunctionError, "Object");
					}
				}
				else
				{
					//Create a new array
					asAtom* proxyArgs=g_newa(asAtom, 2);
					ASObject* namearg = abstract_s(context->mi->context->root->getSystemState(),name->normalizedName(context->mi->context->root->getSystemState()));
					namearg->setProxyProperty(*name);
					proxyArgs[0]=asAtom::fromObject(namearg);
					proxyArgs[1]=*args;
					
					//We now suppress special handling
					LOG_CALL(_("Proxy::callProperty"));
					ASATOM_INCREF(oproxy);
					asAtom ret;
					oproxy.callFunction(ret,obj,proxyArgs,2,true);
					ASATOM_DECREF(oproxy);
				}
				LOG_CALL(_("End of calling proxy custom caller ") << *name);
			}
			else if(o.type != T_INVALID)
			{
				if(o.is<IFunction>())
					o.callFunction(ret,obj,args,1,false);
				else if(o.is<Class_base>())
					o.as<Class_base>()->generator(ret,args,1);
				else if(o.is<RegExp>())
					RegExp::exec(ret,context->mi->context->root->getSystemState(),o,args,1);
				else
				{
					LOG(LOG_ERROR,"trying to call an object as a function:"<<o.toDebugString() <<" on "<<obj.toDebugString());
					throwError<TypeError>(kCallOfNonFunctionError, "Object");
				}
				LOG_CALL(_("End of calling proxy ") << *name);
			}
		}
		if (ret.type == T_INVALID)
		{
			if (pobj->hasPropertyByMultiname(*name,true,true))
			{
				tiny_string clsname = pobj->getClass()->getQualifiedClassName();
				throwError<ReferenceError>(kWriteOnlyError, name->normalizedName(context->mi->context->root->getSystemState()), clsname);
			}
			if (pobj->getClass() && pobj->getClass()->isSealed)
			{
				tiny_string clsname = pobj->getClass()->getQualifiedClassName();
				throwError<ReferenceError>(kReadSealedError, name->normalizedName(context->mi->context->root->getSystemState()), clsname);
			}
			if (obj.is<Class_base>())
			{
				tiny_string clsname = obj.as<Class_base>()->class_name.getQualifiedName(context->mi->context->root->getSystemState());
				throwError<TypeError>(kCallNotFoundError, name->qualifiedString(context->mi->context->root->getSystemState()), clsname);
			}
			else
			{
				tiny_string clsname = pobj->getClassName();
				throwError<TypeError>(kCallNotFoundError, name->qualifiedString(context->mi->context->root->getSystemState()), clsname);
			}
			ret.setUndefined();
		}
	}
	LOG_CALL(_("End of calling ") << *name);
}
void ABCVm::abc_callpropertyStaticName_localresult(call_context* context)
{
	//callproperty
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));

	RUNTIME_STACK_POP_CREATE(context,args);
	multiname* name=context->exec_pos->cachedmultiname2;

	LOG_CALL( "callProperty_l " << *name);
	RUNTIME_STACK_POP_CREATE(context,obj);
	callpropOneArg(context,context->locals[instrptr->local_pos3-1],*obj,args,name,context->exec_pos);

	++(context->exec_pos);
}

void ABCVm::abc_callpropertyStaticName_constant_constant(call_context* context)
{
	//callproperty
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));

	asAtom* args=instrptr->arg2_constant;
	multiname* name=context->exec_pos->cachedmultiname2;

	asAtom obj= *instrptr->arg1_constant;
	LOG_CALL( "callProperty_cc " << *name);
	asAtom ret;
	callpropOneArg(context,ret,obj,args,name,context->exec_pos);
	RUNTIME_STACK_PUSH(context,ret);

	++(context->exec_pos);
}
void ABCVm::abc_callpropertyStaticName_local_constant(call_context* context)
{
	//callproperty
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));

	asAtom* args=instrptr->arg2_constant;
	multiname* name=context->exec_pos->cachedmultiname2;

	asAtom obj= context->locals[instrptr->local_pos1];
	LOG_CALL( "callProperty_lc " << *name);
	asAtom ret;
	callpropOneArg(context,ret,obj,args,name,context->exec_pos);
	RUNTIME_STACK_PUSH(context,ret);

	++(context->exec_pos);
}
void ABCVm::abc_callpropertyStaticName_constant_local(call_context* context)
{
	//callproperty
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));

	asAtom* args=&context->locals[instrptr->local_pos2];
	multiname* name=context->exec_pos->cachedmultiname2;

	asAtom obj= *instrptr->arg1_constant;
	LOG_CALL( "callProperty_cl " << *name);
	asAtom ret;
	callpropOneArg(context,ret,obj,args,name,context->exec_pos);
	RUNTIME_STACK_PUSH(context,ret);

	++(context->exec_pos);
}
void ABCVm::abc_callpropertyStaticName_local_local(call_context* context)
{
	//callproperty
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));

	asAtom* args=&context->locals[instrptr->local_pos2];
	multiname* name=context->exec_pos->cachedmultiname2;

	asAtom obj= context->locals[instrptr->local_pos1];
	LOG_CALL( "callProperty_ll " << *name);
	asAtom ret;
	callpropOneArg(context,ret,obj,args,name,context->exec_pos);
	RUNTIME_STACK_PUSH(context,ret);

	++(context->exec_pos);
}
void ABCVm::abc_callpropertyStaticName_constant_constant_localresult(call_context* context)
{
	//callproperty
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));

	asAtom* args=instrptr->arg2_constant;
	multiname* name=context->exec_pos->cachedmultiname2;

	asAtom obj= *instrptr->arg1_constant;
	LOG_CALL( "callProperty_ccl " << *name);
	callpropOneArg(context,context->locals[instrptr->local_pos3-1],obj,args,name,context->exec_pos);
	++(context->exec_pos);
}
void ABCVm::abc_callpropertyStaticName_local_constant_localresult(call_context* context)
{
	//callproperty
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));

	asAtom* args=instrptr->arg2_constant;
	multiname* name=context->exec_pos->cachedmultiname2;

	asAtom obj= context->locals[instrptr->local_pos1];
	LOG_CALL( "callProperty_lcl " << *name);
	callpropOneArg(context,context->locals[instrptr->local_pos3-1],obj,args,name,context->exec_pos);

	++(context->exec_pos);
}
void ABCVm::abc_callpropertyStaticName_constant_local_localresult(call_context* context)
{
	//callproperty
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));

	asAtom* args=&context->locals[instrptr->local_pos2];
	multiname* name=context->exec_pos->cachedmultiname2;

	asAtom obj= *instrptr->arg1_constant;
	LOG_CALL( "callProperty_cll " << *name);
	callpropOneArg(context,context->locals[instrptr->local_pos3-1],obj,args,name,context->exec_pos);

	++(context->exec_pos);
}
void ABCVm::abc_callpropertyStaticName_local_local_localresult(call_context* context)
{
	//callproperty
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));

	asAtom* args=&context->locals[instrptr->local_pos2];
	multiname* name=context->exec_pos->cachedmultiname2;

	asAtom obj= context->locals[instrptr->local_pos1];
	LOG_CALL( "callProperty_lll " << *name);
	callpropOneArg(context,context->locals[instrptr->local_pos3-1],obj,args,name,context->exec_pos);

	++(context->exec_pos);
}
void ABCVm::abc_returnvoid(call_context* context)
{
	//returnvoid
	LOG_CALL(_("returnVoid"));
	context->returnvalue = asAtom::invalidAtom;
	context->returning = true;
	++(context->exec_pos);
}
void ABCVm::abc_returnvalue(call_context* context)
{
	//returnvalue
	RUNTIME_STACK_POP(context,context->returnvalue);
	LOG_CALL(_("returnValue ") << context->returnvalue.toDebugString());
	context->returning = true;
	++(context->exec_pos);
}
void ABCVm::abc_returnvalue_constant(call_context* context)
{
	//returnvalue
	context->returnvalue.set(*context->exec_pos->arg1_constant);
	LOG_CALL(_("returnValue_c ") << context->returnvalue.toDebugString());
	context->returning = true;
	++(context->exec_pos);
}
void ABCVm::abc_returnvalue_local(call_context* context)
{
	//returnvalue
	context->returnvalue.set(context->locals[context->exec_pos->local_pos1]);
	LOG_CALL(_("returnValue_l ") << context->returnvalue.toDebugString());
	ASATOM_INCREF(context->locals[context->exec_pos->local_pos1]);
	context->returning = true;
	++(context->exec_pos);
}
void ABCVm::abc_constructsuper(call_context* context)
{
	//constructsuper
	uint32_t t = (++(context->exec_pos))->data;
	constructSuper(context,t);
	++(context->exec_pos);
}
void ABCVm::abc_constructprop(call_context* context)
{
	//constructprop
	uint32_t t = (++(context->exec_pos))->data;
	uint32_t t2 = (++(context->exec_pos))->data;
	constructProp(context,t,t2);
	++(context->exec_pos);
}
void ABCVm::abc_callproplex(call_context* context)
{
	//callproplex
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t t = (++(context->exec_pos))->data;
	uint32_t t2 = (++(context->exec_pos))->data;
	callPropIntern(context,t,t2,true,true,instrptr);
	++(context->exec_pos);
}
void ABCVm::abc_callsupervoid(call_context* context)
{
	//callsupervoid
	uint32_t t = (++(context->exec_pos))->data;
	uint32_t t2 = (++(context->exec_pos))->data;
	method_info* called_mi=NULL;
	callSuper(context,t,t2,&called_mi,false);
	++(context->exec_pos);
}
void ABCVm::abc_callpropvoid(call_context* context)
{
	//callpropvoid
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t t = (++(context->exec_pos))->data;
	uint32_t t2 = (++(context->exec_pos))->data;
	callPropIntern(context,t,t2,false,false,instrptr);
	++(context->exec_pos);
}
void ABCVm::abc_sxi1(call_context* context)
{
	//sxi1
	LOG_CALL( "sxi1");
	RUNTIME_STACK_POINTER_CREATE(context,arg1);
	int32_t ret=arg1->toUInt()&0x1 ? -1 : 0;
	ASATOM_DECREF_POINTER(arg1);
	arg1->setInt(ret);
	++(context->exec_pos);
}
void ABCVm::abc_sxi8(call_context* context)
{
	//sxi8
	LOG_CALL( "sxi8");
	RUNTIME_STACK_POINTER_CREATE(context,arg1);
	int32_t ret=(int8_t)arg1->toUInt();
	ASATOM_DECREF_POINTER(arg1);
	arg1->setInt(ret);
	++(context->exec_pos);
}
void ABCVm::abc_sxi16(call_context* context)
{
	//sxi16
	LOG_CALL( "sxi16");
	RUNTIME_STACK_POINTER_CREATE(context,arg1);
	int32_t ret=(int16_t)arg1->toUInt();
	ASATOM_DECREF_POINTER(arg1);
	arg1->setInt(ret);
	++(context->exec_pos);
}
void ABCVm::abc_constructgenerictype(call_context* context)
{
	//constructgenerictype
	uint32_t t = (++(context->exec_pos))->data;
	constructGenericType(context, t);
	++(context->exec_pos);
}
void ABCVm::abc_newobject(call_context* context)
{
	//newobject
	uint32_t t = (++(context->exec_pos))->data;
	newObject(context,t);
	++(context->exec_pos);
}
void ABCVm::abc_newarray(call_context* context)
{
	//newarray
	uint32_t t = (++(context->exec_pos))->data;
	newArray(context,t);
	++(context->exec_pos);
}
void ABCVm::abc_newactivation(call_context* context)
{
	//newactivation
	RUNTIME_STACK_PUSH(context,asAtom::fromObject(newActivation(context, context->mi)));
	++(context->exec_pos);
}
void ABCVm::abc_newclass(call_context* context)
{
	//newclass
	uint32_t t = (++(context->exec_pos))->data;
	newClass(context,t);
	++(context->exec_pos);
}
void ABCVm::abc_getdescendants(call_context* context)
{
	//getdescendants
	uint32_t t = (++(context->exec_pos))->data;
	getDescendants(context, t);
	++(context->exec_pos);
}
void ABCVm::abc_newcatch(call_context* context)
{
	//newcatch
	uint32_t t = (++(context->exec_pos))->data;
	RUNTIME_STACK_PUSH(context,asAtom::fromObject(newCatch(context,t)));
	++(context->exec_pos);
}
void ABCVm::abc_findpropstrict(call_context* context)
{
	//findpropstrict
//		uint32_t t = (++(context->exec_pos))->data;
//		multiname* name=context->mi->context->getMultiname(t,context);
//		RUNTIME_STACK_PUSH(context,asAtom::fromObject(findPropStrict(context,name)));
//		name->resetNameIfObject();

	asAtom o;
	findPropStrictCache(o,context);
	RUNTIME_STACK_PUSH(context,o);
	++(context->exec_pos);
}
void ABCVm::abc_findproperty(call_context* context)
{
	//findproperty
	uint32_t t = (++(context->exec_pos))->data;
	multiname* name=context->mi->context->getMultiname(t,context);
	RUNTIME_STACK_PUSH(context,asAtom::fromObject(findProperty(context,name)));
	name->resetNameIfObject();
	++(context->exec_pos);
}
void ABCVm::abc_finddef(call_context* context)
{
	//finddef
	uint32_t t = (++(context->exec_pos))->data;
	multiname* name=context->mi->context->getMultiname(t,context);
	LOG(LOG_NOT_IMPLEMENTED,"opcode 0x5f (finddef) not implemented:"<<*name);
	RUNTIME_STACK_PUSH(context,asAtom::nullAtom);
	name->resetNameIfObject();
	++(context->exec_pos);
}
void ABCVm::abc_getlex(call_context* context)
{
	//getlex
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t t = (++(context->exec_pos))->data;
	if ((instrptr->data&ABC_OP_CACHED) == ABC_OP_CACHED)
	{
		RUNTIME_STACK_PUSH(context,asAtom::fromFunction(instrptr->cacheobj1,instrptr->cacheobj2));
		instrptr->cacheobj1->incRef();
		LOG_CALL( "getLex from cache: " <<  instrptr->cacheobj1->toDebugString());
	}
	else if (getLex(context,t))
	{
		// put object in cache
		instrptr->data |= ABC_OP_CACHED;
		RUNTIME_STACK_PEEK_CREATE(context,v);
		
		instrptr->cacheobj1 = v->getObject();
		instrptr->cacheobj2 = v->getClosure();
	}
	++(context->exec_pos);
}
void ABCVm::abc_getlex_localresult(call_context* context)
{
	//getlex
	preloadedcodedata* instrptr = context->exec_pos;
	assert(instrptr->local_pos3 > 0);
	uint32_t t = (++(context->exec_pos))->data;
	if ((instrptr->data&ABC_OP_CACHED) == ABC_OP_CACHED)
	{
		context->locals[instrptr->local_pos3-1].setFunction(instrptr->cacheobj1,instrptr->cacheobj2);
		LOG_CALL( "getLex_l from cache: " <<  instrptr->cacheobj1->toDebugString());
	}
	else if (getLex(context,t,instrptr->local_pos3))
	{
		// put object in cache
		instrptr->data |= ABC_OP_CACHED;
		instrptr->cacheobj1 = context->locals[instrptr->local_pos3-1].getObject();
		instrptr->cacheobj2 = context->locals[instrptr->local_pos3-1].getClosure();
	}
	++(context->exec_pos);
}
void ABCVm::abc_setproperty(call_context* context)
{
	//setproperty
	uint32_t t = (++(context->exec_pos))->data;
	RUNTIME_STACK_POP_CREATE(context,value);

	multiname* name=context->mi->context->getMultiname(t,context);

	RUNTIME_STACK_POP_CREATE(context,obj)

	LOG_CALL(_("setProperty ") << *name << ' ' << obj->toDebugString()<<" " <<value->toDebugString());

	if(obj->type == T_NULL)
	{
		LOG(LOG_ERROR,"calling setProperty on null:" << *name << ' ' << obj->toDebugString()<<" " << value->toDebugString());
		ASATOM_DECREF_POINTER(obj);
		ASATOM_DECREF_POINTER(value);
		throwError<TypeError>(kConvertNullToObjectError);
	}
	if (obj->type == T_UNDEFINED)
	{
		LOG(LOG_ERROR,"calling setProperty on undefined:" << *name << ' ' << obj->toDebugString()<<" " << value->toDebugString());
		ASATOM_DECREF_POINTER(obj);
		ASATOM_DECREF_POINTER(value);
		throwError<TypeError>(kConvertUndefinedToObjectError);
	}
	//Do not allow to set contant traits
	ASObject* o = obj->toObject(context->mi->context->root->getSystemState());
	o->setVariableByMultiname(*name,*value,ASObject::CONST_NOT_ALLOWED);
	o->decRef();
	name->resetNameIfObject();
	++(context->exec_pos);
}
void ABCVm::abc_getlocal(call_context* context)
{
	//getlocal
	uint32_t i = ((context->exec_pos)++)->data>>9;
	LOG_CALL( _("getLocal n ") << i << _(": ") << context->locals[i].toDebugString() );
	ASATOM_INCREF(context->locals[i]);
	RUNTIME_STACK_PUSH(context,context->locals[i]);
}
void ABCVm::abc_setlocal(call_context* context)
{
	//setlocal
	uint32_t i = ((context->exec_pos)++)->data>>9;
	RUNTIME_STACK_POP_CREATE(context,obj)

	LOG_CALL( _("setLocal n ") << i << _(": ") << obj->toDebugString() );
	if (i >= context->locals_size)
	{
		LOG(LOG_ERROR,"abc_setlocal invalid index:"<<i);
		return;
	}
	if ((int)i != context->argarrayposition || obj->type == T_ARRAY)
	{
		if (context->locals[i].getObject() != obj->getObject())
		{
			ASATOM_DECREF(context->locals[i]);
		}
		context->locals[i]=*obj;
	}
}
void ABCVm::abc_getglobalscope(call_context* context)
{
	//getglobalscope
	asAtom ret = asAtom::fromObject(getGlobalScope(context));
	ASATOM_INCREF(ret);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_getscopeobject(call_context* context)
{
	//getscopeobject
	uint32_t t = (++(context->exec_pos))->data;
	assert_and_throw(context->curr_scope_stack > (uint32_t)t);
	asAtom ret=context->scope_stack[t];
	ASATOM_INCREF(ret);
	LOG_CALL( _("getScopeObject: ") << ret.toDebugString());

	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}

void ABCVm::abc_getProperty(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t t = (++(context->exec_pos))->data;
	ASObject* obj= NULL;
	if ((instrptr->data&ABC_OP_CACHED) == ABC_OP_CACHED)
	{
		RUNTIME_STACK_POP_ASOBJECT(context,obj, context->mi->context->root->getSystemState());
		if (instrptr->cacheobj1 == obj->getClass())
		{
			asAtom prop;
			//Call the getter
			LOG_CALL("Calling the getter for " << *context->mi->context->getMultiname(t,context) << " on " << obj->toDebugString());
			assert(instrptr->cacheobj2->type == T_FUNCTION);
			IFunction* f = instrptr->cacheobj2->as<IFunction>();
			f->callGetter(prop,instrptr->cacheobj3 ? instrptr->cacheobj3 : obj);
			LOG_CALL("End of getter"<< ' ' << f->toDebugString()<<" result:"<<prop.toDebugString());
			if(prop.type == T_INVALID)
			{
				multiname* name=context->mi->context->getMultiname(t,context);
				checkPropertyException(obj,name,prop);
			}
			obj->decRef();
			RUNTIME_STACK_PUSH(context,prop);
			++(context->exec_pos);
			return;
		}
	}
	multiname* name=context->mi->context->getMultiname(t,context);

	if (obj == NULL)
	{
		RUNTIME_STACK_POP_ASOBJECT(context,obj, context->mi->context->root->getSystemState());
	}

	LOG_CALL( _("getProperty ") << *name << ' ' << obj->toDebugString() << ' '<<obj->isInitialized());

	asAtom prop;
	bool isgetter = obj->getVariableByMultiname(prop,*name,(name->isStatic && obj->getClass() && obj->getClass()->isSealed)? ASObject::DONT_CALL_GETTER:ASObject::NONE) & GET_VARIABLE_RESULT::GETVAR_ISGETTER;
	if (isgetter)
	{
		//Call the getter
		LOG_CALL("Calling the getter for " << *name << " on " << obj->toDebugString());
		assert(prop.type == T_FUNCTION);
		IFunction* f = prop.as<IFunction>();
		ASObject* closure = prop.getClosure();
		f->callGetter(prop,closure ? closure : obj);
		LOG_CALL("End of getter"<< ' ' << f->toDebugString()<<" result:"<<prop.toDebugString());
		if(prop.type != T_INVALID)
		{
			// cache getter if multiname is static and it is a getter of a sealed class
			instrptr->data |= ABC_OP_CACHED;
			instrptr->cacheobj1 = obj->getClass();
			instrptr->cacheobj2 = f;
			instrptr->cacheobj3 = closure;
		}
	}
	if(prop.type == T_INVALID)
		checkPropertyException(obj,name,prop);
	obj->decRef();
	name->resetNameIfObject();

	RUNTIME_STACK_PUSH(context,prop);
	++(context->exec_pos);
}
void ABCVm::abc_getProperty_constant_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t t = (++(context->exec_pos))->data;
	multiname* name=context->mi->context->getMultinameImpl(*instrptr->arg2_constant,NULL,t,false);
	ASObject* obj= instrptr->arg1_constant->toObject(context->mi->context->root->getSystemState());
	LOG_CALL( _("getProperty_cc ") << *name << ' ' << obj->toDebugString() << ' '<<obj->isInitialized());
	asAtom prop;
	obj->getVariableByMultiname(prop,*name);
	if(prop.type == T_INVALID)
		checkPropertyException(obj,name,prop);
	name->resetNameIfObject();
	RUNTIME_STACK_PUSH(context,prop);
	++(context->exec_pos);
}
void ABCVm::abc_getProperty_local_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t t = (++(context->exec_pos))->data;
	multiname* name=context->mi->context->getMultinameImpl(*instrptr->arg2_constant,NULL,t,false);
	ASObject* obj= context->locals[instrptr->local_pos1].toObject(context->mi->context->root->getSystemState());
	LOG_CALL( _("getProperty_lc ") << *name << ' ' << obj->toDebugString() << ' '<<obj->isInitialized());
	asAtom prop;
	obj->getVariableByMultiname(prop,*name);
	if(prop.type == T_INVALID)
		checkPropertyException(obj,name,prop);
	name->resetNameIfObject();
	RUNTIME_STACK_PUSH(context,prop);
	++(context->exec_pos);
}
void ABCVm::abc_getProperty_constant_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t t = (++(context->exec_pos))->data;
	multiname* name=context->mi->context->getMultinameImpl(context->locals[instrptr->local_pos2],NULL,t,false);
	ASObject* obj= instrptr->arg1_constant->toObject(context->mi->context->root->getSystemState());
	LOG_CALL( _("getProperty_cl ") << *name << ' ' << obj->toDebugString() << ' '<<obj->isInitialized());
	asAtom prop;
	obj->getVariableByMultiname(prop,*name);
	if(prop.type == T_INVALID)
		checkPropertyException(obj,name,prop);
	name->resetNameIfObject();
	RUNTIME_STACK_PUSH(context,prop);
	++(context->exec_pos);
}
void ABCVm::abc_getProperty_local_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t t = (++(context->exec_pos))->data;
	multiname* name=context->mi->context->getMultinameImpl(context->locals[instrptr->local_pos2],NULL,t,false);
	ASObject* obj= context->locals[instrptr->local_pos1].toObject(context->mi->context->root->getSystemState());
	LOG_CALL( _("getProperty_ll ") << *name <<"("<<instrptr->local_pos1<<")"<< ' ' << obj->toDebugString() <<"("<<instrptr->local_pos2<<")"<< ' '<<obj->isInitialized());
	asAtom prop;
	obj->getVariableByMultiname(prop,*name);
	if(prop.type == T_INVALID)
		checkPropertyException(obj,name,prop);
	name->resetNameIfObject();
	RUNTIME_STACK_PUSH(context,prop);
	++(context->exec_pos);
}
void ABCVm::abc_getProperty_constant_constant_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t t = (++(context->exec_pos))->data;
	multiname* name=context->mi->context->getMultinameImpl(*instrptr->arg2_constant,NULL,t,false);
	ASObject* obj= instrptr->arg1_constant->toObject(context->mi->context->root->getSystemState());
	LOG_CALL( _("getProperty_ccl ") << *name << ' ' << obj->toDebugString() << ' '<<obj->isInitialized());
	asAtom prop;
	obj->getVariableByMultiname(prop,*name);
	if(prop.type == T_INVALID)
		checkPropertyException(obj,name,prop);
	name->resetNameIfObject();
	context->locals[instrptr->local_pos3-1].set(prop);
	++(context->exec_pos);
}
void ABCVm::abc_getProperty_local_constant_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t t = (++(context->exec_pos))->data;
	asAtom prop;
	if (instrptr->arg2_constant->type == T_INTEGER 
			&& context->locals[instrptr->local_pos1].is<Array>()
			&& instrptr->arg2_constant->getInt() > 0 
			&& (uint32_t)instrptr->arg2_constant->getInt() < context->locals[instrptr->local_pos1].as<Array>()->currentsize)
	{
		LOG_CALL( _("getProperty_lcl ") << instrptr->arg2_constant->getInt() << ' ' << context->locals[instrptr->local_pos1].toDebugString());
		context->locals[instrptr->local_pos1].as<Array>()->at_nocheck(prop,instrptr->arg2_constant->getInt());
	}
	else
	{
		multiname* name=context->mi->context->getMultinameImpl(*instrptr->arg2_constant,NULL,t,false);
		ASObject* obj= context->locals[instrptr->local_pos1].toObject(context->mi->context->root->getSystemState());
		LOG_CALL( _("getProperty_lcl ") << *name << ' ' << obj->toDebugString() << ' '<<obj->isInitialized());
		obj->getVariableByMultiname(prop,*name);
		if(prop.type == T_INVALID)
			checkPropertyException(obj,name,prop);
		name->resetNameIfObject();
	}
	context->locals[instrptr->local_pos3-1].set(prop);
	++(context->exec_pos);
}
void ABCVm::abc_getProperty_constant_local_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t t = (++(context->exec_pos))->data;
	multiname* name=context->mi->context->getMultinameImpl(context->locals[instrptr->local_pos2],NULL,t,false);
	ASObject* obj= instrptr->arg1_constant->toObject(context->mi->context->root->getSystemState());
	LOG_CALL( _("getProperty_cll ") << *name << ' ' << obj->toDebugString() << ' '<<obj->isInitialized());
	asAtom prop;
	obj->getVariableByMultiname(prop,*name);
	if(prop.type == T_INVALID)
		checkPropertyException(obj,name,prop);
	name->resetNameIfObject();
	context->locals[instrptr->local_pos3-1].set(prop);
	++(context->exec_pos);
}
void ABCVm::abc_getProperty_local_local_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t t = (++(context->exec_pos))->data;
	asAtom prop;
	if (context->locals[instrptr->local_pos2].type == T_INTEGER 
			&& context->locals[instrptr->local_pos1].is<Array>()
			&& context->locals[instrptr->local_pos2].getInt() > 0 
			&& (uint32_t)context->locals[instrptr->local_pos2].getInt() < context->locals[instrptr->local_pos1].as<Array>()->currentsize)
	{
		LOG_CALL( _("getProperty_lll ") << context->locals[instrptr->local_pos2].getInt() << ' ' << context->locals[instrptr->local_pos1].toDebugString());
		context->locals[instrptr->local_pos1].as<Array>()->at_nocheck(prop,context->locals[instrptr->local_pos2].getInt());
	}
	else
	{
		multiname* name=context->mi->context->getMultinameImpl(context->locals[instrptr->local_pos2],NULL,t,false);
		ASObject* obj= context->locals[instrptr->local_pos1].toObject(context->mi->context->root->getSystemState());
		LOG_CALL( _("getProperty_lll ") << *name << ' ' << obj->toDebugString() << ' '<<obj->isInitialized());
		obj->getVariableByMultiname(prop,*name);
		if(prop.type == T_INVALID)
			checkPropertyException(obj,name,prop);
		name->resetNameIfObject();
	}
	context->locals[instrptr->local_pos3-1].set(prop);
	++(context->exec_pos);
}

void ABCVm::abc_getPropertyStaticName_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));
	multiname* name=instrptr->cachedmultiname2;

	ASObject* obj= instrptr->arg1_constant->toObject(context->mi->context->root->getSystemState());
	LOG_CALL( _("getProperty_sc ") << *name << ' ' << obj->toDebugString() << ' '<<obj->isInitialized());
	asAtom prop;
	obj->getVariableByMultiname(prop,*name);
	if(prop.type == T_INVALID)
		checkPropertyException(obj,name,prop);
	RUNTIME_STACK_PUSH(context,prop);
	++(context->exec_pos);
}
void ABCVm::abc_getPropertyStaticName_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));
	asAtom prop;
	multiname* name=instrptr->cachedmultiname2;

	if (name->name_type == multiname::NAME_INT
			&& context->locals[instrptr->local_pos1].is<Array>()
			&& name->name_i > 0 
			&& (uint32_t)name->name_i < context->locals[instrptr->local_pos1].as<Array>()->currentsize)
	{
		LOG_CALL( _("getProperty_sl ") << name->name_i << ' ' << context->locals[instrptr->local_pos1].toDebugString());
		context->locals[instrptr->local_pos1].as<Array>()->at_nocheck(prop,name->name_i);
	}
	else
	{
		ASObject* obj= context->locals[instrptr->local_pos1].toObject(context->mi->context->root->getSystemState());
		LOG_CALL( _("getProperty_sl ") << *name << ' ' << obj->toDebugString() << ' '<<obj->isInitialized());
		obj->getVariableByMultiname(prop,*name);
		if(prop.type == T_INVALID)
			checkPropertyException(obj,name,prop);
	}
	RUNTIME_STACK_PUSH(context,prop);
	++(context->exec_pos);
}
void ABCVm::abc_getPropertyStaticName_constant_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));
	multiname* name=instrptr->cachedmultiname2;

	ASObject* obj= instrptr->arg1_constant->toObject(context->mi->context->root->getSystemState());
	LOG_CALL( _("getProperty_scl ") << *name << ' ' << obj->toDebugString() << ' '<<obj->isInitialized());
	asAtom prop;
	obj->getVariableByMultiname(prop,*name);
	if(prop.type == T_INVALID)
		checkPropertyException(obj,name,prop);
	name->resetNameIfObject();
	context->locals[instrptr->local_pos3-1].set(prop);
	++(context->exec_pos);
}
void ABCVm::abc_getPropertyStaticName_local_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));
	multiname* name=instrptr->cachedmultiname2;

	if ((context->exec_pos->data&ABC_OP_CACHED) == ABC_OP_CACHED)
	{
		ASObject* obj= context->locals[instrptr->local_pos1].toObject(context->mi->context->root->getSystemState());
		if (context->exec_pos->cacheobj1 == obj)
		{
			asAtom prop;
			LOG_CALL( _("getProperty_sll_c ") << *name << ' ' << obj->toDebugString());

			variable* v = context->exec_pos->cachedvar2;
			if (v->getter.type != T_INVALID)
			{
				prop.set(v->getter);
				//Call the getter
				LOG_CALL("Calling the getter for " << *name << " on " << obj->toDebugString());
				assert(prop.type == T_FUNCTION);
				IFunction* f = prop.as<IFunction>();
				ASObject* closure = prop.getClosure();
				f->callGetter(prop,closure ? closure : obj);
				LOG_CALL("End of getter"<< ' ' << f->toDebugString()<<" result:"<<prop.toDebugString());
			}
			else
			{
				prop.set(v->var);
				ASATOM_INCREF(prop);
			}
			
			if(prop.type == T_INVALID)
			{
				checkPropertyException(obj,name,prop);
			}
			if(prop.type == T_INVALID)
				checkPropertyException(obj,name,prop);
			context->locals[instrptr->local_pos3-1].set(prop);
			++(context->exec_pos);
			return;
		}
		else
			context->exec_pos->data = ABC_OP_NOTCACHEABLE;
	}
	
	if (name->name_type == multiname::NAME_INT
			&& context->locals[instrptr->local_pos1].is<Array>()
			&& name->name_i > 0 
			&& (uint32_t)name->name_i < context->locals[instrptr->local_pos1].as<Array>()->currentsize)
	{
		LOG_CALL( _("getProperty_sll ") << name->name_i << ' ' << context->locals[instrptr->local_pos1].toDebugString());
		context->locals[instrptr->local_pos1].as<Array>()->at_nocheck(context->locals[instrptr->local_pos3-1],name->name_i);
	}
	else
	{
		ASObject* obj= context->locals[instrptr->local_pos1].toObject(context->mi->context->root->getSystemState());
		LOG_CALL( _("getProperty_sll ") << *name << ' ' << obj->toDebugString() << ' '<<obj->isInitialized());
		asAtom prop;
		if (((context->exec_pos->data&ABC_OP_NOTCACHEABLE) == 0) 
				&& ((obj->is<Class_base>() && obj->as<Class_base>()->isSealed) ||
					(obj->getClass() 
					 && (obj->getClass()->isSealed 
						 || (obj->getClass() == Class<Array>::getRef(obj->getSystemState()).getPtr())))))
		{
			variable* v = obj->findVariableByMultiname(*name,ASObject::NONE,obj->getClass());
			if (v)
			{
				context->exec_pos->data |= ABC_OP_CACHED;
				context->exec_pos->cachedvar2=v;
				context->exec_pos->cacheobj1=obj;
				if (v->getter.type != T_INVALID)
				{
					prop.set(v->getter);
					//Call the getter
					LOG_CALL("Calling the getter for " << *name << " on " << obj->toDebugString());
					assert(prop.type == T_FUNCTION);
					IFunction* f = prop.as<IFunction>();
					ASObject* closure = prop.getClosure();
					f->callGetter(prop,closure ? closure : obj);
					LOG_CALL("End of getter"<< ' ' << f->toDebugString()<<" result:"<<prop.toDebugString());
				}
				else
				{
					prop.set(v->var);
					ASATOM_INCREF(prop);
				}
			}
		}
		if(prop.type == T_INVALID)
			obj->getVariableByMultiname(prop,*name);
		if(prop.type == T_INVALID)
			checkPropertyException(obj,name,prop);
		context->locals[instrptr->local_pos3-1].set(prop);
	}
	++(context->exec_pos);
}

void ABCVm::abc_initproperty(call_context* context)
{
	//initproperty
	uint32_t t = (++(context->exec_pos))->data;
	RUNTIME_STACK_POP_CREATE(context,value);
	multiname* name=context->mi->context->getMultiname(t,context);
	RUNTIME_STACK_POP_CREATE(context,obj);
	LOG_CALL("initProperty "<<*name<<" on "<< obj->toDebugString());
	obj->toObject(context->mi->context->root->getSystemState())->setVariableByMultiname(*name,*value,ASObject::CONST_ALLOWED);
	ASATOM_DECREF_POINTER(obj);
	name->resetNameIfObject();
	++(context->exec_pos);
}
void ABCVm::abc_deleteproperty(call_context* context)
{
	//deleteproperty
	uint32_t t = (++(context->exec_pos))->data;
	multiname* name = context->mi->context->getMultiname(t,context);
	RUNTIME_STACK_POP_CREATE_ASOBJECT(context,obj, context->mi->context->root->getSystemState());
	bool ret = deleteProperty(obj,name);
	name->resetNameIfObject();
	RUNTIME_STACK_PUSH(context,asAtom(ret));
	++(context->exec_pos);
}
void ABCVm::abc_getslot(call_context* context)
{
	//getslot
	uint32_t t = (++(context->exec_pos))->data;
	RUNTIME_STACK_POINTER_CREATE(context,pval);

	asAtom ret=pval->toObject(context->mi->context->root->getSystemState())->getSlot(t);
	LOG_CALL("getSlot " << t << " " << ret.toDebugString());
	//getSlot can only access properties defined in the current
	//script, so they should already be defind by this script
	ASATOM_INCREF(ret);
	ASATOM_DECREF_POINTER(pval);

	*pval=ret;
	++(context->exec_pos);
}
void ABCVm::abc_setslot(call_context* context)
{
	//setslot
	uint32_t t = (++(context->exec_pos))->data;
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v2, context->mi->context->root->getSystemState());

	LOG_CALL("setSlot " << t << " "<< v2->toDebugString() << " "<< v1->toDebugString());
	v2->setSlot(t,*v1);
	v2->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_getglobalSlot(call_context* context)
{
	//getglobalSlot
	uint32_t t = (++(context->exec_pos))->data;

	Global* globalscope = getGlobalScope(context);
	asAtom ret=globalscope->getSlot(t);
	ASATOM_INCREF(ret);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_setglobalSlot(call_context* context)
{
	//setglobalSlot
	uint32_t t = (++(context->exec_pos))->data;

	Global* globalscope = getGlobalScope(context);
	RUNTIME_STACK_POP_CREATE(context,o);
	globalscope->setSlot(t,*o);
	++(context->exec_pos);
}
void ABCVm::abc_convert_s(call_context* context)
{
	//convert_s
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	LOG_CALL( _("convert_s") );
	if(pval->type != T_STRING)
	{
		tiny_string s = pval->toString(context->mi->context->root->getSystemState());
		ASATOM_DECREF_POINTER(pval);
		pval->replace((ASObject*)abstract_s(context->mi->context->root->getSystemState(),s));
	}
	++(context->exec_pos);
}
void ABCVm::abc_esc_xelem(call_context* context)
{
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	pval->replace(esc_xelem(pval->toObject(context->mi->context->root->getSystemState())));
	++(context->exec_pos);
}
void ABCVm::abc_esc_xattr(call_context* context)
{
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	pval->replace(esc_xattr(pval->toObject(context->mi->context->root->getSystemState())));
	++(context->exec_pos);
}
void ABCVm::abc_convert_i(call_context* context)
{
	//convert_i
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	LOG_CALL("convert_i");
	pval->convert_i();
	++(context->exec_pos);
}
void ABCVm::abc_convert_u(call_context* context)
{
	//convert_u
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	LOG_CALL("convert_u");
	pval->convert_u();
	++(context->exec_pos);
}
void ABCVm::abc_convert_d(call_context* context)
{
	//convert_d
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	LOG_CALL("convert_d");
	pval->convert_d();
	++(context->exec_pos);
}
void ABCVm::abc_convert_d_constant(call_context* context)
{
	//convert_d
	LOG_CALL("convert_d_c");
	asAtom res = *context->exec_pos->arg1_constant;
	res.convert_d();
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_convert_d_local(call_context* context)
{
	//convert_d
	LOG_CALL("convert_d_l:"<<context->exec_pos->local_pos1<<" "<<context->locals_size<<" "<<context->locals[context->exec_pos->local_pos1].toDebugString());
	asAtom res =context->locals[context->exec_pos->local_pos1];
	res.convert_d();
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_convert_d_constant_localresult(call_context* context)
{
	//convert_d
	LOG_CALL("convert_d_cl");
	context->locals[context->exec_pos->local_pos3-1].set(*context->exec_pos->arg1_constant);
	context->locals[context->exec_pos->local_pos3-1].convert_d();
	++(context->exec_pos);
}
void ABCVm::abc_convert_d_local_localresult(call_context* context)
{
	//convert_d
	LOG_CALL("convert_d_ll");
	context->locals[context->exec_pos->local_pos3-1].set(context->locals[context->exec_pos->local_pos1]);
	context->locals[context->exec_pos->local_pos3-1].convert_d();
	++(context->exec_pos);
}
void ABCVm::abc_convert_b(call_context* context)
{
	//convert_b
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	LOG_CALL("convert_b");
	pval->convert_b();
	++(context->exec_pos);
}
void ABCVm::abc_convert_o(call_context* context)
{
	//convert_o
	LOG_CALL("convert_o");
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	if (pval->type == T_NULL)
	{
		LOG(LOG_ERROR,"trying to call convert_o on null");
		throwError<TypeError>(kConvertNullToObjectError);
	}
	if (pval->type == T_UNDEFINED)
	{
		LOG(LOG_ERROR,"trying to call convert_o on undefined");
		throwError<TypeError>(kConvertUndefinedToObjectError);
	}
	++(context->exec_pos);
}
void ABCVm::abc_checkfilter(call_context* context)
{
	//checkfilter
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	checkfilter(pval->toObject(context->mi->context->root->getSystemState()));
	++(context->exec_pos);
}
void ABCVm::abc_coerce(call_context* context)
{
	//coerce
	uint32_t t = (++(context->exec_pos))->data;
	coerce(context, t);
	++(context->exec_pos);
}
void ABCVm::abc_coerce_a(call_context* context)
{
	//coerce_a
	coerce_a();
	++(context->exec_pos);
}
void ABCVm::abc_coerce_s(call_context* context)
{
	//coerce_s
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	LOG_CALL("coerce_s:"<<pval->toDebugString());
	if (pval->type != T_STRING)
		Class<ASString>::getClass(context->mi->context->root->getSystemState())->coerce(context->mi->context->root->getSystemState(),*pval);
	++(context->exec_pos);
}
void ABCVm::abc_astype(call_context* context)
{
	//astype
	uint32_t t = (++(context->exec_pos))->data;
	multiname* name=context->mi->context->getMultiname(t,NULL);

	RUNTIME_STACK_POINTER_CREATE(context,pval);
	pval->replace(asType(context->mi->context, pval->toObject(context->mi->context->root->getSystemState()), name));
	++(context->exec_pos);
}
void ABCVm::abc_astypelate(call_context* context)
{
	//astypelate
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	*pval = pval->asTypelate(*v1);
	++(context->exec_pos);
}
void ABCVm::abc_negate(call_context* context)
{
	//negate
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	number_t ret=-(pval->toNumber());
	ASATOM_DECREF_POINTER(pval);
	pval->setNumber(ret);
	++(context->exec_pos);
}
void ABCVm::abc_increment(call_context* context)
{
	//increment
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	LOG_CALL("increment "<<pval->toDebugString());
	pval->increment();
	++(context->exec_pos);
}
void ABCVm::abc_increment_local(call_context* context)
{
	//increment
	asAtom res = context->locals[context->exec_pos->local_pos1];
	LOG_CALL("increment_l "<<context->exec_pos->local_pos1<<" "<<res.toDebugString());
	res.increment();
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_increment_local_localresult(call_context* context)
{
	//increment
	context->locals[context->exec_pos->local_pos3-1]= context->locals[context->exec_pos->local_pos1];
	LOG_CALL("increment_ll "<<context->exec_pos->local_pos1<<" "<<context->exec_pos->local_pos3<<" "<<context->locals[context->exec_pos->local_pos3-1].toDebugString());
	context->locals[context->exec_pos->local_pos3-1].increment();
	++(context->exec_pos);
}
void ABCVm::abc_inclocal(call_context* context)
{
	//inclocal
	uint32_t t = (++(context->exec_pos))->data;
	incLocal(context, t);
	++(context->exec_pos);
}
void ABCVm::abc_decrement(call_context* context)
{
	//decrement
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	pval->decrement();
	++(context->exec_pos);
}
void ABCVm::abc_decrement_local(call_context* context)
{
	//decrement
	asAtom res = context->locals[context->exec_pos->local_pos1];
	res.decrement();
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_decrement_local_localresult(call_context* context)
{
	//decrement
	context->locals[context->exec_pos->local_pos3-1]= context->locals[context->exec_pos->local_pos1];
	context->locals[context->exec_pos->local_pos3-1].decrement();
	++(context->exec_pos);
}
void ABCVm::abc_declocal(call_context* context)
{
	//declocal
	uint32_t t = (++(context->exec_pos))->data;
	decLocal(context, t);
	++(context->exec_pos);
}
void ABCVm::abc_typeof(call_context* context)
{
	//typeof
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	LOG_CALL(_("typeOf"));
	asAtom ret = pval->typeOf(context->mi->context->root->getSystemState());
	ASATOM_DECREF_POINTER(pval);
	*pval = ret;
	++(context->exec_pos);
}
void ABCVm::abc_not(call_context* context)
{
	//not
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	pval->_not();
	++(context->exec_pos);
}
void ABCVm::abc_bitnot(call_context* context)
{
	//bitnot
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	pval->bitnot();
	++(context->exec_pos);
}
void ABCVm::abc_add(call_context* context)
{
	//add
	RUNTIME_STACK_POP_CREATE(context,v2);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	pval->add(*v2,context->mi->context->root->getSystemState(),true);
	++(context->exec_pos);
}
void ABCVm::abc_add_constant_constant(call_context* context)
{
	//add
	asAtom res = *context->exec_pos->arg1_constant;
	res.add(*context->exec_pos->arg2_constant,context->mi->context->root->getSystemState(),false);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_add_local_constant(call_context* context)
{
	//add
	asAtom res = context->locals[context->exec_pos->local_pos1];
	res.add(*context->exec_pos->arg2_constant,context->mi->context->root->getSystemState(),false);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_add_constant_local(call_context* context)
{
	//add
	asAtom res = *context->exec_pos->arg1_constant;
	res.add(context->locals[context->exec_pos->local_pos2],context->mi->context->root->getSystemState(),false);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_add_local_local(call_context* context)
{
	//add
	asAtom res = context->locals[context->exec_pos->local_pos1];
	res.add(context->locals[context->exec_pos->local_pos2],context->mi->context->root->getSystemState(),false);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_add_constant_constant_localresult(call_context* context)
{
	//add
	asAtom res = *context->exec_pos->arg1_constant;
	res.add(*context->exec_pos->arg2_constant,context->mi->context->root->getSystemState(),false);
	if (res.getObject() != context->locals[context->exec_pos->local_pos3-1].getObject())
	{
		ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	}
	context->locals[context->exec_pos->local_pos3-1].set(res);
	++(context->exec_pos);
}
void ABCVm::abc_add_local_constant_localresult(call_context* context)
{
	//add
	asAtom res = context->locals[context->exec_pos->local_pos1];
	res.add(*context->exec_pos->arg2_constant,context->mi->context->root->getSystemState(),false);
	if (res.getObject() != context->locals[context->exec_pos->local_pos3-1].getObject())
	{
		ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	}
	context->locals[context->exec_pos->local_pos3-1].set(res);
	++(context->exec_pos);
}
void ABCVm::abc_add_constant_local_localresult(call_context* context)
{
	//add
	asAtom res = *context->exec_pos->arg1_constant;
	res.add(context->locals[context->exec_pos->local_pos2],context->mi->context->root->getSystemState(),false);
	if (res.getObject() != context->locals[context->exec_pos->local_pos3-1].getObject())
	{
		ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	}
	context->locals[context->exec_pos->local_pos3-1].set(res);
	++(context->exec_pos);
}
void ABCVm::abc_add_local_local_localresult(call_context* context)
{
	//add
	asAtom res = context->locals[context->exec_pos->local_pos1];
	res.add(context->locals[context->exec_pos->local_pos2],context->mi->context->root->getSystemState(),false);
	if (res.getObject() != context->locals[context->exec_pos->local_pos3-1].getObject())
	{
		ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	}
	context->locals[context->exec_pos->local_pos3-1].set(res);
	++(context->exec_pos);
}

void ABCVm::abc_subtract(call_context* context)
{
	//subtract
	//Be careful, operands in subtract implementation are swapped
	RUNTIME_STACK_POP_CREATE(context,v2);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	pval->subtract(*v2);
	++(context->exec_pos);
}
void ABCVm::abc_subtract_constant_constant(call_context* context)
{
	//subtract
	asAtom res = *context->exec_pos->arg1_constant;
	res.subtract(*context->exec_pos->arg2_constant);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_subtract_local_constant(call_context* context)
{
	//subtract
	asAtom res = context->locals[context->exec_pos->local_pos1];
	res.subtract(*context->exec_pos->arg2_constant);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_subtract_constant_local(call_context* context)
{
	//subtract
	asAtom res = *context->exec_pos->arg1_constant;
	res.subtract(context->locals[context->exec_pos->local_pos2]);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_subtract_local_local(call_context* context)
{
	//subtract
	asAtom res = context->locals[context->exec_pos->local_pos1];
	res.subtract(context->locals[context->exec_pos->local_pos2]);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_subtract_constant_constant_localresult(call_context* context)
{
	//subtract
	asAtom res = *context->exec_pos->arg1_constant;
	res.subtract(*context->exec_pos->arg2_constant);
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	context->locals[context->exec_pos->local_pos3-1].set(res);
	++(context->exec_pos);
}
void ABCVm::abc_subtract_local_constant_localresult(call_context* context)
{
	//subtract
	asAtom res = context->locals[context->exec_pos->local_pos1];
	res.subtract(*context->exec_pos->arg2_constant);
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	context->locals[context->exec_pos->local_pos3-1].set(res);
	++(context->exec_pos);
}
void ABCVm::abc_subtract_constant_local_localresult(call_context* context)
{
	//subtract
	asAtom res = *context->exec_pos->arg1_constant;
	res.subtract(context->locals[context->exec_pos->local_pos2]);
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	context->locals[context->exec_pos->local_pos3-1].set(res);
	++(context->exec_pos);
}
void ABCVm::abc_subtract_local_local_localresult(call_context* context)
{
	//subtract
	asAtom res = context->locals[context->exec_pos->local_pos1];
	res.subtract(context->locals[context->exec_pos->local_pos2]);
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	context->locals[context->exec_pos->local_pos3-1].set(res);
	++(context->exec_pos);
}

void ABCVm::abc_multiply(call_context* context)
{
	//multiply
	RUNTIME_STACK_POP_CREATE(context,v2);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	pval->multiply(*v2);
	++(context->exec_pos);
}
void ABCVm::abc_multiply_constant_constant(call_context* context)
{
	//multiply
	asAtom res = *context->exec_pos->arg1_constant;
	res.multiply(*context->exec_pos->arg2_constant);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_multiply_local_constant(call_context* context)
{
	//multiply
	asAtom res = context->locals[context->exec_pos->local_pos1];
	res.multiply(*context->exec_pos->arg2_constant);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_multiply_constant_local(call_context* context)
{
	//multiply
	asAtom res = *context->exec_pos->arg1_constant;
	res.multiply(context->locals[context->exec_pos->local_pos2]);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_multiply_local_local(call_context* context)
{
	//multiply
	asAtom res = context->locals[context->exec_pos->local_pos1];
	res.multiply(context->locals[context->exec_pos->local_pos2]);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_multiply_constant_constant_localresult(call_context* context)
{
	//multiply
	asAtom res = *context->exec_pos->arg1_constant;
	res.multiply(*context->exec_pos->arg2_constant);
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	context->locals[context->exec_pos->local_pos3-1].set(res);
	++(context->exec_pos);
}
void ABCVm::abc_multiply_local_constant_localresult(call_context* context)
{
	//multiply
	asAtom res = context->locals[context->exec_pos->local_pos1];
	res.multiply(*context->exec_pos->arg2_constant);
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	context->locals[context->exec_pos->local_pos3-1].set(res);
	++(context->exec_pos);
}
void ABCVm::abc_multiply_constant_local_localresult(call_context* context)
{
	//multiply
	asAtom res = *context->exec_pos->arg1_constant;
	res.multiply(context->locals[context->exec_pos->local_pos2]);
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	context->locals[context->exec_pos->local_pos3-1].set(res);
	++(context->exec_pos);
}
void ABCVm::abc_multiply_local_local_localresult(call_context* context)
{
	//multiply
	asAtom res = context->locals[context->exec_pos->local_pos1];
	res.multiply(context->locals[context->exec_pos->local_pos2]);
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	context->locals[context->exec_pos->local_pos3-1].set(res);
	++(context->exec_pos);
}

void ABCVm::abc_divide(call_context* context)
{
	//divide
	RUNTIME_STACK_POP_CREATE(context,v2);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	pval->divide(*v2);
	++(context->exec_pos);
}
void ABCVm::abc_divide_constant_constant(call_context* context)
{
	//divide
	asAtom res = *context->exec_pos->arg1_constant;
	res.divide(*context->exec_pos->arg2_constant);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_divide_local_constant(call_context* context)
{
	//divide
	asAtom res = context->locals[context->exec_pos->local_pos1];
	res.divide(*context->exec_pos->arg2_constant);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_divide_constant_local(call_context* context)
{
	//divide
	asAtom res = *context->exec_pos->arg1_constant;
	res.divide(context->locals[context->exec_pos->local_pos2]);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_divide_local_local(call_context* context)
{
	//divide
	asAtom res = context->locals[context->exec_pos->local_pos1];
	res.divide(context->locals[context->exec_pos->local_pos2]);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_divide_constant_constant_localresult(call_context* context)
{
	//divide
	asAtom res = *context->exec_pos->arg1_constant;
	res.divide(*context->exec_pos->arg2_constant);
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	context->locals[context->exec_pos->local_pos3-1].set(res);
	++(context->exec_pos);
}
void ABCVm::abc_divide_local_constant_localresult(call_context* context)
{
	//divide
	asAtom res = context->locals[context->exec_pos->local_pos1];
	res.divide(*context->exec_pos->arg2_constant);
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	context->locals[context->exec_pos->local_pos3-1].set(res);
	++(context->exec_pos);
}
void ABCVm::abc_divide_constant_local_localresult(call_context* context)
{
	//divide
	asAtom res = *context->exec_pos->arg1_constant;
	res.divide(context->locals[context->exec_pos->local_pos2]);
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	context->locals[context->exec_pos->local_pos3-1].set(res);
	++(context->exec_pos);
}
void ABCVm::abc_divide_local_local_localresult(call_context* context)
{
	//divide
	asAtom res = context->locals[context->exec_pos->local_pos1];
	res.divide(context->locals[context->exec_pos->local_pos2]);
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	context->locals[context->exec_pos->local_pos3-1].set(res);
	++(context->exec_pos);
}
void ABCVm::abc_modulo(call_context* context)
{
	//modulo
	RUNTIME_STACK_POP_CREATE(context,v2);
	RUNTIME_STACK_POINTER_CREATE(context,pval);

	pval->modulo(*v2);
	++(context->exec_pos);
}
void ABCVm::abc_modulo_constant_constant(call_context* context)
{
	//modulo
	asAtom res = *context->exec_pos->arg1_constant;
	res.modulo(*context->exec_pos->arg2_constant);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_modulo_local_constant(call_context* context)
{
	//modulo
	asAtom res = context->locals[context->exec_pos->local_pos1];
	res.modulo(*context->exec_pos->arg2_constant);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_modulo_constant_local(call_context* context)
{
	//modulo
	asAtom res = *context->exec_pos->arg1_constant;
	res.modulo(context->locals[context->exec_pos->local_pos2]);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_modulo_local_local(call_context* context)
{
	//modulo
	asAtom res = context->locals[context->exec_pos->local_pos1];
	res.modulo(context->locals[context->exec_pos->local_pos2]);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_modulo_constant_constant_localresult(call_context* context)
{
	//modulo
	asAtom res = *context->exec_pos->arg1_constant;
	res.modulo(*context->exec_pos->arg2_constant);
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	context->locals[context->exec_pos->local_pos3-1].set(res);
	++(context->exec_pos);
}
void ABCVm::abc_modulo_local_constant_localresult(call_context* context)
{
	//modulo
	asAtom res = context->locals[context->exec_pos->local_pos1];
	res.modulo(*context->exec_pos->arg2_constant);
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	context->locals[context->exec_pos->local_pos3-1].set(res);
	++(context->exec_pos);
}
void ABCVm::abc_modulo_constant_local_localresult(call_context* context)
{
	//modulo
	asAtom res = *context->exec_pos->arg1_constant;
	res.modulo(context->locals[context->exec_pos->local_pos2]);
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	context->locals[context->exec_pos->local_pos3-1].set(res);
	++(context->exec_pos);
}
void ABCVm::abc_modulo_local_local_localresult(call_context* context)
{
	//modulo
	asAtom res = context->locals[context->exec_pos->local_pos1];
	res.modulo(context->locals[context->exec_pos->local_pos2]);
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	context->locals[context->exec_pos->local_pos3-1].set(res);
	++(context->exec_pos);
}
void ABCVm::abc_lshift(call_context* context)
{
	//lshift
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	pval->lshift(*v1);
	++(context->exec_pos);
}
void ABCVm::abc_lshift_constant_constant(call_context* context)
{
	//lshift
	asAtom res = *context->exec_pos->arg1_constant;
	res.lshift(*context->exec_pos->arg2_constant);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_lshift_local_constant(call_context* context)
{
	//lshift
	asAtom res = context->locals[context->exec_pos->local_pos1];
	res.lshift(*context->exec_pos->arg2_constant);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_lshift_constant_local(call_context* context)
{
	//lshift
	asAtom res = *context->exec_pos->arg1_constant;
	res.lshift(context->locals[context->exec_pos->local_pos2]);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_lshift_local_local(call_context* context)
{
	//lshift
	asAtom res = context->locals[context->exec_pos->local_pos1];
	res.lshift(context->locals[context->exec_pos->local_pos2]);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_lshift_constant_constant_localresult(call_context* context)
{
	//lshift
	asAtom res = *context->exec_pos->arg1_constant;
	res.lshift(*context->exec_pos->arg2_constant);
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	context->locals[context->exec_pos->local_pos3-1].set(res);
	++(context->exec_pos);
}
void ABCVm::abc_lshift_local_constant_localresult(call_context* context)
{
	//lshift
	asAtom res = context->locals[context->exec_pos->local_pos1];
	res.lshift(*context->exec_pos->arg2_constant);
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	context->locals[context->exec_pos->local_pos3-1].set(res);
	++(context->exec_pos);
}
void ABCVm::abc_lshift_constant_local_localresult(call_context* context)
{
	//lshift
	asAtom res = *context->exec_pos->arg1_constant;
	res.lshift(context->locals[context->exec_pos->local_pos2]);
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	context->locals[context->exec_pos->local_pos3-1].set(res);
	++(context->exec_pos);
}
void ABCVm::abc_lshift_local_local_localresult(call_context* context)
{
	//lshift
	asAtom res = context->locals[context->exec_pos->local_pos1];
	res.lshift(context->locals[context->exec_pos->local_pos2]);
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	context->locals[context->exec_pos->local_pos3-1].set(res);
	++(context->exec_pos);
}
void ABCVm::abc_rshift(call_context* context)
{
	//rshift
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	pval->rshift(*v1);
	++(context->exec_pos);
}
void ABCVm::abc_rshift_constant_constant(call_context* context)
{
	//rshift
	asAtom res = *context->exec_pos->arg1_constant;
	res.rshift(*context->exec_pos->arg2_constant);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_rshift_local_constant(call_context* context)
{
	//rshift
	asAtom res = context->locals[context->exec_pos->local_pos1];
	res.rshift(*context->exec_pos->arg2_constant);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_rshift_constant_local(call_context* context)
{
	//rshift
	asAtom res = *context->exec_pos->arg1_constant;
	res.rshift(context->locals[context->exec_pos->local_pos2]);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_rshift_local_local(call_context* context)
{
	//rshift
	asAtom res = context->locals[context->exec_pos->local_pos1];
	res.rshift(context->locals[context->exec_pos->local_pos2]);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_rshift_constant_constant_localresult(call_context* context)
{
	//rshift
	asAtom res = *context->exec_pos->arg1_constant;
	res.rshift(*context->exec_pos->arg2_constant);
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	context->locals[context->exec_pos->local_pos3-1].set(res);
	++(context->exec_pos);
}
void ABCVm::abc_rshift_local_constant_localresult(call_context* context)
{
	//rshift
	asAtom res = context->locals[context->exec_pos->local_pos1];
	res.rshift(*context->exec_pos->arg2_constant);
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	context->locals[context->exec_pos->local_pos3-1].set(res);
	++(context->exec_pos);
}
void ABCVm::abc_rshift_constant_local_localresult(call_context* context)
{
	//rshift
	asAtom res = *context->exec_pos->arg1_constant;
	res.rshift(context->locals[context->exec_pos->local_pos2]);
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	context->locals[context->exec_pos->local_pos3-1].set(res);
	++(context->exec_pos);
}
void ABCVm::abc_rshift_local_local_localresult(call_context* context)
{
	//rshift
	asAtom res = context->locals[context->exec_pos->local_pos1];
	res.rshift(context->locals[context->exec_pos->local_pos2]);
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	context->locals[context->exec_pos->local_pos3-1].set(res);
	++(context->exec_pos);
}
void ABCVm::abc_urshift(call_context* context)
{
	//urshift
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	pval->urshift(*v1);
	++(context->exec_pos);
}
void ABCVm::abc_urshift_constant_constant(call_context* context)
{
	//urshift
	asAtom res = *context->exec_pos->arg1_constant;
	res.urshift(*context->exec_pos->arg2_constant);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_urshift_local_constant(call_context* context)
{
	//urshift
	asAtom res = context->locals[context->exec_pos->local_pos1];
	res.urshift(*context->exec_pos->arg2_constant);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_urshift_constant_local(call_context* context)
{
	//urshift
	asAtom res = *context->exec_pos->arg1_constant;
	res.urshift(context->locals[context->exec_pos->local_pos2]);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_urshift_local_local(call_context* context)
{
	//urshift
	asAtom res = context->locals[context->exec_pos->local_pos1];
	res.urshift(context->locals[context->exec_pos->local_pos2]);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_urshift_constant_constant_localresult(call_context* context)
{
	//urshift
	asAtom res = *context->exec_pos->arg1_constant;
	res.urshift(*context->exec_pos->arg2_constant);
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	context->locals[context->exec_pos->local_pos3-1].set(res);
	++(context->exec_pos);
}
void ABCVm::abc_urshift_local_constant_localresult(call_context* context)
{
	//urshift
	asAtom res = context->locals[context->exec_pos->local_pos1];
	res.urshift(*context->exec_pos->arg2_constant);
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	context->locals[context->exec_pos->local_pos3-1].set(res);
	++(context->exec_pos);
}
void ABCVm::abc_urshift_constant_local_localresult(call_context* context)
{
	//urshift
	asAtom res = *context->exec_pos->arg1_constant;
	res.urshift(context->locals[context->exec_pos->local_pos2]);
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	context->locals[context->exec_pos->local_pos3-1].set(res);
	++(context->exec_pos);
}
void ABCVm::abc_urshift_local_local_localresult(call_context* context)
{
	//urshift
	asAtom res = context->locals[context->exec_pos->local_pos1];
	res.urshift(context->locals[context->exec_pos->local_pos2]);
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	context->locals[context->exec_pos->local_pos3-1].set(res);
	++(context->exec_pos);
}
void ABCVm::abc_bitand(call_context* context)
{
	//bitand
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	pval->bit_and(*v1);
	++(context->exec_pos);
}
void ABCVm::abc_bitand_constant_constant(call_context* context)
{
	//bitand
	asAtom res = *context->exec_pos->arg1_constant;
	res.bit_and(*context->exec_pos->arg2_constant);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_bitand_local_constant(call_context* context)
{
	//bitand
	asAtom res = context->locals[context->exec_pos->local_pos1];
	res.bit_and(*context->exec_pos->arg2_constant);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_bitand_constant_local(call_context* context)
{
	//bitand
	asAtom res = *context->exec_pos->arg1_constant;
	res.bit_and(context->locals[context->exec_pos->local_pos2]);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_bitand_local_local(call_context* context)
{
	//bitand
	asAtom res = context->locals[context->exec_pos->local_pos1];
	res.bit_and(context->locals[context->exec_pos->local_pos2]);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_bitand_constant_constant_localresult(call_context* context)
{
	//bitand
	asAtom res = *context->exec_pos->arg1_constant;
	res.bit_and(*context->exec_pos->arg2_constant);
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	context->locals[context->exec_pos->local_pos3-1].set(res);
	++(context->exec_pos);
}
void ABCVm::abc_bitand_local_constant_localresult(call_context* context)
{
	//bitand
	asAtom res = context->locals[context->exec_pos->local_pos1];
	res.bit_and(*context->exec_pos->arg2_constant);
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	context->locals[context->exec_pos->local_pos3-1].set(res);
	++(context->exec_pos);
}
void ABCVm::abc_bitand_constant_local_localresult(call_context* context)
{
	//bitand
	asAtom res = *context->exec_pos->arg1_constant;
	res.bit_and(context->locals[context->exec_pos->local_pos2]);
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	context->locals[context->exec_pos->local_pos3-1].set(res);
	++(context->exec_pos);
}
void ABCVm::abc_bitand_local_local_localresult(call_context* context)
{
	//bitand
	asAtom res = context->locals[context->exec_pos->local_pos1];
	res.bit_and(context->locals[context->exec_pos->local_pos2]);
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	context->locals[context->exec_pos->local_pos3-1].set(res);
	++(context->exec_pos);
}
void ABCVm::abc_bitor(call_context* context)
{
	//bitor
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	pval->bit_or(*v1);
	++(context->exec_pos);
}
void ABCVm::abc_bitor_constant_constant(call_context* context)
{
	//bitor
	asAtom res = *context->exec_pos->arg1_constant;
	res.bit_or(*context->exec_pos->arg2_constant);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_bitor_local_constant(call_context* context)
{
	//bitor
	asAtom res = context->locals[context->exec_pos->local_pos1];
	res.bit_or(*context->exec_pos->arg2_constant);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_bitor_constant_local(call_context* context)
{
	//bitor
	asAtom res = *context->exec_pos->arg1_constant;
	res.bit_or(context->locals[context->exec_pos->local_pos2]);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_bitor_local_local(call_context* context)
{
	//bitor
	asAtom res = context->locals[context->exec_pos->local_pos1];
	res.bit_or(context->locals[context->exec_pos->local_pos2]);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_bitor_constant_constant_localresult(call_context* context)
{
	//bitor
	asAtom res = *context->exec_pos->arg1_constant;
	res.bit_or(*context->exec_pos->arg2_constant);
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	context->locals[context->exec_pos->local_pos3-1].set(res);
	++(context->exec_pos);
}
void ABCVm::abc_bitor_local_constant_localresult(call_context* context)
{
	//bitor
	asAtom res = *context->exec_pos->arg2_constant;
	res.bit_or(context->locals[context->exec_pos->local_pos1]);
	context->locals[context->exec_pos->local_pos3-1].set(res);
	++(context->exec_pos);
}
void ABCVm::abc_bitor_constant_local_localresult(call_context* context)
{
	//bitor
	asAtom res = *context->exec_pos->arg1_constant;
	res.bit_or(context->locals[context->exec_pos->local_pos2]);
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	context->locals[context->exec_pos->local_pos3-1].set(res);
	++(context->exec_pos);
}
void ABCVm::abc_bitor_local_local_localresult(call_context* context)
{
	//bitor
	asAtom res = context->locals[context->exec_pos->local_pos1];
	res.bit_or(context->locals[context->exec_pos->local_pos2]);
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	context->locals[context->exec_pos->local_pos3-1].set(res);
	++(context->exec_pos);
}

void ABCVm::abc_bitxor(call_context* context)
{
	//bitxor
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	pval->bit_xor(*v1);
	++(context->exec_pos);
}
void ABCVm::abc_bitxor_constant_constant(call_context* context)
{
	//bitxor
	asAtom res = *context->exec_pos->arg1_constant;
	res.bit_xor(*context->exec_pos->arg2_constant);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_bitxor_local_constant(call_context* context)
{
	//bitxor
	asAtom res = context->locals[context->exec_pos->local_pos1];
	res.bit_xor(*context->exec_pos->arg2_constant);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_bitxor_constant_local(call_context* context)
{
	//bitxor
	asAtom res = *context->exec_pos->arg1_constant;
	res.bit_xor(context->locals[context->exec_pos->local_pos2]);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_bitxor_local_local(call_context* context)
{
	//bitxor
	asAtom res = context->locals[context->exec_pos->local_pos1];
	res.bit_xor(context->locals[context->exec_pos->local_pos2]);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_bitxor_constant_constant_localresult(call_context* context)
{
	//bitxor
	asAtom res = *context->exec_pos->arg1_constant;
	res.bit_xor(*context->exec_pos->arg2_constant);
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	context->locals[context->exec_pos->local_pos3-1].set(res);
	++(context->exec_pos);
}
void ABCVm::abc_bitxor_local_constant_localresult(call_context* context)
{
	//bitxor
	asAtom res = context->locals[context->exec_pos->local_pos1];
	res.bit_xor(*context->exec_pos->arg2_constant);
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	context->locals[context->exec_pos->local_pos3-1].set(res);
	++(context->exec_pos);
}
void ABCVm::abc_bitxor_constant_local_localresult(call_context* context)
{
	//bitxor
	asAtom res = *context->exec_pos->arg1_constant;
	res.bit_xor(context->locals[context->exec_pos->local_pos2]);
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	context->locals[context->exec_pos->local_pos3-1].set(res);
	++(context->exec_pos);
}
void ABCVm::abc_bitxor_local_local_localresult(call_context* context)
{
	//bitxor
	asAtom res = context->locals[context->exec_pos->local_pos1];
	res.bit_xor(context->locals[context->exec_pos->local_pos2]);
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	context->locals[context->exec_pos->local_pos3-1].set(res);
	++(context->exec_pos);
}
void ABCVm::abc_equals(call_context* context)
{
	//equals
	RUNTIME_STACK_POP_CREATE(context,v2);
	RUNTIME_STACK_POINTER_CREATE(context,pval);

	bool ret=pval->isEqual(context->mi->context->root->getSystemState(),*v2);
	LOG_CALL( _("equals ") << ret);
	ASATOM_DECREF_POINTER(pval);
	ASATOM_DECREF_POINTER(v2);
	pval->setBool(ret);
	++(context->exec_pos);
}
void ABCVm::abc_strictequals(call_context* context)
{
	//strictequals
	RUNTIME_STACK_POP_CREATE(context,v2);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	bool ret = pval->isEqualStrict(context->mi->context->root->getSystemState(),*v2);
	LOG_CALL( _("strictequals ") << ret);
	ASATOM_DECREF_POINTER(pval);
	ASATOM_DECREF_POINTER(v2);
	pval->setBool(ret);
	++(context->exec_pos);
}
void ABCVm::abc_lessthan(call_context* context)
{
	//lessthan
	RUNTIME_STACK_POP_CREATE(context,v2);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	//Real comparision demanded to object
	bool ret=(pval->isLess(context->mi->context->root->getSystemState(),*v2)==TTRUE);
	LOG_CALL(_("lessThan ")<<ret);
	ASATOM_DECREF_POINTER(pval);
	ASATOM_DECREF_POINTER(v2);

	pval->setBool(ret);
	++(context->exec_pos);
}
void ABCVm::abc_lessequals(call_context* context)
{
	//lessequals
	RUNTIME_STACK_POP_CREATE(context,v2);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	//Real comparision demanded to object
	bool ret=(v2->isLess(context->mi->context->root->getSystemState(),*pval)==TFALSE);
	LOG_CALL(_("lessEquals ")<<ret);
	ASATOM_DECREF_POINTER(pval);
	ASATOM_DECREF_POINTER(v2);

	pval->setBool(ret);
	++(context->exec_pos);
}
void ABCVm::abc_greaterthan(call_context* context)
{
	//greaterthan
	RUNTIME_STACK_POP_CREATE(context,v2);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	//Real comparision demanded to object
	bool ret=(v2->isLess(context->mi->context->root->getSystemState(),*pval)==TTRUE);
	LOG_CALL(_("greaterThan ")<<ret);
	ASATOM_DECREF_POINTER(pval);
	ASATOM_DECREF_POINTER(v2);

	pval->setBool(ret);
	++(context->exec_pos);
}
void ABCVm::abc_greaterthan_constant_constant(call_context* context)
{
	//greaterthan

	bool ret=(context->exec_pos->arg2_constant->isLess(context->mi->context->root->getSystemState(),*context->exec_pos->arg1_constant)==TTRUE);
	LOG_CALL(_("greaterThan_cc ")<<ret);

	RUNTIME_STACK_PUSH(context,asAtom(ret));
	++(context->exec_pos);
}
void ABCVm::abc_greaterthan_local_constant(call_context* context)
{
	//greaterthan

	bool ret=(context->exec_pos->arg2_constant->isLess(context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos1])==TTRUE);
	LOG_CALL(_("greaterThan_lc ")<<ret);

	RUNTIME_STACK_PUSH(context,asAtom(ret));
	++(context->exec_pos);
}
void ABCVm::abc_greaterthan_constant_local(call_context* context)
{
	//greaterthan

	bool ret=(context->locals[context->exec_pos->local_pos2].isLess(context->mi->context->root->getSystemState(),*context->exec_pos->arg1_constant)==TTRUE);
	LOG_CALL(_("greaterThan_cl ")<<ret);

	RUNTIME_STACK_PUSH(context,asAtom(ret));
	++(context->exec_pos);
}
void ABCVm::abc_greaterthan_local_local(call_context* context)
{
	//greaterthan

	bool ret=(context->locals[context->exec_pos->local_pos2].isLess(context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos1])==TTRUE);
	LOG_CALL(_("greaterThan_ll ")<<ret);

	RUNTIME_STACK_PUSH(context,asAtom(ret));
	++(context->exec_pos);
}
void ABCVm::abc_greaterthan_constant_constant_localresult(call_context* context)
{
	//greaterthan

	bool ret=(context->exec_pos->arg2_constant->isLess(context->mi->context->root->getSystemState(),*context->exec_pos->arg1_constant)==TTRUE);
	LOG_CALL(_("greaterThan_ccl ")<<ret);

	context->locals[context->exec_pos->local_pos3-1].setBool(ret);
	++(context->exec_pos);
}
void ABCVm::abc_greaterthan_local_constant_localresult(call_context* context)
{
	//greaterthan

	bool ret=(context->exec_pos->arg2_constant->isLess(context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos1])==TTRUE);
	LOG_CALL(_("greaterThan_lcl ")<<ret);

	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	context->locals[context->exec_pos->local_pos3-1].setBool(ret);
	++(context->exec_pos);
}
void ABCVm::abc_greaterthan_constant_local_localresult(call_context* context)
{
	//greaterthan

	bool ret=(context->locals[context->exec_pos->local_pos2].isLess(context->mi->context->root->getSystemState(),*context->exec_pos->arg1_constant)==TTRUE);
	LOG_CALL(_("greaterThan_cll ")<<ret);

	context->locals[context->exec_pos->local_pos3-1].setBool(ret);
	++(context->exec_pos);
}
void ABCVm::abc_greaterthan_local_local_localresult(call_context* context)
{
	//greaterthan

	bool ret=(context->locals[context->exec_pos->local_pos2].isLess(context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos1])==TTRUE);
	LOG_CALL(_("greaterThan_lll ")<<ret);

	context->locals[context->exec_pos->local_pos3-1].setBool(ret);
	++(context->exec_pos);
}
void ABCVm::abc_greaterequals(call_context* context)
{
	//greaterequals
	RUNTIME_STACK_POP_CREATE(context,v2);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	//Real comparision demanded to object
	bool ret=(pval->isLess(context->mi->context->root->getSystemState(),*v2)==TFALSE);
	LOG_CALL(_("greaterEquals ")<<ret);
	ASATOM_DECREF_POINTER(pval);
	ASATOM_DECREF_POINTER(v2);

	pval->setBool(ret);
	++(context->exec_pos);
}
void ABCVm::abc_instanceof(call_context* context)
{
	//instanceof
	RUNTIME_STACK_POP_CREATE_ASOBJECT(context,type, context->mi->context->root->getSystemState());
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	bool ret = instanceOf(pval->toObject(context->mi->context->root->getSystemState()),type);
	ASATOM_DECREF_POINTER(pval);
	pval->setBool(ret);
	++(context->exec_pos);
}
void ABCVm::abc_istype(call_context* context)
{
	//istype
	uint32_t t = (++(context->exec_pos))->data;
	multiname* name=context->mi->context->getMultiname(t,NULL);

	RUNTIME_STACK_POINTER_CREATE(context,pval);

	pval->setBool(isType(context->mi->context,pval->toObject(context->mi->context->root->getSystemState()),name));
	++(context->exec_pos);
}
void ABCVm::abc_istypelate(call_context* context)
{
	//istypelate
	RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v1, context->mi->context->root->getSystemState());
	RUNTIME_STACK_POINTER_CREATE(context,pval);

	pval->setBool(isTypelate(v1, pval->toObject(context->mi->context->root->getSystemState())));
	++(context->exec_pos);
}
void ABCVm::abc_in(call_context* context)
{
	//in
	RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v1, context->mi->context->root->getSystemState());
	RUNTIME_STACK_POINTER_CREATE(context,pval);

	LOG_CALL( _("in") );
	if(v1->is<Null>())
		throwError<TypeError>(kConvertNullToObjectError);

	multiname name(NULL);
	pval->fillMultiname(context->mi->context->root->getSystemState(),name);
	name.ns.emplace_back(context->mi->context->root->getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
	bool ret=v1->hasPropertyByMultiname(name, true, true);
	ASATOM_DECREF_POINTER(pval);
	v1->decRef();
	pval->setBool(ret);

	++(context->exec_pos);
}
void ABCVm::abc_increment_i(call_context* context)
{
	//increment_i
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	LOG_CALL("increment_i:"<<pval->toDebugString());
	pval->increment_i();
	++(context->exec_pos);
}
void ABCVm::abc_decrement_i(call_context* context)
{
	//decrement_i
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	LOG_CALL("decrement_i:"<<pval->toDebugString());
	pval->decrement_i();
	++(context->exec_pos);
}
void ABCVm::abc_inclocal_i(call_context* context)
{
	//inclocal_i
	uint32_t t = (++(context->exec_pos))->data;
	incLocal_i(context, t);
	++(context->exec_pos);
}
void ABCVm::abc_declocal_i(call_context* context)
{
	//declocal_i
	uint32_t t = (++(context->exec_pos))->data;
	decLocal_i(context, t);
	++(context->exec_pos);
}
void ABCVm::abc_negate_i(call_context* context)
{
	//negate_i
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	pval->negate_i();
	++(context->exec_pos);
}
void ABCVm::abc_add_i(call_context* context)
{
	//add_i
	RUNTIME_STACK_POP_CREATE(context,v2);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	pval->add_i(*v2);
	++(context->exec_pos);
}
void ABCVm::abc_subtract_i(call_context* context)
{
	//subtract_i
	RUNTIME_STACK_POP_CREATE(context,v2);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	pval->subtract_i(*v2);
	++(context->exec_pos);
}
void ABCVm::abc_multiply_i(call_context* context)
{
	//multiply_i
	RUNTIME_STACK_POP_CREATE(context,v2);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	pval->multiply_i(*v2);
	++(context->exec_pos);
}
void ABCVm::abc_getlocal_0(call_context* context)
{
	//getlocal_0
	int i=0;
	LOG_CALL( _("getLocal ") << i << _(": ") << context->locals[i].toDebugString() );
	ASATOM_INCREF(context->locals[i]);
	RUNTIME_STACK_PUSH(context,context->locals[i]);
	++(context->exec_pos);
}
void ABCVm::abc_getlocal_1(call_context* context)
{
	//getlocal_1
	int i=1;
	LOG_CALL( _("getLocal ") << i << _(": ") << context->locals[i].toDebugString() );
	ASATOM_INCREF(context->locals[i]);
	RUNTIME_STACK_PUSH(context,context->locals[i]);
	++(context->exec_pos);
}
void ABCVm::abc_getlocal_2(call_context* context)
{
	//getlocal_2
	int i=2;
	LOG_CALL( _("getLocal ") << i << _(": ") << context->locals[i].toDebugString() );
	ASATOM_INCREF(context->locals[i]);
	RUNTIME_STACK_PUSH(context,context->locals[i]);
	++(context->exec_pos);
}
void ABCVm::abc_getlocal_3(call_context* context)
{
	//getlocal_3
	int i=3;
	LOG_CALL( _("getLocal ") << i << _(": ") << context->locals[i].toDebugString() );
	ASATOM_INCREF(context->locals[i]);
	RUNTIME_STACK_PUSH(context,context->locals[i]);
	++(context->exec_pos);
}
void ABCVm::abc_setlocal_0(call_context* context)
{
	//setlocal_0
	unsigned int i=0;
	LOG_CALL( _("setLocal ") << i);
	RUNTIME_STACK_POP_CREATE(context,obj)
	if (i >= context->locals_size)
	{
		LOG(LOG_ERROR,"abc_setlocal invalid index:"<<i);
		return;
	}
	if ((int)i != context->argarrayposition || obj->type == T_ARRAY)
	{
		if (context->locals[i].getObject() != obj->getObject())
		{
			ASATOM_DECREF(context->locals[i]);
		}
		context->locals[i]=*obj;
	}
	++(context->exec_pos);
}
void ABCVm::abc_setlocal_1(call_context* context)
{
	//setlocal_1
	unsigned int i=1;
	LOG_CALL( _("setLocal ") << i);
	++(context->exec_pos);

	RUNTIME_STACK_POP_CREATE(context,obj)
	if (i >= context->locals_size)
	{
		LOG(LOG_ERROR,"abc_setlocal invalid index:"<<i);
		return;
	}
	if ((int)i != context->argarrayposition || obj->type == T_ARRAY)
	{
		if (context->locals[i].getObject() != obj->getObject())
		{
			ASATOM_DECREF(context->locals[i]);
		}
		context->locals[i]=*obj;
	}
}
void ABCVm::abc_setlocal_2(call_context* context)
{
	//setlocal_2
	unsigned int i=2;
	++(context->exec_pos);
	LOG_CALL( _("setLocal ") << i);
	RUNTIME_STACK_POP_CREATE(context,obj)
	if (i >= context->locals_size)
	{
		LOG(LOG_ERROR,"abc_setlocal invalid index:"<<i);
		return;
	}
	if ((int)i != context->argarrayposition || obj->type == T_ARRAY)
	{
		if (context->locals[i].getObject() != obj->getObject())
		{
			ASATOM_DECREF(context->locals[i]);
		}
		context->locals[i]=*obj;
	}
}
void ABCVm::abc_setlocal_3(call_context* context)
{
	//setlocal_3
	unsigned int i=3;
	++(context->exec_pos);
	LOG_CALL( _("setLocal ") << i);
	RUNTIME_STACK_POP_CREATE(context,obj)
	if (i >= context->locals_size)
	{
		LOG(LOG_ERROR,"abc_setlocal invalid index:"<<i);
		return;
	}
	if ((int)i != context->argarrayposition || obj->type == T_ARRAY)
	{
		if (context->locals[i].getObject() != obj->getObject())
		{
			ASATOM_DECREF(context->locals[i]);
		}
		context->locals[i]=*obj;
	}
}
void ABCVm::abc_debug(call_context* context)
{
	//debug
	LOG_CALL( _("debug") );
	++(context->exec_pos);
	++(context->exec_pos);
	++(context->exec_pos);
	++(context->exec_pos);
	++(context->exec_pos);
}
void ABCVm::abc_debugline(call_context* context)
{
	//debugline
	LOG_CALL( _("debugline") );
	++(context->exec_pos);
	++(context->exec_pos);
}
void ABCVm::abc_debugfile(call_context* context)
{
	//debugfile
	LOG_CALL( _("debugfile") );
	++(context->exec_pos);
	++(context->exec_pos);
}
void ABCVm::abc_bkptline(call_context* context)
{
	//bkptline
	LOG_CALL( _("bkptline") );
	++(context->exec_pos);
	++(context->exec_pos);
}
void ABCVm::abc_timestamp(call_context* context)
{
	//timestamp
	LOG_CALL( _("timestamp") );
	++(context->exec_pos);
}
void ABCVm::abc_invalidinstruction(call_context* context)
{
	LOG(LOG_ERROR,"invalid instruction " << hex << (context->exec_pos)->data << dec);
	throw ParseException("Not implemented instruction in interpreter");
}

struct operands
{
	OPERANDTYPES type;
	int32_t index;
	uint32_t codecount;
	uint32_t preloadedcodepos;
	operands(OPERANDTYPES _t,int32_t _i,uint32_t _c, uint32_t _p):type(_t),index(_i),codecount(_c),preloadedcodepos(_p) {}
	void removeArg(method_info* mi)
	{
		if (codecount)
			mi->body->preloadedcode.erase(mi->body->preloadedcode.begin()+preloadedcodepos,mi->body->preloadedcode.begin()+preloadedcodepos+codecount);
	}
	void fillCode(int pos, preloadedcodedata& code, ABCContext* context, bool switchopcode)
	{
		switch (type)
		{
			case OP_LOCAL:
				switch (pos)
				{
					case 0:
						code.local_pos1 = index;
						break;
					case 1:
						code.local_pos2 = index;
						break;
				}
				if (switchopcode)
					code.data+=1+pos; // increase opcode
				break;
			default:
				switch (pos)
				{
					case 0:
						code.arg1_constant = context->getConstantAtom(type,index);
						break;
					case 1:
						code.arg2_constant = context->getConstantAtom(type,index);
						break;
				}
				break;
		}
	}
};
#define ABC_OP_OPTIMZED_INCREMENT 0x00000100
#define ABC_OP_OPTIMZED_DECREMENT 0x00000102
#define ABC_OP_OPTIMZED_PUSHSCOPE 0x00000104
#define ABC_OP_OPTIMZED_GETPROPERTY_STATICNAME 0x00000106
#define ABC_OP_OPTIMZED_GETLEX 0x0000010a
#define ABC_OP_OPTIMZED_CALLPROPERTY_STATICNAME_LOCALRESULT 0x0000010b
#define ABC_OP_OPTIMZED_MULTIPLY 0x00000120
#define ABC_OP_OPTIMZED_BITOR 0x00000128
#define ABC_OP_OPTIMZED_BITXOR 0x00000130
#define ABC_OP_OPTIMZED_SUBTRACT 0x00000138
#define ABC_OP_OPTIMZED_ADD 0x00000140
#define ABC_OP_OPTIMZED_DIVIDE 0x00000148
#define ABC_OP_OPTIMZED_MODULO 0x00000150
#define ABC_OP_OPTIMZED_LSHIFT 0x00000158
#define ABC_OP_OPTIMZED_RSHIFT 0x00000160
#define ABC_OP_OPTIMZED_URSHIFT 0x00000168
#define ABC_OP_OPTIMZED_BITAND 0x00000170
#define ABC_OP_OPTIMZED_GETPROPERTY 0x00000178
#define ABC_OP_OPTIMZED_IFEQ 0x00000180
#define ABC_OP_OPTIMZED_IFNE 0x00000184
#define ABC_OP_OPTIMZED_IFLT 0x00000188
#define ABC_OP_OPTIMZED_IFLE 0x0000018C
#define ABC_OP_OPTIMZED_IFGT 0x00000190
#define ABC_OP_OPTIMZED_IFGE 0x00000194
#define ABC_OP_OPTIMZED_IFSTRICTEQ 0x00000198
#define ABC_OP_OPTIMZED_IFSTRICTNE 0x0000019c
#define ABC_OP_OPTIMZED_CALLPROPERTY_STATICNAME 0x000001a0
#define ABC_OP_OPTIMZED_GREATERTHAN 0x000001a8
#define ABC_OP_OPTIMZED_IFTRUE 0x000001b0
#define ABC_OP_OPTIMZED_IFFALSE 0x000001b2
#define ABC_OP_OPTIMZED_CONVERTD 0x000001b4
#define ABC_OP_OPTIMZED_RETURNVALUE 0x000001b8

bool checkForLocalResult(std::list<operands>& operandlist,method_info* mi,memorystream& code,std::map<int32_t,int32_t>& oldnewpositions,std::set<int32_t>& jumptargets,uint32_t opcode_jumpspace)
{
	bool res = false;
	bool needstwoargs = false;
	uint32_t resultpos=0;
	uint32_t pos = code.tellg()+1;
	uint8_t b = code.peekbyte();
	if (jumptargets.find(pos) == jumptargets.end())
	{
		// check if the next opcode can be skipped
		switch (b)
		{
			case 0x25://pushshort
			case 0x2c://pushstring
			case 0x2d://pushint
			case 0x2e://pushuint
			case 0x2f://pushdouble
			case 0x31://pushnamespace
			case 0x62://getlocal
				pos = code.skipu30FromPosition(pos);
				b = code.peekbyteFromPosition(pos);
				pos++;
				needstwoargs=true;
				break;
			case 0x60://getlex
				pos = code.skipu30FromPosition(pos);
				b = code.peekbyteFromPosition(pos);
				pos++;
				needstwoargs=true;
				resultpos=1;
				break;
			case 0x20://pushnull
			case 0x2a://dup
			case 0xd0://getlocal_0
			case 0xd1://getlocal_1
			case 0xd2://getlocal_2
			case 0xd3://getlocal_3
				b = code.peekbyteFromPosition(pos);
				pos++;
				needstwoargs=true;
				break;
			case 0x24://pushbyte
				pos++;
				b = code.peekbyteFromPosition(pos);
				pos++;
				needstwoargs=true;
				break;
			case 0x82://coerce_a
				code.readbyte();
				b = code.peekbyteFromPosition(pos);
				pos++;
				break;
			default:
				break;
		}
	}
	if (b == 0x10) // jump
	{
		int32_t j = code.peeks24FromPosition(pos);
		uint32_t p = pos+3;//3 bytes from s24;
		bool hastargets = j < 0;
		if (!hastargets)
		{
			// check if the code following the jump is unreachable
			for (int32_t i = 0; i < j; i++)
			{
				if (jumptargets.find(p+i+1) != jumptargets.end())
				{
					hastargets = true;
					break;
				}
			}
		}
		if (!hastargets)
		{
			// skip unreachable code
			pos = p+j; 
			jumptargets.erase(p+j+1);
			b = code.peekbyteFromPosition(pos);
			if (!needstwoargs)
			{
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				code.seekg(pos);
				oldnewpositions[code.tellg()+1] = (int32_t)mi->body->preloadedcode.size();
			}
			pos++;
		}
	}
	// check if we need to store the result of the operation on stack
	switch (b)
	{
		case 0x63://setlocal
		{
			if (!needstwoargs && (jumptargets.find(code.tellg()+1) == jumptargets.end()))
			{
				code.readbyte();
				uint32_t num = code.readu30();
				// set optimized opcode to corresponding opcode with local result 
				mi->body->preloadedcode[mi->body->preloadedcode.size()-1].data += opcode_jumpspace;
				mi->body->preloadedcode[mi->body->preloadedcode.size()-1].local_pos3 = num+1;
				operandlist.push_back(operands(OP_LOCAL,num,0,0));
				res = true;
			}
			break;
		}
		case 0xd4: //setlocal_0
		case 0xd5: //setlocal_1
		case 0xd6: //setlocal_2
		case 0xd7: //setlocal_3
			if (!needstwoargs && (jumptargets.find(code.tellg()+1) == jumptargets.end()))
			{
				// set optimized opcode to corresponding opcode with local result 
				mi->body->preloadedcode[mi->body->preloadedcode.size()-1].data += opcode_jumpspace;
				mi->body->preloadedcode[mi->body->preloadedcode.size()-1].local_pos3 = b-0xd3;
				code.readbyte();
				operandlist.push_back(operands(OP_LOCAL,b-0xd4,0,0));
				res = true;
			}
			break;
		case 0x46://callproperty
		{
			uint32_t t = code.peeku30FromPosition(pos);
			pos = code.skipu30FromPosition(pos);
			uint32_t argcount = code.peeku30FromPosition(pos);
			if (jumptargets.find(pos) == jumptargets.end() 
					&& argcount == 1 
					&& mi->context->constant_pool.multinames[t].runtimeargs == 0
					&& (needstwoargs || (operandlist.size() >= 1)))
			{
				// set optimized opcode to corresponding opcode with local result 
				mi->body->preloadedcode[mi->body->preloadedcode.size()-1].data += opcode_jumpspace;
				mi->body->preloadedcode[mi->body->preloadedcode.size()-1].local_pos3 = mi->body->local_count+2+resultpos;
				operandlist.push_back(operands(OP_LOCAL,mi->body->local_count+1+resultpos,0,0));
				res = true;
			}
			else
				operandlist.clear();
			break;
		}
		case 0x66://getproperty
		{
			uint32_t t = code.peeku30FromPosition(pos);
			uint32_t argcount = needstwoargs ? 1 : 0;
			if (jumptargets.find(pos) == jumptargets.end() && 
					((uint32_t)mi->context->constant_pool.multinames[t].runtimeargs == argcount
					|| (!needstwoargs && (operandlist.size() >= 1) && mi->context->constant_pool.multinames[t].runtimeargs == 1)
					))
			{
				// set optimized opcode to corresponding opcode with local result 
				mi->body->preloadedcode[mi->body->preloadedcode.size()-1].data += opcode_jumpspace;
				mi->body->preloadedcode[mi->body->preloadedcode.size()-1].local_pos3 = mi->body->local_count+2+resultpos;
				operandlist.push_back(operands(OP_LOCAL,mi->body->local_count+1+resultpos,0,0));
				res = true;
			}
			else
				operandlist.clear();
			break;
		}
		case 0x91://increment
		case 0x93://decrement
		case 0x75://convert_d
			if (!needstwoargs && (operandlist.size() > 0) && (jumptargets.find(code.tellg()+1) == jumptargets.end()))
			{
				// set optimized opcode to corresponding opcode with local result 
				mi->body->preloadedcode[mi->body->preloadedcode.size()-1].data += opcode_jumpspace;
				mi->body->preloadedcode[mi->body->preloadedcode.size()-1].local_pos3 = mi->body->local_count+2+resultpos;
				operandlist.push_back(operands(OP_LOCAL,mi->body->local_count+1+resultpos,0,0));
				res = true;
			}
			else
				operandlist.clear();
			break;
		case 0x13://ifeq
		case 0x14://ifne
		case 0x15://iflt
		case 0x16://ifle
		case 0x17://ifgt
		case 0x18://ifge
		case 0x19://ifstricteq
		case 0x1a://ifstrictne
		case 0x48://returnvalue
		case 0xa0://add
		case 0xa1://subtract
		case 0xa2://multiply
		case 0xa3://divide
		case 0xa4://modulo
		case 0xa5://lshift
		case 0xa6://rshift
		case 0xa7://urshift
		case 0xa8://bitand
		case 0xa9://bitor
		case 0xaa://bitxor
		case 0xaf://greaterthan
			if ((needstwoargs || operandlist.size() > 0) && (jumptargets.find(code.tellg()+1) == jumptargets.end()))
			{
				// set optimized opcode to corresponding opcode with local result 
				mi->body->preloadedcode[mi->body->preloadedcode.size()-1].data += opcode_jumpspace;
				mi->body->preloadedcode[mi->body->preloadedcode.size()-1].local_pos3 = mi->body->local_count+2+resultpos;
				operandlist.push_back(operands(OP_LOCAL,mi->body->local_count+1+resultpos,0,0));
				res = true;
			}
			break;
		case 0x10://jump
			// don't clear operandlist yet, because the jump may be skipped
			break;
		default:
			operandlist.clear();
			break;
	}
	return res;
}

void setupInstructionOneArgumentNoResult(std::list<operands>& operandlist,method_info* mi,int operator_start,int opcode,memorystream& code,std::map<int32_t,int32_t>& oldnewpositions,std::set<int32_t>& jumptargets)
{
	bool hasoperands = jumptargets.find(code.tellg()) == jumptargets.end() && operandlist.size() >= 1;
	if (hasoperands)
	{
		auto it = operandlist.end();
		(--it)->removeArg(mi);// remove arg1
		it = operandlist.end();
		mi->body->preloadedcode.push_back(operator_start);
		(--it)->fillCode(0,mi->body->preloadedcode[mi->body->preloadedcode.size()-1],mi->context,true);
		oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
		operandlist.pop_back();
	}
	else
	{
		mi->body->preloadedcode.push_back((uint32_t)opcode);
		oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
	}
}
void setupInstructionTwoArgumentsNoResult(std::list<operands>& operandlist,method_info* mi,int operator_start,int opcode,memorystream& code,std::map<int32_t,int32_t>& oldnewpositions,std::set<int32_t>& jumptargets)
{
	bool hasoperands = operandlist.size() >= 2;
	if (hasoperands)
	{
		auto it = operandlist.end();
		(--it)->removeArg(mi);// remove arg2
		(--it)->removeArg(mi);// remove arg1
		it = operandlist.end();
		// optimized opcodes are in order CONSTANT/CONSTANT, LOCAL/CONSTANT, CONSTANT/LOCAL, LOCAL/LOCAL
		mi->body->preloadedcode.push_back(operator_start);
		(--it)->fillCode(1,mi->body->preloadedcode[mi->body->preloadedcode.size()-1],mi->context,true);
		(--it)->fillCode(0,mi->body->preloadedcode[mi->body->preloadedcode.size()-1],mi->context,true);
		oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
		operandlist.pop_back();
		operandlist.pop_back();
	}
	else
	{
		mi->body->preloadedcode.push_back((uint32_t)opcode);
		oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
	}
}
void setupInstructionOneArgument(std::list<operands>& operandlist,method_info* mi,int operator_start,int opcode,memorystream& code,std::map<int32_t,int32_t>& oldnewpositions,std::set<int32_t>& jumptargets,bool constantsallowed)
{
	bool hasoperands = jumptargets.find(code.tellg()) == jumptargets.end() && operandlist.size() >= 1 && (constantsallowed || operandlist.back().type == OP_LOCAL);
	if (hasoperands)
	{
		auto it = operandlist.end();
		(--it)->removeArg(mi);// remove arg1
		it = operandlist.end();
		mi->body->preloadedcode.push_back(operator_start);
		(--it)->fillCode(0,mi->body->preloadedcode[mi->body->preloadedcode.size()-1],mi->context,constantsallowed);
		oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
		operandlist.pop_back();
	}
	else
		mi->body->preloadedcode.push_back((uint32_t)opcode);
	if (jumptargets.find(code.tellg()+1) == jumptargets.end())
	{
		switch (code.peekbyte())
		{
			case 0x73://convert_i
			case 0x74://convert_u
			case 0x75://convert_d
				if (!constantsallowed)
				{
					oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
					code.readbyte();
				}
				break;
			case 0x82://coerce_a
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				code.readbyte();
				break;
		}
	}
	if (hasoperands)
		checkForLocalResult(operandlist,mi,code,oldnewpositions,jumptargets,constantsallowed ? 2 : 1);
	else
		operandlist.clear();
}

bool setupInstructionTwoArguments(std::list<operands>& operandlist,method_info* mi,int operator_start,int opcode,memorystream& code,std::map<int32_t,int32_t>& oldnewpositions,std::set<int32_t>& jumptargets, bool skip_conversion)
{
	bool hasoperands = jumptargets.find(code.tellg()) == jumptargets.end() && operandlist.size() >= 2;
	if (hasoperands)
	{
		auto it = operandlist.end();
		(--it)->removeArg(mi);// remove arg2
		(--it)->removeArg(mi);// remove arg1
		it = operandlist.end();
		// optimized opcodes are in order CONSTANT/CONSTANT, LOCAL/CONSTANT, CONSTANT/LOCAL, LOCAL/LOCAL
		mi->body->preloadedcode.push_back(operator_start);
		(--it)->fillCode(1,mi->body->preloadedcode[mi->body->preloadedcode.size()-1],mi->context,true);
		(--it)->fillCode(0,mi->body->preloadedcode[mi->body->preloadedcode.size()-1],mi->context,true);
		oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
		operandlist.pop_back();
		operandlist.pop_back();
	}
	else
		mi->body->preloadedcode.push_back((uint32_t)opcode);
	if (jumptargets.find(code.tellg()+1) == jumptargets.end())
	{
		switch (code.peekbyte())
		{
			case 0x75://convert_d
				if (skip_conversion)
				{
					oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
					code.readbyte();
				}
				break;
			case 0x82://coerce_a
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				code.readbyte();
				break;
		}
	}
	if (hasoperands)
		checkForLocalResult(operandlist,mi,code,oldnewpositions,jumptargets,4);
	else
		operandlist.clear();
	return hasoperands;
}

void ABCVm::preloadFunction(const SyntheticFunction* function)
{
	method_info* mi=function->mi;

	const int code_len=mi->body->code.size();
	std::map<int32_t,int32_t> oldnewpositions;
	std::map<int32_t,int32_t> jumppositions;
	std::map<int32_t,int32_t> jumpstartpositions;
	std::map<int32_t,int32_t> switchpositions;
	std::map<int32_t,int32_t> switchstartpositions;

	// first pass: store all jump target points
	std::set<int32_t> jumptargets;

	auto itex = mi->body->exceptions.begin();
	while (itex != mi->body->exceptions.end())
	{
		// add exception jump targets
		jumptargets.insert((int32_t)itex->target+1);
		itex++;
	}

	memorystream codejumps(mi->body->code.data(), code_len);
	while(!codejumps.atend())
	{
		uint8_t opcode = codejumps.readbyte();
		switch(opcode)
		{
			case 0x04://getsuper
			case 0x05://setsuper
			case 0x06://dxns
			case 0x08://kill
			case 0x2c://pushstring
			case 0x2d://pushint
			case 0x2e://pushuint
			case 0x2f://pushdouble
			case 0x31://pushnamespace
			case 0x40://newfunction
			case 0x41://call
			case 0x42://construct
			case 0x49://constructsuper
			case 0x53://constructgenerictype
			case 0x55://newobject
			case 0x56://newarray
			case 0x58://newclass
			case 0x59://getdescendants
			case 0x5a://newcatch
			case 0x5d://findpropstrict
			case 0x5e://findproperty
			case 0x5f://finddef
			case 0x60://getlex
			case 0x61://setproperty
			case 0x62://getlocal
			case 0x63://setlocal
			case 0x65://getscopeobject
			case 0x66://getproperty
			case 0x68://initproperty
			case 0x6a://deleteproperty
			case 0x6c://getslot
			case 0x6d://setslot
			case 0x6e://getglobalSlot
			case 0x6f://setglobalSlot
			case 0x80://coerce
			case 0x86://astype
			case 0x92://inclocal
			case 0x94://declocal
			case 0xb2://istype
			case 0xc2://inclocal_i
			case 0xc3://declocal_i
			case 0xf0://debugline
			case 0xf1://debugfile
			case 0xf2://bkptline
				codejumps.readu30();
				break;
			case 0x0c://ifnlt
			case 0x0d://ifnle
			case 0x0e://ifngt
			case 0x0f://ifnge
			case 0x10://jump
			case 0x11://iftrue
			case 0x12://iffalse
			case 0x13://ifeq
			case 0x14://ifne
			case 0x15://iflt
			case 0x16://ifle
			case 0x17://ifgt
			case 0x18://ifge
			case 0x19://ifstricteq
			case 0x1a://ifstrictne
				jumptargets.insert(codejumps.reads24()+codejumps.tellg()+1);
				break;
			case 0x1b://lookupswitch
			{
				int32_t p = codejumps.tellg()-1;
				jumptargets.insert(p+codejumps.reads24());
				uint32_t count = codejumps.readu30();
				for(unsigned int i=0;i<count+1;i++)
				{
					jumptargets.insert(p+codejumps.reads24());
				}
				break;
			}
			case 0x24://pushbyte
			{
				codejumps.readbyte();
				break;
			}
			case 0x25://pushshort
			{
				codejumps.readu32();
				break;
			}
			case 0x32://hasnext2
			case 0x43://callmethod
			case 0x44://callstatic
			case 0x45://callsuper
			case 0x46://callproperty
			case 0x4c://callproplex
			case 0x4a://constructprop
			case 0x4e://callsupervoid
			case 0x4f://callpropvoid
			{
				codejumps.readu30();
				codejumps.readu30();
				break;
			}
			case 0xef://debug
			{
				codejumps.readbyte();
				codejumps.readu30();
				codejumps.readbyte();
				codejumps.readu30();
				break;
			}
		}
	}
	
	// second pass: use optimized opcode version if it doesn't interfere with a jump target
	std::list<operands> operandlist;
	memorystream code(mi->body->code.data(), code_len);
	while(!code.atend())
	{
		oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
		uint8_t opcode = code.readbyte();
		//LOG(LOG_INFO,"preload opcode:"<<code.tellg()-1<<" "<<hex<<(int)opcode);


		switch(opcode)
		{
			case 0x04://getsuper
			case 0x05://setsuper
			case 0x06://dxns
			case 0x08://kill
			case 0x40://newfunction
			case 0x41://call
			case 0x42://construct
			case 0x49://constructsuper
			case 0x53://constructgenerictype
			case 0x55://newobject
			case 0x56://newarray
			case 0x58://newclass
			case 0x59://getdescendants
			case 0x5a://newcatch
			case 0x5d://findpropstrict
			case 0x5e://findproperty
			case 0x5f://finddef
			case 0x61://setproperty
			case 0x65://getscopeobject
			case 0x68://initproperty
			case 0x6a://deleteproperty
			case 0x6c://getslot
			case 0x6d://setslot
			case 0x6e://getglobalSlot
			case 0x6f://setglobalSlot
			case 0x80://coerce
			case 0x86://astype
			case 0x92://inclocal
			case 0x94://declocal
			case 0xb2://istype
			case 0xc2://inclocal_i
			case 0xc3://declocal_i
			{
				mi->body->preloadedcode.push_back((uint32_t)opcode);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				mi->body->preloadedcode.push_back(code.readu30());
				operandlist.clear();
				break;
			}
			case 0x60://getlex
			{
				int32_t p = code.tellg();
				if (jumptargets.find(p) != jumptargets.end())
					operandlist.clear();
				uint32_t t =code.readu30();
				mi->body->preloadedcode.push_back(ABC_OP_OPTIMZED_GETLEX);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				if (!checkForLocalResult(operandlist,mi,code,oldnewpositions,jumptargets,0))
				{
					// no local result possible, use standard operation
					mi->body->preloadedcode[mi->body->preloadedcode.size()-1].data=(uint32_t)opcode;
					operandlist.clear();
				}
				mi->body->preloadedcode.push_back(t);
				break;
			}
			case 0x62://getlocal
			{
				int32_t p = code.tellg();
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size()+1;
				int32_t value =code.readu30();
				uint32_t num = value<<9 | opcode;
				mi->body->preloadedcode.push_back(num);
				if (jumptargets.find(p) != jumptargets.end())
					operandlist.clear();
				operandlist.push_back(operands(OP_LOCAL,value,1,mi->body->preloadedcode.size()-1));
				break;
			}
			case 0x63://setlocal
			{
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size()+1;
				uint32_t num = code.readu30()<<9 | opcode;
				mi->body->preloadedcode.push_back(num);
				operandlist.clear();
				break;
			}
			case 0xd0://getlocal_0
			case 0xd1://getlocal_1
			case 0xd2://getlocal_2
			case 0xd3://getlocal_3
			{
				int32_t p = code.tellg();
				mi->body->preloadedcode.push_back((uint32_t)opcode);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				if (jumptargets.find(p) != jumptargets.end())
					operandlist.clear();
				operandlist.push_back(operands(OP_LOCAL,((uint32_t)opcode)-0xd0,1,mi->body->preloadedcode.size()-1));
				break;
			}
			case 0x2a://dup
			{
				int32_t p = code.tellg();
				mi->body->preloadedcode.push_back((uint32_t)opcode);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				if (jumptargets.find(p) != jumptargets.end())
					operandlist.clear();
				else if (operandlist.size() > 0)
				{
					operands op = operandlist.back();
					operandlist.push_back(operands(op.type,op.index,1,mi->body->preloadedcode.size()-1));
				}
				break;
			}
			case 0x01://bkpt
			case 0x02://nop
			case 0x82://coerce_a
			{
				//skip
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				break;
			}
			case 0xf0://debugline
			case 0xf1://debugfile
			case 0xf2://bkptline
			{
				//skip
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				code.readu30();
				break;
			}
			case 0x0c://ifnlt
			case 0x0d://ifnle
			case 0x0e://ifngt
			case 0x0f://ifnge
			{
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				jumppositions[mi->body->preloadedcode.size()] = code.reads24();
				jumpstartpositions[mi->body->preloadedcode.size()] = code.tellg();
				mi->body->preloadedcode.push_back((uint32_t)opcode);
				operandlist.clear();
				break;
			}
			case 0x10://jump
			{
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				int32_t j = code.reads24();
				uint32_t p = code.tellg();
				bool hastargets = j < 0;
				if (!hastargets)
				{
					// check if the code following the jump is unreachable
					for (int32_t i = 0; i <= j; i++)
					{
						if (jumptargets.find(p+i) != jumptargets.end())
						{
							hastargets = true;
							break;
						}
					}
				}
				if (hastargets)
				{
					jumppositions[mi->body->preloadedcode.size()] = j;
					jumpstartpositions[mi->body->preloadedcode.size()] = code.tellg();
					mi->body->preloadedcode.push_back((uint32_t)opcode);
					operandlist.clear();
				}
				else
				{
					// skip unreachable code
					for (int32_t i = 0; i < j; i++)
						code.readbyte();
					jumptargets.erase(code.tellg()+1);
					oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				}
				break;
			}
			case 0x11://iftrue
			{
				int32_t p = code.tellg();
				if (jumptargets.find(p) == jumptargets.end())
					setupInstructionOneArgumentNoResult(operandlist,mi,ABC_OP_OPTIMZED_IFTRUE,opcode,code,oldnewpositions, jumptargets);
				else
					mi->body->preloadedcode.push_back((uint32_t)opcode);
				jumppositions[mi->body->preloadedcode.size()-1] = code.reads24();
				jumpstartpositions[mi->body->preloadedcode.size()-1] = code.tellg();
				operandlist.clear();
				break;
			}
			case 0x12://iffalse
			{
				int32_t p = code.tellg();
				if (jumptargets.find(p) == jumptargets.end())
					setupInstructionOneArgumentNoResult(operandlist,mi,ABC_OP_OPTIMZED_IFFALSE,opcode,code,oldnewpositions, jumptargets);
				else
					mi->body->preloadedcode.push_back((uint32_t)opcode);
				jumppositions[mi->body->preloadedcode.size()-1] = code.reads24();
				jumpstartpositions[mi->body->preloadedcode.size()-1] = code.tellg();
				operandlist.clear();
				break;
			}
			case 0x13://ifeq
				setupInstructionTwoArgumentsNoResult(operandlist,mi,ABC_OP_OPTIMZED_IFEQ,opcode,code,oldnewpositions, jumptargets);
				jumppositions[mi->body->preloadedcode.size()-1] = code.reads24();
				jumpstartpositions[mi->body->preloadedcode.size()-1] = code.tellg();
				operandlist.clear();
				break;
			case 0x14://ifne
				setupInstructionTwoArgumentsNoResult(operandlist,mi,ABC_OP_OPTIMZED_IFNE,opcode,code,oldnewpositions, jumptargets);
				jumppositions[mi->body->preloadedcode.size()-1] = code.reads24();
				jumpstartpositions[mi->body->preloadedcode.size()-1] = code.tellg();
				operandlist.clear();
				break;
			case 0x15://iflt
				setupInstructionTwoArgumentsNoResult(operandlist,mi,ABC_OP_OPTIMZED_IFLT,opcode,code,oldnewpositions, jumptargets);
				jumppositions[mi->body->preloadedcode.size()-1] = code.reads24();
				jumpstartpositions[mi->body->preloadedcode.size()-1] = code.tellg();
				operandlist.clear();
				break;
			case 0x16://ifle
				setupInstructionTwoArgumentsNoResult(operandlist,mi,ABC_OP_OPTIMZED_IFLE,opcode,code,oldnewpositions, jumptargets);
				jumppositions[mi->body->preloadedcode.size()-1] = code.reads24();
				jumpstartpositions[mi->body->preloadedcode.size()-1] = code.tellg();
				operandlist.clear();
				break;
			case 0x17://ifgt
				setupInstructionTwoArgumentsNoResult(operandlist,mi,ABC_OP_OPTIMZED_IFGT,opcode,code,oldnewpositions, jumptargets);
				jumppositions[mi->body->preloadedcode.size()-1] = code.reads24();
				jumpstartpositions[mi->body->preloadedcode.size()-1] = code.tellg();
				operandlist.clear();
				break;
			case 0x18://ifge
				setupInstructionTwoArgumentsNoResult(operandlist,mi,ABC_OP_OPTIMZED_IFGE,opcode,code,oldnewpositions, jumptargets);
				jumppositions[mi->body->preloadedcode.size()-1] = code.reads24();
				jumpstartpositions[mi->body->preloadedcode.size()-1] = code.tellg();
				operandlist.clear();
				break;
			case 0x19://ifstricteq
				setupInstructionTwoArgumentsNoResult(operandlist,mi,ABC_OP_OPTIMZED_IFSTRICTEQ,opcode,code,oldnewpositions, jumptargets);
				jumppositions[mi->body->preloadedcode.size()-1] = code.reads24();
				jumpstartpositions[mi->body->preloadedcode.size()-1] = code.tellg();
				operandlist.clear();
				break;
			case 0x1a://ifstrictne
				setupInstructionTwoArgumentsNoResult(operandlist,mi,ABC_OP_OPTIMZED_IFSTRICTNE,opcode,code,oldnewpositions, jumptargets);
				jumppositions[mi->body->preloadedcode.size()-1] = code.reads24();
				jumpstartpositions[mi->body->preloadedcode.size()-1] = code.tellg();
				operandlist.clear();
				break;
			case 0x1b://lookupswitch
			{
				mi->body->preloadedcode.push_back((uint32_t)opcode);
				int32_t p = code.tellg()-1;
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				switchpositions[mi->body->preloadedcode.size()] = code.reads24();
				switchstartpositions[mi->body->preloadedcode.size()] = p;
				mi->body->preloadedcode.push_back(0);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				uint32_t count = code.readu30();
				mi->body->preloadedcode.push_back(count);
				for(unsigned int i=0;i<count+1;i++)
				{
					oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
					switchpositions[mi->body->preloadedcode.size()] = code.reads24();
					switchstartpositions[mi->body->preloadedcode.size()] = p;
					mi->body->preloadedcode.push_back(0);
				}
				operandlist.clear();
				break;
			}
			case 0x20://pushnull
				mi->body->preloadedcode.push_back((uint32_t)opcode);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				if (jumptargets.find(code.tellg()) == jumptargets.end())
					operandlist.push_back(operands(OP_NULL,0,1,mi->body->preloadedcode.size()-1));
				else
					operandlist.clear();
				break;
			case 0x24://pushbyte
			{
				int32_t p = code.tellg();
				mi->body->preloadedcode.push_back((uint32_t)opcode);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				int32_t value = (int32_t)(int8_t)code.readbyte();
				uint8_t index = value;
				mi->body->preloadedcode.push_back(value);
				if (jumptargets.find(p) == jumptargets.end())
					operandlist.push_back(operands(OP_BYTE,index,2,mi->body->preloadedcode.size()-2));
				else
					operandlist.clear();
				break;
			}
			case 0x25://pushshort
			{
				int32_t p = code.tellg();
				mi->body->preloadedcode.push_back((uint32_t)opcode);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				int32_t value = code.readu32();
				uint16_t index = value;
				mi->body->preloadedcode.push_back(value);
				if (jumptargets.find(p) == jumptargets.end())
					operandlist.push_back(operands(OP_SHORT,index,2,mi->body->preloadedcode.size()-2));
				else
					operandlist.clear();
				break;
			}
			case 0x2c://pushstring
			{
				int32_t p = code.tellg();
				mi->body->preloadedcode.push_back((uint32_t)opcode);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				int32_t value = code.readu30();
				mi->body->preloadedcode.push_back(value);
				if (jumptargets.find(p) == jumptargets.end())
					operandlist.push_back(operands(OP_STRING,value,2,mi->body->preloadedcode.size()-2));
				else
					operandlist.clear();
				break;
			}
			case 0x2d://pushint
			{
				int32_t p = code.tellg();
				mi->body->preloadedcode.push_back((uint32_t)opcode);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				int32_t value = code.readu30();
				mi->body->preloadedcode.push_back(value);
				if (jumptargets.find(p) == jumptargets.end())
					operandlist.push_back(operands(OP_INTEGER,value,2,mi->body->preloadedcode.size()-2));
				else
					operandlist.clear();
				break;
			}
			case 0x2e://pushuint
			{
				int32_t p = code.tellg();
				mi->body->preloadedcode.push_back((uint32_t)opcode);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				int32_t value = code.readu30();
				mi->body->preloadedcode.push_back(value);
				if (jumptargets.find(p) == jumptargets.end())
					operandlist.push_back(operands(OP_UINTEGER,value,2,mi->body->preloadedcode.size()-2));
				else
					operandlist.clear();
				break;
			}
			case 0x2f://pushdouble
			{
				int32_t p = code.tellg();
				mi->body->preloadedcode.push_back((uint32_t)opcode);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				int32_t value = code.readu30();
				mi->body->preloadedcode.push_back(value);
				if (jumptargets.find(p) == jumptargets.end())
					operandlist.push_back(operands(OP_DOUBLE,value,2,mi->body->preloadedcode.size()-2));
				else
					operandlist.clear();
				break;
			}
			case 0x30://pushscope
				setupInstructionOneArgumentNoResult(operandlist,mi,ABC_OP_OPTIMZED_PUSHSCOPE,opcode,code,oldnewpositions, jumptargets);
				break;
			case 0x31://pushnamespace
			{
				int32_t p = code.tellg();
				mi->body->preloadedcode.push_back((uint32_t)opcode);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				int32_t value = code.readu30();
				mi->body->preloadedcode.push_back(value);
				if (jumptargets.find(p) == jumptargets.end())
					operandlist.push_back(operands(OP_NAMESPACE,value,2,mi->body->preloadedcode.size()-2));
				else
					operandlist.clear();
				break;
			}
			case 0x32://hasnext2
			case 0x43://callmethod
			case 0x44://callstatic
			case 0x45://callsuper
			case 0x4c://callproplex
			case 0x4a://constructprop
			case 0x4e://callsupervoid
			case 0x4f://callpropvoid
			{
				mi->body->preloadedcode.push_back((uint32_t)opcode);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				mi->body->preloadedcode.push_back(code.readu30());
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				mi->body->preloadedcode.push_back(code.readu30());
				operandlist.clear();
				break;
			}
			case 0xef://debug
			{
				// skip all debug messages
				code.readbyte();
				code.readu30();
				code.readbyte();
				code.readu30();
				break;
			}
			case 0x48://returnvalue
			{
				int32_t p = code.tellg();
				if (jumptargets.find(p) != jumptargets.end())
					operandlist.clear();
				setupInstructionOneArgumentNoResult(operandlist,mi,ABC_OP_OPTIMZED_RETURNVALUE,opcode,code,oldnewpositions, jumptargets);
				break;
			}
			case 0x46://callproperty
			{
				int32_t p = code.tellg();
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size()+1;
				uint32_t t = code.readu30();
				assert_and_throw(t < mi->context->constant_pool.multiname_count);
				uint32_t argcount = code.readu30();
				// TODO optimize non-static multinames and argcount != 1
				if (jumptargets.find(p) == jumptargets.end() && argcount == 1)
				{
					switch (mi->context->constant_pool.multinames[t].runtimeargs)
					{
						case 0:
							if (setupInstructionTwoArguments(operandlist,mi,ABC_OP_OPTIMZED_CALLPROPERTY_STATICNAME,opcode,code,oldnewpositions, jumptargets,false))
							{
								mi->body->preloadedcode.push_back(t);
								mi->body->preloadedcode.at(mi->body->preloadedcode.size()-1).cachedmultiname2 = mi->context->getMultinameImpl(asAtom::nullAtom,NULL,t,false);
							}
							else if (checkForLocalResult(operandlist,mi,code,oldnewpositions,jumptargets,0))
							{
								mi->body->preloadedcode.at(mi->body->preloadedcode.size()-1).data=ABC_OP_OPTIMZED_CALLPROPERTY_STATICNAME_LOCALRESULT;
								mi->body->preloadedcode.push_back(t);
								mi->body->preloadedcode.at(mi->body->preloadedcode.size()-1).cachedmultiname2 = mi->context->getMultinameImpl(asAtom::nullAtom,NULL,t,false);
							}
							else
							{
								operandlist.clear();
								mi->body->preloadedcode.push_back(t);
								mi->body->preloadedcode.push_back(argcount);
							}
							break;
						default:
							mi->body->preloadedcode.push_back((uint32_t)opcode);
							operandlist.clear();
							mi->body->preloadedcode.push_back(t);
							mi->body->preloadedcode.push_back(argcount);
							break;
					}
				}
				else
				{
					mi->body->preloadedcode.push_back((uint32_t)opcode);
					operandlist.clear();
					mi->body->preloadedcode.push_back(t);
					mi->body->preloadedcode.push_back(argcount);
				}
				break;
			}
			case 0x66://getproperty
			{
				int32_t p = code.tellg();
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size()+1;
				uint32_t t = code.readu30();
				assert_and_throw(t < mi->context->constant_pool.multiname_count);
				if (jumptargets.find(p) == jumptargets.end())
				{
					switch (mi->context->constant_pool.multinames[t].runtimeargs)
					{
						case 0:
							setupInstructionOneArgument(operandlist,mi,ABC_OP_OPTIMZED_GETPROPERTY_STATICNAME,opcode,code,oldnewpositions, jumptargets,true);
							mi->body->preloadedcode.at(mi->body->preloadedcode.size()-1).cachedmultiname2 = mi->context->getMultinameImpl(asAtom::nullAtom,NULL,t,false);
							break;
						case 1:
							setupInstructionTwoArguments(operandlist,mi,ABC_OP_OPTIMZED_GETPROPERTY,opcode,code,oldnewpositions, jumptargets,false);
							break;
						default:
							mi->body->preloadedcode.push_back((uint32_t)opcode);
							operandlist.clear();
							break;
					}
				}
				else
				{
					mi->body->preloadedcode.push_back((uint32_t)opcode);
					operandlist.clear();
				}
				mi->body->preloadedcode.push_back(t);
				break;
			}
			case 0x75://convert_d
				setupInstructionOneArgument(operandlist,mi,ABC_OP_OPTIMZED_CONVERTD,opcode,code,oldnewpositions, jumptargets,true);
				break;
			case 0x91://increment
				setupInstructionOneArgument(operandlist,mi,ABC_OP_OPTIMZED_INCREMENT,opcode,code,oldnewpositions, jumptargets,false);
				break;
			case 0x93://decrement
				setupInstructionOneArgument(operandlist,mi,ABC_OP_OPTIMZED_DECREMENT,opcode,code,oldnewpositions, jumptargets,false);
				break;
			case 0xa0://add
				setupInstructionTwoArguments(operandlist,mi,ABC_OP_OPTIMZED_ADD,opcode,code,oldnewpositions, jumptargets,false);
				break;
			case 0xa1://subtract
				setupInstructionTwoArguments(operandlist,mi,ABC_OP_OPTIMZED_SUBTRACT,opcode,code,oldnewpositions, jumptargets,true);
				break;
			case 0xa2://multiply
				setupInstructionTwoArguments(operandlist,mi,ABC_OP_OPTIMZED_MULTIPLY,opcode,code,oldnewpositions, jumptargets,true);
				break;
			case 0xa3://divide
				setupInstructionTwoArguments(operandlist,mi,ABC_OP_OPTIMZED_DIVIDE,opcode,code,oldnewpositions, jumptargets,true);
				break;
			case 0xa4://modulo
				setupInstructionTwoArguments(operandlist,mi,ABC_OP_OPTIMZED_MODULO,opcode,code,oldnewpositions, jumptargets,true);
				break;
			case 0xa5://lshift
				setupInstructionTwoArguments(operandlist,mi,ABC_OP_OPTIMZED_LSHIFT,opcode,code,oldnewpositions, jumptargets,true);
				break;
			case 0xa6://rshift
				setupInstructionTwoArguments(operandlist,mi,ABC_OP_OPTIMZED_RSHIFT,opcode,code,oldnewpositions, jumptargets,true);
				break;
			case 0xa7://urshift
				setupInstructionTwoArguments(operandlist,mi,ABC_OP_OPTIMZED_URSHIFT,opcode,code,oldnewpositions, jumptargets,true);
				break;
			case 0xa8://bitand
				setupInstructionTwoArguments(operandlist,mi,ABC_OP_OPTIMZED_BITAND,opcode,code,oldnewpositions, jumptargets,true);
				break;
			case 0xa9://bitor
				setupInstructionTwoArguments(operandlist,mi,ABC_OP_OPTIMZED_BITOR,opcode,code,oldnewpositions, jumptargets,true);
				break;
			case 0xaa://bitxor
				setupInstructionTwoArguments(operandlist,mi,ABC_OP_OPTIMZED_BITXOR,opcode,code,oldnewpositions, jumptargets,true);
				break;
			case 0xaf://greaterthan
				setupInstructionTwoArguments(operandlist,mi,ABC_OP_OPTIMZED_GREATERTHAN,opcode,code,oldnewpositions, jumptargets,false);
				break;
			default:
			{
				if (abcfunctions[opcode] == abc_invalidinstruction)
				{
					char stropcode[10];
					sprintf(stropcode,"%d",opcode);
					char strcodepos[20];
					sprintf(strcodepos,"%d",code.tellg());
					throwError<VerifyError>(kIllegalOpcodeError,function->getSystemState()->getStringFromUniqueId(function->functionname),stropcode,strcodepos);
				}
				mi->body->preloadedcode.push_back((uint32_t)opcode);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				operandlist.clear();
				break;
			}
		}
	}
	oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();

	// adjust jump positions to new code vector;
	auto itj = jumppositions.begin();
	while (itj != jumppositions.end())
	{
		uint32_t p = jumpstartpositions[itj->first];
		assert (oldnewpositions.find(p) != oldnewpositions.end());
		if (oldnewpositions.find(p+itj->second) == oldnewpositions.end())
		{
			LOG(LOG_ERROR,"preloadfunction: jump position not found:"<<p<<" "<<itj->second);
			mi->body->preloadedcode[itj->first].jumpdata.jump = 0;
		}
		else
			mi->body->preloadedcode[itj->first].jumpdata.jump = (oldnewpositions[p+itj->second]-(oldnewpositions[p]));
		itj++;
	}
	// adjust switch positions to new code vector;
	auto its = switchpositions.begin();
	while (its != switchpositions.end())
	{
		uint32_t p = switchstartpositions[its->first];
		assert (oldnewpositions.find(p) != oldnewpositions.end());
		assert (oldnewpositions.find(p+its->second) != oldnewpositions.end());
		mi->body->preloadedcode[its->first] = oldnewpositions[p+its->second]-(oldnewpositions[p]);
		its++;
	}
	auto itexc = mi->body->exceptions.begin();
	while (itexc != mi->body->exceptions.end())
	{
		assert (oldnewpositions.find(itexc->from) != oldnewpositions.end());
		itexc->from = oldnewpositions[itexc->from];
		assert (oldnewpositions.find(itexc->to) != oldnewpositions.end());
		itexc->to = oldnewpositions[itexc->to];
		assert (oldnewpositions.find(itexc->target) != oldnewpositions.end());
		itexc->target = oldnewpositions[itexc->target];
		itexc++;
	}
}

