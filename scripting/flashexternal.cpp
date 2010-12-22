/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009,2010  Alessandro Pignotti (a.pignotti@sssup.it)

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

#include "flashexternal.h"
#include "class.h"

using namespace lightspark;

SET_NAMESPACE("flash.external");

REGISTER_CLASS_NAME(ExternalInterface);

void ExternalInterface::sinit(Class_base* c)
{
	c->setConstructor(NULL);
	c->setGetterByQName("available","",Class<IFunction>::getFunction(_getAvailable),false);
	c->setGetterByQName("objectID","",Class<IFunction>::getFunction(_getObjectID),false);
}

ASFUNCTIONBODY(ExternalInterface,_getAvailable)
{
	return abstract_b(sys->extScriptObject != NULL);
}

ASFUNCTIONBODY(ExternalInterface,_getObjectID)
{
	if(sys->extScriptObject == NULL)
		return Class<ASString>::getInstanceS("");

	ExtVariantObject* object = sys->extScriptObject->getProperty("name");
	std::string result = object->toString();
	delete object;
	return Class<ASString>::getInstanceS(result);
}
