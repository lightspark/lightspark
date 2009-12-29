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

#include "flashutils.h"
#include "asobjects.h"
#include "class.h"

using namespace std;
using namespace lightspark;

REGISTER_CLASS_NAME(ByteArray);

ByteArray::ByteArray():bytes(NULL),len(0)
{
}

void ByteArray::sinit(Class_base* c)
{
}

ASObject* lightspark::getQualifiedClassName(ASObject* obj, arguments* args)
{
	assert(args->at(0)->prototype);
	cout << args->at(0)->prototype->class_name << endl;
	return new ASString(args->at(0)->prototype->class_name);
}

