#ifndef PPAPI_PLUGIN_H
#define PPAPI_PLUGIN_H

#include "swf.h"
#include "backends/extscriptobject.h"
#include "platforms/engineutils.h"
#include "ppapi/c/ppp_instance.h"
#include "ppapi/c/pp_var.h"
#include "ppapi/c/pp_resource.h"
#include "ppapi/c/pp_completion_callback.h"
#include "ppapi/c/private/ppb_flash_menu.h"

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
public:
	ppDownloadManager(ppPluginInstance* _instance);
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
	SDL_Thread* m_ppLoopThread;
	PP_Resource m_messageloop;
	
	ExtIdentifier m_extmethod_name;
	uint32_t m_extargc;
	PP_Var *m_extargv;
	PP_Var *m_extexception;
	static int worker(void *d);
	PP_Point mousepos;
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
	PP_Flash_Menu ppcontextmenu;
	PP_Resource ppcontextmenuid;
	PP_CompletionCallback contextmenucallback;
	bool buffersswapped;
public:
	SystemState* sys;
	PP_Resource audioconfig;
	ppPluginEngineData(ppPluginInstance* i, uint32_t w, uint32_t h,SystemState* _sys) : EngineData(), instance(i),buffersswapped(false),sys(_sys),audioconfig(0)
	{
		contextmenucallback.func = contextmenucallbackfunc;
		contextmenucallback.user_data = (void*)this;
		contextmenucallback.flags = 0;
		hasExternalFontRenderer=true;
		width = w;
		height = h;
		//needrenderthread=false;
	}
	static void contextmenucallbackfunc(void* user_data, int32_t result);
	PP_Resource getGraphics() { return instance->m_graphics;}
	void stopMainDownload() override;
	bool isSizable() const override { return false; }
	uint32_t getWindowForGnash() override;
	void runInMainThread(SystemState* sys, void (*func) (SystemState*) ) override;

	// local storage handling
	void setLocalStorageAllowedMarker(bool allowed) override;
	bool getLocalStorageAllowedMarker() override;
	bool fillSharedObject(const tiny_string& name, ByteArray* data) override;
	bool flushSharedObject(const tiny_string& name, ByteArray* data) override;
	void removeSharedObject(const tiny_string& name) override;
	
	/* must be called within mainLoopThread */
	SDL_Window* createWidget(uint32_t w,uint32_t h) override;
	/* must be called within mainLoopThread */
	void grabFocus() override;
	void setDisplayState(const tiny_string& displaystate,SystemState* sys) override;
	bool inFullScreenMode() override;

	// context menu handling
	void openContextMenu() override;
	void updateContextMenu(int newselecteditem) override {}
	void updateContextMenuFromMouse(uint32_t windowID, int mousey) override {}
	void renderContextMenu() override {}
	
	void openPageInBrowser(const tiny_string& url, const tiny_string& window) override;
	void setClipboardText(const std::string txt) override;
	bool getScreenData(SDL_DisplayMode* screen) override;
	double getScreenDPI() override;
	StreamCache* createFileStreamCache(SystemState* sys) override;
	
	static void swapbuffer_done_callback(void* userdata,int result);
	static void swapbuffer_start_callback(void* userdata,int result);
	void DoSwapBuffers() override;
	void InitOpenGL() override;
	void DeinitOpenGL() override;
	bool getGLError(uint32_t &errorCode) const override;
	tiny_string getGLDriverInfo() override;
	void getGlCompressedTextureFormats() override;
	void exec_glUniform1f(int location,float v0) override;
	void exec_glUniform2f(int location,float v0, float v1) override;
	void exec_glUniform4f(int location,float v0, float v1, float v2, float v3) override;
	void exec_glBindTexture_GL_TEXTURE_2D(uint32_t id) override;
	void exec_glVertexAttribPointer(uint32_t index, int32_t stride, const void* coords, VERTEXBUFFER_FORMAT format) override;
	void exec_glEnableVertexAttribArray(uint32_t index) override;
	void exec_glDrawArrays_GL_TRIANGLES(int32_t first, int32_t count) override;
	void exec_glDrawElements_GL_TRIANGLES_GL_UNSIGNED_SHORT(int32_t count,const void* indices) override;
	void exec_glDrawArrays_GL_LINE_STRIP(int32_t first, int32_t count) override;
	void exec_glDrawArrays_GL_TRIANGLE_STRIP(int32_t first, int32_t count) override;
	void exec_glDrawArrays_GL_LINES(int32_t first, int32_t count) override;
	void exec_glDisableVertexAttribArray(uint32_t index) override;
	void exec_glUniformMatrix4fv(int32_t location,int32_t count, bool transpose,const float* value) override;
	void exec_glBindBuffer_GL_ELEMENT_ARRAY_BUFFER(uint32_t buffer) override;
	void exec_glBindBuffer_GL_ARRAY_BUFFER(uint32_t buffer) override;
	void exec_glEnable_GL_TEXTURE_2D() override;
	void exec_glEnable_GL_BLEND() override;
	void exec_glEnable_GL_DEPTH_TEST() override;
	void exec_glDepthFunc(DEPTH_FUNCTION depthfunc) override;
	void exec_glDisable_GL_DEPTH_TEST() override;
	void exec_glEnable_GL_STENCIL_TEST() override;
	void exec_glDisable_GL_STENCIL_TEST() override;
	void exec_glDisable_GL_TEXTURE_2D() override;
	void exec_glFlush() override;
	uint32_t exec_glCreateShader_GL_FRAGMENT_SHADER() override;
	uint32_t exec_glCreateShader_GL_VERTEX_SHADER() override;
	void exec_glShaderSource(uint32_t shader, int32_t count, const char** name, int32_t* length) override;
	void exec_glCompileShader(uint32_t shader) override;
	void exec_glGetShaderInfoLog(uint32_t shader,int32_t bufSize,int32_t* length,char* infoLog) override;
	void exec_glGetProgramInfoLog(uint32_t program,int32_t bufSize,int32_t* length,char* infoLog) override;
	void exec_glGetShaderiv_GL_COMPILE_STATUS(uint32_t shader,int32_t* params) override;
	uint32_t exec_glCreateProgram() override;
	void exec_glBindAttribLocation(uint32_t program,uint32_t index, const char* name) override;
	int32_t exec_glGetAttribLocation(uint32_t program,const char* name) override;
	void exec_glAttachShader(uint32_t program, uint32_t shader) override;
	void exec_glDeleteShader(uint32_t shader) override;
	void exec_glLinkProgram(uint32_t program) override;
	void exec_glGetProgramiv_GL_LINK_STATUS(uint32_t program,int32_t* params) override;
	void exec_glBindFramebuffer_GL_FRAMEBUFFER(uint32_t framebuffer) override;
	void exec_glFrontFace(bool ccw) override;
	void exec_glBindRenderbuffer_GL_RENDERBUFFER(uint32_t renderbuffer) override;
	uint32_t exec_glGenFramebuffer() override;
	uint32_t exec_glGenRenderbuffer() override;
	void exec_glFramebufferTexture2D_GL_FRAMEBUFFER(uint32_t textureID) override;
	void exec_glBindRenderbuffer(uint32_t renderBuffer) override;
	void exec_glRenderbufferStorage_GL_RENDERBUFFER_GL_DEPTH_COMPONENT16(uint32_t width,uint32_t height) override;
	void exec_glRenderbufferStorage_GL_RENDERBUFFER_GL_STENCIL_INDEX8(uint32_t width,uint32_t height) override;
	void exec_glFramebufferRenderbuffer_GL_FRAMEBUFFER_GL_DEPTH_ATTACHMENT(uint32_t depthRenderBuffer) override;
	void exec_glFramebufferRenderbuffer_GL_FRAMEBUFFER_GL_STENCIL_ATTACHMENT(uint32_t stencilRenderBuffer) override;
	void exec_glRenderbufferStorage_GL_RENDERBUFFER_GL_DEPTH_STENCIL(uint32_t width,uint32_t height) override;
	void exec_glFramebufferRenderbuffer_GL_FRAMEBUFFER_GL_DEPTH_STENCIL_ATTACHMENT(uint32_t depthStencilRenderBuffer) override;
	void exec_glDeleteTextures(int32_t n,uint32_t* textures) override;
	void exec_glDeleteBuffers(uint32_t size, uint32_t* buffers) override;
	void exec_glBlendFunc(BLEND_FACTOR src, BLEND_FACTOR dst) override;
	void exec_glCullFace(TRIANGLE_FACE mode) override;
	void exec_glActiveTexture_GL_TEXTURE0(uint32_t textureindex) override;
	void exec_glGenBuffers(uint32_t size, uint32_t* buffers) override;
	void exec_glUseProgram(uint32_t program) override;
	void exec_glDeleteProgram(uint32_t program) override;
	int32_t exec_glGetUniformLocation(uint32_t program,const char* name) override;
	void exec_glUniform1i(int32_t location,int32_t v0) override;
	void exec_glUniform4fv(int32_t location, uint32_t count, float* v0) override;
	void exec_glGenTextures(int32_t n,uint32_t* textures) override;
	void exec_glViewport(int32_t x,int32_t y,int32_t width,int32_t height) override;
	void exec_glBufferData_GL_ELEMENT_ARRAY_BUFFER_GL_STATIC_DRAW(int32_t size, const void* data) override;
	void exec_glBufferData_GL_ELEMENT_ARRAY_BUFFER_GL_DYNAMIC_DRAW(int32_t size, const void* data) override;
	void exec_glBufferData_GL_ARRAY_BUFFER_GL_STATIC_DRAW(int32_t size, const void* data) override;
	void exec_glBufferData_GL_ARRAY_BUFFER_GL_DYNAMIC_DRAW(int32_t size, const void* data) override;
	void exec_glTexParameteri_GL_TEXTURE_2D_GL_TEXTURE_MIN_FILTER_GL_LINEAR() override;
	void exec_glTexParameteri_GL_TEXTURE_2D_GL_TEXTURE_MAG_FILTER_GL_LINEAR() override;
	void exec_glTexParameteri_GL_TEXTURE_2D_GL_TEXTURE_MIN_FILTER_GL_NEAREST() override;
	void exec_glTexParameteri_GL_TEXTURE_2D_GL_TEXTURE_MAG_FILTER_GL_NEAREST() override;
	void exec_glSetTexParameters(int32_t lodbias, uint32_t dimension, uint32_t filter, uint32_t mipmap, uint32_t wrap) override;
	void exec_glTexImage2D_GL_TEXTURE_2D_GL_UNSIGNED_BYTE(int32_t level, int32_t width, int32_t height, int32_t border, const void* pixels, bool hasalpha) override;
	void exec_glTexImage2D_GL_TEXTURE_2D_GL_UNSIGNED_INT_8_8_8_8_HOST(int32_t level,int32_t width, int32_t height,int32_t border, const void* pixels) override;
	void exec_glTexImage2D_GL_TEXTURE_2D(int32_t level, int32_t width, int32_t height, int32_t border, void* pixels, TEXTUREFORMAT format, TEXTUREFORMAT_COMPRESSED compressedformat, uint32_t compressedImageSize) override;
	void exec_glDrawBuffer_GL_BACK() override;
	void exec_glClearColor(float red,float green,float blue,float alpha) override;
	void exec_glClearStencil(uint32_t stencil) override;
	void exec_glClearDepthf(float depth) override;
	void exec_glClear_GL_COLOR_BUFFER_BIT() override;
	void exec_glClear(CLEARMASK mask) override;
	void exec_glDepthMask(bool flag) override;
	void exec_glTexSubImage2D_GL_TEXTURE_2D(int32_t level,int32_t xoffset,int32_t yoffset,int32_t width,int32_t height,const void* pixels) override;
	void exec_glGetIntegerv_GL_MAX_TEXTURE_SIZE(int32_t* data) override;
	void exec_glGenerateMipmap_GL_TEXTURE_2D() override;
	void exec_glReadPixels(int32_t width, int32_t height,void* buf) override;
	void exec_glBindTexture_GL_TEXTURE_CUBE_MAP(uint32_t id) override;
	void exec_glTexParameteri_GL_TEXTURE_CUBE_MAP_GL_TEXTURE_MIN_FILTER_GL_LINEAR() override;
	void exec_glTexParameteri_GL_TEXTURE_CUBE_MAP_GL_TEXTURE_MAG_FILTER_GL_LINEAR() override;
	void exec_glTexImage2D_GL_TEXTURE_CUBE_MAP_POSITIVE_X_GL_UNSIGNED_BYTE(uint32_t side, int32_t level,int32_t width, int32_t height,int32_t border, const void* pixels) override;
	void exec_glScissor(int32_t x, int32_t y, int32_t width, int32_t height) override;
	void exec_glDisable_GL_SCISSOR_TEST() override;
	void exec_glColorMask(bool red, bool green, bool blue, bool alpha) override;
	void exec_glStencilFunc_GL_ALWAYS() override;

	// Audio handling
	int audio_StreamInit(AudioStream* s) override;
	void audio_StreamPause(int channel, bool dopause) override;
	void audio_StreamDeinit(int channel) override;
	bool audio_ManagerInit() override;
	void audio_ManagerCloseMixer(AudioManager* manager) override;
	bool audio_ManagerOpenMixer(AudioManager* manager) override;
	void audio_ManagerDeinit() override;
	int audio_getSampleRate() override;
	bool audio_useFloatSampleFormat() override;

	// Text rendering
	uint8_t* getFontPixelBuffer(int32_t externalressource,int width,int height) override;
	int32_t setupFontRenderer(const TextData &_textData, float a, SMOOTH_MODE smoothing) override;
};

}
#endif // PPAPI_PLUGIN_H
