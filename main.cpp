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
#include <GL/gl.h>

using namespace std;

RunState state;
SystemState sys;

int thread_debug(char* msg)
{
	timespec ts;
	clock_gettime(CLOCK_REALTIME,&ts);
	fprintf(stderr,"%u.%010u %s\n",ts.tv_sec,ts.tv_nsec,msg);
}

int main()
{
	ifstream f("flash.swf",ifstream::in);
	SWF_HEADER h(f);
	cout << h.getFrameSize() << endl;

	SDL_Init ( SDL_INIT_VIDEO );
	SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 8 );
	SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 8 );
	SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 8 );
	SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 16 );
	SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, 8 );
	glDisable( GL_DEPTH_TEST );
//	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
	SDL_SetVideoMode( 640, 480, 24, SDL_OPENGL );
	glViewport(0,0,640,480);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0,640,480,0,-65536,0);
	glMatrixMode(GL_MODELVIEW);
//	glLoadIdentity();
//	glScalef(0.1,0.1,0.1);

	ParseThread pt(f);

	try
	{
		while(state.FP!=-1)
		{
			while(1)
			{
				//thread_debug( "RENDER: frames mutex lock");
				sem_wait(&sys.sem_frames);

				if(state.FP<sys.frames.size())
					break;

				//thread_debug( "RENDER: frames mutex unlock");
				sem_post(&sys.sem_frames);
				thread_debug("RENDER: new frame wait");
				sem_wait(&sys.new_frame);
			}
			//Aquired lock on frames list

			state.next_FP=state.FP+1;
			glClearColor(sys.Background.Red/255.0F,sys.Background.Green/255.0F,sys.Background.Blue/255.0F,0);
			glClear(GL_COLOR_BUFFER_BIT);
			sys.frames[state.FP].Render();
			SDL_GL_SwapBuffers( );
			state.FP=state.next_FP;

			//thread_debug( "RENDER: frames mutex unlock");
			sem_post(&sys.sem_frames);
		}
	}
	catch(const char* e)
	{
		cout << e << endl;
	}
	cout << "the end" << endl;
	sleep(2);
//	pt.wait();
	SDL_Quit();
}

