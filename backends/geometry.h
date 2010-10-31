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
#ifndef GEOMETRY_H
#define GEOMETRY_H

#include "compat.h"
#include <list>
#include <iostream>
#include <vector>
#include <map>
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
	Vector2& operator+=(const Vector2& v){ x+=v.x; y+=v.y; return *this;}
	const Vector2 operator*(int p)const { return Vector2(x*p,y*p);}
	Vector2& operator/=(int v) { x/=v; y/=v; return *this;}
	int dot(const Vector2& r) const { return x*r.x+y*r.y;}
};

enum GEOM_TOKEN_TYPE { STRAIGHT=0, CURVE, MOVE, SET_FILL, SET_STROKE };

class GeomToken
{
public:
	GEOM_TOKEN_TYPE type;
	Vector2 p1;
	FILLSTYLE style;
	GeomToken(GEOM_TOKEN_TYPE _t, const Vector2& _p):type(_t),p1(_p),style(-1){}
	GeomToken(GEOM_TOKEN_TYPE _t, const FILLSTYLE _f):type(_t),p1(0,0),style(_f){}
};

class ShapesBuilder
{
private:
	std::map< Vector2, unsigned int > verticesMap;
	std::map< unsigned int, std::vector< std::vector<unsigned int> > > filledShapesMap;
	std::map< unsigned int, std::vector< std::vector<unsigned int> > > strokeShapesMap;
	void joinOutlines();
	static bool isOutlineEmpty(const std::vector<unsigned int>& outline);
	static void extendOutlineForColor(std::map< unsigned int, std::vector< std::vector<Vector2> > >& map);
	const Vector2& getVertex(unsigned int index);
public:
	void extendFilledOutlineForColor(unsigned int fillColor, const Vector2& v1, const Vector2& v2);
	void extendStrokeOutlineForColor(unsigned int stroke, const Vector2& v1, const Vector2& v2);
	/**
		Generate a sequence of cachable tokens that defines the geomtries
		@param styles This list is supposed to survive until as long as the returned tokens array
		@param tokens A vector that will be filled with tokens
	*/
	void outputTokens(const std::list<FILLSTYLE>& styles, std::vector<GeomToken>& tokens);
	void clear();
};

std::ostream& operator<<(std::ostream& s, const Vector2& p);

};

#endif
