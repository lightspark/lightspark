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
	BitmapFilter* th=obj.as<BitmapFilter>();
	return asAtom::fromObject(th->cloneImpl());
}

GlowFilter::GlowFilter(Class_base* c):
	BitmapFilter(c), alpha(1.0), blurX(6.0), blurY(6.0), color(0xFF0000),
	inner(false), knockout(false), quality(1), strength(2.0)
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

ASFUNCTIONBODY_GETTER_SETTER(GlowFilter, alpha);
ASFUNCTIONBODY_GETTER_SETTER(GlowFilter, blurX);
ASFUNCTIONBODY_GETTER_SETTER(GlowFilter, blurY);
ASFUNCTIONBODY_GETTER_SETTER(GlowFilter, color);
ASFUNCTIONBODY_GETTER_SETTER(GlowFilter, inner);
ASFUNCTIONBODY_GETTER_SETTER(GlowFilter, knockout);
ASFUNCTIONBODY_GETTER_SETTER(GlowFilter, quality);
ASFUNCTIONBODY_GETTER_SETTER(GlowFilter, strength);

ASFUNCTIONBODY_ATOM(GlowFilter, _constructor)
{
	GlowFilter *th = obj.as<GlowFilter>();
	ARG_UNPACK_ATOM (th->color, 0xFF0000)
		(th->alpha, 1.0)
		(th->blurX, 6.0)
		(th->blurY, 6.0)
		(th->strength, 2.0)
		(th->quality, 1)
		(th->inner, false)
		(th->knockout, false);
	return asAtom::invalidAtom;
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
	BitmapFilter(c), alpha(1.0), angle(45), blurX(4.0), blurY(4.0),
	color(0), distance(4.0), hideObject(false), inner(false),
	knockout(false), quality(1), strength(1.0)
{
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

ASFUNCTIONBODY_ATOM(DropShadowFilter, _constructor)
{
	DropShadowFilter *th = obj.as<DropShadowFilter>();
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
	return asAtom::invalidAtom;
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
	BitmapFilter(c)
{
}

void GradientGlowFilter::sinit(Class_base* c)
{
	CLASS_SETUP(c, BitmapFilter, _constructor, CLASS_SEALED | CLASS_FINAL);
}

ASFUNCTIONBODY_ATOM(GradientGlowFilter, _constructor)
{
	//GradientGlowFilter *th = obj.as<GradientGlowFilter>();
	LOG(LOG_NOT_IMPLEMENTED,"GradientGlowFilter is not implemented");
	return asAtom::invalidAtom;
}

BitmapFilter* GradientGlowFilter::cloneImpl() const
{
	return Class<GradientGlowFilter>::getInstanceS(getSystemState());
}

BevelFilter::BevelFilter(Class_base* c):
	BitmapFilter(c)
{
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
 
ASFUNCTIONBODY_ATOM(BevelFilter, _constructor)
{
	//BevelFilter *th = obj.as<BevelFilter>();
	LOG(LOG_NOT_IMPLEMENTED,"GradientGlowFilter is not implemented");
	return asAtom::invalidAtom;
}

BitmapFilter* BevelFilter::cloneImpl() const
{
	return Class<BevelFilter>::getInstanceS(getSystemState());
}
ColorMatrixFilter::ColorMatrixFilter(Class_base* c):
	BitmapFilter(c),matrix(NULL)
{
}

void ColorMatrixFilter::sinit(Class_base* c)
{
	CLASS_SETUP(c, BitmapFilter, _constructor, CLASS_SEALED | CLASS_FINAL);
	REGISTER_GETTER_SETTER(c, matrix);
}

ASFUNCTIONBODY_GETTER_SETTER(ColorMatrixFilter, matrix);

ASFUNCTIONBODY_ATOM(ColorMatrixFilter, _constructor)
{
	ColorMatrixFilter *th = obj.as<ColorMatrixFilter>();
	ARG_UNPACK_ATOM(th->matrix,NullRef);
	return asAtom::invalidAtom;
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
	BitmapFilter(c),blurX(4.0),blurY(4.0),quality(1)
{
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

ASFUNCTIONBODY_ATOM(BlurFilter, _constructor)
{
	BlurFilter *th = obj.as<BlurFilter>();
	ARG_UNPACK_ATOM(th->blurX,4.0)(th->blurY,4.0)(th->quality,1);
	LOG(LOG_NOT_IMPLEMENTED,"BlurFilter is not implemented");
	return asAtom::invalidAtom;
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
	BitmapFilter(c)
{
}

void ConvolutionFilter::sinit(Class_base* c)
{
	CLASS_SETUP(c, BitmapFilter, _constructor, CLASS_SEALED | CLASS_FINAL);
}

ASFUNCTIONBODY_ATOM(ConvolutionFilter, _constructor)
{
	//ConvolutionFilter *th = obj.as<ConvolutionFilter>();
	LOG(LOG_NOT_IMPLEMENTED,"ConvolutionFilter is not implemented");
	return asAtom::invalidAtom;
}

BitmapFilter* ConvolutionFilter::cloneImpl() const
{
	return Class<ConvolutionFilter>::getInstanceS(getSystemState());
}

DisplacementMapFilter::DisplacementMapFilter(Class_base* c):
	BitmapFilter(c)
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

ASFUNCTIONBODY_ATOM(DisplacementMapFilter, _constructor)
{
	//DisplacementMapFilter *th = obj.as<DisplacementMapFilter>();
	LOG(LOG_NOT_IMPLEMENTED,"DisplacementMapFilter is not implemented");
	return asAtom::invalidAtom;
}

BitmapFilter* DisplacementMapFilter::cloneImpl() const
{
	return Class<DisplacementMapFilter>::getInstanceS(getSystemState());
}

GradientBevelFilter::GradientBevelFilter(Class_base* c):
	BitmapFilter(c)
{
}

void GradientBevelFilter::sinit(Class_base* c)
{
	CLASS_SETUP(c, BitmapFilter, _constructor, CLASS_SEALED | CLASS_FINAL);
}

ASFUNCTIONBODY_ATOM(GradientBevelFilter, _constructor)
{
	//GradientBevelFilter *th = obj.as<GradientBevelFilter>();
	LOG(LOG_NOT_IMPLEMENTED,"GradientBevelFilter is not implemented");
	return asAtom::invalidAtom;
}

BitmapFilter* GradientBevelFilter::cloneImpl() const
{
	return Class<GradientBevelFilter>::getInstanceS(getSystemState());
}

ShaderFilter::ShaderFilter(Class_base* c):
	BitmapFilter(c)
{
}

void ShaderFilter::sinit(Class_base* c)
{
	CLASS_SETUP(c, BitmapFilter, _constructor, CLASS_SEALED | CLASS_FINAL);
}

ASFUNCTIONBODY_ATOM(ShaderFilter, _constructor)
{
	//ShaderFilter *th = obj.as<ShaderFilter>();
	LOG(LOG_NOT_IMPLEMENTED,"ShaderFilter is not implemented");
	return asAtom::invalidAtom;
}

BitmapFilter* ShaderFilter::cloneImpl() const
{
	return Class<ShaderFilter>::getInstanceS(getSystemState());
}

void BitmapFilterQuality::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructorNotInstantiatable, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("HIGH",nsNameAndKind(),asAtom(3),DECLARED_TRAIT);
	c->setVariableAtomByQName("LOW",nsNameAndKind(),asAtom(1),DECLARED_TRAIT);
	c->setVariableAtomByQName("MEDIUM",nsNameAndKind(),asAtom(3),DECLARED_TRAIT);
}

void DisplacementMapFilterMode::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("CLAMP",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"clamp"),DECLARED_TRAIT);
	c->setVariableAtomByQName("COLOR",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"color"),DECLARED_TRAIT);
	c->setVariableAtomByQName("IGNORE",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"ignore"),DECLARED_TRAIT);
	c->setVariableAtomByQName("WRAP",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"wrap"),DECLARED_TRAIT);
}
