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

void ABCVm::executeFunction(const SyntheticFunction* function, call_context* context)
{
	method_info* mi=function->mi;

	const int code_len=mi->body->code.size();
	memorystream code(mi->body->code.data(), code_len,mi->body->codecache);
	//This may be non-zero and point to the position of an exception handler
	code.seekg(context->exec_pos);

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
	while(1)
	{
#ifdef PROFILING_SUPPORT
		uint32_t instructionPointer=code.tellg();
#endif
		uint8_t opcode = code.readbyte();
//LOG(LOG_INFO,"opcode:"<<hex<<(int)opcode);
		//Save ip for exception handling in SyntheticFunction::callImpl
		context->exec_pos = code.tellg();
		abcfunctions[opcode](context,code);
		
		if (context->returning)
		{
			PROF_ACCOUNT_TIME(mi->profTime[instructionPointer],profilingCheckpoint(startTime));
			return;
		}
		PROF_ACCOUNT_TIME(mi->profTime[instructionPointer],profilingCheckpoint(startTime));
	}

#undef PROF_ACCOUNT_TIME 
#undef PROF_IGNORE_TIME
	//We managed to execute all the function
	context->returning = true;
	RUNTIME_STACK_POP(context,context->returnvalue);
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
	abc_ifnlt,
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
	abc_invalidinstruction

};


void ABCVm::abc_bkpt(call_context* context,memorystream& code)
{
	//bkpt
	LOG_CALL( _("bkpt") );
}
void ABCVm::abc_nop(call_context* context,memorystream& code)
{
	//nop
}
void ABCVm::abc_throw(call_context* context,memorystream& code)
{
	//throw
	_throw(context);
}
void ABCVm::abc_getSuper(call_context* context,memorystream& code)
{
	//getsuper
	uint32_t t = code.readu30();
	getSuper(context,t);
}
void ABCVm::abc_setSuper(call_context* context,memorystream& code)
{
	//setsuper
	uint32_t t = code.readu30();
	setSuper(context,t);
}
void ABCVm::abc_dxns(call_context* context,memorystream& code)
{
	//dxns
	uint32_t t = code.readu30();
	dxns(context,t);
}
void ABCVm::abc_dxnslate(call_context* context,memorystream& code)
{
	//dxnslate
	RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v,context->context->root->getSystemState());
	dxnslate(context, v);
}
void ABCVm::abc_kill(call_context* context,memorystream& code)
{
	//kill
	uint32_t t = code.readu30();
	LOG_CALL( "kill " << t);
	ASATOM_DECREF(context->locals[t]);
	context->locals[t]=asAtom::undefinedAtom;
}
void ABCVm::abc_label(call_context* context,memorystream& code)
{
	//label
}
void ABCVm::abc_ifnlt(call_context* context,memorystream& code)
{
	//ifnlt
	int32_t t = code.reads24();
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POP_CREATE(context,v2);
	bool cond=!(v2.isLess(context->context->root->getSystemState(),v1) == TTRUE);
	ASATOM_DECREF(v2);
	ASATOM_DECREF(v1);
	LOG_CALL(_("ifNLT (") << ((cond)?_("taken)"):_("not taken)")));

	if(cond)
	{
		int here=code.tellg();
		uint32_t dest=here+t;

		//Now 'jump' to the destination, validate on the length
		if(dest >= code.size())
			throw ParseException("Jump out of bounds in interpreter");
		code.seekg(dest);
	}
}
void ABCVm::abc_ifnle(call_context* context,memorystream& code)
{
	//ifnle
	int32_t t = code.reads24();
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POP_CREATE(context,v2);
	bool cond=!(v1.isLess(context->context->root->getSystemState(),v2) == TFALSE);
	ASATOM_DECREF(v2);
	ASATOM_DECREF(v1);
	LOG_CALL(_("ifNLE (") << ((cond)?_("taken)"):_("not taken)")));

	if(cond)
	{
		int here=code.tellg();
		uint32_t dest=here+t;

		//Now 'jump' to the destination, validate on the length
		if(dest >= code.size())
			throw ParseException("Jump out of bounds in interpreter");
		code.seekg(dest);
	}
}
void ABCVm::abc_ifngt(call_context* context,memorystream& code)
{
	//ifngt
	int32_t t = code.reads24();
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POP_CREATE(context,v2);
	bool cond=!(v1.isLess(context->context->root->getSystemState(),v2) == TTRUE);
	ASATOM_DECREF(v2);
	ASATOM_DECREF(v1);
	LOG_CALL(_("ifNGT (") << ((cond)?_("taken)"):_("not taken)")));

	if(cond)
	{
		int here=code.tellg();
		uint32_t dest=here+t;

		//Now 'jump' to the destination, validate on the length
		if(dest >= code.size())
			throw ParseException("Jump out of bounds in interpreter");
		code.seekg(dest);
	}
}
void ABCVm::abc_ifnge(call_context* context,memorystream& code)
{
	//ifnge
	int32_t t = code.reads24();
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POP_CREATE(context,v2);
	bool cond=!(v2.isLess(context->context->root->getSystemState(),v1) == TFALSE);
	ASATOM_DECREF(v2);
	ASATOM_DECREF(v1);
	LOG_CALL(_("ifNGE (") << ((cond)?_("taken)"):_("not taken)")));

	if(cond)
	{
		int here=code.tellg();
		uint32_t dest=here+t;

		//Now 'jump' to the destination, validate on the length
		if(dest >= code.size())
			throw ParseException("Jump out of bounds in interpreter");
		code.seekg(dest);
	}
}
void ABCVm::abc_jump(call_context* context,memorystream& code)
{
	//jump
	int32_t t = code.reads24();

	int here=code.tellg();
	uint32_t dest=here+t;

	//Now 'jump' to the destination, validate on the length
	if(dest >= code.size())
		throw ParseException("Jump out of bounds in interpreter");
	code.seekg(dest);
}
void ABCVm::abc_iftrue(call_context* context,memorystream& code)
{
	//iftrue
	int32_t t = code.reads24();

	RUNTIME_STACK_POP_CREATE(context,v1);
	bool cond=v1.Boolean_concrete();
	LOG_CALL(_("ifTrue (") << ((cond)?_("taken)"):_("not taken)")));
	ASATOM_DECREF(v1);
	if(cond)
	{
		int here=code.tellg();
		uint32_t dest=here+t;

		//Now 'jump' to the destination, validate on the length
		if(dest >= code.size())
			throw ParseException("Jump out of bounds in interpreter");
		code.seekg(dest);
	}
}
void ABCVm::abc_iffalse(call_context* context,memorystream& code)
{
	//iffalse
	int32_t t = code.reads24();

	RUNTIME_STACK_POP_CREATE(context,v1);
	bool cond=!v1.Boolean_concrete();
	LOG_CALL(_("ifFalse (") << ((cond)?_("taken"):_("not taken")) << ')');
	ASATOM_DECREF(v1);
	if(cond)
	{
		int here=code.tellg();
		uint32_t dest=here+t;

		//Now 'jump' to the destination, validate on the length
		if(dest >= code.size())
			throw ParseException("Jump out of bounds in interpreter");
		code.seekg(dest);
	}
}
void ABCVm::abc_ifeq(call_context* context,memorystream& code)
{
	//ifeq
	int32_t t = code.reads24();

	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POP_CREATE(context,v2);
	bool cond=(v1.isEqual(context->context->root->getSystemState(),v2));
	LOG_CALL(_("ifEq (") << ((cond)?_("taken)"):_("not taken)")));
	ASATOM_DECREF(v2);
	ASATOM_DECREF(v1);
	if(cond)
	{
		int here=code.tellg();
		uint32_t dest=here+t;

		//Now 'jump' to the destination, validate on the length
		if(dest >= code.size())
			throw ParseException("Jump out of bounds in interpreter");
		code.seekg(dest);
	}
}
void ABCVm::abc_ifne(call_context* context,memorystream& code)
{
	//ifne
	int32_t t = code.reads24();

	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POP_CREATE(context,v2);
	bool cond=!(v1.isEqual(context->context->root->getSystemState(),v2));
	LOG_CALL(_("ifNE (") << ((cond)?_("taken)"):_("not taken)")));
	ASATOM_DECREF(v2);
	ASATOM_DECREF(v1);
	if(cond)
	{
		int here=code.tellg();
		uint32_t dest=here+t;

		//Now 'jump' to the destination, validate on the length
		if(dest >= code.size())
			throw ParseException("Jump out of bounds in interpreter");
		code.seekg(dest);
	}
}
void ABCVm::abc_iflt(call_context* context,memorystream& code)
{
	//iflt
	int32_t t = code.reads24();


	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POP_CREATE(context,v2);
	bool cond=v2.isLess(context->context->root->getSystemState(),v1) == TTRUE;
	ASATOM_DECREF(v2);
	ASATOM_DECREF(v1);
	LOG_CALL(_("ifLT (") << ((cond)?_("taken)"):_("not taken)")));

	if(cond)
	{
		int here=code.tellg();
		uint32_t dest=here+t;

		//Now 'jump' to the destination, validate on the length
		if(dest >= code.size())
			throw ParseException("Jump out of bounds in interpreter");
		code.seekg(dest);
	}
}
void ABCVm::abc_ifle(call_context* context,memorystream& code)
{
	//ifle
	int32_t t = code.reads24();


	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POP_CREATE(context,v2);
	bool cond=v1.isLess(context->context->root->getSystemState(),v2) == TFALSE;
	ASATOM_DECREF(v2);
	ASATOM_DECREF(v1);
	LOG_CALL(_("ifLE (") << ((cond)?_("taken)"):_("not taken)")));

	if(cond)
	{
		int here=code.tellg();
		uint32_t dest=here+t;

		//Now 'jump' to the destination, validate on the length
		if(dest >= code.size())
			throw ParseException("Jump out of bounds in interpreter");
		code.seekg(dest);
	}
}
void ABCVm::abc_ifgt(call_context* context,memorystream& code)
{
	//ifgt
	int32_t t = code.reads24();

	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POP_CREATE(context,v2);
	bool cond=v1.isLess(context->context->root->getSystemState(),v2) == TTRUE;
	ASATOM_DECREF(v2);
	ASATOM_DECREF(v1);
	LOG_CALL(_("ifGT (") << ((cond)?_("taken)"):_("not taken)")));

	if(cond)
	{
		int here=code.tellg();
		uint32_t dest=here+t;

		//Now 'jump' to the destination, validate on the length
		if(dest >= code.size())
			throw ParseException("Jump out of bounds in interpreter");
		code.seekg(dest);
	}
}
void ABCVm::abc_ifge(call_context* context,memorystream& code)
{
	//ifge
	int32_t t = code.reads24();

	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POP_CREATE(context,v2);
	bool cond=v2.isLess(context->context->root->getSystemState(),v1) == TFALSE;
	ASATOM_DECREF(v2);
	ASATOM_DECREF(v1);
	LOG_CALL(_("ifGE (") << ((cond)?_("taken)"):_("not taken)")));

	if(cond)
	{
		int here=code.tellg();
		uint32_t dest=here+t;

		//Now 'jump' to the destination, validate on the length
		if(dest >= code.size())
			throw ParseException("Jump out of bounds in interpreter");
		code.seekg(dest);
	}
}
void ABCVm::abc_ifstricteq(call_context* context,memorystream& code)
{
	//ifstricteq
	int32_t t = code.reads24();

	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POP_CREATE(context,v2);

	bool cond=v1.isEqualStrict(context->context->root->getSystemState(),v2);
	LOG_CALL(_("ifStrictEq ")<<cond);
	ASATOM_DECREF(v1);
	ASATOM_DECREF(v2);
	if(cond)
	{
		int here=code.tellg();
		uint32_t dest=here+t;

		//Now 'jump' to the destination, validate on the length
		if(dest >= code.size())
			throw ParseException("Jump out of bounds in interpreter");
		code.seekg(dest);
	}
}
void ABCVm::abc_ifstrictne(call_context* context,memorystream& code)
{
	//ifstrictne
	int32_t t = code.reads24();

	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POP_CREATE(context,v2);

	bool cond=!v1.isEqualStrict(context->context->root->getSystemState(),v2);
	LOG_CALL(_("ifStrictNE ")<<cond <<" "<<v1.toDebugString()<<" "<<v2.toDebugString());
	ASATOM_DECREF(v1);
	ASATOM_DECREF(v2);
	if(cond)
	{
		int here=code.tellg();
		uint32_t dest=here+t;

		//Now 'jump' to the destination, validate on the length
		if(dest >= code.size())
			throw ParseException("Jump out of bounds in interpreter");
		code.seekg(dest);
	}
}
void ABCVm::abc_lookupswitch(call_context* context,memorystream& code)
{
	//lookupswitch
	int here=int(code.tellg())-1; //Base for the jumps is the instruction itself for the switch
	int32_t t = code.reads24();
	uint32_t defaultdest=here+t;
	LOG_CALL(_("Switch default dest ") << defaultdest);
	uint32_t count = code.readu30();
	int32_t* offsets=g_newa(int32_t, count+1);
	for(unsigned int i=0;i<count+1;i++)
	{
		offsets[i] = code.reads24();
		LOG_CALL(_("Switch dest ") << i << ' ' << offsets[i]);
	}

	RUNTIME_STACK_POP_CREATE(context,index_obj);
	assert_and_throw(index_obj.type==T_INTEGER);
	unsigned int index=index_obj.toUInt();
	ASATOM_DECREF(index_obj);

	uint32_t dest=defaultdest;
	if(index<=count)
		dest=here+offsets[index];

	if(dest >= code.size())
		throw ParseException("Jump out of bounds in interpreter");
	code.seekg(dest);
}
void ABCVm::abc_pushwith(call_context* context,memorystream& code)
{
	//pushwith
	pushWith(context);
}
void ABCVm::abc_popscope(call_context* context,memorystream& code)
{
	//popscope
	popScope(context);
}
void ABCVm::abc_nextname(call_context* context,memorystream& code)
{
	//nextname
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	LOG_CALL("nextName");
	if(v1.type!=T_UINTEGER)
		throw UnsupportedException("Type mismatch in nextName");

	asAtom ret=pval->toObject(context->context->root->getSystemState())->nextName(v1.toUInt());
	ASATOM_DECREF_POINTER(pval);
	ASATOM_DECREF(v1);
	ASATOM_INCREF(ret);
	*pval = ret;
}
void ABCVm::abc_hasnext(call_context* context,memorystream& code)
{
	//hasnext
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	LOG_CALL("hasNext " << v1.toDebugString() << ' ' << pval->toDebugString());

	uint32_t curIndex=pval->toUInt();

	uint32_t newIndex=v1.toObject(context->context->root->getSystemState())->nextNameIndex(curIndex);
	pval->setInt(newIndex);
}
void ABCVm::abc_pushnull(call_context* context,memorystream& code)
{
	//pushnull
	LOG_CALL("pushnull");
	RUNTIME_STACK_PUSH(context,asAtom::nullAtom);
}
void ABCVm::abc_pushundefined(call_context* context,memorystream& code)
{
	LOG_CALL("pushundefined");
	RUNTIME_STACK_PUSH(context,asAtom::undefinedAtom);
}
void ABCVm::abc_nextvalue(call_context* context,memorystream& code)
{
	//nextvalue
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	
	if(v1.type!=T_UINTEGER)
		throw UnsupportedException("Type mismatch in nextValue");

	asAtom ret=pval->toObject(context->context->root->getSystemState())->nextValue(v1.toUInt());
	ASATOM_DECREF_POINTER(pval);
	ASATOM_DECREF(v1);
	ASATOM_INCREF(ret);
	*pval=ret;
}
void ABCVm::abc_pushbyte(call_context* context,memorystream& code)
{
	//pushbyte
	int8_t t = code.readbyte();
	LOG_CALL("pushbyte "<<(int)t);
	RUNTIME_STACK_PUSH(context,asAtom((int32_t)t));
}
void ABCVm::abc_pushshort(call_context* context,memorystream& code)
{
	//pushshort
	// specs say pushshort is a u30, but it's really a u32
	// see https://bugs.adobe.com/jira/browse/ASC-4181
	uint32_t t = code.readu32();
	LOG_CALL("pushshort "<<t);

	RUNTIME_STACK_PUSH(context,asAtom((int32_t)t));
}
void ABCVm::abc_pushtrue(call_context* context,memorystream& code)
{
	//pushtrue
	LOG_CALL("pushtrue");
	RUNTIME_STACK_PUSH(context,asAtom::trueAtom);
}
void ABCVm::abc_pushfalse(call_context* context,memorystream& code)
{
	//pushfalse
	LOG_CALL("pushfalse");
	RUNTIME_STACK_PUSH(context,asAtom::falseAtom);
}
void ABCVm::abc_pushnan(call_context* context,memorystream& code)
{
	//pushnan
	LOG_CALL("pushNaN");
	RUNTIME_STACK_PUSH(context,asAtom(Number::NaN));
}
void ABCVm::abc_pop(call_context* context,memorystream& code)
{
	//pop
	pop();
	RUNTIME_STACK_POP_CREATE(context,o);
	ASATOM_DECREF(o);
}
void ABCVm::abc_dup(call_context* context,memorystream& code)
{
	//dup
	dup();
	RUNTIME_STACK_PEEK_CREATE(context,o);
	ASATOM_INCREF(o);
	RUNTIME_STACK_PUSH(context,o);
}
void ABCVm::abc_swap(call_context* context,memorystream& code)
{
	//swap
	swap();
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POP_CREATE(context,v2);

	RUNTIME_STACK_PUSH(context,v1);
	RUNTIME_STACK_PUSH(context,v2);
}
void ABCVm::abc_pushstring(call_context* context,memorystream& code)
{
	//pushstring
	uint32_t t = code.readu30();
	RUNTIME_STACK_PUSH(context,asAtom::fromObject(pushString(context,t)));
}
void ABCVm::abc_pushint(call_context* context,memorystream& code)
{
	//pushint
	uint32_t t = code.readu30();
	s32 val=context->context->constant_pool.integer[t];
	pushInt(context, val);

	RUNTIME_STACK_PUSH(context,asAtom(val));
}
void ABCVm::abc_pushuint(call_context* context,memorystream& code)
{
	//pushuint
	uint32_t t = code.readu30();
	u32 val=context->context->constant_pool.uinteger[t];
	pushUInt(context, val);

	RUNTIME_STACK_PUSH(context,asAtom(val));
}
void ABCVm::abc_pushdouble(call_context* context,memorystream& code)
{
	//pushdouble
	uint32_t t = code.readu30();
	d64 val=context->context->constant_pool.doubles[t];
	pushDouble(context, val);

	RUNTIME_STACK_PUSH(context,asAtom(val));
}
void ABCVm::abc_pushScope(call_context* context,memorystream& code)
{
	//pushscope
	pushScope(context);
}
void ABCVm::abc_pushnamespace(call_context* context,memorystream& code)
{
	//pushnamespace
	uint32_t t = code.readu30();
	RUNTIME_STACK_PUSH(context,asAtom::fromObject(pushNamespace(context, t) ));
}
void ABCVm::abc_hasnext2(call_context* context,memorystream& code)
{
	//hasnext2
	uint32_t t = code.readu30();
	uint32_t t2 = code.readu30();

	bool ret=hasNext2(context,t,t2);
	RUNTIME_STACK_PUSH(context,asAtom(ret));
}
//Alchemy opcodes
void ABCVm::abc_li8(call_context* context,memorystream& code)
{
	//li8
	LOG_CALL( "li8");
	loadIntN<uint8_t>(context);
}
void ABCVm::abc_li16(call_context* context,memorystream& code)
{
	//li16
	LOG_CALL( "li16");
	loadIntN<uint16_t>(context);
}
void ABCVm::abc_li32(call_context* context,memorystream& code)
{
	//li32
	LOG_CALL( "li32");
	loadIntN<int32_t>(context);
}
void ABCVm::abc_lf32(call_context* context,memorystream& code)
{
	//lf32
	LOG_CALL( "lf32");
	loadFloat(context);
}
void ABCVm::abc_lf64(call_context* context,memorystream& code)
{
	//lf64
	LOG_CALL( "lf64");
	loadDouble(context);
}
void ABCVm::abc_si8(call_context* context,memorystream& code)
{
	//si8
	LOG_CALL( "si8");
	storeIntN<uint8_t>(context);
}
void ABCVm::abc_si16(call_context* context,memorystream& code)
{
	//si16
	LOG_CALL( "si16");
	storeIntN<uint16_t>(context);
}
void ABCVm::abc_si32(call_context* context,memorystream& code)
{
	//si32
	LOG_CALL( "si32");
	storeIntN<uint32_t>(context);
}
void ABCVm::abc_sf32(call_context* context,memorystream& code)
{
	//sf32
	LOG_CALL( "sf32");
	storeFloat(context);
}
void ABCVm::abc_sf64(call_context* context,memorystream& code)
{
	//sf64
	LOG_CALL( "sf64");
	storeDouble(context);
}
void ABCVm::abc_newfunction(call_context* context,memorystream& code)
{
	//newfunction
	uint32_t t = code.readu30();
	RUNTIME_STACK_PUSH(context,asAtom::fromObject(newFunction(context,t)));
}
void ABCVm::abc_call(call_context* context,memorystream& code)
{
	//call
	uint32_t t = code.readu30();
	method_info* called_mi=NULL;
	call(context,t,&called_mi);
}
void ABCVm::abc_construct(call_context* context,memorystream& code)
{
	//construct
	uint32_t t = code.readu30();
	construct(context,t);
}
void ABCVm::abc_callMethod(call_context* context,memorystream& code)
{
	// callmethod
	LOG(LOG_NOT_IMPLEMENTED,"callmethod behaves exactly like callproperty");
	uint32_t t = code.readu30();
	uint32_t t2 = code.readu30();
	callMethod(context,t,t2);
}
void ABCVm::abc_callstatic(call_context* context,memorystream& code)
{
	//callstatic
	uint32_t t = code.readu30();
	uint32_t t2 = code.readu30();
	callStatic(context,t,t2,NULL,true);
}
void ABCVm::abc_callsuper(call_context* context,memorystream& code)
{
	//callsuper
	uint32_t t = code.readu30();
	uint32_t t2 = code.readu30();
	callSuper(context,t,t2,NULL,true);
}
void ABCVm::abc_callproperty(call_context* context,memorystream& code)
{
	//callproperty
	uint32_t t = code.readu30();
	uint32_t t2 = code.readu30();
	callProperty(context,t,t2,NULL,true);
}
void ABCVm::abc_returnvoid(call_context* context,memorystream& code)
{
	//returnvoid
	LOG_CALL(_("returnVoid"));
	context->returnvalue = asAtom::invalidAtom;
	context->returning = true;
}
void ABCVm::abc_returnvalue(call_context* context,memorystream& code)
{
	//returnvalue
	RUNTIME_STACK_POP(context,context->returnvalue);
	LOG_CALL(_("returnValue ") << context->returnvalue.toDebugString());
	context->returning = true;
}
void ABCVm::abc_constructsuper(call_context* context,memorystream& code)
{
	//constructsuper
	uint32_t t = code.readu30();
	constructSuper(context,t);
}
void ABCVm::abc_constructprop(call_context* context,memorystream& code)
{
	//constructprop
	uint32_t t = code.readu30();
	uint32_t t2 = code.readu30();
	constructProp(context,t,t2);
}
void ABCVm::abc_callproplex(call_context* context,memorystream& code)
{
	//callproplex
	LOG(LOG_NOT_IMPLEMENTED,"callproplex behaves exactly like callproperty");
	uint32_t t = code.readu30();
	uint32_t t2 = code.readu30();
	callProperty(context,t,t2,NULL,true);
}
void ABCVm::abc_callsupervoid(call_context* context,memorystream& code)
{
	//callsupervoid
	uint32_t t = code.readu30();
	uint32_t t2 = code.readu30();
	method_info* called_mi=NULL;
	callSuper(context,t,t2,&called_mi,false);
}
void ABCVm::abc_callpropvoid(call_context* context,memorystream& code)
{
	//callpropvoid
	uint32_t t = code.readu30();
	uint32_t t2 = code.readu30();
	callProperty(context,t,t2,NULL,false);
}
void ABCVm::abc_sxi1(call_context* context,memorystream& code)
{
	//sxi1
	LOG_CALL( "sxi1");
	RUNTIME_STACK_POINTER_CREATE(context,arg1);
	int32_t ret=arg1->toUInt()&0x1 ? -1 : 0;
	ASATOM_DECREF_POINTER(arg1);
	arg1->setInt(ret);
}
void ABCVm::abc_sxi8(call_context* context,memorystream& code)
{
	//sxi8
	LOG_CALL( "sxi8");
	RUNTIME_STACK_POINTER_CREATE(context,arg1);
	int32_t ret=(int8_t)arg1->toUInt();
	ASATOM_DECREF_POINTER(arg1);
	arg1->setInt(ret);
}
void ABCVm::abc_sxi16(call_context* context,memorystream& code)
{
	//sxi16
	LOG_CALL( "sxi16");
	RUNTIME_STACK_POINTER_CREATE(context,arg1);
	int32_t ret=(int16_t)arg1->toUInt();
	ASATOM_DECREF_POINTER(arg1);
	arg1->setInt(ret);
}
void ABCVm::abc_constructgenerictype(call_context* context,memorystream& code)
{
	//constructgenerictype
	uint32_t t = code.readu30();
	constructGenericType(context, t);
}
void ABCVm::abc_newobject(call_context* context,memorystream& code)
{
	//newobject
	uint32_t t = code.readu30();
	newObject(context,t);
}
void ABCVm::abc_newarray(call_context* context,memorystream& code)
{
	//newarray
	uint32_t t = code.readu30();
	newArray(context,t);
}
void ABCVm::abc_newactivation(call_context* context,memorystream& code)
{
	//newactivation
	RUNTIME_STACK_PUSH(context,asAtom::fromObject(newActivation(context, context->mi)));
}
void ABCVm::abc_newclass(call_context* context,memorystream& code)
{
	//newclass
	uint32_t t = code.readu30();
	newClass(context,t);
}
void ABCVm::abc_getdescendants(call_context* context,memorystream& code)
{
	//getdescendants
	uint32_t t = code.readu30();
	getDescendants(context, t);
}
void ABCVm::abc_newcatch(call_context* context,memorystream& code)
{
	//newcatch
	uint32_t t = code.readu30();
	RUNTIME_STACK_PUSH(context,asAtom::fromObject(newCatch(context,t)));
}
void ABCVm::abc_findpropstrict(call_context* context,memorystream& code)
{
	//findpropstrict
//		uint32_t t = code.readu30();
//		multiname* name=context->context->getMultiname(t,context);
//		RUNTIME_STACK_PUSH(context,asAtom::fromObject(findPropStrict(context,name)));
//		name->resetNameIfObject();

		RUNTIME_STACK_PUSH(context,findPropStrictCache(context,code));
}
void ABCVm::abc_findproperty(call_context* context,memorystream& code)
{
	//findproperty
	uint32_t t = code.readu30();
	multiname* name=context->context->getMultiname(t,context);
	RUNTIME_STACK_PUSH(context,asAtom::fromObject(findProperty(context,name)));
	name->resetNameIfObject();
}
void ABCVm::abc_finddef(call_context* context,memorystream& code)
{
	//finddef
	uint32_t t = code.readu30();
	multiname* name=context->context->getMultiname(t,context);
	LOG(LOG_NOT_IMPLEMENTED,"opcode 0x5f (finddef) not implemented:"<<*name);
	RUNTIME_STACK_PUSH(context,asAtom::nullAtom);
	name->resetNameIfObject();
}
void ABCVm::abc_getlex(call_context* context,memorystream& code)
{
	//getlex
	lightspark::method_body_info_cache* cachepos = code.tellcachepos();
	if (cachepos->type == method_body_info_cache::CACHE_TYPE_OBJECT)
	{
		code.seekcachepos(cachepos->nextcachepos);
		RUNTIME_STACK_PUSH(context,asAtom::fromFunction(cachepos->obj,cachepos->closure));
		cachepos->obj->incRef();
		return;
	}
	uint32_t t = code.readu30();
	if (getLex(context,t))
	{
		// put object in cache
		cachepos->type =method_body_info_cache::CACHE_TYPE_OBJECT;
		RUNTIME_STACK_PEEK_CREATE(context,v);
		
		cachepos->obj = v.getObject();
		cachepos->closure = v.getClosure();
		cachepos->obj->incRef();
	}
}
void ABCVm::abc_setproperty(call_context* context,memorystream& code)
{
	//setproperty
	uint32_t t = code.readu30();
	RUNTIME_STACK_POP_CREATE(context,value);

	multiname* name=context->context->getMultiname(t,context);

	RUNTIME_STACK_POP_CREATE(context,obj)

	LOG_CALL(_("setProperty ") << *name << ' ' << obj.toDebugString()<<" " <<value.toDebugString());

	if(obj.type == T_NULL)
	{
		LOG(LOG_ERROR,"calling setProperty on null:" << *name << ' ' << obj.toDebugString()<<" " << value.toDebugString());
		throwError<TypeError>(kConvertNullToObjectError);
	}
	if (obj.type == T_UNDEFINED)
	{
		LOG(LOG_ERROR,"calling setProperty on undefined:" << *name << ' ' << obj.toDebugString()<<" " << value.toDebugString());
		throwError<TypeError>(kConvertUndefinedToObjectError);
	}
	//Do not allow to set contant traits
	ASObject* o = obj.toObject(context->context->root->getSystemState());
	o->setVariableByMultiname(*name,value,ASObject::CONST_NOT_ALLOWED);

	name->resetNameIfObject();
}
void ABCVm::abc_getlocal(call_context* context,memorystream& code)
{
	//getlocal
	uint32_t i = code.readu30();
	ASATOM_INCREF(context->locals[i]);
	LOG_CALL( _("getLocal ") << i << _(": ") << context->locals[i].toDebugString() );
	RUNTIME_STACK_PUSH(context,context->locals[i]);
}
void ABCVm::abc_setlocal(call_context* context,memorystream& code)
{
	//setlocal
	uint32_t i = code.readu30();
	LOG_CALL( _("setLocal ") << i );
	RUNTIME_STACK_POP_CREATE(context,obj)
	if (i >= context->locals_size)
	{
		LOG(LOG_ERROR,"abc_setlocal invalid index:"<<i);
		return;
	}
	if ((int)i != context->argarrayposition || obj.type == T_ARRAY)
	{
		ASATOM_DECREF(context->locals[i]);
		context->locals[i]=obj;
	}
}
void ABCVm::abc_getglobalscope(call_context* context,memorystream& code)
{
	//getglobalscope
	RUNTIME_STACK_PUSH(context,asAtom::fromObject(getGlobalScope(context)));
}
void ABCVm::abc_getscopeobject(call_context* context,memorystream& code)
{
	//getscopeobject
	uint32_t t = code.readu30();
	assert_and_throw(context->curr_scope_stack > t);
	asAtom ret=context->scope_stack[t];
	ASATOM_INCREF(ret);
	LOG_CALL( _("getScopeObject: ") << ret.toDebugString());
	
	RUNTIME_STACK_PUSH(context,ret);
}
void ABCVm::abc_getProperty(call_context* context,memorystream& code)
{
	uint32_t t = code.readu30();
	multiname* name=context->context->getMultiname(t,context);


	RUNTIME_STACK_POP_CREATE_ASOBJECT(context,obj, context->context->root->getSystemState());

	LOG_CALL( _("getProperty ") << *name << ' ' << obj->toDebugString() << ' '<<obj->isInitialized());
	checkDeclaredTraits(obj);


	asAtom prop=obj->getVariableByMultiname(*name);
	if(prop.type == T_INVALID)
	{
		if (obj->getClass() && obj->getClass()->isSealed)
			throwError<ReferenceError>(kReadSealedError, name->normalizedNameUnresolved(obj->getSystemState()), obj->getClass()->getQualifiedClassName());
		if (name->isEmpty() || !name->hasEmptyNS)
			throwError<ReferenceError>(kReadSealedErrorNs, name->normalizedNameUnresolved(obj->getSystemState()), obj->getClassName());
		if (obj->is<Undefined>())
			throwError<TypeError>(kConvertUndefinedToObjectError);
		if (Log::getLevel() >= LOG_NOT_IMPLEMENTED && (!obj->getClass() || obj->getClass()->isSealed))
			LOG(LOG_NOT_IMPLEMENTED,"getProperty: " << name->normalizedNameUnresolved(context->context->root->getSystemState()) << " not found on " << obj->toDebugString() << " "<<obj->getClassName());
		prop = asAtom::undefinedAtom;
	}
	else
	{
		ASATOM_INCREF(prop);
	}
	obj->decRef();
	name->resetNameIfObject();

	RUNTIME_STACK_PUSH(context,prop);
}
void ABCVm::abc_initproperty(call_context* context,memorystream& code)
{
	//initproperty
	uint32_t t = code.readu30();
	RUNTIME_STACK_POP_CREATE(context,value);
	multiname* name=context->context->getMultiname(t,context);
	LOG_CALL("initProperty "<<*name);
	RUNTIME_STACK_POP_CREATE(context,obj);
	checkDeclaredTraits(obj.toObject(context->context->root->getSystemState()));
	obj.toObject(context->context->root->getSystemState())->setVariableByMultiname(*name,value,ASObject::CONST_ALLOWED);
	ASATOM_DECREF(obj);
	name->resetNameIfObject();
}
void ABCVm::abc_deleteproperty(call_context* context,memorystream& code)
{
	//deleteproperty
	uint32_t t = code.readu30();
	multiname* name = context->context->getMultiname(t,context);
	RUNTIME_STACK_POP_CREATE_ASOBJECT(context,obj, context->context->root->getSystemState());
	bool ret = deleteProperty(obj,name);
	name->resetNameIfObject();
	RUNTIME_STACK_PUSH(context,asAtom(ret));
}
void ABCVm::abc_getslot(call_context* context,memorystream& code)
{
	//getslot
	uint32_t t = code.readu30();
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	
	asAtom ret=pval->toObject(context->context->root->getSystemState())->getSlot(t);
	LOG_CALL("getSlot " << t << " " << ret.toDebugString());
	//getSlot can only access properties defined in the current
	//script, so they should already be defind by this script
	ASATOM_INCREF(ret);

	*pval=ret;
}
void ABCVm::abc_setslot(call_context* context,memorystream& code)
{
	//setslot
	uint32_t t = code.readu30();
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v2, context->context->root->getSystemState());

	LOG_CALL("setSlot " << t << " "<< v2->toDebugString() << " "<< v1.toDebugString());
	v2->setSlot(t,v1);
	v2->decRef();
}
void ABCVm::abc_getglobalSlot(call_context* context,memorystream& code)
{
	//getglobalSlot
	uint32_t t = code.readu30();

	Global* globalscope = getGlobalScope(context);
	RUNTIME_STACK_PUSH(context,globalscope->getSlot(t));
}
void ABCVm::abc_setglobalSlot(call_context* context,memorystream& code)
{
	//setglobalSlot
	uint32_t t = code.readu30();

	Global* globalscope = getGlobalScope(context);
	RUNTIME_STACK_POP_CREATE(context,o);
	globalscope->setSlot(t,o);
}
void ABCVm::abc_convert_s(call_context* context,memorystream& code)
{
	//convert_s
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	pval->replace(convert_s(pval->toObject(context->context->root->getSystemState())));
}
void ABCVm::abc_esc_xelem(call_context* context,memorystream& code)
{
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	pval->replace(esc_xelem(pval->toObject(context->context->root->getSystemState())));
}
void ABCVm::abc_esc_xattr(call_context* context,memorystream& code)
{
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	pval->replace(esc_xattr(pval->toObject(context->context->root->getSystemState())));
}
void ABCVm::abc_convert_i(call_context* context,memorystream& code)
{
	//convert_i
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	LOG_CALL("convert_i");
	pval->convert_i();
}
void ABCVm::abc_convert_u(call_context* context,memorystream& code)
{
	//convert_u
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	LOG_CALL("convert_u");
	pval->convert_u();
}
void ABCVm::abc_convert_d(call_context* context,memorystream& code)
{
	//convert_d
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	LOG_CALL("convert_d");
	pval->convert_d();
}
void ABCVm::abc_convert_b(call_context* context,memorystream& code)
{
	//convert_b
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	LOG_CALL("convert_b");
	pval->convert_b();
}
void ABCVm::abc_convert_o(call_context* context,memorystream& code)
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
}
void ABCVm::abc_checkfilter(call_context* context,memorystream& code)
{
	//checkfilter
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	checkfilter(pval->toObject(context->context->root->getSystemState()));
}
void ABCVm::abc_coerce(call_context* context,memorystream& code)
{
	//coerce
	uint32_t t = code.readu30();
	coerce(context, t);
}
void ABCVm::abc_coerce_a(call_context* context,memorystream& code)
{
	//coerce_a
	coerce_a();
}
void ABCVm::abc_coerce_s(call_context* context,memorystream& code)
{
	//coerce_s
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	if (pval->type != T_STRING)
		pval->replace(coerce_s(pval->toObject(context->context->root->getSystemState())));
}
void ABCVm::abc_astype(call_context* context,memorystream& code)
{
	//astype
	uint32_t t = code.readu30();
	multiname* name=context->context->getMultiname(t,NULL);

	RUNTIME_STACK_POINTER_CREATE(context,pval);
	pval->replace(asType(context->context, pval->toObject(context->context->root->getSystemState()), name));
}
void ABCVm::abc_astypelate(call_context* context,memorystream& code)
{
	//astypelate
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	*pval = pval->asTypelate(v1);
}
void ABCVm::abc_negate(call_context* context,memorystream& code)
{
	//negate
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	number_t ret=-(pval->toNumber());
	ASATOM_DECREF_POINTER(pval);
	pval->setNumber(ret);
}
void ABCVm::abc_increment(call_context* context,memorystream& code)
{
	//increment
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	pval->increment();
}
void ABCVm::abc_inclocal(call_context* context,memorystream& code)
{
	//inclocal
	uint32_t t = code.readu30();
	incLocal(context, t);
}
void ABCVm::abc_decrement(call_context* context,memorystream& code)
{
	//decrement
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	pval->decrement();
}
void ABCVm::abc_declocal(call_context* context,memorystream& code)
{
	//declocal
	uint32_t t = code.readu30();
	decLocal(context, t);
}
void ABCVm::abc_typeof(call_context* context,memorystream& code)
{
	//typeof
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	LOG_CALL(_("typeOf"));
	asAtom ret = pval->typeOf(context->context->root->getSystemState());
	ASATOM_DECREF_POINTER(pval);
	*pval = ret;
}
void ABCVm::abc_not(call_context* context,memorystream& code)
{
	//not
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	pval->_not();
}
void ABCVm::abc_bitnot(call_context* context,memorystream& code)
{
	//bitnot
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	pval->bitnot();
}
void ABCVm::abc_add(call_context* context,memorystream& code)
{
	//add
	RUNTIME_STACK_POP_CREATE(context,v2);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	pval->add(v2,context->context->root->getSystemState());
}
void ABCVm::abc_subtract(call_context* context,memorystream& code)
{
	//subtract
	//Be careful, operands in subtract implementation are swapped
	RUNTIME_STACK_POP_CREATE(context,v2);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	pval->subtract(v2);
}
void ABCVm::abc_multiply(call_context* context,memorystream& code)
{
	//multiply
	RUNTIME_STACK_POP_CREATE(context,v2);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	pval->multiply(v2);
}
void ABCVm::abc_divide(call_context* context,memorystream& code)
{
	//divide
	RUNTIME_STACK_POP_CREATE(context,v2);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	pval->divide(v2);
}
void ABCVm::abc_modulo(call_context* context,memorystream& code)
{
	//modulo
	RUNTIME_STACK_POP_CREATE(context,v2);
	RUNTIME_STACK_POINTER_CREATE(context,pval);

	pval->modulo(v2);
}
void ABCVm::abc_lshift(call_context* context,memorystream& code)
{
	//lshift
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	pval->lshift(v1);
}
void ABCVm::abc_rshift(call_context* context,memorystream& code)
{
	//rshift
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	pval->rshift(v1);
}
void ABCVm::abc_urshift(call_context* context,memorystream& code)
{
	//urshift
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	pval->urshift(v1);
}
void ABCVm::abc_bitand(call_context* context,memorystream& code)
{
	//bitand
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	pval->bit_and(v1);
}
void ABCVm::abc_bitor(call_context* context,memorystream& code)
{
	//bitor
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	pval->bit_or(v1);
}
void ABCVm::abc_bitxor(call_context* context,memorystream& code)
{
	//bitxor
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	pval->bit_xor(v1);
}
void ABCVm::abc_equals(call_context* context,memorystream& code)
{
	//equals
	RUNTIME_STACK_POP_CREATE(context,v2);
	RUNTIME_STACK_POINTER_CREATE(context,pval);

	bool ret=pval->isEqual(context->context->root->getSystemState(),v2);
	LOG_CALL( _("equals ") << ret);
	ASATOM_DECREF_POINTER(pval);
	ASATOM_DECREF(v2);
	pval->setBool(ret);
}
void ABCVm::abc_strictequals(call_context* context,memorystream& code)
{
	//strictequals
	RUNTIME_STACK_POP_CREATE(context,v2);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	bool ret = pval->isEqualStrict(context->context->root->getSystemState(),v2);
	LOG_CALL( _("strictequals ") << ret);
	ASATOM_DECREF_POINTER(pval);
	ASATOM_DECREF(v2);
	pval->setBool(ret);
}
void ABCVm::abc_lessthan(call_context* context,memorystream& code)
{
	//lessthan
	RUNTIME_STACK_POP_CREATE(context,v2);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	//Real comparision demanded to object
	bool ret=(pval->isLess(context->context->root->getSystemState(),v2)==TTRUE);
	LOG_CALL(_("lessThan ")<<ret);
	ASATOM_DECREF_POINTER(pval);
	ASATOM_DECREF(v2);

	pval->setBool(ret);
}
void ABCVm::abc_lessequals(call_context* context,memorystream& code)
{
	//lessequals
	RUNTIME_STACK_POP_CREATE(context,v2);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	//Real comparision demanded to object
	bool ret=(v2.isLess(context->context->root->getSystemState(),*pval)==TFALSE);
	LOG_CALL(_("lessEquals ")<<ret);
	ASATOM_DECREF_POINTER(pval);
	ASATOM_DECREF(v2);

	pval->setBool(ret);
}
void ABCVm::abc_greaterthan(call_context* context,memorystream& code)
{
	//greaterthan
	RUNTIME_STACK_POP_CREATE(context,v2);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	//Real comparision demanded to object
	bool ret=(v2.isLess(context->context->root->getSystemState(),*pval)==TTRUE);
	LOG_CALL(_("greaterThan ")<<ret);
	ASATOM_DECREF_POINTER(pval);
	ASATOM_DECREF(v2);

	pval->setBool(ret);
}
void ABCVm::abc_greaterequals(call_context* context,memorystream& code)
{
	//greaterequals
	RUNTIME_STACK_POP_CREATE(context,v2);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	//Real comparision demanded to object
	bool ret=(pval->isLess(context->context->root->getSystemState(),v2)==TFALSE);
	LOG_CALL(_("greaterEquals ")<<ret);
	ASATOM_DECREF_POINTER(pval);
	ASATOM_DECREF(v2);

	pval->setBool(ret);
}
void ABCVm::abc_instanceof(call_context* context,memorystream& code)
{
	//instanceof
	RUNTIME_STACK_POP_CREATE_ASOBJECT(context,type, context->context->root->getSystemState());
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	pval->setBool(instanceOf(pval->toObject(context->context->root->getSystemState()),type));
}
void ABCVm::abc_istype(call_context* context,memorystream& code)
{
	//istype
	uint32_t t = code.readu30();
	multiname* name=context->context->getMultiname(t,NULL);

	RUNTIME_STACK_POINTER_CREATE(context,pval);

	pval->setBool(isType(context->context,pval->toObject(context->context->root->getSystemState()),name));
}
void ABCVm::abc_istypelate(call_context* context,memorystream& code)
{
	//istypelate
	RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v1, context->context->root->getSystemState());
	RUNTIME_STACK_POINTER_CREATE(context,pval);

	pval->setBool(isTypelate(v1, pval->toObject(context->context->root->getSystemState())));
}
void ABCVm::abc_in(call_context* context,memorystream& code)
{
	//in
	RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v1, context->context->root->getSystemState());
	RUNTIME_STACK_POINTER_CREATE(context,pval);

	pval->setBool(in(v1,pval->toObject(context->context->root->getSystemState())));
}
void ABCVm::abc_increment_i(call_context* context,memorystream& code)
{
	//increment_i
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	pval->increment_i();
}
void ABCVm::abc_decrement_i(call_context* context,memorystream& code)
{
	//decrement_i
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	pval->decrement_i();
}
void ABCVm::abc_inclocal_i(call_context* context,memorystream& code)
{
	//inclocal_i
	uint32_t t = code.readu30();
	incLocal_i(context, t);
}
void ABCVm::abc_declocal_i(call_context* context,memorystream& code)
{
	//declocal_i
	uint32_t t = code.readu30();
	decLocal_i(context, t);
}
void ABCVm::abc_negate_i(call_context* context,memorystream& code)
{
	//negate_i
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	pval->negate_i();
}
void ABCVm::abc_add_i(call_context* context,memorystream& code)
{
	//add_i
	RUNTIME_STACK_POP_CREATE(context,v2);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	pval->add_i(v2);
}
void ABCVm::abc_subtract_i(call_context* context,memorystream& code)
{
	//subtract_i
	RUNTIME_STACK_POP_CREATE(context,v2);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	pval->subtract_i(v2);
}
void ABCVm::abc_multiply_i(call_context* context,memorystream& code)
{
	//multiply_i
	RUNTIME_STACK_POP_CREATE(context,v2);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	pval->multiply_i(v2);
}
void ABCVm::abc_getlocal_0(call_context* context,memorystream& code)
{
	//getlocal_0
	int i=0;
	LOG_CALL( _("getLocal ") << i << _(": ") << context->locals[i].toDebugString() );
	ASATOM_INCREF(context->locals[i]);
	RUNTIME_STACK_PUSH(context,context->locals[i]);
}
void ABCVm::abc_getlocal_1(call_context* context,memorystream& code)
{
	//getlocal_1
	int i=1;
	LOG_CALL( _("getLocal ") << i << _(": ") << context->locals[i].toDebugString() );
	ASATOM_INCREF(context->locals[i]);
	RUNTIME_STACK_PUSH(context,context->locals[i]);
}
void ABCVm::abc_getlocal_2(call_context* context,memorystream& code)
{
	//getlocal_2
	int i=2;
	LOG_CALL( _("getLocal ") << i << _(": ") << context->locals[i].toDebugString() );
	ASATOM_INCREF(context->locals[i]);
	RUNTIME_STACK_PUSH(context,context->locals[i]);
}
void ABCVm::abc_getlocal_3(call_context* context,memorystream& code)
{
	//getlocal_3
	int i=3;
	LOG_CALL( _("getLocal ") << i << _(": ") << context->locals[i].toDebugString() );
	ASATOM_INCREF(context->locals[i]);
	RUNTIME_STACK_PUSH(context,context->locals[i]);
}
void ABCVm::abc_setlocal_0(call_context* context,memorystream& code)
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
	if ((int)i != context->argarrayposition || obj.type == T_ARRAY)
	{
		ASATOM_DECREF(context->locals[i]);
		context->locals[i]=obj;
	}
}
void ABCVm::abc_setlocal_1(call_context* context,memorystream& code)
{
	//setlocal_1
	unsigned int i=1;
	LOG_CALL( _("setLocal ") << i);

	RUNTIME_STACK_POP_CREATE(context,obj)
	if (i >= context->locals_size)
	{
		LOG(LOG_ERROR,"abc_setlocal invalid index:"<<i);
		return;
	}
	if ((int)i != context->argarrayposition || obj.type == T_ARRAY)
	{
		ASATOM_DECREF(context->locals[i]);
		context->locals[i]=obj;
	}
}
void ABCVm::abc_setlocal_2(call_context* context,memorystream& code)
{
	//setlocal_2
	unsigned int i=2;
	LOG_CALL( _("setLocal ") << i);
	RUNTIME_STACK_POP_CREATE(context,obj)
	if (i >= context->locals_size)
	{
		LOG(LOG_ERROR,"abc_setlocal invalid index:"<<i);
		return;
	}
	if ((int)i != context->argarrayposition || obj.type == T_ARRAY)
	{
		ASATOM_DECREF(context->locals[i]);
		context->locals[i]=obj;
	}
}
void ABCVm::abc_setlocal_3(call_context* context,memorystream& code)
{
	//setlocal_3
	unsigned int i=3;
	LOG_CALL( _("setLocal ") << i);
	RUNTIME_STACK_POP_CREATE(context,obj)
	if (i >= context->locals_size)
	{
		LOG(LOG_ERROR,"abc_setlocal invalid index:"<<i);
		return;
	}
	if ((int)i != context->argarrayposition || obj.type == T_ARRAY)
	{
		ASATOM_DECREF(context->locals[i]);
		context->locals[i]=obj;
	}
}
void ABCVm::abc_debug(call_context* context,memorystream& code)
{
	//debug
	LOG_CALL( _("debug") );
	code.readbyte();
	code.readu30();
	code.readbyte();
	code.readu30();
}
void ABCVm::abc_debugline(call_context* context,memorystream& code)
{
	//debugline
	LOG_CALL( _("debugline") );
	code.readu30();
}
void ABCVm::abc_debugfile(call_context* context,memorystream& code)
{
	//debugfile
	LOG_CALL( _("debugfile") );
	code.readu30();
}
void ABCVm::abc_bkptline(call_context* context,memorystream& code)
{
	//bkptline
	LOG_CALL( _("bkptline") );
	code.readu30();
}
void ABCVm::abc_timestamp(call_context* context,memorystream& code)
{
	//timestamp
	LOG_CALL( _("timestamp") );
}
void ABCVm::abc_invalidinstruction(call_context* context,memorystream& code)
{
	LOG(LOG_ERROR,_("Not interpreted instruction @") << code.tellg());
	code.seekg(code.tellg()-1);
	uint8_t opcode =code.readbyte();
	LOG(LOG_ERROR,_("dump ") << hex << (unsigned int)opcode << dec);
	throw ParseException("Not implemented instruction in interpreter");
}


void ABCVm::preloadFunction(const SyntheticFunction* function)
{
	method_info* mi=function->mi;

	const int code_len=mi->body->code.size();
	memorystream code(mi->body->code.data(), code_len,mi->body->codecache);
	while(!code.atend())
	{
		uint8_t opcode = code.readbyte();

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
			{
				
				code.fillu30();
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
				code.fills24();
				break;
			}
			case 0x1b://lookupswitch
			{
				code.fills24();
				uint32_t count = code.fillu30();
				for(unsigned int i=0;i<count+1;i++)
				{
					code.fills24();
				}
				break;
			}
			case 0x24://pushbyte
			{
				code.readbyte();
				break;
			}
			case 0x25://pushshort
			{
				code.readu32();
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
				code.fillu30();
				code.fillu30();
				break;
			}
			case 0xef://debug
			{
				
				code.readbyte();
				code.fillu30();
				code.readbyte();
				code.fillu30();
				break;
			}
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
				break;
			}
		}
	}
}
