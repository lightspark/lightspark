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

#include "scripting/flash/system/systemupdater.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include <iomanip>

using namespace lightspark;

void SystemUpdater::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED|CLASS_FINAL);
	c->setDeclaredMethodByQName("cancel","",Class<IFunction>::getFunction(c->getSystemState(),cancel),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("update","",Class<IFunction>::getFunction(c->getSystemState(),update),NORMAL_METHOD,true);
}

ASFUNCTIONBODY_ATOM(SystemUpdater,_constructor)
{
    LOG(LOG_NOT_IMPLEMENTED,"SystemUpdater is not implemented");
}

ASFUNCTIONBODY_ATOM(SystemUpdater,cancel)
{
    LOG(LOG_NOT_IMPLEMENTED,"SystemUpdater.cancel is not implemented");
}

ASFUNCTIONBODY_ATOM(SystemUpdater,update)
{
    LOG(LOG_NOT_IMPLEMENTED,"SystemUpdater.update is not implemented");
}