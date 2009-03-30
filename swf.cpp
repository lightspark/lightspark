#include <iostream>
#include <string.h>
#include <pthread.h>
#include <SDL/SDL.h>
#include <GL/gl.h>


#include "swf.h"
#include "actions.h"

using namespace std;

pthread_t ParseThread::t;

pthread_t InputThread::t;
std::list < IActiveObject* > InputThread::listeners;
sem_t InputThread::sem_listeners;

pthread_t RenderThread::t;
sem_t RenderThread::render;
sem_t RenderThread::end_render;
Frame* RenderThread::cur_frame=NULL;

extern SystemState sys;

int thread_debug(char* msg);
SWF_HEADER::SWF_HEADER(istream& in)
{
	//Valid only on little endian platforms
	in >> Signature[0] >> Signature[1] >> Signature[2];

	if(Signature[0]!='F' || Signature[1]!='W' || Signature[2]!='S')
		throw "bad file\n";
	in >> Version >> FileLenght >> FrameSize >> FrameRate >> FrameCount;
}

MovieClip::MovieClip()
{
	sem_init(&sem_frames,0,1);
}

SystemState::SystemState():currentDisplayList(&clip.displayList),currentState(&clip.state)
{
	sem_init(&sem_dict,0,1);
	sem_init(&new_frame,0,0);
	sem_init(&sem_run,0,0);
}

bool list_orderer(const DisplayListTag* a, int d)
{
	return a->getDepth()<d;
}

void* ParseThread::worker(void* in_ptr)
{
	istream& f=*(istream*)in_ptr;
	SWF_HEADER h(f);
	cout << h.getFrameSize() << endl;

	int done=0;

	try
	{
		TagFactory factory(f);
		while(1)
		{
			Tag* tag=factory.readTag();
			switch(tag->getType())
			{
			//	case TAG:
				case END_TAG:
					//sleep(5);
					cout << "End of File" << endl;
					return 0;
				case RENDER_TAG:
				//	std::cout << "add to dict" << std::endl;
				//	thread_debug( "PARSER: dict mutex lock");
					sem_wait(&sys.sem_dict);
					sys.dictionary.push_back(dynamic_cast<RenderTag*>(tag));
				//	thread_debug( "PARSER: dict mutex unlock");
					sem_post(&sys.sem_dict);
					break;
				case DISPLAY_LIST_TAG:
					if(dynamic_cast<DisplayListTag*>(tag)->add_to_list)
					{
						list<DisplayListTag*>::iterator it=lower_bound(sys.clip.displayList.begin(),
								sys.clip.displayList.end(),
								dynamic_cast<DisplayListTag*>(tag)->getDepth(),
								list_orderer);
						sys.clip.displayList.insert(it,dynamic_cast<DisplayListTag*>(tag));
					}
					break;
				case SHOW_TAG:
				{
				//	thread_debug( "PARSER: frames mutex lock");
					sem_wait(&sys.clip.sem_frames);
					sys.clip.frames.push_back(Frame(sys.clip.displayList));
				//	thread_debug( "PARSER: frames mutex unlock" );
					sem_post(&sys.new_frame);
				//\	thread_debug("PARSER: new frame signal");
					sem_post(&sys.clip.sem_frames);
					break;
				}
				case CONTROL_TAG:
					dynamic_cast<ControlTag*>(tag)->execute();
					break;
			}
		}
	}
	catch(const char* s)
	{
		cout << "CATCH!!!!" << endl;
		cout << s << endl;
		cout << "ERRORE!!!!" << endl;
		exit(-1);
	}


}

ParseThread::ParseThread(istream& in)
{
	pthread_create(&t,NULL,worker,&in);
}

void ParseThread::wait()
{
	pthread_join(t,NULL);
}

InputThread::InputThread()
{
	cout << "creating input" << endl;
	sem_init(&sem_listeners,0,1);
	pthread_create(&t,NULL,worker,NULL);
}

void InputThread::wait()
{
	pthread_join(t,NULL);
}

void* InputThread::worker(void* in_ptr)
{
	SDL_Event event;
	cout << "waiting for input" << endl;
	while(SDL_WaitEvent(&event))
	{
		switch(event.type)
		{
			case SDL_KEYDOWN:
			{
				switch(event.key.keysym.sym)
				{
					case SDLK_q:
						exit(0);
						break;
					case SDLK_n:
						list<IActiveObject*>::const_iterator it=listeners.begin();
						cout << "Fake mouse event" << endl;
						int c=0;
						for(it;it!=listeners.end();it++)
						{
							if(c==2)
								(*it)->MouseEvent(0,0);
							c++;
						}
						sem_post(&sys.sem_run);
						break;
				}
				break;
			}
			case SDL_MOUSEMOTION:
			{
				printf("Oh! mouse\n");
				break;
			}
		}
	}
}

void InputThread::addListener(IActiveObject* ob)
{
	sem_wait(&sem_listeners);

	listeners.push_back(ob);

	sem_post(&sem_listeners);
}

RenderThread::RenderThread(ENGINE e,void* param)
{
	sem_init(&render,0,0);
	sem_init(&end_render,0,0);
	if(e==SDL)
		pthread_create(&t,NULL,sdl_worker,0);
}

void* RenderThread::sdl_worker(void*)
{
	SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 8 );
	SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 8 );
	SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 8 );
	SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 24 );
	SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, 8 );
	SDL_SetVideoMode( 640, 480, 24, SDL_OPENGL );
	glEnable( GL_DEPTH_TEST );
	glDepthFunc(GL_LEQUAL);

//	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
	glViewport(0,0,640,480);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0,640,480,0,-100,0);
	glMatrixMode(GL_MODELVIEW);

	try
	{
		while(1)
		{
			if(cur_frame==NULL)
				continue;
			sem_wait(&render);
			glClearColor(sys.Background.Red/255.0F,sys.Background.Green/255.0F,sys.Background.Blue/255.0F,0);
			glClearDepth(0xffff);
			glClearStencil(5);
			glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
			glLoadIdentity();

			glScalef(0.1,0.1,1);

			cur_frame->Render(0);

			SDL_GL_SwapBuffers( );
			sys.clip.state.FP=sys.clip.state.next_FP;
			sem_post(&end_render);
		}
	}
	catch(const char* e)
	{
		cout << e << endl;
		cout << "ERRORE main" << endl;
		exit(-1);
	}
}

void RenderThread::draw(Frame* f)
{
	//TODO: sync (by copy)
	cur_frame=f;
	sem_post(&render);
	sem_wait(&end_render);

}
