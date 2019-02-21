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

#ifndef SCRIPTING_FLASH_DISPLAY_GRAPHICS_H
#define SCRIPTING_FLASH_DISPLAY_GRAPHICS_H 1

#include <vector>
#include "asobject.h"
#include "backends/geometry.h"

namespace lightspark
{

class TokenContainer;
class Array;
class Matrix;
class BitmapData;
class Vector;

/* This objects paints to its owners tokens */
class Graphics: public ASObject
{
private:
	TokenContainer *const owner;
	void checkAndSetScaling();
	static void solveVertexMapping(double x1, double y1,
				       double x2, double y2,
				       double x3, double y3,
				       double u1, double u2, double u3,
				       double c[3]);
	int movex;
	int movey;
	bool inFilling;
	bool hasChanged;
	void dorender(bool closepath);
public:
	Graphics(Class_base* c):ASObject(c),owner(NULL),movex(0),movey(0),inFilling(false),hasChanged(false)
	{
//		throw RunTimeException("Cannot instantiate a Graphics object");
	}
	Graphics(Class_base* c, TokenContainer* _o)
		: ASObject(c),owner(_o),movex(0),movey(0),inFilling(false),hasChanged(false) {}
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	static FILLSTYLE createGradientFill(const tiny_string& type,
					    _NR<Array> colors,
					    _NR<Array> alphas,
					    _NR<Array> ratios,
					    _NR<Matrix> matrix,
					    const tiny_string& spreadMethod,
					    const tiny_string& interpolationMethod,
					    number_t focalPointRatio);
	static FILLSTYLE createBitmapFill(_R<BitmapData> bitmap,
					  _NR<Matrix> matrix,
					  bool repeat,
					  bool smooth);
	static FILLSTYLE createSolidFill(uint32_t color, uint8_t alpha);
	static void pathToTokens(_NR<Vector> commands,
				 _NR<Vector> data,
				 tiny_string windings,
				 std::vector<_NR<GeomToken>, reporter_allocator<_NR<GeomToken>> > &tokens);
	static void drawTrianglesToTokens(_NR<Vector> vertices,
					  _NR<Vector> indices,
					  _NR<Vector> uvtData,
					  tiny_string culling,
					  std::vector<_NR<GeomToken>, reporter_allocator<_NR<GeomToken>>> &tokens);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(lineBitmapStyle);
	ASFUNCTION_ATOM(lineGradientStyle);
	ASFUNCTION_ATOM(lineStyle);
	ASFUNCTION_ATOM(beginFill);
	ASFUNCTION_ATOM(beginGradientFill);
	ASFUNCTION_ATOM(beginBitmapFill);
	ASFUNCTION_ATOM(endFill);
	ASFUNCTION_ATOM(drawRect);
	ASFUNCTION_ATOM(drawRoundRect);
	ASFUNCTION_ATOM(drawRoundRectComplex);// this is an undocumented function (probably taken from mx.utils.GraphicsUtil)
	ASFUNCTION_ATOM(drawCircle);
	ASFUNCTION_ATOM(drawEllipse);
	ASFUNCTION_ATOM(drawGraphicsData);
	ASFUNCTION_ATOM(drawPath);
	ASFUNCTION_ATOM(drawTriangles);
	ASFUNCTION_ATOM(moveTo);
	ASFUNCTION_ATOM(lineTo);
	ASFUNCTION_ATOM(curveTo);
	ASFUNCTION_ATOM(cubicCurveTo);
	ASFUNCTION_ATOM(clear);
	ASFUNCTION_ATOM(copyFrom);
};

}

#endif /* SCRIPTING_FLASH_DISPLAY_GRAPHICS_H */
