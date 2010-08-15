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
#include <iostream>
#include "backends/rendering.h"

using namespace lightspark;
using namespace std;

extern TLSDATA SystemState* sys;
extern TLSDATA RenderThread* rt;

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
	LOG(LOG_CALLS,"SoundTransform constructor");
	return NULL;
}

void Video::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->super=Class<DisplayObject>::getClass();
	c->max_level=c->super->max_level+1;
	c->setGetterByQName("videoWidth","",Class<IFunction>::getFunction(_getVideoWidth));
	c->setGetterByQName("videoHeight","",Class<IFunction>::getFunction(_getVideoHeight));
	c->setGetterByQName("width","",Class<IFunction>::getFunction(Video::_getWidth));
	c->setSetterByQName("width","",Class<IFunction>::getFunction(Video::_setWidth));
	c->setGetterByQName("height","",Class<IFunction>::getFunction(Video::_getHeight));
	c->setSetterByQName("height","",Class<IFunction>::getFunction(Video::_setHeight));
	c->setVariableByQName("attachNetStream","",Class<IFunction>::getFunction(attachNetStream));
}

void Video::buildTraits(ASObject* o)
{
}

Video::~Video()
{
	if(rt)
	{
		rt->acquireResourceMutex();
		rt->removeResource(&videoTexture);
	}
	videoTexture.shutdown();
	if(rt)
	{
		rt->releaseResourceMutex();
		sem_destroy(&mutex);
	}
}

void Video::inputRender()
{
	sem_wait(&mutex);
	if(netStream && netStream->lockIfReady())
	{
		//All operations here should be non blocking
		//Get size
		videoWidth=netStream->getVideoWidth();
		videoHeight=netStream->getVideoHeight();

		MatrixApplier ma(getMatrix());

		glBegin(GL_QUADS);
			glVertex2i(0,0);
			glVertex2i(width,0);
			glVertex2i(width,height);
			glVertex2i(0,height);
		glEnd();
		ma.unapply();
		netStream->unlock();
	}
	sem_post(&mutex);
}

void Video::Render()
{
	if(!initialized)
	{
		videoTexture.init(0,0,GL_LINEAR);
		rt->addResource(&videoTexture);
		initialized=true;
	}

	sem_wait(&mutex);
	if(netStream && netStream->lockIfReady())
	{
		//All operations here should be non blocking
		//Get size
		videoWidth=netStream->getVideoWidth();
		videoHeight=netStream->getVideoHeight();

		MatrixApplier ma(getMatrix());

		if(!isSimple())
			rt->glAcquireTempBuffer(0,width,0,height);

		bool frameReady=netStream->copyFrameToTexture(videoTexture);
		videoTexture.bind();
		videoTexture.setTexScale(rt->fragmentTexScaleUniform);

		//Enable texture lookup and YUV to RGB conversion
		if(frameReady)
		{
			glColor4f(0,0,0,1);
			//width and height should not change now
			glBegin(GL_QUADS);
				glTexCoord2f(0,0);
				glVertex2i(0,0);

				glTexCoord2f(1,0);
				glVertex2i(width,0);

				glTexCoord2f(1,1);
				glVertex2i(width,height);

				glTexCoord2f(0,1);
				glVertex2i(0,height);
			glEnd();
		}

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
	ymin=height;
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
