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
#include <time.h>
#include <iostream>
#include <fstream>
#include <list>

#include <SDL/SDL.h>

using namespace std;

//RunState state;
SystemState sys;

void thread_debug(char* msg)
{
	timespec ts;
	clock_gettime(CLOCK_REALTIME,&ts);
	fprintf(stderr,"%u.%010u %s\n",ts.tv_sec,ts.tv_nsec,msg);
}

int main()
{
	ifstream f("flash.swf",ifstream::in);
	SDL_Init ( SDL_INIT_VIDEO|SDL_INIT_EVENTTHREAD );
	ParseThread pt(f);
	RenderThread rt(SDL,NULL);
	InputThread it;

		while(1)
		{
			if(sys.clip.state.stop_FP && !sys.update_request)
				sem_wait(&sys.sem_run);
			while(1)
			{
				//thread_debug( "RENDER: frames mutex lock");
				sem_wait(&sys.clip.sem_frames);

				if(sys.clip.state.FP<sys.clip.frames.size())
					break;

				//thread_debug( "RENDER: frames mutex unlock");
				sem_post(&sys.clip.sem_frames);
				//thread_debug("RENDER: new frame wait");
				sem_wait(&sys.new_frame);
			}
			//Aquired lock on frames list
			sys.update_request=false;
			sys.clip.state.next_FP=sys.clip.state.FP+1;
			rt.draw(&sys.clip.frames[sys.clip.state.FP]);
			sys.clip.state.FP=sys.clip.state.next_FP;

			sem_post(&sys.clip.sem_frames);
		}


	cout << "the end" << endl;
	it.wait();
	SDL_Quit();
}

