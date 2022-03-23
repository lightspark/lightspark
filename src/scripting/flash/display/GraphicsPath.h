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

#ifndef SCRIPTING_FLASH_DISPLAY_GRAPHICSPATH_H
#define SCRIPTING_FLASH_DISPLAY_GRAPHICSPATH_H 1

#include "asobject.h"
#include "tiny_string.h"
#include "scripting/flash/display/IGraphicsPath.h"
#include "scripting/flash/display/IGraphicsData.h"

namespace lightspark
{

class Vector;

class GraphicsPath: public ASObject, public IGraphicsPath, public IGraphicsData
{
private:
	void ensureValid();
public:
	GraphicsPath(ASWorker* wrk,Class_base* c);
	static void sinit(Class_base* c);
	void finalize() override;
	ASFUNCTION_ATOM(_constructor);
	ASPROPERTY_GETTER_SETTER(_NR<Vector>, commands);
	ASPROPERTY_GETTER_SETTER(_NR<Vector>, data);
	ASPROPERTY_GETTER_SETTER(tiny_string, winding);
	ASFUNCTION_ATOM(curveTo);
	ASFUNCTION_ATOM(lineTo);
	ASFUNCTION_ATOM(moveTo);
	ASFUNCTION_ATOM(wideLineTo);
	ASFUNCTION_ATOM(wideMoveTo);
	void appendToTokens(std::vector<uint64_t>& tokens,Graphics* graphics) override;
};

}

#endif /* SCRIPTING_FLASH_DISPLAY_GRAPHICSPATH_H */
