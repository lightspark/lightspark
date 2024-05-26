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

union floatVec
{
	uint64_t key;
	struct
	{
		float x;
		float y;
	} vec;
};

struct GeomToken
{
	union
	{
		GEOM_TOKEN_TYPE type;
		struct
		{
			float x;
			float y;
		} vec;
		const FILLSTYLE*  fillStyle; // make sure the pointer is valid until rendering is done
		const LINESTYLE2* lineStyle; // make sure the pointer is valid until rendering is done
		number_t value;
		uint64_t uval;// this is used to have direct access to the value as it is stored in a vector<uint64_t> for performance
	};
	GeomToken(GEOM_TOKEN_TYPE t):type(t) {}
	GeomToken(floatVec& v)
	{
		vec.x = v.vec.x;
		vec.y = v.vec.y;
	}
	GeomToken(uint64_t v, bool /*uint64_indicator*/)
	{
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
	uint16_t currentLineWidth;
	tokensVector():boundsRect(INT32_MAX,INT32_MIN,INT32_MAX,INT32_MIN),currentLineWidth(0)
	{
	}
	tokensVector(const tokensVector& r):filltokens(r.filltokens),stroketokens(r.stroketokens),boundsRect(r.boundsRect),currentLineWidth(r.currentLineWidth)
	{
	}
	void clear()
	{
		boundsRect = RECT(INT32_MAX,INT32_MIN,INT32_MAX,INT32_MIN);
		filltokens.clear();
		stroketokens.clear();
		currentLineWidth=0;
	}
	uint32_t size() const
	{
		return filltokens.size()+stroketokens.size();
	}
	bool empty() const
	{
		return filltokens.empty() && stroketokens.empty();
	}
	void updateTokenBounds(int x, int y);
	bool operator==(const tokensVector& r);
	tokensVector& operator=(const tokensVector& r);
};

class ShapePathSegment {
public:
	floatVec from;
	floatVec quadctrl;
	floatVec to;
	int linestyleindex;
	ShapePathSegment(floatVec from, floatVec quadctrl, floatVec to, int linestyleindex): from(from), quadctrl(quadctrl), to(to),linestyleindex(linestyleindex) {}
	ShapePathSegment()
	{
		from.key=0;
		quadctrl.key=0;
		to.key=0;
		linestyleindex=0;
	}
	ShapePathSegment reverse() const {return ShapePathSegment(to, quadctrl, from, linestyleindex);}
};

class ShapesBuilder
{
private:
	std::vector<ShapePathSegment> currentSubpath;
	std::map< unsigned int, std::vector<ShapePathSegment> > filledShapesMap;
	std::map< unsigned int, std::vector<ShapePathSegment> > strokeShapesMap;

	static bool isOutlineEmpty(const std::vector<ShapePathSegment>& outline);
	static floatVec makeVertex(const Vector2f& v) 
	{ 
		floatVec ret;
		ret.vec.x = v.x;
		ret.vec.y = v.y;
		return ret;
	}
	std::map<uint16_t,LINESTYLE2>::iterator getStrokeLineStyle(const std::list<MORPHLINESTYLE2>::iterator& stylesIt, uint16_t ratio, std::map<uint16_t, LINESTYLE2>* linestylecache, const RECT& boundsrc);
public:
	void extendOutline(const Vector2f& v1, const Vector2f& v2, int linestyleindex);
	void extendOutlineCurve(const Vector2f& start, const Vector2f& control, const Vector2f& end, int linestyleindex);
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
