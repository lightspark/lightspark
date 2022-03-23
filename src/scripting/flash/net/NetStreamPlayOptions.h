/**************************************************************************
    Lightspark, a free flash player implementation

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

#ifndef NETSTREAMPLAYOPTIONS_H
#define NETSTREAMPLAYOPTIONS_H

#include "asobject.h"
#include "scripting/flash/events/flashevents.h"

namespace lightspark
{

class NetStreamPlayOptions: public EventDispatcher
{
private:
	ASPROPERTY_GETTER_SETTER(number_t,len);
	ASPROPERTY_GETTER_SETTER(number_t,offset);
	ASPROPERTY_GETTER_SETTER(tiny_string,oldStreamName);
	ASPROPERTY_GETTER_SETTER(number_t,start);
	ASPROPERTY_GETTER_SETTER(tiny_string,streamName);
	ASPROPERTY_GETTER_SETTER(tiny_string,transition);
public:
	NetStreamPlayOptions(ASWorker* wrk,Class_base* c);
	static void sinit(Class_base*);
	ASFUNCTION_ATOM(_constructor);
};

}
#endif // NETSTREAMPLAYOPTIONS_H
