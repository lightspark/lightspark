/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2011  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef TOPLEVEL_DATE_H
#define TOPLEVEL_DATE_H

#include "asobject.h"

namespace lightspark
{

class Date: public ASObject
{
CLASSBUILDABLE(Date);
private:
	int extrayears;
	int year;
	int month;
	int day;
	int day_of_week;
	int hour;
	int minute;
	int second;
	int millisecond;
	Date();
	GDateTime *datetime;
	ASObject *msSinceEpoch();
public:
	static void sinit(Class_base*);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
	ASFUNCTION(UTC);
	ASFUNCTION(getTimezoneOffset);
	ASFUNCTION(getTime);
	ASFUNCTION(getFullYear);
	ASFUNCTION(getMonth);
	ASFUNCTION(getDate);
	ASFUNCTION(getDay);
	ASFUNCTION(getHours);
	ASFUNCTION(getMinutes);
	ASFUNCTION(getSeconds);
	ASFUNCTION(getMilliseconds);
	ASFUNCTION(getUTCFullYear);
	ASFUNCTION(getUTCMonth);
	ASFUNCTION(getUTCDate);
	ASFUNCTION(getUTCDay);
	ASFUNCTION(getUTCHours);
	ASFUNCTION(getUTCMinutes);
	ASFUNCTION(getUTCSeconds);
	ASFUNCTION(getUTCMilliseconds);
	ASFUNCTION(valueOf);
	ASFUNCTION(setFullYear);
	ASFUNCTION(setMonth);
	ASFUNCTION(setDate);
	ASFUNCTION(setHours);
	ASFUNCTION(setMinutes);
	ASFUNCTION(setSeconds);
	ASFUNCTION(setMilliseconds);
	ASFUNCTION(setUTCFullYear);
	ASFUNCTION(setUTCMonth);
	ASFUNCTION(setUTCDate);
	ASFUNCTION(setUTCHours);
	ASFUNCTION(setUTCMinutes);
	ASFUNCTION(setUTCSeconds);
	ASFUNCTION(setUTCMilliseconds);
	ASFUNCTION(timezoneOffset);
	tiny_string toString(bool debugMsg=false);
	tiny_string toString_priv() const;
	//Serialization interface
	void serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
			std::map<const ASObject*, uint32_t>& objMap) const;
};
}
#endif
