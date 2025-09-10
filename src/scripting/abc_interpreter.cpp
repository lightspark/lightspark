/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2013  Alessandro Pignotti (a.pignotti@sssup.it)
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
#include "compat.h"
#include "exceptions.h"
#include "scripting/abc_interpreter_helper.h"
#include "scripting/abc_optimized_add.h"
#include "scripting/abc_optimized_bitnot.h"
#include "scripting/abc_optimized_callproperty.h"
#include "scripting/abc_optimized_coerce_s.h"
#include "scripting/abc_optimized_convert_s.h"
#include "scripting/abc_optimized_functionbuiltin.h"
#include "scripting/abc_optimized_functionsynthetic.h"
#include "scripting/abc_optimized_getproperty.h"
#include "scripting/abc_optimized_getscopeobject.h"
#include "scripting/abc_optimized_getslot.h"
#include "scripting/abc_optimized_incdec.h"
#include "scripting/abc_optimized_negate.h"
#include "scripting/abc_optimized_newcatch.h"
#include "scripting/abc_optimized_setproperty.h"
#include "scripting/abc_optimized_setslot.h"
#include "scripting/class.h"
#include "scripting/toplevel/toplevel.h"
#include "scripting/toplevel/ASString.h"
#include "scripting/toplevel/Global.h"
#include "scripting/toplevel/Namespace.h"
#include "scripting/toplevel/Number.h"
#include "scripting/toplevel/Boolean.h"
#include "scripting/toplevel/Integer.h"
#include "scripting/toplevel/UInteger.h"
#include "scripting/toplevel/RegExp.h"
#include "scripting/toplevel/Vector.h"
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

#ifndef NDEBUG
std::map<abc_function,uint32_t> opcodecounter;
void ABCVm::dumpOpcodeCounters(uint32_t threshhold)
{
	auto it = opcodecounter.begin();
	while (it != opcodecounter.end())
	{
		if (it->second > threshhold)
		{
			for (uint32_t i = 0; i < 0x31c; i++)
			{
				if (abcfunctions[i] == it->first)
					LOG(LOG_INFO,"opcode counter:"<<hex<<i<<":"<<dec<<it->second);
			}
		}
		it++;
	}
}
void ABCVm::clearOpcodeCounters()
{
	opcodecounter.clear();
}
#endif

void ABCVm::executeFunction(call_context* context)
{
#ifdef PROFILING_SUPPORT
	if(context->mi->profTime.empty())
		context->mi->profTime.resize(context->mi->body->preloadedcode.size(),0);
	uint64_t startTime=compat_get_thread_cputime_us();
#define PROF_ACCOUNT_TIME(a, b)  do{a+=b;}while(0)
#define PROF_IGNORE_TIME(a) do{ a; } while(0)
#else
#define PROF_ACCOUNT_TIME(a, b) do{ ; }while(0)
#define PROF_IGNORE_TIME(a) do{ ; } while(0)
#endif

	asAtom* ret = &context->locals[context->mi->body->getReturnValuePos()];
	while(asAtomHandler::isInvalid(*ret) && !context->exceptionthrown)
	{
#ifdef PROFILING_SUPPORT
		uint32_t instructionPointer=context->exec_pos- &context->mi->body->preloadedcode.front();
#endif
		//LOG_CALL("stack:"<<(context->stackp-context->stack)<<" code position:"<<(context->exec_pos- &context->mi->body->preloadedcode.front()));

#ifndef NDEBUG
		uint32_t c = opcodecounter[context->exec_pos->func];
		opcodecounter[context->exec_pos->func] = c+1;
#endif
		// context->exec_pos points to the current instruction, every abc_function has to make sure
		// it points to the next valid instruction after execution
		context->exec_pos->func(context);
		PROF_ACCOUNT_TIME(context->mi->profTime[instructionPointer],profilingCheckpoint(startTime));
	}
	return;

#undef PROF_ACCOUNT_TIME
#undef PROF_IGNORE_TIME
}

abc_function ABCVm::abcfunctions[]={
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
	abc_coerce_o,
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

	// instructions for optimized opcodes (indicated by 0x03xx)
	abc_increment_constant, // 0x100 ABC_OP_OPTIMZED_INCREMENT
	abc_increment_local,
	abc_increment_constant_localresult,
	abc_increment_local_localresult,
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
	abc_setPropertyStaticName_constant_constant, // 0x11c ABC_OP_OPTIMZED_SETPROPERTY_STATICNAME
	abc_setPropertyStaticName_local_constant,
	abc_setPropertyStaticName_constant_local,
	abc_setPropertyStaticName_local_local,

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
	abc_pushcachedconstant,// 0x1ba ABC_OP_OPTIMZED_PUSHCACHEDCONSTANT
	abc_getlexfromslot,// 0x1bb ABC_OP_OPTIMZED_GETLEX_FROMSLOT
	abc_getlexfromslot_localresult,
	abc_callpropertyStaticName,// 0x1bd ABC_OP_OPTIMZED_CALLPROPERTY_STATICNAME_MULTIARGS
	abc_callpropvoidStaticName,// 0x1be ABC_OP_OPTIMZED_CALLPROPVOID_STATICNAME_MULTIARGS
	abc_getPropertyStaticName_localresult, // 0x1bf ABC_OP_OPTIMZED_GETPROPERTY_STATICNAME_LOCALRESULT

	abc_lessequals_constant_constant,// 0x1c0 ABC_OP_OPTIMZED_LESSEQUALS
	abc_lessequals_local_constant,
	abc_lessequals_constant_local,
	abc_lessequals_local_local,
	abc_lessequals_constant_constant_localresult,
	abc_lessequals_local_constant_localresult,
	abc_lessequals_constant_local_localresult,
	abc_lessequals_local_local_localresult,
	abc_greaterequals_constant_constant,// 0x1c8 ABC_OP_OPTIMZED_GREATEREQUALS
	abc_greaterequals_local_constant,
	abc_greaterequals_constant_local,
	abc_greaterequals_local_local,
	abc_greaterequals_constant_constant_localresult,
	abc_greaterequals_local_constant_localresult,
	abc_greaterequals_constant_local_localresult,
	abc_greaterequals_local_local_localresult,
	abc_equals_constant_constant,// 0x1d0 ABC_OP_OPTIMZED_EQUALS
	abc_equals_local_constant,
	abc_equals_constant_local,
	abc_equals_local_local,
	abc_equals_constant_constant_localresult,
	abc_equals_local_constant_localresult,
	abc_equals_constant_local_localresult,
	abc_equals_local_local_localresult,
	abc_not_constant,// 0x1d8 ABC_OP_OPTIMZED_NOT
	abc_not_local,
	abc_not_constant_localresult,
	abc_not_local_localresult,
	abc_iftrue_dup_constant,// 0x1dc ABC_OP_OPTIMZED_IFTRUE_DUP
	abc_iftrue_dup_local,
	abc_iffalse_dup_constant,// 0x1de ABC_OP_OPTIMZED_IFFALSE_DUP
	abc_iffalse_dup_local,
	abc_callpropertyStaticName_constant,// 0x1e0 ABC_OP_OPTIMZED_CALLPROPERTY_STATICNAME_NOARGS
	abc_callpropertyStaticName_local,
	abc_callpropertyStaticName_constant_localresult,
	abc_callpropertyStaticName_local_localresult,
	abc_callpropvoidStaticName_constant_constant,// 0x1e4 ABC_OP_OPTIMZED_CALLPROPVOID_STATICNAME
	abc_callpropvoidStaticName_local_constant,
	abc_callpropvoidStaticName_constant_local,
	abc_callpropvoidStaticName_local_local,
	abc_callpropvoidStaticName_constant,// 0x1e8 ABC_OP_OPTIMZED_CALLPROPVOID_STATICNAME_NOARGS
	abc_callpropvoidStaticName_local,
	abc_constructsuper_constant,// 0x1ea ABC_OP_OPTIMZED_CONSTRUCTSUPER
	abc_constructsuper_local,
	abc_getslot_constant,// 0x1ec ABC_OP_OPTIMZED_GETSLOT
	abc_getslot_local,
	abc_getslot_constant_localresult,
	abc_getslot_local_localresult,
	abc_callFunctionNoArgs_constant, // 0x1f0 ABC_OP_OPTIMZED_CALLFUNCTION_NOARGS
	abc_callFunctionNoArgs_local,
	abc_callFunctionNoArgs_constant_localresult,
	abc_callFunctionNoArgs_local_localresult,
	abc_callFunctionSyntheticOneArgVoid_constant_constant, // 0x1f4 ABC_OP_OPTIMZED_CALLFUNCTIONSYNTHETIC_ONEARG_VOID
	abc_callFunctionSyntheticOneArgVoid_local_constant,
	abc_callFunctionSyntheticOneArgVoid_constant_local,
	abc_callFunctionSyntheticOneArgVoid_local_local,
	abc_increment_i_local, // 0x1f8 ABC_OP_OPTIMZED_INCREMENT_I
	abc_increment_i_local_localresult,
	abc_decrement_i_local, // 0x1fa ABC_OP_OPTIMZED_DECREMENT_I
	abc_decrement_i_local_localresult,
	abc_getslot_constant_setslotnocoerce, // 0x1fc ABC_OP_OPTIMZED_GETSLOT_SETSLOT
	abc_getslot_local_setslotnocoerce,
	abc_getscopeobject_localresult, // 0x1fe ABC_OP_OPTIMZED_GETSCOPEOBJECT_LOCALRESULT
	abc_callpropvoidStaticNameCached,// 0x1ff ABC_OP_OPTIMZED_CALLPROPVOID_STATICNAME_MULTIARGS_CACHED

	abc_li8_constant,// 0x200 ABC_OP_OPTIMZED_LI8
	abc_li8_local,
	abc_li8_constant_localresult,
	abc_li8_local_localresult,
	abc_li16_constant,// 0x204 ABC_OP_OPTIMZED_LI16
	abc_li16_local,
	abc_li16_constant_localresult,
	abc_li16_local_localresult,
	abc_li32_constant,// 0x208 ABC_OP_OPTIMZED_LI32
	abc_li32_local,
	abc_li32_constant_localresult,
	abc_li32_local_localresult,
	abc_ifnlt, //0x20c  jump data may have the 0x200 bit set, so we also add the jump opcodes here
	abc_ifnle,
	abc_ifngt,
	abc_ifnge,

	abc_jump, // 0x210
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
	abc_lf32_constant,// 0x21c ABC_OP_OPTIMZED_LF32
	abc_lf32_local,
	abc_lf32_constant_localresult,
	abc_lf32_local_localresult,
	abc_lf64_constant,// 0x220 ABC_OP_OPTIMZED_LF64
	abc_lf64_local,
	abc_lf64_constant_localresult,
	abc_lf64_local_localresult,
	abc_si8_constant_constant,// 0x224 ABC_OP_OPTIMZED_SI8
	abc_si8_local_constant,
	abc_si8_constant_local,
	abc_si8_local_local,
	abc_si16_constant_constant,// 0x228 ABC_OP_OPTIMZED_SI16
	abc_si16_local_constant,
	abc_si16_constant_local,
	abc_si16_local_local,
	abc_si32_constant_constant,// 0x22c ABC_OP_OPTIMZED_SI32
	abc_si32_local_constant,
	abc_si32_constant_local,
	abc_si32_local_local,
	abc_sf32_constant_constant,// 0x230 ABC_OP_OPTIMZED_SF32
	abc_sf32_local_constant,
	abc_sf32_constant_local,
	abc_sf32_local_local,
	abc_sf64_constant_constant,// 0x234 ABC_OP_OPTIMZED_SF64
	abc_sf64_local_constant,
	abc_sf64_constant_local,
	abc_sf64_local_local,

	abc_setslot_constant_constant, // 0x238 ABC_OP_OPTIMZED_SETSLOT
	abc_setslot_local_constant,
	abc_setslot_constant_local,
	abc_setslot_local_local,
	abc_convert_i_constant,// 0x23c ABC_OP_OPTIMZED_CONVERTI
	abc_convert_i_local,
	abc_convert_i_constant_localresult,
	abc_convert_i_local_localresult,

	abc_convert_u_constant,// 0x240 ABC_OP_OPTIMZED_CONVERTU
	abc_convert_u_local,
	abc_convert_u_constant_localresult,
	abc_convert_u_local_localresult,
	abc_constructpropStaticName_constant,// 0x244 ABC_OP_OPTIMZED_CONSTRUCTPROP_STATICNAME_NOARGS
	abc_constructpropStaticName_local,
	abc_constructpropStaticName_constant_localresult,
	abc_constructpropStaticName_local_localresult,
	abc_convert_b_constant,// 0x248 ABC_OP_OPTIMZED_CONVERTB
	abc_convert_b_local,
	abc_convert_b_constant_localresult,
	abc_convert_b_local_localresult,
	abc_construct_constant,// 0x24c ABC_OP_OPTIMZED_CONSTRUCT_NOARGS
	abc_construct_local,
	abc_construct_constant_localresult,
	abc_construct_local_localresult,

	abc_setslotNoCoerce_constant_constant, // 0x250 ABC_OP_OPTIMZED_SETSLOT_NOCOERCE
	abc_setslotNoCoerce_local_constant,
	abc_setslotNoCoerce_constant_local,
	abc_setslotNoCoerce_local_local,
	abc_callFunctionNoArgsVoid_constant, // 0x254 ABC_OP_OPTIMZED_CALLFUNCTION_NOARGS_VOID
	abc_callFunctionNoArgsVoid_local,
	abc_li8_constant_setslotnocoerce,// 0x256 ABC_OP_OPTIMZED_LI8_SETSLOT
	abc_li8_local_setslotnocoerce,
	abc_lessthan_constant_constant,// 0x258 ABC_OP_OPTIMZED_LESSTHAN
	abc_lessthan_local_constant,
	abc_lessthan_constant_local,
	abc_lessthan_local_local,
	abc_lessthan_constant_constant_localresult,
	abc_lessthan_local_constant_localresult,
	abc_lessthan_constant_local_localresult,
	abc_lessthan_local_local_localresult,

	abc_add_i_constant_constant, // 0x260 ABC_OP_OPTIMZED_ADD_I
	abc_add_i_local_constant,
	abc_add_i_constant_local,
	abc_add_i_local_local,
	abc_add_i_constant_constant_localresult,
	abc_add_i_local_constant_localresult,
	abc_add_i_constant_local_localresult,
	abc_add_i_local_local_localresult,
	abc_typeof_constant,// 0x268 ABC_OP_OPTIMZED_TYPEOF
	abc_typeof_local,
	abc_typeof_constant_localresult,
	abc_typeof_local_localresult,
	abc_callFunctionBuiltinOneArgVoid_constant_constant, // 0x26c ABC_OP_OPTIMZED_CALLFUNCTIONBUILTIN_ONEARG_VOID
	abc_callFunctionBuiltinOneArgVoid_local_constant,
	abc_callFunctionBuiltinOneArgVoid_constant_local,
	abc_callFunctionBuiltinOneArgVoid_local_local,

	abc_callFunctionBuiltinMultiArgsVoid_constant, // 0x279 ABC_OP_OPTIMZED_CALLFUNCTIONBUILTIN_MULTIARGS_VOID
	abc_callFunctionBuiltinMultiArgsVoid_local,
	abc_callFunctionSyntheticMultiArgsVoid_constant, // 0x272 ABC_OP_OPTIMZED_CALLFUNCTIONSYNTHETIC_MULTIARGS_VOID
	abc_callFunctionSyntheticMultiArgsVoid_local,
	abc_callFunctionBuiltinMultiArgs_constant, // 0x274 ABC_OP_OPTIMZED_CALLFUNCTIONBUILTIN_MULTIARGS
	abc_callFunctionBuiltinMultiArgs_local,
	abc_callFunctionBuiltinMultiArgs_constant_localResult,
	abc_callFunctionBuiltinMultiArgs_local_localResult,
	abc_callFunctionSyntheticMultiArgs_constant, // 0x278 ABC_OP_OPTIMZED_CALLFUNCTIONSYNTHETIC_MULTIARGS
	abc_callFunctionSyntheticMultiArgs_local,
	abc_callFunctionSyntheticMultiArgs_constant_localResult,
	abc_callFunctionSyntheticMultiArgs_local_localResult,
	abc_callpropertyStaticNameCached_constant,// 0x27c ABC_OP_OPTIMZED_CALLPROPERTY_STATICNAME_MULTIARGS_CACHED_CALLER
	abc_callpropertyStaticNameCached_local,
	abc_callpropertyStaticNameCached_constant_localResult,
	abc_callpropertyStaticNameCached_local_localResult,

	abc_callpropertyStaticNameCached,// 0x280 ABC_OP_OPTIMZED_CALLPROPERTY_STATICNAME_MULTIARGS_CACHED
	abc_callpropertyStaticNameCached_localResult,
	abc_callpropvoidStaticNameCached_constant,// 0x282 ABC_OP_OPTIMZED_CALLPROPVOID_STATICNAME_MULTIARGS_CACHED_CALLER
	abc_callpropvoidStaticNameCached_local,
	abc_setPropertyStaticName, // 0x284 ABC_OP_OPTIMZED_SETPROPERTY_STATICNAME_SIMPLE
	abc_getPropertyInteger, // 0x285 ABC_OP_OPTIMZED_GETPROPERTY_INTEGER_SIMPLE
	abc_setPropertyInteger, // 0x286 ABC_OP_OPTIMZED_SETPROPERTY_INTEGER_SIMPLE
	abc_pushcachedslot,// 0x287 ABC_OP_OPTIMZED_PUSHCACHEDSLOT
	abc_getPropertyInteger_constant_constant, // 0x288 ABC_OP_OPTIMZED_GETPROPERTY_INTEGER
	abc_getPropertyInteger_local_constant,
	abc_getPropertyInteger_constant_local,
	abc_getPropertyInteger_local_local,
	abc_getPropertyInteger_constant_constant_localresult,
	abc_getPropertyInteger_local_constant_localresult,
	abc_getPropertyInteger_constant_local_localresult,
	abc_getPropertyInteger_local_local_localresult,

	abc_setPropertyInteger_constant_constant_constant, // 0x290 ABC_OP_OPTIMZED_SETPROPERTY_INTEGER
	abc_setPropertyInteger_constant_local_constant,
	abc_setPropertyInteger_constant_constant_local,
	abc_setPropertyInteger_constant_local_local,
	abc_setPropertyInteger_local_constant_constant,
	abc_setPropertyInteger_local_local_constant,
	abc_setPropertyInteger_local_constant_local,
	abc_setPropertyInteger_local_local_local,
	abc_ifnlt_constant_constant, // 0x298 ABC_OP_OPTIMZED_IFNLT
	abc_ifnlt_local_constant,
	abc_ifnlt_constant_local,
	abc_ifnlt_local_local,
	abc_ifnge_constant_constant, // 0x29c ABC_OP_OPTIMZED_IFNGE
	abc_ifnge_local_constant,
	abc_ifnge_constant_local,
	abc_ifnge_local_local,

	abc_setlocal_constant, // 0x2a0 ABC_OP_OPTIMZED_SETLOCAL
	abc_setlocal_local,
	abc_callFunctionSyntheticMultiArgs, // 0x2a2 ABC_OP_OPTIMZED_CALLFUNCTIONSYNTHETIC_STATICNAME_MULTIARGS
	abc_callFunctionSyntheticMultiArgsVoid, // 0x2a3 ABC_OP_OPTIMZED_CALLFUNCTIONVOIDSYNTHETIC_STATICNAME_MULTIARGS
	abc_inclocal_i_optimized, // 0x2a4 ABC_OP_OPTIMZED_INCLOCAL_I
	abc_declocal_i_optimized, // 0x2a5 ABC_OP_OPTIMZED_DECLOCAL_I
	abc_dup_local, // 0x2a6 ABC_OP_OPTIMZED_DUP
	abc_dup_local_localresult,
	abc_dup_increment_local_localresult, // 0x2a8 ABC_OP_OPTIMZED_DUP_INCDEC
	abc_dup_decrement_local_localresult,
	abc_dup_increment_i_local_localresult,
	abc_dup_decrement_i_local_localresult,
	abc_add_i_constant_constant_setslotnocoerce, // 0x2ac ABC_OP_OPTIMZED_ADD_I_SETSLOT
	abc_add_i_local_constant_setslotnocoerce,
	abc_add_i_constant_local_setslotnocoerce,
	abc_add_i_local_local_setslotnocoerce,

	abc_setPropertyIntegerVector_constant_constant_constant, // 0x2b0 ABC_OP_OPTIMZED_SETPROPERTY_INTEGER_VECTOR
	abc_setPropertyIntegerVector_constant_local_constant,
	abc_setPropertyIntegerVector_constant_constant_local,
	abc_setPropertyIntegerVector_constant_local_local,
	abc_setPropertyIntegerVector_local_constant_constant,
	abc_setPropertyIntegerVector_local_local_constant,
	abc_setPropertyIntegerVector_local_constant_local,
	abc_setPropertyIntegerVector_local_local_local,
	abc_istypelate_constant_constant,// 0x2b8 ABC_OP_OPTIMZED_ISTYPELATE
	abc_istypelate_local_constant,
	abc_istypelate_constant_local,
	abc_istypelate_local_local,
	abc_istypelate_constant_constant_localresult,
	abc_istypelate_local_constant_localresult,
	abc_istypelate_constant_local_localresult,
	abc_istypelate_local_local_localresult,

	abc_astypelate_constant_constant,// 0x2c0 ABC_OP_OPTIMZED_ASTYPELATE
	abc_astypelate_local_constant,
	abc_astypelate_constant_local,
	abc_astypelate_local_local,
	abc_astypelate_constant_constant_localresult,
	abc_astypelate_local_constant_localresult,
	abc_astypelate_constant_local_localresult,
	abc_astypelate_local_local_localresult,
	abc_li16_constant_setslotnocoerce,// 0x2c8 ABC_OP_OPTIMZED_LI16_SETSLOT
	abc_li16_local_setslotnocoerce,
	abc_li32_constant_setslotnocoerce,// 0x2ca ABC_OP_OPTIMZED_LI32_SETSLOT
	abc_li32_local_setslotnocoerce,
	abc_lshift_constant_constant_setslotnocoerce,// 0x2cc ABC_OP_OPTIMZED_LSHIFT_SETSLOT
	abc_lshift_local_constant_setslotnocoerce,
	abc_lshift_constant_local_setslotnocoerce,
	abc_lshift_local_local_setslotnocoerce,

	abc_rshift_constant_constant_setslotnocoerce,// 0x2d0 ABC_OP_OPTIMZED_RSHIFT_SETSLOT
	abc_rshift_local_constant_setslotnocoerce,
	abc_rshift_constant_local_setslotnocoerce,
	abc_rshift_local_local_setslotnocoerce,
	abc_add_constant_constant_setslotnocoerce,// 0x2d4 ABC_OP_OPTIMZED_ADD_SETSLOT
	abc_add_local_constant_setslotnocoerce,
	abc_add_constant_local_setslotnocoerce,
	abc_add_local_local_setslotnocoerce,
	abc_subtract_constant_constant_setslotnocoerce,// 0x2d8 ABC_OP_OPTIMZED_SUBTRACT_SETSLOT
	abc_subtract_local_constant_setslotnocoerce,
	abc_subtract_constant_local_setslotnocoerce,
	abc_subtract_local_local_setslotnocoerce,
	abc_multiply_constant_constant_setslotnocoerce,// 0x2dc ABC_OP_OPTIMZED_MULTIPLY_SETSLOT
	abc_multiply_local_constant_setslotnocoerce,
	abc_multiply_constant_local_setslotnocoerce,
	abc_multiply_local_local_setslotnocoerce,

	abc_divide_constant_constant_setslotnocoerce,// 0x2e0 ABC_OP_OPTIMZED_DIVIDE_SETSLOT
	abc_divide_local_constant_setslotnocoerce,
	abc_divide_constant_local_setslotnocoerce,
	abc_divide_local_local_setslotnocoerce,
	abc_modulo_constant_constant_setslotnocoerce,// 0x2e4 ABC_OP_OPTIMZED_MODULO_SETSLOT
	abc_modulo_local_constant_setslotnocoerce,
	abc_modulo_constant_local_setslotnocoerce,
	abc_modulo_local_local_setslotnocoerce,
	abc_urshift_constant_constant_setslotnocoerce,// 0x2e8 ABC_OP_OPTIMZED_URSHIFT_SETSLOT
	abc_urshift_local_constant_setslotnocoerce,
	abc_urshift_constant_local_setslotnocoerce,
	abc_urshift_local_local_setslotnocoerce,
	abc_bitand_constant_constant_setslotnocoerce,// 0x2ec ABC_OP_OPTIMZED_BITAND_SETSLOT
	abc_bitand_local_constant_setslotnocoerce,
	abc_bitand_constant_local_setslotnocoerce,
	abc_bitand_local_local_setslotnocoerce,

	abc_bitor_constant_constant_setslotnocoerce,// 0x2f0 ABC_OP_OPTIMZED_BITOR_SETSLOT
	abc_bitor_local_constant_setslotnocoerce,
	abc_bitor_constant_local_setslotnocoerce,
	abc_bitor_local_local_setslotnocoerce,
	abc_bitxor_constant_constant_setslotnocoerce,// 0x2f4 ABC_OP_OPTIMZED_BITXOR_SETSLOT
	abc_bitxor_local_constant_setslotnocoerce,
	abc_bitxor_constant_local_setslotnocoerce,
	abc_bitxor_local_local_setslotnocoerce,
	abc_convert_i_constant_setslotnocoerce,// 0x2f8 ABC_OP_OPTIMZED_CONVERTI_SETSLOT
	abc_convert_i_local_setslotnocoerce,
	abc_convert_u_constant_setslotnocoerce,// 0x2fa ABC_OP_OPTIMZED_CONVERTU_SETSLOT
	abc_convert_u_local_setslotnocoerce,
	abc_convert_d_constant_setslotnocoerce,// 0x2fc ABC_OP_OPTIMZED_CONVERTD_SETSLOT
	abc_convert_d_local_setslotnocoerce,
	abc_convert_b_constant_setslotnocoerce,// 0x2fe ABC_OP_OPTIMZED_CONVERTB_SETSLOT
	abc_convert_b_local_setslotnocoerce,

	abc_lf32_constant_setslotnocoerce,// 0x300 ABC_OP_OPTIMZED_LF32_SETSLOT
	abc_lf32_local_setslotnocoerce,
	abc_lf64_constant_setslotnocoerce,// 0x302 ABC_OP_OPTIMZED_LF64_SETSLOT
	abc_lf64_local_setslotnocoerce,
	abc_callFunctionNoArgs_constant_setslotnocoerce,// 0x304 ABC_OP_OPTIMZED_CALLFUNCTION_NOARGS_SETSLOT
	abc_callFunctionNoArgs_local_setslotnocoerce,
	abc_increment_i_local_setslotnocoerce, // 0x306 ABC_OP_OPTIMZED_INCREMENT_I_SETSLOT
	abc_decrement_i_local_setslotnocoerce, // 0x307 ABC_OP_OPTIMZED_DECREMENT_I_SETSLOT
	abc_astypelate_constant_constant_setslotnocoerce,// 0x308 ABC_OP_OPTIMZED_ASTYPELATE_SETSLOT
	abc_astypelate_local_constant_setslotnocoerce,
	abc_astypelate_constant_local_setslotnocoerce,
	abc_astypelate_local_local_setslotnocoerce,
	abc_ifnlt, //0x30c  jump data may have the 0x200 bit set, so we also add the jump opcodes here
	abc_ifnle,
	abc_ifngt,
	abc_ifnge,

	abc_jump, // 0x310
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

	abc_dup_local_setslotnocoerce, // 0x31c ABC_OP_OPTIMZED_DUP_SETSLOT
	abc_newobject_noargs_localresult,
	abc_getfuncscopeobject, // 0x31e ABC_OP_OPTIMZED_GETFUNCSCOPEOBJECT
	abc_getfuncscopeobject_localresult,

	abc_callFunctionSyntheticOneArg_constant_constant, // 0x320 ABC_OP_OPTIMZED_CALLFUNCTIONSYNTHETIC_ONEARG
	abc_callFunctionSyntheticOneArg_local_constant,
	abc_callFunctionSyntheticOneArg_constant_local,
	abc_callFunctionSyntheticOneArg_local_local,
	abc_callFunctionSyntheticOneArg_constant_constant_localresult,
	abc_callFunctionSyntheticOneArg_local_constant_localresult,
	abc_callFunctionSyntheticOneArg_constant_local_localresult,
	abc_callFunctionSyntheticOneArg_local_local_localresult,

	abc_callFunctionBuiltinOneArg_constant_constant, // 0x328 ABC_OP_OPTIMZED_CALLFUNCTIONBUILTIN_ONEARG
	abc_callFunctionBuiltinOneArg_local_constant,
	abc_callFunctionBuiltinOneArg_constant_local,
	abc_callFunctionBuiltinOneArg_local_local,
	abc_callFunctionBuiltinOneArg_constant_constant_localresult,
	abc_callFunctionBuiltinOneArg_local_constant_localresult,
	abc_callFunctionBuiltinOneArg_constant_local_localresult,
	abc_callFunctionBuiltinOneArg_local_local_localresult,

	abc_instanceof_constant_constant,// 0x330 ABC_OP_OPTIMZED_INSTANCEOF
	abc_instanceof_local_constant,
	abc_instanceof_constant_local,
	abc_instanceof_local_local,
	abc_instanceof_constant_constant_localresult,
	abc_instanceof_local_constant_localresult,
	abc_instanceof_constant_local_localresult,
	abc_instanceof_local_local_localresult,

	abc_subtract_i_constant_constant, // 0x338 ABC_OP_OPTIMZED_SUBTRACT_I
	abc_subtract_i_local_constant,
	abc_subtract_i_constant_local,
	abc_subtract_i_local_local,
	abc_subtract_i_constant_constant_localresult,
	abc_subtract_i_local_constant_localresult,
	abc_subtract_i_constant_local_localresult,
	abc_subtract_i_local_local_localresult,

	abc_multiply_i_constant_constant, // 0x340 ABC_OP_OPTIMZED_MULTIPLY_I
	abc_multiply_i_local_constant,
	abc_multiply_i_constant_local,
	abc_multiply_i_local_local,
	abc_multiply_i_constant_constant_localresult,
	abc_multiply_i_local_constant_localresult,
	abc_multiply_i_constant_local_localresult,
	abc_multiply_i_local_local_localresult,

	abc_subtract_i_constant_constant_setslotnocoerce, // 0x348 ABC_OP_OPTIMZED_SUBTRACT_I_SETSLOT
	abc_subtract_i_local_constant_setslotnocoerce,
	abc_subtract_i_constant_local_setslotnocoerce,
	abc_subtract_i_local_local_setslotnocoerce,
	abc_multiply_i_constant_constant_setslotnocoerce, // 0x34c ABC_OP_OPTIMZED_MULTIPLY_I_SETSLOT
	abc_multiply_i_local_constant_setslotnocoerce,
	abc_multiply_i_constant_local_setslotnocoerce,
	abc_multiply_i_local_local_setslotnocoerce,

	abc_inclocal_i_postfix, // 0x350 ABC_OP_OPTIMZED_INCLOCAL_I_POSTFIX
	abc_declocal_i_postfix, // 0x351 ABC_OP_OPTIMZED_DECLOCAL_I_POSTFIX
	abc_lookupswitch_constant, // 0x352 ABC_OP_OPTIMZED_LOOKUPSWITCH
	abc_lookupswitch_local,
	abc_ifnle_constant_constant, // 0x354 ABC_OP_OPTIMZED_IFNLE
	abc_ifnle_local_constant,
	abc_ifnle_constant_local,
	abc_ifnle_local_local,

	abc_ifngt_constant_constant, // 0x358 ABC_OP_OPTIMZED_IFNGT
	abc_ifngt_local_constant,
	abc_ifngt_constant_local,
	abc_ifngt_local_local,
	abc_callvoid_constant_constant, // 0x35c ABC_OP_OPTIMZED_CALL_VOID
	abc_callvoid_local_constant,
	abc_callvoid_constant_local,
	abc_callvoid_local_local,

	abc_call_constant_constant, // 0x360 ABC_OP_OPTIMZED_CALL
	abc_call_local_constant,
	abc_call_constant_local,
	abc_call_local_local,
	abc_call_constant_constant_localresult,
	abc_call_local_constant_localresult,
	abc_call_constant_local_localresult,
	abc_call_local_local_localresult,

	abc_coerce_constant,// 0x368 ABC_OP_OPTIMZED_COERCE
	abc_coerce_local,
	abc_coerce_constant_localresult,
	abc_coerce_local_localresult,
	abc_sxi1_constant,// 0x36c ABC_OP_OPTIMZED_SXI1
	abc_sxi1_local,
	abc_sxi1_constant_localresult,
	abc_sxi1_local_localresult,

	abc_sxi8_constant,// 0x370 ABC_OP_OPTIMZED_SXI8
	abc_sxi8_local,
	abc_sxi8_constant_localresult,
	abc_sxi8_local_localresult,
	abc_sxi16_constant,// 0x374 ABC_OP_OPTIMZED_SXI16
	abc_sxi16_local,
	abc_sxi16_constant_localresult,
	abc_sxi16_local_localresult,

	abc_nextvalue_constant_constant,// 0x378 ABC_OP_OPTIMZED_NEXTVALUE
	abc_nextvalue_local_constant,
	abc_nextvalue_constant_local,
	abc_nextvalue_local_local,
	abc_nextvalue_constant_constant_localresult,
	abc_nextvalue_local_constant_localresult,
	abc_nextvalue_constant_local_localresult,
	abc_nextvalue_local_local_localresult,

	abc_hasnext2_localresult,
	abc_hasnext2_iftrue,
	abc_getSlotFromScopeObject, // 0x382 ABC_OP_OPTIMZED_GETSLOTFROMSCOPEOBJECT
	abc_getSlotFromScopeObject_localresult,
	abc_constructpropMultiArgs_constant, // 0x384 ABC_OP_OPTIMZED_CONSTRUCTPROP_MULTIARGS
	abc_constructpropMultiArgs_local,
	abc_constructpropMultiArgs_constant_localresult,
	abc_constructpropMultiArgs_local_localresult,

	abc_nextname_constant_constant,// 0x388 ABC_OP_OPTIMZED_NEXTNAME
	abc_nextname_local_constant,
	abc_nextname_constant_local,
	abc_nextname_local_local,
	abc_nextname_constant_constant_localresult,
	abc_nextname_local_constant_localresult,
	abc_nextname_constant_local_localresult,
	abc_nextname_local_local_localresult,

	abc_callFunctionBuiltinNoArgs_constant, // 0x390 ABC_OP_OPTIMZED_CALLBUILTINFUNCTION_NOARGS
	abc_callFunctionBuiltinNoArgs_local,
	abc_callFunctionBuiltinNoArgs_constant_localresult,
	abc_callFunctionBuiltinNoArgs_local_localresult,
	abc_negate_constant,// 0x394 ABC_OP_OPTIMZED_NEGATE
	abc_negate_local,
	abc_negate_constant_localresult,
	abc_negate_local_localresult,

	abc_negate_constant_setslotnocoerce,// 0x398 ABC_OP_OPTIMZED_NEGATE_SETSLOT
	abc_negate_local_setslotnocoerce,
	abc_bitnot_constant,// 0x39a ABC_OP_OPTIMZED_BITNOT
	abc_bitnot_local,
	abc_bitnot_constant_localresult,
	abc_bitnot_local_localresult,
	abc_negate_constant_setslotnocoerce,// 0x39e ABC_OP_OPTIMZED_BITNOT_SETSLOT
	abc_negate_local_setslotnocoerce,

	abc_convert_s_constant,// 0x3a0 ABC_OP_OPTIMZED_CONVERTS
	abc_convert_s_local,
	abc_convert_s_constant_localresult,
	abc_convert_s_local_localresult,
	abc_convert_s_constant_setslotnocoerce,// 0x3a4 ABC_OP_OPTIMZED_CONVERTS_SETSLOT
	abc_convert_s_local_setslotnocoerce,
	abc_callpropvoidSlotVar_constant,// 0x3a6 ABC_OP_OPTIMZED_CALLPROPVOID_SLOTVAR_NOARGS
	abc_callpropvoidSlotVar_local,

	abc_coerce_s_constant,// 0x3a8 ABC_OP_OPTIMZED_COERCES
	abc_coerce_s_local,
	abc_coerce_s_constant_localresult,
	abc_coerce_s_local_localresult,
	abc_coerce_s_constant_setslotnocoerce,// 0x3ac ABC_OP_OPTIMZED_COERCES_SETSLOT
	abc_coerce_s_local_setslotnocoerce,
	abc_callpropvoidSlotVarCached_constant,// 0x3ae ABC_OP_OPTIMZED_CALLPROPVOID_SLOTVAR_MULTIARGS_CACHED_CALLER
	abc_callpropvoidSlotVarCached_local,

	abc_callpropvoidSlotVar_constant_constant,// 0x3b0 ABC_OP_OPTIMZED_CALLPROPVOID_SLOTVAR
	abc_callpropvoidSlotVar_local_constant,
	abc_callpropvoidSlotVar_constant_local,
	abc_callpropvoidSlotVar_local_local,
	abc_callpropertySlotVar_constant,// 0x3b4 ABC_OP_OPTIMZED_CALLPROPERTY_SLOTVAR_NOARGS
	abc_callpropertySlotVar_local,
	abc_callpropertySlotVar_constant_localresult,
	abc_callpropertySlotVar_local_localresult,

	abc_callpropertySlotVar_constant_constant,// 0x3b8 ABC_OP_OPTIMZED_CALLPROPERTY_SLOTVAR
	abc_callpropertySlotVar_local_constant,
	abc_callpropertySlotVar_constant_local,
	abc_callpropertySlotVar_local_local,
	abc_callpropertySlotVar_constant_constant_localresult,
	abc_callpropertySlotVar_local_constant_localresult,
	abc_callpropertySlotVar_constant_local_localresult,
	abc_callpropertySlotVar_local_local_localresult,

	abc_callpropertySlotVarCached_constant,// 0x3c0 ABC_OP_OPTIMZED_CALLPROPERTY_SLOTVAR_MULTIARGS_CACHED_CALLER
	abc_callpropertySlotVarCached_local,
	abc_callpropertySlotVarCached_constant_localResult,
	abc_callpropertySlotVarCached_local_localResult,
	abc_callpropvoidBorrowedSlot_constant,// 0x3c4 ABC_OP_OPTIMZED_CALLPROPVOID_BORROWEDSLOT_NOARGS
	abc_callpropvoidBorrowedSlot_local,
	abc_callpropvoidBorrowedSlotCached_constant,// 0x3c6 ABC_OP_OPTIMZED_CALLPROPVOID_BORROWEDSLOT_MULTIARGS_CACHED_CALLER
	abc_callpropvoidBorrowedSlotCached_local,

	abc_callpropertyBorrowedSlot_constant_constant,// 0x3c8 ABC_OP_OPTIMZED_CALLPROPERTY_BORROWEDSLOT
	abc_callpropertyBorrowedSlot_local_constant,
	abc_callpropertyBorrowedSlot_constant_local,
	abc_callpropertyBorrowedSlot_local_local,
	abc_callpropertyBorrowedSlot_constant_constant_localresult,
	abc_callpropertyBorrowedSlot_local_constant_localresult,
	abc_callpropertyBorrowedSlot_constant_local_localresult,
	abc_callpropertyBorrowedSlot_local_local_localresult,

	abc_callpropertyBorrowedSlot_constant,// 0x3d0 ABC_OP_OPTIMZED_CALLPROPERTY_BORROWEDSLOT_NOARGS
	abc_callpropertyBorrowedSlot_local,
	abc_callpropertyBorrowedSlot_constant_localresult,
	abc_callpropertyBorrowedSlot_local_localresult,
	abc_callpropvoidBorrowedSlot_constant_constant,// 0x3d4 ABC_OP_OPTIMZED_CALLPROPVOID_BORROWEDSLOT
	abc_callpropvoidBorrowedSlot_local_constant,
	abc_callpropvoidBorrowedSlot_constant_local,
	abc_callpropvoidBorrowedSlot_local_local,

	abc_callpropertyBorrowedSlotCached_constant,// 0x3d8 ABC_OP_OPTIMZED_CALLPROPERTY_BORROWEDSLOT_MULTIARGS_CACHED_CALLER
	abc_callpropertyBorrowedSlotCached_local,
	abc_callpropertyBorrowedSlotCached_constant_localResult,
	abc_callpropertyBorrowedSlotCached_local_localResult,
	abc_decrement_constant, // 0x3dc ABC_OP_OPTIMZED_DECREMENT
	abc_decrement_local,
	abc_decrement_constant_localresult,
	abc_decrement_local_localresult,

	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction
};


void ABCVm::preloadFunction(SyntheticFunction* function, ASWorker* wrk)
{
	method_info* mi=function->mi;

	const int code_len=mi->body->code.size();
	preloadstate state(function,wrk);
	std::map<int32_t,int32_t> jumppositions;
	std::map<int32_t,int32_t> jumpstartpositions;
	std::map<int32_t,int32_t> switchpositions;
	std::map<int32_t,int32_t> switchstartpositions;

	// this is used in a simple mechanism to detect if kill opcodes can be skipped
	// we just check if no getlocal opcode occurs after the kill
	std::set<uint32_t> skippablekills;

	// first pass:
	// - store all jump target points
	std::multimap<int32_t,int32_t> jumppoints;
	std::set<int32_t> exceptionjumptargets;
	std::map<int32_t,int32_t> unreachabletargets;

	for (int32_t i = 0; i < (int32_t)(mi->numArgs()-mi->numOptions())+1; i++)
	{
		state.unchangedlocals.insert(i);
	}
	if (!function->getMethodInfo()->returnType)
		function->checkParamTypes();
	function->checkExceptionTypes();
	state.localtypes.push_back(function->inClass);
	state.defaultlocaltypes.push_back(function->inClass);
	state.defaultlocaltypescacheable.push_back(true);
	for (uint32_t i = 1; i < mi->body->getReturnValuePos(); i++)
	{
		state.localtypes.push_back(nullptr);
		state.defaultlocaltypes.push_back(nullptr);
		state.defaultlocaltypescacheable.push_back(true);
		if (mi->needsArgs() && i == mi->numArgs()+1) // don't cache argument array
			state.defaultlocaltypescacheable[i]=false;
		if (i > 0 && i <= mi->paramTypes.size() && dynamic_cast<const Class_base*>(mi->paramTypes[i-1]))
			state.defaultlocaltypes[i]= (Class_base*)mi->paramTypes[i-1]; // cache types of arguments
	}
	for (uint32_t i = state.mi->numArgs()+1; i < state.mi->body->local_count; i++)
	{
		state.canlocalinitialize.push_back(true);
	}
#ifdef ENABLE_OPTIMIZATION
	if (!mi->body->exceptions.empty())
	{
		// reserve first local result for exception object if exception is caught
		mi->body->localresultcount++;
		state.localtypes.push_back(nullptr);
		state.defaultlocaltypes.push_back(nullptr);
		state.defaultlocaltypescacheable.push_back(true);
	}
#endif

	auto itex = mi->body->exceptions.begin();
	while (itex != mi->body->exceptions.end())
	{
		// add exception jump targets
		state.jumptargets[(int32_t)itex->target+1]=1;
		exceptionjumptargets.insert((int32_t)itex->target+1);
		itex++;
	}
	uint32_t simple_getter_opcode_pos = 0;
	uint8_t simple_getter_opcodes[] {
				0xd0, //getlocal_0
				0x30, //pushscope
				0x60, //getlex
				0x48, //returnvalue
				0x00
			};
	uint32_t simple_setter_opcode_pos = 0;
	uint8_t simple_setter_opcodes[] {
				0xd0, //getlocal_0
				0x30, //pushscope
				0x5e, //findproperty
				0xd1, //getlocal_1
				0x68, //initproperty
				0x47, //returnvoid
				0x00
			};
	uint8_t opcode=0;
	memorystream codejumps(mi->body->code.data(), code_len);
	std::vector<asAtom> constantsstack;
	while(!codejumps.atend())
	{
		uint8_t prevopcode=opcode;
		opcode = codejumps.readbyte();
		if (simple_getter_opcode_pos != UINT32_MAX && opcode && opcode == simple_getter_opcodes[simple_getter_opcode_pos])
			++simple_getter_opcode_pos;
		else
			simple_getter_opcode_pos = UINT32_MAX;
		if (simple_setter_opcode_pos != UINT32_MAX && opcode && opcode == simple_setter_opcodes[simple_setter_opcode_pos])
			++simple_setter_opcode_pos;
		else
			simple_setter_opcode_pos = UINT32_MAX;
		//LOG(LOG_ERROR,"preload pass1:"<<function->getSystemState()->getStringFromUniqueId(function->functionname)<<" "<< codejumps.tellg()-1<<" "<<" "<<hex<<(int)opcode);
		switch(opcode)
		{
			case 0x04://getsuper
			case 0x05://setsuper
			case 0x06://dxns
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
			case 0x65://getscopeobject
			case 0x66://getproperty
			case 0x68://initproperty
			case 0x6a://deleteproperty
			case 0x6c://getslot
			case 0x6d://setslot
			case 0x6e://getglobalSlot
			case 0x6f://setglobalSlot
			case 0x86://astype
			case 0xb2://istype
				codejumps.readu30();
				constantsstack.clear();
				break;
			case 0x08://kill
			{
				uint32_t t = codejumps.readu30();
				skippablekills.insert(t);
				constantsstack.clear();
				break;
			}
			case 0x62://getlocal
			{
				uint32_t t = codejumps.readu30();
				skippablekills.erase(t);
				constantsstack.clear();
				break;
			}
			case 0xd0://getlocal_0
			case 0xd1://getlocal_1
			case 0xd2://getlocal_2
			case 0xd3://getlocal_3
				skippablekills.erase(opcode-0xd0);
				constantsstack.clear();
				break;
			case 0x80://coerce
				codejumps.readu30();
				constantsstack.clear();
				break;
			case 0xf0://debugline
			case 0xf1://debugfile
			case 0xf2://bkptline
			case 0x63://setlocal
			case 0x92://inclocal
			case 0x94://declocal
			case 0xc2://inclocal_i
			case 0xc3://declocal_i
				codejumps.readu30();
				constantsstack.clear();
				break;
			case 0x2c://pushstring
			{
				uint32_t value = codejumps.readu30();
				constantsstack.push_back(*function->mi->context->getConstantAtom(OP_STRING,value));
				break;
			}
			case 0x2d://pushint
			{
				uint32_t value = codejumps.readu30();
				constantsstack.push_back(*function->mi->context->getConstantAtom(OP_INTEGER,value));
				break;
			}
			case 0x2e://pushuint
			{
				uint32_t value = codejumps.readu30();
				constantsstack.push_back(*function->mi->context->getConstantAtom(OP_UINTEGER,value));
				break;
			}
			case 0x2f://pushdouble
			{
				uint32_t value = codejumps.readu30();
				constantsstack.push_back(*function->mi->context->getConstantAtom(OP_DOUBLE,value));
				break;
			}
			case 0x31://pushnamespace
			{
				uint32_t value = codejumps.readu30();
				constantsstack.push_back(*function->mi->context->getConstantAtom(OP_NAMESPACE,value));
				break;
			}
			case 0x10://jump
			{
				int32_t p = codejumps.tellg();
				int32_t p1 = codejumps.reads24()+codejumps.tellg()+1;
				if (p1 > p)
				{
					int32_t nextreachable = p1;
					// find the first jump target after the current position
					auto it = state.jumptargets.begin();
					while (it != state.jumptargets.end() && it->first < nextreachable)
					{
						if (it->first > p && it->first <nextreachable)
							nextreachable = it->first;
						it++;
					}
					unreachabletargets[p] = nextreachable;
				}
				if (state.jumptargets.count(p1))
					state.jumptargeteresulttypes.erase(p1);
				state.jumptargets[p1]++;
				jumppoints.insert(make_pair(p,p1));
				constantsstack.clear();
				break;
			}
			case 0x0c://ifnlt
			case 0x0d://ifnle
			case 0x0e://ifngt
			case 0x0f://ifnge
			case 0x15://iflt
			case 0x16://ifle
			case 0x17://ifgt
			case 0x18://ifge
			case 0x19://ifstricteq
			case 0x1a://ifstrictne
			{
				// TODO check for unreachable code
				int32_t p = codejumps.tellg();
				int32_t p1 = codejumps.reads24()+codejumps.tellg()+1;
				if (p1==p+1)
				{
					// this is a jump over an empty branch,it can be skipped
					// we simply replace it with two "pop" opcodes (for the args) and two "nop" filler opcodes
					// in the main code array
					mi->body->code[p  ]=0x29;//pop
					mi->body->code[p+1]=0x29;//pop
					mi->body->code[p+2]=0x02;//nop
					mi->body->code[p+3]=0x02;//nop
					break;
				}
				state.jumptargeteresulttypes.erase(p1);
				state.jumptargets[p1]++;
				jumppoints.insert(make_pair(p,p1));
				constantsstack.clear();
				break;
			}
			case 0x13://ifeq
			{
				int32_t p = codejumps.tellg();
				int32_t p1 = codejumps.reads24()+codejumps.tellg()+1;
				if (p1 > p && constantsstack.size()>1 &&
						asAtomHandler::isEqual(constantsstack[constantsstack.size()-1]
												, wrk
												, constantsstack[constantsstack.size()-1])
						)//opcode is preceded by two constants, so we can compare them and check for unreachable code
				{
					int32_t nextreachable = p1;
					// find the first jump target after the current position
					auto it = state.jumptargets.begin();
					while (it != state.jumptargets.end() && it->first < nextreachable)
					{
						if (it->first > p && it->first <nextreachable)
							nextreachable = it->first;
						it++;
					}
					unreachabletargets[p] = nextreachable;
				}
				state.jumptargeteresulttypes.erase(p1);
				state.jumptargets[p1]++;
				jumppoints.insert(make_pair(p,p1));
				constantsstack.clear();
				break;
			}
			case 0x14://ifne
			{
				int32_t p = codejumps.tellg();
				int32_t p1 = codejumps.reads24()+codejumps.tellg()+1;
				if (p1 > p && constantsstack.size()>1 &&
						!asAtomHandler::isEqual(constantsstack[constantsstack.size()-1]
												, wrk
												, constantsstack[constantsstack.size()-1])
						)//opcode is preceded by two constants, so we can compare them and check for unreachable code
				{
					int32_t nextreachable = p1;
					// find the first jump target after the current position
					auto it = state.jumptargets.begin();
					while (it != state.jumptargets.end() && it->first < nextreachable)
					{
						if (it->first > p && it->first <nextreachable)
							nextreachable = it->first;
						it++;
					}
					unreachabletargets[p] = nextreachable;
				}
				state.jumptargeteresulttypes.erase(p1);
				state.jumptargets[p1]++;
				jumppoints.insert(make_pair(p,p1));
				constantsstack.clear();
				break;
			}
			case 0x11://iftrue
			{
				int32_t p = codejumps.tellg();
				int32_t p1 = codejumps.reads24()+codejumps.tellg()+1;
				if (p1 > p && state.jumptargets.find(p) == state.jumptargets.end() && prevopcode==0x26) //pushtrue
				{
					int32_t nextreachable = p1;
					// find the first jump target after the current position
					auto it = state.jumptargets.begin();
					while (it != state.jumptargets.end() && it->first < nextreachable)
					{
						if (it->first > p && it->first <nextreachable)
							nextreachable = it->first;
						it++;
					}
					unreachabletargets[p] = nextreachable;
				}
				state.jumptargeteresulttypes.erase(p1);
				state.jumptargets[p1]++;
				jumppoints.insert(make_pair(p,p1));
				constantsstack.clear();
				break;
			}
			case 0x12://iffalse
			{
				int32_t p = codejumps.tellg();
				int32_t p1 = codejumps.reads24()+codejumps.tellg()+1;
				if (p1 > p && state.jumptargets.find(p) == state.jumptargets.end() && prevopcode==0x27) //pushfalse
				{
					int32_t nextreachable = p1;
					// find the first jump target after the current position
					auto it = state.jumptargets.begin();
					while (it != state.jumptargets.end() && it->first < nextreachable)
					{
						if (it->first > p && it->first <nextreachable)
							nextreachable = it->first;
						it++;
					}
					unreachabletargets[p] = nextreachable;
				}
				state.jumptargeteresulttypes.erase(p1);
				state.jumptargets[p1]++;
				jumppoints.insert(make_pair(p,p1));
				constantsstack.clear();
				break;
			}
			case 0x1b://lookupswitch
			{
				int32_t p = codejumps.tellg();
				int32_t p1 = p+codejumps.reads24();
				state.jumptargeteresulttypes.erase(p1);
				state.jumptargets[p1]++;
				jumppoints.insert(make_pair(p,p1));
				uint32_t count = codejumps.readu30();
				for(unsigned int i=0;i<count+1;i++)
				{
					p1 = p+codejumps.reads24();
					state.jumptargeteresulttypes.erase(p1);
					state.jumptargets[p1]++;
					jumppoints.insert(make_pair(p,p1));
				}
				constantsstack.clear();
				break;
			}
			case 0x24://pushbyte
			{
				int32_t value = (int32_t)(int8_t)codejumps.readbyte();
				constantsstack.push_back(asAtomHandler::fromInt(value));
				break;
			}
			case 0x25://pushshort
			{
				int32_t value = (int32_t)(int16_t)codejumps.readu32();
				constantsstack.push_back(asAtomHandler::fromInt(value));
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
				constantsstack.clear();
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
			case 0x47://returnvoid
			case 0x48://returnvalue
			case 0x03://throw
			{
				int32_t p = codejumps.tellg();
				int32_t nextreachable = codejumps.size();
				// find the first jump target after the current position
				auto it = state.jumptargets.begin();
				while (it != state.jumptargets.end() && it->first <= (int)codejumps.tellg())
				{
					it++;
				}
				if (it != state.jumptargets.end())
					nextreachable = it->first;
				unreachabletargets[p] = nextreachable;
				constantsstack.clear();
				break;
			}
			case 0x81://coerce_b
				// deprecated  according to https://konghack.com/content/19-avm2instructions
				// use convert_b instead
				mi->body->code[codejumps.tellg()-1]=0x76; //convert_b
				constantsstack.clear();
				break;
			case 0x83://coerce_i
				// deprecated  according to https://konghack.com/content/19-avm2instructions
				// use convert_i instead
				mi->body->code[codejumps.tellg()-1]=0x73; //convert_i
				constantsstack.clear();
				break;
			case 0x84://coerce_d
				// deprecated  according to https://konghack.com/content/19-avm2instructions
				// use convert_d instead
				mi->body->code[codejumps.tellg()-1]=0x75; //convert_d
				constantsstack.clear();
				break;
			case 0x88://coerce_u
				// deprecated  according to https://konghack.com/content/19-avm2instructions
				// use convert_u instead
				mi->body->code[codejumps.tellg()-1]=0x74; //convert_u
				constantsstack.clear();
				break;
			default:
				constantsstack.clear();
				break;
		}
	}
	// remove all really unreachable targets
#ifdef ENABLE_OPTIMIZATION
	auto it = unreachabletargets.begin();
	while (it != unreachabletargets.end())
	{
		int32_t realUnreachableStart = it->first;
		int32_t realNextReachable = it->second;

		// search for exception targets that point inside the unreachable area and adjust realUnreachableEnd
		auto itexctarget = exceptionjumptargets.begin();
		while (itexctarget != exceptionjumptargets.end())
		{
			if (*itexctarget < realNextReachable && *itexctarget >= realUnreachableStart)
				realNextReachable = *itexctarget;
			itexctarget++;
		}

		// search backwards for jumps that point inside the unreachable area and adjust realUnreachableEnd
		auto itpoint = jumppoints.rbegin();
		while (itpoint != jumppoints.rend())
		{
			if (itpoint->first < realNextReachable)
				break; // jump is inside the unreachable area
			else if (itpoint->second < realNextReachable && itpoint->second > realUnreachableStart)
				realNextReachable = itpoint->second; // jump points inside the unreachable area, adjust end of unreachable area
			itpoint++;
		}
		// remove all jump targets inside the adjusted unreachable area
		auto ittarget = state.jumptargets.rbegin();
		while (ittarget != state.jumptargets.rend())
		{
			if (ittarget->first <= realUnreachableStart)
				break; // beginning of unreachable area reached, we can stop now
			if (ittarget->first < realNextReachable && exceptionjumptargets.find(ittarget->first) == exceptionjumptargets.end())
			{
				state.jumptargets.erase(ittarget->first); // jump is inside unreachable area, can be removed
			}
			ittarget++;
		}
		it++;
	}
#endif
	// second pass:
	// - compute types of the locals and detect if they don't change during execution
	preloadFunction_secondPass(state);
	// third pass:
	// - use optimized opcode version if it doesn't interfere with a jump target

	std::vector<typestackentry> typestack; // contains the type or the global object of the arguments currently on the stack
	Class_base* lastlocalresulttype=nullptr;
	clearOperands(state,true,&lastlocalresulttype);
	memorystream code(mi->body->code.data(), code_len);
	Activation_object* activationobject=nullptr;
	std::list<Catchscope_object*> catchscopelist;
	int dup_indicator=0;
	bool opcode_skipped=false;
	bool coercereturnvalue=false;
	bool reverse_iftruefalse=false;
	auto itcurEx = mi->body->exceptions.begin();
	opcode=0;
	while(!code.atend())
	{
		state.refreshOldNewPosition(code);
		state.atexceptiontarget=false;
		if (state.jumptargets.find(code.tellg()) != state.jumptargets.end())
			state.canlocalinitialize.clear();
		while (itcurEx != mi->body->exceptions.end() && itcurEx->target == code.tellg())
		{
#ifdef ENABLE_OPTIMIZATION
			// set exception object via getlocal
			state.preloadedcode.push_back((uint32_t)0x62);//getlocal
			uint32_t value = state.mi->body->getReturnValuePos()+1; // first localresult is reserved for exception catch
			state.preloadedcode.back().pcode.arg3_uint=value;
			state.operandlist.push_back(operands(OP_LOCAL,itcurEx->exc_class,value,1,state.preloadedcode.size()-1));
			state.atexceptiontarget=true;
#endif
			typestack.push_back(typestackentry(itcurEx->exc_class,false));
			itcurEx++;
		}
		uint8_t prevopcode=opcode;
		opcode = code.readbyte();
		// if (typestack.empty() || typestack.back().obj==nullptr)
		// 	LOG(LOG_INFO,"preload pass3 opcode:"<<function->getSystemState()->getStringFromUniqueId(function->functionname)<<" "<< code.tellg()-1<<" "<<state.operandlist.size()<<" "<<typestack.size()<<" "<<state.preloadedcode.size()<<" "<<hex<<(int)opcode);
		// else
		// 	LOG(LOG_INFO,"preload pass3 opcode:"<<function->getSystemState()->getStringFromUniqueId(function->functionname)<<" "<< code.tellg()-1<<" "<<state.operandlist.size()<<" "<<typestack.size()<<" "<<typestack.back().obj->toDebugString()<<" "<<state.preloadedcode.size()<<" "<< hex<<(int)opcode);
		if (opcode_skipped)
			opcode_skipped=false;
		else
		{
			switch (dup_indicator)
			{
				case 0:
					break;
				case 1:// dup found
					dup_indicator=2;
					break;
				case 2:// opcode after dup handled
					dup_indicator=0;
					break;
			}
		}


		switch(opcode)
		{
			case 0x5f://finddef
			{
				uint32_t t = code.readu30();
				assert_and_throw(t < mi->context->constant_pool.multiname_count);
				removetypestack(typestack,mi->context->constant_pool.multinames[t].runtimeargs);
				typestack.push_back(typestackentry(nullptr,false));
				state.preloadedcode.push_back((uint32_t)opcode);
				state.refreshOldNewPosition(code);
				state.preloadedcode.back().pcode.arg3_uint=t;
				clearOperands(state,true,&lastlocalresulttype);
				break;
			}
			case 0x04://getsuper
			case 0x59://getdescendants
			{
				uint32_t t = code.readu30();
				assert_and_throw(t < mi->context->constant_pool.multiname_count);
				removetypestack(typestack,mi->context->constant_pool.multinames[t].runtimeargs+1);
				typestack.push_back(typestackentry(nullptr,false));
				state.preloadedcode.push_back((uint32_t)opcode);
				state.refreshOldNewPosition(code);
				state.preloadedcode.back().pcode.arg3_uint=t;
				clearOperands(state,true,&lastlocalresulttype);
				break;
			}
			case 0x05://setsuper
			{
				uint32_t t = code.readu30();
				assert_and_throw(t < mi->context->constant_pool.multiname_count);
				removetypestack(typestack,mi->context->constant_pool.multinames[t].runtimeargs+1);
				state.preloadedcode.push_back((uint32_t)opcode);
				state.refreshOldNewPosition(code);
				state.preloadedcode.back().pcode.arg3_uint=t;
				clearOperands(state,true,&lastlocalresulttype);
				break;
			}
			case 0x40://newfunction
			{
				uint32_t t = code.readu30();
				typestack.push_back(typestackentry(nullptr,false));
				state.preloadedcode.push_back((uint32_t)opcode);
				state.refreshOldNewPosition(code);
				state.preloadedcode.back().pcode.arg1_uint = t;
				clearOperands(state,true,&lastlocalresulttype);
				break;
			}
			case 0x41://call
			{
				uint32_t p = code.tellg();
				uint32_t t = code.readu30();
#ifdef ENABLE_OPTIMIZATION
				bool resultused=true;
				Class_base* resulttype=nullptr;
				if (state.operandlist.size() > t+1 && t < UINT16_MAX)
				{
					if (code.peekbyte() == 0x29 //pop
							&& state.jumptargets.find(code.tellg()) == state.jumptargets.end())
					{
						resultused=false;
						state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size()+1;
						code.readbyte(); // skip pop
					}
					std::vector<operands> tmpoperandlist;
					auto it = state.operandlist.rbegin();
					tmpoperandlist.assign(it,it+t);
					auto ittmp = tmpoperandlist.begin();
					for(uint32_t i= 0; i < t; i++)
					{
						state.operandlist.pop_back();
						it++;
						ittmp->removeArg(state);
						ittmp++;
					}
					if (resultused)
						setupInstructionTwoArguments(state,ABC_OP_OPTIMZED_CALL,opcode,code,false,false,true,p,resulttype);
					else
						setupInstructionTwoArgumentsNoResult(state,ABC_OP_OPTIMZED_CALL_VOID,opcode,code);
					state.preloadedcode.back().pcode.local3.flags=t;
					ittmp = tmpoperandlist.begin();
					for(uint32_t i= 0; i < t; i++)
					{
						state.preloadedcode.push_back(0);
						ittmp->fillCode(state,0,state.preloadedcode.size()-1,false);
						state.preloadedcode.back().pcode.arg2_uint = ittmp->type;
						ittmp++;
					}
					removetypestack(typestack,t+2);
					if (resultused)
						typestack.push_back(typestackentry(resulttype,false));
					if (resultused)
					{
						bool skip = false;
						switch (code.peekbyte())
						{
							case 0x73://convert_i
								skip = resulttype == Class<Integer>::getRef(function->getSystemState()).getPtr();
								break;
							case 0x74://convert_u
								skip = resulttype == Class<UInteger>::getRef(function->getSystemState()).getPtr();
								break;
							case 0x75://convert_d
								skip = resulttype == Class<Number>::getRef(function->getSystemState()).getPtr();
								break;
							case 0x76://convert_b
								skip = resulttype == Class<Boolean>::getRef(function->getSystemState()).getPtr();
								break;
						}
						if (skip && state.jumptargets.find(code.tellg()+1) == state.jumptargets.end())
							code.readbyte();
					}
					break;
				}
#endif
				removetypestack(typestack,t+2);
				typestack.push_back(typestackentry(nullptr,false));
				state.preloadedcode.push_back((uint32_t)opcode);
				state.refreshOldNewPosition(code);
				state.preloadedcode.back().pcode.arg1_uint = t;
				clearOperands(state,true,&lastlocalresulttype);
				break;
			}
			case 0x53://constructgenerictype
			{
				uint32_t t = code.readu30();
#ifdef ENABLE_OPTIMIZATION
				if (t==1 && state.operandlist.size()>t && function->func_scope.getPtr() && (state.scopelist.begin()==state.scopelist.end() || !state.scopelist.back().considerDynamic))
				{
					uint32_t i = 0;
					ASObject** args=LS_STACKALLOC(ASObject*, t);
					auto it = state.operandlist.rbegin();
					while (i < t)
					{
						if ((*it).type != OP_CACHED_CONSTANT)
							break;
						args[t-i-1]=asAtomHandler::toObject(*mi->context->getConstantAtom(OP_CACHED_CONSTANT,(*it).index),function->worker);
						it++;
						i++;
					}
					if (i >= t)
					{
						asAtom ret = constructGenericType_intern(mi->context,asAtomHandler::toObject(*mi->context->getConstantAtom(OP_CACHED_CONSTANT,(*it).index),function->worker),t,args);
						if (asAtomHandler::isInvalid(ret))
						{
							createError<TypeError>(function->worker,0,"Wrong type in applytype");
							break;
						}

						removeOperands(state,true,&lastlocalresulttype,t+1);
						for (i=0; i < t+1; i++)
							state.preloadedcode.pop_back(); // remove getlocals
						addCachedConstant(state,mi, ret,code);
						removetypestack(typestack,t+1);
						typestack.push_back(typestackentry(nullptr,false));
						if (asAtomHandler::isUndefined(ret))
							break;
						// Register the type name in the global scope.
						ASObject* global =asAtomHandler::toObject(function->func_scope->scope.front().object,function->worker);
						QName qname = asAtomHandler::as<Class_base>(ret)->class_name;
						if (!global->hasPropertyByMultiname(qname, false, false,function->worker))
							global->setVariableAtomByQName(global->getSystemState()->getStringFromUniqueId(qname.nameId),nsNameAndKind(global->getSystemState(),qname.nsStringId,NAMESPACE),ret,DECLARED_TRAIT);
						break;
					}

				}
#endif
				removetypestack(typestack,t+1);
				typestack.push_back(typestackentry(nullptr,false));
				state.preloadedcode.push_back((uint32_t)opcode);
				state.refreshOldNewPosition(code);
				state.preloadedcode.back().pcode.arg3_uint=t;
				clearOperands(state,true,&lastlocalresulttype);
				break;
			}
			case 0x55://newobject
			{
				uint32_t t = code.readu30();
				removetypestack(typestack,t*2);
				Class_base* resulttype = Class<ASObject>::getRef(function->getSystemState()).getPtr();
				typestack.push_back(typestackentry(resulttype,false));
#ifdef ENABLE_OPTIMIZATION
				bool done = false;
				switch (t)
				{
					case 0:
						state.preloadedcode.push_back((uint32_t)opcode);
						state.refreshOldNewPosition(code);
						state.preloadedcode.back().pcode.arg3_uint=t;
						if (checkForLocalResult(state,code,0,resulttype))
							state.preloadedcode[state.preloadedcode.size()-1].pcode.func = abc_newobject_noargs_localresult;
						else
							clearOperands(state,true,&lastlocalresulttype);
						done=true;
						break;
					default:
						// TODO optimize newobject with arguments
						break;
				}
				if (!done)
#endif
				{
					state.preloadedcode.push_back((uint32_t)opcode);
					state.refreshOldNewPosition(code);
					state.preloadedcode.back().pcode.arg3_uint=t;
					clearOperands(state,true,&lastlocalresulttype);
				}
				break;
			}
			case 0x56://newarray
			{
				uint32_t t = code.readu30();
				removetypestack(typestack,t);
				typestack.push_back(typestackentry(nullptr,false));
				state.preloadedcode.push_back((uint32_t)opcode);
				state.refreshOldNewPosition(code);
				state.preloadedcode.back().pcode.arg3_uint=t;
				clearOperands(state,true,&lastlocalresulttype);
				break;
			}
			case 0x58://newclass
			{
				uint32_t t = code.readu30();
				removetypestack(typestack,1);
				typestack.push_back(typestackentry(nullptr,false));
				state.preloadedcode.push_back((uint32_t)opcode);
				state.refreshOldNewPosition(code);
				state.preloadedcode.back().pcode.arg3_uint=t;
				clearOperands(state,true,&lastlocalresulttype);
				break;
			}
			case 0x5a://newcatch
			{
				state.preloadedcode.push_back((uint32_t)opcode);
				state.refreshOldNewPosition(code);
				uint32_t t = code.readu30();
#ifdef ENABLE_OPTIMIZATION
				Catchscope_object* catchscope = new_catchscopeObject(wrk,state.mi, t);
				catchscopelist.push_back(catchscope);
				typestack.push_back(typestackentry(catchscope,false));
				if (checkForLocalResult(state,code,0,nullptr))
				{
					state.operandlist.at(state.operandlist.size()-1).instance = catchscope;
					state.preloadedcode.at(state.preloadedcode.size()-1).pcode.func=abc_newcatch_localresult;
					state.preloadedcode.at(state.preloadedcode.size()-1).pcode.arg2_uint=t;
				}
				else
#endif
				{
					state.preloadedcode.back().pcode.arg3_uint=t;
					clearOperands(state,true,&lastlocalresulttype);
				}
				break;
			}
			case 0x6e://getglobalSlot
			{
				uint32_t t = code.readu30();
				typestack.push_back(typestackentry(nullptr,false));
				state.preloadedcode.push_back((uint32_t)opcode);
				state.refreshOldNewPosition(code);
				state.preloadedcode.back().pcode.arg1_uint=t;
				clearOperands(state,true,&lastlocalresulttype);
				break;
			}
			case 0x6a://deleteproperty
			case 0xb2://istype
			{
				uint32_t t = code.readu30();
				assert_and_throw(t < mi->context->constant_pool.multiname_count);
				removetypestack(typestack,mi->context->constant_pool.multinames[t].runtimeargs+1);
				typestack.push_back(typestackentry(Class<Boolean>::getRef(mi->context->applicationDomain->getSystemState()).getPtr(),false));
				state.preloadedcode.push_back((uint32_t)opcode);
				state.refreshOldNewPosition(code);
				state.preloadedcode.back().pcode.arg1_uint=t;
				clearOperands(state,true,&lastlocalresulttype);
				break;
			}
			case 0x6f://setglobalSlot
			{
				uint32_t t = code.readu30();
				removetypestack(typestack,1);
				state.preloadedcode.push_back((uint32_t)opcode);
				state.refreshOldNewPosition(code);
				state.preloadedcode.back().pcode.arg1_uint=t;
				clearOperands(state,true,&lastlocalresulttype);
				break;
			}
			case 0x86://astype
			{
				int32_t p = code.tellg();
				uint32_t t = code.readu30();
				if (state.jumptargets.find(p) == state.jumptargets.end() && prevopcode == 0x20) //pushnull
				{
					break;
				}
				removetypestack(typestack,1);
				typestack.push_back(typestackentry(nullptr,false));
				state.preloadedcode.push_back((uint32_t)opcode);
				state.refreshOldNewPosition(code);
				state.preloadedcode.back().pcode.arg1_uint=t;
				clearOperands(state,true,&lastlocalresulttype);
				break;
			}
			case 0xc2://inclocal_i
			{
				uint32_t p = code.tellg();
				uint32_t t = code.readu30();
				if (!checkforpostfix(state,code,p,typestack, t,ABC_OP_OPTIMZED_INCLOCAL_I_POSTFIX))
				{
					state.preloadedcode.push_back(ABC_OP_OPTIMZED_INCLOCAL_I);
					state.preloadedcode.back().pcode.arg1_uint = t;
					state.preloadedcode.back().pcode.arg2_uint = 1;
					state.refreshOldNewPosition(code);
					setOperandModified(state,OP_LOCAL,t);
					clearOperands(state,true,&lastlocalresulttype);
				}
				break;
			}
			case 0xc3://declocal_i
			{
				uint32_t p = code.tellg();
				uint32_t t = code.readu30();
				if (!checkforpostfix(state,code,p,typestack, t,ABC_OP_OPTIMZED_DECLOCAL_I_POSTFIX))
				{
					state.preloadedcode.push_back(ABC_OP_OPTIMZED_DECLOCAL_I);
					state.preloadedcode.back().pcode.arg1_uint = t;
					state.preloadedcode.back().pcode.arg2_uint = 1;
					state.refreshOldNewPosition(code);
					setOperandModified(state,OP_LOCAL,t);
					clearOperands(state,true,&lastlocalresulttype);
				}
				break;
			}
			case 0x08://kill
			{
				int32_t p = code.tellg();
				state.refreshOldNewPosition(code);
				uint32_t t = code.readu30();
#ifdef ENABLE_OPTIMIZATION
				if (state.jumptargets.find(p) != state.jumptargets.end())
				{
					state.jumptargeteresulttypes.erase(code.tellg()+1);
					state.jumptargets[code.tellg()+1]++;
					clearOperands(state,true,&lastlocalresulttype);
				}
				if (skippablekills.count(t))
				{
					opcode_skipped=true;
				}
				else
				{
					for (auto it = state.operandlist.rbegin(); it != state.operandlist.rend(); ++it)
					{
						if (it->type == OP_LOCAL && uint32_t(it->index) == t)
						{
							// local value is still needed, so don't kill it
							opcode_skipped=true;
							break;
						}
					}
				}
				if (!opcode_skipped)
#endif
				{
					state.preloadedcode.push_back((uint32_t)opcode);
					state.oldnewpositions[p] = (int32_t)state.preloadedcode.size();
					state.preloadedcode.back().pcode.arg3_uint = t;
				}
				setOperandModified(state,OP_LOCAL,t);

				break;
			}
			case 0x92://inclocal
			case 0x94://declocal
			{
				state.preloadedcode.push_back((uint32_t)opcode);
				state.refreshOldNewPosition(code);
				uint32_t t = code.readu30();
				state.preloadedcode.back().pcode.arg1_uint=t;
				setOperandModified(state,OP_LOCAL,t);
				clearOperands(state,true,&lastlocalresulttype,true,OP_LOCAL,t);
				break;
			}
			case 0x06://dxns
			{
				state.preloadedcode.push_back((uint32_t)opcode);
				state.refreshOldNewPosition(code);
				state.preloadedcode.back().pcode.arg3_uint = code.readu30();
				clearOperands(state,true,&lastlocalresulttype);
				break;
			}
			case 0x1c://pushwith
				removetypestack(typestack,1);
				state.scopelist.push_back(scope_entry(asAtomHandler::invalidAtom,true));
				state.preloadedcode.push_back((uint32_t)opcode);
				state.refreshOldNewPosition(code);
				clearOperands(state,true,&lastlocalresulttype);
				break;
			case 0x1d://popscope
				if (!state.scopelist.empty())
					state.scopelist.pop_back();
				state.preloadedcode.push_back((uint32_t)opcode);
				state.refreshOldNewPosition(code);
				clearOperands(state,true,&lastlocalresulttype);
				break;
			case 0x49://constructsuper
			{
				int32_t p = code.tellg();
				state.checkClearOperands(p,&lastlocalresulttype);
				uint32_t t =code.readu30();
				removetypestack(typestack,t+1);
#ifdef ENABLE_OPTIMIZATION
				if (function->inClass && t==0) // class method with 0 params
				{
					if (function->inClass->super.getPtr() == Class<ASObject>::getClass(function->getSystemState()) // super class is ASObject, so constructsuper can be skipped
							&& !state.operandlist.empty())
					{
						state.preloadedcode.pop_back();
						state.operandlist.pop_back();
					}
					else
					{
						if (!setupInstructionOneArgumentNoResult(state,ABC_OP_OPTIMZED_CONSTRUCTSUPER,opcode,code,p))
							state.preloadedcode.back().pcode.arg3_uint=t;
					}
				}
				else
#endif
				{
					state.preloadedcode.push_back((uint32_t)opcode);
					state.preloadedcode.back().pcode.arg3_uint=t;
					clearOperands(state,true,&lastlocalresulttype);
				}
				break;
			}
			case 0x5e://findproperty
			case 0x5d://findpropstrict
			{
				int32_t p = code.tellg();
				state.checkClearOperands(p,&lastlocalresulttype);
				uint32_t t =code.readu30();
				removetypestack(typestack,mi->context->constant_pool.multinames[t].runtimeargs);
				if (!preload_findprop(state,code,t,true,&lastlocalresulttype,typestack))
				{
					state.preloadedcode.push_back((uint32_t)opcode);
					state.refreshOldNewPosition(code);
					state.preloadedcode.back().pcode.local3.pos=t;
					clearOperands(state,true,&lastlocalresulttype);
					typestack.push_back(typestackentry(nullptr,false));
				}
				break;
			}
			case 0x60://getlex
				preload_getlex(state,typestack,code,opcode,&lastlocalresulttype,state.scopelist,simple_getter_opcode_pos);
				break;
			case 0x61://setproperty
			case 0x68://initproperty
				preload_setproperty(state, typestack, code, opcode, &lastlocalresulttype,simple_setter_opcode_pos);
				break;
			case 0x62://getlocal
			{
				int32_t p = code.tellg();
				uint32_t value =code.readu30();
#ifdef ENABLE_OPTIMIZATION
				if ((state.jumptargets.find(code.tellg()+1) == state.jumptargets.end()) && code.peekbyte()==0x29) // pop
				{
					code.readbyte();
					break;
				}
#endif
				assert_and_throw(value < mi->body->getReturnValuePos());
				removeInitializeLocalToConstant(state,value);
				state.preloadedcode.push_back((uint32_t)opcode);
				state.refreshOldNewPosition(code);
				state.preloadedcode.back().pcode.arg3_uint=value;
				state.checkClearOperands(p,&lastlocalresulttype);
				state.operandlist.push_back(operands(OP_LOCAL,state.localtypes[value],value,1,state.preloadedcode.size()-1));
				typestack.push_back(typestackentry(state.localtypes[value],false));
				break;
			}
			case 0x63://setlocal
			{
				int32_t p = code.tellg();
				uint32_t value =code.readu30();
				assert_and_throw(value < mi->body->getReturnValuePos());
#ifdef ENABLE_OPTIMIZATION
				if (state.setlocal_handled.find(p)!=state.setlocal_handled.end()
						|| checkInitializeLocalToConstant(state,value))
				{
					state.setlocal_handled.erase(p);
					removetypestack(typestack,1);
					opcode_skipped=true;
					break;
				}
				removeInitializeLocalToConstant(state,value);
				if (state.operandlist.size() && state.operandlist.back().type==OP_LOCAL && state.operandlist.back().index==(int32_t)value)
				{
					// getlocal followed by setlocal on same index, can be skipped
					state.operandlist.back().removeArg(state);
					state.operandlist.pop_back();
					removetypestack(typestack,1);
					opcode_skipped=true;
					removeInitializeLocalToConstant(state,value);
					break;
				}
				setOperandModified(state,OP_LOCAL,value);
				if (state.operandlist.size() && state.operandlist.back().duparg1)
				{
					// the argument to set is the argument of a dup, so we just modify the localresult of the dup and skip this opcode
					state.preloadedcode.at(state.operandlist.back().preloadedcodepos-1).pcode.local3.pos =value;
					state.preloadedcode.at(state.operandlist.back().preloadedcodepos).pcode.arg3_uint =value;
					state.operandlist.back().removeArg(state);
					state.operandlist.pop_back();
					opcode_skipped=true;
				}
				else
#endif
				{
					setupInstructionOneArgumentNoResult(state,ABC_OP_OPTIMZED_SETLOCAL,opcode,code,p);
					state.preloadedcode.at(state.preloadedcode.size()-1).pcode.arg3_uint = value;
				}
				if (typestack.back().obj && typestack.back().obj->is<Class_base>())
					state.localtypes[value]=typestack.back().obj->as<Class_base>();
				else
					state.localtypes[value]=nullptr;
				removetypestack(typestack,1);
				break;
			}
			case 0x65://getscopeobject
			{
				state.preloadedcode.push_back((uint32_t)opcode);
				state.refreshOldNewPosition(code);
				uint32_t t =code.readu30();
#ifdef ENABLE_OPTIMIZATION
				assert_and_throw(t < state.scopelist.size());
				auto it = state.scopelist.begin();
				for (uint32_t i=0; i < t; i++)
					++it;
				asAtom a = (it)->object;
				Class_base* resulttype = nullptr;
				if (asAtomHandler::is<Activation_object>(a)
						|| asAtomHandler::is<Catchscope_object>(a))
					resulttype = Class<ASObject>::getRef(function->getSystemState()).getPtr();
				int32_t p = code.tellg();
				if (state.jumptargets.find(p) == state.jumptargets.end() && code.peekbyteFromPosition(p) == 0x6c)//getslot
				{
					// common case getScopeObject followed by getSlot (accessing variables of ActivationObject)
					state.refreshOldNewPosition(code);
					code.readbyte();
					int32_t tslot =code.readu30();
					assert_and_throw(tslot);
					if (asAtomHandler::is<Activation_object>(a))
						resulttype = asAtomHandler::as<Activation_object>(a)->getSlotType(tslot,state.mi->context);
					if (checkForLocalResult(state,code,0,resulttype))
						state.preloadedcode.at(state.preloadedcode.size()-1).opcode = (uint32_t)ABC_OP_OPTIMZED_GETSLOTFROMSCOPEOBJECT+1;
					else
					{
						state.preloadedcode.at(state.preloadedcode.size()-1).opcode = (uint32_t)ABC_OP_OPTIMZED_GETSLOTFROMSCOPEOBJECT;
						clearOperands(state,true,&lastlocalresulttype);
					}
					state.preloadedcode.at(state.preloadedcode.size()-1).pcode.arg1_uint =t;
					state.preloadedcode.at(state.preloadedcode.size()-1).pcode.arg2_uint =tslot-1;
					typestack.push_back(typestackentry(resulttype,false));
					break;
				}
				typestack.push_back(typestackentry(resulttype,false));
				if (checkForLocalResult(state,code,0,resulttype))
				{
					if (asAtomHandler::is<Activation_object>(a)
							|| asAtomHandler::is<Catchscope_object>(a))
						state.operandlist.at(state.operandlist.size()-1).instance = asAtomHandler::getObjectNoCheck(a);
					state.preloadedcode.at(state.preloadedcode.size()-1).pcode.func=abc_getscopeobject_localresult;
					state.preloadedcode.at(state.preloadedcode.size()-1).pcode.arg2_uint=t;
				}
				else
#endif
				{
					state.preloadedcode.at(state.preloadedcode.size()-1).pcode.arg2_uint=t;
					clearOperands(state,true,&lastlocalresulttype);
				}
				break;
			}
			case 0x6c://getslot
			{
				removetypestack(typestack,1);
				int32_t p = code.tellg();
				state.checkClearOperands(p,&lastlocalresulttype);
				int32_t t =code.readu30();
				assert_and_throw(t);
				Class_base* resulttype = nullptr;
#ifdef ENABLE_OPTIMIZATION
				if (state.operandlist.size() > 0)
				{
					auto it = state.operandlist.rbegin();
					bool hasslotresult = it->instance && (it->instance->is<Activation_object>() || it->instance->is<Catchscope_object>());
					if (it->type == OP_LOCAL)
					{
						if (state.unchangedlocals.find(it->index) != state.unchangedlocals.end())
						{
							if (hasslotresult)
								resulttype = it->instance->getSlotType(t,state.mi->context);
							else if (it->objtype && !it->objtype->isInterface && it->objtype->isInitialized())
							{
								if (it->objtype->is<Class_inherit>())
									it->objtype->as<Class_inherit>()->checkScriptInit();
								// check if we can replace getProperty by getSlot
								resulttype = getSlotResultTypeFromClass(it->objtype, t, state);
							}
							if (resulttype
								&& resulttype != Class_object::getRef(state.function->getSystemState()).getPtr())
							{
								it->removeArg(state);
								state.operandlist.pop_back();
								addCachedSlot(state,it->index,t,code,resulttype);
								typestack.push_back(typestackentry(resulttype,false));
								break;
							}
						}
					}
					else if (it->type != OP_CACHED_SLOT && it->type != OP_LOCAL)
					{
						asAtom* o = mi->context->getConstantAtom(it->type,it->index);
						if (asAtomHandler::getObject(*o) && (
									asAtomHandler::getObjectNoCheck(*o)->getSlotKind(t) == TRAIT_KIND::CONSTANT_TRAIT
									|| (asAtomHandler::getObjectNoCheck(*o)->is<Global>() && asAtomHandler::getObjectNoCheck(*o)->getSlotKind(t) == TRAIT_KIND::DECLARED_TRAIT)
									))
						{
							variable* v = asAtomHandler::getObjectNoCheck(*o)->getSlotVar(t);
							resulttype = asAtomHandler::getObject(*o)->getSlotType(t,state.mi->context);
							if (resulttype && !resulttype->isConstructed())
								resulttype=nullptr;

							asAtom cval = v->getVar(state.worker,UINT16_MAX);
							if (!asAtomHandler::isNull(cval) && !asAtomHandler::isUndefined(cval)
									&& (v->kind == TRAIT_KIND::CONSTANT_TRAIT
										|| asAtomHandler::is<Class_base>(cval)
										))
							{
								it->removeArg(state);
								state.operandlist.pop_back();
								addCachedConstant(state,mi, cval,code);
								typestack.push_back(typestackentry(resulttype,false));
								break;
							}
						}
					}
					if (hasslotresult)
					{
						resulttype = it->instance->getSlotType(t,state.mi->context);
					}
					else if (it->objtype && !it->objtype->isInterface && it->objtype->isInitialized())
					{
						if (it->objtype->is<Class_inherit>())
							it->objtype->as<Class_inherit>()->checkScriptInit();
						resulttype = getSlotResultTypeFromClass(it->objtype, t, state);
					}
				}
#endif
				if (setupInstructionOneArgument(state,ABC_OP_OPTIMZED_GETSLOT,opcode,code,true,false,resulttype,p,true,false,false,true,ABC_OP_OPTIMZED_GETSLOT_SETSLOT))
					state.preloadedcode.at(state.preloadedcode.size()-1).pcode.arg2_uint =t-1;
				else
					state.preloadedcode.at(state.preloadedcode.size()-1).pcode.arg2_uint =t;
				typestack.push_back(typestackentry(resulttype,false));
				break;
			}
			case 0x6d://setslot
			{
				removetypestack(typestack,2);
				int32_t p = code.tellg();
				state.checkClearOperands(p,&lastlocalresulttype);
				int32_t t =code.readu30();
				if (!setupInstructionTwoArgumentsNoResult(state,ABC_OP_OPTIMZED_SETSLOT,opcode,code))
					clearOperands(state,true,&lastlocalresulttype);
				state.preloadedcode.at(state.preloadedcode.size()-1).pcode.arg3_uint=t-1;
				break;
			}
			case 0x80://coerce
			{
				int32_t p = code.tellg();
				int32_t t = code.readu30();
				bool skip = false;
				ASObject* tobj = nullptr;
				multiname* name =  mi->context->getMultinameImpl(asAtomHandler::nullAtom,nullptr,t,false);
				assert_and_throw(name->isStatic);
#ifdef ENABLE_OPTIMIZATION
				// skip coerce followed by coerce
				skip = (state.jumptargets.find(p) == state.jumptargets.end() && code.peekbyte() == 0x80);//coerce
				if (!skip && state.jumptargets.find(p) == state.jumptargets.end() && typestack.size() > 0)
				{
					Type* tp = Type::getTypeFromMultiname(name, mi->context);
					Class_base* cls =dynamic_cast<Class_base*>(tp);
					tobj = typestack.back().obj;
					if (tobj && tobj->is<Class_base>())
					{
						if (!state.operandlist.empty())
						{
							// coerce follows push_<x> opcode, value may be converted to matching constant and opcode can be skipped
							asAtom v=asAtomHandler::invalidAtom;
							switch(state.operandlist.back().type)
							{
								case OP_FALSE:
								case OP_TRUE:
								case OP_BYTE:
								case OP_SHORT:
								case OP_INTEGER:
								case OP_UINTEGER:
								case OP_NAN:
								case OP_DOUBLE:
									if (cls == Class<Number>::getRef(mi->context->applicationDomain->getSystemState()).getPtr())
									{
										asAtom* pv = mi->context->getConstantAtom(state.operandlist.back().type,state.operandlist.back().index);
										v = asAtomHandler::fromNumber(wrk,asAtomHandler::toNumber(*pv),true);
										skip=true;
									}
									else if (cls  == Class<Integer>::getRef(mi->context->applicationDomain->getSystemState()).getPtr())
									{
										asAtom* pv = mi->context->getConstantAtom(state.operandlist.back().type,state.operandlist.back().index);
										v = asAtomHandler::fromInt(asAtomHandler::toInt(*pv));
										skip=true;
									}
									else if (cls  == Class<UInteger>::getRef(mi->context->applicationDomain->getSystemState()).getPtr())
									{
										asAtom* pv = mi->context->getConstantAtom(state.operandlist.back().type,state.operandlist.back().index);
										v = asAtomHandler::fromUInt(asAtomHandler::toUInt(*pv));
										skip=true;
									}
									else if (cls == Class<Boolean>::getRef(mi->context->applicationDomain->getSystemState()).getPtr())
									{
										asAtom* pv = mi->context->getConstantAtom(state.operandlist.back().type,state.operandlist.back().index);
										v = asAtomHandler::fromBool(asAtomHandler::toInt(*pv));
										skip = true;
									}
									break;
								default:
									break;
							}
							if (skip)
							{
								removeOperands(state,false,nullptr,1);
								state.preloadedcode.pop_back();
								addCachedConstant(state,mi,v,code);
								tobj = (Class_base*)cls;
							}
						}
						if (!skip)
						{
							if (tobj == Class<Number>::getRef(mi->context->applicationDomain->getSystemState()).getPtr() ||
									tobj  == Class<Integer>::getRef(mi->context->applicationDomain->getSystemState()).getPtr() ||
									tobj  == Class<UInteger>::getRef(mi->context->applicationDomain->getSystemState()).getPtr() ||
									tobj  == Class<Boolean>::getRef(mi->context->applicationDomain->getSystemState()).getPtr())
								skip = cls == Class<Number>::getRef(mi->context->applicationDomain->getSystemState()).getPtr() || cls == Class<Integer>::getRef(mi->context->applicationDomain->getSystemState()).getPtr() || cls == Class<UInteger>::getRef(mi->context->applicationDomain->getSystemState()).getPtr();
							else if (tobj != Class<ASString>::getRef(mi->context->applicationDomain->getSystemState()).getPtr())
								skip = tobj==cls;
						}
					}
					if (skip || cls != nullptr)
						tobj = (Class_base*)cls;
				}
				if (!skip && state.operandlist.size()>0 && state.operandlist.back().type==OP_NULL  // coerce following a pushnull can be skipped if not coercing to a numeric value
						&& tobj != Class<Number>::getRef(mi->context->applicationDomain->getSystemState()).getPtr()
						&& tobj != Class<Integer>::getRef(mi->context->applicationDomain->getSystemState()).getPtr()
						&& tobj != Class<UInteger>::getRef(mi->context->applicationDomain->getSystemState()).getPtr()
						&& tobj != Class<Boolean>::getRef(mi->context->applicationDomain->getSystemState()).getPtr())
					skip=true;
				if (skip)
				{
					// check if this coerce is followed by a non-skippable jump
					uint8_t bjump = code.peekbyte();
					uint32_t pos = code.tellg()+1;
					skipjump(state,bjump,code,pos,false);
					skip = jumppoints.find(pos)==jumppoints.end();
				}
#endif
				if (!skip)
				{
					setupInstructionOneArgument(state,ABC_OP_OPTIMZED_COERCE,opcode,code,true,true,tobj && tobj->is<Class_base>() ? tobj->as<Class_base>() : nullptr,p,true);
					state.preloadedcode.at(state.preloadedcode.size()-1).pcode.cachedmultiname2 = name;
				}
				else
					opcode_skipped=true;
				if (state.operandlist.size() && tobj && tobj->is<Class_base>())
					state.operandlist.back().objtype=tobj->as<Class_base>();
				if (tobj)
				{
					removetypestack(typestack,1);
					typestack.push_back(typestackentry(tobj,false));
				}
				break;
			}
			case 0xd0://getlocal_0
			case 0xd1://getlocal_1
			case 0xd2://getlocal_2
			case 0xd3://getlocal_3
			{
				int32_t p = code.tellg();
#ifdef ENABLE_OPTIMIZATION
				if ((state.jumptargets.find(code.tellg()+1) == state.jumptargets.end()) && code.peekbyte()==0x29) // pop
				{
					code.readbyte();
					break;
				}
				removeInitializeLocalToConstant(state,opcode-0xd0);
				if (p==1 && opcode == 0xd0) //getlocal_0
				{
					if (code.peekbyte() == 0x30) // pushscope
					{
						// function begins with getlocal_0 and pushscope, can be skipped
						state.refreshOldNewPosition(code);
						code.readbyte();
						state.refreshOldNewPosition(code);
						mi->needsscope=true;
						state.scopelist.push_back(scope_entry(function->inClass ? asAtomHandler::fromObjectNoPrimitive(function->inClass) : asAtomHandler::invalidAtom,false));
						break;
					}
				}
				if (((uint32_t)opcode)-0xd0 >= mi->body->local_count)
				{
					// this may happen in unreachable obfuscated code, so we just ignore the opcode
					LOG(LOG_ERROR,"preload getlocal with argument > local_count:"<<mi->body->local_count<<" "<<hex<<(int)opcode);
					state.refreshOldNewPosition(code);
					state.checkClearOperands(p,&lastlocalresulttype);
				}
#endif
				state.preloadedcode.push_back((uint32_t)opcode);
				state.refreshOldNewPosition(code);
				state.checkClearOperands(p,&lastlocalresulttype);
				state.operandlist.push_back(operands(OP_LOCAL,state.localtypes[((uint32_t)opcode)-0xd0],((uint32_t)opcode)-0xd0,1,state.preloadedcode.size()-1));
				typestack.push_back(typestackentry(state.localtypes[((uint32_t)opcode)-0xd0],false));
				break;
			}
			case 0x2a://dup
				preload_dup(state,typestack,code,opcode,&lastlocalresulttype,jumppositions,jumpstartpositions);
				break;
			case 0x01://bkpt
			case 0x02://nop
			case 0x82://coerce_a
			case 0x09://label
			{
				//skip
				int32_t p = code.tellg();
				state.refreshOldNewPosition(code);
				if (state.jumptargets.find(p) != state.jumptargets.end())
				{
					state.jumptargeteresulttypes.erase(p+1);
					state.jumptargets[p+1]++;
				}
				opcode_skipped=true;
				break;
			}
			case 0xf0://debugline
			case 0xf1://debugfile
			case 0xf2://bkptline
			{
				//skip
				int32_t p = code.tellg();
				state.refreshOldNewPosition(code);
				code.readu30();
				if (state.jumptargets.find(p) != state.jumptargets.end())
				{
					state.jumptargeteresulttypes.erase(code.tellg()+1);
					state.jumptargets[code.tellg()+1]++;
					clearOperands(state,true,&lastlocalresulttype);
				}
				opcode_skipped=true;
				break;
			}
			case 0x0c://ifnlt
				state.canlocalinitialize.clear();
				removetypestack(typestack,2);
				setupInstructionTwoArgumentsNoResult(state,ABC_OP_OPTIMZED_IFNLT,opcode,code);
				jumppositions[state.preloadedcode.size()-1] = code.reads24();
				jumpstartpositions[state.preloadedcode.size()-1] = code.tellg();
				clearOperands(state,true,&lastlocalresulttype);
				break;
			case 0x0f://ifnge
				state.canlocalinitialize.clear();
				removetypestack(typestack,2);
				setupInstructionTwoArgumentsNoResult(state,ABC_OP_OPTIMZED_IFNGE,opcode,code);
				jumppositions[state.preloadedcode.size()-1] = code.reads24();
				jumpstartpositions[state.preloadedcode.size()-1] = code.tellg();
				clearOperands(state,true,&lastlocalresulttype);
				break;
			case 0x0d://ifnle
				state.canlocalinitialize.clear();
				removetypestack(typestack,2);
				setupInstructionTwoArgumentsNoResult(state,ABC_OP_OPTIMZED_IFNLE,opcode,code);
				jumppositions[state.preloadedcode.size()-1] = code.reads24();
				jumpstartpositions[state.preloadedcode.size()-1] = code.tellg();
				clearOperands(state,true,&lastlocalresulttype);
				break;
			case 0x0e://ifngt
				state.canlocalinitialize.clear();
				removetypestack(typestack,2);
				setupInstructionTwoArgumentsNoResult(state,ABC_OP_OPTIMZED_IFNGT,opcode,code);
				jumppositions[state.preloadedcode.size()-1] = code.reads24();
				jumpstartpositions[state.preloadedcode.size()-1] = code.tellg();
				clearOperands(state,true,&lastlocalresulttype);
				break;
			case 0x10://jump
			{
				state.canlocalinitialize.clear();
				uint32_t pstart= code.tellg();
				int32_t j = code.reads24();
#ifdef ENABLE_OPTIMIZATION
				if (state.preloadedcode.size()
						&& state.preloadedcode.back().opcode == 0x03 //throw
						&& state.jumptargets.find(pstart) == state.jumptargets.end())
				{
					// jump follows throw opcode and is no jump target -> unreachable, jump can be skipped
					state.refreshOldNewPosition(code);
					opcode_skipped=true;
					break;
				}
				state.oldnewpositions[pstart] = (int32_t)state.preloadedcode.size();
				uint32_t p = code.tellg();
				bool hastargets = j < 0;
				int32_t nextreachable = j;
				if (!hastargets)
				{
					// check if the code following the jump is unreachable
					for (int32_t i = 0; i <= j; i++)
					{
						if (state.jumptargets.find(p+i) != state.jumptargets.end())
						{
							hastargets = true;
							nextreachable =i-1;
							break;
						}
					}
				}
				if (hastargets)
				{
					jumppositions[state.preloadedcode.size()] = j;
					jumpstartpositions[state.preloadedcode.size()] = code.tellg();
					state.preloadedcode.push_back((uint32_t)opcode);
					state.refreshOldNewPosition(code);
					// skip unreachable code
					for (int32_t i = 0; i < nextreachable; i++)
						code.readbyte();
					clearOperands(state,true,&lastlocalresulttype);
				}
				else
				{
					// skip complete jump
					for (int32_t i = 0; i < j; i++)
						code.readbyte();
					auto it = state.jumptargets.find(p+j+1);
					if (it != state.jumptargets.end() && it->second > 1)
						state.jumptargets[p+j+1]--;
					else
						state.jumptargets.erase(p+j+1);
					state.refreshOldNewPosition(code);
					opcode_skipped=true;
				}
#else
				jumppositions[state.preloadedcode.size()] = j;
				jumpstartpositions[state.preloadedcode.size()] = code.tellg();
				state.preloadedcode.push_back((uint32_t)opcode);
				state.refreshOldNewPosition(code);
				clearOperands(state,true,&lastlocalresulttype);
#endif
				break;
			}
			case 0x11://iftrue
			{
				state.canlocalinitialize.clear();
				removetypestack(typestack,1);
				int32_t p = code.tellg();
				int j = code.reads24();
				int32_t p1 = code.tellg();
#ifdef ENABLE_OPTIMIZATION
				if (state.jumptargets.find(p) == state.jumptargets.end())
				{
					// iftrue following pushtrue is always true, so the following code is unreachable
					if (j > 0 && prevopcode==0x26) //pushtrue
					{
						state.refreshOldNewPosition(code);
						skipunreachablecode(state,code);
						auto it = state.jumptargets.find(code.tellg()+1);
						if (it != state.jumptargets.end() && it->second > 1)
							state.jumptargets[code.tellg()+1]--;
						else
							state.jumptargets.erase(code.tellg()+1);
						state.refreshOldNewPosition(code);
						if (int(code.tellg())-p1 >= j)
						{
							// remove pushtrue
							opcode_skipped=true;
							assert(state.operandlist.size() > 0);
							state.operandlist.back().removeArg(state);
							state.operandlist.pop_back();
							clearOperands(state,true,&lastlocalresulttype);
							break;
						}
					}
					setupInstructionOneArgumentNoResult(state,reverse_iftruefalse ? ABC_OP_OPTIMZED_IFFALSE : ABC_OP_OPTIMZED_IFTRUE,reverse_iftruefalse ? 0x12/*iffalse*/ : opcode,code,p);
					reverse_iftruefalse=false;
				}
				else
#endif
					state.preloadedcode.push_back((uint32_t)opcode);
				jumppositions[state.preloadedcode.size()-1] = j;
				jumpstartpositions[state.preloadedcode.size()-1] = p1;
				clearOperands(state,true,&lastlocalresulttype);
				break;
			}
			case 0x12://iffalse
			{
				state.canlocalinitialize.clear();
				removetypestack(typestack,1);
				int32_t p = code.tellg();
				int j = code.reads24();
				int32_t p1 = code.tellg();
#ifdef ENABLE_OPTIMIZATION
				if (state.jumptargets.find(p) == state.jumptargets.end())
				{
					// iffalse following pushfalse is always true, so the following code is unreachable
					if (j > 0 && prevopcode==0x27) //pushfalse
					{
						state.refreshOldNewPosition(code);
						skipunreachablecode(state,code);
						auto it = state.jumptargets.find(code.tellg()+1);
						if (it != state.jumptargets.end() && it->second > 1)
							state.jumptargets[code.tellg()+1]--;
						else
							state.jumptargets.erase(code.tellg()+1);
						state.refreshOldNewPosition(code);
						if (int(code.tellg())-p1 >= j)
						{
							// remove pushfalse
							opcode_skipped=true;
							assert(state.operandlist.size() > 0);
							state.operandlist.back().removeArg(state);
							state.operandlist.pop_back();
							clearOperands(state,true,&lastlocalresulttype);
							break;
						}
					}
					setupInstructionOneArgumentNoResult(state,reverse_iftruefalse ? ABC_OP_OPTIMZED_IFTRUE : ABC_OP_OPTIMZED_IFFALSE,reverse_iftruefalse ? 0x11/*iftrue*/ : opcode,code,p);
					reverse_iftruefalse=false;
				}
				else
#endif
					state.preloadedcode.push_back((uint32_t)opcode);
				jumppositions[state.preloadedcode.size()-1] = j;
				jumpstartpositions[state.preloadedcode.size()-1] = p1;
				clearOperands(state,true,&lastlocalresulttype);
				break;
			}
			case 0x13://ifeq
			{
				state.canlocalinitialize.clear();
				removetypestack(typestack,2);
				int32_t p = code.tellg();
				int j = code.reads24();
				int32_t p1 = code.tellg();
#ifdef ENABLE_OPTIMIZATION
				if (state.jumptargets.find(p) == state.jumptargets.end())
				{
					// if we are comparing two constants and the result is true, the following code is unreachable
					if (j > 0 && state.operandlist.size() > 1
							&& state.operandlist[state.operandlist.size()-1].type != OP_LOCAL
							&& state.operandlist[state.operandlist.size()-1].type != OP_CACHED_SLOT
							&& state.operandlist[state.operandlist.size()-2].type != OP_LOCAL
							&& state.operandlist[state.operandlist.size()-2].type != OP_CACHED_SLOT
							&& asAtomHandler::isEqual(*function->mi->context->getConstantAtom(state.operandlist[state.operandlist.size()-1].type,state.operandlist[state.operandlist.size()-1].index)
													  , wrk
													  ,*function->mi->context->getConstantAtom(state.operandlist[state.operandlist.size()-2].type,state.operandlist[state.operandlist.size()-2].index)
													  ))
					{
						state.refreshOldNewPosition(code);
						skipunreachablecode(state,code);
						auto it = state.jumptargets.find(code.tellg()+1);
						if (it != state.jumptargets.end() && it->second > 1)
							state.jumptargets[code.tellg()+1]--;
						else
							state.jumptargets.erase(code.tellg()+1);
						state.refreshOldNewPosition(code);
						if (int(code.tellg())-p1 >= j)
						{
							// remove the two arguments
							opcode_skipped=true;
							assert(state.operandlist.size() > 1);
							state.operandlist.back().removeArg(state);
							state.operandlist.pop_back();
							state.operandlist.back().removeArg(state);
							state.operandlist.pop_back();
							clearOperands(state,true,&lastlocalresulttype);
							break;
						}
						else
						{
							setupInstructionTwoArgumentsNoResult(state,ABC_OP_OPTIMZED_IFEQ,opcode,code);
							state.oldnewpositions[p1] = (int32_t)state.preloadedcode.size();
						}
					}
					else
						setupInstructionTwoArgumentsNoResult(state,ABC_OP_OPTIMZED_IFEQ,opcode,code);
				}
				else
#endif
					state.preloadedcode.push_back((uint32_t)opcode);
				jumppositions[state.preloadedcode.size()-1] = j;
				jumpstartpositions[state.preloadedcode.size()-1] = p1;
				clearOperands(state,true,&lastlocalresulttype);
				break;
			}
			case 0x14://ifne
			{
				state.canlocalinitialize.clear();
				removetypestack(typestack,2);
				int32_t p = code.tellg();
				int j = code.reads24();
				int32_t p1 = code.tellg();
#ifdef ENABLE_OPTIMIZATION
				if (state.jumptargets.find(p) == state.jumptargets.end())
				{
					// if we are comparing two constants and the result is false, the following code is unreachable
					if (j > 0 && state.operandlist.size() > 1
							&& state.operandlist[state.operandlist.size()-1].type != OP_LOCAL
							&& state.operandlist[state.operandlist.size()-1].type != OP_CACHED_SLOT
							&& state.operandlist[state.operandlist.size()-2].type != OP_LOCAL
							&& state.operandlist[state.operandlist.size()-2].type != OP_CACHED_SLOT
							&& !asAtomHandler::isEqual(*function->mi->context->getConstantAtom(state.operandlist[state.operandlist.size()-1].type,state.operandlist[state.operandlist.size()-1].index)
													   , wrk
													   ,*function->mi->context->getConstantAtom(state.operandlist[state.operandlist.size()-2].type,state.operandlist[state.operandlist.size()-2].index)
													   ))
					{
						state.refreshOldNewPosition(code);
						skipunreachablecode(state,code);
						auto it = state.jumptargets.find(code.tellg()+1);
						if (it != state.jumptargets.end() && it->second > 1)
							state.jumptargets[code.tellg()+1]--;
						else
							state.jumptargets.erase(code.tellg()+1);
						state.refreshOldNewPosition(code);
						if (int(code.tellg())-p1 >= j)
						{
							// remove the two arguments
							opcode_skipped=true;
							assert(state.operandlist.size() > 1);
							state.operandlist.back().removeArg(state);
							state.operandlist.pop_back();
							state.operandlist.back().removeArg(state);
							state.operandlist.pop_back();
							clearOperands(state,true,&lastlocalresulttype);
							break;
						}
						else
						{
							setupInstructionTwoArgumentsNoResult(state,ABC_OP_OPTIMZED_IFNE,opcode,code);
							state.oldnewpositions[p1] = (int32_t)state.preloadedcode.size();
						}
					}
					else
						setupInstructionTwoArgumentsNoResult(state,ABC_OP_OPTIMZED_IFNE,opcode,code);
				}
				else
#endif
					state.preloadedcode.push_back((uint32_t)opcode);
				jumppositions[state.preloadedcode.size()-1] = j;
				jumpstartpositions[state.preloadedcode.size()-1] = p1;
				clearOperands(state,true,&lastlocalresulttype);
				break;
			}
			case 0x15://iflt
				state.canlocalinitialize.clear();
				removetypestack(typestack,2);
				setupInstructionTwoArgumentsNoResult(state,ABC_OP_OPTIMZED_IFLT,opcode,code);
				jumppositions[state.preloadedcode.size()-1] = code.reads24();
				jumpstartpositions[state.preloadedcode.size()-1] = code.tellg();
				clearOperands(state,true,&lastlocalresulttype);
				break;
			case 0x16://ifle
				state.canlocalinitialize.clear();
				removetypestack(typestack,2);
				setupInstructionTwoArgumentsNoResult(state,ABC_OP_OPTIMZED_IFLE,opcode,code);
				jumppositions[state.preloadedcode.size()-1] = code.reads24();
				jumpstartpositions[state.preloadedcode.size()-1] = code.tellg();
				clearOperands(state,true,&lastlocalresulttype);
				break;
			case 0x17://ifgt
				state.canlocalinitialize.clear();
				removetypestack(typestack,2);
				setupInstructionTwoArgumentsNoResult(state,ABC_OP_OPTIMZED_IFGT,opcode,code);
				jumppositions[state.preloadedcode.size()-1] = code.reads24();
				jumpstartpositions[state.preloadedcode.size()-1] = code.tellg();
				clearOperands(state,true,&lastlocalresulttype);
				break;
			case 0x18://ifge
				state.canlocalinitialize.clear();
				removetypestack(typestack,2);
				setupInstructionTwoArgumentsNoResult(state,ABC_OP_OPTIMZED_IFGE,opcode,code);
				jumppositions[state.preloadedcode.size()-1] = code.reads24();
				jumpstartpositions[state.preloadedcode.size()-1] = code.tellg();
				clearOperands(state,true,&lastlocalresulttype);
				break;
			case 0x19://ifstricteq
				state.canlocalinitialize.clear();
				removetypestack(typestack,2);
				setupInstructionTwoArgumentsNoResult(state,ABC_OP_OPTIMZED_IFSTRICTEQ,opcode,code);
				jumppositions[state.preloadedcode.size()-1] = code.reads24();
				jumpstartpositions[state.preloadedcode.size()-1] = code.tellg();
				clearOperands(state,true,&lastlocalresulttype);
				break;
			case 0x1a://ifstrictne
				state.canlocalinitialize.clear();
				setupInstructionTwoArgumentsNoResult(state,ABC_OP_OPTIMZED_IFSTRICTNE,opcode,code);
				jumppositions[state.preloadedcode.size()-1] = code.reads24();
				jumpstartpositions[state.preloadedcode.size()-1] = code.tellg();
				clearOperands(state,true,&lastlocalresulttype);
				break;
			case 0x1b://lookupswitch
			{
				state.canlocalinitialize.clear();
				removetypestack(typestack,1);
				int32_t p = code.tellg()-1;
				if (setupInstructionOneArgumentNoResult(state,ABC_OP_OPTIMZED_LOOKUPSWITCH,opcode,code,code.tellg()))
					state.oldnewpositions[p] = (int32_t)state.preloadedcode.size()-1;
				state.refreshOldNewPosition(code);
				switchpositions[state.preloadedcode.size()] = code.reads24();
				switchstartpositions[state.preloadedcode.size()] = p;
				state.preloadedcode.push_back(0);
				state.refreshOldNewPosition(code);
				uint32_t count = code.readu30();
				state.preloadedcode.push_back(0);
				state.preloadedcode.back().pcode.arg3_uint=count;
				for(unsigned int i=0;i<count+1;i++)
				{
					state.refreshOldNewPosition(code);
					switchpositions[state.preloadedcode.size()] = code.reads24();
					switchstartpositions[state.preloadedcode.size()] = p;
					state.preloadedcode.push_back(0);
				}
				clearOperands(state,true,&lastlocalresulttype);
				break;
			}
			case 0x20://pushnull
				if ((state.jumptargets.find(code.tellg()+1) == state.jumptargets.end()) && code.peekbyte()==0x29) // pop
				{
					code.readbyte();
					break;
				}
				state.preloadedcode.push_back((uint32_t)opcode);
				if (state.jumptargets.find(code.tellg()) == state.jumptargets.end())
					state.operandlist.push_back(operands(OP_NULL, nullptr,0,1,state.preloadedcode.size()-1));
				else
					clearOperands(state,true,&lastlocalresulttype);
				typestack.push_back(typestackentry(nullptr,false));
				break;
			case 0x21://pushundefined
				if ((state.jumptargets.find(code.tellg()+1) == state.jumptargets.end()) && code.peekbyte()==0x29) // pop
				{
					code.readbyte();
					break;
				}
				state.preloadedcode.push_back((uint32_t)opcode);
				if (state.jumptargets.find(code.tellg()) == state.jumptargets.end())
					state.operandlist.push_back(operands(OP_UNDEFINED, nullptr,0,1,state.preloadedcode.size()-1));
				else
					clearOperands(state,true,&lastlocalresulttype);
				typestack.push_back(typestackentry(nullptr,false));
				break;
			case 0x24://pushbyte
			{
				int32_t p = code.tellg();
				int32_t value = (int32_t)(int8_t)code.readbyte();
				if ((state.jumptargets.find(code.tellg()+1) == state.jumptargets.end()) && code.peekbyte()==0x29) // pop
				{
					code.readbyte();
					break;
				}
				state.preloadedcode.push_back((uint32_t)opcode);
				uint8_t index = value;
				state.preloadedcode.back().pcode.arg3_int=value;
				state.checkClearOperands(p,&lastlocalresulttype);
				if (state.jumptargets.find(code.tellg()) != state.jumptargets.end() && code.peekbyte() == 0x74)//convert_u
				{
					state.operandlist.push_back(operands(OP_UINTEGER,Class<UInteger>::getRef(mi->context->applicationDomain->getSystemState()).getPtr(),(uint32_t)value,1,state.preloadedcode.size()-1));
					typestack.push_back(typestackentry(Class<UInteger>::getRef(function->getSystemState()).getPtr(),false));
					code.readbyte();
				}
				else
				{
					state.operandlist.push_back(operands(OP_BYTE,Class<Integer>::getRef(mi->context->applicationDomain->getSystemState()).getPtr(),index,1,state.preloadedcode.size()-1));
					typestack.push_back(typestackentry(Class<Integer>::getRef(function->getSystemState()).getPtr(),false));
				}
				break;
			}
			case 0x25://pushshort
			{
				int32_t p = code.tellg();
				int32_t value = (int32_t)(int16_t)code.readu32();
				if ((state.jumptargets.find(code.tellg()+1) == state.jumptargets.end()) && code.peekbyte()==0x29) // pop
				{
					code.readbyte();
					break;
				}
				state.preloadedcode.push_back((uint32_t)opcode);
				uint16_t index = value;
				state.preloadedcode.back().pcode.arg3_int=value;
				state.checkClearOperands(p,&lastlocalresulttype);
				state.operandlist.push_back(operands(OP_SHORT,Class<Integer>::getRef(mi->context->applicationDomain->getSystemState()).getPtr(),index,1,state.preloadedcode.size()-1));
				typestack.push_back(typestackentry(Class<Integer>::getRef(function->getSystemState()).getPtr(),false));
				break;
			}
			case 0x26://pushtrue
			{
				if ((state.jumptargets.find(code.tellg()+1) == state.jumptargets.end()) && code.peekbyte()==0x29) // pop
				{
					code.readbyte();
					break;
				}
				int32_t p = code.tellg();
				state.preloadedcode.push_back((uint32_t)opcode);
				state.refreshOldNewPosition(code);
				state.checkClearOperands(p,&lastlocalresulttype);
				state.operandlist.push_back(operands(OP_TRUE, Class<Boolean>::getRef(mi->context->applicationDomain->getSystemState()).getPtr(),0,1,state.preloadedcode.size()-1));
				typestack.push_back(typestackentry(Class<Boolean>::getRef(function->getSystemState()).getPtr(),false));
				break;
			}
			case 0x27://pushfalse
			{
				if ((state.jumptargets.find(code.tellg()+1) == state.jumptargets.end()) && code.peekbyte()==0x29) // pop
				{
					code.readbyte();
					break;
				}
				int32_t p = code.tellg();
				state.preloadedcode.push_back((uint32_t)opcode);
				state.refreshOldNewPosition(code);
				state.checkClearOperands(p,&lastlocalresulttype);
				state.operandlist.push_back(operands(OP_FALSE, Class<Boolean>::getRef(mi->context->applicationDomain->getSystemState()).getPtr(),0,1,state.preloadedcode.size()-1));
				typestack.push_back(typestackentry(Class<Boolean>::getRef(function->getSystemState()).getPtr(),false));
				break;
			}
			case 0x28://pushnan
			{
				if ((state.jumptargets.find(code.tellg()+1) == state.jumptargets.end()) && code.peekbyte()==0x29) // pop
				{
					code.readbyte();
					break;
				}
				int32_t p = code.tellg();
				state.preloadedcode.push_back((uint32_t)opcode);
				state.refreshOldNewPosition(code);
				state.checkClearOperands(p,&lastlocalresulttype);
				state.operandlist.push_back(operands(OP_NAN, Class<Number>::getRef(mi->context->applicationDomain->getSystemState()).getPtr(),0,1,state.preloadedcode.size()-1));
				typestack.push_back(typestackentry(Class<Number>::getRef(function->getSystemState()).getPtr(),false));
				break;
			}
			case 0x2c://pushstring
			{
				int32_t p = code.tellg();
				uint32_t value = code.readu30();
				if ((state.jumptargets.find(code.tellg()+1) == state.jumptargets.end()) && code.peekbyte()==0x29) // pop
				{
					code.readbyte();
					break;
				}
				state.preloadedcode.push_back((uint32_t)opcode);
				state.preloadedcode.back().pcode.arg3_uint = value;
				state.checkClearOperands(p,&lastlocalresulttype);
				state.operandlist.push_back(operands(OP_STRING,Class<ASString>::getRef(mi->context->applicationDomain->getSystemState()).getPtr(),value,1,state.preloadedcode.size()-1));
				typestack.push_back(typestackentry(Class<ASString>::getRef(function->getSystemState()).getPtr(),false));
				break;
			}
			case 0x2d://pushint
			{
				int32_t p = code.tellg();
				uint32_t value = code.readu30();
				if ((state.jumptargets.find(code.tellg()+1) == state.jumptargets.end()) && code.peekbyte()==0x29) // pop
				{
					code.readbyte();
					break;
				}
				state.preloadedcode.push_back((uint32_t)opcode);
				state.preloadedcode.back().pcode.arg3_uint = value;
				state.checkClearOperands(p,&lastlocalresulttype);
				state.operandlist.push_back(operands(OP_INTEGER,Class<Integer>::getRef(mi->context->applicationDomain->getSystemState()).getPtr(),value,1,state.preloadedcode.size()-1));
				typestack.push_back(typestackentry(Class<Integer>::getRef(function->getSystemState()).getPtr(),false));
				break;
			}
			case 0x2e://pushuint
			{
				int32_t p = code.tellg();
				uint32_t value = code.readu30();
				if ((state.jumptargets.find(code.tellg()+1) == state.jumptargets.end()) && code.peekbyte()==0x29) // pop
				{
					code.readbyte();
					break;
				}
				state.preloadedcode.push_back((uint32_t)opcode);
				state.preloadedcode.back().pcode.arg3_uint = value;
				state.checkClearOperands(p,&lastlocalresulttype);
				state.operandlist.push_back(operands(OP_UINTEGER,Class<UInteger>::getRef(mi->context->applicationDomain->getSystemState()).getPtr(),value,1,state.preloadedcode.size()-1));
				typestack.push_back(typestackentry(Class<UInteger>::getRef(function->getSystemState()).getPtr(),false));
				break;
			}
			case 0x2f://pushdouble
			{
				int32_t p = code.tellg();
				uint32_t value = code.readu30();
				if ((state.jumptargets.find(code.tellg()+1) == state.jumptargets.end()) && code.peekbyte()==0x29) // pop
				{
					code.readbyte();
					break;
				}
				state.preloadedcode.push_back((uint32_t)opcode);
				state.preloadedcode.back().pcode.arg3_uint = value;
				state.checkClearOperands(p,&lastlocalresulttype);
				state.operandlist.push_back(operands(OP_DOUBLE,Class<Number>::getRef(mi->context->applicationDomain->getSystemState()).getPtr(),value,1,state.preloadedcode.size()-1));
				typestack.push_back(typestackentry(Class<Number>::getRef(function->getSystemState()).getPtr(),false));
				break;
			}
			case 0x30://pushscope
				if (typestack.back().obj)
					state.scopelist.push_back(scope_entry(asAtomHandler::fromObjectNoPrimitive(typestack.back().obj),false));
				else
					state.scopelist.push_back(scope_entry(asAtomHandler::invalidAtom,false));
				removetypestack(typestack,1);
				setupInstructionOneArgumentNoResult(state,ABC_OP_OPTIMZED_PUSHSCOPE,opcode,code,code.tellg());
				break;
			case 0x31://pushnamespace
			{
				int32_t p = code.tellg();
				uint32_t value = code.readu30();
				if ((state.jumptargets.find(code.tellg()+1) == state.jumptargets.end()) && code.peekbyte()==0x29) // pop
				{
					code.readbyte();
					break;
				}
				state.preloadedcode.push_back((uint32_t)opcode);
				state.preloadedcode.back().pcode.arg3_uint = value;
				state.checkClearOperands(p,&lastlocalresulttype);
				state.operandlist.push_back(operands(OP_NAMESPACE,Class<Namespace>::getRef(mi->context->applicationDomain->getSystemState()).getPtr(),value,1,state.preloadedcode.size()-1));
				typestack.push_back(typestackentry(Class<Namespace>::getRef(function->getSystemState()).getPtr(),false));
				break;
			}
			case 0x32://hasnext2
			{
				state.preloadedcode.push_back((uint32_t)opcode);
				state.refreshOldNewPosition(code);
				state.preloadedcode.back().pcode.arg1_uint = code.readu30();
				state.refreshOldNewPosition(code);
				state.preloadedcode.back().pcode.arg2_uint = code.readu30();
				Class_base* resulttype = Class<Boolean>::getRef(function->getSystemState()).getPtr();
#ifdef ENABLE_OPTIMIZATION
				int32_t p = code.tellg();
				if (state.jumptargets.find(p) == state.jumptargets.end() && code.peekbyteFromPosition(p) == 0x11) //iftrue
				{
					// common case hasnext2 followed by iftrue (for each loop)
					state.refreshOldNewPosition(code);
					code.readbyte();
					state.preloadedcode.at(state.preloadedcode.size()-1).pcode.func=abc_hasnext2_iftrue;
					jumppositions[state.preloadedcode.size()-1] = code.reads24();
					jumpstartpositions[state.preloadedcode.size()-1] = code.tellg();
					state.refreshOldNewPosition(code);
					clearOperands(state,true,&lastlocalresulttype);
				}
				else if (checkForLocalResult(state,code,0,resulttype))
					state.preloadedcode.at(state.preloadedcode.size()-1).pcode.func=abc_hasnext2_localresult;
				else
#endif
					clearOperands(state,true,&lastlocalresulttype);
				typestack.push_back(typestackentry(resulttype,false));
				break;
			}
			case 0x43://callmethod
			case 0x44://callstatic
			{
				state.preloadedcode.push_back((uint32_t)opcode);
				state.refreshOldNewPosition(code);
				state.preloadedcode.back().pcode.arg1_uint = code.readu30();
				state.refreshOldNewPosition(code);
				int32_t t = code.readu30();
				state.preloadedcode.back().pcode.arg2_int = t;
				clearOperands(state,true,&lastlocalresulttype);
				removetypestack(typestack,t+1);
				typestack.push_back(typestackentry(nullptr,false));
				break;
			}
			case 0x4c://callproplex
			{
				state.preloadedcode.push_back((uint32_t)opcode);
				state.refreshOldNewPosition(code);
				state.preloadedcode.push_back(0);
				uint32_t t = code.readu30();
				state.preloadedcode.back().pcode.arg1_uint = t;
				state.refreshOldNewPosition(code);
				uint32_t t2 = code.readu30();
				state.preloadedcode.back().pcode.arg2_uint = t2;
				clearOperands(state,true,&lastlocalresulttype);
				removetypestack(typestack,mi->context->constant_pool.multinames[t].runtimeargs+t2+1);
				typestack.push_back(typestackentry(nullptr,false));
				break;
			}
			case 0x35://li8
				setupInstructionOneArgument(state,ABC_OP_OPTIMZED_LI8,opcode,code,true,true,Class<Integer>::getRef(function->getSystemState()).getPtr(),code.tellg(),true,false,false,true,ABC_OP_OPTIMZED_LI8_SETSLOT);
				removetypestack(typestack,1);
				typestack.push_back(typestackentry(Class<Integer>::getRef(function->getSystemState()).getPtr(),false));
				break;
			case 0x36://li16
				setupInstructionOneArgument(state,ABC_OP_OPTIMZED_LI16,opcode,code,true,true,Class<Integer>::getRef(function->getSystemState()).getPtr(),code.tellg(),true,false,false,true,ABC_OP_OPTIMZED_LI16_SETSLOT);
				removetypestack(typestack,1);
				typestack.push_back(typestackentry(Class<Integer>::getRef(function->getSystemState()).getPtr(),false));
				break;
			case 0x37://li32
				setupInstructionOneArgument(state,ABC_OP_OPTIMZED_LI32,opcode,code,true,true,Class<Integer>::getRef(function->getSystemState()).getPtr(),code.tellg(),true,false,false,true,ABC_OP_OPTIMZED_LI32_SETSLOT);
				removetypestack(typestack,1);
				typestack.push_back(typestackentry(Class<Integer>::getRef(function->getSystemState()).getPtr(),false));
				break;
			case 0x38://lf32
				setupInstructionOneArgument(state,ABC_OP_OPTIMZED_LF32,opcode,code,true,true,Class<Number>::getRef(function->getSystemState()).getPtr(),code.tellg(),true,false,false,true,ABC_OP_OPTIMZED_LF32_SETSLOT);
				removetypestack(typestack,1);
				typestack.push_back(typestackentry(Class<Number>::getRef(function->getSystemState()).getPtr(),false));
				break;
			case 0x39://lf64
				setupInstructionOneArgument(state,ABC_OP_OPTIMZED_LF64,opcode,code,true,true,Class<Number>::getRef(function->getSystemState()).getPtr(),code.tellg(),true,false,false,true,ABC_OP_OPTIMZED_LF64_SETSLOT);
				removetypestack(typestack,1);
				typestack.push_back(typestackentry(Class<Number>::getRef(function->getSystemState()).getPtr(),false));
				break;
			case 0x3a://si8
				if (!setupInstructionTwoArgumentsNoResult(state,ABC_OP_OPTIMZED_SI8,opcode,code))
					clearOperands(state,true,&lastlocalresulttype);
				removetypestack(typestack,2);
				break;
			case 0x3b://si16
				if (!setupInstructionTwoArgumentsNoResult(state,ABC_OP_OPTIMZED_SI16,opcode,code))
					clearOperands(state,true,&lastlocalresulttype);
				removetypestack(typestack,2);
				break;
			case 0x3c://si32
				if (!setupInstructionTwoArgumentsNoResult(state,ABC_OP_OPTIMZED_SI32,opcode,code))
					clearOperands(state,true,&lastlocalresulttype);
				removetypestack(typestack,2);
				break;
			case 0x3d://sf32
				if (!setupInstructionTwoArgumentsNoResult(state,ABC_OP_OPTIMZED_SF32,opcode,code))
					clearOperands(state,true,&lastlocalresulttype);
				removetypestack(typestack,2);
				break;
			case 0x3e://sf64
				if (!setupInstructionTwoArgumentsNoResult(state,ABC_OP_OPTIMZED_SF64,opcode,code))
					clearOperands(state,true,&lastlocalresulttype);
				removetypestack(typestack,2);
				break;
			case 0xef://debug
			{
				// skip all debug messages
				code.readbyte();
				code.readu30();
				code.readbyte();
				code.readu30();
				break;
			}
			case 0x42://construct
			{
				int32_t p = code.tellg();
				state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size()+1;
				uint32_t argcount = code.readu30();
				removetypestack(typestack,argcount+1);
#ifdef ENABLE_OPTIMIZATION
				if (state.jumptargets.find(p) == state.jumptargets.end())
				{
					switch (argcount)
					{
						case 0:
						{
							Class_base* restype = state.operandlist.size()> 0 ? state.operandlist.back().objtype : nullptr;
							if (!setupInstructionOneArgument(state,ABC_OP_OPTIMZED_CONSTRUCT_NOARGS,opcode,code,true,false,restype,p,true))
								state.preloadedcode.back().pcode.arg3_uint = argcount;
							typestack.push_back(typestackentry(restype,false));
							break;
						}
						default:
							// TODO handle construct with one or more arguments
							state.preloadedcode.push_back((uint32_t)opcode);
							clearOperands(state,true,&lastlocalresulttype);
							state.preloadedcode.back().pcode.arg3_uint = argcount;
							typestack.push_back(typestackentry(nullptr,false));
							break;
					}
					break;
				}
				else
#endif
				{
					state.preloadedcode.push_back((uint32_t)opcode);
					clearOperands(state,true,&lastlocalresulttype);
					state.preloadedcode.back().pcode.arg3_uint = argcount;
					typestack.push_back(typestackentry(nullptr,false));
				}
				break;
			}
			case 0x47://returnvoid
			{
				state.canlocalinitialize.clear();
				state.preloadedcode.push_back((uint32_t)opcode);
				state.refreshOldNewPosition(code);
				clearOperands(state,true,&lastlocalresulttype);
				skipunreachablecode(state,code);
				break;
			}
			case 0x48://returnvalue
			{
				state.canlocalinitialize.clear();
				if (!mi->returnType)
					function->checkParamTypes();
				int32_t p = code.tellg();
#ifdef ENABLE_OPTIMIZATION
				bool checkresulttype=mi->returnType != Type::anyType && mi->returnType != Type::voidType;
				if (checkresulttype && !coercereturnvalue) // there was no returnvalue opcode yet that needs coercion, so we keep checking
				{
					if (state.jumptargets.find(p) != state.jumptargets.end())
					{
						Class_base* resulttype = state.jumptargeteresulttypes[p];
						checkresulttype = resulttype && resulttype == typestack.back().obj;
						if (!checkresulttype)
						{
							clearOperands(state,true,&lastlocalresulttype);
							coercereturnvalue = true;
						}
					}
					if (checkresulttype)
					{
						if (state.operandlist.size() > 0 && state.operandlist.back().objtype && dynamic_cast<const Class_base*>(mi->returnType))
						{
							if (checkmatchingLastObjtype(state,state.operandlist.back().objtype,(Class_base*)(dynamic_cast<const Class_base*>(mi->returnType)))
									|| state.operandlist.back().objtype->isSubClass(dynamic_cast<Class_base*>(mi->returnType)))
							{
								// return type matches type of last operand, no need to continue checking
//								if ((state.preloadedcode.at(state.preloadedcode.size()-1).pcode.local3.flags & ABC_OP_FORCEINT)
//									&& mi->returnType == Class<Integer>::getRef(state.mi->context->root->getSystemState()).getPtr())

								checkresulttype=false;
							}
						}
					}
					if (checkresulttype)
					{
						coercereturnvalue = true;
					}
				}
				if (state.operandlist.size() > 0 && state.operandlist.back().type == OP_LOCAL && state.operandlist.back().index == (int32_t)mi->body->getReturnValuePos())
				{
					// if localresult of previous action is put into returnvalue, we can skip this opcode
					opcode_skipped=true;
					state.refreshOldNewPosition(code);
				}
				else
#endif
					setupInstructionOneArgumentNoResult(state,ABC_OP_OPTIMZED_RETURNVALUE,opcode,code,p);
				skipunreachablecode(state,code);
				removetypestack(typestack,1);
				break;
			}
			case 0x4a://constructprop
			{
				int32_t p = code.tellg();
				state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size()+1;
				uint32_t t = code.readu30();
				assert_and_throw(t < mi->context->constant_pool.multiname_count);
				uint32_t argcount = code.readu30();
				removetypestack(typestack,argcount+mi->context->constant_pool.multinames[t].runtimeargs+1);
#ifdef ENABLE_OPTIMIZATION
				if (state.jumptargets.find(p) == state.jumptargets.end())
				{
					multiname* name =  mi->context->getMultinameImpl(asAtomHandler::nullAtom,nullptr,t,false);
					switch (mi->context->constant_pool.multinames[t].runtimeargs)
					{
						case 0:
						{
							Class_base* resulttype = nullptr;
							ASObject* constructor = nullptr;
							if (state.operandlist.size() > argcount && state.operandlist[state.operandlist.size()-(argcount+1)].type != OP_LOCAL && state.operandlist[state.operandlist.size()-(argcount+1)].type != OP_CACHED_SLOT)
							{
								// common case: constructprop called to create a class instance
								ASObject* a = asAtomHandler::getObject(*mi->context->getConstantAtom(state.operandlist[state.operandlist.size()-(argcount+1)].type,state.operandlist[state.operandlist.size()-(argcount+1)].index));
								if(a)
								{
									asAtom o;
									a->getVariableByMultiname(o,*name,GET_VARIABLE_OPTION::NONE,wrk);
									if (asAtomHandler::isObject(o))
									{
										constructor = asAtomHandler::getObject(o);
										if (!constructor->getConstant())
										{
											// constructor is a dynamically set function, don't cache
											constructor->decRef();
											constructor=nullptr;
										}
										else if (constructor->is<Class_base>())
											resulttype = constructor->as<Class_base>();
										else if (constructor->is<IFunction>())
											resulttype = dynamic_cast<Class_base*>(constructor->as<IFunction>()->getReturnType());
									}
								}
							}
							switch (argcount)
							{
								case 0:
								{
									if (setupInstructionOneArgument(state,ABC_OP_OPTIMZED_CONSTRUCTPROP_STATICNAME_NOARGS,opcode,code,true,false,resulttype,p,true))
									{
										state.preloadedcode.at(state.preloadedcode.size()-1).pcode.cachedmultiname2 = name;
										state.preloadedcode.push_back(0);
										state.preloadedcode.at(state.preloadedcode.size()-1).pcode.cacheobj2 = constructor;
									}
									else
									{
										state.preloadedcode.back().pcode.arg1_uint=t;
										state.preloadedcode.back().pcode.arg2_uint=argcount;
									}
									typestack.push_back(typestackentry(resulttype,false));
									break;
								}
								default:
									if (state.operandlist.size() > argcount)
									{
										state.preloadedcode.push_back((uint32_t)ABC_OP_OPTIMZED_CONSTRUCTPROP_MULTIARGS);
										state.preloadedcode.at(state.preloadedcode.size()-1).pcode.local2.pos = argcount;
										auto it = state.operandlist.rbegin();
										for(uint32_t i= 0; i < argcount; i++)
										{
											it->removeArg(state);
											state.preloadedcode.push_back(0);
											it->fillCode(state,0,state.preloadedcode.size()-1,false);
											state.preloadedcode.back().pcode.arg2_uint = it->type;
											it++;
										}
										uint32_t oppos = state.preloadedcode.size()-1-argcount;
										it->fillCode(state,0,oppos,true);
										it->removeArg(state);
										oppos = state.preloadedcode.size()-1-argcount;
										state.preloadedcode.push_back(0);
										state.preloadedcode.back().pcode.cachedmultiname2 = name;
										state.preloadedcode.back().pcode.cacheobj1 = constructor;
										removeOperands(state,true,&lastlocalresulttype,argcount+1);
										checkForLocalResult(state,code,2,resulttype,oppos,oppos);
										typestack.push_back(typestackentry(resulttype,false));
										break;
									}
									state.preloadedcode.push_back((uint32_t)opcode);
									clearOperands(state,true,&lastlocalresulttype);
									state.preloadedcode.back().pcode.arg1_uint=t;
									state.preloadedcode.back().pcode.arg2_uint=argcount;
									typestack.push_back(typestackentry(resulttype,false));
									break;
							}
							break;
						}
						default:
							state.preloadedcode.push_back((uint32_t)opcode);
							clearOperands(state,true,&lastlocalresulttype);
							state.preloadedcode.back().pcode.arg1_uint=t;
							state.preloadedcode.back().pcode.arg2_uint=argcount;
							typestack.push_back(typestackentry(nullptr,false));
							break;
					}
				}
				else
#endif
				{
					state.preloadedcode.push_back((uint32_t)opcode);
					clearOperands(state,true,&lastlocalresulttype);
					state.preloadedcode.back().pcode.arg1_uint=t;
					state.preloadedcode.back().pcode.arg2_uint=argcount;
					typestack.push_back(typestackentry(nullptr,false));
				}
				break;
			}
			case 0x45://callsuper
				preload_callprop(state,typestack,code,opcode,&lastlocalresulttype,prevopcode);
				break;
			case 0x4e://callsupervoid
				preload_callprop(state,typestack,code,opcode,&lastlocalresulttype,prevopcode);
				break;
			case 0x46://callproperty
			case 0x4f://callpropvoid
				preload_callprop(state,typestack,code,opcode,&lastlocalresulttype,prevopcode);
				break;
			case 0x64://getglobalscope
			{
				int32_t p = code.tellg();
				state.checkClearOperands(p,&lastlocalresulttype);
				ASObject* resulttype=nullptr;
#ifdef ENABLE_OPTIMIZATION
				if (function->func_scope.getPtr() && (state.scopelist.begin()==state.scopelist.end() || !state.scopelist.back().considerDynamic))
				{
					asAtom ret = function->func_scope->scope.front().object;
					addCachedConstant(state,mi, ret,code);
					resulttype = asAtomHandler::getObject(ret);
				}
				else
#endif
				{
					state.preloadedcode.push_back((uint32_t)opcode);
					clearOperands(state,true,&lastlocalresulttype);
				}
				typestack.push_back(typestackentry(resulttype,false));
				break;
			}
			case 0x66://getproperty
				preload_getproperty(state, typestack, code, opcode, &lastlocalresulttype);
				break;
			case 0x73://convert_i
#ifdef ENABLE_OPTIMIZATION
				if (opcode == code.peekbyte() && state.jumptargets.find(code.tellg()+1) == state.jumptargets.end())
					break;
				if (state.jumptargets.find(code.tellg()) == state.jumptargets.end() && typestack.size() > 0 && typestack.back().obj == Class<Integer>::getRef(mi->context->applicationDomain->getSystemState()).getPtr())
					break;
				if (state.jumptargets.find(code.tellg()) == state.jumptargets.end() && state.operandlist.size() > 0
						&& state.operandlist.back().objtype == Class<Integer>::getRef(mi->context->applicationDomain->getSystemState()).getPtr())
					state.refreshOldNewPosition(code);
				else
#endif
					setupInstructionOneArgument(state,ABC_OP_OPTIMZED_CONVERTI,opcode,code,true,true,Class<Integer>::getRef(function->getSystemState()).getPtr(),code.tellg(),true,false,false,true,ABC_OP_OPTIMZED_CONVERTI_SETSLOT);
				removetypestack(typestack,1);
				typestack.push_back(typestackentry(Class<Integer>::getRef(mi->context->applicationDomain->getSystemState()).getPtr(),false));
				break;
			case 0x74://convert_u
#ifdef ENABLE_OPTIMIZATION
				if (opcode == code.peekbyte() && state.jumptargets.find(code.tellg()+1) == state.jumptargets.end())
					break;
				if (state.jumptargets.find(code.tellg()) == state.jumptargets.end() && typestack.size() > 0 && typestack.back().obj == Class<UInteger>::getRef(mi->context->applicationDomain->getSystemState()).getPtr())
					break;
				if (state.jumptargets.find(code.tellg()) == state.jumptargets.end() && state.operandlist.size() > 0
						&& (state.operandlist.back().objtype == Class<UInteger>::getRef(mi->context->applicationDomain->getSystemState()).getPtr()
							|| state.preloadedcode.back().opcode==0x24))//pushbyte
					state.refreshOldNewPosition(code);
				else
#endif
					setupInstructionOneArgument(state,ABC_OP_OPTIMZED_CONVERTU,opcode,code,true,true,Class<UInteger>::getRef(function->getSystemState()).getPtr(),code.tellg(),true,false,false,true,ABC_OP_OPTIMZED_CONVERTU_SETSLOT);
				removetypestack(typestack,1);
				typestack.push_back(typestackentry(Class<UInteger>::getRef(mi->context->applicationDomain->getSystemState()).getPtr(),false));
				break;
			case 0x75://convert_d
				state.refreshOldNewPosition(code);
#ifdef ENABLE_OPTIMIZATION
				if (opcode == code.peekbyte() && state.jumptargets.find(code.tellg()+1) == state.jumptargets.end())
					break;
				if (state.jumptargets.find(code.tellg()) == state.jumptargets.end() && typestack.size() > 0 &&
						(typestack.back().obj == Class<Number>::getRef(mi->context->applicationDomain->getSystemState()).getPtr() ||
						 typestack.back().obj == Class<Integer>::getRef(mi->context->applicationDomain->getSystemState()).getPtr() ||
						 typestack.back().obj == Class<UInteger>::getRef(mi->context->applicationDomain->getSystemState()).getPtr()))
					break;
				if (state.jumptargets.find(code.tellg()) == state.jumptargets.end() && state.operandlist.size() > 0
						&& (state.operandlist.back().objtype == Class<Number>::getRef(mi->context->applicationDomain->getSystemState()).getPtr()
							|| state.operandlist.back().objtype == Class<Integer>::getRef(mi->context->applicationDomain->getSystemState()).getPtr()
							|| state.operandlist.back().objtype == Class<UInteger>::getRef(mi->context->applicationDomain->getSystemState()).getPtr()))
					state.refreshOldNewPosition(code);
				else
#endif
					setupInstructionOneArgument(state,ABC_OP_OPTIMZED_CONVERTD,opcode,code,true,true,Class<Number>::getRef(function->getSystemState()).getPtr(),code.tellg(),true,false,false,true,ABC_OP_OPTIMZED_CONVERTD_SETSLOT);
				removetypestack(typestack,1);
				typestack.push_back(typestackentry(Class<Number>::getRef(mi->context->applicationDomain->getSystemState()).getPtr(),false));
				break;
			case 0x76://convert_b
#ifdef ENABLE_OPTIMIZATION
				if (opcode == code.peekbyte() && state.jumptargets.find(code.tellg()+1) == state.jumptargets.end())
					break;
				if (state.operandlist.empty() && state.jumptargets.find(code.tellg()) == state.jumptargets.end() && typestack.size() > 0 && typestack.back().obj == Class<Boolean>::getRef(mi->context->applicationDomain->getSystemState()).getPtr())
					break;
				if ((state.jumptargets.find(code.tellg()) == state.jumptargets.end()
					 && state.operandlist.size() > 0 && state.operandlist.back().objtype == Class<Boolean>::getRef(mi->context->applicationDomain->getSystemState()).getPtr()) ||
					((code.peekbyte() == 0x11 || //iftrue
					  code.peekbyte() == 0x12 || //iffalse
					  code.peekbyte() == 0x96    //not
					  )))
					state.refreshOldNewPosition(code);
				else
#endif
					setupInstructionOneArgument(state,ABC_OP_OPTIMZED_CONVERTB,opcode,code,true,true,Class<Boolean>::getRef(function->getSystemState()).getPtr(),code.tellg(),true,false,false,true,ABC_OP_OPTIMZED_CONVERTB_SETSLOT);
				removetypestack(typestack,1);
				typestack.push_back(typestackentry(Class<Boolean>::getRef(mi->context->applicationDomain->getSystemState()).getPtr(),false));
				break;
			case 0x91://increment
#ifdef ENABLE_OPTIMIZATION
				if (typestack.back().obj == Class<Integer>::getRef(function->getSystemState()).getPtr())
				{
					// argument is an int, so we can use the increment_i optimization instead
					setupInstructionIncDecInteger(state,code,typestack,&lastlocalresulttype,dup_indicator,0xc0);//increment_i
					break;
				}
#endif
				setupInstructionOneArgument(state,ABC_OP_OPTIMZED_INCREMENT,opcode,code,true,true,Class<Number>::getRef(function->getSystemState()).getPtr(),code.tellg(),dup_indicator == 0);
				dup_indicator=0;
				removetypestack(typestack,1);
				typestack.push_back(typestackentry(Class<Number>::getRef(mi->context->applicationDomain->getSystemState()).getPtr(),false));
				break;
			case 0x93://decrement
#ifdef ENABLE_OPTIMIZATION
				if (typestack.back().obj == Class<Integer>::getRef(function->getSystemState()).getPtr())
				{
					// argument is an int, so we can use the decrement_i optimization instead
					setupInstructionIncDecInteger(state,code,typestack,&lastlocalresulttype,dup_indicator,0xc1);//decrement_i
					break;
				}
#endif
				setupInstructionOneArgument(state,ABC_OP_OPTIMZED_DECREMENT,opcode,code,true,true,Class<Number>::getRef(function->getSystemState()).getPtr(),code.tellg(),dup_indicator == 0);
				dup_indicator=0;
				removetypestack(typestack,1);
				typestack.push_back(typestackentry(Class<Number>::getRef(mi->context->applicationDomain->getSystemState()).getPtr(),false));
				break;
			case 0x95: //typeof
				setupInstructionOneArgument(state,ABC_OP_OPTIMZED_TYPEOF,opcode,code,true,true,Class<ASString>::getRef(function->getSystemState()).getPtr(),code.tellg(),true);
				removetypestack(typestack,1);
				typestack.push_back(typestackentry(Class<ASString>::getRef(mi->context->applicationDomain->getSystemState()).getPtr(),false));
				break;
			case 0x96: //not
#ifdef ENABLE_OPTIMIZATION
				if (state.jumptargets.find(code.tellg()+1) == state.jumptargets.end() && (
					code.peekbyte() == 0x11 ||  //iftrue
					code.peekbyte() == 0x12 ))  //iffalse
				{
					if (state.jumptargets.find(code.tellg()) != state.jumptargets.end())
						clearOperands(state,true,&lastlocalresulttype);
					// "not" followed by iftrue/iffalse, can be skipped, iftrue/iffalse will be reversed
					reverse_iftruefalse = true;
					break;
				}
#endif
				setupInstructionOneArgument(state,ABC_OP_OPTIMZED_NOT,opcode,code,true,false,Class<Boolean>::getRef(function->getSystemState()).getPtr(),code.tellg(),true);
				removetypestack(typestack,1);
				typestack.push_back(typestackentry(Class<Boolean>::getRef(mi->context->applicationDomain->getSystemState()).getPtr(),false));
				break;
			case 0xa0://add
			{
				removetypestack(typestack,2);
				Class_base* resulttype = nullptr;
#ifdef ENABLE_OPTIMIZATION
				if (state.operandlist.size() > 1)
				{
					auto it = state.operandlist.rbegin();
					Class_base* objtype1 = it->objtype;
					it++;
					Class_base* objtype2 = it->objtype;
					if(objtype1==Class<Integer>::getRef(function->getSystemState()).getPtr() && objtype2==Class<Integer>::getRef(function->getSystemState()).getPtr())
					{
						setupInstructionTwoArguments(state,ABC_OP_OPTIMZED_ADD_I,0xc5 //add_i
													 ,code,false,false,true,code.tellg(),nullptr,ABC_OP_OPTIMZED_ADD_I_SETSLOT);
						typestack.push_back(typestackentry(Class<Integer>::getRef(mi->context->applicationDomain->getSystemState()).getPtr(),false));
						break;
					}
					if((objtype1==Class<Integer>::getRef(function->getSystemState()).getPtr() || objtype1==Class<UInteger>::getRef(function->getSystemState()).getPtr() || objtype1==Class<Number>::getRef(function->getSystemState()).getPtr())
							&& (objtype2==Class<Integer>::getRef(function->getSystemState()).getPtr() || objtype2==Class<UInteger>::getRef(function->getSystemState()).getPtr() || objtype2==Class<Number>::getRef(function->getSystemState()).getPtr()))
						resulttype=Class<Number>::getRef(function->getSystemState()).getPtr();
				}
#endif
				setupInstructionTwoArguments(state,ABC_OP_OPTIMZED_ADD,opcode,code,false,false,true,code.tellg(),resulttype,ABC_OP_OPTIMZED_ADD_SETSLOT);
				setForceInt(state,code,&resulttype);
				typestack.push_back(typestackentry(resulttype,false));
				break;
			}
			case 0xa1://subtract
			{
				removetypestack(typestack,2);
				Class_base* resulttype = Class<Number>::getRef(state.mi->context->applicationDomain->getSystemState()).getPtr();
				setupInstructionTwoArguments(state,ABC_OP_OPTIMZED_SUBTRACT,opcode,code,false,false,true,code.tellg(),resulttype,ABC_OP_OPTIMZED_SUBTRACT_SETSLOT);
				setForceInt(state,code,&resulttype);
				typestack.push_back(typestackentry(resulttype,false));
				break;
			}
			case 0xa2://multiply
			{
				removetypestack(typestack,2);
				Class_base* resulttype = Class<Number>::getRef(state.mi->context->applicationDomain->getSystemState()).getPtr();
				setupInstructionTwoArguments(state,ABC_OP_OPTIMZED_MULTIPLY,opcode,code,false,false,true,code.tellg(),resulttype,ABC_OP_OPTIMZED_MULTIPLY_SETSLOT);
				setForceInt(state,code,&resulttype);
				typestack.push_back(typestackentry(resulttype,false));
				break;
			}
			case 0xa3://divide
			{
				removetypestack(typestack,2);
				Class_base* resulttype = Class<Number>::getRef(state.mi->context->applicationDomain->getSystemState()).getPtr();
				setupInstructionTwoArguments(state,ABC_OP_OPTIMZED_DIVIDE,opcode,code,false,false,true,code.tellg(),resulttype,ABC_OP_OPTIMZED_DIVIDE_SETSLOT);
				setForceInt(state,code,&resulttype);
				typestack.push_back(typestackentry(resulttype,false));
				break;
			}
			case 0xa4://modulo
			{
				removetypestack(typestack,2);
				Class_base* resulttype = Class<Number>::getRef(state.mi->context->applicationDomain->getSystemState()).getPtr();
				setupInstructionTwoArguments(state,ABC_OP_OPTIMZED_MODULO,opcode,code,true,true,true,code.tellg(),resulttype,ABC_OP_OPTIMZED_MODULO_SETSLOT);
				setForceInt(state,code,&resulttype);
				typestack.push_back(typestackentry(resulttype,false));
				break;
			}
			case 0xa5://lshift
				setupInstructionTwoArguments(state,ABC_OP_OPTIMZED_LSHIFT,opcode,code,true,true,true,code.tellg(),nullptr,ABC_OP_OPTIMZED_LSHIFT_SETSLOT);
				removetypestack(typestack,2);
				typestack.push_back(typestackentry(Class<Integer>::getRef(function->getSystemState()).getPtr(),false));
				break;
			case 0xa6://rshift
				setupInstructionTwoArguments(state,ABC_OP_OPTIMZED_RSHIFT,opcode,code,true,true,true,code.tellg(),nullptr,ABC_OP_OPTIMZED_RSHIFT_SETSLOT);
				removetypestack(typestack,2);
				typestack.push_back(typestackentry(Class<Integer>::getRef(function->getSystemState()).getPtr(),false));
				break;
			case 0xa7://urshift
				setupInstructionTwoArguments(state,ABC_OP_OPTIMZED_URSHIFT,opcode,code,true,true,true,code.tellg(),nullptr,ABC_OP_OPTIMZED_URSHIFT_SETSLOT);
				removetypestack(typestack,2);
				typestack.push_back(typestackentry(Class<UInteger>::getRef(function->getSystemState()).getPtr(),false));
				break;
			case 0xa8://bitand
				setupInstructionTwoArguments(state,ABC_OP_OPTIMZED_BITAND,opcode,code,true,true,true,code.tellg(),nullptr,ABC_OP_OPTIMZED_BITAND_SETSLOT);
				removetypestack(typestack,2);
				typestack.push_back(typestackentry(Class<Integer>::getRef(function->getSystemState()).getPtr(),false));
				break;
			case 0xa9://bitor
				setupInstructionTwoArguments(state,ABC_OP_OPTIMZED_BITOR,opcode,code,true,true,true,code.tellg(),nullptr,ABC_OP_OPTIMZED_BITOR_SETSLOT);
				removetypestack(typestack,2);
				typestack.push_back(typestackentry(Class<Integer>::getRef(function->getSystemState()).getPtr(),false));
				break;
			case 0xaa://bitxor
				setupInstructionTwoArguments(state,ABC_OP_OPTIMZED_BITXOR,opcode,code,true,true,true,code.tellg(),nullptr,ABC_OP_OPTIMZED_BITXOR_SETSLOT);
				removetypestack(typestack,2);
				typestack.push_back(typestackentry(Class<Integer>::getRef(function->getSystemState()).getPtr(),false));
				break;
			case 0xab://equals
				setupInstructionComparison(state,ABC_OP_OPTIMZED_EQUALS,opcode,code,ABC_OP_OPTIMZED_IFEQ,ABC_OP_OPTIMZED_IFNE,typestack,&lastlocalresulttype,jumppositions, jumpstartpositions);
				break;
			case 0xad://lessthan
				setupInstructionComparison(state,ABC_OP_OPTIMZED_LESSTHAN,opcode,code,ABC_OP_OPTIMZED_IFLT,ABC_OP_OPTIMZED_IFNLT,typestack,&lastlocalresulttype,jumppositions, jumpstartpositions);
				break;
			case 0xae://lessequals
				setupInstructionComparison(state,ABC_OP_OPTIMZED_LESSEQUALS,opcode,code,ABC_OP_OPTIMZED_IFLE,ABC_OP_OPTIMZED_IFNLE,typestack,&lastlocalresulttype,jumppositions, jumpstartpositions);
				break;
			case 0xaf://greaterthan
				setupInstructionComparison(state,ABC_OP_OPTIMZED_GREATERTHAN,opcode,code,ABC_OP_OPTIMZED_IFGT,ABC_OP_OPTIMZED_IFNGT,typestack,&lastlocalresulttype,jumppositions, jumpstartpositions);
				break;
			case 0xb0://greaterequals
				setupInstructionComparison(state,ABC_OP_OPTIMZED_GREATEREQUALS,opcode,code,ABC_OP_OPTIMZED_IFGE,ABC_OP_OPTIMZED_IFNGE,typestack,&lastlocalresulttype,jumppositions, jumpstartpositions);
				break;
			case 0xc0://increment_i
			case 0xc1://decrement_i
				setupInstructionIncDecInteger(state,code,typestack,&lastlocalresulttype,dup_indicator,opcode);
				break;
			case 0xc5://add_i
				setupInstructionTwoArguments(state,ABC_OP_OPTIMZED_ADD_I,opcode,code,false,false,true,code.tellg(),nullptr,ABC_OP_OPTIMZED_ADD_I_SETSLOT);
				if (state.preloadedcode.back().opcode==ABC_OP_OPTIMZED_ADD_I+5
						&& state.preloadedcode.back().pcode.local3.pos == state.preloadedcode.back().pcode.local_pos1
						&& state.preloadedcode.back().pcode.local3.pos < function->mi->body->getReturnValuePos())
				{
					// add_i is optimized to abc_add_i_local_constant_localresult and localresult and local1 point to the same poition
					// can be replaced by inclocal_i with constant int argument
					// optimizes actionscript code like
					// x += 4;
					state.preloadedcode.back().opcode = ABC_OP_OPTIMZED_INCLOCAL_I;
					removetypestack(typestack,1);
				}
				else
				{
					removetypestack(typestack,2);
					typestack.push_back(typestackentry(Class<Integer>::getRef(mi->context->applicationDomain->getSystemState()).getPtr(),false));
				}
				break;
			case 0x03://throw
				state.canlocalinitialize.clear();
				state.preloadedcode.push_back((uint32_t)opcode);
				state.refreshOldNewPosition(code);
				clearOperands(state,true,&lastlocalresulttype);
				removetypestack(typestack,1);
				skipunreachablecode(state,code);
				break;
			case 0x29://pop
#ifdef ENABLE_OPTIMIZATION
				if (state.operandlist.size()>0 && state.duplocalresult)
				{
					state.operandlist.back().removeArg(state);
					state.operandlist.pop_back();
				}
				else
#endif
				{
					state.preloadedcode.push_back((uint32_t)opcode);
					state.refreshOldNewPosition(code);
					clearOperands(state,true,&lastlocalresulttype);
				}
				removetypestack(typestack,1);
				break;
			case 0x2b://swap
				state.refreshOldNewPosition(code);
#ifdef ENABLE_OPTIMIZATION
				if (state.operandlist.size()>1
						&& state.jumptargets.find(code.tellg()) == state.jumptargets.end())
				{
					std::swap(state.operandlist[state.operandlist.size()-2],state.operandlist[state.operandlist.size()-1]);
					opcode_skipped=true;
				}
				else
#endif
				{
					state.preloadedcode.push_back((uint32_t)opcode);
					clearOperands(state,true,&lastlocalresulttype);
				}
				std::swap(typestack[typestack.size()-2],typestack[typestack.size()-1]);
				break;
			case 0x57://newactivation
			{
				state.preloadedcode.push_back((uint32_t)opcode);
				state.refreshOldNewPosition(code);
				clearOperands(state,true,&lastlocalresulttype);
				activationobject = new_activationObject(wrk);
				activationobject->Variables.Variables.reserve(mi->body->trait_count);
				std::vector<multiname*> additionalslots;
				for(unsigned int i=0;i<mi->body->trait_count;i++)
					mi->context->buildTrait(activationobject,additionalslots,&mi->body->traits[i],false,-1,false);
				activationobject->initAdditionalSlots(additionalslots);
				typestack.push_back(typestackentry(activationobject,false));
				break;
			}
			case 0x87://astypelate
			{
				ASObject* restype = typestack.back().classvar ? typestack.back().obj : nullptr;
				bool handled = false;
#ifdef ENABLE_OPTIMIZATION
				if (state.jumptargets.find(code.tellg()) == state.jumptargets.end()
					&& state.operandlist.size() > 1 && restype == Class<Integer>::getRef(function->getSystemState()).getPtr()
					&& code.peekbyte() == 0x73) //convert_i
				{
					// astypelate with Integer argument following an arithmetic operation
					// and followed by convert_i
					// can be skipped by setting ABC_OP_FORCEINT flag
					switch (state.preloadedcode[state.preloadedcode.size()-2].operator_start)
					{
						case ABC_OP_OPTIMZED_ADD:
						case ABC_OP_OPTIMZED_SUBTRACT:
						case ABC_OP_OPTIMZED_MULTIPLY:
						case ABC_OP_OPTIMZED_DIVIDE:
						case ABC_OP_OPTIMZED_MODULO:
							state.preloadedcode[state.preloadedcode.size()-2].pcode.local3.flags |= ABC_OP_FORCEINT;
							state.preloadedcode.pop_back();
							state.operandlist.pop_back();
							handled = true;
							break;
					}
				}
#endif
				if (!handled)
					setupInstructionTwoArguments(state,ABC_OP_OPTIMZED_ASTYPELATE,opcode,code,false,false,true,code.tellg(),restype && restype->is<Class_base>() ? restype->as<Class_base>() : nullptr,ABC_OP_OPTIMZED_ASTYPELATE_SETSLOT);
				removetypestack(typestack,2);
				typestack.push_back(typestackentry(restype,restype && restype->is<Class_base>()));
				break;
			}
			case 0xc6://subtract_i
				setupInstructionTwoArguments(state,ABC_OP_OPTIMZED_SUBTRACT_I,opcode,code,false,false,true,code.tellg(),nullptr,ABC_OP_OPTIMZED_SUBTRACT_I_SETSLOT);
				if (state.preloadedcode.back().opcode==ABC_OP_OPTIMZED_SUBTRACT_I+5
						&& state.preloadedcode.back().pcode.local3.pos == state.preloadedcode.back().pcode.local_pos1
						&& state.preloadedcode.back().pcode.local3.pos < function->mi->body->getReturnValuePos())
				{
					// add_i is optimized to abc_add_i_local_constant_localresult and localresult and local1 point to the same poition
					// can be replaced by inclocal_i with constant int argument
					// optimizes actionscript code like
					// x -= 4;
					state.preloadedcode.back().opcode = ABC_OP_OPTIMZED_DECLOCAL_I;
					removetypestack(typestack,1);
				}
				else
				{
					removetypestack(typestack,2);
					typestack.push_back(typestackentry(Class<Integer>::getRef(mi->context->applicationDomain->getSystemState()).getPtr(),false));
				}
				break;
			case 0x97://bitnot
				setupInstructionOneArgument(state,ABC_OP_OPTIMZED_BITNOT,opcode,code,true,true,Class<Integer>::getRef(function->getSystemState()).getPtr(),code.tellg(),true,false,false,true,ABC_OP_OPTIMZED_BITNOT_SETSLOT);
				removetypestack(typestack,1);
				typestack.push_back(typestackentry(Class<Integer>::getRef(mi->context->applicationDomain->getSystemState()).getPtr(),false));
				break;
			case 0xc4://negate_i
				state.preloadedcode.push_back((uint32_t)opcode);
				state.refreshOldNewPosition(code);
				clearOperands(state,true,&lastlocalresulttype);
				removetypestack(typestack,1);
				typestack.push_back(typestackentry(Class<Integer>::getRef(mi->context->applicationDomain->getSystemState()).getPtr(),false));
				break;
			case 0xb3://istypelate
				setupInstructionTwoArguments(state,ABC_OP_OPTIMZED_ISTYPELATE,opcode,code,false,false,true,code.tellg(),Class<Boolean>::getRef(mi->context->applicationDomain->getSystemState()).getPtr());
				removetypestack(typestack,2);
				typestack.push_back(typestackentry(Class<Boolean>::getRef(mi->context->applicationDomain->getSystemState()).getPtr(),false));
				break;
			case 0xb1://instanceof
				setupInstructionTwoArguments(state,ABC_OP_OPTIMZED_INSTANCEOF,opcode,code,false,false,true,code.tellg(),Class<Boolean>::getRef(mi->context->applicationDomain->getSystemState()).getPtr());
				removetypestack(typestack,2);
				typestack.push_back(typestackentry(Class<Boolean>::getRef(mi->context->applicationDomain->getSystemState()).getPtr(),false));
				break;
			case 0xac://strictequals
			case 0xb4://in
				state.preloadedcode.push_back((uint32_t)opcode);
				state.refreshOldNewPosition(code);
				clearOperands(state,true,&lastlocalresulttype);
				removetypestack(typestack,2);
				typestack.push_back(typestackentry(Class<Boolean>::getRef(mi->context->applicationDomain->getSystemState()).getPtr(),false));
				break;
			case 0x1e://nextname
				setupInstructionTwoArguments(state,ABC_OP_OPTIMZED_NEXTNAME,opcode,code,false,false,true,code.tellg());
				removetypestack(typestack,2);
				typestack.push_back(typestackentry(nullptr,false));
				break;
			case 0x23://nextvalue
			{
				Class_base* resulttype=nullptr;
#ifdef ENABLE_OPTIMIZATION
				ASObject* obj = typestack.at(typestack.size()-2).obj;
				if (obj && obj->is<Vector>())
					resulttype = obj->as<Vector>()->getType();
#endif
				setupInstructionTwoArguments(state,ABC_OP_OPTIMZED_NEXTVALUE,opcode,code,false,false,true,code.tellg(),resulttype);
				removetypestack(typestack,2);
				typestack.push_back(typestackentry(resulttype,false));
				break;
			}
			case 0x77://convert_o
			case 0x78://checkfilter
				state.preloadedcode.push_back((uint32_t)opcode);
				state.refreshOldNewPosition(code);
				clearOperands(state,true,&lastlocalresulttype);
				removetypestack(typestack,1);
				typestack.push_back(typestackentry(nullptr,false));
				break;
			case 0x70://convert_s
				state.refreshOldNewPosition(code);
#ifdef ENABLE_OPTIMIZATION
				if (opcode == code.peekbyte() && state.jumptargets.find(code.tellg()+1) == state.jumptargets.end())
					break;
				if (state.jumptargets.find(code.tellg()) == state.jumptargets.end() && typestack.size() > 0 &&
						(typestack.back().obj == Class<ASString>::getRef(mi->context->applicationDomain->getSystemState()).getPtr()))
					break;
				if (state.jumptargets.find(code.tellg()) == state.jumptargets.end() && state.operandlist.size() > 0
						&& (state.operandlist.back().objtype == Class<ASString>::getRef(mi->context->applicationDomain->getSystemState()).getPtr()))
					state.refreshOldNewPosition(code);
				else
#endif
					setupInstructionOneArgument(state,ABC_OP_OPTIMZED_CONVERTS,opcode,code,true,true,Class<ASString>::getRef(function->getSystemState()).getPtr(),code.tellg(),true,false,false,true,ABC_OP_OPTIMZED_CONVERTS_SETSLOT);
				removetypestack(typestack,1);
				typestack.push_back(typestackentry(Class<ASString>::getRef(mi->context->applicationDomain->getSystemState()).getPtr(),false));
				break;
			case 0x85://coerce_s
				state.refreshOldNewPosition(code);
#ifdef ENABLE_OPTIMIZATION
				if (opcode == code.peekbyte() && state.jumptargets.find(code.tellg()+1) == state.jumptargets.end())
					break;
				if (state.jumptargets.find(code.tellg()) == state.jumptargets.end() && typestack.size() > 0 &&
						(typestack.back().obj == Class<ASString>::getRef(mi->context->applicationDomain->getSystemState()).getPtr()))
					break;
				if (state.jumptargets.find(code.tellg()) == state.jumptargets.end() && state.operandlist.size() > 0
						&& (state.operandlist.back().objtype == Class<ASString>::getRef(mi->context->applicationDomain->getSystemState()).getPtr()))
					state.refreshOldNewPosition(code);
				else if (state.jumptargets.find(code.tellg()) == state.jumptargets.end() && state.operandlist.size()>0 && state.operandlist.back().type != OP_LOCAL && state.operandlist.back().type != OP_CACHED_SLOT)
				{
					// constant atom can be coerced directly
					asAtom res = *state.mi->context->getConstantAtom(state.operandlist.back().type,state.operandlist.back().index);
					Class<ASString>::getClass(function->getSystemState())->coerce(wrk,res);
					state.operandlist.back().removeArg(state);
					state.operandlist.pop_back();
					addCachedConstant(state,mi, res,code);
				}
				else
#endif
					setupInstructionOneArgument(state,ABC_OP_OPTIMZED_COERCES,opcode,code,true,true,Class<ASString>::getRef(function->getSystemState()).getPtr(),code.tellg(),true,false,false,true,ABC_OP_OPTIMZED_COERCES_SETSLOT);
				removetypestack(typestack,1);
				typestack.push_back(typestackentry(Class<ASString>::getRef(mi->context->applicationDomain->getSystemState()).getPtr(),false));
				break;
			case 0x90://negate
				setupInstructionOneArgument(state,ABC_OP_OPTIMZED_NEGATE,opcode,code,true,true,Class<Number>::getRef(function->getSystemState()).getPtr(),code.tellg(),true,false,false,true,ABC_OP_OPTIMZED_NEGATE_SETSLOT);
				removetypestack(typestack,1);
				typestack.push_back(typestackentry(Class<Number>::getRef(mi->context->applicationDomain->getSystemState()).getPtr(),false));
				break;
			case 0x71://esc_xelem
			case 0x72://esc_xattr
				state.preloadedcode.push_back((uint32_t)opcode);
				state.refreshOldNewPosition(code);
				clearOperands(state,true,&lastlocalresulttype);
				removetypestack(typestack,1);
				typestack.push_back(typestackentry(nullptr,false));
				break;
			case 0xc7://multiply_i
				setupInstructionTwoArguments(state,ABC_OP_OPTIMZED_MULTIPLY_I,opcode,code,false,false,true,code.tellg(),nullptr,ABC_OP_OPTIMZED_MULTIPLY_I_SETSLOT);
				removetypestack(typestack,2);
				typestack.push_back(typestackentry(Class<Integer>::getRef(mi->context->applicationDomain->getSystemState()).getPtr(),false));
				break;
			case 0xd4://setlocal_0
			case 0xd5://setlocal_1
			case 0xd6://setlocal_2
			case 0xd7://setlocal_3
			{
				int32_t p = code.tellg();
#ifdef ENABLE_OPTIMIZATION
				assert_and_throw(uint32_t(opcode-0xd4) < mi->body->local_count);
				setOperandModified(state,OP_LOCAL,opcode-0xd4);
				if (state.setlocal_handled.find(p)!=state.setlocal_handled.end()
						|| checkInitializeLocalToConstant(state,opcode-0xd4))
				{
					state.setlocal_handled.erase(p);
					removetypestack(typestack,1);
					opcode_skipped=true;
					break;
				}
				if (state.operandlist.size() && state.operandlist.back().duparg1)
				{
					// the argument to set is the argument of a dup, so we just modify the localresult of the dup and skip this opcode
					state.preloadedcode.at(state.operandlist.back().preloadedcodepos-1).pcode.local3.pos =(opcode-0xd4);
					state.preloadedcode.at(state.operandlist.back().preloadedcodepos).pcode.arg3_uint =(opcode-0xd4);
					state.operandlist.back().removeArg(state);
					state.operandlist.pop_back();
					opcode_skipped=true;
				}
				else
#endif
				{
					setupInstructionOneArgumentNoResult(state,ABC_OP_OPTIMZED_SETLOCAL,opcode,code,p);
					state.preloadedcode.at(state.preloadedcode.size()-1).pcode.arg3_uint =(opcode-0xd4);
				}
#ifdef ENABLE_OPTIMIZATION
				if (typestack.back().obj && typestack.back().obj->is<Class_base>())
					state.localtypes[opcode-0xd4]=typestack.back().obj->as<Class_base>();
				else
					state.localtypes[opcode-0xd4]=nullptr;
#endif
				removetypestack(typestack,1);
				break;
			}
			case 0x50://sxi1
				setupInstructionOneArgument(state,ABC_OP_OPTIMZED_SXI1,opcode,code,true,true,Class<Integer>::getRef(function->getSystemState()).getPtr(),code.tellg(),true);
				removetypestack(typestack,1);
				typestack.push_back(typestackentry(Class<Integer>::getRef(mi->context->applicationDomain->getSystemState()).getPtr(),false));
				break;
			case 0x51://sxi8
				setupInstructionOneArgument(state,ABC_OP_OPTIMZED_SXI8,opcode,code,true,true,Class<Integer>::getRef(function->getSystemState()).getPtr(),code.tellg(),true);
				removetypestack(typestack,1);
				typestack.push_back(typestackentry(Class<Integer>::getRef(mi->context->applicationDomain->getSystemState()).getPtr(),false));
				break;
			case 0x52://sxi16
				setupInstructionOneArgument(state,ABC_OP_OPTIMZED_SXI16,opcode,code,true,true,Class<Integer>::getRef(function->getSystemState()).getPtr(),code.tellg(),true);
				removetypestack(typestack,1);
				typestack.push_back(typestackentry(Class<Integer>::getRef(mi->context->applicationDomain->getSystemState()).getPtr(),false));
				break;
			case 0xf3://timestamp
				state.preloadedcode.push_back((uint32_t)opcode);
				state.refreshOldNewPosition(code);
				clearOperands(state,true,&lastlocalresulttype);
				break;
			case 0x07://dxnslate
				state.preloadedcode.push_back((uint32_t)opcode);
				state.refreshOldNewPosition(code);
				clearOperands(state,true,&lastlocalresulttype);
				removetypestack(typestack,1);
				break;
			case 0x1f://hasnext
				state.preloadedcode.push_back((uint32_t)opcode);
				state.refreshOldNewPosition(code);
				clearOperands(state,true,&lastlocalresulttype);
				removetypestack(typestack,1);
				break;
			case 0x89://coerce_o
				state.preloadedcode.push_back((uint32_t)opcode);
				state.refreshOldNewPosition(code);
				clearOperands(state,true,&lastlocalresulttype);
				break;
			default:
			{
				char stropcode[10];
				sprintf(stropcode,"%d",opcode);
				char strcodepos[20];
				sprintf(strcodepos,"%d",code.tellg());
				createError<VerifyError>(wrk,kIllegalOpcodeError,function->getSystemState()->getStringFromUniqueId(function->functionname),stropcode,strcodepos);
				return;
			}
		}
	}
	mi->needscoerceresult = coercereturnvalue;
	state.refreshOldNewPosition(code);
	// also add position for end of code, as it seems that jumps to this position are allowed
	state.oldnewpositions[code.tellg()+1] = (int32_t)state.preloadedcode.size();

	// adjust jump positions to new code vector;
	auto itj = jumppositions.begin();
	while (itj != jumppositions.end())
	{
		uint32_t p = jumpstartpositions[itj->first];
		assert (state.oldnewpositions.find(p) != state.oldnewpositions.end());
		if (state.oldnewpositions.find(p+itj->second) == state.oldnewpositions.end() && p+itj->second < code.tellg())
		{
			LOG(LOG_ERROR,"preloadfunction: jump position not found:"<<p<<" "<<itj->second<<" "<<code.tellg());
			createError<VerifyError>(wrk,kInvalidBranchTargetError);
		}
		else
			state.preloadedcode[itj->first].pcode.arg3_int = (state.oldnewpositions[p+itj->second]-(state.oldnewpositions[p]))+1;
		itj++;
	}
	// adjust switch positions to new code vector;
	auto its = switchpositions.begin();
	while (its != switchpositions.end())
	{
		uint32_t p = switchstartpositions[its->first];
		assert (state.oldnewpositions.find(p) != state.oldnewpositions.end());
		assert (state.oldnewpositions.find(p+its->second) != state.oldnewpositions.end());
		state.preloadedcode[its->first].pcode.arg3_int = state.oldnewpositions[p+its->second]-(state.oldnewpositions[p]);
		its++;
	}
	auto itexc = mi->body->exceptions.begin();
	while (itexc != mi->body->exceptions.end())
	{
		uint32_t excpos = itexc->from;
		// the exception region start may be inside unreachable code, so we extend the region to the last reachable code position
		while (state.oldnewpositions.find(excpos) == state.oldnewpositions.end())
		{
			excpos--;
			if (excpos == 0)
				break;
		}
		assert (state.oldnewpositions.find(excpos) != state.oldnewpositions.end());
		itexc->from = state.oldnewpositions[excpos];

		excpos = itexc->to;
		// the exception region end may be inside unreachable code, so we extend the region to the last reachable code position
		while (state.oldnewpositions.find(excpos) == state.oldnewpositions.end())
		{
			excpos--;
			if (excpos == 0)
				break;
		}
		assert (state.oldnewpositions.find(excpos) != state.oldnewpositions.end());
		itexc->to = state.oldnewpositions[excpos];

		assert (state.oldnewpositions.find(itexc->target) != state.oldnewpositions.end());
		itexc->target = state.oldnewpositions[itexc->target];
		itexc++;
	}
	assert(mi->body->preloadedcode.size()==0);
	for (auto itc = state.preloadedcode.begin(); itc != state.preloadedcode.end(); itc++)
	{
		mi->body->preloadedcode.push_back((*itc).pcode);
		if (!mi->body->preloadedcode[mi->body->preloadedcode.size()-1].func)
			mi->body->preloadedcode[mi->body->preloadedcode.size()-1].func = ABCVm::abcfunctions[itc->opcode];
		// adjust cached local slots to localresultcount
		if ((*itc).cachedslot1)
			mi->body->preloadedcode[mi->body->preloadedcode.size()-1].local_pos1+= mi->body->getReturnValuePos()+1+mi->body->localresultcount;
		if ((*itc).cachedslot2)
			mi->body->preloadedcode[mi->body->preloadedcode.size()-1].local_pos2+= mi->body->getReturnValuePos()+1+mi->body->localresultcount;
		if ((*itc).cachedslot3)
			mi->body->preloadedcode[mi->body->preloadedcode.size()-1].local3.pos+= mi->body->getReturnValuePos()+1+mi->body->localresultcount;
	}
	if (activationobject)
		activationobject->decRef();
	for (auto it = catchscopelist.begin(); it != catchscopelist.end(); it++)
		(*it)->decRef();
}

