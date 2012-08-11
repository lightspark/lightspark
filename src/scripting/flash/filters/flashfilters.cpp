/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2012  Alessandro Pignotti (a.pignotti@sssup.it)

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

#include "scripting/flash/filters/flashfilters.h"
#include "scripting/class.h"

using namespace std;
using namespace lightspark;

void BitmapFilter::sinit(Class_base* c)
{
	c->setConstructor(NULL);
	c->setSuper(Class<ASObject>::getRef());
	c->setDeclaredMethodByQName("clone","",Class<IFunction>::getFunction(clone),NORMAL_METHOD,true);
}

BitmapFilter* BitmapFilter::cloneImpl() const
{
	return Class<BitmapFilter>::getInstanceS();
}

ASFUNCTIONBODY(BitmapFilter,clone)
{
	BitmapFilter* th=static_cast<BitmapFilter*>(obj);
	return th->cloneImpl();
}

void GlowFilter::sinit(Class_base* c)
{
	c->setConstructor(NULL);
	c->setSuper(Class<BitmapFilter>::getRef());
}

GlowFilter* GlowFilter::cloneImpl() const
{
	return Class<GlowFilter>::getInstanceS();
}

void DropShadowFilter::sinit(Class_base* c)
{
	c->setConstructor(NULL);
	c->setSuper(Class<BitmapFilter>::getRef());
}

DropShadowFilter* DropShadowFilter::cloneImpl() const
{
	return Class<DropShadowFilter>::getInstanceS();
}
