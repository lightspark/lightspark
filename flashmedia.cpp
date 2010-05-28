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

#include "flashmedia.h"
#include "class.h"
#include <iostream>

using namespace lightspark;
using namespace std;

extern TLSDATA SystemState* sys;
extern TLSDATA RenderThread* rt;

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
}

void Video::buildTraits(ASObject* o)
{
	o->setGetterByQName("videoWidth","",Class<IFunction>::getFunction(_getVideoWidth));
	o->setGetterByQName("videoHeight","",Class<IFunction>::getFunction(_getVideoHeight));
	o->setGetterByQName("width","",Class<IFunction>::getFunction(Video::_getWidth));
	o->setSetterByQName("width","",Class<IFunction>::getFunction(Video::_setWidth));
	o->setGetterByQName("height","",Class<IFunction>::getFunction(Video::_getHeight));
	o->setSetterByQName("height","",Class<IFunction>::getFunction(Video::_setHeight));
	o->setVariableByQName("attachNetStream","",Class<IFunction>::getFunction(attachNetStream));
}

void Video::Render()
{
	if(!initialized)
	{
		videoTexture.init(0,0,GL_LINEAR);
		initialized=true;
	}

	if(netStream)
	{
		//Get size
		videoWidth=netStream->getVideoWidth();
		videoHeight=netStream->getVideoHeight();

		float matrix[16];
		getMatrix().get4DMatrix(matrix);
		glPushMatrix();
		glMultMatrixf(matrix);

		rt->glAcquireFramebuffer(0,width,0,height);

		bool frameReady=netStream->copyFrameToTexture(videoTexture);
		videoTexture.bind();
		videoTexture.setTexScale(rt->fragmentTexScaleUniform);

		//Enable texture lookup and YUV to RGB conversion
		if(frameReady)
		{
			glColor4f(0,0,0,1);
			//width and height should not change now
			sem_wait(&mutex);
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
			sem_post(&mutex);
		}

		rt->glBlitFramebuffer(0,width,0,height);
		
		//Render click sensible area if needed
		if(rt->glAcquireIdBuffer())
		{
			glBegin(GL_QUADS);
				glVertex2i(0,0);
				glVertex2i(width,0);
				glVertex2i(width,height);
				glVertex2i(0,height);
			glEnd();
			
		}
		glPopMatrix();
	}
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
	//Validate the parameter
	if(args[0]->getPrototype()!=Class<NetStream>::getClass())
		::abort();

	//Acquire the netStream
	args[0]->incRef();

	assert_and_throw(th->netStream==NULL);
	th->netStream=Class<NetStream>::cast(args[0]);
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
