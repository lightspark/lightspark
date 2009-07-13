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
#include <math.h>

#include "abc.h"
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

ASFUNCTIONBODY(ASArray,_constructor)
{
	if(args==NULL)
		return NULL;
	LOG(NOT_IMPLEMENTED,"Called Array constructor");
	ASArray* th=static_cast<ASArray*>(obj);
	th->length=0;
	th->setVariableByName("length",&th->length);
	th->length.incRef();
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


ISWFObject* ASArray::getVariableByMultiname(const multiname& name, bool& found)
{
	ISWFObject* ret;
	bool number=true;
	found=false;
	for(int i=0;i<name.name.size();i++)
	{
		if(!isdigit(name.name[i]))
		{
			number=false;
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
		ret=ASObject::getVariableByMultiname(name,found);

	return ret;
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

ISWFObject* ASObject::getVariableByMultiname(const multiname& name, bool& found)
{
	ISWFObject* ret=ISWFObject::getVariableByMultiname(name,found);
	if(!found && super)
		ret=super->getVariableByMultiname(name,found);

	if(!found && prototype)
		ret=prototype->getVariableByMultiname(name,found);

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

ASObject::ASObject():
	debug_id(0),prototype(NULL),super(NULL)
{
	setVariableByName("toString",new MethodDefinable("toString",ASObject::_toString));
}

ASFUNCTIONBODY(ASObject,_toString)
{
	cout << "ss" << endl;
	return new ASString(obj->toString());
}

ASFUNCTIONBODY(ASObject,_constructor)
{
}

ASString::ASString()
{
	setVariableByName("Call",new Function(ASString::String));
	setVariableByName("toString",new Function(ASObject::_toString));
}

ASString::ASString(const string& s):data(s)
{
	setVariableByName("Call",new Function(ASString::String));
	setVariableByName("toString",new Function(ASObject::_toString));
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

string ASObject::toString() const
{
	return "Object";
}

string ASString::toString() const
{
	return data.data();
}

double ASString::toNumber() const
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

bool Number::isEqual(const ISWFObject* o) const
{
	if(o->getObjectType()==T_INTEGER)
		return val==o->toNumber();
	else if(o->getObjectType()==T_NUMBER)
		return val==o->toNumber();
	else
	{
		return ISWFObject::isLess(o);
	}
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

double Number::toNumber() const
{
	return val;
}

int Number::toInt() const
{
	return val;
}

Date::Date():year(-1),month(-1),date(-1),hour(-1),minute(-1),second(-1),millisecond(-1)
{
	constructor=new Function(_constructor);
}

ASFUNCTIONBODY(Date,_constructor)
{
	Date* th=static_cast<Date*>(obj);
	th->setVariableByName("getTimezoneOffset",new Function(getTimezoneOffset));
	th->setVariableByName("valueOf",new Function(valueOf));
	th->setVariableByName(Qname(AS3,"getTime"),new Function(getTime));
	th->year=1990;
	th->month=1;
	th->date=1;
	th->hour=0;
	th->minute=0;
	th->second=0;
	th->millisecond=0;
}

ASFUNCTIONBODY(Date,getTimezoneOffset)
{
	LOG(NOT_IMPLEMENTED,"getTimezoneOffset");
	return new Number(120);
}

ASFUNCTIONBODY(Date,getTime)
{
	Date* th=dynamic_cast<Date*>(obj);
	long ret=0;
	//TODO: leap year
	ret+=(th->year-1990)*365*24*3600*1000;
	//TODO: month length
	ret+=(th->month-1)*30*24*3600*1000;
	ret+=(th->date-1)*24*3600*1000;
	ret+=th->hour*3600*1000;
	ret+=th->minute*60*1000;
	ret+=th->second*1000;
	ret+=th->millisecond;
	return new Number(ret);
}

ASFUNCTIONBODY(Date,valueOf)
{
	Date* th=dynamic_cast<Date*>(obj);
	return th->getTime(obj,args);
}

IFunction* SyntheticFunction::toFunction()
{
	return this;
}

IFunction* Function::toFunction()
{
	return this;
}

SyntheticFunction::SyntheticFunction(method_info* m):mi(m)
{
	m->synt_method();
	if(m->f)
	{
		val=(synt_function)m->vm->ex->getPointerToFunction(m->f);
	}
	else
	{
		LOG(ERROR,"Cannot synt the method");
		abort();
	}
}

ISWFObject* SyntheticFunction::call(ISWFObject* obj, arguments* args)
{
	call_context* cc=new call_context(mi);
	cc->scope_stack=func_scope;
	ISWFObject* ret;
	if(!bound)
		ret=val(obj,args,cc);
	else
	{
		LOG(CALLS,"Calling with closure");
		LOG(CALLS,"args 0 " << args->at(0));
		ret=val(closure_this,args,cc);
	}
	delete cc;
	return ret;
}

ISWFObject* Function::call(ISWFObject* obj, arguments* args)
{
	if(!bound)
		return val(obj,args);
	else
	{
		LOG(CALLS,"Calling with closure");
		LOG(CALLS,"args 0 " << args->at(0));
		return val(closure_this,args);
	}
}

Math::Math()
{
	setVariableByName("PI",new Number(M_PI));
	setVariableByName("sqrt",new Function(sqrt));
	setVariableByName("atan2",new Function(atan2));
	setVariableByName("floor",new Function(floor));
	setVariableByName("random",new Function(random));
}

ASFUNCTIONBODY(Math,atan2)
{
	Number* n1=dynamic_cast<Number*>(args->at(0));
	Number* n2=dynamic_cast<Number*>(args->at(1));
	if(n1 && n2)
		return new Number(::atan2(*n1,*n2));
	else
	{
		LOG(CALLS, "Invalid argument type ("<<args->at(0)->getObjectType()<<','<<args->at(1)->getObjectType()<<')' );
		abort();
	}
}

ASFUNCTIONBODY(Math,floor)
{
	Number* n=dynamic_cast<Number*>(args->at(0));
	if(n)
		return new Number(::floor(*n));
	else
	{
		LOG(TRACE,"Invalid argument");
		abort();
	}
}

ASFUNCTIONBODY(Math,sqrt)
{
	Number* n=dynamic_cast<Number*>(args->at(0));
	if(n)
		return new Number(::sqrt(*n));
	else
	{
		LOG(TRACE,"Invalid argument");
		abort();
	}
}

ASFUNCTIONBODY(Math,random)
{
	double ret=rand();
	ret/=RAND_MAX;
	return new Integer(ret);
}
