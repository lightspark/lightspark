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

#include "abc.h"
#include "compat.h"
#include "exceptions.h"
#include "abcutils.h"
#include <string>
#include <sstream>

using namespace std;
using namespace lightspark;

struct OpcodeData
{
	union
	{
		int32_t ints[0];
		uint32_t uints[0];
		double doubles[0];
		ASObject* objs[0];
		const multiname* names[0];
		const Type* types[0];
	};
};

ASObject* ABCVm::executeFunctionFast(const SyntheticFunction* function, call_context* context)
{
	method_info* mi=function->mi;

	const char* const code=&(mi->body->code[0]);
	//This may be non-zero and point to the position of an exception handler

	const uint32_t code_len=mi->body->code.size();
	uint32_t instructionPointer=context->exec_pos;

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
		assert(instructionPointer<code_len);
		uint8_t opcode=code[instructionPointer];
		//Save ip for exception handling in SyntheticFunction::callImpl
		context->exec_pos = instructionPointer;
		instructionPointer++;
		const OpcodeData* data=reinterpret_cast<const OpcodeData*>(code+instructionPointer);

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
				getSuper(context,data->uints[0]);
				instructionPointer+=4;
				break;
			}
			case 0x05:
			{
				//setsuper
				setSuper(context,data->uints[0]);
				instructionPointer+=4;
				break;
			}
			case 0x06:
			{
				//dxns
				dxns(context,data->uints[0]);
				instructionPointer+=4;
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
				uint32_t t=data->uints[0];
				LOG(LOG_CALLS, "kill " << t);
				instructionPointer+=4;
				assert_and_throw(context->locals[t]);
				context->locals[t]->decRef();
				context->locals[t]=getSys()->getUndefinedRef();
				break;
			}
			case 0x0c:
			{
				//ifnlt
				uint32_t dest=data->uints[0];
				instructionPointer+=4;
				ASObject* v1=context->runtime_stack_pop();
				ASObject* v2=context->runtime_stack_pop();
				bool cond=ifNLT(v1, v2);
				if(cond)
				{
					assert(dest < code_len);
					instructionPointer=dest;
				}
				break;
			}
			case 0x0d:
			{
				//ifnle
				uint32_t dest=data->uints[0];
				instructionPointer+=4;
				ASObject* v1=context->runtime_stack_pop();
				ASObject* v2=context->runtime_stack_pop();
				bool cond=ifNLE(v1, v2);
				if(cond)
				{
					assert(dest < code_len);
					instructionPointer=dest;
				}
				break;
			}
			case 0x0e:
			{
				//ifngt
				uint32_t dest=data->uints[0];
				instructionPointer+=4;
				ASObject* v1=context->runtime_stack_pop();
				ASObject* v2=context->runtime_stack_pop();
				bool cond=ifNGT(v1, v2);
				if(cond)
				{
					assert(dest < code_len);
					instructionPointer=dest;
				}
				break;
			}
			case 0x0f:
			{
				//ifnge
				uint32_t dest=data->uints[0];
				instructionPointer+=4;
				ASObject* v1=context->runtime_stack_pop();
				ASObject* v2=context->runtime_stack_pop();
				bool cond=ifNGE(v1, v2);
				if(cond)
				{
					assert(dest < code_len);
					instructionPointer=dest;
				}
				break;
			}
			case 0x10:
			{
				//jump
				uint32_t dest=data->uints[0];
				instructionPointer+=4;

				assert(dest < code_len);
				instructionPointer=dest;
				break;
			}
			case 0x11:
			{
				//iftrue
				uint32_t dest=data->uints[0];
				instructionPointer+=4;

				ASObject* v1=context->runtime_stack_pop();
				bool cond=ifTrue(v1);
				if(cond)
				{
					assert(dest < code_len);
					instructionPointer=dest;
				}
				break;
			}
			case 0x12:
			{
				//iffalse
				uint32_t dest=data->uints[0];
				instructionPointer+=4;

				ASObject* v1=context->runtime_stack_pop();
				bool cond=ifFalse(v1);
				if(cond)
				{
					assert(dest < code_len);
					instructionPointer=dest;
				}
				break;
			}
			case 0x13:
			{
				//ifeq
				uint32_t dest=data->uints[0];
				instructionPointer+=4;

				ASObject* v1=context->runtime_stack_pop();
				ASObject* v2=context->runtime_stack_pop();
				bool cond=ifEq(v1, v2);
				if(cond)
				{
					assert(dest < code_len);
					instructionPointer=dest;
				}
				break;
			}
			case 0x14:
			{
				//ifne
				uint32_t dest=data->uints[0];
				instructionPointer+=4;

				ASObject* v1=context->runtime_stack_pop();
				ASObject* v2=context->runtime_stack_pop();
				bool cond=ifNE(v1, v2);
				if(cond)
				{
					assert(dest < code_len);
					instructionPointer=dest;
				}
				break;
			}
			case 0x15:
			{
				//iflt
				uint32_t dest=data->uints[0];
				instructionPointer+=4;

				ASObject* v1=context->runtime_stack_pop();
				ASObject* v2=context->runtime_stack_pop();
				bool cond=ifLT(v1, v2);
				if(cond)
				{
					assert(dest < code_len);
					instructionPointer=dest;
				}
				break;
			}
			case 0x16:
			{
				//ifle
				uint32_t dest=data->uints[0];
				instructionPointer+=4;

				ASObject* v1=context->runtime_stack_pop();
				ASObject* v2=context->runtime_stack_pop();
				bool cond=ifLE(v1, v2);
				if(cond)
				{
					assert(dest < code_len);
					instructionPointer=dest;
				}
				break;
			}
			case 0x17:
			{
				//ifgt
				uint32_t dest=data->uints[0];
				instructionPointer+=4;

				ASObject* v1=context->runtime_stack_pop();
				ASObject* v2=context->runtime_stack_pop();
				bool cond=ifGT(v1, v2);
				if(cond)
				{
					assert(dest < code_len);
					instructionPointer=dest;
				}
				break;
			}
			case 0x18:
			{
				//ifge
				uint32_t dest=data->uints[0];
				instructionPointer+=4;

				ASObject* v1=context->runtime_stack_pop();
				ASObject* v2=context->runtime_stack_pop();
				bool cond=ifGE(v1, v2);
				if(cond)
				{
					assert(dest < code_len);
					instructionPointer=dest;
				}
				break;
			}
			case 0x19:
			{
				//ifstricteq
				uint32_t dest=data->uints[0];
				instructionPointer+=4;

				ASObject* v1=context->runtime_stack_pop();
				ASObject* v2=context->runtime_stack_pop();
				bool cond=ifStrictEq(v1, v2);
				if(cond)
				{
					assert(dest < code_len);
					instructionPointer=dest;
				}
				break;
			}
			case 0x1a:
			{
				//ifstrictne
				uint32_t dest=data->uints[0];
				instructionPointer+=4;

				ASObject* v1=context->runtime_stack_pop();
				ASObject* v2=context->runtime_stack_pop();
				bool cond=ifStrictNE(v1, v2);
				if(cond)
				{
					assert(dest < code_len);
					instructionPointer=dest;
				}
				break;
			}
			case 0x1b:
			{
				//lookupswitch
				uint32_t defaultdest=data->uints[0];
				LOG(LOG_CALLS,_("Switch default dest ") << defaultdest);
				uint32_t count=data->uints[1];

				ASObject* index_obj=context->runtime_stack_pop();
				assert_and_throw(index_obj->getObjectType()==T_INTEGER);
				unsigned int index=index_obj->toUInt();
				index_obj->decRef();

				uint32_t dest=defaultdest;
				if(index<=count)
					dest=data->uints[2+index];

				assert(dest < code_len);
				instructionPointer=dest;
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
				int8_t t=code[instructionPointer];
				instructionPointer++;
				context->runtime_stack_push(abstract_i(t));
				pushByte(t);
				break;
			}
			case 0x25:
			{
				//pushshort
				// specs say pushshort is a u30, but it's really a u32
				// see https://bugs.adobe.com/jira/browse/ASC-4181
				uint32_t t=data->uints[0];
				instructionPointer+=4;
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
				context->runtime_stack_push(pushString(context,data->uints[0]));
				instructionPointer+=4;
				break;
			}
			case 0x2d:
			{
				//pushint
				int32_t t=data->ints[0];
				instructionPointer+=4;
				pushInt(context, t);
				ASObject* i=abstract_i(t);
				context->runtime_stack_push(i);
				break;
			}
			case 0x2e:
			{
				//pushuint
				uint32_t t=data->uints[0];
				instructionPointer+=4;
				pushUInt(context, t);

				ASObject* i=abstract_ui(t);
				context->runtime_stack_push(i);
				break;
			}
			case 0x2f:
			{
				//pushdouble
				double t=data->doubles[0];
				instructionPointer+=8;
				pushDouble(context, t);

				ASObject* d=abstract_d(t);
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
				context->runtime_stack_push( pushNamespace(context, data->uints[0]) );
				instructionPointer+=4;
				break;
			}
			case 0x32:
			{
				//hasnext2
				uint32_t t=data->uints[0];
				uint32_t t2=data->uints[1];
				instructionPointer+=8;

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
				context->runtime_stack_push(newFunction(context,data->uints[0]));
				instructionPointer+=4;
				break;
			}
			case 0x41:
			{
				//call
				uint32_t t=data->uints[0];
				method_info* called_mi=NULL;
				PROF_ACCOUNT_TIME(mi->profTime[instructionPointer],profilingCheckpoint(startTime));
				call(context,t,&called_mi);
				if(called_mi)
					PROF_ACCOUNT_TIME(mi->profCalls[called_mi],profilingCheckpoint(startTime));
				else
					PROF_IGNORE_TIME(profilingCheckpoint(startTime));
				instructionPointer+=4;
				break;
			}
			case 0x42:
			{
				//construct
				construct(context,data->uints[0]);
				instructionPointer+=4;
				break;
			}
			case 0x45:
			{
				//callsuper
				uint32_t t=data->uints[0];
				uint32_t t2=data->uints[1];
				method_info* called_mi=NULL;
				PROF_ACCOUNT_TIME(mi->profTime[instructionPointer],profilingCheckpoint(startTime));
				callSuper(context,t,t2,&called_mi,true);
				if(called_mi)
					PROF_ACCOUNT_TIME(mi->profCalls[called_mi],profilingCheckpoint(startTime));
				else
					PROF_IGNORE_TIME(profilingCheckpoint(startTime));
				instructionPointer+=8;
				break;
			}
			case 0x46:
			case 0x4c: //callproplex seems to be exactly like callproperty
			{
				//callproperty
				uint32_t t=data->uints[0];
				uint32_t t2=data->uints[1];
				method_info* called_mi=NULL;
				PROF_ACCOUNT_TIME(mi->profTime[instructionPointer],profilingCheckpoint(startTime));
				callProperty(context,t,t2,&called_mi,true);
				if(called_mi)
					PROF_ACCOUNT_TIME(mi->profCalls[called_mi],profilingCheckpoint(startTime));
				else
					PROF_IGNORE_TIME(profilingCheckpoint(startTime));
				instructionPointer+=8;
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
				constructSuper(context,data->uints[0]);
				instructionPointer+=4;
				break;
			}
			case 0x4a:
			{
				//constructprop
				uint32_t t=data->uints[0];
				uint32_t t2=data->uints[1];
				instructionPointer+=8;
				constructProp(context,t,t2);
				break;
			}
			case 0x4e:
			{
				//callsupervoid
				uint32_t t=data->uints[0];
				uint32_t t2=data->uints[1];
				method_info* called_mi=NULL;
				PROF_ACCOUNT_TIME(mi->profTime[instructionPointer],profilingCheckpoint(startTime));
				callSuper(context,t,t2,&called_mi,false);
				if(called_mi)
					PROF_ACCOUNT_TIME(mi->profCalls[called_mi],profilingCheckpoint(startTime));
				else
					PROF_IGNORE_TIME(profilingCheckpoint(startTime));
				instructionPointer+=8;
				break;
			}
			case 0x4f:
			{
				//callpropvoid
				uint32_t t=data->uints[0];
				uint32_t t2=data->uints[1];
				method_info* called_mi=NULL;
				PROF_ACCOUNT_TIME(mi->profTime[instructionPointer],profilingCheckpoint(startTime));
				callProperty(context,t,t2,&called_mi,false);
				if(called_mi)
					PROF_ACCOUNT_TIME(mi->profCalls[called_mi],profilingCheckpoint(startTime));
				else
					PROF_IGNORE_TIME(profilingCheckpoint(startTime));
				instructionPointer+=8;
				break;
			}
			case 0x53:
			{
				//constructgenerictype
				constructGenericType(context, data->uints[0]);
				instructionPointer+=4;
				break;
			}
			case 0x55:
			{
				//newobject
				newObject(context,data->uints[0]);
				instructionPointer+=4;
				break;
			}
			case 0x56:
			{
				//newarray
				newArray(context,data->uints[0]);
				instructionPointer+=4;
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
				newClass(context,data->uints[0]);
				instructionPointer+=4;
				break;
			}
			case 0x59:
			{
				//getdescendants
				getDescendants(context, data->uints[0]);
				instructionPointer+=4;
				break;
			}
			case 0x5a:
			{
				//newcatch
				context->runtime_stack_push(newCatch(context,data->uints[0]));
				instructionPointer+=4;
				break;
			}
			case 0x5d:
			{
				//findpropstrict
				uint32_t t=data->uints[0];
				instructionPointer+=4;
				multiname* name=context->context->getMultiname(t,context);
				context->runtime_stack_push(findPropStrict(context,name));
				name->resetNameIfObject();
				break;
			}
			case 0x5e:
			{
				//findproperty
				uint32_t t=data->uints[0];
				instructionPointer+=4;
				multiname* name=context->context->getMultiname(t,context);
				context->runtime_stack_push(findProperty(context,name));
				name->resetNameIfObject();
				break;
			}
			case 0x60:
			{
				//getlex
				uint32_t t=data->uints[0];
				instructionPointer+=4;
				getLex(context,t);
				break;
			}
			case 0x61:
			{
				//setproperty
				uint32_t t=data->uints[0];
				instructionPointer+=4;
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
				uint32_t i=data->uints[0];
				instructionPointer+=4;
				assert_and_throw(context->locals[i]);
				context->locals[i]->incRef();
				LOG(LOG_CALLS, _("getLocal ") << i << _(": ") << context->locals[i]->toDebugString() );
				context->runtime_stack_push(context->locals[i]);
				break;
			}
			case 0x63:
			{
				//setlocal
				uint32_t i=data->uints[0];
				instructionPointer+=4;
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
				uint32_t t=data->uints[0];
				instructionPointer+=4;
				context->runtime_stack_push(getScopeObject(context,t));
				break;
			}
			case 0x66:
			{
				//getproperty
				uint32_t t=data->uints[0];
				instructionPointer+=4;
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
				uint32_t t=data->uints[0];
				instructionPointer+=4;
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
				uint32_t t=data->uints[0];
				instructionPointer+=4;
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
				uint32_t t=data->uints[0];
				instructionPointer+=4;
				ASObject* obj=context->runtime_stack_pop();
				ASObject* ret=getSlot(obj, t);
				context->runtime_stack_push(ret);
				break;
			}
			case 0x6d:
			{
				//setslot
				uint32_t t=data->uints[0];
				instructionPointer+=4;

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
				const multiname* name=data->names[0];
				char* rewriteableCode = &(mi->body->code[0]);
				const Type* type = Type::getTypeFromMultiname(name, context->context);
				OpcodeData* rewritableData=reinterpret_cast<OpcodeData*>(rewriteableCode+instructionPointer);
				//Rewrite this to a coerceEarly
				rewriteableCode[instructionPointer-1]=0xfc;
				rewritableData->types[0]=type;

				LOG(LOG_CALLS,"coerceOnce " << *name);

				ASObject* o=context->runtime_stack_pop();
				o=type->coerce(o);
				context->runtime_stack_push(o);

				instructionPointer+=8;
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
				uint32_t t=data->uints[0];
				instructionPointer+=4;
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
				uint32_t t=data->uints[0];
				instructionPointer+=4;
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
				uint32_t t=data->uints[0];
				instructionPointer+=4;
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
				uint32_t t=data->uints[0];
				instructionPointer+=4;
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
				uint32_t t=data->uints[0];
				instructionPointer+=4;
				incLocal_i(context, t);
				break;
			}
			case 0xc3:
			{
				//declocal_i
				uint32_t t=data->uints[0];
				instructionPointer+=4;
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
				LOG(LOG_CALLS, "getLocal " << i << ": " << context->locals[i]->toDebugString() );
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
				LOG(LOG_CALLS, "setLocal " << i );
				ASObject* obj=context->runtime_stack_pop();
				if(context->locals[i])
					context->locals[i]->decRef();
				context->locals[i]=obj;
				break;
			}
			//lightspark custom opcodes
			case 0xfb:
			{
				//setslot_no_coerce
				uint32_t t=data->uints[0];
				instructionPointer+=4;

				ASObject* value=context->runtime_stack_pop();
				ASObject* obj=context->runtime_stack_pop();

				LOG(LOG_CALLS,"setSlotNoCoerce " << t);
				obj->setSlotNoCoerce(t,value);
				obj->decRef();
				break;
			}
			case 0xfc:
			{
				//coerceearly
				const Type* type = data->types[0];
				LOG(LOG_CALLS,"coerceEarly " << type);

				ASObject* o=context->runtime_stack_pop();
				o=type->coerce(o);
				context->runtime_stack_push(o);

				instructionPointer+=8;
				break;
			}
			case 0xfd:
			{
				//getscopeatindex
				//This opcode is similar to getscopeobject, but it allows access to any
				//index of the scope stack
				uint32_t t=data->uints[0];
				LOG(LOG_CALLS, "getScopeAtIndex " << t);
				assert(t<context->scope_stack.size());
				ASObject* obj=context->scope_stack[t].object.getPtr();
				obj->incRef();
				context->runtime_stack_push(obj);
				instructionPointer+=4;
				break;
			}
			case 0xfe:
			{
				//getlexonce
				//This opcode execute a lookup on the application domain
				//and rewrites itself to a pushearly
				const multiname* name=data->names[0];
				LOG(LOG_CALLS, "getLexOnce " << *name);
				ASObject* target;
				ASObject* obj=ABCVm::getCurrentApplicationDomain(context)->getVariableAndTargetByMultiname(*name,target);
				//The object must exists, since it was found during optimization
				assert_and_throw(obj);
				char* rewriteableCode = &(mi->body->code[0]);
				OpcodeData* rewritableData=reinterpret_cast<OpcodeData*>(rewriteableCode+instructionPointer);
				//Rewrite this to a pushearly
				rewriteableCode[instructionPointer-1]=0xff;
				rewritableData->objs[0]=obj;
				//Also push the object right away
				obj->incRef();
				context->runtime_stack_push(obj);
				//Move to the next instruction
				instructionPointer+=8;
				break;
			}
			case 0xff:
			{
				//pushearly
				ASObject* o=data->objs[0];
				instructionPointer+=8;
				LOG(LOG_CALLS, "pushEarly " << o);
				o->incRef();
				context->runtime_stack_push(o);
				break;
			}
			default:
				LOG(LOG_ERROR,_("Not interpreted instruction @") << instructionPointer);
				LOG(LOG_ERROR,_("dump ") << hex << (unsigned int)opcode << dec);
				throw ParseException("Not implemented instruction in fast interpreter");
		}
		PROF_ACCOUNT_TIME(mi->profTime[instructionPointer],profilingCheckpoint(startTime));
	}

#undef PROF_ACCOUNT_TIME 
#undef PROF_IGNORE_TIME
	//We managed to execute all the function
	return context->runtime_stack_pop();
}
