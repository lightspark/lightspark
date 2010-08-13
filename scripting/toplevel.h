/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009,2010  Alessandro Pignotti (a.pignotti@sssup.it)

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
#include "frame.h"
#include "exceptions.h"
#include "threading.h"

namespace lightspark
{
const tiny_string AS3="http://adobe.com/AS3/2006/builtin";

class Event;
class method_info;
struct call_context;

class InterfaceClass: public ASObject
{
protected:
	static void lookupAndLink(ASObject* o, const tiny_string& name, const tiny_string& interfaceNs);
};

class Class_base: public ASObject
{
friend class ABCVm;
friend class ABCContext;
private:
	mutable std::vector<multiname> interfaces;
	mutable std::vector<Class_base*> interfaces_added;
	bool use_protected;
	tiny_string protected_ns;
	int _maxlevel()
	{
		return max_level;
	}
	void recursiveBuild(ASObject* target);
	IFunction* constructor;
	//Naive garbage collection until reference cycles are detected
	Mutex referencedObjectsMutex;
	std::set<ASObject*> referencedObjects;

public:
	Class_base* super;
	//We need to know what is the context we are referring to
	ABCContext* context;
	tiny_string class_name;
	int class_index;
	int max_level;
	void handleConstruction(ASObject* target, ASObject* const* args, unsigned int argslen, bool buildAndLink);
	void setConstructor(IFunction* c);
	Class_base(const tiny_string& name);
	~Class_base();
	virtual ASObject* getInstance(bool construct, ASObject* const* args, const unsigned int argslen)=0;
	ASObject* getVariableByMultiname(const multiname& name, bool skip_impl, bool enableOverride=true, ASObject* base=NULL)
	{
		ASObject* ret=ASObject::getVariableByMultiname(name, skip_impl, enableOverride, base);
		if(ret==NULL && super)
			ret=super->getVariableByMultiname(name, skip_impl, enableOverride, base);
		return ret;
	}
	intptr_t getVariableByMultiname_i(const multiname& name)
	{
		throw UnsupportedException("Class_base::getVariableByMultiname_i");
		return 0;
/*		intptr_t ret=ASObject::getVariableByMultiname(name);
		if(==NULL && super)
			ret=super->getVariableByMultiname(name);
		return ret;*/
	}
	ASObject* getVariableByQName(const tiny_string& name, const tiny_string& ns, bool skip_impl=false)
	{
		ASObject* ret=ASObject::getVariableByQName(name,ns,skip_impl);
		if(ret==NULL && super)
			ret=super->getVariableByQName(name,ns,skip_impl);
		return ret;
	}
/*	void setVariableByMultiname_i(const multiname& name, intptr_t value)
	{
		abort();
	}
	void setVariableByMultiname(const multiname& name, ASObject* o)
	{
		abort();
	}
	void setVariableByQName(const tiny_string& name, const tiny_string& ns, ASObject* o, bool find_back=true)
	{
		abort();
	}*/
	void addImplementedInterface(const multiname& i);
	void addImplementedInterface(Class_base* i);
	virtual void buildInstanceTraits(ASObject* o) const=0;
	const std::vector<Class_base*>& getInterfaces() const;
	void linkInterface(Class_base* c) const;
	bool isSubClass(const Class_base* cls) const;
	tiny_string getQualifiedClassName() const;
	tiny_string toString(bool debugMsg);
	virtual ASObject* generator(ASObject* const* args, const unsigned int argslen);
	
	//DEPRECATED: naive garbage collector
	void abandonObject(ASObject* ob);
	void acquireObject(ASObject* ob);
	void cleanUp();
};

class Class_object: public Class_base
{
private:
	Class_object():Class_base("Class"){}
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
	//Class_function is both used as the prototype for each function and as the Function classs object
	Class_function():Class_base("Function"),f(NULL),asprototype(NULL){}
	Class_function(IFunction* _f, ASObject* _p):Class_base("Function"),f(_f),asprototype(_p){}
	tiny_string class_name;
	ASObject* getVariableByMultiname(const multiname& name, bool skip_impl=false, bool enableOverride=true, ASObject* base=NULL)
	{
		ASObject* ret=Class_base::getVariableByMultiname(name,skip_impl, enableOverride, base);
		if(ret==NULL && asprototype)
			ret=asprototype->getVariableByMultiname(name,skip_impl, enableOverride, base);
		return ret;
	}
	intptr_t getVariableByMultiname_i(const multiname& name)
	{
		throw UnsupportedException("Class_function::getVariableByMultiname_i");
		return 0;
/*		intptr_t ret=ASObject::getVariableByMultiname(name);
		if(ret==NULL && super)
			ret=super->getVariableByMultiname(name);
		return ret;*/
	}
	ASObject* getVariableByQName(const tiny_string& name, const tiny_string& ns, bool skip_impl=false)
	{
		ASObject* ret=Class_base::getVariableByQName(name,ns,skip_impl);
		if(ret==NULL && asprototype)
			ret=asprototype->getVariableByQName(name,ns,skip_impl);
		return ret;
	}
	void setVariableByMultiname_i(const multiname& name, intptr_t value)
	{
		throw UnsupportedException("Class_function::setVariableByMultiname_i");
	}
	void setVariableByMultiname(const multiname& name, ASObject* o, bool enableOverride, ASObject* base=NULL)
	{
		throw UnsupportedException("Class_function::setVariableByMultiname");
	}
	void setVariableByQName(const tiny_string& name, const tiny_string& ns, ASObject* o, bool skip_impl=false)
	{
		throw UnsupportedException("Class_function::setVariableByQName");
	}
	static Class_function* getClass();
};

class IFunction: public ASObject
{
CLASSBUILDABLE(IFunction);
protected:
	IFunction();
	virtual IFunction* clone()=0;
	ASObject* closure_this;
	int closure_level;
	bool bound;
	IFunction* overriden_by;
public:
	ASFUNCTION(apply);
	ASFUNCTION(_call);
	virtual ASObject* call(ASObject* obj, ASObject* const* args, uint32_t num_args, bool thisOverride=false)=0;
	IFunction* bind(ASObject* c, int level)
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
				ret->prototype=NULL; //Drop the prototype and set it ex novo
				ret->setPrototype(getPrototype());
			}
			ret->bound=true;
			ret->closure_this=c;
			if(c)
				c->incRef();
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
	void override(IFunction* f)
	{
		overriden_by=f;
	}
	IFunction* getOverride()
	{
		//Get the last override
		IFunction* cur=overriden_by;
		if(cur==NULL)
			return this;
		else
			return cur->getOverride();
	}
	bool isOverridden() const
	{
		return overriden_by!=NULL;
	}
	static void sinit(Class_base* c);
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
public:
	ASObject* call(ASObject* obj, ASObject* const* args, uint32_t num_args, bool thisOverride=false);
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
public:
	ASObject* call(ASObject* obj, ASObject* const* args, uint32_t num_args, bool thisOverride=false);
	IFunction* toFunction();
	std::vector<ASObject*> func_scope;
	bool isEqual(ASObject* r)
	{
		SyntheticFunction* sf=dynamic_cast<SyntheticFunction*>(r);
		if(sf==NULL)
			return false;
		return mi==sf->mi;
	}
	void acquireScope(const std::vector<ASObject*>& scope)
	{
		assert_and_throw(func_scope.empty());
		func_scope=scope;
		for(unsigned int i=0;i<func_scope.size();i++)
			func_scope[i]->incRef();
	}
	void addToScope(ASObject* s)
	{
		func_scope.push_back(s);
	}
};

//Specialized class for IFunction
template<>
class Class<IFunction>: public Class_base
{
private:
	Class<IFunction>():Class_base("Function"){}
	ASObject* getInstance(bool construct, ASObject* const* args, const unsigned int argslen)
	{
		throw UnsupportedException("Class<IFunction>::getInstance");
		return NULL;
	}
public:
	static Class<IFunction>* getClass();
	static Function* getFunction(Function::as_function v)
	{
		Class<IFunction>* c=Class<IFunction>::getClass();
		Function* ret=new Function(v);
		ret->setPrototype(c);
		//c->handleConstruction(ret,NULL,0,true);
		return ret;
	}
	static SyntheticFunction* getSyntheticFunction(method_info* m)
	{
		Class<IFunction>* c=Class<IFunction>::getClass();
		SyntheticFunction* ret=new SyntheticFunction(m);
		ret->setPrototype(c);
		c->handleConstruction(ret,NULL,0,true);
		return ret;
	}
	void buildInstanceTraits(ASObject* o) const
	{
	}
};

class Boolean: public ASObject
{
friend bool Boolean_concrete(ASObject* obj);
CLASSBUILDABLE(Boolean);
private:
	bool val;
	Boolean(){}
	Boolean(bool v):val(v){type=T_BOOLEAN;}
public:
	int32_t toInt()
	{
		return val;
	}
	bool isEqual(ASObject* r);
	tiny_string toString(bool debugMsg);
	static void buildTraits(ASObject* o){};
};

class Undefined : public ASObject
{
public:
	ASFUNCTION(call);
	Undefined();
	tiny_string toString(bool debugMsg);
	int toInt();
	double toNumber();
	bool isEqual(ASObject* r);
	TRISTATE isLess(ASObject* r);
	virtual ~Undefined(){}
};

class ASString: public ASObject
{
CLASSBUILDABLE(ASString);
private:
	tiny_string toString_priv() const;
	ASString();
	ASString(const std::string& s);
	ASString(const tiny_string& s);
	ASString(const char* s);
	ASString(const char* s, uint32_t len);
public:
	std::string data;
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	ASFUNCTION(split);
	ASFUNCTION(_constructor);
	ASFUNCTION(_getLength);
	ASFUNCTION(replace);
	ASFUNCTION(concat);
	ASFUNCTION(slice);
	ASFUNCTION(match);
	ASFUNCTION(substr);
	ASFUNCTION(indexOf);
	ASFUNCTION(charCodeAt);
	ASFUNCTION(charAt);
	ASFUNCTION(toLowerCase);
	ASFUNCTION(toUpperCase);
	bool isEqual(ASObject* r);
	TRISTATE isLess(ASObject* r);
	tiny_string toString(bool debugMsg=false);
	double toNumber();
	int32_t toInt();
};

class Null: public ASObject
{
public:
	Null(){type=T_NULL;}
	tiny_string toString(bool debugMsg);
	bool isEqual(ASObject* r);
};

class ASQName: public ASObject
{
friend class ABCContext;
CLASSBUILDABLE(ASQName);
private:
	tiny_string uri;
	tiny_string local_name;
	ASQName(){type=T_QNAME;}
public:
	static void sinit(Class_base*);
	ASFUNCTION(_constructor);
};

class Namespace: public ASObject
{
friend class ASQName;
friend class ABCContext;
CLASSBUILDABLE(Namespace);
private:
	tiny_string uri;
	Namespace(){type=T_NAMESPACE;}
	Namespace(const tiny_string& _uri):uri(_uri){type=T_NAMESPACE;}
public:
	static void sinit(Class_base*);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
};

enum DATA_TYPE {DATA_OBJECT=0,DATA_INT};

struct data_slot
{
	DATA_TYPE type;
	union
	{
		ASObject* data;
		int32_t data_i;
	};
	explicit data_slot(ASObject* o,DATA_TYPE t=DATA_OBJECT):type(t),data(o){}
	data_slot():type(DATA_OBJECT),data(NULL){}
	explicit data_slot(intptr_t i):type(DATA_INT),data_i(i){}
};

class Array: public ASObject
{
friend class ABCVm;
CLASSBUILDABLE(Array);
protected:
	std::vector<data_slot> data;
	void outofbounds() const;
	Array();
private:
	static bool sortComparator(const data_slot& d1, const data_slot& d2);
	class sortComparatorWrapper
	{
	private:
		IFunction* comparator;
	public:
		sortComparatorWrapper(IFunction* c):comparator(c){}
		bool operator()(const data_slot& d1, const data_slot& d2);
	};
public:
	//These utility methods are also used by ByteArray 
	static bool isValidMultiname(const multiname& name, unsigned int& index);
	static bool isValidQName(const tiny_string& name, const tiny_string& ns, unsigned int& index);

	virtual ~Array();
	static void sinit(Class_base*);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
	ASFUNCTION(_push);
	ASFUNCTION(_concat);
	ASFUNCTION(_pop);
	ASFUNCTION(join);
	ASFUNCTION(shift);
	ASFUNCTION(unshift);
	ASFUNCTION(splice);
	ASFUNCTION(_sort);
	ASFUNCTION(sortOn);
	ASFUNCTION(filter);
	ASFUNCTION(indexOf);
	ASFUNCTION(_getLength);
	ASObject* at(unsigned int index) const
	{
		if(index<data.size())
		{
			assert_and_throw(data[index].data);
			return data[index].data;
		}
		else
		{
			outofbounds();
			return NULL;
		}
	}
	void set(unsigned int index, ASObject* o)
	{
		if(index<data.size())
		{
			assert_and_throw(data[index].data==NULL);
			data[index].data=o;
			data[index].type=DATA_OBJECT;
		}
		else
			outofbounds();
	}
	int size() const
	{
		return data.size();
	}
	void push(ASObject* o)
	{
		data.push_back(data_slot(o,DATA_OBJECT));
	}
	void resize(int n)
	{
		data.resize(n);
	}
	ASObject* getVariableByQName(const tiny_string& name, const tiny_string& ns, bool skip_impl=false);
	ASObject* getVariableByMultiname(const multiname& name, bool skip_impl, bool enableOverride, ASObject* base=NULL);
	intptr_t getVariableByMultiname_i(const multiname& name);
	void setVariableByQName(const tiny_string& name, const tiny_string& ns, ASObject* o, bool skip_impl=false);
	void setVariableByMultiname(const multiname& name, ASObject* o, bool enableOverride=true, ASObject* base=NULL);
	void setVariableByMultiname_i(const multiname& name, intptr_t value);
	tiny_string toString(bool debugMsg=false);
	bool isEqual(ASObject* r);
	bool hasNext(unsigned int& index, bool& out);
	bool nextName(unsigned int index, ASObject*& out);
	bool nextValue(unsigned int index, ASObject*& out);
	tiny_string toString_priv() const;
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
	int32_t val;
	Integer(int32_t v=0):val(v){type=T_INTEGER;}
	Integer(Manager* m):ASObject(m),val(0){type=T_INTEGER;}
public:
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
};

class UInteger: public ASObject
{
private:
	uint32_t val;
public:
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
};

class Number : public ASObject
{
friend ASObject* abstract_d(number_t i);
friend class ABCContext;
CLASSBUILDABLE(Number);
private:
	double val;
	Number(){}
	Number(double v):val(v){type=T_NUMBER;}
	Number(Manager* m):ASObject(m),val(0){type=T_NUMBER;}
public:
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
};

class ASMovieClipLoader: public ASObject
{
public:
	ASMovieClipLoader();
	ASFUNCTION(addListener);
	ASFUNCTION(constructor);

};

class ASXML: public ASObject
{
private:
	char* xml_buf;
	int xml_index;
	static size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp);
public:
	ASXML();
	ASFUNCTION(constructor);
	ASFUNCTION(load);
};

class Date: public ASObject
{
CLASSBUILDABLE(Date);
private:
	int year;
	int month;
	int date;
	int hour;
	int minute;
	int second;
	int millisecond;
	int32_t toInt();
	Date();
public:
	static void sinit(Class_base*);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
	ASFUNCTION(getTimezoneOffset);
	ASFUNCTION(getTime);
	ASFUNCTION(getFullYear);
	ASFUNCTION(getHours);
	ASFUNCTION(getMinutes);
	ASFUNCTION(valueOf);
	tiny_string toString(bool debugMsg=false);
	tiny_string toString_priv() const;
};

//Internal objects used to store traits declared in scripts and object placed, but not yet valid
class Definable : public ASObject
{
public:
	Definable(){type=T_DEFINABLE;}
	virtual void define(ASObject* g)=0;
};

class ScriptDefinable: public Definable
{
private:
	IFunction* f;
public:
	ScriptDefinable(IFunction* _f):f(_f){}
	//The global object will be passed from the calling context
	void define(ASObject* g){ f->call(g,NULL,0); }
};

class Math: public ASObject
{
public:
	static void sinit(Class_base* c);
	ASFUNCTION(atan2);
	ASFUNCTION(abs);
	ASFUNCTION(sin);
	ASFUNCTION(cos);
	ASFUNCTION(floor);
	ASFUNCTION(ceil);
	ASFUNCTION(round);
	ASFUNCTION(sqrt);
	ASFUNCTION(log);
	ASFUNCTION(random);
	ASFUNCTION(_max);
	ASFUNCTION(_min);
	ASFUNCTION(pow);
};

class RegExp: public ASObject
{
CLASSBUILDABLE(RegExp);
friend class ASString;
private:
	std::string re;
	bool global;
	bool ignoreCase;
	bool extended;
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

class ASError: public ASObject
{
CLASSBUILDABLE(ASError);
private:
	tiny_string message;
	tiny_string name;
	int errorID;
public:
	ASError(const tiny_string& error_message = "", int id = 0) : message(error_message), name("Error"), errorID(id) {}
	ASFUNCTION(getStackTrace);
	ASFUNCTION(_setName);
	ASFUNCTION(_getName);
	ASFUNCTION(_setMessage);
	ASFUNCTION(_getMessage);
	ASFUNCTION(_getErrorID);
	tiny_string toString(bool debugMsg=false);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
};

bool Boolean_concrete(ASObject* obj);
ASObject* parseInt(ASObject* obj,ASObject* const* args, const unsigned int argslen);
ASObject* parseFloat(ASObject* obj,ASObject* const* args, const unsigned int argslen);
ASObject* isNaN(ASObject* obj,ASObject* const* args, const unsigned int argslen);
ASObject* isFinite(ASObject* obj,ASObject* const* args, const unsigned int argslen);
ASObject* unescape(ASObject* obj,ASObject* const* args, const unsigned int argslen);
};

#endif
