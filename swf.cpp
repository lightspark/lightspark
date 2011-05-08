/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2011  Alessandro Pignotti (a.pignotti@sssup.it)

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

#include <string>
#include <pthread.h>
#include <algorithm>
#include "scripting/abc.h"
#include "scripting/flashdisplay.h"
#include "scripting/flashevents.h"
#include "scripting/flashutils.h"
#include "swf.h"
#include "logger.h"
#include "parsing/streams.h"
#include "asobject.h"
#include "scripting/class.h"
#include "backends/rendering.h"

#include "SDL.h"
#include <GL/glew.h>
#ifdef ENABLE_CURL
#include <curl/curl.h>
#endif

#include "compat.h"

#ifdef ENABLE_LIBAVCODEC
extern "C" {
#include <libavcodec/avcodec.h>
}
#endif

#ifdef COMPILE_PLUGIN
#include <gdk/gdkx.h>
#endif

using namespace std;
using namespace lightspark;

extern TLSDATA ParseThread* pt;

RootMovieClip::RootMovieClip(LoaderInfo* li, bool isSys):mutex("mutexRoot"),initialized(false),parsingIsFailed(false),frameRate(0),
	mutexFrames("mutexFrame"),toBind(false),mutexChildrenClips("mutexChildrenClips")
{
	this->incRef();
	root=_MR(this);
	sem_init(&new_frame,0,0);
	if(li)
		li->incRef();
	loaderInfo=_MNR(li);
	//Reset framesLoaded, as there are still not available
	framesLoaded=0;

	//We set the protoype to a generic MovieClip
	if(!isSys)
		setPrototype(Class<MovieClip>::getClass());
}

RootMovieClip::~RootMovieClip()
{
	if(this!=sys)
		sys->removeJob(this);
	sem_destroy(&new_frame);
}

void RootMovieClip::parsingFailed()
{
	//The parsing is failed, we have no change to be ever valid
	parsingIsFailed=true;
	sem_post(&new_frame);
}

void RootMovieClip::bindToName(const tiny_string& n)
{
	assert_and_throw(toBind==false);
	toBind=true;
	bindName=n;
}

void RootMovieClip::setOrigin(const tiny_string& u, const tiny_string& filename)
{
	//We can use this origin to implement security measures.
	//It also allows loading files without specifying a fully qualified path.
	//Note that for plugins, this url is NOT the page url, but it is the swf file url.
	origin = URLInfo(u);
	//If this URL doesn't contain a filename, add the one passed as an argument (used in main.cpp)
	if(origin.getPathFile() == "" && filename != "")
		origin = origin.goToURL(filename);

	if(!loaderInfo.isNull())
	{
		loaderInfo->url=origin.getParsedURL();
		loaderInfo->loaderURL=origin.getParsedURL();
	}
}

void RootMovieClip::registerChildClip(MovieClip* clip)
{
	Locker l(mutexChildrenClips);
	clip->incRef();
	childrenClips.insert(clip);
}

void RootMovieClip::unregisterChildClip(MovieClip* clip)
{
	Locker l(mutexChildrenClips);
	if(childrenClips.erase(clip))
		clip->decRef();
}

void SystemState::registerEnterFrameListener(DisplayObject* obj)
{
	Locker l(mutexEnterFrameListeners);
	obj->incRef();
	enterFrameListeners.insert(obj);
}

void SystemState::unregisterEnterFrameListener(DisplayObject* obj)
{
	Locker l(mutexEnterFrameListeners);
	if(enterFrameListeners.erase(obj))
		obj->decRef();
}

void SystemState::registerTag(Tag* t)
{
	SpinlockLocker l(tagsStorageLock);
	tagsStorage.push_back(t);
}

void RootMovieClip::setOnStage(bool staged)
{
	Locker l(mutexFrames);

	//Execute the event registered for the first frame, if any
	if(!frameScripts[0].isNull())
	{
		_R<FunctionEvent> funcEvent(new FunctionEvent(_MR(frameScripts[0])));
		getVm()->addEvent(NullRef, funcEvent);
	}

	MovieClip::setOnStage(staged);
}

RootMovieClip* RootMovieClip::getInstance(LoaderInfo* li)
{
	RootMovieClip* ret=new RootMovieClip(li);
	ret->setPrototype(Class<MovieClip>::getClass());
	return ret;
}

void SystemState::staticInit()
{
	//Do needed global initialization
#ifdef ENABLE_CURL
	curl_global_init(CURL_GLOBAL_ALL);
#endif
#ifdef ENABLE_LIBAVCODEC
	avcodec_register_all();
	av_register_all();
#endif

	// seed the random number generator
	srand(time(NULL));
}

void SystemState::staticDeinit()
{
#ifdef ENABLE_CURL
	curl_global_cleanup();
#endif
}

SystemState::SystemState(ParseThread* parseThread, uint32_t fileSize):
	RootMovieClip(NULL,true),renderRate(0),error(false),shutdown(false),
	renderThread(NULL),inputThread(NULL),engine(NONE),fileDumpAvailable(0),
	waitingForDump(false),vmVersion(VMNONE),childPid(0),useGnashFallback(false),
	mutexEnterFrameListeners("mutexEnterFrameListeners"),invalidateQueueHead(NULL),
	invalidateQueueTail(NULL),showProfilingData(false),currentVm(NULL),
	useInterpreter(true),useJit(false),downloadManager(NULL),
	extScriptObject(NULL),scaleMode(SHOW_ALL)
{
	cookiesFileName[0]=0;
	//Create the thread pool
	sys=this;
	sem_init(&terminated,0,0);

	//Get starting time
	if(parseThread) //ParseThread may be null in tightspark
		parseThread->root=this;
	threadPool=new ThreadPool(this);
	timerThread=new TimerThread(this);
	config=new Config;
	config->load();
	pluginManager = new PluginManager;
	audioManager=new AudioManager(pluginManager);
	intervalManager=new IntervalManager();
	securityManager=new SecurityManager();
	loaderInfo=_MR(Class<LoaderInfo>::getInstanceS());
	//If the size is not known those will stay at zero
	loaderInfo->setBytesLoaded(fileSize);
	loaderInfo->setBytesTotal(fileSize);
	stage=Class<Stage>::getInstanceS();
	stage->incRef();
	setParent(_MR(stage));
	startTime=compat_msectiming();
	
	setPrototype(Class<MovieClip>::getClass());

	renderThread=new RenderThread(this);
	inputThread=new InputThread(this);
}

void SystemState::setDownloadedPath(const tiny_string& p)
{
	dumpedSWFPath=p;
	Locker l(mutex);
	if(waitingForDump)
		fileDumpAvailable.signal();
}

void SystemState::setCookies(const char* c)
{
	rawCookies=c;
}

void SystemState::parseParametersFromFlashvars(const char* v)
{
	if(useGnashFallback) //Save a copy of the string
		rawParameters=v;
	ASObject* params=Class<ASObject>::getInstanceS();
	//Add arguments to SystemState
	string vars(v);
	uint32_t cur=0;
	while(cur<vars.size())
	{
		int n1=vars.find('=',cur);
		if(n1==-1) //Incomplete parameters string, ignore the last
			break;

		int n2=vars.find('&',n1+1);
		if(n2==-1)
			n2=vars.size();

		string varName=vars.substr(cur,(n1-cur));

		//The variable value has to be urldecoded
		bool ok=true;
		string varValue;
		varValue.reserve(n2-n1); //The maximum lenght
		for(int j=n1+1;j<n2;j++)
		{
			if(vars[j]!='%')
				varValue.push_back(vars[j]);
			else
			{
				if((n2-j)<3) //Not enough characters
				{
					ok=false;
					break;
				}

				int t1=Math::hexToInt(vars[j+1]);
				int t2=Math::hexToInt(vars[j+2]);
				if(t1==-1 || t2==-1)
				{
					ok=false;
					break;
				}

				int c=(t1*16)+t2;
				varValue.push_back(c);
				j+=2;
			}
		}

		if(ok)
		{
			//cout << varName << ' ' << varValue << endl;
			params->setVariableByQName(varName.c_str(),"",
					lightspark::Class<lightspark::ASString>::getInstanceS(varValue));
		}
		cur=n2+1;
	}
	setParameters(params);
}

void SystemState::parseParametersFromFile(const char* f)
{
	ifstream i(f);
	if(!i)
	{
		LOG(LOG_ERROR,_("Parameters file not found"));
		return;
	}
	ASObject* ret=Class<ASObject>::getInstanceS();
	while(!i.eof())
	{
		string name,value;
		getline(i,name);
		getline(i,value);

		ret->setVariableByQName(name,"",Class<ASString>::getInstanceS(value));
	}
	setParameters(ret);
	i.close();
}

void SystemState::setParameters(ASObject* p)
{
	loaderInfo->setVariableByQName("parameters","",p);
}

void SystemState::stopEngines()
{
	if(threadPool)
		threadPool->forceStop();
	timerThread->wait();
	delete downloadManager;
	downloadManager=NULL;
	delete securityManager;
	securityManager=NULL;
	delete config;
	config=NULL;
	if(currentVm)
		currentVm->shutdown();
	delete threadPool;
	threadPool=NULL;
	//Now stop the managers
	delete audioManager;
	audioManager=NULL;
	delete pluginManager;
	pluginManager=NULL;
	
}

#ifdef PROFILING_SUPPORT
void SystemState::saveProfilingInformation()
{
	if(profOut.len())
	{
		ofstream f(profOut.raw_buf());
		f << "events: Time" << endl;
		for(uint32_t i=0;i<contextes.size();i++)
			contextes[i]->dumpProfilingData(f);
		f.close();
	}
}
#endif

void SystemState::finalize()
{
	RootMovieClip::finalize();
	invalidateQueueHead.reset();
	invalidateQueueTail.reset();
}

SystemState::~SystemState()
{
#ifdef PROFILING_SUPPORT
	saveProfilingInformation();
#endif
	//Kill our child process if any
	if(childPid)
	{
		assert(childPid!=getpid());
		kill_child(childPid);
	}
	//Delete the temporary cookies file
	if(cookiesFileName[0])
		unlink(cookiesFileName);
	assert(shutdown);

	renderThread->stop();
	//The thread pool should be stopped before everything
	if(threadPool)
		threadPool->forceStop();
	stopEngines();

	//Finalize ourselves
	finalize();

	//We are already being destroyed, make our prototype abandon us
	setPrototype(NULL);
	
	/*
	   Now we have to kill all objects that are still alive. This is done is two passes
	   1) call finalize on all objects, this will decRef all referenced objects
	   2) delete all the objects. Now destroying an object should not cause accesses to
	   	any other object */

	std::map<QName, Class_base*>::iterator it=classes.begin();
	for(;it!=classes.end();++it)
		it->second->finalize();

	//Destroy the contents of all the classes
	it=classes.begin();
	for(;it!=classes.end();++it)
	{
		//Make sure classes survives their cleanUp
		it->second->incRef();
		it->second->cleanUp();
	}

	//Destroy all registered classes
	it=classes.begin();
	for(;it!=classes.end();++it)
	{
		//DEPRECATED: to force garbage collection we delete all the classes
		delete it->second;
		//it->second->decRef()
	}
	//The Vm must be destroyed this late to clean all managed integers and numbers
	delete currentVm;

	//Some objects needs to remove the jobs when destroyed so keep the timerThread until now
	delete timerThread;
	timerThread=NULL;

	//Also destroy all tags
	for(unsigned int i=0;i<tagsStorage.size();i++)
		delete tagsStorage[i];

	delete renderThread;
	renderThread=NULL;
	delete inputThread;
	inputThread=NULL;
	sem_destroy(&terminated);
}

bool SystemState::isOnError() const
{
	return error;
}

bool SystemState::isShuttingDown() const
{
	return shutdown;
}

bool SystemState::shouldTerminate() const
{
	return shutdown || error;
}

void SystemState::setError(const string& c)
{
	//We record only the first error for easier fix and reporting
	if(!error)
	{
		error=true;
		errorCause=c;
		timerThread->stop();
		//Disable timed rendering
		removeJob(renderThread);
		renderThread->draw(true);
	}
}

void SystemState::setShutdownFlag()
{
	Locker l(mutex);
	if(currentVm)
	{
		_R<ShutdownEvent> e(new ShutdownEvent);
		currentVm->addEvent(NullRef,e);
	}
	shutdown=true;

	sem_post(&terminated);
}

void SystemState::wait()
{
	sem_wait(&terminated);
	if(engine==SDL)
	{
		SDL_Event event;
		event.type = SDL_USEREVENT;
		event.user.code = SHUTDOWN;
		event.user.data1 = 0;
		event.user.data1 = 0;
		SDL_PushEvent(&event);
	}
	//Acquire the mutex to sure that the engines are not being started right now
	Locker l(mutex);
	renderThread->wait();
	inputThread->wait();
	if(currentVm)
		currentVm->shutdown();
}

float SystemState::getRenderRate()
{
	return renderRate;
}

void SystemState::startRenderTicks()
{
	assert(renderThread);
	assert(renderRate);
	removeJob(renderThread);
	addTick(1000/renderRate,renderThread);
}

void SystemState::EngineCreator::execute()
{
	sys->createEngines();
}

void SystemState::EngineCreator::threadAbort()
{
	sys->fileDumpAvailable.signal();
	sys->getRenderThread()->forceInitialization();
}

#ifndef GNASH_PATH
#error No GNASH_PATH defined
#endif

void SystemState::enableGnashFallback()
{
	//Check if the gnash standalone executable is available
	ifstream f(GNASH_PATH);
	if(f)
		useGnashFallback=true;
	f.close();
}

#ifdef COMPILE_PLUGIN

void SystemState::delayedCreation(SystemState* th)
{
	NPAPI_params& p=th->npapiParams;
	//Create a plug in the XEmbed window
	p.container=gtk_plug_new((GdkNativeWindow)p.window);
	//Realize the widget now, as we need the X window
	gtk_widget_realize(p.container);
	//Show it now
	gtk_widget_show(p.container);
	gtk_widget_map(p.container);
	p.window=GDK_WINDOW_XWINDOW(p.container->window);
	XSync(p.display, False);
	//The lock is needed to avoid thread creation/destruction races
	Locker l(th->mutex);
	if(th->shutdown)
		return;
	th->renderThread->start(th->engine, &th->npapiParams);
	th->inputThread->start(th->engine, &th->npapiParams);
	//If the render rate is known start the render ticks
	if(th->renderRate)
		th->startRenderTicks();
}

void SystemState::delayedStopping(SystemState* th)
{
	sys=th;
	//This is called from the plugin, also kill the stream
	th->npapiParams.stopDownloaderHelper(th->npapiParams.helperArg);
	th->stopEngines();
	sys=NULL;
}

#endif

void SystemState::createEngines()
{
	Locker l(mutex);
	if(shutdown)
	{
		//A shutdown request has arrived before the creation of engines
		return;
	}
#ifdef COMPILE_PLUGIN
	//Check if we should fall back on gnash
	if(useGnashFallback && engine==GTKPLUG && vmVersion!=AVM2)
	{
		if(dumpedSWFPath.len()==0) //The path is not known yet
		{
			waitingForDump=true;
			l.unlock();
			fileDumpAvailable.wait();
			if(shutdown)
				return;
			l.lock();
		}
		LOG(LOG_NO_INFO,_("Trying to invoke gnash!"));
		//Dump the cookies to a temporary file
		strcpy(cookiesFileName,"/tmp/lightsparkcookiesXXXXXX");
		int file=mkstemp(cookiesFileName);
		if(file!=-1)
		{
			std::string data("Set-Cookie: " + rawCookies);
			size_t res;
			size_t written = 0;
			// Keep writing until everything we wanted to write actually got written
			do
			{
				res = write(file, data.c_str()+written, data.size()-written);
				if(res < 0)
				{
					LOG(LOG_ERROR, _("Error during writing of cookie file for Gnash"));
					break;
				}
				written += res;
			} while(written < data.size());
			close(file);
			setenv("GNASH_COOKIES_IN", cookiesFileName, 1);
		}
		else
			cookiesFileName[0]=0;
		sigset_t oldset;
		sigset_t set;
		sigfillset(&set);
		//Blocks all signal to avoid terminating while forking with the browser signal handlers on
		pthread_sigmask(SIG_SETMASK, &set, &oldset);

		// This will be used to pipe the SWF's data to Gnash's stdin
		int gnashStdin[2];
		pipe(gnashStdin);

		childPid=fork();
		if(childPid==-1)
		{
			//Restore handlers
			pthread_sigmask(SIG_SETMASK, &oldset, NULL);
			LOG(LOG_ERROR,_("Child process creation failed, lightspark continues"));
			childPid=0;
		}
		else if(childPid==0) //Child process scope
		{
			// Close write end of Gnash's stdin pipe, we will only read
			close(gnashStdin[1]);
			// Point stdin to the read end of Gnash's stdin pipe
			dup2(gnashStdin[0], fileno(stdin));
			// Close the read end of the pipe
			close(gnashStdin[0]);

			//Allocate some buffers to store gnash arguments
			char bufXid[32];
			char bufWidth[32];
			char bufHeight[32];
			snprintf(bufXid,32,"%lu",npapiParams.window);
			snprintf(bufWidth,32,"%u",npapiParams.width);
			snprintf(bufHeight,32,"%u",npapiParams.height);
			string params("FlashVars=");
			params+=rawParameters;
			char *const args[] =
			{
				strdup("gnash"), //argv[0]
				strdup("-x"), //Xid
				bufXid,
				strdup("-j"), //Width
				bufWidth,
				strdup("-k"), //Height
				bufHeight,
				strdup("-u"), //SWF url
				strdup(origin.getParsedURL().raw_buf()),
				strdup("-P"), //SWF parameters
				strdup(params.c_str()),
				strdup("-vv"),
				strdup("-"),
				NULL
			};

			// Print out an informative message about how we are invoking Gnash
			{
				int i = 1;
				std::string argsStr = "";
				while(args[i] != NULL)
				{
					argsStr += " ";
					argsStr += args[i];
					i++;
				}
				cerr << "Invoking '" << GNASH_PATH << argsStr << " < " << dumpedSWFPath.raw_buf() << "'" << endl;
			}

			//Avoid calling browser signal handler during the short time between enabling signals and execve
			sigaction(SIGTERM, NULL, NULL);
			//Restore handlers
			pthread_sigmask(SIG_SETMASK, &oldset, NULL);
			execve(GNASH_PATH, args, environ);
			//If we are here execve failed, print an error and die
			cerr << _("Execve failed, content will not be rendered") << endl;
			exit(0);
		}
		else //Parent process scope
		{
			// Pass the SWF's data to Gnash
			{
				// Close read end of stdin pipe, we will only write to it.
				close(gnashStdin[0]);
				// Open the SWF file
				std::ifstream swfStream(dumpedSWFPath.raw_buf(), ios::binary);
				// Read the SWF file and write it to Gnash's stdin
				char data[1024];
				std::streamsize written, ret;
				bool stop = false;
				while(!swfStream.eof() && !swfStream.fail() && !stop)
				{
					swfStream.read(data, 1024);
					// Keep writing until everything we wanted to write actually got written
					written = 0;
					do
					{
						ret = write(gnashStdin[1], data+written, swfStream.gcount()-written);
						if(ret < 0)
						{
							LOG(LOG_ERROR, _("Error during writing of SWF file to Gnash"));
							stop = true;
							break;
						}
						written += ret;
					} while(written < swfStream.gcount());
				}
				// Close the write end of Gnash's stdin, signalling EOF to Gnash.
				close(gnashStdin[1]);
				// Close the SWF file
				swfStream.close();
			}

			//Restore handlers
			pthread_sigmask(SIG_SETMASK, &oldset, NULL);
			//Engines should not be started, stop everything
			l.unlock();
			//We cannot stop the engines now, as this is inside a ThreadPool job
			npapiParams.helper(npapiParams.helperArg, (helper_t)delayedStopping, this);
			return;
		}
	}
#else 
	//COMPILE_PLUGIN not defined
	if(useGnashFallback && engine==GTKPLUG && vmVersion!=AVM2)
	{
		throw new UnsupportedException("GNASH fallback not available when not built with COMPILE_PLUGIN");
	}
#endif

	if(engine==GTKPLUG) //The engines must be created in the context of the main thread
	{
#ifdef COMPILE_PLUGIN
		npapiParams.helper(npapiParams.helperArg, (helper_t)delayedCreation, this);
#else
		throw new UnsupportedException("Plugin engine not available when not built with COMPILE_PLUGIN");
#endif
	}
	else //SDL engine
	{
		renderThread->start(engine, NULL);
		inputThread->start(engine, NULL);
		//If the render rate is known start the render ticks
		if(renderRate)
			startRenderTicks();
	}
	l.unlock();
	renderThread->waitForInitialization();
	l.lock();
	//As we lost the lock the shutdown procesure might have started
	if(shutdown)
		return;
	if(currentVm)
		currentVm->start();
}

void SystemState::needsAVM2(bool n)
{
	Locker l(mutex);
	assert(currentVm==NULL);
	//Create the virtual machine if needed
	if(n)
	{
		vmVersion=AVM2;
		LOG(LOG_NO_INFO,_("Creating VM"));
		currentVm=new ABCVm(this);
	}
	else
		vmVersion=AVM1;
	if(engine)
		addJob(new EngineCreator);
}

void SystemState::setParamsAndEngine(ENGINE e, NPAPI_params* p)
{
	Locker l(mutex);
	if(p)
		npapiParams=*p;
	engine=e;
	if(vmVersion)
		addJob(new EngineCreator);
}

void SystemState::setRenderRate(float rate)
{
	Locker l(mutex);
	if(renderRate>=rate)
		return;
	
	//The requested rate is higher, let's reschedule the job
	renderRate=rate;
	startRenderTicks();
}

void SystemState::tick()
{
	RootMovieClip::tick();
	{
		SpinlockLocker l(profileDataSpinlock);
		list<ThreadProfile>::iterator it=profilingData.begin();
		for(;it!=profilingData.end();++it)
			it->tick();
	}
	//Send enterFrame events, if needed
	{
		Locker l(mutexEnterFrameListeners);
		if(!enterFrameListeners.empty())
		{
			_R<Event> e(Class<Event>::getInstanceS("enterFrame"));
			auto it=enterFrameListeners.begin();
			for(;it!=enterFrameListeners.end();it++)
			{
				(*it)->incRef();
				getVm()->addEvent(_MR(*it),e);
			}
		}
	}
	//Enter frame should be sent to the stage too
	if(stage->hasEventListener("enterFrame"))
	{
		_R<Event> e(Class<Event>::getInstanceS("enterFrame"));
		stage->incRef();
		getVm()->addEvent(_MR(stage),e);
	}
}

void SystemState::addJob(IThreadJob* j)
{
	threadPool->addJob(j);
}

void SystemState::addTick(uint32_t tickTime, ITickJob* job)
{
	timerThread->addTick(tickTime,job);
}

void SystemState::addWait(uint32_t waitTime, ITickJob* job)
{
	timerThread->addWait(waitTime,job);
}

bool SystemState::removeJob(ITickJob* job)
{
	return timerThread->removeJob(job);
}

ThreadProfile* SystemState::allocateProfiler(const lightspark::RGB& color)
{
	SpinlockLocker l(profileDataSpinlock);
	profilingData.push_back(ThreadProfile(color,100));
	ThreadProfile* ret=&profilingData.back();
	return ret;
}

void SystemState::addToInvalidateQueue(_R<DisplayObject> d)
{
	SpinlockLocker l(invalidateQueueLock);
	//Check if the object is already in the queue
	if(!d->invalidateQueueNext.isNull() || d==invalidateQueueTail)
		return;
	if(invalidateQueueHead==NULL)
		invalidateQueueHead=invalidateQueueTail=d;
	else
	{
		d->invalidateQueueNext=invalidateQueueHead;
		invalidateQueueHead=d;
	}
}

void SystemState::flushInvalidationQueue()
{
	SpinlockLocker l(invalidateQueueLock);
	_NR<DisplayObject> cur=invalidateQueueHead;
	while(!cur.isNull())
	{
		cur->invalidate();
		_NR<DisplayObject> next=cur->invalidateQueueNext;
		cur->invalidateQueueNext=NullRef;
		cur=next;
	}
	invalidateQueueHead=NullRef;
	invalidateQueueTail=NullRef;
}

#ifdef PROFILING_SUPPORT
void SystemState::setProfilingOutput(const tiny_string& t)
{
	profOut=t;
}


const tiny_string& SystemState::getProfilingOutput() const
{
	return profOut;
}
#endif

void ThreadProfile::setTag(const std::string& t)
{
	Locker locker(mutex);
	if(data.empty())
		data.push_back(ProfilingData(tickCount,0));
	
	data.back().tag=t;
}

void ThreadProfile::accountTime(uint32_t time)
{
	Locker locker(mutex);
	if(data.empty() || data.back().index!=tickCount)
		data.push_back(ProfilingData(tickCount, time));
	else
		data.back().timing+=time;
}

void ThreadProfile::tick()
{
	Locker locker(mutex);
	tickCount++;
	//Purge first sample if the second is already old enough
	if(data.size()>2 && (tickCount-data[1].index)>uint32_t(len))
	{
		if(!data[0].tag.empty() && data[1].tag.empty())
			data[0].tag.swap(data[1].tag);
		data.pop_front();
	}
}

void ThreadProfile::plot(uint32_t maxTime, FTFont* font)
{
	if(data.size()<=1)
		return;

	Locker locker(mutex);
	RECT size=sys->getFrameSize();
	int width=size.Xmax/20;
	int height=size.Ymax/20;
	
	int32_t start=tickCount-len;
	if(int32_t(data[0].index-start)>0)
		start=data[0].index;
	
	glPushAttrib(GL_TEXTURE_BIT | GL_LINE_BIT);
	glColor3ub(color.Red,color.Green,color.Blue);
	glDisable(GL_TEXTURE_2D);
	glLineWidth(2);

	glBegin(GL_LINE_STRIP);
		for(unsigned int i=0;i<data.size();i++)
		{
			int32_t relx=int32_t(data[i].index-start)*width/len;
			glVertex2i(relx,data[i].timing*height/maxTime);
		}
	glEnd();
	glPopAttrib();
	
	//Draw tags
	string* curTag=NULL;
	int curTagX=0;
	int curTagY=maxTime;
	int curTagLen=0;
	int curTagH=0;
	for(unsigned int i=0;i<data.size();i++)
	{
		int32_t relx=int32_t(data[i].index-start)*width/len;
		if(!data[i].tag.empty())
		{
			//New tag, flush the old one if present
			if(curTag)
				font->Render(curTag->c_str() ,-1,FTPoint(curTagX,imax(curTagY-curTagH,0)));
			//Measure tag
			FTBBox tagBox=font->BBox(data[i].tag.c_str(),-1);
			curTagLen=(tagBox.Upper()-tagBox.Lower()).X();
			curTagH=(tagBox.Upper()-tagBox.Lower()).Y();
			curTag=&data[i].tag;
			curTagX=relx;
			curTagY=maxTime;
		}
		if(curTag)
		{
			if(relx<(curTagX+curTagLen))
				curTagY=imin(curTagY,data[i].timing*height/maxTime);
			else
			{
				//Tag is before this sample
				font->Render(curTag->c_str() ,-1,FTPoint(curTagX,imax(curTagY-curTagH,0)));
				curTag=NULL;
			}
		}
	}
}

ParseThread::ParseThread(RootMovieClip* r,istream& in):f(in),zlibFilter(NULL),backend(NULL),isEnded(false),fileType(NONE),root(NULL),
	version(0),useAVM2(false)
{
	root=r;
	sem_init(&ended,0,0);
}

ParseThread::~ParseThread()
{
	if(zlibFilter)
	{
		//Restore the istream
		f.rdbuf(backend);
		delete zlibFilter;
	}
	sem_destroy(&ended);
}

void ParseThread::checkType(uint8_t c1, uint8_t c2, uint8_t c3, uint8_t c4)
{
	if(c1=='F' && c2=='W' && c3=='S')
	{
		fileType=SWF;
		version=c4;
		root->version=version;
	}
	else if(c1=='C' && c2=='W' && c3=='S')
	{
		fileType=COMPRESSED_SWF;
		version=c4;
		root->version=version;
	}
	else if((c1&0x80) && c2=='P' && c3=='N' && c4=='G')
		fileType=PNG;
	else if(c1==0xff && c2==0xd8 && c3==0xff && c4==0xe0)
		fileType=JPEG;
	else if(c1=='G' && c2=='I' && c3=='F' && c4=='8')
		fileType=GIF;
}

void ParseThread::parseSWFHeader()
{
	UI32_SWF FileLength;
	RECT FrameSize;
	UI16_SWF FrameRate;
	UI16_SWF FrameCount;

	f >> FileLength;
	//Enable decompression if needed
	if(fileType==SWF)
		LOG(LOG_NO_INFO, _("Uncompressed SWF file: Version ") << (int)version);
	else if(fileType==COMPRESSED_SWF)
	{
		LOG(LOG_NO_INFO, _("Compressed SWF file: Version ") << (int)version);
		//The file is compressed, create a filtering streambuf
		backend=f.rdbuf();
		f.rdbuf(new zlib_filter(backend));
	}

	f >> FrameSize >> FrameRate >> FrameCount;

	root->fileLength=FileLength;
	float frameRate=FrameRate;
	frameRate/=256;
	LOG(LOG_NO_INFO,_("FrameRate ") << frameRate);
	root->setFrameRate(frameRate);
	//TODO: setting render rate should be done when the clip is added to the displaylist
	sys->setRenderRate(frameRate);
	root->setFrameSize(FrameSize);
	root->setFrameCount(FrameCount);
}

void ParseThread::execute()
{
	pt=this;
	try
	{
		UI8 Signature[4];
		f >> Signature[0] >> Signature[1] >> Signature[2] >> Signature[3];
		checkType(Signature[0],Signature[1],Signature[2],Signature[3]);
		if(fileType==NONE)
			throw ParseException("Not a supported file");
		if(fileType==PNG || fileType==JPEG || fileType==GIF)
		{
			//Not really supported
			pt=NULL;
			sem_post(&ended);
			return;
		}
		parseSWFHeader();

		//Create a top level TagFactory
		TagFactory factory(f, true);
		bool done=false;
		bool empty=true;
		while(!done)
		{
			Tag* tag=factory.readTag();
			sys->registerTag(tag);
			switch(tag->getType())
			{
				case END_TAG:
				{
					LOG(LOG_NO_INFO,_("End of parsing @ ") << f.tellg());
					if(!empty)
						root->commitFrame(false);
					else
						root->revertFrame();
					done=true;
					root->check();
					break;
				}
				case DICT_TAG:
				{
					DictionaryTag* d=static_cast<DictionaryTag*>(tag);
					d->setLoadedFrom(root);
					root->addToDictionary(d);
					break;
				}
				case DISPLAY_LIST_TAG:
					root->addToFrame(static_cast<DisplayListTag*>(tag));
					empty=false;
					break;
				case SHOW_TAG:
					root->commitFrame(true);
					empty=true;
					break;
				case CONTROL_TAG:
					root->addToFrame(static_cast<ControlTag*>(tag));
					empty=false;
					break;
				case FRAMELABEL_TAG:
					root->labelCurrentFrame(static_cast<FrameLabelTag*>(tag)->Name);
					empty=false;
					break;
				case TAG:
					//Not yet implemented tag, ignore it
					break;
			}
			if(sys->shouldTerminate() || aborting)
				break;
		}
	}
	catch(LightsparkException& e)
	{
		LOG(LOG_ERROR,_("Exception in ParseThread ") << e.cause);
		root->parsingFailed();
		sys->setError(e.cause);
	}
	pt=NULL;

	sem_post(&ended);
}

void ParseThread::threadAbort()
{
	//Tell the our RootMovieClip that the parsing is ending
	root->parsingFailed();
}

void RootMovieClip::initialize()
{
	//NOTE: No references to frames must be done here!
	if(!initialized && sys->currentVm)
	{
		if(sys->currentVm)
		{
			initialized=true;
			//Let's see if we have to bind the root movie clip itself
			if(bindName.len()) //The object is constructed after binding
			{
				this->incRef();
				sys->currentVm->addEvent(NullRef,_MR(new BindClassEvent(_MR(this),bindName,BindClassEvent::ISROOT)));
				sys->currentVm->addEvent(NullRef,_MR(new SysOnStageEvent()));
			}
			else
			{
				setConstructed();
				setOnStage(true);
			}

			//Now signal the completion for this root
			loaderInfo->sendInit();
		}
		else
		{
			setConstructed();
			setOnStage(true);
		}
	}
}

bool RootMovieClip::getBounds(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const
{
	RECT f=getFrameSize();
	xmin=0;
	ymin=0;
	xmax=f.Xmax/20;
	ymax=f.Ymax/20;
	return true;
}

void RootMovieClip::Render(bool maskEnabled)
{
	Locker l(mutexFrames);
	while(1)
	{
		//Check if the next frame we are going to play is available
		if(state.FP<frames.size())
			break;

		l.unlock();
		sem_wait(&new_frame);
		if(parsingIsFailed)
			return;
		l.lock();
	}

	MovieClip::Render(maskEnabled);
}

void RootMovieClip::setFrameCount(int f)
{
	Locker l(mutexFrames);
	setTotalFrames(f);
	state.max_FP=f;
	//TODO, maybe the next is a regular assert
	assert_and_throw(cur_frame==&frames.back());
	//Reserving guarantees than the vector is never invalidated
	//Add 1 as the commit procedure will add one more (see commitFrame)
	frames.reserve(f+1);
	cur_frame=&frames.back();
}

void RootMovieClip::setFrameSize(const lightspark::RECT& f)
{
	frameSize=f;
	assert_and_throw(f.Xmin==0 && f.Ymin==0);
}

lightspark::RECT RootMovieClip::getFrameSize() const
{
	return frameSize;
}

void RootMovieClip::setFrameRate(float f)
{
	frameRate=f;
}

float RootMovieClip::getFrameRate() const
{
	return frameRate;
}

void RootMovieClip::addToDictionary(DictionaryTag* r)
{
	SpinlockLocker l(dictSpinlock);
	dictionary.push_back(r);
}

void RootMovieClip::addToFrame(DisplayListTag* t)
{
	Locker l(mutex);
	MovieClip::addToFrame(t);
}

void RootMovieClip::labelCurrentFrame(const STRING& name)
{
	Locker l(mutexFrames);
	frames.back().Label=(const char*)name;
}

void RootMovieClip::addToFrame(ControlTag* t)
{
	cur_frame->controls.push_back(t);
}

void RootMovieClip::commitFrame(bool another)
{
	Locker l(mutexFrames);
	framesLoaded=frames.size();
	if(another)
	{
		frames.emplace_back();
		cur_frame=&frames.back();
	}
	else
		cur_frame=NULL;

	if(framesLoaded==1)
	{
		//Let's initialize the first frame of this movieclip
		bootstrap();
		//From now on no more references to frames must be done! See NOTE in initialize
		l.unlock();
		//Root movie clips are initialized now, after the first frame is really ready 
		initialize();

		//When the first frame is committed the frame rate is known
		sys->addTick(1000/frameRate,this);
	}
	sem_post(&new_frame);
}

void RootMovieClip::revertFrame()
{
	Locker l(mutexFrames);
	//TODO: The next should be a regular assert
	assert_and_throw(frames.size() && framesLoaded==(frames.size()-1));
	frames.pop_back();
	cur_frame=NULL;
}

RGB RootMovieClip::getBackground()
{
	return Background;
}

void RootMovieClip::setBackground(const RGB& bg)
{
	Background=bg;
}

DictionaryTag* RootMovieClip::dictionaryLookup(int id)
{
	SpinlockLocker l(dictSpinlock);
	list< DictionaryTag*>::iterator it = dictionary.begin();
	for(;it!=dictionary.end();++it)
	{
		if((*it)->getId()==id)
			break;
	}
	if(it==dictionary.end())
	{
		LOG(LOG_ERROR,_("No such Id on dictionary ") << id);
		throw RunTimeException("Could not find an object on the dictionary");
	}
	DictionaryTag* ret=*it;
	return ret;
}

void RootMovieClip::tick()
{
	//Frame advancement may cause exceptions
	try
	{
		advanceFrame();
		//advanceFrame may be blocking (if Frame init happens)
		Locker l(mutexChildrenClips);
		vector<MovieClip*> tmpChildren(childrenClips.begin(),childrenClips.end());
		for(uint32_t i=0;i<tmpChildren.size();i++)
			tmpChildren[i]->incRef();
		l.unlock();
		//Advance all the children to the next frame using the temporary copy
		for(uint32_t i=0;i<tmpChildren.size();i++)
		{
			tmpChildren[i]->advanceFrame();
			//Release when done
			tmpChildren[i]->decRef();
		}
	}
	catch(LightsparkException& e)
	{
		LOG(LOG_ERROR,_("Exception in RootMovieClip::tick ") << e.cause);
		sys->setError(e.cause);
	}
}

/*ASObject* RootMovieClip::getVariableByQName(const tiny_string& name, const tiny_string& ns, ASObject*& owner)
{
	sem_wait(&mutex);
	ASObject* ret=ASObject::getVariableByQName(name, ns, owner);
	sem_post(&mutex);
	return ret;
}

void RootMovieClip::setVariableByMultiname(multiname& name, ASObject* o)
{
	sem_wait(&mutex);
	ASObject::setVariableByMultiname(name,o);
	sem_post(&mutex);
}

void RootMovieClip::setVariableByQName(const tiny_string& name, const tiny_string& ns, ASObject* o)
{
	sem_wait(&mutex);
	ASObject::setVariableByQName(name,ns,o);
	sem_post(&mutex);
}

void RootMovieClip::setVariableByString(const string& s, ASObject* o)
{
	abort();
	//TODO: ActionScript2 support
	string sub;
	int f=0;
	int l=0;
	ASObject* target=this;
	for(l;l<s.size();l++)
	{
		if(s[l]=='.')
		{
			sub=s.substr(f,l-f);
			ASObject* next_target;
			if(f==0 && sub=="__Packages")
			{
				next_target=&sys->cur_render_thread->vm.Global;
				owner=&sys->cur_render_thread->vm.Global;
			}
			else
				next_target=target->getVariableByQName(sub.c_str(),"",owner);

			f=l+1;
			if(!owner)
			{
				next_target=new Package;
				target->setVariableByQName(sub.c_str(),"",next_target);
			}
			target=next_target;
		}
	}
	sub=s.substr(f,l-f);
	target->setVariableByQName(sub.c_str(),"",o);
}*/


