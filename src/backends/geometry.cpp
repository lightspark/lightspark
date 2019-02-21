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
	map< unsigned int, vector< vector <ShapePathSegment> > >::iterator it=filledShapesMap.begin();
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

unsigned int ShapesBuilder::makeVertex(const Vector2& v) {
	map<Vector2, unsigned int>::const_iterator it=verticesMap.find(v);
	if(it!=verticesMap.end())
		return it->second;

	unsigned int i = verticesMap.size();
	verticesMap.insert(make_pair(v, i));
	verticesVector.push_back(v);
	return i;
}

void ShapesBuilder::extendFilledOutlineForColor(unsigned int color, const Vector2& v1, const Vector2& v2)
{
	assert_and_throw(color);
	//Find the indices of the vertex
	unsigned int v1Index = makeVertex(v1);
	unsigned int v2Index = makeVertex(v2);

	vector< vector<ShapePathSegment> >& outlinesForColor=filledShapesMap[color];
	//Search a suitable outline to attach this new vertex
	for(unsigned int i=outlinesForColor.size();i;)
	{
		--i;
		assert_and_throw(outlinesForColor[i].size()>=2);
		if(outlinesForColor[i].front()==outlinesForColor[i].back())
			continue;
		if(outlinesForColor[i].back().i==v1Index)
		{
			outlinesForColor[i].push_back(ShapePathSegment(PATH_STRAIGHT, v2Index));
			return;
		}
	}
	//No suitable outline found, create one
	vector<ShapePathSegment> vertices;
	vertices.reserve(2);
	vertices.push_back(ShapePathSegment(PATH_START, v1Index));
	vertices.push_back(ShapePathSegment(PATH_STRAIGHT, v2Index));
	outlinesForColor.push_back(vertices);
}

void ShapesBuilder::extendFilledOutlineForColorCurve(unsigned int color, const Vector2& v1, const Vector2& v2, const Vector2& v3)
{
	assert_and_throw(color);
	//Find the indices of the vertex
	unsigned int v1Index = makeVertex(v1);
	unsigned int v2Index = makeVertex(v2);
	unsigned int v3Index = makeVertex(v3);

	vector< vector<ShapePathSegment> >& outlinesForColor=filledShapesMap[color];
	//Search a suitable outline to attach this new vertex
	for(unsigned int i=outlinesForColor.size();i;)
	{
		--i;
		assert_and_throw(outlinesForColor[i].size()>=2);
		if(outlinesForColor[i].front()==outlinesForColor[i].back())
			continue;
		if(outlinesForColor[i].back().i==v1Index)
		{
			outlinesForColor[i].emplace_back(ShapePathSegment(PATH_CURVE_QUADRATIC, v2Index));
			outlinesForColor[i].emplace_back(ShapePathSegment(PATH_CURVE_QUADRATIC, v3Index));
			return;
		}
	}
	//No suitable outline found, create one
	vector<ShapePathSegment> vertices;
	vertices.reserve(2);
	vertices.emplace_back(ShapePathSegment(PATH_START, v1Index));
	vertices.emplace_back(ShapePathSegment(PATH_CURVE_QUADRATIC, v2Index));
	vertices.emplace_back(ShapePathSegment(PATH_CURVE_QUADRATIC, v3Index));
	outlinesForColor.push_back(vertices);
}

void ShapesBuilder::extendStrokeOutline(unsigned int stroke, const Vector2 &v1, const Vector2 &v2)
{
	assert_and_throw(stroke);
	//Find the indices of the vertex
	unsigned int v1Index = makeVertex(v1);
	unsigned int v2Index = makeVertex(v2);

	vector< vector<ShapePathSegment> >& outlinesForStroke=strokeShapesMap[stroke];
	//Search a suitable outline to attach this new vertex
	for(unsigned int i=outlinesForStroke.size();i;)
	{
		--i;
		assert_and_throw(outlinesForStroke[i].size()>=2);
		if(outlinesForStroke[i].front()==outlinesForStroke[i].back())
			continue;
		if(outlinesForStroke[i].back().i==v1Index)
		{
			outlinesForStroke[i].push_back(ShapePathSegment(PATH_STRAIGHT, v2Index));
			return;
		}
	}
	//No suitable outline found, create one
	vector<ShapePathSegment> vertices;
	vertices.reserve(2);
	vertices.push_back(ShapePathSegment(PATH_START, v1Index));
	vertices.push_back(ShapePathSegment(PATH_STRAIGHT, v2Index));
	outlinesForStroke.push_back(vertices);
}

void ShapesBuilder::extendStrokeOutlineCurve(unsigned int color, const Vector2& v1, const Vector2& v2, const Vector2& v3)
{
	assert_and_throw(color);
	//Find the indices of the vertex
	unsigned int v1Index = makeVertex(v1);
	unsigned int v2Index = makeVertex(v2);
	unsigned int v3Index = makeVertex(v3);

	vector< vector<ShapePathSegment> >& outlinesForColor=strokeShapesMap[color];
	//Search a suitable outline to attach this new vertex
	for(unsigned int i=outlinesForColor.size();i;)
	{
		--i;
		assert_and_throw(outlinesForColor[i].size()>=2);
		if(outlinesForColor[i].front()==outlinesForColor[i].back())
			continue;
		if(outlinesForColor[i].back().i==v1Index)
		{
			outlinesForColor[i].emplace_back(ShapePathSegment(PATH_CURVE_QUADRATIC, v2Index));
			outlinesForColor[i].emplace_back(ShapePathSegment(PATH_CURVE_QUADRATIC, v3Index));
			return;
		}
	}
	//No suitable outline found, create one
	vector<ShapePathSegment> vertices;
	vertices.reserve(2);
	vertices.emplace_back(ShapePathSegment(PATH_START, v1Index));
	vertices.emplace_back(ShapePathSegment(PATH_CURVE_QUADRATIC, v2Index));
	vertices.emplace_back(ShapePathSegment(PATH_CURVE_QUADRATIC, v3Index));
	outlinesForColor.push_back(vertices);
}

const Vector2& ShapesBuilder::getVertex(unsigned int index)
{
	if (index < verticesVector.size())
		return verticesVector[index];
	/*
	//Linear lookup
	map<Vector2, unsigned int>::const_iterator it=verticesMap.begin();
	for(;it!=verticesMap.end();++it)
	{
		if(it->second==index)
			return it->first;
	}
	*/
	::abort();
}

void ShapesBuilder::outputTokens(const std::list<FILLSTYLE>& styles,const std::list<LINESTYLE2>& linestyles, tokensVector& tokens)
{
	joinOutlines();
	//Try to greedily condense as much as possible the output
	map< unsigned int, vector< vector<ShapePathSegment> > >::iterator it=filledShapesMap.begin();
	//For each color
	for(;it!=filledShapesMap.end();++it)
	{
		assert(!it->second.empty());
		//Find the style given the index
		std::list<FILLSTYLE>::const_iterator stylesIt=styles.begin();
		assert(it->first);
		for(unsigned int i=0;i<it->first-1;i++)
		{
			++stylesIt;
			assert(stylesIt!=styles.end());
		}
		//Set the fill style
		tokens.filltokens.emplace_back(_MR(new GeomToken(SET_FILL,*stylesIt)));
		vector<vector<ShapePathSegment> >& outlinesForColor=it->second;
		for(unsigned int i=0;i<outlinesForColor.size();i++)
		{
			vector<ShapePathSegment>& segments=outlinesForColor[i];
			assert (segments[0].type == PATH_START);
			tokens.filltokens.push_back(_MR(new GeomToken(MOVE, getVertex(segments[0].i))));
			for(unsigned int j=1;j<segments.size();j++) {
				ShapePathSegment segment = segments[j];
				assert(segment.type != PATH_START);
				if (segment.type == PATH_STRAIGHT)
					tokens.filltokens.push_back(_MR(new GeomToken(STRAIGHT, getVertex(segment.i))));
				if (segment.type == PATH_CURVE_QUADRATIC)
					tokens.filltokens.push_back(_MR(new GeomToken(CURVE_QUADRATIC, getVertex(segment.i), getVertex(segments[++j].i))));
			}
		}
	}
	if (strokeShapesMap.size() > 0)
	{
		tokens.stroketokens.emplace_back(_MR(new GeomToken(CLEAR_FILL)));
		it=strokeShapesMap.begin();
		//For each stroke
		for(;it!=strokeShapesMap.end();++it)
		{
			assert(!it->second.empty());
			//Find the style given the index
			std::list<LINESTYLE2>::const_iterator stylesIt=linestyles.begin();
			assert(it->first);
			for(unsigned int i=0;i<it->first-1;i++)
			{
				++stylesIt;
				assert(stylesIt!=linestyles.end());
			}
			//Set the line style
			vector<vector<ShapePathSegment> >& outlinesForStroke=it->second;
			tokens.stroketokens.emplace_back(_MR(new GeomToken(SET_STROKE,*stylesIt)));
			for(unsigned int i=0;i<outlinesForStroke.size();i++)
			{
				vector<ShapePathSegment>& segments=outlinesForStroke[i];
				assert (segments[0].type == PATH_START);
				tokens.stroketokens.push_back(_MR(new GeomToken(MOVE, getVertex(segments[0].i))));
				for(unsigned int j=1;j<segments.size();j++) {
					ShapePathSegment segment = segments[j];
					assert(segment.type != PATH_START);
					if (segment.type == PATH_STRAIGHT)
						tokens.stroketokens.push_back(_MR(new GeomToken(STRAIGHT, getVertex(segment.i))));
					if (segment.type == PATH_CURVE_QUADRATIC)
						tokens.stroketokens.push_back(_MR(new GeomToken(CURVE_QUADRATIC, getVertex(segment.i), getVertex(segments[++j].i))));
				}
			}
		}
	}
}

void ShapesBuilder::outputMorphTokens(const std::list<MORPHFILLSTYLE> &styles, const std::list<MORPHLINESTYLE2> &linestyles, tokensVector &tokens, uint16_t ratio)
{
	joinOutlines();
	//Try to greedily condense as much as possible the output
	map< unsigned int, vector< vector<ShapePathSegment> > >::iterator it=filledShapesMap.begin();
	//For each color
	for(;it!=filledShapesMap.end();++it)
	{
		assert(!it->second.empty());
		//Find the style given the index
		std::list<MORPHFILLSTYLE>::const_iterator stylesIt=styles.begin();
		assert(it->first);
		for(unsigned int i=0;i<it->first-1;i++)
		{
			++stylesIt;
			assert(stylesIt!=styles.end());
		}
		//Set the fill style
		FILLSTYLE f = *stylesIt;
		switch (stylesIt->FillStyleType)
		{
			case LINEAR_GRADIENT:
			{
				f.Matrix = stylesIt->StartGradientMatrix;
				f.Gradient.GradientRecords.reserve(stylesIt->StartColors.size());
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
					f.Gradient.GradientRecords.push_back(gr);
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
		tokens.filltokens.emplace_back(_MR(new GeomToken(SET_FILL,f)));
		vector<vector<ShapePathSegment> >& outlinesForColor=it->second;
		for(unsigned int i=0;i<outlinesForColor.size();i++)
		{
			vector<ShapePathSegment>& segments=outlinesForColor[i];
			assert (segments[0].type == PATH_START);
			tokens.filltokens.push_back(_MR(new GeomToken(MOVE, getVertex(segments[0].i))));
			for(unsigned int j=1;j<segments.size();j++) {
				ShapePathSegment segment = segments[j];
				assert(segment.type != PATH_START);
				if (segment.type == PATH_STRAIGHT)
					tokens.filltokens.push_back(_MR(new GeomToken(STRAIGHT, getVertex(segment.i))));
				if (segment.type == PATH_CURVE_QUADRATIC)
					tokens.filltokens.push_back(_MR(new GeomToken(CURVE_QUADRATIC, getVertex(segment.i), getVertex(segments[++j].i))));
			}
		}
	}
	if (strokeShapesMap.size() > 0)
	{
		tokens.stroketokens.emplace_back(_MR(new GeomToken(CLEAR_FILL)));
		it=strokeShapesMap.begin();
		//For each stroke
		for(;it!=strokeShapesMap.end();++it)
		{
			assert(!it->second.empty());
			//Find the style given the index
			std::list<MORPHLINESTYLE2>::const_iterator stylesIt=linestyles.begin();
			assert(it->first);
			for(unsigned int i=0;i<it->first-1;i++)
			{
				++stylesIt;
				assert(stylesIt!=linestyles.end());
			}
			//Set the line style
			LOG(LOG_NOT_IMPLEMENTED,"morphing for line styles");
			vector<vector<ShapePathSegment> >& outlinesForStroke=it->second;
			tokens.stroketokens.emplace_back(_MR(new GeomToken(SET_STROKE,*stylesIt)));
			for(unsigned int i=0;i<outlinesForStroke.size();i++)
			{
				vector<ShapePathSegment>& segments=outlinesForStroke[i];
				assert (segments[0].type == PATH_START);
				tokens.stroketokens.push_back(_MR(new GeomToken(MOVE, getVertex(segments[0].i))));
				for(unsigned int j=1;j<segments.size();j++) {
					ShapePathSegment segment = segments[j];
					assert(segment.type != PATH_START);
					if (segment.type == PATH_STRAIGHT)
						tokens.stroketokens.push_back(_MR(new GeomToken(STRAIGHT, getVertex(segment.i))));
					if (segment.type == PATH_CURVE_QUADRATIC)
						tokens.stroketokens.push_back(_MR(new GeomToken(CURVE_QUADRATIC, getVertex(segment.i), getVertex(segments[++j].i))));
				}
			}
		}
		
	}
}


GeomToken::GeomToken(GEOM_TOKEN_TYPE _t, const MORPHLINESTYLE2& _s):fillStyle(0xff),lineStyle(0xff),type(_t),p1(0,0),p2(0,0),p3(0,0)
{
	
}
