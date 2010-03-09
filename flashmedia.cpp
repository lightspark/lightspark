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
}

void Video::Render()
{
	if(!initialized)
	{
		glGenTextures(1,&videoTexture);
		glGenBuffers(1,&videoBuffer);
		initialized=true;

		//Per texture initialization
		glBindTexture(GL_TEXTURE_2D, videoTexture);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
		
		glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );
	}

	LOG(LOG_NOT_IMPLEMENTED,"Video::Render not implemented");
	rt->glAcquireFramebuffer();

	float matrix[16];
	Matrix.get4DMatrix(matrix);
	glPushMatrix();
	glMultMatrixf(matrix);

	glBindTexture(GL_TEXTURE_2D, videoTexture);
	uint32_t* buffer=new uint32_t[width*height];
	for(int i=0;i<(width*height);i++)
	{
		buffer[i]=0xff000000+(i&0xff);
	}
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, buffer);

	//Enable texture lookup
	glColor3f(0,0,1);

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

	rt->glBlitFramebuffer();
	glPopMatrix();
	delete[] buffer;
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
	return abstract_i(320);
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
	th->width=args[0]->toInt();
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
	th->height=args[0]->toInt();
	return NULL;
}

