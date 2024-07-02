#include "avm1media.h"
#include "scripting/class.h"

using namespace lightspark;

void AVM1Video::sinit(Class_base* c)
{
	Video::sinit(c);
	Video::AVM1SetupMethods(c);
	c->setDeclaredMethodByQName("attachVideo","",c->getSystemState()->getBuiltinFunction(attachNetStream),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("smoothing","",c->getSystemState()->getBuiltinFunction(_getter_smoothing),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("smoothing","",c->getSystemState()->getBuiltinFunction(_setter_smoothing),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("_width","",c->getSystemState()->getBuiltinFunction(Video::_getWidth),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("_width","",c->getSystemState()->getBuiltinFunction(Video::_setWidth),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("_height","",c->getSystemState()->getBuiltinFunction(Video::_getHeight),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("_height","",c->getSystemState()->getBuiltinFunction(Video::_setHeight),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("width","",c->getSystemState()->getBuiltinFunction(Video::_getVideoWidth),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("height","",c->getSystemState()->getBuiltinFunction(Video::_getVideoHeight),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("clear","",c->getSystemState()->getBuiltinFunction(clear),NORMAL_METHOD,true);
}
