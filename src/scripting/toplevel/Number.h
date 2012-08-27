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

#ifndef SCRIPTING_TOPLEVEL_NUMBER_H
#define SCRIPTING_TOPLEVEL_NUMBER_H 1

#include <cmath>
#include "compat.h"
#include "asobject.h"

namespace lightspark
{

class Number : public ASObject
{
friend ASObject* abstract_d(number_t i);
friend class ABCContext;
friend class ABCVm;
private:
	static void purgeTrailingZeroes(char* buf);
public:
	Number(Class_base* c, double v=NaN):ASObject(c),val(v){type=T_NUMBER;}
	static const number_t NaN;
	double val;
	ASFUNCTION(_constructor);
	ASFUNCTION(_toString);
	ASFUNCTION(toFixed);
	tiny_string toString();
	static tiny_string toString(number_t val);
	static tiny_string toStringRadix(number_t val, int radix);
	static bool isInteger(number_t val)
	{
		return floor(val) == val;
	}
	unsigned int toUInt()
	{
		return (unsigned int)(val);
	}
	/* ECMA-262 9.5 ToInt32 */
	int32_t toInt()
	{
		double posInt;

		/* step 2 */
		if(std::isnan(val) || std::isinf(val) || val == 0)
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
		return (int32_t)copysign(posInt, val);
	}
	TRISTATE isLess(ASObject* o);
	bool isEqual(ASObject* o);
	static void buildTraits(ASObject* o){};
	static void sinit(Class_base* c);
	ASFUNCTION(generator);
	std::string toDebugString() { return toString()+"d"; }
	//Serialization interface
	void serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap);
};


};

#endif /* SCRIPTING_TOPLEVEL_NUMBER_H */
