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

#include <list>
#include <algorithm>
#include <cstring>
#include <sstream>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <iomanip>
#define _USE_MATH_DEFINES
#include <cmath>
#include <limits>
#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <cerrno>

#include <glib.h>

#include "scripting/abc.h"
#include "scripting/toplevel/toplevel.h"
#include "scripting/flash/events/flashevents.h"
#include "swf.h"
#include "compat.h"
#include "scripting/class.h"
#include "exceptions.h"
#include "backends/urlutils.h"
#include "parsing/amf3_generator.h"
#include "scripting/argconv.h"
#include "scripting/toplevel/Number.h"
#include "scripting/toplevel/XML.h"

using namespace std;
using namespace lightspark;

Any* const Type::anyType = new Any();
Void* const Type::voidType = new Void();

Null::Null():ASObject((Class_base*)(NULL))
{
	type=T_NULL;
}

Undefined::Undefined():ASObject((Class_base*)(NULL))
{
	type=T_UNDEFINED;
}

ASFUNCTIONBODY(Undefined,call)
{
	LOG(LOG_CALLS,_("Undefined function"));
	return NULL;
}

TRISTATE Undefined::isLess(ASObject* r)
{
	//ECMA-262 complaiant
	//As undefined became NaN when converted to number the operation is undefined
	return TUNDEFINED;
}

bool Undefined::isEqual(ASObject* r)
{
	switch(r->getObjectType())
	{
		case T_UNDEFINED:
		case T_NULL:
			return true;
		case T_NUMBER:
		case T_INTEGER:
		case T_UINTEGER:
		case T_STRING:
		case T_BOOLEAN:
			return false;
		default:
			return r->isEqual(this);
	}
}

int Undefined::toInt()
{
	return 0;
}

ASObject *Undefined::describeType() const
{
	return getSys()->getUndefinedRef();
}

void Undefined::serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap)
{
	out->writeByte(undefined_marker);
}

void Undefined::setVariableByMultiname(const multiname& name, ASObject* o, CONST_ALLOWED_FLAG allowConst)
{
	LOG(LOG_NOT_IMPLEMENTED, "Ignoring set on Undefined " << name);
	o->decRef();
}

IFunction::IFunction(Class_base* c):ASObject(c),length(0),inClass(NULL)
{
	type=T_FUNCTION;
}

void IFunction::finalize()
{
	ASObject::finalize();
	closure_this.reset();
}

ASFUNCTIONBODY_GETTER_SETTER(IFunction,prototype);
ASFUNCTIONBODY_GETTER(IFunction,length);

ASFUNCTIONBODY(IFunction,apply)
{
	/* This function never changes the 'this' pointer of a method closure */
	IFunction* th=static_cast<IFunction*>(obj);
	assert_and_throw(argslen<=2);

	ASObject* newObj=NULL;
	ASObject** newArgs=NULL;
	int newArgsLen=0;
	//Validate parameters
	if(argslen==0 || args[0]->is<Null>() || args[0]->is<Undefined>())
	{
		//get the current global object
		newObj=getVm()->currentCallContext->scope_stack[0].object->as<Global>();
		newObj->incRef();
	}
	else
	{
		newObj=args[0];
		newObj->incRef();
	}
	if(argslen == 2 && args[1]->getObjectType()==T_ARRAY)
	{
		Array* array=Class<Array>::cast(args[1]);
		newArgsLen=array->size();
		newArgs=g_newa(ASObject*, newArgsLen);
		for(int i=0;i<newArgsLen;i++)
		{
			_R<ASObject> val = array->at(i);
			val->incRef();
			newArgs[i]=val.getPtr();
		}
	}

	ASObject* ret=th->call(newObj,newArgs,newArgsLen);
	return ret;
}

ASFUNCTIONBODY(IFunction,_call)
{
	/* This function never changes the 'this' pointer of a method closure */
	IFunction* th=static_cast<IFunction*>(obj);
	ASObject* newObj=NULL;
	ASObject** newArgs=NULL;
	uint32_t newArgsLen=0;
	if(argslen==0 || args[0]->is<Null>() || args[0]->is<Undefined>())
	{
		//get the current global object
		newObj=getVm()->currentCallContext->scope_stack[0].object->as<Global>();
		newObj->incRef();
	}
	else
	{
		newObj=args[0];
		newObj->incRef();
	}
	if(argslen > 1)
	{
		newArgsLen=argslen-1;
		newArgs=g_newa(ASObject*, newArgsLen);
		for(unsigned int i=0;i<newArgsLen;i++)
		{
			newArgs[i]=args[i+1];
			newArgs[i]->incRef();
		}
	}
	ASObject* ret=th->call(newObj,newArgs,newArgsLen);
	return ret;
}

ASFUNCTIONBODY(IFunction,_toString)
{
	return Class<ASString>::getInstanceS("function Function() {}");
}

ASObject *IFunction::describeType() const
{
	xmlpp::DomParser p;
	xmlpp::Element* root=p.get_document()->create_root_node("type");

	root->set_attribute("name", "Function");
	root->set_attribute("base", "Object");
	root->set_attribute("isDynamic", "true");
	root->set_attribute("isFinal", "false");
	root->set_attribute("isStatic", "false");

	xmlpp::Element* node=root->add_child("extendsClass");
	node->set_attribute("type", "Object");

	// TODO: accessor
	LOG(LOG_NOT_IMPLEMENTED, "describeType for Function not completely implemented");

	return Class<XML>::getInstanceS(root);
}

SyntheticFunction::SyntheticFunction(Class_base* c,method_info* m):IFunction(c),mi(m),val(NULL)
{
	if(mi)
		length = mi->numArgs();
}

SyntheticFunction::~SyntheticFunction()
{
	finalize();
}

void SyntheticFunction::finalize()
{
	IFunction::finalize();
	func_scope.clear();
}

/**
 * This prepares a new call_context and then executes the ABC bytecode function
 * by ABCVm::executeFunction() or through JIT.
 * It consumes one reference of obj and one of each arg
 */
ASObject* SyntheticFunction::call(ASObject* obj, ASObject* const* args, uint32_t numArgs)
{
	const uint32_t opt_hit_threshold=1;
	const uint32_t jit_hit_threshold=20;
	assert_and_throw(mi->body);
	const uint16_t hit_count = mi->body->hit_count;
	const method_body_info::CODE_STATUS& codeStatus = mi->body->codeStatus;

	uint32_t& cur_recursion = getVm()->cur_recursion;
	if(cur_recursion == getVm()->limits.max_recursion)
	{
		for(uint32_t i=0;i<numArgs;i++)
			args[i]->decRef();
		obj->decRef();
		throw Class<ASError>::getInstanceS("Error #1023: Stack overflow occurred");
	}

	/* resolve argument and return types */
	if(!mi->returnType)
	{
		mi->hasExplicitTypes = false;
		mi->paramTypes.reserve(mi->numArgs());
		for(size_t i=0;i < mi->numArgs();++i)
		{
			const Type* t = Type::getTypeFromMultiname(mi->paramTypeName(i), mi->context);
			mi->paramTypes.push_back(t);
			if(t != Type::anyType)
				mi->hasExplicitTypes = true;
		}

		const Type* t = Type::getTypeFromMultiname(mi->returnTypeName(), mi->context);
		mi->returnType = t;
	}

	if(numArgs < mi->numArgs()-mi->numOptions())
	{
		/* Not enough arguments provided.
		 * We throw if this is a method.
		 * We won't throw if all arguments are of 'Any' type.
		 * This is in accordance with the proprietary player. */
		if(isMethod() || mi->hasExplicitTypes)
			throw Class<ArgumentError>::getInstanceS("Error #1063: Not enough arguments provided");
	}

	//For sufficiently hot methods, optimize them to the internal bytecode
	if(hit_count>=opt_hit_threshold && codeStatus==method_body_info::ORIGINAL && getSys()->useFastInterpreter)
	{
		ABCVm::optimizeFunction(this);
	}

	//Temporarily disable JITting
	if(mi->body->exceptions.size()==0 && getSys()->useJit && ((hit_count>=jit_hit_threshold && codeStatus==method_body_info::OPTIMIZED) || getSys()->useInterpreter==false))
	{
		//We passed the hot function threshold, synt the function
		val=mi->synt_method();
		assert(val);
	}
	mi->body->hit_count++;

	//Prepare arguments
	uint32_t args_len=mi->numArgs();
	int passedToLocals=imin(numArgs,args_len);
	uint32_t passedToRest=(numArgs > args_len)?(numArgs-mi->numArgs()):0;

	/* setup argumentsArray if needed */
	Array* argumentsArray = NULL;
	if(mi->needsArgs())
	{
		//The arguments does not contain default values of optional parameters,
		//i.e. f(a,b=3) called as f(7) gives arguments = { 7 }
		argumentsArray=Class<Array>::getInstanceS();
		argumentsArray->resize(numArgs);
		for(uint32_t j=0;j<numArgs;j++)
		{
			args[j]->incRef();
			argumentsArray->set(j,_MR(args[j]));
		}
		//Add ourself as the callee property
		incRef();
		argumentsArray->setVariableByQName("callee","",this,DECLARED_TRAIT);
	}

	/* setup call_context */
	call_context cc;
	cc.inClass = inClass;
	cc.mi=mi;
	cc.locals_size=mi->body->local_count+1;
	ASObject** locals = g_newa(ASObject*, cc.locals_size);
	cc.locals=locals;
	memset(cc.locals,0,sizeof(ASObject*)*cc.locals_size);
	cc.max_stack = mi->body->max_stack;
	ASObject** stack = g_newa(ASObject*, cc.max_stack);
	cc.stack=stack;
	cc.stack_index=0;
	cc.context=mi->context;
	//cc.code= new istringstream(mi->body->code);
	cc.scope_stack=func_scope;
	cc.initialScopeStack=func_scope.size();
	cc.exec_pos=0;

	/* Set the current global object, each script in each DoABCTag has its own */
	call_context* saved_cc = getVm()->currentCallContext;
	getVm()->currentCallContext = &cc;

	if(isBound())
	{ /* closure_this can never been overriden */
		LOG(LOG_CALLS,_("Calling with closure ") << this);
		if(obj)
			obj->decRef();
		obj=closure_this.getPtr();
		obj->incRef();
	}

	assert_and_throw(obj);
	obj->incRef(); //this is free'd in ~call_context
	cc.locals[0]=obj;

	/* coerce arguments to expected types */
	for(int i=0;i<passedToLocals;++i)
	{
		cc.locals[i+1] = mi->paramTypes[i]->coerce(args[i]);
	}

	//Fill missing parameters until optional parameters begin
	//like fun(a,b,c,d=3,e=5) called as fun(1,2) becomes
	//locals = {this, 1, 2, Undefined, 3, 5}
	for(uint32_t i=passedToLocals;i<args_len;++i)
	{
		int iOptional = mi->numOptions()-args_len+i;
		if(iOptional >= 0)
			cc.locals[i+1]=mi->paramTypes[i]->coerce(mi->getOptional(iOptional));
		else {
			assert(mi->paramTypes[i] == Type::anyType);
			cc.locals[i+1]=getSys()->getUndefinedRef();
		}
	}

	assert_and_throw(mi->needsArgs()==false || mi->needsRest()==false);
	if(mi->needsRest()) //TODO
	{
		assert_and_throw(argumentsArray==NULL);
		Array* rest=Class<Array>::getInstanceS();
		rest->resize(passedToRest);
		//Give the reference of the other args to an array
		for(uint32_t j=0;j<passedToRest;j++)
			rest->set(j,_MR(args[passedToLocals+j]));

		assert_and_throw(cc.locals_size>args_len+1);
		cc.locals[args_len+1]=rest;
	}
	else if(mi->needsArgs())
	{
		assert_and_throw(cc.locals_size>args_len+1);
		cc.locals[args_len+1]=argumentsArray;
	}
	//Parameters are ready

	ASObject* ret;
	//obtain a local reference to this function, as it may delete itself
	this->incRef();

	cur_recursion++; //increment current recursion depth
	Log::calls_indent++;
	while (true)
	{
		try
		{
			if(mi->body->exceptions.size() || (val==NULL && getSys()->useInterpreter))
			{
				if(codeStatus == method_body_info::OPTIMIZED && getSys()->useFastInterpreter)
				{
					//This is a mildy hot function, execute it using the fast interpreter
					ret=ABCVm::executeFunctionFast(this,&cc);
				}
				else
				{
					//Switch the codeStatus to USED to make sure the method will not be optimized while being used
					const method_body_info::CODE_STATUS oldCodeStatus = codeStatus;
					mi->body->codeStatus = method_body_info::USED;
					//This is not a hot function, execute it using the interpreter
					ret=ABCVm::executeFunction(this,&cc);
					//Restore the previous codeStatus
					mi->body->codeStatus = oldCodeStatus;
				}
			}
			else
				ret=val(&cc);
		}
		catch (ASObject* excobj) // Doesn't have to be an ASError at all.
		{
			unsigned int pos = cc.exec_pos;
			bool no_handler = true;

			LOG(LOG_TRACE, "got an " << excobj->toString());
			LOG(LOG_TRACE, "pos=" << pos);
			for (unsigned int i=0;i<mi->body->exceptions.size();i++)
			{
				exception_info exc=mi->body->exceptions[i];
				multiname* name=mi->context->getMultiname(exc.exc_type, NULL);
				LOG(LOG_TRACE, "f=" << exc.from << " t=" << exc.to << " type=" << *name);
				if (pos >= exc.from && pos <= exc.to && mi->context->isinstance(excobj, name))
				{
					no_handler = false;
					cc.exec_pos = exc.target;
					cc.runtime_stack_clear();
					cc.runtime_stack_push(excobj);
					cc.scope_stack.clear();
					cc.scope_stack=func_scope;
					cc.initialScopeStack=func_scope.size();
					break;
				}
			}
			if (no_handler)
			{
				cur_recursion--; //decrement current recursion depth
				Log::calls_indent--;
				getVm()->currentCallContext = saved_cc;
				throw;
			}
			continue;
		}
		break;
	}
	cur_recursion--; //decrement current recursion depth
	Log::calls_indent--;
	getVm()->currentCallContext = saved_cc;

	this->decRef(); //free local ref
	obj->decRef();

	if(ret==NULL)
		ret=getSys()->getUndefinedRef();

	return mi->returnType->coerce(ret);
}

/**
 * This executes a C++ function.
 * It consumes one reference of obj and one of each arg
 */
ASObject* Function::call(ASObject* obj, ASObject* const* args, uint32_t num_args)
{
	/*
	 * We do not enforce ABCVm::limits.max_recursion here.
	 * This should be okey, because there is no infinite recursion
	 * using only builtin functions.
	 * Additionally, we still need to run builtin code (such as the ASError constructor) when
	 * ABCVm::limits.max_recursion is reached in SyntheticFunction::call.
	 */
	ASObject* ret;
	if(isBound())
	{ /* closure_this can never been overriden */
		LOG(LOG_CALLS,_("Calling with closure ") << this);
		if(obj)
			obj->decRef();
		obj=closure_this.getPtr();
		obj->incRef();
	}
	assert_and_throw(obj);
	try
	{
		ret=val(obj,args,num_args);
	}
	catch(ASObject* excobj)
	{
		//If an exception is thrown from native code, clean up and rethrow
		for(uint32_t i=0;i<num_args;i++)
			args[i]->decRef();
		obj->decRef();
		throw;
	}

	for(uint32_t i=0;i<num_args;i++)
		args[i]->decRef();
	obj->decRef();
	if(ret==NULL)
		ret=getSys()->getUndefinedRef();
	return ret;
}

bool Null::isEqual(ASObject* r)
{
	switch(r->getObjectType())
	{
		case T_NULL:
		case T_UNDEFINED:
			return true;
		case T_INTEGER:
		case T_UINTEGER:
		case T_NUMBER:
		case T_STRING:
		case T_BOOLEAN:
			return false;
		default:
			return r->isEqual(this);
	}
}

TRISTATE Null::isLess(ASObject* r)
{
	if(r->getObjectType()==T_INTEGER)
	{
		Integer* i=static_cast<Integer*>(r);
		return (0<i->toInt())?TTRUE:TFALSE;
	}
	else if(r->getObjectType()==T_UINTEGER)
	{
		UInteger* i=static_cast<UInteger*>(r);
		return (0<i->toUInt())?TTRUE:TFALSE;
	}
	else if(r->getObjectType()==T_NUMBER)
	{
		Number* i=static_cast<Number*>(r);
		if(std::isnan(i->toNumber())) return TUNDEFINED;
		return (0<i->toNumber())?TTRUE:TFALSE;
	}
	else if(r->getObjectType()==T_BOOLEAN)
	{
		Boolean* i=static_cast<Boolean*>(r);
		return (0<i->val)?TTRUE:TFALSE;
	}
	else if(r->getObjectType()==T_NULL)
	{
		return TFALSE;
	}
	else if(r->getObjectType()==T_UNDEFINED)
	{
		return TUNDEFINED;
	}
	else if(r->getObjectType()==T_STRING)
	{
		double val2=r->toNumber();
		if(std::isnan(val2)) return TUNDEFINED;
		return (0<val2)?TTRUE:TFALSE;
	}
	else
	{
		double val2=r->toPrimitive()->toNumber();
		if(std::isnan(val2)) return TUNDEFINED;
		return (0<val2)?TTRUE:TFALSE;
	}
}

int32_t Null::getVariableByMultiname_i(const multiname& name)
{
	throw Class<TypeError>::getInstanceS("Error #1009: null has no properties.");
}

_NR<ASObject> Null::getVariableByMultiname(const multiname& name, GET_VARIABLE_OPTION opt)
{
	throw Class<TypeError>::getInstanceS("Error #1009: null has no properties.");
}

int Null::toInt()
{
	return 0;
}

void Null::serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap)
{
	out->writeByte(null_marker);
}

void Null::setVariableByMultiname(const multiname& name, ASObject* o, CONST_ALLOWED_FLAG allowConst)
{
	o->decRef();
	throw Class<TypeError>::getInstanceS("Cannot set on null");
}

ASObject* Void::coerce(ASObject* o) const
{
	if(!o->is<Undefined>())
		throw Class<TypeError>::getInstanceS("Trying to coerce o!=undefined to void");
	return o;
}

Type* Type::getBuiltinType(const multiname* mn)
{
	assert_and_throw(mn->isQName());
	assert(mn->name_type == multiname::NAME_STRING);
	if(mn == 0)
		return Type::anyType; //any
	if(mn->name_type == multiname::NAME_STRING && mn->name_s_id==BUILTIN_STRINGS::ANY
		&& mn->ns[0].hasEmptyName())
		return Type::anyType;
	if(mn->name_type == multiname::NAME_STRING && mn->name_s_id==BUILTIN_STRINGS::VOID
		&& mn->ns[0].hasEmptyName())
		return Type::voidType;

	//Check if the class has already been defined
	ASObject* target;
	ASObject* tmp=getSys()->systemDomain->getVariableAndTargetByMultiname(*mn, target);
	if(tmp && tmp->getObjectType()==T_CLASS)
		return static_cast<Class_base*>(tmp);
	else
		return NULL;
}

/*
 * This should only be called after all global objects have been created
 * by running ABCContext::exec() for all ABCContexts.
 * Therefore, all classes are at least declared.
 */
const Type* Type::getTypeFromMultiname(const multiname* mn, const ABCContext* context)
{
	if(mn == 0) //multiname idx zero indicates any type
		return Type::anyType;

	if(mn->name_type == multiname::NAME_STRING && mn->name_s_id==BUILTIN_STRINGS::ANY
		&& mn->ns.size() == 1 && mn->ns[0].hasEmptyName())
		return Type::anyType;

	if(mn->name_type == multiname::NAME_STRING && mn->name_s_id==BUILTIN_STRINGS::VOID
		&& mn->ns.size() == 1 && mn->ns[0].hasEmptyName())
		return Type::voidType;

	ASObject* typeObject;
	/*
	 * During the newClass opcode, the class is added to context->classesBeingDefined.
	 * The class variable in the global scope is only set a bit later.
	 * When the class has to be resolved in between (for example, the
	 * class has traits of the class's type), then we'll find it in
	 * classesBeingDefined, but context->root->getVariableAndTargetByMultiname()
	 * would still return "Undefined".
	 */
	auto i = context->classesBeingDefined.find(mn);
	if(i != context->classesBeingDefined.end())
		typeObject = i->second;
	else
	{
		ASObject* target;
		typeObject=context->root->applicationDomain->getVariableAndTargetByMultiname(*mn,target);
	}

	if(!typeObject)
	{
		//HACK: until we have implemented all flash classes, we need this hack
		LOG(LOG_NOT_IMPLEMENTED,"getTypeFromMultiname: could not find " << *mn << ", using AnyType");
		return Type::anyType;
	}

	if(!typeObject->is<Type>())
	{
		//It actually happens in the wild that the class for a member might be a private class
		//that is defined later in the same script. We have no way to solve the dependency, so return any
		LOG(LOG_NOT_IMPLEMENTED,"Not resolvable type " << *mn << ", using AnyType");
		return Type::anyType;
	}
	return typeObject->as<Type>();
}

Class_base::Class_base(const QName& name, MemoryAccount* m):ASObject(Class_object::getClass()),protected_ns("",NAMESPACE),constructor(NULL),
	borrowedVariables(m),
	context(NULL),class_name(name),memoryAccount(m),length(1),class_index(-1),isFinal(false),isSealed(false),use_protected(false)
{
	type=T_CLASS;
}

Class_base::Class_base(const Class_object*):ASObject((MemoryAccount*)NULL),protected_ns("",NAMESPACE),constructor(NULL),
	borrowedVariables(NULL),
	context(NULL),class_name("Class",""),memoryAccount(NULL),length(1),class_index(-1),isFinal(false),isSealed(false),use_protected(false)
{
	type=T_CLASS;
	//We have tested that (Class is Class == true) so the classdef is 'this'
	setClass(this);
	//The super is Class<ASObject> but we set it in SystemState constructor to avoid an infinite loop
}

/*
 * This copies the non-static traits of the super class to this
 * class.
 *
 * If a property is in the protected namespace of the super class, a copy is
 * created with the protected namespace of this class.
 * That is necessary, because superclass methods are called with the protected ns
 * of the current class.
 *
 * use_protns and protectedns must be set before this function is called
 */
void Class_base::copyBorrowedTraitsFromSuper()
{
	assert(borrowedVariables.Variables.empty());
	variables_map::var_iterator i = super->borrowedVariables.Variables.begin();
	for(;i != super->borrowedVariables.Variables.end(); ++i)
	{
		const varName& name = i->first;
		variable& v = i->second;
		if(v.var)
			v.var->incRef();
		if(v.getter)
			v.getter->incRef();
		if(v.setter)
			v.setter->incRef();

		borrowedVariables.Variables.insert(make_pair(name,v));
	}
}


ASObject* Class_base::coerce(ASObject* o) const
{
	if(o->is<Null>() || o->is<Undefined>())
		return o;
	if(o->is<Class_base>())
	{ /* classes can be cast to the type 'Object' or 'Class' */
	       if(this == Class<ASObject>::getClass()
		|| (class_name.name=="Class" && class_name.ns==""))
		       return o; /* 'this' is the type of a class */
	       else
		       throw Class<TypeError>::getInstanceS("Error #1034: Wrong type");
	}
	//o->getClass() == NULL for primitive types
	//those are handled in overloads Class<Number>::coerce etc.
	if(!o->getClass() || !o->getClass()->isSubClass(this))
		throw Class<TypeError>::getInstanceS("Error #1034: Wrong type");
	return o;
}

ASFUNCTIONBODY(Class_base,_toString)
{
	Class_base* th = obj->as<Class_base>();
	tiny_string ret;
	ret = "[class ";
	ret += th->class_name.name;
	ret += "]";
	return Class<ASString>::getInstanceS(ret);
}

void Class_base::addPrototypeGetter()
{
	setDeclaredMethodByQName("prototype","",Class<IFunction>::getFunction(_getter_prototype),GETTER_METHOD,false);
}

void Class_base::addLengthGetter()
{
	setDeclaredMethodByQName("length","",Class<IFunction>::getFunction(_getter_length),GETTER_METHOD,false);
}

Class_base::~Class_base()
{
	if(!referencedObjects.empty())
		LOG(LOG_ERROR,_("Class destroyed without cleanUp called"));
}

ASObject* Class_base::_getter_prototype(ASObject* obj, ASObject* const* args, const unsigned int argslen)
{
	if(!obj->is<Class_base>())
		throw Class<ArgumentError>::getInstanceS("Function applied to wrong object");
	Class_base* th = obj->as<Class_base>();
	if(argslen != 0)
		throw Class<ArgumentError>::getInstanceS("Arguments provided in getter");
	ASObject* ret=th->prototype->getObj();
	ret->incRef();
	return ret;
}
ASFUNCTIONBODY_GETTER(Class_base, length);

ASObject* Class_base::generator(ASObject* const* args, const unsigned int argslen)
{
	ASObject *ret=ASObject::generator(NULL, args, argslen);
	for(unsigned int i=0;i<argslen;i++)
		args[i]->decRef();
	return ret;
}

void Class_base::addImplementedInterface(const multiname& i)
{
	interfaces.push_back(i);
}

void Class_base::addImplementedInterface(Class_base* i)
{
	interfaces_added.push_back(i);
}

tiny_string Class_base::toString()
{
	tiny_string ret="[Class ";
	ret+=class_name.name;
	ret+="]";
	return ret;
}

void Class_base::recursiveBuild(ASObject* target)
{
	if(super)
		super->recursiveBuild(target);

	buildInstanceTraits(target);
}

void Class_base::setConstructor(IFunction* c)
{
	assert_and_throw(constructor==NULL);
	constructor=c;
}

void Class_base::handleConstruction(ASObject* target, ASObject* const* args, unsigned int argslen, bool buildAndLink)
{
	if(buildAndLink)
	{
	#ifndef NDEBUG
		assert_and_throw(!target->initialized);
	#endif
		//HACK: suppress implementation handling of variables just now
		bool bak=target->implEnable;
		target->implEnable=false;
		recursiveBuild(target);
		//And restore it
		target->implEnable=bak;
	#ifndef NDEBUG
		target->initialized=true;
	#endif
		//Tell the object that the construction is complete
		target->constructionComplete();
	}

	//TODO: is there any valid case for not having a constructor?
	if(constructor)
	{
		target->incRef();
		ASObject* ret=constructor->call(target,args,argslen);
		assert_and_throw(ret->is<Undefined>());
		ret->decRef();
	}
	else
	{
		for(uint32_t i=0;i<argslen;i++)
			args[i]->decRef();
	}
}

void Class_base::acquireObject(ASObject* ob)
{
	Locker l(referencedObjectsMutex);
	assert_and_throw(!ob->is_linked());
	referencedObjects.push_back(*ob);
}

void Class_base::abandonObject(ASObject* ob)
{
	Locker l(referencedObjectsMutex);
	assert_and_throw(ob->is_linked());
#ifdef EXPENSIVE_DEBUG
	//Check that the object is really referenced by this class
	int count=0;
	for (auto it=referencedObjects.cbegin(); it!=referencedObjects.cend(); ++it)
	{
		if ((&*it) == ob)
			count++;
	}
	assert_and_throw(count==1);
#endif

	referencedObjects.erase(referencedObjects.iterator_to(*ob));
}

void Class_base::finalizeObjects()
{
	while(!referencedObjects.empty())
	{
		ASObject *tmp=&referencedObjects.front();
		tmp->incRef();
		//Finalizing the object does also release the classdef and call abandonObject
		tmp->finalize();
		tmp->decRef();
	}
}

void Class_base::finalize()
{
	finalizeObjects();

	ASObject::finalize();
	borrowedVariables.destroyContents();
	super.reset();
	prototype.reset();
	if(constructor)
	{
		constructor->decRef();
		constructor=NULL;
	}
}

Template_base::Template_base(QName name) : ASObject((Class_base*)(NULL)),template_name(name)
{
	type = T_TEMPLATE;
}

Class_object* Class_object::getClass()
{
	//We check if we are registered already
	//if not we register ourselves (see also Class<T>::getClass)
	//Class object position in the map is hardwired to 0
	uint32_t classId=0;
	Class_object* ret=NULL;
	Class_base** retAddr=&getSys()->builtinClasses[classId];
	if(*retAddr==NULL)
	{
		//Create the class
		ret=new (getSys()->unaccountedMemory) Class_object();
		ret->incRef();
		*retAddr=ret;
	}
	else
		ret=static_cast<Class_object*>(*retAddr);

	return ret;
}

_R<Class_object> Class_object::getRef()
{
	Class_object* ret = getClass();
	ret->incRef();
	return _MR(ret);
}

void Class_object::finalize()
{
	//Remove the cyclic reference to itself
	setClass(NULL);
	Class_base::finalize();
}

const std::vector<Class_base*>& Class_base::getInterfaces() const
{
	if(!interfaces.empty())
	{
		//Recursively get interfaces implemented by this interface
		for(unsigned int i=0;i<interfaces.size();i++)
		{
			ASObject* target;
			ASObject* interface_obj=this->context->root->applicationDomain->
				getVariableAndTargetByMultiname(interfaces[i], target);
			assert_and_throw(interface_obj && interface_obj->getObjectType()==T_CLASS);
			Class_base* inter=static_cast<Class_base*>(interface_obj);

			interfaces_added.push_back(inter);
			//Probe the interface for its interfaces
			inter->getInterfaces();
		}
		//Clean the interface vector to save some space
		interfaces.clear();
	}
	return interfaces_added;
}

void Class_base::linkInterface(Class_base* c) const
{
	assert(class_index!=-1);
	//Recursively link interfaces implemented by this interface
	for(unsigned int i=0;i<getInterfaces().size();i++)
		getInterfaces()[i]->linkInterface(c);

	assert_and_throw(context);

	//Link traits of this interface
	for(unsigned int j=0;j<context->instances[class_index].trait_count;j++)
	{
		traits_info* t=&context->instances[class_index].traits[j];
		context->linkTrait(c,t);
	}

	if(constructor)
	{
		LOG(LOG_CALLS,_("Calling interface init for ") << class_name);
		ASObject* ret=constructor->call(c,NULL,0);
		assert_and_throw(ret==NULL);
	}
}

bool Class_base::isSubClass(const Class_base* cls) const
{
	check();
	if(cls==this || cls==Class<ASObject>::getClass())
		return true;

	//Now check the interfaces
	for(unsigned int i=0;i<getInterfaces().size();i++)
	{
		if(getInterfaces()[i]->isSubClass(cls))
			return true;
	}

	//Now ask the super
	if(super && super->isSubClass(cls))
		return true;
	return false;
}

tiny_string Class_base::getQualifiedClassName() const
{
	//TODO: use also the namespace
	if(class_index==-1)
		return class_name.name;
	else
	{
		assert_and_throw(context);
		int name_index=context->instances[class_index].name;
		assert_and_throw(name_index);
		const multiname* mname=context->getMultiname(name_index,NULL);
		return mname->qualifiedString();
	}
}

ASObject *Class_base::describeType() const
{
	xmlpp::DomParser p;
	xmlpp::Element* root=p.get_document()->create_root_node("type");

	root->set_attribute("name", getQualifiedClassName().raw_buf());
	root->set_attribute("base", "Class");
	root->set_attribute("isDynamic", "true");
	root->set_attribute("isFinal", "true");
	root->set_attribute("isStatic", "true");

	// extendsClass
	xmlpp::Element* extends_class=root->add_child("extendsClass");
	extends_class->set_attribute("type", "Class");
	extends_class=root->add_child("extendsClass");
	extends_class->set_attribute("type", "Object");

	// variable
	if(class_index>=0)
		describeTraits(root, context->classes[class_index].traits);

	// factory
	xmlpp::Element* factory=root->add_child("factory");
	factory->set_attribute("type", getQualifiedClassName().raw_buf());
	describeInstance(factory);
	
	return Class<XML>::getInstanceS(root);
}

void Class_base::describeInstance(xmlpp::Element* root) const
{
	// extendsClass
	const Class_base* c=super.getPtr();
	while(c)
	{
		xmlpp::Element* extends_class=root->add_child("extendsClass");
		extends_class->set_attribute("type", c->getQualifiedClassName().raw_buf());
		c=c->super.getPtr();
	}

	// implementsInterface
	c=this;
	while(c && c->class_index>=0)
	{
		const std::vector<Class_base*>& interfaces=c->getInterfaces();
		auto it=interfaces.begin();
		for(; it!=interfaces.end(); ++it)
		{
			xmlpp::Element* node=root->add_child("implementsInterface");
			node->set_attribute("type", (*it)->getQualifiedClassName().raw_buf());
		}
		c=c->super.getPtr();
	}

	// variables, methods, accessors
	c=this;
	while(c && c->class_index>=0)
	{
		c->describeTraits(root, c->context->instances[c->class_index].traits);
		c=c->super.getPtr();
	}
}

void Class_base::describeTraits(xmlpp::Element* root,
				std::vector<traits_info>& traits) const
{
	std::map<u30, xmlpp::Element*> accessorNodes;
	for(unsigned int i=0;i<traits.size();i++)
	{
		traits_info& t=traits[i];
		int kind=t.kind&0xf;
		multiname* mname=context->getMultiname(t.name,NULL);
		if (mname->name_type!=multiname::NAME_STRING ||
		    (mname->ns.size()==1 && !mname->ns[0].hasEmptyName()) ||
		    mname->ns.size() > 1)
			continue;
		
		if(kind==traits_info::Slot || kind==traits_info::Const)
		{
			multiname* type=context->getMultiname(t.type_name,NULL);
			const char *nodename=kind==traits_info::Const?"constant":"variable";
			xmlpp::Element* node=root->add_child(nodename);
			node->set_attribute("name", getSys()->getStringFromUniqueId(mname->name_s_id).raw_buf());
			node->set_attribute("type", type->qualifiedString().raw_buf());

			describeMetadata(node, t);
		}
		else if (kind==traits_info::Method)
		{
			xmlpp::Element* node=root->add_child("method");
			node->set_attribute("name", getSys()->getStringFromUniqueId(mname->name_s_id).raw_buf());
			node->set_attribute("declaredBy", getQualifiedClassName().raw_buf());

			method_info& method=context->methods[t.method];
			const multiname* rtname=method.returnTypeName();
			node->set_attribute("returnType", rtname->qualifiedString().raw_buf());

			assert(method.numArgs() >= method.numOptions());
			uint32_t firstOpt=method.numArgs() - method.numOptions();
			for(uint32_t j=0;j<method.numArgs(); j++)
			{
				xmlpp::Element* param=node->add_child("parameter");
				param->set_attribute("index", UInteger::toString(j+1).raw_buf());
				param->set_attribute("type", method.paramTypeName(j)->qualifiedString().raw_buf());
				param->set_attribute("optional", j>=firstOpt?"true":"false");
			}

			describeMetadata(node, t);
		}
		else if (kind==traits_info::Getter || kind==traits_info::Setter)
		{
			// The getters and setters are separate
			// traits. Check if we have already created a
			// node for this multiname with the
			// complementary accessor. If we have, update
			// the access attribute to "readwrite".
			xmlpp::Element* node;
			auto existing=accessorNodes.find(t.name);
			if(existing==accessorNodes.end())
			{
				node=root->add_child("accessor");
				accessorNodes[t.name]=node;
			}
			else
				node=existing->second;

			node->set_attribute("name", getSys()->getStringFromUniqueId(mname->name_s_id).raw_buf());

			const char* access=NULL;
			tiny_string oldAccess;
			xmlpp::Attribute* oldAttr=node->get_attribute("access");
			if(oldAttr)
				oldAccess=oldAttr->get_value();

			if(kind==traits_info::Getter && oldAccess=="")
				access="readonly";
			else if(kind==traits_info::Setter && oldAccess=="")
				access="writeonly";
			else if((kind==traits_info::Getter && oldAccess=="writeonly") || 
				(kind==traits_info::Setter && oldAccess=="readonly"))
				access="readwrite";

			if(access)
				node->set_attribute("access", access);

			tiny_string type;
			method_info& method=context->methods[t.method];
			if(kind==traits_info::Getter)
			{
				const multiname* rtname=method.returnTypeName();
				type=rtname->qualifiedString();
			}
			else if(method.numArgs()>0) // setter
			{
				type=method.paramTypeName(0)->qualifiedString();
			}
			if(!type.empty())
				node->set_attribute("type", type.raw_buf());

			node->set_attribute("declaredBy", getQualifiedClassName().raw_buf());

			describeMetadata(node, t);
		}
	}
}

void Class_base::describeMetadata(xmlpp::Element* root, const traits_info& trait) const
{
	if((trait.kind&traits_info::Metadata) == 0)
		return;

	for(unsigned int i=0;i<trait.metadata_count;i++)
	{
		xmlpp::Element *metadata_node=root->add_child("metadata");
		metadata_info& minfo = context->metadata[trait.metadata[i]];
		metadata_node->set_attribute("name", context->getString(minfo.name));

		for(unsigned int j=0;j<minfo.item_count;++j)
		{
			xmlpp::Element *arg_node=metadata_node->add_child("arg");
			arg_node->set_attribute("key", context->getString(minfo.items[j].key));
			arg_node->set_attribute("value", context->getString(minfo.items[j].value));
		}
	}
}

void Class_base::initializeProtectedNamespace(const tiny_string& name, const namespace_info& ns)
{
	Class_inherit* cur=dynamic_cast<Class_inherit*>(super.getPtr());
	nsNameAndKind* baseNs=NULL;
	while(cur)
	{
		if(cur->use_protected)
		{
			baseNs=&cur->protected_ns;
			break;
		}
		cur=dynamic_cast<Class_inherit*>(cur->super.getPtr());
	}
	if(baseNs==NULL)
		protected_ns=nsNameAndKind(name,(NS_KIND)(int)ns.kind);
	else
		protected_ns=nsNameAndKind(name,baseNs->nsId,(NS_KIND)(int)ns.kind);
}

const variable* Class_base::findBorrowedGettable(const multiname& name) const
{
	return ASObject::findGettableImpl(borrowedVariables,name);
}

variable* Class_base::findBorrowedSettable(const multiname& name, bool* has_getter)
{
	return ASObject::findSettableImpl(borrowedVariables,name,has_getter);
}

EARLY_BIND_STATUS Class_base::resolveMultinameStatically(const multiname& name) const
{
	if(findBorrowedGettable(name)!=NULL)
		return BINDED;
	else
		return NOT_BINDED;
}

ASQName::ASQName(Class_base* c):ASObject(c)
{
	type=T_QNAME; uri_is_null=false;
}
void ASQName::setByNode(xmlpp::Node* node)
{
	uri_is_null=false;
	local_name = node->get_name();
	uri=node->get_namespace_uri();
}

void ASQName::sinit(Class_base* c)
{
	c->setSuper(Class<ASObject>::getRef());
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setDeclaredMethodByQName("uri","",Class<IFunction>::getFunction(_getURI),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("localName","",Class<IFunction>::getFunction(_getLocalName),GETTER_METHOD,true);
	c->prototype->setVariableByQName("toString",AS3,Class<IFunction>::getFunction(_toString),DYNAMIC_TRAIT);
}

ASFUNCTIONBODY(ASQName,_constructor)
{
	ASQName* th=static_cast<ASQName*>(obj);
	assert_and_throw(argslen<3);

	ASObject *nameval;
	ASObject *namespaceval;

	if(argslen==0)
	{
		th->local_name="";
		th->uri_is_null=false;
		th->uri=getVm()->getDefaultXMLNamespace();
		return NULL;
	}
	if(argslen==1)
	{
		nameval=args[0];
		namespaceval=NULL;
	}
	else if(argslen==2)
	{
		namespaceval=args[0];
		nameval=args[1];
	}

	// Set local_name
	if(nameval->getObjectType()==T_QNAME)
	{
		ASQName *q=static_cast<ASQName*>(nameval);
		th->local_name=q->local_name;
		if(!namespaceval)
		{
			th->uri_is_null=q->uri_is_null;
			th->uri=q->uri;
			return NULL;
		}
	}
	else if(nameval->getObjectType()==T_UNDEFINED)
		th->local_name="";
	else
		th->local_name=nameval->toString();

	// Set uri
	th->uri_is_null=false;
	if(!namespaceval || namespaceval->getObjectType()==T_UNDEFINED)
	{
		if(th->local_name=="*")
		{
			th->uri_is_null=true;
			th->uri="";
		}
		else
		{
			th->uri=getVm()->getDefaultXMLNamespace();
		}
	}
	else if(namespaceval->getObjectType()==T_NULL)
	{
		th->uri_is_null=true;
		th->uri="";
	}
	else
	{
		if(namespaceval->getObjectType()==T_QNAME && 
		   !(static_cast<ASQName*>(namespaceval)->uri_is_null))
		{
			ASQName* q=static_cast<ASQName*>(namespaceval);
			th->uri=q->uri;
		}
		else
			th->uri=namespaceval->toString();
	}

	return NULL;
}
ASFUNCTIONBODY(ASQName,generator)
{
	ASQName* th=Class<ASQName>::getInstanceS();
	assert_and_throw(argslen<3);

	ASObject *nameval;
	ASObject *namespaceval;

	if(argslen==0)
	{
		th->local_name="";
		th->uri_is_null=false;
		th->uri="";
		// Should set th->uri to the default namespace
		LOG(LOG_NOT_IMPLEMENTED, "QName constructor not completely implemented");
		return th;
	}
	if(argslen==1)
	{
		nameval=args[0];
		namespaceval=NULL;
	}
	else if(argslen==2)
	{
		namespaceval=args[0];
		nameval=args[1];
	}

	// Set local_name
	if(nameval->getObjectType()==T_QNAME)
	{
		ASQName *q=static_cast<ASQName*>(nameval);
		th->local_name=q->local_name;
		if(!namespaceval)
		{
			th->uri_is_null=q->uri_is_null;
			th->uri=q->uri;
			return th;
		}
	}
	else if(nameval->getObjectType()==T_UNDEFINED)
		th->local_name="";
	else
		th->local_name=nameval->toString();

	// Set uri
	th->uri_is_null=false;
	if(!namespaceval || namespaceval->getObjectType()==T_UNDEFINED)
	{
		if(th->local_name=="*")
		{
			th->uri_is_null=true;
			th->uri="";
		}
		else
		{
			// Should set th->uri to the default namespace
			LOG(LOG_NOT_IMPLEMENTED, "QName constructor not completely implemented");
			th->uri="";
		}
	}
	else if(namespaceval->getObjectType()==T_NULL)
	{
		th->uri_is_null=true;
		th->uri="";
	}
	else
	{
		if(namespaceval->getObjectType()==T_QNAME && 
		   !(static_cast<ASQName*>(namespaceval)->uri_is_null))
		{
			ASQName* q=static_cast<ASQName*>(namespaceval);
			th->uri=q->uri;
		}
		else
			th->uri=namespaceval->toString();
	}
	return th;
}

ASFUNCTIONBODY(ASQName,_getURI)
{
	ASQName* th=static_cast<ASQName*>(obj);
	if(th->uri_is_null)
		return getSys()->getNullRef();
	else
		return Class<ASString>::getInstanceS(th->uri);
}

ASFUNCTIONBODY(ASQName,_getLocalName)
{
	ASQName* th=static_cast<ASQName*>(obj);
	return Class<ASString>::getInstanceS(th->local_name);
}

ASFUNCTIONBODY(ASQName,_toString)
{
	if(!obj->is<ASQName>())
		throw Class<TypeError>::getInstanceS("QName.toString is not generic");
	ASQName* th=static_cast<ASQName*>(obj);
	return Class<ASString>::getInstanceS(th->toString());
}

bool ASQName::isEqual(ASObject* o)
{
	if(o->getObjectType()==T_QNAME)
	{
		ASQName *q=static_cast<ASQName *>(o);
		return uri_is_null==q->uri_is_null && uri==q->uri && local_name==q->local_name;
	}

	return ASObject::isEqual(o);
}

tiny_string ASQName::toString()
{
	tiny_string s;
	if(uri_is_null)
		s = "*::";
	else if(uri!="")
		s = uri + "::";

	return s + local_name;
}

Namespace::Namespace(Class_base* c):ASObject(c)
{
	type=T_NAMESPACE;
	prefix_is_undefined=false;
}

Namespace::Namespace(Class_base* c, const tiny_string& _uri, const tiny_string& _prefix)
  : ASObject(c),uri(_uri),prefix(_prefix)
{
	type=T_NAMESPACE;
	prefix_is_undefined=false;
}

void Namespace::sinit(Class_base* c)
{
	c->setSuper(Class<ASObject>::getRef());
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setDeclaredMethodByQName("uri","",Class<IFunction>::getFunction(_setURI),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("uri","",Class<IFunction>::getFunction(_getURI),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("prefix","",Class<IFunction>::getFunction(_setPrefix),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("prefix","",Class<IFunction>::getFunction(_getPrefix),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("valueOf",AS3,Class<IFunction>::getFunction(_valueOf),NORMAL_METHOD,true);
	c->prototype->setVariableByQName("toString","",Class<IFunction>::getFunction(_toString),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("valueOf","",Class<IFunction>::getFunction(_ECMA_valueOf),DYNAMIC_TRAIT);
}

void Namespace::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(Namespace,_constructor)
{
	ASObject *urival;
	ASObject *prefixval;
	Namespace* th=static_cast<Namespace*>(obj);
	assert_and_throw(argslen<3);

	if (argslen == 0)
	{
		//Return before resetting the value to preserve those eventually set by the C++ constructor
		return NULL;
	}
	else if (argslen == 1)
	{
		urival = args[0];
		prefixval = NULL;
	}
	else
	{
		prefixval = args[0];
		urival = args[1];
	}
	th->prefix_is_undefined=false;
	th->prefix = "";
	th->uri = "";

	if(!prefixval)
	{
		if(urival->getObjectType()==T_NAMESPACE)
		{
			Namespace* n=static_cast<Namespace*>(urival);
			th->uri=n->uri;
			th->prefix=n->prefix;
			th->prefix_is_undefined=n->prefix_is_undefined;
		}
		else if(urival->getObjectType()==T_QNAME && 
		   !(static_cast<ASQName*>(urival)->uri_is_null))
		{
			ASQName* q=static_cast<ASQName*>(urival);
			th->uri=q->uri;
		}
		else
		{
			th->uri=urival->toString();
			if(th->uri!="")
			{
				th->prefix_is_undefined=true;
				th->prefix="";
			}
		}
	}
	else // has both urival and prefixval
	{
		if(urival->getObjectType()==T_QNAME &&
		   !(static_cast<ASQName*>(urival)->uri_is_null))
		{
			ASQName* q=static_cast<ASQName*>(urival);
			th->uri=q->uri;
		}
		else
		{
			th->uri=urival->toString();
		}

		if(th->uri=="")
		{
			if(prefixval->getObjectType()==T_UNDEFINED ||
			   prefixval->toString()=="")
				th->prefix="";
			else
				throw Class<TypeError>::getInstanceS("Namespace prefix for empty uri not allowed");
		}
		else if(prefixval->getObjectType()==T_UNDEFINED ||
			!isXMLName(prefixval))
		{
			th->prefix_is_undefined=true;
			th->prefix="";
		}
		else
		{
			th->prefix=prefixval->toString();
		}
	}

	return NULL;
}
ASFUNCTIONBODY(Namespace,generator)
{
	Namespace* th=Class<Namespace>::getInstanceS();
	ASObject *urival;
	ASObject *prefixval;
	assert_and_throw(argslen<3);

	if (argslen == 0)
	{
		th->prefix_is_undefined=false;
		th->prefix = "";
		th->uri = "";
		return th;
	}
	else if (argslen == 1)
	{
		urival = args[0];
		prefixval = NULL;
	}
	else
	{
		prefixval = args[0];
		urival = args[1];
	}
	th->prefix_is_undefined=false;
	th->prefix = "";
	th->uri = "";

	if(!prefixval)
	{
		if(urival->getObjectType()==T_NAMESPACE)
		{
			Namespace* n=static_cast<Namespace*>(urival);
			th->uri=n->uri;
			th->prefix=n->prefix;
			th->prefix_is_undefined=n->prefix_is_undefined;
		}
		else if(urival->getObjectType()==T_QNAME && 
		   !(static_cast<ASQName*>(urival)->uri_is_null))
		{
			ASQName* q=static_cast<ASQName*>(urival);
			th->uri=q->uri;
		}
		else
		{
			th->uri=urival->toString();
			if(th->uri!="")
			{
				th->prefix_is_undefined=true;
				th->prefix="";
			}
		}
	}
	else // has both urival and prefixval
	{
		if(urival->getObjectType()==T_QNAME &&
		   !(static_cast<ASQName*>(urival)->uri_is_null))
		{
			ASQName* q=static_cast<ASQName*>(urival);
			th->uri=q->uri;
		}
		else
		{
			th->uri=urival->toString();
		}

		if(th->uri=="")
		{
			if(prefixval->getObjectType()==T_UNDEFINED ||
			   prefixval->toString()=="")
				th->prefix="";
			else
				throw Class<TypeError>::getInstanceS("Namespace prefix for empty uri not allowed");
		}
		else if(prefixval->getObjectType()==T_UNDEFINED ||
			!isXMLName(prefixval))
		{
			th->prefix_is_undefined=true;
			th->prefix="";
		}
		else
		{
			th->prefix=prefixval->toString();
		}
	}
	return th;
}

ASFUNCTIONBODY(Namespace,_setURI)
{
	Namespace* th=static_cast<Namespace*>(obj);
	th->uri=args[0]->toString();
	return NULL;
}

ASFUNCTIONBODY(Namespace,_getURI)
{
	Namespace* th=static_cast<Namespace*>(obj);
	return Class<ASString>::getInstanceS(th->uri);
}

ASFUNCTIONBODY(Namespace,_setPrefix)
{
	Namespace* th=static_cast<Namespace*>(obj);
	if(args[0]->getObjectType()==T_UNDEFINED)
	{
		th->prefix_is_undefined=true;
		th->prefix="";
	}
	else
	{
		th->prefix_is_undefined=false;
		th->prefix=args[0]->toString();
	}
	return NULL;
}

ASFUNCTIONBODY(Namespace,_getPrefix)
{
	Namespace* th=static_cast<Namespace*>(obj);
	if(th->prefix_is_undefined)
		return getSys()->getUndefinedRef();
	else
		return Class<ASString>::getInstanceS(th->prefix);
}

ASFUNCTIONBODY(Namespace,_toString)
{
	if(!obj->is<Namespace>())
		throw Class<TypeError>::getInstanceS("Namespace.toString is not generic");
	Namespace* th=static_cast<Namespace*>(obj);
	return Class<ASString>::getInstanceS(th->uri);
}

ASFUNCTIONBODY(Namespace,_valueOf)
{
	return Class<ASString>::getInstanceS(obj->as<Namespace>()->uri);
}

ASFUNCTIONBODY(Namespace,_ECMA_valueOf)
{
	if(!obj->is<Namespace>())
		throw Class<TypeError>::getInstanceS("Namespace.valueOf is not generic");
	Namespace* th=static_cast<Namespace*>(obj);
	return Class<ASString>::getInstanceS(th->uri);
}

bool Namespace::isEqual(ASObject* o)
{
	if(o->getObjectType()==T_NAMESPACE)
	{
		Namespace *n=static_cast<Namespace *>(o);
		return uri==n->uri;
	}

	return ASObject::isEqual(o);
}


ASObject* ASNop(ASObject* obj, ASObject* const* args, const unsigned int argslen)
{
	return getSys()->getUndefinedRef();
}

IFunction* Class<IFunction>::getNopFunction()
{
	IFunction* ret=new (this->memoryAccount) Function(this, ASNop);
	//Similarly to newFunction, we must create a prototype object
	ret->prototype = _MR(new_asobject());
	return ret;
}

ASObject* Class<IFunction>::getInstance(bool construct, ASObject* const* args, const unsigned int argslen, Class_base* realClass)
{
	if (argslen > 0)
		throw Class<EvalError>::getInstanceS("Error #1066: Function('function body') is not supported");
	return getNopFunction();
}

Class<IFunction>* Class<IFunction>::getClass()
{
	uint32_t classId=ClassName<IFunction>::id;
	Class<IFunction>* ret=NULL;
	Class_base** retAddr=&getSys()->builtinClasses[classId];
	if(*retAddr==NULL)
	{
		//Create the class
		MemoryAccount* memoryAccount = getSys()->allocateMemoryAccount(ClassName<IFunction>::name);
		ret=new (getSys()->unaccountedMemory) Class<IFunction>(memoryAccount);
		//This function is called from Class<ASObject>::getRef(),
		//so the Class<ASObject> we obtain will not have any
		//declared methods yet! Therefore, set super will not copy
		//up any borrowed traits from there. We do that by ourself.
		ret->setSuper(Class<ASObject>::getRef());
		//The prototype for Function seems to be a function object. Use the special FunctionPrototype
		ret->prototype = _MNR(new_functionPrototype(ret, ret->super->prototype));
		ret->incRef();
		ret->prototype->getObj()->setVariableByQName("constructor","",ret,DYNAMIC_TRAIT);
		ret->incRef();
		*retAddr = ret;

		//we cannot use sinit, as we need to setup 'this_class' before calling
		//addPrototypeGetter and setDeclaredMethodByQName.
		//Thus we make sure that everything is in order when getFunction() below is called
		ret->addPrototypeGetter();
		//copy borrowed traits from ASObject by ourself
		ASObject::sinit(ret);
		ret->setDeclaredMethodByQName("call",AS3,Class<IFunction>::getFunction(IFunction::_call),NORMAL_METHOD,true);
		ret->setDeclaredMethodByQName("apply",AS3,Class<IFunction>::getFunction(IFunction::apply),NORMAL_METHOD,true);
		ret->setDeclaredMethodByQName("prototype","",Class<IFunction>::getFunction(IFunction::_getter_prototype),GETTER_METHOD,true);
		ret->setDeclaredMethodByQName("prototype","",Class<IFunction>::getFunction(IFunction::_setter_prototype),SETTER_METHOD,true);
		ret->setDeclaredMethodByQName("length","",Class<IFunction>::getFunction(IFunction::_getter_length),GETTER_METHOD,true);
		ret->prototype->setVariableByQName("toString",AS3,Class<IFunction>::getFunction(IFunction::_toString),DYNAMIC_TRAIT);
		ret->setDeclaredMethodByQName("toString",AS3,Class<IFunction>::getFunction(Class_base::_toString),NORMAL_METHOD,false);
	}
	else
		ret=static_cast<Class<IFunction>*>(*retAddr);

	return ret;
}


Global::Global(Class_base* cb, ABCContext* c, int s):ASObject(cb),scriptId(s),context(c)
{
}

void Global::sinit(Class_base* c)
{
	c->setSuper(Class<ASObject>::getRef());
}

_NR<ASObject> Global::getVariableByMultinameOpportunistic(const multiname& name)
{
	_NR<ASObject> ret = ASObject::getVariableByMultiname(name, NONE);
	//Do not attempt to define the variable now in any case
	return ret;
}

_NR<ASObject> Global::getVariableByMultiname(const multiname& name, GET_VARIABLE_OPTION opt)
{
	_NR<ASObject> ret = ASObject::getVariableByMultiname(name, opt);
	/*
	 * All properties are registered by now, even if the script init has
	 * not been run. Thus if ret == NULL, we don't have to run the script init.
	 */
	if(ret.isNull() || !context || context->hasRunScriptInit[scriptId])
		return ret;
	LOG(LOG_CALLS,"Access to " << name << ", running script init");
	context->runScriptInit(scriptId, this);
	return ASObject::getVariableByMultiname(name, opt);
}

void Global::registerBuiltin(const char* name, const char* ns, _R<ASObject> o)
{
	o->incRef();
	setVariableByQName(name,nsNameAndKind(ns,NAMESPACE),o.getPtr(),DECLARED_TRAIT);
}

ASFUNCTIONBODY(lightspark,eval)
{
    // eval is not allowed in AS3, but an exception should be thrown
	throw Class<EvalError>::getInstanceS("EvalError");
}

ASFUNCTIONBODY(lightspark,parseInt)
{
	tiny_string str;
	int radix;
	ARG_UNPACK (str, "") (radix, 0);

	if(radix != 0 && (radix < 2 || radix > 36))
		return abstract_d(numeric_limits<double>::quiet_NaN());

	const char* cur=str.raw_buf();
	int64_t ret;
	bool valid=Integer::fromStringFlashCompatible(cur,ret,radix);

	if(valid==false)
		return abstract_d(numeric_limits<double>::quiet_NaN());
	if(ret==INT64_MAX)
		return abstract_d(numeric_limits<double>::infinity());
	if(ret==INT64_MIN)
		return abstract_d(-numeric_limits<double>::infinity());
	return abstract_d(ret);
}

ASFUNCTIONBODY(lightspark,parseFloat)
{
	tiny_string str;
	const char *p;
	char *end;
	ARG_UNPACK (str, "");

	// parsing of hex numbers is not allowed
	char* p1 = str.strchr('x');
	if (p1) *p1='y';
	p1 = str.strchr('X');
	if (p1) *p1='Y';

	p=str.raw_buf();
	double d=strtod(p, &end);

	if (end==p)
		return abstract_d(numeric_limits<double>::quiet_NaN());

	return abstract_d(d);
}

ASFUNCTIONBODY(lightspark,isNaN)
{
	if(argslen==0)
		return abstract_b(true);
	else if(args[0]->getObjectType()==T_UNDEFINED)
		return abstract_b(true);
	else if(args[0]->getObjectType()==T_INTEGER)
		return abstract_b(false);
	else if(args[0]->getObjectType()==T_BOOLEAN)
		return abstract_b(false);
	else if(args[0]->getObjectType()==T_NULL)
		return abstract_b(false); // because Number(null) == 0
	else
		return abstract_b(std::isnan(args[0]->toNumber()));
}

ASFUNCTIONBODY(lightspark,isFinite)
{
	if(argslen==0)
		return abstract_b(false);
	else
		return abstract_b(isfinite(args[0]->toNumber()));
}

ASFUNCTIONBODY(lightspark,encodeURI)
{
	tiny_string str;
	ARG_UNPACK (str, "undefined");
	return Class<ASString>::getInstanceS(URLInfo::encode(str, URLInfo::ENCODE_URI));
}

ASFUNCTIONBODY(lightspark,decodeURI)
{
	tiny_string str;
	ARG_UNPACK (str, "undefined");
	return Class<ASString>::getInstanceS(URLInfo::decode(str, URLInfo::ENCODE_URI));
}

ASFUNCTIONBODY(lightspark,encodeURIComponent)
{
	tiny_string str;
	ARG_UNPACK (str, "undefined");
	return Class<ASString>::getInstanceS(URLInfo::encode(str, URLInfo::ENCODE_URICOMPONENT));
}

ASFUNCTIONBODY(lightspark,decodeURIComponent)
{
	tiny_string str;
	ARG_UNPACK (str, "undefined");
	return Class<ASString>::getInstanceS(URLInfo::decode(str, URLInfo::ENCODE_URICOMPONENT));
}

ASFUNCTIONBODY(lightspark,escape)
{
	tiny_string str;
	ARG_UNPACK (str, "undefined");
	return Class<ASString>::getInstanceS(URLInfo::encode(str, URLInfo::ENCODE_ESCAPE));
}

ASFUNCTIONBODY(lightspark,unescape)
{
	tiny_string str;
	ARG_UNPACK (str, "undefined");
	return Class<ASString>::getInstanceS(URLInfo::decode(str, URLInfo::ENCODE_ESCAPE));
}

ASFUNCTIONBODY(lightspark,print)
{
	if(args[0]->getObjectType() == T_STRING)
	{
		ASString* str = static_cast<ASString*>(args[0]);
		Log::print(str->data);
	}
	else
		Log::print(args[0]->toString());
	return NULL;
}

ASFUNCTIONBODY(lightspark,trace)
{
	stringstream s;
	for(uint32_t i = 0; i< argslen;i++)
	{
		if(i > 0)
			s << " ";

		if(args[i]->getObjectType() == T_STRING)
		{
			ASString* str = static_cast<ASString*>(args[i]);
			s << str->data;
		}
		else
			s << args[i]->toString();
	}
	Log::print(s.str());
	return NULL;
}

bool lightspark::isXMLName(ASObject *obj)
{
	tiny_string name;

	if(obj->getObjectType()==lightspark::T_QNAME)
	{
		ASQName *q=static_cast<ASQName*>(obj);
		name=q->getLocalName();
	}
	else if(obj->getObjectType()==lightspark::T_UNDEFINED ||
		obj->getObjectType()==lightspark::T_NULL)
		name="";
	else
		name=obj->toString();

	if(name.empty())
		return false;

	// http://www.w3.org/TR/2006/REC-xml-names-20060816/#NT-NCName
	// Note: Flash follows the second edition (20060816) of the
	// standard. The character range definitions were changed in
	// the newer edition.
	#define NC_START_CHAR(x) \
	  ((0x0041 <= x && x <= 0x005A) || x == 0x5F || \
	  (0x0061 <= x && x <= 0x007A) || (0x00C0 <= x && x <= 0x00D6) || \
	  (0x00D8 <= x && x <= 0x00F6) || (0x00F8 <= x && x <= 0x00FF) || \
	  (0x0100 <= x && x <= 0x0131) || (0x0134 <= x && x <= 0x013E) || \
	  (0x0141 <= x && x <= 0x0148) || (0x014A <= x && x <= 0x017E) || \
	  (0x0180 <= x && x <= 0x01C3) || (0x01CD <= x && x <= 0x01F0) || \
	  (0x01F4 <= x && x <= 0x01F5) || (0x01FA <= x && x <= 0x0217) || \
	  (0x0250 <= x && x <= 0x02A8) || (0x02BB <= x && x <= 0x02C1) || \
	  x == 0x0386 || (0x0388 <= x && x <= 0x038A) || x == 0x038C || \
	  (0x038E <= x && x <= 0x03A1) || (0x03A3 <= x && x <= 0x03CE) || \
	  (0x03D0 <= x && x <= 0x03D6) || x == 0x03DA || x == 0x03DC || \
	  x == 0x03DE || x == 0x03E0 || (0x03E2 <= x && x <= 0x03F3) || \
	  (0x0401 <= x && x <= 0x040C) || (0x040E <= x && x <= 0x044F) || \
	  (0x0451 <= x && x <= 0x045C) || (0x045E <= x && x <= 0x0481) || \
	  (0x0490 <= x && x <= 0x04C4) || (0x04C7 <= x && x <= 0x04C8) || \
	  (0x04CB <= x && x <= 0x04CC) || (0x04D0 <= x && x <= 0x04EB) || \
	  (0x04EE <= x && x <= 0x04F5) || (0x04F8 <= x && x <= 0x04F9) || \
	  (0x0531 <= x && x <= 0x0556) || x == 0x0559 || \
	  (0x0561 <= x && x <= 0x0586) || (0x05D0 <= x && x <= 0x05EA) || \
	  (0x05F0 <= x && x <= 0x05F2) || (0x0621 <= x && x <= 0x063A) || \
	  (0x0641 <= x && x <= 0x064A) || (0x0671 <= x && x <= 0x06B7) || \
	  (0x06BA <= x && x <= 0x06BE) || (0x06C0 <= x && x <= 0x06CE) || \
	  (0x06D0 <= x && x <= 0x06D3) || x == 0x06D5 || \
	  (0x06E5 <= x && x <= 0x06E6) || (0x0905 <= x && x <= 0x0939) || \
	  x == 0x093D || (0x0958 <= x && x <= 0x0961) || \
	  (0x0985 <= x && x <= 0x098C) || (0x098F <= x && x <= 0x0990) || \
	  (0x0993 <= x && x <= 0x09A8) || (0x09AA <= x && x <= 0x09B0) || \
	  x == 0x09B2 || (0x09B6 <= x && x <= 0x09B9) || \
	  (0x09DC <= x && x <= 0x09DD) || (0x09DF <= x && x <= 0x09E1) || \
	  (0x09F0 <= x && x <= 0x09F1) || (0x0A05 <= x && x <= 0x0A0A) || \
	  (0x0A0F <= x && x <= 0x0A10) || (0x0A13 <= x && x <= 0x0A28) || \
	  (0x0A2A <= x && x <= 0x0A30) || (0x0A32 <= x && x <= 0x0A33) || \
	  (0x0A35 <= x && x <= 0x0A36) || (0x0A38 <= x && x <= 0x0A39) || \
	  (0x0A59 <= x && x <= 0x0A5C) || x == 0x0A5E || \
	  (0x0A72 <= x && x <= 0x0A74) || (0x0A85 <= x && x <= 0x0A8B) || \
	  x == 0x0A8D || (0x0A8F <= x && x <= 0x0A91) || \
	  (0x0A93 <= x && x <= 0x0AA8) || (0x0AAA <= x && x <= 0x0AB0) || \
	  (0x0AB2 <= x && x <= 0x0AB3) || (0x0AB5 <= x && x <= 0x0AB9) || \
	  x == 0x0ABD || x == 0x0AE0 || (0x0B05 <= x && x <= 0x0B0C) || \
	  (0x0B0F <= x && x <= 0x0B10) || (0x0B13 <= x && x <= 0x0B28) || \
	  (0x0B2A <= x && x <= 0x0B30) || (0x0B32 <= x && x <= 0x0B33) || \
	  (0x0B36 <= x && x <= 0x0B39) || x == 0x0B3D || \
	  (0x0B5C <= x && x <= 0x0B5D) || (0x0B5F <= x && x <= 0x0B61) || \
	  (0x0B85 <= x && x <= 0x0B8A) || (0x0B8E <= x && x <= 0x0B90) || \
	  (0x0B92 <= x && x <= 0x0B95) || (0x0B99 <= x && x <= 0x0B9A) || \
	  x == 0x0B9C || (0x0B9E <= x && x <= 0x0B9F) || \
	  (0x0BA3 <= x && x <= 0x0BA4) || (0x0BA8 <= x && x <= 0x0BAA) || \
	  (0x0BAE <= x && x <= 0x0BB5) || (0x0BB7 <= x && x <= 0x0BB9) || \
	  (0x0C05 <= x && x <= 0x0C0C) || (0x0C0E <= x && x <= 0x0C10) || \
	  (0x0C12 <= x && x <= 0x0C28) || (0x0C2A <= x && x <= 0x0C33) || \
	  (0x0C35 <= x && x <= 0x0C39) || (0x0C60 <= x && x <= 0x0C61) || \
	  (0x0C85 <= x && x <= 0x0C8C) || (0x0C8E <= x && x <= 0x0C90) || \
	  (0x0C92 <= x && x <= 0x0CA8) || (0x0CAA <= x && x <= 0x0CB3) || \
	  (0x0CB5 <= x && x <= 0x0CB9) || x == 0x0CDE || \
	  (0x0CE0 <= x && x <= 0x0CE1) || (0x0D05 <= x && x <= 0x0D0C) || \
	  (0x0D0E <= x && x <= 0x0D10) || (0x0D12 <= x && x <= 0x0D28) || \
	  (0x0D2A <= x && x <= 0x0D39) || (0x0D60 <= x && x <= 0x0D61) || \
	  (0x0E01 <= x && x <= 0x0E2E) || x == 0x0E30 || \
	  (0x0E32 <= x && x <= 0x0E33) || (0x0E40 <= x && x <= 0x0E45) || \
	  (0x0E81 <= x && x <= 0x0E82) || x == 0x0E84 || \
	  (0x0E87 <= x && x <= 0x0E88) || x == 0x0E8A || x == 0x0E8D || \
	  (0x0E94 <= x && x <= 0x0E97) || (0x0E99 <= x && x <= 0x0E9F) || \
	  (0x0EA1 <= x && x <= 0x0EA3) || x == 0x0EA5 || x == 0x0EA7 || \
	  (0x0EAA <= x && x <= 0x0EAB) || (0x0EAD <= x && x <= 0x0EAE) || \
	  x == 0x0EB0 || (0x0EB2 <= x && x <= 0x0EB3) || x == 0x0EBD || \
	  (0x0EC0 <= x && x <= 0x0EC4) || (0x0F40 <= x && x <= 0x0F47) || \
	  (0x0F49 <= x && x <= 0x0F69) || (0x10A0 <= x && x <= 0x10C5) || \
	  (0x10D0 <= x && x <= 0x10F6) || x == 0x1100 || \
	  (0x1102 <= x && x <= 0x1103) || (0x1105 <= x && x <= 0x1107) || \
	  x == 0x1109 || (0x110B <= x && x <= 0x110C) || \
	  (0x110E <= x && x <= 0x1112) || x == 0x113C || x == 0x113E || \
	  x == 0x1140 || x == 0x114C || x == 0x114E || x == 0x1150 || \
	  (0x1154 <= x && x <= 0x1155) || x == 0x1159 || \
	  (0x115F <= x && x <= 0x1161) || x == 0x1163 || x == 0x1165 || \
	  x == 0x1167 || x == 0x1169 || (0x116D <= x && x <= 0x116E) || \
	  (0x1172 <= x && x <= 0x1173) || x == 0x1175 || x == 0x119E || \
	  x == 0x11A8 || x == 0x11AB || (0x11AE <= x && x <= 0x11AF) || \
	  (0x11B7 <= x && x <= 0x11B8) || x == 0x11BA || \
	  (0x11BC <= x && x <= 0x11C2) || x == 0x11EB || x == 0x11F0 || \
	  x == 0x11F9 || (0x1E00 <= x && x <= 0x1E9B) || \
	  (0x1EA0 <= x && x <= 0x1EF9) || (0x1F00 <= x && x <= 0x1F15) || \
	  (0x1F18 <= x && x <= 0x1F1D) || (0x1F20 <= x && x <= 0x1F45) || \
	  (0x1F48 <= x && x <= 0x1F4D) || (0x1F50 <= x && x <= 0x1F57) || \
	  x == 0x1F59 || x == 0x1F5B || x == 0x1F5D || \
	  (0x1F5F <= x && x <= 0x1F7D) || (0x1F80 <= x && x <= 0x1FB4) || \
	  (0x1FB6 <= x && x <= 0x1FBC) || x == 0x1FBE || \
	  (0x1FC2 <= x && x <= 0x1FC4) || (0x1FC6 <= x && x <= 0x1FCC) || \
	  (0x1FD0 <= x && x <= 0x1FD3) || (0x1FD6 <= x && x <= 0x1FDB) || \
	  (0x1FE0 <= x && x <= 0x1FEC) || (0x1FF2 <= x && x <= 0x1FF4) || \
	  (0x1FF6 <= x && x <= 0x1FFC) || x == 0x2126 || \
	  (0x212A <= x && x <= 0x212B) || x == 0x212E || \
	  (0x2180 <= x && x <= 0x2182) || (0x3041 <= x && x <= 0x3094) || \
	  (0x30A1 <= x && x <= 0x30FA) || (0x3105 <= x && x <= 0x312C) || \
	  (0xAC00 <= x && x <= 0xD7A3) || (0x4E00 <= x && x <= 0x9FA5) || \
	  x == 0x3007 || (0x3021 <= x && x <= 0x3029))
	#define NC_CHAR(x) \
	  (NC_START_CHAR(x) || x == 0x2E || x == 0x2D || x == 0x5F || \
	  (0x0030 <= x && x <= 0x0039) || (0x0660 <= x && x <= 0x0669) || \
	  (0x06F0 <= x && x <= 0x06F9) || (0x0966 <= x && x <= 0x096F) || \
	  (0x09E6 <= x && x <= 0x09EF) || (0x0A66 <= x && x <= 0x0A6F) || \
	  (0x0AE6 <= x && x <= 0x0AEF) || (0x0B66 <= x && x <= 0x0B6F) || \
	  (0x0BE7 <= x && x <= 0x0BEF) || (0x0C66 <= x && x <= 0x0C6F) || \
	  (0x0CE6 <= x && x <= 0x0CEF) || (0x0D66 <= x && x <= 0x0D6F) || \
	  (0x0E50 <= x && x <= 0x0E59) || (0x0ED0 <= x && x <= 0x0ED9) || \
	  (0x0F20 <= x && x <= 0x0F29) || (0x0300 <= x && x <= 0x0345) || \
	  (0x0360 <= x && x <= 0x0361) || (0x0483 <= x && x <= 0x0486) || \
	  (0x0591 <= x && x <= 0x05A1) || (0x05A3 <= x && x <= 0x05B9) || \
	  (0x05BB <= x && x <= 0x05BD) || x == 0x05BF || \
	  (0x05C1 <= x && x <= 0x05C2) || x == 0x05C4 || \
	  (0x064B <= x && x <= 0x0652) || x == 0x0670 || \
	  (0x06D6 <= x && x <= 0x06DC) || (0x06DD <= x && x <= 0x06DF) || \
	  (0x06E0 <= x && x <= 0x06E4) || (0x06E7 <= x && x <= 0x06E8) || \
	  (0x06EA <= x && x <= 0x06ED) || (0x0901 <= x && x <= 0x0903) || \
	  x == 0x093C || (0x093E <= x && x <= 0x094C) || x == 0x094D || \
	  (0x0951 <= x && x <= 0x0954) || (0x0962 <= x && x <= 0x0963) || \
	  (0x0981 <= x && x <= 0x0983) || x == 0x09BC || x == 0x09BE || \
	  x == 0x09BF || (0x09C0 <= x && x <= 0x09C4) || \
	  (0x09C7 <= x && x <= 0x09C8) || (0x09CB <= x && x <= 0x09CD) || \
	  x == 0x09D7 || (0x09E2 <= x && x <= 0x09E3) || x == 0x0A02 || \
	  x == 0x0A3C || x == 0x0A3E || x == 0x0A3F || \
	  (0x0A40 <= x && x <= 0x0A42) || (0x0A47 <= x && x <= 0x0A48) || \
	  (0x0A4B <= x && x <= 0x0A4D) || (0x0A70 <= x && x <= 0x0A71) || \
	  (0x0A81 <= x && x <= 0x0A83) || x == 0x0ABC || \
	  (0x0ABE <= x && x <= 0x0AC5) || (0x0AC7 <= x && x <= 0x0AC9) || \
	  (0x0ACB <= x && x <= 0x0ACD) || (0x0B01 <= x && x <= 0x0B03) || \
	  x == 0x0B3C || (0x0B3E <= x && x <= 0x0B43) || \
	  (0x0B47 <= x && x <= 0x0B48) || (0x0B4B <= x && x <= 0x0B4D) || \
	  (0x0B56 <= x && x <= 0x0B57) || (0x0B82 <= x && x <= 0x0B83) || \
	  (0x0BBE <= x && x <= 0x0BC2) || (0x0BC6 <= x && x <= 0x0BC8) || \
	  (0x0BCA <= x && x <= 0x0BCD) || x == 0x0BD7 || \
	  (0x0C01 <= x && x <= 0x0C03) || (0x0C3E <= x && x <= 0x0C44) || \
	  (0x0C46 <= x && x <= 0x0C48) || (0x0C4A <= x && x <= 0x0C4D) || \
	  (0x0C55 <= x && x <= 0x0C56) || (0x0C82 <= x && x <= 0x0C83) || \
	  (0x0CBE <= x && x <= 0x0CC4) || (0x0CC6 <= x && x <= 0x0CC8) || \
	  (0x0CCA <= x && x <= 0x0CCD) || (0x0CD5 <= x && x <= 0x0CD6) || \
	  (0x0D02 <= x && x <= 0x0D03) || (0x0D3E <= x && x <= 0x0D43) || \
	  (0x0D46 <= x && x <= 0x0D48) || (0x0D4A <= x && x <= 0x0D4D) || \
	  x == 0x0D57 || x == 0x0E31 || (0x0E34 <= x && x <= 0x0E3A) || \
	  (0x0E47 <= x && x <= 0x0E4E) || x == 0x0EB1 || \
	  (0x0EB4 <= x && x <= 0x0EB9) || (0x0EBB <= x && x <= 0x0EBC) || \
	  (0x0EC8 <= x && x <= 0x0ECD) || (0x0F18 <= x && x <= 0x0F19) || \
	  x == 0x0F35 || x == 0x0F37 || x == 0x0F39 || x == 0x0F3E || \
	  x == 0x0F3F || (0x0F71 <= x && x <= 0x0F84) || \
	  (0x0F86 <= x && x <= 0x0F8B) || (0x0F90 <= x && x <= 0x0F95) || \
	  x == 0x0F97 || (0x0F99 <= x && x <= 0x0FAD) || \
	  (0x0FB1 <= x && x <= 0x0FB7) || x == 0x0FB9 || \
	  (0x20D0 <= x && x <= 0x20DC) || x == 0x20E1 || \
	  (0x302A <= x && x <= 0x302F) || x == 0x3099 || x == 0x309A || \
	  x == 0x00B7 || x == 0x02D0 || x == 0x02D1 || x == 0x0387 || \
	  x == 0x0640 || x == 0x0E46 || x == 0x0EC6 || x == 0x3005 || \
	  (0x3031 <= x && x <= 0x3035) || (0x309D <= x && x <= 0x309E) || \
	  (0x30FC <= x && x <= 0x30FE))

	auto it=name.begin();
	if(!NC_START_CHAR(*it))
		return false;
	++it;

	for(;it!=name.end(); ++it)
	{
		if(!(NC_CHAR(*it)))
			return false;
	}

	#undef NC_CHAR
	#undef NC_START_CHAR

	return true;
}

ASFUNCTIONBODY(lightspark,_isXMLName)
{
	assert_and_throw(argslen <= 1);
	if(argslen==0)
		return abstract_b(false);

	return abstract_b(isXMLName(args[0]));
}

ObjectPrototype::ObjectPrototype(Class_base* c) : ASObject(c)
{
}

void ObjectPrototype::finalize()
{
	ASObject::finalize();
	prevPrototype.reset();
}

_NR<ASObject> ObjectPrototype::getVariableByMultiname(const multiname& name, GET_VARIABLE_OPTION opt)
{
	_NR<ASObject> ret=ASObject::getVariableByMultiname(name, opt);
	if(!ret.isNull() || prevPrototype.isNull())
		return ret;

	return prevPrototype->getObj()->getVariableByMultiname(name, opt);
}

FunctionPrototype::FunctionPrototype(Class_base* c, _NR<Prototype> p) : Function(c, ASNop)
{
	prevPrototype=p;
	//Add the prototype to the Nop function
	this->prototype = _MR(new_asobject());
}

void FunctionPrototype::finalize()
{
	Function::finalize();
	prevPrototype.reset();
}

_NR<ASObject> FunctionPrototype::getVariableByMultiname(const multiname& name, GET_VARIABLE_OPTION opt)
{
	_NR<ASObject> ret=Function::getVariableByMultiname(name, opt);
	if(!ret.isNull() || prevPrototype.isNull())
		return ret;

	return prevPrototype->getObj()->getVariableByMultiname(name, opt);
}

Function_object::Function_object(Class_base* c, _R<ASObject> p) : ASObject(c), functionPrototype(p)
{
}

void Function_object::finalize()
{
	functionPrototype.reset();
	ASObject::finalize();
}

_NR<ASObject> Function_object::getVariableByMultiname(const multiname& name, GET_VARIABLE_OPTION opt)
{
	_NR<ASObject> ret=ASObject::getVariableByMultiname(name, opt);
	assert(!functionPrototype.isNull());
	if(!ret.isNull())
		return ret;

	return functionPrototype->getVariableByMultiname(name, opt);
}

EARLY_BIND_STATUS ActivationType::resolveMultinameStatically(const multiname& name) const
{
	std::cerr << "Looking for " << name << std::endl;
	for(unsigned int i=0;i<mi->body->trait_count;i++)
	{
		const traits_info* t=&mi->body->traits[i];
		multiname* mname=mi->context->getMultiname(t->name,NULL);
		std::cerr << "\t in " << *mname << std::endl;
		assert_and_throw(mname->ns.size()==1 && mname->name_type==multiname::NAME_STRING);
		if(mname->name_s_id!=name.normalizedNameId())
			continue;
		if(find(name.ns.begin(),name.ns.end(),mname->ns[0])==name.ns.end())
			continue;
		return BINDED;
	}
	return NOT_BINDED;
}

const multiname* ActivationType::resolveSlotTypeName(uint32_t slotId) const
{
	std::cerr << "Resolving type at id " << slotId << std::endl;
	for(unsigned int i=0;i<mi->body->trait_count;i++)
	{
		const traits_info* t=&mi->body->traits[i];
		if(t->slot_id!=slotId)
			continue;

		multiname* tname=mi->context->getMultiname(t->type_name,NULL);
		return tname;
	}
	return NULL;
}
