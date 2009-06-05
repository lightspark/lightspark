/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009  Alessandro Pignotti (a.pignotti@sssup.it)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#ifndef VM_H
#define VM_H

#include <semaphore.h>
#include <vector>
#include "swftypes.h"
class FunctionTag;
class Stack
{
private:
	std::vector<ISWFObject*> data;
public:
	ISWFObject* operator()(int i){return *(data.rbegin()+i);}
	void push(ISWFObject* o){ data.push_back(o);}
	ISWFObject* pop()
	{
		if(data.size()==0)
			LOG(ERROR,"Empty stack");
		ISWFObject* ret=data.back();
		data.pop_back(); 
		return ret;
	}
};

class VirtualMachine
{
private:
	sem_t mutex;
	std::vector<STRING> ConstantPool;
public:
	Stack stack;
	VirtualMachine();
	void setConstantPool(std::vector<STRING>& p);
	STRING getConstantByIndex(int index);
};

#endif
