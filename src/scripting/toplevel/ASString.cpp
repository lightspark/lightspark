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

#include <pcre.h>

#include "scripting/toplevel/ASString.h"
#include "scripting/flash/utils/ByteArray.h"
#include "compat.h"
#include "scripting/argconv.h"
#include "parsing/amf3_generator.h"
#include "scripting/toplevel/RegExp.h"

using namespace std;
using namespace lightspark;

ASString::ASString(Class_base* c):ASObject(c,T_STRING),currentindex(0),hasId(true),datafilled(true)
{
	stringId = BUILTIN_STRINGS::EMPTY;
}

ASString::ASString(Class_base* c,const string& s) : ASObject(c,T_STRING),data(s),currentindex(0),hasId(false),datafilled(true)
{
}

ASString::ASString(Class_base* c,const tiny_string& s) : ASObject(c,T_STRING),data(s),currentindex(0),hasId(false),datafilled(true)
{
}

ASString::ASString(Class_base* c,const char* s) : ASObject(c,T_STRING),data(s, /*copy:*/true),currentindex(0),hasId(false),datafilled(true)
{
}

ASString::ASString(Class_base* c,const char* s, uint32_t len) : ASObject(c,T_STRING)
{
	data = std::string(s,len);
	hasId = false;
	datafilled=true;
	currentindex=0;
}

ASFUNCTIONBODY_ATOM(ASString,_constructor)
{
	ASString* th=asAtomHandler::as<ASString>(obj);
	if(args && argslen==1)
	{
		th->data=asAtomHandler::toString(args[0],sys);
		th->hasId = false;
		th->stringId = UINT32_MAX;
		th->datafilled = true;
	}
}

ASFUNCTIONBODY_ATOM(ASString,_getLength)
{
	// fast path if obj is ASString
	if (asAtomHandler::isString(obj) && asAtomHandler::getObject(obj))
	{
		ASString* th = asAtomHandler::getObject(obj)->as<ASString>();
		asAtomHandler::setInt(ret,sys,(int32_t)th->getData().numChars());
	}
	else
		asAtomHandler::setInt(ret,sys,(int32_t)asAtomHandler::toString(obj,sys).numChars());
}

void ASString::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_FINAL | CLASS_SEALED);
	c->isReusable = true;
	c->setDeclaredMethodByQName("split",AS3,Class<IFunction>::getFunction(c->getSystemState(),split,2),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("substr",AS3,Class<IFunction>::getFunction(c->getSystemState(),substr,2),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("substring",AS3,Class<IFunction>::getFunction(c->getSystemState(),substring,2),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("replace",AS3,Class<IFunction>::getFunction(c->getSystemState(),replace),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("concat",AS3,Class<IFunction>::getFunction(c->getSystemState(),concat),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("match",AS3,Class<IFunction>::getFunction(c->getSystemState(),match),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("search",AS3,Class<IFunction>::getFunction(c->getSystemState(),search),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("indexOf",AS3,Class<IFunction>::getFunction(c->getSystemState(),indexOf,2),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("lastIndexOf",AS3,Class<IFunction>::getFunction(c->getSystemState(),lastIndexOf,2),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("charCodeAt",AS3,Class<IFunction>::getFunction(c->getSystemState(),charCodeAt),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("charAt",AS3,Class<IFunction>::getFunction(c->getSystemState(),charAt,1),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("slice",AS3,Class<IFunction>::getFunction(c->getSystemState(),slice,2),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("toLocaleLowerCase",AS3,Class<IFunction>::getFunction(c->getSystemState(),toLowerCase),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("toLocaleUpperCase",AS3,Class<IFunction>::getFunction(c->getSystemState(),toUpperCase),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("toLowerCase",AS3,Class<IFunction>::getFunction(c->getSystemState(),toLowerCase),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("toUpperCase",AS3,Class<IFunction>::getFunction(c->getSystemState(),toUpperCase),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("localeCompare",AS3,Class<IFunction>::getFunction(c->getSystemState(),localeCompare),NORMAL_METHOD,true);
	// According to specs fromCharCode belongs to AS3 namespace,
	// but also empty namespace is seen in the wild and should be
	// supported.
	c->setDeclaredMethodByQName("fromCharCode",AS3,Class<IFunction>::getFunction(c->getSystemState(),fromCharCode),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("fromCharCode","",Class<IFunction>::getFunction(c->getSystemState(),fromCharCode),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("length","",Class<IFunction>::getFunction(c->getSystemState(),_getLength),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("toString",AS3,Class<IFunction>::getFunction(c->getSystemState(),_toString),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("valueOf",AS3,Class<IFunction>::getFunction(c->getSystemState(),_toString),NORMAL_METHOD,true);

	c->prototype->setVariableByQName("split","",Class<IFunction>::getFunction(c->getSystemState(),split,2),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("substr","",Class<IFunction>::getFunction(c->getSystemState(),substr,2),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("substring","",Class<IFunction>::getFunction(c->getSystemState(),substring,2),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("replace","",Class<IFunction>::getFunction(c->getSystemState(),replace),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("concat","",Class<IFunction>::getFunction(c->getSystemState(),concat),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("match","",Class<IFunction>::getFunction(c->getSystemState(),match),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("search","",Class<IFunction>::getFunction(c->getSystemState(),search),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("indexOf","",Class<IFunction>::getFunction(c->getSystemState(),indexOf,2),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("lastIndexOf","",Class<IFunction>::getFunction(c->getSystemState(),lastIndexOf,2),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("charCodeAt","",Class<IFunction>::getFunction(c->getSystemState(),charCodeAt),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("charAt","",Class<IFunction>::getFunction(c->getSystemState(),charAt,1),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("slice","",Class<IFunction>::getFunction(c->getSystemState(),slice,2),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("toLocaleLowerCase","",Class<IFunction>::getFunction(c->getSystemState(),toLowerCase),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("toLocaleUpperCase","",Class<IFunction>::getFunction(c->getSystemState(),toUpperCase),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("toLowerCase","",Class<IFunction>::getFunction(c->getSystemState(),toLowerCase),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("toUpperCase","",Class<IFunction>::getFunction(c->getSystemState(),toUpperCase),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("toString","",Class<IFunction>::getFunction(c->getSystemState(),_toString),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("valueOf","",Class<IFunction>::getFunction(c->getSystemState(),_toString),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("localeCompare","",Class<IFunction>::getFunction(c->getSystemState(),localeCompare_prototype),CONSTANT_TRAIT);
}

void ASString::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY_ATOM(ASString,search)
{
	tiny_string data = asAtomHandler::toString(obj,sys);
	int res = -1;
	if(argslen == 0 || asAtomHandler::isUndefined(args[0]))
	{
		asAtomHandler::setInt(ret,sys,res);
		return;
	}

	int options=PCRE_UTF8|PCRE_NEWLINE_ANY;//|PCRE_JAVASCRIPT_COMPAT;
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
		restr = asAtomHandler::toString(args[0],sys);
	}

	const char* error;
	int errorOffset;
	pcre* pcreRE=pcre_compile(restr.raw_buf(), options, &error, &errorOffset,NULL);
	if(error)
	{
		asAtomHandler::setInt(ret,sys,res);
		return;
	}
	int capturingGroups;
	int infoOk=pcre_fullinfo(pcreRE, NULL, PCRE_INFO_CAPTURECOUNT, &capturingGroups);
	if(infoOk!=0)
	{
		pcre_free(pcreRE);
		asAtomHandler::setInt(ret,sys,res);
		return;
	}
	pcre_extra extra;
	extra.match_limit_recursion=200;
	extra.flags = PCRE_EXTRA_MATCH_LIMIT_RECURSION;
	int ovector[(capturingGroups+1)*3];
	int offset=0;
	//Global is not used in search
	int rc=pcre_exec(pcreRE, &extra, data.raw_buf(), data.numBytes(), offset, 0, ovector, (capturingGroups+1)*3);
	if(rc<0)
	{
		//No matches or error
		pcre_free(pcreRE);
		asAtomHandler::setInt(ret,sys,res);
		return;
	}
	res=ovector[0];
	// pcre_exec returns byte position, so we have to convert it to character position 
	tiny_string tmp = data.substr_bytes(0, res);
	res = tmp.numChars();
	asAtomHandler::setInt(ret,sys,res);
}

ASFUNCTIONBODY_ATOM(ASString,match)
{
	tiny_string data = asAtomHandler::toString(obj,sys);
	if(argslen == 0 || asAtomHandler::isNull(args[0]) || asAtomHandler::isUndefined(args[0]))
	{
		asAtomHandler::setNull(ret);
		return;
	}
	ASObject* res=NULL;
	RegExp* re;

	if(asAtomHandler::is<RegExp>(args[0]))
	{
		re = asAtomHandler::as<RegExp>(args[0]);
		re->incRef();
	}
	else
	{
		re = Class<RegExp>::getInstanceS(sys,asAtomHandler::toString(args[0],sys));
	}

	if (re->global)
	{
		Array *resarr = Class<Array>::getInstanceSNoArgs(sys);
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
			resarr->push(match->as<Array>()->at(0));
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
	if(Class<ASString>::getClass(sys)->prototype->getObj() == asAtomHandler::getObject(obj))
	{
		ret = asAtomHandler::fromStringID(BUILTIN_STRINGS::EMPTY);
		return;
	}
	if(!asAtomHandler::is<ASString>(obj))
	{
		LOG(LOG_ERROR,"String.toString is not generic:"<<asAtomHandler::toDebugString(obj));
		throw Class<TypeError>::getInstanceS(sys,"String.toString is not generic");
	}
	assert_and_throw(argslen==0);

	//As ASStrings are immutable, we can just return ourself
	ASATOM_INCREF(obj);
	ret = obj;
}

ASFUNCTIONBODY_ATOM(ASString,split)
{
	tiny_string data = asAtomHandler::toString(obj,sys);
	Array* res=Class<Array>::getInstanceSNoArgs(sys);
	uint32_t limit = 0x7fffffff;
	if(argslen == 0 )
	{
		res->push(asAtomHandler::fromObject(abstract_s(sys,data)));
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
				res->push(asAtomHandler::fromObject(abstract_s(sys, tiny_string::fromChar(*i) ) ));
			}
			ret = asAtomHandler::fromObject(res);
			return;
		}

		pcre* pcreRE = re->compile();
		if (!pcreRE)
		{
			ret = asAtomHandler::fromObject(res);
			return;
		}
		int capturingGroups;
		int infoOk=pcre_fullinfo(pcreRE, NULL, PCRE_INFO_CAPTURECOUNT, &capturingGroups);
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
			int rc=pcre_exec(pcreRE, &extra, data.raw_buf(), data.numBytes(), offset, 0, ovector, (capturingGroups+1)*3);
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
			ASObject* s=abstract_s(sys,data.substr_bytes(lastMatch,end-lastMatch));
			res->push(asAtomHandler::fromObject(s));
			lastMatch=offset=ovector[1];

			//Insert capturing groups
			for(int i=1;i<rc;i++)
			{
				if (res->size() >= limit)
					break;
				//use string interface through raw(), because we index on bytes, not on UTF-8 characters
				ASObject* s=abstract_s(sys,data.substr_bytes(ovector[i*2],ovector[i*2+1]-ovector[i*2]));
				res->push(asAtomHandler::fromObject(s));
			}
		}
		while(end<data.numBytes() && res->size() < limit);
		if(res->size() < limit && lastMatch != data.numBytes()+1)
		{
			ASObject* s=abstract_s(sys,data.substr_bytes(lastMatch,data.numBytes()-lastMatch));
			res->push(asAtomHandler::fromObject(s));
		}
		pcre_free(pcreRE);
	}
	else
	{
		const tiny_string& del=asAtomHandler::toString(args[0],sys);
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
				res->push(asAtomHandler::fromObject(abstract_s(sys, tiny_string::fromChar(*i) ) ));
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
			ASObject* s=abstract_s(sys,data.substr(start,(match-start)));
			if (res->size() >= limit)
				break;
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
	tiny_string data = asAtomHandler::toString(obj,sys);
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
	if(start<0) {
		start=data.numChars()+start;
		if(start<0)
			start=0;
	}
	if(start>(int)data.numChars())
		start=data.numChars();

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
	}
	ret = asAtomHandler::fromObject(abstract_s(sys,data.substr(start,len)));
}

ASFUNCTIONBODY_ATOM(ASString,substring)
{
	tiny_string data = asAtomHandler::toString(obj,sys);

	number_t start, end;
	ARG_UNPACK_ATOM (start,0) (end,0x7fffffff);
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

	ret = asAtomHandler::fromObject(abstract_s(sys,data.substr(start,end-start)));
}

number_t ASString::toNumber()
{
	assert_and_throw(implEnable);

	const char *s = getData().raw_buf();
	while (*s && isEcmaSpace(g_utf8_get_char(s)))
		s = g_utf8_next_char(s);

	double val;
	char *end = NULL;
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
	return Integer::stringToASInteger(getData().raw_buf(), 0,true);
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
	r->toPrimitive(rprim);
	if(getObjectType()==T_STRING && asAtomHandler::isString(rprim))
	{
		ASString* rstr=static_cast<ASString*>(asAtomHandler::toObject(rprim,getSystemState()));
		return (getData()<rstr->getData())?TTRUE:TFALSE;
	}
	number_t a=toNumber();
	number_t b=asAtomHandler::toNumber(rprim);
	if(std::isnan(a) || std::isnan(b))
		return TUNDEFINED;
	return (a<b)?TTRUE:TFALSE;
}

TRISTATE ASString::isLessAtom(asAtom& r)
{
	//ECMA-262 11.8.5 algorithm
	assert_and_throw(implEnable);
	asAtom rprim = r;
	if (asAtomHandler::getObject(r))
		asAtomHandler::getObject(r)->toPrimitive(rprim);
	if(getObjectType()==T_STRING && asAtomHandler::isString(rprim))
	{
		ASString* rstr=static_cast<ASString*>(asAtomHandler::toObject(rprim,getSystemState()));
		return (getData()<rstr->getData())?TTRUE:TFALSE;
	}
	number_t a=toNumber();
	number_t b=asAtomHandler::toNumber(rprim);
	if(std::isnan(a) || std::isnan(b))
		return TUNDEFINED;
	return (a<b)?TTRUE:TFALSE;
}

void ASString::serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap)
{
	if (out->getObjectEncoding() == ObjectEncoding::AMF0)
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

ASFUNCTIONBODY_ATOM(ASString,slice)
{
	tiny_string data = asAtomHandler::toString(obj,sys);
	int startIndex=0;
	if(argslen>=1)
		startIndex=asAtomHandler::toInt(args[0]);
	if(startIndex<0) {
		startIndex=data.numChars()+startIndex;
		if(startIndex<0)
			startIndex=0;
	}
	if(startIndex>(int)data.numChars())
		startIndex=data.numChars();

	int endIndex=0x7fffffff;
	if(argslen>=2)
		endIndex=asAtomHandler::toInt(args[1]);
	if(endIndex<0) {
		endIndex=data.numChars()+endIndex;
		if(endIndex<0)
			endIndex=0;
	}
	if(endIndex>(int)data.numChars())
		endIndex=data.numChars();
	if(endIndex<=startIndex)
		ret = asAtomHandler::fromStringID(BUILTIN_STRINGS::EMPTY);
	else
		ret = asAtomHandler::fromObject(abstract_s(sys,data.substr(startIndex,endIndex-startIndex)));
}
ASFUNCTIONBODY_ATOM(ASString,charAt)
{
	number_t index;
	ARG_UNPACK_ATOM (index, 0);

	// fast path if obj is ASString
	if (asAtomHandler::is<ASString>(obj) && asAtomHandler::getObject(obj))
	{
		ASString* th = asAtomHandler::as<ASString>(obj);
		int maxIndex=th->getData().numChars();
		
		if(index<0 || index>=maxIndex || std::isinf(index))
		{
			ret = asAtomHandler::fromStringID(BUILTIN_STRINGS::EMPTY);
			return;
		}
		uint32_t c;
		if (th->getData().isSinglebyte())
			c = th->getData().charAt(index);
		else
		{
			if (index == 0)
			{
				th->currentpos = th->getData().begin();
				th->currentindex = 0;
				c = *(th->currentpos);
			}
			else if (th->currentpos.isValid() && th->currentindex == index-1)
			{
				th->currentindex++;
				c = *(++th->currentpos);
			}
			else
				c = th->getData().charAt(index);
		}
		ret = c < BUILTIN_STRINGS_CHAR_MAX ? asAtomHandler::fromStringID(c) : asAtomHandler::fromObject(abstract_s(sys, tiny_string::fromChar(c) ));
		return;
	}
	tiny_string data = asAtomHandler::toString(obj,sys);
	int maxIndex=data.numChars();
	if(index<0 || index>=maxIndex || std::isinf(index))
		ret = asAtomHandler::fromStringID(BUILTIN_STRINGS::EMPTY);
	else
	{
		uint32_t c = data.charAt(index);
		ret = c < BUILTIN_STRINGS_CHAR_MAX ? asAtomHandler::fromStringID(c) : asAtomHandler::fromObject(abstract_s(sys, tiny_string::fromChar(c) ));
	}
}

ASFUNCTIONBODY_ATOM(ASString,charCodeAt)
{
	int64_t index;
	
	ARG_UNPACK_ATOM (index, 0);

	// fast path if obj is ASString
	if (asAtomHandler::isStringID(obj))
	{
		tiny_string s = sys->getStringFromUniqueId(asAtomHandler::getStringId(obj));
		if(index<0 || index>=(int64_t)s.numChars())
			asAtomHandler::setNumber(ret,sys,Number::NaN);
		else
			asAtomHandler::setInt(ret,sys,(int32_t)s.charAt(index));
	}
	else if (asAtomHandler::isString(obj) && asAtomHandler::getObject(obj))
	{
		ASString* th = asAtomHandler::as<ASString>(obj);
		if(index<0 || index>=(int64_t)th->getData().numChars())
			asAtomHandler::setNumber(ret,sys,Number::NaN);
		else
		{
			uint32_t c;
			if (th->getData().isSinglebyte())
				c = th->getData().charAt(index);
			else
			{
				if (index == 0)
				{
					th->currentpos = th->getData().begin();
					th->currentindex = 0;
					c = *(th->currentpos);
				}
				else if (th->currentpos.isValid() && th->currentindex == index-1)
				{
					th->currentindex++;
					c = *(++th->currentpos);
				}
				else
					c = th->getData().charAt(index);
			}
			asAtomHandler::setInt(ret,sys,(int32_t)c);
		}
	}
	else
	{
		tiny_string data = asAtomHandler::toString(obj,sys);
		if(index<0 || index>=(int64_t)data.numChars())
			asAtomHandler::setNumber(ret,sys,Number::NaN);
		else
		{
			//Character codes are expected to be positive
			asAtomHandler::setInt(ret,sys,(int32_t)data.charAt(index));
		}
	}
}

ASFUNCTIONBODY_ATOM(ASString,indexOf)
{
	if (argslen == 0)
	{
		asAtomHandler::setInt(ret,sys,-1);
		return;
	}
	tiny_string data = asAtomHandler::toString(obj,sys);
	tiny_string arg0=asAtomHandler::toString(args[0],sys);
	int startIndex=0;
	if(argslen>1)
		startIndex=asAtomHandler::toInt(args[1]);
	startIndex = imin(imax(startIndex, 0), data.numChars());

	size_t pos = data.find(arg0.raw_buf(), startIndex);
	if(pos == data.npos)
		asAtomHandler::setInt(ret,sys,-1);
	else
		asAtomHandler::setInt(ret,sys,(int32_t)pos);
}

ASFUNCTIONBODY_ATOM(ASString,lastIndexOf)
{
	assert_and_throw(argslen==1 || argslen==2);
	tiny_string data = asAtomHandler::toString(obj,sys);
	tiny_string val=asAtomHandler::toString(args[0],sys);
	size_t startIndex=data.npos;
	if(argslen > 1 && !asAtomHandler::isUndefined(args[1]) && !std::isnan(asAtomHandler::toNumber(args[1])) && !(asAtomHandler::toNumber(args[1]) > 0 && std::isinf(asAtomHandler::toNumber(args[1]))))
	{
		int32_t i = asAtomHandler::toInt(args[1]);
		if(i<0)
		{
			asAtomHandler::setInt(ret,sys,-1);
			return;
		}
		startIndex = i;
	}

	startIndex = imin(startIndex, data.numChars());

	size_t pos=data.rfind(val.raw_buf(), startIndex);
	if(pos==data.npos)
		asAtomHandler::setInt(ret,sys,-1);
	else
		asAtomHandler::setInt(ret,sys,(int32_t)pos);
}

ASFUNCTIONBODY_ATOM(ASString,toLowerCase)
{
	tiny_string data = asAtomHandler::toString(obj,sys);
	if (asAtomHandler::is<ASString>(obj) && asAtomHandler::toStringId(obj,sys) != UINT32_MAX)
		ret = asAtomHandler::fromStringID(sys->getUniqueStringId(data.lowercase()));
	else
		ret = asAtomHandler::fromObject(abstract_s(sys,data.lowercase()));
}

ASFUNCTIONBODY_ATOM(ASString,toUpperCase)
{
	tiny_string data = asAtomHandler::toString(obj,sys);
	if (asAtomHandler::is<ASString>(obj) && asAtomHandler::toStringId(obj,sys) != UINT32_MAX)
		ret = asAtomHandler::fromStringID(sys->getUniqueStringId(data.uppercase()));
	else
		ret = asAtomHandler::fromObject(abstract_s(sys,data.uppercase()));
}
ASFUNCTIONBODY_ATOM(ASString,localeCompare)
{
	tiny_string data = asAtomHandler::toString(obj,sys);
	tiny_string other;
	ARG_UNPACK_ATOM_MORE_ALLOWED(other);
	if (argslen > 1)
		LOG(LOG_NOT_IMPLEMENTED,"localeCompare with more than one parameter not implemented");
	if (sys->getSwfVersion() < 11)
	{
		if (asAtomHandler::is<Null>(args[0]) || asAtomHandler::is<Undefined>(args[0]))
		{
			asAtomHandler::setInt(ret,sys,data == "" ? 1 : 0);
			return;
		}
	}
	int res = data.compare(other);
	asAtomHandler::setInt(ret,sys,res);
}
ASFUNCTIONBODY_ATOM(ASString,localeCompare_prototype)
{
	tiny_string data = asAtomHandler::toString(obj,sys);
	tiny_string other;
	ARG_UNPACK_ATOM_MORE_ALLOWED(other);
	if (argslen > 1)
		throwError<ArgumentError>(kWrongArgumentCountError, "localeCompare", "1",Integer::toString(argslen));

	if (sys->getSwfVersion() < 11)
	{
		if (asAtomHandler::is<Null>(args[0]) || asAtomHandler::is<Undefined>(args[0]))
		{
			asAtomHandler::setInt(ret,sys,data == "" ? 1 : 0);
			return;
		}
	}
	int res = data.compare(other);
	asAtomHandler::setInt(ret,sys,res);
}

ASFUNCTIONBODY_ATOM(ASString,fromCharCode)
{
	if (argslen == 1 && ((asAtomHandler::toUInt(args[0])& 0xFFFF) != 0))
	{
		ret = asAtomHandler::fromStringID(asAtomHandler::toUInt(args[0])& 0xFFFF);
		return;
	}
	ASString* res=abstract_s(sys)->as<ASString>();
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
	tiny_string data = asAtomHandler::toString(obj,sys);
	enum REPLACE_TYPE { STRING=0, FUNC };
	REPLACE_TYPE type;
	ASString* res=abstract_s(sys,data)->as<ASString>();

	tiny_string replaceWith;
	if(argslen < 2)
	{
		type = STRING;
		replaceWith="";
	}
	else if(!asAtomHandler::isFunction(args[1]))
	{
		type = STRING;
		replaceWith=asAtomHandler::toString(args[1],sys);
	}
	else
	{
		replaceWith=asAtomHandler::toString(args[1],sys);
		type = FUNC;
	}
	if(asAtomHandler::is<RegExp>(args[0]))
	{
		RegExp* re=asAtomHandler::as<RegExp>(args[0]);

		pcre* pcreRE = re->compile();
		if (!pcreRE)
		{
			ret = asAtomHandler::fromObject(res);
			return;
		}

		int capturingGroups;
		int infoOk=pcre_fullinfo(pcreRE, NULL, PCRE_INFO_CAPTURECOUNT, &capturingGroups);
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
			int rc=pcre_exec(pcreRE, &extra, res->getData().raw_buf(), res->getData().numBytes(), offset, 0, ovector, (capturingGroups+1)*3);
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
				asAtom* subargs = g_newa(asAtom, 3+capturingGroups);
				//we index on bytes, not on UTF-8 characters
				subargs[0]=asAtomHandler::fromObject(abstract_s(sys,res->data.substr_bytes(ovector[0],ovector[1]-ovector[0])));
				for(int i=0;i<capturingGroups;i++)
					subargs[i+1]=asAtomHandler::fromObject(abstract_s(sys,res->data.substr_bytes(ovector[i*2+2],ovector[i*2+3]-ovector[i*2+2])));
				subargs[capturingGroups+1]=asAtomHandler::fromInt((int32_t)(ovector[0]-retDiff));
				
				subargs[capturingGroups+2]=asAtomHandler::fromObject(abstract_s(sys,data));
				asAtom ret=asAtomHandler::invalidAtom;
				asAtom obj = asAtomHandler::nullAtom;
				if (asAtomHandler::as<IFunction>(args[1])->closure_this) // use closure as "this" in function call
					obj = asAtomHandler::fromObject(asAtomHandler::as<IFunction>(args[1])->closure_this.getPtr());
				asAtomHandler::callFunction(args[1],ret,obj, subargs, 3+capturingGroups,true);
				replaceWithTmp=asAtomHandler::toString(ret,sys).raw_buf();
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
							if (j <0 || j> 9 || rc < 10*i + j)
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
		const tiny_string& s=asAtomHandler::toString(args[0],sys);
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
	tiny_string data = asAtomHandler::toString(obj,sys);
	ASString* res=abstract_s(sys,data)->as<ASString>();
	for(unsigned int i=0;i<argslen;i++)
	{
		res->hasId = false;
		res->getData()+=asAtomHandler::toString(args[i],sys).raw_buf();
	}

	ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(ASString,generator)
{
	assert(argslen<=1);
	if (argslen == 0)
		ret = asAtomHandler::fromStringID(BUILTIN_STRINGS::EMPTY);
	else
		ret = asAtomHandler::fromObject(abstract_s(sys,asAtomHandler::toString(args[0],sys)));
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
