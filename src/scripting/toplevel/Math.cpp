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
	c->setVariableAtomByQName("E",nsNameAndKind(),asAtomHandler::fromNumber(c->getSystemState(),2.71828182845904523536,true),CONSTANT_TRAIT);
	c->setVariableAtomByQName("LN10",nsNameAndKind(),asAtomHandler::fromNumber(c->getSystemState(),2.30258509299404568402,true),CONSTANT_TRAIT);
	c->setVariableAtomByQName("LN2",nsNameAndKind(),asAtomHandler::fromNumber(c->getSystemState(),0.693147180559945309417,true),CONSTANT_TRAIT);
	c->setVariableAtomByQName("LOG10E",nsNameAndKind(),asAtomHandler::fromNumber(c->getSystemState(),0.434294481903251827651,true),CONSTANT_TRAIT);
	c->setVariableAtomByQName("LOG2E",nsNameAndKind(),asAtomHandler::fromNumber(c->getSystemState(),1.44269504088896340736,true),CONSTANT_TRAIT);
	c->setVariableAtomByQName("PI",nsNameAndKind(),asAtomHandler::fromNumber(c->getSystemState(),3.14159265358979323846,true),CONSTANT_TRAIT);
	c->setVariableAtomByQName("SQRT1_2",nsNameAndKind(),asAtomHandler::fromNumber(c->getSystemState(),0.707106781186547524401,true),CONSTANT_TRAIT);
	c->setVariableAtomByQName("SQRT2",nsNameAndKind(),asAtomHandler::fromNumber(c->getSystemState(),1.41421356237309504880,true),CONSTANT_TRAIT);

	// public methods
	c->setDeclaredMethodByQName("abs","",Class<IFunction>::getFunction(c->getSystemState(),abs,1,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("acos","",Class<IFunction>::getFunction(c->getSystemState(),acos,1,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("asin","",Class<IFunction>::getFunction(c->getSystemState(),asin,1,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("atan","",Class<IFunction>::getFunction(c->getSystemState(),atan,1,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("atan2","",Class<IFunction>::getFunction(c->getSystemState(),atan2,2,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("ceil","",Class<IFunction>::getFunction(c->getSystemState(),ceil,1,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("cos","",Class<IFunction>::getFunction(c->getSystemState(),cos,1,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("exp","",Class<IFunction>::getFunction(c->getSystemState(),exp,1,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("floor","",Class<IFunction>::getFunction(c->getSystemState(),floor,1,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("log","",Class<IFunction>::getFunction(c->getSystemState(),log,1,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("max","",Class<IFunction>::getFunction(c->getSystemState(),_max,2,Class<Number>::getRef(c->getSystemState()).getPtr(),Class<Integer>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("min","",Class<IFunction>::getFunction(c->getSystemState(),_min,2,Class<Number>::getRef(c->getSystemState()).getPtr(),Class<Integer>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("pow","",Class<IFunction>::getFunction(c->getSystemState(),pow,2,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("random","",Class<IFunction>::getFunction(c->getSystemState(),random,0,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("round","",Class<IFunction>::getFunction(c->getSystemState(),round,1,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("sin","",Class<IFunction>::getFunction(c->getSystemState(),sin,1,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("sqrt","",Class<IFunction>::getFunction(c->getSystemState(),sqrt,1,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("tan","",Class<IFunction>::getFunction(c->getSystemState(),tan,1,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false);
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
	asAtomHandler::setNumber(ret,sys,::atan2(n1,n2));
}

ASFUNCTIONBODY_ATOM(Math,_max)
{
	double largest = -numeric_limits<double>::infinity();

	bool isint = argslen > 0;
	for(unsigned int i = 0; i < argslen; i++)
	{
		if (!asAtomHandler::isInteger(args[i]))
			isint = false;
		double arg = asAtomHandler::toNumber(args[i]);
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
	if (isint)
		asAtomHandler::setInt(ret,sys,largest);
	else
		asAtomHandler::setNumber(ret,sys,largest);
}

ASFUNCTIONBODY_ATOM(Math,_min)
{
	double smallest = numeric_limits<double>::infinity();

	bool isint = argslen > 0;
	for(unsigned int i = 0; i < argslen; i++)
	{
		if (!asAtomHandler::isInteger(args[i]))
			isint = false;
		double arg = asAtomHandler::toNumber(args[i]);
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
	if (isint)
		asAtomHandler::setInt(ret,sys,smallest);
	else
		asAtomHandler::setNumber(ret,sys,smallest);
}

ASFUNCTIONBODY_ATOM(Math,exp)
{
	number_t n;
	ARG_UNPACK_ATOM (n);
	asAtomHandler::setNumber(ret,sys,::exp(n));
}

ASFUNCTIONBODY_ATOM(Math,acos)
{
	//Angle is in radians
	number_t n;
	ARG_UNPACK_ATOM (n);
	asAtomHandler::setNumber(ret,sys,::acos(n));
}

ASFUNCTIONBODY_ATOM(Math,asin)
{
	//Angle is in radians
	number_t n;
	ARG_UNPACK_ATOM (n);
	asAtomHandler::setNumber(ret,sys,::asin(n));
}

ASFUNCTIONBODY_ATOM(Math,atan)
{
	//Angle is in radians
	number_t n;
	ARG_UNPACK_ATOM (n);
	asAtomHandler::setNumber(ret,sys,::atan(n));
}

ASFUNCTIONBODY_ATOM(Math,cos)
{
	//Angle is in radians
	number_t n;
	ARG_UNPACK_ATOM (n);
	asAtomHandler::setNumber(ret,sys,::cos(n));
}

ASFUNCTIONBODY_ATOM(Math,sin)
{
	//Angle is in radians
	number_t n;
	ARG_UNPACK_ATOM (n);
	asAtomHandler::setNumber(ret,sys,::sin(n));
}

ASFUNCTIONBODY_ATOM(Math,tan)
{
	//Angle is in radians
	number_t n;
	ARG_UNPACK_ATOM (n);
	asAtomHandler::setNumber(ret,sys,::tan(n));
}

ASFUNCTIONBODY_ATOM(Math,abs)
{
	if(argslen == 0)
		throwError<ArgumentError>(kWrongArgumentCountError, "Math", "1", "0");
	asAtom& a = args[0];
	switch (asAtomHandler::getObjectType(a))
	{
		case T_INTEGER:
		{
			int32_t n = asAtomHandler::toInt(a);
			if (n == INT32_MIN)
				asAtomHandler::setUInt(ret,sys,(uint32_t)n);
			else
				asAtomHandler::setInt(ret,sys,n < 0 ? -n : n);
			break;
		}
		case T_UINTEGER:
			asAtomHandler::set(ret,a);
			break;
		case T_UNDEFINED:
			asAtomHandler::setNumber(ret,sys,Number::NaN);
			break;
		case T_NULL:
			asAtomHandler::setInt(ret,sys,(int32_t)0);
			break;
		case T_NUMBER:
		{
			number_t n = asAtomHandler::toNumber(a);
			if (n  == 0.)
				asAtomHandler::setInt(ret,sys,(int32_t)0);
			else
				asAtomHandler::setNumber(ret,sys,(number_t)::fabs(n));
			break;
		}
		default:
		{
			number_t n = asAtomHandler::toNumber(a);
			if (n  == 0.)
				asAtomHandler::setInt(ret,sys,(int32_t)0);
			else
				asAtomHandler::setNumber(ret,sys,(number_t)::fabs(n));
			break;
		}
	}
}

ASFUNCTIONBODY_ATOM(Math,ceil)
{
	number_t n;
	ARG_UNPACK_ATOM (n);
	if (std::isnan(n))
		asAtomHandler::setNumber(ret,sys,Number::NaN);
	else if (n == 0.)
		asAtomHandler::setNumber(ret,sys,std::signbit(n) ? -0. : 0.);
	else if (n > INT32_MIN && n < INT32_MAX)
	{
		number_t n2 = ::ceil(n);
		if (n2 == 0.)
			asAtomHandler::setNumber(ret,sys,std::signbit(n2) ? -0. : 0.);
		else
			asAtomHandler::setInt(ret,sys,(int32_t)n2);
	}
	else
		asAtomHandler::setNumber(ret,sys,::ceil(n));
}

ASFUNCTIONBODY_ATOM(Math,log)
{
	number_t n;
	ARG_UNPACK_ATOM (n);
	asAtomHandler::setNumber(ret,sys,::log(n));
}

ASFUNCTIONBODY_ATOM(Math,floor)
{
	number_t n;
	ARG_UNPACK_ATOM (n);
	if (n == 0.)
		asAtomHandler::setNumber(ret,sys,std::signbit(n) ? -0. : 0.);
	else if (n > INT32_MIN && n < INT32_MAX)
	{
		number_t n2 = ::floor(n);
		if (n2 == 0.)
			asAtomHandler::setNumber(ret,sys,std::signbit(n2) ? -0. : 0.);
		else
			asAtomHandler::setInt(ret,sys,(int32_t)n2);
	}
	else
		asAtomHandler::setNumber(ret,sys,(number_t)::floor(n));
}

ASFUNCTIONBODY_ATOM(Math,round)
{
	number_t n;
	// for unknown reasons Adobe allows round with zero arguments on AVM1.
	// I have no idea what the return value should be in that case, for now we just use 1.0
	if (sys->mainClip->usesActionScript3)
		ARG_UNPACK_ATOM (n);
	else
		ARG_UNPACK_ATOM (n,1.0);
	if (std::isnan(n))
		asAtomHandler::setNumber(ret,sys,Number::NaN);
	else if (n < 0 && n >= -0.5)
		// it seems that adobe violates ECMA-262, chapter 15.8.2 on Math class, but avmplus got it right on Number class
		asAtomHandler::setNumber(ret,sys,asAtomHandler::getObject(obj) == Class<Number>::getClass(sys) ? -0. : 0.);
	else if (n == 0.)
		// it seems that adobe violates ECMA-262, chapter 15.8.2 on Math class, but avmplus got it right on Number class
		asAtomHandler::setNumber(ret,sys,asAtomHandler::getObject(obj) == Class<Number>::getClass(sys) ? (std::signbit(n) ? -0. : 0.) : 0.);
	else if (n > INT32_MIN && n < INT32_MAX)
		asAtomHandler::setInt(ret,sys,(int32_t)::floor(n+0.5));
	else
		asAtomHandler::setNumber(ret,sys,(number_t)::floor(n+0.5));
}

ASFUNCTIONBODY_ATOM(Math,sqrt)
{
	number_t n;
	ARG_UNPACK_ATOM (n);
	asAtomHandler::setNumber(ret,sys,::sqrt(n));
}

ASFUNCTIONBODY_ATOM(Math,pow)
{
	number_t x, y;
	ARG_UNPACK_ATOM (x) (y);
	if (::fabs(x) == 1 && (std::isnan(y) || std::isinf(y)) )
		asAtomHandler::setNumber(ret,sys,Number::NaN);
	else
		asAtomHandler::setNumber(ret,sys,::pow(x,y));
}

ASFUNCTIONBODY_ATOM(Math,random)
{
	if(argslen > 0)
		throwError<ArgumentError>(kWrongArgumentCountError, "object", "", "");

	number_t res=rand();
	res/=(number_t(1.)+RAND_MAX);
	asAtomHandler::setNumber(ret,sys,res);
}


