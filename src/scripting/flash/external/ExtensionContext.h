#ifndef FLASH_EXTERNAL_EXTENSIONCONTEXT_H
#define FLASH_EXTERNAL_EXTENSIONCONTEXT_H

#include "scripting/flash/events/flashevents.h"

namespace lightspark
{

class ExtensionContext : public EventDispatcher
{
private:
	tiny_string extensionID;
	tiny_string contextType;
public:
	ExtensionContext(ASWorker* wrk,Class_base* c);
	static void sinit(Class_base*);
	ASFUNCTION_ATOM(createExtensionContext);
	ASFUNCTION_ATOM(_call);
};

}
#endif
