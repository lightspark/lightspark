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
#include <glib/gstdio.h>
#include "version.h"
#include "backends/security.h"
#include "backends/config.h"
#include "backends/streamcache.h"
#include "backends/sdl/event_loop.h"
#include "swf.h"
#include "logger.h"
#include "platforms/engineutils.h"
#include "compat.h"
#include "flash/utils/ByteArray.h"
#include "scripting/flash/display/RootMovieClip.h"
#include <sys/stat.h>
#include "parsing/streams.h"
#include "launcher.h"
#include "timer.h"
#include "utils/filesystem.h"
#include "utils/path.h"

#ifdef __MINGW32__
    #ifndef PATH_MAX
    #define PATH_MAX _MAX_PATH
    #endif
    #define realpath(N,R) _fullpath((R),(N),_MAX_PATH)
#endif
using namespace std;
using namespace lightspark;
namespace fs = FileSystem;

class StandaloneEngineData: public EngineData
{
	Path basePath;
	Path appStoragePath;

	bool isValidFilePath(const Path& path)
	{
		return !path.contains("./") && path != ".." && !path.contains("~");
	}

	bool createDirs(const Path& path, const fs::Perms& perms)
	{
		Path base = path;
		for (; !base.exists(); base = base.getDir());

		if (!fs::createDirs(path))
			return false;

		for (auto _path = path; _path != base; _path = _path.getDir())
		{
			try
			{
				fs::setPerms(_path, perms);
			}
			catch (...)
			{
				return false;
			}
		}
		return true;
	}

	bool tryMoveDir(const Path& path, const Path& oldPath)
	{
		if (!createDirs(path, fs::Perms::OwnerAll))
			return false;

		if (!oldPath.exists())
			return false;

		try
		{
			fs::rename(oldPath, path);
			return true;
		}
		catch (...)
		{
			return false;
		}
	}

	Optional<Path> tryGetFullPath
	(
		SystemState* sys,
		const Path& path,
		bool isFullPath
	)
	{
		if (!isValidFilePath(path))
			return {};
		Path ret = isFullPath ? path : Path(FileFullPath(sys, path));
		if (ret.empty())
			return {};
		return ret;
	}
public:
	StandaloneEngineData(const Path& fileDataPath, const Path& absPath)
	{
		auto config = Config::getConfig();
		Path userDataPath = tiny_string(config->getUserDataDirectory());
		Path cachePath = tiny_string(config->getCacheDirectory());
		sharedObjectDatapath = userDataPath / "data";
		// move sharedObject data from old location to new location, if new location doesn't exist yet
		if (!tryMoveDir(sharedObjectDatapath, cachePath / "data"))
			LOG(LOG_ERROR, "couldn't move shared object data from cache directory to user data directory");

		sharedObjectDatapath += fileDataPath;

		appStoragePath = userDataPath / fileDataPath;

		// move ApplicationStorage data from old location to new location, if new location doesn't exist yet
		if (!tryMoveDir(appStoragePath, cachePath / fileDataPath))
			LOG(LOG_ERROR, "couldn't move appstorage data from cache directory to user data directory");

		appStoragePath /= "appstorage";
		basePath = absPath.getDir();
	}

	~StandaloneEngineData()
	{
		fs::removeAll(tiny_string(Config::getConfig()->getDataDirectory()));
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
	bool getAIRApplicationDescriptor(SystemState* sys,tiny_string& xmlstring) override
	{
		if (sys->flashMode != SystemState::FLASH_MODE::AIR)
			return false;

		Path path
		(
			Path(FileFullPath(sys, "")).getDir() /
			"META-INF" /
			"AIR" /
			"application.xml"
		);

		xmlstring = FileRead(sys, path, true);
		return true;
	}

	bool FileExists(SystemState* sys, const tiny_string& filename, bool isFullPath) override
	{
		return tryGetFullPath
		(
			sys,
			filename,
			isFullPath
		).transformOr(false, [&](const Path& path) { return path.exists(); });
	}

	bool FileIsHidden(SystemState* sys, const tiny_string& filename, bool isFullPath) override
	{
		return tryGetFullPath
		(
			sys,
			filename,
			isFullPath
		).transformOr(false, [&](const Path& path)
		{
			// TODO: Add a portable way of checking if a path is hidden
			// to `FileSystem`.
			#ifdef _WIN32
			LOG(LOG_NOT_IMPLEMENTED,"File.IsHidden not properly implemented for windows");
			#endif
			return path.getFilename().getStr().startsWith('.');
		});
	}

	bool FileIsDirectory(SystemState* sys, const tiny_string& filename, bool isFullPath) override
	{
		return tryGetFullPath
		(
			sys,
			filename,
			isFullPath
		).transformOr(false, [&](const Path& path) { return path.isDir(); });
	}

	uint32_t FileSize(SystemState* sys, const tiny_string& filename, bool isFullPath) override
	{
		return tryGetFullPath
		(
			sys,
			filename,
			isFullPath
		).filter([&](const Path& path)
		{
			return path.isFile();
		}).transformOr(0, [&](const Path& path)
		{
			return fs::fileSize(path);
		});
	}

	tiny_string FileFullPath(SystemState* sys, const tiny_string& filename) override
	{
		Path path(filename);
		bool isAIR = sys->flashMode == SystemState::FLASH_MODE::AIR;

		Path ret;

		if (isAIR && !basePath.empty())
			ret = !path.isAbsolute() ? basePath : "";
		else
			ret = Config::getConfig()->getDataDirectory();

		if (ret.empty())
			return "";
		return ret /= filename;
	}

	tiny_string FileBasename(SystemState* sys, const tiny_string& filename, bool isFullPath) override
	{
		return tryGetFullPath
		(
			sys,
			filename,
			isFullPath
		).transformOr(tiny_string(), [&](const Path& path)
		{
			return path.getFilename().getStr();
		});
	}

	tiny_string FileRead(SystemState* sys, const tiny_string& filename, bool isFullPath) override
	{
		return tryGetFullPath
		(
			sys,
			filename,
			isFullPath
		).filter([&](const Path& path)
		{
			return path.isFile();
		}).transformOr(tiny_string(), [&](const Path& path)
		{
			std::ifstream file(path.getStr(), std::ios::in | std::ios::binary);
			return tiny_string(file, fs::fileSize(path));
		});
	}

	void FileWrite(SystemState* sys, const tiny_string& filename, const tiny_string& data, bool isFullPath) override
	{
		(void)tryGetFullPath
		(
			sys,
			filename,
			isFullPath
		).filter([&](const Path& path)
		{
			return path.isFile();
		}).andThen([&](const Path& path) -> Optional<Path>
		{
			std::ofstream file(path.getStr(), std::ios::out | std::ios::binary);
			file << data;
			return {};
		});
	}

	uint8_t FileReadUnsignedByte(SystemState* sys, const tiny_string &filename, uint32_t startpos, bool isFullPath) override
	{
		return tryGetFullPath
		(
			sys,
			filename,
			isFullPath
		).filter([&](const Path& path)
		{
			return path.isFile();
		}).transformOr(0, [&](const Path& path)
		{
			uint8_t ret;
			std::ifstream file(path.getStr(), std::ios::in | std::ios::binary);
			file.seekg(startpos);
			file.read(reinterpret_cast<char*>(&ret), 1);
			return ret;
		});
	}

	void FileReadByteArray(SystemState* sys, const tiny_string &filename, ByteArray* res, uint32_t startpos, uint32_t length, bool isFullPath) override
	{
		(void)tryGetFullPath
		(
			sys,
			filename,
			isFullPath
		).filter([&](const Path& path)
		{
			return path.isFile();
		}).andThen([&](const Path& path) -> Optional<Path>
		{
			size_t len = std::min<decltype(fs::fileSize(path))>
			(
				length, fs::fileSize(path) - startpos
			);
			uint8_t* buf = new uint8_t[len];
			std::ifstream file(path.getStr(), std::ios::in | std::ios::binary);
			file.seekg(startpos);
			file.read(reinterpret_cast<char*>(buf), len);
			res->writeBytes(buf, len);
			delete[] buf;
			return path;
		});
	}

	void FileWriteByteArray(SystemState* sys, const tiny_string& filename, ByteArray* data, uint32_t startpos, uint32_t length, bool isFullPath) override
	{
		(void)tryGetFullPath
		(
			sys,
			filename,
			isFullPath
		).filter([&](const Path& path)
		{
			return path.isFile();
		}).andThen([&](const Path& path) -> Optional<Path>
		{
			size_t len =
			(
				length != UINT32_MAX &&
				startpos + length <= data->getLength()
			) ? length : data->getLength();
			uint8_t* buf = data->getBuffer(data->getLength(), false);
			std::ofstream file(path.getStr(), std::ios::out | std::ios::binary);
			file.write(reinterpret_cast<char*>(buf+startpos), len);
			return path;
		});
	}

	bool FileCreateDirectory(SystemState* sys, const tiny_string& filename, bool isFullPath) override
	{
		return tryGetFullPath
		(
			sys,
			filename,
			isFullPath
		).transformOr(false, [&](const Path& path)
		{
			// Remove the path's filename, if it has one.
			//
			// NOTE: We can't use `removeFilename()` because it modifies the
			// path, but because `transformOr()` is const, that's not allowed.
			auto dir = path.hasFilename() ? path.getDir() : path;
			return createDirs
			(
				dir,
				fs::Perms::All ^
				(
					fs::Perms::GroupWrite |
					fs::Perms::OthersWrite
				)
			);
		});
	}

	bool FilGetDirectoryListing(SystemState* sys, const tiny_string &filename, bool isFullPath, std::vector<tiny_string>& filelist) override
	{
		return tryGetFullPath
		(
			sys,
			filename,
			isFullPath
		).filter([&](const Path& path)
		{
			return path.isDir();
		}).transformOr(false, [&](const Path& path)
		{
			std::transform
			(
				fs::DirIter(path),
				fs::DirIter(),
				std::back_inserter(filelist),
				[](const fs::DirEntry& entry)
				{
					return entry.getPath().getStr();
				}
			);
			return true;
		});
	}

	bool FilePathIsAbsolute(const tiny_string& filename) override
	{
		return Path(filename).isAbsolute();
	}

	tiny_string FileGetApplicationStorageDir() override
	{
		return appStoragePath;
	}
};

int main(int argc, char* argv[])
{
	Path fileName;
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
	
	tiny_string spoof_os;
	for(int i=1;i<argc;i++)
	{
		if(strcmp(argv[i],"-u")==0 || 
			strcmp(argv[i],"--url")==0)
		{
			i++;
			if(i==argc)
			{
				fileName.clear();
				break;
			}

			url=argv[i];
		}
		else if(strcmp(argv[i],"--air")==0)
			flashMode=SystemState::AIR;
		else if(strcmp(argv[i],"--avmplus")==0)
			LOG(LOG_INFO,"--avmplus is deprecated and ignored");
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
				fileName.clear();
				break;
			}
			startscalefactor = max(1.0,atof(argv[i]));
		}
		else if(strcmp(argv[i],"-l")==0 || strcmp(argv[i],"--log-level")==0)
		{
			i++;
			if(i==argc)
			{
				fileName.clear();
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
				fileName.clear();
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
				fileName.clear();
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
				fileName.clear();
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
				fileName.clear();
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
				fileName.clear();
				break;
			}
			HTTPcookie=argv[i];
		}
		else if(strcmp(argv[i],"-os")==0 || 
				 strcmp(argv[i],"--spoof-os")==0)
		{
			i++;
			if(i==argc)
			{
				break;
			}
			spoof_os = argv[i];
		}
		else if(strcmp(argv[i],"-h")==0 || 
				 strcmp(argv[i],"--help")==0)
		{
			LOG(LOG_ERROR, "Usage: " << argv[0] << " [--url|-u http://loader.url/file.swf]" <<
							   " [--disable-interpreter|-ni] [--enable-fast-interpreter|-fi]" <<
#ifdef LLVM_ENABLED
							   " [--enable-jit|-j]" <<
#endif
							   " [--log-level|-l 0-4] [--parameters-file|-p params-file] [--security-sandbox|-s sandbox]" <<
							   " [--exit-on-error] [--HTTP-cookies cookie] [--air] [--disable-rendering]" <<
#ifdef PROFILING_SUPPORT
							   " [--profiling-output|-o profiling-file]" <<
#endif
							   " [--ignore-unhandled-exceptions|-ne]"
							   " [--fullscreen|-fs]"
							   " [--scale|-sc]"
							   " [--load-extension|-le extension-file"
							   " [--help|-h]" <<
							   " [--version|-v]" <<
							   " [<file.swf>]");
			exit(0);
		}
		else
		{
			//No options flag, so set the swf file name
			if (!fileName.empty()) //If already set, exit in error status
			{
				fileName.clear();
				break;
			}
			fileName = tiny_string(argv[i], true);
		}
	}
	
	// no filename give, we start the launcher
	Launcher l; // define launcher here, so that the char pointers into the tiny_strings stay valid until the end
	if (fileName.empty())
	{
		bool fileselected = l.start();
		if (fileselected)
		{
			if (l.selectedfile.empty())
				exit(0);
			ignoreUnhandledExceptions=true; // always ignore exception when running from launcher
			fileName = l.selectedfile;
			if (l.needsAIR)
				flashMode=SystemState::AIR;
			if (l.needsfilesystem && l.needsnetwork)
				sandboxType = SecurityManager::LOCAL_TRUSTED;
			else if (l.needsfilesystem)
				sandboxType = SecurityManager::LOCAL_WITH_FILE;
			else if (l.needsnetwork)
				sandboxType = SecurityManager::LOCAL_WITH_NETWORK;
			
			url = !l.baseurl.empty() ? (char*)l.baseurl.raw_buf() : nullptr;
		}
		else
			exit(0);
	}

	Log::setLogLevel(log_level);
	lsfilereader r(fileName.rawBuf());
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

	SDLEventLoop* eventLoop = new SDLEventLoop(new Time());
	Path absPath;
	try
	{
		absPath = fs::canonical(fileName);
	}
	catch (fs::Exception& e)
	{
		LOG(LOG_ERROR, "Unable to resolve path \"" << fileName.getStr() << "\". Reason: " << e.what());
		exit(1);
	}
	if (flashMode==SystemState::AIR)
		EngineData::checkForNativeAIRExtensions(extensions,(char*)absPath.rawBuf());
	//NOTE: see SystemState declaration
	SystemState* sys = new SystemState(fileSize, flashMode, eventLoop);
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
	//When running in a local sandbox, set the root URL to the current working dir
	else if(sandboxType != SecurityManager::REMOTE)
	{
		char* baseurl = g_filename_to_uri(fs::currentPath().rawBuf(),nullptr,nullptr);
		string cwdStr = string(baseurl);
		free(baseurl);
		cwdStr += "/";
		sys->mainClip->setOrigin(cwdStr, fileName);
	}
	else
	{
		sys->mainClip->setOrigin(tiny_string("file://") + fileName.getGenericStr());
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
	Path homeDir(g_get_home_dir());
	// remove home dir, if file is located below home dir
	Path fileDataPath = fs::relative(absPath, homeDir);

	sys->setParamsAndEngine(new StandaloneEngineData(fileDataPath, absPath), true);
	// on standalone local storage is always allowed
	sys->getEngineData()->setLocalStorageAllowedMarker(true);
	sys->getEngineData()->startInFullScreenMode=startInFullScreenMode;
	sys->getEngineData()->startscalefactor=startscalefactor;
	if (!spoof_os.empty())
		sys->getEngineData()->platformOS=spoof_os;

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
	
	if (!EngineData::startSDLMain(eventLoop))
	{
		LOG(LOG_ERROR,"SDL initialization failed, aborting");
		SystemState::staticDeinit();
		exit(3);
	}

	/* Destroy blocks until the 'terminated' flag is set by
	 * SystemState::setShutdownFlag.
	 */
	sys->destroy();
	int exitcode = sys->getExitCode();

	delete pt;
	delete sys;

	SystemState::staticDeinit();
	
	return exitcode;
}

