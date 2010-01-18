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
	objAndLevel getVariableByMultiname(const multiname& name, ASObject*& owner)
	{
		LOG(LOG_CALLS,"DebugTracer " << tracer_name << ": getVariableByMultiname " << name);
		return ASObject::getVariableByMultiname(name,owner);
	}
	intptr_t getVariableByMultiname_i(const multiname& name, ASObject*& owner)
	{
		LOG(LOG_CALLS,"DebugTracer " << tracer_name << ": getVariableByMultiname_i " << name);
		return ASObject::getVariableByMultiname_i(name,owner);
	}
	objAndLevel getVariableByQName(const tiny_string& name, const tiny_string& ns, ASObject*& owner)
	{
		LOG(LOG_CALLS,"DebugTracer " << tracer_name << ": getVariableByQName " << name << " on namespace " << ns);
		return ASObject::getVariableByQName(name,ns,owner);
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

class Class_base: public ASObject
{
friend class ABCVm;
friend class ABCContext;
friend void ASObject::handleConstruction(arguments* args, bool linkInterfaces, bool buildTraits);
private:
	std::vector<multiname> interfaces;
	std::vector<Class_base*> interfaces_added;
public:
	Class_base* super;
	IFunction* constructor;
	//We need to know what is the context we are referring to
	ABCContext* context;
	int class_index;
	Class_base(const tiny_string& name):super(NULL),constructor(NULL),context(NULL),class_name(name),class_index(-1){type=T_CLASS;}
	~Class_base();
	virtual IInterface* getInstance(bool construct=false)=0;
	tiny_string class_name;
	objAndLevel getVariableByMultiname(const multiname& name, ASObject*& owner)
	{
		objAndLevel ret=ASObject::getVariableByMultiname(name,owner);
		if(owner==NULL && super)
			ret=super->getVariableByMultiname(name,owner);
		return ret;
	}
	intptr_t getVariableByMultiname_i(const multiname& name, ASObject*& owner)
	{
		abort();
		return 0;
/*		intptr_t ret=ASObject::getVariableByMultiname(name.owner);
		if(owner==NULL && super)
			ret=super->getVariableByMultiname(name,owner);
		return ret;*/
	}
	objAndLevel getVariableByQName(const tiny_string& name, const tiny_string& ns, ASObject*& owner)
	{
		objAndLevel ret=ASObject::getVariableByQName(name,ns,owner);
		if(owner==NULL && super)
			ret=super->getVariableByQName(name,ns,owner);
		return ret;
	}
	void addImplementedInterface(const multiname& i);
	virtual void buildInstanceTraits(ASObject* o) const=0;
};

class Class_object: public Class_base
{
private:
	Class_object():Class_base("Class"){};
public:
	IInterface* getInstance(bool construct=false)
	{
		abort();
	}
	static Class_object* getClass();
	void buildInstanceTraits(ASObject* o) const
	{
		abort();
	}
};

class IFunction: public ASObject
{
public:
	IFunction():bound(false){type=T_FUNCTION;}
	typedef ASObject* (*as_function)(ASObject*, arguments*);
	virtual ASObject* call(ASObject* obj, arguments* args, int level)=0;
	virtual ASObject* fast_call(ASObject* obj, ASObject** args,int num_args, int level)=0;
	virtual IFunction* clone()=0;
	void bind(ASObject* c)
	{
		bound=true;
		closure_this=c;
		std::cout << "Binding " << this << std::endl;
	}
protected:
	ASObject* closure_this;
	bool bound;
};

class Function_Object: public IFunction
{
public:
	//The interface of a function, but the behaviour of an object
	Function_Object(){type=T_OBJECT;}
	ASObject* call(ASObject* obj, arguments* args, int level)
	{
		abort();
		return NULL;
	}
	ASObject* fast_call(ASObject* obj, ASObject** args,int num_args, int level)
	{
		abort();
		return NULL;
	}
	IFunction* toFunction()
	{
		abort();
		return NULL;
	}
	Function_Object* clone()
	{
		abort();
		return NULL;
	}
	tiny_string toString() const;
};

class Function : public IFunction
{
public:
	Function(){}
	Function(as_function v):val(v){}
	ASObject* call(ASObject* obj, arguments* args, int level);
	ASObject* fast_call(ASObject* obj, ASObject** args, int num_args, int level);
	IFunction* toFunction();
	Function* clone()
	{
		return new Function(*this);
	}
	bool isEqual(ASObject* r)
	{
		abort();
	}

private:
	as_function val;
};

class SyntheticFunction : public IFunction
{
friend class ABCVm;
friend void ASObject::handleConstruction(arguments* args, bool linkInterfaces, bool buildTraits);
public:
	typedef ASObject* (*synt_function)(ASObject*, arguments*, call_context* cc);
	SyntheticFunction(method_info* m);
	ASObject* call(ASObject* obj, arguments* args, int level);
	ASObject* fast_call(ASObject* obj, ASObject** args,int num_args, int level);
	IFunction* toFunction();
	std::vector<ASObject*> func_scope;
	SyntheticFunction* clone()
	{
		return new SyntheticFunction(*this);
	}
	bool isEqual(ASObject* r)
	{
		SyntheticFunction* sf=dynamic_cast<SyntheticFunction*>(r);
		if(sf==NULL)
			return false;
		return mi==sf->mi;
	}

private:
	method_info* mi;
	synt_function val;
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
class ASString: public ASObject
{
friend class ASQName;
friend class ABCContext;
private:
	std::string data;
	void registerMethods();
public:
	ASString();
	ASString(const std::string& s);
	ASString(const tiny_string& s);
	ASString(const char* s);
	ASFUNCTION(String);
	ASFUNCTION(split);
	ASFUNCTION(_getLength);
	ASFUNCTION(replace);
	ASFUNCTION(concat);
	ASFUNCTION(slice);
	ASFUNCTION(indexOf);
	ASFUNCTION(charCodeAt);
	ASFUNCTION(toLowerCase);
	tiny_string toString() const;
	double toNumber() const;
	bool isEqual(ASObject* r);
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
	tiny_string toString() const;
};

class arguments: public Array
{
public:
	arguments(int n, bool construct=true)
	{
		data.resize(n,NULL);
//		if(construct)
//			_constructor(this,NULL);
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

class Math: public ASObject
{
public:
	Math();
	ASFUNCTION(atan2);
	ASFUNCTION(abs);
	ASFUNCTION(sin);
	ASFUNCTION(cos);
	ASFUNCTION(floor);
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
	ASFUNCTION(_constructor);
	ASFUNCTION(exec);
	ASFUNCTION(replace);
};

bool isSubclass(ASObject* obj, const Class_base* c);

};

#endif
