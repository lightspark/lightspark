#include "swf.h"
#include "tags.h"
#include <iostream>
#include <fstream>
#include <list>
#include <SDL/SDL.h>
#include "gltags.h"

using namespace std;

list < Tag* > dictionary;

int main()
{
	ifstream f("flash.swf",ifstream::in);
	SWF_HEADER h(f);
	cout << h.getFrameSize() << endl;
	list < GLObject* > displayList;

	SDL_Init ( SDL_INIT_VIDEO );
	SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 8 );
	SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 8 );
	SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 8 );
	SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 16 );
//	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
	SDL_SetVideoMode( 640, 480, 24, SDL_OPENGL );
	glViewport(0,0,640,480);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0,640,480,0,-10,10);

	try
	{
		TagFactory factory(f);
		while(1)
		{
			Tag* tag=factory.readTag();
			std::cout << "end readTag" << std::endl;
			switch(tag->getType())
			{
				case TAG:
				case RENDER_TAG:
					std::cout << "add to dict" << std::endl;
					dictionary.push_back(tag);
					break;
				case DISPLAY_LIST_TAG:
					displayList.push_back(dynamic_cast<DisplayListTag*>(tag)->Render(dictionary));
					break;
				case SHOW_TAG:
				{
					glClear(GL_COLOR_BUFFER_BIT); 
					list < GLObject* >::iterator i=displayList.begin();
					glColor3f(0,1,0);
					for(i;i!=displayList.end();i++)
					{
						if(*i!=NULL)
						{
							glCallList((*i)->list);
							std::cout << "call list" << std::endl;
						}
					}
					SDL_GL_SwapBuffers( );
					std::cout << "end render" << std::endl;
					sleep(10);
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

	SDL_Quit();
}

