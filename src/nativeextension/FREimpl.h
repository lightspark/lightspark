/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2012-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef NATIVEEXTENSION_FREIMPL_H
#define NATIVEEXTENSION_FREIMPL_H 1

#include "compat.h"

extern "C" {
#if defined(_WIN32)
#	define FRE_DLL_PUBLIC __declspec(dllexport)
#else
#	define FRE_DLL_PUBLIC __attribute__ ((visibility("default")))
#endif

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

typedef struct
{
	uint32_t length;
	uint8_t* bytes;
} FREByteArray;

typedef struct
{
	uint32_t width;
	uint32_t height;
	uint32_t hasAlpha;
	uint32_t isPremultiplied;
	uint32_t lineStride32;
	uint32_t* bits32;
} FREBitmapData;
typedef struct
{
	uint32_t width;
	uint32_t height;
	uint32_t hasAlpha;
	uint32_t isPremultiplied;
	uint32_t lineStride32;
	uint32_t isInvertedY;
	uint32_t* bits32;
} FREBitmapData2;

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

FREResult FRE_DLL_PUBLIC FREAcquireBitmapData ( FREObject object, FREBitmapData* descriptorToSet );
FREResult FRE_DLL_PUBLIC FREAcquireBitmapData2 ( FREObject object, FREBitmapData2* descriptorToSet );
FREResult FRE_DLL_PUBLIC FREAcquireByteArray( FREObject object, FREByteArray* byteArrayToSet );
FREResult FRE_DLL_PUBLIC FRECallObjectMethod( FREObject object, const uint8_t* methodName, uint32_t argc, FREObject argv[], FREObject* result, FREObject* thrownException );
FREResult FRE_DLL_PUBLIC FREDispatchStatusEventAsync( FREContext ctx, const uint8_t* code, const uint8_t* level );
FREResult FRE_DLL_PUBLIC FREGetArrayElementAt( FREObject arrayOrVector, uint32_t index, FREObject* value );
FREResult FRE_DLL_PUBLIC FREGetArrayLength( FREObject arrayOrVector, uint32_t* length );
FREResult FRE_DLL_PUBLIC FREGetContextActionScriptData( FREContext ctx, FREObject *actionScriptData);
FREResult FRE_DLL_PUBLIC FREGetContextNativeData( FREContext ctx, void** nativeData );
FREResult FRE_DLL_PUBLIC FREGetObjectAsBool (FREObject object, uint32_t* value );
FREResult FRE_DLL_PUBLIC FREGetObjectAsDouble ( FREObject object, double *value );
FREResult FRE_DLL_PUBLIC FREGetObjectAsInt32 ( FREObject object, int32_t *value );
FREResult FRE_DLL_PUBLIC FREGetObjectAsUint32 ( FREObject object, uint32_t *value );
FREResult FRE_DLL_PUBLIC FREGetObjectAsUTF8( FREObject object, uint32_t* length, const uint8_t** value );
FREResult FRE_DLL_PUBLIC FREGetObjectProperty( FREObject object, const uint8_t* propertyName, FREObject* propertyValue, FREObject* thrownException );
FREResult FRE_DLL_PUBLIC FREGetObjectType( FREObject object, FREObjectType *objectType );
FREResult FRE_DLL_PUBLIC FREInvalidateBitmapDataRect( FREObject object, uint32_t x, uint32_t y, uint32_t width, uint32_t height );
FREResult FRE_DLL_PUBLIC FRENewObject( const uint8_t* className, uint32_t argc, FREObject argv[], FREObject* object, FREObject* thrownException );
FREResult FRE_DLL_PUBLIC FRENewObjectFromBool ( uint32_t value, FREObject* object);
FREResult FRE_DLL_PUBLIC FRENewObjectFromDouble ( double value, FREObject* object);
FREResult FRE_DLL_PUBLIC FRENewObjectFromInt32 ( int32_t value, FREObject* object);
FREResult FRE_DLL_PUBLIC FRENewObjectFromUint32 ( uint32_t value, FREObject* object);
FREResult FRE_DLL_PUBLIC FRENewObjectFromUTF8(uint32_t length, const uint8_t* value, FREObject* object);
FREResult FRE_DLL_PUBLIC FREReleaseBitmapData (FREObject object);
FREResult FRE_DLL_PUBLIC FREReleaseByteArray (FREObject object);
FREResult FRE_DLL_PUBLIC FRESetArrayElementAt ( FREObject arrayOrVector, uint32_t index, FREObject value );
FREResult FRE_DLL_PUBLIC FRESetArrayLength ( FREObject arrayOrVector, uint32_t length );
FREResult FRE_DLL_PUBLIC FRESetContextActionScriptData( FREContext ctx, FREObject actionScriptData);
FREResult FRE_DLL_PUBLIC FRESetContextNativeData( FREContext ctx, void* nativeData );
FREResult FRE_DLL_PUBLIC FRESetObjectProperty( FREObject object, const uint8_t* propertyName, FREObject propertyValue, FREObject* thrownException );
}

namespace lightspark
{
class DLL_PUBLIC FREObjectInterface
{
public:
	virtual ~FREObjectInterface()
	{
	}
	virtual FREResult toBool(FREObject object, uint32_t *value)=0;
	virtual FREResult toUTF8(FREObject object, uint32_t* length, const uint8_t** value)=0;
	virtual FREResult fromBool(uint32_t value, FREObject* object)=0;
	virtual FREResult fromInt32(int32_t value, FREObject* object)=0;
	virtual FREResult fromUint32(uint32_t value, FREObject* object)=0;
	virtual FREResult fromUTF8(uint32_t length, const uint8_t* value, FREObject* object)=0;
	virtual FREResult SetObjectProperty(FREObject object, const uint8_t* propertyName, FREObject propertyValue, FREObject* thrownException)=0;
	virtual FREResult AcquireByteArray(FREObject object, FREByteArray* byteArrayToSet)=0;
	virtual FREResult ReleaseByteArray (FREObject object)=0;
	virtual FREResult DispatchStatusEventAsync(FREContext ctx, const uint8_t* code, const uint8_t* level )=0;
	static void registerFREObjectInterface(FREObjectInterface* freObjectConverter);
};

}
#endif /* NATIVEEXTENSION_FREIMPL_H */
