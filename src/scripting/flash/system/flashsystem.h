/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2011  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef _FLASH_SYSTEM_H
#define _FLASH_SYSTEM_H

#include "compat.h"
#include "asobject.h"
#include "../utils/flashutils.h"

namespace lightspark
{

class Capabilities: public ASObject
{
public:
	DLL_PUBLIC static const char* EMULATED_VERSION;
	Capabilities(){};
	static void sinit(Class_base* c);
	ASFUNCTION(_getLanguage);
	ASFUNCTION(_getPlayerType);
	ASFUNCTION(_getCPUArchitecture);
	ASFUNCTION(_getIsDebugger);
	ASFUNCTION(_getIsEmbeddedInAcrobat);
	ASFUNCTION(_getLocalFileReadDisable);
	ASFUNCTION(_getOS);
	ASFUNCTION(_getVersion);
	ASFUNCTION(_getServerString);
};

class ApplicationDomain: public ASObject
{
private:
	std::vector<Global*> globalScopes;
public:
	ApplicationDomain(){}
	void finalize();
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	void registerGlobalScope(Global* scope);
	ASObject* getVariableByString(const std::string& name, ASObject*& target);
	ASObject* getVariableAndTargetByMultiname(const multiname& name, ASObject*& target);
	ASFUNCTION(_constructor);
	ASFUNCTION(_getCurrentDomain);
	ASFUNCTION(_getMinDomainMemoryLength);
	ASFUNCTION(hasDefinition);
	ASFUNCTION(getDefinition);
	ASPROPERTY_GETTER_SETTER(_NR<ByteArray>, domainMemory);
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

class SecurityDomain: public ASObject
{
public:
	SecurityDomain(){}
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
	ASFUNCTION(_getCurrentDomain);
};

class Security: public ASObject
{
public:
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

};
#endif
