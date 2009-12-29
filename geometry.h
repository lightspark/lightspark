/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009  Alessandro Pignotti (a.pignotti@sssup.it)

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

class Path;
class Vector2;
class GeomShape;

class FilterIterator
{
private:
	std::vector<Vector2>::const_iterator it;
	std::vector<Vector2>::const_iterator end;
	int filter;
public:
	FilterIterator(const std::vector<Vector2>::const_iterator& i, const std::vector<Vector2>::const_iterator& e, int f);
	FilterIterator operator++(int);
	bool operator==(FilterIterator& i);
	bool operator!=(FilterIterator& i);
	const Vector2& operator*();
};

class Vector2
{
public:
	int x,y;
	int index;
	Vector2(int a, int b, int i):x(a),y(b),index(i){}
	Vector2(int a, int b):x(a),y(b),index(-1){}
	bool operator==(const Vector2& v)const{return v.x==x && v.y==y;}
	bool operator!=(const Vector2& v)const{return v.x!=x || v.y!=y;}
	bool operator==(int i){return index==i;}
	bool operator<(const Vector2& v) const {return (y==v.y)?(x < v.x):(y < v.y);}
//	bool operator<=(const Vector2& v) const {return y<=v.y;}
//	bool operator>=(const Vector2& v) const {return y>=v.y;}
//	bool operator<(int i) const {return index<i;}
	const Vector2 operator-(const Vector2& v)const { return Vector2(x-v.x,y-v.y);}
	const Vector2 operator+(const Vector2& v)const { return Vector2(x+v.x,y+v.y);}
	const Vector2 operator*(int p)const { return Vector2(x*p,y*p);}
	Vector2& operator/=(int v) { x/=v; y/=v; return *this;}
	int dot(const Vector2& r) const { return x*r.x+y*r.y;}

};

class Triangle
{
public:
	Vector2 v1,v2,v3;
	Triangle(Vector2 a,Vector2 b, Vector2 c):v1(a),v2(b),v3(c){}
};

#include "packed_begin.h"
struct arrayElem
{
	GLint coord[2];
	GLfloat colors[3];
	GLfloat texcoord[4];
} PACKED;
#include "packed_end.h"

class Edge
{
	friend class GeomShape;
public:
	Vector2 p1;
	Vector2 p2;
	int color0;
	int color1;
	Edge(const Vector2& a,const Vector2& b,int c1, int c2):color0(c1),color1(c2),p1(a),p2(b)
	{
	}
	bool yIntersect(const Vector2& p, int& dist);
//	bool xIntersect(int x,int32_t& d);
//	bool edgeIntersect(const Edge& e);
	bool operator<(const Edge& e) const
	{
		//Edges are ordered first by the lowest vertex and then by highest
		Vector2 l1(p1),l2(p2),r1(e.p1),r2(e.p2);
		if(p2<p1)
		{
			l2=p1;
			l1=p2;
		}
		if(e.p2<e.p1)
		{
			r2=e.p1;
			r1=e.p2;
		}

		if(l1<r1)
			return true;
		else if(r1<l1)
			return false;
		else
		{
			if(l2<r2)
				return true;
			else
				return false;
		}
	}
	const Vector2& highPoint()
	{
		if(p1<p2)
			return p1;
		else
			return p2;
	}
	const Vector2& lowPoint()
	{
		if(p1<p2)
			return p2;
		else
			return p1;
	}
};

class GeomShape
{
friend class DefineTextTag;
friend class DefineShape2Tag;
friend class DefineShape3Tag;
private:
	void TessellateSimple();
	void SetStyles(FILLSTYLE* styles);
	FILLSTYLE* style;
	arrayElem* varray;
public:
	std::vector<Triangle> interior;
	std::vector<std::vector<Vector2> > triangle_strips;
	std::vector<Vector2> outline;
	std::vector<GeomShape> sub_shapes;
	std::vector<Edge> edges;

	bool closed;
	int id;
	int color;

	//DEBUG
	void dumpEdges();
	void dumpInterior();

	void Render(int x=0, int y=0) const;
	void BuildFromEdges(FILLSTYLE* styles);

	bool operator<(const GeomShape& r) const;

};

};

#endif
