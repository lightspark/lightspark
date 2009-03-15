#include "frame.h"
#include "tags.h"
#include <list>
#include <GL/gl.h>


using namespace std;

void Frame::Render()
{
	cout << "Start Frame" << endl << endl;
	list < DisplayListTag* >::iterator i=displayList.begin();
	for(i;i!=displayList.end();i++)
	{
		if(*i!=NULL)
		{
			//std::cout << "Depth " << (*i)->getDepth() <<std::endl;
			glTranslatef(0,0,(*i)->getDepth());
			(*i)->Render();
		}
	}
}

void Frame::setLabel(STRING l)
{
	Label=l;
}
