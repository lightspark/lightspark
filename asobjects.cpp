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

#include <list>
#include <algorithm>
#include <curl/curl.h>
#include <libxml/parser.h>
#include <string.h>

#include "asobjects.h"
#include "flashevents.h"
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

ASFUNCTIONBODY(ASArray,constructor)
{
	LOG(NOT_IMPLEMENTED,"Called Array constructor");
	return NULL;
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

ASFUNCTIONBODY(ASMovieClipLoader,constructor)
{
	LOG(NOT_IMPLEMENTED,"Called MovieClipLoader constructor");
	return NULL;
}

ASFUNCTIONBODY(ASMovieClipLoader,addListener)
{
	LOG(NOT_IMPLEMENTED,"Called MovieClipLoader::addListener");
	return NULL;
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

ASFUNCTIONBODY(ASXML,constructor)
{
	LOG(NOT_IMPLEMENTED,"Called XML constructor");
	return NULL;
}


size_t ASXML::write_data(void *buffer, size_t size, size_t nmemb, void *userp)
{
	ASXML* th=(ASXML*)userp;
	memcpy(th->xml_buf+th->xml_index,buffer,size*nmemb);
	th->xml_index+=size*nmemb;
	return size*nmemb;
}

ASFUNCTIONBODY(ASXML,load)
{
	ASXML* th=dynamic_cast<ASXML*>(obj);
	LOG(NOT_IMPLEMENTED,"Called ASXML::load " << args->args[0]->toString());
	CURL *curl;
	CURLcode res;
	curl = curl_easy_init();
	STRING base("www.youtube.com");
	STRING url=base+args->args[0]->toString();
	if(curl)
	{
		curl_easy_setopt(curl, CURLOPT_URL, (string(url)).c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, obj);
		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);
	}
	xmlDocPtr doc=xmlReadMemory(th->xml_buf,th->xml_index,(string(url)).c_str(),NULL,0);

	bool found;
	IFunction* on_load=obj->getVariableByName("onLoad",found)->toFunction();
	arguments a;
	a.args.push_back(new Integer(1));
	on_load->call(NULL,&a);
	return new Integer(1);
}

void ASObject::_register()
{
	setVariableByName("constructor",SWFObject(new Function(constructor)));
}

ASFUNCTIONBODY(ASObject,constructor)
{
	LOG(NOT_IMPLEMENTED,"Called Object constructor");
	return NULL;
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

ASMovieClip::ASMovieClip():_visible(1),_x(0),_y(0),_height(100),_width(100),hack(0),_framesloaded(0),_totalframes(1),displayListLimit(0)
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
	displayListLimit=displayList.size();
}

void ASMovieClip::handleEvent(Event* e)
{
	map<std::string, IFunction*>::iterator h=handlers.find(e->type);
	if(h==handlers.end())
	{
		LOG(NOT_IMPLEMENTED,"Not handled event");
		return;
	}

	cout << "MovieClip event " << h->first<< endl;
	arguments args;
	args.args.push_back((ISWFObject*)0xdeadbeaf);
	h->second->call(this,&args);
}

ASFUNCTIONBODY(ASMovieClip,addEventListener)
{
	if(args->args[0]->getObjectType()!=T_STRING || args->args[1]->getObjectType()!=T_FUNCTION)
	{
		LOG(ERROR,"Type mismatch");
		abort();
	}
	sys->cur_input_thread->addListener(args->args[0]->toString(),dynamic_cast<InteractiveObject*>(obj));
	ASMovieClip* th=dynamic_cast<ASMovieClip*>(obj);

	Function* f=dynamic_cast<Function*>(args->args[1].getData());
	Function* f2=(Function*)f->clone();
	f2->bind();

	th->handlers.insert(make_pair(args->args[0]->toString(),f2->toFunction()));
	sys->events_name.push_back(args->args[0]->toString());
}

ASFUNCTIONBODY(ASMovieClip,createEmptyMovieClip)
{
	ASMovieClip* th=dynamic_cast<ASMovieClip*>(obj);
	if(th==NULL)
		LOG(ERROR,"Not a valid ASMovieClip");

	LOG(CALLS,"Called createEmptyMovieClip: " << args->args[0]->toString() << " " << args->args[1]->toString());
	ASMovieClip* ret=new ASMovieClip();

	//HACK
	ret->hack=1;

	IDisplayListElem* t=new ASObjectWrapper(ret,args->args[1]->toInt());
	list<IDisplayListElem*>::iterator it=lower_bound(th->dynamicDisplayList.begin(),th->dynamicDisplayList.end(),t->getDepth(),list_orderer);
	th->dynamicDisplayList.insert(it,t);

	th->setVariableByName(args->args[0]->toString(),ret);
	return ret;
}

ASFUNCTIONBODY(ASMovieClip,moveTo)
{
	LOG(NOT_IMPLEMENTED,"Called moveTo");
	return NULL;
}

ASFUNCTIONBODY(ASMovieClip,lineTo)
{
	LOG(NOT_IMPLEMENTED,"Called lineTo");
	return NULL;
}

ASFUNCTIONBODY(ASMovieClip,lineStyle)
{
	LOG(NOT_IMPLEMENTED,"Called lineStyle");
	return NULL;
}

ASFUNCTIONBODY(ASMovieClip,swapDepths)
{
	LOG(NOT_IMPLEMENTED,"Called swapDepths");
	return NULL;
}

void ASMovieClip::_register()
{
	setVariableByName("_visible",SWFObject(&_visible,true));
	setVariableByName("y",SWFObject(&_y,true));
	setVariableByName("x",SWFObject(&_x,true));
	setVariableByName("width",SWFObject(&_width,true));
	setVariableByName("height",SWFObject(&_height,true));
	setVariableByName("_framesloaded",SWFObject(&_framesloaded,true));
	setVariableByName("_totalframes",SWFObject(&_totalframes,true));
	setVariableByName("swapDepths",SWFObject(new Function(swapDepths)));
	setVariableByName("lineStyle",SWFObject(new Function(lineStyle)));
	setVariableByName("lineTo",SWFObject(new Function(lineTo)));
	setVariableByName("moveTo",SWFObject(new Function(moveTo)));
	setVariableByName("createEmptyMovieClip",SWFObject(new Function(createEmptyMovieClip)));
	setVariableByName("addEventListener",SWFObject(new Function(addEventListener)));
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

	if(!state.stop_FP)
		state.next_FP=min(state.FP+1,frames.size()-1);
	else
		state.next_FP=state.FP;

	list<Frame>::iterator frame=frames.begin();
	for(int i=0;i<state.FP;i++)
		frame++;

	//Apply local transformation
	glPushMatrix();
	glTranslatef(_x,_y,0);
	frame->Render(displayListLimit);

	glPopMatrix();

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

Number::Number(const ISWFObject* obj)
{
	const Integer* i=dynamic_cast<const Integer*>(obj);
	if(i)
	{
		val=i->val;
		return;
	}
	const Number* n=dynamic_cast<const Number*>(obj);
	if(n)
	{
		val=n->val;
		return;
	}

	cout << obj->getObjectType() << endl;
	abort();
}

STRING Number::toString()
{
	char buf[20];
	snprintf(buf,20,"%g",val);
	return STRING(buf);
}

float Number::toFloat()
{
	return val;
}

int Number::toInt()
{
	return val;
}

