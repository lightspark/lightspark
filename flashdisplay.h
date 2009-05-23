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

#include "swftypes.h"
#include "abc.h"
#include "asobjects.h"

class LoaderInfo: public ASObject
{
public:
	LoaderInfo(bool build)
	{
		debug_id=0x12345678;
		parameters.debug_id=0x1111;
		if(build)
		{
			constructor();
		}
	}
	void constructor();
	ASObject parameters;
};
