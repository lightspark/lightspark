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

const tiny_string AS3="http://adobe.com/AS3/2006/builtin";

class Event;
class method_info;
class call_context;

class IFunction: public ASObject
{
public:
	IFunction():bound(false),ASObject("IFunction",this){type=T_FUNCTION;}
	typedef ASObject* (*as_function)(ASObject*, arguments*);
	virtual ASObject* call(ASObject* obj, arguments* args)=0;
	virtual ASObject* fast_call(ASObject* obj, ASObject** args,int num_args)=0;
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

class Function : public IFunction
{
public:
	Function(){}
	Function(as_function v):val(v){}
	ASObject* call(ASObject* obj, arguments* args);
	ASObject* fast_call(ASObject* obj, ASObject** args,int num_args);
	IFunction* toFunction();
	ASObject* clone()
	{
		return new Function(*this);
	}

private:
	as_function val;
};

class SyntheticFunction : public IFunction
{
friend class ABCVm;
public:
	typedef ASObject* (*synt_function)(ASObject*, arguments*, call_context* cc);
	SyntheticFunction(method_info* m);
	ASObject* call(ASObject* obj, arguments* args);
	ASObject* fast_call(ASObject* obj, ASObject** args,int num_args);
	IFunction* toFunction();
	ASObject* clone()
	{
		return new SyntheticFunction(*this);
	}
	std::vector<ASObject*> func_scope;

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
	Boolean(bool v):val(v),ASObject("Boolean",this){type=T_BOOLEAN;}
	int toInt() const
	{
		return val;
	}
	bool isEqual(const ASObject* r) const;
	tiny_string toString() const;
};

class Undefined : public ASObject
{
public:
	ASFUNCTION(call);
	Undefined();
	tiny_string toString() const;
	bool isEqual(const ASObject* r) const;
	ASObject* clone()
	{
		return new Undefined;
	}
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
	ASFUNCTION(indexOf);
	ASFUNCTION(charCodeAt);
	tiny_string toString() const;
	double toNumber() const;
	bool isEqual(const ASObject* r) const;
	ASObject* clone()
	{
		return new ASString(*this);
	}
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
	Null():ASObject("Null",this){type=T_NULL;}
	tiny_string toString() const;
	ASObject* clone()
	{
		return new Null;
	}
	bool isEqual(const ASObject* r) const;
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

class ASArray: public ASObject
{
friend class ABCVm;
protected:
	std::vector<data_slot> data;
public:
	ASArray();
	virtual ~ASArray();
	ASFUNCTION(_constructor);
	ASFUNCTION(Array);
	ASFUNCTION(_push);
	ASFUNCTION(_pop);
	ASFUNCTION(join);
	ASFUNCTION(shift);
	ASFUNCTION(unshift);
	ASFUNCTION(_getLength);
	ASObject* clone()
	{
		return new ASArray(*this);
	}
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
	ASObject* getVariableByQName(const tiny_string& name, const tiny_string& ns, ASObject*& owner);
	ASObject* getVariableByMultiname(const multiname& name, ASObject*& owner);
	intptr_t getVariableByMultiname_i(const multiname& name, ASObject*& owner);
	void setVariableByQName(const tiny_string& name, const tiny_string& ns, ASObject* o);
	void setVariableByMultiname(multiname& name, ASObject* o);
	void setVariableByMultiname_i(multiname& name, intptr_t value);
	bool isEqual(const ASObject* r) const;
	tiny_string toString() const;
};

class arguments: public ASArray
{
public:
	arguments(int n, bool construct=true)
	{
		data.resize(n,NULL);
		if(construct)
			_constructor(this,NULL);
	}
	ASObject* clone()
	{
		return new arguments(*this);
	}
};

class RunState
{
public:
	int FP;
	int next_FP;
	int max_FP;
	bool stop_FP;
public:
	RunState();
	void prepareNextFP();
};

class Integer : public ASObject
{
friend class Number;
friend class ASArray;
friend class ABCVm;
friend class ABCContext;
friend ASObject* abstract_i(intptr_t i);
private:
	int val;
public:
	Integer(int v):ASObject("Integer",this),val(v){type=T_INTEGER;}
	Integer(Manager* m):ASObject("Integer",this,m),val(0){type=T_INTEGER;}
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
	bool isLess(const ASObject* r) const;
	bool isGreater(const ASObject* r) const;
	bool isEqual(const ASObject* o) const;
	ASObject* clone()
	{
		return new Integer(*this);
	}
};

class Number : public ASObject
{
friend ASObject* abstract_d(number_t i);
private:
	double val;
public:
	Number(double v):ASObject("Number",this),val(v){type=T_NUMBER;}
	Number(Manager* m):ASObject("Number",this,m),val(0){type=T_NUMBER;}
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
	ASObject* clone()
	{
		return new Number(*this);
	}
	void copyFrom(const ASObject* o);
	bool isLess(const ASObject* o) const;
	bool isGreater(const ASObject* o) const;
	bool isEqual(const ASObject* o) const;
};

class ASMovieClipLoader: public ASObject
{
public:
	ASMovieClipLoader();
	ASFUNCTION(addListener);
	ASFUNCTION(constructor);

	ASObject* clone()
	{
		return new ASMovieClipLoader(*this);
	}
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
	ASObject* clone()
	{
		return new ASXML(*this);
	}
};

class Date: public ASObject
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
	ASFUNCTION(_constructor);
	ASFUNCTION(getTimezoneOffset);
	ASFUNCTION(getTime);
	ASFUNCTION(getFullYear);
	ASFUNCTION(getHours);
	ASFUNCTION(getMinutes);
	ASFUNCTION(valueOf);
	tiny_string toString() const;
	int toInt() const; 
	ASObject* clone()
	{
		return new Date(*this);
	}
};

//Internal objects used to store traits declared in scripts and object placed, but not yet valid
class Definable : public ASObject
{
public:
	Definable():ASObject("Definable",this){type=T_DEFINABLE;}
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
	ASFUNCTION(floor);
	ASFUNCTION(sqrt);
	ASFUNCTION(random);
};

class RegExp: public ASObject
{
private:
	std::string re;
	std::string flags;
public:
	RegExp();
	ASFUNCTION(_constructor);
	RegExp* clone()
	{
		return new RegExp(*this);
	}
};

#endif
