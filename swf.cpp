#include <iostream>
#include <string.h>
#include <pthread.h>
#include <SDL/SDL.h>


#include "swf.h"
#include "actions.h"

using namespace std;

pthread_t ParseThread::t;
std::list < DisplayListTag* > ParseThread::displayList;

pthread_t InputThread::t;
std::list < IActiveObject* > InputThread::listeners;
sem_t InputThread::sem_listeners;

extern SystemState sys;
extern RunState state;

int thread_debug(char* msg);
SWF_HEADER::SWF_HEADER(ifstream& in)
{
	//Valid only on little endian platforms
	in >> Signature[0] >> Signature[1] >> Signature[2];

	if(Signature[0]!='F' || Signature[1]!='W' || Signature[2]!='S')
		throw "bad file\n";
	in >> Version >> FileLenght >> FrameSize >> FrameRate >> FrameCount;
}

SystemState::SystemState()
{
	sem_init(&sem_frames,0,1);
	sem_init(&sem_dict,0,1);
	sem_init(&new_frame,0,0);
}

void* ParseThread::worker(void* in_ptr)
{
	ifstream& f=*(ifstream*)in_ptr;
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
						displayList.push_back(dynamic_cast<DisplayListTag*>(tag));
					break;
				case SHOW_TAG:
				{
				//	thread_debug( "PARSER: frames mutex lock");
					sem_wait(&sys.sem_frames);
					sys.frames.push_back(Frame(displayList));
				//	thread_debug( "PARSER: frames mutex unlock" );
					sem_post(&sys.new_frame);
				//\	thread_debug("PARSER: new frame signal");
					sem_post(&sys.sem_frames);
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

ParseThread::ParseThread(ifstream& in)
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
						for(it;it!=listeners.end();it++)
						{
							(*it)->MouseEvent(0,0);
						}
						sem_post(&state.sem_run);
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
