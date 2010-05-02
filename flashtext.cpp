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

#include "flashtext.h"
#include "class.h"

using namespace std;
using namespace lightspark;

REGISTER_CLASS_NAME2(lightspark::Font,"Font");
REGISTER_CLASS_NAME(TextField);

void lightspark::Font::sinit(Class_base* c)
{
//	c->constructor=new Function(_constructor);
	c->setConstructor(NULL);
	c->setVariableByQName("enumerateFonts","",new Function(enumerateFonts));
}

ASFUNCTIONBODY(lightspark::Font,enumerateFonts)
{
	return Class<Array>::getInstanceS();
}

void TextField::sinit(Class_base* c)
{
	c->setConstructor(NULL);
	c->super=Class<DisplayObject>::getClass();
	c->max_level=c->super->max_level+1;
}

void TextField::buildTraits(ASObject* o)
{
	o->setGetterByQName("width","",new Function(TextField::_getWidth));
	o->setSetterByQName("width","",new Function(TextField::_setWidth));
	o->setGetterByQName("height","",new Function(TextField::_getHeight));
	o->setSetterByQName("height","",new Function(TextField::_setHeight));
}

bool TextField::getBounds(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const
{
	xmin=0;
	xmax=width;
	ymin=0;
	ymax=height;
	return true;
}

ASFUNCTIONBODY(TextField,_getWidth)
{
	TextField* th=Class<TextField>::cast(obj);
	return abstract_i(th->width);
}

ASFUNCTIONBODY(TextField,_setWidth)
{
	TextField* th=Class<TextField>::cast(obj);
	assert(argslen==1);
	th->width=args[0]->toInt();
	return NULL;
}

ASFUNCTIONBODY(TextField,_getHeight)
{
	TextField* th=Class<TextField>::cast(obj);
	return abstract_i(th->height);
}

ASFUNCTIONBODY(TextField,_setHeight)
{
	TextField* th=Class<TextField>::cast(obj);
	assert(argslen==1);
	th->height=args[0]->toInt();
	return NULL;
}

void TextField::Render()
{
	//TODO: implement
	LOG(LOG_NOT_IMPLEMENTED,"TextField::Render");
}
