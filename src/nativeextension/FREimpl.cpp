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

#include "FREimpl.h"
#include "asobject.h"

using namespace lightspark;

FREObjectInterface* FREObjectConverter=nullptr;


FREResult FREAcquireBitmapData ( FREObject object, FREBitmapData* descriptorToSet )
{
	std::cerr << "FRE NOT_IMPLEMENTED:FREAcquireBitmapData"<<std::endl;
	return FRE_ILLEGAL_STATE;
}
FREResult FREAcquireBitmapData2 ( FREObject object, FREBitmapData2* descriptorToSet )
{
	std::cerr << "FRE NOT_IMPLEMENTED:FREAcquireBitmapData2"<<std::endl;
	return FRE_ILLEGAL_STATE;
}
FREResult FREAcquireByteArray(FREObject object, FREByteArray* byteArrayToSet)
{
	if (!object)
		return FRE_INVALID_OBJECT;
	return FREObjectConverter->AcquireByteArray(object,byteArrayToSet);
}
FREResult FRECallObjectMethod( FREObject object, const uint8_t* methodName, uint32_t argc, FREObject argv[], FREObject* result, FREObject* thrownException )
{
	std::cerr << "FRE NOT_IMPLEMENTED:FRECallObjectMethod"<<std::endl;
	return FRE_ILLEGAL_STATE;
}
FREResult FREDispatchStatusEventAsync( FREContext ctx, const uint8_t* code, const uint8_t* level )
{
	return FREObjectConverter->DispatchStatusEventAsync(ctx,code,level);
}
FREResult FREGetArrayElementAt( FREObject arrayOrVector, uint32_t index, FREObject* value )
{
	std::cerr << "FRE NOT_IMPLEMENTED:FREGetArrayElementAt"<<std::endl;
	return FRE_ILLEGAL_STATE;
}
FREResult FREGetArrayLength( FREObject arrayOrVector, uint32_t* length )
{
	std::cerr << "FRE NOT_IMPLEMENTED:FREGetArrayLength"<<std::endl;
	return FRE_ILLEGAL_STATE;
}
FREResult FREGetContextActionScriptData( FREContext ctx, FREObject *actionScriptData)
{
	std::cerr << "FRE NOT_IMPLEMENTED:FREGetContextActionScriptData"<<std::endl;
	return FRE_ILLEGAL_STATE;
}
FREResult FREGetContextNativeData( FREContext ctx, void** nativeData )
{
	std::cerr << "FRE NOT_IMPLEMENTED:FREGetContextNativeData"<<std::endl;
	return FRE_ILLEGAL_STATE;
}
FREResult FREGetObjectAsBool ( FREObject object, uint32_t *value )
{
	if (!object)
		return FRE_INVALID_OBJECT;
	return FREObjectConverter->toBool(object,value);
}
FREResult FREGetObjectAsDouble ( FREObject object, double *value )
{
	std::cerr << "FRE NOT_IMPLEMENTED:FREGetObjectAsDouble"<<std::endl;
	return FRE_ILLEGAL_STATE;
}
FREResult FREGetObjectAsInt32 ( FREObject object, int32_t *value )
{
	std::cerr << "FRE NOT_IMPLEMENTED:FREGetObjectAsInt32"<<std::endl;
	return FRE_ILLEGAL_STATE;
}
FREResult FREGetObjectAsUint32 ( FREObject object, uint32_t *value )
{
	std::cerr << "FRE NOT_IMPLEMENTED:FREGetObjectAsUint32"<<std::endl;
	return FRE_ILLEGAL_STATE;
}
FREResult FREGetObjectAsUTF8( FREObject object, uint32_t* length, const uint8_t** value )
{
	if (!object)
		return FRE_INVALID_OBJECT;
	return FREObjectConverter->toUTF8(object,length,value);
}
FREResult FREGetObjectProperty( FREObject object, const uint8_t* propertyName, FREObject* propertyValue, FREObject* thrownException )
{
	std::cerr << "FRE NOT_IMPLEMENTED:FREGetObjectProperty"<<std::endl;
	return FRE_ILLEGAL_STATE;
}
FREResult FREGetObjectType( FREObject object, FREObjectType *objectType )
{
	std::cerr << "FRE NOT_IMPLEMENTED:FREGetObjectType"<<std::endl;
	return FRE_ILLEGAL_STATE;
}
FREResult FREInvalidateBitmapDataRect( FREObject object, uint32_t x, uint32_t y, uint32_t width, uint32_t height )
{
	std::cerr << "FRE NOT_IMPLEMENTED:FREInvalidateBitmapDataRect"<<std::endl;
	return FRE_ILLEGAL_STATE;
}
FREResult FRENewObject( const uint8_t* className, uint32_t argc, FREObject argv[], FREObject* object, FREObject* thrownException )
{
	std::cerr << "FRE NOT_IMPLEMENTED:FRENewObject"<<std::endl;
	return FRE_ILLEGAL_STATE;
}
FREResult FRENewObjectFromBool ( uint32_t value, FREObject* object)
{
	if (!object)
		return FRE_INVALID_ARGUMENT;
	return FREObjectConverter->fromBool(value,object);
}
FREResult FRENewObjectFromDouble(double value, FREObject* object)
{
	std::cerr << "FRE NOT_IMPLEMENTED:FRENewObjectFromDouble"<<std::endl;
	return FRE_OK;
}
FREResult FRENewObjectFromInt32 ( int32_t value, FREObject* object)
{
	if (!object)
		return FRE_INVALID_ARGUMENT;
	return FREObjectConverter->fromInt32(value,object);
}
FREResult FRENewObjectFromUint32 ( uint32_t value, FREObject* object)
{
	if (!object)
		return FRE_INVALID_ARGUMENT;
	return FREObjectConverter->fromUint32(value,object);
}
FREResult FRENewObjectFromUTF8(uint32_t length, const uint8_t* value, FREObject* object)
{
	if (!object)
		return FRE_INVALID_ARGUMENT;
	return FREObjectConverter->fromUTF8(length,value,object);
}
FREResult FREReleaseBitmapData (FREObject object)
{
	std::cerr << "FRE NOT_IMPLEMENTED:FREReleaseBitmapData"<<std::endl;
	return FRE_ILLEGAL_STATE;
}
FREResult FREReleaseByteArray (FREObject object)
{
	if (!object)
		return FRE_INVALID_OBJECT;
	return FREObjectConverter->ReleaseByteArray(object);
}
FREResult FRESetArrayElementAt ( FREObject arrayOrVector, uint32_t index, FREObject value )
{
	std::cerr << "FRE NOT_IMPLEMENTED:FRESetArrayElementAt"<<std::endl;
	return FRE_ILLEGAL_STATE;
}
FREResult FRESetArrayLength(FREObject arrayOrVector, uint32_t length)
{
	std::cerr << "FRE NOT_IMPLEMENTED:FRESetArrayLength"<<std::endl;
	return FRE_ILLEGAL_STATE;
}
FREResult FRESetContextActionScriptData( FREContext ctx, FREObject actionScriptData)
{
	std::cerr << "FRE NOT_IMPLEMENTED:FRESetContextActionScriptData"<<std::endl;
	return FRE_ILLEGAL_STATE;
}
FREResult FRESetContextNativeData( FREContext ctx, void* nativeData )
{
	std::cerr << "FRE NOT_IMPLEMENTED:FRESetContextNativeData"<<std::endl;
	return FRE_ILLEGAL_STATE;
}
FREResult FRESetObjectProperty( FREObject object, const uint8_t* propertyName, FREObject propertyValue, FREObject* thrownException )
{
	if (!object)
		return FRE_INVALID_ARGUMENT;
	return FREObjectConverter->SetObjectProperty(object, propertyName, propertyValue, thrownException );
}


void FREObjectInterface::registerFREObjectInterface(FREObjectInterface* freObjectConverter)
{
	if (FREObjectConverter==nullptr)
		FREObjectConverter=freObjectConverter;
}
