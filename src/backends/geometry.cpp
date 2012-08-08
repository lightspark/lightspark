/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2012  Alessandro Pignotti (a.pignotti@sssup.it)

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
	for(unsigned int i=0;i<outlinesForColor.size();i++)
	{
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
	for(unsigned int i=0;i<outlinesForColor.size();i++)
	{
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
	//Linear lookup
	map<Vector2, unsigned int>::const_iterator it=verticesMap.begin();
	for(;it!=verticesMap.end();++it)
	{
		if(it->second==index)
			return it->first;
	}
	::abort();
}

void ShapesBuilder::outputTokens(const std::list<FILLSTYLE>& styles, tokensVector& tokens)
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
		tokens.emplace_back(GeomToken(SET_FILL,*stylesIt));
		vector<vector<ShapePathSegment> >& outlinesForColor=it->second;
		for(unsigned int i=0;i<outlinesForColor.size();i++)
		{
			vector<ShapePathSegment>& segments=outlinesForColor[i];
			assert (segments[0].type == PATH_START);
			tokens.push_back(GeomToken(MOVE, getVertex(segments[0].i)));
			for(unsigned int j=1;j<segments.size();j++) {
				ShapePathSegment segment = segments[j];
				assert(segment.type != PATH_START);
				if (segment.type == PATH_STRAIGHT)
					tokens.push_back(GeomToken(STRAIGHT, getVertex(segment.i)));
				if (segment.type == PATH_CURVE_QUADRATIC)
					tokens.push_back(GeomToken(CURVE_QUADRATIC, getVertex(segment.i), getVertex(segments[++j].i)));
			}
		}
	}
}

