/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2012  Alessandro Pignotti (a.pignotti@sssup.it)

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
		return as<Number>()->val;
	case T_INTEGER:
		return as<Integer>()->val;
	case T_UINTEGER:
		return as<UInteger>()->val;
	case T_STRING:
		return as<ASString>()->toNumber();
	default:
		//everything else is an Object regarding to the spec
		return toPrimitive(NUMBER_HINT)->toNumber();
	}
}

bool Number::isEqual(ASObject* o)
{
	switch(o->getObjectType())
	{
		case T_INTEGER:
		case T_UINTEGER:
		case T_NUMBER:
		case T_STRING:
		case T_BOOLEAN:
			return val==o->toNumber();
		case T_NULL:
		case T_UNDEFINED:
			return false;
		default:
			return o->isEqual(this);
	}
}

TRISTATE Number::isLess(ASObject* o)
{
	if(std::isnan(val))
		return TUNDEFINED;
	if(o->getObjectType()==T_INTEGER)
	{
		const Integer* i=static_cast<const Integer*>(o);
		return (val<i->val)?TTRUE:TFALSE;
	}
	if(o->getObjectType()==T_UINTEGER)
	{
		const UInteger* i=static_cast<const UInteger*>(o);
		return (val<i->val)?TTRUE:TFALSE;
	}
	else if(o->getObjectType()==T_NUMBER)
	{
		const Number* i=static_cast<const Number*>(o);
		if(std::isnan(i->val)) return TUNDEFINED;
		return (val<i->val)?TTRUE:TFALSE;
	}
	else if(o->getObjectType()==T_BOOLEAN)
	{
		return (val<o->toNumber())?TTRUE:TFALSE;
	}
	else if(o->getObjectType()==T_UNDEFINED)
	{
		//Undefined is NaN, so the result is undefined
		return TUNDEFINED;
	}
	else if(o->getObjectType()==T_STRING)
	{
		double val2=o->toNumber();
		if(std::isnan(val2)) return TUNDEFINED;
		return (val<val2)?TTRUE:TFALSE;
	}
	else if(o->getObjectType()==T_NULL)
	{
		return (val<0)?TTRUE:TFALSE;
	}
	else
	{
		double val2=o->toPrimitive()->toNumber();
		if(std::isnan(val2)) return TUNDEFINED;
		return (val<val2)?TTRUE:TFALSE;
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

ASFUNCTIONBODY(Number,_toString)
{
	if(Class<Number>::getClass()->prototype->getObj() == obj)
		return Class<ASString>::getInstanceS("0");
	if(!obj->is<Number>())
		throw Class<TypeError>::getInstanceS("Error #1004: Number.toString is not generic");
	Number* th=static_cast<Number*>(obj);
	int radix=10;
	ARG_UNPACK (radix,10);

	if(radix==10 || std::isnan(th->val) || std::isinf(th->val))
	{
		//see e 15.7.4.2
		return Class<ASString>::getInstanceS(th->toString());
	}
	else
	{
		return Class<ASString>::getInstanceS(Number::toStringRadix(th->val, radix));
	}
}

ASFUNCTIONBODY(Number,generator)
{
	if(argslen==0)
		return abstract_d(0.);
	else
		return abstract_d(args[0]->toNumber());
}

tiny_string Number::toString()
{
	return Number::toString(val);
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
	if(fabs(val) >= 1e+21 || fabs(val) <= 1e-6)
		snprintf(buf,40,"%.15e",val);
	else
		snprintf(buf,40,"%.15f",val);
	purgeTrailingZeroes(buf);
	return tiny_string(buf,true);
}

tiny_string Number::toStringRadix(number_t val, int radix)
{
	if(radix < 2 || radix > 36)
		throw Class<RangeError>::getInstanceS("Error #1003");

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
	c->isFinal = true;
	c->setSuper(Class<ASObject>::getRef());
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	//Must create and link the number the hard way
	Number* ninf=new (c->memoryAccount) Number(c, -numeric_limits<double>::infinity());
	Number* pinf=new (c->memoryAccount) Number(c, numeric_limits<double>::infinity());
	Number* pmax=new (c->memoryAccount) Number(c, numeric_limits<double>::max());
	Number* pmin=new (c->memoryAccount) Number(c, numeric_limits<double>::min());
	Number* pnan=new (c->memoryAccount) Number(c, numeric_limits<double>::quiet_NaN());
	c->setVariableByQName("NEGATIVE_INFINITY","",ninf,CONSTANT_TRAIT);
	c->setVariableByQName("POSITIVE_INFINITY","",pinf,CONSTANT_TRAIT);
	c->setVariableByQName("MAX_VALUE","",pmax,CONSTANT_TRAIT);
	c->setVariableByQName("MIN_VALUE","",pmin,CONSTANT_TRAIT);
	c->setVariableByQName("NaN","",pnan,CONSTANT_TRAIT);
	c->prototype->setVariableByQName("toString",AS3,Class<IFunction>::getFunction(Number::_toString),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("toLocaleString",AS3,Class<IFunction>::getFunction(Number::_toString),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("toFixed",AS3,Class<IFunction>::getFunction(Number::toFixed),DYNAMIC_TRAIT);
}

ASFUNCTIONBODY(Number,_constructor)
{
	Number* th=static_cast<Number*>(obj);
	if(argslen==0)
	{
		//The number is already initialized to NaN
		return NULL;
	}
	th->val=args[0]->toNumber();
	return NULL;
}

ASFUNCTIONBODY(Number,toFixed)
{
	number_t val = obj->toNumber();
	int fractiondigits=0;
	ARG_UNPACK (fractiondigits,0);
	if (fractiondigits < 0 || fractiondigits > 20)
		throw Class<RangeError>::getInstanceS("Error #1002");
	if(std::isnan(val))
		return  Class<ASString>::getInstanceS("NaN");
	number_t fractpart, intpart;
	if (fractiondigits == 0)
		val+=0.5;
	fractpart = modf (val , &intpart);

	tiny_string res("");
	number_t v = fabs(intpart);
	char buf[40];
	snprintf(buf,40,"%ld",int64_t(v));
	res += buf;
	
	if (fractiondigits > 0)
	{
		int x = fractiondigits;
		res += ".";
		while (fractiondigits) 
		{
			fractpart*=10.0;
			fractiondigits--;
		}
		fractpart+=0.5;
		snprintf(buf,40,"%0*ld",x,int64_t(fractpart));
		res += buf;
	}
	if ( val < 0)
		res = tiny_string::fromChar('-')+res;
	return Class<ASString>::getInstanceS(res);
}

void Number::serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap)
{
	out->writeByte(double_marker);
	//We have to write the double in network byte order (big endian)
	const uint64_t* tmpPtr=reinterpret_cast<const uint64_t*>(&val);
	uint64_t bigEndianVal=GINT64_FROM_BE(*tmpPtr);
	uint8_t* bigEndianPtr=reinterpret_cast<uint8_t*>(&bigEndianVal);

	for(uint32_t i=0;i<8;i++)
		out->writeByte(bigEndianPtr[i]);
}
