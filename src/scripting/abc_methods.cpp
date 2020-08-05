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

void ABCVm::abc_bkpt(call_context* context)
{
	//bkpt
	LOG_CALL( _("bkpt") );
	++(context->exec_pos);
}
void ABCVm::abc_nop(call_context* context)
{
	//nop
	++(context->exec_pos);
}
void ABCVm::abc_throw(call_context* context)
{
	//throw
	_throw(context);
}
void ABCVm::abc_getSuper(call_context* context)
{
	//getsuper
	uint32_t t = (++(context->exec_pos))->data;
	getSuper(context,t);
	++(context->exec_pos);
}
void ABCVm::abc_setSuper(call_context* context)
{
	//setsuper
	uint32_t t = (++(context->exec_pos))->data;
	setSuper(context,t);
	++(context->exec_pos);
}
void ABCVm::abc_dxns(call_context* context)
{
	//dxns
	uint32_t t = (++(context->exec_pos))->data;
	dxns(context,t);
	++(context->exec_pos);
}
void ABCVm::abc_dxnslate(call_context* context)
{
	//dxnslate
	RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v,context->mi->context->root->getSystemState());
	dxnslate(context, v);
	++(context->exec_pos);
}
void ABCVm::abc_kill(call_context* context)
{
	//kill
	uint32_t t = (++(context->exec_pos))->data;
	LOG_CALL( "kill " << t);
	ASATOM_DECREF(context->locals[t]);
	context->locals[t]=asAtomHandler::undefinedAtom;
	++(context->exec_pos);
}
void ABCVm::abc_label(call_context* context)
{
	//label
	LOG_CALL("label");
	++(context->exec_pos);
}
void ABCVm::abc_ifnlt(call_context* context)
{
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POP_CREATE(context,v2);
	bool cond=!(asAtomHandler::isLess(*v2,context->mi->context->root->getSystemState(),*v1) == TTRUE);
	ASATOM_DECREF_POINTER(v2);
	ASATOM_DECREF_POINTER(v1);
	LOG_CALL(_("ifNLT (") << ((cond)?_("taken)"):_("not taken)")));

	if(cond)
		context->exec_pos += (*context->exec_pos).jumpdata.jump+1;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifnle(call_context* context)
{
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POP_CREATE(context,v2);
	bool cond=!(asAtomHandler::isLess(*v1,context->mi->context->root->getSystemState(),*v2) == TFALSE);
	ASATOM_DECREF_POINTER(v2);
	ASATOM_DECREF_POINTER(v1);
	LOG_CALL(_("ifNLE (") << ((cond)?_("taken)"):_("not taken)")));

	if(cond)
		context->exec_pos += (*context->exec_pos).jumpdata.jump+1;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifngt(call_context* context)
{
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POP_CREATE(context,v2);
	bool cond=!(asAtomHandler::isLess(*v1,context->mi->context->root->getSystemState(),*v2) == TTRUE);
	ASATOM_DECREF_POINTER(v2);
	ASATOM_DECREF_POINTER(v1);
	LOG_CALL(_("ifNGT (") << ((cond)?_("taken)"):_("not taken)")));

	if(cond)
		context->exec_pos += (*context->exec_pos).jumpdata.jump+1;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifnge(call_context* context)
{
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POP_CREATE(context,v2);
	bool cond=!(asAtomHandler::isLess(*v2,context->mi->context->root->getSystemState(),*v1) == TFALSE);
	ASATOM_DECREF_POINTER(v2);
	ASATOM_DECREF_POINTER(v1);
	LOG_CALL(_("ifNGE (") << ((cond)?_("taken)"):_("not taken)")));

	if(cond)
		context->exec_pos += (*context->exec_pos).jumpdata.jump+1;
	else
		++(context->exec_pos);
}
void ABCVm::abc_jump(call_context* context)
{
	LOG_CALL("jump:"<<(*context->exec_pos).jumpdata.jump);
	context->exec_pos += (*context->exec_pos).jumpdata.jump+1;
}
void ABCVm::abc_iftrue(call_context* context)
{
	RUNTIME_STACK_POP_CREATE(context,v1);
	bool cond=asAtomHandler::Boolean_concrete(*v1);
	LOG_CALL(_("ifTrue (") << ((cond)?_("taken)"):_("not taken)")));
	ASATOM_DECREF_POINTER(v1);
	if(cond)
		context->exec_pos += (*context->exec_pos).jumpdata.jump+1;
	else
		++(context->exec_pos);
}
void ABCVm::abc_iffalse(call_context* context)
{
	RUNTIME_STACK_POP_CREATE(context,v1);
	bool cond=!asAtomHandler::Boolean_concrete(*v1);
	LOG_CALL(_("ifFalse (") << ((cond)?_("taken"):_("not taken")) << ')');
	ASATOM_DECREF_POINTER(v1);
	if(cond)
		context->exec_pos += (*context->exec_pos).jumpdata.jump+1;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifeq(call_context* context)
{
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POP_CREATE(context,v2);
	bool cond=(asAtomHandler::isEqual(*v1,context->mi->context->root->getSystemState(),*v2));
	LOG_CALL(_("ifEq (") << ((cond)?_("taken)"):_("not taken)")));
	ASATOM_DECREF_POINTER(v2);
	ASATOM_DECREF_POINTER(v1);
	if(cond)
		context->exec_pos += (*context->exec_pos).jumpdata.jump+1;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifne(call_context* context)
{
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POP_CREATE(context,v2);
	bool cond=!(asAtomHandler::isEqual(*v1,context->mi->context->root->getSystemState(),*v2));
	LOG_CALL(_("ifNE (") << ((cond)?_("taken)"):_("not taken)")));
	ASATOM_DECREF_POINTER(v2);
	ASATOM_DECREF_POINTER(v1);
	if(cond)
		context->exec_pos += (*context->exec_pos).jumpdata.jump+1;
	else
		++(context->exec_pos);
}
void ABCVm::abc_iflt(call_context* context)
{
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POP_CREATE(context,v2);
	bool cond=asAtomHandler::isLess(*v2,context->mi->context->root->getSystemState(),*v1) == TTRUE;
	ASATOM_DECREF_POINTER(v2);
	ASATOM_DECREF_POINTER(v1);
	LOG_CALL(_("ifLT (") << ((cond)?_("taken)"):_("not taken)")));

	if(cond)
		context->exec_pos += (*context->exec_pos).jumpdata.jump+1;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifle(call_context* context)
{
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POP_CREATE(context,v2);
	bool cond=asAtomHandler::isLess(*v1,context->mi->context->root->getSystemState(),*v2) == TFALSE;
	ASATOM_DECREF_POINTER(v2);
	ASATOM_DECREF_POINTER(v1);
	LOG_CALL(_("ifLE (") << ((cond)?_("taken)"):_("not taken)")));

	if(cond)
		context->exec_pos += (*context->exec_pos).jumpdata.jump+1;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifgt(call_context* context)
{
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POP_CREATE(context,v2);
	bool cond=asAtomHandler::isLess(*v1,context->mi->context->root->getSystemState(),*v2) == TTRUE;
	ASATOM_DECREF_POINTER(v2);
	ASATOM_DECREF_POINTER(v1);
	LOG_CALL(_("ifGT (") << ((cond)?_("taken)"):_("not taken)")));

	if(cond)
		context->exec_pos += (*context->exec_pos).jumpdata.jump+1;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifge(call_context* context)
{
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POP_CREATE(context,v2);
	bool cond=asAtomHandler::isLess(*v2,context->mi->context->root->getSystemState(),*v1) == TFALSE;
	ASATOM_DECREF_POINTER(v2);
	ASATOM_DECREF_POINTER(v1);
	LOG_CALL(_("ifGE (") << ((cond)?_("taken)"):_("not taken)")));

	if(cond)
		context->exec_pos += (*context->exec_pos).jumpdata.jump+1;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifstricteq(call_context* context)
{
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POP_CREATE(context,v2);

	bool cond=asAtomHandler::isEqualStrict(*v1,context->mi->context->root->getSystemState(),*v2);
	LOG_CALL(_("ifStrictEq ")<<cond);
	ASATOM_DECREF_POINTER(v1);
	ASATOM_DECREF_POINTER(v2);
	if(cond)
		context->exec_pos += (*context->exec_pos).jumpdata.jump+1;
	else
		++(context->exec_pos);
}
void ABCVm::abc_ifstrictne(call_context* context)
{
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POP_CREATE(context,v2);

	bool cond=!asAtomHandler::isEqualStrict(*v1,context->mi->context->root->getSystemState(),*v2);
	LOG_CALL(_("ifStrictNE ")<<cond <<" "<<asAtomHandler::toDebugString(*v1)<<" "<<asAtomHandler::toDebugString(*v2));
	ASATOM_DECREF_POINTER(v1);
	ASATOM_DECREF_POINTER(v2);
	if(cond)
		context->exec_pos += (*context->exec_pos).jumpdata.jump+1;
	else
		++(context->exec_pos);
}
void ABCVm::abc_lookupswitch(call_context* context)
{
	//lookupswitch
	preloadedcodedata* here=context->exec_pos; //Base for the jumps is the instruction itself for the switch
	int32_t t = (++(context->exec_pos))->idata;
	preloadedcodedata* defaultdest=here+t;
	LOG_CALL(_("Switch default dest ") << t);
	uint32_t count = (++(context->exec_pos))->data;

	RUNTIME_STACK_POP_CREATE(context,index_obj);
	assert_and_throw(asAtomHandler::isNumeric(*index_obj));
	unsigned int index=asAtomHandler::toUInt(*index_obj);
	ASATOM_DECREF_POINTER(index_obj);

	preloadedcodedata* dest=defaultdest;
	if(index<=count)
		dest=here+(context->exec_pos+index+1)->idata;

	context->exec_pos = dest;
}
void ABCVm::abc_pushwith(call_context* context)
{
	//pushwith
	pushWith(context);
	++(context->exec_pos);
}
void ABCVm::abc_popscope(call_context* context)
{
	//popscope
	popScope(context);
	++(context->exec_pos);
}
void ABCVm::abc_nextname(call_context* context)
{
	//nextname
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	LOG_CALL("nextName");
	if(!asAtomHandler::isUInteger(*v1))
		throw UnsupportedException("Type mismatch in nextName");

	asAtom ret=asAtomHandler::invalidAtom;
	asAtomHandler::toObject(*pval,context->mi->context->root->getSystemState())->nextName(ret,asAtomHandler::toUInt(*v1));
	ASATOM_DECREF_POINTER(pval);
	ASATOM_DECREF_POINTER(v1);
	*pval = ret;
	++(context->exec_pos);
}
void ABCVm::abc_hasnext(call_context* context)
{
	//hasnext
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	LOG_CALL("hasNext " << asAtomHandler::toDebugString(*v1) << ' ' << asAtomHandler::toDebugString(*pval));

	uint32_t curIndex=asAtomHandler::toUInt(*pval);

	uint32_t newIndex=asAtomHandler::toObject(*v1,context->mi->context->root->getSystemState())->nextNameIndex(curIndex);
	ASATOM_DECREF_POINTER(pval);
	ASATOM_DECREF_POINTER(v1);
	asAtomHandler::setInt(*pval,context->mi->context->root->getSystemState(),newIndex);
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
	if(!asAtomHandler::isUInteger(*v1))
		throw UnsupportedException("Type mismatch in nextValue");

	asAtom ret=asAtomHandler::invalidAtom;
	asAtomHandler::toObject(*pval,context->mi->context->root->getSystemState())->nextValue(ret,asAtomHandler::toUInt(*v1));
	ASATOM_DECREF_POINTER(pval);
	ASATOM_DECREF_POINTER(v1);
	*pval=ret;
	++(context->exec_pos);
}
void ABCVm::abc_pushbyte(call_context* context)
{
	//pushbyte
	int32_t t = (++(context->exec_pos))->idata;
	LOG_CALL("pushbyte "<<(int)t);
	RUNTIME_STACK_PUSH(context,asAtomHandler::fromInt(t));
	++(context->exec_pos);
}
void ABCVm::abc_pushshort(call_context* context)
{
	//pushshort
	// specs say pushshort is a u30, but it's really a u32
	// see https://bugs.adobe.com/jira/browse/ASC-4181
	int32_t t = (++(context->exec_pos))->idata;
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
	RUNTIME_STACK_PUSH(context,context->mi->context->root->getSystemState()->nanAtom);
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
	//pushstring
	uint32_t t = (++(context->exec_pos))->data;
	LOG_CALL( _("pushString ") << context->mi->context->root->getSystemState()->getStringFromUniqueId(context->mi->context->getString(t)) );
	RUNTIME_STACK_PUSH(context,asAtomHandler::fromStringID(context->mi->context->getString(t)));
	++(context->exec_pos);
}
void ABCVm::abc_pushint(call_context* context)
{
	//pushint
	uint32_t t = (++(context->exec_pos))->data;
	s32 val=context->mi->context->constant_pool.integer[t];
	LOG_CALL( "pushInt " << val);

	RUNTIME_STACK_PUSH(context,asAtomHandler::fromInt(val));
	++(context->exec_pos);
}
void ABCVm::abc_pushuint(call_context* context)
{
	//pushuint
	uint32_t t = (++(context->exec_pos))->data;
	u32 val=context->mi->context->constant_pool.uinteger[t];
	LOG_CALL( "pushUInt " << val);

	RUNTIME_STACK_PUSH(context,asAtomHandler::fromUInt(val));
	++(context->exec_pos);
}
void ABCVm::abc_pushdouble(call_context* context)
{
	//pushdouble
	uint32_t t = (++(context->exec_pos))->data;
	asAtom* a = context->mi->context->getConstantAtom(OPERANDTYPES::OP_DOUBLE,t);
	LOG_CALL( "pushDouble " << asAtomHandler::toDebugString(*a));

	RUNTIME_STACK_PUSH(context,*a);
	++(context->exec_pos);
}
void ABCVm::abc_pushScope(call_context* context)
{
	//pushscope
	pushScope(context);
	++(context->exec_pos);
}
void ABCVm::abc_pushnamespace(call_context* context)
{
	//pushnamespace
	uint32_t t = (++(context->exec_pos))->data;
	RUNTIME_STACK_PUSH(context,asAtomHandler::fromObjectNoPrimitive(pushNamespace(context, t) ));
	++(context->exec_pos);
}
void ABCVm::abc_hasnext2(call_context* context)
{
	//hasnext2
	uint32_t t = (++(context->exec_pos))->data;
	uint32_t t2 = (++(context->exec_pos))->data;

	bool ret=hasNext2(context,t,t2);
	RUNTIME_STACK_PUSH(context,asAtomHandler::fromBool(ret));
	++(context->exec_pos);
}
//Alchemy opcodes
void ABCVm::abc_li8(call_context* context)
{
	//li8
	LOG_CALL( "li8");
	loadIntN<uint8_t>(context);
	++(context->exec_pos);
}
void ABCVm::abc_li16(call_context* context)
{
	//li16
	LOG_CALL( "li16");
	loadIntN<uint16_t>(context);
	++(context->exec_pos);
}
void ABCVm::abc_li32(call_context* context)
{
	//li32
	LOG_CALL( "li32");
	loadIntN<int32_t>(context);
	++(context->exec_pos);
}
void ABCVm::abc_lf32(call_context* context)
{
	//lf32
	LOG_CALL( "lf32");
	loadFloat(context);
	++(context->exec_pos);
}
void ABCVm::abc_lf64(call_context* context)
{
	//lf64
	LOG_CALL( "lf64");
	loadDouble(context);
	++(context->exec_pos);
}
void ABCVm::abc_si8(call_context* context)
{
	//si8
	LOG_CALL( "si8");
	storeIntN<uint8_t>(context);
	++(context->exec_pos);
}
void ABCVm::abc_si16(call_context* context)
{
	//si16
	LOG_CALL( "si16");
	storeIntN<uint16_t>(context);
	++(context->exec_pos);
}
void ABCVm::abc_si32(call_context* context)
{
	//si32
	LOG_CALL( "si32");
	storeIntN<uint32_t>(context);
	++(context->exec_pos);
}
void ABCVm::abc_sf32(call_context* context)
{
	//sf32
	LOG_CALL( "sf32");
	storeFloat(context);
	++(context->exec_pos);
}
void ABCVm::abc_sf64(call_context* context)
{
	//sf64
	LOG_CALL( "sf64");
	storeDouble(context);
	++(context->exec_pos);
}
void ABCVm::abc_newfunction(call_context* context)
{
	//newfunction
	uint32_t t = (++(context->exec_pos))->data;
	RUNTIME_STACK_PUSH(context,asAtomHandler::fromObjectNoPrimitive(newFunction(context,t)));
	++(context->exec_pos);
}
void ABCVm::abc_call(call_context* context)
{
	//call
	uint32_t t = (++(context->exec_pos))->data;
	method_info* called_mi=NULL;
	call(context,t,&called_mi);
	++(context->exec_pos);
}
void ABCVm::abc_construct(call_context* context)
{
	//construct
	uint32_t t = (++(context->exec_pos))->data;
	construct(context,t);
	++(context->exec_pos);
}
void ABCVm::abc_callMethod(call_context* context)
{
	// callmethod
	uint32_t t = (++(context->exec_pos))->data;
	uint32_t t2 = (++(context->exec_pos))->data;
	callMethod(context,t,t2);
	++(context->exec_pos);
}
void ABCVm::abc_callstatic(call_context* context)
{
	//callstatic
	uint32_t t = (++(context->exec_pos))->data;
	uint32_t t2 = (++(context->exec_pos))->data;
	callStatic(context,t,t2,NULL,true);
	++(context->exec_pos);
}
void ABCVm::abc_callsuper(call_context* context)
{
	//callsuper
	uint32_t t = (++(context->exec_pos))->data;
	uint32_t t2 = (++(context->exec_pos))->data;
	callSuper(context,t,t2,NULL,true);
	++(context->exec_pos);
}
void ABCVm::abc_callproperty(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t t = (++(context->exec_pos))->data;
	uint32_t t2 = (++(context->exec_pos))->data;
	callPropIntern(context,t,t2,true,false,instrptr);
	++(context->exec_pos);
}
void ABCVm::abc_returnvoid(call_context* context)
{
	//returnvoid
	LOG_CALL(_("returnVoid"));
	context->returnvalue = asAtomHandler::invalidAtom;
	context->returning = true;
	++(context->exec_pos);
}
void ABCVm::abc_returnvalue(call_context* context)
{
	//returnvalue
	RUNTIME_STACK_POP(context,context->returnvalue);
	LOG_CALL(_("returnValue ") << asAtomHandler::toDebugString(context->returnvalue));
	context->returning = true;
	++(context->exec_pos);
}
void ABCVm::abc_constructsuper(call_context* context)
{
	//constructsuper
	uint32_t t = (++(context->exec_pos))->data;
	constructSuper(context,t);
	++(context->exec_pos);
}
void ABCVm::abc_constructprop(call_context* context)
{
	//constructprop
	uint32_t t = (++(context->exec_pos))->data;
	uint32_t t2 = (++(context->exec_pos))->data;
	constructProp(context,t,t2);
	++(context->exec_pos);
}
void ABCVm::abc_callproplex(call_context* context)
{
	//callproplex
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t t = (++(context->exec_pos))->data;
	uint32_t t2 = (++(context->exec_pos))->data;
	callPropIntern(context,t,t2,true,true,instrptr);
	++(context->exec_pos);
}
void ABCVm::abc_callsupervoid(call_context* context)
{
	//callsupervoid
	uint32_t t = (++(context->exec_pos))->data;
	uint32_t t2 = (++(context->exec_pos))->data;
	method_info* called_mi=NULL;
	callSuper(context,t,t2,&called_mi,false);
	++(context->exec_pos);
}
void ABCVm::abc_callpropvoid(call_context* context)
{
	//callpropvoid
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t t = (++(context->exec_pos))->data;
	uint32_t t2 = (++(context->exec_pos))->data;
	callPropIntern(context,t,t2,false,false,instrptr);
	++(context->exec_pos);
}
void ABCVm::abc_sxi1(call_context* context)
{
	//sxi1
	LOG_CALL( "sxi1");
	RUNTIME_STACK_POINTER_CREATE(context,arg1);
	int32_t ret=asAtomHandler::toUInt(*arg1)&0x1 ? -1 : 0;
	ASATOM_DECREF_POINTER(arg1);
	asAtomHandler::setInt(*arg1,context->mi->context->root->getSystemState(),ret);
	++(context->exec_pos);
}
void ABCVm::abc_sxi8(call_context* context)
{
	//sxi8
	LOG_CALL( "sxi8");
	RUNTIME_STACK_POINTER_CREATE(context,arg1);
	int32_t ret=(int8_t)asAtomHandler::toUInt(*arg1);
	ASATOM_DECREF_POINTER(arg1);
	asAtomHandler::setInt(*arg1,context->mi->context->root->getSystemState(),ret);
	++(context->exec_pos);
}
void ABCVm::abc_sxi16(call_context* context)
{
	//sxi16
	LOG_CALL( "sxi16");
	RUNTIME_STACK_POINTER_CREATE(context,arg1);
	int32_t ret=(int16_t)asAtomHandler::toUInt(*arg1);
	ASATOM_DECREF_POINTER(arg1);
	asAtomHandler::setInt(*arg1,context->mi->context->root->getSystemState(),ret);
	++(context->exec_pos);
}
void ABCVm::abc_constructgenerictype(call_context* context)
{
	//constructgenerictype
	uint32_t t = (++(context->exec_pos))->data;
	constructGenericType(context, t);
	++(context->exec_pos);
}
void ABCVm::abc_newobject(call_context* context)
{
	//newobject
	uint32_t t = (++(context->exec_pos))->data;
	newObject(context,t);
	++(context->exec_pos);
}
void ABCVm::abc_newarray(call_context* context)
{
	//newarray
	uint32_t t = (++(context->exec_pos))->data;
	newArray(context,t);
	++(context->exec_pos);
}
void ABCVm::abc_newactivation(call_context* context)
{
	//newactivation
	RUNTIME_STACK_PUSH(context,asAtomHandler::fromObjectNoPrimitive(newActivation(context, context->mi)));
	++(context->exec_pos);
}
void ABCVm::abc_newclass(call_context* context)
{
	//newclass
	uint32_t t = (++(context->exec_pos))->data;
	newClass(context,t);
	++(context->exec_pos);
}
void ABCVm::abc_getdescendants(call_context* context)
{
	//getdescendants
	uint32_t t = (++(context->exec_pos))->data;
	getDescendants(context, t);
	++(context->exec_pos);
}
void ABCVm::abc_newcatch(call_context* context)
{
	//newcatch
	uint32_t t = (++(context->exec_pos))->data;
	RUNTIME_STACK_PUSH(context,asAtomHandler::fromObjectNoPrimitive(newCatch(context,t)));
	++(context->exec_pos);
}
void ABCVm::abc_findpropstrict(call_context* context)
{
	//findpropstrict
//		uint32_t t = (++(context->exec_pos))->data;
//		multiname* name=context->mi->context->getMultiname(t,context);
//		RUNTIME_STACK_PUSH(context,asAtomHandler::fromObject(findPropStrict(context,name)));
//		name->resetNameIfObject();

	asAtom o=asAtomHandler::invalidAtom;
	findPropStrictCache(o,context);
	RUNTIME_STACK_PUSH(context,o);
	++(context->exec_pos);
}
void ABCVm::abc_findproperty(call_context* context)
{
	//findproperty
	uint32_t t = (++(context->exec_pos))->data;
	multiname* name=context->mi->context->getMultiname(t,context);
	RUNTIME_STACK_PUSH(context,asAtomHandler::fromObject(findProperty(context,name)));
	name->resetNameIfObject();
	++(context->exec_pos);
}
void ABCVm::abc_finddef(call_context* context)
{
	//finddef
	uint32_t t = (++(context->exec_pos))->data;
	multiname* name=context->mi->context->getMultiname(t,context);
	ASObject* target = NULL;
	asAtom o=asAtomHandler::invalidAtom;
	getCurrentApplicationDomain(context)->getVariableAndTargetByMultiname(o,*name, target);
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
	//getlex
	preloadedcodedata* instrptr = context->exec_pos;
	if ((instrptr->data&ABC_OP_CACHED) == ABC_OP_CACHED)
	{
		RUNTIME_STACK_PUSH(context,asAtomHandler::fromObject(instrptr->cacheobj1));
		instrptr->cacheobj1->incRef();
		LOG_CALL( "getLex from cache: " <<  instrptr->cacheobj1->toDebugString());
	}
	else if (getLex_multiname(context,instrptr->cachedmultiname2,0))
	{
		// put object in cache
		instrptr->data |= ABC_OP_CACHED;
		RUNTIME_STACK_PEEK_CREATE(context,v);

		instrptr->cacheobj1 = asAtomHandler::getObject(*v);
		instrptr->cacheobj2 = asAtomHandler::getClosure(*v);
	}
	++(context->exec_pos);
}
void ABCVm::abc_setproperty(call_context* context)
{
	//setproperty
	uint32_t t = (++(context->exec_pos))->data;
	RUNTIME_STACK_POP_CREATE(context,value);

	multiname* name=context->mi->context->getMultiname(t,context);

	RUNTIME_STACK_POP_CREATE(context,obj)

	LOG_CALL(_("setProperty ") << *name << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));

	if(asAtomHandler::isNull(*obj))
	{
		LOG(LOG_ERROR,"calling setProperty on null:" << *name << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));
		ASATOM_DECREF_POINTER(obj);
		ASATOM_DECREF_POINTER(value);
		throwError<TypeError>(kConvertNullToObjectError);
	}
	if (asAtomHandler::isUndefined(*obj))
	{
		LOG(LOG_ERROR,"calling setProperty on undefined:" << *name << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));
		ASATOM_DECREF_POINTER(obj);
		ASATOM_DECREF_POINTER(value);
		throwError<TypeError>(kConvertUndefinedToObjectError);
	}
	//Do not allow to set contant traits
	ASObject* o = asAtomHandler::toObject(*obj,context->mi->context->root->getSystemState());
	bool alreadyset=false;
	o->setVariableByMultiname(*name,*value,ASObject::CONST_NOT_ALLOWED,&alreadyset);
	if (alreadyset)
		ASATOM_DECREF_POINTER(value);
	o->decRef();
	name->resetNameIfObject();
	++(context->exec_pos);
}
void ABCVm::abc_getlocal(call_context* context)
{
	//getlocal
	uint32_t i = ((context->exec_pos)++)->data>>OPCODE_SIZE;
	LOG_CALL( _("getLocal n ") << i << _(": ") << asAtomHandler::toDebugString(context->locals[i]) );
	ASATOM_INCREF(context->locals[i]);
	RUNTIME_STACK_PUSH(context,context->locals[i]);
}
void ABCVm::abc_setlocal(call_context* context)
{
	uint32_t i = ((context->exec_pos)++)->data>>OPCODE_SIZE;
	RUNTIME_STACK_POP_CREATE(context,obj)

	LOG_CALL( _("setLocal n ") << i << _(": ") << asAtomHandler::toDebugString(*obj) );
	assert(i <= context->mi->body->local_count+1+context->mi->body->localresultcount);
	if ((int)i != context->argarrayposition || asAtomHandler::isArray(*obj))
	{
		ASATOM_DECREF(context->locals[i]);
		context->locals[i]=*obj;
	}
}
void ABCVm::abc_getglobalscope(call_context* context)
{
	//getglobalscope
	asAtom ret = asAtomHandler::fromObject(getGlobalScope(context));
	ASATOM_INCREF(ret);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_getscopeobject(call_context* context)
{
	//getscopeobject
	uint32_t t = (++(context->exec_pos))->data;
	assert_and_throw(context->curr_scope_stack > (uint32_t)t);
	asAtom ret=context->scope_stack[t];
	ASATOM_INCREF(ret);
	LOG_CALL( _("getScopeObject: ") << asAtomHandler::toDebugString(ret)<<" "<<t);

	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_getProperty(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t t = (++(context->exec_pos))->data;
	ASObject* obj= NULL;
	if ((instrptr->data&ABC_OP_CACHED) == ABC_OP_CACHED)
	{
		RUNTIME_STACK_POP_ASOBJECT(context,obj, context->mi->context->root->getSystemState());
		if (instrptr->cacheobj1 == obj->getClass())
		{
			asAtom prop=asAtomHandler::invalidAtom;
			//Call the getter
			LOG_CALL("Calling the getter for " << *context->mi->context->getMultiname(t,context) << " on " << obj->toDebugString());
			assert(instrptr->cacheobj2->type == T_FUNCTION);
			IFunction* f = instrptr->cacheobj2->as<IFunction>();
			f->callGetter(prop,instrptr->cacheobj3 ? instrptr->cacheobj3 : obj);
			LOG_CALL("End of getter"<< ' ' << f->toDebugString()<<" result:"<<asAtomHandler::toDebugString(prop));
			if(asAtomHandler::isInvalid(prop))
			{
				multiname* name=context->mi->context->getMultiname(t,context);
				checkPropertyException(obj,name,prop);
			}
			obj->decRef();
			RUNTIME_STACK_PUSH(context,prop);
			++(context->exec_pos);
			return;
		}
	}
	multiname* name=context->mi->context->getMultiname(t,context);

	if (obj == NULL)
	{
		RUNTIME_STACK_POP_ASOBJECT(context,obj, context->mi->context->root->getSystemState());
	}

	LOG_CALL( _("getProperty ") << *name << ' ' << obj->toDebugString() << ' '<<obj->isInitialized());

	asAtom prop=asAtomHandler::invalidAtom;
	bool isgetter = obj->getVariableByMultiname(prop,*name,(name->isStatic && obj->getClass() && obj->getClass()->isSealed)? GET_VARIABLE_OPTION::DONT_CALL_GETTER:GET_VARIABLE_OPTION::NONE) & GET_VARIABLE_RESULT::GETVAR_ISGETTER;
	if (isgetter)
	{
		//Call the getter
		LOG_CALL("Calling the getter for " << *name << " on " << obj->toDebugString());
		assert(asAtomHandler::isFunction(prop));
		IFunction* f = asAtomHandler::as<IFunction>(prop);
		ASObject* closure = asAtomHandler::getClosure(prop);
		prop = asAtom();
		f->callGetter(prop,closure ? closure : obj);
		LOG_CALL("End of getter"<< ' ' << f->toDebugString()<<" result:"<<asAtomHandler::toDebugString(prop));
		if(asAtomHandler::isValid(prop))
		{
			// cache getter if multiname is static and it is a getter of a sealed class
			instrptr->data |= ABC_OP_CACHED;
			instrptr->cacheobj1 = obj->getClass();
			instrptr->cacheobj2 = f;
			instrptr->cacheobj3 = closure;
		}
	}
	if(asAtomHandler::isInvalid(prop))
		checkPropertyException(obj,name,prop);
	obj->decRef();
	name->resetNameIfObject();

	RUNTIME_STACK_PUSH(context,prop);
	++(context->exec_pos);
}
void ABCVm::abc_initproperty(call_context* context)
{
	//initproperty
	uint32_t t = (++(context->exec_pos))->data;
	RUNTIME_STACK_POP_CREATE(context,value);
	multiname* name=context->mi->context->getMultiname(t,context);
	RUNTIME_STACK_POP_CREATE(context,obj);
	LOG_CALL("initProperty "<<*name<<" on "<< asAtomHandler::toDebugString(*obj)<<" to "<<asAtomHandler::toDebugString(*value));
	bool alreadyset=false;
	asAtomHandler::toObject(*obj,context->mi->context->root->getSystemState())->setVariableByMultiname(*name,*value,ASObject::CONST_ALLOWED,&alreadyset);
	if (alreadyset)
		ASATOM_DECREF_POINTER(value);
	ASATOM_DECREF_POINTER(obj);
	name->resetNameIfObject();
	++(context->exec_pos);
}
void ABCVm::abc_deleteproperty(call_context* context)
{
	//deleteproperty
	uint32_t t = (++(context->exec_pos))->data;
	multiname* name = context->mi->context->getMultiname(t,context);
	RUNTIME_STACK_POP_CREATE_ASOBJECT(context,obj, context->mi->context->root->getSystemState());
	bool ret = deleteProperty(obj,name);
	name->resetNameIfObject();
	RUNTIME_STACK_PUSH(context,asAtomHandler::fromBool(ret));
	++(context->exec_pos);
}
void ABCVm::abc_getslot(call_context* context)
{
	//getslot
	uint32_t t = (++(context->exec_pos))->data;
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
	uint32_t t = (++(context->exec_pos))->data;
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v2, context->mi->context->root->getSystemState());

	LOG_CALL("setSlot " << t << " "<< v2->toDebugString() << " "<< asAtomHandler::toDebugString(*v1));
	if (!v2->setSlot(t,*v1))
		ASATOM_DECREF_POINTER(v1);
	v2->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_getglobalSlot(call_context* context)
{
	//getglobalSlot
	uint32_t t = (++(context->exec_pos))->data;

	Global* globalscope = getGlobalScope(context);
	asAtom ret=globalscope->getSlot(t);
	ASATOM_INCREF(ret);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void ABCVm::abc_setglobalSlot(call_context* context)
{
	//setglobalSlot
	uint32_t t = (++(context->exec_pos))->data;

	Global* globalscope = getGlobalScope(context);
	RUNTIME_STACK_POP_CREATE(context,o);
	globalscope->setSlot(t,*o);
	++(context->exec_pos);
}
void ABCVm::abc_convert_s(call_context* context)
{
	//convert_s
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	LOG_CALL( _("convert_s") );
	if(!asAtomHandler::isString(*pval))
	{
		tiny_string s = asAtomHandler::toString(*pval,context->mi->context->root->getSystemState());
		ASATOM_DECREF_POINTER(pval);
		asAtomHandler::replace(*pval,(ASObject*)abstract_s(context->mi->context->root->getSystemState(),s));
	}
	++(context->exec_pos);
}
void ABCVm::abc_esc_xelem(call_context* context)
{
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	asAtomHandler::replace(*pval,esc_xelem(asAtomHandler::toObject(*pval,context->mi->context->root->getSystemState())));
	++(context->exec_pos);
}
void ABCVm::abc_esc_xattr(call_context* context)
{
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	asAtomHandler::replace(*pval,esc_xattr(asAtomHandler::toObject(*pval,context->mi->context->root->getSystemState())));
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
		asAtomHandler::setInt(*pval,context->mi->context->root->getSystemState(),v);
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
		asAtomHandler::setUInt(*pval,context->mi->context->root->getSystemState(),v);
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
		asAtomHandler::setNumber(*pval,context->mi->context->root->getSystemState(),v);
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
		throwError<TypeError>(kConvertNullToObjectError);
	}
	if (asAtomHandler::isUndefined(*pval))
	{
		LOG(LOG_ERROR,"trying to call convert_o on undefined");
		throwError<TypeError>(kConvertUndefinedToObjectError);
	}
	++(context->exec_pos);
}
void ABCVm::abc_checkfilter(call_context* context)
{
	//checkfilter
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	checkfilter(asAtomHandler::toObject(*pval,context->mi->context->root->getSystemState()));
	++(context->exec_pos);
}
void ABCVm::abc_coerce(call_context* context)
{
	//coerce
	uint32_t t = (++(context->exec_pos))->data;
	coerce(context, t);
	++(context->exec_pos);
}
void ABCVm::abc_coerce_a(call_context* context)
{
	//coerce_a
	coerce_a();
	++(context->exec_pos);
}
void ABCVm::abc_coerce_s(call_context* context)
{
	//coerce_s
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	LOG_CALL("coerce_s:"<<asAtomHandler::toDebugString(*pval));
	if (!asAtomHandler::isString(*pval))
	{
		asAtom v = *pval;
		if (Class<ASString>::getClass(context->mi->context->root->getSystemState())->coerce(context->mi->context->root->getSystemState(),*pval))
			ASATOM_DECREF(v);
	}
	++(context->exec_pos);
}
void ABCVm::abc_astype(call_context* context)
{
	//astype
	uint32_t t = (++(context->exec_pos))->data;
	multiname* name=context->mi->context->getMultiname(t,NULL);

	RUNTIME_STACK_POINTER_CREATE(context,pval);
	asAtomHandler::replace(*pval,asType(context->mi->context, asAtomHandler::toObject(*pval,context->mi->context->root->getSystemState()), name));
	++(context->exec_pos);
}
void ABCVm::abc_astypelate(call_context* context)
{
	//astypelate
	RUNTIME_STACK_POP_CREATE(context,v1);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	*pval = asAtomHandler::asTypelate(*pval,*v1);
	++(context->exec_pos);
}
void ABCVm::abc_negate(call_context* context)
{
	//negate
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	LOG_CALL("negate "<<asAtomHandler::toDebugString(*pval));
	if (asAtomHandler::isInteger(*pval) && asAtomHandler::toInt(*pval) != 0)
	{
		int32_t ret=-(asAtomHandler::toInt(*pval));
		ASATOM_DECREF_POINTER(pval);
		asAtomHandler::setInt(*pval,context->mi->context->root->getSystemState(), ret);
	}
	else
	{
		number_t ret=-(asAtomHandler::toNumber(*pval));
		ASATOM_DECREF_POINTER(pval);
		asAtomHandler::setNumber(*pval,context->mi->context->root->getSystemState(), ret);
	}
	++(context->exec_pos);
}
void ABCVm::abc_increment(call_context* context)
{
	//increment
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	LOG_CALL("increment "<<asAtomHandler::toDebugString(*pval));
	asAtomHandler::increment(*pval,context->mi->context->root->getSystemState());
	++(context->exec_pos);
}
void ABCVm::abc_inclocal(call_context* context)
{
	//inclocal
	uint32_t t = (++(context->exec_pos))->data;
	incLocal(context, t);
	++(context->exec_pos);
}
void ABCVm::abc_decrement(call_context* context)
{
	//decrement
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	asAtomHandler::decrement(*pval,context->mi->context->root->getSystemState());
	++(context->exec_pos);
}
void ABCVm::abc_declocal(call_context* context)
{
	//declocal
	uint32_t t = (++(context->exec_pos))->data;
	decLocal(context, t);
	++(context->exec_pos);
}
void ABCVm::abc_typeof(call_context* context)
{
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	LOG_CALL(_("typeOf"));
	asAtom ret = asAtomHandler::typeOf(*pval);
	ASATOM_DECREF_POINTER(pval);
	*pval = ret;
	++(context->exec_pos);
}
void ABCVm::abc_not(call_context* context)
{
	//not
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	asAtomHandler::_not(*pval);
	++(context->exec_pos);
}
void ABCVm::abc_bitnot(call_context* context)
{
	//bitnot
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	asAtomHandler::bitnot(*pval,context->mi->context->root->getSystemState());

	++(context->exec_pos);
}
void ABCVm::abc_add(call_context* context)
{
	LOG_CALL("add");
	RUNTIME_STACK_POP_CREATE(context,v2);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	ASObject* o = asAtomHandler::getObject(*pval);
	if (asAtomHandler::add(*pval,*v2,context->mi->context->root->getSystemState()))
	{
		ASATOM_DECREF_POINTER(v2);
		if (o)
			o->decRef();
	}
	++(context->exec_pos);
}
void ABCVm::abc_subtract(call_context* context)
{
	//subtract
	//Be careful, operands in subtract implementation are swapped
	RUNTIME_STACK_POP_CREATE(context,v2);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	ASObject* o = asAtomHandler::getObject(*pval);
	asAtomHandler::subtract(*pval,context->mi->context->root->getSystemState(),*v2);
	ASATOM_DECREF_POINTER(v2);
	if (o)
		o->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_multiply(call_context* context)
{
	//multiply
	RUNTIME_STACK_POP_CREATE(context,v2);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	ASObject* o = asAtomHandler::getObject(*pval);
	asAtomHandler::multiply(*pval,context->mi->context->root->getSystemState(),*v2);
	ASATOM_DECREF_POINTER(v2);
	if (o)
		o->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_divide(call_context* context)
{
	//divide
	RUNTIME_STACK_POP_CREATE(context,v2);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	ASObject* o = asAtomHandler::getObject(*pval);
	asAtomHandler::divide(*pval,context->mi->context->root->getSystemState(),*v2);
	ASATOM_DECREF_POINTER(v2);
	if (o)
		o->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_modulo(call_context* context)
{
	//modulo
	RUNTIME_STACK_POP_CREATE(context,v2);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	ASObject* o = asAtomHandler::getObject(*pval);
	asAtomHandler::modulo(*pval,context->mi->context->root->getSystemState(),*v2);
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
	asAtomHandler::lshift(*pval,context->mi->context->root->getSystemState(),*v1);
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
	asAtomHandler::rshift(*pval,context->mi->context->root->getSystemState(),*v1);
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
	asAtomHandler::urshift(*pval,context->mi->context->root->getSystemState(),*v1);
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
	asAtomHandler::bit_and(*pval,context->mi->context->root->getSystemState(),*v1);
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
	asAtomHandler::bit_or(*pval,context->mi->context->root->getSystemState(),*v1);
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
	asAtomHandler::bit_xor(*pval,context->mi->context->root->getSystemState(),*v1);
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

	bool ret=asAtomHandler::isEqual(*pval,context->mi->context->root->getSystemState(),*v2);
	LOG_CALL( _("equals ") << ret);
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
	bool ret = asAtomHandler::isEqualStrict(*pval,context->mi->context->root->getSystemState(),*v2);
	LOG_CALL( _("strictequals ") << ret);
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
	bool ret=(asAtomHandler::isLess(*pval,context->mi->context->root->getSystemState(),*v2)==TTRUE);
	LOG_CALL(_("lessThan ")<<ret<<" "<<asAtomHandler::toDebugString(*pval)<<" "<<asAtomHandler::toDebugString(*v2));
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
	bool ret=(asAtomHandler::isLess(*v2,context->mi->context->root->getSystemState(),*pval)==TFALSE);
	LOG_CALL(_("lessEquals ")<<ret);
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
	bool ret=(asAtomHandler::isLess(*v2,context->mi->context->root->getSystemState(),*pval)==TTRUE);
	LOG_CALL(_("greaterThan ")<<ret);
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
	bool ret=(asAtomHandler::isLess(*pval,context->mi->context->root->getSystemState(),*v2)==TFALSE);
	LOG_CALL(_("greaterEquals ")<<ret);
	ASATOM_DECREF_POINTER(pval);
	ASATOM_DECREF_POINTER(v2);

	asAtomHandler::setBool(*pval,ret);
	++(context->exec_pos);
}
void ABCVm::abc_instanceof(call_context* context)
{
	//instanceof
	RUNTIME_STACK_POP_CREATE_ASOBJECT(context,type, context->mi->context->root->getSystemState());
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	bool ret = instanceOf(asAtomHandler::toObject(*pval,context->mi->context->root->getSystemState()),type);
	ASATOM_DECREF_POINTER(pval);
	asAtomHandler::setBool(*pval,ret);
	++(context->exec_pos);
}
void ABCVm::abc_istype(call_context* context)
{
	//istype
	uint32_t t = (++(context->exec_pos))->data;
	multiname* name=context->mi->context->getMultiname(t,NULL);

	RUNTIME_STACK_POINTER_CREATE(context,pval);
	ASObject* o = asAtomHandler::toObject(*pval,context->mi->context->root->getSystemState());
	asAtomHandler::setBool(*pval,isType(context->mi->context,o,name));
	o->decRef();
	++(context->exec_pos);
}
void ABCVm::abc_istypelate(call_context* context)
{
	//istypelate
	RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v1, context->mi->context->root->getSystemState());
	RUNTIME_STACK_POINTER_CREATE(context,pval);

	bool ret =asAtomHandler::isTypelate(*pval,v1);
	ASATOM_DECREF_POINTER(pval);
	asAtomHandler::setBool(*pval,ret);
	++(context->exec_pos);
}
void ABCVm::abc_in(call_context* context)
{
	//in
	RUNTIME_STACK_POP_CREATE_ASOBJECT(context,v1, context->mi->context->root->getSystemState());
	RUNTIME_STACK_POINTER_CREATE(context,pval);

	LOG_CALL( _("in") );
	if(v1->is<Null>())
		throwError<TypeError>(kConvertNullToObjectError);

	multiname name(NULL);
	asAtomHandler::fillMultiname(*pval,context->mi->context->root->getSystemState(),name);
	name.ns.emplace_back(context->mi->context->root->getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
	bool ret=v1->hasPropertyByMultiname(name, true, true);
	ASATOM_DECREF_POINTER(pval);
	v1->decRef();
	asAtomHandler::setBool(*pval,ret);

	++(context->exec_pos);
}
void ABCVm::abc_increment_i(call_context* context)
{
	//increment_i
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	LOG_CALL("increment_i:"<<asAtomHandler::toDebugString(*pval));
	asAtomHandler::increment_i(*pval,context->mi->context->root->getSystemState());
	++(context->exec_pos);
}
void ABCVm::abc_decrement_i(call_context* context)
{
	//decrement_i
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	LOG_CALL("decrement_i:"<<asAtomHandler::toDebugString(*pval));
	asAtomHandler::decrement_i(*pval,context->mi->context->root->getSystemState());
	++(context->exec_pos);
}
void ABCVm::abc_inclocal_i(call_context* context)
{
	//inclocal_i
	uint32_t t = (++(context->exec_pos))->data;
	incLocal_i(context, t);
	++(context->exec_pos);
}
void ABCVm::abc_declocal_i(call_context* context)
{
	//declocal_i
	uint32_t t = (++(context->exec_pos))->data;
	decLocal_i(context, t);
	++(context->exec_pos);
}
void ABCVm::abc_negate_i(call_context* context)
{
	//negate_i
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	asAtomHandler::negate_i(*pval,context->mi->context->root->getSystemState());
	++(context->exec_pos);
}
void ABCVm::abc_add_i(call_context* context)
{
	RUNTIME_STACK_POP_CREATE(context,v2);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	asAtomHandler::add_i(*pval,context->mi->context->root->getSystemState(),*v2);
	++(context->exec_pos);
}
void ABCVm::abc_subtract_i(call_context* context)
{
	//subtract_i
	RUNTIME_STACK_POP_CREATE(context,v2);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	asAtomHandler::subtract_i(*pval,context->mi->context->root->getSystemState(),*v2);
	++(context->exec_pos);
}
void ABCVm::abc_multiply_i(call_context* context)
{
	//multiply_i
	RUNTIME_STACK_POP_CREATE(context,v2);
	RUNTIME_STACK_POINTER_CREATE(context,pval);
	asAtomHandler::multiply_i(*pval,context->mi->context->root->getSystemState(),*v2);
	++(context->exec_pos);
}
void ABCVm::abc_getlocal_0(call_context* context)
{
	//getlocal_0
	int i=0;
	LOG_CALL( _("getLocal ") << i << _(": ") << asAtomHandler::toDebugString(context->locals[i]) );
	ASATOM_INCREF(context->locals[i]);
	RUNTIME_STACK_PUSH(context,context->locals[i]);
	++(context->exec_pos);
}
void ABCVm::abc_getlocal_1(call_context* context)
{
	//getlocal_1
	int i=1;
	LOG_CALL( _("getLocal ") << i << _(": ") << asAtomHandler::toDebugString(context->locals[i]) );
	ASATOM_INCREF(context->locals[i]);
	RUNTIME_STACK_PUSH(context,context->locals[i]);
	++(context->exec_pos);
}
void ABCVm::abc_getlocal_2(call_context* context)
{
	//getlocal_2
	int i=2;
	LOG_CALL( _("getLocal ") << i << _(": ") << asAtomHandler::toDebugString(context->locals[i]) );
	ASATOM_INCREF(context->locals[i]);
	RUNTIME_STACK_PUSH(context,context->locals[i]);
	++(context->exec_pos);
}
void ABCVm::abc_getlocal_3(call_context* context)
{
	//getlocal_3
	int i=3;
	LOG_CALL( _("getLocal ") << i << _(": ") << asAtomHandler::toDebugString(context->locals[i]) );
	ASATOM_INCREF(context->locals[i]);
	RUNTIME_STACK_PUSH(context,context->locals[i]);
	++(context->exec_pos);
}
void ABCVm::abc_setlocal_0(call_context* context)
{
	//setlocal_0
	unsigned int i=0;
	LOG_CALL( _("setLocal ") << i);
	RUNTIME_STACK_POP_CREATE(context,obj)
	if (i > context->mi->body->local_count)
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
	//setlocal_1
	unsigned int i=1;
	LOG_CALL( _("setLocal ") << i);
	++(context->exec_pos);

	RUNTIME_STACK_POP_CREATE(context,obj)
	if (i > context->mi->body->local_count)
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
	//setlocal_2
	unsigned int i=2;
	++(context->exec_pos);
	RUNTIME_STACK_POP_CREATE(context,obj)
	LOG_CALL( _("setLocal ") << i<<" "<<asAtomHandler::toDebugString(*obj));
	if (i > context->mi->body->local_count)
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
	//setlocal_3
	unsigned int i=3;
	++(context->exec_pos);
	LOG_CALL( _("setLocal ") << i);
	RUNTIME_STACK_POP_CREATE(context,obj)
	if (i > context->mi->body->local_count)
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
	//debug
	LOG_CALL( _("debug") );
	++(context->exec_pos);
	++(context->exec_pos);
	++(context->exec_pos);
	++(context->exec_pos);
	++(context->exec_pos);
}
void ABCVm::abc_debugline(call_context* context)
{
	//debugline
	LOG_CALL( _("debugline") );
	++(context->exec_pos);
	++(context->exec_pos);
}
void ABCVm::abc_debugfile(call_context* context)
{
	//debugfile
	LOG_CALL( _("debugfile") );
	++(context->exec_pos);
	++(context->exec_pos);
}
void ABCVm::abc_bkptline(call_context* context)
{
	//bkptline
	LOG_CALL( _("bkptline") );
	++(context->exec_pos);
	++(context->exec_pos);
}
void ABCVm::abc_timestamp(call_context* context)
{
	//timestamp
	LOG_CALL( _("timestamp") );
	++(context->exec_pos);
}
void ABCVm::abc_invalidinstruction(call_context* context)
{
	LOG(LOG_ERROR,"invalid instruction " << hex << (context->exec_pos)->data << dec);
	throw ParseException("Not implemented instruction in interpreter");
}
