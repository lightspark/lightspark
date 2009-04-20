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
#include "swftypes.h"

class ASObject: public ISWFObject_impl
{
public:
	void _register();
	SWFOBJECT_TYPE getObjectType() { return T_OBJECT; }
	static void constructor(ASObject* th, arguments* args);
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
	static void constructor(ASArray* th, arguments* args);
	void _register();
	ISWFObject* clone()
	{
		return new ASArray(*this);
	}
};

class ASMovieClip: public ASObject
{
private:
	Integer _visible;
public:
	ASMovieClip():_visible(1){}
	void _register();
};

#endif
