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

#include "flashtext.h"
#include "class.h"

using namespace std;
using namespace lightspark;

REGISTER_CLASS_NAME2(lightspark::Font,"Font");
REGISTER_CLASS_NAME(TextField);

void lightspark::Font::sinit(Class_base* c)
{
//	c->constructor=Class<IFunction>::getFunction(_constructor);
	c->setConstructor(NULL);
	c->setVariableByQName("enumerateFonts","",Class<IFunction>::getFunction(enumerateFonts));
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
	c->setGetterByQName("width","",Class<IFunction>::getFunction(TextField::_getWidth));
	c->setSetterByQName("width","",Class<IFunction>::getFunction(TextField::_setWidth));
	c->setGetterByQName("height","",Class<IFunction>::getFunction(TextField::_getHeight));
	c->setSetterByQName("height","",Class<IFunction>::getFunction(TextField::_setHeight));
}

void TextField::buildTraits(ASObject* o)
{
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
	assert_and_throw(argslen==1);
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
	assert_and_throw(argslen==1);
	th->height=args[0]->toInt();
	return NULL;
}

void TextField::Render()
{
	//TODO: implement
	LOG(LOG_NOT_IMPLEMENTED,"TextField::Render");
}
