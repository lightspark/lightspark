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
#include <sstream>
#include <iomanip>

#include "asobjects.h"
#include "flashevents.h"
#include "swf.h"

using namespace std;

extern __thread SystemState* sys;

ASStage::ASStage():width(640),height(480)
{
	setVariableByName("width",&width);
	setVariableByName("height",&height);
}

void ASArray::_register()
{
	setVariableByName("constructor",new Function(constructor));
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
	setVariableByName("constructor",new Function(constructor));
	setVariableByName("addListener",new Function(addListener));
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
	setVariableByName("constructor",new Function(constructor));
	setVariableByName("load",new Function(load));
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
	string base("www.youtube.com");
	string url=base+args->args[0]->toString();
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
	setVariableByName("constructor",new Function(constructor));
}

ASFUNCTIONBODY(ASObject,constructor)
{
	LOG(NOT_IMPLEMENTED,"Called Object constructor");
	return NULL;
}

ASString::ASString()
{
	setVariableByName("Call",new Function(ASString::String));
}

ASString::ASString(const string& s):data(s)
{
	setVariableByName("Call",new Function(ASString::String));
}

arguments::~arguments()
{
	for(int i=0;i<args.size();i++)
		args[i]->decRef();
}

ASFUNCTIONBODY(ASString,String)
{
	ASString* th=dynamic_cast<ASString*>(obj);
	if(args->args[0]->getObjectType()==T_DOUBLE)
	{
		Number* n=dynamic_cast<Number*>(args->args[0]);
		ostringstream oss;
		oss << setprecision(8) << fixed << *n;

		th->data=oss.str();
		return th;
	}
	else
	{
		LOG(CALLS,"Cannot convert " << args->args[0]->getObjectType() << " to String");
		abort();
	}
}

string ASString::toString() const
{
	return data.data();
}

float ASString::toFloat()
{
	LOG(ERROR,"Cannot convert string " << data << " to float");
	return 0;
}

ASMovieClip::ASMovieClip():_visible(1),_x(0),_y(0),_height(100),_width(100),_framesloaded(0),_totalframes(1),
	displayListLimit(0),rotation(0.0)
{
	sem_init(&sem_frames,0,1);
	class_name="MovieClip";
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
	map<string, IFunction*>::iterator h=handlers.find(e->type);
	if(h==handlers.end())
	{
		LOG(NOT_IMPLEMENTED,"Not handled event");
		return;
	}

	LOG(CALLS, "MovieClip event " << h->first);
	arguments args;
	args.args.push_back(e);
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

	Function* f=dynamic_cast<Function*>(args->args[1]);
	Function* f2=(Function*)f->clone();
	f2->bind();

	th->handlers.insert(make_pair(args->args[0]->toString(),f2->toFunction()));
	sys->events_name.push_back(args->args[0]->toString());
}

ASFUNCTIONBODY(ASMovieClip,createEmptyMovieClip)
{
	LOG(NOT_IMPLEMENTED,"createEmptyMovieClip");
	return new Undefined;
/*	ASMovieClip* th=dynamic_cast<ASMovieClip*>(obj);
	if(th==NULL)
		LOG(ERROR,"Not a valid ASMovieClip");

	LOG(CALLS,"Called createEmptyMovieClip: " << args->args[0]->toString() << " " << args->args[1]->toString());
	ASMovieClip* ret=new ASMovieClip();

	IDisplayListElem* t=new ASObjectWrapper(ret,args->args[1]->toInt());
	list<IDisplayListElem*>::iterator it=lower_bound(th->dynamicDisplayList.begin(),th->dynamicDisplayList.end(),t->getDepth(),list_orderer);
	th->dynamicDisplayList.insert(it,t);

	th->setVariableByName(args->args[0]->toString(),ret);
	return ret;*/
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
	setVariableByName("_visible",&_visible);
	setVariableByName("y",&_y);
	setVariableByName("x",&_x);
	setVariableByName("width",&_width);
	rotation.bind();
	setVariableByName("rotation",&rotation,true);
	setVariableByName("height",&_height);
	setVariableByName("_framesloaded",&_framesloaded);
	setVariableByName("_totalframes",&_totalframes);
	setVariableByName("swapDepths",new Function(swapDepths));
	setVariableByName("lineStyle",new Function(lineStyle));
	setVariableByName("lineTo",new Function(lineTo));
	setVariableByName("moveTo",new Function(moveTo));
	setVariableByName("createEmptyMovieClip",new Function(createEmptyMovieClip));
	setVariableByName("addEventListener",new Function(addEventListener));
}

void ASMovieClip::Render()
{
	LOG(TRACE,"Render MovieClip");
	parent=sys->currentClip;
	ASMovieClip* clip_bak=sys->currentClip;
	sys->currentClip=this;

	if(!state.stop_FP && class_name=="MovieClip")
		state.next_FP=min(state.FP+1,frames.size()-1);
	else
		state.next_FP=state.FP;

	list<Frame>::iterator frame=frames.begin();

	//Apply local transformation
	glPushMatrix();
	//glTranslatef(_x,_y,0);
	glRotatef(rotation,0,0,1);
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

void Number::copyFrom(const ISWFObject* o)
{
	if(o->getObjectType()!=T_DOUBLE)
	{
		LOG(ERROR,"Copying Number from type " << o->getObjectType() << " is not supported");
		abort();
	}
	
	const Number* n=dynamic_cast<const Number*>(o);

	val=n->val;
	LOG(TRACE,"Set to " << n->val);
}

bool Number::isLess(const ISWFObject* o) const
{
	if(o->getObjectType()==T_INTEGER)
	{
		const Integer* i=dynamic_cast<const Integer*>(o);
		return val<*i;
	}
	else
	{
		return ISWFObject::isLess(o);
	}
}

string Number::toString() const
{
	char buf[20];
	snprintf(buf,20,"%g",val);
	return string(buf);
}

float Number::toFloat()
{
	return val;
}

int Number::toInt()
{
	return val;
}

