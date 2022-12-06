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

#include <cmath>
#include "parsing/amf3_generator.h"
#include "scripting/argconv.h"
#include "scripting/toplevel/Integer.h"
#include "scripting/toplevel/Number.h"
#include "scripting/toplevel/UInteger.h"
#include "scripting/flash/utils/ByteArray.h"

using namespace std;
using namespace lightspark;

ASFUNCTIONBODY_ATOM(Integer,_toString)
{
	if(Class<Integer>::getClass(wrk->getSystemState())->prototype->getObj() == asAtomHandler::getObject(obj))
	{
		ret = asAtomHandler::fromString(wrk->getSystemState(),"0");
		return;
	}

	int radix=10;
	if(argslen==1)
		radix=asAtomHandler::toUInt(args[0]);

	if(radix==10)
	{
		char buf[20];
		snprintf(buf,20,"%i",asAtomHandler::toInt(obj));
		ret = asAtomHandler::fromObject(abstract_s(wrk,buf));
	}
	else
	{
		tiny_string s=Number::toStringRadix(asAtomHandler::toNumber(obj), radix);
		ret = asAtomHandler::fromObject(abstract_s(wrk,s));
	}
}

ASFUNCTIONBODY_ATOM(Integer,_valueOf)
{
	if(Class<Integer>::getClass(wrk->getSystemState())->prototype->getObj() == asAtomHandler::getObject(obj))
	{
		asAtomHandler::setInt(ret,wrk,0);
		return;
	}

	if(!asAtomHandler::isInteger(obj))
	{
		createError<TypeError>(wrk,0,"");
		return;
	}

	ASATOM_INCREF(obj);
	ret = obj;
}

ASFUNCTIONBODY_ATOM(Integer,_constructor)
{
	Integer* th=asAtomHandler::as<Integer>(obj);
	if(argslen==0)
	{
		//The int is already initialized to 0
		return;
	}
	th->val=asAtomHandler::toInt(args[0]);
}

ASFUNCTIONBODY_ATOM(Integer,generator)
{
	if (argslen == 0)
		asAtomHandler::setInt(ret,wrk,(int32_t)0);
	else
		asAtomHandler::setInt(ret,wrk,asAtomHandler::toInt(args[0]));
}

TRISTATE Integer::isLess(ASObject* o)
{
	switch(o->getObjectType())
	{
		case T_INTEGER:
			{
				Integer* i=static_cast<Integer*>(o);
				return (val < i->toInt())?TTRUE:TFALSE;
			}
			break;

		case T_UINTEGER:
			{
				UInteger* i=static_cast<UInteger*>(o);
				return (val < 0 || ((uint32_t)val)  < i->val)?TTRUE:TFALSE;
			}
			break;
		
		case T_NUMBER:
			{
				Number* i=static_cast<Number*>(o);
				if(std::isnan(i->toNumber())) return TUNDEFINED;
				return (val < i->toNumber())?TTRUE:TFALSE;
			}
			break;
		
		case T_STRING:
			{
				double val2=o->toNumber();
				if(std::isnan(val2)) return TUNDEFINED;
				return (val<val2)?TTRUE:TFALSE;
			}
			break;
		
		case T_BOOLEAN:
			{
				Boolean* i=static_cast<Boolean*>(o);
				return (val < i->toInt())?TTRUE:TFALSE;
			}
			break;
		
		case T_UNDEFINED:
			{
				return TUNDEFINED;
			}
			break;
			
		case T_NULL:
			{
				return (val < 0)?TTRUE:TFALSE;
			}
			break;

		default:
			break;
	}
	
	asAtom val2p=asAtomHandler::invalidAtom;
	bool isrefcounted;
	o->toPrimitive(val2p,isrefcounted);
	double val2=asAtomHandler::toNumber(val2p);
	if (isrefcounted)
		ASATOM_DECREF(val2p);
	if(std::isnan(val2)) return TUNDEFINED;
	return (val<val2)?TTRUE:TFALSE;
}

TRISTATE Integer::isLessAtom(asAtom& r)
{
	switch(asAtomHandler::getObjectType(r))
	{
		case T_INTEGER:
			return (val < asAtomHandler::toInt(r))?TTRUE:TFALSE;
		case T_UINTEGER:
			return (val < 0 || ((uint32_t)val)  < asAtomHandler::toUInt(r))?TTRUE:TFALSE;
		
		case T_NUMBER:
			if(std::isnan(asAtomHandler::toNumber(r))) return TUNDEFINED;
			return (val < asAtomHandler::toNumber(r))?TTRUE:TFALSE;
		case T_STRING:
			if(std::isnan(asAtomHandler::toNumber(r))) return TUNDEFINED;
			return (val<asAtomHandler::toNumber(r))?TTRUE:TFALSE;
			break;
		case T_BOOLEAN:
			return (val < asAtomHandler::toInt(r))?TTRUE:TFALSE;
		case T_UNDEFINED:
			return TUNDEFINED;
		case T_NULL:
			return (val < 0)?TTRUE:TFALSE;
		default:
			break;
	}
	
	asAtom val2p=asAtomHandler::invalidAtom;
	bool isrefcounted;
	asAtomHandler::getObject(r)->toPrimitive(val2p,isrefcounted);
	double val2=asAtomHandler::toNumber(val2p);
	if (isrefcounted)
		ASATOM_DECREF(val2p);
	if(std::isnan(val2)) return TUNDEFINED;
	return (val<val2)?TTRUE:TFALSE;
}

bool Integer::isEqual(ASObject* o)
{
	switch(o->getObjectType())
	{
		case T_INTEGER:
			return val==o->toInt();
		case T_UINTEGER:
			return val >= 0 && val==o->toInt();
		case T_NUMBER:
			return val==o->toNumber();
		case T_BOOLEAN:
			return val==o->toInt();
		case T_STRING:
			return val==o->toNumber();
		case T_NULL:
		case T_UNDEFINED:
			return false;
		default:
			return o->isEqual(this);
	}
}

tiny_string Integer::toString() const
{
	return Integer::toString(val);
}

/* static helper function */
tiny_string Integer::toString(int32_t val)
{
	char buf[20];
	if(val<0)
	{
		//This can be a slow path, as it not used for array access
		snprintf(buf,20,"%i",val);
		return tiny_string(buf,true);
	}
	buf[19]=0;
	char* cur=buf+19;

	int v=val;
	do
	{
		cur--;
		*cur='0'+(v%10);
		v/=10;
	}
	while(v!=0);
	return tiny_string(cur,true); //Create a copy
}

void Integer::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED | CLASS_FINAL);
	c->isReusable = true;
	c->setVariableAtomByQName("MAX_VALUE",nsNameAndKind(),asAtomHandler::fromInt(numeric_limits<int32_t>::max()),CONSTANT_TRAIT);
	c->setVariableAtomByQName("MIN_VALUE",nsNameAndKind(),asAtomHandler::fromInt(numeric_limits<int32_t>::min()),CONSTANT_TRAIT);
	c->setDeclaredMethodByQName("toString",AS3,Class<IFunction>::getFunction(c->getSystemState(),_toString,1,Class<ASString>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("toFixed",AS3,Class<IFunction>::getFunction(c->getSystemState(),_toFixed,1,Class<ASString>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("toExponential",AS3,Class<IFunction>::getFunction(c->getSystemState(),_toExponential,1,Class<ASString>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("toPrecision",AS3,Class<IFunction>::getFunction(c->getSystemState(),_toPrecision,1,Class<ASString>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("valueOf",AS3,Class<IFunction>::getFunction(c->getSystemState(),_valueOf,0,Class<Integer>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->prototype->setVariableByQName("toExponential","",Class<IFunction>::getFunction(c->getSystemState(),Integer::_toExponential,1,Class<ASString>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("toFixed","",Class<IFunction>::getFunction(c->getSystemState(),Integer::_toFixed,1,Class<ASString>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("toPrecision","",Class<IFunction>::getFunction(c->getSystemState(),Integer::_toPrecision,1,Class<ASString>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("toString","",Class<IFunction>::getFunction(c->getSystemState(),Integer::_toString,1,Class<ASString>::getRef(c->getSystemState()).getPtr()),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("valueOf","",Class<IFunction>::getFunction(c->getSystemState(),_valueOf,1,Class<Integer>::getRef(c->getSystemState()).getPtr()),DYNAMIC_TRAIT);
}

void Integer::serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap,ASWorker* wrk)
{
	serializeValue(out,val);
}

void Integer::serializeValue(ByteArray* out, int32_t val)
{
	if (out->getObjectEncoding() == OBJECT_ENCODING::AMF0)
	{
		// write as double
		out->writeByte(amf0_number_marker);
		out->serializeDouble(val);
		return;
	}
	if(val>=0x40000000 || val<=(int32_t)0xbfffffff)
	{
		// write as double
		out->writeByte(double_marker);
		out->serializeDouble(val);
	}
	else
	{
		out->writeByte(integer_marker);
		out->writeU29((uint32_t)val);
	}
}

int32_t parseIntDigit(char ch)
{
	if (ch >= '0' && ch <= '9') {
		return (ch - '0');
	} else if (ch >= 'a' && ch <= 'z') {
		return (ch - 'a' + 10);
	} else if (ch >= 'A' && ch <= 'Z') {
		return (ch - 'A' + 10);
	} else {
		return -1;
	}
}
// algorithm taken from avmplus:
bool parseIntECMA262(number_t& result, tiny_string& s, int32_t radix,bool negate, bool strict )
{
	bool gotDigits = false;
	result = 0;
	uint32_t index = 0;

	// Make sure radix is valid, and we have digits
	if (radix >= 2 && radix <= 36 && index < s.numChars()) {
		result = 0;
		int32_t start = index;
		const char* cur = s.raw_buf();

		// Read the digits, generate result
		while (index < s.numChars()) {
			int32_t v = parseIntDigit(g_utf8_get_char(cur));
			if (v == -1 || v >= radix) {
				break;
			}
			result = result * radix + v;
			gotDigits = true;
			index++;
			cur = g_utf8_next_char(cur);
		}

		while(ASString::isEcmaSpace(g_utf8_get_char(cur))) // trailing whitespace is valid.
		{
			cur = g_utf8_next_char(cur);
			index++;
		}
		if (strict && index < s.numChars()) {
			result = Number::NaN;
			return false;
		}

		if ( result >= 0x20000000000000LL &&  // i.e. if the result may need at least 54 bits of mantissa
				(radix == 2 || radix == 4 || radix == 8 || radix == 16 || radix == 32) )  {

			// CN:  we're here because we may have incurred roundoff error with the above.
			//  Error will creep in once we need more than the available 53 bits
			//  of precision in the mantissa portion of a double.  No way to deduce
			//  this from the result, so we have to recalculate it more slowly.
			result = 0;

			int32_t powOf2 = 1;
			for(int32_t x = radix; (x != 1); x >>= 1)
				powOf2++;
			powOf2--; // each word contains one less than this # of bits.
			index = start;
			int32_t v=0;

			uint32_t end,next;
			// skip leading zeros
			for(end=index; end < s.numChars() && s.charAt(end) == '0'; end++)
				;
			if (end >= s.numChars())
			{
				result = 0;
				return false;
			}
			for (next=0; next*powOf2 <= 52; next++) { // read first 52 non-zero digits.  Once charPosition*log2(radix) is > 53, we can have rounding issues
				v = parseIntDigit(s.charAt(end++));
				if (v == -1 || v >= radix) {
					v = 0;
					break;
				}
				result = result * radix + v;
				if (end >= s.numChars())
					break;
			}
			if (next*powOf2 > 52) { // If number contains more than 53 bits of precision, may need to roundUp last digit processed.
				bool roundUp = false;
				int32_t bit53 = 0;
				int32_t bit54 = 0;

				double factor = 1;

				switch(radix) {
				case 32:  // last word read contained digits 51,52,53,54,55
					bit53 = v & (1 << 2);
					bit54 = v & (1 << 1);
					roundUp = (v & 1);
					break;
				case 16:  // last word read contained digits 50,51,52,53
					bit53 = v & (1 << 0);
					v = parseIntDigit(s.charAt(end));
					if (v != -1 && v < radix) {
						factor *= radix;
						bit54 = v & (1 << 3);
						roundUp = (v & 0x3) != 0;  // check if any bit after bit54 is set
					} else {
						roundUp = bit53 != 0;
					}
					break;
				case 8: // last work read contained digits 49,50,51, next word contains 52,53,54
					v = parseIntDigit(s.charAt(end));
					if (v == -1 || v >= radix) {
						v = 0;
					}
					factor *= radix;
					bit53 = v & (1 << 1);
					bit54 = v & (1 << 0);
					break;
				case 4: // 51,52 - 53,54
					v = parseIntDigit(s.charAt(end));
					if (v == -1 || v >= radix) {
						v = 0;
					}
					factor *= radix;
					bit53 = v & (1 << 1);
					bit54 = v & (1 << 0);
					break;
				case 2: // 52 - 53 - 54
					/*
					v = parseIntDigit(s[end++]);
					result = result * radix;  // add factor before round-off adjustment for 52 bit
					*/
					bit53 = v & (1 << 0);
					v = parseIntDigit(s.charAt(end));
					if (v != -1 && v < radix) {
						factor *= radix;
						bit54 = (v != -1 ? (v & (1 << 0)) : 0); // Might be there are only 53 digits.
					}

					break;
				}

				bit53 = !!bit53;
				bit54 = !!bit54;


				while(++end < s.numChars()) {
					v = parseIntDigit(s.charAt(end));
					if (v == -1 || v >= radix) {
						break;
					}
					roundUp |= (v != 0); // any trailing positive bit causes us to round up
					factor *= radix;
				}
				roundUp = bit54 && (bit53 || roundUp);
				result += (roundUp ? 1.0 : 0.0);
				result *= factor;
			}

		}
		/*
		else if (radix == 10 && result >= 0x20000000000000)
		// if there are more than 15 digits, roundoff error may affect us.  Need to use exact integer rep instead of float
		//int32_t numDigits = len - (s - sStart);
		*/
		if (negate) {
			result = -result;
		}
	}
	if (!gotDigits)
		result = Number::NaN;
	return gotDigits;
}


bool Integer::fromStringFlashCompatible(const char* cur, number_t& ret, int radix,bool strict)
{
	//Skip whitespace chars
	while(ASString::isEcmaSpace(g_utf8_get_char(cur)))
		cur = g_utf8_next_char(cur);

	bool negate=false;
	//Skip and take note of plus/minus sign
	if(*cur=='+')
		cur++;
	else if(*cur=='-')
	{
		negate=true;
		cur++;
	}
	if (radix == 0 || radix==16)
	{
		if (g_str_has_prefix(cur,"0x") || g_str_has_prefix(cur,"0X"))
		{
			radix = 16;
			cur+=2;
		}
		else if (radix == 0)
			radix = 10;
	}
	//Skip leading zeroes
	if (radix == 0)
	{
		int count=0;
		while(*cur=='0')
		{
			cur++;
			count++;
		}

		//The string consisted of all zeroes
		if(count>0 && *cur=='\0')
		{
			ret = 0;
			return true;
		}
	}
	
	tiny_string s(cur);
	return parseIntECMA262(ret, s,radix,negate,strict);
}

int32_t Integer::stringToASInteger(const char* cur, int radix, bool strict, bool* isValid)
{
	number_t value;
	bool valid=Integer::fromStringFlashCompatible(cur, value, radix,strict);
	if (isValid)
		*isValid=valid;
	if (!valid)
		return 0;
	else
		return (int32_t)(int64_t)value;
}

ASFUNCTIONBODY_ATOM(Integer,_toExponential)
{
	double v = asAtomHandler::toNumber(obj);
	int32_t fractionDigits;
	ARG_CHECK(ARG_UNPACK(fractionDigits, 0));
	if (argslen == 0 || asAtomHandler::is<Undefined>(args[0]))
	{
		if (v == 0)
			fractionDigits = 1;
		else
			fractionDigits = imin(imax((int32_t)ceil(::log10(::fabs(v))), 1), 20);
	}
	ret = asAtomHandler::fromObject(abstract_s(wrk,Number::toExponentialString(v, fractionDigits)));
}

ASFUNCTIONBODY_ATOM(Integer,_toFixed)
{
	int fractiondigits;
	ARG_CHECK(ARG_UNPACK (fractiondigits, 0));
	ret = asAtomHandler::fromObject(abstract_s(wrk,Number::toFixedString(asAtomHandler::toNumber(obj), fractiondigits)));
}

ASFUNCTIONBODY_ATOM(Integer,_toPrecision)
{
	if (argslen == 0 || asAtomHandler::is<Undefined>(args[0]))
	{
		ret = asAtomHandler::fromObject(abstract_s(wrk,asAtomHandler::toString(obj,wrk)));
		return;
	}
	int precision;
	ARG_CHECK(ARG_UNPACK (precision));
	ret = asAtomHandler::fromObject(abstract_s(wrk,Number::toPrecisionString(asAtomHandler::toNumber(obj), precision)));
}
