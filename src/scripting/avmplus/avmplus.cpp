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

using namespace lightspark;

avmplusFile::avmplusFile(Class_base* c):
	ASObject(c)
{
}

void avmplusFile::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED);
	c->setDeclaredMethodByQName("exists","",Class<IFunction>::getFunction(exists),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("read","",Class<IFunction>::getFunction(read),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("write","",Class<IFunction>::getFunction(write),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("readByteArray","",Class<IFunction>::getFunction(readByteArray),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("writeByteArray","",Class<IFunction>::getFunction(writeByteArray),NORMAL_METHOD,false);
}

ASFUNCTIONBODY(avmplusFile,exists)
{
	LOG(LOG_NOT_IMPLEMENTED, _("avmplus.File.exists is unimplemented."));
	return NULL;
}
ASFUNCTIONBODY(avmplusFile,read)
{
	LOG(LOG_NOT_IMPLEMENTED, _("avmplus.File.read is unimplemented."));
	return NULL;
}
ASFUNCTIONBODY(avmplusFile,write)
{
	LOG(LOG_NOT_IMPLEMENTED, _("avmplus.File.write is unimplemented."));
	return NULL;
}
ASFUNCTIONBODY(avmplusFile,readByteArray)
{
	avmplusFile* th=static_cast<avmplusFile*>(obj);
	tiny_string filename;
	ARG_UNPACK(filename);
	
	LOG(LOG_NOT_IMPLEMENTED, _("avmplus.File.readByteArray is unimplemented."));
	return Class<ByteArray>::getInstanceS();
}
ASFUNCTIONBODY(avmplusFile,writeByteArray)
{
	LOG(LOG_NOT_IMPLEMENTED, _("avmplus.File.writeByteArray is unimplemented."));
	return NULL;
}

avmplusSystem::avmplusSystem(Class_base* c):
	ASObject(c)
{
}

void avmplusSystem::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED);
	c->setDeclaredMethodByQName("getFeatures","",Class<IFunction>::getFunction(getFeatures),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("queueCollection","",Class<IFunction>::getFunction(queueCollection),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("forceFullCollection","",Class<IFunction>::getFunction(forceFullCollection),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("getAvmplusVersion","",Class<IFunction>::getFunction(getAvmplusVersion),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("pauseForGCIfCollectionImminent","",Class<IFunction>::getFunction(pauseForGCIfCollectionImminent),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("getRunmode","",Class<IFunction>::getFunction(getRunmode),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("isDebugger","",Class<IFunction>::getFunction(isDebugger),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("isGlobal","",Class<IFunction>::getFunction(isGlobal),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("freeMemory","",Class<IFunction>::getFunction(_freeMemory),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("totalMemory","",Class<IFunction>::getFunction(_totalMemory),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("privateMemory","",Class<IFunction>::getFunction(_privateMemory),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("argv","",Class<IFunction>::getFunction(argv),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("exec","",Class<IFunction>::getFunction(exec),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("write","",Class<IFunction>::getFunction(write),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("exit","",Class<IFunction>::getFunction(exit),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("trace","",Class<IFunction>::getFunction(lightspark::trace),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("canonicalizeNumber","",Class<IFunction>::getFunction(canonicalizeNumber),NORMAL_METHOD,false);
}

ASFUNCTIONBODY(avmplusSystem,getFeatures)
{
	LOG(LOG_NOT_IMPLEMENTED, _("avmplus.System.getFeatures is unimplemented."));
	return Class<ASString>::getInstanceS("");
}
ASFUNCTIONBODY(avmplusSystem,getRunmode)
{
	LOG(LOG_NOT_IMPLEMENTED, _("avmplus.System.getRunmode is unimplemented."));
	return Class<ASString>::getInstanceS("jit");
}

ASFUNCTIONBODY(avmplusSystem,queueCollection)
{
	LOG(LOG_NOT_IMPLEMENTED, _("avmplus.System.queueCollection is unimplemented."));
	return NULL;
}

ASFUNCTIONBODY(avmplusSystem,forceFullCollection)
{
	LOG(LOG_NOT_IMPLEMENTED, _("avmplus.System.forceFullCollection is unimplemented."));
	return NULL;
}

ASFUNCTIONBODY(avmplusSystem,getAvmplusVersion)
{
	LOG(LOG_NOT_IMPLEMENTED, _("avmplus.System.getAvmplusVersion is unimplemented."));
	return Class<ASString>::getInstanceS("0");
}
ASFUNCTIONBODY(avmplusSystem,pauseForGCIfCollectionImminent)
{
	LOG(LOG_NOT_IMPLEMENTED, _("avmplus.System.pauseForGCIfCollectionImminent is unimplemented."));
	return NULL;
}

ASFUNCTIONBODY(avmplusSystem,isDebugger)
{
	LOG(LOG_NOT_IMPLEMENTED, _("avmplus.System.isDebugger is unimplemented."));
	return abstract_b(false);
}
ASFUNCTIONBODY(avmplusSystem,isGlobal)
{
	LOG(LOG_NOT_IMPLEMENTED, _("avmplus.System.isDebugger is unimplemented."));
	return abstract_b(false);
}
ASFUNCTIONBODY(avmplusSystem,_freeMemory)
{
	LOG(LOG_NOT_IMPLEMENTED, _("avmplus.System.freeMemory is unimplemented."));
	return abstract_d(1024);
}
ASFUNCTIONBODY(avmplusSystem,_totalMemory)
{
	LOG(LOG_NOT_IMPLEMENTED, _("avmplus.System.totalMemory is unimplemented."));
	return abstract_d(1024);
}
ASFUNCTIONBODY(avmplusSystem,_privateMemory)
{
	LOG(LOG_NOT_IMPLEMENTED, _("avmplus.System.privateMemory is unimplemented."));
	return abstract_d(1024);
}
ASFUNCTIONBODY(avmplusSystem,argv)
{
	LOG(LOG_NOT_IMPLEMENTED, _("avmplus.System.argv is unimplemented."));
	return Class<Array>::getInstanceS();
}
ASFUNCTIONBODY(avmplusSystem,exec)
{
	LOG(LOG_NOT_IMPLEMENTED, _("avmplus.System.exec is unimplemented."));
	return NULL;
}
ASFUNCTIONBODY(avmplusSystem,write)
{
	LOG(LOG_NOT_IMPLEMENTED, _("avmplus.System.write is unimplemented."));
	return NULL;
}
ASFUNCTIONBODY(avmplusSystem,exit)
{
	LOG(LOG_NOT_IMPLEMENTED, _("avmplus.System.exit is unimplemented."));
	return NULL;
}
ASFUNCTIONBODY(avmplusSystem,canonicalizeNumber)
{
	LOG(LOG_NOT_IMPLEMENTED, _("avmplus.System.canonicalizeNumber is unimplemented."));
	_NR<ASObject> o;
	ARG_UNPACK(o);
	o->incRef();
	return o.getPtr();
}

avmplusDomain::avmplusDomain(Class_base* c):
	ApplicationDomain(c)
{
	domainMemory->setLength(0);
}

void avmplusDomain::sinit(Class_base* c)
{
	CLASS_SETUP(c, ApplicationDomain,_constructor, CLASS_SEALED);
	c->setDeclaredMethodByQName("currentDomain","",Class<IFunction>::getFunction(_getCurrentDomain),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("MIN_DOMAIN_MEMORY_LENGTH","",Class<IFunction>::getFunction(_getMinDomainMemoryLength),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("load","",Class<IFunction>::getFunction(load),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("loadBytes","",Class<IFunction>::getFunction(load),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getClass","",Class<IFunction>::getFunction(getClass),NORMAL_METHOD,true);
}
ASFUNCTIONBODY(avmplusDomain,_constructor)
{
	LOG(LOG_NOT_IMPLEMENTED, _("avmplus.Domain constructor is unimplemented."));
	return NULL;
}

ASFUNCTIONBODY(avmplusDomain,load)
{
	LOG(LOG_NOT_IMPLEMENTED, _("avmplus.Domain.load is unimplemented."));
	throwError<VerifyError>(kIllegalOpcodeError,"","","");
	return NULL;
}
ASFUNCTIONBODY(avmplusDomain,loadBytes)
{
	LOG(LOG_NOT_IMPLEMENTED, _("avmplus.Domain.loadBytes is unimplemented."));
	return NULL;
}
ASFUNCTIONBODY(avmplusDomain,getClass)
{
	LOG(LOG_NOT_IMPLEMENTED, _("avmplus.Domain.getClass is unimplemented."));
	return Class<ASObject>::getRef()->getClass();
}
