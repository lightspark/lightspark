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

#include "abc.h"
#include "flashgeom.h"
#include "class.h"

using namespace lightspark;
using namespace std;

SET_NAMESPACE("flash.geom");

REGISTER_CLASS_NAME(Transform);
REGISTER_CLASS_NAME(ColorTransform);
REGISTER_CLASS_NAME(Point);
REGISTER_CLASS_NAME2(lightspark::Rectangle,"Rectangle","flash.geom");

void Rectangle::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	IFunction* gleft=Class<IFunction>::getFunction(_getLeft);
	c->setGetterByQName("left","",gleft,true);
	gleft->incRef();
	c->setGetterByQName("x","",gleft,true);
	IFunction* sleft=Class<IFunction>::getFunction(_setLeft);
	c->setSetterByQName("left","",sleft,true);
	sleft->incRef();
	c->setSetterByQName("x","",sleft,true);
	c->setGetterByQName("right","",Class<IFunction>::getFunction(_getRight),true);
	c->setSetterByQName("right","",Class<IFunction>::getFunction(_setRight),true);
	c->setGetterByQName("width","",Class<IFunction>::getFunction(_getWidth),true);
	c->setSetterByQName("width","",Class<IFunction>::getFunction(_setWidth),true);

	IFunction* gtop=Class<IFunction>::getFunction(_getTop);
	c->setGetterByQName("top","",gtop,true);
	gtop->incRef();
	c->setGetterByQName("y","",gtop,true);
	IFunction* stop=Class<IFunction>::getFunction(_setTop);
	c->setSetterByQName("top","",stop,true);
	stop->incRef();
	c->setSetterByQName("y","",stop,true);

	c->setGetterByQName("bottom","",Class<IFunction>::getFunction(_getBottom),true);
	c->setSetterByQName("bottom","",Class<IFunction>::getFunction(_setBottom),true);
	c->setGetterByQName("height","",Class<IFunction>::getFunction(_getHeight),true);
	c->setSetterByQName("height","",Class<IFunction>::getFunction(_setHeight),true);

	c->setGetterByQName("bottomRight","",Class<IFunction>::getFunction(_getBottomRight),true);
	c->setSetterByQName("bottomRight","",Class<IFunction>::getFunction(_setBottomRight),true);

	c->setGetterByQName("size","",Class<IFunction>::getFunction(_getSize),true);
	c->setSetterByQName("size","",Class<IFunction>::getFunction(_setSize),true);

	c->setGetterByQName("topLeft","",Class<IFunction>::getFunction(_getTopLeft),true);
	c->setSetterByQName("topLeft","",Class<IFunction>::getFunction(_setTopLeft),true);

	c->setMethodByQName("clone","",Class<IFunction>::getFunction(clone),true);
	c->setMethodByQName("contains","",Class<IFunction>::getFunction(contains),true);
	c->setMethodByQName("containsPoint","",Class<IFunction>::getFunction(containsPoint),true);
	c->setMethodByQName("containsRect","",Class<IFunction>::getFunction(containsRect),true);
	c->setMethodByQName("equals","",Class<IFunction>::getFunction(equals),true);
	c->setMethodByQName("inflate","",Class<IFunction>::getFunction(inflate),true);
	c->setMethodByQName("inflatePoint","",Class<IFunction>::getFunction(inflatePoint),true);
	c->setMethodByQName("intersection","",Class<IFunction>::getFunction(intersection),true);
	c->setMethodByQName("intersects","",Class<IFunction>::getFunction(intersects),true);
	c->setMethodByQName("isEmpty","",Class<IFunction>::getFunction(isEmpty),true);
	c->setMethodByQName("offset","",Class<IFunction>::getFunction(offset),true);
	c->setMethodByQName("offsetPoint","",Class<IFunction>::getFunction(offsetPoint),true);
	c->setMethodByQName("setEmpty","",Class<IFunction>::getFunction(setEmpty),true);
	c->setMethodByQName("union","",Class<IFunction>::getFunction(_union),true);
}

void Rectangle::buildTraits(ASObject* o)
{
}

tiny_string Rectangle::toString(bool debugMsg)
{
	assert_and_throw(implEnable);
	
	char buf[512];
	snprintf(buf,512,"(x=%f, y=%f, w=%f, h=%f)",x,y,width,height);
	
	return tiny_string(buf, true);
}

const lightspark::RECT Rectangle::getRect() const
{
	return lightspark::RECT(x,y,x+width,y+height);
}

ASFUNCTIONBODY(Rectangle,_constructor)
{
	Rectangle* th=static_cast<Rectangle*>(obj);

	if(argslen>=1)
		th->x=args[0]->toNumber();
	if(argslen>=2)
		th->y=args[1]->toNumber();
	if(argslen>=3)
		th->width=args[2]->toNumber();
	if(argslen>=4)
		th->height=args[3]->toNumber();

	return NULL;
}

ASFUNCTIONBODY(Rectangle,_getLeft)
{
	Rectangle* th=static_cast<Rectangle*>(obj);
	return abstract_d(th->x);
}

ASFUNCTIONBODY(Rectangle,_setLeft)
{
	Rectangle* th=static_cast<Rectangle*>(obj);
	assert_and_throw(argslen==1);
	th->x=args[0]->toNumber();
	return NULL;
}

ASFUNCTIONBODY(Rectangle,_getRight)
{
	Rectangle* th=static_cast<Rectangle*>(obj);
	return abstract_d(th->x + th->width);
}

ASFUNCTIONBODY(Rectangle,_setRight)
{
	Rectangle* th=static_cast<Rectangle*>(obj);
	assert_and_throw(argslen==1);
	th->width=(args[0]->toNumber()-th->x);
	return NULL;
}

ASFUNCTIONBODY(Rectangle,_getWidth)
{
	Rectangle* th=static_cast<Rectangle*>(obj);
	return abstract_d(th->width);
}

ASFUNCTIONBODY(Rectangle,_setWidth)
{
	Rectangle* th=static_cast<Rectangle*>(obj);
	assert_and_throw(argslen==1);
	th->width=args[0]->toNumber();
	return NULL;
}

ASFUNCTIONBODY(Rectangle,_getTop)
{
	Rectangle* th=static_cast<Rectangle*>(obj);
	return abstract_d(th->y);
}

ASFUNCTIONBODY(Rectangle,_setTop)
{
	Rectangle* th=static_cast<Rectangle*>(obj);
	assert_and_throw(argslen==1);
	th->y=args[0]->toNumber();
	return NULL;
}

ASFUNCTIONBODY(Rectangle,_getBottom)
{
	Rectangle* th=static_cast<Rectangle*>(obj);
	return abstract_d(th->y + th->height);
}

ASFUNCTIONBODY(Rectangle,_setBottom)
{
	Rectangle* th=static_cast<Rectangle*>(obj);
	assert_and_throw(argslen==1);
	th->height=(args[0]->toNumber()-th->y);
	return NULL;
}

ASFUNCTIONBODY(Rectangle,_getBottomRight)
{
	assert_and_throw(argslen==0);
	Rectangle* th = static_cast<Rectangle*>(obj);
	Point* ret = Class<Point>::getInstanceS(th->x + th->width, th->y + th->height);
	return ret;
}

ASFUNCTIONBODY(Rectangle,_setBottomRight)
{
	assert_and_throw(argslen == 1);
	Rectangle* th = static_cast<Rectangle*>(obj);
	Point* br = static_cast<Point*>(args[0]);
	th->width = br->getX() - th->x;
	th->height = br->getY() - th->y;
	return NULL;
}

ASFUNCTIONBODY(Rectangle,_getTopLeft)
{
	assert_and_throw(argslen==0);
	Rectangle* th = static_cast<Rectangle*>(obj);
	Point* ret = Class<Point>::getInstanceS(th->x, th->y);
	return ret;
}

ASFUNCTIONBODY(Rectangle,_setTopLeft)
{
	assert_and_throw(argslen == 1);
	Rectangle* th = static_cast<Rectangle*>(obj);
	Point* br = static_cast<Point*>(args[0]);
	th->width = br->getX();
	th->height = br->getY();
	return NULL;
}

ASFUNCTIONBODY(Rectangle,_getSize)
{
	assert_and_throw(argslen==0);
	Rectangle* th = static_cast<Rectangle*>(obj);
	Point* ret = Class<Point>::getInstanceS(th->width, th->height);
	return ret;
}

ASFUNCTIONBODY(Rectangle,_setSize)
{
	assert_and_throw(argslen == 1);
	Rectangle* th = static_cast<Rectangle*>(obj);
	Point* br = static_cast<Point*>(args[0]);
	th->width = br->getX();
	th->height = br->getY();
	return NULL;
}

ASFUNCTIONBODY(Rectangle,_getHeight)
{
	Rectangle* th=static_cast<Rectangle*>(obj);
	return abstract_d(th->height);
}

ASFUNCTIONBODY(Rectangle,_setHeight)
{
	Rectangle* th=static_cast<Rectangle*>(obj);
	assert_and_throw(argslen==1);
	th->height=args[0]->toNumber();
	return NULL;
}

ASFUNCTIONBODY(Rectangle,clone)
{
	Rectangle* th=static_cast<Rectangle*>(obj);
	Rectangle* ret=Class<Rectangle>::getInstanceS();
	ret->x=th->x;
	ret->y=th->y;
	ret->width=th->width;
	ret->height=th->height;
	return ret;
}

ASFUNCTIONBODY(Rectangle,contains)
{
	assert_and_throw(argslen == 2);
	Rectangle* th = static_cast<Rectangle*>(obj);
	number_t x = args[0]->toNumber();
	number_t y = args[1]->toNumber();

	return abstract_b( th->x <= x && x <= th->x + th->width
						&& th->y <= y && y <= th->y + th->height );
}

ASFUNCTIONBODY(Rectangle,containsPoint)
{
	assert_and_throw(argslen == 1);
	Rectangle* th = static_cast<Rectangle*>(obj);
	Point* br = static_cast<Point*>(args[0]);
	number_t x = br->getX();
	number_t y = br->getY();

	return abstract_b( th->x <= x && x <= th->x + th->width
						&& th->y <= y && y <= th->y + th->height );
}

ASFUNCTIONBODY(Rectangle,containsRect)
{
	assert_and_throw(argslen == 1);
	Rectangle* th = static_cast<Rectangle*>(obj);
	Rectangle* cr = static_cast<Rectangle*>(args[0]);

	return abstract_b( th->x <= cr->x && cr->x + cr->width <= th->x + th->width
						&& th->y <= cr->y && cr->y + cr->height <= th->y + th->height );
}

ASFUNCTIONBODY(Rectangle,equals)
{
	assert_and_throw(argslen == 1);
	Rectangle* th = static_cast<Rectangle*>(obj);
	Rectangle* co = static_cast<Rectangle*>(args[0]);

	return abstract_b( th->x == co->x && th->width == co->width
						&& th->y == co->y && th->height == co->height );
}

ASFUNCTIONBODY(Rectangle,inflate)
{
	assert_and_throw(argslen == 2);
	Rectangle* th = static_cast<Rectangle*>(obj);
	number_t dx = args[0]->toNumber();
	number_t dy = args[1]->toNumber();

	th->x -= dx;
	th->width += 2 * dx;
	th->y -= dy;
	th->height += 2 * dy;

	return NULL;
}

ASFUNCTIONBODY(Rectangle,inflatePoint)
{
	assert_and_throw(argslen == 1);
	Rectangle* th = static_cast<Rectangle*>(obj);
	Point* po = static_cast<Point*>(args[0]);
	number_t dx = po->getX();
	number_t dy = po->getY();

	th->x -= dx;
	th->width += 2 * dx;
	th->y -= dy;
	th->height += 2 * dy;

	return NULL;
}

ASFUNCTIONBODY(Rectangle,intersection)
{
	assert_and_throw(argslen == 1);
	Rectangle* th = static_cast<Rectangle*>(obj);
	Rectangle* ti = static_cast<Rectangle*>(args[0]);
	Rectangle* ret = Class<Rectangle>::getInstanceS();

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
		ret->x = 0;
		ret->y = 0;
		ret->width = 0;
		ret->height = 0;
		return ret;
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

	ret->x = rightmost->x;
	ret->width = min(leftmost->x + leftmost->width, rightmost->x + rightmost->width) - rightmost->x;
	ret->y = bottommost->y;
	ret->height = min(topmost->y + topmost->height, bottommost->y + bottommost->height) - bottommost->y;

	return ret;
}

ASFUNCTIONBODY(Rectangle,intersects)
{
	assert_and_throw(argslen == 1);
	Rectangle* th = static_cast<Rectangle*>(obj);
	Rectangle* ti = static_cast<Rectangle*>(args[0]);

	number_t thtop = th->y;
	number_t thleft = th->x;
	number_t thright = th->x + th->width;
	number_t thbottom = th->y + th->height;

	number_t titop = ti->y;
	number_t tileft = ti->x;
	number_t tiright = ti->x + ti->width;
	number_t tibottom = ti->y + ti->height;

	return abstract_b( !(thtop > tibottom || thright < tileft ||
						thbottom < titop || thleft > tiright) );
}

ASFUNCTIONBODY(Rectangle,isEmpty)
{
	assert_and_throw(argslen == 0);
	Rectangle* th = static_cast<Rectangle*>(obj);

	return abstract_b( th->width <= 0 || th->height <= 0 );
}

ASFUNCTIONBODY(Rectangle,offset)
{
	assert_and_throw(argslen == 2);
	Rectangle* th = static_cast<Rectangle*>(obj);

	th->x += args[0]->toNumber();
	th->y += args[1]->toNumber();

	return NULL;
}

ASFUNCTIONBODY(Rectangle,offsetPoint)
{
	assert_and_throw(argslen == 1);
	Rectangle* th = static_cast<Rectangle*>(obj);
	Point* po = static_cast<Point*>(args[0]);

	th->x += po->getX();
	th->y += po->getY();

	return NULL;
}

ASFUNCTIONBODY(Rectangle,setEmpty)
{
	assert_and_throw(argslen == 0);
	Rectangle* th = static_cast<Rectangle*>(obj);

	th->x = 0;
	th->y = 0;
	th->width = 0;
	th->height = 0;

	return NULL;
}

ASFUNCTIONBODY(Rectangle,_union)
{
	assert_and_throw(argslen == 1);
	Rectangle* th = static_cast<Rectangle*>(obj);
	Rectangle* ti = static_cast<Rectangle*>(args[0]);
	Rectangle* ret = Class<Rectangle>::getInstanceS();

	ret->x = th->x;
	ret->y = th->y;
	ret->width = th->width;
	ret->height = th->height;

	if ( ti->width == 0 || ti->height == 0 )
	{
		return ret;
	}

	ret->x = min(th->x, ti->x);
	ret->y = min(th->y, ti->y);
	ret->width = max(th->x + th->width, ti->x + ti->width);
	ret->height = max(th->y + th->height, ti->y + ti->height);

	return ret;
}

void ColorTransform::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
}

ASFUNCTIONBODY(ColorTransform,_constructor)
{
	ColorTransform* th=static_cast<ColorTransform*>(obj);
	assert_and_throw(argslen<=8);
	if(0 < argslen)
		th->redMultiplier=args[0]->toNumber();
	else
		th->redMultiplier=1.0;
	if(1 < argslen)
		th->greenMultiplier=args[1]->toNumber();
	else
		th->greenMultiplier=1.0;
	if(2 < argslen)
		th->blueMultiplier=args[2]->toNumber();
	else
		th->blueMultiplier=1.0;
	if(3 < argslen)
		th->alphaMultiplier=args[3]->toNumber();
	else
		th->alphaMultiplier=1.0;
	if(4 < argslen)
		th->redOffset=args[4]->toNumber();
	else
		th->redOffset=0.0;
	if(5 < argslen)
		th->greenOffset=args[5]->toNumber();
	else
		th->greenOffset=0.0;
	if(6 < argslen)
		th->blueOffset=args[6]->toNumber();
	else
		th->blueOffset=0.0;
	if(7 < argslen)
		th->alphaOffset=args[7]->toNumber();
	else
		th->alphaOffset=0.0;
	return NULL;
}

ASFUNCTIONBODY(ColorTransform,setColor)
{
	ColorTransform* th=static_cast<ColorTransform*>(obj);
	assert_and_throw(argslen==1);
	uintptr_t tmp=args[0]->toInt();
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
	return NULL;
}

ASFUNCTIONBODY(ColorTransform,getColor)
{
	assert_and_throw(false && "getColor not implemented");
	return NULL;
}

void Point::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setGetterByQName("x","",Class<IFunction>::getFunction(_getX),true);
	c->setGetterByQName("y","",Class<IFunction>::getFunction(_getY),true);
	c->setGetterByQName("length","",Class<IFunction>::getFunction(_getlength),true);
	c->setSetterByQName("x","",Class<IFunction>::getFunction(_setX),true);
	c->setSetterByQName("y","",Class<IFunction>::getFunction(_setY),true);
	c->setMethodByQName("interpolate","",Class<IFunction>::getFunction(interpolate),true);
	c->setMethodByQName("distance","",Class<IFunction>::getFunction(distance),true);
	c->setMethodByQName("add","",Class<IFunction>::getFunction(add),true);
	c->setMethodByQName("subtract","",Class<IFunction>::getFunction(subtract),true);
	c->setMethodByQName("clone","",Class<IFunction>::getFunction(clone),true);
	c->setMethodByQName("equals","",Class<IFunction>::getFunction(equals),true);
	c->setMethodByQName("normalize","",Class<IFunction>::getFunction(normalize),true);
	c->setMethodByQName("offset","",Class<IFunction>::getFunction(offset),true);
	c->setMethodByQName("polar","",Class<IFunction>::getFunction(polar),true);
}

void Point::buildTraits(ASObject* o)
{
}

tiny_string Point::toString(bool debugMsg)
{
	assert_and_throw(implEnable);
	
	char buf[512];
	snprintf(buf,512,"(a=%f, b=%f)",x,y);
	
	return tiny_string(buf, true);
}

number_t Point::len() const
{
	return sqrt(x*x + y*y);
}

ASFUNCTIONBODY(Point,_constructor)
{
	Point* th=static_cast<Point*>(obj);
	if(argslen>=1)
		th->x=args[0]->toNumber();
	if(argslen>=2)
		th->y=args[1]->toNumber();

	return NULL;
}

ASFUNCTIONBODY(Point,_getX)
{
	Point* th=static_cast<Point*>(obj);
	return abstract_d(th->x);
}

ASFUNCTIONBODY(Point,_getY)
{
	Point* th=static_cast<Point*>(obj);
	return abstract_d(th->y);
}

ASFUNCTIONBODY(Point,_setX)
{
	Point* th=static_cast<Point*>(obj);
	assert_and_throw(argslen==1);
	th->x = args[0]->toNumber();
	return NULL;
}

ASFUNCTIONBODY(Point,_setY)
{
	Point* th=static_cast<Point*>(obj);
	assert_and_throw(argslen==1);
	th->y = args[0]->toNumber();
	return NULL;
}

ASFUNCTIONBODY(Point,_getlength)
{
	Point* th=static_cast<Point*>(obj);
	assert_and_throw(argslen==0);
	return abstract_d(th->len());
}

ASFUNCTIONBODY(Point,interpolate)
{
	assert_and_throw(argslen==3);
	Point* pt1=static_cast<Point*>(args[0]);
	Point* pt2=static_cast<Point*>(args[1]);
	number_t f=args[2]->toNumber();
	Point* ret=Class<Point>::getInstanceS();
	ret->x = pt1->x + pt2->x * f;
	ret->y = pt1->y + pt2->y * f;
	return ret;
}

ASFUNCTIONBODY(Point,distance)
{
	assert_and_throw(argslen==2);
	Point* pt1=static_cast<Point*>(args[0]);
	Point* pt2=static_cast<Point*>(args[1]);
	Point temp(pt2->x - pt1->x, pt2->y - pt1->x);
	return abstract_d(temp.len());
}

ASFUNCTIONBODY(Point,add)
{
	Point* th=static_cast<Point*>(obj);
	assert_and_throw(argslen==1);
	Point* v=static_cast<Point*>(args[0]);
	Point* ret=Class<Point>::getInstanceS();
	ret->x = th->x + v->x;
	ret->y = th->y + v->y;
	return ret;
}

ASFUNCTIONBODY(Point,subtract)
{
	Point* th=static_cast<Point*>(obj);
	assert_and_throw(argslen==1);
	Point* v=static_cast<Point*>(args[0]);
	Point* ret=Class<Point>::getInstanceS();
	ret->x = th->x - v->x;
	ret->y = th->y - v->y;
	return ret;
}

ASFUNCTIONBODY(Point,clone)
{
	Point* th=static_cast<Point*>(obj);
	assert_and_throw(argslen==0);
	Point* ret=Class<Point>::getInstanceS();
	ret->x = th->x;
	ret->y = th->y;
	return ret;
}

ASFUNCTIONBODY(Point,equals)
{
	Point* th=static_cast<Point*>(obj);
	assert_and_throw(argslen==1);
	Point* toCompare=static_cast<Point*>(args[0]);
	return abstract_b((th->x == toCompare->x) & (th->y == toCompare->y));
}

ASFUNCTIONBODY(Point,normalize)
{
	Point* th=static_cast<Point*>(obj);
	assert_and_throw(argslen<2);
	number_t thickness = argslen > 0 ? args[0]->toNumber() : 1.0;
	number_t len = th->len();
	th->x = len == 0 ? 0 : th->x * thickness / len;
	th->y = len == 0 ? 0 : th->y * thickness / len;
	return NULL;
}

ASFUNCTIONBODY(Point,offset)
{
	Point* th=static_cast<Point*>(obj);
	assert_and_throw(argslen==2);
	number_t dx = args[0]->toNumber();
	number_t dy = args[1]->toNumber();
	th->x += dx;
	th->y += dy;
	return NULL;
}

ASFUNCTIONBODY(Point,polar)
{
	assert_and_throw(argslen==2);
	number_t len = args[0]->toNumber();
	number_t angle = args[1]->toNumber();
	Point* ret=Class<Point>::getInstanceS();
	ret->x = len * cos(angle);
	ret->y = len * sin(angle);
	return ret;
}

void Transform::sinit(Class_base* c)
{
	//c->constructor=Class<IFunction>::getFunction(_constructor);
	c->setConstructor(NULL);
	c->setSetterByQName("colorTransform","",Class<IFunction>::getFunction(undefinedFunction),true);
}

void Transform::buildTraits(ASObject* o)
{
}

void Matrix::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	
	//Properties
	c->setGetterByQName("a","",Class<IFunction>::getFunction(_get_a),true);
	c->setGetterByQName("b","",Class<IFunction>::getFunction(_get_b),true);
	c->setGetterByQName("c","",Class<IFunction>::getFunction(_get_c),true);
	c->setGetterByQName("d","",Class<IFunction>::getFunction(_get_d),true);
	c->setGetterByQName("tx","",Class<IFunction>::getFunction(_get_tx),true);
	c->setGetterByQName("ty","",Class<IFunction>::getFunction(_get_ty),true);
	
	c->setSetterByQName("a","",Class<IFunction>::getFunction(_set_a),true);
	c->setSetterByQName("b","",Class<IFunction>::getFunction(_set_b),true);
	c->setSetterByQName("c","",Class<IFunction>::getFunction(_set_c),true);
	c->setSetterByQName("d","",Class<IFunction>::getFunction(_set_d),true);
	c->setSetterByQName("tx","",Class<IFunction>::getFunction(_set_tx),true);
	c->setSetterByQName("ty","",Class<IFunction>::getFunction(_set_ty),true);
	
	//Methods 
	c->setMethodByQName("clone","",Class<IFunction>::getFunction(clone),true);
	c->setMethodByQName("concat","",Class<IFunction>::getFunction(concat),true);
	c->setMethodByQName("identity","",Class<IFunction>::getFunction(identity),true);
	c->setMethodByQName("invert","",Class<IFunction>::getFunction(invert),true);
	c->setMethodByQName("rotate","",Class<IFunction>::getFunction(rotate),true);
	c->setMethodByQName("scale","",Class<IFunction>::getFunction(scale),true);
	c->setMethodByQName("translate","",Class<IFunction>::getFunction(translate),true);
}

/**
 * NOTE: Many of these functions are wrong. They replace the current values instead of multiplying them out.
 */

ASFUNCTIONBODY(Matrix,_constructor)
{
	assert_and_throw(argslen <= 6);
	ASObject::_constructor(obj,NULL,0);
	
	Matrix* th=static_cast<Matrix*>(obj);
	
	//Identity matrix
	th->a = 1.0; th->c = 0.0; th->tx = 0.0;
	th->b = 0.0; th->d = 1.0; th->ty = 0.0;
	
	if (argslen >= 1)
		th->a = args[0]->toNumber();
	if (argslen >= 2)
		th->b = args[1]->toNumber();
	if (argslen >= 3)
		th->c = args[2]->toNumber();
	if (argslen >= 4)
		th->d = args[3]->toNumber();
	if (argslen >= 5)
		th->tx = args[4]->toNumber();
	if (argslen == 6)
		th->ty = args[5]->toNumber();

	return NULL;
}

void Matrix::buildTraits(ASObject* o)
{
}

tiny_string Matrix::toString(bool debugMsg)
{
	assert_and_throw(implEnable);
	
	char buf[512];
	snprintf(buf,512,"(a=%f, b=%f, c=%f, d=%f, tx=%f, ty=%f)",
		a, b, c, d, tx, ty);
	
	return tiny_string(buf, true);
}

ASFUNCTIONBODY(Matrix,_get_a)
{
	Matrix* th=static_cast<Matrix*>(obj);
	return abstract_d(th->a);
}

ASFUNCTIONBODY(Matrix,_set_a)
{
	Matrix* th=static_cast<Matrix*>(obj);
	assert_and_throw(argslen==1);
	th->a = args[0]->toNumber();
	return NULL;
}

ASFUNCTIONBODY(Matrix,_get_b)
{
	Matrix* th=static_cast<Matrix*>(obj);
	return abstract_d(th->b);
}

ASFUNCTIONBODY(Matrix,_set_b)
{
	Matrix* th=static_cast<Matrix*>(obj);
	assert_and_throw(argslen==1);
	th->b = args[0]->toNumber();
	return NULL;
}

ASFUNCTIONBODY(Matrix,_get_c)
{
	Matrix* th=static_cast<Matrix*>(obj);
	return abstract_d(th->c);
}

ASFUNCTIONBODY(Matrix,_set_c)
{
	Matrix* th=static_cast<Matrix*>(obj);
	assert_and_throw(argslen==1);
	th->c = args[0]->toNumber();
	return NULL;
}

ASFUNCTIONBODY(Matrix,_get_d)
{
	Matrix* th=static_cast<Matrix*>(obj);
	return abstract_d(th->d);
}

ASFUNCTIONBODY(Matrix,_set_d)
{
	Matrix* th=static_cast<Matrix*>(obj);
	assert_and_throw(argslen==1);
	th->d = args[0]->toNumber();
	return NULL;
}

ASFUNCTIONBODY(Matrix,_get_tx)
{
	Matrix* th=static_cast<Matrix*>(obj);
	return abstract_d(th->tx);
}

ASFUNCTIONBODY(Matrix,_set_tx)
{
	Matrix* th=static_cast<Matrix*>(obj);
	assert_and_throw(argslen==1);
	th->tx = args[0]->toNumber();
	return NULL;
}

ASFUNCTIONBODY(Matrix,_get_ty)
{
	Matrix* th=static_cast<Matrix*>(obj);
	return abstract_d(th->ty);
}

ASFUNCTIONBODY(Matrix,_set_ty)
{
	Matrix* th=static_cast<Matrix*>(obj);
	assert_and_throw(argslen==1);
	th->ty = args[0]->toNumber();
	return NULL;
}

ASFUNCTIONBODY(Matrix,clone)
{
	assert_and_throw(argslen==0);

	Matrix* th=static_cast<Matrix*>(obj);
	Matrix* ret=Class<Matrix>::getInstanceS();
	
	ret->a = th->a; ret->c = th->c; ret->tx = th->tx;
	ret->b = th->b; ret->d = th->d; ret->ty = th->ty;
		
	return ret;
}

ASFUNCTIONBODY(Matrix,concat)
{
	assert_and_throw(argslen==1);

	Matrix* th=static_cast<Matrix*>(obj);
	Matrix* m=static_cast<Matrix*>(args[0]);

	number_t ta, tb, tc, td;

	ta = th->a * m->a + th->c * m->c + th->tx * m->tx;
	tb = th->b * m->a + th->d * m->c + th->ty * m->tx;
	tc = th->a * m->b + th->c * m->d + th->tx * m->ty;
	td = th->b * m->b + th->d * m->d + th->ty * m->ty;

	th->a = ta;
	th->b = tb;
	th->c = tc;
	th->d = td;

	return NULL;
}

ASFUNCTIONBODY(Matrix,identity)
{
	Matrix* th=static_cast<Matrix*>(obj);
	assert_and_throw(argslen==0);
	
	th->a = 1.0; th->c = 0.0; th->tx = 0.0;
	th->b = 0.0; th->d = 1.0; th->ty = 0.0;
		
	return NULL;
}

ASFUNCTIONBODY(Matrix,invert)
{
	assert_and_throw(argslen==0);
	Matrix* th=static_cast<Matrix*>(obj);
	
	number_t ta, tb, tc, td, ttx, tty;
	number_t Z;

	Z = th->a * th->d - th->b * th-> c;
	ta = th->d;
	ta /= Z;
	tc = -(th->c);
	tc /= Z;
	ttx = th->c * th->ty + th->d * th->tx;
	ttx /= Z;
	tb = -(th->b);
	tb /= Z;
	td = th->a;
	td /= Z;
	tty = th->b * th->tx - th->a * th->ty;
	tty /= Z;

	th->a = ta;
	th->b = tb;
	th->c = tc;
	th->d = td;
	th->tx = ttx;
	th->ty = tty;

	return NULL;
}

ASFUNCTIONBODY(Matrix,rotate)
{
	Matrix* th=static_cast<Matrix*>(obj);
	assert_and_throw(argslen==1);
	double angle = args[0]->toNumber();
	th->a = ::cos(angle); th->c = -::sin(angle); th->tx = 0.0;
	th->b = ::sin(angle); th->d =  ::cos(angle); th->ty = 0.0;

	return NULL;
}

ASFUNCTIONBODY(Matrix,scale)
{
	Matrix* th=static_cast<Matrix*>(obj);
	assert_and_throw(argslen==2);
	double sx = args[0]->toNumber();
	double sy = args[1]->toNumber();
	th->a = sx;   th->c = 0.0; th->tx = 0.0;
	th->b = 0.0;  th->d = sy;  th->ty = 0.0;
		
	return NULL;
}

ASFUNCTIONBODY(Matrix,translate)
{
	Matrix* th=static_cast<Matrix*>(obj);
	assert_and_throw(argslen==2);
	double dx = args[0]->toNumber();
	double dy = args[1]->toNumber();
	th->tx += dx;
	th->ty += dy;
		
	return NULL;
}
