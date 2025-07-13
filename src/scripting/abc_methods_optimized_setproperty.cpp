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
#include "scripting/abc_optimized_setproperty.h"


using namespace std;
using namespace lightspark;

void lightspark::abc_setPropertyStaticName(call_context* context)
{
	multiname* name=context->exec_pos->cachedmultiname2;
	RUNTIME_STACK_POP_CREATE(context,value);
	RUNTIME_STACK_POP_CREATE(context,obj);

	LOG_CALL("setProperty_s " << *name << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));
	if(asAtomHandler::isNull(*obj))
	{
		LOG(LOG_ERROR,"calling setProperty on null:" << *name << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));
		ASATOM_DECREF_POINTER(value);
		ASATOM_DECREF_POINTER(obj);
		createError<TypeError>(context->worker,kConvertNullToObjectError);
		return;
	}
	if (asAtomHandler::isUndefined(*obj))
	{
		LOG(LOG_ERROR,"calling setProperty on undefined:" << *name << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));
		ASATOM_DECREF_POINTER(value);
		ASATOM_DECREF_POINTER(obj);
		createError<TypeError>(context->worker,kConvertUndefinedToObjectError);
		return;
	}
	ASObject* o = asAtomHandler::toObject(*obj,context->worker);
	bool alreadyset=false;
	multiname* simplesettername = nullptr;
	if (context->exec_pos->local3.pos == 0x68)//initproperty
		simplesettername =o->setVariableByMultiname(*name,*value,CONST_ALLOWED,&alreadyset,context->worker);
	else//Do not allow to set contant traits
		simplesettername =o->setVariableByMultiname(*name,*value,CONST_NOT_ALLOWED,&alreadyset,context->worker);
	if (simplesettername)
		context->exec_pos->cachedmultiname2 = simplesettername;
	if (alreadyset || context->exceptionthrown)
		ASATOM_DECREF_POINTER(value);
	ASATOM_DECREF_POINTER(obj);
	++(context->exec_pos);
}
void lightspark::abc_setPropertyStaticName_constant_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));
	multiname* name=context->exec_pos->cachedmultiname2;
	asAtom* obj = instrptr->arg1_constant;
	asAtom* value = instrptr->arg2_constant;

	LOG_CALL("setProperty_scc " << *name << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));

	if(asAtomHandler::isNull(*obj))
	{
		LOG(LOG_ERROR,"calling setProperty on null:" << *name << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));
		createError<TypeError>(context->worker,kConvertNullToObjectError);
		return;
	}
	if (asAtomHandler::isUndefined(*obj))
	{
		LOG(LOG_ERROR,"calling setProperty on undefined:" << *name << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));
		createError<TypeError>(context->worker,kConvertUndefinedToObjectError);
		return;
	}

	ASObject* o = asAtomHandler::toObject(*obj,context->worker);
	multiname* simplesettername = nullptr;
	if (context->exec_pos->local3.pos == 0x68)//initproperty
		simplesettername =o->setVariableByMultiname(*name,*value,CONST_ALLOWED,nullptr,context->worker);
	else//Do not allow to set contant traits
		simplesettername =o->setVariableByMultiname(*name,*value,CONST_NOT_ALLOWED,nullptr,context->worker);
	if (simplesettername)
		context->exec_pos->cachedmultiname2 = simplesettername;
	++(context->exec_pos);
}
void lightspark::abc_setPropertyStaticName_local_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));
	multiname* name=context->exec_pos->cachedmultiname2;
	asAtom* obj = &CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	asAtom* value = instrptr->arg2_constant;

	LOG_CALL("setProperty_slc " << *name << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value)<<" "<<instrptr->local_pos1);

	if(asAtomHandler::isNull(*obj))
	{
		LOG(LOG_ERROR,"calling setProperty on null:" << *name << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));
		createError<TypeError>(context->worker,kConvertNullToObjectError);
		return;
	}
	if (asAtomHandler::isUndefined(*obj))
	{
		LOG(LOG_ERROR,"calling setProperty on undefined:" << *name << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));
		createError<TypeError>(context->worker,kConvertUndefinedToObjectError);
		return;
	}
	ASObject* o = asAtomHandler::toObject(*obj,context->worker);
	o->incRef(); // this is neccessary for reference counting in case of exception thrown in setVariableByMultiname
	multiname* simplesettername = nullptr;
	if (context->exec_pos->local3.pos == 0x68)//initproperty
		simplesettername =o->setVariableByMultiname(*name,*value,CONST_ALLOWED,nullptr,context->worker);
	else//Do not allow to set contant traits
		simplesettername =o->setVariableByMultiname(*name,*value,CONST_NOT_ALLOWED,nullptr,context->worker);
	if (simplesettername)
		context->exec_pos->cachedmultiname2 = simplesettername;
	o->decRef(); // this is neccessary for reference counting in case of exception thrown in setVariableByMultiname
	++(context->exec_pos);
}
void lightspark::abc_setPropertyStaticName_constant_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));
	multiname* name=context->exec_pos->cachedmultiname2;
	asAtom* obj = instrptr->arg1_constant;
	asAtom* value = &CONTEXT_GETLOCAL(context,instrptr->local_pos2);

	LOG_CALL("setProperty_scl " << *name << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));

	if(asAtomHandler::isNull(*obj))
	{
		LOG(LOG_ERROR,"calling setProperty on null:" << *name << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));
		createError<TypeError>(context->worker,kConvertNullToObjectError);
		return;
	}
	if (asAtomHandler::isUndefined(*obj))
	{
		LOG(LOG_ERROR,"calling setProperty on undefined:" << *name << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));
		createError<TypeError>(context->worker,kConvertUndefinedToObjectError);
		return;
	}
	ASObject* o = asAtomHandler::toObject(*obj,context->worker);
	ASATOM_INCREF_POINTER(value);
	bool alreadyset=false;
	multiname* simplesettername = nullptr;
	if (context->exec_pos->local3.pos == 0x68)//initproperty
		simplesettername =o->setVariableByMultiname(*name,*value,CONST_ALLOWED,&alreadyset,context->worker);
	else//Do not allow to set contant traits
		simplesettername =o->setVariableByMultiname(*name,*value,CONST_NOT_ALLOWED,&alreadyset,context->worker);
	if (simplesettername)
		context->exec_pos->cachedmultiname2 = simplesettername;
	if (alreadyset || context->exceptionthrown)
		ASATOM_DECREF_POINTER(value);
	++(context->exec_pos);
}
void lightspark::abc_setPropertyStaticName_local_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));
	multiname* name=context->exec_pos->cachedmultiname2;
	asAtom* obj = &CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	asAtom* value = &CONTEXT_GETLOCAL(context,instrptr->local_pos2);

	LOG_CALL("setProperty_sll " << *name << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));

	if(asAtomHandler::isNull(*obj))
	{
		LOG(LOG_ERROR,"calling setProperty on null:" << *name << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));
		createError<TypeError>(context->worker,kConvertNullToObjectError);
		return;
	}
	if (asAtomHandler::isUndefined(*obj))
	{
		LOG(LOG_ERROR,"calling setProperty on undefined:" << *name << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));
		createError<TypeError>(context->worker,kConvertUndefinedToObjectError);
		return;
	}
	ASObject* o = asAtomHandler::toObject(*obj,context->worker);
	o->incRef(); // this is neccessary for reference counting in case of exception thrown in setVariableByMultiname
	ASATOM_INCREF_POINTER(value);
	bool alreadyset=false;
	multiname* simplesettername = nullptr;
	if (context->exec_pos->local3.pos == 0x68)//initproperty
		simplesettername =o->setVariableByMultiname(*name,*value,CONST_ALLOWED,&alreadyset,context->worker);
	else//Do not allow to set contant traits
		simplesettername =o->setVariableByMultiname(*name,*value,CONST_NOT_ALLOWED,&alreadyset,context->worker);
	if (simplesettername)
		context->exec_pos->cachedmultiname2 = simplesettername;
	if (alreadyset || context->exceptionthrown)
		ASATOM_DECREF_POINTER(value);
	o->decRef(); // this is neccessary for reference counting in case of exception thrown in setVariableByMultiname
	++(context->exec_pos);
}
void lightspark::abc_setPropertyInteger(call_context* context)
{
	RUNTIME_STACK_POP_CREATE(context,value);
	RUNTIME_STACK_POP_CREATE(context,idx);
	int index = asAtomHandler::getInt(*idx);
	RUNTIME_STACK_POP_CREATE(context,obj);

	LOG_CALL("setPropertyInteger " << index << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));

	if(asAtomHandler::isNull(*obj))
	{
		LOG(LOG_ERROR,"calling setProperty on null:" << index << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));
		ASATOM_DECREF_POINTER(value);
		ASATOM_DECREF_POINTER(obj);
		createError<TypeError>(context->worker,kConvertNullToObjectError);
		return;
	}
	if (asAtomHandler::isUndefined(*obj))
	{
		LOG(LOG_ERROR,"calling setProperty on undefined:" << index << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));
		ASATOM_DECREF_POINTER(value);
		ASATOM_DECREF_POINTER(obj);
		createError<TypeError>(context->worker,kConvertUndefinedToObjectError);
		return;
	}

	ASObject* o = asAtomHandler::getObject(*obj);
	if (!o)
		o = asAtomHandler::toObject(*obj,context->worker);
	bool alreadyset=false;
	if (context->exec_pos->local3.pos == 0x68)//initproperty
		o->setVariableByInteger(index,*value,CONST_ALLOWED,&alreadyset,context->worker);
	else//Do not allow to set contant traits
		o->setVariableByInteger(index,*value,CONST_NOT_ALLOWED,&alreadyset,context->worker);
	if (alreadyset || context->exceptionthrown)
		ASATOM_DECREF_POINTER(value);
	ASATOM_DECREF_POINTER(obj);
	++(context->exec_pos);
}
void lightspark::abc_setPropertyInteger_constant_constant_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));
	asAtom* obj = instrptr->arg3_constant;
	int index = asAtomHandler::getInt(*instrptr->arg1_constant);
	asAtom* value = instrptr->arg2_constant;

	LOG_CALL("setProperty_i_ccc " << index << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));

	if(asAtomHandler::isNull(*obj))
	{
		LOG(LOG_ERROR,"calling setProperty on null:" << index << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));
		createError<TypeError>(context->worker,kConvertNullToObjectError);
		return;
	}
	if (asAtomHandler::isUndefined(*obj))
	{
		LOG(LOG_ERROR,"calling setProperty on undefined:" << index << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));
		createError<TypeError>(context->worker,kConvertUndefinedToObjectError);
		return;
	}

	ASObject* o = asAtomHandler::getObject(*obj);
	if (!o)
		o = asAtomHandler::toObject(*obj,context->worker);
	ASATOM_INCREF_POINTER(value);
	bool alreadyset=false;
	if (context->exec_pos->local3.pos == 0x68)//initproperty
		o->setVariableByInteger(index,*value,CONST_ALLOWED,&alreadyset,context->worker);
	else//Do not allow to set contant traits
		o->setVariableByInteger(index,*value,CONST_NOT_ALLOWED,&alreadyset,context->worker);
	if (alreadyset || context->exceptionthrown)
		ASATOM_DECREF_POINTER(value);
	++(context->exec_pos);
}
void lightspark::abc_setPropertyInteger_constant_local_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));
	asAtom* obj = instrptr->arg3_constant;
	int index = asAtomHandler::getInt(CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	asAtom* value = instrptr->arg2_constant;

	LOG_CALL("setProperty_i_clc " << index << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));

	if(asAtomHandler::isNull(*obj))
	{
		LOG(LOG_ERROR,"calling setProperty on null:" << index << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));
		createError<TypeError>(context->worker,kConvertNullToObjectError);
		return;
	}
	if (asAtomHandler::isUndefined(*obj))
	{
		LOG(LOG_ERROR,"calling setProperty on undefined:" << index << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));
		createError<TypeError>(context->worker,kConvertUndefinedToObjectError);
		return;
	}

	ASObject* o = asAtomHandler::getObject(*obj);
	if (!o)
		o = asAtomHandler::toObject(*obj,context->worker);
	ASATOM_INCREF_POINTER(value);
	bool alreadyset=false;
	if (context->exec_pos->local3.pos == 0x68)//initproperty
		o->setVariableByInteger(index,*value,CONST_ALLOWED,&alreadyset,context->worker);
	else//Do not allow to set contant traits
		o->setVariableByInteger(index,*value,CONST_NOT_ALLOWED,&alreadyset,context->worker);
	if (alreadyset || context->exceptionthrown)
		ASATOM_DECREF_POINTER(value);
	++(context->exec_pos);
}
void lightspark::abc_setPropertyInteger_constant_constant_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));
	asAtom* obj = instrptr->arg3_constant;
	int index = asAtomHandler::getInt(*instrptr->arg1_constant);
	asAtom* value = &CONTEXT_GETLOCAL(context,instrptr->local_pos2);

	LOG_CALL("setProperty_i_ccl " << index << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));

	if(asAtomHandler::isNull(*obj))
	{
		LOG(LOG_ERROR,"calling setProperty on null:" << index << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));
		createError<TypeError>(context->worker,kConvertNullToObjectError);
		return;
	}
	if (asAtomHandler::isUndefined(*obj))
	{
		LOG(LOG_ERROR,"calling setProperty on undefined:" << index << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));
		createError<TypeError>(context->worker,kConvertUndefinedToObjectError);
		return;
	}

	ASObject* o = asAtomHandler::getObject(*obj);
	if (!o)
		o = asAtomHandler::toObject(*obj,context->worker);
	ASATOM_INCREF_POINTER(value);
	bool alreadyset=false;
	if (context->exec_pos->local3.pos == 0x68)//initproperty
		o->setVariableByInteger(index,*value,CONST_ALLOWED,&alreadyset,context->worker);
	else//Do not allow to set contant traits
		o->setVariableByInteger(index,*value,CONST_NOT_ALLOWED,&alreadyset,context->worker);
	if (alreadyset || context->exceptionthrown)
		ASATOM_DECREF_POINTER(value);
	++(context->exec_pos);
}
void lightspark::abc_setPropertyInteger_constant_local_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));
	asAtom* obj = instrptr->arg3_constant;
	int index = asAtomHandler::getInt(CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	asAtom* value = &CONTEXT_GETLOCAL(context,instrptr->local_pos2);

	LOG_CALL("setProperty_i_cll " << index << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));

	if(asAtomHandler::isNull(*obj))
	{
		LOG(LOG_ERROR,"calling setProperty on null:" << index << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));
		createError<TypeError>(context->worker,kConvertNullToObjectError);
		return;
	}
	if (asAtomHandler::isUndefined(*obj))
	{
		LOG(LOG_ERROR,"calling setProperty on undefined:" << index << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));
		createError<TypeError>(context->worker,kConvertUndefinedToObjectError);
		return;
	}

	ASATOM_INCREF_POINTER(value);
	ASObject* o = asAtomHandler::getObject(*obj);
	if (!o)
		o = asAtomHandler::toObject(*obj,context->worker);
	bool alreadyset=false;
	if (context->exec_pos->local3.pos == 0x68)//initproperty
		o->setVariableByInteger(index,*value,CONST_ALLOWED,&alreadyset,context->worker);
	else//Do not allow to set contant traits
		o->setVariableByInteger(index,*value,CONST_NOT_ALLOWED,&alreadyset,context->worker);
	if (alreadyset || context->exceptionthrown)
		ASATOM_DECREF_POINTER(value);
	++(context->exec_pos);
}
void lightspark::abc_setPropertyInteger_local_constant_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));
	asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local3.pos);
	int index = asAtomHandler::getInt(*instrptr->arg1_constant);
	asAtom* value = instrptr->arg2_constant;

	LOG_CALL("setProperty_i_lcc " << index << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));

	if(asAtomHandler::isNull(obj))
	{
		LOG(LOG_ERROR,"calling setProperty on null:" << index << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));
		createError<TypeError>(context->worker,kConvertNullToObjectError);
		return;
	}
	if (asAtomHandler::isUndefined(obj))
	{
		LOG(LOG_ERROR,"calling setProperty on undefined:" << index << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));
		createError<TypeError>(context->worker,kConvertUndefinedToObjectError);
		return;
	}

	ASObject* o = asAtomHandler::getObject(obj);
	if (!o)
		o = asAtomHandler::toObject(obj,context->worker);
	ASATOM_INCREF_POINTER(value);
	bool alreadyset=false;
	if (context->exec_pos->local3.pos == 0x68)//initproperty
		o->setVariableByInteger(index,*value,CONST_ALLOWED,&alreadyset,context->worker);
	else//Do not allow to set contant traits
		o->setVariableByInteger(index,*value,CONST_NOT_ALLOWED,&alreadyset,context->worker);
	if (alreadyset || context->exceptionthrown)
		ASATOM_DECREF_POINTER(value);
	++(context->exec_pos);
}
void lightspark::abc_setPropertyInteger_local_local_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));
	asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local3.pos);
	int index = asAtomHandler::getInt(CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	asAtom* value = instrptr->arg2_constant;

	LOG_CALL("setProperty_i_llc " << index << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));

	if(asAtomHandler::isNull(obj))
	{
		LOG(LOG_ERROR,"calling setProperty on null:" << index << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));
		createError<TypeError>(context->worker,kConvertNullToObjectError);
		return;
	}
	if (asAtomHandler::isUndefined(obj))
	{
		LOG(LOG_ERROR,"calling setProperty on undefined:" << index << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));
		createError<TypeError>(context->worker,kConvertUndefinedToObjectError);
		return;
	}

	ASObject* o = asAtomHandler::getObject(obj);
	if (!o)
		o = asAtomHandler::toObject(obj,context->worker);
	ASATOM_INCREF_POINTER(value);
	bool alreadyset=false;
	if (context->exec_pos->local3.pos == 0x68)//initproperty
		o->setVariableByInteger(index,*value,CONST_ALLOWED,&alreadyset,context->worker);
	else//Do not allow to set contant traits
		o->setVariableByInteger(index,*value,CONST_NOT_ALLOWED,&alreadyset,context->worker);
	if (alreadyset || context->exceptionthrown)
		ASATOM_DECREF_POINTER(value);
	++(context->exec_pos);
}
void lightspark::abc_setPropertyInteger_local_constant_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));
	asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local3.pos);
	int index = asAtomHandler::getInt(*instrptr->arg1_constant);
	asAtom* value = &CONTEXT_GETLOCAL(context,instrptr->local_pos2);

	LOG_CALL("setProperty_i_lcl " << index << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));

	if(asAtomHandler::isNull(obj))
	{
		LOG(LOG_ERROR,"calling setProperty on null:" << index << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));
		createError<TypeError>(context->worker,kConvertNullToObjectError);
		return;
	}
	if (asAtomHandler::isUndefined(obj))
	{
		LOG(LOG_ERROR,"calling setProperty on undefined:" << index << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));
		createError<TypeError>(context->worker,kConvertUndefinedToObjectError);
		return;
	}

	ASATOM_INCREF_POINTER(value);
	ASObject* o = asAtomHandler::getObject(obj);
	if (!o)
		o = asAtomHandler::toObject(obj,context->worker);
	bool alreadyset=false;
	if (context->exec_pos->local3.pos == 0x68)//initproperty
		o->setVariableByInteger(index,*value,CONST_ALLOWED,&alreadyset,context->worker);
	else//Do not allow to set contant traits
		o->setVariableByInteger(index,*value,CONST_NOT_ALLOWED,&alreadyset,context->worker);
	if (alreadyset || context->exceptionthrown)
		ASATOM_DECREF_POINTER(value);
	++(context->exec_pos);
}
void lightspark::abc_setPropertyInteger_local_local_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));
	asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local3.pos);
	int index = asAtomHandler::getInt(CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	asAtom* value = &CONTEXT_GETLOCAL(context,instrptr->local_pos2);

	LOG_CALL("setProperty_i_lll " << index << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value)<<" "<<instrptr->local_pos1);

	if(asAtomHandler::isNull(obj))
	{
		LOG(LOG_ERROR,"calling setProperty on null:" << index << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));
		createError<TypeError>(context->worker,kConvertNullToObjectError);
		return;
	}
	if (asAtomHandler::isUndefined(obj))
	{
		LOG(LOG_ERROR,"calling setProperty on undefined:" << index << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));
		createError<TypeError>(context->worker,kConvertUndefinedToObjectError);
		return;
	}

	ASATOM_INCREF_POINTER(value);
	ASObject* o = asAtomHandler::getObject(obj);
	if (!o)
		o = asAtomHandler::toObject(obj,context->worker);
	bool alreadyset=false;
	if (context->exec_pos->local3.pos == 0x68)//initproperty
		o->setVariableByInteger(index,*value,CONST_ALLOWED,&alreadyset,context->worker);
	else//Do not allow to set contant traits
		o->setVariableByInteger(index,*value,CONST_NOT_ALLOWED,&alreadyset,context->worker);
	if (alreadyset || context->exceptionthrown)
		ASATOM_DECREF_POINTER(value);
	++(context->exec_pos);
}


void lightspark::abc_setPropertyIntegerVector_constant_constant_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));
	asAtom* obj = instrptr->arg3_constant;
	int index = asAtomHandler::getInt(*instrptr->arg1_constant);
	asAtom* value = instrptr->arg2_constant;

	LOG_CALL("setProperty_iv_ccc " << index << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));

	if(asAtomHandler::isNull(*obj))
	{
		LOG(LOG_ERROR,"calling setProperty on null:" << index << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));
		ASATOM_DECREF_POINTER(value);
		createError<TypeError>(context->worker,kConvertNullToObjectError);
		return;
	}
	if (asAtomHandler::isUndefined(*obj))
	{
		LOG(LOG_ERROR,"calling setProperty on undefined:" << index << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));
		ASATOM_DECREF_POINTER(value);
		createError<TypeError>(context->worker,kConvertUndefinedToObjectError);
		return;
	}

	ASATOM_INCREF_POINTER(value);
	Vector* o = asAtomHandler::as<Vector>(*obj);
	bool alreadyset=false;
	o->setVariableByIntegerNoCoerce(index,*value,&alreadyset,context->worker);
	if (alreadyset || context->exceptionthrown)
		ASATOM_DECREF_POINTER(value);
	++(context->exec_pos);
}
void lightspark::abc_setPropertyIntegerVector_constant_local_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));
	asAtom* obj = instrptr->arg3_constant;
	int index = asAtomHandler::getInt(CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	asAtom* value = instrptr->arg2_constant;

	LOG_CALL("setProperty_iv_clc " << index << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));

	if(asAtomHandler::isNull(*obj))
	{
		LOG(LOG_ERROR,"calling setProperty on null:" << index << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));
		ASATOM_DECREF_POINTER(value);
		createError<TypeError>(context->worker,kConvertNullToObjectError);
		return;
	}
	if (asAtomHandler::isUndefined(*obj))
	{
		LOG(LOG_ERROR,"calling setProperty on undefined:" << index << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));
		ASATOM_DECREF_POINTER(value);
		createError<TypeError>(context->worker,kConvertUndefinedToObjectError);
		return;
	}

	ASATOM_INCREF_POINTER(value);
	Vector* o = asAtomHandler::as<Vector>(*obj);
	bool alreadyset=false;
	o->setVariableByIntegerNoCoerce(index,*value,&alreadyset,context->worker);
	if (alreadyset || context->exceptionthrown)
		ASATOM_DECREF_POINTER(value);
	++(context->exec_pos);
}
void lightspark::abc_setPropertyIntegerVector_constant_constant_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));
	asAtom* obj = instrptr->arg3_constant;
	int index = asAtomHandler::getInt(*instrptr->arg1_constant);
	asAtom* value = &CONTEXT_GETLOCAL(context,instrptr->local_pos2);

	LOG_CALL("setProperty_iv_ccl " << index << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));

	if(asAtomHandler::isNull(*obj))
	{
		LOG(LOG_ERROR,"calling setProperty on null:" << index << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));
		ASATOM_DECREF_POINTER(value);
		createError<TypeError>(context->worker,kConvertNullToObjectError);
		return;
	}
	if (asAtomHandler::isUndefined(*obj))
	{
		LOG(LOG_ERROR,"calling setProperty on undefined:" << index << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));
		ASATOM_DECREF_POINTER(value);
		createError<TypeError>(context->worker,kConvertUndefinedToObjectError);
		return;
	}

	ASATOM_INCREF_POINTER(value);
	Vector* o = asAtomHandler::as<Vector>(*obj);
	bool alreadyset=false;
	o->setVariableByIntegerNoCoerce(index,*value,&alreadyset,context->worker);
	if (alreadyset || context->exceptionthrown)
		ASATOM_DECREF_POINTER(value);
	++(context->exec_pos);
}
void lightspark::abc_setPropertyIntegerVector_constant_local_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));
	asAtom* obj = instrptr->arg3_constant;
	int index = asAtomHandler::getInt(CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	asAtom* value = &CONTEXT_GETLOCAL(context,instrptr->local_pos2);

	LOG_CALL("setProperty_iv_cll " << index << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));

	if(asAtomHandler::isNull(*obj))
	{
		LOG(LOG_ERROR,"calling setProperty on null:" << index << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));
		createError<TypeError>(context->worker,kConvertNullToObjectError);
		return;
	}
	if (asAtomHandler::isUndefined(*obj))
	{
		LOG(LOG_ERROR,"calling setProperty on undefined:" << index << ' ' << asAtomHandler::toDebugString(*obj)<<" " <<asAtomHandler::toDebugString(*value));
		createError<TypeError>(context->worker,kConvertUndefinedToObjectError);
		return;
	}

	ASATOM_INCREF_POINTER(value);
	Vector* o = asAtomHandler::as<Vector>(*obj);
	bool alreadyset=false;
	o->setVariableByIntegerNoCoerce(index,*value,&alreadyset,context->worker);
	if (alreadyset || context->exceptionthrown)
		ASATOM_DECREF_POINTER(value);
	++(context->exec_pos);
}
void lightspark::abc_setPropertyIntegerVector_local_constant_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));
	asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local3.pos);
	int index = asAtomHandler::getInt(*instrptr->arg1_constant);
	asAtom* value = instrptr->arg2_constant;

	LOG_CALL("setProperty_iv_lcc " << index << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));

	if(asAtomHandler::isNull(obj))
	{
		LOG(LOG_ERROR,"calling setProperty on null:" << index << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));
		ASATOM_DECREF_POINTER(value);
		createError<TypeError>(context->worker,kConvertNullToObjectError);
		return;
	}
	if (asAtomHandler::isUndefined(obj))
	{
		LOG(LOG_ERROR,"calling setProperty on undefined:" << index << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));
		ASATOM_DECREF_POINTER(value);
		createError<TypeError>(context->worker,kConvertUndefinedToObjectError);
		return;
	}

	ASATOM_INCREF_POINTER(value);
	Vector* o = asAtomHandler::as<Vector>(obj);
	bool alreadyset=false;
	o->setVariableByIntegerNoCoerce(index,*value,&alreadyset,context->worker);
	if (alreadyset || context->exceptionthrown)
		ASATOM_DECREF_POINTER(value);
	++(context->exec_pos);
}
void lightspark::abc_setPropertyIntegerVector_local_local_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));
	asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local3.pos);
	int index = asAtomHandler::getInt(CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	asAtom* value = instrptr->arg2_constant;

	LOG_CALL("setProperty_iv_llc " << index << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));

	if(asAtomHandler::isNull(obj))
	{
		LOG(LOG_ERROR,"calling setProperty on null:" << index << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));
		ASATOM_DECREF_POINTER(value);
		createError<TypeError>(context->worker,kConvertNullToObjectError);
		return;
	}
	if (asAtomHandler::isUndefined(obj))
	{
		LOG(LOG_ERROR,"calling setProperty on undefined:" << index << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));
		ASATOM_DECREF_POINTER(value);
		createError<TypeError>(context->worker,kConvertUndefinedToObjectError);
		return;
	}

	ASATOM_INCREF_POINTER(value);
	Vector* o = asAtomHandler::as<Vector>(obj);
	bool alreadyset=false;
	o->setVariableByIntegerNoCoerce(index,*value,&alreadyset,context->worker);
	if (alreadyset || context->exceptionthrown)
		ASATOM_DECREF_POINTER(value);
	++(context->exec_pos);
}
void lightspark::abc_setPropertyIntegerVector_local_constant_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));
	asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local3.pos);
	int index = asAtomHandler::getInt(*instrptr->arg1_constant);
	asAtom* value = &CONTEXT_GETLOCAL(context,instrptr->local_pos2);

	LOG_CALL("setProperty_iv_lcl " << index << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));

	if(asAtomHandler::isNull(obj))
	{
		LOG(LOG_ERROR,"calling setProperty on null:" << index << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));
		createError<TypeError>(context->worker,kConvertNullToObjectError);
		return;
	}
	if (asAtomHandler::isUndefined(obj))
	{
		LOG(LOG_ERROR,"calling setProperty on undefined:" << index << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));
		createError<TypeError>(context->worker,kConvertUndefinedToObjectError);
		return;
	}

	ASATOM_INCREF_POINTER(value);
	Vector* o = asAtomHandler::as<Vector>(obj);
	bool alreadyset=false;
	o->setVariableByIntegerNoCoerce(index,*value,&alreadyset,context->worker);
	if (alreadyset || context->exceptionthrown)
		ASATOM_DECREF_POINTER(value);
	++(context->exec_pos);
}
void lightspark::abc_setPropertyIntegerVector_local_local_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));
	asAtom obj = CONTEXT_GETLOCAL(context,instrptr->local3.pos);
	int index = asAtomHandler::getInt(CONTEXT_GETLOCAL(context,instrptr->local_pos1));
	asAtom* value = &CONTEXT_GETLOCAL(context,instrptr->local_pos2);

	LOG_CALL("setProperty_iv_lll " << index << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value)<<" "<<instrptr->local_pos1);

	if(asAtomHandler::isNull(obj))
	{
		LOG(LOG_ERROR,"calling setProperty on null:" << index << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));
		createError<TypeError>(context->worker,kConvertNullToObjectError);
		return;
	}
	if (asAtomHandler::isUndefined(obj))
	{
		LOG(LOG_ERROR,"calling setProperty on undefined:" << index << ' ' << asAtomHandler::toDebugString(obj)<<" " <<asAtomHandler::toDebugString(*value));
		createError<TypeError>(context->worker,kConvertUndefinedToObjectError);
		return;
	}

	ASATOM_INCREF_POINTER(value);
	Vector* o = asAtomHandler::as<Vector>(obj);
	bool alreadyset=false;
	o->setVariableByIntegerNoCoerce(index,*value,&alreadyset,context->worker);
	if (alreadyset || context->exceptionthrown)
		ASATOM_DECREF_POINTER(value);
	++(context->exec_pos);
}
