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

#include "scripting/argconv.h"
#include "scripting/toplevel/RegExp.h"

using namespace std;
using namespace lightspark;

RegExp::RegExp(ASWorker* wrk, Class_base* c):ASObject(wrk,c,T_OBJECT,SUBTYPE_REGEXP),dotall(false),global(false),ignoreCase(false),
	extended(false),multiline(false),lastIndex(0)
{
}

RegExp::RegExp(ASWorker* wrk,Class_base* c, const tiny_string& _re):ASObject(wrk,c,T_OBJECT,SUBTYPE_REGEXP),dotall(false),global(false),ignoreCase(false),
	extended(false),multiline(false),lastIndex(0),source(_re)
{
}

void RegExp::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_DYNAMIC_NOT_FINAL);
	c->setDeclaredMethodByQName("exec","",Class<IFunction>::getFunction(c->getSystemState(),exec),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("exec",AS3,Class<IFunction>::getFunction(c->getSystemState(),exec),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("test","",Class<IFunction>::getFunction(c->getSystemState(),test),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("test",AS3,Class<IFunction>::getFunction(c->getSystemState(),test),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("toString","",Class<IFunction>::getFunction(c->getSystemState(),_toString),NORMAL_METHOD,true);
	c->prototype->setVariableByQName("toString","",Class<IFunction>::getFunction(c->getSystemState(),_toString),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("exec","",Class<IFunction>::getFunction(c->getSystemState(),exec),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("exec",AS3,Class<IFunction>::getFunction(c->getSystemState(),exec),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("test","",Class<IFunction>::getFunction(c->getSystemState(),test),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("test",AS3,Class<IFunction>::getFunction(c->getSystemState(),test),CONSTANT_TRAIT);
	REGISTER_GETTER(c,dotall);
	REGISTER_GETTER(c,global);
	REGISTER_GETTER(c,ignoreCase);
	REGISTER_GETTER(c,extended);
	REGISTER_GETTER(c,multiline);
	REGISTER_GETTER_SETTER(c,lastIndex);
	REGISTER_GETTER(c,source);
}

void RegExp::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY_ATOM(RegExp,_constructor)
{
	RegExp* th=asAtomHandler::as<RegExp>(obj);
	if(argslen > 0 && asAtomHandler::is<RegExp>(args[0]))
	{
		if(argslen > 1 && !asAtomHandler::is<Undefined>(args[1]))
		{
			createError<TypeError>(wrk,kRegExpFlagsArgumentError);
			return;
		}
		RegExp *src=asAtomHandler::as<RegExp>(args[0]);
		th->source=src->source;
		th->dotall=src->dotall;
		th->global=src->global;
		th->ignoreCase=src->ignoreCase;
		th->extended=src->extended;
		th->multiline=src->multiline;
		return;
	}
	else if(argslen > 0)
		th->source=asAtomHandler::toString(args[0],wrk).raw_buf();
	if(argslen>1 && !asAtomHandler::is<Undefined>(args[1]))
	{
		const tiny_string& flags=asAtomHandler::toString(args[1],wrk);
		for(auto i=flags.begin();i!=flags.end();++i)
		{
			switch(*i)
			{
				case 'g':
					th->global=true;
					break;
				case 'i':
					th->ignoreCase=true;
					break;
				case 'x':
					th->extended=true;
					break;
				case 'm':
					th->multiline=true;
					break;
				case 's':
					// Defined in the Adobe online
					// help but not in ECMA
					th->dotall=true;
					break;
				default:
					break;
			}
		}
	}
}


ASFUNCTIONBODY_ATOM(RegExp,generator)
{
	if(argslen == 0)
	{
		ret = asAtomHandler::fromObject(Class<RegExp>::getInstanceS(wrk,""));
	}
	else if(asAtomHandler::is<RegExp>(args[0]))
	{
		ASATOM_INCREF(args[0]);
		ret = args[0];
	}
	else
	{
		if (argslen > 1)
			LOG(LOG_NOT_IMPLEMENTED, "RegExp generator: flags argument not implemented");
		ret = asAtomHandler::fromObject(Class<RegExp>::getInstanceS(wrk,asAtomHandler::toString(args[0],wrk)));
	}
}

ASFUNCTIONBODY_GETTER(RegExp, dotall)
ASFUNCTIONBODY_GETTER(RegExp, global)
ASFUNCTIONBODY_GETTER(RegExp, ignoreCase)
ASFUNCTIONBODY_GETTER(RegExp, extended)
ASFUNCTIONBODY_GETTER(RegExp, multiline)
ASFUNCTIONBODY_GETTER_SETTER(RegExp, lastIndex)
ASFUNCTIONBODY_GETTER(RegExp, source)

ASFUNCTIONBODY_ATOM(RegExp,exec)
{
	RegExp* th=static_cast<RegExp*>(asAtomHandler::getObject(obj));
	assert_and_throw(argslen==1);
	const tiny_string& arg0=asAtomHandler::toString(args[0],wrk);
	ret = asAtomHandler::fromObject(th->match(arg0));
}

ASObject *RegExp::match(const tiny_string& str)
{
	pcre* pcreRE = compile(!str.isSinglebyte());
	if (!pcreRE)
		return getSystemState()->getNullRef();
	int capturingGroups;
	int infoOk=pcre_fullinfo(pcreRE, nullptr, PCRE_INFO_CAPTURECOUNT, &capturingGroups);
	if(infoOk!=0)
	{
		pcre_free(pcreRE);
		return getSystemState()->getNullRef();
	}
	//Get information about named capturing groups
	int namedGroups;
	infoOk=pcre_fullinfo(pcreRE, nullptr, PCRE_INFO_NAMECOUNT, &namedGroups);
	if(infoOk!=0)
	{
		pcre_free(pcreRE);
		return getSystemState()->getNullRef();
	}
	//Get information about the size of named entries
	int namedSize;
	infoOk=pcre_fullinfo(pcreRE, nullptr, PCRE_INFO_NAMEENTRYSIZE, &namedSize);
	if(infoOk!=0)
	{
		pcre_free(pcreRE);
		return getSystemState()->getNullRef();
	}
	struct nameEntry
	{
		uint16_t number;
		char name[0];
	};
	char* entries;
	infoOk=pcre_fullinfo(pcreRE, nullptr, PCRE_INFO_NAMETABLE, &entries);
	if(infoOk!=0)
	{
		pcre_free(pcreRE);
		lastIndex=0;
		return getSystemState()->getNullRef();
	}
	pcre_extra extra;
	extra.match_limit_recursion=500;
	extra.flags = PCRE_EXTRA_MATCH_LIMIT_RECURSION;
	int ovector[(capturingGroups+1)*3];
	int offset=global?lastIndex:0;
	if(offset<0)
	{
		//beyond last match
		pcre_free(pcreRE);
		lastIndex=0;
		return getSystemState()->getNullRef();
	}
	int rc=pcre_exec(pcreRE,capturingGroups > 500 ? &extra : nullptr, str.raw_buf(), str.numBytes(), offset, PCRE_NO_UTF8_CHECK, ovector, (capturingGroups+1)*3);
	if(rc<0)
	{
		//No matches or error
		pcre_free(pcreRE);
		lastIndex=0;
		return getSystemState()->getNullRef();
	}
	Array* a=Class<Array>::getInstanceSNoArgs(getInstanceWorker());
	//Push the whole result and the captured strings
	for(int i=0;i<capturingGroups+1;i++)
	{
		if(ovector[i*2] >= 0)
			a->push(asAtomHandler::fromObject(abstract_s(getInstanceWorker(), str.substr_bytes(ovector[i*2],ovector[i*2+1]-ovector[i*2]) )));
		else
			a->push(asAtomHandler::fromObject(getSystemState()->getUndefinedRef()));
	}
	a->setVariableByQName("input","",abstract_s(getInstanceWorker(),str),DYNAMIC_TRAIT);

	// pcre_exec returns byte position, so we have to convert it to character position 
	tiny_string tmp = str.substr_bytes(0, ovector[0]);
	int index = tmp.numChars();

	a->setVariableAtomByQName("index",nsNameAndKind(),asAtomHandler::fromInt(index),DYNAMIC_TRAIT);
	for(int i=0;i<namedGroups;i++)
	{
		nameEntry* entry=reinterpret_cast<nameEntry*>(entries);
		uint16_t num=GINT16_FROM_BE(entry->number);
		asAtom captured=a->at(num);
		ASATOM_INCREF(captured);
		a->setVariableAtomByQName(getSystemState()->getUniqueStringId(tiny_string(entry->name, true)),nsNameAndKind(BUILTIN_NAMESPACES::EMPTY_NS),captured,DYNAMIC_TRAIT);
		entries+=namedSize;
	}
	lastIndex=ovector[1];
	pcre_free(pcreRE);
	return a;
}

ASFUNCTIONBODY_ATOM(RegExp,test)
{
	if (!asAtomHandler::is<RegExp>(obj))
	{
		asAtomHandler::setBool(ret,true);
		return;
	}
	RegExp* th=asAtomHandler::as<RegExp>(obj);

	const tiny_string& arg0 = asAtomHandler::toString(args[0],wrk);
	if (wrk->currentCallContext->exceptionthrown)
		return;
	pcre* pcreRE = th->compile(!arg0.isSinglebyte());
	if (!pcreRE)
	{
		asAtomHandler::setNull(ret);
		return;
	}
	int capturingGroups;
	int infoOk=pcre_fullinfo(pcreRE, NULL, PCRE_INFO_CAPTURECOUNT, &capturingGroups);
	if(infoOk!=0)
	{
		pcre_free(pcreRE);
		asAtomHandler::setNull(ret);
		return;
	}
	int ovector[(capturingGroups+1)*3];
	
	int offset=(th->global)?th->lastIndex:0;
	pcre_extra extra;
	extra.match_limit_recursion=200;
	extra.flags = PCRE_EXTRA_MATCH_LIMIT_RECURSION;
	int rc = pcre_exec(pcreRE, &extra, arg0.raw_buf(), arg0.numBytes(), offset, PCRE_NO_UTF8_CHECK, ovector, (capturingGroups+1)*3);
	bool res = (rc >= 0);
	pcre_free(pcreRE);
	asAtomHandler::setBool(ret,res);
}

ASFUNCTIONBODY_ATOM(RegExp,_toString)
{
	if(Class<RegExp>::getClass(wrk->getSystemState())->prototype->getObj() == asAtomHandler::getObject(obj))
	{
		ret = asAtomHandler::fromString(wrk->getSystemState(),"/(?:)/");
		return;
	}
	if(!asAtomHandler::is<RegExp>(obj))
	{
		createError<TypeError>(wrk,0,"RegExp.toString is not generic");
		return;
	}

	RegExp* th=asAtomHandler::as<RegExp>(obj);
	tiny_string res;
	res = "/";
	res += th->source;
	res += "/";
	if(th->global)
		res += "g";
	if(th->ignoreCase)
		res += "i";
	if(th->multiline)
		res += "m";
	if(th->dotall)
		res += "s";
	ret = asAtomHandler::fromObject(abstract_s(wrk,res));
}

pcre* RegExp::compile(bool isutf8)
{
	int options = PCRE_NEWLINE_ANY | PCRE_NO_UTF8_CHECK;
	if(isutf8)
		options |= PCRE_UTF8;
	if(ignoreCase)
		options |= PCRE_CASELESS;
	if(extended)
		options |= PCRE_EXTENDED;
	if(multiline)
		options |= PCRE_MULTILINE;
	if(dotall)
		options|=PCRE_DOTALL;

	const char * error;
	int errorOffset;
	int errorcode;
	pcre* pcreRE=pcre_compile2(source.raw_buf(), options,&errorcode,  &error, &errorOffset,nullptr);
	if(error)
	{
//		if (errorcode == 64) // invalid pattern in javascript compatibility mode (we try again in normal mode to match flash behaviour)
//		{
//			options &= ~PCRE_JAVASCRIPT_COMPAT;
//			pcreRE=pcre_compile2(source.raw_buf(), options,&errorcode,  &error, &errorOffset,NULL);
//		}
		if (error)
			return nullptr;
	}
	return pcreRE;
}
