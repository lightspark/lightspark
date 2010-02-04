/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009  Alessandro Pignotti (a.pignotti@sssup.it)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#include "swf.h"
#include "tags.h"
#include "actions.h"
#include "frame.h"
#include "geometry.h"
#include "logger.h"
#include "streams.h"
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

std::vector<fps_profiling> fps_profs;

int main(int argc, char* argv[])
{
	char* fileName=NULL;
	char* url=NULL;

	for(int i=1;i<argc;i++)
	{
		if(strcmp(argv[i],"-u")==0 || 
			strcmp(argv[i],"--url")==0)
		{
			i++;
			if(i==argc)
				break;

			url=argv[i];
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
		cout << "Usage: " << argv[0] << " [--url|-u http://loader.url/file.swf] <file.swf>" << endl;
		exit(-1);
	}

#ifndef WIN32
	struct rlimit rl;
	getrlimit(RLIMIT_AS,&rl);
	rl.rlim_cur=400000000;
	rl.rlim_max=rl.rlim_cur;
	//setrlimit(RLIMIT_AS,&rl);
#endif

	Log::initLogging(LOG_NOT_IMPLEMENTED);
	sys=new SystemState;
	fps_profs.push_back(fps_profiling());
	sys->fps_prof=&fps_profs.back();

	//Set a bit of SystemState using parameters
	if(url)
		sys->setUrl(url);

	zlib_file_filter zf(argv[1]);
	istream f(&zf);
	f.exceptions ( istream::eofbit | istream::failbit | istream::badbit );
	
	SDL_Init ( SDL_INIT_VIDEO |SDL_INIT_EVENTTHREAD );
	ParseThread pt(sys,sys,f);
	RenderThread rt(sys,SDL,NULL);
	InputThread it(sys,SDL,NULL);
	sys->cur_input_thread=&it;
	sys->cur_render_thread=&rt;
	ThreadPool tp(sys);
	sys->cur_thread_pool=&tp;

#ifndef WIN32
	timespec ts,td,tperf,tperf2;
	clock_gettime(CLOCK_REALTIME,&ts);
#endif
	int count=0;
	int sec_count=0;

	LOG(LOG_CALLS,"sys 0x" << sys);
	while(1)
	{
		if(sys->shutdown)
			break;
		rt.draw();

		count++;
#ifndef WIN32
		clock_gettime(CLOCK_REALTIME,&td);
		if(timeDiff(ts,td)>1000)
		{
			ts=td;
			//LOG(NO_INFO,"FPS: " << dec <<count);
			cout << "FPS: " << dec << count << endl;
			sys->fps_prof->fps=count;
			count=0;
			sec_count++;
			fps_profs.push_back(fps_profiling());
			sys->fps_prof=&fps_profs.back();
			if(sec_count>120)
			{
				cout << "exiting" << endl;
				sys->setShutdownFlag();
				break;
			}
		}
#endif
	}

	it.wait();
	rt.wait();
	pt.wait();

	ofstream prof("lightspark.dat");
	for(int i=0;i<fps_profs.size();i++)
		prof << i << ' ' << fps_profs[i].render_time << ' ' << fps_profs[i].action_time << ' ' << 
			fps_profs[i].cache_time << ' ' << fps_profs[i].fps*10 << ' ' << fps_profs[i].event_count << 
			' ' << fps_profs[i].event_time << endl;
	prof.close();

	abort();
	SDL_Quit();
	return 0;
}

