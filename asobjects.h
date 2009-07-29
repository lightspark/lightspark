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

const std::string AS3="http://adobe.com/AS3/2006/builtin";

class Event;
class method_info;
class call_context;

class ASObject: public ISWFObject
{
friend class ABCVm;
public:
	ASObject* prototype;
	ASObject* super;
	ASObject();
	virtual ~ASObject();
	SWFOBJECT_TYPE getObjectType() const { return T_OBJECT; }
	ASFUNCTION(_constructor);
	ASFUNCTION(_toString);
	std::string toString() const;
	ISWFObject* clone()
	{
		return new ASObject(*this);
	}
	ISWFObject* getVariableByName(const Qname& name, ISWFObject*& owner);
	ISWFObject* getVariableByMultiname(const multiname& name, ISWFObject*& owner);
	IFunction* getGetterByName(const Qname& name, ISWFObject*& owner);

	//DEBUG
	int debug_id;
};

class IFunction: public ASObject
{
public:
	typedef ISWFObject* (*as_function)(ISWFObject*, arguments*);
	IFunction():bound(false){}
	ISWFObject* closure_this;
	virtual ISWFObject* call(ISWFObject* obj, arguments* args)=0;
	void bind()
	{
		bound=true;
	}
protected:
	bool bound;
};

class Function : public IFunction
{
public:
	Function(){}
	Function(as_function v):val(v){}
	SWFOBJECT_TYPE getObjectType()const {return T_FUNCTION;}
	ISWFObject* call(ISWFObject* obj, arguments* args);
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
	SWFOBJECT_TYPE getObjectType()const {return T_FUNCTION;}
	ISWFObject* call(ISWFObject* obj, arguments* args);
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
	Boolean(bool v):val(v){}
	bool isEqual(const ISWFObject* r) const;
	SWFOBJECT_TYPE getObjectType() const { return T_BOOLEAN; }
	std::string toString() const;
};

class Undefined : public ASObject
{
public:
	ASFUNCTION(call);
	Undefined();
	SWFOBJECT_TYPE getObjectType() const {return T_UNDEFINED;}
	std::string toString() const;
	bool isEqual(const ISWFObject* r) const;
	ISWFObject* clone()
	{
		return new Undefined;
	}
};

class ASString: public ASObject
{
private:
	std::string data;
public:
	ASString();
	ASString(const std::string& s);
	ASFUNCTION(String);
	std::string toString() const;
	double toNumber() const;
	bool isEqual(const ISWFObject* r) const;
	SWFOBJECT_TYPE getObjectType() const {return T_STRING;}
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
	SWFOBJECT_TYPE getObjectType() const {return T_NULL;}
	std::string toString() const;
	ISWFObject* clone()
	{
		return new Null;
	}
	bool isEqual(const ISWFObject* r) const;
};

class ASArray: public ASObject
{
friend class ABCVm;
private:
	std::vector<ISWFObject*> data;
	Integer length;
public:
	ASArray():length(0)
	{
		constructor=new Function(_constructor);
	}
	SWFOBJECT_TYPE getObjectType() const { return T_ARRAY; }
	virtual ~ASArray();
	ASFUNCTION(_constructor);
	ASFUNCTION(_push);
	ASFUNCTION(shift);
	ISWFObject* clone()
	{
		return new ASArray(*this);
	}
	ISWFObject* at(int index) const
	{
		if(index<data.size())
			return data[index];
		else
			abort();
	}
	ISWFObject*& at(int index)
	{
		if(index<data.size())
			return data[index];
		else
			abort();
	}
	int size() const
	{
		return data.size();
	}
	void push(ISWFObject* o)
	{
		data.push_back(o);
		length=length+1;
	}
	void resize(int n)
	{
		data.resize(n,new Undefined);
		length=n;
	}
	ISWFObject* getVariableByName(const Qname& name, ISWFObject*& owner);
	ISWFObject* getVariableByMultiname(const multiname& name, ISWFObject*& owner);
	ISWFObject* setVariableByName(const Qname& name, ISWFObject* o, bool force=false);
	bool isEqual(const ISWFObject* r) const;
	std::string toString() const;
};

class arguments: public ASArray
{
public:
	arguments(int n)
	{
		resize(n);
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
private:
	double val;
public:
	Number(const ISWFObject* obj);
	Number(double v):val(v){}
	SWFOBJECT_TYPE getObjectType()const {return T_NUMBER;}
	std::string toString() const;
	int toInt() const; 
	double toNumber() const;
	operator double(){return val;}
	ISWFObject* clone()
	{
		return new Number(*this);
	}
	void copyFrom(const ISWFObject* o);
	bool isLess(const ISWFObject* o) const;
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
	ASFUNCTION(getHours);
	ASFUNCTION(getMinutes);
	ASFUNCTION(valueOf);
	ISWFObject* clone()
	{
		return new Date(*this);
	}
};

//Internal objects used to store traits declared in scripts and object placed, but not yet valid
class Definable : public ISWFObject
{
public:
	SWFOBJECT_TYPE getObjectType() const {return T_DEFINABLE;}
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

class MethodDefinable: public Definable
{
private:
	IFunction::as_function f;
	std::string name;
public:
	MethodDefinable(const std::string& _n,IFunction::as_function _f):name(_n),f(_f){}
	void define(ISWFObject* g)
	{
		g->setVariableByName(name,new Function(f));
	}
};

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
	ASFUNCTION(floor);
	ASFUNCTION(sqrt);
	ASFUNCTION(random);
};

#endif
