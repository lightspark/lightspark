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
friend ASObject* abstract_d(ASWorker* worker,number_t i);
friend ASObject* abstract_d_constant(ASWorker* worker,number_t i);
friend ASObject* abstract_di(ASWorker* worker,int32_t i);
friend class ABCContext;
friend class ABCVm;
private:
	static tiny_string purgeExponentLeadingZeros(const tiny_string& exponentialForm);
	static int32_t countSignificantDigits(double v);
	enum DTOSTRMODE { DTOSTR_NORMAL, DTOSTR_FIXED, DTOSTR_PRECISION, DTOSTR_EXPONENTIAL };
	static tiny_string toString(number_t value, DTOSTRMODE mode, int32_t precision);
public:
	Number(ASWorker* worker,Class_base* c, double v=Number::NaN):ASObject(worker,c,T_NUMBER),dval(v),isfloat(true){}
	static const number_t NaN;
	union {
		number_t dval;
		int64_t ival;
	};
	bool isfloat:1;
	inline void setNumber(number_t v)
	{
		isfloat = true;
		dval=v;
	}
	inline number_t toNumber() override { return isfloat ? dval : ival; }
	inline bool destruct() override { dval=Number::NaN; isfloat = true; return destructIntern(); }
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(_toString);
	ASFUNCTION_ATOM(_toLocaleString);
	ASFUNCTION_ATOM(toExponential);
	ASFUNCTION_ATOM(toPrecision);
	ASFUNCTION_ATOM(toFixed);
	ASFUNCTION_ATOM(_valueOf);
	tiny_string toString() const;
	static tiny_string toString(number_t val) { return toString(val,DTOSTR_NORMAL,15);}
	static tiny_string toStringRadix(number_t val, int radix);
	static tiny_string toExponentialString(double v, int32_t fractionDigits);
	static tiny_string toFixedString(double v, int32_t fractionDigits);
	static tiny_string toPrecisionString(double v, int32_t precision);
	static bool isInteger(number_t val)
	{
		return trunc(val) == val;
	}
	unsigned int toUInt() override
	{
		return (unsigned int)toInt();
	}
	int64_t toInt64() override
	{
		if (!isfloat) return ival;
		if(std::isnan(dval) || std::isinf(dval))
			return INT64_MAX;
		return (int64_t)dval;
	}

	/* ECMA-262 9.5 ToInt32 */
	int32_t toInt() override
	{
		if (!isfloat) return ival;
		return toInt(dval);
	}
	static int32_t toInt(number_t val)
	{
		double posInt;

		/* step 2 */
		if(std::isnan(val) || std::isinf(val) || val == 0.0)
			return 0;
		/* step 3 */
		posInt = floor(fabs(val));
		/* step 4 */
		if (posInt > 4294967295.0)
			posInt = fmod(posInt, 4294967296.0);
		/* step 5 */
		if (posInt >= 2147483648.0) {
			// follow tamarin
			if(val < 0.0)
				return 0x80000000 - (int32_t)(posInt - 2147483648.0);
			else
				return 0x80000000 + (int32_t)(posInt - 2147483648.0);
		}
		return (int32_t)(val < 0.0 ? -posInt : posInt);
	}
	TRISTATE isLess(ASObject* o) override;
	bool isEqual(ASObject* o) override;
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(generator);
	std::string toDebugString() const override;
	//Serialization interface
	void serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap, ASWorker* wrk) override;
};


}

#endif /* SCRIPTING_TOPLEVEL_NUMBER_H */
