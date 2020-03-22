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

#ifndef SCRIPTING_FLASH_FILTERS_FLASHFILTERS_H
#define SCRIPTING_FLASH_FILTERS_FLASHFILTERS_H 1

#include "compat.h"
#include "asobject.h"

namespace lightspark
{

class BitmapFilter: public ASObject
{
private:
	virtual BitmapFilter* cloneImpl() const;
public:
	BitmapFilter(Class_base* c, CLASS_SUBTYPE st=SUBTYPE_BITMAPFILTER):ASObject(c,T_OBJECT,st){}
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(clone);
};

class GlowFilter: public BitmapFilter
{
private:
	ASPROPERTY_GETTER_SETTER(number_t, alpha);
	ASPROPERTY_GETTER_SETTER(number_t, blurX);
	ASPROPERTY_GETTER_SETTER(number_t, blurY);
	ASPROPERTY_GETTER_SETTER(uint32_t, color);
	ASPROPERTY_GETTER_SETTER(bool, inner);
	ASPROPERTY_GETTER_SETTER(bool, knockout);
	ASPROPERTY_GETTER_SETTER(int32_t, quality);
	ASPROPERTY_GETTER_SETTER(number_t, strength);
	BitmapFilter* cloneImpl() const override;
public:
	GlowFilter(Class_base* c);
	GlowFilter(Class_base* c,const GLOWFILTER& filter);
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(_constructor);
};

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
	DropShadowFilter(Class_base* c);
	DropShadowFilter(Class_base* c,const DROPSHADOWFILTER& filter);
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(_constructor);
};

class GradientGlowFilter: public BitmapFilter
{
private:
	BitmapFilter* cloneImpl() const override;
public:
	GradientGlowFilter(Class_base* c);
	GradientGlowFilter(Class_base* c, const GRADIENTGLOWFILTER& filter);
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(_constructor);
	ASPROPERTY_GETTER_SETTER(number_t,distance);
	ASPROPERTY_GETTER_SETTER(number_t, angle);
	ASPROPERTY_GETTER_SETTER(_NR<Array>,colors);
	ASPROPERTY_GETTER_SETTER(_NR<Array>,alphas);
	ASPROPERTY_GETTER_SETTER(_NR<Array>,ratios);
	ASPROPERTY_GETTER_SETTER(number_t,blurX);
	ASPROPERTY_GETTER_SETTER(number_t,blurY);
	ASPROPERTY_GETTER_SETTER(number_t,strength);
	ASPROPERTY_GETTER_SETTER(int32_t,quality);
	ASPROPERTY_GETTER_SETTER(tiny_string,type);
	ASPROPERTY_GETTER_SETTER(bool,knockout);
};

class BevelFilter: public BitmapFilter
{
private:
	BitmapFilter* cloneImpl() const override;
public:
	BevelFilter(Class_base* c);
	BevelFilter(Class_base* c,const BEVELFILTER& filter);
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(_constructor);
	ASPROPERTY_GETTER_SETTER(number_t, angle);
	ASPROPERTY_GETTER_SETTER(number_t,blurX);
	ASPROPERTY_GETTER_SETTER(number_t,blurY);
	ASPROPERTY_GETTER_SETTER(number_t,distance);
	ASPROPERTY_GETTER_SETTER(number_t,highlightAlpha);
	ASPROPERTY_GETTER_SETTER(uint32_t,highlightColor);
	ASPROPERTY_GETTER_SETTER(bool,knockout);
	ASPROPERTY_GETTER_SETTER(int32_t,quality);
	ASPROPERTY_GETTER_SETTER(number_t,shadowAlpha);
	ASPROPERTY_GETTER_SETTER(uint32_t,shadowColor);
	ASPROPERTY_GETTER_SETTER(number_t,strength);
	ASPROPERTY_GETTER_SETTER(tiny_string,type);
};
class ColorMatrixFilter: public BitmapFilter
{
private:
	BitmapFilter* cloneImpl() const override;
public:
	ColorMatrixFilter(Class_base* c);
	ColorMatrixFilter(Class_base* c,const COLORMATRIXFILTER& filter);
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(_constructor);
	ASPROPERTY_GETTER_SETTER(_NR<Array>, matrix);
};
class BlurFilter: public BitmapFilter
{
private:
	BitmapFilter* cloneImpl() const override;
public:
	BlurFilter(Class_base* c);
	BlurFilter(Class_base* c,const BLURFILTER& filter);
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(_constructor);
	ASPROPERTY_GETTER_SETTER(number_t, blurX);
	ASPROPERTY_GETTER_SETTER(number_t, blurY);
	ASPROPERTY_GETTER_SETTER(int, quality);
};
class ConvolutionFilter: public BitmapFilter
{
private:
	BitmapFilter* cloneImpl() const override;
public:
	ConvolutionFilter(Class_base* c);
	ConvolutionFilter(Class_base* c,const CONVOLUTIONFILTER& filter);
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(_constructor);
	ASPROPERTY_GETTER_SETTER(number_t,alpha);
	ASPROPERTY_GETTER_SETTER(number_t,bias);
	ASPROPERTY_GETTER_SETTER(bool, clamp);
	ASPROPERTY_GETTER_SETTER(uint32_t, color);
	ASPROPERTY_GETTER_SETTER(number_t, divisor);
	ASPROPERTY_GETTER_SETTER(_NR<Array>, matrix);
	ASPROPERTY_GETTER_SETTER(number_t, matrixX);
	ASPROPERTY_GETTER_SETTER(number_t, matrixY);
	ASPROPERTY_GETTER_SETTER(bool, preserveAlpha);
};
class DisplacementMapFilter: public BitmapFilter
{
private:
	BitmapFilter* cloneImpl() const override;
public:
	DisplacementMapFilter(Class_base* c);
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(_constructor);
	ASPROPERTY_GETTER_SETTER(number_t,alpha);
	ASPROPERTY_GETTER_SETTER(uint32_t,color);
	ASPROPERTY_GETTER_SETTER(uint32_t,componentX);
	ASPROPERTY_GETTER_SETTER(uint32_t,componentY);
	ASPROPERTY_GETTER_SETTER(_NR<BitmapData>,mapBitmap);
	ASPROPERTY_GETTER_SETTER(_NR<Point>,mapPoint);
	ASPROPERTY_GETTER_SETTER(tiny_string,mode);
	ASPROPERTY_GETTER_SETTER(number_t,scaleX);
	ASPROPERTY_GETTER_SETTER(number_t,scaleY);
};
class GradientBevelFilter: public BitmapFilter
{
private:
	BitmapFilter* cloneImpl() const override;
public:
	GradientBevelFilter(Class_base* c);
	GradientBevelFilter(Class_base* c, const GRADIENTBEVELFILTER& filter);
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(_constructor);
	ASPROPERTY_GETTER_SETTER(_NR<Array>, alphas);
	ASPROPERTY_GETTER_SETTER(number_t, angle);
	ASPROPERTY_GETTER_SETTER(number_t, blurX);
	ASPROPERTY_GETTER_SETTER(number_t, blurY);
	ASPROPERTY_GETTER_SETTER(_NR<Array>, colors);
	ASPROPERTY_GETTER_SETTER(number_t, distance);
	ASPROPERTY_GETTER_SETTER(bool,knockout);
	ASPROPERTY_GETTER_SETTER(int32_t,quality);
	ASPROPERTY_GETTER_SETTER(_NR<Array>, ratios);
	ASPROPERTY_GETTER_SETTER(number_t, strength);
	ASPROPERTY_GETTER_SETTER(tiny_string, type);
};
class ShaderFilter: public BitmapFilter
{
private:
	BitmapFilter* cloneImpl() const override;
public:
	ShaderFilter(Class_base* c);
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(_constructor);
};

class BitmapFilterQuality: public ASObject
{
public:
	BitmapFilterQuality(Class_base* c):ASObject(c) {}
	static void sinit(Class_base* c);
};

class DisplacementMapFilterMode: public ASObject
{
public:
	DisplacementMapFilterMode(Class_base* c):ASObject(c) {}
	static void sinit(Class_base* c);
};

class BitmapFilterType: public ASObject
{
public:
	BitmapFilterType(Class_base* c):ASObject(c) {}
	static void sinit(Class_base* c);
};


}

#endif /* SCRIPTING_FLASH_FILTERS_FLASHFILTERS_H */
