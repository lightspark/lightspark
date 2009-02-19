#include "swf.h"
#include "tags.h"
#include <iostream>
#include <fstream>
#include <list>
#include <SDL/SDL.h>
#include <GL/gl.h>

using namespace std;

list < RenderTag* > dictionary;
list < DisplayListTag* > displayList;

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
	//glEnable( GL_DEPTH_TEST );
//	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
	SDL_SetVideoMode( 640, 480, 24, SDL_OPENGL );
	glViewport(0,0,640,480);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0,640,480,0,-10,10);
	glMatrixMode(GL_MODELVIEW);
//	glLoadIdentity();
//	glScalef(0.1,0.1,0.1);
	int done=0;
	try
	{
		TagFactory factory(f);
		while(1)
		{
			Tag* tag=factory.readTag();
			std::cout << "end readTag" << std::endl;
			switch(tag->getType())
			{
			//	case TAG:
				case RENDER_TAG:
					std::cout << "add to dict" << std::endl;
					dictionary.push_back(dynamic_cast<RenderTag*>(tag));
					break;
				case DISPLAY_LIST_TAG:
					if(dynamic_cast<DisplayListTag*>(tag)->add_to_list)
						displayList.push_back(dynamic_cast<DisplayListTag*>(tag));
					break;
				case SHOW_TAG:
				{
					glClear(GL_COLOR_BUFFER_BIT); 
					list < DisplayListTag* >::iterator i=displayList.begin();
					glColor3f(0,1,0);
					int count=0;
					for(i;i!=displayList.end();i++)
					{
						count++;
						if(*i!=NULL)
						{
							glLoadIdentity();
							glTranslatef(0,0,float(count)/10);
							glScalef(0.1,0.1,0.1);
							(*i)->Render();
						}
						if(count>0)
							break;
					}
					SDL_GL_SwapBuffers( );
					std::cout << "end render" << std::endl;
					if(done>0)
					{
						sleep(5);
						goto exit;
					}
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
exit:
	SDL_Quit();
}

