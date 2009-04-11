#ifndef VM_H
#define VM_H

#include <semaphore.h>
#include <vector>
#include "swftypes.h"
class FunctionTag;
class STACK_OBJECT;

enum STACK_TYPE { OBJECT=0 };

class Stack
{
private:
	std::vector<STACK_OBJECT*> data;
public:
	STACK_OBJECT* operator()(int i){return *(data.rbegin()+i);}
	void push(STACK_OBJECT* o){ data.push_back(o);}
	STACK_OBJECT* pop()
	{
		STACK_OBJECT* ret=data.back(); data.pop_back(); 
		return ret;
	}
};

class VirtualMachine
{
private:
	sem_t mutex;
	std::vector<STRING> ConstantPool;
	std::vector<FunctionTag*> Functions;
public:
	Stack stack;
	VirtualMachine();
	void setConstantPool(std::vector<STRING>& p);
	STRING getConstantByIndex(int index);
	void registerFunction(FunctionTag* f);
	FunctionTag* getFunctionByName(const STRING& name);
};

#endif
