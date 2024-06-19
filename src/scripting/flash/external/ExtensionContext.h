#ifndef FLASH_EXTERNAL_EXTENSIONCONTEXT_H
#define FLASH_EXTERNAL_EXTENSIONCONTEXT_H

#include "scripting/flash/events/flashevents.h"
#include "nativeextension/FREimpl.h"

namespace lightspark
{
class DLL_PUBLIC ExtensionContext : public EventDispatcher
{
private:
	tiny_string extensionID;
	tiny_string contextType;
	void* nativelibrary;
	void finalizeExtensionContext();
	FREContextInitializer contextinitializer;
	FREContextFinalizer contextfinalizer;
	uint32_t numFunctionsToSet;
	const FRENamedFunction* functionsToSet;
	void* nativeExtData;
	std::vector<asAtom> atomlist;
	std::vector<uint8_t*> stringlist;
public:
	void finalize() override;
	bool destruct() override;
	void prepareShutdown() override;
	ExtensionContext(ASWorker* wrk,Class_base* c);
	static void sinit(Class_base*);
	ASFUNCTION_ATOM(createExtensionContext);
	ASFUNCTION_ATOM(_call);
	ASFUNCTION_ATOM(dispose);
	static void registerExtension(const tiny_string& filepath);
};

}
#endif
