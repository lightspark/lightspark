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
#include <algorithm>
#include <curl/curl.h>
#include <libxml/parser.h>
#include <string.h>

#include "asobjects.h"
#include "swf.h"

using namespace std;

extern __thread SystemState* sys;

ASStage::ASStage():width(640),height(480)
{
	setVariableByName("width",SWFObject(&width));
	setVariableByName("height",SWFObject(&height));
}

void ASArray::_register()
{
	setVariableByName("constructor",SWFObject(new Function(constructor)));
}

SWFObject ASArray::constructor(const SWFObject& th, arguments* args)
{
	LOG(NOT_IMPLEMENTED,"Called Array constructor");
	return SWFObject();
}

ASMovieClipLoader::ASMovieClipLoader()
{
	_register();
}

void ASMovieClipLoader::_register()
{
	setVariableByName("constructor",SWFObject(new Function(constructor)));
	setVariableByName("addListener",SWFObject(new Function(addListener)));
}

SWFObject ASMovieClipLoader::constructor(const SWFObject&, arguments* args)
{
	LOG(NOT_IMPLEMENTED,"Called MovieClipLoader constructor");
	return SWFObject();
}

SWFObject ASMovieClipLoader::addListener(const SWFObject&, arguments* args)
{
	LOG(NOT_IMPLEMENTED,"Called MovieClipLoader::addListener");
	return SWFObject();
}

ASXML::ASXML()
{
	_register();
	xml_buf=new char[1024*20];
	xml_index=0;
}

void ASXML::_register()
{
	setVariableByName("constructor",SWFObject(new Function(constructor)));
	setVariableByName("load",SWFObject(new Function(load)));
}

SWFObject ASXML::constructor(const SWFObject&, arguments* args)
{
	LOG(NOT_IMPLEMENTED,"Called XML constructor");
	return SWFObject();
}


size_t ASXML::write_data(void *buffer, size_t size, size_t nmemb, void *userp)
{
	ASXML* th=(ASXML*)userp;
	memcpy(th->xml_buf+th->xml_index,buffer,size*nmemb);
	th->xml_index+=size*nmemb;
	return size*nmemb;
}

SWFObject ASXML::load(const SWFObject& obj, arguments* args)
{
	ASXML* th=dynamic_cast<ASXML*>(obj.getData());
	LOG(NOT_IMPLEMENTED,"Called ASXML::load " << args->args[0]->toString());
	CURL *curl;
	CURLcode res;
	curl = curl_easy_init();
	STRING base("www.youtube.com");
	STRING url=base+args->args[0]->toString();
	if(curl)
	{
		curl_easy_setopt(curl, CURLOPT_URL, (const char*)(url));
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, obj.getData());
		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);
	}
	xmlDocPtr doc=xmlReadMemory(th->xml_buf,th->xml_index,url,NULL,0);

	IFunction* on_load=obj->getVariableByName("onLoad")->toFunction();
	arguments a;
	a.args.push_back(new Integer(1));
	on_load->call(NULL,&a);
	return SWFObject(new Integer(1));
}

void ASObject::_register()
{
	setVariableByName("constructor",SWFObject(new Function(constructor)));
}

SWFObject ASObject::constructor(const SWFObject&, arguments* args)
{
	LOG(NOT_IMPLEMENTED,"Called Object constructor");
	return SWFObject();
}

ASString::ASString(const STRING& s)
{
	data.reserve(s.String.size());
	for(int i=0;i<s.String.size();i++)
		data.push_back(s.String[i]);
}

STRING ASString::toString()
{
	return STRING(data.data());
}

float ASString::toFloat()
{
	LOG(ERROR,"Cannot convert string " << data << " to float");
	return 0;
}

ASMovieClip::ASMovieClip():_visible(1),_width(100),hack(0),_framesloaded(0),_totalframes(1)
{
	sem_init(&sem_frames,0,1);
	ASMovieClip::_register();
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

SWFObject ASMovieClip::createEmptyMovieClip(const SWFObject& obj, arguments* args)
{
	ASMovieClip* th=dynamic_cast<ASMovieClip*>(obj.getData());
	if(th==NULL)
		LOG(ERROR,"Not a valid ASMovieClip");

	LOG(CALLS,"Called createEmptyMovieClip: " << args->args[0]->toString() << " " << args->args[1]->toString());
	ASMovieClip* ret=new ASMovieClip();

	//HACK
	ret->hack=1;

	IDisplayListElem* t=new ASObjectWrapper(ret,args->args[1]->toInt());
	list<IDisplayListElem*>::iterator it=lower_bound(th->dynamicDisplayList.begin(),th->dynamicDisplayList.end(),t->getDepth(),list_orderer);
	th->dynamicDisplayList.insert(it,t);

	SWFObject r(ret);
	th->setVariableByName(args->args[0]->toString(),r);
	return r;
}

SWFObject ASMovieClip::moveTo(const SWFObject&, arguments* args)
{
	LOG(NOT_IMPLEMENTED,"Called moveTo");
	return SWFObject();
}

SWFObject ASMovieClip::lineTo(const SWFObject&, arguments* args)
{
	LOG(NOT_IMPLEMENTED,"Called lineTo");
	return SWFObject();
}

SWFObject ASMovieClip::lineStyle(const SWFObject&, arguments* args)
{
	LOG(NOT_IMPLEMENTED,"Called lineStyle");
	return SWFObject();
}

SWFObject ASMovieClip::swapDepths(const SWFObject&, arguments* args)
{
	LOG(NOT_IMPLEMENTED,"Called swapDepths");
	return SWFObject();
}

void ASMovieClip::_register()
{
	setVariableByName("_visible",SWFObject(&_visible,true));
	setVariableByName("_width",SWFObject(&_width,true));
	setVariableByName("_framesloaded",SWFObject(&_framesloaded,true));
	setVariableByName("_totalframes",SWFObject(&_totalframes,true));
	setVariableByName("swapDepths",SWFObject(new Function(swapDepths)));
	setVariableByName("lineStyle",SWFObject(new Function(lineStyle)));
	setVariableByName("lineTo",SWFObject(new Function(lineTo)));
	setVariableByName("moveTo",SWFObject(new Function(moveTo)));
	setVariableByName("createEmptyMovieClip",SWFObject(new Function(createEmptyMovieClip)));
}

void ASMovieClip::Render()
{
	LOG(TRACE,"Render MovieClip");
	if(hack)
	{
		glColor3f(0,1,0);
		glBegin(GL_QUADS);
			glVertex2i(0,0);
			glVertex2i(_width,0);
			glVertex2i(_width,100);
			glVertex2i(0,100);
		glEnd();
		return;
	}
	parent=sys->currentClip;
	ASMovieClip* clip_bak=sys->currentClip;
	sys->currentClip=this;

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
	LOG(TRACE,"End Render MovieClip");

	sys->currentClip=clip_bak;
}

