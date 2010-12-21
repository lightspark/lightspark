/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2010  Alessandro Pignotti (a.pignotti@sssup.it)
    Copyright (C) 2010  Timon Van Overveldt (timonvo@gmail.com)

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

#ifndef _NP_SCRIPT_OBJECT_H
#define _NP_SCRIPT_OBJECT_H

#include <string.h>
#include <string>
#include <sstream>

#include "npapi.h"
#include "npruntime.h"

#include "backends/extscriptobject.h"

class NPVariantObject : public lightspark::ExtVariantObject
{
public:
	NPVariantObject() { NULL_TO_NPVARIANT(variant); }
	NPVariantObject(const NPVariantObject& other) { other.copy(variant); }
	NPVariantObject(const NPVariant& other) { copy(other, variant); }
	~NPVariantObject() { NPN_ReleaseVariantValue(&variant); }
	
	NPVariantObject& operator=(const NPVariantObject& other)
	{
		NPN_ReleaseVariantValue(&variant);
		other.copy(variant);
		return *this;
	}
	NPVariantObject& operator=(const NPVariant& other)
	{
		NPN_ReleaseVariantValue(&variant);
		copy(other, variant);
		return *this;
	}

	void copy(NPVariant& dest) const { copy(variant, dest); }

	EVO_TYPE getType() const { return getType(variant); }
	static EVO_TYPE getType(const NPVariant& variant)
	{
		if(NPVARIANT_IS_STRING(variant))
			return EVO_STRING;
		else if(NPVARIANT_IS_INT32(variant))
			return EVO_INT32;
		else if(NPVARIANT_IS_DOUBLE(variant))
			return EVO_DOUBLE;
		else if(NPVARIANT_IS_BOOLEAN(variant))
			return EVO_BOOLEAN;
		else if(NPVARIANT_IS_OBJECT(variant))
			return EVO_OBJECT;
		else if(NPVARIANT_IS_NULL(variant))
			return EVO_NULL;
		else if(NPVARIANT_IS_VOID(variant))
			return EVO_VOID;
		else
			return EVO_UNKNOWN;
	}

	std::string toString() const { return toString(variant); }
	static std::string toString(const NPVariant& variant)
	{
		switch(getType(variant))
		{
		case EVO_STRING:
			{
				NPString val = NPVARIANT_TO_STRING(variant);
				return std::string(val.UTF8Characters, val.UTF8Length);
				break;
			}
		case EVO_INT32:
			{
				int32_t val = NPVARIANT_TO_INT32(variant);
				std::stringstream out;
				out << val;
				return out.str();
				break;
			}
		case EVO_DOUBLE:
			{
				double val = NPVARIANT_TO_DOUBLE(variant);
				std::stringstream out;
				out << val;
				return out.str();
				break;
			}
		case EVO_BOOLEAN:
			{
				bool val = NPVARIANT_TO_BOOLEAN(variant);
				return (val ? "true" : "false");
				break;
			}
		case EVO_OBJECT:
			return "object";
			break;
		case EVO_NULL:
			return "null";
			break;
		case EVO_VOID:
			return "void";
			break;
		default: {}
		}
		return "unknown type";
	}

private:
	NPVariant variant;

	static void copy(const NPVariant& from, NPVariant& dest)
	{
		dest = from;

		switch(from.type)
		{
		case NPVariantType_String:
			{
				const NPString& value = NPVARIANT_TO_STRING(from);
				
				NPUTF8* newValue = static_cast<NPUTF8*>(NPN_MemAlloc(value.UTF8Length));
				memcpy(newValue, value.UTF8Characters, value.UTF8Length);

				STRINGN_TO_NPVARIANT(newValue, value.UTF8Length, dest);
				break;
			}
		case NPVariantType_Object:
			NPN_RetainObject(NPVARIANT_TO_OBJECT(dest));
			break;
		default: {}
		}
	}
};

#endif
