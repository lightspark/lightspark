/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2010  Ennio Barbaro (e.barbaro@sssup.it)
    Copyright (C) 2010-2011  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef _AMF3_GENERATOR_H

#include <string>
namespace lightspark
{

class ASObject;

class Amf3Deserializer
{
private:
	char* start;
	char* const end;
public:
	Amf3Deserializer(char* s, char* e):start(s),end(e)
	{
	}
	bool generateObjects(std::vector<ASObject*>& objs);
};

};
#endif
