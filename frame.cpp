/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2011  Alessandro Pignotti (a.pignotti@sssup.it)

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

extern TLSDATA bool isVmThread;

Frame::Frame(const Frame& r):
	initialized(r.initialized),invalid(r.invalid),
	constructed(ACQUIRE_READ(r.constructed)),
	blueprint(r.blueprint),displayList(r.displayList),
	controls(r.controls)
{
}

Frame::Frame(const Frame&& r):
	initialized(r.initialized),invalid(r.invalid),
	constructed(ACQUIRE_READ(r.constructed)),
	blueprint(std::move(r.blueprint)),displayList(std::move(r.displayList)),
	controls(std::move(r.controls))
{
}

Frame& Frame::operator=(const Frame& r)
{
	initialized=r.initialized;
	invalid=r.invalid;
	RELEASE_WRITE(constructed,ACQUIRE_READ(r.constructed));
	blueprint=r.blueprint;

	displayList=r.displayList;
	controls=r.controls;
	return *this;
}

void Frame::Render(bool maskEnabled)
{
	DisplayListType::const_iterator i=displayList.begin();

	//Render objects of this frame;
	for(;i!=displayList.end();++i)
	{
		//Assign object data from current transformation
		i->second->setMatrix(i->first.Matrix);
		i->second->Render(maskEnabled);
	}
}

void dumpDisplayList(list<DisplayObject*>& l)
{
	list<DisplayObject*>::iterator it=l.begin();
	for(;it!=l.end();++it)
	{
		cout << *it << endl;
	}
}

void Frame::construct(_R<MovieClip> parent)
{
	assert(!constructed);
	//Update the displayList using the tags in this frame
	std::list<DisplayListTag*>::iterator it=blueprint.begin();
	for(;it!=blueprint.end();++it)
		(*it)->execute(parent.getPtr(), displayList);
	blueprint.clear();

	//As part of initialization set the transformation matrix for the child objects
	DisplayListType::const_iterator i=displayList.begin();
	for(;i!=displayList.end();++i)
	{
		i->second->setMatrix(i->first.Matrix);
		//Take a chance to also invalidate the content
		if(i->second->isOnStage())
			i->second->requestInvalidation();
	}
	invalid=false;
	setConstructed();
}

void Frame::init(_R<MovieClip> parent, const DisplayListType& d)
{
	if(!initialized)
	{
		//Execute control tags for this frame
		//Only the root movie clip can have control tags
		if(!controls.empty())
		{
			assert_and_throw(parent->getRoot()==parent);
			for(unsigned int i=0;i<controls.size();i++)
				controls[i]->execute(parent->getRoot().getPtr());
			controls.clear();
		}

		displayList=d;

		if(sys->currentVm && !isVmThread)
		{
			//Add a FrameConstructedEvent to the queue, the whole frame construction
			//will happen in the VM context
			parent->incRef();
			_R<ConstructFrameEvent> ce(new ConstructFrameEvent(*this, parent));
			sys->currentVm->addEvent(NullRef, ce);
		}
		else
			construct(parent);

		initialized=true;
	}
}
