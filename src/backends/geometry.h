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
#ifndef BACKENDS_GEOMETRY_H
#define BACKENDS_GEOMETRY_H 1

#include "compat.h"
#include "swftypes.h"
#include <list>
#include <vector>
#include <map>

namespace lightspark
{

template<class T>
class Vector2Tmpl
{

public:
	T x,y;
	Vector2Tmpl(T a=0, T b=0):x(a),y(b){}
	/* conversion Vector2 -> Vector2f is implicit */
	Vector2Tmpl(const Vector2Tmpl<int>& o) : x(o.x),y(o.y) {}
	/* conversion Vector2f -> Vector2 is explicit */
	Vector2Tmpl<int> round() const { return Vector2Tmpl<int>(x,y); }
	bool operator==(const Vector2Tmpl<T>& v)const{return v.x==x && v.y==y;}
	bool operator!=(const Vector2Tmpl<T>& v)const{return v.x!=x || v.y!=y;}
	bool operator<(const Vector2Tmpl<T>& v) const {return (y==v.y)?(x < v.x):(y < v.y);}
	Vector2Tmpl<T> operator-() const { return Vector2Tmpl<T>(-x,-y); }
	Vector2Tmpl<T> operator-(const Vector2Tmpl<T>& v)const { return Vector2Tmpl<T>(x-v.x,y-v.y);}
	Vector2Tmpl<T> operator+(const Vector2Tmpl<T>& v)const { return Vector2Tmpl<T>(x+v.x,y+v.y);}
	Vector2Tmpl<T>& operator+=(const Vector2Tmpl<T>& v){ x+=v.x; y+=v.y; return *this;}
	Vector2Tmpl<T> operator*(int p)const { return Vector2Tmpl<T>(x*p,y*p);}
	Vector2Tmpl<T>& operator/=(T v) { x/=v; y/=v; return *this;}
	int dot(const Vector2Tmpl<T>& r) const { return x*r.x+y*r.y;}
	Vector2Tmpl<T> projectInto(const RECT& r) const
	{
		Vector2Tmpl<T> out;
		out.x = maxTmpl<T>(minTmpl<T>(x,r.Xmax),r.Xmin);
		out.y = maxTmpl<T>(minTmpl<T>(y,r.Ymax),r.Ymin);
		return out;
	}
	std::ostream& operator<<(std::ostream& s)
	{
		s << "{ "<< x << ',' << y << " }";
		return s;
	}
};
typedef Vector2Tmpl<int> Vector2;
typedef Vector2Tmpl<double> Vector2f;

enum GEOM_TOKEN_TYPE { STRAIGHT=0, CURVE_QUADRATIC, MOVE, SET_FILL, SET_STROKE, CLEAR_FILL, CLEAR_STROKE, CURVE_CUBIC, FILL_KEEP_SOURCE, FILL_TRANSFORM_TEXTURE };

class GeomToken
{
public:
	FILLSTYLE  fillStyle;
	LINESTYLE2 lineStyle;
	MATRIX textureTransform;
	GEOM_TOKEN_TYPE type;
	Vector2 p1;
	Vector2 p2;
	Vector2 p3;
	GeomToken(GEOM_TOKEN_TYPE _t):fillStyle(0xff),lineStyle(0xff),type(_t),p1(0,0),p2(0,0),p3(0,0){}
	GeomToken(GEOM_TOKEN_TYPE _t, const Vector2& _p):fillStyle(0xff),lineStyle(0xff),type(_t),p1(_p),p2(0,0),p3(0,0){}
	GeomToken(GEOM_TOKEN_TYPE _t, const Vector2& _p1, const Vector2& _p2):fillStyle(0xff),lineStyle(0xff),type(_t),p1(_p1),p2(_p2),p3(0,0){}
	GeomToken(GEOM_TOKEN_TYPE _t, const Vector2& _p1, const Vector2& _p2, const Vector2& _p3):fillStyle(0xff),lineStyle(0xff),type(_t),
			p1(_p1),p2(_p2),p3(_p3){}
	GeomToken(GEOM_TOKEN_TYPE _t, const FILLSTYLE  _f):fillStyle(_f),lineStyle(0xff),type(_t),p1(0,0),p2(0,0),p3(0,0){}
	GeomToken(GEOM_TOKEN_TYPE _t, const LINESTYLE2 _s):fillStyle(0xff),lineStyle(_s),type(_t),p1(0,0),p2(0,0),p3(0,0){}
	GeomToken(GEOM_TOKEN_TYPE _t, const MATRIX _m):fillStyle(0xff),lineStyle(0xff),textureTransform(_m),type(_t),p1(0,0),p2(0,0),p3(0,0){}
};

typedef std::vector<GeomToken, reporter_allocator<GeomToken>> tokensVector;

enum SHAPE_PATH_SEGMENT_TYPE { PATH_START=0, PATH_STRAIGHT, PATH_CURVE_QUADRATIC };

class ShapePathSegment {
public:
	SHAPE_PATH_SEGMENT_TYPE type;
	unsigned int i;
	ShapePathSegment(SHAPE_PATH_SEGMENT_TYPE _t, unsigned int _i):type(_t),i(_i){}
	bool operator==(const ShapePathSegment& v)const{return v.i == i;}
};

class ShapesBuilder
{
private:
	std::map< Vector2, unsigned int > verticesMap;
	std::map< unsigned int, std::vector< std::vector<ShapePathSegment> > > filledShapesMap;
	std::map< unsigned int, std::vector< std::vector<ShapePathSegment> > > strokeShapesMap;
	void joinOutlines();
	static bool isOutlineEmpty(const std::vector<ShapePathSegment>& outline);
	static void extendOutlineForColor(std::map< unsigned int, std::vector< std::vector<Vector2> > >& map);
	unsigned int makeVertex(const Vector2& v);
	const Vector2& getVertex(unsigned int index);
public:
	void extendFilledOutlineForColor(unsigned int fillColor, const Vector2& v1, const Vector2& v2);
	void extendFilledOutlineForColorCurve(unsigned int color, const Vector2& start, const Vector2& control, const Vector2& end);
	void extendStrokeOutlineForColor(unsigned int stroke, const Vector2& v1, const Vector2& v2);
	/**
		Generate a sequence of cachable tokens that defines the geomtries
		@param styles This list is supposed to survive until as long as the returned tokens array
		@param tokens A vector that will be filled with tokens
	*/
	void outputTokens(const std::list<FILLSTYLE>& styles, tokensVector& tokens);
	void clear();
};

std::ostream& operator<<(std::ostream& s, const Vector2& p);

};

#endif /* BACKENDS_GEOMETRY_H */
