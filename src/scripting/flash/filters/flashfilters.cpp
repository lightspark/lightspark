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

void BitmapFilter::applyFilter(BitmapContainer* target, BitmapContainer* source, const RECT& sourceRect, int xpos, int ypos,number_t scalex,number_t scaley)
{
	LOG(LOG_ERROR,"applyFilter for "<<this->toDebugString());
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

void BitmapFilter::applyBlur(uint8_t* data, uint32_t width, uint32_t height, number_t blurx, number_t blury, int quality, number_t scalex, number_t scaley)
{
	blurx*=scalex;
	blury*=scaley;
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
void BitmapFilter::applyDropShadowFilter(BitmapContainer* target, uint8_t* tmpdata, const RECT& sourceRect, int xpos, int ypos, number_t strength, number_t alpha, uint32_t color, bool inner, bool knockout,number_t scalex,number_t scaley)
{
	xpos *= scalex;
	ypos *= scaley;
	uint32_t width = sourceRect.Xmax-sourceRect.Xmin;
	uint32_t height = sourceRect.Ymax-sourceRect.Ymin;
	uint32_t size = width*height;

	uint8_t* data = target->getData();
	int32_t startpos = ypos*target->getWidth();
	int32_t targetsize = target->getWidth()*target->getHeight()*4;
	for (uint32_t i = 0; i < size*4; i+=4)
	{
		if (i && i%(width*4)==0)
			startpos+=target->getWidth();
		if (startpos < 0)
			continue;
		int32_t targetpos = (xpos+startpos)*4+i%(width*4);
		if (targetpos < 0)
			continue;
		if (targetpos+3 >= targetsize)
			break;
		number_t glowalpha = (inner ? 0xff - tmpdata[i+3] : tmpdata[i+3]);
		number_t srcalpha = max(0.0,min(1.0,glowalpha*alpha*strength/255.0));
		number_t dstalpha = number_t(data[targetpos+3])/255.0;
		if (inner)
		{
			if (knockout)
			{
				data[targetpos  ] = min(uint32_t(0xff),uint32_t(number_t((color    )&0xff)*srcalpha*dstalpha));
				data[targetpos+1] = min(uint32_t(0xff),uint32_t(number_t((color>> 8)&0xff)*srcalpha*dstalpha));
				data[targetpos+2] = min(uint32_t(0xff),uint32_t(number_t((color>>16)&0xff)*srcalpha*dstalpha));
				data[targetpos+3] = min(uint32_t(0xff),uint32_t(number_t(0xff            )*srcalpha*dstalpha));
			}
			else
			{
				data[targetpos  ] = min(uint32_t(0xff),uint32_t(number_t((color    )&0xff)*srcalpha*dstalpha+number_t(data[targetpos  ])*(1.0-srcalpha)));
				data[targetpos+1] = min(uint32_t(0xff),uint32_t(number_t((color>> 8)&0xff)*srcalpha*dstalpha+number_t(data[targetpos+1])*(1.0-srcalpha)));
				data[targetpos+2] = min(uint32_t(0xff),uint32_t(number_t((color>>16)&0xff)*srcalpha*dstalpha+number_t(data[targetpos+2])*(1.0-srcalpha)));
				data[targetpos+3] = min(uint32_t(0xff),uint32_t(number_t(0xff            )*srcalpha*dstalpha+number_t(data[targetpos+3])*(1.0-srcalpha)));
			}
		}
		else
		{
			if (knockout)
			{
				data[targetpos  ] = min(uint32_t(0xff),uint32_t(number_t((color    )&0xff)*srcalpha*(1.0-dstalpha)));
				data[targetpos+1] = min(uint32_t(0xff),uint32_t(number_t((color>> 8)&0xff)*srcalpha*(1.0-dstalpha)));
				data[targetpos+2] = min(uint32_t(0xff),uint32_t(number_t((color>>16)&0xff)*srcalpha*(1.0-dstalpha)));
				data[targetpos+3] = min(uint32_t(0xff),uint32_t(number_t(0xff            )*srcalpha*(1.0-dstalpha)));
			}
			else
			{
				data[targetpos  ] = min(uint32_t(0xff),uint32_t(number_t((color    )&0xff)*srcalpha*(1.0-dstalpha)+number_t(data[targetpos  ])));
				data[targetpos+1] = min(uint32_t(0xff),uint32_t(number_t((color>> 8)&0xff)*srcalpha*(1.0-dstalpha)+number_t(data[targetpos+1])));
				data[targetpos+2] = min(uint32_t(0xff),uint32_t(number_t((color>>16)&0xff)*srcalpha*(1.0-dstalpha)+number_t(data[targetpos+2])));
				data[targetpos+3] = min(uint32_t(0xff),uint32_t(number_t(0xff            )*srcalpha*(1.0-dstalpha)+number_t(data[targetpos+3])));
			}
		}
	}
}
void BitmapFilter::fillGradientColors(number_t* gradientalphas, uint32_t* gradientcolors,Array* ratios,Array* alphas,Array* colors)
{
	number_t alpha = 1.0;
	uint32_t color = 0x000000;
	uint32_t ratioidx = 0;
	uint32_t nextratio = 0;
	uint32_t currratioidx = 0;
	for (uint32_t i=0; i <256; i++)
	{
		if (i >= nextratio)
		{
			currratioidx= ratioidx;
			ratioidx++;
			if (ratios && ratios->size() > ratioidx)
			{
				asAtom a=asAtomHandler::invalidAtom;
				ratios->at_nocheck(a,ratioidx);
				nextratio = asAtomHandler::toUInt(a);
			}
		}
		if (alphas && alphas->size() > currratioidx)
		{
			asAtom a=asAtomHandler::invalidAtom;
			alphas->at_nocheck(a,currratioidx);
			alpha = asAtomHandler::toNumber(a);
		}
		if (colors && colors->size() > currratioidx)
		{
			asAtom a=asAtomHandler::invalidAtom;
			colors->at_nocheck(a,currratioidx);
			color = asAtomHandler::toUInt(a);
		}
		gradientalphas[i] = alpha;
		gradientcolors[i] = color;
	}
}
void BitmapFilter::applyGradientFilter(BitmapContainer* target, uint8_t* tmpdata, const RECT& sourceRect, int xpos, int ypos, number_t strength, number_t* alphas, uint32_t* colors, bool inner, bool knockout,number_t scalex,number_t scaley)
{
	xpos *= scalex;
	ypos *= scaley;
	uint32_t width = sourceRect.Xmax-sourceRect.Xmin;
	uint32_t height = sourceRect.Ymax-sourceRect.Ymin;
	uint32_t size = width*height;

	uint8_t* data = target->getData();
	uint32_t startpos = ypos*target->getWidth();
	uint32_t targetsize = target->getWidth()*target->getHeight()*4;
	for (uint32_t i = 0; i < size*4; i+=4)
	{
		if (i && i%(width*4)==0)
			startpos+=target->getWidth();
		uint32_t targetpos = (xpos+startpos)*4+i%(width*4);
		if (targetpos+3 >= targetsize)
			break;
		number_t glowalpha = (inner ? 0xff - tmpdata[i+3] : tmpdata[i+3]);
		number_t alpha = alphas[uint32_t(glowalpha)];
		number_t srcalpha = max(0.0,min(1.0,glowalpha*alpha*strength/255.0));
		number_t dstalpha = number_t(data[targetpos+3])/255.0;
		uint32_t color = colors[uint32_t(glowalpha)];
		if (inner)
		{
			if (knockout)
			{
				data[targetpos  ] = min(uint32_t(0xff),uint32_t(number_t((color    )&0xff)*srcalpha*dstalpha));
				data[targetpos+1] = min(uint32_t(0xff),uint32_t(number_t((color>> 8)&0xff)*srcalpha*dstalpha));
				data[targetpos+2] = min(uint32_t(0xff),uint32_t(number_t((color>>16)&0xff)*srcalpha*dstalpha));
				data[targetpos+3] = min(uint32_t(0xff),uint32_t(number_t(0xff            )*srcalpha*dstalpha));
			}
			else
			{
				data[targetpos  ] = min(uint32_t(0xff),uint32_t(number_t((color    )&0xff)*srcalpha*dstalpha+number_t(data[targetpos  ])*(1.0-srcalpha)));
				data[targetpos+1] = min(uint32_t(0xff),uint32_t(number_t((color>> 8)&0xff)*srcalpha*dstalpha+number_t(data[targetpos+1])*(1.0-srcalpha)));
				data[targetpos+2] = min(uint32_t(0xff),uint32_t(number_t((color>>16)&0xff)*srcalpha*dstalpha+number_t(data[targetpos+2])*(1.0-srcalpha)));
				data[targetpos+3] = min(uint32_t(0xff),uint32_t(number_t(0xff            )*srcalpha*dstalpha+number_t(data[targetpos+3])*(1.0-srcalpha)));
			}
		}
		else
		{
			if (knockout)
			{
				data[targetpos  ] = min(uint32_t(0xff),uint32_t(number_t((color    )&0xff)*srcalpha*(1.0-dstalpha)));
				data[targetpos+1] = min(uint32_t(0xff),uint32_t(number_t((color>> 8)&0xff)*srcalpha*(1.0-dstalpha)));
				data[targetpos+2] = min(uint32_t(0xff),uint32_t(number_t((color>>16)&0xff)*srcalpha*(1.0-dstalpha)));
				data[targetpos+3] = min(uint32_t(0xff),uint32_t(number_t(0xff            )*srcalpha*(1.0-dstalpha)));
			}
			else
			{
				data[targetpos  ] = min(uint32_t(0xff),uint32_t(number_t((color    )&0xff)*srcalpha*(1.0-dstalpha)+number_t(data[targetpos  ])));
				data[targetpos+1] = min(uint32_t(0xff),uint32_t(number_t((color>> 8)&0xff)*srcalpha*(1.0-dstalpha)+number_t(data[targetpos+1])));
				data[targetpos+2] = min(uint32_t(0xff),uint32_t(number_t((color>>16)&0xff)*srcalpha*(1.0-dstalpha)+number_t(data[targetpos+2])));
				data[targetpos+3] = min(uint32_t(0xff),uint32_t(number_t(0xff            )*srcalpha*(1.0-dstalpha)+number_t(data[targetpos+3])));
			}
		}
	}
}

ASFUNCTIONBODY_ATOM(BitmapFilter,clone)
{
	BitmapFilter* th=asAtomHandler::as<BitmapFilter>(obj);
	ret = asAtomHandler::fromObject(th->cloneImpl());
}

GlowFilter::GlowFilter(ASWorker* wrk, Class_base* c):
	BitmapFilter(wrk,c,SUBTYPE_GLOWFILTER), alpha(1.0), blurX(6.0), blurY(6.0), color(0xFF0000),
	inner(false), knockout(false), quality(1), strength(2.0)
{
}
GlowFilter::GlowFilter(ASWorker* wrk, Class_base* c, const GLOWFILTER& filter):
	BitmapFilter(wrk,c,SUBTYPE_GLOWFILTER), alpha(filter.GlowColor.af()), blurX(filter.BlurX), blurY(filter.BlurY), color(RGB(filter.GlowColor.Red,filter.GlowColor.Green,filter.GlowColor.Blue).toUInt()),
	inner(filter.InnerGlow), knockout(filter.Knockout), quality(filter.Passes), strength(filter.Strength)
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

ASFUNCTIONBODY_GETTER_SETTER(GlowFilter, alpha)
ASFUNCTIONBODY_GETTER_SETTER(GlowFilter, blurX)
ASFUNCTIONBODY_GETTER_SETTER(GlowFilter, blurY)
ASFUNCTIONBODY_GETTER_SETTER(GlowFilter, color)
ASFUNCTIONBODY_GETTER_SETTER(GlowFilter, inner)
ASFUNCTIONBODY_GETTER_SETTER(GlowFilter, knockout)
ASFUNCTIONBODY_GETTER_SETTER(GlowFilter, quality)
ASFUNCTIONBODY_GETTER_SETTER(GlowFilter, strength)

ASFUNCTIONBODY_ATOM(GlowFilter,_constructor)
{
	GlowFilter *th = asAtomHandler::as<GlowFilter>(obj);
	ARG_CHECK(ARG_UNPACK (th->color, 0xFF0000)
		(th->alpha, 1.0)
		(th->blurX, 6.0)
		(th->blurY, 6.0)
		(th->strength, 2.0)
		(th->quality, 1)
		(th->inner, false)
		(th->knockout, false));
}

BitmapFilter* GlowFilter::cloneImpl() const
{
	GlowFilter *cloned = Class<GlowFilter>::getInstanceS(getInstanceWorker());
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
void GlowFilter::applyFilter(BitmapContainer* target, BitmapContainer* source, const RECT& sourceRect, int xpos, int ypos,number_t scalex,number_t scaley)
{
	uint8_t* tmpdata = nullptr;
	if (source)
		tmpdata = source->getRectangleData(sourceRect);
	else
		tmpdata = target->getRectangleData(sourceRect);

	applyBlur(tmpdata,sourceRect.Xmax-sourceRect.Xmin,sourceRect.Ymax-sourceRect.Ymin,blurX,blurY,quality,scalex,scaley);
	applyDropShadowFilter(target, tmpdata, sourceRect, xpos, ypos, strength, alpha, color, inner, knockout,scalex,scaley);
	delete[] tmpdata;
}

DropShadowFilter::DropShadowFilter(ASWorker* wrk,Class_base* c):
	BitmapFilter(wrk,c,SUBTYPE_DROPSHADOWFILTER), alpha(1.0), angle(45), blurX(4.0), blurY(4.0),
	color(0), distance(4.0), hideObject(false), inner(false),
	knockout(false), quality(1), strength(1.0)
{
}
DropShadowFilter::DropShadowFilter(ASWorker* wrk,Class_base* c,const DROPSHADOWFILTER& filter):
	BitmapFilter(wrk,c,SUBTYPE_DROPSHADOWFILTER), alpha(filter.DropShadowColor.af()), angle(filter.Angle), blurX(filter.BlurX), blurY(filter.BlurY),
	color(RGB(filter.DropShadowColor.Red,filter.DropShadowColor.Green,filter.DropShadowColor.Blue).toUInt()), distance(filter.Distance), hideObject(false), inner(filter.InnerShadow),
	knockout(filter.Knockout), quality(filter.Passes), strength(filter.Strength)
{
}
void DropShadowFilter::applyFilter(BitmapContainer* target, BitmapContainer* source, const RECT& sourceRect, int xpos, int ypos, number_t scalex, number_t scaley)
{
	xpos += cos(angle) * distance;
	ypos +=	sin(angle) * distance;
	if (hideObject)
		LOG(LOG_NOT_IMPLEMENTED,"DropShadowFilter.hideObject");
	uint8_t* tmpdata = nullptr;
	if (source)
		tmpdata = source->getRectangleData(sourceRect);
	else
		tmpdata = target->getRectangleData(sourceRect);

	applyBlur(tmpdata,sourceRect.Xmax-sourceRect.Xmin,sourceRect.Ymax-sourceRect.Ymin,blurX,blurY,quality,scalex,scaley);
	applyDropShadowFilter(target, tmpdata, sourceRect, xpos, ypos, strength, alpha, color, inner, knockout,scalex,scaley);
	delete[] tmpdata;
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

ASFUNCTIONBODY_GETTER_SETTER(DropShadowFilter, alpha)
ASFUNCTIONBODY_GETTER_SETTER(DropShadowFilter, angle)
ASFUNCTIONBODY_GETTER_SETTER(DropShadowFilter, blurX)
ASFUNCTIONBODY_GETTER_SETTER(DropShadowFilter, blurY)
ASFUNCTIONBODY_GETTER_SETTER(DropShadowFilter, color)
ASFUNCTIONBODY_GETTER_SETTER(DropShadowFilter, distance)
ASFUNCTIONBODY_GETTER_SETTER(DropShadowFilter, hideObject)
ASFUNCTIONBODY_GETTER_SETTER(DropShadowFilter, inner)
ASFUNCTIONBODY_GETTER_SETTER(DropShadowFilter, knockout)
ASFUNCTIONBODY_GETTER_SETTER(DropShadowFilter, quality)
ASFUNCTIONBODY_GETTER_SETTER(DropShadowFilter, strength)

ASFUNCTIONBODY_ATOM(DropShadowFilter,_constructor)
{
	DropShadowFilter *th = asAtomHandler::as<DropShadowFilter>(obj);
	ARG_CHECK(ARG_UNPACK (th->distance, 4.0)
		(th->angle, 45)
		(th->color, 0)
		(th->alpha, 1.0)
		(th->blurX, 4.0)
		(th->blurY, 4.0)
		(th->strength, 1.0)
		(th->quality, 1)
		(th->inner, false)
		(th->knockout, false)
		(th->hideObject, false));
}

BitmapFilter* DropShadowFilter::cloneImpl() const
{
	DropShadowFilter *cloned = Class<DropShadowFilter>::getInstanceS(getInstanceWorker());
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

GradientGlowFilter::GradientGlowFilter(ASWorker* wrk, Class_base* c):
	BitmapFilter(wrk,c,SUBTYPE_GRADIENTGLOWFILTER),distance(4.0),angle(45), blurX(4.0), blurY(4.0), strength(1), quality(1), type("inner"), knockout(false)
{
}
GradientGlowFilter::GradientGlowFilter(ASWorker* wrk, Class_base* c, const GRADIENTGLOWFILTER& filter):
	BitmapFilter(wrk,c,SUBTYPE_GRADIENTGLOWFILTER),distance(filter.Distance),angle(filter.Angle), blurX(filter.BlurX), blurY(filter.BlurY), strength(filter.Strength), quality(filter.Passes),
	type(filter.InnerGlow ? "inner": "outer"),// TODO: is type set based on "onTop" ?
	knockout(filter.Knockout)
{
	if (filter.GradientColors.size())
	{
		colors = _MR(Class<Array>::getInstanceSNoArgs(wrk));
		alphas = _MR(Class<Array>::getInstanceSNoArgs(wrk));
		auto it = filter.GradientColors.begin();
		while (it != filter.GradientColors.end())
		{
			colors->push(asAtomHandler::fromUInt(RGB(it->Red,it->Green,it->Blue).toUInt()));
			alphas->push(asAtomHandler::fromNumber(wrk,it->af(),false));
			it++;
		}
	}
	if (filter.GradientRatio.size())
	{
		ratios = _MR(Class<Array>::getInstanceSNoArgs(wrk));
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

void GradientGlowFilter::applyFilter(BitmapContainer* target, BitmapContainer* source, const RECT& sourceRect, int xpos, int ypos, number_t scalex, number_t scaley)
{
	uint8_t* tmpdata = nullptr;
	if (source)
		tmpdata = source->getRectangleData(sourceRect);
	else
		tmpdata = target->getRectangleData(sourceRect);

	number_t gradientalphas[256];
	uint32_t gradientcolors[256];
	fillGradientColors(gradientalphas,gradientcolors,this->ratios.getPtr(), this->alphas.getPtr(), this->colors.getPtr());
	applyBlur(tmpdata,sourceRect.Xmax-sourceRect.Xmin,sourceRect.Ymax-sourceRect.Ymin,blurX,blurY,quality,scalex,scaley);
	applyGradientFilter(target, tmpdata, sourceRect, xpos, ypos, strength, gradientalphas, gradientcolors, type=="inner", knockout,scalex,scaley);
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

ASFUNCTIONBODY_GETTER_SETTER(GradientGlowFilter, distance)
ASFUNCTIONBODY_GETTER_SETTER(GradientGlowFilter, angle)
ASFUNCTIONBODY_GETTER_SETTER(GradientGlowFilter, colors)
ASFUNCTIONBODY_GETTER_SETTER(GradientGlowFilter, alphas)
ASFUNCTIONBODY_GETTER_SETTER(GradientGlowFilter, ratios)
ASFUNCTIONBODY_GETTER_SETTER(GradientGlowFilter, blurX)
ASFUNCTIONBODY_GETTER_SETTER(GradientGlowFilter, blurY)
ASFUNCTIONBODY_GETTER_SETTER(GradientGlowFilter, strength)
ASFUNCTIONBODY_GETTER_SETTER(GradientGlowFilter, quality)
ASFUNCTIONBODY_GETTER_SETTER(GradientGlowFilter, type)
ASFUNCTIONBODY_GETTER_SETTER(GradientGlowFilter, knockout)

ASFUNCTIONBODY_ATOM(GradientGlowFilter,_constructor)
{
	GradientGlowFilter *th = asAtomHandler::as<GradientGlowFilter>(obj);
	ARG_CHECK(ARG_UNPACK(th->distance,4.0)(th->angle,45)(th->colors,NullRef)(th->alphas,NullRef)(th->ratios,NullRef)(th->blurX,4.0)(th->blurY,4.0)(th->strength,1)(th->quality,1)(th->type,"inner")(th->knockout,false));
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

BevelFilter::BevelFilter(ASWorker* wrk,Class_base* c):
	BitmapFilter(wrk,c,SUBTYPE_BEVELFILTER), angle(45), blurX(4.0), blurY(4.0),distance(4.0),
	highlightAlpha(1.0), highlightColor(0xFFFFFF),
	knockout(false), quality(1),
	shadowAlpha(1.0), shadowColor(0x000000),
	strength(1), type("inner")
{
}
BevelFilter::BevelFilter(ASWorker* wrk, Class_base* c, const BEVELFILTER& filter):
	BitmapFilter(wrk,c,SUBTYPE_BEVELFILTER),angle(filter.Angle),blurX(filter.BlurX), blurY(filter.BlurY),distance(filter.Distance), 
	highlightAlpha(filter.HighlightColor.af()), highlightColor(RGB(filter.HighlightColor.Red,filter.HighlightColor.Green,filter.HighlightColor.Blue).toUInt()),
	knockout(filter.Knockout), quality(filter.Passes),
	shadowAlpha(filter.ShadowColor.af()), shadowColor(RGB(filter.ShadowColor.Red,filter.ShadowColor.Green,filter.ShadowColor.Blue).toUInt()),
	strength(filter.Strength), type(filter.InnerShadow ? "inner" : "outer") // TODO: is type set based on "onTop" ?
{
	if (filter.OnTop)
		LOG(LOG_NOT_IMPLEMENTED,"BevelFilter onTop flag");
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

ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(BevelFilter,angle)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(BevelFilter,blurX)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(BevelFilter,blurY)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(BevelFilter,distance)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(BevelFilter,highlightAlpha)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(BevelFilter,highlightColor)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(BevelFilter,knockout)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(BevelFilter,quality)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(BevelFilter,shadowAlpha)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(BevelFilter,shadowColor)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(BevelFilter,strength)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(BevelFilter,type)

void BevelFilter::applyFilter(BitmapContainer* target, BitmapContainer* source, const RECT& sourceRect, int xpos, int ypos, number_t scalex, number_t scaley)
{
	if (type=="full")
		LOG(LOG_NOT_IMPLEMENTED,"BevelFilter type 'full'");
	uint8_t* tmpdata = nullptr;
	if (source)
		tmpdata = source->getRectangleData(sourceRect);
	else
		tmpdata = target->getRectangleData(sourceRect);

	applyBlur(tmpdata,sourceRect.Xmax-sourceRect.Xmin,sourceRect.Ymax-sourceRect.Ymin,blurX,blurY,quality,scalex,scaley);
	// TODO I've not found any useful documentation how BevelFilter should be implemented, so we just apply two dropShadowFilters with different angles and colors on the blurred data
	applyDropShadowFilter(target, tmpdata, sourceRect, xpos+cos(angle+M_PI) * distance, ypos+sin(angle+M_PI) * distance, strength, highlightAlpha, highlightColor, type=="inner", knockout,scalex,scaley);
	applyDropShadowFilter(target, tmpdata, sourceRect, xpos+cos(angle     ) * distance, ypos+sin(angle     ) * distance, strength, shadowAlpha   , shadowColor   , type=="inner", knockout,scalex,scaley);
	delete[] tmpdata;

}

ASFUNCTIONBODY_ATOM(BevelFilter,_constructor)
{
	BevelFilter *th = asAtomHandler::as<BevelFilter>(obj);
	ARG_CHECK(ARG_UNPACK(th->distance,4.0)(th->angle,45)(th->highlightColor,0xFFFFFF)(th->highlightAlpha,1.0)(th->shadowColor,0x000000)(th->shadowAlpha,1.0)(th->blurX,4.0)(th->blurY,4.0)(th->strength,1)(th->quality,1)(th->type,"inner")(th->knockout,false));
}

BitmapFilter* BevelFilter::cloneImpl() const
{
	BevelFilter* cloned = Class<BevelFilter>::getInstanceS(getInstanceWorker());
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
ColorMatrixFilter::ColorMatrixFilter(ASWorker* wrk, Class_base* c):
	BitmapFilter(wrk,c,SUBTYPE_COLORMATRIXFILTER)
{
}
ColorMatrixFilter::ColorMatrixFilter(ASWorker* wrk, Class_base* c, const COLORMATRIXFILTER& filter):
	BitmapFilter(wrk,c,SUBTYPE_COLORMATRIXFILTER)
{
	matrix = _MR(Class<Array>::getInstanceSNoArgs(wrk));
	for (uint32_t i = 0; i < 20 ; i++)
	{
		FLOAT f = filter.Matrix[i];
		matrix->push(asAtomHandler::fromNumber(wrk,f,false));
	}
}

void ColorMatrixFilter::sinit(Class_base* c)
{
	CLASS_SETUP(c, BitmapFilter, _constructor, CLASS_SEALED | CLASS_FINAL);
	REGISTER_GETTER_SETTER(c, matrix);
}

void ColorMatrixFilter::applyFilter(BitmapContainer* target, BitmapContainer* source, const RECT& sourceRect, int xpos, int ypos, number_t scalex, number_t scaley)
{
	xpos *= scalex;
	ypos *= scaley;
	assert_and_throw(matrix->size() >= 20);
	number_t m[20];
	for (int i=0; i < 20; i++)
	{
		m[i] = asAtomHandler::toNumber(matrix->at(i));
	}
	uint32_t width = sourceRect.Xmax-sourceRect.Xmin;
	uint32_t height = sourceRect.Ymax-sourceRect.Ymin;
	uint32_t size = width*height;
	uint8_t* tmpdata = nullptr;
	if (source)
		tmpdata = source->getRectangleData(sourceRect);
	else
		tmpdata = target->getRectangleData(sourceRect);
	
	uint8_t* data = target->getData();
	uint32_t startpos = ypos*target->getWidth();
	uint32_t targetsize = target->getWidth()*target->getHeight()*4;
	for (uint32_t i = 0; i < size*4; i+=4)
	{
		if (i && i%(width*4)==0)
			startpos+=target->getWidth();
		uint32_t targetpos = (xpos+startpos)*4+i%(width*4);
		if (targetpos+3 >= targetsize)
			break;

		number_t srcA = number_t(tmpdata[i+3]);
		number_t srcR = number_t(tmpdata[i+2])*srcA/255.0;
		number_t srcG = number_t(tmpdata[i+1])*srcA/255.0;
		number_t srcB = number_t(tmpdata[i  ])*srcA/255.0;
		number_t redResult   = (m[0 ]*srcR) + (m[1 ]*srcG) + (m[2 ]*srcB) + (m[3 ]*srcA) + m[4 ];
		number_t greenResult = (m[5 ]*srcR) + (m[6 ]*srcG) + (m[7 ]*srcB) + (m[8 ]*srcA) + m[9 ];
		number_t blueResult  = (m[10]*srcR) + (m[11]*srcG) + (m[12]*srcB) + (m[13]*srcA) + m[14];
		number_t alphaResult = (m[15]*srcR) + (m[16]*srcG) + (m[17]*srcB) + (m[18]*srcA) + m[19];

		data[targetpos  ] = (max(int32_t(0),min(int32_t(0xff),int32_t(blueResult *alphaResult/255.0))));
		data[targetpos+1] = (max(int32_t(0),min(int32_t(0xff),int32_t(greenResult*alphaResult/255.0))));
		data[targetpos+2] = (max(int32_t(0),min(int32_t(0xff),int32_t(redResult  *alphaResult/255.0))));
		data[targetpos+3] = (max(int32_t(0),min(int32_t(0xff),int32_t(alphaResult))));
	}
	delete[] tmpdata;
}

ASFUNCTIONBODY_GETTER_SETTER(ColorMatrixFilter, matrix)

ASFUNCTIONBODY_ATOM(ColorMatrixFilter,_constructor)
{
	ColorMatrixFilter *th = asAtomHandler::as<ColorMatrixFilter>(obj);
	ARG_CHECK(ARG_UNPACK(th->matrix,NullRef));
}

BitmapFilter* ColorMatrixFilter::cloneImpl() const
{
	ColorMatrixFilter *cloned = Class<ColorMatrixFilter>::getInstanceS(getInstanceWorker());
	if (!matrix.isNull())
	{
		matrix->incRef();
		cloned->matrix = matrix;
	}
	return cloned;
}

void ColorMatrixFilter::prepareShutdown()
{
	if (preparedforshutdown)
		return;
	BitmapFilter::prepareShutdown();
	if (matrix)
		matrix->prepareShutdown();
}

BlurFilter::BlurFilter(ASWorker* wrk,Class_base* c):
	BitmapFilter(wrk,c,SUBTYPE_BLURFILTER),blurX(4.0),blurY(4.0),quality(1)
{
}
BlurFilter::BlurFilter(ASWorker* wrk,Class_base* c,const BLURFILTER& filter):
	BitmapFilter(wrk,c,SUBTYPE_BLURFILTER),blurX(filter.BlurX),blurY(filter.BlurY),quality(filter.Passes)
{
}

void BlurFilter::sinit(Class_base* c)
{
	CLASS_SETUP(c, BitmapFilter, _constructor, CLASS_SEALED | CLASS_FINAL);
	REGISTER_GETTER_SETTER(c, blurX);
	REGISTER_GETTER_SETTER(c, blurY);
	REGISTER_GETTER_SETTER(c, quality);
}

ASFUNCTIONBODY_GETTER_SETTER(BlurFilter, blurX)
ASFUNCTIONBODY_GETTER_SETTER(BlurFilter, blurY)
ASFUNCTIONBODY_GETTER_SETTER(BlurFilter, quality)

ASFUNCTIONBODY_ATOM(BlurFilter,_constructor)
{
	BlurFilter *th = asAtomHandler::as<BlurFilter>(obj);
	ARG_CHECK(ARG_UNPACK(th->blurX,4.0)(th->blurY,4.0)(th->quality,1));
}
void BlurFilter::applyFilter(BitmapContainer* target, BitmapContainer* source, const RECT& sourceRect, int xpos, int ypos, number_t scalex, number_t scaley)
{
	uint8_t* tmpdata = nullptr;
	if (source)
		tmpdata = source->getRectangleData(sourceRect);
	else
		tmpdata = target->getRectangleData(sourceRect);
	applyBlur(tmpdata,sourceRect.Xmax-sourceRect.Xmin,sourceRect.Ymax-sourceRect.Ymin,blurX,blurY,quality,scalex,scaley);

	uint32_t width = sourceRect.Xmax-sourceRect.Xmin;
	uint32_t height = sourceRect.Ymax-sourceRect.Ymin;
	uint8_t* data = target->getData();
	for (uint32_t i = 0; i < height; i++)
	{
		memcpy(data+((ypos+i)*width+xpos)*4, tmpdata+i*width*4,width*4);
	}
	delete[] tmpdata;
}
BitmapFilter* BlurFilter::cloneImpl() const
{
	BlurFilter* cloned = Class<BlurFilter>::getInstanceS(getInstanceWorker());
	cloned->blurX = blurX;
	cloned->blurY = blurY;
	cloned->quality = quality;
	return cloned;
}

ConvolutionFilter::ConvolutionFilter(ASWorker* wrk,Class_base* c):
	BitmapFilter(wrk,c,SUBTYPE_CONVOLUTIONFILTER),
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
ConvolutionFilter::ConvolutionFilter(ASWorker* wrk,Class_base* c, const CONVOLUTIONFILTER& filter):
	BitmapFilter(wrk,c,SUBTYPE_CONVOLUTIONFILTER),
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
		matrix = _MR(Class<Array>::getInstanceSNoArgs(wrk));
		auto it = filter.Matrix.begin();
		while (it != filter.Matrix.end())
		{
			matrix->push(asAtomHandler::fromNumber(wrk,(FLOAT)*it,false));
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

void ConvolutionFilter::applyFilter(BitmapContainer* target, BitmapContainer* source, const RECT& sourceRect, int xpos, int ypos, number_t scalex, number_t scaley)
{
	xpos *= scalex;
	ypos *= scaley;
	// spec is not really clear how this should be implemented, especially when using something different than a 3x3 matrix:
	// "
	// For a 3 x 3 matrix convolution, the following formula is used for each independent color channel:
	// dst (x, y) = ((src (x-1, y-1) * a0 + src(x, y-1) * a1....
	//					  src(x, y+1) * a7 + src (x+1,y+1) * a8) / divisor) + bias
	// "
	uint32_t mX = matrixX;
	uint32_t mY = matrixY;
	number_t* m =new number_t[mX*mY];
	for (uint32_t y=0; y < mY ; y++)
	{
		for (uint32_t x=0; x < mX ; x++)
		{
			m[y*mX+x] = matrix->size() < y*mX+x ? asAtomHandler::toNumber(matrix->at(y*mX+x)) : 0;
		}
	}
	uint32_t width = sourceRect.Xmax-sourceRect.Xmin;
	uint32_t height = sourceRect.Ymax-sourceRect.Ymin;
	uint32_t size = width*height;
	uint8_t* tmpdata = nullptr;
	if (source)
		tmpdata = source->getRectangleData(sourceRect);
	else
		tmpdata = target->getRectangleData(sourceRect);
	
	uint8_t* data = target->getData();
	uint32_t startpos = ypos*target->getWidth();
	int32_t mstartpos=-(mY/2*width*4+mX/2);
	uint32_t targetsize = target->getWidth()*target->getHeight()*4;
	number_t realdivisor = divisor==0 ? 1.0 : divisor;
	for (uint32_t i = 0; i < size*4; i+=4)
	{
		if (i && i%(width*4)==0)
			startpos+=target->getWidth();
		uint32_t targetpos = (xpos+startpos)*4+i%(width*4);
		if (targetpos+3 >= targetsize)
			break;

		number_t redResult   = 0;
		number_t greenResult = 0;
		number_t blueResult  = 0;
		number_t alphaResult = 0;
		for (uint32_t y=0; y <mY; y++)
		{
			for (uint32_t x=0; x <mX; x++)
			{
				if (
						(i%(width*4) <= mX/2
						|| i%(width*4) >= (width-mX/2)*4
						|| i/(width*4) <= mY/2
						|| i/(width*4) >= height-mY/2))
				{
					if (clamp)
					{
						alphaResult += number_t(tmpdata[i+3])*m[y*mX+x];
						redResult   += number_t(tmpdata[i+2])*m[y*mX+x];
						greenResult += number_t(tmpdata[i+1])*m[y*mX+x];
						blueResult  += number_t(tmpdata[i  ])*m[y*mX+x];
					}
					else
					{
						alphaResult += ((alpha*255)     )*m[y*mX+x];
						redResult   += ((color>>16)&0xff)*m[y*mX+x];
						greenResult += ((color>> 8)&0xff)*m[y*mX+x];
						blueResult  += ((color    )&0xff)*m[y*mX+x];
					}
				}
				else
				{
					alphaResult += number_t(tmpdata[mstartpos + y*mY +x+3])*m[y*mX+x];
					redResult   += number_t(tmpdata[mstartpos + y*mY +x+2])*m[y*mX+x];
					greenResult += number_t(tmpdata[mstartpos + y*mY +x+1])*m[y*mX+x];
					blueResult  += number_t(tmpdata[mstartpos + y*mY +x  ])*m[y*mX+x];
				}
			}
		}
		data[targetpos  ] = (max(int32_t(0),min(int32_t(0xff),int32_t(blueResult  / realdivisor + bias))));
		data[targetpos+1] = (max(int32_t(0),min(int32_t(0xff),int32_t(greenResult / realdivisor + bias))));
		data[targetpos+2] = (max(int32_t(0),min(int32_t(0xff),int32_t(redResult   / realdivisor + bias))));
		if (!preserveAlpha)
			data[targetpos+3] = (max(int32_t(0),min(int32_t(0xff),int32_t(alphaResult / realdivisor + bias))));
		mstartpos+=4;
	}
	delete[] m;
	delete[] tmpdata;
}

void ConvolutionFilter::prepareShutdown()
{
	if (preparedforshutdown)
		return;
	BitmapFilter::prepareShutdown();
	if (matrix)
		matrix->prepareShutdown();
}

ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(ConvolutionFilter,alpha)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(ConvolutionFilter,bias)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(ConvolutionFilter,clamp)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(ConvolutionFilter,color)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(ConvolutionFilter,divisor)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(ConvolutionFilter,matrix)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(ConvolutionFilter,matrixX)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(ConvolutionFilter,matrixY)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(ConvolutionFilter,preserveAlpha)

ASFUNCTIONBODY_ATOM(ConvolutionFilter,_constructor)
{
	ConvolutionFilter *th = asAtomHandler::as<ConvolutionFilter>(obj);
	ARG_CHECK(ARG_UNPACK(th->matrixX,0)(th->matrixY,0)(th->matrix,NullRef)(th->divisor,1.0)(th->bias,0.0)(th->preserveAlpha,true)(th->clamp,true)(th->color,0)(th->alpha,0.0));
}

BitmapFilter* ConvolutionFilter::cloneImpl() const
{
	ConvolutionFilter* cloned = Class<ConvolutionFilter>::getInstanceS(getInstanceWorker());
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

DisplacementMapFilter::DisplacementMapFilter(ASWorker* wrk,Class_base* c):
	BitmapFilter(wrk,c,SUBTYPE_DISPLACEMENTFILTER)
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

void DisplacementMapFilter::applyFilter(BitmapContainer* target, BitmapContainer* source, const RECT& sourceRect, int xpos, int ypos, number_t scalex, number_t scaley)
{
	xpos *= scalex;
	ypos *= scaley;
	uint32_t width = sourceRect.Xmax-sourceRect.Xmin;
	uint32_t height = sourceRect.Ymax-sourceRect.Ymin;
	uint32_t size = width*height;
	uint32_t mapx = mapPoint ? uint32_t(mapPoint->getX()) : 0;
	uint32_t mapy = mapPoint ? uint32_t(mapPoint->getX()) : 0;
	if (mapBitmap.isNull() || mapBitmap->getBitmapContainer().isNull() || (mapBitmap->getBitmapContainer()->getWidth()-mapx)*(mapBitmap->getBitmapContainer()->getHeight()-mapy) > size)
		return;
	uint8_t* tmpdata = nullptr;
	if (source)
		tmpdata = source->getRectangleData(sourceRect);
	else
		tmpdata = target->getRectangleData(sourceRect);
	
	uint8_t* data = target->getData();
	uint32_t startpos = ypos*target->getWidth();
	uint32_t targetsize = target->getWidth()*target->getHeight()*4;
	uint32_t mapPointstartpos = mapPoint ? (mapx+mapy*mapBitmap->getWidth())*4 : 0;
	uint8_t* mapbitmapdata=mapBitmap->getBitmapContainer()->getData();
	uint32_t mapchannelindexX = 0;
	uint32_t mapchannelindexY = 0;
	switch (componentX)
	{
		case BitmapDataChannel::ALPHA:
			mapchannelindexX=3;
			break;
		case BitmapDataChannel::RED:
			mapchannelindexX=2;
			break;
		case BitmapDataChannel::GREEN:
			mapchannelindexX=1;
			break;
		case BitmapDataChannel::BLUE:
			mapchannelindexX=0;
			break;
	}
	switch (componentY)
	{
		case BitmapDataChannel::ALPHA:
			mapchannelindexY=3;
			break;
		case BitmapDataChannel::RED:
			mapchannelindexY=2;
			break;
		case BitmapDataChannel::GREEN:
			mapchannelindexY=1;
			break;
		case BitmapDataChannel::BLUE:
			mapchannelindexY=0;
			break;
	}

	for (uint32_t i = 0; i < size*4; i+=4)
	{
		if (i && i%(width*4)==0)
		{
			startpos+=target->getWidth();
			mapPointstartpos+=mapBitmap->getWidth();
		}
		uint32_t targetpos = (xpos+startpos)*4+i%(width*4);
		uint32_t mappointpos = (mapPointstartpos)*4+i%(width*4);
		if (targetpos+3 >= targetsize)
			break;

		int32_t alphaResult = tmpdata[i+3 + int32_t((mapbitmapdata[mappointpos+mapchannelindexX]-128)*scaleX/256 + (mapbitmapdata[mappointpos+mapchannelindexY]-128)*scaleY/256)];
		int32_t redResult   = tmpdata[i+2 + int32_t((mapbitmapdata[mappointpos+mapchannelindexX]-128)*scaleX/256 + (mapbitmapdata[mappointpos+mapchannelindexY]-128)*scaleY/256)];
		int32_t greenResult = tmpdata[i+1 + int32_t((mapbitmapdata[mappointpos+mapchannelindexX]-128)*scaleX/256 + (mapbitmapdata[mappointpos+mapchannelindexY]-128)*scaleY/256)];
		int32_t blueResult  = tmpdata[i   + int32_t((mapbitmapdata[mappointpos+mapchannelindexX]-128)*scaleX/256 + (mapbitmapdata[mappointpos+mapchannelindexY]-128)*scaleY/256)];

		data[targetpos  ] = max(int32_t(0),min(int32_t(0xff),blueResult ));
		data[targetpos+1] = max(int32_t(0),min(int32_t(0xff),greenResult));
		data[targetpos+2] = max(int32_t(0),min(int32_t(0xff),redResult  ));
		data[targetpos+3] = max(int32_t(0),min(int32_t(0xff),alphaResult));
	}
	delete[] tmpdata;
}

void DisplacementMapFilter::prepareShutdown()
{
	if (preparedforshutdown)
		return;
	BitmapFilter::prepareShutdown();
	if (mapBitmap)
		mapBitmap->prepareShutdown();
	if (mapPoint)
		mapPoint->prepareShutdown();
}

ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(DisplacementMapFilter,alpha)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(DisplacementMapFilter,color)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(DisplacementMapFilter,componentX)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(DisplacementMapFilter,componentY)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(DisplacementMapFilter,mapBitmap)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(DisplacementMapFilter,mapPoint)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(DisplacementMapFilter,mode)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(DisplacementMapFilter,scaleX)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(DisplacementMapFilter,scaleY)

ASFUNCTIONBODY_ATOM(DisplacementMapFilter,_constructor)
{
	DisplacementMapFilter *th = asAtomHandler::as<DisplacementMapFilter>(obj);
	ARG_CHECK(ARG_UNPACK(th->mapBitmap,NullRef)(th->mapPoint,NullRef)(th->componentX,0)(th->componentY,0)(th->scaleX,0.0)(th->scaleY,0.0)(th->mode,"wrap")(th->color,0)(th->alpha,0.0));
}

BitmapFilter* DisplacementMapFilter::cloneImpl() const
{
	DisplacementMapFilter* cloned = Class<DisplacementMapFilter>::getInstanceS(getInstanceWorker());
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

GradientBevelFilter::GradientBevelFilter(ASWorker* wrk,Class_base* c):
	BitmapFilter(wrk,c,SUBTYPE_GRADIENTBEVELFILTER),
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
GradientBevelFilter::GradientBevelFilter(ASWorker* wrk,Class_base* c, const GRADIENTBEVELFILTER& filter):
	BitmapFilter(wrk,c,SUBTYPE_GRADIENTBEVELFILTER),
	angle(filter.Angle),
	blurX(filter.BlurX),
	blurY(filter.BlurY),
	distance(filter.Distance),
	knockout(filter.Knockout),
	quality(filter.Passes),
	strength(filter.Strength),
	type(filter.InnerShadow ? "inner" : "outer") // TODO: is type set based on "onTop" ?
{
	if (filter.OnTop)
		LOG(LOG_NOT_IMPLEMENTED,"GradientBevelFilter onTop flag");
	if (filter.GradientColors.size())
	{
		colors = _MR(Class<Array>::getInstanceSNoArgs(wrk));
		alphas = _MR(Class<Array>::getInstanceSNoArgs(wrk));
		auto it = filter.GradientColors.begin();
		while (it != filter.GradientColors.end())
		{
			colors->push(asAtomHandler::fromUInt(RGB(it->Red,it->Green,it->Blue).toUInt()));
			alphas->push(asAtomHandler::fromNumber(wrk,it->af(),false));
			it++;
		}
	}
	if (filter.GradientRatio.size())
	{
		ratios = _MR(Class<Array>::getInstanceSNoArgs(wrk));
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

ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(GradientBevelFilter,alphas)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(GradientBevelFilter,angle)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(GradientBevelFilter,blurX)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(GradientBevelFilter,blurY)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(GradientBevelFilter,colors)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(GradientBevelFilter,distance)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(GradientBevelFilter,knockout)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(GradientBevelFilter,quality)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(GradientBevelFilter,ratios)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(GradientBevelFilter,strength)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(GradientBevelFilter,type)

void GradientBevelFilter::applyFilter(BitmapContainer* target, BitmapContainer* source, const RECT& sourceRect, int xpos, int ypos, number_t scalex, number_t scaley)
{
	if (type=="full")
		LOG(LOG_NOT_IMPLEMENTED,"GradientBevelFilter type 'full'");
	uint8_t* tmpdata = nullptr;
	if (source)
		tmpdata = source->getRectangleData(sourceRect);
	else
		tmpdata = target->getRectangleData(sourceRect);

	number_t gradientalphas[256];
	uint32_t gradientcolors[256];
	fillGradientColors(gradientalphas,gradientcolors,this->ratios.getPtr(), this->alphas.getPtr(), this->colors.getPtr());
	applyBlur(tmpdata,sourceRect.Xmax-sourceRect.Xmin,sourceRect.Ymax-sourceRect.Ymin,blurX,blurY,quality,scalex,scaley);
	// TODO I've not found any useful documentation how BevelFilter should be implemented, so we just apply two dropShadowFilters with different angles
	applyGradientFilter(target, tmpdata, sourceRect, xpos+cos(angle+M_PI) * distance, ypos+sin(angle+M_PI) * distance, strength, gradientalphas, gradientcolors, type=="inner", knockout,scalex,scaley);
	applyGradientFilter(target, tmpdata, sourceRect, xpos+cos(angle     ) * distance, ypos+sin(angle     ) * distance, strength, gradientalphas, gradientcolors, type=="inner", knockout,scalex,scaley);
	delete[] tmpdata;
}

void GradientBevelFilter::prepareShutdown()
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

ASFUNCTIONBODY_ATOM(GradientBevelFilter,_constructor)
{
	GradientBevelFilter *th = asAtomHandler::as<GradientBevelFilter>(obj);
	ARG_CHECK(ARG_UNPACK(th->distance,4.0)(th->angle,45)(th->colors,NullRef)(th->alphas,NullRef)(th->ratios,NullRef)(th->blurX,4.0)(th->blurY,4.0)(th->strength,1)(th->quality,1)(th->type,"inner")(th->knockout,false));
}

BitmapFilter* GradientBevelFilter::cloneImpl() const
{
	GradientBevelFilter* cloned = Class<GradientBevelFilter>::getInstanceS(getInstanceWorker());
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

ShaderFilter::ShaderFilter(ASWorker* wrk,Class_base* c):
	BitmapFilter(wrk,c,SUBTYPE_SHADERFILTER)
{
}

void ShaderFilter::sinit(Class_base* c)
{
	CLASS_SETUP(c, BitmapFilter, _constructor, CLASS_SEALED | CLASS_FINAL);
}

void ShaderFilter::applyFilter(BitmapContainer* target, BitmapContainer* source, const RECT& sourceRect, int xpos, int ypos, number_t scalex, number_t scaley)
{
	LOG(LOG_NOT_IMPLEMENTED,"applyFilter for ShaderFilter");
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
