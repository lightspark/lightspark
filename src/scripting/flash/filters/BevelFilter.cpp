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

#include "scripting/flash/filters/BevelFilter.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include "scripting/flash/display/BitmapData.h"

using namespace std;
using namespace lightspark;

BevelFilter::BevelFilter(ASWorker* wrk,Class_base* c):
	BitmapFilter(wrk,c,SUBTYPE_BEVELFILTER), angle(45), blurX(4.0), blurY(4.0),distance(4.0),
	highlightAlpha(1.0), highlightColor(0xFFFFFF),
	knockout(false), quality(1),
	shadowAlpha(1.0), shadowColor(0x000000),
	strength(1), type("inner")
{
}
BevelFilter::BevelFilter(ASWorker* wrk, Class_base* c, const BEVELFILTER& filter):
	BitmapFilter(wrk,c,SUBTYPE_BEVELFILTER),angle(filter.Angle),blurX(filter.BlurX), blurY(filter.BlurY),distance(filter.Distance), 
	highlightAlpha(filter.HighlightColor.af()), highlightColor(RGB(filter.HighlightColor.Red,filter.HighlightColor.Green,filter.HighlightColor.Blue).toUInt()),
	knockout(filter.Knockout), quality(filter.Passes),
	shadowAlpha(filter.ShadowColor.af()), shadowColor(RGB(filter.ShadowColor.Red,filter.ShadowColor.Green,filter.ShadowColor.Blue).toUInt()),
	strength(filter.Strength), type(filter.InnerShadow ? "inner" : "outer") // TODO: is type set based on "onTop" ?
{
	if (filter.OnTop)
		LOG(LOG_NOT_IMPLEMENTED,"BevelFilter onTop flag");
}

void BevelFilter::sinit(Class_base* c)
{
	CLASS_SETUP(c, BitmapFilter, _constructor, CLASS_SEALED | CLASS_FINAL);
	REGISTER_GETTER_SETTER(c,angle);
	REGISTER_GETTER_SETTER(c,blurX);
	REGISTER_GETTER_SETTER(c,blurY);
	REGISTER_GETTER_SETTER(c,distance);
	REGISTER_GETTER_SETTER(c,highlightAlpha);
	REGISTER_GETTER_SETTER(c,highlightColor);
	REGISTER_GETTER_SETTER(c,knockout);
	REGISTER_GETTER_SETTER(c,quality);
	REGISTER_GETTER_SETTER(c,shadowAlpha);
	REGISTER_GETTER_SETTER(c,shadowColor);
	REGISTER_GETTER_SETTER(c,strength);
	REGISTER_GETTER_SETTER(c,type);
}

ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(BevelFilter,angle)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(BevelFilter,blurX)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(BevelFilter,blurY)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(BevelFilter,distance)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(BevelFilter,highlightAlpha)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(BevelFilter,highlightColor)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(BevelFilter,knockout)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(BevelFilter,quality)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(BevelFilter,shadowAlpha)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(BevelFilter,shadowColor)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(BevelFilter,strength)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(BevelFilter,type)

void BevelFilter::applyFilter(BitmapContainer* target, BitmapContainer* source, const RECT& sourceRect, number_t xpos, number_t ypos, number_t scalex, number_t scaley, DisplayObject* owner)
{
	if (type=="full")
		LOG(LOG_NOT_IMPLEMENTED,"BevelFilter type 'full'");
	uint8_t* tmpdata = nullptr;
	if (source)
		tmpdata = source->getRectangleData(sourceRect);
	else
		tmpdata = target->getRectangleData(sourceRect);

	// create gradient color list for fixed highlight/shadow colors
	number_t gradientalphas[256];
	uint32_t gradientcolors[256];
	number_t deltashadow = shadowAlpha/128.0;
	number_t deltahighlight = highlightAlpha/127.0;
	for (uint32_t i =0;i<128;i++)
	{
		gradientalphas[i] = number_t(128-i)*deltashadow;
		gradientalphas[i+128] = number_t(i)*deltahighlight;

		//precompile alpha to colors
		gradientcolors[i] = shadowColor;
		gradientcolors[i+128] = highlightColor;
	}
	applyBlur(tmpdata,sourceRect.Xmax-sourceRect.Xmin,sourceRect.Ymax-sourceRect.Ymin,blurX,blurY,quality);
	applyBevelFilter(target,sourceRect,tmpdata,owner,distance,strength,type=="inner",knockout,gradientcolors,gradientalphas,angle,xpos,ypos,scalex,scaley);
	delete[] tmpdata;
}

ASFUNCTIONBODY_ATOM(BevelFilter,_constructor)
{
	BevelFilter *th = asAtomHandler::as<BevelFilter>(obj);
	ARG_CHECK(ARG_UNPACK(th->distance,4.0)(th->angle,45)(th->highlightColor,0xFFFFFF)(th->highlightAlpha,1.0)(th->shadowColor,0x000000)(th->shadowAlpha,1.0)(th->blurX,4.0)(th->blurY,4.0)(th->strength,1)(th->quality,1)(th->type,"inner")(th->knockout,false));
}
bool BevelFilter::compareFILTER(const FILTER& filter) const
{
	LOG(LOG_NOT_IMPLEMENTED, "comparing BevelFilter");
	return false;
}

void BevelFilter::getRenderFilterGradientColors(float* gradientcolors) const
{
	number_t deltashadow = shadowAlpha/128.0;
	number_t deltahighlight = highlightAlpha/127.0;
	RGBA cshadow(shadowColor,0);
	RGBA chighlight(highlightColor,0);
	for (uint32_t i =0;i<128;i++)
	{
		gradientcolors[i*4  ] = cshadow.rf();
		gradientcolors[i*4+1] = cshadow.gf();
		gradientcolors[i*4+2] = cshadow.bf();
		gradientcolors[i*4+3] = number_t(128-i)*deltashadow;
		
		gradientcolors[(i+128)*4  ] = chighlight.rf();
		gradientcolors[(i+128)*4+1] = chighlight.gf();
		gradientcolors[(i+128)*4+2] = chighlight.bf();
		gradientcolors[(i+128)*4+3] = number_t(i)*deltahighlight;
	}
}

void BevelFilter::getRenderFilterArgs(uint32_t step,float* args) const
{
	if (type=="full")
		LOG(LOG_NOT_IMPLEMENTED,"BevelFilter type 'full'");
	uint32_t nextstep;
	if (getRenderFilterArgsBlur(args,blurX,blurY,step,quality,nextstep))
		return;
	else if (step == nextstep)
		getRenderFilterArgsBevel(args,type=="inner",knockout,strength,distance,angle);
	else
		args[0]=0.0;
}

BitmapFilter* BevelFilter::cloneImpl() const
{
	BevelFilter* cloned = Class<BevelFilter>::getInstanceS(getInstanceWorker());
	cloned->angle = angle;
	cloned->blurX = blurX;
	cloned->blurY = blurY;
	cloned->distance = distance;
	cloned->highlightAlpha = highlightAlpha;
	cloned->highlightColor = highlightColor;
	cloned->knockout = knockout;
	cloned->quality = quality;
	cloned->shadowAlpha = shadowAlpha;
	cloned->shadowColor = shadowColor;
	cloned->strength = strength;
	cloned->type = type;
	return cloned;
}
