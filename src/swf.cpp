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
#include "scripting/flash/events/flashevents.h"
#include "scripting/flash/utils/flashutils.h"
#include "swf.h"
#include "logger.h"
#include "parsing/streams.h"
#include "asobject.h"
#include "scripting/class.h"
#include "backends/audio.h"
#include "backends/config.h"
#include "backends/pluginmanager.h"
#include "backends/rendering.h"
#include "backends/security.h"
#include "backends/image.h"

#ifdef ENABLE_CURL
#include <curl/curl.h>
#endif

#include "compat.h"

#ifdef ENABLE_LIBAVCODEC
extern "C" {
#include <libavcodec/avcodec.h>
}
#endif

#include <gdk/gdkx.h>

using namespace std;
using namespace lightspark;

extern TLSDATA ParseThread* pt;

RootMovieClip::RootMovieClip(LoaderInfo* li, bool isSys):mutex("mutexRoot"),parsingIsFailed(false),frameRate(0),
	toBind(false), finishedLoading(false)
{
	if(li)
		li->incRef();
	loaderInfo=_MNR(li);

	//We set the protoype to a generic MovieClip
	if(!isSys)
		setClass(Class<MovieClip>::getClass());
}


void RootMovieClip::parsingFailed()
{
	//The parsing is failed, we have no change to be ever valid
	parsingIsFailed=true;
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

void SystemState::registerFrameListener(_R<DisplayObject> obj)
{
	Locker l(mutexFrameListeners);
	obj->incRef();
	frameListeners.insert(obj);
}

void SystemState::unregisterFrameListener(_R<DisplayObject> obj)
{
	Locker l(mutexFrameListeners);
	frameListeners.erase(obj);
}

void RootMovieClip::setOnStage(bool staged)
{
	MovieClip::setOnStage(staged);
}

RootMovieClip* RootMovieClip::getInstance(LoaderInfo* li)
{
	RootMovieClip* ret=new RootMovieClip(li);
	ret->setClass(Class<MovieClip>::getClass());
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
	delete Type::anyType;
	delete Type::voidType;
#ifdef ENABLE_CURL
	curl_global_cleanup();
#endif
}

SystemState::SystemState(ParseThread* parseThread, uint32_t fileSize):
	RootMovieClip(NULL,true),renderRate(0),error(false),shutdown(false),
	renderThread(NULL),inputThread(NULL),engineData(NULL),fileDumpAvailable(0),
	waitingForDump(false),vmVersion(VMNONE),childPid(0),useGnashFallback(false),
	parameters(NullRef),mutexFrameListeners("mutexFrameListeners"),
	invalidateQueueHead(NullRef),invalidateQueueTail(NullRef),showProfilingData(false),
	currentVm(NULL),useInterpreter(true),useJit(false),exitOnError(false),downloadManager(NULL),
	extScriptObject(NULL),scaleMode(SHOW_ALL)
{
	cookiesFileName[0]=0;
	//Create the thread pool
	sys=this;
	sem_init(&terminated,0,0);

	//Get starting time
	if(parseThread) //ParseThread may be null in tightspark
		parseThread->setRootMovie(this);
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
	this->incRef();
	stage->_addChildAt(_MR(this),0);
	startTime=compat_msectiming();
	
	setClass(Class<MovieClip>::getClass());

	//Override getStage as for SystemState that can't be null
	setDeclaredMethodByQName("stage","",Class<IFunction>::getFunction(_getStage),GETTER_METHOD,false);

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

static int hexToInt(char c)
{
	if(c>='0' && c<='9')
		return c-'0';
	else if(c>='a' && c<='f')
		return c-'a'+10;
	else if(c>='A' && c<='F')
		return c-'A'+10;
	else
		return -1;
}

void SystemState::parseParametersFromFlashvars(const char* v)
{
	if(useGnashFallback) //Save a copy of the string
		rawParameters=v;
	_R<ASObject> params=_MR(Class<ASObject>::getInstanceS());
	//Add arguments to SystemState
	string vars(v);
	uint32_t cur=0;
	char* pfile = getenv("LIGHTSPARK_PLUGIN_PARAMFILE");
        ofstream f;
	if(pfile)
		f.open(pfile);
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

				int t1=hexToInt(vars[j+1]);
				int t2=hexToInt(vars[j+2]);
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
			if(pfile)
				f << varName << endl << varValue << endl;
			params->setVariableByQName(varName,"",
					lightspark::Class<lightspark::ASString>::getInstanceS(varValue),DYNAMIC_TRAIT);
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
	_R<ASObject> ret=_MR(Class<ASObject>::getInstanceS());
	while(!i.eof())
	{
		string name,value;
		getline(i,name);
		getline(i,value);

		ret->setVariableByQName(name,"",Class<ASString>::getInstanceS(value),DYNAMIC_TRAIT);
	}
	setParameters(ret);
	i.close();
}

void SystemState::setParameters(_R<ASObject> p)
{
	parameters=p;
	loaderInfo->parameters = p;
}

_NR<ASObject> SystemState::getParameters() const
{
	return parameters;
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
	parameters.reset();
	frameListeners.clear();
}

SystemState::~SystemState()
{
}

void SystemState::destroy()
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
	/*
	   Stop the downloads so that the thread pool does not keep waiting for data.
	   Standalone downloader does not really need this as the downloading threads will
	   be stopped with the whole thread pool, but in plugin mode this is necessary.
	*/
	if(downloadManager)
		downloadManager->stopAll();
	//The thread pool should be stopped before everything
	if(threadPool)
		threadPool->forceStop();
	stopEngines();

	delete intervalManager;
	//Finalize ourselves
	finalize();

	//We are already being destroyed, make our classdef abandon us
	setClass(NULL);
	
	//Free the stage. This should free all objects on the displaylist
	stage->decRef();
	stage = NULL;

	/*
	 * 1) call finalize on all objects, this will free all referenced objects and thereby
	 * cut circular references. After that, all ASObjects but classes and templates should
	 * have been deleted through decRef. Else it is an error.
	 * 2) decRef all the classes and templates to which we hold a reference through the
	 * 'classes' and 'templates' maps.
	 */

	std::map<QName, Class_base*>::iterator it=classes.begin();
	for(;it!=classes.end();++it)
		it->second->finalize();

	//Here we clean the events queue
	if(currentVm)
		currentVm->finalize();

	//Free classes by decRef'ing them
	for(auto i = classes.begin(); i != classes.end(); ++i)
		i->second->decRef();

	//Free templates by decRef'ing them
	for(auto i = templates.begin(); i != templates.end(); ++i)
		i->second->decRef();

	//The Vm must be destroyed this late to clean all managed integers and numbers
	//This deletes the {int,uint,number}_managers; therefore no Number/.. object may be
	//decRef'ed after this line as it would cause a manager->put()
	delete currentVm;

	//Some objects needs to remove the jobs when destroyed so keep the timerThread until now
	delete timerThread;
	timerThread=NULL;

	delete renderThread;
	renderThread=NULL;
	delete inputThread;
	inputThread=NULL;
	delete engineData;
	sem_destroy(&terminated);
	this->decRef(); //free a reference we obtained by 'new SystemState'
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
	if(exitOnError)
		exit(1);
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
	if(standalone)
		gtk_main_quit();
}

void SystemState::wait()
{
	sem_wait(&terminated);
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

void SystemState::delayedCreation(SystemState* th)
{
	EngineData* d=th->engineData;
	//Create a plug in the XEmbed window
	GtkWidget* plug=gtk_plug_new(d->window);
	if(d->isSizable())
	{
		int32_t reqWidth=th->getFrameSize().Xmax/20;
		int32_t reqHeight=th->getFrameSize().Ymax/20;
		if(th->standalone)
			gtk_widget_set_size_request(plug, reqWidth, reqHeight);
		d->width=reqWidth;
		d->height=reqHeight;
	}
	d->container = plug;
	//Realize the widget now, as we need the X window
	gtk_widget_realize(plug);
	//Show it now
	gtk_widget_show(plug);
	gtk_widget_map(plug);
	if (th->standalone)
	{
		gtk_widget_set_can_focus(plug, true);
		gtk_widget_grab_focus(plug);
	}
	d->window=GDK_WINDOW_XID(gtk_widget_get_window(plug));
	XSync(d->display, False);
	//The lock is needed to avoid thread creation/destruction races
	Locker l(th->mutex);
	if(th->shutdown)
		return;
	th->renderThread->start(th->engineData);
	th->inputThread->start(th->engineData);
	//If the render rate is known start the render ticks
	if(th->renderRate)
		th->startRenderTicks();
}

void SystemState::delayedStopping(SystemState* th)
{
	sys=th;
	//This is called from the plugin, also kill the stream
	th->engineData->stopMainDownload();
	th->stopEngines();
	sys=NULL;
}

void SystemState::createEngines()
{
	Locker l(mutex);
	if(shutdown)
	{
		//A shutdown request has arrived before the creation of engines
		return;
	}
	//Check if we should fall back on gnash
	if(useGnashFallback && vmVersion!=AVM2)
	{
		if(dumpedSWFPath.empty()) //The path is not known yet
		{
			waitingForDump=true;
			l.unlock();
			fileDumpAvailable.wait();
			if(shutdown)
				return;
			l.lock();
		}
		LOG(LOG_INFO,_("Trying to invoke gnash!"));
		//Dump the cookies to a temporary file
		strcpy(cookiesFileName,"/tmp/lightsparkcookiesXXXXXX");
		int file=mkstemp(cookiesFileName);
		if(file!=-1)
		{
			std::string data("Set-Cookie: " + rawCookies);
			ssize_t res;
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
		if (pipe(gnashStdin) != 0)
			LOG(LOG_ERROR,_("Pipe creation failed"));

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
			snprintf(bufXid,32,"%lu",engineData->window);
			snprintf(bufWidth,32,"%u",engineData->width);
			snprintf(bufHeight,32,"%u",engineData->height);
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
			engineData->setupMainThreadCallback((ls_callback_t)delayedStopping, this);
			return;
		}
	}

	l.unlock();
	//The engines must be created in the context of the main thread
	engineData->setupMainThreadCallback((ls_callback_t)delayedCreation, this);

	renderThread->waitForInitialization();

	// If the SWF file is AVM1 and Gnash fallback isn't enabled, just shut down.
	if(vmVersion != AVM2)
	{
		LOG(LOG_INFO, "Unsupported flash file (AVM1), shutting down...");
		setShutdownFlag();
	}

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
		LOG(LOG_INFO,_("Creating VM"));
		currentVm=new ABCVm(this);
	}
	else
		vmVersion=AVM1;

	if(engineData)
		addJob(new EngineCreator);
}

void SystemState::setParamsAndEngine(EngineData* e, bool s)
{
	Locker l(mutex);
	engineData=e;
	standalone=s;
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
	if(!invalidateQueueHead)
	{
		invalidateQueueHead=invalidateQueueTail=d;
		//This is the first object added to the invalidation queue
		//TODO: support the case without the VM
		if(currentVm)
		{
			//Let's ask the VM to invalidate the queue ater all events already queued are run
			currentVm->addEvent(NullRef,_MR(new FlushInvalidationQueueEvent));
		}
	}
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

_NR<Stage> SystemState::getStage() const
{
	stage->incRef();
	return _MR(stage);
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

void ThreadProfile::plot(uint32_t maxTime, cairo_t *cr)
{
	if(data.size()<=1)
		return;

	Locker locker(mutex);
	RECT size=sys->getFrameSize();
	int width=size.Xmax/20;
	int height=size.Ymax/20;
	
	GLfloat *vertex_coords = new GLfloat[data.size()*2];
	GLfloat *color_coords = new GLfloat[data.size()*4];

	int32_t start=tickCount-len;
	if(int32_t(data[0].index-start)>0)
		start=data[0].index;
	
	for(unsigned int i=0;i<data.size();i++)
	{
		vertex_coords[i*2] = int32_t(data[i].index-start)*width/len;
		vertex_coords[i*2+1] = data[i].timing*height/maxTime;
		color_coords[i*4] = color.Red;
		color_coords[i*4+1] = color.Green;
		color_coords[i*4+2] = color.Blue;
		color_coords[i*4+3] = 1;
	}

	glVertexAttribPointer(VERTEX_ATTRIB, 2, GL_FLOAT, GL_FALSE, 0, vertex_coords);
	glVertexAttribPointer(COLOR_ATTRIB, 4, GL_FLOAT, GL_FALSE, 0, color_coords);
	glEnableVertexAttribArray(VERTEX_ATTRIB);
	glEnableVertexAttribArray(COLOR_ATTRIB);
	glDrawArrays(GL_LINE_STRIP, 0, data.size());
	glDisableVertexAttribArray(VERTEX_ATTRIB);
	glDisableVertexAttribArray(COLOR_ATTRIB);

	cairo_set_source_rgb(cr, float(color.Red)/255, float(color.Green)/255, float(color.Blue)/255);

	//Draw tags
	string* curTag=NULL;
	int curTagX=0;
	int curTagY=maxTime;
	int curTagLen=0;
	int curTagH=0;
	cairo_text_extents_t te;
	for(unsigned int i=0;i<data.size();i++)
	{
		int32_t relx=int32_t(data[i].index-start)*width/len;
		if(!data[i].tag.empty())
		{
			//New tag, flush the old one if present
			if(curTag)
				rt->renderText(cr, curTag->c_str(),curTagX,imax(curTagY-curTagH,0));
			//Measure tag
			cairo_text_extents (cr, data[i].tag.c_str(), &te);
			curTagLen=te.width;
			curTagH=te.height;
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
				rt->renderText(cr, curTag->c_str(), curTagX, imax(curTagY-curTagH,0));
				curTag=NULL;
			}
		}
	}
}

ParseThread::ParseThread(istream& in, Loader *_loader, tiny_string srcurl)
  : version(0),useAVM2(false),useNetwork(false),
    f(in),zlibFilter(NULL),backend(NULL),loader(_loader),
    parsedObject(NullRef),url(srcurl),fileType(FT_UNKNOWN)
{
	f.exceptions ( istream::eofbit | istream::failbit | istream::badbit );
}

ParseThread::ParseThread(std::istream& in, RootMovieClip *root)
  : version(0),useAVM2(false),useNetwork(false),
    f(in),zlibFilter(NULL),backend(NULL),loader(NULL),
    parsedObject(NullRef),url(),fileType(FT_UNKNOWN)
{
	f.exceptions ( istream::eofbit | istream::failbit | istream::badbit );
	setRootMovie(root);
}

ParseThread::~ParseThread()
{
	if(zlibFilter)
	{
		//Restore the istream
		f.rdbuf(backend);
		delete zlibFilter;
	}
	parsedObject.reset();
}

FILE_TYPE ParseThread::recognizeFile(uint8_t c1, uint8_t c2, uint8_t c3, uint8_t c4)
{
	if(c1=='F' && c2=='W' && c3=='S')
		return FT_SWF;
	else if(c1=='C' && c2=='W' && c3=='S')
		return FT_COMPRESSED_SWF;
	else if((c1&0x80) && c2=='P' && c3=='N' && c4=='G')
		return FT_PNG;
	else if(c1==0xff && c2==0xd8 && c3==0xff)
		return FT_JPEG;
	else if(c1=='G' && c2=='I' && c3=='F' && c4=='8')
		return FT_GIF;
	else
		return FT_UNKNOWN;
}

void ParseThread::parseSWFHeader(RootMovieClip *root, UI8 ver)
{
	UI32_SWF FileLength;
	RECT FrameSize;
	UI16_SWF FrameRate;
	UI16_SWF FrameCount;

	version=ver;
	root->version=version;
	f >> FileLength;
	//Enable decompression if needed
	if(fileType==FT_SWF)
		LOG(LOG_INFO, _("Uncompressed SWF file: Version ") << (int)version);
	else if(fileType==FT_COMPRESSED_SWF)
	{
		LOG(LOG_INFO, _("Compressed SWF file: Version ") << (int)version);
		//The file is compressed, create a filtering streambuf
		backend=f.rdbuf();
		zlibFilter = new zlib_filter(backend);
		f.rdbuf(zlibFilter);
	}

	f >> FrameSize >> FrameRate >> FrameCount;

	root->fileLength=FileLength;
	float frameRate=FrameRate;
	frameRate/=256;
	LOG(LOG_INFO,_("FrameRate ") << frameRate);
	root->setFrameRate(frameRate);
	//TODO: setting render rate should be done when the clip is added to the displaylist
	sys->setRenderRate(frameRate);
	root->setFrameSize(FrameSize);
	root->totalFrames_unreliable = FrameCount;
}

void ParseThread::execute()
{
	pt=this;
	try
	{
		UI8 Signature[4];
		f >> Signature[0] >> Signature[1] >> Signature[2] >> Signature[3];
		fileType=recognizeFile(Signature[0],Signature[1],Signature[2],Signature[3]);
		if(fileType==FT_UNKNOWN)
			throw ParseException("Not a supported file");
		else if(fileType==FT_PNG || fileType==FT_JPEG || fileType==FT_GIF)
		{
			f.putback(Signature[3]).putback(Signature[2]).
			  putback(Signature[1]).putback(Signature[0]);
			parseBitmap();
		}
		else
		{
			parseSWF(Signature[3]);
		}
	}
	catch(LightsparkException& e)
	{
		LOG(LOG_ERROR,_("Exception in ParseThread ") << e.cause);
		sys->setError(e.cause);
	}
	catch(std::exception& e)
	{
		LOG(LOG_ERROR,_("Stream exception in ParseThread ") << e.what());
	}
	pt=NULL;
}

void ParseThread::parseSWF(UI8 ver)
{
	RootMovieClip* root=getRootMovie();
	assert_and_throw(root);

	try
	{
		parseSWFHeader(root, ver);

		//Create a top level TagFactory
		TagFactory factory(f, true);
		bool done=false;
		bool empty=true;
		while(!done)
		{
			_NR<Tag> tag=factory.readTag();
			switch(tag->getType())
			{
				case END_TAG:
				{
					if(!empty)
						root->commitFrame(false);
					else
						root->revertFrame();
					RELEASE_WRITE(root->finishedLoading,true);
					done=true;
					root->check();
					break;
				}
				case DICT_TAG:
				{
					_R<DictionaryTag> d=tag.cast<DictionaryTag>();
					d->setLoadedFrom(root);
					root->addToDictionary(d);
					break;
				}
				case DISPLAY_LIST_TAG:
					root->addToFrame(tag.cast<DisplayListTag>());
					empty=false;
					break;
				case SHOW_TAG:
					root->commitFrame(true);
					empty=true;
					break;
				case CONTROL_TAG:
					/* The spec is not clear about that,
					 * but it seems that all the CONTROL_TAGs
					 * (=not one of the other tag types here)
					 * appear in the first frame only.
					 * We rely on that by not using
					 * locking in ctag->execute's implementation.
					 * ABC_TAG's are an exception, as they require no locking.
					 */
					assert(root->frames.size()==1);
					//fall through
				case ABC_TAG:
				{
					_R<ControlTag> ctag = tag.cast<ControlTag>();
					ctag->execute(root);
					break;
				}
				case FRAMELABEL_TAG:
					/* No locking required, as the last frames is not
					 * commited to the vm yet.
					 */
					root->addFrameLabel(root->frames.size()-1,static_cast<FrameLabelTag*>(tag.getPtr())->Name);
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
	catch(std::exception& e)
	{
		root->parsingFailed();
		throw;
	}
}

void ParseThread::parseBitmap()
{
	_NR<Bitmap> tmp=_MNR(Class<Bitmap>::getInstanceS(&f, fileType));

	{
		SpinlockLocker l(objectSpinlock);
		parsedObject=tmp;
	}
}

_NR<DisplayObject> ParseThread::getParsedObject()
{
	SpinlockLocker l(objectSpinlock);
	return parsedObject;
}

void ParseThread::setRootMovie(RootMovieClip *root)
{
	SpinlockLocker l(objectSpinlock);
	assert(root);
	root->incRef();
	parsedObject=_MNR(root);
}

RootMovieClip *ParseThread::getRootMovie()
{
	objectSpinlock.lock();
	RootMovieClip *root=Class<RootMovieClip>::dyncast(parsedObject.getPtr());
	objectSpinlock.unlock();
	if(root)
		return root;
	else if(fileType==FT_SWF || fileType==FT_COMPRESSED_SWF)
	{
		LoaderInfo *li=loader?loader->getContentLoaderInfo().getPtr():NULL;
		root=RootMovieClip::getInstance(li);
		objectSpinlock.lock();
		parsedObject=_MNR(root);
		objectSpinlock.unlock();
		if(!url.empty())
			root->setOrigin(url, "");
		return root;
	}
	else
		return NULL;
}

void ParseThread::threadAbort()
{
	//Tell the our RootMovieClip that the parsing is ending
	RootMovieClip *root=getRootMovie();
	if(root)
		root->parsingFailed();
}

bool RootMovieClip::boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const
{
	RECT f=getFrameSize();
	xmin=0;
	ymin=0;
	xmax=f.Xmax/20;
	ymax=f.Ymax/20;
	return true;
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

void RootMovieClip::commitFrame(bool another)
{
	setFramesLoaded(frames.size());

	if(another)
		frames.emplace_back();

	if(getFramesLoaded()==1 && frameRate!=0)
	{
		if(this==sys)
		{
			/* now the frameRate is available and all SymbolClass tags have created their classes */
			sys->addTick(1000/frameRate,sys);
		}
		else
		{
			this->incRef();
			sys->currentVm->addEvent(NullRef, _MR(new InitFrameEvent(_MNR(this))));
		}
	}
}

void RootMovieClip::revertFrame()
{
	//TODO: The next should be a regular assert
	assert_and_throw(frames.size() && getFramesLoaded()==(frames.size()-1));
	frames.pop_back();
}

RGB RootMovieClip::getBackground()
{
	return Background;
}

void RootMovieClip::setBackground(const RGB& bg)
{
	Background=bg;
}

/* called in parser's thread context */
void RootMovieClip::addToDictionary(_R<DictionaryTag> r)
{
	SpinlockLocker l(dictSpinlock);
	dictionary.push_back(r);
}

/* called in vm's thread context */
_R<DictionaryTag> RootMovieClip::dictionaryLookup(int id)
{
	SpinlockLocker l(dictSpinlock);
	auto it = dictionary.begin();
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
	return *it;
}

_NR<RootMovieClip> RootMovieClip::getRoot()
{
	this->incRef();
	return _MR(this);
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


void SystemState::tick()
{
	{
		SpinlockLocker l(profileDataSpinlock);
		list<ThreadProfile>::iterator it=profilingData.begin();
		for(;it!=profilingData.end();++it)
			it->tick();
	}
	if(sys->currentVm==NULL)
		return;
	/* See http://www.senocular.com/flash/tutorials/orderofoperations/
	 * for the description of steps.
	 */
	/* TODO: Step 1: declare new objects */

	/* Step 2: Send enterFrame events, if needed */
	{
		Locker l(mutexFrameListeners);
		if(!frameListeners.empty())
		{
			_R<Event> e(Class<Event>::getInstanceS("enterFrame"));
			auto it=frameListeners.begin();
			for(;it!=frameListeners.end();it++)
				getVm()->addEvent(*it,e);
		}
	}

	/* Step 3: create legacy objects, which are new in this frame (top-down),
	 * run their constructors (bottom-up)
	 * and their frameScripts (Step 5) (bottom-up) */
	sys->currentVm->addEvent(NullRef, _MR(new InitFrameEvent()));

	/* Step 4: dispatch frameConstructed events */
	/* (TODO: should be run between step 3 and 5 */
	{
		Locker l(mutexFrameListeners);
		if(!frameListeners.empty())
		{
			_R<Event> e(Class<Event>::getInstanceS("frameConstructed"));
			auto it=frameListeners.begin();
			for(;it!=frameListeners.end();it++)
				getVm()->addEvent(*it,e);
		}
	}
	/* Step 6: dispatch exitFrame event */
	{
		Locker l(mutexFrameListeners);
		if(!frameListeners.empty())
		{
			_R<Event> e(Class<Event>::getInstanceS("exitFrame"));
			auto it=frameListeners.begin();
			for(;it!=frameListeners.end();it++)
				getVm()->addEvent(*it,e);
		}
	}
	/* TODO: Step 7: dispatch render event (Assuming stage.invalidate() has been called) */

	/* Step 0: Set current frame number to the next frame */
	_R<AdvanceFrameEvent> advFrame = _MR(new AdvanceFrameEvent());
	if(sys->currentVm->addEvent(NullRef, advFrame))
		advFrame->done.wait();
}

void SystemState::resizeCompleted() const
{
	if(currentVm && scaleMode==NO_SCALE)
	{
		stage->incRef();
		currentVm->addEvent(_MR(stage),_MR(Class<Event>::getInstanceS("resize",false)));
	}
}

/* This is run in vm's thread context */
void RootMovieClip::initFrame()
{
	LOG(LOG_CALLS,"Root:initFrame " << getFramesLoaded() << " " << state.FP);
	/* We have to wait for at least one frame
	 * so our class get the right classdef. Else we will
	 * call the wrong constructor. */
	if(getFramesLoaded() == 0)
		return;

	MovieClip::initFrame();
}

/* This is run in vm's thread context */
void RootMovieClip::advanceFrame()
{
	/* We have to wait for at least one frame */
	if(getFramesLoaded() == 0)
		return;

	MovieClip::advanceFrame();
}

void RootMovieClip::constructionComplete()
{
	MovieClip::constructionComplete();
	if(this==sys)
		loaderInfo->sendInit();
}
