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

#include "scripting/flash/filters/DropShadowFilter.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include "scripting/flash/display/BitmapData.h"


using namespace std;
using namespace lightspark;


DropShadowFilter::DropShadowFilter(ASWorker* wrk,Class_base* c):
	BitmapFilter(wrk,c,SUBTYPE_DROPSHADOWFILTER), alpha(1.0), angle(45), blurX(4.0), blurY(4.0),
	color(0), distance(4.0), hideObject(false), inner(false),
	knockout(false), quality(1), strength(1.0)
{
}
DropShadowFilter::DropShadowFilter(ASWorker* wrk,Class_base* c,const DROPSHADOWFILTER& filter):
	BitmapFilter(wrk,c,SUBTYPE_DROPSHADOWFILTER), alpha(filter.DropShadowColor.af()), angle(filter.Angle), blurX(filter.BlurX), blurY(filter.BlurY),
	color(RGB(filter.DropShadowColor.Red,filter.DropShadowColor.Green,filter.DropShadowColor.Blue).toUInt()), distance(filter.Distance), hideObject(false), inner(filter.InnerShadow),
	knockout(filter.Knockout), quality(filter.Passes), strength(filter.Strength)
{
}
void DropShadowFilter::applyFilter(BitmapContainer* target, BitmapContainer* source, const RECT& sourceRect, number_t xpos, number_t ypos, number_t scalex, number_t scaley, DisplayObject* owner)
{
	xpos += cos(angle) * distance;
	ypos +=	sin(angle) * distance;
	if (hideObject)
		LOG(LOG_NOT_IMPLEMENTED,"DropShadowFilter.hideObject");
	uint8_t* tmpdata = nullptr;
	if (source)
		tmpdata = source->getRectangleData(sourceRect);
	else
		tmpdata = target->getRectangleData(sourceRect);

	applyBlur(tmpdata,sourceRect.Xmax-sourceRect.Xmin,sourceRect.Ymax-sourceRect.Ymin,blurX,blurY,quality);
	applyDropShadowFilter(target->getData(),target->getWidth(),target->getHeight(), tmpdata, sourceRect, xpos, ypos, strength, alpha, color, inner, knockout,scalex,scaley);
	delete[] tmpdata;
}


void DropShadowFilter::sinit(Class_base* c)
{
	CLASS_SETUP(c, BitmapFilter, _constructor, CLASS_SEALED | CLASS_FINAL);
	REGISTER_GETTER_SETTER(c, alpha);
	REGISTER_GETTER_SETTER(c, angle);
	REGISTER_GETTER_SETTER(c, blurX);
	REGISTER_GETTER_SETTER(c, blurY);
	REGISTER_GETTER_SETTER(c, color);
	REGISTER_GETTER_SETTER(c, distance);
	REGISTER_GETTER_SETTER(c, hideObject);
	REGISTER_GETTER_SETTER(c, inner);
	REGISTER_GETTER_SETTER(c, knockout);
	REGISTER_GETTER_SETTER(c, quality);
	REGISTER_GETTER_SETTER(c, strength);
}

ASFUNCTIONBODY_GETTER_SETTER(DropShadowFilter, alpha)
ASFUNCTIONBODY_GETTER_SETTER(DropShadowFilter, angle)
ASFUNCTIONBODY_GETTER_SETTER(DropShadowFilter, blurX)
ASFUNCTIONBODY_GETTER_SETTER(DropShadowFilter, blurY)
ASFUNCTIONBODY_GETTER_SETTER(DropShadowFilter, color)
ASFUNCTIONBODY_GETTER_SETTER(DropShadowFilter, distance)
ASFUNCTIONBODY_GETTER_SETTER(DropShadowFilter, hideObject)
ASFUNCTIONBODY_GETTER_SETTER(DropShadowFilter, inner)
ASFUNCTIONBODY_GETTER_SETTER(DropShadowFilter, knockout)
ASFUNCTIONBODY_GETTER_SETTER(DropShadowFilter, quality)
ASFUNCTIONBODY_GETTER_SETTER(DropShadowFilter, strength)

ASFUNCTIONBODY_ATOM(DropShadowFilter,_constructor)
{
	DropShadowFilter *th = asAtomHandler::as<DropShadowFilter>(obj);
	ARG_CHECK(ARG_UNPACK (th->distance, 4.0)
		(th->angle, 45)
		(th->color, 0)
		(th->alpha, 1.0)
		(th->blurX, 4.0)
		(th->blurY, 4.0)
		(th->strength, 1.0)
		(th->quality, 1)
		(th->inner, false)
		(th->knockout, false)
		(th->hideObject, false));
}

bool DropShadowFilter::compareFILTER(const FILTER& filter) const
{
	return filter.FilterID == FILTER::FILTER_DROPSHADOW
			&& filter.DropShadowFilter.DropShadowColor.af() == this->alpha
			&& filter.DropShadowFilter.Angle == this->angle
			&& filter.DropShadowFilter.BlurX == this->blurX
			&& filter.DropShadowFilter.BlurY == this->blurY
			&& filter.DropShadowFilter.DropShadowColor.Red == ((this->color>>16)&0xff)
			&& filter.DropShadowFilter.DropShadowColor.Green == ((this->color>>8)&0xff)
			&& filter.DropShadowFilter.DropShadowColor.Blue == ((this->color)&0xff)
			&& filter.DropShadowFilter.Distance == this->distance
			&& filter.DropShadowFilter.InnerShadow == this->inner
			&& filter.DropShadowFilter.Knockout == this->knockout
			&& filter.DropShadowFilter.Strength == this->strength
			&& filter.DropShadowFilter.Passes == this->quality;
}
void DropShadowFilter::getRenderFilterArgs(uint32_t step,float* args) const
{
	if (hideObject)
		LOG(LOG_NOT_IMPLEMENTED,"DropShadowFilter.hideObject");
	uint32_t nextstep;
	if (getRenderFilterArgsBlur(args,blurX,blurY,step,quality,nextstep))
		return;
	else if (step == nextstep)
		getRenderFilterArgsDropShadow(args,inner,knockout,strength,color,alpha,cos(angle) * distance,sin(angle) * distance);
	else
		args[0]=0.0;
}

BitmapFilter* DropShadowFilter::cloneImpl() const
{
	DropShadowFilter *cloned = Class<DropShadowFilter>::getInstanceS(getInstanceWorker());
	cloned->alpha = alpha;
	cloned->angle = angle;
	cloned->blurX = blurX;
	cloned->blurY = blurY;
	cloned->color = color;
	cloned->distance = distance;
	cloned->hideObject = hideObject;
	cloned->inner = inner;
	cloned->knockout = knockout;
	cloned->quality = quality;
	cloned->strength = strength;
	return cloned;
}
