/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2010-2012  Alessandro Pignotti (a.pignotti@sssup.it)
    Copyright (C) 2010-2012  Timon Van Overveldt (timonvo@gmail.com)

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

#ifndef BACKENDS_EXTSCRIPTOBJECT_H
#define BACKENDS_EXTSCRIPTOBJECT_H 1

#include <string>
#include <map>

#include "asobject.h"
#include "compat.h"
#include "scripting/flash/events/flashevents.h"

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
	ExtIdentifier();
	ExtIdentifier(const std::string& value);
	ExtIdentifier(const char* value);
	ExtIdentifier(int32_t value);
	ExtIdentifier(const ExtIdentifier& other);
	virtual ~ExtIdentifier() {}

	// Since these objects get used as keys in std::maps, they need to be comparable.
	virtual bool operator<(const ExtIdentifier& other) const;
	// Since this class is used as keys in property maps
	// it must implement a proper copy operator that must
	// deal with any subclass by acquiring the contents in
	// the internal data structures
	ExtIdentifier& operator=(const ExtIdentifier& other);

	enum EI_TYPE { EI_STRING, EI_INT32 };
	virtual EI_TYPE getType() const { return type; }

	// These methods return the value of the ExtIdentifier.
	// Returned values for non-matching types are undefined.
	virtual std::string getString() const { return strValue; }
	virtual int32_t getInt() const { return intValue; }
private:
	EI_TYPE type;
	std::string strValue;
	int32_t intValue;
	void stringToInt();
};

class ExtVariant;
/**
 * This class represents an object containing key-value pairs 
 * of ExtIdentifiers & ExtVariants.
 */
class DLL_PUBLIC ExtObject
{
public:
	ExtObject();
	ExtObject(const ExtObject& other);
	virtual ~ExtObject() {}

	ExtObject& operator=(const ExtObject& other);
	void copy(std::map<ExtIdentifier, ExtVariant>& dest) const;

	bool hasProperty(const ExtIdentifier& id) const;
	// The returned value should be "delete"d by the caller after use
	ExtVariant* getProperty(const ExtIdentifier& id) const;
	void setProperty(const ExtIdentifier& id, const ExtVariant& value);
	bool removeProperty(const ExtIdentifier& id);

	bool enumerate(ExtIdentifier*** ids, uint32_t* count) const;
	uint32_t getLength() const { return properties.size(); }

	enum EO_TYPE { EO_OBJECT, EO_ARRAY };
	EO_TYPE getType() const { return type; }
	void setType(EO_TYPE _type) { type = _type; }
protected:
	EO_TYPE type;
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
	ExtVariant();
	ExtVariant(const std::string& value);
	ExtVariant(const char* value);
	ExtVariant(int32_t value);
	ExtVariant(double value);
	ExtVariant(bool value);
	ExtVariant(const ExtVariant& other);
	ExtVariant(_R<ASObject> other);

	// Since this class is used as value in property maps
	// it must implement a proper copy operator that must
	// deal with any subclass by acquiring the contents in
	// the internal data structures
	ExtVariant& operator=(const ExtVariant& other);

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
	ASObject* getASObject() const;
private:
	EV_TYPE type;
	std::string strValue;
	int32_t intValue;
	double doubleValue;
	bool booleanValue;
	ExtObject objectValue;
};

class ExtScriptObject;

/**
 * This class provides an interface to use for external callback functions.
 */
class DLL_PUBLIC ExtCallback
{
public:
	ExtCallback() : success(false), exceptionThrown(false) {}
	virtual ~ExtCallback() {}

	// Don't forget to delete this copy after use
	virtual ExtCallback* copy()=0;

	virtual void call(const ExtScriptObject& so, const ExtIdentifier& id,
		const ExtVariant** args, uint32_t argc, bool synchronous)=0;
	virtual void wait()=0;
	virtual void wakeUp()=0;
	// The result variable should be "delete"d by the caller after use.
	virtual bool getResult(const ExtScriptObject& so, ExtVariant** result)=0;
protected:
	bool success;
	bool exceptionThrown;
	tiny_string exception;
};

class ExternalCallEvent;
/**
 * ExtCallback specialization for IFunctions
 */
class DLL_PUBLIC ExtASCallback : public ExtCallback
{
public:
	ExtASCallback(IFunction* _func) : func(_func), result(NULL), asArgs(NULL), funcWasCalled(false) { func->incRef(); }
	~ExtASCallback();

	// Don't forget to delete this copy after use
	ExtASCallback* copy() { return new ExtASCallback(func); }

	void call(const ExtScriptObject& so, const ExtIdentifier& id,
		const ExtVariant** args, uint32_t argc, bool synchronous);
	void wait();
	void wakeUp();
	// The result variable should be "delete"d by the caller after use.
	bool getResult(const ExtScriptObject& so, ExtVariant** _result);
private:
	IFunction* func;
	_NR<ExternalCallEvent> funcEvent;
	ASObject* result;
        ASObject** asArgs;
	bool funcWasCalled;
};

/**
 * ExtCallback specialization for builtin functions
 */
class DLL_PUBLIC ExtBuiltinCallback : public ExtCallback
{
public:
	// The signature for a hard-coded callback function.
	typedef bool (*funcPtr)(const ExtScriptObject& so, const ExtIdentifier& id,
		const ExtVariant** args, uint32_t argc, ExtVariant** result);

	ExtBuiltinCallback(funcPtr _func) : func(_func), result(NULL) {}
	~ExtBuiltinCallback() {}
	
	// Don't forget to delete this copy after use
	ExtBuiltinCallback* copy() { return new ExtBuiltinCallback(func); }

	void call(const ExtScriptObject& so, const ExtIdentifier& id,
		const ExtVariant** args, uint32_t argc, bool synchronous);
	void wait();
	void wakeUp();
	// The result variable should be "delete"d by the caller after use.
	bool getResult(const ExtScriptObject& so, ExtVariant** _result);
private:
	funcPtr func;
	ExtVariant* result;
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
	virtual void setMethod(const ExtIdentifier& id, ExtCallback* func) = 0;
	virtual bool removeMethod(const ExtIdentifier& id) = 0;

	virtual bool hasProperty(const ExtIdentifier& id) const = 0;
	// The returned value should be "delete"d by the caller after use
	virtual ExtVariant* getProperty(const ExtIdentifier& id) const = 0;
	virtual void setProperty(const ExtIdentifier& id, const ExtVariant& value) = 0;
	virtual bool removeProperty(const ExtIdentifier& id) = 0;

	virtual bool enumerate(ExtIdentifier*** ids, uint32_t* count) const = 0;

	virtual bool callExternal(const ExtIdentifier& id, const ExtVariant** args, uint32_t argc, ASObject** result) = 0;

	virtual void setException(const std::string& message) const = 0;
	virtual void setMarshallExceptions(bool marshall) = 0;
	virtual bool getMarshallExceptions() const = 0;
};

};

#endif /* BACKENDS_EXTSCRIPTOBJECT_H */
