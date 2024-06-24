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

#include "scripting/flash/filters/GlowFilter.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include "scripting/flash/display/BitmapData.h"


using namespace std;
using namespace lightspark;


GlowFilter::GlowFilter(ASWorker* wrk, Class_base* c):
	BitmapFilter(wrk,c,SUBTYPE_GLOWFILTER), alpha(1.0), blurX(6.0), blurY(6.0), color(0xFF0000),
	inner(false), knockout(false), quality(1), strength(2.0)
{
}
GlowFilter::GlowFilter(ASWorker* wrk, Class_base* c, const GLOWFILTER& filter):
	BitmapFilter(wrk,c,SUBTYPE_GLOWFILTER), alpha(filter.GlowColor.af()), blurX(filter.BlurX), blurY(filter.BlurY), color(RGB(filter.GlowColor.Red,filter.GlowColor.Green,filter.GlowColor.Blue).toUInt()),
	inner(filter.InnerGlow), knockout(filter.Knockout), quality(filter.Passes), strength(filter.Strength)
{
}

void GlowFilter::sinit(Class_base* c)
{
	CLASS_SETUP(c, BitmapFilter, _constructor, CLASS_SEALED | CLASS_FINAL);
	REGISTER_GETTER_SETTER(c, alpha);
	REGISTER_GETTER_SETTER(c, blurX);
	REGISTER_GETTER_SETTER(c, blurY);
	REGISTER_GETTER_SETTER(c, color);
	REGISTER_GETTER_SETTER(c, inner);
	REGISTER_GETTER_SETTER(c, knockout);
	REGISTER_GETTER_SETTER(c, quality);
	REGISTER_GETTER_SETTER(c, strength);
}

ASFUNCTIONBODY_GETTER_SETTER(GlowFilter, alpha)
ASFUNCTIONBODY_GETTER_SETTER(GlowFilter, blurX)
ASFUNCTIONBODY_GETTER_SETTER(GlowFilter, blurY)
ASFUNCTIONBODY_GETTER_SETTER(GlowFilter, color)
ASFUNCTIONBODY_GETTER_SETTER(GlowFilter, inner)
ASFUNCTIONBODY_GETTER_SETTER(GlowFilter, knockout)
ASFUNCTIONBODY_GETTER_SETTER(GlowFilter, quality)
ASFUNCTIONBODY_GETTER_SETTER(GlowFilter, strength)

ASFUNCTIONBODY_ATOM(GlowFilter,_constructor)
{
	GlowFilter *th = asAtomHandler::as<GlowFilter>(obj);
	ARG_CHECK(ARG_UNPACK (th->color, 0xFF0000)
		(th->alpha, 1.0)
		(th->blurX, 6.0)
		(th->blurY, 6.0)
		(th->strength, 2.0)
		(th->quality, 1)
		(th->inner, false)
		(th->knockout, false));
}

bool GlowFilter::compareFILTER(const FILTER& filter) const
{
	return filter.FilterID == FILTER::FILTER_GLOW
			&& filter.GlowFilter.GlowColor.af() == this->alpha
			&& filter.GlowFilter.BlurX == this->blurX
			&& filter.GlowFilter.BlurY == this->blurY
			&& filter.GlowFilter.GlowColor.Red == ((this->color>>16)&0xff)
			&& filter.GlowFilter.GlowColor.Green == ((this->color>>8)&0xff)
			&& filter.GlowFilter.GlowColor.Blue == ((this->color)&0xff)
			&& filter.GlowFilter.InnerGlow == this->inner
			&& filter.GlowFilter.Knockout == this->knockout
			&& filter.GlowFilter.Strength == this->strength
			&& filter.GlowFilter.Passes == this->quality;
}

void GlowFilter::getRenderFilterArgs(uint32_t step,float* args) const
{
	uint32_t nextstep;
	if (getRenderFilterArgsBlur(args,blurX,blurY,step,quality,nextstep))
		return;
	else if (step == nextstep)
		getRenderFilterArgsDropShadow(args,inner,knockout,strength,color,alpha,0.0,0.0);
	else
		args[0]=0.0;
}

BitmapFilter* GlowFilter::cloneImpl() const
{
	GlowFilter *cloned = Class<GlowFilter>::getInstanceS(getInstanceWorker());
	cloned->alpha = alpha;
	cloned->blurX = blurX;
	cloned->blurY = blurY;
	cloned->color = color;
	cloned->inner = inner;
	cloned->knockout = knockout;
	cloned->quality = quality;
	cloned->strength = strength;
	return cloned;
}
void GlowFilter::applyFilter(BitmapContainer* target, BitmapContainer* source, const RECT& sourceRect, number_t xpos, number_t ypos,number_t scalex,number_t scaley, DisplayObject* owner)
{
	uint8_t* tmpdata = nullptr;
	if (source)
		tmpdata = source->getRectangleData(sourceRect);
	else
		tmpdata = target->getRectangleData(sourceRect);
	applyBlur(tmpdata,sourceRect.Xmax-sourceRect.Xmin,sourceRect.Ymax-sourceRect.Ymin,blurX,blurY,quality);
	applyDropShadowFilter(target->getData(),target->getWidth(),target->getHeight(), tmpdata, sourceRect, xpos, ypos, strength, alpha, color, inner, knockout,scalex,scaley);
	delete[] tmpdata;
}
