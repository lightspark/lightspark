/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2008-2012  Alessandro Pignotti (a.pignotti@sssup.it)

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
#include "swf.h"
#include "logger.h"
#include "backends/security.h"
#include "platforms/engineutils.h"
#ifndef _WIN32
#	include <sys/resource.h>
#endif
#include "compat.h"


using namespace std;
using namespace lightspark;

class StandaloneEngineData: public EngineData
{
	guint destroyHandlerId;
public:
	StandaloneEngineData() : destroyHandlerId(0)
	{
#ifndef _WIN32
		visual = XVisualIDFromVisual(gdk_x11_visual_get_xvisual(gdk_visual_get_system()));
#endif
	}
	static void destroyWidget(GtkWidget* widget)
	{
		gdk_threads_enter();
		gtk_widget_destroy(widget);
		gdk_threads_leave();
	}
	virtual ~StandaloneEngineData()
	{
		if(widget)
		{
			if(destroyHandlerId)
				g_signal_handler_disconnect(widget, destroyHandlerId);

			runInGtkThread(sigc::bind(&destroyWidget,widget));
		}
	}
	static void StandaloneDestroy(GtkWidget *widget, gpointer data)
	{
		StandaloneEngineData* e = (StandaloneEngineData*)data;
		RecMutex::Lock l(e->mutex);
		/* no need to destroy it - it's already done */
		e->widget = NULL;
		getSys()->setShutdownFlag();
	}
	GtkWidget* createGtkWidget()
	{
		GtkWidget* window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
		gtk_window_set_title((GtkWindow*)window,"Lightspark");
		destroyHandlerId = g_signal_connect(window,"destroy",G_CALLBACK(StandaloneDestroy),this);
		return window;
	}
	NativeWindow getWindowForGnash()
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

	LOG(LOG_INFO,"Lightspark version " << VERSION << " Copyright 2009-2012 Alessandro Pignotti and others");

	//Make GTK thread enabled
#ifdef HAVE_G_THREAD_INIT
	g_thread_init(NULL);
#endif
	gdk_threads_init();
	//Give GTK a chance to parse its own options
	gtk_init (&argc, &argv);

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
			" [--exit-on-error] [--HTTP-cookies cookie] [--air]" <<
#ifdef PROFILING_SUPPORT
			" [--profiling-output|-o profiling-file]" <<
#endif
			" <file.swf>");
		exit(1);
	}

#ifndef _WIN32
	struct rlimit rl;
	getrlimit(RLIMIT_AS,&rl);
	rl.rlim_cur=400000000;
	rl.rlim_max=rl.rlim_cur;
	//setrlimit(RLIMIT_AS,&rl);

#endif

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
	EngineData::startGTKMain();
	//NOTE: see SystemState declaration
#ifdef MEMORY_USAGE_PROFILING
	MemoryAccount sysAccount("sysAccount");
	SystemState* sys = new (&sysAccount) SystemState(fileSize, flashMode);
#else
	SystemState* sys = new ((MemoryAccount*)NULL) SystemState(fileSize, flashMode);
#endif
	ParseThread* pt = new ParseThread(f, sys);
	setTLSSys(sys);
	sys->setDownloadedPath(fileName);

	//This setting allows qualifying filename-only paths to fully qualified paths
	//When the URL parameter is set, set the root URL to the given parameter
	if(url)
	{
		sys->setOrigin(url, fileName);
		sys->parseParametersFromURL(sys->getOrigin());
		sandboxType = SecurityManager::REMOTE;
	}
#ifndef _WIN32
	//When running in a local sandbox, set the root URL to the current working dir
	else if(sandboxType != SecurityManager::REMOTE)
	{
		char * cwd = get_current_dir_name();
		string cwdStr = string("file://") + string(cwd);
		free(cwd);
		cwdStr += "/";
		sys->setOrigin(cwdStr, fileName);
	}
#endif
	else
	{
		sys->setOrigin(string("file://") + fileName);
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
	delete pt;

	SystemState::staticDeinit();
	EngineData::quitGTKMain();
	return 0;
}

