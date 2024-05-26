/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2013  Antti Ajanki (antti.ajanki@iki.fi)

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

#ifndef SCRIPTING_FLASH_DISPLAY_IGRAPHICSDATA_H
#define SCRIPTING_FLASH_DISPLAY_IGRAPHICSDATA_H 1

#include <vector>
#include "backends/geometry.h"

namespace lightspark
{

class Class_base;
class Graphics;

class IGraphicsData
{
protected:
	virtual ~IGraphicsData() {}
public:
	static void linkTraits(Class_base* c) {}
	// Appends GeomTokens for drawing this object into tokens
	virtual void appendToTokens(tokensVector& tokens,Graphics* graphics) = 0;
};

}
#endif /* SCRIPTING_FLASH_DISPLAY_IGRAPHICSDATA_H */
