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
#include "scripting/toplevel/Class_base.h"
#include "scripting/flash/display/RootMovieClip.h"
#include "scripting/toplevel/toplevel.h"
#include "scripting/toplevel/AVM1Function.h"
#include "scripting/toplevel/Integer.h"
#include "scripting/toplevel/Number.h"
#include "scripting/toplevel/UInteger.h"
#include "scripting/toplevel/Vector.h"
#include "scripting/toplevel/XML.h"
#include "swf.h"
#include "compat.h"
#include "scripting/argconv.h"

using namespace std;
using namespace lightspark;

ASAny* Type::anyType = new ASAny();
Void* Type::voidType = new Void();

Type* Type::getBuiltinType(ASWorker* wrk, multiname* mn)
{
	if(mn->isStatic && mn->cachedType)
		return mn->cachedType;
	assert_and_throw(mn->isQName());
	assert(mn->name_type == multiname::NAME_STRING);
	if(mn == 0)
		return Type::anyType; //any
	if(mn->name_type == multiname::NAME_STRING && mn->name_s_id==BUILTIN_STRINGS::ANY
		&& mn->hasEmptyNS)
		return Type::anyType;
	if(mn->name_type == multiname::NAME_STRING && mn->name_s_id==BUILTIN_STRINGS::VOID
		&& mn->hasEmptyNS)
		return Type::voidType;

	//Check if the class has already been defined
	ASObject* target;
	asAtom tmp=asAtomHandler::invalidAtom;
	wrk->getSystemState()->systemDomain->getVariableAndTargetByMultiname(tmp,*mn, target,wrk);
	if(asAtomHandler::isClass(tmp))
	{
		if (mn->isStatic)
			mn->cachedType = dynamic_cast<Class_base*>(asAtomHandler::getObjectNoCheck(tmp));
		return dynamic_cast<Class_base*>(asAtomHandler::getObjectNoCheck(tmp));
	}
	else
		return nullptr;
}

bool Type::coerceForTemplate(ASWorker* wrk, asAtom& o, bool allowconversion)
{
	switch (asAtomHandler::getObjectType(o))
	{
		case T_UNDEFINED:
			asAtomHandler::setNull(o);
			return true;
		case T_NULL:
			return true;
		case T_BOOLEAN:
			if(this == Class<Boolean>::getRef(wrk->getSystemState()).getPtr())
				return true;
			break;
		case T_UINTEGER:
			if(this == Class<UInteger>::getRef(wrk->getSystemState()).getPtr())
				return true;
			break;
		case T_INTEGER:
			if(this == Class<Integer>::getRef(wrk->getSystemState()).getPtr())
				return true;
			break;
		case T_NUMBER:
			if(this == Class<Number>::getRef(wrk->getSystemState()).getPtr())
				return true;
			break;
		case T_STRING:
			if(this == Class<ASString>::getRef(wrk->getSystemState()).getPtr())
				return true;
			break;
		default:
			break;
	}
	if(this == Class<UInteger>::getRef(wrk->getSystemState()).getPtr())
	{
		asAtomHandler::setUInt(o,wrk,asAtomHandler::toUInt(o));
		return true;
	}
	if(this == Class<Integer>::getRef(wrk->getSystemState()).getPtr())
	{
		asAtomHandler::setInt(o,wrk,asAtomHandler::toInt(o));
		return true;
	}
	if(this == Class<Number>::getRef(wrk->getSystemState()).getPtr())
	{
		asAtomHandler::setNumber(o,wrk,asAtomHandler::toNumber(o));
		return true;
	}
	if(allowconversion && this == Class<ASString>::getRef(wrk->getSystemState()).getPtr())
	{
		o = asAtomHandler::fromObject(abstract_s(wrk,asAtomHandler::toString(o,wrk)));
		return true;
	}
	if(this == Class<Boolean>::getRef(wrk->getSystemState()).getPtr())
	{
		asAtomHandler::setBool(o,asAtomHandler::isString(o) || asAtomHandler::toInt(o));
		return true;
	}
	if (asAtomHandler::getObject(o) && asAtomHandler::getObject(o)->is<ObjectConstructor>())
		return true;
	return false;
}

/*
 * This should only be called after all global objects have been created
 * by running ABCContext::exec() for all ABCContexts.
 * Therefore, all classes are at least declared.
 */
Type* Type::getTypeFromMultiname(multiname* mn, ABCContext* context, bool opportunistic)
{
	if(mn == 0) //multiname idx zero indicates any type
		return Type::anyType;

	if(mn->isStatic && mn->cachedType)
		return mn->cachedType;

	if(mn->name_type == multiname::NAME_STRING && mn->name_s_id==BUILTIN_STRINGS::ANY
		&& mn->ns.size() == 1 && mn->hasEmptyNS)
		return Type::anyType;

	if(mn->name_type == multiname::NAME_STRING && mn->name_s_id==BUILTIN_STRINGS::VOID
		&& mn->ns.size() == 1 && mn->hasEmptyNS)
		return Type::voidType;

	ASObject* typeObject = nullptr;
	/*
	 * During the newClass opcode, the class is added to context->root->applicationDomain->classesBeingDefined.
	 * The class variable in the global scope is only set a bit later.
	 * When the class has to be resolved in between (for example, the
	 * class has traits of the class's type), then we'll find it in
	 * classesBeingDefined, but context->root->getVariableAndTargetByMultiname()
	 * would still return "Undefined".
	 */
	auto i = context->applicationDomain->classesBeingDefined.find(mn);
	if(i != context->applicationDomain->classesBeingDefined.end())
		typeObject = i->second;
	else
	{
		if (opportunistic)
			typeObject = context->applicationDomain->getVariableByMultinameOpportunistic(*mn,context->applicationDomain->getInstanceWorker());
		else
		{
			ASObject* target;
			asAtom o=asAtomHandler::invalidAtom;
			context->applicationDomain->getVariableAndTargetByMultiname(o,*mn,target,context->applicationDomain->getInstanceWorker());
			if (asAtomHandler::isValid(o))
				typeObject=asAtomHandler::toObject(o,context->applicationDomain->getInstanceWorker());
		}
	}
	if(!typeObject)
	{
		if (mn->ns.size() >= 1 && mn->ns[0].nsNameId == BUILTIN_STRINGS::STRING_AS3VECTOR)
		{
			if (mn->templateinstancenames.size() == 1)
			{
				Type* instancetype = getTypeFromMultiname(mn->templateinstancenames.front(),context,opportunistic);
				if (instancetype==nullptr)
				{
					multiname* mti = mn->templateinstancenames.front();
					auto it = context->applicationDomain->classesBeingDefined.begin();
					while(it != context->applicationDomain->classesBeingDefined.end())
					{
						const multiname* m = it->first;
						if (mti->name_type == multiname::NAME_STRING && m->name_type == multiname::NAME_STRING
								&& mti->name_s_id == m->name_s_id
								&& mti->ns == m->ns)
						{
							instancetype = it->second;
							break;
						}
						it++;
					}
				}
				if (instancetype)
					typeObject = Template<Vector>::getTemplateInstance(instancetype,context->applicationDomain).getPtr();
			}
			else
			{
				LOG(LOG_NOT_IMPLEMENTED,"getTypeFromMultiname with "<<mn->templateinstancenames.size()<<" instance types");
				QName qname(mn->name_s_id,mn->ns[0].nsNameId);
				typeObject = Template<Vector>::getTemplateInstance(qname,context,context->applicationDomain).getPtr();
			}
		}
	}
	if (typeObject && typeObject->is<Class_base>())
		mn->cachedType = typeObject->as<Type>();
	return typeObject ? typeObject->as<Type>() : nullptr;
}

Class_base::Class_base(const QName& name, uint32_t _classID, MemoryAccount* m):ASObject(getSys()->worker,Class_object::getClass(getSys()),T_CLASS),protected_ns(getSys(),"",NAMESPACE),constructor(nullptr),
	qualifiedClassnameID(UINT32_MAX),global(nullptr),
	context(nullptr),class_name(name),memoryAccount(m),length(1),class_index(-1),
	isFinal(false),isSealed(false),isInterface(false),
	isReusable(false),use_protected(false),classID(_classID)
{
	setSystemState(getSys());
	setRefConstant();
}

Class_base::Class_base(const Class_object* c):ASObject((MemoryAccount*)nullptr),protected_ns(getSys(),BUILTIN_STRINGS::EMPTY,NAMESPACE),constructor(nullptr),
	qualifiedClassnameID(UINT32_MAX),global(nullptr),
	context(nullptr),class_name(BUILTIN_STRINGS::STRING_CLASS,BUILTIN_STRINGS::EMPTY),memoryAccount(nullptr),length(1),class_index(-1),isFinal(false),isSealed(false),isInterface(false),isReusable(false),use_protected(false),classID(UINT32_MAX)
{
	type=T_CLASS;
	//We have tested that (Class is Class == true) so the classdef is 'this'
	setClass(this);
	//The super is Class<ASObject> but we set it in SystemState constructor to avoid an infinite loop

	setSystemState(getSys());
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
void Class_base::copyBorrowedTraits(Class_base* src)
{
	//assert(borrowedVariables.Variables.empty());
	variables_map::var_iterator i = src->borrowedVariables.Variables.begin();
	for(;i != src->borrowedVariables.Variables.end(); ++i)
	{
		variable& v = i->second;
		variable* existingvar = borrowedVariables.findObjVar(i->first,v.ns,TRAIT_KIND::NO_CREATE_TRAIT,TRAIT_KIND::DECLARED_TRAIT);
		if (existingvar)
		{
			// variable is already overwritten in this class, but it may be that only getter or setter was overridden, so we check them
			if (asAtomHandler::isInvalid(existingvar->getter))
			{
				existingvar->getter = v.getter;
				ASATOM_INCREF(v.getter);
			}
			if (asAtomHandler::isInvalid(existingvar->setter))
			{
				existingvar->setter = v.setter;
				ASATOM_INCREF(v.setter);
			}
			setupBorrowedSlot(existingvar);
			continue;
		}
		v.issealed = src->isSealed;
		ASObject* var = v.getObjectVar();
		if (var)
			var->incRef();
		ASATOM_INCREF(v.getter);
		ASATOM_INCREF(v.setter);
		borrowedVariables.Variables.insert(make_pair(i->first,v));
		setupBorrowedSlot(&v);
	}
}

void Class_base::initStandardProps()
{
	constructorprop = _NR<ObjectConstructor>(new_objectConstructor(this,0));
	addConstructorGetter();

	prototype->setVariableByQName("constructor","",this,DECLARED_TRAIT);
	if(super)
		prototype->prevPrototype=super->prototype;
	addPrototypeGetter();
	addLengthConstant();
}

void Class_base::AVM1initPrototype()
{
	prototype->setVariableByQName("call","",getSystemState()->getBuiltinFunction(IFunction::_call,1),DYNAMIC_TRAIT);
	prototype->setVariableByQName("apply","",getSystemState()->getBuiltinFunction(IFunction::apply,2),DYNAMIC_TRAIT);
}

bool Class_base::coerce(ASWorker* wrk, asAtom& o)
{
	switch (asAtomHandler::getObjectType(o))
	{
		case T_UNDEFINED:
			asAtomHandler::setNull(o);
			return true;
		case T_NULL:
			return false;
		case T_INTEGER:
		case T_UINTEGER:
		case T_NUMBER:
			// int/uint/number are interchangeable
			if(this == Class<Number>::getRef(getSystemState()).getPtr() || this == Class<UInteger>::getRef(getSystemState()).getPtr() || this == Class<Integer>::getRef(getSystemState()).getPtr())
				return false;
			break;
		default:
			break;
	}
	if(this ==getSystemState()->getObjectClassRef())
		return false;
	if(asAtomHandler::isClass(o))
	{ /* classes can be cast to the type 'Object' or 'Class' */
		if(this == getSystemState()->getObjectClassRef()
		|| (class_name.nameId==BUILTIN_STRINGS::STRING_CLASS && class_name.nsStringId==BUILTIN_STRINGS::EMPTY))
			return false; /* 'this' is the type of a class */
		else
		{
			createError<TypeError>(wrk,kCheckTypeFailedError, o, getQualifiedClassName());
			return false;
		}
	}
	if (asAtomHandler::getObject(o) && asAtomHandler::getObject(o)->is<ObjectConstructor>())
		return false;

	//o->getClass() == NULL for primitive types
	//those are handled in overloads Class<Number>::coerce etc.
	if(!asAtomHandler::getObject(o) ||  !asAtomHandler::getObject(o)->getClass() || !asAtomHandler::getObject(o)->getClass()->isSubClass(this))
		createError<TypeError>(wrk,kCheckTypeFailedError, o, getQualifiedClassName());
	return false;
}

void Class_base::setSuper(_R<Class_base> super_)
{
	assert(!super);
	super = super_;
	copyBorrowedTraits(super.getPtr());
}

void Class_base::addConstructorGetter()
{
	setDeclaredMethodByQName("constructor","",getSystemState()->getBuiltinFunction(_getter_constructorprop),GETTER_METHOD,false);
}

void Class_base::addPrototypeGetter()
{
	setDeclaredMethodByQName("prototype","",getSystemState()->getBuiltinFunction(_getter_prototype,0,Class<ASObject>::getRef(getSystemState()).getPtr()),GETTER_METHOD,false);
}

void Class_base::addLengthConstant()
{
	if (length >=0)
		setVariableAtomByQName("length",nsNameAndKind(getSystemState(),BUILTIN_STRINGS::STRING_AS3NS,NAMESPACE),asAtomHandler::fromInt(length),CONSTANT_TRAIT);
}

Class_base::~Class_base()
{
}

void Class_base::_getter_constructorprop(asAtom& ret, ASWorker* wrk, asAtom& obj, asAtom* args, const unsigned int argslen)
{
	Class_base* th = nullptr;
	if(asAtomHandler::is<Class_base>(obj))
		th = asAtomHandler::as<Class_base>(obj);
	else
		th = asAtomHandler::getObject(obj)->getClass();
	if(argslen != 0)
	{
		createError<ArgumentError>(wrk,0,"Arguments provided in getter");
		return;
	}
	ASObject* res=th->constructorprop.getPtr();
	res->incRef();
	ret = asAtomHandler::fromObject(res);
}

Prototype* Class_base::getPrototype(ASWorker* wrk) const
{
	assert(wrk==getWorker() && !wrk->inFinalization());
	// workers need their own prototype objects for every class
	if (prototype.isNull() || this->is<Class_inherit>() || wrk->isPrimordial)
		return prototype.getPtr();
	else
		return wrk->getClassPrototype(this);
}


void Class_base::_getter_prototype(asAtom& ret, ASWorker* wrk,asAtom& obj, asAtom* args, const unsigned int argslen)
{
	if(!asAtomHandler::is<Class_base>(obj))
	{
		createError<ArgumentError>(wrk,0,"Function applied to wrong object");
		return;
	}
	Class_base* th = asAtomHandler::as<Class_base>(obj);
	if(argslen != 0)
	{
		createError<ArgumentError>(wrk,0,"Arguments provided in getter");
		return;
	}

	ret = asAtomHandler::fromObject(th->getPrototype(wrk)->getObj());
	ASATOM_INCREF(ret);
}

void Class_base::generator(ASWorker* wrk, asAtom& ret, asAtom* args, const unsigned int argslen)
{
	if (argslen==1)
	{
		ASObject* o = asAtomHandler::getObject(args[0]);
		if (o && o->getClass()->isSubClass(this))
		{
			ASATOM_INCREF(args[0]);
			ret = args[0];
		}
		else
			ret = asAtomHandler::nullAtom;
	}
	else
		createError<ArgumentError>(wrk,kCoerceArgumentCountError,Integer::toString(argslen));
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
	tiny_string ret="[class ";
	ret+=getSystemState()->getStringFromUniqueId(class_name.nameId);
	ret+="]";
	return ret;
}

void Class_base::setConstructor(ASObject* c)
{
	assert_and_throw(constructor==nullptr);
	if (c)
	{
		c->setRefConstant();
		assert_and_throw(c->is<IFunction>());
		constructor=c->as<IFunction>();
	}
}
void Class_base::handleConstruction(asAtom& target, asAtom* args, unsigned int argslen, bool buildAndLink, bool _explicit, bool callSyntheticConstructor)
{
	if (!asAtomHandler::isObject(target) || asAtomHandler::is<Class_base>(target))
		return;
	ASObject* t = asAtomHandler::getObjectNoCheck(target);
	if(buildAndLink)
	{
		setupDeclaredTraits(t);
	}

	if (asAtomHandler::isObject(target))
	{
		if (buildAndLink)
			asAtomHandler::getObjectNoCheck(target)->beforeConstruction(_explicit);
	}

	if(constructor && (callSyntheticConstructor ||
						(constructor->getMethodInfo() && constructor->getMethodInfo()->numArgs()-constructor->getMethodInfo()->numOptions()==0)))
	{
		LOG_CALL("handleConstruction for "<<asAtomHandler::toDebugString(target));
		t->incRef();
		asAtom ret=asAtomHandler::invalidAtom;
		asAtom c = asAtomHandler::fromObject(constructor);
		asAtomHandler::callFunction(c,asAtomHandler::getObjectNoCheck(target)->getInstanceWorker(),ret,target,args,argslen,true);
		t->constructIndicator = true;
		target = asAtomHandler::fromObject(asAtomHandler::getObjectNoCheck(target));
		LOG_CALL("handleConstruction done for "<<asAtomHandler::toDebugString(target));
	}
	else
	{
		t->constructIndicator = true;
		for(uint32_t i=0;i<argslen;i++)
			ASATOM_DECREF(args[i]);
	}
	if (asAtomHandler::isObject(target))
	{
		// Tell the object that the constructor of the builtin object has been called
		if (this->isBuiltin())
			asAtomHandler::getObjectNoCheck(target)->constructionComplete(_explicit);
		if(buildAndLink)
			asAtomHandler::getObjectNoCheck(target)->afterConstruction(_explicit);
	}
	else
		t->decRef();
}


void Class_base::finalize()
{
	borrowedVariables.destroyContents();
	super.reset();
	prototype.reset();
	interfaces_added.clear();
	protected_ns = nsNameAndKind(getSystemState(),"",NAMESPACE);
	ASObject* p =constructorprop.getPtr();
	constructorprop.reset();
	if (p)
		p->decRef();
	if(constructor)
		constructor=nullptr;
	context = nullptr;
	global = nullptr;
	length = 1;
	class_index = -1;
	isFinal = false;
	isSealed = false;
	isInterface = false;
	use_protected = false;
}
void Class_base::prepareShutdown()
{
	if (this->preparedforshutdown)
		return;
	this->isReusable=false;
	ASObject::prepareShutdown();
	borrowedVariables.prepareShutdown();
	if(constructor)
		constructor->prepareShutdown();
	constructor=nullptr;
	if (constructorprop)
		constructorprop->prepareShutdown();
	if (prototype && prototype->getObj())
		prototype->getObj()->prepareShutdown();
	if (super)
		super->prepareShutdown();
}
Template_base::Template_base(ASWorker* wrk,QName name) : ASObject(wrk,(Class_base*)(nullptr)),template_name(name)
{
	type = T_TEMPLATE;
}
void Template_base::addPrototypeGetter(SystemState* sys)
{
	this->setSystemState(sys);
	setDeclaredMethodByQName("prototype","",Class<IFunction>::getFunction(sys,_getter_prototype),GETTER_METHOD,false);
}
void Template_base::_getter_prototype(asAtom& ret, ASWorker* wrk,asAtom& obj, asAtom* args, const unsigned int argslen)
{
	if(!asAtomHandler::is<Template_base>(obj))
	{
		createError<ArgumentError>(wrk,0,"Function applied to wrong object");
		return;
	}
	Template_base* th = asAtomHandler::as<Template_base>(obj);
	if(argslen != 0)
	{
		createError<ArgumentError>(wrk,0,"Arguments provided in getter");
		return;
	}
	ASObject* res=th->prototype->getObj();
	res->incRef();
	ret = asAtomHandler::fromObject(res);
}

Class_object* Class_object::getClass(SystemState *sys)
{
	//We check if we are registered already
	//if not we register ourselves (see also Class<T>::getClass)
	//Class object position in the map is hardwired to 0
	uint32_t classId=0;
	Class_object* ret=nullptr;
	Class_base** retAddr=&sys->builtinClasses[classId];
	if(*retAddr==nullptr)
	{
		//Create the class
		ret=new (sys->unaccountedMemory) Class_object();
		ret->setWorker(sys->worker);
		ret->setSystemState(sys);
		ret->incRef();
		*retAddr=ret;
	}
	else
		ret=static_cast<Class_object*>(*retAddr);

	return ret;
}

const std::vector<Class_base*>& Class_base::getInterfaces(bool *alldefined) const
{
	if (alldefined)
		*alldefined = true;
	if(!interfaces.empty())
	{
		//Recursively get interfaces implemented by this interface
		std::vector<multiname>::iterator it = interfaces.begin();
		while (it !=interfaces.end())
		{
			ASObject* target;
			asAtom interface_obj=asAtomHandler::invalidAtom;
			this->context->applicationDomain->getVariableAndTargetByMultiname(interface_obj,*it, target,context->applicationDomain->getInstanceWorker());
			if (asAtomHandler::isClass(interface_obj))
			{
				Class_base* inter=static_cast<Class_base*>(asAtomHandler::getObject(interface_obj));
				//Probe the interface for its interfaces
				bool bAllDefinedSub;
				inter->getInterfaces(&bAllDefinedSub);

				if (bAllDefinedSub)
				{
					interfaces_added.push_back(inter);
					interfaces.erase(it);
					continue;
				}
				else if (alldefined)
					*alldefined = false;
			}
			else if (alldefined)
				*alldefined = false;
			it++;
		}
	}
	return interfaces_added;
}

void Class_base::linkInterface(Class_base* c) const
{
	assert(class_index!=-1);
	//Recursively link interfaces implemented by this interface
	const std::vector<Class_base*> interfaces = getInterfaces();
	for(unsigned int i=0;i<interfaces.size();i++)
		interfaces[i]->linkInterface(c);

	assert_and_throw(context);

	//Link traits of this interface
	for(unsigned int j=0;j<context->instances[class_index].trait_count;j++)
	{
		traits_info* t=&context->instances[class_index].traits[j];
		context->linkTrait(c,t);
	}

	if(constructor)
	{
		LOG_CALL("Calling interface init for " << class_name);
		asAtom v = asAtomHandler::fromObject(c);
		asAtom ret=asAtomHandler::invalidAtom;
		asAtom constr = asAtomHandler::fromObject(constructor);
		asAtomHandler::callFunction(constr,context->applicationDomain->getInstanceWorker(),ret,v,nullptr,0,false);
		assert_and_throw(asAtomHandler::isInvalid(ret));
	}
}

bool Class_base::isSubClass(Class_base* cls, bool considerInterfaces)
{
	if(cls==this || cls==cls->getSystemState()->getObjectClassRef())
		return true;
	auto it = subclasses_map.find(cls);
	if (it != subclasses_map.end())
		return (*it).second;

	//Now check the interfaces
	//an interface can't be subclass of a normal class, we only check the interfaces if cls is an interface itself
	if (considerInterfaces && cls->isInterface)
	{
		const std::vector<Class_base*> interfaces = getInterfaces();
		for(unsigned int i=0;i<interfaces.size();i++)
		{
			if(interfaces[i]->isSubClass(cls, considerInterfaces))
			{
				subclasses_map.insert(make_pair(cls,true));
				return true;
			}
		}
	}

	//Now ask the super
	if(super && super->isSubClass(cls, considerInterfaces))
	{
		subclasses_map.insert(make_pair(cls,true));
		return true;
	}
	if (cls->isConstructed() && this->isConstructed())
		subclasses_map.insert(make_pair(cls,false));
	return false;
}

const tiny_string Class_base::getQualifiedClassName(bool fullName) const
{
	if (qualifiedClassnameID != UINT32_MAX && fullName)
		return getSystemState()->getStringFromUniqueId(qualifiedClassnameID);
	if(class_index==-1)
		return class_name.getQualifiedName(getSystemState(),fullName);
	else
	{
		assert_and_throw(context);
		int name_index=context->instances[class_index].name;
		assert_and_throw(name_index);
		const multiname* mname=context->getMultiname(name_index,nullptr);
		return mname->qualifiedString(getSystemState(),fullName);
	}
}
uint32_t Class_base::getQualifiedClassNameID()
{
	if (qualifiedClassnameID == UINT32_MAX)
	{
		if(class_index==-1)
		{
			qualifiedClassnameID = getSystemState()->getUniqueStringId(class_name.getQualifiedName(getSystemState(),true));
		}
		else
		{
			assert_and_throw(context);
			int name_index=context->instances[class_index].name;
			assert_and_throw(name_index);
			const multiname* mname=context->getMultiname(name_index,nullptr);
			qualifiedClassnameID=getSystemState()->getUniqueStringId(mname->qualifiedString(getSystemState(),true));
		}
	}
	return qualifiedClassnameID;
}

tiny_string Class_base::getName(bool dotnotation) const
{
	return (class_name.nsStringId == BUILTIN_STRINGS::EMPTY ?
				this->getSystemState()->getStringFromUniqueId(class_name.nameId)
			  : this->getSystemState()->getStringFromUniqueId(class_name.nsStringId) +(dotnotation ? "." : "$")+ this->getSystemState()->getStringFromUniqueId(class_name.nameId));
}

ASObject *Class_base::describeType(ASWorker* wrk) const
{
	pugi::xml_document p;
	pugi::xml_node root = p.append_child("type");

	root.append_attribute("name").set_value(getQualifiedClassName(true).raw_buf());
	root.append_attribute("base").set_value("Class");
	root.append_attribute("isDynamic").set_value("true");
	root.append_attribute("isFinal").set_value("true");
	root.append_attribute("isStatic").set_value("true");

	// extendsClass
	pugi::xml_node node=root.append_child("extendsClass");
	node.append_attribute("type").set_value("Class");
	node=root.append_child("extendsClass");
	node.append_attribute("type").set_value("Object");

	// prototype
	pugi::xml_node prototypenode=root.append_child("accessor");
	prototypenode.append_attribute("name").set_value("prototype");
	prototypenode.append_attribute("access").set_value("readonly");
	prototypenode.append_attribute("type").set_value("*");
	prototypenode.append_attribute("declaredBy").set_value("Class");

	std::map<varName,pugi::xml_node> propnames;
	// variable
	if(class_index>=0)
	{
		describeTraits(root, context->classes[class_index].traits,propnames,true,false,false);
	}
	else
	{
		// builtin class
		describeVariables(root,this,propnames,Variables,false,false,false);
	}


	// factory
	node=root.append_child("factory");
	node.append_attribute("type").set_value(getQualifiedClassName(true).raw_buf());
	describeInstance(node,false,false,true);
	return XML::createFromNode(wrk,root);
}

void Class_base::describeInstance(pugi::xml_node& root, bool istemplate, bool forinstance, bool forfactory, bool forJSON) const
{
	if (forfactory && this==Class_object::getRef(getSystemState()).getPtr())
	{
		// prototype
		pugi::xml_node prototypenode=root.append_child("accessor");
		prototypenode.append_attribute("name").set_value("prototype");
		prototypenode.append_attribute("access").set_value("readonly");
		prototypenode.append_attribute("type").set_value("*");
		prototypenode.append_attribute("declaredBy").set_value("Class");
	}

	describeConstructor(root);
	// extendsClass
	const Class_base* c=super.getPtr();
	while(c)
	{
		pugi::xml_node node=root.append_child("extendsClass");
		node.append_attribute("type").set_value(c->getQualifiedClassName(true).raw_buf());
		c=c->super.getPtr();
		if (istemplate)
			break;
	}
	this->describeClassMetadata(root);

	// implementsInterface
	std::set<Class_base*> allinterfaces;
	c=this;
	while(c && c->class_index>=0)
	{
		const std::vector<Class_base*>& interfaces=c->getInterfaces();
		auto it=interfaces.begin();
		for(; it!=interfaces.end(); ++it)
		{
			if (allinterfaces.find(*it)!= allinterfaces.end())
				continue;
			allinterfaces.insert(*it);
			pugi::xml_node node=root.append_child("implementsInterface");
			node.append_attribute("type").set_value((*it)->getQualifiedClassName(true).raw_buf());
		}
		c=c->super.getPtr();
	}

	// variables, methods, accessors
	std::map<varName,pugi::xml_node> propnames;
	c=this;
	bool bfirst = true;
	while(c && c->class_index>=0)
	{
		c->describeTraits(root, c->context->instances[c->class_index].traits,propnames,bfirst,forinstance,forfactory);
		bfirst = false;
		c=c->super.getPtr();
		if (istemplate)
			break;
	}
	if ((this->isBuiltin() || forJSON) && c && c->class_index<0 && !forfactory)
	{
		// builtin class
		while(c)
		{
			describeVariables(root,c,propnames,forinstance || !bfirst ? c->borrowedVariables : c->Variables,istemplate,forinstance, forJSON);
			bfirst = false;
			c = c->super.getPtr();
		}
	}

}

void Class_base::describeVariables(pugi::xml_node& root, const Class_base* c, std::map<varName,pugi::xml_node>& propnames, const variables_map& map, bool isTemplate,bool forinstance, bool forJSON) const
{
	variables_map::const_var_iterator it=map.Variables.cbegin();
	for(;it!=map.Variables.cend();++it)
	{
		const char* nodename;
		const char* access = nullptr;
		tiny_string type;
		tiny_string returnType;
		tiny_string declaredBy;
		if (!forJSON && forinstance && it->second.ns.nsNameId == BUILTIN_STRINGS::STRING_AS3NS)
			continue;
		IFunction* f=nullptr;
		switch (it->second.kind)
		{
			case CONSTANT_TRAIT:
				nodename = "constant";
				type = it->second.getClassVar(getSystemState())->getQualifiedClassName(true).raw_buf();
				break;
			case INSTANCE_TRAIT:
			case DECLARED_TRAIT:
				if (it->second.isValidVar())
				{
					if (isTemplate)
						continue;
					nodename="method";
					if (it->second.kind==INSTANCE_TRAIT)
						type = it->second.getClassVar(getSystemState())->getQualifiedClassName(true).raw_buf();
					else
					{
						f = it->second.getFunctionVar();
						if (f)
						{
							Type* rt = f->getReturnType();
							if (rt==Type::anyType)
								returnType="*";
							else if (rt==Type::voidType)
								returnType="void";
							else if (rt)
							{
								Class_base* c = dynamic_cast<Class_base*>(rt);
								if (c)
									returnType=c->getQualifiedClassName(true);
							}
						}
					}
				}
				else
				{
					nodename="accessor";
					if (asAtomHandler::isValid(it->second.getter) && asAtomHandler::isValid(it->second.setter))
						access = "readwrite";
					else if (asAtomHandler::isValid(it->second.getter))
						access = "readonly";
					else if (asAtomHandler::isValid(it->second.setter))
						access = "writeonly";
				}
				break;
			default:
				continue;
		}
		tiny_string name = getSystemState()->getStringFromUniqueId(it->first);
		if (name == "constructor" || name == "prototype")
			continue;
		varName vn(it->first,nsNameAndKind());
		auto existing=propnames.find(vn);
		if(existing != propnames.cend())
			continue;

		pugi::xml_node node=root.append_child(nodename);
		node.append_attribute("name").set_value(name.raw_buf());
		if (access)
			node.append_attribute("access").set_value(access);

		ASObject* obj = nullptr;
		if (asAtomHandler::isFunction(it->second.getter))
			obj = asAtomHandler::getObject(it->second.getter);
		else if (asAtomHandler::isFunction(it->second.setter))
			obj = asAtomHandler::getObject(it->second.setter);
		if (obj)
		{
			if (obj->is<SyntheticFunction>())
				node.append_attribute("type").set_value(obj->as<SyntheticFunction>()->getMethodInfo()->returnTypeName()->qualifiedString(getSystemState(),true).raw_buf());
			else if (obj->is<Function>())
			{
				if (obj->as<Function>()->returnType)
				{
					Class_base* c = dynamic_cast<Class_base*>(obj->as<Function>()->returnType);
					assert (c);
					node.append_attribute("type").set_value(c->getQualifiedClassName(true).raw_buf());
				}
				else
					LOG(LOG_NOT_IMPLEMENTED,"describeType: return type not known:"<<this->class_name<<"  property "<<name);
			}
		}
		if (it->second.kind != CONSTANT_TRAIT)
		{
			if (isTemplate)
				declaredBy="__AS3__.vec::Vector.<*>";
			else
				declaredBy=c->getQualifiedClassName(true);
			if (!it->second.ns.hasEmptyName())
				node.append_attribute("uri").set_value(getSystemState()->getStringFromUniqueId(it->second.ns.nsNameId).raw_buf());
		}
		if (!type.empty())
			node.append_attribute("type").set_value(type.raw_buf());
		if (!declaredBy.empty())
			node.append_attribute("declaredBy").set_value(declaredBy.raw_buf());
		if (!returnType.empty())
			node.append_attribute("returnType").set_value(returnType.raw_buf());
		if (f)
		{
			uint32_t startoptional = f->length-f->getParamOptionalCount();
			for (uint32_t i = 0; i < f->length; i++)
			{
				pugi::xml_node paramnode = node.append_child("parameter");
				paramnode.append_attribute("index").set_value(i+1);
				Type* tp = f->getParamType(i);
				if (tp)
				{
					tiny_string t;
					if (tp == Type::anyType)
						t="*";
					else if (tp == Type::voidType)
						t="void";
					else
						t=dynamic_cast<Class_base*>(tp)->getQualifiedClassName(true);
					paramnode.append_attribute("type").set_value(t.raw_buf());
					paramnode.append_attribute("optional").set_value(i >= startoptional);
				}
				else
				{
					LOG(LOG_NOT_IMPLEMENTED,"describeType: missing argument type:"<<i<<" "<<f->toDebugString());
					paramnode.append_attribute("type").set_value("*");
					paramnode.append_attribute("optional").set_value(true);
				}
			}
		}

		propnames[vn] = node;
	}
}
void Class_base::describeConstructor(pugi::xml_node &root) const
{
	if (!this->constructor)
		return;
	if (!this->constructor->is<SyntheticFunction>())
	{
		if (!this->getTemplate())
		{
			pugi::xml_node node=root.append_child("constructor");
			uint32_t startoptional = this->constructor->as<IFunction>()->length-this->constructor->as<IFunction>()->getParamOptionalCount();
			for (uint32_t i = 0; i < this->constructor->as<IFunction>()->length; i++)
			{
				pugi::xml_node paramnode = node.append_child("parameter");
				paramnode.append_attribute("index").set_value(i+1);
				Type* tp = this->constructor->as<IFunction>()->getParamType(i);
				if (tp)
				{
					tiny_string t;
					if (tp == Type::anyType)
						t="*";
					else if (tp == Type::voidType)
						t="void";
					else
						t=dynamic_cast<Class_base*>(tp)->getQualifiedClassName(true);
					paramnode.append_attribute("type").set_value(t.raw_buf());
					paramnode.append_attribute("optional").set_value(i >= startoptional);
				}
				else
				{
					LOG(LOG_NOT_IMPLEMENTED,"describeType: missing constructor argument type:"<<i<<" "<<this->toDebugString());
					paramnode.append_attribute("type").set_value("*");
					paramnode.append_attribute("optional").set_value(true);
				}
			}
		}
		return;
	}
	method_info* mi = this->constructor->getMethodInfo();
	if (mi->numArgs() == 0)
		return;
	pugi::xml_node node=root.append_child("constructor");

	for (uint32_t i = 0; i < mi->numArgs(); i++)
	{
		pugi::xml_node paramnode = node.append_child("parameter");
		paramnode.append_attribute("index").set_value(i+1);
		paramnode.append_attribute("type").set_value(mi->paramTypeName(i)->qualifiedString(getSystemState(),true).raw_buf());
		paramnode.append_attribute("optional").set_value(i >= mi->numArgs()-mi->numOptions());
	}
}

void Class_base::describeTraits(pugi::xml_node &root, std::vector<traits_info>& traits, std::map<varName,pugi::xml_node> &propnames, bool first, bool forinstance, bool forfactory) const
{
	std::map<u30, pugi::xml_node> accessorNodes;
	for(unsigned int i=0;i<traits.size();i++)
	{
		traits_info& t=traits[i];
		int kind=t.kind&0xf;
		multiname* mname=context->getMultiname(t.name,nullptr);
		if (mname->name_type!=multiname::NAME_STRING ||
			(mname->ns.size()==1 && (mname->ns[0].kind != NAMESPACE)) ||
			(!forinstance && forfactory
			  && (!first || (this->super && !this->super->isBuiltin()))
			  && mname->ns[0].nsNameId == BUILTIN_STRINGS::STRING_AS3NS) ||
			(forinstance
			  && (!first || (this->super && !this->super->isBuiltin()))
			  && mname->ns[0].nsNameId == BUILTIN_STRINGS::STRING_AS3NS) ||
			mname->ns.size() > 1)
			continue;
		pugi::xml_node node;
		varName vn(mname->name_s_id,mname->ns.size()==1 && first && !(kind==traits_info::Getter || kind==traits_info::Setter) ? mname->ns[0] : nsNameAndKind());
		auto existing=accessorNodes.find(t.name);
		auto existingprop = propnames.find(vn);
		if (existingprop != propnames.end())
		{
			if (!(kind==traits_info::Getter || kind==traits_info::Setter))
				continue;
		}

		if(kind==traits_info::Slot || kind==traits_info::Const)
		{
			multiname* type=context->getMultiname(t.type_name,nullptr);
			const char *nodename=kind==traits_info::Const?"constant":"variable";
			node=root.append_child(nodename);
			node.append_attribute("name").set_value(getSystemState()->getStringFromUniqueId(mname->name_s_id).raw_buf());
			node.append_attribute("type").set_value(type->qualifiedString(getSystemState(),true).raw_buf());
			if (mname->ns.size() > 0 && !mname->ns[0].hasEmptyName())
				node.append_attribute("uri").set_value(getSystemState()->getStringFromUniqueId(mname->ns[0].nsNameId).raw_buf());
			describeMetadata(node, t);
		}
		else if (kind==traits_info::Method)
		{
			node=root.append_child("method");
			node.append_attribute("name").set_value(getSystemState()->getStringFromUniqueId(mname->name_s_id).raw_buf());
			node.append_attribute("declaredBy").set_value(getQualifiedClassName(true).raw_buf());

			method_info& method=context->methods[t.method];
			const multiname* rtname=method.returnTypeName();
			node.append_attribute("returnType").set_value(rtname->qualifiedString(getSystemState(),true).raw_buf());
			if (mname->ns.size() > 0 && !mname->ns[0].hasEmptyName())
				node.append_attribute("uri").set_value(getSystemState()->getStringFromUniqueId(mname->ns[0].nsNameId).raw_buf());

			assert(method.numArgs() >= method.numOptions());
			uint32_t firstOpt=method.numArgs() - method.numOptions();
			for(uint32_t j=0;j<method.numArgs(); j++)
			{
				pugi::xml_node param=node.append_child("parameter");
				param.append_attribute("index").set_value(UInteger::toString(j+1).raw_buf());
				param.append_attribute("type").set_value(method.paramTypeName(j)->qualifiedString(getSystemState(),true).raw_buf());
				param.append_attribute("optional").set_value(j>=firstOpt?"true":"false");
			}

			describeMetadata(node, t);
		}
		else if (kind==traits_info::Getter || kind==traits_info::Setter)
		{
			const char* access=nullptr;
			// The getters and setters are separate
			// traits. Check if we have already created a
			// node for this multiname with the
			// complementary accessor. If we have, update
			// the access attribute to "readwrite".
			if(existing==accessorNodes.end())
			{
				if (existingprop == propnames.end())
				{
					node=root.append_child("accessor");
					node.append_attribute("name").set_value(getSystemState()->getStringFromUniqueId(mname->name_s_id).raw_buf());
				}
				else
				{
					node=existingprop->second;
					pugi::xml_attribute prevAttr=node.attribute("access");
					tiny_string prevAccess=prevAttr.value();
					if((kind==traits_info::Getter && prevAccess=="readonly") ||
						(kind==traits_info::Setter && prevAccess=="writeonly"))
						continue;
				}

			}
			else
				node=existing->second;

			pugi::xml_attribute oldAttr=node.attribute("access");
			tiny_string oldAccess=oldAttr.value();

			if(kind==traits_info::Getter && oldAccess=="")
				access="readonly";
			else if(kind==traits_info::Setter && oldAccess=="")
				access="writeonly";
			else if((kind==traits_info::Getter && oldAccess=="writeonly") ||
				(kind==traits_info::Setter && oldAccess=="readonly") ||
					 oldAccess=="readwrite")
				access="readwrite";

			node.remove_attribute("access");
			node.append_attribute("access").set_value(access);

			tiny_string type;
			method_info& method=context->methods[t.method];
			if(kind==traits_info::Getter)
			{
				const multiname* rtname=method.returnTypeName();
				type=rtname->qualifiedString(getSystemState(),true);
			}
			else if(method.numArgs()>0) // setter
			{
				type=method.paramTypeName(0)->qualifiedString(getSystemState(),true);
			}
			if(!type.empty())
			{
				node.remove_attribute("type");
				node.append_attribute("type").set_value(type.raw_buf());
			}
			tiny_string declaredby=node.attribute("declaredBy").value();
			if (declaredby.empty())
				declaredby = getQualifiedClassName(true).raw_buf();
			node.remove_attribute("declaredBy");
			node.append_attribute("declaredBy").set_value(declaredby.raw_buf());
			if (mname->ns.size() > 0 && !mname->ns[0].hasEmptyName())
			{
				node.remove_attribute("uri");
				node.append_attribute("uri").set_value(getSystemState()->getStringFromUniqueId(mname->ns[0].nsNameId).raw_buf());
			}
			if (oldAccess!="readwrite")
				describeMetadata(node, t);
			accessorNodes[t.name]=node;
		}
		propnames.insert(make_pair(vn,node));
	}
}

void Class_base::describeMetadata(pugi::xml_node& root, const traits_info& trait) const
{
	if((trait.kind&traits_info::Metadata) == 0)
		return;

	for(unsigned int i=0;i<trait.metadata_count;i++)
	{
		pugi::xml_node metadata_node=root.append_child("metadata");
		metadata_info& minfo = context->metadata[trait.metadata[i]];
		metadata_node.append_attribute("name").set_value(context->applicationDomain->getSystemState()->getStringFromUniqueId(context->getString(minfo.name)).raw_buf());

		for(unsigned int j=0;j<minfo.item_count;++j)
		{
			pugi::xml_node arg_node=metadata_node.append_child("arg");
			arg_node.append_attribute("key").set_value(context->applicationDomain->getSystemState()->getStringFromUniqueId(context->getString(minfo.items[j].key)).raw_buf());
			arg_node.append_attribute("value").set_value(context->applicationDomain->getSystemState()->getStringFromUniqueId(context->getString(minfo.items[j].value)).raw_buf());
		}
	}
}

void Class_base::initializeProtectedNamespace(uint32_t nameId, const namespace_info& ns, ApplicationDomain *appdomain)
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
		protected_ns=nsNameAndKind(getSystemState(),nameId,(NS_KIND)(int)ns.kind,appdomain);
	else
		protected_ns=nsNameAndKind(getSystemState(),nameId,baseNs->nsId,(NS_KIND)(int)ns.kind,appdomain);
}

variable* Class_base::findBorrowedSettable(const multiname& name, bool* has_getter)
{
	return ASObject::findSettableImpl(getInstanceWorker(),borrowedVariables,name,has_getter);
}

variable* Class_base::findSettableInPrototype(const multiname& name, bool* has_getter)
{
	Prototype* proto = prototype.getPtr();
	while(proto)
	{
		variable *obj = proto->getObj()->findSettable(name,has_getter);
		if (obj)
			return obj;

		proto = proto->prevPrototype.getPtr();
	}

	return nullptr;
}

EARLY_BIND_STATUS Class_base::resolveMultinameStatically(const multiname& name) const
{
	if(findBorrowedGettable(name)!=NULL)
		return BINDED;
	else
		return NOT_BINDED;
}

bool Class_base::checkExistingFunction(const multiname &name)
{
	variable* v = Variables.findObjVar(getInstanceWorker(),name, DECLARED_TRAIT);
	if (!v)
		v = borrowedVariables.findObjVar(getInstanceWorker(),name, DECLARED_TRAIT);
	if (v && v->isValidVar())
		return this->isSealed;
	else if (!this->isBuiltin())
	{
		// TODO check traits directly instead of constructing a new object
		asAtom otmp = asAtomHandler::invalidAtom;
		getInstance(this->getInstanceWorker(),otmp,false,nullptr,0);
		setupDeclaredTraits(asAtomHandler::getObject(otmp),false);
		v = asAtomHandler::getObject(otmp)->findVariableByMultiname(name,nullptr,nullptr,nullptr,false,this->getInstanceWorker());
		if (v && v->isValidVar())
		{
			ASATOM_DECREF(otmp);
			return this->isSealed;
		}
		ASATOM_DECREF(otmp);
	}
	if (!super.isNull())
		return super->checkExistingFunction(name);
	return false;
}

multiname* Class_base::getClassVariableByMultiname(asAtom& ret, const multiname &name, ASWorker* wrk, asAtom& closure, uint16_t resultlocalnumberpos)
{
	uint32_t nsRealId;
	variable* obj = ASObject::findGettableImpl(getInstanceWorker(), borrowedVariables,name,&nsRealId);
	if(!obj && name.hasEmptyNS)
	{
		//Check prototype chain
		Prototype* proto = prototype.getPtr();
		while(proto)
		{
			obj=proto->getObj()->Variables.findObjVar(getInstanceWorker(),name,DECLARED_TRAIT|DYNAMIC_TRAIT,&nsRealId);
			if(obj)
			{
				//It seems valid for a class to redefine only the setter, so if we can't find
				//something to get, it's ok
				if(!(asAtomHandler::isValid(obj->getter) || obj->isValidVar()))
					obj=nullptr;
			}
			if(obj)
				break;
			proto = proto->prevPrototype.getPtr();
		}
	}
	if(!obj)
		return nullptr;


	if (obj->kind == INSTANCE_TRAIT)
	{
		if (getSystemState()->getNamespaceFromUniqueId(nsRealId).kind != STATIC_PROTECTED_NAMESPACE)
		{
			createError<TypeError>(wrk,kCallOfNonFunctionError,name.normalizedNameUnresolved(getSystemState()));
			return nullptr;
		}
	}

	if(asAtomHandler::isValid(obj->getter))
	{
		//Call the getter
		LOG_CALL("Calling the getter for " << name << " on " << asAtomHandler::toDebugString(obj->getter));
		assert(asAtomHandler::isFunction(obj->getter));
		asAtom closureAtom = asAtomHandler::getClosureAtom(obj->getter,asAtomHandler::isValid(closure) ? closure : asAtomHandler::fromObject(this));
		multiname* simplegetter = asAtomHandler::as<IFunction>(obj->getter)->callGetter(ret,closureAtom,wrk,resultlocalnumberpos);
		LOG_CALL("End of getter"<< ' ' << asAtomHandler::toDebugString(obj->getter)<<" result:"<<asAtomHandler::toDebugString(ret));
		return simplegetter;
	}
	else
	{
		assert_and_throw(asAtomHandler::isInvalid(obj->setter));
		ASObject* func = obj->getObjectVar();
		if (func)
			func->incRef();
		if(func && func->is<IFunction>() && func->as<IFunction>()->isMethod())
		{
			if (asAtomHandler::isValid(func->as<IFunction>()->closure_this))
			{
				LOG_CALL("class function " << name << " is already bound to "<<func->toDebugString() );
				ret = asAtomHandler::fromObjectNoPrimitive(func);
			}
			else
			{
				LOG_CALL("Attaching this class " << this->toDebugString() << " to function " << name << " "<<func->toDebugString());
				asAtomHandler::setFunction(ret,func,closure,wrk);
			}
		}
		else
			ret = obj->getVar(getInstanceWorker());
	}
	return nullptr;
}

bool Class_base::isInterfaceMethod(const multiname& name)
{
	multiname m(nullptr);
	m.name_type = multiname::NAME_STRING;
	m.name_s_id = name.name_s_id;
	const std::vector<Class_base*> interfaces = getInterfaces();
	for(unsigned int i=0;i<interfaces.size();i++)
	{
		variable* v = interfaces[i]->borrowedVariables.findObjVar(getInstanceWorker(),m,DECLARED_TRAIT);
		if (v)
			return true;
	}
	return false;
}

void Class_base::removeAllDeclaredProperties()
{
	Variables.removeAllDeclaredProperties();
	borrowedVariables.removeAllDeclaredProperties();
}

void Class_base::setupBorrowedSlot(variable* var)
{
	uint32_t slotID = findBorrowedSlotID(var);
	if (slotID==UINT32_MAX)
	{
		slotID=borrowedVariableSlots.size();
		QName nm(var->nameStringID,var->ns.nsNameId);
		borrowedVariableNameSlotMap[nm]=slotID;
		borrowedVariableSlots.push_back(var);
	}
	else
	{
		if (borrowedVariableSlots.size() <= slotID)
			borrowedVariableSlots.resize(slotID+1);
		borrowedVariableSlots[slotID]=var;
	}
}

uint32_t Class_base::findBorrowedSlotID(variable* var)
{
	QName nm(var->nameStringID,var->ns.nsNameId);
	auto it = borrowedVariableNameSlotMap.find(nm);
	if (it != borrowedVariableNameSlotMap.end())
		return (*it).second;
	if (super)
		return super->findBorrowedSlotID(var);
	return UINT32_MAX;
}

multiname* Class_base::setVariableByMultiname(multiname& name, asAtom& o, CONST_ALLOWED_FLAG allowConst, bool* alreadyset, ASWorker* wrk)
{
	Prototype* pr = this->getPrototype(wrk);
	ASObject* dynvars =pr ? pr->getWorkerDynamicClassVars() : nullptr;
	if (dynvars)
	{
		if (ASObject::hasPropertyByMultiname(name,false,false,wrk))
			return setVariableByMultiname_intern(name,o,allowConst,this->getClass(),alreadyset,wrk);
		else
			return dynvars->setVariableByMultiname(name,o,allowConst,alreadyset,wrk);
	}
	return ASObject::setVariableByMultiname(name,o,allowConst,alreadyset,wrk);
}

GET_VARIABLE_RESULT Class_base::getVariableByMultiname(asAtom& ret, const multiname& name, GET_VARIABLE_OPTION opt, ASWorker* wrk)
{
	Prototype* pr = this->getPrototype(wrk);
	ASObject* dynvars =pr ? pr->getWorkerDynamicClassVars() : nullptr;
	if (dynvars == nullptr)
		return ASObject::getVariableByMultiname(ret,name,opt,wrk);
	GET_VARIABLE_RESULT res = dynvars->getVariableByMultiname(ret,name,opt,wrk);
	if (asAtomHandler::isValid(ret))
		return res;
	return ASObject::getVariableByMultiname(ret,name,opt,wrk);
}

bool Class_base::hasPropertyByMultiname(const multiname& name, bool considerDynamic, bool considerPrototype,ASWorker* wrk)
{
	Prototype* pr = this->getPrototype(wrk);
	ASObject* dynvars =pr ? pr->getWorkerDynamicClassVars() : nullptr;
	if (dynvars && considerDynamic && dynvars->hasPropertyByMultiname(name,considerDynamic, false,wrk))
		return true;
	return ASObject::hasPropertyByMultiname(name,considerDynamic, considerPrototype,wrk);
}

variable* Class_base::findVariableByMultiname(const multiname& name, Class_base* cls, uint32_t* nsRealID, bool* isborrowed, bool considerdynamic, ASWorker* wrk)
{
	variable* v=nullptr;
	Prototype* pr = this->getPrototype(wrk);
	ASObject* dynvars =pr ? pr->getWorkerDynamicClassVars() : nullptr;
	if (dynvars)
		v = dynvars->findVariableByMultiname(name,cls,nsRealID,isborrowed,true,wrk);
	if (!v)
		v = ASObject::findVariableByMultiname(name,cls,nsRealID,isborrowed,considerdynamic,wrk);
	return v;
}


EARLY_BIND_STATUS ActivationType::resolveMultinameStatically(const multiname& name) const
{
	std::cerr << "Looking for " << name << std::endl;
	for(unsigned int i=0;i<mi->body->trait_count;i++)
	{
		const traits_info* t=&mi->body->traits[i];
		multiname* mname=mi->context->getMultiname(t->name,nullptr);
		std::cerr << "\t in " << *mname << std::endl;
		assert_and_throw(mname->ns.size()==1 && mname->name_type==multiname::NAME_STRING);
		if(mname->name_s_id!=name.normalizedNameId(mi->context->applicationDomain->getInstanceWorker()))
			continue;
		bool found=false;
		for (auto it = name.ns.begin(); it != name.ns.end(); it++)
		{
			if ((*it)==mname->ns[0])
			{
				found = true;
				break;
			}
		}
		if(!found)
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

		multiname* tname=mi->context->getMultiname(t->type_name,nullptr);
		return tname;
	}
	return nullptr;
}

void Prototype::setVariableByQName(const tiny_string &name, const tiny_string &ns, ASObject *o, TRAIT_KIND traitKind, uint8_t min_swfversion)
{
	if (o->is<Function>())
		o->as<Function>()->setRefConstant();
	obj->setVariableByQName(name,ns,o,traitKind,false,min_swfversion);
	originalPrototypeVars->setVariableByQName(name,ns,o,traitKind,false);
}
void Prototype::setVariableByQName(const tiny_string &name, const nsNameAndKind &ns, ASObject *o, TRAIT_KIND traitKind, uint8_t min_swfversion)
{
	if (o->is<Function>())
		o->as<Function>()->setRefConstant();
	uint32_t nameID = obj->getSystemState()->getUniqueStringId(name);
	obj->setVariableByQName(nameID,ns,o,traitKind,false,min_swfversion);
	originalPrototypeVars->setVariableByQName(nameID,ns,o,traitKind,false);
}
void Prototype::setVariableByQName(uint32_t nameID, const nsNameAndKind &ns, ASObject *o, TRAIT_KIND traitKind, uint8_t min_swfversion)
{
	if (o->is<Function>())
		o->as<Function>()->setRefConstant();
	obj->setVariableByQName(nameID,ns,o,traitKind,false,min_swfversion);
	originalPrototypeVars->setVariableByQName(nameID,ns,o,traitKind,false);
}

void Prototype::setVariableAtomByQName(const tiny_string &name, const nsNameAndKind &ns, asAtom o, TRAIT_KIND traitKind, uint8_t min_swfversion)
{
	if (asAtomHandler::is<Function>(o))
		asAtomHandler::as<Function>(o)->setRefConstant();
	uint32_t nameID = obj->getSystemState()->getUniqueStringId(name);
	obj->setVariableAtomByQName(nameID,ns,o,traitKind,min_swfversion);
	originalPrototypeVars->setVariableAtomByQName(nameID,ns,o,traitKind);
}

void Prototype::setDeclaredMethodByQName(const tiny_string& name, const tiny_string& ns, ASObject* o, METHOD_TYPE type, bool isBorrowed, bool isEnumerable, uint8_t min_swfversion)
{
	obj->setDeclaredMethodByQName(name, ns, o, type, isBorrowed, isEnumerable,min_swfversion);
	o->setRefConstant();
}
void Prototype::copyOriginalValues(Prototype* target)
{
	if (this->prevPrototype)
		this->prevPrototype->copyOriginalValues(target);
	originalPrototypeVars->copyValuesForPrototype(target->getObj(),target->getObj()->getInstanceWorker());
	if (!target->workerDynamicClassVars)
		target->workerDynamicClassVars = new_asobject(target->getObj()->getInstanceWorker());
	target->getObj()->setRefConstant();
}

