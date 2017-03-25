#ifndef PPEXTSCRIPTOBJECT_H
#define PPEXTSCRIPTOBJECT_H
#include "backends/extscriptobject.h"
#include "ppapi/c/pp_var.h"

namespace lightspark
{
class ppPluginInstance;


class ppExtScriptObject : public ExtScriptObject
{
private:
	ppPluginInstance* instance;
	void sendExternalCallSignal();
public:
	PP_Var ppScriptObject;
	ppExtScriptObject(ppPluginInstance* _instance,SystemState* sys);
	
	// Enumeration
	ExtIdentifier* createEnumerationIdentifier(const ExtIdentifier& id) const;

	void setException(const std::string& message) const;
	
	void callAsync(HOST_CALL_DATA* data);

	// This is called from hostCallHandler() via doHostCall(EXTERNAL_CALL, ...)
	virtual bool callExternalHandler(const char* scriptString, const lightspark::ExtVariant** args, uint32_t argc, lightspark::ASObject** result);
	
	bool invoke(const ExtIdentifier &method_name, uint32_t argc, const ExtVariant **objArgs, PP_Var* result);
	ppPluginInstance* getInstance() const { return instance; }
	void handleExternalCall(ExtIdentifier& method_name, uint32_t argc, struct PP_Var* argv, struct PP_Var* exception);
	PP_Var externalcallresult;
};

}
#endif // PPEXTSCRIPTOBJECT_H
