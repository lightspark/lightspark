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
#include "backends/rendering.h"

using namespace std;
using namespace lightspark;

/*! \brief Renders the shape interior and outline setting the correct
* *        parameters for the shader
* * \param x Optional x translation
* * \param y Optional y translation */

extern TLSDATA RenderThread* rt;


void GeomShape::Render(int x, int y) const
{
	if(outlines.empty())
	{
		LOG(LOG_TRACE,"No edges in this shape");
		return;
	}

	bool filled=false;
	if(hasFill && color)
	{
		if(!rt->materialOverride)
			style->setFragmentProgram();

		//Render the strips
		for(unsigned int i=0;i<triangle_strips.size();i++)
		{
			glBegin(GL_TRIANGLE_STRIP);
			for(unsigned int j=0;j<triangle_strips[i].size();j++)
				glVertex2i(triangle_strips[i][j].x+x,triangle_strips[i][j].y+y);
			glEnd();
		}
		
		//Render the fans
		for(unsigned int i=0;i<triangle_fans.size();i++)
		{
			glBegin(GL_TRIANGLE_FAN);
			for(unsigned int j=0;j<triangle_fans[i].size();j++)
				glVertex2i(triangle_fans[i][j].x+x,triangle_fans[i][j].y+y);
			glEnd();
		}

		//Render lone triangles
		glBegin(GL_TRIANGLES);
		for(unsigned int i=0;i<triangles.size();i++)
			glVertex2i(triangles[i].x+x,triangles[i].y+y);
		glEnd();
		filled=true;

		/*char* t=(char*)varray;
		glVertexPointer(2,GL_INT,sizeof(arrayElem),t);
		glColorPointer(3,GL_FLOAT,sizeof(arrayElem),t+2*sizeof(GLint));
		glTexCoordPointer(4,GL_FLOAT,sizeof(arrayElem),t+2*sizeof(GLint)+3*sizeof(GLfloat));

		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_COLOR_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		int used=0;
		for(int i=0;i<triangle_strips.size();i++)
		{
			glDrawArrays(GL_TRIANGLE_STRIP,used,triangle_strips[i].size());
			used+=triangle_strips[i].size();
		}
		glDisableClientState(GL_VERTEX_ARRAY);*/
	}

	if(/*graphic.stroked ||*/ !filled && color)
	{
		//LOG(TRACE,"Line tracing");
		if(!rt->materialOverride)
			FILLSTYLE::fixedColor(0,0,0);
		for(unsigned int i=0;i<outlines.size();i++)
		{
			std::vector<Vector2>::const_iterator it=outlines[i].begin();
			glBegin(GL_LINE_STRIP);
			for(;it!=outlines[i].end();it++)
				glVertex2i(it->x+x,it->y+y);
			glEnd();
		}
	}
}

void GeomShape::SetStyles(const std::list<FILLSTYLE>* styles)
{
	if(styles)
	{
		assert_and_throw(color);
		if(color > styles->size())
			throw RunTimeException("Not enough styles in GeomShape::SetStyles");

		//Simulate array access :-(
		list<FILLSTYLE>::const_iterator it=styles->begin();
		for(unsigned int i=0;i<(color-1);i++)
			it++;
		style=&(*it);
	}
}

void GeomShape::BuildFromEdges(const std::list<FILLSTYLE>* styles)
{
	if(outlines.empty())
		return;

	SetStyles(styles);

	TessellateGLU();
}

void GeomShape::TessellateGLU()
{
	//NOTE: do not invalidate the contents of outline in this function
	GLUtesselator* tess=gluNewTess();

	gluTessCallback(tess,GLU_TESS_BEGIN_DATA,(void(CALLBACK *)())GLUCallbackBegin);
	gluTessCallback(tess,GLU_TESS_VERTEX_DATA,(void(CALLBACK *)())GLUCallbackVertex);
	gluTessCallback(tess,GLU_TESS_END_DATA,(void(CALLBACK *)())GLUCallbackEnd);
	gluTessCallback(tess,GLU_TESS_COMBINE_DATA,(void(CALLBACK *)())GLUCallbackCombine);
	gluTessProperty(tess,GLU_TESS_WINDING_RULE,GLU_TESS_WINDING_ODD);
	
	//Let's create a vector of pointers to store temporary coordinates
	//passed to GLU
	vector<GLdouble*> tmpCoord;
	gluTessBeginPolygon(tess,this);
	for(unsigned int i=0;i<outlines.size();i++)
	{
		//Check if the contour is closed
		if(outlines[i].front()==outlines[i].back())
			hasFill=true;
		else
			continue;
		gluTessBeginContour(tess);
			//First and last vertex are automatically linked
			for(unsigned int j=1;j<outlines[i].size();j++)
			{
				GLdouble* loc=new GLdouble[3];
				loc[0]=outlines[i][j].x;
				loc[1]=outlines[i][j].y;
				loc[2]=0;
				tmpCoord.push_back(loc);
				//As the data we pass the Vector2 pointer
				gluTessVertex(tess,loc,&outlines[i][j]);
			}
		gluTessEndContour(tess);
	}
	gluTessEndPolygon(tess);
	
	//Clean up locations
	for(unsigned int i=0;i<tmpCoord.size();i++)
		delete[] tmpCoord[i];
	
	//Clean up temporary vertices
	for(unsigned int i=0;i<tmpVertices.size();i++)
		delete tmpVertices[i];

	gluDeleteTess(tess);
}

void CALLBACK GeomShape::GLUCallbackBegin(GLenum type, GeomShape* obj)
{
	assert_and_throw(obj->curTessTarget==0);
	if(type==GL_TRIANGLE_FAN)
	{
		obj->triangle_fans.push_back(vector<Vector2>());
		obj->curTessTarget=GL_TRIANGLE_FAN;
	}
	else if(type==GL_TRIANGLE_STRIP)
	{
		obj->triangle_strips.push_back(vector<Vector2>());
		obj->curTessTarget=GL_TRIANGLE_STRIP;
	}
	else if(type==GL_TRIANGLES)
	{
		obj->curTessTarget=GL_TRIANGLES;
	}
	else
		::abort();
}

void CALLBACK GeomShape::GLUCallbackVertex(Vector2* vertexData, GeomShape* obj)
{
	assert_and_throw(obj->curTessTarget!=0);
	if(obj->curTessTarget==GL_TRIANGLE_FAN)
	{
		obj->triangle_fans.back().push_back(*vertexData);
	}
	else if(obj->curTessTarget==GL_TRIANGLE_STRIP)
	{
		obj->triangle_strips.back().push_back(*vertexData);
	}
	else if(obj->curTessTarget==GL_TRIANGLES)
	{
		obj->triangles.push_back(*vertexData);
	}
}

void CALLBACK GeomShape::GLUCallbackEnd(GeomShape* obj)
{
	assert_and_throw(obj->curTessTarget!=0);
	if(obj->curTessTarget==GL_TRIANGLES)
		assert_and_throw(obj->triangles.size()%3==0);
	obj->curTessTarget=0;
}

void CALLBACK GeomShape::GLUCallbackCombine(GLdouble coords[3], void* vertex_data[4], 
				   GLfloat weight[4], Vector2** outData, GeomShape* obj)
{
	//No real operations, apart from generating a new vertex at the passed coordinates
	obj->tmpVertices.push_back(new Vector2(coords[0],coords[1]));
	*outData=obj->tmpVertices.back();
}

bool ShapesBuilder::isOutlineEmpty(const std::vector< Vector2 >& outline)
{
	return outline.empty();
}

void ShapesBuilder::clear()
{
	shapesMap.clear();
}

void ShapesBuilder::joinOutlines()
{
	map< unsigned int, vector< vector<Vector2> > >::iterator it=shapesMap.begin();
	for(;it!=shapesMap.end();it++)
	{
		vector< vector<Vector2> >& outlinesForColor=it->second;
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

void ShapesBuilder::extendOutlineForColor(unsigned int color, const Vector2& v1, const Vector2& v2)
{
	assert_and_throw(color);
	vector< vector<Vector2> >& outlinesForColor=shapesMap[color];
	//Search a suitable outline to attach this new vertex
	for(unsigned int i=0;i<outlinesForColor.size();i++)
	{
		assert_and_throw(outlinesForColor[i].size()>=2);
		if(outlinesForColor[i].front()==outlinesForColor[i].back())
			continue;
		if(outlinesForColor[i].back()==v1)
		{
			outlinesForColor[i].push_back(v2);
			return;
		}
	}
	//No suitable outline found, create one
	outlinesForColor.push_back(vector<Vector2>());
	outlinesForColor.back().reserve(2);
	outlinesForColor.back().push_back(v1);
	outlinesForColor.back().push_back(v2);
}

void ShapesBuilder::outputShapes(vector< GeomShape >& shapes)
{
	//Let's join shape pieces together
	joinOutlines();

	map< unsigned int, vector< vector<Vector2> > >::iterator it=shapesMap.begin();

	for(;it!=shapesMap.end();it++)
	{
		shapes.push_back(GeomShape());
		shapes.back().outlines.swap(it->second);
		shapes.back().color=it->first;
	}
}
