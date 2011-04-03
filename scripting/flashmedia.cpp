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

#include "flashmedia.h"
#include "class.h"
#include "compat.h"
#include <iostream>
#include "backends/rendering.h"
#include "backends/input.h"

using namespace lightspark;
using namespace std;

SET_NAMESPACE("flash.media");

REGISTER_CLASS_NAME(SoundTransform);
REGISTER_CLASS_NAME(Video);
REGISTER_CLASS_NAME(Sound);

void SoundTransform::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
}

ASFUNCTIONBODY(SoundTransform,_constructor)
{
	LOG(LOG_CALLS,_("SoundTransform constructor"));
	return NULL;
}

void Video::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->super=Class<DisplayObject>::getClass();
	c->max_level=c->super->max_level+1;
	c->setGetterByQName("videoWidth","",Class<IFunction>::getFunction(_getVideoWidth),true);
	c->setGetterByQName("videoHeight","",Class<IFunction>::getFunction(_getVideoHeight),true);
	c->setGetterByQName("width","",Class<IFunction>::getFunction(Video::_getWidth),true);
	c->setSetterByQName("width","",Class<IFunction>::getFunction(Video::_setWidth),true);
	c->setGetterByQName("height","",Class<IFunction>::getFunction(Video::_getHeight),true);
	c->setSetterByQName("height","",Class<IFunction>::getFunction(Video::_setHeight),true);
	c->setMethodByQName("attachNetStream","",Class<IFunction>::getFunction(attachNetStream),true);
}

void Video::buildTraits(ASObject* o)
{
}

Video::~Video()
{
	sem_destroy(&mutex);
}

void Video::Render(bool maskEnabled)
{
	sem_wait(&mutex);
	if(skipRender(maskEnabled))
	{
		sem_post(&mutex);
		return;
	}
	if(netStream && netStream->lockIfReady())
	{
		//All operations here should be non blocking
		//Get size
		videoWidth=netStream->getVideoWidth();
		videoHeight=netStream->getVideoHeight();

		MatrixApplier ma(getConcatenatedMatrix());

		if(!isSimple())
			rt->glAcquireTempBuffer(0,width,0,height);

		//Enable texture lookup and YUV to RGB conversion
		glColor4f(0,0,0,1);
		//width and height will not change now (the Video mutex is acquired)
		rt->renderTextured(netStream->getTexture(), 0, 0, width, height);

		if(!isSimple())
			rt->glBlitTempBuffer(0,width,0,height);
		
		ma.unapply();
		netStream->unlock();
	}
	sem_post(&mutex);
}

bool Video::getBounds(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const
{
	xmin=0;
	xmax=width;
	ymin=0;
	ymax=height;
	return true;
}

ASFUNCTIONBODY(Video,_constructor)
{
	Video* th=Class<Video>::cast(obj);
	assert_and_throw(argslen<=2);
	if(0 < argslen)
		th->width=args[0]->toInt();
	if(1 < argslen)
		th->height=args[1]->toInt();
	return NULL;
}

ASFUNCTIONBODY(Video,_getVideoWidth)
{
	Video* th=Class<Video>::cast(obj);
	return abstract_i(th->videoWidth);
}

ASFUNCTIONBODY(Video,_getVideoHeight)
{
	Video* th=Class<Video>::cast(obj);
	return abstract_i(th->videoHeight);
}

ASFUNCTIONBODY(Video,_getWidth)
{
	Video* th=Class<Video>::cast(obj);
	return abstract_i(th->width);
}

ASFUNCTIONBODY(Video,_setWidth)
{
	Video* th=Class<Video>::cast(obj);
	assert_and_throw(argslen==1);
	sem_wait(&th->mutex);
	th->width=args[0]->toInt();
	sem_post(&th->mutex);
	return NULL;
}

ASFUNCTIONBODY(Video,_getHeight)
{
	Video* th=Class<Video>::cast(obj);
	return abstract_i(th->height);
}

ASFUNCTIONBODY(Video,_setHeight)
{
	Video* th=Class<Video>::cast(obj);
	assert_and_throw(argslen==1);
	sem_wait(&th->mutex);
	th->height=args[0]->toInt();
	sem_post(&th->mutex);
	return NULL;
}

ASFUNCTIONBODY(Video,attachNetStream)
{
	Video* th=Class<Video>::cast(obj);
	assert_and_throw(argslen==1);
	if(args[0]->getObjectType()==T_NULL) //Drop the connection
	{
		sem_wait(&th->mutex);
		th->netStream=NULL;
		sem_post(&th->mutex);
		return NULL;
	}

	//Validate the parameter
	if(args[0]->getPrototype()!=Class<NetStream>::getClass())
		throw RunTimeException("Type mismatch in Video::attachNetStream");

	//Acquire the netStream
	args[0]->incRef();

	assert_and_throw(th->netStream==NULL);
	sem_wait(&th->mutex);
	th->netStream=Class<NetStream>::cast(args[0]);
	sem_post(&th->mutex);
	return NULL;
}

InteractiveObject* Video::hitTest(InteractiveObject* last, number_t x, number_t y)
{
	assert_and_throw(!sys->getInputThread()->isMaskPresent());
	assert_and_throw(mask==NULL);
	if(x>=0 && x<=width && y>=0 && y<=height)
		return last;
	else
		return NULL;
}

void Sound::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->super=Class<EventDispatcher>::getClass();
	c->max_level=c->super->max_level+1;
}

void Sound::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(Sound,_constructor)
{
	return NULL;
}

