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

#include "flashgeom.h"

using namespace lightspark;

REGISTER_CLASS_NAME(ColorTransform);
REGISTER_CLASS_NAME2(lightspark::Rectangle,"Rectangle");

void Rectangle::sinit(Class_base* c)
{
	assert(c->constructor==NULL);
	c->constructor=new Function(_constructor);
}

ASFUNCTIONBODY(Rectangle,_constructor)
{
	Rectangle* th=static_cast<Rectangle*>(obj->implementation);
	if(args && args->size()!=0)
		abort();
	th->x=0;
	th->y=0;
	th->width=0;
	th->height=0;
	obj->setGetterByQName("left","",new Function(_getLeft));
	obj->setGetterByQName("right","",new Function(_getRight));
	obj->setGetterByQName("top","",new Function(_getTop));
	obj->setGetterByQName("bottom","",new Function(_getBottom));
	return NULL;
}

ASFUNCTIONBODY(Rectangle,_getLeft)
{
	Rectangle* th=static_cast<Rectangle*>(obj->implementation);
	return abstract_d(th->x);
}

ASFUNCTIONBODY(Rectangle,_getRight)
{
	Rectangle* th=static_cast<Rectangle*>(obj->implementation);
	return abstract_d(th->x + th->width);
}

ASFUNCTIONBODY(Rectangle,_getTop)
{
	Rectangle* th=static_cast<Rectangle*>(obj->implementation);
	return abstract_d(th->y);
}

ASFUNCTIONBODY(Rectangle,_getBottom)
{
	Rectangle* th=static_cast<Rectangle*>(obj->implementation);
	return abstract_d(th->y + th->height);
}

void ColorTransform::sinit(Class_base* c)
{
	assert(c->constructor==NULL);
	c->constructor=new Function(_constructor);
}

ASFUNCTIONBODY(ColorTransform,_constructor)
{
	ColorTransform* th=static_cast<ColorTransform*>(obj->implementation);
	if(args->size()!=0)
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
	ColorTransform* th=static_cast<ColorTransform*>(obj->implementation);
	if(args->size()!=1)
		abort();
	uintptr_t tmp=args->at(0)->toInt();
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
