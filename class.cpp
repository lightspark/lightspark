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

#include "class.h"

using namespace lightspark;

ASObject* Class<ASObject>::getVariableByMultiname(const multiname& name, bool skip_impl, bool enableOverride, ASObject* base)
{
	ASObject* ret=ASObject::getVariableByMultiname(name, skip_impl, enableOverride, base);
	//No super here, ever
	if(!ret)
	{
		//Check if we should do lazy definition
		if(name.name_s=="toString")
		{
			ASObject* ret=Class<IFunction>::getFunction(ASObject::_toString);
			setVariableByQName("toString","",ret);
			return ret;
		}
		else if(name.name_s=="hasOwnProperty")
		{
			ASObject* ret=Class<IFunction>::getFunction(ASObject::hasOwnProperty);
			setVariableByQName("hasOwnProperty","",ret);
			return ret;
		}

	}
	return ret;
}

