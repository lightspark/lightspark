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
#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <list>
#include <iostream>
#include <vector>
/*#ifdef WIN32
#include <windows.h>
#endif*/
#include <GL/glew.h>

namespace lightspark
{

class Vector2
{
public:
	int x,y;
	Vector2(int a, int b):x(a),y(b){}
	bool operator==(const Vector2& v)const{return v.x==x && v.y==y;}
	bool operator!=(const Vector2& v)const{return v.x!=x || v.y!=y;}
	bool operator<(const Vector2& v) const {return (y==v.y)?(x < v.x):(y < v.y);}
	const Vector2 operator-(const Vector2& v)const { return Vector2(x-v.x,y-v.y);}
	const Vector2 operator+(const Vector2& v)const { return Vector2(x+v.x,y+v.y);}
	const Vector2 operator*(int p)const { return Vector2(x*p,y*p);}
	Vector2& operator/=(int v) { x/=v; y/=v; return *this;}
	int dot(const Vector2& r) const { return x*r.x+y*r.y;}

};

#include "packed_begin.h"
struct arrayElem
{
	GLint coord[2];
	GLfloat colors[3];
	GLfloat texcoord[4];
} PACKED;
#include "packed_end.h"

class GeomShape
{
friend class DefineTextTag;
friend class DefineShape2Tag;
friend class DefineShape3Tag;
private:
	void TessellateGLU();
	static void GLUCallbackBegin(GLenum type, GeomShape* obj);
	static void GLUCallbackEnd(GeomShape* obj);
	static void GLUCallbackVertex(Vector2* vertexData, GeomShape* obj);
	static void GLUCallbackCombine(GLdouble coords[3], void *vertex_data[4], 
				       GLfloat weight[4], Vector2** outData, GeomShape* obj);
	GLenum curTessTarget;
	std::vector<Vector2*> tmpVertices;
	void SetStyles(const std::list<FILLSTYLE>* styles);
	const FILLSTYLE* style;
	arrayElem* varray;
	bool closed;
public:
	GeomShape():curTessTarget(0),style(NULL),varray(NULL),closed(false),color(0){}
	std::vector<Vector2> triangles;
	std::vector<std::vector<Vector2> > triangle_strips;
	std::vector<std::vector<Vector2> > triangle_fans;

	std::vector<Vector2> outline;
	std::vector<GeomShape> sub_shapes;

	unsigned int color;

	//DEBUG
	void dumpEdges();

	void Render(int x=0, int y=0) const;
	void BuildFromEdges(const std::list<FILLSTYLE>* styles);

	bool operator<(const GeomShape& r) const;

};

class GlyphShape: public GeomShape
{
public:
	GlyphShape(const GeomShape& g, int i):GeomShape(g),id(i){}
	int id;
};

};

#endif
