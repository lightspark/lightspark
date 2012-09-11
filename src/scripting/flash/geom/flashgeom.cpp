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

#include "scripting/abc.h"
#include "scripting/flash/geom/flashgeom.h"
#include "scripting/argconv.h"

using namespace lightspark;
using namespace std;

void Rectangle::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	IFunction* gleft=Class<IFunction>::getFunction(_getLeft);
	c->setDeclaredMethodByQName("left","",gleft,GETTER_METHOD,true);
	gleft->incRef();
	c->setDeclaredMethodByQName("x","",gleft,GETTER_METHOD,true);
	IFunction* sleft=Class<IFunction>::getFunction(_setLeft);
	c->setDeclaredMethodByQName("left","",sleft,SETTER_METHOD,true);
	sleft->incRef();
	c->setDeclaredMethodByQName("x","",sleft,SETTER_METHOD,true);
	c->setDeclaredMethodByQName("right","",Class<IFunction>::getFunction(_getRight),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("right","",Class<IFunction>::getFunction(_setRight),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("width","",Class<IFunction>::getFunction(_getWidth),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("width","",Class<IFunction>::getFunction(_setWidth),SETTER_METHOD,true);

	IFunction* gtop=Class<IFunction>::getFunction(_getTop);
	c->setDeclaredMethodByQName("top","",gtop,GETTER_METHOD,true);
	gtop->incRef();
	c->setDeclaredMethodByQName("y","",gtop,GETTER_METHOD,true);
	IFunction* stop=Class<IFunction>::getFunction(_setTop);
	c->setDeclaredMethodByQName("top","",stop,SETTER_METHOD,true);
	stop->incRef();
	c->setDeclaredMethodByQName("y","",stop,SETTER_METHOD,true);

	c->setDeclaredMethodByQName("bottom","",Class<IFunction>::getFunction(_getBottom),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("bottom","",Class<IFunction>::getFunction(_setBottom),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("height","",Class<IFunction>::getFunction(_getHeight),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("height","",Class<IFunction>::getFunction(_setHeight),SETTER_METHOD,true);

	c->setDeclaredMethodByQName("bottomRight","",Class<IFunction>::getFunction(_getBottomRight),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("bottomRight","",Class<IFunction>::getFunction(_setBottomRight),SETTER_METHOD,true);

	c->setDeclaredMethodByQName("size","",Class<IFunction>::getFunction(_getSize),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("size","",Class<IFunction>::getFunction(_setSize),SETTER_METHOD,true);

	c->setDeclaredMethodByQName("topLeft","",Class<IFunction>::getFunction(_getTopLeft),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("topLeft","",Class<IFunction>::getFunction(_setTopLeft),SETTER_METHOD,true);

	c->setDeclaredMethodByQName("clone","",Class<IFunction>::getFunction(clone),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("contains","",Class<IFunction>::getFunction(contains),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("containsPoint","",Class<IFunction>::getFunction(containsPoint),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("containsRect","",Class<IFunction>::getFunction(containsRect),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("equals","",Class<IFunction>::getFunction(equals),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("inflate","",Class<IFunction>::getFunction(inflate),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("inflatePoint","",Class<IFunction>::getFunction(inflatePoint),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("intersection","",Class<IFunction>::getFunction(intersection),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("intersects","",Class<IFunction>::getFunction(intersects),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("isEmpty","",Class<IFunction>::getFunction(isEmpty),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("offset","",Class<IFunction>::getFunction(offset),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("offsetPoint","",Class<IFunction>::getFunction(offsetPoint),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setEmpty","",Class<IFunction>::getFunction(setEmpty),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("union","",Class<IFunction>::getFunction(_union),NORMAL_METHOD,true);
	c->prototype->setVariableByQName("toString","",Class<IFunction>::getFunction(_toString),DYNAMIC_TRAIT);
}

void Rectangle::buildTraits(ASObject* o)
{
}

const lightspark::RECT Rectangle::getRect() const
{
	return lightspark::RECT(x,x+width,y,y+height);
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

ASFUNCTIONBODY(Rectangle,_toString)
{
	Rectangle* th=static_cast<Rectangle*>(obj);
	char buf[512];
	snprintf(buf,512,"(x=%.2f, y=%.2f, w=%.2f, h=%.2f)",th->x,th->y,th->width,th->height);
	return Class<ASString>::getInstanceS(buf);
}

void ColorTransform::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));

	// properties
	c->setDeclaredMethodByQName("color","",Class<IFunction>::getFunction(getColor),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("color","",Class<IFunction>::getFunction(setColor),SETTER_METHOD,true);

	c->setDeclaredMethodByQName("redMultiplier","",Class<IFunction>::getFunction(getRedMultiplier),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("redMultiplier","",Class<IFunction>::getFunction(setRedMultiplier),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("greenMultiplier","",Class<IFunction>::getFunction(getGreenMultiplier),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("greenMultiplier","",Class<IFunction>::getFunction(setGreenMultiplier),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("blueMultiplier","",Class<IFunction>::getFunction(getBlueMultiplier),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("blueMultiplier","",Class<IFunction>::getFunction(setBlueMultiplier),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("alphaMultiplier","",Class<IFunction>::getFunction(getAlphaMultiplier),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("alphaMultiplier","",Class<IFunction>::getFunction(setAlphaMultiplier),SETTER_METHOD,true);

	c->setDeclaredMethodByQName("redOffset","",Class<IFunction>::getFunction(getRedOffset),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("redOffset","",Class<IFunction>::getFunction(setRedOffset),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("greenOffset","",Class<IFunction>::getFunction(getGreenOffset),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("greenOffset","",Class<IFunction>::getFunction(setGreenOffset),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("blueOffset","",Class<IFunction>::getFunction(getBlueOffset),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("blueOffset","",Class<IFunction>::getFunction(setBlueOffset),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("alphaOffset","",Class<IFunction>::getFunction(getAlphaOffset),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("alphaOffset","",Class<IFunction>::getFunction(setAlphaOffset),SETTER_METHOD,true);

	// methods
	c->setDeclaredMethodByQName("concat","",Class<IFunction>::getFunction(concat),NORMAL_METHOD,true);
	c->prototype->setVariableByQName("toString","",Class<IFunction>::getFunction(_toString),DYNAMIC_TRAIT);
}

void ColorTransform::buildTraits(ASObject* o)
{
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
	uint32_t tmp=args[0]->toInt();
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
	assert_and_throw(argslen==1);
	ColorTransform* th=static_cast<ColorTransform*>(obj);

	int ao, ro, go, bo;
	ao = static_cast<int>(th->alphaOffset) & 0xff;
	ro = static_cast<int>(th->redOffset) & 0xff;
	go = static_cast<int>(th->greenOffset) & 0xff;
	bo = static_cast<int>(th->blueOffset) & 0xff;

	number_t color = (ao<<24) | (ro<<16) | (go<<8) | bo;

	return abstract_d(color);
}

ASFUNCTIONBODY(ColorTransform,getRedMultiplier)
{
	ColorTransform* th=static_cast<ColorTransform*>(obj);
	return abstract_d(th->redMultiplier);
}

ASFUNCTIONBODY(ColorTransform,setRedMultiplier)
{
	assert_and_throw(argslen==1);
	ColorTransform* th=static_cast<ColorTransform*>(obj);
	th->redMultiplier = args[0]->toNumber();
	return NULL;
}

ASFUNCTIONBODY(ColorTransform,getGreenMultiplier)
{
	ColorTransform* th=static_cast<ColorTransform*>(obj);
	return abstract_d(th->greenMultiplier);
}

ASFUNCTIONBODY(ColorTransform,setGreenMultiplier)
{
	assert_and_throw(argslen==1);
	ColorTransform* th=static_cast<ColorTransform*>(obj);
	th->greenMultiplier = args[0]->toNumber();
	return NULL;
}

ASFUNCTIONBODY(ColorTransform,getBlueMultiplier)
{
	ColorTransform* th=static_cast<ColorTransform*>(obj);
	return abstract_d(th->blueMultiplier);
}

ASFUNCTIONBODY(ColorTransform,setBlueMultiplier)
{
	assert_and_throw(argslen==1);
	ColorTransform* th=static_cast<ColorTransform*>(obj);
	th->blueMultiplier = args[0]->toNumber();
	return NULL;
}

ASFUNCTIONBODY(ColorTransform,getAlphaMultiplier)
{
	ColorTransform* th=static_cast<ColorTransform*>(obj);
	return abstract_d(th->alphaMultiplier);
}

ASFUNCTIONBODY(ColorTransform,setAlphaMultiplier)
{
	assert_and_throw(argslen==1);
	ColorTransform* th=static_cast<ColorTransform*>(obj);
	th->alphaMultiplier = args[0]->toNumber();
	return NULL;
}

ASFUNCTIONBODY(ColorTransform,getRedOffset)
{
	ColorTransform* th=static_cast<ColorTransform*>(obj);
	return abstract_d(th->redOffset);
}

ASFUNCTIONBODY(ColorTransform,setRedOffset)
{
	assert_and_throw(argslen==1);
	ColorTransform* th=static_cast<ColorTransform*>(obj);
	th->redOffset = args[0]->toNumber();
	return NULL;
}

ASFUNCTIONBODY(ColorTransform,getGreenOffset)
{
	ColorTransform* th=static_cast<ColorTransform*>(obj);
	return abstract_d(th->greenOffset);
}

ASFUNCTIONBODY(ColorTransform,setGreenOffset)
{
	assert_and_throw(argslen==1);
	ColorTransform* th=static_cast<ColorTransform*>(obj);
	th->greenOffset = args[0]->toNumber();
	return NULL;
}

ASFUNCTIONBODY(ColorTransform,getBlueOffset)
{
	ColorTransform* th=static_cast<ColorTransform*>(obj);
	return abstract_d(th->blueOffset);
}

ASFUNCTIONBODY(ColorTransform,setBlueOffset)
{
	assert_and_throw(argslen==1);
	ColorTransform* th=static_cast<ColorTransform*>(obj);
	th->blueOffset = args[0]->toNumber();
	return NULL;
}

ASFUNCTIONBODY(ColorTransform,getAlphaOffset)
{
	ColorTransform* th=static_cast<ColorTransform*>(obj);
	return abstract_d(th->alphaOffset);
}

ASFUNCTIONBODY(ColorTransform,setAlphaOffset)
{
	assert_and_throw(argslen==1);
	ColorTransform* th=static_cast<ColorTransform*>(obj);
	th->alphaOffset = args[0]->toNumber();
	return NULL;
}

ASFUNCTIONBODY(ColorTransform,concat)
{
	assert_and_throw(argslen==1);
	ColorTransform* th=static_cast<ColorTransform*>(obj);
	ColorTransform* ct=static_cast<ColorTransform*>(args[0]);

	th->redMultiplier *= ct->redMultiplier;
	th->redOffset = th->redOffset * ct->redMultiplier + ct->redOffset;
	th->greenMultiplier *= ct->greenMultiplier;
	th->greenOffset = th->greenOffset * ct->greenMultiplier + ct->greenOffset;
	th->blueMultiplier *= ct->blueMultiplier;
	th->blueOffset = th->blueOffset * ct->blueMultiplier + ct->blueOffset;
	th->alphaMultiplier *= ct->alphaMultiplier;
	th->alphaOffset = th->alphaOffset * ct->alphaMultiplier + ct->alphaOffset;

	return NULL;
}

ASFUNCTIONBODY(ColorTransform,_toString)
{
	ColorTransform* th=static_cast<ColorTransform*>(obj);
	char buf[1024];
	snprintf(buf,1024,"(redOffset=%f, redMultiplier=%f, greenOffset=%f, greenMultiplier=%f blueOffset=%f blueMultiplier=%f alphaOffset=%f, alphaMultiplier=%f)",
			th->redOffset, th->redMultiplier, th->greenOffset, th->greenMultiplier, th->blueOffset, th->blueMultiplier, th->alphaOffset, th->alphaMultiplier);
	
	return Class<ASString>::getInstanceS(buf);
}

void Point::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setDeclaredMethodByQName("x","",Class<IFunction>::getFunction(_getX),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("y","",Class<IFunction>::getFunction(_getY),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("length","",Class<IFunction>::getFunction(_getlength),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("x","",Class<IFunction>::getFunction(_setX),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("y","",Class<IFunction>::getFunction(_setY),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("interpolate","",Class<IFunction>::getFunction(interpolate),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("distance","",Class<IFunction>::getFunction(distance),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("add","",Class<IFunction>::getFunction(add),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("subtract","",Class<IFunction>::getFunction(subtract),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("clone","",Class<IFunction>::getFunction(clone),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("equals","",Class<IFunction>::getFunction(equals),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("normalize","",Class<IFunction>::getFunction(normalize),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("offset","",Class<IFunction>::getFunction(offset),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("polar","",Class<IFunction>::getFunction(polar),NORMAL_METHOD,false);
	c->prototype->setVariableByQName("toString","",Class<IFunction>::getFunction(_toString),DYNAMIC_TRAIT);
}

void Point::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(Point,_toString)
{
	Point* th=static_cast<Point*>(obj);
	char buf[512];
	snprintf(buf,512,"(a=%f, b=%f)",th->x,th->y);
	return Class<ASString>::getInstanceS(buf);
}

number_t Point::lenImpl(number_t x, number_t y)
{
	return sqrt(x*x + y*y);
}

number_t Point::len() const
{
	return lenImpl(x, y);
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
	number_t ret=lenImpl(pt2->x - pt1->x, pt2->y - pt1->y);
	return abstract_d(ret);
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

Transform::Transform(Class_base* c, _R<DisplayObject> o):ASObject(c),owner(o)
{
}

void Transform::finalize()
{
	owner.reset();
	ASObject::finalize();
}

void Transform::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setDeclaredMethodByQName("colorTransform","",Class<IFunction>::getFunction(undefinedFunction),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("matrix","",Class<IFunction>::getFunction(_getMatrix),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("matrix","",Class<IFunction>::getFunction(_setMatrix),SETTER_METHOD,true);
}

ASFUNCTIONBODY(Transform,_constructor)
{
	assert_and_throw(argslen==0);
	return NULL;
}

ASFUNCTIONBODY(Transform,_getMatrix)
{
	Transform* th=Class<Transform>::cast(obj);
	assert_and_throw(argslen==0);
	const MATRIX& ret=th->owner->getMatrix();
	return Class<Matrix>::getInstanceS(ret);
}

ASFUNCTIONBODY(Transform,_setMatrix)
{
	Transform* th=Class<Transform>::cast(obj);
	_NR<Matrix> m;
	ARG_UNPACK(m);
	if(m.isNull())
		return NULL;

	th->owner->setMatrix(m->getMATRIX());
	return NULL;
}

void Transform::buildTraits(ASObject* o)
{
}

Matrix::Matrix(Class_base* c):ASObject(c)
{
}

Matrix::Matrix(Class_base* c, const MATRIX& m):ASObject(c),matrix(m)
{
}

void Matrix::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	
	//Properties
	c->setDeclaredMethodByQName("a","",Class<IFunction>::getFunction(_get_a),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("b","",Class<IFunction>::getFunction(_get_b),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("c","",Class<IFunction>::getFunction(_get_c),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("d","",Class<IFunction>::getFunction(_get_d),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("tx","",Class<IFunction>::getFunction(_get_tx),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("ty","",Class<IFunction>::getFunction(_get_ty),GETTER_METHOD,true);
	
	c->setDeclaredMethodByQName("a","",Class<IFunction>::getFunction(_set_a),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("b","",Class<IFunction>::getFunction(_set_b),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("c","",Class<IFunction>::getFunction(_set_c),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("d","",Class<IFunction>::getFunction(_set_d),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("tx","",Class<IFunction>::getFunction(_set_tx),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("ty","",Class<IFunction>::getFunction(_set_ty),SETTER_METHOD,true);
	
	//Methods 
	c->setDeclaredMethodByQName("clone","",Class<IFunction>::getFunction(clone),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("concat","",Class<IFunction>::getFunction(concat),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("createBox","",Class<IFunction>::getFunction(createBox),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("createGradientBox","",Class<IFunction>::getFunction(createGradientBox),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("deltaTransformPoint","",Class<IFunction>::getFunction(deltaTransformPoint),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("identity","",Class<IFunction>::getFunction(identity),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("invert","",Class<IFunction>::getFunction(invert),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("rotate","",Class<IFunction>::getFunction(rotate),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("scale","",Class<IFunction>::getFunction(scale),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("transformPoint","",Class<IFunction>::getFunction(transformPoint),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("translate","",Class<IFunction>::getFunction(translate),NORMAL_METHOD,true);
	c->prototype->setVariableByQName("toString","",Class<IFunction>::getFunction(_toString),DYNAMIC_TRAIT);
}

ASFUNCTIONBODY(Matrix,_constructor)
{
	assert_and_throw(argslen <= 6);
	ASObject::_constructor(obj,NULL,0);
	
	Matrix* th=static_cast<Matrix*>(obj);
	
	//Mapping to cairo_matrix_t
	//a -> xx
	//b -> yx
	//c -> xy
	//d -> yy
	//tx -> x0
	//ty -> y0
	
	if (argslen >= 1)
		th->matrix.xx = args[0]->toNumber();
	if (argslen >= 2)
		th->matrix.yx = args[1]->toNumber();
	if (argslen >= 3)
		th->matrix.xy = args[2]->toNumber();
	if (argslen >= 4)
		th->matrix.yy = args[3]->toNumber();
	if (argslen >= 5)
		th->matrix.x0 = args[4]->toNumber();
	if (argslen == 6)
		th->matrix.y0 = args[5]->toNumber();

	return NULL;
}

void Matrix::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(Matrix,_toString)
{
	Matrix* th=static_cast<Matrix*>(obj);
	char buf[512];
	snprintf(buf,512,"(a=%f, b=%f, c=%f, d=%f, tx=%f, ty=%f)",
			th->matrix.xx, th->matrix.yx, th->matrix.xy, th->matrix.yy, th->matrix.x0, th->matrix.y0);
	return Class<ASString>::getInstanceS(buf);
}

MATRIX Matrix::getMATRIX() const
{
	return matrix;
}

ASFUNCTIONBODY(Matrix,_get_a)
{
	Matrix* th=static_cast<Matrix*>(obj);
	return abstract_d(th->matrix.xx);
}

ASFUNCTIONBODY(Matrix,_set_a)
{
	Matrix* th=static_cast<Matrix*>(obj);
	assert_and_throw(argslen==1);
	th->matrix.xx = args[0]->toNumber();
	return NULL;
}

ASFUNCTIONBODY(Matrix,_get_b)
{
	Matrix* th=static_cast<Matrix*>(obj);
	return abstract_d(th->matrix.yx);
}

ASFUNCTIONBODY(Matrix,_set_b)
{
	Matrix* th=static_cast<Matrix*>(obj);
	assert_and_throw(argslen==1);
	th->matrix.yx = args[0]->toNumber();
	return NULL;
}

ASFUNCTIONBODY(Matrix,_get_c)
{
	Matrix* th=static_cast<Matrix*>(obj);
	return abstract_d(th->matrix.xy);
}

ASFUNCTIONBODY(Matrix,_set_c)
{
	Matrix* th=static_cast<Matrix*>(obj);
	assert_and_throw(argslen==1);
	th->matrix.xy = args[0]->toNumber();
	return NULL;
}

ASFUNCTIONBODY(Matrix,_get_d)
{
	Matrix* th=static_cast<Matrix*>(obj);
	return abstract_d(th->matrix.yy);
}

ASFUNCTIONBODY(Matrix,_set_d)
{
	Matrix* th=static_cast<Matrix*>(obj);
	assert_and_throw(argslen==1);
	th->matrix.yy = args[0]->toNumber();
	return NULL;
}

ASFUNCTIONBODY(Matrix,_get_tx)
{
	Matrix* th=static_cast<Matrix*>(obj);
	return abstract_d(th->matrix.x0);
}

ASFUNCTIONBODY(Matrix,_set_tx)
{
	Matrix* th=static_cast<Matrix*>(obj);
	assert_and_throw(argslen==1);
	th->matrix.x0 = args[0]->toNumber();
	return NULL;
}

ASFUNCTIONBODY(Matrix,_get_ty)
{
	Matrix* th=static_cast<Matrix*>(obj);
	return abstract_d(th->matrix.y0);
}

ASFUNCTIONBODY(Matrix,_set_ty)
{
	Matrix* th=static_cast<Matrix*>(obj);
	assert_and_throw(argslen==1);
	th->matrix.y0 = args[0]->toNumber();
	return NULL;
}

ASFUNCTIONBODY(Matrix,clone)
{
	assert_and_throw(argslen==0);

	Matrix* th=static_cast<Matrix*>(obj);
	Matrix* ret=Class<Matrix>::getInstanceS(th->matrix);
	return ret;
}

ASFUNCTIONBODY(Matrix,concat)
{
	assert_and_throw(argslen==1);

	Matrix* th=static_cast<Matrix*>(obj);
	Matrix* m=static_cast<Matrix*>(args[0]);

	//Premultiply, which is flash convention
	cairo_matrix_multiply(&th->matrix,&th->matrix,&m->matrix);
	return NULL;
}

ASFUNCTIONBODY(Matrix,identity)
{
	Matrix* th=static_cast<Matrix*>(obj);
	assert_and_throw(argslen==0);
	
	cairo_matrix_init_identity(&th->matrix);
	return NULL;
}

ASFUNCTIONBODY(Matrix,invert)
{
	assert_and_throw(argslen==0);
	Matrix* th=static_cast<Matrix*>(obj);
	th->matrix=th->matrix.getInverted();
	return NULL;
}

ASFUNCTIONBODY(Matrix,translate)
{
	assert_and_throw(argslen==2);
	Matrix* th=static_cast<Matrix*>(obj);
	number_t dx = args[0]->toNumber();
	number_t dy = args[1]->toNumber();

	th->matrix.translate(dx,dy);
	return NULL;
}

ASFUNCTIONBODY(Matrix,rotate)
{
	assert_and_throw(argslen==1);
	Matrix* th=static_cast<Matrix*>(obj);
	number_t angle = args[0]->toNumber();

	th->matrix.rotate(angle);
	return NULL;
}

ASFUNCTIONBODY(Matrix,scale)
{
	assert_and_throw(argslen==2);
	Matrix* th=static_cast<Matrix*>(obj);
	number_t sx = args[0]->toNumber();
	number_t sy = args[1]->toNumber();

	th->matrix.scale(sx, sy);
	return NULL;
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

ASFUNCTIONBODY(Matrix,createBox)
{
	assert_and_throw(argslen>=2 && argslen <= 5);
	Matrix* th=static_cast<Matrix*>(obj);
	number_t scaleX = args[0]->toNumber();
	number_t scaleY = args[1]->toNumber();

	number_t angle = 0;
	if ( argslen > 2 ) angle = args[2]->toNumber();

	number_t translateX = 0;
	if ( argslen > 3 ) translateX = args[3]->toNumber();

	number_t translateY = 0;
	if ( argslen > 4 ) translateY = args[4]->toNumber();

	th->_createBox(scaleX, scaleY, angle, translateX, translateY);

	return NULL;
}

ASFUNCTIONBODY(Matrix,createGradientBox)
{
	assert_and_throw(argslen>=2 && argslen <= 5);
	Matrix* th=static_cast<Matrix*>(obj);
	number_t width  = args[0]->toNumber();
	number_t height = args[1]->toNumber();

	number_t angle = 0;
	if ( argslen > 2 ) angle = args[2]->toNumber();

	number_t translateX = width/2;
	if ( argslen > 3 ) translateX += args[3]->toNumber();

	number_t translateY = height/2;
	if ( argslen > 4 ) translateY += args[4]->toNumber();

	th->_createBox(width / 1638.4, height / 1638.4, angle, translateX, translateY);

	return NULL;
}

ASFUNCTIONBODY(Matrix,transformPoint)
{
	assert_and_throw(argslen==1);
	Matrix* th=static_cast<Matrix*>(obj);
	Point* pt=static_cast<Point*>(args[0]);

	number_t ttx = pt->getX();
	number_t tty = pt->getY();
	cairo_matrix_transform_point(&th->matrix,&ttx,&tty);
	return Class<Point>::getInstanceS(ttx, tty);
}

ASFUNCTIONBODY(Matrix,deltaTransformPoint)
{
	assert_and_throw(argslen==1);
	Matrix* th=static_cast<Matrix*>(obj);
	Point* pt=static_cast<Point*>(args[0]);

	number_t ttx = pt->getX();
	number_t tty = pt->getY();
	cairo_matrix_transform_distance(&th->matrix,&ttx,&tty);
	return Class<Point>::getInstanceS(ttx, tty);
}

void Vector3D::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));

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
	c->setDeclaredMethodByQName("w","",Class<IFunction>::getFunction(_get_w),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("x","",Class<IFunction>::getFunction(_get_x),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("y","",Class<IFunction>::getFunction(_get_y),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("z","",Class<IFunction>::getFunction(_get_z),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("length","",Class<IFunction>::getFunction(_get_length),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("lengthSquared","",Class<IFunction>::getFunction(_get_lengthSquared),GETTER_METHOD,true);
	
	c->setDeclaredMethodByQName("w","",Class<IFunction>::getFunction(_set_w),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("x","",Class<IFunction>::getFunction(_set_x),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("y","",Class<IFunction>::getFunction(_set_y),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("z","",Class<IFunction>::getFunction(_set_z),SETTER_METHOD,true);
	
	// methods 
	c->setDeclaredMethodByQName("add","",Class<IFunction>::getFunction(add),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("angleBetween","",Class<IFunction>::getFunction(angleBetween),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("clone","",Class<IFunction>::getFunction(clone),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("crossProduct","",Class<IFunction>::getFunction(crossProduct),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("decrementBy","",Class<IFunction>::getFunction(decrementBy),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("distance","",Class<IFunction>::getFunction(distance),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("dotProduct","",Class<IFunction>::getFunction(dotProduct),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("equals","",Class<IFunction>::getFunction(equals),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("incrementBy","",Class<IFunction>::getFunction(incrementBy),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("nearEquals","",Class<IFunction>::getFunction(nearEquals),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("negate","",Class<IFunction>::getFunction(negate),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("normalize","",Class<IFunction>::getFunction(normalize),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("project","",Class<IFunction>::getFunction(project),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("scaleBy","",Class<IFunction>::getFunction(scaleBy),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("subtract","",Class<IFunction>::getFunction(subtract),NORMAL_METHOD,true);
	c->prototype->setVariableByQName("toString","",Class<IFunction>::getFunction(_toString),DYNAMIC_TRAIT);
}

ASFUNCTIONBODY(Vector3D,_constructor)
{
	assert_and_throw(argslen <= 4);
	ASObject::_constructor(obj,NULL,0);
	
	Vector3D * th=static_cast<Vector3D*>(obj);
	
	th->w = 0;
	th->x = 0;
	th->y = 0;
	th->z = 0;
	
	if (argslen >= 1)
		th->x = args[0]->toNumber();
	if (argslen >= 2)
		th->y = args[1]->toNumber();
	if (argslen >= 3)
		th->z = args[2]->toNumber();
	if (argslen == 4)
		th->w = args[3]->toNumber();

	return NULL;
}

void Vector3D::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(Vector3D,_toString)
{
	Vector3D* th=static_cast<Vector3D*>(obj);
	char buf[512];
	snprintf(buf,512,"(x=%f, y=%f, z=%f)", th->x, th->y, th->z);
	return Class<ASString>::getInstanceS(buf);
}

ASFUNCTIONBODY(Vector3D,_get_w)
{
	assert_and_throw(argslen==0);

	Vector3D* th=static_cast<Vector3D*>(obj);
	return abstract_d(th->w);
}

ASFUNCTIONBODY(Vector3D,_set_w)
{
	assert_and_throw(argslen==1);
	Vector3D* th=static_cast<Vector3D*>(obj);
	th->w = args[0]->toNumber();
	return NULL;
}

ASFUNCTIONBODY(Vector3D,_get_x)
{
	assert_and_throw(argslen==0);

	Vector3D* th=static_cast<Vector3D*>(obj);
	return abstract_d(th->x);
}

ASFUNCTIONBODY(Vector3D,_set_x)
{
	assert_and_throw(argslen==1);
	Vector3D* th=static_cast<Vector3D*>(obj);
	th->x = args[0]->toNumber();
	return NULL;
}

ASFUNCTIONBODY(Vector3D,_get_y)
{
	assert_and_throw(argslen==0);

	Vector3D* th=static_cast<Vector3D*>(obj);
	return abstract_d(th->y);
}

ASFUNCTIONBODY(Vector3D,_set_y)
{
	assert_and_throw(argslen==1);
	Vector3D* th=static_cast<Vector3D*>(obj);
	th->y = args[0]->toNumber();
	return NULL;
}

ASFUNCTIONBODY(Vector3D,_get_z)
{
	assert_and_throw(argslen==0);

	Vector3D* th=static_cast<Vector3D*>(obj);
	return abstract_d(th->z);
}

ASFUNCTIONBODY(Vector3D,_set_z)
{
	assert_and_throw(argslen==1);
	Vector3D* th=static_cast<Vector3D*>(obj);
	th->z = args[0]->toNumber();
	return NULL;
}

ASFUNCTIONBODY(Vector3D,_get_length)
{
	assert_and_throw(argslen==0);

	Vector3D* th=static_cast<Vector3D*>(obj);
	return abstract_d(sqrt(th->x * th->x + th->y * th->y + th->z * th->z));
}

ASFUNCTIONBODY(Vector3D,_get_lengthSquared)
{
	assert_and_throw(argslen==0);

	Vector3D* th=static_cast<Vector3D*>(obj);
	return abstract_d(th->x * th->x + th->y * th->y + th->z * th->z);
}

ASFUNCTIONBODY(Vector3D,clone)
{
	assert_and_throw(argslen==0);

	Vector3D* th=static_cast<Vector3D*>(obj);
	Vector3D* ret=Class<Vector3D>::getInstanceS();

	ret->w = th->w;
	ret->x = th->x;
	ret->y = th->y;
	ret->z = th->z;

	return ret;
}

ASFUNCTIONBODY(Vector3D,add)
{
	assert_and_throw(argslen==1);

	Vector3D* th=static_cast<Vector3D*>(obj);
	Vector3D* vc=static_cast<Vector3D*>(args[0]);
	Vector3D* ret=Class<Vector3D>::getInstanceS();

	ret->x = th->x + vc->x;
	ret->y = th->y + vc->y;
	ret->z = th->z + vc->z;

	return ret;
}

ASFUNCTIONBODY(Vector3D,angleBetween)
{
	assert_and_throw(argslen==2);

	Vector3D* vc1=static_cast<Vector3D*>(args[0]);
	Vector3D* vc2=static_cast<Vector3D*>(args[1]);

	number_t angle = vc1->x * vc2->x + vc1->y * vc2->y + vc1->z * vc2->z;
	angle /= sqrt(vc1->x * vc1->x + vc1->y * vc1->y + vc1->z * vc1->z);
	angle /= sqrt(vc2->x * vc2->x + vc2->y * vc2->y + vc2->z * vc2->z);
	angle = acos(angle);

	return abstract_d(angle);
}

ASFUNCTIONBODY(Vector3D,crossProduct)
{
	assert_and_throw(argslen==1);

	Vector3D* th=static_cast<Vector3D*>(obj);
	Vector3D* vc=static_cast<Vector3D*>(args[0]);
	Vector3D* ret=Class<Vector3D>::getInstanceS();

	ret->x = th->y * vc->z - th->z * vc->y;
	ret->y = th->z * vc->x - th->x * vc->z;
	ret->z = th->x * vc->y - th->y * vc->x;

	return ret;
}

ASFUNCTIONBODY(Vector3D,decrementBy)
{
	assert_and_throw(argslen==1);

	Vector3D* th=static_cast<Vector3D*>(obj);
	Vector3D* vc=static_cast<Vector3D*>(args[0]);

	th->x -= vc->x;
	th->y -= vc->y;
	th->z -= vc->z;

	return NULL;
}

ASFUNCTIONBODY(Vector3D,distance)
{
	assert_and_throw(argslen==2);

	Vector3D* vc1=static_cast<Vector3D*>(args[0]);
	Vector3D* vc2=static_cast<Vector3D*>(args[1]);

	number_t dx, dy, dz, dist;
	dx = vc1->x - vc2->x;
	dy = vc1->y - vc2->y;
	dz = vc1->z - vc2->z;
	dist = sqrt(dx * dx + dy * dy + dz * dz);

	return abstract_d(dist);
}

ASFUNCTIONBODY(Vector3D,dotProduct)
{
	assert_and_throw(argslen==1);

	Vector3D* th=static_cast<Vector3D*>(obj);
	Vector3D* vc=static_cast<Vector3D*>(args[0]);

	return abstract_d(th->x * vc->x + th->y * vc->y + th->z * vc->z);
}

ASFUNCTIONBODY(Vector3D,equals)
{
	assert_and_throw(argslen==1 || argslen==2);

	Vector3D* th=static_cast<Vector3D*>(obj);
	Vector3D* vc=static_cast<Vector3D*>(args[0]);
	int32_t allfour = 0;

	if ( argslen == 2 )
	{
		Boolean* af=static_cast<Boolean*>(args[1]);
		allfour = af->toInt();
	}

	return abstract_b(th->x == vc->x &&  th->y == vc->y && th->z == vc->z && allfour ? th->w == vc->w : true);
}

ASFUNCTIONBODY(Vector3D,incrementBy)
{
	assert_and_throw(argslen==1);

	Vector3D* th=static_cast<Vector3D*>(obj);
	Vector3D* vc=static_cast<Vector3D*>(args[0]);

	th->x += vc->x;
	th->y += vc->y;
	th->z += vc->z;

	return NULL;
}

ASFUNCTIONBODY(Vector3D,nearEquals)
{
	assert_and_throw(argslen==2 && argslen==3);

	Vector3D* th=static_cast<Vector3D*>(obj);
	Vector3D* vc=static_cast<Vector3D*>(args[0]);
	number_t tolerance = args[1]->toNumber();
	int32_t allfour = 0;

	if (argslen == 3 )
	{
		Boolean* af=static_cast<Boolean*>(args[2]);
		allfour = af->toInt();
	}

	bool dx, dy, dz, dw;
	dx = (th->x - vc->x) < tolerance;
	dy = (th->y - vc->y) < tolerance;
	dz = (th->z - vc->z) < tolerance;
	dw = allfour ? (th->w - vc->w) < tolerance : true;

	return abstract_b(dx && dy && dz && dw);
}

ASFUNCTIONBODY(Vector3D,negate)
{
	assert_and_throw(argslen==0);

	Vector3D* th=static_cast<Vector3D*>(obj);

	th->x = -th->x;
	th->y = -th->y;
	th->z = -th->z;

	return NULL;
}

ASFUNCTIONBODY(Vector3D,normalize)
{
	assert_and_throw(argslen==0);

	Vector3D* th=static_cast<Vector3D*>(obj);

	number_t len = sqrt(th->x * th->x + th->y * th->y + th->z * th->z);

	th->x /= len;
	th->y /= len;
	th->z /= len;

	return NULL;
}

ASFUNCTIONBODY(Vector3D,project)
{
	assert_and_throw(argslen==0);

	Vector3D* th=static_cast<Vector3D*>(obj);

	th->x /= th->w;
	th->y /= th->w;
	th->z /= th->w;

	return NULL;
}

ASFUNCTIONBODY(Vector3D,scaleBy)
{
	assert_and_throw(argslen==1);

	Vector3D* th=static_cast<Vector3D*>(obj);
	number_t scale = args[0]->toNumber();

	th->x *= scale;
	th->y *= scale;
	th->z *= scale;

	return NULL;
}

ASFUNCTIONBODY(Vector3D,subtract)
{
	assert_and_throw(argslen==1);

	Vector3D* th=static_cast<Vector3D*>(obj);
	Vector3D* vc=static_cast<Vector3D*>(args[0]);
	Vector3D* ret=Class<Vector3D>::getInstanceS();

	ret->x = th->x - vc->x;
	ret->y = th->y - vc->y;
	ret->z = th->z - vc->z;

	return ret;
}
