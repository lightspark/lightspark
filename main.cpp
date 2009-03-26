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

//RunState state;
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

	SDL_Init ( SDL_INIT_VIDEO|SDL_INIT_EVENTTHREAD );
	SDL_SetVideoMode( 640, 480, 24, SDL_OPENGL );
	SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 8 );
	SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 8 );
	SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 8 );
	SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 24 );
	SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, 8 );
	glEnable( GL_DEPTH_TEST );
	glDepthFunc(GL_LEQUAL);

//	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
	glViewport(0,0,640,480);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0,640,480,0,-100,0);
	glMatrixMode(GL_MODELVIEW);

	InputThread it;
	ParseThread pt(f);

			float* buffer=new float[640*240];
	try
	{
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
			glClearColor(sys.Background.Red/255.0F,sys.Background.Green/255.0F,sys.Background.Blue/255.0F,0);
			glClearDepth(0xffff);
			glClearStencil(5);
			glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
			glLoadIdentity();

			glScalef(0.1,0.1,1);

			//if(sys.clip.state.FP>=43)
			//	sys.clip.frames[sys.clip.state.FP].hack=1;
			sys.clip.frames[sys.clip.state.FP].Render();

			if(1)
			{

			glReadPixels(0,240,640,240,GL_DEPTH_COMPONENT,GL_FLOAT,buffer);
			/*glReadPixels(0,240,640,240,GL_STENCIL_INDEX,GL_FLOAT,buffer);
			for(int i=0;i<240*640;i++)
				buffer[i]/=10;*/
			glDrawPixels(640,240,GL_LUMINANCE,GL_FLOAT,buffer);
			}
			SDL_GL_SwapBuffers( );
			sys.clip.state.FP=sys.clip.state.next_FP;

			//thread_debug( "RENDER: frames mutex unlock");
			sem_post(&sys.clip.sem_frames);
			cout << "Frame " << sys.clip.state.FP << endl;
		}
	}
	catch(const char* e)
	{
		cout << e << endl;
		cout << "ERRORE main" << endl;
		exit(-1);
	}
			delete[] buffer;
	cout << "the end" << endl;
	//sleep(20);
	it.wait();
	SDL_Quit();
}

