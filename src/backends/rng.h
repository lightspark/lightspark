/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2026  mr b0nk 500 (b0nk@b0nk.xyz)

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

#ifndef BACKENDS_RNG_H
#define BACKENDS_RNG_H 1

#include <cstdint>

namespace lightspark
{

// Based on avmplus' RNG function.
class AVMRng
{
private:
	// https://github.com/adobe/avmplus/blob/858d034a3bd3a54d9b70909386435cf4aec81d21/core/MathUtils.cpp#L1546-L1548
	static constexpr int32_t c3 = 15731;
	static constexpr int32_t c2 = 789221;
	static constexpr int32_t c1 = 1376312589;

	// https://github.com/adobe/avmplus/blob/858d034a3bd3a54d9b70909386435cf4aec81d21/core/MathUtils.h#L43
	static constexpr int32_t kRandomPureMax = 0x7fffffff;

	// https://github.com/adobe/avmplus/blob/858d034a3bd3a54d9b70909386435cf4aec81d21/core/MathUtils.cpp#L1551-L1566
	// NOTE: Despite being a table, avmplus always picks the 29th
	// (zero-based) entry.
	static constexpr uint32_t xorMask = 0x48000000;

	uint32_t initSeed;
	uint32_t currentVal;

	void initWithSeed(uint32_t seed)
	{
		initSeed = seed;
		currentVal = seed;
	}

	// https://github.com/adobe/avmplus/blob/858d034a3bd3a54d9b70909386435cf4aec81d21/core/MathUtils.cpp#L1630-L1635
	int32_t randomFastNext();

	// https://github.com/adobe/avmplus/blob/858d034a3bd3a54d9b70909386435cf4aec81d21/core/MathUtils.cpp#L1672-L1698
	int32_t randomPureHasher(int32_t seed);
public:
	AVMRng() : initSeed(0), currentVal(0) {}

	// https://github.com/adobe/avmplus/blob/858d034a3bd3a54d9b70909386435cf4aec81d21/core/MathUtils.cpp#L1700-L1713
	int32_t generateRandomNumber();
};

}
#endif /* BACKENDS_RNG_H */
