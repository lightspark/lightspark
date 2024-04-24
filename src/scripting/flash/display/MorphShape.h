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

#ifndef SCRIPTING_FLASH_DISPLAY_MORPHSHAPE_H
#define SCRIPTING_FLASH_DISPLAY_MORPHSHAPE_H 1

#include "scripting/flash/display/DisplayObject.h"
#include "scripting/flash/display/TokenContainer.h"

namespace lightspark
{

class DefineMorphShapeTag;
class MorphShape: public DisplayObject, public TokenContainer
{
private:
	DefineMorphShapeTag* morphshapetag;
	uint16_t currentratio;
protected:
	bool boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax, bool visibleOnly) override;
	_NR<DisplayObject> hitTestImpl(const Vector2f& globalPoint, const Vector2f& localPoint, DisplayObject::HIT_TYPE type,bool interactiveObjectsOnly) override;
public:
	MorphShape(ASWorker* wrk,Class_base* c);
	MorphShape(ASWorker* wrk, Class_base* c, DefineMorphShapeTag* _morphshapetag);
	static void sinit(Class_base* c);
	void requestInvalidation(InvalidateQueue* q, bool forceTextureRefresh=false) override { TokenContainer::requestInvalidation(q,forceTextureRefresh); }
	IDrawable* invalidate(bool smoothing) override;
	void checkRatio(uint32_t ratio, bool inskipping) override;
	uint32_t getTagID() const override;
	float getScaleFactor() const override { return this->scaling; }
};

}

#endif /* SCRIPTING_FLASH_DISPLAY_MORPHSHAPE_H */
