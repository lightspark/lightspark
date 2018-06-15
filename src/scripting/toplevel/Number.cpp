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
#include "scripting/toplevel/Math.h"
#include "scripting/flash/utils/ByteArray.h"

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
			asAtom val2p;
			toPrimitive(val2p,NUMBER_HINT);
			return val2p.toNumber();
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
			asAtom val2p;
			o->toPrimitive(val2p,NUMBER_HINT);
			double val2=val2p.toNumber();
			if(std::isnan(val2)) return TUNDEFINED;
			return (toNumber()<val2)?TTRUE:TFALSE;
		}
	}
}

/*
 * This purges trailing zeros from the right, i.e.
 * "144124.45600" -> "144124.456"
 * "144124.000" -> "144124"
 * "144124.45600e+12" -> "144124.456e+12"
 * "144124.45600e+07" -> 144124.456e+7"
 * and it transforms the ',' into a '.' if found.
 */
void Number::purgeTrailingZeroes(char* buf)
{
	int i=strlen(buf)-1;
	int Epos = 0;
	if(i>4 && buf[i-3] == 'e')
	{
		Epos = i-3;
		i=i-4;
	}
	for(;i>0;i--)
	{
		if(buf[i]!='0')
			break;
	}
	bool commaFound=false;
	if(buf[i]==',' || buf[i]=='.')
	{
		i--;
		commaFound=true;
	}
	if(Epos)
	{
		//copy e+12 to the current place
		strncpy(buf+i+1,buf+Epos,5);
		if(buf[i+3] == '0')
		{
			//this looks like e+07, so turn it into e+7
			buf[i+3] = buf[i+4];
			buf[i+4] = '\0';
		}
	}
	else
		buf[i+1]='\0';

	if(commaFound)
		return;

	//Also change the comma to the point if needed
	for(;i>0;i--)
	{
		if(buf[i]==',')
		{
			buf[i]='.';
			break;
		}
	}
}

ASFUNCTIONBODY_ATOM(Number,_toString)
{
	if(Class<Number>::getClass(sys)->prototype->getObj() == obj.getObject())
	{
		ret = asAtom::fromString(sys,"0");
		return;
	}
	if(!obj.isNumeric())
		throwError<TypeError>(kInvokeOnIncompatibleObjectError, "Number.toString");
	int radix=10;
	ARG_UNPACK_ATOM (radix,10);

	if((radix==10) || (obj.is<Number>() &&
					   (std::isnan(obj.toNumber()) ||
					   (std::isinf(obj.toNumber())))))
	{
		//see e 15.7.4.2
		ret = asAtom::fromObject(abstract_s(sys,obj.toString(sys)));
	}
	else
	{
		ret = asAtom::fromObject(abstract_s(sys,Number::toStringRadix(obj.toNumber(), radix)));
	}
}
ASFUNCTIONBODY_ATOM(Number,_toLocaleString)
{
	if(Class<Number>::getClass(sys)->prototype->getObj() == obj.getObject())
	{
		ret = asAtom::fromString(sys,"0");
		return;
	}
	if(!obj.isNumeric())
	{
		ret = asAtom::fromString(sys,"0");
		return;
	}
	int radix=10;

	if((radix==10) || (obj.is<Number>() &&
					   (std::isnan(obj.toNumber()) ||
					   (std::isinf(obj.toNumber())))))
	{
		//see e 15.7.4.2
		ret = asAtom::fromObject(abstract_s(sys,obj.toString(sys)));
	}
	else
	{
		ret = asAtom::fromObject(abstract_s(sys,Number::toStringRadix(obj.toNumber(), radix)));
	}
}

ASFUNCTIONBODY_ATOM(Number,generator)
{
	if(argslen==0)
		ret.setNumber(0.);
	else
		ret.setNumber(args[0].toNumber());
}

tiny_string Number::toString()
{
	return Number::toString(isfloat ? dval : ival);
}

/* static helper function */
tiny_string Number::toString(number_t val)
{
	if(std::isnan(val))
		return "NaN";
	if(std::isinf(val))
	{
		if(val > 0)
			return "Infinity";
		else
			return "-Infinity";
	}
	if(val == 0) //this also handles the case '-0'
		return "0";

	//See ecma3 8.9.1
	char buf[40];
	if (val==(int)(val))
		snprintf(buf,40,"%d",(int)(val));
	else
	{
		if(fabs(val) >= 1e+21 || fabs(val) <= 1e-6)
			snprintf(buf,40,"%.15e",val);
		else
			snprintf(buf,40,"%.14f",val);
		purgeTrailingZeroes(buf);
	}
	return tiny_string(buf,true);
}

tiny_string Number::toStringRadix(number_t val, int radix)
{
	if(radix < 2 || radix > 36)
		throwError<RangeError>(kInvalidRadixError, Integer::toString(radix));

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
	c->setVariableAtomByQName("NEGATIVE_INFINITY",nsNameAndKind(),asAtom(-numeric_limits<double>::infinity()),CONSTANT_TRAIT);
	c->setVariableAtomByQName("POSITIVE_INFINITY",nsNameAndKind(),asAtom(numeric_limits<double>::infinity()),CONSTANT_TRAIT);
	c->setVariableAtomByQName("MAX_VALUE",nsNameAndKind(),asAtom(numeric_limits<double>::max()),CONSTANT_TRAIT);
	c->setVariableAtomByQName("MIN_VALUE",nsNameAndKind(),asAtom(numeric_limits<double>::min()),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NaN",nsNameAndKind(),asAtom(numeric_limits<double>::quiet_NaN()),CONSTANT_TRAIT);
	c->setDeclaredMethodByQName("toString",AS3,Class<IFunction>::getFunction(c->getSystemState(),_toString),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("toFixed",AS3,Class<IFunction>::getFunction(c->getSystemState(),toFixed,1),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("toExponential",AS3,Class<IFunction>::getFunction(c->getSystemState(),toExponential,1),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("toPrecision",AS3,Class<IFunction>::getFunction(c->getSystemState(),toPrecision,1),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("valueOf",AS3,Class<IFunction>::getFunction(c->getSystemState(),_valueOf),NORMAL_METHOD,true);
	c->prototype->setVariableByQName("toString","",Class<IFunction>::getFunction(c->getSystemState(),Number::_toString),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("toLocaleString","",Class<IFunction>::getFunction(c->getSystemState(),Number::_toLocaleString),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("toFixed","",Class<IFunction>::getFunction(c->getSystemState(),Number::toFixed, 1),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("toExponential","",Class<IFunction>::getFunction(c->getSystemState(),Number::toExponential, 1),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("toPrecision","",Class<IFunction>::getFunction(c->getSystemState(),Number::toPrecision, 1),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("valueOf","",Class<IFunction>::getFunction(c->getSystemState(),_valueOf),DYNAMIC_TRAIT);

	// if needed add AVMPLUS definitions
	if(c->getSystemState()->flashMode==SystemState::AVMPLUS)
	{
		c->setVariableAtomByQName("E",nsNameAndKind(),asAtom(2.71828182845905),CONSTANT_TRAIT,false);
		c->setVariableAtomByQName("LN10",nsNameAndKind(),asAtom(2.302585092994046),CONSTANT_TRAIT,false);
		c->setVariableAtomByQName("LN2",nsNameAndKind(),asAtom(0.6931471805599453),CONSTANT_TRAIT,false);
		c->setVariableAtomByQName("LOG10E",nsNameAndKind(),asAtom(0.4342944819032518),CONSTANT_TRAIT,false);
		c->setVariableAtomByQName("LOG2E",nsNameAndKind(),asAtom(1.442695040888963387),CONSTANT_TRAIT,false);
		c->setVariableAtomByQName("PI",nsNameAndKind(),asAtom(3.141592653589793),CONSTANT_TRAIT,false);
		c->setVariableAtomByQName("SQRT1_2",nsNameAndKind(),asAtom(0.7071067811865476),CONSTANT_TRAIT,false);
		c->setVariableAtomByQName("SQRT2",nsNameAndKind(),asAtom(1.4142135623730951),CONSTANT_TRAIT,false);
		
		c->setDeclaredMethodByQName("abs","",Class<IFunction>::getFunction(c->getSystemState(),Math::abs,1),NORMAL_METHOD,false,false);
		c->setDeclaredMethodByQName("acos","",Class<IFunction>::getFunction(c->getSystemState(),Math::acos,1),NORMAL_METHOD,false,false);
		c->setDeclaredMethodByQName("asin","",Class<IFunction>::getFunction(c->getSystemState(),Math::asin,1),NORMAL_METHOD,false,false);
		c->setDeclaredMethodByQName("atan","",Class<IFunction>::getFunction(c->getSystemState(),Math::atan,1),NORMAL_METHOD,false,false);
		c->setDeclaredMethodByQName("atan2","",Class<IFunction>::getFunction(c->getSystemState(),Math::atan2,2),NORMAL_METHOD,false,false);
		c->setDeclaredMethodByQName("ceil","",Class<IFunction>::getFunction(c->getSystemState(),Math::ceil,1),NORMAL_METHOD,false,false);
		c->setDeclaredMethodByQName("cos","",Class<IFunction>::getFunction(c->getSystemState(),Math::cos,1),NORMAL_METHOD,false,false);
		c->setDeclaredMethodByQName("exp","",Class<IFunction>::getFunction(c->getSystemState(),Math::exp,1),NORMAL_METHOD,false,false);
		c->setDeclaredMethodByQName("floor","",Class<IFunction>::getFunction(c->getSystemState(),Math::floor,1),NORMAL_METHOD,false,false);
		c->setDeclaredMethodByQName("log","",Class<IFunction>::getFunction(c->getSystemState(),Math::log,1),NORMAL_METHOD,false,false);
		c->setDeclaredMethodByQName("max","",Class<IFunction>::getFunction(c->getSystemState(),Math::_max,2),NORMAL_METHOD,false,false);
		c->setDeclaredMethodByQName("min","",Class<IFunction>::getFunction(c->getSystemState(),Math::_min,2),NORMAL_METHOD,false,false);
		c->setDeclaredMethodByQName("pow","",Class<IFunction>::getFunction(c->getSystemState(),Math::pow,2),NORMAL_METHOD,false,false);
		c->setDeclaredMethodByQName("random","",Class<IFunction>::getFunction(c->getSystemState(),Math::random),NORMAL_METHOD,false,false);
		c->setDeclaredMethodByQName("round","",Class<IFunction>::getFunction(c->getSystemState(),Math::round,1),NORMAL_METHOD,false,false);
		c->setDeclaredMethodByQName("sin","",Class<IFunction>::getFunction(c->getSystemState(),Math::sin,1),NORMAL_METHOD,false,false);
		c->setDeclaredMethodByQName("sqrt","",Class<IFunction>::getFunction(c->getSystemState(),Math::sqrt,1),NORMAL_METHOD,false,false);
		c->setDeclaredMethodByQName("tan","",Class<IFunction>::getFunction(c->getSystemState(),Math::tan,1),NORMAL_METHOD,false,false);
	}
}

ASFUNCTIONBODY_ATOM(Number,_constructor)
{
	Number* th=obj.as<Number>();
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
	switch (args[0].type)
	{
		case T_INTEGER:
		case T_BOOLEAN:
		case T_UINTEGER:
			th->ival = args[0].toInt();
			th->isfloat = false;
			break;
		default:
			th->dval=args[0].toNumber();
			th->isfloat = true;
			break;
	}
}

ASFUNCTIONBODY_ATOM(Number,toFixed)
{
	number_t val = obj.toNumber();
	int fractiondigits;
	ARG_UNPACK_ATOM (fractiondigits,0);
	ret = asAtom::fromObject(abstract_s(sys,toFixedString(val, fractiondigits)));
}

tiny_string Number::toFixedString(double v, int32_t fractiondigits)
{
	if (fractiondigits < 0 || fractiondigits > 20)
		throwError<RangeError>(kInvalidPrecisionError);
	if (std::isnan(v))
		return  "NaN";
	if (v >= pow(10., 21))
		return toString(v);
	number_t fractpart, intpart;
	double rounded = v + 0.5*pow(10., -fractiondigits);
	fractpart = modf(rounded , &intpart);

	tiny_string res("");
	char buf[40];
	snprintf(buf,40,"%ld",int64_t(fabs(intpart)));
	res += buf;
	
	if (fractiondigits > 0)
	{
		number_t x = fractpart;
		res += ".";
		for (int i=0; i<fractiondigits; i++)
		{
			x*=10.0;
			int n = (int)x;
			x -= n;
			res += tiny_string::fromChar('0' + n);
		}
	}
	if ( v < 0)
		res = tiny_string::fromChar('-')+res;
	return res;
}

ASFUNCTIONBODY_ATOM(Number,toExponential)
{
	double v = obj.toNumber();
	int32_t fractionDigits;
	ARG_UNPACK_ATOM(fractionDigits, 0);
	if (argslen == 0 || args[0].is<Undefined>())
		fractionDigits = imin(imax(Number::countSignificantDigits(v)-1, 1), 20);
	ret =asAtom::fromObject(abstract_s(sys,toExponentialString(v, fractionDigits)));
}

tiny_string Number::toExponentialString(double v, int32_t fractionDigits)
{
	if (std::isnan(v) || std::isinf(v))
		return toString(v);

	tiny_string res;
	if (v < 0)
	{
		res = "-";
		v = -v;
	}

	if (fractionDigits < 0 || fractionDigits > 20)
		throwError<RangeError>(kInvalidPrecisionError);
	
	char buf[40];
	snprintf(buf,40,"%.*e", fractionDigits, v);
	res += buf;
	res = purgeExponentLeadingZeros(res);
	return res;
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
	double v = obj.toNumber();
	if (argslen == 0 || args[0].is<Undefined>())
	{
		ret = asAtom::fromObject(abstract_s(sys,toString(v)));
		return;
	}

	int32_t precision;
	ARG_UNPACK_ATOM(precision);
	ret = asAtom::fromObject(abstract_s(sys,toPrecisionString(v, precision)));
}

tiny_string Number::toPrecisionString(double v, int32_t precision)
{
	if (precision < 1 || precision > 21)
	{
		throwError<RangeError>(kInvalidPrecisionError);
		return NULL;
	}
	else if (std::isnan(v) || std::isinf(v))
		return toString(v);
	else if (::fabs(v) > pow(10., precision))
		return toExponentialString(v, precision-1);
	else if (v == 0)
	{
		tiny_string zero = "0.";
		for (int i=0; i<precision; i++)
			zero += "0";
		return zero;
	}
	else
	{
		int n = (int)::ceil(::log10(::fabs(v)));
		if (n < 0)
			return toExponentialString(v, precision-1);
		else
			return toFixedString(v, precision-n);
	}
}

ASFUNCTIONBODY_ATOM(Number,_valueOf)
{
	if(Class<Number>::getClass(sys)->prototype->getObj() == obj.getObject())
	{
		ret.setInt(0);
		return;
	}

	if(!obj.isNumeric())
		throwError<TypeError>(kInvokeOnIncompatibleObjectError);

	ASATOM_INCREF(obj);
	ret = obj;
}

void Number::serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap)
{
	if (out->getObjectEncoding() == ObjectEncoding::AMF0)
	{
		out->writeByte(amf0_number_marker);
		out->serializeDouble(toNumber());
		return;
	}
	out->writeByte(double_marker);
	out->serializeDouble(toNumber());
}
