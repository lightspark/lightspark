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

#include "version.h"
#include "backends/security.h"
#include "backends/config.h"
#include "swf.h"
#include "logger.h"
#include "platforms/engineutils.h"
#include "compat.h"
#include <SDL2/SDL.h>
#include "boost/filesystem.hpp"
#include "flash/utils/ByteArray.h"


using namespace std;
using namespace lightspark;

class StandaloneEngineData: public EngineData
{
	SDL_GLContext mSDLContext;
public:
	StandaloneEngineData()
	{
	}
	~StandaloneEngineData()
	{
		boost::filesystem::path p(Config::getConfig()->getDataDirectory());
		if (!p.empty())
			boost::filesystem::remove_all(p);
	}
	SDL_Window* createWidget(uint32_t w, uint32_t h)
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
	
	uint32_t getWindowForGnash()
	{
		/* passing and invalid window id to gnash makes
		 * it create its own window */
		return 0;
	}
	void stopMainDownload() {}
	bool isSizable() const
	{
		return true;
	}
	void grabFocus()
	{
		/* Nothing to do because the standalone main window is
		 * always focused */
	}
	void openPageInBrowser(const tiny_string& url, const tiny_string& window)
	{
		LOG(LOG_NOT_IMPLEMENTED, "openPageInBrowser not implemented in the standalone mode");
	}
	bool getScreenData(SDL_DisplayMode* screen)
	{
		if (SDL_GetDesktopDisplayMode(0, screen) != 0) {
			LOG(LOG_ERROR,"Capabilities: SDL_GetDesktopDisplayMode failed:"<<SDL_GetError());
			return false;
		}
		return true;
	}
	double getScreenDPI()
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
	void DoSwapBuffers()
	{
		uint32_t err;
		if (getGLError(err))
			LOG(LOG_ERROR,"swapbuffers:"<<widget<<" "<<err);
		SDL_GL_SwapWindow(widget);
	}
	void InitOpenGL()
	{
		mSDLContext = SDL_GL_CreateContext(widget);
		if (!mSDLContext)
			LOG(LOG_ERROR,"failed to create openGL context:"<<SDL_GetError());
		initGLEW();
	}
	void DeinitOpenGL()
	{
		SDL_GL_DeleteContext(mSDLContext);
	}
	bool FileExists(const tiny_string& filename)
	{
		if (!boost::filesystem::portable_file_name(filename))
			return false;
		boost::filesystem::path p(Config::getConfig()->getDataDirectory());
		if (p.empty())
			return false;
		p /= filename.raw_buf();
		return boost::filesystem::exists(p);
	}
	tiny_string FileRead(const tiny_string& filename)
	{
		if (!boost::filesystem::portable_file_name(filename))
			return "";
		boost::filesystem::path p(Config::getConfig()->getDataDirectory());
		if (p.empty())
			return "";
		p /= filename.raw_buf();
		if (!boost::filesystem::exists(p))
			return "";
		uint32_t len = boost::filesystem::file_size(p);
		std::ifstream file;
		
		file.open(p.string(), std::ios::in|std::ios::binary);
		
		tiny_string res(file,len);
		file.close();
		return res;
	}
	void FileWrite(const tiny_string& filename, const tiny_string& data)
	{
		if (!boost::filesystem::portable_file_name(filename))
			return;
		boost::filesystem::path p(Config::getConfig()->getDataDirectory());
		if (p.empty())
			return;
		p /= filename.raw_buf();
		std::ofstream file;
		
		file.open(p.string(), std::ios::out|std::ios::binary);
		file << data;
		file.close();
	}
	void FileReadByteArray(const tiny_string &filename,ByteArray* res)
	{
		if (!boost::filesystem::portable_file_name(filename))
			return;
		boost::filesystem::path p(Config::getConfig()->getDataDirectory());
		if (p.empty())
			return;
		p /= filename.raw_buf();
		if (!boost::filesystem::exists(p))
			return;
		uint32_t len = boost::filesystem::file_size(p);
		std::ifstream file;
		uint8_t buf[len];
		file.open(p.string(), std::ios::in|std::ios::binary);
		file.read((char*)buf,len);
		res->writeBytes(buf,len);
		file.close();
	}
	
	void FileWriteByteArray(const tiny_string &filename, ByteArray *data)
	{
		if (data == NULL)
			return;
		if (!boost::filesystem::portable_file_name(filename))
			return;
		boost::filesystem::path p(Config::getConfig()->getDataDirectory());
		if (p.empty())
			return;
		p /= filename.raw_buf();
		std::ofstream file;
		
		file.open(p.string(), std::ios::out|std::ios::binary|std::ios::trunc);
		uint8_t* buf = data->getBuffer(data->getLength(),false);
		file.write((char*)buf,data->getLength());
		file.close();
	}
};

int main(int argc, char* argv[])
{
	char* fileName=NULL;
	char* url=NULL;
	char* paramsFileName=NULL;
#ifdef PROFILING_SUPPORT
	char* profilingFileName=NULL;
#endif
	char *HTTPcookie=NULL;
	SecurityManager::SANDBOXTYPE sandboxType=SecurityManager::LOCAL_WITH_FILE;
	bool useInterpreter=true;
	bool useFastInterpreter=false;
	bool useJit=false;
	SystemState::ERROR_TYPE exitOnError=SystemState::ERROR_PARSING;
	LOG_LEVEL log_level=LOG_INFO;
	SystemState::FLASH_MODE flashMode=SystemState::FLASH;

	setlocale(LC_ALL, "");
#ifdef _WIN32
	const char* localedir = getExectuablePath();
#else
	const char* localedir = "/usr/share/locale";
#endif
	bindtextdomain("lightspark", localedir);
	textdomain("lightspark");

	LOG(LOG_INFO,"Lightspark version " << VERSION << " Copyright 2009-2013 Alessandro Pignotti and others");

	for(int i=1;i<argc;i++)
	{
		if(strcmp(argv[i],"-u")==0 || 
			strcmp(argv[i],"--url")==0)
		{
			i++;
			if(i==argc)
			{
				fileName=NULL;
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
		else if(strcmp(argv[i],"-l")==0 || strcmp(argv[i],"--log-level")==0)
		{
			i++;
			if(i==argc)
			{
				fileName=NULL;
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
				fileName=NULL;
				break;
			}
			paramsFileName=argv[i];
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
				fileName=NULL;
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
				fileName=NULL;
				break;
			}
			HTTPcookie=argv[i];
		}
		else
		{
			//No options flag, so set the swf file name
			if(fileName) //If already set, exit in error status
			{
				fileName=NULL;
				break;
			}
			fileName=argv[i];
		}
	}

	if(fileName==NULL)
	{
		LOG(LOG_ERROR, "Usage: " << argv[0] << " [--url|-u http://loader.url/file.swf]" <<
			" [--disable-interpreter|-ni] [--enable-fast-interpreter|-fi] [--enable-jit|-j]" <<
			" [--log-level|-l 0-4] [--parameters-file|-p params-file] [--security-sandbox|-s sandbox]" <<
			" [--exit-on-error] [--HTTP-cookies cookie] [--air] [--avmplus] [--disable-rendering]" <<
#ifdef PROFILING_SUPPORT
			" [--profiling-output|-o profiling-file]" <<
#endif
			" [--version|-v]" <<
			" <file.swf>");
		exit(1);
	}

	Log::setLogLevel(log_level);
	ifstream f(fileName, ios::in|ios::binary);
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
	setTLSSys(sys);
	sys->setDownloadedPath(fileName);

	//This setting allows qualifying filename-only paths to fully qualified paths
	//When the URL parameter is set, set the root URL to the given parameter
	if(url)
	{
		sys->mainClip->setOrigin(url, fileName);
		sys->parseParametersFromURL(sys->mainClip->getOrigin());
		if (sandboxType != SecurityManager::REMOTE &&
		    sys->mainClip->getOrigin().getProtocol() != "file")
		{
			LOG(LOG_INFO, _("Switching to remote sandbox because of remote url"));
			sandboxType = SecurityManager::REMOTE;
		}
	}
#ifndef _WIN32
	//When running in a local sandbox, set the root URL to the current working dir
	else if(sandboxType != SecurityManager::REMOTE)
	{
		char * cwd = get_current_dir_name();
		string cwdStr = string("file://") + string(cwd);
		free(cwd);
		cwdStr += "/";
		sys->mainClip->setOrigin(cwdStr, fileName);
	}
#endif
	else
	{
		sys->mainClip->setOrigin(string("file://") + fileName);
		LOG(LOG_INFO, _("Warning: running with no origin URL set."));
	}

	//One of useInterpreter or useJit must be enabled
	if(!(useInterpreter || useJit))
	{
		LOG(LOG_ERROR,_("No execution model enabled"));
		exit(1);
	}
	sys->useInterpreter=useInterpreter;
	sys->useFastInterpreter=useFastInterpreter;
	sys->useJit=useJit;
	sys->exitOnError=exitOnError;
	if(paramsFileName)
		sys->parseParametersFromFile(paramsFileName);
#ifdef PROFILING_SUPPORT
	if(profilingFileName)
		sys->setProfilingOutput(profilingFileName);
#endif
	if(HTTPcookie)
		sys->setCookies(HTTPcookie);

	sys->setParamsAndEngine(new StandaloneEngineData(), true);

	sys->securityManager->setSandboxType(sandboxType);
	if(sandboxType == SecurityManager::REMOTE)
		LOG(LOG_INFO, _("Running in remote sandbox"));
	else if(sandboxType == SecurityManager::LOCAL_WITH_NETWORK)
		LOG(LOG_INFO, _("Running in local-with-networking sandbox"));
	else if(sandboxType == SecurityManager::LOCAL_WITH_FILE)
		LOG(LOG_INFO, _("Running in local-with-filesystem sandbox"));
	else if(sandboxType == SecurityManager::LOCAL_TRUSTED)
		LOG(LOG_INFO, _("Running in local-trusted sandbox"));

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
	EngineData::mainLoopThread->join();

	delete pt;
	delete sys;

	SystemState::staticDeinit();
	
	return isonerror ? 1 : 0;
}

