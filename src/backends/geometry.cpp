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
#include "swftypes.h"
#include "logger.h"
#include "backends/geometry.h"
#include "compat.h"
#include "scripting/flash/display/BitmapData.h"

using namespace std;
using namespace lightspark;

bool ShapesBuilder::isOutlineEmpty(const std::vector<ShapePathSegment>& outline)
{
	return outline.empty();
}

void ShapesBuilder::clear()
{
	filledShapesMap.clear();
	strokeShapesMap.clear();
}

static void mmremove(multimap<uint64_t, int>& map, uint64_t pos, int idx)
{
	auto itpair = map.equal_range(pos);
	while (itpair.first != itpair.second) {
		if (itpair.first->second == idx) {
			map.erase(itpair.first);
			return;
		}
		++itpair.first;
	}
}

static void joinOutlines(vector<ShapePathSegment>& segments)
{
	// Greedily reassemble paths from individual edges
	// For some malformed files, this is ambiguous, and the order of selection
	// of candidate edges matters.

	assert(!segments.empty());

	set<int> unconsumed;
	multimap<uint64_t, int> byStartPos;
	multimap<uint64_t, int> byEndPos;
	for (int i = 0; i < (int)segments.size(); ++i)
	{
		auto s = segments[i];
		unconsumed.insert(i);
		byStartPos.insert(make_pair(s.from.key, i));
		byEndPos.insert(make_pair(s.to.key, i));
	}

	vector<ShapePathSegment> res;
	int i = -1;
	ShapePathSegment prev;
	while (!unconsumed.empty())
	{
		bool reverse = false;
		if (i != -1)
		{
			multimap<uint64_t, int>::const_iterator it;
			auto next_it = unconsumed.upper_bound(i);
			if (next_it != unconsumed.end() && segments[*next_it].from.key == prev.to.key)
				i = *next_it;
			else if ((it = byStartPos.find(prev.to.key)) != byStartPos.end())
				i = it->second;
			else if ((it = byEndPos.find(prev.to.key)) != byEndPos.end()) {
				i = it->second;
				reverse = true;
			}
			else
				i = -1;
		}

		if (i == -1)
			i = *unconsumed.begin();

		prev = segments[i];
		mmremove(byStartPos, prev.from.key, i);
		mmremove(byEndPos, prev.to.key, i);
		if (reverse)
			prev = prev.reverse();
		res.push_back(prev);
		unconsumed.erase(i);
	}

	segments = res;
}

void ShapesBuilder::extendOutline(const Vector2f& v1, const Vector2f& v2, int linestyleindex)
{
	floatVec v1Index = makeVertex(v1);
	floatVec v2Index = makeVertex(v2);

	currentSubpath.emplace_back(v1Index, v2Index, v2Index, linestyleindex);
}

void ShapesBuilder::extendOutlineCurve(const Vector2f& v1, const Vector2f& v2, const Vector2f& v3, int linestyleindex)
{
	floatVec v1Index = makeVertex(v1);
	floatVec v2Index = makeVertex(v2);
	floatVec v3Index = makeVertex(v3);

	currentSubpath.emplace_back(v1Index, v2Index, v3Index, linestyleindex);
}

void ShapesBuilder::endSubpathForStyles(unsigned fill0, unsigned fill1, unsigned stroke, bool formorphing)
{
	if (currentSubpath.empty())
		return;

	// Seems important :
	// 'natural' order of edges in file affects output of path reconstruction
	if (fill0)
	{
		auto& segments = filledShapesMap[fill0];
		if (!formorphing || !segments.empty())
		{
			for (int i = currentSubpath.size()-1; i >= 0; --i) {
				segments.push_back(currentSubpath[i].reverse());
			}
		}
		else // it seems that for morphshapes the first subpath has to be added in "normal" order
			segments.insert(segments.end(), currentSubpath.begin(), currentSubpath.end());
	}

	if (fill1) {
		auto& segments = filledShapesMap[fill1];
		segments.insert(segments.end(), currentSubpath.begin(), currentSubpath.end());
	}

	if (stroke && !fill0 && !fill1) {
		auto& segments = strokeShapesMap[stroke];
		segments.insert(segments.end(), currentSubpath.begin(), currentSubpath.end());
	}

	currentSubpath.clear();
}

void ShapesBuilder::outputTokens(const std::list<FILLSTYLE> &styles, const std::list<LINESTYLE2> &linestyles, tokensVector& tokens)
{
	assert(currentSubpath.empty());
	auto it=filledShapesMap.begin();
	//For each color
	for(;it!=filledShapesMap.end();++it)
	{
		joinOutlines(it->second);

		//Find the style given the index
		auto stylesIt=styles.begin();
		assert(it->first);
		for(unsigned int i=0;i<it->first-1;i++)
		{
			++stylesIt;
			assert(stylesIt!=styles.end());
		}
		//Set the fill style
		tokens.filltokens.push_back(GeomToken(SET_FILL).uval);
		tokens.filltokens.push_back(GeomToken(*stylesIt).uval);
		vector<ShapePathSegment>& segments = it->second;
		for (size_t j = 0; j < segments.size(); ++j)
		{
			ShapePathSegment segment = segments[j];
			if (j == 0 || segment.from.key != segments[j-1].to.key) {
				tokens.filltokens.push_back(GeomToken(MOVE).uval);
				tokens.filltokens.push_back(GeomToken(segment.from).uval);
			}
			if (segment.quadctrl.key == segment.from.key || segment.quadctrl.key == segment.to.key) {
				tokens.filltokens.push_back(GeomToken(STRAIGHT).uval);
				tokens.filltokens.push_back(GeomToken(segment.to).uval);
			}
			else {
				tokens.filltokens.push_back(GeomToken(CURVE_QUADRATIC).uval);
				tokens.filltokens.push_back(GeomToken(segment.quadctrl).uval);
				tokens.filltokens.push_back(GeomToken(segment.to).uval);
			}
		}
		tokens.filltokens.push_back(GeomToken(CLEAR_FILL).uval);
		int currentlinestyle=0;
		// add tokens for strokes intertwined with current fill
		for (size_t j = 0; j < segments.size(); ++j)
		{
			ShapePathSegment segment = segments[j];
			
			if (segment.linestyleindex != currentlinestyle)
			{
				currentlinestyle = segment.linestyleindex;
				if (segment.linestyleindex)
				{
					auto linestylesIt=linestyles.begin();
					for(int i=0;i<segment.linestyleindex-1;i++)
					{
						++linestylesIt;
						assert(linestylesIt!=linestyles.end());
					}
					//Set the line style for strokes inside filltokens
					tokens.filltokens.push_back(GeomToken(SET_STROKE).uval);
					tokens.filltokens.push_back(GeomToken(*linestylesIt).uval);
					if (tokens.currentLineWidth < (*linestylesIt).Width)
						tokens.currentLineWidth = (*linestylesIt).Width;
					for (size_t k = j; k < segments.size(); ++k)
					{
						ShapePathSegment strokesegment = segments[k];
						if (strokesegment.linestyleindex != currentlinestyle)
						{
							j=k;
							tokens.filltokens.push_back(GeomToken(CLEAR_STROKE).uval);
							break;
						}
						if (k == j || strokesegment.from.key != segments[k-1].to.key) {
							tokens.filltokens.push_back(GeomToken(MOVE).uval);
							tokens.filltokens.push_back(GeomToken(strokesegment.from).uval);
						}
						if (strokesegment.quadctrl.key == strokesegment.from.key || strokesegment.quadctrl.key == strokesegment.to.key) {
							tokens.filltokens.push_back(GeomToken(STRAIGHT).uval);
							tokens.filltokens.push_back(GeomToken(strokesegment.to).uval);
						}
						else {
							tokens.filltokens.push_back(GeomToken(CURVE_QUADRATIC).uval);
							tokens.filltokens.push_back(GeomToken(strokesegment.quadctrl).uval);
							tokens.filltokens.push_back(GeomToken(strokesegment.to).uval);
						}
					}
				}
			}
		}
		if (currentlinestyle)
			tokens.filltokens.push_back(GeomToken(CLEAR_STROKE).uval);
	}
	if (strokeShapesMap.size() > 0)
	{
		it=strokeShapesMap.begin();
		//For each stroke
		for(;it!=strokeShapesMap.end();++it)
		{
			joinOutlines(it->second);

			//Find the style given the index
			auto stylesIt=linestyles.begin();
			assert(it->first);
			for(unsigned int i=0;i<it->first-1;i++)
			{
				++stylesIt;
				assert(stylesIt!=linestyles.end());
			}
			//Set the line style
			vector<ShapePathSegment>& segments = it->second;
			tokens.stroketokens.push_back(GeomToken(SET_STROKE).uval);
			tokens.stroketokens.push_back(GeomToken(*stylesIt).uval);
			if (tokens.currentLineWidth < (*stylesIt).Width)
				tokens.currentLineWidth = (*stylesIt).Width;
			for (size_t j = 0; j < segments.size(); ++j)
			{
				ShapePathSegment segment = segments[j];
				if (j == 0 || segment.from.key != segments[j - 1].to.key) {
					tokens.stroketokens.push_back(GeomToken(MOVE).uval);
					tokens.stroketokens.push_back(GeomToken(segment.from).uval);
				}
				if (segment.quadctrl.key == segment.from.key || segment.quadctrl.key == segment.to.key) {
					tokens.stroketokens.push_back(GeomToken(STRAIGHT).uval);
					tokens.stroketokens.push_back(GeomToken(segment.to).uval);
				}
				else {
					tokens.stroketokens.push_back(GeomToken(CURVE_QUADRATIC).uval);
					tokens.stroketokens.push_back(GeomToken(segment.quadctrl).uval);
					tokens.stroketokens.push_back(GeomToken(segment.to).uval);
				}
			}
		}
	}
}
std::map<uint16_t,LINESTYLE2>::iterator ShapesBuilder::getStrokeLineStyle(const std::list<MORPHLINESTYLE2>::iterator& stylesIt, uint16_t ratio,std::map<uint16_t,LINESTYLE2>* linestylecache, const RECT& boundsrc)
{
	auto itls =linestylecache->find(ratio);
	if (itls == linestylecache->end())
	{
		LINESTYLE2 ls(0xff);
		ls.FillType.FillStyleType=stylesIt->FillType.FillStyleType;
		ls.StartCapStyle = stylesIt->StartCapStyle;
		ls.JointStyle = stylesIt->JoinStyle;
		ls.HasFillFlag = stylesIt->HasFillFlag;
		ls.NoHScaleFlag = stylesIt->NoHScaleFlag;
		ls.NoVScaleFlag = stylesIt->NoVScaleFlag;
		ls.PixelHintingFlag = stylesIt->PixelHintingFlag;
		ls.NoClose = stylesIt->NoClose;
		ls.EndCapStyle = stylesIt->EndCapStyle;
		ls.MiterLimitFactor = stylesIt->MiterLimitFactor;
		ls.Width = stylesIt->StartWidth + (stylesIt->EndWidth - stylesIt->StartWidth)*(number_t(ratio)/65535.0);
		uint8_t compratio = ratio>>8;
		if (stylesIt->HasFillFlag)
		{
			switch (stylesIt->FillType.FillStyleType)
			{
				case LINEAR_GRADIENT:
				case RADIAL_GRADIENT:
				case FOCAL_RADIAL_GRADIENT:
					{
						number_t gradratio = float(ratio)/65535.0;
						MATRIX ratiomatrix;
						
						ratiomatrix.x0=stylesIt->FillType.StartGradientMatrix.x0*(1.0-gradratio)+stylesIt->FillType.EndGradientMatrix.x0*gradratio;
						ratiomatrix.y0=stylesIt->FillType.StartGradientMatrix.y0*(1.0-gradratio)+stylesIt->FillType.EndGradientMatrix.y0*gradratio;
						ratiomatrix.xx=stylesIt->FillType.StartGradientMatrix.xx*(1.0-gradratio)+stylesIt->FillType.EndGradientMatrix.xx*gradratio;
						ratiomatrix.yx=stylesIt->FillType.StartGradientMatrix.yx*(1.0-gradratio)+stylesIt->FillType.EndGradientMatrix.yx*gradratio;
						ratiomatrix.xy=stylesIt->FillType.StartGradientMatrix.xy*(1.0-gradratio)+stylesIt->FillType.EndGradientMatrix.xy*gradratio;
						ratiomatrix.yy=stylesIt->FillType.StartGradientMatrix.yy*(1.0-gradratio)+stylesIt->FillType.EndGradientMatrix.yy*gradratio;
						
						ls.FillType.Matrix = ratiomatrix;
						std::vector<GRADRECORD>& gradrecords = stylesIt->FillType.FillStyleType==FOCAL_RADIAL_GRADIENT ? ls.FillType.FocalGradient.GradientRecords : ls.FillType.Gradient.GradientRecords;
						gradrecords.reserve(stylesIt->FillType.StartColors.size());
						GRADRECORD gr(0xff);
						for (uint32_t i1=0; i1 < stylesIt->FillType.StartColors.size(); i1++)
						{
							switch (compratio)
							{
								case 0:
									gr.Color = stylesIt->FillType.StartColors[i1];
									gr.Ratio = stylesIt->FillType.StartRatios[i1];
									break;
								case 0xff:
									gr.Color = stylesIt->FillType.EndColors[i1];
									gr.Ratio = stylesIt->FillType.EndRatios[i1];
									break;
								default:
									{
										gr.Color.Red = stylesIt->FillType.StartColors[i1].Red + ((int32_t)ratio * ((int32_t)stylesIt->FillType.EndColors[i1].Red-(int32_t)stylesIt->FillType.StartColors[i1].Red)/(int32_t)UINT16_MAX);
										gr.Color.Green = stylesIt->FillType.StartColors[i1].Green + ((int32_t)ratio * ((int32_t)stylesIt->FillType.EndColors[i1].Green-(int32_t)stylesIt->FillType.StartColors[i1].Green)/(int32_t)UINT16_MAX);
										gr.Color.Blue = stylesIt->FillType.StartColors[i1].Blue + ((int32_t)ratio * ((int32_t)stylesIt->FillType.EndColors[i1].Blue-(int32_t)stylesIt->FillType.StartColors[i1].Blue)/(int32_t)UINT16_MAX);
										gr.Color.Alpha = stylesIt->FillType.StartColors[i1].Alpha + ((int32_t)ratio * ((int32_t)stylesIt->FillType.EndColors[i1].Alpha-(int32_t)stylesIt->FillType.StartColors[i1].Alpha)/(int32_t)UINT16_MAX);
										uint8_t diff = stylesIt->FillType.EndRatios[i1]-stylesIt->FillType.StartRatios[i1];
										gr.Ratio = stylesIt->FillType.StartRatios[i1] + (diff/compratio);
										break;
									}
							}
							ls.FillType.FocalGradient.InterpolationMode=stylesIt->FillType.InterpolationMode;
							ls.FillType.FocalGradient.SpreadMode=stylesIt->FillType.SpreadMode;
							ls.FillType.FocalGradient.FocalPoint=stylesIt->FillType.StartFocalPoint + (stylesIt->FillType.EndFocalPoint-stylesIt->FillType.StartFocalPoint)*gradratio;
							gradrecords.push_back(gr);
							ls.FillType.ShapeBounds = boundsrc;
						}
						break;
					}
				case SOLID_FILL:
					{
						switch (compratio)
						{
							case 0:
								ls.FillType.Color = stylesIt->StartColor;
								break;
							case 0xff:
								ls.FillType.Color = stylesIt->EndColor;
								break;
							default:
								{
									ls.FillType.Color.Red = stylesIt->StartColor.Red + ((int32_t)ratio * ((int32_t)stylesIt->EndColor.Red-(int32_t)stylesIt->StartColor.Red)/(int32_t)UINT16_MAX);
									ls.FillType.Color.Green = stylesIt->StartColor.Green + ((int32_t)ratio * ((int32_t)stylesIt->EndColor.Green-(int32_t)stylesIt->StartColor.Green)/(int32_t)UINT16_MAX);
									ls.FillType.Color.Blue = stylesIt->StartColor.Blue + ((int32_t)ratio * ((int32_t)stylesIt->EndColor.Blue-(int32_t)stylesIt->StartColor.Blue)/(int32_t)UINT16_MAX);
									ls.FillType.Color.Alpha = stylesIt->EndColor.Alpha + ((int32_t)ratio * ((int32_t)stylesIt->EndColor.Alpha-(int32_t)stylesIt->StartColor.Alpha)/(int32_t)UINT16_MAX);
									break;
								}
						}
						ls.FillType.ShapeBounds = boundsrc;
						break;
					}
				default:
					LOG(LOG_NOT_IMPLEMENTED,"morphing for line style type:"<<hex<<stylesIt->FillType.FillStyleType);
					break;
			}
		}
		else
		{
			switch (compratio)
			{
				case 0:
					ls.FillType.Color = stylesIt->StartColor;
					break;
				case 0xff:
					ls.FillType.Color = stylesIt->EndColor;
					break;
				default:
					{
						ls.FillType.Color.Red = stylesIt->StartColor.Red + ((int32_t)ratio * ((int32_t)stylesIt->EndColor.Red-(int32_t)stylesIt->StartColor.Red)/(int32_t)UINT16_MAX);
						ls.FillType.Color.Green = stylesIt->StartColor.Green + ((int32_t)ratio * ((int32_t)stylesIt->EndColor.Green-(int32_t)stylesIt->StartColor.Green)/(int32_t)UINT16_MAX);
						ls.FillType.Color.Blue = stylesIt->StartColor.Blue + ((int32_t)ratio * ((int32_t)stylesIt->EndColor.Blue-(int32_t)stylesIt->StartColor.Blue)/(int32_t)UINT16_MAX);
						ls.FillType.Color.Alpha = stylesIt->EndColor.Alpha + ((int32_t)ratio * ((int32_t)stylesIt->EndColor.Alpha-(int32_t)stylesIt->StartColor.Alpha)/(int32_t)UINT16_MAX);
						break;
					}
			}
			ls.Color = ls.FillType.Color;
		}
		itls = stylesIt->linestylecache.insert(make_pair(ratio,ls)).first;
	}
	return itls;
}
void ShapesBuilder::outputMorphTokens(std::list<MORPHFILLSTYLE>& styles, std::list<MORPHLINESTYLE2> &linestyles, tokensVector &tokens, uint16_t ratio, const RECT& boundsrc)
{
//	assert(currentSubpath.empty());
	auto it=filledShapesMap.begin();
	//For each color
	for(;it!=filledShapesMap.end();++it)
	{
		joinOutlines(it->second);

		//Find the style given the index
		auto stylesIt=styles.begin();
		assert(it->first);
		for(unsigned int i=0;i<it->first-1;i++)
		{
			++stylesIt;
			assert(stylesIt!=styles.end());
		}
		//Set the fill style
		auto itfs =stylesIt->fillstylecache.find(ratio);
		if (itfs == stylesIt->fillstylecache.end())
		{
			FILLSTYLE f = *stylesIt;
			switch (stylesIt->FillStyleType)
			{
				case LINEAR_GRADIENT:
				case RADIAL_GRADIENT:
				case FOCAL_RADIAL_GRADIENT:
				{
					number_t gradratio = float(ratio)/65535.0;
					MATRIX ratiomatrix;
					
					ratiomatrix.x0=stylesIt->StartGradientMatrix.x0*(1.0-gradratio)+stylesIt->EndGradientMatrix.x0*gradratio;
					ratiomatrix.y0=stylesIt->StartGradientMatrix.y0*(1.0-gradratio)+stylesIt->EndGradientMatrix.y0*gradratio;
					ratiomatrix.xx=stylesIt->StartGradientMatrix.xx*(1.0-gradratio)+stylesIt->EndGradientMatrix.xx*gradratio;
					ratiomatrix.yx=stylesIt->StartGradientMatrix.yx*(1.0-gradratio)+stylesIt->EndGradientMatrix.yx*gradratio;
					ratiomatrix.xy=stylesIt->StartGradientMatrix.xy*(1.0-gradratio)+stylesIt->EndGradientMatrix.xy*gradratio;
					ratiomatrix.yy=stylesIt->StartGradientMatrix.yy*(1.0-gradratio)+stylesIt->EndGradientMatrix.yy*gradratio;

					f.Matrix = ratiomatrix;
					std::vector<GRADRECORD>& gradrecords = stylesIt->FillStyleType==FOCAL_RADIAL_GRADIENT ? f.FocalGradient.GradientRecords : f.Gradient.GradientRecords;
					gradrecords.reserve(stylesIt->StartColors.size());
					GRADRECORD gr(0xff);
					uint8_t compratio = ratio>>8;
					for (uint32_t i1=0; i1 < stylesIt->StartColors.size(); i1++)
					{
						switch (compratio)
						{
							case 0:
								gr.Color = stylesIt->StartColors[i1];
								gr.Ratio = stylesIt->StartRatios[i1];
								break;
							case 0xff:
								gr.Color = stylesIt->EndColors[i1];
								gr.Ratio = stylesIt->EndRatios[i1];
								break;
							default:
							{
								gr.Color.Red = stylesIt->StartColors[i1].Red + ((int32_t)ratio * ((int32_t)stylesIt->EndColors[i1].Red-(int32_t)stylesIt->StartColors[i1].Red)/(int32_t)UINT16_MAX);
								gr.Color.Green = stylesIt->StartColors[i1].Green + ((int32_t)ratio * ((int32_t)stylesIt->EndColors[i1].Green-(int32_t)stylesIt->StartColors[i1].Green)/(int32_t)UINT16_MAX);
								gr.Color.Blue = stylesIt->StartColors[i1].Blue + ((int32_t)ratio * ((int32_t)stylesIt->EndColors[i1].Blue-(int32_t)stylesIt->StartColors[i1].Blue)/(int32_t)UINT16_MAX);
								gr.Color.Alpha = stylesIt->StartColors[i1].Alpha + ((int32_t)ratio * ((int32_t)stylesIt->EndColors[i1].Alpha-(int32_t)stylesIt->StartColors[i1].Alpha)/(int32_t)UINT16_MAX);
								uint8_t diff = stylesIt->EndRatios[i1]-stylesIt->StartRatios[i1];
								gr.Ratio = stylesIt->StartRatios[i1] + (diff/compratio);
								break;
							}
						}
						f.FocalGradient.InterpolationMode=stylesIt->InterpolationMode;
						f.FocalGradient.SpreadMode=stylesIt->SpreadMode;
						f.FocalGradient.FocalPoint=stylesIt->StartFocalPoint + (stylesIt->EndFocalPoint-stylesIt->StartFocalPoint)*gradratio;
						gradrecords.push_back(gr);
						f.ShapeBounds = boundsrc;
					}
					break;
				}
				case SOLID_FILL:
				{
					uint8_t compratio = ratio>>8;
					switch (compratio)
					{
						case 0:
							f.Color = stylesIt->StartColor;
							break;
						case 0xff:
							f.Color = stylesIt->EndColor;
							break;
						default:
						{
							f.Color.Red = stylesIt->StartColor.Red + ((int32_t)ratio * ((int32_t)stylesIt->EndColor.Red-(int32_t)stylesIt->StartColor.Red)/(int32_t)UINT16_MAX);
							f.Color.Green = stylesIt->StartColor.Green + ((int32_t)ratio * ((int32_t)stylesIt->EndColor.Green-(int32_t)stylesIt->StartColor.Green)/(int32_t)UINT16_MAX);
							f.Color.Blue = stylesIt->StartColor.Blue + ((int32_t)ratio * ((int32_t)stylesIt->EndColor.Blue-(int32_t)stylesIt->StartColor.Blue)/(int32_t)UINT16_MAX);
							f.Color.Alpha = stylesIt->EndColor.Alpha + ((int32_t)ratio * ((int32_t)stylesIt->EndColor.Alpha-(int32_t)stylesIt->StartColor.Alpha)/(int32_t)UINT16_MAX);
							break;
						}
					}
					f.ShapeBounds = boundsrc;
					break;
				}
				default:
					LOG(LOG_NOT_IMPLEMENTED,"morphing for fill style type:"<<stylesIt->FillStyleType);
					break;
			}
			itfs = stylesIt->fillstylecache.insert(make_pair(ratio,f)).first;
		}
		tokens.filltokens.push_back(GeomToken(SET_FILL).uval);
		tokens.filltokens.push_back(GeomToken((*itfs).second).uval);
		vector<ShapePathSegment>& segments = it->second;
		for (size_t j = 0; j < segments.size(); ++j)
		{
			ShapePathSegment segment = segments[j];
			if (j == 0 || segment.from.key != segments[j - 1].to.key) {
				tokens.filltokens.push_back(GeomToken(MOVE).uval);
				tokens.filltokens.push_back(GeomToken(segment.from).uval);
			}
			if (segment.quadctrl.key == segment.from.key || segment.quadctrl.key == segment.to.key) {
				tokens.filltokens.push_back(GeomToken(STRAIGHT).uval);
				tokens.filltokens.push_back(GeomToken(segment.to).uval);
			}
			else {
				tokens.filltokens.push_back(GeomToken(CURVE_QUADRATIC).uval);
				tokens.filltokens.push_back(GeomToken(segment.quadctrl).uval);
				tokens.filltokens.push_back(GeomToken(segment.to).uval);
			}
		}
		tokens.filltokens.push_back(GeomToken(CLEAR_FILL).uval);
		// add tokens for strokes intertwined with current fill
		int currentlinestyle=0;
		for (size_t j = 0; j < segments.size(); ++j)
		{
			ShapePathSegment segment = segments[j];
			if (segment.linestyleindex != currentlinestyle)
			{
				currentlinestyle = segment.linestyleindex;
				if (segment.linestyleindex)
				{
					std::list<MORPHLINESTYLE2>::iterator linestylesIt=linestyles.begin();
					for(int i=0;i<segment.linestyleindex-1;i++)
					{
						++linestylesIt;
						assert(linestylesIt!=linestyles.end());
					}
					auto itls = getStrokeLineStyle(linestylesIt,ratio,&linestylesIt->linestylecache,boundsrc);
					//Set the line style for strokes inside filltokens
					tokens.filltokens.push_back(GeomToken(SET_STROKE).uval);
					tokens.filltokens.push_back(GeomToken((*itls).second).uval);
					if (tokens.currentLineWidth < (*itls).second.Width)
						tokens.currentLineWidth = (*itls).second.Width;
					for (size_t k = j; k < segments.size(); ++k)
					{
						ShapePathSegment strokesegment = segments[k];
						if (strokesegment.linestyleindex != currentlinestyle)
						{
							j=k;
							tokens.filltokens.push_back(GeomToken(CLEAR_STROKE).uval);
							break;
						}
						if (k == j || strokesegment.from.key != segments[k-1].to.key) {
							tokens.filltokens.push_back(GeomToken(MOVE).uval);
							tokens.filltokens.push_back(GeomToken(strokesegment.from).uval);
						}
						if (strokesegment.quadctrl.key == strokesegment.from.key || strokesegment.quadctrl.key == strokesegment.to.key) {
							tokens.filltokens.push_back(GeomToken(STRAIGHT).uval);
							tokens.filltokens.push_back(GeomToken(strokesegment.to).uval);
						}
						else {
							tokens.filltokens.push_back(GeomToken(CURVE_QUADRATIC).uval);
							tokens.filltokens.push_back(GeomToken(strokesegment.quadctrl).uval);
							tokens.filltokens.push_back(GeomToken(strokesegment.to).uval);
						}
					}
				}
			}
		}
		if (currentlinestyle)
			tokens.filltokens.push_back(GeomToken(CLEAR_STROKE).uval);
	}
	if (strokeShapesMap.size() > 0)
	{
		it=strokeShapesMap.begin();
		//For each stroke
		for(;it!=strokeShapesMap.end();++it)
		{
			joinOutlines(it->second);
			//Find the style given the index
			std::list<MORPHLINESTYLE2>::iterator stylesIt=linestyles.begin();
			assert(it->first);
			for(unsigned int i=0;i<it->first-1;i++)
			{
				++stylesIt;
				assert(stylesIt!=linestyles.end());
			}
			//Set the line style
			auto itls = getStrokeLineStyle(stylesIt,ratio,&stylesIt->linestylecache,boundsrc);
			vector<ShapePathSegment>& segments = it->second;
			tokens.stroketokens.push_back(GeomToken(SET_STROKE).uval);
			tokens.stroketokens.push_back(GeomToken((*itls).second).uval);
			if (tokens.currentLineWidth < (*itls).second.Width)
				tokens.currentLineWidth = (*itls).second.Width;
			for (size_t j = 0; j < segments.size(); ++j)
			{
				ShapePathSegment segment = segments[j];
				if (j == 0 || segment.from.key != segments[j - 1].to.key) {
					tokens.stroketokens.push_back(GeomToken(MOVE).uval);
					tokens.stroketokens.push_back(GeomToken(segment.from).uval);
				}
				if (segment.quadctrl.key == segment.from.key || segment.quadctrl.key == segment.to.key) {
					tokens.stroketokens.push_back(GeomToken(STRAIGHT).uval);
					tokens.stroketokens.push_back(GeomToken(segment.to).uval);
				}
				else {
					tokens.stroketokens.push_back(GeomToken(CURVE_QUADRATIC).uval);
					tokens.stroketokens.push_back(GeomToken(segment.quadctrl).uval);
					tokens.stroketokens.push_back(GeomToken(segment.to).uval);
				}
			}
		}
	}
}

void tokensVector::updateTokenBounds(int x, int y)
{
	if (x < boundsRect.Xmin)
		boundsRect.Xmin=x;
	if (x > boundsRect.Xmax)
		boundsRect.Xmax=x;
	if (y < boundsRect.Ymin)
		boundsRect.Ymin=y;
	if (y > boundsRect.Ymax)
		boundsRect.Ymax=y;
}

bool tokensVector::operator==(const tokensVector& r)
{
	return currentLineWidth == r.currentLineWidth && boundsRect == r.boundsRect && filltokens == r.filltokens && stroketokens == r.stroketokens;
}

tokensVector& tokensVector::operator=(const tokensVector& r)
{
	boundsRect = r.boundsRect;
	currentLineWidth = r.currentLineWidth;
	filltokens = r.filltokens;
	stroketokens = r.stroketokens;
	return *this;
}
