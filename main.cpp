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
#include "streams.h"
#include <time.h>
#include <iostream>
#include <fstream>
#include <list>

#include <SDL/SDL.h>

using namespace std;

__thread SystemState* sys;

inline long timeDiff(timespec& s, timespec& d)
{
	timespec temp;
	if ((end.tv_nsec-start.tv_nsec)<0) {
		temp.tv_sec = end.tv_sec-start.tv_sec-1;
		temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
	} else {
		temp.tv_sec = end.tv_sec-start.tv_sec;
		temp.tv_nsec = end.tv_nsec-start.tv_nsec;
	}
	return temp.tv_sec*1000+(temp.tv_nsec)/1000000;
}

int main(int argc, char* argv[])
{
	if(argc!=2)
	{
		cout << "Usage: " << argv[0] << " <file.swf>" << endl;
		exit(-1);
	}
	Log::initLogging(NOT_IMPLEMENTED);
	sys=new SystemState;
	sys->performance_profiling=true;
	zlib_file_filter zf;
	zf.open(argv[1],ios_base::in);
	istream f(&zf);
	
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

