/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2011-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef SCRIPTING_TOPLEVEL_VECTOR_H
#define SCRIPTING_TOPLEVEL_VECTOR_H 1

#include "scripting/flash/system/flashsystem.h"
#include "scripting/flash/display/RootMovieClip.h"
#include "class.h"

namespace lightspark
{

/* This is a class which was instantiated from a Template<T> */
template<class T>
class TemplatedClass : public Class<T>
{
private:
	/* the Template<T>* this class was generated from */
	const Template_base* templ;
	std::vector<Type*> types;
	asfreelist freelist;
public:
	TemplatedClass(const QName& name, const std::vector<Type*>& _types, Template_base* _templ, MemoryAccount* m)
		: Class<T>(name,ClassName<T>::id, m), templ(_templ), types(_types)
	{
	}
	asfreelist* getFreeList(ASWorker*) override
	{
		return &freelist;
	}

	void getInstance(ASWorker* wrk,asAtom& ret, bool construct, asAtom* args,
			const unsigned int argslen, Class_base* realClass=nullptr) override
	{
		if(realClass==nullptr)
			realClass=this;
		ret = asAtomHandler::fromObject(realClass->getFreeList(wrk)->getObjectFromFreeList());
		if (asAtomHandler::isInvalid(ret))
			ret=asAtomHandler::fromObject(new (realClass->memoryAccount) T(wrk,realClass));
		asAtomHandler::resetCached(ret);
		asAtomHandler::as<T>(ret)->setTypes(types);
		if(construct)
			this->handleConstruction(ret,args,argslen,true);
	}

	/* This function is called for as3 code like v = Vector.<String>(["Hello", "World"])
	 * this->types will be Class<ASString> on entry of this function.
	 */
	void generator(ASWorker* wrk,asAtom& ret, asAtom* args, const unsigned int argslen) override
	{
		asAtom th = asAtomHandler::fromObject(this);
		T::generator(ret,wrk,th,args,argslen);
	}

	const Template_base* getTemplate() const override
	{
		return templ;
	}

	std::vector<Type*> getTypes() const
	{
		return types;
	}
	void addType(Type* type)
	{
		types.push_back(type);
	}

	bool coerce(ASWorker* wrk,asAtom& o) override
	{
		if (asAtomHandler::isUndefined(o))
		{
			asAtomHandler::setNull(o);
			return true;
		}
		else if (asAtomHandler::isNull(o) || (asAtomHandler::isObject(o) && asAtomHandler::is<T>(o) && asAtomHandler::getObjectNoCheck(o)->as<T>()->sameType(this)))
		{
			// Vector.<x> can be coerced to Vector.<y>
			// only if x and y are the same type
			return false;
		}
		else
		{
			tiny_string clsname = asAtomHandler::getObject(o) ? asAtomHandler::getObject(o)->getClassName() : "";
			createError<TypeError>(wrk,kCheckTypeFailedError, clsname,
								  Class<T>::getQualifiedClassName());
		}
		return false;
	}
};

/* this is modeled closely after the Class/Class_base pattern */
template<class T>
class Template : public Template_base
{
public:
	Template(ASWorker* wrk,QName name) : Template_base(wrk,name) {}

	QName getQName(SystemState* sys, const std::vector<Type*>& types)
	{
		//This is the naming scheme that the ABC compiler uses,
		//and we need to stay in sync here
		tiny_string name = ClassName<T>::name;
		for(size_t i=0;i<types.size();++i)
		{
			name += "$";
			name += types[i]->getName();
		}
		QName ret(sys->getUniqueStringId(name),sys->getUniqueStringId(ClassName<T>::ns));
		return ret;
	}

	Class_base* applyType(const std::vector<Type*>& types,_NR<ApplicationDomain> applicationDomain)
	{
		_NR<ApplicationDomain> appdomain = applicationDomain;
		
		// if type is a builtin class, it is handled in the systemDomain
		if (appdomain.isNull() || (types.size() > 0 && types[0]->isBuiltin()))
			appdomain = _MR(getSys()->systemDomain);
		QName instantiatedQName = getQName(appdomain->getSystemState(),types);

		std::map<QName, Class_base*>::iterator it=appdomain->instantiatedTemplates.find(instantiatedQName);
		Class<T>* ret=nullptr;
		if(it==appdomain->instantiatedTemplates.end()) //This class is not yet in the map, create it
		{
			MemoryAccount* m = appdomain->getSystemState()->allocateMemoryAccount(instantiatedQName.getQualifiedName(appdomain->getSystemState()));
			ret=new (m) TemplatedClass<T>(instantiatedQName,types,this,m);
			appdomain->instantiatedTemplates.insert(std::make_pair(instantiatedQName,ret));
			ret->prototype = _MNR(new_objectPrototype(appdomain->getInstanceWorker()));
			T::sinit(ret);
			if(ret->super)
				ret->prototype->prevPrototype=ret->super->prototype;
			ret->addPrototypeGetter();
		}
		else
		{
			TemplatedClass<T>* tmp = static_cast<TemplatedClass<T>*>(it->second);
			if (tmp->getTypes().size() == 0)
				tmp->addType(types[0]);
			ret= tmp;
		}

		ret->incRef();
		return ret;
	}
	Class_base* applyTypeByQName(const QName& qname,_NR<ApplicationDomain> applicationDomain)
	{
		const std::vector<Type*> types;
		_NR<ApplicationDomain> appdomain = applicationDomain;
		std::map<QName, Class_base*>::iterator it=appdomain->instantiatedTemplates.find(qname);
		Class<T>* ret=nullptr;
		if(it==appdomain->instantiatedTemplates.end()) //This class is not yet in the map, create it
		{
			MemoryAccount* m = appdomain->getSystemState()->allocateMemoryAccount(qname.getQualifiedName(appdomain->getSystemState()));
			ret=new (m) TemplatedClass<T>(qname,types,this,m);
			appdomain->instantiatedTemplates.insert(std::make_pair(qname,ret));
			ret->prototype = _MNR(new_objectPrototype(appdomain->getInstanceWorker()));
			T::sinit(ret);
			if(ret->super)
				ret->prototype->prevPrototype=ret->super->prototype;
			ret->addPrototypeGetter();
		}
		else
			ret=static_cast<TemplatedClass<T>*>(it->second);

		ret->incRef();
		return ret;
	}

	static Ref<Class_base> getTemplateInstance(RootMovieClip* root,Type* type,_NR<ApplicationDomain> appdomain)
	{
		std::vector<Type*> t(1,type);
		Template<T>* templ=getTemplate(root);
		Ref<Class_base> ret=_MR(templ->applyType(t, appdomain));
		templ->decRef();
		return ret;
	}

	static Ref<Class_base> getTemplateInstance(RootMovieClip* root,const QName& qname, ABCContext* context,_NR<ApplicationDomain> appdomain)
	{
		Template<T>* templ=getTemplate(root);
		Ref<Class_base> ret=_MR(templ->applyTypeByQName(qname,appdomain));
		ret->context = context;
		templ->decRef();
		return ret;
	}
	static void getInstanceS(ASWorker* wrk,asAtom& ret, RootMovieClip* root,Type* type,_NR<ApplicationDomain> appdomain)
	{
		getTemplateInstance(root,type,appdomain).getPtr()->getInstance(wrk,ret,true,nullptr,0);
	}

	static Template<T>* getTemplate(RootMovieClip* root,const QName& name)
	{
		std::map<QName, Template_base*>::iterator it=root->templates.find(name);
		Template<T>* ret=nullptr;
		if(it==root->templates.end()) //This class is not yet in the map, create it
		{
			MemoryAccount* m = root->getSystemState()->allocateMemoryAccount(name.getQualifiedName(root->getSystemState()));
			ASWorker* wrk = root->getInstanceWorker();
			ret=new (m) Template<T>(wrk,name);
			ret->prototype = _MNR(new_objectPrototype(wrk));
			ret->addPrototypeGetter(root->getSystemState());
			ret->setRefConstant();
			root->templates.insert(std::make_pair(name,ret));
		}
		else
			ret=static_cast<Template<T>*>(it->second);

		ret->incRef();
		return ret;
	}

	static Template<T>* getTemplate(RootMovieClip* root)
	{
		return getTemplate(root,QName(root->getSystemState()->getUniqueStringId(ClassName<T>::name),root->getSystemState()->getUniqueStringId(ClassName<T>::ns)));
	}
};


class Vector: public ASObject
{
	Type* vec_type;
	bool fixed;
	std::vector<asAtom, reporter_allocator<asAtom>> vec;
	int capIndex(int i) const;
	class sortComparatorDefault
	{
	private:
		bool isNumeric;
		bool isCaseInsensitive;
		bool isDescending;
	public:
		sortComparatorDefault(bool n, bool ci, bool d):isNumeric(n),isCaseInsensitive(ci),isDescending(d){}
		bool operator()(const asAtom& d1, const asAtom& d2);
	};
	asAtom getDefaultValue();
public:
	class sortComparatorWrapper
	{
	private:
		asAtom comparator;
	public:
		sortComparatorWrapper(asAtom c):comparator(c){}
		number_t compare(const asAtom& d1, const asAtom& d2);
	};
	Vector(ASWorker* wrk,Class_base* c, Type *vtype=nullptr);
	~Vector();
	bool destruct() override;
	void finalize() override;
	void prepareShutdown() override;
	bool countCylicMemberReferences(garbagecollectorstate& gcstate) override;

	static void sinit(Class_base* c);
	static void generator(asAtom& ret, ASWorker* wrk, asAtom& o_class, asAtom* args, const unsigned int argslen);

	void setTypes(const std::vector<Type*>& types);
	bool sameType(const Class_base* cls) const;
	Class_base* getType() const { return (Class_base*)vec_type; }

	//Overloads
	tiny_string toString();
	multiname* setVariableByMultiname(multiname& name, asAtom &o, CONST_ALLOWED_FLAG allowConst, bool* alreadyset, ASWorker* wrk) override;
	void setVariableByInteger(int index, asAtom& o, CONST_ALLOWED_FLAG allowConst, bool* alreadyset,ASWorker* wrk) override;
	FORCE_INLINE void setVariableByIntegerNoCoerce(int index, asAtom &o, bool* alreadyset,ASWorker* wrk)
	{
		if (USUALLY_FALSE(index < 0))
		{
			setVariableByInteger_intern(index,o,ASObject::CONST_ALLOWED,alreadyset,wrk);
			return;
		}
		*alreadyset=false;
		if(size_t(index) < vec.size())
		{
			if (vec[index].uintval != o.uintval)
			{
				ASObject* obj = asAtomHandler::getObject(vec[index]);
				if (obj)
					obj->removeStoredMember();
				obj = asAtomHandler::getObject(o);
				if (obj)
					obj->addStoredMember();
				vec[index] = o;
			}
			else
				*alreadyset=true;
		}
		else if(!fixed && size_t(index) == vec.size())
		{
			ASObject* obj = asAtomHandler::getObject(o);
			if (obj)
				obj->addStoredMember();
			vec.push_back(o);
		}
		else
		{
			throwRangeError(index);
		}
	}
	void throwRangeError(int index);
	
	bool hasPropertyByMultiname(const multiname& name, bool considerDynamic, bool considerPrototype, ASWorker* wrk) override;
	GET_VARIABLE_RESULT getVariableByMultiname(asAtom& ret, const multiname& name, GET_VARIABLE_OPTION opt, ASWorker* wrk) override;
	GET_VARIABLE_RESULT getVariableByInteger(asAtom& ret, int index, GET_VARIABLE_OPTION opt,ASWorker* wrk) override;
	FORCE_INLINE void getVariableByIntegerDirect(asAtom& ret, int index, ASWorker* wrk)
	{
		if (index >=0 && uint32_t(index) < size())
			ret = vec[index];
		else
			getVariableByIntegerIntern(ret,index,GET_VARIABLE_OPTION::NONE,wrk);
	}
	static bool isValidMultiname(SystemState* sys, const multiname& name, uint32_t& index, bool *isNumber = nullptr);

	tiny_string toJSON(std::vector<ASObject *> &path, asAtom replacer, const tiny_string &spaces,const tiny_string& filter) override;

	uint32_t nextNameIndex(uint32_t cur_index) override;
	void nextName(asAtom &ret, uint32_t index) override;
	void nextValue(asAtom &ret, uint32_t index) override;

	uint32_t size() const
	{
		return vec.size();
	}
	asAtom at(unsigned int index) const
	{
		return vec.at(index);
	}
	bool ensureLength(uint32_t len);
	void set(uint32_t index, asAtom v)
	{
		if (index < size())
		{
			ASObject* obj = asAtomHandler::getObject(vec[index]);
			if (obj)
				obj->removeStoredMember();
			obj = asAtomHandler::getObject(v);
			if (obj)
				obj->addStoredMember();
			vec[index] = v;
		}
	}
	//Get value at index, or return defaultValue (a borrowed
	//reference) if index is out-of-range
	asAtom at(unsigned int index, asAtom defaultValue) const;

	//Appends an object to the Vector. o is coerced to vec_type.
	//Takes ownership of o.
	void append(asAtom& o);
	void setFixed(bool v) { fixed = v; }
	
	void remove(ASObject* o);

	//TODO: do we need to implement generator?
	ASFUNCTION_ATOM(_constructor);

	ASFUNCTION_ATOM(push);
	ASFUNCTION_ATOM(_concat);
	ASFUNCTION_ATOM(_pop);
	ASFUNCTION_ATOM(join);
	ASFUNCTION_ATOM(shift);
	ASFUNCTION_ATOM(unshift);
	ASFUNCTION_ATOM(splice);
	ASFUNCTION_ATOM(_sort);
	ASFUNCTION_ATOM(getLength);
	ASFUNCTION_ATOM(setLength);
	ASFUNCTION_ATOM(filter);
	ASFUNCTION_ATOM(indexOf);
	ASFUNCTION_ATOM(_getLength);
	ASFUNCTION_ATOM(_setLength);
	ASFUNCTION_ATOM(getFixed);
	ASFUNCTION_ATOM(setFixed);
	ASFUNCTION_ATOM(forEach);
	ASFUNCTION_ATOM(_reverse);
	ASFUNCTION_ATOM(lastIndexOf);
	ASFUNCTION_ATOM(_map);
	ASFUNCTION_ATOM(_toString);
	ASFUNCTION_ATOM(slice);
	ASFUNCTION_ATOM(every);
	ASFUNCTION_ATOM(some);
	ASFUNCTION_ATOM(insertAt);
	ASFUNCTION_ATOM(removeAt);

	ASObject* describeType(ASWorker* wrk) const override;
	//Serialization interface
	void serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap, ASWorker* wrk) override;
};

}
#endif /* SCRIPTING_TOPLEVEL_VECTOR_H */
