/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2011-2013  Alessandro Pignotti (a.pignotti@sssup.it)
    Copyright (C) 2011       Matthias Gehre (M.Gehre@gmx.de)

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

#include "platforms/engineutils.h"
#include "swf.h"
#include "backends/audio.h"
#include "backends/decoder.h"
#include "backends/input.h"
#include "backends/lsopengl.h"
#include "backends/rendering.h"
#include "backends/sdl/event_loop.h"
#include "version.h"
#include "abc.h"
#include "class.h"
#include "scripting/flash/events/flashevents.h"
#include "scripting/flash/display/Stage.h"
#include "scripting/flash/geom/Rectangle.h"
#include "flash/display/NativeMenuItem.h"
#include "flash/utils/ByteArray.h"
#include "flash/geom/flashgeom.h"
#include "launcher.h"
#include <glib/gstdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#ifdef ENABLE_GLES2
#define IMGUI_IMPL_OPENGL_ES2
#include <imgui_impl_opengl3.h>
#elif defined(ENABLE_GLES3)
#define IMGUI_IMPL_OPENGL_ES3
#include <imgui_impl_opengl3.h>
#else
#include <imgui_impl_opengl2.h>
#endif

extern "C" {
#if defined(ENABLE_GLES2)
extern NVGcontext* nvgCreateGLES2(int flags);
extern void nvgDeleteGLES2(NVGcontext* ctx);
#elif defined(ENABLE_GLES3)
extern NVGcontext* nvgCreateGLES3(int flags);
extern void nvgDeleteGLES3(NVGcontext* ctx);
#else
extern NVGcontext* nvgCreateGL2(int flags);
extern void nvgDeleteGL2(NVGcontext* ctx);
#endif
extern int nvgCreateFontMem(NVGcontext* ctx, const char* name, unsigned char* data, int ndata, int freeData);
}
//The interpretation of texture data change with the endianness
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
#define GL_UNSIGNED_INT_8_8_8_8_HOST GL_UNSIGNED_INT_8_8_8_8_REV
#else
#define GL_UNSIGNED_INT_8_8_8_8_HOST GL_UNSIGNED_BYTE
#endif

extern const char* getDefaultFontBase85();

using namespace std;
using namespace lightspark;

uint32_t EngineData::userevent = (uint32_t)-1;
SDL_Thread* EngineData::mainLoopThread = nullptr;
bool EngineData::mainthread_running = false;
bool EngineData::needinit = true;
bool EngineData::enablerendering = true;
SDL_Cursor* EngineData::handCursor = nullptr;
SDL_Cursor* EngineData::arrowCursor = nullptr;
SDL_Cursor* EngineData::ibeamCursor = nullptr;
Semaphore EngineData::mainthread_initialized(0);

bool lightspark::isEventLoopThread()
{
	auto thread = EngineData::mainLoopThread;
	if (thread != nullptr)
		return SDL_GetThreadID(thread) == SDL_ThreadID();
	else
		return isMainThread();
}

EngineData::EngineData() : sdleventtickjob(nullptr),incontextmenu(false),incontextmenupreparing(false),widget(nullptr),
	nvgcontext(nullptr),
	width(0), height(0),needrenderthread(true),supportPackedDepthStencil(false),hasExternalFontRenderer(false),
	startInFullScreenMode(false),startscalefactor(1.0)
{
#if   defined(_WIN32)
	platformOS="Windows";
#elif defined(__APPLE__)
	platformOS="Mac OS";
#else
	platformOS="Linux";
#endif
}

EngineData::~EngineData()
{
	if (handCursor)
		SDL_FreeCursor(handCursor);
	handCursor=nullptr;
	if (nvgcontext)
#if defined(ENABLE_GLES2)
		nvgDeleteGLES2(nvgcontext);
#elif defined(ENABLE_GLES3)
		nvgDeleteGLES3(nvgcontext);
#else
		nvgDeleteGL2(nvgcontext);
#endif
	handleQuit();
}

void EngineData::runInTrueMainThread(SystemState* sys, MainThreadCallback func)
{
	runInMainThread(sys, func);
}

void EngineData::runInMainThread(SystemState* sys, MainThreadCallback func)
{
	if (!isEventLoopThread())
		sys->pushEvent(LSExecEvent(func));
	else
		func(sys);
}

void EngineData::handleQuit()
{
	if (!SDL_WasInit(0))
		SDL_Quit ();
}

bool EngineData::mainloop_handleevent(const LSEvent& event, SystemState* sys)
{
	using FocusType = LSWindowFocusEvent::FocusType;

	if (event.isInvalid())
		return false;
	bool hasSys = sys != nullptr;
	bool hasEngineData = hasSys && sys->getEngineData() != nullptr;
	bool isMiscType = true;
	bool quit = event.visit(makeVisitor
	(
		[&](const LSInitEvent& init)
		{
			sys = init.sys;
			setTLSSys(sys);
			setTLSWorker(sys->worker);
			return false;
		},
		[&](const LSExecEvent& exec)
		{
			if (exec.callback)
				exec.callback(sys);
			return false;
		},
		[&](const LSOpenContextMenuEvent& open)
		{
			if (hasEngineData)
				sys->getEngineData()->openContextMenuIntern(open.obj);
			return false;
		},
		[&](const LSSelectItemContextMenuEvent&)
		{
			if (hasEngineData)
				sys->getEngineData()->selectContextMenuItemIntern();
			return false;
		},
		[&](const LSQuitEvent& quit)
		{
			if (quit.quitType == LSQuitEvent::QuitType::User)
			{
				assert(hasEngineData);
				sys->getEngineData()->handleQuit();
				return true;
			}
			isMiscType = false;
			return false;
		},
		[&](const LSEvent&) { isMiscType = false; return false; }
	));

	if (quit || isMiscType)
		return quit;
	if (hasSys && sys->getInputThread() != nullptr && sys->getInputThread()->queueEvent(event))
		return false;

	event.visit(makeVisitor
	(
		[&](const LSWindowResizedEvent& resize)
		{
			if (hasSys && (!hasEngineData || !sys->getEngineData()->inFullScreenMode()) && sys->getRenderThread() != nullptr)
				sys->getRenderThread()->requestResize(resize.size,false);
		},
		[&](const LSWindowMovedEvent& move)
		{
			// NOTE: `TODO` is specific to SDL.
			// TODO there doesn't seem to be a way to detect the starting of window movement in SDL, so for now we emit "moving" and "moved" events for every SDL_WINDOWEVENT_MOVED
			sys->getEngineData()->x = move.pos.x;
			sys->getEngineData()->y = move.pos.y;
			if (hasSys && getVm(sys) != nullptr && !sys->stage->nativeWindow.isNull())
			{
				Rectangle *rectBefore=Class<Rectangle>::getInstanceS(sys->worker);
				rectBefore->x = sys->getEngineData()->old_x;
				rectBefore->y = sys->getEngineData()->old_y;
				rectBefore->width = sys->getEngineData()->width;
				rectBefore->height = sys->getEngineData()->height;
				Rectangle *rectAfter=Class<Rectangle>::getInstanceS(sys->worker);
				rectAfter->x = sys->getEngineData()->x;
				rectAfter->y = sys->getEngineData()->y;
				rectAfter->width = sys->getEngineData()->width;
				rectAfter->height = sys->getEngineData()->height;
				getVm(sys)->addEvent(_MR(sys->stage->nativeWindow),_MR(Class<NativeWindowBoundsEvent>::getInstanceS(sys->worker,"moving",_MR(rectBefore),_MR(rectAfter))));
			}
			if (hasSys && getVm(sys) != nullptr && !sys->stage->nativeWindow.isNull())
			{
				Rectangle *rectBefore=Class<Rectangle>::getInstanceS(sys->worker);
				rectBefore->x = sys->getEngineData()->old_x;
				rectBefore->y = sys->getEngineData()->old_y;
				rectBefore->width = sys->getEngineData()->width;
				rectBefore->height = sys->getEngineData()->height;
				Rectangle *rectAfter=Class<Rectangle>::getInstanceS(sys->worker);
				rectAfter->x = sys->getEngineData()->x;
				rectAfter->y = sys->getEngineData()->y;
				rectAfter->width = sys->getEngineData()->width;
				rectAfter->height = sys->getEngineData()->height;
				getVm(sys)->addEvent(_MR(sys->stage->nativeWindow),_MR(Class<NativeWindowBoundsEvent>::getInstanceS(sys->worker,"move",_MR(rectBefore),_MR(rectAfter))));
			}
			sys->getEngineData()->old_x = move.pos.x;
			sys->getEngineData()->old_y = move.pos.y;
		},
		[&](const LSWindowExposedEvent&)
		{
			//Signal the renderThread
			if (hasSys && sys->getRenderThread() != nullptr)
			{
				sys->getRenderThread()->draw(true);
				// it seems that sometimes the systemstate thread stops ticking when in background, this ensures it starts ticking again...
				sys->sendMainSignal();
			}
		},
		[&](const LSWindowFocusEvent& focus)
		{
			if (focus.focusType == FocusType::Keyboard)
			{
				if (focus.focused)
					sys->addBroadcastEvent("activate");
				else
					sys->addBroadcastEvent("deactivate");
			}
		},
		[&](const LSQuitEvent&) { sys->setShutdownFlag(); },
		[](const LSEvent&) {}
	));
	return false;
}
bool EngineData::initSDL()
{
	bool sdl_available = !EngineData::needinit;
	if (EngineData::needinit)
	{
		if (!EngineData::enablerendering)
		{
			if (SDL_WasInit(0)) // some part of SDL already was initialized
				sdl_available = true;
			else
				sdl_available = !SDL_Init ( SDL_INIT_EVENTS );
		}
		else
		{
			if (SDL_WasInit(0)) // some part of SDL already was initialized
				sdl_available = !SDL_InitSubSystem ( SDL_INIT_VIDEO );
			else
				sdl_available = !SDL_Init ( SDL_INIT_VIDEO );
			SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8); // needed for nanovg
			SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24 );
			SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);// needed for nanovg
			SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);// needed for nanovg
#if defined(ENABLE_GLES2)
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(ENABLE_GLES3)
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif
		}
		SDL_StopTextInput();
	}
	return sdl_available;
}
/* main loop handling */
static int mainloop_runner(IEventLoop* th)
{
	if (!EngineData::initSDL())
	{
		LOG(LOG_ERROR,"Unable to initialize SDL:"<<SDL_GetError());
		//EngineData::mainthread_initialized.signal();
		return 0;
	}
	else
	{
		EngineData::mainthread_running = true;
		//EngineData::mainthread_initialized.signal();
		Optional<LSEventStorage> event;
		SystemState* sys = getSys();
		while (event = th->waitEvent(sys), event.hasValue())
		{
			if (EngineData::mainloop_handleevent(*event, sys))
			{
				delete th;
				EngineData::mainthread_running = false;
				return 0;
			}
		}
	}
	return 0;
}
void EngineData::mainloop_from_plugin(SystemState* sys, IEventLoop* eventLoop)
{
	initSDL();
	EngineData::needinit = false;
	Optional<LSEventStorage> event;
	setTLSSys(sys);
	while (event = eventLoop->waitEvent(sys), event.hasValue())
		mainloop_handleevent(*event, sys);
	setTLSSys(nullptr);
}

void EngineData::sdl_mainloop_from_plugin(SystemState* sys)
{
	initSDL();
	EngineData::needinit = false;
	SDL_Event event;
	setTLSSys(sys);
	while (SDL_PollEvent(&event))
		mainloop_handleevent(SDLEventLoop::toLSEvent(sys, event),sys);
	setTLSSys(nullptr);
}

class SDLEventTicker:public ITickJob
{
	EngineData* m_engine;
	SystemState* m_sys;
public:
	SDLEventTicker(EngineData* engine,SystemState* sys):m_engine(engine),m_sys(sys) {}
	void tick() override
	{
		m_engine->runInMainThread(m_sys,EngineData::sdl_mainloop_from_plugin);
		if (!m_engine->inFullScreenMode() && !m_engine->inContextMenu() && !m_engine->inContextMenuPreparing() )
			stopMe=true;
	}
	void tickFence() override
	{
		m_engine->resetSDLEventTicker();
		delete this;
	}
};


void EngineData::startSDLEventTicker(SystemState* sys)
{
	if (sdleventtickjob)
		return;
	sdleventtickjob = new SDLEventTicker(this,sys);
	sys->addTick(50,sdleventtickjob);
}

/* This is not run in the linux plugin, as firefox
 * runs its own gtk_main, which we must not interfere with.
 */
bool EngineData::startSDLMain(EventLoop* eventLoop)
{
	assert(!mainLoopThread);
	//mainLoopThread = SDL_CreateThread(mainloop_runner,"mainloop",eventLoop);
	//mainthread_initialized.wait();
	mainloop_runner(eventLoop);
	return mainthread_running;
}

bool EngineData::FileExists(SystemState* sys, const tiny_string &filename, bool isfullpath)
{
	LOG(LOG_ERROR,"FileExists not implemented");
	return false;
}
bool EngineData::FileIsHidden(SystemState* sys, const tiny_string &filename, bool isfullpath)
{
	LOG(LOG_ERROR,"FileIsHidden not implemented");
	return false;
}
bool EngineData::FileIsDirectory(SystemState* sys, const tiny_string &filename, bool isfullpath)
{
	LOG(LOG_ERROR,"FileIsDirectory not implemented");
	return false;
}

uint32_t EngineData::FileSize(SystemState* sys, const tiny_string& filename, bool isfullpath)
{
	LOG(LOG_ERROR,"FileSize not implemented");
	return 0;
}

tiny_string EngineData::FileFullPath(SystemState* sys, const tiny_string& filename)
{
	LOG(LOG_ERROR,"FileFullPath not implemented");
	return "";
}

tiny_string EngineData::FileBasename(SystemState* sys, const tiny_string& filename, bool isfullpath)
{
	LOG(LOG_ERROR,"FileBasename not implemented");
	return "";
}

tiny_string EngineData::FileRead(SystemState* sys,const tiny_string &filename, bool isfullpath)
{
	LOG(LOG_ERROR,"FileRead not implemented");
	return "";
}

void EngineData::FileWrite(SystemState* sys,const tiny_string &filename, const tiny_string &data, bool isfullpath)
{
	LOG(LOG_ERROR,"FileWrite not implemented");
}

uint8_t EngineData::FileReadUnsignedByte(SystemState* sys, const tiny_string& filename, uint32_t startpos, bool isfullpath)
{
	LOG(LOG_ERROR,"FileReadUnsignedByte not implemented");
	return 0;
}

void EngineData::FileReadByteArray(SystemState* sys, const tiny_string &filename, ByteArray* res, uint32_t startpos, uint32_t length, bool isfullpath)
{
	LOG(LOG_ERROR,"FileReadByteArray not implemented");
}

void EngineData::FileWriteByteArray(SystemState* sys, const tiny_string &filename, ByteArray *data, uint32_t startpos, uint32_t length, bool isfullpath)
{
	LOG(LOG_ERROR,"FileWriteByteArray not implemented");
}

bool EngineData::FileCreateDirectory(SystemState* sys, const tiny_string &filename, bool isfullpath)
{
	LOG(LOG_ERROR,"FileCreateDirectory not implemented");
	return false;
}
bool EngineData::FilGetDirectoryListing(SystemState* sys, const tiny_string &filename, bool isfullpath, std::vector<tiny_string>& filelist)
{
	LOG(LOG_ERROR,"FilGetDirectoryListing not implemented");
	return false;
}

bool EngineData::FilePathIsAbsolute(const tiny_string& filename)
{
	LOG(LOG_ERROR,"FilePathIsAbsolute not implemented");
	return false;
}

tiny_string EngineData::FileGetApplicationStorageDir()
{
	LOG(LOG_ERROR,"FileGetApplicationStorageDir not implemented");
	return "";
}

void EngineData::initGLEW()
{
//For now GLEW does not work with GLES2
#if defined(ENABLE_GLES2)
	supportPackedDepthStencil=SDL_GL_ExtensionSupported("GL_OES_packed_depth_stencil");
#elif defined(ENABLE_GLES3)
	supportPackedDepthStencil=true;
#else
	//Now we can initialize GLEW
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
#ifdef GLEW_ERROR_NO_GLX_DISPLAY
		char* videodriver = getenv("SDL_VIDEODRIVER"); // ignore GLEW_ERROR_NO_GLX_DISPLAY when running on wayland
		if (!videodriver || strcmp(videodriver,"wayland")!= 0 || err != GLEW_ERROR_NO_GLX_DISPLAY)
#endif
		{
			LOG(LOG_ERROR,"Cannot initialize GLEW: cause " << glewGetErrorString(err));
			throw RunTimeException("Rendering: Cannot initialize GLEW!");
		}
	}

	if(!GLEW_VERSION_2_0)
	{
		LOG(LOG_ERROR,"Video card does not support OpenGL 2.0... Aborting");
		throw RunTimeException("Rendering: OpenGL driver does not support OpenGL 2.0");
	}
	if(!GLEW_ARB_framebuffer_object)
	{
		LOG(LOG_ERROR,"OpenGL does not support framebuffer objects!");
		throw RunTimeException("Rendering: OpenGL driver does not support framebuffer objects");
	}
	supportPackedDepthStencil = GLEW_EXT_packed_depth_stencil;
#endif
	initNanoVG();
}

extern int nanoVGdefaultFontID;

void EngineData::initNanoVG()
{
#if defined(ENABLE_GLES2)
	nvgcontext=nvgCreateGLES2(0);
#elif defined(ENABLE_GLES3)
	nvgcontext=nvgCreateGLES3(0);
#else
	nvgcontext=nvgCreateGL2(0);
#endif
	if (nvgcontext == nullptr)
		LOG(LOG_ERROR,"couldn't initialize nanovg");
	else
	{
		// create default font for nanoVG text rendering
		unsigned int buf_decompressed_size=0;
		unsigned char* buf_decompressed_data = getUncompressedDefaultFont(buf_decompressed_size);

		nanoVGdefaultFontID = nvgCreateFontMem(nvgcontext,"__lightspark_font__",buf_decompressed_data,buf_decompressed_size,0);
	}
}
SDL_Window* EngineData::createMainSDLWidget(uint32_t w, uint32_t h)
{
	SDL_Window* window = SDL_CreateWindow("Lightspark",
										  SDL_WINDOWPOS_UNDEFINED,
										  SDL_WINDOWPOS_UNDEFINED,
										  w,
										  h,
										  SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	if (window == 0) {
		// try creation of window without opengl support
		window = SDL_CreateWindow("Lightspark",
								  SDL_WINDOWPOS_UNDEFINED,
								  SDL_WINDOWPOS_UNDEFINED,
								  w,
								  h,
								  SDL_WINDOW_RESIZABLE);
		if (window == 0)
			LOG(LOG_ERROR, "createWidget failed:" << SDL_GetError());
	}
	return window;
}

SDL_GLContext EngineData::createSDLGLContext(SDL_Window* widget)
{
	return SDL_GL_CreateContext(widget);
}

void EngineData::deleteSDLGLContext(SDL_GLContext ctx)
{
	SDL_GL_DeleteContext(ctx);
}

void EngineData::showWindow(uint32_t w, uint32_t h)
{
	assert(!widget);
	this->origwidth = w;
	this->origheight = h;

	// width and height may already be set from the plugin
	if (this->width == 0)
		this->width = w;
	if (this->height == 0)
		this->height = h;
	x=0;
	y=0;
	widget = createWidget(this->width,this->height);
	if (widget)
		SDL_GetWindowPosition(widget,&x,&y);
	old_x =x;
	old_y =y;
	old_width = this->width;
	old_height = this->height;
	// plugins create a hidden window that should not be shown
	if (widget && !(SDL_GetWindowFlags(widget) & SDL_WINDOW_HIDDEN))
		SDL_ShowWindow(widget);
	grabFocus();

}

void EngineData::checkForNativeAIRExtensions(std::vector<tiny_string>& extensions, char* fileName)
{
	tiny_string p = g_path_get_dirname(fileName);
	p += G_DIR_SEPARATOR_S;
	p += "META-INF";
	p += G_DIR_SEPARATOR_S;
	p += "AIR";
	p += G_DIR_SEPARATOR_S;
	p += "extensions";
	GDir* dir = g_dir_open(p.raw_buf(),0,nullptr);
	if (dir)
	{
		while (true)
		{
			const char* subpath = g_dir_read_name(dir);
			if (!subpath)
				break;
			tiny_string extensionpath=p;
			extensionpath += G_DIR_SEPARATOR_S;
			extensionpath += subpath;
			extensionpath += G_DIR_SEPARATOR_S;
			extensionpath += "library.swf";
			if (g_file_test(p.raw_buf(),G_FILE_TEST_EXISTS))
			{
				LOG(LOG_INFO,"native extension found:"<<extensionpath);
				extensions.push_back(extensionpath);
			}
		}
		g_dir_close(dir);
	}
	if (!extensions.empty())
	{
		// try to load additional libraries that may be needed for the extensions
		tiny_string basedir = g_path_get_dirname(fileName);
		GDir* dir = g_dir_open(basedir.raw_buf(),0,nullptr);
		if (dir)
		{
			while (true)
			{
				const char* file = g_dir_read_name(dir);
				if (!file)
					break;
				if (g_file_test(file,G_FILE_TEST_IS_DIR))
					continue;
				tiny_string s=file;
#ifdef _WIN32
				const char* suffix = ".dll";
#else
				const char* suffix = ".so";
#endif
				if (!s.endsWith(suffix))
					continue;
				tiny_string libpath=basedir;
				libpath += G_DIR_SEPARATOR_S;
				libpath += s;
				void* lib = SDL_LoadObject(libpath.raw_buf());
				if (lib==nullptr)
					LOG(LOG_ERROR,"loading additional lib failed:"<<SDL_GetError()<<" "<<libpath);
				else
					LOG(LOG_INFO,"additional lib loaded:"<<lib<<" "<<libpath);
			}
			g_dir_close(dir);
		}
	}
}

void EngineData::addQuitEvent()
{
	if (getSys()->runSingleThreaded || !isEventLoopThread())
	{
		getSys()->pushEvent(LSQuitEvent(LSQuitEvent::QuitType::User));
		SDL_WaitThread(EngineData::mainLoopThread,nullptr);
	}
	else
	{
		handleQuit();
	}
}

void EngineData::notifyTimer()
{
	SDL_Event event;
	SDL_zero(event);
	event.type = LS_USEREVENT_NEW_TIMER;
	SDL_PushEvent(&event);
}

std::string EngineData::getsharedobjectfilename(const tiny_string& name)
{
	tiny_string subdir = sharedObjectDatapath + G_DIR_SEPARATOR_S;
	subdir += "sharedObjects";
	g_mkdir_with_parents(subdir.raw_buf(),0700);

	std::string p = subdir.raw_buf();
	p += G_DIR_SEPARATOR_S;
	p += name.raw_buf();
	p += ".sol";
	return p;
}
void EngineData::setLocalStorageAllowedMarker(bool allowed)
{
	tiny_string subdir = sharedObjectDatapath + G_DIR_SEPARATOR_S;
	g_mkdir_with_parents(subdir.raw_buf(),0700);

	std::string p = subdir.raw_buf();
	p += G_DIR_SEPARATOR_S;
	p += "localStorageAllowed";
	if (allowed)
	{
		if (!g_file_test(p.c_str(),G_FILE_TEST_EXISTS))
			g_creat(p.c_str(),0600);
	}
	else
		g_unlink(p.c_str());
}
bool EngineData::getLocalStorageAllowedMarker()
{
	tiny_string subdir = sharedObjectDatapath + G_DIR_SEPARATOR_S;
	if (!g_file_test(subdir.raw_buf(),G_FILE_TEST_EXISTS))
		return false;
	g_mkdir_with_parents(subdir.raw_buf(),0700);

	std::string p = subdir.raw_buf();
	p += G_DIR_SEPARATOR_S;
	p += "localStorageAllowed";
	return g_file_test(p.c_str(),G_FILE_TEST_EXISTS);
}
bool EngineData::fillSharedObject(const tiny_string &name, ByteArray *data)
{
	if (!getLocalStorageAllowedMarker())
		return false;
	std::string p = getsharedobjectfilename(name);
	if (!g_file_test(p.c_str(),G_FILE_TEST_EXISTS))
		return false;
	GStatBuf st_buf;
	g_stat(p.c_str(),&st_buf);
	uint32_t len = st_buf.st_size;
	std::ifstream file;
	uint8_t buf[len];
	file.open(p, std::ios::in|std::ios::binary);
	file.read((char*)buf,len);
	data->writeBytes(buf,len);
	file.close();
	return true;
}
bool EngineData::flushSharedObject(const tiny_string &name, ByteArray *data)
{
	if (!getLocalStorageAllowedMarker())
		return false;
	std::string p = getsharedobjectfilename(name);
	std::ofstream file;
	file.open(p, std::ios::out|std::ios::binary|std::ios::trunc);
	uint8_t* buf = data->getBuffer(data->getLength(),false);
	file.write((char*)buf,data->getLength());
	file.close();
	return true;
}
void EngineData::removeSharedObject(const tiny_string &name)
{
	std::string p = getsharedobjectfilename(name);
	g_unlink(p.c_str());
}

void EngineData::setDisplayState(const tiny_string &displaystate,SystemState* sys)
{
	if (!this->widget)
	{
		LOG(LOG_ERROR,"no widget available for setting displayState");
		return;
	}
	SDL_SetWindowFullscreen(widget, displaystate.startsWith("fullScreen") ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
	int w,h;
	SDL_GetWindowSize(widget,&w,&h);
	sys->getRenderThread()->requestResize(w,h,true);
}

bool EngineData::inFullScreenMode()
{
	if (!this->widget)
	{
		LOG(LOG_ERROR,"no widget available for getting fullscreen mode");
		return false;
	}
	return SDL_GetWindowFlags(widget) & SDL_WINDOW_FULLSCREEN_DESKTOP;
}

void EngineData::openContextMenuIntern(InteractiveObject *dispatcher)
{
	if (incontextmenu)
		return;
	incontextmenu=true;
	dispatcher->incRef();
	contextmenuDispatcher = _MR(dispatcher);
	currentcontextmenuitems.clear();
	contextmenuOwner= dispatcher->getCurrentContextMenuItems(currentcontextmenuitems);
	openContextMenu();
}

void EngineData::openContextMenu()
{
	int x=0,y=0;
#if SDL_VERSION_ATLEAST(2, 0, 4)
	SDL_GetGlobalMouseState(&x,&y);
#else
	LOG(LOG_ERROR,"SDL2 version too old to get mouse position");
	SDL_GetWindowPosition(widget,&x,&y);
#endif
	int contextmenuheight = 0;
	for (uint32_t i = 0; i <currentcontextmenuitems.size(); i++)
	{
		NativeMenuItem* item = currentcontextmenuitems.at(i).getPtr();
		contextmenuheight += item->isSeparator ? 1 : CONTEXTMENUITEMHEIGHT;
	}
#if SDL_VERSION_ATLEAST(2, 0, 5)
	SDL_Window* window = SDL_CreateWindow("",x,y,CONTEXTMENUWIDTH,contextmenuheight,SDL_WINDOW_OPENGL|SDL_WINDOW_BORDERLESS|SDL_WINDOW_SHOWN|SDL_WINDOW_INPUT_FOCUS|SDL_WINDOW_MOUSE_FOCUS|SDL_WINDOW_POPUP_MENU);
#else
	LOG(LOG_ERROR,"SDL2 version too old to create popup menu");
	SDL_Window* window = SDL_CreateWindow("",x,y,CONTEXTMENUWIDTH,contextmenuheight,SDL_WINDOW_OPENGL|SDL_WINDOW_BORDERLESS|SDL_WINDOW_SHOWN|SDL_WINDOW_INPUT_FOCUS|SDL_WINDOW_MOUSE_FOCUS);
#endif
	if (window == nullptr)
	{
		printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
		return;
	}
	SDL_GLContext gl_context = SDL_GL_CreateContext(window);
	SDL_GL_MakeCurrent(window, gl_context);
	SDL_GL_SetSwapInterval(1); // Enable vsync

	SDL_SetWindowInputFocus(window);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
	io.IniFilename=nullptr;
	io.Fonts->AddFontFromMemoryCompressedBase85TTF(Launcher::getDefaultFontBase85(), 14);

	// Setup Dear ImGui style
	ImGui::StyleColorsClassic();
	ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign,ImVec2(0,0.5));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,ImVec2(0,0));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize,1);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(0,0));
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize,0);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,ImVec2(5,0));

	ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(0.9,0.9,0.9,1.0));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered,ImVec4(0.5,0.5,0.5,1.0));
	ImGui::PushStyleColor(ImGuiCol_Text,ImVec4(0.0,0.0,0.0,1.0));






	// Setup Platform/Renderer backends
	ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
#ifdef ENABLE_GLES2
	ImGui_ImplOpenGL3_Init("#version 100");
#elif defined(ENABLE_GLES3)
	ImGui_ImplOpenGL3_Init("#version 100");
#else
	ImGui_ImplOpenGL2_Init();
#endif
	incontextmenu=true;
	contextmenucurrentitem=-1;

	// Main loop
	bool done = false;
	while (!done)
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			ImGui_ImplSDL2_ProcessEvent(&event);
			if (event.type == SDL_QUIT)
				done = true;
			if (event.type == SDL_WINDOWEVENT
				&& event.window.event == SDL_WINDOWEVENT_CLOSE
				&& event.window.windowID == SDL_GetWindowID(window))
				done = true;
			if (event.type == SDL_WINDOWEVENT
				&& event.window.event == SDL_WINDOWEVENT_FOCUS_LOST
				&& event.window.windowID == SDL_GetWindowID(window))
				done = true;
		}

		// Start the Dear ImGui frame
#if defined(ENABLE_GLES2) || defined(ENABLE_GLES3)
		ImGui_ImplOpenGL3_NewFrame();
#else
		ImGui_ImplOpenGL2_NewFrame();
#endif
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();
		ImGui::SetNextWindowSize(io.DisplaySize);
		ImGui::SetNextWindowPos(ImVec2(0,0));

		ImGui::Begin("Lightspark ContextMenu",nullptr,ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize);

		for (uint32_t i = 0; i <currentcontextmenuitems.size(); i++)
		{
			NativeMenuItem* item = currentcontextmenuitems.at(i).getPtr();
			if (item->isSeparator)
			{
				ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(0,0,0,1.0));
				ImGui::Button("##",ImVec2(CONTEXTMENUWIDTH,1.0));
				ImGui::PopStyleColor();
			}
			else
			{
				if (ImGui::Button(item->label.raw_buf(),ImVec2(CONTEXTMENUWIDTH,CONTEXTMENUITEMHEIGHT)))
				{
					contextmenucurrentitem=i;
					done = true;
				}
			}
		}
		ImGui::End();

		// Rendering
		ImGui::Render();
		glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
		glClearColor(0.0,0.0,0.0,1.0);
		glClear(GL_COLOR_BUFFER_BIT);
#if defined(ENABLE_GLES2) || defined(ENABLE_GLES3)
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#else
		ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
#endif
		SDL_GL_SwapWindow(window);
	}

	// Cleanup
#if defined(ENABLE_GLES2) || defined(ENABLE_GLES3)
	ImGui_ImplOpenGL3_Shutdown();
#else
	ImGui_ImplOpenGL2_Shutdown();
#endif
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	SDL_GL_DeleteContext(gl_context);
	SDL_DestroyWindow(window);
	incontextmenu=false;
	selectContextMenuItemIntern();
}

void EngineData::selectContextMenuItem()
{
	getSys()->pushEvent(LSSelectItemContextMenuEvent());
}
void EngineData::InteractiveObjectRemovedFromStage()
{
	getSys()->pushEvent(LSRemovedFromStageEvent());
}

void EngineData::selectContextMenuItemIntern()
{
	if (contextmenucurrentitem >=0)
	{
		NativeMenuItem* item = currentcontextmenuitems.at(contextmenucurrentitem).getPtr();
		if (item->label == "Settings")
		{
			item->getSystemState()->getRenderThread()->inSettings=true;
			return;
		}
		if (item->label=="Save" ||
			item->label=="Zoom In" ||
			item->label=="Zoom Out" ||
			item->label=="100%" ||
			item->label=="Show all" ||
			item->label=="Quality" ||
			item->label=="Play" ||
			item->label=="Loop" ||
			item->label=="Rewind" ||
			item->label=="Forward" ||
			item->label=="Back" ||
			item->label=="Print")
		{
			tiny_string msg("context menu handling not implemented for \"");
			msg += item->label;
			msg += "\"";
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,"Lightspark",msg.raw_buf(),widget);
			return;
		}
		else if (item->label=="About")
		{
			tiny_string msg("Lightspark version ");
			msg += VERSION;
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION,"Lightspark",msg.raw_buf(),widget);
			return;
		}
		else
		{
			item->incRef();
			getVm(item->getSystemState())->addEvent(_MR(item),_MR(Class<ContextMenuEvent>::getInstanceS(item->getInstanceWorker(),"menuItemSelect",contextmenuDispatcher,contextmenuOwner)));
		}
	}
}

SDL_Window* EngineData::createWidget(uint32_t w, uint32_t h)
{
	// Setup SDL
	SDL_Window* window = createMainSDLWidget(w, h);
	Launcher::setWindowIcon(window);
	return window;
}


void EngineData::showMouseCursor(SystemState* /*sys*/)
{
	SDL_ShowCursor(SDL_ENABLE);
}

void EngineData::hideMouseCursor(SystemState* /*sys*/)
{
	SDL_ShowCursor(SDL_DISABLE);
}

void EngineData::setMouseHandCursor(SystemState* /*sys*/)
{
	if (!handCursor)
		handCursor = SDL_CreateSystemCursor(SDL_SystemCursor::SDL_SYSTEM_CURSOR_HAND);
	SDL_SetCursor(handCursor);
}
void EngineData::resetMouseHandCursor(SystemState* /*sys*/)
{
	SDL_SetCursor(SDL_GetDefaultCursor());
}
void EngineData::setMouseCursor(SystemState* /*sys*/, const tiny_string& name)
{
	if (name=="hand")
	{
		if (!handCursor)
			handCursor = SDL_CreateSystemCursor(SDL_SystemCursor::SDL_SYSTEM_CURSOR_HAND);
		SDL_SetCursor(handCursor);
	}
	else if (name=="arrow")
	{
		if (!arrowCursor)
			arrowCursor = SDL_CreateSystemCursor(SDL_SystemCursor::SDL_SYSTEM_CURSOR_ARROW);
		SDL_SetCursor(arrowCursor);
	}
	else if (name=="ibeam")
	{
		if (!ibeamCursor)
			ibeamCursor = SDL_CreateSystemCursor(SDL_SystemCursor::SDL_SYSTEM_CURSOR_IBEAM);
		SDL_SetCursor(ibeamCursor);
	}
	else if (name=="auto")
		SDL_SetCursor(SDL_GetDefaultCursor());
	else
	{
		LOG(LOG_NOT_IMPLEMENTED,"showCursor for name:"<<name);
		SDL_SetCursor(SDL_GetDefaultCursor());
	}
}
tiny_string EngineData::getMouseCursor(SystemState *sys)
{
	SDL_Cursor* cursor = SDL_GetCursor();
	if (cursor == handCursor)
		return "hand";
	if (cursor == arrowCursor)
		return "arrow";
	if (cursor == ibeamCursor)
		return "ibeam";
	return "auto";
}
void EngineData::setClipboardText(const std::string txt)
{
	int ret = SDL_SetClipboardText(txt.c_str());
	if (ret == 0)
		LOG(LOG_INFO, "Copied error to clipboard");
	else
		LOG(LOG_ERROR, "copying text to clipboard failed:"<<SDL_GetError());
}

bool EngineData::getScreenData(SDL_DisplayMode* screen)
{
	if (SDL_GetDesktopDisplayMode(0, screen) != 0) {
		LOG(LOG_ERROR,"Capabilities: SDL_GetDesktopDisplayMode failed:"<<SDL_GetError());
		return false;
	}
	return true;
}

double EngineData::getScreenDPI()
{
#if SDL_VERSION_ATLEAST(2, 0, 4)
	float ddpi;
	float hdpi;
	float vdpi;
	SDL_GetDisplayDPI(SDL_GetWindowDisplayIndex(widget),&ddpi,&hdpi,&vdpi);
	return ddpi;
#else
	LOG(LOG_NOT_IMPLEMENTED,"getScreenDPI needs SDL version >= 2.0.4");
	return 96.0;
#endif
}

void EngineData::setWindowPosition(int x, int y, uint32_t width, uint32_t height)
{
	if (widget)
	{
		SDL_SetWindowPosition(widget,x,y);
		SDL_SetWindowSize(widget,width,height);
	}
}

void EngineData::setWindowPosition(const Vector2& pos, const Vector2& size)
{
	setWindowPosition(pos.x, pos.y, size.x, size.y);
}

void EngineData::getWindowPosition(int* x, int* y)
{
	if (widget)
	{
		SDL_GetWindowPosition(widget,x,y);
	}
	else
	{
		*x=0;
		*y=0;
	}
}

Vector2 EngineData::getWindowPosition()
{
	Vector2 pos;
	getWindowPosition(&pos.x, &pos.y);
	return pos;
}

StreamCache *EngineData::createFileStreamCache(SystemState* sys)
{
	return new FileStreamCache(sys);
}

void EngineData::DoSwapBuffers()
{
	uint32_t err;
	if (getGLError(err))
		LOG(LOG_ERROR,"swapbuffers:"<<widget<<" "<<err);
	SDL_GL_SwapWindow(widget);
}

void EngineData::InitOpenGL()
{
	mSDLContext = createSDLGLContext(widget);
	if (!mSDLContext)
		LOG(LOG_ERROR,"failed to create openGL context:"<<SDL_GetError());
	initGLEW();
}

void EngineData::DeinitOpenGL()
{
	deleteSDLGLContext(mSDLContext);
}

bool EngineData::getGLError(uint32_t &errorCode) const
{
	errorCode=glGetError();
	return errorCode!=GL_NO_ERROR;
}

tiny_string EngineData::getGLDriverInfo()
{
	tiny_string res = "OpenGL Vendor=";
	res += (const char*)glGetString(GL_VENDOR);
	res += " Version=";
	res += (const char*)glGetString(GL_VERSION);
	res += " Renderer=";
	res += (const char*)glGetString(GL_RENDERER);
	res += " GLSL=";
	res += (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
	return res;
}

void EngineData::getGlCompressedTextureFormats()
{
	int32_t numformats;
	glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS,&numformats);
	if (numformats == 0)
		return;
	int32_t* formats = new int32_t[numformats];
	glGetIntegerv(GL_COMPRESSED_TEXTURE_FORMATS,formats);
	for (int32_t i = 0; i < numformats; i++)
	{
		LOG(LOG_INFO,"OpenGL supported compressed texture format:"<<hex<<formats[i]);
		if (formats[i] == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT)
			compressed_texture_formats.push_back(TEXTUREFORMAT_COMPRESSED::DXT5);
		if (formats[i] == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT)
			compressed_texture_formats.push_back(TEXTUREFORMAT_COMPRESSED::DXT1);
	}
	delete [] formats;
}

void EngineData::exec_glUniform1f(int location,float v0)
{
	glUniform1f(location,v0);
}

void EngineData::exec_glUniform2f(int location,float v0,float v1)
{
	glUniform2f(location,v0,v1);
}
void EngineData::exec_glUniform4f(int location,float v0,float v1,float v2,float v3)
{
	glUniform4f(location,v0,v1,v2,v3);
}

void EngineData::exec_glUniform1fv(int location, uint32_t size, float* v)
{
	glUniform1fv(location,size,v);
}

void EngineData::exec_glBindTexture_GL_TEXTURE_2D(uint32_t id)
{
	glBindTexture(GL_TEXTURE_2D, id);
}

void EngineData::exec_glVertexAttribPointer(uint32_t index, int32_t stride, const void* coords, VERTEXBUFFER_FORMAT format)
{
	switch (format)
	{
		case BYTES_4: glVertexAttribPointer(index, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride, coords); break;
		case FLOAT_4: glVertexAttribPointer(index, 4, GL_FLOAT, GL_FALSE, stride, coords); break;
		case FLOAT_3: glVertexAttribPointer(index, 3, GL_FLOAT, GL_FALSE, stride, coords); break;
		case FLOAT_2: glVertexAttribPointer(index, 2, GL_FLOAT, GL_FALSE, stride, coords); break;
		case FLOAT_1: glVertexAttribPointer(index, 1, GL_FLOAT, GL_FALSE, stride, coords); break;
		default:
			LOG(LOG_ERROR,"invalid VERTEXBUFFER_FORMAT");
			break;
	}
}

void EngineData::exec_glEnableVertexAttribArray(uint32_t index)
{
	glEnableVertexAttribArray(index);
}

void EngineData::exec_glDrawArrays_GL_TRIANGLES(int32_t first,int32_t count)
{
	glDrawArrays(GL_TRIANGLES,first,count);
}
void EngineData::exec_glDrawElements_GL_TRIANGLES_GL_UNSIGNED_SHORT(int32_t count,const void* indices)
{
	glDrawElements(GL_TRIANGLES,count,GL_UNSIGNED_SHORT,indices);
}
void EngineData::exec_glDrawArrays_GL_LINE_STRIP(int32_t first,int32_t count)
{
	glDrawArrays(GL_LINE_STRIP,first,count);
}

void EngineData::exec_glDrawArrays_GL_TRIANGLE_STRIP(int32_t first, int32_t count)
{
	glDrawArrays(GL_TRIANGLE_STRIP,first,count);
}

void EngineData::exec_glDrawArrays_GL_LINES(int32_t first, int32_t count)
{
	glDrawArrays(GL_LINES,first,count);
}

void EngineData::exec_glDisableVertexAttribArray(uint32_t index)
{
	glDisableVertexAttribArray(index);
}

void EngineData::exec_glUniformMatrix4fv(int32_t location,int32_t count, bool transpose,const float* value)
{
	glUniformMatrix4fv(location, count, transpose, value);
}
void EngineData::exec_glBindBuffer_GL_ELEMENT_ARRAY_BUFFER(uint32_t buffer)
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,buffer);
}
void EngineData::exec_glBindBuffer_GL_ARRAY_BUFFER(uint32_t buffer)
{
	glBindBuffer(GL_ARRAY_BUFFER,buffer);
}
void EngineData::exec_glEnable_GL_TEXTURE_2D()
{
#ifndef ENABLE_GLES2
	glEnable(GL_TEXTURE_2D);
#endif
}
void EngineData::exec_glEnable_GL_BLEND()
{
	glEnable(GL_BLEND);
}
void EngineData::exec_glEnable_GL_DEPTH_TEST()
{
	glEnable(GL_DEPTH_TEST);
}
void EngineData::exec_glDisable_GL_STENCIL_TEST()
{
	glDisable(GL_STENCIL_TEST);
}
void EngineData::exec_glDepthFunc(DEPTHSTENCIL_FUNCTION depthfunc)
{
	switch (depthfunc)
	{
		case DEPTHSTENCIL_ALWAYS:
			glDepthFunc(GL_ALWAYS);
			break;
		case DEPTHSTENCIL_EQUAL:
			glDepthFunc(GL_EQUAL);
			break;
		case DEPTHSTENCIL_GREATER:
			glDepthFunc(GL_GREATER);
			break;
		case DEPTHSTENCIL_GREATER_EQUAL:
			glDepthFunc(GL_GEQUAL);
			break;
		case DEPTHSTENCIL_LESS:
			glDepthFunc(GL_LESS);
			break;
		case DEPTHSTENCIL_LESS_EQUAL:
			glDepthFunc(GL_LEQUAL);
			break;
		case DEPTHSTENCIL_NEVER:
			glDepthFunc(GL_NEVER);
			break;
		case DEPTHSTENCIL_NOT_EQUAL:
			glDepthFunc(GL_NOTEQUAL);
			break;
	}
}

void EngineData::exec_glDisable_GL_DEPTH_TEST()
{
	glDisable(GL_DEPTH_TEST);
}

void EngineData::exec_glEnable_GL_STENCIL_TEST()
{
	glEnable(GL_STENCIL_TEST);
}

void EngineData::exec_glDisable_GL_TEXTURE_2D()
{
#ifndef ENABLE_GLES2
	glDisable(GL_TEXTURE_2D);
#endif
}
void EngineData::exec_glFlush()
{
	glFlush();
}
void EngineData::exec_glFinish()
{
	glFinish();
}

uint32_t EngineData::exec_glCreateShader_GL_FRAGMENT_SHADER()
{
	return glCreateShader(GL_FRAGMENT_SHADER);
}

uint32_t EngineData::exec_glCreateShader_GL_VERTEX_SHADER()
{
	return glCreateShader(GL_VERTEX_SHADER);
}

void EngineData::exec_glShaderSource(uint32_t shader, int32_t count, const char** name, int32_t* length)
{
	glShaderSource(shader,count,name,length);
}

void EngineData::exec_glCompileShader(uint32_t shader)
{
	glCompileShader(shader);
}

void EngineData::exec_glGetShaderInfoLog(uint32_t shader,int32_t bufSize,int32_t* length,char* infoLog)
{
	glGetShaderInfoLog(shader,bufSize,length,infoLog);
}

void EngineData::exec_glGetProgramInfoLog(uint32_t program,int32_t bufSize,int32_t* length,char* infoLog)
{
	glGetProgramInfoLog(program,bufSize,length,infoLog);
}


void EngineData::exec_glGetShaderiv_GL_COMPILE_STATUS(uint32_t shader,int32_t* params)
{
	glGetShaderiv(shader,GL_COMPILE_STATUS,params);
}

uint32_t EngineData::exec_glCreateProgram()
{
	return glCreateProgram();
}

void EngineData::exec_glBindAttribLocation(uint32_t program,uint32_t index, const char* name)
{
	glBindAttribLocation(program,index,name);
}

int32_t EngineData::exec_glGetAttribLocation(uint32_t program, const char *name)
{
	return glGetAttribLocation(program,name);
}

void EngineData::exec_glAttachShader(uint32_t program, uint32_t shader)
{
	glAttachShader(program,shader);
}
void EngineData::exec_glDeleteShader(uint32_t shader)
{
	glDeleteShader(shader);
}

void EngineData::exec_glLinkProgram(uint32_t program)
{
	glLinkProgram(program);
}

void EngineData::exec_glGetProgramiv_GL_LINK_STATUS(uint32_t program,int32_t* params)
{
	glGetProgramiv(program,GL_LINK_STATUS,params);
}

void EngineData::exec_glBindFramebuffer_GL_FRAMEBUFFER(uint32_t framebuffer)
{
	glBindFramebuffer(GL_FRAMEBUFFER,framebuffer);
}
void EngineData::exec_glFrontFace(bool ccw)
{
	glFrontFace(ccw ? GL_CCW : GL_CW);
}

void EngineData::exec_glBindRenderbuffer_GL_RENDERBUFFER(uint32_t renderbuffer)
{
	glBindRenderbuffer(GL_RENDERBUFFER,renderbuffer);
}

uint32_t EngineData::exec_glGenFramebuffer()
{
	uint32_t framebuffer;
	glGenFramebuffers(1,&framebuffer);
	return framebuffer;
}
uint32_t EngineData::exec_glGenRenderbuffer()
{
	uint32_t renderbuffer;
	glGenRenderbuffers(1,&renderbuffer);
	return renderbuffer;
}

void EngineData::exec_glFramebufferTexture2D_GL_FRAMEBUFFER(uint32_t textureID)
{
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureID, 0);
	if (textureID)
		glDrawBuffer(GL_COLOR_ATTACHMENT0);
}

void EngineData::exec_glBindRenderbuffer(uint32_t renderBuffer)
{
	glBindRenderbuffer(GL_RENDERBUFFER,renderBuffer);
}

void EngineData::exec_glRenderbufferStorage_GL_RENDERBUFFER_GL_DEPTH_STENCIL(uint32_t width, uint32_t height)
{
#if defined(ENABLE_GLES2)
#ifdef GL_DEPTH24_STENCIL8_OES
	glRenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH24_STENCIL8_OES,width,height);
#endif
#else
	glRenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH24_STENCIL8,width,height);
#endif
}

void EngineData::exec_glFramebufferRenderbuffer_GL_FRAMEBUFFER_GL_DEPTH_STENCIL_ATTACHMENT(uint32_t depthStencilRenderBuffer)
{
#ifdef ENABLE_GLES2
	glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,depthStencilRenderBuffer);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER,depthStencilRenderBuffer);
#else
	glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER,depthStencilRenderBuffer);
#endif
}

void EngineData::exec_glRenderbufferStorage_GL_RENDERBUFFER_GL_DEPTH_COMPONENT16(uint32_t width, uint32_t height)
{
	glRenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH_COMPONENT16,width,height);
}

void EngineData::exec_glRenderbufferStorage_GL_RENDERBUFFER_GL_STENCIL_INDEX8(uint32_t width, uint32_t height)
{
	glRenderbufferStorage(GL_RENDERBUFFER,GL_STENCIL_INDEX8,width,height);
}

void EngineData::exec_glFramebufferRenderbuffer_GL_FRAMEBUFFER_GL_DEPTH_ATTACHMENT(uint32_t depthRenderBuffer)
{
	glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,depthRenderBuffer);
}

void EngineData::exec_glFramebufferRenderbuffer_GL_FRAMEBUFFER_GL_STENCIL_ATTACHMENT(uint32_t stencilRenderBuffer)
{
	glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER,stencilRenderBuffer);
}

void EngineData::exec_glDeleteTextures(int32_t n,uint32_t* textures)
{
	glDeleteTextures(n,textures);
}

void EngineData::exec_glDeleteBuffers(uint32_t size, uint32_t* buffers)
{
	glDeleteBuffers(size,buffers);
}

void EngineData::exec_glDeleteFramebuffers(uint32_t size, uint32_t* buffers)
{
	glDeleteFramebuffers(size,buffers);
}

void EngineData::exec_glDeleteRenderbuffers(uint32_t size, uint32_t* buffers)
{
	glDeleteRenderbuffers(size,buffers);
}

void EngineData::exec_glBlendFunc(BLEND_FACTOR src, BLEND_FACTOR dst)
{
	GLenum glsrc, gldst;
	switch (src)
	{
		case BLEND_ONE: glsrc = GL_ONE; break;
		case BLEND_ZERO: glsrc = GL_ZERO; break;
		case BLEND_SRC_ALPHA: glsrc = GL_SRC_ALPHA; break;
		case BLEND_SRC_COLOR: glsrc = GL_SRC_COLOR; break;
		case BLEND_DST_ALPHA: glsrc = GL_DST_ALPHA; break;
		case BLEND_DST_COLOR: glsrc = GL_DST_COLOR; break;
		case BLEND_ONE_MINUS_SRC_ALPHA: glsrc = GL_ONE_MINUS_SRC_ALPHA; break;
		case BLEND_ONE_MINUS_SRC_COLOR: glsrc = GL_ONE_MINUS_SRC_COLOR; break;
		case BLEND_ONE_MINUS_DST_ALPHA: glsrc = GL_ONE_MINUS_DST_ALPHA; break;
		case BLEND_ONE_MINUS_DST_COLOR: glsrc = GL_ONE_MINUS_DST_COLOR; break;
		default:
			LOG(LOG_ERROR,"invalid src in glBlendFunc:"<<(uint32_t)src);
			return;
	}
	switch (dst)
	{
		case BLEND_ONE: gldst = GL_ONE; break;
		case BLEND_ZERO: gldst = GL_ZERO; break;
		case BLEND_SRC_ALPHA: gldst = GL_SRC_ALPHA; break;
		case BLEND_SRC_COLOR: gldst = GL_SRC_COLOR; break;
		case BLEND_DST_ALPHA: gldst = GL_DST_ALPHA; break;
		case BLEND_DST_COLOR: gldst = GL_DST_COLOR; break;
		case BLEND_ONE_MINUS_SRC_ALPHA: gldst = GL_ONE_MINUS_SRC_ALPHA; break;
		case BLEND_ONE_MINUS_SRC_COLOR: gldst = GL_ONE_MINUS_SRC_COLOR; break;
		case BLEND_ONE_MINUS_DST_ALPHA: gldst = GL_ONE_MINUS_DST_ALPHA; break;
		case BLEND_ONE_MINUS_DST_COLOR: gldst = GL_ONE_MINUS_DST_COLOR; break;
		default:
			LOG(LOG_ERROR,"invalid dst in glBlendFunc:"<<(uint32_t)dst);
			return;
	}
	glBlendFunc(glsrc, gldst);
}

void EngineData::exec_glCullFace(TRIANGLE_FACE mode)
{
	switch (mode)
	{
		case FACE_BACK:
			glEnable(GL_CULL_FACE);
			glCullFace(GL_BACK);
			break;
		case FACE_FRONT:
			glEnable(GL_CULL_FACE);
			glCullFace(GL_FRONT);
			break;
		case FACE_FRONT_AND_BACK:
			glEnable(GL_CULL_FACE);
			glCullFace(GL_FRONT_AND_BACK);
			break;
		case FACE_NONE:
			glDisable(GL_CULL_FACE);
			break;
	}
}

void EngineData::exec_glActiveTexture_GL_TEXTURE0(uint32_t textureindex)
{
	glActiveTexture(GL_TEXTURE0+textureindex);
}

void EngineData::exec_glGenBuffers(uint32_t size, uint32_t* buffers)
{
	glGenBuffers(size,buffers);
}

void EngineData::exec_glUseProgram(uint32_t program)
{
	glUseProgram(program);
}
void EngineData::exec_glDeleteProgram(uint32_t program)
{
	glDeleteProgram(program);
}

int32_t EngineData::exec_glGetUniformLocation(uint32_t program,const char* name)
{
	return glGetUniformLocation(program,name);
}

void EngineData::exec_glUniform1i(int32_t location,int32_t v0)
{
	glUniform1i(location,v0);
}

void EngineData::exec_glUniform4fv(int32_t location,uint32_t count, float* v0)
{
	glUniform4fv(location,count,v0);
}

void EngineData::exec_glGenTextures(int32_t n,uint32_t* textures)
{
	glGenTextures(n,textures);
}

void EngineData::exec_glViewport(int32_t x,int32_t y,int32_t width,int32_t height)
{
	glViewport(x,y,width,height);
	uint32_t code = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (code != GL_FRAMEBUFFER_COMPLETE)
		LOG(LOG_ERROR,"invalid framebuffer:"<<hex<<code);
}

void EngineData::exec_glBufferData_GL_ELEMENT_ARRAY_BUFFER_GL_STATIC_DRAW(int32_t size,const void* data)
{
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,size, data,GL_STATIC_DRAW);
}
void EngineData::exec_glBufferData_GL_ELEMENT_ARRAY_BUFFER_GL_DYNAMIC_DRAW(int32_t size,const void* data)
{
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,size, data,GL_DYNAMIC_DRAW);
}
void EngineData::exec_glBufferData_GL_ARRAY_BUFFER_GL_STATIC_DRAW(int32_t size,const void* data)
{
	glBufferData(GL_ARRAY_BUFFER,size, data,GL_STATIC_DRAW);
}
void EngineData::exec_glBufferData_GL_ARRAY_BUFFER_GL_DYNAMIC_DRAW(int32_t size,const void* data)
{
	glBufferData(GL_ARRAY_BUFFER,size, data,GL_DYNAMIC_DRAW);
}

void EngineData::exec_glTexParameteri_GL_TEXTURE_2D_GL_TEXTURE_MIN_FILTER_GL_LINEAR()
{
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}
void EngineData::exec_glTexParameteri_GL_TEXTURE_2D_GL_TEXTURE_MAG_FILTER_GL_LINEAR()
{
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}
void EngineData::exec_glTexParameteri_GL_TEXTURE_2D_GL_TEXTURE_MIN_FILTER_GL_NEAREST()
{
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
}
void EngineData::exec_glTexParameteri_GL_TEXTURE_2D_GL_TEXTURE_MAG_FILTER_GL_NEAREST()
{
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}
void EngineData::exec_glSetTexParameters(int32_t lodbias, uint32_t dimension, uint32_t filter, uint32_t mipmap, uint32_t wrap)
{
	switch (mipmap)
	{
		case 0: // disable
			glTexParameteri(dimension ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter ? GL_LINEAR : GL_NEAREST);
			break;
		case 1: // nearest
			glTexParameteri(dimension ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter ? GL_NEAREST_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_NEAREST);
			break;
		case 2: // linear
			glTexParameteri(dimension ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR_MIPMAP_NEAREST);
			break;
	}
	glTexParameteri(dimension ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter ? GL_LINEAR : GL_NEAREST);
	glTexParameteri(dimension ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap&1 ? GL_REPEAT : GL_CLAMP_TO_EDGE);
	glTexParameteri(dimension ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap&2 ? GL_REPEAT : GL_CLAMP_TO_EDGE);
#if defined(ENABLE_GLES2) || defined(ENABLE_GLES3)
	if (lodbias != 0)
		LOG(LOG_NOT_IMPLEMENTED,"Context3D: GL_TEXTURE_LOD_BIAS not available for OpenGL ES");
#else
	glTexParameterf(dimension ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS,(float)(lodbias)/8.0);
#endif
}


void EngineData::exec_glTexImage2D_GL_TEXTURE_2D_GL_UNSIGNED_BYTE(int32_t level,int32_t width, int32_t height,int32_t border, const void* pixels, bool hasalpha)
{
	glTexImage2D(GL_TEXTURE_2D, level, hasalpha ? GL_RGBA8 : GL_RGB, width, height, border, hasalpha ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, pixels);
}
void EngineData::exec_glTexImage2D_GL_TEXTURE_2D_GL_UNSIGNED_INT_8_8_8_8_HOST(int32_t level,int32_t width, int32_t height,int32_t border, const void* pixels)
{
#ifdef ENABLE_GLES3
	glTexImage2D(GL_TEXTURE_2D, level, GL_RGBA8, width, height, border, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_HOST, pixels);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_BLUE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
#else
	glTexImage2D(GL_TEXTURE_2D, level, GL_RGBA8, width, height, border, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_HOST, pixels);
#endif
}
void EngineData::glTexImage2Dintern(uint32_t type,int32_t level,int32_t width, int32_t height,int32_t border, void* pixels, TEXTUREFORMAT format, TEXTUREFORMAT_COMPRESSED compressedformat,uint32_t compressedImageSize)
{
	switch (format)
	{
		case TEXTUREFORMAT::BGRA:
#ifdef ENABLE_GLES3
			glTexImage2D(type, level, GL_RGBA8, width, height, border, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
			glTexParameteri(type, GL_TEXTURE_SWIZZLE_R, GL_BLUE);
			glTexParameteri(type, GL_TEXTURE_SWIZZLE_B, GL_RED);
#else
			glTexImage2D(type, level, GL_RGBA8, width, height, border, GL_BGRA, GL_UNSIGNED_BYTE, pixels);
#endif
			break;
		case TEXTUREFORMAT::BGR:
#if ENABLE_GLES2
			for (int i = 0; i < width*height*3; i += 3) {
				uint8_t t = ((uint8_t*)pixels)[i];
				((uint8_t*)pixels)[i] = ((uint8_t*)pixels)[i+2];
				((uint8_t*)pixels)[i+2] = t;
			}
			glTexImage2D(type, level, GL_RGB, width, height, border, GL_RGB, GL_UNSIGNED_BYTE, pixels);
#elif ENABLE_GLES3
			glTexImage2D(type, level, GL_RGB, width, height, border, GL_RGB, GL_UNSIGNED_BYTE, pixels);
			glTexParameteri(type, GL_TEXTURE_SWIZZLE_R, GL_BLUE);
			glTexParameteri(type, GL_TEXTURE_SWIZZLE_B, GL_RED);
#else
			glTexImage2D(type, level, GL_RGB, width, height, border, GL_BGR, GL_UNSIGNED_BYTE, pixels);
#endif
			break;
		case TEXTUREFORMAT::BGRA_PACKED:
#ifdef ENABLE_GLES3
			glTexImage2D(type, level, GL_RGBA8, width, height, border, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, pixels);
			glTexParameteri(type, GL_TEXTURE_SWIZZLE_R, GL_BLUE);
			glTexParameteri(type, GL_TEXTURE_SWIZZLE_B, GL_RED);
#else
			glTexImage2D(type, level, GL_RGBA8, width, height, border, GL_BGRA, GL_UNSIGNED_SHORT_4_4_4_4, pixels);
#endif
			break;
		case TEXTUREFORMAT::BGR_PACKED:
#if ENABLE_GLES2
			LOG(LOG_NOT_IMPLEMENTED,"textureformat BGR_PACKED for opengl es");
#elif ENABLE_GLES3
			glTexImage2D(type, level, GL_RGB, width, height, border, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, pixels);
			glTexParameteri(type, GL_TEXTURE_SWIZZLE_R, GL_BLUE);
			glTexParameteri(type, GL_TEXTURE_SWIZZLE_B, GL_RED);
			#else
			glTexImage2D(type, level, GL_RGB, width, height, border, GL_BGR, GL_UNSIGNED_SHORT_5_6_5, pixels);
#endif
			break;
		case TEXTUREFORMAT::COMPRESSED:
		case TEXTUREFORMAT::COMPRESSED_ALPHA:
		{
			switch (compressedformat)
			{
				case TEXTUREFORMAT_COMPRESSED::DXT5:
					glCompressedTexImage2D(type, level, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, width, height, border, compressedImageSize, pixels);
					break;
				case TEXTUREFORMAT_COMPRESSED::DXT1:
					glCompressedTexImage2D(type, level, GL_COMPRESSED_RGB_S3TC_DXT1_EXT, width, height, border, compressedImageSize, pixels);
					break;
				default:
					LOG(LOG_NOT_IMPLEMENTED,"upload texture in compressed format "<<compressedformat);
					break;
			}
			break;
		}
		case TEXTUREFORMAT::RGBA_HALF_FLOAT:
			LOG(LOG_NOT_IMPLEMENTED,"upload texture in format "<<format);
			break;
		default:
			LOG(LOG_ERROR,"invalid format for upload texture:"<<format);
			break;
	}
}
void EngineData::exec_glTexImage2D_GL_TEXTURE_2D(int32_t level,int32_t width, int32_t height,int32_t border, void* pixels, TEXTUREFORMAT format, TEXTUREFORMAT_COMPRESSED compressedformat,uint32_t compressedImageSize, bool isRectangleTexture)
{
#if defined(ENABLE_GLES2) || defined(ENABLE_GLES3)
	glTexImage2Dintern(GL_TEXTURE_2D ,level, width, height, border, pixels, format, compressedformat,compressedImageSize);
#else
	glTexImage2Dintern(isRectangleTexture ? GL_TEXTURE_RECTANGLE : GL_TEXTURE_2D,level, width, height, border, pixels, format, compressedformat,compressedImageSize);
#endif
}
void EngineData::exec_glDrawBuffer_GL_BACK()
{
	glDrawBuffer(GL_BACK);
}

void EngineData::exec_glClearColor(float red,float green,float blue,float alpha)
{
	glClearColor(red,green,blue,alpha);
}

void EngineData::exec_glClearStencil(uint32_t stencil)
{
	glClearStencil(stencil);
}
void EngineData::exec_glClearDepthf(float depth)
{
#if defined(ENABLE_GLES2) || defined(ENABLE_GLES3)
	glClearDepthf(depth);
#else
	// glClearDepthf is not available in OpenGL2
	glClearDepth(depth);
#endif
}

void EngineData::exec_glClear_GL_COLOR_BUFFER_BIT()
{
	glClear(GL_COLOR_BUFFER_BIT);
}
void EngineData::exec_glClear(CLEARMASK mask)
{
	uint32_t clearmask = 0;
	if ((mask & CLEARMASK::COLOR) != 0)
		clearmask |= GL_COLOR_BUFFER_BIT;
	if ((mask & CLEARMASK::DEPTH) != 0)
		clearmask |= GL_DEPTH_BUFFER_BIT;
	if ((mask & CLEARMASK::STENCIL) != 0)
		clearmask |= GL_STENCIL_BUFFER_BIT;
	glClear(clearmask);
}

void EngineData::exec_glDepthMask(bool flag)
{
	glDepthMask(flag);
}

void EngineData::exec_glTexSubImage2D_GL_TEXTURE_2D(int32_t level, int32_t xoffset, int32_t yoffset, int32_t width, int32_t height, const void* pixels)
{
#ifndef ENABLE_GLES3
	glTexSubImage2D(GL_TEXTURE_2D, level, xoffset, yoffset, width, height, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_HOST, pixels);
#else
	glTexSubImage2D(GL_TEXTURE_2D, level, xoffset, yoffset, width, height, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_HOST, pixels);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_BLUE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
#endif
}
void EngineData::exec_glGetIntegerv_GL_MAX_TEXTURE_SIZE(int32_t* data)
{
	glGetIntegerv(GL_MAX_TEXTURE_SIZE,data);
}

void EngineData::exec_glGenerateMipmap_GL_TEXTURE_2D()
{
	glGenerateMipmap(GL_TEXTURE_2D);
}
void EngineData::exec_glGenerateMipmap_GL_TEXTURE_CUBE_MAP()
{
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
}

void EngineData::exec_glReadPixels(int32_t width, int32_t height, void *buf)
{
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadPixels(0,0,width, height, GL_RGB, GL_UNSIGNED_BYTE, buf);
}
void EngineData::exec_glReadPixels_GL_BGRA(int32_t width, int32_t height,void *buf)
{
#ifndef ENABLE_GLES3
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadPixels(0,0,width, height, GL_BGRA, GL_UNSIGNED_BYTE, buf);
#else
	glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, buf);

	unsigned char* pixels = reinterpret_cast<unsigned char*>(buf);
	for (int i = 0; i < width * height; i++) {
		std::swap(pixels[i * 4], pixels[i * 4 + 2]);
	}
#endif
}

void EngineData::exec_glGetTexImage_GL_TEXTURE_2D(void *buf)
{
#if defined(ENABLE_GLES2) || defined(ENABLE_GLES3)
	LOG(LOG_ERROR,"glGetTexImage not available for OpenGL ES");
#else
	glGetTexImage(GL_TEXTURE_2D,0,GL_BGRA, GL_UNSIGNED_BYTE, buf);
#endif
}
void EngineData::exec_glBindTexture_GL_TEXTURE_CUBE_MAP(uint32_t id)
{
	glBindTexture(GL_TEXTURE_CUBE_MAP, id);
}
void EngineData::exec_glTexParameteri_GL_TEXTURE_CUBE_MAP_GL_TEXTURE_MIN_FILTER_GL_LINEAR()
{
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}
void EngineData::exec_glTexParameteri_GL_TEXTURE_CUBE_MAP_GL_TEXTURE_MAG_FILTER_GL_LINEAR()
{
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}
void EngineData::exec_glTexImage2D_GL_TEXTURE_CUBE_MAP_POSITIVE_X_GL_UNSIGNED_BYTE(uint32_t side, int32_t level, int32_t width, int32_t height, int32_t border, void* pixels, TEXTUREFORMAT format, TEXTUREFORMAT_COMPRESSED compressedformat, uint32_t compressedImageSize)
{
	glTexImage2Dintern(GL_TEXTURE_CUBE_MAP_POSITIVE_X+side, level, width, height, border, pixels, format, compressedformat,compressedImageSize);
}

void EngineData::exec_glScissor(int32_t x, int32_t y, int32_t width, int32_t height)
{
	glEnable(GL_SCISSOR_TEST);
	glScissor(x,y,width,height);
}
void EngineData::exec_glDisable_GL_SCISSOR_TEST()
{
	glDisable(GL_SCISSOR_TEST);
}

void EngineData::exec_glColorMask(bool red, bool green, bool blue, bool alpha)
{
	glColorMask(red,green,blue,alpha);
}

void EngineData::exec_glStencilFunc_GL_ALWAYS()
{
	glStencilFunc(GL_ALWAYS, 0, 0xff);
}

void EngineData::exec_glStencilFunc_GL_NEVER()
{
	glStencilFunc(GL_NEVER, 0, 0xff);
}

void EngineData::exec_glStencilFunc_GL_EQUAL(int32_t ref, uint32_t mask)
{
	glStencilFunc(GL_EQUAL, ref, mask);
}

void EngineData::exec_glStencilOp_GL_INCR()
{
	glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
}

void EngineData::exec_glStencilOp_GL_DECR()
{
	glStencilOp(GL_KEEP, GL_KEEP, GL_DECR);
}

void EngineData::exec_glStencilOp_GL_KEEP()
{
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
}

void EngineData::exec_glStencilOpSeparate(TRIANGLE_FACE face, DEPTHSTENCIL_OP sfail, DEPTHSTENCIL_OP dpfail, DEPTHSTENCIL_OP dppass)
{
	GLenum glface=GL_FRONT;
	switch (face)
	{
		case FACE_BACK:
			glface=GL_BACK;
			break;
		case FACE_FRONT:
			glface=GL_FRONT;
			break;
		case FACE_FRONT_AND_BACK:
			glface=GL_FRONT_AND_BACK;
			break;
		case FACE_NONE:
			glface=GL_NONE;
			break;
	}
	GLenum glsfail=GL_KEEP;
	switch (sfail)
	{
		case DEPTHSTENCIL_KEEP:
			glsfail=GL_KEEP;
			break;
		case DEPTHSTENCIL_ZERO:
			glsfail=GL_ZERO;
			break;
		case DEPTHSTENCIL_REPLACE:
			glsfail=GL_REPLACE;
			break;
		case DEPTHSTENCIL_INCR:
			glsfail=GL_INCR;
			break;
		case DEPTHSTENCIL_INCR_WRAP:
			glsfail=GL_INCR_WRAP;
			break;
		case DEPTHSTENCIL_DECR:
			glsfail=GL_DECR;
			break;
		case DEPTHSTENCIL_DECR_WRAP:
			glsfail=GL_DECR_WRAP;
			break;
		case DEPTHSTENCIL_INVERT:
			glsfail=GL_INVERT;
			break;
	}
	GLenum gldpfail=GL_KEEP;
	switch (dpfail)
	{
		case DEPTHSTENCIL_KEEP:
			gldpfail=GL_KEEP;
			break;
		case DEPTHSTENCIL_ZERO:
			gldpfail=GL_ZERO;
			break;
		case DEPTHSTENCIL_REPLACE:
			gldpfail=GL_REPLACE;
			break;
		case DEPTHSTENCIL_INCR:
			gldpfail=GL_INCR;
			break;
		case DEPTHSTENCIL_INCR_WRAP:
			gldpfail=GL_INCR_WRAP;
			break;
		case DEPTHSTENCIL_DECR:
			gldpfail=GL_DECR;
			break;
		case DEPTHSTENCIL_DECR_WRAP:
			gldpfail=GL_DECR_WRAP;
			break;
		case DEPTHSTENCIL_INVERT:
			gldpfail=GL_INVERT;
			break;
	}
	GLenum gldppass=GL_KEEP;
	switch (dppass)
	{
		case DEPTHSTENCIL_KEEP:
			gldppass=GL_KEEP;
			break;
		case DEPTHSTENCIL_ZERO:
			gldppass=GL_ZERO;
			break;
		case DEPTHSTENCIL_REPLACE:
			gldppass=GL_REPLACE;
			break;
		case DEPTHSTENCIL_INCR:
			gldppass=GL_INCR;
			break;
		case DEPTHSTENCIL_INCR_WRAP:
			gldppass=GL_INCR_WRAP;
			break;
		case DEPTHSTENCIL_DECR:
			gldppass=GL_DECR;
			break;
		case DEPTHSTENCIL_DECR_WRAP:
			gldppass=GL_DECR_WRAP;
			break;
		case DEPTHSTENCIL_INVERT:
			gldppass=GL_INVERT;
			break;
	}
	glStencilOpSeparate(glface, glsfail, gldpfail, gldppass);
}

void EngineData::exec_glStencilMask(uint32_t mask)
{
	glStencilMask(mask);
}

void EngineData::exec_glStencilFunc(DEPTHSTENCIL_FUNCTION func, uint32_t ref, uint32_t mask)
{
	uint32_t f = GL_ALWAYS;
	switch (func)
	{
		case DEPTHSTENCIL_ALWAYS:
			f = GL_ALWAYS;
			break;
		case DEPTHSTENCIL_EQUAL:
			f = GL_EQUAL;
			break;
		case DEPTHSTENCIL_GREATER:
			f = GL_GREATER;
			break;
		case DEPTHSTENCIL_GREATER_EQUAL:
			f = GL_GEQUAL;
			break;
		case DEPTHSTENCIL_LESS:
			f = GL_LESS;
			break;
		case DEPTHSTENCIL_LESS_EQUAL:
			f = GL_LEQUAL;
			break;
		case DEPTHSTENCIL_NEVER:
			f = GL_NEVER;
			break;
		case DEPTHSTENCIL_NOT_EQUAL:
			f = GL_NOTEQUAL;
			break;
	}
	glStencilFunc(f,ref,mask);
}

void audioCallback(void * userdata, uint8_t * stream, int len)
{
	AudioManager* manager = (AudioManager*)userdata;

	SDL_memset(stream, 0, len);

	{
		Locker l(manager->streamMutex);
		for (auto it = manager->streams.begin(); it != manager->streams.end(); it++)
		{
			AudioStream* s = (*it);
			if (s->ispaused())
				continue;
			s->startMixing();
			const float fmaxvolume = 1.0f / ((float)SDL_MIX_MAXVOLUME);
			const float fvolume = (float)s->getVolume()*(float)SDL_MIX_MAXVOLUME;
			uint32_t readcount = 0;
			uint8_t* buf = new uint8_t[len];
			while (readcount < ((uint32_t)len))
			{
				uint32_t ret = s->getDecoder()->copyFrameF32((float *)(buf+readcount), ((uint32_t)len)-readcount);
				if (!ret)
					break;
				float* src32=(float *)(buf+readcount);
				float* dst32=(float *)(stream+readcount);
				readcount += ret;

				float src1left, src1right, src2;
				double dst_sample;

				const double max_audioval = 3.402823466e+38F;
				const double min_audioval = -3.402823466e+38F;
				float* panning = s->getPanning();
				int curpanning=0;
				while (ret)
				{
					src1left = *src32 * fvolume * fmaxvolume * panning[curpanning];
					src1right = *(src32+1) * fvolume * fmaxvolume * panning[curpanning+1];
					curpanning = 2-curpanning;
					if (!curpanning)
						src32+=2;
					src2 = *dst32;
					dst_sample = double(src1left) + double(src1right) + double(src2);
					if (dst_sample > max_audioval) {
						dst_sample = max_audioval;
					} else if (dst_sample < min_audioval) {
						dst_sample = min_audioval;
					}
					*(dst32++) = (float)dst_sample;
					ret-=4;
				}
			}
			delete[] buf;
		}
    }
}

int EngineData::audio_StreamInit(AudioStream* s)
{
	return 0;
}

void EngineData::audio_StreamPause(int channel, bool dopause)
{
}

void EngineData::audio_StreamDeinit(int channel)
{
}

bool EngineData::audio_ManagerInit()
{
	bool sdl_available = false;
	if (SDL_WasInit(0)) // some part of SDL already was initialized
		sdl_available = !SDL_InitSubSystem ( SDL_INIT_AUDIO );
	else
		sdl_available = !SDL_Init ( SDL_INIT_AUDIO );
	return sdl_available;
}

void EngineData::audio_ManagerCloseMixer(AudioManager* manager)
{
	if (manager->device)
	{
		SDL_PauseAudioDevice(manager->device, 1);
		SDL_CloseAudioDevice(manager->device);
		manager->device=0;
	}
}

bool EngineData::audio_ManagerOpenMixer(AudioManager* manager)
{
	SDL_AudioSpec spec;
	spec.freq=audio_getSampleRate();
	spec.format=AUDIO_F32SYS;
	spec.channels = 2;
	spec.samples = LIGHTSPARK_AUDIO_BUFFERSIZE/2;
	spec.callback = audioCallback;
	spec.userdata = manager;

	manager->device = SDL_OpenAudioDevice(nullptr, 0, &spec, nullptr, 0);
	if (manager->device != 0)
		SDL_PauseAudioDevice(manager->device, 0);
	return manager->device!=0;
}

void EngineData::audio_ManagerDeinit()
{
	SDL_QuitSubSystem ( SDL_INIT_AUDIO );
	if (!SDL_WasInit(0))
		SDL_Quit ();
}
bool EngineData::audio_useFloatSampleFormat()
{
	return true;
}

int EngineData::audio_getSampleRate()
{
	return 44100;
}
IDrawable *EngineData::getTextRenderDrawable(const TextData &_textData, const MATRIX &_m, int32_t _x, int32_t _y, int32_t _w, int32_t _h, float _xs, float _ys, bool _isMask, bool _cacheAsBitmap, float _s, float _a, const ColorTransformBase& _colortransform, SMOOTH_MODE smoothing,AS_BLENDMODE _blendmode)
{
	if (hasExternalFontRenderer)
		return new externalFontRenderer(_textData,this, _x, _y, _w, _h,_xs,_ys,_isMask, _cacheAsBitmap, _a,
										_colortransform, smoothing,_blendmode,_m);
	return nullptr;
}

externalFontRenderer::externalFontRenderer(const TextData &_textData, EngineData *engine, int32_t x, int32_t y, int32_t w, int32_t h, float xs, float ys, bool _isMask, bool _cacheAsBitmap, float a,
										   const ColorTransformBase& _colortransform, SMOOTH_MODE smoothing,AS_BLENDMODE _blendmode, const MATRIX &_m)
	: IDrawable(w, h, x, y,xs,ys, 1, 1, _isMask, _cacheAsBitmap, 1.0, a,
				_colortransform,smoothing,_blendmode,_m),m_engine(engine)
{
	externalressource = engine->setupFontRenderer(_textData,a,smoothing);
}

uint8_t *externalFontRenderer::getPixelBuffer(bool *isBufferOwner, uint32_t* bufsize)
{
	if (isBufferOwner)
		*isBufferOwner = true;
	if (bufsize)
		*bufsize=width*height*4;
	return m_engine->getFontPixelBuffer(externalressource,this->width,this->height);
}
