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
#include "swf.h"

using namespace lightspark;

static Mutex logmutex;
LOG_LEVEL Log::log_level=LOG_INFO;
const char* Log::level_names[]={"ERROR", "INFO","NOT_IMPLEMENTED","CALLS","TRACE"};
int Log::calls_indent = 0;
asAtom logAtom = asAtomHandler::invalidAtom;
std::ostream* logStream = &std::cerr;
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
#ifndef NDEBUG
		*logStream << level_names[cur_level] << ": " << asAtomHandler::toDebugString(logAtom)<<" " << message.str();
#else
		*logStream << level_names[cur_level] << ": " << message.str();
#endif
	}
}

std::ostream& Log::operator()()
{
	return message;
}

void Logger::trace(const tiny_string& str)
{
	Locker l(logmutex);
	*outStream << str << std::endl;
}

void Logger::setStream(std::ostream* stream)
{
	Locker l(logmutex);
	outStream = stream;
}

void Log::redirect(std::string filename)
{
	Locker l(logmutex);
	static std::ofstream file(filename);
	logStream = &file;
}

void Log::setLogStream(std::ostream* stream)
{
	Locker l(logmutex);
	logStream = stream;
}
