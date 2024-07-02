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

#include "scripting/flash/filters/GradientGlowFilter.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include "scripting/flash/display/BitmapData.h"
#include "backends/rendering.h"
#include "scripting/toplevel/Array.h"


using namespace std;
using namespace lightspark;


GradientGlowFilter::GradientGlowFilter(ASWorker* wrk, Class_base* c):
	BitmapFilter(wrk,c,SUBTYPE_GRADIENTGLOWFILTER),distance(4.0),angle(45), blurX(4.0), blurY(4.0), strength(1), quality(1), type("inner"), knockout(false)
{
}
GradientGlowFilter::GradientGlowFilter(ASWorker* wrk, Class_base* c, const GRADIENTGLOWFILTER& filter):
	BitmapFilter(wrk,c,SUBTYPE_GRADIENTGLOWFILTER),distance(filter.Distance),angle(filter.Angle), blurX(filter.BlurX), blurY(filter.BlurY), strength(filter.Strength), quality(filter.Passes),
	type(filter.InnerGlow ? "inner": "outer"),// TODO: is type set based on "onTop" ?
	knockout(filter.Knockout)
{
	if (filter.GradientColors.size())
	{
		colors = _MR(Class<Array>::getInstanceSNoArgs(wrk));
		alphas = _MR(Class<Array>::getInstanceSNoArgs(wrk));
		auto it = filter.GradientColors.begin();
		while (it != filter.GradientColors.end())
		{
			colors->push(asAtomHandler::fromUInt(RGB(it->Red,it->Green,it->Blue).toUInt()));
			alphas->push(asAtomHandler::fromNumber(wrk,it->af(),false));
			it++;
		}
	}
	if (filter.GradientRatio.size())
	{
		ratios = _MR(Class<Array>::getInstanceSNoArgs(wrk));
		auto it = filter.GradientRatio.begin();
		while (it != filter.GradientRatio.end())
		{
			ratios->push(asAtomHandler::fromUInt(*it));
			it++;
		}
	}
}


void GradientGlowFilter::sinit(Class_base* c)
{
	CLASS_SETUP(c, BitmapFilter, _constructor, CLASS_SEALED | CLASS_FINAL);

	REGISTER_GETTER_SETTER(c, distance);
	REGISTER_GETTER_SETTER(c, angle);
	REGISTER_GETTER_SETTER(c, colors);
	REGISTER_GETTER_SETTER(c, alphas);
	REGISTER_GETTER_SETTER(c, ratios);
	REGISTER_GETTER_SETTER(c, blurX);
	REGISTER_GETTER_SETTER(c, blurY);
	REGISTER_GETTER_SETTER(c, strength);
	REGISTER_GETTER_SETTER(c, quality);
	REGISTER_GETTER_SETTER(c, type);
	REGISTER_GETTER_SETTER(c, knockout);
}

void GradientGlowFilter::applyFilter(BitmapContainer* target, BitmapContainer* source, const RECT& sourceRect, number_t xpos, number_t ypos, number_t scalex, number_t scaley, DisplayObject* owner)
{
	uint8_t* tmpdata = nullptr;
	if (source)
		tmpdata = source->getRectangleData(sourceRect);
	else
		tmpdata = target->getRectangleData(sourceRect);

	number_t gradientalphas[256];
	uint32_t gradientcolors[256];
	fillGradientColors(gradientalphas,gradientcolors,this->ratios.getPtr(), this->alphas.getPtr(), this->colors.getPtr());
	applyBlur(tmpdata,sourceRect.Xmax-sourceRect.Xmin,sourceRect.Ymax-sourceRect.Ymin,blurX,blurY,quality);
	applyGradientFilter(target->getData(),target->getWidth(),target->getHeight(), tmpdata, sourceRect, xpos, ypos, strength, gradientalphas, gradientcolors, type=="inner", knockout,scalex,scaley);
	delete[] tmpdata;
}

void GradientGlowFilter::prepareShutdown()
{
	if (preparedforshutdown)
		return;
	BitmapFilter::prepareShutdown();
	if (colors)
		colors->prepareShutdown();
	if (alphas)
		alphas->prepareShutdown();
	if (ratios)
		ratios->prepareShutdown();
}

void GradientGlowFilter::getRenderFilterGradientColors(float* gradientcolors) const
{
	number_t tmpgradientalphas[256];
	uint32_t tmpgradientcolors[256];
	fillGradientColors(tmpgradientalphas,tmpgradientcolors,this->ratios.getPtr(), this->alphas.getPtr(), this->colors.getPtr());
	for (uint32_t i = 0; i < 256; i++)
	{
		RGBA c(tmpgradientcolors[i],0);
		gradientcolors[i*4  ] = c.rf();
		gradientcolors[i*4+1] = c.gf();
		gradientcolors[i*4+2] = c.bf();
		gradientcolors[i*4+3] = tmpgradientalphas[i];
	}
}

ASFUNCTIONBODY_GETTER_SETTER(GradientGlowFilter, distance)
ASFUNCTIONBODY_GETTER_SETTER(GradientGlowFilter, angle)
ASFUNCTIONBODY_GETTER_SETTER(GradientGlowFilter, colors)
ASFUNCTIONBODY_GETTER_SETTER(GradientGlowFilter, alphas)
ASFUNCTIONBODY_GETTER_SETTER(GradientGlowFilter, ratios)
ASFUNCTIONBODY_GETTER_SETTER(GradientGlowFilter, blurX)
ASFUNCTIONBODY_GETTER_SETTER(GradientGlowFilter, blurY)
ASFUNCTIONBODY_GETTER_SETTER(GradientGlowFilter, strength)
ASFUNCTIONBODY_GETTER_SETTER(GradientGlowFilter, quality)
ASFUNCTIONBODY_GETTER_SETTER(GradientGlowFilter, type)
ASFUNCTIONBODY_GETTER_SETTER(GradientGlowFilter, knockout)

ASFUNCTIONBODY_ATOM(GradientGlowFilter,_constructor)
{
	GradientGlowFilter *th = asAtomHandler::as<GradientGlowFilter>(obj);
	ARG_CHECK(ARG_UNPACK(th->distance,4.0)(th->angle,45)(th->colors,NullRef)(th->alphas,NullRef)(th->ratios,NullRef)(th->blurX,4.0)(th->blurY,4.0)(th->strength,1)(th->quality,1)(th->type,"inner")(th->knockout,false));
}

bool GradientGlowFilter::compareFILTER(const FILTER& filter) const
{
	LOG(LOG_NOT_IMPLEMENTED, "comparing GradientGlowFilter");
	return false;
}
void GradientGlowFilter::getRenderFilterArgs(uint32_t step,float* args) const
{
	if (type=="full")
		LOG(LOG_NOT_IMPLEMENTED,"GradientBevelFilter type 'full'");
	uint32_t nextstep;
	if (getRenderFilterArgsBlur(args,blurX,blurY,step,quality,nextstep))
		return;
	else if (step == nextstep)
	{
		args[0]=float(FILTERSTEP_GRADIENT_GLOW);
		args[1]=type=="inner";
		args[2]=knockout;
		args[3]=strength;
		args[4]=cos(angle+(type=="inner" ? M_PI:0.0)) * distance / float(getSystemState()->getRenderThread()->windowWidth);
		args[5]=sin(angle+(type=="inner" ? 0.0:M_PI)) * distance / float(getSystemState()->getRenderThread()->windowHeight);
	}
	else
		args[0]=0.0;
}
BitmapFilter* GradientGlowFilter::cloneImpl() const
{
	GradientGlowFilter *cloned = Class<GradientGlowFilter>::getInstanceS(getInstanceWorker());
	cloned->distance = distance;
	cloned->angle = angle;
	cloned->colors = colors;
	cloned->alphas = alphas;
	cloned->ratios = ratios;
	cloned->blurX = blurX;
	cloned->blurY = blurY;
	cloned->strength = strength;
	cloned->quality = quality;
	cloned->type = type;
	cloned->knockout = knockout;
	return cloned;
}
