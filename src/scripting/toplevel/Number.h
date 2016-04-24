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

#ifndef SCRIPTING_TOPLEVEL_NUMBER_H
#define SCRIPTING_TOPLEVEL_NUMBER_H 1

#include <cmath>
#include "compat.h"
#include "asobject.h"

namespace lightspark
{

class Number : public ASObject
{
friend ASObject* abstract_d(SystemState* sys,number_t i);
friend ASObject* abstract_di(SystemState* sys,int32_t i);
friend class ABCContext;
friend class ABCVm;
private:
	static void purgeTrailingZeroes(char* buf);
	static tiny_string purgeExponentLeadingZeros(const tiny_string& exponentialForm);
	static int32_t countSignificantDigits(double v);
public:
	Number(Class_base* c, double v=Number::NaN):ASObject(c),dval(v),isfloat(true){type=T_NUMBER;}
	static const number_t NaN;
	union {
		number_t dval;
		int64_t ival;
	};
	bool isfloat;
	inline number_t toNumber() { return isfloat ? dval : ival; }
	inline void finalize() { dval=Number::NaN; isfloat = true;}
	ASFUNCTION(_constructor);
	ASFUNCTION(_toString);
	ASFUNCTION(toExponential);
	ASFUNCTION(toPrecision);
	ASFUNCTION(toFixed);
	ASFUNCTION(_valueOf);
	tiny_string toString();
	static tiny_string toString(number_t val);
	static tiny_string toStringRadix(number_t val, int radix);
	static tiny_string toExponentialString(double v, int32_t fractionDigits);
	static tiny_string toFixedString(double v, int32_t fractionDigits);
	static tiny_string toPrecisionString(double v, int32_t precision);
	static bool isInteger(number_t val)
	{
		return trunc(val) == val;
	}
	unsigned int toUInt()
	{
		return isfloat ? (unsigned int)(dval) : ival;
	}
	int64_t toInt64()
	{
		if (!isfloat) return ival;
		if(std::isnan(dval) || std::isinf(dval))
			return INT64_MAX;
		return (int64_t)dval;
	}

	/* ECMA-262 9.5 ToInt32 */
	int32_t toInt()
	{
		if (!isfloat) return ival;
		double posInt;

		/* step 2 */
		if(std::isnan(dval) || std::isinf(dval) || dval == 0.0)
			return 0;
		/* step 3 */
		posInt = floor(fabs(dval));
		/* step 4 */
		if (posInt > 4294967295.0)
			posInt = fmod(posInt, 4294967296.0);
		/* step 5 */
		if (posInt >= 2147483648.0) {
			// follow tamarin
			if(dval < 0.0)
				return 0x80000000 - (int32_t)(posInt - 2147483648.0);
			else
				return 0x80000000 + (int32_t)(posInt - 2147483648.0);
		}
		return (int32_t)(dval < 0.0 ? -posInt : posInt);
	}
	TRISTATE isLess(ASObject* o);
	bool isEqual(ASObject* o);
	static void buildTraits(ASObject* o){};
	static void sinit(Class_base* c);
	ASFUNCTION(generator);
	std::string toDebugString() { return toString()+(isfloat ? "d" : "di"); }
	//Serialization interface
	void serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap);
};


};

#endif /* SCRIPTING_TOPLEVEL_NUMBER_H */
