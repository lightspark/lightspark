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
#include "scripting/toplevel/Number.h"
#include "scripting/toplevel/Integer.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include "scripting/flash/display/RootMovieClip.h"

using namespace std;
using namespace lightspark;

void Math::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED | CLASS_FINAL);
	// public constants
	c->setVariableAtomByQName("E",nsNameAndKind(),asAtomHandler::fromNumber(c->getInstanceWorker(),2.71828182845904523536,true),CONSTANT_TRAIT);
	c->setVariableAtomByQName("LN10",nsNameAndKind(),asAtomHandler::fromNumber(c->getInstanceWorker(),2.30258509299404568402,true),CONSTANT_TRAIT);
	c->setVariableAtomByQName("LN2",nsNameAndKind(),asAtomHandler::fromNumber(c->getInstanceWorker(),0.693147180559945309417,true),CONSTANT_TRAIT);
	c->setVariableAtomByQName("LOG10E",nsNameAndKind(),asAtomHandler::fromNumber(c->getInstanceWorker(),0.434294481903251827651,true),CONSTANT_TRAIT);
	c->setVariableAtomByQName("LOG2E",nsNameAndKind(),asAtomHandler::fromNumber(c->getInstanceWorker(),1.44269504088896340736,true),CONSTANT_TRAIT);
	c->setVariableAtomByQName("PI",nsNameAndKind(),asAtomHandler::fromNumber(c->getInstanceWorker(),3.14159265358979323846,true),CONSTANT_TRAIT);
	c->setVariableAtomByQName("SQRT1_2",nsNameAndKind(),asAtomHandler::fromNumber(c->getInstanceWorker(),0.707106781186547524401,true),CONSTANT_TRAIT);
	c->setVariableAtomByQName("SQRT2",nsNameAndKind(),asAtomHandler::fromNumber(c->getInstanceWorker(),1.41421356237309504880,true),CONSTANT_TRAIT);

	// public methods
	c->setDeclaredMethodByQName("abs","",c->getSystemState()->getBuiltinFunction(abs,1,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("acos","",c->getSystemState()->getBuiltinFunction(acos,1,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("asin","",c->getSystemState()->getBuiltinFunction(asin,1,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("atan","",c->getSystemState()->getBuiltinFunction(atan,1,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("atan2","",c->getSystemState()->getBuiltinFunction(atan2,2,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("ceil","",c->getSystemState()->getBuiltinFunction(ceil,1,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("cos","",c->getSystemState()->getBuiltinFunction(cos,1,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("exp","",c->getSystemState()->getBuiltinFunction(exp,1,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("floor","",c->getSystemState()->getBuiltinFunction(floor,1,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("log","",c->getSystemState()->getBuiltinFunction(log,1,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("max","",c->getSystemState()->getBuiltinFunction(_max,2,Class<Number>::getRef(c->getSystemState()).getPtr(),Class<Integer>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("min","",c->getSystemState()->getBuiltinFunction(_min,2,Class<Number>::getRef(c->getSystemState()).getPtr(),Class<Integer>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("pow","",c->getSystemState()->getBuiltinFunction(pow,2,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("random","",c->getSystemState()->getBuiltinFunction(random,0,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("round","",c->getSystemState()->getBuiltinFunction(round,1,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("sin","",c->getSystemState()->getBuiltinFunction(sin,1,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("sqrt","",c->getSystemState()->getBuiltinFunction(sqrt,1,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("tan","",c->getSystemState()->getBuiltinFunction(tan,1,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false);
}

ASFUNCTIONBODY_ATOM(Math,_constructor)
{
	createError<TypeError>(wrk,kMathNotConstructorError);
}

ASFUNCTIONBODY_ATOM(Math,generator)
{
	createError<TypeError>(wrk,kMathNotFunctionError);
}

ASFUNCTIONBODY_ATOM(Math,atan2)
{
	number_t n1, n2;
	ARG_CHECK(ARG_UNPACK (n1) (n2));
	wrk->setBuiltinCallResultLocalNumber(ret, ::atan2(n1,n2));
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
		asAtomHandler::setInt(ret,wrk,largest);
	else
		wrk->setBuiltinCallResultLocalNumber(ret, largest);
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
		asAtomHandler::setInt(ret,wrk,smallest);
	else
		wrk->setBuiltinCallResultLocalNumber(ret, smallest);
}

ASFUNCTIONBODY_ATOM(Math,exp)
{
	number_t n;
	ARG_CHECK(ARG_UNPACK (n));
	wrk->setBuiltinCallResultLocalNumber(ret, ::exp(n));
}

ASFUNCTIONBODY_ATOM(Math,acos)
{
	//Angle is in radians
	number_t n;
	ARG_CHECK(ARG_UNPACK (n));
	wrk->setBuiltinCallResultLocalNumber(ret, ::acos(n));
}

ASFUNCTIONBODY_ATOM(Math,asin)
{
	//Angle is in radians
	number_t n;
	ARG_CHECK(ARG_UNPACK (n));
	wrk->setBuiltinCallResultLocalNumber(ret, ::asin(n));
}

ASFUNCTIONBODY_ATOM(Math,atan)
{
	//Angle is in radians
	number_t n;
	ARG_CHECK(ARG_UNPACK (n));
	wrk->setBuiltinCallResultLocalNumber(ret, ::atan(n));
}

ASFUNCTIONBODY_ATOM(Math,cos)
{
	//Angle is in radians
	number_t n;
	ARG_CHECK(ARG_UNPACK (n));
	wrk->setBuiltinCallResultLocalNumber(ret, ::cos(n));
}

ASFUNCTIONBODY_ATOM(Math,sin)
{
	//Angle is in radians
	number_t n;
	ARG_CHECK(ARG_UNPACK (n));
	wrk->setBuiltinCallResultLocalNumber(ret, ::sin(n));
}

ASFUNCTIONBODY_ATOM(Math,tan)
{
	//Angle is in radians
	number_t n;
	ARG_CHECK(ARG_UNPACK (n));
	wrk->setBuiltinCallResultLocalNumber(ret, ::tan(n));
}

ASFUNCTIONBODY_ATOM(Math,abs)
{
	if(argslen == 0)
	{
		createError<ArgumentError>(wrk,kWrongArgumentCountError, "Math", "1", "0");
		return;
	}
	asAtom& a = args[0];
	switch (asAtomHandler::getObjectType(a))
	{
		case T_INTEGER:
		{
			int32_t n = asAtomHandler::toInt(a);
			if (n == INT32_MIN)
				asAtomHandler::setUInt(ret,wrk,(uint32_t)n);
			else
				asAtomHandler::setInt(ret,wrk,n < 0 ? -n : n);
			break;
		}
		case T_UINTEGER:
			asAtomHandler::set(ret,a);
			break;
		case T_UNDEFINED:
			wrk->setBuiltinCallResultLocalNumber(ret, Number::NaN);
			break;
		case T_NULL:
			asAtomHandler::setInt(ret,wrk,(int32_t)0);
			break;
		case T_NUMBER:
		{
			number_t n = asAtomHandler::getNumber(wrk,a);
			if (n  == 0.)
				asAtomHandler::setInt(ret,wrk,(int32_t)0);
			else
				wrk->setBuiltinCallResultLocalNumber(ret, ::fabs(n));
			break;
		}
		default:
		{
			number_t n = asAtomHandler::toNumber(a);
			if (n  == 0.)
				asAtomHandler::setInt(ret,wrk,(int32_t)0);
			else
				wrk->setBuiltinCallResultLocalNumber(ret, ::fabs(n));
			break;
		}
	}
}

ASFUNCTIONBODY_ATOM(Math,ceil)
{
	number_t n;
	ARG_CHECK(ARG_UNPACK (n));
	if (std::isnan(n))
		wrk->setBuiltinCallResultLocalNumber(ret, Number::NaN);
	else if (n == 0.)
		wrk->setBuiltinCallResultLocalNumber(ret, std::signbit(n) ? -0. : 0.);
	else if (n > INT32_MIN && n < INT32_MAX)
	{
		number_t n2 = ::ceil(n);
		if (n2 == 0.)
			wrk->setBuiltinCallResultLocalNumber(ret, std::signbit(n2) ? -0. : 0.);
		else
			asAtomHandler::setInt(ret,wrk,(int32_t)n2);
	}
	else
		wrk->setBuiltinCallResultLocalNumber(ret, ::ceil(n));
}

ASFUNCTIONBODY_ATOM(Math,log)
{
	number_t n;
	ARG_CHECK(ARG_UNPACK (n));
	wrk->setBuiltinCallResultLocalNumber(ret, ::log(n));
}

ASFUNCTIONBODY_ATOM(Math,floor)
{
	number_t n;
	ARG_CHECK(ARG_UNPACK (n));
	if (n == 0.)
		wrk->setBuiltinCallResultLocalNumber(ret, std::signbit(n) ? -0. : 0.);
	else if (n > INT32_MIN && n < INT32_MAX)
	{
		number_t n2 = ::floor(n);
		if (n2 == 0.)
			wrk->setBuiltinCallResultLocalNumber(ret, std::signbit(n2) ? -0. : 0.);
		else
			asAtomHandler::setInt(ret,wrk,(int32_t)n2);
	}
	else
		wrk->setBuiltinCallResultLocalNumber(ret, ::floor(n));
}

ASFUNCTIONBODY_ATOM(Math,round)
{
	number_t n;
	// for unknown reasons Adobe allows round with zero arguments on AVM1.
	// I have no idea what the return value should be in that case, for now we just use 1.0
	if (wrk->getSystemState()->mainClip->needsActionScript3())
	{
		ARG_CHECK(ARG_UNPACK (n));
	}
	else
	{
		ARG_CHECK(ARG_UNPACK (n,1.0));
	}
	if (std::isnan(n))
		wrk->setBuiltinCallResultLocalNumber(ret, Number::NaN);
	else if (n < 0 && n >= -0.5)
		// it seems that adobe violates ECMA-262, chapter 15.8.2 on Math class, but avmplus got it right on Number class
		wrk->setBuiltinCallResultLocalNumber(ret, asAtomHandler::getObject(obj) == Class<Number>::getClass(wrk->getSystemState()) ? -0. : 0.);
	else if (n == 0.)
		// it seems that adobe violates ECMA-262, chapter 15.8.2 on Math class, but avmplus got it right on Number class
		wrk->setBuiltinCallResultLocalNumber(ret, asAtomHandler::getObject(obj) == Class<Number>::getClass(wrk->getSystemState()) ? (std::signbit(n) ? -0. : 0.) : 0.);
	else if (n > INT32_MIN && n < INT32_MAX)
		asAtomHandler::setInt(ret,wrk,(int32_t)::floor(n+0.5));
	else
		wrk->setBuiltinCallResultLocalNumber(ret, ::floor(n+0.5));
}

ASFUNCTIONBODY_ATOM(Math,sqrt)
{
	number_t n;
	ARG_CHECK(ARG_UNPACK (n));
	wrk->setBuiltinCallResultLocalNumber(ret, ::sqrt(n));
}

ASFUNCTIONBODY_ATOM(Math,pow)
{
	number_t x, y;
	ARG_CHECK(ARG_UNPACK (x) (y));
	if (::fabs(x) == 1 && (std::isnan(y) || std::isinf(y)) )
		wrk->setBuiltinCallResultLocalNumber(ret, Number::NaN);
	else if (std::isinf(x) && x<0 && y<0 && Number::isInteger(y))
	{
		// special case, see ruffle test from_avmplus/as3/Types/Number/pow
		wrk->setBuiltinCallResultLocalNumber(ret, -0.0);
	}
	else if (x==0 && std::signbit(x) && y>0 && Number::isInteger(y))
	{
		// special case, see ruffle test from_avmplus/as3/Types/Number/pow
		wrk->setBuiltinCallResultLocalNumber(ret, uint64_t(y)%2 ? -0.0 : 0.0);
	}
	else
	{
		number_t val = ::pow(x,y);
		if (Number::isInteger(val))
		{
			// turn result into (u)int if possible
			if (::abs(val) < INT32_MAX)
				asAtomHandler::setInt(ret,wrk,val);
			else if (val > 0 && val < UINT32_MAX)
				asAtomHandler::setUInt(ret,wrk,val);
			else
				wrk->setBuiltinCallResultLocalNumber(ret, val);
		}
		else
			wrk->setBuiltinCallResultLocalNumber(ret, val);
	}
}

ASFUNCTIONBODY_ATOM(Math,random)
{
	number_t max=1.0;
	if(argslen > 0 && wrk->getSystemState()->mainClip->needsActionScript3())
	{
		createError<ArgumentError>(wrk,kWrongArgumentCountError, "object", "", "");
		return;
	}
	else
		ARG_CHECK(ARG_UNPACK(max,1.0));

	number_t res=rand();
	res/=(number_t(1.)+RAND_MAX)*max;
	wrk->setBuiltinCallResultLocalNumber(ret, res);
}


