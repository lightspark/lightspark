/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009,2010  Alessandro Pignotti (a.pignotti@sssup.it)

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

#include "swf.h"
#include <fstream>
#include <math.h>
#include <algorithm>
#include <GL/glew.h>
#include "swftypes.h"
#include "logger.h"
#include "geometry.h"
#include "compat.h"

using namespace std;
using namespace lightspark;

bool ShapesBuilder::isOutlineEmpty(const std::vector< unsigned int >& outline)
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
	map< unsigned int, vector< vector <unsigned int> > >::iterator it=filledShapesMap.begin();
	for(;it!=filledShapesMap.end();it++)
	{
		vector< vector<unsigned int> >& outlinesForColor=it->second;
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
					outlinesForColor[j].insert(outlinesForColor[j].end(),
									outlinesForColor[i].rbegin()+1,
									outlinesForColor[i].rend());
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

void ShapesBuilder::extendFilledOutlineForColor(unsigned int color, const Vector2& v1, const Vector2& v2)
{
	assert_and_throw(color);
	//Find the indices of the vertex
	unsigned int v1Index, v2Index;
	map<Vector2, unsigned int>::const_iterator it=verticesMap.find(v1);
	if(it==verticesMap.end())
	{
		v1Index=verticesMap.size();
		verticesMap.insert(make_pair(v1,v1Index));
	}
	else
		v1Index=it->second;
	it=verticesMap.find(v2);
	if(it==verticesMap.end())
	{
		v2Index=verticesMap.size();
		verticesMap.insert(make_pair(v2,v2Index));
	}
	else
		v2Index=it->second;

	vector< vector<unsigned int> >& outlinesForColor=filledShapesMap[color];
	//Search a suitable outline to attach this new vertex
	for(unsigned int i=0;i<outlinesForColor.size();i++)
	{
		assert_and_throw(outlinesForColor[i].size()>=2);
		if(outlinesForColor[i].front()==outlinesForColor[i].back())
			continue;
		if(outlinesForColor[i].back()==v1Index)
		{
			outlinesForColor[i].push_back(v2Index);
			return;
		}
	}
	//No suitable outline found, create one
	outlinesForColor.push_back(vector<unsigned int>());
	outlinesForColor.back().reserve(2);
	outlinesForColor.back().push_back(v1Index);
	outlinesForColor.back().push_back(v2Index);
}

const Vector2& ShapesBuilder::getVertex(unsigned int index)
{
	//Linear lookup
	map<Vector2, unsigned int>::const_iterator it=verticesMap.begin();
	for(;it!=verticesMap.end();it++)
	{
		if(it->second==index)
			return it->first;
	}
	::abort();
}

void ShapesBuilder::outputTokens(const std::list<FILLSTYLE>& styles, vector< GeomToken >& tokens)
{
	joinOutlines();
	//Try to greedily condense as much as possible the output
	map< unsigned int, vector< vector<unsigned int> > >::iterator it=filledShapesMap.begin();
	//For each color
	for(;it!=filledShapesMap.end();it++)
	{
		assert(!it->second.empty());
		//Find the style given the index
		std::list<FILLSTYLE>::const_iterator stylesIt=styles.begin();
		assert(it->first);
		for(unsigned int i=0;i<it->first-1;i++)
		{
			stylesIt++;
			assert(stylesIt!=styles.end());
		}
		//Set the fill style
		tokens.emplace_back(SET_FILL,*stylesIt);
		vector<vector<unsigned int> >& outlinesForColor=it->second;
		for(unsigned int i=0;i<outlinesForColor.size();i++)
		{
			vector<unsigned int>& vertices=outlinesForColor[i];
			tokens.emplace_back(MOVE,getVertex(vertices[0]));
			for(unsigned int j=1;j<vertices.size();j++)
				tokens.emplace_back(STRAIGHT,getVertex(vertices[j]));
		}
	}
}

std::ostream& lightspark::operator<<(std::ostream& s, const Vector2& p)
{
	s << "{ "<< p.x << ',' << p.y << " }";
	return s;
}

