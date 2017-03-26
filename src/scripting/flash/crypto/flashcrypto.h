#ifndef FLASHCRYPTO_H
#define FLASHCRYPTO_H

#include "compat.h"
#include "asobject.h"

namespace lightspark
{

ASObject* generateRandomBytes(ASObject* obj,ASObject* const* args, const unsigned int argslen);
}

#endif // FLASHCRYPTO_H
