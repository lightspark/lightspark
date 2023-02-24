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

#include "3rdparty/avmplus/pcre/pcre.h"

#include "scripting/toplevel/ASString.h"
#include "scripting/toplevel/Number.h"
#include "scripting/toplevel/Integer.h"
#include "scripting/flash/utils/ByteArray.h"
#include "compat.h"
#include "scripting/argconv.h"
#include "parsing/amf3_generator.h"
#include "scripting/toplevel/RegExp.h"

using namespace std;
using namespace lightspark;

ASString::ASString(ASWorker* wrk,Class_base* c):ASObject(wrk,c,T_STRING),hasId(true),datafilled(true)
{
	stringId = BUILTIN_STRINGS::EMPTY;
}

ASString::ASString(ASWorker* wrk,Class_base* c,const string& s) : ASObject(wrk,c,T_STRING),data(s),hasId(false),datafilled(true)
{
}

ASString::ASString(ASWorker* wrk,Class_base* c,const tiny_string& s) : ASObject(wrk,c,T_STRING),data(s),hasId(false),datafilled(true)
{
}

ASString::ASString(ASWorker* wrk,Class_base* c,const char* s) : ASObject(wrk,c,T_STRING),data(s, /*copy:*/true),hasId(false),datafilled(true)
{
}

ASString::ASString(ASWorker* wrk,Class_base* c,const char* s, uint32_t len) : ASObject(wrk,c,T_STRING)
{
	data = std::string(s,len);
	hasId = false;
	datafilled=true;
}

ASFUNCTIONBODY_ATOM(ASString,_constructor)
{
	ASString* th=asAtomHandler::as<ASString>(obj);
	if(args && argslen==1)
	{
		th->data=asAtomHandler::toString(args[0],wrk);
		th->hasId = false;
		th->stringId = UINT32_MAX;
		th->datafilled = true;
	}
}

ASFUNCTIONBODY_ATOM(ASString,_getLength)
{
	// fast path if obj is ASString
	if (asAtomHandler::isStringID(obj))
	{
		asAtomHandler::setInt(ret,wrk,int32_t(wrk->getSystemState()->getStringFromUniqueId(asAtomHandler::toStringId(obj,wrk)).numChars()));
	}
	else if (asAtomHandler::isString(obj))
	{
		ASString* th = asAtomHandler::getObjectNoCheck(obj)->as<ASString>();
		asAtomHandler::setInt(ret,wrk,int32_t(th->getData().numChars()));
	}
	else
	{
		asAtomHandler::setInt(ret,wrk,int32_t(tiny_string(asAtomHandler::toString(obj,wrk)).numChars()));
	}
}

void ASString::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_FINAL | CLASS_SEALED);
	c->isReusable = true;
	c->setDeclaredMethodByQName("split",AS3,Class<IFunction>::getFunction(c->getSystemState(),split,2,Class<Array>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("substr",AS3,Class<IFunction>::getFunction(c->getSystemState(),substr,2,Class<ASString>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("substring",AS3,Class<IFunction>::getFunction(c->getSystemState(),substring,2,Class<ASString>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("replace",AS3,Class<IFunction>::getFunction(c->getSystemState(),replace,2,Class<ASString>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("concat",AS3,Class<IFunction>::getFunction(c->getSystemState(),concat,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("match",AS3,Class<IFunction>::getFunction(c->getSystemState(),match,1,Class<Array>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("search",AS3,Class<IFunction>::getFunction(c->getSystemState(),search,1,Class<Integer>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("indexOf",AS3,Class<IFunction>::getFunction(c->getSystemState(),indexOf,2,Class<Integer>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("lastIndexOf",AS3,Class<IFunction>::getFunction(c->getSystemState(),lastIndexOf,2,Class<Integer>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("charCodeAt",AS3,Class<IFunction>::getFunction(c->getSystemState(),charCodeAt,0,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("charAt",AS3,Class<IFunction>::getFunction(c->getSystemState(),charAt,1,Class<ASString>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("slice",AS3,Class<IFunction>::getFunction(c->getSystemState(),slice,2,Class<ASString>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("toLocaleLowerCase",AS3,Class<IFunction>::getFunction(c->getSystemState(),toLowerCase,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("toLocaleUpperCase",AS3,Class<IFunction>::getFunction(c->getSystemState(),toUpperCase,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("toLowerCase",AS3,Class<IFunction>::getFunction(c->getSystemState(),toLowerCase,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("toUpperCase",AS3,Class<IFunction>::getFunction(c->getSystemState(),toUpperCase,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("localeCompare",AS3,Class<IFunction>::getFunction(c->getSystemState(),localeCompare,1,Class<Integer>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	// According to specs fromCharCode belongs to AS3 namespace,
	// but also empty namespace is seen in the wild and should be
	// supported.
	c->setDeclaredMethodByQName("fromCharCode",AS3,Class<IFunction>::getFunction(c->getSystemState(),fromCharCode,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("fromCharCode","",Class<IFunction>::getFunction(c->getSystemState(),fromCharCode,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("length","",Class<IFunction>::getFunction(c->getSystemState(),_getLength,0,Class<Integer>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("toString",AS3,Class<IFunction>::getFunction(c->getSystemState(),_toString,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("valueOf",AS3,Class<IFunction>::getFunction(c->getSystemState(),_toString,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);

	c->prototype->setVariableByQName("split","",Class<IFunction>::getFunction(c->getSystemState(),split,2,Class<Array>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("substr","",Class<IFunction>::getFunction(c->getSystemState(),substr,2,Class<ASString>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("substring","",Class<IFunction>::getFunction(c->getSystemState(),substring,2,Class<ASString>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("replace","",Class<IFunction>::getFunction(c->getSystemState(),replace,2,Class<ASString>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("concat","",Class<IFunction>::getFunction(c->getSystemState(),concat,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("match","",Class<IFunction>::getFunction(c->getSystemState(),match,1,Class<Array>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("search","",Class<IFunction>::getFunction(c->getSystemState(),search,1,Class<Integer>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("indexOf","",Class<IFunction>::getFunction(c->getSystemState(),indexOf,2,Class<Integer>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("lastIndexOf","",Class<IFunction>::getFunction(c->getSystemState(),lastIndexOf,2,Class<Integer>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("charCodeAt","",Class<IFunction>::getFunction(c->getSystemState(),charCodeAt,0,Class<Number>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("charAt","",Class<IFunction>::getFunction(c->getSystemState(),charAt,1,Class<ASString>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("slice","",Class<IFunction>::getFunction(c->getSystemState(),slice,2,Class<ASString>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("toLocaleLowerCase","",Class<IFunction>::getFunction(c->getSystemState(),toLowerCase,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("toLocaleUpperCase","",Class<IFunction>::getFunction(c->getSystemState(),toUpperCase,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("toLowerCase","",Class<IFunction>::getFunction(c->getSystemState(),toLowerCase,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("toUpperCase","",Class<IFunction>::getFunction(c->getSystemState(),toUpperCase,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("toString","",Class<IFunction>::getFunction(c->getSystemState(),_toString,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("valueOf","",Class<IFunction>::getFunction(c->getSystemState(),_toString,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("localeCompare","",Class<IFunction>::getFunction(c->getSystemState(),localeCompare_prototype,1,Class<Integer>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
}

ASFUNCTIONBODY_ATOM(ASString,search)
{
	tiny_string data = asAtomHandler::toString(obj,wrk);
	int res = -1;
	if(argslen == 0 || asAtomHandler::isUndefined(args[0]))
	{
		asAtomHandler::setInt(ret,wrk,res);
		return;
	}

	int options=PCRE_UTF8|PCRE_NEWLINE_ANY|PCRE_NO_UTF8_CHECK;//|PCRE_JAVASCRIPT_COMPAT;
	tiny_string restr;
	if(asAtomHandler::is<RegExp>(args[0]))
	{
		RegExp* re=asAtomHandler::as<RegExp>(args[0]);
		restr = re->source;
		if(re->ignoreCase)
			options|=PCRE_CASELESS;
		if(re->extended)
			options|=PCRE_EXTENDED;
		if(re->multiline)
			options|=PCRE_MULTILINE;
		if(re->dotall)
			options|=PCRE_DOTALL;
	}
	else
	{
		restr = asAtomHandler::toString(args[0],wrk);
	}

	const char* error;
	int errorOffset;
	pcre* pcreRE=pcre_compile(restr.raw_buf(), options, &error, &errorOffset,nullptr);
	if(error)
	{
		pcre_free(pcreRE);
		asAtomHandler::setInt(ret,wrk,res);
		return;
	}
	int capturingGroups;
	int infoOk=pcre_fullinfo(pcreRE, nullptr, PCRE_INFO_CAPTURECOUNT, &capturingGroups);
	if(infoOk!=0)
	{
		pcre_free(pcreRE);
		asAtomHandler::setInt(ret,wrk,res);
		return;
	}
	pcre_extra extra;
	extra.match_limit_recursion=500;
	extra.flags = PCRE_EXTRA_MATCH_LIMIT_RECURSION;
	int ovector[(capturingGroups+1)*3];
	int offset=0;
	//Global is not used in search
	int rc=pcre_exec(pcreRE, &extra, data.raw_buf(), data.numBytes(), offset, PCRE_NO_UTF8_CHECK, ovector, (capturingGroups+1)*3);
	if(rc<0)
	{
		//No matches or error
		pcre_free(pcreRE);
		asAtomHandler::setInt(ret,wrk,res);
		return;
	}
	pcre_free(pcreRE);
	res=ovector[0];
	// pcre_exec returns byte position, so we have to convert it to character position 
	tiny_string tmp = data.substr_bytes(0, res);
	res = tmp.numChars();
	asAtomHandler::setInt(ret,wrk,res);
}

ASFUNCTIONBODY_ATOM(ASString,match)
{
	tiny_string data = asAtomHandler::toString(obj,wrk);
	if(argslen == 0 || asAtomHandler::isNull(args[0]) || asAtomHandler::isUndefined(args[0]))
	{
		asAtomHandler::setNull(ret);
		return;
	}
	ASObject* res=nullptr;
	RegExp* re;

	if(asAtomHandler::is<RegExp>(args[0]))
	{
		re = asAtomHandler::as<RegExp>(args[0]);
		re->incRef();
	}
	else
	{
		re = Class<RegExp>::getInstanceS(wrk,asAtomHandler::toString(args[0],wrk));
	}

	if (re->global)
	{
		Array *resarr = Class<Array>::getInstanceSNoArgs(wrk);
		int prevLastIndex = 0;
		re->lastIndex = 0;

		while (true)
		{
			ASObject *match = re->match(data);

			if (match->is<Null>())
				break;

			if (re->lastIndex == prevLastIndex)
				// ECMA-262 Section 15.5.4.10 says
				// that we should increase
				// re->lastIndex by one and repeat,
				// but this is closer to the observed
				// behaviour.
				break;

			prevLastIndex = re->lastIndex;

			assert(match->is<Array>());
			asAtom a = match->as<Array>()->at(0);
			ASATOM_INCREF(a);
			resarr->push(a);
		}

		// According to ECMA we should return Null if resarr
		// is empty, but the tested behavior is to return the
		// empty array.
		res = resarr;
	}
	else
	{
		res = re->match(data);
	}

	re->decRef();

	ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(ASString,_toString)
{
	if(Class<ASString>::getClass(wrk->getSystemState())->prototype->getObj() == asAtomHandler::getObject(obj))
	{
		ret = asAtomHandler::fromStringID(BUILTIN_STRINGS::EMPTY);
		return;
	}
	if(!asAtomHandler::is<ASString>(obj))
	{
		LOG(LOG_ERROR,"String.toString is not generic:"<<asAtomHandler::toDebugString(obj));
		createError<TypeError>(wrk,0,"String.toString is not generic");
		return;
	}
	assert_and_throw(argslen==0);

	//As ASStrings are immutable, we can just return ourself
	ASATOM_INCREF(obj);
	ret = obj;
}

ASFUNCTIONBODY_ATOM(ASString,split)
{
	tiny_string data;
	asAtomHandler::getStringView(data,obj,wrk);
	Array* res=Class<Array>::getInstanceSNoArgs(wrk);
	uint32_t limit = 0x7fffffff;
	if(argslen == 0)
	{
		res->push(asAtomHandler::fromObject(abstract_s(wrk,data)));
		ret = asAtomHandler::fromObject(res);
		return;
	}
	if (argslen > 1 && !asAtomHandler::is<Undefined>(args[1]))
		limit = asAtomHandler::toUInt(args[1]);
	if (limit == 0)
	{
		ret = asAtomHandler::fromObject(res);
		return;
	}
	if(asAtomHandler::is<RegExp>(args[0]))
	{
		RegExp* re=asAtomHandler::as<RegExp>(args[0]);

		if(re->source.empty())
		{
			//the RegExp is empty, so split every character
			for(auto i=data.begin();i!=data.end();++i)
			{
				if (res->size() >= limit)
					break;
				res->push(asAtomHandler::fromObject(abstract_s(wrk, tiny_string::fromChar(*i) ) ));
			}
			ret = asAtomHandler::fromObject(res);
			return;
		}

		pcre* pcreRE = re->compile(!data.isSinglebyte());
		if (!pcreRE)
		{
			ret = asAtomHandler::fromObject(res);
			return;
		}
		int capturingGroups;
		int infoOk=pcre_fullinfo(pcreRE, nullptr, PCRE_INFO_CAPTURECOUNT, &capturingGroups);
		if(infoOk!=0)
		{
			pcre_free(pcreRE);
			ret = asAtomHandler::fromObject(res);
			return;
		}
		pcre_extra extra;
		extra.match_limit_recursion=200;
		extra.flags = PCRE_EXTRA_MATCH_LIMIT_RECURSION;
		int ovector[(capturingGroups+1)*3];
		int offset=0;
		unsigned int end;
		uint32_t lastMatch = 0;
		do
		{
			//offset is a byte offset that must point to the beginning of an utf8 character
			int rc=pcre_exec(pcreRE, &extra, data.raw_buf(), data.numBytes(), offset, PCRE_NO_UTF8_CHECK, ovector, (capturingGroups+1)*3);
			end=ovector[0];
			if(rc<0)
				break;
			if(ovector[0] == ovector[1])
			{ //matched the empty string
				offset++;
				continue;
			}
			if (res->size() >= limit)
				break;
			//Extract string from last match until the beginning of the current match
			ASObject* s=abstract_s(wrk,data.substr_bytes(lastMatch,end-lastMatch));
			res->push(asAtomHandler::fromObject(s));
			lastMatch=offset=ovector[1];

			//Insert capturing groups
			for(int i=1;i<rc;i++)
			{
				if (res->size() >= limit)
					break;
				//use string interface through raw(), because we index on bytes, not on UTF-8 characters
				ASObject* s=abstract_s(wrk,data.substr_bytes(ovector[i*2],ovector[i*2+1]-ovector[i*2]));
				res->push(asAtomHandler::fromObject(s));
			}
		}
		while(end<data.numBytes() && res->size() < limit);
		if(res->size() < limit && lastMatch != data.numBytes()+1)
		{
			ASObject* s=abstract_s(wrk,data.substr_bytes(lastMatch,data.numBytes()-lastMatch));
			res->push(asAtomHandler::fromObject(s));
		}
		pcre_free(pcreRE);
	}
	else
	{
		tiny_string del;
		asAtomHandler::getStringView(del,args[0],wrk);
		if(del.empty())
		{
			//the string is empty, so split every character

			if (data.numChars() == 0)
			{
				res->push(asAtomHandler::fromStringID(BUILTIN_STRINGS::EMPTY));
			}
			uint32_t j = 0;
			for(auto i=data.begin();i!=data.end();++i)
			{
				if (j >= limit)
					break;
				j++;
				res->push(asAtomHandler::fromObject(abstract_s(wrk, tiny_string::fromChar(*i) ) ));
			}
			ret = asAtomHandler::fromObject(res);
			return;
		}
		unsigned int start=0;
		unsigned int len = data.numChars();
		do
		{
			int match=data.find(del,start);
			if(del.empty())
				match++;
			if(match==-1)
				match= len;
			if (res->size() >= limit)
				break;
			ASObject* s;
			if (data.isSinglebyte()) // fast path for ascii strings to avoid unneccessary buffer copying
				s=abstract_s(wrk,data.raw_buf()+start,match-start,match-start,data.isSinglebyte(),data.hasNullEntries());
			else
				s=abstract_s(wrk,data.substr(start,(match-start)));
			res->push(asAtomHandler::fromObject(s));
			start=match+del.numChars();
			if (start == len)
				res->push(asAtomHandler::fromStringID(BUILTIN_STRINGS::EMPTY));
		}
		while(start<len && res->size() < limit);
	}
	ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(ASString,substr)
{
	int start=0;
	if(argslen>=1)
	{
		if (!std::isnan(asAtomHandler::toNumber(args[0])))
			start=asAtomHandler::toInt(args[0]);
		if (start >= 0  && std::isinf(asAtomHandler::toNumber(args[0])))
		{
			ret = asAtomHandler::fromStringID(BUILTIN_STRINGS::EMPTY);
			return;
		}
	}
	int numchars=0;
	if (asAtomHandler::isStringID(obj))
		numchars = wrk->getSystemState()->getStringFromUniqueId(asAtomHandler::getStringId(obj)).numChars();
	else if (asAtomHandler::is<ASString>(obj))
		numchars = asAtomHandler::as<ASString>(obj)->getData().numChars();
	else
		numchars = asAtomHandler::toString(obj,wrk).numChars();
	if(start<0) {
		start=numchars+start;
		if(start<0)
			start=0;
	}
	if(start>numchars)
		start=numchars;
	int len=0x7fffffff;
	if (argslen==2 && !asAtomHandler::is<Undefined>(args[1]))
	{
		if (std::isinf(asAtomHandler::toNumber(args[1])))
		{
			if (asAtomHandler::toInt(args[1]) < 0)
				len = 0;
		}
		else
			len=asAtomHandler::toInt(args[1]);
		if (len<0)
		{
			len+=numchars;
			if(len<0)
				len=0;
		}
	}

	if (asAtomHandler::isStringID(obj))
		ret = asAtomHandler::fromObject(abstract_s(wrk,wrk->getSystemState()->getStringFromUniqueId(asAtomHandler::getStringId(obj)).substr(start,len)));
	else if (asAtomHandler::is<ASString>(obj))
	{
		ASString* th = asAtomHandler::as<ASString>(obj);
		ret = asAtomHandler::fromObject(abstract_s(wrk,th->data.substr_bytes(th->getBytePosition(start),start+len >= numchars ? UINT32_MAX  : (th->getBytePosition(start+len)-th->getBytePosition(start)))));
	}
	else
		ret = asAtomHandler::fromObject(abstract_s(wrk,asAtomHandler::toString(obj,wrk).substr(start,len)));
}

ASFUNCTIONBODY_ATOM(ASString,substring)
{
	tiny_string data = asAtomHandler::toString(obj,wrk);

	number_t start, end;
	ARG_CHECK(ARG_UNPACK (start,0) (end,0x7fffffff));
	if(start<0 || std::isnan(start))
		start=0;
	if(start>(int)data.numChars() || std::isinf(start))
		start=data.numChars();

	if(end<0 || std::isnan(end))
		end=0;
	if(end>(int)data.numChars() || std::isinf(end))
		end=data.numChars();

	if(start>end) {
		number_t tmp=start;
		start=end;
		end=tmp;
	}

	ret = asAtomHandler::fromObject(abstract_s(wrk,data.substr(start,end-start)));
}

number_t ASString::toNumber()
{
	assert_and_throw(implEnable);

	const char *s = getData().raw_buf();
	while (*s && isEcmaSpace(g_utf8_get_char(s)))
		s = g_utf8_next_char(s);

	double val;
	char *end = nullptr;
	val = parseStringInfinite(s, &end);

	// If did not parse as infinite, try decimal
	if (!std::isinf(val))
	{
		errno = 0;
		val = g_ascii_strtod(s, &end);

		if (errno == ERANGE)
		{
			if (val == HUGE_VAL)
				val = numeric_limits<double>::infinity();
			else if (val == -HUGE_VAL)
				val = -numeric_limits<double>::infinity();
		}
		else if (std::isinf(val))
		{
			// strtod accepts values such as "inf" and lowercase
			// "infinity" which are not valid values in Flash
			return numeric_limits<double>::quiet_NaN();
		}
	}

	// Fail if there is any rubbish after the converted number
	while(*end) {
		if(!isEcmaSpace(g_utf8_get_char(end)))
			return numeric_limits<double>::quiet_NaN();
		end = g_utf8_next_char(end);
	}

	return val;
}

number_t ASString::parseStringInfinite(const char *s, char **end) const
{
	if (end)
		*end = const_cast<char *>(s);
	double sign = 1.;
	if (*s == '+')
	{
		sign = +1.;
		s++;
	}
	else if (*s == '-')
	{
		sign = -1.;
		s++;
	}
	if (strncmp(s, "Infinity", 8) == 0)
	{
		if (end)
			*end = const_cast<char *>(s+8);
		return sign*numeric_limits<double>::infinity();
	}

	return 0.; // not an infinite value
}

int32_t ASString::toInt()
{
	assert_and_throw(implEnable);
	return Integer::stringToASInteger(getData().raw_buf(), 0);
}
int32_t ASString::toIntStrict()
{
	assert_and_throw(implEnable);
	bool isvalid=false;
	int32_t ret = Integer::stringToASInteger(getData().raw_buf(), 0,true,&isvalid);
	if (!isvalid)
	{
		number_t n = toNumber();
		if (!std::isnan(n))
			return n;
	}
	return ret;
}
int64_t ASString::toInt64()
{
	number_t value;
	bool valid=Integer::fromStringFlashCompatible(getData().raw_buf(), value, 0);
	if (!valid)
		return 0;
	return (int64_t)value;
}
uint32_t ASString::toUInt()
{
	assert_and_throw(implEnable);
	return static_cast<uint32_t>(toInt());
}

bool ASString::isEqual(ASObject* r)
{
	assert_and_throw(implEnable);
	switch(r->getObjectType())
	{
		case T_STRING:
		{
			if (!this->isConstructed())
				return !r->isConstructed();
			ASString* s=static_cast<ASString*>(r);
			return s->hasId && hasId ? s->stringId == stringId : s->getData()==getData();
		}
		case T_INTEGER:
		case T_UINTEGER:
		case T_NUMBER:
		case T_BOOLEAN:
			return toNumber()==r->toNumber();
		case T_NULL:
		case T_UNDEFINED:
			if (!this->isConstructed())
				return true;
			return false;
		default:
			return r->isEqual(this);
	}
}

TRISTATE ASString::isLess(ASObject* r)
{
	//ECMA-262 11.8.5 algorithm
	assert_and_throw(implEnable);
	asAtom rprim=asAtomHandler::invalidAtom;
	number_t a=0;
	number_t b=0;
	bool isrefcounted;
	r->toPrimitive(rprim,isrefcounted);
	if(getObjectType()==T_STRING && asAtomHandler::isString(rprim))
	{
		ASString* rstr=static_cast<ASString*>(asAtomHandler::toObject(rprim,getInstanceWorker()));
		TRISTATE res = (getData()<rstr->getData())?TTRUE:TFALSE;
		if (isrefcounted)
			ASATOM_DECREF(rprim);
		return res;
	}
	a=toNumber();
	b=asAtomHandler::toNumber(rprim);
	if (isrefcounted)
		ASATOM_DECREF(rprim);
	if(std::isnan(a) || std::isnan(b))
		return TUNDEFINED;
	return (a<b)?TTRUE:TFALSE;
}

TRISTATE ASString::isLessAtom(asAtom& r)
{
	//ECMA-262 11.8.5 algorithm
	assert_and_throw(implEnable);
	bool isrefcounted=false;
	asAtom rprim = r;
	if (asAtomHandler::getObject(r))
		asAtomHandler::getObject(r)->toPrimitive(rprim,isrefcounted);
	if(getObjectType()==T_STRING && asAtomHandler::isString(rprim))
	{
		ASString* rstr=static_cast<ASString*>(asAtomHandler::toObject(rprim,getInstanceWorker()));
		return (getData()<rstr->getData())?TTRUE:TFALSE;
	}
	number_t a=toNumber();
	number_t b=asAtomHandler::toNumber(rprim);
	if (isrefcounted)
		ASATOM_DECREF(rprim);
	if(std::isnan(a) || std::isnan(b))
		return TUNDEFINED;
	return (a<b)?TTRUE:TFALSE;
}

void ASString::serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap,ASWorker* wrk)
{
	if (out->getObjectEncoding() == OBJECT_ENCODING::AMF0)
	{
		out->writeByte(amf0_string_marker);
		out->writeStringAMF0(getData());
	}
	else
	{
		out->writeByte(string_marker);
		out->writeStringVR(stringMap, getData());
	}
}

string ASString::toDebugString() const
{
	tiny_string ret;
	if (!datafilled && hasId)
		ret = std::string("\"") + std::string(getSystemState()->getStringFromUniqueId(stringId)) + "\"_id";
	else
		ret = std::string("\"") + std::string(data) + "\"";
#ifndef _NDEBUG
	char buf[300];
	sprintf(buf,"(%p/%d/%d/%d%s) ",this,this->getRefCount(),this->storedmembercount,this->getConstant(),this->isConstructed()?"":" not constructed");
	ret += buf;
#endif
	return ret;
}

ASFUNCTIONBODY_ATOM(ASString,slice)
{
	tiny_string data;
	asAtomHandler::getStringView(data,obj,wrk);
	int startIndex=0;
	if(argslen>=1)
		startIndex=asAtomHandler::toInt(args[0]);
	if(startIndex<0) {
		startIndex=data.numChars()+startIndex;
		if(startIndex<0)
			startIndex=0;
	}
	if(startIndex>(int)data.numChars())
		startIndex=(int)data.numChars();

	int endIndex=0x7fffffff;
	if(argslen>=2)
		endIndex=asAtomHandler::toInt(args[1]);
	if(endIndex<0) {
		endIndex=data.numChars()+endIndex;
		if(endIndex<0)
			endIndex=0;
	}
	if(endIndex>(int)data.numChars())
		endIndex=(int)data.numChars();
	if(endIndex<=startIndex)
		ret = asAtomHandler::fromStringID(BUILTIN_STRINGS::EMPTY);
	else
	{
		if (data.isSinglebyte()) // fast path for ascii strings to avoid unneccessary buffer copying
			ret = asAtomHandler::fromObject(abstract_s(wrk,data.raw_buf()+startIndex,endIndex-startIndex,endIndex-startIndex,data.isSinglebyte(),data.hasNullEntries()));
		else
			ret = asAtomHandler::fromObject(abstract_s(wrk,data.substr(startIndex,endIndex-startIndex)));
	}
}
ASFUNCTIONBODY_ATOM(ASString,charAt)
{
	number_t index;
	ARG_CHECK(ARG_UNPACK (index, 0));
	// fast path if obj is ASString
	if (asAtomHandler::isStringID(obj))
	{
		const tiny_string& s = wrk->getSystemState()->getStringFromUniqueId(asAtomHandler::toStringId(obj,wrk));
		int maxIndex=s.numChars();
		if(index<0 || index>=maxIndex || std::isinf(index))
		{
			ret = asAtomHandler::fromStringID(BUILTIN_STRINGS::EMPTY);
			return;
		}
		uint32_t c = s.charAt(index);
		ret = c < BUILTIN_STRINGS_CHAR_MAX ? asAtomHandler::fromStringID(c) : asAtomHandler::fromObject(abstract_s(wrk, tiny_string::fromChar(c) ));
		return;
	}
	else if (asAtomHandler::isString(obj))
	{
		ASString* th = asAtomHandler::as<ASString>(obj);
		int maxIndex=th->getData().numChars();
		
		if(index<0 || index>=maxIndex || std::isinf(index))
		{
			ret = asAtomHandler::fromStringID(BUILTIN_STRINGS::EMPTY);
			return;
		}
		uint32_t c = g_utf8_get_char(th->getData().raw_buf()+th->getBytePosition(index));
		ret = c < BUILTIN_STRINGS_CHAR_MAX ? asAtomHandler::fromStringID(c) : asAtomHandler::fromObject(abstract_s(wrk, tiny_string::fromChar(c) ));
		return;
	}
	tiny_string data = asAtomHandler::toString(obj,wrk);
	int maxIndex=data.numChars();
	if(index<0 || index>=maxIndex || std::isinf(index))
		ret = asAtomHandler::fromStringID(BUILTIN_STRINGS::EMPTY);
	else
	{
		uint32_t c = data.charAt(index);
		ret = c < BUILTIN_STRINGS_CHAR_MAX ? asAtomHandler::fromStringID(c) : asAtomHandler::fromObject(abstract_s(wrk, tiny_string::fromChar(c) ));
	}
}

ASFUNCTIONBODY_ATOM(ASString,charCodeAt)
{
	int64_t index;
	
	ARG_CHECK(ARG_UNPACK (index, 0));

	// fast path if obj is ASString
	if (asAtomHandler::isStringID(obj))
	{
		tiny_string s = wrk->getSystemState()->getStringFromUniqueId(asAtomHandler::getStringId(obj));
		if(index<0 || index>=(int64_t)s.numChars())
			asAtomHandler::setNumber(ret,wrk,Number::NaN);
		else
			asAtomHandler::setInt(ret,wrk,(int32_t)s.charAt(index));
	}
	else if (asAtomHandler::isString(obj) && asAtomHandler::getObject(obj))
	{
		ASString* th = asAtomHandler::as<ASString>(obj);
		if(index<0 || index>=(int64_t)th->getData().numChars())
			asAtomHandler::setNumber(ret,wrk,Number::NaN);
		else
		{
			uint32_t c = g_utf8_get_char(th->getData().raw_buf()+th->getBytePosition(index));
			asAtomHandler::setInt(ret,wrk,(int32_t)c);
		}
	}
	else
	{
		tiny_string data = asAtomHandler::toString(obj,wrk);
		if(index<0 || index>=(int64_t)data.numChars())
			asAtomHandler::setNumber(ret,wrk,Number::NaN);
		else
		{
			//Character codes are expected to be positive
			asAtomHandler::setInt(ret,wrk,(int32_t)data.charAt(index));
		}
	}
}

ASFUNCTIONBODY_ATOM(ASString,indexOf)
{
	if (argslen == 0)
	{
		asAtomHandler::setInt(ret,wrk,-1);
		return;
	}
	tiny_string data;
	asAtomHandler::getStringView(data,obj,wrk);
	tiny_string arg0;
	asAtomHandler::getStringView(arg0,args[0],wrk);
	int startIndex=0;
	if(argslen>1)
		startIndex=asAtomHandler::toInt(args[1]);
	startIndex = imin(imax(startIndex, 0), data.numChars());

	size_t pos = data.find(arg0, startIndex);
	if(pos == data.npos)
		asAtomHandler::setInt(ret,wrk,-1);
	else
		asAtomHandler::setInt(ret,wrk,(int32_t)pos);
}

ASFUNCTIONBODY_ATOM(ASString,lastIndexOf)
{
	assert_and_throw(argslen==1 || argslen==2);
	tiny_string data;
	asAtomHandler::getStringView(data,obj,wrk);
	tiny_string val=asAtomHandler::toString(args[0],wrk);
	size_t startIndex=data.npos;
	if(argslen > 1 && !asAtomHandler::isUndefined(args[1]) && !std::isnan(asAtomHandler::toNumber(args[1])) && !(asAtomHandler::toNumber(args[1]) > 0 && std::isinf(asAtomHandler::toNumber(args[1]))))
	{
		int32_t i = asAtomHandler::toInt(args[1]);
		if(i<0)
		{
			asAtomHandler::setInt(ret,wrk,-1);
			return;
		}
		startIndex = i;
	}

	startIndex = imin(startIndex, data.numChars());

	size_t pos=data.rfind(val.raw_buf(), startIndex);
	if(pos==data.npos)
		asAtomHandler::setInt(ret,wrk,-1);
	else
		asAtomHandler::setInt(ret,wrk,(int32_t)pos);
}

ASFUNCTIONBODY_ATOM(ASString,toLowerCase)
{
	tiny_string data = asAtomHandler::toString(obj,wrk);
	if (asAtomHandler::is<ASString>(obj) && asAtomHandler::toStringId(obj,wrk) != UINT32_MAX)
		ret = asAtomHandler::fromStringID(wrk->getSystemState()->getUniqueStringId(data.lowercase()));
	else
		ret = asAtomHandler::fromObject(abstract_s(wrk,data.lowercase()));
}

ASFUNCTIONBODY_ATOM(ASString,toUpperCase)
{
	tiny_string data = asAtomHandler::toString(obj,wrk);
	if (asAtomHandler::is<ASString>(obj) && asAtomHandler::toStringId(obj,wrk) != UINT32_MAX)
		ret = asAtomHandler::fromStringID(wrk->getSystemState()->getUniqueStringId(data.uppercase()));
	else
		ret = asAtomHandler::fromObject(abstract_s(wrk,data.uppercase()));
}
ASFUNCTIONBODY_ATOM(ASString,localeCompare)
{
	tiny_string data = asAtomHandler::toString(obj,wrk);
	tiny_string other;
	ARG_CHECK(ARG_UNPACK_MORE_ALLOWED(other));
	if (argslen > 1)
		LOG(LOG_NOT_IMPLEMENTED,"localeCompare with more than one parameter not implemented");
	if (wrk->getSystemState()->getSwfVersion() < 11)
	{
		if (asAtomHandler::is<Null>(args[0]) || asAtomHandler::is<Undefined>(args[0]))
		{
			asAtomHandler::setInt(ret,wrk,data == "" ? 1 : 0);
			return;
		}
	}
	int res = data.compare(other);
	asAtomHandler::setInt(ret,wrk,res);
}
ASFUNCTIONBODY_ATOM(ASString,localeCompare_prototype)
{
	tiny_string data = asAtomHandler::toString(obj,wrk);
	tiny_string other;
	ARG_CHECK(ARG_UNPACK_MORE_ALLOWED(other));
	if (argslen > 1)
	{
		createError<ArgumentError>(wrk,kWrongArgumentCountError, "localeCompare", "1",Integer::toString(argslen));
		return;
	}

	if (wrk->getSystemState()->getSwfVersion() < 11)
	{
		if (asAtomHandler::is<Null>(args[0]) || asAtomHandler::is<Undefined>(args[0]))
		{
			asAtomHandler::setInt(ret,wrk,data == "" ? 1 : 0);
			return;
		}
	}
	int res = data.compare(other);
	asAtomHandler::setInt(ret,wrk,res);
}

ASFUNCTIONBODY_ATOM(ASString,fromCharCode)
{
	if (argslen == 1 && ((asAtomHandler::toUInt(args[0])& 0xFFFF) != 0))
	{
		ret = asAtomHandler::fromStringID(asAtomHandler::toUInt(args[0])& 0xFFFF);
		return;
	}
	ASString* res=abstract_s(wrk)->as<ASString>();
	for(uint32_t i=0;i<argslen;i++)
	{
		res->stringId = UINT32_MAX;
		res->hasId = false;
		res->getData() += tiny_string::fromChar(asAtomHandler::toUInt(args[i])& 0xFFFF);
	}
	ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(ASString,replace)
{
	tiny_string data;
	asAtomHandler::getStringView(data,obj,wrk);
	enum REPLACE_TYPE { STRING=0, FUNC };
	REPLACE_TYPE type;
	ASString* res=abstract_s(wrk,data.raw_buf(),data.numBytes(), data.numChars(), data.isSinglebyte(), data.hasNullEntries())->as<ASString>();
	tiny_string replaceWith;
	if(argslen < 2)
	{
		type = STRING;
	}
	else if(!asAtomHandler::isFunction(args[1]))
	{
		type = STRING;
		asAtomHandler::getStringView(replaceWith,args[1],wrk);
	}
	else
	{
		asAtomHandler::getStringView(replaceWith,args[1],wrk);
		type = FUNC;
	}
	if(asAtomHandler::is<RegExp>(args[0]))
	{
		RegExp* re=asAtomHandler::as<RegExp>(args[0]);

		pcre* pcreRE = re->compile(!data.isSinglebyte());
		if (!pcreRE)
		{
			ret = asAtomHandler::fromObject(res);
			return;
		}

		int capturingGroups;
		int infoOk=pcre_fullinfo(pcreRE, nullptr, PCRE_INFO_CAPTURECOUNT, &capturingGroups);
		if(infoOk!=0)
		{
			pcre_free(pcreRE);
			ret = asAtomHandler::fromObject(res);
			return;
		}
		pcre_extra extra;
		extra.match_limit_recursion=200;
		extra.flags = PCRE_EXTRA_MATCH_LIMIT_RECURSION;
		int ovector[(capturingGroups+1)*3];
		int offset=0;
		int retDiff=0;
		tiny_string prevsubstring = "";
		do
		{
			tiny_string replaceWithTmp = replaceWith;
			int rc=pcre_exec(pcreRE, &extra, res->getData().raw_buf(), res->getData().numBytes(), offset, PCRE_NO_UTF8_CHECK, ovector, (capturingGroups+1)*3);
			if(rc<0)
			{
				//No matches or error
				pcre_free(pcreRE);
				ret = asAtomHandler::fromObject(res);
				return;
			}
			prevsubstring += res->getData().substr_bytes(offset,ovector[0]-offset);
			if(type==FUNC)
			{
				//Get the replace for this match
				vector<asAtom> subargs;
				subargs.reserve(3+capturingGroups);

				//we index on bytes, not on UTF-8 characters
				subargs.push_back(asAtomHandler::fromObject(abstract_s(wrk,res->data.substr_bytes(ovector[0],ovector[1]-ovector[0]))));
				for(int i=0;i<capturingGroups;i++)
				{
					if (ovector[i*2+2] >= 0)
						subargs.push_back(asAtomHandler::fromObject(abstract_s(wrk,res->data.substr_bytes(ovector[i*2+2],ovector[i*2+3]-ovector[i*2+2]))));
					else
						subargs.push_back(asAtomHandler::undefinedAtom);
				}
				subargs.push_back(asAtomHandler::fromInt((int32_t)(ovector[0]-retDiff)));
				subargs.push_back(asAtomHandler::fromObject(abstract_s(wrk,data)));
				asAtom ret=asAtomHandler::invalidAtom;
				asAtom obj = asAtomHandler::nullAtom;
				if (asAtomHandler::as<IFunction>(args[1])->closure_this) // use closure as "this" in function call
					obj = asAtomHandler::fromObject(asAtomHandler::as<IFunction>(args[1])->closure_this);
				asAtomHandler::callFunction(args[1],wrk,ret,obj, subargs.data(), subargs.size(),true);
				replaceWithTmp=asAtomHandler::toString(ret,wrk).raw_buf();
				ASATOM_DECREF(ret);
			} else {
				size_t pos, ipos = 0;
				tiny_string group;
				int i, j;
				while((pos = replaceWithTmp.find("$", ipos)) != tiny_string::npos) {
					i = 0;
					ipos = pos;
					/* docu is not clear what to do if the $nn value is higher 
						 * than the number of matching groups, 
						 * but the ecma3/String/eregress_104375 test indicates
						 * that we should take it as an $n value
						 */
					
					while (++ipos < replaceWithTmp.numChars() && i<10) {
						j = replaceWithTmp.charAt(ipos)-'0';
						if ((j <0) || (j> 9) || (rc < 10*i + j))
							break;
						i = 10*i + j;
					}
					if (i == 0)
					{
						switch (replaceWithTmp.charAt(ipos))
						{
							case '$':
								replaceWithTmp.replace(ipos,1,"");
								break;
							case '&':
								replaceWithTmp.replace(ipos-1,2,res->getData().substr_bytes(ovector[0], ovector[1]-ovector[0]));
								break;
							case '`':
								replaceWithTmp.replace(ipos-1,2,prevsubstring);
								break;
							case '\'':
								replaceWithTmp.replace(ipos-1,2,res->getData().substr_bytes(ovector[1],res->getData().numBytes()-ovector[1]));
								break;
						}
						continue;
					}
					group = (i >= rc) ? "" : res->getData().substr_bytes(ovector[i*2], ovector[i*2+1]-ovector[i*2]);
					replaceWithTmp.replace(pos, ipos-pos, group);
				}
			}
			prevsubstring += res->getData().substr_bytes(ovector[0],ovector[1]-ovector[0]);
			res->hasId = false;
			res->getData().replace_bytes(ovector[0],ovector[1]-ovector[0],replaceWithTmp);
			res->getData().checkValidUTF();
			offset=ovector[0]+replaceWithTmp.numBytes();
			if (ovector[0] == ovector[1])
				offset+=1;
			retDiff+=replaceWithTmp.numBytes()-(ovector[1]-ovector[0]);
		}
		while(re->global);
		pcre_free(pcreRE);
	}
	else
	{
		tiny_string s;
		asAtomHandler::getStringView(s,args[0],wrk);
		int index=res->getData().find(s,0);
		if(index==-1) //No result
		{
			ret = asAtomHandler::fromObject(res);
			return;
		}
		res->hasId = false;
		res->getData().replace(index,s.numChars(),replaceWith);
	}
	ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(ASString,concat)
{
	tiny_string data = asAtomHandler::toString(obj,wrk);
	ASString* res=abstract_s(wrk,data)->as<ASString>();
	for(unsigned int i=0;i<argslen;i++)
	{
		res->hasId = false;
		res->getData()+=asAtomHandler::toString(args[i],wrk).raw_buf();
	}

	ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(ASString,generator)
{
	assert(argslen<=1);
	if (argslen == 0)
		ret = asAtomHandler::fromStringID(BUILTIN_STRINGS::EMPTY);
	else
		ret = asAtomHandler::fromObject(abstract_s(wrk,asAtomHandler::toString(args[0],wrk)));
}

bool ASString::isEcmaSpace(uint32_t c)
{
	return (c == 0x09) || (c == 0x0B) || (c == 0x0C) || (c == 0x20) ||
		(c == 0xA0) || (c == 0x1680) || (c == 0x180E) ||
		((c >= 0x2000) && (c <= 0x200B)) || (c == 0x202F) ||
		(c == 0x205F) || (c == 0x3000) || (c == 0xFEFF) || 
		isEcmaLineTerminator(c);
}

bool ASString::isEcmaLineTerminator(uint32_t c)
{
	return (c == 0x0A) || (c == 0x0D) || (c == 0x2028) || (c == 0x2029);
}
