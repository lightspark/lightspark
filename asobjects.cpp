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
	setVariableByName("length",&length);
	length.incRef();
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
	LOG(NOT_IMPLEMENTED,"Called ASXML::load " << args->at(0)->toString());
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
	return new Integer(1);
}


ISWFObject* ASArray::getVariableByName(const Qname& name, bool& found)
{
	ISWFObject* ret;
	bool number=true;
	for(int i=0;i<name.name.size();i++)
	{
		if(!isdigit(name.name[i]))
		{
			number=false;
			found=false;
			break;
		}

	}
	if(number)
	{
		int index=atoi(name.name.c_str());
		if(index<data.size())
		{
			ret=data[index];
			found=true;
		}
		else
			found=false;
	}

	if(!found)
		ret=ASObject::getVariableByName(name,found);

	return ret;
}

ISWFObject* ASObject::getVariableByName(const Qname& name, bool& found)
{
	ISWFObject* ret=ISWFObject::getVariableByName(name,found);
	if(!found && super)
		ret=super->getVariableByName(name,found);

	if(!found && prototype)
		ret=prototype->getVariableByName(name,found);

	return ret;
}

void ASObject::_register()
{
//	setVariableByName("constructor",new Function(constructor));
}

/*ASFUNCTIONBODY(ASObject,constructor)
{
	LOG(NOT_IMPLEMENTED,"Called Object constructor");
	return NULL;
}*/

ASString::ASString()
{
	setVariableByName("Call",new Function(ASString::String));
}

ASString::ASString(const string& s):data(s)
{
	setVariableByName("Call",new Function(ASString::String));
}

ASArray::~ASArray()
{
	for(int i=0;i<data.size();i++)
		data[i]->decRef();
}

ASFUNCTIONBODY(ASString,String)
{
	ASString* th=dynamic_cast<ASString*>(obj);
	if(args->at(0)->getObjectType()==T_NUMBER)
	{
		Number* n=dynamic_cast<Number*>(args->at(0));
		ostringstream oss;
		oss << setprecision(8) << fixed << *n;

		th->data=oss.str();
		return th;
	}
	else
	{
		LOG(CALLS,"Cannot convert " << args->at(0)->getObjectType() << " to String");
		abort();
	}
}

string ASString::toString() const
{
	return data.data();
}

double ASString::toNumber()
{
	LOG(ERROR,"Cannot convert string " << data << " to float");
	return 0;
}

ASFUNCTIONBODY(Undefined,call)
{
	LOG(CALLS,"Undefined function");
}

string Undefined::toString() const
{
	return string("undefined");
}

bool ASString::isEqual(const ISWFObject* r) const
{
	if(r->getObjectType()==T_STRING)
	{
		const ASString* s=dynamic_cast<const ASString*>(r);
		if(s->data==data)
			return true;
		else
			return false;
	}
	else
		return false;
}

bool Undefined::isEqual(const ISWFObject* r) const
{
	if(r->getObjectType()==T_UNDEFINED)
		return true;
	else
		return false;
}

Undefined::Undefined()
{
	setVariableByName(".Call",new Function(call));
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
	if(o->getObjectType()!=T_NUMBER)
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

double Number::toNumber()
{
	return val;
}

int Number::toInt()
{
	return val;
}

