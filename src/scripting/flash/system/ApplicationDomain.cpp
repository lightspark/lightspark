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

#include "scripting/flash/system/ApplicationDomain.h"
#include "scripting/flash/display/RootMovieClip.h"
#include "scripting/flash/display/LoaderInfo.h"
#include "scripting/toplevel/Global.h"
#include "scripting/toplevel/UInteger.h"
#include "scripting/toplevel/Vector.h"
#include "scripting/abc.h"
#include "scripting/argconv.h"
#include "parsing/tags.h"
#include <glib.h>

using namespace lightspark;

ApplicationDomain::ApplicationDomain(ASWorker* wrk, Class_base* c, _NR<ApplicationDomain> p):ASObject(wrk,c,T_OBJECT,SUBTYPE_APPLICATIONDOMAIN)
	,defaultDomainMemory(Class<ByteArray>::getInstanceSNoArgs(wrk)),frameRate(0),version(0),usesActionScript3(false), parentDomain(p)
{
	defaultDomainMemory->setLength(MIN_DOMAIN_MEMORY_LIMIT);
	currentDomainMemory=defaultDomainMemory.getPtr();
}

void ApplicationDomain::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED | CLASS_FINAL);
	//Static
	c->setDeclaredMethodByQName("currentDomain","",c->getSystemState()->getBuiltinFunction(_getCurrentDomain,0,Class<ApplicationDomain>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("MIN_DOMAIN_MEMORY_LENGTH","",c->getSystemState()->getBuiltinFunction(_getMinDomainMemoryLength,0,Class<UInteger>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
	//Instance
	c->setDeclaredMethodByQName("hasDefinition","",c->getSystemState()->getBuiltinFunction(hasDefinition,1,Class<Boolean>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getDefinition","",c->getSystemState()->getBuiltinFunction(getDefinition,0,Class<ASObject>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getQualifiedDefinitionNames","",c->getSystemState()->getBuiltinFunction(getQualifiedDefinitionNames,0,Class<Vector>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,domainMemory,ByteArray);
	REGISTER_GETTER_RESULTTYPE(c,parentDomain,ApplicationDomain);
}

ASFUNCTIONBODY_GETTER_SETTER_CB(ApplicationDomain,domainMemory,cbDomainMemory)
ASFUNCTIONBODY_GETTER(ApplicationDomain,parentDomain)
void ApplicationDomain::cbDomainMemory(_NR<ByteArray> oldvalue)
{
	if (!this->domainMemory.isNull() && this->domainMemory->getLength() < MIN_DOMAIN_MEMORY_LIMIT)
	{
		createError<ASError>(this->getInstanceWorker(),kEndOfFileError);
		domainMemory=oldvalue;
		return;
	}
	checkDomainMemory();
}

void ApplicationDomain::finalize()
{
	ASObject::finalize();
	for(auto it=dictionary.begin();it!=dictionary.end();++it)
		delete it->second;
	dictionary.clear();
	aliasMap.clear();
	domainMemory.reset();
	defaultDomainMemory.reset();
	for(auto it = instantiatedTemplates.begin(); it != instantiatedTemplates.end(); ++it)
		it->second->finalize();
	//Free template instantations by decRef'ing them
	for(auto it = instantiatedTemplates.begin(); it != instantiatedTemplates.end(); ++it)
		it->second->decRef();
	globalScopes.clear();
	instantiatedTemplates.clear();
}

void ApplicationDomain::prepareShutdown()
{
	if (this->preparedforshutdown)
		return;
	ASObject::prepareShutdown();
	aliasMap.clear();
	if (domainMemory)
		domainMemory->prepareShutdown();
	if (defaultDomainMemory)
		defaultDomainMemory->prepareShutdown();
	if (parentDomain)
		parentDomain->prepareShutdown();
	for(auto it = instantiatedTemplates.begin(); it != instantiatedTemplates.end(); ++it)
		it->second->prepareShutdown();
}

void ApplicationDomain::addSuperClassNotFilled(Class_base *cls)
{
	auto i = classesBeingDefined.cbegin();
	while (i != classesBeingDefined.cend())
	{
		if(i->second == cls->super.getPtr())
		{
			classesSuperNotFilled.insert(make_pair(cls,cls->super.getPtr()));
			break;
		}
		i++;
	}
}

void ApplicationDomain::SetAllClassLinks()
{
	for (unsigned int i = 0; i < classesToLinkInterfaces.size(); i++)
	{
		Class_base* cls = classesToLinkInterfaces[i];
		if (!cls)
			continue;
		if (newClassRecursiveLink(cls, cls))
			classesToLinkInterfaces[i] = nullptr;
	}
}
void ApplicationDomain::AddClassLinks(Class_base* target)
{
	classesToLinkInterfaces.push_back(target);
}

bool ApplicationDomain::newClassRecursiveLink(Class_base* target, Class_base* c)
{
	if(c->super)
	{
		if (!newClassRecursiveLink(target, c->super.getPtr()))
			return false;
	}
	bool bAllDefined = false;
	const vector<Class_base*>& interfaces=c->getInterfaces(&bAllDefined);
	if (!bAllDefined)
	{
		return false;
	}
	for(unsigned int i=0;i<interfaces.size();i++)
	{
		LOG_CALL("Linking with interface " << interfaces[i]->class_name);
		interfaces[i]->linkInterface(target);
	}
	return true;
}

void ApplicationDomain::copyBorrowedTraitsFromSuper(Class_base *cls)
{
	if (!cls)
		return;
	bool super_initialized=true;
	bool cls_initialized=cls->super.isNull() || classesSuperNotFilled.empty();
	if (cls->super)
	{
		for (auto itsup = classesSuperNotFilled.begin();itsup != classesSuperNotFilled.end(); itsup++)
		{
			if(itsup->second == cls->super.getPtr())
			{
				super_initialized=false;
				break;
			}
		}
	}
	auto it = classesSuperNotFilled.begin();
	while (it != classesSuperNotFilled.end())
	{
		if(it->second == cls)
		{
			it->first->copyBorrowedTraits(cls);
			if (super_initialized)
			{
				it = classesSuperNotFilled.erase(it);
				cls_initialized=true;
			}
			else
			{
				// cls->super was not yet initialized, make sure all classes that have cls as super get borrowed traits from cls->super
				it->second = cls->super.getPtr();
				it++;
			}
		}
		else
			it++;
	}
	if (cls_initialized)
	{
		// borrowed traits are all available for this class, so we can link the interfaces
		if(!cls->isInterface)
		{
			//Link all the interfaces for this class and all the bases
			if (!newClassRecursiveLink(cls, cls))
			{
				// remember classes where not all interfaces are defined yet
				AddClassLinks(cls);
			}
		}
	}
}

void ApplicationDomain::addToDictionary(DictionaryTag* r)
{
	Locker l(dictSpinlock);
	dictionary[r->getId()] = r;
}

DictionaryTag* ApplicationDomain::dictionaryLookup(int id)
{
	Locker l(dictSpinlock);
	auto it = dictionary.find(id);
	if(it==dictionary.end())
	{
		LOG(LOG_ERROR,"No such Id on dictionary " << id << " for " << origin);
		//throw RunTimeException("Could not find an object on the dictionary");
		return nullptr;
	}
	return it->second;
}

DictionaryTag* ApplicationDomain::dictionaryLookupByName(uint32_t nameID, bool recursive)
{
	uint32_t nameIDcaseInsensitive=UINT32_MAX;
	if (this->needsCaseInsensitiveNames())
		nameIDcaseInsensitive = getSystemState()->getUniqueStringId(getSystemState()->getStringFromUniqueId(nameID),false);
	Locker l(dictSpinlock);
	auto it = dictionary.begin();
	for(;it!=dictionary.end();++it)
	{
		if(it->second->nameID==nameID || (nameIDcaseInsensitive != UINT32_MAX && it->second->nameID==nameIDcaseInsensitive))
			return it->second;
	}
	if (recursive && parentDomain)
		return parentDomain->dictionaryLookupByName(nameID,recursive);
	LOG(LOG_ERROR,"No such name on dictionary " << getSystemState()->getStringFromUniqueId(nameID) << " for " << origin);
	return nullptr;
}

void ApplicationDomain::registerEmbeddedFont(const tiny_string fontname, FontTag* tag)
{
	if (!fontname.empty())
	{
		auto it = embeddedfonts.find(fontname);
		if (it == embeddedfonts.end())
		{
			embeddedfonts[fontname] = tag;
			// it seems that adobe allows fontnames to be lowercased and stripped of spaces and numbers
			tiny_string tmp = fontname.lowercase();
			tiny_string fontnamenormalized;
			for (auto it = tmp.begin();it != tmp.end(); it++)
			{
				if (*it == ' ' || (*it >= '0' &&  *it <= '9'))
					continue;
				fontnamenormalized += *it;
			}
			embeddedfonts[fontnamenormalized] = tag;
		}
	}
	embeddedfontsByID[tag->getId()] = tag;
}

FontTag* ApplicationDomain::getEmbeddedFont(const tiny_string fontname) const
{
	auto it = embeddedfonts.find(fontname);
	if (it != embeddedfonts.end())
		return it->second;
	it = embeddedfonts.find(fontname.lowercase());
	if (it != embeddedfonts.end())
		return it->second;
	return nullptr;
}

FontTag* ApplicationDomain::getEmbeddedFontByID(uint32_t fontID) const
{
	auto it = embeddedfontsByID.find(fontID);
	if (it != embeddedfontsByID.end())
		return it->second;
	return nullptr;
}

void ApplicationDomain::addToScalingGrids(const DefineScalingGridTag* r)
{
	Locker l(scalinggridsmutex);
	scalinggrids[r->CharacterId] = r->Splitter;
}

RECT* ApplicationDomain::ScalingGridsLookup(int id)
{
	Locker l(scalinggridsmutex);
	auto it = scalinggrids.find(id);
	if(it==scalinggrids.end())
		return nullptr;
	return &(*it).second;
}

void ApplicationDomain::addBinding(const tiny_string& name, DictionaryTag *tag)
{
	// This function will be called only be the parsing thread,
	// and will only access the last frame, so no locking needed.
	tag->bindingclassname = name;
	uint32_t pos = name.rfind(".");
	if (pos== tiny_string::npos)
		classesToBeBound[QName(getSystemState()->getUniqueStringId(name),BUILTIN_STRINGS::EMPTY)] =  tag;
	else
		classesToBeBound[QName(getSystemState()->getUniqueStringId(name.substr(pos+1,name.numChars()-(pos+1))),getSystemState()->getUniqueStringId(name.substr(0,pos)))] = tag;
}

void ApplicationDomain::bindClass(const QName& classname, Class_inherit* cls)
{
	if (cls->isBinded() || classesToBeBound.empty())
		return;

	auto it=classesToBeBound.find(classname);
	if(it!=classesToBeBound.end())
	{
		DictionaryTag* tag = it->second;
		classesToBeBound.erase(it);
		cls->bindToTag(tag);
	}
}

void ApplicationDomain::checkBinding(DictionaryTag *tag)
{
	if (tag->bindingclassname.empty())
		return;
	multiname clsname(nullptr);
	clsname.name_type=multiname::NAME_STRING;
	clsname.isAttribute = false;

	uint32_t pos = tag->bindingclassname.rfind(".");
	tiny_string ns;
	tiny_string nm;
	if (pos != tiny_string::npos)
	{
		nm = tag->bindingclassname.substr(pos+1,tag->bindingclassname.numBytes());
		ns = tag->bindingclassname.substr(0,pos);
		clsname.hasEmptyNS=false;
	}
	else
	{
		nm = tag->bindingclassname;
		ns = "";
	}
	clsname.name_s_id=getSystemState()->getUniqueStringId(nm);
	clsname.ns.push_back(nsNameAndKind(getSystemState(),ns,NAMESPACE));

	ASObject* typeObject = nullptr;
	auto i = classesBeingDefined.cbegin();
	while (i != classesBeingDefined.cend())
	{
		if(i->first->name_s_id == clsname.name_s_id && i->first->ns[0].nsRealId == clsname.ns[0].nsRealId)
		{
			typeObject = i->second;
			break;
		}
		i++;
	}
	if (typeObject == nullptr)
	{
		ASObject* target;
		asAtom o=asAtomHandler::invalidAtom;
		getVariableAndTargetByMultiname(o,clsname,target,getInstanceWorker());
		if (asAtomHandler::isValid(o))
			typeObject=asAtomHandler::getObject(o);
	}
	if (typeObject != nullptr)
	{
		Class_inherit* cls = typeObject->as<Class_inherit>();
		if (cls)
		{
			buildClassAndBindTag(tag->bindingclassname.raw_buf(), tag,cls);
			tag->bindedTo=cls;
			tag->bindingclassname = "";
			cls->bindToTag(tag);
		}
	}
}
bool ApplicationDomain::buildClassAndBindTag(const string& s, DictionaryTag* t, Class_inherit* derived_cls)
{
	Class_inherit* derived_class_tmp = derived_cls? derived_cls : findClassInherit(s);
	if(!derived_class_tmp)
		return false;
	derived_class_tmp->checkScriptInit();
	//It seems to be acceptable for the same base to be binded multiple times.
	//In such cases the first binding is bidirectional (instances created using PlaceObject
	//use the binded class and instances created using 'new' use the binded tag). Any other
	//bindings will be unidirectional (only instances created using new will use the binded tag)
	if(t->bindedTo==nullptr)
		t->bindedTo=derived_class_tmp;

	derived_class_tmp->bindToTag(t);
	return true;
}
void ApplicationDomain::buildClassAndInjectBase(const string& s, _R<RootMovieClip> base)
{
	if (base.getPtr() != base->getSystemState()->mainClip
		&& !base->isOnStage()
		&& base->loaderInfo->hasAVM1Target())
	{
		base->setConstructorCallComplete();
		return; // loader was removed from stage. clip is ignored
	}
	Class_inherit* derived_class_tmp = findClassInherit(s);
	if(!derived_class_tmp)
		return;

	//Let's override the class
	base->setClass(derived_class_tmp);
	// ensure that traits are initialized for movies loaded from actionscript
	base->setIsInitialized(false);
	derived_class_tmp->bindToRoot();
	// the root movie clip may have it's own constructor, so we make sure it is called
	asAtom r = asAtomHandler::fromObject(base.getPtr());
	derived_class_tmp->handleConstruction(r,nullptr,0,true);
	base->setConstructorCallComplete();
}

Class_inherit* ApplicationDomain::findClassInherit(const string& s)
{
	LOG(LOG_CALLS,"Setting class name to " << s);
	ASObject* target;
	ASObject* derived_class=getVariableByString(s,target);
	if(derived_class==nullptr || derived_class->getObjectType()==T_NULL)
	{
		//LOG(LOG_ERROR,"Class " << s << " not found in global for "<<root->getOrigin());
		//throw RunTimeException("Class not found in global");
		return nullptr;
	}

	assert_and_throw(derived_class->getObjectType()==T_CLASS);

	//Now the class is valid, check that it's not a builtin one
	assert_and_throw(static_cast<Class_base*>(derived_class)->class_index!=-1);
	Class_inherit* derived_class_tmp=static_cast<Class_inherit*>(derived_class);
	if(derived_class_tmp->isBinded())
	{
		//LOG(LOG_ERROR, "Class already binded to a tag. Not binding:"<<s<< " class:"<<derived_class_tmp->getQualifiedClassName());
		return nullptr;
	}
	return derived_class_tmp;
}


bool ApplicationDomain::needsCaseInsensitiveNames()
{
	return this->version <= 6 || (parentDomain  && parentDomain != getSystemState()->systemDomain && parentDomain->needsCaseInsensitiveNames());
}

void ApplicationDomain::setOrigin(const tiny_string& u, const tiny_string& filename)
{
	//We can use this origin to implement security measures.
	//Note that for plugins, this url is NOT the page url, but it is the swf file url.
	origin = URLInfo(u);
	//If this URL doesn't contain a filename, add the one passed as an argument (used in main.cpp)
	if(origin.getPathFile() == "" && filename != "")
	{
		tiny_string fileurl;
		if (g_path_is_absolute(filename.raw_buf()))
		{
			gchar* uri = g_filename_to_uri(filename.raw_buf(), nullptr,nullptr);
			fileurl = uri;
			g_free(uri);
		}
		else
			fileurl = filename;
		origin = origin.goToURL(fileurl);
	}
}

void ApplicationDomain::setFrameSize(const lightspark::RECT& f)
{
	frameSize=f;
}

RECT ApplicationDomain::getFrameSize() const
{
	return frameSize;
}

void ApplicationDomain::setFrameRate(float f)
{
	if (frameRate != f)
	{
		frameRate=f;
		if (this == getSystemState()->mainClip->applicationDomain.getPtr() )
			getSystemState()->setRenderRate(frameRate);
	}
}

float ApplicationDomain::getFrameRate() const
{
	return frameRate;
}

void ApplicationDomain::setBaseURL(const tiny_string& url)
{
	//Set the URL to be used in resolving relative paths. For the
	//plugin this is either the value of base attribute in the
	//OBJECT or EMBED tag or, if the attribute is not provided,
	//the address of the hosting HTML page.
	baseURL = URLInfo(url);
}
const URLInfo& ApplicationDomain::getBaseURL()
{
	//The plugin uses the address of the HTML page (baseURL) for
	//resolving relative paths. AIR and the standalone Lightspark
	//use the SWF location (origin).
	if(baseURL.isValid())
		return baseURL;
	else
		return getOrigin();
}

tiny_string ApplicationDomain::findClassAlias(Class_base* cls)
{
	auto aliasIt=aliasMap.begin();
	const auto aliasEnd=aliasMap.end();
	//Linear search for alias
	tiny_string alias;
	for(;aliasIt!=aliasEnd;++aliasIt)
	{
		if(aliasIt->second==cls)
		{
			return aliasIt->first;
		}
	}
	return "";
}
ASFUNCTIONBODY_ATOM(ApplicationDomain,_constructor)
{
	ApplicationDomain* th = asAtomHandler::as<ApplicationDomain>(obj);
	_NR<ApplicationDomain> parentDomain;
	ARG_CHECK(ARG_UNPACK (parentDomain, NullRef));
	if(!th->parentDomain.isNull())
		// Don't override parentDomain if it was set in the
		// C++ constructor
		return;
	else if(parentDomain.isNull())
		th->parentDomain = _MR(wrk->getSystemState()->systemDomain);
	else
		th->parentDomain = parentDomain;
}

ASFUNCTIONBODY_ATOM(ApplicationDomain,_getMinDomainMemoryLength)
{
	asAtomHandler::setUInt(ret,wrk,(uint32_t)MIN_DOMAIN_MEMORY_LIMIT);
}

ASFUNCTIONBODY_ATOM(ApplicationDomain,_getCurrentDomain)
{
	ApplicationDomain* res= nullptr;
	if (wrk->currentCallContext)
		res=ABCVm::getCurrentApplicationDomain(wrk->currentCallContext);
	else
		res = wrk->rootClip->applicationDomain.getPtr();
	res->incRef();
	ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(ApplicationDomain,hasDefinition)
{
	ApplicationDomain* th = asAtomHandler::as<ApplicationDomain>(obj);
	assert(argslen==1);
	const tiny_string& tmp=asAtomHandler::toString(args[0],wrk);

	multiname name(nullptr);
	name.name_type=multiname::NAME_STRING;

	tiny_string nsName;
	tiny_string tmpName;
	stringToQName(tmp,tmpName,nsName);
	name.name_s_id=wrk->getSystemState()->getUniqueStringId(tmpName);
	if (nsName != "")
		name.ns.push_back(nsNameAndKind(wrk->getSystemState(),nsName,NAMESPACE));

	LOG(LOG_CALLS,"Looking for definition of " << name);
	ASObject* target;
	asAtom o=asAtomHandler::invalidAtom;
	ret = asAtomHandler::invalidAtom;
	th->getVariableAndTargetByMultinameIncludeTemplatedClasses(o,name,target,wrk);
	if(asAtomHandler::isInvalid(o))
		asAtomHandler::setBool(ret,false);
	else
	{
		LOG(LOG_CALLS,"Found definition for " << name<<"|"<<asAtomHandler::toDebugString(o)<<"|"<<asAtomHandler::toDebugString(obj));
		asAtomHandler::setBool(ret,true);
	}
}

ASFUNCTIONBODY_ATOM(ApplicationDomain,getDefinition)
{
	ApplicationDomain* th = asAtomHandler::as<ApplicationDomain>(obj);
	assert(argslen==1);
	const tiny_string& tmp=asAtomHandler::toString(args[0],wrk);

	multiname name(nullptr);
	name.name_type=multiname::NAME_STRING;

	tiny_string nsName;
	tiny_string tmpName;
	stringToQName(tmp,tmpName,nsName);
	name.name_s_id=wrk->getSystemState()->getUniqueStringId(tmpName);
	name.hasEmptyNS=nsName == "";
	name.ns.push_back(nsNameAndKind(wrk->getSystemState(),nsName,NAMESPACE));

	LOG(LOG_CALLS,"Looking for definition of " << name);
	ret = asAtomHandler::invalidAtom;
	ASObject* target;
	th->getVariableAndTargetByMultinameIncludeTemplatedClasses(ret,name,target,wrk);
	if(asAtomHandler::isInvalid(ret))
	{
		ret = asAtomHandler::undefinedAtom;
		createError<ReferenceError>(wrk,kUndefinedVarError,name.normalizedNameUnresolved(wrk->getSystemState()));
		return;
	}

	//TODO: specs says that also namespaces and function may be returned
	//assert_and_throw(o->getObjectType()==T_CLASS);

	LOG(LOG_CALLS,"Getting definition for " << name<<" "<<asAtomHandler::toDebugString(ret));
	ASATOM_INCREF(ret);
}
ASFUNCTIONBODY_ATOM(ApplicationDomain,getQualifiedDefinitionNames)
{
	ApplicationDomain* th = asAtomHandler::as<ApplicationDomain>(obj);
	asAtom v=asAtomHandler::invalidAtom;
	Template<Vector>::getInstanceS(wrk,v,Class<ASString>::getClass(wrk->getSystemState()),th);
	Vector *result = asAtomHandler::as<Vector>(v);
	for(uint32_t j=0;j<th->globalScopes.size();j++)
	{
		auto vars = th->globalScopes[j]->getVariables();
		for (auto it = vars->Variables.cbegin(); it != vars->Variables.cend(); ++it)
		{
			if ((*it).second.ns.kind == PRIVATE_NAMESPACE || (*it).second.ns.nsNameId == BUILTIN_STRINGS::STRING_AS3VECTOR)
				continue;
			multiname m(nullptr);
			m.name_type=multiname::NAME_STRING;
			m.name_s_id=(*it).first;
			m.ns.push_back((*it).second.ns);
			tiny_string s = m.qualifiedString(wrk->getSystemState(),true);
			asAtom a = asAtomHandler::fromString(wrk->getSystemState(),s);
			result->append(a);
		}
	}
	ret = asAtomHandler::fromObject(result);
}
void ApplicationDomain::registerGlobalScope(Global* scope)
{
	globalScopes.push_back(scope);
}

ASObject* ApplicationDomain::getVariableByString(const std::string& str, ASObject*& target)
{
	size_t index=str.rfind('.');
	multiname name(nullptr);
	name.name_type=multiname::NAME_STRING;
	if(index==str.npos) //No dot
	{
		name.name_s_id=getSystemState()->getUniqueStringId(str);
		name.ns.push_back(nsNameAndKind(getSystemState(),"",NAMESPACE)); //TODO: use ns kind
	}
	else
	{
		name.name_s_id=getSystemState()->getUniqueStringId(str.substr(index+1));
		name.ns.push_back(nsNameAndKind(getSystemState(),str.substr(0,index),NAMESPACE));
		name.hasEmptyNS=false;
	}
	asAtom ret=asAtomHandler::invalidAtom;
	getVariableAndTargetByMultiname(ret,name, target,getInstanceWorker());
	return asAtomHandler::toObject(ret,getInstanceWorker());
}

bool ApplicationDomain::findTargetByMultiname(const multiname& name, ASObject*& target, ASWorker* wrk)
{
	//Check in the parent first
	if(!parentDomain.isNull())
	{
		bool ret=parentDomain->findTargetByMultiname(name, target,wrk);
		if(ret)
			return true;
	}

	for(uint32_t i=0;i<globalScopes.size();i++)
	{
		bool ret=globalScopes[i]->hasPropertyByMultiname(name,true,true,wrk);
		if(ret)
		{
			target=globalScopes[i];
			return true;
		}
	}
	return false;
}


GET_VARIABLE_RESULT ApplicationDomain::getVariableAndTargetByMultiname(asAtom& ret, const multiname& name, ASObject*& target, ASWorker* wrk)
{
	GET_VARIABLE_RESULT res = GET_VARIABLE_RESULT::GETVAR_NORMAL;
	for(uint32_t i=0;i<globalScopes.size();i++)
	{
		res = globalScopes[i]->getVariableByMultiname(ret,name,NO_INCREF,wrk);
		if(asAtomHandler::isValid(ret))
		{
			target=globalScopes[i];
			// No incRef, return a reference borrowed from globalScopes
			return res;
		}
	}
	auto it = classesBeingDefined.find(&name);
	if (it != classesBeingDefined.end())
	{
		target =((*it).second);
		return res;
	}
	if(!parentDomain.isNull())
	{
		res = parentDomain->getVariableAndTargetByMultiname(ret,name, target,wrk);
		if(asAtomHandler::isValid(ret))
			return res;
	}
	return res;
}
void ApplicationDomain::getVariableAndTargetByMultinameIncludeTemplatedClasses(asAtom& ret, const multiname& name, ASObject*& target, ASWorker* wrk)
{
	getVariableAndTargetByMultiname(ret,name, target,wrk);
	if (asAtomHandler::isValid(ret))
		return;
	if (name.ns.size() >= 1 && name.ns[0].nsNameId == BUILTIN_STRINGS::STRING_AS3VECTOR)
	{
		tiny_string s = getSystemState()->getStringFromUniqueId(name.name_s_id);
		if (s.startsWith("Vector.<"))
		{
			tiny_string vtype = s.substr_bytes(8,s.numBytes()-9);

			tiny_string tname;
			tiny_string tns;
			stringToQName(vtype,tname,tns);
			multiname tn(nullptr);
			tn.name_type=multiname::NAME_STRING;
			tn.name_s_id=getSystemState()->getUniqueStringId(tname);
			tn.ns.push_back(nsNameAndKind(getSystemState(),tns,NAMESPACE));
			tn.hasEmptyNS=tns.empty();
			ASObject* tntarget;
			asAtom typeobj=asAtomHandler::invalidAtom;
			getVariableAndTargetByMultiname(typeobj,tn, tntarget,wrk);
			if (asAtomHandler::isValid(typeobj))
			{
				Type* t = asAtomHandler::getObject(typeobj)->as<Type>();
				ret = asAtomHandler::fromObject(Template<Vector>::getTemplateInstance(t,this).getPtr());
			}
		}
	}
}
ASObject* ApplicationDomain::getVariableByMultinameOpportunistic(const multiname& name,ASWorker* wrk)
{
	auto it = classesBeingDefined.find(&name);
	if (it != classesBeingDefined.end())
	{
		return ((*it).second);
	}
	for(uint32_t i=0;i<globalScopes.size();i++)
	{
		asAtom o=asAtomHandler::invalidAtom;
		globalScopes[i]->getVariableByMultinameOpportunistic(o,name,wrk);
		if(asAtomHandler::isValid(o))
		{
			// No incRef, return a reference borrowed from globalScopes
			return asAtomHandler::getObject(o);
		}
	}
	if(!parentDomain.isNull())
	{
		ASObject* ret=parentDomain->getVariableByMultinameOpportunistic(name,wrk);
		if(ret)
			return ret;
	}
	return nullptr;
}

void ApplicationDomain::throwRangeError()
{
	createError<RangeError>(getWorker(),kInvalidRangeError);
}

void ApplicationDomain::checkDomainMemory()
{
	if(domainMemory.isNull())
	{
		domainMemory = defaultDomainMemory;
		domainMemory->setLength(MIN_DOMAIN_MEMORY_LIMIT);
	}
	currentDomainMemory=domainMemory.getPtr();
}

