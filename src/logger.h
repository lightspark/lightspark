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

#ifndef LOGGER_H
#define LOGGER_H 1

#include "compat.h"
#include <iostream>
#include <sstream>
#include <vector>
#include <set>

enum LOG_LEVEL { LOG_ERROR=0, LOG_INFO=1, LOG_NOT_IMPLEMENTED=2,LOG_CALLS=3,LOG_TRACE=4};

// LOG_CALL will only generate output in DEBUG builds
// this is done because the LOG(LOG_CALLS...) macro creates measurable perfomance loss 
// when used inside the ABCVm::executeFunction loop even on lower log levels
#ifndef NDEBUG
#define LOG_CALL(esp)					\
do {							\
	if(LOG_CALLS<=Log::getLevel())			\
	{						\
		Log l(LOG_CALLS);			\
		l() << esp << std::endl;		\
	}						\
} while(0)
#else
#define LOG_CALL(esp)
#endif
#define LOG(level,esp)					\
do {							\
	if(level<=Log::getLevel())			\
	{						\
		Log l(level);				\
		l() << esp << std::endl;		\
	}						\
} while(0)

class Log
{
private:
	static const char* level_names[];
	static LOG_LEVEL log_level DLL_PUBLIC;
	std::stringstream message;
	LOG_LEVEL cur_level;
	bool valid;
public:
	static void print(const std::string& s);
	Log(LOG_LEVEL l) DLL_PUBLIC;
	~Log() DLL_PUBLIC;
	std::ostream& operator()() DLL_PUBLIC;
	operator bool() { return valid; }
	static void setLogLevel(LOG_LEVEL l) { log_level = l; }
	static LOG_LEVEL getLevel() {return log_level;}
	static int calls_indent;
	/* redirect logging and print() to that file */
	static void redirect(std::string filename) DLL_PUBLIC;

};

template<typename T>
std::ostream& printContainer(std::ostream& os, const T& v)
{
	os << "[";
	for (typename T::const_iterator i = v.begin(); i != v.end(); ++i)
	{
		if(i != v.begin())
			os << " ";
		os << *i;
	}
	os << "]";
	return os;
}

/* convenience function to log std containers */
namespace std {
template<typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& v)
{
	return printContainer(os,v);
}
template<typename T>
std::ostream& operator<<(std::ostream& os, const std::set<T>& v)
{
	return printContainer(os,v);
}
}

#endif /* LOGGER_H */
