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

#ifndef SCRIPTING_AVM1_AVM1TEXT_H
#define SCRIPTING_AVM1_AVM1TEXT_H


#include "asobject.h"
#include "scripting/toplevel/ASString.h"
#include "scripting/flash/text/flashtext.h"

namespace lightspark
{

class AVM1TextField: public TextField
{
public:
	AVM1TextField(ASWorker* wrk,Class_base* c, const TextData& textData=TextData(), bool _selectable=true, bool readOnly=true, const char* varname="", DefineEditTextTag* _tag=nullptr)
		:TextField(wrk,c,textData,_selectable,readOnly,varname,_tag)
	{}
	static void sinit(Class_base* c);
};
class AVM1TextFormat: public TextFormat
{
public:
	AVM1TextFormat(ASWorker* wrk,Class_base* c):TextFormat(wrk,c)
	{}
	static void sinit(Class_base* c);
};

}
#endif // SCRIPTING_AVM1_AVM1TEXT_H
