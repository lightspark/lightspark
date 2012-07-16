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

#ifndef SCRIPTING_FLASH_SYSTEM_FLASHSYSTEM_H
#define SCRIPTING_FLASH_SYSTEM_FLASHSYSTEM_H 1

#include "compat.h"
#include "asobject.h"
#include "scripting/flash/utils/flashutils.h"

namespace lightspark
{

class SecurityDomain;

class Capabilities: public ASObject
{
public:
	DLL_PUBLIC static const char* EMULATED_VERSION;
	static const char* MANUFACTURER;
	Capabilities(Class_base* c):ASObject(c){};
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
};

class ApplicationDomain: public ASObject
{
private:
	std::vector<Global*> globalScopes;
public:
	ApplicationDomain(Class_base* c, _NR<ApplicationDomain> p=NullRef);
	void finalize();
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
	LoaderContext(Class_base* c):ASObject(c){};
	static void sinit(Class_base* c);
	ASFUNCTION(_constructor);
	ASPROPERTY_GETTER_SETTER(_NR<ApplicationDomain>, applicationDomain);
	ASPROPERTY_GETTER_SETTER(_NR<SecurityDomain>, securityDomain);
	void finalize();
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
};

ASObject* fscommand(ASObject* obj,ASObject* const* args, const unsigned int argslen);

class System: public ASObject
{
public:
	System(Class_base* c):ASObject(c){}
	static void sinit(Class_base* c);
	ASFUNCTION(totalMemory);
};

};
#endif /* SCRIPTING_FLASH_SYSTEM_FLASHSYSTEM_H */
