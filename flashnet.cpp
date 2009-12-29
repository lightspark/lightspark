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

#include "flashnet.h"
#include "class.h"

using namespace std;
using namespace lightspark;

REGISTER_CLASS_NAME(URLLoader);
REGISTER_CLASS_NAME(URLLoaderDataFormat);
REGISTER_CLASS_NAME(URLRequest);

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
	return new ASString(th->url);
}

URLLoader::URLLoader():dataFormat("text")
{
}

void URLLoader::sinit(Class_base* c)
{
	assert(c->constructor==NULL);
	c->constructor=new Function(_constructor);
}

ASFUNCTIONBODY(URLLoader,_constructor)
{
	EventDispatcher::_constructor(obj,NULL);
	obj->setGetterByQName("dataFormat","",new Function(_getDataFormat));
	obj->setSetterByQName("dataFormat","",new Function(_setDataFormat));
	obj->setVariableByQName("load","",new Function(load));
	return NULL;
}

ASFUNCTIONBODY(URLLoader,load)
{
	URLLoader* th=static_cast<URLLoader*>(obj->implementation);
	ASObject* arg=args->at(0);
	assert(arg && arg->prototype->class_name=="URLRequest");
	URLRequest* req=static_cast<URLRequest*>(arg->implementation);
	tiny_string url=req->url;
	cout << "url is " << url << endl;
	ASObject* owner;
	ASObject* data=arg->getVariableByQName("data","",owner).obj;
	assert(owner);
	for(int i=0;i<data->numVariables();i++)
	{
		ASObject* cur=data->getValueAt(i);
		if(cur->getObjectType()!=T_FUNCTION)
			cout << "\t" << data->getNameAt(i) << ": " << cur->toString() << endl;
	}
	return NULL;
}

ASFUNCTIONBODY(URLLoader,_getDataFormat)
{
	URLLoader* th=static_cast<URLLoader*>(obj->implementation);
	return new ASString(th->dataFormat);
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
	c->setVariableByQName("VARIABLES","",new ASString("variables"));
	c->setVariableByQName("TEXT","",new ASString("text"));
	c->setVariableByQName("BINARY","",new ASString("binary"));
}

