/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2013  Alessandro Pignotti (a.pignotti@sssup.it)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#ifndef SCRIPTING_FLASH_ACCESSIBILITY_FLASHACCESSIBILITY_H
#define SCRIPTING_FLASH_ACCESSIBILITY_FLASHACCESSIBILITY_H 1

#include "asobject.h"

namespace lightspark
{

class AccessibilityProperties : public ASObject
{
private:
	ASPROPERTY_GETTER_SETTER(tiny_string, description);
	ASPROPERTY_GETTER_SETTER(bool, forceSimple);
	ASPROPERTY_GETTER_SETTER(tiny_string, name);
	ASPROPERTY_GETTER_SETTER(bool, noAutoLabeling);
	ASPROPERTY_GETTER_SETTER(tiny_string, shortcut);
	ASPROPERTY_GETTER_SETTER(bool, silent);
public:
	AccessibilityProperties(ASWorker* wrk,Class_base* c);
	static void sinit(Class_base*);
	ASFUNCTION_ATOM(_constructor);
};

class AccessibilityImplementation : public ASObject
{
public:
	AccessibilityImplementation(ASWorker* wrk,Class_base* c):ASObject(wrk,c){}
	static void sinit(Class_base*);
	ASFUNCTION_ATOM(_constructor);
};

class Accessibility : public ASObject
{
private:
	_NR<AccessibilityProperties> properties;
public:
	Accessibility(ASWorker* wrk,Class_base* c):ASObject(wrk,c),properties(nullptr){}
	static void sinit(Class_base*);
	ASFUNCTION_ATOM(updateProperties);
};

}
#endif /* SCRIPTING_FLASH_ACCESSIBILITY_FLASHACCESSIBILITY_H */
