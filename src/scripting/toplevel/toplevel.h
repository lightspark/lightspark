/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2011  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef TOPLEVEL_H
#define TOPLEVEL_H
#include "compat.h"
#include <vector>
#include <set>
#include "asobject.h"
#include "exceptions.h"
#include "threading.h"
#include <libxml/tree.h>
#include <libxml++/parsers/domparser.h>
#include "scripting/abcutils.h"
#include <glibmm/ustring.h>
#include "Boolean.h"
#include "Error.h"

namespace lightspark
{
const tiny_string AS3="http://adobe.com/AS3/2006/builtin";

class Event;
class ABCContext;
class Template_base;
class method_info;
struct call_context;
struct traits_info;

class InterfaceClass: public ASObject
{
protected:
	static void lookupAndLink(Class_base* c, const tiny_string& name, const tiny_string& interfaceNs);
};

class Prototype: public ASObject
{
public:
	Prototype(const _R<Class_base>& _classdef) : classdef(_classdef) {}
	_NR<Prototype> prototype;
	ASObject* getVariableByMultiname(const multiname& name, GET_VARIABLE_OPTION opt);
	/* this is the class such that classdef->prototype == this */
	_R<Class_base> classdef;
};

class Class_base: public ASObject
{
friend class ABCVm;
friend class ABCContext;
private:
	mutable std::vector<multiname> interfaces;
	mutable std::vector<Class_base*> interfaces_added;
	bool use_protected;
	nsNameAndKind protected_ns;
	int _maxlevel()
	{
		return max_level;
	}
	void recursiveBuild(ASObject* target);
	IFunction* constructor;
	void describeTraits(xmlpp::Element* root, std::vector<traits_info>& traits) const;
	//Naive garbage collection until reference cycles are detected
	Mutex referencedObjectsMutex;
	std::set<ASObject*> referencedObjects;
	void finalizeObjects() const;
public:
	void addPrototypeGetter();
	ASPROPERTY_GETTER(_NR<Prototype>,prototype);
	Class_base* super;
	//We need to know what is the context we are referring to
	ABCContext* context;
	QName class_name;
	int class_index;
	int max_level;
	void handleConstruction(ASObject* target, ASObject* const* args, unsigned int argslen, bool buildAndLink);
	void setConstructor(IFunction* c);
	Class_base(const QName& name);
	~Class_base();
	void finalize();
	virtual ASObject* getInstance(bool construct, ASObject* const* args, const unsigned int argslen)=0;
	void addImplementedInterface(const multiname& i);
	void addImplementedInterface(Class_base* i);
	virtual void buildInstanceTraits(ASObject* o) const=0;
	const std::vector<Class_base*>& getInterfaces() const;
	void linkInterface(Class_base* c) const;
	bool isSubClass(const Class_base* cls) const;
	tiny_string getQualifiedClassName() const;
	tiny_string toString(bool debugMsg);
	virtual ASObject* generator(ASObject* const* args, const unsigned int argslen);
	ASObject *describeType() const;
	void describeInstance(xmlpp::Element* root) const;
	virtual const Template_base* getTemplate() const { return NULL; }
	//DEPRECATED: naive garbage collector
	void abandonObject(ASObject* ob) DLL_PUBLIC;
	void acquireObject(ASObject* ob) DLL_PUBLIC;
};

class Template_base : public ASObject
{
private:
	QName template_name;
public:
	Template_base(QName name);
	virtual Class_base* applyType(ASObject* const* args, const unsigned int argslen)=0;
};

class Class_object: public Class_base
{
private:
	Class_object():Class_base(QName("Class","")){}
	ASObject* getInstance(bool construct, ASObject* const* args, const unsigned int argslen)
	{
		throw RunTimeException("Class_object::getInstance");
		return NULL;
	}
	void buildInstanceTraits(ASObject* o) const
	{
		throw RunTimeException("Class_object::buildInstanceTraits");
	}
public:
	static Class_object* getClass();
};

//Adaptor from fuction to class, it does not seems to be a good idea to
//derive IFunction from Class_base, because it's too heavyweight
class Class_function: public Class_base
{
private:
	IFunction* f;
	ASObject* asprototype;
	ASObject* getInstance(bool construct, ASObject* const* args, const unsigned int argslen)
	{
		throw RunTimeException("Class_function::getInstance");
		return NULL;
	}
	void buildInstanceTraits(ASObject* o) const
	{
		throw RunTimeException("Class_function::buildInstanceTraits");
	}
public:
	Class_function(IFunction* _f, ASObject* _p);
	ASObject* getVariableByMultiname(const multiname& name, GET_VARIABLE_OPTION opt=NONE)
	{
		ASObject* ret=Class_base::getVariableByMultiname(name,opt);
		if(ret==NULL && asprototype)
			ret=asprototype->getVariableByMultiname(name,opt);
		return ret;
	}
	intptr_t getVariableByMultiname_i(const multiname& name)
	{
		throw UnsupportedException("Class_function::getVariableByMultiname_i");
		return 0;
	}
	void setVariableByMultiname_i(const multiname& name, intptr_t value)
	{
		throw UnsupportedException("Class_function::setVariableByMultiname_i");
	}
	void setVariableByMultiname(const multiname& name, ASObject* o)
	{
		throw UnsupportedException("Class_function::setVariableByMultiname");
	}
	bool hasPropertyByMultiname(const multiname& name, bool considerDynamic);
};

class IFunction: public ASObject
{
CLASSBUILDABLE(IFunction);
private:
	virtual ASObject* callImpl(ASObject* obj, ASObject* const* args, uint32_t num_args, bool thisOverride)=0;
protected:
	IFunction();
	virtual IFunction* clone()=0;
	_NR<ASObject> closure_this;
	int closure_level;
	bool bound;
public:
	void finalize();
	ASFUNCTION(apply);
	ASFUNCTION(_call);
	ASObject* call(ASObject* obj, ASObject* const* args, uint32_t num_args);
	IFunction* bind(_NR<ASObject> c, int level)
	{
		if(!bound)
		{
			IFunction* ret=NULL;
			if(c==NULL)
			{
				//If binding with null we are generated from newFunction, don't copy
				ret=this;
			}
			else
			{
				//Generate a copy
				ret=clone();
				ret->classdef=NULL; //Drop the classdef and set it ex novo
				ret->setClass(getClass());
			}
			ret->bound=true;
			ret->closure_this=c;
			//std::cout << "Binding " << ret << std::endl;
			return ret;
		}
		else
		{
			incRef();
			return this;
		}
	}
	void bindLevel(int l)
	{
		closure_level=l;
	}
	virtual method_info* getMethodInfo() const=0;
	virtual ASObject *describeType() const;
};

class Function : public IFunction
{
friend class Class<IFunction>;
public:
	typedef ASObject* (*as_function)(ASObject*, ASObject* const *, const unsigned int);
private:
	as_function val;
	Function(){}
	Function(as_function v):val(v){}
	Function* clone()
	{
		return new Function(*this);
	}
	method_info* getMethodInfo() const { return NULL; }
	ASObject* callImpl(ASObject* obj, ASObject* const* args, uint32_t num_args, bool thisOverride=false);
public:
	IFunction* toFunction();
	bool isEqual(ASObject* r)
	{
		Function* f=dynamic_cast<Function*>(r);
		if(f==NULL)
			return false;
		return val==f->val;
	}
};

class SyntheticFunction : public IFunction
{
friend class ABCVm;
friend class Class<IFunction>;
public:
	typedef ASObject* (*synt_function)(call_context* cc);
private:
	int hit_count;
	method_info* mi;
	synt_function val;
	SyntheticFunction(method_info* m);
	SyntheticFunction* clone()
	{
		return new SyntheticFunction(*this);
	}
	method_info* getMethodInfo() const { return mi; }
	ASObject* callImpl(ASObject* obj, ASObject* const* args, uint32_t num_args, bool thisOverride=false);
public:
	void finalize();
	IFunction* toFunction();
	std::vector<scope_entry> func_scope;
	bool isEqual(ASObject* r)
	{
		SyntheticFunction* sf=dynamic_cast<SyntheticFunction*>(r);
		if(sf==NULL)
			return false;
		return mi==sf->mi;
	}
	void acquireScope(const std::vector<scope_entry>& scope)
	{
		assert_and_throw(func_scope.empty());
		func_scope=scope;
	}
	void addToScope(const scope_entry& s)
	{
		func_scope.emplace_back(s);
	}
};

//Specialized class for IFunction
template<>
class Class<IFunction>: public Class_base
{
private:
	Class<IFunction>():Class_base(QName("Function","")){}
	ASObject* getInstance(bool construct, ASObject* const* args, const unsigned int argslen);
public:
	static Class<IFunction>* getClass();
	static Function* getFunction(Function::as_function v)
	{
		Class<IFunction>* c=Class<IFunction>::getClass();
		Function* ret=new Function(v);
		ret->setClass(c);
		ret->resetLevel();
		return ret;
	}
	static SyntheticFunction* getSyntheticFunction(method_info* m)
	{
		Class<IFunction>* c=Class<IFunction>::getClass();
		SyntheticFunction* ret=new SyntheticFunction(m);
		ret->setClass(c);
		c->handleConstruction(ret,NULL,0,true);
		return ret;
	}
	void buildInstanceTraits(ASObject* o) const
	{
	}
};

class Undefined : public ASObject
{
public:
	ASFUNCTION(call);
	Undefined();
	tiny_string toString(bool debugMsg);
	int32_t toInt();
	double toNumber();
	bool isEqual(ASObject* r);
	TRISTATE isLess(ASObject* r);
	ASObject *describeType() const;
	//Serialization interface
	void serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
			std::map<const ASObject*, uint32_t>& objMap) const;
};

class ASString: public ASObject
{
CLASSBUILDABLE(ASString);
private:
	tiny_string toString_priv() const;
	ASString();
	ASString(const std::string& s);
	ASString(const Glib::ustring& s);
	ASString(const tiny_string& s);
	ASString(const char* s);
	ASString(const char* s, uint32_t len);
public:
	Glib::ustring data;
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
	ASFUNCTION(charAt);
	ASFUNCTION(charCodeAt);
	ASFUNCTION(concat);
	ASFUNCTION(fromCharCode);
	ASFUNCTION(indexOf);
	ASFUNCTION(lastIndexOf);
	ASFUNCTION(match);
	ASFUNCTION(replace);
	ASFUNCTION(search);
	ASFUNCTION(slice);
	ASFUNCTION(split);
	ASFUNCTION(substr);
	ASFUNCTION(substring);
	ASFUNCTION(toLowerCase);
	ASFUNCTION(toUpperCase);
	ASFUNCTION(_toString);
	ASFUNCTION(_getLength);
	bool isEqual(ASObject* r);
	TRISTATE isLess(ASObject* r);
	tiny_string toString(bool debugMsg=false);
	double toNumber();
	int32_t toInt();
	uint32_t toUInt();
	ASFUNCTION(generator);
	//Serialization interface
	void serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
			std::map<const ASObject*, uint32_t>& objMap) const;
};

class Null: public ASObject
{
public:
	Null(){type=T_NULL;}
	tiny_string toString(bool debugMsg);
	bool isEqual(ASObject* r);
	TRISTATE isLess(ASObject* r);
	int32_t toInt();
	double toNumber();
	//Serialization interface
	void serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
			std::map<const ASObject*, uint32_t>& objMap) const;
};

class ASQName: public ASObject
{
friend class ABCContext;
friend class Namespace;
CLASSBUILDABLE(ASQName);
private:
	bool uri_is_null;
	tiny_string uri;
	tiny_string local_name;
	ASQName(){type=T_QNAME; uri_is_null=false;}
public:
	static void sinit(Class_base*);
	ASFUNCTION(_constructor);
	ASFUNCTION(_getURI);
	ASFUNCTION(_getLocalName);
	ASFUNCTION(_toString);
	tiny_string getURI() const { return uri; }
	tiny_string getLocalName() const { return local_name; }
	bool isEqual(ASObject* o);
	tiny_string toString(bool debugMsg);
};

class Namespace: public ASObject
{
friend class ASQName;
friend class ABCContext;
CLASSBUILDABLE(Namespace);
private:
	bool prefix_is_undefined;
	tiny_string uri;
	tiny_string prefix;
	Namespace(){type=T_NAMESPACE; prefix_is_undefined=false;}
	Namespace(const tiny_string& _uri):uri(_uri){type=T_NAMESPACE; prefix_is_undefined=false;}
public:
	static void sinit(Class_base*);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
	ASFUNCTION(_getURI);
	ASFUNCTION(_setURI);
	ASFUNCTION(_getPrefix);
	ASFUNCTION(_setPrefix);
	ASFUNCTION(_toString);
	bool isEqual(ASObject* o);
	tiny_string toString(bool debugMsg);
};

class Integer : public ASObject
{
friend class Number;
friend class Array;
friend class ABCVm;
friend class ABCContext;
friend ASObject* abstract_i(intptr_t i);
CLASSBUILDABLE(Integer);
private:
	Integer(int32_t v=0):val(v){type=T_INTEGER;}
	Integer(Manager* m):ASObject(m),val(0){type=T_INTEGER;}
public:
	int32_t val;
	static void buildTraits(ASObject* o){};
	static void sinit(Class_base* c);
	ASFUNCTION(_toString);
	tiny_string toString(bool debugMsg);
	int32_t toInt()
	{
		return val;
	}
	double toNumber()
	{
		return val;
	}
	TRISTATE isLess(ASObject* r);
	bool isEqual(ASObject* o);
	ASFUNCTION(generator);
	//Serialization interface
	void serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
			std::map<const ASObject*, uint32_t>& objMap) const;
};

class UInteger: public ASObject
{
friend ASObject* abstract_ui(uint32_t i);
CLASSBUILDABLE(UInteger);
private:
	UInteger(Manager* m):ASObject(m),val(0){type=T_UINTEGER;}
public:
	uint32_t val;
	UInteger(uint32_t v=0):val(v){type=T_UINTEGER;}

	static void sinit(Class_base* c);
	tiny_string toString(bool debugMsg);
	int32_t toInt()
	{
		return val;
	}
	uint32_t toUInt()
	{
		return val;
	}
	double toNumber()
	{
		return val;
	}
	TRISTATE isLess(ASObject* r);
	bool isEqual(ASObject* o);
	ASFUNCTION(generator);
	//CHECK: should this have a special serialization?
};

class Number : public ASObject
{
friend ASObject* abstract_d(number_t i);
friend class ABCContext;
friend class ABCVm;
CLASSBUILDABLE(Number);
private:
	Number():val(0) {type=T_NUMBER;}
	Number(double v):val(v){type=T_NUMBER;}
	Number(Manager* m):ASObject(m),val(0){type=T_NUMBER;}
	static void purgeTrailingZeroes(char* buf);
public:
	double val;
	ASFUNCTION(_constructor);
	ASFUNCTION(_toString);
	tiny_string toString(bool debugMsg);
	unsigned int toUInt()
	{
		return (unsigned int)(val);
	}
	int32_t toInt()
	{
		if(val<0)
			return int(val);
		else
		{
			uint32_t ret=val;
			return ret;
		}
	}
	double toNumber()
	{
		return val;
	}
	TRISTATE isLess(ASObject* o);
	bool isEqual(ASObject* o);
	static void buildTraits(ASObject* o){};
	static void sinit(Class_base* c);
	ASFUNCTION(generator);
	//Serialization interface
	void serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
			std::map<const ASObject*, uint32_t>& objMap) const;
};

class XML: public ASObject
{
private:
	//The parser will destroy the document and all the childs on destruction
	xmlpp::DomParser parser;
	//Pointer to the root XML element, the one that owns the parser that created this node
	_NR<XML> root;
	//The node this object represent
	xmlpp::Node* node;
	static void recursiveGetDescendantsByQName(_R<XML> root, xmlpp::Node* node, const tiny_string& name, const tiny_string& ns, 
			std::vector<_R<XML>>& ret);
	tiny_string toString_priv();
	void buildFromString(const std::string& str);
	bool constructed;
	bool nodesEqual(xmlpp::Node *a, xmlpp::Node *b) const;
public:
	XML();
	XML(const std::string& str);
	XML(_R<XML> _r, xmlpp::Node* _n);
	XML(xmlpp::Node* _n);
	void finalize();
	ASFUNCTION(_constructor);
	ASFUNCTION(_toString);
	ASFUNCTION(toXMLString);
	ASFUNCTION(nodeKind);
	ASFUNCTION(children);
	ASFUNCTION(attributes);
	ASFUNCTION(appendChild);
	ASFUNCTION(localName);
	ASFUNCTION(name);
	ASFUNCTION(descendants);
	ASFUNCTION(generator);
	ASFUNCTION(_hasSimpleContent);
	ASFUNCTION(_hasComplexContent);
	static void buildTraits(ASObject* o){};
	static void sinit(Class_base* c);
	void getDescendantsByQName(const tiny_string& name, const tiny_string& ns, std::vector<_R<XML> >& ret);
	ASObject* getVariableByMultiname(const multiname& name, GET_VARIABLE_OPTION opt);
	bool hasPropertyByMultiname(const multiname& name, bool considerDynamic);
	tiny_string toString(bool debugMsg=false);
	void toXMLString_priv(xmlBufferPtr buf);
	bool hasSimpleContent() const;
	bool hasComplexContent() const;
        xmlElementType getNodeKind() const;
	bool isEqual(ASObject* r);
	uint32_t nextNameIndex(uint32_t cur_index);
	_R<ASObject> nextName(uint32_t index);
	_R<ASObject> nextValue(uint32_t index);
	//Serialization interface
	void serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
			std::map<const ASObject*, uint32_t>& objMap) const;
	_R<ASObject> toPrimitive();
};

class XMLList: public ASObject
{
private:
	std::vector<_R<XML> > nodes;
	bool constructed;
	tiny_string toString_priv() const;
	void buildFromString(const std::string& str);
	void toXMLString_priv(xmlBufferPtr buf) const;
public:
	XMLList():constructed(false){}
	/*
	   Special constructor to build empty XMLList out of AS code
	*/
	XMLList(bool c):constructed(c){assert(c);}
	XMLList(const std::vector<_R<XML> >& r):nodes(r),constructed(true){}
	XMLList(const std::string& str);
	void finalize();
	static void buildTraits(ASObject* o){};
	static void sinit(Class_base* c);
	ASFUNCTION(_constructor);
	ASFUNCTION(_getLength);
	ASFUNCTION(appendChild);
	ASFUNCTION(_hasSimpleContent);
	ASFUNCTION(_hasComplexContent);
	ASFUNCTION(_toString);
	ASFUNCTION(toXMLString);
	ASFUNCTION(generator);
	ASFUNCTION(descendants);
	ASObject* getVariableByMultiname(const multiname& name, GET_VARIABLE_OPTION opt);
	void setVariableByMultiname(const multiname& name, ASObject* o);
	bool hasPropertyByMultiname(const multiname& name, bool considerDynamic);
	void getDescendantsByQName(const tiny_string& name, const tiny_string& ns, std::vector<_R<XML> >& ret);
	_NR<XML> convertToXML() const;
	bool hasSimpleContent() const;
	bool hasComplexContent() const;
	void append(_R<XML> x);
	void append(_R<XMLList> x);
	tiny_string toString(bool debugMsg=false);
	bool isEqual(ASObject* r);
	uint32_t nextNameIndex(uint32_t cur_index);
	_R<ASObject> nextName(uint32_t index);
	_R<ASObject> nextValue(uint32_t index);
	_R<ASObject> toPrimitive();
};

//Internal objects used to store traits declared in scripts but not yet valid
class Definable : public ASObject
{
public:
	Definable(){type=T_DEFINABLE;}
	virtual void define(ASObject* g)=0;
};

class ScriptDefinable: public Definable
{
private:
	_R<IFunction> f;
public:
	ScriptDefinable(IFunction* _f):f(_MR(_f)){}
	//The global object will be passed from the calling context
	void define(ASObject* g){ g->incRef(); f->call(g,NULL,0); }
};

class RegExp: public ASObject
{
CLASSBUILDABLE(RegExp);
friend class ASString;
private:
	Glib::ustring re;
	bool global;
	bool ignoreCase;
	bool extended;
	bool multiline;
	int lastIndex;
	RegExp();
public:
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
	ASFUNCTION(exec);
	ASFUNCTION(test);
	ASFUNCTION(_getGlobal);
};

class GlobalObject
{
public:
	void registerGlobalScope(ASObject* scope);
	std::vector<ASObject*> globalScopes;
	ASObject* getVariableByString(const std::string& name, ASObject*& target);
	ASObject* getVariableAndTargetByMultiname(const multiname& name, ASObject*& target);
	~GlobalObject();
};

template<class T>
class ArgumentConversion
{
public:
	static T toConcrete(ASObject* obj);
	static ASObject* toAbstract(const T& val);
};

template<class T>
class ArgumentConversion<Ref<T>>
{
public:
	static Ref<T> toConcrete(ASObject* obj)
	{
		T* o = dynamic_cast<T*>(obj);
		if(!o)
			throw ArgumentError("Wrong type");
		o->incRef();
		return _MR(o);
	}
	static ASObject* toAbstract(const Ref<T>& val)
	{
		val->incRef();
		return val.getPtr();
	}
};

template<class T>
class ArgumentConversion<NullableRef<T>>
{
public:
	static NullableRef<T> toConcrete(ASObject* obj)
	{
		if(obj->getObjectType() == T_NULL)
			return NullRef;

		T* o = dynamic_cast<T*>(obj);
		if(!o)
			throw ArgumentError("Wrong type");
		o->incRef();
		return _MNR(o);
	}
	static ASObject* toAbstract(const NullableRef<T>& val)
	{
		if(val.isNull())
			return new Null();
		val->incRef();
		return val.getPtr();
	}
};

template<>
number_t ArgumentConversion<number_t>::toConcrete(ASObject* obj);
template<>
bool ArgumentConversion<bool>::toConcrete(ASObject* obj);
template<>
uint32_t ArgumentConversion<uint32_t>::toConcrete(ASObject* obj);
template<>
int32_t ArgumentConversion<int32_t>::toConcrete(ASObject* obj);
template<>
tiny_string ArgumentConversion<tiny_string>::toConcrete(ASObject* obj);
template<>
RGB ArgumentConversion<RGB>::toConcrete(ASObject* obj);

template<>
ASObject* ArgumentConversion<int32_t>::toAbstract(const int32_t& val);
template<>
ASObject* ArgumentConversion<uint32_t>::toAbstract(const uint32_t& val);
template<>
ASObject* ArgumentConversion<number_t>::toAbstract(const number_t& val);
template<>
ASObject* ArgumentConversion<bool>::toAbstract(const bool& val);
template<>
ASObject* ArgumentConversion<tiny_string>::toAbstract(const tiny_string& val);
template<>
ASObject* ArgumentConversion<RGB>::toAbstract(const RGB& val);

ASObject* parseInt(ASObject* obj,ASObject* const* args, const unsigned int argslen);
ASObject* parseFloat(ASObject* obj,ASObject* const* args, const unsigned int argslen);
ASObject* isNaN(ASObject* obj,ASObject* const* args, const unsigned int argslen);
ASObject* isFinite(ASObject* obj,ASObject* const* args, const unsigned int argslen);
ASObject* encodeURI(ASObject* obj,ASObject* const* args, const unsigned int argslen);
ASObject* decodeURI(ASObject* obj,ASObject* const* args, const unsigned int argslen);
ASObject* encodeURIComponent(ASObject* obj,ASObject* const* args, const unsigned int argslen);
ASObject* decodeURIComponent(ASObject* obj,ASObject* const* args, const unsigned int argslen);
ASObject* escape(ASObject* obj,ASObject* const* args, const unsigned int argslen);
ASObject* unescape(ASObject* obj,ASObject* const* args, const unsigned int argslen);
ASObject* print(ASObject* obj,ASObject* const* args, const unsigned int argslen);
ASObject* trace(ASObject* obj,ASObject* const* args, const unsigned int argslen);
bool isXMLName(ASObject* obj);
ASObject* _isXMLName(ASObject* obj,ASObject* const* args, const unsigned int argslen);


inline void Manager::put(ASObject* o)
{
	if(available.size()>maxCache)
		delete o;
	else
	{
		//The Manager now owns this object
		if(o->classdef)
			o->classdef->abandonObject(o);
		available.push_back(o);
	}
}

template<class T>
T* Manager::get()
{
	if(available.size())
	{
		T* ret=static_cast<T*>(available.back());
		available.pop_back();
		ret->incRef();
		//Transfer ownership back to the classdef
		if(ret->getClass())
			ret->getClass()->acquireObject(ret);
		//std::cout << "getting[" << name << "] " << ret << std::endl;
		return ret;
	}
	else
	{
		T* ret=Class<T>::getInstanceS(this);
		//std::cout << "newing" << ret << std::endl;
		return ret;
	}
}

inline Manager::~Manager()
{
	for(auto i = available.begin(); i != available.end(); ++i)
	{
		// ~ASObject will call abandonObject() again
		if((*i)->classdef)
			(*i)->classdef->acquireObject(*i);
		delete *i;
	}
}

};

#endif
