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
#include "tags.h"
#include "actions.h"
#include "frame.h"
#include "geometry.h"
#include "logger.h"
#include "streams.h"
#include "netutils.h"
#include <time.h>
#ifndef WIN32
#include <sys/resource.h>
#endif
#include <iostream>
#include <fstream>
#include <list>

#ifdef WIN32
#include <windows.h>
#endif
#include <SDL.h>

using namespace std;
using namespace lightspark;

TLSDATA SystemState* sys;
TLSDATA RenderThread* rt=NULL;
TLSDATA ParseThread* pt=NULL;

int main(int argc, char* argv[])
{
	char* fileName=NULL;
	char* url=NULL;
	char* paramsFileName=NULL;
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
		cout << "Usage: " << argv[0] << " [--url|-u http://loader.url/file.swf] [--disable-interpreter|-ni] [--enable-jit|-j] [--log-level|-l 0-4] [--parameters-file|-p params-file] <file.swf>" << endl;
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
	//NOTE: see SystemState declaration
	sys=new SystemState;

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
	{
		ifstream p(paramsFileName);
		if(p)
		{
			sys->parseParameters(p);
			p.close();
		}
	}

	sys->setOrigin(fileName);
	zlib_file_filter zf(fileName);
	istream f(&zf);
	f.exceptions ( istream::eofbit | istream::failbit | istream::badbit );
	cout.exceptions( ios::failbit | ios::badbit);
	cerr.exceptions( ios::failbit | ios::badbit);
	
	SDL_Init ( SDL_INIT_VIDEO |SDL_INIT_EVENTTHREAD );
	ParseThread* pt = new ParseThread(sys,f);
	RenderThread rt(sys,SDL,NULL);
	InputThread it(sys,SDL,NULL);
	sys->inputThread=&it;
	sys->renderThread=&rt;
	sys->downloadManager=new CurlDownloadManager();
	//Start the parser
	sys->addJob(pt);

	sys->wait();
	it.wait();
	rt.wait();
	pt->wait();
	delete sys;
	delete pt;

	SDL_Quit();
	return 0;
}

