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
#include <fstream>
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>

#include "flashdisplay.h"
#include "swf.h"
#include "flashgeom.h"
#include "flashnet.h"
#include "streams.h"

using namespace std;

extern __thread SystemState* sys;
extern __thread RenderThread* rt;
extern __thread ParseThread* pt;

ASFUNCTIONBODY(LoaderInfo,_constructor)
{
	EventDispatcher::_constructor(obj,args);
	ISWFObject* ret=obj->setVariableByName("sharedEvents",new EventDispatcher); //TODO: Read only
	ret->constructor->call(ret,NULL);
//	setVariableByName("parameters",&parameters);

}

ASFUNCTIONBODY(Loader,_constructor)
{
	ISWFObject* ret=obj->setVariableByName("contentLoaderInfo",new LoaderInfo);
	ret->bind();
	obj->setVariableByName("load",new Function(load));
	ret->constructor->call(ret,NULL);
}

ASFUNCTIONBODY(Loader,load)
{
	Loader* th=static_cast<Loader*>(obj);
/*	if(th->loading)
		return NULL;
	th->loading=true;*/
	if(args->at(0)->class_name!="URLRequest")
	{
		LOG(ERROR,"ArgumentError");
		abort();
	}
	URLRequest* r=static_cast<URLRequest*>(args->at(0));
	th->url=r->url->toString();
	sys->cur_thread_pool->addJob(th);
}

void Loader::execute()
{
	LOG(NOT_IMPLEMENTED,"Loader async execution " << url);
	local_root=new RootMovieClip;
	zlib_file_filter zf;
	zf.open(url.c_str(),ios_base::in);
	istream s(&zf);

	ParseThread local_pt(sys,local_root,s);
	local_pt.wait();
	loaded=true;
/*	CURL *curl;
	CURLcode res;
	curl = curl_easy_init();
	string base("www.youtube.com");
	string url=base+args->at(0)->toString();
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
	a.push(new Integer(1));
	on_load->call(NULL,&a);*/
}

void Loader::Render()
{
	if(!loaded)
		return;

	local_root->Render();
}

Sprite::Sprite():_visible(1),_x(0),_y(0),_height(100),_width(100),rotation(0.0)
{
	class_name="Sprite";
	constructor=new Function(_constructor);
}

ASFUNCTIONBODY(Sprite,_constructor)
{
	Sprite* th=static_cast<Sprite*>(obj);
	EventDispatcher::_constructor(th,NULL);
	th->setVariableByName("root",new Null);
	th->setVariableByName("stage",sys);
	th->setVariableByName("_visible",&th->_visible);
	th->setVariableByName("y",&th->_y);
	th->setVariableByName("x",&th->_x);
	th->setVariableByName("width",&th->_width);
	th->rotation.bind();
	th->setVariableByName("rotation",&th->rotation,true);
	th->setVariableByName("height",&th->_height);
	th->setVariableByName("getBounds",new Function(getBounds));
	th->setGetterByName("parent",new Function(_getParent));
}

ASFUNCTIONBODY(Sprite,getBounds)
{
	LOG(NOT_IMPLEMENTED,"Calculate real bounds");
	return new Rectangle(0,0,100,100);
}

ASFUNCTIONBODY(Sprite,_getParent)
{
	Sprite* th=static_cast<Sprite*>(obj);
	if(th->parent==NULL)
		return new Undefined;

	return th->parent;
}

MovieClip::MovieClip():_framesloaded(0),_totalframes(1),cur_frame(&dynamicDisplayList),initialized(false)
{
	class_name="MovieClip";
	constructor=new Function(_constructor);
}

void MovieClip::addToFrame(DisplayListTag* t)
{
/*	list<IDisplayListElem*>::iterator it=lower_bound(displayList.begin(),displayList.end(),t->getDepth(),list_orderer);
	displayList.insert(it,t);
	displayListLimit=displayList.size();

	t->root=root;
	ASObject* o=dynamic_cast<ASObject*>(t);
	if(o)
		o->setVariableByName("root",this,true);*/

	cur_frame.blueprint.push_back(t);
}

ASFUNCTIONBODY(MovieClip,addChild)
{
	MovieClip* th=static_cast<MovieClip*>(obj);
	if(args->at(0)->parent==th)
		return args->at(0);
	else if(args->at(0)->parent!=NULL)
	{
		//Remove from current parent list
		abort();
	}

	ASObject* cur=dynamic_cast<ASObject*>(args->at(0));
	if(cur->getObjectType()==T_DEFINABLE)
		abort();
	IDisplayListElem* e=NULL;
	//Look for a renderable object in the super chain
	do
	{
		e=dynamic_cast<IDisplayListElem*>(cur);
		cur=cur->super;
	}
	while(e==NULL && cur!=NULL);

	if(e==NULL)
	{
		LOG(ERROR,"Cannot add arg to display list");
		abort();
	}
	args->at(0)->parent=th;
	th->dynamicDisplayList.push_back(e);

	e->root=th->root;
	args->at(0)->setVariableByName("root",th->root,true);
}

ASFUNCTIONBODY(MovieClip,addFrameScript)
{
	MovieClip* th=static_cast<MovieClip*>(obj);
	if(args->size()%2)
	{
		LOG(ERROR,"Invalid arguments to addFrameScript");
		abort();
	}
	for(int i=0;i<args->size();i+=2)
	{
		int f=args->at(i+0)->toInt();
		IFunction* g=args->at(i+1)->toFunction();

		//Should wait for frames to be received
		if(f>=th->frames.size())
		{
			LOG(ERROR,"Invalid frame number passed to addFrameScript");
			abort();
		}

		th->frames[f].setScript(g);
	}
}

ASFUNCTIONBODY(MovieClip,createEmptyMovieClip)
{
	LOG(NOT_IMPLEMENTED,"createEmptyMovieClip");
	return new Undefined;
/*	MovieClip* th=dynamic_cast<MovieClip*>(obj);
	if(th==NULL)
		LOG(ERROR,"Not a valid MovieClip");

	LOG(CALLS,"Called createEmptyMovieClip: " << args->args[0]->toString() << " " << args->args[1]->toString());
	MovieClip* ret=new MovieClip();

	IDisplayListElem* t=new ASObjectWrapper(ret,args->args[1]->toInt());
	list<IDisplayListElem*>::iterator it=lower_bound(th->dynamicDisplayList.begin(),th->dynamicDisplayList.end(),t->getDepth(),list_orderer);
	th->dynamicDisplayList.insert(it,t);

	th->setVariableByName(args->args[0]->toString(),ret);
	return ret;*/
}

ASFUNCTIONBODY(MovieClip,moveTo)
{
	LOG(NOT_IMPLEMENTED,"Called moveTo");
	return NULL;
}

ASFUNCTIONBODY(MovieClip,lineTo)
{
	LOG(NOT_IMPLEMENTED,"Called lineTo");
	return NULL;
}

ASFUNCTIONBODY(MovieClip,lineStyle)
{
	LOG(NOT_IMPLEMENTED,"Called lineStyle");
	return NULL;
}

ASFUNCTIONBODY(MovieClip,swapDepths)
{
	LOG(NOT_IMPLEMENTED,"Called swapDepths");
	return NULL;
}

ASFUNCTIONBODY(MovieClip,stop)
{
	MovieClip* th=static_cast<MovieClip*>(obj);
	th->state.stop_FP=true;
}

ASFUNCTIONBODY(MovieClip,_currentFrame)
{
	MovieClip* th=static_cast<MovieClip*>(obj);
	//currentFrame is 1-based
	return new Integer(th->state.FP+1);
}

ASFUNCTIONBODY(MovieClip,_constructor)
{
	MovieClip* th=static_cast<MovieClip*>(obj);
	Sprite::_constructor(th,NULL);
	th->setVariableByName("_framesloaded",&th->_framesloaded);
	th->setVariableByName("framesLoaded",&th->_framesloaded);
	th->_framesloaded.bind();
	th->setVariableByName("_totalframes",&th->_totalframes);
	th->setVariableByName("totalFrames",&th->_totalframes);
	th->_totalframes.bind();
	th->setVariableByName("swapDepths",new Function(swapDepths));
	th->setVariableByName("lineStyle",new Function(lineStyle));
	th->setVariableByName("lineTo",new Function(lineTo));
	th->setVariableByName("moveTo",new Function(moveTo));
	th->setVariableByName("createEmptyMovieClip",new Function(createEmptyMovieClip));
	th->setVariableByName("addFrameScript",new Function(addFrameScript));
	th->setVariableByName("addChild",new Function(addChild));
	th->setVariableByName("stop",new Function(stop));

	cout << "curframe cons on " << th << endl;
	th->setGetterByName("currentFrame",new Function(_currentFrame));
}

void MovieClip::Render()
{
	LOG(TRACE,"Render MovieClip");
	parent=rt->currentClip;
	MovieClip* clip_bak=rt->currentClip;
	rt->currentClip=this;

	if(!initialized)
	{
		initialize();
		initialized=true;
	}

	if(!state.stop_FP /*&& (class_name=="MovieClip" || class_name=="SystemState")*/)
		state.next_FP=min(state.FP+1,frames.size()-1); //TODO: use framecount
	else
		state.next_FP=state.FP;

	if(!frames.empty())
	{
		if(!state.stop_FP)
			frames[state.FP].runScript();

		//Set the id in the secondary color
		glPushAttrib(GL_CURRENT_BIT);
		glSecondaryColor3f(id,0,0);

		//Apply local transformation
		glPushMatrix();
		//glTranslatef(_x,_y,0);
		glRotatef(rotation,0,0,1);
		frames[state.FP].Render();

		glPopMatrix();
		glPopAttrib();
	}

	if(state.FP!=state.next_FP)
		state.FP=state.next_FP;
	else
		state.stop_FP=true;
	LOG(TRACE,"End Render MovieClip");

	rt->currentClip=clip_bak;
}

void MovieClip::initialize()
{
	if(!initialized)
	{
		for(int i=0;i<frames.size();i++)
			frames[i].init(this,displayList);
		initialized=true;
	}
}

DisplayObject::DisplayObject()
{
	setVariableByName("Call",new Function(_call));
}

ASFUNCTIONBODY(DisplayObject,_call)
{
	LOG(CALLS,"DisplayObject Call");
	return new Undefined;
}
