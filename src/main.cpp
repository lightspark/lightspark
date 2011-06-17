/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2011  Alessandro Pignotti (a.pignotti@sssup.it)

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

#include "swf.h"
#include "logger.h"
#include "parsing/streams.h"
#include "backends/netutils.h"
#include "backends/security.h"
#ifndef WIN32
#include <sys/resource.h>
#include <unistd.h>
#include <gdk/gdkx.h>
#endif
#include <iostream>
#include <fstream>
#include "compat.h"

#ifdef WIN32
#include <windows.h>
#undef main
#endif

using namespace std;
using namespace lightspark;

TLSDATA DLL_PUBLIC SystemState* sys;
TLSDATA DLL_PUBLIC RenderThread* rt=NULL;
TLSDATA DLL_PUBLIC ParseThread* pt=NULL;

class StandaloneEngineData: public EngineData
{
public:
	StandaloneEngineData(Display* d, VisualID v, Window win):
		EngineData(d,v,win,0,0){}
	void setupMainThreadCallback(ls_callback_t func, void* arg)
	{
		//Synchronizing with the main gtk thread is what we actually need
		gdk_threads_enter();
		func(arg);
		gdk_threads_leave();
	}
	void stopMainDownload()
	{
	}
	bool isSizable() const
	{
		return true;
	}
};

static void StandaloneDestroy(GtkWidget *widget, gpointer data)
{
	sys->setShutdownFlag();
	gtk_main_quit();
}

int main(int argc, char* argv[])
{
	char* fileName=NULL;
	char* url=NULL;
	char* paramsFileName=NULL;
#ifdef PROFILING_SUPPORT
	char* profilingFileName=NULL;
#endif
	SecurityManager::SANDBOXTYPE sandboxType=SecurityManager::LOCAL_WITH_FILE;
	bool useInterpreter=true;
	bool useJit=false;
	LOG_LEVEL log_level=LOG_NOT_IMPLEMENTED;

	setlocale(LC_ALL, "");
	bindtextdomain("lightspark", "/usr/share/locale");
	textdomain("lightspark");

	cout << "Lightspark version " << VERSION << " Copyright 2009-2011 Alessandro Pignotti and others" << endl;

	//Make GTK thread enabled
	g_thread_init(NULL);
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
		else if(strcmp(argv[i],"-ni")==0 || strcmp(argv[i],"--disable-interpreter")==0)
			useInterpreter=false;
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

			log_level=(LOG_LEVEL)atoi(argv[i]);
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
		cout << endl << "Usage: " << argv[0] << " [--url|-u http://loader.url/file.swf]" << 
			" [--disable-interpreter|-ni] [--enable-jit|-j] [--log-level|-l 0-4]" << 
			" [--parameters-file|-p params-file] [--security-sandbox|-s sandbox]" <<
#ifdef PROFILING_SUPPORT
			" [--profiling-output|-o profiling-file]" << 
#endif
			" <file.swf>" << endl;
		exit(1);
	}

#ifndef WIN32
	struct rlimit rl;
	getrlimit(RLIMIT_AS,&rl);
	rl.rlim_cur=400000000;
	rl.rlim_max=rl.rlim_cur;
	//setrlimit(RLIMIT_AS,&rl);

#endif

	Log::initLogging(log_level);
	ifstream f(fileName);
	f.seekg(0, ios::end);
	uint32_t fileSize=f.tellg();
	f.seekg(0, ios::beg);
	if(!f)
	{
		cout << argv[0] << ": " << fileName << ": No such file or directory" << endl;
		exit(2);
	}
	f.exceptions ( istream::eofbit | istream::failbit | istream::badbit );
	cout.exceptions( ios::failbit | ios::badbit);
	cerr.exceptions( ios::failbit | ios::badbit);
	ParseThread* pt = new ParseThread(NULL,f);
	SystemState::staticInit();
	//NOTE: see SystemState declaration
	sys=new SystemState(pt, fileSize);

	//This setting allows qualifying filename-only paths to fully qualified paths
	//When the URL parameter is set, set the root URL to the given parameter
	if(url)
	{
		sys->setOrigin(url, fileName);
		sandboxType = SecurityManager::REMOTE;
	}
#ifndef WIN32
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
		LOG(LOG_NO_INFO, _("Warning: running with no origin URL set."));
	}

	//One of useInterpreter or useJit must be enabled
	if(!(useInterpreter || useJit))
	{
		LOG(LOG_ERROR,_("No execution model enabled"));
		exit(1);
	}
	sys->useInterpreter=useInterpreter;
	sys->useJit=useJit;
	if(paramsFileName)
		sys->parseParametersFromFile(paramsFileName);
#ifdef PROFILING_SUPPORT
	if(profilingFileName)
		sys->setProfilingOutput(profilingFileName);
#endif

	gdk_threads_enter();
	//Create the main window
	GtkWidget* window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title((GtkWindow*)window,"Lightspark");
	g_signal_connect(window,"destroy",G_CALLBACK(StandaloneDestroy),NULL);
	GtkWidget* socket=gtk_socket_new();
	gtk_container_add(GTK_CONTAINER(window), socket);
	gtk_widget_show(socket);
	gtk_widget_show(window);

	VisualID visual=XVisualIDFromVisual(gdk_x11_visual_get_xvisual(gdk_visual_get_system()));
	Display* display=gdk_x11_display_get_xdisplay(gdk_display_get_default());
	Window xembedWindow=gtk_socket_get_id((GtkSocket*)socket);

	StandaloneEngineData* e=new StandaloneEngineData(display, visual, xembedWindow);

	sys->setParamsAndEngine(e, true);
	gdk_threads_leave();

	sys->securityManager->setSandboxType(sandboxType);
	if(sandboxType == SecurityManager::REMOTE)
		LOG(LOG_NO_INFO, _("Running in remote sandbox"));
	else if(sandboxType == SecurityManager::LOCAL_WITH_NETWORK)
		LOG(LOG_NO_INFO, _("Running in local-with-networking sandbox"));
	else if(sandboxType == SecurityManager::LOCAL_WITH_FILE)
		LOG(LOG_NO_INFO, _("Running in local-with-filesystem sandbox"));
	else if(sandboxType == SecurityManager::LOCAL_TRUSTED)
		LOG(LOG_NO_INFO, _("Running in local-trusted sandbox"));

	sys->downloadManager=new StandaloneDownloadManager();

	//Start the parser
	sys->addJob(pt);

	gdk_threads_enter();
	gtk_main();
	gdk_threads_leave();

	sys->wait();
	delete sys;
	delete pt;

	SystemState::staticDeinit();
	return 0;
}

