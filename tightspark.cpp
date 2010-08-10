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

#include "scripting/abc.h"

#include <fstream>
#ifndef _WIN32
// WINTODO: Proper CMake check
#include <sys/resource.h>
#endif
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
	std::vector<char*> fileNames;
	bool useInterpreter=true;
	bool useJit=false;
	LOG_LEVEL log_level=LOG_NOT_IMPLEMENTED;
	bool error=false;

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
				error=true;
				break;
			}

			log_level=(LOG_LEVEL)atoi(argv[i]);
		}
		else
		{
			//More than a file is allowed in tightspark
			fileNames.push_back(argv[i]);
		}
	}


	if(fileNames.empty() || error)
	{
		cout << "Usage: " << argv[0] << " [--disable-interpreter|-ni] [--enable-jit|-j] [--log-level|-l 0-4] <file.abc> [<file2.abc>]" << endl;
		exit(-1);
	}

	Log::initLogging(log_level);
	SystemState::staticInit();
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

	sys->setOrigin(fileNames[0]);

#ifndef WIN32
	struct rlimit rl;
	getrlimit(RLIMIT_AS,&rl);
	rl.rlim_cur=1500000000;
	rl.rlim_max=rl.rlim_cur;
	//setrlimit(RLIMIT_AS,&rl);
#endif

	ABCVm* vm=new ABCVm(sys);
	sys->currentVm=vm;
	vector<ABCContext*> contexts;
	for(unsigned int i=0;i<fileNames.size();i++)
	{
		ifstream f(fileNames[i]);
		ABCContext* context=new ABCContext(f);
		contexts.push_back(context);
		f.close();
		ABCContextInitEvent* e=new ABCContextInitEvent(context);
		vm->addEvent(NULL,e);
		e->decRef();
	}
	sys->setShutdownFlag();
	sys->wait();
	delete sys;
	//Clean up (mostly useful to clean up valgrind logs)
	for(unsigned int i=0;i<contexts.size();i++)
		delete contexts[i];
	SystemState::staticDeinit();
}
