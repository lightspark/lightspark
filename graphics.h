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

#ifndef _GRAPHICS_H
#define _GRAPHICS_H

#include <GL/glew.h>
#include "compat.h"

namespace lightspark
{

void cleanGLErrors();

class TextureBuffer
{
private:
	GLuint texId;
	GLenum filtering;
	uint32_t allocWidth;
	uint32_t allocHeight;
	uint32_t width;
	uint32_t height;
	bool inited;
	uint32_t nearestPOT(uint32_t a) const;
	void setAllocSize(uint32_t w, uint32_t h);
public:
	TextureBuffer(bool initNow, uint32_t width=0, uint32_t height=0, GLenum filtering=GL_NEAREST);
	~TextureBuffer();
	GLuint getId() {return texId;}
	void init();
	void init(uint32_t width, uint32_t height, GLenum filtering=GL_NEAREST);
	void bind();
	void unbind();
	void setTexScale(GLuint uniformLocation);
	void setBGRAData(uint8_t* bgraData, uint32_t w, uint32_t h);
	void resize(uint32_t width, uint32_t height);
	uint32_t getAllocWidth() const { return allocWidth;}
	uint32_t getAllocHeight() const { return allocHeight;}
};

};
#endif
