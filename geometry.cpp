/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009,2010  Alessandro Pignotti (a.pignotti@sssup.it)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#include <fstream>
#include <math.h>
#include <algorithm>
#include <GL/glew.h>
#include "swftypes.h"
#include "logger.h"
#include "geometry.h"
#include "swf.h"


using namespace std;
using namespace lightspark;

/*! \brief Renders the shape interior and outline setting the correct
* *        parameters for the shader
* * \param x Optional x translation
* * \param y Optional y translation */

extern TLSDATA RenderThread* rt;


void GeomShape::Render(int x, int y) const
{
	if(outline.empty())
	{
		LOG(LOG_TRACE,"No edges in this shape");
		return;
	}

	bool filled=false;
	if(closed && color)
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
		assert(triangles.size()%3==0);
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
		std::vector<Vector2>::const_iterator it=outline.begin();
		if(closed)
			glBegin(GL_LINE_LOOP);
		else
			glBegin(GL_LINE_STRIP);
		for(;it!=outline.end();it++)
			glVertex2i(it->x+x,it->y+y);
		glEnd();
	}

	for(unsigned int i=0;i<sub_shapes.size();i++)
		sub_shapes[i].Render();
}

void GeomShape::dumpEdges()
{
	ofstream f("edges.dat");

	for(unsigned int i=0;i<outline.size();i++)
		f << outline[i].x << ' ' << outline[i].y << endl;
	f.close();
}

void GeomShape::SetStyles(const std::list<FILLSTYLE>* styles)
{
	static FILLSTYLE* clearStyle=NULL;
	if(!clearStyle)
	{
		clearStyle=new FILLSTYLE;
		clearStyle->FillStyleType=0x00;
		clearStyle->Color=RGBA(0,0,0,0);
	}

	if(styles)
	{
		if(color)
		{
			if(color > styles->size())
				abort();

			//Simulate array access :-(
			list<FILLSTYLE>::const_iterator it=styles->begin();
			for(unsigned int i=0;i<(color-1);i++)
				it++;
			style=&(*it);
		}
		else
			style=clearStyle;
	}
}

void GeomShape::BuildFromEdges(const std::list<FILLSTYLE>* styles)
{
	if(outline.empty())
		return;

	if(outline.size()>1 && outline.front()==outline.back())
	{
		closed=true;
		outline.pop_back();
	}
	else
		closed=false;

	SetStyles(styles);

	//Tessellate the shape using GLU
	if(closed)
		TessellateGLU();
}

void GeomShape::TessellateGLU()
{
	//NOTE: do not invalidate the contents of outline in this function
	GLUtesselator* tess=gluNewTess();

	gluTessCallback(tess,GLU_TESS_BEGIN_DATA,(void(*)())GLUCallbackBegin);
	gluTessCallback(tess,GLU_TESS_VERTEX_DATA,(void(*)())GLUCallbackVertex);
	gluTessCallback(tess,GLU_TESS_END_DATA,(void(*)())GLUCallbackEnd);
	gluTessCallback(tess,GLU_TESS_COMBINE_DATA,(void(*)())GLUCallbackCombine);
	
	//Let's create a vector of pointers to store temporary coordinates
	//passed to GLU
	vector<GLdouble*> tmpCoord;
	tmpCoord.reserve(outline.size());
	
	gluTessBeginPolygon(tess,this);
		gluTessBeginContour(tess);
			for(unsigned int i=0;i<outline.size();i++)
			{
				GLdouble* loc=new GLdouble[3];
				loc[0]=outline[i].x;
				loc[1]=outline[i].y;
				loc[2]=0;
				tmpCoord.push_back(loc);
				//As the data we pass the Vector2 pointer
				gluTessVertex(tess,loc,&outline[i]);
			}
		gluTessEndContour(tess);
	gluTessEndPolygon(tess);
	
	//Clean up locations
	for(unsigned int i=0;i<tmpCoord.size();i++)
		delete[] tmpCoord[i];
	
	//Clean up temporary vertices
	for(unsigned int i=0;i<tmpVertices.size();i++)
		delete tmpVertices[i];
	
	tmpVertices.clear();
	
}

void GeomShape::GLUCallbackBegin(GLenum type, GeomShape* obj)
{
	assert(obj->curTessTarget==0);
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

void GeomShape::GLUCallbackVertex(Vector2* vertexData, GeomShape* obj)
{
	assert(obj->curTessTarget!=0);
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

void GeomShape::GLUCallbackEnd(GeomShape* obj)
{
	assert(obj->curTessTarget!=0);
	obj->curTessTarget=0;
}

void GeomShape::GLUCallbackCombine(GLdouble coords[3], void* vertex_data[4], 
				   GLfloat weight[4], Vector2** outData, GeomShape* obj)
{
	//No real operations, apart from generating a new vertex at the passed coordinates
	obj->tmpVertices.push_back(new Vector2(coords[0],coords[1]));
	*outData=obj->tmpVertices.back();
}

//Shape are compared using the minimum vertex
bool GeomShape::operator<(const GeomShape& r) const
{
	Vector2 vl=*min_element(outline.begin(),outline.end());
	Vector2 vr=*min_element(r.outline.begin(),r.outline.end());

	if(vl<vr)
		return true;
	else
		return false;
}
