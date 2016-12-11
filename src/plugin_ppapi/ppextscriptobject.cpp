/**************************************************************************
    Lighspark, a free flash player implementation

    Copyright (C) 2016 Ludger Krämer <dbluelle@onlinehome.de>

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
#include "ppextscriptobject.h"
#include "plugin_ppapi/plugin.h"

using namespace lightspark;

ppExtScriptObject::ppExtScriptObject(ppPluginInstance *_instance):instance(_instance),marshallExceptions(false)
{
    
}

bool ppExtScriptObject::removeMethod(const ExtIdentifier &id)
{
	std::map<ExtIdentifier, lightspark::ExtCallback*>::iterator it = methods.find(id);
	if(it == methods.end())
		return false;

	delete (*it).second;
	methods.erase(it);
	return true;
}

const ExtVariant &ppExtScriptObject::getProperty(const ExtIdentifier &id) const
{
	std::map<ExtIdentifier, ExtVariant>::const_iterator it = properties.find(id);
	assert(it != properties.end());
	return it->second;
}

bool ppExtScriptObject::removeProperty(const ExtIdentifier &id)
{
	std::map<ExtIdentifier, ExtVariant>::iterator it = properties.find(id);
	if(it == properties.end())
		return false;

	properties.erase(it);
	return true;
}

bool ppExtScriptObject::enumerate(ExtIdentifier ***ids, uint32_t *count) const
{
    LOG(LOG_NOT_IMPLEMENTED,"ppExtScriptObject::enumerate");
    return false;
}

bool ppExtScriptObject::callExternal(const ExtIdentifier &id, const ExtVariant **args, uint32_t argc, ASObject **result)
{
	std::map<const ExtObject*, ASObject*> asObjectsMap;
	std::string argsString;
	std::string argsString2;
	for(uint32_t i=0;i<argc;i++)
	{
		char buf[20];
		if((i+1)==argc)
			snprintf(buf,20,"a%u",i);
		else
			snprintf(buf,20,"a%u,",i);
		argsString2 += buf;
		std::vector<ASObject *> path;
		tiny_string spaces;
		tiny_string filter;
		argsString += args[i]->getASObject(asObjectsMap)->toJSON(path,NULL,spaces,filter);
		if (i+1 < argc)
			argsString += ",";
	}
	std::string scriptString = "JSON.stringify(";
	scriptString += "function(";
	scriptString += argsString2;
	scriptString += ") { return (" + id.getString();
	scriptString += ")(" + argsString + "); });";
	*result = instance->executeScript(scriptString);
	LOG(LOG_INFO,"result:"<<(*result)->toDebugString());
	return true;
}

void ppExtScriptObject::setException(const std::string &message) const
{
}
