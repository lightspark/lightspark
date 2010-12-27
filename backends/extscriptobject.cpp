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

#include <string>

#include "asobject.h"
#include "scripting/class.h"
#include "scripting/toplevel.h"
#include "compat.h"

#include "extscriptobject.h"

using namespace lightspark;
using namespace std;

void ExtObject::copy(std::map<ExtIdentifierObject, ExtVariantObject>& dest) const
{
	dest = properties;
}

bool ExtObject::hasProperty(const ExtIdentifierObject& id) const
{
	return properties.find(id) != properties.end();
}
ExtVariantObject* ExtObject::getProperty(const ExtIdentifierObject& id) const
{
	std::map<ExtIdentifierObject, ExtVariantObject>::const_iterator it = properties.find(id);
	if(it == properties.end())
		return NULL;

	const ExtVariantObject& temp = it->second;
	ExtVariantObject* result = new ExtVariantObject(temp);
	return result;
}
void ExtObject::setProperty(const ExtIdentifierObject& id, const ExtVariantObject& value)
{
	properties[id] = value;
}
bool ExtObject::removeProperty(const ExtIdentifierObject& id)
{
	std::map<ExtIdentifierObject, ExtVariantObject>::iterator it = properties.find(id);
	if(it == properties.end())
		return false;

	properties.erase(it);
	return true;
}
bool ExtObject::enumerate(ExtIdentifierObject*** ids, uint32_t* count) const
{
	*count = properties.size();
	*ids = new ExtIdentifierObject*[properties.size()];
	std::map<ExtIdentifierObject, ExtVariantObject>::const_iterator it;
	int i = 0;
	for(it = properties.begin(); it != properties.end(); ++it)
	{
		(*ids)[i] = new ExtIdentifierObject(it->first);
		i++;
	}

	return true;
}

/* More advanced copy constructors */
ExtVariantObject::ExtVariantObject(const ExtVariantObject& other)
{
	type = other.getType();
	strValue = other.getString();
	intValue = other.getInt();
	doubleValue = other.getDouble();
	booleanValue = other.getBoolean();
	objectValue = *other.getObject();
}
ExtVariantObject::ExtVariantObject(ASObject* other) :
	strValue(""), intValue(0), doubleValue(0), booleanValue(false)
{
	switch(other->getObjectType())
	{
	case T_STRING:
		strValue = other->toString().raw_buf();
		type = EVO_STRING;
		break;
	case T_INTEGER:
		intValue = other->toInt();
		type = EVO_INT32;
		break;
	case T_NUMBER:
		doubleValue = other->toNumber();
		type = EVO_DOUBLE;
		break;
	case T_ARRAY:
		objectValue.setType(ExtObject::EO_ARRAY);
	case T_OBJECT:
		type = EVO_OBJECT;
		{
			bool hasNext = false;
			ASObject* nextName = NULL;
			ASObject* nextValue = NULL;
			unsigned int index = 0;
			while(other->hasNext(index, hasNext) && hasNext)
			{
				other->nextName(index, nextName);
				other->nextValue(index-1, nextValue);
				if(nextName->getObjectType() == T_INTEGER)
				{
					objectValue.setProperty(nextName->toInt(), nextValue);
				}
				else
				{
					objectValue.setProperty(nextName->toString().raw_buf(), nextValue);
				}
				nextName->decRef();
				nextValue->decRef();
			}
		}
		break;
	case T_NULL:
		type = EVO_NULL;
		break;
	case T_UNDEFINED:
	default:
		type = EVO_VOID;
		break;
	}
}

ASObject* ExtVariantObject::getASObject() const
{
	switch(getType())
	{
	case EVO_STRING:
		return Class<ASString>::getInstanceS(getString());
	case EVO_INT32:
		return abstract_i(getInt());
	case EVO_DOUBLE:
		return abstract_d(getDouble());
	case EVO_BOOLEAN:
		return abstract_b(getBoolean());
	case EVO_OBJECT:
		{
			ExtObject* objValue = getObject();
			ASObject* asobj;

			ExtVariantObject* property;
			uint32_t count;

			// We are converting an array, so lets set indexes
			if(objValue->getType() == ExtObject::EO_ARRAY)
			{
				asobj = Class<Array>::getInstanceS();
				count = objValue->getLength();
				static_cast<Array*>(asobj)->resize(count);
				for(uint32_t i = 0; i < count; i++)
				{
					property = objValue->getProperty(i);
					static_cast<Array*>(asobj)->set(i, property->getASObject());
					delete property;
				}
			}
			// We are converting an object, so lets set variables
			else
			{
				asobj = Class<ASObject>::getInstanceS();
			
				ExtIdentifierObject** ids;
				uint32_t count;
				std::stringstream strOut;
				if(objValue != NULL && objValue->enumerate(&ids, &count))
				{
					for(uint32_t i = 0; i < count; i++)
					{
						property = objValue->getProperty(*ids[i]);

						if(ids[i]->getType() == ExtIdentifierObject::EI_STRING)
							asobj->setVariableByQName(ids[i]->getString(), "", property->getASObject());
						else
						{
							strOut << ids[i]->getInt();
							asobj->setVariableByQName(strOut.str().c_str(), "", property->getASObject());
						}
						delete property;
						delete ids[i];
					}
				}
				delete ids;
			}
			if(objValue != NULL)
				delete objValue;

			asobj->incRef();
			return asobj;
		}
	case EVO_NULL:
		return new Null;
	case EVO_VOID:
	default:
		return new Undefined;
	}
}

bool ExtCallbackFunction::operator()(const ExtScriptObject& so, const ExtIdentifierObject& id,
		const ExtVariantObject** args, uint32_t argc, ExtVariantObject** result)
{
	if(iFunc != NULL)
	{
		// Convert raw arguments to objects
		ASObject* objArgs[argc];
		for(uint32_t i = 0; i < argc; i++)
		{
			objArgs[i] = args[i]->getASObject();
		}

		ASObject* objResult = iFunc->call(new Null, objArgs, argc);
		if(objResult != NULL)
			*result = new ExtVariantObject(objResult);
		return true;
	}
	else
		return extFunc(so, id, args, argc, result);
}
