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

#include "scripting/flash/filters/flashfilters.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include "scripting/flash/display/BitmapData.h"
#include "scripting/flash/geom/flashgeom.h"

using namespace std;
using namespace lightspark;

void BitmapFilter::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructorNotInstantiatable, CLASS_SEALED);
	c->setDeclaredMethodByQName("clone","",Class<IFunction>::getFunction(c->getSystemState(),clone),NORMAL_METHOD,true);
}

BitmapFilter* BitmapFilter::cloneImpl() const
{
	return Class<BitmapFilter>::getInstanceS(getSystemState());
}

ASFUNCTIONBODY_ATOM(BitmapFilter,clone)
{
	BitmapFilter* th=asAtomHandler::as<BitmapFilter>(obj);
	ret = asAtomHandler::fromObject(th->cloneImpl());
}

GlowFilter::GlowFilter(Class_base* c):
	BitmapFilter(c,SUBTYPE_GLOWFILTER), alpha(1.0), blurX(6.0), blurY(6.0), color(0xFF0000),
	inner(false), knockout(false), quality(1), strength(2.0)
{
}
GlowFilter::GlowFilter(Class_base* c,const GLOWFILTER& filter):
	BitmapFilter(c,SUBTYPE_GLOWFILTER), alpha(filter.GlowColor.af()), blurX(filter.BlurX), blurY(filter.BlurY), color(RGB(filter.GlowColor.Red,filter.GlowColor.Green,filter.GlowColor.Blue).toUInt()),
	inner(filter.InnerGlow), knockout(filter.Knockout), quality(filter.Passes), strength(filter.Strength)
{
	LOG(LOG_NOT_IMPLEMENTED,"GlowFilter from Tag");
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

ASFUNCTIONBODY_GETTER_SETTER(GlowFilter, alpha);
ASFUNCTIONBODY_GETTER_SETTER(GlowFilter, blurX);
ASFUNCTIONBODY_GETTER_SETTER(GlowFilter, blurY);
ASFUNCTIONBODY_GETTER_SETTER(GlowFilter, color);
ASFUNCTIONBODY_GETTER_SETTER(GlowFilter, inner);
ASFUNCTIONBODY_GETTER_SETTER(GlowFilter, knockout);
ASFUNCTIONBODY_GETTER_SETTER(GlowFilter, quality);
ASFUNCTIONBODY_GETTER_SETTER(GlowFilter, strength);

ASFUNCTIONBODY_ATOM(GlowFilter,_constructor)
{
	GlowFilter *th = asAtomHandler::as<GlowFilter>(obj);
	ARG_UNPACK_ATOM (th->color, 0xFF0000)
		(th->alpha, 1.0)
		(th->blurX, 6.0)
		(th->blurY, 6.0)
		(th->strength, 2.0)
		(th->quality, 1)
		(th->inner, false)
		(th->knockout, false);
	LOG(LOG_NOT_IMPLEMENTED,"GlowFilter does nothing");
}

BitmapFilter* GlowFilter::cloneImpl() const
{
	GlowFilter *cloned = Class<GlowFilter>::getInstanceS(getSystemState());
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

DropShadowFilter::DropShadowFilter(Class_base* c):
	BitmapFilter(c,SUBTYPE_DROPSHADOWFILTER), alpha(1.0), angle(45), blurX(4.0), blurY(4.0),
	color(0), distance(4.0), hideObject(false), inner(false),
	knockout(false), quality(1), strength(1.0)
{
}
DropShadowFilter::DropShadowFilter(Class_base* c,const DROPSHADOWFILTER& filter):
	BitmapFilter(c,SUBTYPE_DROPSHADOWFILTER), alpha(filter.DropShadowColor.af()), angle(filter.Angle), blurX(filter.BlurX), blurY(filter.BlurY),
	color(RGB(filter.DropShadowColor.Red,filter.DropShadowColor.Green,filter.DropShadowColor.Blue).toUInt()), distance(filter.Distance), hideObject(false), inner(filter.InnerShadow),
	knockout(filter.Knockout), quality(filter.Passes), strength(filter.Strength)
{
	LOG(LOG_NOT_IMPLEMENTED,"DropShadowFilter from Tag");
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

ASFUNCTIONBODY_GETTER_SETTER(DropShadowFilter, alpha);
ASFUNCTIONBODY_GETTER_SETTER(DropShadowFilter, angle);
ASFUNCTIONBODY_GETTER_SETTER(DropShadowFilter, blurX);
ASFUNCTIONBODY_GETTER_SETTER(DropShadowFilter, blurY);
ASFUNCTIONBODY_GETTER_SETTER(DropShadowFilter, color);
ASFUNCTIONBODY_GETTER_SETTER(DropShadowFilter, distance);
ASFUNCTIONBODY_GETTER_SETTER(DropShadowFilter, hideObject);
ASFUNCTIONBODY_GETTER_SETTER(DropShadowFilter, inner);
ASFUNCTIONBODY_GETTER_SETTER(DropShadowFilter, knockout);
ASFUNCTIONBODY_GETTER_SETTER(DropShadowFilter, quality);
ASFUNCTIONBODY_GETTER_SETTER(DropShadowFilter, strength);

ASFUNCTIONBODY_ATOM(DropShadowFilter,_constructor)
{
	DropShadowFilter *th = asAtomHandler::as<DropShadowFilter>(obj);
	ARG_UNPACK_ATOM (th->distance, 4.0)
		(th->angle, 45)
		(th->color, 0)
		(th->alpha, 1.0)
		(th->blurX, 4.0)
		(th->blurY, 4.0)
		(th->strength, 1.0)
		(th->quality, 1)
		(th->inner, false)
		(th->knockout, false)
		(th->hideObject, false);
	LOG(LOG_NOT_IMPLEMENTED,"DropShadowFilter does nothing");
}

BitmapFilter* DropShadowFilter::cloneImpl() const
{
	DropShadowFilter *cloned = Class<DropShadowFilter>::getInstanceS(getSystemState());
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

GradientGlowFilter::GradientGlowFilter(Class_base* c):
	BitmapFilter(c,SUBTYPE_GRADIENTGLOWFILTER),distance(4.0),angle(45), blurX(4.0), blurY(4.0), strength(1), quality(1), type("inner"), knockout(false)
{
}
GradientGlowFilter::GradientGlowFilter(Class_base* c, const GRADIENTGLOWFILTER& filter):
	BitmapFilter(c,SUBTYPE_GRADIENTGLOWFILTER),distance(filter.Distance),angle(filter.Angle), blurX(filter.BlurX), blurY(filter.BlurY), strength(filter.Strength), quality(filter.Passes),
	type("inner"),// TODO: is type set based on "onTop" ?
	knockout(filter.Knockout)
{
	LOG(LOG_NOT_IMPLEMENTED,"GradientGlowFilter from Tag");
	if (filter.GradientColors.size())
	{
		colors = _MR(Class<Array>::getInstanceSNoArgs(c->getSystemState()));
		alphas = _MR(Class<Array>::getInstanceSNoArgs(c->getSystemState()));
		auto it = filter.GradientColors.begin();
		while (it != filter.GradientColors.end())
		{
			colors->push(asAtomHandler::fromUInt(RGB(it->Red,it->Green,it->Blue).toUInt()));
			alphas->push(asAtomHandler::fromNumber(c->getSystemState(),it->af(),false));
			it++;
		}
	}
	if (filter.GradientRatio.size())
	{
		ratios = _MR(Class<Array>::getInstanceSNoArgs(c->getSystemState()));
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

ASFUNCTIONBODY_GETTER_SETTER(GradientGlowFilter, distance);
ASFUNCTIONBODY_GETTER_SETTER(GradientGlowFilter, angle);
ASFUNCTIONBODY_GETTER_SETTER(GradientGlowFilter, colors);
ASFUNCTIONBODY_GETTER_SETTER(GradientGlowFilter, alphas);
ASFUNCTIONBODY_GETTER_SETTER(GradientGlowFilter, ratios);
ASFUNCTIONBODY_GETTER_SETTER(GradientGlowFilter, blurX);
ASFUNCTIONBODY_GETTER_SETTER(GradientGlowFilter, blurY);
ASFUNCTIONBODY_GETTER_SETTER(GradientGlowFilter, strength);
ASFUNCTIONBODY_GETTER_SETTER(GradientGlowFilter, quality);
ASFUNCTIONBODY_GETTER_SETTER(GradientGlowFilter, type);
ASFUNCTIONBODY_GETTER_SETTER(GradientGlowFilter, knockout);

ASFUNCTIONBODY_ATOM(GradientGlowFilter,_constructor)
{
	//GradientGlowFilter *th = obj.as<GradientGlowFilter>();
	LOG(LOG_NOT_IMPLEMENTED,"GradientGlowFilter is not implemented");
}

BitmapFilter* GradientGlowFilter::cloneImpl() const
{
	GradientGlowFilter *cloned = Class<GradientGlowFilter>::getInstanceS(getSystemState());
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

BevelFilter::BevelFilter(Class_base* c):
	BitmapFilter(c,SUBTYPE_BEVELFILTER), angle(45), blurX(4.0), blurY(4.0),distance(4.0),
	highlightAlpha(1.0), highlightColor(0xFFFFFF),
	knockout(false), quality(1),
	shadowAlpha(1.0), shadowColor(0x000000),
	strength(1), type("inner")
{
}
BevelFilter::BevelFilter(Class_base* c,const BEVELFILTER& filter):
	BitmapFilter(c,SUBTYPE_BEVELFILTER),angle(filter.Angle),blurX(filter.BlurX), blurY(filter.BlurY),distance(filter.Distance), 
	highlightAlpha(filter.HighlightColor.af()), highlightColor(RGB(filter.HighlightColor.Red,filter.HighlightColor.Green,filter.HighlightColor.Blue).toUInt()),
	knockout(filter.Knockout), quality(filter.Passes),
	shadowAlpha(filter.ShadowColor.af()), shadowColor(RGB(filter.ShadowColor.Red,filter.ShadowColor.Green,filter.ShadowColor.Blue).toUInt()),
	strength(filter.Strength), type("inner") // TODO: is type set based on "onTop" ?
{
	LOG(LOG_NOT_IMPLEMENTED,"BevelFilter from Tag");
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
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(BevelFilter,angle);
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(BevelFilter,blurX);
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(BevelFilter,blurY);
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(BevelFilter,distance);
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(BevelFilter,highlightAlpha);
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(BevelFilter,highlightColor);
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(BevelFilter,knockout);
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(BevelFilter,quality);
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(BevelFilter,shadowAlpha);
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(BevelFilter,shadowColor);
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(BevelFilter,strength);
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(BevelFilter,type);
 
ASFUNCTIONBODY_ATOM(BevelFilter,_constructor)
{
	//BevelFilter *th = obj.as<BevelFilter>();
	LOG(LOG_NOT_IMPLEMENTED,"BevelFilter is not implemented");
}

BitmapFilter* BevelFilter::cloneImpl() const
{
	BevelFilter* cloned = Class<BevelFilter>::getInstanceS(getSystemState());
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
ColorMatrixFilter::ColorMatrixFilter(Class_base* c):
	BitmapFilter(c,SUBTYPE_COLORMATRIXFILTER)
{
}
ColorMatrixFilter::ColorMatrixFilter(Class_base* c,const COLORMATRIXFILTER& filter):
	BitmapFilter(c,SUBTYPE_COLORMATRIXFILTER)
{
	LOG(LOG_NOT_IMPLEMENTED,"ColorMatrixFilter from Tag");
	matrix = _MR(Class<Array>::getInstanceSNoArgs(c->getSystemState()));
	for (uint32_t i = 0; i < 20 ; i++)
	{
		FLOAT f = filter.Matrix[i];
		matrix->push(asAtomHandler::fromNumber(c->getSystemState(),f,false));
	}
}

void ColorMatrixFilter::sinit(Class_base* c)
{
	CLASS_SETUP(c, BitmapFilter, _constructor, CLASS_SEALED | CLASS_FINAL);
	REGISTER_GETTER_SETTER(c, matrix);
}

ASFUNCTIONBODY_GETTER_SETTER(ColorMatrixFilter, matrix);

ASFUNCTIONBODY_ATOM(ColorMatrixFilter,_constructor)
{
	ColorMatrixFilter *th = asAtomHandler::as<ColorMatrixFilter>(obj);
	ARG_UNPACK_ATOM(th->matrix,NullRef);
	LOG(LOG_NOT_IMPLEMENTED,"ColorMatrixFilter does nothing");
}

BitmapFilter* ColorMatrixFilter::cloneImpl() const
{
	ColorMatrixFilter *cloned = Class<ColorMatrixFilter>::getInstanceS(getSystemState());
	if (!matrix.isNull())
	{
		matrix->incRef();
		cloned->matrix = matrix;
	}
	return cloned;
}
BlurFilter::BlurFilter(Class_base* c):
	BitmapFilter(c,SUBTYPE_BLURFILTER),blurX(4.0),blurY(4.0),quality(1)
{
}
BlurFilter::BlurFilter(Class_base* c,const BLURFILTER& filter):
	BitmapFilter(c,SUBTYPE_BLURFILTER),blurX(filter.BlurX),blurY(filter.BlurY),quality(filter.Passes)
{
	LOG(LOG_NOT_IMPLEMENTED,"BlurFilter from Tag");
}

void BlurFilter::sinit(Class_base* c)
{
	CLASS_SETUP(c, BitmapFilter, _constructor, CLASS_SEALED | CLASS_FINAL);
	REGISTER_GETTER_SETTER(c, blurX);
	REGISTER_GETTER_SETTER(c, blurY);
	REGISTER_GETTER_SETTER(c, quality);
}
ASFUNCTIONBODY_GETTER_SETTER(BlurFilter, blurX);
ASFUNCTIONBODY_GETTER_SETTER(BlurFilter, blurY);
ASFUNCTIONBODY_GETTER_SETTER(BlurFilter, quality);

ASFUNCTIONBODY_ATOM(BlurFilter,_constructor)
{
	BlurFilter *th = asAtomHandler::as<BlurFilter>(obj);
	ARG_UNPACK_ATOM(th->blurX,4.0)(th->blurY,4.0)(th->quality,1);
	LOG(LOG_NOT_IMPLEMENTED,"BlurFilter is not implemented");
}

BitmapFilter* BlurFilter::cloneImpl() const
{
	BlurFilter* cloned = Class<BlurFilter>::getInstanceS(getSystemState());
	cloned->blurX = blurX;
	cloned->blurY = blurY;
	cloned->quality = quality;
	return cloned;
}

ConvolutionFilter::ConvolutionFilter(Class_base* c):
	BitmapFilter(c,SUBTYPE_CONVOLUTIONFILTER),
	alpha(0.0),
	bias(0.0),
	clamp(true),
	color(0),
	divisor(1.0),
	matrixX(0),
	matrixY(0),
	preserveAlpha(true)
{
}
ConvolutionFilter::ConvolutionFilter(Class_base* c, const CONVOLUTIONFILTER& filter):
	BitmapFilter(c,SUBTYPE_CONVOLUTIONFILTER),
	alpha(filter.DefaultColor.af()),
	bias((FLOAT)filter.Bias),
	clamp(filter.Clamp),
	color(RGB(filter.DefaultColor.Red,filter.DefaultColor.Green,filter.DefaultColor.Blue).toUInt()),
	divisor((FLOAT)filter.Divisor),
	matrixX(filter.MatrixX),
	matrixY((uint32_t)filter.MatrixY),
	preserveAlpha(filter.PreserveAlpha)
{
	LOG(LOG_NOT_IMPLEMENTED,"ConvolutionFilter from Tag");
	if (filter.Matrix.size())
	{
		matrix = _MR(Class<Array>::getInstanceSNoArgs(c->getSystemState()));
		auto it = filter.Matrix.begin();
		while (it != filter.Matrix.end())
		{
			matrix->push(asAtomHandler::fromNumber(c->getSystemState(),(FLOAT)*it,false));
			it++;
		}
	}
}

void ConvolutionFilter::sinit(Class_base* c)
{
	CLASS_SETUP(c, BitmapFilter, _constructor, CLASS_SEALED | CLASS_FINAL);
	REGISTER_GETTER_SETTER(c,alpha);
	REGISTER_GETTER_SETTER(c,bias);
	REGISTER_GETTER_SETTER(c,clamp);
	REGISTER_GETTER_SETTER(c,color);
	REGISTER_GETTER_SETTER(c,divisor);
	REGISTER_GETTER_SETTER(c,matrix);
	REGISTER_GETTER_SETTER(c,matrixX);
	REGISTER_GETTER_SETTER(c,matrixY);
	REGISTER_GETTER_SETTER(c,preserveAlpha);
}
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(ConvolutionFilter,alpha);
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(ConvolutionFilter,bias);
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(ConvolutionFilter,clamp);
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(ConvolutionFilter,color);
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(ConvolutionFilter,divisor);
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(ConvolutionFilter,matrix);
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(ConvolutionFilter,matrixX);
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(ConvolutionFilter,matrixY);
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(ConvolutionFilter,preserveAlpha);

ASFUNCTIONBODY_ATOM(ConvolutionFilter,_constructor)
{
	//ConvolutionFilter *th = obj.as<ConvolutionFilter>();
	LOG(LOG_NOT_IMPLEMENTED,"ConvolutionFilter is not implemented");
}

BitmapFilter* ConvolutionFilter::cloneImpl() const
{
	ConvolutionFilter* cloned = Class<ConvolutionFilter>::getInstanceS(getSystemState());
	cloned->alpha = alpha;
	cloned->bias = bias;
	cloned->clamp = clamp;
	cloned->color = color;
	cloned->divisor = divisor;
	cloned->matrix = matrix;
	cloned-> matrixX = matrixX;
	cloned-> matrixY = matrixY;
	cloned->preserveAlpha = preserveAlpha;
	return cloned;
}

DisplacementMapFilter::DisplacementMapFilter(Class_base* c):
	BitmapFilter(c,SUBTYPE_DISPLACEMENTFILTER)
{
}

void DisplacementMapFilter::sinit(Class_base* c)
{
	CLASS_SETUP(c, BitmapFilter, _constructor, CLASS_SEALED | CLASS_FINAL);
	REGISTER_GETTER_SETTER(c,alpha);
	REGISTER_GETTER_SETTER(c,color);
	REGISTER_GETTER_SETTER(c,componentX);
	REGISTER_GETTER_SETTER(c,componentY);
	REGISTER_GETTER_SETTER(c,mapBitmap);
	REGISTER_GETTER_SETTER(c,mapPoint);
	REGISTER_GETTER_SETTER(c,mode);
	REGISTER_GETTER_SETTER(c,scaleX);
	REGISTER_GETTER_SETTER(c,scaleY);
}
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(DisplacementMapFilter,alpha);
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(DisplacementMapFilter,color);
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(DisplacementMapFilter,componentX);
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(DisplacementMapFilter,componentY);
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(DisplacementMapFilter,mapBitmap);
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(DisplacementMapFilter,mapPoint);
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(DisplacementMapFilter,mode);
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(DisplacementMapFilter,scaleX);
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(DisplacementMapFilter,scaleY);

ASFUNCTIONBODY_ATOM(DisplacementMapFilter,_constructor)
{
	//DisplacementMapFilter *th = obj.as<DisplacementMapFilter>();
	LOG(LOG_NOT_IMPLEMENTED,"DisplacementMapFilter is not implemented");
}

BitmapFilter* DisplacementMapFilter::cloneImpl() const
{
	DisplacementMapFilter* cloned = Class<DisplacementMapFilter>::getInstanceS(getSystemState());
	cloned->alpha = alpha;
	cloned->color = color;
	cloned->componentX = componentX;
	cloned->componentY = componentY;
	cloned->mapBitmap = mapBitmap;
	cloned->mapPoint = mapPoint;
	cloned->mode = mode;
	cloned->scaleX = scaleX;
	cloned->scaleY = scaleY;
	return cloned;
}

GradientBevelFilter::GradientBevelFilter(Class_base* c):
	BitmapFilter(c,SUBTYPE_GRADIENTBEVELFILTER),
	angle(45),
	blurX(4.0),
	blurY(4.0),
	distance(4.0),
	knockout(false),
	quality(1),
	strength(1),
	type("inner")
{
}
GradientBevelFilter::GradientBevelFilter(Class_base* c, const GRADIENTBEVELFILTER& filter):
	BitmapFilter(c,SUBTYPE_GRADIENTBEVELFILTER),
	angle(filter.Angle),
	blurX(filter.BlurX),
	blurY(filter.BlurY),
	distance(filter.Distance),
	knockout(filter.Knockout),
	quality(filter.Passes),
	strength(filter.Strength),
	type("inner") // TODO: is type set based on "onTop" ?
{
	LOG(LOG_NOT_IMPLEMENTED,"GradientBevelFilter from Tag");
	if (filter.GradientColors.size())
	{
		colors = _MR(Class<Array>::getInstanceSNoArgs(c->getSystemState()));
		alphas = _MR(Class<Array>::getInstanceSNoArgs(c->getSystemState()));
		auto it = filter.GradientColors.begin();
		while (it != filter.GradientColors.end())
		{
			colors->push(asAtomHandler::fromUInt(RGB(it->Red,it->Green,it->Blue).toUInt()));
			alphas->push(asAtomHandler::fromNumber(c->getSystemState(),it->af(),false));
			it++;
		}
	}
	if (filter.GradientRatio.size())
	{
		ratios = _MR(Class<Array>::getInstanceSNoArgs(c->getSystemState()));
		auto it = filter.GradientRatio.begin();
		while (it != filter.GradientRatio.end())
		{
			ratios->push(asAtomHandler::fromUInt(*it));
			it++;
		}
	}
}


void GradientBevelFilter::sinit(Class_base* c)
{
	CLASS_SETUP(c, BitmapFilter, _constructor, CLASS_SEALED | CLASS_FINAL);
	REGISTER_GETTER_SETTER(c,alphas);
	REGISTER_GETTER_SETTER(c,angle);
	REGISTER_GETTER_SETTER(c,blurX);
	REGISTER_GETTER_SETTER(c,blurY);
	REGISTER_GETTER_SETTER(c,colors);
	REGISTER_GETTER_SETTER(c,distance);
	REGISTER_GETTER_SETTER(c,knockout);
	REGISTER_GETTER_SETTER(c,quality);
	REGISTER_GETTER_SETTER(c,ratios);
	REGISTER_GETTER_SETTER(c,strength);
	REGISTER_GETTER_SETTER(c,type);
}

ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(GradientBevelFilter,alphas);
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(GradientBevelFilter,angle);
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(GradientBevelFilter,blurX);
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(GradientBevelFilter,blurY);
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(GradientBevelFilter,colors);
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(GradientBevelFilter,distance);
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(GradientBevelFilter,knockout);
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(GradientBevelFilter,quality);
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(GradientBevelFilter,ratios);
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(GradientBevelFilter,strength);
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(GradientBevelFilter,type);

ASFUNCTIONBODY_ATOM(GradientBevelFilter,_constructor)
{
	//GradientBevelFilter *th = obj.as<GradientBevelFilter>();
	LOG(LOG_NOT_IMPLEMENTED,"GradientBevelFilter is not implemented");
}

BitmapFilter* GradientBevelFilter::cloneImpl() const
{
	GradientBevelFilter* cloned = Class<GradientBevelFilter>::getInstanceS(getSystemState());
	cloned->alphas = alphas;
	cloned->angle = angle;
	cloned->blurX = blurX;
	cloned->blurY = blurY;
	cloned->colors = colors;
	cloned->distance = distance;
	cloned->knockout = knockout;
	cloned->quality = quality;
	cloned->ratios = ratios;
	cloned->strength = strength;
	cloned->type = type;
	return cloned;
}

ShaderFilter::ShaderFilter(Class_base* c):
	BitmapFilter(c,SUBTYPE_SHADERFILTER)
{
}

void ShaderFilter::sinit(Class_base* c)
{
	CLASS_SETUP(c, BitmapFilter, _constructor, CLASS_SEALED | CLASS_FINAL);
}

ASFUNCTIONBODY_ATOM(ShaderFilter,_constructor)
{
	//ShaderFilter *th = obj.as<ShaderFilter>();
	LOG(LOG_NOT_IMPLEMENTED,"ShaderFilter is not implemented");
}

BitmapFilter* ShaderFilter::cloneImpl() const
{
	return Class<ShaderFilter>::getInstanceS(getSystemState());
}

void BitmapFilterQuality::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructorNotInstantiatable, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("HIGH",nsNameAndKind(),asAtomHandler::fromUInt(3),DECLARED_TRAIT);
	c->setVariableAtomByQName("LOW",nsNameAndKind(),asAtomHandler::fromUInt(1),DECLARED_TRAIT);
	c->setVariableAtomByQName("MEDIUM",nsNameAndKind(),asAtomHandler::fromUInt(2),DECLARED_TRAIT);
}

void DisplacementMapFilterMode::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("CLAMP",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"clamp"),DECLARED_TRAIT);
	c->setVariableAtomByQName("COLOR",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"color"),DECLARED_TRAIT);
	c->setVariableAtomByQName("IGNORE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"ignore"),DECLARED_TRAIT);
	c->setVariableAtomByQName("WRAP",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"wrap"),DECLARED_TRAIT);
}

void BitmapFilterType::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructorNotInstantiatable, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("FULL",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"full"),DECLARED_TRAIT);
	c->setVariableAtomByQName("INNER",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"inner"),DECLARED_TRAIT);
	c->setVariableAtomByQName("OUTER",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"outer"),DECLARED_TRAIT);
}
