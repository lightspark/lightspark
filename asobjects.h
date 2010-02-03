/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009  Alessandro Pignotti (a.pignotti@sssup.it)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#ifndef ASOBJECTS_H
#define ASOBJECTS_H
#include <vector>
#include <list>
#include "swftypes.h"
#include "frame.h"
#include "input.h"
#include "compat.h"

namespace lightspark
{
	class SystemState;
};

extern TLSDATA lightspark::SystemState* sys;

namespace lightspark
{
const tiny_string AS3="http://adobe.com/AS3/2006/builtin";

class Event;
class method_info;
struct call_context;

class DebugTracer: public ASObject
{
private:
	tiny_string tracer_name;
public:
	DebugTracer(const tiny_string& n):tracer_name(n){}
	objAndLevel getVariableByMultiname(const multiname& name)
	{
		LOG(LOG_CALLS,"DebugTracer " << tracer_name << ": getVariableByMultiname " << name);
		return ASObject::getVariableByMultiname(name);
	}
	intptr_t getVariableByMultiname_i(const multiname& name)
	{
		LOG(LOG_CALLS,"DebugTracer " << tracer_name << ": getVariableByMultiname_i " << name);
		return ASObject::getVariableByMultiname_i(name);
	}
	objAndLevel getVariableByQName(const tiny_string& name, const tiny_string& ns)
	{
		LOG(LOG_CALLS,"DebugTracer " << tracer_name << ": getVariableByQName " << name << " on namespace " << ns);
		return ASObject::getVariableByQName(name,ns);
	}
	void setVariableByMultiname_i(const multiname& name, intptr_t value)
	{
		LOG(LOG_CALLS,"DebugTracer " << tracer_name << ": setVariableByMultiname_i " << name << " to value " << value);
		ASObject::setVariableByMultiname_i(name,value);
	}
	void setVariableByMultiname(const multiname& name, ASObject* o)
	{
		LOG(LOG_CALLS,"DebugTracer " << tracer_name << ": setVariableByMultiname " << name << " to an object");
		ASObject::setVariableByMultiname(name,o);
	}
	void setVariableByQName(const tiny_string& name, const tiny_string& ns, ASObject* o)
	{
		LOG(LOG_CALLS,"DebugTracer " << tracer_name << ": setVariableByQName " << name << " on namespace " << ns);
		ASObject::setVariableByQName(name,ns,o);
	}
};

class InterfaceClass: public IInterface
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
public:
	Class_base* super;
	IFunction* constructor;
	//We need to know what is the context we are referring to
	ABCContext* context;
	int class_index;
	Class_base(const tiny_string& name):super(NULL),constructor(NULL),context(NULL),class_name(name),class_index(-1),
		use_protected(false){type=T_CLASS;}
	~Class_base();
	virtual IInterface* getInstance(bool construct, arguments* args)=0;
	tiny_string class_name;
	objAndLevel getVariableByMultiname(const multiname& name)
	{
		objAndLevel ret=ASObject::getVariableByMultiname(name);
		if(ret.obj==NULL && super)
			ret=super->getVariableByMultiname(name);
		return ret;
	}
	intptr_t getVariableByMultiname_i(const multiname& name)
	{
		abort();
		return 0;
/*		intptr_t ret=ASObject::getVariableByMultiname(name);
		if(==NULL && super)
			ret=super->getVariableByMultiname(name);
		return ret;*/
	}
	objAndLevel getVariableByQName(const tiny_string& name, const tiny_string& ns)
	{
		objAndLevel ret=ASObject::getVariableByQName(name,ns);
		if(ret.obj==NULL && super)
			ret=super->getVariableByQName(name,ns);
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
	void linkInterface(ASObject* obj) const;
	bool isSubClass(const Class_base* cls) const;
	tiny_string getQualifiedClassName() const;
	virtual tiny_string toString() const;
};

class Class_object: public Class_base
{
private:
	Class_object():Class_base("Class"){};
	IInterface* getInstance(bool construct, arguments* args)
	{
		abort();
	}
	void buildInstanceTraits(ASObject* o) const
	{
		abort();
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
	IInterface* getInstance(bool construct, arguments* args)
	{
		abort();
	}
	void buildInstanceTraits(ASObject* o) const
	{
		abort();
	}
public:
	Class_function(IFunction* _f, ASObject* _p):f(_f),Class_base("Function"),asprototype(_p){}
	tiny_string class_name;
	objAndLevel getVariableByMultiname(const multiname& name)
	{
		objAndLevel ret=Class_base::getVariableByMultiname(name);
		if(ret.obj==NULL && asprototype)
			ret=asprototype->getVariableByMultiname(name);
		return ret;
	}
	intptr_t getVariableByMultiname_i(const multiname& name)
	{
		abort();
		return 0;
/*		intptr_t ret=ASObject::getVariableByMultiname(name);
		if(ret==NULL && super)
			ret=super->getVariableByMultiname(name);
		return ret;*/
	}
	objAndLevel getVariableByQName(const tiny_string& name, const tiny_string& ns)
	{
		objAndLevel ret=Class_base::getVariableByQName(name,ns);
		if(ret.obj==NULL && asprototype)
			ret=asprototype->getVariableByQName(name,ns);
		return ret;
	}
	void setVariableByMultiname_i(const multiname& name, intptr_t value)
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
	}
};

class IFunction: public ASObject
{
public:
	IFunction():bound(false),closure_this(NULL){type=T_FUNCTION;}
	typedef ASObject* (*as_function)(ASObject*, arguments*);
	virtual ASObject* call(ASObject* obj, arguments* args, int level)=0;
	virtual ASObject* fast_call(ASObject* obj, ASObject** args,int num_args, int level)=0;
	IFunction* bind(ASObject* c)
	{
		if(!bound)
		{
			//If binding with null we are not a class method
			IFunction* ret;
			if(c!=NULL)
			{
				c->incRef();
				ret=clone();
			}
			else
			{
				incRef();
				ret=this;
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
protected:
	virtual IFunction* clone()=0;
	ASObject* closure_this;
	bool bound;
};

class Function : public IFunction
{
public:
	Function(){}
	Function(as_function v):val(v){}
	ASObject* call(ASObject* obj, arguments* args, int level);
	ASObject* fast_call(ASObject* obj, ASObject** args, int num_args, int level);
	IFunction* toFunction();
	bool isEqual(ASObject* r)
	{
		abort();
	}

private:
	as_function val;
	Function* clone()
	{
		return new Function(*this);
	}
};

class SyntheticFunction : public IFunction
{
friend class ABCVm;
friend void ASObject::handleConstruction(arguments* args, bool linkInterfaces, bool buildTraits);
public:
	typedef ASObject* (*synt_function)(call_context* cc);
	SyntheticFunction(method_info* m);
	ASObject* call(ASObject* obj, arguments* args, int level);
	ASObject* fast_call(ASObject* obj, ASObject** args,int num_args, int level);
	IFunction* toFunction();
	std::vector<ASObject*> func_scope;
	bool isEqual(ASObject* r)
	{
		SyntheticFunction* sf=dynamic_cast<SyntheticFunction*>(r);
		if(sf==NULL)
			return false;
		return mi==sf->mi;
	}

private:
	int hit_count;
	method_info* mi;
	synt_function val;
	SyntheticFunction* clone()
	{
		return new SyntheticFunction(*this);
	}
};

class Boolean: public ASObject
{
friend bool Boolean_concrete(ASObject* obj);
private:
	bool val;
public:
	Boolean(bool v):val(v){type=T_BOOLEAN;}
	int toInt() const
	{
		return val;
	}
	bool isEqual(ASObject* r);
	tiny_string toString() const;
};

class Undefined : public ASObject
{
public:
	ASFUNCTION(call);
	Undefined();
	tiny_string toString() const;
	bool isEqual(ASObject* r);
	virtual ~Undefined(){}
};

//ASString has to be converted to an Interface... it's still to much used as an object
class ASString: public IInterface
{
friend class ASQName;
friend class ABCContext;
private:
	std::string data;
	void registerMethods();
	tiny_string toString() const;
public:
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	ASString();
	ASString(const std::string& s);
	ASString(const tiny_string& s);
	ASString(const char* s);
	ASFUNCTION(split);
	ASFUNCTION(_getLength);
	ASFUNCTION(replace);
	ASFUNCTION(concat);
	ASFUNCTION(slice);
	ASFUNCTION(indexOf);
	ASFUNCTION(charCodeAt);
	ASFUNCTION(toLowerCase);
	bool isEqual(bool& ret, ASObject* o);
	bool toString(tiny_string& ret);
};

class Null: public ASObject
{
public:
	Null(){type=T_NULL;}
	tiny_string toString() const;
	bool isEqual(ASObject* r);
};

class ASQName: public IInterface
{
friend class ABCContext;
private:
	tiny_string uri;
	tiny_string local_name;
public:
	ASQName(){type=T_QNAME;}
	static void sinit(Class_base*);
	ASFUNCTION(_constructor);
};

class Namespace: public IInterface
{
friend class ASQName;
friend class ABCContext;
private:
	tiny_string uri;
public:
	Namespace(){type=T_NAMESPACE;}
	Namespace(const tiny_string& _uri):uri(_uri){type=T_NAMESPACE;}
	static void sinit(Class_base*);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
};

struct data_slot
{
	STACK_TYPE type;
	union
	{
		ASObject* data;
		intptr_t data_i;
	};
	data_slot(ASObject* o,STACK_TYPE t=STACK_OBJECT):data(o),type(t){}
	data_slot():data(NULL),type(STACK_OBJECT){}
	data_slot(intptr_t i):type(STACK_INT),data_i(i){}
};

class Array: public IInterface
{
friend class ABCVm;
private:
	bool isValidMultiname(const multiname& name, int& index) const;
	bool isValidQName(const tiny_string& name, const tiny_string& ns, int& index);
protected:
	std::vector<data_slot> data;
public:
	Array();
	virtual ~Array();
	static void sinit(Class_base*);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
	ASFUNCTION(_push);
	ASFUNCTION(_concat);
	ASFUNCTION(_pop);
	ASFUNCTION(join);
	ASFUNCTION(shift);
	ASFUNCTION(_sort);
	ASFUNCTION(unshift);
	ASFUNCTION(_getLength);
	ASObject* at(unsigned int index) const
	{
		if(index<data.size())
		{
			if(data[index].data==NULL)
				abort();
			return data[index].data;
		}
		else
			abort();
	}
	void set(unsigned int index, ASObject* o)
	{
		if(index<data.size())
		{
			if(data[index].data)
			{
				std::cout << "overwriting" << std::endl;
				abort();
			}
			data[index].data=o;
			data[index].type=STACK_OBJECT;
		}
		else
			abort();
	}
	int size() const
	{
		return data.size();
	}
	void push(ASObject* o)
	{
		data.push_back(data_slot(o,STACK_OBJECT));
	}
	void resize(int n)
	{
		data.resize(n);
	}
	bool getVariableByQName(const tiny_string& name, const tiny_string& ns, ASObject*& out);
	bool getVariableByMultiname(const multiname& name, ASObject*& out);
	bool getVariableByMultiname_i(const multiname& name, intptr_t& out);
	bool setVariableByQName(const tiny_string& name, const tiny_string& ns, ASObject* o);
	bool setVariableByMultiname(const multiname& name, ASObject* o);
	bool setVariableByMultiname_i(const multiname& name, intptr_t value);
	bool toString(tiny_string& ret);
	bool isEqual(bool& ret, ASObject* r);
	bool hasNext(int& index, bool& out);
	bool nextName(int index, ASObject*& out)
	{
		abort();
	}
	bool nextValue(int index, ASObject*& out);
	tiny_string toString() const;
};

class arguments: public Array
{
public:
	arguments(int n)
	{
		data.resize(n,NULL);
	}
};

class Integer : public ASObject
{
friend class Number;
friend class Array;
friend class ABCVm;
friend class ABCContext;
friend ASObject* abstract_i(intptr_t i);
private:
	int val;
	//int debug;
public:
	Integer(int v):val(v){type=T_INTEGER;
		//debug=rand();
		//std::cout << "new " << this << std::endl;
	}
	Integer(Manager* m):val(0),ASObject(m){type=T_INTEGER;
		//debug=rand();
		//std::cout << "new " << this << std::endl;
	}
	virtual ~Integer(){/*std::cout << "deleting "  << debug << std::endl;*/}
	Integer& operator=(int v){val=v; return *this; }
	tiny_string toString() const;
	int toInt() const
	{
		return val;
	}
	double toNumber() const
	{
		return val;
	}
	operator int() const{return val;} 
	bool isLess(ASObject* r);
	bool isGreater(ASObject* r);
	bool isEqual(ASObject* o);
};

class Number : public ASObject
{
friend ASObject* abstract_d(number_t i);
friend class ABCContext;
private:
	double val;
public:
	Number(double v):val(v){type=T_NUMBER;}
	Number(Manager* m):val(0),ASObject(m){type=T_NUMBER;}
	tiny_string toString() const;
	int toInt() const
	{
		return int(val);
	}
	double toNumber() const
	{
		return val;
	}
	operator double() const {return val;}
	bool isLess(ASObject* o);
	bool isGreater(ASObject* o);
	bool isEqual(ASObject* o);
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

class Date: public IInterface
{
private:
	int year;
	int month;
	int date;
	int hour;
	int minute;
	int second;
	int millisecond;
	int toInt() const;
public:
	Date();
	static void sinit(Class_base*);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
	ASFUNCTION(getTimezoneOffset);
	ASFUNCTION(getTime);
	ASFUNCTION(getFullYear);
	ASFUNCTION(getHours);
	ASFUNCTION(getMinutes);
	ASFUNCTION(valueOf);
	bool toString(tiny_string& ret);
	tiny_string toString() const;
	bool toInt(int& ret);
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

//Deprecated
/*class MethodDefinable: public Definable
{
private:
	IFunction::as_function f;
	std::string name;
public:
	MethodDefinable(const std::string& _n,IFunction::as_function _f):name(_n),f(_f){}
	void define(ASObject* g)
	{
		g->setVariableByQName(name,new Function(f));
	}
};*/

class PlaceObject2Tag;

class DictionaryDefinable: public Definable
{
private:
	int dict_id;
	PlaceObject2Tag* p;
public:
	DictionaryDefinable(int d, PlaceObject2Tag* _p):dict_id(d),p(_p){}
	void define(ASObject* g);
};

class Math: public IInterface
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
	ASFUNCTION(random);
	ASFUNCTION(_max);
	ASFUNCTION(_min);
	ASFUNCTION(pow);
};

class RegExp: public IInterface
{
friend class ASString;
private:
	std::string re;
	std::string flags;
public:
	RegExp();
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
	ASFUNCTION(exec);
	ASFUNCTION(replace);
};

};

#endif
