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

#ifndef PLATFORMS_ENGINEUTILS_H
#define PLATFORMS_ENGINEUTILS_H 1

#include <SDL2/SDL.h>
// on ppc SDL.h includes altivec.h, so we have to undefine vector
#undef vector
#include "compat.h"
#include "threading.h"
#include "tiny_string.h"
#include "backends/graphics.h"
#include "flash/utils/ByteArray.h"

#define LIGHTSPARK_AUDIO_BUFFERSIZE 8192

namespace lightspark
{

#ifndef _WIN32
//taken from X11/X.h
typedef unsigned long VisualID;
typedef unsigned long XID;
#endif
#define LS_USEREVENT_INIT EngineData::userevent
#define LS_USEREVENT_EXEC EngineData::userevent+1
#define LS_USEREVENT_QUIT EngineData::userevent+2
class SystemState;
class StreamCache;
class AudioStream;

enum DEPTH_FUNCTION { ALWAYS, EQUAL, GREATER, GREATER_EQUAL, LESS, LESS_EQUAL, NEVER, NOT_EQUAL };
enum TRIANGLE_FACE { FACE_BACK, FACE_FRONT, FACE_FRONT_AND_BACK, FACE_NONE };
enum BLEND_FACTOR { BLEND_ONE,BLEND_ZERO,BLEND_SRC_ALPHA,BLEND_SRC_COLOR,BLEND_DST_ALPHA,BLEND_DST_COLOR,BLEND_ONE_MINUS_SRC_ALPHA,BLEND_ONE_MINUS_SRC_COLOR,BLEND_ONE_MINUS_DST_ALPHA,BLEND_ONE_MINUS_DST_COLOR };
enum VERTEXBUFFER_FORMAT { BYTES_4, FLOAT_1, FLOAT_2, FLOAT_3, FLOAT_4 };
enum CLEARMASK { COLOR = 0x1, DEPTH = 0x2, STENCIL = 0x4 };

// this is only used for font rendering in PPAPI plugin
class externalFontRenderer : public IDrawable
{
	int32_t externalressource;
	class EngineData* m_engine;
public:
	externalFontRenderer(const TextData &_textData,class EngineData* engine, int32_t w, int32_t h, int32_t x, int32_t y, float a, const std::vector<MaskData>& m, bool smoothing);
	
	uint8_t* getPixelBuffer() override;
	void applyCairoMask(cairo_t* cr, int32_t offsetX, int32_t offsetY) const override {}
};

class DLL_PUBLIC EngineData
{
	friend class RenderThread;
protected:
	/* use a recursive mutex, because g_signal_connect may directly call
	 * the specific handler */
	RecMutex mutex;
	virtual SDL_Window* createWidget(uint32_t w,uint32_t h)=0;
public:
	uint32_t pixelBuffers[2];
	uint32_t currentPixelBuffer;
	intptr_t currentPixelBufferOffset;
	uint8_t* currentPixelBufPtr;
	uint32_t pixelBufferWidth;
	uint32_t pixelBufferHeight;
	SDL_Window* widget;
	static uint32_t userevent;
	static Thread* mainLoopThread;
	uint32_t width;
	uint32_t height;
	uint32_t origwidth;
	uint32_t origheight;
	bool needrenderthread;
	bool supportPackedDepthStencil;
	bool hasExternalFontRenderer;
	tiny_string driverInfoString;
	EngineData();
	virtual ~EngineData();
	virtual bool isSizable() const = 0;
	virtual void stopMainDownload() = 0;
	/* you may not call getWindowForGnash and showWindow on the same EngineData! */
	virtual uint32_t getWindowForGnash()=0;
	/* Runs 'func' in the mainLoopThread */
	virtual void runInMainThread(SystemState* sys, void (*func) (SystemState*) )
	{
		SDL_Event event;
		SDL_zero(event);
		event.type = LS_USEREVENT_EXEC;
		event.user.data1 = (void*)func;
		SDL_PushEvent(&event);
	}
	static bool mainloop_handleevent(SDL_Event* event,SystemState* sys);
	static gboolean mainloop_from_plugin(SystemState* sys);
	
	/* This function must be called from mainLoopThread
	 * It fills this->widget and this->window.
	 */
	void showWindow(uint32_t w, uint32_t h);
	/* must be called within mainLoopThread */
	virtual void grabFocus()=0;
	virtual void openPageInBrowser(const tiny_string& url, const tiny_string& window)=0;

	static bool sdl_needinit;
	static bool enablerendering;
	static bool mainthread_running;
	static Semaphore mainthread_initialized;
	static bool startSDLMain();

	virtual bool FileExists(const tiny_string& filename);
	virtual tiny_string FileRead(const tiny_string& filename);
	virtual void FileWrite(const tiny_string& filename, const tiny_string& data);
	virtual void FileReadByteArray(const tiny_string &filename,ByteArray* res);
	virtual void FileWriteByteArray(const tiny_string& filename, ByteArray* data);
	void initGLEW();
	void resizePixelBuffers(uint32_t w, uint32_t h);
	void bindCurrentBuffer();
	
	/* show/hide mouse cursor, must be called from mainLoopThread */
	static void showMouseCursor(SystemState *sys);
	static void hideMouseCursor(SystemState *sys);
	virtual void setClipboardText(const std::string txt);
	virtual bool getScreenData(SDL_DisplayMode* screen) = 0;
	virtual double getScreenDPI() = 0;
	virtual StreamCache* createFileStreamCache(SystemState *sys);
	
	// OpenGL methods
	virtual void DoSwapBuffers() = 0;
	virtual void InitOpenGL() = 0;
	virtual void DeinitOpenGL() = 0;
	virtual bool getGLError(uint32_t& errorCode) const;
	virtual uint8_t* getCurrentPixBuf() const;
	virtual uint8_t* switchCurrentPixBuf(uint32_t w, uint32_t h);
	virtual tiny_string getGLDriverInfo();
	virtual void exec_glUniform1f(int location,float v0);
	virtual void exec_glBindTexture_GL_TEXTURE_2D(uint32_t id);
	virtual void exec_glVertexAttribPointer(uint32_t index, int32_t stride, const void* coords, VERTEXBUFFER_FORMAT format);
	virtual void exec_glEnableVertexAttribArray(uint32_t index);
	virtual void exec_glDrawArrays_GL_TRIANGLES(int32_t first, int32_t count);
	virtual void exec_glDrawElements_GL_TRIANGLES_GL_UNSIGNED_SHORT(int32_t count,const void* indices);
	virtual void exec_glDrawArrays_GL_LINE_STRIP(int32_t first, int32_t count);
	virtual void exec_glDrawArrays_GL_TRIANGLE_STRIP(int32_t first, int32_t count);
	virtual void exec_glDrawArrays_GL_LINES(int32_t first, int32_t count);
	virtual void exec_glDisableVertexAttribArray(uint32_t index);
	virtual void exec_glUniformMatrix4fv(int32_t location,int32_t count, bool transpose,const float* value);
	virtual void exec_glBindBuffer_GL_PIXEL_UNPACK_BUFFER(uint32_t buffer);
	virtual void exec_glBindBuffer_GL_ELEMENT_ARRAY_BUFFER(uint32_t buffer);
	virtual void exec_glBindBuffer_GL_ARRAY_BUFFER(uint32_t buffer);
	virtual uint8_t* exec_glMapBuffer_GL_PIXEL_UNPACK_BUFFER_GL_WRITE_ONLY();
	virtual void exec_glUnmapBuffer_GL_PIXEL_UNPACK_BUFFER();
	virtual void exec_glEnable_GL_TEXTURE_2D();
	virtual void exec_glEnable_GL_BLEND();
	virtual void exec_glEnable_GL_DEPTH_TEST();
	virtual void exec_glEnable_GL_STENCIL_TEST();
	virtual void exec_glDepthFunc(DEPTH_FUNCTION depthfunc);
	virtual void exec_glDisable_GL_DEPTH_TEST();
	virtual void exec_glDisable_GL_STENCIL_TEST();
	virtual void exec_glDisable_GL_TEXTURE_2D();
	virtual void exec_glFlush();
	virtual uint32_t exec_glCreateShader_GL_FRAGMENT_SHADER();
	virtual uint32_t exec_glCreateShader_GL_VERTEX_SHADER();
	virtual void exec_glShaderSource(uint32_t shader, int32_t count, const char **name, int32_t* length);
	virtual void exec_glCompileShader(uint32_t shader);
	virtual void exec_glGetShaderInfoLog(uint32_t shader,int32_t bufSize,int32_t* length,char* infoLog);
	virtual void exec_glGetProgramInfoLog(uint32_t program,int32_t bufSize,int32_t* length,char* infoLog);
	virtual void exec_glGetShaderiv_GL_COMPILE_STATUS(uint32_t shader,int32_t* params);
	virtual uint32_t exec_glCreateProgram();
	virtual void exec_glBindAttribLocation(uint32_t program,uint32_t index, const char* name);
	virtual int32_t exec_glGetAttribLocation(uint32_t program,const char* name);
	virtual void exec_glAttachShader(uint32_t program, uint32_t shader);
	virtual void exec_glDeleteShader(uint32_t shader);
	virtual void exec_glLinkProgram(uint32_t program);
	virtual void exec_glGetProgramiv_GL_LINK_STATUS(uint32_t program,int32_t* params);
	virtual void exec_glBindFramebuffer_GL_FRAMEBUFFER(uint32_t framebuffer);
	virtual void exec_glFrontFace(bool ccw);
	virtual void exec_glBindRenderbuffer_GL_RENDERBUFFER(uint32_t renderbuffer);
	virtual uint32_t exec_glGenFramebuffer();
	virtual uint32_t exec_glGenRenderbuffer();
	virtual void exec_glFramebufferTexture2D_GL_FRAMEBUFFER(uint32_t textureID);
	virtual void exec_glBindRenderbuffer(uint32_t renderBuffer);
	virtual void exec_glRenderbufferStorage_GL_RENDERBUFFER_GL_DEPTH_COMPONENT16(uint32_t width,uint32_t height);
	virtual void exec_glRenderbufferStorage_GL_RENDERBUFFER_GL_STENCIL_INDEX8(uint32_t width,uint32_t height);
	virtual void exec_glFramebufferRenderbuffer_GL_FRAMEBUFFER_GL_DEPTH_ATTACHMENT(uint32_t depthRenderBuffer);
	virtual void exec_glFramebufferRenderbuffer_GL_FRAMEBUFFER_GL_STENCIL_ATTACHMENT(uint32_t stencilRenderBuffer);
	virtual void exec_glRenderbufferStorage_GL_RENDERBUFFER_GL_DEPTH_STENCIL(uint32_t width,uint32_t height);
	virtual void exec_glFramebufferRenderbuffer_GL_FRAMEBUFFER_GL_DEPTH_STENCIL_ATTACHMENT(uint32_t depthStencilRenderBuffer);
	virtual void exec_glDeleteTextures(int32_t n,uint32_t* textures);
	virtual void exec_glDeleteBuffers(uint32_t size, uint32_t* buffers);
	virtual void exec_glBlendFunc(BLEND_FACTOR src, BLEND_FACTOR dst);
	virtual void exec_glCullFace(TRIANGLE_FACE mode);
	virtual void exec_glActiveTexture_GL_TEXTURE0(uint32_t textureindex);
	virtual void exec_glGenBuffers(uint32_t size, uint32_t* buffers);
	virtual void exec_glUseProgram(uint32_t program);
	virtual void exec_glDeleteProgram(uint32_t program);
	virtual int32_t exec_glGetUniformLocation(uint32_t program,const char* name);
	virtual void exec_glUniform1i(int32_t location,int32_t v0);
	virtual void exec_glUniform4fv(int32_t location, uint32_t count, float* v0);
	virtual void exec_glGenTextures(int32_t n,uint32_t* textures);
	virtual void exec_glViewport(int32_t x,int32_t y,int32_t width,int32_t height);
	virtual void exec_glBufferData_GL_PIXEL_UNPACK_BUFFER_GL_STREAM_DRAW(int32_t size, const void* data);
	virtual void exec_glBufferData_GL_ELEMENT_ARRAY_BUFFER_GL_STATIC_DRAW(int32_t size, const void* data);
	virtual void exec_glBufferData_GL_ELEMENT_ARRAY_BUFFER_GL_DYNAMIC_DRAW(int32_t size, const void* data);
	virtual void exec_glBufferData_GL_ARRAY_BUFFER_GL_STATIC_DRAW(int32_t size, const void* data);
	virtual void exec_glBufferData_GL_ARRAY_BUFFER_GL_DYNAMIC_DRAW(int32_t size, const void* data);
	virtual void exec_glTexParameteri_GL_TEXTURE_2D_GL_TEXTURE_MIN_FILTER_GL_LINEAR();
	virtual void exec_glTexParameteri_GL_TEXTURE_2D_GL_TEXTURE_MAG_FILTER_GL_LINEAR();
	virtual void exec_glSetTexParameters(int32_t lodbias, uint32_t dimension, uint32_t filter, uint32_t mipmap, uint32_t wrap);
	virtual void exec_glTexImage2D_GL_TEXTURE_2D_GL_UNSIGNED_BYTE(int32_t level,int32_t width, int32_t height,int32_t border, const void* pixels);
	virtual void exec_glTexImage2D_GL_TEXTURE_2D_GL_UNSIGNED_INT_8_8_8_8_HOST(int32_t level,int32_t width, int32_t height,int32_t border, const void* pixels);
	virtual void exec_glDrawBuffer_GL_BACK();
	virtual void exec_glClearColor(float red,float green,float blue,float alpha);
	virtual void exec_glClearStencil(uint32_t stencil);
	virtual void exec_glClearDepthf(float depth);
	virtual void exec_glClear_GL_COLOR_BUFFER_BIT();
	virtual void exec_glClear(CLEARMASK mask);
	virtual void exec_glDepthMask(bool flag);
	virtual void exec_glPixelStorei_GL_UNPACK_ROW_LENGTH(int32_t param);
	virtual void exec_glPixelStorei_GL_UNPACK_SKIP_PIXELS(int32_t param);
	virtual void exec_glPixelStorei_GL_UNPACK_SKIP_ROWS(int32_t param);
	virtual void exec_glTexSubImage2D_GL_TEXTURE_2D(int32_t level, int32_t xoffset, int32_t yoffset, int32_t width, int32_t height, const void* pixels, uint32_t w, uint32_t curX, uint32_t curY);
	virtual void exec_glGetIntegerv_GL_MAX_TEXTURE_SIZE(int32_t* data);
	virtual void exec_glGenerateMipmap_GL_TEXTURE_2D();

	// Audio handling
	virtual int audio_StreamInit(AudioStream* s);
	virtual void audio_StreamPause(int channel, bool dopause);
	virtual void audio_StreamSetVolume(int channel, double volume);
	virtual void audio_StreamDeinit(int channel);
	virtual bool audio_ManagerInit();
	virtual void audio_ManagerCloseMixer();
	virtual bool audio_ManagerOpenMixer();
	virtual void audio_ManagerDeinit();
	virtual int audio_getSampleRate();
	
	// Text rendering
	virtual uint8_t* getFontPixelBuffer(int32_t externalressource,int width,int height) { return nullptr; }
	virtual int32_t setupFontRenderer(const TextData &_textData,float a, bool smoothing) { return 0; }
	IDrawable* getTextRenderDrawable(const TextData& _textData, const MATRIX& _m, int32_t _x, int32_t _y, int32_t _w, int32_t _h, float _s, float _a, const std::vector<IDrawable::MaskData>& _ms,bool smoothing);
};

}
#endif /* PLATFORMS_ENGINEUTILS_H */
