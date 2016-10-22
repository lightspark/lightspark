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

ASFUNCTIONBODY(BitmapFilter,clone)
{
	BitmapFilter* th=static_cast<BitmapFilter*>(obj);
	return th->cloneImpl();
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

ASFUNCTIONBODY(GlowFilter, _constructor)
{
	GlowFilter *th = obj->as<GlowFilter>();
	ARG_UNPACK (th->color, 0xFF0000)
		(th->alpha, 1.0)
		(th->blurX, 6.0)
		(th->blurY, 6.0)
		(th->strength, 2.0)
		(th->quality, 1)
		(th->inner, false)
		(th->knockout, false);
	return NULL;
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

ASFUNCTIONBODY(DropShadowFilter, _constructor)
{
	DropShadowFilter *th = obj->as<DropShadowFilter>();
	ARG_UNPACK (th->distance, 4.0)
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
	return NULL;
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

ASFUNCTIONBODY(GradientGlowFilter, _constructor)
{
	//GradientGlowFilter *th = obj->as<GradientGlowFilter>();
	LOG(LOG_NOT_IMPLEMENTED,"GradientGlowFilter is not implemented");
	return NULL;
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
}

ASFUNCTIONBODY(BevelFilter, _constructor)
{
	//BevelFilter *th = obj->as<BevelFilter>();
	LOG(LOG_NOT_IMPLEMENTED,"GradientGlowFilter is not implemented");
	return NULL;
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

ASFUNCTIONBODY(ColorMatrixFilter, _constructor)
{
	ColorMatrixFilter *th = obj->as<ColorMatrixFilter>();
	ARG_UNPACK(th->matrix,NullRef);
	return NULL;
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

ASFUNCTIONBODY(BlurFilter, _constructor)
{
	BlurFilter *th = obj->as<BlurFilter>();
	ARG_UNPACK(th->blurX,4.0)(th->blurY,4.0)(th->quality,1);
	LOG(LOG_NOT_IMPLEMENTED,"BlurFilter is not implemented");
	return NULL;
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

ASFUNCTIONBODY(ConvolutionFilter, _constructor)
{
	//ConvolutionFilter *th = obj->as<ConvolutionFilter>();
	LOG(LOG_NOT_IMPLEMENTED,"ConvolutionFilter is not implemented");
	return NULL;
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
}

ASFUNCTIONBODY(DisplacementMapFilter, _constructor)
{
	//DisplacementMapFilter *th = obj->as<DisplacementMapFilter>();
	LOG(LOG_NOT_IMPLEMENTED,"DisplacementMapFilter is not implemented");
	return NULL;
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

ASFUNCTIONBODY(GradientBevelFilter, _constructor)
{
	//GradientBevelFilter *th = obj->as<GradientBevelFilter>();
	LOG(LOG_NOT_IMPLEMENTED,"GradientBevelFilter is not implemented");
	return NULL;
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

ASFUNCTIONBODY(ShaderFilter, _constructor)
{
	//ShaderFilter *th = obj->as<ShaderFilter>();
	LOG(LOG_NOT_IMPLEMENTED,"ShaderFilter is not implemented");
	return NULL;
}

BitmapFilter* ShaderFilter::cloneImpl() const
{
	return Class<ShaderFilter>::getInstanceS(getSystemState());
}

void BitmapFilterQuality::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructorNotInstantiatable, CLASS_SEALED | CLASS_FINAL);
	c->setVariableByQName("HIGH","",abstract_i(c->getSystemState(),3),DECLARED_TRAIT);
	c->setVariableByQName("LOW","",abstract_i(c->getSystemState(),1),DECLARED_TRAIT);
	c->setVariableByQName("MEDIUM","",abstract_i(c->getSystemState(),3),DECLARED_TRAIT);
}
