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
#include "flashnet.h"
#include "class.h"
#include <curl/curl.h>

using namespace std;
using namespace lightspark;

REGISTER_CLASS_NAME(URLLoader);
REGISTER_CLASS_NAME(URLLoaderDataFormat);
REGISTER_CLASS_NAME(URLRequest);
REGISTER_CLASS_NAME(SharedObject);
REGISTER_CLASS_NAME(ObjectEncoding);
REGISTER_CLASS_NAME(NetConnection);

URLRequest::URLRequest()
{
}

void URLRequest::sinit(Class_base* c)
{
	assert(c->constructor==NULL);
	c->constructor=new Function(_constructor);
}

ASFUNCTIONBODY(URLRequest,_constructor)
{
	URLRequest* th=static_cast<URLRequest*>(obj->implementation);
/*	if(args->at(0)->getObjectType()!=T_STRING)
	{
		abort();
	}
	th->url=static_cast<ASString*>(args->at(0));*/
	if(args->size()>0 && args->at(0)->getObjectType()==T_STRING)
	{
		th->url=args->at(0)->toString();
		cout << "url is " << th->url << endl;
	}
	obj->setSetterByQName("url","",new Function(_setUrl));
	obj->setGetterByQName("url","",new Function(_getUrl));
	return NULL;
}

ASFUNCTIONBODY(URLRequest,_setUrl)
{
	URLRequest* th=static_cast<URLRequest*>(obj->implementation);
	th->url=args->at(0)->toString();
	cout << "Setting url to " << th->url << endl;
	return NULL;
}

ASFUNCTIONBODY(URLRequest,_getUrl)
{
	URLRequest* th=static_cast<URLRequest*>(obj->implementation);
	return Class<ASString>::getInstanceS(true,th->url)->obj;
}

URLLoader::URLLoader():dataFormat("text"),data(NULL)
{
}

void URLLoader::sinit(Class_base* c)
{
	assert(c->constructor==NULL);
	c->constructor=new Function(_constructor);
	c->super=Class<EventDispatcher>::getClass();
	c->max_level=c->super->max_level+1;
}

void URLLoader::buildTraits(ASObject* o)
{
	o->setGetterByQName("dataFormat","",new Function(_getDataFormat));
	o->setGetterByQName("data","",new Function(_getData));
	o->setSetterByQName("dataFormat","",new Function(_setDataFormat));
	o->setVariableByQName("load","",new Function(load));
}

ASFUNCTIONBODY(URLLoader,_constructor)
{
	EventDispatcher::_constructor(obj,NULL);
	return NULL;
}

ASFUNCTIONBODY(URLLoader,load)
{
	URLLoader* th=static_cast<URLLoader*>(obj->implementation);
	ASObject* arg=args->at(0);
	assert(arg->prototype==Class<URLRequest>::getClass());
	th->urlRequest=static_cast<URLRequest*>(arg->implementation);
	ASObject* data=arg->getVariableByQName("data","").obj;
	//POST data is not yet supported
	if(data)
		abort();
	assert(th->dataFormat=="binary");
	sys->cur_thread_pool->addJob(th);
	return NULL;
}

void URLLoader::execute()
{
	CurlDownloader curlDownloader;

	bool done=curlDownloader.download(urlRequest->url);
	if(done)
	{
		ByteArray* byteArray=Class<ByteArray>::getInstanceS(true);
		byteArray->acquireBuffer(curlDownloader.getBuffer(),curlDownloader.getLen());
		data=byteArray->obj;
		//Send a complete event for this object
		sys->currentVm->addEvent(this,Class<Event>::getInstanceS(true,"complete",obj));
	}
	else
	{
		//Notify an error during loading
		sys->currentVm->addEvent(this,Class<Event>::getInstanceS(true,"ioError",obj));
	}
}

ASFUNCTIONBODY(URLLoader,_getDataFormat)
{
	URLLoader* th=static_cast<URLLoader*>(obj->implementation);
	return Class<ASString>::getInstanceS(true,th->dataFormat)->obj;
}

ASFUNCTIONBODY(URLLoader,_getData)
{
	URLLoader* th=static_cast<URLLoader*>(obj->implementation);
	if(th->data==NULL)
		return new Undefined;
	
	th->data->incRef();
	return th->data;
}

ASFUNCTIONBODY(URLLoader,_setDataFormat)
{
	URLLoader* th=static_cast<URLLoader*>(obj->implementation);
	assert(args->at(0));
	th->dataFormat=args->at(0)->toString();
	return NULL;
}

void URLLoaderDataFormat::sinit(Class_base* c)
{
	c->setVariableByQName("VARIABLES","",Class<ASString>::getInstanceS(true,"variables")->obj);
	c->setVariableByQName("TEXT","",Class<ASString>::getInstanceS(true,"text")->obj);
	c->setVariableByQName("BINARY","",Class<ASString>::getInstanceS(true,"binary")->obj);
}

void SharedObject::sinit(Class_base* c)
{
};

void ObjectEncoding::sinit(Class_base* c)
{
	c->setVariableByQName("AMF0","",new Integer(0));
	c->setVariableByQName("AMF3","",new Integer(3));
	c->setVariableByQName("DEFAULT","",new Integer(3));
};

NetConnection::NetConnection()
{
}

void NetConnection::sinit(Class_base* c)
{
	//assert(c->constructor==NULL);
	//c->constructor=new Function(_constructor);
	c->super=Class<EventDispatcher>::getClass();
	c->max_level=c->super->max_level+1;
}

void NetConnection::buildTraits(ASObject* o)
{
}

void NetConnection::execute()
{
	abort();
}

bool CurlDownloader::download(const tiny_string& s)
{
	CURL *curl;
	CURLcode res;
	curl = curl_easy_init();
	bool ret=false;
	if(curl)
	{
		curl_easy_setopt(curl, CURLOPT_URL, s.raw_buf());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);
		curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, write_header);
		curl_easy_setopt(curl, CURLOPT_HEADERDATA, this);
		res = curl_easy_perform(curl);
		if(res==0)
			ret=true;
		curl_easy_cleanup(curl);
	}

	return ret;
}

size_t CurlDownloader::write_data(void *buffer, size_t size, size_t nmemb, void *userp)
{
	CurlDownloader* th=static_cast<CurlDownloader*>(userp);
	memcpy(th->buffer + th->offset,buffer,size*nmemb);
	th->offset+=(size*nmemb);
	return size*nmemb;
}

size_t CurlDownloader::write_header(void *buffer, size_t size, size_t nmemb, void *userp)
{
	CurlDownloader* th=static_cast<CurlDownloader*>(userp);
	char* headerLine=(char*)buffer;
	if(strncmp(headerLine,"Content-Length: ",16)==0)
	{
		//Now read the length and allocate the byteArray
		assert(th->buffer==NULL);
		th->len=atoi(headerLine+16);
		th->buffer=new uint8_t[th->len];
	}
	return size*nmemb;
}

