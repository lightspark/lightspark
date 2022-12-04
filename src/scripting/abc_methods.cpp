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
#include "scripting/flash/system/flashsystem.h"
#include "parsing/streams.h"
#include <string>
#include <sstream>

using namespace std;
using namespace lightspark;

void ABCVm::abc_bkpt(call_context* context)
{
	LOG_CALL( "bkpt" );
	++(context->exec_pos);
}
void ABCVm::abc_nop(call_context* context)
{
	++(context->exec_pos);
}
void ABCVm::abc_throw(call_context* context)
{
	_throw(context);
}
void ABCVm::abc_getSuper(call_context* context)
{
	uint32_t t = context->exec_pos->arg3_uint;
	getSuper(context,t);
	++(context->exec_pos);
}
void ABCVm::abc_setSuper(call_context* context)
{
	uint32_t t = context->exec_pos->arg3_uint;
	setSuper(context,t);
	++(context->exec_pos);
}
void ABCVm::abc_dxns(call_context* context)
{
	uint32_t t = context->exec_pos->arg3_uint;
	dxns(context,t);
	++(context->exec_pos);
}
void ABCVm::abc_dxnslate(call_context* context)
{
	RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v);
	dxnslate(context, v);
	++(context->exec_pos);
}
void ABCVm::abc_kill(call_context* context)
{
	uint32_t t = context->exec_pos->arg3_uint;
	LOG_CALL( "kill " << t);
	ASATOM_DECREF(context->locals[t]);
	context->locals[t]=asAtomHandler::undefinedAtom;
	++(context->exec_pos);
}
void ABCVm::abc_label(call_context* context)
{
	LOG_CALL("label");
	++(context->exec_pos);
}
void ABCVm::abc_ifnlt(call_context* context)
{
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POP_CREATE(context,v2);
	bool cond=!(asAtomHandler::isLess(*v2,context->worker,*v1) == TTRUE);
	ASATOM_DECREF_POINTER(v2);
	ASATOM_DECREF_POINTER(v1);
	LOG_CALL("ifNLT (" << ((cond)?"taken)":"not taken)"));

	if(cond)
		context->exec_pos += (*context->exec_pos).arg3_int;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifnle(call_context* context)
{
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POP_CREATE(context,v2);
	bool cond=!(asAtomHandler::isLess(*v1,context->worker,*v2) == TFALSE);
	ASATOM_DECREF_POINTER(v2);
	ASATOM_DECREF_POINTER(v1);
	LOG_CALL("ifNLE (" << ((cond)?"taken)":"not taken)"));

	if(cond)
		context->exec_pos += (*context->exec_pos).arg3_int;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifngt(call_context* context)
{
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POP_CREATE(context,v2);
	bool cond=!(asAtomHandler::isLess(*v1,context->worker,*v2) == TTRUE);
	ASATOM_DECREF_POINTER(v2);
	ASATOM_DECREF_POINTER(v1);
	LOG_CALL("ifNGT (" << ((cond)?"taken)":"not taken)"));

	if(cond)
		context->exec_pos += (*context->exec_pos).arg3_int;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifnge(call_context* context)
{
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POP_CREATE(context,v2);
	bool cond=!(asAtomHandler::isLess(*v2,context->worker,*v1) == TFALSE);
	ASATOM_DECREF_POINTER(v2);
	ASATOM_DECREF_POINTER(v1);
	LOG_CALL("ifNGE (" << ((cond)?"taken)":"not taken)"));

	if(cond)
		context->exec_pos += (*context->exec_pos).arg3_int;
	else
		++(context->exec_pos);
}
void ABCVm::abc_jump(call_context* context)
{
	LOG_CALL("jump:"<<(*context->exec_pos).arg3_int);
	context->exec_pos += (*context->exec_pos).arg3_int;
}
void ABCVm::abc_iftrue(call_context* context)
{
	RUNTIME_STACK_POP_CREATE(context,v1);
	bool cond=asAtomHandler::Boolean_concrete(*v1);
	LOG_CALL("ifTrue (" << ((cond)?"taken)":"not taken)")<<" "<<(*context->exec_pos).arg3_int);
	ASATOM_DECREF_POINTER(v1);
	if(cond)
		context->exec_pos += (*context->exec_pos).arg3_int;
	else
		++(context->exec_pos);
}
void ABCVm::abc_iffalse(call_context* context)
{
	RUNTIME_STACK_POP_CREATE(context,v1);
	bool cond=!asAtomHandler::Boolean_concrete(*v1);
	LOG_CALL("ifFalse (" << ((cond)?"taken)":"not taken)"));
	ASATOM_DECREF_POINTER(v1);
	if(cond)
		context->exec_pos += (*context->exec_pos).arg3_int;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifeq(call_context* context)
{
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POP_CREATE(context,v2);
	bool cond=(asAtomHandler::isEqual(*v1,context->worker,*v2));
	LOG_CALL("ifEq (" << ((cond)?"taken)":"not taken)"));
	ASATOM_DECREF_POINTER(v2);
	ASATOM_DECREF_POINTER(v1);
	if(cond)
		context->exec_pos += (*context->exec_pos).arg3_int;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifne(call_context* context)
{
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POP_CREATE(context,v2);
	bool cond=!(asAtomHandler::isEqual(*v1,context->worker,*v2));
	LOG_CALL("ifNE (" << ((cond)?"taken)":"not taken)"));
	ASATOM_DECREF_POINTER(v2);
	ASATOM_DECREF_POINTER(v1);
	if(cond)
		context->exec_pos += (*context->exec_pos).arg3_int;
	else
		++(context->exec_pos);
}
void ABCVm::abc_iflt(call_context* context)
{
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POP_CREATE(context,v2);
	bool cond=asAtomHandler::isLess(*v2,context->worker,*v1) == TTRUE;
	ASATOM_DECREF_POINTER(v2);
	ASATOM_DECREF_POINTER(v1);
	LOG_CALL("ifLT (" << ((cond)?"taken)":"not taken)"));

	if(cond)
		context->exec_pos += (*context->exec_pos).arg3_int;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifle(call_context* context)
{
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POP_CREATE(context,v2);
	bool cond=asAtomHandler::isLess(*v1,context->worker,*v2) == TFALSE;
	ASATOM_DECREF_POINTER(v2);
	ASATOM_DECREF_POINTER(v1);
	LOG_CALL("ifLE (" << ((cond)?"taken)":"not taken)"));

	if(cond)
		context->exec_pos += (*context->exec_pos).arg3_int;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifgt(call_context* context)
{
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POP_CREATE(context,v2);
	bool cond=asAtomHandler::isLess(*v1,context->worker,*v2) == TTRUE;
	ASATOM_DECREF_POINTER(v2);
	ASATOM_DECREF_POINTER(v1);
	LOG_CALL("ifGT (" << ((cond)?"taken)":"not taken)"));

	if(cond)
		context->exec_pos += (*context->exec_pos).arg3_int;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifge(call_context* context)
{
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POP_CREATE(context,v2);
	bool cond=asAtomHandler::isLess(*v2,context->worker,*v1) == TFALSE;
	ASATOM_DECREF_POINTER(v2);
	ASATOM_DECREF_POINTER(v1);
	LOG_CALL("ifGE (" << ((cond)?"taken)":"not taken)"));

	if(cond)
		context->exec_pos += (*context->exec_pos).arg3_int;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifstricteq(call_context* context)
{
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POP_CREATE(context,v2);

	bool cond=asAtomHandler::isEqualStrict(*v1,context->worker,*v2);
	LOG_CALL("ifStrictEq "<<cond);
	ASATOM_DECREF_POINTER(v1);
	ASATOM_DECREF_POINTER(v2);
	if(cond)
		context->exec_pos += (*context->exec_pos).arg3_int;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifstrictne(call_context* context)
{
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POP_CREATE(context,v2);

	bool cond=!asAtomHandler::isEqualStrict(*v1,context->worker,*v2);
	LOG_CALL("ifStrictNE "<<cond <<" "<<asAtomHandler::toDebugString(*v1)<<" "<<asAtomHandler::toDebugString(*v2));
	ASATOM_DECREF_POINTER(v1);
	ASATOM_DECREF_POINTER(v2);
	if(cond)
		context->exec_pos += (*context->exec_pos).arg3_int;
	else
		++(context->exec_pos);
}
void ABCVm::abc_lookupswitch(call_context* context)
{
	preloadedcodedata* here=context->exec_pos; //Base for the jumps is the instruction itself for the switch
	int32_t t = (++(context->exec_pos))->arg3_int;
	preloadedcodedata* defaultdest=here+t;
	uint32_t count = (++(context->exec_pos))->arg3_uint;

	RUNTIME_STACK_POP_CREATE(context,index_obj);
	LOG_CALL("lookupswitch " << t <<" "<<asAtomHandler::toDebugString(*index_obj));
	assert_and_throw(asAtomHandler::isNumeric(*index_obj));
	unsigned int index=asAtomHandler::toUInt(*index_obj);
	ASATOM_DECREF_POINTER(index_obj);

	preloadedcodedata* dest=defaultdest;
	if(index<=count)
		dest=here+(context->exec_pos+index+1)->arg3_int;

	context->exec_pos = dest;
}
void ABCVm::abc_pushwith(call_context* context)
{
	pushWith(context);
	++(context->exec_pos);
}
void ABCVm::abc_popscope(call_context* context)
{
	popScope(context);
	++(context->exec_pos);
}
void ABCVm::abc_nextname(call_context* context)
{
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	LOG_CALL("nextName");
	if(!asAtomHandler::isUInteger(*v1) && !asAtomHandler::isInteger(*v1))
		throw UnsupportedException("Type mismatch in nextName");

	asAtom ret=asAtomHandler::invalidAtom;
	asAtomHandler::toObject(*pval,context->worker)->nextName(ret,asAtomHandler::toUInt(*v1));
	ASATOM_DECREF_POINTER(pval);
	ASATOM_DECREF_POINTER(v1);
	*pval = ret;
	++(context->exec_pos);
}
void ABCVm::abc_hasnext(call_context* context)
{
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	LOG_CALL("hasNext " << asAtomHandler::toDebugString(*v1) << ' ' << asAtomHandler::toDebugString(*pval));

	uint32_t curIndex=asAtomHandler::toUInt(*pval);

	uint32_t newIndex=asAtomHandler::toObject(*v1,context->worker)->nextNameIndex(curIndex);
	ASATOM_DECREF_POINTER(pval);
	ASATOM_DECREF_POINTER(v1);
	asAtomHandler::setInt(*pval,context->worker,newIndex);
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
	if(!asAtomHandler::isUInteger(*v1) && !asAtomHandler::isInteger(*v1))
		throw UnsupportedException("Type mismatch in nextValue");

	asAtom ret=asAtomHandler::invalidAtom;
	asAtomHandler::toObject(*pval,context->worker)->nextValue(ret,asAtomHandler::toUInt(*v1));
	ASATOM_DECREF_POINTER(pval);
	ASATOM_DECREF_POINTER(v1);
	*pval=ret;
	++(context->exec_pos);
}
void ABCVm::abc_pushbyte(call_context* context)
{
	int32_t t = context->exec_pos->arg3_int;
	LOG_CALL("pushbyte "<<(int)t);
	RUNTIME_STACK_PUSH(context,asAtomHandler::fromInt(t));
	++(context->exec_pos);
}
void ABCVm::abc_pushshort(call_context* context)
{
	// specs say pushshort is a u30, but it's really a u32
	// see https://bugs.adobe.com/jira/browse/ASC-4181
	int32_t t = context->exec_pos->arg3_int;
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
	RUNTIME_STACK_PUSH(context,context->sys->nanAtom);
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
	uint32_t t = context->exec_pos->arg3_uint;
	LOG_CALL( "pushString " << context->sys->getStringFromUniqueId(context->mi->context->getString(t)) );
	RUNTIME_STACK_PUSH(context,asAtomHandler::fromStringID(context->mi->context->getString(t)));
	++(context->exec_pos);
}
void ABCVm::abc_pushint(call_context* context)
{
	uint32_t t = context->exec_pos->arg3_uint;
	s32 val=context->mi->context->constant_pool.integer[t];
	LOG_CALL( "pushInt " << val);

	RUNTIME_STACK_PUSH(context,asAtomHandler::fromInt(val));
	++(context->exec_pos);
}
void ABCVm::abc_pushuint(call_context* context)
{
	uint32_t t = context->exec_pos->arg3_uint;
	u32 val=context->mi->context->constant_pool.uinteger[t];
	LOG_CALL( "pushUInt " << val);

	RUNTIME_STACK_PUSH(context,asAtomHandler::fromUInt(val));
	++(context->exec_pos);
}
void ABCVm::abc_pushdouble(call_context* context)
{
	uint32_t t = context->exec_pos->arg3_uint;
	asAtom* a = context->mi->context->getConstantAtom(OPERANDTYPES::OP_DOUBLE,t);
	LOG_CALL( "pushDouble " << asAtomHandler::toDebugString(*a));

	RUNTIME_STACK_PUSH(context,*a);
	++(context->exec_pos);
}
void ABCVm::abc_pushScope(call_context* context)
{
	pushScope(context);
	++(context->exec_pos);
}
void ABCVm::abc_pushnamespace(call_context* context)
{
	uint32_t t = context->exec_pos->arg3_uint;
	RUNTIME_STACK_PUSH(context,asAtomHandler::fromObjectNoPrimitive(pushNamespace(context, t) ));
	++(context->exec_pos);
}
void ABCVm::abc_hasnext2(call_context* context)
{
	uint32_t t = context->exec_pos->arg1_uint;
	uint32_t t2 = context->exec_pos->arg2_uint;

	bool ret=hasNext2(context,t,t2);
	RUNTIME_STACK_PUSH(context,asAtomHandler::fromBool(ret));
	++(context->exec_pos);
}
//Alchemy opcodes
void ABCVm::abc_li8(call_context* context)
{
	LOG_CALL( "li8");
	ApplicationDomain::loadIntN<uint8_t>(context->mi->context->root->applicationDomain.getPtr(), context);
	++(context->exec_pos);
}
void ABCVm::abc_li16(call_context* context)
{
	LOG_CALL( "li16");
	ApplicationDomain::loadIntN<uint16_t>(context->mi->context->root->applicationDomain.getPtr(), context);
	++(context->exec_pos);
}
void ABCVm::abc_li32(call_context* context)
{
	LOG_CALL( "li32");
	ApplicationDomain::loadIntN<int32_t>(context->mi->context->root->applicationDomain.getPtr(), context);
	++(context->exec_pos);
}
void ABCVm::abc_lf32(call_context* context)
{
	LOG_CALL( "lf32");
	ApplicationDomain::loadFloat(context->mi->context->root->applicationDomain.getPtr(), context);
	++(context->exec_pos);
}
void ABCVm::abc_lf64(call_context* context)
{
	LOG_CALL( "lf64");
	ApplicationDomain::loadDouble(context->mi->context->root->applicationDomain.getPtr(), context);
	++(context->exec_pos);
}
void ABCVm::abc_si8(call_context* context)
{
	LOG_CALL( "si8");
	ApplicationDomain::storeIntN<uint8_t>(context->mi->context->root->applicationDomain.getPtr(), context);
	++(context->exec_pos);
}
void ABCVm::abc_si16(call_context* context)
{
	LOG_CALL( "si16");
	ApplicationDomain::storeIntN<uint16_t>(context->mi->context->root->applicationDomain.getPtr(), context);
	++(context->exec_pos);
}
void ABCVm::abc_si32(call_context* context)
{
	LOG_CALL( "si32");
	ApplicationDomain::storeIntN<uint32_t>(context->mi->context->root->applicationDomain.getPtr(), context);
	++(context->exec_pos);
}
void ABCVm::abc_sf32(call_context* context)
{
	LOG_CALL( "sf32");
	ApplicationDomain::storeFloat(context->mi->context->root->applicationDomain.getPtr(), context);
	++(context->exec_pos);
}
void ABCVm::abc_sf64(call_context* context)
{
	LOG_CALL( "sf64");
	ApplicationDomain::storeDouble(context->mi->context->root->applicationDomain.getPtr(), context);
	++(context->exec_pos);
}
void ABCVm::abc_newfunction(call_context* context)
{
	uint32_t t = context->exec_pos->arg1_uint;
	RUNTIME_STACK_PUSH(context,asAtomHandler::fromObjectNoPrimitive(newFunction(context,t)));
	++(context->exec_pos);
}
void ABCVm::abc_call(call_context* context)
{
	uint32_t t = context->exec_pos->arg1_uint;
	method_info* called_mi=nullptr;
	call(context,t,&called_mi);
	++(context->exec_pos);
}
void ABCVm::abc_construct(call_context* context)
{
	uint32_t t = context->exec_pos->arg3_uint;
	construct(context,t);
	++(context->exec_pos);
}
void ABCVm::abc_callMethod(call_context* context)
{
	uint32_t t = context->exec_pos->arg1_uint;
	uint32_t t2 = context->exec_pos->arg2_uint;
	callMethod(context,t,t2);
	++(context->exec_pos);
}
void ABCVm::abc_callstatic(call_context* context)
{
	uint32_t t = context->exec_pos->arg1_uint;
	uint32_t t2 = context->exec_pos->arg2_uint;
	callStatic(context,t,t2,nullptr,true);
	++(context->exec_pos);
}
void ABCVm::abc_callsuper(call_context* context)
{
	uint32_t t = context->exec_pos->arg1_uint;
	uint32_t t2 = context->exec_pos->arg2_uint;
	callSuper(context,t,t2,nullptr,true);
	++(context->exec_pos);
}
void ABCVm::abc_callproperty(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t t = (++(context->exec_pos))->arg1_uint;
	uint32_t t2 = context->exec_pos->arg2_uint;
	callPropIntern(context,t,t2,true,false,instrptr);
	++(context->exec_pos);
}
void ABCVm::abc_returnvoid(call_context* context)
{
	LOG_CALL("returnVoid");
	context->locals[context->mi->body->getReturnValuePos()]= asAtomHandler::undefinedAtom;
	++(context->exec_pos);
}
void ABCVm::abc_returnvalue(call_context* context)
{
	RUNTIME_STACK_POP(context,context->locals[context->mi->body->getReturnValuePos()]);
	LOG_CALL("returnValue " << asAtomHandler::toDebugString(context->locals[context->mi->body->getReturnValuePos()]));
	++(context->exec_pos);
}
void ABCVm::abc_constructsuper(call_context* context)
{
	uint32_t t = context->exec_pos->arg3_uint;
	constructSuper(context,t);
	++(context->exec_pos);
}
void ABCVm::abc_constructprop(call_context* context)
{
	uint32_t t = context->exec_pos->arg1_uint;
	uint32_t t2 = context->exec_pos->arg2_uint;
	constructProp(context,t,t2);
	++(context->exec_pos);
}
void ABCVm::abc_callproplex(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t t = (++(context->exec_pos))->arg1_uint;
	uint32_t t2 = context->exec_pos->arg2_uint;
	callPropIntern(context,t,t2,true,true,instrptr);
	++(context->exec_pos);
}
void ABCVm::abc_callsupervoid(call_context* context)
{
	uint32_t t = context->exec_pos->arg1_uint;
	uint32_t t2 = context->exec_pos->arg2_uint;
	callSuper(context,t,t2,nullptr,false);
	++(context->exec_pos);
}
void ABCVm::abc_callpropvoid(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t t = (++(context->exec_pos))->arg1_uint;
	uint32_t t2 = context->exec_pos->arg2_uint;
	callPropIntern(context,t,t2,false,false,instrptr);
	++(context->exec_pos);
}
void ABCVm::abc_sxi1(call_context* context)
{
	LOG_CALL( "sxi1");
	RUNTIME_STACK_POINTER_CREATE(context,arg1);
	int32_t ret=asAtomHandler::toUInt(*arg1)&0x1 ? -1 : 0;
	ASATOM_DECREF_POINTER(arg1);
	asAtomHandler::setInt(*arg1,context->worker,ret);
	++(context->exec_pos);
}
void ABCVm::abc_sxi8(call_context* context)
{
	LOG_CALL( "sxi8");
	RUNTIME_STACK_POINTER_CREATE(context,arg1);
	int32_t ret=(int8_t)asAtomHandler::toUInt(*arg1);
	ASATOM_DECREF_POINTER(arg1);
	asAtomHandler::setInt(*arg1,context->worker,ret);
	++(context->exec_pos);
}
void ABCVm::abc_sxi16(call_context* context)
{
	LOG_CALL( "sxi16");
	RUNTIME_STACK_POINTER_CREATE(context,arg1);
	int32_t ret=(int16_t)asAtomHandler::toUInt(*arg1);
	ASATOM_DECREF_POINTER(arg1);
	asAtomHandler::setInt(*arg1,context->worker,ret);
	++(context->exec_pos);
}
void ABCVm::abc_constructgenerictype(call_context* context)
{
	uint32_t t = context->exec_pos->arg3_uint;
	constructGenericType(context, t);
	++(context->exec_pos);
}
void ABCVm::abc_newobject(call_context* context)
{
	uint32_t t = context->exec_pos->arg3_uint;
	newObject(context,t);
	++(context->exec_pos);
}
void ABCVm::abc_newarray(call_context* context)
{
	uint32_t t = context->exec_pos->arg3_uint;
	newArray(context,t);
	++(context->exec_pos);
}
void ABCVm::abc_newactivation(call_context* context)
{
	RUNTIME_STACK_PUSH(context,asAtomHandler::fromObjectNoPrimitive(newActivation(context, context->mi)));
	++(context->exec_pos);
}
void ABCVm::abc_newclass(call_context* context)
{
	uint32_t t = context->exec_pos->arg3_uint;
	newClass(context,t);
	++(context->exec_pos);
}
void ABCVm::abc_getdescendants(call_context* context)
{
	uint32_t t = context->exec_pos->arg3_uint;
	getDescendants(context, t);
	++(context->exec_pos);
}
void ABCVm::abc_newcatch(call_context* context)
{
	uint32_t t = context->exec_pos->arg3_uint;
	RUNTIME_STACK_PUSH(context,asAtomHandler::fromObjectNoPrimitive(newCatch(context,t)));
	++(context->exec_pos);
}
void ABCVm::abc_findpropstrict(call_context* context)
{
	asAtom o=asAtomHandler::invalidAtom;
	findPropStrictCache(o,context);
	RUNTIME_STACK_PUSH(context,o);
	++(context->exec_pos);
}
void ABCVm::abc_findproperty(call_context* context)
{
	uint32_t t = context->exec_pos->local3.pos;
	multiname* name=context->mi->context->getMultiname(t,context);
	RUNTIME_STACK_PUSH(context,asAtomHandler::fromObject(findProperty(context,name)));
	name->resetNameIfObject();
	++(context->exec_pos);
}
void ABCVm::abc_finddef(call_context* context)
{
	uint32_t t = context->exec_pos->arg3_uint;
	multiname* name=context->mi->context->getMultiname(t,context);
	ASObject* target = NULL;
	asAtom o=asAtomHandler::invalidAtom;
	getCurrentApplicationDomain(context)->getVariableAndTargetByMultiname(o,*name, target,context->worker);
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
	preloadedcodedata* instrptr = context->exec_pos;
	if ((instrptr->local3.flags&ABC_OP_CACHED) == ABC_OP_CACHED)
	{
		RUNTIME_STACK_PUSH(context,asAtomHandler::fromObject(instrptr->cacheobj1));
		instrptr->cacheobj1->incRef();
		LOG_CALL( "getLex from cache: " <<  instrptr->cacheobj1->toDebugString());
	}
	else if (getLex_multiname(context,instrptr->cachedmultiname2,UINT32_MAX))
	{
		// put object in cache
		instrptr->local3.flags = ABC_OP_CACHED;
		RUNTIME_STACK_PEEK_CREATE(context,v);

		instrptr->cacheobj1 = asAtomHandler::getObject(*v);
		instrptr->cacheobj2 = asAtomHandler::getClosure(*v);
	}
	++(context->exec_pos);
}
void ABCVm::abc_setproperty(call_context* context)
{
	uint32_t t = context->exec_pos->local3.pos;
	RUNTIME_STACK_POP_CREATE(context,value);

	multiname* name=context->mi->context->getMultiname(t,context);

	RUNTIME_STACK_POP_CREATE(context,obj)

	LOG_CALL("setProperty " << *name << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));

	if(asAtomHandler::isNull(*obj))
	{
		LOG(LOG_ERROR,"calling setProperty on null:" << *name << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));
		ASATOM_DECREF_POINTER(obj);
		ASATOM_DECREF_POINTER(value);
		name->resetNameIfObject();
		createError<TypeError>(context->worker,kConvertNullToObjectError);
		return;
	}
	if (asAtomHandler::isUndefined(*obj))
	{
		LOG(LOG_ERROR,"calling setProperty on undefined:" << *name << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));
		ASATOM_DECREF_POINTER(obj);
		ASATOM_DECREF_POINTER(value);
		name->resetNameIfObject();
		createError<TypeError>(context->worker,kConvertUndefinedToObjectError);
		return;
	}
	//Do not allow to set contant traits
	ASObject* o = asAtomHandler::toObject(*obj,context->worker);
	bool alreadyset=false;
	o->setVariableByMultiname(*name,*value,ASObject::CONST_NOT_ALLOWED,&alreadyset,context->worker);
	if (alreadyset)
		ASATOM_DECREF_POINTER(value);
	o->decRef();
	name->resetNameIfObject();
	++(context->exec_pos);
}
void ABCVm::abc_getlocal(call_context* context)
{
	uint32_t i = ((context->exec_pos)++)->arg3_uint;
	LOG_CALL( "getLocal n " << i << ": " << asAtomHandler::toDebugString(context->locals[i]) );
	ASATOM_INCREF(context->locals[i]);
	RUNTIME_STACK_PUSH(context,context->locals[i]);
}
void ABCVm::abc_setlocal(call_context* context)
{
	uint32_t i = ((context->exec_pos)++)->arg3_uint;
	RUNTIME_STACK_POP_CREATE(context,obj)

	LOG_CALL( "setLocal n " << i << ": " << asAtomHandler::toDebugString(*obj) );
	assert(i <= context->mi->body->getReturnValuePos()+context->mi->body->localresultcount);
	if ((int)i != context->argarrayposition || asAtomHandler::isArray(*obj))
	{
		ASATOM_DECREF(context->locals[i]);
		context->locals[i]=*obj;
	}
}
void ABCVm::abc_getglobalscope(call_context* context)
{
	asAtom ret = asAtomHandler::fromObject(getGlobalScope(context));
	ASATOM_INCREF(ret);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_getscopeobject(call_context* context)
{
	uint32_t t = context->exec_pos->arg2_uint;
	assert_and_throw(context->curr_scope_stack > (uint32_t)t);
	asAtom ret=context->scope_stack[t];
	ASATOM_INCREF(ret);
	LOG_CALL( "getScopeObject: " << asAtomHandler::toDebugString(ret)<<" "<<t);

	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_getProperty(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t t = (++(context->exec_pos))->local3.pos;
	ASObject* obj= nullptr;
	if ((context->exec_pos->local3.flags&ABC_OP_CACHED) == ABC_OP_CACHED)
	{
		RUNTIME_STACK_POP_ASOBJECT(context,obj);
		if (instrptr->cacheobj1 == obj->getClass())
		{
			asAtom prop=asAtomHandler::invalidAtom;
			//Call the getter
			LOG_CALL("Calling the getter for " << *context->mi->context->getMultiname(t,context) << " on " << obj->toDebugString());
			assert(instrptr->cacheobj2->type == T_FUNCTION);
			IFunction* f = instrptr->cacheobj2->as<IFunction>();
			f->callGetter(prop,instrptr->cacheobj3 ? instrptr->cacheobj3 : obj,context->worker);
			LOG_CALL("End of getter"<< ' ' << f->toDebugString()<<" result:"<<asAtomHandler::toDebugString(prop));
			if(asAtomHandler::isInvalid(prop))
			{
				multiname* name=context->mi->context->getMultiname(t,context);
				if (checkPropertyException(obj,name,prop))
				{
					obj->decRef();
					return;
				}
			}
			obj->decRef();
			RUNTIME_STACK_PUSH(context,prop);
			++(context->exec_pos);
			return;
		}
	}
	multiname* name=context->mi->context->getMultiname(t,context);

	if (obj == nullptr)
	{
		RUNTIME_STACK_POP_ASOBJECT(context,obj);
	}

	LOG_CALL("getProperty " << *name << ' ' << obj->toDebugString() << ' '<<obj->isInitialized());

	asAtom prop=asAtomHandler::invalidAtom;
	bool isgetter = obj->getVariableByMultiname(prop,*name,(name->isStatic && obj->getClass() && obj->getClass()->isSealed)? GET_VARIABLE_OPTION::DONT_CALL_GETTER:GET_VARIABLE_OPTION::NONE,context->worker) & GET_VARIABLE_RESULT::GETVAR_ISGETTER;
	if (isgetter)
	{
		//Call the getter
		LOG_CALL("Calling the getter for " << *name << " on " << obj->toDebugString());
		assert(asAtomHandler::isFunction(prop));
		IFunction* f = asAtomHandler::as<IFunction>(prop);
		ASObject* closure = asAtomHandler::getClosure(prop);
		prop = asAtom();
		f->callGetter(prop,closure ? closure : obj,context->worker);
		LOG_CALL("End of getter"<< ' ' << f->toDebugString()<<" result:"<<asAtomHandler::toDebugString(prop));
		if(asAtomHandler::isValid(prop))
		{
			// cache getter if multiname is static and it is a getter of a sealed class
			context->exec_pos->local3.flags = ABC_OP_CACHED;
			instrptr->cacheobj1 = obj->getClass();
			instrptr->cacheobj2 = f;
			if (f->clonedFrom)
				f->incRef();
			instrptr->cacheobj3 = closure;
		}
	}
	if(asAtomHandler::isInvalid(prop))
	{
		if (checkPropertyException(obj,name,prop))
		{
			obj->decRef();
			return;
		}
	}
	obj->decRef();
	name->resetNameIfObject();

	RUNTIME_STACK_PUSH(context,prop);
	++(context->exec_pos);
}
void ABCVm::abc_initproperty(call_context* context)
{
	uint32_t t = context->exec_pos->local3.pos;
	RUNTIME_STACK_POP_CREATE(context,value);
	multiname* name=context->mi->context->getMultiname(t,context);
	RUNTIME_STACK_POP_CREATE(context,obj);
	LOG_CALL("initProperty "<<*name<<" on "<< asAtomHandler::toDebugString(*obj)<<" to "<<asAtomHandler::toDebugString(*value));
	bool alreadyset=false;
	asAtomHandler::toObject(*obj,context->worker)->setVariableByMultiname(*name,*value,ASObject::CONST_ALLOWED,&alreadyset,context->worker);
	if (alreadyset)
		ASATOM_DECREF_POINTER(value);
	ASATOM_DECREF_POINTER(obj);
	name->resetNameIfObject();
	++(context->exec_pos);
}
void ABCVm::abc_deleteproperty(call_context* context)
{
	uint32_t t = context->exec_pos->arg1_uint;
	multiname* name = context->mi->context->getMultiname(t,context);
	RUNTIME_STACK_POP_CREATE_ASOBJECT(context,obj);
	bool ret = deleteProperty(obj,name);
	name->resetNameIfObject();
	RUNTIME_STACK_PUSH(context,asAtomHandler::fromBool(ret));
	++(context->exec_pos);
}
void ABCVm::abc_getslot(call_context* context)
{
	uint32_t t = context->exec_pos->arg2_uint;
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
void ABCVm::abc_setslot(call_context* context)
{
	uint32_t t = context->exec_pos->arg3_uint;
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v2);

	LOG_CALL("setSlot " << t << " "<< v2->toDebugString() << " "<< asAtomHandler::toDebugString(*v1));
	if (!v2->setSlot(context->worker,t,*v1))
		ASATOM_DECREF_POINTER(v1);
	v2->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_getglobalSlot(call_context* context)
{
	uint32_t t = context->exec_pos->arg1_uint;

	Global* globalscope = getGlobalScope(context);
	asAtom ret=globalscope->getSlot(t);
	ASATOM_INCREF(ret);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_setglobalSlot(call_context* context)
{
	uint32_t t = context->exec_pos->arg1_uint;

	Global* globalscope = getGlobalScope(context);
	RUNTIME_STACK_POP_CREATE(context,o);
	globalscope->setSlot(context->worker,t-1,*o);
	++(context->exec_pos);
}
void ABCVm::abc_convert_s(call_context* context)
{
	//convert_s
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	LOG_CALL( "convert_s" );
	if(!asAtomHandler::isString(*pval))
	{
		tiny_string s = asAtomHandler::toString(*pval,context->worker);
		ASATOM_DECREF_POINTER(pval);
		asAtomHandler::replace(*pval,(ASObject*)abstract_s(context->worker,s));
	}
	++(context->exec_pos);
}
void ABCVm::abc_esc_xelem(call_context* context)
{
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	asAtomHandler::replace(*pval,esc_xelem(asAtomHandler::toObject(*pval,context->worker)));
	++(context->exec_pos);
}
void ABCVm::abc_esc_xattr(call_context* context)
{
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	asAtomHandler::replace(*pval,esc_xattr(asAtomHandler::toObject(*pval,context->worker)));
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
		asAtomHandler::setInt(*pval,context->worker,v);
	}
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
		asAtomHandler::setUInt(*pval,context->worker,v);
	}
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
		asAtomHandler::setNumber(*pval,context->worker,v);
	}
	++(context->exec_pos);
}
void ABCVm::abc_convert_b(call_context* context)
{
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	LOG_CALL("convert_b");
	asAtomHandler::convert_b(*pval,true);
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
		createError<TypeError>(context->worker,kConvertNullToObjectError);
	}
	if (asAtomHandler::isUndefined(*pval))
	{
		LOG(LOG_ERROR,"trying to call convert_o on undefined");
		createError<TypeError>(context->worker,kConvertUndefinedToObjectError);
	}
	++(context->exec_pos);
}
void ABCVm::abc_checkfilter(call_context* context)
{
	//checkfilter
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	checkfilter(asAtomHandler::toObject(*pval,context->worker));
	++(context->exec_pos);
}
void ABCVm::abc_coerce(call_context* context)
{
	multiname* mn = context->exec_pos->cachedmultiname2;
	LOG_CALL("coerce " << *mn);
	RUNTIME_STACK_POINTER_CREATE(context,o);
	const Type* type = mn->cachedType != nullptr ? mn->cachedType : Type::getTypeFromMultiname(mn, context->mi->context);
	if (type == nullptr)
	{
		LOG(LOG_ERROR,"coerce: type not found:"<< *mn);
		createError<TypeError>(context->worker,kClassNotFoundError,mn->qualifiedString(getSys()));
		return;
	}
	asAtom v= *o;
	if (type->coerce(context->worker,*o))
		ASATOM_DECREF(v);
	++(context->exec_pos);
}
void ABCVm::abc_coerce_a(call_context* context)
{
	coerce_a();
	++(context->exec_pos);
}
void ABCVm::abc_coerce_s(call_context* context)
{
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	LOG_CALL("coerce_s:"<<asAtomHandler::toDebugString(*pval));
	if (!asAtomHandler::isString(*pval))
	{
		asAtom v = *pval;
		if (Class<ASString>::getClass(context->sys)->coerce(context->worker,*pval))
			ASATOM_DECREF(v);
	}
	++(context->exec_pos);
}
void ABCVm::abc_astype(call_context* context)
{
	uint32_t t = context->exec_pos->arg1_uint;
	multiname* name=context->mi->context->getMultiname(t,nullptr);

	RUNTIME_STACK_POINTER_CREATE(context,pval);
	asAtomHandler::replace(*pval,asType(context->mi->context, asAtomHandler::toObject(*pval,context->worker), name));
	++(context->exec_pos);
}
void ABCVm::abc_astypelate(call_context* context)
{
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	asAtom a = asAtomHandler::asTypelate(*pval,*v1,context->worker);
	if (asAtomHandler::isNull(a))
		ASATOM_DECREF_POINTER(pval);
	*pval = a;
	++(context->exec_pos);
}
void ABCVm::abc_negate(call_context* context)
{
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	LOG_CALL("negate "<<asAtomHandler::toDebugString(*pval));
	if (asAtomHandler::isInteger(*pval) && asAtomHandler::toInt(*pval) != 0)
	{
		int32_t ret=-(asAtomHandler::toInt(*pval));
		ASATOM_DECREF_POINTER(pval);
		asAtomHandler::setInt(*pval,context->worker, ret);
	}
	else
	{
		number_t ret=-(asAtomHandler::toNumber(*pval));
		ASATOM_DECREF_POINTER(pval);
		asAtomHandler::setNumber(*pval,context->worker, ret);
	}
	++(context->exec_pos);
}
void ABCVm::abc_increment(call_context* context)
{
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	LOG_CALL("increment "<<asAtomHandler::toDebugString(*pval));
	asAtom oldval = *pval;
	asAtomHandler::increment(*pval,context->worker);
	ASATOM_DECREF(oldval);
	++(context->exec_pos);
}
void ABCVm::abc_inclocal(call_context* context)
{
	uint32_t t = context->exec_pos->arg1_uint;
	incLocal(context, t);
	++(context->exec_pos);
}
void ABCVm::abc_decrement(call_context* context)
{
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	asAtom oldval = *pval;
	asAtomHandler::decrement(*pval,context->worker);
	ASATOM_DECREF(oldval);
	++(context->exec_pos);
}
void ABCVm::abc_declocal(call_context* context)
{
	uint32_t t = context->exec_pos->arg1_uint;
	decLocal(context, t);
	++(context->exec_pos);
}
void ABCVm::abc_typeof(call_context* context)
{
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	LOG_CALL("typeOf");
	asAtom ret = asAtomHandler::typeOf(*pval);
	ASATOM_DECREF_POINTER(pval);
	*pval = ret;
	++(context->exec_pos);
}
void ABCVm::abc_not(call_context* context)
{
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	asAtomHandler::_not(*pval);
	++(context->exec_pos);
}
void ABCVm::abc_bitnot(call_context* context)
{
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	asAtomHandler::bitnot(*pval,context->worker);

	++(context->exec_pos);
}
void ABCVm::abc_add(call_context* context)
{
	LOG_CALL("add");
	RUNTIME_STACK_POP_CREATE(context,v2);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	ASObject* o = asAtomHandler::getObject(*pval);
	if (asAtomHandler::add(*pval,*v2,context->worker,context->exec_pos->local3.flags & ABC_OP_FORCEINT))
	{
		if (o)
			o->decRef();
	}
	ASATOM_DECREF_POINTER(v2);
	++(context->exec_pos);
}
void ABCVm::abc_subtract(call_context* context)
{
	//Be careful, operands in subtract implementation are swapped
	RUNTIME_STACK_POP_CREATE(context,v2);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	ASObject* o = asAtomHandler::getObject(*pval);
	asAtomHandler::subtract(*pval,context->worker,*v2,context->exec_pos->local3.flags & ABC_OP_FORCEINT);
	ASATOM_DECREF_POINTER(v2);
	if (o)
		o->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_multiply(call_context* context)
{
	RUNTIME_STACK_POP_CREATE(context,v2);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	ASObject* o = asAtomHandler::getObject(*pval);
	asAtomHandler::multiply(*pval,context->worker,*v2,context->exec_pos->local3.flags & ABC_OP_FORCEINT);
	ASATOM_DECREF_POINTER(v2);
	if (o)
		o->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_divide(call_context* context)
{
	RUNTIME_STACK_POP_CREATE(context,v2);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	ASObject* o = asAtomHandler::getObject(*pval);
	asAtomHandler::divide(*pval,context->worker,*v2,context->exec_pos->local3.flags & ABC_OP_FORCEINT);
	ASATOM_DECREF_POINTER(v2);
	if (o)
		o->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_modulo(call_context* context)
{
	RUNTIME_STACK_POP_CREATE(context,v2);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	ASObject* o = asAtomHandler::getObject(*pval);
	asAtomHandler::modulo(*pval,context->worker,*v2,context->exec_pos->local3.flags & ABC_OP_FORCEINT);
	ASATOM_DECREF_POINTER(v2);
	if (o)
		o->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_lshift(call_context* context)
{
	//lshift
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	ASObject* o = asAtomHandler::getObject(*pval);
	asAtomHandler::lshift(*pval,context->worker,*v1);
	ASATOM_DECREF_POINTER(v1);
	if (o)
		o->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_rshift(call_context* context)
{
	//rshift
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	ASObject* o = asAtomHandler::getObject(*pval);
	asAtomHandler::rshift(*pval,context->worker,*v1);
	ASATOM_DECREF_POINTER(v1);
	if (o)
		o->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_urshift(call_context* context)
{
	//urshift
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	ASObject* o = asAtomHandler::getObject(*pval);
	asAtomHandler::urshift(*pval,context->worker,*v1);
	ASATOM_DECREF_POINTER(v1);
	if (o)
		o->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_bitand(call_context* context)
{
	//bitand
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	ASObject* o = asAtomHandler::getObject(*pval);
	asAtomHandler::bit_and(*pval,context->worker,*v1);
	ASATOM_DECREF_POINTER(v1);
	if (o)
		o->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_bitor(call_context* context)
{
	//bitor
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	ASObject* o = asAtomHandler::getObject(*pval);
	asAtomHandler::bit_or(*pval,context->worker,*v1);
	ASATOM_DECREF_POINTER(v1);
	if (o)
		o->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_bitxor(call_context* context)
{
	//bitxor
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	ASObject* o = asAtomHandler::getObject(*pval);
	asAtomHandler::bit_xor(*pval,context->worker,*v1);
	ASATOM_DECREF_POINTER(v1);
	if (o)
		o->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_equals(call_context* context)
{
	//equals
	RUNTIME_STACK_POP_CREATE(context,v2);
	RUNTIME_STACK_POINTER_CREATE(context,pval);

	bool ret=asAtomHandler::isEqual(*pval,context->worker,*v2);
	LOG_CALL( "equals " << ret);
	ASATOM_DECREF_POINTER(pval);
	ASATOM_DECREF_POINTER(v2);
	asAtomHandler::setBool(*pval,ret);
	++(context->exec_pos);
}
void ABCVm::abc_strictequals(call_context* context)
{
	//strictequals
	RUNTIME_STACK_POP_CREATE(context,v2);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	bool ret = asAtomHandler::isEqualStrict(*pval,context->worker,*v2);
	LOG_CALL( "strictequals " << ret);
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
	bool ret=(asAtomHandler::isLess(*pval,context->worker,*v2)==TTRUE);
	LOG_CALL("lessThan "<<ret<<" "<<asAtomHandler::toDebugString(*pval)<<" "<<asAtomHandler::toDebugString(*v2));
	ASATOM_DECREF_POINTER(pval);
	ASATOM_DECREF_POINTER(v2);

	asAtomHandler::setBool(*pval,ret);
	++(context->exec_pos);
}
void ABCVm::abc_lessequals(call_context* context)
{
	//lessequals
	RUNTIME_STACK_POP_CREATE(context,v2);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	//Real comparision demanded to object
	bool ret=(asAtomHandler::isLess(*v2,context->worker,*pval)==TFALSE);
	LOG_CALL("lessEquals "<<ret);
	ASATOM_DECREF_POINTER(pval);
	ASATOM_DECREF_POINTER(v2);

	asAtomHandler::setBool(*pval,ret);
	++(context->exec_pos);
}
void ABCVm::abc_greaterthan(call_context* context)
{
	//greaterthan
	RUNTIME_STACK_POP_CREATE(context,v2);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	//Real comparision demanded to object
	bool ret=(asAtomHandler::isLess(*v2,context->worker,*pval)==TTRUE);
	LOG_CALL("greaterThan "<<ret);
	ASATOM_DECREF_POINTER(pval);
	ASATOM_DECREF_POINTER(v2);

	asAtomHandler::setBool(*pval,ret);
	++(context->exec_pos);
}
void ABCVm::abc_greaterequals(call_context* context)
{
	//greaterequals
	RUNTIME_STACK_POP_CREATE(context,v2);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	//Real comparision demanded to object
	bool ret=(asAtomHandler::isLess(*pval,context->worker,*v2)==TFALSE);
	LOG_CALL("greaterEquals "<<ret);
	ASATOM_DECREF_POINTER(pval);
	ASATOM_DECREF_POINTER(v2);

	asAtomHandler::setBool(*pval,ret);
	++(context->exec_pos);
}
void ABCVm::abc_instanceof(call_context* context)
{
	//instanceof
	RUNTIME_STACK_POP_CREATE_ASOBJECT(context,type);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	bool ret = instanceOf(asAtomHandler::toObject(*pval,context->worker),type);
	ASATOM_DECREF_POINTER(pval);
	asAtomHandler::setBool(*pval,ret);
	++(context->exec_pos);
}
void ABCVm::abc_istype(call_context* context)
{
	uint32_t t = context->exec_pos->arg1_uint;
	multiname* name=context->mi->context->getMultiname(t,nullptr);

	RUNTIME_STACK_POINTER_CREATE(context,pval);
	ASObject* o = asAtomHandler::toObject(*pval,context->worker);
	asAtomHandler::setBool(*pval,isType(context->mi->context,o,name));
	o->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_istypelate(call_context* context)
{
	//istypelate
	RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v1);
	RUNTIME_STACK_POINTER_CREATE(context,pval);

	bool ret =asAtomHandler::isTypelate(*pval,v1);
	ASATOM_DECREF_POINTER(pval);
	asAtomHandler::setBool(*pval,ret);
	++(context->exec_pos);
}
void ABCVm::abc_in(call_context* context)
{
	RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v1);
	RUNTIME_STACK_POINTER_CREATE(context,pval);

	LOG_CALL( "in" );
	if(v1->is<Null>())
	{
		ASATOM_DECREF_POINTER(pval);
		v1->decRef();
		createError<TypeError>(context->worker,kConvertNullToObjectError);
		return;
	}

	multiname name(nullptr);
	asAtomHandler::fillMultiname(*pval,context->worker,name);
	name.ns.emplace_back(context->sys,BUILTIN_STRINGS::EMPTY,NAMESPACE);
	bool ret=v1->hasPropertyByMultiname(name, true, true,context->worker);
	ASATOM_DECREF_POINTER(pval);
	v1->decRef();
	asAtomHandler::setBool(*pval,ret);

	++(context->exec_pos);
}
void ABCVm::abc_increment_i(call_context* context)
{
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	LOG_CALL("increment_i:"<<asAtomHandler::toDebugString(*pval));
	asAtomHandler::increment_i(*pval,context->worker);
	++(context->exec_pos);
}
void ABCVm::abc_decrement_i(call_context* context)
{
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	LOG_CALL("decrement_i:"<<asAtomHandler::toDebugString(*pval));
	asAtomHandler::decrement_i(*pval,context->worker);
	++(context->exec_pos);
}
void ABCVm::abc_inclocal_i(call_context* context)
{
	uint32_t t = context->exec_pos->arg1_uint;
	incLocal_i(context, t);
	++(context->exec_pos);
}
void ABCVm::abc_declocal_i(call_context* context)
{
	uint32_t t = context->exec_pos->arg1_uint;
	decLocal_i(context, t);
	++(context->exec_pos);
}
void ABCVm::abc_negate_i(call_context* context)
{
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	asAtomHandler::negate_i(*pval,context->worker);
	++(context->exec_pos);
}
void ABCVm::abc_add_i(call_context* context)
{
	RUNTIME_STACK_POP_CREATE(context,v2);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	asAtomHandler::add_i(*pval,context->worker,*v2);
	++(context->exec_pos);
}
void ABCVm::abc_subtract_i(call_context* context)
{
	RUNTIME_STACK_POP_CREATE(context,v2);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	asAtomHandler::subtract_i(*pval,context->worker,*v2);
	++(context->exec_pos);
}
void ABCVm::abc_multiply_i(call_context* context)
{
	RUNTIME_STACK_POP_CREATE(context,v2);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	asAtomHandler::multiply_i(*pval,context->worker,*v2);
	++(context->exec_pos);
}
void ABCVm::abc_getlocal_0(call_context* context)
{
	int i=0;
	LOG_CALL( "getLocal " << i << ": " << asAtomHandler::toDebugString(context->locals[i]) );
	ASATOM_INCREF(context->locals[i]);
	RUNTIME_STACK_PUSH(context,context->locals[i]);
	++(context->exec_pos);
}
void ABCVm::abc_getlocal_1(call_context* context)
{
	int i=1;
	LOG_CALL( "getLocal " << i << ": " << asAtomHandler::toDebugString(context->locals[i]) );
	ASATOM_INCREF(context->locals[i]);
	RUNTIME_STACK_PUSH(context,context->locals[i]);
	++(context->exec_pos);
}
void ABCVm::abc_getlocal_2(call_context* context)
{
	int i=2;
	LOG_CALL( "getLocal " << i << ": " << asAtomHandler::toDebugString(context->locals[i]) );
	ASATOM_INCREF(context->locals[i]);
	RUNTIME_STACK_PUSH(context,context->locals[i]);
	++(context->exec_pos);
}
void ABCVm::abc_getlocal_3(call_context* context)
{
	int i=3;
	LOG_CALL( "getLocal " << i << ": " << asAtomHandler::toDebugString(context->locals[i]) );
	ASATOM_INCREF(context->locals[i]);
	RUNTIME_STACK_PUSH(context,context->locals[i]);
	++(context->exec_pos);
}
void ABCVm::abc_setlocal_0(call_context* context)
{
	unsigned int i=0;
	LOG_CALL( "setLocal " << i);
	RUNTIME_STACK_POP_CREATE(context,obj)
	if (USUALLY_FALSE(i >= context->mi->body->getReturnValuePos()))
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
	unsigned int i=1;
	LOG_CALL( "setLocal " << i);
	++(context->exec_pos);

	RUNTIME_STACK_POP_CREATE(context,obj)
	if (USUALLY_FALSE(i >= context->mi->body->getReturnValuePos()))
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
	unsigned int i=2;
	++(context->exec_pos);
	RUNTIME_STACK_POP_CREATE(context,obj)
	LOG_CALL( "setLocal " << i<<" "<<asAtomHandler::toDebugString(*obj));
	if (USUALLY_FALSE(i >= context->mi->body->getReturnValuePos()))
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
	unsigned int i=3;
	++(context->exec_pos);
	LOG_CALL( "setLocal " << i);
	RUNTIME_STACK_POP_CREATE(context,obj)
	if (USUALLY_FALSE(i >= context->mi->body->getReturnValuePos()))
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
	LOG_CALL( "debug" );
	++(context->exec_pos);
	++(context->exec_pos);
	++(context->exec_pos);
	++(context->exec_pos);
	++(context->exec_pos);
}
void ABCVm::abc_debugline(call_context* context)
{
	//debugline
	LOG_CALL( "debugline" );
	++(context->exec_pos);
	++(context->exec_pos);
}
void ABCVm::abc_debugfile(call_context* context)
{
	//debugfile
	LOG_CALL( "debugfile" );
	++(context->exec_pos);
	++(context->exec_pos);
}
void ABCVm::abc_bkptline(call_context* context)
{
	//bkptline
	LOG_CALL( "bkptline" );
	++(context->exec_pos);
	++(context->exec_pos);
}
void ABCVm::abc_timestamp(call_context* context)
{
	//timestamp
	LOG_CALL( "timestamp" );
	++(context->exec_pos);
}
void ABCVm::abc_invalidinstruction(call_context* context)
{
	LOG(LOG_ERROR,"invalid instruction");
	throw ParseException("Not implemented instruction in interpreter");
}
