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

#ifndef SCRIPTING_FLASH_DISPLAY_GRAPHICSSTROKE_H
#define SCRIPTING_FLASH_DISPLAY_GRAPHICSSTROKE_H 1

#include "asobject.h"
#include "scripting/flash/display/IGraphicsStroke.h"
#include "scripting/flash/display/IGraphicsData.h"

namespace lightspark
{

class GraphicsStroke: public ASObject, public IGraphicsStroke, public IGraphicsData
{
protected:
	void validateFill(_NR<ASObject> oldValue);
public:
	GraphicsStroke(ASWorker* wrk,Class_base* c);
	static void sinit(Class_base* c);
	void finalize() override;
	bool countCylicMemberReferences(garbagecollectorstate& gcstate) override;
	ASFUNCTION_ATOM(_constructor);
	ASPROPERTY_GETTER_SETTER(tiny_string, caps);
	ASPROPERTY_GETTER_SETTER(_NR<ASObject>, fill); // really IGraphicsFill
	ASPROPERTY_GETTER_SETTER(tiny_string, joints);
	ASPROPERTY_GETTER_SETTER(number_t, miterLimit);
	ASPROPERTY_GETTER_SETTER(bool, pixelHinting);
	ASPROPERTY_GETTER_SETTER(tiny_string, scaleMode);
	ASPROPERTY_GETTER_SETTER(number_t, thickness);
	void appendToTokens(tokensVector& tokens, Graphics* graphics) override;
};

}

#endif /* SCRIPTING_FLASH_DISPLAY_GRAPHICSSTROKE_H */
