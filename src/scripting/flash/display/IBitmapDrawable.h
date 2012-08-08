/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2012  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef SCRIPTING_FLASH_DISPLAY_IBITMAPDRAWABLE_H
#define SCRIPTING_FLASH_DISPLAY_IBITMAPDRAWABLE_H 1

namespace lightspark
{

class Class_base;

class IBitmapDrawable
{
public:
	static void linkTraits(Class_base* c);
};

};
#endif /* SCRIPTING_FLASH_DISPLAY_IBITMAPDRAWABLE_H */
