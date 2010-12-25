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

/**
 * ExtVariantObjects are the basic datatype for interfacing with an external container.
 * They can represent: void, null, string, int32, double & boolean datatypes.
 * They are used throughout the ExtScriptObject to pass & return data.
 *
 * This class can be extended to fit a specific interface (e.g.: NPVariantObject & NPRuntime).
 * These subclasses should provide a means to convert an ExtVariantObject.
 * This way, subclasses of ExtScriptObject can transparently work with different subclasses of ExtVariantObject.
 */
class DLL_PUBLIC ExtVariantObject
{
public:
	ExtVariantObject() :
		type(EVO_VOID), strValue(""), intValue(0), doubleValue(0), booleanValue(false) {}
	ExtVariantObject(const std::string& value) :
		type(EVO_STRING), strValue(value), intValue(0), doubleValue(0), booleanValue(false) {}
	ExtVariantObject(const char* value) :
		type(EVO_STRING), strValue(value), intValue(0), doubleValue(0), booleanValue(false) {}
	ExtVariantObject(int32_t value) :
		type(EVO_INT32), strValue(""), intValue(value), doubleValue(0), booleanValue(false) {}
	ExtVariantObject(double value) :
		type(EVO_DOUBLE), strValue(""), intValue(0), doubleValue(value), booleanValue(false) {}
	ExtVariantObject(bool value) :
		type(EVO_BOOLEAN), strValue(""), intValue(0), doubleValue(0), booleanValue(value) {}
	ExtVariantObject(const ExtVariantObject& other)
	{
		type = other.getType();
		strValue = other.getString();
		intValue = other.getInt();
		doubleValue = other.getDouble();
		booleanValue = other.getBoolean();
	}

	virtual ~ExtVariantObject() {}

	enum EVO_TYPE
	{ EVO_STRING, EVO_INT32, EVO_DOUBLE, EVO_BOOLEAN, EVO_OBJECT, EVO_NULL, EVO_VOID };
	virtual EVO_TYPE getType() const { return type; }

	// These methods return the value of the ExtVariantObject.
	// Returned values for non-matching types are undefined.
	// As such, don't get a string value for an integer ExtVariantObject
	virtual std::string getString() const { return strValue; }
	virtual int32_t getInt() const { return intValue; }
	virtual double getDouble() const { return doubleValue; }
	virtual bool getBoolean() const { return booleanValue; }
private:
	EVO_TYPE type;
	std::string strValue;
	int32_t intValue;
	double doubleValue;
	bool booleanValue;
};

/**
 * ExtIdentifierObjects are the basic identifiers for interfacing with an external container.
 * They can represent: int & string datatypes.
 * They are used to name properties of objects throughout ExtScriptObject.
 *
 * This class can be extended to fit a specific interface (e.g.: NPIdentifierObject & NPRuntime).
 * These subclasses should provide a means to convert an ExtIdentifierObject.
 * This way, subclasses of ExtIdentifierObjects can transparently work with different subclasses of ExtIdentifierObject.
 */
class DLL_PUBLIC ExtIdentifierObject
{
public:
	ExtIdentifierObject() : type(EI_STRING), strValue(""), intValue(0) {}
	ExtIdentifierObject(const std::string& value) :
		type(EI_STRING), strValue(value), intValue(0) {}
	ExtIdentifierObject(const char* value) :
		type(EI_STRING), strValue(value), intValue(0) {}
	ExtIdentifierObject(int32_t value) :
		type(EI_INT32), strValue(""), intValue(value) {}
	ExtIdentifierObject(const ExtIdentifierObject& other)
	{
		type = other.getType();
		strValue = other.getString();
		intValue = other.getInt();
	}

	virtual ~ExtIdentifierObject() {}

	// Since these objects get used as keys in std::maps, they need to be comparable.
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

	enum EI_TYPE { EI_STRING, EI_INT32 };
	virtual EI_TYPE getType() const { return type; }

	// These methods return the value of the ExtIdentifierObject.
	// Returned values for non-matching types are undefined.
	// As such, don't get a string value for an integer ExtIdentifierObject.
	virtual std::string getString() const { return strValue; }
	virtual int32_t getInt() const { return intValue; }
private:
	EI_TYPE type;
	std::string strValue;
	int32_t intValue;
};

// The result variable should be "delete"d by the caller after use.
typedef bool (*ExtCallbackFunctionPtr)(const ExtIdentifierObject& id,
		const ExtVariantObject* args, uint32_t argc, ExtVariantObject** result);

/**
 * An ExtScriptObject represents the interface LS presents to the external container.
 * There should be a 1-to-1 relationship between LS instances & ExtScriptObjects.
 *
 * ExtScriptObjects can present properties & methods to the external container.
 * Both are identified by an ExtIdentifierObject (or an object of a derived class).
 * Properties have a value of type ExtVariantObject (or a derived type).
 */
class DLL_PUBLIC ExtScriptObject
{
public:
	virtual ~ExtScriptObject() {};

	virtual bool hasMethod(const ExtIdentifierObject& id) = 0;
	// There currently is no way to invoke the set methods. There's no need for it anyway.
	virtual void setMethod(const ExtIdentifierObject& id, const ExtCallbackFunctionPtr& func) = 0;

	virtual bool hasProperty(const ExtIdentifierObject& id) = 0;
	// The returned value should be "delete"d by the caller after use
	virtual ExtVariantObject* getProperty(const ExtIdentifierObject& id) = 0;
	virtual void setProperty(const ExtIdentifierObject& id, const ExtVariantObject& value) = 0;
	virtual bool removeProperty(const ExtIdentifierObject& id) = 0;
};

};

#endif
