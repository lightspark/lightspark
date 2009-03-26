#include "frame.h"
#include "tags.h"
#include <list>
#include <GL/gl.h>


using namespace std;

void Frame::Render()
{
	cout << "Start Frame" << endl << endl;
	list < DisplayListTag* >::iterator i=displayList.begin();
	int count=0;
	for(i;i!=displayList.end();i++)
	{
		if(*i!=NULL)
		{
			count++;
			//std::cout << "Depth " << (*i)->getDepth() <<std::endl;
			glPushMatrix();
			glTranslatef(0,0,(*i)->getDepth()-baseLayer);
			//(*i)->printInfo();
			(*i)->Render();
			glPopMatrix();

			//if(count>5 && hack)
			//	break;
		}
	}
}

void Frame::setLabel(STRING l)
{
	Label=l;
}
