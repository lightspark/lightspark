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
	c->setConstructor(new Function(_constructor));
}

void Rectangle::buildTraits(ASObject* o)
{
	IFunction* left=new Function(_getLeft);
	o->setGetterByQName("left","",left);
	left->incRef();
	o->setGetterByQName("x","",left);
	o->setGetterByQName("right","",new Function(_getRight));
	o->setGetterByQName("width","",new Function(_getWidth));
	o->setSetterByQName("width","",new Function(_setWidth));

	IFunction* top=new Function(_getTop);
	o->setGetterByQName("top","",top);
	top->incRef();
	o->setGetterByQName("y","",top);

	o->setGetterByQName("bottom","",new Function(_getBottom));
	o->setGetterByQName("height","",new Function(_getHeight));
	o->setSetterByQName("height","",new Function(_setHeight));

	o->setVariableByQName("clone","",new Function(clone));
}

const RECT Rectangle::getRect() const
{
	return RECT(x,y,x+width,y+height);
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

ASFUNCTIONBODY(Rectangle,_getRight)
{
	Rectangle* th=static_cast<Rectangle*>(obj);
	return abstract_d(th->x + th->width);
}

ASFUNCTIONBODY(Rectangle,_getWidth)
{
	Rectangle* th=static_cast<Rectangle*>(obj);
	return abstract_d(th->width);
}

ASFUNCTIONBODY(Rectangle,_setWidth)
{
	Rectangle* th=static_cast<Rectangle*>(obj);
	assert(argslen==1);
	th->width=args[0]->toNumber();
	return NULL;
}

ASFUNCTIONBODY(Rectangle,_getTop)
{
	Rectangle* th=static_cast<Rectangle*>(obj);
	return abstract_d(th->y);
}

ASFUNCTIONBODY(Rectangle,_getBottom)
{
	Rectangle* th=static_cast<Rectangle*>(obj);
	return abstract_d(th->y + th->height);
}

ASFUNCTIONBODY(Rectangle,_getHeight)
{
	Rectangle* th=static_cast<Rectangle*>(obj);
	return abstract_d(th->height);
}

ASFUNCTIONBODY(Rectangle,_setHeight)
{
	Rectangle* th=static_cast<Rectangle*>(obj);
	assert(argslen==1);
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
	c->setConstructor(new Function(_constructor));
}

ASFUNCTIONBODY(ColorTransform,_constructor)
{
	ColorTransform* th=static_cast<ColorTransform*>(obj);
	if(argslen!=0)
		abort();
	//Setting multiplier to default
	th->redMultiplier=1.0;
	th->greenMultiplier=1.0;
	th->blueMultiplier=1.0;
	th->alphaMultiplier=1.0;
	//Setting offset to the input value
	th->redOffset=0.0;
	th->greenOffset=0.0;
	th->blueOffset=0.0;
	th->alphaOffset=0.0;
	return NULL;
}

ASFUNCTIONBODY(ColorTransform,setColor)
{
	ColorTransform* th=static_cast<ColorTransform*>(obj);
	if(argslen!=1)
		abort();
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
	abort();
	return NULL;
}

void Point::sinit(Class_base* c)
{
	c->setConstructor(new Function(_constructor));
}

void Point::buildTraits(ASObject* o)
{
	o->setGetterByQName("x","",new Function(_getX));
	o->setGetterByQName("y","",new Function(_getY));
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
	//c->constructor=new Function(_constructor);
	c->setConstructor(NULL);
}

void Transform::buildTraits(ASObject* o)
{
	o->setSetterByQName("colorTransform","",new Function(undefinedFunction));
}

