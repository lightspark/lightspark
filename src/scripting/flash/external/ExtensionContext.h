#ifndef FLASH_EXTERNAL_EXTENSIONCONTEXT_H
#define FLASH_EXTERNAL_EXTENSIONCONTEXT_H

#include "scripting/flash/events/flashevents.h"

// Definitions taken from https://help.adobe.com/en_US/air/extensions/air_extensions.pdf
typedef void* FREContext;
typedef void* FREObject;

enum FREObjectType
{
	FRE_TYPE_OBJECT = 0,
	FRE_TYPE_NUMBER = 1,
	FRE_TYPE_STRING = 2,
	FRE_TYPE_BYTEARRAY = 3,
	FRE_TYPE_ARRAY = 4,
	FRE_TYPE_VECTOR = 5,
	FRE_TYPE_BITMAPDATA = 6,
	FRE_TYPE_BOOLEAN = 7,
	FRE_TYPE_NULL = 8,
	FREObjectType_ENUMPADDING = 0xfffff
};

enum FREResult
{
	FRE_OK                  = 0,
	FRE_NO_SUCH_NAME        = 1,
	FRE_INVALID_OBJECT      = 2,
	FRE_TYPE_MISMATCH       = 3,
	FRE_ACTIONSCRIPT_ERROR  = 4,
	FRE_INVALID_ARGUMENT    = 5,
	FRE_READ_ONLY           = 6,
	FRE_WRONG_THREAD        = 7,
	FRE_ILLEGAL_STATE       = 8,
	FRE_INSUFFICIENT_MEMORY = 9,
	FREResult_ENUMPADDING   = 0xffff
};

typedef FREObject (*FREFunction)(
	FREContext ctx,
	void* functionData,
	uint32_t argc,
	FREObject argv[]
);

typedef struct FRENamedFunction_
{
	const uint8_t* name;
	void* functionData;
	FREFunction function;
} FRENamedFunction;

typedef void (*FREContextInitializer)(
	void* extData,
	const uint8_t* ctxType,
	FREContext ctx,
	uint32_t* numFunctionsToSet,
	const FRENamedFunction** functionsToSet
);

typedef void (*FREContextFinalizer)(
	FREContext ctx
);

typedef void (*FREInitializer)(
	void** extDataToSet,
	FREContextInitializer* ctxInitializerToSet,
	FREContextFinalizer* contextFinalizerToSet
);

namespace lightspark
{
class ExtensionContext : public EventDispatcher
{
private:
	tiny_string extensionID;
	tiny_string contextType;
	uint32_t numFunctionsToSet;
	const FRENamedFunction* functionsToSet;
	void* nativelibrary;
	void* nativeExtData;
	void finalizeExtensionContext();
public:
	void finalize() override;
	bool destruct() override;
	ExtensionContext(ASWorker* wrk,Class_base* c);
	static void sinit(Class_base*);
	ASFUNCTION_ATOM(createExtensionContext);
	ASFUNCTION_ATOM(_call);
	ASFUNCTION_ATOM(dispose);
	static void registerExtension(const tiny_string& filepath);
};

}
#endif
