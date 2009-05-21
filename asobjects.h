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

class ASObject: public ISWFObject_impl
{
public:
	ASObject():debug_id(0){}
	void _register();
	SWFOBJECT_TYPE getObjectType() { return T_OBJECT; }
	ASFUNCTION(constructor);
	ISWFObject* clone()
	{
		return new ASObject(*this);
	}

	//DEBUG
	int debug_id;
};

class ASString: public ASObject
{
private:
	std::string data;
public:
	ASString(const std::string& s):data(s){}
	STRING toString();
	float toFloat();
	SWFOBJECT_TYPE getObjectType(){return T_STRING;}
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
public:
	ASFUNCTION(constructor);
	void _register();
	ISWFObject* clone()
	{
		return new ASArray(*this);
	}
};

typedef Integer Number; //TODO: Implement number for real

class arguments: public ASArray
{
public:
	std::vector<SWFObject> args;
//public:
//	static SWFObject push(arguments* th, arguments* 
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

class ASMovieClip: public ASObject, public IRenderObject
{
private:
	static bool list_orderer(const IDisplayListElem* a, int d);
protected:
	Integer _visible;
	Integer _width;
	Integer _framesloaded;
	Integer _totalframes;
	std::list < IDisplayListElem* > dynamicDisplayList;
	std::list < IDisplayListElem* > displayList;
public:
	//Frames mutex (shared with drawing thread)
	sem_t sem_frames;
	std::list<Frame> frames;
	RunState state;

public:
	int hack;
	int displayListLimit;

	ASMovieClip();
	ASFUNCTION(moveTo);
	ASFUNCTION(lineStyle);
	ASFUNCTION(lineTo);
	ASFUNCTION(swapDepths);
	ASFUNCTION(createEmptyMovieClip);

	virtual void addToDisplayList(IDisplayListElem* r);

	//ASObject interface
	void _register();

	//IRenderObject interface
	void Render();
	ISWFObject* clone()
	{
		return new ASMovieClip(*this);
	}
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
	ASFUNCTION(constructor)
	ASFUNCTION(load)
	void _register();
	ISWFObject* clone()
	{
		return new ASXML(*this);
	}
};

#endif
