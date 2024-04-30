/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2012-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef SCRIPTING_FLASH_DISPLAY_TOKENCONTAINER_H
#define SCRIPTING_FLASH_DISPLAY_TOKENCONTAINER_H 1

#include <vector>
#include "backends/geometry.h"
#include "backends/graphics.h"
#include "forwards/scripting/toplevel/Vector.h"
#include "forwards/scripting/flash/display/DisplayObject.h"
#include "forwards/scripting/flash/display/Graphics.h"

namespace lightspark
{
class RenderContext;
class InteractiveObject;
class DefineMorphShapeTag;

class TokenContainer
{
	friend class Graphics;
	friend class MorphShape;
	friend class TextField;
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
	tokensVector tokens;
	static void FromShaperecordListToShapeVector(const std::vector<SHAPERECORD>& shapeRecords,
					 tokensVector& tokens, const std::list<FILLSTYLE>& fillStyles,
					 const MATRIX& matrix = MATRIX(), const std::list<LINESTYLE2>& lineStyles = std::list<LINESTYLE2>(), const RECT &shapebounds= RECT());
	static void FromDefineMorphShapeTagToShapeVector(DefineMorphShapeTag *tag,
					 tokensVector& tokens, uint16_t ratio);
	static void getTextureSize(std::vector<uint64_t>& tokens, int *width, int *height);
	static bool boundsRectFromTokens(const tokensVector& tokens,float scaling, number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax);
	uint16_t getCurrentLineWidth() const;
	float scaling;
	bool renderWithNanoVG;
	void fillGraphicsData(Vector* v);
protected:
	TokenContainer(DisplayObject* _o);
	TokenContainer(DisplayObject* _o, const tokensVector& _tokens, float _scaling);
	IDrawable* invalidate(SMOOTH_MODE smoothing, bool fromgraphics);
	void requestInvalidation(InvalidateQueue* q, bool forceTextureRefresh=false);
	bool boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const;
	bool hitTestImpl(const Vector2f& point) const;
	bool tokensEmpty() const { return tokens.empty(); }
};

}
#endif /* SCRIPTING_FLASH_DISPLAY_TOKENCONTAINER_H */
