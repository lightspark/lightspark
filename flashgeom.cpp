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

REGISTER_CLASS_NAME(Transform);
REGISTER_CLASS_NAME(ColorTransform);
REGISTER_CLASS_NAME(Point);
REGISTER_CLASS_NAME2(lightspark::Rectangle,"Rectangle");

void Rectangle::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
}

void Rectangle::buildTraits(ASObject* o)
{
	IFunction* gleft=Class<IFunction>::getFunction(_getLeft);
	o->setGetterByQName("left","",gleft);
	gleft->incRef();
	o->setGetterByQName("x","",gleft);
	IFunction* sleft=Class<IFunction>::getFunction(_setLeft);
	o->setSetterByQName("left","",sleft);
	sleft->incRef();
	o->setSetterByQName("x","",sleft);
	o->setGetterByQName("right","",Class<IFunction>::getFunction(_getRight));
	o->setSetterByQName("right","",Class<IFunction>::getFunction(_setRight));
	o->setGetterByQName("width","",Class<IFunction>::getFunction(_getWidth));
	o->setSetterByQName("width","",Class<IFunction>::getFunction(_setWidth));

	IFunction* gtop=Class<IFunction>::getFunction(_getTop);
	o->setGetterByQName("top","",gtop);
	gtop->incRef();
	o->setGetterByQName("y","",gtop);
	IFunction* stop=Class<IFunction>::getFunction(_setTop);
	o->setSetterByQName("top","",stop);
	stop->incRef();
	o->setSetterByQName("y","",stop);

	o->setGetterByQName("bottom","",Class<IFunction>::getFunction(_getBottom));
	o->setSetterByQName("bottom","",Class<IFunction>::getFunction(_setBottom));
	o->setGetterByQName("height","",Class<IFunction>::getFunction(_getHeight));
	o->setSetterByQName("height","",Class<IFunction>::getFunction(_setHeight));

	o->setVariableByQName("clone","",Class<IFunction>::getFunction(clone));
}

const lightspark::RECT Rectangle::getRect() const
{
	return lightspark::RECT(x,y,x+width,y+height);
}

ASFUNCTIONBODY(Rectangle,_constructor)
{
	Rectangle* th=static_cast<Rectangle*>(obj);

	if(argslen>=1)
		th->x=args[0]->toInt();
	if(argslen>=2)
		th->y=args[1]->toInt();
	if(argslen>=3)
		th->width=args[2]->toInt();
	if(argslen>=4)
		th->height=args[3]->toInt();

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
}

void Point::buildTraits(ASObject* o)
{
	o->setGetterByQName("x","",Class<IFunction>::getFunction(_getX));
	o->setGetterByQName("y","",Class<IFunction>::getFunction(_getY));
}

ASFUNCTIONBODY(Point,_constructor)
{
	Point* th=static_cast<Point*>(obj);
	if(argslen>=1)
		th->x=args[0]->toInt();
	if(argslen>=2)
		th->y=args[1]->toInt();

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

void Transform::sinit(Class_base* c)
{
	//c->constructor=Class<IFunction>::getFunction(_constructor);
	c->setConstructor(NULL);
}

void Transform::buildTraits(ASObject* o)
{
	o->setSetterByQName("colorTransform","",Class<IFunction>::getFunction(undefinedFunction));
}
