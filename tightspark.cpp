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

#include "abc.h"

#include <fstream>
#include <sys/resource.h>
#include "compat.h"

using namespace std;
using namespace lightspark;

TLSDATA DLL_PUBLIC SystemState* sys=NULL;
TLSDATA DLL_PUBLIC RenderThread* rt=NULL;
TLSDATA DLL_PUBLIC ParseThread* pt=NULL;

extern int count_reuse;
extern int count_alloc;

int main(int argc, char* argv[])
{
	char* fileName=NULL;
	bool useInterpreter=true;
	bool useJit=false;
	LOG_LEVEL log_level=LOG_NOT_IMPLEMENTED;

	for(int i=1;i<argc;i++)
	{
		if(strcmp(argv[i],"-ni")==0 || 
			strcmp(argv[i],"--disable-interpreter")==0)
		{
			useInterpreter=false;
		}
		else if(strcmp(argv[i],"-j")==0 || 
			strcmp(argv[i],"--enable-jit")==0)
		{
			useJit=true;
		}
		else if(strcmp(argv[i],"-l")==0 || 
			strcmp(argv[i],"--log-level")==0)
		{
			i++;
			if(i==argc)
			{
				fileName=NULL;
				break;
			}

			log_level=(LOG_LEVEL)atoi(argv[i]);
		}
		else
		{
			//No options flag, so set the swf file name
			if(fileName) //If already set, exit in error status
			{
				fileName=NULL;
				break;
			}
			fileName=argv[i];
		}
	}


	if(fileName==NULL)
	{
		cout << "Usage: " << argv[0] << " [--disable-interpreter|-ni] [--enable-jit|-j] [--log-level|-l 0-4] <file.abc>" << endl;
		exit(-1);
	}

	Log::initLogging(log_level);
	//NOTE: see SystemState declaration
	sys=new SystemState(NULL);

	//Set a bit of SystemState using parameters
	//One of useInterpreter or useJit must be enabled
	if(!(useInterpreter || useJit))
	{
		LOG(LOG_ERROR,"No execution model enabled");
		exit(-1);
	}
	sys->useInterpreter=useInterpreter;
	sys->useJit=useJit;

	sys->setOrigin(fileName);

#ifndef WIN32
	struct rlimit rl;
	getrlimit(RLIMIT_AS,&rl);
	rl.rlim_cur=1500000000;
	rl.rlim_max=rl.rlim_cur;
	//setrlimit(RLIMIT_AS,&rl);
#endif

	ABCVm vm(sys);
	sys->currentVm=&vm;
	ifstream f(fileName);
	ABCContext* context=new ABCContext(f);
	vm.addEvent(NULL,new ABCContextInitEvent(context));
	vm.addEvent(NULL,new ShutdownEvent);
	vm.wait();
}
