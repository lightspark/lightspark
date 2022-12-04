#include "URLRequestHeader.h"
#include "class.h"
#include "argconv.h"

using namespace lightspark;

URLRequestHeader::URLRequestHeader(ASWorker* wrk,Class_base* c):ASObject(wrk,c)
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

ASFUNCTIONBODY_ATOM(URLRequestHeader,_constructor)
{
	URLRequestHeader* th=asAtomHandler::as<URLRequestHeader>(obj);
	ARG_CHECK(ARG_UNPACK (th->name, "") (th->value, ""));
}

ASFUNCTIONBODY_GETTER_SETTER(URLRequestHeader,name)
ASFUNCTIONBODY_GETTER_SETTER(URLRequestHeader,value)
