#include "URLRequestHeader.h"
#include "class.h"
#include "argconv.h"

using namespace lightspark;

URLRequestHeader::URLRequestHeader(Class_base* c) : ASObject(c)
{
}

void URLRequestHeader::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_FINAL | CLASS_SEALED);
	REGISTER_GETTER_SETTER(c,name);
	REGISTER_GETTER_SETTER(c,value);
}

void URLRequestHeader::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(URLRequestHeader,_constructor)
{
	URLRequestHeader* th=Class<URLRequestHeader>::cast(obj);
	ARG_UNPACK (th->name, "") (th->value, "");
	return NULL;
}

ASFUNCTIONBODY_GETTER_SETTER(URLRequestHeader,name);
ASFUNCTIONBODY_GETTER_SETTER(URLRequestHeader,value);
