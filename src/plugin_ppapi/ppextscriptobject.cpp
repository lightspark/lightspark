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

ppExtScriptObject::ppExtScriptObject(ppPluginInstance *_instance, SystemState *sys):ExtScriptObject(sys),instance(_instance)
{
	ppScriptObject= PP_MakeUndefined();
}

ExtIdentifier *ppExtScriptObject::createEnumerationIdentifier(const ExtIdentifier &id) const
{
	return new ExtIdentifier(id);
}

void ppExtScriptObject::setException(const std::string &message) const
{
	LOG(LOG_NOT_IMPLEMENTED,"ppExtScriptObject::setException:"<<message);
}

void ppExtScriptObject::callAsync(ExtScriptObject::HOST_CALL_DATA *data)
{
	instance->executeScriptAsync(data);
}

bool ppExtScriptObject::callExternalHandler(const char *scriptString, const ExtVariant **args, uint32_t argc, ASObject **result)
{
	return instance->executeScript(scriptString,args,argc,result);
}

bool ppExtScriptObject::invoke(const ExtIdentifier& method_name, uint32_t argc, const ExtVariant** objArgs, PP_Var *result)
{
	// This will hold our eventual callback result
	const lightspark::ExtVariant* objResult = NULL;
	bool res = doinvoke(method_name,objArgs,argc,objResult);

	// Delete converted arguments
	for(uint32_t i = 0; i < argc; i++)
		delete objArgs[i];

	if(objResult != NULL)
	{
		// Copy the result into the raw ppVariant result and delete intermediate result
		std::map<const ExtObject*, PP_Var> objectsMap;
		ppVariantObject::ExtVariantToppVariant(objectsMap,instance->getppInstance(),*objResult, *result);
		delete objResult;
	}
	return res;
}

void ppExtScriptObject::handleExternalCall(ExtIdentifier &method_name, uint32_t argc, PP_Var *argv, PP_Var *exception)
{
	setTLSSys(getSystemState());
	setTLSWorker(getSystemState()->worker);
	externalcallresult = PP_MakeUndefined();
	LOG(LOG_INFO,"ppExtScriptObject::handleExternalCall:"<< method_name.getString());
	std::map<int64_t, std::unique_ptr<ExtObject>> objectsMap;
	const ExtVariant** objArgs = g_newa(const ExtVariant*,argc);
	for (uint32_t i = 0; i < argc; i++)
	{
		objArgs[i]=new ppVariantObject(objectsMap,argv[i]);
	}
	invoke(method_name,argc,objArgs,&externalcallresult);
		
	LOG(LOG_INFO,"ppExtScriptObject::handleExternalCall done:"<<method_name.getString());
}
void ppExtScriptObject::sendExternalCallSignal()
{
	instance->signaliodone();
}
