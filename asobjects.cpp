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

#include "asobjects.h"

ASStage::ASStage():width(640),height(480)
{
	setVariableByName("width",SWFObject(&width,true));
	setVariableByName("height",SWFObject(&height,true));
}

void ASArray::_register()
{
	setVariableByName("constructor",SWFObject(new Function(constructor),true));
}

SWFObject ASArray::constructor(const SWFObject& th, arguments* args)
{
	LOG(CALLS,"Called Array constructor");
	return SWFObject();
}

void ASMovieClipLoader::_register()
{
	setVariableByName("constructor",SWFObject(new Function(constructor),true));
}

SWFObject ASMovieClipLoader::constructor(const SWFObject&, arguments* args)
{
	LOG(CALLS,"Called MoviewClipLoader constructor");
	return SWFObject();
}

ASXML::ASXML()
{
	_register();
}

void ASXML::_register()
{
	setVariableByName("constructor",SWFObject(new Function(constructor),true));
}

SWFObject ASXML::constructor(const SWFObject&, arguments* args)
{
	LOG(CALLS,"Called XML constructor");
	return SWFObject();
}

void ASObject::_register()
{
	setVariableByName("constructor",SWFObject(new Function(constructor),true));
}

SWFObject ASObject::constructor(const SWFObject&, arguments* args)
{
	LOG(CALLS,"Called Object constructor");
	return SWFObject();
}

SWFObject ASMovieClip::swapDepths(const SWFObject&, arguments* args)
{
	LOG(CALLS,"Called swapDepths");
	return SWFObject();
}

void ASMovieClip::_register()
{
	setVariableByName("_visible",SWFObject(&_visible,true));
	setVariableByName("_width",SWFObject(&_width,true));
	setVariableByName("swapDepths",SWFObject(new Function(swapDepths),true));
}
