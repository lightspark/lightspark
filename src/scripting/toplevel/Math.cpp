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
	c->setVariableByQName("E","",abstract_d(c->getSystemState(),2.71828182845905),CONSTANT_TRAIT);
	c->setVariableByQName("LN10","",abstract_d(c->getSystemState(),2.302585092994046),CONSTANT_TRAIT);
	c->setVariableByQName("LN2","",abstract_d(c->getSystemState(),0.6931471805599453),CONSTANT_TRAIT);
	c->setVariableByQName("LOG10E","",abstract_d(c->getSystemState(),0.4342944819032518),CONSTANT_TRAIT);
	c->setVariableByQName("LOG2E","",abstract_d(c->getSystemState(),1.442695040888963387),CONSTANT_TRAIT);
	c->setVariableByQName("PI","",abstract_d(c->getSystemState(),3.141592653589793),CONSTANT_TRAIT);
	c->setVariableByQName("SQRT1_2","",abstract_d(c->getSystemState(),0.7071067811865476),CONSTANT_TRAIT);
	c->setVariableByQName("SQRT2","",abstract_d(c->getSystemState(),1.4142135623730951),CONSTANT_TRAIT);

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

ASFUNCTIONBODY(Math,_constructor)
{
	throwError<TypeError>(kMathNotConstructorError);
	return NULL;
}

ASFUNCTIONBODY_ATOM(Math,generator)
{
	throwError<TypeError>(kMathNotFunctionError);
	return asAtom::invalidAtom;
}

ASFUNCTIONBODY(Math,atan2)
{
	number_t n1, n2;
	ARG_UNPACK (n1) (n2);
	return abstract_d(obj->getSystemState(),::atan2(n1,n2));
}

ASFUNCTIONBODY(Math,_max)
{
	double largest = -numeric_limits<double>::infinity();

	for(unsigned int i = 0; i < argslen; i++)
	{
		double arg = args[i]->toNumber();
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
	return abstract_d(obj->getSystemState(),largest);
}

ASFUNCTIONBODY(Math,_min)
{
	double smallest = numeric_limits<double>::infinity();

	for(unsigned int i = 0; i < argslen; i++)
	{
		double arg = args[i]->toNumber();
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

	return abstract_d(obj->getSystemState(),smallest);
}

ASFUNCTIONBODY(Math,exp)
{
	number_t n;
	ARG_UNPACK (n);
	return abstract_d(obj->getSystemState(),::exp(n));
}

ASFUNCTIONBODY(Math,acos)
{
	//Angle is in radians
	number_t n;
	ARG_UNPACK (n);
	return abstract_d(obj->getSystemState(),::acos(n));
}

ASFUNCTIONBODY(Math,asin)
{
	//Angle is in radians
	number_t n;
	ARG_UNPACK (n);
	return abstract_d(obj->getSystemState(),::asin(n));
}

ASFUNCTIONBODY(Math,atan)
{
	//Angle is in radians
	number_t n;
	ARG_UNPACK (n);
	return abstract_d(obj->getSystemState(),::atan(n));
}

ASFUNCTIONBODY(Math,cos)
{
	//Angle is in radians
	number_t n;
	ARG_UNPACK (n);
	return abstract_d(obj->getSystemState(),::cos(n));
}

ASFUNCTIONBODY(Math,sin)
{
	//Angle is in radians
	number_t n;
	ARG_UNPACK (n);
	return abstract_d(obj->getSystemState(),::sin(n));
}

ASFUNCTIONBODY(Math,tan)
{
	//Angle is in radians
	number_t n;
	ARG_UNPACK (n);
	return abstract_d(obj->getSystemState(),::tan(n));
}

ASFUNCTIONBODY(Math,abs)
{
	number_t n;
	ARG_UNPACK (n);
	return abstract_d(obj->getSystemState(),::fabs(n));
}

ASFUNCTIONBODY(Math,ceil)
{
	number_t n;
	ARG_UNPACK (n);
	return abstract_d(obj->getSystemState(),::ceil(n));
}

ASFUNCTIONBODY(Math,log)
{
	number_t n;
	ARG_UNPACK (n);
	return abstract_d(obj->getSystemState(),::log(n));
}

ASFUNCTIONBODY(Math,floor)
{
	number_t n;
	ARG_UNPACK (n);
	return abstract_d(obj->getSystemState(),::floor(n));
}

ASFUNCTIONBODY(Math,round)
{
	number_t n;
	ARG_UNPACK (n);
	if (n < 0 && n >= -0.5)
		return abstract_d(obj->getSystemState(),0);
	return abstract_d(obj->getSystemState(),::round(n));
}

ASFUNCTIONBODY(Math,sqrt)
{
	number_t n;
	ARG_UNPACK (n);
	return abstract_d(obj->getSystemState(),::sqrt(n));
}

ASFUNCTIONBODY(Math,pow)
{
	number_t x, y;
	ARG_UNPACK (x) (y);
	if (::fabs(x) == 1 && (std::isnan(y) || std::isinf(y)) )
		return abstract_d(obj->getSystemState(),Number::NaN);
	return abstract_d(obj->getSystemState(),::pow(x,y));
}

ASFUNCTIONBODY(Math,random)
{
	if(argslen > 0)
		throwError<ArgumentError>(kWrongArgumentCountError, "object", "", "");

	number_t ret=rand();
	ret/=(number_t(1.)+RAND_MAX);
	return abstract_d(obj->getSystemState(),ret);
}


