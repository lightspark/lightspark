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

#include "vm.h"
#include "swf.h"
#include "actions.h"
#include "flashdisplay.h"
#include "compat.h"

using namespace std;
using namespace lightspark;

extern TLSDATA SystemState* sys;

VirtualMachine::VirtualMachine()
{
	sem_init(&mutex,0,1);
	//Global.setVariableByQName("MovieClip","",new MovieClip);
}

VirtualMachine::~VirtualMachine()
{
	sem_destroy(&mutex);
}

void VirtualMachine::setConstantPool(vector<STRING>& p)
{
	//sem_wait(&mutex);
	ConstantPool=p;
	//sem_post(&mutex);
}

STRING VirtualMachine::getConstantByIndex(int i)
{
	return ConstantPool[i];
}

