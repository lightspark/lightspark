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

#include "scripting/flash/accessibility/flashaccessibility.h"
#include "scripting/class.h"
#include "scripting/argconv.h"

using namespace lightspark;

AccessibilityProperties::AccessibilityProperties(ASWorker* wrk, Class_base* c):
	ASObject(wrk,c), forceSimple(false), noAutoLabeling(false), silent(false)
{
}

void AccessibilityProperties::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED);
	REGISTER_GETTER_SETTER(c, description);
	REGISTER_GETTER_SETTER(c, forceSimple);
	REGISTER_GETTER_SETTER(c, name);
	REGISTER_GETTER_SETTER(c, noAutoLabeling);
	REGISTER_GETTER_SETTER(c, shortcut);
	REGISTER_GETTER_SETTER(c, silent);
}

ASFUNCTIONBODY_ATOM(AccessibilityProperties,_constructor)
{
}

ASFUNCTIONBODY_GETTER_SETTER(AccessibilityProperties, description)
ASFUNCTIONBODY_GETTER_SETTER(AccessibilityProperties, forceSimple)
ASFUNCTIONBODY_GETTER_SETTER(AccessibilityProperties, name)
ASFUNCTIONBODY_GETTER_SETTER(AccessibilityProperties, noAutoLabeling)
ASFUNCTIONBODY_GETTER_SETTER(AccessibilityProperties, shortcut)
ASFUNCTIONBODY_GETTER_SETTER(AccessibilityProperties, silent)

void AccessibilityImplementation::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED);
}

ASFUNCTIONBODY_ATOM(AccessibilityImplementation,_constructor)
{
	LOG(LOG_NOT_IMPLEMENTED, "AccessibilityImplementation class is unimplemented.");
}


void Accessibility::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructorNotInstantiatable, CLASS_FINAL);
	c->setVariableByQName("active","",abstract_b(c->getSystemState(),false),CONSTANT_TRAIT);
}

ASFUNCTIONBODY_ATOM(Accessibility,updateProperties)
{
	Accessibility* th=asAtomHandler::as<Accessibility>(obj);
	LOG(LOG_NOT_IMPLEMENTED, "Accessibility is not supported.");
	ARG_CHECK(ARG_UNPACK(th->properties));
}
