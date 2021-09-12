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

void BitmapFilter::applyFilter(BitmapContainer* target, BitmapContainer* source, const RECT& sourceRect, int xpos, int ypos)
{
	LOG(LOG_NOT_IMPLEMENTED,"applyFilter for "<<this->toDebugString());
}

BitmapFilter* BitmapFilter::cloneImpl() const
{
	return Class<BitmapFilter>::getInstanceS(getSystemState());
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
	number_t f;

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
				for (y = 0; i<h; i++)
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
						f = 255.0 / number_t(pa);
						pr = int((uint32_t(r * ms) >> ss) * f);
						pg = int((uint32_t(g * ms) >> ss) * f);
						pb = int((uint32_t(b * ms) >> ss) * f);
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

void BitmapFilter::applyDropShadowFilter(BitmapContainer* target, BitmapContainer* source, const RECT& sourceRect, int xpos, int ypos, number_t blurx, number_t blury, int quality, number_t strength, number_t alpha, uint32_t color, bool inner, bool knockout)
{
	uint32_t width = sourceRect.Xmax-sourceRect.Xmin;
	uint32_t height = sourceRect.Ymax-sourceRect.Ymin;
	uint32_t size = width*height;
	uint8_t* tmpdata = nullptr;
	if (source)
		tmpdata = source->getRectangleData(sourceRect);
	else
		tmpdata = target->getRectangleData(sourceRect);

	applyBlur(tmpdata,width,height,blurx,blury,quality);
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
	delete[] tmpdata;
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
void GlowFilter::applyFilter(BitmapContainer* target, BitmapContainer* source, const RECT& sourceRect, int xpos, int ypos)
{
	applyDropShadowFilter(target, source, sourceRect, xpos, ypos, blurX, blurY, quality, strength, alpha, color, inner, knockout);
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
}
void DropShadowFilter::applyFilter(BitmapContainer* target, BitmapContainer* source, const RECT& sourceRect, int xpos, int ypos)
{
	xpos += cos(angle*M_PI/180.0) * distance;
	ypos +=	sin(angle*M_PI/180.0) * distance;
	applyDropShadowFilter(target, source, sourceRect, xpos, ypos, blurX, blurY, quality, strength, alpha, color, inner, knockout);
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

void ColorMatrixFilter::applyFilter(BitmapContainer* target, BitmapContainer* source, const RECT& sourceRect, int xpos, int ypos)
{
	LOG(LOG_NOT_IMPLEMENTED,"applyFilter on ColorMatrixFilter");
//	assert_and_throw(matrix->size() >= 20);
//	for (int32_t y = sourceRect.Ymin; y < sourceRect.Ymax; y++)
//	{
//		for (int32_t x = sourceRect.Xmin; x < sourceRect.Xmax; x++)
//		{
//			uint32_t sourcecolor= source ? source->getPixel(x,y) : target->getPixel(x,y);
//			number_t srcR = number_t((sourcecolor    )&0xff)/256.0;
//			number_t srcG = number_t((sourcecolor>> 8)&0xff)/256.0;
//			number_t srcB = number_t((sourcecolor>>16)&0xff)/256.0;
//			number_t srcA = number_t((sourcecolor>>24)&0xff)/256.0;
//			number_t redResult   = (asAtomHandler::toNumber(matrix->at(0))  * srcR) + (asAtomHandler::toNumber(matrix->at(1))  * srcG) + (asAtomHandler::toNumber(matrix->at(2))  * srcB) + (asAtomHandler::toNumber(matrix->at(3))  * srcA) + asAtomHandler::toNumber(matrix->at(4) );
//			number_t greenResult = (asAtomHandler::toNumber(matrix->at(5))  * srcR) + (asAtomHandler::toNumber(matrix->at(6))  * srcG) + (asAtomHandler::toNumber(matrix->at(7))  * srcB) + (asAtomHandler::toNumber(matrix->at(8))  * srcA) + asAtomHandler::toNumber(matrix->at(9) );
//			number_t blueResult  = (asAtomHandler::toNumber(matrix->at(10)) * srcR) + (asAtomHandler::toNumber(matrix->at(11)) * srcG) + (asAtomHandler::toNumber(matrix->at(12)) * srcB) + (asAtomHandler::toNumber(matrix->at(13)) * srcA) + asAtomHandler::toNumber(matrix->at(14));
//			number_t alphaResult = (asAtomHandler::toNumber(matrix->at(15)) * srcR) + (asAtomHandler::toNumber(matrix->at(16)) * srcG) + (asAtomHandler::toNumber(matrix->at(17)) * srcB) + (asAtomHandler::toNumber(matrix->at(18)) * srcA) + asAtomHandler::toNumber(matrix->at(19));
//			target->setPixel(xpos+x,ypos+y,
//							 (max(uint32_t(0),min(uint32_t(0xff),uint32_t(redResult  *255.0)%0xff)    ))|
//							 (max(uint32_t(0),min(uint32_t(0xff),uint32_t(greenResult*255.0)%0xff)<<8 ))|
//							 (max(uint32_t(0),min(uint32_t(0xff),uint32_t(blueResult *255.0)%0xff)<<16))|
//							 (max(uint32_t(0),min(uint32_t(0xff),uint32_t(alphaResult*255.0)%0xff)<<24)),true);
//		}
//	}
}

ASFUNCTIONBODY_GETTER_SETTER(ColorMatrixFilter, matrix);

ASFUNCTIONBODY_ATOM(ColorMatrixFilter,_constructor)
{
	ColorMatrixFilter *th = asAtomHandler::as<ColorMatrixFilter>(obj);
	ARG_UNPACK_ATOM(th->matrix,NullRef);
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
}
void BlurFilter::applyFilter(BitmapContainer* target, BitmapContainer* source, const RECT& sourceRect, int xpos, int ypos)
{
	uint32_t width = sourceRect.Xmax-sourceRect.Xmin;
	uint32_t height = sourceRect.Ymax-sourceRect.Ymin;
	uint8_t* tmpdata = nullptr;
	if (source)
		tmpdata = source->getRectangleData(sourceRect);
	else
		tmpdata = target->getRectangleData(sourceRect);
	applyBlur(tmpdata,width,height,blurX,blurY,quality);
	uint8_t* data = target->getData();
	for (uint32_t i = 0; i < height; i++)
	{
		memcpy(data+((ypos+i)*width+xpos)*4, tmpdata+i*width*4,width*4);
	}
	delete[] tmpdata;
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
