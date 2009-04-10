/**************************************************************************
    Lighspark, a free flash player implementation

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
#include <time.h>
#include <iostream>
#include <fstream>
#include <list>

#include <SDL/SDL.h>

using namespace std;

__thread SystemState* sys;

void thread_debug(char* msg)
{
	timespec ts;
	clock_gettime(CLOCK_REALTIME,&ts);
	fprintf(stderr,"%u.%010u %s\n",ts.tv_sec,ts.tv_nsec,msg);
}

inline long timeDiff(timespec& s,timespec& d)
{
	return (d.tv_sec-s.tv_sec)*1000+(d.tv_nsec-s.tv_nsec)/1000000;
}

int main()
{
	sys=new SystemState;
	Log::initLogging(ERROR);
	sys->performance_profiling=true;
	ifstream f("flash.swf",ifstream::in);
	SDL_Init ( SDL_INIT_VIDEO|SDL_INIT_EVENTTHREAD );
	ParseThread pt(sys,f);
	RenderThread rt(sys,SDL,NULL);
	InputThread it(sys,SDL,NULL);

	timespec ts,td;
	clock_gettime(CLOCK_REALTIME,&ts);
	int count=0;
	while(1)
	{
		sys->waitToRun();
		rt.draw(&sys->getFrameAtFP());
		sys->advanceFP();
		count++;
		clock_gettime(CLOCK_REALTIME,&td);
		if(timeDiff(ts,td)>1000)
		{
			ts=td;
			LOG(NO_INFO,"FPS: " << count);
			count=0;
		}
	}


	cout << "the end" << endl;
	it.wait();
	SDL_Quit();
}

