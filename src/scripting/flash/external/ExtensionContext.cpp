/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2011-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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

#include "ExtensionContext.h"
#include "class.h"
#include "argconv.h"
#include "SDL.h"
#include "3rdparty/pugixml/src/pugixml.hpp"

using namespace lightspark;

struct FlashRuntimeExtension
{
	tiny_string librarypath;
	tiny_string initializername;
	tiny_string finalizername;
	FREContextInitializer contextinitializer;
	FREContextFinalizer contextfinalizer;
	void* library;
};
unordered_map<uint32_t, FlashRuntimeExtension> extensions;

ExtensionContext::ExtensionContext(ASWorker* wrk, Class_base* c) : EventDispatcher(wrk,c),
	nativelibrary(nullptr),nativeExtData(nullptr)
{
	subtype = SUBTYPE_EXTENSIONCONTEXT;
}
void ExtensionContext::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, EventDispatcher, CLASS_SEALED|CLASS_FINAL);
	c->setDeclaredMethodByQName("createExtensionContext","",Class<IFunction>::getFunction(c->getSystemState(),createExtensionContext,2,Class<ExtensionContext>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("call","",Class<IFunction>::getFunction(c->getSystemState(),_call,1,Class<ASObject>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("dispose","",Class<IFunction>::getFunction(c->getSystemState(),dispose,0),NORMAL_METHOD,true);
}
void ExtensionContext::finalize()
{
	finalizeExtensionContext();
	EventDispatcher::finalize();
}
bool ExtensionContext::destruct()
{
	finalizeExtensionContext();
	return EventDispatcher::destruct();
}
void ExtensionContext::finalizeExtensionContext()
{
	auto it = extensions.find(getSystemState()->getUniqueStringId(extensionID));
	if (it != extensions.end())
	{
		(*it).second.contextfinalizer(this);
		nativelibrary=nullptr;
		nativeExtData=nullptr;
	}	
}

ASFUNCTIONBODY_ATOM(ExtensionContext,createExtensionContext)
{
	ExtensionContext* ctxt = Class<ExtensionContext>::getInstanceSNoArgs(wrk);
	ARG_CHECK(ARG_UNPACK(ctxt->extensionID)(ctxt->contextType));
	auto it = extensions.find(wrk->getSystemState()->getUniqueStringId(ctxt->extensionID));
	if (it != extensions.end())
	{
		if (!(*it).second.library)
			(*it).second.library = SDL_LoadObject((*it).second.librarypath.raw_buf());
		if (!(*it).second.library)
		{
			LOG(LOG_ERROR,"loading native library failed:"<<SDL_GetError()<<" "<<(*it).second.librarypath);
			ret = asAtomHandler::undefinedAtom;
			return;
		}
		
		ctxt->nativelibrary=(*it).second.library;
		FREInitializer func = (FREInitializer)SDL_LoadFunction(ctxt->nativelibrary,(*it).second.initializername.raw_buf());
		func(&ctxt->nativeExtData,&(*it).second.contextinitializer,&(*it).second.contextfinalizer);
		
		(*it).second.contextinitializer(&ctxt->nativeExtData,(const uint8_t*)ctxt->contextType.raw_buf(),ctxt,&ctxt->numFunctionsToSet,&ctxt->functionsToSet);
	}	
	ret = asAtomHandler::fromObjectNoPrimitive(ctxt);
}
ASFUNCTIONBODY_ATOM(ExtensionContext,_call)
{
	ExtensionContext* th = asAtomHandler::as<ExtensionContext>(obj);
	tiny_string methodname;
	ARG_CHECK(ARG_UNPACK_MORE_ALLOWED(methodname));
	for (uint32_t i =0; i < th->numFunctionsToSet; i++)
	{
		if (methodname == th->functionsToSet[i].name)
		{
			FREFunction func = th->functionsToSet[i].function;
			FREObject freres = nullptr;
			if (argslen == 1) // no arguments for method
			{
				freres = func(th,th->functionsToSet[i].functionData,0,nullptr);
			}
			else
			{
				LOG(LOG_NOT_IMPLEMENTED,"ExtensionContext.call with arguments:"<<methodname<<" "<<argslen);
				ret = asAtomHandler::undefinedAtom;
				return;
			}
			
#ifdef LIGHTSPARK_64
			uint64_t result=(uint64_t)freres;
#else
			uint32_t result=(uint32_t)freres;
#endif
			// it seems that adobe uses the avmplus atom definition (see https://github.com/adobe/avmplus/blob/master/core/atom.h)
			// as format for arguments/results of native function calls
			switch (result&0x7)
			{
				case 5: // bool
					ret = result == 5 ? asAtomHandler::falseAtom : asAtomHandler::trueAtom;
					break;
				case 6: // uint
					ret = asAtomHandler::fromUInt(result>>3);
					break;
				default:
					LOG(LOG_NOT_IMPLEMENTED,"result type for native extension return value:"<<result);
					break;
			}
			break;
		}
	}
}

ASFUNCTIONBODY_ATOM(ExtensionContext,dispose)
{
	ExtensionContext* th = asAtomHandler::as<ExtensionContext>(obj);
	th->finalizeExtensionContext();
}

void ExtensionContext::registerExtension(const tiny_string& filepath)
{
	tiny_string extdir(g_path_get_dirname(filepath.raw_buf()));
	extdir+=G_DIR_SEPARATOR_S;
	extdir+="META-INF";
	extdir+=G_DIR_SEPARATOR_S;
	extdir+="ANE";
	extdir+=G_DIR_SEPARATOR_S;
	tiny_string extxml=extdir;
	extxml+="extension.xml";
	pugi::xml_document doc;
	pugi::xml_parse_result res = doc.load_file(extxml.raw_buf());
	if (res.status != pugi::status_ok)
		LOG(LOG_ERROR,"failed to parse extension xml file:"<<res.status<<" "<<extxml);
	else
	{
		pugi::xml_node n = doc.root().child("extension");
		tiny_string extensionid = n.child("id").first_child().value();
		n = n.child("platforms");
#ifdef _WIN32
#ifdef LIGHTSPARK_64
		tiny_string platformname = "Windows-x86-64";
#else
		tiny_string platformname = "Windows-x86";
#endif
#else
		tiny_string platformname = "Linux-x86-64";
#endif
		auto it = n.children().begin();
		while (it != n.children().end())
		{
			if (platformname == (*it).attribute("name").value())
			{
				FlashRuntimeExtension fre;
				fre.library=nullptr;
				fre.contextinitializer=nullptr;
				fre.contextfinalizer=nullptr;
				tiny_string nativelibraryname = (*it).child("applicationDeployment").child("nativeLibrary").first_child().value();
				fre.initializername = (*it).child("applicationDeployment").child("initializer").first_child().value();
				fre.finalizername = (*it).child("applicationDeployment").child("finalizer").first_child().value();
				if (nativelibraryname.empty() || fre.initializername.empty())
					LOG(LOG_ERROR,"native library not found:"<<filepath);
				else
				{
					tiny_string lib = extdir+platformname;
					lib += G_DIR_SEPARATOR_S;
					lib += nativelibraryname;
					fre.librarypath=lib;
					uint32_t extid = getSys()->getUniqueStringId(extensionid);
					extensions.insert(make_pair(extid,fre));
				}
				break;
			}
			it++;
		}
	}
}
