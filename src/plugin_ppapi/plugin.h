#ifndef PPAPI_PLUGIN_H
#define PPAPI_PLUGIN_H

#include "swf.h"
#include "ppapi/c/ppp_instance.h"
#include "ppapi/c/pp_var.h"
#include "ppapi/c/pp_resource.h"
#include "ppapi/c/pp_completion_callback.h"

namespace lightspark
{
class ppDownloader;
class ppPluginInstance;


class ppFileStreamCache : public StreamCache {
	friend class ppPluginInstance;
private:
	class ppFileStreamCacheReader : public std::streambuf {
	friend class ppPluginInstance;
	private:
		bool iodone;
		std::streampos curpos;
		std::streamsize readrequest;
		std::streamsize bytesread;
		char* readbuffer;
		_R<ppFileStreamCache> buffer;
		virtual int underflow();
		virtual std::streamsize xsgetn(char* s, std::streamsize n);
		static void readioCallback(void *userdata, int result);
		static void readioCallbackDone(void *userdata, int result);
		
		std::streampos seekoff (std::streamoff off, std::ios_base::seekdir way,std::ios_base::openmode which);
		std::streampos seekpos (std::streampos sp, std::ios_base::openmode which);
	public:
		ppFileStreamCacheReader(_R<ppFileStreamCache> buffer);
	};

	//Cache filename
	PP_Resource cache;
	PP_Resource cacheref;
	int64_t writeoffset;
	ppPluginInstance* m_instance;
	std::vector<uint8_t> internalbuffer;
	ppFileStreamCacheReader* reader;
	bool iodone;

	static void writeioCallback(void* userdata,int result);
	static void writeioCallbackDone(void* userdata,int result);
	static void waitioCallback(void* userdata,int result);
	static void openioCallback(void* userdata,int result);
	void openCache();
	void openExistingCache(const tiny_string& filename, bool forWriting=true);

	// Block until the cache file is opened by the writer stream
	bool waitForCache();

	virtual void handleAppend(const unsigned char* buffer, size_t length);
	
	bool checkCacheFile();
	void write(const unsigned char *buffer, size_t length);
public:
	ppFileStreamCache(ppPluginInstance* instance, SystemState *sys);
	virtual ~ppFileStreamCache();

	virtual std::streambuf *createReader();

	void openForWriting();
};

class ppDownloadManager: public StandaloneDownloadManager
{
private:
	ppPluginInstance* m_instance;
	SystemState* m_sys;
public:
	ppDownloadManager(ppPluginInstance* _instance,SystemState* sys);
	Downloader* download(const URLInfo& url,
					 _R<StreamCache> cache,
					 ILoadable* owner);
	Downloader* downloadWithData(const URLInfo& url,
			_R<StreamCache> cache, const std::vector<uint8_t>& data,
			const std::list<tiny_string>& headers, ILoadable* owner);
	void destroy(Downloader* downloader);
};

class ppDownloader: public Downloader
{
private:
	bool isMainClipDownloader;
	SystemState* m_sys;
	ppPluginInstance* m_pluginInstance;
	uint32_t downloadedlength;
	PP_Resource ppurlloader;
	uint8_t buffer[4096];
	static void dlStartCallback(void* userdata,int result);
	static void dlReadResponseCallback(void* userdata,int result);
	static void dlStartDownloadCallback(void* userdata,int result);
	void startDownload();
public:
	enum STATE { INIT=0, STREAM_DESTROYED, ASYNC_DESTROY };
	STATE state;
	//Constructor used for the main file
	ppDownloader(const tiny_string& _url, ILoadable* owner, ppPluginInstance* ppinstance);
	ppDownloader(const tiny_string& _url, _R<StreamCache> cache, ppPluginInstance* ppinstance, ILoadable* owner);
	ppDownloader(const tiny_string& _url, _R<StreamCache> cache, const std::vector<uint8_t>& _data,
			const std::list<tiny_string>& headers, ppPluginInstance* ppinstance, ILoadable* owner);
};

class ppVariantObject : public ExtVariant
{
public:
	ppVariantObject(std::map<int64_t, std::unique_ptr<ExtObject>>& objectsMap, PP_Var &other);
	static void ExtVariantToppVariant(std::map<const ExtObject*, PP_Var>& objectsMap, PP_Instance instance, const ExtVariant& value, PP_Var& variant);

	static EV_TYPE getTypeS(const PP_Var& variant);
};

class ppObjectObject : public ExtObject
{
public:
	ppObjectObject(std::map<int64_t, std::unique_ptr<ExtObject> > &objectsMap, PP_Var& obj);
	
	static PP_Var getppObject(std::map<const ExtObject*, PP_Var>& objectsMap, PP_Instance instance, const ExtObject* obj);
};

class ppPluginInstance
{
friend class ppPluginEngineData;
	PP_Instance m_ppinstance;
	struct PP_Size m_last_size;
	PP_Resource m_graphics;
	PP_Resource m_cachefilesystem;
	PP_Resource m_cachedirectory_ref;
	ATOMIC_INT32(m_cachefilename);
	SystemState* m_sys;
	std::streambuf *mainDownloaderStreambuf;
	std::istream mainDownloaderStream;
	ppDownloader* mainDownloader;
	ParseThread* m_pt;
	Thread* m_ppLoopThread;
	PP_Resource m_messageloop;
	
	ExtIdentifier m_extmethod_name;
	uint32_t m_extargc;
	PP_Var *m_extargv;
	PP_Var *m_extexception;
	void worker();
	
public:
	ACQUIRE_RELEASE_FLAG(inReading);
	ACQUIRE_RELEASE_FLAG(inWriting);
	ppPluginInstance(PP_Instance instance, int16_t argc,const char* argn[],const char* argv[]);
	virtual ~ppPluginInstance();
	void handleResize(PP_Resource view);
	PP_Bool handleInputEvent(PP_Resource input_event);
	bool executeScript(const std::string script, const ExtVariant **args, uint32_t argc, ASObject **result);
	void executeScriptAsync(ExtScriptObject::HOST_CALL_DATA *data);
	SystemState* getSystemState() const { return m_sys;}
	void startMainParser();
	PP_Instance getppInstance() { return m_ppinstance; }
	PP_Instance getMessageLoop() { return m_messageloop; }
	PP_Resource getFileSystem() { return m_cachefilesystem; }
	PP_Resource createCacheFileRef();
	static void openfilesystem_callback(void *userdata, int result);
	void handleExternalCall(ExtIdentifier &method_name, uint32_t argc, PP_Var *argv, PP_Var *exception);
	void waitiodone(ppFileStreamCache *cache);
	void waitioreaddone(ppFileStreamCache::ppFileStreamCacheReader *cachereader);
	void signaliodone();
	int postwork(PP_CompletionCallback callback);
};

class ppPluginEngineData: public EngineData
{
private:
	ppPluginInstance* instance;
	bool buffersswapped;
public:
	SystemState* sys;
	PP_Resource audioconfig;
	ppPluginEngineData(ppPluginInstance* i, uint32_t w, uint32_t h,SystemState* _sys) : EngineData(), instance(i),buffersswapped(false),sys(_sys),audioconfig(0)
	{
		width = w;
		height = h;
		//needrenderthread=false;
	}
	PP_Resource getGraphics() { return instance->m_graphics;}
	void stopMainDownload();
	bool isSizable() const { return false; }
	uint32_t getWindowForGnash();
	void runInMainThread(SystemState* sys, void (*func) (SystemState*) );
	/* must be called within mainLoopThread */
	SDL_Window* createWidget(uint32_t w,uint32_t h);
	/* must be called within mainLoopThread */
	void grabFocus();
	void openPageInBrowser(const tiny_string& url, const tiny_string& window);
	void setClipboardText(const std::string txt);
	bool getScreenData(SDL_DisplayMode* screen);
	double getScreenDPI();
	StreamCache* createFileStreamCache(SystemState* sys);
	
	static void swapbuffer_done_callback(void* userdata,int result);
	static void swapbuffer_start_callback(void* userdata,int result);
	void DoSwapBuffers();
	void InitOpenGL();
	void DeinitOpenGL();
	bool getGLError(uint32_t &errorCode) const;
	uint8_t* getCurrentPixBuf() const;
	uint8_t* switchCurrentPixBuf(uint32_t w, uint32_t h);
	tiny_string getGLDriverInfo();
	void exec_glUniform1f(int location,float v0);
	void exec_glBindTexture_GL_TEXTURE_2D(uint32_t id);
	void exec_glVertexAttribPointer(uint32_t index, int32_t stride, const void* coords, VERTEXBUFFER_FORMAT format);
	void exec_glEnableVertexAttribArray(uint32_t index);
	void exec_glDrawArrays_GL_TRIANGLES(int32_t first, int32_t count);
	void exec_glDrawElements_GL_TRIANGLES_GL_UNSIGNED_SHORT(int32_t count,const void* indices);
	void exec_glDrawArrays_GL_LINE_STRIP(int32_t first, int32_t count);
	void exec_glDrawArrays_GL_TRIANGLE_STRIP(int32_t first, int32_t count);
	void exec_glDrawArrays_GL_LINES(int32_t first, int32_t count);
	void exec_glDisableVertexAttribArray(uint32_t index);
	void exec_glUniformMatrix4fv(int32_t location,int32_t count, bool transpose,const float* value);
	void exec_glBindBuffer_GL_PIXEL_UNPACK_BUFFER(uint32_t buffer);
	void exec_glBindBuffer_GL_ELEMENT_ARRAY_BUFFER(uint32_t buffer);
	void exec_glBindBuffer_GL_ARRAY_BUFFER(uint32_t buffer);
	uint8_t* exec_glMapBuffer_GL_PIXEL_UNPACK_BUFFER_GL_WRITE_ONLY();
	void exec_glUnmapBuffer_GL_PIXEL_UNPACK_BUFFER();
	void exec_glEnable_GL_TEXTURE_2D();
	void exec_glEnable_GL_BLEND();
	void exec_glEnable_GL_DEPTH_TEST();
	void exec_glDepthFunc(DEPTH_FUNCTION depthfunc);
	void exec_glDisable_GL_DEPTH_TEST();
	void exec_glEnable_GL_STENCIL_TEST();
	void exec_glDisable_GL_STENCIL_TEST();
	void exec_glDisable_GL_TEXTURE_2D();
	void exec_glFlush();
	uint32_t exec_glCreateShader_GL_FRAGMENT_SHADER();
	uint32_t exec_glCreateShader_GL_VERTEX_SHADER();
	void exec_glShaderSource(uint32_t shader, int32_t count, const char** name, int32_t* length);
	void exec_glCompileShader(uint32_t shader);
	void exec_glGetShaderInfoLog(uint32_t shader,int32_t bufSize,int32_t* length,char* infoLog);
	void exec_glGetProgramInfoLog(uint32_t program,int32_t bufSize,int32_t* length,char* infoLog);
	void exec_glGetShaderiv_GL_COMPILE_STATUS(uint32_t shader,int32_t* params);
	uint32_t exec_glCreateProgram();
	void exec_glBindAttribLocation(uint32_t program,uint32_t index, const char* name);
	int32_t exec_glGetAttribLocation(uint32_t program,const char* name);
	void exec_glAttachShader(uint32_t program, uint32_t shader);
	void exec_glDeleteShader(uint32_t shader);
	void exec_glLinkProgram(uint32_t program);
	void exec_glGetProgramiv_GL_LINK_STATUS(uint32_t program,int32_t* params);
	void exec_glBindFramebuffer_GL_FRAMEBUFFER(uint32_t framebuffer);
	void exec_glBindRenderbuffer_GL_RENDERBUFFER(uint32_t renderbuffer);
	uint32_t exec_glGenFramebuffer();
	uint32_t exec_glGenRenderbuffer();
	void exec_glFramebufferTexture2D_GL_FRAMEBUFFER(uint32_t textureID);
	void exec_glBindRenderbuffer(uint32_t renderBuffer);
	void exec_glRenderbufferStorage_GL_RENDERBUFFER_GL_DEPTH_COMPONENT16(uint32_t width,uint32_t height);
	void exec_glRenderbufferStorage_GL_RENDERBUFFER_GL_STENCIL_INDEX8(uint32_t width,uint32_t height);
	void exec_glFramebufferRenderbuffer_GL_FRAMEBUFFER_GL_DEPTH_ATTACHMENT(uint32_t depthRenderBuffer);
	void exec_glFramebufferRenderbuffer_GL_FRAMEBUFFER_GL_STENCIL_ATTACHMENT(uint32_t stencilRenderBuffer);
	void exec_glRenderbufferStorage_GL_RENDERBUFFER_GL_DEPTH_STENCIL(uint32_t width,uint32_t height);
	void exec_glFramebufferRenderbuffer_GL_FRAMEBUFFER_GL_DEPTH_STENCIL_ATTACHMENT(uint32_t depthStencilRenderBuffer);
	void exec_glDeleteTextures(int32_t n,uint32_t* textures);
	void exec_glDeleteBuffers(uint32_t size, uint32_t* buffers);
	void exec_glBlendFunc(BLEND_FACTOR src, BLEND_FACTOR dst);
	void exec_glCullFace(TRIANGLE_FACE mode);
	void exec_glActiveTexture_GL_TEXTURE0(uint32_t textureindex);
	void exec_glGenBuffers(uint32_t size, uint32_t* buffers);
	void exec_glUseProgram(uint32_t program);
	void exec_glDeleteProgram(uint32_t program);
	int32_t exec_glGetUniformLocation(uint32_t program,const char* name);
	void exec_glUniform1i(int32_t location,int32_t v0);
	void exec_glUniform4fv(int32_t location, uint32_t count, float* v0);
	void exec_glGenTextures(int32_t n,uint32_t* textures);
	void exec_glViewport(int32_t x,int32_t y,int32_t width,int32_t height);
	void exec_glBufferData_GL_PIXEL_UNPACK_BUFFER_GL_STREAM_DRAW(int32_t size,const void* data);
	void exec_glBufferData_GL_ELEMENT_ARRAY_BUFFER_GL_STATIC_DRAW(int32_t size, const void* data);
	void exec_glBufferData_GL_ELEMENT_ARRAY_BUFFER_GL_DYNAMIC_DRAW(int32_t size, const void* data);
	void exec_glBufferData_GL_ARRAY_BUFFER_GL_STATIC_DRAW(int32_t size, const void* data);
	void exec_glBufferData_GL_ARRAY_BUFFER_GL_DYNAMIC_DRAW(int32_t size, const void* data);
	void exec_glTexParameteri_GL_TEXTURE_2D_GL_TEXTURE_MIN_FILTER_GL_LINEAR();
	void exec_glTexParameteri_GL_TEXTURE_2D_GL_TEXTURE_MAG_FILTER_GL_LINEAR();
	void exec_glSetTexParameters(int32_t lodbias, uint32_t dimension, uint32_t filter, uint32_t mipmap, uint32_t wrap);
	void exec_glTexImage2D_GL_TEXTURE_2D_GL_UNSIGNED_BYTE(int32_t level,int32_t width, int32_t height,int32_t border, const void* pixels);
	void exec_glTexImage2D_GL_TEXTURE_2D_GL_UNSIGNED_INT_8_8_8_8_HOST(int32_t level,int32_t width, int32_t height,int32_t border, const void* pixels);
	void exec_glDrawBuffer_GL_BACK();
	void exec_glClearColor(float red,float green,float blue,float alpha);
	void exec_glClearStencil(uint32_t stencil);
	void exec_glClearDepthf(float depth);
	void exec_glClear_GL_COLOR_BUFFER_BIT();
	void exec_glClear(CLEARMASK mask);
	void exec_glDepthMask(bool flag);
	void exec_glPixelStorei_GL_UNPACK_ROW_LENGTH(int32_t param);
	void exec_glPixelStorei_GL_UNPACK_SKIP_PIXELS(int32_t param);
	void exec_glPixelStorei_GL_UNPACK_SKIP_ROWS(int32_t param);
	void exec_glTexSubImage2D_GL_TEXTURE_2D(int32_t level,int32_t xoffset,int32_t yoffset,int32_t width,int32_t height,const void* pixels, uint32_t w, uint32_t curX, uint32_t curY);
	void exec_glGetIntegerv_GL_MAX_TEXTURE_SIZE(int32_t* data);
	void exec_glGenerateMipmap_GL_TEXTURE_2D();

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
	virtual IDrawable* getTextRenderDrawable(const TextData& _textData, const MATRIX& _m, int32_t _x, int32_t _y, int32_t _w, int32_t _h, float _s, float _a, const std::vector<IDrawable::MaskData>& _ms);
};

class ppFontRenderer : public IDrawable
{
	PP_Resource ppimage;
public:
	ppFontRenderer(int32_t w, int32_t h, int32_t x, int32_t y, float a, const std::vector<MaskData>& m,PP_Resource _image_data);
	~ppFontRenderer();
	/*
	 * This method returns a raster buffer of the image
	 * The various implementation are responsible for applying the
	 * masks
	 */
	virtual uint8_t* getPixelBuffer();
	/*
	 * This method creates a cairo path that can be used as a mask for
	 * another object
	 */
	virtual void applyCairoMask(cairo_t* cr, int32_t offsetX, int32_t offsetY) const;
	
};

}
#endif // PPAPI_PLUGIN_H
