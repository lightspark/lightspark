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

extern TLSDATA SystemState* sys;
extern TLSDATA RenderThread* rt;

REGISTER_CLASS_NAME(SoundTransform);
REGISTER_CLASS_NAME(Video);

void SoundTransform::sinit(Class_base* c)
{
	assert(c->constructor==NULL);
	c->constructor=new Function(_constructor);
}

ASFUNCTIONBODY(SoundTransform,_constructor)
{
	LOG(LOG_CALLS,"SoundTransform constructor");
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
	o->setGetterByQName("width","",new Function(Video::_getWidth));
	o->setSetterByQName("width","",new Function(Video::_setWidth));
	o->setGetterByQName("height","",new Function(Video::_getHeight));
	o->setSetterByQName("height","",new Function(Video::_setHeight));
	o->setVariableByQName("attachNetStream","",new Function(attachNetStream));
}

void Video::Render()
{
	if(!initialized)
	{
		initialized=true;
		glGenTextures(1,&videoTexture);
		glGenBuffers(2,videoBuffers);

		//Per texture initialization
		glBindTexture(GL_TEXTURE_2D, videoTexture);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
		
		glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );
	}

	if(netStream)
	{
		//Get size
		uint32_t newWidth=netStream->getVideoWidth();
		uint32_t newHeight=netStream->getVideoHeight();

		if(videoWidth==0 || videoHeight==0)
		{
			videoWidth=newWidth;
			videoHeight=newHeight;
		}
		else
		{
			if(videoWidth!=newWidth || videoHeight!=newHeight)
				abort();
		}

		//If size is not yet valid
		if(videoWidth==0 || videoHeight==0)
			return;

		assert(videoWidth==480);
		//Size is validated and has not changed

		//Increment and wrap current buffer index
		unsigned int nextBuffer = (curBuffer + 1)%2;
		rt->glAcquireFramebuffer();

		float matrix[16];
		Matrix.get4DMatrix(matrix);
		glPushMatrix();
		glMultMatrixf(matrix);

		glBindTexture(GL_TEXTURE_2D, videoTexture);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, videoBuffers[curBuffer]);

		//Copy content of the pbo to the texture, 0 is the offset in the pbo
//		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, videoWidth, videoHeight, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, 0); 
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, videoWidth, videoHeight, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, 0); 
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, videoBuffers[nextBuffer]);
//		glBufferData(GL_PIXEL_UNPACK_BUFFER, videoWidth*videoHeight*4, 0, GL_STREAM_DRAW);
		glBufferData(GL_PIXEL_UNPACK_BUFFER, videoWidth*videoHeight, 0, GL_STREAM_DRAW);

		while(glGetError());
		uint8_t* buffer = (uint8_t*)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
		if(buffer==NULL)
		{
			cout << glGetError() << endl;
			abort();
		}

		netStream->copyBuffer(buffer);

		glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

		//Enable texture lookup
		glColor3f(0,0,1);

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

		rt->glBlitFramebuffer();
		glPopMatrix();

		curBuffer = nextBuffer;
	}
/*	else
	{

		for(unsigned int i=0;i<(videoWidth*videoHeight);i++)
		{
			buffer[i]=0xff000000+(i&0xff);
		}
	}*/

}

ASFUNCTIONBODY(Video,_constructor)
{
	Video* th=Class<Video>::cast(obj->implementation);
	assert(argslen<2);
	if(0 < argslen)
		th->width=args[0]->toInt();
	if(1 < argslen)
		th->height=args[1]->toInt();
	return NULL;
}

ASFUNCTIONBODY(Video,_getVideoWidth)
{
	Video* th=Class<Video>::cast(obj->implementation);
	return abstract_i(th->videoWidth);
}

ASFUNCTIONBODY(Video,_getVideoHeight)
{
	Video* th=Class<Video>::cast(obj->implementation);
	return abstract_i(th->videoHeight);
}

ASFUNCTIONBODY(Video,_getWidth)
{
	Video* th=Class<Video>::cast(obj->implementation);
	return abstract_i(th->width);
}

ASFUNCTIONBODY(Video,_setWidth)
{
	Video* th=Class<Video>::cast(obj->implementation);
	assert(argslen==1);
	sem_wait(&th->mutex);
	th->width=args[0]->toInt();
	sem_post(&th->mutex);
	return NULL;
}

ASFUNCTIONBODY(Video,_getHeight)
{
	Video* th=Class<Video>::cast(obj->implementation);
	return abstract_i(th->height);
}

ASFUNCTIONBODY(Video,_setHeight)
{
	Video* th=Class<Video>::cast(obj->implementation);
	assert(argslen==1);
	sem_wait(&th->mutex);
	th->height=args[0]->toInt();
	sem_post(&th->mutex);
	return NULL;
}

ASFUNCTIONBODY(Video,attachNetStream)
{
	Video* th=Class<Video>::cast(obj->implementation);
	assert(argslen==1);
	//Validate the parameter
	if(args[0]->prototype!=Class<NetStream>::getClass())
		abort();

	//Acquire the netStream
	args[0]->incRef();

	th->netStream=Class<NetStream>::cast(args[0]->implementation);
	return NULL;
}
