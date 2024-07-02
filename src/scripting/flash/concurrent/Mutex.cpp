/**************************************************************************
    Lightspark, a free flash player implementation

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

#include "scripting/flash/concurrent/Mutex.h"
#include "scripting/class.h"
#include "scripting/argconv.h"

using namespace std;
using namespace lightspark;

ASMutex::ASMutex(ASWorker* wrk, Class_base* c):ASObject(wrk,c,T_OBJECT,SUBTYPE_MUTEX),lockcount(0)
{
	
}
void ASMutex::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_FINAL);
	c->setDeclaredMethodByQName("lock","",c->getSystemState()->getBuiltinFunction(_lock),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("unlock","",c->getSystemState()->getBuiltinFunction(_unlock),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("tryLock","",c->getSystemState()->getBuiltinFunction(_trylock),NORMAL_METHOD,true);
}

ASFUNCTIONBODY_ATOM(ASMutex,_constructor)
{
}
ASFUNCTIONBODY_ATOM(ASMutex,_lock)
{
	ASMutex* th=asAtomHandler::as<ASMutex>(obj);
	th->mutex.lock();
	th->lockcount++;
}
ASFUNCTIONBODY_ATOM(ASMutex,_unlock)
{
	ASMutex* th=asAtomHandler::as<ASMutex>(obj);
	th->mutex.unlock();
	th->lockcount--;
}
ASFUNCTIONBODY_ATOM(ASMutex,_trylock)
{
	ASMutex* th=asAtomHandler::as<ASMutex>(obj);
	asAtomHandler::setBool(ret,th->mutex.trylock());
}

