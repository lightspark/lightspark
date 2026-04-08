/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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

#include <fstream>
#include <cmath>
#include <algorithm>
#include "backends/geometry.h"
#include "compat.h"
#include "3rdparty/nanovg/src/nanovg.h"
#include "platforms/engineutils.h"
#include "swf.h"
#include "backends/rendering.h"


using namespace std;
using namespace lightspark;

tokensVector::~tokensVector()
{
	if (nvgctxt)
		nvgDeleteHitTest(nvgctxt);
}

void tokensVector::clear()
{
	filltokens.reset();
	stroketokens.reset();
	boundsRect = RECT(INT32_MAX,INT32_MIN,INT32_MAX,INT32_MIN);
}

void tokensVector::destruct()
{
	if (next)
		next->destruct();
	filltokens.reset();
	stroketokens.reset();
	if (nvgctxt)
		nvgDeleteHitTest(nvgctxt);
	nvgctxt=nullptr;
}

bool tokensVector::hitTest(SystemState* sys, const Vector2f& point, float scaling)
{
	if (empty())
		return false;
	const tokensVector* tkstart = this;
	const tokensVector* tk = this;
	if (!nvgctxt)
		nvgctxt = nvgCreateHitTest(0);
	if (!nvgctxt)
		return false;
	bool ret = false;
	nvgReset(nvgctxt);
	nvgResetTransform(nvgctxt);
	nvgHitTestBeginFrame(nvgctxt, sys->getRenderThread()->currentframebufferWidth, sys->getRenderThread()->currentframebufferHeight, 1.0);
	NVGcolor startcolor = nvgRGBA(1,1,1,1);
	nvgBeginPath(nvgctxt);
	nvgFillColor(nvgctxt,startcolor);
	nvgStrokeColor(nvgctxt,startcolor);
	number_t strokescalex=1.0;
	number_t strokescaley=1.0;
	bool instroke = false;
	bool infill = false;
	bool renderneeded=false;
	bool firstglyph=true;
	int tokentype = 1;
	int hitregionid = 1;
	bool firstmove=true;
	float x = point.x/scaling;
	float y = point.y/scaling;
	while (tokentype)
	{
		TokenList::const_iterator it;
		TokenList::const_iterator itbegin;
		TokenList::const_iterator itend;
		bool skip = false;
		switch(tokentype)
		{
			case 1:
				if (!tk->filltokens)
				{
					tokentype++;
					skip = true;
					break;
				}
				itbegin = tk->filltokens->tokens.begin();
				itend = tk->filltokens->tokens.end();
				it = tk->filltokens->tokens.begin();
				if (tk->isGlyph)
				{
					if (!firstglyph)
					{
						nvgClosePath(nvgctxt);
						ret = nvgHitTest(nvgctxt,x,y, NVG_TEST_ALL) >=0;
						nvgHitTestEndFrame(nvgctxt);
						if (ret)
							return ret;
						nvgHitTestBeginFrame(nvgctxt, sys->getRenderThread()->currentframebufferWidth, sys->getRenderThread()->currentframebufferHeight, 1.0);
					}
					firstglyph=false;
					infill=true;
					nvgResetTransform(nvgctxt);
					nvgTransform(nvgctxt,tk->startMatrix.xx,tk->startMatrix.yx,tk->startMatrix.xy,tk->startMatrix.yy,tk->startMatrix.x0,tk->startMatrix.y0);
				}
				if (tk->next)
					tk = tk->next;
				else
				{
					tk = tkstart;
					tokentype++;
				}
				break;
			case 2:
				if (!tk->stroketokens)
				{
					tokentype=0;
					break;
				}
				it = tk->stroketokens->tokens.begin();
				itbegin = tk->stroketokens->tokens.begin();
				itend = tk->stroketokens->tokens.end();
				if (tk->next)
					tk = tk->next;
				else
				{
					tk = tkstart;
					tokentype++;
				}
				break;
			default:
				tokentype = 0;
				break;
		}
		if (skip)
		{
			skip = false;
			tk = tkstart;
			continue;
		}
		if (tokentype == 0)
			break;
		while (it != itend && tokentype)
		{
			GeomToken p(*it,false);
			switch(p.type)
			{
				case MOVE:
				{
					GeomToken p1(*(++it),false);
					if (renderneeded && instroke && !infill)
					{
						nvgStrokeHitRegion(nvgctxt, hitregionid);
						renderneeded=false;
						nvgClosePath(nvgctxt);
						nvgBeginPath(nvgctxt);
					}
					firstmove=false;
					nvgMoveTo(nvgctxt, (p1.vec.x)*strokescalex, (p1.vec.y)*strokescaley);
					break;
				}
				case STRAIGHT:
				{
					if (firstmove)
						nvgMoveTo(nvgctxt, 0, 0);
					firstmove=false;
					renderneeded=true;
					GeomToken p1(*(++it),false);
					nvgLineTo(nvgctxt, (p1.vec.x)*strokescalex, (p1.vec.y)*strokescaley);
					break;
				}
				case CURVE_QUADRATIC:
				{
					if (firstmove)
						nvgMoveTo(nvgctxt, 0, 0);
					firstmove=false;
					renderneeded=true;
					GeomToken p1(*(++it),false);
					GeomToken p2(*(++it),false);
					nvgQuadTo(nvgctxt, (p1.vec.x)*strokescalex, (p1.vec.y)*strokescaley, (p2.vec.x)*strokescalex, (p2.vec.y)*strokescaley);
					break;
				}
				case CURVE_CUBIC:
				{
					if (firstmove)
						nvgMoveTo(nvgctxt, 0, 0);
					firstmove=false;
					renderneeded=true;
					GeomToken p1(*(++it),false);
					GeomToken p2(*(++it),false);
					GeomToken p3(*(++it),false);
					nvgBezierTo(nvgctxt, (p1.vec.x)*strokescalex, (p1.vec.y)*strokescaley, (p2.vec.x)*strokescalex, (p2.vec.y)*strokescaley, (p3.vec.x)*strokescalex, (p3.vec.y)*strokescaley);
					break;
				}
				case SET_FILL:
				{
					GeomToken p1(*(++it),false);
					if (renderneeded)
					{
						if (instroke)
							nvgStrokeHitRegion(nvgctxt, hitregionid);
						if (infill)
							nvgFillHitRegion(nvgctxt, hitregionid);
						renderneeded=false;
						nvgClosePath(nvgctxt);
						nvgBeginPath(nvgctxt);
					}
					if (strokescalex != 1.0)
					{
						nvgScale(nvgctxt,strokescalex,1.0);
						strokescalex = 1.0;
					}
					if (strokescaley != 1.0)
					{
						nvgScale(nvgctxt,2.0,strokescaley);
						strokescaley = 1.0;
					}
					infill=true;
					break;
				}
				case SET_STROKE:
				{
					GeomToken p1(*(++it),false);
					if (renderneeded)
					{
						if (instroke)
							nvgStrokeHitRegion(nvgctxt, hitregionid);
						if (infill)
							nvgFillHitRegion(nvgctxt, hitregionid);
						renderneeded=false;
						nvgClosePath(nvgctxt);
						nvgBeginPath(nvgctxt);
					}
					instroke = true;
					// TODO: EndCapStyle
					const LINESTYLE2* style = p1.lineStyle;
					if (style->StartCapStyle == 0)
						nvgLineCap(nvgctxt,NVG_ROUND);
					else if (style->StartCapStyle == 1)
						nvgLineCap(nvgctxt,NVG_BUTT);
					else if (style->StartCapStyle == 2)
						nvgLineCap(nvgctxt,NVG_SQUARE);
					if (style->JointStyle == 0)
						nvgLineJoin(nvgctxt, NVG_ROUND);
					else if (style->JointStyle == 1)
						nvgLineJoin(nvgctxt, NVG_BEVEL);
					else if (style->JointStyle == 2) {
						nvgLineJoin(nvgctxt, NVG_MITER);
						nvgMiterLimit(nvgctxt,style->MiterLimitFactor);
					}
					nvgStrokeWidth(nvgctxt,style->Width==0 ? 1 :(float)style->Width);
					if (style->NoHScaleFlag)
					{
						strokescalex=scaling;
						nvgScale(nvgctxt,1.0/strokescalex,1.0);
					}
					if (style->NoVScaleFlag)
					{
						strokescaley=scaling;
						nvgScale(nvgctxt,1.0,1.0/strokescaley);
					}
					break;
				}
				case CLEAR_FILL:
				case FILL_KEEP_SOURCE:
				{
					if (renderneeded)
					{
						if (instroke)
							nvgStrokeHitRegion(nvgctxt, hitregionid);
						if (infill)
							nvgFillHitRegion(nvgctxt, hitregionid);
						renderneeded=false;
						nvgClosePath(nvgctxt);
						nvgBeginPath(nvgctxt);
					}
					if (strokescalex != 1.0)
					{
						nvgScale(nvgctxt,strokescalex,1.0);
						strokescalex = 1.0;
					}
					if (strokescaley != 1.0)
					{
						nvgScale(nvgctxt,1.0,strokescaley);
						strokescaley = 1.0;
					}
					infill=false;
					break;
				}
				case CLEAR_STROKE:
					if (renderneeded)
					{
						if (instroke)
							nvgStrokeHitRegion(nvgctxt, hitregionid);
						if (infill)
							nvgFillHitRegion(nvgctxt, hitregionid);
						renderneeded=false;
						nvgClosePath(nvgctxt);
						nvgBeginPath(nvgctxt);
					}
					if (strokescalex != 1.0)
					{
						nvgScale(nvgctxt,strokescalex,1.0);
						strokescalex = 1.0;
					}
					if (strokescaley != 1.0)
					{
						nvgScale(nvgctxt,2.0,strokescaley);
						strokescaley = 1.0;
					}
					instroke = false;
					break;
				default:
					assert(false);
			}
			it++;
		}
	}
	if (renderneeded)
	{
		if (instroke)
			nvgStrokeHitRegion(nvgctxt, hitregionid);
		if (infill)
			nvgFillHitRegion(nvgctxt, hitregionid);
	}
	nvgClosePath(nvgctxt);
	ret = nvgHitTest(nvgctxt,x,y, NVG_TEST_ALL) >=0;
	nvgHitTestEndFrame(nvgctxt);

	return ret;
}

tokenListRef::~tokenListRef()
{
	if (fillStyles)
		delete fillStyles;
	if (lineStyles)
		delete lineStyles;
}

FILLSTYLE& tokenListRef::addFillStyle(const FILLSTYLE& fs)
{
	fillstylecache* cacheentry = new fillstylecache(fs);
	cacheentry->next=fillStyles;
	fillStyles=cacheentry;
	return cacheentry->style;
}

LINESTYLE2& tokenListRef::addLineStyle(const LINESTYLE2& ls)
{
	linestylecache* cacheentry = new linestylecache(ls);
	cacheentry->next=lineStyles;
	lineStyles=cacheentry;
	return cacheentry->style;
}

void tokenListRef::clone(tokenListRef* source)
{
	tokens.assign(source->tokens.begin(),source->tokens.end());
	fillstylecache* fs = source->fillStyles;
	while (fs)
	{
		addFillStyle(fs->style);
		fs = fs->next;
	}
	linestylecache* ls = source->lineStyles;
	while (ls)
	{
		addLineStyle(ls->style);
		ls = ls->next;
	}
}
