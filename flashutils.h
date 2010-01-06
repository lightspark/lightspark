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

#ifndef _FLASH_UTILS_H
#define _FLASH_UTILS_H

#include "swftypes.h"
#include "flashevents.h"
#include "thread_pool.h"

namespace lightspark
{

class ByteArray: public IInterface
{
friend class Loader;
protected:
	uint8_t* bytes;
	int len;
public:
	ByteArray();
	static void sinit(Class_base* c);
};

class Timer: public EventDispatcher, public IThreadJob
{
private:
	void execute();
protected:
	uint32_t delay;
public:
	Timer():delay(0){};
	static void sinit(Class_base* c);
	ASFUNCTION(_constructor);
	ASFUNCTION(start);
};

ASObject* getQualifiedClassName(ASObject* , arguments* args);

};

#endif
