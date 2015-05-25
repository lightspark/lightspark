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

#ifndef SCRIPTING_FLASH_SYSTEM_FLASHSYSTEM_H
#define SCRIPTING_FLASH_SYSTEM_FLASHSYSTEM_H 1

#include "compat.h"
#include "asobject.h"
#include "scripting/flash/utils/ByteArray.h"
#include "scripting/toplevel/toplevel.h"
#include "scripting/flash/events/flashevents.h"

namespace lightspark
{

class SecurityDomain;

class Capabilities: public ASObject
{
public:
	DLL_PUBLIC static const char* EMULATED_VERSION;
	static const char* MANUFACTURER;
	Capabilities(Class_base* c):ASObject(c){}
	static void sinit(Class_base* c);
	ASFUNCTION(_getLanguage);
	ASFUNCTION(_getPlayerType);
	ASFUNCTION(_getCPUArchitecture);
	ASFUNCTION(_getIsDebugger);
	ASFUNCTION(_getIsEmbeddedInAcrobat);
	ASFUNCTION(_getLocalFileReadDisable);
	ASFUNCTION(_getManufacturer);
	ASFUNCTION(_getOS);
	ASFUNCTION(_getVersion);
	ASFUNCTION(_getServerString);
	ASFUNCTION(_getScreenResolutionX);
	ASFUNCTION(_getScreenResolutionY);
	ASFUNCTION(_getHasAccessibility);
	ASFUNCTION(_getScreenDPI);
};

class ApplicationDomain: public ASObject
{
private:
	std::vector<Global*> globalScopes;
public:
	ApplicationDomain(Class_base* c, _NR<ApplicationDomain> p=NullRef);
	void finalize();
	std::map<const multiname*, Class_base*> classesBeingDefined;
	
	// list of classes where super class is not defined yet 
	std::list<Class_base*> classesSuperNotFilled;

	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	void registerGlobalScope(Global* scope);
	ASObject* getVariableByString(const std::string& name, ASObject*& target);
	bool findTargetByMultiname(const multiname& name, ASObject*& target);
	ASObject* getVariableAndTargetByMultiname(const multiname& name, ASObject*& target);
	/*
	 * This method is an opportunistic resolution operator used by the optimizer:
	 * Only returns the value if the variable has been already defined.
	 */
	ASObject* getVariableByMultinameOpportunistic(const multiname& name);
	ASFUNCTION(_constructor);
	ASFUNCTION(_getCurrentDomain);
	ASFUNCTION(_getMinDomainMemoryLength);
	ASFUNCTION(hasDefinition);
	ASFUNCTION(getDefinition);
	ASPROPERTY_GETTER_SETTER(_NR<ByteArray>, domainMemory);
	ASPROPERTY_GETTER(_NR<ApplicationDomain>, parentDomain);
	template<class T>
	T readFromDomainMemory(uint32_t addr) const
	{
		if(domainMemory.isNull())
			throw RunTimeException("No memory domain is set");
		uint32_t bufLen=domainMemory->getLength();
		if(bufLen < (addr+sizeof(T)))
			throw RunTimeException("Memory domain access is out of bounds");
		uint8_t* buf=domainMemory->getBuffer(bufLen, false);
		return *reinterpret_cast<T*>(buf+addr);
	}
	template<class T>
	void writeToDomainMemory(uint32_t addr, T val)
	{
		if(domainMemory.isNull())
			throw RunTimeException("No memory domain is set");
		uint32_t bufLen=domainMemory->getLength();
		if(bufLen < (addr+sizeof(T)))
			throw RunTimeException("Memory domain access is out of bounds");
		uint8_t* buf=domainMemory->getBuffer(bufLen, false);
		*reinterpret_cast<T*>(buf+addr)=val;
	}
};

class LoaderContext: public ASObject
{
public:
	LoaderContext(Class_base* c);
	static void sinit(Class_base* c);
	ASFUNCTION(_constructor);
	ASPROPERTY_GETTER_SETTER(bool, allowCodeImport);
	ASPROPERTY_GETTER_SETTER(_NR<ApplicationDomain>, applicationDomain);
	ASPROPERTY_GETTER_SETTER(bool, checkPolicyFile);
	ASPROPERTY_GETTER_SETTER(_NR<ASObject>, parameters);
	ASPROPERTY_GETTER_SETTER(_NR<SecurityDomain>, securityDomain);
	void finalize();
	bool getAllowCodeImport();
	bool getCheckPolicyFile();
};

class SecurityDomain: public ASObject
{
public:
	SecurityDomain(Class_base* c):ASObject(c){}
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
	ASFUNCTION(_getCurrentDomain);
};

class Security: public ASObject
{
public:
	Security(Class_base* c):ASObject(c){}
	static void sinit(Class_base* c);
	ASFUNCTION(_getExactSettings);
	ASFUNCTION(_setExactSettings);
	ASFUNCTION(_getSandboxType);
	ASFUNCTION(allowDomain);
	ASFUNCTION(allowInsecureDomain);
	ASFUNCTION(loadPolicyFile);
	ASFUNCTION(showSettings);
	ASFUNCTION(pageDomain);
};

ASObject* fscommand(ASObject* obj,ASObject* const* args, const unsigned int argslen);

class System: public ASObject
{
public:
	System(Class_base* c):ASObject(c){}
	static void sinit(Class_base* c);
	ASFUNCTION(totalMemory);
};
class ASWorker: public EventDispatcher
{
public:
	ASWorker(Class_base* c);
	static void sinit(Class_base*);
	ASFUNCTION(_getCurrent);
};

}
#endif /* SCRIPTING_FLASH_SYSTEM_FLASHSYSTEM_H */
