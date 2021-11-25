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
#include "scripting/flash/display/flashdisplay.h"
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

void ShapesBuilder::joinOutlines()
{
	auto it=filledShapesMap.begin();
	for(;it!=filledShapesMap.end();++it)
	{
		vector< vector<ShapePathSegment> >& outlinesForColor=it->second;
		//Repack outlines of the same color, avoiding excessive copying
		for(int i=0;i<int(outlinesForColor.size());i++)
		{
			assert_and_throw(outlinesForColor[i].size()>=2);
			//Already closed paths are ok
			if(outlinesForColor[i].front()==outlinesForColor[i].back())
				continue;
			for(int j=outlinesForColor.size()-1;j>=0;j--)
			{
				if(j==i || outlinesForColor[j].empty())
					continue;
				if(outlinesForColor[i].front()==outlinesForColor[j].back())
				{
					//Copy all the vertex but the origin in this one
					outlinesForColor[j].insert(outlinesForColor[j].end(),
									outlinesForColor[i].begin()+1,
									outlinesForColor[i].end());
					//Invalidate the origin, but not the high level vector
					outlinesForColor[i].clear();
					break;
				}
				else if(outlinesForColor[i].back()==outlinesForColor[j].back())
				{
					//CHECK: this works for adjacent shapes of the same color?
					//Copy all the vertex but the origin in this one
					for (int k = outlinesForColor[i].size()-2; k >= 0; k--)
						outlinesForColor[j].emplace_back(ShapePathSegment(outlinesForColor[i][k+1].type, outlinesForColor[i][k].i));
					//Invalidate the origin, but not the high level vector
					outlinesForColor[i].clear();
					break;
				}
			}
		}
		
		//Kill all the empty outlines
		outlinesForColor.erase(remove_if(outlinesForColor.begin(),outlinesForColor.end(), isOutlineEmpty),
				       outlinesForColor.end());
	}
}

std::vector<ShapePathSegment>* ShapesBuilder::extendOutline(std::vector< std::vector<ShapePathSegment> >* outlines, const Vector2& v1, const Vector2& v2,std::vector<ShapePathSegment>* lastOutline)
{
	assert_and_throw(outlines);
	//Find the indices of the vertex
	uint64_t v1Index = makeVertex(v1);
	uint64_t v2Index = makeVertex(v2);
	if (lastOutline)
	{
		lastOutline->push_back(ShapePathSegment(PATH_STRAIGHT, v2Index));
		return lastOutline;
	}

	//Search a suitable outline to attach this new vertex
	auto it = outlines->rbegin();
	while (it !=outlines->rend())
	{
		assert(it->size()>=2);
		if(it->front()==it->back())
		{
			++it;
			continue;
		}
		if(it->back().i==v1Index)
		{
			it->push_back(ShapePathSegment(PATH_STRAIGHT, v2Index));
			return &(*it);
		}
		++it;
	}
	//No suitable outline found, create one
	outlines->push_back(vector<ShapePathSegment>());
	outlines->back().push_back(ShapePathSegment(PATH_START, v1Index));
	outlines->back().push_back(ShapePathSegment(PATH_STRAIGHT, v2Index));
	return &(outlines->back());
}

std::vector<ShapePathSegment>* ShapesBuilder::extendOutlineCurve(std::vector< std::vector<ShapePathSegment> >* outlines, const Vector2& v1, const Vector2& v2, const Vector2& v3,std::vector<ShapePathSegment>* lastOutline)
{
	assert_and_throw(outlines);
	//Find the indices of the vertex
	uint64_t v1Index = makeVertex(v1);
	uint64_t v2Index = makeVertex(v2);
	uint64_t v3Index = makeVertex(v3);
	if (lastOutline)
	{
		lastOutline->emplace_back(ShapePathSegment(PATH_CURVE_QUADRATIC, v2Index));
		lastOutline->emplace_back(ShapePathSegment(PATH_CURVE_QUADRATIC, v3Index));
		return lastOutline;
	}

	//Search a suitable outline to attach this new vertex
	auto it = outlines->rbegin();
	while (it !=outlines->rend())
	{
		assert(it->size()>=2);
		if(it->front()==it->back())
		{
			++it;
			continue;
		}
		if(it->back().i==v1Index)
		{
			it->emplace_back(ShapePathSegment(PATH_CURVE_QUADRATIC, v2Index));
			it->emplace_back(ShapePathSegment(PATH_CURVE_QUADRATIC, v3Index));
			return &(*it);
		}
		++it;
	}
	//No suitable outline found, create one
	outlines->push_back(vector<ShapePathSegment>());
	outlines->back().emplace_back(ShapePathSegment(PATH_START, v1Index));
	outlines->back().emplace_back(ShapePathSegment(PATH_CURVE_QUADRATIC, v2Index));
	outlines->back().emplace_back(ShapePathSegment(PATH_CURVE_QUADRATIC, v3Index));
	return &(outlines->back());
}

void ShapesBuilder::outputTokens(const std::list<FILLSTYLE> &styles, const std::list<LINESTYLE2> &linestyles, tokensVector& tokens)
{
	joinOutlines();
	//Try to greedily condense as much as possible the output
	auto it=filledShapesMap.begin();
	//For each color
	for(;it!=filledShapesMap.end();++it)
	{
		assert(!it->second.empty());
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
		vector<vector<ShapePathSegment> >& outlinesForColor=it->second;
		for(unsigned int i=0;i<outlinesForColor.size();i++)
		{
			vector<ShapePathSegment>& segments=outlinesForColor[i];
			assert (segments[0].type == PATH_START);
			tokens.filltokens.push_back(GeomToken(MOVE).uval);
			tokens.filltokens.push_back(GeomToken(segments[0].i,true).uval);
			for(unsigned int j=1;j<segments.size();j++) {
				ShapePathSegment segment = segments[j];
				assert(segment.type != PATH_START);
				if (segment.type == PATH_STRAIGHT)
				{
					tokens.filltokens.push_back(GeomToken(STRAIGHT).uval);
					tokens.filltokens.push_back(GeomToken(segment.i,true).uval);
				}
				if (segment.type == PATH_CURVE_QUADRATIC)
				{
					tokens.filltokens.push_back(GeomToken(CURVE_QUADRATIC).uval);
					tokens.filltokens.push_back(GeomToken(segment.i,true).uval);
					tokens.filltokens.push_back(GeomToken(segments[++j].i,true).uval);
				}
			}
		}
	}
	if (strokeShapesMap.size() > 0)
	{
		tokens.stroketokens.push_back(GeomToken(CLEAR_FILL).uval);
		it=strokeShapesMap.begin();
		//For each stroke
		for(;it!=strokeShapesMap.end();++it)
		{
			assert(!it->second.empty());
			//Find the style given the index
			auto stylesIt=linestyles.begin();
			assert(it->first);
			for(unsigned int i=0;i<it->first-1;i++)
			{
				++stylesIt;
				assert(stylesIt!=linestyles.end());
			}
			//Set the line style
			vector<vector<ShapePathSegment> >& outlinesForStroke=it->second;
			tokens.stroketokens.push_back(GeomToken(SET_STROKE).uval);
			tokens.stroketokens.push_back(GeomToken(*stylesIt).uval);
			for(unsigned int i=0;i<outlinesForStroke.size();i++)
			{
				vector<ShapePathSegment>& segments=outlinesForStroke[i];
				assert (segments[0].type == PATH_START);
				tokens.stroketokens.push_back(GeomToken(MOVE).uval);
				tokens.stroketokens.push_back(GeomToken(segments[0].i,true).uval);
				for(unsigned int j=1;j<segments.size();j++) {
					ShapePathSegment segment = segments[j];
					assert(segment.type != PATH_START);
					if (segment.type == PATH_STRAIGHT)
					{
						tokens.stroketokens.push_back(GeomToken(STRAIGHT).uval);
						tokens.stroketokens.push_back(GeomToken(segment.i,true).uval);
					}
					if (segment.type == PATH_CURVE_QUADRATIC)
					{
						tokens.stroketokens.push_back(GeomToken(CURVE_QUADRATIC).uval);
						tokens.stroketokens.push_back(GeomToken(segment.i,true).uval);
						tokens.stroketokens.push_back(GeomToken(segments[++j].i,true).uval);
					}
				}
			}
		}
	}
}

void ShapesBuilder::outputMorphTokens(std::list<MORPHFILLSTYLE>& styles, std::list<MORPHLINESTYLE2> &linestyles, tokensVector &tokens, uint16_t ratio, const RECT& boundsrc)
{
	joinOutlines();
	//Try to greedily condense as much as possible the output
	auto it=filledShapesMap.begin();
	//For each color
	for(;it!=filledShapesMap.end();++it)
	{
		assert(!it->second.empty());
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

					ratiomatrix.scale(stylesIt->StartGradientMatrix.getScaleX()+(stylesIt->EndGradientMatrix.getScaleX()-stylesIt->StartGradientMatrix.getScaleX())*gradratio,
									  stylesIt->StartGradientMatrix.getScaleY()+(stylesIt->EndGradientMatrix.getScaleY()-stylesIt->StartGradientMatrix.getScaleY())*gradratio);
					ratiomatrix.rotate((stylesIt->StartGradientMatrix.getRotation()+(stylesIt->EndGradientMatrix.getRotation()-stylesIt->StartGradientMatrix.getRotation())*gradratio)*180.0/M_PI);
					ratiomatrix.translate(stylesIt->StartGradientMatrix.getTranslateX() +(stylesIt->EndGradientMatrix.getTranslateX()-stylesIt->StartGradientMatrix.getTranslateX())*gradratio,
										  stylesIt->StartGradientMatrix.getTranslateY() +(stylesIt->EndGradientMatrix.getTranslateY()-stylesIt->StartGradientMatrix.getTranslateY())*gradratio);

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
		vector<vector<ShapePathSegment> >& outlinesForColor=it->second;
		for(unsigned int i=0;i<outlinesForColor.size();i++)
		{
			vector<ShapePathSegment>& segments=outlinesForColor[i];
			assert (segments[0].type == PATH_START);
			tokens.filltokens.push_back(GeomToken(MOVE).uval);
			tokens.filltokens.push_back(GeomToken(segments[0].i,true).uval);
			for(unsigned int j=1;j<segments.size();j++) {
				ShapePathSegment segment = segments[j];
				assert(segment.type != PATH_START);
				if (segment.type == PATH_STRAIGHT)
				{
					tokens.filltokens.push_back(GeomToken(STRAIGHT).uval);
					tokens.filltokens.push_back(GeomToken(segment.i,true).uval);
				}
				if (segment.type == PATH_CURVE_QUADRATIC)
				{
					tokens.filltokens.push_back(GeomToken(CURVE_QUADRATIC).uval);
					tokens.filltokens.push_back(GeomToken(segment.i,true).uval);
					tokens.filltokens.push_back(GeomToken(segments[++j].i,true).uval);
				}
			}
		}
	}
	if (strokeShapesMap.size() > 0)
	{
		tokens.stroketokens.push_back(GeomToken(CLEAR_FILL).uval);
		it=strokeShapesMap.begin();
		//For each stroke
		for(;it!=strokeShapesMap.end();++it)
		{
			assert(!it->second.empty());
			//Find the style given the index
			std::list<MORPHLINESTYLE2>::iterator stylesIt=linestyles.begin();
			assert(it->first);
			for(unsigned int i=0;i<it->first-1;i++)
			{
				++stylesIt;
				assert(stylesIt!=linestyles.end());
			}
			//Set the line style
			auto itls =stylesIt->linestylecache.find(ratio);
			if (itls == stylesIt->linestylecache.end())
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
							
							ratiomatrix.scale(stylesIt->FillType.StartGradientMatrix.getScaleX()+(stylesIt->FillType.EndGradientMatrix.getScaleX()-stylesIt->FillType.StartGradientMatrix.getScaleX())*gradratio,
											  stylesIt->FillType.StartGradientMatrix.getScaleY()+(stylesIt->FillType.EndGradientMatrix.getScaleY()-stylesIt->FillType.StartGradientMatrix.getScaleY())*gradratio);
							ratiomatrix.rotate((stylesIt->FillType.StartGradientMatrix.getRotation()+(stylesIt->FillType.EndGradientMatrix.getRotation()-stylesIt->FillType.StartGradientMatrix.getRotation())*gradratio)*180.0/M_PI);
							ratiomatrix.translate(stylesIt->FillType.StartGradientMatrix.getTranslateX() +(stylesIt->FillType.EndGradientMatrix.getTranslateX()-stylesIt->FillType.StartGradientMatrix.getTranslateX())*gradratio,
												  stylesIt->FillType.StartGradientMatrix.getTranslateY() +(stylesIt->FillType.EndGradientMatrix.getTranslateY()-stylesIt->FillType.StartGradientMatrix.getTranslateY())*gradratio);
							
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
				}
				itls = stylesIt->linestylecache.insert(make_pair(ratio,ls)).first;
			}
			vector<vector<ShapePathSegment> >& outlinesForStroke=it->second;
			tokens.stroketokens.push_back(GeomToken(SET_STROKE).uval);
			tokens.stroketokens.push_back(GeomToken((*itls).second).uval);
			for(unsigned int i=0;i<outlinesForStroke.size();i++)
			{
				vector<ShapePathSegment>& segments=outlinesForStroke[i];
				assert (segments[0].type == PATH_START);
				tokens.stroketokens.push_back(GeomToken(MOVE).uval);
				tokens.stroketokens.push_back(GeomToken(segments[0].i,true).uval);
				for(unsigned int j=1;j<segments.size();j++) {
					ShapePathSegment segment = segments[j];
					assert(segment.type != PATH_START);
					if (segment.type == PATH_STRAIGHT)
					{
						tokens.stroketokens.push_back(GeomToken(STRAIGHT).uval);
						tokens.stroketokens.push_back(GeomToken(segment.i,true).uval);
					}
					if (segment.type == PATH_CURVE_QUADRATIC)
					{
						tokens.stroketokens.push_back(GeomToken(CURVE_QUADRATIC).uval);
						tokens.stroketokens.push_back(GeomToken(segment.i,true).uval);
						tokens.stroketokens.push_back(GeomToken(segments[++j].i,true).uval);
					}
				}
			}
		}
		
	}
}
