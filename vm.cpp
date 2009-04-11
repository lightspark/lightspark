#include "vm.h"
#include "swf.h"
#include "actions.h"

using namespace std;

extern __thread SystemState* sys;

VirtualMachine::VirtualMachine()
{
	sem_init(&mutex,0,1);
}

void VirtualMachine::setConstantPool(vector<STRING>& p)
{
	//sem_wait(&mutex);
	ConstantPool=p;
	//sem_post(&mutex);
}

void VirtualMachine::registerFunction(FunctionTag* f)
{
	sem_wait(&mutex);
	Functions.push_back(f);
	sem_post(&mutex);
}

FunctionTag* VirtualMachine::getFunctionByName(const STRING& name)
{
	FunctionTag* ret=NULL;
	sem_wait(&mutex);
	for(int i=0;i<Functions.size();i++)
	{
		if(Functions[i]->getName()==name)
		{
			ret=Functions[i];
			break;
		}
	}
	sem_post(&mutex);
	return ret;
}

STRING VirtualMachine::getConstantByIndex(int i)
{
	return ConstantPool[i];
}

STRING ConstantReference::toString()
{
	return sys->vm.getConstantByIndex(index);
}

int ConstantReference::toInt()
{
	LOG(ERROR,"Cannot convert ConstRef to Int");
	return 0;
}
