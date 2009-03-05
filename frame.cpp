#include "frame.h"
#include <list>
#include <GL/gl.h>


using namespace std;

void Frame::Render()
{
	list < DisplayListTag* >::iterator i=displayList.begin();
	int count=0;
	for(i;i!=displayList.end();i++)
	{
		if(*i!=NULL)
		{
			count++;
			if(count!=3)
				continue;
			std::cout << "Depth " << (*i)->getDepth() <<std::endl;
			//glTranslatef(0,0,float(count)/10);
			glLoadIdentity();
			glScalef(0.1,0.1,0.1);
			(*i)->Render();
		}
	}
}

void Frame::setLabel(STRING l)
{
	Label=l;
}
