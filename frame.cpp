/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009,2010  Alessandro Pignotti (a.pignotti@sssup.it)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#include "scripting/abc.h"
#include "frame.h"
#include "parsing/tags.h"
#include "swf.h"
#include "compat.h"

using namespace std;
using namespace lightspark;

extern TLSDATA SystemState* sys;

Frame::~Frame()
{
	list <pair<PlaceInfo, DisplayObject*> >::iterator i=displayList.begin();

	if(sys && !sys->finalizingDestruction)
	{
		//Decrease the refcount of childs
		for(;i!=displayList.end();i++)
		{
			assert(i->second);
			i->second->decRef();
		}
	}
}

void Frame::inputRender()
{
	list <pair<PlaceInfo, DisplayObject*> >::iterator i=displayList.begin();

	//Render objects of this frame;
	for(;i!=displayList.end();i++)
	{
		assert(i->second);

		//Assign object data from current transformation
		i->second->setMatrix(i->first.Matrix);
		i->second->inputRender();
	}
}

void Frame::Render()
{
	list <pair<PlaceInfo, DisplayObject*> >::iterator i=displayList.begin();

	//Render objects of this frame;
	for(;i!=displayList.end();i++)
	{
		assert(i->second);

		//Assign object data from current transformation
		i->second->setMatrix(i->first.Matrix);
		i->second->Render();
	}
}

void dumpDisplayList(list<DisplayObject*>& l)
{
	list<DisplayObject*>::iterator it=l.begin();
	for(;it!=l.end();it++)
	{
		cout << *it << endl;
	}
}

void Frame::init(MovieClip* parent, list <pair<PlaceInfo, DisplayObject*> >& d)
{
	if(!initialized)
	{
		//Execute control tags for this frame
		//Only the root movie clip can have control tags
		if(!controls.empty())
		{
			assert_and_throw(parent->getRoot()==parent);
			for(unsigned int i=0;i<controls.size();i++)
				controls[i]->execute(parent->getRoot());
			controls.clear();

			if(sys->currentVm)
			{
				//We stop execution until execution engine catches up
				SynchronizationEvent* se=new SynchronizationEvent;
				bool added=sys->currentVm->addEvent(NULL, se);
				if(!added)
				{
					se->decRef();
					throw RunTimeException("Could not add event");
				}
				se->wait();
				se->decRef();
				//Now the bindings are effective for all tags
			}
		}

		//Update the displayList using the tags in this frame
		std::list<DisplayListTag*>::iterator it=blueprint.begin();
		for(;it!=blueprint.end();it++)
			(*it)->execute(parent, d);
		blueprint.clear();
		displayList=d;
		//Acquire a new reference to every child
		list <pair<PlaceInfo, DisplayObject*> >::const_iterator dit=displayList.begin();
		for(;dit!=displayList.end();dit++)
			dit->second->incRef();
		initialized=true;

		//As part of initialization set the transformation matrix for the child objects
		list <pair<PlaceInfo, DisplayObject*> >::iterator i=displayList.begin();

		for(;i!=displayList.end();i++)
			i->second->setMatrix(i->first.Matrix);
	}
}
