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

class ASObject: public ISWFObject
{
friend class ABCVm;
friend class ABCContext;
private:
	//Used during binding to inject an object in the hierarchy
	ASObject* super_inject;
public:
	ASObject* prototype;
	ASObject* super;
	ASObject();
	ASObject(const std::string& c);
	ASObject(Manager* m);
	virtual ~ASObject();
	ASFUNCTION(_constructor);
	ASFUNCTION(_toString);
	tiny_string toString() const;
	ISWFObject* clone()
	{
		return new ASObject(*this);
	}
	ISWFObject* getVariableByQName(const tiny_string& name, const tiny_string& ns, ISWFObject*& owner);
	ISWFObject* getVariableByMultiname(const multiname& name, ISWFObject*& owner);
	void setVariableByMultiname(multiname& name, ISWFObject* o);

	//DEBUG
	int debug_id;
};

class IFunction: public ASObject
{
public:
	IFunction():bound(false){type=T_FUNCTION;}
	typedef ISWFObject* (*as_function)(ISWFObject*, arguments*);
	ISWFObject* closure_this;
	virtual ISWFObject* call(ISWFObject* obj, arguments* args)=0;
	virtual ISWFObject* fast_call(ISWFObject* obj, ISWFObject** args,int num_args)=0;
	void bind()
	{
		bound=true;
		std::cout << "Binding" << std::endl;
	}
protected:
	bool bound;
};

class Function : public IFunction
{
public:
	Function(){}
	Function(as_function v):val(v){}
	ISWFObject* call(ISWFObject* obj, arguments* args);
	ISWFObject* fast_call(ISWFObject* obj, ISWFObject** args,int num_args);
	IFunction* toFunction();
	ISWFObject* clone()
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
	typedef ISWFObject* (*synt_function)(ISWFObject*, arguments*, call_context* cc);
	SyntheticFunction(method_info* m);
	ISWFObject* call(ISWFObject* obj, arguments* args);
	ISWFObject* fast_call(ISWFObject* obj, ISWFObject** args,int num_args);
	IFunction* toFunction();
	ISWFObject* clone()
	{
		return new SyntheticFunction(*this);
	}
	std::vector<ISWFObject*> func_scope;

private:
	method_info* mi;
	synt_function val;
};

class Boolean: public ASObject
{
friend bool Boolean_concrete(ISWFObject* obj);
private:
	bool val;
public:
	Boolean(bool v):val(v){type=T_BOOLEAN;}
	int toInt() const
	{
		return val;
	}
	bool isEqual(const ISWFObject* r) const;
	tiny_string toString() const;
};

class Undefined : public ASObject
{
public:
	ASFUNCTION(call);
	Undefined();
	tiny_string toString() const;
	bool isEqual(const ISWFObject* r) const;
	ISWFObject* clone()
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
	tiny_string toString() const;
	double toNumber() const;
	bool isEqual(const ISWFObject* r) const;
	ISWFObject* clone()
	{
		return new ASString(*this);
	}
};

class ASStage: public ASObject
{
private:
	Integer width;
	Integer height;
public:
	ASStage();
};

class Null : public ASObject
{
public:
	Null(){type=T_NULL;}
	tiny_string toString() const;
	ISWFObject* clone()
	{
		return new Null;
	}
	bool isEqual(const ISWFObject* r) const;
};

struct data_slot
{
	STACK_TYPE type;
	union
	{
		ISWFObject* data;
		intptr_t data_i;
	};
	data_slot(ISWFObject* o,STACK_TYPE t=STACK_OBJECT):data(o),type(t){}
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
	ISWFObject* clone()
	{
		return new ASArray(*this);
	}
	ISWFObject* at(int index) const
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
	void set(int index, ISWFObject* o)
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
	void push(ISWFObject* o)
	{
		data.push_back(data_slot(o,STACK_OBJECT));
	}
	void resize(int n)
	{
		data.resize(n);
	}
	ISWFObject* getVariableByQName(const tiny_string& name, const tiny_string& ns, ISWFObject*& owner);
	ISWFObject* getVariableByMultiname(const multiname& name, ISWFObject*& owner);
	intptr_t getVariableByMultiname_i(const multiname& name, ISWFObject*& owner);
	void setVariableByQName(const tiny_string& name, const tiny_string& ns, ISWFObject* o);
	void setVariableByMultiname(multiname& name, ISWFObject* o);
	void setVariableByMultiname_i(multiname& name, intptr_t value);
	bool isEqual(const ISWFObject* r) const;
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
	ISWFObject* clone()
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

class Number : public ASObject
{
friend ISWFObject* abstract_d(number_t i);
private:
	double val;
public:
	Number(double v):val(v){type=T_NUMBER;}
	Number(Manager* m):ASObject(m),val(0){type=T_NUMBER;}
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
	ISWFObject* clone()
	{
		return new Number(*this);
	}
	void copyFrom(const ISWFObject* o);
	bool isLess(const ISWFObject* o) const;
	bool isGreater(const ISWFObject* o) const;
	bool isEqual(const ISWFObject* o) const;
};

class ASMovieClipLoader: public ASObject
{
public:
	ASMovieClipLoader();
	ASFUNCTION(addListener);
	ASFUNCTION(constructor);

	ISWFObject* clone()
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
	ISWFObject* clone()
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
	ISWFObject* clone()
	{
		return new Date(*this);
	}
};

//Internal objects used to store traits declared in scripts and object placed, but not yet valid
class Definable : public ISWFObject
{
public:
	Definable(){type=T_DEFINABLE;}
	virtual void define(ISWFObject* g)=0;
};

class ScriptDefinable: public Definable
{
private:
	IFunction* f;
public:
	ScriptDefinable(IFunction* _f):f(_f){}
	//The global object will be passed from the calling context
	void define(ISWFObject* g){ f->call(g,NULL); }
};

//Deprecated
/*class MethodDefinable: public Definable
{
private:
	IFunction::as_function f;
	std::string name;
public:
	MethodDefinable(const std::string& _n,IFunction::as_function _f):name(_n),f(_f){}
	void define(ISWFObject* g)
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
	void define(ISWFObject* g);
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
	ISWFObject* clone()
	{
		return new RegExp(*this);
	}
};

#endif
