/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2012  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H 1

#include "compat.h"
#include <exception>
#include <string>

#define STRINGIFY(n) #n
#define TOSTRING(x) STRINGIFY(x)

#define assert_and_throw(cond) if(!(cond)) \
{					\
	throw AssertionException(#cond " " __FILE__ ":" TOSTRING(__LINE__)); \
}


namespace lightspark
{

class LightsparkException: public std::exception
{
public:
	std::string cause;
	LightsparkException(const std::string& c):cause(c)
	{
	}
	~LightsparkException() throw(){}
};

class RunTimeException: public LightsparkException
{
public:
	RunTimeException(const std::string& c):LightsparkException(c)
	{
	}
	const char* what() const throw()
	{
		if(cause.length() == 0)
			return "Lightspark error";
		else
			return cause.c_str();
	}
};

class UnsupportedException: public LightsparkException
{
public:
	UnsupportedException(const std::string& c):LightsparkException(c)
	{
	}
	const char* what() const throw()
	{
		if(cause.length() == 0)
			return "Lightspark unsupported operation";
		else
			return cause.c_str();
	}
};

class ParseException: public LightsparkException
{
public:
	ParseException(const std::string& c):LightsparkException(c)
	{
	}
	const char* what() const throw()
	{
		if(cause.length() == 0)
			return "Lightspark invalid file";
		else
			return cause.c_str();
	}
};

class AssertionException: public LightsparkException
{
public:
	AssertionException(const std::string& c):LightsparkException(c)
	{
	}
	const char* what() const throw()
	{
		if(cause.length() == 0)
			return "Lightspark hit an unexpected condition";
		else
			return cause.c_str();
	}
};

class JobTerminationException: public std::exception
{
public:
	const char* what() const throw()
	{
		return "Job terminated";
	}
};

class ConfigException: public LightsparkException
{
public:
	ConfigException(const std::string& c):LightsparkException(c)
	{
	}
	const char* what() const throw()
	{
		if(cause.length() == 0)
			return "Lightspark invalid configuration file";
		else
			return cause.c_str();
	}
};

};
#endif /* EXCEPTIONS_H */
