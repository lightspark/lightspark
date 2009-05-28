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
#include <iostream>
#include <fstream>
#include <list>

#include <SDL/SDL.h>

using namespace std;

__thread SystemState* sys;

long timeDiff(timespec& s, timespec& d)
{
	timespec temp;
	if ((d.tv_nsec-s.tv_nsec)<0) {
		temp.tv_sec = d.tv_sec-s.tv_sec-1;
		temp.tv_nsec = 1000000000+d.tv_nsec-s.tv_nsec;
	} else {
		temp.tv_sec = d.tv_sec-s.tv_sec;
		temp.tv_nsec = d.tv_nsec-s.tv_nsec;
	}
	return temp.tv_sec*1000+(temp.tv_nsec)/1000000;
}

std::vector<fps_profiling> fps_profs;

int main(int argc, char* argv[])
{
	if(argc!=2)
	{
		cout << "Usage: " << argv[0] << " <file.swf>" << endl;
		exit(-1);
	}
	Log::initLogging(TRACE);
	sys=new SystemState;
	sys->performance_profiling=false;
	zlib_file_filter zf;
	zf.open(argv[1],ios_base::in);
	istream f(&zf);
	
	SDL_Init ( SDL_INIT_VIDEO|SDL_INIT_EVENTTHREAD );
	ParseThread pt(sys,f);
	RenderThread rt(sys,SDL,NULL);
	InputThread it(sys,SDL,NULL);
	sys->cur_input_thread=&it;

	timespec ts,td,tperf,tperf2;
	clock_gettime(CLOCK_REALTIME,&ts);
	int count=0;

	fps_profs.push_back(fps_profiling());
	sys->fps_prof=&fps_profs.back();

	while(1)
	{
		sys->waitToRun();
		if(sys->shutdown)
			break;
		rt.draw(&sys->getFrameAtFP());
		sys->advanceFP();

		count++;
		clock_gettime(CLOCK_REALTIME,&td);
		if(timeDiff(ts,td)>1000)
		{
			ts=td;
			LOG(NO_INFO,"FPS: " << count);
			sys->fps_prof->fps=count;
			count=0;
			fps_profs.push_back(fps_profiling());
			sys->fps_prof=&fps_profs.back();
		}
	}

	it.wait();
	rt.wait();
	pt.wait();

	ofstream prof("lightspark.dat");
	for(int i=0;i<fps_profs.size();i++)
		prof << i << ' ' << fps_profs[i].render_time << ' ' << fps_profs[i].action_time << ' ' << fps_profs[i].cache_time << ' ' << fps_profs[i].fps*10 << endl;
	prof.close();

	SDL_Quit();
}

