/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2011  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef LSOPENGL_H
#define LSOPENGL_H

#ifdef ENABLE_GLES2
	#define SUPPORT_X11 1 //Needed for SGX/OMAP GL stack
	#include <EGL/egl.h>
	#include <GLES2/gl2.h>
	#include <GLES2/gl2ext.h>
	//Texture formats
	#ifdef GL_EXT_texture_format_BGRA8888
		#define GL_RGBA8 GL_RGBA
		#define GL_BGRA GL_RGBA
	#else
		#error GL_EXT_texture_format_BGRA8888 extension needed
	#endif

	//there are no multiple buffers in GLES 2.0
	#define glDrawBuffer(x)
	#define glBindBuffer(...)
	#define glBufferData(...)
	#define glPixelStorei(...)
#else
	#include <GL/glew.h>
	#ifndef WIN32
		#include <GL/glx.h>
	#endif
#endif

#endif
