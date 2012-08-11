/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2012  Alessandro Pignotti (a.pignotti@sssup.it)

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

void AccessibilityProperties::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<ASObject>::getRef());
	REGISTER_GETTER_SETTER(c,name);
}

ASFUNCTIONBODY(AccessibilityProperties,_constructor)
{
	LOG(LOG_NOT_IMPLEMENTED, _("AccessibilityProperties class is unimplemented."));
	return NULL;
}

ASFUNCTIONBODY_GETTER_SETTER(AccessibilityProperties,name);

void AccessibilityImplementation::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<ASObject>::getRef());
}

ASFUNCTIONBODY(AccessibilityImplementation,_constructor)
{
	LOG(LOG_NOT_IMPLEMENTED, _("AccessibilityImplementation class is unimplemented."));
	return NULL;
}
