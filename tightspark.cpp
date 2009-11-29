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

#include "abc.h"

#include <fstream>
#include <sys/resource.h>

using namespace std;

__thread SystemState* sys=NULL;
__thread RenderThread* rt=NULL;
__thread ParseThread* pt=NULL;

extern int count_reuse;
extern int count_alloc;

int main(int argc, char* argv[])
{
	if(argc!=2)
	{
		cout << "Usage: " << argv[0] << " <file.abc>" << endl;
		exit(-1);
	}
	struct rlimit rl;
	getrlimit(RLIMIT_AS,&rl);
	rl.rlim_cur=1500000000;
	rl.rlim_max=rl.rlim_cur;
	//setrlimit(RLIMIT_AS,&rl);

	Log::initLogging(NOT_IMPLEMENTED);
	sys=new SystemState;
	sys->fps_prof=new fps_profiling();

	ifstream f(argv[1]);
	//TODO: Inject Context event
	ABCVm vm(sys);
	ABCContext* context=new ABCContext(&vm,f);
	vm.addEvent(NULL,new ABCContextInitEvent(context));
	vm.addEvent(NULL,new ShutdownEvent);
	vm.wait();
	delete context;
}
