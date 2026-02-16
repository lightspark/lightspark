/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2010-2013  Alessandro Pignotti (a.pignotti@sssup.it)
	Copyright (C) 2026  Ludger Krämer <dbluelle@onlinehome.de>

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

#ifndef BACKENDS_COLORTRANSFORMBASE_H
#define BACKENDS_COLORTRANSFORMBASE_H 1

#include "swftypes.h"

namespace lightspark
{
class ColorTransformBase
{
public:
	number_t redMultiplier;
	number_t greenMultiplier;
	number_t blueMultiplier;
	number_t alphaMultiplier;
	number_t redOffset;
	number_t greenOffset;
	number_t blueOffset;
	number_t alphaOffset;
	ColorTransformBase():
		redMultiplier(1.0),
		greenMultiplier(1.0),
		blueMultiplier(1.0),
		alphaMultiplier(1.0),
		redOffset(0),
		greenOffset(0),
		blueOffset(0),
		alphaOffset(0)
	{}
	
	ColorTransformBase(const ColorTransformBase& r)
	{
		*this=r;
	}
	ColorTransformBase& operator=(const ColorTransformBase& r)
	{
		redMultiplier=r.redMultiplier;
		greenMultiplier=r.greenMultiplier;
		blueMultiplier=r.blueMultiplier;
		alphaMultiplier=r.alphaMultiplier;
		redOffset=r.redOffset;
		greenOffset=r.greenOffset;
		blueOffset=r.blueOffset;
		alphaOffset=r.alphaOffset;
		return *this;
	}
	bool operator==(const ColorTransformBase& r)
	{
		return redMultiplier==r.redMultiplier &&
				greenMultiplier==r.greenMultiplier &&
				blueMultiplier==r.blueMultiplier &&
				alphaMultiplier==r.alphaMultiplier &&
				redOffset==r.redOffset &&
				greenOffset==r.greenOffset &&
				blueOffset==r.blueOffset &&
				alphaOffset==r.alphaOffset;
	}
	void fillConcatenated(DisplayObject* src, bool ignoreBlendMode=false);
	void applyTransformation(uint8_t* bm, uint32_t size) const;
	uint8_t* applyTransformation(BitmapContainer* bm);
	bool isIdentity() const
	{
		return (redMultiplier==1.0 &&
				greenMultiplier==1.0 &&
				blueMultiplier==1.0 &&
				alphaMultiplier==1.0 &&
				redOffset==0 &&
				greenOffset==0 &&
				blueOffset==0 &&
				alphaOffset==0);
	}
	void resetTransformation()
	{
		redMultiplier=1.0;
		greenMultiplier=1.0;
		blueMultiplier=1.0;
		alphaMultiplier=1.0;
		redOffset=0;
		greenOffset=0;
		blueOffset=0;
		alphaOffset=0;
	}
	void setProperties(const CXFORMWITHALPHA& cx)
	{
		cx.getParameters(redMultiplier, greenMultiplier, blueMultiplier, alphaMultiplier,
						 redOffset, greenOffset, blueOffset, alphaOffset);
	}
	uint32_t getRGB() const
	{
		uint32_t ro, go, bo;
		ro = uint32_t(redOffset) & 0xff;
		go = uint32_t(greenOffset) & 0xff;
		bo = uint32_t(blueOffset) & 0xff;

		return (ro<<16) | (go<<8) | bo;
	}
	void setfromRGB(uint32_t rgb)
	{
		//Setting multiplier to 0
		redMultiplier=0;
		greenMultiplier=0;
		blueMultiplier=0;
		//Setting offset to the input value
		redOffset=(rgb>>16)&0xff;
		greenOffset=(rgb>>8)&0xff;
		blueOffset=rgb&0xff;

	}

	// returning r,g,b,a values are between 0.0 and 1.0
	void applyTransformation(const RGBA &color, float& r, float& g, float& b, float &a)
	{
		a = std::max(0.0f,std::min(255.0f,float((color.Alpha * alphaMultiplier * 255.0f)/255.0f + alphaOffset)))/255.0f;
		r = std::max(0.0f,std::min(255.0f,float((color.Red   *   redMultiplier * 255.0f)/255.0f +   redOffset)))/255.0f;
		g = std::max(0.0f,std::min(255.0f,float((color.Green * greenMultiplier * 255.0f)/255.0f + greenOffset)))/255.0f;
		b = std::max(0.0f,std::min(255.0f,float((color.Blue  *  blueMultiplier * 255.0f)/255.0f +  blueOffset)))/255.0f;
	}
	ColorTransformBase multiplyTransform(const ColorTransformBase& r);
};

}
#endif /* BACKENDS_COLORTRANSFORMBASE_H */
