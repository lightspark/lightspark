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

#ifndef SCRIPTING_FLASH_SYSTEM_APPLICATIONDOMAIN_H
#define SCRIPTING_FLASH_SYSTEM_APPLICATIONDOMAIN_H 1

#include "scripting/flash/utils/ByteArray.h"
#include "scripting/abcutils.h"
#include "backends/urlutils.h"

#define MIN_DOMAIN_MEMORY_LIMIT 1024
namespace lightspark
{
class Global;
class DefineScalingGridTag;
class DictionaryTag;
class FontTag;

class ApplicationDomain: public ASObject
{
private:
	std::vector<Global*> globalScopes;
	_NR<ByteArray> defaultDomainMemory;
	void cbDomainMemory(_NR<ByteArray> oldvalue);
	// list of classes where super class is not defined yet
	std::unordered_map<Class_base*,Class_base*> classesSuperNotFilled;
	// list of classes where interfaces are not yet linked
	std::vector<Class_base*> classesToLinkInterfaces;

	Mutex dictSpinlock;
	std::unordered_map < uint32_t, DictionaryTag* > dictionary;
	std::map < QName, DictionaryTag* > classesToBeBound;
	std::map < tiny_string,FontTag* > embeddedfonts;
	std::map < uint32_t,FontTag* > embeddedfontsByID;
	Mutex scalinggridsmutex;
	std::unordered_map < uint32_t, RECT > scalinggrids;
	URLInfo origin;
	//frameSize and frameRate are valid only after the header has been parsed
	RECT frameSize;
	float frameRate;
	URLInfo baseURL;
	AVM1Function* AVM1getClassConstructorIntern(uint32_t nameID);
public:
	//map of all classed defined in the swf. They own one reference to each class/template
	//key is the stringID of the class name (without namespace)
	std::multimap<uint32_t, Class_base*> customClasses;
	std::map<QName,std::unordered_set<uint32_t>*> customclassoverriddenmethods;
	/*
	 * Support for class aliases in AMF3 serialization
	 */
	std::map<tiny_string, Class_base*> aliasMap;
	std::map<QName, Template_base*> templates;

	uint32_t version;
	bool usesActionScript3;
	bool needsCaseInsensitiveNames();// returns true if the swf version of this domain or any parent domain is <= 6
	ByteArray* currentDomainMemory;
	ApplicationDomain(ASWorker* wrk, Class_base* c, _NR<ApplicationDomain> p=NullRef);
	void finalize() override;
	void prepareShutdown() override;
	std::map<const multiname*, Class_base*> classesBeingDefined;
	std::map<QName, Class_base*> instantiatedTemplates;

	void SetAllClassLinks();
	void AddClassLinks(Class_base* target);
	bool newClassRecursiveLink(Class_base* target, Class_base* c);
	void addSuperClassNotFilled(Class_base* cls);
	void copyBorrowedTraitsFromSuper(Class_base* cls);
	void addToDictionary(DictionaryTag* r);
	DictionaryTag* dictionaryLookup(int id);
	DictionaryTag* dictionaryLookupByName(uint32_t nameID, bool recursive=false);
	void registerEmbeddedFont(const tiny_string fontname, FontTag *tag);
	FontTag* getEmbeddedFont(const tiny_string fontname) const;
	FontTag* getEmbeddedFontByID(uint32_t fontID) const;
	void addToScalingGrids(const DefineScalingGridTag* r);
	RECT* ScalingGridsLookup(int id);
	void addBinding(const tiny_string& name, DictionaryTag *tag);
	void bindClass(const QName &classname, Class_inherit* cls);
	void checkBinding(DictionaryTag* tag);
	AVM1Function* AVM1getClassConstructor(DisplayObject *d);
	void setOrigin(const tiny_string& u, const tiny_string& filename="");
	URLInfo& getOrigin() { return origin; }
	void setFrameSize(const RECT& f);
	RECT getFrameSize() const;
	float getFrameRate() const DLL_PUBLIC;
	void setFrameRate(float f);
	void setBaseURL(const tiny_string& url);
	const URLInfo& getBaseURL();
	tiny_string findClassAlias(Class_base* cls);
	bool buildClassAndBindTag(const std::string& s, DictionaryTag* t, Class_inherit *derived_cls=nullptr);
	void buildClassAndInjectBase(const std::string& s, _R<RootMovieClip> base);
	Class_inherit* findClassInherit(const std::string& s);


	static void sinit(Class_base* c);
	void registerGlobalScope(Global* scope);
	Global* getLastGlobalScope() const  { return globalScopes.back(); }
	ASObject* getVariableByString(const std::string& name, ASObject*& target);
	bool findTargetByMultiname(const multiname& name, ASObject*& target, ASWorker* wrk);
	GET_VARIABLE_RESULT getVariableAndTargetByMultiname(asAtom& ret, const multiname& name, ASObject*& target,ASWorker* wrk);
	void getVariableAndTargetByMultinameIncludeTemplatedClasses(asAtom& ret, const multiname& name, ASObject*& target, ASWorker* wrk);

	/*
	 * This method is an opportunistic resolution operator used by the optimizer:
	 * Only returns the value if the variable has been already defined.
	 */
	ASObject* getVariableByMultinameOpportunistic(const multiname& name, ASWorker* wrk);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(_getCurrentDomain);
	ASFUNCTION_ATOM(_getMinDomainMemoryLength);
	ASFUNCTION_ATOM(hasDefinition);
	ASFUNCTION_ATOM(getDefinition);
	ASFUNCTION_ATOM(getQualifiedDefinitionNames);
	ASPROPERTY_GETTER_SETTER(_NR<ByteArray>, domainMemory);
	ASPROPERTY_GETTER(_NR<ApplicationDomain>, parentDomain);
	static void throwRangeError();
	template<class T>
	T readFromDomainMemory(uint32_t addr)
	{
		if(currentDomainMemory->getLength() < (addr+sizeof(T)))
		{
			throwRangeError();
			return T(0);
		}
		uint8_t* buf=currentDomainMemory->getBufferNoCheck();
		return *reinterpret_cast<T*>(buf+addr);
	}
	template<class T>
	void writeToDomainMemory(uint32_t addr, T val)
	{
		if(currentDomainMemory->getLength() < (addr+sizeof(T)))
		{
			throwRangeError();
			return;
		}
		uint8_t* buf=currentDomainMemory->getBufferNoCheck();
		*reinterpret_cast<T*>(buf+addr)=val;
	}
	template<class T>
	static void loadIntN(ApplicationDomain* appDomain,call_context* th)
	{
		RUNTIME_STACK_POP_CREATE(th,arg1);
		uint32_t addr=asAtomHandler::toUInt(*arg1);
		T ret=appDomain->readFromDomainMemory<T>(addr);
		ASATOM_DECREF_POINTER(arg1);
		RUNTIME_STACK_PUSH(th,asAtomHandler::fromInt(ret));
	}
	template<class T>
	static void storeIntN(ApplicationDomain* appDomain,call_context* th)
	{
		RUNTIME_STACK_POP_CREATE(th,arg1);
		RUNTIME_STACK_POP_CREATE(th,arg2);
		uint32_t addr=asAtomHandler::toUInt(*arg1);
		ASATOM_DECREF_POINTER(arg1);
		int32_t val=asAtomHandler::toInt(*arg2);
		ASATOM_DECREF_POINTER(arg2);
		appDomain->writeToDomainMemory<T>(addr, val);
	}
	template<class T>
	static FORCE_INLINE void loadIntN(ApplicationDomain* appDomain,asAtom& ret, asAtom& arg1)
	{
		uint32_t addr=asAtomHandler::toUInt(arg1);
		ByteArray* dm = appDomain->currentDomainMemory;
		if(dm->getLength() < (addr+sizeof(T)))
		{
			throwRangeError();
			return;
		}
		ret = asAtomHandler::fromInt(*reinterpret_cast<T*>(dm->getBufferNoCheck()+addr));
	}
	template<class T>
	static FORCE_INLINE void storeIntN(ApplicationDomain* appDomain, asAtom& arg1, asAtom& arg2)
	{
		uint32_t addr=asAtomHandler::toUInt(arg1);
		int32_t val=asAtomHandler::toInt(arg2);
		ByteArray* dm = appDomain->currentDomainMemory;
		if(dm->getLength() < (addr+sizeof(T)))
		{
			throwRangeError();
			return;
		}
		*reinterpret_cast<T*>(dm->getBufferNoCheck()+addr)=val;
	}

	static FORCE_INLINE void loadFloat(ApplicationDomain* appDomain,call_context *th)
	{
		RUNTIME_STACK_POP_CREATE(th,arg1);
		uint32_t addr=asAtomHandler::toUInt(*arg1);
		number_t ret=appDomain->readFromDomainMemory<float>(addr);
		ASATOM_DECREF_POINTER(arg1);
		RUNTIME_STACK_PUSH(th,asAtomHandler::fromNumber(appDomain->getInstanceWorker(),ret,false));
	}
	static FORCE_INLINE void loadFloat(ApplicationDomain* appDomain,asAtom& ret, asAtom& arg1, uint16_t resultlocalnumber=UINT16_MAX)
	{
		uint32_t addr=asAtomHandler::toUInt(arg1);
		number_t res=appDomain->readFromDomainMemory<float>(addr);
		ASObject* oldret = asAtomHandler::getObject(ret);
		if (resultlocalnumber != UINT16_MAX)
		{
			asAtomHandler::setNumber(ret,appDomain->getInstanceWorker(),res,resultlocalnumber);
			if (oldret)
				oldret->decRef();
		}
		else if (asAtomHandler::replaceNumber(ret,appDomain->getInstanceWorker(),res))
		{
			if (oldret)
				oldret->decRef();
		}
	}
	static FORCE_INLINE void loadDouble(ApplicationDomain* appDomain,call_context *th)
	{
		RUNTIME_STACK_POP_CREATE(th,arg1);
		uint32_t addr=asAtomHandler::toUInt(*arg1);
		number_t res=appDomain->readFromDomainMemory<double>(addr);
		ASATOM_DECREF_POINTER(arg1);
		RUNTIME_STACK_PUSH(th,asAtomHandler::fromNumber(appDomain->getInstanceWorker(),res,false));
	}
	static FORCE_INLINE void loadDouble(ApplicationDomain* appDomain,asAtom& ret, asAtom& arg1, uint16_t resultlocalnumber=UINT16_MAX)
	{
		uint32_t addr=asAtomHandler::toUInt(arg1);
		number_t res=appDomain->readFromDomainMemory<double>(addr);
		ASObject* oldret = asAtomHandler::getObject(ret);
		if (resultlocalnumber != UINT16_MAX)
		{
			asAtomHandler::setNumber(ret,appDomain->getInstanceWorker(),res,resultlocalnumber);
			if (oldret)
				oldret->decRef();
		}
		else if (asAtomHandler::replaceNumber(ret,appDomain->getInstanceWorker(),res))
		{
			if (oldret)
				oldret->decRef();
		}
	}

	static FORCE_INLINE void storeFloat(ApplicationDomain* appDomain,call_context *th)
	{
		RUNTIME_STACK_POP_CREATE(th,arg1);
		RUNTIME_STACK_POP_CREATE(th,arg2);
		number_t addr=asAtomHandler::toNumber(*arg1);
		ASATOM_DECREF_POINTER(arg1);
		float val=(float)asAtomHandler::toNumber(*arg2);
		ASATOM_DECREF_POINTER(arg2);
		appDomain->writeToDomainMemory<float>(addr, val);
	}
	static FORCE_INLINE void storeFloat(ApplicationDomain* appDomain, asAtom& arg1, asAtom& arg2)
	{
		number_t addr=asAtomHandler::toNumber(arg1);
		float val=(float)asAtomHandler::toNumber(arg2);
		appDomain->writeToDomainMemory<float>(addr, val);
	}

	static FORCE_INLINE void storeDouble(ApplicationDomain* appDomain,call_context *th)
	{
		RUNTIME_STACK_POP_CREATE(th,arg1);
		RUNTIME_STACK_POP_CREATE(th,arg2);
		number_t addr=asAtomHandler::toNumber(*arg1);
		ASATOM_DECREF_POINTER(arg1);
		double val=asAtomHandler::toNumber(*arg2);
		ASATOM_DECREF_POINTER(arg2);
		appDomain->writeToDomainMemory<double>(addr, val);
	}
	static FORCE_INLINE void storeDouble(ApplicationDomain* appDomain, asAtom& arg1, asAtom& arg2)
	{
		number_t addr=asAtomHandler::toNumber(arg1);
		double val=asAtomHandler::toNumber(arg2);
		appDomain->writeToDomainMemory<double>(addr, val);
	}
	void checkDomainMemory();
};

}
#endif /* SCRIPTING_FLASH_SYSTEM_APPLICATIONDOMAIN_H */
