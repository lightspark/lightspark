#include "frame.h"
#include "tags.h"
#include <list>
#include <GL/gl.h>


using namespace std;

void Frame::Render()
{
	list < DisplayListTag* >::iterator i=displayList.begin();
	for(i;i!=displayList.end();i++)
	{
		if(*i!=NULL)
		{
			//std::cout << "Depth " << (*i)->getDepth() <<std::endl;
			glLoadIdentity();
			//glTranslatef(0,0,(*i)->getDepth());
			glScalef(0.1,0.1,0.1);
			(*i)->Render();
		}
	}
}

void Frame::setLabel(STRING l)
{
	Label=l;
}
