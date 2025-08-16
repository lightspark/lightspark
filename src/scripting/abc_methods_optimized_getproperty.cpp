/**************************************************************************
    Lightspark, a free flash player implementation

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

#include "scripting/abc.h"
#include "scripting/abc_optimized.h"
#include "scripting/abc_optimized_getproperty.h"
#include "scripting/flash/errors/flasherrors.h"
#include "scripting/toplevel/Array.h"
#include "scripting/class.h"

using namespace std;
using namespace lightspark;

bool checkPropertyException(const asAtom& obj, multiname* name, asAtom& prop, ASWorker* wrk)
{
	if(asAtomHandler::isValid(prop))
	{
		name->resetNameIfObject();
		return false;
	}
	Class_base* cls = asAtomHandler::getClass(obj,wrk->getSystemState(),false) ;
	if (name->name_type != multiname::NAME_OBJECT // avoid calling toString() of multiname object
		&& cls && cls->findBorrowedSettable(*name))
	{
		createError<ReferenceError>(wrk,kWriteOnlyError, name->normalizedNameUnresolved(wrk->getSystemState()), cls->getQualifiedClassName());
		name->resetNameIfObject();
		return true;
	}
	else if (cls && cls->isSealed)
	{
		createError<ReferenceError>(wrk,kReadSealedError, name->normalizedNameUnresolved(wrk->getSystemState()), cls->getQualifiedClassName());
		name->resetNameIfObject();
		return true;
	}
	else if (name->isEmpty() || (!name->hasEmptyNS && !name->ns.empty()))
	{
		createError<ReferenceError>(wrk,kReadSealedErrorNs, name->normalizedNameUnresolved(wrk->getSystemState()), cls->getQualifiedClassName());
		name->resetNameIfObject();
		return true;
	}
	else if (asAtomHandler::isUndefined(obj))
	{
		createError<TypeError>(wrk,kConvertUndefinedToObjectError);
		name->resetNameIfObject();
		return true;
	}
	else if (asAtomHandler::isNull(obj))
	{
		createError<TypeError>(wrk,kConvertNullToObjectError);
		name->resetNameIfObject();
		return true;
	}
	name->resetNameIfObject();
	prop = asAtomHandler::undefinedAtom;
	return false;
}

FORCE_INLINE bool checkPropertyExceptionInteger(const asAtom& obj,int index, asAtom& prop,ASWorker* wrk)
{
	multiname m(nullptr);
	m.name_type = multiname::NAME_INT;
	m.name_i = index;
	return checkPropertyException(obj,&m, prop, wrk);
}

void lightspark::abc_getProperty_constant_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t t = (++(context->exec_pos))->arg3_uint;
	multiname* name=context->mi->context->getMultinameImpl(*instrptr->arg2_constant,nullptr,t,false);
	asAtom obj= *instrptr->arg1_constant;
	LOG_CALL( "getProperty_cc " << *name << ' ' << asAtomHandler::toDebugString(obj));
	asAtom prop=asAtomHandler::invalidAtom;
	bool canCache=false;
	asAtomHandler::getVariableByMultiname(obj,prop,*name,context->worker,canCache,GET_VARIABLE_OPTION::DONT_CHECK_PROTOTYPE);
	if (checkPropertyException(obj,name,prop,context->worker))
		return;
	RUNTIME_STACK_PUSH(context,prop);
	++(context->exec_pos);
}
void lightspark::abc_getProperty_local_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t t = (++(context->exec_pos))->arg3_uint;
	multiname* name=context->mi->context->getMultinameImpl(*instrptr->arg2_constant,nullptr,t,false);
	asAtom obj= CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	LOG_CALL( "getProperty_lc " << *name << ' ' << asAtomHandler::toDebugString(obj));
	asAtom prop=asAtomHandler::invalidAtom;
	bool canCache=false;
	asAtomHandler::getVariableByMultiname(obj,prop,*name,context->worker,canCache,GET_VARIABLE_OPTION::NONE);
	if (checkPropertyException(obj,name,prop,context->worker))
		return;
	RUNTIME_STACK_PUSH(context,prop);
	++(context->exec_pos);
}
void lightspark::abc_getProperty_constant_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t t = (++(context->exec_pos))->arg3_uint;
	multiname* name=context->mi->context->getMultinameImpl(CONTEXT_GETLOCAL(context,instrptr->local_pos2),nullptr,t,false);
	asAtom obj= *instrptr->arg1_constant;
	LOG_CALL( "getProperty_cl " << *name << ' ' << asAtomHandler::toDebugString(obj));
	asAtom prop=asAtomHandler::invalidAtom;
	bool canCache=false;
	asAtomHandler::getVariableByMultiname(obj,prop,*name,context->worker,canCache,GET_VARIABLE_OPTION::NONE);
	if (checkPropertyException(obj,name,prop,context->worker))
		return;
	RUNTIME_STACK_PUSH(context,prop);
	++(context->exec_pos);
}
void lightspark::abc_getProperty_local_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t t = (++(context->exec_pos))->arg3_uint;
	multiname* name=context->mi->context->getMultinameImpl(CONTEXT_GETLOCAL(context,instrptr->local_pos2),nullptr,t,false);
	asAtom obj= CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	LOG_CALL( "getProperty_ll " << *name <<"("<<instrptr->local_pos2<<")"<< ' ' << asAtomHandler::toDebugString(obj) <<"("<<instrptr->local_pos1<<")");
	asAtom prop=asAtomHandler::invalidAtom;
	bool canCache=false;
	asAtomHandler::getVariableByMultiname(obj,prop,*name,context->worker,canCache,GET_VARIABLE_OPTION::DONT_CHECK_PROTOTYPE);
	if (checkPropertyException(obj,name,prop,context->worker))
		return;
	RUNTIME_STACK_PUSH(context,prop);
	++(context->exec_pos);
}
void lightspark::abc_getProperty_constant_constant_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t t = (++(context->exec_pos))->arg3_uint;
	multiname* name=context->mi->context->getMultinameImpl(*instrptr->arg2_constant,nullptr,t,false);
	asAtom obj= *instrptr->arg1_constant;
	LOG_CALL( "getProperty_ccl " << *name << ' ' << asAtomHandler::toDebugString(obj));
	asAtom prop=asAtomHandler::invalidAtom;
	bool canCache=false;
	asAtomHandler::getVariableByMultiname(obj,prop,*name,context->worker,canCache,GET_VARIABLE_OPTION::DONT_CHECK_PROTOTYPE,instrptr->local3.pos);
	if (checkPropertyException(obj,name,prop,context->worker))
		return;
	REPLACELOCALRESULT(context,instrptr->local3.pos,prop);
	++(context->exec_pos);
}
void lightspark::abc_getProperty_local_constant_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t t = (++(context->exec_pos))->arg3_uint;
	asAtom prop=asAtomHandler::invalidAtom;
	asAtom obj= CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	if (asAtomHandler::isInteger(*instrptr->arg2_constant))
	{
		int index = asAtomHandler::toInt(*instrptr->arg2_constant);
		LOG_CALL( "getProperty_lcl int " << index << ' ' << asAtomHandler::toDebugString(obj));
		asAtomHandler::getVariableByInteger(obj,prop,index,context->worker);
		if (checkPropertyExceptionInteger(obj,index,prop,context->worker))
			return;
	}
	else
	{
		multiname* name=context->mi->context->getMultinameImpl(*instrptr->arg2_constant,nullptr,t,false);
		LOG_CALL( "getProperty_lcl " << *name << ' ' << asAtomHandler::toDebugString(obj));
		bool canCache=false;
		asAtomHandler::getVariableByMultiname(obj,prop,*name,context->worker,canCache,GET_VARIABLE_OPTION::DONT_CHECK_PROTOTYPE,instrptr->local3.pos);
		if (checkPropertyException(obj,name,prop,context->worker))
			return;
	}
	REPLACELOCALRESULT(context,instrptr->local3.pos,prop);
	++(context->exec_pos);
}
void lightspark::abc_getProperty_constant_local_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t t = (++(context->exec_pos))->arg3_uint;
	multiname* name=context->mi->context->getMultinameImpl(CONTEXT_GETLOCAL(context,instrptr->local_pos2),nullptr,t,false);
	asAtom obj= *instrptr->arg1_constant;
	LOG_CALL( "getProperty_cll " << *name << ' ' << asAtomHandler::toDebugString(obj));
	asAtom prop=asAtomHandler::invalidAtom;
	bool canCache=false;
	asAtomHandler::getVariableByMultiname(obj,prop,*name,context->worker,canCache,GET_VARIABLE_OPTION::DONT_CHECK_PROTOTYPE,instrptr->local3.pos);
	if (checkPropertyException(obj,name,prop,context->worker))
		return;
	REPLACELOCALRESULT(context,instrptr->local3.pos,prop);
	++(context->exec_pos);
}
void lightspark::abc_getProperty_local_local_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t t = (++(context->exec_pos))->arg3_uint;
	asAtom prop=asAtomHandler::invalidAtom;
	asAtom obj= CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	if (asAtomHandler::isInteger(CONTEXT_GETLOCAL(context,instrptr->local_pos2)))
	{
		int index = asAtomHandler::toInt(CONTEXT_GETLOCAL(context,instrptr->local_pos2));
		LOG_CALL( "getProperty_lll int " << index << ' ' << asAtomHandler::toDebugString(obj));
		asAtomHandler::getVariableByInteger(obj,prop,index,context->worker);
		if (checkPropertyExceptionInteger(obj,index,prop,context->worker))
			return;
	}
	else
	{
		multiname* name=context->mi->context->getMultinameImpl(CONTEXT_GETLOCAL(context,instrptr->local_pos2),nullptr,t,false);
		LOG_CALL( "getProperty_lll " << *name << ' ' << asAtomHandler::toDebugString(obj));
		bool canCache=false;
		asAtomHandler::getVariableByMultiname(obj,prop,*name,context->worker,canCache,GET_VARIABLE_OPTION::DONT_CHECK_PROTOTYPE,instrptr->local3.pos);
		if (checkPropertyException(obj,name,prop,context->worker))
			return;
	}
	REPLACELOCALRESULT(context,instrptr->local3.pos,prop);
	++(context->exec_pos);
}
void lightspark::abc_getPropertyInteger(call_context* context)
{
	RUNTIME_STACK_POP_CREATE(context,arg1);
	RUNTIME_STACK_POP_CREATE(context,obj);
	int index=asAtomHandler::toInt(*arg1);
	LOG_CALL( "getPropertyInteger " << index << ' ' << asAtomHandler::toDebugString(*obj));
	asAtom prop=asAtomHandler::invalidAtom;
	asAtomHandler::getVariableByInteger(*obj,prop,index,context->worker);
	if (checkPropertyExceptionInteger(*obj,index,prop,context->worker))
		return;
	ASATOM_DECREF(*obj);
	RUNTIME_STACK_PUSH(context,prop);
	++(context->exec_pos);
}
void lightspark::abc_getPropertyInteger_constant_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	int index=asAtomHandler::toInt(*instrptr->arg2_constant);
	asAtom obj= *instrptr->arg1_constant;
	LOG_CALL( "getPropertyInteger_cc " << index << ' ' << asAtomHandler::toDebugString(obj));
	asAtom prop=asAtomHandler::invalidAtom;
	asAtomHandler::getVariableByInteger(obj,prop,index,context->worker);
	if (checkPropertyExceptionInteger(obj,index,prop,context->worker))
		return;
	RUNTIME_STACK_PUSH(context,prop);
	++(context->exec_pos);
}
void lightspark::abc_getPropertyInteger_local_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	int index=asAtomHandler::toInt(*instrptr->arg2_constant);
	asAtom obj= CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	LOG_CALL( "getPropertyInteger_lc " << index << ' ' << asAtomHandler::toDebugString(obj));
	asAtom prop=asAtomHandler::invalidAtom;
	asAtomHandler::getVariableByInteger(obj,prop,index,context->worker);
	if (checkPropertyExceptionInteger(obj,index,prop,context->worker))
		return;
	RUNTIME_STACK_PUSH(context,prop);
	++(context->exec_pos);
}
void lightspark::abc_getPropertyInteger_constant_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	int index=asAtomHandler::toInt(CONTEXT_GETLOCAL(context,instrptr->local_pos2));
	asAtom obj= *instrptr->arg1_constant;
	LOG_CALL( "getPropertyInteger_cl " << index << ' ' << asAtomHandler::toDebugString(obj));
	asAtom prop=asAtomHandler::invalidAtom;
	asAtomHandler::getVariableByInteger(obj,prop,index,context->worker);
	if (checkPropertyExceptionInteger(obj,index,prop,context->worker))
		return;
	RUNTIME_STACK_PUSH(context,prop);
	++(context->exec_pos);
}
void lightspark::abc_getPropertyInteger_local_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	int index=asAtomHandler::toInt(CONTEXT_GETLOCAL(context,instrptr->local_pos2));
	asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	LOG_CALL( "getPropertyInteger_ll " << index <<"("<<instrptr->local_pos2<<")"<< ' ' << asAtomHandler::toDebugString(obj) <<"("<<instrptr->local_pos1<<")");
	asAtom prop=asAtomHandler::invalidAtom;
	asAtomHandler::getVariableByInteger(obj,prop,index,context->worker);
	if (checkPropertyExceptionInteger(obj,index,prop,context->worker))
		return;
	RUNTIME_STACK_PUSH(context,prop);
	++(context->exec_pos);
}
void lightspark::abc_getPropertyInteger_constant_constant_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	int index=asAtomHandler::toInt(*instrptr->arg2_constant);
	asAtom obj= *instrptr->arg1_constant;
	LOG_CALL( "getPropertyInteger_ccl " << index << ' ' << asAtomHandler::toDebugString(obj));
	asAtom prop=asAtomHandler::invalidAtom;
	asAtomHandler::getVariableByInteger(obj,prop,index,context->worker);
	if (checkPropertyExceptionInteger(obj,index,prop,context->worker))
		return;
	REPLACELOCALRESULT(context,instrptr->local3.pos,prop);
	++(context->exec_pos);
}
void lightspark::abc_getPropertyInteger_local_constant_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	int index=asAtomHandler::toInt(*instrptr->arg2_constant);
	asAtom obj= CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	LOG_CALL( "getPropertyInteger_lcl " << index << ' ' << asAtomHandler::toDebugString(obj));
	asAtom prop=asAtomHandler::invalidAtom;
	asAtomHandler::getVariableByInteger(obj,prop,index,context->worker);
	if (checkPropertyExceptionInteger(obj,index,prop,context->worker))
		return;
	REPLACELOCALRESULT(context,instrptr->local3.pos,prop);
	++(context->exec_pos);
}
void lightspark::abc_getPropertyInteger_constant_local_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	int index=asAtomHandler::toInt(CONTEXT_GETLOCAL(context,instrptr->local_pos2));
	asAtom obj= *instrptr->arg1_constant;
	LOG_CALL( "getPropertyInteger_cll " << index << ' ' << asAtomHandler::toDebugString(obj));
	asAtom prop=asAtomHandler::invalidAtom;
	asAtomHandler::getVariableByInteger(obj,prop,index,context->worker);
	if (checkPropertyExceptionInteger(obj,index,prop,context->worker))
		return;
	REPLACELOCALRESULT(context,instrptr->local3.pos,prop);
	++(context->exec_pos);
}
void lightspark::abc_getPropertyInteger_local_local_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	int index=asAtomHandler::toInt(CONTEXT_GETLOCAL(context,instrptr->local_pos2));
	asAtom obj= CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	LOG_CALL( "getPropertyInteger_lll " << index << ' ' << asAtomHandler::toDebugString(obj));
	asAtom prop=asAtomHandler::invalidAtom;
	asAtomHandler::getVariableByInteger(obj,prop,index,context->worker);
	if (checkPropertyExceptionInteger(obj,index,prop,context->worker))
		return;
	REPLACELOCALRESULT(context,instrptr->local3.pos,prop);
	++(context->exec_pos);
}
void lightspark::abc_getPropertyStaticName_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	multiname* name=instrptr->cachedmultiname2;

	asAtom obj= *instrptr->arg1_constant;
	LOG_CALL( "getProperty_sc " << *name << ' ' << asAtomHandler::toDebugString(obj));
	asAtom prop=asAtomHandler::invalidAtom;
	bool canCache=false;
	multiname* simplegetter = asAtomHandler::getVariableByMultiname(obj,prop,*name,context->worker,canCache,GET_VARIABLE_OPTION::NONE);
	if (simplegetter)
		instrptr->cachedmultiname2 = simplegetter;
	if(checkPropertyException(obj,name,prop,context->worker))
		return;
	RUNTIME_STACK_PUSH(context,prop);
	++(context->exec_pos);
}
void lightspark::abc_getPropertyStaticName_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom prop=asAtomHandler::invalidAtom;
	multiname* name=instrptr->cachedmultiname2;

	if (name->name_type == multiname::NAME_INT
		&& asAtomHandler::is<Array>(CONTEXT_GETLOCAL(context,instrptr->local_pos1))
		&& name->name_i > 0
		&& (uint32_t)name->name_i < asAtomHandler::as<Array>(CONTEXT_GETLOCAL(context,instrptr->local_pos1))->getCurrentSize())
	{
		LOG_CALL( "getProperty_sl int " << name->name_i << ' ' << asAtomHandler::toDebugString(CONTEXT_GETLOCAL(context,instrptr->local_pos1)));
		asAtomHandler::as<Array>(CONTEXT_GETLOCAL(context,instrptr->local_pos1))->at_nocheck(prop,name->name_i);
		if (asAtomHandler::isUndefined(CONTEXT_GETLOCAL(context,instrptr->local3.pos))) // item not found, check prototype
			asAtomHandler::as<Array>(CONTEXT_GETLOCAL(context,instrptr->local_pos1))->getVariableByInteger(CONTEXT_GETLOCAL(context,instrptr->local3.pos),name->name_i,GET_VARIABLE_OPTION::NO_INCREF,context->worker);
		ASATOM_INCREF(prop);
	}
	else
	{
		asAtom obj= CONTEXT_GETLOCAL(context,instrptr->local_pos1);
		LOG_CALL( "getProperty_sl " << *name << ' ' << asAtomHandler::toDebugString(obj));
		bool canCache=false;
		multiname* simplegetter = asAtomHandler::getVariableByMultiname(obj,prop,*name,context->worker,canCache,GET_VARIABLE_OPTION::NONE);
		if (simplegetter)
			instrptr->cachedmultiname2 = simplegetter;
		if(checkPropertyException(obj,name,prop,context->worker))
			return;
	}
	RUNTIME_STACK_PUSH(context,prop);
	++(context->exec_pos);
}
void lightspark::abc_getPropertyStaticName_constant_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	multiname* name=instrptr->cachedmultiname2;

	asAtom obj= *instrptr->arg1_constant;
	LOG_CALL( "getProperty_scl " << *name << ' ' << asAtomHandler::toDebugString(obj));
	asAtom prop=asAtomHandler::invalidAtom;
	bool canCache=false;
	multiname* simplegetter = asAtomHandler::getVariableByMultiname(obj,prop,*name,context->worker,canCache,GET_VARIABLE_OPTION::NONE,instrptr->local3.pos);
	if (simplegetter)
		instrptr->cachedmultiname2 = simplegetter;
	if(checkPropertyException(obj,name,prop,context->worker))
		return;
	REPLACELOCALRESULT(context,instrptr->local3.pos,prop);
	++(context->exec_pos);
}
void lightspark::abc_getPropertyStaticName_local_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	multiname* name=instrptr->cachedmultiname2;

	if (name->name_type == multiname::NAME_INT
		&& asAtomHandler::is<Array>(CONTEXT_GETLOCAL(context,instrptr->local_pos1))
		&& name->name_i > 0
		&& (uint32_t)name->name_i < asAtomHandler::as<Array>(CONTEXT_GETLOCAL(context,instrptr->local_pos1))->getCurrentSize())
	{
		LOG_CALL( "getProperty_slli " << name->name_i << ' ' << asAtomHandler::toDebugString(CONTEXT_GETLOCAL(context,instrptr->local_pos1)));
		asAtom oldres = CONTEXT_GETLOCAL(context,instrptr->local3.pos);
		asAtomHandler::as<Array>(CONTEXT_GETLOCAL(context,instrptr->local_pos1))->at_nocheck(CONTEXT_GETLOCAL(context,instrptr->local3.pos),name->name_i);
		if (asAtomHandler::isUndefined(CONTEXT_GETLOCAL(context,instrptr->local3.pos))) // item not found, check prototype
			asAtomHandler::as<Array>(CONTEXT_GETLOCAL(context,instrptr->local_pos1))->getVariableByInteger(CONTEXT_GETLOCAL(context,instrptr->local3.pos),name->name_i,GET_VARIABLE_OPTION::NO_INCREF,context->worker);
		ASATOM_INCREF(CONTEXT_GETLOCAL(context,instrptr->local3.pos));
		ASATOM_DECREF(oldres);
	}
	else
	{
		LOG_CALL( "getProperty_sll " << *name << ' ' << asAtomHandler::toDebugString(CONTEXT_GETLOCAL(context,instrptr->local_pos1)));
		asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local_pos1);
		asAtom prop=asAtomHandler::invalidAtom;
		bool canCache=false;
		multiname* simplegetter = asAtomHandler::getVariableByMultiname(obj,prop,*name,context->worker,canCache,GET_VARIABLE_OPTION::NONE,instrptr->local3.pos);
		if (simplegetter)
			instrptr->cachedmultiname2 = simplegetter;
		LOG_CALL("getProperty_sll done " << *name << ' ' << asAtomHandler::toDebugString(obj)<<" "<<instrptr->local3.pos<<" "<<asAtomHandler::toDebugString(prop));
		if(checkPropertyException(obj,name,prop,context->worker))
			return;
		REPLACELOCALRESULT(context,instrptr->local3.pos,prop);
	}
	++(context->exec_pos);
}
void lightspark::abc_getPropertyStaticName_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	multiname* name=instrptr->cachedmultiname2;

	RUNTIME_STACK_POP_CREATE(context,obj);
	LOG_CALL( "getProperty_slr " << *name << ' ' << asAtomHandler::toDebugString(*obj)<<" "<<instrptr->local3.pos);
	asAtom prop=asAtomHandler::invalidAtom;
	bool canCache=false;
	multiname* simplegetter = asAtomHandler::getVariableByMultiname(*obj,prop,*name,context->worker,canCache,GET_VARIABLE_OPTION::NONE,instrptr->local3.pos);
	if (simplegetter)
		instrptr->cachedmultiname2 = simplegetter;
	if(checkPropertyException(*obj,name,prop,context->worker))
		return;
	REPLACELOCALRESULT(context,instrptr->local3.pos,prop);
	ASATOM_DECREF(*obj);
	++(context->exec_pos);
}
