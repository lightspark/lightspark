/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2017  Ludger Krämer <dbluelle@onlinehome.de>

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

#include "scripting/flash/crypto/flashcrypto.h"
#include "scripting/flash/utils/ByteArray.h"
#include "scripting/argconv.h"
#include <random>

using namespace lightspark;

ASFUNCTIONBODY_ATOM(lightspark,generateRandomBytes)
{
	uint32_t numbytes;
	ARG_CHECK(ARG_UNPACK(numbytes));
	std::independent_bits_engine<std::default_random_engine, CHAR_BIT, unsigned char> engine;
	ByteArray *res = Class<ByteArray>::getInstanceS(wrk);

    for (uint32_t i = 0; i < numbytes; i++)
        res->writeByte((uint8_t)engine());
    ret = asAtomHandler::fromObject(res);
}
