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

#include <list>

#include "asobjects.h"
#include "swf.h"

using namespace std;

extern __thread SystemState* sys;

ASStage::ASStage():width(640),height(480)
{
	setVariableByName("width",SWFObject(&width,true));
	setVariableByName("height",SWFObject(&height,true));
}

void ASArray::_register()
{
	setVariableByName("constructor",SWFObject(new Function(constructor),true));
}

SWFObject ASArray::constructor(const SWFObject& th, arguments* args)
{
	LOG(CALLS,"Called Array constructor");
	return SWFObject();
}

void ASMovieClipLoader::_register()
{
	setVariableByName("constructor",SWFObject(new Function(constructor),true));
}

SWFObject ASMovieClipLoader::constructor(const SWFObject&, arguments* args)
{
	LOG(CALLS,"Called MoviewClipLoader constructor");
	return SWFObject();
}

ASXML::ASXML()
{
	_register();
}

void ASXML::_register()
{
	setVariableByName("constructor",SWFObject(new Function(constructor),true));
}

SWFObject ASXML::constructor(const SWFObject&, arguments* args)
{
	LOG(CALLS,"Called XML constructor");
	return SWFObject();
}

void ASObject::_register()
{
	setVariableByName("constructor",SWFObject(new Function(constructor),true));
}

SWFObject ASObject::constructor(const SWFObject&, arguments* args)
{
	LOG(CALLS,"Called Object constructor");
	return SWFObject();
}

ASMovieClip::ASMovieClip():_visible(1),_width(100)
{
	sem_init(&sem_frames,0,1);
}

bool ASMovieClip::list_orderer(const IDisplayListElem* a, int d)
{
	return a->getDepth()<d;
}

void ASMovieClip::addToDisplayList(IDisplayListElem* t)
{
	list<IDisplayListElem*>::iterator it=lower_bound(displayList.begin(),displayList.end(),t->getDepth(),list_orderer);
	displayList.insert(it,t);
}

SWFObject ASMovieClip::swapDepths(const SWFObject&, arguments* args)
{
	LOG(CALLS,"Called swapDepths");
	return SWFObject();
}

void ASMovieClip::_register()
{
	setVariableByName("_visible",SWFObject(&_visible,true));
	setVariableByName("_width",SWFObject(&_width,true));
	setVariableByName("swapDepths",SWFObject(new Function(swapDepths),true));
}

void ASMovieClip::Render(int)
{
	LOG(TRACE,"Render MovieClip");
	RunState* bak=sys->currentState;
	sys->currentState=&state;
	state.next_FP=min(state.FP+1,frames.size()-1);

	list<Frame>::iterator frame=frames.begin();
	for(int i=0;i<state.FP;i++)
		frame++;
	frame->Render(0);

	//Render objects added at runtime;
	list<IDisplayListElem*>::iterator it=dynamicDisplayList.begin();
	for(it;it!=dynamicDisplayList.end();it++)
		(*it)->Render();

	if(state.FP!=state.next_FP)
	{
		state.FP=state.next_FP;
		sys->setUpdateRequest(true);
	}
	sys->currentState=bak;
	LOG(TRACE,"End Render MovieClip");
}

