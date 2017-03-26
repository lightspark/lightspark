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
	BitmapFilter(Class_base* c):ASObject(c){}
	static void sinit(Class_base* c);
//	static void buildTraits(ASObject* o);
	ASFUNCTION(clone);
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
	virtual BitmapFilter* cloneImpl() const;
public:
	GlowFilter(Class_base* c);
	static void sinit(Class_base* c);
//	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
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
	virtual BitmapFilter* cloneImpl() const;
public:
	DropShadowFilter(Class_base* c);
	static void sinit(Class_base* c);
//	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
};

class GradientGlowFilter: public BitmapFilter
{
private:
	virtual BitmapFilter* cloneImpl() const;
public:
	GradientGlowFilter(Class_base* c);
	static void sinit(Class_base* c);
	ASFUNCTION(_constructor);
};

class BevelFilter: public BitmapFilter
{
private:
	virtual BitmapFilter* cloneImpl() const;
public:
	BevelFilter(Class_base* c);
	static void sinit(Class_base* c);
	ASFUNCTION(_constructor);
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
	virtual BitmapFilter* cloneImpl() const;
public:
	ColorMatrixFilter(Class_base* c);
	static void sinit(Class_base* c);
	ASFUNCTION(_constructor);
	ASPROPERTY_GETTER_SETTER(_NR<Array>, matrix);
};
class BlurFilter: public BitmapFilter
{
private:
	virtual BitmapFilter* cloneImpl() const;
public:
	BlurFilter(Class_base* c);
	static void sinit(Class_base* c);
	ASFUNCTION(_constructor);
	ASPROPERTY_GETTER_SETTER(number_t, blurX);
	ASPROPERTY_GETTER_SETTER(number_t, blurY);
	ASPROPERTY_GETTER_SETTER(int, quality);
};
class ConvolutionFilter: public BitmapFilter
{
private:
	virtual BitmapFilter* cloneImpl() const;
public:
	ConvolutionFilter(Class_base* c);
	static void sinit(Class_base* c);
	ASFUNCTION(_constructor);
};
class DisplacementMapFilter: public BitmapFilter
{
private:
	virtual BitmapFilter* cloneImpl() const;
public:
	DisplacementMapFilter(Class_base* c);
	static void sinit(Class_base* c);
	ASFUNCTION(_constructor);
};
class GradientBevelFilter: public BitmapFilter
{
private:
	virtual BitmapFilter* cloneImpl() const;
public:
	GradientBevelFilter(Class_base* c);
	static void sinit(Class_base* c);
	ASFUNCTION(_constructor);
};
class ShaderFilter: public BitmapFilter
{
private:
	virtual BitmapFilter* cloneImpl() const;
public:
	ShaderFilter(Class_base* c);
	static void sinit(Class_base* c);
	ASFUNCTION(_constructor);
};

class BitmapFilterQuality: public ASObject
{
public:
	BitmapFilterQuality(Class_base* c):ASObject(c) {}
	static void sinit(Class_base* c);
};

};

#endif /* SCRIPTING_FLASH_FILTERS_FLASHFILTERS_H */
