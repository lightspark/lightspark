/**************************************************************************
    Lightspark, a free flash player implementation

	Copyright (C) 2009-2013  Alessandro Pignotti (a.pignotti@sssup.it)
	Copyright (C) 2026  Ludger Krämer <dbluelle@onlinehome.de>

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
#ifndef BACKENDS_SHAPESBUILDER_H
#define BACKENDS_SHAPESBUILDER_H 1

#include "backends/geometry.h"

namespace lightspark
{

class ShapePathSegment {
public:
	floatVec from;
	floatVec quadctrl;
	floatVec to;
	int linestyleindex;
	ShapePathSegment(floatVec from, floatVec quadctrl, floatVec to, int linestyleindex): from(from), quadctrl(quadctrl), to(to),linestyleindex(linestyleindex) {}
	ShapePathSegment()
	{
		from.key=0;
		quadctrl.key=0;
		to.key=0;
		linestyleindex=0;
	}
	ShapePathSegment reverse() const {return ShapePathSegment(to, quadctrl, from, linestyleindex);}
};

class ShapesBuilder
{
private:
	std::vector<ShapePathSegment> currentSubpath;
	std::map< unsigned int, std::vector<ShapePathSegment> > filledShapesMap;
	std::map< unsigned int, std::vector<ShapePathSegment> > strokeShapesMap;

	static bool isOutlineEmpty(const std::vector<ShapePathSegment>& outline);
	static floatVec makeVertex(const Vector2f& v) 
	{ 
		floatVec ret;
		ret.vec.x = v.x;
		ret.vec.y = v.y;
		return ret;
	}
	std::map<uint16_t,LINESTYLE2>::iterator getStrokeLineStyle(const std::list<MORPHLINESTYLE2>::iterator& stylesIt, uint16_t ratio, std::map<uint16_t, LINESTYLE2>* linestylecache, const RECT& boundsrc);
	void fillTokensFromSegment(std::vector<ShapePathSegment>& segments, tokenListRef* tokens);
public:
	void extendOutline(const Vector2f& v1, const Vector2f& v2, int linestyleindex);
	void extendOutlineCurve(const Vector2f& start, const Vector2f& control, const Vector2f& end, int linestyleindex);
	void endSubpathForStyles(unsigned fill0, unsigned fill1, unsigned stroke, bool formorphing);
	/**
		Generate a sequence of cachable tokens that defines the geomtries
		@param styles This list is supposed to survive until as long as the returned tokens array
		@param tokens A vector that will be filled with tokens
	*/
	void outputTokens(const std::list<FILLSTYLE>& styles, const std::list<LINESTYLE2>& linestyles, tokensVector& tokens, bool isGlyph);
	void outputMorphTokens(std::list<MORPHFILLSTYLE>& styles, std::list<MORPHLINESTYLE2>& linestyles, tokensVector& tokens, uint16_t ratio, const RECT& boundsrc);
	void clear();
};

std::ostream& operator<<(std::ostream& s, const Vector2& p);

}

#endif /* BACKENDS_SHAPESBUILDER_H */
