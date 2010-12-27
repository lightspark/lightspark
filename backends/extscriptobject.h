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
#include <sstream>
#include <map>

#include "asobject.h"
#include "compat.h"

namespace lightspark
{

/**
 * ExtIdentifiers are the basic identifiers for interfacing with an external container.
 * They can represent: int & string datatypes.
 * They are used to name properties of objects throughout ExtScriptObject.
 *
 * This class can be extended to fit a specific interface (e.g.: NPIdentifierObject & NPRuntime).
 * These subclasses should provide a means to convert an ExtIdentifier.
 * This way, subclasses of ExtIdentifiers can transparently work with different subclasses of ExtIdentifier.
 */
class DLL_PUBLIC ExtIdentifier
{
public:
	ExtIdentifier() : type(EI_STRING), strValue(""), intValue(0) {}
	ExtIdentifier(const std::string& value) :
		type(EI_STRING), strValue(value), intValue(0)
	{
		intValue = atoi(strValue.c_str());
		std::stringstream out;
		out << intValue;
		// Convert integer string identifiers to integer identifiers
		if(out.str() == strValue)
			type = EI_INT32;
	}
	ExtIdentifier(const char* value) :
		type(EI_STRING), strValue(value), intValue(0)
	{
		intValue = atoi(value);
		std::stringstream out;
		out << intValue;
		// Convert integer string identifiers to integer identifiers
		if(out.str() == std::string(strValue))
			type = EI_INT32;
	}
	ExtIdentifier(int32_t value) :
		type(EI_INT32), strValue(""), intValue(value) {}
	ExtIdentifier(const ExtIdentifier& other)
	{
		type = other.getType();
		strValue = other.getString();
		intValue = other.getInt();
	}

	virtual ~ExtIdentifier() {}

	// Since these objects get used as keys in std::maps, they need to be comparable.
	virtual bool operator<(const ExtIdentifier& other) const
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

	// These methods return the value of the ExtIdentifier.
	// Returned values for non-matching types are undefined.
	// As such, don't get a string value for an integer ExtIdentifier.
	virtual std::string getString() const { return strValue; }
	virtual int32_t getInt() const { return intValue; }
private:
	EI_TYPE type;
	std::string strValue;
	int32_t intValue;
};

class ExtVariant;
/**
 * This class represents an object containing key-value pairs 
 * of ExtIdentifiers & ExtVariants.
 */
class DLL_PUBLIC ExtObject
{
public:
	ExtObject() : type(EO_OBJECT) {}
	ExtObject(const ExtObject& other) { type = other.getType(); other.copy(properties); }
	virtual ~ExtObject() {}
	virtual ExtObject& operator=(const ExtObject& other)
	{
		type = other.getType();
		other.copy(properties);
		return *this;
	}

	void copy(std::map<ExtIdentifier, ExtVariant>& dest) const;

	virtual bool hasProperty(const ExtIdentifier& id) const;
	// The returned value should be "delete"d by the caller after use
	virtual ExtVariant* getProperty(const ExtIdentifier& id) const;
	virtual void setProperty(const ExtIdentifier& id, const ExtVariant& value);
	virtual bool removeProperty(const ExtIdentifier& id);

	virtual bool enumerate(ExtIdentifier*** ids, uint32_t* count) const;

	enum EO_TYPE { EO_OBJECT, EO_ARRAY };
	virtual EO_TYPE getType() const { return type; }
	virtual void setType(EO_TYPE _type) { type = _type; }
	virtual uint32_t getLength() const { return properties.size(); }
protected:
	EO_TYPE type;
private:
	std::map<ExtIdentifier, ExtVariant> properties;
};

/**
 * ExtVariants are the basic datatype for interfacing with an external container.
 * They can represent: void, null, string, int32, double & boolean datatypes.
 * They are used throughout the ExtScriptObject to pass & return data.
 *
 * This class can be extended to fit a specific interface (e.g.: NPVariantObject & NPRuntime).
 * These subclasses should provide a means to convert an ExtVariant.
 * This way, subclasses of ExtScriptObject can transparently 
 * work with different subclasses of ExtVariant.
 * This class is also able to convert an ASObject* to an ExtVariant and the other way around.
 */
class DLL_PUBLIC ExtVariant
{
public:
	ExtVariant() :
		type(EV_VOID), strValue(""), intValue(0), doubleValue(0), booleanValue(false) {}
	ExtVariant(const std::string& value) :
		type(EV_STRING), strValue(value), intValue(0), doubleValue(0), booleanValue(false) {}
	ExtVariant(const char* value) :
		type(EV_STRING), strValue(value), intValue(0), doubleValue(0), booleanValue(false) {}
	ExtVariant(int32_t value) :
		type(EV_INT32), strValue(""), intValue(value), doubleValue(0), booleanValue(false) {}
	ExtVariant(double value) :
		type(EV_DOUBLE), strValue(""), intValue(0), doubleValue(value), booleanValue(false) {}
	ExtVariant(bool value) :
		type(EV_BOOLEAN), strValue(""), intValue(0), doubleValue(0), booleanValue(value) {}
	ExtVariant(const ExtVariant& other);
	ExtVariant(ASObject* other);

	virtual ~ExtVariant() {}

	enum EV_TYPE
	{ EV_STRING, EV_INT32, EV_DOUBLE, EV_BOOLEAN, EV_OBJECT, EV_NULL, EV_VOID };
	virtual EV_TYPE getType() const { return type; }

	// These methods return the value of the ExtVariant.
	// Returned values for non-matching types are undefined.
	// As such, don't get a string value for an integer ExtVariant
	virtual std::string getString() const { return strValue; }
	virtual int32_t getInt() const { return intValue; }
	virtual double getDouble() const { return doubleValue; }
	virtual bool getBoolean() const { return booleanValue; }
	// Returned pointer should get "delete"d by caller after use
	virtual ExtObject* getObject() const { return new ExtObject(objectValue); }
	virtual ASObject* getASObject() const;
private:
	EV_TYPE type;
	std::string strValue;
	int32_t intValue;
	double doubleValue;
	bool booleanValue;
	ExtObject objectValue;
};

class ExtScriptObject;
// The signature for a hard-coded callback function.
// The result variable should be "delete"d by the caller after use.
typedef bool (*ExtCallbackFunctionPtr)(const ExtScriptObject& so, const ExtIdentifier& id,
		const ExtVariant** args, uint32_t argc, ExtVariant** result);

/**
 * This class provides a unified interface to hard-coded & runtime callback functions.
 * It stores either a IFunction* or a ExtCallbackFunctionPtr and provides a
 * operator() to call them.
 */
class DLL_PUBLIC ExtCallbackFunction
{
public:
	ExtCallbackFunction() : iFunc(NULL), extFunc(NULL) {}
	ExtCallbackFunction(IFunction* func) : iFunc(func), extFunc(NULL) {}
	ExtCallbackFunction(ExtCallbackFunctionPtr func) : iFunc(NULL), extFunc(func) {}

	// The result variable should be "delete"d by the caller after use.
	bool operator()(const ExtScriptObject& so, const ExtIdentifier& id,
		const ExtVariant** args, uint32_t argc, ExtVariant** result);
private:
	IFunction* iFunc;
	ExtCallbackFunctionPtr extFunc;
};

/**
 * An ExtScriptObject represents the interface LS presents to the external container.
 * There should be a 1-to-1 relationship between LS instances & ExtScriptObjects.
 *
 * ExtScriptObjects can present properties & methods to the external container.
 * Both are identified by an ExtIdentifier (or an object of a derived class).
 * Properties have a value of type ExtVariant (or a derived type).
 */
class DLL_PUBLIC ExtScriptObject
{
public:
	virtual ~ExtScriptObject() {};

	virtual bool hasMethod(const ExtIdentifier& id) const = 0;
	// There currently is no way to invoke the set methods. There's no need for it anyway.
	virtual void setMethod(const ExtIdentifier& id, const ExtCallbackFunction& func) = 0;
	virtual bool removeMethod(const ExtIdentifier& id) = 0;

	virtual bool hasProperty(const ExtIdentifier& id) const = 0;
	// The returned value should be "delete"d by the caller after use
	virtual ExtVariant* getProperty(const ExtIdentifier& id) const = 0;
	virtual void setProperty(const ExtIdentifier& id, const ExtVariant& value) = 0;
	virtual bool removeProperty(const ExtIdentifier& id) = 0;

	virtual bool enumerate(ExtIdentifier*** ids, uint32_t* count) const = 0;
};

};

#endif
