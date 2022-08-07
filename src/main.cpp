/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2008-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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

#include <glib.h>
#include "version.h"
#include "backends/security.h"
#include "backends/config.h"
#include "swf.h"
#include "logger.h"
#include "platforms/engineutils.h"
#include "compat.h"
#include <SDL2/SDL.h>
#include "flash/utils/ByteArray.h"
#include <sys/stat.h>
#include "parsing/streams.h"

#ifdef __MINGW32__
    #ifndef PATH_MAX
    #define PATH_MAX _MAX_PATH
    #endif
    #define realpath(N,R) _fullpath((R),(N),_MAX_PATH)
#endif
using namespace std;
using namespace lightspark;

class StandaloneEngineData: public EngineData
{
	SDL_GLContext mSDLContext;
	char* mBaseDir;
	tiny_string mApplicationStoragePath;
	void removedir(const char* dir)
	{
		GDir* d = g_dir_open(dir,0,nullptr);
		if (!d)
			return;
		while (true)
		{
			const char* filename = g_dir_read_name (d);
			if (!filename)
			{
				g_dir_close(d);
				return;
			}
			string path = dir;
			path += G_DIR_SEPARATOR_S;
			path += filename;
			struct stat st;
			stat(path.c_str(), &st);
			if(st.st_mode & S_IFDIR)
				removedir(path.c_str());
			else
				::remove(path.c_str());
		}
		g_dir_close(d);
		removedir(dir);
	}
	bool isvalidfilename(const tiny_string& filename)
	{
		if (filename.find("./") != tiny_string::npos || filename == ".." ||
				filename.find("~") != tiny_string::npos)
			return false;
		return true;
	}
	size_t getfilesize(const char* filepath)
	{
		struct stat st;
		stat(filepath, &st);
		return st.st_size;
	}
public:
	StandaloneEngineData(const tiny_string& filedatapath)
	{
		sharedObjectDatapath = Config::getConfig()->getCacheDirectory();
		sharedObjectDatapath += G_DIR_SEPARATOR_S;
		sharedObjectDatapath += "data";
		sharedObjectDatapath += filedatapath;

		mApplicationStoragePath = Config::getConfig()->getCacheDirectory();
		mApplicationStoragePath += filedatapath;
		mApplicationStoragePath += G_DIR_SEPARATOR_S;
		mApplicationStoragePath += "appstorage";
		mBaseDir = g_get_current_dir();
	}
	~StandaloneEngineData()
	{
		removedir(Config::getConfig()->getDataDirectory().c_str());
		if (mBaseDir)
			g_free(mBaseDir);
	}
	SDL_Window* createWidget(uint32_t w, uint32_t h) override
	{
		SDL_Window* window = SDL_CreateWindow("Lightspark",SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,w,h,SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
		if (window == 0)
		{
			// try creation of window without opengl support
			window = SDL_CreateWindow("Lightspark",SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,w,h,SDL_WINDOW_RESIZABLE);
			if (window == 0)
				LOG(LOG_ERROR,"createWidget failed:"<<SDL_GetError());
		}
		return window;
	}

	uint32_t getWindowForGnash() override
	{
		/* passing and invalid window id to gnash makes
		 * it create its own window */
		return 0;
	}
	void stopMainDownload() override {}
	bool isSizable() const override
	{
		return true;
	}
	void grabFocus() override
	{
		/* Nothing to do because the standalone main window is
		 * always focused */
	}
	void openPageInBrowser(const tiny_string& url, const tiny_string& window) override
	{
		LOG(LOG_NOT_IMPLEMENTED, "openPageInBrowser not implemented in the standalone mode");
	}
	bool getScreenData(SDL_DisplayMode* screen) override
	{
		if (SDL_GetDesktopDisplayMode(0, screen) != 0) {
			LOG(LOG_ERROR,"Capabilities: SDL_GetDesktopDisplayMode failed:"<<SDL_GetError());
			return false;
		}
		return true;
	}
	double getScreenDPI() override
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
	void DoSwapBuffers() override
	{
		uint32_t err;
		if (getGLError(err))
			LOG(LOG_ERROR,"swapbuffers:"<<widget<<" "<<err);
		SDL_GL_SwapWindow(widget);
	}
	void InitOpenGL() override
	{
		mSDLContext = SDL_GL_CreateContext(widget);
		if (!mSDLContext)
			LOG(LOG_ERROR,"failed to create openGL context:"<<SDL_GetError());
		initGLEW();
	}
	void DeinitOpenGL() override
	{
		SDL_GL_DeleteContext(mSDLContext);
	}

	bool FileExists(SystemState* sys,const tiny_string& filename, bool isfullpath) override
	{
		if (!isvalidfilename(filename))
			return false;
		tiny_string p = isfullpath ? filename : FileFullPath(sys,filename);
		if (p.empty())
			return false;
		return g_file_test(p.raw_buf(),G_FILE_TEST_EXISTS);
	}

	uint32_t FileSize(SystemState* sys,const tiny_string& filename, bool isfullpath) override
	{
		if (!isvalidfilename(filename))
			return 0;
		tiny_string p = isfullpath ? filename : FileFullPath(sys,filename);
		if (p.empty())
			return 0;
		return getfilesize(p.raw_buf());
	}
	
	tiny_string FileFullPath(SystemState* sys, const tiny_string& filename) override
	{
		std::string p;
		if (sys->flashMode == SystemState::FLASH_MODE::AIR && mBaseDir)
		{
			if (!g_path_is_absolute(filename.raw_buf()))
				p = mBaseDir;
		}
		else
			p = Config::getConfig()->getDataDirectory();
		if (p.empty())
			return "";
		p += G_DIR_SEPARATOR_S;
		p += filename.raw_buf();
		return p;
	}

	tiny_string FileRead(SystemState* sys,const tiny_string& filename, bool isfullpath) override
	{
		if (!isvalidfilename(filename))
			return "";
		tiny_string p = isfullpath ? filename : FileFullPath(sys,filename);
		if (p.empty())
			return "";

		if (!g_file_test(p.raw_buf(),G_FILE_TEST_EXISTS))
			return "";
		uint32_t len = getfilesize(p.raw_buf());
		std::ifstream file;
		
		file.open(p.raw_buf(), std::ios::in|std::ios::binary);
		
		tiny_string res(file,len);
		file.close();
		return res;
	}
	void FileWrite(SystemState* sys,const tiny_string& filename, const tiny_string& data, bool isfullpath) override
	{
		if (!isvalidfilename(filename))
			return;
		tiny_string p = isfullpath ? filename : FileFullPath(sys,filename);
		if (p.empty())
			return;
		std::ofstream file;

		file.open(p.raw_buf(), std::ios::out|std::ios::binary);
		file << data;
		file.close();
	}
	uint8_t FileReadUnsignedByte(SystemState* sys, const tiny_string& filename, uint32_t startpos, bool isfullpath) override
	{
		if (!isvalidfilename(filename))
			return 0;
		tiny_string p = isfullpath ? filename : FileFullPath(sys,filename);
		if (p.empty())
			return 0;
		if (!g_file_test(p.raw_buf(),G_FILE_TEST_EXISTS))
			return 0;
		std::ifstream file;
		file.open(p.raw_buf(), std::ios::in|std::ios::binary);
		file.seekg(startpos);
		uint8_t buf;
		file.read((char*)&buf,1);
		file.close();
		return buf;
	}
	void FileReadByteArray(SystemState* sys,const tiny_string &filename,ByteArray* res, uint32_t startpos, uint32_t length, bool isfullpath) override
	{
		if (!isvalidfilename(filename))
			return;
		tiny_string p = isfullpath ? filename : FileFullPath(sys,filename);
		if (p.empty())
			return;
		if (!g_file_test(p.raw_buf(),G_FILE_TEST_EXISTS))
			return;
		uint32_t len = min(length,uint32_t(getfilesize(p.raw_buf())-startpos));
		std::ifstream file;
		uint8_t* buf = new uint8_t[len];
		file.open(p.raw_buf(), std::ios::in|std::ios::binary);
		file.seekg(startpos);
		file.read((char*)buf,len);
		res->writeBytes(buf,len);
		file.close();
		delete[] buf;
	}
	
	void FileWriteByteArray(SystemState* sys,const tiny_string &filename, ByteArray *data, uint32_t startpos, uint32_t length, bool isfullpath) override
	{
		if (!isvalidfilename(filename))
			return;
		tiny_string p = isfullpath ? filename : FileFullPath(sys,filename);
		if (p.empty())
			return;
		std::ofstream file;
		
		file.open(p.raw_buf(), std::ios::out|std::ios::binary|std::ios::trunc);
		uint8_t* buf = data->getBuffer(data->getLength(),false);
		uint32_t len = length;
		if (length == UINT32_MAX || startpos+length > data->getLength())
			len = data->getLength()-startpos;
		file.write((char*)buf+startpos,len);
		file.close();
	}
	bool FileCreateDirectory(SystemState* sys,const tiny_string& filename, bool isfullpath) override
	{
		if (!isvalidfilename(filename))
			return false;
		tiny_string p = isfullpath ? filename : FileFullPath(sys,filename);
		if (p.empty())
			return false;
		if (!p.endsWith(G_DIR_SEPARATOR_S))
		{
			// if filename doesn't end with a directory separator, it is treated as a full path to a file
			gchar* dir = g_path_get_dirname(p.raw_buf());
			p=dir;
			g_free(dir);
		}
		return g_mkdir_with_parents(p.raw_buf(),0755) == 0;
	}
	bool FilePathIsAbsolute(const tiny_string& filename) override
	{
		return g_path_is_absolute(filename.raw_buf());
	}
	tiny_string FileGetApplicationStorageDir() override
	{
		return mApplicationStoragePath;
	}
};

int main(int argc, char* argv[])
{
	char* fileName=nullptr;
	char* url=nullptr;
	char* paramsFileName=nullptr;
#ifdef PROFILING_SUPPORT
	char* profilingFileName=nullptr;
#endif
	char *HTTPcookie=nullptr;
	SecurityManager::SANDBOXTYPE sandboxType=SecurityManager::LOCAL_WITH_FILE;
	bool useInterpreter=true;
	bool useFastInterpreter=false;
	bool useJit=false;
	bool ignoreUnhandledExceptions = false;
	bool startInFullScreenMode=false;
	double startscalefactor=1.0;
	SystemState::ERROR_TYPE exitOnError=SystemState::ERROR_PARSING;
	LOG_LEVEL log_level=LOG_INFO;
	SystemState::FLASH_MODE flashMode=SystemState::FLASH;
	std::vector<tiny_string> extensions;

	LOG(LOG_INFO,"Lightspark version " << VERSION << " Copyright 2009-2013 Alessandro Pignotti and others");

	for(int i=1;i<argc;i++)
	{
		if(strcmp(argv[i],"-u")==0 || 
			strcmp(argv[i],"--url")==0)
		{
			i++;
			if(i==argc)
			{
				fileName=nullptr;
				break;
			}

			url=argv[i];
		}
		else if(strcmp(argv[i],"--air")==0)
			flashMode=SystemState::AIR;
		else if(strcmp(argv[i],"--avmplus")==0)
			flashMode=SystemState::AVMPLUS;
		else if(strcmp(argv[i],"-ni")==0 || strcmp(argv[i],"--disable-interpreter")==0)
			useInterpreter=false;
		else if(strcmp(argv[i],"-fi")==0 || strcmp(argv[i],"--enable-fast-interpreter")==0)
			useFastInterpreter=true;
		else if(strcmp(argv[i],"-j")==0 || strcmp(argv[i],"--enable-jit")==0)
			useJit=true;
		else if(strcmp(argv[i],"-ne")==0 || strcmp(argv[i],"--ignore-unhandled-exceptions")==0)
			ignoreUnhandledExceptions=true;
		else if(strcmp(argv[i],"-fs")==0 || strcmp(argv[i],"--fullscreen")==0)
			startInFullScreenMode=true;
		else if(strcmp(argv[i],"-sc")==0 || strcmp(argv[i],"--scale")==0)
		{
			i++;
			if(i==argc)
			{
				fileName=nullptr;
				break;
			}
			startscalefactor = max(1.0,atof(argv[i]));
		}
		else if(strcmp(argv[i],"-l")==0 || strcmp(argv[i],"--log-level")==0)
		{
			i++;
			if(i==argc)
			{
				fileName=nullptr;
				break;
			}

			log_level=(LOG_LEVEL) min(4, max(0, atoi(argv[i])));
		}
		else if(strcmp(argv[i],"-p")==0 || 
			strcmp(argv[i],"--parameters-file")==0)
		{
			i++;
			if(i==argc)
			{
				fileName=nullptr;
				break;
			}
			paramsFileName=argv[i];
		}
		else if (strcmp(argv[i],"-le")==0 || 
				 strcmp(argv[i],"--load-extension")==0)
		{
			i++;
			if(i==argc)
			{
				fileName=nullptr;
				break;
			}
			tiny_string ext = argv[i];
			extensions.push_back(ext);
		}
#ifdef PROFILING_SUPPORT
		else if(strcmp(argv[i],"-o")==0 || 
			strcmp(argv[i],"--profiling-output")==0)
		{
			i++;
			if(i==argc)
			{
				fileName=NULL;
				break;
			}
			profilingFileName=argv[i];
		}
#endif
		else if(strcmp(argv[i],"-s")==0 || 
			strcmp(argv[i],"--security-sandbox")==0)
		{
			i++;
			if(i==argc)
			{
				fileName=nullptr;
				break;
			}
			if(strncmp(argv[i], "remote", 6) == 0)
				sandboxType = SecurityManager::REMOTE;
			else if(strncmp(argv[i], "local-with-filesystem", 21) == 0)
				sandboxType = SecurityManager::LOCAL_WITH_FILE;
			else if(strncmp(argv[i], "local-with-networking", 21) == 0)
				sandboxType = SecurityManager::LOCAL_WITH_NETWORK;
			else if(strncmp(argv[i], "local-trusted", 13) == 0)
				sandboxType = SecurityManager::LOCAL_TRUSTED;
		}
		else if(strcmp(argv[i],"-v")==0 || 
			strcmp(argv[i],"--version")==0)
		{
			exit(0);
		}
		else if(strcmp(argv[i],"--exit-on-error")==0)
		{
			exitOnError = SystemState::ERROR_ANY;
		}
		else if(strcmp(argv[i],"--disable-rendering")==0)
		{
			EngineData::enablerendering = false;
		}
		
		else if(strcmp(argv[i],"--HTTP-cookies")==0)
		{
			i++;
			if(i==argc)
			{
				fileName=nullptr;
				break;
			}
			HTTPcookie=argv[i];
		}
		else
		{
			//No options flag, so set the swf file name
			if(fileName) //If already set, exit in error status
			{
				fileName=nullptr;
				break;
			}
			fileName=argv[i];
		}
	}

	if(fileName==nullptr)
	{
		LOG(LOG_ERROR, "Usage: " << argv[0] << " [--url|-u http://loader.url/file.swf]" <<
			" [--disable-interpreter|-ni] [--enable-fast-interpreter|-fi]" <<
#ifdef LLVM_ENABLED
			" [--enable-jit|-j]" <<
#endif
			" [--log-level|-l 0-4] [--parameters-file|-p params-file] [--security-sandbox|-s sandbox]" <<
			" [--exit-on-error] [--HTTP-cookies cookie] [--air] [--avmplus] [--disable-rendering]" <<
#ifdef PROFILING_SUPPORT
			" [--profiling-output|-o profiling-file]" <<
#endif
			" [--ignore-unhandled-exceptions|-ne]"
			" [--fullscreen|-fs]"
			" [--scale|-sc]"
			" [--load-extension|-le extension-file"
			" [--version|-v]" <<
			" <file.swf>");
		exit(1);
	}

	Log::setLogLevel(log_level);
	lsfilereader r(fileName);
	istream f(&r);
	f.seekg(0, ios::end);
	uint32_t fileSize=f.tellg();
	f.seekg(0, ios::beg);
	if(!f)
	{
		LOG(LOG_ERROR, argv[0] << ": " << fileName << ": No such file or directory");
		exit(2);
	}
	f.exceptions ( istream::eofbit | istream::failbit | istream::badbit );
	cout.exceptions( ios::failbit | ios::badbit);
	cerr.exceptions( ios::failbit | ios::badbit);
	SystemState::staticInit();
	if (!EngineData::startSDLMain())
	{
		LOG(LOG_ERROR,"SDL initialization failed, aborting");
		SystemState::staticDeinit();
		exit(3);
	}
	//NOTE: see SystemState declaration
	SystemState* sys = new SystemState(fileSize, flashMode);
	ParseThread* pt = new ParseThread(f, sys->mainClip);
	pt->addExtensions(extensions);
	setTLSSys(sys);
	setTLSWorker(sys->worker);

	sys->setDownloadedPath(fileName);
	sys->allowFullscreen=true;
	sys->allowFullscreenInteractive=true;

	//This setting allows qualifying filename-only paths to fully qualified paths
	//When the URL parameter is set, set the root URL to the given parameter
	if(url)
	{
		sys->mainClip->setOrigin(url, fileName);
		sys->parseParametersFromURL(sys->mainClip->getOrigin());
		if (sandboxType != SecurityManager::REMOTE &&
		    sys->mainClip->getOrigin().getProtocol() != "file")
		{
			LOG(LOG_INFO, "Switching to remote sandbox because of remote url");
			sandboxType = SecurityManager::REMOTE;
		}
	}
#ifndef _WIN32
	//When running in a local sandbox, set the root URL to the current working dir
	else if(sandboxType != SecurityManager::REMOTE)
	{
		char * cwd = g_get_current_dir();
		string cwdStr = string("file://") + string(cwd);
		free(cwd);
		cwdStr += "/";
		sys->mainClip->setOrigin(cwdStr, fileName);
	}
#endif
	else
	{
		sys->mainClip->setOrigin(string("file://") + fileName);
		LOG(LOG_INFO, "Warning: running with no origin URL set.");
	}

	//One of useInterpreter or useJit must be enabled
	if(!(useInterpreter || useJit))
	{
		LOG(LOG_ERROR,"No execution model enabled");
		exit(1);
	}
	sys->useInterpreter=useInterpreter;
	sys->useFastInterpreter=useFastInterpreter;
	sys->useJit=useJit;
	sys->ignoreUnhandledExceptions=ignoreUnhandledExceptions;
	sys->exitOnError=exitOnError;
	if(paramsFileName)
		sys->parseParametersFromFile(paramsFileName);
#ifdef PROFILING_SUPPORT
	if(profilingFileName)
		sys->setProfilingOutput(profilingFileName);
#endif
	if(HTTPcookie)
		sys->setCookies(HTTPcookie);

	// create path for shared object local storage
	char absolutepath[PATH_MAX];
	if (realpath(fileName,absolutepath) == nullptr)
	{
		LOG(LOG_ERROR, "Unable to resolve file");
		exit(1);
	}
	tiny_string homedir(g_get_home_dir());
	tiny_string filedatapath = absolutepath;
	if (filedatapath.find(homedir) == 0) // remove home dir, if file is located below home dir
		filedatapath = filedatapath.substr_bytes(homedir.numBytes(),UINT32_MAX);

	sys->setParamsAndEngine(new StandaloneEngineData(filedatapath), true);
	// on standalone local storage is always allowed
	sys->getEngineData()->setLocalStorageAllowedMarker(true);
	sys->getEngineData()->startInFullScreenMode=startInFullScreenMode;
	sys->getEngineData()->startscalefactor=startscalefactor;

	sys->securityManager->setSandboxType(sandboxType);
	if(sandboxType == SecurityManager::REMOTE)
		LOG(LOG_INFO, "Running in remote sandbox");
	else if(sandboxType == SecurityManager::LOCAL_WITH_NETWORK)
		LOG(LOG_INFO, "Running in local-with-networking sandbox");
	else if(sandboxType == SecurityManager::LOCAL_WITH_FILE)
		LOG(LOG_INFO, "Running in local-with-filesystem sandbox");
	else if(sandboxType == SecurityManager::LOCAL_TRUSTED)
		LOG(LOG_INFO, "Running in local-trusted sandbox");

	sys->downloadManager=new StandaloneDownloadManager();

	//Start the parser
	sys->addJob(pt);

	/* Destroy blocks until the 'terminated' flag is set by
	 * SystemState::setShutdownFlag.
	 */
	sys->destroy();
	bool isonerror = sys->exitOnError==SystemState::ERROR_ANY && sys->isOnError();
	SDL_Event event;
	SDL_zero(event);
	event.type = LS_USEREVENT_QUIT;
	SDL_PushEvent(&event);
	SDL_WaitThread(EngineData::mainLoopThread,nullptr);

	delete pt;
	delete sys;

	SystemState::staticDeinit();
	
	return isonerror ? 1 : 0;
}

