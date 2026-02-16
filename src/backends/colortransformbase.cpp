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

#include <cassert>

#include "backends/colortransformbase.h"
#include "scripting/flash/display/flashdisplay.h"
#include "scripting/flash/display/BitmapContainer.h"

using namespace lightspark;

void ColorTransformBase::fillConcatenated(DisplayObject* src, bool ignoreBlendMode)
{
	ColorTransformBase ct = src->colorTransform;
	DisplayObjectContainer* p = src->getParent();
	*this=ct;
	while (p)
	{
		ct = p->colorTransform;
		redMultiplier*=ct.redMultiplier;
		greenMultiplier*=ct.greenMultiplier;
		blueMultiplier*=ct.blueMultiplier;
		alphaMultiplier*=ct.alphaMultiplier;
		redOffset+=ct.redOffset;
		greenOffset+=ct.greenOffset;
		blueOffset+=ct.blueOffset;
		alphaOffset+=ct.alphaOffset;
		if (!ignoreBlendMode && p->isShaderBlendMode(p->getBlendMode()))
			break;
		p = p->getParent();
	}
}
void ColorTransformBase::applyTransformation(uint8_t* bm, uint32_t size) const
{
	if (isIdentity())
		return;

	for (uint32_t i = 0; i < size; i+=4)
	{
		bm[i+3] = max(0,min(255,int(((number_t(bm[i+3]) * alphaMultiplier) + alphaOffset))));
		bm[i+2] = max(0,min(255,int(((number_t(bm[i+2]) *  blueMultiplier) +  blueOffset)*(number_t(bm[i+3])/255.0))));
		bm[i+1] = max(0,min(255,int(((number_t(bm[i+1]) * greenMultiplier) + greenOffset)*(number_t(bm[i+3])/255.0))));
		bm[i  ] = max(0,min(255,int(((number_t(bm[i  ]) *   redMultiplier) +   redOffset)*(number_t(bm[i+3])/255.0))));
	}
}
uint8_t *ColorTransformBase::applyTransformation(BitmapContainer* bm)
{
	if (redMultiplier==1.0 &&
		greenMultiplier==1.0 &&
		blueMultiplier==1.0 &&
		alphaMultiplier==1.0 &&
		redOffset==0.0 &&
		greenOffset==0.0 &&
		blueOffset==0.0 &&
		alphaOffset==0.0)
		return (uint8_t*)bm->getData();

	const uint8_t* src = bm->getOriginalData();
	uint8_t* dst = bm->getDataColorTransformed();
	uint32_t size = bm->getWidth()*bm->getHeight()*4;
	for (uint32_t i = 0; i < size; i+=4)
	{
		dst[i+3] = max(0,min(255,int(((number_t(src[i+3]) * alphaMultiplier) + alphaOffset))));
		dst[i+2] = max(0,min(255,int(((number_t(src[i+2]) *  blueMultiplier) +  blueOffset)*(number_t(dst[i+3])/255.0))));
		dst[i+1] = max(0,min(255,int(((number_t(src[i+1]) * greenMultiplier) + greenOffset)*(number_t(dst[i+3])/255.0))));
		dst[i  ] = max(0,min(255,int(((number_t(src[i  ]) *   redMultiplier) +   redOffset)*(number_t(dst[i+3])/255.0))));
	}
	return (uint8_t*)bm->getDataColorTransformed();
}

ColorTransformBase ColorTransformBase::multiplyTransform(const ColorTransformBase& r)
{
	ColorTransformBase ret;
	ret.redMultiplier = redMultiplier * r.redMultiplier;
	ret.greenMultiplier = greenMultiplier * r.greenMultiplier;
	ret.blueMultiplier = blueMultiplier * r.blueMultiplier;
	ret.alphaMultiplier = alphaMultiplier * r.alphaMultiplier;

	ret.redOffset = redOffset + int16_t(floor(redMultiplier * r.redOffset));
	ret.greenOffset = greenOffset + int16_t(floor(greenMultiplier * r.greenOffset));
	ret.blueOffset = blueOffset + int16_t(floor(blueMultiplier * r.blueOffset));
	ret.alphaOffset = alphaOffset + int16_t(floor(alphaMultiplier * r.alphaOffset));
	return ret;
}
