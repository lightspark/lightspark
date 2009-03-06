#include <iostream>
#include <string.h>
#include <pthread.h>


#include "swf.h"
#include "actions.h"

using namespace std;

pthread_t ParseThread::t;
std::list < DisplayListTag* > ParseThread::displayList;

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
					sys.dictionary.push_back(dynamic_cast<RenderTag*>(tag));
					break;
				case DISPLAY_LIST_TAG:
					if(dynamic_cast<DisplayListTag*>(tag)->add_to_list)
						displayList.push_back(dynamic_cast<DisplayListTag*>(tag));
					break;
				case SHOW_TAG:
				{
					//thread_debug( "PARSER: frames mutex lock");
					sem_wait(&sys.sem_frames);
					sys.frames.push_back(Frame(displayList));
					//thread_debug( "PARSER: frames mutex unlock" );
					sem_post(&sys.new_frame);
					//thread_debug("PARSER: new frame signal");
					sem_post(&sys.sem_frames);
					/*if(done>30)
					{
						//sleep(5);
						goto exit;
					}*/
					done++;
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
		cout << s << endl;
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
