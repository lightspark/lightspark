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

#include "abc.h"
#include "compat.h"
#include "exceptions.h"
#include "abcutils.h"
#include <string>
#include <sstream>
#include "scripting/class.h"
#include "scripting/toplevel/Number.h"
#include "scripting/flash/system/flashsystem.h"

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
		multiname* names[0];
		const Type* types[0];
	};
};

ASObject* ABCVm::executeFunctionFast(const SyntheticFunction* function, call_context* context, ASObject *caller)
{
	method_info* mi=function->mi;

	const char* const code=&(mi->body->code[0]);
	//This may be non-zero and point to the position of an exception handler

#if defined (PROFILING_SUPPORT) || !defined(NDEBUG)
	const uint32_t code_len=mi->body->code.size();
#endif
	uint32_t instructionPointer=context->exec_pos-context->mi->body->preloadedcode.data();

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
		context->exec_pos = mi->body->preloadedcode.data()+instructionPointer;
		instructionPointer++;
		const OpcodeData* data=reinterpret_cast<const OpcodeData*>(code+instructionPointer);

		switch(opcode)
		{
			case 0x01:
			{
				//bkpt
				LOG_CALL( "bkpt" );
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
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v);
				dxnslate(context, v);
				break;
			}
			case 0x08:
			{
				//kill
				uint32_t t=data->uints[0];
				LOG_CALL( "kill " << t);
				instructionPointer+=4;
				ASATOM_DECREF(context->locals[t]);
				context->locals[t]=asAtomHandler::fromObject(function->getSystemState()->getUndefinedRef());
				break;
			}
			case 0x0c:
			{
				//ifnlt
				uint32_t dest=data->uints[0];
				instructionPointer+=4;
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v1);
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v2);
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
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v1);
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v2);
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
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v1);
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v2);
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
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v1);
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v2);
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

				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v1);
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

				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v1);
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

				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v1);
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v2);
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

				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v1);
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v2);
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

				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v1);
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v2);
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

				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v1);
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v2);
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

				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v1);
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v2);
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

				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v1);
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v2);
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

				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v1);
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v2);
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

				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v1);
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v2);
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
				LOG_CALL("Switch default dest " << defaultdest);
				uint32_t count=data->uints[1];

				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,index_obj);
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
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v1);
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v2);
				RUNTIME_STACK_PUSH(context,asAtomHandler::fromObject(nextName(v1,v2)));
				break;
			}
			case 0x1f:
			{
				//hasnext
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v1);
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v2);
				RUNTIME_STACK_PUSH(context,asAtomHandler::fromObject(hasNext(v1,v2)));
				break;
			}
			case 0x20:
			{
				//pushnull
				RUNTIME_STACK_PUSH(context,asAtomHandler::fromObject(pushNull()));
				break;
			}
			case 0x21:
			{
				//pushundefined
				RUNTIME_STACK_PUSH(context,asAtomHandler::fromObject(pushUndefined()));
				break;
			}
			case 0x23:
			{
				//nextvalue
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v1);
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v2);
				RUNTIME_STACK_PUSH(context,asAtomHandler::fromObject(nextValue(v1,v2)));
				break;
			}
			case 0x24:
			{
				//pushbyte
				int8_t t=code[instructionPointer];
				instructionPointer++;
				RUNTIME_STACK_PUSH(context,asAtomHandler::fromInt((int32_t)t));
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
				RUNTIME_STACK_PUSH(context,asAtomHandler::fromInt((int32_t)t));
				pushShort(t);
				break;
			}
			case 0x26:
			{
				//pushtrue
				RUNTIME_STACK_PUSH(context,asAtomHandler::trueAtom);
				break;
			}
			case 0x27:
			{
				//pushfalse
				RUNTIME_STACK_PUSH(context,asAtomHandler::falseAtom);
				break;
			}
			case 0x28:
			{
				//pushnan
				RUNTIME_STACK_PUSH(context,asAtomHandler::fromNumber(context->worker, Number::NaN,false));
				break;
			}
			case 0x29:
			{
				//pop
				pop();
				RUNTIME_STACK_POP_CREATE(context,o);
				ASATOM_DECREF_POINTER(o);
				break;
			}
			case 0x2a:
			{
				//dup
				dup();
				RUNTIME_STACK_PEEK_CREATE(context,o);
				ASATOM_INCREF_POINTER(o);
				RUNTIME_STACK_PUSH(context,*o);
				break;
			}
			case 0x2b:
			{
				//swap
				swap();
				asAtom v1=asAtomHandler::invalidAtom;
				RUNTIME_STACK_POP(context,v1);
				asAtom v2=asAtomHandler::invalidAtom;
				RUNTIME_STACK_POP(context,v2);

				RUNTIME_STACK_PUSH(context,v1);
				RUNTIME_STACK_PUSH(context,v2);
				break;
			}
			case 0x2c:
			{
				//pushstring
				RUNTIME_STACK_PUSH(context,asAtomHandler::fromObject(pushString(context,data->uints[0])));
				instructionPointer+=4;
				break;
			}
			case 0x2d:
			{
				//pushint
				int32_t t=data->ints[0];
				instructionPointer+=4;
				pushInt(context, t);
				RUNTIME_STACK_PUSH(context,asAtomHandler::fromInt(t));
				break;
			}
			case 0x2e:
			{
				//pushuint
				uint32_t t=data->uints[0];
				instructionPointer+=4;
				pushUInt(context, t);

				RUNTIME_STACK_PUSH(context,asAtomHandler::fromUInt(t));
				break;
			}
			case 0x2f:
			{
				//pushdouble
				double t=data->doubles[0];
				instructionPointer+=8;
				pushDouble(context, t);

				ASObject* d=abstract_d(context->worker,t);
				RUNTIME_STACK_PUSH(context,asAtomHandler::fromObject(d));
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
				RUNTIME_STACK_PUSH(context,asAtomHandler::fromObject(pushNamespace(context, data->uints[0]) ));
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
				RUNTIME_STACK_PUSH(context,asAtomHandler::fromBool(ret));
				break;
			}
			//Alchemy opcodes
			case 0x35:
			{
				//li8
				LOG_CALL( "li8");
				ApplicationDomain::loadIntN<uint8_t>(context->mi->context->root->applicationDomain.getPtr(), context);
				break;
			}
			case 0x36:
			{
				//li16
				LOG_CALL( "li16");
				ApplicationDomain::loadIntN<uint16_t>(context->mi->context->root->applicationDomain.getPtr(), context);
				break;
			}
			case 0x37:
			{
				//li32
				LOG_CALL( "li32");
				ApplicationDomain::loadIntN<uint32_t>(context->mi->context->root->applicationDomain.getPtr(), context);
				break;
			}
			case 0x38:
			{
				//lf32
				LOG_CALL( "lf32");
				ApplicationDomain::loadFloat(context->mi->context->root->applicationDomain.getPtr(), context);
				break;
			}
			case 0x39:
			{
				//lf32
				LOG_CALL( "lf64");
				ApplicationDomain::loadDouble(context->mi->context->root->applicationDomain.getPtr(), context);
				break;
			}
			case 0x3a:
			{
				//si8
				LOG_CALL( "si8");
				ApplicationDomain::storeIntN<uint8_t>(context->mi->context->root->applicationDomain.getPtr(), context);
				break;
			}
			case 0x3b:
			{
				//si16
				LOG_CALL( "si16");
				ApplicationDomain::storeIntN<uint16_t>(context->mi->context->root->applicationDomain.getPtr(), context);
				break;
			}
			case 0x3c:
			{
				//si32
				LOG_CALL( "si32");
				ApplicationDomain::storeIntN<uint32_t>(context->mi->context->root->applicationDomain.getPtr(), context);
				break;
			}
			case 0x3d:
			{
				//sf32
				LOG_CALL( "sf32");
				ApplicationDomain::storeFloat(context->mi->context->root->applicationDomain.getPtr(), context);
				break;
			}
			case 0x3e:
			{
				//sf32
				LOG_CALL( "sf64");
				ApplicationDomain::storeDouble(context->mi->context->root->applicationDomain.getPtr(), context);
				break;
			}
			case 0x40:
			{
				//newfunction
				RUNTIME_STACK_PUSH(context,asAtomHandler::fromObject(newFunction(context,data->uints[0])));
				instructionPointer+=4;
				break;
			}
			case 0x41:
			{
				//call
				uint32_t t=data->uints[0];
				method_info* called_mi=nullptr;
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
			case 0x43:
			{
				//callmethod
				uint32_t t=data->uints[0];
				uint32_t t2=data->uints[1];
				callMethod(context,t,t2);
				instructionPointer+=8;
				break;
			}
			case 0x44:
			{
				//callstatic
				uint32_t t=data->uints[0];
				uint32_t t2=data->uints[1];
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
			case 0x4c: 
			{
				//callproplex
				uint32_t t=data->uints[0];
				uint32_t t2=data->uints[1];
				method_info* called_mi=NULL;
				PROF_ACCOUNT_TIME(mi->profTime[instructionPointer],profilingCheckpoint(startTime));
				callPropLex(context,t,t2,&called_mi,true);
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
				LOG_CALL("returnVoid");
				PROF_ACCOUNT_TIME(mi->profTime[instructionPointer],profilingCheckpoint(startTime));
				return NULL;
			}
			case 0x48:
			{
				//returnvalue
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,ret);
				LOG_CALL("returnValue " << ret);
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
			case 0x50:
			{
				//sxi1
				LOG_CALL( "sxi1");
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,arg1);
				int32_t ret=arg1->toUInt() & 0x1;
				arg1->decRef();
				RUNTIME_STACK_PUSH(context,asAtomHandler::fromInt(ret));
				break;
			}
			case 0x51:
			{
				//sxi8
				LOG_CALL( "sxi8");
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,arg1);
				int32_t ret=(int8_t)arg1->toUInt();
				arg1->decRef();
				RUNTIME_STACK_PUSH(context,asAtomHandler::fromInt(ret));
				break;
			}
			case 0x52:
			{
				//sxi16
				LOG_CALL( "sxi16");
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,arg1);
				int32_t ret=(int16_t)arg1->toUInt();
				arg1->decRef();
				RUNTIME_STACK_PUSH(context,asAtomHandler::fromInt(ret));
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
				RUNTIME_STACK_PUSH(context,asAtomHandler::fromObject(newActivation(context, mi)));
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
				RUNTIME_STACK_PUSH(context,asAtomHandler::fromObject(newCatch(context,data->uints[0])));
				instructionPointer+=4;
				break;
			}
			case 0x5d:
			{
				//findpropstrict
				uint32_t t=data->uints[0];
				instructionPointer+=4;
				multiname* name=context->mi->context->getMultiname(t,context);
				RUNTIME_STACK_PUSH(context,asAtomHandler::fromObject(findPropStrict(context,name)));
				name->resetNameIfObject();
				break;
			}
			case 0x5e:
			{
				//findproperty
				uint32_t t=data->uints[0];
				instructionPointer+=4;
				multiname* name=context->mi->context->getMultiname(t,context);
				RUNTIME_STACK_PUSH(context,asAtomHandler::fromObject(findProperty(context,name)));
				name->resetNameIfObject();
				break;
			}
			case 0x5f:
			{
				//finddef
				uint32_t t=data->uints[0];
				instructionPointer+=4;
				multiname* name=context->mi->context->getMultiname(t,context);
				LOG(LOG_NOT_IMPLEMENTED,"opcode 0x5f (finddef) not implemented:"<< *name);
				RUNTIME_STACK_PUSH(context,asAtomHandler::fromObject(pushNull()));
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
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,value);

				multiname* name=context->mi->context->getMultiname(t,context);

				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,obj);

				setProperty(value,obj,name);
				name->resetNameIfObject();
				break;
			}
			case 0x62:
			{
				//getlocal
				uint32_t i=data->uints[0];
				instructionPointer+=4;
				if (asAtomHandler::isInvalid(context->locals[i]))
				{
					LOG_CALL( "getLocal " << i << " not set, pushing Undefined");
					RUNTIME_STACK_PUSH(context,asAtomHandler::fromObject(function->getSystemState()->getUndefinedRef()));
					break;
				}
				ASATOM_INCREF(context->locals[i]);
				LOG_CALL( "getLocal " << i << ": " << asAtomHandler::toDebugString(context->locals[i]) );
				RUNTIME_STACK_PUSH(context,context->locals[i]);
				break;
			}
			case 0x63:
			{
				//setlocal
				uint32_t i=data->uints[0];
				instructionPointer+=4;
				LOG_CALL( "setLocal " << i );
				RUNTIME_STACK_POP_CREATE(context,obj)
				if ((int)i != context->argarrayposition || asAtomHandler::isArray(*obj))
				{
					ASATOM_DECREF(context->locals[i]);
					context->locals[i]=*obj;
				}
				break;
			}
			case 0x64:
			{
				//getglobalscope
				RUNTIME_STACK_PUSH(context,asAtomHandler::fromObject(getGlobalScope(context)));
				break;
			}
			case 0x65:
			{
				//getscopeobject
				uint32_t t=data->uints[0];
				instructionPointer+=4;
				assert_and_throw(context->curr_scope_stack > t);
				asAtom ret=context->scope_stack[t];
				ASATOM_INCREF(ret);
				LOG_CALL( "getScopeObject: " << asAtomHandler::toDebugString(ret));

				RUNTIME_STACK_PUSH(context,ret);
				break;
			}
			case 0x66:
			{
				//getproperty
				uint32_t t=data->uints[0];
				instructionPointer+=4;
				multiname* name=context->mi->context->getMultiname(t,context);

				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,obj);

				ASObject* ret=getProperty(obj,name);
				name->resetNameIfObject();

				RUNTIME_STACK_PUSH(context,asAtomHandler::fromObject(ret));
				break;
			}
			case 0x68:
			{
				//initproperty
				uint32_t t=data->uints[0];
				instructionPointer+=4;
				RUNTIME_STACK_POP_CREATE(context,value);
				multiname* name=context->mi->context->getMultiname(t,context);
				LOG_CALL("initProperty "<<*name);
				RUNTIME_STACK_POP_CREATE(context,obj);
				asAtomHandler::toObject(*obj,context->worker)->setVariableByMultiname(*name,*value,ASObject::CONST_ALLOWED,nullptr,context->worker);
				ASATOM_DECREF_POINTER(obj);
				name->resetNameIfObject();
				break;
			}
			case 0x6a:
			{
				//deleteproperty
				uint32_t t=data->uints[0];
				instructionPointer+=4;
				multiname* name = context->mi->context->getMultiname(t,context);
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,obj);
				bool ret = deleteProperty(obj,name);
				name->resetNameIfObject();
				RUNTIME_STACK_PUSH(context,asAtomHandler::fromBool(ret));
				break;
			}
			case 0x6c:
			{
				//getslot
				uint32_t t=data->uints[0];
				instructionPointer+=4;
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,obj);
				ASObject* ret=getSlot(context,obj, t);
				RUNTIME_STACK_PUSH(context,asAtomHandler::fromObject(ret));
				break;
			}
			case 0x6d:
			{
				//setslot
				uint32_t t=data->uints[0];
				instructionPointer+=4;

				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v1);
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v2);

				setSlot(context,v1, v2, t);
				break;
			}
			case 0x6e:
			{
				//getglobalSlot
				uint32_t t=data->uints[0];
				instructionPointer+=4;

				Global* globalscope = getGlobalScope(context);
				RUNTIME_STACK_PUSH(context,globalscope->getSlot(t));
				break;
			}
			case 0x6f:
			{
				//setglobalSlot
				uint32_t t=data->uints[0];
				instructionPointer+=4;

				Global* globalscope = getGlobalScope(context);
				RUNTIME_STACK_POP_CREATE(context,obj);
				globalscope->setSlot(context->worker,t-1,*obj);
				break;
			}
			case 0x70:
			{
				//convert_s
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,val);
				RUNTIME_STACK_PUSH(context,asAtomHandler::fromObject(convert_s(val)));
				break;
			}
			case 0x71:
			{
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,val);
				RUNTIME_STACK_PUSH(context,asAtomHandler::fromObject(esc_xelem(val)));
				break;
			}
			case 0x72:
			{
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,val);
				RUNTIME_STACK_PUSH(context,asAtomHandler::fromObject(esc_xattr(val)));
				break;
			}case 0x73:
			{
				//convert_i
				RUNTIME_STACK_POP_CREATE(context,pval);
				if(!asAtomHandler::isInteger(*pval))
				{
					int32_t v= asAtomHandler::toIntStrict(*pval);
					ASATOM_DECREF_POINTER(pval);
					asAtomHandler::setInt(*pval,context->worker,v);
				}
				break;
			}
			case 0x74:
			{
				//convert_u
				RUNTIME_STACK_POP_CREATE(context,pval);
				if(!asAtomHandler::isUInteger(*pval))
				{
					uint32_t v= asAtomHandler::toUInt(*pval);
					ASATOM_DECREF_POINTER(pval);
					asAtomHandler::setUInt(*pval,context->worker,v);
				}
				break;
			}
			case 0x75:
			{
				//convert_d
				RUNTIME_STACK_POP_CREATE(context,pval);
				if(!asAtomHandler::isNumeric(*pval))
				{
					number_t v= asAtomHandler::toNumber(*pval);
					ASATOM_DECREF_POINTER(pval);
					asAtomHandler::setNumber(*pval,context->worker,v);
				}
				break;
			}
			case 0x76:
			{
				//convert_b
				RUNTIME_STACK_POP_CREATE(context,val);
				asAtomHandler::convert_b(*val,true);
				break;
			}
			case 0x77:
			{
				//convert_o
				RUNTIME_STACK_PEEK_CREATE(context,val);
				if (asAtomHandler::isNull(*val))
				{
					RUNTIME_STACK_POP_CREATE(context,ret);
					(void)ret;
					LOG(LOG_ERROR,"trying to call convert_o on null");
					createError<TypeError>(context->worker,kConvertNullToObjectError);
				}
				if (asAtomHandler::isUndefined(*val))
				{
					RUNTIME_STACK_POP_CREATE(context,ret);
					(void)ret;
					LOG(LOG_ERROR,"trying to call convert_o on undefined");
					createError<TypeError>(context->worker,kConvertUndefinedToObjectError);
				}
				break;
			}
			case 0x78:
			{
				//checkfilter
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,val);
				RUNTIME_STACK_PUSH(context,asAtomHandler::fromObject(checkfilter(val)));
				break;
			}
			case 0x80:
			{
				//coerce
				multiname* name=data->names[0];
				char* rewriteableCode = &(mi->body->code[0]);
				const Type* type = Type::getTypeFromMultiname(name, context->mi->context);
				OpcodeData* rewritableData=reinterpret_cast<OpcodeData*>(rewriteableCode+instructionPointer);
				//Rewrite this to a coerceEarly
				rewriteableCode[instructionPointer-1]=0xfc;
				rewritableData->types[0]=type;

				LOG_CALL("coerceOnce " << *name);

				RUNTIME_STACK_POP_CREATE(context,o);
				asAtom v = *o;
				if (type->coerce(function->getInstanceWorker(),*o))
					ASATOM_DECREF(v);
				RUNTIME_STACK_PUSH(context,*o);

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
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,val);
				RUNTIME_STACK_PUSH(context,asAtomHandler::fromObject(val->is<ASString>() ? val : coerce_s(val)));
				break;
			}
			case 0x86:
			{
				//astype
				uint32_t t=data->uints[0];
				instructionPointer+=4;
				multiname* name=context->mi->context->getMultiname(t,NULL);

				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v1);

				ASObject* ret=asType(context->mi->context, v1, name);
				RUNTIME_STACK_PUSH(context,asAtomHandler::fromObject(ret));
				break;
			}
			case 0x87:
			{
				//astypelate
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v1);
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v2);

				ASObject* ret=asTypelate(v1, v2);
				RUNTIME_STACK_PUSH(context,asAtomHandler::fromObject(ret));
				break;
			}
			case 0x90:
			{
				//negate
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,val);
				ASObject* ret;
				if ((val->is<Integer>() || val->is<UInteger>() || (val->is<Number>() && !val->as<Number>()->isfloat)) && val->toInt64() != 0 && val->toInt64() == val->toInt())
					ret=abstract_di(context->worker,negate_i(val));
				else
					ret=abstract_d(context->worker,negate(val));
				RUNTIME_STACK_PUSH(context,asAtomHandler::fromObject(ret));
				break;
			}
			case 0x91:
			{
				//increment
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,val);
				ASObject* ret;
				if (val->is<Integer>() || (val->is<Number>() && !val->as<Number>()->isfloat))
					ret=abstract_di(context->worker,increment_i(val));
				else
					ret=abstract_d(context->worker,increment(val));
				RUNTIME_STACK_PUSH(context,asAtomHandler::fromObject(ret));
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
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,val);
				ASObject* ret;
				if (val->is<Integer>() || val->is<UInteger>() || (val->is<Number>() && !val->as<Number>()->isfloat))
					ret=abstract_di(context->worker,decrement_di(val));
				else
					ret=abstract_d(context->worker,decrement(val));
				RUNTIME_STACK_PUSH(context,asAtomHandler::fromObject(ret));
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
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,val);
				ASObject* ret=typeOf(val);
				RUNTIME_STACK_PUSH(context,asAtomHandler::fromObject(ret));
				break;
			}
			case 0x96:
			{
				//not
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,val);
				ASObject* ret=abstract_b(context->sys,_not(val));
				RUNTIME_STACK_PUSH(context,asAtomHandler::fromObject(ret));
				break;
			}
			case 0x97:
			{
				//bitnot
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,val);
				ASObject* ret=abstract_i(context->worker,bitNot(val));
				RUNTIME_STACK_PUSH(context,asAtomHandler::fromObject(ret));
				break;
			}
			case 0xa0:
			{
				//add
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v2);
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v1);

				ASObject* ret=add(v2, v1);
				RUNTIME_STACK_PUSH(context,asAtomHandler::fromObject(ret));
				break;
			}
			case 0xa1:
			{
				//subtract
				//Be careful, operands in subtract implementation are swapped
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v2);
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v1);

				ASObject* ret;
				// if both values are Integers or int Numbers the result is also an int Number
				if( (v1->is<Integer>() || v1->is<UInteger>() || (v1->is<Number>() && !v1->as<Number>()->isfloat)) &&
					(v2->is<Integer>() || v2->is<UInteger>() || (v2->is<Number>() && !v2->as<Number>()->isfloat)))
				{
					int64_t num1=v1->toInt64();
					int64_t num2=v2->toInt64();
					LOG_CALL("subtractI "  << num1 << '-' << num2);
					v1->decRef();
					v2->decRef();
					ret = abstract_di(context->worker, num1-num2);
				}
				else
					ret=abstract_d(context->worker,subtract(v2, v1));
				RUNTIME_STACK_PUSH(context,asAtomHandler::fromObject(ret));
				break;
			}
			case 0xa2:
			{
				//multiply
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v2);
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v1);

				ASObject* ret;
				// if both values are Integers or int Numbers the result is also an int Number
				if( (v1->is<Integer>() || v1->is<UInteger>() || (v1->is<Number>() && !v1->as<Number>()->isfloat)) &&
					(v2->is<Integer>() || v2->is<UInteger>() || (v2->is<Number>() && !v2->as<Number>()->isfloat)))
				{
					int64_t num1=v1->toInt64();
					int64_t num2=v2->toInt64();
					LOG_CALL("multiplyI "  << num1 << '*' << num2);
					v1->decRef();
					v2->decRef();
					ret = abstract_di(context->worker, num1*num2);
				}
				else
					ret=abstract_d(context->worker,multiply(v2, v1));
				RUNTIME_STACK_PUSH(context,asAtomHandler::fromObject(ret));
				break;
			}
			case 0xa3:
			{
				//divide
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v2);
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v1);

				ASObject* ret=abstract_d(context->worker,divide(v2, v1));
				RUNTIME_STACK_PUSH(context,asAtomHandler::fromObject(ret));
				break;
			}
			case 0xa4:
			{
				//modulo
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v2);
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v1);

				ASObject* ret;
				// if both values are Integers or int Numbers the result is also an int Number
				if( (v1->is<Integer>() || v1->is<UInteger>() || (v1->is<Number>() && !v1->as<Number>()->isfloat)) &&
					(v2->is<Integer>() || v2->is<UInteger>() || (v2->is<Number>() && !v2->as<Number>()->isfloat)))
				{
					int64_t num1=v1->toInt64();
					int64_t num2=v2->toInt64();
					LOG_CALL("moduloI "  << num1 << '%' << num2);
					v1->decRef();
					v2->decRef();
					if (num2 == 0)
						ret=abstract_d(context->worker,Number::NaN);
					else
						ret = abstract_di(context->worker, num1%num2);
				}
				else
					ret=abstract_d(context->worker,modulo(v1, v2));
				RUNTIME_STACK_PUSH(context,asAtomHandler::fromObject(ret));
				break;
			}
			case 0xa5:
			{
				//lshift
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v1);
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v2);

				ASObject* ret=abstract_i(context->worker,lShift(v1, v2));
				RUNTIME_STACK_PUSH(context,asAtomHandler::fromObject(ret));
				break;
			}
			case 0xa6:
			{
				//rshift
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v1);
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v2);

				ASObject* ret=abstract_i(context->worker,rShift(v1, v2));
				RUNTIME_STACK_PUSH(context,asAtomHandler::fromObject(ret));
				break;
			}
			case 0xa7:
			{
				//urshift
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v1);
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v2);

				ASObject* ret=abstract_i(context->worker,urShift(v1, v2));
				RUNTIME_STACK_PUSH(context,asAtomHandler::fromObject(ret));
				break;
			}
			case 0xa8:
			{
				//bitand
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v1);
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v2);

				ASObject* ret=abstract_i(context->worker,bitAnd(v1, v2));
				RUNTIME_STACK_PUSH(context,asAtomHandler::fromObject(ret));
				break;
			}
			case 0xa9:
			{
				//bitor
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v1);
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v2);

				ASObject* ret=abstract_i(context->worker,bitOr(v1, v2));
				RUNTIME_STACK_PUSH(context,asAtomHandler::fromObject(ret));
				break;
			}
			case 0xaa:
			{
				//bitxor
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v1);
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v2);

				ASObject* ret=abstract_i(context->worker,bitXor(v1, v2));
				RUNTIME_STACK_PUSH(context,asAtomHandler::fromObject(ret));
				break;
			}
			case 0xab:
			{
				//equals
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v2);
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v1);

				ASObject* ret=abstract_b(context->sys,equals(v1, v2));
				RUNTIME_STACK_PUSH(context,asAtomHandler::fromObject(ret));
				break;
			}
			case 0xac:
			{
				//strictequals
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v2);
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v1);

				ASObject* ret=abstract_b(context->sys,strictEquals(v1, v2));
				RUNTIME_STACK_PUSH(context,asAtomHandler::fromObject(ret));
				break;
			}
			case 0xad:
			{
				//lessthan
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v2);
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v1);

				ASObject* ret=abstract_b(context->sys,lessThan(v1, v2));
				RUNTIME_STACK_PUSH(context,asAtomHandler::fromObject(ret));
				break;
			}
			case 0xae:
			{
				//lessequals
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v2);
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v1);

				ASObject* ret=abstract_b(context->sys,lessEquals(v1, v2));
				RUNTIME_STACK_PUSH(context,asAtomHandler::fromObject(ret));
				break;
			}
			case 0xaf:
			{
				//greaterthan
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v2);
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v1);

				ASObject* ret=abstract_b(context->sys,greaterThan(v1, v2));
				RUNTIME_STACK_PUSH(context,asAtomHandler::fromObject(ret));
				break;
			}
			case 0xb0:
			{
				//greaterequals
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v2);
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v1);

				ASObject* ret=abstract_b(context->sys,greaterEquals(v1, v2));
				RUNTIME_STACK_PUSH(context,asAtomHandler::fromObject(ret));
				break;
			}
			case 0xb1:
			{
				//instanceof
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,type);
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,value);
				bool ret=instanceOf(value, type);
				RUNTIME_STACK_PUSH(context,asAtomHandler::fromBool(ret));
				break;
			}
			case 0xb2:
			{
				//istype
				uint32_t t=data->uints[0];
				instructionPointer+=4;
				multiname* name=context->mi->context->getMultiname(t,NULL);

				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v1);

				ASObject* ret=abstract_b(context->sys,isType(context->mi->context, v1, name));
				RUNTIME_STACK_PUSH(context,asAtomHandler::fromObject(ret));
				break;
			}
			case 0xb3:
			{
				//istypelate
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v1);
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v2);

				ASObject* ret=abstract_b(context->sys,isTypelate(v1, v2));
				RUNTIME_STACK_PUSH(context,asAtomHandler::fromObject(ret));
				break;
			}
			case 0xb4:
			{
				//in
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v1);
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v2);

				ASObject* ret=abstract_b(context->sys,in(v1, v2));
				RUNTIME_STACK_PUSH(context,asAtomHandler::fromObject(ret));
				break;
			}
			case 0xc0:
			{
				//increment_i
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,val);
				ASObject* ret=abstract_i(context->worker,increment_i(val));
				RUNTIME_STACK_PUSH(context,asAtomHandler::fromObject(ret));
				break;
			}
			case 0xc1:
			{
				//decrement_i
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,val);
				ASObject* ret=abstract_i(context->worker,decrement_i(val));
				RUNTIME_STACK_PUSH(context,asAtomHandler::fromObject(ret));
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
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,val);
				ASObject* ret=abstract_i(context->worker,negate_i(val));
				RUNTIME_STACK_PUSH(context,asAtomHandler::fromObject(ret));
				break;
			}
			case 0xc5:
			{
				//add_i
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v2);
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v1);

				ASObject* ret=abstract_i(context->worker,add_i(v2, v1));
				RUNTIME_STACK_PUSH(context,asAtomHandler::fromObject(ret));
				break;
			}
			case 0xc6:
			{
				//subtract_i
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v2);
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v1);

				ASObject* ret=abstract_i(context->worker,subtract_i(v2, v1));
				RUNTIME_STACK_PUSH(context,asAtomHandler::fromObject(ret));
				break;
			}
			case 0xc7:
			{
				//multiply_i
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v2);
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v1);

				ASObject* ret=abstract_i(context->worker,multiply_i(v2, v1));
				RUNTIME_STACK_PUSH(context,asAtomHandler::fromObject(ret));
				break;
			}
			case 0xd0:
			case 0xd1:
			case 0xd2:
			case 0xd3:
			{
				//getlocal_n
				int i=opcode&3;
				if (asAtomHandler::isInvalid(context->locals[i]))
				{
					LOG_CALL( "getLocal " << i << " not set, pushing Undefined");
					RUNTIME_STACK_PUSH(context,asAtomHandler::fromObject(function->getSystemState()->getUndefinedRef()));
					break;
				}
				LOG_CALL( "getLocal " << i << ": " << asAtomHandler::toDebugString(context->locals[i]) );
				ASATOM_INCREF(context->locals[i]);
				RUNTIME_STACK_PUSH(context,context->locals[i]);
				break;
			}
			case 0xd4:
			case 0xd5:
			case 0xd6:
			case 0xd7:
			{
				//setlocal_n
				int i=opcode&3;
				LOG_CALL( "setLocal " << i );
				RUNTIME_STACK_POP_CREATE(context,obj)
				if ((int)i != context->argarrayposition || asAtomHandler::isArray(*obj))
				{
					ASATOM_DECREF(context->locals[i]);
					context->locals[i]=*obj;
				}
				break;
			}
			case 0xf2:
			{
				//bkptline
				LOG_CALL( "bkptline" );
				instructionPointer+=4;
				break;
			}
			case 0xf3:
			{
				//timestamp
				LOG_CALL( "timestamp" );
				instructionPointer+=4;
				break;
			}
			//lightspark custom opcodes
			case 0xfb:
			{
				//setslot_no_coerce
				uint32_t t=data->uints[0];
				instructionPointer+=4;

				RUNTIME_STACK_POP_CREATE(context,value);
				RUNTIME_STACK_POP_CREATE_ASOBJECT(context,obj);

				LOG_CALL("setSlotNoCoerce " << t);
				obj->setSlotNoCoerce(t-1,*value);
				obj->decRef();
				break;
			}
			case 0xfc:
			{
				//coerceearly
				const Type* type = data->types[0];
				LOG_CALL("coerceEarly " << type);

				RUNTIME_STACK_POP_CREATE(context,o);
				asAtom v = *o;
				if (type->coerce(function->getInstanceWorker(),*o))
					ASATOM_DECREF(v);
				RUNTIME_STACK_PUSH(context,*o);

				instructionPointer+=8;
				break;
			}
			case 0xfd:
			{
				//getscopeatindex
				//This opcode is similar to getscopeobject, but it allows access to any
				//index of the scope stack
				uint32_t t=data->uints[0];
				LOG_CALL( "getScopeAtIndex " << t);
				asAtom obj=asAtomHandler::invalidAtom;
				uint32_t parentsize = context->parent_scope_stack ? context->parent_scope_stack->scope.size() : 0;
				if (context->parent_scope_stack && t<parentsize)
					obj = context->parent_scope_stack->scope[t].object;
				else
				{
					assert_and_throw(t-parentsize <context->curr_scope_stack);
					obj=context->scope_stack[t-parentsize];
				}
				ASATOM_INCREF(obj);
				RUNTIME_STACK_PUSH(context,obj);
				instructionPointer+=4;
				break;
			}
			case 0xfe:
			{
				//getlexonce
				//This opcode execute a lookup on the application domain
				//and rewrites itself to a pushearly
				const multiname* name=data->names[0];
				LOG_CALL( "getLexOnce " << *name);
				ASObject* target;
				asAtom obj=asAtomHandler::invalidAtom;
				ABCVm::getCurrentApplicationDomain(context)->getVariableAndTargetByMultiname(obj,*name,target,context->worker);
				//The object must exists, since it was found during optimization
				assert_and_throw(asAtomHandler::isValid(obj));
				char* rewriteableCode = &(mi->body->code[0]);
				OpcodeData* rewritableData=reinterpret_cast<OpcodeData*>(rewriteableCode+instructionPointer);
				//Rewrite this to a pushearly
				rewriteableCode[instructionPointer-1]=0xff;
				rewritableData->objs[0]=asAtomHandler::toObject(obj,context->worker);
				//Also push the object right away
				ASATOM_INCREF(obj);
				RUNTIME_STACK_PUSH(context,obj);
				//Move to the next instruction
				instructionPointer+=8;
				break;
			}
			case 0xff:
			{
				//pushearly
				ASObject* o=data->objs[0];
				instructionPointer+=8;
				LOG_CALL( "pushEarly " << o);
				o->incRef();
				RUNTIME_STACK_PUSH(context,asAtomHandler::fromObject(o));
				break;
			}
			default:
				LOG(LOG_ERROR,"Not interpreted instruction @" << instructionPointer);
				LOG(LOG_ERROR,"dump " << hex << (unsigned int)opcode << dec);
				throw ParseException("Not implemented instruction in fast interpreter");
		}
		PROF_ACCOUNT_TIME(mi->profTime[instructionPointer],profilingCheckpoint(startTime));
	}

#undef PROF_ACCOUNT_TIME 
#undef PROF_IGNORE_TIME
	//We managed to execute all the function
	RUNTIME_STACK_POP_CREATE_ASOBJECT(context,returnvalue);
	return returnvalue;
}
