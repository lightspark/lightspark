/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2011-2013  Alessandro Pignotti (a.pignotti@sssup.it)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#ifndef SCRIPTING_AVM1_AVM1MEDIA_H
#define SCRIPTING_AVM1_AVM1MEDIA_H


#include "asobject.h"
#include "scripting/flash/media/flashmedia.h"

namespace lightspark
{

class AVM1Video: public Video
{
public:
	AVM1Video(ASWorker* wrk,Class_base* c):Video(wrk,c){}
	AVM1Video(ASWorker* wk,Class_base* c, uint32_t w, uint32_t h, DefineVideoStreamTag* v):Video(wk,c,w,h,v) {}
	static void sinit(Class_base* c);
};

}
#endif // SCRIPTING_AVM1_AVM1MEDIA_H
