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
#include <GL/glew.h>

#include "abc.h"
#include "flashdisplay.h"
#include "swf.h"
#include "flashgeom.h"
#include "flashnet.h"
#include "streams.h"
#include "compat.h"
#include "class.h"

using namespace std;
using namespace lightspark;

extern TLSDATA SystemState* sys;
extern TLSDATA RenderThread* rt;
extern TLSDATA ParseThread* pt;

REGISTER_CLASS_NAME(LoaderInfo);
REGISTER_CLASS_NAME(MovieClip);
REGISTER_CLASS_NAME(DisplayObject);
REGISTER_CLASS_NAME(DisplayObjectContainer);
REGISTER_CLASS_NAME(Sprite);
REGISTER_CLASS_NAME(Loader);
REGISTER_CLASS_NAME(Shape);
REGISTER_CLASS_NAME(Stage);
REGISTER_CLASS_NAME(Graphics);

void LoaderInfo::sinit(Class_base* c)
{
	assert(c->constructor==NULL);
	c->constructor=new Function(_constructor);
}

ASFUNCTIONBODY(LoaderInfo,_constructor)
{
	EventDispatcher::_constructor(obj,args);
/*	ASObject* ret=new EventDispatcher;
	obj->setVariableByQName("sharedEvents","",ret); //TODO: Read only
	ret->constructor->call(ret,NULL);*/
	ASObject* params=new ASObject;
	obj->setVariableByQName("parameters","",params);
	obj->setGetterByQName("loaderURL","",new Function(_getLoaderUrl));
	obj->setGetterByQName("url","",new Function(_getUrl));
	obj->setGetterByQName("bytesLoaded","",new Function(_getBytesLoaded));
	obj->setGetterByQName("bytesTotal","",new Function(_getBytesTotal));

	params->setVariableByQName("length_seconds","",new ASString("558"));
	params->setVariableByQName("cr","",new ASString("US"));
	params->setVariableByQName("plid","",new ASString("AAR2WiuKNxt32IdW"));
	params->setVariableByQName("t","",new ASString("vjVQa1PpcFOpt5byKez5Rz71Hui_DHUbc_z_AtcWBa0="));
	params->setVariableByQName("vq","",new ASString("medium"));
	params->setVariableByQName("sk","",new ASString("PaOie8-uH5AhsKIdWMmBUtdP4LkQJERtC"));
	params->setVariableByQName("video_id","",new ASString("G4S9tV8ZLcE"));
	params->setVariableByQName("fmt_map","",new ASString("34/0/9/0/115,5/0/7/0/0"));
	params->setVariableByQName("fmt_url_map","",new ASString("34|http://v9.lscache2.c.youtube.com/videoplayback?ip=0.0.0.0&sparams=id%2Cexpire%2Cip%2Cipbits%2Citag%2Calgorithm%2Cburst%2Cfactor&fexp=903900%2C900025%2C905207&algorithm=throttle-factor&itag=34&ipbits=0&signature=A45863BF4EEB761129156DB5275FD1B69119666F.7633A2E1E476044B0D313C22E5641B2B6FBD3601&sver=3&expire=1256054400&key=yt1&factor=1.25&burst=40&id=1b84bdb55f192dc1,5|http://v3.lscache8.c.youtube.com/videoplayback?ip=0.0.0.0&sparams=id%2Cexpire%2Cip%2Cipbits%2Citag%2Calgorithm%2Cburst%2Cfactor&fexp=903900%2C900025%2C905207&algorithm=throttle-factor&itag=5&ipbits=0&signature=D04298060B1F95665DB6622292E34D316C23D761.8E3DB733F92F8C90BEB8EAE558516067B76D32CB&sver=3&expire=1256054400&key=yt1&factor=1.25&burst=40&id=1b84bdb55f192dc1"));
//rv.2.thumbnailUrl": "http%3A%2F%2Fi1.ytimg.com%2Fvi%2FxWyMjzVXq7Y%2Fdefault.jpg"
//"rv.7.length_seconds": "439",
//"rv.0.url": "http%3A%2F%2Fwww.youtube.com%2Fwatch%3Fv%3D15lKSqY86Ew", 
//"rv.0.view_count": "194",
//"rv.2.title": "Paul+Mawhinney+Records+Largest+Collection+in+the+World", 
//"rv.7.thumbnailUrl": "http%3A%2F%2Fi3.ytimg.com%2Fvi%2Fza7e9rwF_Cc%2Fdefault.jpg",
// "rv.4.rating": "4.92647058824",
//"rv.7.title": "Using+glue+to+clean+vinyl+record+albums", 
//"rv.3.view_count": "20281", 
//"allow_embed": "1", 
//"rv.5.title": "T+Pain+Obama+Auto-Tune", 
//"is_doubleclick_tracked": "1", 
//"rv.4.thumbnailUrl": "http%3A%2F%2Fi3.ytimg.com%2Fvi%2F2j7F_4S2lgM%2Fdefault.jpg"
//"fmt_map": "34%2F0%2F9%2F0%2F115%2C5%2F0%2F7%2F0%2F0", 
//"fmt_url_map": "34%7Chttp%3A%2F%2Fv9.lscache2.c.youtube.com%2Fvideoplayback%3Fip%3D0.0.0.0%26sparams%3Did%252Cexpire%252Cip%252Cipbits%252Citag%252Calgorithm%252Cburst%252Cfactor%26fexp%3D903900%252C900025%252C905207%26algorithm%3Dthrottle-factor%26itag%3D34%26ipbits%3D0%26signature%3DA45863BF4EEB761129156DB5275FD1B69119666F.7633A2E1E476044B0D313C22E5641B2B6FBD3601%26sver%3D3%26expire%3D1256054400%26key%3Dyt1%26factor%3D1.25%26burst%3D40%26id%3D1b84bdb55f192dc1%2C5%7Chttp%3A%2F%2Fv3.lscache8.c.youtube.com%2Fvideoplayback%3Fip%3D0.0.0.0%26sparams%3Did%252Cexpire%252Cip%252Cipbits%252Citag%252Calgorithm%252Cburst%252Cfactor%26fexp%3D903900%252C900025%252C905207%26algorithm%3Dthrottle-factor%26itag%3D5%26ipbits%3D0%26signature%3DD04298060B1F95665DB6622292E34D316C23D761.8E3DB733F92F8C90BEB8EAE558516067B76D32CB%26sver%3D3%26expire%3D1256054400%26key%3Dyt1%26factor%3D1.25%26burst%3D40%26id%3D1b84bdb55f192dc1", 
//"rv.0.length_seconds": "232", 
//"rv.2.rating": "4.91304347826", 
//"keywords": "Pack%2CRats%2CVinyl%2CPAUL", 
//"rv.1.url": "http%3A%2F%2Fwww.youtube.com%2Fwatch%3Fv%3DuBLuMVOr3nw", 
//"rv.6.thumbnailUrl": "http%3A%2F%2Fi2.ytimg.com%2Fvi%2FmolqV4L23FM%2Fdefault.jpg", 
//"rv.1.id": "uBLuMVOr3nw", 
//"rv.3.rating": "4.93258426966", 
//"rv.6.title": "How+Vinyl+Records+Are+Made", 
//"rv.7.id": "za7e9rwF_Cc", 
//"rv.1.title": "%27The+Archive%27+by+Sean+Dunne", 
//"rv.1.thumbnailUrl": "http%3A%2F%2Fi2.ytimg.com%2Fvi%2FuBLuMVOr3nw%2Fdefault.jpg", 
//"rv.5.view_count": "994171", 
//"rv.0.rating": "5.0", 
//"watermark": "http%3A%2F%2Fs.ytimg.com%2Fyt%2Fswf%2Flogo-vfl106645.swf%2Chttp%3A%2F%2Fs.ytimg.com%2Fyt%2Fswf%2Fhdlogo-vfl100714.swf", 
//"rv.6.author": "RockItOutBlog", 
//"rv.3.title": "Vinyl+LP+records+-+still+spinning+after+60+years%21", 
//"rv.5.id": "ITT6bYYGVfM", 
//"rv.4.author": "ROCKETBOOM", 
//"rv.0.id": "15lKSqY86Ew", 
//"rv.3.length_seconds": "410", 
//"rv.5.rating": "4.82385398981", 
//"rv.4.url": "http%3A%2F%2Fwww.youtube.com%2Fwatch%3Fv%3D2j7F_4S2lgM", 
//"fexp": "903900%2C900025%2C905207", 
//"rv.1.author": "ithentic", 
//"rv.1.rating": "5.0", 
//"rv.4.title": "The+World%27s+Largest+Record+Collection", 
//"rv.5.thumbnailUrl": "http%3A%2F%2Fi2.ytimg.com%2Fvi%2FITT6bYYGVfM%2Fdefault.jpg", 
//"rv.5.url": "http%3A%2F%2Fwww.youtube.com%2Fwatch%3Fv%3DITT6bYYGVfM", 
//"rv.6.length_seconds": "405", 
//"rv.0.title": "Jerry+Weber+of+Jerry%27s+Records+on+%22Pack+Rats%22", 
//"rv.0.author": "dougpaynedotcom", 
//"rv.3.thumbnailUrl": "http%3A%2F%2Fi1.ytimg.com%2Fvi%2FtggLYE87Ed0%2Fdefault.jpg", 
//"rv.2.author": "Frunzenskaya", 
//"rv.6.url": "http%3A%2F%2Fwww.youtube.com%2Fwatch%3Fv%3DmolqV4L23FM", 
//"rv.7.rating": "5.0", 
//"rv.3.url": "http%3A%2F%2Fwww.youtube.com%2Fwatch%3Fv%3DtggLYE87Ed0", 
//"hl": "en", 
//"rv.7.url": "http%3A%2F%2Fwww.youtube.com%2Fwatch%3Fv%3Dza7e9rwF_Cc", 
//"rv.2.view_count": "6753", 
//"rv.4.length_seconds": "257", 
//"rv.4.view_count": "44120", 
//"rv.2.url": "http%3A%2F%2Fwww.youtube.com%2Fwatch%3Fv%3DxWyMjzVXq7Y", 
//"rv.5.length_seconds": "118", 
//"rv.0.thumbnailUrl": "http%3A%2F%2Fi2.ytimg.com%2Fvi%2F15lKSqY86Ew%2Fdefault.jpg", 
//"rv.7.author": "hrgiger6", 
//"rv.1.view_count": "535", 
//"rv.1.length_seconds": "456", 
//"rv.6.rating": "4.93965517241", 
//"rv.5.author": "JimmyKimmelLive", 
//"rv.3.id": "tggLYE87Ed0", 
//"rv.2.id": "xWyMjzVXq7Y", 
//"rv.2.length_seconds": "456", 
//"rv.6.id": "molqV4L23FM", 
//"rv.6.view_count": "2493", 
//"rv.3.author": "bctvguy", 
//"rv.4.id": "2j7F_4S2lgM", 
//"rv.7.view_count": "301"
	return NULL;
}

ASFUNCTIONBODY(LoaderInfo,_getLoaderUrl)
{
	return new ASString("www.youtube.com");
}

ASFUNCTIONBODY(LoaderInfo,_getUrl)
{
	return new ASString("www.youtube.com/watch.swf");
}

ASFUNCTIONBODY(LoaderInfo,_getBytesLoaded)
{
	LoaderInfo* th=static_cast<LoaderInfo*>(obj->implementation);
	return abstract_i(th->bytesLoaded);
}

ASFUNCTIONBODY(LoaderInfo,_getBytesTotal)
{
	LoaderInfo* th=static_cast<LoaderInfo*>(obj->implementation);
	return abstract_i(th->bytesTotal);
}

ASFUNCTIONBODY(Loader,_constructor)
{
	Loader* th=static_cast<Loader*>(obj->implementation);
	assert(th);
	th->contentLoaderInfo=Class<LoaderInfo>::getInstanceS();
	th->contentLoaderInfo->_constructor(th->contentLoaderInfo->obj,NULL);

	obj->setVariableByQName("contentLoaderInfo","",th->contentLoaderInfo->obj);

//	obj->setVariableByQName("load","",new Function(load));
//	obj->setVariableByQName("loadBytes","",new Function(loadBytes));
	return NULL;
}

ASFUNCTIONBODY(Loader,load)
{
	Loader* th=static_cast<Loader*>(obj->implementation);
/*	if(th->loading)
		return NULL;
	th->loading=true;*/
	abort();
/*	if(args->at(0)->getClassName()!="URLRequest")
	{
		LOG(ERROR,"ArgumentError");
		abort();
	}*/
	URLRequest* r=static_cast<URLRequest*>(args->at(0)->implementation);
	th->url=r->url;
	th->source=URL;
	sys->cur_thread_pool->addJob(th);
	return NULL;
}

ASFUNCTIONBODY(Loader,loadBytes)
{
	Loader* th=static_cast<Loader*>(obj->implementation);
	if(th->loading)
		return NULL;
	//Find the actual ByteArray object
	ASObject* cur=args->at(0);
	assert(cur);
	cur=cur->prototype;
	abort();
//	while(cur->getClassName()!="ByteArray")
//		cur=cur->super;
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

void Loader::sinit(Class_base* c)
{
	assert(c->constructor==NULL);
	c->constructor=new Function(_constructor);
}

void Loader::execute()
{
	static char name[]="0puppa";
	LOG(LOG_NOT_IMPLEMENTED,"Loader async execution " << url);
	if(source==URL)
	{
		abort();
		/*local_root=new RootMovieClip;
		zlib_file_filter zf;
		zf.open(url.raw_buf(),ios_base::in);
		istream s(&zf);

		ParseThread local_pt(sys,local_root,s);
		local_pt.wait();*/
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

Sprite::Sprite():graphics(NULL)
{
}

void Sprite::sinit(Class_base* c)
{
	assert(c->constructor==NULL);
	c->constructor=new Function(_constructor);
	c->super=Class<DisplayObjectContainer>::getClass();
}

void Sprite::Render()
{
	float matrix[16];
	Matrix.get4DMatrix(matrix);
	glPushMatrix();
	glMultMatrixf(matrix);

	//Draw the dynamically added graphics, if any
	if(graphics)
		graphics->Render();

	//Now draw also the display list
	list<IDisplayListElem*>::iterator it=dynamicDisplayList.begin();
	for(it;it!=dynamicDisplayList.end();it++)
		(*it)->Render();
	glPopMatrix();

}

ASFUNCTIONBODY(Sprite,_constructor)
{
	Sprite* th=static_cast<Sprite*>(obj->implementation);
	DisplayObjectContainer::_constructor(obj,NULL);

	obj->setGetterByQName("graphics","",new Function(_getGraphics));
	return NULL;
}

ASFUNCTIONBODY(Sprite,_getGraphics)
{
	Sprite* th=static_cast<Sprite*>(obj->implementation);
	//Probably graphics is not used often, so create it here
	if(th->graphics==NULL)
		th->graphics=Class<Graphics>::getInstanceS(true);

	th->graphics->obj->incRef();
	return th->graphics->obj;
}

void MovieClip::sinit(Class_base* c)
{
	assert(c->constructor==NULL);
	c->constructor=new Function(_constructor);
	c->super=Class<Sprite>::getClass();
}

MovieClip::MovieClip():framesLoaded(0),totalFrames(1),cur_frame(&dynamicDisplayList),initialized(false)
{
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

ASFUNCTIONBODY(MovieClip,addFrameScript)
{
	MovieClip* th=static_cast<MovieClip*>(obj->implementation);
	if(args->size()%2)
	{
		LOG(LOG_ERROR,"Invalid arguments to addFrameScript");
		abort();
	}
	for(int i=0;i<args->size();i+=2)
	{
		int f=args->at(i+0)->toInt();
		IFunction* g=args->at(i+1)->toFunction();

		//Should wait for frames to be received
		if(f>=th->frames.size())
		{
			LOG(LOG_ERROR,"Invalid frame number passed to addFrameScript");
			abort();
		}

		th->frames[f].setScript(g);
	}
	return NULL;
}

ASFUNCTIONBODY(MovieClip,createEmptyMovieClip)
{
	LOG(LOG_NOT_IMPLEMENTED,"createEmptyMovieClip");
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
	LOG(LOG_NOT_IMPLEMENTED,"Called moveTo");
	return NULL;
}

ASFUNCTIONBODY(MovieClip,lineTo)
{
	LOG(LOG_NOT_IMPLEMENTED,"Called lineTo");
	return NULL;
}

ASFUNCTIONBODY(MovieClip,lineStyle)
{
	LOG(LOG_NOT_IMPLEMENTED,"Called lineStyle");
	return NULL;
}

ASFUNCTIONBODY(MovieClip,swapDepths)
{
	LOG(LOG_NOT_IMPLEMENTED,"Called swapDepths");
	return NULL;
}

ASFUNCTIONBODY(MovieClip,stop)
{
	MovieClip* th=static_cast<MovieClip*>(obj->implementation);
	th->state.stop_FP=true;
	return NULL;
}

ASFUNCTIONBODY(MovieClip,nextFrame)
{
	MovieClip* th=static_cast<MovieClip*>(obj->implementation);
	//Quite not right, for the duration of this event at least the frame should be constant
	assert(th->state.FP<th->state.max_FP);
	th->state.FP++;
	return NULL;
}

ASFUNCTIONBODY(MovieClip,_getFramesLoaded)
{
	MovieClip* th=static_cast<MovieClip*>(obj->implementation);
	//currentFrame is 1-based
	return new Integer(th->framesLoaded);
}

ASFUNCTIONBODY(MovieClip,_getTotalFrames)
{
	MovieClip* th=static_cast<MovieClip*>(obj->implementation);
	//currentFrame is 1-based
	return new Integer(th->totalFrames);
}

ASFUNCTIONBODY(MovieClip,_getCurrentFrame)
{
	MovieClip* th=static_cast<MovieClip*>(obj->implementation);
	//currentFrame is 1-based
	return new Integer(th->state.FP+1);
}

ASFUNCTIONBODY(MovieClip,_constructor)
{
	MovieClip* th=static_cast<MovieClip*>(obj->implementation);
	Sprite::_constructor(obj,NULL);
	obj->setGetterByQName("currentFrame","",new Function(_getCurrentFrame));
	obj->setGetterByQName("totalFrames","",new Function(_getTotalFrames));
	obj->setGetterByQName("framesLoaded","",new Function(_getFramesLoaded));
	obj->setVariableByQName("stop","",new Function(stop));
	obj->setVariableByQName("nextFrame","",new Function(nextFrame));
/*	th->setVariableByQName("swapDepths","",new Function(swapDepths));
	th->setVariableByQName("lineStyle","",new Function(lineStyle));
	th->setVariableByQName("lineTo","",new Function(lineTo));
	th->setVariableByQName("moveTo","",new Function(moveTo));
	th->setVariableByQName("createEmptyMovieClip","",new Function(createEmptyMovieClip));
	th->setVariableByQName("addFrameScript","",new Function(addFrameScript));*/
	return NULL;
}

void MovieClip::Render()
{
	LOG(LOG_TRACE,"Render MovieClip");
//	parent=rt->currentClip;
	MovieClip* clip_bak=rt->currentClip;
	rt->currentClip=this;

	if(!initialized)
		initialize();

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
	LOG(LOG_TRACE,"End Render MovieClip");

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

void MovieClip::getBounds(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax)
{
	//Iterate over the displaylist of the current frame
	//TODO: add dynamic dysplay list
	std::list<std::pair<PlaceInfo, IDisplayListElem*> >::iterator it=frames[state.FP].displayList.begin();
	assert(frames[state.FP].displayList.size()==1);
	it->second->getBounds(xmin,xmax,ymin,ymax);
	//TODO: take rotation into account
	Matrix.multiply2D(xmin,ymin,xmin,ymin);
	Matrix.multiply2D(xmax,ymax,xmax,ymax);
}

DisplayObject::DisplayObject():height(100),width(100),loaderInfo(NULL),rotation(0.0),parent(NULL)
{
}

void DisplayObject::sinit(Class_base* c)
{
	assert(c->constructor==NULL);
	c->constructor=new Function(_constructor);
	c->constructor=new Function(_getX);
}

ASFUNCTIONBODY(DisplayObject,_getX)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj->implementation);
	return abstract_d(th->Matrix.TranslateX);
}

ASFUNCTIONBODY(DisplayObject,_setX)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj->implementation);
	assert(args && args->size()==1);
	th->Matrix.TranslateX=args->at(0)->toInt();
	return NULL;
}

ASFUNCTIONBODY(DisplayObject,_getY)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj->implementation);
	return abstract_d(th->Matrix.TranslateY);
}

ASFUNCTIONBODY(DisplayObject,_setY)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj->implementation);
	assert(args && args->size()==1);
	th->Matrix.TranslateY=args->at(0)->toInt();
	return NULL;
}

ASFUNCTIONBODY(DisplayObject,_getBounds)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj->implementation);
	Rectangle* ret=Class<Rectangle>::getInstanceS(true);
	number_t x1,x2,y1,y2;
	th->getBounds(x1,x2,y1,y2);

	//Bounds are in the form [XY]{min,max}
	//convert it to rect (x,y,width,height) representation
	ret->x=x1;
	ret->width=x2-x1;
	ret->y=y1;
	ret->height=y2-y1;
	return ret->obj;
}

ASFUNCTIONBODY(DisplayObject,_constructor)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj->implementation);
	EventDispatcher::_constructor(obj,NULL);
	if(th->loaderInfo)
	{
		assert(th->loaderInfo->obj!=NULL);
		th->loaderInfo->_constructor(th->loaderInfo->obj,NULL);
		obj->setVariableByQName("loaderInfo","",th->loaderInfo->obj);
	}

	obj->setGetterByQName("width","",new Function(_getWidth));
	obj->setGetterByQName("x","",new Function(_getX));
	obj->setGetterByQName("y","",new Function(_getY));
	obj->setGetterByQName("height","",new Function(_getHeight));
	obj->setGetterByQName("visible","",new Function(_getVisible));
	obj->setGetterByQName("rotation","",new Function(_getRotation));
	obj->setGetterByQName("name","",new Function(_getName));
	obj->setGetterByQName("parent","",new Function(_getParent));
	obj->setGetterByQName("root","",new Function(_getRoot));
	obj->setGetterByQName("blendMode","",new Function(_getBlendMode));
	obj->setGetterByQName("scale9Grid","",new Function(_getScale9Grid));
	obj->setGetterByQName("stage","",new Function(_getStage));
	obj->setVariableByQName("getBounds","",new Function(_getBounds));
	obj->setVariableByQName("localToGlobal","",new Function(localToGlobal));
	obj->setSetterByQName("width","",new Function(_setWidth));
	obj->setSetterByQName("name","",new Function(_setName));
	obj->setSetterByQName("x","",new Function(_setX));
	obj->setSetterByQName("y","",new Function(_setY));
	return NULL;
}

ASFUNCTIONBODY(DisplayObject,_getStage)
{
	assert(sys->stage);
	sys->stage->obj->incRef();
	return sys->stage->obj;
}

ASFUNCTIONBODY(DisplayObject,_getScale9Grid)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj->implementation);
	return new Undefined;
}

ASFUNCTIONBODY(DisplayObject,_getBlendMode)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj->implementation);
	return new Undefined;
}

ASFUNCTIONBODY(DisplayObject,localToGlobal)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj->implementation);
	return new Undefined;
}

ASFUNCTIONBODY(DisplayObject,_setRotation)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj->implementation);
	th->rotation=args->at(0)->toNumber();
	return NULL;
}

ASFUNCTIONBODY(DisplayObject,_setWidth)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj->implementation);
	th->width=args->at(0)->toInt();
	return NULL;
}

ASFUNCTIONBODY(DisplayObject,_setName)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj->implementation);
	return NULL;
}

ASFUNCTIONBODY(DisplayObject,_getName)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj->implementation);
	return new Undefined;
}

ASFUNCTIONBODY(DisplayObject,_getParent)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj->implementation);
	if(th->parent==NULL)
		return new Undefined;

	th->parent->obj->incRef();
	return th->parent->obj;
}

ASFUNCTIONBODY(DisplayObject,_getRoot)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj->implementation);
	assert(th->root->obj);
	th->root->obj->incRef();
	return th->root->obj;
}

ASFUNCTIONBODY(DisplayObject,_getRotation)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj->implementation);
	return new Number(th->rotation);
}

ASFUNCTIONBODY(DisplayObject,_getVisible)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj->implementation);
	return abstract_b(true);
}

ASFUNCTIONBODY(DisplayObject,_getWidth)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj->implementation);
	return abstract_i(th->width);
}

ASFUNCTIONBODY(DisplayObject,_getHeight)
{
	DisplayObject* th=static_cast<DisplayObject*>(obj->implementation);
	return abstract_i(th->height);
}

void DisplayObjectContainer::sinit(Class_base* c)
{
	assert(c->constructor==NULL);
	c->constructor=new Function(_constructor);
	c->super=Class<DisplayObject>::getClass();
}

DisplayObjectContainer::DisplayObjectContainer()
{
}

ASFUNCTIONBODY(DisplayObjectContainer,_constructor)
{
	DisplayObject::_constructor(obj,NULL);
	obj->setGetterByQName("numChildren","",new Function(_getNumChildren));
	obj->setVariableByQName("addChild","",new Function(addChild));
	obj->setVariableByQName("addChildAt","",new Function(addChildAt));
	return NULL;
}

ASFUNCTIONBODY(DisplayObjectContainer,_getNumChildren)
{
	return new Integer(0);;
}

void DisplayObjectContainer::_addChildAt(DisplayObject* child, int index)
{
	//For now just set the root variable
	//TODO: Interesting things such as inserting the object in the display list will be done later

	//Set the root of the movie to this container
	assert(child->root==NULL);
	assert(root==sys);
	child->root=root;

	//The HACK for this supports only Sprites now
	assert(isSubclass(child->obj ,Class<Sprite>::getClass()));

	//If the child has no parent, set this container to parent
	//If there is a previous parent, purge the child from his list
	assert(child->parent==NULL);
	if(child->parent==this) //Child already in this container
		return;
	child->parent=this;

	//We insert the object in the back of the list
	//TODO: support the 'at index' version of the call
	dynamicDisplayList.push_back(child);
}

ASFUNCTIONBODY(DisplayObjectContainer,addChildAt)
{
	DisplayObjectContainer* th=static_cast<DisplayObjectContainer*>(obj->implementation);
	assert(args->size()==2);
	//Validate object type
	assert(isSubclass(args->at(0),Class<DisplayObject>::getClass()));

	//Cast to object
	DisplayObject* d=static_cast<DisplayObject*>(args->at(0)->implementation);
	th->_addChildAt(d,0);

	return d->obj;
}

ASFUNCTIONBODY(DisplayObjectContainer,addChild)
{
	DisplayObjectContainer* th=static_cast<DisplayObjectContainer*>(obj->implementation);
	assert(args->size()==1);
	//Validate object type
	assert(isSubclass(args->at(0),Class<DisplayObject>::getClass()));

	//Cast to object
	DisplayObject* d=static_cast<DisplayObject*>(args->at(0)->implementation);
	th->_addChildAt(d,0);

	return d->obj;
}

void Shape::sinit(Class_base* c)
{
	assert(c->constructor==NULL);
	c->constructor=new Function(_constructor);
	c->super=Class<DisplayObject>::getClass();
}

ASFUNCTIONBODY(Shape,_constructor)
{
	DisplayObject::_constructor(obj,NULL);
	return NULL;
}

void Stage::sinit(Class_base* c)
{
	assert(c->constructor==NULL);
	c->constructor=new Function(_constructor);
	c->super=Class<DisplayObjectContainer>::getClass();
}

Stage::Stage()
{
}

ASFUNCTIONBODY(Stage,_constructor)
{
	obj->setGetterByQName("stageWidth","",new Function(_getStageWidth));
	obj->setGetterByQName("stageHeight","",new Function(_getStageHeight));
	return NULL;
}

ASFUNCTIONBODY(Stage,_getStageWidth)
{
	Stage* th=static_cast<Stage*>(obj->implementation);
	RECT size=sys->getFrameSize();
	int width=size.Xmax/20;
	return abstract_d(width);
}

ASFUNCTIONBODY(Stage,_getStageHeight)
{
	Stage* th=static_cast<Stage*>(obj->implementation);
	RECT size=sys->getFrameSize();
	int height=size.Ymax/20;
	return abstract_d(height);
}

void Graphics::sinit(Class_base* c)
{
	assert(c->constructor==NULL);
	c->constructor=new Function(_constructor);
}

ASFUNCTIONBODY(Graphics,_constructor)
{
	obj->setVariableByQName("clear","",new Function(clear));
	obj->setVariableByQName("drawRect","",new Function(drawRect));
}

ASFUNCTIONBODY(Graphics,clear)
{
	Graphics* th=static_cast<Graphics*>(obj->implementation);
	sem_wait(&th->geometry_mutex);
	th->geometry.clear();
	sem_post(&th->geometry_mutex);
}

ASFUNCTIONBODY(Graphics,drawRect)
{
	Graphics* th=static_cast<Graphics*>(obj->implementation);
	assert(args->size()==4);

	int x=args->at(0)->toInt();
	int y=args->at(1)->toInt();
	int width=args->at(2)->toInt();
	int height=args->at(3)->toInt();

	//Build a shape and add it to the geometry vector
	GeomShape tmpShape;
	tmpShape.outline.push_back(Vector2(x,y));
	tmpShape.outline.push_back(Vector2(x+width,y));
	tmpShape.outline.push_back(Vector2(x+width,y+height));
	tmpShape.outline.push_back(Vector2(x,y+height));
	tmpShape.outline.push_back(Vector2(x,y));

	sem_wait(&th->geometry_mutex);
	th->geometry.push_back(tmpShape);
	sem_post(&th->geometry_mutex);
	return NULL;
}

void Graphics::Render()
{
	sem_wait(&geometry_mutex);
	for(int i=0;i<geometry.size();i++)
		geometry[i].Render();

/*	if(geometry.size()==0)
	{
		FILLSTYLE::fixedColor(0,0,0);
		glBegin(GL_QUADS);
			glVertex2i(0,0);
			glVertex2i(100,0);
			glVertex2i(100,100);
			glVertex2i(0,100);
		glEnd();
	}*/
	sem_post(&geometry_mutex);

}
