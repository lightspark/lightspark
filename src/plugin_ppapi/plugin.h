#ifndef PPAPI_PLUGIN_H
#define PPAPI_PLUGIN_H

#include "swf.h"
#include "ppapi/c/ppp_instance.h"

namespace lightspark
{

class ppDownloader;

class ppDownloadManager: public lightspark::StandaloneDownloadManager
{
private:
	PP_Instance instance;
	SystemState* m_sys;
public:
	ppDownloadManager(PP_Instance _instance,SystemState* sys);
	lightspark::Downloader* download(const lightspark::URLInfo& url,
					 _R<StreamCache> cache,
					 lightspark::ILoadable* owner);
	lightspark::Downloader* downloadWithData(const lightspark::URLInfo& url,
			_R<StreamCache> cache, const std::vector<uint8_t>& data,
			const std::list<tiny_string>& headers, lightspark::ILoadable* owner);
	void destroy(lightspark::Downloader* downloader);
};

class ppDownloader: public lightspark::Downloader
{
private:
	bool isMainClipDownloader;
	SystemState* m_sys;
	uint32_t downloadedlength;
	PP_Resource ppurlloader;
	uint8_t buffer[4096];
	
	static void dlStartCallback(void* userdata,int result);
	static void dlReadResponseCallback(void* userdata,int result);
public:
	enum STATE { INIT=0, STREAM_DESTROYED, ASYNC_DESTROY };
	STATE state;
	//Constructor used for the main file
	ppDownloader(const lightspark::tiny_string& _url, PP_Instance _instance, lightspark::ILoadable* owner, SystemState *sys);
	ppDownloader(const lightspark::tiny_string& _url, _R<StreamCache> cache, PP_Instance _instance, lightspark::ILoadable* owner);
	ppDownloader(const lightspark::tiny_string& _url, _R<StreamCache> cache, const std::vector<uint8_t>& _data,
			const std::list<tiny_string>& headers, PP_Instance _instance, lightspark::ILoadable* owner);
};

class ppPluginInstance
{
friend class ppPluginEngineData;
	PP_Instance m_ppinstance;
	struct PP_Size m_last_size;
	PP_Resource m_graphics;
	lightspark::SystemState* m_sys;
	std::streambuf *mainDownloaderStreambuf;
	std::istream mainDownloaderStream;
	ppDownloader* mainDownloader;
	//NPScriptObjectGW* scriptObject;
	lightspark::ParseThread* m_pt;
public:
	ppPluginInstance(PP_Instance instance, int16_t argc,const char* argn[],const char* argv[]);
	virtual ~ppPluginInstance();
	void handleResize(PP_Resource view);
};

class ppPluginEngineData: public EngineData
{
private:
	ppPluginInstance* instance;
public:
	SystemState* sys;
	ppPluginEngineData(ppPluginInstance* i, uint32_t w, uint32_t h,SystemState* _sys) : EngineData(), instance(i),sys(_sys)
	{
		width = w;
		height = h;
		needrenderthread=false;
	}
	PP_Resource getGraphics() { return instance->m_graphics;}
	void stopMainDownload();
	bool isSizable() const { return false; }
	uint32_t getWindowForGnash();
	/* must be called within mainLoopThread */
	SDL_Window* createWidget(uint32_t w,uint32_t h);
	/* must be called within mainLoopThread */
	void grabFocus();
	void openPageInBrowser(const tiny_string& url, const tiny_string& window);
	void setClipboardText(const std::string txt);
	bool getScreenData(SDL_DisplayMode* screen);
	double getScreenDPI();
	void SwapBuffers();
	void InitOpenGL();
	void DeinitOpenGL();
	bool getGLError(uint32_t &errorCode) const;
	void exec_glUniform1f(int location,float v0);
	void exec_glBindTexture_GL_TEXTURE_2D(uint32_t id);
	void exec_glVertexAttribPointer(uint32_t index,int32_t size, int32_t stride, const void* coords);
	void exec_glEnableVertexAttribArray(uint32_t index);
	void exec_glDrawArrays_GL_TRIANGLES(int32_t first, int32_t count);
	void exec_glDrawArrays_GL_LINE_STRIP(int32_t first, int32_t count);
	void exec_glDrawArrays_GL_TRIANGLE_STRIP(int32_t first, int32_t count);
	void exec_glDrawArrays_GL_LINES(int32_t first, int32_t count);
	void exec_glDisableVertexAttribArray(uint32_t index);
	void exec_glUniformMatrix4fv(int32_t location,int32_t count, bool transpose,const float* value);
	void exec_glBindBuffer_GL_PIXEL_UNPACK_BUFFER(uint32_t buffer);
	uint8_t* exec_glMapBuffer_GL_PIXEL_UNPACK_BUFFER_GL_WRITE_ONLY();
	void exec_glUnmapBuffer_GL_PIXEL_UNPACK_BUFFER();
	void exec_glEnable_GL_TEXTURE_2D();
	void exec_glEnable_GL_BLEND();
	void exec_glDisable_GL_TEXTURE_2D();
	void exec_glFlush();
	uint32_t exec_glCreateShader_GL_FRAGMENT_SHADER();
	uint32_t exec_glCreateShader_GL_VERTEX_SHADER();
	void exec_glShaderSource(uint32_t shader, int32_t count, const char** name, int32_t* length);
	void exec_glCompileShader(uint32_t shader);
	void exec_glGetShaderInfoLog(uint32_t shader,int32_t bufSize,int32_t* length,char* infoLog);
	void exec_glGetShaderiv_GL_COMPILE_STATUS(uint32_t shader,int32_t* params);
	uint32_t exec_glCreateProgram();
	void exec_glBindAttribLocation(uint32_t program,uint32_t index, const char* name);
	void exec_glAttachShader(uint32_t program, uint32_t shader);
	void exec_glLinkProgram(uint32_t program);
	void exec_glGetProgramiv_GL_LINK_STATUS(uint32_t program,int32_t* params);
	void exec_glBindFramebuffer_GL_FRAMEBUFFER(uint32_t framebuffer);
	void exec_glDeleteTextures(int32_t n,uint32_t* textures);
	void exec_glDeleteBuffers(int32_t n,uint32_t* buffers);
	void exec_glBlendFunc_GL_ONE_GL_ONE_MINUS_SRC_ALPHA();
	void exec_glActiveTexture_GL_TEXTURE0();
	void exec_glGenBuffers(int32_t n,uint32_t* buffers);
	void exec_glUseProgram(uint32_t program);
	int32_t exec_glGetUniformLocation(uint32_t program,const char* name);
	void exec_glUniform1i(int32_t location,int32_t v0);
	void exec_glGenTextures(int32_t n,uint32_t* textures);
	void exec_glViewport(int32_t x,int32_t y,int32_t width,int32_t height);
	void exec_glBufferData_GL_PIXEL_UNPACK_BUFFER_GL_STREAM_DRAW(int32_t size,const void* data);
	void exec_glTexParameteri_GL_TEXTURE_2D_GL_TEXTURE_MIN_FILTER_GL_LINEAR();
	void exec_glTexParameteri_GL_TEXTURE_2D_GL_TEXTURE_MAG_FILTER_GL_LINEAR();
	void exec_glTexImage2D_GL_TEXTURE_2D_GL_UNSIGNED_BYTE(int32_t level,int32_t width, int32_t height,int32_t border, const void* pixels);
	void exec_glTexImage2D_GL_TEXTURE_2D_GL_UNSIGNED_INT_8_8_8_8_HOST(int32_t level,int32_t width, int32_t height,int32_t border, const void* pixels);
	void exec_glDrawBuffer_GL_BACK();
	void exec_glClearColor(float red,float green,float blue,float alpha);
	void exec_glClear_GL_COLOR_BUFFER_BIT();
	void exec_glPixelStorei_GL_UNPACK_ROW_LENGTH(int32_t param);
	void exec_glPixelStorei_GL_UNPACK_SKIP_PIXELS(int32_t param);
	void exec_glPixelStorei_GL_UNPACK_SKIP_ROWS(int32_t param);
	void exec_glTexSubImage2D_GL_TEXTURE_2D(int32_t level,int32_t xoffset,int32_t yoffset,int32_t width,int32_t height,const void* pixels);
	void exec_glGetIntegerv_GL_MAX_TEXTURE_SIZE(int32_t* data);
};

}
#endif // PPAPI_PLUGIN_H
