/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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

#include <fstream>
#include "logger.h"
#include "threading.h"

using namespace lightspark;

static Mutex logmutex;
LOG_LEVEL Log::log_level=LOG_INFO;
const char* Log::level_names[]={"ERROR", "INFO","NOT_IMPLEMENTED","CALLS","TRACE"};
int Log::calls_indent = 0;

Log::Log(LOG_LEVEL l)
{
	if(l<=log_level)
	{
		cur_level=l;
		valid=true;
		if(l >= LOG_CALLS)
			message << std::string(2*calls_indent,' ');
	}
	else
		valid=false;
}

Log::~Log()
{
	if(valid)
	{
		Locker l(logmutex);
		std::cerr << level_names[cur_level] << ": " << message.str();
	}
}

std::ostream& Log::operator()()
{
	return message;
}

void Log::print(const std::string& s)
{
	Locker l(logmutex);
	std::cout << s << std::endl;
}

void Log::redirect(std::string filename)
{
	Locker l(logmutex);
	static std::ofstream file(filename);
	std::cout.rdbuf(file.rdbuf());
	std::cerr.rdbuf(file.rdbuf());
}
