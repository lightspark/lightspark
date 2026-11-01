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
#include "scripting/toplevel/Integer.h"


using namespace std;
using namespace lightspark;


GradientGlowFilter::GradientGlowFilter(ASWorker* wrk, Class_base* c):
	BitmapFilter(wrk,c,SUBTYPE_GRADIENTGLOWFILTER),distance(4.0),angle(45), blurX(4.0), blurY(4.0), strength(1), quality(1), type("inner"), knockout(false)
{
}
GradientGlowFilter::GradientGlowFilter(ASWorker* wrk, Class_base* c, const GRADIENTGLOWFILTER& filter)
	:BitmapFilter(wrk,c,SUBTYPE_GRADIENTGLOWFILTER)
	,distance(filter.Distance)
	,angle(filter.Angle*180.0/M_PI)
	,blurX(filter.BlurX)
	,blurY(filter.BlurY)
	,strength(filter.Strength)
	,quality(filter.Passes)
	,type(filter.OnTop ? "full" : (filter.InnerGlow ? "inner": "outer"))
	,knockout(filter.Knockout)
{
	colors = _MR(Class<Array>::getInstanceSNoArgs(wrk));
	alphas = _MR(Class<Array>::getInstanceSNoArgs(wrk));
	if (filter.NumColors)
	{
		for (uint8_t i = 0; i < filter.NumColors; i++)
		{
			colors->push(asAtomHandler::fromUInt(RGB(filter.GradientColors[i].Red,filter.GradientColors[i].Green,filter.GradientColors[i].Blue).toUInt()));
			alphas->push(asAtomHandler::fromNumber(filter.GradientColors[i].af()));
		}
	}
	ratios = _MR(Class<Array>::getInstanceSNoArgs(wrk));
	if (filter.NumColors)
	{
		for (uint8_t i = 0; i < filter.NumColors; i++)
		{
			ratios->push(asAtomHandler::fromUInt(filter.GradientRatio[i]));
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
	REGISTER_GETTER_SETTER_RESULTTYPE(c, blurX,Number);
	REGISTER_GETTER_SETTER_RESULTTYPE(c, blurY,Number);
	REGISTER_GETTER_SETTER(c, strength);
	REGISTER_GETTER_SETTER_RESULTTYPE(c, quality,Integer);
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

void GradientGlowFilter::getRenderFilterGradient
(
	float* gradientColors,
	float* gradientStops
) const
{
	fillGradient(ratios, alphas, colors, gradientColors, gradientStops);
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
ASFUNCTIONBODY_GETTER_SETTER_CB(GradientGlowFilter, blurX,onSetBlurX)
ASFUNCTIONBODY_GETTER_SETTER_CB(GradientGlowFilter, blurY,onSetBlurY)
ASFUNCTIONBODY_GETTER_SETTER(GradientGlowFilter, strength)
ASFUNCTIONBODY_GETTER_SETTER(GradientGlowFilter, quality)
ASFUNCTIONBODY_GETTER_SETTER(GradientGlowFilter, type)
ASFUNCTIONBODY_GETTER_SETTER(GradientGlowFilter, knockout)

ASFUNCTIONBODY_ATOM(GradientGlowFilter,_constructor)
{
	GradientGlowFilter *th = asAtomHandler::as<GradientGlowFilter>(obj);
	asAtom colors = asAtomHandler::invalidAtom;
	asAtom alphas = asAtomHandler::invalidAtom;
	asAtom ratios = asAtomHandler::invalidAtom;
	ARG_CHECK(ARG_UNPACK(th->distance,4.0)(th->angle,45)(colors,asAtomHandler::nullAtom)(alphas,asAtomHandler::nullAtom)(ratios,asAtomHandler::nullAtom)(th->blurX,4.0)(th->blurY,4.0)(th->strength,1)(th->quality,1)(th->type,"inner")(th->knockout,false));
	if (asAtomHandler::is<Array>(colors))
	{
		th->colors = _MR(asAtomHandler::as<Array>(colors));
		th->colors->incRef();
	}
	else
		th->colors = _MR(Class<Array>::getInstanceSNoArgs(wrk));

	if (asAtomHandler::is<Array>(alphas))
	{
		th->alphas = _MR(asAtomHandler::as<Array>(alphas));
		th->alphas->incRef();
	}
	else
		th->alphas = _MR(Class<Array>::getInstanceSNoArgs(wrk));
	if (asAtomHandler::is<Array>(ratios))
	{
		th->ratios = _MR(asAtomHandler::as<Array>(ratios));
		th->ratios->incRef();
	}
	else
		th->ratios = _MR(Class<Array>::getInstanceSNoArgs(wrk));
	// ensure all arrays have the same size
	th->alphas->resize(th->colors->size());
	th->ratios->resize(th->colors->size());
	th->onSetBlurX(0);
	th->onSetBlurY(0);
}

bool GradientGlowFilter::compareFILTER(const FILTER& filter) const
{
	bool ret = filter.FilterID == FILTER::FILTER_GRADIENTGLOW
			   && filter.GradientGlowFilter.Angle == this->angle
			   && filter.GradientGlowFilter.BlurX == this->blurX
			   && filter.GradientGlowFilter.BlurY == this->blurY
			   && filter.GradientGlowFilter.Distance == this->distance
			   && filter.GradientGlowFilter.Knockout == this->knockout
			   && filter.GradientGlowFilter.Passes == this->quality
			   && filter.GradientGlowFilter.Strength == this->strength
			   && this->type == (filter.GradientGlowFilter.InnerGlow ? "inner" : "outer");
	if (ret)
	{
		if (colors.isNull() || colors->size()!=filter.GradientGlowFilter.NumColors
			|| alphas.isNull() || alphas->size()!=filter.GradientGlowFilter.NumColors
			|| ratios.isNull() || ratios->size()!=filter.GradientGlowFilter.NumColors)
			ret = false;
	}
	if (ret)
	{
		if (filter.GradientGlowFilter.NumColors)
		{
			for (uint32_t  i = 0; i < filter.GradientGlowFilter.NumColors; i++)
			{
				asAtom a = asAtomHandler::invalidAtom;
				colors->at_nocheck(a,i);
				if (RGB(filter.GradientGlowFilter.GradientColors[i].Red,filter.GradientGlowFilter.GradientColors[i].Green,filter.GradientGlowFilter.GradientColors[i].Blue).toUInt() != asAtomHandler::toUInt(a))
				{
					ret = false;
					break;
				}
				alphas->at_nocheck(a,i);
				if (filter.GradientGlowFilter.GradientColors[i].af() != asAtomHandler::toNumber(a))
				{
					ret = false;
					break;
				}
			}
		}
	}
	if (ret)
	{
		if (filter.GradientGlowFilter.NumColors)
		{
			for (uint32_t  i = 0; i < filter.GradientGlowFilter.NumColors; i++)
			{
				asAtom a = asAtomHandler::invalidAtom;
				ratios->at_nocheck(a,i);
				if (filter.GradientGlowFilter.GradientRatio[i] != asAtomHandler::toUInt(a))
				{
					ret = false;
					break;
				}
			}
		}
	}
	return ret;
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
		int oX;
		int oY;
		float sX;
		float sY;
		getSystemState()->stageCoordinateMapping(getSystemState()->getRenderThread()->windowWidth, getSystemState()->getRenderThread()->windowHeight, oX, oY, sX, sY);

		args[0]=float(FILTERSTEP_GRADIENT_GLOW);
		args[1]=type=="inner";
		args[2]=knockout;
		args[3]=strength;
		args[4] = (cos(angle + M_PI) * distance * sX);
		args[5] = (sin(angle) * distance * sY);
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

void GradientGlowFilter::onSetBlurX(number_t oldval)
{
	if (blurX < 0.0)
		blurX = 0.0;
	if (blurX > 255.0)
		blurX = 255.0;
}

void GradientGlowFilter::onSetBlurY(number_t oldval)
{
	if (blurY < 0.0)
		blurY = 0.0;
	if (blurY > 255.0)
		blurY = 255.0;
}

