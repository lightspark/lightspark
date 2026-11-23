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

#include "backends/rng.h"

using namespace lightspark;

// https://github.com/adobe/avmplus/blob/858d034a3bd3a54d9b70909386435cf4aec81d21/core/MathUtils.cpp#L1630-L1635
int32_t AVMRng::randomFastNext()
{
	return
	(
		currentVal & 1 ?
		currentVal >> 1 ^ xorMask :
		currentVal >> 1
	);
}

// https://github.com/adobe/avmplus/blob/858d034a3bd3a54d9b70909386435cf4aec81d21/core/MathUtils.cpp#L1672-L1698
int32_t AVMRng::randomPureHasher(int32_t seed)
{
	seed = ((seed << 13) ^ seed) - (seed >> 21);

	auto ret = (seed * (seed * seed * c3 + c2) + c1) & kRandomPureMax;
	ret += seed;
	return ((ret << 13) ^ ret) - (ret >> 21);
}

// https://github.com/adobe/avmplus/blob/858d034a3bd3a54d9b70909386435cf4aec81d21/core/MathUtils.cpp#L1700-L1713
int32_t AVMRng::generateRandomNumber()
{
	// NOTE: avmplus initializes it's RNG on first use.
	if (!currentVal)
		initWithSeed(getSys()->getRngSeed());
	return randomPureHasher(randomFastNext() * 71) & kRandomPureMax;
}
