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
	if (name->name_type != multiname::NAME_OBJECT // avoid calling toString() of multiname object
			&& obj->getClass() && obj->getClass()->findBorrowedSettable(*name))
		throwError<ReferenceError>(kWriteOnlyError, name->normalizedNameUnresolved(obj->getSystemState()), obj->getClassName());
	if (obj->getClass() && obj->getClass()->isSealed)
		throwError<ReferenceError>(kReadSealedError, name->normalizedNameUnresolved(obj->getSystemState()), obj->getClass()->getQualifiedClassName());
	if (name->isEmpty() || !name->hasEmptyNS)
		throwError<ReferenceError>(kReadSealedErrorNs, name->normalizedNameUnresolved(obj->getSystemState()), obj->getClassName());
	if (obj->is<Undefined>())
		throwError<TypeError>(kConvertUndefinedToObjectError);
	prop = asAtomHandler::undefinedAtom;
}

#ifndef NDEBUG
std::map<uint32_t,uint32_t> opcodecounter;
void ABCVm::dumpOpcodeCounters(uint32_t threshhold)
{
	auto it = opcodecounter.begin();
	while (it != opcodecounter.end())
	{
		if (it->second > threshhold)
			LOG(LOG_INFO,"opcode counter:"<<hex<<it->first<<":"<<dec<<it->second);
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

	//Each case block builds the correct parameters for the interpreter function and call it
	while(!context->returning)
	{
#ifdef PROFILING_SUPPORT
		uint32_t instructionPointer=context->exec_pos- &context->mi->body->preloadedcode.front();
#endif
		//LOG(LOG_INFO,"opcode:"<<(context->stackp-context->stack)<<" "<< hex<<(int)((context->exec_pos->data)&0x3ff));

#ifndef NDEBUG
		uint32_t c = opcodecounter[(context->exec_pos->data)&0x3ff];
		opcodecounter[(context->exec_pos->data)&0x3ff] = c+1;
#endif
		// context->exec_pos points to the current instruction, every abc_function has to make sure
		// it points to the next valid instruction after execution
		abcfunctions[(context->exec_pos->data)&0x3ff](context);

		PROF_ACCOUNT_TIME(context->mi->profTime[instructionPointer],profilingCheckpoint(startTime));
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

	// instructions for optimized opcodes (indicated by 0x03xx)
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
	abc_iffalse_dup_constant,// 0x1de ABC_OP_OPTIMZED_IFFALSE_FALSE
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
	abc_callFunctionMultiArgsVoid, // 0x1fc ABC_OP_OPTIMZED_CALLFUNCTION_MULTIARGS_VOID
	abc_callFunctionMultiArgs, // 0x1fd ABC_OP_OPTIMZED_CALLFUNCTION_MULTIARGS
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
	abc_lf64_constant,// 0x229 ABC_OP_OPTIMZED_LF64
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
	abc_invalidinstruction,
	abc_invalidinstruction,
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

	abc_invalidinstruction, // 0x290
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
	abc_invalidinstruction,

	abc_invalidinstruction, // 0x2a0
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
	abc_invalidinstruction,

	abc_invalidinstruction, // 0x2b0
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
	abc_invalidinstruction,

	abc_invalidinstruction, // 0x2c0
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
	abc_invalidinstruction,

	abc_invalidinstruction, // 0x2d0
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
	abc_invalidinstruction,

	abc_invalidinstruction, // 0x2e0
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
	abc_invalidinstruction,

	abc_invalidinstruction, // 0x2f0
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
	abc_invalidinstruction,

	abc_invalidinstruction, // 0x300
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
	context->locals[t]=asAtomHandler::undefinedAtom;
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
	bool cond=!(asAtomHandler::isLess(*v2,context->mi->context->root->getSystemState(),*v1) == TTRUE);
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
	bool cond=!(asAtomHandler::isLess(*v1,context->mi->context->root->getSystemState(),*v2) == TFALSE);
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
	bool cond=!(asAtomHandler::isLess(*v1,context->mi->context->root->getSystemState(),*v2) == TTRUE);
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
	bool cond=!(asAtomHandler::isLess(*v2,context->mi->context->root->getSystemState(),*v1) == TFALSE);
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
	bool cond=asAtomHandler::Boolean_concrete(*v1);
	LOG_CALL(_("ifTrue (") << ((cond)?_("taken)"):_("not taken)")));
	ASATOM_DECREF_POINTER(v1);
	if(cond)
		context->exec_pos += t+1;
	else
		++(context->exec_pos);
}
void ABCVm::abc_iftrue_constant(call_context* context)
{
	int32_t t = (*context->exec_pos).jumpdata.jump;
	bool cond=asAtomHandler::Boolean_concrete(*context->exec_pos->arg1_constant);
	LOG_CALL(_("ifTrue_c (") << ((cond)?_("taken)"):_("not taken)")));
	if(cond)
		context->exec_pos += t+1;
	else
		++(context->exec_pos);
}
void ABCVm::abc_iftrue_local(call_context* context)
{
	int32_t t = (*context->exec_pos).jumpdata.jump;
	bool cond=asAtomHandler::Boolean_concrete(context->locals[context->exec_pos->local_pos1]);
	LOG_CALL(_("ifTrue_l (") << ((cond)?_("taken)"):_("not taken)")));
	if(cond)
		context->exec_pos += t+1;
	else
		++(context->exec_pos);
}
void ABCVm::abc_iftrue_dup_constant(call_context* context)
{
	int32_t t = (*context->exec_pos).jumpdata.jump;
	preloadedcodedata* instrptr = context->exec_pos;
	bool cond=asAtomHandler::Boolean_concrete(*instrptr->arg1_constant);
	LOG_CALL(_("ifTrue_dup_c (") << ((cond)?_("taken)"):_("not taken)")));
	if(cond)
		context->exec_pos += t+1;
	else
		++(context->exec_pos);
	RUNTIME_STACK_PUSH(context,*instrptr->arg1_constant);
}
void ABCVm::abc_iftrue_dup_local(call_context* context)
{
	int32_t t = (*context->exec_pos).jumpdata.jump;
	preloadedcodedata* instrptr = context->exec_pos;
	bool cond=asAtomHandler::Boolean_concrete(context->locals[instrptr->local_pos1]);
	LOG_CALL(_("ifTrue_dup_l (") << ((cond)?_("taken)"):_("not taken)")));
	if(cond)
		context->exec_pos += t+1;
	else
		++(context->exec_pos);
	ASATOM_INCREF(context->locals[instrptr->local_pos1]);
	RUNTIME_STACK_PUSH(context,context->locals[instrptr->local_pos1]);
}
void ABCVm::abc_iffalse(call_context* context)
{
	//iffalse
	int32_t t = (*context->exec_pos).jumpdata.jump;

	RUNTIME_STACK_POP_CREATE(context,v1);
	bool cond=!asAtomHandler::Boolean_concrete(*v1);
	LOG_CALL(_("ifFalse (") << ((cond)?_("taken"):_("not taken")) << ')');
	ASATOM_DECREF_POINTER(v1);
	if(cond)
		context->exec_pos += t+1;
	else
		++(context->exec_pos);
}
void ABCVm::abc_iffalse_constant(call_context* context)
{
	int32_t t = (*context->exec_pos).jumpdata.jump;
	bool cond=!asAtomHandler::Boolean_concrete(*context->exec_pos->arg1_constant);
	LOG_CALL(_("ifFalse_c (") << ((cond)?_("taken)"):_("not taken)")));
	if(cond)
		context->exec_pos += t+1;
	else
		++(context->exec_pos);
}
void ABCVm::abc_iffalse_local(call_context* context)
{
	int32_t t = (*context->exec_pos).jumpdata.jump;
	bool cond=!asAtomHandler::Boolean_concrete(context->locals[context->exec_pos->local_pos1]);
	LOG_CALL(_("ifFalse_l (") << ((cond)?_("taken)"):_("not taken)")));
	if(cond)
		context->exec_pos += t+1;
	else
		++(context->exec_pos);
}
void ABCVm::abc_iffalse_dup_constant(call_context* context)
{
	int32_t t = (*context->exec_pos).jumpdata.jump;
	preloadedcodedata* instrptr = context->exec_pos;
	bool cond=!asAtomHandler::Boolean_concrete(*instrptr->arg1_constant);
	LOG_CALL(_("ifFalse_dup_c (") << ((cond)?_("taken)"):_("not taken)")));
	if(cond)
		context->exec_pos += t+1;
	else
		++(context->exec_pos);
	RUNTIME_STACK_PUSH(context,*instrptr->arg1_constant);
}
void ABCVm::abc_iffalse_dup_local(call_context* context)
{
	int32_t t = (*context->exec_pos).jumpdata.jump;
	preloadedcodedata* instrptr = context->exec_pos;
	bool cond=!asAtomHandler::Boolean_concrete(context->locals[instrptr->local_pos1]);
	LOG_CALL(_("ifFalse_dup_l (") << ((cond)?_("taken)"):_("not taken)")));
	if(cond)
		context->exec_pos += t+1;
	else
		++(context->exec_pos);
	ASATOM_INCREF(context->locals[instrptr->local_pos1]);
	RUNTIME_STACK_PUSH(context,context->locals[instrptr->local_pos1]);
}
void ABCVm::abc_ifeq(call_context* context)
{
	//ifeq
	int32_t t = (*context->exec_pos).jumpdata.jump;

	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POP_CREATE(context,v2);
	bool cond=(asAtomHandler::isEqual(*v1,context->mi->context->root->getSystemState(),*v2));
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
	bool cond=asAtomHandler::isEqual(*context->exec_pos->arg1_constant,context->mi->context->root->getSystemState(),*context->exec_pos->arg2_constant);
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
	bool cond=asAtomHandler::isEqual(*context->exec_pos->arg2_constant,context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos1]);
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
	bool cond=asAtomHandler::isEqual(*context->exec_pos->arg1_constant,context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos2]);
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
	bool cond=asAtomHandler::isEqual(context->locals[context->exec_pos->local_pos1],context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos2]);
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
	bool cond=!(asAtomHandler::isEqual(*v1,context->mi->context->root->getSystemState(),*v2));
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
	bool cond=!asAtomHandler::isEqual(*context->exec_pos->arg1_constant,context->mi->context->root->getSystemState(),*context->exec_pos->arg2_constant) == TTRUE;
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
	bool cond=!asAtomHandler::isEqual(*context->exec_pos->arg2_constant,context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos1]);
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
	bool cond=!asAtomHandler::isEqual(*context->exec_pos->arg1_constant,context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos2]);
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
	bool cond=!asAtomHandler::isEqual(context->locals[context->exec_pos->local_pos1],context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos2]);
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
	bool cond=asAtomHandler::isLess(*v2,context->mi->context->root->getSystemState(),*v1) == TTRUE;
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
	bool cond=asAtomHandler::isLess(*context->exec_pos->arg1_constant,context->mi->context->root->getSystemState(),*context->exec_pos->arg2_constant) == TTRUE;
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
	bool cond=asAtomHandler::isLess(context->locals[context->exec_pos->local_pos1],context->mi->context->root->getSystemState(),*context->exec_pos->arg2_constant) == TTRUE;
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
	bool cond=asAtomHandler::isLess(*context->exec_pos->arg1_constant,context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos2]) == TTRUE;
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
	bool cond=asAtomHandler::isLess(context->locals[context->exec_pos->local_pos1],context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos2]) == TTRUE;
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
	bool cond=asAtomHandler::isLess(*v1,context->mi->context->root->getSystemState(),*v2) == TFALSE;
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
	bool cond=asAtomHandler::isLess(*context->exec_pos->arg2_constant,context->mi->context->root->getSystemState(),*context->exec_pos->arg1_constant) == TFALSE;
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
	bool cond=asAtomHandler::isLess(*context->exec_pos->arg2_constant,context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos1]) == TFALSE;
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
	bool cond=asAtomHandler::isLess(context->locals[context->exec_pos->local_pos2],context->mi->context->root->getSystemState(),*context->exec_pos->arg1_constant) == TFALSE;
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
	bool cond=asAtomHandler::isLess(context->locals[context->exec_pos->local_pos2],context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos1]) == TFALSE;
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
	bool cond=asAtomHandler::isLess(*v1,context->mi->context->root->getSystemState(),*v2) == TTRUE;
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
	bool cond=asAtomHandler::isLess(*context->exec_pos->arg2_constant,context->mi->context->root->getSystemState(),*context->exec_pos->arg1_constant) == TTRUE;
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
	bool cond=asAtomHandler::isLess(*context->exec_pos->arg2_constant,context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos1]) == TTRUE;
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
	bool cond=asAtomHandler::isLess(context->locals[context->exec_pos->local_pos2],context->mi->context->root->getSystemState(),*context->exec_pos->arg1_constant) == TTRUE;
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
	bool cond=asAtomHandler::isLess(context->locals[context->exec_pos->local_pos2],context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos1]) == TTRUE;
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
	bool cond=asAtomHandler::isLess(*v2,context->mi->context->root->getSystemState(),*v1) == TFALSE;
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
	bool cond=asAtomHandler::isLess(*context->exec_pos->arg1_constant,context->mi->context->root->getSystemState(),*context->exec_pos->arg2_constant) == TFALSE;
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
	bool cond=asAtomHandler::isLess(context->locals[context->exec_pos->local_pos1],context->mi->context->root->getSystemState(),*context->exec_pos->arg2_constant) == TFALSE;
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
	bool cond=asAtomHandler::isLess(*context->exec_pos->arg1_constant,context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos2]) == TFALSE;
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
	bool cond=asAtomHandler::isLess(context->locals[context->exec_pos->local_pos1],context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos2]) == TFALSE;
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

	bool cond=asAtomHandler::isEqualStrict(*v1,context->mi->context->root->getSystemState(),*v2);
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
	bool cond=asAtomHandler::isEqualStrict(*context->exec_pos->arg1_constant,context->mi->context->root->getSystemState(),*context->exec_pos->arg2_constant);
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
	bool cond=asAtomHandler::isEqualStrict(context->locals[context->exec_pos->local_pos1],context->mi->context->root->getSystemState(),*context->exec_pos->arg2_constant);
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
	bool cond=asAtomHandler::isEqualStrict(*context->exec_pos->arg1_constant,context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos2]);
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
	LOG_CALL(_("ifstricteq_ll (") << asAtomHandler::toDebugString(context->locals[context->exec_pos->local_pos1])<<" "<<asAtomHandler::toDebugString(context->locals[context->exec_pos->local_pos2]));
	bool cond=asAtomHandler::isEqualStrict(context->locals[context->exec_pos->local_pos1],context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos2]);
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

	bool cond=!asAtomHandler::isEqualStrict(*v1,context->mi->context->root->getSystemState(),*v2);
	LOG_CALL(_("ifStrictNE ")<<cond <<" "<<asAtomHandler::toDebugString(*v1)<<" "<<asAtomHandler::toDebugString(*v2));
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
	bool cond=!asAtomHandler::isEqualStrict(*context->exec_pos->arg1_constant,context->mi->context->root->getSystemState(),*context->exec_pos->arg2_constant);
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
	bool cond=!asAtomHandler::isEqualStrict(context->locals[context->exec_pos->local_pos1],context->mi->context->root->getSystemState(),*context->exec_pos->arg2_constant);
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
	bool cond=!asAtomHandler::isEqualStrict(*context->exec_pos->arg1_constant,context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos2]);
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
	bool cond=!asAtomHandler::isEqualStrict(context->locals[context->exec_pos->local_pos1],context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos2]);
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

	RUNTIME_STACK_POP_CREATE(context,index_obj);
	assert_and_throw(asAtomHandler::isNumeric(*index_obj));
	unsigned int index=asAtomHandler::toUInt(*index_obj);
	ASATOM_DECREF_POINTER(index_obj);

	preloadedcodedata* dest=defaultdest;
	if(index<=count)
		dest=here+(context->exec_pos+index+1)->idata;

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
	if(!asAtomHandler::isUInteger(*v1))
		throw UnsupportedException("Type mismatch in nextName");

	asAtom ret=asAtomHandler::invalidAtom;
	asAtomHandler::toObject(*pval,context->mi->context->root->getSystemState())->nextName(ret,asAtomHandler::toUInt(*v1));
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
	LOG_CALL("hasNext " << asAtomHandler::toDebugString(*v1) << ' ' << asAtomHandler::toDebugString(*pval));

	uint32_t curIndex=asAtomHandler::toUInt(*pval);

	uint32_t newIndex=asAtomHandler::toObject(*v1,context->mi->context->root->getSystemState())->nextNameIndex(curIndex);
	ASATOM_DECREF_POINTER(pval);
	ASATOM_DECREF_POINTER(v1);
	asAtomHandler::setInt(*pval,context->mi->context->root->getSystemState(),newIndex);
	++(context->exec_pos);
}
void ABCVm::abc_pushnull(call_context* context)
{
	//pushnull
	LOG_CALL("pushnull");
	RUNTIME_STACK_PUSH(context,asAtomHandler::nullAtom);
	++(context->exec_pos);
}
void ABCVm::abc_pushundefined(call_context* context)
{
	LOG_CALL("pushundefined");
	RUNTIME_STACK_PUSH(context,asAtomHandler::undefinedAtom);
	++(context->exec_pos);
}
void ABCVm::abc_nextvalue(call_context* context)
{
	//nextvalue
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	LOG_CALL("nextvalue:"<<asAtomHandler::toDebugString(*v1)<<" "<< asAtomHandler::toDebugString(*pval));
	if(!asAtomHandler::isUInteger(*v1))
		throw UnsupportedException("Type mismatch in nextValue");

	asAtom ret=asAtomHandler::invalidAtom;
	asAtomHandler::toObject(*pval,context->mi->context->root->getSystemState())->nextValue(ret,asAtomHandler::toUInt(*v1));
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
	RUNTIME_STACK_PUSH(context,asAtomHandler::fromInt(t));
	++(context->exec_pos);
}
void ABCVm::abc_pushshort(call_context* context)
{
	//pushshort
	// specs say pushshort is a u30, but it's really a u32
	// see https://bugs.adobe.com/jira/browse/ASC-4181
	int32_t t = (++(context->exec_pos))->idata;
	LOG_CALL("pushshort "<<t);

	RUNTIME_STACK_PUSH(context,asAtomHandler::fromInt(t));
	++(context->exec_pos);
}
void ABCVm::abc_pushtrue(call_context* context)
{
	//pushtrue
	LOG_CALL("pushtrue");
	RUNTIME_STACK_PUSH(context,asAtomHandler::trueAtom);
	++(context->exec_pos);
}
void ABCVm::abc_pushfalse(call_context* context)
{
	//pushfalse
	LOG_CALL("pushfalse");
	RUNTIME_STACK_PUSH(context,asAtomHandler::falseAtom);
	++(context->exec_pos);
}
void ABCVm::abc_pushnan(call_context* context)
{
	//pushnan
	LOG_CALL("pushNaN");
	RUNTIME_STACK_PUSH(context,context->mi->context->root->getSystemState()->nanAtom);
	++(context->exec_pos);
}
void ABCVm::abc_pushcachedconstant(call_context* context)
{
	int32_t t = (++(context->exec_pos))->idata;
	assert(t <= (int)context->mi->context->atomsCachedMaxID);
	asAtom a = context->mi->context->constantAtoms_cached[t];
	LOG_CALL("pushcachedconstant "<<t<<" "<<asAtomHandler::toDebugString(a));
	ASATOM_INCREF(a);
	RUNTIME_STACK_PUSH(context,a);
	++(context->exec_pos);
}
void ABCVm::abc_getlexfromslot(call_context* context)
{
	uint32_t t = context->exec_pos->data>>OPCODE_SIZE;
	
	ASObject* s = asAtomHandler::toObject(*context->locals,context->mi->context->root->getSystemState());
	asAtom a = s->getSlot(t);
	LOG_CALL("getlexfromslot "<<s->toDebugString()<<" "<<t);
	ASATOM_INCREF(a);
	RUNTIME_STACK_PUSH(context,a);
	++(context->exec_pos);
}
void ABCVm::abc_getlexfromslot_localresult(call_context* context)
{
	uint32_t t = context->exec_pos->data>>OPCODE_SIZE;
	
	ASObject* s = asAtomHandler::toObject(*context->locals,context->mi->context->root->getSystemState());
	asAtom a = s->getSlot(t);
	LOG_CALL("getlexfromslot_l "<<s->toDebugString()<<" "<<t);
	ASATOM_INCREF(a);
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	asAtomHandler::set(context->locals[context->exec_pos->local_pos3-1],a);
	++(context->exec_pos);
}

void ABCVm::abc_pop(call_context* context)
{
	//pop
	RUNTIME_STACK_POP_CREATE(context,o);
	LOG_CALL("pop "<<asAtomHandler::toDebugString(*o));
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
	asAtom v1=asAtomHandler::invalidAtom;
	RUNTIME_STACK_POP(context,v1);
	asAtom v2=asAtomHandler::invalidAtom;
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
	RUNTIME_STACK_PUSH(context,asAtomHandler::fromStringID(context->mi->context->getString(t)));
	++(context->exec_pos);
}
void ABCVm::abc_pushint(call_context* context)
{
	//pushint
	uint32_t t = (++(context->exec_pos))->data;
	s32 val=context->mi->context->constant_pool.integer[t];
	LOG_CALL( "pushInt " << val);

	RUNTIME_STACK_PUSH(context,asAtomHandler::fromInt(val));
	++(context->exec_pos);
}
void ABCVm::abc_pushuint(call_context* context)
{
	//pushuint
	uint32_t t = (++(context->exec_pos))->data;
	u32 val=context->mi->context->constant_pool.uinteger[t];
	LOG_CALL( "pushUInt " << val);

	RUNTIME_STACK_PUSH(context,asAtomHandler::fromUInt(val));
	++(context->exec_pos);
}
void ABCVm::abc_pushdouble(call_context* context)
{
	//pushdouble
	uint32_t t = (++(context->exec_pos))->data;
	asAtom* a = context->mi->context->getConstantAtom(OPERANDTYPES::OP_DOUBLE,t);
	LOG_CALL( "pushDouble " << asAtomHandler::toDebugString(*a));

	RUNTIME_STACK_PUSH(context,*a);
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
	LOG_CALL( _("pushScope_c ") << asAtomHandler::toDebugString(*t) );
	assert_and_throw(context->curr_scope_stack < context->mi->body->max_scope_depth);
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
	LOG_CALL( _("pushScope_l ") << asAtomHandler::toDebugString(*t) );
	assert_and_throw(context->curr_scope_stack < context->mi->body->max_scope_depth);
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
	RUNTIME_STACK_PUSH(context,asAtomHandler::fromObjectNoPrimitive(pushNamespace(context, t) ));
	++(context->exec_pos);
}
void ABCVm::abc_hasnext2(call_context* context)
{
	//hasnext2
	uint32_t t = (++(context->exec_pos))->data;
	uint32_t t2 = (++(context->exec_pos))->data;

	bool ret=hasNext2(context,t,t2);
	RUNTIME_STACK_PUSH(context,asAtomHandler::fromBool(ret));
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
void ABCVm::abc_li8_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "li8_c");
	asAtom ret=asAtomHandler::invalidAtom;
	loadIntN<uint8_t>(context,ret,*instrptr->arg1_constant);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_li8_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "li8_l");
	asAtom ret=asAtomHandler::invalidAtom;
	loadIntN<uint8_t>(context,ret,context->locals[instrptr->local_pos1]);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_li8_constant_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "li8_cl");
	asAtom ret=asAtomHandler::invalidAtom;
	loadIntN<uint8_t>(context,ret,*instrptr->arg1_constant);
	ASObject* o = asAtomHandler::getObject(context->locals[instrptr->local_pos3-1]);
	asAtomHandler::set(context->locals[instrptr->local_pos3-1],ret);
	if (o)
		o->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_li8_local_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "li8_ll");
	asAtom ret=asAtomHandler::invalidAtom;
	loadIntN<uint8_t>(context,ret,context->locals[instrptr->local_pos1]);
	ASObject* o = asAtomHandler::getObject(context->locals[instrptr->local_pos3-1]);
	asAtomHandler::set(context->locals[instrptr->local_pos3-1],ret);
	if (o)
		o->decRef();
	++(context->exec_pos);
}

void ABCVm::abc_li16(call_context* context)
{
	//li16
	LOG_CALL( "li16");
	loadIntN<uint16_t>(context);
	++(context->exec_pos);
}
void ABCVm::abc_li16_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "li16_c");
	asAtom ret=asAtomHandler::invalidAtom;
	loadIntN<uint16_t>(context,ret,*instrptr->arg1_constant);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_li16_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "li16_l");
	asAtom ret=asAtomHandler::invalidAtom;
	loadIntN<uint16_t>(context,ret,context->locals[instrptr->local_pos1]);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_li16_constant_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "li16_cl");
	asAtom ret=asAtomHandler::invalidAtom;
	loadIntN<uint16_t>(context,ret,*instrptr->arg1_constant);
	ASObject* o = asAtomHandler::getObject(context->locals[instrptr->local_pos3-1]);
	asAtomHandler::set(context->locals[instrptr->local_pos3-1],ret);
	if (o)
		o->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_li16_local_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "li16_ll");
	asAtom ret=asAtomHandler::invalidAtom;
	loadIntN<uint16_t>(context,ret,context->locals[instrptr->local_pos1]);
	ASObject* o = asAtomHandler::getObject(context->locals[instrptr->local_pos3-1]);
	asAtomHandler::set(context->locals[instrptr->local_pos3-1],ret);
	if (o)
		o->decRef();
	++(context->exec_pos);
}

void ABCVm::abc_li32(call_context* context)
{
	//li32
	LOG_CALL( "li32");
	loadIntN<int32_t>(context);
	++(context->exec_pos);
}
void ABCVm::abc_li32_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "li32_c");
	asAtom ret=asAtomHandler::invalidAtom;
	loadIntN<int32_t>(context,ret,*instrptr->arg1_constant);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_li32_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "li32_l");
	asAtom ret=asAtomHandler::invalidAtom;
	loadIntN<int32_t>(context,ret,context->locals[instrptr->local_pos1]);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_li32_constant_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "li32_cl");
	asAtom ret=asAtomHandler::invalidAtom;
	loadIntN<int32_t>(context,ret,*instrptr->arg1_constant);
	ASObject* o = asAtomHandler::getObject(context->locals[instrptr->local_pos3-1]);
	asAtomHandler::set(context->locals[instrptr->local_pos3-1],ret);
	if (o)
		o->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_li32_local_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "li32_ll");
	asAtom ret=asAtomHandler::invalidAtom;
	loadIntN<int32_t>(context,ret,context->locals[instrptr->local_pos1]);
	ASObject* o = asAtomHandler::getObject(context->locals[instrptr->local_pos3-1]);
	asAtomHandler::set(context->locals[instrptr->local_pos3-1],ret);
	if (o)
		o->decRef();
	++(context->exec_pos);
}

void ABCVm::abc_lf32(call_context* context)
{
	//lf32
	LOG_CALL( "lf32");
	loadFloat(context);
	++(context->exec_pos);
}
void ABCVm::abc_lf32_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "lf32_c");
	asAtom ret=asAtomHandler::invalidAtom;
	loadFloat(context,ret,*instrptr->arg1_constant);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_lf32_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "lf32_l");
	asAtom ret=asAtomHandler::invalidAtom;
	loadFloat(context,ret,context->locals[instrptr->local_pos1]);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_lf32_constant_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "lf32_cl");
	asAtom ret=asAtomHandler::invalidAtom;
	loadFloat(context,ret,*instrptr->arg1_constant);
	ASObject* o = asAtomHandler::getObject(context->locals[instrptr->local_pos3-1]);
	asAtomHandler::set(context->locals[instrptr->local_pos3-1],ret);
	if (o)
		o->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_lf32_local_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "lf32_ll");
	asAtom ret=asAtomHandler::invalidAtom;
	loadFloat(context,ret,context->locals[instrptr->local_pos1]);
	ASObject* o = asAtomHandler::getObject(context->locals[instrptr->local_pos3-1]);
	asAtomHandler::set(context->locals[instrptr->local_pos3-1],ret);
	if (o)
		o->decRef();
	++(context->exec_pos);
}

void ABCVm::abc_lf64(call_context* context)
{
	//lf64
	LOG_CALL( "lf64");
	loadDouble(context);
	++(context->exec_pos);
}
void ABCVm::abc_lf64_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "lf64_c");
	asAtom ret=asAtomHandler::invalidAtom;
	loadDouble(context,ret,*instrptr->arg1_constant);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_lf64_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "lf64_l");
	asAtom ret=asAtomHandler::invalidAtom;
	loadDouble(context,ret,context->locals[instrptr->local_pos1]);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_lf64_constant_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "lf64_cl");
	asAtom ret=asAtomHandler::invalidAtom;
	loadDouble(context,ret,*instrptr->arg1_constant);
	ASObject* o = asAtomHandler::getObject(context->locals[instrptr->local_pos3-1]);
	asAtomHandler::set(context->locals[instrptr->local_pos3-1],ret);
	if (o)
		o->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_lf64_local_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	LOG_CALL( "lf64_ll");
	asAtom ret=asAtomHandler::invalidAtom;
	loadDouble(context,ret,context->locals[instrptr->local_pos1]);
	ASObject* o = asAtomHandler::getObject(context->locals[instrptr->local_pos3-1]);
	asAtomHandler::set(context->locals[instrptr->local_pos3-1],ret);
	if (o)
		o->decRef();
	++(context->exec_pos);
}

void ABCVm::abc_si8(call_context* context)
{
	//si8
	LOG_CALL( "si8");
	storeIntN<uint8_t>(context);
	++(context->exec_pos);
}
void ABCVm::abc_si8_constant_constant(call_context* context)
{
	LOG_CALL( "si8_cc");
	preloadedcodedata* instrptr = context->exec_pos;
	storeIntN<uint8_t>(context,*instrptr->arg2_constant,*instrptr->arg1_constant);
	++(context->exec_pos);
}
void ABCVm::abc_si8_local_constant(call_context* context)
{
	LOG_CALL( "si8_lc");
	preloadedcodedata* instrptr = context->exec_pos;
	storeIntN<uint8_t>(context,*instrptr->arg2_constant,context->locals[instrptr->local_pos1]);
	++(context->exec_pos);
}
void ABCVm::abc_si8_constant_local(call_context* context)
{
	LOG_CALL( "si8_cl");
	preloadedcodedata* instrptr = context->exec_pos;
	storeIntN<uint8_t>(context,context->locals[instrptr->local_pos2],*instrptr->arg1_constant);
	++(context->exec_pos);
}
void ABCVm::abc_si8_local_local(call_context* context)
{
	LOG_CALL( "si8_ll");
	preloadedcodedata* instrptr = context->exec_pos;
	storeIntN<uint8_t>(context,context->locals[instrptr->local_pos2],context->locals[instrptr->local_pos1]);
	++(context->exec_pos);
}

void ABCVm::abc_si16(call_context* context)
{
	//si16
	LOG_CALL( "si16");
	storeIntN<uint16_t>(context);
	++(context->exec_pos);
}
void ABCVm::abc_si16_constant_constant(call_context* context)
{
	LOG_CALL( "si16_cc");
	preloadedcodedata* instrptr = context->exec_pos;
	storeIntN<uint16_t>(context,*instrptr->arg2_constant,*instrptr->arg1_constant);
	++(context->exec_pos);
}
void ABCVm::abc_si16_local_constant(call_context* context)
{
	LOG_CALL( "si16_lc");
	preloadedcodedata* instrptr = context->exec_pos;
	storeIntN<uint16_t>(context,*instrptr->arg2_constant,context->locals[instrptr->local_pos1]);
	++(context->exec_pos);
}
void ABCVm::abc_si16_constant_local(call_context* context)
{
	LOG_CALL( "si16_cl");
	preloadedcodedata* instrptr = context->exec_pos;
	storeIntN<uint16_t>(context,context->locals[instrptr->local_pos2],*instrptr->arg1_constant);
	++(context->exec_pos);
}
void ABCVm::abc_si16_local_local(call_context* context)
{
	LOG_CALL( "si16_ll");
	preloadedcodedata* instrptr = context->exec_pos;
	storeIntN<uint16_t>(context,context->locals[instrptr->local_pos2],context->locals[instrptr->local_pos1]);
	++(context->exec_pos);
}

void ABCVm::abc_si32(call_context* context)
{
	//si32
	LOG_CALL( "si32");
	storeIntN<uint32_t>(context);
	++(context->exec_pos);
}
void ABCVm::abc_si32_constant_constant(call_context* context)
{
	LOG_CALL( "si32_cc");
	preloadedcodedata* instrptr = context->exec_pos;
	storeIntN<uint32_t>(context,*instrptr->arg2_constant,*instrptr->arg1_constant);
	++(context->exec_pos);
}
void ABCVm::abc_si32_local_constant(call_context* context)
{
	LOG_CALL( "si32_lc");
	preloadedcodedata* instrptr = context->exec_pos;
	storeIntN<uint32_t>(context,*instrptr->arg2_constant,context->locals[instrptr->local_pos1]);
	++(context->exec_pos);
}
void ABCVm::abc_si32_constant_local(call_context* context)
{
	LOG_CALL( "si32_cl");
	preloadedcodedata* instrptr = context->exec_pos;
	storeIntN<uint32_t>(context,context->locals[instrptr->local_pos2],*instrptr->arg1_constant);
	++(context->exec_pos);
}
void ABCVm::abc_si32_local_local(call_context* context)
{
	LOG_CALL( "si32_ll");
	preloadedcodedata* instrptr = context->exec_pos;
	storeIntN<uint32_t>(context,context->locals[instrptr->local_pos2],context->locals[instrptr->local_pos1]);
	++(context->exec_pos);
}

void ABCVm::abc_sf32(call_context* context)
{
	//sf32
	LOG_CALL( "sf32");
	storeFloat(context);
	++(context->exec_pos);
}
void ABCVm::abc_sf32_constant_constant(call_context* context)
{
	LOG_CALL( "sf32_cc");
	preloadedcodedata* instrptr = context->exec_pos;
	storeFloat(context,*instrptr->arg2_constant,*instrptr->arg1_constant);
	++(context->exec_pos);
}
void ABCVm::abc_sf32_local_constant(call_context* context)
{
	LOG_CALL( "sf32_lc");
	preloadedcodedata* instrptr = context->exec_pos;
	storeFloat(context,*instrptr->arg2_constant,context->locals[instrptr->local_pos1]);
	++(context->exec_pos);
}
void ABCVm::abc_sf32_constant_local(call_context* context)
{
	LOG_CALL( "sf32_cl");
	preloadedcodedata* instrptr = context->exec_pos;
	storeFloat(context,context->locals[instrptr->local_pos2],*instrptr->arg1_constant);
	++(context->exec_pos);
}
void ABCVm::abc_sf32_local_local(call_context* context)
{
	LOG_CALL( "sf32_ll");
	preloadedcodedata* instrptr = context->exec_pos;
	storeFloat(context,context->locals[instrptr->local_pos2],context->locals[instrptr->local_pos1]);
	++(context->exec_pos);
}
void ABCVm::abc_sf64(call_context* context)
{
	//sf64
	LOG_CALL( "sf64");
	storeDouble(context);
	++(context->exec_pos);
}
void ABCVm::abc_sf64_constant_constant(call_context* context)
{
	LOG_CALL( "sf64_cc");
	preloadedcodedata* instrptr = context->exec_pos;
	storeDouble(context,*instrptr->arg2_constant,*instrptr->arg1_constant);
	++(context->exec_pos);
}
void ABCVm::abc_sf64_local_constant(call_context* context)
{
	LOG_CALL( "sf64_lc");
	preloadedcodedata* instrptr = context->exec_pos;
	storeDouble(context,*instrptr->arg2_constant,context->locals[instrptr->local_pos1]);
	++(context->exec_pos);
}
void ABCVm::abc_sf64_constant_local(call_context* context)
{
	LOG_CALL( "sf64_cl");
	preloadedcodedata* instrptr = context->exec_pos;
	storeDouble(context,context->locals[instrptr->local_pos2],*instrptr->arg1_constant);
	++(context->exec_pos);
}
void ABCVm::abc_sf64_local_local(call_context* context)
{
	LOG_CALL( "sf64_ll");
	preloadedcodedata* instrptr = context->exec_pos;
	storeDouble(context,context->locals[instrptr->local_pos2],context->locals[instrptr->local_pos1]);
	++(context->exec_pos);
}

void ABCVm::abc_newfunction(call_context* context)
{
	//newfunction
	uint32_t t = (++(context->exec_pos))->data;
	RUNTIME_STACK_PUSH(context,asAtomHandler::fromObjectNoPrimitive(newFunction(context,t)));
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
void ABCVm::construct_noargs_intern(call_context* context,asAtom& ret,asAtom& obj)
{
	LOG_CALL(_("Constructing"));

	switch(asAtomHandler::getObjectType(obj))
	{
		case T_CLASS:
		{
			Class_base* o_class=asAtomHandler::as<Class_base>(obj);
			o_class->getInstance(ret,true,nullptr,0);
			break;
		}
		case T_FUNCTION:
		{
			constructFunction(ret,context, obj, nullptr,0);
			break;
		}

		default:
		{
			throwError<TypeError>(kConstructOfNonFunctionError);
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
	asAtom obj= context->locals[context->exec_pos->local_pos1];
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
	ASObject* o = asAtomHandler::getObject(context->locals[instrptr->local_pos3-1]);
	asAtomHandler::set(context->locals[instrptr->local_pos3-1],res);
	if (o)
		o->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_construct_local_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj= context->locals[context->exec_pos->local_pos1];
	LOG_CALL( "construct_noargs_l_lr ");
	asAtom res=asAtomHandler::invalidAtom;
	construct_noargs_intern(context,res,obj);
	ASObject* o = asAtomHandler::getObject(context->locals[instrptr->local_pos3-1]);
	asAtomHandler::set(context->locals[instrptr->local_pos3-1],res);
	if (o)
		o->decRef();
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
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t t = (++(context->exec_pos))->data;
	uint32_t t2 = (++(context->exec_pos))->data;
	callPropIntern(context,t,t2,true,false,instrptr);
	++(context->exec_pos);
}

void callprop_intern(call_context* context,asAtom& ret,asAtom& obj,asAtom* args, uint32_t argsnum,multiname* name,preloadedcodedata* cacheptr,bool refcounted, bool needreturn, bool coercearguments)
{
	if ((cacheptr->data&ABC_OP_CACHED) == ABC_OP_CACHED)
	{
		if (asAtomHandler::isObject(obj) && 
				((asAtomHandler::is<Class_base>(obj) && asAtomHandler::getObjectNoCheck(obj) == cacheptr->cacheobj1)
				|| asAtomHandler::getObjectNoCheck(obj)->getClass() == cacheptr->cacheobj1))
		{
			asAtom o = asAtomHandler::fromObjectNoPrimitive(cacheptr->cacheobj3);
			LOG_CALL( "callProperty from cache:"<<*name<<" "<<asAtomHandler::toDebugString(obj)<<" "<<asAtomHandler::toDebugString(o)<<" "<<coercearguments);
			if(asAtomHandler::is<IFunction>(o))
				asAtomHandler::callFunction(o,ret,obj,args,argsnum,refcounted,needreturn,coercearguments);
			else if(asAtomHandler::is<Class_base>(o))
			{
				asAtomHandler::as<Class_base>(o)->generator(ret,args,argsnum);
				if (refcounted)
				{
					for(uint32_t i=0;i<argsnum;i++)
						ASATOM_DECREF(args[i]);
					ASATOM_DECREF(obj);
				}
			}
			else if(asAtomHandler::is<RegExp>(o))
				RegExp::exec(ret,context->mi->context->root->getSystemState(),o,args,argsnum);
			else
			{
				LOG(LOG_ERROR,"trying to call an object as a function:"<<asAtomHandler::toDebugString(o) <<" on "<<asAtomHandler::toDebugString(obj));
				throwError<TypeError>(kCallOfNonFunctionError, "Object");
			}
			LOG_CALL("End of calling cached property "<<*name<<" "<<asAtomHandler::toDebugString(ret));
			return;
		}
		else
		{
			if (cacheptr->cacheobj3 && cacheptr->cacheobj3->is<Function>() && cacheptr->cacheobj3->as<IFunction>()->isCloned)
				cacheptr->cacheobj3->decRef();
			cacheptr->data |= ABC_OP_NOTCACHEABLE;
			cacheptr->data &= ~ABC_OP_CACHED;
		}
	}
	if(asAtomHandler::is<Null>(obj))
	{
		LOG(LOG_ERROR,"trying to call property on null:"<<*name);
		throwError<TypeError>(kConvertNullToObjectError);
	}
	if (asAtomHandler::is<Undefined>(obj))
	{
		LOG(LOG_ERROR,"trying to call property on undefined:"<<*name);
		throwError<TypeError>(kConvertUndefinedToObjectError);
	}
	ASObject* pobj = asAtomHandler::getObject(obj);
	asAtom o=asAtomHandler::invalidAtom;
	bool canCache = false;
	if (!pobj)
	{
		// fast path for primitives to avoid creation of ASObjects
		asAtomHandler::getVariableByMultiname(obj,o,context->mi->context->root->getSystemState(),*name);
		canCache = asAtomHandler::isValid(o);
	}
	if(asAtomHandler::isInvalid(o))
	{
		pobj = asAtomHandler::toObject(obj,context->mi->context->root->getSystemState());
		//We should skip the special implementation of get
		canCache = pobj->getVariableByMultiname(o,*name, GET_VARIABLE_OPTION(SKIP_IMPL | NO_INCREF)) & GET_VARIABLE_RESULT::GETVAR_CACHEABLE;
	}
	name->resetNameIfObject();
	if(asAtomHandler::isInvalid(o) && asAtomHandler::is<Class_base>(obj))
	{
		// check super classes
		_NR<Class_base> tmpcls = asAtomHandler::as<Class_base>(obj)->super;
		while (tmpcls && !tmpcls.isNull())
		{
			tmpcls->getVariableByMultiname(o,*name, GET_VARIABLE_OPTION(SKIP_IMPL | NO_INCREF));
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
					&& (cacheptr->data & ABC_OP_NOTCACHEABLE)==0 
					&& asAtomHandler::canCacheMethod(obj,name) 
					&& asAtomHandler::getObject(o) 
					&& ((asAtomHandler::is<Class_base>(obj) && asAtomHandler::as<IFunction>(o)->inClass == asAtomHandler::as<Class_base>(obj)) || (asAtomHandler::as<IFunction>(o)->inClass && asAtomHandler::getClass(obj,context->mi->context->root->getSystemState())->isSubClass(asAtomHandler::as<IFunction>(o)->inClass))))
			{
				// cache method if multiname is static and it is a method of a sealed class
				cacheptr->data |= ABC_OP_CACHED;
				if (argsnum==2 && asAtomHandler::is<SyntheticFunction>(o) && cacheptr->cacheobj1 && cacheptr->cacheobj3) // special case 2 parameters with known parameter types: check if coercion can be skipped
				{
					SyntheticFunction* f = asAtomHandler::as<SyntheticFunction>(o);
					if (!f->getMethodInfo()->returnType)
						f->checkParamTypes();
					if (f->getMethodInfo()->paramTypes.size()==2 &&
						f->canSkipCoercion(0,(Class_base*)cacheptr->cacheobj1) &&
						f->canSkipCoercion(1,(Class_base*)cacheptr->cacheobj3))
					{
						cacheptr->data |= ABC_OP_COERCED;
					}
				}
				cacheptr->cacheobj1 = asAtomHandler::getClass(obj,context->mi->context->root->getSystemState());
				cacheptr->cacheobj3 = asAtomHandler::getObject(o);
				LOG_CALL("caching callproperty:"<<*name<<" "<<cacheptr->cacheobj1->toDebugString()<<" "<<cacheptr->cacheobj3->toDebugString());
			}
			else
			{
				cacheptr->data |= ABC_OP_NOTCACHEABLE;
				cacheptr->data &= ~ABC_OP_CACHED;
			}
			obj = asAtomHandler::getClosureAtom(o,obj);
			asAtomHandler::callFunction(o,ret,obj,args,argsnum,refcounted,needreturn,coercearguments);
			if (!(cacheptr->data & ABC_OP_CACHED) && asAtomHandler::as<IFunction>(o)->isCloned)
				asAtomHandler::as<IFunction>(o)->decRef();
		}
		else if(asAtomHandler::is<Class_base>(o))
		{
			asAtomHandler::as<Class_base>(o)->generator(ret,args,argsnum);
			if (refcounted)
			{
				for(uint32_t i=0;i<argsnum;i++)
					ASATOM_DECREF(args[i]);
				ASATOM_DECREF(obj);
			}
		}
		else if(asAtomHandler::is<RegExp>(o))
			RegExp::exec(ret,context->mi->context->root->getSystemState(),o,args,argsnum);
		else
		{
			LOG(LOG_ERROR,"trying to call an object as a function:"<<asAtomHandler::toDebugString(o) <<" on "<<asAtomHandler::toDebugString(obj));
			throwError<TypeError>(kCallOfNonFunctionError, "Object");
		}
	}
	else
	{
		//If the object is a Proxy subclass, try to invoke callProperty
		if(asAtomHandler::is<Proxy>(obj))
		{
			//Check if there is a custom caller defined, skipping implementation to avoid recursive calls
			multiname callPropertyName(NULL);
			callPropertyName.name_type=multiname::NAME_STRING;
			callPropertyName.name_s_id=context->mi->context->root->getSystemState()->getUniqueStringId("callProperty");
			callPropertyName.ns.emplace_back(context->mi->context->root->getSystemState(),flash_proxy,NAMESPACE);
			asAtom oproxy=asAtomHandler::invalidAtom;
			pobj->getVariableByMultiname(oproxy,callPropertyName,SKIP_IMPL);
			if(asAtomHandler::isValid(oproxy))
			{
				assert_and_throw(asAtomHandler::isFunction(oproxy));
				if(asAtomHandler::isValid(o))
				{
					if(asAtomHandler::is<IFunction>(o))
						asAtomHandler::callFunction(o,ret,obj,args,argsnum,refcounted,needreturn,coercearguments);
					else if(asAtomHandler::is<Class_base>(o))
					{
						asAtomHandler::as<Class_base>(o)->generator(ret,args,argsnum);
						if (refcounted)
						{
							for(uint32_t i=0;i<argsnum;i++)
								ASATOM_DECREF(args[i]);
							ASATOM_DECREF(obj);
						}
					}
					else if(asAtomHandler::is<RegExp>(o))
						RegExp::exec(ret,context->mi->context->root->getSystemState(),o,args,argsnum);
					else
					{
						LOG(LOG_ERROR,"trying to call an object as a function:"<<asAtomHandler::toDebugString(o) <<" on "<<asAtomHandler::toDebugString(obj));
						throwError<TypeError>(kCallOfNonFunctionError, "Object");
					}
				}
				else
				{
					//Create a new array
					asAtom* proxyArgs=g_newa(asAtom,argsnum+1);
					ASObject* namearg = abstract_s(context->mi->context->root->getSystemState(),name->normalizedName(context->mi->context->root->getSystemState()));
					namearg->setProxyProperty(*name);
					proxyArgs[0]=asAtomHandler::fromObject(namearg);
					for(uint32_t i=0;i<argsnum;i++)
						proxyArgs[i+1]=args[i];
					
					//We now suppress special handling
					LOG_CALL(_("Proxy::callProperty"));
					ASATOM_INCREF(oproxy);
					asAtomHandler::callFunction(oproxy,ret,obj,proxyArgs,argsnum+1,true,needreturn,coercearguments);
					ASATOM_DECREF(oproxy);
				}
				LOG_CALL(_("End of calling proxy custom caller ") << *name);
			}
			else if(asAtomHandler::isValid(o))
			{
				if(asAtomHandler::is<IFunction>(o))
					asAtomHandler::callFunction(o,ret,obj,args,argsnum,refcounted,needreturn,coercearguments);
				else if(asAtomHandler::is<Class_base>(o))
				{
					asAtomHandler::as<Class_base>(o)->generator(ret,args,argsnum);
					if (refcounted)
					{
						for(uint32_t i=0;i<argsnum;i++)
							ASATOM_DECREF(args[i]);
						ASATOM_DECREF(obj);
					}
				}
				else if(asAtomHandler::is<RegExp>(o))
					RegExp::exec(ret,context->mi->context->root->getSystemState(),o,args,argsnum);
				else
				{
					LOG(LOG_ERROR,"trying to call an object as a function:"<<asAtomHandler::toDebugString(o) <<" on "<<asAtomHandler::toDebugString(obj));
					throwError<TypeError>(kCallOfNonFunctionError, "Object");
				}
				LOG_CALL(_("End of calling proxy ") << *name);
			}
		}
		if (asAtomHandler::isInvalid(ret))
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
			if (asAtomHandler::is<Class_base>(obj))
			{
				tiny_string clsname = asAtomHandler::as<Class_base>(obj)->class_name.getQualifiedName(context->mi->context->root->getSystemState());
				throwError<TypeError>(kCallOfNonFunctionError, name->qualifiedString(context->mi->context->root->getSystemState()), clsname);
			}
			else
			{
				tiny_string clsname = pobj->getClassName();
				throwError<TypeError>(kCallNotFoundError, name->qualifiedString(context->mi->context->root->getSystemState()), clsname);
			}
			asAtomHandler::setUndefined(ret);
		}
	}
	LOG_CALL(_("End of calling ") << *name<<" "<<asAtomHandler::toDebugString(ret));
}

void ABCVm::abc_callpropertyStaticName(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t argcount = (instrptr->data&ABC_OP_AVAILABLEBITS) >>OPCODE_SIZE;
	multiname* name=instrptr->cachedmultiname2;
	LOG_CALL( "callProperty_staticname " << *name<<" "<<argcount);
	RUNTIME_STACK_POP_N_CREATE(context,argcount+1,args);
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern(context,ret,*args,args+1,argcount,name,instrptr,true,true,(instrptr->data&ABC_OP_COERCED)==0);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_callpropertyStaticName_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));

	RUNTIME_STACK_POP_CREATE(context,args);
	multiname* name=context->exec_pos->cachedmultiname2;

	LOG_CALL( "callProperty_l " << *name);
	RUNTIME_STACK_POP_CREATE(context,obj);
	asAtom res=asAtomHandler::invalidAtom;
	callprop_intern(context,res,*obj,args,1,name,context->exec_pos,true,true,true);
	ASObject* o = asAtomHandler::getObject(context->locals[instrptr->local_pos3-1]);
	asAtomHandler::set(context->locals[instrptr->local_pos3-1],res);
	if (o)
		o->decRef();

	++(context->exec_pos);
}
void ABCVm::abc_callpropertyStaticNameCached(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t argcount = (instrptr->data&ABC_OP_AVAILABLEBITS) >>OPCODE_SIZE;
	multiname* name=instrptr->cachedmultiname2;
	LOG_CALL( "callProperty_staticnameCached " << *name<<" "<<argcount);
	asAtom* args = g_newa(asAtom,argcount);
	for (uint32_t i = argcount; i > 0 ; i--)
	{
		(++(context->exec_pos));
		if ((context->exec_pos->data>>OPCODE_SIZE) == OPERANDTYPES::OP_LOCAL)
			args[i-1] = context->locals[context->exec_pos->local_pos1];
		else
			args[i-1] = *context->exec_pos->arg1_constant;
	}
	RUNTIME_STACK_POP_CREATE(context,obj);
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern(context,ret,*obj,args,argcount,name,instrptr,false,true,(instrptr->data&ABC_OP_COERCED)==0);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_callpropertyStaticNameCached_localResult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t argcount = (instrptr->data&ABC_OP_AVAILABLEBITS) >>OPCODE_SIZE;
	multiname* name=instrptr->cachedmultiname2;
	LOG_CALL( "callProperty_staticnameCached_l " << *name<<" "<<argcount);
	asAtom* args = g_newa(asAtom,argcount);
	for (uint32_t i = argcount; i > 0 ; i--)
	{
		(++(context->exec_pos));
		if ((context->exec_pos->data>>OPCODE_SIZE) == OPERANDTYPES::OP_LOCAL)
			args[i-1] = context->locals[context->exec_pos->local_pos1];
		else
			args[i-1] = *context->exec_pos->arg1_constant;
	}
	RUNTIME_STACK_POP_CREATE(context,obj);
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern(context,ret,*obj,args,argcount,name,instrptr,false,true,(instrptr->data&ABC_OP_COERCED)==0);
	// local result pos is stored in the context->exec_pos of the last argument
	ASObject* o = asAtomHandler::getObject(context->locals[context->exec_pos->local_pos3-1]);
	asAtomHandler::set(context->locals[context->exec_pos->local_pos3-1],ret);
	if (o)
		o->decRef();
	++(context->exec_pos);
}

void ABCVm::abc_callpropertyStaticNameCached_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t argcount = (instrptr->data&ABC_OP_AVAILABLEBITS) >>OPCODE_SIZE;
	multiname* name=instrptr->cachedmultiname2;
	LOG_CALL( "callProperty_staticnameCached_c " << *name<<" "<<argcount);
	asAtom* args = g_newa(asAtom,argcount);
	for (uint32_t i = argcount; i > 0 ; i--)
	{
		(++(context->exec_pos));
		if ((context->exec_pos->data>>OPCODE_SIZE) == OPERANDTYPES::OP_LOCAL)
			args[i-1] = context->locals[context->exec_pos->local_pos1];
		else
			args[i-1] = *context->exec_pos->arg1_constant;
	}
	asAtom obj = *context->exec_pos->arg2_constant;
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern(context,ret,obj,args,argcount,name,instrptr,false,true,(instrptr->data&ABC_OP_COERCED)==0);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_callpropertyStaticNameCached_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t argcount = (instrptr->data&ABC_OP_AVAILABLEBITS) >>OPCODE_SIZE;
	multiname* name=instrptr->cachedmultiname2;
	LOG_CALL( "callProperty_staticnameCached_l " << *name<<" "<<argcount);
	asAtom* args = g_newa(asAtom,argcount);
	for (uint32_t i = argcount; i > 0 ; i--)
	{
		(++(context->exec_pos));
		if ((context->exec_pos->data>>OPCODE_SIZE) == OPERANDTYPES::OP_LOCAL)
			args[i-1] = context->locals[context->exec_pos->local_pos1];
		else
			args[i-1] = *context->exec_pos->arg1_constant;
	}
	asAtom obj = context->locals[context->exec_pos->local_pos2];
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern(context,ret,obj,args,argcount,name,instrptr,false,true,(instrptr->data&ABC_OP_COERCED)==0);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_callpropertyStaticNameCached_constant_localResult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t argcount = (instrptr->data&ABC_OP_AVAILABLEBITS) >>OPCODE_SIZE;
	multiname* name=instrptr->cachedmultiname2;
	LOG_CALL( "callProperty_staticnameCached_cl " << *name<<" "<<argcount);
	asAtom* args = g_newa(asAtom,argcount);
	for (uint32_t i = argcount; i > 0 ; i--)
	{
		(++(context->exec_pos));
		if ((context->exec_pos->data>>OPCODE_SIZE) == OPERANDTYPES::OP_LOCAL)
			args[i-1] = context->locals[context->exec_pos->local_pos1];
		else
			args[i-1] = *context->exec_pos->arg1_constant;
	}
	asAtom obj = *context->exec_pos->arg2_constant;
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern(context,ret,obj,args,argcount,name,instrptr,false,true,(instrptr->data&ABC_OP_COERCED)==0);
	// local result pos is stored in the context->exec_pos of the last argument
	ASObject* o = asAtomHandler::getObject(context->locals[context->exec_pos->local_pos3-1]);
	asAtomHandler::set(context->locals[context->exec_pos->local_pos3-1],ret);
	if (o)
		o->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_callpropertyStaticNameCached_local_localResult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t argcount = (instrptr->data&ABC_OP_AVAILABLEBITS) >>OPCODE_SIZE;
	multiname* name=instrptr->cachedmultiname2;
	LOG_CALL( "callProperty_staticnameCached_ll " << *name<<" "<<argcount);
	asAtom* args = g_newa(asAtom,argcount);
	for (uint32_t i = argcount; i > 0 ; i--)
	{
		(++(context->exec_pos));
		if ((context->exec_pos->data>>OPCODE_SIZE) == OPERANDTYPES::OP_LOCAL)
			args[i-1] = context->locals[context->exec_pos->local_pos1];
		else
			args[i-1] = *context->exec_pos->arg1_constant;
	}
	asAtom obj = context->locals[context->exec_pos->local_pos2];
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern(context,ret,obj,args,argcount,name,instrptr,false,true,(instrptr->data&ABC_OP_COERCED)==0);
	// local result pos is stored in the context->exec_pos of the last argument
	ASObject* o = asAtomHandler::getObject(context->locals[context->exec_pos->local_pos3-1]);
	asAtomHandler::set(context->locals[context->exec_pos->local_pos3-1],ret);
	if (o)
		o->decRef();
	++(context->exec_pos);
}

void ABCVm::abc_callpropertyStaticName_constant_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));

	asAtom args=*instrptr->arg2_constant;
	multiname* name=context->exec_pos->cachedmultiname2;

	asAtom obj= *instrptr->arg1_constant;
	LOG_CALL( "callProperty_cc " << *name);
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern(context,ret,obj,&args,1,name,context->exec_pos,false,true,true);
	RUNTIME_STACK_PUSH(context,ret);

	++(context->exec_pos);
}
void ABCVm::abc_callpropertyStaticName_local_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));

	asAtom args=*instrptr->arg2_constant;
	multiname* name=context->exec_pos->cachedmultiname2;

	asAtom obj= context->locals[instrptr->local_pos1];
	LOG_CALL( "callProperty_lc " << *name);
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern(context,ret,obj,&args,1,name,context->exec_pos,false,true,true);
	RUNTIME_STACK_PUSH(context,ret);

	++(context->exec_pos);
}
void ABCVm::abc_callpropertyStaticName_constant_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));

	asAtom* args=&context->locals[instrptr->local_pos2];
	multiname* name=context->exec_pos->cachedmultiname2;

	asAtom obj= *instrptr->arg1_constant;
	LOG_CALL( "callProperty_cl " << *name);
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern(context,ret,obj,args,1,name,context->exec_pos,false,true,true);
	RUNTIME_STACK_PUSH(context,ret);

	++(context->exec_pos);
}
void ABCVm::abc_callpropertyStaticName_local_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));

	asAtom* args=&context->locals[instrptr->local_pos2];
	multiname* name=context->exec_pos->cachedmultiname2;

	asAtom obj= context->locals[instrptr->local_pos1];
	LOG_CALL( "callProperty_ll " << *name);
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern(context,ret,obj,args,1,name,context->exec_pos,false,true,true);
	RUNTIME_STACK_PUSH(context,ret);

	++(context->exec_pos);
}
void ABCVm::abc_callpropertyStaticName_constant_constant_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));

	asAtom args=*instrptr->arg2_constant;
	multiname* name=context->exec_pos->cachedmultiname2;

	asAtom obj= *instrptr->arg1_constant;
	LOG_CALL( "callProperty_ccl " << *name);
	asAtom res=asAtomHandler::invalidAtom;
	callprop_intern(context,res,obj,&args,1,name,context->exec_pos,false,true,true);
	ASObject* o = asAtomHandler::getObject(context->locals[instrptr->local_pos3-1]);
	asAtomHandler::set(context->locals[instrptr->local_pos3-1],res);
	if (o)
		o->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_callpropertyStaticName_local_constant_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));

	asAtom args=*instrptr->arg2_constant;
	multiname* name=context->exec_pos->cachedmultiname2;

	asAtom obj= context->locals[instrptr->local_pos1];
	LOG_CALL( "callProperty_lcl " << *name);
	asAtom res=asAtomHandler::invalidAtom;
	callprop_intern(context,res,obj,&args,1,name,context->exec_pos,false,true,true);
	ASObject* o = asAtomHandler::getObject(context->locals[instrptr->local_pos3-1]);
	asAtomHandler::set(context->locals[instrptr->local_pos3-1],res);
	if (o)
		o->decRef();

	++(context->exec_pos);
}
void ABCVm::abc_callpropertyStaticName_constant_local_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));

	asAtom* args=&context->locals[instrptr->local_pos2];
	multiname* name=context->exec_pos->cachedmultiname2;

	asAtom obj= *instrptr->arg1_constant;
	LOG_CALL( "callProperty_cll " << *name);
	asAtom res=asAtomHandler::invalidAtom;
	callprop_intern(context,res,obj,args,1,name,context->exec_pos,false,true,true);
	ASObject* o = asAtomHandler::getObject(context->locals[instrptr->local_pos3-1]);
	asAtomHandler::set(context->locals[instrptr->local_pos3-1],res);
	if (o)
		o->decRef();

	++(context->exec_pos);
}
void ABCVm::abc_callpropertyStaticName_local_local_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));

	asAtom* args=&context->locals[instrptr->local_pos2];
	multiname* name=context->exec_pos->cachedmultiname2;

	asAtom obj= context->locals[instrptr->local_pos1];
	LOG_CALL( "callProperty_lll " << *name);
	asAtom res=asAtomHandler::invalidAtom;
	callprop_intern(context,res,obj,args,1,name,context->exec_pos,false,true,true);
	ASObject* o = asAtomHandler::getObject(context->locals[instrptr->local_pos3-1]);
	asAtomHandler::set(context->locals[instrptr->local_pos3-1],res);
	if (o)
		o->decRef();

	++(context->exec_pos);
}

void ABCVm::abc_callpropertyStaticName_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));

	multiname* name=context->exec_pos->cachedmultiname2;

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
	(++(context->exec_pos));

	multiname* name=context->exec_pos->cachedmultiname2;

	asAtom obj= context->locals[instrptr->local_pos1];
	LOG_CALL( "callProperty_noargs_l " << *name);
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern(context,ret,obj,nullptr,0,name,context->exec_pos,false,true,false);
	RUNTIME_STACK_PUSH(context,ret);

	++(context->exec_pos);
}
void ABCVm::abc_callpropertyStaticName_constant_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));

	multiname* name=context->exec_pos->cachedmultiname2;

	asAtom obj= *instrptr->arg1_constant;
	LOG_CALL( "callProperty_noargs_c_lr " << *name);
	asAtom res=asAtomHandler::invalidAtom;
	callprop_intern(context,res,obj,nullptr,0,name,context->exec_pos,false,true,false);
	ASObject* o = asAtomHandler::getObject(context->locals[instrptr->local_pos3-1]);
	asAtomHandler::set(context->locals[instrptr->local_pos3-1],res);
	if (o)
		o->decRef();

	++(context->exec_pos);
}
void ABCVm::abc_callpropertyStaticName_local_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));

	multiname* name=context->exec_pos->cachedmultiname2;

	asAtom obj= context->locals[instrptr->local_pos1];
	LOG_CALL( "callProperty_noargs_l_lr " << *name);
	asAtom res=asAtomHandler::invalidAtom;
	callprop_intern(context,res,obj,nullptr,0,name,context->exec_pos,false,true,false);
	ASObject* o = asAtomHandler::getObject(context->locals[instrptr->local_pos3-1]);
	asAtomHandler::set(context->locals[instrptr->local_pos3-1],res);
	if (o)
		o->decRef();

	++(context->exec_pos);
}

void ABCVm::abc_returnvoid(call_context* context)
{
	//returnvoid
	LOG_CALL(_("returnVoid"));
	context->returnvalue = asAtomHandler::invalidAtom;
	context->returning = true;
	++(context->exec_pos);
}
void ABCVm::abc_returnvalue(call_context* context)
{
	//returnvalue
	RUNTIME_STACK_POP(context,context->returnvalue);
	LOG_CALL(_("returnValue ") << asAtomHandler::toDebugString(context->returnvalue));
	context->returning = true;
	++(context->exec_pos);
}
void ABCVm::abc_returnvalue_constant(call_context* context)
{
	//returnvalue
	asAtomHandler::set(context->returnvalue,*context->exec_pos->arg1_constant);
	LOG_CALL(_("returnValue_c ") << asAtomHandler::toDebugString(context->returnvalue));
	context->returning = true;
	++(context->exec_pos);
}
void ABCVm::abc_returnvalue_local(call_context* context)
{
	//returnvalue
	asAtomHandler::set(context->returnvalue,context->locals[context->exec_pos->local_pos1]);
	ASATOM_INCREF(context->locals[context->exec_pos->local_pos1]);
	LOG_CALL(_("returnValue_l ") << asAtomHandler::toDebugString(context->returnvalue));
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
void ABCVm::abc_constructsuper_constant(call_context* context)
{
	LOG_CALL( _("constructSuper_c "));
	asAtom obj=*context->exec_pos->arg1_constant;
	context->inClass->super->handleConstruction(obj,nullptr, 0, false);
	LOG_CALL(_("End super construct ")<<asAtomHandler::toDebugString(obj));
	++(context->exec_pos);
}
void ABCVm::abc_constructsuper_local(call_context* context)
{
	LOG_CALL( _("constructSuper_l "));
	asAtom obj= context->locals[context->exec_pos->local_pos1];
	context->inClass->super->handleConstruction(obj,nullptr, 0, false);
	LOG_CALL(_("End super construct ")<<asAtomHandler::toDebugString(obj));
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

void ABCVm::constructpropnoargs_intern(call_context* context,asAtom& ret,asAtom& obj,multiname* name, ASObject* constructor)
{
	asAtom o=asAtomHandler::invalidAtom;
	if (constructor)
		o = asAtomHandler::fromObjectNoPrimitive(constructor);
	else
		asAtomHandler::toObject(obj,context->mi->context->root->getSystemState())->getVariableByMultiname(o,*name);

	if(asAtomHandler::isInvalid(o))
	{
		if (asAtomHandler::is<Undefined>(obj))
		{
			ASATOM_DECREF(obj);
			throwError<TypeError>(kConvertUndefinedToObjectError);
		}
		if (asAtomHandler::isPrimitive(obj))
		{
			ASATOM_DECREF(obj);
			throwError<TypeError>(kConstructOfNonFunctionError);
		}
		throwError<ReferenceError>(kUndefinedVarError, name->normalizedNameUnresolved(context->mi->context->root->getSystemState()));
	}
	try
	{
		if(asAtomHandler::isClass(o))
		{
			Class_base* o_class=asAtomHandler::as<Class_base>(o);
			o_class->getInstance(ret,true,nullptr,0);
		}
		else if(asAtomHandler::isFunction(o))
		{
			constructFunction(ret,context, o, nullptr, 0);
		}
		else if (asAtomHandler::isTemplate(o))
			throwError<TypeError>(kConstructOfNonFunctionError);
		else
			throwError<TypeError>(kNotConstructorError);
	}
	catch(ASObject* exc)
	{
		LOG_CALL(_("Exception during object construction. Returning Undefined"));
		//Handle eventual exceptions from the constructor, to fix the stack
		RUNTIME_STACK_PUSH(context,obj);
		throw;
	}
	if (asAtomHandler::isObject(ret))
		asAtomHandler::getObjectNoCheck(ret)->setConstructorCallComplete();
	LOG_CALL(_("End of constructing ") << asAtomHandler::toDebugString(ret));
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
	asAtom obj= context->locals[context->exec_pos->local_pos1];
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
	ASObject* o = asAtomHandler::getObject(context->locals[instrptr->local_pos3-1]);
	asAtomHandler::set(context->locals[instrptr->local_pos3-1],res);
	if (o && o != asAtomHandler::getObject(res))
		o->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_constructpropStaticName_local_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	multiname* name=context->exec_pos->cachedmultiname2;
	asAtom obj= context->locals[context->exec_pos->local_pos1];
	++(context->exec_pos);
	LOG_CALL( "constructprop_noargs_l_lr " << *name);
	asAtom res=asAtomHandler::invalidAtom;
	constructpropnoargs_intern(context,res,obj,name,nullptr);
	ASObject* o = asAtomHandler::getObject(context->locals[instrptr->local_pos3-1]);
	asAtomHandler::set(context->locals[instrptr->local_pos3-1],res);
	if (o && o != asAtomHandler::getObject(res))
		o->decRef();
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
void ABCVm::abc_callpropvoidStaticName(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t argcount = (instrptr->data&ABC_OP_AVAILABLEBITS) >>OPCODE_SIZE;
	multiname* name=instrptr->cachedmultiname2;
	LOG_CALL( "callPropvoid_staticname " << *name<<" "<<argcount);
	RUNTIME_STACK_POP_N_CREATE(context,argcount+1,args);
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern(context,ret,*args,args+1,argcount,name,instrptr,true,false,(instrptr->data&ABC_OP_COERCED)==0);
	ASATOM_DECREF(ret);
	++(context->exec_pos);
}
void ABCVm::abc_callpropvoidStaticNameCached(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t argcount = (instrptr->data&ABC_OP_AVAILABLEBITS) >>OPCODE_SIZE;
	multiname* name=instrptr->cachedmultiname2;
	LOG_CALL( "callPropvoid_staticnameCached " << *name<<" "<<argcount);
	asAtom* args = g_newa(asAtom,argcount);
	for (uint32_t i = argcount; i > 0 ; i--)
	{
		(++(context->exec_pos));
		if ((context->exec_pos->data>>OPCODE_SIZE) == OPERANDTYPES::OP_LOCAL)
			args[i-1] = context->locals[context->exec_pos->local_pos1];
		else
			args[i-1] = *context->exec_pos->arg1_constant;
	}
	RUNTIME_STACK_POP_CREATE(context,obj);
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern(context,ret,*obj,args,argcount,name,instrptr,false,false,(instrptr->data&ABC_OP_COERCED)==0);
	ASATOM_DECREF(ret);
	++(context->exec_pos);
}
void ABCVm::abc_callpropvoidStaticNameCached_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t argcount = (instrptr->data&ABC_OP_AVAILABLEBITS) >>OPCODE_SIZE;
	multiname* name=instrptr->cachedmultiname2;
	LOG_CALL( "callPropvoid_staticnameCached_c " << *name<<" "<<argcount);
	asAtom* args = g_newa(asAtom,argcount);
	for (uint32_t i = argcount; i > 0 ; i--)
	{
		(++(context->exec_pos));
		if ((context->exec_pos->data>>OPCODE_SIZE) == OPERANDTYPES::OP_LOCAL)
			args[i-1] = context->locals[context->exec_pos->local_pos1];
		else
			args[i-1] = *context->exec_pos->arg1_constant;
	}
	asAtom obj = *context->exec_pos->arg2_constant;
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern(context,ret,obj,args,argcount,name,instrptr,false,false,(instrptr->data&ABC_OP_COERCED)==0);
	ASATOM_DECREF(ret);
	++(context->exec_pos);
}
void ABCVm::abc_callpropvoidStaticNameCached_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t argcount = (instrptr->data&ABC_OP_AVAILABLEBITS) >>OPCODE_SIZE;
	multiname* name=instrptr->cachedmultiname2;
	LOG_CALL( "callPropvoid_staticnameCached_c " << *name<<" "<<argcount);
	asAtom* args = g_newa(asAtom,argcount);
	for (uint32_t i = argcount; i > 0 ; i--)
	{
		(++(context->exec_pos));
		if ((context->exec_pos->data>>OPCODE_SIZE) == OPERANDTYPES::OP_LOCAL)
			args[i-1] = context->locals[context->exec_pos->local_pos1];
		else
			args[i-1] = *context->exec_pos->arg1_constant;
	}
	asAtom obj = context->locals[context->exec_pos->local_pos2];
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern(context,ret,obj,args,argcount,name,instrptr,false,false,(instrptr->data&ABC_OP_COERCED)==0);
	ASATOM_DECREF(ret);
	++(context->exec_pos);
}
void ABCVm::abc_callpropvoidStaticName_constant_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));

	asAtom args=*instrptr->arg2_constant;
	multiname* name=context->exec_pos->cachedmultiname2;

	asAtom obj= *instrptr->arg1_constant;
	LOG_CALL( "callPropvoid_cc " << *name);
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern(context,ret,obj,&args,1,name,context->exec_pos,false,false,true);
	ASATOM_DECREF(ret);

	++(context->exec_pos);
}
void ABCVm::abc_callpropvoidStaticName_local_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));

	asAtom args=*instrptr->arg2_constant;
	multiname* name=context->exec_pos->cachedmultiname2;

	asAtom obj= context->locals[instrptr->local_pos1];
	LOG_CALL( "callPropvoid_lc " << *name);
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern(context,ret,obj,&args,1,name,context->exec_pos,false,false,true);
	ASATOM_DECREF(ret);

	++(context->exec_pos);
}
void ABCVm::abc_callpropvoidStaticName_constant_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));

	asAtom* args=&context->locals[instrptr->local_pos2];
	multiname* name=context->exec_pos->cachedmultiname2;

	asAtom obj= *instrptr->arg1_constant;
	LOG_CALL( "callPropvoid_cl " << *name);
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern(context,ret,obj,args,1,name,context->exec_pos,false,false,true);
	ASATOM_DECREF(ret);
	++(context->exec_pos);
}
void ABCVm::abc_callpropvoidStaticName_local_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));

	asAtom* args=&context->locals[instrptr->local_pos2];
	multiname* name=context->exec_pos->cachedmultiname2;

	asAtom obj= context->locals[instrptr->local_pos1];
	LOG_CALL( "callPropvoid_ll " << *name);
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern(context,ret,obj,args,1,name,context->exec_pos,false,false,true);
	ASATOM_DECREF(ret);

	++(context->exec_pos);
}
void ABCVm::abc_callpropvoidStaticName_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));

	multiname* name=context->exec_pos->cachedmultiname2;

	asAtom obj= *instrptr->arg1_constant;
	LOG_CALL( "callPropvoid_noargs_c " << *name);
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern(context,ret,obj,nullptr,0,name,context->exec_pos,false,false,false);
	ASATOM_DECREF(ret);

	++(context->exec_pos);
}
void ABCVm::abc_callpropvoidStaticName_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));

	multiname* name=context->exec_pos->cachedmultiname2;

	asAtom obj= context->locals[instrptr->local_pos1];
	LOG_CALL( "callPropvoid_noargs_l " << *name);
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern(context,ret,obj,nullptr,0,name,context->exec_pos,false,false,false);
	ASATOM_DECREF(ret);

	++(context->exec_pos);
}

void ABCVm::abc_sxi1(call_context* context)
{
	//sxi1
	LOG_CALL( "sxi1");
	RUNTIME_STACK_POINTER_CREATE(context,arg1);
	int32_t ret=asAtomHandler::toUInt(*arg1)&0x1 ? -1 : 0;
	ASATOM_DECREF_POINTER(arg1);
	asAtomHandler::setInt(*arg1,context->mi->context->root->getSystemState(),ret);
	++(context->exec_pos);
}
void ABCVm::abc_sxi8(call_context* context)
{
	//sxi8
	LOG_CALL( "sxi8");
	RUNTIME_STACK_POINTER_CREATE(context,arg1);
	int32_t ret=(int8_t)asAtomHandler::toUInt(*arg1);
	ASATOM_DECREF_POINTER(arg1);
	asAtomHandler::setInt(*arg1,context->mi->context->root->getSystemState(),ret);
	++(context->exec_pos);
}
void ABCVm::abc_sxi16(call_context* context)
{
	//sxi16
	LOG_CALL( "sxi16");
	RUNTIME_STACK_POINTER_CREATE(context,arg1);
	int32_t ret=(int16_t)asAtomHandler::toUInt(*arg1);
	ASATOM_DECREF_POINTER(arg1);
	asAtomHandler::setInt(*arg1,context->mi->context->root->getSystemState(),ret);
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
	RUNTIME_STACK_PUSH(context,asAtomHandler::fromObjectNoPrimitive(newActivation(context, context->mi)));
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
	RUNTIME_STACK_PUSH(context,asAtomHandler::fromObjectNoPrimitive(newCatch(context,t)));
	++(context->exec_pos);
}
void ABCVm::abc_findpropstrict(call_context* context)
{
	//findpropstrict
//		uint32_t t = (++(context->exec_pos))->data;
//		multiname* name=context->mi->context->getMultiname(t,context);
//		RUNTIME_STACK_PUSH(context,asAtomHandler::fromObject(findPropStrict(context,name)));
//		name->resetNameIfObject();

	asAtom o=asAtomHandler::invalidAtom;
	findPropStrictCache(o,context);
	RUNTIME_STACK_PUSH(context,o);
	++(context->exec_pos);
}
void ABCVm::abc_findproperty(call_context* context)
{
	//findproperty
	uint32_t t = (++(context->exec_pos))->data;
	multiname* name=context->mi->context->getMultiname(t,context);
	RUNTIME_STACK_PUSH(context,asAtomHandler::fromObject(findProperty(context,name)));
	name->resetNameIfObject();
	++(context->exec_pos);
}
void ABCVm::abc_finddef(call_context* context)
{
	//finddef
	uint32_t t = (++(context->exec_pos))->data;
	multiname* name=context->mi->context->getMultiname(t,context);
	ASObject* target = NULL;
	asAtom o=asAtomHandler::invalidAtom;
	getCurrentApplicationDomain(context)->getVariableAndTargetByMultiname(o,*name, target);
	if (target)
	{
		target->incRef();
		RUNTIME_STACK_PUSH(context,asAtomHandler::fromObject(target));
	}
	else
		RUNTIME_STACK_PUSH(context,asAtomHandler::nullAtom);
	name->resetNameIfObject();
	++(context->exec_pos);
}
void ABCVm::abc_getlex(call_context* context)
{
	//getlex
	preloadedcodedata* instrptr = context->exec_pos;
	if ((instrptr->data&ABC_OP_CACHED) == ABC_OP_CACHED)
	{
		RUNTIME_STACK_PUSH(context,asAtomHandler::fromObject(instrptr->cacheobj1));
		instrptr->cacheobj1->incRef();
		LOG_CALL( "getLex from cache: " <<  instrptr->cacheobj1->toDebugString());
	}
	else if (getLex_multiname(context,instrptr->cachedmultiname2,0))
	{
		// put object in cache
		instrptr->data |= ABC_OP_CACHED;
		RUNTIME_STACK_PEEK_CREATE(context,v);
		
		instrptr->cacheobj1 = asAtomHandler::getObject(*v);
		instrptr->cacheobj2 = asAtomHandler::getClosure(*v);
	}
	++(context->exec_pos);
}
void ABCVm::abc_getlex_localresult(call_context* context)
{
	//getlex
	preloadedcodedata* instrptr = context->exec_pos;
	assert(instrptr->local_pos3 > 0);
	if ((instrptr->data&ABC_OP_CACHED) == ABC_OP_CACHED)
	{
		if(instrptr->cacheobj2)
			instrptr->cacheobj2->incRef();
		ASObject* o = asAtomHandler::getObject(context->locals[instrptr->local_pos3-1]);
		asAtomHandler::setFunction(context->locals[instrptr->local_pos3-1],instrptr->cacheobj1,nullptr);//,instrptr->cacheobj2);
		ASATOM_INCREF(context->locals[instrptr->local_pos3-1]);
		if (o)
			o->decRef();
		LOG_CALL( "getLex_l from cache: " <<  instrptr->cacheobj1->toDebugString());
	}
	else if (getLex_multiname(context,instrptr->cachedmultiname2,instrptr->local_pos3))
	{
		// put object in cache
		instrptr->data |= ABC_OP_CACHED;
		instrptr->cacheobj1 = asAtomHandler::getObject(context->locals[instrptr->local_pos3-1]);
		instrptr->cacheobj2 = asAtomHandler::getClosure(context->locals[instrptr->local_pos3-1]);
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

	LOG_CALL(_("setProperty ") << *name << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));

	if(asAtomHandler::isNull(*obj))
	{
		LOG(LOG_ERROR,"calling setProperty on null:" << *name << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));
		ASATOM_DECREF_POINTER(obj);
		ASATOM_DECREF_POINTER(value);
		throwError<TypeError>(kConvertNullToObjectError);
	}
	if (asAtomHandler::isUndefined(*obj))
	{
		LOG(LOG_ERROR,"calling setProperty on undefined:" << *name << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));
		ASATOM_DECREF_POINTER(obj);
		ASATOM_DECREF_POINTER(value);
		throwError<TypeError>(kConvertUndefinedToObjectError);
	}
	//Do not allow to set contant traits
	ASObject* o = asAtomHandler::toObject(*obj,context->mi->context->root->getSystemState());
	bool alreadyset=false;
	o->setVariableByMultiname(*name,*value,ASObject::CONST_NOT_ALLOWED,&alreadyset);
	if (alreadyset)
		ASATOM_DECREF_POINTER(value);
	o->decRef();
	name->resetNameIfObject();
	++(context->exec_pos);
}

void ABCVm::abc_setPropertyStaticName_constant_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));
	multiname* name=context->exec_pos->cachedmultiname2;
	asAtom* obj = instrptr->arg1_constant;
	asAtom* value = instrptr->arg2_constant;

	LOG_CALL(_("setProperty_scc ") << *name << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));

	if(asAtomHandler::isNull(*obj))
	{
		LOG(LOG_ERROR,"calling setProperty on null:" << *name << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));
		throwError<TypeError>(kConvertNullToObjectError);
	}
	if (asAtomHandler::isUndefined(*obj))
	{
		LOG(LOG_ERROR,"calling setProperty on undefined:" << *name << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));
		throwError<TypeError>(kConvertUndefinedToObjectError);
	}
	
	ASObject* o = asAtomHandler::toObject(*obj,context->mi->context->root->getSystemState());
	multiname* simplesettername = nullptr;
	if (context->exec_pos->local_pos3 == 0x68)//initproperty
		simplesettername =o->setVariableByMultiname(*name,*value,ASObject::CONST_ALLOWED);
	else//Do not allow to set contant traits
		simplesettername =o->setVariableByMultiname(*name,*value,ASObject::CONST_NOT_ALLOWED);
	if (simplesettername)
		context->exec_pos->cachedmultiname2 = simplesettername;
	++(context->exec_pos);
}
void ABCVm::abc_setPropertyStaticName_local_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));
	multiname* name=context->exec_pos->cachedmultiname2;
	asAtom* obj = &context->locals[instrptr->local_pos1];
	asAtom* value = instrptr->arg2_constant;

	LOG_CALL(_("setProperty_slc ") << *name << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value)<<" "<<instrptr->local_pos1);

	if(asAtomHandler::isNull(*obj))
	{
		LOG(LOG_ERROR,"calling setProperty on null:" << *name << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));
		throwError<TypeError>(kConvertNullToObjectError);
	}
	if (asAtomHandler::isUndefined(*obj))
	{
		LOG(LOG_ERROR,"calling setProperty on undefined:" << *name << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));
		throwError<TypeError>(kConvertUndefinedToObjectError);
	}
	ASObject* o = asAtomHandler::toObject(*obj,context->mi->context->root->getSystemState());
	multiname* simplesettername = nullptr;
	if (context->exec_pos->local_pos3 == 0x68)//initproperty
		simplesettername =o->setVariableByMultiname(*name,*value,ASObject::CONST_ALLOWED);
	else//Do not allow to set contant traits
		simplesettername =o->setVariableByMultiname(*name,*value,ASObject::CONST_NOT_ALLOWED);
	if (simplesettername)
		context->exec_pos->cachedmultiname2 = simplesettername;
	++(context->exec_pos);
}
void ABCVm::abc_setPropertyStaticName_constant_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));
	multiname* name=context->exec_pos->cachedmultiname2;
	asAtom* obj = instrptr->arg1_constant;
	asAtom* value = &context->locals[instrptr->local_pos2];

	LOG_CALL(_("setProperty_scl ") << *name << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));

	if(asAtomHandler::isNull(*obj))
	{
		LOG(LOG_ERROR,"calling setProperty on null:" << *name << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));
		throwError<TypeError>(kConvertNullToObjectError);
	}
	if (asAtomHandler::isUndefined(*obj))
	{
		LOG(LOG_ERROR,"calling setProperty on undefined:" << *name << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));
		throwError<TypeError>(kConvertUndefinedToObjectError);
	}
	ASObject* o = asAtomHandler::toObject(*obj,context->mi->context->root->getSystemState());
	ASATOM_INCREF_POINTER(value);
	multiname* simplesettername = nullptr;
	if (context->exec_pos->local_pos3 == 0x68)//initproperty
		simplesettername =o->setVariableByMultiname(*name,*value,ASObject::CONST_ALLOWED);
	else//Do not allow to set contant traits
		simplesettername =o->setVariableByMultiname(*name,*value,ASObject::CONST_NOT_ALLOWED);
	if (simplesettername)
		context->exec_pos->cachedmultiname2 = simplesettername;
	++(context->exec_pos);
}
void ABCVm::abc_setPropertyStaticName_local_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));
	multiname* name=context->exec_pos->cachedmultiname2;
	asAtom* obj = &context->locals[instrptr->local_pos1];
	asAtom* value = &context->locals[instrptr->local_pos2];

	LOG_CALL(_("setProperty_sll ") << *name << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));

	if(asAtomHandler::isNull(*obj))
	{
		LOG(LOG_ERROR,"calling setProperty on null:" << *name << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));
		throwError<TypeError>(kConvertNullToObjectError);
	}
	if (asAtomHandler::isUndefined(*obj))
	{
		LOG(LOG_ERROR,"calling setProperty on undefined:" << *name << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));
		throwError<TypeError>(kConvertUndefinedToObjectError);
	}
	ASObject* o = asAtomHandler::toObject(*obj,context->mi->context->root->getSystemState());
	ASATOM_INCREF_POINTER(value);
	multiname* simplesettername = nullptr;
	if (context->exec_pos->local_pos3 == 0x68)//initproperty
		simplesettername =o->setVariableByMultiname(*name,*value,ASObject::CONST_ALLOWED);
	else//Do not allow to set contant traits
		simplesettername =o->setVariableByMultiname(*name,*value,ASObject::CONST_NOT_ALLOWED);
	if (simplesettername)
		context->exec_pos->cachedmultiname2 = simplesettername;
	++(context->exec_pos);
}

void ABCVm::abc_getlocal(call_context* context)
{
	//getlocal
	uint32_t i = ((context->exec_pos)++)->data>>OPCODE_SIZE;
	LOG_CALL( _("getLocal n ") << i << _(": ") << asAtomHandler::toDebugString(context->locals[i]) );
	ASATOM_INCREF(context->locals[i]);
	RUNTIME_STACK_PUSH(context,context->locals[i]);
}
void ABCVm::abc_setlocal(call_context* context)
{
	//setlocal
	uint32_t i = ((context->exec_pos)++)->data>>OPCODE_SIZE;
	RUNTIME_STACK_POP_CREATE(context,obj)

	LOG_CALL( _("setLocal n ") << i << _(": ") << asAtomHandler::toDebugString(*obj) );
	if (i > context->mi->body->local_count)
	{
		LOG(LOG_ERROR,"abc_setlocal invalid index:"<<i);
		return;
	}
	if ((int)i != context->argarrayposition || asAtomHandler::isArray(*obj))
	{
		ASATOM_DECREF(context->locals[i]);
		context->locals[i]=*obj;
	}
}
void ABCVm::abc_getglobalscope(call_context* context)
{
	//getglobalscope
	asAtom ret = asAtomHandler::fromObject(getGlobalScope(context));
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
	LOG_CALL( _("getScopeObject: ") << asAtomHandler::toDebugString(ret)<<" "<<t);

	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_getscopeobject_localresult(call_context* context)
{
	uint32_t t = context->exec_pos->data>>OPCODE_SIZE;
	assert_and_throw(context->curr_scope_stack > (uint32_t)t);
	asAtom ret=context->scope_stack[t];
	if (ret.uintval != context->locals[context->exec_pos->local_pos3-1].uintval)
	{
		ASObject* o = asAtomHandler::getObject(context->locals[context->exec_pos->local_pos3-1]);
		asAtomHandler::set(context->locals[context->exec_pos->local_pos3-1],ret);
		ASATOM_INCREF(ret);
		if (o)
			o->decRef();
	}
	LOG_CALL("getScopeObject_l " << asAtomHandler::toDebugString(context->locals[context->exec_pos->local_pos3-1])<<" "<<t);
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
			asAtom prop=asAtomHandler::invalidAtom;
			//Call the getter
			LOG_CALL("Calling the getter for " << *context->mi->context->getMultiname(t,context) << " on " << obj->toDebugString());
			assert(instrptr->cacheobj2->type == T_FUNCTION);
			IFunction* f = instrptr->cacheobj2->as<IFunction>();
			f->callGetter(prop,instrptr->cacheobj3 ? instrptr->cacheobj3 : obj);
			LOG_CALL("End of getter"<< ' ' << f->toDebugString()<<" result:"<<asAtomHandler::toDebugString(prop));
			if(asAtomHandler::isInvalid(prop))
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

	asAtom prop=asAtomHandler::invalidAtom;
	bool isgetter = obj->getVariableByMultiname(prop,*name,(name->isStatic && obj->getClass() && obj->getClass()->isSealed)? GET_VARIABLE_OPTION::DONT_CALL_GETTER:GET_VARIABLE_OPTION::NONE) & GET_VARIABLE_RESULT::GETVAR_ISGETTER;
	if (isgetter)
	{
		//Call the getter
		LOG_CALL("Calling the getter for " << *name << " on " << obj->toDebugString());
		assert(asAtomHandler::isFunction(prop));
		IFunction* f = asAtomHandler::as<IFunction>(prop);
		ASObject* closure = asAtomHandler::getClosure(prop);
		prop = asAtom();
		f->callGetter(prop,closure ? closure : obj);
		LOG_CALL("End of getter"<< ' ' << f->toDebugString()<<" result:"<<asAtomHandler::toDebugString(prop));
		if(asAtomHandler::isValid(prop))
		{
			// cache getter if multiname is static and it is a getter of a sealed class
			instrptr->data |= ABC_OP_CACHED;
			instrptr->cacheobj1 = obj->getClass();
			instrptr->cacheobj2 = f;
			instrptr->cacheobj3 = closure;
		}
	}
	if(asAtomHandler::isInvalid(prop))
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
	ASObject* obj= asAtomHandler::toObject(*instrptr->arg1_constant,context->mi->context->root->getSystemState());
	LOG_CALL( _("getProperty_cc ") << *name << ' ' << obj->toDebugString() << ' '<<obj->isInitialized());
	asAtom prop=asAtomHandler::invalidAtom;
	obj->getVariableByMultiname(prop,*name);
	if(asAtomHandler::isInvalid(prop))
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
	ASObject* obj= asAtomHandler::toObject(context->locals[instrptr->local_pos1],context->mi->context->root->getSystemState());
	LOG_CALL( _("getProperty_lc ") << *name << ' ' << obj->toDebugString() << ' '<<obj->isInitialized());
	asAtom prop=asAtomHandler::invalidAtom;
	obj->getVariableByMultiname(prop,*name);
	if(asAtomHandler::isInvalid(prop))
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
	ASObject* obj= asAtomHandler::toObject(*instrptr->arg1_constant,context->mi->context->root->getSystemState());
	LOG_CALL( _("getProperty_cl ") << *name << ' ' << obj->toDebugString() << ' '<<obj->isInitialized());
	asAtom prop=asAtomHandler::invalidAtom;
	obj->getVariableByMultiname(prop,*name);
	if(asAtomHandler::isInvalid(prop))
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
	ASObject* obj= asAtomHandler::toObject(context->locals[instrptr->local_pos1],context->mi->context->root->getSystemState());
	LOG_CALL( _("getProperty_ll ") << *name <<"("<<instrptr->local_pos2<<")"<< ' ' << obj->toDebugString() <<"("<<instrptr->local_pos1<<")"<< ' '<<obj->isInitialized());
	asAtom prop=asAtomHandler::invalidAtom;
	obj->getVariableByMultiname(prop,*name);
	if(asAtomHandler::isInvalid(prop))
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
	ASObject* obj= asAtomHandler::toObject(*instrptr->arg1_constant,context->mi->context->root->getSystemState());
	LOG_CALL( _("getProperty_ccl ") << *name << ' ' << obj->toDebugString() << ' '<<obj->isInitialized());
	asAtom prop=asAtomHandler::invalidAtom;
	obj->getVariableByMultiname(prop,*name);
	if(asAtomHandler::isInvalid(prop))
		checkPropertyException(obj,name,prop);
	name->resetNameIfObject();
	ASObject* o = asAtomHandler::getObject(context->locals[instrptr->local_pos3-1]);
	asAtomHandler::set(context->locals[instrptr->local_pos3-1],prop);
	ASATOM_INCREF(context->locals[instrptr->local_pos3-1]);
	if (o)
		o->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_getProperty_local_constant_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t t = (++(context->exec_pos))->data;
	asAtom prop=asAtomHandler::invalidAtom;
	if (asAtomHandler::isInteger(*instrptr->arg2_constant)
			&& asAtomHandler::is<Array>(context->locals[instrptr->local_pos1])
			&& asAtomHandler::getInt(*instrptr->arg2_constant) > 0 
			&& (uint32_t)asAtomHandler::getInt(*instrptr->arg2_constant) < asAtomHandler::as<Array>(context->locals[instrptr->local_pos1])->currentsize)
	{
		LOG_CALL( _("getProperty_lcl ") << asAtomHandler::getInt(*instrptr->arg2_constant) << ' ' << asAtomHandler::toDebugString(context->locals[instrptr->local_pos1]));
		asAtomHandler::as<Array>(context->locals[instrptr->local_pos1])->at_nocheck(prop,asAtomHandler::getInt(*instrptr->arg2_constant));
	}
	else
	{
		multiname* name=context->mi->context->getMultinameImpl(*instrptr->arg2_constant,NULL,t,false);
		ASObject* obj= asAtomHandler::toObject(context->locals[instrptr->local_pos1],context->mi->context->root->getSystemState());
		LOG_CALL( _("getProperty_lcl ") << *name << ' ' << obj->toDebugString() << ' '<<obj->isInitialized());
		obj->getVariableByMultiname(prop,*name,GET_VARIABLE_OPTION::NO_INCREF);
		if(asAtomHandler::isInvalid(prop))
			checkPropertyException(obj,name,prop);
		name->resetNameIfObject();
	}
	ASObject* o = asAtomHandler::getObject(context->locals[instrptr->local_pos3-1]);
	asAtomHandler::set(context->locals[instrptr->local_pos3-1],prop);
	ASATOM_INCREF(context->locals[instrptr->local_pos3-1]);
	if (o)
		o->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_getProperty_constant_local_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t t = (++(context->exec_pos))->data;
	multiname* name=context->mi->context->getMultinameImpl(context->locals[instrptr->local_pos2],NULL,t,false);
	ASObject* obj= asAtomHandler::toObject(*instrptr->arg1_constant,context->mi->context->root->getSystemState(),true);
	LOG_CALL( _("getProperty_cll ") << *name << ' ' << obj->toDebugString() << ' '<<obj->isInitialized());
	asAtom prop=asAtomHandler::invalidAtom;
	obj->getVariableByMultiname(prop,*name,GET_VARIABLE_OPTION::NO_INCREF);
	if(asAtomHandler::isInvalid(prop))
		checkPropertyException(obj,name,prop);
	name->resetNameIfObject();
	ASObject* o = asAtomHandler::getObject(context->locals[instrptr->local_pos3-1]);
	asAtomHandler::set(context->locals[instrptr->local_pos3-1],prop);
	ASATOM_INCREF(context->locals[instrptr->local_pos3-1]);
	if (o)
		o->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_getProperty_local_local_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t t = (++(context->exec_pos))->data;
	asAtom prop=asAtomHandler::invalidAtom;
	if (asAtomHandler::isInteger(context->locals[instrptr->local_pos2])
			&& asAtomHandler::is<Array>(context->locals[instrptr->local_pos1])
			&& asAtomHandler::getInt(context->locals[instrptr->local_pos2]) > 0 
			&& (uint32_t)asAtomHandler::getInt(context->locals[instrptr->local_pos2]) < asAtomHandler::as<Array>(context->locals[instrptr->local_pos1])->currentsize)
	{
		LOG_CALL( _("getProperty_lll int ") << asAtomHandler::getInt(context->locals[instrptr->local_pos2]) << ' ' << asAtomHandler::toDebugString(context->locals[instrptr->local_pos1]));
		asAtomHandler::as<Array>(context->locals[instrptr->local_pos1])->at_nocheck(prop,asAtomHandler::getInt(context->locals[instrptr->local_pos2]));
	}
	else
	{
		multiname* name=context->mi->context->getMultinameImpl(context->locals[instrptr->local_pos2],NULL,t,false);
		ASObject* obj= asAtomHandler::toObject(context->locals[instrptr->local_pos1],context->mi->context->root->getSystemState());
		LOG_CALL( _("getProperty_lll ") << *name << ' ' << obj->toDebugString() << ' '<<obj->isInitialized());
		obj->getVariableByMultiname(prop,*name,GET_VARIABLE_OPTION::NO_INCREF);
		if(asAtomHandler::isInvalid(prop))
			checkPropertyException(obj,name,prop);
		name->resetNameIfObject();
	}
	ASObject* o = asAtomHandler::getObject(context->locals[instrptr->local_pos3-1]);
	asAtomHandler::set(context->locals[instrptr->local_pos3-1],prop);
	ASATOM_INCREF(context->locals[instrptr->local_pos3-1]);
	if (o)
		o->decRef();
	++(context->exec_pos);
}

void ABCVm::abc_getPropertyStaticName_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));
	multiname* name=instrptr->cachedmultiname2;

	ASObject* obj= asAtomHandler::toObject(*instrptr->arg1_constant,context->mi->context->root->getSystemState());
	LOG_CALL( _("getProperty_sc ") << *name << ' ' << obj->toDebugString() << ' '<<obj->isInitialized());
	asAtom prop=asAtomHandler::invalidAtom;
	if(asAtomHandler::isInvalid(prop))
	{
		bool isgetter = obj->getVariableByMultiname(prop,*name,GET_VARIABLE_OPTION::DONT_CALL_GETTER) & GET_VARIABLE_RESULT::GETVAR_ISGETTER;
		if (isgetter)
		{
			//Call the getter
			LOG_CALL("Calling the getter for " << *name << " on " << obj->toDebugString());
			assert(asAtomHandler::isFunction(prop));
			IFunction* f = asAtomHandler::as<IFunction>(prop);
			ASObject* closure = asAtomHandler::getClosure(prop);
			prop = asAtom();
			multiname* simplegetter = f->callGetter(prop,closure ? closure : obj);
			if (simplegetter)
			{
				LOG_CALL("is simple getter " << *simplegetter);
				instrptr->cachedmultiname2 = simplegetter;
			}
			LOG_CALL("End of getter"<< ' ' << f->toDebugString()<<" result:"<<asAtomHandler::toDebugString(prop));
		}
	}
	if(asAtomHandler::isInvalid(prop))
		checkPropertyException(obj,name,prop);
	RUNTIME_STACK_PUSH(context,prop);
	++(context->exec_pos);
}
void ABCVm::abc_getPropertyStaticName_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));
	asAtom prop=asAtomHandler::invalidAtom;
	multiname* name=instrptr->cachedmultiname2;

	if (name->name_type == multiname::NAME_INT
			&& asAtomHandler::is<Array>(context->locals[instrptr->local_pos1])
			&& name->name_i > 0 
			&& (uint32_t)name->name_i < asAtomHandler::as<Array>(context->locals[instrptr->local_pos1])->currentsize)
	{
		LOG_CALL( _("getProperty_sl ") << name->name_i << ' ' << asAtomHandler::toDebugString(context->locals[instrptr->local_pos1]));
		asAtomHandler::as<Array>(context->locals[instrptr->local_pos1])->at_nocheck(prop,name->name_i);
	}
	else
	{
		ASObject* obj= asAtomHandler::toObject(context->locals[instrptr->local_pos1],context->mi->context->root->getSystemState());
		LOG_CALL( _("getProperty_sl ") << *name << ' ' << obj->toDebugString() << ' '<<obj->isInitialized());
		if(asAtomHandler::isInvalid(prop))
		{
			bool isgetter = obj->getVariableByMultiname(prop,*name,GET_VARIABLE_OPTION::DONT_CALL_GETTER) & GET_VARIABLE_RESULT::GETVAR_ISGETTER;
			if (isgetter)
			{
				//Call the getter
				LOG_CALL("Calling the getter for " << *name << " on " << obj->toDebugString());
				assert(asAtomHandler::isFunction(prop));
				IFunction* f = asAtomHandler::as<IFunction>(prop);
				ASObject* closure = asAtomHandler::getClosure(prop);
				prop = asAtom();
				multiname* simplegetter = f->callGetter(prop,closure ? closure : obj);
				if (simplegetter)
				{
					LOG_CALL("is simple getter " << *simplegetter);
					instrptr->cachedmultiname2 = simplegetter;
				}
				LOG_CALL("End of getter"<< ' ' << f->toDebugString()<<" result:"<<asAtomHandler::toDebugString(prop));
			}
		}
		if(asAtomHandler::isInvalid(prop))
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

	ASObject* obj= asAtomHandler::toObject(*instrptr->arg1_constant,context->mi->context->root->getSystemState(),true);
	LOG_CALL( _("getProperty_scl ") << *name << ' ' << obj->toDebugString() << ' '<<obj->isInitialized());
	asAtom prop=asAtomHandler::invalidAtom;
	if(asAtomHandler::isInvalid(prop))
	{
		bool isgetter = obj->getVariableByMultiname(prop,*name,(GET_VARIABLE_OPTION)(GET_VARIABLE_OPTION::NO_INCREF | GET_VARIABLE_OPTION::DONT_CALL_GETTER)) & GET_VARIABLE_RESULT::GETVAR_ISGETTER;
		if (isgetter)
		{
			//Call the getter
			LOG_CALL("Calling the getter for " << *name << " on " << obj->toDebugString());
			assert(asAtomHandler::isFunction(prop));
			IFunction* f = asAtomHandler::as<IFunction>(prop);
			ASObject* closure = asAtomHandler::getClosure(prop);
			prop = asAtom();
			multiname* simplegetter = f->callGetter(prop,closure ? closure : obj);
			if (simplegetter)
			{
				LOG_CALL("is simple getter " << *simplegetter);
				instrptr->cachedmultiname2 = simplegetter;
			}
			LOG_CALL("End of getter"<< ' ' << f->toDebugString()<<" result:"<<asAtomHandler::toDebugString(prop));
		}
	}
	if(asAtomHandler::isInvalid(prop))
		checkPropertyException(obj,name,prop);
	name->resetNameIfObject();
	ASObject* o = asAtomHandler::getObject(context->locals[instrptr->local_pos3-1]);
	asAtomHandler::set(context->locals[instrptr->local_pos3-1],prop);
	ASATOM_INCREF(context->locals[instrptr->local_pos3-1]);
	if (o)
		o->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_getPropertyStaticName_local_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));
	multiname* name=instrptr->cachedmultiname2;

	if ((context->exec_pos->data&ABC_OP_CACHED) == ABC_OP_CACHED)
	{
		ASObject* obj= asAtomHandler::toObject(context->locals[instrptr->local_pos1],context->mi->context->root->getSystemState());
		if (context->exec_pos->cacheobj1 == obj)
		{
			asAtom prop=asAtomHandler::invalidAtom;
			LOG_CALL( _("getProperty_sll_c ") << *name << ' ' << obj->toDebugString());

			variable* v = context->exec_pos->cachedvar2;
			if (asAtomHandler::isValid(v->getter))
			{
				asAtomHandler::set(prop,v->getter);
				//Call the getter
				LOG_CALL("Calling the getter for " << *name << " on " << obj->toDebugString());
				assert(asAtomHandler::isFunction(prop));
				IFunction* f = asAtomHandler::as<IFunction>(prop);
				ASObject* closure = asAtomHandler::getClosure(prop);
				prop = asAtom();
				f->callGetter(prop,closure ? closure : obj);
				LOG_CALL("End of getter"<< ' ' << f->toDebugString()<<" result:"<<asAtomHandler::toDebugString(prop));
			}
			else
			{
				asAtomHandler::set(prop,v->var);
			}
			
			if(asAtomHandler::isInvalid(prop))
			{
				checkPropertyException(obj,name,prop);
			}
			if(asAtomHandler::isInvalid(prop))
				checkPropertyException(obj,name,prop);
			ASObject* o = asAtomHandler::getObject(context->locals[instrptr->local_pos3-1]);
			asAtomHandler::set(context->locals[instrptr->local_pos3-1],prop);
			ASATOM_INCREF(context->locals[instrptr->local_pos3-1]);
			if (o)
				o->decRef();
			++(context->exec_pos);
			return;
		}
		else
			context->exec_pos->data = ABC_OP_NOTCACHEABLE;
	}
	
	if (name->name_type == multiname::NAME_INT
			&& asAtomHandler::is<Array>(context->locals[instrptr->local_pos1])
			&& name->name_i > 0 
			&& (uint32_t)name->name_i < asAtomHandler::as<Array>(context->locals[instrptr->local_pos1])->currentsize)
	{
		LOG_CALL( _("getProperty_sll ") << name->name_i << ' ' << asAtomHandler::toDebugString(context->locals[instrptr->local_pos1]));
		asAtomHandler::as<Array>(context->locals[instrptr->local_pos1])->at_nocheck(context->locals[instrptr->local_pos3-1],name->name_i);
	}
	else
	{
		ASObject* obj= asAtomHandler::toObject(context->locals[instrptr->local_pos1],context->mi->context->root->getSystemState());
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
			bool isgetter = obj->getVariableByMultiname(prop,*name,(GET_VARIABLE_OPTION)(GET_VARIABLE_OPTION::NO_INCREF| GET_VARIABLE_OPTION::DONT_CALL_GETTER)) & GET_VARIABLE_RESULT::GETVAR_ISGETTER;
			if (isgetter)
			{
				//Call the getter
				LOG_CALL("Calling the getter for " << *name << " on " << obj->toDebugString());
				assert(asAtomHandler::isFunction(prop));
				IFunction* f = asAtomHandler::as<IFunction>(prop);
				ASObject* closure = asAtomHandler::getClosure(prop);
				prop = asAtom();
				multiname* simplegetter = f->callGetter(prop,closure ? closure : obj);
				if (simplegetter)
				{
					LOG_CALL("is simple getter " << *simplegetter);
					instrptr->cachedmultiname2 = simplegetter;
				}
				LOG_CALL("End of getter"<< ' ' << f->toDebugString()<<" result:"<<asAtomHandler::toDebugString(prop));
			}
		}
		if(asAtomHandler::isInvalid(prop))
			checkPropertyException(obj,name,prop);
		ASObject* o = asAtomHandler::getObject(context->locals[instrptr->local_pos3-1]);
		asAtomHandler::set(context->locals[instrptr->local_pos3-1],prop);
		ASATOM_INCREF(context->locals[instrptr->local_pos3-1]);
		if (o)
			o->decRef();
	}
	++(context->exec_pos);
}
void ABCVm::abc_getPropertyStaticName_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));
	multiname* name=instrptr->cachedmultiname2;

	RUNTIME_STACK_POP_CREATE_ASOBJECT(context,obj,context->mi->context->root->getSystemState());
	LOG_CALL( _("getProperty_sll ") << *name << ' ' << obj->toDebugString() << ' '<<obj->isInitialized());
	asAtom prop=asAtomHandler::invalidAtom;
	if(asAtomHandler::isInvalid(prop))
	{
		bool isgetter = obj->getVariableByMultiname(prop,*name,(GET_VARIABLE_OPTION)(GET_VARIABLE_OPTION::NO_INCREF | GET_VARIABLE_OPTION::DONT_CALL_GETTER)) & GET_VARIABLE_RESULT::GETVAR_ISGETTER;
		if (isgetter)
		{
			//Call the getter
			LOG_CALL("Calling the getter for " << *name << " on " << obj->toDebugString());
			assert(asAtomHandler::isFunction(prop));
			IFunction* f = asAtomHandler::as<IFunction>(prop);
			ASObject* closure = asAtomHandler::getClosure(prop);
			prop = asAtom();
			multiname* simplegetter = f->callGetter(prop,closure ? closure : obj);
			if (simplegetter)
			{
				LOG_CALL("is simple getter " << *simplegetter);
				instrptr->cachedmultiname2 = simplegetter;
			}
			LOG_CALL("End of getter"<< ' ' << f->toDebugString()<<" result:"<<asAtomHandler::toDebugString(prop));
		}
	}
	if(asAtomHandler::isInvalid(prop))
		checkPropertyException(obj,name,prop);
	name->resetNameIfObject();
	ASObject* o = asAtomHandler::getObject(context->locals[instrptr->local_pos3-1]);
	asAtomHandler::set(context->locals[instrptr->local_pos3-1],prop);
	ASATOM_INCREF(context->locals[instrptr->local_pos3-1]);
	if (o)
		o->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_callFunctionNoArgs_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = *instrptr->arg1_constant;
	asAtom func = asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj2);
	asAtom ret;
	LOG_CALL("callFunctionNoArgs_c " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << " on " << asAtomHandler::toDebugString(obj));
	asAtomHandler::callFunction(func,ret,obj,nullptr,0,false,false);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_callFunctionNoArgs_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = context->locals[instrptr->local_pos1];
	asAtom func = asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj2);
	asAtom ret;
	LOG_CALL("callFunctionNoArgs_l " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << " on " << asAtomHandler::toDebugString(obj));
	asAtomHandler::callFunction(func,ret,obj,nullptr,0,false,false);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_callFunctionNoArgs_constant_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = *instrptr->arg1_constant;
	asAtom func = asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj2);
	ASObject* o = asAtomHandler::getObject(context->locals[instrptr->local_pos3-1]);
	LOG_CALL("callFunctionNoArgs_cl " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << " on " << asAtomHandler::toDebugString(obj));
	asAtomHandler::callFunction(func,context->locals[instrptr->local_pos3-1],obj,nullptr,0,false,false);
	if (o)
		o->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_callFunctionNoArgs_local_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = context->locals[instrptr->local_pos1];
	asAtom func = asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj2);
	ASObject* o = asAtomHandler::getObject(context->locals[instrptr->local_pos3-1]);
	LOG_CALL("callFunctionNoArgs_ll " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << " on " << asAtomHandler::toDebugString(obj));
	asAtomHandler::callFunction(func,context->locals[instrptr->local_pos3-1],obj,nullptr,0,false,false);
	if (o)
		o->decRef();
	++(context->exec_pos);
}

void ABCVm::abc_callFunctionNoArgsVoid_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = *instrptr->arg1_constant;
	asAtom func = asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj2);
	asAtom ret;
	LOG_CALL("callFunctionNoArgsVoid_c " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << " on " << asAtomHandler::toDebugString(obj));
	asAtomHandler::callFunction(func,ret,obj,nullptr,0,false,false,false);
	++(context->exec_pos);
}
void ABCVm::abc_callFunctionNoArgsVoid_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = context->locals[instrptr->local_pos1];
	asAtom func = asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj2);
	asAtom ret;
	LOG_CALL("callFunctionNoArgsVoid_l " << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << " on " << asAtomHandler::toDebugString(obj));
	asAtomHandler::callFunction(func,ret,obj,nullptr,0,false,false,false);
	++(context->exec_pos);
}

void ABCVm::abc_callFunctionSyntheticOneArgVoid_constant_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = *instrptr->arg1_constant;
	asAtom* value = instrptr->arg2_constant;
	asAtom func = asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj3);
	LOG_CALL(_("callFunctionSyntheticOneArgVoid_cc ") << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));
	asAtom ret;
	asAtomHandler::getObjectNoCheck(func)->as<SyntheticFunction>()->call(ret,obj, value, 1,false,(instrptr->data&ABC_OP_COERCED)==0);
	++(context->exec_pos);
}
void ABCVm::abc_callFunctionSyntheticOneArgVoid_local_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = context->locals[instrptr->local_pos1];
	asAtom* value = instrptr->arg2_constant;
	asAtom func = asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj3);
	LOG_CALL(_("callFunctionSyntheticOneArgVoid_lc ") << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));
	asAtom ret;
	asAtomHandler::getObjectNoCheck(func)->as<SyntheticFunction>()->call(ret,obj, value, 1,false,(instrptr->data&ABC_OP_COERCED)==0);
	++(context->exec_pos);
}
void ABCVm::abc_callFunctionSyntheticOneArgVoid_constant_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = *instrptr->arg1_constant;
	asAtom* value = &context->locals[instrptr->local_pos2];
	asAtom func = asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj3);
	LOG_CALL(_("callFunctionOneSyntheticArgVoid_cl ") << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));
	asAtom ret;
	ASATOM_INCREF_POINTER(value);
	asAtomHandler::getObjectNoCheck(func)->as<SyntheticFunction>()->call(ret,obj, value, 1,false,(instrptr->data&ABC_OP_COERCED)==0);
	++(context->exec_pos);
}
void ABCVm::abc_callFunctionSyntheticOneArgVoid_local_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = context->locals[instrptr->local_pos1];
	asAtom* value = &context->locals[instrptr->local_pos2];
	asAtom func = asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj3);
	LOG_CALL(_("callFunctionOneSyntheticArgVoid_ll ") << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));
	asAtom ret;
	ASATOM_INCREF_POINTER(value);
	asAtomHandler::getObjectNoCheck(func)->as<SyntheticFunction>()->call(ret,obj, value, 1,false,(instrptr->data&ABC_OP_COERCED)==0);
	++(context->exec_pos);
}
void ABCVm::abc_callFunctionBuiltinOneArgVoid_constant_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = *instrptr->arg1_constant;
	asAtom* value = instrptr->arg2_constant;
	asAtom func = asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj3);
	LOG_CALL(_("callFunctionOneBuiltinArgVoid_cc ") << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));
	asAtom ret;
	asAtomHandler::getObjectNoCheck(func)->as<Function>()->call(ret, obj, value, 1);
	++(context->exec_pos);
}
void ABCVm::abc_callFunctionBuiltinOneArgVoid_local_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = context->locals[instrptr->local_pos1];
	asAtom* value = instrptr->arg2_constant;
	asAtom func = asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj3);
	LOG_CALL(_("callFunctionOneBuiltinArgVoid_lc ") << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));
	asAtom ret;
	asAtomHandler::getObjectNoCheck(func)->as<Function>()->call(ret, obj, value, 1);
	++(context->exec_pos);
}
void ABCVm::abc_callFunctionBuiltinOneArgVoid_constant_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = *instrptr->arg1_constant;
	asAtom* value = &context->locals[instrptr->local_pos2];
	asAtom func = asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj3);
	LOG_CALL(_("callFunctionOneBuiltinArgVoid_cl ") << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));
	asAtom ret;
	asAtomHandler::getObjectNoCheck(func)->as<Function>()->call(ret, obj, value, 1);
	++(context->exec_pos);
}
void ABCVm::abc_callFunctionBuiltinOneArgVoid_local_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = context->locals[instrptr->local_pos1];
	asAtom* value = &context->locals[instrptr->local_pos2];
	asAtom func = asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj3);
	LOG_CALL(_("callFunctionBuiltinOneArgVoid_ll ") << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));
	asAtom ret;
	asAtomHandler::getObjectNoCheck(func)->as<Function>()->call(ret, obj, value, 1);
	++(context->exec_pos);
}
void ABCVm::abc_callFunctionSyntheticMultiArgsVoid_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = *instrptr->arg1_constant;
	uint32_t argcount = (instrptr->data&ABC_OP_AVAILABLEBITS) >>OPCODE_SIZE;
	asAtom func = asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj3);
	LOG_CALL(_("callFunctionSyntheticMultiArgsVoid_c ") << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" "<<argcount);
	asAtom* args = g_newa(asAtom,argcount);
	for (uint32_t i = argcount; i > 0 ; i--)
	{
		(++(context->exec_pos));
		if ((context->exec_pos->data>>OPCODE_SIZE) == OPERANDTYPES::OP_LOCAL)
		{
			args[i-1] = context->locals[context->exec_pos->local_pos1];
			ASATOM_INCREF(args[i-1]);
		}
		else
			args[i-1] = *context->exec_pos->arg1_constant;
	}
	asAtom ret;
	asAtomHandler::getObjectNoCheck(func)->as<SyntheticFunction>()->call(ret,obj, args, argcount,false,(instrptr->data&ABC_OP_COERCED)==0);
	++(context->exec_pos);
}
void ABCVm::abc_callFunctionSyntheticMultiArgsVoid_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = context->locals[instrptr->local_pos1];
	uint32_t argcount = (instrptr->data&ABC_OP_AVAILABLEBITS) >>OPCODE_SIZE;
	asAtom func = asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj3);
	LOG_CALL(_("callFunctionSyntheticMultiArgsVoid_l ") << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" "<<argcount);
	asAtom* args = g_newa(asAtom,argcount);
	for (uint32_t i = argcount; i > 0 ; i--)
	{
		(++(context->exec_pos));
		if ((context->exec_pos->data>>OPCODE_SIZE) == OPERANDTYPES::OP_LOCAL)
		{
			args[i-1] = context->locals[context->exec_pos->local_pos1];
			ASATOM_INCREF(args[i-1]);
		}
		else
			args[i-1] = *context->exec_pos->arg1_constant;
	}
	asAtom ret;
	asAtomHandler::getObjectNoCheck(func)->as<SyntheticFunction>()->call(ret,obj, args, argcount,false,(instrptr->data&ABC_OP_COERCED)==0);
	++(context->exec_pos);
}
void ABCVm::abc_callFunctionSyntheticMultiArgs_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = *instrptr->arg1_constant;
	uint32_t argcount = (instrptr->data&ABC_OP_AVAILABLEBITS) >>OPCODE_SIZE;
	asAtom func = asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj3);
	LOG_CALL(_("callFunctionSyntheticMultiArgs_c ") << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" "<<argcount);
	asAtom* args = g_newa(asAtom,argcount);
	for (uint32_t i = argcount; i > 0 ; i--)
	{
		(++(context->exec_pos));
		if ((context->exec_pos->data>>OPCODE_SIZE) == OPERANDTYPES::OP_LOCAL)
		{
			args[i-1] = context->locals[context->exec_pos->local_pos1];
			ASATOM_INCREF(args[i-1]);
		}
		else
			args[i-1] = *context->exec_pos->arg1_constant;
	}
	asAtom ret;
	asAtomHandler::getObjectNoCheck(func)->as<SyntheticFunction>()->call(ret,obj, args, argcount,false,(instrptr->data&ABC_OP_COERCED)==0);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_callFunctionSyntheticMultiArgs_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = context->locals[instrptr->local_pos1];
	uint32_t argcount = (instrptr->data&ABC_OP_AVAILABLEBITS) >>OPCODE_SIZE;
	asAtom func = asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj3);
	LOG_CALL(_("callFunctionSyntheticMultiArgs_l ") << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" "<<argcount);
	asAtom* args = g_newa(asAtom,argcount);
	for (uint32_t i = argcount; i > 0 ; i--)
	{
		(++(context->exec_pos));
		if ((context->exec_pos->data>>OPCODE_SIZE) == OPERANDTYPES::OP_LOCAL)
		{
			args[i-1] = context->locals[context->exec_pos->local_pos1];
			ASATOM_INCREF(args[i-1]);
		}
		else
			args[i-1] = *context->exec_pos->arg1_constant;
	}
	asAtom ret;
	asAtomHandler::getObjectNoCheck(func)->as<SyntheticFunction>()->call(ret,obj, args, argcount,false,(instrptr->data&ABC_OP_COERCED)==0);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_callFunctionSyntheticMultiArgs_constant_localResult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = *instrptr->arg1_constant;
	uint32_t argcount = (instrptr->data&ABC_OP_AVAILABLEBITS) >>OPCODE_SIZE;
	asAtom func = asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj3);
	LOG_CALL(_("callFunctionSyntheticMultiArgs_c_lr ") << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" "<<argcount);
	asAtom* args = g_newa(asAtom,argcount);
	for (uint32_t i = argcount; i > 0 ; i--)
	{
		(++(context->exec_pos));
		if ((context->exec_pos->data>>OPCODE_SIZE) == OPERANDTYPES::OP_LOCAL)
		{
			args[i-1] = context->locals[context->exec_pos->local_pos1];
			ASATOM_INCREF(args[i-1]);
		}
		else
			args[i-1] = *context->exec_pos->arg1_constant;
	}
	asAtom ret;
	asAtomHandler::getObjectNoCheck(func)->as<SyntheticFunction>()->call(ret,obj, args, argcount,false,(instrptr->data&ABC_OP_COERCED)==0);
	// local result pos is stored in the context->exec_pos of the last argument
	ASObject* o = asAtomHandler::getObject(context->locals[context->exec_pos->local_pos3-1]);
	asAtomHandler::set(context->locals[context->exec_pos->local_pos3-1],ret);
	if (o)
		o->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_callFunctionSyntheticMultiArgs_local_localResult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = context->locals[instrptr->local_pos1];
	uint32_t argcount = (instrptr->data&ABC_OP_AVAILABLEBITS) >>OPCODE_SIZE;
	asAtom func = asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj3);
	LOG_CALL(_("callFunctionSyntheticMultiArgs_l_lr ") << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" "<<argcount);
	asAtom* args = g_newa(asAtom,argcount);
	for (uint32_t i = argcount; i > 0 ; i--)
	{
		(++(context->exec_pos));
		if ((context->exec_pos->data>>OPCODE_SIZE) == OPERANDTYPES::OP_LOCAL)
		{
			args[i-1] = context->locals[context->exec_pos->local_pos1];
			ASATOM_INCREF(args[i-1]);
		}
		else
			args[i-1] = *context->exec_pos->arg1_constant;
	}
	asAtom ret;
	asAtomHandler::getObjectNoCheck(func)->as<SyntheticFunction>()->call(ret,obj, args, argcount,false,(instrptr->data&ABC_OP_COERCED)==0);
	// local result pos is stored in the context->exec_pos of the last argument
	ASObject* o = asAtomHandler::getObject(context->locals[context->exec_pos->local_pos3-1]);
	asAtomHandler::set(context->locals[context->exec_pos->local_pos3-1],ret);
	if (o)
		o->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_callFunctionBuiltinMultiArgsVoid_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = *instrptr->arg1_constant;
	uint32_t argcount = (instrptr->data&ABC_OP_AVAILABLEBITS) >>OPCODE_SIZE;
	asAtom func = asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj3);
	LOG_CALL(_("callFunctionBuiltinMultiArgsVoid_c ") << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" "<<argcount);
	asAtom* args = g_newa(asAtom,argcount);
	for (uint32_t i = argcount; i > 0 ; i--)
	{
		(++(context->exec_pos));
		if ((context->exec_pos->data>>OPCODE_SIZE) == OPERANDTYPES::OP_LOCAL)
			args[i-1] = context->locals[context->exec_pos->local_pos1];
		else
			args[i-1] = *context->exec_pos->arg1_constant;
	}
	asAtom ret;
	asAtomHandler::getObjectNoCheck(func)->as<Function>()->call(ret,obj, args, argcount);
	++(context->exec_pos);
}
void ABCVm::abc_callFunctionBuiltinMultiArgsVoid_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = context->locals[instrptr->local_pos1];
	uint32_t argcount = (instrptr->data&ABC_OP_AVAILABLEBITS) >>OPCODE_SIZE;
	asAtom func = asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj3);
	LOG_CALL(_("callFunctionBuiltinMultiArgsVoid_l ") << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" "<<argcount);
	asAtom* args = g_newa(asAtom,argcount);
	for (uint32_t i = argcount; i > 0 ; i--)
	{
		(++(context->exec_pos));
		if ((context->exec_pos->data>>OPCODE_SIZE) == OPERANDTYPES::OP_LOCAL)
			args[i-1] = context->locals[context->exec_pos->local_pos1];
		else
			args[i-1] = *context->exec_pos->arg1_constant;
	}
	asAtom ret;
	asAtomHandler::getObjectNoCheck(func)->as<Function>()->call(ret,obj, args, argcount);
	++(context->exec_pos);
}
void ABCVm::abc_callFunctionBuiltinMultiArgs_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = *instrptr->arg1_constant;
	uint32_t argcount = (instrptr->data&ABC_OP_AVAILABLEBITS) >>OPCODE_SIZE;
	asAtom func = asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj3);
	LOG_CALL(_("callFunctionBuiltinMultiArgs_c ") << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" "<<argcount);
	asAtom* args = g_newa(asAtom,argcount);
	for (uint32_t i = argcount; i > 0 ; i--)
	{
		(++(context->exec_pos));
		if ((context->exec_pos->data>>OPCODE_SIZE) == OPERANDTYPES::OP_LOCAL)
			args[i-1] = context->locals[context->exec_pos->local_pos1];
		else
			args[i-1] = *context->exec_pos->arg1_constant;
	}
	asAtom ret;
	asAtomHandler::getObjectNoCheck(func)->as<Function>()->call(ret,obj, args, argcount);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_callFunctionBuiltinMultiArgs_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = context->locals[instrptr->local_pos1];
	uint32_t argcount = (instrptr->data&ABC_OP_AVAILABLEBITS) >>OPCODE_SIZE;
	asAtom func = asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj3);
	LOG_CALL(_("callFunctionBuiltinMultiArgs_l ") << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" "<<argcount);
	asAtom* args = g_newa(asAtom,argcount);
	for (uint32_t i = argcount; i > 0 ; i--)
	{
		(++(context->exec_pos));
		if ((context->exec_pos->data>>OPCODE_SIZE) == OPERANDTYPES::OP_LOCAL)
			args[i-1] = context->locals[context->exec_pos->local_pos1];
		else
			args[i-1] = *context->exec_pos->arg1_constant;
	}
	asAtom ret;
	asAtomHandler::getObjectNoCheck(func)->as<Function>()->call(ret,obj, args, argcount);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_callFunctionBuiltinMultiArgs_constant_localResult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = *instrptr->arg1_constant;
	uint32_t argcount = (instrptr->data&ABC_OP_AVAILABLEBITS) >>OPCODE_SIZE;
	asAtom func = asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj3);
	LOG_CALL(_("callFunctionBuiltinMultiArgs_c_lr ") << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" "<<argcount);
	asAtom* args = g_newa(asAtom,argcount);
	for (uint32_t i = argcount; i > 0 ; i--)
	{
		(++(context->exec_pos));
		if ((context->exec_pos->data>>OPCODE_SIZE) == OPERANDTYPES::OP_LOCAL)
			args[i-1] = context->locals[context->exec_pos->local_pos1];
		else
			args[i-1] = *context->exec_pos->arg1_constant;
	}
	asAtom ret;
	asAtomHandler::getObjectNoCheck(func)->as<Function>()->call(ret,obj, args, argcount);
	// local result pos is stored in the context->exec_pos of the last argument
	ASObject* o = asAtomHandler::getObject(context->locals[context->exec_pos->local_pos3-1]);
	asAtomHandler::set(context->locals[context->exec_pos->local_pos3-1],ret);
	if (o)
		o->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_callFunctionBuiltinMultiArgs_local_localResult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom obj = context->locals[instrptr->local_pos1];
	uint32_t argcount = (instrptr->data&ABC_OP_AVAILABLEBITS) >>OPCODE_SIZE;
	asAtom func = asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj3);
	LOG_CALL(_("callFunctionBuiltinMultiArgs_l_lr ") << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname) << ' ' << asAtomHandler::toDebugString(obj)<<" "<<argcount);
	asAtom* args = g_newa(asAtom,argcount);
	for (uint32_t i = argcount; i > 0 ; i--)
	{
		(++(context->exec_pos));
		if ((context->exec_pos->data>>OPCODE_SIZE) == OPERANDTYPES::OP_LOCAL)
			args[i-1] = context->locals[context->exec_pos->local_pos1];
		else
			args[i-1] = *context->exec_pos->arg1_constant;
	}
	asAtom ret;
	asAtomHandler::getObjectNoCheck(func)->as<Function>()->call(ret,obj, args, argcount);
	// local result pos is stored in the context->exec_pos of the last argument
	ASObject* o = asAtomHandler::getObject(context->locals[context->exec_pos->local_pos3-1]);
	asAtomHandler::set(context->locals[context->exec_pos->local_pos3-1],ret);
	if (o)
		o->decRef();
	++(context->exec_pos);
}

void ABCVm::abc_callFunctionMultiArgsVoid(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t argcount = (instrptr->data&ABC_OP_AVAILABLEBITS) >>OPCODE_SIZE;
	asAtom func = asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj3);
	LOG_CALL(_("callFunctionMultiArgVoid ") << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname)<<" "<<argcount);
	RUNTIME_STACK_POP_N_CREATE(context,argcount+1,args);
	asAtom ret;
	asAtomHandler::callFunction(func,ret,*args,args+1,argcount,false,false,(instrptr->data&ABC_OP_COERCED)==0);
	++(context->exec_pos);
}
void ABCVm::abc_callFunctionMultiArgs(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t argcount = (instrptr->data&ABC_OP_AVAILABLEBITS) >>OPCODE_SIZE;
	asAtom func = asAtomHandler::fromObjectNoPrimitive(instrptr->cacheobj3);
	LOG_CALL(_("callFunctionMultiArg ") << asAtomHandler::as<IFunction>(func)->getSystemState()->getStringFromUniqueId(asAtomHandler::as<IFunction>(func)->functionname)<<" "<<argcount);
	RUNTIME_STACK_POP_N_CREATE(context,argcount+1,args);
	asAtom ret=asAtomHandler::invalidAtom;
	asAtomHandler::callFunction(func,ret,*args,args+1,argcount,false,true,(instrptr->data&ABC_OP_COERCED)==0);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}

void ABCVm::abc_initproperty(call_context* context)
{
	//initproperty
	uint32_t t = (++(context->exec_pos))->data;
	RUNTIME_STACK_POP_CREATE(context,value);
	multiname* name=context->mi->context->getMultiname(t,context);
	RUNTIME_STACK_POP_CREATE(context,obj);
	LOG_CALL("initProperty "<<*name<<" on "<< asAtomHandler::toDebugString(*obj)<<" to "<<asAtomHandler::toDebugString(*value));
	bool alreadyset=false;
	asAtomHandler::toObject(*obj,context->mi->context->root->getSystemState())->setVariableByMultiname(*name,*value,ASObject::CONST_ALLOWED,&alreadyset);
	if (alreadyset)
		ASATOM_DECREF_POINTER(value);
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
	RUNTIME_STACK_PUSH(context,asAtomHandler::fromBool(ret));
	++(context->exec_pos);
}
void ABCVm::abc_getslot(call_context* context)
{
	//getslot
	uint32_t t = (++(context->exec_pos))->data;
	RUNTIME_STACK_POINTER_CREATE(context,pval);

	asAtom ret=asAtomHandler::getObject(*pval)->getSlot(t);
	LOG_CALL("getSlot " << t << " " << asAtomHandler::toDebugString(ret));
	//getSlot can only access properties defined in the current
	//script, so they should already be defind by this script
	ASATOM_INCREF(ret);
	ASATOM_DECREF_POINTER(pval);

	*pval=ret;
	++(context->exec_pos);
}
void ABCVm::abc_getslot_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t t = (++(context->exec_pos))->data;
	asAtom* pval = instrptr->arg1_constant;
	asAtom ret=asAtomHandler::getObject(*pval)->getSlotNoCheck(t);
	LOG_CALL("getSlot_c " << t << " " << asAtomHandler::toDebugString(ret));
	ASATOM_INCREF(ret);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_getslot_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t t = (++(context->exec_pos))->data;
	ASObject* obj = asAtomHandler::getObject(context->locals[instrptr->local_pos1]);
	if (!obj)
		throwError<TypeError>(kConvertNullToObjectError);
	asAtom res = obj->getSlotNoCheck(t);
	LOG_CALL("getSlot_l " << t << " " << asAtomHandler::toDebugString(res));
	ASATOM_INCREF(res);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_getslot_constant_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t t = (++(context->exec_pos))->data;
	asAtom res = asAtomHandler::getObject(*instrptr->arg1_constant)->getSlotNoCheck(t);
	ASObject* o = asAtomHandler::getObject(context->locals[instrptr->local_pos3-1]);
	asAtomHandler::set(context->locals[instrptr->local_pos3-1],res);
	ASATOM_INCREF(context->locals[instrptr->local_pos3-1]);
	if (o)
		o->decRef();
	LOG_CALL("getSlot_cl " << t << " " << asAtomHandler::toDebugString(context->locals[instrptr->local_pos3-1]));
	++(context->exec_pos);
}
void ABCVm::abc_getslot_local_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t t = (++(context->exec_pos))->data;
	ASObject* obj = asAtomHandler::getObject(context->locals[instrptr->local_pos1]);
	if (!obj)
		throwError<TypeError>(kConvertNullToObjectError);
	asAtom res = obj->getSlotNoCheck(t);
	ASObject* o = asAtomHandler::getObject(context->locals[instrptr->local_pos3-1]);
	asAtomHandler::set(context->locals[instrptr->local_pos3-1],res);
	ASATOM_INCREF(context->locals[instrptr->local_pos3-1]);
	if (o)
		o->decRef();
	LOG_CALL("getSlot_ll " << t << " " <<instrptr->local_pos1<<":"<< asAtomHandler::toDebugString(context->locals[instrptr->local_pos1])<<" "<< asAtomHandler::toDebugString(context->locals[instrptr->local_pos3-1]));
	++(context->exec_pos);
}
void ABCVm::abc_setslot(call_context* context)
{
	uint32_t t = (++(context->exec_pos))->data;
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v2, context->mi->context->root->getSystemState());

	LOG_CALL("setSlot " << t << " "<< v2->toDebugString() << " "<< asAtomHandler::toDebugString(*v1));
	if (!v2->setSlot(t,*v1))
		ASATOM_DECREF_POINTER(v1);
	v2->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_setslot_constant_constant(call_context* context)
{
	uint32_t t = context->exec_pos->data>>OPCODE_SIZE;
	asAtom v1 = *context->exec_pos->arg1_constant;
	asAtom v2 = *context->exec_pos->arg2_constant;
	LOG_CALL("setSlot_cc " << t << " "<< asAtomHandler::toDebugString(v2) << " "<< asAtomHandler::toDebugString(v1));
	if (asAtomHandler::getObject(v1)->setSlot(t,v2))
		ASATOM_INCREF(v2);
	++(context->exec_pos);
}
void ABCVm::abc_setslot_local_constant(call_context* context)
{
	uint32_t t = context->exec_pos->data>>OPCODE_SIZE;
	asAtom v1 = context->locals[context->exec_pos->local_pos1];
	asAtom v2 = *context->exec_pos->arg2_constant;
	LOG_CALL("setSlot_lc " << t << " "<< asAtomHandler::toDebugString(v2) << " "<< asAtomHandler::toDebugString(v1));
	if (asAtomHandler::getObject(v1)->setSlot(t,v2))
		ASATOM_INCREF(v2);
	++(context->exec_pos);
}
void ABCVm::abc_setslot_constant_local(call_context* context)
{
	//setslot
	uint32_t t = context->exec_pos->data>>OPCODE_SIZE;
	asAtom v1 = *context->exec_pos->arg1_constant;
	asAtom v2 = context->locals[context->exec_pos->local_pos2];
	LOG_CALL("setSlot_cl " << t << " "<< asAtomHandler::toDebugString(v2) << " "<< asAtomHandler::toDebugString(v1));
	if (asAtomHandler::getObject(v1)->setSlot(t,v2))
		ASATOM_INCREF(v2);
	++(context->exec_pos);
}
void ABCVm::abc_setslot_local_local(call_context* context)
{
	uint32_t t = context->exec_pos->data>>OPCODE_SIZE;
	asAtom v1 = context->locals[context->exec_pos->local_pos1];
	asAtom v2 = context->locals[context->exec_pos->local_pos2];
	LOG_CALL("setSlot_ll " << t << " "<< asAtomHandler::toDebugString(v2) << " "<< asAtomHandler::toDebugString(v1));
	if (asAtomHandler::getObject(v1)->setSlot(t,v2))
		ASATOM_INCREF(v2);
	++(context->exec_pos);
}
void ABCVm::abc_setslotNoCoerce_constant_constant(call_context* context)
{
	uint32_t t = context->exec_pos->data>>OPCODE_SIZE;
	asAtom v1 = *context->exec_pos->arg1_constant;
	asAtom v2 = *context->exec_pos->arg2_constant;
	LOG_CALL("setSlotNoCoerce_cc " << t << " "<< asAtomHandler::toDebugString(v2) << " "<< asAtomHandler::toDebugString(v1));
	if (asAtomHandler::getObject(v1)->setSlotNoCoerce(t,v2))
		ASATOM_INCREF(v2);
	++(context->exec_pos);
}
void ABCVm::abc_setslotNoCoerce_local_constant(call_context* context)
{
	uint32_t t = context->exec_pos->data>>OPCODE_SIZE;
	asAtom v1 = context->locals[context->exec_pos->local_pos1];
	asAtom v2 = *context->exec_pos->arg2_constant;
	LOG_CALL("setSlotNoCoerce_lc " << t << " "<< asAtomHandler::toDebugString(v2) << " "<< asAtomHandler::toDebugString(v1));
	if (asAtomHandler::getObject(v1)->setSlotNoCoerce(t,v2))
		ASATOM_INCREF(v2);
	++(context->exec_pos);
}
void ABCVm::abc_setslotNoCoerce_constant_local(call_context* context)
{
	uint32_t t = context->exec_pos->data>>OPCODE_SIZE;
	asAtom v1 = *context->exec_pos->arg1_constant;
	asAtom v2 = context->locals[context->exec_pos->local_pos2];
	LOG_CALL("setSlotNoCoerce_cl " << t << " "<< asAtomHandler::toDebugString(v2) << " "<< asAtomHandler::toDebugString(v1));
	if (asAtomHandler::getObject(v1)->setSlotNoCoerce(t,v2))
		ASATOM_INCREF(v2);
	++(context->exec_pos);
}
void ABCVm::abc_setslotNoCoerce_local_local(call_context* context)
{
	uint32_t t = context->exec_pos->data>>OPCODE_SIZE;
	asAtom v1 = context->locals[context->exec_pos->local_pos1];
	asAtom v2 = context->locals[context->exec_pos->local_pos2];
	LOG_CALL("setSlotNoCoerce_ll " << t << " "<< asAtomHandler::toDebugString(v2) << " "<< asAtomHandler::toDebugString(v1));
	if (asAtomHandler::getObject(v1)->setSlotNoCoerce(t,v2))
		ASATOM_INCREF(v2);
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
	if(!asAtomHandler::isString(*pval))
	{
		tiny_string s = asAtomHandler::toString(*pval,context->mi->context->root->getSystemState());
		ASATOM_DECREF_POINTER(pval);
		asAtomHandler::replace(*pval,(ASObject*)abstract_s(context->mi->context->root->getSystemState(),s));
	}
	++(context->exec_pos);
}
void ABCVm::abc_esc_xelem(call_context* context)
{
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	asAtomHandler::replace(*pval,esc_xelem(asAtomHandler::toObject(*pval,context->mi->context->root->getSystemState())));
	++(context->exec_pos);
}
void ABCVm::abc_esc_xattr(call_context* context)
{
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	asAtomHandler::replace(*pval,esc_xattr(asAtomHandler::toObject(*pval,context->mi->context->root->getSystemState())));
	++(context->exec_pos);
}
void ABCVm::abc_convert_i(call_context* context)
{
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	LOG_CALL("convert_i:"<<asAtomHandler::toDebugString(*pval));
	if(!asAtomHandler::isInteger(*pval))
	{
		int32_t v= asAtomHandler::toIntStrict(*pval);
		ASATOM_DECREF_POINTER(pval);
		asAtomHandler::setInt(*pval,context->mi->context->root->getSystemState(),v);
	}
	++(context->exec_pos);
}
void ABCVm::abc_convert_i_constant(call_context* context)
{
	LOG_CALL("convert_i_c");
	asAtom res = *context->exec_pos->arg1_constant;
	if(!asAtomHandler::isInteger(res))
	{
		int32_t v= asAtomHandler::toIntStrict(res);
		asAtomHandler::setInt(res,context->mi->context->root->getSystemState(),v);
	}
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_convert_i_local(call_context* context)
{
	LOG_CALL("convert_i_l:"<<asAtomHandler::toDebugString(context->locals[context->exec_pos->local_pos1]));
	asAtom res =context->locals[context->exec_pos->local_pos1];
	if(!asAtomHandler::isInteger(res))
	{
		int32_t v= asAtomHandler::toIntStrict(res);
		asAtomHandler::setInt(res,context->mi->context->root->getSystemState(),v);
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
		asAtomHandler::setInt(res,context->mi->context->root->getSystemState(),v);
	}
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	asAtomHandler::set(context->locals[context->exec_pos->local_pos3-1],res);
	++(context->exec_pos);
}
void ABCVm::abc_convert_i_local_localresult(call_context* context)
{
	LOG_CALL("convert_i_ll");
	asAtom res = context->locals[context->exec_pos->local_pos1];
	if(!asAtomHandler::isInteger(res))
	{
		int32_t v= asAtomHandler::toIntStrict(res);
		asAtomHandler::setInt(res,context->mi->context->root->getSystemState(),v);
	}
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	asAtomHandler::set(context->locals[context->exec_pos->local_pos3-1],res);
	++(context->exec_pos);
}
void ABCVm::abc_convert_u(call_context* context)
{
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	LOG_CALL("convert_u:"<<asAtomHandler::toDebugString(*pval));
	if(!asAtomHandler::isUInteger(*pval))
	{
		uint32_t v= asAtomHandler::toUInt(*pval);
		ASATOM_DECREF_POINTER(pval);
		asAtomHandler::setUInt(*pval,context->mi->context->root->getSystemState(),v);
	}
	++(context->exec_pos);
}
void ABCVm::abc_convert_u_constant(call_context* context)
{
	LOG_CALL("convert_u_c");
	asAtom res = *context->exec_pos->arg1_constant;
	if(!asAtomHandler::isUInteger(res))
	{
		int32_t v= asAtomHandler::toUInt(res);
		asAtomHandler::setUInt(res,context->mi->context->root->getSystemState(),v);
	}
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_convert_u_local(call_context* context)
{
	LOG_CALL("convert_u_l:"<<asAtomHandler::toDebugString(context->locals[context->exec_pos->local_pos1]));
	asAtom res =context->locals[context->exec_pos->local_pos1];
	if(!asAtomHandler::isUInteger(res))
	{
		int32_t v= asAtomHandler::toUInt(res);
		asAtomHandler::setUInt(res,context->mi->context->root->getSystemState(),v);
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
		asAtomHandler::setUInt(res,context->mi->context->root->getSystemState(),v);
	}
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	asAtomHandler::set(context->locals[context->exec_pos->local_pos3-1],res);
	++(context->exec_pos);
}
void ABCVm::abc_convert_u_local_localresult(call_context* context)
{
	LOG_CALL("convert_u_ll");
	asAtom res = context->locals[context->exec_pos->local_pos1];
	if(!asAtomHandler::isUInteger(res))
	{
		int32_t v= asAtomHandler::toUInt(res);
		asAtomHandler::setUInt(res,context->mi->context->root->getSystemState(),v);
	}
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	asAtomHandler::set(context->locals[context->exec_pos->local_pos3-1],res);
	++(context->exec_pos);
}
void ABCVm::abc_convert_d(call_context* context)
{
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	LOG_CALL("convert_d:"<<asAtomHandler::toDebugString(*pval));
	if(!asAtomHandler::isNumeric(*pval))
	{
		number_t v= asAtomHandler::toNumber(*pval);
		ASATOM_DECREF_POINTER(pval);
		asAtomHandler::setNumber(*pval,context->mi->context->root->getSystemState(),v);
	}
	++(context->exec_pos);
}
void ABCVm::abc_convert_d_constant(call_context* context)
{
	LOG_CALL("convert_d_c");
	asAtom res = *context->exec_pos->arg1_constant;
	if(!asAtomHandler::isNumeric(res))
		asAtomHandler::setNumber(res,context->mi->context->root->getSystemState(),asAtomHandler::toNumber(res));
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_convert_d_local(call_context* context)
{
	LOG_CALL("convert_d_l:"<<asAtomHandler::toDebugString(context->locals[context->exec_pos->local_pos1]));
	asAtom res =context->locals[context->exec_pos->local_pos1];
	if(!asAtomHandler::isNumeric(res))
		asAtomHandler::setNumber(res,context->mi->context->root->getSystemState(),asAtomHandler::toNumber(res));
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
		asAtomHandler::setNumber(res,context->mi->context->root->getSystemState(),asAtomHandler::toNumber(res));
	else
		ASATOM_INCREF(res);
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	asAtomHandler::set(context->locals[context->exec_pos->local_pos3-1],res);
	++(context->exec_pos);
}
void ABCVm::abc_convert_d_local_localresult(call_context* context)
{
	asAtom res = context->locals[context->exec_pos->local_pos1];
	LOG_CALL("convert_d_ll "<<asAtomHandler::toDebugString(res)<<" "<<asAtomHandler::getObject(res)<<" "<< context->exec_pos->local_pos1<<" "<<(context->exec_pos->local_pos3-1));
	if(!asAtomHandler::isNumeric(res))
		asAtomHandler::setNumber(res,context->mi->context->root->getSystemState(),asAtomHandler::toNumber(res));
	else
		ASATOM_INCREF(res);
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	asAtomHandler::set(context->locals[context->exec_pos->local_pos3-1],res);
	++(context->exec_pos);
}
void ABCVm::abc_convert_b(call_context* context)
{
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	LOG_CALL("convert_b");
	asAtomHandler::convert_b(*pval,true);
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
	LOG_CALL("convert_b_l:"<<asAtomHandler::toDebugString(context->locals[context->exec_pos->local_pos1]));
	asAtom res =context->locals[context->exec_pos->local_pos1];
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
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	asAtomHandler::set(context->locals[context->exec_pos->local_pos3-1],res);
	++(context->exec_pos);
}
void ABCVm::abc_convert_b_local_localresult(call_context* context)
{
	LOG_CALL("convert_b_ll");
	asAtom res = context->locals[context->exec_pos->local_pos1];
	if(!asAtomHandler::isBool(res))
		asAtomHandler::convert_b(res,false);
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	asAtomHandler::set(context->locals[context->exec_pos->local_pos3-1],res);
	++(context->exec_pos);
}

void ABCVm::abc_convert_o(call_context* context)
{
	//convert_o
	LOG_CALL("convert_o");
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	if (asAtomHandler::isNull(*pval))
	{
		LOG(LOG_ERROR,"trying to call convert_o on null");
		throwError<TypeError>(kConvertNullToObjectError);
	}
	if (asAtomHandler::isUndefined(*pval))
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
	checkfilter(asAtomHandler::toObject(*pval,context->mi->context->root->getSystemState()));
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
	LOG_CALL("coerce_s:"<<asAtomHandler::toDebugString(*pval));
	if (!asAtomHandler::isString(*pval))
	{
		asAtom v = *pval;
		if (Class<ASString>::getClass(context->mi->context->root->getSystemState())->coerce(context->mi->context->root->getSystemState(),*pval))
			ASATOM_DECREF(v);
	}
	++(context->exec_pos);
}
void ABCVm::abc_astype(call_context* context)
{
	//astype
	uint32_t t = (++(context->exec_pos))->data;
	multiname* name=context->mi->context->getMultiname(t,NULL);

	RUNTIME_STACK_POINTER_CREATE(context,pval);
	asAtomHandler::replace(*pval,asType(context->mi->context, asAtomHandler::toObject(*pval,context->mi->context->root->getSystemState()), name));
	++(context->exec_pos);
}
void ABCVm::abc_astypelate(call_context* context)
{
	//astypelate
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	*pval = asAtomHandler::asTypelate(*pval,*v1);
	++(context->exec_pos);
}
void ABCVm::abc_negate(call_context* context)
{
	//negate
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	LOG_CALL("negate "<<asAtomHandler::toDebugString(*pval));
	if (asAtomHandler::isInteger(*pval) && asAtomHandler::toInt(*pval) != 0)
	{
		int32_t ret=-(asAtomHandler::toInt(*pval));
		ASATOM_DECREF_POINTER(pval);
		asAtomHandler::setInt(*pval,context->mi->context->root->getSystemState(), ret);
	}
	else
	{
		number_t ret=-(asAtomHandler::toNumber(*pval));
		ASATOM_DECREF_POINTER(pval);
		asAtomHandler::setNumber(*pval,context->mi->context->root->getSystemState(), ret);
	}
	++(context->exec_pos);
}
void ABCVm::abc_increment(call_context* context)
{
	//increment
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	LOG_CALL("increment "<<asAtomHandler::toDebugString(*pval));
	asAtomHandler::increment(*pval,context->mi->context->root->getSystemState());
	++(context->exec_pos);
}
void ABCVm::abc_increment_local(call_context* context)
{
	//increment
	asAtom res = context->locals[context->exec_pos->local_pos1];
	LOG_CALL("increment_l "<<context->exec_pos->local_pos1<<" "<<asAtomHandler::toDebugString(res));
	asAtomHandler::increment(res,context->mi->context->root->getSystemState());
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_increment_local_localresult(call_context* context)
{
	//increment
	LOG_CALL("increment_ll "<<context->exec_pos->local_pos1<<" "<<context->exec_pos->local_pos3<<" "<<asAtomHandler::toDebugString(context->locals[context->exec_pos->local_pos3-1]));
	asAtom res = context->locals[context->exec_pos->local_pos1];
	if (context->exec_pos->local_pos3-1 != context->exec_pos->local_pos1)
		ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	asAtomHandler::set(context->locals[context->exec_pos->local_pos3-1],res);
	asAtomHandler::increment(context->locals[context->exec_pos->local_pos3-1],context->mi->context->root->getSystemState());
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
	asAtomHandler::decrement(*pval,context->mi->context->root->getSystemState());
	++(context->exec_pos);
}
void ABCVm::abc_decrement_local(call_context* context)
{
	//decrement
	asAtom res = context->locals[context->exec_pos->local_pos1];
	asAtomHandler::decrement(res,context->mi->context->root->getSystemState());
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_decrement_local_localresult(call_context* context)
{
	//decrement
	asAtom res = context->locals[context->exec_pos->local_pos1];
	if (context->exec_pos->local_pos3-1 != context->exec_pos->local_pos1)
		ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	asAtomHandler::set(context->locals[context->exec_pos->local_pos3-1],res);
	asAtomHandler::decrement(context->locals[context->exec_pos->local_pos3-1],context->mi->context->root->getSystemState());
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
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	LOG_CALL(_("typeOf"));
	asAtom ret = asAtomHandler::typeOf(*pval);
	ASATOM_DECREF_POINTER(pval);
	*pval = ret;
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
	LOG_CALL("typeof_l "<<asAtomHandler::toDebugString(context->locals[context->exec_pos->local_pos1]));
	asAtom res = asAtomHandler::typeOf(context->locals[context->exec_pos->local_pos1]);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_typeof_constant_localresult(call_context* context)
{
	LOG_CALL("typeof_cl");
	asAtom res = asAtomHandler::typeOf(*context->exec_pos->arg1_constant);
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	asAtomHandler::set(context->locals[context->exec_pos->local_pos3-1],res);
	++(context->exec_pos);
}
void ABCVm::abc_typeof_local_localresult(call_context* context)
{
	LOG_CALL("typeof_ll");
	asAtom res = asAtomHandler::typeOf(context->locals[context->exec_pos->local_pos1]);
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	asAtomHandler::set(context->locals[context->exec_pos->local_pos3-1],res);
	++(context->exec_pos);
}
void ABCVm::abc_not(call_context* context)
{
	//not
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	asAtomHandler::_not(*pval);
	++(context->exec_pos);
}
void ABCVm::abc_not_constant(call_context* context)
{
	LOG_CALL("not_c");
	asAtom res = asAtomHandler::fromBool(!asAtomHandler::Boolean_concrete(*context->exec_pos->arg1_constant));
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_not_local(call_context* context)
{
	LOG_CALL("not_l "<<asAtomHandler::toDebugString(context->locals[context->exec_pos->local_pos1]));
	asAtom res = asAtomHandler::fromBool(!asAtomHandler::Boolean_concrete(context->locals[context->exec_pos->local_pos1]));
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_not_constant_localresult(call_context* context)
{
	LOG_CALL("not_cl");
	bool res = !asAtomHandler::Boolean_concrete(*context->exec_pos->arg1_constant);
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	asAtomHandler::setBool(context->locals[context->exec_pos->local_pos3-1],res);
	++(context->exec_pos);
}
void ABCVm::abc_not_local_localresult(call_context* context)
{
	LOG_CALL("not_ll");
	bool res = !asAtomHandler::Boolean_concrete(context->locals[context->exec_pos->local_pos1]);
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	asAtomHandler::setBool(context->locals[context->exec_pos->local_pos3-1],res);
	++(context->exec_pos);
}

void ABCVm::abc_bitnot(call_context* context)
{
	//bitnot
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	asAtomHandler::bitnot(*pval,context->mi->context->root->getSystemState());
	
	++(context->exec_pos);
}
void ABCVm::abc_add(call_context* context)
{
	LOG_CALL("add");
	RUNTIME_STACK_POP_CREATE(context,v2);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	ASObject* o = asAtomHandler::getObject(*pval);
	if (asAtomHandler::add(*pval,*v2,context->mi->context->root->getSystemState()))
	{
		ASATOM_DECREF_POINTER(v2);
		if (o) 
			o->decRef();
	}
	++(context->exec_pos);
}
void ABCVm::abc_add_constant_constant(call_context* context)
{
	LOG_CALL("add_cc");
	asAtom res = *context->exec_pos->arg1_constant;
	asAtomHandler::add(res,*context->exec_pos->arg2_constant,context->mi->context->root->getSystemState());
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_add_local_constant(call_context* context)
{
	LOG_CALL("add_lc");
	asAtom res = context->locals[context->exec_pos->local_pos1];
	asAtomHandler::add(res,*context->exec_pos->arg2_constant,context->mi->context->root->getSystemState());
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_add_constant_local(call_context* context)
{
	LOG_CALL("add_cl");
	asAtom res = *context->exec_pos->arg1_constant;
	asAtomHandler::add(res,context->locals[context->exec_pos->local_pos2],context->mi->context->root->getSystemState());
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_add_local_local(call_context* context)
{
	LOG_CALL("add_ll");
	asAtom res = context->locals[context->exec_pos->local_pos1];
	asAtomHandler::add(res,context->locals[context->exec_pos->local_pos2],context->mi->context->root->getSystemState());
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_add_constant_constant_localresult(call_context* context)
{
	LOG_CALL("add_ccl");
	asAtomHandler::addreplace(context->locals[context->exec_pos->local_pos3-1],context->mi->context->root->getSystemState(),*context->exec_pos->arg1_constant,*context->exec_pos->arg2_constant);
	++(context->exec_pos);
}
void ABCVm::abc_add_local_constant_localresult(call_context* context)
{
	LOG_CALL("add_lcl");
	asAtomHandler::addreplace(context->locals[context->exec_pos->local_pos3-1],context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos1],*context->exec_pos->arg2_constant);
	++(context->exec_pos);
}
void ABCVm::abc_add_constant_local_localresult(call_context* context)
{
	LOG_CALL("add_cll");
	asAtomHandler::addreplace(context->locals[context->exec_pos->local_pos3-1],context->mi->context->root->getSystemState(),*context->exec_pos->arg1_constant,context->locals[context->exec_pos->local_pos2]);
	++(context->exec_pos);
}
void ABCVm::abc_add_local_local_localresult(call_context* context)
{
	LOG_CALL("add_lll");
	asAtomHandler::addreplace(context->locals[context->exec_pos->local_pos3-1],context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos1],context->locals[context->exec_pos->local_pos2]);
	++(context->exec_pos);
}

void ABCVm::abc_subtract(call_context* context)
{
	//subtract
	//Be careful, operands in subtract implementation are swapped
	RUNTIME_STACK_POP_CREATE(context,v2);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	ASObject* o = asAtomHandler::getObject(*pval);
	asAtomHandler::subtract(*pval,context->mi->context->root->getSystemState(),*v2);
	ASATOM_DECREF_POINTER(v2);
	if (o) 
		o->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_subtract_constant_constant(call_context* context)
{
	//subtract
	asAtom res = *context->exec_pos->arg1_constant;
	asAtomHandler::subtract(res,context->mi->context->root->getSystemState(),*context->exec_pos->arg2_constant);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_subtract_local_constant(call_context* context)
{
	//subtract
	asAtom res = context->locals[context->exec_pos->local_pos1];
	asAtomHandler::subtract(res,context->mi->context->root->getSystemState(),*context->exec_pos->arg2_constant);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_subtract_constant_local(call_context* context)
{
	//subtract
	asAtom res = *context->exec_pos->arg1_constant;
	asAtomHandler::subtract(res,context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos2]);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_subtract_local_local(call_context* context)
{
	//subtract
	asAtom res = context->locals[context->exec_pos->local_pos1];
	asAtomHandler::subtract(res,context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos2]);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_subtract_constant_constant_localresult(call_context* context)
{
	asAtomHandler::subtractreplace(context->locals[context->exec_pos->local_pos3-1],context->mi->context->root->getSystemState(),*context->exec_pos->arg1_constant,*context->exec_pos->arg2_constant);
	++(context->exec_pos);
}
void ABCVm::abc_subtract_local_constant_localresult(call_context* context)
{
	asAtomHandler::subtractreplace(context->locals[context->exec_pos->local_pos3-1],context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos1],*context->exec_pos->arg2_constant);
	++(context->exec_pos);
}
void ABCVm::abc_subtract_constant_local_localresult(call_context* context)
{
	asAtomHandler::subtractreplace(context->locals[context->exec_pos->local_pos3-1],context->mi->context->root->getSystemState(),*context->exec_pos->arg1_constant,context->locals[context->exec_pos->local_pos2]);
	++(context->exec_pos);
}
void ABCVm::abc_subtract_local_local_localresult(call_context* context)
{
	asAtomHandler::subtractreplace(context->locals[context->exec_pos->local_pos3-1],context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos1],context->locals[context->exec_pos->local_pos2]);
	++(context->exec_pos);
}

void ABCVm::abc_multiply(call_context* context)
{
	//multiply
	RUNTIME_STACK_POP_CREATE(context,v2);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	ASObject* o = asAtomHandler::getObject(*pval);
	asAtomHandler::multiply(*pval,context->mi->context->root->getSystemState(),*v2);
	ASATOM_DECREF_POINTER(v2);
	if (o) 
		o->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_multiply_constant_constant(call_context* context)
{
	//multiply
	asAtom res = *context->exec_pos->arg1_constant;
	asAtomHandler::multiply(res,context->mi->context->root->getSystemState(),*context->exec_pos->arg2_constant);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_multiply_local_constant(call_context* context)
{
	//multiply
	asAtom res = context->locals[context->exec_pos->local_pos1];
	asAtomHandler::multiply(res,context->mi->context->root->getSystemState(),*context->exec_pos->arg2_constant);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_multiply_constant_local(call_context* context)
{
	//multiply
	asAtom res = *context->exec_pos->arg1_constant;
	asAtomHandler::multiply(res,context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos2]);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_multiply_local_local(call_context* context)
{
	//multiply
	asAtom res = context->locals[context->exec_pos->local_pos1];
	asAtomHandler::multiply(res,context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos2]);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_multiply_constant_constant_localresult(call_context* context)
{
	asAtomHandler::multiplyreplace(context->locals[context->exec_pos->local_pos3-1],context->mi->context->root->getSystemState(),*context->exec_pos->arg1_constant,*context->exec_pos->arg2_constant);
	++(context->exec_pos);
}
void ABCVm::abc_multiply_local_constant_localresult(call_context* context)
{
	asAtomHandler::multiplyreplace(context->locals[context->exec_pos->local_pos3-1],context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos1],*context->exec_pos->arg2_constant);
	++(context->exec_pos);
}
void ABCVm::abc_multiply_constant_local_localresult(call_context* context)
{
	asAtomHandler::multiplyreplace(context->locals[context->exec_pos->local_pos3-1],context->mi->context->root->getSystemState(),*context->exec_pos->arg1_constant,context->locals[context->exec_pos->local_pos2]);
	++(context->exec_pos);
}
void ABCVm::abc_multiply_local_local_localresult(call_context* context)
{
	asAtomHandler::multiplyreplace(context->locals[context->exec_pos->local_pos3-1],context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos1],context->locals[context->exec_pos->local_pos2]);
	++(context->exec_pos);
}

void ABCVm::abc_divide(call_context* context)
{
	//divide
	RUNTIME_STACK_POP_CREATE(context,v2);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	ASObject* o = asAtomHandler::getObject(*pval);
	asAtomHandler::divide(*pval,context->mi->context->root->getSystemState(),*v2);
	ASATOM_DECREF_POINTER(v2);
	if (o) 
		o->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_divide_constant_constant(call_context* context)
{
	//divide
	asAtom res = *context->exec_pos->arg1_constant;
	asAtomHandler::divide(res,context->mi->context->root->getSystemState(),*context->exec_pos->arg2_constant);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_divide_local_constant(call_context* context)
{
	//divide
	asAtom res = context->locals[context->exec_pos->local_pos1];
	asAtomHandler::divide(res,context->mi->context->root->getSystemState(),*context->exec_pos->arg2_constant);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_divide_constant_local(call_context* context)
{
	//divide
	asAtom res = *context->exec_pos->arg1_constant;
	asAtomHandler::divide(res,context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos2]);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_divide_local_local(call_context* context)
{
	//divide
	asAtom res = context->locals[context->exec_pos->local_pos1];
	asAtomHandler::divide(res,context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos2]);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_divide_constant_constant_localresult(call_context* context)
{
	asAtomHandler::dividereplace(context->locals[context->exec_pos->local_pos3-1],context->mi->context->root->getSystemState(),*context->exec_pos->arg1_constant,*context->exec_pos->arg2_constant);
	++(context->exec_pos);
}
void ABCVm::abc_divide_local_constant_localresult(call_context* context)
{
	asAtomHandler::dividereplace(context->locals[context->exec_pos->local_pos3-1],context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos1],*context->exec_pos->arg2_constant);
	++(context->exec_pos);
}
void ABCVm::abc_divide_constant_local_localresult(call_context* context)
{
	asAtomHandler::dividereplace(context->locals[context->exec_pos->local_pos3-1],context->mi->context->root->getSystemState(),*context->exec_pos->arg1_constant,context->locals[context->exec_pos->local_pos2]);
	++(context->exec_pos);
}
void ABCVm::abc_divide_local_local_localresult(call_context* context)
{
	asAtomHandler::dividereplace(context->locals[context->exec_pos->local_pos3-1],context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos1],context->locals[context->exec_pos->local_pos2]);
	++(context->exec_pos);
}
void ABCVm::abc_modulo(call_context* context)
{
	//modulo
	RUNTIME_STACK_POP_CREATE(context,v2);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	ASObject* o = asAtomHandler::getObject(*pval);
	asAtomHandler::modulo(*pval,context->mi->context->root->getSystemState(),*v2);
	ASATOM_DECREF_POINTER(v2);
	if (o) 
		o->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_modulo_constant_constant(call_context* context)
{
	//modulo
	asAtom res = *context->exec_pos->arg1_constant;
	asAtomHandler::modulo(res,context->mi->context->root->getSystemState(),*context->exec_pos->arg2_constant);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_modulo_local_constant(call_context* context)
{
	//modulo
	asAtom res = context->locals[context->exec_pos->local_pos1];
	asAtomHandler::modulo(res,context->mi->context->root->getSystemState(),*context->exec_pos->arg2_constant);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_modulo_constant_local(call_context* context)
{
	//modulo
	asAtom res = *context->exec_pos->arg1_constant;
	asAtomHandler::modulo(res,context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos2]);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_modulo_local_local(call_context* context)
{
	//modulo
	asAtom res = context->locals[context->exec_pos->local_pos1];
	asAtomHandler::modulo(res,context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos2]);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_modulo_constant_constant_localresult(call_context* context)
{
	LOG_CALL("modulo_ccl");
	asAtomHandler::moduloreplace(context->locals[context->exec_pos->local_pos3-1],context->mi->context->root->getSystemState(),*context->exec_pos->arg1_constant,*context->exec_pos->arg2_constant);
	++(context->exec_pos);
}
void ABCVm::abc_modulo_local_constant_localresult(call_context* context)
{
	LOG_CALL("modulo_lcl");
	asAtomHandler::moduloreplace(context->locals[context->exec_pos->local_pos3-1],context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos1],*context->exec_pos->arg2_constant);
	++(context->exec_pos);
}
void ABCVm::abc_modulo_constant_local_localresult(call_context* context)
{
	LOG_CALL("modulo_cll");
	asAtomHandler::moduloreplace(context->locals[context->exec_pos->local_pos3-1],context->mi->context->root->getSystemState(),*context->exec_pos->arg1_constant,context->locals[context->exec_pos->local_pos2]);
	++(context->exec_pos);
}
void ABCVm::abc_modulo_local_local_localresult(call_context* context)
{
	LOG_CALL("modulo_lll");
	asAtomHandler::moduloreplace(context->locals[context->exec_pos->local_pos3-1],context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos1],context->locals[context->exec_pos->local_pos2]);
	++(context->exec_pos);
}
void ABCVm::abc_lshift(call_context* context)
{
	//lshift
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	ASObject* o = asAtomHandler::getObject(*pval);
	asAtomHandler::lshift(*pval,context->mi->context->root->getSystemState(),*v1);
	ASATOM_DECREF_POINTER(v1);
	if (o) 
		o->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_lshift_constant_constant(call_context* context)
{
	//lshift
	asAtom res = *context->exec_pos->arg1_constant;
	asAtomHandler::lshift(res,context->mi->context->root->getSystemState(),*context->exec_pos->arg2_constant);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_lshift_local_constant(call_context* context)
{
	//lshift
	asAtom res = context->locals[context->exec_pos->local_pos1];
	asAtomHandler::lshift(res,context->mi->context->root->getSystemState(),*context->exec_pos->arg2_constant);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_lshift_constant_local(call_context* context)
{
	//lshift
	asAtom res = *context->exec_pos->arg1_constant;
	asAtomHandler::lshift(res,context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos2]);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_lshift_local_local(call_context* context)
{
	//lshift
	asAtom res = context->locals[context->exec_pos->local_pos1];
	asAtomHandler::lshift(res,context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos2]);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_lshift_constant_constant_localresult(call_context* context)
{
	//lshift
	asAtom res = *context->exec_pos->arg1_constant;
	asAtomHandler::lshift(res,context->mi->context->root->getSystemState(),*context->exec_pos->arg2_constant);
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	asAtomHandler::set(context->locals[context->exec_pos->local_pos3-1],res);
	++(context->exec_pos);
}
void ABCVm::abc_lshift_local_constant_localresult(call_context* context)
{
	//lshift
	asAtom res = context->locals[context->exec_pos->local_pos1];
	asAtomHandler::lshift(res,context->mi->context->root->getSystemState(),*context->exec_pos->arg2_constant);
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	asAtomHandler::set(context->locals[context->exec_pos->local_pos3-1],res);
	++(context->exec_pos);
}
void ABCVm::abc_lshift_constant_local_localresult(call_context* context)
{
	//lshift
	asAtom res = *context->exec_pos->arg1_constant;
	asAtomHandler::lshift(res,context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos2]);
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	asAtomHandler::set(context->locals[context->exec_pos->local_pos3-1],res);
	++(context->exec_pos);
}
void ABCVm::abc_lshift_local_local_localresult(call_context* context)
{
	//lshift
	asAtom res = context->locals[context->exec_pos->local_pos1];
	asAtomHandler::lshift(res,context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos2]);
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	asAtomHandler::set(context->locals[context->exec_pos->local_pos3-1],res);
	++(context->exec_pos);
}
void ABCVm::abc_rshift(call_context* context)
{
	//rshift
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	ASObject* o = asAtomHandler::getObject(*pval);
	asAtomHandler::rshift(*pval,context->mi->context->root->getSystemState(),*v1);
	ASATOM_DECREF_POINTER(v1);
	if (o) 
		o->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_rshift_constant_constant(call_context* context)
{
	//rshift
	asAtom res = *context->exec_pos->arg1_constant;
	asAtomHandler::rshift(res,context->mi->context->root->getSystemState(),*context->exec_pos->arg2_constant);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_rshift_local_constant(call_context* context)
{
	//rshift
	asAtom res = context->locals[context->exec_pos->local_pos1];
	asAtomHandler::rshift(res,context->mi->context->root->getSystemState(),*context->exec_pos->arg2_constant);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_rshift_constant_local(call_context* context)
{
	//rshift
	asAtom res = *context->exec_pos->arg1_constant;
	asAtomHandler::rshift(res,context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos2]);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_rshift_local_local(call_context* context)
{
	//rshift
	asAtom res = context->locals[context->exec_pos->local_pos1];
	asAtomHandler::rshift(res,context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos2]);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_rshift_constant_constant_localresult(call_context* context)
{
	//rshift
	asAtom res = *context->exec_pos->arg1_constant;
	asAtomHandler::rshift(res,context->mi->context->root->getSystemState(),*context->exec_pos->arg2_constant);
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	asAtomHandler::set(context->locals[context->exec_pos->local_pos3-1],res);
	++(context->exec_pos);
}
void ABCVm::abc_rshift_local_constant_localresult(call_context* context)
{
	//rshift
	asAtom res = context->locals[context->exec_pos->local_pos1];
	asAtomHandler::rshift(res,context->mi->context->root->getSystemState(),*context->exec_pos->arg2_constant);
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	asAtomHandler::set(context->locals[context->exec_pos->local_pos3-1],res);
	++(context->exec_pos);
}
void ABCVm::abc_rshift_constant_local_localresult(call_context* context)
{
	//rshift
	asAtom res = *context->exec_pos->arg1_constant;
	asAtomHandler::rshift(res,context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos2]);
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	asAtomHandler::set(context->locals[context->exec_pos->local_pos3-1],res);
	++(context->exec_pos);
}
void ABCVm::abc_rshift_local_local_localresult(call_context* context)
{
	//rshift
	asAtom res = context->locals[context->exec_pos->local_pos1];
	asAtomHandler::rshift(res,context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos2]);
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	asAtomHandler::set(context->locals[context->exec_pos->local_pos3-1],res);
	++(context->exec_pos);
}
void ABCVm::abc_urshift(call_context* context)
{
	//urshift
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	ASObject* o = asAtomHandler::getObject(*pval);
	asAtomHandler::urshift(*pval,context->mi->context->root->getSystemState(),*v1);
	ASATOM_DECREF_POINTER(v1);
	if (o) 
		o->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_urshift_constant_constant(call_context* context)
{
	//urshift
	asAtom res = *context->exec_pos->arg1_constant;
	asAtomHandler::urshift(res,context->mi->context->root->getSystemState(),*context->exec_pos->arg2_constant);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_urshift_local_constant(call_context* context)
{
	//urshift
	asAtom res = context->locals[context->exec_pos->local_pos1];
	asAtomHandler::urshift(res,context->mi->context->root->getSystemState(),*context->exec_pos->arg2_constant);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_urshift_constant_local(call_context* context)
{
	//urshift
	asAtom res = *context->exec_pos->arg1_constant;
	asAtomHandler::urshift(res,context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos2]);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_urshift_local_local(call_context* context)
{
	//urshift
	asAtom res = context->locals[context->exec_pos->local_pos1];
	asAtomHandler::urshift(res,context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos2]);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_urshift_constant_constant_localresult(call_context* context)
{
	//urshift
	asAtom res = *context->exec_pos->arg1_constant;
	asAtomHandler::urshift(res,context->mi->context->root->getSystemState(),*context->exec_pos->arg2_constant);
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	asAtomHandler::set(context->locals[context->exec_pos->local_pos3-1],res);
	++(context->exec_pos);
}
void ABCVm::abc_urshift_local_constant_localresult(call_context* context)
{
	//urshift
	asAtom res = context->locals[context->exec_pos->local_pos1];
	asAtomHandler::urshift(res,context->mi->context->root->getSystemState(),*context->exec_pos->arg2_constant);
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	asAtomHandler::set(context->locals[context->exec_pos->local_pos3-1],res);
	++(context->exec_pos);
}
void ABCVm::abc_urshift_constant_local_localresult(call_context* context)
{
	//urshift
	asAtom res = *context->exec_pos->arg1_constant;
	asAtomHandler::urshift(res,context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos2]);
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	asAtomHandler::set(context->locals[context->exec_pos->local_pos3-1],res);
	++(context->exec_pos);
}
void ABCVm::abc_urshift_local_local_localresult(call_context* context)
{
	//urshift
	asAtom res = context->locals[context->exec_pos->local_pos1];
	asAtomHandler::urshift(res,context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos2]);
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	asAtomHandler::set(context->locals[context->exec_pos->local_pos3-1],res);
	++(context->exec_pos);
}
void ABCVm::abc_bitand(call_context* context)
{
	//bitand
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	ASObject* o = asAtomHandler::getObject(*pval);
	asAtomHandler::bit_and(*pval,context->mi->context->root->getSystemState(),*v1);
	ASATOM_DECREF_POINTER(v1);
	if (o) 
		o->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_bitand_constant_constant(call_context* context)
{
	//bitand
	asAtom res = *context->exec_pos->arg1_constant;
	asAtomHandler::bit_and(res,context->mi->context->root->getSystemState(),*context->exec_pos->arg2_constant);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_bitand_local_constant(call_context* context)
{
	//bitand
	asAtom res = context->locals[context->exec_pos->local_pos1];
	asAtomHandler::bit_and(res,context->mi->context->root->getSystemState(),*context->exec_pos->arg2_constant);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_bitand_constant_local(call_context* context)
{
	//bitand
	asAtom res = *context->exec_pos->arg1_constant;
	asAtomHandler::bit_and(res,context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos2]);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_bitand_local_local(call_context* context)
{
	//bitand
	asAtom res = context->locals[context->exec_pos->local_pos1];
	asAtomHandler::bit_and(res,context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos2]);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_bitand_constant_constant_localresult(call_context* context)
{
	//bitand
	asAtom res = *context->exec_pos->arg1_constant;
	asAtomHandler::bit_and(res,context->mi->context->root->getSystemState(),*context->exec_pos->arg2_constant);
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	asAtomHandler::set(context->locals[context->exec_pos->local_pos3-1],res);
	++(context->exec_pos);
}
void ABCVm::abc_bitand_local_constant_localresult(call_context* context)
{
	//bitand
	asAtom res = context->locals[context->exec_pos->local_pos1];
	asAtomHandler::bit_and(res,context->mi->context->root->getSystemState(),*context->exec_pos->arg2_constant);
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	asAtomHandler::set(context->locals[context->exec_pos->local_pos3-1],res);
	++(context->exec_pos);
}
void ABCVm::abc_bitand_constant_local_localresult(call_context* context)
{
	//bitand
	asAtom res = *context->exec_pos->arg1_constant;
	asAtomHandler::bit_and(res,context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos2]);
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	asAtomHandler::set(context->locals[context->exec_pos->local_pos3-1],res);
	++(context->exec_pos);
}
void ABCVm::abc_bitand_local_local_localresult(call_context* context)
{
	//bitand
	asAtom res = context->locals[context->exec_pos->local_pos1];
	asAtomHandler::bit_and(res,context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos2]);
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	asAtomHandler::set(context->locals[context->exec_pos->local_pos3-1],res);
	++(context->exec_pos);
}
void ABCVm::abc_bitor(call_context* context)
{
	//bitor
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	ASObject* o = asAtomHandler::getObject(*pval);
	asAtomHandler::bit_or(*pval,context->mi->context->root->getSystemState(),*v1);
	ASATOM_DECREF_POINTER(v1);
	if (o) 
		o->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_bitor_constant_constant(call_context* context)
{
	//bitor
	asAtom res = *context->exec_pos->arg1_constant;
	asAtomHandler::bit_or(res,context->mi->context->root->getSystemState(),*context->exec_pos->arg2_constant);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_bitor_local_constant(call_context* context)
{
	//bitor
	asAtom res = context->locals[context->exec_pos->local_pos1];
	asAtomHandler::bit_or(res,context->mi->context->root->getSystemState(),*context->exec_pos->arg2_constant);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_bitor_constant_local(call_context* context)
{
	//bitor
	asAtom res = *context->exec_pos->arg1_constant;
	asAtomHandler::bit_or(res,context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos2]);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_bitor_local_local(call_context* context)
{
	//bitor
	asAtom res = context->locals[context->exec_pos->local_pos1];
	asAtomHandler::bit_or(res,context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos2]);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_bitor_constant_constant_localresult(call_context* context)
{
	//bitor
	asAtom res = *context->exec_pos->arg1_constant;
	asAtomHandler::bit_or(res,context->mi->context->root->getSystemState(),*context->exec_pos->arg2_constant);
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	asAtomHandler::set(context->locals[context->exec_pos->local_pos3-1],res);
	++(context->exec_pos);
}
void ABCVm::abc_bitor_local_constant_localresult(call_context* context)
{
	//bitor
	asAtom res = *context->exec_pos->arg2_constant;
	asAtomHandler::bit_or(res,context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos1]);
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	asAtomHandler::set(context->locals[context->exec_pos->local_pos3-1],res);
	++(context->exec_pos);
}
void ABCVm::abc_bitor_constant_local_localresult(call_context* context)
{
	//bitor
	asAtom res = *context->exec_pos->arg1_constant;
	asAtomHandler::bit_or(res,context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos2]);
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	asAtomHandler::set(context->locals[context->exec_pos->local_pos3-1],res);
	++(context->exec_pos);
}
void ABCVm::abc_bitor_local_local_localresult(call_context* context)
{
	//bitor
	asAtom res = context->locals[context->exec_pos->local_pos1];
	asAtomHandler::bit_or(res,context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos2]);
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	asAtomHandler::set(context->locals[context->exec_pos->local_pos3-1],res);
	++(context->exec_pos);
}

void ABCVm::abc_bitxor(call_context* context)
{
	//bitxor
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	ASObject* o = asAtomHandler::getObject(*pval);
	asAtomHandler::bit_xor(*pval,context->mi->context->root->getSystemState(),*v1);
	ASATOM_DECREF_POINTER(v1);
	if (o) 
		o->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_bitxor_constant_constant(call_context* context)
{
	//bitxor
	asAtom res = *context->exec_pos->arg1_constant;
	asAtomHandler::bit_xor(res,context->mi->context->root->getSystemState(),*context->exec_pos->arg2_constant);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_bitxor_local_constant(call_context* context)
{
	//bitxor
	asAtom res = context->locals[context->exec_pos->local_pos1];
	asAtomHandler::bit_xor(res,context->mi->context->root->getSystemState(),*context->exec_pos->arg2_constant);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_bitxor_constant_local(call_context* context)
{
	//bitxor
	asAtom res = *context->exec_pos->arg1_constant;
	asAtomHandler::bit_xor(res,context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos2]);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_bitxor_local_local(call_context* context)
{
	//bitxor
	asAtom res = context->locals[context->exec_pos->local_pos1];
	asAtomHandler::bit_xor(res,context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos2]);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_bitxor_constant_constant_localresult(call_context* context)
{
	//bitxor
	asAtom res = *context->exec_pos->arg1_constant;
	asAtomHandler::bit_xor(res,context->mi->context->root->getSystemState(),*context->exec_pos->arg2_constant);
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	asAtomHandler::set(context->locals[context->exec_pos->local_pos3-1],res);
	++(context->exec_pos);
}
void ABCVm::abc_bitxor_local_constant_localresult(call_context* context)
{
	//bitxor
	asAtom res = context->locals[context->exec_pos->local_pos1];
	asAtomHandler::bit_xor(res,context->mi->context->root->getSystemState(),*context->exec_pos->arg2_constant);
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	asAtomHandler::set(context->locals[context->exec_pos->local_pos3-1],res);
	++(context->exec_pos);
}
void ABCVm::abc_bitxor_constant_local_localresult(call_context* context)
{
	//bitxor
	asAtom res = *context->exec_pos->arg1_constant;
	asAtomHandler::bit_xor(res,context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos2]);
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	asAtomHandler::set(context->locals[context->exec_pos->local_pos3-1],res);
	++(context->exec_pos);
}
void ABCVm::abc_bitxor_local_local_localresult(call_context* context)
{
	//bitxor
	asAtom res = context->locals[context->exec_pos->local_pos1];
	asAtomHandler::bit_xor(res,context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos2]);
	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	asAtomHandler::set(context->locals[context->exec_pos->local_pos3-1],res);
	++(context->exec_pos);
}
void ABCVm::abc_equals(call_context* context)
{
	//equals
	RUNTIME_STACK_POP_CREATE(context,v2);
	RUNTIME_STACK_POINTER_CREATE(context,pval);

	bool ret=asAtomHandler::isEqual(*pval,context->mi->context->root->getSystemState(),*v2);
	LOG_CALL( _("equals ") << ret);
	ASATOM_DECREF_POINTER(pval);
	ASATOM_DECREF_POINTER(v2);
	asAtomHandler::setBool(*pval,ret);
	++(context->exec_pos);
}
void ABCVm::abc_equals_constant_constant(call_context* context)
{
	//equals

	bool ret=(asAtomHandler::isEqual(*context->exec_pos->arg2_constant,context->mi->context->root->getSystemState(),*context->exec_pos->arg1_constant));
	LOG_CALL(_("equals_cc ")<<ret);

	RUNTIME_STACK_PUSH(context,asAtomHandler::fromBool(ret));
	++(context->exec_pos);
}
void ABCVm::abc_equals_local_constant(call_context* context)
{
	//equals

	bool ret=(asAtomHandler::isEqual(*context->exec_pos->arg2_constant,context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos1]));
	LOG_CALL(_("equals_lc ")<<ret);

	RUNTIME_STACK_PUSH(context,asAtomHandler::fromBool(ret));
	++(context->exec_pos);
}
void ABCVm::abc_equals_constant_local(call_context* context)
{
	//equals

	bool ret=(asAtomHandler::isEqual(*context->exec_pos->arg1_constant,context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos2]));
	LOG_CALL(_("equals_cl ")<<ret);

	RUNTIME_STACK_PUSH(context,asAtomHandler::fromBool(ret));
	++(context->exec_pos);
}
void ABCVm::abc_equals_local_local(call_context* context)
{
	//equals

	bool ret=(asAtomHandler::isEqual(context->locals[context->exec_pos->local_pos2],context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos1]));
	LOG_CALL(_("equals_ll ")<<ret);

	RUNTIME_STACK_PUSH(context,asAtomHandler::fromBool(ret));
	++(context->exec_pos);
}
void ABCVm::abc_equals_constant_constant_localresult(call_context* context)
{
	//equals

	bool ret=(asAtomHandler::isEqual(*context->exec_pos->arg2_constant,context->mi->context->root->getSystemState(),*context->exec_pos->arg1_constant));
	LOG_CALL(_("equals_ccl ")<<ret);

	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	asAtomHandler::setBool(context->locals[context->exec_pos->local_pos3-1],ret);
	++(context->exec_pos);
}
void ABCVm::abc_equals_local_constant_localresult(call_context* context)
{
	//equals

	bool ret=(asAtomHandler::isEqual(*context->exec_pos->arg2_constant,context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos1]));
	LOG_CALL(_("equals_lcl ")<<ret);

	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	asAtomHandler::setBool(context->locals[context->exec_pos->local_pos3-1],ret);
	++(context->exec_pos);
}
void ABCVm::abc_equals_constant_local_localresult(call_context* context)
{
	//equals

	bool ret=(asAtomHandler::isEqual(*context->exec_pos->arg1_constant,context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos2]));
	LOG_CALL(_("equals_cll ")<<ret);

	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	asAtomHandler::setBool(context->locals[context->exec_pos->local_pos3-1],ret);
	++(context->exec_pos);
}
void ABCVm::abc_equals_local_local_localresult(call_context* context)
{
	//equals

	bool ret=(asAtomHandler::isEqual(context->locals[context->exec_pos->local_pos2],context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos1]));
	LOG_CALL(_("equals_lll ")<<ret);

	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	asAtomHandler::setBool(context->locals[context->exec_pos->local_pos3-1],ret);
	++(context->exec_pos);
}

void ABCVm::abc_strictequals(call_context* context)
{
	//strictequals
	RUNTIME_STACK_POP_CREATE(context,v2);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	bool ret = asAtomHandler::isEqualStrict(*pval,context->mi->context->root->getSystemState(),*v2);
	LOG_CALL( _("strictequals ") << ret);
	ASATOM_DECREF_POINTER(pval);
	ASATOM_DECREF_POINTER(v2);
	asAtomHandler::setBool(*pval,ret);
	++(context->exec_pos);
}
void ABCVm::abc_lessthan(call_context* context)
{
	RUNTIME_STACK_POP_CREATE(context,v2);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	//Real comparision demanded to object
	bool ret=(asAtomHandler::isLess(*pval,context->mi->context->root->getSystemState(),*v2)==TTRUE);
	LOG_CALL(_("lessThan ")<<ret<<" "<<asAtomHandler::toDebugString(*pval)<<" "<<asAtomHandler::toDebugString(*v2));
	ASATOM_DECREF_POINTER(pval);
	ASATOM_DECREF_POINTER(v2);

	asAtomHandler::setBool(*pval,ret);
	++(context->exec_pos);
}
void ABCVm::abc_lessthan_constant_constant(call_context* context)
{
	bool ret=(asAtomHandler::isLess(*context->exec_pos->arg1_constant,context->mi->context->root->getSystemState(),*context->exec_pos->arg2_constant)==TTRUE);
	LOG_CALL(_("lessthan_cc ")<<ret);

	RUNTIME_STACK_PUSH(context,asAtomHandler::fromBool(ret));
	++(context->exec_pos);
}
void ABCVm::abc_lessthan_local_constant(call_context* context)
{
	bool ret=(asAtomHandler::isLess(context->locals[context->exec_pos->local_pos1],context->mi->context->root->getSystemState(),*context->exec_pos->arg2_constant)==TTRUE);
	LOG_CALL(_("lessthan_lc ")<<ret);

	RUNTIME_STACK_PUSH(context,asAtomHandler::fromBool(ret));
	++(context->exec_pos);
}
void ABCVm::abc_lessthan_constant_local(call_context* context)
{
	bool ret=(asAtomHandler::isLess(*context->exec_pos->arg1_constant,context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos2])==TTRUE);
	LOG_CALL(_("lessthan_cl ")<<ret);

	RUNTIME_STACK_PUSH(context,asAtomHandler::fromBool(ret));
	++(context->exec_pos);
}
void ABCVm::abc_lessthan_local_local(call_context* context)
{
	bool ret=(asAtomHandler::isLess(context->locals[context->exec_pos->local_pos1],context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos2])==TTRUE);
	LOG_CALL(_("lessthan_ll ")<<ret);

	RUNTIME_STACK_PUSH(context,asAtomHandler::fromBool(ret));
	++(context->exec_pos);
}
void ABCVm::abc_lessthan_constant_constant_localresult(call_context* context)
{
	bool ret=(asAtomHandler::isLess(*context->exec_pos->arg1_constant,context->mi->context->root->getSystemState(),*context->exec_pos->arg2_constant)==TTRUE);
	LOG_CALL(_("lessthan_ccl ")<<ret);

	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	asAtomHandler::setBool(context->locals[context->exec_pos->local_pos3-1],ret);
	++(context->exec_pos);
}
void ABCVm::abc_lessthan_local_constant_localresult(call_context* context)
{
	bool ret=(asAtomHandler::isLess(context->locals[context->exec_pos->local_pos1],context->mi->context->root->getSystemState(),*context->exec_pos->arg2_constant)==TTRUE);
	LOG_CALL(_("lessthan_lcl ")<<ret);

	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	asAtomHandler::setBool(context->locals[context->exec_pos->local_pos3-1],ret);
	++(context->exec_pos);
}
void ABCVm::abc_lessthan_constant_local_localresult(call_context* context)
{
	bool ret=(asAtomHandler::isLess(*context->exec_pos->arg1_constant,context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos2])==TTRUE);
	LOG_CALL(_("lessthan_cll ")<<ret);

	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	asAtomHandler::setBool(context->locals[context->exec_pos->local_pos3-1],ret);
	++(context->exec_pos);
}
void ABCVm::abc_lessthan_local_local_localresult(call_context* context)
{
	bool ret=(asAtomHandler::isLess(context->locals[context->exec_pos->local_pos1],context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos2])==TTRUE);
	LOG_CALL(_("lessthan_lll ")<<ret);

	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	asAtomHandler::setBool(context->locals[context->exec_pos->local_pos3-1],ret);
	++(context->exec_pos);
}


void ABCVm::abc_lessequals(call_context* context)
{
	//lessequals
	RUNTIME_STACK_POP_CREATE(context,v2);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	//Real comparision demanded to object
	bool ret=(asAtomHandler::isLess(*v2,context->mi->context->root->getSystemState(),*pval)==TFALSE);
	LOG_CALL(_("lessEquals ")<<ret);
	ASATOM_DECREF_POINTER(pval);
	ASATOM_DECREF_POINTER(v2);

	asAtomHandler::setBool(*pval,ret);
	++(context->exec_pos);
}
void ABCVm::abc_lessequals_constant_constant(call_context* context)
{
	//lessequals

	bool ret=(asAtomHandler::isLess(*context->exec_pos->arg2_constant,context->mi->context->root->getSystemState(),*context->exec_pos->arg1_constant)==TFALSE);
	LOG_CALL(_("lessequals_cc ")<<ret);

	RUNTIME_STACK_PUSH(context,asAtomHandler::fromBool(ret));
	++(context->exec_pos);
}
void ABCVm::abc_lessequals_local_constant(call_context* context)
{
	//lessequals

	bool ret=(asAtomHandler::isLess(*context->exec_pos->arg2_constant,context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos1])==TFALSE);
	LOG_CALL(_("lessequals_lc ")<<ret);

	RUNTIME_STACK_PUSH(context,asAtomHandler::fromBool(ret));
	++(context->exec_pos);
}
void ABCVm::abc_lessequals_constant_local(call_context* context)
{
	//lessequals

	bool ret=(asAtomHandler::isLess(context->locals[context->exec_pos->local_pos2],context->mi->context->root->getSystemState(),*context->exec_pos->arg1_constant)==TFALSE);
	LOG_CALL(_("lessequals_cl ")<<ret);

	RUNTIME_STACK_PUSH(context,asAtomHandler::fromBool(ret));
	++(context->exec_pos);
}
void ABCVm::abc_lessequals_local_local(call_context* context)
{
	//lessequals

	bool ret=(asAtomHandler::isLess(context->locals[context->exec_pos->local_pos2],context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos1])==TFALSE);
	LOG_CALL(_("lessequals_ll ")<<ret);

	RUNTIME_STACK_PUSH(context,asAtomHandler::fromBool(ret));
	++(context->exec_pos);
}
void ABCVm::abc_lessequals_constant_constant_localresult(call_context* context)
{
	//lessequals

	bool ret=(asAtomHandler::isLess(*context->exec_pos->arg2_constant,context->mi->context->root->getSystemState(),*context->exec_pos->arg1_constant)==TFALSE);
	LOG_CALL(_("lessequals_ccl ")<<ret);

	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	asAtomHandler::setBool(context->locals[context->exec_pos->local_pos3-1],ret);
	++(context->exec_pos);
}
void ABCVm::abc_lessequals_local_constant_localresult(call_context* context)
{
	//lessequals

	bool ret=(asAtomHandler::isLess(*context->exec_pos->arg2_constant,context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos1])==TFALSE);
	LOG_CALL(_("lessequals_lcl ")<<ret);

	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	asAtomHandler::setBool(context->locals[context->exec_pos->local_pos3-1],ret);
	++(context->exec_pos);
}
void ABCVm::abc_lessequals_constant_local_localresult(call_context* context)
{
	//lessequals

	bool ret=(asAtomHandler::isLess(context->locals[context->exec_pos->local_pos2],context->mi->context->root->getSystemState(),*context->exec_pos->arg1_constant)==TFALSE);
	LOG_CALL(_("lessequals_cll ")<<ret);

	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	asAtomHandler::setBool(context->locals[context->exec_pos->local_pos3-1],ret);
	++(context->exec_pos);
}
void ABCVm::abc_lessequals_local_local_localresult(call_context* context)
{
	//lessequals

	bool ret=(asAtomHandler::isLess(context->locals[context->exec_pos->local_pos2],context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos1])==TFALSE);
	LOG_CALL(_("lessequals_lll ")<<ret);

	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	asAtomHandler::setBool(context->locals[context->exec_pos->local_pos3-1],ret);
	++(context->exec_pos);
}

void ABCVm::abc_greaterthan(call_context* context)
{
	//greaterthan
	RUNTIME_STACK_POP_CREATE(context,v2);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	//Real comparision demanded to object
	bool ret=(asAtomHandler::isLess(*v2,context->mi->context->root->getSystemState(),*pval)==TTRUE);
	LOG_CALL(_("greaterThan ")<<ret);
	ASATOM_DECREF_POINTER(pval);
	ASATOM_DECREF_POINTER(v2);

	asAtomHandler::setBool(*pval,ret);
	++(context->exec_pos);
}
void ABCVm::abc_greaterthan_constant_constant(call_context* context)
{
	//greaterthan

	bool ret=(asAtomHandler::isLess(*context->exec_pos->arg2_constant,context->mi->context->root->getSystemState(),*context->exec_pos->arg1_constant)==TTRUE);
	LOG_CALL(_("greaterThan_cc ")<<ret);

	RUNTIME_STACK_PUSH(context,asAtomHandler::fromBool(ret));
	++(context->exec_pos);
}
void ABCVm::abc_greaterthan_local_constant(call_context* context)
{
	//greaterthan

	bool ret=(asAtomHandler::isLess(*context->exec_pos->arg2_constant,context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos1])==TTRUE);
	LOG_CALL(_("greaterThan_lc ")<<ret);

	RUNTIME_STACK_PUSH(context,asAtomHandler::fromBool(ret));
	++(context->exec_pos);
}
void ABCVm::abc_greaterthan_constant_local(call_context* context)
{
	//greaterthan

	bool ret=(asAtomHandler::isLess(context->locals[context->exec_pos->local_pos2],context->mi->context->root->getSystemState(),*context->exec_pos->arg1_constant)==TTRUE);
	LOG_CALL(_("greaterThan_cl ")<<ret);

	RUNTIME_STACK_PUSH(context,asAtomHandler::fromBool(ret));
	++(context->exec_pos);
}
void ABCVm::abc_greaterthan_local_local(call_context* context)
{
	//greaterthan

	bool ret=(asAtomHandler::isLess(context->locals[context->exec_pos->local_pos2],context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos1])==TTRUE);
	LOG_CALL(_("greaterThan_ll ")<<ret);

	RUNTIME_STACK_PUSH(context,asAtomHandler::fromBool(ret));
	++(context->exec_pos);
}
void ABCVm::abc_greaterthan_constant_constant_localresult(call_context* context)
{
	//greaterthan

	bool ret=(asAtomHandler::isLess(*context->exec_pos->arg2_constant,context->mi->context->root->getSystemState(),*context->exec_pos->arg1_constant)==TTRUE);
	LOG_CALL(_("greaterThan_ccl ")<<ret);

	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	asAtomHandler::setBool(context->locals[context->exec_pos->local_pos3-1],ret);
	++(context->exec_pos);
}
void ABCVm::abc_greaterthan_local_constant_localresult(call_context* context)
{
	//greaterthan

	bool ret=(asAtomHandler::isLess(*context->exec_pos->arg2_constant,context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos1])==TTRUE);
	LOG_CALL(_("greaterThan_lcl ")<<ret);

	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	asAtomHandler::setBool(context->locals[context->exec_pos->local_pos3-1],ret);
	++(context->exec_pos);
}
void ABCVm::abc_greaterthan_constant_local_localresult(call_context* context)
{
	//greaterthan

	bool ret=(asAtomHandler::isLess(context->locals[context->exec_pos->local_pos2],context->mi->context->root->getSystemState(),*context->exec_pos->arg1_constant)==TTRUE);
	LOG_CALL(_("greaterThan_cll ")<<ret);

	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	asAtomHandler::setBool(context->locals[context->exec_pos->local_pos3-1],ret);
	++(context->exec_pos);
}
void ABCVm::abc_greaterthan_local_local_localresult(call_context* context)
{
	//greaterthan

	bool ret=(asAtomHandler::isLess(context->locals[context->exec_pos->local_pos2],context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos1])==TTRUE);
	LOG_CALL(_("greaterThan_lll ")<<ret);

	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	asAtomHandler::setBool(context->locals[context->exec_pos->local_pos3-1],ret);
	++(context->exec_pos);
}
void ABCVm::abc_greaterequals(call_context* context)
{
	//greaterequals
	RUNTIME_STACK_POP_CREATE(context,v2);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	//Real comparision demanded to object
	bool ret=(asAtomHandler::isLess(*pval,context->mi->context->root->getSystemState(),*v2)==TFALSE);
	LOG_CALL(_("greaterEquals ")<<ret);
	ASATOM_DECREF_POINTER(pval);
	ASATOM_DECREF_POINTER(v2);

	asAtomHandler::setBool(*pval,ret);
	++(context->exec_pos);
}
void ABCVm::abc_greaterequals_constant_constant(call_context* context)
{
	//greaterequals

	bool ret=(asAtomHandler::isLess(*context->exec_pos->arg1_constant,context->mi->context->root->getSystemState(),*context->exec_pos->arg2_constant)==TFALSE);
	LOG_CALL(_("greaterequals_cc ")<<ret);

	RUNTIME_STACK_PUSH(context,asAtomHandler::fromBool(ret));
	++(context->exec_pos);
}
void ABCVm::abc_greaterequals_local_constant(call_context* context)
{
	//greaterequals

	bool ret=(asAtomHandler::isLess(context->locals[context->exec_pos->local_pos1],context->mi->context->root->getSystemState(),*context->exec_pos->arg2_constant)==TFALSE);
	LOG_CALL(_("greaterequals_lc ")<<ret);

	RUNTIME_STACK_PUSH(context,asAtomHandler::fromBool(ret));
	++(context->exec_pos);
}
void ABCVm::abc_greaterequals_constant_local(call_context* context)
{
	//greaterequals

	bool ret=(asAtomHandler::isLess(*context->exec_pos->arg1_constant,context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos2])==TFALSE);
	LOG_CALL(_("greaterequals_cl ")<<ret);

	RUNTIME_STACK_PUSH(context,asAtomHandler::fromBool(ret));
	++(context->exec_pos);
}
void ABCVm::abc_greaterequals_local_local(call_context* context)
{
	//greaterequals

	bool ret=(asAtomHandler::isLess(context->locals[context->exec_pos->local_pos1],context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos2])==TFALSE);
	LOG_CALL(_("greaterequals_ll ")<<ret);

	RUNTIME_STACK_PUSH(context,asAtomHandler::fromBool(ret));
	++(context->exec_pos);
}
void ABCVm::abc_greaterequals_constant_constant_localresult(call_context* context)
{
	//greaterequals

	bool ret=(asAtomHandler::isLess(*context->exec_pos->arg1_constant,context->mi->context->root->getSystemState(),*context->exec_pos->arg2_constant)==TFALSE);
	LOG_CALL(_("greaterequals_ccl ")<<ret);

	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	asAtomHandler::setBool(context->locals[context->exec_pos->local_pos3-1],ret);
	++(context->exec_pos);
}
void ABCVm::abc_greaterequals_local_constant_localresult(call_context* context)
{
	//greaterequals

	bool ret=(asAtomHandler::isLess(context->locals[context->exec_pos->local_pos1],context->mi->context->root->getSystemState(),*context->exec_pos->arg2_constant)==TFALSE);
	LOG_CALL(_("greaterequals_lcl ")<<ret);

	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	asAtomHandler::setBool(context->locals[context->exec_pos->local_pos3-1],ret);
	++(context->exec_pos);
}
void ABCVm::abc_greaterequals_constant_local_localresult(call_context* context)
{
	//greaterequals

	bool ret=(asAtomHandler::isLess(*context->exec_pos->arg1_constant,context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos2])==TFALSE);
	LOG_CALL(_("greaterequals_cll ")<<ret);

	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	asAtomHandler::setBool(context->locals[context->exec_pos->local_pos3-1],ret);
	++(context->exec_pos);
}
void ABCVm::abc_greaterequals_local_local_localresult(call_context* context)
{
	//greaterequals

	bool ret=(asAtomHandler::isLess(context->locals[context->exec_pos->local_pos1],context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos2])==TFALSE);
	LOG_CALL(_("greaterequals_lll ")<<ret<<" "<<asAtomHandler::toDebugString(context->locals[context->exec_pos->local_pos1])<<" "<<asAtomHandler::toDebugString(context->locals[context->exec_pos->local_pos2]));

	ASATOM_DECREF(context->locals[context->exec_pos->local_pos3-1]);
	asAtomHandler::setBool(context->locals[context->exec_pos->local_pos3-1],ret);
	++(context->exec_pos);
}




void ABCVm::abc_instanceof(call_context* context)
{
	//instanceof
	RUNTIME_STACK_POP_CREATE_ASOBJECT(context,type, context->mi->context->root->getSystemState());
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	bool ret = instanceOf(asAtomHandler::toObject(*pval,context->mi->context->root->getSystemState()),type);
	ASATOM_DECREF_POINTER(pval);
	asAtomHandler::setBool(*pval,ret);
	++(context->exec_pos);
}
void ABCVm::abc_istype(call_context* context)
{
	//istype
	uint32_t t = (++(context->exec_pos))->data;
	multiname* name=context->mi->context->getMultiname(t,NULL);

	RUNTIME_STACK_POINTER_CREATE(context,pval);
	ASObject* o = asAtomHandler::toObject(*pval,context->mi->context->root->getSystemState());
	asAtomHandler::setBool(*pval,isType(context->mi->context,o,name));
	o->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_istypelate(call_context* context)
{
	//istypelate
	RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v1, context->mi->context->root->getSystemState());
	RUNTIME_STACK_POINTER_CREATE(context,pval);

	bool ret =asAtomHandler::isTypelate(*pval,v1);
	ASATOM_DECREF_POINTER(pval);
	asAtomHandler::setBool(*pval,ret);
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
	asAtomHandler::fillMultiname(*pval,context->mi->context->root->getSystemState(),name);
	name.ns.emplace_back(context->mi->context->root->getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
	bool ret=v1->hasPropertyByMultiname(name, true, true);
	ASATOM_DECREF_POINTER(pval);
	v1->decRef();
	asAtomHandler::setBool(*pval,ret);

	++(context->exec_pos);
}
void ABCVm::abc_increment_i(call_context* context)
{
	//increment_i
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	LOG_CALL("increment_i:"<<asAtomHandler::toDebugString(*pval));
	asAtomHandler::increment_i(*pval,context->mi->context->root->getSystemState());
	++(context->exec_pos);
}
void ABCVm::abc_increment_i_local(call_context* context)
{
	asAtom res = context->locals[context->exec_pos->local_pos1];
	LOG_CALL("increment_i_l "<<context->exec_pos->local_pos1<<" "<<asAtomHandler::toDebugString(res));
	asAtomHandler::increment_i(res,context->mi->context->root->getSystemState());
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_increment_i_local_localresult(call_context* context)
{
	LOG_CALL("increment_i_ll "<<context->exec_pos->local_pos1<<" "<<context->exec_pos->local_pos3<<" "<<asAtomHandler::toDebugString(context->locals[context->exec_pos->local_pos3-1]));
	asAtom res = context->locals[context->exec_pos->local_pos1];
	ASObject* o = asAtomHandler::getObject(context->locals[context->exec_pos->local_pos3-1]);
	asAtomHandler::set(context->locals[context->exec_pos->local_pos3-1],res);
	asAtomHandler::increment_i(context->locals[context->exec_pos->local_pos3-1],context->mi->context->root->getSystemState());
	if (o)
		o->decRef();
	++(context->exec_pos);
}

void ABCVm::abc_decrement_i(call_context* context)
{
	//decrement_i
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	LOG_CALL("decrement_i:"<<asAtomHandler::toDebugString(*pval));
	asAtomHandler::decrement_i(*pval,context->mi->context->root->getSystemState());
	++(context->exec_pos);
}
void ABCVm::abc_decrement_i_local(call_context* context)
{
	asAtom res = context->locals[context->exec_pos->local_pos1];
	LOG_CALL("decrement_i_l "<<context->exec_pos->local_pos1<<" "<<asAtomHandler::toDebugString(res));
	asAtomHandler::decrement_i(res,context->mi->context->root->getSystemState());
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_decrement_i_local_localresult(call_context* context)
{
	LOG_CALL("decrement_i_ll "<<context->exec_pos->local_pos1<<" "<<context->exec_pos->local_pos3<<" "<<asAtomHandler::toDebugString(context->locals[context->exec_pos->local_pos3-1]));
	asAtom res = context->locals[context->exec_pos->local_pos1];
	ASObject* o = asAtomHandler::getObject(context->locals[context->exec_pos->local_pos3-1]);
	asAtomHandler::set(context->locals[context->exec_pos->local_pos3-1],res);
	asAtomHandler::decrement_i(context->locals[context->exec_pos->local_pos3-1],context->mi->context->root->getSystemState());
	if (o)
		o->decRef();
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
	asAtomHandler::negate_i(*pval,context->mi->context->root->getSystemState());
	++(context->exec_pos);
}
void ABCVm::abc_add_i(call_context* context)
{
	RUNTIME_STACK_POP_CREATE(context,v2);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	asAtomHandler::add_i(*pval,context->mi->context->root->getSystemState(),*v2);
	++(context->exec_pos);
}
void ABCVm::abc_add_i_constant_constant(call_context* context)
{
	LOG_CALL("add_i_cc");
	asAtom res = *context->exec_pos->arg1_constant;
	asAtomHandler::add_i(res,context->mi->context->root->getSystemState(),*context->exec_pos->arg2_constant);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_add_i_local_constant(call_context* context)
{
	LOG_CALL("add_i_lc");
	asAtom res = context->locals[context->exec_pos->local_pos1];
	asAtomHandler::add_i(res,context->mi->context->root->getSystemState(),*context->exec_pos->arg2_constant);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_add_i_constant_local(call_context* context)
{
	LOG_CALL("add_i_cl");
	asAtom res = *context->exec_pos->arg1_constant;
	asAtomHandler::add_i(res,context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos2]);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_add_i_local_local(call_context* context)
{
	LOG_CALL("add_i_ll");
	asAtom res = context->locals[context->exec_pos->local_pos1];
	asAtomHandler::add_i(res,context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos2]);
	RUNTIME_STACK_PUSH(context,res);
	++(context->exec_pos);
}
void ABCVm::abc_add_i_constant_constant_localresult(call_context* context)
{
	LOG_CALL("add_i_ccl");
	asAtom res = *context->exec_pos->arg1_constant;
	ASObject* o = asAtomHandler::getObject(context->locals[context->exec_pos->local_pos3-1]);
	asAtomHandler::add_i(res,context->mi->context->root->getSystemState(),*context->exec_pos->arg2_constant);
	asAtomHandler::set(context->locals[context->exec_pos->local_pos3-1],res);
	if (o)
		o->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_add_i_local_constant_localresult(call_context* context)
{
	LOG_CALL("add_i_lcl");
	asAtom res = context->locals[context->exec_pos->local_pos1];
	ASObject* o = asAtomHandler::getObject(context->locals[context->exec_pos->local_pos3-1]);
	asAtomHandler::add_i(res,context->mi->context->root->getSystemState(),*context->exec_pos->arg2_constant);
	asAtomHandler::set(context->locals[context->exec_pos->local_pos3-1],res);
	if (o)
		o->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_add_i_constant_local_localresult(call_context* context)
{
	LOG_CALL("add_i_cll");
	asAtom res = *context->exec_pos->arg1_constant;
	ASObject* o = asAtomHandler::getObject(context->locals[context->exec_pos->local_pos3-1]);
	asAtomHandler::add_i(res,context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos2]);
	asAtomHandler::set(context->locals[context->exec_pos->local_pos3-1],res);
	if (o)
		o->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_add_i_local_local_localresult(call_context* context)
{
	LOG_CALL("add_i_lll");
	asAtom res = context->locals[context->exec_pos->local_pos1];
	ASObject* o = asAtomHandler::getObject(context->locals[context->exec_pos->local_pos3-1]);
	asAtomHandler::add_i(res,context->mi->context->root->getSystemState(),context->locals[context->exec_pos->local_pos2]);
	asAtomHandler::set(context->locals[context->exec_pos->local_pos3-1],res);
	if (o)
		o->decRef();
	++(context->exec_pos);
}

void ABCVm::abc_subtract_i(call_context* context)
{
	//subtract_i
	RUNTIME_STACK_POP_CREATE(context,v2);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	asAtomHandler::subtract_i(*pval,context->mi->context->root->getSystemState(),*v2);
	++(context->exec_pos);
}
void ABCVm::abc_multiply_i(call_context* context)
{
	//multiply_i
	RUNTIME_STACK_POP_CREATE(context,v2);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	asAtomHandler::multiply_i(*pval,context->mi->context->root->getSystemState(),*v2);
	++(context->exec_pos);
}
void ABCVm::abc_getlocal_0(call_context* context)
{
	//getlocal_0
	int i=0;
	LOG_CALL( _("getLocal ") << i << _(": ") << asAtomHandler::toDebugString(context->locals[i]) );
	ASATOM_INCREF(context->locals[i]);
	RUNTIME_STACK_PUSH(context,context->locals[i]);
	++(context->exec_pos);
}
void ABCVm::abc_getlocal_1(call_context* context)
{
	//getlocal_1
	int i=1;
	LOG_CALL( _("getLocal ") << i << _(": ") << asAtomHandler::toDebugString(context->locals[i]) );
	ASATOM_INCREF(context->locals[i]);
	RUNTIME_STACK_PUSH(context,context->locals[i]);
	++(context->exec_pos);
}
void ABCVm::abc_getlocal_2(call_context* context)
{
	//getlocal_2
	int i=2;
	LOG_CALL( _("getLocal ") << i << _(": ") << asAtomHandler::toDebugString(context->locals[i]) );
	ASATOM_INCREF(context->locals[i]);
	RUNTIME_STACK_PUSH(context,context->locals[i]);
	++(context->exec_pos);
}
void ABCVm::abc_getlocal_3(call_context* context)
{
	//getlocal_3
	int i=3;
	LOG_CALL( _("getLocal ") << i << _(": ") << asAtomHandler::toDebugString(context->locals[i]) );
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
	if (i > context->mi->body->local_count)
	{
		LOG(LOG_ERROR,"abc_setlocal invalid index:"<<i);
		return;
	}
	if ((int)i != context->argarrayposition || asAtomHandler::isArray(*obj))
	{
		ASATOM_DECREF(context->locals[i]);
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
	if (i > context->mi->body->local_count)
	{
		LOG(LOG_ERROR,"abc_setlocal invalid index:"<<i);
		return;
	}
	if ((int)i != context->argarrayposition || asAtomHandler::isArray(*obj))
	{
		ASATOM_DECREF(context->locals[i]);
		context->locals[i]=*obj;
	}
}
void ABCVm::abc_setlocal_2(call_context* context)
{
	//setlocal_2
	unsigned int i=2;
	++(context->exec_pos);
	RUNTIME_STACK_POP_CREATE(context,obj)
	LOG_CALL( _("setLocal ") << i<<" "<<asAtomHandler::toDebugString(*obj));
	if (i > context->mi->body->local_count)
	{
		LOG(LOG_ERROR,"abc_setlocal invalid index:"<<i);
		return;
	}
	if ((int)i != context->argarrayposition || asAtomHandler::isArray(*obj))
	{
		ASATOM_DECREF(context->locals[i]);
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
	if (i > context->mi->body->local_count)
	{
		LOG(LOG_ERROR,"abc_setlocal invalid index:"<<i);
		return;
	}
	if ((int)i != context->argarrayposition || asAtomHandler::isArray(*obj))
	{
		ASATOM_DECREF(context->locals[i]);
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
	Class_base* objtype;
	int32_t index;
	uint32_t codecount;
	uint32_t preloadedcodepos;
	operands(OPERANDTYPES _t, Class_base* _o,int32_t _i,uint32_t _c, uint32_t _p):type(_t),objtype(_o),index(_i),codecount(_c),preloadedcodepos(_p) {}
	void removeArg(method_info* mi)
	{
		if (codecount)
			mi->body->preloadedcode.erase(mi->body->preloadedcode.begin()+preloadedcodepos,mi->body->preloadedcode.begin()+preloadedcodepos+codecount);
	}
	bool fillCode(int pos, preloadedcodedata& code, ABCContext* context, bool switchopcode)
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
				else
					return true;
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
		return false;
	}
};
#define ABC_OP_OPTIMZED_INCREMENT 0x00000100
#define ABC_OP_OPTIMZED_DECREMENT 0x00000102
#define ABC_OP_OPTIMZED_PUSHSCOPE 0x00000104
#define ABC_OP_OPTIMZED_GETPROPERTY_STATICNAME 0x00000106
#define ABC_OP_OPTIMZED_GETLEX 0x0000010a
#define ABC_OP_OPTIMZED_CALLPROPERTY_STATICNAME_LOCALRESULT 0x0000010b
#define ABC_OP_OPTIMZED_SETPROPERTY_STATICNAME 0x0000011c 
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
#define ABC_OP_OPTIMZED_PUSHCACHEDCONSTANT 0x000001ba
#define ABC_OP_OPTIMZED_LESSEQUALS 0x000001c0
#define ABC_OP_OPTIMZED_GREATEREQUALS 0x000001c8
#define ABC_OP_OPTIMZED_GETLEX_FROMSLOT 0x000001bb 
#define ABC_OP_OPTIMZED_CALLPROPERTY_STATICNAME_MULTIARGS 0x000001bd
#define ABC_OP_OPTIMZED_CALLPROPVOID_STATICNAME_MULTIARGS 0x000001be
#define ABC_OP_OPTIMZED_GETPROPERTY_STATICNAME_LOCALRESULT 0x000001bf
#define ABC_OP_OPTIMZED_EQUALS 0x000001d0
#define ABC_OP_OPTIMZED_NOT 0x000001d8
#define ABC_OP_OPTIMZED_IFTRUE_DUP 0x000001dc
#define ABC_OP_OPTIMZED_IFFALSE_DUP 0x000001de
#define ABC_OP_OPTIMZED_CALLPROPERTY_STATICNAME_NOARGS 0x000001e0
#define ABC_OP_OPTIMZED_CALLPROPVOID_STATICNAME 0x000001e4
#define ABC_OP_OPTIMZED_CALLPROPVOID_STATICNAME_NOARGS 0x000001e8
#define ABC_OP_OPTIMZED_CONSTRUCTSUPER 0x000001ea 
#define ABC_OP_OPTIMZED_GETSLOT 0x000001ec
#define ABC_OP_OPTIMZED_CALLFUNCTION_NOARGS 0x000001f0 
#define ABC_OP_OPTIMZED_CALLFUNCTIONSYNTHETIC_ONEARG_VOID 0x000001f4
#define ABC_OP_OPTIMZED_INCREMENT_I 0x000001f8
#define ABC_OP_OPTIMZED_DECREMENT_I 0x000001fa
#define ABC_OP_OPTIMZED_CALLFUNCTION_MULTIARGS_VOID 0x000001fc
#define ABC_OP_OPTIMZED_CALLFUNCTION_MULTIARGS 0x000001fd
#define ABC_OP_OPTIMZED_GETSCOPEOBJECT_LOCALRESULT 0x000001fe
#define ABC_OP_OPTIMZED_CALLPROPVOID_STATICNAME_MULTIARGS_CACHED 0x000001ff

#define ABC_OP_OPTIMZED_LI8 0x00000200
#define ABC_OP_OPTIMZED_LI16 0x00000204
#define ABC_OP_OPTIMZED_LI32 0x00000208
#define ABC_OP_OPTIMZED_LF32 0x0000021c
#define ABC_OP_OPTIMZED_LF64 0x00000220
#define ABC_OP_OPTIMZED_SI8 0x00000224
#define ABC_OP_OPTIMZED_SI16 0x00000228
#define ABC_OP_OPTIMZED_SI32 0x0000022c
#define ABC_OP_OPTIMZED_SF32 0x00000230
#define ABC_OP_OPTIMZED_SF64 0x00000234
#define ABC_OP_OPTIMZED_SETSLOT 0x00000238
#define ABC_OP_OPTIMZED_CONVERTI 0x0000023c
#define ABC_OP_OPTIMZED_CONVERTU 0x00000240
#define ABC_OP_OPTIMZED_CONSTRUCTPROP_STATICNAME_NOARGS 0x00000244 
#define ABC_OP_OPTIMZED_CONVERTB 0x00000248 
#define ABC_OP_OPTIMZED_CONSTRUCT_NOARGS 0x0000024c
#define ABC_OP_OPTIMZED_SETSLOT_NOCOERCE 0x00000250
#define ABC_OP_OPTIMZED_CALLFUNCTION_NOARGS_VOID 0x00000254

#define ABC_OP_OPTIMZED_LESSTHAN 0x00000258
#define ABC_OP_OPTIMZED_ADD_I 0x00000260
#define ABC_OP_OPTIMZED_TYPEOF 0x00000268
#define ABC_OP_OPTIMZED_CALLFUNCTIONBUILTIN_ONEARG_VOID 0x0000026c
#define ABC_OP_OPTIMZED_CALLFUNCTIONBUILTIN_MULTIARGS_VOID 0x00000270
#define ABC_OP_OPTIMZED_CALLFUNCTIONSYNTHETIC_MULTIARGS_VOID 0x00000272
#define ABC_OP_OPTIMZED_CALLFUNCTIONBUILTIN_MULTIARGS 0x00000274
#define ABC_OP_OPTIMZED_CALLFUNCTIONSYNTHETIC_MULTIARGS 0x00000278
#define ABC_OP_OPTIMZED_CALLPROPERTY_STATICNAME_MULTIARGS_CACHED_CALLER 0x0000027c
#define ABC_OP_OPTIMZED_CALLPROPERTY_STATICNAME_MULTIARGS_CACHED 0x00000280
#define ABC_OP_OPTIMZED_CALLPROPVOID_STATICNAME_MULTIARGS_CACHED_CALLER 0x00000282

void skipjump(uint8_t& b,method_info* mi,memorystream& code,uint32_t& pos,std::map<int32_t,int32_t>& oldnewpositions,std::map<int32_t,int32_t>& jumptargets,bool jumpInCode)
{
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
			auto it = jumptargets.find(p+j+1);
			if (it != jumptargets.end() && it->second > 1)
				jumptargets[p+j+1]--;
			else
				jumptargets.erase(p+j+1);
			b = code.peekbyteFromPosition(pos);
			if (jumpInCode)
			{
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				code.seekg(pos);
				oldnewpositions[code.tellg()+1] = (int32_t)mi->body->preloadedcode.size();
			}
			pos++;
		}
	}
}
void clearOperands(method_info* mi,Class_base** localtypes,std::list<operands>& operandlist, Class_base** defaultlocaltypes,Class_base** lastlocalresulttype )
{
	for (uint32_t i = 0; i < mi->body->local_count+2; i++)
		localtypes[i] = defaultlocaltypes[i];
	operandlist.clear();
	if (lastlocalresulttype)
		*lastlocalresulttype=nullptr;
}
bool canCallFunctionDirect(operands& op,multiname* name)
{
	return ((op.type == OP_LOCAL || op.type == OP_CACHED_CONSTANT) &&
		op.objtype &&
		!op.objtype->isInterface && // it's not an interface
		op.objtype->isSealed && // it's sealed
		(
		!op.objtype->is<Class_inherit>() || // type is builtin class
		!op.objtype->as<Class_inherit>()->hasoverriddenmethod(name) // current method is not in overridden methods
		));
}

bool checkForLocalResult(std::list<operands>& operandlist,method_info* mi,memorystream& code,std::map<int32_t,int32_t>& oldnewpositions,std::map<int32_t,int32_t>& jumptargets,uint32_t opcode_jumpspace, Class_base** localtypes, Class_base* restype, Class_base** defaultlocaltypes,int preloadpos=-1,int preloadlocalpos=-1)
{
	bool res = false;
	bool hasdup = false;
	uint32_t resultpos=0;
	uint32_t pos = code.tellg()+1;
	uint8_t b = code.peekbyte();
	bool keepchecking=true;
	if (preloadpos == -1)
		preloadpos = mi->body->preloadedcode.size()-1;
	if (preloadlocalpos == -1)
		preloadlocalpos = mi->body->preloadedcode.size()-1;
	uint32_t argsneeded=0;
	bool localresultfound=false;
	int localresultused=0;
	for (auto it = operandlist.begin(); it != operandlist.end(); it++)
	{
		if (it->type != OP_LOCAL)
			continue;
		if (uint32_t(it->index) > mi->body->local_count) // local result index already used
		{
			resultpos = 1- (it->index- (mi->body->local_count+1)); // use free entry for resultpos
			localresultfound=true;
			break;
		}
	}
	while (jumptargets.find(pos) == jumptargets.end() && keepchecking)
	{
		skipjump(b,mi,code,pos,oldnewpositions,jumptargets,false);
		// check if the next opcode can be skipped
		switch (b)
		{
			case 0x24://pushbyte
				pos++;
				b = code.peekbyteFromPosition(pos);
				pos++;
				argsneeded++;
				if (b==0x73)//convert_i
				{
					b = code.peekbyteFromPosition(pos);
					pos++;
				}
				break;
			case 0x25://pushshort
			case 0x2d://pushint
				pos = code.skipu30FromPosition(pos);
				b = code.peekbyteFromPosition(pos);
				pos++;
				argsneeded++;
				if (b==0x73)//convert_i
				{
					b = code.peekbyteFromPosition(pos);
					pos++;
				}
				break;
			case 0x26://pushtrue
				b = code.peekbyteFromPosition(pos);
				pos++;
				argsneeded++;
				if (b==0x76)//convert_b
				{
					b = code.peekbyteFromPosition(pos);
					pos++;
				}
				keepchecking=false;
				break;
			case 0x27://pushfalse
				b = code.peekbyteFromPosition(pos);
				pos++;
				argsneeded++;
				if (b==0x76)//convert_b
				{
					b = code.peekbyteFromPosition(pos);
					pos++;
				}
				break;
			case 0x2e://pushuint
				pos = code.skipu30FromPosition(pos);
				b = code.peekbyteFromPosition(pos);
				pos++;
				argsneeded++;
				if (b==0x74)//convert_u
				{
					b = code.peekbyteFromPosition(pos);
					pos++;
				}
				break;
			case 0x2f://pushdouble
				pos = code.skipu30FromPosition(pos);
				b = code.peekbyteFromPosition(pos);
				pos++;
				argsneeded++;
				if (b==0x75)//convert_d
				{
					b = code.peekbyteFromPosition(pos);
					pos++;
				}
				break;
			case 0x2c://pushstring
			case 0x31://pushnamespace
			case 0x62://getlocal
				pos = code.skipu30FromPosition(pos);
				b = code.peekbyteFromPosition(pos);
				pos++;
				argsneeded++;
				break;
			case 0x60://getlex
				if (localresultused<=1)
				{
					pos = code.skipu30FromPosition(pos);
					b = code.peekbyteFromPosition(pos);
					pos++;
					argsneeded++;
					localresultused++;
				}
				else
					keepchecking=false;
				break;
			case 0x73://convert_i
				if (restype == Class<Integer>::getRef(mi->context->root->getSystemState()).getPtr())
				{
					b = code.peekbyteFromPosition(pos);
					pos++;
				}
				else
					keepchecking=false;
				break;
			case 0x74://convert_u
				if (restype == Class<UInteger>::getRef(mi->context->root->getSystemState()).getPtr())
				{
					b = code.peekbyteFromPosition(pos);
					pos++;
				}
				else
					keepchecking=false;
				break;
			case 0x75://convert_d
				if (restype == Class<Number>::getRef(mi->context->root->getSystemState()).getPtr())
				{
					b = code.peekbyteFromPosition(pos);
					pos++;
				}
				else
					keepchecking=false;
				break;
			case 0x76://convert_b
				if (restype == Class<Boolean>::getRef(mi->context->root->getSystemState()).getPtr()
						|| code.peekbyteFromPosition(pos) == 0x11 //iftrue
						|| code.peekbyteFromPosition(pos) == 0x12 //iffalse
					)
				{
					b = code.peekbyteFromPosition(pos);
					pos++;
				}
				else
					keepchecking=false;
				break;
			case 0x2a://dup
				b = code.peekbyteFromPosition(pos);
				pos++;
				argsneeded++;
				hasdup=true;
				keepchecking=false;
				break;
			case 0x20://pushnull
				b = code.peekbyteFromPosition(pos);
				pos++;
				argsneeded++;
				if (b==0x80)//coerce
				{
					uint32_t t = code.peeku30FromPosition(pos);
					multiname* name =  mi->context->getMultinameImpl(asAtomHandler::nullAtom,nullptr,t,false);
					const Type* tp = Type::getTypeFromMultiname(name, mi->context);
					const Class_base* cls =dynamic_cast<const Class_base*>(tp);
					if (cls != Class<Number>::getRef(mi->context->root->getSystemState()).getPtr() &&
						cls != Class<Integer>::getRef(mi->context->root->getSystemState()).getPtr() &&
						cls != Class<UInteger>::getRef(mi->context->root->getSystemState()).getPtr() &&
						cls != Class<Boolean>::getRef(mi->context->root->getSystemState()).getPtr())
					{
						pos = code.skipu30FromPosition(pos);
						b = code.peekbyteFromPosition(pos);
						pos++;
					}
				}
				break;
			case 0x28://pushnan
			case 0xd0://getlocal_0
			case 0xd1://getlocal_1
			case 0xd2://getlocal_2
			case 0xd3://getlocal_3
				b = code.peekbyteFromPosition(pos);
				pos++;
				argsneeded++;
				break;
			case 0x80://coerce
			{
				uint32_t t = code.peeku30FromPosition(pos);
				multiname* name =  mi->context->getMultinameImpl(asAtomHandler::nullAtom,nullptr,t,false);
				if (name->isStatic)
				{
					const Type* tp = Type::getTypeFromMultiname(name,mi->context);
					if (tp && tp == restype)
					{
						pos = code.skipu30FromPosition(pos);
						b = code.peekbyteFromPosition(pos);
						pos++;
					}
				}
				keepchecking=false;
				break;
			}
			case 0x82://coerce_a
				b = code.peekbyteFromPosition(pos);
				pos++;
				break;
			default:
				keepchecking=false;
				break;
		}
	}
	if (localresultused > (localresultfound ? 0 : 1))
	{
		// both positions for resultpos already used, no local result possible
		clearOperands(mi,localtypes,operandlist, defaultlocaltypes,nullptr);
		return res;
	}
	
	skipjump(b,mi,code,pos,oldnewpositions,jumptargets,!argsneeded);
	// check if we need to store the result of the operation on stack
	switch (b)
	{
		case 0x63://setlocal
		{
			if (!argsneeded && jumptargets.find(pos) == jumptargets.end())
			{
				uint32_t num = code.peeku30FromPosition(pos);
				pos = code.skipu30FromPosition(pos);
				code.seekg(pos);
				// set optimized opcode to corresponding opcode with local result 
				mi->body->preloadedcode[preloadpos].data += opcode_jumpspace;
				mi->body->preloadedcode[preloadlocalpos].local_pos3 = num+1;
				localtypes[num] = restype;
				res = true;
			}
			else
				clearOperands(mi,localtypes,operandlist, defaultlocaltypes,nullptr);
			break;
		}
		case 0xd4: //setlocal_0
		case 0xd5: //setlocal_1
		case 0xd6: //setlocal_2
		case 0xd7: //setlocal_3
			if (!argsneeded && jumptargets.find(pos) == jumptargets.end())
			{
				code.seekg(pos);
				// set optimized opcode to corresponding opcode with local result 
				mi->body->preloadedcode[preloadpos].data += opcode_jumpspace;
				mi->body->preloadedcode[preloadlocalpos].local_pos3 = b-0xd3;
				localtypes[b-0xd4] = restype;
				res = true;
			}
			else
				clearOperands(mi,localtypes,operandlist, defaultlocaltypes,nullptr);
			break;
		case 0x4f://callpropvoid
		case 0x46://callproperty
		{
			uint32_t t = code.peeku30FromPosition(pos);
			uint32_t pos2 = code.skipu30FromPosition(pos);
			uint32_t argcount = code.peeku30FromPosition(pos2);
			if (jumptargets.find(pos) == jumptargets.end() 
					&& mi->context->constant_pool.multinames[t].runtimeargs == 0 &&
					(argcount >= argsneeded) &&
					(operandlist.size() >= argcount-argsneeded)
					)
			{
				// set optimized opcode to corresponding opcode with local result 
				mi->body->preloadedcode[preloadpos].data += opcode_jumpspace;
				mi->body->preloadedcode[preloadlocalpos].local_pos3 = mi->body->local_count+2+resultpos;
				operandlist.push_back(operands(OP_LOCAL,restype,mi->body->local_count+1+resultpos,0,0));
				res = true;
			}
			else
				clearOperands(mi,localtypes,operandlist, defaultlocaltypes,nullptr);
			break;
		}
		case 0x42://construct
		{
			uint32_t argcount = code.peeku30FromPosition(pos);
			if (jumptargets.find(pos) == jumptargets.end() 
					&& argcount == 0
					&& !argsneeded && (operandlist.size() >= 1))
			{
				// set optimized opcode to corresponding opcode with local result 
				mi->body->preloadedcode[preloadpos].data += opcode_jumpspace;
				mi->body->preloadedcode[preloadlocalpos].local_pos3 = mi->body->local_count+2+resultpos;
				operandlist.push_back(operands(OP_LOCAL,restype, mi->body->local_count+1+resultpos,0,0));
				res = true;
			}
			else
				clearOperands(mi,localtypes,operandlist,defaultlocaltypes,nullptr);
			break;
		}
		case 0x4a://constructprop
		{
			uint32_t t = code.peeku30FromPosition(pos);
			uint32_t pos2 = code.skipu30FromPosition(pos);
			uint32_t argcount = code.peeku30FromPosition(pos2);
			if (jumptargets.find(pos) == jumptargets.end() 
					&& argcount == 0
					&& mi->context->constant_pool.multinames[t].runtimeargs == 0
					&& !argsneeded && (operandlist.size() >= 1))
			{
				// set optimized opcode to corresponding opcode with local result 
				mi->body->preloadedcode[preloadpos].data += opcode_jumpspace;
				mi->body->preloadedcode[preloadlocalpos].local_pos3 = mi->body->local_count+2+resultpos;
				operandlist.push_back(operands(OP_LOCAL,restype, mi->body->local_count+1+resultpos,0,0));
				res = true;
			}
			else
				clearOperands(mi,localtypes,operandlist,defaultlocaltypes,nullptr);
			break;
		}
		case 0x61://setproperty
		case 0x68://initproperty
		{
			uint32_t t = code.peeku30FromPosition(pos);
			if (jumptargets.find(pos) == jumptargets.end() && (argsneeded<=1) &&
					((argsneeded==1) || operandlist.size() >= 1) &&
					(uint32_t)mi->context->constant_pool.multinames[t].runtimeargs == 0
					)
			{
				// set optimized opcode to corresponding opcode with local result 
				mi->body->preloadedcode[preloadpos].data += opcode_jumpspace;
				mi->body->preloadedcode[preloadlocalpos].local_pos3 = mi->body->local_count+2+resultpos;
				operandlist.push_back(operands(OP_LOCAL,restype,mi->body->local_count+1+resultpos,0,0));
				res = true;
			}
			else
				clearOperands(mi,localtypes,operandlist, defaultlocaltypes,nullptr);
			break;
		}
		case 0x66://getproperty
		{
			uint32_t t = code.peeku30FromPosition(pos);
			uint32_t argcount = argsneeded ? 1 : 0;
			if (jumptargets.find(pos) == jumptargets.end() && (argsneeded<=1) &&
					((uint32_t)mi->context->constant_pool.multinames[t].runtimeargs == argcount
					|| (!argsneeded && (operandlist.size() >= 1) && mi->context->constant_pool.multinames[t].runtimeargs == 1)
					))
			{
				// set optimized opcode to corresponding opcode with local result 
				mi->body->preloadedcode[preloadpos].data += opcode_jumpspace;
				mi->body->preloadedcode[preloadlocalpos].local_pos3 = mi->body->local_count+2+resultpos;
				operandlist.push_back(operands(OP_LOCAL,restype,mi->body->local_count+1+resultpos,0,0));
				res = true;
			}
			else
				clearOperands(mi,localtypes,operandlist, defaultlocaltypes,nullptr);
			break;
		}
		case 0x11://iftrue
		case 0x12://iffalse
			if ((!argsneeded || (argsneeded==1 && hasdup)) && (jumptargets.find(pos) == jumptargets.end()))
			{
				// set optimized opcode to corresponding opcode with local result 
				mi->body->preloadedcode[preloadpos].data += opcode_jumpspace;
				mi->body->preloadedcode[preloadlocalpos].local_pos3 = mi->body->local_count+2+resultpos;
				operandlist.push_back(operands(OP_LOCAL,restype,mi->body->local_count+1+resultpos,0,0));
				res = true;
			}
			else
				clearOperands(mi,localtypes,operandlist, defaultlocaltypes,nullptr);
			break;
		case 0x91://increment
		case 0x93://decrement
		case 0x95://typeof
		case 0x96://not
		case 0x6c://getslot
		case 0x73://convert_i
		case 0x74://convert_u
		case 0x75://convert_d
		case 0xc0://increment_i
		case 0xc1://decrement_i
		case 0x35://li8
		case 0x36://li16
		case 0x37://li32
		case 0x38://lf32
		case 0x39://lf64
			if (!argsneeded && (jumptargets.find(pos) == jumptargets.end()))
			{
				// set optimized opcode to corresponding opcode with local result 
				mi->body->preloadedcode[preloadpos].data += opcode_jumpspace;
				mi->body->preloadedcode[preloadlocalpos].local_pos3 = mi->body->local_count+2+resultpos;
				operandlist.push_back(operands(OP_LOCAL,restype,mi->body->local_count+1+resultpos,0,0));
				res = true;
			}
			else
				clearOperands(mi,localtypes,operandlist, defaultlocaltypes,nullptr);
			break;
		case 0x13://ifeq
		case 0x14://ifne
		case 0x15://iflt
		case 0x16://ifle
		case 0x17://ifgt
		case 0x18://ifge
		case 0x19://ifstricteq
		case 0x1a://ifstrictne
		case 0x6d://setslot
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
		case 0xab://equals
		case 0xad://lessthan
		case 0xae://lessequals
		case 0xaf://greaterthan
		case 0xb0://greaterequals
		case 0x3a://si8
		case 0x3b://si16
		case 0x3c://si32
		case 0x3d://sf32
		case 0x3e://sf64
			if ((argsneeded==1 || (!argsneeded && operandlist.size() > 0)) && (jumptargets.find(pos) == jumptargets.end()))
			{
				// set optimized opcode to corresponding opcode with local result 
				mi->body->preloadedcode[preloadpos].data += opcode_jumpspace;
				mi->body->preloadedcode[preloadlocalpos].local_pos3 = mi->body->local_count+2+resultpos;
				operandlist.push_back(operands(OP_LOCAL,restype,mi->body->local_count+1+resultpos,0,0));
				res = true;
			}
			else
				clearOperands(mi,localtypes,operandlist, defaultlocaltypes,nullptr);
			break;
		case 0x48://returnvalue
			if (!argsneeded && (jumptargets.find(pos) == jumptargets.end()))
			{
				// set optimized opcode to corresponding opcode with local result 
				mi->body->preloadedcode[preloadpos].data += opcode_jumpspace;
				mi->body->preloadedcode[preloadlocalpos].local_pos3 = mi->body->local_count+2+resultpos;
				operandlist.push_back(operands(OP_LOCAL,restype,mi->body->local_count+1+resultpos,0,0));
				res = true;
			}
			else
				clearOperands(mi,localtypes,operandlist, defaultlocaltypes,nullptr);
			break;
		case 0x10://jump
			// don't clear operandlist yet, because the jump may be skipped
			break;
		default:
			clearOperands(mi,localtypes,operandlist, defaultlocaltypes,nullptr);
			break;
	}
	return res;
}

bool setupInstructionOneArgumentNoResult(std::list<operands>& operandlist,method_info* mi,int operator_start,int opcode,memorystream& code,std::map<int32_t,int32_t>& oldnewpositions,std::map<int32_t,int32_t>& jumptargets, uint32_t startcodepos)
{
	bool hasoperands = jumptargets.find(startcodepos) == jumptargets.end() && operandlist.size() >= 1;
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
	return hasoperands;
}
bool setupInstructionTwoArgumentsNoResult(std::list<operands>& operandlist,method_info* mi,int operator_start,int opcode,memorystream& code,std::map<int32_t,int32_t>& oldnewpositions,std::map<int32_t,int32_t>& jumptargets)
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
	return hasoperands;
}
bool setupInstructionOneArgument(std::list<operands>& operandlist,method_info* mi,int operator_start,int opcode,memorystream& code,std::map<int32_t,int32_t>& oldnewpositions,std::map<int32_t,int32_t>& jumptargets,bool constantsallowed, bool useargument_for_skip, Class_base** localtypes, Class_base** defaultlocaltypes, Class_base* resulttype, uint32_t startcodepos, bool checkforlocalresult)
{
	bool hasoperands = jumptargets.find(startcodepos) == jumptargets.end() && operandlist.size() >= 1 && (constantsallowed || operandlist.back().type == OP_LOCAL);
	Class_base* skiptype = resulttype;
	if (hasoperands)
	{
		auto it = operandlist.end();
		(--it)->removeArg(mi);// remove arg1
		it = operandlist.end();
		mi->body->preloadedcode.push_back(operator_start);
		(--it)->fillCode(0,mi->body->preloadedcode[mi->body->preloadedcode.size()-1],mi->context,constantsallowed);
		if (useargument_for_skip)
			skiptype = it->objtype;
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
				if (!constantsallowed || skiptype == Class<Integer>::getRef(mi->context->root->getSystemState()).getPtr())
				{
					oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
					code.readbyte();
				}
				break;
			case 0x74://convert_u
				if (!constantsallowed || skiptype == Class<UInteger>::getRef(mi->context->root->getSystemState()).getPtr())
				{
					oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
					code.readbyte();
				}
				break;
			case 0x75://convert_d
				if (!constantsallowed || skiptype == Class<Number>::getRef(mi->context->root->getSystemState()).getPtr()  || skiptype == Class<Integer>::getRef(mi->context->root->getSystemState()).getPtr() || skiptype == Class<UInteger>::getRef(mi->context->root->getSystemState()).getPtr())
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
	if (hasoperands && checkforlocalresult)
		checkForLocalResult(operandlist,mi,code,oldnewpositions,jumptargets,constantsallowed ? 2 : 1,localtypes,resulttype, defaultlocaltypes);
	else
		clearOperands(mi,localtypes,operandlist, defaultlocaltypes,nullptr);
	return hasoperands;
}

bool setupInstructionTwoArguments(std::list<operands>& operandlist,method_info* mi,int operator_start,int opcode,memorystream& code,std::map<int32_t,int32_t>& oldnewpositions,std::map<int32_t,int32_t>& jumptargets, bool skip_conversion,bool cancollapse,bool checklocalresult, Class_base** localtypes, Class_base** defaultlocaltypes, uint32_t startcodepos)
{
	bool hasoperands = jumptargets.find(startcodepos) == jumptargets.end() && operandlist.size() >= 2;
	Class_base* resulttype = nullptr;
	if (hasoperands)
	{
		auto it = operandlist.end();
		if (cancollapse && (--it)->type != OP_LOCAL && (--it)->type != OP_LOCAL) // two constants means we can compute the result now and use it
		{
			it =operandlist.end();
			(--it)->removeArg(mi);// remove arg2
			(--it)->removeArg(mi);// remove arg1
			it = operandlist.end();
			--it;
			asAtom* op2 = mi->context->getConstantAtom((it)->type,(it)->index);
			--it;
			asAtom* op1 = mi->context->getConstantAtom((it)->type,(it)->index);
			asAtom res = *op1;
			switch (operator_start)
			{
				case ABC_OP_OPTIMZED_SUBTRACT:
					asAtomHandler::subtract(res,mi->context->root->getSystemState(),*op2);
					break;
				case ABC_OP_OPTIMZED_MULTIPLY:
					asAtomHandler::multiply(res,mi->context->root->getSystemState(),*op2);
					break;
				case ABC_OP_OPTIMZED_DIVIDE:
					asAtomHandler::divide(res,mi->context->root->getSystemState(),*op2);
					break;
				case ABC_OP_OPTIMZED_MODULO:
					asAtomHandler::modulo(res,mi->context->root->getSystemState(),*op2);
					break;
				case ABC_OP_OPTIMZED_LSHIFT:
					asAtomHandler::lshift(res,mi->context->root->getSystemState(),*op2);
					break;
				case ABC_OP_OPTIMZED_RSHIFT:
					asAtomHandler::rshift(res,mi->context->root->getSystemState(),*op2);
					break;
				case ABC_OP_OPTIMZED_URSHIFT:
					asAtomHandler::urshift(res,mi->context->root->getSystemState(),*op2);
					break;
				case ABC_OP_OPTIMZED_BITAND:
					asAtomHandler::bit_and(res,mi->context->root->getSystemState(),*op2);
					break;
				case ABC_OP_OPTIMZED_BITOR:
					asAtomHandler::bit_or(res,mi->context->root->getSystemState(),*op2);
					break;
				case ABC_OP_OPTIMZED_BITXOR:
					asAtomHandler::bit_xor(res,mi->context->root->getSystemState(),*op2);
					break;
				default:
					LOG(LOG_ERROR,"setupInstructionTwoArguments: trying to collapse invalid opcode:"<<hex<<operator_start);
					break;
			}
			operandlist.pop_back();
			operandlist.pop_back();
			if (asAtomHandler::isObject(res))
				asAtomHandler::getObjectNoCheck(res)->setConstant();
			uint32_t value = mi->context->addCachedConstantAtom(res);
			mi->body->preloadedcode.push_back(ABC_OP_OPTIMZED_PUSHCACHEDCONSTANT);
			oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
			mi->body->preloadedcode.push_back(value);
			operandlist.push_back(operands(OP_CACHED_CONSTANT,asAtomHandler::getClass(res,mi->context->root->getSystemState()), value,2,mi->body->preloadedcode.size()-2));
			return true;
		}
		
		it =operandlist.end();
		(--it)->removeArg(mi);// remove arg2
		(--it)->removeArg(mi);// remove arg1
		it = operandlist.end();
		// optimized opcodes are in order CONSTANT/CONSTANT, LOCAL/CONSTANT, CONSTANT/LOCAL, LOCAL/LOCAL
		mi->body->preloadedcode.push_back(operator_start);
		(--it)->fillCode(1,mi->body->preloadedcode[mi->body->preloadedcode.size()-1],mi->context,true);
		Class_base* op1type = it->objtype;
		(--it)->fillCode(0,mi->body->preloadedcode[mi->body->preloadedcode.size()-1],mi->context,true);
		Class_base* op2type = it->objtype;
		oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
		operandlist.pop_back();
		operandlist.pop_back();
		switch (operator_start)
		{
			case ABC_OP_OPTIMZED_ADD:
				// if both operands are numeric, the result is always a number, so we can skip convert_d opcode
				skip_conversion = 
						(op1type == Class<Integer>::getRef(mi->context->root->getSystemState()).getPtr() || op1type == Class<UInteger>::getRef(mi->context->root->getSystemState()).getPtr() || op1type == Class<Number>::getRef(mi->context->root->getSystemState()).getPtr()) &&
						(op2type == Class<Integer>::getRef(mi->context->root->getSystemState()).getPtr() || op2type == Class<UInteger>::getRef(mi->context->root->getSystemState()).getPtr() || op2type == Class<Number>::getRef(mi->context->root->getSystemState()).getPtr());
				if (skip_conversion)
				{
					if (op1type == Class<Integer>::getRef(mi->context->root->getSystemState()).getPtr() && op2type == Class<Integer>::getRef(mi->context->root->getSystemState()).getPtr())
						resulttype = Class<Integer>::getRef(mi->context->root->getSystemState()).getPtr();
					else
						resulttype = Class<Number>::getRef(mi->context->root->getSystemState()).getPtr();
				}
				break;
			case ABC_OP_OPTIMZED_SUBTRACT:
			case ABC_OP_OPTIMZED_MULTIPLY:
			case ABC_OP_OPTIMZED_MODULO:
				if (op1type == Class<Integer>::getRef(mi->context->root->getSystemState()).getPtr() && op2type == Class<Integer>::getRef(mi->context->root->getSystemState()).getPtr())
					resulttype = Class<Integer>::getRef(mi->context->root->getSystemState()).getPtr();
				else
					resulttype = Class<Number>::getRef(mi->context->root->getSystemState()).getPtr();
				break;
			case ABC_OP_OPTIMZED_DIVIDE:
				resulttype = Class<Number>::getRef(mi->context->root->getSystemState()).getPtr();
				break;
			case ABC_OP_OPTIMZED_ADD_I:
			case ABC_OP_OPTIMZED_LSHIFT:
			case ABC_OP_OPTIMZED_RSHIFT:
			case ABC_OP_OPTIMZED_BITAND:
			case ABC_OP_OPTIMZED_BITOR:
			case ABC_OP_OPTIMZED_BITXOR:
			case ABC_OP_OPTIMZED_URSHIFT:
				resulttype = Class<Integer>::getRef(mi->context->root->getSystemState()).getPtr();
				break;
			default:
				break;
		}
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
	if (checklocalresult)
	{
		if (hasoperands)
			checkForLocalResult(operandlist,mi,code,oldnewpositions,jumptargets,4,localtypes, resulttype, defaultlocaltypes);
		else
			clearOperands(mi,localtypes,operandlist, defaultlocaltypes,nullptr);
	}
	return hasoperands;
}
void addCachedConstant(method_info* mi, asAtom& val,std::list<operands>& operandlist,std::map<int32_t,int32_t>& oldnewpositions,memorystream& code)
{
	if (asAtomHandler::isObject(val))
		asAtomHandler::getObject(val)->setConstant();
	uint32_t value = mi->context->addCachedConstantAtom(val);
	mi->body->preloadedcode.push_back(ABC_OP_OPTIMZED_PUSHCACHEDCONSTANT);
	oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
	mi->body->preloadedcode.push_back(value);
	operandlist.push_back(operands(OP_CACHED_CONSTANT,asAtomHandler::getClass(val,mi->context->root->getSystemState()),value,2,mi->body->preloadedcode.size()-2));
}
void setdefaultlocaltype(uint32_t t,method_info* mi,Class_base* c,Class_base** defaultlocaltypes, bool* defaultlocaltypescacheable)
{
	if (c==nullptr)
	{
		defaultlocaltypes[t] = nullptr;
		defaultlocaltypescacheable[t]=false;
		return;
	}
	if (t < mi->body->local_count+2 && defaultlocaltypescacheable[t])
	{
		if (defaultlocaltypes[t] == nullptr)
			defaultlocaltypes[t] = c;
		if (defaultlocaltypes[t] != c)
		{
			defaultlocaltypescacheable[t]=false;
			defaultlocaltypes[t]=nullptr;
		}
	}
}
void ABCVm::preloadFunction(SyntheticFunction* function)
{
	method_info* mi=function->mi;

	const int code_len=mi->body->code.size();
	std::map<int32_t,int32_t> oldnewpositions;
	std::map<int32_t,int32_t> jumppositions;
	std::map<int32_t,int32_t> jumpstartpositions;
	std::map<int32_t,int32_t> switchpositions;
	std::map<int32_t,int32_t> switchstartpositions;

	// first pass:
	// - store all jump target points
	// - compute types of the locals and detect if they don't change during execution
	std::map<int32_t,int32_t> jumptargets;
	Class_base* defaultlocaltypes[mi->body->local_count+2];
	bool defaultlocaltypescacheable[mi->body->local_count+2];
	defaultlocaltypes[0]= function->inClass;
	defaultlocaltypescacheable[0]=true;
	for (uint32_t i = 1; i < mi->body->local_count+2; i++)
	{
		defaultlocaltypes[i]=nullptr;
		defaultlocaltypescacheable[i]=true;
		if (mi->needsArgs() && i == mi->numArgs()+1) // don't cache argument array
			defaultlocaltypescacheable[i]=false;
		if (i > 0 && i <= mi->paramTypes.size() && dynamic_cast<const Class_base*>(mi->paramTypes[i-1]))
			defaultlocaltypes[i]= (Class_base*)mi->paramTypes[i-1]; // cache types of arguments
	}

	auto itex = mi->body->exceptions.begin();
	while (itex != mi->body->exceptions.end())
	{
		// add exception jump targets
		jumptargets[(int32_t)itex->target+1]=1;
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
	Class_base* currenttype=nullptr;
	memorystream codejumps(mi->body->code.data(), code_len);
	while(!codejumps.atend())
	{
		uint8_t opcode = codejumps.readbyte();
		if (simple_getter_opcode_pos != UINT32_MAX && opcode && opcode == simple_getter_opcodes[simple_getter_opcode_pos])
			++simple_getter_opcode_pos;
		else
			simple_getter_opcode_pos = UINT32_MAX;
		if (simple_setter_opcode_pos != UINT32_MAX && opcode && opcode == simple_setter_opcodes[simple_setter_opcode_pos])
			++simple_setter_opcode_pos;
		else
			simple_setter_opcode_pos = UINT32_MAX;
		
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
			case 0x60://getlex
			case 0x61://setproperty
			case 0x62://getlocal
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
				currenttype=nullptr;
				break;
			case 0x80://coerce
			{
				uint32_t t = codejumps.readu30();
				multiname* name =  mi->context->getMultinameImpl(asAtomHandler::nullAtom,nullptr,t,false);
				if (name->isStatic)
				{
					const Type* tp = Type::getTypeFromMultiname(name, mi->context);
					const Class_base* cls =dynamic_cast<const Class_base*>(tp);
					currenttype = (Class_base*)cls;
				}
				else
					currenttype=nullptr;
				break;
			}
			case 0xf0://debugline
			case 0xf1://debugfile
			case 0xf2://bkptline
				codejumps.readu30();
				break;
			case 0x82://coerce_a
				break;
			case 0x85://coerce_s
				currenttype=Class<ASString>::getRef(function->getSystemState()).getPtr();
				break;
			case 0x73://convert_i
				currenttype=Class<Integer>::getRef(function->getSystemState()).getPtr();
				break;
			case 0x74://convert_u
				currenttype=Class<UInteger>::getRef(function->getSystemState()).getPtr();
				break;
			case 0x75://convert_d
				currenttype=Class<Number>::getRef(function->getSystemState()).getPtr();
				break;
			case 0x76://convert_b
				currenttype=Class<Boolean>::getRef(function->getSystemState()).getPtr();
				break;
			case 0x2c://pushstring
				codejumps.readu30();
				currenttype=Class<ASString>::getRef(function->getSystemState()).getPtr();
				break;
			case 0x2d://pushint
				codejumps.readu30();
				currenttype=Class<Integer>::getRef(function->getSystemState()).getPtr();
				break;
			case 0x2e://pushuint
				codejumps.readu30();
				currenttype=Class<UInteger>::getRef(function->getSystemState()).getPtr();
				break;
			case 0x2f://pushdouble
				codejumps.readu30();
				currenttype=Class<Number>::getRef(function->getSystemState()).getPtr();
				break;
			case 0x31://pushnamespace
				codejumps.readu30();
				currenttype=Class<Namespace>::getRef(function->getSystemState()).getPtr();
				break;
			case 0x63://setlocal
			{
				uint32_t t = codejumps.readu30();
				setdefaultlocaltype(t,mi,currenttype,defaultlocaltypes,defaultlocaltypescacheable);
				currenttype=nullptr;
				break;
			}
			case 0xd4://setlocal_0
			case 0xd5://setlocal_1
			case 0xd6://setlocal_2
			case 0xd7://setlocal_3
				setdefaultlocaltype(opcode-0xd4,mi,currenttype,defaultlocaltypes,defaultlocaltypescacheable);
				currenttype=nullptr;
				break;
			case 0x92://inclocal
			case 0x94://declocal
			{
				uint32_t t = codejumps.readu30();
				setdefaultlocaltype(t,mi,Class<Number>::getRef(function->getSystemState()).getPtr(),defaultlocaltypes,defaultlocaltypescacheable);
				currenttype=nullptr;
				break;
			}
			case 0xc2://inclocal_i
			case 0xc3://declocal_i
			{
				uint32_t t = codejumps.readu30();
				setdefaultlocaltype(t,mi,Class<Integer>::getRef(function->getSystemState()).getPtr(),defaultlocaltypes,defaultlocaltypescacheable);
				currenttype=nullptr;
				break;
			}
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
				jumptargets[codejumps.reads24()+codejumps.tellg()+1]++;
				currenttype=nullptr;
				break;
			case 0x1b://lookupswitch
			{
				int32_t p = codejumps.tellg();
				jumptargets[p+codejumps.reads24()]++;
				uint32_t count = codejumps.readu30();
				for(unsigned int i=0;i<count+1;i++)
				{
					jumptargets[p+codejumps.reads24()]++;
				}
				currenttype=nullptr;
				break;
			}
			case 0x24://pushbyte
			{
				codejumps.readbyte();
				currenttype=Class<Integer>::getRef(function->getSystemState()).getPtr();
				break;
			}
			case 0x25://pushshort
			{
				codejumps.readu32();
				currenttype=Class<Integer>::getRef(function->getSystemState()).getPtr();
				break;
			}
			case 0x26://pushtrue
			case 0x27://pushfalse
			{
				currenttype=Class<Boolean>::getRef(function->getSystemState()).getPtr();
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
				currenttype=nullptr;
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
			case 0x09://label
			case 0x2a://dup
				break;
			default:
				currenttype=nullptr;
				break;
		}
	}
	
	// second pass: use optimized opcode version if it doesn't interfere with a jump target
	std::list<operands> operandlist;
	Class_base* localtypes[mi->body->local_count+2];
	Class_base* lastlocalresulttype=nullptr;
	clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
	memorystream code(mi->body->code.data(), code_len);
	std::list<bool> scopelist;
	int dup_indicator=0;
	while(!code.atend())
	{
		oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
		uint8_t opcode = code.readbyte();
		//LOG(LOG_INFO,"preload opcode:"<<code.tellg()-1<<" "<<operandlist.size()<<" "<<hex<<(int)opcode);
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


		switch(opcode)
		{
			case 0x04://getsuper
			case 0x05://setsuper
			case 0x06://dxns
			case 0x08://kill
			case 0x40://newfunction
			case 0x41://call
			case 0x53://constructgenerictype
			case 0x55://newobject
			case 0x56://newarray
			case 0x58://newclass
			case 0x59://getdescendants
			case 0x5a://newcatch
			case 0x5f://finddef
			case 0x6a://deleteproperty
			case 0x6e://getglobalSlot
			case 0x6f://setglobalSlot
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
				clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				break;
			}
			case 0x09://label
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				break;
			case 0x1c://pushwith
				scopelist.push_back(true);
				mi->body->preloadedcode.push_back((uint32_t)opcode);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				break;
			case 0x1d://popscope
				scopelist.pop_back();
				mi->body->preloadedcode.push_back((uint32_t)opcode);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				break;
			case 0x49://constructsuper
			{
				int32_t p = code.tellg();
				if (jumptargets.find(p) != jumptargets.end())
					clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				uint32_t t =code.readu30();
				if (function->inClass && t==0) // class method with 0 params
				{
					if (function->inClass->super.getPtr() == Class<ASObject>::getClass(function->getSystemState()) // super class is ASObject, so constructsuper can be skipped
							&& !operandlist.empty())
					{
						mi->body->preloadedcode.pop_back();
						operandlist.pop_back();
					}
					else
						setupInstructionOneArgumentNoResult(operandlist,mi,ABC_OP_OPTIMZED_CONSTRUCTSUPER,opcode,code,oldnewpositions, jumptargets,p);
				}
				else
				{
					mi->body->preloadedcode.push_back((uint32_t)opcode);
					mi->body->preloadedcode.push_back(t);
					clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				}
				break;
			}
			case 0x5e://findproperty
			case 0x5d://findpropstrict
			{
				int32_t p = code.tellg();
				if (jumptargets.find(p) != jumptargets.end())
					clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				uint32_t t =code.readu30();
				asAtom o=asAtomHandler::invalidAtom;
				if (function->inClass && function->inClass->isSealed && (scopelist.begin()==scopelist.end() || !scopelist.back())) // class method
				{
					bool found = false;
					multiname* name=mi->context->getMultiname(t,nullptr);
					if (name && name->isStatic)
					{
						bool isborrowed = false;
						variable* v = nullptr;
						Class_base* cls = function->inClass;
						do
						{
							v = cls->findVariableByMultiname(*name,cls,nullptr,&isborrowed);
							cls = cls->super.getPtr();
						}
						while (!v && cls && cls->isSealed);
						if (v)
						{
							found =true;
							if (!function->isStatic && (isborrowed || v->kind == INSTANCE_TRAIT))
							{
								mi->body->preloadedcode.push_back((uint32_t)0xd0); // convert to getlocal_0
								oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
								operandlist.push_back(operands(OP_LOCAL,function->inClass, 0,1,mi->body->preloadedcode.size()-1));
								break;
							}
						}
						if(!function->func_scope.isNull()) // check scope stack
						{
							auto it=function->func_scope->scope.rbegin();
							while(it!=function->func_scope->scope.rend())
							{
								if (it->considerDynamic)
								{
									found = true;
									break;
								}
								ASObject* obj = asAtomHandler::toObject(it->object,mi->context->root->getSystemState());
								v = obj->findVariableByMultiname(*name,obj->is<Class_base>() ? obj->as<Class_base>() : nullptr,nullptr,&isborrowed);
									
								if (v)
								{
									found = true;
									break;
								}
								++it;
							}
						}
						if (!found)
						{
							ASObject* target;
							if (mi->context->root->applicationDomain->findTargetByMultiname(*name, target))
							{
								o=asAtomHandler::fromObject(target);
								addCachedConstant(mi, o,operandlist,oldnewpositions,code);
								break;
							}
						}
						//LOG(LOG_ERROR,"findpropstrict preload inclass not found:"<<*name<<"|"<<function->isStatic<<"|"<<function->inClass->isInitialized()<<"|"<<function->inClass->toDebugString());
					}
				}
				if(asAtomHandler::isInvalid(o))
				{
					mi->body->preloadedcode.push_back((uint32_t)opcode);
					oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
					mi->body->preloadedcode.push_back(t);
					clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				}
				break;
			}
			case 0x60://getlex
			{
				int32_t p = code.tellg();
				if (jumptargets.find(p) != jumptargets.end())
					clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				uint32_t t =code.readu30();
				multiname* name=mi->context->getMultiname(t,NULL);
				if (!name || !name->isStatic)
					throwError<VerifyError>(kIllegalOpMultinameError,"getlex","multiname not static");
				if (function->inClass) // class method
				{
					if ((simple_getter_opcode_pos != UINT32_MAX) // function is simple getter
							&& function->inClass->isFinal // TODO also enable optimization for classes where it is guarranteed that the method is not overridden in derived classes
							&& function->inClass->getInterfaces().empty()) // class doesn't implement any interfaces
					{
						variable* v = function->inClass->findVariableByMultiname(*name,nullptr);
						if (v && v->kind == TRAIT_KIND::INSTANCE_TRAIT)
							function->simpleGetterOrSetterName = name;
					}
					asAtom o=asAtomHandler::invalidAtom;
					if(!function->func_scope.isNull()) // check scope stack
					{
						auto it=function->func_scope->scope.rbegin();
						while(it!=function->func_scope->scope.rend())
						{
							GET_VARIABLE_OPTION opt= (GET_VARIABLE_OPTION)(FROM_GETLEX | DONT_CALL_GETTER | NO_INCREF);
							if(!it->considerDynamic)
								opt=(GET_VARIABLE_OPTION)(opt | SKIP_IMPL);
							else
								break;
							asAtomHandler::toObject(it->object,mi->context->root->getSystemState())->getVariableByMultiname(o,*name, opt);
							if(asAtomHandler::isValid(o))
								break;
							++it;
						}
					}
					if(asAtomHandler::isInvalid(o))
					{
						ASObject* var = mi->context->root->applicationDomain->getVariableByMultinameOpportunistic(*name);
						if (var)
							o = asAtomHandler::fromObject(var);
					}
					if (asAtomHandler::is<Class_base>(o))
					{
						if (asAtomHandler::as<Class_base>(o)->isConstructed())
						{
							addCachedConstant(mi, o,operandlist,oldnewpositions,code);
							break;
						}
					}
					else if (function->inClass->super.isNull()) //TODO slot access for derived classes
					{
						uint32_t slotid = function->inClass->findInstanceSlotByMultiname(name);
						if (slotid != UINT32_MAX)
						{
							uint32_t num = slotid<<OPCODE_SIZE | ABC_OP_OPTIMZED_GETLEX_FROMSLOT;
							mi->body->preloadedcode.push_back(num);
							oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
							checkForLocalResult(operandlist,mi,code,oldnewpositions,jumptargets,1,localtypes,nullptr, defaultlocaltypes);
							break;
						}
					}
				}
				mi->body->preloadedcode.push_back(ABC_OP_OPTIMZED_GETLEX);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				mi->body->preloadedcode[mi->body->preloadedcode.size()-1].cachedmultiname2=name;
				if (!checkForLocalResult(operandlist,mi,code,oldnewpositions,jumptargets,0,localtypes,nullptr, defaultlocaltypes))
				{
					// no local result possible, use standard operation
					mi->body->preloadedcode[mi->body->preloadedcode.size()-1].data=(uint32_t)opcode;
					clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				}
				break;
			}
			case 0x61://setproperty
			case 0x68://initproperty
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
						{
							multiname* name =  mi->context->getMultinameImpl(asAtomHandler::nullAtom,NULL,t,false);
							if (operandlist.size() > 1)
							{
								auto it = operandlist.rbegin();
								Class_base* contenttype = it->objtype;
								it++;
								if (canCallFunctionDirect((*it),name))
								{
									variable* v = it->objtype->getBorrowedVariableByMultiname(*name);
									if (v)
									{
										if (asAtomHandler::is<SyntheticFunction>(v->setter))
										{
											setupInstructionTwoArgumentsNoResult(operandlist,mi,ABC_OP_OPTIMZED_CALLFUNCTIONSYNTHETIC_ONEARG_VOID,opcode,code,oldnewpositions, jumptargets);
											if (contenttype)
											{
												SyntheticFunction* f = asAtomHandler::as<SyntheticFunction>(v->setter);
												if (!f->getMethodInfo()->returnType)
													f->checkParamTypes();
												if (f->getMethodInfo()->paramTypes.size() && f->canSkipCoercion(0,contenttype))
													mi->body->preloadedcode.at(mi->body->preloadedcode.size()-1).data |= ABC_OP_COERCED;
											}
											mi->body->preloadedcode.at(mi->body->preloadedcode.size()-1).cacheobj3 = asAtomHandler::getObject(v->setter);
											break;
										}
										if (asAtomHandler::is<Function>(v->setter))
										{
											setupInstructionTwoArgumentsNoResult(operandlist,mi,ABC_OP_OPTIMZED_CALLFUNCTIONBUILTIN_ONEARG_VOID,opcode,code,oldnewpositions, jumptargets);
											mi->body->preloadedcode.at(mi->body->preloadedcode.size()-1).cacheobj3 = asAtomHandler::getObject(v->setter);
											break;
										}
									}
								}
								if ((it->type == OP_LOCAL || it->type == OP_CACHED_CONSTANT) && it->objtype && !it->objtype->isInterface && it->objtype->isInitialized())
								{
									if (it->objtype->is<Class_inherit>())
										it->objtype->as<Class_inherit>()->checkScriptInit();
									// check if we can replace setProperty by setSlot
									asAtom o = asAtomHandler::invalidAtom;
									it->objtype->getInstance(o,false,nullptr,0);
									it->objtype->setupDeclaredTraits(asAtomHandler::getObject(o),false);
		
									variable* v = asAtomHandler::getObject(o)->findVariableByMultiname(*name,nullptr);
									if (v && v->slotid)
									{
										// we can skip coercing when setting the slot value if
										// - contenttype is the same as the variable type or
										// - variable type is any or void or
										// - contenttype is subclass of variable type or
										// - contenttype is numeric and variable type is Number
										int operator_start = v->isResolved && ((contenttype && contenttype == v->type) || !dynamic_cast<const Class_base*>(v->type)) ? ABC_OP_OPTIMZED_SETSLOT_NOCOERCE : ABC_OP_OPTIMZED_SETSLOT;
										if (contenttype && v->isResolved && contenttype != v->type && dynamic_cast<const Class_base*>(v->type))
										{
											Class_base* vtype = (Class_base*)v->type;
											if (contenttype->isSubClass(vtype)
												|| (( contenttype == Class<Number>::getRef(function->getSystemState()).getPtr() ||
													   contenttype == Class<Integer>::getRef(function->getSystemState()).getPtr() ||
													   contenttype == Class<UInteger>::getRef(function->getSystemState()).getPtr()) &&
													 ( vtype == Class<Number>::getRef(function->getSystemState()).getPtr()))
												)
												operator_start = ABC_OP_OPTIMZED_SETSLOT_NOCOERCE;
										}
										setupInstructionTwoArgumentsNoResult(operandlist,mi,operator_start,opcode,code,oldnewpositions, jumptargets);
										mi->body->preloadedcode.at(mi->body->preloadedcode.size()-1).data |=v->slotid<<OPCODE_SIZE;
										ASATOM_DECREF(o);
										break;
									}
									else
										ASATOM_DECREF(o);
								}
							}
							
							setupInstructionTwoArgumentsNoResult(operandlist,mi,ABC_OP_OPTIMZED_SETPROPERTY_STATICNAME,opcode,code,oldnewpositions, jumptargets);
							mi->body->preloadedcode.push_back(t);
							mi->body->preloadedcode.at(mi->body->preloadedcode.size()-1).cachedmultiname2 =name;
							mi->body->preloadedcode.at(mi->body->preloadedcode.size()-1).local_pos3 = opcode; // use local_pos3 as indicator for setproperty/initproperty
							if ((simple_setter_opcode_pos != UINT32_MAX) // function is simple setter
									&& function->inClass->isFinal // TODO also enable optimization for classes where it is guarranteed that the method is not overridden in derived classes
									&& function->inClass->getInterfaces().empty()) // class doesn't implement any interfaces
							{
								variable* v = function->inClass->findVariableByMultiname(*name,nullptr);
								if (v && v->kind == TRAIT_KIND::INSTANCE_TRAIT)
									function->simpleGetterOrSetterName = name;
							}
							break;
						}
						default:
							mi->body->preloadedcode.push_back((uint32_t)opcode);
							mi->body->preloadedcode.push_back(t);
							clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
							break;
					}
				}
				else
				{
					mi->body->preloadedcode.push_back((uint32_t)opcode);
					mi->body->preloadedcode.push_back(t);
					clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				}
				break;
			}
			case 0x62://getlocal
			{
				int32_t p = code.tellg();
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size()+1;
				uint32_t value =code.readu30();
				assert_and_throw(value < mi->body->local_count);
				uint32_t num = value<<OPCODE_SIZE | opcode;
				mi->body->preloadedcode.push_back(num);
				if (jumptargets.find(p) != jumptargets.end())
					clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				operandlist.push_back(operands(OP_LOCAL,localtypes[value],value,1,mi->body->preloadedcode.size()-1));
				break;
			}
			case 0x63://setlocal
			{
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size()+1;
				uint32_t value =code.readu30();
				assert_and_throw(value < mi->body->local_count);
				uint32_t num = value<<OPCODE_SIZE | opcode;
				mi->body->preloadedcode.push_back(num);
				clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				break;
			}
			case 0x65://getscopeobject
			{
				mi->body->preloadedcode.push_back((uint32_t)opcode);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				uint32_t t =code.readu30();
				if (checkForLocalResult(operandlist,mi,code,oldnewpositions,jumptargets,0,localtypes,nullptr, defaultlocaltypes))
				{
					mi->body->preloadedcode.at(mi->body->preloadedcode.size()-1).data=ABC_OP_OPTIMZED_GETSCOPEOBJECT_LOCALRESULT;
					mi->body->preloadedcode.at(mi->body->preloadedcode.size()-1).data|=t<<OPCODE_SIZE;
				}
				else
				{
					mi->body->preloadedcode.push_back(t);
					clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				}
				break;
			}
			case 0x6c://getslot
			{
				int32_t p = code.tellg();
				if (jumptargets.find(p) != jumptargets.end())
					clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				int32_t t =code.readu30();
				Class_base* resulttype = nullptr;
				if (!mi->needsActivation() && operandlist.size() > 0)
				{
					auto it = operandlist.rbegin();
					if (it->type != OP_LOCAL)
					{
						asAtom* o = mi->context->getConstantAtom(it->type,it->index);
						if (asAtomHandler::getObject(*o) && asAtomHandler::getObject(*o)->getSlotKind(t) == TRAIT_KIND::CONSTANT_TRAIT)
						{
							asAtom cval = asAtomHandler::getObject(*o)->getSlot(t);
							if (!asAtomHandler::isNull(cval) && !asAtomHandler::isUndefined(cval))
							{
								it->removeArg(mi);
								operandlist.pop_back();
								addCachedConstant(mi, cval,operandlist,oldnewpositions,code);
								break;
							}
						}
					}
					if (it->objtype && !it->objtype->isInterface && it->objtype->isInitialized())
					{
						if (it->objtype->is<Class_inherit>())
							it->objtype->as<Class_inherit>()->checkScriptInit();
						// check if we can replace getProperty by getSlot
						asAtom o = asAtomHandler::invalidAtom;
						it->objtype->getInstance(o,false,nullptr,0);
						it->objtype->setupDeclaredTraits(asAtomHandler::getObject(o));
						ASObject* obj =asAtomHandler::getObject(o);
						if (obj)
							resulttype = obj->getSlotType(t);
					}
				}
				setupInstructionOneArgument(operandlist,mi,ABC_OP_OPTIMZED_GETSLOT,opcode,code,oldnewpositions, jumptargets,true,false,localtypes, defaultlocaltypes,resulttype,p,true);
				mi->body->preloadedcode.push_back(t);
				break;
			}
			case 0x6d://setslot
			{
				int32_t p = code.tellg();
				if (jumptargets.find(p) != jumptargets.end())
					clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				int32_t t =code.readu30();
				if (setupInstructionTwoArgumentsNoResult(operandlist,mi,ABC_OP_OPTIMZED_SETSLOT,opcode,code,oldnewpositions, jumptargets))
					mi->body->preloadedcode.at(mi->body->preloadedcode.size()-1).data |=t<<OPCODE_SIZE;
				else
					mi->body->preloadedcode.push_back(t);
				break;
			}
			case 0x80://coerce
			{
				int32_t p = code.tellg();
				int32_t t = code.readu30();
				multiname* name =  mi->context->getMultinameImpl(asAtomHandler::nullAtom,nullptr,t,false);
				bool skip = false;
				if (jumptargets.find(p) == jumptargets.end() &&
					(operandlist.size() > 0)
					)
				{
					assert_and_throw(name->isStatic);
					const Type* tp = Type::getTypeFromMultiname(name, mi->context);
					const Class_base* cls =dynamic_cast<const Class_base*>(tp);
					if (operandlist.back().type == OP_LOCAL)
					{
						if (operandlist.back().objtype == Class<Number>::getRef(mi->context->root->getSystemState()).getPtr() ||
								 operandlist.back().objtype == Class<Integer>::getRef(mi->context->root->getSystemState()).getPtr() ||
								 operandlist.back().objtype == Class<UInteger>::getRef(mi->context->root->getSystemState()).getPtr() ||
								 operandlist.back().objtype == Class<Boolean>::getRef(mi->context->root->getSystemState()).getPtr())
							skip = cls == Class<Number>::getRef(mi->context->root->getSystemState()).getPtr() || cls == Class<Integer>::getRef(mi->context->root->getSystemState()).getPtr() || cls == Class<UInteger>::getRef(mi->context->root->getSystemState()).getPtr();
						else
							skip = cls && operandlist.back().objtype && cls->isSubClass(operandlist.back().objtype);
					}
					else
					{
						asAtom o = *mi->context->getConstantAtom(operandlist.back().type,operandlist.back().index);
						if (asAtomHandler::isValid(o))
						{
							if (cls)
							{
								switch (asAtomHandler::getObjectType(o))
								{
									case T_NULL:
										skip = cls != Class<Number>::getRef(mi->context->root->getSystemState()).getPtr() &&
												cls != Class<Integer>::getRef(mi->context->root->getSystemState()).getPtr() &&
												cls != Class<UInteger>::getRef(mi->context->root->getSystemState()).getPtr() &&
												cls != Class<Boolean>::getRef(mi->context->root->getSystemState()).getPtr();
										break;
									case T_INTEGER:
									case T_NUMBER:
									case T_UINTEGER:
										skip = cls == Class<Number>::getRef(mi->context->root->getSystemState()).getPtr() || cls == Class<Integer>::getRef(mi->context->root->getSystemState()).getPtr() || cls == Class<UInteger>::getRef(mi->context->root->getSystemState()).getPtr();
										break;
									default:
										skip= operandlist.back().objtype && cls->isSubClass(operandlist.back().objtype);
										break;
								}
							}
						}
					}
				}
				if (!skip)
				{
					mi->body->preloadedcode.push_back((uint32_t)opcode);
					oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
					mi->body->preloadedcode.push_back(t);
					clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				}
				break;
			}
			case 0xd0://getlocal_0
			case 0xd1://getlocal_1
			case 0xd2://getlocal_2
			case 0xd3://getlocal_3
			{
				int32_t p = code.tellg();
				if (p==1 && opcode == 0xd0) //getlocal_0
				{
					if (code.peekbyte() == 0x30) // pushscope
					{
						// function begins with getlocal_0 and pushscope, can be skipped
						oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
						code.readbyte();
						oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
						mi->needsscope=true;
						break;
					}
				}
				assert_and_throw(((uint32_t)opcode)-0xd0 < mi->body->local_count);
				mi->body->preloadedcode.push_back((uint32_t)opcode);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				if (jumptargets.find(p) != jumptargets.end())
					clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				operandlist.push_back(operands(OP_LOCAL,localtypes[((uint32_t)opcode)-0xd0],((uint32_t)opcode)-0xd0,1,mi->body->preloadedcode.size()-1));
				break;
			}
			case 0x2a://dup
			{
				int32_t p = code.tellg();
				mi->body->preloadedcode.push_back((uint32_t)opcode);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				if (jumptargets.find(p) != jumptargets.end())
					clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				else if (operandlist.size() > 0)
				{
					operands op = operandlist.back();
					operandlist.push_back(operands(op.type,op.objtype,op.index,1,mi->body->preloadedcode.size()-1));
					dup_indicator=1;
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
				clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
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
					clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
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
				{
					if (dup_indicator)
					{
						operandlist.back().removeArg(mi);
						operandlist.pop_back();
						setupInstructionOneArgumentNoResult(operandlist,mi,ABC_OP_OPTIMZED_IFTRUE_DUP,opcode,code,oldnewpositions, jumptargets,p);
					}
					else
						setupInstructionOneArgumentNoResult(operandlist,mi,ABC_OP_OPTIMZED_IFTRUE,opcode,code,oldnewpositions, jumptargets,p);
				}
				else
					mi->body->preloadedcode.push_back((uint32_t)opcode);
				jumppositions[mi->body->preloadedcode.size()-1] = code.reads24();
				jumpstartpositions[mi->body->preloadedcode.size()-1] = code.tellg();
				clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				break;
			}
			case 0x12://iffalse
			{
				int32_t p = code.tellg();
				if (jumptargets.find(p) == jumptargets.end())
				{
					if (dup_indicator)
					{
						operandlist.back().removeArg(mi);
						operandlist.pop_back();
						setupInstructionOneArgumentNoResult(operandlist,mi,ABC_OP_OPTIMZED_IFFALSE_DUP,opcode,code,oldnewpositions, jumptargets,p);
					}
					else
						setupInstructionOneArgumentNoResult(operandlist,mi,ABC_OP_OPTIMZED_IFFALSE,opcode,code,oldnewpositions, jumptargets,p);
				}
				else
					mi->body->preloadedcode.push_back((uint32_t)opcode);
				jumppositions[mi->body->preloadedcode.size()-1] = code.reads24();
				jumpstartpositions[mi->body->preloadedcode.size()-1] = code.tellg();
				clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				break;
			}
			case 0x13://ifeq
				setupInstructionTwoArgumentsNoResult(operandlist,mi,ABC_OP_OPTIMZED_IFEQ,opcode,code,oldnewpositions, jumptargets);
				jumppositions[mi->body->preloadedcode.size()-1] = code.reads24();
				jumpstartpositions[mi->body->preloadedcode.size()-1] = code.tellg();
				clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				break;
			case 0x14://ifne
				setupInstructionTwoArgumentsNoResult(operandlist,mi,ABC_OP_OPTIMZED_IFNE,opcode,code,oldnewpositions, jumptargets);
				jumppositions[mi->body->preloadedcode.size()-1] = code.reads24();
				jumpstartpositions[mi->body->preloadedcode.size()-1] = code.tellg();
				clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				break;
			case 0x15://iflt
				setupInstructionTwoArgumentsNoResult(operandlist,mi,ABC_OP_OPTIMZED_IFLT,opcode,code,oldnewpositions, jumptargets);
				jumppositions[mi->body->preloadedcode.size()-1] = code.reads24();
				jumpstartpositions[mi->body->preloadedcode.size()-1] = code.tellg();
				clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				break;
			case 0x16://ifle
				setupInstructionTwoArgumentsNoResult(operandlist,mi,ABC_OP_OPTIMZED_IFLE,opcode,code,oldnewpositions, jumptargets);
				jumppositions[mi->body->preloadedcode.size()-1] = code.reads24();
				jumpstartpositions[mi->body->preloadedcode.size()-1] = code.tellg();
				clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				break;
			case 0x17://ifgt
				setupInstructionTwoArgumentsNoResult(operandlist,mi,ABC_OP_OPTIMZED_IFGT,opcode,code,oldnewpositions, jumptargets);
				jumppositions[mi->body->preloadedcode.size()-1] = code.reads24();
				jumpstartpositions[mi->body->preloadedcode.size()-1] = code.tellg();
				clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				break;
			case 0x18://ifge
				setupInstructionTwoArgumentsNoResult(operandlist,mi,ABC_OP_OPTIMZED_IFGE,opcode,code,oldnewpositions, jumptargets);
				jumppositions[mi->body->preloadedcode.size()-1] = code.reads24();
				jumpstartpositions[mi->body->preloadedcode.size()-1] = code.tellg();
				clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				break;
			case 0x19://ifstricteq
				setupInstructionTwoArgumentsNoResult(operandlist,mi,ABC_OP_OPTIMZED_IFSTRICTEQ,opcode,code,oldnewpositions, jumptargets);
				jumppositions[mi->body->preloadedcode.size()-1] = code.reads24();
				jumpstartpositions[mi->body->preloadedcode.size()-1] = code.tellg();
				clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				break;
			case 0x1a://ifstrictne
				setupInstructionTwoArgumentsNoResult(operandlist,mi,ABC_OP_OPTIMZED_IFSTRICTNE,opcode,code,oldnewpositions, jumptargets);
				jumppositions[mi->body->preloadedcode.size()-1] = code.reads24();
				jumpstartpositions[mi->body->preloadedcode.size()-1] = code.tellg();
				clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
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
				clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				break;
			}
			case 0x20://pushnull
				mi->body->preloadedcode.push_back((uint32_t)opcode);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				if (jumptargets.find(code.tellg()) == jumptargets.end())
					operandlist.push_back(operands(OP_NULL, nullptr,0,1,mi->body->preloadedcode.size()-1));
				else
					clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
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
					operandlist.push_back(operands(OP_BYTE,Class<Integer>::getRef(mi->context->root->getSystemState()).getPtr(),index,2,mi->body->preloadedcode.size()-2));
				else
					clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
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
					operandlist.push_back(operands(OP_SHORT,Class<Integer>::getRef(mi->context->root->getSystemState()).getPtr(),index,2,mi->body->preloadedcode.size()-2));
				else
					clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				break;
			}
			case 0x26://pushtrue
			{
				mi->body->preloadedcode.push_back((uint32_t)opcode);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				if (jumptargets.find(code.tellg()) == jumptargets.end())
					operandlist.push_back(operands(OP_TRUE, Class<Boolean>::getRef(mi->context->root->getSystemState()).getPtr(),0,1,mi->body->preloadedcode.size()-1));
				else
					clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				break;
			}
			case 0x27://pushfalse
			{
				mi->body->preloadedcode.push_back((uint32_t)opcode);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				if (jumptargets.find(code.tellg()) == jumptargets.end())
					operandlist.push_back(operands(OP_FALSE, Class<Boolean>::getRef(mi->context->root->getSystemState()).getPtr(),0,1,mi->body->preloadedcode.size()-1));
				else
					clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				break;
			}
			case 0x28://pushnan
			{
				mi->body->preloadedcode.push_back((uint32_t)opcode);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				if (jumptargets.find(code.tellg()) == jumptargets.end())
					operandlist.push_back(operands(OP_NAN, Class<Number>::getRef(mi->context->root->getSystemState()).getPtr(),0,1,mi->body->preloadedcode.size()-1));
				else
					clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
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
					operandlist.push_back(operands(OP_STRING,Class<ASString>::getRef(mi->context->root->getSystemState()).getPtr(),value,2,mi->body->preloadedcode.size()-2));
				else
					clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
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
					operandlist.push_back(operands(OP_INTEGER,Class<Integer>::getRef(mi->context->root->getSystemState()).getPtr(),value,2,mi->body->preloadedcode.size()-2));
				else
					clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
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
					operandlist.push_back(operands(OP_UINTEGER,Class<UInteger>::getRef(mi->context->root->getSystemState()).getPtr(),value,2,mi->body->preloadedcode.size()-2));
				else
					clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
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
					operandlist.push_back(operands(OP_DOUBLE,Class<Number>::getRef(mi->context->root->getSystemState()).getPtr(),value,2,mi->body->preloadedcode.size()-2));
				else
					clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				break;
			}
			case 0x30://pushscope
				scopelist.push_back(false);
				setupInstructionOneArgumentNoResult(operandlist,mi,ABC_OP_OPTIMZED_PUSHSCOPE,opcode,code,oldnewpositions, jumptargets,code.tellg());
				break;
			case 0x31://pushnamespace
			{
				int32_t p = code.tellg();
				mi->body->preloadedcode.push_back((uint32_t)opcode);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				int32_t value = code.readu30();
				mi->body->preloadedcode.push_back(value);
				if (jumptargets.find(p) == jumptargets.end())
					operandlist.push_back(operands(OP_NAMESPACE,Class<Namespace>::getRef(mi->context->root->getSystemState()).getPtr(),value,2,mi->body->preloadedcode.size()-2));
				else
					clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				break;
			}
			case 0x32://hasnext2
			case 0x43://callmethod
			case 0x44://callstatic
			case 0x45://callsuper
			case 0x4c://callproplex
			case 0x4e://callsupervoid
			{
				mi->body->preloadedcode.push_back((uint32_t)opcode);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				mi->body->preloadedcode.push_back(code.readu30());
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				mi->body->preloadedcode.push_back(code.readu30());
				clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				break;
			}
			case 0x35://li8
				setupInstructionOneArgument(operandlist,mi,ABC_OP_OPTIMZED_LI8,opcode,code,oldnewpositions, jumptargets,true,true,localtypes, defaultlocaltypes, Class<Integer>::getRef(function->getSystemState()).getPtr(),code.tellg(),true);
				break;
			case 0x36://li16
				setupInstructionOneArgument(operandlist,mi,ABC_OP_OPTIMZED_LI16,opcode,code,oldnewpositions, jumptargets,true,true,localtypes, defaultlocaltypes, Class<Integer>::getRef(function->getSystemState()).getPtr(),code.tellg(),true);
				break;
			case 0x37://li32
				setupInstructionOneArgument(operandlist,mi,ABC_OP_OPTIMZED_LI32,opcode,code,oldnewpositions, jumptargets,true,true,localtypes, defaultlocaltypes, Class<Integer>::getRef(function->getSystemState()).getPtr(),code.tellg(),true);
				break;
			case 0x38://lf32
				setupInstructionOneArgument(operandlist,mi,ABC_OP_OPTIMZED_LF32,opcode,code,oldnewpositions, jumptargets,true,true,localtypes, defaultlocaltypes, Class<Integer>::getRef(function->getSystemState()).getPtr(),code.tellg(),true);
				break;
			case 0x39://lf64
				setupInstructionOneArgument(operandlist,mi,ABC_OP_OPTIMZED_LF64,opcode,code,oldnewpositions, jumptargets,true,true,localtypes, defaultlocaltypes, Class<Integer>::getRef(function->getSystemState()).getPtr(),code.tellg(),true);
				break;
			case 0x3a://si8
				setupInstructionTwoArgumentsNoResult(operandlist,mi,ABC_OP_OPTIMZED_SI8,opcode,code,oldnewpositions, jumptargets);
				break;
			case 0x3b://si16
				setupInstructionTwoArgumentsNoResult(operandlist,mi,ABC_OP_OPTIMZED_SI16,opcode,code,oldnewpositions, jumptargets);
				break;
			case 0x3c://si32
				setupInstructionTwoArgumentsNoResult(operandlist,mi,ABC_OP_OPTIMZED_SI32,opcode,code,oldnewpositions, jumptargets);
				break;
			case 0x3d://sf32
				setupInstructionTwoArgumentsNoResult(operandlist,mi,ABC_OP_OPTIMZED_SF32,opcode,code,oldnewpositions, jumptargets);
				break;
			case 0x3e://sf64
				setupInstructionTwoArgumentsNoResult(operandlist,mi,ABC_OP_OPTIMZED_SF64,opcode,code,oldnewpositions, jumptargets);
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
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size()+1;
				uint32_t argcount = code.readu30();
				if (jumptargets.find(p) == jumptargets.end())
				{
					switch (argcount)
					{
						case 0:
						{
							Class_base* restype = operandlist.size()> 0 ? operandlist.back().objtype : nullptr;
							if (!setupInstructionOneArgument(operandlist,mi,ABC_OP_OPTIMZED_CONSTRUCT_NOARGS,opcode,code,oldnewpositions, jumptargets,true,false,localtypes, defaultlocaltypes,restype,p,true))
								mi->body->preloadedcode.push_back(argcount);
							break;
						}
						default:
							// TODO handle construct with one or more arguments
							mi->body->preloadedcode.push_back((uint32_t)opcode);
							clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
							mi->body->preloadedcode.push_back(argcount);
							break;
					}
					break;
				}
				else
				{
					mi->body->preloadedcode.push_back((uint32_t)opcode);
					clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
					mi->body->preloadedcode.push_back(argcount);
				}
				break;
			}
			case 0x47: //returnvoid
			{
				mi->body->preloadedcode.push_back((uint32_t)opcode);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				// skip unreachable code
				while (!code.atend() && jumptargets.find(code.tellg()+1) == jumptargets.end())
					code.readbyte();
				break;
			}
			case 0x48://returnvalue
			{
				int32_t p = code.tellg();
				if (jumptargets.find(p) != jumptargets.end())
					clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				setupInstructionOneArgumentNoResult(operandlist,mi,ABC_OP_OPTIMZED_RETURNVALUE,opcode,code,oldnewpositions, jumptargets,p);
				// skip unreachable code
				while (!code.atend() && jumptargets.find(code.tellg()+1) == jumptargets.end())
					code.readbyte();
				break;
			}
			case 0x4a://constructprop
			{
				int32_t p = code.tellg();
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size()+1;
				uint32_t t = code.readu30();
				assert_and_throw(t < mi->context->constant_pool.multiname_count);
				uint32_t argcount = code.readu30();
				if (jumptargets.find(p) == jumptargets.end())
				{
					multiname* name =  mi->context->getMultinameImpl(asAtomHandler::nullAtom,nullptr,t,false);
					switch (mi->context->constant_pool.multinames[t].runtimeargs)
					{
						case 0:
							switch (argcount)
							{
								case 0:
								{
									Class_base* resulttype = nullptr;
									ASObject* constructor = nullptr;
									if (operandlist.size() > 0 && operandlist.back().type != OP_LOCAL)
									{
										// common case: constructprop called to create a class instance
										ASObject* a = asAtomHandler::getObject(*mi->context->getConstantAtom(operandlist.back().type,operandlist.back().index));
										if(a)
										{
											asAtom o;
											a->getVariableByMultiname(o,*name);
											constructor = asAtomHandler::getObject(o);
											if (constructor->is<Class_base>())
												resulttype = constructor->as<Class_base>();
											else if (constructor->is<IFunction>())
												resulttype = constructor->as<IFunction>()->getReturnType();
										}
									}
									if (setupInstructionOneArgument(operandlist,mi,ABC_OP_OPTIMZED_CONSTRUCTPROP_STATICNAME_NOARGS,opcode,code,oldnewpositions, jumptargets,true,false,localtypes, defaultlocaltypes,resulttype,p,true))
									{
										mi->body->preloadedcode.at(mi->body->preloadedcode.size()-1).cachedmultiname2 = name;
										mi->body->preloadedcode.push_back(argcount);
										mi->body->preloadedcode.at(mi->body->preloadedcode.size()-1).cacheobj2 = constructor;
									}
									else
									{
										mi->body->preloadedcode.push_back(t);
										mi->body->preloadedcode.push_back(argcount);
									}
									break;
								}
								default:
									// TODO handle constructprop with one or more arguments
									mi->body->preloadedcode.push_back((uint32_t)opcode);
									clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
									mi->body->preloadedcode.push_back(t);
									mi->body->preloadedcode.push_back(argcount);
									break;
							}
							break;
						default:
							mi->body->preloadedcode.push_back((uint32_t)opcode);
							clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
							mi->body->preloadedcode.push_back(t);
							mi->body->preloadedcode.push_back(argcount);
							break;
					}
				}
				else
				{
					mi->body->preloadedcode.push_back((uint32_t)opcode);
					clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
					mi->body->preloadedcode.push_back(t);
					mi->body->preloadedcode.push_back(argcount);
				}
				break;
			}
			case 0x46://callproperty
			case 0x4f://callpropvoid
			{
				int32_t p = code.tellg();
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size()+1;
				uint32_t t = code.readu30();
				assert_and_throw(t < mi->context->constant_pool.multiname_count);
				uint32_t argcount = code.readu30();
				if (opcode == 0x46 && code.peekbyte() == 0x29 && jumptargets.find(p+1) == jumptargets.end())
				{
					// callproperty is followed by pop
					opcode = 0x4f; // treat call as callpropvoid
					oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size()+1;
					code.readbyte(); // skip pop
				}
				// TODO optimize non-static multinames
				if (jumptargets.find(p) == jumptargets.end())
				{
					multiname* name =  mi->context->getMultinameImpl(asAtomHandler::nullAtom,nullptr,t,false);
					switch (mi->context->constant_pool.multinames[t].runtimeargs)
					{
						case 0:
							switch (argcount)
							{
								case 0:
									if (operandlist.size() > 0 && (operandlist.back().type == OP_LOCAL || operandlist.back().type == OP_CACHED_CONSTANT) && operandlist.back().objtype)
									{
										if (canCallFunctionDirect(operandlist.back(),name))
										{
											variable* v = operandlist.back().objtype->getBorrowedVariableByMultiname(*name);
											if (v && asAtomHandler::is<IFunction>(v->var))
											{
												Class_base* resulttype = v->isResolved && dynamic_cast<const Class_base*>(v->type) ? (Class_base*)v->type : nullptr;
												if (opcode == 0x46)
													setupInstructionOneArgument(operandlist,mi,ABC_OP_OPTIMZED_CALLFUNCTION_NOARGS ,opcode,code,oldnewpositions, jumptargets,true, false,localtypes, defaultlocaltypes,resulttype,p,true);
												else
													setupInstructionOneArgumentNoResult(operandlist,mi,ABC_OP_OPTIMZED_CALLFUNCTION_NOARGS_VOID,opcode,code,oldnewpositions, jumptargets,p);
												mi->body->preloadedcode.at(mi->body->preloadedcode.size()-1).cacheobj2 = asAtomHandler::getObject(v->var);
												break;
											}
											if (!operandlist.back().objtype->is<Class_inherit>())
												LOG(LOG_NOT_IMPLEMENTED,"missing result type for builtin method:"<<*name<<" "<<operandlist.back().objtype->toDebugString());
										}
									}
									if ((opcode == 0x4f && setupInstructionOneArgumentNoResult(operandlist,mi,ABC_OP_OPTIMZED_CALLPROPVOID_STATICNAME_NOARGS,opcode,code,oldnewpositions, jumptargets,p)) ||
									   ((opcode == 0x46 && setupInstructionOneArgument(operandlist,mi,ABC_OP_OPTIMZED_CALLPROPERTY_STATICNAME_NOARGS,opcode,code,oldnewpositions, jumptargets,true,false,localtypes, defaultlocaltypes,nullptr,p,true))))
									{
										mi->body->preloadedcode.push_back(t);
										mi->body->preloadedcode.at(mi->body->preloadedcode.size()-1).cachedmultiname2 = name;
									}
									else
									{
										mi->body->preloadedcode.at(mi->body->preloadedcode.size()-1).data= (opcode == 0x4f ? ABC_OP_OPTIMZED_CALLPROPVOID_STATICNAME_MULTIARGS:ABC_OP_OPTIMZED_CALLPROPERTY_STATICNAME_MULTIARGS) | (argcount<<OPCODE_SIZE) | ABC_OP_COERCED;
										clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
										mi->body->preloadedcode.at(mi->body->preloadedcode.size()-1).cachedmultiname2 = name;
									}
									break;
								case 1:
									if (opcode == 0x4f && operandlist.size() > 1)
									{
										auto it = operandlist.rbegin();
										Class_base* argtype = it->objtype;
										it++;
										if (canCallFunctionDirect((*it),name))
										{
											variable* v = it->objtype->getBorrowedVariableByMultiname(*name);
											if (v)
											{
												if (asAtomHandler::is<SyntheticFunction>(v->var))
												{
													setupInstructionTwoArgumentsNoResult(operandlist,mi,ABC_OP_OPTIMZED_CALLFUNCTIONSYNTHETIC_ONEARG_VOID,opcode,code,oldnewpositions, jumptargets);
													if (argtype)
													{
														SyntheticFunction* f = asAtomHandler::as<SyntheticFunction>(v->var);
														if (!f->getMethodInfo()->returnType)
															f->checkParamTypes();
														if (f->getMethodInfo()->paramTypes.size() && f->canSkipCoercion(0,argtype))
															mi->body->preloadedcode.at(mi->body->preloadedcode.size()-1).data |= ABC_OP_COERCED;
													}
													mi->body->preloadedcode.at(mi->body->preloadedcode.size()-1).cacheobj3 = asAtomHandler::getObject(v->var);
													break;
												}
												if (asAtomHandler::is<Function>(v->var))
												{
													setupInstructionTwoArgumentsNoResult(operandlist,mi,ABC_OP_OPTIMZED_CALLFUNCTIONBUILTIN_ONEARG_VOID,opcode,code,oldnewpositions, jumptargets);
													mi->body->preloadedcode.at(mi->body->preloadedcode.size()-1).cacheobj3 = asAtomHandler::getObject(v->var);
													break;
												}
											}
										}
									}
									if ((opcode == 0x4f && setupInstructionTwoArgumentsNoResult(operandlist,mi,ABC_OP_OPTIMZED_CALLPROPVOID_STATICNAME,opcode,code,oldnewpositions, jumptargets)) ||
									   ((opcode == 0x46 && setupInstructionTwoArguments(operandlist,mi,ABC_OP_OPTIMZED_CALLPROPERTY_STATICNAME,opcode,code,oldnewpositions, jumptargets,false,false,true,localtypes, defaultlocaltypes,p))))
									{
										mi->body->preloadedcode.push_back(t);
										mi->body->preloadedcode.at(mi->body->preloadedcode.size()-1).cachedmultiname2 = name;
									}
									else if (opcode == 0x46 && checkForLocalResult(operandlist,mi,code,oldnewpositions,jumptargets,0,localtypes,nullptr, defaultlocaltypes))
									{
										mi->body->preloadedcode.at(mi->body->preloadedcode.size()-1).data=ABC_OP_OPTIMZED_CALLPROPERTY_STATICNAME_LOCALRESULT;
										mi->body->preloadedcode.push_back(t);
										mi->body->preloadedcode.at(mi->body->preloadedcode.size()-1).cachedmultiname2 = name;
									}
									else
									{
										mi->body->preloadedcode.at(mi->body->preloadedcode.size()-1).data= (opcode == 0x4f ? ABC_OP_OPTIMZED_CALLPROPVOID_STATICNAME_MULTIARGS:ABC_OP_OPTIMZED_CALLPROPERTY_STATICNAME_MULTIARGS) | (argcount<<OPCODE_SIZE);
										clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
										mi->body->preloadedcode.at(mi->body->preloadedcode.size()-1).cachedmultiname2 = name;
									}
									break;
								default:
									if (operandlist.size() >= argcount)
									{
										mi->body->preloadedcode.push_back((uint32_t)(opcode == 0x4f ? ABC_OP_OPTIMZED_CALLPROPVOID_STATICNAME_MULTIARGS_CACHED:ABC_OP_OPTIMZED_CALLPROPERTY_STATICNAME_MULTIARGS_CACHED) | (argcount<<OPCODE_SIZE));
										mi->body->preloadedcode.at(mi->body->preloadedcode.size()-1).cachedmultiname2 = name;
										auto it = operandlist.rbegin();
										for(uint32_t i= 0; i < argcount; i++)
										{
											it->removeArg(mi);
											mi->body->preloadedcode.push_back(it->type<<OPCODE_SIZE);
											it->fillCode(0,mi->body->preloadedcode[mi->body->preloadedcode.size()-1],mi->context,false);
											it++;
										}
										if (operandlist.size() > argcount)
										{
											uint32_t oppos = mi->body->preloadedcode.size()-1-argcount;
											if (canCallFunctionDirect((*it),name))
											{
												variable* v = it->objtype->getBorrowedVariableByMultiname(*name);
												if (v)
												{
													if (asAtomHandler::is<SyntheticFunction>(v->var))
													{
														mi->body->preloadedcode.at(oppos).data = (opcode == 0x4f ? ABC_OP_OPTIMZED_CALLFUNCTIONSYNTHETIC_MULTIARGS_VOID : ABC_OP_OPTIMZED_CALLFUNCTIONSYNTHETIC_MULTIARGS) | (argcount<<OPCODE_SIZE);
														SyntheticFunction* f = asAtomHandler::as<SyntheticFunction>(v->var);
														if (!f->getMethodInfo()->returnType)
															f->checkParamTypes();
														if (f->getMethodInfo()->paramTypes.size() >= argcount)
														{
															bool skipcoerce=true;
															auto it2 = operandlist.rbegin();
															for(uint32_t i= 0; i < argcount; i++)
															{
																if (!f->canSkipCoercion(i,it2->objtype))
																{
																	skipcoerce=false;
																	break;
																}
																it2++;
															}
															if (skipcoerce)
															{
																mi->body->preloadedcode.at(oppos).data |= ABC_OP_COERCED;
															}
														}
														it->fillCode(0,mi->body->preloadedcode.at(oppos),mi->context,true);
														it->removeArg(mi);
														oppos = mi->body->preloadedcode.size()-1-argcount;
														mi->body->preloadedcode.at(oppos).cacheobj3 = asAtomHandler::getObject(v->var);
														clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
														if (opcode == 0x46)
															checkForLocalResult(operandlist,mi,code,oldnewpositions,jumptargets,2,localtypes,nullptr, defaultlocaltypes,oppos,mi->body->preloadedcode.size()-1);
														break;
													}
													else if (asAtomHandler::is<Function>(v->var))
													{
														mi->body->preloadedcode.at(oppos).data = (opcode == 0x4f ? ABC_OP_OPTIMZED_CALLFUNCTIONBUILTIN_MULTIARGS_VOID : ABC_OP_OPTIMZED_CALLFUNCTIONBUILTIN_MULTIARGS) | (argcount<<OPCODE_SIZE);
														it->fillCode(0,mi->body->preloadedcode.at(oppos),mi->context,true);
														it->removeArg(mi);
														oppos = mi->body->preloadedcode.size()-1-argcount;
														mi->body->preloadedcode.at(oppos).cacheobj3 = asAtomHandler::getObject(v->var);
														clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
														if (opcode == 0x46)
															checkForLocalResult(operandlist,mi,code,oldnewpositions,jumptargets,2,localtypes,nullptr, defaultlocaltypes,oppos,mi->body->preloadedcode.size()-1);
														break;
													}
												}
											}
											mi->body->preloadedcode.at(oppos).data = (opcode == 0x4f ? ABC_OP_OPTIMZED_CALLPROPVOID_STATICNAME_MULTIARGS_CACHED_CALLER:ABC_OP_OPTIMZED_CALLPROPERTY_STATICNAME_MULTIARGS_CACHED_CALLER) | (argcount<<OPCODE_SIZE);
											
											it->removeArg(mi);
											oppos = mi->body->preloadedcode.size()-1-argcount;
											if (it->fillCode(1,mi->body->preloadedcode.at(mi->body->preloadedcode.size()-1),mi->context,false))
												mi->body->preloadedcode.at(oppos).data++;
											clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
											if (opcode == 0x46)
												checkForLocalResult(operandlist,mi,code,oldnewpositions,jumptargets,2,localtypes,nullptr, defaultlocaltypes,oppos,mi->body->preloadedcode.size()-1);
											break;
										}
										clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
										if (opcode == 0x46)
											checkForLocalResult(operandlist,mi,code,oldnewpositions,jumptargets,1,localtypes,nullptr, defaultlocaltypes,mi->body->preloadedcode.size()-1-argcount,mi->body->preloadedcode.size()-1);
									}
									else
									{
										mi->body->preloadedcode.push_back((uint32_t)(opcode == 0x4f ? ABC_OP_OPTIMZED_CALLPROPVOID_STATICNAME_MULTIARGS:ABC_OP_OPTIMZED_CALLPROPERTY_STATICNAME_MULTIARGS) | (argcount<<OPCODE_SIZE));
										clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
										mi->body->preloadedcode.at(mi->body->preloadedcode.size()-1).cachedmultiname2 = name;
									}
									break;
							}
							break;
						default:
							mi->body->preloadedcode.push_back((uint32_t)opcode);
							clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
							mi->body->preloadedcode.push_back(t);
							mi->body->preloadedcode.push_back(argcount);
							break;
					}
				}
				else
				{
					mi->body->preloadedcode.push_back((uint32_t)opcode);
					clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
					mi->body->preloadedcode.push_back(t);
					mi->body->preloadedcode.push_back(argcount);
				}
				break;
			}
			case 0x64://getglobalscope
			{
				int32_t p = code.tellg();
				if (jumptargets.find(p) != jumptargets.end())
					clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				if (function->func_scope.getPtr() && (scopelist.begin()==scopelist.end() || !scopelist.back()))
				{
					asAtom ret = function->func_scope->scope.front().object;
					addCachedConstant(mi, ret,operandlist,oldnewpositions,code);
				}
				else
				{
					mi->body->preloadedcode.push_back((uint32_t)opcode);
					clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				}
				break;
			}
			case 0x66://getproperty
			{
				int32_t p = code.tellg();
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size()+1;
				uint32_t t = code.readu30();
				assert_and_throw(t < mi->context->constant_pool.multiname_count);
				bool addname = true;
				if (jumptargets.find(p) == jumptargets.end())
				{
					switch (mi->context->constant_pool.multinames[t].runtimeargs)
					{
						case 0:
						{
							multiname* name = mi->context->getMultinameImpl(asAtomHandler::nullAtom,nullptr,t,false);
							if (operandlist.size() > 0 && operandlist.back().type != OP_LOCAL)
							{
								asAtom* a = mi->context->getConstantAtom(operandlist.back().type,operandlist.back().index);
								if (asAtomHandler::getObject(*a))
								{
									variable* v = asAtomHandler::getObject(*a)->findVariableByMultiname(*name,asAtomHandler::getObject(*a)->getClass());
									if (v && v->kind == CONSTANT_TRAIT && asAtomHandler::isInvalid(v->getter))
									{
										operandlist.back().removeArg(mi);
										operandlist.pop_back();
										addCachedConstant(mi, v->var,operandlist,oldnewpositions,code);
										addname = false;
										break;
									}
									if (v && asAtomHandler::isValid(v->getter))
									{
										Class_base* resulttype = (Class_base*)(v->isResolved ? dynamic_cast<const Class_base*>(v->type):nullptr);
										if (!setupInstructionOneArgument(operandlist,mi,ABC_OP_OPTIMZED_CALLFUNCTION_NOARGS,opcode,code,oldnewpositions, jumptargets,true, false,localtypes, defaultlocaltypes,resulttype,p,true))
											lastlocalresulttype = resulttype;
										mi->body->preloadedcode.at(mi->body->preloadedcode.size()-1).cacheobj2 = asAtomHandler::getObject(v->getter);
										addname = false;
										break;
									}
									if (v && v->slotid)
									{
										Class_base* resulttype = (Class_base*)(v->isResolved ? dynamic_cast<const Class_base*>(v->type):nullptr);
										if (asAtomHandler::getObject(*a)->is<Global>())
										{
											// ensure init script is run
											asAtom ret = asAtomHandler::invalidAtom;
											asAtomHandler::getObject(*a)->getVariableByMultiname(ret,*name,GET_VARIABLE_OPTION(DONT_CALL_GETTER|FROM_GETLEX|NO_INCREF));
										}
										if (setupInstructionOneArgument(operandlist,mi,ABC_OP_OPTIMZED_GETSLOT,opcode,code,oldnewpositions, jumptargets,true,false,localtypes, defaultlocaltypes,resulttype,p,true))
										{
											mi->body->preloadedcode.push_back(v->slotid);
											addname = false;
											break;
										}
										else
											lastlocalresulttype = resulttype;
									}
								}
							}
							if (operandlist.size() > 0 && (operandlist.back().type == OP_LOCAL || operandlist.back().type == OP_CACHED_CONSTANT) && operandlist.back().objtype)
							{
								if (canCallFunctionDirect(operandlist.back(),name))
								{
									variable* v = operandlist.back().objtype->getBorrowedVariableByMultiname(*name);
									if (v && asAtomHandler::is<IFunction>(v->getter))
									{
										Class_base* resulttype = v->isResolved && dynamic_cast<const Class_base*>(v->type) ? (Class_base*)v->type : nullptr;
										if (!setupInstructionOneArgument(operandlist,mi,ABC_OP_OPTIMZED_CALLFUNCTION_NOARGS,opcode,code,oldnewpositions, jumptargets,true, false,localtypes, defaultlocaltypes,resulttype,p,true))
											lastlocalresulttype = resulttype;
										mi->body->preloadedcode.at(mi->body->preloadedcode.size()-1).cacheobj2 = asAtomHandler::getObject(v->getter);
										addname = false;
										break;
									}
								}

								if (operandlist.back().objtype && !operandlist.back().objtype->isInterface && operandlist.back().objtype->isInitialized())
								{
									if (operandlist.back().objtype->is<Class_inherit>())
										operandlist.back().objtype->as<Class_inherit>()->checkScriptInit();
									// check if we can replace getProperty by getSlot
									asAtom o = asAtomHandler::invalidAtom;
									operandlist.back().objtype->getInstance(o,false,nullptr,0);
									operandlist.back().objtype->setupDeclaredTraits(asAtomHandler::getObject(o));

									variable* v = asAtomHandler::getObject(o)->findVariableByMultiname(*name,nullptr);
									if (v && v->slotid)
									{
										Class_base* resulttype = v->isResolved && dynamic_cast<const Class_base*>(v->type) ? (Class_base*)v->type : nullptr;
										if (setupInstructionOneArgument(operandlist,mi,ABC_OP_OPTIMZED_GETSLOT,opcode,code,oldnewpositions, jumptargets,true,false,localtypes, defaultlocaltypes,resulttype,p,true))
										{
											mi->body->preloadedcode.push_back(v->slotid);
											if (operandlist.empty()) // indicates that checkforlocalresult returned false
												lastlocalresulttype = resulttype;
											addname = false;
											ASATOM_DECREF(o);
											break;
										}
										else
										{
											if (checkForLocalResult(operandlist,mi,code,oldnewpositions,jumptargets,0,localtypes,resulttype, defaultlocaltypes))
												mi->body->preloadedcode.at(mi->body->preloadedcode.size()-1).data=ABC_OP_OPTIMZED_GETPROPERTY_STATICNAME_LOCALRESULT;
											else
												lastlocalresulttype = resulttype;
											mi->body->preloadedcode.at(mi->body->preloadedcode.size()-1).cachedmultiname2 = name;
											ASATOM_DECREF(o);
											break;
										}
									}
									else
										ASATOM_DECREF(o);
								}
							}
							bool hasoperands = setupInstructionOneArgument(operandlist,mi,ABC_OP_OPTIMZED_GETPROPERTY_STATICNAME,opcode,code,oldnewpositions, jumptargets,true, false,localtypes, defaultlocaltypes,nullptr,p,true);
							if (!hasoperands && checkForLocalResult(operandlist,mi,code,oldnewpositions,jumptargets,0,localtypes,nullptr, defaultlocaltypes))
								mi->body->preloadedcode.at(mi->body->preloadedcode.size()-1).data=ABC_OP_OPTIMZED_GETPROPERTY_STATICNAME_LOCALRESULT;
							mi->body->preloadedcode.at(mi->body->preloadedcode.size()-1).cachedmultiname2 = name;
							break;
						}
						case 1:
							setupInstructionTwoArguments(operandlist,mi,ABC_OP_OPTIMZED_GETPROPERTY,opcode,code,oldnewpositions, jumptargets,false,false,true,localtypes, defaultlocaltypes,p);
							break;
						default:
							mi->body->preloadedcode.push_back((uint32_t)opcode);
							clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
							break;
					}
				}
				else
				{
					mi->body->preloadedcode.push_back((uint32_t)opcode);
					clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				}
				if (addname)
					mi->body->preloadedcode.push_back(t);
				break;
			}
			case 0x73://convert_i
				if (jumptargets.find(code.tellg()) == jumptargets.end() && operandlist.size() > 0 && operandlist.back().objtype == Class<Integer>::getRef(mi->context->root->getSystemState()).getPtr())
					oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				else
					setupInstructionOneArgument(operandlist,mi,ABC_OP_OPTIMZED_CONVERTI,opcode,code,oldnewpositions, jumptargets,true,true,localtypes, defaultlocaltypes, Class<Integer>::getRef(function->getSystemState()).getPtr(),code.tellg(),true);
				break;
			case 0x74://convert_u
				if (jumptargets.find(code.tellg()) == jumptargets.end() && operandlist.size() > 0 && operandlist.back().objtype == Class<UInteger>::getRef(mi->context->root->getSystemState()).getPtr())
					oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				else
					setupInstructionOneArgument(operandlist,mi,ABC_OP_OPTIMZED_CONVERTU,opcode,code,oldnewpositions, jumptargets,true,true,localtypes, defaultlocaltypes, Class<UInteger>::getRef(function->getSystemState()).getPtr(),code.tellg(),true);
				break;
			case 0x75://convert_d
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				if (jumptargets.find(code.tellg()) == jumptargets.end() && operandlist.size() > 0 && operandlist.back().objtype == Class<Number>::getRef(mi->context->root->getSystemState()).getPtr())
					oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				else
					setupInstructionOneArgument(operandlist,mi,ABC_OP_OPTIMZED_CONVERTD,opcode,code,oldnewpositions, jumptargets,true,true,localtypes, defaultlocaltypes, Class<Number>::getRef(function->getSystemState()).getPtr(),code.tellg(),true);
				break;
			case 0x76://convert_b
				if (jumptargets.find(code.tellg()) == jumptargets.end() &&
						((operandlist.size() > 0 && operandlist.back().objtype == Class<Boolean>::getRef(mi->context->root->getSystemState()).getPtr()) ||
						((code.peekbyte() == 0x11 ||  //iftrue
						  code.peekbyte() == 0x12 )   //iffalse
						 && jumptargets.find(code.tellg()+1) == jumptargets.end()))
						)
					oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				else
					setupInstructionOneArgument(operandlist,mi,ABC_OP_OPTIMZED_CONVERTB,opcode,code,oldnewpositions, jumptargets,true,true,localtypes, defaultlocaltypes, Class<Boolean>::getRef(function->getSystemState()).getPtr(),code.tellg(),true);
				break;
			case 0x91://increment
				setupInstructionOneArgument(operandlist,mi,ABC_OP_OPTIMZED_INCREMENT,opcode,code,oldnewpositions, jumptargets,false,true,localtypes, defaultlocaltypes, Class<Number>::getRef(function->getSystemState()).getPtr(),code.tellg(),dup_indicator == 0);
				dup_indicator=0;
				break;
			case 0x93://decrement
				setupInstructionOneArgument(operandlist,mi,ABC_OP_OPTIMZED_DECREMENT,opcode,code,oldnewpositions, jumptargets,false,true,localtypes, defaultlocaltypes, Class<Number>::getRef(function->getSystemState()).getPtr(),code.tellg(),dup_indicator == 0);
				dup_indicator=0;
				break;
			case 0x95: //typeof
				setupInstructionOneArgument(operandlist,mi,ABC_OP_OPTIMZED_TYPEOF,opcode,code,oldnewpositions, jumptargets,true,true,localtypes, defaultlocaltypes, Class<ASString>::getRef(function->getSystemState()).getPtr(),code.tellg(),true);
				break;
			case 0x96: //not
				setupInstructionOneArgument(operandlist,mi,ABC_OP_OPTIMZED_NOT,opcode,code,oldnewpositions, jumptargets,true,true,localtypes, defaultlocaltypes, Class<Boolean>::getRef(function->getSystemState()).getPtr(),code.tellg(),true);
				break;
			case 0xa0://add
				if (operandlist.size() > 1)
				{
					auto it = operandlist.rbegin();
					if(it->objtype && it->objtype==Class<Integer>::getRef(function->getSystemState()).getPtr())
					{
						it++;
						if(it->objtype && it->objtype==Class<Integer>::getRef(function->getSystemState()).getPtr())
						{
							setupInstructionTwoArguments(operandlist,mi,ABC_OP_OPTIMZED_ADD_I,0xc5 //add_i
														 ,code,oldnewpositions, jumptargets,false,false,true,localtypes, defaultlocaltypes,code.tellg());
							break;
						}
					}
				}
				setupInstructionTwoArguments(operandlist,mi,ABC_OP_OPTIMZED_ADD,opcode,code,oldnewpositions, jumptargets,false,false,true,localtypes, defaultlocaltypes,code.tellg());
				break;
			case 0xa1://subtract
				setupInstructionTwoArguments(operandlist,mi,ABC_OP_OPTIMZED_SUBTRACT,opcode,code,oldnewpositions, jumptargets,true,true,true,localtypes, defaultlocaltypes,code.tellg());
				break;
			case 0xa2://multiply
				setupInstructionTwoArguments(operandlist,mi,ABC_OP_OPTIMZED_MULTIPLY,opcode,code,oldnewpositions, jumptargets,true,true,true,localtypes, defaultlocaltypes,code.tellg());
				break;
			case 0xa3://divide
				setupInstructionTwoArguments(operandlist,mi,ABC_OP_OPTIMZED_DIVIDE,opcode,code,oldnewpositions, jumptargets,true,true,true,localtypes, defaultlocaltypes,code.tellg());
				break;
			case 0xa4://modulo
				setupInstructionTwoArguments(operandlist,mi,ABC_OP_OPTIMZED_MODULO,opcode,code,oldnewpositions, jumptargets,true,true,true,localtypes, defaultlocaltypes,code.tellg());
				break;
			case 0xa5://lshift
				setupInstructionTwoArguments(operandlist,mi,ABC_OP_OPTIMZED_LSHIFT,opcode,code,oldnewpositions, jumptargets,true,true,true,localtypes, defaultlocaltypes,code.tellg());
				break;
			case 0xa6://rshift
				setupInstructionTwoArguments(operandlist,mi,ABC_OP_OPTIMZED_RSHIFT,opcode,code,oldnewpositions, jumptargets,true,true,true,localtypes, defaultlocaltypes,code.tellg());
				break;
			case 0xa7://urshift
				setupInstructionTwoArguments(operandlist,mi,ABC_OP_OPTIMZED_URSHIFT,opcode,code,oldnewpositions, jumptargets,true,true,true,localtypes, defaultlocaltypes,code.tellg());
				break;
			case 0xa8://bitand
				setupInstructionTwoArguments(operandlist,mi,ABC_OP_OPTIMZED_BITAND,opcode,code,oldnewpositions, jumptargets,true,true,true,localtypes, defaultlocaltypes,code.tellg());
				break;
			case 0xa9://bitor
				setupInstructionTwoArguments(operandlist,mi,ABC_OP_OPTIMZED_BITOR,opcode,code,oldnewpositions, jumptargets,true,true,true,localtypes, defaultlocaltypes,code.tellg());
				break;
			case 0xaa://bitxor
				setupInstructionTwoArguments(operandlist,mi,ABC_OP_OPTIMZED_BITXOR,opcode,code,oldnewpositions, jumptargets,true,true,true,localtypes, defaultlocaltypes,code.tellg());
				break;
			case 0xab://equals
				setupInstructionTwoArguments(operandlist,mi,ABC_OP_OPTIMZED_EQUALS,opcode,code,oldnewpositions, jumptargets,false,false,true,localtypes, defaultlocaltypes,code.tellg());
				break;
			case 0xad://lessthan
				setupInstructionTwoArguments(operandlist,mi,ABC_OP_OPTIMZED_LESSTHAN,opcode,code,oldnewpositions, jumptargets,false,false,true,localtypes, defaultlocaltypes,code.tellg());
				break;
			case 0xae://lessequals
				setupInstructionTwoArguments(operandlist,mi,ABC_OP_OPTIMZED_LESSEQUALS,opcode,code,oldnewpositions, jumptargets,false,false,true,localtypes, defaultlocaltypes,code.tellg());
				break;
			case 0xaf://greaterthan
				setupInstructionTwoArguments(operandlist,mi,ABC_OP_OPTIMZED_GREATERTHAN,opcode,code,oldnewpositions, jumptargets,false,false,true,localtypes, defaultlocaltypes,code.tellg());
				break;
			case 0xb0://greaterequals
				setupInstructionTwoArguments(operandlist,mi,ABC_OP_OPTIMZED_GREATEREQUALS,opcode,code,oldnewpositions, jumptargets,false,false,true,localtypes, defaultlocaltypes,code.tellg());
				break;
			case 0xc0://increment_i
			case 0xc1://decrement_i
			{
				uint32_t p = code.tellg();
				// optimize common case of increment/decrement local variable
				if (operandlist.size() > 0 && 
						operandlist.back().type == OP_LOCAL && 
						operandlist.back().objtype == Class<Integer>::getRef(function->getSystemState()).getPtr() &&
						jumptargets.find(code.tellg()+1) == jumptargets.end())
				{
					int32_t t = -1;
					if (code.peekbyte() == 0x73) //convert_i
						code.readbyte();
					switch (code.peekbyte())
					{
						case 0x63://setlocal
							t = code.peekbyteFromPosition(code.tellg()+1);
							break;
						case 0xd4://setlocal_0
							t = 0;
							break;
						case 0xd5://setlocal_1
							t = 1;
							break;
						case 0xd6://setlocal_2
							t = 2;
							break;
						case 0xd7://setlocal_3
							t = 3;
							break;
					}
					if (t == operandlist.back().index)
					{
						operandlist.back().removeArg(mi);
						mi->body->preloadedcode.push_back((uint32_t)(opcode == 0xc0 ? 0xc2 : 0xc3)); // inclocal_i/declocal_i
						mi->body->preloadedcode.push_back((uint32_t)t);
						oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
						if (code.readbyte() == 0x63) //setlocal
							code.readbyte();
						operandlist.pop_back();
						if (dup_indicator)
							clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
						break;
					}
				}
				setupInstructionOneArgument(operandlist,mi,opcode == 0xc0 ? ABC_OP_OPTIMZED_INCREMENT_I : ABC_OP_OPTIMZED_DECREMENT_I,opcode,code,oldnewpositions, jumptargets,false,true,localtypes, defaultlocaltypes, Class<Integer>::getRef(function->getSystemState()).getPtr(),p,dup_indicator == 0);
				dup_indicator=0;
				break;
			}
			case 0xc5://add_i
				setupInstructionTwoArguments(operandlist,mi,ABC_OP_OPTIMZED_ADD_I,opcode,code,oldnewpositions, jumptargets,false,false,true,localtypes, defaultlocaltypes,code.tellg());
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
				clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				break;
			}
		}
	}
	oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
	// also add position for end of code, as it seems that jumps to this position are allowed
	oldnewpositions[code.tellg()+1] = (int32_t)mi->body->preloadedcode.size();
	
	// adjust jump positions to new code vector;
	auto itj = jumppositions.begin();
	while (itj != jumppositions.end())
	{
		uint32_t p = jumpstartpositions[itj->first];
		assert (oldnewpositions.find(p) != oldnewpositions.end());
		if (oldnewpositions.find(p+itj->second) == oldnewpositions.end() && p+itj->second < code.tellg())
		{
			LOG(LOG_ERROR,"preloadfunction: jump position not found:"<<p<<" "<<itj->second<<" "<<code.tellg());
			throwError<VerifyError>(kInvalidBranchTargetError);
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

