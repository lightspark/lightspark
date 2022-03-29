/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2010-2013  Alessandro Pignotti (a.pignotti@sssup.it)
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
#include <memory>
#include <stack>

#include "asobject.h"
#include "compat.h"
#include "threading.h"

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
	std::string strValue;
	int32_t intValue;
	EI_TYPE type;
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
	/*
	 * NOTE: The caller must make sure that the property exists
	 * using hasProperty
	 */
	const ExtVariant& getProperty(const ExtIdentifier& id) const;
	void setProperty(const ExtIdentifier& id, const ExtVariant& value);
	bool removeProperty(const ExtIdentifier& id);

	bool enumerate(ExtIdentifier*** ids, uint32_t* count) const;
	uint32_t getLength() const { return properties.size(); }

	enum EO_TYPE { EO_OBJECT, EO_ARRAY };
	EO_TYPE getType() const { return type; }
	void setType(EO_TYPE _type) { type = _type; }
protected:
	std::map<ExtIdentifier, ExtVariant> properties;
	EO_TYPE type;
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
	ExtVariant(std::map<const ASObject*, std::unique_ptr<ExtObject>>& objectsMap, _R<ASObject> other);

	~ExtVariant() {}

	enum EV_TYPE
	{ EV_STRING, EV_INT32, EV_DOUBLE, EV_BOOLEAN, EV_OBJECT, EV_NULL, EV_VOID };
	EV_TYPE getType() const { return type; }

	// These methods return the value of the ExtVariant.
	// Returned values for non-matching types are undefined.
	// As such, don't get a string value for an integer ExtVariant
	std::string getString() const { return strValue; }
	int32_t getInt() const { return intValue; }
	double getDouble() const { return doubleValue; }
	bool getBoolean() const { return booleanValue; }
	/*
	 * ExtObject instances are always owned by the objectMaps
	 */
	ExtObject* getObject() const { return objectValue; }
	ASObject* getASObject(ASWorker* wrk, std::map<const lightspark::ExtObject*, lightspark::ASObject*>& objectsMap) const;
protected:
	std::string strValue;
	ExtObject* objectValue;
	double doubleValue;
	int32_t intValue;
	EV_TYPE type;
	bool booleanValue;
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
	virtual bool getResult(std::map<const ASObject*, std::unique_ptr<ExtObject>>& objectsMap,
			const ExtScriptObject& so, const ExtVariant** result)=0;
protected:
	tiny_string exception;
	bool success;
	bool exceptionThrown;
};

class ExternalCallEvent;
class IFunction;
/**
 * ExtCallback specialization for IFunctions
 */
class DLL_PUBLIC ExtASCallback : public ExtCallback
{
private:
	bool funcWasCalled;
	asAtom func=asAtomHandler::invalidAtom;
	_NR<ExternalCallEvent> funcEvent;
	ASObject* result;
        ASObject** asArgs;
public:
	ExtASCallback(asAtom _func);
	~ExtASCallback();

	// Don't forget to delete this copy after use
	ExtASCallback* copy() { return new ExtASCallback(func); }

	void call(const ExtScriptObject& so, const ExtIdentifier& id,
		const ExtVariant** args, uint32_t argc, bool synchronous);
	void wait();
	void wakeUp();
	// The result variable should be "delete"d by the caller after use.
	bool getResult(std::map<const ASObject*, std::unique_ptr<ExtObject>>& objectsMap,
			const ExtScriptObject& so, const ExtVariant** _result);
};

/**
 * ExtCallback specialization for builtin functions
 */
class DLL_PUBLIC ExtBuiltinCallback : public ExtCallback
{
public:
	// The signature for a hard-coded callback function.
	typedef bool (*funcPtr)(const ExtScriptObject& so, const ExtIdentifier& id,
		const ExtVariant** args, uint32_t argc, const ExtVariant** result);

	ExtBuiltinCallback(funcPtr _func) : func(_func), result(NULL) {}
	~ExtBuiltinCallback() {}
	
	// Don't forget to delete this copy after use
	ExtBuiltinCallback* copy() { return new ExtBuiltinCallback(func); }

	void call(const ExtScriptObject& so, const ExtIdentifier& id,
		const ExtVariant** args, uint32_t argc, bool synchronous);
	void wait();
	void wakeUp();
	// The result variable should be "delete"d by the caller after use.
	bool getResult(std::map<const ASObject*, std::unique_ptr<ExtObject>>& objectsMap,
			const ExtScriptObject& so, const ExtVariant** _result);
private:
	funcPtr func;
	const ExtVariant* result;
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
	ExtScriptObject(SystemState* sys);
	virtual ~ExtScriptObject();
	// Stops all waiting external calls, should be called before destruction.
	// Actual destruction should be initiated by the browser, as a last step of destruction.
	void destroy();

	SystemState* getSystemState() const { return m_sys; }
	
	void assertThread() { assert( SDL_GetThreadID(nullptr) == mainThreadID); }

	// Methods
	bool hasMethod(const lightspark::ExtIdentifier& id) const
	{
		return methods.find(id) != methods.end();
	}
	void setMethod(const lightspark::ExtIdentifier& id, lightspark::ExtCallback* func)
	{
		methods[id] = func;
	}
	bool removeMethod(const lightspark::ExtIdentifier& id);

	// Properties
	bool hasProperty(const lightspark::ExtIdentifier& id) const
	{
		return properties.find(id) != properties.end();
	}
	const ExtVariant& getProperty(const lightspark::ExtIdentifier& id) const;
	void setProperty(const lightspark::ExtIdentifier& id, const lightspark::ExtVariant& value)
	{
		properties[id] = value;
	}
	bool removeProperty(const lightspark::ExtIdentifier& id);

	bool enumerate(ExtIdentifier*** ids, uint32_t* count) const;
	virtual ExtIdentifier* createEnumerationIdentifier(const ExtIdentifier& id) const = 0;

	enum HOST_CALL_TYPE {EXTERNAL_CALL};
	typedef struct {
		ExtScriptObject* so;
		Semaphore* callStatus;
		HOST_CALL_TYPE type;
		void* arg1;
		void* arg2;
		void* arg3;
		void* arg4;
		void* returnValue;
	} HOST_CALL_DATA;
	// This method allows calling some method, while making sure
	// no unintended blocking occurs.
	void doHostCall(HOST_CALL_TYPE type, void* returnValue,
		void* arg1, void* arg2=NULL, void* arg3=NULL, void* arg4=NULL);
	static void hostCallHandler(void* d);
	
	bool callExternal(const ExtIdentifier& id, const ExtVariant** args, uint32_t argc, ASObject** result);

	virtual void setException(const std::string& message) const = 0;
	
	virtual void callAsync(HOST_CALL_DATA* data) = 0;

	// This is called from hostCallHandler() via doHostCall(EXTERNAL_CALL, ...)
	virtual bool callExternalHandler(const char* scriptString, const lightspark::ExtVariant** args, uint32_t argc, lightspark::ASObject** result) = 0;
	
	void setMarshallExceptions(bool marshall) { marshallExceptions = marshall; }
	bool getMarshallExceptions() const { return marshallExceptions; }
	// Standard methods
	// These methods are standard to every flash instance.
	// They provide features such as getting/setting internal variables,
	// going to a frame, pausing etc... to the external container.
	static bool stdGetVariable(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, const lightspark::ExtVariant** result);
	static bool stdSetVariable(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, const lightspark::ExtVariant** result);
	static bool stdGotoFrame(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, const lightspark::ExtVariant** result);
	static bool stdIsPlaying(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, const lightspark::ExtVariant** result);
	static bool stdLoadMovie(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, const lightspark::ExtVariant** result);
	static bool stdPan(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, const lightspark::ExtVariant** result);
	static bool stdPercentLoaded(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, const lightspark::ExtVariant** result);
	static bool stdPlay(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, const lightspark::ExtVariant** result);
	static bool stdRewind(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, const lightspark::ExtVariant** result);
	static bool stdStopPlay(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, const lightspark::ExtVariant** result);
	static bool stdSetZoomRect(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, const lightspark::ExtVariant** result);
	static bool stdZoom(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, const lightspark::ExtVariant** result);
	static bool stdTotalFrames(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, const lightspark::ExtVariant** result);
protected:
	bool doinvoke(const ExtIdentifier &id, const ExtVariant**args, uint32_t argc, const ExtVariant *result);
	SystemState* m_sys;
private:
	// Used to determine if a method is called in the main plugin thread
	SDL_threadID mainThreadID;

	// Provides mutual exclusion for external calls
	Mutex mutex;
	std::stack<Semaphore*> callStatusses;
	Mutex externalCall;
	Mutex hostCall;

	// The root callback currently being invoked. If this is not NULL
	// when invoke() gets called, we can assume the invoke()
	// is nested inside another one.
	lightspark::ExtCallback* currentCallback;
	// The data for the external call that needs to be made.
	// If a callback is woken up and this is not NULL,
	// it was a forced wake-up and we should call an external method.
	HOST_CALL_DATA* hostCallData;

	// This map stores this object's methods & properties
	// If an entry is set with a ExtIdentifier or ExtVariant,
	// they get converted to NPIdentifierObject or NPVariantObject by copy-constructors.
	std::map<ExtIdentifier, ExtVariant> properties;
	std::map<ExtIdentifier, lightspark::ExtCallback*> methods;

	// True if this object is being shut down
	bool shuttingDown;
	// True if exceptions should be marshalled to the container
	bool marshallExceptions;
	
	virtual void sendExternalCallSignal() {}
};

};

#endif /* BACKENDS_EXTSCRIPTOBJECT_H */
