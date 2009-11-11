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

using namespace std;

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
	URLRequest* th=static_cast<URLRequest*>(obj->interface);
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
}

ASFUNCTIONBODY(URLRequest,_setUrl)
{
	URLRequest* th=static_cast<URLRequest*>(obj->interface);
	th->url=args->at(0)->toString();
	cout << "Setting url to " << th->url << endl;
}

ASFUNCTIONBODY(URLRequest,_getUrl)
{
	URLRequest* th=static_cast<URLRequest*>(obj->interface);
	return new ASString(th->url);
}
