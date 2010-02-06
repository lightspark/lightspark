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

#ifndef _FLASH_NET_H
#define _FLASH_NET_H

#include "asobjects.h"
#include "flashevents.h"
#include "thread_pool.h"

namespace lightspark
{

class URLRequest: public IInterface
{
friend class Loader;
friend class URLLoader;
private:
	tiny_string url; 
public:
	URLRequest();
	static void sinit(Class_base*);
	ASFUNCTION(_constructor);
	ASFUNCTION(_getUrl);
	ASFUNCTION(_setUrl);
};

class URLLoaderDataFormat: public IInterface
{
public:
	static void sinit(Class_base*);
};

class SharedObject: public IInterface
{
public:
	static void sinit(Class_base*);
};

class ObjectEncoding: public IInterface
{
public:
	static void sinit(Class_base*);
};

class URLLoader: public EventDispatcher, public IThreadJob
{
private:
	tiny_string dataFormat;
	URLRequest* urlRequest;
	ASObject* data;
public:
	URLLoader();
	static void sinit(Class_base*);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
	ASFUNCTION(load);
	ASFUNCTION(_getDataFormat);
	ASFUNCTION(_getData);
	ASFUNCTION(_setDataFormat);
	void execute();
};

};

#endif
