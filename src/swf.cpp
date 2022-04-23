/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2008-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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
#include <algorithm>
#include "backends/security.h"
#include "scripting/abc.h"
#include "scripting/flash/events/flashevents.h"
#include "scripting/flash/utils/flashutils.h"
#include "scripting/flash/utils/IntervalManager.h"
#include "scripting/flash/media/flashmedia.h"
#include "scripting/flash/filesystem/flashfilesystem.h"
#include "scripting/toplevel/ASString.h"
#include "scripting/toplevel/Number.h"
#include "scripting/toplevel/Boolean.h"
#include "scripting/toplevel/Vector.h"
#include "scripting/avm1/avm1display.h"
#include "logger.h"
#include "parsing/streams.h"
#include "asobject.h"
#include "scripting/class.h"
#include "backends/audio.h"
#include "backends/config.h"
#include "backends/rendering.h"
#include "backends/image.h"
#include "backends/extscriptobject.h"
#include "backends/input.h"
#include "backends/locale.h"
#include "backends/currency.h"
#include "memory_support.h"
#include "parsing/tags.h"

#ifdef ENABLE_CURL
#include <curl/curl.h>
#endif

#include "compat.h"

#ifdef ENABLE_LIBAVCODEC
extern "C" {
#include <libavcodec/avcodec.h>
}
#endif

using namespace std;
using namespace lightspark;

DEFINE_AND_INITIALIZE_TLS(tls_system);
SystemState* lightspark::getSys()
{
	SystemState* ret = (SystemState*)tls_get(tls_system);
	return ret;
}

void lightspark::setTLSSys(SystemState* sys)
{
        tls_set(tls_system,sys);
}

DEFINE_AND_INITIALIZE_TLS(parse_thread_tls);
ParseThread* lightspark::getParseThread()
{
	ParseThread* pt = (ParseThread*)tls_get(parse_thread_tls);
	assert(pt);
	return pt;
}

DEFINE_AND_INITIALIZE_TLS(tls_worker);
void lightspark::setTLSWorker(ASWorker* worker)
{
	tls_set(tls_worker,worker);
}
ASWorker* lightspark::getWorker()
{
	return (ASWorker*)tls_get(tls_worker);
}


RootMovieClip::RootMovieClip(ASWorker* wrk, _NR<LoaderInfo> li, _NR<ApplicationDomain> appDomain, _NR<SecurityDomain> secDomain, Class_base* c):
	MovieClip(wrk,c),
	parsingIsFailed(false),waitingforparser(false),Background(0xFF,0xFF,0xFF),frameRate(0),
	finishedLoading(false),applicationDomain(appDomain),securityDomain(secDomain)
{
	subtype=SUBTYPE_ROOTMOVIECLIP;
	loaderInfo=li;
	parsethread=nullptr;
	hasSymbolClass=false;
	hasMainClass=false;
	usesActionScript3=false;
}

RootMovieClip::~RootMovieClip()
{
	for(auto it=dictionary.begin();it!=dictionary.end();++it)
		delete it->second;
}

void RootMovieClip::destroyTags()
{
	for(auto it=frames.begin();it!=frames.end();++it)
		it->destroyTags();
}

void RootMovieClip::parsingFailed()
{
	//The parsing is failed, we have no change to be ever valid
	parsingIsFailed=true;
}

void RootMovieClip::setOrigin(const tiny_string& u, const tiny_string& filename)
{
	//We can use this origin to implement security measures.
	//Note that for plugins, this url is NOT the page url, but it is the swf file url.
	origin = URLInfo(u);
	//If this URL doesn't contain a filename, add the one passed as an argument (used in main.cpp)
	if(origin.getPathFile() == "" && filename != "")
		origin = origin.goToURL(filename);

	if(!loaderInfo.isNull())
	{
		loaderInfo->setURL(origin.getParsedURL(), false);
		loaderInfo->setLoaderURL(origin.getParsedURL());
	}
}

void RootMovieClip::setBaseURL(const tiny_string& url)
{
	//Set the URL to be used in resolving relative paths. For the
	//plugin this is either the value of base attribute in the
	//OBJECT or EMBED tag or, if the attribute is not provided,
	//the address of the hosting HTML page.
	baseURL = URLInfo(url);
}

const URLInfo& RootMovieClip::getBaseURL()
{
	//The plugin uses the address of the HTML page (baseURL) for
	//resolving relative paths. AIR and the standalone Lightspark
	//use the SWF location (origin).
	if(baseURL.isValid())
		return baseURL;
	else
		return origin;
}

void SystemState::registerFrameListener(DisplayObject* obj)
{
	Locker l(mutexFrameListeners);
	frameListeners.insert(obj);
}

void SystemState::unregisterFrameListener(DisplayObject* obj)
{
	Locker l(mutexFrameListeners);
	frameListeners.erase(obj);
}

void SystemState::addBroadcastEvent(const tiny_string& event)
{
	Locker l(mutexFrameListeners);
	if(!frameListeners.empty())
	{
		_R<Event> e(Class<Event>::getInstanceS(this->worker,event));
		auto it=frameListeners.begin();
		for(;it!=frameListeners.end();it++)
		{
			(*it)->incRef();
			getVm(this)->addEvent(_MR(*it),e);
		}
	}
}

RootMovieClip* RootMovieClip::getInstance(ASWorker* wrk,_NR<LoaderInfo> li, _R<ApplicationDomain> appDomain, _R<SecurityDomain> secDomain)
{
	Class_base* movieClipClass = Class<MovieClip>::getClass(getSys());
	RootMovieClip* ret=new (movieClipClass->memoryAccount) RootMovieClip(wrk,li, appDomain, secDomain, movieClipClass);
	ret->constructIndicator = true;
	ret->constructorCallComplete = true;
	ret->loadedFrom=ret;
	return ret;
}

void SystemState::staticInit()
{
	//Do needed global initialization
#ifdef ENABLE_CURL
	curl_global_init(CURL_GLOBAL_ALL);
#endif
#ifdef ENABLE_LIBAVCODEC
#if ( LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(58,10,100) )
	avcodec_register_all();
	av_register_all();
#endif
#endif

	// seed the random number generator
	char *envvar = getenv("LIGHTSPARK_RANDOM_SEED");
	if (envvar)
	{
		LOG(LOG_INFO,"using static random seed:"<<envvar);
		srand(atoi(envvar));
	}
	else
		srand(time(nullptr));
}

void SystemState::staticDeinit()
{
	delete Type::anyType;
	delete Type::voidType;
#ifdef ENABLE_CURL
	curl_global_cleanup();
#endif
}

//See BUILTIN_STRINGS enum
static const char* builtinStrings[] = {"any", "void", "prototype", "Function", "__AS3__.vec","Class", "http://adobe.com/AS3/2006/builtin","http://www.w3.org/XML/1998/namespace","xml","toString","valueOf","length","constructor",
									   "_target","this","_root","_parent","_global","super",
									   "onEnterFrame","onMouseMove","onMouseDown","onMouseUp","onPress","onRelease","onReleaseOutside","onMouseWheel","onLoad",
									   "object","undefined","boolean","number","string","function","onRollOver","onRollOut",
									   "__proto__","target","flash.events:IEventDispatcher","addEventListener","removeEventListener","dispatchEvent","hasEventListener",
									   "onConnect","onData","onClose","onSelect"
									  };

extern uint32_t asClassCount;

SystemState::SystemState(uint32_t fileSize, FLASH_MODE mode):
	terminated(0),renderRate(0),error(false),shutdown(false),firsttick(true),localstorageallowed(false),
	renderThread(nullptr),inputThread(nullptr),engineData(nullptr),dumpedSWFPathAvailable(0),
	vmVersion(VMNONE),childPid(0),
	parameters(NullRef),
	invalidateQueueHead(NullRef),invalidateQueueTail(NullRef),lastUsedStringId(0),lastUsedNamespaceId(0x7fffffff),
	showProfilingData(false),allowFullscreen(false),flashMode(mode),swffilesize(fileSize),avm1global(nullptr),
	currentVm(nullptr),builtinClasses(nullptr),useInterpreter(true),useFastInterpreter(false),useJit(false),ignoreUnhandledExceptions(false),exitOnError(ERROR_NONE),
	systemDomain(nullptr),worker(nullptr),workerDomain(nullptr),singleworker(true),
	downloadManager(nullptr),extScriptObject(nullptr),scaleMode(SHOW_ALL),unaccountedMemory(nullptr),tagsMemory(nullptr),stringMemory(nullptr),textTokenMemory(nullptr),shapeTokenMemory(nullptr),morphShapeTokenMemory(nullptr),bitmapTokenMemory(nullptr),spriteTokenMemory(nullptr),
	static_SoundMixer_bufferTime(0),static_Multitouch_inputMode("gesture"),isinitialized(false)
{
	//Forge the builtin strings
	uniqueStringIDMap.reserve(LAST_BUILTIN_STRING);
	tiny_string sempty;
	uniqueStringMap.emplace(make_pair(sempty,lastUsedStringId));
	uniqueStringIDMap.push_back(sempty);
	lastUsedStringId++;
	for(uint32_t i=1;i<BUILTIN_STRINGS_CHAR_MAX;i++)
	{
		tiny_string s = tiny_string::fromChar(i);
		uniqueStringMap.emplace(make_pair(s,lastUsedStringId));
		uniqueStringIDMap.push_back(s);
		lastUsedStringId++;
	}
	for(uint32_t i=BUILTIN_STRINGS_CHAR_MAX;i<LAST_BUILTIN_STRING;i++)
	{
		tiny_string s(builtinStrings[i-BUILTIN_STRINGS_CHAR_MAX]);
		uniqueStringMap.emplace(make_pair(s,lastUsedStringId));
		uniqueStringIDMap.push_back(s);
		lastUsedStringId++;
	}
	//Forge the empty namespace and make sure it gets id 0
	nsNameAndKindImpl emptyNs(BUILTIN_STRINGS::EMPTY, NAMESPACE);
	uint32_t nsId;
	uint32_t baseId;
	getUniqueNamespaceId(emptyNs, BUILTIN_NAMESPACES::EMPTY_NS, nsId, baseId);
	assert(nsId==0 && baseId==0);
	//Forge the AS3 namespace and make sure it gets id 1
	nsNameAndKindImpl as3Ns(BUILTIN_STRINGS::STRING_AS3NS, NAMESPACE);
	getUniqueNamespaceId(as3Ns, BUILTIN_NAMESPACES::AS3_NS, nsId, baseId);
	assert(nsId==1 && baseId==1);

	cookiesFileName = nullptr;

	setTLSSys(this);
	// it seems Adobe ignores any locale date settings
	setlocale(LC_TIME, "C");
	setlocale(LC_NUMERIC, "POSIX");

	unaccountedMemory = allocateMemoryAccount("Unaccounted");
	tagsMemory = allocateMemoryAccount("Tags");
	stringMemory = allocateMemoryAccount("Tiny_string");
	textTokenMemory = allocateMemoryAccount("Tokens.Text");
	shapeTokenMemory = allocateMemoryAccount("Tokens.Shape");
	morphShapeTokenMemory = allocateMemoryAccount("Tokens.MorphShape");
	bitmapTokenMemory = allocateMemoryAccount("Tokens.Bitmap");
	spriteTokenMemory = allocateMemoryAccount("Tokens.Sprite");

	null=new (unaccountedMemory) Null;
	null->setSystemState(this);
	null->setWorker(this->worker);
	null->setRefConstant();
	undefined=new (unaccountedMemory) Undefined;
	undefined->setSystemState(this);
	undefined->setWorker(this->worker);
	undefined->setRefConstant();

	builtinClasses = new Class_base*[asClassCount];
	memset(builtinClasses,0,asClassCount*sizeof(Class_base*));

	//Untangle the messy relationship between class objects and the Class class
	Class_object* classObject = Class_object::getClass(this);

	worker = new (unaccountedMemory) ASWorker(this);
	worker->setRefConstant();
	Class<ASWorker>::getClass(this)->setupDeclaredTraits(worker);
	worker->setClass(Class<ASWorker>::getClass(this));
	worker->constructionComplete();
	worker->setConstructIndicator();

	classObject->setWorker(worker);

	//Getting the Object class object will set the classdef to the Class_object
	//like any other class. This happens inside Class_base constructor
	_R<Class_base> asobjectClass = Class<ASObject>::getRef(this);

	//The only bit remaining is setting the Object class as the super class for Class
	classObject->setSuper(asobjectClass);
	classObject->decRef();
	objClassRef = asobjectClass.getPtr();

	trueRef=Class<Boolean>::getInstanceS(this->worker,true);
	trueRef->setRefConstant();
	falseRef=Class<Boolean>::getInstanceS(this->worker,false);
	falseRef->setRefConstant();
	
	nanAtom = asAtomHandler::fromNumber(this->worker,Number::NaN,true);

	systemDomain = Class<ApplicationDomain>::getInstanceS(this->worker);
	systemDomain->setRefConstant();
	_NR<ApplicationDomain> applicationDomain=_MR(Class<ApplicationDomain>::getInstanceS(this->worker,_MR(systemDomain)));
	_NR<SecurityDomain> securityDomain = _MR(Class<SecurityDomain>::getInstanceS(this->worker));

	static_SoundMixer_soundTransform  = _MR(Class<SoundTransform>::getInstanceS(this->worker));
	static_SoundMixer_soundTransform->setRefConstant();
	threadPool=new ThreadPool(this);
	downloadThreadPool=new ThreadPool(this);

	timerThread=new TimerThread(this);
	frameTimerThread=new TimerThread(this);
	audioManager=nullptr;
	intervalManager=new IntervalManager();
	securityManager=new SecurityManager();
	localeManager = new LocaleManager();
	currencyManager = new CurrencyManager();

	_NR<LoaderInfo> loaderInfo=_MR(Class<LoaderInfo>::getInstanceS(this->worker));
	loaderInfo->applicationDomain = applicationDomain;
	loaderInfo->setBytesLoaded(0);
	loaderInfo->setBytesTotal(0);
	mainClip=RootMovieClip::getInstance(this->worker,loaderInfo, applicationDomain, securityDomain);
	mainClip->setRefConstant();
	worker->rootClip = _MR(mainClip);
	workerDomain = Class<WorkerDomain>::getInstanceSNoArgs(this->worker);
	workerDomain->setRefConstant();
	addWorker(worker);
	stage=Class<Stage>::getInstanceS(this->worker);
	stage->setRefConstant();
	stage->setRoot(_MR(mainClip));
	//Get starting time
	startTime=compat_msectiming();
	
	renderThread=new RenderThread(this);
	inputThread=new InputThread(this);

	EngineData::userevent = SDL_RegisterEvents(3);
	if (EngineData::sdl_needinit)
	{
		SDL_Event event;
		SDL_zero(event);
		event.type = LS_USEREVENT_INIT;
		event.user.data1 = this;
		SDL_PushEvent(&event);
	}
}

void SystemState::setDownloadedPath(const tiny_string& p)
{
	dumpedSWFPath=p;
	dumpedSWFPathAvailable.signal();
}

void SystemState::setCookies(const char* c)
{
	rawCookies=c;
}

const std::string& SystemState::getCookies()
{
	return rawCookies;
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
	//Save a copy of the string
	rawParameters=v;

	_NR<ASObject> params=getParameters();
	if(params.isNull())
		params=_MNR(Class<ASObject>::getInstanceS(this->worker));
	//Add arguments to SystemState
	string vars(v);
	uint32_t cur=0;
	char* pfile = getenv("LIGHTSPARK_PLUGIN_PARAMFILE");
	ofstream f;
	if(pfile)
		f.open(pfile, ios::binary|ios::out);
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
			cout << varName << endl << varValue << endl;
			if(pfile)
				f << varName << endl << varValue << endl;

			/* That does occur in the wild */
			if(params->hasPropertyByMultiname(QName(getUniqueStringId(varName),BUILTIN_STRINGS::EMPTY), true, true,this->worker))
				LOG(LOG_ERROR,"Flash parameters has duplicate key '" << varName << "' - ignoring");
			else
				params->setVariableByQName(varName,"",
					lightspark::Class<lightspark::ASString>::getInstanceS(this->worker,varValue),DYNAMIC_TRAIT);
		}
		cur=n2+1;
	}
	setParameters(params);
}

void SystemState::parseParametersFromFile(const char* f)
{
	ifstream i(f, ios::in|ios::binary);
	if(!i)
	{
		LOG(LOG_ERROR,"Parameters file not found");
		return;
	}
	_R<ASObject> ret=_MR(Class<ASObject>::getInstanceS(this->worker));
	while(!i.eof())
	{
		string name,value;
		getline(i,name);
		getline(i,value);

		ret->setVariableByQName(name,"",abstract_s(this->worker,value),DYNAMIC_TRAIT);
		cout << name << ' ' << value << endl;
	}
	setParameters(ret);
	i.close();
}

void SystemState::parseParametersFromURL(const URLInfo& url)
{
	_NR<ASObject> params=getParameters();
	if(params.isNull())
		params=_MNR(Class<ASObject>::getInstanceS(this->worker));

	parseParametersFromURLIntoObject(url, params);
	setParameters(params);
}

void SystemState::parseParametersFromURLIntoObject(const URLInfo& url, _R<ASObject> outParams)
{
	std::list< std::pair<tiny_string, tiny_string> > queries=url.getQueryKeyValue();
	std::list< std::pair<tiny_string, tiny_string> >::iterator it;
	for (it=queries.begin(); it!=queries.end(); ++it)
	{
		if(outParams->hasPropertyByMultiname(QName(outParams->getSystemState()->getUniqueStringId(it->first),BUILTIN_STRINGS::EMPTY), true, true,outParams->getInstanceWorker()))
			LOG(LOG_ERROR,"URL query parameters has duplicate key '" << it->first << "' - ignoring");
		else
			outParams->setVariableByQName(it->first,"",
			   lightspark::Class<lightspark::ASString>::getInstanceS(outParams->getInstanceWorker(),it->second),DYNAMIC_TRAIT);
	}
}

void SystemState::setParameters(_R<ASObject> p)
{
	parameters=p;
	mainClip->loaderInfo->parameters = p;
}

_NR<ASObject> SystemState::getParameters() const
{
	return parameters;
}

void SystemState::stopEngines()
{
	if(downloadThreadPool)
		downloadThreadPool->forceStop();
	if(threadPool)
		threadPool->forceStop();
	timerThread->wait();
	frameTimerThread->wait();
	/* first shutdown the vm, because it can use all the others */
	if(currentVm)
		currentVm->shutdown();
	delete downloadManager;
	downloadManager=nullptr;
	delete securityManager;
	securityManager=nullptr;
	delete localeManager;
	localeManager=nullptr;
    delete currencyManager;
    currencyManager=NULL;
	delete threadPool;
	threadPool=nullptr;
	delete downloadThreadPool;
	downloadThreadPool=nullptr;
	//Now stop the managers
	delete audioManager;
	audioManager=nullptr;
}

#ifdef PROFILING_SUPPORT
void SystemState::saveProfilingInformation()
{
	if(profOut.numBytes())
	{
		ofstream f(profOut.raw_buf());
		f << "events: Time" << endl;
		for(uint32_t i=0;i<contextes.size();i++)
			contextes[i]->dumpProfilingData(f);
		f.close();
	}
}
#endif

MemoryAccount* SystemState::allocateMemoryAccount(const tiny_string& name)
{
#ifdef MEMORY_USAGE_PROFILING
	Locker l(memoryAccountsMutex);
	memoryAccounts.emplace_back(name);
	return &memoryAccounts.back();
#else
	return nullptr;
#endif
}

#ifdef MEMORY_USAGE_PROFILING
void SystemState::saveMemoryUsageInformation(ofstream& out, int snapshotCount) const
{
	out << "#-----------\nsnapshot=" << snapshotCount << "\n#-----------\ntime=" << snapshotCount << endl;
	uint32_t totalMem=0;
	uint32_t totalCount=0;
	Locker l(memoryAccountsMutex);
	auto it=memoryAccounts.begin();
	for(;it!=memoryAccounts.end();++it)
	{
		if(it->bytes>0)
		{
			totalMem+=it->bytes;
			totalCount++;
		}
	}

	out << "mem_heap_B=" << totalMem << "\nmem_heap_extra_B=0\nmem_stacks_B=0\nheap_tree=detailed" << endl;
	out << "n" << totalCount << ": " << totalMem << " ActionScript_objects" << endl;
	it=memoryAccounts.begin();
	for(;it!=memoryAccounts.end();++it)
	{
		if(it->bytes>0)
			out << " n0: " << it->bytes << " " << it->name << endl;
	}
}
#endif

void SystemState::systemFinalize()
{
	invalidateQueueHead.reset();
	invalidateQueueTail.reset();
	parameters.reset();
	static_SoundMixer_soundTransform.reset();
	frameListeners.clear();
	auto it = sharedobjectmap.begin();
	while (it != sharedobjectmap.end())
	{
		it->second->doFlush(worker);
		it = sharedobjectmap.erase(it);
	}
	mainClip->destroyTags();
}

SystemState::~SystemState()
{
	// 1) remove all references to variables as they might point to other constant reffed objects
	for (auto it = constantrefs.begin(); it != constantrefs.end(); it++)
	{
		(*it)->destroyContents();
		(*it)->finalize();
	}
	// 2) delete builtin classes
	delete[] builtinClasses;
	// 3) delete the constant reffed objects
	for (auto it = constantrefs.begin(); it != constantrefs.end(); it++)
	{
		delete (*it);
	}
}

void SystemState::destroy()
{
#ifdef PROFILING_SUPPORT
	saveProfilingInformation();
#endif
	terminated.wait();
	//Acquire the mutex to sure that the engines are not being started right now
	Locker l(rootMutex);
	renderThread->wait();
	inputThread->wait();
	if(currentVm)
	{
		//If the VM exists it MUST be started to flush pending events.
		//In some cases it will not be started by regular means, if so
		//we will start it here.
		if(!currentVm->hasEverStarted())
			currentVm->start();
		currentVm->shutdown();
	}

	l.release();

	//Kill our child process if any
	if(childPid)
	{
		LOG(LOG_INFO,"Terminating gnash...");
		kill_child(childPid);
	}
	//Delete the temporary cookies file
	if(cookiesFileName)
	{
		unlink(cookiesFileName);
		g_free(cookiesFileName);
	}
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
	if(downloadThreadPool)
		downloadThreadPool->forceStop();
	if(threadPool)
		threadPool->forceStop();
	stopEngines();

	delete extScriptObject;
	delete intervalManager;
	//Finalize ourselves
	systemFinalize();

	/*
	 * 1) call finalize on all objects, this will free all non constant referenced objects and thereby
	 * cut circular references. After that, all ASObjects but classes and templates should
	 * have been deleted through decRef. Else it is an error.
	 * 2) decRef all the classes and templates to which we hold a reference through the
	 * 'classes' and 'templates' maps.
	 */

	setTLSWorker(this->worker);
	for(uint32_t i=0;i<asClassCount;i++)
	{
		if(builtinClasses[i])
			builtinClasses[i]->finalize();
	}
	for(auto it = mainClip->customClasses.begin(); it != mainClip->customClasses.end(); ++it)
		it->second->finalize();
	for(auto it = mainClip->templates.begin(); it != mainClip->templates.end(); ++it)
		it->second->finalize();

	//Here we clean the events queue
	if(currentVm)
		currentVm->finalize();

	//Free classes by decRef'ing them
	for(uint32_t i=0;i<asClassCount;i++)
	{
		if(builtinClasses[i])
			builtinClasses[i]->decRef();
	}
	for(auto i = mainClip->customClasses.begin(); i != mainClip->customClasses.end(); ++i)
		i->second->decRef();

	//Free templates by decRef'ing them
	for(auto i = mainClip->templates.begin(); i != mainClip->templates.end(); ++i)
		i->second->decRef();

	//The Vm must be destroyed this late to clean all managed integers and numbers
	//This deletes the {int,uint,number}_managers; therefore no Number/.. object may be
	//decRef'ed after this line as it would cause a manager->put()
	delete currentVm;
	currentVm = nullptr;

	//Some objects needs to remove the jobs when destroyed so keep the timerThread until now
	delete timerThread;
	timerThread=nullptr;
	delete frameTimerThread;
	frameTimerThread= nullptr;
	
	delete renderThread;
	renderThread=nullptr;
	delete inputThread;
	inputThread=nullptr;
	delete engineData;
	engineData=nullptr;

	for(auto it=profilingData.begin();it!=profilingData.end();it++)
		delete *it;
	uniqueStringMap.clear();
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

void SystemState::setError(const string& c, ERROR_TYPE type)
{
	if((exitOnError & type) != 0)
	{
		error=true;
		setShutdownFlag();
		return;
	}

	//We record only the first error for easier fix and reporting
	if(!error)
	{
		error=true;
		errorCause=c;
		timerThread->stop();
		frameTimerThread->stop();
		//Disable timed rendering
		removeJob(renderThread);
		renderThread->draw(true);
	}
}

void SystemState::setShutdownFlag()
{
	Locker l(rootMutex);
	if(currentVm)
	{
		_R<ShutdownEvent> e(new (unaccountedMemory) ShutdownEvent);
		currentVm->addEvent(NullRef,e);
	}
	shutdown=true;

	terminated.signal();
}

void SystemState::setLocalStorageAllowed(bool allowed)
{
	localstorageallowed = allowed;
	getEngineData()->setLocalStorageAllowedMarker(allowed);
}

float SystemState::getRenderRate()
{
	return renderRate;
}

void SystemState::addWorker(ASWorker *w)
{
	Locker l(workerMutex);
	asAtom a = asAtomHandler::fromObject(w);
	w->incRef();
	workerDomain->workerlist->append(a);
	singleworker=workerDomain->workerlist->size() <= 1;
}

void SystemState::removeWorker(ASWorker *w)
{
	Locker l(workerMutex);
	workerDomain->workerlist->remove(w);
	singleworker=workerDomain->workerlist->size() <= 1;

}
void SystemState::addEventToBackgroundWorkers(_NR<EventDispatcher> obj, _R<Event> ev)
{
	if(obj.isNull())
		return;
	
	if (ev->is<WaitableEvent>())
		return;
	Locker l(workerMutex);
	for (uint32_t i= 0; i < workerDomain->workerlist->size(); i++)
	{
		asAtom w = workerDomain->workerlist->at(i);
		if (asAtomHandler::is<ASWorker>(w) && !asAtomHandler::as<ASWorker>(w)->isPrimordial && obj->hasEventListener(ev->type))
		{
			asAtomHandler::as<ASWorker>(w)->addEvent(obj,ev);
		}
	}
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
	getSys()->createEngines();
}

void SystemState::EngineCreator::threadAbort()
{
	getSys()->dumpedSWFPathAvailable.signal();
	getSys()->getRenderThread()->forceInitialization();
}

/*
 * This is run from mainLoopThread
 */
void SystemState::delayedCreation(SystemState* sys)
{
	sys->audioManager=new AudioManager(sys->engineData);
	sys->localstorageallowed =sys->getEngineData()->getLocalStorageAllowedMarker();
	int32_t reqWidth=(sys->mainClip->getFrameSize().Xmax-sys->mainClip->getFrameSize().Xmin)/20;
	int32_t reqHeight=(sys->mainClip->getFrameSize().Ymax-sys->mainClip->getFrameSize().Ymin)/20;

	if (EngineData::enablerendering)
		sys->engineData->showWindow(reqWidth, reqHeight);

	sys->inputThread->start(sys->engineData);

	if(EngineData::enablerendering && Config::getConfig()->isRenderingEnabled())
	{
		if (sys->engineData->needrenderthread)
			sys->renderThread->start(sys->engineData);
	}
	else
	{
		sys->getRenderThread()->windowWidth = reqWidth;
		sys->getRenderThread()->windowHeight = reqHeight;
		sys->resizeCompleted();
		//This just signals the 'initalized' semaphore
		sys->renderThread->forceInitialization();
		LOG(LOG_INFO,"Rendering is disabled by configuration");
	}

	if(sys->renderRate)
		sys->startRenderTicks();
	
	{
		Locker l(sys->initializedMutex);
		sys->isinitialized=true;
		sys->initializedCond.broadcast();
	}
}

void SystemState::delayedStopping()
{
	setTLSSys(this);
	//This is called from the plugin, also kill the stream
	engineData->stopMainDownload();
	stopEngines();
	setTLSSys(nullptr);
}

void SystemState::createEngines()
{
	Locker l(rootMutex);
	if(shutdown)
	{
		//A shutdown request has arrived before the creation of engines
		return;
	}
	//Check if we should fall back on gnash
	if(vmVersion==AVM1)
	{
		l.release();
		launchGnash();

		//Engines should not be started, stop everything
		//We cannot stop the engines now, as this is inside a ThreadPool job
		//Running this in the Gtk thread is unnecessary, though. Any other thread
		//would be okey.
		//TODO: delayedStopping may be scheduled after SystemState::destroy has finished
		//      and this SystemState object has been deleted.
		//      We cannot wait for that function to finish, because we run in a ThreadPool
		//      and the function will wait for all ThreadPool jobs to finish.
		//engineData->runInMainThread(sigc::mem_fun(this, &SystemState::delayedStopping));
		return;
	}

	//The engines must be created in the context of the main thread
	engineData->runInMainThread(this,&SystemState::delayedCreation);

	//Wait for delayedCreation to finish so it is protected by our 'mutex'
	//Otherwise SystemState::destroy may delete this object before delayedCreation is scheduled.
	renderThread->waitForInitialization();

	// If the SWF file is AVM1 and Gnash fallback isn't enabled, just shut down.

	//As we lost the lock the shutdown procesure might have started
	if(shutdown)
		return;
	if(currentVm)
		currentVm->start();
}

void SystemState::launchGnash()
{
	Locker l(rootMutex);
	if(Config::getConfig()->getGnashPath().empty())
	{
		LOG(LOG_INFO, "Unsupported flash file (AVM1), and no gnash found");
		l.release();
		setShutdownFlag();
		l.acquire();
		return;
	}

	/* wait for dumpedSWFPath */
	l.release();
	dumpedSWFPathAvailable.wait();
	l.acquire();

	if(dumpedSWFPath.empty())
		return;

	LOG(LOG_INFO,"Trying to invoke gnash!");
	//Dump the cookies to a temporary file
	int file=g_file_open_tmp("lightsparkcookiesXXXXXX",&cookiesFileName,NULL);
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
				LOG(LOG_ERROR, "Error during writing of cookie file for Gnash");
				break;
			}
			written += res;
		} while(written < data.size());
		close(file);
		g_setenv("GNASH_COOKIES_IN", cookiesFileName, 1);
	}
	else
	{
		LOG(LOG_ERROR,"Failed to create temporary coockie for gnash");
	}


	//Allocate some buffers to store gnash arguments
	char bufXid[32];
	char bufWidth[32];
	char bufHeight[32];
	snprintf(bufXid,32,"%lu",(long unsigned)engineData->getWindowForGnash());
	/* Use swf dimensions in standalone mode and window dimensions in plugin mode */
	snprintf(bufWidth,32,"%u",standalone ? mainClip->getFrameSize().Xmax/20 : engineData->width);
	snprintf(bufHeight,32,"%u",standalone ? mainClip->getFrameSize().Ymax/20 : engineData->height);
	/* renderMode: 0: disable sound and rendering
	 *             1: enable rendering and disable sound
	 *             2: enable sound and disable rendering
	 *             3: enable sound and rendering
	 */
	const char* renderMode = "3";
	if(!Config::getConfig()->isRenderingEnabled())
		renderMode = "2";

	string params("FlashVars=");
	params+=rawParameters;
	/* TODO: pass -F hostFD to assist in loading urls */
	char* args[] =
	{
		strdup(Config::getConfig()->getGnashPath().c_str()),
		strdup("-x"), //Xid
		bufXid,
		strdup("-j"), //Width
		bufWidth,
		strdup("-k"), //Height
		bufHeight,
		strdup("-u"), //SWF url
		strdup(mainClip->getOrigin().getParsedURL().raw_buf()),
		strdup("-P"), //SWF parameters
		strdup(params.c_str()),
		strdup("--render-mode"),
		strdup(renderMode),
		strdup("-vv"),
		strdup("-"),
		NULL
	};

	// Print out an informative message about how we are invoking Gnash
	int i = 1;
	std::string argsStr = args[0];
	while(args[i] != NULL)
	{
		argsStr += " ";
		argsStr += args[i];
		i++;
	}
	LOG(LOG_INFO, "Invoking '" << argsStr << " < " << dumpedSWFPath.raw_buf() << "'");

	int gnash_stdin;

	/* Unfortunately, g_spawn_async_with_pipes does not work under win32. First, it needs
	 * an additional helper 'gspawn-helper-console.exe' and second, it crashes with a buffer overflow
	 * when the plugin is run in ipc mode.
	 */
#if _WIN32
	//TODO: escape argumetns, and spaces in filename
	childPid = compat_spawn(args, &gnash_stdin);
	if(!childPid)
	{
		LOG(LOG_ERROR,"Spawning gnash failed!");
		return;
	}
#else
	GError* errmsg = NULL;
	if(!g_spawn_async_with_pipes(NULL, args, NULL, (GSpawnFlags)0, NULL, NULL, &childPid,
			&gnash_stdin, NULL, NULL, &errmsg))
	{
		LOG(LOG_ERROR,"Spawning gnash failed: " << errmsg->message);
		return;
	}
#endif

	// Open the SWF file
	std::ifstream swfStream(dumpedSWFPath.raw_buf(), ios::in|ios::binary);
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
			ret = write(gnash_stdin, data+written, swfStream.gcount()-written);
			if(ret < 0)
			{
				LOG(LOG_ERROR, "Error during writing of SWF file to Gnash");
				stop = true;
				break;
			}
			written += ret;
		} while(written < swfStream.gcount());
	}
	// Close the write end of Gnash's stdin, signalling EOF to Gnash.
	close(gnash_stdin);
	// Close the SWF file
	swfStream.close();
}


void SystemState::needsAVM2(bool avm2)
{
	Locker l(rootMutex);

	/* Check if we already loaded another swf. If not, then
	 * vmVersion is VMNONE.
	 */
	if((vmVersion == AVM1 && avm2)
	|| (vmVersion == AVM2 && !avm2))
	{
		LOG(LOG_NOT_IMPLEMENTED,"Cannot embed AVM1 media into AVM2 media and vice versa!");
		return;
	}

	//Create the virtual machine if needed
	if(avm2)
	{
		//needsAVM2 is only called for the SystemState movie
		assert(!currentVm);
		vmVersion=AVM2;
		LOG(LOG_INFO,"Creating VM");
		MemoryAccount* vmDataMemory=this->allocateMemoryAccount("VM_Data");
		currentVm=new ABCVm(this, vmDataMemory);
	}
	else
		vmVersion=AVM1;

	if(engineData)
		addJob(new EngineCreator);
}

void SystemState::setParamsAndEngine(EngineData* e, bool s)
{
	Locker l(rootMutex);
	engineData=e;
	standalone=s;
	if (flashMode == SystemState::AIR)
	{
		static_ASFile_applicationDirectory=_MNR(Class<ASFile>::getInstanceS(this->worker,getEngineData()->FileFullPath(this,""),true));
	}
	
	if(vmVersion)
		addJob(new EngineCreator);
}

void SystemState::setRenderRate(float rate)
{
	Locker l(rootMutex);
	if(renderRate==rate)
		return;
	
	//The requested rate is different than the current rate, let's reschedule the job
	renderRate=rate;
	startRenderTicks();

	if (this->mainClip && this->mainClip->isConstructed())
	{
		removeJob(this);
		addTick(1000/renderRate,this);
	}
}

void SystemState::addJob(IThreadJob* j)
{
	threadPool->addJob(j);
}
void SystemState::addDownloadJob(IThreadJob* j)
{
	downloadThreadPool->addJob(j);
}

void SystemState::addTick(uint32_t tickTime, ITickJob* job)
{
	timerThread->addTick(tickTime,job);
}

void SystemState::addFrameTick(uint32_t tickTime, ITickJob* job)
{
	frameTimerThread->addTick(tickTime,job);
}

void SystemState::addWait(uint32_t waitTime, ITickJob* job)
{
	timerThread->addWait(waitTime,job);
}

void SystemState::removeJob(ITickJob* job)
{
	if (job == this)
		timerThread->removeJob_noLock(job);
	else
		timerThread->removeJob(job);
}

ThreadProfile* SystemState::allocateProfiler(const lightspark::RGB& color)
{
	Locker l(profileDataSpinlock);
	profilingData.push_back(new ThreadProfile(color,100,engineData));
	ThreadProfile* ret=profilingData.back();
	return ret;
}

void SystemState::addToInvalidateQueue(_R<DisplayObject> d)
{
	Locker l(invalidateQueueLock);
	//Check if the object is already in the queue
	if(!d->invalidateQueueNext.isNull() || d==invalidateQueueTail)
		return;
	if(!invalidateQueueHead)
		invalidateQueueHead=invalidateQueueTail=d;
	else
	{
		d->invalidateQueueNext=invalidateQueueHead;
		invalidateQueueHead=d;
	}
}

void SystemState::flushInvalidationQueue()
{
	if (isShuttingDown())
		return;
	Locker l(invalidateQueueLock);
	_NR<DisplayObject> cur=invalidateQueueHead;
	while(!cur.isNull())
	{
		if(cur->isOnStage() && cur->hasChanged)
		{
			_NR<DisplayObject> drawobj=cur;
			_NR<DisplayObject> cachedBitmap;
			float scalex, scaley;
			int offx, offy;
			stageCoordinateMapping(renderThread->windowWidth, renderThread->windowHeight, offx, offy, scalex, scaley);
			IDrawable* d=cur->invalidate(stage, MATRIX(scalex, scaley), true, nullptr, &cachedBitmap);
			//Check if the drawable is valid and forge a new job to
			//render it and upload it to GPU
			if(d)
			{
				if (cachedBitmap)
					drawobj = cachedBitmap;
				if (drawobj->getNeedsTextureRecalculation() || !d->isCachedSurfaceUsable(drawobj.getPtr()))
				{
					drawjobLock.lock();
					AsyncDrawJob* j = new AsyncDrawJob(d,drawobj);
					if (!drawobj->getTextureRecalculationSkippable())
					{
						for (auto it = drawJobsPending.begin(); it != drawJobsPending.end(); it++)
						{
							if ((*it)->getOwner() == drawobj.getPtr())
							{
								// older drawjob currently running for this DisplayObject, abort it
								(*it)->threadAborting=true;
								drawJobsPending.erase(it);
								break;
							}
						}
						for (auto it = drawJobsNew.begin(); it != drawJobsNew.end(); it++)
						{
							if ((*it)->getOwner() == drawobj.getPtr())
							{
								// older drawjob currently running for this DisplayObject, abort it
								(*it)->threadAborting=true;
								drawJobsNew.erase(it);
								break;
							}
						}
						drawJobsNew.insert(j);
					}
					addJob(j);
					drawjobLock.unlock();
				}
				else
					renderThread->addRefreshableSurface(d,drawobj);
			}
			drawobj->hasChanged=false;
			if (getRenderThread()->isStarted())
				drawobj->resetNeedsTextureRecalculation();
		}
		_NR<DisplayObject> next=cur->invalidateQueueNext;
		cur->invalidateQueueNext=NullRef;
		cur=next;
	}
	renderThread->signalSurfaceRefresh();
	invalidateQueueHead=NullRef;
	invalidateQueueTail=NullRef;
}
void SystemState::AsyncDrawJobCompleted(AsyncDrawJob *j)
{
	drawjobLock.lock();
	drawJobsNew.erase(j);
	drawJobsPending.erase(j);
	if (getRenderThread())
		getRenderThread()->canrender = drawJobsPending.empty();
	drawjobLock.unlock();
}
void SystemState::swapAsyncDrawJobQueue()
{
	drawjobLock.lock();
	drawJobsPending.insert(drawJobsNew.begin(),drawJobsNew.end());
	drawJobsNew.clear();
	if (getRenderThread())
		getRenderThread()->canrender = drawJobsPending.empty();
	drawjobLock.unlock();
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
	RECT size=getSys()->mainClip->getFrameSize();
	int width=(size.Xmax-size.Xmin)/20;
	int height=(size.Ymax-size.Ymin)/20;
	
	float *vertex_coords = new float[data.size()*2];
	float *color_coords = new float[data.size()*4];

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
	if (!engineData)
		engineData = getSys()->getEngineData();
	if (!engineData)
		return;
	engineData->exec_glVertexAttribPointer(VERTEX_ATTRIB, 0, vertex_coords,FLOAT_2);
	engineData->exec_glVertexAttribPointer(COLOR_ATTRIB, 0, color_coords,FLOAT_4);
	engineData->exec_glEnableVertexAttribArray(VERTEX_ATTRIB);
	engineData->exec_glEnableVertexAttribArray(COLOR_ATTRIB);
	engineData->exec_glDrawArrays_GL_LINE_STRIP(0, data.size());
	engineData->exec_glDisableVertexAttribArray(VERTEX_ATTRIB);
	engineData->exec_glDisableVertexAttribArray(COLOR_ATTRIB);

	cairo_set_source_rgb(cr, float(color.Red)/255.0, float(color.Green)/255.0, float(color.Blue)/255.0);

	//Draw tags
	string* curTag=nullptr;
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
				getRenderThread()->renderText(cr, curTag->c_str(),curTagX,imax(curTagY-curTagH,0));
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
				getRenderThread()->renderText(cr, curTag->c_str(), curTagX, imax(curTagY-curTagH,0));
				curTag=nullptr;
			}
		}
	}
}

ParseThread::ParseThread(istream& in, _R<ApplicationDomain> appDomain, _R<SecurityDomain> secDomain, Loader *_loader, tiny_string srcurl)
  : version(0),applicationDomain(appDomain),securityDomain(secDomain),
    f(in),uncompressingFilter(nullptr),backend(nullptr),bytearraybuf(nullptr),loader(_loader),
    parsedObject(NullRef),url(srcurl),fileType(FT_UNKNOWN)
{
	f.exceptions ( istream::eofbit | istream::failbit | istream::badbit );
}

ParseThread::ParseThread(std::istream& in, RootMovieClip *root)
  : version(0),applicationDomain(NullRef),securityDomain(NullRef), //The domains are not needed since the system state create them itself
    f(in),uncompressingFilter(nullptr),backend(nullptr),bytearraybuf(nullptr),loader(nullptr),
    parsedObject(NullRef),url(),fileType(FT_UNKNOWN)
{
	f.exceptions ( istream::eofbit | istream::failbit | istream::badbit );
	setRootMovie(root);
	if (root)
	{
		root->parsethread=this;
		bytearraybuf= f.rdbuf();
	}
}

ParseThread::~ParseThread()
{
	if(uncompressingFilter)
	{
		//Restore the istream
		f.rdbuf(backend);
		delete uncompressingFilter;
	}
	parsedObject.reset();
}

FILE_TYPE ParseThread::recognizeFile(uint8_t c1, uint8_t c2, uint8_t c3, uint8_t c4)
{
	if(c1=='F' && c2=='W' && c3=='S')
		return FT_SWF;
	else if(c1=='C' && c2=='W' && c3=='S')
		return FT_COMPRESSED_SWF;
	else if(c1=='Z' && c2=='W' && c3=='S')
		return FT_LZMA_COMPRESSED_SWF;
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
	{
		LOG(LOG_INFO, "Uncompressed SWF file: Version " << (int)version);
		root->loaderInfo->setBytesTotal(FileLength);
	}
	else
	{
		//The file is compressed, create a filtering streambuf
		backend=f.rdbuf();
		if(fileType==FT_COMPRESSED_SWF)
		{
			LOG(LOG_INFO, "zlib compressed SWF file: Version " << (int)version);
			uncompressingFilter = new zlib_filter(backend);
		}
		else if(fileType==FT_LZMA_COMPRESSED_SWF)
		{
			LOG(LOG_INFO, "lzma compressed SWF file: Version " << (int)version);
			uncompressingFilter = new liblzma_filter(backend);
		}
		else
		{
			// not reached
			assert(false);
		}
		f.rdbuf(uncompressingFilter);
		// the first 8 bytes from the header are always uncompressed (magic bytes + FileLength)
		root->loaderInfo->setBytesTotal(FileLength-8);
	}

	f >> FrameSize >> FrameRate >> FrameCount;

	root->fileLength=FileLength;
	root->loaderInfo->setBytesLoaded(0);
	float frameRate=FrameRate;
	if (frameRate == 0)
		//The Adobe player ignores frameRate == 0 and substitutes
		//some larger value. Value 30 here is arbitrary.
		frameRate = 30;
	else
		frameRate/=256;
	LOG(LOG_INFO,"FrameRate " << frameRate);
	root->setFrameRate(frameRate);
	root->loaderInfo->setFrameRate(frameRate);
	root->setFrameSize(FrameSize);
	root->totalFrames_unreliable = FrameCount;
}

void ParseThread::execute()
{
	tls_set(parse_thread_tls,this);

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
	catch(ParseException& e)
	{
		Locker l(objectSpinlock);
		parsedObject = NullRef;
		// Set system error only for main SWF. Loader classes
		// handle error for loaded SWFs.
		if(!loader)
		{
			LOG(LOG_ERROR,"Exception in ParseThread " << e.cause);
			getSys()->setError(e.cause, SystemState::ERROR_PARSING);
		}
	}
	catch(LightsparkException& e)
	{
		LOG(LOG_ERROR,"Exception in ParseThread " << e.cause);
		getSys()->setError(e.cause, SystemState::ERROR_PARSING);
	}
	catch(std::exception& e)
	{
		LOG(LOG_ERROR,"Stream exception in ParseThread " << e.what());
	}
}

void ParseThread::parseSWF(UI8 ver)
{
	if (loader && !loader->allowLoadingSWF())
	{
		_NR<LoaderInfo> li=loader->getContentLoaderInfo();
		li->incRef();
		getVm(loader->getSystemState())->addEvent(li,_MR(Class<SecurityErrorEvent>::getInstanceS(loader->getInstanceWorker(),
			"Cannot import a SWF file when LoaderContext.allowCodeImport is false."))); // 3226
		return;
	}

	objectSpinlock.lock();
	RootMovieClip* root=nullptr;
	if(parsedObject.isNull())
	{
		_NR<LoaderInfo> li=loader->getContentLoaderInfo();
		root=RootMovieClip::getInstance(applicationDomain->getInstanceWorker(),li, applicationDomain, securityDomain);
		if (!applicationDomain->getInstanceWorker()->isPrimordial)
		{
			root->incRef();
			applicationDomain->getInstanceWorker()->rootClip = _MR(root);
		}
		parsedObject=_MNR(root);
		li->setWaitedObject(parsedObject);
		if(!url.empty())
			root->setOrigin(url, "");
	}
	else
	{
		root=getRootMovie();
		parsedObject->loaderInfo->setWaitedObject(parsedObject);
	}
	objectSpinlock.unlock();

	TAGTYPE lasttagtype = TAG;
	std::queue<const ControlTag*> queuedTags;
	try
	{
		parseSWFHeader(root, ver);
		if (loader)
		{
			_NR<LoaderInfo> li=loader->getContentLoaderInfo();
			li->swfVersion = root->version;
		}
		
		int usegnash = 0;
		char *envvar = getenv("LIGHTSPARK_USE_GNASH");
		if (envvar)
			usegnash= atoi(envvar);
		if(root->version < 9)
		{
			if (usegnash)
			{
				LOG(LOG_INFO,"SWF version " << root->version << " is not handled by lightspark, falling back to gnash (if available)");
				//Enable flash fallback
				root->getSystemState()->needsAVM2(false);
				return; /* no more parsing necessary, handled by fallback */
			}
		}

		TagFactory factory(f);
		Tag* tag=factory.readTag(root);

		if (root->version >= 8)
		{
			FileAttributesTag* fat = dynamic_cast<FileAttributesTag*>(tag);
			if(!fat)
			{
				LOG(LOG_ERROR,"Invalid SWF - First tag must be a FileAttributesTag!");
				//Official implementation is not strict in this regard. Let's continue and hope for the best.
			}
			//Check if this clip is the main clip then honour its FileAttributesTag
			root->usesActionScript3 = fat ? fat->ActionScript3 : root->version>9;
			if(root == root->getSystemState()->mainClip)
			{
				root->getSystemState()->needsAVM2(!usegnash || root->usesActionScript3);
				if(usegnash && fat && !fat->ActionScript3)
				{
					delete fat;
					return; /* no more parsing necessary, handled by fallback */
				}
				if(fat && fat->UseNetwork
						&& root->getSystemState()->securityManager->getSandboxType() == SecurityManager::LOCAL_WITH_FILE)
				{
					root->getSystemState()->securityManager->setSandboxType(SecurityManager::LOCAL_WITH_NETWORK);
					LOG(LOG_INFO, "Switched to local-with-networking sandbox by FileAttributesTag");
				}
			}
		}
		else if(root == root->getSystemState()->mainClip)
		{
			root->getSystemState()->needsAVM2(true);
		}
		root->setupAVM1RootMovie();

		bool done=false;
		bool empty=true;
		while(!done)
		{
			lasttagtype = tag->getType();
			switch(tag->getType())
			{
				case END_TAG:
				{
					// The whole frame has been parsed, now execute all queued tags,
					// in the order in which they appeared in the file.
					while(!queuedTags.empty())
					{
						const ControlTag* t=queuedTags.front();
						t->execute(root);
						delete t;
						queuedTags.pop();
					}

					if(!empty)
						root->commitFrame(false);
					else
						root->revertFrame();

					RELEASE_WRITE(root->finishedLoading,true);
					done=true;
					root->check();
					delete tag;
					break;
				}
				case DICT_TAG:
				{
					DictionaryTag* d=static_cast<DictionaryTag*>(tag);
					root->addToDictionary(d);
					break;
				}
				case DISPLAY_LIST_TAG:
				{
					root->addToFrame(static_cast<DisplayListTag*>(tag));
					empty=false;
					break;
				}
				case AVM1ACTION_TAG:
				{
					if (!(static_cast<AVM1ActionTag*>(tag)->empty()))
						root->addAvm1ActionToFrame(static_cast<AVM1ActionTag*>(tag));
					empty=false;
					break;
				}
				case SHOW_TAG:
				{
					// The whole frame has been parsed, now execute all queued SymbolClass tags,
					// in the order in which they appeared in the file.
					while(!queuedTags.empty())
					{
						const ControlTag* t=queuedTags.front();
						t->execute(root);
						delete t;
						queuedTags.pop();
					}

					root->commitFrame(true);
					empty=true;
					delete tag;
					break;
				}
				case SYMBOL_CLASS_TAG:
				{
					root->hasSymbolClass = true;
				}
				// fall through
				case ABC_TAG:
				case ACTION_TAG:
				case AVM1INITACTION_TAG:
				{
					// Add symbol class tags or action to the queue, to be executed when the rest of the 
					// frame has been parsed. This is to handle invalid SWF files that define ID's
					// used in the SymbolClass tag only after the tag, which would otherwise result
					// in "undefined dictionary ID" errors.
					const ControlTag* stag = static_cast<const ControlTag*>(tag);
					queuedTags.push(stag);
					break;
				}
				case CONTROL_TAG:
				{
					/* The spec is not clear about that,
					 * but it seems that all the CONTROL_TAGs
					 * (=not one of the other tag types here)
					 * appear in the first frame only.
					 * We rely on that by not using
					 * locking in ctag->execute's implementation.
					 * ABC_TAG's are an exception, as they require no locking.
					 */
					if (root->frames.size()!=1)
					{
						delete tag;
						break;
					}
					const ControlTag* ctag = static_cast<const ControlTag*>(tag);
					ctag->execute(root);
					delete tag;
					break;
				}
				case FRAMELABEL_TAG:
				{
					/* No locking required, as the last frames is not
					 * commited to the vm yet.
					 */
					root->addFrameLabel(root->frames.size()-1,static_cast<const FrameLabelTag*>(tag)->Name);
					empty=false;
					delete tag;
					break;
				}
				case BUTTONSOUND_TAG:
				{
					if (!static_cast<DefineButtonSoundTag*>(tag)->button)
						delete tag;
					break;
				}
				case TAG:
				{
					//Not yet implemented tag, ignore it
					delete tag;
					break;
				}
			}// end switch
			if(root->getSystemState()->shouldTerminate() || threadAborting)
				break;

			if (!done)
				tag=factory.readTag(root);
		}// end while
	}
	catch(std::exception& e)
	{
		root->parsingFailed();
		throw;
	}
	if (lasttagtype != END_TAG || root->loaderInfo->getBytesLoaded() != root->loaderInfo->getBytesTotal())
	{
		LOG(LOG_NOT_IMPLEMENTED,"End of parsing, bytesLoaded != bytesTotal:"<< root->loaderInfo->getBytesLoaded()<<"/"<<root->loaderInfo->getBytesTotal());

		// On compressed swf files bytesLoaded and bytesTotal do not always match (besides being loaded properly, I don't know why...)
		// We just set bytesLoaded "manually" to ensure the "complete" event is dispatched
		root->loaderInfo->setBytesLoaded(root->loaderInfo->getBytesTotal());
	}
	root->markSoundFinished();
	LOG(LOG_TRACE,"End of parsing");
}

void ParseThread::parseBitmap()
{
	_NR<LoaderInfo> li;
	li=loader->getContentLoaderInfo();
	switch (fileType)
	{
		case FILE_TYPE::FT_PNG:
			li->contentType = "image/png";
			break;
		case FILE_TYPE::FT_JPEG:
			li->contentType = "image/jpeg";
			break;
		case FILE_TYPE::FT_GIF:
			li->contentType = "image/gif";
			break;
		default:
			break;
	}
	_NR<Bitmap> tmp;
	if (loader->needsActionScript3())
		tmp = _MNR(Class<Bitmap>::getInstanceS(loader->getInstanceWorker(),li, &f, fileType));
	else
		tmp = _MNR(Class<AVM1Bitmap>::getInstanceS(loader->getInstanceWorker(),li, &f, fileType));
	{
		Locker l(objectSpinlock);
		parsedObject=tmp;
	}
	if (li.getPtr())
		li->setComplete();
}

_NR<DisplayObject> ParseThread::getParsedObject()
{
	Locker l(objectSpinlock);
	return parsedObject;
}

void ParseThread::setRootMovie(RootMovieClip *root)
{
	Locker l(objectSpinlock);
	assert(root);
	root->incRef();
	parsedObject=_MNR(root);
}

void ParseThread::getSWFByteArray(ByteArray* ba)
{
	istream f2(bytearraybuf);
	f2.seekg(0,ios_base::end);
	uint32_t len = f2.tellg();
	f2.seekg(0);
	f2.read((char*)ba->getBuffer(len,true),len);
}

RootMovieClip* ParseThread::getRootMovie() const
{
	return dynamic_cast<RootMovieClip*>(parsedObject.getPtr());
}

void ParseThread::threadAbort()
{
	Locker l(objectSpinlock);
	if(parsedObject.isNull())
		return;
	RootMovieClip* root=getRootMovie();
	if(root==nullptr)
		return;
	root->parsingFailed();
}

bool RootMovieClip::boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const
{
	RECT f=getFrameSize();
	xmin=f.Xmin/20;
	ymin=f.Ymin/20;
	xmax=f.Xmax/20;
	ymax=f.Ymax/20;
	return true;
}

void RootMovieClip::setFrameSize(const lightspark::RECT& f)
{
	frameSize=f;
}

lightspark::RECT RootMovieClip::getFrameSize() const
{
	return frameSize;
}

void RootMovieClip::setFrameRate(float f)
{
	if (frameRate != f)
	{
		frameRate=f;
		if (this == getSystemState()->mainClip )
			getSystemState()->setRenderRate(frameRate);
	}
}

float RootMovieClip::getFrameRate() const
{
	return frameRate;
}

void RootMovieClip::commitFrame(bool another)
{
	setFramesLoaded(frames.size());

	if(another)
		frames.push_back(Frame());
	checkSound(frames.size());

	if(getFramesLoaded()==1 && frameRate!=0)
	{
		SystemState* sys = getSys();
		if(this==sys->mainClip || !hasMainClass)
		{
			/* now the frameRate is available and all SymbolClass tags have created their classes */
			// in AS3 this is added to the stage after the construction of the main object is completed (if a main object exists)
			if (!usesActionScript3 || !hasMainClass)
			{
				while (!getVm(sys)->hasEverStarted()) // ensure that all builtin classes are defined
					compat_msleep(10);
				constructionComplete();
				afterConstruction();
			}
		}
	}
}

void RootMovieClip::constructionComplete()
{
	if(isConstructed())
		return;
	if (!isVmThread() && !getInstanceWorker()->isPrimordial)
	{
		this->incRef();
		getVm(getSystemState())->prependEvent(NullRef,_MR(new (getSystemState()->unaccountedMemory) RootConstructedEvent(_MR(this))));
		return;
	}
	if (this!=getSystemState()->mainClip)
	{
		MovieClip::constructionComplete();
		return;
	}
	MovieClip::constructionComplete();
	if (!needsActionScript3())
	{
		declareFrame();
	}

	incRef();
	getSystemState()->stage->_addChildAt(_MR(this),0);
	this->setOnStage(true,true);
	if (!loaderInfo.isNull())
		loaderInfo->setComplete();
	getSystemState()->addTick(1000/frameRate,getSystemState());
}
void RootMovieClip::afterConstruction()
{
	DisplayObject::afterConstruction();
	checkFrameScriptToExecute();
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
void RootMovieClip::addToDictionary(DictionaryTag* r)
{
	Locker l(dictSpinlock);
	dictionary[r->getId()] = r;
}

/* called in vm's thread context */
DictionaryTag* RootMovieClip::dictionaryLookup(int id)
{
	Locker l(dictSpinlock);
	auto it = dictionary.find(id);
	if(it==dictionary.end())
	{
		LOG(LOG_ERROR,"No such Id on dictionary " << id << " for " << origin);
		//throw RunTimeException("Could not find an object on the dictionary");
		return nullptr;
	}
	return it->second;
}
DictionaryTag* RootMovieClip::dictionaryLookupByName(uint32_t nameID)
{
	Locker l(dictSpinlock);
	auto it = dictionary.begin();
	for(;it!=dictionary.end();++it)
	{
		if(it->second->nameID==nameID)
			break;
	}
	if(it==dictionary.end())
	{
		LOG(LOG_ERROR,"No such name on dictionary " << getSystemState()->getStringFromUniqueId(nameID) << " for " << origin);
		return nullptr;
	}
	return it->second;
}

void RootMovieClip::resizeCompleted()
{
	for(auto it=dictionary.begin();it!=dictionary.end();++it)
		it->second->resizeCompleted();
}

_NR<RootMovieClip> RootMovieClip::getRoot()
{
	this->incRef();
	return _MR(this);
}

_NR<Stage> RootMovieClip::getStage()
{
	getSystemState()->stage->incRef();
	return _MR(getSystemState()->stage);
}

/*ASObject* RootMovieClip::getVariableByQName(const tiny_string& name, const tiny_string& ns, ASObject*& owner)
{
	sem_wait(&mutex);
	ASObject* ret=ASObject::getVariableByQName(name, ns, owner);
	sem_post(&mutex);
	return ret;
}

void RootMovieClip::setVariableByMultiname(multiname& name, asAtom o)
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
	if (showProfilingData)
	{
		Locker l(profileDataSpinlock);
		list<ThreadProfile*>::iterator it=profilingData.begin();
		for(;it!=profilingData.end();++it)
			(*it)->tick();
	}
	if(currentVm==nullptr)
		return;
	/* See http://www.senocular.com/flash/tutorials/orderofoperations/
	 * for the description of steps.
	 */

	currentVm->setIdle(false);
	if (firsttick)
	{
		// the first AdvanceFrame is done during the construction of the RootMovieClip,
		// so we skip it here
		firsttick = false;
	}
	else
	{
		/* Step 0: Set current frame number to the next frame 
		 * Step 1: declare new objects */
		_R<AdvanceFrameEvent> advFrame = _MR(new (unaccountedMemory) AdvanceFrameEvent());
		currentVm->addEvent(NullRef, advFrame);
	}

	/* Step 2: Send enterFrame events, if needed */
	addBroadcastEvent("enterFrame");

	/* Step 3: create legacy objects, which are new in this frame (top-down),
	 * run their constructors (bottom-up) */
	stage->incRef();
	currentVm->addEvent(NullRef, _MR(new (unaccountedMemory) InitFrameEvent(_MR(stage))));

	/* Step 4: dispatch frameConstructed events */
	addBroadcastEvent("frameConstructed");

	/* Step 5: run all frameScripts (bottom-up) */
	stage->incRef();
	currentVm->addEvent(NullRef, _MR(new (unaccountedMemory) ExecuteFrameScriptEvent(_MR(stage))));

	/* Step 6: dispatch exitFrame event */
	addBroadcastEvent("exitFrame");
	/* Step 7: dispatch render event (Assuming stage.invalidate() has been called) */
	if (stage->invalidated)
	{
		RELEASE_WRITE(stage->invalidated,false);
		addBroadcastEvent("render");
	}

	/* Step 9: we are idle now, so we can handle all input events */
	_R<IdleEvent> idle = _MR(new (unaccountedMemory) IdleEvent());
	if (currentVm->addEvent(NullRef, idle))
		idle->wait();
}

void SystemState::tickFence()
{
}

void SystemState::resizeCompleted()
{
	mainClip->resizeCompleted();
	stage->hasChanged=true;
	stage->requestInvalidation(this,true);
	
	if(currentVm && scaleMode==NO_SCALE)
	{
		stage->incRef();
		currentVm->addEvent(_MR(stage),_MR(Class<Event>::getInstanceS(this->worker,"resize",false)));
		
		stage->incRef();
		currentVm->addEvent(_MR(stage),_MR(Class<StageVideoAvailabilityEvent>::getInstanceS(this->worker)));
	}
}

const tiny_string& SystemState::getStringFromUniqueId(uint32_t id) const
{
	Locker l(poolMutex);
	assert(uniqueStringIDMap.size() > id);
	return uniqueStringIDMap[id];
}

uint32_t SystemState::getUniqueStringId(const tiny_string& s)
{
	Locker l(poolMutex);
	auto it=uniqueStringMap.find(s);
	if(it==uniqueStringMap.end())
	{
		auto ret=uniqueStringMap.insert(make_pair(s,lastUsedStringId));
		uniqueStringIDMap.push_back(s);
		assert(ret.second);
		it=ret.first;
		lastUsedStringId++;
	}
	return it->second;
}

const nsNameAndKindImpl& SystemState::getNamespaceFromUniqueId(uint32_t id) const
{
	Locker l(poolMutex);
	auto it=uniqueNamespaceIDMap.find(id);
	assert(it!=uniqueNamespaceIDMap.end());
	return it->second;
}

void SystemState::getUniqueNamespaceId(const nsNameAndKindImpl& s, uint32_t& nsId, uint32_t& baseId)
{
	getUniqueNamespaceId(s, 0xffffffff, nsId, baseId);
}

void SystemState::getUniqueNamespaceId(const nsNameAndKindImpl& s, uint32_t hintedId, uint32_t& nsId, uint32_t& baseId)
{
	Locker l(poolMutex);
	auto it=uniqueNamespaceImplMap.find(s);
	if(it==uniqueNamespaceImplMap.end())
	{
		if (hintedId == 0xffffffff)
			hintedId=ATOMIC_DECREMENT(lastUsedNamespaceId);

		auto ret=uniqueNamespaceImplMap.insert(make_pair(s,hintedId));
		uniqueNamespaceIDMap.insert(make_pair(hintedId,s));
		assert(ret.second);
		it=ret.first;
	}

	nsId=it->second;
	baseId=(it->first.baseId==0xffffffff)?nsId:it->first.baseId;
}

void SystemState::stageCoordinateMapping(uint32_t windowWidth, uint32_t windowHeight,
					 int& offsetX, int& offsetY,
					 float& scaleX, float& scaleY)
{
	//Get the size of the content
	RECT r=mainClip->getFrameSize();
	r.Xmin/=20;
	r.Ymin/=20;
	r.Xmax/=20;
	r.Ymax/=20;
	//Now compute the scalings and offsets
	switch(scaleMode)
	{
		case SystemState::SHOW_ALL:
			//Compute both scaling
			scaleX=windowWidth;
			scaleX/=(r.Xmax-r.Xmin);
			scaleY=windowHeight;
			scaleY/=(r.Ymax-r.Ymin);
			//Enlarge with no distortion
			if(scaleX<scaleY)
			{
				//Uniform scaling for Y
				scaleY=scaleX;
				//Apply the offset
				offsetY=-r.Ymin+(windowHeight-(r.Ymax-r.Ymin)*scaleY)/2;
				offsetX=-r.Xmin;
			}
			else
			{
				//Uniform scaling for X
				scaleX=scaleY;
				//Apply the offset
				offsetX=-r.Xmin+(windowWidth-(r.Xmax-r.Xmin)*scaleX)/2;
				offsetY=-r.Ymin;
			}
			break;
		case SystemState::NO_BORDER:
			//Compute both scaling
			scaleX=windowWidth;
			scaleX/=(r.Xmax-r.Xmin);
			scaleY=windowHeight;
			scaleY/=(r.Ymax-r.Ymin);
			//Enlarge with no distortion
			if(scaleX>scaleY)
			{
				//Uniform scaling for Y
				scaleY=scaleX;
				//Apply the offset
				offsetY=-r.Ymin+(windowHeight-(r.Ymax-r.Ymin)*scaleY)/2;
				offsetX=-r.Xmin;
			}
			else
			{
				//Uniform scaling for X
				scaleX=scaleY;
				//Apply the offset
				offsetX=-r.Xmin+(windowWidth-(r.Xmax-r.Xmin)*scaleX)/2;
				offsetY=-r.Ymin;
			}
			break;
		case SystemState::EXACT_FIT:
			//Compute both scaling
			scaleX=windowWidth;
			scaleX/=(r.Xmax-r.Xmin);
			scaleY=windowHeight;
			scaleY/=(r.Ymax-r.Ymin);
			offsetX=-r.Xmin;
			offsetY=-r.Ymin;
			break;
		case SystemState::NO_SCALE:
			scaleX=1;
			scaleY=1;
			offsetX=-r.Xmin;
			offsetY=-r.Ymin;
			break;
	}
}

void SystemState::windowToStageCoordinates(int windowX, int windowY, int& stageX, int& stageY)
{
	int offsetX;
	int offsetY;
	float scaleX;
	float scaleY;
	stageCoordinateMapping(renderThread->windowWidth, renderThread->windowHeight,
			       offsetX, offsetY, scaleX, scaleY);
	stageX = (windowX-offsetX)/scaleX;
	stageY = (windowY-offsetY)/scaleY;
}

void SystemState::openPageInBrowser(const tiny_string& url, const tiny_string& window)
{
	assert(engineData);
	engineData->openPageInBrowser(url, window);
}

void SystemState::showMouseCursor(bool visible)
{
	if (visible)
		engineData->runInMainThread(this,&EngineData::showMouseCursor);
	else
		engineData->runInMainThread(this,&EngineData::hideMouseCursor);
}
void SystemState::setMouseHandCursor(bool sethand)
{
	if (sethand)
		engineData->runInMainThread(this,&EngineData::setMouseHandCursor);
	else
		engineData->runInMainThread(this,&EngineData::resetMouseHandCursor);
}

void SystemState::waitRendering()
{
	getRenderThread()->waitRendering();
}

uint32_t SystemState::getSwfVersion()
{
	return mainClip->version;
}

void SystemState::checkExternalCallEvent()
{
	if (currentVm && isVmThread())
		currentVm->checkExternalCallEvent();
}
void SystemState::waitMainSignal()
{
	checkExternalCallEvent();
	{
		Locker l(mainsignalMutex);
		mainsignalCond.wait(mainsignalMutex);
	}
	checkExternalCallEvent();
}
void SystemState::sendMainSignal()
{
	mainsignalCond.broadcast();
}

void SystemState::waitInitialized()
{
	if (isinitialized)
		return;
	{
		Locker l(initializedMutex);
		initializedCond.wait(initializedMutex);
	}
}

void SystemState::getClassInstanceByName(ASWorker* wrk,asAtom& ret, const tiny_string &clsname)
{
	Class_base* c= nullptr;
	auto it = classnamemap.find(clsname);
	if (it == classnamemap.end())
	{
		asAtom cls = asAtomHandler::invalidAtom;
		asAtom tmp = asAtomHandler::invalidAtom;
		asAtom args = asAtomHandler::fromString(this,clsname);
		getDefinitionByName(cls,wrk,tmp,&args,1);
		assert_and_throw(asAtomHandler::isValid(cls));
		c = asAtomHandler::getObjectNoCheck(cls)->as<Class_base>();
		classnamemap[clsname]=c;
	}
	else
		c = it->second;
	c->getInstance(wrk,ret,true,nullptr,0);
}
/* This is run in vm's thread context */
void RootMovieClip::initFrame()
{
	if (waitingforparser)
		return;
	LOG_CALL("Root:initFrame " << getFramesLoaded() << " " << state.FP<<" "<<state.stop_FP<<" "<<state.next_FP<<" "<<state.explicit_FP);
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
	/* We have to wait until enough frames are available */
	if(getFramesLoaded() == 0 || (state.next_FP>=(uint32_t)getFramesLoaded() && !hasFinishedLoading()))
	{
		waitingforparser=true;
		return;
	}
	waitingforparser=false;

	MovieClip::advanceFrame();
}

void RootMovieClip::executeFrameScript()
{
	if (waitingforparser)
		return;
	MovieClip::executeFrameScript();
}

bool RootMovieClip::destruct()
{
	applicationDomain.reset();
	securityDomain.reset();
	waitingforparser=false;
	parsethread=nullptr;
	return MovieClip::destruct();
}
void RootMovieClip::finalize()
{
	applicationDomain.reset();
	securityDomain.reset();
	parsethread=nullptr;
	MovieClip::finalize();
}

void RootMovieClip::addBinding(const tiny_string& name, DictionaryTag *tag)
{
	// This function will be called only be the parsing thread,
	// and will only access the last frame, so no locking needed.
	tag->bindingclassname = name;
	uint32_t pos = name.rfind(".");
	if (pos== tiny_string::npos)
		classesToBeBound[QName(getSystemState()->getUniqueStringId(name),BUILTIN_STRINGS::EMPTY)] =  tag;
	else
		classesToBeBound[QName(getSystemState()->getUniqueStringId(name.substr(pos+1,name.numChars()-(pos+1))),getSystemState()->getUniqueStringId(name.substr(0,pos)))] = tag;
}

void RootMovieClip::bindClass(const QName& classname, Class_inherit* cls)
{
	if (cls->isBinded() || classesToBeBound.empty())
		return;

	auto it=classesToBeBound.find(classname);
	if(it!=classesToBeBound.end())
	{
		cls->bindToTag(it->second);
		classesToBeBound.erase(it);
	}
}

void RootMovieClip::checkBinding(DictionaryTag *tag)
{
	if (tag->bindingclassname.empty())
		return;
	multiname clsname(nullptr);
	clsname.name_type=multiname::NAME_STRING;
	clsname.isAttribute = false;

	uint32_t pos = tag->bindingclassname.rfind(".");
	tiny_string ns;
	tiny_string nm;
	if (pos != tiny_string::npos)
	{
		nm = tag->bindingclassname.substr(pos+1,tag->bindingclassname.numBytes());
		ns = tag->bindingclassname.substr(0,pos);
		clsname.hasEmptyNS=false;
	}
	else
	{
		nm = tag->bindingclassname;
		ns = "";
	}
	clsname.name_s_id=getSystemState()->getUniqueStringId(nm);
	clsname.ns.push_back(nsNameAndKind(getSystemState(),ns,NAMESPACE));
	
	ASObject* typeObject = nullptr;
	auto i = applicationDomain->classesBeingDefined.cbegin();
	while (i != applicationDomain->classesBeingDefined.cend())
	{
		if(i->first->name_s_id == clsname.name_s_id && i->first->ns[0].nsRealId == clsname.ns[0].nsRealId)
		{
			typeObject = i->second;
			break;
		}
		i++;
	}
	if (typeObject == nullptr)
	{
		ASObject* target;
		asAtom o=asAtomHandler::invalidAtom;
		applicationDomain->getVariableAndTargetByMultiname(o,clsname,target,getInstanceWorker());
		if (asAtomHandler::isValid(o))
			typeObject=asAtomHandler::getObject(o);
	}
	if (typeObject != nullptr)
	{
		Class_inherit* cls = typeObject->as<Class_inherit>();
		if (cls)
		{
			ABCVm *vm = getVm(getSystemState());
			vm->buildClassAndBindTag(tag->bindingclassname.raw_buf(), tag,cls);
			tag->bindedTo=cls;
			tag->bindingclassname = "";
			cls->bindToTag(tag);
		}
	}
}

void RootMovieClip::registerEmbeddedFont(const tiny_string fontname, FontTag *tag)
{
	if (!fontname.empty())
	{
		auto it = embeddedfonts.find(fontname);
		if (it == embeddedfonts.end())
		{
			embeddedfonts[fontname] = tag;
			// it seems that adobe allows fontnames to be lowercased and stripped of spaces and numbers
			tiny_string tmp = fontname.lowercase();
			tiny_string fontnamenormalized;
			for (auto it = tmp.begin();it != tmp.end(); it++)
			{
				if (*it == ' ' || (*it >= '0' &&  *it <= '9'))
					continue;
				fontnamenormalized += *it;
			}
			embeddedfonts[fontnamenormalized] = tag;
		}
	}
	embeddedfontsByID[tag->getId()] = tag;
}

FontTag *RootMovieClip::getEmbeddedFont(const tiny_string fontname) const
{
	auto it = embeddedfonts.find(fontname);
	if (it != embeddedfonts.end())
		return it->second;
	it = embeddedfonts.find(fontname.lowercase());
	if (it != embeddedfonts.end())
		return it->second;
	return nullptr;
}
FontTag *RootMovieClip::getEmbeddedFontByID(uint32_t fontID) const
{
	auto it = embeddedfontsByID.find(fontID);
	if (it != embeddedfontsByID.end())
		return it->second;
	return nullptr;
}

void RootMovieClip::setupAVM1RootMovie()
{
	if (!usesActionScript3)
	{
		this->classdef = Class<AVM1MovieClip>::getRef(getSystemState()).getPtr();
		if (!getSystemState()->avm1global)
			getVm(getSystemState())->registerClassesAVM1();
		// it seems that the url parameters and flash vars are made available as properties of the root movie clip
		// I haven't found anything in the documentation but gnash also does this
		_NR<ASObject> params = getSystemState()->getParameters();
		if (params)
			params->copyValues(this,getInstanceWorker());
	}
}

bool RootMovieClip::AVM1registerTagClass(const tiny_string &name, _NR<IFunction> theClassConstructor)
{
	uint32_t nameID = getSystemState()->getUniqueStringId(name);
	DictionaryTag* t = dictionaryLookupByName(nameID);
	if (!t)
	{
		LOG(LOG_ERROR,"registerClass:no tag found in dictionary for "<<name);
		return false;
	}
	if (theClassConstructor.isNull())
		avm1ClassConstructors.erase(t->getId());
	else
		avm1ClassConstructors.insert(make_pair(t->getId(),theClassConstructor));
	return true;
}

AVM1Function* RootMovieClip::AVM1getClassConstructor(uint32_t spriteID)
{
	auto it = avm1ClassConstructors.find(spriteID);
	if (it == avm1ClassConstructors.end())
		return nullptr;
	return it->second->is<AVM1Function>() ? it->second->as<AVM1Function>() : nullptr;
}

void RootMovieClip::AVM1registerInitActionTag(uint32_t spriteID, AVM1InitActionTag *tag)
{
	avm1InitActionTags[spriteID] = tag;
}
void RootMovieClip::AVM1checkInitActions(MovieClip* sprite)
{
	if (!sprite)
		return;
	auto it = avm1InitActionTags.find(sprite->getTagID());
	if (it != avm1InitActionTags.end())
	{
		AVM1InitActionTag* t = it->second;
		// a new instance of the sprite may be constructed during code execution, so we remove it from the initactionlist before executing the code to ensure it's only executed once
		avm1InitActionTags.erase(it);
		t->executeDirect(sprite);
	}
}
