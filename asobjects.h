/**************************************************************************
    Lighspark, a free flash player implementation

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
#include "swftypes.h"

class ASObject: public ISWFObject_impl
{
public:
	void _register();
	SWFOBJECT_TYPE getObjectType() { return T_OBJECT; }
	static SWFObject constructor(const SWFObject& , arguments* args);
	ISWFObject* clone()
	{
		return new ASObject(*this);
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
	static SWFObject constructor(const SWFObject&, arguments* args);
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

class ASMovieClip: public ASObject
{
private:
	Integer _visible;
	Integer _width;

public:
	ASMovieClip():_visible(1),_width(100){}
	static SWFObject swapDepths(const SWFObject&, arguments* args);
	void _register();
};

class ASMovieClipLoader: public ASObject
{
public:
	static SWFObject constructor(const SWFObject&, arguments* args);
	void _register();
	ISWFObject* clone()
	{
		return new ASMovieClipLoader(*this);
	}
};

class ASXML: public ASObject
{
public:
	ASXML();
	static SWFObject constructor(const SWFObject&, arguments* args);
	void _register();
	ISWFObject* clone()
	{
		return new ASXML(*this);
	}
};

#endif
