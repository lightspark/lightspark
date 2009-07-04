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

class Event;

class ASObjectWrapper: public IDisplayListElem
{
private:
	IRenderObject* wrapped;
	UI16 depth;
public:
	ASObjectWrapper(IRenderObject* w, UI16 d):wrapped(w),depth(d){}
	int getDepth() const
	{
		return depth;
	}
	void Render()
	{
		if(wrapped)
			wrapped->Render();
	}
};

class ASObject: public ISWFObject
{
friend class ABCVm;
public:
	ASObject* prototype;
	ASObject* super;
	ASObject():debug_id(0),prototype(NULL),super(NULL){}
	void _register();
	SWFOBJECT_TYPE getObjectType() const { return T_OBJECT; }
	//ASFUNCTION(constructor);
	ISWFObject* clone()
	{
		return new ASObject(*this);
	}
	ISWFObject* getVariableByName(const Qname& name, bool& found);

	//DEBUG
	int debug_id;
};

class IFunction: public ASObject
{
public:
	virtual ISWFObject* call(ISWFObject* obj, arguments* args)=0;
};

class Function : public IFunction
{
public:
	typedef ISWFObject* (*as_function)(ISWFObject*, arguments*);
	Function(as_function v):val(v),bound(false){}
	SWFOBJECT_TYPE getObjectType()const {return T_FUNCTION;}
	ISWFObject* call(ISWFObject* obj, arguments* args);
	IFunction* toFunction();
	ISWFObject* clone()
	{
		return new Function(*this);
	}
	void bind()
	{
		bound=true;
	}
	ISWFObject* closure_this;

private:
	as_function val;
	bool bound;
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
	virtual ~ASArray();
	ASFUNCTION(_constructor);
	ISWFObject* clone()
	{
		return new ASArray(*this);
	}
	ISWFObject* at(int index) const
	{
		return data[index];
	}
	ISWFObject*& at(int index)
	{
		return data[index];
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
		data.resize(n);
		length=n;
	}
	ISWFObject* getVariableByName(const Qname& name, bool& found);
};

class arguments: public ASArray
{
public:
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
	void tick()
	{
		if(!stop_FP)
			FP=next_FP;
	}
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

	void _register();
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
	void _register();
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
	Function::as_function f;
public:
	ScriptDefinable(Function::as_function _f):f(_f){}
	//The global object will be passed from the calling context
	void define(ISWFObject* g){ f(g,NULL); }
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
#endif
