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

// undef this to run abc code without any optimization
#define ENABLE_OPTIMIZATION

uint64_t ABCVm::profilingCheckpoint(uint64_t& startTime)
{
	uint64_t cur=compat_get_thread_cputime_us();
	uint64_t ret=cur-startTime;
	startTime=cur;
	return ret;
}
bool ABCVm::checkPropertyException(ASObject* obj,multiname* name, asAtom& prop)
{
	if(asAtomHandler::isValid(prop))
	{
		name->resetNameIfObject();
		return false;
	}
	if (name->name_type != multiname::NAME_OBJECT // avoid calling toString() of multiname object
			&& obj->getClass() && obj->getClass()->findBorrowedSettable(*name))
	{
		createError<ReferenceError>(obj->getInstanceWorker(),kWriteOnlyError, name->normalizedNameUnresolved(obj->getSystemState()), obj->getClassName());
		name->resetNameIfObject();
		return true;
	}
	if (obj->getClass() && obj->getClass()->isSealed)
	{
		createError<ReferenceError>(obj->getInstanceWorker(),kReadSealedError, name->normalizedNameUnresolved(obj->getSystemState()), obj->getClass()->getQualifiedClassName());
		name->resetNameIfObject();
		return true;
	}
	if (name->isEmpty() || (!name->hasEmptyNS && !name->ns.empty()))
	{
		createError<ReferenceError>(obj->getInstanceWorker(),kReadSealedErrorNs, name->normalizedNameUnresolved(obj->getSystemState()), obj->getClassName());
		name->resetNameIfObject();
		return true;
	}
	if (obj->is<Undefined>())
	{
		createError<TypeError>(obj->getInstanceWorker(),kConvertUndefinedToObjectError);
		name->resetNameIfObject();
		return true;
	}
	name->resetNameIfObject();
	prop = asAtomHandler::undefinedAtom;
	return false;
}
bool ABCVm::checkPropertyExceptionInteger(ASObject* obj,int index, asAtom& prop)
{
	multiname m(nullptr);
	m.name_type = multiname::NAME_INT;
	m.name_i = index;
	return checkPropertyException(obj,&m, prop);
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
	abc_invalidinstruction,
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

	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction,
	abc_invalidinstruction
};

struct operands;
struct preloadedcodebuffer
{
	preloadedcodedata pcode;
	uint32_t opcode;
	uint32_t operator_start;
	uint32_t operator_setslot;
	bool cachedslot1;
	bool cachedslot2;
	bool cachedslot3;
	preloadedcodebuffer(uint32_t d=0):pcode(),opcode(d),operator_start(d),operator_setslot(UINT32_MAX),cachedslot1(false),cachedslot2(false),cachedslot3(false){}
};
struct preloadstate
{
	std::vector<operands> operandlist;
	std::map<int32_t,int32_t> oldnewpositions;
	std::map<int32_t,int32_t> jumptargets;
	// result type from the top of the typestack at start of jump
	std::map<int32_t,Class_base*> jumptargeteresulttypes;
	// local variables (arguments and "this" object) that do not change during execution
	std::set<int32_t> unchangedlocals;
	std::set<int32_t> setlocal_handled;
	std::vector<Class_base*> localtypes;
	std::vector<Class_base*> defaultlocaltypes;
	std::vector<bool> defaultlocaltypescacheable;
	std::vector<bool> canlocalinitialize;
	SyntheticFunction* function;
	ASWorker* worker;
	method_info* mi;
	std::vector<preloadedcodebuffer> preloadedcode;
	// used to keep first operand of dup opcode (defined as vector because of forward declaration)
	std::vector<operands> dupoperands;
	bool duplocalresult;
	preloadstate(SyntheticFunction* _f, ASWorker* _w):function(_f),worker(_w),mi(_f->getMethodInfo()),duplocalresult(false) {}
};

struct operands
{
	OPERANDTYPES type;
	bool modified;
	bool duparg1;
	Class_base* objtype;
	int32_t index;
	uint32_t codecount;
	uint32_t preloadedcodepos;
	ASObject* instance;
	operands():type(OP_UNDEFINED),modified(false),duparg1(false),objtype(nullptr),index(-1),codecount(0),preloadedcodepos(0),instance(nullptr) {}
	operands(OPERANDTYPES _t, Class_base* _o,int32_t _i,uint32_t _c, uint32_t _p):type(_t),modified(false),duparg1(false),objtype(_o),index(_i),codecount(_c),preloadedcodepos(_p),instance(nullptr) {}
	void removeArg(preloadstate& state)
	{
		if (codecount)
			state.preloadedcode.erase(state.preloadedcode.begin()+preloadedcodepos,state.preloadedcode.begin()+preloadedcodepos+codecount);
	}
	bool fillCode(preloadstate& state,int pos, int codepos, bool switchopcode,int* opcode=nullptr, uint32_t* opcode_setslot=nullptr)
	{
		switch (type)
		{
			case OP_LOCAL:
				switch (pos)
				{
					case 0:
						state.preloadedcode[codepos].pcode.local_pos1 = index;
						break;
					case 1:
						state.preloadedcode[codepos].pcode.local_pos2 = index;
						break;
				}
				if (switchopcode)
				{
					if (opcode)
						*opcode+=1+pos; // increase opcode
					else
						state.preloadedcode[codepos].opcode+=1+pos; // increase opcode
					if (opcode_setslot && *opcode_setslot != UINT32_MAX)
						*opcode_setslot+=1+pos; // increase opcode;
				}
				else
					return true;
				break;
			case OP_CACHED_SLOT:
				switch (pos)
				{
					case 0:
						state.preloadedcode[codepos].pcode.local_pos1 = index;
						state.preloadedcode[codepos].cachedslot1=true;
						break;
					case 1:
						state.preloadedcode[codepos].pcode.local_pos2 = index;
						state.preloadedcode[codepos].cachedslot2=true;
						break;
				}
				if (switchopcode)
				{
					if (opcode)
						*opcode+=1+pos; // increase opcode
					else
						state.preloadedcode[codepos].opcode+=1+pos; // increase opcode
					if (opcode_setslot && *opcode_setslot != UINT32_MAX)
						*opcode_setslot+=1+pos; // increase opcode;
				}
				else
					return true;
				break;
			default:
				switch (pos)
				{
					case 0:
						state.preloadedcode[codepos].pcode.arg1_constant = state.mi->context->getConstantAtom(type,index);
						break;
					case 1:
						state.preloadedcode[codepos].pcode.arg2_constant = state.mi->context->getConstantAtom(type,index);
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
#define ABC_OP_OPTIMZED_GETSLOT_SETSLOT 0x000001fc
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
#define ABC_OP_OPTIMZED_LI8_SETSLOT 0x00000256
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
#define ABC_OP_OPTIMZED_PUSHCACHEDSLOT 0x00000287
#define ABC_OP_OPTIMZED_GETPROPERTY_INTEGER 0x00000288
#define ABC_OP_OPTIMZED_SETPROPERTY_INTEGER 0x00000290
#define ABC_OP_OPTIMZED_IFNLT 0x00000298
#define ABC_OP_OPTIMZED_IFNGE 0x0000029c
#define ABC_OP_OPTIMZED_SETLOCAL 0x000002a0
#define ABC_OP_OPTIMZED_CALLFUNCTIONSYNTHETIC_STATICNAME_MULTIARGS 0x000002a2
#define ABC_OP_OPTIMZED_CALLFUNCTIONVOIDSYNTHETIC_STATICNAME_MULTIARGS 0x000002a3
#define ABC_OP_OPTIMZED_INCLOCAL_I 0x000002a4
#define ABC_OP_OPTIMZED_DECLOCAL_I 0x000002a5
#define ABC_OP_OPTIMZED_DUP 0x000002a6
#define ABC_OP_OPTIMZED_DUP_INCDEC 0x000002a8
#define ABC_OP_OPTIMZED_ADD_I_SETSLOT 0x000002ac
#define ABC_OP_OPTIMZED_SETPROPERTY_INTEGER_VECTOR 0x000002b0
#define ABC_OP_OPTIMZED_ISTYPELATE 0x000002b8
#define ABC_OP_OPTIMZED_ASTYPELATE 0x000002c0
#define ABC_OP_OPTIMZED_LI16_SETSLOT 0x000002c8
#define ABC_OP_OPTIMZED_LI32_SETSLOT 0x000002ca
#define ABC_OP_OPTIMZED_LSHIFT_SETSLOT 0x000002cc
#define ABC_OP_OPTIMZED_RSHIFT_SETSLOT 0x000002d0
#define ABC_OP_OPTIMZED_ADD_SETSLOT 0x000002d4
#define ABC_OP_OPTIMZED_SUBTRACT_SETSLOT 0x000002d8
#define ABC_OP_OPTIMZED_MULTIPLY_SETSLOT 0x000002dc
#define ABC_OP_OPTIMZED_DIVIDE_SETSLOT 0x000002e0
#define ABC_OP_OPTIMZED_MODULO_SETSLOT 0x000002e4
#define ABC_OP_OPTIMZED_URSHIFT_SETSLOT 0x000002e8
#define ABC_OP_OPTIMZED_BITAND_SETSLOT 0x000002ec
#define ABC_OP_OPTIMZED_BITOR_SETSLOT 0x000002f0
#define ABC_OP_OPTIMZED_BITXOR_SETSLOT 0x000002f4
#define ABC_OP_OPTIMZED_CONVERTI_SETSLOT 0x000002f8
#define ABC_OP_OPTIMZED_CONVERTU_SETSLOT 0x000002fa
#define ABC_OP_OPTIMZED_CONVERTD_SETSLOT 0x000002fc
#define ABC_OP_OPTIMZED_CONVERTB_SETSLOT 0x000002fe

#define ABC_OP_OPTIMZED_LF32_SETSLOT 0x00000300
#define ABC_OP_OPTIMZED_LF64_SETSLOT 0x00000302
#define ABC_OP_OPTIMZED_CALLFUNCTION_NOARGS_SETSLOT 0x00000304
#define ABC_OP_OPTIMZED_INCREMENT_I_SETSLOT 0x00000306
#define ABC_OP_OPTIMZED_DECREMENT_I_SETSLOT 0x00000307
#define ABC_OP_OPTIMZED_ASTYPELATE_SETSLOT 0x00000308
#define ABC_OP_OPTIMZED_DUP_SETSLOT 0x0000031c

#define ABC_OP_OPTIMZED_GETFUNCSCOPEOBJECT 0x0000031e
#define ABC_OP_OPTIMZED_CALLFUNCTIONSYNTHETIC_ONEARG 0x00000320
#define ABC_OP_OPTIMZED_CALLFUNCTIONBUILTIN_ONEARG 0x00000328
#define ABC_OP_OPTIMZED_INSTANCEOF 0x00000330
#define ABC_OP_OPTIMZED_SUBTRACT_I 0x00000338
#define ABC_OP_OPTIMZED_MULTIPLY_I 0x00000340
#define ABC_OP_OPTIMZED_SUBTRACT_I_SETSLOT 0x00000348
#define ABC_OP_OPTIMZED_MULTIPLY_I_SETSLOT 0x0000034c
#define ABC_OP_OPTIMZED_INCLOCAL_I_POSTFIX 0x00000350
#define ABC_OP_OPTIMZED_DECLOCAL_I_POSTFIX 0x00000351
#define ABC_OP_OPTIMZED_LOOKUPSWITCH 0x00000352
#define ABC_OP_OPTIMZED_IFNLE 0x00000354
#define ABC_OP_OPTIMZED_IFNGT 0x00000358
#define ABC_OP_OPTIMZED_CALL_VOID 0x0000035c
#define ABC_OP_OPTIMZED_CALL 0x00000360
#define ABC_OP_OPTIMZED_COERCE 0x00000368
#define ABC_OP_OPTIMZED_SXI1 0x0000036c
#define ABC_OP_OPTIMZED_SXI8 0x00000370
#define ABC_OP_OPTIMZED_SXI16 0x00000374

void skipjump(preloadstate& state,uint8_t& b,memorystream& code,uint32_t& pos,bool jumpInCode)
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
				if (state.jumptargets.find(p+i+1) != state.jumptargets.end())
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
			auto it = state.jumptargets.find(p+j+1);
			if (it != state.jumptargets.end() && it->second > 1)
				state.jumptargets[p+j+1]--;
			else
				state.jumptargets.erase(p+j+1);
			state.oldnewpositions[p+j] = (int32_t)state.preloadedcode.size();
			b = code.peekbyteFromPosition(pos);
			if (jumpInCode)
			{
				state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
				code.seekg(pos);
				state.oldnewpositions[code.tellg()+1] = (int32_t)state.preloadedcode.size();
			}
			pos++;
		}
	}
}
void clearOperands(preloadstate& state,bool resetlocaltypes,Class_base** lastlocalresulttype,bool checkchanged=false, OPERANDTYPES type = OP_CACHED_CONSTANT, int index=-1)
{
	if (resetlocaltypes)
	{
		for (uint32_t i = 0; i < state.mi->body->getReturnValuePos()+state.mi->body->localresultcount; i++)
		{
			assert(i < state.defaultlocaltypes.size() && "array out of bounds!"); 
			state.localtypes[i] = state.defaultlocaltypes[i];
		}
	}
	if (lastlocalresulttype)
		*lastlocalresulttype=nullptr;
	bool clear = !checkchanged;
	for (auto it = state.operandlist.begin(); !clear && it != state.operandlist.end();it++)
	{
		if (it->modified && it->type == type && it->index == index)
		{
			clear=true;
			break;
		}
	}
	if (clear)
		state.operandlist.clear();
	state.duplocalresult=false;
}
void removeOperands(preloadstate& state,bool resetlocaltypes,Class_base** lastlocalresulttype,uint32_t opcount)
{
	if (resetlocaltypes)
	{
		for (uint32_t i = 0; i < state.mi->body->getReturnValuePos()+state.mi->body->localresultcount; i++)
		{
			assert(i < state.defaultlocaltypes.size() && "array out of bounds!"); 
			state.localtypes[i] = state.defaultlocaltypes[i];
		}
	}
	if (lastlocalresulttype)
		*lastlocalresulttype=nullptr;
	for (uint32_t i = 0; i < opcount; i++)
		state.operandlist.pop_back();
	state.duplocalresult=false;
}
void setOperandModified(preloadstate& state,OPERANDTYPES type, int index)
{
	for (auto it = state.operandlist.begin(); it != state.operandlist.end();it++)
	{
		if (it->type == type && it->index == index)
		{
			it->modified=true;
		}
	}
}

bool canCallFunctionDirect(operands& op,multiname* name, bool ignoreoverridden=false)
{
	if (op.objtype && op.objtype->is<Class_inherit>() && !op.objtype->isConstructed())
	{
		if (op.objtype->getInstanceWorker()->rootClip && !op.objtype->getInstanceWorker()->rootClip->hasFinishedLoading())
			return false;
		if (!op.objtype->as<Class_inherit>()->checkScriptInit())
			return false;
	}
	return ((op.type == OP_LOCAL || op.type == OP_CACHED_CONSTANT || op.type == OP_CACHED_SLOT) &&
		op.objtype &&
		!op.objtype->isInterface && // it's not an interface
//		op.objtype->isSealed && // it's sealed
		(
		ignoreoverridden ||
		!op.objtype->as<Class_base>()->hasoverriddenmethod(name) // current method is not in overridden methods
		));
}
bool canCallFunctionDirect(ASObject* obj,multiname* name, bool ignoreoverridden)
{
	if (!obj || !obj->is<Class_base>())
		return false;
	Class_base* objtype = obj->as<Class_base>();
	if (objtype->is<Class_inherit>() && !objtype->isConstructed())
	{
		if (objtype->getInstanceWorker()->rootClip && !objtype->getInstanceWorker()->rootClip->hasFinishedLoading())
			return false;
		if (!objtype->as<Class_inherit>()->checkScriptInit())
			return false;
	}
	return !objtype->isInterface && // it's not an interface
		objtype->isSealed && // it's sealed
		(
		!objtype->is<Class_inherit>() || // type is builtin class
		ignoreoverridden || 
		!objtype->as<Class_inherit>()->hasoverriddenmethod(name) // current method is not in overridden methods
		);
}
void setForceInt(preloadstate& state,memorystream& code,Class_base** resulttype)
{
#ifdef ENABLE_OPTIMIZATION
	switch (code.peekbyte())
	{
		case 0x73://convert_i
			state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
			code.readbyte();
			// falls through
		case 0x35://li8
		case 0x36://li16
		case 0x37://li32
		case 0x38://lf32
		case 0x39://lf64
		case 0x3a://si8
		case 0x3b://si16
		case 0x3c://si32
			state.preloadedcode.back().pcode.local3.flags |= ABC_OP_FORCEINT;
			break;
		default:
			break;
	}
	if (state.preloadedcode.back().pcode.local3.flags & ABC_OP_FORCEINT)
		*resulttype = Class<Integer>::getRef(state.mi->context->root->getSystemState()).getPtr();
#endif
}
bool checkForLocalResult(preloadstate& state,memorystream& code,uint32_t opcode_jumpspace, Class_base* restype,int preloadpos=-1,int preloadlocalpos=-1, bool checkchanged=false,bool fromdup = false, uint32_t opcode_setslot=UINT32_MAX)
{
#ifdef ENABLE_OPTIMIZATION
	bool res = false;
	uint32_t resultpos=0;
	uint32_t pos = code.tellg()+1;
	uint8_t b = code.peekbyte();
	bool keepchecking=true;
	if (preloadpos == -1)
		preloadpos = state.preloadedcode.size()-1;
	if (preloadlocalpos == -1)
		preloadlocalpos = state.preloadedcode.size()-1;
	uint32_t argsneeded=0;
	int localresultused=0;
	uint32_t dupskipped=UINT32_MAX;
	for (auto it = state.operandlist.begin(); it != state.operandlist.end(); it++)
	{
		if (it->type != OP_LOCAL && it->type != OP_CACHED_SLOT)
			continue;
		if (uint32_t(it->index) > state.mi->body->getReturnValuePos()) // local result index already used
		{
			resultpos++; // use free entry for resultpos
			localresultused++;
		}
	}
	set<int> localresultaddedindex;
	int currindex = 0;
	int lastlocalpos=-1;
	bool candup=false;
	while (state.jumptargets.find(pos) == state.jumptargets.end() && keepchecking)
	{
		skipjump(state,b,code,pos,false);
		// check if the next opcode can be skipped
		//LOG(LOG_CALLS,"checkforlocal skip:"<<argsneeded<<" "<<state.operandlist.size()<<" "<<candup<<" "<<hex<<(uint32_t)b);
		switch (b)
		{
			case 0x24://pushbyte
				candup=true;
				pos++;
				b = code.peekbyteFromPosition(pos);
				pos++;
				if ((state.jumptargets.find(pos) == state.jumptargets.end()) && b==0x29) // pop
				{
					b = code.peekbyteFromPosition(pos);
					pos++;
					break;
				}
				argsneeded++;
				if (b==0x73 ||//convert_i
						b==0x74)//convert_u
				{
					b = code.peekbyteFromPosition(pos);
					pos++;
				}
				lastlocalpos=-1;
				break;
			case 0x25://pushshort
			case 0x2d://pushint
				candup=true;
				pos = code.skipu30FromPosition(pos);
				b = code.peekbyteFromPosition(pos);
				pos++;
				if ((state.jumptargets.find(pos) == state.jumptargets.end()) && b==0x29) // pop
				{
					b = code.peekbyteFromPosition(pos);
					pos++;
					break;
				}
				argsneeded++;
				if (b==0x73)//convert_i
				{
					b = code.peekbyteFromPosition(pos);
					pos++;
				}
				lastlocalpos=-1;
				break;
			case 0x26://pushtrue
				candup=true;
				b = code.peekbyteFromPosition(pos);
				pos++;
				if ((state.jumptargets.find(pos) == state.jumptargets.end()) && b==0x29) // pop
				{
					b = code.peekbyteFromPosition(pos);
					pos++;
					break;
				}
				argsneeded++;
				if (b==0x76)//convert_b
				{
					b = code.peekbyteFromPosition(pos);
					pos++;
				}
				keepchecking=false;
				lastlocalpos=-1;
				break;
			case 0x27://pushfalse
				candup=true;
				b = code.peekbyteFromPosition(pos);
				pos++;
				if ((state.jumptargets.find(pos) == state.jumptargets.end()) && b==0x29) // pop
				{
					b = code.peekbyteFromPosition(pos);
					pos++;
					break;
				}
				argsneeded++;
				if (b==0x76)//convert_b
				{
					b = code.peekbyteFromPosition(pos);
					pos++;
				}
				lastlocalpos=-1;
				break;
			case 0x2e://pushuint
				candup=true;
				pos = code.skipu30FromPosition(pos);
				b = code.peekbyteFromPosition(pos);
				pos++;
				if ((state.jumptargets.find(pos) == state.jumptargets.end()) && b==0x29) // pop
				{
					b = code.peekbyteFromPosition(pos);
					pos++;
					break;
				}
				argsneeded++;
				if (b==0x74)//convert_u
				{
					b = code.peekbyteFromPosition(pos);
					pos++;
				}
				lastlocalpos=-1;
				break;
			case 0x2f://pushdouble
				candup=true;
				pos = code.skipu30FromPosition(pos);
				b = code.peekbyteFromPosition(pos);
				pos++;
				if ((state.jumptargets.find(pos) == state.jumptargets.end()) && b==0x29) // pop
				{
					b = code.peekbyteFromPosition(pos);
					pos++;
					break;
				}
				argsneeded++;
				if (b==0x75)//convert_d
				{
					b = code.peekbyteFromPosition(pos);
					pos++;
				}
				lastlocalpos=-1;
				break;
			case 0x2c://pushstring
			case 0x31://pushnamespace
				candup=true;
				pos = code.skipu30FromPosition(pos);
				b = code.peekbyteFromPosition(pos);
				pos++;
				if ((state.jumptargets.find(pos) == state.jumptargets.end()) && b==0x29) // pop
				{
					b = code.peekbyteFromPosition(pos);
					pos++;
					break;
				}
				argsneeded++;
				lastlocalpos=-1;
				break;
			case 0x65://getscopeobject
				candup=true;
				pos = code.skipu30FromPosition(pos);
				b = code.peekbyteFromPosition(pos);
				pos++;
				argsneeded++;
				lastlocalpos=-1;
				break;
			case 0x5d://findpropstrict
			case 0x5e://findproperty
				candup=false;
				if (state.function->inClass && state.function->inClass->isSealed && !state.function->isFromNewFunction() && !state.mi->needsActivation())
				{
					uint32_t t = code.peeku30FromPosition(pos);
					if (state.mi->context->constant_pool.multinames[t].runtimeargs == 0)
					{
						multiname* name =  state.mi->context->getMultinameImpl(asAtomHandler::nullAtom,nullptr,t,false);
						if (name->isStatic && !state.function->inClass->hasoverriddenmethod(name))
						{
							pos = code.skipu30FromPosition(pos);
							b = code.peekbyteFromPosition(pos);
							pos++;
							argsneeded++;
							lastlocalpos=-1;
							break;
						}
					}
				}
				keepchecking=false;
				break;
			case 0x6c://getslot
				candup=false;
				if (argsneeded || state.operandlist.size()>0)
				{
					pos = code.skipu30FromPosition(pos);
					b = code.peekbyteFromPosition(pos);
					pos++;
					lastlocalpos=-1;
				}
				else
					keepchecking=false;
				break;
			case 0x62://getlocal
			{
				candup=false;
				uint32_t t = code.peeku30FromPosition(pos);
				pos = code.skipu30FromPosition(pos);
				b = code.peekbyteFromPosition(pos);
				if ((state.jumptargets.find(pos) == state.jumptargets.end()) && b==0x29) // pop
				{
					b = code.peekbyteFromPosition(pos);
					pos++;
					break;
				}
				pos++;
				argsneeded++;
				lastlocalpos=t;
				break;
			}
			case 0x60://getlex
				candup=false;
				pos = code.skipu30FromPosition(pos);
				b = code.peekbyteFromPosition(pos);
				pos++;
				argsneeded++;
				localresultused++;
				localresultaddedindex.insert(currindex);
				lastlocalpos=-1;
				break;
			case 0x73://convert_i
				if (argsneeded
						|| restype == Class<Integer>::getRef(state.mi->context->root->getSystemState()).getPtr()
						|| code.peekbyteFromPosition(pos) == b
						)
				{
					b = code.peekbyteFromPosition(pos);
					pos++;
				}
				else
					keepchecking=false;
				break;
			case 0x74://convert_u
				if (argsneeded
						|| restype == Class<UInteger>::getRef(state.mi->context->root->getSystemState()).getPtr()
						|| code.peekbyteFromPosition(pos) == b
						)
				{
					b = code.peekbyteFromPosition(pos);
					pos++;
				}
				else
					keepchecking=false;
				break;
			case 0x75://convert_d
				if (argsneeded
						|| restype == Class<Number>::getRef(state.mi->context->root->getSystemState()).getPtr()
						|| restype == Class<Integer>::getRef(state.mi->context->root->getSystemState()).getPtr()
						|| restype == Class<UInteger>::getRef(state.mi->context->root->getSystemState()).getPtr()
						|| code.peekbyteFromPosition(pos) == b
						)
				{
					b = code.peekbyteFromPosition(pos);
					pos++;
				}
				else
					keepchecking=false;
				break;
			case 0x76://convert_b
				if (argsneeded || restype == Class<Boolean>::getRef(state.mi->context->root->getSystemState()).getPtr()
						|| code.peekbyteFromPosition(pos) == 0x11 //iftrue
						|| code.peekbyteFromPosition(pos) == 0x12 //iffalse
						|| code.peekbyteFromPosition(pos) == 0x96 //not
						|| code.peekbyteFromPosition(pos) == b
					)
				{
					b = code.peekbyteFromPosition(pos);
					pos++;
				}
				else
					keepchecking=false;
				break;
			case 0x20://pushnull
				candup=true;
				b = code.peekbyteFromPosition(pos);
				pos++;
				if ((state.jumptargets.find(pos) == state.jumptargets.end()) && b==0x29) // pop
				{
					b = code.peekbyteFromPosition(pos);
					pos++;
					break;
				}
				argsneeded++;
				if (b==0x80 //coerce
					&& (state.jumptargets.find(pos) == state.jumptargets.end()))
				{
					uint32_t t = code.peeku30FromPosition(pos);
					multiname* name =  state.mi->context->getMultinameImpl(asAtomHandler::nullAtom,nullptr,t,false);
					const Type* tp = Type::getTypeFromMultiname(name, state.mi->context);
					const Class_base* cls =dynamic_cast<const Class_base*>(tp);
					if (cls != Class<Number>::getRef(state.mi->context->root->getSystemState()).getPtr() &&
						cls != Class<Integer>::getRef(state.mi->context->root->getSystemState()).getPtr() &&
						cls != Class<UInteger>::getRef(state.mi->context->root->getSystemState()).getPtr() &&
						cls != Class<Boolean>::getRef(state.mi->context->root->getSystemState()).getPtr())
					{
						pos = code.skipu30FromPosition(pos);
						b = code.peekbyteFromPosition(pos);
						pos++;
					}
				}
				lastlocalpos=-1;
				break;
			case 0x21://pushundefined
			case 0x28://pushnan
				candup=true;
				b = code.peekbyteFromPosition(pos);
				pos++;
				if ((state.jumptargets.find(pos) == state.jumptargets.end()) && b==0x29) // pop
				{
					b = code.peekbyteFromPosition(pos);
					pos++;
					break;
				}
				lastlocalpos=-1;
				argsneeded++;
				break;
			case 0x2a://dup
				if (!candup)
				{
					keepchecking=false;
					break;
				}
				if (dupskipped==UINT32_MAX)
					dupskipped=argsneeded;
				b = code.peekbyteFromPosition(pos);
				pos++;
				if ((state.jumptargets.find(pos) == state.jumptargets.end()) && b==0x29) // pop
				{
					b = code.peekbyteFromPosition(pos);
					pos++;
					break;
				}
				lastlocalpos=-1;
				argsneeded++;
				break;
			case 0xd0://getlocal_0
			case 0xd1://getlocal_1
			case 0xd2://getlocal_2
			case 0xd3://getlocal_3
				candup=false;
				lastlocalpos=b-0xd0;
				b = code.peekbyteFromPosition(pos);
				pos++;
				if ((state.jumptargets.find(pos) == state.jumptargets.end()) && b==0x29) // pop
				{
					b = code.peekbyteFromPosition(pos);
					pos++;
					break;
				}
				argsneeded++;
				break;
			case 0x80://coerce
			{
				candup=false;
				uint32_t t = code.peeku30FromPosition(pos);
				multiname* name =  state.mi->context->getMultinameImpl(asAtomHandler::nullAtom,nullptr,t,false);
				if (code.peekbyteFromPosition(code.skipu30FromPosition(pos)) == 0x80//coerce
						&& state.jumptargets.find(pos) == state.jumptargets.end())
				{
					pos = code.skipu30FromPosition(pos);
					b = code.peekbyteFromPosition(pos);
					pos++;
					break;
				}
				if (argsneeded && state.jumptargets.find(pos) == state.jumptargets.end())
				{
					pos = code.skipu30FromPosition(pos);
					b = code.peekbyteFromPosition(pos);
					pos++;
					break;
				}
				if (name->isStatic && !argsneeded)
				{
					const Type* tp = Type::getTypeFromMultiname(name,state.mi->context);
					if (tp && (state.jumptargets.find(pos) == state.jumptargets.end()))
					{
						bool skip = false;
						if (restype)
						{
							if (restype == Class<Number>::getRef(state.mi->context->root->getSystemState()).getPtr() ||
									 restype == Class<Integer>::getRef(state.mi->context->root->getSystemState()).getPtr() ||
									 restype == Class<UInteger>::getRef(state.mi->context->root->getSystemState()).getPtr() ||
									 restype == Class<Boolean>::getRef(state.mi->context->root->getSystemState()).getPtr())
								skip = tp == Class<Number>::getRef(state.mi->context->root->getSystemState()).getPtr() || tp == Class<Integer>::getRef(state.mi->context->root->getSystemState()).getPtr() || tp == Class<UInteger>::getRef(state.mi->context->root->getSystemState()).getPtr();
							else if (restype != Class<ASString>::getRef(state.mi->context->root->getSystemState()).getPtr())
								skip = restype == tp;
						}
						if (skip)
						{
							restype = (Class_base*)tp;
							pos = code.skipu30FromPosition(pos);
							b = code.peekbyteFromPosition(pos);
							pos++;
							break;
						}
					}
				}
				keepchecking=false;
				break;
			}
			case 0x35://li8
			case 0x36://li16
			case 0x37://li32
			case 0x38://lf32
			case 0x39://lf64
			case 0x50://sxi1
			case 0x51://sxi8
			case 0x52://sxi16
				candup=false;
				if (argsneeded)
				{
					b = code.peekbyteFromPosition(pos);
					pos++;
				}
				else
					keepchecking=false;
				break;
			case 0x08://kill
				if (state.jumptargets.find(pos) == state.jumptargets.end())
				{
					b = code.peekbyteFromPosition(pos);
					pos++;
				}
				else
					keepchecking=false;
				break;
			case 0x82://coerce_a
				candup=true;
				b = code.peekbyteFromPosition(pos);
				pos++;
				break;
			case 0x66://getproperty
			{
				candup=false;
				uint32_t t = code.peeku30FromPosition(pos);
				if (argsneeded && !fromdup &&
					(uint32_t)state.mi->context->constant_pool.multinames[t].runtimeargs == 0)
				{
					// getproperty without runtimeargs following e.g. getlocal may produce local result
					pos = code.skipu30FromPosition(pos);
					b = code.peekbyteFromPosition(pos);
					pos++;
					// if the last getlocal was for a position that is unchanged, this getproperty will be converted into a cached slot, so no need to use a localresult
					if (!state.unchangedlocals.count(lastlocalpos))
					{
						// if one of the arguments is a localresult, it will be reused and we don't have to increase localresultused
						if (!localresultaddedindex.count(currindex-1) && !localresultaddedindex.count(currindex-2))
							localresultused++;
						localresultaddedindex.insert(currindex);
					}
				}
				else if (argsneeded > 1 && !fromdup &&
					(uint32_t)state.mi->context->constant_pool.multinames[t].runtimeargs == 1)
				{
					// getproperty with 1 runtime arg needs 2 args and produces 1 arg
					argsneeded--;
					pos = code.skipu30FromPosition(pos);
					b = code.peekbyteFromPosition(pos);
					pos++;
				}
				else
					keepchecking=false;
				lastlocalpos=-1;
				break;
			}
			case 0x4f://callpropvoid
			case 0x46://callproperty
			{
				candup=false;
				uint32_t t = code.peeku30FromPosition(pos);
				uint32_t pos2 = code.skipu30FromPosition(pos);
				uint32_t argcount = code.peeku30FromPosition(pos2);
				if (state.jumptargets.find(pos) == state.jumptargets.end()
						&&(argsneeded > argcount) && state.mi->context->constant_pool.multinames[t].runtimeargs == 0)
				{
					argsneeded -= argcount;
					if (b==0x4f) //callpropvoid
						argsneeded--;
					pos = code.skipu30FromPosition(pos2);
					b = code.peekbyteFromPosition(pos);
					pos++;
				}
				else
					keepchecking=false;
				lastlocalpos=-1;
				break;
			}
			case 0x4a://constructprop
			{
				candup=false;
				uint32_t t = code.peeku30FromPosition(pos);
				uint32_t pos2 = code.skipu30FromPosition(pos);
				uint32_t argcount = code.peeku30FromPosition(pos2);
				if (state.jumptargets.find(pos) == state.jumptargets.end()
						&& argsneeded && argcount==0 && state.mi->context->constant_pool.multinames[t].runtimeargs == 0)
				{
					pos = code.skipu30FromPosition(pos2);
					b = code.peekbyteFromPosition(pos);
					pos++;
				}
				else
					keepchecking=false;
				lastlocalpos=-1;
				break;
			}
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
			case 0xc5://add_i
			case 0xc6://subtract_i
			case 0xc7://multiply_i
				candup=false;
				if (argsneeded>=2)
				{
					b = code.peekbyteFromPosition(pos);
					pos++;
					argsneeded--;
					// if one of the arguments is a localresult, it will be reused and we don't have to increase localresultused
					if (!localresultaddedindex.count(currindex-1) && !localresultaddedindex.count(currindex-2))
						localresultused++; 
					localresultaddedindex.insert(currindex);
				}
				else
					keepchecking=false;
				lastlocalpos=-1;
				break;
			case 0xf0://debugline
			case 0xf1://debugfile
			case 0xf2://bkptline
				candup=true;
				pos = code.skipu30FromPosition(pos);
				b = code.peekbyteFromPosition(pos);
				pos++;
				break;
			case 0xc2://inclocal_i
			case 0xc3://declocal_i
				candup=false;
				switch (code.peekbyteFromPosition(pos))
				{
					case 0x63: //setlocal
					case 0xd4: //setlocal_0
					case 0xd5: //setlocal_1
					case 0xd6: //setlocal_2
					case 0xd7: //setlocal_3
						pos = code.skipu30FromPosition(pos);
						b = code.peekbyteFromPosition(pos);
						pos++;
						break;
					default:
						keepchecking=false;
						break;
				}
				break;
			case 0x63://setlocal
				candup=false;
				if (argsneeded)
				{
					pos = code.skipu30FromPosition(pos);
					b = code.peekbyteFromPosition(pos);
					pos++;
					argsneeded--;
					if (dupskipped==argsneeded)
						dupskipped=UINT32_MAX;
				}
				else
					keepchecking=false;
				break;
			case 0xd4://setlocal_0
			case 0xd5://setlocal_1
			case 0xd6://setlocal_2
			case 0xd7://setlocal_3
				candup=false;
				if (argsneeded)
				{
					b = code.peekbyteFromPosition(pos);
					pos++;
					argsneeded--;
					if (dupskipped==argsneeded)
						dupskipped=UINT32_MAX;
				}
				else
					keepchecking=false;
				break;
			default:
				keepchecking=false;
				break;
		}
		currindex++;
	}
	skipjump(state,b,code,pos,!argsneeded);
	// check if we need to store the result of the operation on stack
	switch (b)
	{
		case 0x2a://dup
			if (!argsneeded && !fromdup && state.jumptargets.find(pos) == state.jumptargets.end())
			{
				// set optimized opcode to corresponding opcode with local result 
				state.preloadedcode[preloadpos].opcode += opcode_jumpspace;
				state.preloadedcode[preloadlocalpos].pcode.local3.pos = state.mi->body->getReturnValuePos()+1+resultpos;
				state.preloadedcode[preloadlocalpos].operator_setslot=opcode_setslot;
				state.operandlist.push_back(operands(OP_LOCAL,restype,state.mi->body->getReturnValuePos()+1+resultpos,0,0));
				res = true;
			}
			else
				clearOperands(state,false,nullptr,checkchanged);
			break;
		case 0x63://setlocal
		{
			if (!argsneeded && state.jumptargets.find(pos) == state.jumptargets.end())
			{
				uint32_t num = code.peeku30FromPosition(pos);
				state.setlocal_handled.insert(pos);
				// set optimized opcode to corresponding opcode with local result 
				state.preloadedcode[preloadpos].opcode += opcode_jumpspace;
				state.preloadedcode[preloadlocalpos].pcode.local3.pos = num;
				state.localtypes[num] = restype;
				if (dupskipped==0)
					state.dupoperands.push_back(operands(OP_LOCAL,restype,num,0,0));
				res = true;
			}
			else
				clearOperands(state,false,nullptr,checkchanged);
			break;
		}
		case 0xd4://setlocal_0
		case 0xd5://setlocal_1
		case 0xd6://setlocal_2
		case 0xd7://setlocal_3
			if (!argsneeded && state.jumptargets.find(pos) == state.jumptargets.end())
			{
				state.setlocal_handled.insert(pos);
				// set optimized opcode to corresponding opcode with local result 
				state.preloadedcode[preloadpos].opcode += opcode_jumpspace;
				state.preloadedcode[preloadlocalpos].pcode.local3.pos = b-0xd4;
				state.localtypes[b-0xd4] = restype;
				if (dupskipped==0)
					state.dupoperands.push_back(operands(OP_LOCAL,restype,b-0xd4,0,0));
				res = true;
			}
			else
				clearOperands(state,false,nullptr,checkchanged);
			break;
		case 0x41://call
		{
			uint32_t argcount = code.peeku30FromPosition(pos);
			if (state.jumptargets.find(pos) == state.jumptargets.end()
					&& argcount < UINT16_MAX
					&& (argcount+1 >= argsneeded) && (state.operandlist.size() >= (argcount+1-argsneeded)))
			{
				// set optimized opcode to corresponding opcode with local result 
				state.preloadedcode[preloadpos].opcode += opcode_jumpspace;
				state.preloadedcode[preloadlocalpos].pcode.local3.pos = state.mi->body->getReturnValuePos()+1+resultpos;
				state.preloadedcode[preloadlocalpos].operator_setslot=opcode_setslot;
				state.operandlist.push_back(operands(OP_LOCAL,restype,state.mi->body->getReturnValuePos()+1+resultpos,0,0));
				res = true;
				break;
			}
			clearOperands(state,false,nullptr,checkchanged);
			break;
		}
		case 0x4f://callpropvoid
		case 0x46://callproperty
		{
			uint32_t t = code.peeku30FromPosition(pos);
			uint32_t pos2 = code.skipu30FromPosition(pos);
			uint32_t argcount = code.peeku30FromPosition(pos2);
			if (state.jumptargets.find(pos) == state.jumptargets.end()
					&& state.mi->context->constant_pool.multinames[t].runtimeargs == 0)
			{
				switch (argcount)
				{
					case 0:
						res = (argsneeded == 0);
						break;
					case 1:
						res = (argsneeded == 0 && state.operandlist.size()>0) || argsneeded==1;
						break;
					default:
						res = (argcount >= argsneeded) && (state.operandlist.size() >= argcount-argsneeded);
						break;
				}
				if (res)
				{
					// set optimized opcode to corresponding opcode with local result 
					state.preloadedcode[preloadpos].opcode += opcode_jumpspace;
					state.preloadedcode[preloadlocalpos].pcode.local3.pos = state.mi->body->getReturnValuePos()+1+resultpos;
					state.preloadedcode[preloadlocalpos].operator_setslot=opcode_setslot;
					state.operandlist.push_back(operands(OP_LOCAL,restype,state.mi->body->getReturnValuePos()+1+resultpos,0,0));
					break;
				}
			}
			clearOperands(state,false,nullptr,checkchanged);
			break;
		}
		case 0x42://construct
		{
			uint32_t argcount = code.peeku30FromPosition(pos);
			if (state.jumptargets.find(pos) == state.jumptargets.end()
					&& argcount == 0
					&& !argsneeded && (state.operandlist.size() >= 1))
			{
				// set optimized opcode to corresponding opcode with local result 
				state.preloadedcode[preloadpos].opcode += opcode_jumpspace;
				state.preloadedcode[preloadlocalpos].pcode.local3.pos = state.mi->body->getReturnValuePos()+1+resultpos;
				state.preloadedcode[preloadlocalpos].operator_setslot=opcode_setslot;
				state.operandlist.push_back(operands(OP_LOCAL,restype, state.mi->body->getReturnValuePos()+1+resultpos,0,0));
				res = true;
			}
			else
				clearOperands(state,false,nullptr,checkchanged);
			break;
		}
		case 0x4a://constructprop
		{
			uint32_t t = code.peeku30FromPosition(pos);
			uint32_t pos2 = code.skipu30FromPosition(pos);
			uint32_t argcount = code.peeku30FromPosition(pos2);
			if (state.jumptargets.find(pos) == state.jumptargets.end()
					&& argcount == 0
					&& state.mi->context->constant_pool.multinames[t].runtimeargs == 0
					&& !argsneeded && (state.operandlist.size() >= 1))
			{
				// set optimized opcode to corresponding opcode with local result 
				state.preloadedcode[preloadpos].opcode += opcode_jumpspace;
				state.preloadedcode[preloadlocalpos].pcode.local3.pos = state.mi->body->getReturnValuePos()+1+resultpos;
				state.preloadedcode[preloadlocalpos].operator_setslot=opcode_setslot;
				state.operandlist.push_back(operands(OP_LOCAL,restype,state.mi->body->getReturnValuePos()+1+resultpos,0,0));
				res = true;
			}
			else
				clearOperands(state,false,nullptr,checkchanged);
			break;
		}
		case 0x61://setproperty
		case 0x68://initproperty
		{
			uint32_t t = code.peeku30FromPosition(pos);
			if (state.jumptargets.find(pos) == state.jumptargets.end() && (argsneeded<=1) &&
					(
					   (((argsneeded==1) || (state.operandlist.size() >= 1)) 
					    && (uint32_t)state.mi->context->constant_pool.multinames[t].runtimeargs == 0)
					|| ((argsneeded==0) && (state.operandlist.size() > 1) 
						&& (uint32_t)state.mi->context->constant_pool.multinames[t].runtimeargs == 1
						&& (state.operandlist.back().objtype == Class<Integer>::getRef(state.mi->context->root->getSystemState()).getPtr()))
					)
				)
			{
				// set optimized opcode to corresponding opcode with local result 
				state.preloadedcode[preloadpos].opcode += opcode_jumpspace;
				state.preloadedcode[preloadlocalpos].pcode.local3.pos = state.mi->body->getReturnValuePos()+1+resultpos;
				state.preloadedcode[preloadlocalpos].operator_setslot=opcode_setslot;
				state.operandlist.push_back(operands(OP_LOCAL,restype,state.mi->body->getReturnValuePos()+1+resultpos,0,0));
				res = true;
			}
			else
				clearOperands(state,false,nullptr,checkchanged);
			break;
		}
		case 0x66://getproperty
		{
			uint32_t t = code.peeku30FromPosition(pos);
			uint32_t argcount = argsneeded ? 1 : 0;
			if (state.jumptargets.find(pos) == state.jumptargets.end() && (argsneeded<=1) && !fromdup &&
					((uint32_t)state.mi->context->constant_pool.multinames[t].runtimeargs == argcount
					|| (!argsneeded && (state.operandlist.size() >= 1) && state.mi->context->constant_pool.multinames[t].runtimeargs == 1)
					))
			{
				// set optimized opcode to corresponding opcode with local result 
				state.preloadedcode[preloadpos].opcode += opcode_jumpspace;
				state.preloadedcode[preloadlocalpos].pcode.local3.pos = state.mi->body->getReturnValuePos()+1+resultpos;
				state.preloadedcode[preloadlocalpos].operator_setslot=opcode_setslot;
				state.operandlist.push_back(operands(OP_LOCAL,restype,state.mi->body->getReturnValuePos()+1+resultpos,0,0));
				res = true;
			}
			else
				clearOperands(state,false,nullptr,checkchanged);
			break;
		}
		case 0x11://iftrue
		case 0x12://iffalse
			if (!argsneeded && !fromdup  && (state.jumptargets.find(pos) == state.jumptargets.end()))
			{
				// set optimized opcode to corresponding opcode with local result 
				state.preloadedcode[preloadpos].opcode += opcode_jumpspace;
				state.preloadedcode[preloadlocalpos].pcode.local3.pos = state.mi->body->getReturnValuePos()+1+resultpos;
				state.preloadedcode[preloadlocalpos].operator_setslot=opcode_setslot;
				state.operandlist.push_back(operands(OP_LOCAL,restype,state.mi->body->getReturnValuePos()+1+resultpos,0,0));
				res = true;
			}
			else
				clearOperands(state,false,nullptr,checkchanged);
			break;
		case 0x91://increment
		case 0x93://decrement
		case 0x95://typeof
		case 0x96://not
		case 0x6c://getslot
		case 0x73://convert_i
		case 0x74://convert_u
		case 0x75://convert_d
		case 0x76://convert_b
		case 0x80://coerce
		case 0xc0://increment_i
		case 0xc1://decrement_i
		case 0x35://li8
		case 0x36://li16
		case 0x37://li32
		case 0x38://lf32
		case 0x39://lf64
		case 0x1b://lookupswitch
		case 0x50://sxi1
		case 0x51://sxi8
		case 0x52://sxi16
			if (!argsneeded && (state.jumptargets.find(pos) == state.jumptargets.end()))
			{
				// set optimized opcode to corresponding opcode with local result
				state.preloadedcode[preloadpos].opcode += opcode_jumpspace;
				state.preloadedcode[preloadlocalpos].pcode.local3.pos = state.mi->body->getReturnValuePos()+1+resultpos;
				state.preloadedcode[preloadlocalpos].operator_setslot=opcode_setslot;
				state.operandlist.push_back(operands(OP_LOCAL,restype,state.mi->body->getReturnValuePos()+1+resultpos,0,0));
				res = true;
			}
			else
				clearOperands(state,false,nullptr,checkchanged);
			break;
		case 0x0c://ifnlt
		case 0x0d://ifnle
		case 0x0e://ifngt
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
		case 0x87://astypelate
		case 0xa0://add
		case 0xa1://subtract
		case 0xa2://multiply
		case 0xa3://divide
		case 0xa4://modulo
		case 0xab://equals
		case 0xad://lessthan
		case 0xae://lessequals
		case 0xaf://greaterthan
		case 0xb0://greaterequals
		case 0xb3://istypelate
		case 0x3a://si8
		case 0x3b://si16
		case 0x3c://si32
		case 0x3d://sf32
		case 0x3e://sf64
		case 0xc5://add_i
		case 0xc6://subtract_i
		case 0xc7://multiply_i
			if ((argsneeded==1 || (!argsneeded && state.operandlist.size() > 0)) && (state.jumptargets.find(pos) == state.jumptargets.end()))
			{
				// set optimized opcode to corresponding opcode with local result 
				state.preloadedcode[preloadpos].opcode += opcode_jumpspace;
				state.preloadedcode[preloadlocalpos].pcode.local3.pos = state.mi->body->getReturnValuePos()+1+resultpos;
				state.preloadedcode[preloadlocalpos].operator_setslot=opcode_setslot;
				state.operandlist.push_back(operands(OP_LOCAL,restype,state.mi->body->getReturnValuePos()+1+resultpos,0,0));
				res = true;
			}
			else
				clearOperands(state,false,nullptr,checkchanged);
			break;
		case 0xa5://lshift
		case 0xa6://rshift
		case 0xa9://bitor
		case 0xa8://bitand
		case 0xaa://bitxor
		case 0xa7://urshift
			if ((argsneeded==1 || (!argsneeded && state.operandlist.size() > 0)) && (state.jumptargets.find(pos) == state.jumptargets.end()))
			{
				switch (state.preloadedcode[preloadlocalpos].operator_start)
				{
					case ABC_OP_OPTIMZED_ADD:
					case ABC_OP_OPTIMZED_SUBTRACT:
					case ABC_OP_OPTIMZED_MULTIPLY:
					case ABC_OP_OPTIMZED_DIVIDE:
					case ABC_OP_OPTIMZED_MODULO:
						state.preloadedcode[preloadpos].pcode.local3.flags |= ABC_OP_FORCEINT;
						break;
				}
				// set optimized opcode to corresponding opcode with local result 
				state.preloadedcode[preloadpos].opcode += opcode_jumpspace;
				state.preloadedcode[preloadlocalpos].pcode.local3.pos = state.mi->body->getReturnValuePos()+1+resultpos;
				state.preloadedcode[preloadlocalpos].operator_setslot=opcode_setslot;
				state.operandlist.push_back(operands(OP_LOCAL,restype,state.mi->body->getReturnValuePos()+1+resultpos,0,0));
				res = true;
			}
			else
				clearOperands(state,false,nullptr,checkchanged);
			break;
		case 0x48://returnvalue
			if (!argsneeded && (state.jumptargets.find(pos) == state.jumptargets.end()))
			{
				// set optimized opcode to corresponding opcode with local result 
				state.preloadedcode[preloadpos].opcode += opcode_jumpspace;
				state.preloadedcode[preloadlocalpos].pcode.local3.pos = state.mi->body->getReturnValuePos();
				state.preloadedcode[preloadlocalpos].operator_setslot=opcode_setslot;
				state.operandlist.push_back(operands(OP_LOCAL,restype,state.mi->body->getReturnValuePos(),0,0));
				res = true;
			}
			else
				clearOperands(state,false,nullptr,checkchanged);
			break;
		case 0x10://jump
			// don't clear operandlist yet, because the jump may be skipped
			break;
		default:
			clearOperands(state,false,nullptr,checkchanged);
			break;
	}
	while (res && (localresultused >= state.mi->body->localresultcount))
	{
		state.mi->body->localresultcount++;
		state.localtypes.push_back(localresultused == state.mi->body->localresultcount ? restype : nullptr);
		state.defaultlocaltypes.push_back(nullptr);
		state.defaultlocaltypescacheable.push_back(true);
	}
	if (res)
		state.duplocalresult=true;
	return res;
#else
	clearOperands(state,false,nullptr,checkchanged);
	return false;
#endif
}

bool setupInstructionOneArgumentNoResult(preloadstate& state,int operator_start,int opcode,memorystream& code, uint32_t startcodepos)
{
	bool hasoperands = false;
#ifdef ENABLE_OPTIMIZATION
	hasoperands = state.jumptargets.find(startcodepos) == state.jumptargets.end() && state.operandlist.size() >= 1;
	if (hasoperands)
	{
		auto it = state.operandlist.end();
		(--it)->removeArg(state);// remove arg1
		it = state.operandlist.end();
		state.preloadedcode.push_back(operator_start);
		(--it)->fillCode(state,0,state.preloadedcode.size()-1,true);
		state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
		state.operandlist.pop_back();
	}
	else
#endif
	{
		state.preloadedcode.push_back((uint32_t)opcode);
		state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
	}
	return hasoperands;
}
bool setupInstructionTwoArgumentsNoResult(preloadstate& state,int operator_start,int opcode,memorystream& code)
{
	bool hasoperands = false;
#ifdef ENABLE_OPTIMIZATION
	hasoperands = state.operandlist.size() >= 2;
	if (hasoperands)
	{
		auto it = state.operandlist.end();
		(--it)->removeArg(state);// remove arg2
		(--it)->removeArg(state);// remove arg1
		it = state.operandlist.end();
		// optimized opcodes are in order CONSTANT/CONSTANT, LOCAL/CONSTANT, CONSTANT/LOCAL, LOCAL/LOCAL
		state.preloadedcode.emplace_back();
		(--it)->fillCode(state,1,state.preloadedcode.size()-1,true,&operator_start);
		(--it)->fillCode(state,0,state.preloadedcode.size()-1,true,&operator_start);
		state.preloadedcode.back().pcode.func = ABCVm::abcfunctions[operator_start];
		state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
		state.operandlist.pop_back();
		state.operandlist.pop_back();
	}
	else
#endif
	{
		state.preloadedcode.push_back((uint32_t)opcode);
		state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
	}
	return hasoperands;
}
bool setupInstructionOneArgument(preloadstate& state,int operator_start,int opcode,memorystream& code,bool constantsallowed, bool useargument_for_skip, Class_base* resulttype, uint32_t startcodepos, bool checkforlocalresult, bool addchanged=false,bool fromdup=false, bool checkoperands=true,uint32_t operator_start_setslot=UINT32_MAX)
{
	bool hasoperands = false;
#ifdef ENABLE_OPTIMIZATION
	hasoperands = !checkoperands || (state.jumptargets.find(startcodepos) == state.jumptargets.end() && state.operandlist.size() >= 1 && (constantsallowed || state.operandlist.back().type == OP_LOCAL|| state.operandlist.back().type == OP_CACHED_SLOT));
	Class_base* skiptype = resulttype;
	if (hasoperands)
	{
		auto it = state.operandlist.end();
		(--it)->removeArg(state);// remove arg1
		if (addchanged && (it->type == OP_LOCAL || it->type == OP_CACHED_SLOT))
			setOperandModified(state,it->type,it->index);
		it = state.operandlist.end();
		state.preloadedcode.push_back(operator_start);
		(--it)->fillCode(state,0,state.preloadedcode.size()-1,constantsallowed,nullptr,&operator_start_setslot);
		if (useargument_for_skip)
			skiptype = it->objtype;
		state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
		state.operandlist.pop_back();
	}
	else
		state.preloadedcode.push_back((uint32_t)opcode);
	if (state.jumptargets.find(code.tellg()+1) == state.jumptargets.end())
	{
		switch (code.peekbyte())
		{
			case 0x73://convert_i
				if (!constantsallowed || skiptype == Class<Integer>::getRef(state.mi->context->root->getSystemState()).getPtr())
				{
					state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
					code.readbyte();
				}
				break;
			case 0x74://convert_u
				if (!constantsallowed || skiptype == Class<UInteger>::getRef(state.mi->context->root->getSystemState()).getPtr())
				{
					state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
					code.readbyte();
				}
				break;
			case 0x75://convert_d
				if (!constantsallowed || skiptype == Class<Number>::getRef(state.mi->context->root->getSystemState()).getPtr()  || skiptype == Class<Integer>::getRef(state.mi->context->root->getSystemState()).getPtr() || skiptype == Class<UInteger>::getRef(state.mi->context->root->getSystemState()).getPtr())
				{
					state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
					code.readbyte();
				}
				break;
			case 0x76://convert_b
				if (!constantsallowed || skiptype == Class<Boolean>::getRef(state.mi->context->root->getSystemState()).getPtr())
				{
					state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
					code.readbyte();
				}
				break;
			case 0x82://coerce_a
				state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
				code.readbyte();
				break;
		}
	}
	if (hasoperands && checkforlocalresult)
		checkForLocalResult(state,code,constantsallowed ? 2 : 1,resulttype,-1,-1,false,fromdup,operator_start_setslot);
	else
		clearOperands(state,false,nullptr);
#else
	state.preloadedcode.push_back((uint32_t)opcode);
#endif
	return hasoperands;
}

bool setupInstructionTwoArguments(preloadstate& state,int operator_start,int opcode,memorystream& code, bool skip_conversion,bool cancollapse,bool checklocalresult, uint32_t startcodepos,Class_base* resulttype = nullptr,uint32_t operator_start_setslot=UINT32_MAX)
{
	bool hasoperands = false;
#ifdef ENABLE_OPTIMIZATION
	hasoperands = state.jumptargets.find(startcodepos) == state.jumptargets.end() && state.operandlist.size() >= 2;
	bool op1isconstant=false;
	bool op2isconstant=false;
	if (hasoperands)
	{
		auto it = state.operandlist.end();
		if (cancollapse && (--it)->type != OP_LOCAL && it->type != OP_CACHED_SLOT && (--it)->type != OP_LOCAL && it->type != OP_CACHED_SLOT) // two constants means we can compute the result now and use it
		{
			it =state.operandlist.end();
			(--it)->removeArg(state);// remove arg2
			(--it)->removeArg(state);// remove arg1
			it = state.operandlist.end();
			--it;
			asAtom* op2 = state.mi->context->getConstantAtom((it)->type,(it)->index);
			--it;
			asAtom* op1 = state.mi->context->getConstantAtom((it)->type,(it)->index);
			asAtom res = *op1;
			switch (operator_start)
			{
				case ABC_OP_OPTIMZED_SUBTRACT:
					asAtomHandler::subtract(res,state.worker,*op2,false);
					break;
				case ABC_OP_OPTIMZED_MULTIPLY:
					asAtomHandler::multiply(res,state.worker,*op2,false);
					break;
				case ABC_OP_OPTIMZED_DIVIDE:
					asAtomHandler::divide(res,state.worker,*op2,false);
					break;
				case ABC_OP_OPTIMZED_MODULO:
					asAtomHandler::modulo(res,state.worker,*op2,false);
					break;
				case ABC_OP_OPTIMZED_LSHIFT:
					asAtomHandler::lshift(res,state.worker,*op2);
					break;
				case ABC_OP_OPTIMZED_RSHIFT:
					asAtomHandler::rshift(res,state.worker,*op2);
					break;
				case ABC_OP_OPTIMZED_URSHIFT:
					asAtomHandler::urshift(res,state.worker,*op2);
					break;
				case ABC_OP_OPTIMZED_BITAND:
					asAtomHandler::bit_and(res,state.worker,*op2);
					break;
				case ABC_OP_OPTIMZED_BITOR:
					asAtomHandler::bit_or(res,state.worker,*op2);
					break;
				case ABC_OP_OPTIMZED_BITXOR:
					asAtomHandler::bit_xor(res,state.worker,*op2);
					break;
				default:
					LOG(LOG_ERROR,"setupInstructionTwoArguments: trying to collapse invalid opcode:"<<hex<<operator_start);
					break;
			}
			state.operandlist.pop_back();
			state.operandlist.pop_back();
			if (asAtomHandler::isObject(res))
				asAtomHandler::getObjectNoCheck(res)->setRefConstant();
			uint32_t value = state.mi->context->addCachedConstantAtom(res);
			state.preloadedcode.push_back(ABC_OP_OPTIMZED_PUSHCACHEDCONSTANT);
			state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
			state.preloadedcode.back().pcode.arg3_uint=value;
			state.operandlist.push_back(operands(OP_CACHED_CONSTANT,asAtomHandler::getClass(res,state.mi->context->root->getSystemState()), value,1,state.preloadedcode.size()-1));
			return true;
		}
		it = state.operandlist.end();
		op2isconstant = ((--it)->type != OP_LOCAL && it->type != OP_CACHED_SLOT);
		op1isconstant = ((--it)->type != OP_LOCAL && it->type != OP_CACHED_SLOT);
		
		it =state.operandlist.end();
		(--it)->removeArg(state);// remove arg2
		(--it)->removeArg(state);// remove arg1
		it = state.operandlist.end();
		// optimized opcodes are in order CONSTANT/CONSTANT, LOCAL/CONSTANT, CONSTANT/LOCAL, LOCAL/LOCAL
		state.preloadedcode.push_back(operator_start);
		(--it)->fillCode(state,1,state.preloadedcode.size()-1,true,nullptr,&operator_start_setslot);
		Class_base* op1type = it->objtype;
		(--it)->fillCode(state,0,state.preloadedcode.size()-1,true,nullptr,&operator_start_setslot);
		Class_base* op2type = it->objtype;
		state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
		state.operandlist.pop_back();
		state.operandlist.pop_back();
		switch (operator_start)
		{
			case ABC_OP_OPTIMZED_ADD:
				// if both operands are numeric, the result is always a number, so we can skip convert_d opcode
				skip_conversion = 
						(op1type == Class<Integer>::getRef(state.mi->context->root->getSystemState()).getPtr() || op1type == Class<UInteger>::getRef(state.mi->context->root->getSystemState()).getPtr() || op1type == Class<Number>::getRef(state.mi->context->root->getSystemState()).getPtr()) &&
						(op2type == Class<Integer>::getRef(state.mi->context->root->getSystemState()).getPtr() || op2type == Class<UInteger>::getRef(state.mi->context->root->getSystemState()).getPtr() || op2type == Class<Number>::getRef(state.mi->context->root->getSystemState()).getPtr());
				if (skip_conversion)
				{
					if (op1type == Class<Integer>::getRef(state.mi->context->root->getSystemState()).getPtr() && op2type == Class<Integer>::getRef(state.mi->context->root->getSystemState()).getPtr())
						resulttype = Class<Integer>::getRef(state.mi->context->root->getSystemState()).getPtr();
					else
						resulttype = Class<Number>::getRef(state.mi->context->root->getSystemState()).getPtr();
				}
				setForceInt(state,code,&resulttype);
				break;
			case ABC_OP_OPTIMZED_MODULO:
				if (op1type == Class<Integer>::getRef(state.mi->context->root->getSystemState()).getPtr() && op2type == Class<Integer>::getRef(state.mi->context->root->getSystemState()).getPtr())
					resulttype = Class<Integer>::getRef(state.mi->context->root->getSystemState()).getPtr();
				else
					resulttype = Class<Number>::getRef(state.mi->context->root->getSystemState()).getPtr();
				setForceInt(state,code,&resulttype);
				break;
			case ABC_OP_OPTIMZED_SUBTRACT:
			case ABC_OP_OPTIMZED_MULTIPLY:
			case ABC_OP_OPTIMZED_DIVIDE:
				resulttype = Class<Number>::getRef(state.mi->context->root->getSystemState()).getPtr();
				setForceInt(state,code,&resulttype);
				break;
			case ABC_OP_OPTIMZED_ADD_I:
			case ABC_OP_OPTIMZED_SUBTRACT_I:
			case ABC_OP_OPTIMZED_MULTIPLY_I:
				if (op1isconstant)
					state.preloadedcode[state.preloadedcode.size()-1].pcode.arg1_int= asAtomHandler::toInt(*state.preloadedcode.back().pcode.arg1_constant);
				if (op2isconstant)
					state.preloadedcode[state.preloadedcode.size()-1].pcode.arg2_int= asAtomHandler::toInt(*state.preloadedcode.back().pcode.arg2_constant);
				setForceInt(state,code,&resulttype);
				resulttype = Class<Integer>::getRef(state.mi->context->root->getSystemState()).getPtr();
				break;
			case ABC_OP_OPTIMZED_RSHIFT:
				resulttype = Class<Integer>::getRef(state.mi->context->root->getSystemState()).getPtr();
				break;
			case ABC_OP_OPTIMZED_URSHIFT:
				resulttype = Class<Integer>::getRef(state.mi->context->root->getSystemState()).getPtr();
				// operators are always transformed to uint, so we can do that here if the operators are constants
				if (op1isconstant)
					state.preloadedcode[state.preloadedcode.size()-1].pcode.arg1_int=asAtomHandler::toUInt(*state.preloadedcode[state.preloadedcode.size()-1].pcode.arg1_constant);
				if (op2isconstant)
					state.preloadedcode[state.preloadedcode.size()-1].pcode.arg2_int=asAtomHandler::toUInt(*state.preloadedcode[state.preloadedcode.size()-1].pcode.arg2_constant);
				break;
			case ABC_OP_OPTIMZED_LSHIFT:
			case ABC_OP_OPTIMZED_BITOR:
			case ABC_OP_OPTIMZED_BITAND:
			case ABC_OP_OPTIMZED_BITXOR:
				resulttype = Class<Integer>::getRef(state.mi->context->root->getSystemState()).getPtr();
				// operators are always transformed to int, so we can do that here if the operators are constants
				if (op1isconstant)
					state.preloadedcode[state.preloadedcode.size()-1].pcode.arg1_int=asAtomHandler::toInt(*state.preloadedcode[state.preloadedcode.size()-1].pcode.arg1_constant);
				if (op2isconstant)
					state.preloadedcode[state.preloadedcode.size()-1].pcode.arg2_int=asAtomHandler::toInt(*state.preloadedcode[state.preloadedcode.size()-1].pcode.arg2_constant);
				break;
			default:
				break;
		}
	}
	else
		state.preloadedcode.push_back((uint32_t)opcode);
	if (checklocalresult)
	{
		if (hasoperands)
			checkForLocalResult(state,code,4, resulttype,-1,-1,false,false,operator_start_setslot);
		else
			clearOperands(state,false,nullptr);
	}
#else
	state.preloadedcode.push_back((uint32_t)opcode);
#endif
	return hasoperands;
}
bool checkmatchingLastObjtype(preloadstate& state, const Class_base* resulttype, Class_base* requiredtype)
{
	if (requiredtype == resulttype)
		return true;
	if (requiredtype == Class<Number>::getRef(state.mi->context->root->getSystemState()).getPtr() && 
			(resulttype == Class<Integer>::getRef(state.mi->context->root->getSystemState()).getPtr()
			|| resulttype == Class<UInteger>::getRef(state.mi->context->root->getSystemState()).getPtr()))
		return true;
	if (resulttype== Class<Number>::getRef(state.mi->context->root->getSystemState()).getPtr() &&
			(requiredtype == Class<Integer>::getRef(state.mi->context->root->getSystemState()).getPtr()))
	{
		switch(state.preloadedcode.at(state.preloadedcode.size()-1).opcode)
		{
			case ABC_OP_OPTIMZED_ADD+4:
			case ABC_OP_OPTIMZED_ADD+5:
			case ABC_OP_OPTIMZED_ADD+6:
			case ABC_OP_OPTIMZED_ADD+7:
			case ABC_OP_OPTIMZED_SUBTRACT+4:
			case ABC_OP_OPTIMZED_SUBTRACT+5:
			case ABC_OP_OPTIMZED_SUBTRACT+6:
			case ABC_OP_OPTIMZED_SUBTRACT+7:
			case ABC_OP_OPTIMZED_MULTIPLY+4:
			case ABC_OP_OPTIMZED_MULTIPLY+5:
			case ABC_OP_OPTIMZED_MULTIPLY+6:
			case ABC_OP_OPTIMZED_MULTIPLY+7:
			case ABC_OP_OPTIMZED_DIVIDE+4:
			case ABC_OP_OPTIMZED_DIVIDE+5:
			case ABC_OP_OPTIMZED_DIVIDE+6:
			case ABC_OP_OPTIMZED_DIVIDE+7:
			case ABC_OP_OPTIMZED_MODULO+4:
			case ABC_OP_OPTIMZED_MODULO+5:
			case ABC_OP_OPTIMZED_MODULO+6:
			case ABC_OP_OPTIMZED_MODULO+7:
			{
				state.preloadedcode.at(state.preloadedcode.size()-1).pcode.local3.flags |= ABC_OP_FORCEINT;
				return true;
			}
			default:
				break;
		}
	}
	return false;
}
void addCachedConstant(preloadstate& state,method_info* mi, asAtom& val,memorystream& code)
{
	if (asAtomHandler::isObject(val))
		asAtomHandler::getObject(val)->setRefConstant();
	uint32_t value = mi->context->addCachedConstantAtom(val);
	state.preloadedcode.push_back(ABC_OP_OPTIMZED_PUSHCACHEDCONSTANT);
	state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
	state.preloadedcode.back().pcode.arg3_uint=value;
	state.operandlist.push_back(operands(OP_CACHED_CONSTANT,asAtomHandler::getClass(val,mi->context->root->getSystemState()),value,1,state.preloadedcode.size()-1));
}
void addCachedSlot(preloadstate& state, uint32_t localpos, uint32_t slotid,memorystream& code,Class_base* resulttype)
{
	uint32_t value = 0;
	for (auto it = state.mi->body->localconstantslots.begin(); it != state.mi->body->localconstantslots.end(); it++)
	{
		if (it->local_pos == localpos && it->slot_number == slotid)
			break;
		value++;
	}
	if (value == state.mi->body->localconstantslots.size())
	{
		localconstantslot sl;
		sl.local_pos= localpos;
		sl.slot_number = slotid;
		state.mi->body->localconstantslots.push_back(sl);
	}
	state.preloadedcode.push_back(ABC_OP_OPTIMZED_PUSHCACHEDSLOT);
	state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
	state.preloadedcode.back().pcode.arg3_uint = value;
	state.operandlist.push_back(operands(OP_CACHED_SLOT,resulttype,value,1,state.preloadedcode.size()-1));
}
void setdefaultlocaltype(preloadstate& state,uint32_t t,Class_base* c)
{
	if (c==nullptr && t < state.defaultlocaltypescacheable.size())
	{
		state.defaultlocaltypes[t] = nullptr;
		state.defaultlocaltypescacheable[t]=false;
		return;
	}
	if (t < state.defaultlocaltypescacheable.size() && state.defaultlocaltypescacheable[t])
	{
		if (state.defaultlocaltypes[t] == nullptr)
			state.defaultlocaltypes[t] = c;
		if (state.defaultlocaltypes[t] != c)
		{
			state.defaultlocaltypescacheable[t]=false;
			state.defaultlocaltypes[t]=nullptr;
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
#ifdef ENABLE_OPTIMIZATION
	assert_and_throw(uint32_t(n) <= typestack.size());
	for (int i = 0; i < n; i++)
	{
		typestack.pop_back();
	}
#endif
}
void skipunreachablecode(preloadstate& state, memorystream& code, bool updatetargets=true)
{
	while (!code.atend() && state.jumptargets.find(code.tellg()+1) == state.jumptargets.end())
	{
		uint8_t b = code.readbyte();
		switch (b)
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
			case 0x08://kill
			case 0x62://getlocal
			case 0x80://coerce
			case 0xf0://debugline
			case 0xf1://debugfile
			case 0xf2://bkptline
			case 0x2c://pushstring
			case 0x2d://pushint
			case 0x2e://pushuint
			case 0x2f://pushdouble
			case 0x31://pushnamespace
			case 0x63://setlocal
			case 0x92://inclocal
			case 0x94://declocal
			case 0xc2://inclocal_i
			case 0xc3://declocal_i
				code.readu30();
				break;
			case 0x1b://lookupswitch
			{
				code.reads24();
				uint32_t count = code.readu30();
				for(unsigned int i=0;i<count+1;i++)
				{
					code.reads24();
				}
				break;
			}
			case 0x24://pushbyte
				code.readbyte();
				break;
			case 0x25://pushshort
				code.readu32();
				break;
			case 0x32://hasnext2
			case 0x43://callmethod
			case 0x44://callstatic
			case 0x45://callsuper
			case 0x46://callproperty
			case 0x4c://callproplex
			case 0x4a://constructprop
			case 0x4e://callsupervoid
			case 0x4f://callpropvoid
				code.readu30();
				code.readu30();
				break;
			case 0xef://debug
				code.readbyte();
				code.readu30();
				code.readbyte();
				code.readu30();
				break;
			case 0x10://jump
			case 0x0c://ifnlt
			case 0x0d://ifnle
			case 0x0e://ifngt
			case 0x0f://ifnge
			case 0x13://ifeq
			case 0x14://ifne
			case 0x15://iflt
			case 0x16://ifle
			case 0x17://ifgt
			case 0x18://ifge
			case 0x19://ifstricteq
			case 0x1a://ifstrictne
			case 0x11://iftrue
			case 0x12://iffalse
			{
				// make sure that unreachable jumps get erased from jumptargets
				int32_t p1 = code.reads24()+code.tellg()+1;
				if (updatetargets)
				{
					auto it = state.jumptargets.find(p1);
					if (it != state.jumptargets.end() && it->second > 1)
						state.jumptargets[p1]--;
					else
						state.jumptargets.erase(p1);
				}
				break;
			}
		}
	}
}
bool checkforpostfix(preloadstate& state,memorystream& code,uint32_t startpos,std::vector<typestackentry>& typestack, int32_t localpos, uint32_t postfix_opcode)
{
#ifdef ENABLE_OPTIMIZATION
	if (!state.operandlist.empty() && state.jumptargets.find(startpos) == state.jumptargets.end())
	{
		uint32_t pos = code.tellg();
		if (state.jumptargets.find(pos) == state.jumptargets.end())
		{
			if (code.peekbyteFromPosition(pos) ==0x73) //convert_i
				++pos;
		}
		uint32_t loc = UINT32_MAX;
		if (state.jumptargets.find(pos) == state.jumptargets.end())
		{
			uint8_t peekopcode=code.peekbyteFromPosition(pos);
			switch (peekopcode)
			{
				case 0x63: //setlocal
					loc = code.peeku30FromPosition(pos+1);
					pos = code.skipu30FromPosition(pos+1);
					break;
				case 0xd4: //setlocal_0
				case 0xd5: //setlocal_1
				case 0xd6: //setlocal_2
				case 0xd7: //setlocal_3
					loc = peekopcode-0xd4;
					++pos;
					break;
				default:
					break;
			}
			if (loc != UINT32_MAX && state.jumptargets.find(pos) == state.jumptargets.end())
			{
				if (state.operandlist.back().type == OP_LOCAL && state.operandlist.back().index == localpos)
				{
					// code is a postfix increment/decrement where the result is used directly
					// actionscript code like:
					// y = x++;
					// is  translated sequence is of form
					// getlocal x
					// inclocal_i x
					// convert_i
					// setlocal y
					state.operandlist.pop_back();
					state.preloadedcode.pop_back(); // remove getlocal opcode
					state.preloadedcode.push_back(postfix_opcode);
					state.preloadedcode.back().pcode.arg1_uint = localpos;
					state.preloadedcode.back().pcode.local3.pos = loc;
					state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
					setOperandModified(state,OP_LOCAL,localpos);
					code.seekg(pos);
					return true;
				}
				// current opcode is followed by setlocal, we can swap these opcodes to let setlocal be optimized
				setupInstructionOneArgumentNoResult(state,ABC_OP_OPTIMZED_SETLOCAL,peekopcode,code,startpos);
				state.preloadedcode.at(state.preloadedcode.size()-1).pcode.arg3_uint =loc;
				if (typestack.back().obj && typestack.back().obj->is<Class_base>())
					state.localtypes[loc]=typestack.back().obj->as<Class_base>();
				else
					state.localtypes[loc]=nullptr;
				removetypestack(typestack,1);
				code.seekg(pos);
			}
		}
	}
#endif
	return false;
}

void setupInstructionComparison(preloadstate& state,int operator_start,int opcode,memorystream& code, int operator_replace,int operator_replace_reverse,std::vector<typestackentry>& typestack,Class_base** lastlocalresulttype,std::map<int32_t,int32_t>& jumppositions, std::map<int32_t,int32_t>& jumpstartpositions)
{
	if (setupInstructionTwoArguments(state,operator_start,opcode,code,false,false,true,code.tellg(),Class<Boolean>::getRef(state.mi->context->root->getSystemState()).getPtr()))
	{
#ifdef ENABLE_OPTIMIZATION
		if (state.preloadedcode.back().opcode >= uint32_t(operator_start+4)) // has localresult
		{
			uint32_t pos = code.tellg();
			bool ok = state.jumptargets.find(pos) == state.jumptargets.end() && state.jumptargets.find(pos+1) == state.jumptargets.end();
			bool isnot = code.peekbyteFromPosition(pos) == 0x96; //not
			if (ok && isnot)
			{
				pos++;
				ok = state.jumptargets.find(pos) == state.jumptargets.end();
			}
			if (ok && code.peekbyteFromPosition(pos) == 0x76) //convert_b
			{
				pos++;
				ok = state.jumptargets.find(pos) == state.jumptargets.end();
			}
			if (ok && (state.jumptargets.find(pos+1) == state.jumptargets.end()))
			{
				if (code.peekbyteFromPosition(pos) == 0x11 || //iftrue
						code.peekbyteFromPosition(pos) == 0x12 ) //iffalse
				{
					// comparison operator is followed by iftrue/iffalse, can be optimized into comparison operator with jump (e.g. equals->ifeq/ifne)
					code.seekg(pos);
					state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
					uint8_t b = code.readbyte();
					assert(b== 0x11 || b== 0x12);
					int j = code.reads24();
					int32_t p1 = code.tellg();
					uint32_t opcodeskip = state.preloadedcode.back().opcode - (operator_start+4);
					if (isnot)
						state.preloadedcode.back().opcode = (b==0x12 ? operator_replace : operator_replace_reverse) + opcodeskip;
					else
						state.preloadedcode.back().opcode = (b==0x11 ? operator_replace : operator_replace_reverse) + opcodeskip;
					state.preloadedcode.back().pcode.func = nullptr;
					jumppositions[state.preloadedcode.size()-1] = j;
					jumpstartpositions[state.preloadedcode.size()-1] = p1;
					clearOperands(state,true,lastlocalresulttype);
					removetypestack(typestack,2);
					return;
				}
			}
		}
		else
		{
			uint32_t pos = code.tellg();
			bool ok = state.jumptargets.find(pos) == state.jumptargets.end();
			if (ok && code.peekbyteFromPosition(pos) == 0x76) //convert_b
			{
				code.readbyte();
			}
		}
#endif
	}
	removetypestack(typestack,2);
	typestack.push_back(typestackentry(Class<Boolean>::getRef(state.mi->context->root->getSystemState()).getPtr(),false));
}


void setupInstructionIncDecInteger(preloadstate& state,memorystream& code,std::vector<typestackentry>& typestack,Class_base** lastlocalresulttype,int& dup_indicator, uint8_t opcode)
{
	removetypestack(typestack,1);
	uint32_t p = code.tellg();
	// optimize common case of increment/decrement local variable
#ifdef ENABLE_OPTIMIZATION
	if (state.operandlist.size() > 0 && 
			state.operandlist.back().type == OP_LOCAL && 
			state.operandlist.back().objtype == Class<Integer>::getRef(state.function->getSystemState()).getPtr() &&
			state.jumptargets.find(code.tellg()+1) == state.jumptargets.end())
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
		if (t == state.operandlist.back().index)
		{
			state.operandlist.back().removeArg(state);
			state.preloadedcode.push_back(opcode == 0xc0 ? ABC_OP_OPTIMZED_INCLOCAL_I : ABC_OP_OPTIMZED_DECLOCAL_I); //inclocal_i/declocal_i
			state.preloadedcode.back().pcode.arg1_uint = t;
			state.preloadedcode.back().pcode.arg2_uint = 1;
			state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
			if (code.readbyte() == 0x63) //setlocal
				code.readbyte();
			state.operandlist.pop_back();
			setOperandModified(state,OP_LOCAL,t);
			if (dup_indicator)
				clearOperands(state,false,lastlocalresulttype);
			return;
		}
	}
#endif
	setupInstructionOneArgument(state,opcode == 0xc0 ? ABC_OP_OPTIMZED_INCREMENT_I : ABC_OP_OPTIMZED_DECREMENT_I,opcode,code,false,true, Class<Integer>::getRef(state.function->getSystemState()).getPtr(),p,true,true,false,true,opcode == 0xc0 ? ABC_OP_OPTIMZED_INCREMENT_I_SETSLOT : ABC_OP_OPTIMZED_DECREMENT_I_SETSLOT);
	dup_indicator=0;
	typestack.push_back(typestackentry(Class<Integer>::getRef(state.mi->context->root->getSystemState()).getPtr(),false));
}

bool checkInitializeLocalToConstant(preloadstate& state,int32_t value)
{
	value -= state.mi->numArgs()+1;
	if (state.operandlist.size() && state.operandlist.back().type != OP_LOCAL && state.operandlist.back().type != OP_CACHED_SLOT
			&& value >= 0 && value < int(state.canlocalinitialize.size()) && state.canlocalinitialize[value])
	{
		// local is initialized to constant value
		if (!state.mi->body->localsinitialvalues)
		{
			state.mi->body->localsinitialvalues = new asAtom[state.mi->body->local_count -(state.mi->numArgs()+1)];
			memset(state.mi->body->localsinitialvalues,ATOMTYPE_UNDEFINED_BIT,(state.mi->body->local_count -(state.mi->numArgs()+1))*sizeof(asAtom));
		}
		state.mi->body->localsinitialvalues[value]= *state.mi->context->getConstantAtom(state.operandlist.back().type,state.operandlist.back().index);
		state.canlocalinitialize[value]=false;
		state.operandlist.back().removeArg(state);
		state.operandlist.pop_back();
		return true;
	}
	if (value >= 0 && value < int(state.canlocalinitialize.size()))
		state.canlocalinitialize[value]=false;
	return false;
}
void removeInitializeLocalToConstant(preloadstate& state,int32_t value)
{
	value -= state.mi->numArgs()+1;
	if (value >= 0 && value < int(state.canlocalinitialize.size()))
	{
		state.canlocalinitialize[value]=false;
	}
}

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
#ifdef ENABLE_OPTIMIZATION
	Class_base* currenttype=nullptr;
	memorystream codetypes(mi->body->code.data(), code_len);
	while(!codetypes.atend())
	{
		uint8_t prevopcode=opcode;
		opcode = codetypes.readbyte();
		//LOG(LOG_ERROR,"preload pass2:"<<function->getSystemState()->getStringFromUniqueId(function->functionname)<<" "<< codetypes.tellg()-1<<" "<<currenttype<<" "<<hex<<(int)opcode);
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
			case 0xb2://istype
			case 0x08://kill
			case 0x62://getlocal
				codetypes.readu30();
				currenttype=nullptr;
				break;
			case 0x86://astype
			case 0x80://coerce
			{
				uint32_t t = codetypes.readu30();
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
				codetypes.readu30();
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
				codetypes.readu30();
				currenttype=Class<ASString>::getRef(function->getSystemState()).getPtr();
				break;
			case 0x2d://pushint
				codetypes.readu30();
				currenttype=Class<Integer>::getRef(function->getSystemState()).getPtr();
				break;
			case 0x2e://pushuint
				codetypes.readu30();
				currenttype=Class<UInteger>::getRef(function->getSystemState()).getPtr();
				break;
			case 0x2f://pushdouble
				codetypes.readu30();
				currenttype=Class<Number>::getRef(function->getSystemState()).getPtr();
				break;
			case 0x31://pushnamespace
				codetypes.readu30();
				currenttype=Class<Namespace>::getRef(function->getSystemState()).getPtr();
				break;
			case 0x28://pushnan
				currenttype=Class<Number>::getRef(function->getSystemState()).getPtr();
				break;
			case 0x63://setlocal
			{
				uint32_t t = codetypes.readu30();
				state.unchangedlocals.erase(t);
				setdefaultlocaltype(state,t,currenttype);
				currenttype=nullptr;
				break;
			}
			case 0xd4://setlocal_0
			case 0xd5://setlocal_1
			case 0xd6://setlocal_2
			case 0xd7://setlocal_3
				state.unchangedlocals.erase(opcode-0xd4);
				setdefaultlocaltype(state,opcode-0xd4,currenttype);
				currenttype=nullptr;
				break;
			case 0x92://inclocal
			case 0x94://declocal
			{
				uint32_t t = codetypes.readu30();
				state.unchangedlocals.erase(t);
				setdefaultlocaltype(state,t,Class<Number>::getRef(function->getSystemState()).getPtr());
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
				uint32_t t = codetypes.readu30();
				state.unchangedlocals.erase(t);
				setdefaultlocaltype(state,t,Class<Integer>::getRef(function->getSystemState()).getPtr());
				currenttype=nullptr;
				break;
			}
			case 0x10://jump
			{
				int32_t p = codetypes.tellg();
				int32_t p1 = codetypes.reads24()+codetypes.tellg()+1;
				if (p1 > p)
					skipunreachablecode(state,codetypes,false);
				if (!state.jumptargets.count(p1) && currenttype)
					state.jumptargeteresulttypes[p1] = currenttype;
				currenttype=nullptr;
				break;
			}
			case 0x0c://ifnlt
			case 0x0d://ifnle
			case 0x0e://ifngt
			case 0x0f://ifnge
			case 0x13://ifeq
			case 0x14://ifne
			case 0x15://iflt
			case 0x16://ifle
			case 0x17://ifgt
			case 0x18://ifge
			case 0x19://ifstricteq
			case 0x1a://ifstrictne
			{
				codetypes.reads24();
				currenttype=nullptr;
				break;
			}
			case 0x11://iftrue
			{
				int32_t p = codetypes.tellg();
				int32_t p1 = codetypes.reads24()+codetypes.tellg()+1;
				if (p1 > p && prevopcode==0x26) //pushtrue
				{
					skipunreachablecode(state,codetypes,false);
				}
				currenttype=nullptr;
				break;
			}
			case 0x12://iffalse
			{
				int32_t p = codetypes.tellg();
				int32_t p1 = codetypes.reads24()+codetypes.tellg()+1;
				if (p1 > p && prevopcode==0x27) //pushfalse
				{
					skipunreachablecode(state,codetypes,false);
				}
				currenttype=nullptr;
				break;
			}
			case 0x1b://lookupswitch
			{
				codetypes.reads24();
				uint32_t count = codetypes.readu30();
				for(unsigned int i=0;i<count+1;i++)
				{
					codetypes.reads24();
				}
				currenttype=nullptr;
				break;
			}
			case 0x24://pushbyte
			{
				codetypes.readbyte();
				currenttype=Class<Integer>::getRef(function->getSystemState()).getPtr();
				break;
			}
			case 0x25://pushshort
			{
				codetypes.readu32();
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
				codetypes.readu30();
				codetypes.readu30();
				currenttype=nullptr;
				break;
			}
			case 0xef://debug
			{
				codetypes.readbyte();
				codetypes.readu30();
				codetypes.readbyte();
				codetypes.readu30();
				break;
			}
			case 0xa5://lshift
			case 0xa6://rshift
			case 0xa8://bitand
			case 0xa9://bitor
				currenttype=Class<Integer>::getRef(function->getSystemState()).getPtr();
				break;
			case 0x09://label
			case 0x2a://dup
				break;
			case 0x47://returnvoid
			case 0x48://returnvalue
			case 0x03://throw
				skipunreachablecode(state,codetypes,false);
				currenttype=nullptr;
				break;
			default:
				currenttype=nullptr;
				break;
		}
	}
#endif
	// third pass:
	// - use optimized opcode version if it doesn't interfere with a jump target
	
	std::vector<typestackentry> typestack; // contains the type or the global object of the arguments currently on the stack
	Class_base* lastlocalresulttype=nullptr;
	clearOperands(state,true,&lastlocalresulttype);
	memorystream code(mi->body->code.data(), code_len);
	std::list<scope_entry> scopelist;
	Activation_object* activationobject=nullptr;
	int dup_indicator=0;
	bool opcode_skipped=false;
	bool coercereturnvalue=false;
	bool reverse_iftruefalse=false;
	auto itcurEx = mi->body->exceptions.begin();
	opcode=0;
	while(!code.atend())
	{
		state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
		if (state.jumptargets.find(code.tellg()) != state.jumptargets.end())
			state.canlocalinitialize.clear();
		while (itcurEx != mi->body->exceptions.end() && itcurEx->target == code.tellg())
		{
			typestack.push_back(typestackentry(nullptr,false));
			itcurEx++;
		}
		uint8_t prevopcode=opcode;
		opcode = code.readbyte();
//		if (typestack.empty() || typestack.back().obj==nullptr)
//			LOG(LOG_INFO,"preload pass3 opcode:"<<function->getSystemState()->getStringFromUniqueId(function->functionname)<<" "<< code.tellg()-1<<" "<<state.operandlist.size()<<" "<<typestack.size()<<" "<<state.preloadedcode.size()<<" "<<hex<<(int)opcode);
//		else
//			LOG(LOG_INFO,"preload pass3 opcode:"<<function->getSystemState()->getStringFromUniqueId(function->functionname)<<" "<< code.tellg()-1<<" "<<state.operandlist.size()<<" "<<typestack.size()<<" "<<typestack.back().obj->toDebugString()<<" "<<state.preloadedcode.size()<<" "<< hex<<(int)opcode);
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
				state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
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
				state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
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
				state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
				state.preloadedcode.back().pcode.arg3_uint=t;
				clearOperands(state,true,&lastlocalresulttype);
				break;
			}
			case 0x40://newfunction
			{
				uint32_t t = code.readu30();
				typestack.push_back(typestackentry(nullptr,false));
				state.preloadedcode.push_back((uint32_t)opcode);
				state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
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
				state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
				state.preloadedcode.back().pcode.arg1_uint = t;
				clearOperands(state,true,&lastlocalresulttype);
				break;
			}
			case 0x53://constructgenerictype
			{
				uint32_t t = code.readu30();
				removetypestack(typestack,t+1);
				typestack.push_back(typestackentry(nullptr,false));
				state.preloadedcode.push_back((uint32_t)opcode);
				state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
				state.preloadedcode.back().pcode.arg3_uint=t;
				clearOperands(state,true,&lastlocalresulttype);
				break;
			}
			case 0x55://newobject
			{
				uint32_t t = code.readu30();
				removetypestack(typestack,t*2);
				typestack.push_back(typestackentry(nullptr,false));
				state.preloadedcode.push_back((uint32_t)opcode);
				state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
				state.preloadedcode.back().pcode.arg3_uint=t;
				clearOperands(state,true,&lastlocalresulttype);
				break;
			}
			case 0x56://newarray
			{
				uint32_t t = code.readu30();
				removetypestack(typestack,t);
				typestack.push_back(typestackentry(nullptr,false));
				state.preloadedcode.push_back((uint32_t)opcode);
				state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
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
				state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
				state.preloadedcode.back().pcode.arg3_uint=t;
				clearOperands(state,true,&lastlocalresulttype);
				break;
			}
			case 0x5a://newcatch
			{
				uint32_t t = code.readu30();
				typestack.push_back(typestackentry(Class<ASObject>::getRef(function->getSystemState()).getPtr(),false));
				state.preloadedcode.push_back((uint32_t)opcode);
				state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
				state.preloadedcode.back().pcode.arg3_uint=t;
				clearOperands(state,true,&lastlocalresulttype);
				break;
			}
			case 0x6e://getglobalSlot
			{
				uint32_t t = code.readu30();
				typestack.push_back(typestackentry(nullptr,false));
				state.preloadedcode.push_back((uint32_t)opcode);
				state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
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
				typestack.push_back(typestackentry(Class<Boolean>::getRef(mi->context->root->getSystemState()).getPtr(),false));
				state.preloadedcode.push_back((uint32_t)opcode);
				state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
				state.preloadedcode.back().pcode.arg1_uint=t;
				clearOperands(state,true,&lastlocalresulttype);
				break;
			}
			case 0x6f://setglobalSlot
			{
				uint32_t t = code.readu30();
				removetypestack(typestack,1);
				state.preloadedcode.push_back((uint32_t)opcode);
				state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
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
				state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
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
					state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
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
					state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
					setOperandModified(state,OP_LOCAL,t);
					clearOperands(state,true,&lastlocalresulttype);
				}
				break;
			}
			case 0x08://kill
			{
				int32_t p = code.tellg();
				state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
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
				state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
				uint32_t t = code.readu30();
				state.preloadedcode.back().pcode.arg1_uint=t;
				setOperandModified(state,OP_LOCAL,t);
				clearOperands(state,true,&lastlocalresulttype,true,OP_LOCAL,t);
				break;
			}
			case 0x06://dxns
			{
				state.preloadedcode.push_back((uint32_t)opcode);
				state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
				state.preloadedcode.back().pcode.arg3_uint = code.readu30();
				clearOperands(state,true,&lastlocalresulttype);
				break;
			}
			case 0x1c://pushwith
				removetypestack(typestack,1);
				scopelist.push_back(scope_entry(asAtomHandler::invalidAtom,true));
				state.preloadedcode.push_back((uint32_t)opcode);
				state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
				clearOperands(state,true,&lastlocalresulttype);
				break;
			case 0x1d://popscope
				if (!scopelist.empty())
					scopelist.pop_back();
				state.preloadedcode.push_back((uint32_t)opcode);
				state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
				clearOperands(state,true,&lastlocalresulttype);
				break;
			case 0x49://constructsuper
			{
				int32_t p = code.tellg();
				if (state.jumptargets.find(p) != state.jumptargets.end())
					clearOperands(state,true,&lastlocalresulttype);
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
				if (state.jumptargets.find(p) != state.jumptargets.end())
					clearOperands(state,true,&lastlocalresulttype);
				uint32_t t =code.readu30();
				removetypestack(typestack,mi->context->constant_pool.multinames[t].runtimeargs);
				ASObject* resulttype = nullptr;
				bool classvar=false;
#ifdef ENABLE_OPTIMIZATION
				uint32_t scopepos=UINT32_MAX;
				asAtom o=asAtomHandler::invalidAtom;
				bool found = false;
				bool done=false;
				multiname* name=mi->context->getMultiname(t,nullptr);
				bool isborrowed = false;
				variable* v = nullptr;
				Class_base* cls = function->inClass;
				if (name && name->isStatic && !function->isFromNewFunction())
				{
					if (function->inClass && (scopelist.begin()==scopelist.end() || !scopelist.back().considerDynamic)) // class method
					{
						if (!cls->hasoverriddenmethod(name))
						{
							do
							{
								v = cls->findVariableByMultiname(*name,cls,nullptr,&isborrowed,false,wrk);
								if (v)
									break;
								if (!cls->isSealed)
									break;
								cls = cls->super.getPtr();
							}
							while (cls);
						}
						if (v)
						{
							found =true;
							if (!function->isStatic && (isborrowed || v->kind == INSTANCE_TRAIT))
							{
								state.preloadedcode.push_back((uint32_t)0xd0); // convert to getlocal_0
								state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
								state.operandlist.push_back(operands(OP_LOCAL,function->inClass, 0,1,state.preloadedcode.size()-1));
								typestack.push_back(typestackentry(function->inClass,isborrowed));
								break;
							}
							if (function->isStatic && !isborrowed && (v->kind & DECLARED_TRAIT))
							{
								// if property is a static variable of the class this function belongs to we have to check the scopes first
								found = false;
							}
						}
					}
					if (!found && (!function->inClass || function->inClass->isSealed))
					{
						uint32_t spos = scopelist.size();
						auto it=scopelist.rbegin();
						while(it!=scopelist.rend())
						{
							spos--;
							if (it->considerDynamic)
							{
								found = true;
								break;
							}
							if (!asAtomHandler::isObject(it->object))
							{
								break;
							}
							ASObject* obj = asAtomHandler::getObjectNoCheck(it->object);
							if (obj->hasPropertyByMultiname(*name, false, true,wrk))
							{
								found = true;
								done=true;
								if (function->isStatic && obj==function->inClass)
								{
									// property is a static variable of the class this function belongs to
									o=asAtomHandler::fromObjectNoPrimitive(function->inClass);
									addCachedConstant(state,mi, o,code);
									resulttype = function->inClass;
									classvar=true;
									break;
								}
								else
									scopepos=spos;
								break;
							}
							++it;
						}
					}
					if(!found && !function->func_scope.isNull()) // check scope stack
					{
						uint32_t spos = function->func_scope->scope.size();
						auto it=function->func_scope->scope.rbegin();
						while(it!=function->func_scope->scope.rend())
						{
							spos--;
							if (it->considerDynamic)
							{
								found = true;
								break;
							}
							if (asAtomHandler::is<Class_base>(it->object) && !asAtomHandler::as<Class_base>(it->object)->isSealed)
							{
								break;
							}
							ASObject* obj = asAtomHandler::toObject(it->object,wrk);
							if (obj->hasPropertyByMultiname(*name, false, true,wrk))
							{
								found = true;
								done=true;
								state.preloadedcode.push_back((uint32_t)ABC_OP_OPTIMZED_GETFUNCSCOPEOBJECT);
								state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
								if (checkForLocalResult(state,code,0,obj->is<Class_base>() ? nullptr : obj->getClass()))
								{
									state.preloadedcode.at(state.preloadedcode.size()-1).pcode.func=abc_getfuncscopeobject_localresult;
									state.preloadedcode.at(state.preloadedcode.size()-1).pcode.arg2_uint=spos;
								}
								else
								{
									state.preloadedcode.at(state.preloadedcode.size()-1).pcode.arg2_uint=spos;
									clearOperands(state,true,&lastlocalresulttype);
								}
								break;
							}
							++it;
						}
					}
					if (!found && opcode == 0x5d //findpropstrict
							&& (scopelist.begin()==scopelist.end() || !scopelist.back().considerDynamic)) // class method
					{
						ASObject* target=nullptr;
						if (name->cachedType)
							target = name->cachedType->getGlobalScope();
						if (!target || (target->is<Global>() && target->as<Global>()->isAVM1()))
							mi->context->root->applicationDomain->findTargetByMultiname(*name, target,wrk);
						if (target)
						{
							o=asAtomHandler::fromObjectNoPrimitive(target);
							addCachedConstant(state,mi, o,code);
							typestack.push_back(typestackentry(target,false));
							break;
						}
					}
				}
				if (scopepos!= UINT32_MAX)
				{
					state.preloadedcode.push_back((uint32_t)0x65); //getscopeobject
					state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
					if (checkForLocalResult(state,code,0,nullptr))
					{
						state.preloadedcode.at(state.preloadedcode.size()-1).pcode.func=abc_getscopeobject_localresult;
						state.preloadedcode.at(state.preloadedcode.size()-1).pcode.arg2_uint=scopepos;
					}
					else
					{
						state.preloadedcode.at(state.preloadedcode.size()-1).pcode.arg2_uint=scopepos;
						clearOperands(state,true,&lastlocalresulttype);
					}
					done=true;
				}
				else if(!done)
				{
					if (v && !function->isFromNewFunction()
							&& (function != cls->getConstructor() || !isborrowed)
							&& (!function->isStatic || !isborrowed ))
					{
						asAtom value = asAtomHandler::fromObjectNoPrimitive(cls);
						addCachedConstant(state,mi, value,code);
						typestack.push_back(typestackentry(cls,isborrowed));
						break;
					}
				}
				if(!done)
#endif
				{
					state.preloadedcode.push_back((uint32_t)opcode);
					state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
					state.preloadedcode.back().pcode.local3.pos=t;
					clearOperands(state,true,&lastlocalresulttype);
				}
				typestack.push_back(typestackentry(resulttype,classvar));
				break;
			}
			case 0x60://getlex
			{
				Class_base* resulttype = nullptr;
				int32_t p = code.tellg();
				if (state.jumptargets.find(p) != state.jumptargets.end())
					clearOperands(state,true,&lastlocalresulttype);
				uint32_t t =code.readu30();
				multiname* name=mi->context->getMultiname(t,nullptr);
				if (!name || !name->isStatic)
				{
					createError<VerifyError>(wrk,kIllegalOpMultinameError,"getlex","multiname not static");
					break;
				}
#ifdef ENABLE_OPTIMIZATION
				if (function->inClass && (scopelist.begin()==scopelist.end() || !scopelist.back().considerDynamic)) // class method
				{
					if (function->isStatic)
					{
						variable* v = function->inClass->findVariableByMultiname(*name,nullptr,nullptr,nullptr,false,wrk);
						if (v)
						{
							if (v->kind == TRAIT_KIND::CONSTANT_TRAIT)
							{
								addCachedConstant(state,mi, v->var,code);
								if (v->isResolved && dynamic_cast<const Class_base*>(v->type))
									resulttype = (Class_base*)v->type;
								typestack.push_back(typestackentry(resulttype,false));
								break;
							}
							else if (v->kind==DECLARED_TRAIT)
							{
								// property is static variable from class
								resulttype = (Class_base*)(v->isResolved ? dynamic_cast<const Class_base*>(v->type):nullptr);
								state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
								asAtom clAtom = asAtomHandler::fromObjectNoPrimitive(function->inClass);
								addCachedConstant(state,state.mi,clAtom,code);
								if (v->slotid)
								{
									// convert to getslot on class
									setupInstructionOneArgument(state,ABC_OP_OPTIMZED_GETSLOT,opcode,code,true,false,resulttype,p,true,false,false,false,ABC_OP_OPTIMZED_GETSLOT_SETSLOT);
									state.preloadedcode.at(state.preloadedcode.size()-1).pcode.arg2_uint =v->slotid-1;
								}
								else
								{
									// convert to getprop on class
									setupInstructionOneArgument(state,ABC_OP_OPTIMZED_GETPROPERTY_STATICNAME,0x66,code,true, false,resulttype,p,true,false,false,false);
									state.preloadedcode.at(state.preloadedcode.size()-1).pcode.cachedmultiname2 = name;
								}
								typestack.push_back(typestackentry(resulttype,false));
								break;
							}
						}
					}
					else
					{
						bool isborrowed = false;
						variable* v = nullptr;
						Class_base* cls = function->inClass;
						do
						{
							v = cls->findVariableByMultiname(*name,cls,nullptr,&isborrowed,false,wrk);
							if (!v)
								cls = cls->super.getPtr();
						}
						while (!v && cls && cls->isSealed);
						if (v && cls->is<Class_inherit>() && cls->as<Class_inherit>()->hasoverriddenmethod(name))
							v=nullptr;
						if (v)
						{
							if ((isborrowed || v->kind == INSTANCE_TRAIT) && asAtomHandler::isValid(v->getter))
							{
								// property is getter from class
								resulttype = (Class_base*)(v->isResolved ? dynamic_cast<const Class_base*>(v->type):nullptr);
								state.preloadedcode.push_back((uint32_t)0xd0);
								state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
								state.operandlist.push_back(operands(OP_LOCAL,function->inClass, 0,1,state.preloadedcode.size()-1));
								if (function->inClass->isInterfaceMethod(*name) ||
									(function->inClass->is<Class_inherit>() && function->inClass->as<Class_inherit>()->hasoverriddenmethod(name)))
								{
									// convert to getprop on local[0]
									setupInstructionOneArgument(state,ABC_OP_OPTIMZED_GETPROPERTY_STATICNAME,0x66,code,true, false,resulttype,p,true,false,false,false);
									state.preloadedcode.at(state.preloadedcode.size()-1).pcode.cachedmultiname2 = name;
								}
								else
								{
									// convert to callprop on local[0] (this) with 0 args
									setupInstructionOneArgument(state,ABC_OP_OPTIMZED_CALLFUNCTION_NOARGS,0x46,code,true, false,resulttype,p,true,false,false,false,ABC_OP_OPTIMZED_CALLFUNCTION_NOARGS_SETSLOT);
									state.preloadedcode.at(state.preloadedcode.size()-1).pcode.cacheobj2 = asAtomHandler::getObject(v->getter);
								}
								typestack.push_back(typestackentry(resulttype,false));
								break;
							}
							else if ((isborrowed || v->kind == INSTANCE_TRAIT) && asAtomHandler::isValid(v->var))
							{
								// property is variable from class
								resulttype = (Class_base*)(v->isResolved ? dynamic_cast<const Class_base*>(v->type):nullptr);
								state.preloadedcode.push_back((uint32_t)0xd0);
								state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
								state.operandlist.push_back(operands(OP_LOCAL,function->inClass, 0,1,state.preloadedcode.size()-1));
								if (function->inClass->is<Class_inherit>()
									&& !function->inClass->as<Class_inherit>()->hasoverriddenmethod(name)
									&& v->slotid)
								{
									asAtom o = asAtomHandler::invalidAtom;
									cls->getInstance(wrk,o,false,nullptr,0);
									cls->setupDeclaredTraits(asAtomHandler::getObject(o),false);
									variable* v1 = asAtomHandler::getObject(o)->findVariableByMultiname(*name,nullptr,nullptr,nullptr,false,wrk);
									if (!asAtomHandler::isPrimitive(o) && v1 && v1->slotid)
									{
										// convert to getslot on local[0]
										setupInstructionOneArgument(state,ABC_OP_OPTIMZED_GETSLOT,opcode,code,true,false,resulttype,p,true,false,false,false,ABC_OP_OPTIMZED_GETSLOT_SETSLOT);
										state.preloadedcode.at(state.preloadedcode.size()-1).pcode.arg2_uint =v1->slotid-1;
									}
									else
									{
										// convert to getprop on local[0]
										setupInstructionOneArgument(state,ABC_OP_OPTIMZED_GETPROPERTY_STATICNAME,0x66,code,true, false,resulttype,p,true,false,false,false);
										state.preloadedcode.at(state.preloadedcode.size()-1).pcode.cachedmultiname2 = name;
									}
									ASATOM_DECREF(o);
								}
								else
								{
									// convert to getprop on local[0]
									setupInstructionOneArgument(state,ABC_OP_OPTIMZED_GETPROPERTY_STATICNAME,0x66,code,true, false,resulttype,p,true,false,false,false);
									state.preloadedcode.at(state.preloadedcode.size()-1).pcode.cachedmultiname2 = name;
								}
								typestack.push_back(typestackentry(resulttype,false));
								break;
							}
							else if (!isborrowed && v->kind==DECLARED_TRAIT)
							{
								// property is static variable from class
								resulttype = (Class_base*)(v->isResolved ? dynamic_cast<const Class_base*>(v->type):nullptr);
								state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
								asAtom clAtom = asAtomHandler::fromObjectNoPrimitive(cls);
								addCachedConstant(state,state.mi,clAtom,code);
								if (v->slotid)
								{
									// convert to getslot on class
									setupInstructionOneArgument(state,ABC_OP_OPTIMZED_GETSLOT,opcode,code,true,false,resulttype,p,true,false,false,false,ABC_OP_OPTIMZED_GETSLOT_SETSLOT);
									state.preloadedcode.at(state.preloadedcode.size()-1).pcode.arg2_uint =v->slotid-1;
								}
								else
								{
									// convert to getprop on class
									setupInstructionOneArgument(state,ABC_OP_OPTIMZED_GETPROPERTY_STATICNAME,0x66,code,true, false,resulttype,p,true,false,false,false);
									state.preloadedcode.at(state.preloadedcode.size()-1).pcode.cachedmultiname2 = name;
								}
								typestack.push_back(typestackentry(resulttype,false));
								break;
							}
							else if (v->kind == TRAIT_KIND::CONSTANT_TRAIT && !asAtomHandler::isNull(v->var)) // class may not be constructed yet, so the result is null and we do not cache
							{
								addCachedConstant(state,mi, v->var,code);
								if (v->isResolved && dynamic_cast<const Class_base*>(v->type))
									resulttype = (Class_base*)v->type;
								typestack.push_back(typestackentry(resulttype,false));
								break;
							}
						}
					}
					if ((simple_getter_opcode_pos != UINT32_MAX) // function is simple getter
							&& function->inClass->isFinal // TODO also enable optimization for classes where it is guarranteed that the method is not overridden in derived classes
							&& function->inClass->getInterfaces().empty()) // class doesn't implement any interfaces
					{
						variable* v = function->inClass->findVariableByMultiname(*name,nullptr,nullptr,nullptr,false,wrk);
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
							if (asAtomHandler::is<Class_inherit>(it->object))
								asAtomHandler::as<Class_inherit>(it->object)->checkScriptInit();
							r = asAtomHandler::toObject(it->object,wrk)->getVariableByMultiname(o,*name, opt,wrk);
							if(asAtomHandler::isValid(o))
								break;
							++it;
						}
					}
					if(asAtomHandler::isInvalid(o))
					{
						GET_VARIABLE_OPTION opt= (GET_VARIABLE_OPTION)(FROM_GETLEX | DONT_CALL_GETTER | NO_INCREF);
						r = mi->context->root->applicationDomain->getVariableByMultiname(o,*name,opt,wrk);
					}
					if(asAtomHandler::isInvalid(o))
					{
						ASObject* cls = (ASObject*)dynamic_cast<const Class_base*>(name->cachedType);
						if (cls)
							o = asAtomHandler::fromObjectNoPrimitive(cls);
					}
					// fast check for builtin classes if no custom class with same name is defined
					if(asAtomHandler::isInvalid(o) && mi->context->root->customClasses.find(name->name_s_id) == mi->context->root->customClasses.end())
					{
						ASObject* cls = mi->context->root->getSystemState()->systemDomain->getVariableByMultinameOpportunistic(*name,wrk);
						if (cls)
							o = asAtomHandler::fromObject(cls);
						if (cls && !cls->is<Class_base>() && cls->getConstant())
						{
							// global builtin method/constant
							addCachedConstant(state,mi, o,code);
							typestack.push_back(typestackentry(cls->getClass(),false));
							break;
						}
					}
					if(asAtomHandler::isInvalid(o))
					{
						ASObject* cls = mi->context->root->applicationDomain->getVariableByMultinameOpportunistic(*name,wrk);
						if (cls)
							o = asAtomHandler::fromObject(cls);
					}
					if (asAtomHandler::is<Template_base>(o))
					{
						addCachedConstant(state,mi, o,code);
						typestack.push_back(typestackentry(nullptr,false));
						break;
					}
					if (asAtomHandler::is<Class_base>(o))
					{
						resulttype = asAtomHandler::as<Class_base>(o);
						if (asAtomHandler::as<Class_base>(o)->isConstructed() || asAtomHandler::as<Class_base>(o)->isBuiltin())
						{
							addCachedConstant(state,mi, o,code);
							typestack.push_back(typestackentry(resulttype,true));
							break;
						}
					}
					else if (r & GETVAR_ISCONSTANT && !asAtomHandler::isNull(o)) // class may not be constructed yet, so the result is null and we do not cache
					{
						addCachedConstant(state,mi, o,code);
						typestack.push_back(typestackentry(nullptr,false));
						break;
					}
					else if (function->inClass->super.isNull()) //TODO slot access for derived classes
					{
						uint32_t slotid = function->inClass->findInstanceSlotByMultiname(name);
						if (slotid != UINT32_MAX)
						{
							state.preloadedcode.push_back(ABC_OP_OPTIMZED_GETLEX_FROMSLOT);
							state.preloadedcode.back().pcode.arg1_uint=slotid;
							state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
							checkForLocalResult(state,code,1,nullptr);
							typestack.push_back(typestackentry(nullptr,false));
							if (r & GETVAR_ISNEWOBJECT)
								ASATOM_DECREF(o);
							break;
						}
					}
				}
				else if (!function->fromNewFunction && !function->isStatic && (scopelist.begin()==scopelist.end() || !scopelist.back().considerDynamic))
				{
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
							if (asAtomHandler::is<Class_inherit>(it->object))
								asAtomHandler::as<Class_inherit>(it->object)->checkScriptInit();
							r=asAtomHandler::toObject(it->object,wrk)->getVariableByMultiname(o,*name, opt,wrk);
							if(asAtomHandler::isValid(o))
								break;
							++it;
						}
					}
					if(asAtomHandler::isInvalid(o) || !asAtomHandler::isNull(o))// class may not be constructed yet, so the result is null and we do not cache
					{
						const Type* tp = Type::getTypeFromMultiname(name,mi->context);
						if (dynamic_cast<const Class_base*>(tp))
						{
							resulttype = (Class_base*)dynamic_cast<const Class_base*>(tp);
							if (resulttype->is<Class_inherit>())
								resulttype->as<Class_inherit>()->checkScriptInit();
							if (resulttype->isConstructed() || resulttype->isBuiltin())
								o = asAtomHandler::fromObjectNoPrimitive(resulttype);
						}
					}
					if (asAtomHandler::isValid(o) && !asAtomHandler::isNull(o))// class may not be constructed yet, so the result is null and we do not cache
					{
						addCachedConstant(state,mi, o,code);
						typestack.push_back(typestackentry(resulttype,true));
						break;
					}
					if (r & GETVAR_ISNEWOBJECT)
						ASATOM_DECREF(o);
				}
#endif
				state.preloadedcode.push_back(ABC_OP_OPTIMZED_GETLEX);
				state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
				state.preloadedcode[state.preloadedcode.size()-1].pcode.cachedmultiname2=name;
				if (!checkForLocalResult(state,code,0,resulttype))
				{
					// no local result possible, use standard operation
					state.preloadedcode[state.preloadedcode.size()-1].pcode.func = abc_getlex;
					clearOperands(state,true,&lastlocalresulttype);
				}
				typestack.push_back(typestackentry(resulttype,true));
				break;
			}
			case 0x61://setproperty
			case 0x68://initproperty
			{
				int32_t p = code.tellg();
				state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size()+1;
				uint32_t t = code.readu30();
				assert_and_throw(t < mi->context->constant_pool.multiname_count);
#ifdef ENABLE_OPTIMIZATION
				if (state.jumptargets.find(p) == state.jumptargets.end())
				{
					switch (mi->context->constant_pool.multinames[t].runtimeargs)
					{
						case 0:
						{
							multiname* name =  mi->context->getMultinameImpl(asAtomHandler::nullAtom,nullptr,t,false);
							if (state.operandlist.size() > 1)
							{
								auto it = state.operandlist.rbegin();
								Class_base* contenttype = it->objtype;
								it++;
								if (canCallFunctionDirect((*it),name) && !typestack[typestack.size()-2].classvar)
								{
									variable* v = it->objtype->getBorrowedVariableByMultiname(*name);
									if (v)
									{
										if (asAtomHandler::is<SyntheticFunction>(v->setter))
										{
											setupInstructionTwoArgumentsNoResult(state,ABC_OP_OPTIMZED_CALLFUNCTIONSYNTHETIC_ONEARG_VOID,opcode,code);
											if (contenttype)
											{
												SyntheticFunction* f = asAtomHandler::as<SyntheticFunction>(v->setter);
												if (!f->getMethodInfo()->returnType)
													f->checkParamTypes();
												if (f->getMethodInfo()->paramTypes.size() && f->canSkipCoercion(0,contenttype))
													state.preloadedcode.at(state.preloadedcode.size()-1).pcode.arg3_uint = ABC_OP_COERCED;
											}
											state.preloadedcode.push_back(0);
											state.preloadedcode.back().pcode.cacheobj3 = asAtomHandler::getObject(v->setter);
											removetypestack(typestack,mi->context->constant_pool.multinames[t].runtimeargs+2);
											break;
										}
										if (asAtomHandler::is<Function>(v->setter))
										{
											setupInstructionTwoArgumentsNoResult(state,ABC_OP_OPTIMZED_CALLFUNCTIONBUILTIN_ONEARG_VOID,opcode,code);
											state.preloadedcode.at(state.preloadedcode.size()-1).pcode.cacheobj3 = asAtomHandler::getObject(v->setter);
											removetypestack(typestack,mi->context->constant_pool.multinames[t].runtimeargs+2);
											break;
										}
									}
								}
								if ((it->type == OP_LOCAL || it->type == OP_CACHED_CONSTANT || it->type == OP_CACHED_SLOT) 
										&& it->objtype && !it->objtype->isInterface && it->objtype->isInitialized() && it->objtype->isSealed
										&& (!typestack[typestack.size()-2].obj || !typestack[typestack.size()-2].classvar))
								{
									asAtom o = asAtomHandler::invalidAtom;
									if (it->objtype->is<Class_inherit>())
										it->objtype->as<Class_inherit>()->checkScriptInit();
									// check if we can replace setProperty by setSlot
									it->objtype->getInstance(wrk,o,false,nullptr,0);
									it->objtype->setupDeclaredTraits(asAtomHandler::getObject(o),false);
									variable* v = asAtomHandler::getObject(o)->findVariableByMultiname(*name,nullptr,nullptr,nullptr,false,wrk);
									if (!v && it->objtype->is<Class_inherit>())
									{
										v = it->objtype->findVariableByMultiname(*name,nullptr,nullptr,nullptr,false,wrk);
										if (v && v->kind != DECLARED_TRAIT)
											v=nullptr;
									}
									if (!asAtomHandler::isPrimitive(o) && v && v->slotid)
									{
										// we can skip coercing when setting the slot value if
										// - contenttype is the same as the variable type or
										// - variable type is any or void or
										// - contenttype is subclass of variable type or
										// - contenttype is numeric and variable type is Number
										// - contenttype is Number and variable type is Integer and previous opcode was arithmetic with localresult
										int operator_start = v->isResolved && ((contenttype && contenttype == v->type) || !dynamic_cast<const Class_base*>(v->type)) ? ABC_OP_OPTIMZED_SETSLOT_NOCOERCE : ABC_OP_OPTIMZED_SETSLOT;
										if (contenttype && v->isResolved && dynamic_cast<const Class_base*>(v->type))
										{
											Class_base* vtype = (Class_base*)v->type;
											if (contenttype->isSubClass(vtype)
												|| (( contenttype == Class<Number>::getRef(function->getSystemState()).getPtr() ||
													   contenttype == Class<Integer>::getRef(function->getSystemState()).getPtr() ||
													   contenttype == Class<UInteger>::getRef(function->getSystemState()).getPtr()) &&
													 ( vtype == Class<Number>::getRef(function->getSystemState()).getPtr()))
												)
											{
												operator_start = ABC_OP_OPTIMZED_SETSLOT_NOCOERCE;
											}
											else if (checkmatchingLastObjtype(state,contenttype,vtype))
											{
												operator_start = ABC_OP_OPTIMZED_SETSLOT_NOCOERCE;
											}
										}
										bool getslotisvalue = state.preloadedcode.size() && state.preloadedcode.at(state.preloadedcode.size()-1).operator_start==ABC_OP_OPTIMZED_GETSLOT;
										setupInstructionTwoArgumentsNoResult(state,operator_start,opcode,code);
										if (getslotisvalue && state.preloadedcode.size() > 1 && v->slotid < ABC_OP_BITMASK_USED
											&& state.preloadedcode.at(state.preloadedcode.size()-1).pcode.func == abc_setslotNoCoerce_local_local
											&& state.preloadedcode.at(state.preloadedcode.size()-1).pcode.local_pos1 <0xffff // only optimize if local_pos1 fits in uint16_t
											&& state.preloadedcode.at(state.preloadedcode.size()-2).operator_setslot != UINT32_MAX)
										{
											// this optimized setslot can be combined with previous opcode

											// move local_pos1 to local_pos3
											state.preloadedcode.at(state.preloadedcode.size()-1).pcode.local3.pos = state.preloadedcode.at(state.preloadedcode.size()-1).pcode.local_pos1;
											state.preloadedcode.at(state.preloadedcode.size()-1).cachedslot3 = state.preloadedcode.at(state.preloadedcode.size()-1).cachedslot1;
											// move local1 of previous opcode to local1 of current opcode
											state.preloadedcode.at(state.preloadedcode.size()-1).pcode.cacheobj1 = state.preloadedcode.at(state.preloadedcode.size()-2).pcode.cacheobj1;
											state.preloadedcode.at(state.preloadedcode.size()-1).cachedslot1 = state.preloadedcode.at(state.preloadedcode.size()-2).cachedslot1;
											// move local2 of previous opcode to local2 of current opcode
											state.preloadedcode.at(state.preloadedcode.size()-1).pcode.cacheobj2 = state.preloadedcode.at(state.preloadedcode.size()-2).pcode.cacheobj2;
											state.preloadedcode.at(state.preloadedcode.size()-1).cachedslot2 = state.preloadedcode.at(state.preloadedcode.size()-2).cachedslot2;
											// move flags of previous opcode to flags of current opcode
											state.preloadedcode.at(state.preloadedcode.size()-1).pcode.local3.flags = state.preloadedcode.at(state.preloadedcode.size()-2).pcode.local3.flags;
											// set current opcode to optimized setslot opcode
											state.preloadedcode.at(state.preloadedcode.size()-1).pcode.func = ABCVm::abcfunctions[state.preloadedcode.at(state.preloadedcode.size()-2).operator_setslot];
											// remove previous opcode
											state.preloadedcode.erase(state.preloadedcode.begin()+(state.preloadedcode.size()-2));
											// set slotid into local3.flags of current opcode
											state.preloadedcode.at(state.preloadedcode.size()-1).pcode.local3.flags |=v->slotid-1;

											state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
											if (!state.operandlist.empty())
												state.operandlist.pop_back();
										}
										else
											state.preloadedcode.at(state.preloadedcode.size()-1).pcode.arg3_uint =v->slotid-1;
										ASATOM_DECREF(o);
										removetypestack(typestack,mi->context->constant_pool.multinames[t].runtimeargs+2);
										break;
									}
									else
										ASATOM_DECREF(o);
								}
							}
							if (setupInstructionTwoArgumentsNoResult(state,ABC_OP_OPTIMZED_SETPROPERTY_STATICNAME,opcode,code))
								state.preloadedcode.push_back(0);
							else
							{
								state.preloadedcode.at(state.preloadedcode.size()-1).pcode.func = abc_setPropertyStaticName;
								clearOperands(state,false,&lastlocalresulttype);
							}
							state.preloadedcode.at(state.preloadedcode.size()-1).pcode.cachedmultiname2 =name;
							state.preloadedcode.at(state.preloadedcode.size()-1).pcode.local3.pos = opcode; // use local3.pos as indicator for setproperty/initproperty
							if ((simple_setter_opcode_pos != UINT32_MAX) // function is simple setter
									&& function->inClass->isFinal // TODO also enable optimization for classes where it is guarranteed that the method is not overridden in derived classes
									&& function->inClass->getInterfaces().empty()) // class doesn't implement any interfaces
							{
								variable* v = function->inClass->findVariableByMultiname(*name,nullptr,nullptr,nullptr,false,wrk);
								if (v && v->kind == TRAIT_KIND::INSTANCE_TRAIT)
									function->simpleGetterOrSetterName = name;
							}
							removetypestack(typestack,mi->context->constant_pool.multinames[t].runtimeargs+2);
							break;
						}
						case 1:
							if (state.operandlist.size() > 2 && (state.operandlist[state.operandlist.size()-2].objtype == Class<Integer>::getRef(function->getSystemState()).getPtr()))
							{
								uint32_t startopcode = ABC_OP_OPTIMZED_SETPROPERTY_INTEGER;
								if (state.operandlist[state.operandlist.size()-3].objtype
									&& dynamic_cast<TemplatedClass<Vector>*>(state.operandlist[state.operandlist.size()-3].objtype))
								{
									TemplatedClass<Vector>* cls=state.operandlist[state.operandlist.size()-3].objtype->as<TemplatedClass<Vector>>();
									Class_base* vectype = cls->getTypes().size() > 0 ? (Class_base*)cls->getTypes()[0] : nullptr;
									if (checkmatchingLastObjtype(state,state.operandlist[state.operandlist.size()-1].objtype,vectype))
									{
										// use special fast setproperty without coercing for Vector
										startopcode = ABC_OP_OPTIMZED_SETPROPERTY_INTEGER_VECTOR;
									}
								}
								if (state.operandlist[state.operandlist.size()-3].type == OP_LOCAL || state.operandlist[state.operandlist.size()-3].type == OP_CACHED_SLOT)
								{
									startopcode+= 4;
									int index = state.operandlist[state.operandlist.size()-3].index;
									bool cachedslot = state.operandlist[state.operandlist.size()-3].type == OP_CACHED_SLOT;
									setupInstructionTwoArgumentsNoResult(state,startopcode,opcode,code);
									state.preloadedcode.at(state.preloadedcode.size()-1).pcode.local3.pos=index;
									state.preloadedcode.at(state.preloadedcode.size()-1).cachedslot3 = cachedslot;
									state.preloadedcode.push_back(0);
									state.preloadedcode.at(state.preloadedcode.size()-1).pcode.local3.pos = opcode; // use local3.pos as indicator for setproperty/initproperty
									state.operandlist.back().removeArg(state);
									setOperandModified(state,cachedslot ? OP_CACHED_SLOT : OP_LOCAL,index);
									clearOperands(state,false,&lastlocalresulttype);
								}
								else
								{
									asAtom* arg = mi->context->getConstantAtom(state.operandlist[state.operandlist.size()-3].type,state.operandlist[state.operandlist.size()-3].index);
									setupInstructionTwoArgumentsNoResult(state,startopcode,opcode,code);
									state.preloadedcode.at(state.preloadedcode.size()-1).pcode.arg3_constant=arg;
									state.preloadedcode.push_back(0);
									state.preloadedcode.at(state.preloadedcode.size()-1).pcode.local3.pos = opcode; // use local3.pos as indicator for setproperty/initproperty
									state.operandlist.back().removeArg(state);
									clearOperands(state,false,&lastlocalresulttype);
								}
							}
							else
							{
								if (typestack.size() > 2 && typestack[typestack.size()-2].obj == Class<Integer>::getRef(function->getSystemState()).getPtr())
								{
									state.preloadedcode.push_back((uint32_t)ABC_OP_OPTIMZED_SETPROPERTY_INTEGER_SIMPLE);
									state.preloadedcode.at(state.preloadedcode.size()-1).pcode.local3.flags = opcode; // use local3.flags as indicator for setproperty/initproperty
								}
								else
								{
									state.preloadedcode.push_back((uint32_t)opcode);
									state.preloadedcode.back().pcode.local3.pos=t;
								}
								clearOperands(state,false,&lastlocalresulttype);
							}
							removetypestack(typestack,mi->context->constant_pool.multinames[t].runtimeargs+2);
							break;
						default:
							state.preloadedcode.push_back((uint32_t)opcode);
							state.preloadedcode.back().pcode.local3.pos=t;
							clearOperands(state,false,&lastlocalresulttype);
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
							state.preloadedcode.push_back((uint32_t)ABC_OP_OPTIMZED_SETPROPERTY_STATICNAME_SIMPLE);
							state.preloadedcode.at(state.preloadedcode.size()-1).pcode.cachedmultiname2 =name;
							state.preloadedcode.at(state.preloadedcode.size()-1).pcode.local3.pos = opcode; // use local3.pos as indicator for setproperty/initproperty
							if ((simple_setter_opcode_pos != UINT32_MAX) // function is simple setter
									&& function->inClass->isFinal // TODO also enable optimization for classes where it is guarranteed that the method is not overridden in derived classes
									&& function->inClass->getInterfaces().empty()) // class doesn't implement any interfaces
							{
								variable* v = function->inClass->findVariableByMultiname(*name,nullptr,nullptr,nullptr,false,wrk);
								if (v && v->kind == TRAIT_KIND::INSTANCE_TRAIT)
									function->simpleGetterOrSetterName = name;
							}
							removetypestack(typestack,mi->context->constant_pool.multinames[t].runtimeargs+1);
							break;
						}
						default:
							state.preloadedcode.push_back((uint32_t)opcode);
							state.preloadedcode.back().pcode.local3.pos=t;
							clearOperands(state,false,&lastlocalresulttype);
							removetypestack(typestack,mi->context->constant_pool.multinames[t].runtimeargs+1);
							break;
					}
				}
#else
				state.preloadedcode.push_back((uint32_t)opcode);
				state.preloadedcode.back().pcode.local3.pos=t;
				clearOperands(state,false,&lastlocalresulttype);
				removetypestack(typestack,mi->context->constant_pool.multinames[t].runtimeargs+1);
#endif
				break;
			}
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
				state.preloadedcode.push_back(opcode);
				state.preloadedcode.back().pcode.arg3_uint=value;
				if (state.jumptargets.find(p) != state.jumptargets.end())
					clearOperands(state,true,&lastlocalresulttype);
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
				state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
				uint32_t t =code.readu30();
#ifdef ENABLE_OPTIMIZATION
				assert_and_throw(t < scopelist.size());
				auto it = scopelist.begin();
				for (uint32_t i=0; i < t; i++)
					++it;
				asAtom a = (it)->object;
				Class_base* resulttype = nullptr;
				if (asAtomHandler::is<Activation_object>(a))
					resulttype = Class<ASObject>::getRef(function->getSystemState()).getPtr();
				typestack.push_back(typestackentry(resulttype,false));
				if (checkForLocalResult(state,code,0,resulttype))
				{
					if (asAtomHandler::is<Activation_object>(a))
						state.operandlist.at(state.operandlist.size()-1).instance = asAtomHandler::as<Activation_object>(a);
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
				if (state.jumptargets.find(p) != state.jumptargets.end())
					clearOperands(state,true,&lastlocalresulttype);
				int32_t t =code.readu30();
				assert_and_throw(t);
				Class_base* resulttype = nullptr;
#ifdef ENABLE_OPTIMIZATION
				if (state.operandlist.size() > 0)
				{
					auto it = state.operandlist.rbegin();
					bool isactivationobj = it->instance && it->instance->is<Activation_object>();
					if (it->type == OP_LOCAL)
					{
						if (state.unchangedlocals.find(it->index) != state.unchangedlocals.end())
						{
							it->removeArg(state);
							state.operandlist.pop_back();
							if (isactivationobj)
								resulttype = it->instance->getSlotType(t);
							else if (it->objtype && !it->objtype->isInterface && it->objtype->isInitialized())
							{
								if (it->objtype->is<Class_inherit>())
									it->objtype->as<Class_inherit>()->checkScriptInit();
								// check if we can replace getProperty by getSlot
								asAtom o = asAtomHandler::invalidAtom;
								it->objtype->getInstance(wrk,o,false,nullptr,0);
								it->objtype->setupDeclaredTraits(asAtomHandler::getObject(o));
								ASObject* obj =asAtomHandler::getObject(o);
								if (obj)
								{
									resulttype = obj->getSlotType(t);
									obj->decRef();
								}
							}
							addCachedSlot(state,it->index,t,code,resulttype);
							typestack.push_back(typestackentry(resulttype,false));
							break;
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
							asAtom cval = v->var;
							if (!asAtomHandler::isNull(cval) && !asAtomHandler::isUndefined(cval)
									&& (v->kind == TRAIT_KIND::CONSTANT_TRAIT
										|| asAtomHandler::is<Class_base>(cval)
										))
							{
								it->removeArg(state);
								state.operandlist.pop_back();
								addCachedConstant(state,mi, cval,code);
								resulttype = asAtomHandler::getObject(*o)->getSlotType(t);
								typestack.push_back(typestackentry(resulttype,false));
								break;
							}
						}
					}
					if (isactivationobj)
					{
						resulttype = it->instance->getSlotType(t);
					}
					else if (it->objtype && !it->objtype->isInterface && it->objtype->isInitialized())
					{
						if (it->objtype->is<Class_inherit>())
							it->objtype->as<Class_inherit>()->checkScriptInit();
						// check if we can replace getProperty by getSlot
						asAtom o = asAtomHandler::invalidAtom;
						it->objtype->getInstance(wrk,o,false,nullptr,0);
						it->objtype->setupDeclaredTraits(asAtomHandler::getObject(o));
						ASObject* obj =asAtomHandler::getObject(o);
						if (obj)
						{
							resulttype = obj->getSlotType(t);
							obj->decRef();
						}
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
				if (state.jumptargets.find(p) != state.jumptargets.end())
					clearOperands(state,true,&lastlocalresulttype);
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
						else if (tobj != Class<ASString>::getRef(mi->context->root->getSystemState()).getPtr())
							skip = tobj==cls;
					}
					if (skip || cls != nullptr)
						tobj = (Class_base*)cls;
				}
				if (!skip && state.operandlist.size()>0 && state.operandlist.back().type==OP_NULL  // coerce following a pushnull can be skipped if not coercing to a numeric value
						&& tobj != Class<Number>::getRef(mi->context->root->getSystemState()).getPtr()
						&& tobj != Class<Integer>::getRef(mi->context->root->getSystemState()).getPtr()
						&& tobj != Class<UInteger>::getRef(mi->context->root->getSystemState()).getPtr()
						&& tobj != Class<Boolean>::getRef(mi->context->root->getSystemState()).getPtr())
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
						state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
						code.readbyte();
						state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
						mi->needsscope=true;
						scopelist.push_back(scope_entry(function->inClass ? asAtomHandler::fromObjectNoPrimitive(function->inClass) : asAtomHandler::invalidAtom,false));
						break;
					}
				}
				if (((uint32_t)opcode)-0xd0 >= mi->body->local_count)
				{
					// this may happen in unreachable obfuscated code, so we just ignore the opcode
					LOG(LOG_ERROR,"preload getlocal with argument > local_count:"<<mi->body->local_count<<" "<<hex<<(int)opcode);
					state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
					if (state.jumptargets.find(p) != state.jumptargets.end())
						clearOperands(state,true,&lastlocalresulttype);
				}
#endif
				state.preloadedcode.push_back((uint32_t)opcode);
				state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
				if (state.jumptargets.find(p) != state.jumptargets.end())
					clearOperands(state,true,&lastlocalresulttype);
				state.operandlist.push_back(operands(OP_LOCAL,state.localtypes[((uint32_t)opcode)-0xd0],((uint32_t)opcode)-0xd0,1,state.preloadedcode.size()-1));
				typestack.push_back(typestackentry(state.localtypes[((uint32_t)opcode)-0xd0],false));
				break;
			}
			case 0x2a://dup
			{
				int32_t p = code.tellg();
#ifdef ENABLE_OPTIMIZATION
				if (state.jumptargets.find(p) != state.jumptargets.end())
				{
					state.preloadedcode.push_back((uint32_t)opcode);
					state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
					clearOperands(state,true,&lastlocalresulttype);
					typestack.push_back(typestack.back());
					break;
				}
				bool dupoperand = !state.dupoperands.empty();
				if (dupoperand)
				{
					state.operandlist.push_back(state.dupoperands.front());
					state.dupoperands.clear();
				}
				if (state.operandlist.size() > 0)
				{
					if (state.operandlist.back().type == OP_LOCAL || state.operandlist.back().type == OP_CACHED_SLOT)
					{
						Class_base* restype = nullptr;
						if (typestack.back().obj && typestack.back().obj->is<Class_base>())
							restype=typestack.back().obj->as<Class_base>();
						operands op = state.operandlist.back();
						// if this dup is followed by increment/decrement and setlocal, it can be skipped and converted into a single increment/decrement call
						// if this dup is followed by iftrue/iffalse and pop, it can be skipped and converted into a single iftrue/iffalse call
						bool handled = false;
						uint8_t b = code.peekbyte();
						uint32_t opcode_optimized=0;
						uint32_t pos =p+1;
						uint32_t num=0;
						int32_t jump=0;
						bool is_iftruefalse=false;
						switch (b)
						{
							case 0x91://increment
								opcode_optimized=ABC_OP_OPTIMZED_DUP_INCDEC;
								break;
							case 0x93://decrement
								opcode_optimized=ABC_OP_OPTIMZED_DUP_INCDEC+1;
								break;
							case 0xc0://increment_i
								opcode_optimized=ABC_OP_OPTIMZED_DUP_INCDEC+2;
								break;
							case 0xc1://decrement_i
								opcode_optimized=ABC_OP_OPTIMZED_DUP_INCDEC+3;
								break;
							case 0x11://iftrue
								if (state.jumptargets.find(pos) != state.jumptargets.end())
									break;
								is_iftruefalse=true;
								jump = code.peeks24FromPosition(pos);
								pos +=3;
								opcode_optimized=ABC_OP_OPTIMZED_IFTRUE_DUP+1;
								break;
							case 0x12://iffalse
								if (state.jumptargets.find(pos) != state.jumptargets.end())
									break;
								jump = code.peeks24FromPosition(pos);
								pos +=3;
								is_iftruefalse=true;
								opcode_optimized=ABC_OP_OPTIMZED_IFFALSE_DUP+1;
								break;
							default:
								break;
						}
						if (opcode_optimized)
						{
							b = code.peekbyteFromPosition(pos);
							switch (b)
							{
								case 0x63://setlocal
								{
									num = code.peeku30FromPosition(pos+1);
									pos = code.skipu30FromPosition(pos+1);
									handled = true;
									is_iftruefalse=false;
									break;
								}
								case 0xd4: //setlocal_0
								case 0xd5: //setlocal_1
								case 0xd6: //setlocal_2
								case 0xd7: //setlocal_3
									num = b-0xd4;
									pos++;
									handled = true;
									is_iftruefalse=false;
									break;
								case 0x29: //pop
									if (is_iftruefalse)
									{
										if (state.jumptargets.find(pos) != state.jumptargets.end())
										{
											is_iftruefalse=false;
											break;
										}
										pos++;
										handled = true;
									}
									break;
								default:
									is_iftruefalse=false;
									break;
							}
						}
						if (is_iftruefalse)
						{
							// remove used operand
							auto it = state.operandlist.end();
							(--it)->removeArg(state);
							state.operandlist.pop_back();
							state.canlocalinitialize.clear();
							state.preloadedcode.push_back(opcode_optimized);
							state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
							state.preloadedcode.back().pcode.local_pos1 = op.index;
							state.preloadedcode.back().cachedslot1 = op.type == OP_CACHED_SLOT;
							jumppositions[state.preloadedcode.size()-1] = jump;
							jumpstartpositions[state.preloadedcode.size()-1] = p+3+1;
							state.oldnewpositions[p+3+1] = (int32_t)state.preloadedcode.size();
							// skip pop
							code.seekg(pos);
							clearOperands(state,true,&lastlocalresulttype);
							break;
						}
						else if (handled)
						{
							op.removeArg(state);
							state.preloadedcode.push_back(opcode_optimized);
							state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
							state.preloadedcode.back().pcode.local_pos1 = op.index;
							state.preloadedcode.back().cachedslot1 = op.type == OP_CACHED_SLOT;
							state.preloadedcode.back().pcode.local_pos2 = num;
							int localresultused=0;
							for (auto it = state.operandlist.begin(); it != state.operandlist.end(); it++)
							{
								if (it->type != OP_LOCAL)
									continue;
								if (uint32_t(it->index) > state.mi->body->getReturnValuePos()) // local result index already used
								{
									localresultused++;
								}
							}
							if (localresultused >= state.mi->body->localresultcount)
							{
								state.mi->body->localresultcount++;
								state.localtypes.push_back(localresultused == state.mi->body->localresultcount ? restype : nullptr);
								state.defaultlocaltypes.push_back(restype);
								state.defaultlocaltypescacheable.push_back(true);
							}
							uint32_t num2 = state.mi->body->getReturnValuePos()+1+localresultused;
							state.preloadedcode.back().pcode.local3.pos = num2;
							code.seekg(pos);
							removeOperands(state,false,nullptr,1);
							state.preloadedcode.push_back(0);
							state.preloadedcode.back().pcode.func = abc_getlocal;
							state.preloadedcode.back().pcode.arg3_uint = num2;
							state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
							state.operandlist.push_back(operands(OP_LOCAL,restype,num2,1,state.preloadedcode.size()-1));
							state.operandlist.back().duparg1=true;
							break;
						}
						// this ensures that the "old" value is stored in a localresult and can be used later, as the duplicated value may be changed by an increment etc.
						setupInstructionOneArgument(state,ABC_OP_OPTIMZED_DUP,opcode,code,false,true,restype,code.tellg(),opcode_optimized==0,false,true,true,ABC_OP_OPTIMZED_DUP_SETSLOT);
						if (!dupoperand)
						{
							if (op.type == OP_CACHED_SLOT)
							{
								state.preloadedcode.push_back(ABC_OP_OPTIMZED_PUSHCACHEDSLOT);
								state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
								state.preloadedcode.back().pcode.arg3_uint = op.index;
								state.operandlist.push_back(operands(op.type,op.objtype,op.index,1,state.preloadedcode.size()-1));
							}
							else
							{
								state.preloadedcode.push_back(0);
								state.preloadedcode.back().pcode.func = abc_getlocal;
								state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
								state.preloadedcode.back().pcode.arg3_uint = op.index;
								state.operandlist.push_back(operands(op.type,op.objtype,op.index,1,state.preloadedcode.size()-1));
							}
						}
					}
					else
					{
						operands op = state.operandlist.back();
						uint32_t val = state.preloadedcode.back().pcode.arg3_uint;
						state.preloadedcode.push_back(state.preloadedcode.back().opcode);
						state.preloadedcode.back().pcode.arg3_uint=val;
						state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
						state.operandlist.push_back(operands(op.type,op.objtype,op.index,1,state.preloadedcode.size()-1));
					}
				}
				else
#endif
				{
					state.preloadedcode.push_back((uint32_t)opcode);
					state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
				}
				typestack.push_back(typestack.back());
				break;
			}
			case 0x01://bkpt
			case 0x02://nop
			case 0x82://coerce_a
			case 0x09://label
			{
				//skip
				int32_t p = code.tellg();
				state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
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
				state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
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
					state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
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
					state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
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
					state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
					opcode_skipped=true;
				}
#else
				jumppositions[state.preloadedcode.size()] = j;
				jumpstartpositions[state.preloadedcode.size()] = code.tellg();
				state.preloadedcode.push_back((uint32_t)opcode);
				state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
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
						state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
						skipunreachablecode(state,code);
						auto it = state.jumptargets.find(code.tellg()+1);
						if (it != state.jumptargets.end() && it->second > 1)
							state.jumptargets[code.tellg()+1]--;
						else
							state.jumptargets.erase(code.tellg()+1);
						state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
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
						state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
						skipunreachablecode(state,code);
						auto it = state.jumptargets.find(code.tellg()+1);
						if (it != state.jumptargets.end() && it->second > 1)
							state.jumptargets[code.tellg()+1]--;
						else
							state.jumptargets.erase(code.tellg()+1);
						state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
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
						state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
						skipunreachablecode(state,code);
						auto it = state.jumptargets.find(code.tellg()+1);
						if (it != state.jumptargets.end() && it->second > 1)
							state.jumptargets[code.tellg()+1]--;
						else
							state.jumptargets.erase(code.tellg()+1);
						state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
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
						state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
						skipunreachablecode(state,code);
						auto it = state.jumptargets.find(code.tellg()+1);
						if (it != state.jumptargets.end() && it->second > 1)
							state.jumptargets[code.tellg()+1]--;
						else
							state.jumptargets.erase(code.tellg()+1);
						state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
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
				state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
				switchpositions[state.preloadedcode.size()] = code.reads24();
				switchstartpositions[state.preloadedcode.size()] = p;
				state.preloadedcode.push_back(0);
				state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
				uint32_t count = code.readu30();
				state.preloadedcode.push_back(0);
				state.preloadedcode.back().pcode.arg3_uint=count;
				for(unsigned int i=0;i<count+1;i++)
				{
					state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
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
				if (state.jumptargets.find(p) != state.jumptargets.end())
					clearOperands(state,true,&lastlocalresulttype);
				if (state.jumptargets.find(code.tellg()) != state.jumptargets.end() && code.peekbyte() == 0x74)//convert_u
				{
					state.operandlist.push_back(operands(OP_UINTEGER,Class<UInteger>::getRef(mi->context->root->getSystemState()).getPtr(),(uint32_t)value,1,state.preloadedcode.size()-1));
					typestack.push_back(typestackentry(Class<UInteger>::getRef(function->getSystemState()).getPtr(),false));
					code.readbyte();
				}
				else
				{
					state.operandlist.push_back(operands(OP_BYTE,Class<Integer>::getRef(mi->context->root->getSystemState()).getPtr(),index,1,state.preloadedcode.size()-1));
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
				if (state.jumptargets.find(p) != state.jumptargets.end())
					clearOperands(state,true,&lastlocalresulttype);
				state.operandlist.push_back(operands(OP_SHORT,Class<Integer>::getRef(mi->context->root->getSystemState()).getPtr(),index,1,state.preloadedcode.size()-1));
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
				state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
				if (state.jumptargets.find(p) != state.jumptargets.end())
					clearOperands(state,true,&lastlocalresulttype);
				state.operandlist.push_back(operands(OP_TRUE, Class<Boolean>::getRef(mi->context->root->getSystemState()).getPtr(),0,1,state.preloadedcode.size()-1));
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
				state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
				if (state.jumptargets.find(p) != state.jumptargets.end())
					clearOperands(state,true,&lastlocalresulttype);
				state.operandlist.push_back(operands(OP_FALSE, Class<Boolean>::getRef(mi->context->root->getSystemState()).getPtr(),0,1,state.preloadedcode.size()-1));
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
				state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
				if (state.jumptargets.find(p) != state.jumptargets.end())
					clearOperands(state,true,&lastlocalresulttype);
				state.operandlist.push_back(operands(OP_NAN, Class<Number>::getRef(mi->context->root->getSystemState()).getPtr(),0,1,state.preloadedcode.size()-1));
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
				if (state.jumptargets.find(p) != state.jumptargets.end())
					clearOperands(state,true,&lastlocalresulttype);
				state.operandlist.push_back(operands(OP_STRING,Class<ASString>::getRef(mi->context->root->getSystemState()).getPtr(),value,1,state.preloadedcode.size()-1));
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
				if (state.jumptargets.find(p) != state.jumptargets.end())
					clearOperands(state,true,&lastlocalresulttype);
				state.operandlist.push_back(operands(OP_INTEGER,Class<Integer>::getRef(mi->context->root->getSystemState()).getPtr(),value,1,state.preloadedcode.size()-1));
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
				if (state.jumptargets.find(p) != state.jumptargets.end())
					clearOperands(state,true,&lastlocalresulttype);
				state.operandlist.push_back(operands(OP_UINTEGER,Class<UInteger>::getRef(mi->context->root->getSystemState()).getPtr(),value,1,state.preloadedcode.size()-1));
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
				if (state.jumptargets.find(p) != state.jumptargets.end())
					clearOperands(state,true,&lastlocalresulttype);
				state.operandlist.push_back(operands(OP_DOUBLE,Class<Number>::getRef(mi->context->root->getSystemState()).getPtr(),value,1,state.preloadedcode.size()-1));
				typestack.push_back(typestackentry(Class<Number>::getRef(function->getSystemState()).getPtr(),false));
				break;
			}
			case 0x30://pushscope
				if (typestack.back().obj)
					scopelist.push_back(scope_entry(asAtomHandler::fromObjectNoPrimitive(typestack.back().obj),false));
				else
					scopelist.push_back(scope_entry(asAtomHandler::invalidAtom,false));
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
				if (state.jumptargets.find(p) != state.jumptargets.end())
					clearOperands(state,true,&lastlocalresulttype);
				state.operandlist.push_back(operands(OP_NAMESPACE,Class<Namespace>::getRef(mi->context->root->getSystemState()).getPtr(),value,1,state.preloadedcode.size()-1));
				typestack.push_back(typestackentry(Class<Namespace>::getRef(function->getSystemState()).getPtr(),false));
				break;
			}
			case 0x32://hasnext2
			{
				state.preloadedcode.push_back((uint32_t)opcode);
				state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
				state.preloadedcode.back().pcode.arg1_uint = code.readu30();
				state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
				state.preloadedcode.back().pcode.arg2_uint = code.readu30();
				clearOperands(state,true,&lastlocalresulttype);
				typestack.push_back(typestackentry(Class<Boolean>::getRef(function->getSystemState()).getPtr(),false));
				break;
			}
			case 0x43://callmethod
			case 0x44://callstatic
			{
				state.preloadedcode.push_back((uint32_t)opcode);
				state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
				state.preloadedcode.back().pcode.arg1_uint = code.readu30();
				state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
				int32_t t = code.readu30();
				state.preloadedcode.back().pcode.arg2_int = t;
				clearOperands(state,true,&lastlocalresulttype);
				removetypestack(typestack,t+1);
				typestack.push_back(typestackentry(nullptr,false));
				break;
			}
			case 0x45://callsuper
			{
				state.preloadedcode.push_back((uint32_t)opcode);
				state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
				int32_t t = code.readu30();
				state.preloadedcode.back().pcode.arg1_int = t;
				state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
				int32_t t2 = code.readu30();
				state.preloadedcode.back().pcode.arg2_int = t2;
				clearOperands(state,true,&lastlocalresulttype);
				removetypestack(typestack,mi->context->constant_pool.multinames[t].runtimeargs+t2+1);
				typestack.push_back(typestackentry(nullptr,false));
				break;
			}
			case 0x4c://callproplex
			{
				state.preloadedcode.push_back((uint32_t)opcode);
				state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
				state.preloadedcode.push_back(0);
				uint32_t t = code.readu30();
				state.preloadedcode.back().pcode.arg1_uint = t;
				state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
				uint32_t t2 = code.readu30();
				state.preloadedcode.back().pcode.arg2_uint = t2;
				clearOperands(state,true,&lastlocalresulttype);
				removetypestack(typestack,mi->context->constant_pool.multinames[t].runtimeargs+t2+1);
				typestack.push_back(typestackentry(nullptr,false));
				break;
			}
			case 0x4e://callsupervoid
			{
				state.preloadedcode.push_back((uint32_t)opcode);
				state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
				int32_t t = code.readu30();
				state.preloadedcode.back().pcode.arg1_uint=t;
				state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
				int32_t t2 = code.readu30();
				state.preloadedcode.back().pcode.arg2_uint=t2;
				clearOperands(state,true,&lastlocalresulttype);
				removetypestack(typestack,mi->context->constant_pool.multinames[t].runtimeargs+t2+1);
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
				state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
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
				bool checkresulttype=true;
				if (!coercereturnvalue) // there was no returnvalue opcode yet that needs coercion, so we keep checking
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
							if (checkmatchingLastObjtype(state,(Class_base*)(dynamic_cast<const Class_base*>(mi->returnType)),state.operandlist.back().objtype)
									|| state.operandlist.back().objtype->isSubClass(dynamic_cast<const Class_base*>(mi->returnType)))
							{
								// return type matches type of last operand, no need to continue checking
								checkresulttype=false;
							}
						}
					}
					if (checkresulttype)
					{
						if (dynamic_cast<const Class_base*>(mi->returnType) == Class<Integer>::getRef(state.mi->context->root->getSystemState()).getPtr() &&
								(!typestack.back().obj
								 || !typestack.back().obj->is<Class_base>() 
								 || (dynamic_cast<const Class_base*>(mi->returnType) && !typestack.back().obj->as<Class_base>()->isSubClass(dynamic_cast<const Class_base*>(mi->returnType)))))
						{
							coercereturnvalue = !checkmatchingLastObjtype(state,(Class_base*)(dynamic_cast<const Class_base*>(mi->returnType)),Class<Integer>::getRef(state.mi->context->root->getSystemState()).getPtr());
						}
						else
							coercereturnvalue = true;
					}
				}
				if (state.operandlist.size() > 0 && state.operandlist.back().type == OP_LOCAL && state.operandlist.back().index == (int32_t)mi->body->getReturnValuePos())
				{
					// if localresult of previous action is put into returnvalue, we can skip this opcode
					opcode_skipped=true;
					state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
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
											resulttype = constructor->as<IFunction>()->getReturnType();
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
									// TODO handle constructprop with one or more arguments
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
			case 0x46://callproperty
			case 0x4f://callpropvoid
			{
				int32_t p = code.tellg();
				state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size()+1;
				uint32_t t = code.readu30();
				assert_and_throw(t < mi->context->constant_pool.multiname_count);
				uint32_t argcount = code.readu30();
#ifdef ENABLE_OPTIMIZATION
				if (opcode == 0x46 && code.peekbyte() == 0x29 && state.jumptargets.find(p+1) == state.jumptargets.end())
				{
					// callproperty is followed by pop
					opcode = 0x4f; // treat call as callpropvoid
					state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size()+1;
					code.readbyte(); // skip pop
				}
				// TODO optimize non-static multinames
				if (state.jumptargets.find(p) == state.jumptargets.end())
				{
					multiname* name =  mi->context->getMultinameImpl(asAtomHandler::nullAtom,nullptr,t,false);
					Class_base* resulttype=nullptr;
					bool skipcoerce = false;
					SyntheticFunction* func= nullptr;
					variable* v = nullptr;
					uint32_t operationcount = argcount+mi->context->constant_pool.multinames[t].runtimeargs+1;
					if (typestack.size() >= operationcount)
					{
						bool isoverridden=false;
						ASObject* cls = nullptr;
						if (canCallFunctionDirect(typestack[typestack.size()-operationcount].obj,name,false))
						{
							cls = typestack[typestack.size()-operationcount].obj;
							v = typestack[typestack.size()-operationcount].classvar ?
										cls->findVariableByMultiname(
										*name,
										cls->as<Class_base>(),
										nullptr,nullptr,false,wrk):
										cls->as<Class_base>()->getBorrowedVariableByMultiname(*name);
						}
						else if (canCallFunctionDirect(typestack[typestack.size()-operationcount].obj,name,true))
						{
							isoverridden=true;
							cls = typestack[typestack.size()-operationcount].obj;
							v = typestack[typestack.size()-operationcount].classvar ?
										cls->findVariableByMultiname(
											*name,
											cls->as<Class_base>(),
											nullptr,nullptr,false,wrk):
										cls->as<Class_base>()->getBorrowedVariableByMultiname(*name);
						}
						if (!v
								&& state.operandlist.size()>=operationcount
								&& (state.operandlist[state.operandlist.size()-operationcount].type == OP_LOCAL
									|| state.operandlist[state.operandlist.size()-operationcount].type == OP_CACHED_CONSTANT)
								&& state.operandlist[state.operandlist.size()-operationcount].objtype && state.operandlist[state.operandlist.size()-operationcount].objtype->is<Class_base>()
								&& canCallFunctionDirect(state.operandlist.at(state.operandlist.size()-operationcount),name))
						{
							cls = state.operandlist[state.operandlist.size()-operationcount].objtype;
							v=cls->findVariableByMultiname(*name,cls->as<Class_base>(),nullptr,nullptr,false,wrk);
							if (v && !asAtomHandler::is<SyntheticFunction>(v->var))
								v=nullptr;
						}
						if (v)
						{
							if (asAtomHandler::is<SyntheticFunction>(v->var) && asAtomHandler::as<SyntheticFunction>(v->var)->inClass == cls)
							{
								func = asAtomHandler::as<SyntheticFunction>(v->var);
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
							if (isoverridden) // method is overridden, so we can not further optimize it
							{
								func=nullptr;
								v=nullptr;
							}
						}
					}
					switch (mi->context->constant_pool.multinames[t].runtimeargs)
					{
						case 0:
							switch (argcount)
							{
								case 0:
									if (state.operandlist.size() > 0
											&& (state.operandlist.back().type == OP_LOCAL || state.operandlist.back().type == OP_CACHED_CONSTANT || state.operandlist.back().type == OP_CACHED_SLOT)
											&& state.operandlist.back().objtype)
									{
										if (canCallFunctionDirect(state.operandlist.back(),name))
										{
											if (v && asAtomHandler::is<IFunction>(v->var) && asAtomHandler::as<IFunction>(v->var)->closure_this==nullptr)
											{
												ASObject* cls = state.operandlist.back().objtype;
												if (opcode == 0x46)
												{
													resulttype = asAtomHandler::as<IFunction>(v->var)->getReturnType();
													if (!resulttype && asAtomHandler::is<Function>(v->var))
														LOG(LOG_NOT_IMPLEMENTED,"missing result type for builtin method1:"<<*name<<" "<<cls->toDebugString());
													setupInstructionOneArgument(state,ABC_OP_OPTIMZED_CALLFUNCTION_NOARGS ,opcode,code,true, false,resulttype,p,true,false,false,true,ABC_OP_OPTIMZED_CALLFUNCTION_NOARGS_SETSLOT);
												}
												else
													setupInstructionOneArgumentNoResult(state,ABC_OP_OPTIMZED_CALLFUNCTION_NOARGS_VOID,opcode,code,p);
												state.preloadedcode.at(state.preloadedcode.size()-1).pcode.cacheobj2 = asAtomHandler::getObject(v->var);
												removetypestack(typestack,argcount+mi->context->constant_pool.multinames[t].runtimeargs+1);
												if (opcode == 0x46)
												{
													if (resulttype == nullptr && asAtomHandler::is<Function>(v->var))
													{
														resulttype = asAtomHandler::as<Function>(v->var)->getReturnType();
														if (resulttype == nullptr)
															LOG(LOG_NOT_IMPLEMENTED,"missing result type for builtin method2:"<<*name<<" "<<cls->toDebugString());
													}
													typestack.push_back(typestackentry(resulttype,false));
												}
												break;
											}
										}
									}
									if ((opcode == 0x4f && setupInstructionOneArgumentNoResult(state,ABC_OP_OPTIMZED_CALLPROPVOID_STATICNAME_NOARGS,opcode,code,p)) ||
									   ((opcode == 0x46 && setupInstructionOneArgument(state,ABC_OP_OPTIMZED_CALLPROPERTY_STATICNAME_NOARGS,opcode,code,true,false,resulttype,p,true))))
									{
										state.preloadedcode.at(state.preloadedcode.size()-1).pcode.cachedmultiname2 = name;
										state.preloadedcode.push_back(0);
									}
									else
									{
										if (func)
										{
											state.preloadedcode.at(state.preloadedcode.size()-1).opcode= (opcode == 0x4f ? ABC_OP_OPTIMZED_CALLFUNCTIONVOIDSYNTHETIC_STATICNAME_MULTIARGS:ABC_OP_OPTIMZED_CALLFUNCTIONSYNTHETIC_STATICNAME_MULTIARGS);
											state.preloadedcode.back().pcode.local2.pos = argcount;
											if (skipcoerce)
												state.preloadedcode.back().pcode.local2.flags = ABC_OP_COERCED;
											state.preloadedcode.at(state.preloadedcode.size()-1).pcode.cacheobj3 = func;
										}
										else
										{
											state.preloadedcode.at(state.preloadedcode.size()-1).opcode= (opcode == 0x4f ? ABC_OP_OPTIMZED_CALLPROPVOID_STATICNAME_MULTIARGS:ABC_OP_OPTIMZED_CALLPROPERTY_STATICNAME_MULTIARGS);
											state.preloadedcode.back().pcode.local2.pos = argcount;
											if (skipcoerce)
												state.preloadedcode.back().pcode.local2.flags = ABC_OP_COERCED;
											state.preloadedcode.push_back(0);
											state.preloadedcode.at(state.preloadedcode.size()-1).pcode.cachedmultiname2 = name;
										}
										clearOperands(state,true,&lastlocalresulttype);
									}
									removetypestack(typestack,argcount+mi->context->constant_pool.multinames[t].runtimeargs+1);
									if (opcode == 0x46)
										typestack.push_back(typestackentry(resulttype,false));
									break;
								case 1:
								{
									bool isGenerator=false;
									bool generatorneedsconversion=false;
									if (typestack.size() > 1 &&
											typestack[typestack.size()-2].obj != nullptr &&
											(typestack[typestack.size()-2].obj->is<Global>() ||
											 (typestack[typestack.size()-2].obj->is<Class_base>() && typestack[typestack.size()-2].obj->as<Class_base>()->isSealed)))
									{
										asAtom func = asAtomHandler::invalidAtom;
										GET_VARIABLE_RESULT r = typestack[typestack.size()-2].obj->getVariableByMultiname(func,*name,GET_VARIABLE_OPTION(DONT_CALL_GETTER|FROM_GETLEX|NO_INCREF),wrk);
										if (asAtomHandler::isInvalid(func) && typestack[typestack.size()-2].obj->is<Class_base>())
										{
											variable* mvar = typestack[typestack.size()-2].obj->as<Class_base>()->getBorrowedVariableByMultiname(*name);
											if (mvar)
												func = mvar->var;
										}
										if (asAtomHandler::isClass(func) && asAtomHandler::as<Class_base>(func)->isBuiltin())
										{
											// function is a class generator, we can use it as the result type
											resulttype = asAtomHandler::as<Class_base>(func);
											if (resulttype == Class<Integer>::getRef(function->getSystemState()).getPtr()
													|| resulttype == Class<UInteger>::getRef(function->getSystemState()).getPtr()
													|| resulttype == Class<Number>::getRef(function->getSystemState()).getPtr())
											{
												if (state.operandlist.size() > 2 && typestack.size() > 0 && typestack.back().obj == resulttype)
													isGenerator=true; // generator can be skipped
												else if (state.operandlist.size() > 1)
												{
													// generator will be replaced by a conversion operator
													isGenerator=true;
													if (resulttype != Class<Number>::getRef(function->getSystemState()).getPtr()
															|| (state.operandlist.back().objtype != Class<Integer>::getRef(function->getSystemState()).getPtr()
																&& state.operandlist.back().objtype != Class<UInteger>::getRef(function->getSystemState()).getPtr()
																&& state.operandlist.back().objtype != Class<Number>::getRef(function->getSystemState()).getPtr()))
														generatorneedsconversion=true;
												}
											}
										}
										else if (asAtomHandler::is<SyntheticFunction>(func) && opcode == 0x46)
										{
											SyntheticFunction* f = asAtomHandler::as<SyntheticFunction>(func);
											resulttype = f->getReturnType();
										}
										else if (asAtomHandler::is<Function>(func) && opcode == 0x46)
										{
											Function* f = asAtomHandler::as<Function>(func);
											resulttype = f->getReturnType();
											if (!resulttype)
												LOG(LOG_NOT_IMPLEMENTED,"missing result type for builtin method3:"<<*name<<" "<<typestack[typestack.size()-2].obj->toDebugString());
										}
										if (r & GETVAR_ISNEWOBJECT)
										{
											ASATOM_DECREF(func);
										}
									}
									if (state.operandlist.size() > 1 && !isGenerator)
									{
										auto it = state.operandlist.rbegin();
										Class_base* argtype = it->objtype;
										it++;
										if (canCallFunctionDirect((*it),name))
										{
											if (v && asAtomHandler::is<IFunction>(v->var) && asAtomHandler::as<IFunction>(v->var)->closure_this==nullptr)
											{
												if (asAtomHandler::is<SyntheticFunction>(v->var))
												{
													if (opcode == 0x4f)
														setupInstructionTwoArgumentsNoResult(state,ABC_OP_OPTIMZED_CALLFUNCTIONSYNTHETIC_ONEARG_VOID,opcode,code);
													else
														setupInstructionTwoArguments(state,ABC_OP_OPTIMZED_CALLFUNCTIONSYNTHETIC_ONEARG,opcode,code,false,false,true,p,resulttype);
													if (argtype)
													{
														SyntheticFunction* f = asAtomHandler::as<SyntheticFunction>(v->var);
														if (!f->getMethodInfo()->returnType)
															f->checkParamTypes();
														if (f->getMethodInfo()->paramTypes.size() && f->canSkipCoercion(0,argtype))
															state.preloadedcode.at(state.preloadedcode.size()-1).pcode.local3.flags = ABC_OP_COERCED;
													}
													state.preloadedcode.push_back(0);
													state.preloadedcode.back().pcode.cacheobj3 = asAtomHandler::getObject(v->var);
													removetypestack(typestack,argcount+mi->context->constant_pool.multinames[t].runtimeargs+1);
													if (opcode == 0x46)
														typestack.push_back(typestackentry(resulttype,false));
													break;
												}
												if (asAtomHandler::is<Function>(v->var))
												{
													if (opcode == 0x4f)
													{
														setupInstructionTwoArgumentsNoResult(state,ABC_OP_OPTIMZED_CALLFUNCTIONBUILTIN_ONEARG_VOID,opcode,code);
													}
													else
													{
														setupInstructionTwoArguments(state,ABC_OP_OPTIMZED_CALLFUNCTIONBUILTIN_ONEARG,opcode,code,false,false,true,p,resulttype);
														state.preloadedcode.push_back(0);
													}
													state.preloadedcode.at(state.preloadedcode.size()-1).pcode.cacheobj3 = asAtomHandler::getObject(v->var);
													removetypestack(typestack,argcount+mi->context->constant_pool.multinames[t].runtimeargs+1);
													if (opcode == 0x46)
														typestack.push_back(typestackentry(resulttype,false));
													break;
												}
											}
										}
									}
									// remember operand for isGenerator
									operands op;
									if (state.operandlist.size()>0)
										op = state.operandlist.back();
									bool reuseoperand = prevopcode == 0x62//getlocal
											|| prevopcode == 0xd0//getlocal_0
											|| prevopcode == 0xd1//getlocal_1
											|| prevopcode == 0xd2//getlocal_2
											|| prevopcode == 0xd3//getlocal_3
											;
									uint32_t opsize=state.operandlist.size();
									if ((opcode == 0x4f && setupInstructionTwoArgumentsNoResult(state,ABC_OP_OPTIMZED_CALLPROPVOID_STATICNAME,opcode,code)) ||
									   ((opcode == 0x46 && setupInstructionTwoArguments(state,ABC_OP_OPTIMZED_CALLPROPERTY_STATICNAME,opcode,code,false,false,!(reuseoperand || generatorneedsconversion) || !isGenerator,p,resulttype))))
									{
										// generator for Integer/UInteger/Number can be skipped if argument is already an Integer/UInteger/Number and the result will be used as local result
										if (isGenerator && (reuseoperand || generatorneedsconversion))
										{
											removetypestack(typestack,argcount+mi->context->constant_pool.multinames[t].runtimeargs+1);
											// remove caller
											if (opcode == 0x46 && state.preloadedcode.back().opcode >= ABC_OP_OPTIMZED_CALLPROPERTY_STATICNAME+4 // caller has localresult
													&& state.operandlist.size() > opsize-2) // localresult was added to operandlist
											{
												auto it =state.operandlist.end();
												(--it)->removeArg(state);// remove arg2
												state.operandlist.pop_back();
											}
											state.preloadedcode.pop_back();
											// re-add last operand if it is not the result of the previous operation
											if (reuseoperand || generatorneedsconversion)
											{
												state.operandlist.push_back(operands(op.type, op.objtype,op.index,0, 0));
											}
											state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
											if (generatorneedsconversion)
											{
												// replace call to generator with optimized convert_i/convert_d
												if (resulttype == Class<Integer>::getRef(function->getSystemState()).getPtr())
													setupInstructionOneArgument(state,ABC_OP_OPTIMZED_CONVERTI,
																				0x73 //convert_i
																				,code,true,true,resulttype,code.tellg(),true,false,false,true,ABC_OP_OPTIMZED_CONVERTI_SETSLOT);
												else if (resulttype == Class<UInteger>::getRef(function->getSystemState()).getPtr())
													setupInstructionOneArgument(state,ABC_OP_OPTIMZED_CONVERTU,
																				0x74 //convert_u
																				,code,true,true,resulttype,code.tellg(),true,false,false,true,ABC_OP_OPTIMZED_CONVERTU_SETSLOT);
												else 
													setupInstructionOneArgument(state,ABC_OP_OPTIMZED_CONVERTD,
																				0x75 //convert_d
																				,code,true,true,resulttype,code.tellg(),true,false,false,true,ABC_OP_OPTIMZED_CONVERTD_SETSLOT);
											}
											typestack.push_back(typestackentry(resulttype,false));
											break;
										}
										state.preloadedcode.push_back(0);
										state.preloadedcode.at(state.preloadedcode.size()-1).pcode.cachedmultiname2 = name;
										state.preloadedcode.push_back(0);
										state.preloadedcode.at(state.preloadedcode.size()-1).pcode.local2.flags =(skipcoerce ? ABC_OP_COERCED : 0);
									}
									else if (opcode == 0x46 && checkForLocalResult(state,code,0,resulttype))
									{
										state.preloadedcode.at(state.preloadedcode.size()-1).opcode=ABC_OP_OPTIMZED_CALLPROPERTY_STATICNAME_LOCALRESULT;
										state.preloadedcode.at(state.preloadedcode.size()-1).pcode.cachedmultiname2 = name;
										state.preloadedcode.push_back(0);
										state.preloadedcode.back().pcode.local2.flags = (skipcoerce ? ABC_OP_COERCED : 0);
									}
									else
									{
										if (func)
										{
											state.preloadedcode.at(state.preloadedcode.size()-1).opcode= (opcode == 0x4f ? ABC_OP_OPTIMZED_CALLFUNCTIONVOIDSYNTHETIC_STATICNAME_MULTIARGS:ABC_OP_OPTIMZED_CALLFUNCTIONSYNTHETIC_STATICNAME_MULTIARGS);
											state.preloadedcode.back().pcode.local2.pos = argcount;
											if (skipcoerce)
												state.preloadedcode.back().pcode.local2.flags = ABC_OP_COERCED;
											state.preloadedcode.at(state.preloadedcode.size()-1).pcode.cacheobj3 = func;
										}
										else
										{
											state.preloadedcode.at(state.preloadedcode.size()-1).opcode= (opcode == 0x4f ? ABC_OP_OPTIMZED_CALLPROPVOID_STATICNAME_MULTIARGS:ABC_OP_OPTIMZED_CALLPROPERTY_STATICNAME_MULTIARGS);
											state.preloadedcode.back().pcode.local2.pos = argcount;
											if (skipcoerce)
												state.preloadedcode.back().pcode.local2.flags = ABC_OP_COERCED;
											state.preloadedcode.push_back(0);
											state.preloadedcode.at(state.preloadedcode.size()-1).pcode.cachedmultiname2 = name;
										}
										clearOperands(state,true,&lastlocalresulttype);
									}
									removetypestack(typestack,argcount+mi->context->constant_pool.multinames[t].runtimeargs+1);
									if (opcode == 0x46)
										typestack.push_back(typestackentry(resulttype,false));
									break;
								}
								default:
									if (state.operandlist.size() >= argcount)
									{
										state.preloadedcode.push_back((uint32_t)(opcode == 0x4f ? ABC_OP_OPTIMZED_CALLPROPVOID_STATICNAME_MULTIARGS_CACHED:ABC_OP_OPTIMZED_CALLPROPERTY_STATICNAME_MULTIARGS_CACHED));
										bool allargsint=true;
										auto it = state.operandlist.rbegin();
										for(uint32_t i= 0; i < argcount; i++)
										{
											it->removeArg(state);
											state.preloadedcode.push_back(0);
											it->fillCode(state,0,state.preloadedcode.size()-1,false);
											state.preloadedcode.back().pcode.arg2_uint = it->type;
											if (!it->objtype || it->objtype != Class<Integer>::getRef(function->getSystemState()).getPtr())
												allargsint=false;
											it++;
										}
										
										uint32_t oppos = state.preloadedcode.size()-1-argcount;
										state.preloadedcode.at(oppos+1).pcode.cachedmultiname3 = name;
										if (state.operandlist.size() > argcount)
										{
											if (canCallFunctionDirect((*it),name))
											{
												if (v && asAtomHandler::is<IFunction>(v->var) && asAtomHandler::as<IFunction>(v->var)->closure_this==nullptr)
												{
													if (asAtomHandler::is<SyntheticFunction>(v->var))
													{
														state.preloadedcode.at(oppos).opcode = (opcode == 0x4f ? ABC_OP_OPTIMZED_CALLFUNCTIONSYNTHETIC_MULTIARGS_VOID : ABC_OP_OPTIMZED_CALLFUNCTIONSYNTHETIC_MULTIARGS);
														state.preloadedcode.at(oppos).pcode.local2.pos = argcount;
														if (skipcoerce)
															state.preloadedcode.at(oppos).pcode.local2.flags = ABC_OP_COERCED;
														it->fillCode(state,0,oppos,true);
														it->removeArg(state);
														oppos = state.preloadedcode.size()-1-argcount;
														state.preloadedcode.at(oppos).pcode.cacheobj3 = asAtomHandler::getObject(v->var);
														removeOperands(state,true,&lastlocalresulttype,argcount+1);
														if (opcode == 0x46)
															checkForLocalResult(state,code,2,resulttype,oppos,state.preloadedcode.size()-1);
														removetypestack(typestack,argcount+mi->context->constant_pool.multinames[t].runtimeargs+1);
														if (opcode == 0x46)
															typestack.push_back(typestackentry(resulttype,false));
														break;
													}
													else if (asAtomHandler::is<Function>(v->var))
													{
														if (opcode == 0x46)
															resulttype = asAtomHandler::as<Function>(v->var)->getArgumentDependentReturnType(allargsint);
														state.preloadedcode.at(oppos).opcode = (opcode == 0x4f ? ABC_OP_OPTIMZED_CALLFUNCTIONBUILTIN_MULTIARGS_VOID : ABC_OP_OPTIMZED_CALLFUNCTIONBUILTIN_MULTIARGS);
														state.preloadedcode.at(oppos).pcode.local2.pos = argcount;
														if (skipcoerce)
															state.preloadedcode.at(oppos).pcode.local2.flags = ABC_OP_COERCED;
														it->fillCode(state,0,oppos,true);
														it->removeArg(state);
														oppos = state.preloadedcode.size()-1-argcount;
														state.preloadedcode.at(oppos).pcode.cacheobj3 = asAtomHandler::getObject(v->var);
														removeOperands(state,true,&lastlocalresulttype,argcount+1);
														if (opcode == 0x46)
															checkForLocalResult(state,code,2,resulttype,oppos,state.preloadedcode.size()-1);
														removetypestack(typestack,argcount+mi->context->constant_pool.multinames[t].runtimeargs+1);
														if (opcode == 0x46)
															typestack.push_back(typestackentry(resulttype,false));
														break;
													}
												}
											}
											state.preloadedcode.at(oppos).opcode = (opcode == 0x4f ? ABC_OP_OPTIMZED_CALLPROPVOID_STATICNAME_MULTIARGS_CACHED_CALLER:ABC_OP_OPTIMZED_CALLPROPERTY_STATICNAME_MULTIARGS_CACHED_CALLER);
											state.preloadedcode.at(oppos).pcode.local2.pos = argcount;
											state.preloadedcode.at(oppos).pcode.local2.flags = (skipcoerce ? ABC_OP_COERCED : 0);
											it->removeArg(state);
											oppos = state.preloadedcode.size()-1-argcount;
											state.preloadedcode.push_back(0);
											if (it->fillCode(state,1,state.preloadedcode.size()-1,false))
												state.preloadedcode.at(oppos).opcode++;
											removeOperands(state,true,&lastlocalresulttype,argcount+1);
											if (opcode == 0x46)
												checkForLocalResult(state,code,2,nullptr,oppos,state.preloadedcode.size()-1);
											removetypestack(typestack,argcount+mi->context->constant_pool.multinames[t].runtimeargs+1);
											if (opcode == 0x46)
												typestack.push_back(typestackentry(resulttype,false));
											break;
										}
										state.preloadedcode.at(oppos).pcode.local2.pos = argcount;
										state.preloadedcode.at(oppos).pcode.local2.flags = (skipcoerce ? ABC_OP_COERCED : 0);
										clearOperands(state,true,&lastlocalresulttype);
										if (opcode == 0x46)
											checkForLocalResult(state,code,1,resulttype,state.preloadedcode.size()-1-argcount,state.preloadedcode.size()-1);
										removetypestack(typestack,argcount+mi->context->constant_pool.multinames[t].runtimeargs+1);
										if (opcode == 0x46)
											typestack.push_back(typestackentry(resulttype,false));
									}
									else
									{
										if (func)
										{
											state.preloadedcode.push_back((uint32_t)(opcode == 0x4f ? ABC_OP_OPTIMZED_CALLFUNCTIONVOIDSYNTHETIC_STATICNAME_MULTIARGS:ABC_OP_OPTIMZED_CALLFUNCTIONSYNTHETIC_STATICNAME_MULTIARGS));
											state.preloadedcode.back().pcode.local2.pos = argcount;
											if (skipcoerce)
												state.preloadedcode.back().pcode.local2.flags = ABC_OP_COERCED;
											state.preloadedcode.at(state.preloadedcode.size()-1).pcode.cacheobj3 = func;
										}
										else
										{
											state.preloadedcode.push_back((uint32_t)(opcode == 0x4f ? ABC_OP_OPTIMZED_CALLPROPVOID_STATICNAME_MULTIARGS:ABC_OP_OPTIMZED_CALLPROPERTY_STATICNAME_MULTIARGS));
											state.preloadedcode.back().pcode.local2.pos = argcount;
											if (skipcoerce)
												state.preloadedcode.back().pcode.local2.flags = ABC_OP_COERCED;
											state.preloadedcode.push_back(0);
											state.preloadedcode.back().pcode.cachedmultiname2 = name;
										}
										clearOperands(state,true,&lastlocalresulttype);
										removetypestack(typestack,argcount+mi->context->constant_pool.multinames[t].runtimeargs+1);
										if (opcode == 0x46)
											typestack.push_back(typestackentry(resulttype,false));
									}
									break;
							}
							if (opcode == 0x46)
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
						default:
							state.preloadedcode.push_back((uint32_t)opcode);
							clearOperands(state,true,&lastlocalresulttype);
							state.preloadedcode.push_back(0);
							state.preloadedcode.back().pcode.arg1_uint=t;
							state.preloadedcode.back().pcode.arg2_uint=argcount;
							if (opcode == 0x46)
								typestack.push_back(typestackentry(resulttype,false));
							break;
					}
				}
				else
#endif
				{
					state.preloadedcode.push_back((uint32_t)opcode);
					clearOperands(state,true,&lastlocalresulttype);
					state.preloadedcode.push_back(0);
					state.preloadedcode.back().pcode.arg1_uint=t;
					state.preloadedcode.back().pcode.arg2_uint=argcount;
					if (opcode == 0x46)
						typestack.push_back(typestackentry(nullptr,false));
				}
				break;
			}
			case 0x64://getglobalscope
			{
				int32_t p = code.tellg();
				if (state.jumptargets.find(p) != state.jumptargets.end())
					clearOperands(state,true,&lastlocalresulttype);
				ASObject* resulttype=nullptr;
#ifdef ENABLE_OPTIMIZATION
				if (function->func_scope.getPtr() && (scopelist.begin()==scopelist.end() || !scopelist.back().considerDynamic))
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
			{
				int32_t p = code.tellg();
				state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size()+1;
				uint32_t t = code.readu30();
				assert_and_throw(t < mi->context->constant_pool.multiname_count);
				bool addname = true;
#ifdef ENABLE_OPTIMIZATION
				if (state.jumptargets.find(p) == state.jumptargets.end())
				{
					switch (mi->context->constant_pool.multinames[t].runtimeargs)
					{
						case 0:
						{
							multiname* name = mi->context->getMultinameImpl(asAtomHandler::nullAtom,nullptr,t,false);
							Class_base* resulttype = nullptr;
							if (state.operandlist.size() > 0 && state.operandlist.back().type != OP_LOCAL && state.operandlist.back().type != OP_CACHED_SLOT)
							{
								asAtom* a = mi->context->getConstantAtom(state.operandlist.back().type,state.operandlist.back().index);
								if (asAtomHandler::getObject(*a))
								{
									bool isborrowed=false;
									variable* v = asAtomHandler::getObject(*a)->findVariableByMultiname(*name,asAtomHandler::getObject(*a)->getClass(),nullptr,&isborrowed,false,wrk);
									if (v && v->kind == CONSTANT_TRAIT && asAtomHandler::isInvalid(v->getter) )
									{
										state.operandlist.back().removeArg(state);
										state.operandlist.pop_back();
										addCachedConstant(state,mi, v->var,code);
										addname = false;
										removetypestack(typestack,mi->context->constant_pool.multinames[t].runtimeargs+1);
										typestack.push_back(typestackentry(asAtomHandler::getClass(v->var,function->getSystemState()),isborrowed|| asAtomHandler::isClass(v->var)));
										break;
									}
									if (v && asAtomHandler::isValid(v->getter))
									{
										Class_base* resulttype = (Class_base*)(v->isResolved ? dynamic_cast<const Class_base*>(v->type):nullptr);
										if (!setupInstructionOneArgument(state,ABC_OP_OPTIMZED_CALLFUNCTION_NOARGS,opcode,code,true, false,resulttype,p,true,false,false,true,ABC_OP_OPTIMZED_CALLFUNCTION_NOARGS_SETSLOT))
											lastlocalresulttype = resulttype;
										state.preloadedcode.at(state.preloadedcode.size()-1).pcode.cacheobj2 = asAtomHandler::getObject(v->getter);
										addname = false;
										removetypestack(typestack,mi->context->constant_pool.multinames[t].runtimeargs+1);
										typestack.push_back(typestackentry(resulttype,false));
										break;
									}
									if (v && asAtomHandler::is<Class_base>(*a) && v->kind==DECLARED_TRAIT && !isborrowed)
									{
										if (asAtomHandler::is<IFunction>(v->var)) // in function declarations the resulttype of the function is stored in v->type
											resulttype = Class<IFunction>::getRef(function->getSystemState()).getPtr();
										else
											resulttype = (Class_base*)(v->isResolved ? dynamic_cast<const Class_base*>(v->type):nullptr);
									}
									if (v && v->slotid && (!typestack.back().obj || !typestack.back().classvar) && (!asAtomHandler::is<Class_base>(*a) || v->kind!=INSTANCE_TRAIT))
									{
										Class_base* resulttype = (Class_base*)(v->isResolved ? dynamic_cast<const Class_base*>(v->type):nullptr);
										if (asAtomHandler::getObject(*a)->is<Global>())
										{
											// ensure init script is run
											asAtom ret = asAtomHandler::invalidAtom;
											asAtomHandler::getObject(*a)->getVariableByMultiname(ret,*name,GET_VARIABLE_OPTION(DONT_CALL_GETTER|FROM_GETLEX),wrk);
											ASATOM_DECREF(ret);
										}
										if (setupInstructionOneArgument(state,ABC_OP_OPTIMZED_GETSLOT,opcode,code,true,false,resulttype,p,true,false,false,true,ABC_OP_OPTIMZED_GETSLOT_SETSLOT))
										{
											state.preloadedcode.at(state.preloadedcode.size()-1).pcode.arg2_uint =v->slotid-1;
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
							if (state.operandlist.size() > 0 && (state.operandlist.back().type == OP_LOCAL || state.operandlist.back().type == OP_CACHED_CONSTANT || state.operandlist.back().type == OP_CACHED_SLOT) && state.operandlist.back().objtype)
							{
								bool isClassOperator=false;
								if (state.operandlist.back().type == OP_CACHED_CONSTANT)
								{
									asAtom* a = mi->context->getConstantAtom(state.operandlist.back().type,state.operandlist.back().index);
									isClassOperator = asAtomHandler::is<Class_base>(*a);
								}
								if (!isClassOperator && canCallFunctionDirect(state.operandlist.back(),name))
								{
									variable* v = state.operandlist.back().objtype->getBorrowedVariableByMultiname(*name);
									if (v && asAtomHandler::is<IFunction>(v->getter))
									{
										resulttype = v->isResolved && dynamic_cast<const Class_base*>(v->type) ? (Class_base*)v->type : nullptr;
										if (!state.operandlist.back().objtype->is<Class_inherit>() && resulttype==nullptr)
											LOG(LOG_NOT_IMPLEMENTED,"missing result type for builtin method4:"<<*name<<" "<<state.operandlist.back().objtype->toDebugString()<<" in function "<<getSys()->getStringFromUniqueId(function->functionname));
										if (!setupInstructionOneArgument(state,ABC_OP_OPTIMZED_CALLFUNCTION_NOARGS,opcode,code,true, false,resulttype,p,true,false,false,true,ABC_OP_OPTIMZED_CALLFUNCTION_NOARGS_SETSLOT))
											lastlocalresulttype = resulttype;
										state.preloadedcode.at(state.preloadedcode.size()-1).pcode.cacheobj2 = asAtomHandler::getObject(v->getter);
										addname = false;
										removetypestack(typestack,mi->context->constant_pool.multinames[t].runtimeargs+1);
										typestack.push_back(typestackentry(resulttype,false));
										break;
									}
								}
								if (!isClassOperator && canCallFunctionDirect(state.operandlist.back(),name,true))
								{
									variable* v = state.operandlist.back().objtype->getBorrowedVariableByMultiname(*name);
									if (v)
									{
										resulttype = v->isResolved && dynamic_cast<const Class_base*>(v->type) ? (Class_base*)v->type : nullptr;
									}
								}
								if (state.operandlist.back().objtype && !state.operandlist.back().objtype->isInterface && state.operandlist.back().objtype->isInitialized()
										&& (!typestack.back().obj || !typestack.back().classvar || isClassOperator))
								{
									// check if we can replace getProperty by getSlot
									variable* v = nullptr;
									asAtom o = asAtomHandler::invalidAtom;
									if (isClassOperator)
									{
										asAtom* a = mi->context->getConstantAtom(state.operandlist.back().type,state.operandlist.back().index);
										v = asAtomHandler::getObject(*a)->findVariableByMultiname(*name,nullptr,nullptr,nullptr,false,wrk);
										if (v && v->kind == TRAIT_KIND::INSTANCE_TRAIT)
											v=nullptr;
									}
									else
									{
										if (state.operandlist.back().objtype->is<Class_inherit>())
											state.operandlist.back().objtype->as<Class_inherit>()->checkScriptInit();
										state.operandlist.back().objtype->getInstance(wrk,o,false,nullptr,0);
										state.operandlist.back().objtype->setupDeclaredTraits(asAtomHandler::getObject(o));
										
										v = asAtomHandler::getObject(o)->findVariableByMultiname(*name,nullptr,nullptr,nullptr,false,wrk);
									}
									if (v && v->slotid)
									{
										resulttype = v->isResolved && dynamic_cast<const Class_base*>(v->type) ? (Class_base*)v->type : nullptr;
										if (state.operandlist.back().type == OP_LOCAL)
										{
											if (state.unchangedlocals.find(state.operandlist.back().index) != state.unchangedlocals.end())
											{
												uint32_t index = state.operandlist.back().index;
												state.operandlist.back().removeArg(state);
												state.operandlist.pop_back();
												addname = false;
												addCachedSlot(state,index,v->slotid,code,resulttype);
												removetypestack(typestack,mi->context->constant_pool.multinames[t].runtimeargs+1);
												typestack.push_back(typestackentry(resulttype,false));
												ASATOM_DECREF(o);
												break;
											}
										}
										
										if (setupInstructionOneArgument(state,ABC_OP_OPTIMZED_GETSLOT,opcode,code,true,false,resulttype,p,true,false,false,true,ABC_OP_OPTIMZED_GETSLOT_SETSLOT))
										{
											state.preloadedcode.at(state.preloadedcode.size()-1).pcode.arg2_uint =v->slotid-1;
											if (state.operandlist.empty()) // indicates that checkforlocalresult returned false
												lastlocalresulttype = resulttype;
											addname = false;
											ASATOM_DECREF(o);
											removetypestack(typestack,mi->context->constant_pool.multinames[t].runtimeargs+1);
											typestack.push_back(typestackentry(resulttype,false));
											break;
										}
										else
										{
											if (checkForLocalResult(state,code,0,resulttype))
											{
												state.preloadedcode.at(state.preloadedcode.size()-1).pcode.func = abc_getPropertyStaticName_localresult;
												addname = false;
											}
											else
												lastlocalresulttype = resulttype;
											state.preloadedcode.at(state.preloadedcode.size()-1).pcode.cachedmultiname2 = name;
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
							bool hasoperands = setupInstructionOneArgument(state,ABC_OP_OPTIMZED_GETPROPERTY_STATICNAME,opcode,code,true, false,resulttype,p,true);
							addname = !hasoperands;
							if (!hasoperands && checkForLocalResult(state,code,0,nullptr))
							{
								state.preloadedcode.at(state.preloadedcode.size()-1).pcode.func=abc_getPropertyStaticName_localresult;
								addname = false;
							}
							state.preloadedcode.at(state.preloadedcode.size()-1).pcode.cachedmultiname2 = name;
							removetypestack(typestack,mi->context->constant_pool.multinames[t].runtimeargs+1);
							typestack.push_back(typestackentry(resulttype,false));
							break;
						}
						case 1:
						{
							Class_base* resulttype = nullptr;
							if (state.operandlist.size() > 1
									&& state.operandlist[state.operandlist.size()-2].objtype
									&& dynamic_cast<TemplatedClass<Vector>*>(state.operandlist[state.operandlist.size()-2].objtype))
							{
								TemplatedClass<Vector>* cls=state.operandlist[state.operandlist.size()-2].objtype->as<TemplatedClass<Vector>>();
								resulttype = cls->getTypes().size() > 0 ? (Class_base*)cls->getTypes()[0] : nullptr;
							}
							if (state.operandlist.size() > 0
									&& (state.operandlist.back().objtype == Class<Integer>::getRef(function->getSystemState()).getPtr() || state.operandlist.back().objtype == Class<UInteger>::getRef(function->getSystemState()).getPtr()))
							{
								addname = !setupInstructionTwoArguments(state,ABC_OP_OPTIMZED_GETPROPERTY_INTEGER,opcode,code,false,false,true,p,resulttype);
							}
							else if (state.operandlist.size() == 0 && typestack.size() > 0 && typestack.back().obj == Class<Integer>::getRef(function->getSystemState()).getPtr())
							{
								state.preloadedcode.push_back((uint32_t)ABC_OP_OPTIMZED_GETPROPERTY_INTEGER_SIMPLE);
								clearOperands(state,true,&lastlocalresulttype);
								addname=false;
							}
							else
							{
								if (setupInstructionTwoArguments(state,ABC_OP_OPTIMZED_GETPROPERTY,opcode,code,false,false,true,p,resulttype))
								{
									state.preloadedcode.push_back(0);
									state.preloadedcode.back().pcode.arg3_uint=t;
									addname = false;
								}
							}
							removetypestack(typestack,mi->context->constant_pool.multinames[t].runtimeargs+1);
							typestack.push_back(typestackentry(resulttype,false));
							break;
						}
						default:
							state.preloadedcode.push_back((uint32_t)opcode);
							clearOperands(state,true,&lastlocalresulttype);
							removetypestack(typestack,mi->context->constant_pool.multinames[t].runtimeargs+1);
							typestack.push_back(typestackentry(nullptr,false));
							break;
					}
				}
				else
#endif
				{
					state.preloadedcode.push_back((uint32_t)opcode);
					clearOperands(state,true,&lastlocalresulttype);
					removetypestack(typestack,mi->context->constant_pool.multinames[t].runtimeargs+1);
					typestack.push_back(typestackentry(nullptr,false));
				}
				if (addname)
				{
					state.preloadedcode.push_back(0);
					state.preloadedcode.back().pcode.local3.pos=t;
				}
				break;
			}
			case 0x73://convert_i
#ifdef ENABLE_OPTIMIZATION
				if (opcode == code.peekbyte() && state.jumptargets.find(code.tellg()+1) == state.jumptargets.end())
					break;
				if (state.operandlist.empty() && state.jumptargets.find(code.tellg()) == state.jumptargets.end() && typestack.size() > 0 && typestack.back().obj == Class<Integer>::getRef(mi->context->root->getSystemState()).getPtr())
					break;
				if (state.jumptargets.find(code.tellg()) == state.jumptargets.end() && state.operandlist.size() > 0
						&& state.operandlist.back().objtype == Class<Integer>::getRef(mi->context->root->getSystemState()).getPtr())
					state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
				else
#endif
					setupInstructionOneArgument(state,ABC_OP_OPTIMZED_CONVERTI,opcode,code,true,true,Class<Integer>::getRef(function->getSystemState()).getPtr(),code.tellg(),true,false,false,true,ABC_OP_OPTIMZED_CONVERTI_SETSLOT);
				removetypestack(typestack,1);
				typestack.push_back(typestackentry(Class<Integer>::getRef(mi->context->root->getSystemState()).getPtr(),false));
				break;
			case 0x74://convert_u
#ifdef ENABLE_OPTIMIZATION
				if (opcode == code.peekbyte() && state.jumptargets.find(code.tellg()+1) == state.jumptargets.end())
					break;
				if (state.operandlist.empty() && state.jumptargets.find(code.tellg()) == state.jumptargets.end() && typestack.size() > 0 && typestack.back().obj == Class<UInteger>::getRef(mi->context->root->getSystemState()).getPtr())
					break;
				if (state.jumptargets.find(code.tellg()) == state.jumptargets.end() && state.operandlist.size() > 0
						&& (state.operandlist.back().objtype == Class<UInteger>::getRef(mi->context->root->getSystemState()).getPtr()
							|| state.preloadedcode.back().opcode==0x24))//pushbyte
					state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
				else
#endif
					setupInstructionOneArgument(state,ABC_OP_OPTIMZED_CONVERTU,opcode,code,true,true,Class<UInteger>::getRef(function->getSystemState()).getPtr(),code.tellg(),true,false,false,true,ABC_OP_OPTIMZED_CONVERTU_SETSLOT);
				removetypestack(typestack,1);
				typestack.push_back(typestackentry(Class<UInteger>::getRef(mi->context->root->getSystemState()).getPtr(),false));
				break;
			case 0x75://convert_d
				state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
#ifdef ENABLE_OPTIMIZATION
				if (opcode == code.peekbyte() && state.jumptargets.find(code.tellg()+1) == state.jumptargets.end())
					break;
				if (state.jumptargets.find(code.tellg()) == state.jumptargets.end() && typestack.size() > 0 &&
						(typestack.back().obj == Class<Number>::getRef(mi->context->root->getSystemState()).getPtr() ||
						 typestack.back().obj == Class<Integer>::getRef(mi->context->root->getSystemState()).getPtr() ||
						 typestack.back().obj == Class<UInteger>::getRef(mi->context->root->getSystemState()).getPtr()))
					break;
				if (state.jumptargets.find(code.tellg()) == state.jumptargets.end() && state.operandlist.size() > 0 
						&& (state.operandlist.back().objtype == Class<Number>::getRef(mi->context->root->getSystemState()).getPtr()
							|| state.operandlist.back().objtype == Class<Integer>::getRef(mi->context->root->getSystemState()).getPtr()
							|| state.operandlist.back().objtype == Class<UInteger>::getRef(mi->context->root->getSystemState()).getPtr()))
					state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
				else
#endif
					setupInstructionOneArgument(state,ABC_OP_OPTIMZED_CONVERTD,opcode,code,true,true,Class<Number>::getRef(function->getSystemState()).getPtr(),code.tellg(),true,false,false,true,ABC_OP_OPTIMZED_CONVERTD_SETSLOT);
				removetypestack(typestack,1);
				typestack.push_back(typestackentry(Class<Number>::getRef(mi->context->root->getSystemState()).getPtr(),false));
				break;
			case 0x76://convert_b
#ifdef ENABLE_OPTIMIZATION
				if (opcode == code.peekbyte() && state.jumptargets.find(code.tellg()+1) == state.jumptargets.end())
					break;
				if (state.operandlist.empty() && state.jumptargets.find(code.tellg()) == state.jumptargets.end() && typestack.size() > 0 && typestack.back().obj == Class<Boolean>::getRef(mi->context->root->getSystemState()).getPtr())
					break;
				if ((state.jumptargets.find(code.tellg()) == state.jumptargets.end()
					 && state.operandlist.size() > 0 && state.operandlist.back().objtype == Class<Boolean>::getRef(mi->context->root->getSystemState()).getPtr()) ||
					((code.peekbyte() == 0x11 || //iftrue
					  code.peekbyte() == 0x12 || //iffalse
					  code.peekbyte() == 0x96    //not
					  )))
					state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
				else
#endif
					setupInstructionOneArgument(state,ABC_OP_OPTIMZED_CONVERTB,opcode,code,true,true,Class<Boolean>::getRef(function->getSystemState()).getPtr(),code.tellg(),true,false,false,true,ABC_OP_OPTIMZED_CONVERTB_SETSLOT);
				removetypestack(typestack,1);
				typestack.push_back(typestackentry(Class<Boolean>::getRef(mi->context->root->getSystemState()).getPtr(),false));
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
				setupInstructionOneArgument(state,ABC_OP_OPTIMZED_INCREMENT,opcode,code,false,true,Class<Number>::getRef(function->getSystemState()).getPtr(),code.tellg(),dup_indicator == 0);
				dup_indicator=0;
				removetypestack(typestack,1);
				typestack.push_back(typestackentry(Class<Number>::getRef(mi->context->root->getSystemState()).getPtr(),false));
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
				setupInstructionOneArgument(state,ABC_OP_OPTIMZED_DECREMENT,opcode,code,false,true,Class<Number>::getRef(function->getSystemState()).getPtr(),code.tellg(),dup_indicator == 0);
				dup_indicator=0;
				removetypestack(typestack,1);
				typestack.push_back(typestackentry(Class<Number>::getRef(mi->context->root->getSystemState()).getPtr(),false));
				break;
			case 0x95: //typeof
				setupInstructionOneArgument(state,ABC_OP_OPTIMZED_TYPEOF,opcode,code,true,true,Class<ASString>::getRef(function->getSystemState()).getPtr(),code.tellg(),true);
				removetypestack(typestack,1);
				typestack.push_back(typestackentry(Class<ASString>::getRef(mi->context->root->getSystemState()).getPtr(),false));
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
				typestack.push_back(typestackentry(Class<Boolean>::getRef(mi->context->root->getSystemState()).getPtr(),false));
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
						typestack.push_back(typestackentry(Class<Integer>::getRef(mi->context->root->getSystemState()).getPtr(),false));
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
				Class_base* resulttype = Class<Number>::getRef(state.mi->context->root->getSystemState()).getPtr();
				setupInstructionTwoArguments(state,ABC_OP_OPTIMZED_SUBTRACT,opcode,code,false,false,true,code.tellg(),resulttype,ABC_OP_OPTIMZED_SUBTRACT_SETSLOT);
				setForceInt(state,code,&resulttype);
				typestack.push_back(typestackentry(resulttype,false));
				break;
			}
			case 0xa2://multiply
			{
				removetypestack(typestack,2);
				Class_base* resulttype = Class<Number>::getRef(state.mi->context->root->getSystemState()).getPtr();
				setupInstructionTwoArguments(state,ABC_OP_OPTIMZED_MULTIPLY,opcode,code,false,false,true,code.tellg(),resulttype,ABC_OP_OPTIMZED_MULTIPLY_SETSLOT);
				setForceInt(state,code,&resulttype);
				typestack.push_back(typestackentry(resulttype,false));
				break;
			}
			case 0xa3://divide
			{
				removetypestack(typestack,2);
				Class_base* resulttype = Class<Number>::getRef(state.mi->context->root->getSystemState()).getPtr();
				setupInstructionTwoArguments(state,ABC_OP_OPTIMZED_DIVIDE,opcode,code,false,false,true,code.tellg(),resulttype,ABC_OP_OPTIMZED_DIVIDE_SETSLOT);
				setForceInt(state,code,&resulttype);
				typestack.push_back(typestackentry(resulttype,false));
				break;
			}
			case 0xa4://modulo
			{
				removetypestack(typestack,2);
				Class_base* resulttype = Class<Number>::getRef(state.mi->context->root->getSystemState()).getPtr();
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
				typestack.push_back(typestackentry(Class<Integer>::getRef(function->getSystemState()).getPtr(),false));
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
				setupInstructionComparison(state,ABC_OP_OPTIMZED_LESSTHAN,opcode,code,ABC_OP_OPTIMZED_IFLT,ABC_OP_OPTIMZED_IFGE,typestack,&lastlocalresulttype,jumppositions, jumpstartpositions);
				break;
			case 0xae://lessequals
				setupInstructionComparison(state,ABC_OP_OPTIMZED_LESSEQUALS,opcode,code,ABC_OP_OPTIMZED_IFLE,ABC_OP_OPTIMZED_IFGT,typestack,&lastlocalresulttype,jumppositions, jumpstartpositions);
				break;
			case 0xaf://greaterthan
				setupInstructionComparison(state,ABC_OP_OPTIMZED_GREATERTHAN,opcode,code,ABC_OP_OPTIMZED_IFGT,ABC_OP_OPTIMZED_IFLE,typestack,&lastlocalresulttype,jumppositions, jumpstartpositions);
				break;
			case 0xb0://greaterequals
				setupInstructionComparison(state,ABC_OP_OPTIMZED_GREATEREQUALS,opcode,code,ABC_OP_OPTIMZED_IFGE,ABC_OP_OPTIMZED_IFLT,typestack,&lastlocalresulttype,jumppositions, jumpstartpositions);
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
					typestack.push_back(typestackentry(Class<Integer>::getRef(mi->context->root->getSystemState()).getPtr(),false));
				}
				break;
			case 0x03://throw
				state.canlocalinitialize.clear();
				state.preloadedcode.push_back((uint32_t)opcode);
				state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
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
					state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
					clearOperands(state,true,&lastlocalresulttype);
				}
				removetypestack(typestack,1);
				break;
			case 0x2b://swap
				state.preloadedcode.push_back((uint32_t)opcode);
				state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
				clearOperands(state,true,&lastlocalresulttype);
				removetypestack(typestack,2);
				typestack.push_back(typestackentry(nullptr,false));
				typestack.push_back(typestackentry(nullptr,false));
				break;
			case 0x57://newactivation
			{
				state.preloadedcode.push_back((uint32_t)opcode);
				state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
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
					typestack.push_back(typestackentry(Class<Integer>::getRef(mi->context->root->getSystemState()).getPtr(),false));
				}
				break;
			case 0x97://bitnot
			case 0xc4://negate_i
				state.preloadedcode.push_back((uint32_t)opcode);
				state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
				clearOperands(state,true,&lastlocalresulttype);
				removetypestack(typestack,1);
				typestack.push_back(typestackentry(Class<Integer>::getRef(mi->context->root->getSystemState()).getPtr(),false));
				break;
			case 0xb3://istypelate
				setupInstructionTwoArguments(state,ABC_OP_OPTIMZED_ISTYPELATE,opcode,code,false,false,true,code.tellg(),Class<Boolean>::getRef(mi->context->root->getSystemState()).getPtr());
				removetypestack(typestack,2);
				typestack.push_back(typestackentry(Class<Boolean>::getRef(mi->context->root->getSystemState()).getPtr(),false));
				break;
			case 0xb1://instanceof
				setupInstructionTwoArguments(state,ABC_OP_OPTIMZED_INSTANCEOF,opcode,code,false,false,true,code.tellg(),Class<Boolean>::getRef(mi->context->root->getSystemState()).getPtr());
				removetypestack(typestack,2);
				typestack.push_back(typestackentry(Class<Boolean>::getRef(mi->context->root->getSystemState()).getPtr(),false));
				break;
			case 0xac://strictequals
			case 0xb4://in
				state.preloadedcode.push_back((uint32_t)opcode);
				state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
				clearOperands(state,true,&lastlocalresulttype);
				removetypestack(typestack,2);
				typestack.push_back(typestackentry(Class<Boolean>::getRef(mi->context->root->getSystemState()).getPtr(),false));
				break;
			case 0x1e://nextname
			case 0x23://nextvalue
				state.preloadedcode.push_back((uint32_t)opcode);
				state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
				clearOperands(state,true,&lastlocalresulttype);
				removetypestack(typestack,2);
				typestack.push_back(typestackentry(nullptr,false));
				break;
			case 0x77://convert_o
			case 0x78://checkfilter
				state.preloadedcode.push_back((uint32_t)opcode);
				state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
				clearOperands(state,true,&lastlocalresulttype);
				removetypestack(typestack,1);
				typestack.push_back(typestackentry(nullptr,false));
				break;
			case 0x70://convert_s
			case 0x85://coerce_s
				state.preloadedcode.push_back((uint32_t)opcode);
				state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
				clearOperands(state,true,&lastlocalresulttype);
				removetypestack(typestack,1);
				typestack.push_back(typestackentry(Class<ASString>::getRef(mi->context->root->getSystemState()).getPtr(),false));
				break;
			case 0x90://negate
				state.preloadedcode.push_back((uint32_t)opcode);
				state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
				clearOperands(state,true,&lastlocalresulttype);
				removetypestack(typestack,1);
				typestack.push_back(typestackentry(Class<Number>::getRef(mi->context->root->getSystemState()).getPtr(),false));
				break;
			case 0x71://esc_xelem
			case 0x72://esc_xattr
				state.preloadedcode.push_back((uint32_t)opcode);
				state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
				clearOperands(state,true,&lastlocalresulttype);
				removetypestack(typestack,1);
				typestack.push_back(typestackentry(nullptr,false));
				break;
			case 0xc7://multiply_i
				setupInstructionTwoArguments(state,ABC_OP_OPTIMZED_MULTIPLY_I,opcode,code,false,false,true,code.tellg(),nullptr,ABC_OP_OPTIMZED_MULTIPLY_I_SETSLOT);
				removetypestack(typestack,2);
				typestack.push_back(typestackentry(Class<Integer>::getRef(mi->context->root->getSystemState()).getPtr(),false));
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
				typestack.push_back(typestackentry(Class<Integer>::getRef(mi->context->root->getSystemState()).getPtr(),false));
				break;
			case 0x51://sxi8
				setupInstructionOneArgument(state,ABC_OP_OPTIMZED_SXI8,opcode,code,true,true,Class<Integer>::getRef(function->getSystemState()).getPtr(),code.tellg(),true);
				removetypestack(typestack,1);
				typestack.push_back(typestackentry(Class<Integer>::getRef(mi->context->root->getSystemState()).getPtr(),false));
				break;
			case 0x52://sxi16
				setupInstructionOneArgument(state,ABC_OP_OPTIMZED_SXI16,opcode,code,true,true,Class<Integer>::getRef(function->getSystemState()).getPtr(),code.tellg(),true);
				removetypestack(typestack,1);
				typestack.push_back(typestackentry(Class<Integer>::getRef(mi->context->root->getSystemState()).getPtr(),false));
				break;
			case 0xf3://timestamp
				state.preloadedcode.push_back((uint32_t)opcode);
				state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
				clearOperands(state,true,&lastlocalresulttype);
				break;
			case 0x07://dxnslate
				state.preloadedcode.push_back((uint32_t)opcode);
				state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
				clearOperands(state,true,&lastlocalresulttype);
				removetypestack(typestack,1);
				break;
			case 0x1f://hasnext
				state.preloadedcode.push_back((uint32_t)opcode);
				state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
				clearOperands(state,true,&lastlocalresulttype);
				removetypestack(typestack,1);
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
	state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size();
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
}

