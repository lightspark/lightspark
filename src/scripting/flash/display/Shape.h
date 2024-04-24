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

#ifndef SCRIPTING_FLASH_DISPLAY_SHAPE_H
#define SCRIPTING_FLASH_DISPLAY_SHAPE_H 1

#include "scripting/flash/display/DisplayObject.h"
#include "scripting/flash/display/TokenContainer.h"

namespace lightspark
{

class DefineShapeTag;
class Shape: public DisplayObject, public TokenContainer
{
protected:
	_NR<Graphics> graphics;
	bool boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax, bool visibleOnly) override;
	_NR<DisplayObject> hitTestImpl(const Vector2f& globalPoint, const Vector2f& localPoint, DisplayObject::HIT_TYPE type,bool interactiveObjectsOnly) override;
	
	DefineShapeTag* fromTag;
public:
	void fillGraphicsData(Vector* v, bool recursive) override;
	Shape(ASWorker* wrk,Class_base* c);
	void setupShape(lightspark::DefineShapeTag *tag, float _scaling);
	uint32_t getTagID() const override;
	float getScaleFactor() const override { return this->scaling; }
	bool destruct() override;
	void finalize() override;
	void prepareShutdown() override;
	void startDrawJob() override;
	void endDrawJob() override;

	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(_getGraphics);
	void requestInvalidation(InvalidateQueue* q, bool forceTextureRefresh=false) override;
	IDrawable* invalidate(bool smoothing) override;
};

}

#endif /* SCRIPTING_FLASH_DISPLAY_SHAPE_H */
