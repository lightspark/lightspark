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

#include "scripting/avmplus/avmplus.h"
#include "scripting/flash/utils/ByteArray.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include "abc.h"

using namespace lightspark;

avmplusFile::avmplusFile(Class_base* c):
	ASObject(c)
{
}

void avmplusFile::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED);
	c->setDeclaredMethodByQName("exists","",Class<IFunction>::getFunction(c->getSystemState(),exists),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("read","",Class<IFunction>::getFunction(c->getSystemState(),read),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("write","",Class<IFunction>::getFunction(c->getSystemState(),write),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("readByteArray","",Class<IFunction>::getFunction(c->getSystemState(),readByteArray),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("writeByteArray","",Class<IFunction>::getFunction(c->getSystemState(),writeByteArray),NORMAL_METHOD,false);
}

ASFUNCTIONBODY_ATOM(avmplusFile,exists)
{
	tiny_string filename;
	ARG_UNPACK_ATOM(filename);
	return asAtom(sys->getEngineData()->FileExists(filename));
}
ASFUNCTIONBODY_ATOM(avmplusFile,read)
{
	tiny_string filename;
	ARG_UNPACK_ATOM(filename);
	if (!sys->getEngineData()->FileExists(filename))
		throwError<ASError>(kFileOpenError,filename);
	return asAtom::fromObject(abstract_s(sys,sys->getEngineData()->FileRead(filename)));
}
ASFUNCTIONBODY_ATOM(avmplusFile,write)
{
	tiny_string filename;
	tiny_string data;
	ARG_UNPACK_ATOM(filename)(data);
	sys->getEngineData()->FileWrite(filename,data);
	return asAtom::invalidAtom;
}
ASFUNCTIONBODY_ATOM(avmplusFile,readByteArray)
{
	tiny_string filename;
	ARG_UNPACK_ATOM(filename);
	if (!sys->getEngineData()->FileExists(filename))
		throwError<ASError>(kFileOpenError,filename);
	ByteArray* res = Class<ByteArray>::getInstanceS(sys);
	sys->getEngineData()->FileReadByteArray(filename,res);
	return asAtom::fromObject(res);
}
ASFUNCTIONBODY_ATOM(avmplusFile,writeByteArray)
{
	tiny_string filename;
	_NR<ByteArray> data;
	ARG_UNPACK_ATOM(filename)(data);
	sys->getEngineData()->FileWriteByteArray(filename,data.getPtr());
	return asAtom::invalidAtom;
}

avmplusSystem::avmplusSystem(Class_base* c):
	ASObject(c)
{
}

void avmplusSystem::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED);
	c->setDeclaredMethodByQName("getFeatures","",Class<IFunction>::getFunction(c->getSystemState(),getFeatures),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("queueCollection","",Class<IFunction>::getFunction(c->getSystemState(),queueCollection),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("forceFullCollection","",Class<IFunction>::getFunction(c->getSystemState(),forceFullCollection),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("getAvmplusVersion","",Class<IFunction>::getFunction(c->getSystemState(),getAvmplusVersion),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("pauseForGCIfCollectionImminent","",Class<IFunction>::getFunction(c->getSystemState(),pauseForGCIfCollectionImminent),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("getRunmode","",Class<IFunction>::getFunction(c->getSystemState(),getRunmode),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("isDebugger","",Class<IFunction>::getFunction(c->getSystemState(),isDebugger),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("isGlobal","",Class<IFunction>::getFunction(c->getSystemState(),isGlobal),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("freeMemory","",Class<IFunction>::getFunction(c->getSystemState(),_freeMemory),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("totalMemory","",Class<IFunction>::getFunction(c->getSystemState(),_totalMemory),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("privateMemory","",Class<IFunction>::getFunction(c->getSystemState(),_privateMemory),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("swfVersion","",Class<IFunction>::getFunction(c->getSystemState(),_swfVersion),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("argv","",Class<IFunction>::getFunction(c->getSystemState(),argv),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("exec","",Class<IFunction>::getFunction(c->getSystemState(),exec),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("write","",Class<IFunction>::getFunction(c->getSystemState(),write),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("exit","",Class<IFunction>::getFunction(c->getSystemState(),exit),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("trace","",Class<IFunction>::getFunction(c->getSystemState(),lightspark::trace),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("canonicalizeNumber","",Class<IFunction>::getFunction(c->getSystemState(),canonicalizeNumber),NORMAL_METHOD,false);
}

ASFUNCTIONBODY_ATOM(avmplusSystem,getFeatures)
{
	LOG(LOG_NOT_IMPLEMENTED, _("avmplus.System.getFeatures is unimplemented."));
	return asAtom::fromStringID(BUILTIN_STRINGS::EMPTY);
}
ASFUNCTIONBODY_ATOM(avmplusSystem,getRunmode)
{
	return asAtom::fromString(sys,sys->useJit ? "jit":"interpreted");
}

ASFUNCTIONBODY_ATOM(avmplusSystem,queueCollection)
{
	LOG(LOG_NOT_IMPLEMENTED, _("avmplus.System.queueCollection is unimplemented."));
	return asAtom::invalidAtom;
}

ASFUNCTIONBODY_ATOM(avmplusSystem,forceFullCollection)
{
	LOG(LOG_NOT_IMPLEMENTED, _("avmplus.System.forceFullCollection is unimplemented."));
	return asAtom::invalidAtom;
}

ASFUNCTIONBODY_ATOM(avmplusSystem,getAvmplusVersion)
{
	LOG(LOG_NOT_IMPLEMENTED, _("avmplus.System.getAvmplusVersion is unimplemented."));
	return asAtom::fromString(sys,"0");
}
ASFUNCTIONBODY_ATOM(avmplusSystem,pauseForGCIfCollectionImminent)
{
	LOG(LOG_NOT_IMPLEMENTED, _("avmplus.System.pauseForGCIfCollectionImminent is unimplemented."));
	return asAtom::invalidAtom;
}

ASFUNCTIONBODY_ATOM(avmplusSystem,isDebugger)
{
	LOG(LOG_NOT_IMPLEMENTED, _("avmplus.System.isDebugger is unimplemented."));
	return asAtom::falseAtom;
}
ASFUNCTIONBODY_ATOM(avmplusSystem,isGlobal)
{
	LOG(LOG_NOT_IMPLEMENTED, _("avmplus.System.isDebugger is unimplemented."));
	return asAtom::falseAtom;
}
ASFUNCTIONBODY_ATOM(avmplusSystem,_freeMemory)
{
	LOG(LOG_NOT_IMPLEMENTED, _("avmplus.System.freeMemory is unimplemented."));
	return asAtom(1024);
}
ASFUNCTIONBODY_ATOM(avmplusSystem,_totalMemory)
{
	LOG(LOG_NOT_IMPLEMENTED, _("avmplus.System.totalMemory is unimplemented."));
	return asAtom(1024);
}
ASFUNCTIONBODY_ATOM(avmplusSystem,_privateMemory)
{
	LOG(LOG_NOT_IMPLEMENTED, _("avmplus.System.privateMemory is unimplemented."));
	return asAtom(1024);
}
ASFUNCTIONBODY_ATOM(avmplusSystem,_swfVersion)
{
	return asAtom(sys->getSwfVersion());
}

ASFUNCTIONBODY_ATOM(avmplusSystem,argv)
{
	LOG(LOG_NOT_IMPLEMENTED, _("avmplus.System.argv is unimplemented."));
	return asAtom::fromObject(Class<Array>::getInstanceS(sys));
}
ASFUNCTIONBODY_ATOM(avmplusSystem,exec)
{
	LOG(LOG_NOT_IMPLEMENTED, _("avmplus.System.exec is unimplemented."));
	return asAtom::invalidAtom;
}
ASFUNCTIONBODY_ATOM(avmplusSystem,write)
{
	LOG(LOG_NOT_IMPLEMENTED, _("avmplus.System.write is unimplemented."));
	return asAtom::invalidAtom;
}
ASFUNCTIONBODY_ATOM(avmplusSystem,exit)
{
	LOG(LOG_NOT_IMPLEMENTED, _("avmplus.System.exit is unimplemented."));
	return asAtom::invalidAtom;
}
ASFUNCTIONBODY_ATOM(avmplusSystem,canonicalizeNumber)
{
	_NR<ASObject> o;
	ARG_UNPACK_ATOM(o);
	switch(o->getObjectType())
	{
		case T_NUMBER:
		case T_INTEGER:
		case T_BOOLEAN:
		case T_UINTEGER:
		case T_NULL:
		case T_UNDEFINED:
			return asAtom(o->toNumber());
		case T_QNAME:
		case T_NAMESPACE:
			return asAtom(Number::NaN);
		default:
			o->incRef();
			return asAtom::fromObject(o.getPtr());
	}
}

avmplusDomain::avmplusDomain(Class_base* c):
	ASObject(c)
{
}

void avmplusDomain::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject,_constructor, CLASS_SEALED);
	c->setDeclaredMethodByQName("currentDomain","",Class<IFunction>::getFunction(c->getSystemState(),_getCurrentDomain),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("MIN_DOMAIN_MEMORY_LENGTH","",Class<IFunction>::getFunction(c->getSystemState(),_getMinDomainMemoryLength),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("load","",Class<IFunction>::getFunction(c->getSystemState(),load),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("loadBytes","",Class<IFunction>::getFunction(c->getSystemState(),loadBytes),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getClass","",Class<IFunction>::getFunction(c->getSystemState(),getClass),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("domainMemory","",Class<IFunction>::getFunction(c->getSystemState(),_getDomainMemory),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("domainMemory","",Class<IFunction>::getFunction(c->getSystemState(),_setDomainMemory),SETTER_METHOD,true);
}
ASFUNCTIONBODY_ATOM(avmplusDomain,_constructor)
{
	_NR<avmplusDomain> parentDomain;
	ARG_UNPACK_ATOM(parentDomain);
	avmplusDomain* th = obj.as<avmplusDomain>();
	if (parentDomain.isNull())
		th->appdomain = ABCVm::getCurrentApplicationDomain(getVm(sys)->currentCallContext);
	else
		th->appdomain = _NR<ApplicationDomain>(Class<ApplicationDomain>::getInstanceS(sys,parentDomain->appdomain));
	return asAtom::invalidAtom;
}

ASFUNCTIONBODY_ATOM(avmplusDomain,_getCurrentDomain)
{
	avmplusDomain* ret = Class<avmplusDomain>::getInstanceSNoArgs(sys);
	ret->appdomain = ABCVm::getCurrentApplicationDomain(getVm(sys)->currentCallContext);
	return asAtom::fromObject(ret);
}
ASFUNCTIONBODY_ATOM(avmplusDomain,_getMinDomainMemoryLength)
{
	return asAtom(MIN_DOMAIN_MEMORY_LIMIT);
}
ASFUNCTIONBODY_ATOM(avmplusDomain,load)
{
	avmplusDomain* th = obj.as<avmplusDomain>();
	tiny_string filename;
	uint32_t swfVersion;
	ARG_UNPACK_ATOM(filename)(swfVersion, 0);
	if (swfVersion != 0)
		LOG(LOG_NOT_IMPLEMENTED, "avmplus.Domain.load is unimplemented for swfVersion "<<swfVersion);
	if (!sys->getEngineData()->FileExists(filename))
		throwError<ASError>(kFileOpenError,filename);
	_NR<ByteArray> bytes = _NR<ByteArray>(Class<ByteArray>::getInstanceS(sys));
	sys->getEngineData()->FileReadByteArray(filename,bytes.getPtr());

	// execute loaded abc file
	MemoryStreamCache mc(sys);
	mc.append(bytes->getBuffer(bytes->getLength(),false),bytes->getLength());
	std::streambuf *sbuf = mc.createReader();
	std::istream s(sbuf);
	RootMovieClip* root=getVm(sys)->currentCallContext->context->root.getPtr();
	_NR<ApplicationDomain> origdomain = root->applicationDomain;
	root->applicationDomain = th->appdomain;
	root->incRef();
	ABCContext* context = new ABCContext(_MR(root), s, getVm(root->getSystemState()));
	context->exec(false);
	root->applicationDomain = origdomain;
	delete sbuf;
	return asAtom::invalidAtom;
}
ASFUNCTIONBODY_ATOM(avmplusDomain,loadBytes)
{
	avmplusDomain* th = obj.as<avmplusDomain>();
	_NR<ByteArray> bytes;
	uint32_t swfversion;
	ARG_UNPACK_ATOM (bytes)(swfversion, 0);

	if (swfversion != 0)
		LOG(LOG_NOT_IMPLEMENTED,"Domain.loadBytes ignores parameter swfVersion");
	MemoryStreamCache mc(sys);
	mc.append(bytes->getBuffer(bytes->getLength(),false),bytes->getLength());
	std::streambuf *sbuf = mc.createReader();
	std::istream s(sbuf);
	
	// execute loaded abc bytes
	RootMovieClip* root=getVm(sys)->currentCallContext->context->root.getPtr();
	_NR<ApplicationDomain> origdomain = root->applicationDomain;
	root->applicationDomain = th->appdomain;
	root->incRef();
	ABCContext* context = new ABCContext(_MR(root), s, getVm(root->getSystemState()));
	context->exec(false);
	root->applicationDomain = origdomain;
	delete sbuf;
	return asAtom::invalidAtom;
}
ASFUNCTIONBODY_ATOM(avmplusDomain,getClass)
{
	return getDefinitionByName(sys,obj,args,argslen);
}
ASFUNCTIONBODY_ATOM(avmplusDomain,_getDomainMemory)
{
	avmplusDomain* th = obj.as<avmplusDomain>();
	if (th->appdomain->domainMemory.isNull())
		return asAtom::invalidAtom;
	th->appdomain->domainMemory->incRef();
	return asAtom::fromObject(th->appdomain->domainMemory.getPtr());
}
ASFUNCTIONBODY_ATOM(avmplusDomain,_setDomainMemory)
{
	_NR<ByteArray> b;
	ARG_UNPACK_ATOM(b);
	avmplusDomain* th = obj.as<avmplusDomain>();
	if (b.isNull())
	{
		th->appdomain->domainMemory = b;
		return asAtom::invalidAtom;
	}
		
	if (b->getLength() < MIN_DOMAIN_MEMORY_LIMIT)
		throwError<RangeError>(kEndOfFileError);
	th->appdomain->domainMemory = b;
	return asAtom::invalidAtom;
}
