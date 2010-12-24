/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2010  Alessandro Pignotti (a.pignotti@sssup.it)
    Copyright (C) 2010  Timon Van Overveldt (timonvo@gmail.com)

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

#ifndef _EXT_SCRIPT_OBJECT_H
#define _EXT_SCRIPT_OBJECT_H

#include <string>

#include "compat.h"

namespace lightspark
{

class DLL_PUBLIC ExtVariantObject
{
public:
	ExtVariantObject() {}
	ExtVariantObject(const std::string& value) {}
	ExtVariantObject(const char* value) {}
	ExtVariantObject(int32_t value) {}
	ExtVariantObject(double value) {}
	ExtVariantObject(bool value) {}
	ExtVariantObject(const ExtVariantObject& value) {}
	virtual ~ExtVariantObject() {}

	enum EVO_TYPE
	{ EVO_STRING, EVO_INT32, EVO_DOUBLE, EVO_BOOLEAN, EVO_OBJECT, EVO_NULL, EVO_VOID };
	virtual EVO_TYPE getType() const { return EVO_VOID; }
	virtual std::string getString() const { return ""; }
	virtual int32_t getInt() const { return 0; }
	virtual double getDouble() const { return 0.0; }
	virtual bool getBoolean() const { return 0; }
};

class DLL_PUBLIC ExtIdentifierObject
{
public:
	ExtIdentifierObject() {}
	ExtIdentifierObject(const std::string& value) {}
	ExtIdentifierObject(const char* value) {}
	ExtIdentifierObject(int32_t value) {}
	ExtIdentifierObject(const ExtIdentifierObject& value) {}
	virtual ~ExtIdentifierObject() {}

	virtual bool operator<(const ExtIdentifierObject& other) const
	{
		if(getType() == EI_STRING && other.getType() == EI_STRING)
			return getString() < other.getString();
		else if(getType() == EI_INT32 && other.getType() == EI_INT32)
			return getInt() < other.getInt();
		else if(getType() == EI_INT32 && other.getType() == EI_STRING)
			return true;
		return false;
	}

	enum EI_TYPE { EI_STRING, EI_INT32, EI_UNKNOWN };
	virtual EI_TYPE getType() const { return EI_UNKNOWN; }
	virtual std::string getString() const { return ""; }
	virtual int32_t getInt() const { return 0; }
};

class DLL_PUBLIC ExtScriptObject
{
public:
	virtual ~ExtScriptObject() {};

	/* These methods provide an interface to what properties & methods are available to the external container */
	virtual bool hasMethod(const std::string& method) = 0;
	virtual bool hasProperty(const std::string& property) = 0;
	virtual void setProperty(const std::string& property, const std::string& value) = 0;
	virtual void setProperty(const std::string& property, int value) = 0;
	virtual void setProperty(const std::string& property, double value) = 0;
	virtual ExtVariantObject* getProperty(const std::string& property) = 0;
};

};

#endif
