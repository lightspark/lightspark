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
enum FILTERSTEPS { FILTERSTEP_BLUR_HORIZONTAL=1,FILTERSTEP_BLUR_VERTICAL=2, FILTERSTEP_DROPSHADOW=3, FILTERSTEP_GRADIENT_GLOW=4, FILTERSTEP_BEVEL=5, FILTERSTEP_COLORMATRIX=6, FILTERSTEP_CONVOLUTION=7 };
class BitmapFilter: public ASObject
{
private:
	virtual BitmapFilter* cloneImpl() const;
protected:
	void applyBlur(uint8_t* data, uint32_t width, uint32_t height, number_t blurx, number_t blury, int quality);
	static void applyDropShadowFilter(uint8_t* data, uint32_t datawidth, uint32_t dataheight, uint8_t* tmpdata, const RECT& sourceRect, number_t xpos, number_t ypos, number_t strength, number_t alpha, uint32_t color, bool inner, bool knockout, number_t scalex, number_t scaley);
	static void fillGradientColors(number_t* gradientalphas, uint32_t* gradientcolors, Array* ratios, Array* alphas, Array* colors);
	static void applyGradientFilter(uint8_t* data, uint32_t datawidth, uint32_t dataheight, uint8_t* tmpdata, const RECT& sourceRect, number_t xpos, number_t ypos, number_t strength, number_t* alphas, uint32_t* colors, bool inner, bool knockout, number_t scalex, number_t scaley);
	static void applyBevelFilter(BitmapContainer* target, const RECT& sourceRect, uint8_t* tmpdata, DisplayObject* owner, number_t distance, number_t strength, bool inner, bool knockout, uint32_t* gradientcolors, number_t* gradientalphas, number_t angle, number_t xpos, number_t ypos, number_t scalex, number_t scaley);
	bool getRenderFilterArgsBlur(float* args, float blurX, float blurY, uint32_t step, uint32_t quality, uint32_t& nextstep) const;
	void getRenderFilterArgsBlurHorizontal(float* args, float blurX) const;
	void getRenderFilterArgsBlurVertical(float* args, float blurY) const;
	void getRenderFilterArgsDropShadow(float* args, bool inner, bool knockout, float strength, uint32_t color, float alpha, float xpos, float ypos) const;
	void getRenderFilterArgsBevel(float* args, bool inner, bool knockout, float strength, float distance, float angle) const;
public:
	BitmapFilter(ASWorker* wrk,Class_base* c, CLASS_SUBTYPE st=SUBTYPE_BITMAPFILTER):ASObject(wrk,c,T_OBJECT,st){}
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(clone);
	virtual void applyFilter(BitmapContainer* target, BitmapContainer* source, const RECT& sourceRect, number_t xpos, number_t ypos, number_t scalex, number_t scaley, DisplayObject* owner=nullptr);
	virtual number_t getMaxFilterBorder() const { return 0; }
	virtual bool compareFILTER(const FILTER& filter) const { return false; }
	// last step is signaled by returning 0 in args[0]
	virtual void getRenderFilterArgs(uint32_t step, float* args) const;
	// gradientcolors is array of 256*4 floats (RGBA values)
	virtual void getRenderFilterGradientColors(float* gradientcolors) const;
};

class ShaderFilter: public BitmapFilter
{
private:
	BitmapFilter* cloneImpl() const override;
public:
	ShaderFilter(ASWorker* wrk,Class_base* c);
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(_constructor);
	void applyFilter(BitmapContainer* target, BitmapContainer* source, const RECT& sourceRect, number_t xpos, number_t ypos, number_t scalex, number_t scaley, DisplayObject* owner=nullptr) override;
	bool compareFILTER(const FILTER& filter) const override { return false; }
	void getRenderFilterArgs(uint32_t step, float* args) const override;
};

class BitmapFilterQuality: public ASObject
{
public:
	BitmapFilterQuality(ASWorker* wrk,Class_base* c):ASObject(wrk,c) {}
	static void sinit(Class_base* c);
};

class DisplacementMapFilterMode: public ASObject
{
public:
	DisplacementMapFilterMode(ASWorker* wrk,Class_base* c):ASObject(wrk,c) {}
	static void sinit(Class_base* c);
};

class BitmapFilterType: public ASObject
{
public:
	BitmapFilterType(ASWorker* wrk,Class_base* c):ASObject(wrk,c) {}
	static void sinit(Class_base* c);
};


}

#endif /* SCRIPTING_FLASH_FILTERS_FLASHFILTERS_H */
