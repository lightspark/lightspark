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

#include "scripting/abc.h"
#include "scripting/flash/geom/flashgeom.h"
#include "scripting/argconv.h"
#include "scripting/toplevel/Vector.h"

using namespace lightspark;
using namespace std;

void Rectangle::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED);
	c->isReusable = true;
	
	c->setDeclaredMethodByQName("left","",Class<IFunction>::getFunction(c->getSystemState(),_getLeft),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("left","",Class<IFunction>::getFunction(c->getSystemState(),_setLeft),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("x","",Class<IFunction>::getFunction(c->getSystemState(),_getLeft),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("x","",Class<IFunction>::getFunction(c->getSystemState(),_setLeft),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("right","",Class<IFunction>::getFunction(c->getSystemState(),_getRight),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("right","",Class<IFunction>::getFunction(c->getSystemState(),_setRight),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("width","",Class<IFunction>::getFunction(c->getSystemState(),_getWidth),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("width","",Class<IFunction>::getFunction(c->getSystemState(),_setWidth),SETTER_METHOD,true);

	c->setDeclaredMethodByQName("top","",Class<IFunction>::getFunction(c->getSystemState(),_getTop),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("top","",Class<IFunction>::getFunction(c->getSystemState(),_setTop),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("y","",Class<IFunction>::getFunction(c->getSystemState(),_getTop),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("y","",Class<IFunction>::getFunction(c->getSystemState(),_setTop),SETTER_METHOD,true);

	c->setDeclaredMethodByQName("bottom","",Class<IFunction>::getFunction(c->getSystemState(),_getBottom),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("bottom","",Class<IFunction>::getFunction(c->getSystemState(),_setBottom),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("height","",Class<IFunction>::getFunction(c->getSystemState(),_getHeight),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("height","",Class<IFunction>::getFunction(c->getSystemState(),_setHeight),SETTER_METHOD,true);

	c->setDeclaredMethodByQName("bottomRight","",Class<IFunction>::getFunction(c->getSystemState(),_getBottomRight),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("bottomRight","",Class<IFunction>::getFunction(c->getSystemState(),_setBottomRight),SETTER_METHOD,true);

	c->setDeclaredMethodByQName("size","",Class<IFunction>::getFunction(c->getSystemState(),_getSize),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("size","",Class<IFunction>::getFunction(c->getSystemState(),_setSize),SETTER_METHOD,true);

	c->setDeclaredMethodByQName("topLeft","",Class<IFunction>::getFunction(c->getSystemState(),_getTopLeft),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("topLeft","",Class<IFunction>::getFunction(c->getSystemState(),_setTopLeft),SETTER_METHOD,true);

	c->setDeclaredMethodByQName("clone","",Class<IFunction>::getFunction(c->getSystemState(),clone),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("contains","",Class<IFunction>::getFunction(c->getSystemState(),contains),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("containsPoint","",Class<IFunction>::getFunction(c->getSystemState(),containsPoint),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("containsRect","",Class<IFunction>::getFunction(c->getSystemState(),containsRect),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("equals","",Class<IFunction>::getFunction(c->getSystemState(),equals),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("inflate","",Class<IFunction>::getFunction(c->getSystemState(),inflate),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("inflatePoint","",Class<IFunction>::getFunction(c->getSystemState(),inflatePoint),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("intersection","",Class<IFunction>::getFunction(c->getSystemState(),intersection),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("intersects","",Class<IFunction>::getFunction(c->getSystemState(),intersects),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("isEmpty","",Class<IFunction>::getFunction(c->getSystemState(),isEmpty),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("offset","",Class<IFunction>::getFunction(c->getSystemState(),offset),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("offsetPoint","",Class<IFunction>::getFunction(c->getSystemState(),offsetPoint),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setEmpty","",Class<IFunction>::getFunction(c->getSystemState(),setEmpty),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("union","",Class<IFunction>::getFunction(c->getSystemState(),_union),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setTo","",Class<IFunction>::getFunction(c->getSystemState(),setTo),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("copyFrom","",Class<IFunction>::getFunction(c->getSystemState(),copyFrom),NORMAL_METHOD,true);
	c->prototype->setVariableByQName("toString","",Class<IFunction>::getFunction(c->getSystemState(),_toString),DYNAMIC_TRAIT);
}

void Rectangle::buildTraits(ASObject* o)
{
}

const lightspark::RECT Rectangle::getRect() const
{
	return lightspark::RECT(x,x+width,y,y+height);
}

bool Rectangle::destruct()
{
	x=0;
	y=0;
	width=0;
	height=0;
	return destructIntern();
}

ASFUNCTIONBODY_ATOM(Rectangle,_constructor)
{
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);

	if(argslen>=1)
		th->x=asAtomHandler::toNumber(args[0]);
	if(argslen>=2)
		th->y=asAtomHandler::toNumber(args[1]);
	if(argslen>=3)
		th->width=asAtomHandler::toNumber(args[2]);
	if(argslen>=4)
		th->height=asAtomHandler::toNumber(args[3]);
}

ASFUNCTIONBODY_ATOM(Rectangle,_getLeft)
{
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	asAtomHandler::setNumber(ret,sys,th->x);
}

ASFUNCTIONBODY_ATOM(Rectangle,_setLeft)
{
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	assert_and_throw(argslen==1);
	th->x=asAtomHandler::toNumber(args[0]);
}

ASFUNCTIONBODY_ATOM(Rectangle,_getRight)
{
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	asAtomHandler::setNumber(ret,sys,th->x + th->width);
}

ASFUNCTIONBODY_ATOM(Rectangle,_setRight)
{
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	assert_and_throw(argslen==1);
	th->width=(asAtomHandler::toNumber(args[0])-th->x);
}

ASFUNCTIONBODY_ATOM(Rectangle,_getWidth)
{
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	asAtomHandler::setNumber(ret,sys,th->width);
}

ASFUNCTIONBODY_ATOM(Rectangle,_setWidth)
{
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	assert_and_throw(argslen==1);
	th->width=asAtomHandler::toNumber(args[0]);
}

ASFUNCTIONBODY_ATOM(Rectangle,_getTop)
{
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	asAtomHandler::setNumber(ret,sys,th->y);
}

ASFUNCTIONBODY_ATOM(Rectangle,_setTop)
{
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	assert_and_throw(argslen==1);
	th->y=asAtomHandler::toNumber(args[0]);
}

ASFUNCTIONBODY_ATOM(Rectangle,_getBottom)
{
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	asAtomHandler::setNumber(ret,sys,th->y + th->height);
}

ASFUNCTIONBODY_ATOM(Rectangle,_setBottom)
{
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	assert_and_throw(argslen==1);
	th->height=(asAtomHandler::toNumber(args[0])-th->y);
}

ASFUNCTIONBODY_ATOM(Rectangle,_getBottomRight)
{
	assert_and_throw(argslen==0);
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	Point* res = Class<Point>::getInstanceS(sys,th->x + th->width, th->y + th->height);
	ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(Rectangle,_setBottomRight)
{
	assert_and_throw(argslen == 1);
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	Point* br = asAtomHandler::as<Point>(args[0]);
	th->width = br->getX() - th->x;
	th->height = br->getY() - th->y;
}

ASFUNCTIONBODY_ATOM(Rectangle,_getTopLeft)
{
	assert_and_throw(argslen==0);
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	Point* res = Class<Point>::getInstanceS(sys,th->x, th->y);
	ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(Rectangle,_setTopLeft)
{
	assert_and_throw(argslen == 1);
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	Point* br = asAtomHandler::as<Point>(args[0]);
	th->width = br->getX();
	th->height = br->getY();
}

ASFUNCTIONBODY_ATOM(Rectangle,_getSize)
{
	assert_and_throw(argslen==0);
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	Point* res = Class<Point>::getInstanceS(sys,th->width, th->height);
	ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(Rectangle,_setSize)
{
	assert_and_throw(argslen == 1);
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	Point* br = asAtomHandler::as<Point>(args[0]);
	th->width = br->getX();
	th->height = br->getY();
}

ASFUNCTIONBODY_ATOM(Rectangle,_getHeight)
{
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	asAtomHandler::setNumber(ret,sys,th->height);
}

ASFUNCTIONBODY_ATOM(Rectangle,_setHeight)
{
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	assert_and_throw(argslen==1);
	th->height=asAtomHandler::toNumber(args[0]);
}

ASFUNCTIONBODY_ATOM(Rectangle,clone)
{
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	Rectangle* res=Class<Rectangle>::getInstanceS(sys);
	res->x=th->x;
	res->y=th->y;
	res->width=th->width;
	res->height=th->height;
	ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(Rectangle,contains)
{
	assert_and_throw(argslen == 2);
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	number_t x = asAtomHandler::toNumber(args[0]);
	number_t y = asAtomHandler::toNumber(args[1]);

	asAtomHandler::setBool(ret,th->x <= x && x <= th->x + th->width
						&& th->y <= y && y <= th->y + th->height );
}

ASFUNCTIONBODY_ATOM(Rectangle,containsPoint)
{
	assert_and_throw(argslen == 1);
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	Point* br = asAtomHandler::as<Point>(args[0]);
	number_t x = br->getX();
	number_t y = br->getY();

	asAtomHandler::setBool(ret,th->x <= x && x <= th->x + th->width
						&& th->y <= y && y <= th->y + th->height );
}

ASFUNCTIONBODY_ATOM(Rectangle,containsRect)
{
	assert_and_throw(argslen == 1);
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	Rectangle* cr = asAtomHandler::as<Rectangle>(args[0]);

	asAtomHandler::setBool(ret,th->x <= cr->x && cr->x + cr->width <= th->x + th->width
						&& th->y <= cr->y && cr->y + cr->height <= th->y + th->height );
}

ASFUNCTIONBODY_ATOM(Rectangle,equals)
{
	assert_and_throw(argslen == 1);
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	Rectangle* co = asAtomHandler::as<Rectangle>(args[0]);

	asAtomHandler::setBool(ret,th->x == co->x && th->width == co->width
						&& th->y == co->y && th->height == co->height );
}

ASFUNCTIONBODY_ATOM(Rectangle,inflate)
{
	assert_and_throw(argslen == 2);
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	number_t dx = asAtomHandler::toNumber(args[0]);
	number_t dy = asAtomHandler::toNumber(args[1]);

	th->x -= dx;
	th->width += 2 * dx;
	th->y -= dy;
	th->height += 2 * dy;
}

ASFUNCTIONBODY_ATOM(Rectangle,inflatePoint)
{
	assert_and_throw(argslen == 1);
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	Point* po = asAtomHandler::as<Point>(args[0]);
	number_t dx = po->getX();
	number_t dy = po->getY();

	th->x -= dx;
	th->width += 2 * dx;
	th->y -= dy;
	th->height += 2 * dy;
}

ASFUNCTIONBODY_ATOM(Rectangle,intersection)
{
	assert_and_throw(argslen == 1);
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	Rectangle* ti = asAtomHandler::as<Rectangle>(args[0]);
	Rectangle* res = Class<Rectangle>::getInstanceS(sys);

	number_t thtop = th->y;
	number_t thleft = th->x;
	number_t thright = th->x + th->width;
	number_t thbottom = th->y + th->height;

	number_t titop = ti->y;
	number_t tileft = ti->x;
	number_t tiright = ti->x + ti->width;
	number_t tibottom = ti->y + ti->height;

	if ( thtop > tibottom || thright < tileft ||
						thbottom < titop || thleft > tiright )
	{
		// rectangles don't intersect
		res->x = 0;
		res->y = 0;
		res->width = 0;
		res->height = 0;
		ret = asAtomHandler::fromObject(res);
		return;
	}

	Rectangle* leftmost = ti;
	Rectangle* rightmost = th;

	// find left most
	if ( thleft < tileft )
	{
		leftmost = th;
		rightmost = ti;
	}

	Rectangle* topmost = ti;
	Rectangle* bottommost = th;

	// find top most
	if ( thtop < titop )
	{
		topmost = th;
		bottommost = ti;
	}

	res->x = rightmost->x;
	res->width = min(leftmost->x + leftmost->width, rightmost->x + rightmost->width) - rightmost->x;
	res->y = bottommost->y;
	res->height = min(topmost->y + topmost->height, bottommost->y + bottommost->height) - bottommost->y;

	ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(Rectangle,intersects)
{
	assert_and_throw(argslen == 1);
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	Rectangle* ti = asAtomHandler::as<Rectangle>(args[0]);

	number_t thtop = th->y;
	number_t thleft = th->x;
	number_t thright = th->x + th->width;
	number_t thbottom = th->y + th->height;

	number_t titop = ti->y;
	number_t tileft = ti->x;
	number_t tiright = ti->x + ti->width;
	number_t tibottom = ti->y + ti->height;

	asAtomHandler::setBool(ret,!(thtop > tibottom || thright < tileft ||
						thbottom < titop || thleft > tiright) );
}

ASFUNCTIONBODY_ATOM(Rectangle,isEmpty)
{
	assert_and_throw(argslen == 0);
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);

	asAtomHandler::setBool(ret,th->width <= 0 || th->height <= 0 );
}

ASFUNCTIONBODY_ATOM(Rectangle,offset)
{
	assert_and_throw(argslen == 2);
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);

	th->x += asAtomHandler::toNumber(args[0]);
	th->y += asAtomHandler::toNumber(args[1]);
}

ASFUNCTIONBODY_ATOM(Rectangle,offsetPoint)
{
	assert_and_throw(argslen == 1);
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	Point* po = asAtomHandler::as<Point>(args[0]);

	th->x += po->getX();
	th->y += po->getY();
}

ASFUNCTIONBODY_ATOM(Rectangle,setEmpty)
{
	assert_and_throw(argslen == 0);
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);

	th->x = 0;
	th->y = 0;
	th->width = 0;
	th->height = 0;
}

ASFUNCTIONBODY_ATOM(Rectangle,_union)
{
	assert_and_throw(argslen == 1);
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	Rectangle* ti = asAtomHandler::as<Rectangle>(args[0]);
	Rectangle* res = Class<Rectangle>::getInstanceS(sys);

	res->x = th->x;
	res->y = th->y;
	res->width = th->width;
	res->height = th->height;

	if ( ti->width == 0 || ti->height == 0 )
	{
		ret = asAtomHandler::fromObject(res);
		return;
	}

	res->x = min(th->x, ti->x);
	res->y = min(th->y, ti->y);
	res->width = max(th->x + th->width, ti->x + ti->width);
	res->height = max(th->y + th->height, ti->y + ti->height);

	ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(Rectangle,_toString)
{
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	tiny_string s = "(x=";
	s+= Number::toString(th->x);
	s+= ", y=";
	s+= Number::toString(th->y);
	s+= ", w=";
	s+= Number::toString(th->width);
	s+= ", h=";
	s+= Number::toString(th->height);
	s+= ")";
	ret = asAtomHandler::fromObject(abstract_s(sys,s));
}

ASFUNCTIONBODY_ATOM(Rectangle,setTo)
{
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	number_t x,y,w,h;
	ARG_UNPACK_ATOM(x)(y)(w)(h);
	th->x = x;
	th->y = y;
	th->width = w;
	th->height = h;
}
ASFUNCTIONBODY_ATOM(Rectangle,copyFrom)
{
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	_NR<Rectangle> sourcerect;
	ARG_UNPACK_ATOM(sourcerect);
	if (!sourcerect.isNull())
	{
		th->x = sourcerect->x;
		th->y = sourcerect->y;
		th->width = sourcerect->width;
		th->height = sourcerect->height;
	}
}

ColorTransform::ColorTransform(Class_base* c, const CXFORMWITHALPHA& cx)
  : ASObject(c,T_OBJECT,SUBTYPE_COLORTRANSFORM)
{
	cx.getParameters(redMultiplier, greenMultiplier, blueMultiplier, alphaMultiplier,
					 redOffset, greenOffset, blueOffset, alphaOffset);
}

void ColorTransform::applyTransformation(const RGBA& color, float& r, float& g, float& b, float &a)
{
	a = max(0.0,min(255.0,((color.Alpha * alphaMultiplier * 256.0)/256.0 + alphaOffset)))/256.0;
	r = max(0.0,min(255.0,((color.Red   *   redMultiplier * 256.0)/256.0 +   redOffset)))/256.0;
	g = max(0.0,min(255.0,((color.Green * greenMultiplier * 256.0)/256.0 + greenOffset)))/256.0;
	b = max(0.0,min(255.0,((color.Blue  *  blueMultiplier * 256.0)/256.0 +  blueOffset)))/256.0;
	
}

uint8_t *ColorTransform::applyTransformation(BitmapContainer* bm)
{
	if (redMultiplier==1.0 &&
		greenMultiplier==1.0 &&
		blueMultiplier==1.0 &&
		alphaMultiplier==1.0 &&
		redOffset==0.0 &&
		greenOffset==0.0 &&
		blueOffset==0.0 &&
		alphaOffset==0.0)
		return (uint8_t*)bm->getData();

	uint32_t* src = (uint32_t*)bm->getData();
	uint32_t* dst = (uint32_t*)bm->getDataColorTransformed();
	uint32_t size = bm->getWidth()*bm->getHeight();
	for (uint32_t i = 0; i < size; i++)
	{
		uint32_t color = *src;
		*dst =  max(0,min(255,(((int)((color>>24)&0xff) * (int)alphaMultiplier)/256 + (int)alphaOffset)))<<24 |
				max(0,min(255,(((int)((color>>16)&0xff) * (int)  redMultiplier)/256 + (int)  redOffset)))<<16 |
				max(0,min(255,(((int)((color>> 8)&0xff) * (int)greenMultiplier)/256 + (int)greenOffset)))<<8 |
				max(0,min(255,(((int)((color    )&0xff) * (int) blueMultiplier)/256 + (int) blueOffset)));
		dst++;
		src++;
	}
	return (uint8_t*)bm->getDataColorTransformed();
}

void ColorTransform::setProperties(const CXFORMWITHALPHA &cx)
{
	cx.getParameters(redMultiplier, greenMultiplier, blueMultiplier, alphaMultiplier,
					 redOffset, greenOffset, blueOffset, alphaOffset);
}

void ColorTransform::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED);
	c->isReusable = true;

	// properties
	c->setDeclaredMethodByQName("color","",Class<IFunction>::getFunction(c->getSystemState(),getColor),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("color","",Class<IFunction>::getFunction(c->getSystemState(),setColor),SETTER_METHOD,true);

	c->setDeclaredMethodByQName("redMultiplier","",Class<IFunction>::getFunction(c->getSystemState(),getRedMultiplier),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("redMultiplier","",Class<IFunction>::getFunction(c->getSystemState(),setRedMultiplier),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("greenMultiplier","",Class<IFunction>::getFunction(c->getSystemState(),getGreenMultiplier),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("greenMultiplier","",Class<IFunction>::getFunction(c->getSystemState(),setGreenMultiplier),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("blueMultiplier","",Class<IFunction>::getFunction(c->getSystemState(),getBlueMultiplier),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("blueMultiplier","",Class<IFunction>::getFunction(c->getSystemState(),setBlueMultiplier),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("alphaMultiplier","",Class<IFunction>::getFunction(c->getSystemState(),getAlphaMultiplier),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("alphaMultiplier","",Class<IFunction>::getFunction(c->getSystemState(),setAlphaMultiplier),SETTER_METHOD,true);

	c->setDeclaredMethodByQName("redOffset","",Class<IFunction>::getFunction(c->getSystemState(),getRedOffset),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("redOffset","",Class<IFunction>::getFunction(c->getSystemState(),setRedOffset),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("greenOffset","",Class<IFunction>::getFunction(c->getSystemState(),getGreenOffset),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("greenOffset","",Class<IFunction>::getFunction(c->getSystemState(),setGreenOffset),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("blueOffset","",Class<IFunction>::getFunction(c->getSystemState(),getBlueOffset),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("blueOffset","",Class<IFunction>::getFunction(c->getSystemState(),setBlueOffset),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("alphaOffset","",Class<IFunction>::getFunction(c->getSystemState(),getAlphaOffset),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("alphaOffset","",Class<IFunction>::getFunction(c->getSystemState(),setAlphaOffset),SETTER_METHOD,true);

	// methods
	c->setDeclaredMethodByQName("concat","",Class<IFunction>::getFunction(c->getSystemState(),concat),NORMAL_METHOD,true);
	c->prototype->setVariableByQName("toString","",Class<IFunction>::getFunction(c->getSystemState(),_toString),DYNAMIC_TRAIT);
}

void ColorTransform::buildTraits(ASObject* o)
{
}

bool ColorTransform::destruct()
{
	redMultiplier=1.0;
	greenMultiplier=1.0;
	blueMultiplier=1.0;
	alphaMultiplier=1.0;
	redOffset=0.0;
	greenOffset=0.0;
	blueOffset=0.0;
	alphaOffset=0.0;
	return ASObject::destruct();
}

ASFUNCTIONBODY_ATOM(ColorTransform,_constructor)
{
	ColorTransform* th=asAtomHandler::as<ColorTransform>(obj);
	assert_and_throw(argslen<=8);
	if(0 < argslen)
		th->redMultiplier=asAtomHandler::toNumber(args[0]);
	else
		th->redMultiplier=1.0;
	if(1 < argslen)
		th->greenMultiplier=asAtomHandler::toNumber(args[1]);
	else
		th->greenMultiplier=1.0;
	if(2 < argslen)
		th->blueMultiplier=asAtomHandler::toNumber(args[2]);
	else
		th->blueMultiplier=1.0;
	if(3 < argslen)
		th->alphaMultiplier=asAtomHandler::toNumber(args[3]);
	else
		th->alphaMultiplier=1.0;
	if(4 < argslen)
		th->redOffset=asAtomHandler::toNumber(args[4]);
	else
		th->redOffset=0.0;
	if(5 < argslen)
		th->greenOffset=asAtomHandler::toNumber(args[5]);
	else
		th->greenOffset=0.0;
	if(6 < argslen)
		th->blueOffset=asAtomHandler::toNumber(args[6]);
	else
		th->blueOffset=0.0;
	if(7 < argslen)
		th->alphaOffset=asAtomHandler::toNumber(args[7]);
	else
		th->alphaOffset=0.0;
}

ASFUNCTIONBODY_ATOM(ColorTransform,setColor)
{
	ColorTransform* th=asAtomHandler::as<ColorTransform>(obj);
	assert_and_throw(argslen==1);
	uint32_t tmp=asAtomHandler::toInt(args[0]);
	//Setting multiplier to 0
	th->redMultiplier=0;
	th->greenMultiplier=0;
	th->blueMultiplier=0;
	th->alphaMultiplier=0;
	//Setting offset to the input value
	th->alphaOffset=(tmp>>24)&0xff;
	th->redOffset=(tmp>>16)&0xff;
	th->greenOffset=(tmp>>8)&0xff;
	th->blueOffset=tmp&0xff;
}

ASFUNCTIONBODY_ATOM(ColorTransform,getColor)
{
	ColorTransform* th=asAtomHandler::as<ColorTransform>(obj);

	int ao, ro, go, bo;
	ao = static_cast<int>(th->alphaOffset) & 0xff;
	ro = static_cast<int>(th->redOffset) & 0xff;
	go = static_cast<int>(th->greenOffset) & 0xff;
	bo = static_cast<int>(th->blueOffset) & 0xff;

	number_t color = (ao<<24) | (ro<<16) | (go<<8) | bo;

	asAtomHandler::setNumber(ret,sys,color);
}

ASFUNCTIONBODY_ATOM(ColorTransform,getRedMultiplier)
{
	ColorTransform* th=asAtomHandler::as<ColorTransform>(obj);
	asAtomHandler::setNumber(ret,sys,th->redMultiplier);
}

ASFUNCTIONBODY_ATOM(ColorTransform,setRedMultiplier)
{
	assert_and_throw(argslen==1);
	ColorTransform* th=asAtomHandler::as<ColorTransform>(obj);
	th->redMultiplier = asAtomHandler::toNumber(args[0]);
}

ASFUNCTIONBODY_ATOM(ColorTransform,getGreenMultiplier)
{
	ColorTransform* th=asAtomHandler::as<ColorTransform>(obj);
	asAtomHandler::setNumber(ret,sys,th->greenMultiplier);
}

ASFUNCTIONBODY_ATOM(ColorTransform,setGreenMultiplier)
{
	assert_and_throw(argslen==1);
	ColorTransform* th=asAtomHandler::as<ColorTransform>(obj);
	th->greenMultiplier = asAtomHandler::toNumber(args[0]);
}

ASFUNCTIONBODY_ATOM(ColorTransform,getBlueMultiplier)
{
	ColorTransform* th=asAtomHandler::as<ColorTransform>(obj);
	asAtomHandler::setNumber(ret,sys,th->blueMultiplier);
}

ASFUNCTIONBODY_ATOM(ColorTransform,setBlueMultiplier)
{
	assert_and_throw(argslen==1);
	ColorTransform* th=asAtomHandler::as<ColorTransform>(obj);
	th->blueMultiplier = asAtomHandler::toNumber(args[0]);
}

ASFUNCTIONBODY_ATOM(ColorTransform,getAlphaMultiplier)
{
	ColorTransform* th=asAtomHandler::as<ColorTransform>(obj);
	asAtomHandler::setNumber(ret,sys,th->alphaMultiplier);
}

ASFUNCTIONBODY_ATOM(ColorTransform,setAlphaMultiplier)
{
	assert_and_throw(argslen==1);
	ColorTransform* th=asAtomHandler::as<ColorTransform>(obj);
	th->alphaMultiplier = asAtomHandler::toNumber(args[0]);
}

ASFUNCTIONBODY_ATOM(ColorTransform,getRedOffset)
{
	ColorTransform* th=asAtomHandler::as<ColorTransform>(obj);
	asAtomHandler::setNumber(ret,sys,th->redOffset);
}

ASFUNCTIONBODY_ATOM(ColorTransform,setRedOffset)
{
	assert_and_throw(argslen==1);
	ColorTransform* th=asAtomHandler::as<ColorTransform>(obj);
	th->redOffset = asAtomHandler::toNumber(args[0]);
}

ASFUNCTIONBODY_ATOM(ColorTransform,getGreenOffset)
{
	ColorTransform* th=asAtomHandler::as<ColorTransform>(obj);
	asAtomHandler::setNumber(ret,sys,th->greenOffset);
}

ASFUNCTIONBODY_ATOM(ColorTransform,setGreenOffset)
{
	assert_and_throw(argslen==1);
	ColorTransform* th=asAtomHandler::as<ColorTransform>(obj);
	th->greenOffset = asAtomHandler::toNumber(args[0]);
}

ASFUNCTIONBODY_ATOM(ColorTransform,getBlueOffset)
{
	ColorTransform* th=asAtomHandler::as<ColorTransform>(obj);
	asAtomHandler::setNumber(ret,sys,th->blueOffset);
}

ASFUNCTIONBODY_ATOM(ColorTransform,setBlueOffset)
{
	assert_and_throw(argslen==1);
	ColorTransform* th=asAtomHandler::as<ColorTransform>(obj);
	th->blueOffset = asAtomHandler::toNumber(args[0]);
}

ASFUNCTIONBODY_ATOM(ColorTransform,getAlphaOffset)
{
	ColorTransform* th=asAtomHandler::as<ColorTransform>(obj);
	asAtomHandler::setNumber(ret,sys,th->alphaOffset);
}

ASFUNCTIONBODY_ATOM(ColorTransform,setAlphaOffset)
{
	assert_and_throw(argslen==1);
	ColorTransform* th=asAtomHandler::as<ColorTransform>(obj);
	th->alphaOffset = asAtomHandler::toNumber(args[0]);
}

ASFUNCTIONBODY_ATOM(ColorTransform,concat)
{
	assert_and_throw(argslen==1);
	ColorTransform* th=asAtomHandler::as<ColorTransform>(obj);
	ColorTransform* ct=asAtomHandler::as<ColorTransform>(args[0]);
	th->redMultiplier *= ct->redMultiplier;
	th->redOffset = th->redOffset * ct->redMultiplier + ct->redOffset;
	th->greenMultiplier *= ct->greenMultiplier;
	th->greenOffset = th->greenOffset * ct->greenMultiplier + ct->greenOffset;
	th->blueMultiplier *= ct->blueMultiplier;
	th->blueOffset = th->blueOffset * ct->blueMultiplier + ct->blueOffset;
	th->alphaMultiplier *= ct->alphaMultiplier;
	th->alphaOffset = th->alphaOffset * ct->alphaMultiplier + ct->alphaOffset;
}

ASFUNCTIONBODY_ATOM(ColorTransform,_toString)
{
	ColorTransform* th=asAtomHandler::as<ColorTransform>(obj);
	char buf[1024];
	snprintf(buf,1024,"(redOffset=%f, redMultiplier=%f, greenOffset=%f, greenMultiplier=%f blueOffset=%f blueMultiplier=%f alphaOffset=%f, alphaMultiplier=%f)",
			th->redOffset, th->redMultiplier, th->greenOffset, th->greenMultiplier, th->blueOffset, th->blueMultiplier, th->alphaOffset, th->alphaMultiplier);
	
	ret = asAtomHandler::fromObject(abstract_s(sys,buf));
}

void Point::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED);
	c->isReusable = true;
	c->setDeclaredMethodByQName("x","",Class<IFunction>::getFunction(c->getSystemState(),_getX),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("y","",Class<IFunction>::getFunction(c->getSystemState(),_getY),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("length","",Class<IFunction>::getFunction(c->getSystemState(),_getlength),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("x","",Class<IFunction>::getFunction(c->getSystemState(),_setX),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("y","",Class<IFunction>::getFunction(c->getSystemState(),_setY),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("interpolate","",Class<IFunction>::getFunction(c->getSystemState(),interpolate),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("distance","",Class<IFunction>::getFunction(c->getSystemState(),distance),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("add","",Class<IFunction>::getFunction(c->getSystemState(),add),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("subtract","",Class<IFunction>::getFunction(c->getSystemState(),subtract),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("clone","",Class<IFunction>::getFunction(c->getSystemState(),clone),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("equals","",Class<IFunction>::getFunction(c->getSystemState(),equals),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("normalize","",Class<IFunction>::getFunction(c->getSystemState(),normalize),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("offset","",Class<IFunction>::getFunction(c->getSystemState(),offset),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("polar","",Class<IFunction>::getFunction(c->getSystemState(),polar),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("setTo","",Class<IFunction>::getFunction(c->getSystemState(),setTo),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("copyFrom","",Class<IFunction>::getFunction(c->getSystemState(),copyFrom),NORMAL_METHOD,true);
	c->prototype->setVariableByQName("toString","",Class<IFunction>::getFunction(c->getSystemState(),_toString),DYNAMIC_TRAIT);
}

void Point::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY_ATOM(Point,_toString)
{
	Point* th=asAtomHandler::as<Point>(obj);
	char buf[512];
	snprintf(buf,512,"(a=%f, b=%f)",th->x,th->y);
	ret = asAtomHandler::fromObject(abstract_s(sys,buf));
}

number_t Point::lenImpl(number_t x, number_t y)
{
	return sqrt(x*x + y*y);
}

bool Point::destruct()
{
	x=0;
	y=0;
	return ASObject::destruct();
}

number_t Point::len() const
{
	return lenImpl(x, y);
}

ASFUNCTIONBODY_ATOM(Point,_constructor)
{
	Point* th=asAtomHandler::as<Point>(obj);
	if(argslen>=1)
		th->x=asAtomHandler::toNumber(args[0]);
	if(argslen>=2)
		th->y=asAtomHandler::toNumber(args[1]);
}

ASFUNCTIONBODY_ATOM(Point,_getX)
{
	Point* th=asAtomHandler::as<Point>(obj);
	asAtomHandler::setNumber(ret,sys,th->x);
}

ASFUNCTIONBODY_ATOM(Point,_getY)
{
	Point* th=asAtomHandler::as<Point>(obj);
	asAtomHandler::setNumber(ret,sys,th->y);
}

ASFUNCTIONBODY_ATOM(Point,_setX)
{
	Point* th=asAtomHandler::as<Point>(obj);
	assert_and_throw(argslen==1);
	th->x = asAtomHandler::toNumber(args[0]);
}

ASFUNCTIONBODY_ATOM(Point,_setY)
{
	Point* th=asAtomHandler::as<Point>(obj);
	assert_and_throw(argslen==1);
	th->y = asAtomHandler::toNumber(args[0]);
}

ASFUNCTIONBODY_ATOM(Point,_getlength)
{
	Point* th=asAtomHandler::as<Point>(obj);
	assert_and_throw(argslen==0);
	asAtomHandler::setNumber(ret,sys,th->len());
}

ASFUNCTIONBODY_ATOM(Point,interpolate)
{
	assert_and_throw(argslen==3);
	Point* pt1=asAtomHandler::as<Point>(args[0]);
	Point* pt2=asAtomHandler::as<Point>(args[1]);
	number_t f=asAtomHandler::toNumber(args[2]);
	Point* res=Class<Point>::getInstanceS(sys);
	res->x = pt1->x + pt2->x * f;
	res->y = pt1->y + pt2->y * f;
	ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(Point,distance)
{
	assert_and_throw(argslen==2);
	Point* pt1=asAtomHandler::as<Point>(args[0]);
	Point* pt2=asAtomHandler::as<Point>(args[1]);
	number_t res=lenImpl(pt2->x - pt1->x, pt2->y - pt1->y);
	asAtomHandler::setNumber(ret,sys,res);
}

ASFUNCTIONBODY_ATOM(Point,add)
{
	Point* th=asAtomHandler::as<Point>(obj);
	assert_and_throw(argslen==1);
	Point* v=asAtomHandler::as<Point>(args[0]);
	Point* res=Class<Point>::getInstanceS(sys);
	res->x = th->x + v->x;
	res->y = th->y + v->y;
	ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(Point,subtract)
{
	Point* th=asAtomHandler::as<Point>(obj);
	assert_and_throw(argslen==1);
	Point* v=asAtomHandler::as<Point>(args[0]);
	Point* res=Class<Point>::getInstanceS(sys);
	res->x = th->x - v->x;
	res->y = th->y - v->y;
	ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(Point,clone)
{
	Point* th=asAtomHandler::as<Point>(obj);
	assert_and_throw(argslen==0);
	Point* res=Class<Point>::getInstanceS(sys);
	res->x = th->x;
	res->y = th->y;
	ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(Point,equals)
{
	Point* th=asAtomHandler::as<Point>(obj);
	assert_and_throw(argslen==1);
	Point* toCompare=asAtomHandler::as<Point>(args[0]);
	asAtomHandler::setBool(ret,(th->x == toCompare->x) & (th->y == toCompare->y));
}

ASFUNCTIONBODY_ATOM(Point,normalize)
{
	Point* th=asAtomHandler::as<Point>(obj);
	assert_and_throw(argslen<2);
	number_t thickness = argslen > 0 ? asAtomHandler::toNumber(args[0]) : 1.0;
	number_t len = th->len();
	th->x = len == 0 ? 0 : th->x * thickness / len;
	th->y = len == 0 ? 0 : th->y * thickness / len;
}

ASFUNCTIONBODY_ATOM(Point,offset)
{
	Point* th=asAtomHandler::as<Point>(obj);
	assert_and_throw(argslen==2);
	number_t dx = asAtomHandler::toNumber(args[0]);
	number_t dy = asAtomHandler::toNumber(args[1]);
	th->x += dx;
	th->y += dy;
}

ASFUNCTIONBODY_ATOM(Point,polar)
{
	assert_and_throw(argslen==2);
	number_t len = asAtomHandler::toNumber(args[0]);
	number_t angle = asAtomHandler::toNumber(args[1]);
	Point* res=Class<Point>::getInstanceS(sys);
	res->x = len * cos(angle);
	res->y = len * sin(angle);
	ret = asAtomHandler::fromObject(res);
}
ASFUNCTIONBODY_ATOM(Point,setTo)
{
	Point* th=asAtomHandler::as<Point>(obj);
	number_t x;
	number_t y;
	ARG_UNPACK_ATOM(x)(y);
	th->x = x;
	th->y = y;
}
ASFUNCTIONBODY_ATOM(Point,copyFrom)
{
	Point* th=asAtomHandler::as<Point>(obj);
	_NR<Point> sourcepoint;
	ARG_UNPACK_ATOM(sourcepoint);
	if (!sourcepoint.isNull())
	{
		th->x = sourcepoint->x;
		th->y = sourcepoint->y;
	}
}

Transform::Transform(Class_base* c):ASObject(c),perspectiveProjection(Class<PerspectiveProjection>::getInstanceSNoArgs(c->getSystemState()))
{
}
Transform::Transform(Class_base* c, _R<DisplayObject> o):ASObject(c),owner(o),perspectiveProjection(Class<PerspectiveProjection>::getInstanceSNoArgs(c->getSystemState()))
{
}

bool Transform::destruct()
{
	owner.reset();
	perspectiveProjection.reset();
	matrix3D.reset();
	return destructIntern();
}

void Transform::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED);
	c->isReusable = true;
	c->setDeclaredMethodByQName("colorTransform","",Class<IFunction>::getFunction(c->getSystemState(),_getColorTransform),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("colorTransform","",Class<IFunction>::getFunction(c->getSystemState(),_setColorTransform),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("matrix","",Class<IFunction>::getFunction(c->getSystemState(),_setMatrix),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("matrix","",Class<IFunction>::getFunction(c->getSystemState(),_getMatrix),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("concatenatedMatrix","",Class<IFunction>::getFunction(c->getSystemState(),_getConcatenatedMatrix),GETTER_METHOD,true);
	REGISTER_GETTER_SETTER(c, perspectiveProjection);
	REGISTER_GETTER_SETTER(c, matrix3D);
}
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(Transform, perspectiveProjection);
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(Transform, matrix3D);

ASFUNCTIONBODY_ATOM(Transform,_constructor)
{
	Transform* th=asAtomHandler::as<Transform>(obj);
	// it's not in the specs but it seems to be possible to construct a Transform object with an owner as argment
	ARG_UNPACK_ATOM(th->owner,NullRef);
}

ASFUNCTIONBODY_ATOM(Transform,_getMatrix)
{
	Transform* th=asAtomHandler::as<Transform>(obj);
	assert_and_throw(argslen==0);
	if (th->owner->matrix.isNull())
	{
		asAtomHandler::setNull(ret);
		return;
	}
	const MATRIX& res=th->owner->getMatrix();
	ret = asAtomHandler::fromObject(Class<Matrix>::getInstanceS(sys,res));
}

ASFUNCTIONBODY_ATOM(Transform,_setMatrix)
{
	Transform* th=asAtomHandler::as<Transform>(obj);
	_NR<Matrix> m;
	ARG_UNPACK_ATOM(m);
	th->owner->setMatrix(m);
}

ASFUNCTIONBODY_ATOM(Transform,_getColorTransform)
{
	Transform* th=asAtomHandler::as<Transform>(obj);
	if (th->owner->colorTransform.isNull())
		th->owner->colorTransform = _NR<ColorTransform>(Class<ColorTransform>::getInstanceS(sys));
	th->owner->colorTransform->incRef();
	ret = asAtomHandler::fromObject(th->owner->colorTransform.getPtr());
}

ASFUNCTIONBODY_ATOM(Transform,_setColorTransform)
{
	Transform* th=asAtomHandler::as<Transform>(obj);
	_NR<ColorTransform> ct;
	ARG_UNPACK_ATOM(ct);
	if (ct.isNull())
		throwError<TypeError>(kNullPointerError, "colorTransform");

	ct->incRef();
	th->owner->colorTransform = ct;
}

ASFUNCTIONBODY_ATOM(Transform,_getConcatenatedMatrix)
{
	Transform* th=asAtomHandler::as<Transform>(obj);
	ret = asAtomHandler::fromObject(Class<Matrix>::getInstanceS(sys,th->owner->getConcatenatedMatrix()));
}

void Transform::buildTraits(ASObject* o)
{
}

Matrix::Matrix(Class_base* c):ASObject(c,T_OBJECT,SUBTYPE_MATRIX)
{
}

Matrix::Matrix(Class_base* c, const MATRIX& m):ASObject(c,T_OBJECT,SUBTYPE_MATRIX),matrix(m)
{
}

void Matrix::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED);
	
	//Properties
	c->setDeclaredMethodByQName("a","",Class<IFunction>::getFunction(c->getSystemState(),_get_a),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("b","",Class<IFunction>::getFunction(c->getSystemState(),_get_b),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("c","",Class<IFunction>::getFunction(c->getSystemState(),_get_c),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("d","",Class<IFunction>::getFunction(c->getSystemState(),_get_d),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("tx","",Class<IFunction>::getFunction(c->getSystemState(),_get_tx),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("ty","",Class<IFunction>::getFunction(c->getSystemState(),_get_ty),GETTER_METHOD,true);
	
	c->setDeclaredMethodByQName("a","",Class<IFunction>::getFunction(c->getSystemState(),_set_a),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("b","",Class<IFunction>::getFunction(c->getSystemState(),_set_b),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("c","",Class<IFunction>::getFunction(c->getSystemState(),_set_c),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("d","",Class<IFunction>::getFunction(c->getSystemState(),_set_d),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("tx","",Class<IFunction>::getFunction(c->getSystemState(),_set_tx),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("ty","",Class<IFunction>::getFunction(c->getSystemState(),_set_ty),SETTER_METHOD,true);
	
	//Methods 
	c->setDeclaredMethodByQName("clone","",Class<IFunction>::getFunction(c->getSystemState(),clone),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("concat","",Class<IFunction>::getFunction(c->getSystemState(),concat),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("copyFrom","",Class<IFunction>::getFunction(c->getSystemState(),copyFrom),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("createBox","",Class<IFunction>::getFunction(c->getSystemState(),createBox),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("createGradientBox","",Class<IFunction>::getFunction(c->getSystemState(),createGradientBox),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("deltaTransformPoint","",Class<IFunction>::getFunction(c->getSystemState(),deltaTransformPoint),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("identity","",Class<IFunction>::getFunction(c->getSystemState(),identity),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("invert","",Class<IFunction>::getFunction(c->getSystemState(),invert),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("rotate","",Class<IFunction>::getFunction(c->getSystemState(),rotate),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("scale","",Class<IFunction>::getFunction(c->getSystemState(),scale),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setTo","",Class<IFunction>::getFunction(c->getSystemState(),setTo),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("transformPoint","",Class<IFunction>::getFunction(c->getSystemState(),transformPoint),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("translate","",Class<IFunction>::getFunction(c->getSystemState(),translate),NORMAL_METHOD,true);
	c->prototype->setVariableByQName("toString","",Class<IFunction>::getFunction(c->getSystemState(),_toString),DYNAMIC_TRAIT);
}

ASFUNCTIONBODY_ATOM(Matrix,_constructor)
{
	assert_and_throw(argslen <= 6);
	
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	
	//Mapping to cairo_matrix_t
	//a -> xx
	//b -> yx
	//c -> xy
	//d -> yy
	//tx -> x0
	//ty -> y0
	
	if (argslen >= 1)
		th->matrix.xx = asAtomHandler::toNumber(args[0]);
	if (argslen >= 2)
		th->matrix.yx = asAtomHandler::toNumber(args[1]);
	if (argslen >= 3)
		th->matrix.xy = asAtomHandler::toNumber(args[2]);
	if (argslen >= 4)
		th->matrix.yy = asAtomHandler::toNumber(args[3]);
	if (argslen >= 5)
		th->matrix.x0 = asAtomHandler::toNumber(args[4]);
	if (argslen == 6)
		th->matrix.y0 = asAtomHandler::toNumber(args[5]);
}

void Matrix::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY_ATOM(Matrix,_toString)
{
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	char buf[512];
	snprintf(buf,512,"(a=%f, b=%f, c=%f, d=%f, tx=%f, ty=%f)",
			th->matrix.xx, th->matrix.yx, th->matrix.xy, th->matrix.yy, th->matrix.x0, th->matrix.y0);
	ret = asAtomHandler::fromObject(abstract_s(sys,buf));
}

MATRIX Matrix::getMATRIX() const
{
	return matrix;
}

bool Matrix::destruct()
{
	matrix = MATRIX();
	return destructIntern();
}

ASFUNCTIONBODY_ATOM(Matrix,_get_a)
{
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	asAtomHandler::setNumber(ret,sys,th->matrix.xx);
}

ASFUNCTIONBODY_ATOM(Matrix,_set_a)
{
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	assert_and_throw(argslen==1);
	th->matrix.xx = asAtomHandler::toNumber(args[0]);
}

ASFUNCTIONBODY_ATOM(Matrix,_get_b)
{
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	asAtomHandler::setNumber(ret,sys,th->matrix.yx);
}

ASFUNCTIONBODY_ATOM(Matrix,_set_b)
{
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	assert_and_throw(argslen==1);
	th->matrix.yx = asAtomHandler::toNumber(args[0]);
}

ASFUNCTIONBODY_ATOM(Matrix,_get_c)
{
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	asAtomHandler::setNumber(ret,sys,th->matrix.xy);
}

ASFUNCTIONBODY_ATOM(Matrix,_set_c)
{
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	assert_and_throw(argslen==1);
	th->matrix.xy = asAtomHandler::toNumber(args[0]);
}

ASFUNCTIONBODY_ATOM(Matrix,_get_d)
{
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	asAtomHandler::setNumber(ret,sys,th->matrix.yy);
}

ASFUNCTIONBODY_ATOM(Matrix,_set_d)
{
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	assert_and_throw(argslen==1);
	th->matrix.yy = asAtomHandler::toNumber(args[0]);
}

ASFUNCTIONBODY_ATOM(Matrix,_get_tx)
{
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	asAtomHandler::setNumber(ret,sys,th->matrix.x0);
}

ASFUNCTIONBODY_ATOM(Matrix,_set_tx)
{
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	assert_and_throw(argslen==1);
	th->matrix.x0 = asAtomHandler::toNumber(args[0]);
}

ASFUNCTIONBODY_ATOM(Matrix,_get_ty)
{
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	asAtomHandler::setNumber(ret,sys,th->matrix.y0);
}

ASFUNCTIONBODY_ATOM(Matrix,_set_ty)
{
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	assert_and_throw(argslen==1);
	th->matrix.y0 = asAtomHandler::toNumber(args[0]);
}

ASFUNCTIONBODY_ATOM(Matrix,clone)
{
	assert_and_throw(argslen==0);

	Matrix* th=asAtomHandler::as<Matrix>(obj);
	Matrix* res=Class<Matrix>::getInstanceS(sys,th->matrix);
	ret =asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(Matrix,concat)
{
	assert_and_throw(argslen==1);

	Matrix* th=asAtomHandler::as<Matrix>(obj);
	Matrix* m=asAtomHandler::as<Matrix>(args[0]);

	//Premultiply, which is flash convention
	cairo_matrix_multiply(&th->matrix,&th->matrix,&m->matrix);
}

ASFUNCTIONBODY_ATOM(Matrix,identity)
{
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	assert_and_throw(argslen==0);
	
	cairo_matrix_init_identity(&th->matrix);
}

ASFUNCTIONBODY_ATOM(Matrix,invert)
{
	assert_and_throw(argslen==0);
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	th->matrix=th->matrix.getInverted();
}

ASFUNCTIONBODY_ATOM(Matrix,translate)
{
	assert_and_throw(argslen==2);
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	number_t dx = asAtomHandler::toNumber(args[0]);
	number_t dy = asAtomHandler::toNumber(args[1]);

	th->matrix.translate(dx,dy);
}

ASFUNCTIONBODY_ATOM(Matrix,rotate)
{
	assert_and_throw(argslen==1);
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	number_t angle = asAtomHandler::toNumber(args[0]);

	th->matrix.rotate(angle);
}

ASFUNCTIONBODY_ATOM(Matrix,scale)
{
	assert_and_throw(argslen==2);
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	number_t sx = asAtomHandler::toNumber(args[0]);
	number_t sy = asAtomHandler::toNumber(args[1]);

	th->matrix.scale(sx, sy);
}

void Matrix::_createBox (number_t scaleX, number_t scaleY, number_t angle, number_t x, number_t y) {
	/*
	 * sequence written in the spec:
	 *      identity();
	 *      rotate(angle);
	 *      scale(scaleX, scaleY);
	 *      translate(x, y);
	 */

	//Initialize using rotation
	cairo_matrix_init_rotate(&matrix,angle);

	matrix.scale(scaleX,scaleY);
	matrix.translate(x,y);
}

ASFUNCTIONBODY_ATOM(Matrix,createBox)
{
	assert_and_throw(argslen>=2 && argslen <= 5);
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	number_t scaleX = asAtomHandler::toNumber(args[0]);
	number_t scaleY = asAtomHandler::toNumber(args[1]);

	number_t angle = 0;
	if ( argslen > 2 ) angle = asAtomHandler::toNumber(args[2]);

	number_t translateX = 0;
	if ( argslen > 3 ) translateX = asAtomHandler::toNumber(args[3]);

	number_t translateY = 0;
	if ( argslen > 4 ) translateY = asAtomHandler::toNumber(args[4]);

	th->_createBox(scaleX, scaleY, angle, translateX, translateY);
}

ASFUNCTIONBODY_ATOM(Matrix,createGradientBox)
{
	assert_and_throw(argslen>=2 && argslen <= 5);
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	number_t width  = asAtomHandler::toNumber(args[0]);
	number_t height = asAtomHandler::toNumber(args[1]);

	number_t angle = 0;
	if ( argslen > 2 ) angle = asAtomHandler::toNumber(args[2]);

	number_t translateX = width/2.0;
	if ( argslen > 3 ) translateX += asAtomHandler::toNumber(args[3]);

	number_t translateY = height/2.0;
	if ( argslen > 4 ) translateY += asAtomHandler::toNumber(args[4]);

	th->_createBox(width / 1638.4, height / 1638.4, angle, translateX, translateY);
}

ASFUNCTIONBODY_ATOM(Matrix,transformPoint)
{
	assert_and_throw(argslen==1);
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	Point* pt=asAtomHandler::as<Point>(args[0]);

	number_t ttx = pt->getX();
	number_t tty = pt->getY();
	cairo_matrix_transform_point(&th->matrix,&ttx,&tty);
	ret = asAtomHandler::fromObject(Class<Point>::getInstanceS(sys,ttx, tty));
}

ASFUNCTIONBODY_ATOM(Matrix,deltaTransformPoint)
{
	assert_and_throw(argslen==1);
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	Point* pt=asAtomHandler::as<Point>(args[0]);

	number_t ttx = pt->getX();
	number_t tty = pt->getY();
	cairo_matrix_transform_distance(&th->matrix,&ttx,&tty);
	ret = asAtomHandler::fromObject(Class<Point>::getInstanceS(sys,ttx, tty));
}

ASFUNCTIONBODY_ATOM(Matrix,setTo)
{
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	
	//Mapping to cairo_matrix_t
	//a -> xx
	//b -> yx
	//c -> xy
	//d -> yy
	//tx -> x0
	//ty -> y0
	
	ARG_UNPACK_ATOM(th->matrix.xx)(th->matrix.yx)(th->matrix.xy)(th->matrix.yy)(th->matrix.x0)(th->matrix.y0);
}
ASFUNCTIONBODY_ATOM(Matrix,copyFrom)
{
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	
	_NR<Matrix> sourceMatrix;
	ARG_UNPACK_ATOM(sourceMatrix);
	th->matrix.xx = sourceMatrix->matrix.xx;
	th->matrix.yx = sourceMatrix->matrix.yx;
	th->matrix.xy = sourceMatrix->matrix.xy;
	th->matrix.yy = sourceMatrix->matrix.yy;
	th->matrix.x0 = sourceMatrix->matrix.x0;
	th->matrix.y0 = sourceMatrix->matrix.y0;
}

void Vector3D::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED);
	c->isReusable = true;

	// constants
	Vector3D* tx = new (c->memoryAccount) Vector3D(c);
	tx->x = 1;
	c->setVariableByQName("X_AXIS","", tx, DECLARED_TRAIT);

	Vector3D* ty = new (c->memoryAccount) Vector3D(c);
	ty->y = 1;
	c->setVariableByQName("Y_AXIS","", ty, DECLARED_TRAIT);

	Vector3D* tz = new (c->memoryAccount) Vector3D(c);
	tz->z = 1;
	c->setVariableByQName("Z_AXIS","", tz, DECLARED_TRAIT);

	// properties
	c->setDeclaredMethodByQName("w","",Class<IFunction>::getFunction(c->getSystemState(),_get_w),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("x","",Class<IFunction>::getFunction(c->getSystemState(),_get_x),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("y","",Class<IFunction>::getFunction(c->getSystemState(),_get_y),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("z","",Class<IFunction>::getFunction(c->getSystemState(),_get_z),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("length","",Class<IFunction>::getFunction(c->getSystemState(),_get_length),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("lengthSquared","",Class<IFunction>::getFunction(c->getSystemState(),_get_lengthSquared),GETTER_METHOD,true);
	
	c->setDeclaredMethodByQName("w","",Class<IFunction>::getFunction(c->getSystemState(),_set_w),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("x","",Class<IFunction>::getFunction(c->getSystemState(),_set_x),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("y","",Class<IFunction>::getFunction(c->getSystemState(),_set_y),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("z","",Class<IFunction>::getFunction(c->getSystemState(),_set_z),SETTER_METHOD,true);
	
	// methods 
	c->setDeclaredMethodByQName("add","",Class<IFunction>::getFunction(c->getSystemState(),add),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("angleBetween","",Class<IFunction>::getFunction(c->getSystemState(),angleBetween),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("clone","",Class<IFunction>::getFunction(c->getSystemState(),clone),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("crossProduct","",Class<IFunction>::getFunction(c->getSystemState(),crossProduct),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("decrementBy","",Class<IFunction>::getFunction(c->getSystemState(),decrementBy),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("distance","",Class<IFunction>::getFunction(c->getSystemState(),distance),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("dotProduct","",Class<IFunction>::getFunction(c->getSystemState(),dotProduct),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("equals","",Class<IFunction>::getFunction(c->getSystemState(),equals),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("incrementBy","",Class<IFunction>::getFunction(c->getSystemState(),incrementBy),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("nearEquals","",Class<IFunction>::getFunction(c->getSystemState(),nearEquals),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("negate","",Class<IFunction>::getFunction(c->getSystemState(),negate),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("normalize","",Class<IFunction>::getFunction(c->getSystemState(),normalize),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("project","",Class<IFunction>::getFunction(c->getSystemState(),project),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("scaleBy","",Class<IFunction>::getFunction(c->getSystemState(),scaleBy),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("subtract","",Class<IFunction>::getFunction(c->getSystemState(),subtract),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setTo","",Class<IFunction>::getFunction(c->getSystemState(),setTo),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("copyFrom","",Class<IFunction>::getFunction(c->getSystemState(),copyFrom),NORMAL_METHOD,true);
	c->prototype->setVariableByQName("toString","",Class<IFunction>::getFunction(c->getSystemState(),_toString),DYNAMIC_TRAIT);
}

ASFUNCTIONBODY_ATOM(Vector3D,_constructor)
{
	assert_and_throw(argslen <= 4);
	
	Vector3D* th=asAtomHandler::as<Vector3D>(obj);
	
	th->w = 0;
	th->x = 0;
	th->y = 0;
	th->z = 0;
	
	if (argslen >= 1)
		th->x = asAtomHandler::toNumber(args[0]);
	if (argslen >= 2)
		th->y = asAtomHandler::toNumber(args[1]);
	if (argslen >= 3)
		th->z = asAtomHandler::toNumber(args[2]);
	if (argslen == 4)
		th->w = asAtomHandler::toNumber(args[3]);
}

void Vector3D::buildTraits(ASObject* o)
{
}

bool Vector3D::destruct()
{
	w = 0;
	x = 0;
	y = 0;
	z = 0;
	return destructIntern();
}

ASFUNCTIONBODY_ATOM(Vector3D,_toString)
{
	Vector3D* th=asAtomHandler::as<Vector3D>(obj);
	char buf[512];
	snprintf(buf,512,"(x=%f, y=%f, z=%f)", th->x, th->y, th->z);
	ret = asAtomHandler::fromObject(abstract_s(sys,buf));
}

ASFUNCTIONBODY_ATOM(Vector3D,_get_w)
{
	assert_and_throw(argslen==0);

	Vector3D* th=asAtomHandler::as<Vector3D>(obj);
	asAtomHandler::setNumber(ret,sys,th->w);
}

ASFUNCTIONBODY_ATOM(Vector3D,_set_w)
{
	assert_and_throw(argslen==1);
	Vector3D* th=asAtomHandler::as<Vector3D>(obj);
	th->w = asAtomHandler::toNumber(args[0]);
}

ASFUNCTIONBODY_ATOM(Vector3D,_get_x)
{
	assert_and_throw(argslen==0);

	Vector3D* th=asAtomHandler::as<Vector3D>(obj);
	asAtomHandler::setNumber(ret,sys,th->x);
}

ASFUNCTIONBODY_ATOM(Vector3D,_set_x)
{
	assert_and_throw(argslen==1);
	Vector3D* th=asAtomHandler::as<Vector3D>(obj);
	th->x = asAtomHandler::toNumber(args[0]);
}

ASFUNCTIONBODY_ATOM(Vector3D,_get_y)
{
	assert_and_throw(argslen==0);

	Vector3D* th=asAtomHandler::as<Vector3D>(obj);
	asAtomHandler::setNumber(ret,sys,th->y);
}

ASFUNCTIONBODY_ATOM(Vector3D,_set_y)
{
	assert_and_throw(argslen==1);
	Vector3D* th=asAtomHandler::as<Vector3D>(obj);
	th->y = asAtomHandler::toNumber(args[0]);
}

ASFUNCTIONBODY_ATOM(Vector3D,_get_z)
{
	assert_and_throw(argslen==0);

	Vector3D* th=asAtomHandler::as<Vector3D>(obj);
	asAtomHandler::setNumber(ret,sys,th->z);
}

ASFUNCTIONBODY_ATOM(Vector3D,_set_z)
{
	assert_and_throw(argslen==1);
	Vector3D* th=asAtomHandler::as<Vector3D>(obj);
	th->z = asAtomHandler::toNumber(args[0]);
}

ASFUNCTIONBODY_ATOM(Vector3D,_get_length)
{
	assert_and_throw(argslen==0);

	Vector3D* th=asAtomHandler::as<Vector3D>(obj);
	asAtomHandler::setNumber(ret,sys,sqrt(th->x * th->x + th->y * th->y + th->z * th->z));
}

ASFUNCTIONBODY_ATOM(Vector3D,_get_lengthSquared)
{
	assert_and_throw(argslen==0);

	Vector3D* th=asAtomHandler::as<Vector3D>(obj);
	asAtomHandler::setNumber(ret,sys,th->x * th->x + th->y * th->y + th->z * th->z);
}

ASFUNCTIONBODY_ATOM(Vector3D,clone)
{
	assert_and_throw(argslen==0);

	Vector3D* th=asAtomHandler::as<Vector3D>(obj);
	Vector3D* res=Class<Vector3D>::getInstanceS(sys);

	res->w = th->w;
	res->x = th->x;
	res->y = th->y;
	res->z = th->z;

	ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(Vector3D,add)
{
	assert_and_throw(argslen==1);

	Vector3D* th=asAtomHandler::as<Vector3D>(obj);
	Vector3D* vc=asAtomHandler::as<Vector3D>(args[0]);
	Vector3D* res=Class<Vector3D>::getInstanceS(sys);

	res->x = th->x + vc->x;
	res->y = th->y + vc->y;
	res->z = th->z + vc->z;

	ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(Vector3D,angleBetween)
{
	assert_and_throw(argslen==2);

	Vector3D* vc1=asAtomHandler::as<Vector3D>(args[0]);
	Vector3D* vc2=asAtomHandler::as<Vector3D>(args[1]);

	number_t angle = vc1->x * vc2->x + vc1->y * vc2->y + vc1->z * vc2->z;
	angle /= sqrt(vc1->x * vc1->x + vc1->y * vc1->y + vc1->z * vc1->z);
	angle /= sqrt(vc2->x * vc2->x + vc2->y * vc2->y + vc2->z * vc2->z);
	angle = acos(angle);

	asAtomHandler::setNumber(ret,sys,angle);
}

ASFUNCTIONBODY_ATOM(Vector3D,crossProduct)
{
	assert_and_throw(argslen==1);

	Vector3D* th=asAtomHandler::as<Vector3D>(obj);
	Vector3D* vc=asAtomHandler::as<Vector3D>(args[0]);
	Vector3D* res=Class<Vector3D>::getInstanceS(sys);

	res->x = th->y * vc->z - th->z * vc->y;
	res->y = th->z * vc->x - th->x * vc->z;
	res->z = th->x * vc->y - th->y * vc->x;

	ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(Vector3D,decrementBy)
{
	assert_and_throw(argslen==1);

	Vector3D* th=asAtomHandler::as<Vector3D>(obj);
	Vector3D* vc=asAtomHandler::as<Vector3D>(args[0]);

	th->x -= vc->x;
	th->y -= vc->y;
	th->z -= vc->z;
}

ASFUNCTIONBODY_ATOM(Vector3D,distance)
{
	assert_and_throw(argslen==2);

	Vector3D* vc1=asAtomHandler::as<Vector3D>(args[0]);
	Vector3D* vc2=asAtomHandler::as<Vector3D>(args[1]);

	number_t dx, dy, dz, dist;
	dx = vc1->x - vc2->x;
	dy = vc1->y - vc2->y;
	dz = vc1->z - vc2->z;
	dist = sqrt(dx * dx + dy * dy + dz * dz);

	asAtomHandler::setNumber(ret,sys,dist);
}

ASFUNCTIONBODY_ATOM(Vector3D,dotProduct)
{
	assert_and_throw(argslen==1);

	Vector3D* th=asAtomHandler::as<Vector3D>(obj);
	Vector3D* vc=asAtomHandler::as<Vector3D>(args[0]);

	asAtomHandler::setNumber(ret,sys,th->x * vc->x + th->y * vc->y + th->z * vc->z);
}

ASFUNCTIONBODY_ATOM(Vector3D,equals)
{
	assert_and_throw(argslen==1 || argslen==2);

	Vector3D* th=asAtomHandler::as<Vector3D>(obj);
	Vector3D* vc=asAtomHandler::as<Vector3D>(args[0]);
	int32_t allfour = 0;

	if ( argslen == 2 )
	{
		allfour = asAtomHandler::toInt(args[1]);
	}

	asAtomHandler::setBool(ret,th->x == vc->x &&  th->y == vc->y && th->z == vc->z && allfour ? th->w == vc->w : true);
}

ASFUNCTIONBODY_ATOM(Vector3D,incrementBy)
{
	assert_and_throw(argslen==1);

	Vector3D* th=asAtomHandler::as<Vector3D>(obj);
	Vector3D* vc=asAtomHandler::as<Vector3D>(args[0]);

	th->x += vc->x;
	th->y += vc->y;
	th->z += vc->z;
}

ASFUNCTIONBODY_ATOM(Vector3D,nearEquals)
{
	assert_and_throw(argslen==2 && argslen==3);

	Vector3D* th=asAtomHandler::as<Vector3D>(obj);
	Vector3D* vc=asAtomHandler::as<Vector3D>(args[0]);
	number_t tolerance = asAtomHandler::toNumber(args[1]);
	int32_t allfour = 0;

	if (argslen == 3 )
	{
		allfour = asAtomHandler::toInt(args[2]);
	}

	bool dx, dy, dz, dw;
	dx = (th->x - vc->x) < tolerance;
	dy = (th->y - vc->y) < tolerance;
	dz = (th->z - vc->z) < tolerance;
	dw = allfour ? (th->w - vc->w) < tolerance : true;

	asAtomHandler::setBool(ret,dx && dy && dz && dw);
}

ASFUNCTIONBODY_ATOM(Vector3D,negate)
{
	assert_and_throw(argslen==0);

	Vector3D* th=asAtomHandler::as<Vector3D>(obj);

	th->x = -th->x;
	th->y = -th->y;
	th->z = -th->z;
}

ASFUNCTIONBODY_ATOM(Vector3D,normalize)
{
	assert_and_throw(argslen==0);

	Vector3D* th=asAtomHandler::as<Vector3D>(obj);

	number_t len = sqrt(th->x * th->x + th->y * th->y + th->z * th->z);

	th->x /= len;
	th->y /= len;
	th->z /= len;
}

ASFUNCTIONBODY_ATOM(Vector3D,project)
{
	assert_and_throw(argslen==0);

	Vector3D* th=asAtomHandler::as<Vector3D>(obj);

	th->x /= th->w;
	th->y /= th->w;
	th->z /= th->w;
}

ASFUNCTIONBODY_ATOM(Vector3D,scaleBy)
{
	assert_and_throw(argslen==1);

	Vector3D* th=asAtomHandler::as<Vector3D>(obj);
	number_t scale = asAtomHandler::toNumber(args[0]);

	th->x *= scale;
	th->y *= scale;
	th->z *= scale;
}

ASFUNCTIONBODY_ATOM(Vector3D,subtract)
{
	assert_and_throw(argslen==1);

	Vector3D* th=asAtomHandler::as<Vector3D>(obj);
	Vector3D* vc=asAtomHandler::as<Vector3D>(args[0]);
	Vector3D* res=Class<Vector3D>::getInstanceS(sys);

	res->x = th->x - vc->x;
	res->y = th->y - vc->y;
	res->z = th->z - vc->z;

	ret = asAtomHandler::fromObject(res);
}
ASFUNCTIONBODY_ATOM(Vector3D,setTo)
{
	Vector3D* th=asAtomHandler::as<Vector3D>(obj);
	number_t xa,ya,za;
	ARG_UNPACK_ATOM(xa)(ya)(za);
	th->x = xa;
	th->y = ya;
	th->z = za;
}
ASFUNCTIONBODY_ATOM(Vector3D,copyFrom)
{
	Vector3D* th=asAtomHandler::as<Vector3D>(obj);
	_NR<Vector3D> sourcevector;
	ARG_UNPACK_ATOM(sourcevector);
	if (!sourcevector.isNull())
	{
		th->w = sourcevector->w;
		th->x = sourcevector->x;
		th->y = sourcevector->y;
		th->z = sourcevector->z;
	}
}


void Matrix3D::append(number_t *otherdata)
{
	number_t m111 = data[0];
	number_t m121 = data[4];
	number_t m131 = data[8];
	number_t m141 = data[12];

	number_t m112 = data[1];
	number_t m122 = data[5];
	number_t m132 = data[9];
	number_t m142 = data[13];

	number_t m113 = data[2];
	number_t m123 = data[6];
	number_t m133 = data[10];
	number_t m143 = data[14];

	number_t m114 = data[3];
	number_t m124 = data[7];
	number_t m134 = data[11];
	number_t m144 = data[15];

	number_t m211 = otherdata[0];
	number_t m221 = otherdata[4];
	number_t m231 = otherdata[8];
	number_t m241 = otherdata[12];

	number_t m212 = otherdata[1];
	number_t m222 = otherdata[5];
	number_t m232 = otherdata[9];
	number_t m242 = otherdata[13];

	number_t m213 = otherdata[2];
	number_t m223 = otherdata[6];
	number_t m233 = otherdata[10];
	number_t m243 = otherdata[14];

	number_t m214 = otherdata[3];
	number_t m224 = otherdata[7];
	number_t m234 = otherdata[11];
	number_t m244 = otherdata[15];

	data[0] = m111 * m211 + m112 * m221 + m113 * m231 + m114 * m241;
	data[1] = m111 * m212 + m112 * m222 + m113 * m232 + m114 * m242;
	data[2] = m111 * m213 + m112 * m223 + m113 * m233 + m114 * m243;
	data[3] = m111 * m214 + m112 * m224 + m113 * m234 + m114 * m244;

	data[4] = m121 * m211 + m122 * m221 + m123 * m231 + m124 * m241;
	data[5] = m121 * m212 + m122 * m222 + m123 * m232 + m124 * m242;
	data[6] = m121 * m213 + m122 * m223 + m123 * m233 + m124 * m243;
	data[7] = m121 * m214 + m122 * m224 + m123 * m234 + m124 * m244;

	data[8] = m131 * m211 + m132 * m221 + m133 * m231 + m134 * m241;
	data[9] = m131 * m212 + m132 * m222 + m133 * m232 + m134 * m242;
	data[10] = m131 * m213 + m132 * m223 + m133 * m233 + m134 * m243;
	data[11] = m131 * m214 + m132 * m224 + m133 * m234 + m134 * m244;

	data[12] = m141 * m211 + m142 * m221 + m143 * m231 + m144 * m241;
	data[13] = m141 * m212 + m142 * m222 + m143 * m232 + m144 * m242;
	data[14] = m141 * m213 + m142 * m223 + m143 * m233 + m144 * m243;
	data[15] = m141 * m214 + m142 * m224 + m143 * m234 + m144 * m244;
}
void Matrix3D::prepend(number_t *otherdata)
{
	number_t m111 = otherdata[0];
	number_t m121 = otherdata[4];
	number_t m131 = otherdata[8];
	number_t m141 = otherdata[12];

	number_t m112 = otherdata[1];
	number_t m122 = otherdata[5];
	number_t m132 = otherdata[9];
	number_t m142 = otherdata[13];

	number_t m113 = otherdata[2];
	number_t m123 = otherdata[6];
	number_t m133 = otherdata[10];
	number_t m143 = otherdata[14];

	number_t m114 = otherdata[3];
	number_t m124 = otherdata[7];
	number_t m134 = otherdata[11];
	number_t m144 = otherdata[15];


	number_t m211 = data[0];
	number_t m221 = data[4];
	number_t m231 = data[8];
	number_t m241 = data[12];

	number_t m212 = data[1];
	number_t m222 = data[5];
	number_t m232 = data[9];
	number_t m242 = data[13];

	number_t m213 = data[2];
	number_t m223 = data[6];
	number_t m233 = data[10];
	number_t m243 = data[14];

	number_t m214 = data[3];
	number_t m224 = data[7];
	number_t m234 = data[11];
	number_t m244 = data[15];

	data[0] = m111 * m211 + m112 * m221 + m113 * m231 + m114 * m241;
	data[1] = m111 * m212 + m112 * m222 + m113 * m232 + m114 * m242;
	data[2] = m111 * m213 + m112 * m223 + m113 * m233 + m114 * m243;
	data[3] = m111 * m214 + m112 * m224 + m113 * m234 + m114 * m244;
	
	data[4] = m121 * m211 + m122 * m221 + m123 * m231 + m124 * m241;
	data[5] = m121 * m212 + m122 * m222 + m123 * m232 + m124 * m242;
	data[6] = m121 * m213 + m122 * m223 + m123 * m233 + m124 * m243;
	data[7] = m121 * m214 + m122 * m224 + m123 * m234 + m124 * m244;
	
	data[8] = m131 * m211 + m132 * m221 + m133 * m231 + m134 * m241;
	data[9] = m131 * m212 + m132 * m222 + m133 * m232 + m134 * m242;
	data[10] = m131 * m213 + m132 * m223 + m133 * m233 + m134 * m243;
	data[11] = m131 * m214 + m132 * m224 + m133 * m234 + m134 * m244;
	
	data[12] = m141 * m211 + m142 * m221 + m143 * m231 + m144 * m241;
	data[13] = m141 * m212 + m142 * m222 + m143 * m232 + m144 * m242;
	data[14] = m141 * m213 + m142 * m223 + m143 * m233 + m144 * m243;
	data[15] = m141 * m214 + m142 * m224 + m143 * m234 + m144 * m244;
}
number_t Matrix3D::getDeterminant()
{
	return 1 * ((data[0] * data[5] - data[4] * data[1]) * (data[10] * data[15] - data[14] * data[11])
			- (data[0] * data[9] - data[8] * data[1]) * (data[6] * data[15] - data[14] * data[7])
			+ (data[0] * data[13] - data[12] * data[1]) * (data[6] * data[11] - data[10] * data[7])
			+ (data[4] * data[9] - data[8] * data[5]) * (data[2] * data[15] - data[14] * data[3])
			- (data[4] * data[13] - data[12] * data[5]) * (data[2] * data[11] - data[10] * data[3])
			+ (data[8] * data[13] - data[12] * data[9]) * (data[2] * data[7] - data[6] * data[3]));

}

void Matrix3D::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED);
	c->isReusable = true;
	c->setDeclaredMethodByQName("clone","",Class<IFunction>::getFunction(c->getSystemState(),clone),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("recompose","",Class<IFunction>::getFunction(c->getSystemState(),recompose),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("prepend","",Class<IFunction>::getFunction(c->getSystemState(),prepend),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("prependScale","",Class<IFunction>::getFunction(c->getSystemState(),prependScale),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("prependTranslation","",Class<IFunction>::getFunction(c->getSystemState(),prependTranslation),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("append","",Class<IFunction>::getFunction(c->getSystemState(),append),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("appendTranslation","",Class<IFunction>::getFunction(c->getSystemState(),appendTranslation),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("appendRotation","",Class<IFunction>::getFunction(c->getSystemState(),appendRotation),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("appendScale","",Class<IFunction>::getFunction(c->getSystemState(),appendScale),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("copyColumnTo","",Class<IFunction>::getFunction(c->getSystemState(),copyColumnTo),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("copyFrom","",Class<IFunction>::getFunction(c->getSystemState(),copyFrom),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("copyToMatrix3D","",Class<IFunction>::getFunction(c->getSystemState(),copyToMatrix3D),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("copyRawDataFrom","",Class<IFunction>::getFunction(c->getSystemState(),copyRawDataFrom),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("copyRawDataTo","",Class<IFunction>::getFunction(c->getSystemState(),copyRawDataTo),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("identity","",Class<IFunction>::getFunction(c->getSystemState(),identity),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("invert","",Class<IFunction>::getFunction(c->getSystemState(),invert),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("rawData","",Class<IFunction>::getFunction(c->getSystemState(),_get_rawData),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("rawData","",Class<IFunction>::getFunction(c->getSystemState(),_set_rawData),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("determinant","",Class<IFunction>::getFunction(c->getSystemState(),_get_determinant),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("position","",Class<IFunction>::getFunction(c->getSystemState(),_set_position),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("position","",Class<IFunction>::getFunction(c->getSystemState(),_get_position),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("transformVector","",Class<IFunction>::getFunction(c->getSystemState(),transformVector),NORMAL_METHOD,true);
}

bool Matrix3D::destruct()
{
	return destructIntern();
}

void Matrix3D::getRowAsFloat(uint32_t rownum, float *rowdata)
{
	rowdata[0] = data[rownum];
	rowdata[1] = data[rownum+4];
	rowdata[2] = data[rownum+8];
	rowdata[3] = data[rownum+12];
}
void Matrix3D::getColumnAsFloat(uint32_t column, float *rowdata)
{
	rowdata[0] = data[column*4];
	rowdata[1] = data[column*4+1];
	rowdata[2] = data[column*4+2];
	rowdata[3] = data[column*4+3];
}

void Matrix3D::getRawDataAsFloat(float *rowdata)
{
	for (uint32_t i = 0; i < 16; i++)
		rowdata[i] = data[i];
}

ASFUNCTIONBODY_ATOM(Matrix3D,_constructor)
{
	Matrix3D * th=asAtomHandler::as<Matrix3D>(obj);
	_NR<Vector> v;
	ARG_UNPACK_ATOM(v,NullRef);
	// Identity Matrix
	uint32_t i = 0;
	th->data[i++] = 1.0;
	th->data[i++] = 0.0;
	th->data[i++] = 0.0;
	th->data[i++] = 0.0;
	
	th->data[i++] = 0.0;
	th->data[i++] = 1.0;
	th->data[i++] = 0.0;
	th->data[i++] = 0.0;
	
	th->data[i++] = 0.0;
	th->data[i++] = 0.0;
	th->data[i++] = 1.0;
	th->data[i++] = 0.0;
	
	th->data[i++] = 0.0;
	th->data[i++] = 0.0;
	th->data[i++] = 0.0;
	th->data[i++] = 1.0;
	if (!v.isNull())
	{
		for (uint32_t i = 0; i < v->size() && i < 4*4; i++)
		{
			asAtom a = v->at(i);
			th->data[i] = asAtomHandler::toNumber(a);
		}
	}
}
ASFUNCTIONBODY_ATOM(Matrix3D,clone)
{
	Matrix3D * th=asAtomHandler::as<Matrix3D>(obj);
	Matrix3D * res = Class<Matrix3D>::getInstanceS(sys);
	memcpy(res->data,th->data,4*4*sizeof(number_t));
	ret = asAtomHandler::fromObject(res);
}
ASFUNCTIONBODY_ATOM(Matrix3D,recompose)
{
	_NR<Vector> components;
	tiny_string orientationStyle;
	ARG_UNPACK_ATOM(components)(orientationStyle, "eulerAngles");
	
	LOG(LOG_NOT_IMPLEMENTED, "Matrix3D.recompose does nothing");
	asAtomHandler::setBool(ret,false);
}
ASFUNCTIONBODY_ATOM(Matrix3D,prepend)
{
	Matrix3D * th=asAtomHandler::as<Matrix3D>(obj);
	_NR<Matrix3D> rhs;
	ARG_UNPACK_ATOM(rhs);
	if (rhs.isNull())
		throwError<ArgumentError>(kInvalidArgumentError,"rhs");
	th->prepend(rhs->data);
}
ASFUNCTIONBODY_ATOM(Matrix3D,prependScale)
{
	number_t xScale, yScale, zScale;
	ARG_UNPACK_ATOM(xScale) (yScale) (zScale);
	
	LOG(LOG_NOT_IMPLEMENTED, "Matrix3D.prependScale does nothing");
}
ASFUNCTIONBODY_ATOM(Matrix3D,prependTranslation)
{
	number_t x, y, z;
	ARG_UNPACK_ATOM(x) (y) (z);
	
	LOG(LOG_NOT_IMPLEMENTED, "Matrix3D.prependTranslation does nothing");
}
ASFUNCTIONBODY_ATOM(Matrix3D,append)
{
	Matrix3D * th=asAtomHandler::as<Matrix3D>(obj);
	_NR<Matrix3D> lhs;
	ARG_UNPACK_ATOM(lhs);
	if (lhs.isNull())
		throwError<ArgumentError>(kInvalidArgumentError,"lhs");
	th->append(lhs->data);
}
ASFUNCTIONBODY_ATOM(Matrix3D,appendTranslation)
{
	Matrix3D * th=asAtomHandler::as<Matrix3D>(obj);
	number_t x, y, z;
	ARG_UNPACK_ATOM(x) (y) (z);
	th->data[12] += x;
	th->data[13] += y;
	th->data[14] += z;
}
ASFUNCTIONBODY_ATOM(Matrix3D,appendRotation)
{
	Matrix3D * th=asAtomHandler::as<Matrix3D>(obj);
	
	number_t degrees;
	_NR<Vector3D> axis;
	_NR<Vector3D> pivotPoint;
	
	ARG_UNPACK_ATOM(degrees) (axis) (pivotPoint,NullRef);

	// algorithm taken from https://github.com/openfl/openfl/blob/develop/openfl/geom/Matrix3D.hx
	number_t tx = 0;
	number_t ty = 0;
	number_t tz = 0;
	
	if (!pivotPoint.isNull()) {
		tx = pivotPoint->x;
		ty = pivotPoint->y;
		tz = pivotPoint->z;
	}
	number_t radian = degrees * 3.141592653589793/180.0;
	number_t cos = std::cos(radian);
	number_t sin = std::sin(radian);
	number_t x = axis->x;
	number_t y = axis->y;
	number_t z = axis->z;
	number_t x2 = x * x;
	number_t y2 = y * y;
	number_t z2 = z * z;
	number_t ls = x2 + y2 + z2;
	if (ls != 0) {
		number_t l = std::sqrt(ls);
		x /= l;
		y /= l;
		z /= l;
		x2 /= ls;
		y2 /= ls;
		z2 /= ls;
	}
	number_t ccos = 1 - cos;
	number_t d[4*4];
	d[0]  = x2 + (y2 + z2) * cos;
	d[1]  = x * y * ccos + z * sin;
	d[2]  = x * z * ccos - y * sin;
	d[3]  = 0.0;
	d[4]  = x * y * ccos - z * sin;
	d[5]  = y2 + (x2 + z2) * cos;
	d[6]  = y * z * ccos + x * sin;
	d[7]  = 0.0;
	d[8]  = x * z * ccos + y * sin;
	d[9]  = y * z * ccos - x * sin;
	d[10] = z2 + (x2 + y2) * cos;
	d[11] = 0.0;
	d[12] = (tx * (y2 + z2) - x * (ty * y + tz * z)) * ccos + (ty * z - tz * y) * sin;
	d[13] = (ty * (x2 + z2) - y * (tx * x + tz * z)) * ccos + (tz * x - tx * z) * sin;
	d[14] = (tz * (x2 + y2) - z * (tx * x + ty * y)) * ccos + (tx * y - ty * x) * sin;
	d[15] = 1.0;
	th->append(d);
}
ASFUNCTIONBODY_ATOM(Matrix3D,appendScale)
{
	Matrix3D * th=asAtomHandler::as<Matrix3D>(obj);
	
	number_t xScale;
	number_t yScale;
	number_t zScale;
	ARG_UNPACK_ATOM(xScale) (yScale) (zScale);

	number_t d[4*4];
	d[0]  = xScale;
	d[1]  = 0.0;
	d[2]  = 0.0;
	d[3]  = 0.0;
	
	d[4]  = 0.0;
	d[5]  = yScale;
	d[6]  = 0.0;
	d[7]  = 0.0;
	
	d[8]  = 0.0;
	d[9]  = 0.0;
	d[10] = zScale;
	d[11] = 0.0;
	
	d[12] = 0.0;
	d[13] = 0.0;
	d[14] = 0.0;
	d[15] = 1.0;
	th->append(d);
}
ASFUNCTIONBODY_ATOM(Matrix3D,copyColumnTo)
{
	Matrix3D * th=asAtomHandler::as<Matrix3D>(obj);
	uint32_t column;
	_NR<Vector3D> vector3D;
	ARG_UNPACK_ATOM(column)(vector3D);
	if (vector3D.isNull())
		throwError<ArgumentError>(kInvalidArgumentError,"vector3D");
	if (column < 4)
	{
		vector3D->x = th->data[column*4];
		vector3D->y = th->data[column*4+1];
		vector3D->z = th->data[column*4+2];
		vector3D->w = th->data[column*4+3];
	}
}

ASFUNCTIONBODY_ATOM(Matrix3D,copyRawDataFrom)
{
	Matrix3D * th=asAtomHandler::as<Matrix3D>(obj);
	_NR<Vector> vector;
	uint32_t index;
	bool transpose;
	ARG_UNPACK_ATOM(vector) (index,0) (transpose,false);
	if (transpose)
		LOG(LOG_NOT_IMPLEMENTED, "Matrix3D.copyRawDataFrom ignores parameter 'transpose'");
	for (uint32_t i = 0; i < vector->size()-index && i < 16; i++)
	{
		asAtom a = vector->at(index+i);
		th->data[i] = asAtomHandler::toNumber(a);
	}
}

ASFUNCTIONBODY_ATOM(Matrix3D,copyRawDataTo)
{
	Matrix3D * th=asAtomHandler::as<Matrix3D>(obj);
	_NR<Vector> vector;
	uint32_t index;
	bool transpose;
	ARG_UNPACK_ATOM(vector) (index,0) (transpose,false);
	if (transpose)
		LOG(LOG_NOT_IMPLEMENTED, "Matrix3D.copyRawDataFrom ignores parameter 'transpose'");
	for (uint32_t i = 0; i < vector->size()-index && i < 16; i++)
	{
		vector->set(index+i,asAtomHandler::fromNumber(sys,th->data[i],false));
	}
}

ASFUNCTIONBODY_ATOM(Matrix3D,copyFrom)
{
	Matrix3D * th=asAtomHandler::as<Matrix3D>(obj);
	_NR<Matrix3D> sourceMatrix3D;
	ARG_UNPACK_ATOM(sourceMatrix3D);
	if (sourceMatrix3D.isNull())
		throwError<ArgumentError>(kInvalidArgumentError,"sourceMatrix3D");
	for (uint32_t i = 0; i < 4*4; i++)
	{
		th->data[i] = sourceMatrix3D->data[i];
	}
}
ASFUNCTIONBODY_ATOM(Matrix3D,copyToMatrix3D)
{
	Matrix3D * th=asAtomHandler::as<Matrix3D>(obj);
	_NR<Matrix3D> dest;
	ARG_UNPACK_ATOM(dest);
	if (dest.isNull())
		throwError<ArgumentError>(kInvalidArgumentError,"dest");
	for (uint32_t i = 0; i < 4*4; i++)
	{
		dest->data[i] = th->data[i];
	}
}

ASFUNCTIONBODY_ATOM(Matrix3D,identity)
{
	Matrix3D * th=asAtomHandler::as<Matrix3D>(obj);
	// Identity Matrix
	uint32_t i = 0;
	th->data[i++] = 1.0;
	th->data[i++] = 0.0;
	th->data[i++] = 0.0;
	th->data[i++] = 0.0;
	
	th->data[i++] = 0.0;
	th->data[i++] = 1.0;
	th->data[i++] = 0.0;
	th->data[i++] = 0.0;
	
	th->data[i++] = 0.0;
	th->data[i++] = 0.0;
	th->data[i++] = 1.0;
	th->data[i++] = 0.0;
	
	th->data[i++] = 0.0;
	th->data[i++] = 0.0;
	th->data[i++] = 0.0;
	th->data[i++] = 1.0;
}
ASFUNCTIONBODY_ATOM(Matrix3D,invert)
{
	Matrix3D * th=asAtomHandler::as<Matrix3D>(obj);

	// algorithm taken from https://github.com/openfl/openfl/blob/develop/openfl/geom/Matrix3D.hx
	number_t det = th->getDeterminant();
	bool invertable = abs(det) > 0.00000000001;
	if (invertable)
	{
		number_t d = 1 / det;
		number_t m11 = th->data[0];	number_t m21 = th->data[4]; number_t m31 = th->data[8]; number_t m41 = th->data[12];
		number_t m12 = th->data[1]; number_t m22 = th->data[5]; number_t m32 = th->data[9]; number_t m42 = th->data[13];
		number_t m13 = th->data[2]; number_t m23 = th->data[6]; number_t m33 = th->data[10]; number_t m43 = th->data[14];
		number_t m14 = th->data[3]; number_t m24 = th->data[7]; number_t m34 = th->data[11]; number_t m44 = th->data[15];

		th->data[0] = d * (m22 * (m33 * m44 - m43 * m34) - m32 * (m23 * m44 - m43 * m24) + m42 * (m23 * m34 - m33 * m24));
		th->data[1] = -d * (m12 * (m33 * m44 - m43 * m34) - m32 * (m13 * m44 - m43 * m14) + m42 * (m13 * m34 - m33 * m14));
		th->data[2] = d * (m12 * (m23 * m44 - m43 * m24) - m22 * (m13 * m44 - m43 * m14) + m42 * (m13 * m24 - m23 * m14));
		th->data[3] = -d * (m12 * (m23 * m34 - m33 * m24) - m22 * (m13 * m34 - m33 * m14) + m32 * (m13 * m24 - m23 * m14));
		th->data[4] = -d * (m21 * (m33 * m44 - m43 * m34) - m31 * (m23 * m44 - m43 * m24) + m41 * (m23 * m34 - m33 * m24));
		th->data[5] = d * (m11 * (m33 * m44 - m43 * m34) - m31 * (m13 * m44 - m43 * m14) + m41 * (m13 * m34 - m33 * m14));
		th->data[6] = -d * (m11 * (m23 * m44 - m43 * m24) - m21 * (m13 * m44 - m43 * m14) + m41 * (m13 * m24 - m23 * m14));
		th->data[7] = d * (m11 * (m23 * m34 - m33 * m24) - m21 * (m13 * m34 - m33 * m14) + m31 * (m13 * m24 - m23 * m14));
		th->data[8] = d * (m21 * (m32 * m44 - m42 * m34) - m31 * (m22 * m44 - m42 * m24) + m41 * (m22 * m34 - m32 * m24));
		th->data[9] = -d * (m11 * (m32 * m44 - m42 * m34) - m31 * (m12 * m44 - m42 * m14) + m41 * (m12 * m34 - m32 * m14));
		th->data[10] = d * (m11 * (m22 * m44 - m42 * m24) - m21 * (m12 * m44 - m42 * m14) + m41 * (m12 * m24 - m22 * m14));
		th->data[11] = -d * (m11 * (m22 * m34 - m32 * m24) - m21 * (m12 * m34 - m32 * m14) + m31 * (m12 * m24 - m22 * m14));
		th->data[12] = -d * (m21 * (m32 * m43 - m42 * m33) - m31 * (m22 * m43 - m42 * m23) + m41 * (m22 * m33 - m32 * m23));
		th->data[13] = d * (m11 * (m32 * m43 - m42 * m33) - m31 * (m12 * m43 - m42 * m13) + m41 * (m12 * m33 - m32 * m13));
		th->data[14] = -d * (m11 * (m22 * m43 - m42 * m23) - m21 * (m12 * m43 - m42 * m13) + m41 * (m12 * m23 - m22 * m13));
		th->data[15] = d * (m11 * (m22 * m33 - m32 * m23) - m21 * (m12 * m33 - m32 * m13) + m31 * (m12 * m23 - m22 * m13));
	}
	asAtomHandler::setBool(ret,invertable);
}
ASFUNCTIONBODY_ATOM(Matrix3D,_get_determinant)
{
	Matrix3D * th=asAtomHandler::as<Matrix3D>(obj);
	asAtomHandler::setNumber(ret,sys,th->getDeterminant());
}
ASFUNCTIONBODY_ATOM(Matrix3D,_get_rawData)
{
	Matrix3D * th=asAtomHandler::as<Matrix3D>(obj);
	asAtom v=asAtomHandler::invalidAtom;
	Template<Vector>::getInstanceS(v,sys,Class<Number>::getClass(sys),NullRef);
	Vector *result = asAtomHandler::as<Vector>(v);
	for (uint32_t i = 0; i < 4*4; i++)
	{
		asAtom o = asAtomHandler::fromNumber(sys,th->data[i],false);
		result->append(o);
	}
	ret =asAtomHandler::fromObject(result);
}
ASFUNCTIONBODY_ATOM(Matrix3D,_set_rawData)
{
	Matrix3D * th=asAtomHandler::as<Matrix3D>(obj);
	_NR<Vector> data;
	ARG_UNPACK_ATOM(data);
	// TODO handle not invertible argument
	for (uint32_t i = 0; i < data->size(); i++)
	{
		asAtom a = data->at(i);
		th->data[i] = asAtomHandler::toNumber(a);
	}
}
ASFUNCTIONBODY_ATOM(Matrix3D,_get_position)
{
	Matrix3D * th=asAtomHandler::as<Matrix3D>(obj);
	Vector3D* res = Class<Vector3D>::getInstanceS(sys);
	res->w = 0;
	res->x = th->data[12];
	res->y = th->data[13];
	res->z = th->data[14];
	ret = asAtomHandler::fromObject(res);
}
ASFUNCTIONBODY_ATOM(Matrix3D,_set_position)
{
	Matrix3D * th=asAtomHandler::as<Matrix3D>(obj);
	_NR<Vector3D> value;
	ARG_UNPACK_ATOM(value);
	if (value.isNull())
		throwError<ArgumentError>(kInvalidArgumentError,"value");
	th->data[12] = value->x;
	th->data[13] = value->y;
	th->data[14] = value->z;
}
ASFUNCTIONBODY_ATOM(Matrix3D,transformVector)
{
	Matrix3D * th=asAtomHandler::as<Matrix3D>(obj);
	_NR<Vector3D> v;
	ARG_UNPACK_ATOM(v);
	if (v.isNull())
		throwError<ArgumentError>(kInvalidArgumentError,"v");
	number_t x = v->x;
	number_t y = v->y;
	number_t z = v->z;
	Vector3D* res = Class<Vector3D>::getInstanceS(sys);
	res->x = (x * th->data[0] + y * th->data[4] + z * th->data[8] + th->data[12]);
	res->y = (x * th->data[1] + y * th->data[5] + z * th->data[9] + th->data[13]);
	res->z = (x * th->data[2] + y * th->data[6] + z * th->data[10] + th->data[14]);
	res->w = (x * th->data[3] + y * th->data[7] + z * th->data[11] + th->data[15]);
	ret = asAtomHandler::fromObject(res);
}


void PerspectiveProjection::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED);
	c->isReusable = true;
	REGISTER_GETTER_SETTER(c, fieldOfView);
	REGISTER_GETTER_SETTER(c, focalLength);
	REGISTER_GETTER_SETTER(c, projectionCenter);
	
}

bool PerspectiveProjection::destruct()
{
	fieldOfView = 0;
	focalLength= 0;
	projectionCenter.reset();
	return destructIntern();
}
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(PerspectiveProjection, fieldOfView);
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(PerspectiveProjection, focalLength);
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(PerspectiveProjection, projectionCenter);

ASFUNCTIONBODY_ATOM(PerspectiveProjection,_constructor)
{
	PerspectiveProjection* th=asAtomHandler::as<PerspectiveProjection>(obj);
	th->projectionCenter = _MR(Class<Point>::getInstanceSNoArgs(sys));
	LOG(LOG_NOT_IMPLEMENTED,"PerspectiveProjection is not implemented");
}
