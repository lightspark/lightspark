/**************************************************************************
    Lightspark, a free flash player implementation

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

#include "abc.h"
#include "frame.h"
#include "tags.h"
#include <list>
#include <time.h>
#include "swf.h"
#include <GL/gl.h>

using namespace std;
long timeDiff(timespec& s, timespec& d);

extern __thread SystemState* sys;

void Frame::runScript()
{
	if(script)
		sys->currentVm->addEvent(NULL,new FunctionEvent(script));
}

void Frame::Render()
{
	timespec ts,td;
	clock_gettime(CLOCK_REALTIME,&ts);
	list <IDisplayListElem* >::iterator i=displayList.begin();

	//Render objects of this frame;
	for(i;i!=displayList.end();i++)
	{
		if(*i!=NULL)
			(*i)->Render();
	}
	//Render objects added at runtime
	i=dynamicDisplayList->begin();
	for(i;i!=dynamicDisplayList->end();i++)
		(*i)->Render();

	clock_gettime(CLOCK_REALTIME,&td);
	sys->fps_prof->render_time+=timeDiff(ts,td);
}

void dumpDisplayList(list<IDisplayListElem*>& l)
{
	list<IDisplayListElem*>::iterator it=l.begin();
	for(it;it!=l.end();it++)
	{
		cout << *it << endl;
	}
}

void Frame::setLabel(STRING l)
{
	Label=l;
}

void Frame::init(MovieClip* parent, std::list<IDisplayListElem*>& d)
{
	std::list<DisplayListTag*>::iterator it=blueprint.begin();
	for(it;it!=blueprint.end();it++)
		(*it)->execute(parent, d);
	blueprint.clear();
	displayList=d;
}
