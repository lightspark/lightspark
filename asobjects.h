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
#include "swf.h"

const tiny_string AS3="http://adobe.com/AS3/2006/builtin";

extern __thread SystemState* sys;

class Event;
class method_info;
class call_context;

class DebugTracer: public ASObject
{
private:
	tiny_string tracer_name;
public:
	DebugTracer(const tiny_string& n):tracer_name(n){}
	ASObject* getVariableByMultiname(const multiname& name, ASObject*& owner)
	{
		LOG(CALLS,"DebugTracer " << tracer_name << ": getVariableByMultiname " << name);
		return ASObject::getVariableByMultiname(name,owner);
	}
	intptr_t getVariableByMultiname_i(const multiname& name, ASObject*& owner)
	{
		LOG(CALLS,"DebugTracer " << tracer_name << ": getVariableByMultiname_i " << name);
		return ASObject::getVariableByMultiname_i(name,owner);
	}
	ASObject* getVariableByQName(const tiny_string& name, const tiny_string& ns, ASObject*& owner)
	{
		LOG(CALLS,"DebugTracer " << tracer_name << ": getVariableByQName " << name << " on namespace " << ns);
		return ASObject::getVariableByQName(name,ns,owner);
	}
	void setVariableByMultiname_i(const multiname& name, intptr_t value)
	{
		LOG(CALLS,"DebugTracer " << tracer_name << ": setVariableByMultiname_i " << name << " to value " << value);
		ASObject::setVariableByMultiname_i(name,value);
	}
	void setVariableByMultiname(const multiname& name, ASObject* o)
	{
		LOG(CALLS,"DebugTracer " << tracer_name << ": setVariableByMultiname " << name << " to an object");
		ASObject::setVariableByMultiname(name,o);
	}
	void setVariableByQName(const tiny_string& name, const tiny_string& ns, ASObject* o)
	{
		LOG(CALLS,"DebugTracer " << tracer_name << ": setVariableByQName " << name << " on namespace " << ns);
		ASObject::setVariableByQName(name,ns,o);
	}
};

class Class_base: public ASObject
{
friend class ABCVm;
friend class ABCContext;
friend void ASObject::handleConstruction(ABCContext* context,arguments* args, bool linkInterfaces);
private:
	std::vector<multiname> interfaces;
public:
	Class_base* super;
	IFunction* constructor;
	int class_index;
	Class_base(const tiny_string& name):super(NULL),constructor(NULL),class_name(name),class_index(-1){type=T_CLASS;}
	~Class_base();
	virtual IInterface* getInstance(bool construct=false)=0;
	tiny_string class_name;
	ASObject* getVariableByMultiname(const multiname& name, ASObject*& owner)
	{
		ASObject* ret=ASObject::getVariableByMultiname(name,owner);
		if(owner==NULL && super)
			ret=super->getVariableByMultiname(name,owner);
		return ret;
	}
	intptr_t getVariableByMultiname_i(const multiname& name, ASObject*& owner)
	{
		abort();
/*		intptr_t ret=ASObject::getVariableByMultiname(name.owner);
		if(owner==NULL && super)
			ret=super->getVariableByMultiname(name,owner);
		return ret;*/
	}
	ASObject* getVariableByQName(const tiny_string& name, const tiny_string& ns, ASObject*& owner)
	{
		ASObject* ret=ASObject::getVariableByQName(name,ns,owner);
		if(owner==NULL && super)
			ret=super->getVariableByQName(name,ns,owner);
		return ret;
	}
	void addImplementedInterface(const multiname& i);
};

class IFunction: public ASObject
{
public:
	IFunction():bound(false){type=T_FUNCTION;}
	typedef ASObject* (*as_function)(ASObject*, arguments*);
	virtual ASObject* call(ASObject* obj, arguments* args)=0;
	virtual ASObject* fast_call(ASObject* obj, ASObject** args,int num_args)=0;
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
	ASObject* call(ASObject* obj, arguments* args)
	{
		abort();
	}
	ASObject* fast_call(ASObject* obj, ASObject** args,int num_args)
	{
		abort();
	}
	IFunction* toFunction()
	{
		abort();
	}
	Function_Object* clone()
	{
		abort();
	}
	tiny_string toString() const;
};

class Function : public IFunction
{
public:
	Function(){}
	Function(as_function v):val(v){}
	ASObject* call(ASObject* obj, arguments* args);
	ASObject* fast_call(ASObject* obj, ASObject** args,int num_args);
	IFunction* toFunction();
	Function* clone()
	{
		return new Function(*this);
	}

private:
	as_function val;
};

class SyntheticFunction : public IFunction
{
friend class ABCVm;
friend void ASObject::handleConstruction(ABCContext* context,arguments* args, bool linkInterfaces);
public:
	typedef ASObject* (*synt_function)(ASObject*, arguments*, call_context* cc);
	SyntheticFunction(method_info* m);
	ASObject* call(ASObject* obj, arguments* args);
	ASObject* fast_call(ASObject* obj, ASObject** args,int num_args);
	IFunction* toFunction();
	std::vector<ASObject*> func_scope;
	SyntheticFunction* clone()
	{
		return new SyntheticFunction(*this);
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

class ASString: public ASObject
{
private:
	std::string data;
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
	ASFUNCTION(indexOf);
	ASFUNCTION(charCodeAt);
	tiny_string toString() const;
	double toNumber() const;
	bool isEqual(ASObject* r);
};

class ASStage: public ASObject
{
private:
	uintptr_t width;
	uintptr_t height;
public:
	ASStage();
};

class Null : public ASObject
{
public:
	Null(){type=T_NULL;}
	tiny_string toString() const;
	bool isEqual(ASObject* r);
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
protected:
	std::vector<data_slot> data;
public:
	Array();
	virtual ~Array();
	static void sinit(Class_base*);
	ASFUNCTION(_constructor);
	//ASFUNCTION(Array);
	ASFUNCTION(_push);
	ASFUNCTION(_pop);
	ASFUNCTION(join);
	ASFUNCTION(shift);
	ASFUNCTION(unshift);
	ASFUNCTION(_getLength);
	ASObject* at(int index) const
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
	void set(int index, ASObject* o)
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
public:
	Integer(int v):val(v){type=T_INTEGER;}
	Integer(Manager* m):val(0){type=T_INTEGER;}
	virtual ~Integer(){}
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
private:
	double val;
public:
	Number(double v):val(v){type=T_NUMBER;}
	Number(Manager* m):val(0){type=T_NUMBER;}
	tiny_string toString() const;
	int toInt() const
	{
		return val;
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
	tiny_string toString() const;
	int toInt() const; 
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
	void define(ASObject* g){ f->call(g,NULL); }
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
	ASFUNCTION(floor);
	ASFUNCTION(round);
	ASFUNCTION(sqrt);
	ASFUNCTION(random);
	ASFUNCTION(max);
};

class RegExp: public IInterface
{
private:
	std::string re;
	std::string flags;
public:
	RegExp();
	static void sinit(Class_base* c);
	ASFUNCTION(_constructor);
	ASFUNCTION(exec);
};

template<typename T>
class ClassName
{
public:
	static const char* name;
};

#define REGISTER_CLASS_NAME(X) template<> \
	const char* ClassName<X>::name = #X;

#define REGISTER_CLASS_NAME2(X,NAME) template<> \
	const char* ClassName<X>::name = NAME;

class Class_inherit:public Class_base
{
public:
	Class_inherit(const tiny_string& name):Class_base(name){}
	IInterface* getInstance(bool construct=false);
};

template< class T>
class Class: public Class_base
{
private:
	Class(const tiny_string& name):Class_base(name){}
	T* getInstance(bool construct)
	{
		ASObject* obj=new ASObject;
		obj->max_level=max_level;
		obj->prototype=this;
		obj->actualPrototype=this;
		//TODO: Add interface T to ret
		T* ret=new T;
		ret->obj=obj;
		obj->interface=ret;
		//As we are the prototype we should incRef ourself
		incRef();
		if(construct && constructor)
			constructor->call(obj,NULL);
		return ret;
	}
public:
	static T* getInstanceS(bool construct=false)
	{
		Class<T>* c=Class<T>::getClass();
		return c->getInstance(construct);
	}
	template <typename ARG1>
	static T* getInstanceS(ARG1 a1)
	{
		Class<T>* c=Class<T>::getClass();
		ASObject* obj=new ASObject;
		//TODO: Add interface T to ret
		obj->max_level=c->max_level;
		obj->prototype=c;
		obj->actualPrototype=c;
		T* ret=new T(a1);
		ret->obj=obj;
		obj->interface=ret;
		//As we are the prototype we should incRef ourself
		c->incRef();
		return ret;
	}
	static Class<T>* getClass(const tiny_string& name)
	{
		std::map<tiny_string, Class_base*>::iterator it=sys->classes.find(name);
		if(it==sys->classes.end()) //This class is not yet in the map, create it
		{
			Class<T>* ret=new Class<T>(name);
			T::sinit(ret);
			sys->classes.insert(std::make_pair(name,ret));
			return ret;
		}
		else
			return static_cast<Class<T>*>(it->second);
	}
	static Class<T>* getClass()
	{
		return getClass(ClassName<T>::name);
	}
};

#endif
