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

#include "parsing/amf3_generator.h"
#include "scripting/argconv.h"
#include "scripting/toplevel/Number.h"
#include "scripting/toplevel/Integer.h"
#include "scripting/toplevel/UInteger.h"
#include "scripting/toplevel/Math.h"
#include "scripting/flash/utils/ByteArray.h"
#include "3rdparty/avmplus/core/d2a.h"
#include <iomanip>

using namespace std;
using namespace lightspark;

const number_t Number::NaN = numeric_limits<double>::quiet_NaN();

number_t ASObject::toNumber()
{
	switch(this->getObjectType())
	{
	case T_UNDEFINED:
		return Number::NaN;
	case T_NULL:
		return +0;
	case T_BOOLEAN:
		return as<Boolean>()->val ? 1 : 0;
	case T_NUMBER:
		return as<Number>()->toNumber();
	case T_INTEGER:
		return as<Integer>()->val;
	case T_UINTEGER:
		return as<UInteger>()->val;
	case T_STRING:
		return as<ASString>()->toNumber();
	default:
		{
			//everything else is an Object regarding to the spec
			asAtom val2p=asAtomHandler::invalidAtom;
			bool isrefcounted;
			toPrimitive(val2p,isrefcounted,NUMBER_HINT);
			number_t res = asAtomHandler::toNumber(val2p);
			if (isrefcounted)
				ASATOM_DECREF(val2p);
			return res;
		}
	}
}

bool Number::isEqual(ASObject* o)
{
	switch(o->getObjectType())
	{
		case T_INTEGER:
		case T_UINTEGER:
		case T_BOOLEAN:
			return isfloat ? toNumber()==o->toNumber() : toInt() == o->toInt();
		case T_NUMBER:
		case T_STRING:
			return toNumber()==o->toNumber();
		case T_NULL:
		case T_UNDEFINED:
			return false;
		default:
			return o->isEqual(this);
	}
}

TRISTATE Number::isLess(ASObject* o)
{
	if(isfloat && std::isnan(dval))
		return TUNDEFINED;
	switch(o->getObjectType())
	{
		case T_INTEGER:
			if(isfloat)
				return (dval<o->as<Integer>()->val)?TTRUE:TFALSE;
			else
				return (ival<o->as<Integer>()->val)?TTRUE:TFALSE;
		case T_UINTEGER:
			if(isfloat)
				return (dval<o->as<UInteger>()->val)?TTRUE:TFALSE;
			else
				return (ival<o->as<UInteger>()->val)?TTRUE:TFALSE;
		case T_NUMBER:
		{
			const Number* i=static_cast<const Number*>(o);
			if(i->isfloat)
			{
				if (std::isnan(i->dval)) return TUNDEFINED;
				return (toNumber()<i->dval)?TTRUE:TFALSE;
			}
			return (toInt64()<i->ival)?TTRUE:TFALSE;
		}
		case T_BOOLEAN:
			return (isfloat ? dval<o->toNumber() : ival < o->toInt())?TTRUE:TFALSE;
		case T_UNDEFINED:
			//Undefined is NaN, so the result is undefined
			return TUNDEFINED;
		case T_STRING:
		{
			double val2=o->toNumber();
			if(std::isnan(val2)) return TUNDEFINED;
			return (toNumber()<val2)?TTRUE:TFALSE;
		}
		case T_NULL:
			if(isfloat)
				return (dval<0)?TTRUE:TFALSE;
			return (ival<0)?TTRUE:TFALSE;
		default:
		{
			asAtom val2p=asAtomHandler::invalidAtom;
			bool isrefcounted;
			o->toPrimitive(val2p,isrefcounted,NUMBER_HINT);
			double val2=asAtomHandler::toNumber(val2p);
			if (isrefcounted)
				ASATOM_DECREF(val2p);
			if(std::isnan(val2)) return TUNDEFINED;
			return (toNumber()<val2)?TTRUE:TFALSE;
		}
	}
}

ASFUNCTIONBODY_ATOM(Number,_toString)
{
	if(Class<Number>::getClass(wrk->getSystemState())->prototype->getObj() == asAtomHandler::getObject(obj))
	{
		ret = asAtomHandler::fromString(wrk->getSystemState(),"0");
		return;
	}
	if(!asAtomHandler::isNumeric(obj))
	{
		createError<TypeError>(wrk,kInvokeOnIncompatibleObjectError, "Number.toString");
		return;
	}
	int radix=10;
	ARG_CHECK(ARG_UNPACK (radix,10));

	if((radix==10) || (asAtomHandler::is<Number>(obj) &&
					   (std::isnan(asAtomHandler::toNumber(obj)) ||
					   (std::isinf(asAtomHandler::toNumber(obj))))))
	{
		//see e 15.7.4.2
		ret = asAtomHandler::fromObject(abstract_s(wrk,asAtomHandler::toString(obj,wrk)));
	}
	else
	{
		ret = asAtomHandler::fromObject(abstract_s(wrk,Number::toStringRadix(asAtomHandler::toNumber(obj), radix)));
	}
}
ASFUNCTIONBODY_ATOM(Number,_toLocaleString)
{
	if(Class<Number>::getClass(wrk->getSystemState())->prototype->getObj() == asAtomHandler::getObject(obj))
	{
		ret = asAtomHandler::fromString(wrk->getSystemState(),"0");
		return;
	}
	if(!asAtomHandler::isNumeric(obj))
	{
		ret = asAtomHandler::fromString(wrk->getSystemState(),"0");
		return;
	}
	int radix=10;

	if((radix==10) || (asAtomHandler::is<Number>(obj) &&
					   (std::isnan(asAtomHandler::toNumber(obj)) ||
					   (std::isinf(asAtomHandler::toNumber(obj))))))
	{
		//see e 15.7.4.2
		ret = asAtomHandler::fromObject(abstract_s(wrk,asAtomHandler::toString(obj,wrk)));
	}
	else
	{
		ret = asAtomHandler::fromObject(abstract_s(wrk,Number::toStringRadix(asAtomHandler::toNumber(obj), radix)));
	}
}

ASFUNCTIONBODY_ATOM(Number,generator)
{
	if(argslen==0)
		wrk->setBuiltinCallResultLocalNumber(ret, 0.);
	else if (asAtomHandler::isUndefined(args[0]) && !wrk->needsActionScript3())
	{
		if (wrk->AVM1getSwfVersion() < 7)
			ret = asAtomHandler::fromInt(0);
		else
			ret = wrk->getSystemState()->nanAtom;
	}
	else
		wrk->setBuiltinCallResultLocalNumber(ret, asAtomHandler::toNumber(args[0]));
}

tiny_string Number::toString() const
{
	if (getInstanceWorker()->needsActionScript3())
		return Number::toString(isfloat ? dval : ival);
	else
		return Number::AVM1toString(isfloat ? dval : ival,10);
}

// AVM1 doubleToString taken from gnash
std::string Number::AVM1toString(double val, int radix)
{
	// Examples:
	//
	// e.g. for 9*.1234567890123456789:
	// 9999.12345678901
	// 99999.123456789
	// 999999.123456789
	// 9999999.12345679
	// [...]
	// 999999999999.123
	// 9999999999999.12
	// 99999999999999.1
	// 999999999999999
	// 1e+16
	// 1e+17
	//
	// For 1*.111111111111111111111111111111111111:
	// 1111111111111.11
	// 11111111111111.1
	// 111111111111111
	// 1.11111111111111e+15
	// 1.11111111111111e+16
	//
	// For 1.234567890123456789 * 10^-i:
	// 1.23456789012346
	// 0.123456789012346
	// 0.0123456789012346
	// 0.00123456789012346
	// 0.000123456789012346
	// 0.0000123456789012346
	// 0.00000123456789012346
	// 1.23456789012346e-6
	// 1.23456789012346e-7

	// Handle non-numeric values.
	if (std::isnan(val)) return "NaN";

	if (std::isinf(val)) return val < 0 ? "-Infinity" : "Infinity";

	if (val == 0.0 || val == -0.0) return "0";

	std::ostringstream ostr;

	if (radix == 10) {

		// ActionScript always expects dot as decimal point.
		ostr.imbue(std::locale::classic());

		// force to decimal notation for this range (because the
		// reference player does)
		if (std::abs(val) < 0.0001 && std::abs(val) >= 0.00001) {

			// All nineteen digits (4 zeros + up to 15 significant digits)
			ostr << std::fixed << std::setprecision(19) << val;

			std::string str = ostr.str();

			// Because 'fixed' also adds trailing zeros, remove them.
			std::string::size_type pos = str.find_last_not_of('0');
			if (pos != std::string::npos) {
				str.erase(pos + 1);
			}
			return str;
		}

		ostr << std::setprecision(15) << val;

		std::string str = ostr.str();

		// Remove a leading zero from 2-digit exponent if any
		std::string::size_type pos = str.find("e", 0);

		if (pos != std::string::npos && str.at(pos + 2) == '0') {
			str.erase(pos + 2, 1);
		}

		return str;
	}

	// Radix isn't 10
	bool negative = (val < 0);
	if (negative) val = -val;

	double left = std::floor(val);
	if (left < 1) return "0";

	std::string str;
	const std::string digits = "0123456789abcdefghijklmnopqrstuvwxyz";

	// Construct the string backwards for speed, then reverse.
	while (left) {
		double n = left;
		left = std::floor(left / radix);
		n -= (left * radix);
		str.push_back(digits[static_cast<int>(n)]);
	}
	if (negative) str.push_back('-');

	std::string strreversed;
	strreversed.reserve(str.size());
	for (auto it = str.rbegin(); it != str.rend();it++)
		strreversed.push_back(*it);
	return strreversed;

	// not available in c++14
	// std::reverse(str.begin(), str.end());
	//return str;
}

// doubletostring algorithm taken from https://github.com/adobe/avmplus/core/MathUtils.cpp
tiny_string Number::toString(number_t value, DTOSTRMODE mode, int32_t precision)
{
	if(std::isnan(value))
		return "NaN";
	if(std::isinf(value))
	{
		if(value > 0)
			return "Infinity";
		else
			return "-Infinity";
	}

	char buffer[500];
	if (mode == DTOSTR_NORMAL) {
		int32_t intValue = int32_t(value);
		if ((value == (double)(intValue)) && ((uint32_t)intValue != 0x80000000))
		{
			snprintf(buffer,40,"%d",(int)(value));
			return tiny_string(buffer,true);
		}
	}

	const bool negative = value < 0.0;
	const bool zero = value == 0.0;
	bool round = true;
	// Pointer to the next available character
	char *s = buffer;

	// Negate negative numbers and make space for the sign.  The sign
	// has to be placed just before we finish because we don't know yet
	// if it will be in buffer[0] or buffer[1].

	if (negative) {
		value = -value;
		s++;
	}

	// Set up the digit generator.
	avmplus::D2A d2a(value, mode != DTOSTR_NORMAL, precision);
	int32_t exp10 = d2a.expBase10()-1;     // Why "-1"?

	// Sentinel is used for rounding - points to the first digit
	char* const sentinel = s;

	// Format type selection.

	enum FormatType {
		kNormal,
		kExponential,
		kFraction,
		kFixedFraction
	};

	FormatType format;

	switch (mode) {
	case DTOSTR_FIXED:
		{
			if (exp10 < 0) {
				format = kFixedFraction;
			} else {
				format = kNormal;
				precision++;
			}
		}
		break;
	case DTOSTR_PRECISION:
		{
			// FIXME: Bugzilla 442974: This is simply not according to spec,
			// the canonical test case is Number.MIN_VALUE.toPrecision(21), which
			// formats as a 346-character string.
			if (exp10 < 0) {
				format = kFraction;
			} else if (exp10 >= precision) {
				format = kExponential;
			} else {
				format = kNormal;
			}
		}
		break;
	case DTOSTR_EXPONENTIAL:
		format = kExponential;
		precision++;
		break;
	default:
		if (exp10 < 0 && exp10 > -7) {
			// Number is of form 0.######
			if (exp10 < -precision) {
				exp10 = -precision-1;
			}
			format = kFraction;
		} else if (exp10 > 20) { // ECMA spec 9.8.1
			format = kExponential;
		} else {
			format = kNormal;
		}
	}

	// Digit generation.

	bool wroteDecimal = false;
	switch (format) {
	case kNormal:
		{
			int32_t digits = 0;
			int32_t digit;
			*s++ = '0';
			digit = d2a.nextDigit();
			if (digit > 0) {
				*s++ = (char)(digit + '0');
			}
			while (exp10 > 0) {
				digit = (d2a.finished) ? 0 : d2a.nextDigit();
				*s++ = (char)(digit + '0');
				exp10--;
				digits++;
			}
			if (mode == DTOSTR_FIXED) {
				digits = 0;
			}
			if (mode == DTOSTR_NORMAL) {
				if (!d2a.finished) {
					*s++ = '.';
					wroteDecimal = true;
					while( !d2a.finished ) {
						*s++ = (char)(d2a.nextDigit() + '0');
					}
				}
			}
			else if (digits < precision-1)
			{
				*s++ = '.';
				wroteDecimal = true;
				for (; digits < precision-1; digits++) {
					digit = d2a.finished ? 0 : d2a.nextDigit();
					*s++ = (char)(digit + '0');
				}
			}
		}
		break;
	case kFixedFraction:
		{
			*s++ = '0'; // Sentinel
			*s++ = '0';
			*s++ = '.';
			wroteDecimal = true;
			// Write out leading zeroes
			int32_t digits = 0;
			if (exp10 > 0) {
				while (++exp10 < 10 && digits < precision) {
					*s++ = '0';
					digits++;
				}
			} else if (exp10 < 0) {
				while ((++exp10 < 0) && (precision-- > 0))
					*s++ = '0';
			}

			// Write out significand
			for ( ; digits<precision; digits++) {
				if (d2a.finished)
				{
					if (mode == DTOSTR_NORMAL)
						break;
					*s++ = '0';
				} else {
					*s++ = (char)(d2a.nextDigit() + '0');
				}
			}
			exp10 = 0;
		}
		break;
	case kFraction:
		{
			*s++ = '0'; // Sentinel
			*s++ = '0';
			*s++ = '.';
			wroteDecimal = true;

			// Write out leading zeros
			if (!zero)
			{
				for (int32_t i=exp10; i<-1; i++) {
					*s++ = '0';
				}
			}

			// Write out significand
			int32_t i=0;
			while (!d2a.finished)
			{
				*s++ = (char)(d2a.nextDigit() + '0');
				if (mode != DTOSTR_NORMAL && ++i >= precision)
					break;
			}
			if (mode == DTOSTR_PRECISION)
			{
				while(i++ < precision)
					*s++ = (char)(d2a.finished ? '0' : d2a.nextDigit() + '0');
			}
			exp10 = 0;
		}
		break;
	case kExponential:
		{
			int32_t digit;
			digit = d2a.finished ? 0 : d2a.nextDigit();
			*s++ = (char)(digit + '0');
			if ( ((mode == DTOSTR_NORMAL) && !d2a.finished) ||
				 ((mode != DTOSTR_NORMAL) && precision > 1) ) {
				*s++ = '.';
				wroteDecimal = true;
				for (int32_t i=0; i<precision-1; i++) {
					if (d2a.finished)
					{
						if (mode == DTOSTR_NORMAL)
							break;
						*s++ = '0';
					} else {
						*s++ = (char)(d2a.nextDigit() + '0');
					}
				}
			}
		}
		break;
	}

	// Rounding and truncation.

	if (round && (d2a.bFastEstimateOk || mode == DTOSTR_FIXED || mode == DTOSTR_PRECISION))
	{
		if (d2a.nextDigit() > 4) {
			// Round the value up.
			
			for ( char* ptr=s-1 ; ptr >= sentinel ; ptr-- ) {
				// Skip the decimal point
				if (*ptr == '.')
					continue;
				
				// Carry in
				*ptr = *ptr + 1;

				// Done if there's no carry out
				if (*ptr != ('9'+1))
					break;
				
				// Carry out
				*ptr = '0';
			}
		}

		if (mode == DTOSTR_NORMAL && wroteDecimal) {
			// Remove trailing zeroes, and if necessary the decimal point.
			while (*(s-1) == '0') {
				s--;
			}
			if (*(s-1) == '.') {
				s--;
			}
		}
	}

	// Clean up zeroes, and place the exponent
	if (exp10) {

		// Handle the case where all digits ended up zero.
		// FIXME: Bugzilla 442974: This code is broken, it will find the decimal point.

		char *firstNonZero = sentinel;
		while (firstNonZero < s && *firstNonZero == '0') {
			firstNonZero++;
		}
		if (s == firstNonZero) {
			// FIXME: Bugzilla 442974:  This seems wrong, because it places the '1' at
			// the end of the string, not at the beginning.  Also it is arguably wrong
			// if the value is zero, and I don't have any proof that that can't happen.
			*s++ = '1';
			exp10++;
		}
		else if (!zero) {
			// Remove trailing zeroes, as might appear in 10e+95
			char *lastNonZero = s;
			while (lastNonZero > firstNonZero && *--lastNonZero == '0')
				;

			if (firstNonZero == lastNonZero) {
				exp10 += (int32_t) (s - firstNonZero - 1);  // What if exp10 is already the negative of that?
				s = lastNonZero+1;
			}
		}
		
		// Place the exponent
		*s++ = 'e';
		if (exp10 > 0) {
			*s++ = '+';
		}
		tiny_string e = Integer::toString(exp10);
		char* t = (char*)e.raw_buf();
		while (*t) { *s++ = *t++; }
	}

	int32_t len = (int32_t)(s-buffer);
	*s = 0;
	s = sentinel;
	
	// Deal with the sentinel introduced for the kFraction case: we might have a leading '00.'
	
	if (sentinel[0] == '0' && sentinel[1] != '.') {
		s = sentinel + 1;
		len--;
	}
	if (negative)
		*--s = '-';
	return tiny_string(s,true);
}

tiny_string Number::toStringRadix(number_t val, int radix)
{
	if((radix < 2) || (radix > 36))
	{
		createError<RangeError>(getWorker(),kInvalidRadixError, Integer::toString(radix));
		return "";
	}

	if(std::isnan(val) || std::isinf(val))
		return Number::toString(val);

	tiny_string res = "";
	static char digits[] ="0123456789abcdefghijklmnopqrstuvwxyz";
	number_t v = val;
	const number_t r = (number_t)radix;
	bool negative = v<0;
	if (negative) 
		v = -v;
	do 
	{
		res = tiny_string::fromChar(digits[(int)(v-(floor(v/r)*r))])+res;
		v = v/r;
	} 
	while (v >= 1.0);
	if (negative)
		res = tiny_string::fromChar('-')+res;
	return res;
}

void Number::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED | CLASS_FINAL);
	c->isReusable = true;
	c->setVariableAtomByQName("NEGATIVE_INFINITY",nsNameAndKind(),asAtomHandler::fromNumber(c->getInstanceWorker(),-numeric_limits<double>::infinity(),true),CONSTANT_TRAIT);
	c->setVariableAtomByQName("POSITIVE_INFINITY",nsNameAndKind(),asAtomHandler::fromNumber(c->getInstanceWorker(),numeric_limits<double>::infinity(),true),CONSTANT_TRAIT);
	c->setVariableAtomByQName("MAX_VALUE",nsNameAndKind(),asAtomHandler::fromNumber(c->getInstanceWorker(),numeric_limits<double>::max(),true),CONSTANT_TRAIT);
	c->setVariableAtomByQName("MIN_VALUE",nsNameAndKind(),asAtomHandler::fromNumber(c->getInstanceWorker(),numeric_limits<double>::min(),true),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NaN",nsNameAndKind(),asAtomHandler::fromNumber(c->getInstanceWorker(),numeric_limits<double>::quiet_NaN(),true),CONSTANT_TRAIT);
	c->setDeclaredMethodByQName("toString",AS3,c->getSystemState()->getBuiltinFunction(_toString,1,Class<ASString>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("toFixed",AS3,c->getSystemState()->getBuiltinFunction(toFixed,1,Class<ASString>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("toExponential",AS3,c->getSystemState()->getBuiltinFunction(toExponential,1,Class<ASString>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("toPrecision",AS3,c->getSystemState()->getBuiltinFunction(toPrecision,1,Class<ASString>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("valueOf",AS3,c->getSystemState()->getBuiltinFunction(_valueOf,0,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->prototype->setVariableByQName("toString","",c->getSystemState()->getBuiltinFunction(Number::_toString,1,Class<ASString>::getRef(c->getSystemState()).getPtr()),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("toLocaleString","",c->getSystemState()->getBuiltinFunction(Number::_toLocaleString,1,Class<ASString>::getRef(c->getSystemState()).getPtr()),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("toFixed","",c->getSystemState()->getBuiltinFunction(Number::toFixed,1,Class<ASString>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("toExponential","",c->getSystemState()->getBuiltinFunction(Number::toExponential,1,Class<ASString>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("toPrecision","",c->getSystemState()->getBuiltinFunction(Number::toPrecision,1,Class<ASString>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("valueOf","",c->getSystemState()->getBuiltinFunction(_valueOf,0,Class<Number>::getRef(c->getSystemState()).getPtr()),DYNAMIC_TRAIT);

	c->setVariableAtomByQName("E",nsNameAndKind(),asAtomHandler::fromNumber(c->getInstanceWorker(),2.71828182845905,true),CONSTANT_TRAIT,false);
	c->setVariableAtomByQName("LN10",nsNameAndKind(),asAtomHandler::fromNumber(c->getInstanceWorker(),2.302585092994046,true),CONSTANT_TRAIT,false);
	c->setVariableAtomByQName("LN2",nsNameAndKind(),asAtomHandler::fromNumber(c->getInstanceWorker(),0.6931471805599453,true),CONSTANT_TRAIT,false);
	c->setVariableAtomByQName("LOG10E",nsNameAndKind(),asAtomHandler::fromNumber(c->getInstanceWorker(),0.4342944819032518,true),CONSTANT_TRAIT,false);
	c->setVariableAtomByQName("LOG2E",nsNameAndKind(),asAtomHandler::fromNumber(c->getInstanceWorker(),1.442695040888963387,true),CONSTANT_TRAIT,false);
	c->setVariableAtomByQName("PI",nsNameAndKind(),asAtomHandler::fromNumber(c->getInstanceWorker(),3.141592653589793,true),CONSTANT_TRAIT,false);
	c->setVariableAtomByQName("SQRT1_2",nsNameAndKind(),asAtomHandler::fromNumber(c->getInstanceWorker(),0.7071067811865476,true),CONSTANT_TRAIT,false);
	c->setVariableAtomByQName("SQRT2",nsNameAndKind(),asAtomHandler::fromNumber(c->getInstanceWorker(),1.4142135623730951,true),CONSTANT_TRAIT,false);

	c->setDeclaredMethodByQName("abs","",c->getSystemState()->getBuiltinFunction(Math::abs,1,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false,false);
	c->setDeclaredMethodByQName("acos","",c->getSystemState()->getBuiltinFunction(Math::acos,1,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false,false);
	c->setDeclaredMethodByQName("asin","",c->getSystemState()->getBuiltinFunction(Math::asin,1,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false,false);
	c->setDeclaredMethodByQName("atan","",c->getSystemState()->getBuiltinFunction(Math::atan,1,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false,false);
	c->setDeclaredMethodByQName("atan2","",c->getSystemState()->getBuiltinFunction(Math::atan2,2,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false,false);
	c->setDeclaredMethodByQName("ceil","",c->getSystemState()->getBuiltinFunction(Math::ceil,1,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false,false);
	c->setDeclaredMethodByQName("cos","",c->getSystemState()->getBuiltinFunction(Math::cos,1,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false,false);
	c->setDeclaredMethodByQName("exp","",c->getSystemState()->getBuiltinFunction(Math::exp,1,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false,false);
	c->setDeclaredMethodByQName("floor","",c->getSystemState()->getBuiltinFunction(Math::floor,1,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false,false);
	c->setDeclaredMethodByQName("log","",c->getSystemState()->getBuiltinFunction(Math::log,1,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false,false);
	c->setDeclaredMethodByQName("max","",c->getSystemState()->getBuiltinFunction(Math::_max,2,Class<Number>::getRef(c->getSystemState()).getPtr(),Class<Integer>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false,false);
	c->setDeclaredMethodByQName("min","",c->getSystemState()->getBuiltinFunction(Math::_min,2,Class<Number>::getRef(c->getSystemState()).getPtr(),Class<Integer>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false,false);
	c->setDeclaredMethodByQName("pow","",c->getSystemState()->getBuiltinFunction(Math::pow,2,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false,false);
	c->setDeclaredMethodByQName("random","",c->getSystemState()->getBuiltinFunction(Math::random,0,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false,false);
	c->setDeclaredMethodByQName("round","",c->getSystemState()->getBuiltinFunction(Math::round,1,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false,false);
	c->setDeclaredMethodByQName("sin","",c->getSystemState()->getBuiltinFunction(Math::sin,1,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false,false);
	c->setDeclaredMethodByQName("sqrt","",c->getSystemState()->getBuiltinFunction(Math::sqrt,1,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false,false);
	c->setDeclaredMethodByQName("tan","",c->getSystemState()->getBuiltinFunction(Math::tan,1,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false,false);
}

string Number::toDebugString() const
{
	string ret = toString()+(isfloat ? "d" : "di");
#ifndef NDEBUG
	char buf[300];
	sprintf(buf,"(%p/%d/%d/%d)",this,this->getRefCount(),this->storedmembercount,this->getConstant());
	ret += buf;
#endif
	return ret;
}

ASFUNCTIONBODY_ATOM(Number,_constructor)
{
	Number* th=asAtomHandler::as<Number>(obj);
	if(argslen==0)
	{
		// not constructed Numbers are set to NaN, so we have to set it to the default value during dynamic construction
		if (std::isnan(th->dval))
		{
			th->ival = 0;
			th->isfloat =false;
		}
		return;
	}
	switch (asAtomHandler::getObjectType(args[0]))
	{
		case T_INTEGER:
		case T_BOOLEAN:
		case T_UINTEGER:
			th->ival = asAtomHandler::toInt(args[0]);
			th->isfloat = false;
			break;
		default:
			th->dval=asAtomHandler::toNumber(args[0]);
			th->isfloat = true;
			break;
	}
}

ASFUNCTIONBODY_ATOM(Number,toFixed)
{
	number_t val = asAtomHandler::toNumber(obj);
	int fractiondigits;
	ARG_CHECK(ARG_UNPACK (fractiondigits,0));
	ret = asAtomHandler::fromObject(abstract_s(wrk,toFixedString(val, fractiondigits)));
}

tiny_string Number::toFixedString(double v, int32_t fractionDigits)
{
	if ((fractionDigits < 0) || (fractionDigits > 20))
	{
		createError<RangeError>(getWorker(),kInvalidPrecisionError);
		return "";
	}
	return toString(v,DTOSTR_FIXED,fractionDigits);
}

ASFUNCTIONBODY_ATOM(Number,toExponential)
{
	double v = asAtomHandler::toNumber(obj);
	int32_t fractionDigits;
	ARG_CHECK(ARG_UNPACK(fractionDigits, 0));
	if (argslen == 0 || asAtomHandler::is<Undefined>(args[0]))
		fractionDigits = imin(imax(Number::countSignificantDigits(v)-1, 1), 20);
	ret =asAtomHandler::fromObject(abstract_s(wrk,toExponentialString(v, fractionDigits)));
}

tiny_string Number::toExponentialString(double v, int32_t fractionDigits)
{
	if ((fractionDigits < 0) || (fractionDigits > 20))
	{
		createError<RangeError>(getWorker(),kInvalidPrecisionError);
		return "";
	}
	return toString(v,DTOSTR_EXPONENTIAL,fractionDigits);
}

tiny_string Number::purgeExponentLeadingZeros(const tiny_string& exponentialForm)
{
	uint32_t i = exponentialForm.find("e");
	if (i == tiny_string::npos)
		return exponentialForm;

	tiny_string res;
	res = exponentialForm.substr(0, i+1);

	i++;
	if (i >= exponentialForm.numChars())
		return res;

	uint32_t c = exponentialForm.charAt(i);
	if (c == '-' || c == '+')
	{
		res += c;
		i++;
	}

	bool leadingZero = true;
	while (i < exponentialForm.numChars())
	{
		uint32_t c = exponentialForm.charAt(i);
		if (!leadingZero || (leadingZero && c != '0'))
		{
			res += c;
			leadingZero = false;
		}

		i++;
	}

	if (leadingZero)
		res += '0';

	return res;
}

/*
 * Should return the number of significant decimal digits necessary to
 * uniquely specify v. The actual implementation is a quick-and-dirty
 * approximation.
 */
int32_t Number::countSignificantDigits(double v) {
	char buf[40];
	snprintf(buf,40,"%.20e", v);
	
	char *p = &buf[0];
	while (*p == '0' || *p == '.')
		p++;
	
	int32_t digits = 0;
	int32_t consecutiveZeros = 0;
	while ((('0' <= *p && *p <= '9') || *p == '.') && consecutiveZeros < 10)
	{
		if (*p != '.')
			digits++;

		if (*p == '0')
			consecutiveZeros++;
		else if (*p != '.')
			consecutiveZeros = 0;
		p++;
	}

	digits -= consecutiveZeros;

	if (digits <= 0)
		digits = 1;

	return digits;
}

ASFUNCTIONBODY_ATOM(Number,toPrecision)
{
	double v = asAtomHandler::toNumber(obj);
	if (argslen == 0 || asAtomHandler::is<Undefined>(args[0]))
	{
		ret = asAtomHandler::fromObject(abstract_s(wrk,toString(v)));
		return;
	}

	int32_t precision;
	ARG_CHECK(ARG_UNPACK(precision));
	ret = asAtomHandler::fromObject(abstract_s(wrk,toPrecisionString(v, precision)));
}

tiny_string Number::toPrecisionString(double v, int32_t precision)
{
	if ((precision < 1) || (precision > 21))
	{
		createError<RangeError>(getWorker(),kInvalidPrecisionError);
		return "";
	}
	return toString(v,DTOSTR_PRECISION,precision);
}

ASFUNCTIONBODY_ATOM(Number,_valueOf)
{
	if(Class<Number>::getClass(wrk->getSystemState())->prototype->getObj() == asAtomHandler::getObject(obj))
	{
		asAtomHandler::setInt(ret,wrk,0);
		return;
	}

	if(!asAtomHandler::isNumeric(obj))
	{
		createError<TypeError>(wrk,kInvokeOnIncompatibleObjectError);
		return;
	}
	ASATOM_INCREF(obj);
	ret = obj;
}

void Number::serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap,ASWorker* wrk)
{
	serializeValue(out,toNumber());
}
void Number::serializeValue(ByteArray* out, number_t val)
{
	if (out->getObjectEncoding() == OBJECT_ENCODING::AMF0)
	{
		out->writeByte(amf0_number_marker);
		out->serializeDouble(val);
		return;
	}
	out->writeByte(double_marker);
	out->serializeDouble(val);
}
