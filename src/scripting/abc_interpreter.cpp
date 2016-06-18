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

ASObject* ABCVm::executeFunction(const SyntheticFunction* function, call_context* context, ASObject* caller)
{
	method_info* mi=function->mi;

	const int code_len=mi->body->code.size();
	memorystream code(mi->body->code.data(), code_len,mi->body->codecache);
	//This may be non-zero and point to the position of an exception handler
	code.seekg(context->exec_pos);


	u8 opcode;

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
		opcode = code.readbyte();

		//Save ip for exception handling in SyntheticFunction::callImpl
		context->exec_pos = code.tellg();
		switch(opcode)
		{
			case 0x01:
			{
				//bkpt
				LOG_CALL( _("bkpt") );
				break;
			}
			case 0x02:
			{
				//nop
				break;
			}
			case 0x03:
			{
				//throw
				_throw(context);
				break;
			}
			case 0x04:
			{
				//getsuper
				uint32_t t = code.readu30();
				getSuper(context,t);
				break;
			}
			case 0x05:
			{
				//setsuper
				uint32_t t = code.readu30();
				setSuper(context,t);
				break;
			}
			case 0x06:
			{
				//dxns
				uint32_t t = code.readu30();
				dxns(context,t);
				break;
			}
			case 0x07:
			{
				//dxnslate
				ASObject* v=context->runtime_stack_pop();
				dxnslate(context, v);
				break;
			}
			case 0x08:
			{
				//kill
				uint32_t t = code.readu30();
				LOG_CALL( "kill " << t);
				assert_and_throw(context->locals[t]);
				context->locals[t]->decRef();
				context->locals[t]=function->getSystemState()->getUndefinedRef();
				break;
			}
			case 0x09:
			{
				//label
				break;
			}
			case 0x0c:
			{
				//ifnlt
				int32_t t = code.reads24();
				ASObject* v1=context->runtime_stack_pop();
				ASObject* v2=context->runtime_stack_pop();
				bool cond=ifNLT(v1, v2);
				if(cond)
				{
					int here=code.tellg();
					int dest=here+t;

					//Now 'jump' to the destination, validate on the length
					if(dest >= code_len)
						throw ParseException("Jump out of bounds in interpreter");
					code.seekg(dest);
				}
				break;
			}
			case 0x0d:
			{
				//ifnle
				int32_t t = code.reads24();
				ASObject* v1=context->runtime_stack_pop();
				ASObject* v2=context->runtime_stack_pop();
				bool cond=ifNLE(v1, v2);
				if(cond)
				{
					int here=code.tellg();
					int dest=here+t;

					//Now 'jump' to the destination, validate on the length
					if(dest >= code_len)
						throw ParseException("Jump out of bounds in interpreter");
					code.seekg(dest);
				}
				break;
			}
			case 0x0e:
			{
				//ifngt
				int32_t t = code.reads24();
				ASObject* v1=context->runtime_stack_pop();
				ASObject* v2=context->runtime_stack_pop();
				bool cond=ifNGT(v1, v2);
				if(cond)
				{
					int here=code.tellg();
					int dest=here+t;

					//Now 'jump' to the destination, validate on the length
					if(dest >= code_len)
						throw ParseException("Jump out of bounds in interpreter");
					code.seekg(dest);
				}
				break;
			}
			case 0x0f:
			{
				//ifnge
				int32_t t = code.reads24();
				ASObject* v1=context->runtime_stack_pop();
				ASObject* v2=context->runtime_stack_pop();
				bool cond=ifNGE(v1, v2);
				if(cond)
				{
					int here=code.tellg();
					int dest=here+t;

					//Now 'jump' to the destination, validate on the length
					if(dest >= code_len)
						throw ParseException("Jump out of bounds in interpreter");
					code.seekg(dest);
				}
				break;
			}
			case 0x10:
			{
				//jump
				int32_t t = code.reads24();

				int here=code.tellg();
				int dest=here+t;

				//Now 'jump' to the destination, validate on the length
				if(dest >= code_len)
					throw ParseException("Jump out of bounds in interpreter");
				code.seekg(dest);
				break;
			}
			case 0x11:
			{
				//iftrue
				int32_t t = code.reads24();

				ASObject* v1=context->runtime_stack_pop();
				bool cond=ifTrue(v1);
				if(cond)
				{
					int here=code.tellg();
					int dest=here+t;

					//Now 'jump' to the destination, validate on the length
					if(dest >= code_len)
						throw ParseException("Jump out of bounds in interpreter");
					code.seekg(dest);
				}
				break;
			}
			case 0x12:
			{
				//iffalse
				int32_t t = code.reads24();

				ASObject* v1=context->runtime_stack_pop();
				bool cond=ifFalse(v1);
				if(cond)
				{
					int here=code.tellg();
					int dest=here+t;

					//Now 'jump' to the destination, validate on the length
					if(dest >= code_len)
						throw ParseException("Jump out of bounds in interpreter");
					code.seekg(dest);
				}
				break;
			}
			case 0x13:
			{
				//ifeq
				int32_t t = code.reads24();

				ASObject* v1=context->runtime_stack_pop();
				ASObject* v2=context->runtime_stack_pop();
				bool cond=ifEq(v1, v2);
				if(cond)
				{
					int here=code.tellg();
					int dest=here+t;

					//Now 'jump' to the destination, validate on the length
					if(dest >= code_len)
						throw ParseException("Jump out of bounds in interpreter");
					code.seekg(dest);
				}
				break;
			}
			case 0x14:
			{
				//ifne
				int32_t t = code.reads24();

				ASObject* v1=context->runtime_stack_pop();
				ASObject* v2=context->runtime_stack_pop();
				bool cond=ifNE(v1, v2);
				if(cond)
				{
					int here=code.tellg();
					int dest=here+t;

					//Now 'jump' to the destination, validate on the length
					if(dest >= code_len)
						throw ParseException("Jump out of bounds in interpreter");
					code.seekg(dest);
				}
				break;
			}
			case 0x15:
			{
				//iflt
				int32_t t = code.reads24();

				ASObject* v1=context->runtime_stack_pop();
				ASObject* v2=context->runtime_stack_pop();
				bool cond=ifLT(v1, v2);
				if(cond)
				{
					int here=code.tellg();
					int dest=here+t;

					//Now 'jump' to the destination, validate on the length
					if(dest >= code_len)
						throw ParseException("Jump out of bounds in interpreter");
					code.seekg(dest);
				}
				break;
			}
			case 0x16:
			{
				//ifle
				int32_t t = code.reads24();

				ASObject* v1=context->runtime_stack_pop();
				ASObject* v2=context->runtime_stack_pop();
				bool cond=ifLE(v1, v2);
				if(cond)
				{
					int here=code.tellg();
					int dest=here+t;

					//Now 'jump' to the destination, validate on the length
					if(dest >= code_len)
						throw ParseException("Jump out of bounds in interpreter");
					code.seekg(dest);
				}
				break;
			}
			case 0x17:
			{
				//ifgt
				int32_t t = code.reads24();

				ASObject* v1=context->runtime_stack_pop();
				ASObject* v2=context->runtime_stack_pop();
				bool cond=ifGT(v1, v2);
				if(cond)
				{
					int here=code.tellg();
					int dest=here+t;

					//Now 'jump' to the destination, validate on the length
					if(dest >= code_len)
						throw ParseException("Jump out of bounds in interpreter");
					code.seekg(dest);
				}
				break;
			}
			case 0x18:
			{
				//ifge
				int32_t t = code.reads24();

				ASObject* v1=context->runtime_stack_pop();
				ASObject* v2=context->runtime_stack_pop();
				bool cond=ifGE(v1, v2);
				if(cond)
				{
					int here=code.tellg();
					int dest=here+t;

					//Now 'jump' to the destination, validate on the length
					if(dest >= code_len)
						throw ParseException("Jump out of bounds in interpreter");
					code.seekg(dest);
				}
				break;
			}
			case 0x19:
			{
				//ifstricteq
				int32_t t = code.reads24();

				ASObject* v1=context->runtime_stack_pop();
				ASObject* v2=context->runtime_stack_pop();
				bool cond=ifStrictEq(v1, v2);
				if(cond)
				{
					int here=code.tellg();
					int dest=here+t;

					//Now 'jump' to the destination, validate on the length
					if(dest >= code_len)
						throw ParseException("Jump out of bounds in interpreter");
					code.seekg(dest);
				}
				break;
			}
			case 0x1a:
			{
				//ifstrictne
				int32_t t = code.reads24();

				ASObject* v1=context->runtime_stack_pop();
				ASObject* v2=context->runtime_stack_pop();
				bool cond=ifStrictNE(v1, v2);
				if(cond)
				{
					int here=code.tellg();
					int dest=here+t;

					//Now 'jump' to the destination, validate on the length
					if(dest >= code_len)
						throw ParseException("Jump out of bounds in interpreter");
					code.seekg(dest);
				}
				break;
			}
			case 0x1b:
			{
				//lookupswitch
				int here=int(code.tellg())-1; //Base for the jumps is the instruction itself for the switch
				int32_t t = code.reads24();
				int defaultdest=here+t;
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

				int dest=defaultdest;
				if(index<=count)
					dest=here+offsets[index];

				if(dest >= code_len)
					throw ParseException("Jump out of bounds in interpreter");
				code.seekg(dest);
				break;
			}
			case 0x1c:
			{
				//pushwith
				pushWith(context);
				break;
			}
			case 0x1d:
			{
				//popscope
				popScope(context);
				break;
			}
			case 0x1e:
			{
				//nextname
				ASObject* v1=context->runtime_stack_pop();
				ASObject** pval=context->runtime_stack_pointer();
				*pval=nextName(v1,*pval);
				break;
			}
			case 0x20:
			{
				//pushnull
				context->runtime_stack_push(pushNull());
				break;
			}
			case 0x21:
			{
				//pushundefined
				context->runtime_stack_push(pushUndefined());
				break;
			}
			case 0x23:
			{
				//nextvalue
				ASObject* v1=context->runtime_stack_pop();
				ASObject** pval=context->runtime_stack_pointer();
				*pval=nextValue(v1,*pval);
				break;
			}
			case 0x24:
			{
				//pushbyte
				lightspark::method_body_info_cache* cachepos = code.tellcachepos();
				if (cachepos->type == method_body_info_cache::CACHE_TYPE_OBJECT)
				{
					context->runtime_stack_push(cachepos->obj);
					code.seekpos(cachepos->nextcodepos);
					break;
				}
					
				int8_t t = code.readbyte();
				cachepos->nextcodepos = code.tellpos();
				pushByte(t);
			
				ASObject* d= abstract_i(function->getSystemState(),t);
				d->setConstant();
				cachepos->type =method_body_info_cache::CACHE_TYPE_OBJECT;
				cachepos->obj = d;
				context->runtime_stack_push(d);
				break;
			}
			case 0x25:
			{
				//pushshort
				lightspark::method_body_info_cache* cachepos = code.tellcachepos();
				if (cachepos->type == method_body_info_cache::CACHE_TYPE_OBJECT)
				{
					context->runtime_stack_push(cachepos->obj);
					code.seekpos(cachepos->nextcodepos);
					break;
				}
				// specs say pushshort is a u30, but it's really a u32
				// see https://bugs.adobe.com/jira/browse/ASC-4181
				uint32_t t = code.readu32();
				cachepos->nextcodepos = code.tellpos();

				ASObject* i= abstract_i(function->getSystemState(),t);
				i->setConstant();
				cachepos->type =method_body_info_cache::CACHE_TYPE_OBJECT;
				cachepos->obj = i;
				context->runtime_stack_push(i);

				pushShort(t);
				break;
			}
			case 0x26:
			{
				//pushtrue
				context->runtime_stack_push(abstract_b(function->getSystemState(),pushTrue()));
				break;
			}
			case 0x27:
			{
				//pushfalse
				context->runtime_stack_push(abstract_b(function->getSystemState(),pushFalse()));
				break;
			}
			case 0x28:
			{
				//pushnan
				context->runtime_stack_push(pushNaN());
				break;
			}
			case 0x29:
			{
				//pop
				pop();
				ASObject* o=context->runtime_stack_pop();
				if(o)
					o->decRef();
				break;
			}
			case 0x2a:
			{
				//dup
				dup();
				ASObject* o=context->runtime_stack_peek();
				o->incRef();
				context->runtime_stack_push(o);
				break;
			}
			case 0x2b:
			{
				//swap
				swap();
				ASObject* v1=context->runtime_stack_pop();
				ASObject* v2=context->runtime_stack_pop();

				context->runtime_stack_push(v1);
				context->runtime_stack_push(v2);
				break;
			}
			case 0x2c:
			{
				//pushstring
				uint32_t t = code.readu30();
				context->runtime_stack_push(pushString(context,t));
				break;
			}
			case 0x2d:
			{
				//pushint
				lightspark::method_body_info_cache* cachepos = code.tellcachepos();
				if (cachepos->type == method_body_info_cache::CACHE_TYPE_OBJECT)
				{
					context->runtime_stack_push(cachepos->obj);
					code.seekpos(cachepos->nextcodepos);
					break;
				}

				uint32_t t = code.readu30();
				s32 val=context->context->constant_pool.integer[t];
				pushInt(context, val);

				ASObject* i=abstract_i(function->getSystemState(),val);
				i->setConstant();
				cachepos->type =method_body_info_cache::CACHE_TYPE_OBJECT;
				cachepos->obj = i;
				context->runtime_stack_push(i);
				break;
			}
			case 0x2e:
			{
				//pushuint
				lightspark::method_body_info_cache* cachepos = code.tellcachepos();
				if (cachepos->type == method_body_info_cache::CACHE_TYPE_OBJECT)
				{
					context->runtime_stack_push(cachepos->obj);
					code.seekpos(cachepos->nextcodepos);
					break;
				}

				uint32_t t = code.readu30();
				u32 val=context->context->constant_pool.uinteger[t];
				pushUInt(context, val);

				ASObject* i=abstract_ui(function->getSystemState(),val);
				i->setConstant();
				cachepos->type =method_body_info_cache::CACHE_TYPE_OBJECT;
				cachepos->obj = i;
				context->runtime_stack_push(i);
				break;
			}
			case 0x2f:
			{
				//pushdouble
				lightspark::method_body_info_cache* cachepos = code.tellcachepos();
				if (cachepos->type == method_body_info_cache::CACHE_TYPE_OBJECT)
				{
					context->runtime_stack_push(cachepos->obj);
					code.seekpos(cachepos->nextcodepos);
					break;
				}
				uint32_t t = code.readu30();
				d64 val=context->context->constant_pool.doubles[t];
				pushDouble(context, val);
			
				ASObject* d= abstract_d(function->getSystemState(),val);
				d->setConstant();
				cachepos->type =method_body_info_cache::CACHE_TYPE_OBJECT;
				cachepos->obj = d;
				context->runtime_stack_push(d);
				break;
			}
			case 0x30:
			{
				//pushscope
				pushScope(context);
				break;
			}
			case 0x31:
			{
				//pushnamespace
				uint32_t t = code.readu30();
				context->runtime_stack_push( pushNamespace(context, t) );
				break;
			}
			case 0x32:
			{
				//hasnext2
				uint32_t t = code.readu30();
				uint32_t t2 = code.readu30();

				bool ret=hasNext2(context,t,t2);
				context->runtime_stack_push(abstract_b(function->getSystemState(),ret));
				break;
			}
			//Alchemy opcodes
			case 0x35:
			{
				//li8
				LOG_CALL( "li8");
				loadIntN<uint8_t>(context);
				break;
			}
			case 0x36:
			{
				//li16
				LOG_CALL( "li16");
				loadIntN<uint16_t>(context);
				break;
			}
			case 0x37:
			{
				//li32
				LOG_CALL( "li32");
				loadIntN<uint32_t>(context);
				break;
			}
			case 0x38:
			{
				//lf32
				LOG_CALL( "lf32");
				loadFloat(context);
				break;
			}
			case 0x39:
			{
				//lf32
				LOG_CALL( "lf64");
				loadDouble(context);
				break;
			}
			case 0x3a:
			{
				//si8
				LOG_CALL( "si8");
				storeIntN<uint8_t>(context);
				break;
			}
			case 0x3b:
			{
				//si16
				LOG_CALL( "si16");
				storeIntN<uint16_t>(context);
				break;
			}
			case 0x3c:
			{
				//si32
				LOG_CALL( "si32");
				storeIntN<uint32_t>(context);
				break;
			}
			case 0x3d:
			{
				//sf32
				LOG_CALL( "sf32");
				storeFloat(context);
				break;
			}
			case 0x3e:
			{
				//sf32
				LOG_CALL( "sf64");
				storeDouble(context);
				break;
			}
			case 0x40:
			{
				//newfunction
				uint32_t t = code.readu30();
				context->runtime_stack_push(newFunction(context,t));
				break;
			}
			case 0x41:
			{
				//call
				uint32_t t = code.readu30();
				method_info* called_mi=NULL;
				PROF_ACCOUNT_TIME(mi->profTime[instructionPointer],profilingCheckpoint(startTime));
				call(context,t,&called_mi);
				if(called_mi)
					PROF_ACCOUNT_TIME(mi->profCalls[called_mi],profilingCheckpoint(startTime));
				else
					PROF_IGNORE_TIME(profilingCheckpoint(startTime));
				break;
			}
			case 0x42:
			{
				//construct
				uint32_t t = code.readu30();
				construct(context,t);
				break;
			}
			case 0x44:
			{
				//callstatic
				uint32_t t = code.readu30();
				uint32_t t2 = code.readu30();
				method_info* called_mi=NULL;
				PROF_ACCOUNT_TIME(mi->profTime[instructionPointer],profilingCheckpoint(startTime));
				callStatic(context,t,t2,&called_mi,true);
				if(called_mi)
					PROF_ACCOUNT_TIME(mi->profCalls[called_mi],profilingCheckpoint(startTime));
				else
					PROF_IGNORE_TIME(profilingCheckpoint(startTime));
				break;
			}
			case 0x45:
			{
				//callsuper
				uint32_t t = code.readu30();
				uint32_t t2 = code.readu30();
				method_info* called_mi=NULL;
				PROF_ACCOUNT_TIME(mi->profTime[instructionPointer],profilingCheckpoint(startTime));
				callSuper(context,t,t2,&called_mi,true);
				if(called_mi)
					PROF_ACCOUNT_TIME(mi->profCalls[called_mi],profilingCheckpoint(startTime));
				else
					PROF_IGNORE_TIME(profilingCheckpoint(startTime));
				break;
			}
			case 0x46:
			case 0x4c: //callproplex seems to be exactly like callproperty
			{
				//callproperty
				uint32_t t = code.readu30();
				uint32_t t2 = code.readu30();
				method_info* called_mi=NULL;
				PROF_ACCOUNT_TIME(mi->profTime[instructionPointer],profilingCheckpoint(startTime));
				callProperty(context,t,t2,&called_mi,true);
				if(called_mi)
					PROF_ACCOUNT_TIME(mi->profCalls[called_mi],profilingCheckpoint(startTime));
				else
					PROF_IGNORE_TIME(profilingCheckpoint(startTime));
				break;
			}
			case 0x47:
			{
				//returnvoid
				LOG_CALL(_("returnVoid"));
				PROF_ACCOUNT_TIME(mi->profTime[instructionPointer],profilingCheckpoint(startTime));
				return NULL;
			}
			case 0x48:
			{
				//returnvalue
				ASObject* ret=context->runtime_stack_pop();
				LOG_CALL(_("returnValue ") << ret);
				PROF_ACCOUNT_TIME(mi->profTime[instructionPointer],profilingCheckpoint(startTime));
				return ret;
			}
			case 0x49:
			{
				//constructsuper
				uint32_t t = code.readu30();
				constructSuper(context,t);
				break;
			}
			case 0x4a:
			{
				//constructprop
				uint32_t t = code.readu30();
				uint32_t t2 = code.readu30();
				constructProp(context,t,t2);
				break;
			}
			case 0x4e:
			{
				//callsupervoid
				uint32_t t = code.readu30();
				uint32_t t2 = code.readu30();
				method_info* called_mi=NULL;
				PROF_ACCOUNT_TIME(mi->profTime[instructionPointer],profilingCheckpoint(startTime));
				callSuper(context,t,t2,&called_mi,false);
				if(called_mi)
					PROF_ACCOUNT_TIME(mi->profCalls[called_mi],profilingCheckpoint(startTime));
				else
					PROF_IGNORE_TIME(profilingCheckpoint(startTime));
				break;
			}
			case 0x4f:
			{
				//callpropvoid
				uint32_t t = code.readu30();
				uint32_t t2 = code.readu30();
				method_info* called_mi=NULL;
				PROF_ACCOUNT_TIME(mi->profTime[instructionPointer],profilingCheckpoint(startTime));
				callProperty(context,t,t2,&called_mi,false);
				if(called_mi)
					PROF_ACCOUNT_TIME(mi->profCalls[called_mi],profilingCheckpoint(startTime));
				else
					PROF_IGNORE_TIME(profilingCheckpoint(startTime));
				break;
			}
			case 0x50:
			{
				//sxi1
				LOG_CALL( "sxi1");
				ASObject* arg1=context->runtime_stack_pop();
				int32_t ret=arg1->toUInt() >>31;
				arg1->decRef();
				context->runtime_stack_push(abstract_i(function->getSystemState(),ret));
				break;
			}
			case 0x51:
			{
				//sxi8
				LOG_CALL( "sxi8");
				ASObject* arg1=context->runtime_stack_pop();
				int32_t ret=(int8_t)arg1->toUInt();
				arg1->decRef();
				context->runtime_stack_push(abstract_i(function->getSystemState(),ret));
				break;
			}
			case 0x52:
			{
				//sxi16
				LOG_CALL( "sxi16");
				ASObject* arg1=context->runtime_stack_pop();
				int32_t ret=(int16_t)arg1->toUInt();
				arg1->decRef();
				context->runtime_stack_push(abstract_i(function->getSystemState(),ret));
				break;
			}
			case 0x53:
			{
				//constructgenerictype
				uint32_t t = code.readu30();
				constructGenericType(context, t);
				break;
			}
			case 0x55:
			{
				//newobject
				uint32_t t = code.readu30();
				newObject(context,t);
				break;
			}
			case 0x56:
			{
				//newarray
				uint32_t t = code.readu30();
				newArray(context,t);
				break;
			}
			case 0x57:
			{
				//newactivation
				context->runtime_stack_push(newActivation(context, mi,caller));
				break;
			}
			case 0x58:
			{
				//newclass
				uint32_t t = code.readu30();
				newClass(context,t);
				break;
			}
			case 0x59:
			{
				//getdescendants
				uint32_t t = code.readu30();
				getDescendants(context, t);
				break;
			}
			case 0x5a:
			{
				//newcatch
				uint32_t t = code.readu30();
				context->runtime_stack_push(newCatch(context,t));
				break;
			}
			case 0x5d:
			{
				//findpropstrict
				uint32_t t = code.readu30();
				multiname* name=context->context->getMultiname(t,context);
				context->runtime_stack_push(findPropStrict(context,name));
				name->resetNameIfObject();
				break;
			}
			case 0x5e:
			{
				//findproperty
				uint32_t t = code.readu30();
				multiname* name=context->context->getMultiname(t,context);
				context->runtime_stack_push(findProperty(context,name));
				name->resetNameIfObject();
				break;
			}
			case 0x5f:
			{
				//finddef
				uint32_t t = code.readu30();
				multiname* name=context->context->getMultiname(t,context);
				LOG(LOG_NOT_IMPLEMENTED,"opcode 0x5f (finddef) not implemented:"<<*name);
				context->runtime_stack_push(function->getSystemState()->getNullRef());
				name->resetNameIfObject();
				break;
			}
			case 0x60:
			{
				//getlex
				uint32_t t = code.readu30();
				getLex(context,t);
				break;
			}
			case 0x61:
			{
				//setproperty
				uint32_t t = code.readu30();
				ASObject* value=context->runtime_stack_pop();

				multiname* name=context->context->getMultiname(t,context);

				ASObject* obj=context->runtime_stack_pop();

				setProperty(value,obj,name);
				name->resetNameIfObject();
				break;
			}
			case 0x62:
			{
				//getlocal
				uint32_t i = code.readu30();
				if (!context->locals[i])
				{
					LOG_CALL( _("getLocal ") << i << " not set, pushing Undefined");
					context->runtime_stack_push(function->getSystemState()->getUndefinedRef());
					break;
				}
				context->locals[i]->incRef();
				LOG_CALL( _("getLocal ") << i << _(": ") << context->locals[i]->toDebugString() );
				context->runtime_stack_push(context->locals[i]);
				break;
			}
			case 0x63:
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
				break;
			}
			case 0x64:
			{
				//getglobalscope
				context->runtime_stack_push(getGlobalScope(context));
				break;
			}
			case 0x65:
			{
				//getscopeobject
				uint32_t t = code.readu30();
				context->runtime_stack_push(getScopeObject(context,t));
				break;
			}
			case 0x66:
			{
				//getproperty
				uint32_t t = code.readu30();
				multiname* name=context->context->getMultiname(t,context);

				ASObject* obj=context->runtime_stack_pop();

				ASObject* ret=getProperty(obj,name);
				name->resetNameIfObject();

				context->runtime_stack_push(ret);
				break;
			}
			case 0x68:
			{
				//initproperty
				uint32_t t = code.readu30();
				ASObject* value=context->runtime_stack_pop();
				multiname* name=context->context->getMultiname(t,context);
				ASObject* obj=context->runtime_stack_pop();
				initProperty(obj,value,name);
				name->resetNameIfObject();
				break;
			}
			case 0x6a:
			{
				//deleteproperty
				uint32_t t = code.readu30();
				multiname* name = context->context->getMultiname(t,context);
				ASObject* obj=context->runtime_stack_pop();
				bool ret = deleteProperty(obj,name);
				name->resetNameIfObject();
				context->runtime_stack_push(abstract_b(function->getSystemState(),ret));
				break;
			}
			case 0x6c:
			{
				//getslot
				uint32_t t = code.readu30();
				ASObject** pval=context->runtime_stack_pointer();
				*pval=getSlot(*pval, t);
				break;
			}
			case 0x6d:
			{
				//setslot
				uint32_t t = code.readu30();
				ASObject* v1=context->runtime_stack_pop();
				ASObject* v2=context->runtime_stack_pop();

				setSlot(v1, v2, t);
				break;
			}
			case 0x6e:
			{
				//getglobalSlot
				uint32_t t = code.readu30();

				Global* globalscope = getGlobalScope(context);
				context->runtime_stack_push(globalscope->getSlot(t));
				break;
			}
			case 0x6f:
			{
				//setglobalSlot
				uint32_t t = code.readu30();

				Global* globalscope = getGlobalScope(context);
				ASObject* obj=context->runtime_stack_pop();
				globalscope->setSlot(t,obj);
				break;
			}
			case 0x70:
			{
				//convert_s
				ASObject** pval=context->runtime_stack_pointer();
				*pval=convert_s(*pval);
				break;
			}
			case 0x71:
			{
				ASObject** pval=context->runtime_stack_pointer();
				*pval = esc_xelem(*pval);
				break;
			}
			case 0x72:
			{
				ASObject** pval=context->runtime_stack_pointer();
				*pval = esc_xattr(*pval);
				break;
			}case 0x73:
			{
				//convert_i
				ASObject** pval=context->runtime_stack_pointer();
				if (!(*pval)->is<Integer>())
					*pval = abstract_i(function->getSystemState(),convert_i(*pval));
				break;
			}
			case 0x74:
			{
				//convert_u
				ASObject** pval=context->runtime_stack_pointer();
				if (!(*pval)->is<UInteger>())
					*pval = abstract_ui(function->getSystemState(),convert_u(*pval));
				break;
			}
			case 0x75:
			{
				//convert_d
				ASObject** pval=context->runtime_stack_pointer();
				switch ((*pval)->getObjectType())
				{
					case T_INTEGER:
					case T_BOOLEAN:
					case T_UINTEGER:
						*pval = abstract_di(function->getSystemState(),convert_di(*pval));
						break;
					case T_NUMBER:
						break;
					default:
						*pval = abstract_d(function->getSystemState(),convert_d(*pval));
						break;
				}
				break;
			}
			case 0x76:
			{
				//convert_b
				ASObject** pval=context->runtime_stack_pointer();
				if (!(*pval)->is<Boolean>())
					*pval = abstract_b(function->getSystemState(),convert_b(*pval));
				break;
			}
			case 0x77:
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
				break;
			}
			case 0x78:
			{
				//checkfilter
				ASObject** pval=context->runtime_stack_pointer();
				*pval=checkfilter(*pval);
				break;
			}
			case 0x80:
			{
				//coerce
				uint32_t t = code.readu30();
				coerce(context, t);
				break;
			}
			case 0x82:
			{
				//coerce_a
				coerce_a();
				break;
			}
			case 0x85:
			{
				//coerce_s
				ASObject** pval=context->runtime_stack_pointer();
				if (!(*pval)->is<ASString>())
					*pval = coerce_s(*pval);
				break;
			}
			case 0x86:
			{
				//astype
				uint32_t t = code.readu30();
				multiname* name=context->context->getMultiname(t,NULL);

				ASObject** pval=context->runtime_stack_pointer();
				*pval=asType(context->context, *pval, name);
				break;
			}
			case 0x87:
			{
				//astypelate
				ASObject* v1=context->runtime_stack_pop();
				ASObject** pval=context->runtime_stack_pointer();
				*pval=asTypelate(v1, *pval);
				break;
			}
			case 0x90:
			{
				//negate
				ASObject** pval=context->runtime_stack_pointer();
				if (((*pval)->is<Integer>() || (*pval)->is<UInteger>() || ((*pval)->is<Number>() && !(*pval)->as<Number>()->isfloat)) && (*pval)->toInt64() != 0 && (*pval)->toInt64() == (*pval)->toInt())
					*pval=abstract_di(function->getSystemState(),negate_i(*pval));
				else
					*pval=abstract_d(function->getSystemState(),negate(*pval));
				break;
			}
			case 0x91:
			{
				//increment
				ASObject** pval=context->runtime_stack_pointer();
				if ((*pval)->is<Integer>() || (*pval)->is<UInteger>() || ((*pval)->is<Number>() && !(*pval)->as<Number>()->isfloat ))
					*pval=abstract_di(function->getSystemState(),increment_di(*pval));
				else
					*pval=abstract_d(function->getSystemState(),increment(*pval));
				break;
			}
			case 0x92:
			{
				//inclocal
				uint32_t t = code.readu30();
				incLocal(context, t);
				break;
			}
			case 0x93:
			{
				//decrement
				ASObject** pval=context->runtime_stack_pointer();
				if ((*pval)->is<Integer>() || (*pval)->is<UInteger>() || ((*pval)->is<Number>() && !(*pval)->as<Number>()->isfloat ))
					*pval=abstract_di(function->getSystemState(),decrement_di(*pval));
				else
					*pval=abstract_d(function->getSystemState(),decrement(*pval));
				break;
			}
			case 0x94:
			{
				//declocal
				uint32_t t = code.readu30();
				decLocal(context, t);
				break;
			}
			case 0x95:
			{
				//typeof
				ASObject** pval=context->runtime_stack_pointer();
				*pval=typeOf(*pval);
				break;
			}
			case 0x96:
			{
				//not
				ASObject** pval=context->runtime_stack_pointer();
				*pval=abstract_b(function->getSystemState(),_not(*pval));
				break;
			}
			case 0x97:
			{
				//bitnot
				ASObject** pval=context->runtime_stack_pointer();
				*pval=abstract_i(function->getSystemState(),bitNot(*pval));
				break;
			}
			case 0xa0:
			{
				//add
				ASObject* v2=context->runtime_stack_pop();
				ASObject** pval=context->runtime_stack_pointer();
				*pval=add(v2, *pval);
				break;
			}
			case 0xa1:
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
					*pval = abstract_di(function->getSystemState(), num1-num2);
				}
				else
					*pval=abstract_d(function->getSystemState(),subtract(v2, *pval));
				break;
			}
			case 0xa2:
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
					*pval = abstract_di(function->getSystemState(), num1*num2);
				}
				else
					*pval=abstract_d(function->getSystemState(),multiply(v2, (*pval)));
				break;
			}
			case 0xa3:
			{
				//divide
				ASObject* v2=context->runtime_stack_pop();
				ASObject** pval=context->runtime_stack_pointer();
				*pval=abstract_d(function->getSystemState(),divide(v2, *pval));
				break;
			}
			case 0xa4:
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
						*pval=abstract_d(function->getSystemState(),Number::NaN);
					else
						*pval = abstract_di(function->getSystemState(), num1%num2);
				}
				else
					*pval=abstract_d(function->getSystemState(),modulo(*pval, v2));
				break;
			}
			case 0xa5:
			{
				//lshift
				ASObject* v1=context->runtime_stack_pop();
				ASObject** pval=context->runtime_stack_pointer();

				*pval=abstract_i(function->getSystemState(),lShift(v1, *pval));
				break;
			}
			case 0xa6:
			{
				//rshift
				ASObject* v1=context->runtime_stack_pop();
				ASObject** pval=context->runtime_stack_pointer();

				*pval=abstract_i(function->getSystemState(),rShift(v1, *pval));
				break;
			}
			case 0xa7:
			{
				//urshift
				ASObject* v1=context->runtime_stack_pop();
				ASObject** pval=context->runtime_stack_pointer();

				*pval=abstract_ui(function->getSystemState(),urShift(v1, *pval));
				break;
			}
			case 0xa8:
			{
				//bitand
				ASObject* v1=context->runtime_stack_pop();
				ASObject** pval=context->runtime_stack_pointer();

				*pval=abstract_i(function->getSystemState(),bitAnd(v1, *pval));
				break;
			}
			case 0xa9:
			{
				//bitor
				ASObject* v1=context->runtime_stack_pop();
				ASObject** pval=context->runtime_stack_pointer();

				*pval=abstract_i(function->getSystemState(),bitOr(v1, *pval));
				break;
			}
			case 0xaa:
			{
				//bitxor
				ASObject* v1=context->runtime_stack_pop();
				ASObject** pval=context->runtime_stack_pointer();

				*pval=abstract_i(function->getSystemState(),bitXor(v1, *pval));
				break;
			}
			case 0xab:
			{
				//equals
				ASObject* v2=context->runtime_stack_pop();
				ASObject** pval=context->runtime_stack_pointer();

				*pval=abstract_b(function->getSystemState(),equals(*pval, v2));
				break;
			}
			case 0xac:
			{
				//strictequals
				ASObject* v2=context->runtime_stack_pop();
				ASObject** pval=context->runtime_stack_pointer();

				*pval=abstract_b(function->getSystemState(),strictEquals(*pval, v2));
				break;
			}
			case 0xad:
			{
				//lessthan
				ASObject* v2=context->runtime_stack_pop();
				ASObject** pval=context->runtime_stack_pointer();

				*pval=abstract_b(function->getSystemState(),lessThan(*pval, v2));
				break;
			}
			case 0xae:
			{
				//lessequals
				ASObject* v2=context->runtime_stack_pop();
				ASObject** pval=context->runtime_stack_pointer();

				*pval=abstract_b(function->getSystemState(),lessEquals(*pval, v2));
				break;
			}
			case 0xaf:
			{
				//greaterthan
				ASObject* v2=context->runtime_stack_pop();
				ASObject** pval=context->runtime_stack_pointer();

				*pval=abstract_b(function->getSystemState(),greaterThan(*pval, v2));
				break;
			}
			case 0xb0:
			{
				//greaterequals
				ASObject* v2=context->runtime_stack_pop();
				ASObject** pval=context->runtime_stack_pointer();

				*pval=abstract_b(function->getSystemState(),greaterEquals(*pval, v2));
				break;
			}
			case 0xb1:
			{
				//instanceof
				ASObject* type=context->runtime_stack_pop();
				ASObject** pval=context->runtime_stack_pointer();
				*pval=abstract_b(function->getSystemState(),instanceOf(*pval, type));
				break;
			}
			case 0xb2:
			{
				//istype
				uint32_t t = code.readu30();
				multiname* name=context->context->getMultiname(t,NULL);

				ASObject** pval=context->runtime_stack_pointer();

				*pval=abstract_b(function->getSystemState(),isType(context->context, *pval, name));
				break;
			}
			case 0xb3:
			{
				//istypelate
				ASObject* v1=context->runtime_stack_pop();
				ASObject** pval=context->runtime_stack_pointer();

				*pval=abstract_b(function->getSystemState(),isTypelate(v1, *pval));
				break;
			}
			case 0xb4:
			{
				//in
				ASObject* v1=context->runtime_stack_pop();
				ASObject** pval=context->runtime_stack_pointer();

				*pval=abstract_b(function->getSystemState(),in(v1, *pval));
				break;
			}
			case 0xc0:
			{
				//increment_i
				ASObject** pval=context->runtime_stack_pointer();
				*pval=abstract_i(function->getSystemState(),increment_i(*pval));
				break;
			}
			case 0xc1:
			{
				//decrement_i
				ASObject** pval=context->runtime_stack_pointer();
				*pval=abstract_i(function->getSystemState(),decrement_i(*pval));
				break;
			}
			case 0xc2:
			{
				//inclocal_i
				uint32_t t = code.readu30();
				incLocal_i(context, t);
				break;
			}
			case 0xc3:
			{
				//declocal_i
				uint32_t t = code.readu30();
				decLocal_i(context, t);
				break;
			}
			case 0xc4:
			{
				//negate_i
				ASObject** pval=context->runtime_stack_pointer();
				*pval=abstract_i(function->getSystemState(),negate_i(*pval));
				break;
			}
			case 0xc5:
			{
				//add_i
				ASObject* v2=context->runtime_stack_pop();
				ASObject** pval=context->runtime_stack_pointer();

				*pval=abstract_i(function->getSystemState(),add_i(v2, *pval));
				break;
			}
			case 0xc6:
			{
				//subtract_i
				ASObject* v2=context->runtime_stack_pop();
				ASObject** pval=context->runtime_stack_pointer();

				*pval=abstract_i(function->getSystemState(),subtract_i(v2, *pval));
				break;
			}
			case 0xc7:
			{
				//multiply_i
				ASObject* v2=context->runtime_stack_pop();
				ASObject** pval=context->runtime_stack_pointer();

				*pval=abstract_i(function->getSystemState(),multiply_i(v2, *pval));
				break;
			}
			case 0xd0:
			case 0xd1:
			case 0xd2:
			case 0xd3:
			{
				//getlocal_n
				int i=opcode&3;
				if (!context->locals[i])
				{
					LOG_CALL( _("getLocal ") << i << " not set, pushing Undefined");
					context->runtime_stack_push(function->getSystemState()->getUndefinedRef());
					break;
				}
				LOG_CALL( _("getLocal ") << i << _(": ") << context->locals[i]->toDebugString() );
				context->locals[i]->incRef();
				context->runtime_stack_push(context->locals[i]);
				break;
			}
			case 0xd4:
			case 0xd5:
			case 0xd6:
			case 0xd7:
			{
				//setlocal_n
				int i=opcode&3;
				LOG_CALL( _("setLocal ") << i);
				ASObject* obj=context->runtime_stack_pop();
				if ((int)i != context->argarrayposition || obj->is<Array>())
				{
					if(context->locals[i])
						context->locals[i]->decRef();
					context->locals[i]=obj;
				}
				break;
			}
			case 0xef:
			{
				//debug
				LOG_CALL( _("debug") );
				code.readbyte();
				code.readu30();
				code.readbyte();
				code.readu30();
				break;
			}
			case 0xf0:
			{
				//debugline
				LOG_CALL( _("debugline") );
				code.readu30();
				break;
			}
			case 0xf1:
			{
				//debugfile
				LOG_CALL( _("debugfile") );
				code.readu30();
				break;
			}
			case 0xf2:
			{
				//bkptline
				LOG_CALL( _("bkptline") );
				code.readu30();
				break;
			}
			case 0xf3:
			{
				//timestamp
				LOG_CALL( _("timestamp") );
				break;
			}
			default:
				LOG(LOG_ERROR,_("Not interpreted instruction @") << code.tellg());
				LOG(LOG_ERROR,_("dump ") << hex << (unsigned int)opcode << dec);
				throw ParseException("Not implemented instruction in interpreter");
		}
		PROF_ACCOUNT_TIME(mi->profTime[instructionPointer],profilingCheckpoint(startTime));
	}

#undef PROF_ACCOUNT_TIME 
#undef PROF_IGNORE_TIME
	//We managed to execute all the function
	return context->runtime_stack_pop();
}
