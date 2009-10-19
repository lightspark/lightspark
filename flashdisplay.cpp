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

#include "abc.h"
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
	ASObject* ret=new EventDispatcher;
	obj->setVariableByQName("sharedEvents","",ret); //TODO: Read only
	ret->constructor->call(ret,NULL);
	ASObject* params=new ASObject("Object");
	obj->setVariableByQName("parameters","",params);
	params->setVariableByQName("connect","",new ASString("true"));
//	{"chat_bosh_port":"80","chat_username":encodeURIComponent(active_user.chatUsername()),"channel_id":encodeURIComponent(channel_id),"permission_slk":"false","chat_password":encodeURIComponent(active_user.chatPassword()),"enable_bosh":encodeURIComponent(active_user.boshEnabled()),"user_vars":encodeURIComponent(active_user.userVars()),"chat_ip":"216.246.59.237","permission_mtx_api":"false","enable_compression":encodeURIComponent(active_user.compressionEnabled()),"chat_host":"of1.kongregate.com","api_host":"http%3A%2F%2Fapi.kongregate.com","permission_chat_api":"false","game_permalink":"cube-colossus","game_title":"Cube%20Colossus","chat_port":"5222","game_auth_token":encodeURIComponent(active_user.gameAuthToken()),"permission_sc_api":"true","user_vars_sig":encodeURIComponent(active_user.userVarsSig()),"javascript_listener":"konduitToHolodeck","chat_bosh_host":"of1.kongregate.com","connect":"true","game_id":"53988","game_url":"http%3A%2F%2Fwww.kongregate.com%2Fgames%2Flucidrine%2Fcube-colossus","debug_level":encodeURIComponent(active_user.debugLevel())},{"allowscriptaccess":"always","allownetworking":"all"},{});

}

ASFUNCTIONBODY(Loader,_constructor)
{
	Loader* th=static_cast<Loader*>(obj);
	th->contentLoaderInfo=new LoaderInfo;
	obj->setVariableByQName("contentLoaderInfo","",th->contentLoaderInfo);
	th->contentLoaderInfo->constructor->call(th->contentLoaderInfo,NULL);

	obj->setVariableByQName("load","",new Function(load));
	obj->setVariableByQName("loadBytes","",new Function(loadBytes));
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
	th->source=URL;
	sys->cur_thread_pool->addJob(th);
}

ASFUNCTIONBODY(Loader,loadBytes)
{
	Loader* th=static_cast<Loader*>(obj);
	if(th->loading)
		return NULL;
	//Find the actual ByteArray object
	ASObject* cur=args->at(0);
	assert(cur);
	cur=cur->prototype;
	while(cur->class_name!="ByteArray")
		cur=cur->super;
	th->bytes=dynamic_cast<ByteArray*>(cur);
	assert(th->bytes);
	if(th->bytes->bytes)
	{
		th->loading=true;
		th->source=BYTES;
		sys->cur_thread_pool->addJob(th);
	}

	/*if(args->at(0)->class_name!="URLRequest")
	{
		LOG(ERROR,"ArgumentError");
		abort();
	}
	URLRequest* r=static_cast<URLRequest*>(args->at(0));*/
}

void Loader::execute()
{
	static char name[]="0puppa";
	LOG(NOT_IMPLEMENTED,"Loader async execution " << url);
	if(source==URL)
	{
		local_root=new RootMovieClip;
		zlib_file_filter zf;
		zf.open(url.c_str(),ios_base::in);
		istream s(&zf);

		ParseThread local_pt(sys,local_root,s);
		local_pt.wait();
	}
	else if(source==BYTES)
	{
		//Implement loadBytes, now just dump
		assert(bytes->bytes);

		FILE* f=fopen(name,"w");
		fwrite(bytes->bytes,1,bytes->len,f);
		fclose(f);

		name[0]++;
	}
	loaded=true;
	//Add a complete event for this object
	sys->currentVm->addEvent(contentLoaderInfo,new Event("complete"));

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

Sprite::Sprite():_visible(1),_x(0),_y(0),rotation(0.0)
{
	class_name="Sprite";
	if(constructor)
		constructor->decRef();
	constructor=new Function(_constructor);
}

ASFUNCTIONBODY(Sprite,_constructor)
{
	Sprite* th=static_cast<Sprite*>(obj);
	EventDispatcher::_constructor(th,NULL);
	th->setVariableByQName("root","",new Null);
	if(sys)
		sys->incRef();
	th->setVariableByQName("stage","",sys);
	//th->setVariableByName("_visible",&th->_visible);
	//th->setVariableByName("width",&th->_width);
	//th->setVariableByName("height",&th->_height);
	th->setVariableByQName("getBounds","",new Function(getBounds));
	th->setGetterByQName("rotation","",new Function(getRotation));
	th->setSetterByQName("rotation","",new Function(setRotation));
	th->setGetterByQName("parent","",new Function(_getParent));
	th->setGetterByQName("y","",new Function(getY));
	th->setGetterByQName("x","",new Function(getX));
}

ASFUNCTIONBODY(Sprite,getY)
{
	Sprite* th=static_cast<Sprite*>(obj);
	return new Number(th->_y);
}

ASFUNCTIONBODY(Sprite,getX)
{
	Sprite* th=static_cast<Sprite*>(obj);
	return new Number(th->_x);
}

ASFUNCTIONBODY(Sprite,setRotation)
{
	Sprite* th=static_cast<Sprite*>(obj);
	th->rotation=args->at(0)->toNumber();
}

ASFUNCTIONBODY(Sprite,getRotation)
{
	Sprite* th=static_cast<Sprite*>(obj);
	return new Number(th->rotation);
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
	//class_name="MovieClip";
	if(constructor)
		constructor->decRef();
	constructor=new Function(_constructor);
}

void MovieClip::addToFrame(DisplayListTag* t)
{
/*	list<IDisplayListElem*>::iterator it=lower_bound(displayList.begin(),displayList.end(),t->getDepth(),list_orderer);
	displayList.insert(it,t);
	displayListLimit=displayList.size();

	t->root=root;
	ASObject* o=static_cast<ASObject*>(t);
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

	ASObject* cur=args->at(0);
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
	args->at(0)->setVariableByQName("root","",th->root);
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
/*	MovieClip* th=static_cast<MovieClip*>(obj);
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
	th->setVariableByQName("_framesloaded","",&th->_framesloaded);
	th->setVariableByQName("framesLoaded","",&th->_framesloaded);
	th->setVariableByQName("_totalframes","",&th->_totalframes);
	th->setVariableByQName("totalFrames","",&th->_totalframes);
	th->setVariableByQName("swapDepths","",new Function(swapDepths));
	th->setVariableByQName("lineStyle","",new Function(lineStyle));
	th->setVariableByQName("lineTo","",new Function(lineTo));
	th->setVariableByQName("moveTo","",new Function(moveTo));
	th->setVariableByQName("createEmptyMovieClip","",new Function(createEmptyMovieClip));
	th->setVariableByQName("addFrameScript","",new Function(addFrameScript));
	th->setVariableByQName("addChild","",new Function(addChild));
	th->setVariableByQName("stop","",new Function(stop));

	cout << "curframe cons on " << th << endl;
	th->setGetterByQName("currentFrame","",new Function(_currentFrame));
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

	if(!state.stop_FP /*&& (class_name=="MovieClip")*/)
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

DisplayObject::DisplayObject():height(100),width(100)
{
	setVariableByQName("Call","",new Function(_call));
	setGetterByQName("width","",new Function(_getWidth));
	setGetterByQName("height","",new Function(_getHeight));
}

ASFUNCTIONBODY(DisplayObject,_call)
{
	LOG(CALLS,"DisplayObject Call");
	return new Undefined;
}

ASFUNCTIONBODY(DisplayObject,_getWidth)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	return abstract_i(th->width);
}

ASFUNCTIONBODY(DisplayObject,_getHeight)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj);
	return abstract_i(th->height);
}

DisplayObjectContainer::DisplayObjectContainer()
{
	cout << "DisplayObjectContainer constructor" << endl;
	setGetterByQName("numChildren","",new Function(_getNumChildren));
}

ASFUNCTIONBODY(DisplayObjectContainer,_getNumChildren)
{
	return new Integer(0);;
}
