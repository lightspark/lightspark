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

#include "scripting/toplevel/Math.h"
#include "scripting/class.h"
#include "scripting/argconv.h"

using namespace std;
using namespace lightspark;

void Math::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED | CLASS_FINAL);
	// public constants
	c->setVariableAtomByQName("E",nsNameAndKind(),asAtom(2.71828182845904523536),CONSTANT_TRAIT);
	c->setVariableAtomByQName("LN10",nsNameAndKind(),asAtom(2.30258509299404568402),CONSTANT_TRAIT);
	c->setVariableAtomByQName("LN2",nsNameAndKind(),asAtom(0.693147180559945309417),CONSTANT_TRAIT);
	c->setVariableAtomByQName("LOG10E",nsNameAndKind(),asAtom(0.434294481903251827651),CONSTANT_TRAIT);
	c->setVariableAtomByQName("LOG2E",nsNameAndKind(),asAtom(1.44269504088896340736),CONSTANT_TRAIT);
	c->setVariableAtomByQName("PI",nsNameAndKind(),asAtom(3.14159265358979323846),CONSTANT_TRAIT);
	c->setVariableAtomByQName("SQRT1_2",nsNameAndKind(),asAtom(0.707106781186547524401),CONSTANT_TRAIT);
	c->setVariableAtomByQName("SQRT2",nsNameAndKind(),asAtom(1.41421356237309504880),CONSTANT_TRAIT);

	// public methods
	c->setDeclaredMethodByQName("abs","",Class<IFunction>::getFunction(c->getSystemState(),abs,1),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("acos","",Class<IFunction>::getFunction(c->getSystemState(),acos,1),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("asin","",Class<IFunction>::getFunction(c->getSystemState(),asin,1),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("atan","",Class<IFunction>::getFunction(c->getSystemState(),atan,1),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("atan2","",Class<IFunction>::getFunction(c->getSystemState(),atan2,2),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("ceil","",Class<IFunction>::getFunction(c->getSystemState(),ceil,1),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("cos","",Class<IFunction>::getFunction(c->getSystemState(),cos,1),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("exp","",Class<IFunction>::getFunction(c->getSystemState(),exp,1),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("floor","",Class<IFunction>::getFunction(c->getSystemState(),floor,1),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("log","",Class<IFunction>::getFunction(c->getSystemState(),log,1),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("max","",Class<IFunction>::getFunction(c->getSystemState(),_max,2),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("min","",Class<IFunction>::getFunction(c->getSystemState(),_min,2),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("pow","",Class<IFunction>::getFunction(c->getSystemState(),pow,2),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("random","",Class<IFunction>::getFunction(c->getSystemState(),random),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("round","",Class<IFunction>::getFunction(c->getSystemState(),round,1),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("sin","",Class<IFunction>::getFunction(c->getSystemState(),sin,1),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("sqrt","",Class<IFunction>::getFunction(c->getSystemState(),sqrt,1),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("tan","",Class<IFunction>::getFunction(c->getSystemState(),tan,1),NORMAL_METHOD,false);
}

ASFUNCTIONBODY_ATOM(Math,_constructor)
{
	throwError<TypeError>(kMathNotConstructorError);
}

ASFUNCTIONBODY_ATOM(Math,generator)
{
	throwError<TypeError>(kMathNotFunctionError);
}

ASFUNCTIONBODY_ATOM(Math,atan2)
{
	number_t n1, n2;
	ARG_UNPACK_ATOM (n1) (n2);
	ret.setNumber(::atan2(n1,n2));
}

ASFUNCTIONBODY_ATOM(Math,_max)
{
	double largest = -numeric_limits<double>::infinity();

	for(unsigned int i = 0; i < argslen; i++)
	{
		double arg = args[i].toNumber();
		if (std::isnan(arg))
		{
			largest = numeric_limits<double>::quiet_NaN();
			break;
		}
		if(largest == arg && signbit(largest) > signbit(arg))
			largest = 0.0; //Spec 15.8.2.11: 0.0 should be larger than -0.0
		else
			largest = (arg>largest) ? arg : largest;
	}
	ret.setNumber(largest);
}

ASFUNCTIONBODY_ATOM(Math,_min)
{
	double smallest = numeric_limits<double>::infinity();

	for(unsigned int i = 0; i < argslen; i++)
	{
		double arg = args[i].toNumber();
		if (std::isnan(arg))
		{
			smallest = numeric_limits<double>::quiet_NaN();
			break;
		}
		if(smallest == arg && signbit(arg) > signbit(smallest))
			smallest = -0.0; //Spec 15.8.2.11: 0.0 should be larger than -0.0
		else
			smallest = (arg<smallest)? arg : smallest;
	}

	ret.setNumber(smallest);
}

ASFUNCTIONBODY_ATOM(Math,exp)
{
	number_t n;
	ARG_UNPACK_ATOM (n);
	ret.setNumber(::exp(n));
}

ASFUNCTIONBODY_ATOM(Math,acos)
{
	//Angle is in radians
	number_t n;
	ARG_UNPACK_ATOM (n);
	ret.setNumber(::acos(n));
}

ASFUNCTIONBODY_ATOM(Math,asin)
{
	//Angle is in radians
	number_t n;
	ARG_UNPACK_ATOM (n);
	ret.setNumber(::asin(n));
}

ASFUNCTIONBODY_ATOM(Math,atan)
{
	//Angle is in radians
	number_t n;
	ARG_UNPACK_ATOM (n);
	ret.setNumber(::atan(n));
}

ASFUNCTIONBODY_ATOM(Math,cos)
{
	//Angle is in radians
	number_t n;
	ARG_UNPACK_ATOM (n);
	ret.setNumber(::cos(n));
}

ASFUNCTIONBODY_ATOM(Math,sin)
{
	//Angle is in radians
	number_t n;
	ARG_UNPACK_ATOM (n);
	ret.setNumber(::sin(n));
}

ASFUNCTIONBODY_ATOM(Math,tan)
{
	//Angle is in radians
	number_t n;
	ARG_UNPACK_ATOM (n);
	ret.setNumber(::tan(n));
}

ASFUNCTIONBODY_ATOM(Math,abs)
{
	if(argslen == 0)
		throwError<ArgumentError>(kWrongArgumentCountError, "Math", "1", "0");
	asAtom& a = args[0];
	switch (a.type)
	{
		case T_INTEGER:
		{
			int32_t n = a.toInt();
			if (n == INT32_MIN)
				ret.setUInt((uint32_t)n);
			else
				ret.setInt(n < 0 ? -n : n);
			break;
		}
		case T_UINTEGER:
			ret.set(a);
			break;
		case T_UNDEFINED:
			ret.setNumber(Number::NaN);
			break;
		case T_NULL:
			ret.setInt((int32_t)0);
			break;
		default:
		{
			number_t n = a.toNumber();
			if (n  == 0.)
				ret.setInt((int32_t)0);
			else
				ret.setNumber((number_t)::fabs(n));
			break;
		}
	}
}

ASFUNCTIONBODY_ATOM(Math,ceil)
{
	number_t n;
	ARG_UNPACK_ATOM (n);
	if (std::isnan(n))
		ret.setNumber(Number::NaN);
	else if (n == 0.)
		ret.setNumber(std::signbit(n) ? -0. : 0.);
	else if (n > INT32_MIN && n < INT32_MAX)
	{
		number_t n2 = ::ceil(n);
		if (n2 == 0.)
			ret.setNumber(std::signbit(n2) ? -0. : 0.);
		else
			ret.setInt((int32_t)n2);
	}
	else
		ret.setNumber(::ceil(n));
}

ASFUNCTIONBODY_ATOM(Math,log)
{
	number_t n;
	ARG_UNPACK_ATOM (n);
	ret.setNumber(::log(n));
}

ASFUNCTIONBODY_ATOM(Math,floor)
{
	number_t n;
	ARG_UNPACK_ATOM (n);
	if (n == 0.)
		ret.setNumber(std::signbit(n) ? -0. : 0.);
	else if (n > INT32_MIN && n < INT32_MAX)
	{
		number_t n2 = ::floor(n);
		if (n2 == 0.)
			ret.setNumber(std::signbit(n2) ? -0. : 0.);
		else
			ret.setInt((int32_t)n2);
	}
	else
		ret.setNumber((number_t)::floor(n));
}

ASFUNCTIONBODY_ATOM(Math,round)
{
	number_t n;
	ARG_UNPACK_ATOM (n);
	if (std::isnan(n))
		ret.setNumber(Number::NaN);
	else if (n < 0 && n >= -0.5)
		// it seems that adobe violates ECMA-262, chapter 15.8.2 on Math class, but avmplus got it right on Number class
		ret.setNumber(obj.getObject() == Class<Number>::getClass(sys) ? -0. : 0.);
	else if (n == 0.)
		// it seems that adobe violates ECMA-262, chapter 15.8.2 on Math class, but avmplus got it right on Number class
		ret.setNumber(obj.getObject() == Class<Number>::getClass(sys) ? (std::signbit(n) ? -0. : 0.) : 0.);
	else if (n > INT32_MIN && n < INT32_MAX)
		ret.setInt((int32_t)::floor(n+0.5));
	else
		ret.setNumber((number_t)::floor(n+0.5));
}

ASFUNCTIONBODY_ATOM(Math,sqrt)
{
	number_t n;
	ARG_UNPACK_ATOM (n);
	ret.setNumber(::sqrt(n));
}

ASFUNCTIONBODY_ATOM(Math,pow)
{
	number_t x, y;
	ARG_UNPACK_ATOM (x) (y);
	if (::fabs(x) == 1 && (std::isnan(y) || std::isinf(y)) )
		ret.setNumber(Number::NaN);
	else
		ret.setNumber(::pow(x,y));
}

ASFUNCTIONBODY_ATOM(Math,random)
{
	if(argslen > 0)
		throwError<ArgumentError>(kWrongArgumentCountError, "object", "", "");

	number_t res=rand();
	res/=(number_t(1.)+RAND_MAX);
	ret.setNumber(res);
}


