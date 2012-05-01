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

#ifndef _TOKEN_CONTAINER_H
#define _TOKEN_CONTAINER_H

#include <vector>
#include "backends/geometry.h"
#include "backends/graphics.h"
#include "DisplayObject.h"

namespace lightspark
{

class DisplayObject;
class InteractiveObject;

class TokenContainer
{
	friend class Graphics;
public:
	DisplayObject* owner;
	/* multiply shapes' coordinates by this
	 * value to get pixel.
	 * DefineShapeTags set a scaling of 1/20,
	 * DefineTextTags set a scaling of 1/1024/20.
	 * If any drawing function is called and
	 * scaling is not 1.0f,
	 * the tokens are cleared and scaling is set
	 * to 1.0f.
	 */
	float scaling;
	std::vector<GeomToken> tokens;
	static void FromShaperecordListToShapeVector(const std::vector<SHAPERECORD>& shapeRecords,
					 tokensVector& tokens, const std::list<FILLSTYLE>& fillStyles,
					 const Vector2& offset = Vector2(), int scaling = 1);
	void getTextureSize(int *width, int *height) const;
protected:
	TokenContainer(DisplayObject* _o);
	TokenContainer(DisplayObject* _o, const tokensVector& _tokens, float _scaling);
	void invalidate();
	void requestInvalidation(InvalidateQueue* q);
	bool boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const;
	_NR<InteractiveObject> hitTestImpl(_NR<InteractiveObject> last, number_t x, number_t y, DisplayObject::HIT_TYPE type) const;
	void renderImpl(RenderContext& ctxt, bool maskEnabled, number_t t1, number_t t2, number_t t3, number_t t4) const;
	bool tokensEmpty() const { return tokens.empty(); }
	bool isOpaqueImpl(number_t x, number_t y) const;
};

};
#endif
