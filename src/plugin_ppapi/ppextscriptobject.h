#ifndef PPEXTSCRIPTOBJECT_H
#define PPEXTSCRIPTOBJECT_H
#include "backends/extscriptobject.h"

namespace lightspark
{
class ppPluginInstance;

class ppExtScriptObject : public ExtScriptObject
{
private:
	ppPluginInstance* instance;
	// True if exceptions should be marshalled to the container
	bool marshallExceptions;

	// This map stores this object's methods & properties
	// If an entry is set with a ExtIdentifier or ExtVariant,
	// they get converted to NPIdentifierObject or NPVariantObject by copy-constructors.
	std::map<ExtIdentifier, ExtVariant> properties;
	std::map<ExtIdentifier, lightspark::ExtCallback*> methods;
public:
	ppExtScriptObject(ppPluginInstance* _instance);
	
	/* ExtScriptObject interface */
	// Methods
	bool hasMethod(const ExtIdentifier& id) const
	{
		return methods.find(id) != methods.end();
	}
	void setMethod(const ExtIdentifier& id, lightspark::ExtCallback* func)
	{
		methods[id] = func;
	}
	bool removeMethod(const ExtIdentifier& id);

	// Properties
	bool hasProperty(const ExtIdentifier& id) const
	{
		return properties.find(id) != properties.end();
	}
	const ExtVariant& getProperty(const ExtIdentifier& id) const;
	void setProperty(const ExtIdentifier& id, const lightspark::ExtVariant& value)
	{
		properties[id] = value;
	}
	bool removeProperty(const ExtIdentifier& id);

	// Enumeration
	bool enumerate(ExtIdentifier*** ids, uint32_t* count) const;

	bool callExternal(const ExtIdentifier& id, const ExtVariant** args, uint32_t argc, ASObject** result);

	void setException(const std::string& message) const;
	void setMarshallExceptions(bool marshall) { marshallExceptions = marshall; }
	bool getMarshallExceptions() const { return marshallExceptions; }
	
};

}
#endif // PPEXTSCRIPTOBJECT_H
