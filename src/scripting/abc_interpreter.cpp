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
void ABCVm::checkPropertyException(ASObject* obj,multiname* name, asAtom& prop)
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
void ABCVm::checkPropertyExceptionInteger(ASObject* obj,int index, asAtom& prop)
{
	multiname m(nullptr);
	m.name_type = multiname::NAME_INT;
	m.name_i = index;
	checkPropertyException(obj,&m, prop);
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
	abc_setPropertyStaticName, // 0x284 ABC_OP_OPTIMZED_SETPROPERTY_STATICNAME_SIMPLE
	abc_getPropertyInteger, // 0x285 ABC_OP_OPTIMZED_GETPROPERTY_INTEGER_SIMPLE
	abc_setPropertyInteger, // 0x286 ABC_OP_OPTIMZED_SETPROPERTY_INTEGER_SIMPLE
	abc_ifnltInt, // 0x287 ABC_OP_OPTIMZED_IFNLT_INT_SIMPLE
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
	abc_ifnltInt_constant_constant, // 0x29c ABC_OP_OPTIMZED_IFNLT_INT
	abc_ifnltInt_local_constant,
	abc_ifnltInt_constant_local,
	abc_ifnltInt_local_local,

	abc_setlocal_constant, // 0x2a0 ABC_OP_OPTIMZED_SETLOCAL
	abc_setlocal_local,
	abc_ifneInt, // 0x2a2 ABC_OP_OPTIMZED_IFNE_INT_SIMPLE
	abc_ifngeInt, // 0x2a3 ABC_OP_OPTIMZED_IFNGE_INT_SIMPLE
	abc_ifneInt_constant_constant, // 0x2a4 ABC_OP_OPTIMZED_IFNE_INT
	abc_ifneInt_local_constant,
	abc_ifneInt_constant_local,
	abc_ifneInt_local_local,
	abc_callFunctionSyntheticMultiArgs, // 0x2a8 ABC_OP_OPTIMZED_CALLFUNCTIONSYNTHETIC_STATICNAME_MULTIARGS
	abc_callFunctionSyntheticMultiArgsVoid, // 0x2a9 ABC_OP_OPTIMZED_CALLFUNCTIONVOIDSYNTHETIC_STATICNAME_MULTIARGS
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,

	abc_ifnge_constant_constant, // 0x2b0 ABC_OP_OPTIMZED_IFNGE
	abc_ifnge_local_constant,
	abc_ifnge_constant_local,
	abc_ifnge_local_local,
	abc_ifngeInt_constant_constant, // 0x2b4 ABC_OP_OPTIMZED_IFNGE_INT
	abc_ifngeInt_local_constant,
	abc_ifngeInt_constant_local,
	abc_ifngeInt_local_local,
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
#define ABC_OP_OPTIMZED_SETPROPERTY_STATICNAME_SIMPLE 0x00000284
#define ABC_OP_OPTIMZED_GETPROPERTY_INTEGER_SIMPLE 0x00000285
#define ABC_OP_OPTIMZED_SETPROPERTY_INTEGER_SIMPLE 0x00000286
#define ABC_OP_OPTIMZED_IFNLT_INT_SIMPLE 0x00000287
#define ABC_OP_OPTIMZED_GETPROPERTY_INTEGER 0x00000288
#define ABC_OP_OPTIMZED_SETPROPERTY_INTEGER 0x00000290
#define ABC_OP_OPTIMZED_IFNLT 0x00000298
#define ABC_OP_OPTIMZED_IFNLT_INT 0x0000029c
#define ABC_OP_OPTIMZED_SETLOCAL 0x000002a0
#define ABC_OP_OPTIMZED_IFNE_INT_SIMPLE 0x000002a2
#define ABC_OP_OPTIMZED_IFNGE_INT_SIMPLE 0x000002a3
#define ABC_OP_OPTIMZED_IFNE_INT 0x000002a4
#define ABC_OP_OPTIMZED_CALLFUNCTIONSYNTHETIC_STATICNAME_MULTIARGS 0x000002a8
#define ABC_OP_OPTIMZED_CALLFUNCTIONVOIDSYNTHETIC_STATICNAME_MULTIARGS 0x000002a9
#define ABC_OP_OPTIMZED_IFNGE 0x000002b0
#define ABC_OP_OPTIMZED_IFNGE_INT 0x000002b4

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
void clearOperands(method_info* mi,Class_base** localtypes,std::vector<operands>& operandlist, Class_base** defaultlocaltypes,Class_base** lastlocalresulttype )
{
	if (localtypes)
	{
		for (uint32_t i = 0; i < mi->body->local_count+2; i++)
		{
			// AFAIK there is no method to check the size of a c array, so we can't assert here
			//assert(i < defaultlocaltypes.len && "array out of bounds!"); 
			localtypes[i] = defaultlocaltypes[i];
		}
	}
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
bool canCallFunctionDirect(ASObject* obj,multiname* name)
{
	if (!obj || !obj->is<Class_base>())
		return false;
	Class_base* objtype = obj->as<Class_base>();
	return 
		!objtype->isInterface && // it's not an interface
		objtype->isSealed && // it's sealed
		(
		!objtype->is<Class_inherit>() || // type is builtin class
		!objtype->as<Class_inherit>()->hasoverriddenmethod(name) // current method is not in overridden methods
		);
}

bool checkForLocalResult(std::vector<operands>& operandlist,method_info* mi,memorystream& code,std::map<int32_t,int32_t>& oldnewpositions,std::map<int32_t,int32_t>& jumptargets,uint32_t opcode_jumpspace, Class_base** localtypes, Class_base* restype, Class_base** defaultlocaltypes,int preloadpos=-1,int preloadlocalpos=-1)
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
				clearOperands(mi,nullptr,operandlist, defaultlocaltypes,nullptr);
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
				clearOperands(mi,nullptr,operandlist, defaultlocaltypes,nullptr);
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
				clearOperands(mi,nullptr,operandlist, defaultlocaltypes,nullptr);
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
				clearOperands(mi,nullptr,operandlist,defaultlocaltypes,nullptr);
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
				clearOperands(mi,nullptr,operandlist,defaultlocaltypes,nullptr);
			break;
		}
		case 0x61://setproperty
		case 0x68://initproperty
		{
			uint32_t t = code.peeku30FromPosition(pos);
			if (jumptargets.find(pos) == jumptargets.end() && (argsneeded<=1) &&
					(
					   (((argsneeded==1) || (operandlist.size() >= 1)) 
					    && (uint32_t)mi->context->constant_pool.multinames[t].runtimeargs == 0)
					|| ((argsneeded==0) && (operandlist.size() > 1) 
						&& (uint32_t)mi->context->constant_pool.multinames[t].runtimeargs == 1
						&& (operandlist.back().objtype == Class<Integer>::getRef(mi->context->root->getSystemState()).getPtr()))
					)
				)
			{
				// set optimized opcode to corresponding opcode with local result 
				mi->body->preloadedcode[preloadpos].data += opcode_jumpspace;
				mi->body->preloadedcode[preloadlocalpos].local_pos3 = mi->body->local_count+2+resultpos;
				operandlist.push_back(operands(OP_LOCAL,restype,mi->body->local_count+1+resultpos,0,0));
				res = true;
			}
			else
				clearOperands(mi,nullptr,operandlist, defaultlocaltypes,nullptr);
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
				clearOperands(mi,nullptr,operandlist, defaultlocaltypes,nullptr);
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
				clearOperands(mi,nullptr,operandlist, defaultlocaltypes,nullptr);
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
				clearOperands(mi,nullptr,operandlist, defaultlocaltypes,nullptr);
			break;
		case 0x0c://ifnlt
		case 0x0f://ifnge
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
				clearOperands(mi,nullptr,operandlist, defaultlocaltypes,nullptr);
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
				clearOperands(mi,nullptr,operandlist, defaultlocaltypes,nullptr);
			break;
		case 0x10://jump
			// don't clear operandlist yet, because the jump may be skipped
			break;
		default:
			clearOperands(mi,nullptr,operandlist, defaultlocaltypes,nullptr);
			break;
	}
	return res;
}

bool setupInstructionOneArgumentNoResult(std::vector<operands>& operandlist,method_info* mi,int operator_start,int opcode,memorystream& code,std::map<int32_t,int32_t>& oldnewpositions,std::map<int32_t,int32_t>& jumptargets, uint32_t startcodepos)
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
bool setupInstructionTwoArgumentsNoResult(std::vector<operands>& operandlist,method_info* mi,int operator_start,int opcode,memorystream& code,std::map<int32_t,int32_t>& oldnewpositions,std::map<int32_t,int32_t>& jumptargets)
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
bool setupInstructionOneArgument(std::vector<operands>& operandlist,method_info* mi,int operator_start,int opcode,memorystream& code,std::map<int32_t,int32_t>& oldnewpositions,std::map<int32_t,int32_t>& jumptargets,bool constantsallowed, bool useargument_for_skip, Class_base** localtypes, Class_base** defaultlocaltypes, Class_base* resulttype, uint32_t startcodepos, bool checkforlocalresult)
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
		clearOperands(mi,nullptr,operandlist, defaultlocaltypes,nullptr);
	return hasoperands;
}

bool setupInstructionTwoArguments(std::vector<operands>& operandlist,method_info* mi,int operator_start,int opcode,memorystream& code,std::map<int32_t,int32_t>& oldnewpositions,std::map<int32_t,int32_t>& jumptargets, bool skip_conversion,bool cancollapse,bool checklocalresult, Class_base** localtypes, Class_base** defaultlocaltypes, uint32_t startcodepos)
{
	bool hasoperands = jumptargets.find(startcodepos) == jumptargets.end() && operandlist.size() >= 2;
	Class_base* resulttype = nullptr;
	bool op1isconstant=false;
	bool op2isconstant=false;
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
		if (cancollapse)
		{
			it = operandlist.end();
			op2isconstant = ((--it)->type != OP_LOCAL);
			op1isconstant = ((--it)->type != OP_LOCAL);
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
			case ABC_OP_OPTIMZED_MODULO:
				if (op1type == Class<Integer>::getRef(mi->context->root->getSystemState()).getPtr() && op2type == Class<Integer>::getRef(mi->context->root->getSystemState()).getPtr())
					resulttype = Class<Integer>::getRef(mi->context->root->getSystemState()).getPtr();
				else
					resulttype = Class<Number>::getRef(mi->context->root->getSystemState()).getPtr();
				break;
			case ABC_OP_OPTIMZED_SUBTRACT:
			case ABC_OP_OPTIMZED_MULTIPLY:
			case ABC_OP_OPTIMZED_DIVIDE:
				resulttype = Class<Number>::getRef(mi->context->root->getSystemState()).getPtr();
				break;
			case ABC_OP_OPTIMZED_ADD_I:
			case ABC_OP_OPTIMZED_LSHIFT:
			case ABC_OP_OPTIMZED_RSHIFT:
			case ABC_OP_OPTIMZED_BITOR:
				resulttype = Class<Integer>::getRef(mi->context->root->getSystemState()).getPtr();
				break;
			case ABC_OP_OPTIMZED_URSHIFT:
				resulttype = Class<Integer>::getRef(mi->context->root->getSystemState()).getPtr();
				// operators are always transformed to uint, so we can do that here if the operators are constants
				if (op1isconstant)
					mi->body->preloadedcode[mi->body->preloadedcode.size()-1].arg1_int=asAtomHandler::toUInt(*mi->body->preloadedcode[mi->body->preloadedcode.size()-1].arg1_constant);
				if (op2isconstant)
					mi->body->preloadedcode[mi->body->preloadedcode.size()-1].arg2_int=asAtomHandler::toUInt(*mi->body->preloadedcode[mi->body->preloadedcode.size()-1].arg2_constant);
				break;
			case ABC_OP_OPTIMZED_BITAND:
			case ABC_OP_OPTIMZED_BITXOR:
				resulttype = Class<Integer>::getRef(mi->context->root->getSystemState()).getPtr();
				// operators are always transformed to int, so we can do that here if the operators are constants
				if (op1isconstant)
					mi->body->preloadedcode[mi->body->preloadedcode.size()-1].arg1_int=asAtomHandler::toInt(*mi->body->preloadedcode[mi->body->preloadedcode.size()-1].arg1_constant);
				if (op2isconstant)
					mi->body->preloadedcode[mi->body->preloadedcode.size()-1].arg2_int=asAtomHandler::toInt(*mi->body->preloadedcode[mi->body->preloadedcode.size()-1].arg2_constant);
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
			clearOperands(mi,nullptr,operandlist, defaultlocaltypes,nullptr);
	}
	return hasoperands;
}
void addCachedConstant(method_info* mi, asAtom& val,std::vector<operands>& operandlist,std::map<int32_t,int32_t>& oldnewpositions,memorystream& code)
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
struct typestackentry
{
	ASObject* obj;
	bool classvar;
	typestackentry(ASObject* o, bool c):obj(o),classvar(c){}
};
void removetypestack(std::vector<typestackentry>& typestack,int n)
{
	assert_and_throw(uint32_t(n) <= typestack.size());
	for (int i = 0; i < n; i++)
	{
		typestack.pop_back();
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
	std::multimap<int32_t,int32_t> unreachabletargets;
	int32_t nextreachable = -1;
	int32_t unreachablestart = -1;
	
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
		//LOG(LOG_ERROR,"preload pass1:"<<function->getSystemState()->getStringFromUniqueId(function->functionname)<<" "<< codejumps.tellg()-1<<" "<<currenttype<<" "<<hex<<(int)opcode);
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
			case 0xc0://increment_i
			case 0xc1://decrement_i
				currenttype=Class<Integer>::getRef(function->getSystemState()).getPtr();
				break;
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
			{
				int32_t p = codejumps.reads24()+codejumps.tellg()+1;
				jumptargets[p]++;
				if (p >= nextreachable)
				{
					// we reached the position of the next reachable jump point, so we end checking for unreachables here
					unreachablestart = -1;
				}
				if (unreachablestart != -1)
				{
					// this jump points to a position that is potentially not reachable
					unreachabletargets.insert(make_pair(unreachablestart,p));
				}
				currenttype=nullptr;
				break;
			}
			case 0x1b://lookupswitch
			{
				int32_t p = codejumps.tellg();
				int32_t p1 = p+codejumps.reads24();
				if (p1 >= nextreachable)
				{
					// we reached the position of the next reachable jump point, so we end checking for unreachables here
					unreachablestart = -1;
				}
				if (unreachablestart != -1)
				{
					// this jump points to a position that is potentially not reachable
					unreachabletargets.insert(make_pair(unreachablestart,p1));
				}
				jumptargets[p1]++;
				uint32_t count = codejumps.readu30();
				for(unsigned int i=0;i<count+1;i++)
				{
					p1 = p+codejumps.reads24();
					jumptargets[p1]++;
					if (p1 >= nextreachable)
					{
						// we reached the position of the next reachable jump point, so we end checking for unreachables here
						unreachablestart = -1;
					}
					if (unreachablestart != -1)
					{
						// this jump points to a position that is potentially not reachable
						unreachabletargets.insert(make_pair(unreachablestart,p1));
					}
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
			case 0xa5://lshift
			case 0xa6://rshift
			case 0xa8://bitand
			case 0xa9://bitor
			{
				currenttype=Class<Integer>::getRef(function->getSystemState()).getPtr();
				break;
			}
			case 0x09://label
			case 0x2a://dup
				break;
			case 0x47: //returnvoid
			case 0x48://returnvalue
				if (unreachablestart == -1)
				{
					unreachablestart = codejumps.tellg();
					nextreachable = codejumps.size();
					// find the first jump target after the current position
					auto it = jumptargets.begin();
					while (it != jumptargets.end() && it->first < (int)codejumps.tellg())
					{
						it++;
					}
					if (it != jumptargets.end())
						nextreachable = it->second;
				}
				currenttype=nullptr;
				break;
			default:
				currenttype=nullptr;
				break;
		}
	}
	// remove all really unreachable targets
	auto it = unreachabletargets.begin();
	while (it != unreachabletargets.end())
	{
		auto ittarget = jumptargets.begin();
		while (ittarget != jumptargets.end())
		{
			// find first target after unreachable start position
			if (ittarget->first <= it->first)
				ittarget++;
			else
			{
				if (ittarget->first > it->second)
				{
					jumptargets.erase(ittarget);
				}
				break;
			}
		}
		it++;
	}
	// second pass: use optimized opcode version if it doesn't interfere with a jump target
	std::vector<operands> operandlist;
	
	std::vector<typestackentry> typestack; // contains the type or the global object of the arguments currently on the stack
	Class_base* localtypes[mi->body->local_count+2];
	Class_base* lastlocalresulttype=nullptr;
	clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
	memorystream code(mi->body->code.data(), code_len);
	std::list<bool> scopelist;
	int dup_indicator=0;
	bool opcode_skipped=false;
	bool coercereturnvalue=false;
	auto itcurEx = mi->body->exceptions.begin();
	while(!code.atend())
	{
		oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
		while (itcurEx != mi->body->exceptions.end() && itcurEx->target == code.tellg())
		{
			typestack.push_back(typestackentry(nullptr,false));
			itcurEx++;
		}
		uint8_t opcode = code.readbyte();
		//LOG(LOG_INFO,"preload opcode:"<<function->getSystemState()->getStringFromUniqueId(function->functionname)<<" "<< code.tellg()-1<<" "<<operandlist.size()<<" "<<typestack.size()<<" "<<hex<<(int)opcode);
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
				mi->body->preloadedcode.push_back((uint32_t)opcode);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				mi->body->preloadedcode.push_back(t);
				clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				break;
			}
			case 0x04://getsuper
			case 0x59://getdescendants
			{
				uint32_t t = code.readu30();
				assert_and_throw(t < mi->context->constant_pool.multiname_count);
				removetypestack(typestack,mi->context->constant_pool.multinames[t].runtimeargs+1);
				typestack.push_back(typestackentry(nullptr,false));
				mi->body->preloadedcode.push_back((uint32_t)opcode);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				mi->body->preloadedcode.push_back(t);
				clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				break;
			}
			case 0x05://setsuper
			{
				uint32_t t = code.readu30();
				assert_and_throw(t < mi->context->constant_pool.multiname_count);
				removetypestack(typestack,mi->context->constant_pool.multinames[t].runtimeargs+1);
				mi->body->preloadedcode.push_back((uint32_t)opcode);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				mi->body->preloadedcode.push_back(t);
				clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				break;
			}
			case 0x40://newfunction
			{
				uint32_t t = code.readu30();
				typestack.push_back(typestackentry(nullptr,false));
				mi->body->preloadedcode.push_back((uint32_t)opcode);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				mi->body->preloadedcode.push_back(t);
				clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				break;
			}
			case 0x41://call
			{
				uint32_t t = code.readu30();
				removetypestack(typestack,t+2);
				typestack.push_back(typestackentry(nullptr,false));
				mi->body->preloadedcode.push_back((uint32_t)opcode);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				mi->body->preloadedcode.push_back(t);
				clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				break;
			}
			case 0x53://constructgenerictype
			{
				uint32_t t = code.readu30();
				removetypestack(typestack,t+1);
				typestack.push_back(typestackentry(nullptr,false));
				mi->body->preloadedcode.push_back((uint32_t)opcode);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				mi->body->preloadedcode.push_back(t);
				clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				break;
			}
			case 0x55://newobject
			{
				uint32_t t = code.readu30();
				removetypestack(typestack,t*2);
				typestack.push_back(typestackentry(nullptr,false));
				mi->body->preloadedcode.push_back((uint32_t)opcode);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				mi->body->preloadedcode.push_back(t);
				clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				break;
			}
			case 0x56://newarray
			{
				uint32_t t = code.readu30();
				removetypestack(typestack,t);
				typestack.push_back(typestackentry(nullptr,false));
				mi->body->preloadedcode.push_back((uint32_t)opcode);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				mi->body->preloadedcode.push_back(t);
				clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				break;
			}
			case 0x58://newclass
			{
				uint32_t t = code.readu30();
				removetypestack(typestack,1);
				typestack.push_back(typestackentry(nullptr,false));
				mi->body->preloadedcode.push_back((uint32_t)opcode);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				mi->body->preloadedcode.push_back(t);
				clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				break;
			}
			case 0x5a://newcatch
			{
				uint32_t t = code.readu30();
				typestack.push_back(typestackentry(nullptr,false));
				mi->body->preloadedcode.push_back((uint32_t)opcode);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				mi->body->preloadedcode.push_back(t);
				clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				break;
			}
			case 0x6e://getglobalSlot
			{
				uint32_t t = code.readu30();
				typestack.push_back(typestackentry(nullptr,false));
				mi->body->preloadedcode.push_back((uint32_t)opcode);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				mi->body->preloadedcode.push_back(t);
				clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				break;
			}
			case 0x6a://deleteproperty
			case 0xb2://istype
			{
				uint32_t t = code.readu30();
				assert_and_throw(t < mi->context->constant_pool.multiname_count);
				removetypestack(typestack,mi->context->constant_pool.multinames[t].runtimeargs+1);
				typestack.push_back(typestackentry(Class<Boolean>::getRef(mi->context->root->getSystemState()).getPtr(),false));
				mi->body->preloadedcode.push_back((uint32_t)opcode);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				mi->body->preloadedcode.push_back(t);
				clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				break;
			}
			case 0x6f://setglobalSlot
			{
				uint32_t t = code.readu30();
				removetypestack(typestack,1);
				mi->body->preloadedcode.push_back((uint32_t)opcode);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				mi->body->preloadedcode.push_back(t);
				clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				break;
			}
			case 0x86://astype
			{
				uint32_t t = code.readu30();
				removetypestack(typestack,1);
				typestack.push_back(typestackentry(nullptr,false));
				mi->body->preloadedcode.push_back((uint32_t)opcode);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				mi->body->preloadedcode.push_back(t);
				clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				break;
			}

			
			case 0x06://dxns
			case 0x08://kill
			case 0x92://inclocal
			case 0x94://declocal
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
				removetypestack(typestack,1);
				scopelist.push_back(true);
				mi->body->preloadedcode.push_back((uint32_t)opcode);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				break;
			case 0x1d://popscope
				if (!scopelist.empty())
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
				removetypestack(typestack,t+1);
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
				removetypestack(typestack,mi->context->constant_pool.multinames[t].runtimeargs);
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
								typestack.push_back(typestackentry(function->inClass,isborrowed));
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
								typestack.push_back(typestackentry(target,false));
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
				typestack.push_back(typestackentry(nullptr,false));
				break;
			}
			case 0x60://getlex
			{
				Class_base* resulttype = nullptr;
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
					GET_VARIABLE_RESULT r = GETVAR_NORMAL;
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
							r = asAtomHandler::toObject(it->object,mi->context->root->getSystemState())->getVariableByMultiname(o,*name, opt);
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
						resulttype = asAtomHandler::as<Class_base>(o);
						if (asAtomHandler::as<Class_base>(o)->isConstructed())
						{
							addCachedConstant(mi, o,operandlist,oldnewpositions,code);
							typestack.push_back(typestackentry(resulttype,true));
							break;
						}
					}
					else if (r & GETVAR_ISCONSTANT && !asAtomHandler::isNull(o)) // class may not be constructed yet, so the result is null and we do not cache
					{
						addCachedConstant(mi, o,operandlist,oldnewpositions,code);
						typestack.push_back(typestackentry(nullptr,false));
						break;
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
							typestack.push_back(typestackentry(nullptr,false));
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
				typestack.push_back(typestackentry(resulttype,true));
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
											removetypestack(typestack,mi->context->constant_pool.multinames[t].runtimeargs+2);
											break;
										}
										if (asAtomHandler::is<Function>(v->setter))
										{
											setupInstructionTwoArgumentsNoResult(operandlist,mi,ABC_OP_OPTIMZED_CALLFUNCTIONBUILTIN_ONEARG_VOID,opcode,code,oldnewpositions, jumptargets);
											mi->body->preloadedcode.at(mi->body->preloadedcode.size()-1).cacheobj3 = asAtomHandler::getObject(v->setter);
											removetypestack(typestack,mi->context->constant_pool.multinames[t].runtimeargs+2);
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
										removetypestack(typestack,mi->context->constant_pool.multinames[t].runtimeargs+2);
										break;
									}
									else
										ASATOM_DECREF(o);
								}
							}
							
							if (!setupInstructionTwoArgumentsNoResult(operandlist,mi,ABC_OP_OPTIMZED_SETPROPERTY_STATICNAME,opcode,code,oldnewpositions, jumptargets))
								mi->body->preloadedcode.at(mi->body->preloadedcode.size()-1).data = uint32_t(ABC_OP_OPTIMZED_SETPROPERTY_STATICNAME_SIMPLE);
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
							clearOperands(mi,nullptr,operandlist, defaultlocaltypes,&lastlocalresulttype);
							removetypestack(typestack,mi->context->constant_pool.multinames[t].runtimeargs+2);
							break;
						}
						case 1:
							if (operandlist.size() > 2 && (operandlist[operandlist.size()-2].objtype == Class<Integer>::getRef(function->getSystemState()).getPtr()))
							{
								if (operandlist[operandlist.size()-3].type == OP_LOCAL)
								{
									int index = operandlist[operandlist.size()-3].index;
									setupInstructionTwoArgumentsNoResult(operandlist,mi,ABC_OP_OPTIMZED_SETPROPERTY_INTEGER+4,opcode,code,oldnewpositions, jumptargets);
									mi->body->preloadedcode.at(mi->body->preloadedcode.size()-1).local_pos3=index;
									mi->body->preloadedcode.push_back(t);
									mi->body->preloadedcode.at(mi->body->preloadedcode.size()-1).local_pos3 = opcode; // use local_pos3 as indicator for setproperty/initproperty
									operandlist.back().removeArg(mi);
								}
								else
								{
									asAtom* arg = mi->context->getConstantAtom(operandlist[operandlist.size()-3].type,operandlist[operandlist.size()-3].index);
									setupInstructionTwoArgumentsNoResult(operandlist,mi,ABC_OP_OPTIMZED_SETPROPERTY_INTEGER+0,opcode,code,oldnewpositions, jumptargets);
									mi->body->preloadedcode.at(mi->body->preloadedcode.size()-1).arg3_constant=arg;
									mi->body->preloadedcode.push_back(t);
									mi->body->preloadedcode.at(mi->body->preloadedcode.size()-1).local_pos3 = opcode; // use local_pos3 as indicator for setproperty/initproperty
									operandlist.back().removeArg(mi);
								}
								clearOperands(mi,nullptr,operandlist, defaultlocaltypes,&lastlocalresulttype);
							}
							else
							{
								if (typestack.size() > 2 && typestack[typestack.size()-2].obj == Class<Integer>::getRef(function->getSystemState()).getPtr())
								{
									mi->body->preloadedcode.push_back((uint32_t)ABC_OP_OPTIMZED_SETPROPERTY_INTEGER_SIMPLE);
									mi->body->preloadedcode.push_back(t);
									mi->body->preloadedcode.at(mi->body->preloadedcode.size()-1).local_pos3 = opcode; // use local_pos3 as indicator for setproperty/initproperty
								}
								else
								{
									mi->body->preloadedcode.push_back((uint32_t)opcode);
									mi->body->preloadedcode.push_back(t);
								}
								clearOperands(mi,nullptr,operandlist, defaultlocaltypes,&lastlocalresulttype);
							}
							removetypestack(typestack,mi->context->constant_pool.multinames[t].runtimeargs+2);
							break;
						default:
							mi->body->preloadedcode.push_back((uint32_t)opcode);
							mi->body->preloadedcode.push_back(t);
							clearOperands(mi,nullptr,operandlist, defaultlocaltypes,&lastlocalresulttype);
							removetypestack(typestack,mi->context->constant_pool.multinames[t].runtimeargs+1);
							break;
					}
				}
				else
				{
					switch (mi->context->constant_pool.multinames[t].runtimeargs)
					{
						case 0:
						{
							multiname* name =  mi->context->getMultinameImpl(asAtomHandler::nullAtom,nullptr,t,false);
							mi->body->preloadedcode.push_back((uint32_t)ABC_OP_OPTIMZED_SETPROPERTY_STATICNAME_SIMPLE);
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
							removetypestack(typestack,mi->context->constant_pool.multinames[t].runtimeargs+1);
							break;
						}
						default:
							mi->body->preloadedcode.push_back((uint32_t)opcode);
							mi->body->preloadedcode.push_back(t);
							clearOperands(mi,nullptr,operandlist, defaultlocaltypes,&lastlocalresulttype);
							removetypestack(typestack,mi->context->constant_pool.multinames[t].runtimeargs+1);
							break;
					}
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
				typestack.push_back(typestackentry(localtypes[value],false));
				break;
			}
			case 0x63://setlocal
			{
				int32_t p = code.tellg();
				uint32_t value =code.readu30();
				assert_and_throw(value < mi->body->local_count);
				setupInstructionOneArgumentNoResult(operandlist,mi,ABC_OP_OPTIMZED_SETLOCAL,opcode,code,oldnewpositions, jumptargets,p);
				mi->body->preloadedcode.at(mi->body->preloadedcode.size()-1).data|=value<<OPCODE_SIZE;
				if (typestack.back().obj && typestack.back().obj->is<Class_base>())
					localtypes[value]=typestack.back().obj->as<Class_base>();
				else
					localtypes[value]=nullptr;
				removetypestack(typestack,1);
				break;
			}
			case 0x65://getscopeobject
			{
				mi->body->preloadedcode.push_back((uint32_t)opcode);
				typestack.push_back(typestackentry(nullptr,false));
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
				removetypestack(typestack,1);
				int32_t p = code.tellg();
				if (jumptargets.find(p) != jumptargets.end())
					clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				int32_t t =code.readu30();assert_and_throw(t);
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
								typestack.push_back(typestackentry(nullptr,false));
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
				if (setupInstructionOneArgument(operandlist,mi,ABC_OP_OPTIMZED_GETSLOT,opcode,code,oldnewpositions, jumptargets,true,false,localtypes, defaultlocaltypes,resulttype,p,true))
					mi->body->preloadedcode.at(mi->body->preloadedcode.size()-1).data |=(t-1)<<OPCODE_SIZE;
				else
					mi->body->preloadedcode.push_back(t);
				typestack.push_back(typestackentry(resulttype,false));
				break;
			}
			case 0x6d://setslot
			{
				removetypestack(typestack,2);
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
				ASObject* tobj = nullptr;
				if (jumptargets.find(p) == jumptargets.end() && typestack.size() > 0)
				{
					assert_and_throw(name->isStatic);
					const Type* tp = Type::getTypeFromMultiname(name, mi->context);
					const Class_base* cls =dynamic_cast<const Class_base*>(tp);
					tobj = typestack.back().obj;
					if (tobj && tobj->is<Class_base>())
					{
						if (tobj == Class<Number>::getRef(mi->context->root->getSystemState()).getPtr() ||
								 tobj  == Class<Integer>::getRef(mi->context->root->getSystemState()).getPtr() ||
								 tobj  == Class<UInteger>::getRef(mi->context->root->getSystemState()).getPtr() ||
								 tobj  == Class<Boolean>::getRef(mi->context->root->getSystemState()).getPtr())
							skip = cls == Class<Number>::getRef(mi->context->root->getSystemState()).getPtr() || cls == Class<Integer>::getRef(mi->context->root->getSystemState()).getPtr() || cls == Class<UInteger>::getRef(mi->context->root->getSystemState()).getPtr();
						else
							skip = cls && cls->isSubClass(tobj->as<Class_base>());
					}
					else if (operandlist.size() > 0 && operandlist.back().type != OP_LOCAL)
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
				else
					opcode_skipped=true;
				removetypestack(typestack,1);
				typestack.push_back(typestackentry(tobj,false));
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
				if (((uint32_t)opcode)-0xd0 >= mi->body->local_count)
				{
					// this may happen in unreachable obfuscated code, so we just ignore the opcode
					LOG(LOG_ERROR,"preload getlocal with argument > local_count:"<<mi->body->local_count<<" "<<hex<<(int)opcode);
					oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
					if (jumptargets.find(p) != jumptargets.end())
						clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				}
				mi->body->preloadedcode.push_back((uint32_t)opcode);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				if (jumptargets.find(p) != jumptargets.end())
					clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				operandlist.push_back(operands(OP_LOCAL,localtypes[((uint32_t)opcode)-0xd0],((uint32_t)opcode)-0xd0,1,mi->body->preloadedcode.size()-1));
				typestack.push_back(typestackentry(localtypes[((uint32_t)opcode)-0xd0],false));
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
				typestack.push_back(typestack.back());
				break;
			}
			case 0x01://bkpt
			case 0x02://nop
			case 0x82://coerce_a
			{
				//skip
				int32_t p = code.tellg();
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				if (jumptargets.find(p) != jumptargets.end())
					jumptargets[p+1]++;
				opcode_skipped=true;
				break;
			}
			case 0xf0://debugline
			case 0xf1://debugfile
			case 0xf2://bkptline
			{
				//skip
				int32_t p = code.tellg();
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				code.readu30();
				if (jumptargets.find(p) != jumptargets.end())
					jumptargets[code.tellg()+1]++;
				opcode_skipped=true;
				break;
			}
			case 0x0c://ifnlt
			{
				bool intcomparison = (typestack.size() >=2 
						&& typestack[typestack.size()-1].obj ==Class<Integer>::getRef(mi->context->root->getSystemState()).getPtr()
						&& typestack[typestack.size()-2].obj ==Class<Integer>::getRef(mi->context->root->getSystemState()).getPtr());
				removetypestack(typestack,2);
				if (!setupInstructionTwoArgumentsNoResult(operandlist,mi,intcomparison ? ABC_OP_OPTIMZED_IFNLT_INT : ABC_OP_OPTIMZED_IFNLT,opcode,code,oldnewpositions, jumptargets))
				{
					if (intcomparison)
						mi->body->preloadedcode.at(mi->body->preloadedcode.size()-1).data=uint32_t(ABC_OP_OPTIMZED_IFNLT_INT_SIMPLE);
				}
				jumppositions[mi->body->preloadedcode.size()-1] = code.reads24();
				jumpstartpositions[mi->body->preloadedcode.size()-1] = code.tellg();
				clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				break;
			}
			case 0x0f://ifnge
			{
				bool intcomparison = (typestack.size() >=2
						&& typestack[typestack.size()-1].obj ==Class<Integer>::getRef(mi->context->root->getSystemState()).getPtr()
						&& typestack[typestack.size()-2].obj ==Class<Integer>::getRef(mi->context->root->getSystemState()).getPtr());
				removetypestack(typestack,2);
				if (!setupInstructionTwoArgumentsNoResult(operandlist,mi,intcomparison ? ABC_OP_OPTIMZED_IFNGE_INT : ABC_OP_OPTIMZED_IFNGE,opcode,code,oldnewpositions, jumptargets))
				{
					if (intcomparison)
						mi->body->preloadedcode.at(mi->body->preloadedcode.size()-1).data=uint32_t(ABC_OP_OPTIMZED_IFNGE_INT_SIMPLE);
				}
				jumppositions[mi->body->preloadedcode.size()-1] = code.reads24();
				jumpstartpositions[mi->body->preloadedcode.size()-1] = code.tellg();
				clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				break;
			}
			case 0x0d://ifnle
			case 0x0e://ifngt
			{
				removetypestack(typestack,2);
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
					opcode_skipped=true;
				}
				break;
			}
			case 0x11://iftrue
			{
				removetypestack(typestack,1);
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
				removetypestack(typestack,1);
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
				removetypestack(typestack,2);
				setupInstructionTwoArgumentsNoResult(operandlist,mi,ABC_OP_OPTIMZED_IFEQ,opcode,code,oldnewpositions, jumptargets);
				jumppositions[mi->body->preloadedcode.size()-1] = code.reads24();
				jumpstartpositions[mi->body->preloadedcode.size()-1] = code.tellg();
				clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				break;
			case 0x14://ifne
			{
				bool intcomparison = (typestack.size() >=2 
						&& typestack[typestack.size()-1].obj ==Class<Integer>::getRef(mi->context->root->getSystemState()).getPtr()
						&& typestack[typestack.size()-2].obj ==Class<Integer>::getRef(mi->context->root->getSystemState()).getPtr());
				removetypestack(typestack,2);
				if (!setupInstructionTwoArgumentsNoResult(operandlist,mi,intcomparison ? ABC_OP_OPTIMZED_IFNE_INT : ABC_OP_OPTIMZED_IFNE,opcode,code,oldnewpositions, jumptargets))
				{
					if (intcomparison)
						mi->body->preloadedcode.at(mi->body->preloadedcode.size()-1).data=uint32_t(ABC_OP_OPTIMZED_IFNE_INT_SIMPLE);
				}
				jumppositions[mi->body->preloadedcode.size()-1] = code.reads24();
				jumpstartpositions[mi->body->preloadedcode.size()-1] = code.tellg();
				clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				break;
			}
			case 0x15://iflt
				removetypestack(typestack,2);
				setupInstructionTwoArgumentsNoResult(operandlist,mi,ABC_OP_OPTIMZED_IFLT,opcode,code,oldnewpositions, jumptargets);
				jumppositions[mi->body->preloadedcode.size()-1] = code.reads24();
				jumpstartpositions[mi->body->preloadedcode.size()-1] = code.tellg();
				clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				break;
			case 0x16://ifle
				removetypestack(typestack,2);
				setupInstructionTwoArgumentsNoResult(operandlist,mi,ABC_OP_OPTIMZED_IFLE,opcode,code,oldnewpositions, jumptargets);
				jumppositions[mi->body->preloadedcode.size()-1] = code.reads24();
				jumpstartpositions[mi->body->preloadedcode.size()-1] = code.tellg();
				clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				break;
			case 0x17://ifgt
				removetypestack(typestack,2);
				setupInstructionTwoArgumentsNoResult(operandlist,mi,ABC_OP_OPTIMZED_IFGT,opcode,code,oldnewpositions, jumptargets);
				jumppositions[mi->body->preloadedcode.size()-1] = code.reads24();
				jumpstartpositions[mi->body->preloadedcode.size()-1] = code.tellg();
				clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				break;
			case 0x18://ifge
				removetypestack(typestack,2);
				setupInstructionTwoArgumentsNoResult(operandlist,mi,ABC_OP_OPTIMZED_IFGE,opcode,code,oldnewpositions, jumptargets);
				jumppositions[mi->body->preloadedcode.size()-1] = code.reads24();
				jumpstartpositions[mi->body->preloadedcode.size()-1] = code.tellg();
				clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				break;
			case 0x19://ifstricteq
				removetypestack(typestack,2);
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
				removetypestack(typestack,1);
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
				typestack.push_back(typestackentry(nullptr,false));
				break;
			case 0x21://pushundefined
				mi->body->preloadedcode.push_back((uint32_t)opcode);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				if (jumptargets.find(code.tellg()) == jumptargets.end())
					operandlist.push_back(operands(OP_UNDEFINED, nullptr,0,1,mi->body->preloadedcode.size()-1));
				else
					clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				typestack.push_back(typestackentry(nullptr,false));
				break;
			case 0x24://pushbyte
			{
				int32_t p = code.tellg();
				mi->body->preloadedcode.push_back((uint32_t)opcode);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				int32_t value = (int32_t)(int8_t)code.readbyte();
				uint8_t index = value;
				mi->body->preloadedcode.push_back(value);
				if (jumptargets.find(p) != jumptargets.end())
					clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				operandlist.push_back(operands(OP_BYTE,Class<Integer>::getRef(mi->context->root->getSystemState()).getPtr(),index,2,mi->body->preloadedcode.size()-2));
				typestack.push_back(typestackentry(Class<Integer>::getRef(function->getSystemState()).getPtr(),false));
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
				if (jumptargets.find(p) != jumptargets.end())
					clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				operandlist.push_back(operands(OP_SHORT,Class<Integer>::getRef(mi->context->root->getSystemState()).getPtr(),index,2,mi->body->preloadedcode.size()-2));
				typestack.push_back(typestackentry(Class<Integer>::getRef(function->getSystemState()).getPtr(),false));
				break;
			}
			case 0x26://pushtrue
			{
				int32_t p = code.tellg();
				mi->body->preloadedcode.push_back((uint32_t)opcode);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				if (jumptargets.find(p) != jumptargets.end())
					clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				operandlist.push_back(operands(OP_TRUE, Class<Boolean>::getRef(mi->context->root->getSystemState()).getPtr(),0,1,mi->body->preloadedcode.size()-1));
				typestack.push_back(typestackentry(Class<Boolean>::getRef(function->getSystemState()).getPtr(),false));
				break;
			}
			case 0x27://pushfalse
			{
				int32_t p = code.tellg();
				mi->body->preloadedcode.push_back((uint32_t)opcode);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				if (jumptargets.find(p) != jumptargets.end())
					clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				operandlist.push_back(operands(OP_FALSE, Class<Boolean>::getRef(mi->context->root->getSystemState()).getPtr(),0,1,mi->body->preloadedcode.size()-1));
				typestack.push_back(typestackentry(Class<Boolean>::getRef(function->getSystemState()).getPtr(),false));
				break;
			}
			case 0x28://pushnan
			{
				int32_t p = code.tellg();
				mi->body->preloadedcode.push_back((uint32_t)opcode);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				if (jumptargets.find(p) != jumptargets.end())
					clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				operandlist.push_back(operands(OP_NAN, Class<Number>::getRef(mi->context->root->getSystemState()).getPtr(),0,1,mi->body->preloadedcode.size()-1));
				typestack.push_back(typestackentry(Class<Number>::getRef(function->getSystemState()).getPtr(),false));
				break;
			}
			case 0x2c://pushstring
			{
				int32_t p = code.tellg();
				mi->body->preloadedcode.push_back((uint32_t)opcode);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				int32_t value = code.readu30();
				mi->body->preloadedcode.push_back(value);
				if (jumptargets.find(p) != jumptargets.end())
					clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				operandlist.push_back(operands(OP_STRING,Class<ASString>::getRef(mi->context->root->getSystemState()).getPtr(),value,2,mi->body->preloadedcode.size()-2));
				typestack.push_back(typestackentry(Class<ASString>::getRef(function->getSystemState()).getPtr(),false));
				break;
			}
			case 0x2d://pushint
			{
				int32_t p = code.tellg();
				mi->body->preloadedcode.push_back((uint32_t)opcode);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				int32_t value = code.readu30();
				mi->body->preloadedcode.push_back(value);
				if (jumptargets.find(p) != jumptargets.end())
					clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				operandlist.push_back(operands(OP_INTEGER,Class<Integer>::getRef(mi->context->root->getSystemState()).getPtr(),value,2,mi->body->preloadedcode.size()-2));
				typestack.push_back(typestackentry(Class<Integer>::getRef(function->getSystemState()).getPtr(),false));
				break;
			}
			case 0x2e://pushuint
			{
				int32_t p = code.tellg();
				mi->body->preloadedcode.push_back((uint32_t)opcode);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				int32_t value = code.readu30();
				mi->body->preloadedcode.push_back(value);
				if (jumptargets.find(p) != jumptargets.end())
					clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				operandlist.push_back(operands(OP_UINTEGER,Class<UInteger>::getRef(mi->context->root->getSystemState()).getPtr(),value,2,mi->body->preloadedcode.size()-2));
				typestack.push_back(typestackentry(Class<UInteger>::getRef(function->getSystemState()).getPtr(),false));
				break;
			}
			case 0x2f://pushdouble
			{
				int32_t p = code.tellg();
				mi->body->preloadedcode.push_back((uint32_t)opcode);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				int32_t value = code.readu30();
				mi->body->preloadedcode.push_back(value);
				if (jumptargets.find(p) != jumptargets.end())
					clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				operandlist.push_back(operands(OP_DOUBLE,Class<Number>::getRef(mi->context->root->getSystemState()).getPtr(),value,2,mi->body->preloadedcode.size()-2));
				typestack.push_back(typestackentry(Class<Number>::getRef(function->getSystemState()).getPtr(),false));
				break;
			}
			case 0x30://pushscope
				removetypestack(typestack,1);
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
				if (jumptargets.find(p) != jumptargets.end())
					clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				operandlist.push_back(operands(OP_NAMESPACE,Class<Namespace>::getRef(mi->context->root->getSystemState()).getPtr(),value,2,mi->body->preloadedcode.size()-2));
				typestack.push_back(typestackentry(Class<Namespace>::getRef(function->getSystemState()).getPtr(),false));
				break;
			}
			case 0x32://hasnext2
			{
				mi->body->preloadedcode.push_back((uint32_t)opcode);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				mi->body->preloadedcode.push_back(code.readu30());
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				mi->body->preloadedcode.push_back(code.readu30());
				clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				typestack.push_back(typestackentry(Class<Boolean>::getRef(function->getSystemState()).getPtr(),false));
				break;
			}
			case 0x43://callmethod
			case 0x44://callstatic
			{
				mi->body->preloadedcode.push_back((uint32_t)opcode);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				mi->body->preloadedcode.push_back(code.readu30());
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				int32_t t = code.readu30();
				mi->body->preloadedcode.push_back(t);
				clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				removetypestack(typestack,t+1);
				typestack.push_back(typestackentry(nullptr,false));
				break;
			}
			case 0x45://callsuper
			case 0x4c://callproplex
			{
				mi->body->preloadedcode.push_back((uint32_t)opcode);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				int32_t t = code.readu30();
				mi->body->preloadedcode.push_back(t);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				int32_t t2 = code.readu30();
				mi->body->preloadedcode.push_back(t2);
				clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				removetypestack(typestack,mi->context->constant_pool.multinames[t].runtimeargs+t2+1);
				typestack.push_back(typestackentry(nullptr,false));
				break;
			}
			case 0x4e://callsupervoid
			{
				mi->body->preloadedcode.push_back((uint32_t)opcode);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				int32_t t = code.readu30();
				mi->body->preloadedcode.push_back(t);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				int32_t t2 = code.readu30();
				mi->body->preloadedcode.push_back(t2);
				clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				removetypestack(typestack,mi->context->constant_pool.multinames[t].runtimeargs+t2+1);
				break;
			}
			case 0x35://li8
				setupInstructionOneArgument(operandlist,mi,ABC_OP_OPTIMZED_LI8,opcode,code,oldnewpositions, jumptargets,true,true,localtypes, defaultlocaltypes, Class<Integer>::getRef(function->getSystemState()).getPtr(),code.tellg(),true);
				removetypestack(typestack,1);
				typestack.push_back(typestackentry(Class<Integer>::getRef(function->getSystemState()).getPtr(),false));
				break;
			case 0x36://li16
				setupInstructionOneArgument(operandlist,mi,ABC_OP_OPTIMZED_LI16,opcode,code,oldnewpositions, jumptargets,true,true,localtypes, defaultlocaltypes, Class<Integer>::getRef(function->getSystemState()).getPtr(),code.tellg(),true);
				removetypestack(typestack,1);
				typestack.push_back(typestackentry(Class<Integer>::getRef(function->getSystemState()).getPtr(),false));
				break;
			case 0x37://li32
				setupInstructionOneArgument(operandlist,mi,ABC_OP_OPTIMZED_LI32,opcode,code,oldnewpositions, jumptargets,true,true,localtypes, defaultlocaltypes, Class<Integer>::getRef(function->getSystemState()).getPtr(),code.tellg(),true);
				removetypestack(typestack,1);
				typestack.push_back(typestackentry(Class<Integer>::getRef(function->getSystemState()).getPtr(),false));
				break;
			case 0x38://lf32
				setupInstructionOneArgument(operandlist,mi,ABC_OP_OPTIMZED_LF32,opcode,code,oldnewpositions, jumptargets,true,true,localtypes, defaultlocaltypes, Class<Number>::getRef(function->getSystemState()).getPtr(),code.tellg(),true);
				removetypestack(typestack,1);
				typestack.push_back(typestackentry(Class<Number>::getRef(function->getSystemState()).getPtr(),false));
				break;
			case 0x39://lf64
				setupInstructionOneArgument(operandlist,mi,ABC_OP_OPTIMZED_LF64,opcode,code,oldnewpositions, jumptargets,true,true,localtypes, defaultlocaltypes, Class<Number>::getRef(function->getSystemState()).getPtr(),code.tellg(),true);
				removetypestack(typestack,1);
				typestack.push_back(typestackentry(Class<Number>::getRef(function->getSystemState()).getPtr(),false));
				break;
			case 0x3a://si8
				setupInstructionTwoArgumentsNoResult(operandlist,mi,ABC_OP_OPTIMZED_SI8,opcode,code,oldnewpositions, jumptargets);
				removetypestack(typestack,2);
				break;
			case 0x3b://si16
				setupInstructionTwoArgumentsNoResult(operandlist,mi,ABC_OP_OPTIMZED_SI16,opcode,code,oldnewpositions, jumptargets);
				removetypestack(typestack,2);
				break;
			case 0x3c://si32
				setupInstructionTwoArgumentsNoResult(operandlist,mi,ABC_OP_OPTIMZED_SI32,opcode,code,oldnewpositions, jumptargets);
				removetypestack(typestack,2);
				break;
			case 0x3d://sf32
				setupInstructionTwoArgumentsNoResult(operandlist,mi,ABC_OP_OPTIMZED_SF32,opcode,code,oldnewpositions, jumptargets);
				removetypestack(typestack,2);
				break;
			case 0x3e://sf64
				setupInstructionTwoArgumentsNoResult(operandlist,mi,ABC_OP_OPTIMZED_SF64,opcode,code,oldnewpositions, jumptargets);
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
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size()+1;
				uint32_t argcount = code.readu30();
				removetypestack(typestack,argcount+1);
				if (jumptargets.find(p) == jumptargets.end())
				{
					switch (argcount)
					{
						case 0:
						{
							Class_base* restype = operandlist.size()> 0 ? operandlist.back().objtype : nullptr;
							if (!setupInstructionOneArgument(operandlist,mi,ABC_OP_OPTIMZED_CONSTRUCT_NOARGS,opcode,code,oldnewpositions, jumptargets,true,false,localtypes, defaultlocaltypes,restype,p,true))
								mi->body->preloadedcode.push_back(argcount);
							typestack.push_back(typestackentry(restype,false));
							break;
						}
						default:
							// TODO handle construct with one or more arguments
							mi->body->preloadedcode.push_back((uint32_t)opcode);
							clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
							mi->body->preloadedcode.push_back(argcount);
							typestack.push_back(typestackentry(nullptr,false));
							break;
					}
					break;
				}
				else
				{
					mi->body->preloadedcode.push_back((uint32_t)opcode);
					clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
					mi->body->preloadedcode.push_back(argcount);
					typestack.push_back(typestackentry(nullptr,false));
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
				else
				{
					if (!typestack.back().obj
							|| !typestack.back().obj->is<Class_base>() 
							|| (dynamic_cast<const Class_base*>(mi->returnType) && !typestack.back().obj->as<Class_base>()->isSubClass(dynamic_cast<const Class_base*>(mi->returnType))))
					{
						coercereturnvalue = true;
					}
				}
				setupInstructionOneArgumentNoResult(operandlist,mi,ABC_OP_OPTIMZED_RETURNVALUE,opcode,code,oldnewpositions, jumptargets,p);
				// skip unreachable code
				while (!code.atend() && jumptargets.find(code.tellg()+1) == jumptargets.end())
					code.readbyte();
				removetypestack(typestack,1);
				break;
			}
			case 0x4a://constructprop
			{
				int32_t p = code.tellg();
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size()+1;
				uint32_t t = code.readu30();
				assert_and_throw(t < mi->context->constant_pool.multiname_count);
				uint32_t argcount = code.readu30();
				removetypestack(typestack,argcount+mi->context->constant_pool.multinames[t].runtimeargs+1);
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
									typestack.push_back(typestackentry(resulttype,false));
									break;
								}
								default:
									// TODO handle constructprop with one or more arguments
									mi->body->preloadedcode.push_back((uint32_t)opcode);
									clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
									mi->body->preloadedcode.push_back(t);
									mi->body->preloadedcode.push_back(argcount);
									typestack.push_back(typestackentry(nullptr,false));
									break;
							}
							break;
						default:
							mi->body->preloadedcode.push_back((uint32_t)opcode);
							clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
							mi->body->preloadedcode.push_back(t);
							mi->body->preloadedcode.push_back(argcount);
							typestack.push_back(typestackentry(nullptr,false));
							break;
					}
				}
				else
				{
					mi->body->preloadedcode.push_back((uint32_t)opcode);
					clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
					mi->body->preloadedcode.push_back(t);
					mi->body->preloadedcode.push_back(argcount);
					typestack.push_back(typestackentry(nullptr,false));
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
					Class_base* resulttype=nullptr;
					bool skipcoerce = false;
					SyntheticFunction* func= nullptr;
					variable* v = nullptr;
					if (typestack.size() >= argcount+mi->context->constant_pool.multinames[t].runtimeargs+1)
					{
						if (canCallFunctionDirect(typestack[typestack.size()-(argcount+mi->context->constant_pool.multinames[t].runtimeargs+1)].obj,name))
						{
							v = typestack[typestack.size()-(argcount+mi->context->constant_pool.multinames[t].runtimeargs+1)].classvar ?
										typestack[typestack.size()-(argcount+mi->context->constant_pool.multinames[t].runtimeargs+1)].obj->findVariableByMultiname(
										*name,
										typestack[typestack.size()-(argcount+mi->context->constant_pool.multinames[t].runtimeargs+1)].obj->as<Class_base>(),
										nullptr,nullptr,false):
										typestack[typestack.size()-(argcount+mi->context->constant_pool.multinames[t].runtimeargs+1)].obj->as<Class_base>()->getBorrowedVariableByMultiname(*name);
							if (v)
							{
								if (asAtomHandler::is<SyntheticFunction>(v->var))
								{
									func = asAtomHandler::as<SyntheticFunction>(v->var);
									if (!func->getMethodInfo()->returnType)
										func->checkParamTypes();
									resulttype = func->getReturnType();
									if (func->getMethodInfo()->paramTypes.size() >= argcount)
									{
										skipcoerce=true;
										auto it2 = typestack.rbegin();
										for(uint32_t i= argcount; i > 0; i--)
										{
											if (!(*it2).obj || !(*it2).obj->is<Class_base>() || !func->canSkipCoercion(i-1,(*it2).obj->as<Class_base>()))
											{
												skipcoerce=false;
												break;
											}
											it2++;
										}
									}
								}
							}
						}
					}
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
											if (v && asAtomHandler::is<IFunction>(v->var))
											{
												if (opcode == 0x46)
													setupInstructionOneArgument(operandlist,mi,ABC_OP_OPTIMZED_CALLFUNCTION_NOARGS ,opcode,code,oldnewpositions, jumptargets,true, false,localtypes, defaultlocaltypes,resulttype,p,true);
												else
													setupInstructionOneArgumentNoResult(operandlist,mi,ABC_OP_OPTIMZED_CALLFUNCTION_NOARGS_VOID,opcode,code,oldnewpositions, jumptargets,p);
												mi->body->preloadedcode.at(mi->body->preloadedcode.size()-1).cacheobj2 = asAtomHandler::getObject(v->var);
												removetypestack(typestack,argcount+mi->context->constant_pool.multinames[t].runtimeargs+1);
												if (opcode == 0x46)
													typestack.push_back(typestackentry(resulttype,false));
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
										if (func)
										{
											mi->body->preloadedcode.at(mi->body->preloadedcode.size()-1).data= (opcode == 0x4f ? ABC_OP_OPTIMZED_CALLFUNCTIONVOIDSYNTHETIC_STATICNAME_MULTIARGS:ABC_OP_OPTIMZED_CALLFUNCTIONSYNTHETIC_STATICNAME_MULTIARGS) | (argcount<<OPCODE_SIZE) | ABC_OP_COERCED;
											mi->body->preloadedcode.at(mi->body->preloadedcode.size()-1).cacheobj3 = func;
										}
										else
										{
											mi->body->preloadedcode.at(mi->body->preloadedcode.size()-1).data= (opcode == 0x4f ? ABC_OP_OPTIMZED_CALLPROPVOID_STATICNAME_MULTIARGS:ABC_OP_OPTIMZED_CALLPROPERTY_STATICNAME_MULTIARGS) | (argcount<<OPCODE_SIZE) | ABC_OP_COERCED;
											mi->body->preloadedcode.at(mi->body->preloadedcode.size()-1).cachedmultiname2 = name;
										}
										clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
									}
									removetypestack(typestack,argcount+mi->context->constant_pool.multinames[t].runtimeargs+1);
									if (opcode == 0x46)
										typestack.push_back(typestackentry(resulttype,false));
									break;
								case 1:
								{
									if (typestack.size() > 1 &&
											typestack[typestack.size()-2].obj != nullptr &&
											(typestack[typestack.size()-2].obj->is<Global>() ||
											 (typestack[typestack.size()-2].obj->is<Class_base>() && typestack[typestack.size()-2].obj->as<Class_base>()->isSealed)))
									{
										asAtom func = asAtomHandler::invalidAtom;
										typestack[typestack.size()-2].obj->getVariableByMultiname(func,*name,GET_VARIABLE_OPTION(DONT_CALL_GETTER|FROM_GETLEX|NO_INCREF));
										if (asAtomHandler::isClass(func) && asAtomHandler::as<Class_base>(func)->isBuiltin())
										{
											// function is a class generator, we can use it as the result type
											resulttype = asAtomHandler::as<Class_base>(func);
										}
										else if (asAtomHandler::is<SyntheticFunction>(func))
										{
											SyntheticFunction* f = asAtomHandler::as<SyntheticFunction>(func);
											if (!f->getMethodInfo()->returnType)
												f->checkParamTypes();
										}
									}
									if (opcode == 0x4f && operandlist.size() > 1)
									{
										auto it = operandlist.rbegin();
										Class_base* argtype = it->objtype;
										it++;
										if (canCallFunctionDirect((*it),name))
										{
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
													removetypestack(typestack,argcount+mi->context->constant_pool.multinames[t].runtimeargs+1);
													break;
												}
												if (asAtomHandler::is<Function>(v->var))
												{
													setupInstructionTwoArgumentsNoResult(operandlist,mi,ABC_OP_OPTIMZED_CALLFUNCTIONBUILTIN_ONEARG_VOID,opcode,code,oldnewpositions, jumptargets);
													mi->body->preloadedcode.at(mi->body->preloadedcode.size()-1).cacheobj3 = asAtomHandler::getObject(v->var);
													removetypestack(typestack,argcount+mi->context->constant_pool.multinames[t].runtimeargs+1);
													break;
												}
											}
										}
									}
									if ((opcode == 0x4f && setupInstructionTwoArgumentsNoResult(operandlist,mi,ABC_OP_OPTIMZED_CALLPROPVOID_STATICNAME,opcode,code,oldnewpositions, jumptargets)) ||
									   ((opcode == 0x46 && setupInstructionTwoArguments(operandlist,mi,ABC_OP_OPTIMZED_CALLPROPERTY_STATICNAME,opcode,code,oldnewpositions, jumptargets,false,false,true,localtypes, defaultlocaltypes,p))))
									{
										mi->body->preloadedcode.at(mi->body->preloadedcode.size()-1).data|=(skipcoerce ? ABC_OP_COERCED : 0);
										mi->body->preloadedcode.push_back(t);
										mi->body->preloadedcode.at(mi->body->preloadedcode.size()-1).cachedmultiname2 = name;
									}
									else if (opcode == 0x46 && checkForLocalResult(operandlist,mi,code,oldnewpositions,jumptargets,0,localtypes,nullptr, defaultlocaltypes))
									{
										mi->body->preloadedcode.at(mi->body->preloadedcode.size()-1).data=ABC_OP_OPTIMZED_CALLPROPERTY_STATICNAME_LOCALRESULT| (skipcoerce ? ABC_OP_COERCED : 0);
										mi->body->preloadedcode.push_back(t);
										mi->body->preloadedcode.at(mi->body->preloadedcode.size()-1).cachedmultiname2 = name;
									}
									else
									{
										if (func)
										{
											mi->body->preloadedcode.at(mi->body->preloadedcode.size()-1).data= (opcode == 0x4f ? ABC_OP_OPTIMZED_CALLFUNCTIONVOIDSYNTHETIC_STATICNAME_MULTIARGS:ABC_OP_OPTIMZED_CALLFUNCTIONSYNTHETIC_STATICNAME_MULTIARGS) | (argcount<<OPCODE_SIZE)| (skipcoerce ? ABC_OP_COERCED : 0);
											mi->body->preloadedcode.at(mi->body->preloadedcode.size()-1).cacheobj3 = func;
										}
										else
										{
											mi->body->preloadedcode.at(mi->body->preloadedcode.size()-1).data= (opcode == 0x4f ? ABC_OP_OPTIMZED_CALLPROPVOID_STATICNAME_MULTIARGS:ABC_OP_OPTIMZED_CALLPROPERTY_STATICNAME_MULTIARGS) | (argcount<<OPCODE_SIZE)| (skipcoerce ? ABC_OP_COERCED : 0);
											mi->body->preloadedcode.at(mi->body->preloadedcode.size()-1).cachedmultiname2 = name;
										}
										clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
									}
									removetypestack(typestack,argcount+mi->context->constant_pool.multinames[t].runtimeargs+1);
									if (opcode == 0x46)
										typestack.push_back(typestackentry(resulttype,false));
									break;
								}
								default:
									if (operandlist.size() >= argcount)
									{
										mi->body->preloadedcode.push_back((uint32_t)(opcode == 0x4f ? ABC_OP_OPTIMZED_CALLPROPVOID_STATICNAME_MULTIARGS_CACHED:ABC_OP_OPTIMZED_CALLPROPERTY_STATICNAME_MULTIARGS_CACHED) | (argcount<<OPCODE_SIZE)| (skipcoerce ? ABC_OP_COERCED : 0));
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
												if (v)
												{
													if (asAtomHandler::is<SyntheticFunction>(v->var))
													{
														mi->body->preloadedcode.at(oppos).data = (opcode == 0x4f ? ABC_OP_OPTIMZED_CALLFUNCTIONSYNTHETIC_MULTIARGS_VOID : ABC_OP_OPTIMZED_CALLFUNCTIONSYNTHETIC_MULTIARGS) | (argcount<<OPCODE_SIZE)| (skipcoerce ? ABC_OP_COERCED : 0);
														if (skipcoerce)
															mi->body->preloadedcode.at(oppos).data |= ABC_OP_COERCED;
														it->fillCode(0,mi->body->preloadedcode.at(oppos),mi->context,true);
														it->removeArg(mi);
														oppos = mi->body->preloadedcode.size()-1-argcount;
														mi->body->preloadedcode.at(oppos).cacheobj3 = asAtomHandler::getObject(v->var);
														clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
														if (opcode == 0x46)
															checkForLocalResult(operandlist,mi,code,oldnewpositions,jumptargets,2,localtypes,nullptr, defaultlocaltypes,oppos,mi->body->preloadedcode.size()-1);
														removetypestack(typestack,argcount+mi->context->constant_pool.multinames[t].runtimeargs+1);
														if (opcode == 0x46)
															typestack.push_back(typestackentry(resulttype,false));
														break;
													}
													else if (asAtomHandler::is<Function>(v->var))
													{
														mi->body->preloadedcode.at(oppos).data = (opcode == 0x4f ? ABC_OP_OPTIMZED_CALLFUNCTIONBUILTIN_MULTIARGS_VOID : ABC_OP_OPTIMZED_CALLFUNCTIONBUILTIN_MULTIARGS) | (argcount<<OPCODE_SIZE)| (skipcoerce ? ABC_OP_COERCED : 0);
														it->fillCode(0,mi->body->preloadedcode.at(oppos),mi->context,true);
														it->removeArg(mi);
														oppos = mi->body->preloadedcode.size()-1-argcount;
														mi->body->preloadedcode.at(oppos).cacheobj3 = asAtomHandler::getObject(v->var);
														clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
														if (opcode == 0x46)
															checkForLocalResult(operandlist,mi,code,oldnewpositions,jumptargets,2,localtypes,nullptr, defaultlocaltypes,oppos,mi->body->preloadedcode.size()-1);
														removetypestack(typestack,argcount+mi->context->constant_pool.multinames[t].runtimeargs+1);
														if (opcode == 0x46)
															typestack.push_back(typestackentry(resulttype,false));
														break;
													}
												}
											}
											mi->body->preloadedcode.at(oppos).data = (opcode == 0x4f ? ABC_OP_OPTIMZED_CALLPROPVOID_STATICNAME_MULTIARGS_CACHED_CALLER:ABC_OP_OPTIMZED_CALLPROPERTY_STATICNAME_MULTIARGS_CACHED_CALLER) | (argcount<<OPCODE_SIZE)| (skipcoerce ? ABC_OP_COERCED : 0);
											
											it->removeArg(mi);
											oppos = mi->body->preloadedcode.size()-1-argcount;
											if (it->fillCode(1,mi->body->preloadedcode.at(mi->body->preloadedcode.size()-1),mi->context,false))
												mi->body->preloadedcode.at(oppos).data++;
											clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
											if (opcode == 0x46)
												checkForLocalResult(operandlist,mi,code,oldnewpositions,jumptargets,2,localtypes,nullptr, defaultlocaltypes,oppos,mi->body->preloadedcode.size()-1);
											removetypestack(typestack,argcount+mi->context->constant_pool.multinames[t].runtimeargs+1);
											if (opcode == 0x46)
												typestack.push_back(typestackentry(resulttype,false));
											break;
										}
										clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
										if (opcode == 0x46)
											checkForLocalResult(operandlist,mi,code,oldnewpositions,jumptargets,1,localtypes,nullptr, defaultlocaltypes,mi->body->preloadedcode.size()-1-argcount,mi->body->preloadedcode.size()-1);
										removetypestack(typestack,argcount+mi->context->constant_pool.multinames[t].runtimeargs+1);
										if (opcode == 0x46)
											typestack.push_back(typestackentry(resulttype,false));
									}
									else
									{
										if (func)
										{
											mi->body->preloadedcode.push_back((uint32_t)(opcode == 0x4f ? ABC_OP_OPTIMZED_CALLFUNCTIONVOIDSYNTHETIC_STATICNAME_MULTIARGS:ABC_OP_OPTIMZED_CALLFUNCTIONSYNTHETIC_STATICNAME_MULTIARGS) | (argcount<<OPCODE_SIZE)| (skipcoerce ? ABC_OP_COERCED : 0));
											mi->body->preloadedcode.at(mi->body->preloadedcode.size()-1).cacheobj3 = func;
										}
										else
										{
											mi->body->preloadedcode.push_back((uint32_t)(opcode == 0x4f ? ABC_OP_OPTIMZED_CALLPROPVOID_STATICNAME_MULTIARGS:ABC_OP_OPTIMZED_CALLPROPERTY_STATICNAME_MULTIARGS) | (argcount<<OPCODE_SIZE)| (skipcoerce ? ABC_OP_COERCED : 0));
											mi->body->preloadedcode.at(mi->body->preloadedcode.size()-1).cachedmultiname2 = name;
										}
										clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
										removetypestack(typestack,argcount+mi->context->constant_pool.multinames[t].runtimeargs+1);
										if (opcode == 0x46)
											typestack.push_back(typestackentry(resulttype,false));
									}
									break;
							}
							break;
						default:
							mi->body->preloadedcode.push_back((uint32_t)opcode);
							clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
							mi->body->preloadedcode.push_back(t);
							mi->body->preloadedcode.push_back(argcount);
							if (opcode == 0x46)
								typestack.push_back(typestackentry(resulttype,false));
							break;
					}
				}
				else
				{
					mi->body->preloadedcode.push_back((uint32_t)opcode);
					clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
					mi->body->preloadedcode.push_back(t);
					mi->body->preloadedcode.push_back(argcount);
					if (opcode == 0x46)
						typestack.push_back(typestackentry(nullptr,false));
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
				typestack.push_back(typestackentry(nullptr,false));
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
										removetypestack(typestack,mi->context->constant_pool.multinames[t].runtimeargs+1);
										typestack.push_back(typestackentry(asAtomHandler::getClass(v->var,function->getSystemState()),false));
										break;
									}
									if (v && asAtomHandler::isValid(v->getter))
									{
										Class_base* resulttype = (Class_base*)(v->isResolved ? dynamic_cast<const Class_base*>(v->type):nullptr);
										if (!setupInstructionOneArgument(operandlist,mi,ABC_OP_OPTIMZED_CALLFUNCTION_NOARGS,opcode,code,oldnewpositions, jumptargets,true, false,localtypes, defaultlocaltypes,resulttype,p,true))
											lastlocalresulttype = resulttype;
										mi->body->preloadedcode.at(mi->body->preloadedcode.size()-1).cacheobj2 = asAtomHandler::getObject(v->getter);
										addname = false;
										removetypestack(typestack,mi->context->constant_pool.multinames[t].runtimeargs+1);
										typestack.push_back(typestackentry(resulttype,false));
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
											mi->body->preloadedcode.at(mi->body->preloadedcode.size()-1).data |=(v->slotid-1)<<OPCODE_SIZE;
											addname = false;
											removetypestack(typestack,mi->context->constant_pool.multinames[t].runtimeargs+1);
											typestack.push_back(typestackentry(resulttype,false));
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
										removetypestack(typestack,mi->context->constant_pool.multinames[t].runtimeargs+1);
										typestack.push_back(typestackentry(resulttype,false));
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
											mi->body->preloadedcode.at(mi->body->preloadedcode.size()-1).data |=(v->slotid-1)<<OPCODE_SIZE;
											if (operandlist.empty()) // indicates that checkforlocalresult returned false
												lastlocalresulttype = resulttype;
											addname = false;
											ASATOM_DECREF(o);
											removetypestack(typestack,mi->context->constant_pool.multinames[t].runtimeargs+1);
											typestack.push_back(typestackentry(resulttype,false));
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
											removetypestack(typestack,mi->context->constant_pool.multinames[t].runtimeargs+1);
											typestack.push_back(typestackentry(resulttype,false));
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
							removetypestack(typestack,mi->context->constant_pool.multinames[t].runtimeargs+1);
							typestack.push_back(typestackentry(nullptr,false));
							break;
						}
						case 1:
							if (operandlist.size() > 0 && (operandlist.back().type == OP_LOCAL || operandlist.back().type == OP_CACHED_CONSTANT)
									&& operandlist.back().objtype == Class<Integer>::getRef(function->getSystemState()).getPtr())
							{
								addname = !setupInstructionTwoArguments(operandlist,mi,ABC_OP_OPTIMZED_GETPROPERTY_INTEGER,opcode,code,oldnewpositions, jumptargets,false,false,true,localtypes, defaultlocaltypes,p);
							}
							else if (operandlist.size() == 0 && typestack.size() > 0 && typestack.back().obj == Class<Integer>::getRef(function->getSystemState()).getPtr())
							{
								mi->body->preloadedcode.push_back((uint32_t)ABC_OP_OPTIMZED_GETPROPERTY_INTEGER_SIMPLE);
								clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
								addname=false;
							}
							else
								setupInstructionTwoArguments(operandlist,mi,ABC_OP_OPTIMZED_GETPROPERTY,opcode,code,oldnewpositions, jumptargets,false,false,true,localtypes, defaultlocaltypes,p);
							removetypestack(typestack,mi->context->constant_pool.multinames[t].runtimeargs+1);
							typestack.push_back(typestackentry(nullptr,false));
							break;
						default:
							mi->body->preloadedcode.push_back((uint32_t)opcode);
							clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
							removetypestack(typestack,mi->context->constant_pool.multinames[t].runtimeargs+1);
							typestack.push_back(typestackentry(nullptr,false));
							break;
					}
				}
				else
				{
					mi->body->preloadedcode.push_back((uint32_t)opcode);
					clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
					removetypestack(typestack,mi->context->constant_pool.multinames[t].runtimeargs+1);
					typestack.push_back(typestackentry(nullptr,false));
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
				removetypestack(typestack,1);
				typestack.push_back(typestackentry(Class<Integer>::getRef(mi->context->root->getSystemState()).getPtr(),false));
				break;
			case 0x74://convert_u
				if (jumptargets.find(code.tellg()) == jumptargets.end() && operandlist.size() > 0 && operandlist.back().objtype == Class<UInteger>::getRef(mi->context->root->getSystemState()).getPtr())
					oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				else
					setupInstructionOneArgument(operandlist,mi,ABC_OP_OPTIMZED_CONVERTU,opcode,code,oldnewpositions, jumptargets,true,true,localtypes, defaultlocaltypes, Class<UInteger>::getRef(function->getSystemState()).getPtr(),code.tellg(),true);
				removetypestack(typestack,1);
				typestack.push_back(typestackentry(Class<UInteger>::getRef(mi->context->root->getSystemState()).getPtr(),false));
				break;
			case 0x75://convert_d
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				if (jumptargets.find(code.tellg()) == jumptargets.end() && operandlist.size() > 0 && operandlist.back().objtype == Class<Number>::getRef(mi->context->root->getSystemState()).getPtr())
					oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				else
					setupInstructionOneArgument(operandlist,mi,ABC_OP_OPTIMZED_CONVERTD,opcode,code,oldnewpositions, jumptargets,true,true,localtypes, defaultlocaltypes, Class<Number>::getRef(function->getSystemState()).getPtr(),code.tellg(),true);
				removetypestack(typestack,1);
				typestack.push_back(typestackentry(Class<Number>::getRef(mi->context->root->getSystemState()).getPtr(),false));
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
				removetypestack(typestack,1);
				typestack.push_back(typestackentry(Class<Boolean>::getRef(mi->context->root->getSystemState()).getPtr(),false));
				break;
			case 0x91://increment
				setupInstructionOneArgument(operandlist,mi,ABC_OP_OPTIMZED_INCREMENT,opcode,code,oldnewpositions, jumptargets,false,true,localtypes, defaultlocaltypes, Class<Number>::getRef(function->getSystemState()).getPtr(),code.tellg(),dup_indicator == 0);
				dup_indicator=0;
				removetypestack(typestack,1);
				typestack.push_back(typestackentry(Class<Number>::getRef(mi->context->root->getSystemState()).getPtr(),false));
				break;
			case 0x93://decrement
				setupInstructionOneArgument(operandlist,mi,ABC_OP_OPTIMZED_DECREMENT,opcode,code,oldnewpositions, jumptargets,false,true,localtypes, defaultlocaltypes, Class<Number>::getRef(function->getSystemState()).getPtr(),code.tellg(),dup_indicator == 0);
				dup_indicator=0;
				removetypestack(typestack,1);
				typestack.push_back(typestackentry(Class<Number>::getRef(mi->context->root->getSystemState()).getPtr(),false));
				break;
			case 0x95: //typeof
				setupInstructionOneArgument(operandlist,mi,ABC_OP_OPTIMZED_TYPEOF,opcode,code,oldnewpositions, jumptargets,true,true,localtypes, defaultlocaltypes, Class<ASString>::getRef(function->getSystemState()).getPtr(),code.tellg(),true);
				removetypestack(typestack,1);
				typestack.push_back(typestackentry(Class<ASString>::getRef(mi->context->root->getSystemState()).getPtr(),false));
				break;
			case 0x96: //not
				setupInstructionOneArgument(operandlist,mi,ABC_OP_OPTIMZED_NOT,opcode,code,oldnewpositions, jumptargets,true,true,localtypes, defaultlocaltypes, Class<Boolean>::getRef(function->getSystemState()).getPtr(),code.tellg(),true);
				removetypestack(typestack,1);
				typestack.push_back(typestackentry(Class<Boolean>::getRef(mi->context->root->getSystemState()).getPtr(),false));
				break;
			case 0xa0://add
				removetypestack(typestack,2);
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
							typestack.push_back(typestackentry(Class<Integer>::getRef(mi->context->root->getSystemState()).getPtr(),false));
							break;
						}
					}
				}
				setupInstructionTwoArguments(operandlist,mi,ABC_OP_OPTIMZED_ADD,opcode,code,oldnewpositions, jumptargets,false,false,true,localtypes, defaultlocaltypes,code.tellg());
				typestack.push_back(typestackentry(nullptr,false));
				break;
			case 0xa1://subtract
				setupInstructionTwoArguments(operandlist,mi,ABC_OP_OPTIMZED_SUBTRACT,opcode,code,oldnewpositions, jumptargets,true,true,true,localtypes, defaultlocaltypes,code.tellg());
				removetypestack(typestack,2);
				typestack.push_back(typestackentry(nullptr,false));
				break;
			case 0xa2://multiply
				setupInstructionTwoArguments(operandlist,mi,ABC_OP_OPTIMZED_MULTIPLY,opcode,code,oldnewpositions, jumptargets,true,true,true,localtypes, defaultlocaltypes,code.tellg());
				removetypestack(typestack,2);
				typestack.push_back(typestackentry(nullptr,false));
				break;
			case 0xa3://divide
				setupInstructionTwoArguments(operandlist,mi,ABC_OP_OPTIMZED_DIVIDE,opcode,code,oldnewpositions, jumptargets,true,true,true,localtypes, defaultlocaltypes,code.tellg());
				removetypestack(typestack,2);
				typestack.push_back(typestackentry(nullptr,false));
				break;
			case 0xa4://modulo
				setupInstructionTwoArguments(operandlist,mi,ABC_OP_OPTIMZED_MODULO,opcode,code,oldnewpositions, jumptargets,true,true,true,localtypes, defaultlocaltypes,code.tellg());
				removetypestack(typestack,2);
				typestack.push_back(typestackentry(nullptr,false));
				break;
			case 0xa5://lshift
				setupInstructionTwoArguments(operandlist,mi,ABC_OP_OPTIMZED_LSHIFT,opcode,code,oldnewpositions, jumptargets,true,true,true,localtypes, defaultlocaltypes,code.tellg());
				removetypestack(typestack,2);
				typestack.push_back(typestackentry(Class<Integer>::getRef(function->getSystemState()).getPtr(),false));
				break;
			case 0xa6://rshift
				setupInstructionTwoArguments(operandlist,mi,ABC_OP_OPTIMZED_RSHIFT,opcode,code,oldnewpositions, jumptargets,true,true,true,localtypes, defaultlocaltypes,code.tellg());
				removetypestack(typestack,2);
				typestack.push_back(typestackentry(Class<Integer>::getRef(function->getSystemState()).getPtr(),false));
				break;
			case 0xa7://urshift
				setupInstructionTwoArguments(operandlist,mi,ABC_OP_OPTIMZED_URSHIFT,opcode,code,oldnewpositions, jumptargets,true,true,true,localtypes, defaultlocaltypes,code.tellg());
				removetypestack(typestack,2);
				typestack.push_back(typestackentry(nullptr,false));
				break;
			case 0xa8://bitand
				setupInstructionTwoArguments(operandlist,mi,ABC_OP_OPTIMZED_BITAND,opcode,code,oldnewpositions, jumptargets,true,true,true,localtypes, defaultlocaltypes,code.tellg());
				removetypestack(typestack,2);
				typestack.push_back(typestackentry(Class<Integer>::getRef(function->getSystemState()).getPtr(),false));
				break;
			case 0xa9://bitor
				setupInstructionTwoArguments(operandlist,mi,ABC_OP_OPTIMZED_BITOR,opcode,code,oldnewpositions, jumptargets,true,true,true,localtypes, defaultlocaltypes,code.tellg());
				removetypestack(typestack,2);
				typestack.push_back(typestackentry(Class<Integer>::getRef(function->getSystemState()).getPtr(),false));
				break;
			case 0xaa://bitxor
				setupInstructionTwoArguments(operandlist,mi,ABC_OP_OPTIMZED_BITXOR,opcode,code,oldnewpositions, jumptargets,true,true,true,localtypes, defaultlocaltypes,code.tellg());
				removetypestack(typestack,2);
				typestack.push_back(typestackentry(Class<Integer>::getRef(function->getSystemState()).getPtr(),false));
				break;
			case 0xab://equals
				setupInstructionTwoArguments(operandlist,mi,ABC_OP_OPTIMZED_EQUALS,opcode,code,oldnewpositions, jumptargets,false,false,true,localtypes, defaultlocaltypes,code.tellg());
				break;
			case 0xad://lessthan
				setupInstructionTwoArguments(operandlist,mi,ABC_OP_OPTIMZED_LESSTHAN,opcode,code,oldnewpositions, jumptargets,false,false,true,localtypes, defaultlocaltypes,code.tellg());
				removetypestack(typestack,2);
				typestack.push_back(typestackentry(Class<Boolean>::getRef(mi->context->root->getSystemState()).getPtr(),false));
				break;
			case 0xae://lessequals
				setupInstructionTwoArguments(operandlist,mi,ABC_OP_OPTIMZED_LESSEQUALS,opcode,code,oldnewpositions, jumptargets,false,false,true,localtypes, defaultlocaltypes,code.tellg());
				removetypestack(typestack,2);
				typestack.push_back(typestackentry(Class<Boolean>::getRef(mi->context->root->getSystemState()).getPtr(),false));
				break;
			case 0xaf://greaterthan
				setupInstructionTwoArguments(operandlist,mi,ABC_OP_OPTIMZED_GREATERTHAN,opcode,code,oldnewpositions, jumptargets,false,false,true,localtypes, defaultlocaltypes,code.tellg());
				removetypestack(typestack,2);
				typestack.push_back(typestackentry(Class<Boolean>::getRef(mi->context->root->getSystemState()).getPtr(),false));
				break;
			case 0xb0://greaterequals
				setupInstructionTwoArguments(operandlist,mi,ABC_OP_OPTIMZED_GREATEREQUALS,opcode,code,oldnewpositions, jumptargets,false,false,true,localtypes, defaultlocaltypes,code.tellg());
				removetypestack(typestack,2);
				typestack.push_back(typestackentry(Class<Boolean>::getRef(mi->context->root->getSystemState()).getPtr(),false));
				break;
			case 0xc0://increment_i
			case 0xc1://decrement_i
			{
				removetypestack(typestack,1);
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
							clearOperands(mi,nullptr,operandlist, defaultlocaltypes,&lastlocalresulttype);
						break;
					}
				}
				setupInstructionOneArgument(operandlist,mi,opcode == 0xc0 ? ABC_OP_OPTIMZED_INCREMENT_I : ABC_OP_OPTIMZED_DECREMENT_I,opcode,code,oldnewpositions, jumptargets,false,true,localtypes, defaultlocaltypes, Class<Integer>::getRef(function->getSystemState()).getPtr(),p,dup_indicator == 0);
				dup_indicator=0;
				typestack.push_back(typestackentry(Class<Integer>::getRef(mi->context->root->getSystemState()).getPtr(),false));
				break;
			}
			case 0xc5://add_i
				setupInstructionTwoArguments(operandlist,mi,ABC_OP_OPTIMZED_ADD_I,opcode,code,oldnewpositions, jumptargets,false,false,true,localtypes, defaultlocaltypes,code.tellg());
				removetypestack(typestack,2);
				typestack.push_back(typestackentry(Class<Integer>::getRef(mi->context->root->getSystemState()).getPtr(),false));
				break;
			case 0x03://throw
			case 0x29://pop
				mi->body->preloadedcode.push_back((uint32_t)opcode);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				removetypestack(typestack,1);
				break;
			case 0x2b://swap
				mi->body->preloadedcode.push_back((uint32_t)opcode);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				removetypestack(typestack,2);
				typestack.push_back(typestackentry(nullptr,false));
				typestack.push_back(typestackentry(nullptr,false));
				break;
			case 0x57://newactivation
				mi->body->preloadedcode.push_back((uint32_t)opcode);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				typestack.push_back(typestackentry(nullptr,false));
				break;
			case 0x87://astypelate
			case 0xc6://subtract_i
				mi->body->preloadedcode.push_back((uint32_t)opcode);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				removetypestack(typestack,2);
				typestack.push_back(typestackentry(nullptr,false));
				break;
			case 0x97://bitnot
			case 0xc4://negate_i
				mi->body->preloadedcode.push_back((uint32_t)opcode);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				removetypestack(typestack,1);
				typestack.push_back(typestackentry(Class<Integer>::getRef(mi->context->root->getSystemState()).getPtr(),false));
				break;
			case 0xb3://istypelate
				mi->body->preloadedcode.push_back((uint32_t)opcode);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				removetypestack(typestack,2);
				typestack.push_back(typestackentry(Class<Boolean>::getRef(mi->context->root->getSystemState()).getPtr(),false));
				break;
			case 0xac://strictequals
			case 0xb1://instanceof
			case 0xb4://in
				mi->body->preloadedcode.push_back((uint32_t)opcode);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				removetypestack(typestack,2);
				typestack.push_back(typestackentry(Class<Boolean>::getRef(mi->context->root->getSystemState()).getPtr(),false));
				break;
			case 0x1e://nextname
			case 0x23://nextvalue
				mi->body->preloadedcode.push_back((uint32_t)opcode);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				removetypestack(typestack,2);
				typestack.push_back(typestackentry(nullptr,false));
				break;
			case 0x77://convert_o
			case 0x78://checkfilter
				mi->body->preloadedcode.push_back((uint32_t)opcode);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				removetypestack(typestack,1);
				typestack.push_back(typestackentry(nullptr,false));
				break;
			case 0x70://convert_s
			case 0x85://coerce_s
				mi->body->preloadedcode.push_back((uint32_t)opcode);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				removetypestack(typestack,1);
				typestack.push_back(typestackentry(Class<ASString>::getRef(mi->context->root->getSystemState()).getPtr(),false));
				break;
			case 0x90://negate
				mi->body->preloadedcode.push_back((uint32_t)opcode);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				removetypestack(typestack,1);
				typestack.push_back(typestackentry(Class<Number>::getRef(mi->context->root->getSystemState()).getPtr(),false));
				break;
			case 0x71://esc_xelem
			case 0x72://esc_xattr
			case 0xc7://multiply_i
				mi->body->preloadedcode.push_back((uint32_t)opcode);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				removetypestack(typestack,1);
				typestack.push_back(typestackentry(nullptr,false));
				break;
			case 0xd4://setlocal_0
			case 0xd5://setlocal_1
			case 0xd6://setlocal_2
			case 0xd7://setlocal_3
			{
				int32_t p = code.tellg();
				assert_and_throw(uint32_t(opcode-0xd4) < mi->body->local_count);
				setupInstructionOneArgumentNoResult(operandlist,mi,ABC_OP_OPTIMZED_SETLOCAL,opcode,code,oldnewpositions, jumptargets,p);
				mi->body->preloadedcode.at(mi->body->preloadedcode.size()-1).data|=(opcode-0xd4)<<OPCODE_SIZE;
				if (typestack.back().obj && typestack.back().obj->is<Class_base>())
					localtypes[opcode-0xd4]=typestack.back().obj->as<Class_base>();
				else
					localtypes[opcode-0xd4]=nullptr;
				removetypestack(typestack,1);
				break;
			}
			case 0x50://sxi1
			case 0x51://sxi8
			case 0x52://sxi16
			case 0xf3://timestamp
				mi->body->preloadedcode.push_back((uint32_t)opcode);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				break;
			case 0x07://dxnslate
				mi->body->preloadedcode.push_back((uint32_t)opcode);
				oldnewpositions[code.tellg()] = (int32_t)mi->body->preloadedcode.size();
				clearOperands(mi,localtypes,operandlist, defaultlocaltypes,&lastlocalresulttype);
				removetypestack(typestack,1);
				break;
			default:
			{
				char stropcode[10];
				sprintf(stropcode,"%d",opcode);
				char strcodepos[20];
				sprintf(strcodepos,"%d",code.tellg());
				throwError<VerifyError>(kIllegalOpcodeError,function->getSystemState()->getStringFromUniqueId(function->functionname),stropcode,strcodepos);
				break;
			}
		}
	}
	mi->needscoerceresult = coercereturnvalue;
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

