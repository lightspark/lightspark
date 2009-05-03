/**************************************************************************
    Lighspark, a free flash player implementation

    Copyright (C) 2009  Alessandro Pignotti (a.pignotti@sssup.it)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#include "frame.h"
#include "tags.h"
#include <list>
#include <GL/gl.h>
#include <time.h>

using namespace std;
long timeDiff(timespec& s, timespec& d);

extern __thread SystemState* sys;
void Frame::Render(int baseLayer)
{
	timespec ts,td;
	clock_gettime(CLOCK_REALTIME,&ts);
	list < IDisplayListElem* >::iterator i=displayList.begin();
	int count=0;
	for(i;i!=displayList.end();i++)
	{
		if(*i!=NULL)
			(*i)->Render();
		count++;
		if(count>baseLayer)
			break;
	}
	clock_gettime(CLOCK_REALTIME,&td);
	sys->fps_prof->render_time+=timeDiff(ts,td);
}

void Frame::setLabel(STRING l)
{
	Label=l;
}
