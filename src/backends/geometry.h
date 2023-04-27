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
	Vector2Tmpl(const Vector2Tmpl<int32_t>& o) : x(o.x),y(o.y) {}
	/* conversion Vector2f -> Vector2 is explicit */
	Vector2Tmpl<int32_t> round() const { return Vector2Tmpl<int32_t>(x,y); }
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

enum GEOM_TOKEN_TYPE { STRAIGHT=0, CURVE_QUADRATIC, MOVE, SET_FILL, SET_STROKE, CLEAR_FILL, CLEAR_STROKE, CURVE_CUBIC, FILL_KEEP_SOURCE, FILL_TRANSFORM_TEXTURE };

struct GeomToken
{
	union
	{
		GEOM_TOKEN_TYPE type;
		struct
		{
			int32_t x;
			int32_t y;
		} vec;
		const FILLSTYLE*  fillStyle; // make sure the pointer is valid until rendering is done
		const LINESTYLE2* lineStyle; // make sure the pointer is valid until rendering is done
		number_t value;
		uint64_t uval;// this is used to have direct access to the value as it is stored in a vector<uint64_t> for performance
	};
	GeomToken(GEOM_TOKEN_TYPE t):type(t) {}
	GeomToken(uint64_t v, bool isvec)
	{
		if (isvec)
		{
			vec.x = (int32_t(v&0xffffffff));
			vec.y = (int32_t(v>>32));
		}
		else
			uval = v;
	}
	GeomToken(const FILLSTYLE& fs):fillStyle(&fs) {}
	GeomToken(const LINESTYLE2& ls):lineStyle(&ls) {}
	GeomToken(number_t val):value(val) {}
	GeomToken(const Vector2& _vec)
	{
		vec.x=_vec.x;
		vec.y=_vec.y;
	}
};

struct tokensVector
{
	std::vector<uint64_t> filltokens;
	std::vector<uint64_t> stroketokens;
	RECT boundsRect;
	bool canRenderToGL;
	tokensVector():canRenderToGL(true) {}
	void clear()
	{
		filltokens.clear();
		stroketokens.clear();
		canRenderToGL=true;
	}
	uint32_t size() const
	{
		return filltokens.size()+stroketokens.size();
	}
	bool empty() const
	{
		return filltokens.empty() && stroketokens.empty();
	}
};


class ShapePathSegment {
public:
	uint64_t from;
	uint64_t quadctrl;
	uint64_t to;
	ShapePathSegment(uint64_t from, uint64_t quadctrl, uint64_t to): from(from), quadctrl(quadctrl), to(to) {}
	ShapePathSegment reverse() const {return ShapePathSegment(to, quadctrl, from);}
};

class ShapesBuilder
{
private:
	std::vector<ShapePathSegment> currentSubpath;
	std::map< unsigned int, std::vector<ShapePathSegment> > filledShapesMap;
	std::map< unsigned int, std::vector<ShapePathSegment> > strokeShapesMap;

	static bool isOutlineEmpty(const std::vector<ShapePathSegment>& outline);
	static uint64_t makeVertex(const Vector2& v) { return (uint64_t(v.y)<<32) | (uint64_t(v.x)&0xffffffff); }
public:
	void extendOutline(const Vector2& v1, const Vector2& v2);
	void extendOutlineCurve(const Vector2& start, const Vector2& control, const Vector2& end);
	void endSubpathForStyles(unsigned fill0, unsigned fill1, unsigned stroke, bool formorphing);
	/**
		Generate a sequence of cachable tokens that defines the geomtries
		@param styles This list is supposed to survive until as long as the returned tokens array
		@param tokens A vector that will be filled with tokens
	*/
	void outputTokens(const std::list<FILLSTYLE>& styles, const std::list<LINESTYLE2>& linestyles, tokensVector& tokens);
	void outputMorphTokens(std::list<MORPHFILLSTYLE>& styles, std::list<MORPHLINESTYLE2>& linestyles, tokensVector& tokens, uint16_t ratio, const RECT& boundsrc);
	void clear();
};

std::ostream& operator<<(std::ostream& s, const Vector2& p);

}

#endif /* BACKENDS_GEOMETRY_H */
