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

#include "flashgeom.h"

Rectangle::Rectangle(int l, int t, int r, int b)
{
/*	setVariableByName("left",new Integer(l));
	setVariableByName("top",new Integer(t));
	setVariableByName("top",new Integer(t));*/
	//TODO: Use getters
	setVariableByName("xMax",new Integer(r));
	setVariableByName("xMin",new Integer(l));
	setVariableByName("yMax",new Integer(b));
	setVariableByName("yMin",new Integer(t));
}
