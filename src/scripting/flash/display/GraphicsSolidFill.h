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

#ifndef SCRIPTING_FLASH_DISPLAY_GRAPHICSSOLIDFILL_H
#define SCRIPTING_FLASH_DISPLAY_GRAPHICSSOLIDFILL_H 1

#include "asobject.h"
#include "scripting/flash/display/IGraphicsFill.h"
#include "scripting/flash/display/IGraphicsData.h"

namespace lightspark
{

class GraphicsSolidFill: public ASObject, public IGraphicsFill, public IGraphicsData
{
public:
	GraphicsSolidFill(ASWorker* wrk,Class_base* c);
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(_constructor);
	ASPROPERTY_GETTER_SETTER(number_t, alpha);
	ASPROPERTY_GETTER_SETTER(uint32_t, color);
	FILLSTYLE toFillStyle();
	void appendToTokens(std::vector<uint64_t>& tokens,Graphics* graphics);
};

}

#endif /* SCRIPTING_FLASH_DISPLAY_GRAPHICSSOLIDFILL_H */
