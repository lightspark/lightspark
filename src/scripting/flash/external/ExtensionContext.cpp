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
#include "abc.h"
#include "class.h"
#include "argconv.h"
#include "SDL.h"
#include "3rdparty/pugixml/src/pugixml.hpp"
#include "scripting/toplevel/Array.h"
#include "scripting/toplevel/Vector.h"
#include "scripting/flash/events/StatusEvent.h"
#include "utils/filesystem.h"

using namespace lightspark;

struct FlashRuntimeExtension
{
	tiny_string librarypath;
	tiny_string initializername;
	tiny_string finalizername;
	FREContextInitializer contextinitializer;
	FREContextFinalizer contextfinalizer;
	uint32_t numFunctionsToSet;
	const FRENamedFunction* functionsToSet;
	void* nativeExtData;
	void* library;
};
unordered_map<uint32_t, FlashRuntimeExtension> extensions;

// class to handle conversion of asAtoms from/to FREObjects
class asAtomFREObjectInterface : public FREObjectInterface
{
public:
	asAtomFREObjectInterface()
	{
	}
	FREResult toBool(FREObject object, uint32_t *value) override
	{
		ASWorker* wrk = getWorker();
		if (!wrk || wrk->nativeExtensionCallCount==0)
			return FRE_WRONG_THREAD;
		*value = asAtomHandler::Boolean_concrete(*(asAtom*)object);
		LOG(LOG_CALLS,"nativeExtension:toBool:"<<asAtomHandler::toDebugString(*(asAtom*)object));
		return FRE_OK;
	}
	FREResult toUInt32(FREObject object, uint32_t *value) override
	{
		ASWorker* wrk = getWorker();
		if (!wrk || wrk->nativeExtensionCallCount==0)
			return FRE_WRONG_THREAD;
		*value = asAtomHandler::toUInt(*(asAtom*)object);
		LOG(LOG_CALLS,"nativeExtension:toUInt32:"<<asAtomHandler::toDebugString(*(asAtom*)object));
		return FRE_OK;
	}
	FREResult toUTF8(FREObject object, uint32_t* length, const uint8_t** value) override
	{
		ASWorker* wrk = getWorker();
		if (!wrk || wrk->nativeExtensionCallCount==0)
			return FRE_WRONG_THREAD;
		tiny_string s = asAtomHandler::toString(*(asAtom*)object,wrk);
		*length = s.numBytes()+1;
		uint8_t* buf = new uint8_t[*length];
		memcpy(buf,s.raw_buf(),(*length)-1);
		buf[(*length)-1]=0;
		*value=buf;
		LOG(LOG_CALLS,"nativeExtension:toUTF8:"<<asAtomHandler::toDebugString(*(asAtom*)object));
		wrk->nativeExtensionStringlist.push_back(buf);
		return FRE_OK;
	}
	FREResult fromBool(uint32_t value, FREObject* object) override
	{
		ASWorker* wrk = getWorker();
		if (!wrk)
			return FRE_WRONG_THREAD;
		*object = value ? &asAtomHandler::trueAtom : &asAtomHandler::falseAtom;
		LOG(LOG_CALLS,"nativeExtension:fromBool:"<<value);
		return FRE_OK;
	}
	FREResult fromDouble(double value, FREObject* object) override
	{
		ASWorker* wrk = getWorker();
		if (!wrk || wrk->nativeExtensionCallCount==0)
			return FRE_WRONG_THREAD;
		wrk->nativeExtensionAtomlist.push_back(asAtomHandler::fromNumber(wrk,value,false));
		*object = &wrk->nativeExtensionAtomlist.back();
		LOG(LOG_CALLS,"nativeExtension:fromDouble:"<<value);
		return FRE_OK;
	}
	FREResult fromInt32(int32_t value, FREObject* object) override
	{
		ASWorker* wrk = getWorker();
		if (!wrk || wrk->nativeExtensionCallCount==0)
			return FRE_WRONG_THREAD;
		wrk->nativeExtensionAtomlist.push_back(asAtomHandler::fromInt(value));
		*object = &wrk->nativeExtensionAtomlist.back();
		LOG(LOG_CALLS,"nativeExtension:fromInt32:"<<value);
		return FRE_OK;
	}
	FREResult fromUint32(uint32_t value, FREObject* object) override
	{
		ASWorker* wrk = getWorker();
		if (!wrk || wrk->nativeExtensionCallCount==0)
			return FRE_WRONG_THREAD;
		wrk->nativeExtensionAtomlist.push_back(asAtomHandler::fromUInt(value));
		*object = &wrk->nativeExtensionAtomlist.back();
		LOG(LOG_CALLS,"nativeExtension:fromUint32:"<<value);
		return FRE_OK;
	}
	FREResult fromUTF8(uint32_t length, const uint8_t* value, FREObject* object) override
	{
		ASWorker* wrk = getWorker();
		if (!wrk || wrk->nativeExtensionCallCount==0)
			return FRE_WRONG_THREAD;
		ASObject* res = abstract_s(wrk, (const char*)value, length-1);
		wrk->nativeExtensionAtomlist.push_back(asAtomHandler::fromObject(res));
		*object = &wrk->nativeExtensionAtomlist.back();
		LOG(LOG_CALLS,"nativeExtension:fromUTF8:"<<res->toDebugString());
		return FRE_OK;
	}
	FREResult NewObject(const uint8_t* className, uint32_t argc, FREObject argv[], FREObject* object, FREObject* thrownException) override
	{
		ASWorker* wrk = getWorker();
		if (!wrk || wrk->nativeExtensionCallCount==0)
			return FRE_WRONG_THREAD;
		LOG(LOG_CALLS,"nativeExtension:NewObject:"<<className);
		tiny_string strcls((const char*)className,true);
		tiny_string nsName;
		tiny_string tmpName;
		stringToQName(strcls,tmpName,nsName);
		multiname name(nullptr);
		name.name_type=multiname::NAME_STRING;
		name.name_s_id=wrk->getSystemState()->getUniqueStringId(tmpName);
		name.hasEmptyNS=nsName == "";
		name.ns.push_back(nsNameAndKind(wrk->getSystemState(),nsName,NAMESPACE));
		asAtom ret= asAtomHandler::invalidAtom;
		ASObject* target=nullptr;
		if (nsName.empty() || nsName.startsWith("flash."))
			wrk->getSystemState()->systemDomain->getVariableAndTargetByMultinameIncludeTemplatedClasses(ret,name,target,wrk);
		if(asAtomHandler::isInvalid(ret))
			ABCVm::getCurrentApplicationDomain(wrk->currentCallContext)->getVariableAndTargetByMultinameIncludeTemplatedClasses(ret,name,target,wrk);

		if(asAtomHandler::isInvalid(ret))
		{
			LOG(LOG_CALLS,"nativeExtension:NewObject class not found:"<<className);
			return FRE_NO_SUCH_NAME;
		}
		assert(asAtomHandler::isClass(ret));
		asAtom res= asAtomHandler::invalidAtom;
		asAtomHandler::as<Class_base>(ret)->getInstance(wrk,res,true,(asAtom*)argv,argc);
		if(wrk->currentCallContext->exceptionthrown)
		{
			LOG(LOG_CALLS,"nativeExtension:NewObject exception thrown:"<<className);
			wrk->nativeExtensionAtomlist.push_back(asAtomHandler::fromObject(wrk->currentCallContext->exceptionthrown));
			*thrownException = &wrk->nativeExtensionAtomlist.back();
			return FRE_ACTIONSCRIPT_ERROR;
		}
		wrk->nativeExtensionAtomlist.push_back(res);
		*object = &wrk->nativeExtensionAtomlist.back();
		LOG(LOG_CALLS,"nativeExtension:NewObject created:"<<asAtomHandler::toDebugString(res));
		return FRE_OK;
	}
	FREResult SetArrayElementAt(FREObject arrayOrVector, uint32_t index, FREObject value) override
	{
		ASWorker* wrk = getWorker();
		if (!wrk || wrk->nativeExtensionCallCount==0)
			return FRE_WRONG_THREAD;
		if (arrayOrVector==nullptr || value==nullptr)
			return FRE_INVALID_OBJECT;
		
		asAtom o = *(asAtom*)arrayOrVector;
		if (asAtomHandler::isInvalid(o))
			return FRE_INVALID_OBJECT;
		
		ASObject* obj = asAtomHandler::toObject(o,wrk);
		asAtom v = *(asAtom*)value;
		LOG(LOG_CALLS,"nativeExtension:setArrayElementAt:"<<index<<" "<<obj->toDebugString()<<" "<<asAtomHandler::toDebugString(v));
		if (obj->is<Array>())
		{
			obj->as<Array>()->set(index,v);
		}
		else if (obj->is<Vector>())
		{
			ASATOM_INCREF(v);
			obj->as<Vector>()->set(index,v);
		}
		else
			return FRE_TYPE_MISMATCH;
		return FRE_OK;
	}	
	FREResult SetArrayLength(FREObject arrayOrVector, uint32_t length) override
	{
		ASWorker* wrk = getWorker();
		if (!wrk || wrk->nativeExtensionCallCount==0)
			return FRE_WRONG_THREAD;
		if (arrayOrVector==nullptr)
			return FRE_INVALID_OBJECT;
			
		asAtom o = *(asAtom*)arrayOrVector;
		if (asAtomHandler::isInvalid(o))
			return FRE_INVALID_OBJECT;
		
		ASObject* obj = asAtomHandler::toObject(o,wrk);
		LOG(LOG_CALLS,"nativeExtension:setArrayLength:"<<length<<" "<<obj->toDebugString());
		if (obj->is<Array>())
			obj->as<Array>()->resize(length);
		else if (obj->is<Vector>())
		{
			if (obj->as<Vector>()->isFixed())
				return FRE_READ_ONLY;
			obj->as<Vector>()->ensureLength(length);
		}
		else
			return FRE_TYPE_MISMATCH;
		return FRE_OK;
	}
	FREResult SetObjectProperty(FREObject object, const uint8_t* propertyName, FREObject propertyValue, FREObject* thrownException) override
	{
		ASWorker* wrk = getWorker();
		if (!wrk || wrk->nativeExtensionCallCount==0)
			return FRE_WRONG_THREAD;
		if (propertyName==nullptr)
			return FRE_INVALID_ARGUMENT;
		asAtom o = *(asAtom*)object;
		ASObject* obj = asAtomHandler::toObject(o,wrk);
		multiname m(nullptr);
		// TODO what about property names with namespaces?
		m.name_type= multiname::NAME_STRING;
		tiny_string s((const char*)propertyName);
		m.name_s_id= getSys()->getUniqueStringId(s);
		ASATOM_INCREF(*(asAtom*)propertyValue);
		obj->setVariableByMultiname(m,*(asAtom*)propertyValue,CONST_NOT_ALLOWED,nullptr,obj->getInstanceWorker());
		LOG(LOG_CALLS,"nativeExtension:setObjectProperty:"<<m<<" "<<obj->toDebugString()<<" "<<asAtomHandler::toDebugString(*(asAtom*)propertyValue));
		
		if (obj->getInstanceWorker()->currentCallContext && obj->getInstanceWorker()->currentCallContext->exceptionthrown)
		{
			if (thrownException)
			{
				obj->getInstanceWorker()->currentCallContext->exceptionthrown->incRef();
				asAtom exc = asAtomHandler::fromObjectNoPrimitive(obj->getInstanceWorker()->currentCallContext->exceptionthrown);
				wrk->nativeExtensionAtomlist.push_back(exc);
				*thrownException=&wrk->nativeExtensionAtomlist.back();
			}
			return FRE_ACTIONSCRIPT_ERROR;
		}
		return FRE_OK;
	}
	FREResult AcquireByteArray(FREObject object, FREByteArray* byteArrayToSet) override
	{
		ASWorker* wrk = getWorker();
		if (!wrk || wrk->nativeExtensionCallCount==0)
			return FRE_WRONG_THREAD;
		if (byteArrayToSet==nullptr)
			return FRE_INVALID_ARGUMENT;
		ASObject* obj = asAtomHandler::toObject(*(asAtom*)object,wrk);
		if (!obj->is<ByteArray>())
			return FRE_TYPE_MISMATCH;
		obj->as<ByteArray>()->lock();
		obj->incRef();
		byteArrayToSet->bytes = obj->as<ByteArray>()->getBufferNoCheck();
		byteArrayToSet->length = obj->as<ByteArray>()->getLength();
		LOG(LOG_CALLS,"nativeExtension:AcquireByteArray:"<<obj->toDebugString()<<" "<<byteArrayToSet->length);
		return FRE_OK;
	}
	FREResult ReleaseByteArray (FREObject object) override
	{
		ASWorker* wrk = getWorker();
		if (!wrk || wrk->nativeExtensionCallCount==0)
			return FRE_WRONG_THREAD;
		if (!asAtomHandler::is<ByteArray>(*(asAtom*)object))
			return FRE_TYPE_MISMATCH;
		ASObject* obj = asAtomHandler::toObject(*(asAtom*)object,wrk);
		obj->as<ByteArray>()->unlock();
		obj->decRef();
		LOG(LOG_CALLS,"nativeExtension:ReleaseByteArray:"<<obj->toDebugString());
		return FRE_OK;
	}
	FREResult DispatchStatusEventAsync(FREContext ctx, const uint8_t* code, const uint8_t* level ) override
	{
		if (code==nullptr)
			return FRE_INVALID_ARGUMENT;
		if (level==nullptr)
			return FRE_INVALID_ARGUMENT;
		ExtensionContext* ctxt =(ExtensionContext*)ctx;
		if (ctxt==nullptr)
			return FRE_INVALID_ARGUMENT;
		ctxt->incRef();
		tiny_string c((const char*)code,true);
		tiny_string l((const char*)level,true);
		asAtom statuscode = asAtomHandler::fromString(ctxt->getSystemState(),c);
		getVm(ctxt->getSystemState())->addEvent(_MR(ctxt), _MR(Class<StatusEvent>::getInstanceS(ctxt->getInstanceWorker(),l,statuscode)));
		LOG(LOG_CALLS,"nativeExtension:DispatchStatusEventAsync:"<<ctxt->toDebugString()<<" "<<c<<" "<<l);
		return FRE_OK;
	}
};
asAtomFREObjectInterface freobjectinterface;

ExtensionContext::ExtensionContext(ASWorker* wrk, Class_base* c) : EventDispatcher(wrk,c),
	nativelibrary(nullptr)
{
	subtype = SUBTYPE_EXTENSIONCONTEXT;
}
void ExtensionContext::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, EventDispatcher, CLASS_SEALED|CLASS_FINAL);
	c->setDeclaredMethodByQName("createExtensionContext","",c->getSystemState()->getBuiltinFunction(createExtensionContext,2,Class<ExtensionContext>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("call","",c->getSystemState()->getBuiltinFunction(_call,1,Class<ASObject>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("dispose","",c->getSystemState()->getBuiltinFunction(dispose,0),NORMAL_METHOD,true);
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
void ExtensionContext::prepareShutdown()
{
}
void ExtensionContext::finalizeExtensionContext()
{
	this->contextfinalizer(this);
	this->nativelibrary=nullptr;
}

ASFUNCTIONBODY_ATOM(ExtensionContext,createExtensionContext)
{
	ExtensionContext* ctxt = Class<ExtensionContext>::getInstanceSNoArgs(wrk);
	ARG_CHECK(ARG_UNPACK(ctxt->extensionID)(ctxt->contextType));
	auto it = extensions.find(wrk->getSystemState()->getUniqueStringId(ctxt->extensionID));
	if (it != extensions.end())
	{
		if (!(*it).second.library)
		{
			(*it).second.library = SDL_LoadObject((*it).second.librarypath.raw_buf());
			if (!(*it).second.library)
			{
				LOG(LOG_ERROR,"loading native library failed:"<<SDL_GetError()<<" "<<(*it).second.librarypath);
				ret = asAtomHandler::undefinedAtom;
				return;
			}
			FREObjectInterface::registerFREObjectInterface(&freobjectinterface);
			FREInitializer func = (FREInitializer)SDL_LoadFunction((*it).second.library,(*it).second.initializername.raw_buf());
			func(&((*it).second.nativeExtData),&((*it).second.contextinitializer),&((*it).second.contextfinalizer));
		}
		ctxt->nativelibrary=(*it).second.library;
		ctxt->contextinitializer=(*it).second.contextinitializer;
		ctxt->contextfinalizer=(*it).second.contextfinalizer;
		ctxt->nativeExtData=(*it).second.nativeExtData;
		ctxt->contextinitializer(&((*it).second.nativeExtData),(const uint8_t*)ctxt->contextType.raw_buf(),ctxt,&ctxt->numFunctionsToSet,&ctxt->functionsToSet);
	}
	ret = asAtomHandler::fromObjectNoPrimitive(ctxt);
}

ASFUNCTIONBODY_ATOM(ExtensionContext,_call)
{
	ExtensionContext* th = asAtomHandler::as<ExtensionContext>(obj);
	tiny_string methodname;
	ARG_CHECK(ARG_UNPACK_MORE_ALLOWED(methodname));
	FREObject* freargs;
	if (argslen == 1)
		freargs=nullptr;
	else
	{
		freargs = (FREObject*)malloc(sizeof(FREObject)*(argslen-1));
		for (uint32_t i = 1; i < argslen; i++)
		{
			ASATOM_INCREF(args[i]);
			wrk->nativeExtensionAtomlist.push_back(args[i]);
			freargs[i-1]=&wrk->nativeExtensionAtomlist.back();
			
			LOG(LOG_CALLS,"nativeExtension call arg:"<<i<<" "<<asAtomHandler::toDebugString(args[i])<<" "<<hex<<((asAtom*)freargs[i-1])->uintval);
		}
	}
	wrk->nativeExtensionCallCount++;
	ret = asAtomHandler::undefinedAtom;
	for (uint32_t i =0; i < th->numFunctionsToSet; i++)
	{
		if (methodname == std::string((const char*)th->functionsToSet[i].name))
		{
			FREFunction func = th->functionsToSet[i].function;
			ret = *(asAtom*)func(th,th->functionsToSet[i].functionData,argslen-1,freargs);
			ASATOM_INCREF(ret);
		}
	}
	wrk->nativeExtensionCallCount--;
	if (freargs)
		free(freargs);
	if (wrk->nativeExtensionCallCount==0)
	{
		// this was the main call to a native extension function, so we clean up all created values
		for (auto it = wrk->nativeExtensionAtomlist.begin(); it != wrk->nativeExtensionAtomlist.end(); it++)
		{
			ASATOM_DECREF(*it);
		}
		wrk->nativeExtensionAtomlist.clear();
		for (auto it = wrk->nativeExtensionStringlist.begin(); it != wrk->nativeExtensionStringlist.end(); it++)
		{
			delete[] (*it);
		}
		wrk->nativeExtensionStringlist.clear();
	}
	LOG(LOG_CALLS,"nativeExtension call done:"<<methodname<<" "<<asAtomHandler::toDebugString(args[0])<<" "<<asAtomHandler::toDebugString(ret));
}

ASFUNCTIONBODY_ATOM(ExtensionContext,dispose)
{
	ExtensionContext* th = asAtomHandler::as<ExtensionContext>(obj);
	th->finalizeExtensionContext();
}

void ExtensionContext::registerExtension(const tiny_string& filepath)
{
	Path p(filepath);
	Path extdir = p.getDir() / "META-INF" / "ANE";
	Path extxml = extdir / "extension.xml";
	pugi::xml_document doc;
	pugi::xml_parse_result res = doc.load_file(extxml.rawBuf());
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
				tiny_string nativelibraryname = (*it).child("applicationDeployment").child("nativeLibrary").first_child().value();
				fre.initializername = (*it).child("applicationDeployment").child("initializer").first_child().value();
				fre.finalizername = (*it).child("applicationDeployment").child("finalizer").first_child().value();
				if (nativelibraryname.empty() || fre.initializername.empty())
					LOG(LOG_ERROR,"native library not found:"<<filepath);
				else
				{
					Path lib = extdir / platformname / nativelibraryname;
					fre.librarypath=lib.rawBuf();
					uint32_t extid = getSys()->getUniqueStringId(extensionid);
					extensions.insert(make_pair(extid,fre));
				}
				break;
			}
			it++;
		}
	}
}
