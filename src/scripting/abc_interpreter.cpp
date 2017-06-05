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
	context->returnvalue = context->runtime_stack_pop();
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
	abc_invalidinstruction,
	abc_callstatic,
	abc_callsuper,
	abc_callproperty,
	abc_returnvoid,
	abc_returnvalue,
	abc_constructsuper,
	abc_constructprop,
	abc_invalidinstruction,
	abc_callproperty,
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
	ASObject* v=context->runtime_stack_pop();
	dxnslate(context, v);
}
void ABCVm::abc_kill(call_context* context,memorystream& code)
{
	//kill
	uint32_t t = code.readu30();
	LOG_CALL( "kill " << t);
	assert_and_throw(context->locals[t]);
	context->locals[t]->decRef();
	context->locals[t]=context->context->root->getSystemState()->getUndefinedRef();
}
void ABCVm::abc_label(call_context* context,memorystream& code)
{
	//label
}
void ABCVm::abc_ifnlt(call_context* context,memorystream& code)
{
	//ifnlt
	int32_t t = code.reads24();
	ASObject* v1=context->runtime_stack_pop();
	ASObject* v2=context->runtime_stack_pop();
	bool cond=ifNLT(v1, v2);
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
	ASObject* v1=context->runtime_stack_pop();
	ASObject* v2=context->runtime_stack_pop();
	bool cond=ifNLE(v1, v2);
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
	ASObject* v1=context->runtime_stack_pop();
	ASObject* v2=context->runtime_stack_pop();
	bool cond=ifNGT(v1, v2);
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
	ASObject* v1=context->runtime_stack_pop();
	ASObject* v2=context->runtime_stack_pop();
	bool cond=ifNGE(v1, v2);
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

	ASObject* v1=context->runtime_stack_pop();
	bool cond=ifTrue(v1);
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

	ASObject* v1=context->runtime_stack_pop();
	bool cond=ifFalse(v1);
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

	ASObject* v1=context->runtime_stack_pop();
	ASObject* v2=context->runtime_stack_pop();
	bool cond=ifEq(v1, v2);
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

	ASObject* v1=context->runtime_stack_pop();
	ASObject* v2=context->runtime_stack_pop();
	bool cond=ifNE(v1, v2);
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

	ASObject* v1=context->runtime_stack_pop();
	ASObject* v2=context->runtime_stack_pop();
	bool cond=ifLT(v1, v2);
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

	ASObject* v1=context->runtime_stack_pop();
	ASObject* v2=context->runtime_stack_pop();
	bool cond=ifLE(v1, v2);
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

	ASObject* v1=context->runtime_stack_pop();
	ASObject* v2=context->runtime_stack_pop();
	bool cond=ifGT(v1, v2);
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

	ASObject* v1=context->runtime_stack_pop();
	ASObject* v2=context->runtime_stack_pop();
	bool cond=ifGE(v1, v2);
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

	ASObject* v1=context->runtime_stack_pop();
	ASObject* v2=context->runtime_stack_pop();
	bool cond=ifStrictEq(v1, v2);
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

	ASObject* v1=context->runtime_stack_pop();
	ASObject* v2=context->runtime_stack_pop();
	bool cond=ifStrictNE(v1, v2);
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

	ASObject* index_obj=context->runtime_stack_pop();
	assert_and_throw(index_obj->getObjectType()==T_INTEGER);
	unsigned int index=index_obj->toUInt();
	index_obj->decRef();

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
	ASObject* v1=context->runtime_stack_pop();
	ASObject** pval=context->runtime_stack_pointer();
	*pval=(nextName(v1,*pval));
}
void ABCVm::abc_hasnext(call_context* context,memorystream& code)
{
	//hasnext
	ASObject* v1=context->runtime_stack_pop();
	ASObject** pval=context->runtime_stack_pointer();
	*pval=hasNext(v1,*pval);
}
void ABCVm::abc_pushnull(call_context* context,memorystream& code)
{
	//pushnull
	LOG_CALL("pushnull");
	context->runtime_stack_push(context->context->root->getSystemState()->getNullRef());
}
void ABCVm::abc_pushundefined(call_context* context,memorystream& code)
{
	LOG_CALL("pushundefined");
	context->runtime_stack_push(context->context->root->getSystemState()->getUndefinedRef());
}
void ABCVm::abc_nextvalue(call_context* context,memorystream& code)
{
	//nextvalue
	ASObject* v1=context->runtime_stack_pop();
	ASObject** pval=context->runtime_stack_pointer();
	*pval=(nextValue(v1,*pval));
}
void ABCVm::abc_pushbyte(call_context* context,memorystream& code)
{
	lightspark::method_body_info_cache* cachepos = code.tellcachepos();
	if (cachepos->type == method_body_info_cache::CACHE_TYPE_OBJECT)
	{
		context->runtime_stack_push(cachepos->obj);
		code.seekcachepos(cachepos->nextcachepos);
		return;
	}
		
	int8_t t = code.readbyte();
	code.setNextCachePos(cachepos);
	pushByte(t);

	ASObject* d= abstract_i(context->context->root->getSystemState(),t);
	d->setConstant();
	cachepos->type =method_body_info_cache::CACHE_TYPE_OBJECT;
	cachepos->obj = d;
	context->runtime_stack_push(d);
}
void ABCVm::abc_pushshort(call_context* context,memorystream& code)
{
	//pushshort
	lightspark::method_body_info_cache* cachepos = code.tellcachepos();
	if (cachepos->type == method_body_info_cache::CACHE_TYPE_OBJECT)
	{
		context->runtime_stack_push(cachepos->obj);
		code.seekcachepos(cachepos->nextcachepos);
		return;
	}
	// specs say pushshort is a u30, but it's really a u32
	// see https://bugs.adobe.com/jira/browse/ASC-4181
	uint32_t t = code.readu32();
	code.setNextCachePos(cachepos);

	ASObject* i= abstract_i(context->context->root->getSystemState(),t);
	i->setConstant();
	cachepos->type =method_body_info_cache::CACHE_TYPE_OBJECT;
	cachepos->obj = i;
	context->runtime_stack_push(i);

	pushShort(t);
}
void ABCVm::abc_pushtrue(call_context* context,memorystream& code)
{
	//pushtrue
	context->runtime_stack_push(abstract_b(context->context->root->getSystemState(),pushTrue()));
}
void ABCVm::abc_pushfalse(call_context* context,memorystream& code)
{
	//pushfalse
	context->runtime_stack_push(abstract_b(context->context->root->getSystemState(),pushFalse()));
}
void ABCVm::abc_pushnan(call_context* context,memorystream& code)
{
	//pushnan
	context->runtime_stack_push(pushNaN());
}
void ABCVm::abc_pop(call_context* context,memorystream& code)
{
	//pop
	pop();
	ASObject* o=context->runtime_stack_pop();
	if(o)
		o->decRef();
}
void ABCVm::abc_dup(call_context* context,memorystream& code)
{
	//dup
	dup();
	ASObject* o=context->runtime_stack_peek();
	o->incRef();
	context->runtime_stack_push(o);
}
void ABCVm::abc_swap(call_context* context,memorystream& code)
{
	//swap
	swap();
	ASObject* v1=context->runtime_stack_pop();
	ASObject* v2=context->runtime_stack_pop();

	context->runtime_stack_push(v1);
	context->runtime_stack_push(v2);
}
void ABCVm::abc_pushstring(call_context* context,memorystream& code)
{
	//pushstring
	uint32_t t = code.readu30();
	context->runtime_stack_push(pushString(context,t));
}
void ABCVm::abc_pushint(call_context* context,memorystream& code)
{
	//pushint
	lightspark::method_body_info_cache* cachepos = code.tellcachepos();
	if (cachepos->type == method_body_info_cache::CACHE_TYPE_OBJECT)
	{
		context->runtime_stack_push(cachepos->obj);
		code.seekcachepos(cachepos->nextcachepos);
		return;
	}

	uint32_t t = code.readu30();
	s32 val=context->context->constant_pool.integer[t];
	pushInt(context, val);

	ASObject* i=abstract_i(context->context->root->getSystemState(),val);
	i->setConstant();
	cachepos->type =method_body_info_cache::CACHE_TYPE_OBJECT;
	cachepos->obj = i;
	context->runtime_stack_push(i);
}
void ABCVm::abc_pushuint(call_context* context,memorystream& code)
{
	//pushuint
	lightspark::method_body_info_cache* cachepos = code.tellcachepos();
	if (cachepos->type == method_body_info_cache::CACHE_TYPE_OBJECT)
	{
		context->runtime_stack_push(cachepos->obj);
		code.seekcachepos(cachepos->nextcachepos);
		return;
	}

	uint32_t t = code.readu30();
	u32 val=context->context->constant_pool.uinteger[t];
	pushUInt(context, val);

	ASObject* i=abstract_ui(context->context->root->getSystemState(),val);
	i->setConstant();
	cachepos->type =method_body_info_cache::CACHE_TYPE_OBJECT;
	cachepos->obj = i;
	context->runtime_stack_push(i);
}
void ABCVm::abc_pushdouble(call_context* context,memorystream& code)
{
	//pushdouble
	//pushdouble
	lightspark::method_body_info_cache* cachepos = code.tellcachepos();
	if (cachepos->type == method_body_info_cache::CACHE_TYPE_OBJECT)
	{
		context->runtime_stack_push(cachepos->obj);
		code.seekcachepos(cachepos->nextcachepos);
		return;
	}
	uint32_t t = code.readu30();
	d64 val=context->context->constant_pool.doubles[t];
	pushDouble(context, val);

	ASObject* d= abstract_d(context->context->root->getSystemState(),val);
	d->setConstant();
	cachepos->type =method_body_info_cache::CACHE_TYPE_OBJECT;
	cachepos->obj = d;
	context->runtime_stack_push(d);
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
	context->runtime_stack_push( pushNamespace(context, t) );
}
void ABCVm::abc_hasnext2(call_context* context,memorystream& code)
{
	//hasnext2
	uint32_t t = code.readu30();
	uint32_t t2 = code.readu30();

	bool ret=hasNext2(context,t,t2);
	context->runtime_stack_push(abstract_b(context->context->root->getSystemState(),ret));
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
	loadIntN<uint32_t>(context);
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
	context->runtime_stack_push(newFunction(context,t));
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
void ABCVm::abc_callstatic(call_context* context,memorystream& code)
{
	//callstatic
	uint32_t t = code.readu30();
	uint32_t t2 = code.readu30();
	method_info* called_mi=NULL;
	callStatic(context,t,t2,&called_mi,true);
}
void ABCVm::abc_callsuper(call_context* context,memorystream& code)
{
	//callsuper
	uint32_t t = code.readu30();
	uint32_t t2 = code.readu30();
	method_info* called_mi=NULL;
	callSuper(context,t,t2,&called_mi,true);
}
void ABCVm::abc_callproperty(call_context* context,memorystream& code)
{
	//callproplex seems to be exactly like callproperty
	//callproperty
	uint32_t t = code.readu30();
	uint32_t t2 = code.readu30();
	callProperty(context,t,t2,NULL,true);
}
void ABCVm::abc_returnvoid(call_context* context,memorystream& code)
{
	//returnvoid
	LOG_CALL(_("returnVoid"));
	context->returnvalue = NULL;
	context->returning = true;
}
void ABCVm::abc_returnvalue(call_context* context,memorystream& code)
{
	//returnvalue
	context->returnvalue = context->runtime_stack_pop();
	LOG_CALL(_("returnValue ") << context->returnvalue);
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
	method_info* called_mi=NULL;
	callProperty(context,t,t2,&called_mi,false);
}
void ABCVm::abc_sxi1(call_context* context,memorystream& code)
{
	//sxi1
	LOG_CALL( "sxi1");
	ASObject* arg1=context->runtime_stack_pop();
	int32_t ret=arg1->toUInt()&0x1 ? -1 : 0;
	arg1->decRef();
	context->runtime_stack_push(abstract_i(context->context->root->getSystemState(),ret));
}
void ABCVm::abc_sxi8(call_context* context,memorystream& code)
{
	//sxi8
	LOG_CALL( "sxi8");
	ASObject* arg1=context->runtime_stack_pop();
	int32_t ret=(int8_t)arg1->toUInt();
	arg1->decRef();
	context->runtime_stack_push(abstract_i(context->context->root->getSystemState(),ret));
}
void ABCVm::abc_sxi16(call_context* context,memorystream& code)
{
	//sxi16
	LOG_CALL( "sxi16");
	ASObject* arg1=context->runtime_stack_pop();
	int32_t ret=(int16_t)arg1->toUInt();
	arg1->decRef();
	context->runtime_stack_push(abstract_i(context->context->root->getSystemState(),ret));
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
	context->runtime_stack_push(newActivation(context, context->mi));
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
	context->runtime_stack_push(newCatch(context,t));
}
void ABCVm::abc_findpropstrict(call_context* context,memorystream& code)
{
	//findpropstrict
//		uint32_t t = code.readu30();
//		multiname* name=context->context->getMultiname(t,context);
//		context->runtime_stack_push(findPropStrict(context,name));
//		name->resetNameIfObject();
		
		context->runtime_stack_push(findPropStrictCache(context,code));
}
void ABCVm::abc_findproperty(call_context* context,memorystream& code)
{
	//findproperty
	uint32_t t = code.readu30();
	multiname* name=context->context->getMultiname(t,context);
	context->runtime_stack_push(findProperty(context,name));
	name->resetNameIfObject();
}
void ABCVm::abc_finddef(call_context* context,memorystream& code)
{
	//finddef
	uint32_t t = code.readu30();
	multiname* name=context->context->getMultiname(t,context);
	LOG(LOG_NOT_IMPLEMENTED,"opcode 0x5f (finddef) not implemented:"<<*name);
	context->runtime_stack_push(context->context->root->getSystemState()->getNullRef());
	name->resetNameIfObject();
}
void ABCVm::abc_getlex(call_context* context,memorystream& code)
{
	//getlex
	lightspark::method_body_info_cache* cachepos = code.tellcachepos();
	if (cachepos->type == method_body_info_cache::CACHE_TYPE_OBJECT)
	{
		code.seekcachepos(cachepos->nextcachepos);
		context->runtime_stack_push(cachepos->obj);
		cachepos->obj->incRef();
		return;
	}
	uint32_t t = code.readu30();
	if (getLex(context,t))
	{
		// put object in cache
		cachepos->type =method_body_info_cache::CACHE_TYPE_OBJECT;
		cachepos->obj = context->runtime_stack_peek();
		cachepos->obj->incRef();
	}
}
void ABCVm::abc_setproperty(call_context* context,memorystream& code)
{
	//setproperty
	uint32_t t = code.readu30();
	ASObject* value=context->runtime_stack_pop();

	multiname* name=context->context->getMultiname(t,context);

	ASObject* obj=context->runtime_stack_pop();

	setProperty(value,obj,name);
	name->resetNameIfObject();
}
void ABCVm::abc_getlocal(call_context* context,memorystream& code)
{
	//getlocal
	uint32_t i = code.readu30();
	if (!context->locals[i])
	{
		LOG_CALL( _("getLocal ") << i << " not set, pushing Undefined");
		context->runtime_stack_push(context->context->root->getSystemState()->getUndefinedRef());
		return;
	}
	context->locals[i]->incRef();
	LOG_CALL( _("getLocal ") << i << _(": ") << context->locals[i]->toDebugString() );
	context->runtime_stack_push(context->locals[i]);
}
void ABCVm::abc_setlocal(call_context* context,memorystream& code)
{
	//setlocal
	uint32_t i = code.readu30();
	LOG_CALL( _("setLocal ") << i );
	ASObject* obj=context->runtime_stack_pop();
	assert_and_throw(obj);
	if ((int)i != context->argarrayposition || obj->is<Array>())
	{
		if(context->locals[i])
			context->locals[i]->decRef();
		context->locals[i]=obj;
	}
}
void ABCVm::abc_getglobalscope(call_context* context,memorystream& code)
{
	//getglobalscope
	context->runtime_stack_push(getGlobalScope(context));
}
void ABCVm::abc_getscopeobject(call_context* context,memorystream& code)
{
	//getscopeobject
	uint32_t t = code.readu30();
	context->runtime_stack_push(getScopeObject(context,t));
}
void ABCVm::abc_getProperty(call_context* context,memorystream& code)
{
	uint32_t t = code.readu30();
	multiname* name=context->context->getMultiname(t,context);

	ASObject* obj=context->runtime_stack_pop();

	ASObject* ret=getProperty(obj,name);
	name->resetNameIfObject();

	context->runtime_stack_push(ret);
}
void ABCVm::abc_initproperty(call_context* context,memorystream& code)
{
	//initproperty
	uint32_t t = code.readu30();
	ASObject* value=context->runtime_stack_pop();
	multiname* name=context->context->getMultiname(t,context);
	ASObject* obj=context->runtime_stack_pop();
	initProperty(obj,value,name);
	name->resetNameIfObject();
}
void ABCVm::abc_deleteproperty(call_context* context,memorystream& code)
{
	//deleteproperty
	uint32_t t = code.readu30();
	multiname* name = context->context->getMultiname(t,context);
	ASObject* obj=context->runtime_stack_pop();
	bool ret = deleteProperty(obj,name);
	name->resetNameIfObject();
	context->runtime_stack_push(abstract_b(context->context->root->getSystemState(),ret));
}
void ABCVm::abc_getslot(call_context* context,memorystream& code)
{
	//getslot
	uint32_t t = code.readu30();
	ASObject** pval=context->runtime_stack_pointer();
	*pval=getSlot(*pval, t);
}
void ABCVm::abc_setslot(call_context* context,memorystream& code)
{
	//setslot
	uint32_t t = code.readu30();
	ASObject* v1=context->runtime_stack_pop();
	ASObject* v2=context->runtime_stack_pop();

	setSlot(v1, v2, t);
}
void ABCVm::abc_getglobalSlot(call_context* context,memorystream& code)
{
	//getglobalSlot
	uint32_t t = code.readu30();

	Global* globalscope = getGlobalScope(context);
	context->runtime_stack_push(globalscope->getSlot(t));
}
void ABCVm::abc_setglobalSlot(call_context* context,memorystream& code)
{
	//setglobalSlot
	uint32_t t = code.readu30();

	Global* globalscope = getGlobalScope(context);
	ASObject* obj=context->runtime_stack_pop();
	globalscope->setSlot(t,obj);
}
void ABCVm::abc_convert_s(call_context* context,memorystream& code)
{
	//convert_s
	ASObject** pval=context->runtime_stack_pointer();
	*pval=convert_s(*pval);
}
void ABCVm::abc_esc_xelem(call_context* context,memorystream& code)
{
	ASObject** pval=context->runtime_stack_pointer();
	*pval = esc_xelem(*pval);
}
void ABCVm::abc_esc_xattr(call_context* context,memorystream& code)
{
	ASObject** pval=context->runtime_stack_pointer();
	*pval = esc_xattr(*pval);
}
void ABCVm::abc_convert_i(call_context* context,memorystream& code)
{
	//convert_i
	ASObject** pval=context->runtime_stack_pointer();
	if (!(*pval)->is<Integer>())
		*pval = abstract_i(context->context->root->getSystemState(),convert_i(*pval));
}
void ABCVm::abc_convert_u(call_context* context,memorystream& code)
{
	//convert_u
	ASObject** pval=context->runtime_stack_pointer();
	if (!(*pval)->is<UInteger>())
		*pval = abstract_ui(context->context->root->getSystemState(),convert_u(*pval));
}
void ABCVm::abc_convert_d(call_context* context,memorystream& code)
{
	//convert_d
	ASObject** pval=context->runtime_stack_pointer();
	switch ((*pval)->getObjectType())
	{
		case T_INTEGER:
		case T_BOOLEAN:
		case T_UINTEGER:
			*pval = abstract_di(context->context->root->getSystemState(),convert_di(*pval));
			break;
		case T_NUMBER:
			break;
		default:
			*pval = abstract_d(context->context->root->getSystemState(),convert_d(*pval));
			break;
	}
}
void ABCVm::abc_convert_b(call_context* context,memorystream& code)
{
	//convert_b
	ASObject** pval=context->runtime_stack_pointer();
	if (!(*pval)->is<Boolean>())
		*pval = abstract_b(context->context->root->getSystemState(),convert_b(*pval));
}
void ABCVm::abc_convert_o(call_context* context,memorystream& code)
{
	//convert_o
	ASObject** pval=context->runtime_stack_pointer();
	if ((*pval)->is<Null>())
	{
		LOG(LOG_ERROR,"trying to call convert_o on null");
		throwError<TypeError>(kConvertNullToObjectError);
	}
	if ((*pval)->is<Undefined>())
	{
		LOG(LOG_ERROR,"trying to call convert_o on undefined");
		throwError<TypeError>(kConvertUndefinedToObjectError);
	}
}
void ABCVm::abc_checkfilter(call_context* context,memorystream& code)
{
	//checkfilter
	ASObject** pval=context->runtime_stack_pointer();
	*pval=checkfilter(*pval);
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
	ASObject** pval=context->runtime_stack_pointer();
	if (!(*pval)->is<ASString>())
		*pval = coerce_s(*pval);
}
void ABCVm::abc_astype(call_context* context,memorystream& code)
{
	//astype
	uint32_t t = code.readu30();
	multiname* name=context->context->getMultiname(t,NULL);

	ASObject** pval=context->runtime_stack_pointer();
	*pval=asType(context->context, *pval, name);
}
void ABCVm::abc_astypelate(call_context* context,memorystream& code)
{
	//astypelate
	ASObject* v1=context->runtime_stack_pop();
	ASObject** pval=context->runtime_stack_pointer();
	*pval=asTypelate(v1, *pval);
}
void ABCVm::abc_negate(call_context* context,memorystream& code)
{
	//negate
	ASObject** pval=context->runtime_stack_pointer();
	if (((*pval)->is<Integer>() || (*pval)->is<UInteger>() || ((*pval)->is<Number>() && !(*pval)->as<Number>()->isfloat)) && (*pval)->toInt64() != 0 && (*pval)->toInt64() == (*pval)->toInt())
		*pval=abstract_di(context->context->root->getSystemState(),negate_i(*pval));
	else
		*pval=abstract_d(context->context->root->getSystemState(),negate(*pval));
}
void ABCVm::abc_increment(call_context* context,memorystream& code)
{
	//increment
	ASObject** pval=context->runtime_stack_pointer();
	if ((*pval)->is<Integer>() || (*pval)->is<UInteger>() || ((*pval)->is<Number>() && !(*pval)->as<Number>()->isfloat ))
		*pval=abstract_di(context->context->root->getSystemState(),increment_di(*pval));
	else
		*pval=abstract_d(context->context->root->getSystemState(),increment(*pval));
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
	ASObject** pval=context->runtime_stack_pointer();
	if ((*pval)->is<Integer>() || (*pval)->is<UInteger>() || ((*pval)->is<Number>() && !(*pval)->as<Number>()->isfloat ))
		*pval=abstract_di(context->context->root->getSystemState(),decrement_di(*pval));
	else
		*pval=abstract_d(context->context->root->getSystemState(),decrement(*pval));
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
	ASObject** pval=context->runtime_stack_pointer();
	*pval=typeOf(*pval);
}
void ABCVm::abc_not(call_context* context,memorystream& code)
{
	//not
	ASObject** pval=context->runtime_stack_pointer();
	*pval=abstract_b(context->context->root->getSystemState(),_not(*pval));
}
void ABCVm::abc_bitnot(call_context* context,memorystream& code)
{
	//bitnot
	ASObject** pval=context->runtime_stack_pointer();
	*pval=abstract_i(context->context->root->getSystemState(),bitNot(*pval));
}
void ABCVm::abc_add(call_context* context,memorystream& code)
{
	//add
	ASObject* v2=context->runtime_stack_pop();
	ASObject** pval=context->runtime_stack_pointer();
	*pval=add(v2, *pval);
}
void ABCVm::abc_subtract(call_context* context,memorystream& code)
{
	//subtract
	//Be careful, operands in subtract implementation are swapped
	ASObject* v2=context->runtime_stack_pop();
	ASObject** pval=context->runtime_stack_pointer();

	// if both values are Integers or int Numbers the result is also an int Number
	if( ((*pval)->is<Integer>() || (*pval)->is<UInteger>() || ((*pval)->is<Number>() && !(*pval)->as<Number>()->isfloat)) &&
		(v2->is<Integer>() || v2->is<UInteger>() || (v2->is<Number>() && !v2->as<Number>()->isfloat)))
	{
		int64_t num1=(*pval)->toInt64();
		int64_t num2=v2->toInt64();
		LOG_CALL(_("subtractI ")  << num1 << '-' << num2);
		(*pval)->decRef();
		v2->decRef();
		*pval = abstract_di(context->context->root->getSystemState(), num1-num2);
	}
	else
		*pval=abstract_d(context->context->root->getSystemState(),subtract(v2, *pval));
}
void ABCVm::abc_multiply(call_context* context,memorystream& code)
{
	//multiply
	ASObject* v2=context->runtime_stack_pop();
	ASObject** pval=context->runtime_stack_pointer();

	// if both values are Integers or int Numbers the result is also an int Number
	if( ((*pval)->is<Integer>() || (*pval)->is<UInteger>() || ((*pval)->is<Number>() && !(*pval)->as<Number>()->isfloat)) &&
		(v2->is<Integer>() || v2->is<UInteger>() || (v2->is<Number>() && !v2->as<Number>()->isfloat)))
	{
		int64_t num1=(*pval)->toInt64();
		int64_t num2=v2->toInt64();
		LOG_CALL(_("multiplyI ")  << num1 << '*' << num2);
		(*pval)->decRef();
		v2->decRef();
		*pval = abstract_di(context->context->root->getSystemState(), num1*num2);
	}
	else
		*pval=abstract_d(context->context->root->getSystemState(),multiply(v2, (*pval)));
}
void ABCVm::abc_divide(call_context* context,memorystream& code)
{
	//divide
	ASObject* v2=context->runtime_stack_pop();
	ASObject** pval=context->runtime_stack_pointer();
	*pval=abstract_d(context->context->root->getSystemState(),divide(v2, *pval));
}
void ABCVm::abc_modulo(call_context* context,memorystream& code)
{
	//modulo
	ASObject* v2=context->runtime_stack_pop();
	ASObject** pval=context->runtime_stack_pointer();

	// if both values are Integers or int Numbers the result is also an int Number
	if( ((*pval)->is<Integer>() || (*pval)->is<UInteger>() || ((*pval)->is<Number>() && !(*pval)->as<Number>()->isfloat)) &&
		(v2->is<Integer>() || v2->is<UInteger>() || (v2->is<Number>() && !v2->as<Number>()->isfloat)))
	{
		int64_t num1=(*pval)->toInt64();
		int64_t num2=v2->toInt64();
		LOG_CALL(_("moduloI ")  << num1 << '%' << num2);
		(*pval)->decRef();
		v2->decRef();
		if (num2 == 0)
			*pval=abstract_d(context->context->root->getSystemState(),Number::NaN);
		else
			*pval = abstract_di(context->context->root->getSystemState(), num1%num2);
	}
	else
		*pval=abstract_d(context->context->root->getSystemState(),modulo(*pval, v2));
}
void ABCVm::abc_lshift(call_context* context,memorystream& code)
{
	//lshift
	ASObject* v1=context->runtime_stack_pop();
	ASObject** pval=context->runtime_stack_pointer();

	*pval=abstract_i(context->context->root->getSystemState(),lShift(v1, *pval));
}
void ABCVm::abc_rshift(call_context* context,memorystream& code)
{
	//rshift
	ASObject* v1=context->runtime_stack_pop();
	ASObject** pval=context->runtime_stack_pointer();

	*pval=abstract_i(context->context->root->getSystemState(),rShift(v1, *pval));
}
void ABCVm::abc_urshift(call_context* context,memorystream& code)
{
	//urshift
	ASObject* v1=context->runtime_stack_pop();
	ASObject** pval=context->runtime_stack_pointer();

	*pval=abstract_ui(context->context->root->getSystemState(),urShift(v1, *pval));
}
void ABCVm::abc_bitand(call_context* context,memorystream& code)
{
	//bitand
	ASObject* v1=context->runtime_stack_pop();
	ASObject** pval=context->runtime_stack_pointer();

	*pval=abstract_i(context->context->root->getSystemState(),bitAnd(v1, *pval));
}
void ABCVm::abc_bitor(call_context* context,memorystream& code)
{
	//bitor
	ASObject* v1=context->runtime_stack_pop();
	ASObject** pval=context->runtime_stack_pointer();

	*pval=abstract_i(context->context->root->getSystemState(),bitOr(v1, *pval));
}
void ABCVm::abc_bitxor(call_context* context,memorystream& code)
{
	//bitxor
	ASObject* v1=context->runtime_stack_pop();
	ASObject** pval=context->runtime_stack_pointer();

	*pval=abstract_i(context->context->root->getSystemState(),bitXor(v1, *pval));
}
void ABCVm::abc_equals(call_context* context,memorystream& code)
{
	//equals
	ASObject* v2=context->runtime_stack_pop();
	ASObject** pval=context->runtime_stack_pointer();

	*pval=abstract_b(context->context->root->getSystemState(),equals(*pval, v2));
}
void ABCVm::abc_strictequals(call_context* context,memorystream& code)
{
	//strictequals
	ASObject* v2=context->runtime_stack_pop();
	ASObject** pval=context->runtime_stack_pointer();

	*pval=abstract_b(context->context->root->getSystemState(),strictEquals(*pval, v2));
}
void ABCVm::abc_lessthan(call_context* context,memorystream& code)
{
	//lessthan
	ASObject* v2=context->runtime_stack_pop();
	ASObject** pval=context->runtime_stack_pointer();

	*pval=abstract_b(context->context->root->getSystemState(),lessThan(*pval, v2));
}
void ABCVm::abc_lessequals(call_context* context,memorystream& code)
{
	//lessequals
	ASObject* v2=context->runtime_stack_pop();
	ASObject** pval=context->runtime_stack_pointer();

	*pval=abstract_b(context->context->root->getSystemState(),lessEquals(*pval, v2));
}
void ABCVm::abc_greaterthan(call_context* context,memorystream& code)
{
	//greaterthan
	ASObject* v2=context->runtime_stack_pop();
	ASObject** pval=context->runtime_stack_pointer();

	*pval=abstract_b(context->context->root->getSystemState(),greaterThan(*pval, v2));
}
void ABCVm::abc_greaterequals(call_context* context,memorystream& code)
{
	//greaterequals
	ASObject* v2=context->runtime_stack_pop();
	ASObject** pval=context->runtime_stack_pointer();

	*pval=abstract_b(context->context->root->getSystemState(),greaterEquals(*pval, v2));
}
void ABCVm::abc_instanceof(call_context* context,memorystream& code)
{
	//instanceof
	ASObject* type=context->runtime_stack_pop();
	ASObject** pval=context->runtime_stack_pointer();
	*pval=abstract_b(context->context->root->getSystemState(),instanceOf(*pval, type));
}
void ABCVm::abc_istype(call_context* context,memorystream& code)
{
	//istype
	uint32_t t = code.readu30();
	multiname* name=context->context->getMultiname(t,NULL);

	ASObject** pval=context->runtime_stack_pointer();

	*pval=abstract_b(context->context->root->getSystemState(),isType(context->context, *pval, name));
}
void ABCVm::abc_istypelate(call_context* context,memorystream& code)
{
	//istypelate
	ASObject* v1=context->runtime_stack_pop();
	ASObject** pval=context->runtime_stack_pointer();

	*pval=abstract_b(context->context->root->getSystemState(),isTypelate(v1, *pval));
}
void ABCVm::abc_in(call_context* context,memorystream& code)
{
	//in
	ASObject* v1=context->runtime_stack_pop();
	ASObject** pval=context->runtime_stack_pointer();

	*pval=abstract_b(context->context->root->getSystemState(),in(v1, *pval));
}
void ABCVm::abc_increment_i(call_context* context,memorystream& code)
{
	//increment_i
	ASObject** pval=context->runtime_stack_pointer();
	*pval=abstract_i(context->context->root->getSystemState(),increment_i(*pval));
}
void ABCVm::abc_decrement_i(call_context* context,memorystream& code)
{
	//decrement_i
	ASObject** pval=context->runtime_stack_pointer();
	*pval=abstract_i(context->context->root->getSystemState(),decrement_i(*pval));
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
	ASObject** pval=context->runtime_stack_pointer();
	*pval=abstract_i(context->context->root->getSystemState(),negate_i(*pval));
}
void ABCVm::abc_add_i(call_context* context,memorystream& code)
{
	//add_i
	ASObject* v2=context->runtime_stack_pop();
	ASObject** pval=context->runtime_stack_pointer();

	*pval=abstract_i(context->context->root->getSystemState(),add_i(v2, *pval));
}
void ABCVm::abc_subtract_i(call_context* context,memorystream& code)
{
	//subtract_i
	ASObject* v2=context->runtime_stack_pop();
	ASObject** pval=context->runtime_stack_pointer();

	*pval=abstract_i(context->context->root->getSystemState(),subtract_i(v2, *pval));
}
void ABCVm::abc_multiply_i(call_context* context,memorystream& code)
{
	//multiply_i
	ASObject* v2=context->runtime_stack_pop();
	ASObject** pval=context->runtime_stack_pointer();

	*pval=abstract_i(context->context->root->getSystemState(),multiply_i(v2, *pval));
}
void ABCVm::abc_getlocal_0(call_context* context,memorystream& code)
{
	//getlocal_0
	int i=0;
	if (!context->locals[i])
	{
		LOG_CALL( _("getLocal ") << i << " not set, pushing Undefined");
		context->runtime_stack_push(context->context->root->getSystemState()->getUndefinedRef());
		return;
	}
	LOG_CALL( _("getLocal ") << i << _(": ") << context->locals[i]->toDebugString() );
	context->locals[i]->incRef();
	context->runtime_stack_push(context->locals[i]);
}
void ABCVm::abc_getlocal_1(call_context* context,memorystream& code)
{
	//getlocal_1
	int i=1;
	if (!context->locals[i])
	{
		LOG_CALL( _("getLocal ") << i << " not set, pushing Undefined");
		context->runtime_stack_push(context->context->root->getSystemState()->getUndefinedRef());
		return;
	}
	LOG_CALL( _("getLocal ") << i << _(": ") << context->locals[i]->toDebugString() );
	context->locals[i]->incRef();
	context->runtime_stack_push(context->locals[i]);
}
void ABCVm::abc_getlocal_2(call_context* context,memorystream& code)
{
	//getlocal_2
	int i=2;
	if (!context->locals[i])
	{
		LOG_CALL( _("getLocal ") << i << " not set, pushing Undefined");
		context->runtime_stack_push(context->context->root->getSystemState()->getUndefinedRef());
		return;
	}
	LOG_CALL( _("getLocal ") << i << _(": ") << context->locals[i]->toDebugString() );
	context->locals[i]->incRef();
	context->runtime_stack_push(context->locals[i]);
}
void ABCVm::abc_getlocal_3(call_context* context,memorystream& code)
{
	//getlocal_3
	int i=3;
	if (!context->locals[i])
	{
		LOG_CALL( _("getLocal ") << i << " not set, pushing Undefined");
		context->runtime_stack_push(context->context->root->getSystemState()->getUndefinedRef());
		return;
	}
	LOG_CALL( _("getLocal ") << i << _(": ") << context->locals[i]->toDebugString() );
	context->locals[i]->incRef();
	context->runtime_stack_push(context->locals[i]);
}
void ABCVm::abc_setlocal_0(call_context* context,memorystream& code)
{
	//setlocal_0
	int i=0;
	LOG_CALL( _("setLocal ") << i);
	ASObject* obj=context->runtime_stack_pop();
	if ((int)i != context->argarrayposition || obj->is<Array>())
	{
		if(context->locals[i])
			context->locals[i]->decRef();
		context->locals[i]=obj;
	}
}
void ABCVm::abc_setlocal_1(call_context* context,memorystream& code)
{
	//setlocal_1
	int i=1;
	LOG_CALL( _("setLocal ") << i);
	ASObject* obj=context->runtime_stack_pop();
	if ((int)i != context->argarrayposition || obj->is<Array>())
	{
		if(context->locals[i])
			context->locals[i]->decRef();
		context->locals[i]=obj;
	}
}
void ABCVm::abc_setlocal_2(call_context* context,memorystream& code)
{
	//setlocal_2
	int i=2;
	LOG_CALL( _("setLocal ") << i);
	ASObject* obj=context->runtime_stack_pop();
	if ((int)i != context->argarrayposition || obj->is<Array>())
	{
		if(context->locals[i])
			context->locals[i]->decRef();
		context->locals[i]=obj;
	}
}
void ABCVm::abc_setlocal_3(call_context* context,memorystream& code)
{
	//setlocal_3
	int i=3;
	LOG_CALL( _("setLocal ") << i);
	ASObject* obj=context->runtime_stack_pop();
	if ((int)i != context->argarrayposition || obj->is<Array>())
	{
		if(context->locals[i])
			context->locals[i]->decRef();
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
