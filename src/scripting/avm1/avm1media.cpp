#include "avm1media.h"
#include "scripting/class.h"

using namespace lightspark;

void AVM1Video::sinit(Class_base* c)
{
	Video::sinit(c);
	Video::AVM1SetupMethods(c);
	c->setDeclaredMethodByQName("attachVideo","",Class<IFunction>::getFunction(c->getSystemState(),attachNetStream),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("smoothing","",Class<IFunction>::getFunction(c->getSystemState(),_getter_smoothing),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("smoothing","",Class<IFunction>::getFunction(c->getSystemState(),_setter_smoothing),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("_width","",Class<IFunction>::getFunction(c->getSystemState(),Video::_getWidth),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("_width","",Class<IFunction>::getFunction(c->getSystemState(),Video::_setWidth),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("_height","",Class<IFunction>::getFunction(c->getSystemState(),Video::_getHeight),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("_height","",Class<IFunction>::getFunction(c->getSystemState(),Video::_setHeight),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("width","",Class<IFunction>::getFunction(c->getSystemState(),Video::_getVideoWidth),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("height","",Class<IFunction>::getFunction(c->getSystemState(),Video::_getVideoHeight),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("clear","",Class<IFunction>::getFunction(c->getSystemState(),clear),NORMAL_METHOD,true);
}
