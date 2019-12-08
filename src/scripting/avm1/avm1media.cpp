#include "avm1media.h"
#include "scripting/class.h"

using namespace lightspark;

void AVM1Video::sinit(Class_base* c)
{
	Video::sinit(c);
	Video::AVM1SetupMethods(c);
	c->setDeclaredMethodByQName("attachVideo","",Class<IFunction>::getFunction(c->getSystemState(),attachNetStream),NORMAL_METHOD,true);
}
