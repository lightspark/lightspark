/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009,2010  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef VM_H
#define VM_H

#include "compat.h"
#include <semaphore.h>
#include <vector>
#include "swftypes.h"

namespace lightspark
{

//#include "asobjects.h"

/*class Stack
{
private:
	std::vector<ASObject*> data;
public:
	ASObject* operator()(int i){return *(data.rbegin()+i);}
	void push(ASObject* o){ data.push_back(o);}
	ASObject* pop()
	{
		if(data.size()==0)
			LOG(ERROR,"Empty stack");
		ASObject* ret=data.back();
		data.pop_back(); 
		return ret;
	}
};*/

class VirtualMachine
{
private:
	sem_t mutex;
	std::vector<STRING> ConstantPool;
public:
//	Stack stack;
	VirtualMachine();
	~VirtualMachine();
	void setConstantPool(std::vector<STRING>& p);
	STRING getConstantByIndex(int index);
//	ASObject Global;
};

};
#endif
