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

#ifndef SCRIPTING_ABC_INTERPRETER_HELPER_H
#define SCRIPTING_ABC_INTERPRETER_HELPER_H 1

#include "scripting/abctypes.h"

// undef this to run abc code without any optimization
#define ENABLE_OPTIMIZATION

namespace lightspark
{
class SyntheticFunction;
class ASWorker;
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
	bool hasLocalResult;
	preloadedcodebuffer(uint32_t d=0):pcode(),opcode(d),
		operator_start(d),operator_setslot(UINT32_MAX),
		cachedslot1(false),cachedslot2(false),cachedslot3(false),
		hasLocalResult(false)
	{
	}
};
struct preloadstate
{
	std::vector<operands> operandlist;
	// map from position in original code to position in optimized code
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
	uint32_t lastlocalresultpos;
	bool duplocalresult;
	bool atexceptiontarget;
	preloadstate(SyntheticFunction* _f, ASWorker* _w);
	void refreshOldNewPosition(memorystream& code);
	void checkClearOperands(uint32_t p,Class_base** lastlocalresulttype);
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
struct typestackentry
{
	ASObject* obj;
	bool classvar;
	typestackentry(ASObject* o, bool c):obj(o),classvar(c){}
};

void skipjump(preloadstate& state,uint8_t& b,memorystream& code,uint32_t& pos,bool jumpInCode);
void clearOperands(preloadstate& state,bool resetlocaltypes,Class_base** lastlocalresulttype,bool checkchanged=false, OPERANDTYPES type = OP_CACHED_CONSTANT, int index=-1);
void removeOperands(preloadstate& state,bool resetlocaltypes,Class_base** lastlocalresulttype,uint32_t opcount);
void setOperandModified(preloadstate& state,OPERANDTYPES type, int index);
bool canCallFunctionDirect(operands& op,multiname* name, bool ignoreoverridden=false);
bool canCallFunctionDirect(ASObject* obj,multiname* name, bool ignoreoverridden);
void setForceInt(preloadstate& state,memorystream& code,Class_base** resulttype);
bool checkForLocalResult(preloadstate& state,memorystream& code,uint32_t opcode_jumpspace, Class_base* restype,int preloadpos=-1,int preloadlocalpos=-1, bool checkchanged=false,bool fromdup = false, uint32_t opcode_setslot=UINT32_MAX);
bool setupInstructionOneArgumentNoResult(preloadstate& state,int operator_start,int opcode,memorystream& code, uint32_t startcodepos);
bool setupInstructionTwoArgumentsNoResult(preloadstate& state,int operator_start,int opcode,memorystream& code);
bool setupInstructionOneArgument(preloadstate& state,int operator_start,int opcode,memorystream& code,bool constantsallowed, bool useargument_for_skip, Class_base* resulttype, uint32_t startcodepos, bool checkforlocalresult, bool addchanged=false,bool fromdup=false, bool checkoperands=true,uint32_t operator_start_setslot=UINT32_MAX);
bool setupInstructionTwoArguments(preloadstate& state,int operator_start,int opcode,memorystream& code, bool skip_conversion,bool cancollapse,bool checklocalresult, uint32_t startcodepos,Class_base* resulttype = nullptr,uint32_t operator_start_setslot=UINT32_MAX);
bool checkmatchingLastObjtype(preloadstate& state, Type* resulttype, Class_base* requiredtype);
void addOperand(preloadstate& state,operands& op,memorystream& code);
void addCachedConstant(preloadstate& state,method_info* mi, asAtom& val,memorystream& code);
void addCachedSlot(preloadstate& state, uint32_t localpos, uint32_t slotid,memorystream& code,Class_base* resulttype);
void setdefaultlocaltype(preloadstate& state,uint32_t t,Class_base* c);
void removetypestack(std::vector<typestackentry>& typestack,int n);
void skipunreachablecode(preloadstate& state, memorystream& code, bool updatetargets=true);
bool checkforpostfix(preloadstate& state,memorystream& code,uint32_t startpos,std::vector<typestackentry>& typestack, int32_t localpos, uint32_t postfix_opcode);
void setupInstructionComparison(preloadstate& state,int operator_start,int opcode,memorystream& code, int operator_replace,int operator_replace_reverse,std::vector<typestackentry>& typestack,Class_base** lastlocalresulttype,std::map<int32_t,int32_t>& jumppositions, std::map<int32_t,int32_t>& jumpstartpositions);
void setupInstructionIncDecInteger(preloadstate& state,memorystream& code,std::vector<typestackentry>& typestack,Class_base** lastlocalresulttype,int& dup_indicator, uint8_t opcode);
bool checkInitializeLocalToConstant(preloadstate& state,int32_t value);
void removeInitializeLocalToConstant(preloadstate& state,int32_t value);
bool getLexFindClass(preloadstate& state, multiname* name, bool checkfuncscope,std::vector<typestackentry>& typestack,memorystream& code);
void preloadFunction_secondPass(preloadstate& state);
void preload_dup(preloadstate& state, std::vector<typestackentry>& typestack, memorystream& code, int opcode, lightspark::Class_base** lastlocalresulttype,std::map<int32_t,int32_t>& jumppositions,std::map<int32_t,int32_t>& jumpstartpositions);
void preload_callprop(preloadstate& state,std::vector<typestackentry>& typestack,memorystream& code,int opcode,Class_base** lastlocalresulttype,uint8_t prevopcode);
void preload_getlex(preloadstate& state, std::vector<typestackentry>& typestack, memorystream& code, int opcode, lightspark::Class_base** lastlocalresulttype,std::list<scope_entry>& scopelist,uint32_t simple_getter_opcode_pos);
}
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
#define ABC_OP_OPTIMZED_NEXTVALUE 0x00000378
#define ABC_OP_OPTIMZED_GETSLOTFROMSCOPEOBJECT 0x00000382
#define ABC_OP_OPTIMZED_CONSTRUCTPROP_MULTIARGS 0x00000384
#define ABC_OP_OPTIMZED_NEXTNAME 0x00000388
#define ABC_OP_OPTIMZED_CALLBUILTINFUNCTION_NOARGS 0x00000390
#define ABC_OP_OPTIMZED_NEGATE 0x00000394
#define ABC_OP_OPTIMZED_NEGATE_SETSLOT 0x00000398
#define ABC_OP_OPTIMZED_NEGATE 0x00000394
#define ABC_OP_OPTIMZED_NEGATE_SETSLOT 0x00000398
#define ABC_OP_OPTIMZED_BITNOT 0x0000039a
#define ABC_OP_OPTIMZED_BITNOT_SETSLOT 0x0000039e
#define ABC_OP_OPTIMZED_CONVERTS 0x000003a0
#define ABC_OP_OPTIMZED_CONVERTS_SETSLOT 0x000003a4
#define ABC_OP_OPTIMZED_CALLPROPVOID_SLOTVAR_NOARGS 0x000003a6
#define ABC_OP_OPTIMZED_COERCES 0x000003a8
#define ABC_OP_OPTIMZED_COERCES_SETSLOT 0x000003ac
#define ABC_OP_OPTIMZED_CALLPROPVOID_SLOTVAR_MULTIARGS_CACHED_CALLER 0x000003ae
#define ABC_OP_OPTIMZED_CALLPROPVOID_SLOTVAR 0x000003b0
#define ABC_OP_OPTIMZED_CALLPROPERTY_SLOTVAR_NOARGS 0x000003b4
#define ABC_OP_OPTIMZED_CALLPROPERTY_SLOTVAR 0x000003b8
#define ABC_OP_OPTIMZED_CALLPROPERTY_SLOTVAR_MULTIARGS_CACHED_CALLER 0x000003c0
#define ABC_OP_OPTIMZED_CALLPROPVOID_BORROWEDSLOT_NOARGS 0x000003c4
#define ABC_OP_OPTIMZED_CALLPROPVOID_BORROWEDSLOT_MULTIARGS_CACHED_CALLER 0x000003c6
#define ABC_OP_OPTIMZED_CALLPROPERTY_BORROWEDSLOT 0x000003c8
#define ABC_OP_OPTIMZED_CALLPROPERTY_BORROWEDSLOT_NOARGS 0x000003d0
#define ABC_OP_OPTIMZED_CALLPROPVOID_BORROWEDSLOT 0x000003d4
#define ABC_OP_OPTIMZED_CALLPROPERTY_BORROWEDSLOT_MULTIARGS_CACHED_CALLER 0x000003d8

#endif /* SCRIPTING_ABC_INTERPRETER_HELPER_H */
