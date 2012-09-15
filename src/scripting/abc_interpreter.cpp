/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2012  Alessandro Pignotti (a.pignotti@sssup.it)

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

ASObject* ABCVm::executeFunction(const SyntheticFunction* function, call_context* context)
{
	method_info* mi=function->mi;

	memorystream code(mi->body->code.data(), mi->body->code.size());
	//This may be non-zero and point to the position of an exception handler
	code.seekg(context->exec_pos);

	const int code_len=mi->body->code.size();

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
		code >> opcode;
		if(code.eof())
			throw ParseException("End of code in interpreter");

		//Save ip for exception handling in SyntheticFunction::callImpl
		context->exec_pos = code.tellg();
		switch(opcode)
		{
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
				u30 t;
				code >> t;
				getSuper(context,t);
				break;
			}
			case 0x05:
			{
				//setsuper
				u30 t;
				code >> t;
				setSuper(context,t);
				break;
			}
			case 0x06:
			{
				//dxns
				u30 t;
				code >> t;
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
				u30 t;
				code >> t;
				LOG(LOG_CALLS, "kill " << t);
				assert_and_throw(context->locals[t]);
				context->locals[t]->decRef();
				context->locals[t]=getSys()->getUndefinedRef();
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
				s24 t;
				code >> t;
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
				s24 t;
				code >> t;
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
				s24 t;
				code >> t;
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
				s24 t;
				code >> t;
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
				s24 t;
				code >> t;

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
				s24 t;
				code >> t;

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
				s24 t;
				code >> t;

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
				s24 t;
				code >> t;

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
				s24 t;
				code >> t;

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
				s24 t;
				code >> t;

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
				s24 t;
				code >> t;

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
				s24 t;
				code >> t;

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
				s24 t;
				code >> t;

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
				s24 t;
				code >> t;

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
				s24 t;
				code >> t;

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
				s24 t;
				code >> t;
				int defaultdest=here+t;
				LOG(LOG_CALLS,_("Switch default dest ") << defaultdest);
				u30 count;
				code >> count;
				s24* offsets=g_newa(s24, count+1);
				for(unsigned int i=0;i<count+1;i++)
				{
					code >> offsets[i];
					LOG(LOG_CALLS,_("Switch dest ") << i << ' ' << offsets[i]);
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
				ASObject* v2=context->runtime_stack_pop();
				context->runtime_stack_push(nextName(v1,v2));
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
				ASObject* v2=context->runtime_stack_pop();
				context->runtime_stack_push(nextValue(v1,v2));
				break;
			}
			case 0x24:
			{
				//pushbyte
				int8_t t;
				code.read((char*)&t,1);
				context->runtime_stack_push(abstract_i(t));
				pushByte(t);
				break;
			}
			case 0x25:
			{
				//pushshort
				// specs say pushshort is a u30, but it's really a u32
				// see https://bugs.adobe.com/jira/browse/ASC-4181
				u32 t;
				code >> t;
				context->runtime_stack_push(abstract_i(t));
				pushShort(t);
				break;
			}
			case 0x26:
			{
				//pushtrue
				context->runtime_stack_push(abstract_b(pushTrue()));
				break;
			}
			case 0x27:
			{
				//pushfalse
				context->runtime_stack_push(abstract_b(pushFalse()));
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
				u30 t;
				code >> t;
				context->runtime_stack_push(pushString(context,t));
				break;
			}
			case 0x2d:
			{
				//pushint
				u30 t;
				code >> t;
				s32 val=context->context->constant_pool.integer[t];
				pushInt(context, val);

				ASObject* i=abstract_i(val);
				context->runtime_stack_push(i);
				break;
			}
			case 0x2e:
			{
				//pushuint
				u30 t;
				code >> t;
				u32 val=context->context->constant_pool.uinteger[t];
				pushUInt(context, val);

				ASObject* i=abstract_ui(val);
				context->runtime_stack_push(i);
				break;
			}
			case 0x2f:
			{
				//pushdouble
				u30 t;
				code >> t;
				d64 val=context->context->constant_pool.doubles[t];
				pushDouble(context, val);

				ASObject* d=abstract_d(val);
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
				u30 t;
				code >> t;
				context->runtime_stack_push( pushNamespace(context, t) );
				break;
			}
			case 0x32:
			{
				//hasnext2
				u30 t,t2;
				code >> t;
				code >> t2;

				bool ret=hasNext2(context,t,t2);
				context->runtime_stack_push(abstract_b(ret));
				break;
			}
			//Alchemy opcodes
			case 0x35:
			{
				//li8
				LOG(LOG_CALLS, "li8");
				loadIntN<uint8_t>(context);
				break;
			}
			case 0x36:
			{
				//li16
				LOG(LOG_CALLS, "li16");
				loadIntN<uint16_t>(context);
				break;
			}
			case 0x37:
			{
				//li32
				LOG(LOG_CALLS, "li32");
				loadIntN<uint32_t>(context);
				break;
			}
			case 0x3a:
			{
				//si8
				LOG(LOG_CALLS, "si8");
				storeIntN<uint8_t>(context);
				break;
			}
			case 0x3b:
			{
				//si16
				LOG(LOG_CALLS, "si16");
				storeIntN<uint16_t>(context);
				break;
			}
			case 0x3c:
			{
				//si32
				LOG(LOG_CALLS, "si32");
				storeIntN<uint32_t>(context);
				break;
			}
			case 0x40:
			{
				//newfunction
				u30 t;
				code >> t;
				context->runtime_stack_push(newFunction(context,t));
				break;
			}
			case 0x41:
			{
				//call
				u30 t;
				code >> t;
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
				u30 t;
				code >> t;
				construct(context,t);
				break;
			}
			case 0x45:
			{
				//callsuper
				u30 t,t2;
				code >> t;
				code >> t2;
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
				u30 t,t2;
				code >> t;
				code >> t2;
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
				LOG(LOG_CALLS,_("returnVoid"));
				PROF_ACCOUNT_TIME(mi->profTime[instructionPointer],profilingCheckpoint(startTime));
				return NULL;
			}
			case 0x48:
			{
				//returnvalue
				ASObject* ret=context->runtime_stack_pop();
				LOG(LOG_CALLS,_("returnValue ") << ret);
				PROF_ACCOUNT_TIME(mi->profTime[instructionPointer],profilingCheckpoint(startTime));
				return ret;
			}
			case 0x49:
			{
				//constructsuper
				u30 t;
				code >> t;
				constructSuper(context,t);
				break;
			}
			case 0x4a:
			{
				//constructprop
				u30 t,t2;
				code >> t;
				code >> t2;
				constructProp(context,t,t2);
				break;
			}
			case 0x4e:
			{
				//callsupervoid
				u30 t,t2;
				code >> t;
				code >> t2;
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
				u30 t,t2;
				code >> t;
				code >> t2;
				method_info* called_mi=NULL;
				PROF_ACCOUNT_TIME(mi->profTime[instructionPointer],profilingCheckpoint(startTime));
				callProperty(context,t,t2,&called_mi,false);
				if(called_mi)
					PROF_ACCOUNT_TIME(mi->profCalls[called_mi],profilingCheckpoint(startTime));
				else
					PROF_IGNORE_TIME(profilingCheckpoint(startTime));
				break;
			}
			case 0x53:
			{
				//constructgenerictype
				u30 t;
				code >> t;
				constructGenericType(context, t);
				break;
			}
			case 0x55:
			{
				//newobject
				u30 t;
				code >> t;
				newObject(context,t);
				break;
			}
			case 0x56:
			{
				//newarray
				u30 t;
				code >> t;
				newArray(context,t);
				break;
			}
			case 0x57:
			{
				//newactivation
				context->runtime_stack_push(newActivation(context, mi));
				break;
			}
			case 0x58:
			{
				//newclass
				u30 t;
				code >> t;
				newClass(context,t);
				break;
			}
			case 0x59:
			{
				//getdescendants
				u30 t;
				code >> t;
				getDescendants(context, t);
				break;
			}
			case 0x5a:
			{
				//newcatch
				u30 t;
				code >> t;
				context->runtime_stack_push(newCatch(context,t));
				break;
			}
			case 0x5d:
			{
				//findpropstrict
				u30 t;
				code >> t;
				multiname* name=context->context->getMultiname(t,context);
				context->runtime_stack_push(findPropStrict(context,name));
				name->resetNameIfObject();
				break;
			}
			case 0x5e:
			{
				//findproperty
				u30 t;
				code >> t;
				multiname* name=context->context->getMultiname(t,context);
				context->runtime_stack_push(findProperty(context,name));
				name->resetNameIfObject();
				break;
			}
			case 0x60:
			{
				//getlex
				u30 t;
				code >> t;
				getLex(context,t);
				break;
			}
			case 0x61:
			{
				//setproperty
				u30 t;
				code >> t;
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
				u30 i;
				code >> i;
				assert_and_throw(context->locals[i]);
				context->locals[i]->incRef();
				LOG(LOG_CALLS, _("getLocal ") << i << _(": ") << context->locals[i]->toDebugString() );
				context->runtime_stack_push(context->locals[i]);
				break;
			}
			case 0x63:
			{
				//setlocal
				u30 i;
				code >> i;
				LOG(LOG_CALLS, _("setLocal ") << i );
				ASObject* obj=context->runtime_stack_pop();
				assert_and_throw(obj);
				if(context->locals[i])
					context->locals[i]->decRef();
				context->locals[i]=obj;
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
				u30 t;
				code >> t;
				context->runtime_stack_push(getScopeObject(context,t));
				break;
			}
			case 0x66:
			{
				//getproperty
				u30 t;
				code >> t;
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
				u30 t;
				code >> t;
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
				u30 t;
				code >> t;
				multiname* name = context->context->getMultiname(t,context);
				ASObject* obj=context->runtime_stack_pop();
				bool ret = deleteProperty(obj,name);
				name->resetNameIfObject();
				context->runtime_stack_push(abstract_b(ret));
				break;
			}
			case 0x6c:
			{
				//getslot
				u30 t;
				code >> t;
				ASObject* obj=context->runtime_stack_pop();
				ASObject* ret=getSlot(obj, t);
				context->runtime_stack_push(ret);
				break;
			}
			case 0x6d:
			{
				//setslot
				u30 t;
				code >> t;

				ASObject* v1=context->runtime_stack_pop();
				ASObject* v2=context->runtime_stack_pop();

				setSlot(v1, v2, t);
				break;
			}
			case 0x70:
			{
				//convert_s
				ASObject* val=context->runtime_stack_pop();
				context->runtime_stack_push(convert_s(val));
				break;
			}
			case 0x71:
			{
				//FIXME: Properly escape as described in ECMA-357 section 10.2
				//esc_xelem
				ASObject* val=context->runtime_stack_pop();
				context->runtime_stack_push(convert_s(val));
				break;
			}
			case 0x72:
			{
				//FIXME: Properly escape as described in ECMA-357 section 10.2
				//esc_xattr
				ASObject* val=context->runtime_stack_pop();
				context->runtime_stack_push(esc_xattr(val));
				break;
			}case 0x73:
			{
				//convert_i
				ASObject* val=context->runtime_stack_pop();
				context->runtime_stack_push(abstract_i(convert_i(val)));
				break;
			}
			case 0x74:
			{
				//convert_u
				ASObject* val=context->runtime_stack_pop();
				context->runtime_stack_push(abstract_ui(convert_u(val)));
				break;
			}
			case 0x75:
			{
				//convert_d
				ASObject* val=context->runtime_stack_pop();
				context->runtime_stack_push(abstract_d(convert_d(val)));
				break;
			}
			case 0x76:
			{
				//convert_b
				ASObject* val=context->runtime_stack_pop();
				context->runtime_stack_push(abstract_b(convert_b(val)));
				break;
			}
			case 0x78:
			{
				//checkfilter
				ASObject* val=context->runtime_stack_pop();
				context->runtime_stack_push(checkfilter(val));
				break;
			}
			case 0x80:
			{
				//coerce
				u30 t;
				code >> t;
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
				context->runtime_stack_push(coerce_s(context->runtime_stack_pop()));
				break;
			}
			case 0x86:
			{
				//astype
				u30 t;
				code >> t;
				multiname* name=context->context->getMultiname(t,NULL);

				ASObject* v1=context->runtime_stack_pop();

				ASObject* ret=asType(context->context, v1, name);
				context->runtime_stack_push(ret);
				break;
			}
			case 0x87:
			{
				//astypelate
				ASObject* v1=context->runtime_stack_pop();
				ASObject* v2=context->runtime_stack_pop();

				ASObject* ret=asTypelate(v1, v2);
				context->runtime_stack_push(ret);
				break;
			}
			case 0x90:
			{
				//negate
				ASObject* val=context->runtime_stack_pop();
				ASObject* ret=abstract_d(negate(val));
				context->runtime_stack_push(ret);
				break;
			}
			case 0x91:
			{
				//increment
				ASObject* val=context->runtime_stack_pop();
				ASObject* ret=abstract_d(increment(val));
				context->runtime_stack_push(ret);
				break;
			}
			case 0x92:
			{
				//inclocal
				u30 t;
				code >> t;
				incLocal(context, t);
				break;
			}
			case 0x93:
			{
				//decrement
				ASObject* val=context->runtime_stack_pop();
				ASObject* ret=abstract_d(decrement(val));
				context->runtime_stack_push(ret);
				break;
			}
			case 0x94:
			{
				//declocal
				u30 t;
				code >> t;
				decLocal(context, t);
				break;
			}
			case 0x95:
			{
				//typeof
				ASObject* val=context->runtime_stack_pop();
				ASObject* ret=typeOf(val);
				context->runtime_stack_push(ret);
				break;
			}
			case 0x96:
			{
				//not
				ASObject* val=context->runtime_stack_pop();
				ASObject* ret=abstract_b(_not(val));
				context->runtime_stack_push(ret);
				break;
			}
			case 0x97:
			{
				//bitnot
				ASObject* val=context->runtime_stack_pop();
				ASObject* ret=abstract_i(bitNot(val));
				context->runtime_stack_push(ret);
				break;
			}
			case 0xa0:
			{
				//add
				ASObject* v2=context->runtime_stack_pop();
				ASObject* v1=context->runtime_stack_pop();

				ASObject* ret=add(v2, v1);
				context->runtime_stack_push(ret);
				break;
			}
			case 0xa1:
			{
				//subtract
				//Be careful, operands in subtract implementation are swapped
				ASObject* v2=context->runtime_stack_pop();
				ASObject* v1=context->runtime_stack_pop();

				ASObject* ret=abstract_d(subtract(v2, v1));
				context->runtime_stack_push(ret);
				break;
			}
			case 0xa2:
			{
				//multiply
				ASObject* v2=context->runtime_stack_pop();
				ASObject* v1=context->runtime_stack_pop();

				ASObject* ret=abstract_d(multiply(v2, v1));
				context->runtime_stack_push(ret);
				break;
			}
			case 0xa3:
			{
				//divide
				ASObject* v2=context->runtime_stack_pop();
				ASObject* v1=context->runtime_stack_pop();

				ASObject* ret=abstract_d(divide(v2, v1));
				context->runtime_stack_push(ret);
				break;
			}
			case 0xa4:
			{
				//modulo
				ASObject* v2=context->runtime_stack_pop();
				ASObject* v1=context->runtime_stack_pop();

				ASObject* ret=abstract_d(modulo(v1, v2));
				context->runtime_stack_push(ret);
				break;
			}
			case 0xa5:
			{
				//lshift
				ASObject* v1=context->runtime_stack_pop();
				ASObject* v2=context->runtime_stack_pop();

				ASObject* ret=abstract_i(lShift(v1, v2));
				context->runtime_stack_push(ret);
				break;
			}
			case 0xa6:
			{
				//rshift
				ASObject* v1=context->runtime_stack_pop();
				ASObject* v2=context->runtime_stack_pop();

				ASObject* ret=abstract_i(rShift(v1, v2));
				context->runtime_stack_push(ret);
				break;
			}
			case 0xa7:
			{
				//urshift
				ASObject* v1=context->runtime_stack_pop();
				ASObject* v2=context->runtime_stack_pop();

				ASObject* ret=abstract_i(urShift(v1, v2));
				context->runtime_stack_push(ret);
				break;
			}
			case 0xa8:
			{
				//bitand
				ASObject* v1=context->runtime_stack_pop();
				ASObject* v2=context->runtime_stack_pop();

				ASObject* ret=abstract_i(bitAnd(v1, v2));
				context->runtime_stack_push(ret);
				break;
			}
			case 0xa9:
			{
				//bitor
				ASObject* v1=context->runtime_stack_pop();
				ASObject* v2=context->runtime_stack_pop();

				ASObject* ret=abstract_i(bitOr(v1, v2));
				context->runtime_stack_push(ret);
				break;
			}
			case 0xaa:
			{
				//bitxor
				ASObject* v1=context->runtime_stack_pop();
				ASObject* v2=context->runtime_stack_pop();

				ASObject* ret=abstract_i(bitXor(v1, v2));
				context->runtime_stack_push(ret);
				break;
			}
			case 0xab:
			{
				//equals
				ASObject* v2=context->runtime_stack_pop();
				ASObject* v1=context->runtime_stack_pop();

				ASObject* ret=abstract_b(equals(v1, v2));
				context->runtime_stack_push(ret);
				break;
			}
			case 0xac:
			{
				//strictequals
				ASObject* v2=context->runtime_stack_pop();
				ASObject* v1=context->runtime_stack_pop();

				ASObject* ret=abstract_b(strictEquals(v1, v2));
				context->runtime_stack_push(ret);
				break;
			}
			case 0xad:
			{
				//lessthan
				ASObject* v2=context->runtime_stack_pop();
				ASObject* v1=context->runtime_stack_pop();

				ASObject* ret=abstract_b(lessThan(v1, v2));
				context->runtime_stack_push(ret);
				break;
			}
			case 0xae:
			{
				//lessequals
				ASObject* v2=context->runtime_stack_pop();
				ASObject* v1=context->runtime_stack_pop();

				ASObject* ret=abstract_b(lessEquals(v1, v2));
				context->runtime_stack_push(ret);
				break;
			}
			case 0xaf:
			{
				//greaterthan
				ASObject* v2=context->runtime_stack_pop();
				ASObject* v1=context->runtime_stack_pop();

				ASObject* ret=abstract_b(greaterThan(v1, v2));
				context->runtime_stack_push(ret);
				break;
			}
			case 0xb0:
			{
				//greaterequals
				ASObject* v2=context->runtime_stack_pop();
				ASObject* v1=context->runtime_stack_pop();

				ASObject* ret=abstract_b(greaterEquals(v1, v2));
				context->runtime_stack_push(ret);
				break;
			}
			case 0xb1:
			{
				//instanceof
				ASObject* type=context->runtime_stack_pop();
				ASObject* value=context->runtime_stack_pop();
				bool ret=instanceOf(value, type);
				context->runtime_stack_push(abstract_b(ret));
				break;
			}
			case 0xb2:
			{
				//istype
				u30 t;
				code >> t;
				multiname* name=context->context->getMultiname(t,NULL);

				ASObject* v1=context->runtime_stack_pop();

				ASObject* ret=abstract_b(isType(context->context, v1, name));
				context->runtime_stack_push(ret);
				break;
			}
			case 0xb3:
			{
				//istypelate
				ASObject* v1=context->runtime_stack_pop();
				ASObject* v2=context->runtime_stack_pop();

				ASObject* ret=abstract_b(isTypelate(v1, v2));
				context->runtime_stack_push(ret);
				break;
			}
			case 0xb4:
			{
				//in
				ASObject* v1=context->runtime_stack_pop();
				ASObject* v2=context->runtime_stack_pop();

				ASObject* ret=abstract_b(in(v1, v2));
				context->runtime_stack_push(ret);
				break;
			}
			case 0xc0:
			{
				//increment_i
				ASObject* val=context->runtime_stack_pop();
				ASObject* ret=abstract_i(increment_i(val));
				context->runtime_stack_push(ret);
				break;
			}
			case 0xc1:
			{
				//decrement_i
				ASObject* val=context->runtime_stack_pop();
				ASObject* ret=abstract_i(decrement_i(val));
				context->runtime_stack_push(ret);
				break;
			}
			case 0xc2:
			{
				//inclocal_i
				u30 t;
				code >> t;
				incLocal_i(context, t);
				break;
			}
			case 0xc3:
			{
				//declocal_i
				u30 t;
				code >> t;
				decLocal_i(context, t);
				break;
			}
			case 0xc4:
			{
				//negate_i
				ASObject *val=context->runtime_stack_pop();
				ASObject* ret=abstract_i(negate_i(val));
				context->runtime_stack_push(ret);
				break;
			}
			case 0xc5:
			{
				//add_i
				ASObject* v2=context->runtime_stack_pop();
				ASObject* v1=context->runtime_stack_pop();

				ASObject* ret=abstract_i(add_i(v2, v1));
				context->runtime_stack_push(ret);
				break;
			}
			case 0xc6:
			{
				//subtract_i
				ASObject* v2=context->runtime_stack_pop();
				ASObject* v1=context->runtime_stack_pop();

				ASObject* ret=abstract_i(subtract_i(v2, v1));
				context->runtime_stack_push(ret);
				break;
			}
			case 0xc7:
			{
				//multiply_i
				ASObject* v2=context->runtime_stack_pop();
				ASObject* v1=context->runtime_stack_pop();

				ASObject* ret=abstract_i(multiply_i(v2, v1));
				context->runtime_stack_push(ret);
				break;
			}
			case 0xd0:
			case 0xd1:
			case 0xd2:
			case 0xd3:
			{
				//getlocal_n
				int i=opcode&3;
				assert_and_throw(context->locals[i]);
				LOG(LOG_CALLS, _("getLocal ") << i << _(": ") << context->locals[i]->toDebugString() );
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
				LOG(LOG_CALLS, _("setLocal ") << i );
				ASObject* obj=context->runtime_stack_pop();
				if(context->locals[i])
					context->locals[i]->decRef();
				context->locals[i]=obj;
				break;
			}
			case 0xef:
			{
				//debug
				LOG(LOG_CALLS, _("debug") );
				uint8_t debug_type;
				u30 index;
				uint8_t reg;
				u30 extra;
				code.read((char*)&debug_type,1);
				code >> index;
				code.read((char*)&reg,1);
				code >> extra;
				break;
			}
			case 0xf0:
			{
				//debugline
				LOG(LOG_CALLS, _("debugline") );
				u30 t;
				code >> t;
				break;
			}
			case 0xf1:
			{
				//debugfile
				LOG(LOG_CALLS, _("debugfile") );
				u30 t;
				code >> t;
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
