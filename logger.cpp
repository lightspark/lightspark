/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009,2010  Alessandro Pignotti (a.pignotti@sssup.it)

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

#include "logger.h"

sem_t Log::mutex;
bool Log::loggingInited = false;
LOG_LEVEL Log::log_level=LOG_NO_INFO;
const char* Log::level_names[]={"INFO","ERROR","NOT_IMPLEMENTED","CALLS","TRACE"};

Log::Log(LOG_LEVEL l)
{
	if(l<=log_level)
	{
		cur_level=l;
		valid=true;
		sem_wait(&mutex);
	}
	else
		valid=false;
}

Log::~Log()
{
	if(valid)
		sem_post(&mutex);
}

std::ostream& Log::operator()()
{
	std::cout << level_names[cur_level] << ": ";
	return std::cout;
}

void Log::initLogging(LOG_LEVEL l)
{
	if(!loggingInited)
	{
		loggingInited=true;
		sem_init(&mutex,0,1);
		log_level=l;
	}
}
