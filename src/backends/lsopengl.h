/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2011-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef BACKENDS_LSOPENGL_H
#define BACKENDS_LSOPENGL_H 1

#ifdef ENABLE_GLES2
	#include <EGL/egl.h>
	#include <GLES2/gl2.h>
	#include <GLES2/gl2ext.h>
	//Texture formats
	#ifdef GL_EXT_texture_format_BGRA8888
		#define GL_RGBA8 GL_RGBA
		#define GL_BGRA GL_BGRA_EXT
	#else
		#error GL_EXT_texture_format_BGRA8888 extension needed
	#endif

	//there are no multiple buffers in GLES 2.0
	#define glDrawBuffer(x) {}
//	#define glBindBuffer(...)
//	#define glBufferData(...)
	#define glPixelStorei(...)
#elif defined(ENABLE_GLES3)
	#include "SDL_opengles2.h"
	#include <GLES3/gl3.h>
	#include <GLES3/gl3ext.h>
	// No GL_EXT_texture_format_BGRA8888 on GLES3, fun

	//there are no multiple buffers in GLES 3.0... I think
	#define glDrawBuffer(x) {}
#else
#include "3rdparty/opengl/glcorearb.h"

// add some older constants not defined in official api
#define GL_GENERATE_MIPMAP 0x8191
#define GL_COLOR_ARRAY 0x8076
#define GL_COLOR_MATERIAL 0x0B57
#define GL_ENABLE_BIT 0x00002000
#define GL_LIGHTING 0x0B50
#define GL_LUMINANCE 0x1909
#define GL_MODELVIEW 0x1700
#define GL_MODULATE 0x2100
#define GL_NORMAL_ARRAY 0x8075
#define GL_PROJECTION 0x1701
#define GL_SHADE_MODEL 0x0B54
#define GL_SMOOTH 0x1D01
#define GL_TEXTURE_COORD_ARRAY 0x8078
#define GL_TEXTURE_ENV 0x2300
#define GL_TEXTURE_ENV_MODE 0x2200
#define GL_TRANSFORM_BIT 0x00001000



	extern PFNGLACTIVETEXTUREPROC glActiveTexture;
	extern PFNGLATTACHSHADERPROC glAttachShader;
	extern PFNGLBINDATTRIBLOCATIONPROC glBindAttribLocation;
	extern PFNGLBINDBUFFERPROC glBindBuffer;
	extern PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer;
	extern PFNGLBINDRENDERBUFFERPROC glBindRenderbuffer;
	extern PFNGLBINDTEXTUREPROC glBindTexture;
	extern PFNGLBLENDFUNCPROC glBlendFunc;
	extern PFNGLBLENDFUNCSEPARATEPROC glBlendFuncSeparate;
	extern PFNGLBUFFERDATAPROC glBufferData;
	extern PFNGLBUFFERSUBDATAPROC glBufferSubData;
	extern PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus;
	extern PFNGLCLEARPROC glClear;
	extern PFNGLCLEARCOLORPROC glClearColor;
	extern PFNGLCLEARDEPTHPROC glClearDepth;
	extern PFNGLCLEARSTENCILPROC glClearStencil;
	extern PFNGLCOLORMASKPROC glColorMask;
	extern PFNGLCOMPILESHADERPROC glCompileShader;
	extern PFNGLCOMPRESSEDTEXIMAGE2DPROC glCompressedTexImage2D;
	extern PFNGLCREATEPROGRAMPROC glCreateProgram;
	extern PFNGLCREATESHADERPROC glCreateShader;
	extern PFNGLCULLFACEPROC glCullFace;
	extern PFNGLDELETEBUFFERSPROC glDeleteBuffers;
	extern PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers;
	extern PFNGLDELETEPROGRAMPROC glDeleteProgram;
	extern PFNGLDELETERENDERBUFFERSPROC glDeleteRenderbuffers;
	extern PFNGLDELETESHADERPROC glDeleteShader;
	extern PFNGLDELETETEXTURESPROC glDeleteTextures;
	extern PFNGLDEPTHFUNCPROC glDepthFunc;
	extern PFNGLDEPTHMASKPROC glDepthMask;
	extern PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray;
	extern PFNGLDISABLEPROC glDisable;
	extern PFNGLDRAWARRAYSPROC glDrawArrays;
	extern PFNGLDRAWBUFFERPROC glDrawBuffer;
	extern PFNGLDRAWELEMENTSPROC glDrawElements;
	extern PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
	extern PFNGLENABLEPROC glEnable;
	extern PFNGLFINISHPROC glFinish;
	extern PFNGLFLUSHPROC glFlush;
	extern PFNGLFRAMEBUFFERRENDERBUFFERPROC glFramebufferRenderbuffer;
	extern PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D;
	extern PFNGLFRONTFACEPROC glFrontFace;
	extern PFNGLGENBUFFERSPROC glGenBuffers;
	extern PFNGLGENERATEMIPMAPPROC glGenerateMipmap;
	extern PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers;
	extern PFNGLGENRENDERBUFFERSPROC glGenRenderbuffers;
	extern PFNGLGENTEXTURESPROC glGenTextures;
	extern PFNGLGETATTRIBLOCATIONPROC glGetAttribLocation;
	extern PFNGLGETERRORPROC glGetError;
	extern PFNGLGETINTEGERVPROC glGetIntegerv;
	extern PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
	extern PFNGLGETPROGRAMIVPROC glGetProgramiv;
	extern PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
	extern PFNGLGETSHADERIVPROC glGetShaderiv;
	extern PFNGLGETSTRINGPROC glGetString;
	extern PFNGLGETTEXIMAGEPROC glGetTexImage;
	extern PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
	extern PFNGLLINKPROGRAMPROC glLinkProgram;
	extern PFNGLPIXELSTOREIPROC glPixelStorei;
	extern PFNGLPOLYGONMODEPROC glPolygonMode;
	extern PFNGLREADPIXELSPROC glReadPixels;
	extern PFNGLRENDERBUFFERSTORAGEPROC glRenderbufferStorage;
	extern PFNGLSCISSORPROC glScissor;
	extern PFNGLSHADERSOURCEPROC glShaderSource;
	extern PFNGLSTENCILFUNCPROC glStencilFunc;
	extern PFNGLSTENCILMASKPROC glStencilMask;
	extern PFNGLSTENCILOPPROC glStencilOp;
	extern PFNGLSTENCILOPSEPARATEPROC glStencilOpSeparate;
	extern PFNGLTEXIMAGE2DPROC glTexImage2D;
	extern PFNGLTEXPARAMETERFPROC glTexParameterf;
	extern PFNGLTEXPARAMETERIPROC glTexParameteri;
	extern PFNGLTEXSUBIMAGE2DPROC glTexSubImage2D;
	extern PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;
	extern PFNGLVIEWPORTPROC glViewport;
	extern PFNGLUNIFORM1IPROC glUniform1i;
	extern PFNGLUNIFORM1FPROC glUniform1f;
	extern PFNGLUNIFORM1FVPROC glUniform1fv;
	extern PFNGLUNIFORM2FPROC glUniform2f;
	extern PFNGLUNIFORM2FVPROC glUniform2fv;
	extern PFNGLUNIFORM4FPROC glUniform4f;
	extern PFNGLUNIFORM4FVPROC glUniform4fv;
	extern PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv;
	extern PFNGLUSEPROGRAMPROC glUseProgram;

	//additional functions needed by ImGUI
	typedef void (*PFNGLCOLORPOINTERPROC)(GLint size,GLenum type,GLsizei stride,const void * pointer);
	typedef void (*PFNGLDISABLECLIENTSTATEPROC)(GLenum cap);
	typedef void (*PFNGLENABLECLIENTSTATEPROC)(GLenum cap);
	typedef void (*PFNGLGETTEXENVIVPROC)(GLenum target,GLenum pname,GLint * params);
	typedef void (*PFNGLLOADIDENTITYPROC)();
	typedef void (*PFNGLMATRIXMODEPROC)(GLenum mode);
	typedef void (*PFNGLORTHOPROC)(GLdouble left,GLdouble right,GLdouble bottom,GLdouble top,GLdouble nearVal,GLdouble farVal);
	typedef void (*PFNGLPOPATTRIBPROC)();
	typedef void (*PFNGLPOPMATRIXPROC)();
	typedef void (*PFNGLPUSHATTRIBPROC)(GLbitfield mask);
	typedef void (*PFNGLPUSHMATRIXPROC)();
	typedef void (*PFNGLSHADEMODELPROC)(GLenum mode);
	typedef void (*PFNGLTEXCOORDPOINTERPROC)(GLint size,GLenum type,GLsizei stride,const void * pointer);
	typedef void (*PFNGLTEXENVIPROC)(GLenum target,GLenum pname,GLint param);
	typedef void (*PFNGLVERTEXPOINTERPROC)(GLint size,GLenum type,GLsizei stride,const void * pointer);

	extern PFNGLCOLORPOINTERPROC glColorPointer;
	extern PFNGLDISABLECLIENTSTATEPROC glDisableClientState;
	extern PFNGLENABLECLIENTSTATEPROC glEnableClientState;
	extern PFNGLGETTEXENVIVPROC glGetTexEnviv;
	extern PFNGLLOADIDENTITYPROC glLoadIdentity;
	extern PFNGLMATRIXMODEPROC glMatrixMode;
	extern PFNGLORTHOPROC glOrtho;
	extern PFNGLPOPATTRIBPROC glPopAttrib;
	extern PFNGLPOPMATRIXPROC glPopMatrix;
	extern PFNGLPUSHATTRIBPROC glPushAttrib;
	extern PFNGLPUSHMATRIXPROC glPushMatrix;
	extern PFNGLSHADEMODELPROC glShadeModel;
	extern PFNGLTEXCOORDPOINTERPROC glTexCoordPointer;
	extern PFNGLTEXENVIPROC glTexEnvi;
	extern PFNGLVERTEXPOINTERPROC glVertexPointer;
#endif

#endif /* BACKENDS_LSOPENGL_H */
