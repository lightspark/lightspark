/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009,2010  Alessandro Pignotti (a.pignotti@sssup.it)

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
#ifndef WIN32
#include <sys/resource.h>
#endif
#include <iostream>
#include <fstream>

#ifdef WIN32
#include <windows.h>
#endif
#include <SDL.h>
#ifdef WIN32
#undef main
#endif

using namespace std;
using namespace lightspark;

TLSDATA DLL_PUBLIC SystemState* sys;
TLSDATA DLL_PUBLIC RenderThread* rt=NULL;
TLSDATA DLL_PUBLIC ParseThread* pt=NULL;

int main(int argc, char* argv[])
{
	char* fileName=NULL;
	char* url=NULL;
	char* paramsFileName=NULL;
	SECURITY_SANDBOXTYPE sandboxType=SECURITY_SANDBOX_REMOTE;
	bool useInterpreter=true;
	bool useJit=false;
	LOG_LEVEL log_level=LOG_NOT_IMPLEMENTED;

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
		else if(strcmp(argv[i],"-ni")==0 || 
			strcmp(argv[i],"--disable-interpreter")==0)
		{
			useInterpreter=false;
		}
		else if(strcmp(argv[i],"-j")==0 || 
			strcmp(argv[i],"--enable-jit")==0)
		{
			useJit=true;
		}
		else if(strcmp(argv[i],"-l")==0 || 
			strcmp(argv[i],"--log-level")==0)
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
			{
				sandboxType = SECURITY_SANDBOX_REMOTE;
			}
			else if(strncmp(argv[i], "local-with-filesystem", 21) == 0)
			{
				sandboxType = SECURITY_SANDBOX_LOCAL_WITH_FILE;
			}
			else if(strncmp(argv[i], "local-with-networking", 21) == 0)
			{
				sandboxType = SECURITY_SANDBOX_LOCAL_WITH_NETWORK;
			}
			else if(strncmp(argv[i], "local-trusted", 13) == 0)
			{
				sandboxType = SECURITY_SANDBOX_LOCAL_TRUSTED;
			}
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
		cout << "Usage: " << argv[0] << " [--url|-u http://loader.url/file.swf]" << 
			" [--disable-interpreter|-ni] [--enable-jit|-j] [--log-level|-l 0-4]" << 
			" [--parameters-file|-p params-file] [--security-sandbox|-s sandbox] <file.swf>" << endl;
		exit(-1);
	}

#ifndef WIN32
	struct rlimit rl;
	getrlimit(RLIMIT_AS,&rl);
	rl.rlim_cur=400000000;
	rl.rlim_max=rl.rlim_cur;
	//setrlimit(RLIMIT_AS,&rl);

#endif

	Log::initLogging(log_level);
	zlib_file_filter zf(fileName);
	istream f(&zf);
	f.exceptions ( istream::eofbit | istream::failbit | istream::badbit );
	cout.exceptions( ios::failbit | ios::badbit);
	cerr.exceptions( ios::failbit | ios::badbit);
	ParseThread* pt = new ParseThread(NULL,f);
	SystemState::staticInit();
	//NOTE: see SystemState declaration
	sys=new SystemState(pt);

	//Set a bit of SystemState using parameters
	if(url)
		sys->setUrl(url);

	//One of useInterpreter or useJit must be enabled
	if(!(useInterpreter || useJit))
	{
		LOG(LOG_ERROR,"No execution model enabled");
		exit(-1);
	}
	sys->useInterpreter=useInterpreter;
	sys->useJit=useJit;
	if(paramsFileName)
		sys->parseParametersFromFile(paramsFileName);

	sys->setOrigin(fileName);
	
	SDL_Init ( SDL_INIT_VIDEO |SDL_INIT_EVENTTHREAD );
	sys->setParamsAndEngine(SDL, NULL);
	sys->setSandboxType(sandboxType);

	//SECURITY_SANDBOX_LOCAL_TRUSTED should actually be able to use both local and network files
	if(sandboxType == SECURITY_SANDBOX_REMOTE || 
			sandboxType == SECURITY_SANDBOX_LOCAL_WITH_NETWORK ||
			sandboxType == SECURITY_SANDBOX_LOCAL_TRUSTED)
	{
		LOG(LOG_NO_INFO, "Running in remote sandbox");
		sys->downloadManager=new CurlDownloadManager();
	}
	else {
		LOG(LOG_NO_INFO, "Running in local-with-filesystem sandbox");
		sys->downloadManager=new LocalDownloadManager();
	}

	//Start the parser
	sys->addJob(pt);

	sys->wait();
	pt->wait();
	delete sys;
	delete pt;

	SystemState::staticDeinit();
	SDL_Quit();
	return 0;
}

