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

#include "flashmedia.h"
#include "class.h"
#include <iostream>

using namespace lightspark;
using namespace std;

REGISTER_CLASS_NAME(SoundTransform);
REGISTER_CLASS_NAME(Video);

void SoundTransform::sinit(Class_base* c)
{
	assert(c->constructor==NULL);
	c->constructor=new Function(_constructor);
}

ASFUNCTIONBODY(SoundTransform,_constructor)
{
	cout << "SoundTransform constructor" << endl;
	return NULL;
}

void Video::sinit(Class_base* c)
{
	assert(c->constructor==NULL);
	c->constructor=new Function(_constructor);
	c->super=Class<DisplayObject>::getClass();
	c->max_level=c->super->max_level+1;
}

void Video::buildTraits(ASObject* o)
{
	o->setGetterByQName("videoWidth","",new Function(_getVideoWidth));
}

void Video::Render()
{
	cout << "Video::Render not implemented" << endl;
}

ASFUNCTIONBODY(Video,_constructor)
{
	cout << "Video constructor" << endl;
	return NULL;
}

ASFUNCTIONBODY(Video,_getVideoWidth)
{
	return abstract_i(320);
}

