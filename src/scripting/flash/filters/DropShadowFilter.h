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

#ifndef SCRIPTING_FLASH_FILTERS_DROPSHADOWFILTER_H
#define SCRIPTING_FLASH_FILTERS_DROPSHADOWFILTER_H 1

#include "scripting/flash/filters/flashfilters.h"

namespace lightspark
{

class DropShadowFilter: public BitmapFilter
{
private:
	ASPROPERTY_GETTER_SETTER(number_t, alpha);
	ASPROPERTY_GETTER_SETTER(number_t, angle);
	ASPROPERTY_GETTER_SETTER(number_t, blurX);
	ASPROPERTY_GETTER_SETTER(number_t, blurY);
	ASPROPERTY_GETTER_SETTER(uint32_t, color);
	ASPROPERTY_GETTER_SETTER(number_t, distance);
	ASPROPERTY_GETTER_SETTER(bool, hideObject);
	ASPROPERTY_GETTER_SETTER(bool, inner);
	ASPROPERTY_GETTER_SETTER(bool, knockout);
	ASPROPERTY_GETTER_SETTER(int32_t, quality);
	ASPROPERTY_GETTER_SETTER(number_t, strength);
	BitmapFilter* cloneImpl() const override;
public:
	DropShadowFilter(ASWorker* wrk,Class_base* c);
	DropShadowFilter(ASWorker* wrk,Class_base* c,const DROPSHADOWFILTER& filter);
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(_constructor);
	void applyFilter(BitmapContainer* target, BitmapContainer* source, const RECT& sourceRect, number_t xpos, number_t ypos, number_t scalex, number_t scaley, DisplayObject* owner=nullptr) override;
	number_t getMaxFilterBorder() const override { return max(max(blurX,blurY),distance); }
	bool compareFILTER(const FILTER& filter) const override;
	void getRenderFilterArgs(uint32_t step, float* args) const override;
};

}
#endif // SCRIPTING_FLASH_FILTERS_DROPSHADOWFILTER_H
