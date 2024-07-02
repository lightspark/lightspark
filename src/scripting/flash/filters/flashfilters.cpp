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
#include "scripting/flash/filters/BevelFilter.h"
#include "scripting/flash/filters/BlurFilter.h"
#include "scripting/flash/filters/ColorMatrixFilter.h"
#include "scripting/flash/filters/ConvolutionFilter.h"
#include "scripting/flash/filters/DisplacementMapFilter.h"
#include "scripting/flash/filters/DropShadowFilter.h"
#include "scripting/flash/filters/GlowFilter.h"
#include "scripting/flash/filters/GradientBevelFilter.h"
#include "scripting/flash/filters/GradientGlowFilter.h"
#include "scripting/flash/display/flashdisplay.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include "scripting/flash/display/BitmapData.h"
#include "scripting/toplevel/Array.h"
#include "backends/rendering.h"

using namespace std;
using namespace lightspark;

void BitmapFilter::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructorNotInstantiatable, CLASS_SEALED);
	c->setDeclaredMethodByQName("clone","",c->getSystemState()->getBuiltinFunction(clone),NORMAL_METHOD,true);
}

void BitmapFilter::applyFilter(BitmapContainer* target, BitmapContainer* source, const RECT& sourceRect, number_t xpos, number_t ypos, number_t scalex, number_t scaley, DisplayObject* owner)
{
	LOG(LOG_ERROR,"applyFilter for "<<this->toDebugString());
}

void BitmapFilter::getRenderFilterArgs(uint32_t step, float* args) const 
{
	args[0]=0; 
	//Not supposed to be called
	throw RunTimeException("BitmapFilter::getRenderFilterArgs");
}

void BitmapFilter::getRenderFilterGradientColors(float* gradientcolors) const
{
}

BitmapFilter* BitmapFilter::cloneImpl() const
{
	return Class<BitmapFilter>::getInstanceS(getInstanceWorker());
}

// blur algorithm taken from haxe https://github.com/haxelime/lime/blob/develop/src/lime/_internal/graphics/StackBlur.hx
int MUL_TABLE[] =
{
	1, 171, 205, 293, 57, 373, 79, 137, 241, 27, 391, 357, 41, 19, 283, 265, 497, 469, 443, 421, 25, 191, 365, 349, 335, 161, 155, 149, 9, 278, 269, 261,
	505, 245, 475, 231, 449, 437, 213, 415, 405, 395, 193, 377, 369, 361, 353, 345, 169, 331, 325, 319, 313, 307, 301, 37, 145, 285, 281, 69, 271, 267,
	263, 259, 509, 501, 493, 243, 479, 118, 465, 459, 113, 446, 55, 435, 429, 423, 209, 413, 51, 403, 199, 393, 97, 3, 379, 375, 371, 367, 363, 359, 355,
	351, 347, 43, 85, 337, 333, 165, 327, 323, 5, 317, 157, 311, 77, 305, 303, 75, 297, 294, 73, 289, 287, 71, 141, 279, 277, 275, 68, 135, 67, 133, 33,
	262, 260, 129, 511, 507, 503, 499, 495, 491, 61, 121, 481, 477, 237, 235, 467, 232, 115, 457, 227, 451, 7, 445, 221, 439, 218, 433, 215, 427, 425,
	211, 419, 417, 207, 411, 409, 203, 202, 401, 399, 396, 197, 49, 389, 387, 385, 383, 95, 189, 47, 187, 93, 185, 23, 183, 91, 181, 45, 179, 89, 177, 11,
	175, 87, 173, 345, 343, 341, 339, 337, 21, 167, 83, 331, 329, 327, 163, 81, 323, 321, 319, 159, 79, 315, 313, 39, 155, 309, 307, 153, 305, 303, 151,
	75, 299, 149, 37, 295, 147, 73, 291, 145, 289, 287, 143, 285, 71, 141, 281, 35, 279, 139, 69, 275, 137, 273, 17, 271, 135, 269, 267, 133, 265, 33,
	263, 131, 261, 130, 259, 129, 257, 1
};
int SHG_TABLE[] =
{
	0, 9, 10, 11, 9, 12, 10, 11, 12, 9, 13, 13, 10, 9, 13, 13, 14, 14, 14, 14, 10, 13, 14, 14, 14, 13, 13, 13, 9, 14, 14, 14, 15, 14, 15, 14, 15, 15, 14,
	15, 15, 15, 14, 15, 15, 15, 15, 15, 14, 15, 15, 15, 15, 15, 15, 12, 14, 15, 15, 13, 15, 15, 15, 15, 16, 16, 16, 15, 16, 14, 16, 16, 14, 16, 13, 16,
	16, 16, 15, 16, 13, 16, 15, 16, 14, 9, 16, 16, 16, 16, 16, 16, 16, 16, 16, 13, 14, 16, 16, 15, 16, 16, 10, 16, 15, 16, 14, 16, 16, 14, 16, 16, 14, 16,
	16, 14, 15, 16, 16, 16, 14, 15, 14, 15, 13, 16, 16, 15, 17, 17, 17, 17, 17, 17, 14, 15, 17, 17, 16, 16, 17, 16, 15, 17, 16, 17, 11, 17, 16, 17, 16,
	17, 16, 17, 17, 16, 17, 17, 16, 17, 17, 16, 16, 17, 17, 17, 16, 14, 17, 17, 17, 17, 15, 16, 14, 16, 15, 16, 13, 16, 15, 16, 14, 16, 15, 16, 12, 16,
	15, 16, 17, 17, 17, 17, 17, 13, 16, 15, 17, 17, 17, 16, 15, 17, 17, 17, 16, 15, 17, 17, 14, 16, 17, 17, 16, 17, 17, 16, 15, 17, 16, 14, 17, 16, 15,
	17, 16, 17, 17, 16, 17, 15, 16, 17, 14, 17, 16, 15, 17, 16, 17, 13, 17, 16, 17, 17, 16, 17, 14, 17, 16, 17, 16, 17, 16, 17, 9
};

void BitmapFilter::applyBlur(uint8_t* data, uint32_t width, uint32_t height, number_t blurx, number_t blury, int quality)
{
	int oX;
	int oY;
	float sX;
	float sY;
	getSystemState()->stageCoordinateMapping(getSystemState()->getRenderThread()->windowWidth, getSystemState()->getRenderThread()->windowHeight, oX, oY, sX, sY);

	blurx*=sX;
	blury*=sY;
	int radiusX = int(round(blurx)) >> 1;
	int radiusY = int(round(blury)) >> 1;
	if (radiusX >= int(sizeof(MUL_TABLE)/sizeof(int)))
		radiusX = sizeof(MUL_TABLE)/sizeof(int)-1;
	if (radiusY >= int(sizeof(MUL_TABLE)/sizeof(int)))
		radiusY = sizeof(MUL_TABLE)/sizeof(int)-1;
	if (radiusX<=0 || radiusY <= 0)
		return;

	int iterations = quality;

	uint8_t* px = data;
	int x, y, i, p, yp, yi, yw;
	int r, g, b, a, pr, pg, pb, pa;

	int divx = (radiusX + radiusX + 1);
	int divy = (radiusY + radiusY + 1);
	int w = width;
	int h = height;

	int w1 = w - 1;
	int h1 = h - 1;
	int rxp1 = radiusX + 1;
	int ryp1 = radiusY + 1;

	std::vector<RGBA> ssx;
	ssx.resize(divx);
	std::vector<RGBA> ssy;
	ssy.resize(divy);

	int mtx = MUL_TABLE[radiusX];
	int stx = SHG_TABLE[radiusX];
	int mty = MUL_TABLE[radiusY];
	int sty = SHG_TABLE[radiusY];

	while (iterations > 0)
	{
		iterations--;
		yw = yi = 0;
		int ms = mtx;
		int ss = stx;
		y = h;
		do
		{
			r = rxp1 * (pr = px[yi]);
			g = rxp1 * (pg = px[yi + 1]);
			b = rxp1 * (pb = px[yi + 2]);
			a = rxp1 * (pa = px[yi + 3]);
			auto sx = ssx.begin();
			i = rxp1;
			do
			{
				(*sx).Red = pr;
				(*sx).Green = pg;
				(*sx).Blue = pb;
				(*sx).Alpha = pa;
				sx++;
				if (sx == ssx.end())
					sx = ssx.begin();
			}
			while (--i > -1);
			
			for (i = 1; i < rxp1; i++)
			{
				p = yi + ((w1 < i ? w1 : i) << 2);
				r += ((*sx).Red = px[p]);
				g += ((*sx).Green = px[p + 1]);
				b += ((*sx).Blue = px[p + 2]);
				a += ((*sx).Alpha = px[p + 3]);
				sx++;
				if (sx == ssx.end())
					sx = ssx.begin();
			}
			
			auto si = ssx.begin();
			for (x = 0; x < w; x++)
			{
				px[yi++] = uint32_t(r * ms) >> ss;
				px[yi++] = uint32_t(g * ms) >> ss;
				px[yi++] = uint32_t(b * ms) >> ss;
				px[yi++] = uint32_t(a * ms) >> ss;
				p = x + radiusX + 1;
				p = (yw + (p < w1 ? p : w1)) << 2;
				r -= (*si).Red;
				r += ((*si).Red = px[p]);
				g -= (*si).Green;
				g += ((*si).Green = px[p + 1]);
				b -= (*si).Blue;
				b += ((*si).Blue = px[p + 2]);
				a -= (*si).Alpha;
				a += ((*si).Alpha = px[p + 3]);
				si++;
				if (si == ssx.end())
					si = ssx.begin();
			}
			yw += w;
		}
		while (--y > 0);
		
		ms = mty;
		ss = sty;
		for (x = 0; x < w; x++)
		{
			yi = x << 2;
			r = ryp1 * (pr = px[yi]);
			g = ryp1 * (pg = px[yi + 1]);
			b = ryp1 * (pb = px[yi + 2]);
			a = ryp1 * (pa = px[yi + 3]);
			auto sy = ssy.begin();
			for (i = 0; i< ryp1; i++)
			{
				(*sy).Red = pr;
				(*sy).Green = pg;
				(*sy).Blue = pb;
				(*sy).Alpha = pa;
				sy++;
				if (sy == ssy.end())
					sy = ssy.begin();
			}
			yp = w;
			for (i = 1; i < (radiusY + 1); i++)
			{
				yi = (yp + x) << 2;
				r += ((*sy).Red = px[yi]);
				g += ((*sy).Green = px[yi + 1]);
				b += ((*sy).Blue = px[yi + 2]);
				a += ((*sy).Alpha = px[yi + 3]);
				sy++;
				if (sy == ssy.end())
					sy = ssy.begin();
				if (i < h1)
				{
					yp += w;
				}
			}
			yi = x;
			auto si = ssy.begin();
			
			if (iterations > 0)
			{
				for (y = 0; y<h; y++)
				{
					p = yi << 2;
					pa = uint32_t(a * ms) >> ss;
					px[p + 3] = pa;
					if (pa > 0)
					{
						px[p] = (uint32_t(r * ms) >> ss);
						px[p + 1] = (uint32_t(g * ms) >> ss);
						px[p + 2] = (uint32_t(b * ms) >> ss);
					}
					else
					{
						px[p] = px[p + 1] = px[p + 2] = 0;
					}
					p = y + ryp1;
					p = (x + ((p < h1 ? p : h1) * w)) << 2;
					r -= (*si).Red;
					r += ((*si).Red = px[p]);
					g -= (*si).Green;
					g += ((*si).Green = px[p + 1]);
					b -= (*si).Blue;
					b += ((*si).Blue = px[p + 2]);
					a -= (*si).Alpha;
					a += ((*si).Alpha = px[p + 3]);
					si++;
					if (si == ssy.end())
						si = ssy.begin();
					yi += w;
				}
			}
			else
			{
				for (y = 0; y < h; y++)
				{
					p = yi << 2;
					px[p + 3] = pa = uint32_t(a * ms) >> ss;
					if (pa > 0)
					{
						pr = (uint32_t(r * ms) >> ss);
						pg = (uint32_t(g * ms) >> ss);
						pb = (uint32_t(b * ms) >> ss);
						px[p] = pr > 255 ? 255 : pr;
						px[p + 1] = pg > 255 ? 255 : pg;
						px[p + 2] = pb > 255 ? 255 : pb;
					}
					else
					{
						px[p] = px[p + 1] = px[p + 2] = 0;
					}
					p = y + ryp1;
					p = (x + ((p < h1 ? p : h1) * w)) << 2;
					r -= (*si).Red - ((*si).Red = px[p]);
					g -= (*si).Green - ((*si).Green = px[p + 1]);
					b -= (*si).Blue - ((*si).Blue = px[p + 2]);
					a -= (*si).Alpha - ((*si).Alpha = px[p + 3]);
					si++;
					if (si == ssy.end())
						si = ssy.begin();
					yi += w;
				}
			}
		}
	}
}

uint32_t dropShadowPixel(uint32_t dstpixel, uint8_t tmpalpha, number_t strength, number_t alpha, uint32_t color, bool inner, bool knockout)
{
	uint32_t ret;
	uint8_t* ptr = (uint8_t*)&ret;
	uint8_t* dstptr = (uint8_t*)&dstpixel;
	number_t glowalpha = number_t(inner ? 0xff - tmpalpha : tmpalpha)/255.0;
	number_t srcalpha = max(0.0,min(1.0,glowalpha*alpha*strength));
	number_t dstalpha = number_t(dstptr[3])/255.0;
	if (inner)
	{
		if (knockout)
		{
			ptr[0] = min(uint32_t(0xff),uint32_t(number_t((color    )&0xff)*srcalpha*dstalpha));
			ptr[1] = min(uint32_t(0xff),uint32_t(number_t((color>> 8)&0xff)*srcalpha*dstalpha));
			ptr[2] = min(uint32_t(0xff),uint32_t(number_t((color>>16)&0xff)*srcalpha*dstalpha));
			ptr[3] = min(uint32_t(0xff),uint32_t(number_t(0xff)*srcalpha*dstalpha));
		}
		else
		{
			ptr[0] = min(uint32_t(0xff),uint32_t(number_t((color    )&0xff)*srcalpha*dstalpha+number_t(dstptr[0])*(1.0-srcalpha)));
			ptr[1] = min(uint32_t(0xff),uint32_t(number_t((color>> 8)&0xff)*srcalpha*dstalpha+number_t(dstptr[1])*(1.0-srcalpha)));
			ptr[2] = min(uint32_t(0xff),uint32_t(number_t((color>>16)&0xff)*srcalpha*dstalpha+number_t(dstptr[2])*(1.0-srcalpha)));
			ptr[3] = min(uint32_t(0xff),uint32_t(number_t(0xff)*srcalpha*dstalpha+number_t(dstptr[3])*(1.0-srcalpha)));
		}
	}
	else
	{
		if (knockout)
		{
			ptr[0] = min(uint32_t(0xff),uint32_t(number_t((color    )&0xff)*srcalpha*(1.0-dstalpha)));
			ptr[1] = min(uint32_t(0xff),uint32_t(number_t((color>> 8)&0xff)*srcalpha*(1.0-dstalpha)));
			ptr[2] = min(uint32_t(0xff),uint32_t(number_t((color>>16)&0xff)*srcalpha*(1.0-dstalpha)));
			ptr[3] = min(uint32_t(0xff),uint32_t(number_t(0xff)*srcalpha*(1.0-dstalpha)));
		}
		else
		{
			ptr[0] = min(uint32_t(0xff),uint32_t(number_t((color    )&0xff)*srcalpha*(1.0-dstalpha)+number_t(dstptr[0])));
			ptr[1] = min(uint32_t(0xff),uint32_t(number_t((color>> 8)&0xff)*srcalpha*(1.0-dstalpha)+number_t(dstptr[1])));
			ptr[2] = min(uint32_t(0xff),uint32_t(number_t((color>>16)&0xff)*srcalpha*(1.0-dstalpha)+number_t(dstptr[2])));
			ptr[3] = min(uint32_t(0xff),uint32_t(number_t(0xff)*srcalpha*(1.0-dstalpha)+number_t(dstptr[3])));
		}
	}
	return ret;
}

void BitmapFilter::applyDropShadowFilter(uint8_t* data, uint32_t datawidth, uint32_t dataheight, uint8_t* tmpdata, const RECT& sourceRect, number_t xpos, number_t ypos, number_t strength, number_t alpha, uint32_t color, bool inner, bool knockout,number_t scalex,number_t scaley)
{
	xpos *= scalex;
	ypos *= scaley;
	uint32_t* datapixel = (uint32_t*)data;
	uint32_t width = sourceRect.Xmax-sourceRect.Xmin;
	uint32_t height = sourceRect.Ymax-sourceRect.Ymin;
	uint32_t size = width*height;
	int32_t startpos = round(ypos)*datawidth+round(xpos);
	int32_t targetsize = datawidth*dataheight;
	for (uint32_t i = 0; i < size; i++)
	{
		if (i && i%width==0)
			startpos+=datawidth;
		if (startpos < 0)
			continue;
		int32_t targetpos = startpos+(i%width);
		if (targetpos < 0)
			continue;
		if (targetpos >= targetsize)
			break;
		datapixel[targetpos] = dropShadowPixel(datapixel[targetpos], tmpdata[i*4+3], strength, alpha, color, inner, knockout);
	}
}
void BitmapFilter::fillGradientColors(number_t* gradientalphas, uint32_t* gradientcolors, Array* ratios, Array* alphas, Array* colors)
{
	number_t alpha = 1.0;
	number_t red = 255.0;
	number_t green = 255.0;
	number_t blue = 255.0;
	number_t lastalpha = 0.0;
	number_t lastred = 0.0;
	number_t lastgreen = 0.0;
	number_t lastblue = 0.0;
	number_t nextalpha = 0.0;
	number_t nextred = 0.0;
	number_t nextgreen = 0.0;
	number_t nextblue = 0.0;
	number_t deltaAlpha=0.0;
	number_t deltaRed=0;
	number_t deltaGreen=0;
	number_t deltaBlue=0;
	uint32_t lastratio = 0;
	uint32_t nextratio = 0;
	uint32_t ratioidx = 0;
	number_t ratiostep=1;
	// get initial entries for ratio 0, if available
	if (ratios && ratios->size())
	{
		asAtom a=asAtomHandler::invalidAtom;
		ratios->at_nocheck(a,ratioidx);
		if (asAtomHandler::toUInt(a)==0)
		{
			if (alphas && alphas->size() > ratioidx)
			{
				asAtom a=asAtomHandler::invalidAtom;
				alphas->at_nocheck(a,ratioidx);
				nextalpha = asAtomHandler::toNumber(a);
			}
			if (colors && colors->size() > ratioidx)
			{
				asAtom a=asAtomHandler::invalidAtom;
				colors->at_nocheck(a,ratioidx);
				uint32_t c = asAtomHandler::toUInt(a);
				nextred = (c>>16)&0xff;
				nextgreen = (c>>8)&0xff ;
				nextblue = c&0xff;
			}
		}
	}
	for (uint32_t i=0; i <256; i++)
	{
		if (i == nextratio)
		{
			lastratio = nextratio;
			ratioidx++;
			if (ratios && ratios->size() > ratioidx)
			{
				asAtom a=asAtomHandler::invalidAtom;
				ratios->at_nocheck(a,ratioidx);
				nextratio = asAtomHandler::toUInt(a);
			}
			lastalpha = alpha = nextalpha;
			ratiostep = nextratio==lastratio ? 1 : nextratio-lastratio;
			if (alphas && alphas->size() > ratioidx)
			{
				asAtom a=asAtomHandler::invalidAtom;
				alphas->at_nocheck(a,ratioidx);
				nextalpha = asAtomHandler::toNumber(a);
			}
			deltaAlpha = (nextalpha-lastalpha)/ratiostep;
			
			lastred = red = nextred;
			lastgreen = green = nextgreen;
			lastblue = blue = nextblue;
			if (colors && colors->size() > ratioidx)
			{
				asAtom a=asAtomHandler::invalidAtom;
				colors->at_nocheck(a,ratioidx);
				uint32_t c = asAtomHandler::toUInt(a);
				nextred = (c>>16)&0xff;
				nextgreen = (c>>8)&0xff ;
				nextblue = c&0xff;
			}
			deltaRed = (nextred-lastred)/ratiostep;
			deltaGreen = (nextgreen-lastgreen)/ratiostep;
			deltaBlue = (nextblue-lastblue)/ratiostep;
		}
		gradientalphas[i] = alpha;
		gradientcolors[i] = (uint32_t(red)&0xff)<<16|(uint32_t(green)&0xff)<<8|(uint32_t(blue)&0xff);
		
		alpha += deltaAlpha;
		red += deltaRed;
		green += deltaGreen;
		blue += deltaBlue;
	}
}
void BitmapFilter::applyBevelFilter(BitmapContainer* target, const RECT& sourceRect,uint8_t* tmpdata, DisplayObject* owner, number_t distance, number_t strength, bool inner, bool knockout, uint32_t* gradientcolors, number_t* gradientalphas, number_t angle, number_t xpos, number_t ypos, number_t scalex, number_t scaley)
{
	// HACK: this is _very_ strange but the rotation of the parent DisplayObject seems to have an influence on the angle used for the dropshadows
	// we just subtract the current rotation of the parent of the DisplayObject from the angle, that seems to get mostly correct results
	number_t realangle = angle;
	if (owner && owner->getParent())
	{
		MATRIX m = owner->getParent()->getMatrix();
		if (m.getScaleX()<0 || m.getScaleY()<0)
			realangle -= (m.getRotation()+180)*M_PI/180.0;
		else
			realangle -= m.getRotation()*M_PI/180.0;
	}
	Vector2f highlightOffset(xpos+cos(realangle+(inner? 0.0:M_PI)) * distance, ypos+sin(realangle+(inner? 0.0:M_PI)) * distance);
	Vector2f shadowOffset(xpos+cos(realangle+(inner?M_PI: 0.0)) * distance, ypos+sin(realangle+(inner?M_PI: 0.0)) * distance);
	int32_t shadowstartpos = round(shadowOffset.y*scaley)*target->getWidth()+round(shadowOffset.x*scalex);
	int32_t highlightstartpos = round(highlightOffset.y*scaley)*target->getWidth()+round(highlightOffset.x*scalex);
	uint8_t combinedpixel[4];

	auto applyPixel = [&](int i, int32_t& startpos) -> uint32_t
	{
		uint32_t* datapixel = (uint32_t*)target->getData();
		const int32_t size = target->getDataSize()/4;
		const int bluroffset = i-startpos;
		if (i < size)
		{
			uint32_t pixel = inner ? datapixel[i] : 0;
			return i >= startpos && bluroffset < size ? dropShadowPixel(pixel, tmpdata[bluroffset*4+3], 1.0, 1.0, 0xffffff, inner, true) : pixel;
		}
		return 0;
	};
	for (int i = 0; i < target->getWidth()*target->getHeight(); i++)
	{
		int alphahigh = (int) (number_t(applyPixel(i, highlightstartpos) >> 24) * strength);
		int alphashadow = (int) (number_t(applyPixel(i, shadowstartpos) >> 24) * strength);
		int gradientindex = max(-128,min(127,(alphahigh - alphashadow)/2));
		combinedpixel[0] = (gradientcolors[128 + gradientindex]    )&0xff;
		combinedpixel[1] = (gradientcolors[128 + gradientindex]>>8 )&0xff;
		combinedpixel[2] = (gradientcolors[128 + gradientindex]>>16)&0xff;
		combinedpixel[3] = min(0xffU,uint32_t(gradientalphas[128 + gradientindex]*255.0));
		if (inner)
		{
			// premultiply alpha
			combinedpixel[0] = uint32_t(combinedpixel[0])*uint32_t(combinedpixel[3])/255;
			combinedpixel[1] = uint32_t(combinedpixel[1])*uint32_t(combinedpixel[3])/255;
			combinedpixel[2] = uint32_t(combinedpixel[2])*uint32_t(combinedpixel[3])/255;
			if (knockout)
			{
				target->getData()[i*4  ] = combinedpixel[0];
				target->getData()[i*4+1] = combinedpixel[1];
				target->getData()[i*4+2] = combinedpixel[2];
				target->getData()[i*4+3] = uint32_t(combinedpixel[3])*uint32_t(target->getData()[i*4+3])/255;
			}
			else
			{
				target->getData()[i*4  ] = min(0xffU,uint32_t(number_t(target->getData()[i*4  ])*number_t(1.0-gradientalphas[128 + gradientindex])+combinedpixel[0]));
				target->getData()[i*4+1] = min(0xffU,uint32_t(number_t(target->getData()[i*4+1])*number_t(1.0-gradientalphas[128 + gradientindex])+combinedpixel[1]));
				target->getData()[i*4+2] = min(0xffU,uint32_t(number_t(target->getData()[i*4+2])*number_t(1.0-gradientalphas[128 + gradientindex])+combinedpixel[2]));
			}
		}
		else
		{
			if (knockout)
			{
				target->getData()[i*4  ] = combinedpixel[0];
				target->getData()[i*4+1] = combinedpixel[1];
				target->getData()[i*4+2] = combinedpixel[2];
				target->getData()[i*4+3] = min(0xffU,uint32_t(number_t(combinedpixel[3])*number_t(255.0-target->getData()[i*4+3])/255.0));
			}
			else
			{
				target->getData()[i*4  ] = min(0xffU,uint32_t(number_t(combinedpixel[0])*number_t(255.0-target->getData()[i*4+3])/255.0+target->getData()[i*4  ]));
				target->getData()[i*4+1] = min(0xffU,uint32_t(number_t(combinedpixel[1])*number_t(255.0-target->getData()[i*4+3])/255.0+target->getData()[i*4+1]));
				target->getData()[i*4+2] = min(0xffU,uint32_t(number_t(combinedpixel[2])*number_t(255.0-target->getData()[i*4+3])/255.0+target->getData()[i*4+2]));
				target->getData()[i*4+3] = min(0xffU,uint32_t(number_t(combinedpixel[3])*number_t(255.0-target->getData()[i*4+3])/255.0+target->getData()[i*4+3]));
			}
		}
	}
}

void BitmapFilter::applyGradientFilter(uint8_t* data, uint32_t datawidth, uint32_t dataheight, uint8_t* tmpdata, const RECT& sourceRect, number_t xpos, number_t ypos, number_t strength, number_t* alphas, uint32_t* colors, bool inner, bool knockout,number_t scalex,number_t scaley)
{
	
	xpos *= scalex;
	ypos *= scaley;
	uint32_t width = sourceRect.Xmax-sourceRect.Xmin;
	uint32_t height = sourceRect.Ymax-sourceRect.Ymin;
	uint32_t size = width*height;

	int32_t startpos = int(ypos)*datawidth+int(xpos);
	int32_t targetsize = datawidth*dataheight*4;
	for (uint32_t i = 0; i < size; i++)
	{
		if (i && i%width==0)
			startpos+=datawidth;
		if (startpos < 0)
			continue;
		int32_t targetpos = startpos*4+(i%width)*4;
		if (targetpos < 0)
			continue;
		if (targetpos+3 >= targetsize)
			break;
		uint32_t glowalpha = min(uint32_t(0xff),uint32_t(number_t(inner ? 0xff - tmpdata[i*4+3] : tmpdata[i*4+3])*strength));
		number_t alpha = alphas[glowalpha];
		number_t srcalpha = max(0.0,min(1.0,alpha));
		number_t dstalpha = number_t(data[targetpos+3])/255.0;
		uint32_t color = colors[glowalpha];
		uint32_t newalpha;
		if (inner)
		{
			if (knockout)
			{
				data[targetpos  ] = min(uint32_t(0xff),uint32_t(number_t((color    )&0xff)*srcalpha*dstalpha));
				data[targetpos+1] = min(uint32_t(0xff),uint32_t(number_t((color>> 8)&0xff)*srcalpha*dstalpha));
				data[targetpos+2] = min(uint32_t(0xff),uint32_t(number_t((color>>16)&0xff)*srcalpha*dstalpha));
				newalpha = min(uint32_t(0xff),uint32_t(number_t(0xff)*srcalpha*dstalpha));
			}
			else
			{
				data[targetpos  ] = min(uint32_t(0xff),uint32_t(number_t((color    )&0xff)*srcalpha*dstalpha+number_t(data[targetpos  ])*(1.0-srcalpha)));
				data[targetpos+1] = min(uint32_t(0xff),uint32_t(number_t((color>> 8)&0xff)*srcalpha*dstalpha+number_t(data[targetpos+1])*(1.0-srcalpha)));
				data[targetpos+2] = min(uint32_t(0xff),uint32_t(number_t((color>>16)&0xff)*srcalpha*dstalpha+number_t(data[targetpos+2])*(1.0-srcalpha)));
				newalpha = min(uint32_t(0xff),uint32_t(number_t(0xff)*srcalpha*dstalpha+number_t(data[targetpos+3])*(1.0-srcalpha)));
			}
		}
		else
		{
			if (knockout)
			{
				data[targetpos  ] = min(uint32_t(0xff),uint32_t(number_t((color    )&0xff)*srcalpha*(1.0-dstalpha)));
				data[targetpos+1] = min(uint32_t(0xff),uint32_t(number_t((color>> 8)&0xff)*srcalpha*(1.0-dstalpha)));
				data[targetpos+2] = min(uint32_t(0xff),uint32_t(number_t((color>>16)&0xff)*srcalpha*(1.0-dstalpha)));
				newalpha = min(uint32_t(0xff),uint32_t(number_t(0xff)*srcalpha*(1.0-dstalpha)));
			}
			else
			{
				data[targetpos  ] = min(uint32_t(0xff),uint32_t(number_t((color    )&0xff)*srcalpha*(1.0-dstalpha)+number_t(data[targetpos  ])));
				data[targetpos+1] = min(uint32_t(0xff),uint32_t(number_t((color>> 8)&0xff)*srcalpha*(1.0-dstalpha)+number_t(data[targetpos+1])));
				data[targetpos+2] = min(uint32_t(0xff),uint32_t(number_t((color>>16)&0xff)*srcalpha*(1.0-dstalpha)+number_t(data[targetpos+2])));
				newalpha = min(uint32_t(0xff),uint32_t(number_t(0xff)*srcalpha*(1.0-dstalpha)+number_t(data[targetpos+3])));
			}
		}
		if (newalpha)
		{
			// un-premultiply alpha into result
			data[targetpos  ] = min(uint32_t(0xff),uint32_t(number_t(data[targetpos  ])*255.0/number_t(newalpha)));
			data[targetpos+1] = min(uint32_t(0xff),uint32_t(number_t(data[targetpos+1])*255.0/number_t(newalpha)));
			data[targetpos+2] = min(uint32_t(0xff),uint32_t(number_t(data[targetpos+2])*255.0/number_t(newalpha)));
		}
		data[targetpos+3] = newalpha;
	}
}


bool BitmapFilter::getRenderFilterArgsBlur(float* args, float blurX, float blurY, uint32_t step, uint32_t quality, uint32_t& nextstep) const
{
	nextstep=step+1;
	bool twosteps = blurX>0.0 && blurY > 0.0;
	
	if (step > (uint32_t)quality*(twosteps ? 2 : 1))
		return false;
	if (step == (uint32_t)quality*(twosteps ? 2 : 1))
	{
		nextstep=step;
		return false;
	}
	if (twosteps)
	{
		if (step%2)
			getRenderFilterArgsBlurVertical(args,blurY);
		else
			getRenderFilterArgsBlurHorizontal(args,blurX);
	}
	else
	{
		if (blurX>0.0)
			getRenderFilterArgsBlurHorizontal(args,blurX);
		else
			getRenderFilterArgsBlurVertical(args,blurY);
	}
	return true;
}

void BitmapFilter::getRenderFilterArgsBlurHorizontal(float* args, float blurX) const
{
	int oX;
	int oY;
	float sX;
	float sY;
	getSystemState()->stageCoordinateMapping(getSystemState()->getRenderThread()->windowWidth, getSystemState()->getRenderThread()->windowHeight, oX, oY, sX, sY);
	args[0]=float(FILTERSTEP_BLUR_HORIZONTAL );
	args[1]=blurX*sX;
}
void BitmapFilter::getRenderFilterArgsBlurVertical(float* args, float blurY) const
{
	int oX;
	int oY;
	float sX;
	float sY;
	getSystemState()->stageCoordinateMapping(getSystemState()->getRenderThread()->windowWidth, getSystemState()->getRenderThread()->windowHeight, oX, oY, sX, sY);
	args[0]=float(FILTERSTEP_BLUR_VERTICAL );
	args[1]=blurY*sY;
}
void BitmapFilter::getRenderFilterArgsDropShadow(float* args, bool inner, bool knockout,float strength,uint32_t color,float alpha,float xpos,float ypos) const
{
	int oX;
	int oY;
	float sX;
	float sY;
	getSystemState()->stageCoordinateMapping(getSystemState()->getRenderThread()->windowWidth, getSystemState()->getRenderThread()->windowHeight, oX, oY, sX, sY);
	args[0]=float(FILTERSTEP_DROPSHADOW);
	args[1]=inner;
	args[2]=knockout;
	args[3]=strength;
	RGBA c = RGBA(color,0);
	args[4]=c.rf();
	args[5]=c.gf();
	args[6]=c.bf();
	args[7]=alpha;
	args[8]=(xpos * sX);
	args[9]=(ypos * sY);
}

void BitmapFilter::getRenderFilterArgsBevel(float* args, bool inner, bool knockout, float strength, float distance, float angle) const
{
	int oX;
	int oY;
	float sX;
	float sY;
	args[0]=float(FILTERSTEP_BEVEL);
	args[1]=inner;
	args[2]=knockout;
	args[3]=strength;

	getSystemState()->stageCoordinateMapping(getSystemState()->getRenderThread()->windowWidth, getSystemState()->getRenderThread()->windowHeight, oX, oY, sX, sY);
	//highlightOffset
	args[4]=(cos(angle+(inner ? 0.0:M_PI)) * distance * sX);
	args[5]=(sin(angle+(inner ? 0.0:M_PI)) * distance * sY);
	//shadowOffset
	args[6]=(cos(angle+(inner ? M_PI:0.0)) * distance * sX);
	args[7]=(sin(angle+(inner ? M_PI:0.0)) * distance * sY);
}

ASFUNCTIONBODY_ATOM(BitmapFilter,clone)
{
	BitmapFilter* th=asAtomHandler::as<BitmapFilter>(obj);
	ret = asAtomHandler::fromObject(th->cloneImpl());
}



ShaderFilter::ShaderFilter(ASWorker* wrk,Class_base* c):
	BitmapFilter(wrk,c,SUBTYPE_SHADERFILTER)
{
}

void ShaderFilter::sinit(Class_base* c)
{
	CLASS_SETUP(c, BitmapFilter, _constructor, CLASS_SEALED | CLASS_FINAL);
}

void ShaderFilter::applyFilter(BitmapContainer* target, BitmapContainer* source, const RECT& sourceRect, number_t xpos, number_t ypos, number_t scalex, number_t scaley, DisplayObject* owner)
{
	LOG(LOG_NOT_IMPLEMENTED,"applyFilter for ShaderFilter");
}

void ShaderFilter::getRenderFilterArgs(uint32_t step, float* args) const
{
	LOG(LOG_NOT_IMPLEMENTED,"getRenderFilterArgs not yet implemented for "<<this->toDebugString());
	args[0]=0;
}

ASFUNCTIONBODY_ATOM(ShaderFilter,_constructor)
{
	//ShaderFilter *th = obj.as<ShaderFilter>();
	LOG(LOG_NOT_IMPLEMENTED,"ShaderFilter is not implemented");
}

BitmapFilter* ShaderFilter::cloneImpl() const
{
	return Class<ShaderFilter>::getInstanceS(getInstanceWorker());
}

void BitmapFilterQuality::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructorNotInstantiatable, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("HIGH",nsNameAndKind(),asAtomHandler::fromUInt(3),CONSTANT_TRAIT);
	c->setVariableAtomByQName("LOW",nsNameAndKind(),asAtomHandler::fromUInt(1),CONSTANT_TRAIT);
	c->setVariableAtomByQName("MEDIUM",nsNameAndKind(),asAtomHandler::fromUInt(2),CONSTANT_TRAIT);
}

void DisplacementMapFilterMode::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("CLAMP",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"clamp"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("COLOR",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"color"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("IGNORE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"ignore"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("WRAP",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"wrap"),CONSTANT_TRAIT);
}

void BitmapFilterType::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructorNotInstantiatable, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("FULL",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"full"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("INNER",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"inner"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("OUTER",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"outer"),CONSTANT_TRAIT);
}
