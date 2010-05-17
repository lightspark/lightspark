/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009,2010  Alessandro Pignotti (a.pignotti@sssup.it)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include <exception>
#include <string>

namespace lightspark
{

class LightsparkException: public std::exception
{
public:
	std::string cause;
	std::string url;
	LightsparkException(const std::string& c, const std::string u):cause(c),url(u)
	{
	}
	~LightsparkException() throw(){}
};

class RunTimeException: public LightsparkException
{
public:
	RunTimeException(const std::string& c, const std::string& u):LightsparkException(c,u)
	{
	}
	const char* what() const throw()
	{
		return "Lightspark error";
	}
};

class UnsupportedException: public LightsparkException
{
public:
	UnsupportedException(const std::string& c, const std::string& u):LightsparkException(c,u)
	{
	}
	const char* what() const throw()
	{
		return "Lightspark unsupported operation";
	}
};

class ParseException: public LightsparkException
{
public:
	ParseException(const std::string& c, const std::string& u):LightsparkException(c,u)
	{
	}
	const char* what() const throw()
	{
		return "Lightspark invalid file";
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

};
#endif
