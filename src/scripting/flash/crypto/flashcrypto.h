#ifndef FLASHCRYPTO_H
#define FLASHCRYPTO_H

#include "compat.h"
#include "asobject.h"

namespace lightspark
{

void generateRandomBytes(asAtom& ret, SystemState* sys, asAtom& obj, asAtom* args, const unsigned int argslen);
}

#endif // FLASHCRYPTO_H
