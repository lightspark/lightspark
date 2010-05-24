/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009,2010  Alessandro Pignotti (a.pignotti@sssup.it)

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

#include <string>
#include <sstream>
#include <pthread.h>
#include <algorithm>
#include "compat.h"
#include <SDL.h>
#include "abc.h"
#include "flashdisplay.h"
#include "flashevents.h"
#include "swf.h"
#include "logger.h"
#include "actions.h"
#include "streams.h"
#include "asobjects.h"
#include "textfile.h"
#include "class.h"
#include "netutils.h"

#include <GL/glew.h>
#include <curl/curl.h>
extern "C" {
#include <libavcodec/avcodec.h>
}
#ifndef WIN32
#include <GL/glx.h>
#endif

#ifdef COMPILE_PLUGIN
#include <gdk/gdkgl.h>
#include <gtk/gtkglwidget.h>
#endif

using namespace std;
using namespace lightspark;

extern TLSDATA SystemState* sys;
extern TLSDATA RenderThread* rt;
extern TLSDATA ParseThread* pt;

SWF_HEADER::SWF_HEADER(istream& in):valid(false)
{
	in >> Signature[0] >> Signature[1] >> Signature[2];

	in >> Version >> FileLength;
	if(Signature[0]=='F' && Signature[1]=='W' && Signature[2]=='S')
	{
		LOG(LOG_NO_INFO, "Uncompressed SWF file: Version " << (int)Version << " Length " << FileLength);
	}
	else if(Signature[0]=='C' && Signature[1]=='W' && Signature[2]=='S')
	{
		LOG(LOG_NO_INFO, "Compressed SWF file: Version " << (int)Version << " Length " << FileLength);
	}
	else
	{
		LOG(LOG_NO_INFO,"No SWF file signature found");
		return;
	}
	pt->version=Version;
	in >> FrameSize >> FrameRate >> FrameCount;
	float frameRate=FrameRate;
	frameRate/=256;
	LOG(LOG_NO_INFO,"FrameRate " << frameRate);

	pt->root->setFrameRate(frameRate);
	//TODO: setting render rate should be done when the clip is added to the displaylist
	sys->setRenderRate(frameRate);
	pt->root->version=Version;
	pt->root->fileLenght=FileLength;
	valid=true;
}

RootMovieClip::RootMovieClip(LoaderInfo* li, bool isSys):initialized(false),parsingIsFailed(false),frameRate(0),toBind(false)
{
	root=this;
	sem_init(&mutex,0,1);
	sem_init(&sem_frames,0,1);
	sem_init(&new_frame,0,0);
	sem_init(&sem_valid_size,0,0);
	sem_init(&sem_valid_rate,0,0);
	loaderInfo=li;
	//Reset framesLoaded, as there are still not available
	framesLoaded=0;

	//We set the protoype to a generic MovieClip
	if(!isSys)
	{
		prototype=Class<MovieClip>::getClass();
		prototype->incRef();
	}
}

RootMovieClip::~RootMovieClip()
{
	sem_destroy(&mutex);
	sem_destroy(&sem_frames);
	sem_destroy(&new_frame);
	sem_destroy(&sem_valid_rate);
	sem_destroy(&sem_valid_size);
}

void RootMovieClip::parsingFailed()
{
	//The parsing is failed, we have no change to be ever valid
	parsingIsFailed=true;
	sem_post(&new_frame);
	sem_post(&sem_valid_size);
	sem_post(&sem_valid_rate);
}

void RootMovieClip::bindToName(const tiny_string& n)
{
	assert(toBind==false);
	toBind=true;
	bindName=n;
}

SystemState::SystemState():RootMovieClip(NULL,true),renderRate(0),error(false),shutdown(false),showProfilingData(false),
	showInteractiveMap(false),showDebug(false),xOffset(0),yOffset(0),currentVm(NULL),inputThread(NULL),
	renderThread(NULL),useInterpreter(true),useJit(false), downloadManager(NULL)
{
	//Do needed global initialization
	curl_global_init(CURL_GLOBAL_ALL);
	avcodec_register_all();

	//Create the thread pool
	sys=this;
	sem_init(&terminated,0,0);

	//Get starting time
	threadPool=new ThreadPool(this);
	timerThread=new TimerThread(this);
	loaderInfo=Class<LoaderInfo>::getInstanceS();
	stage=Class<Stage>::getInstanceS();
	startTime=compat_msectiming();
	
	prototype=Class<MovieClip>::getClass();
	prototype->incRef();
}

void SystemState::setUrl(const tiny_string& url)
{
	loaderInfo->url=url;
	loaderInfo->loaderURL=url;
}

void SystemState::parseParameters(istream& i)
{
	ASObject* ret=new ASObject;
	while(!i.eof())
	{
		string name,value;
		getline(i,name);
		getline(i,value);

		ret->setVariableByQName(name.c_str(),"",Class<ASString>::getInstanceS(value));
	}
	setParameters(ret);
}

void SystemState::setParameters(ASObject* p)
{
	loaderInfo->setVariableByQName("parameters","",p);
}

SystemState::~SystemState()
{
	assert(shutdown);
	timerThread->stop();
	delete threadPool;
	delete downloadManager;
	delete currentVm;
	delete timerThread;

	//decRef all registered classes
	std::map<tiny_string, Class_base*>::iterator it=classes.begin();
	for(;it!=classes.end();++it)
		it->second->decRef();

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

void SystemState::setError(string& c)
{
	//We record only the first error for easier fix and reporting
	if(!error)
	{
		error=true;
		errorCause=c;
		timerThread->stop();
		if(renderThread)
		{
			//Disable timed rendering
			removeJob(renderThread);
			renderThread->draw();
		}
	}
}

void SystemState::setShutdownFlag()
{
	sem_wait(&mutex);
	shutdown=true;
	if(currentVm)
		currentVm->addEvent(NULL,new ShutdownEvent());

	sem_post(&terminated);
	sem_post(&mutex);
}

void SystemState::wait()
{
	sem_wait(&terminated);
}

float SystemState::getRenderRate()
{
	return renderRate;
}

void SystemState::setRenderRate(float rate)
{
	if(renderRate>=rate)
		return;
	
	//The requested rate is higher, let's reschedule the job
	renderRate=rate;
	assert(renderThread);
	removeJob(renderThread);
	addTick(1000/rate,renderThread);
}

void SystemState::tick()
{
	inputThread->broadcastEvent("enterFrame");
 	sem_wait(&mutex);
	list<ThreadProfile>::iterator it=profilingData.begin();
	for(;it!=profilingData.end();it++)
		it->tick();
	sem_post(&mutex);
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
	sem_wait(&mutex);
	profilingData.push_back(ThreadProfile(color,100));
	ThreadProfile* ret=&profilingData.back();
	sem_post(&mutex);
	return ret;
}

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
				font->Render(curTag->c_str() ,-1,FTPoint(curTagX,max(curTagY-curTagH,0)));
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
				curTagY=min(curTagY,data[i].timing*height/maxTime);
			else
			{
				//Tag is before this sample
				font->Render(curTag->c_str() ,-1,FTPoint(curTagX,max(curTagY-curTagH,0)));
				curTag=NULL;
			}
		}
	}
}

ParseThread::ParseThread(RootMovieClip* r,istream& in):f(in)
{
	root=r;
	sem_init(&ended,0,0);
}

ParseThread::~ParseThread()
{
	sem_destroy(&ended);
}

void ParseThread::execute()
{
	pt=this;
	try
	{
		SWF_HEADER h(f);
		if(!h.valid)
			throw ParseException("Not an SWF file");
		root->setFrameSize(h.getFrameSize());
		root->setFrameCount(h.FrameCount);

		TagFactory factory(f);
		bool done=false;
		bool empty=true;
		while(!done)
		{
			Tag* tag=factory.readTag();
			switch(tag->getType())
			{
				case END_TAG:
				{
					LOG(LOG_NO_INFO,"End of parsing @ " << f.tellg());
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
		LOG(LOG_ERROR,"Exception in ParseThread " << e.what());
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

void ParseThread::wait()
{
	sem_wait(&ended);
}

void RenderThread::wait()
{
	if(terminated)
		return;
	terminated=true;
	//Signal potentially blocking semaphore
	sem_post(&render);
	int ret=pthread_join(t,NULL);
	assert(ret==0);
}

InputThread::InputThread(SystemState* s,ENGINE e, void* param):m_sys(s),t(0),terminated(false),
	mutexListeners("Input listeners"),mutexDragged("Input dragged"),curDragged(NULL)
{
	LOG(LOG_NO_INFO,"Creating input thread");
	if(e==SDL)
		pthread_create(&t,NULL,(thread_worker)sdl_worker,this);
#ifdef COMPILE_PLUGIN
	else if(e==NPAPI)
	{
		npapi_params=(NPAPI_params*)param;
		//Under NPAPI is a bad idea to handle input in another thread
		//pthread_create(&t,NULL,(thread_worker)npapi_worker,this);
		
		//Let's hook into the Xt event handling of the browser
		X11Intrinsic::Widget xtwidget = X11Intrinsic::XtWindowToWidget(npapi_params->display, npapi_params->window);
		assert(xtwidget);

		//mXtwidget = xtwidget;
		long event_mask = ExposureMask|PointerMotionMask|ButtonPressMask|KeyPressMask;
		XSelectInput(npapi_params->display, npapi_params->window, event_mask);
		X11Intrinsic::XtAddEventHandler(xtwidget, event_mask, False, (X11Intrinsic::XtEventHandler)npapi_worker, this);
	}
	else if(e==GTKPLUG)
	{
		gdk_threads_enter();
		npapi_params=(NPAPI_params*)param;
		GtkWidget* container=npapi_params->container;
		gtk_widget_set_can_focus(container,True);
		gtk_widget_add_events(container,GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK |
						GDK_POINTER_MOTION_MASK | GDK_SCROLL_MASK | GDK_EXPOSURE_MASK | GDK_VISIBILITY_NOTIFY_MASK |
						GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK | GDK_FOCUS_CHANGE_MASK);
		g_signal_connect(G_OBJECT(container), "event", G_CALLBACK(gtkplug_worker), this);
		gdk_threads_leave();
	}
#endif
	else
		::abort();
}

InputThread::~InputThread()
{
	wait();
}

void InputThread::wait()
{
	if(terminated)
		return;
	if(t)
		pthread_join(t,NULL);
	terminated=true;
}

#ifdef COMPILE_PLUGIN
//This is a GTK event handler and the gdk lock is already acquired
gboolean InputThread::gtkplug_worker(GtkWidget *widget, GdkEvent *event, InputThread* th)
{
	cout << "Event" << endl;
	return False;
}

void InputThread::npapi_worker(X11Intrinsic::Widget xt_w, InputThread* th, XEvent* xevent, X11Intrinsic::Boolean* b)
{
	switch(xevent->type)
	{
		case KeyPress:
		{
			int key=XLookupKeysym(&xevent->xkey,0);
			switch(key)
			{
				case 'd':
					th->m_sys->showDebug=!th->m_sys->showDebug;
					break;
				case 'i':
					th->m_sys->showInteractiveMap=!th->m_sys->showInteractiveMap;
					break;
				case 'p':
					th->m_sys->showProfilingData=!th->m_sys->showProfilingData;
					break;
				default:
					break;
			}
			*b=False;
			break;
		}
		case Expose:
			//Signal the renderThread
			th->m_sys->renderThread->draw();
			*b=False;
			break;
		default:
			*b=True;
#ifdef EXPENSIVE_DEBUG
			cout << "TYPE " << dec << xevent->type << endl;
#endif
	}
}
#endif

void* InputThread::sdl_worker(InputThread* th)
{
	sys=th->m_sys;
	SDL_Event event;
	while(SDL_WaitEvent(&event))
	{
		switch(event.type)
		{
			case SDL_KEYDOWN:
			{
				switch(event.key.keysym.sym)
				{
					case SDLK_d:
						th->m_sys->showDebug=!th->m_sys->showDebug;
						break;
					case SDLK_i:
						th->m_sys->showInteractiveMap=!th->m_sys->showInteractiveMap;
						break;
					case SDLK_p:
						th->m_sys->showProfilingData=!th->m_sys->showProfilingData;
						break;
					case SDLK_q:
						th->m_sys->setShutdownFlag();
						if(th->m_sys->currentVm)
							LOG(LOG_CALLS,"We still miss " << sys->currentVm->getEventQueueSize() << " events");
						pthread_exit(0);
						break;
					case SDLK_s:
						th->m_sys->state.stop_FP=true;
						break;
					case SDLK_DOWN:
						th->m_sys->yOffset-=10;
						break;
					case SDLK_UP:
						th->m_sys->yOffset+=10;
						break;
					case SDLK_LEFT:
						th->m_sys->xOffset-=10;
						break;
					case SDLK_RIGHT:
						th->m_sys->xOffset+=10;
						break;
					//Ignore any other keystrokes
					default:
						break;
				}
				break;
			}
			case SDL_MOUSEBUTTONDOWN:
			{
				Locker locker(th->mutexListeners);
				sys->renderThread->requestInput();
				float selected=sys->renderThread->getIdAt(event.button.x,event.button.y);
				if(selected==0)
				{
					sys->renderThread->selectedDebug=NULL;
					break;
				}

				int index=lrint(th->listeners.size()*selected);
				index--;

				//Add event to the event queue
				sys->currentVm->addEvent(th->listeners[index],Class<Event>::getInstanceS("mouseDown"));
				//And select that object for debugging (if needed)
				if(sys->showDebug)
					sys->renderThread->selectedDebug=th->listeners[index];
				break;
			}
		}
	}
	return NULL;
}

void InputThread::addListener(InteractiveObject* ob)
{
	Locker locker(mutexListeners);

#ifndef NDEBUG
	vector<InteractiveObject*>::const_iterator it=find(listeners.begin(),listeners.end(),ob);
	//Object is already register, should not happen
	assert(it==listeners.end());
#endif
	
	//Register the listener
	listeners.push_back(ob);
	unsigned int count=listeners.size();

	//Set a unique id for listeners in the range [0,1]
	//count is the number of listeners, this is correct so that no one gets 0
	float increment=1.0f/count;
	float cur=increment;
	for(unsigned int i=0;i<count;i++)
	{
		listeners[i]->setId(cur);
		cur+=increment;
	}
}

void InputThread::removeListener(InteractiveObject* ob)
{
	Locker locker(mutexListeners);

	vector<InteractiveObject*>::iterator it=find(listeners.begin(),listeners.end(),ob);
	if(it==listeners.end()) //Listener not found
		return;
	
	//Unregister the listener
	listeners.erase(it);
	
	unsigned int count=listeners.size();

	//Set a unique id for listeners in the range [0,1]
	//count is the number of listeners, this is correct so that no one gets 0
	float increment=1.0f/count;
	float cur=increment;
	for(unsigned int i=0;i<count;i++)
	{
		listeners[i]->setId(cur);
		cur+=increment;
	}
}

void InputThread::broadcastEvent(const tiny_string& t)
{
	Event* e=Class<Event>::getInstanceS(t);
	Locker locker(mutexListeners);
	assert(e->getRefCount()==1);
	for(unsigned int i=0;i<listeners.size();i++)
		sys->currentVm->addEvent(listeners[i],e);
	//while(e->getRefCount()>1){sleep(1);}
	e->check();
	e->decRef();
}

void InputThread::enableDrag(Sprite* s, const lightspark::RECT& limit)
{
	Locker locker(mutexDragged);
	if(s==curDragged)
		return;
	
	if(curDragged) //Stop dragging the previous sprite
		curDragged->decRef();
	
	assert(s);
	//We need to avoid that the object is destroyed
	s->incRef();
	
	curDragged=s;
	dragLimit=limit;
}

void InputThread::disableDrag()
{
	Locker locker(mutexDragged);
	if(curDragged)
	{
		curDragged->decRef();
		curDragged=NULL;
	}
}

RenderThread::RenderThread(SystemState* s,ENGINE e,void* params):m_sys(s),terminated(false),inputNeeded(false),
	interactive_buffer(NULL),fbAcquired(false),frameCount(0),secsCount(0),selectedDebug(NULL),currentId(0),
	materialOverride(false)
{
	LOG(LOG_NO_INFO,"RenderThread this=" << this);
	m_sys=s;
	sem_init(&render,0,0);
	sem_init(&inputDone,0,0);
	if(e==SDL)
		pthread_create(&t,NULL,(thread_worker)sdl_worker,this);
#ifdef COMPILE_PLUGIN
	else if(e==GTKPLUG)
	{
		npapi_params=(NPAPI_params*)params;
		pthread_create(&t,NULL,(thread_worker)gtkplug_worker,this);
	}
	else if(e==NPAPI)
	{
		npapi_params=(NPAPI_params*)params;
		pthread_create(&t,NULL,(thread_worker)npapi_worker,this);
	}
	clock_gettime(CLOCK_REALTIME,&ts);
#endif
}

RenderThread::~RenderThread()
{
	wait();
	sem_destroy(&render);
	sem_destroy(&inputDone);
	delete[] interactive_buffer;
	LOG(LOG_NO_INFO,"~RenderThread this=" << this);
}

void RenderThread::glClearIdBuffer()
{
	glDrawBuffer(GL_COLOR_ATTACHMENT2);
	
	glDisable(GL_BLEND);
	glClearColor(0,0,0,1);
	glClear(GL_COLOR_BUFFER_BIT);
}

void RenderThread::requestInput()
{
	inputNeeded=true;
	sem_post(&render);
	sem_wait(&inputDone);
}

bool RenderThread::glAcquireIdBuffer()
{
	if(currentId!=0)
	{
		glDrawBuffer(GL_COLOR_ATTACHMENT2);
		materialOverride=true;
		FILLSTYLE::fixedColor(currentId,currentId,currentId);
		return true;
	}
	
	return false;
}

void RenderThread::glAcquireFramebuffer(number_t xmin, number_t xmax, number_t ymin, number_t ymax)
{
	assert(fbAcquired==false);
	fbAcquired=true;

	glDrawBuffer(GL_COLOR_ATTACHMENT1);
	materialOverride=false;
	
	glDisable(GL_BLEND);
	glColor4f(0,0,0,0); //No output is fairly ok to clear
	glBegin(GL_QUADS);
		glVertex2f(xmin,ymin);
		glVertex2f(xmax,ymin);
		glVertex2f(xmax,ymax);
		glVertex2f(xmin,ymax);
	glEnd();
}

void RenderThread::glBlitFramebuffer(number_t xmin, number_t xmax, number_t ymin, number_t ymax)
{
	assert(fbAcquired==true);
	fbAcquired=false;

	//Use the blittler program to blit only the used buffer
	glUseProgram(blitter_program);
	glEnable(GL_BLEND);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	glBindTexture(GL_TEXTURE_2D,rt->spare_tex);
	glBegin(GL_QUADS);
		glVertex2f(xmin,ymin);
		glVertex2f(xmax,ymin);
		glVertex2f(xmax,ymax);
		glVertex2f(xmin,ymax);
	glEnd();
	glUseProgram(gpu_program);
}

#ifdef COMPILE_PLUGIN
void* RenderThread::gtkplug_worker(RenderThread* th)
{
	sys=th->m_sys;
	rt=th;
	NPAPI_params* p=th->npapi_params;

	RECT size=sys->getFrameSize();
	int swf_width=size.Xmax/20;
	int swf_height=size.Ymax/20;

	int window_width=p->width;
	int window_height=p->height;

	float scalex=window_width;
	scalex/=swf_width;
	float scaley=window_height;
	scaley/=swf_height;

	rt->width=window_width;
	rt->height=window_height;
	th->interactive_buffer=new uint32_t[window_width*window_height];
	unsigned int t2[3];
	gdk_threads_enter();
	GtkWidget* container=p->container;
	gtk_widget_show(container);
	gdk_gl_init(0, 0);
	GdkGLConfig* glConfig=gdk_gl_config_new_by_mode((GdkGLConfigMode)(GDK_GL_MODE_RGBA|GDK_GL_MODE_DOUBLE|GDK_GL_MODE_DEPTH));
	assert(glConfig);
	
	GtkWidget* drawing_area = gtk_drawing_area_new ();
	//gtk_widget_set_size_request (drawing_area, 0, 0);

	gtk_widget_set_gl_capability (drawing_area,glConfig,NULL,TRUE,GDK_GL_RGBA_TYPE);
	gtk_widget_show (drawing_area);
	gtk_container_add (GTK_CONTAINER (container), drawing_area);
	GdkGLContext *glContext = gtk_widget_get_gl_context (drawing_area);
	GdkGLDrawable *glDrawable = gtk_widget_get_gl_drawable(drawing_area);	
	bool ret=gdk_gl_drawable_gl_begin(glDrawable,glContext);
	assert(ret);
	th->commonGLInit(window_width, window_height, t2);
	ThreadProfile* profile=sys->allocateProfiler(RGB(200,0,0));
	profile->setTag("Render");
	FTTextureFont font("/usr/share/fonts/truetype/ttf-liberation/LiberationSerif-Regular.ttf");
	if(font.Error())
		throw RunTimeException("Unable to load font");
	
	font.FaceSize(20);

	glEnable(GL_TEXTURE_2D);
	gdk_gl_drawable_gl_end(glDrawable);
	gdk_threads_leave();
	Chronometer chronometer;
	try
	{
		while(1)
		{
			sem_wait(&th->render);
			if(th->m_sys->isShuttingDown())
				pthread_exit(0);

			chronometer.checkpoint();

			gdk_threads_enter();
			bool ret=gdk_gl_drawable_gl_begin(glDrawable,glContext);
			assert(ret);
			gdk_gl_drawable_swap_buffers(glDrawable);

			if(th->inputNeeded)
			{
				glBindTexture(GL_TEXTURE_2D,t2[2]);
				glGetTexImage(GL_TEXTURE_2D,0,GL_BGRA,GL_UNSIGNED_BYTE,th->interactive_buffer);
				th->inputNeeded=false;
				sem_post(&th->inputDone);
			}

			//Before starting rendering, cleanup all the request arrived in the meantime
			int fakeRenderCount=0;
			while(sem_trywait(&th->render)==0)
			{
				if(th->m_sys->isShuttingDown())
					pthread_exit(0);
				fakeRenderCount++;
			}

			if(fakeRenderCount)
				LOG(LOG_NO_INFO,"Faking " << fakeRenderCount << " renderings");

			glBindFramebuffer(GL_FRAMEBUFFER, rt->fboId);
			//Clear the id buffer
			glDrawBuffer(GL_COLOR_ATTACHMENT2);
			glClearColor(1,0,0,1);
			glClear(GL_COLOR_BUFFER_BIT);
			
			//Clear the back buffer
			glDrawBuffer(GL_COLOR_ATTACHMENT0);
			RGB bg=sys->getBackground();
			glClearColor(bg.Red/255.0F,bg.Green/255.0F,bg.Blue/255.0F,1);
			glClear(GL_COLOR_BUFFER_BIT);

			glLoadIdentity();
			glTranslatef(th->m_sys->xOffset,th->m_sys->yOffset,0);
			glScalef(scalex,scaley,1);
			
			th->m_sys->Render();

			glFlush();

			glLoadIdentity();

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glDrawBuffer(GL_BACK);
			glUseProgram(0);

			glBindTexture(GL_TEXTURE_2D,((sys->showInteractiveMap)?t2[2]:t2[0]));
			glBegin(GL_QUADS);
				glTexCoord2f(0,1);
				glVertex2i(0,0);
				glTexCoord2f(1,1);
				glVertex2i(th->width,0);
				glTexCoord2f(1,0);
				glVertex2i(th->width,th->height);
				glTexCoord2f(0,0);
				glVertex2i(0,th->height);
			glEnd();
			
			if(th->m_sys->showDebug)
			{
				glDisable(GL_TEXTURE_2D);
				th->m_sys->debugRender(&font, true);
				glEnable(GL_TEXTURE_2D);
			}

			if(th->m_sys->showProfilingData)
			{
				glColor3f(0,0,0);
				char frameBuf[20];
				snprintf(frameBuf,20,"Frame %u",th->m_sys->state.FP);
				font.Render(frameBuf,-1,FTPoint(0,0));

				//Draw bars
				glColor4f(0.7,0.7,0.7,0.7);
				glBegin(GL_LINES);
				for(int i=1;i<10;i++)
				{
					glVertex2i(0,(i*th->height/10));
					glVertex2i(th->width,(i*th->height/10));
				}
				glEnd();
				
				list<ThreadProfile>::iterator it=th->m_sys->profilingData.begin();
				for(;it!=th->m_sys->profilingData.end();it++)
					it->plot(1000000/sys->getFrameRate(),&font);
			}
			//Call glFlush to offload work on the GPU
			glFlush();
			glUseProgram(th->gpu_program);

			gdk_gl_drawable_gl_end(glDrawable);
			gdk_threads_leave();

			profile->accountTime(chronometer.checkpoint());
		}
	}
	catch(const char* e)
	{
		LOG(LOG_ERROR,"Exception caught " << e);
		::abort();
	}
	ret=gdk_gl_drawable_gl_begin(glDrawable,glContext);
	assert(ret);
	glDisable(GL_TEXTURE_2D);
	gdk_gl_drawable_gl_end(glDrawable);
	delete p;
	th->commonGLDeinit(t2);
	return NULL;
}
#endif

#ifdef COMPILE_PLUGIN
void* RenderThread::npapi_worker(RenderThread* th)
{
	sys=th->m_sys;
	rt=th;
	NPAPI_params* p=th->npapi_params;

	RECT size=sys->getFrameSize();
	int swf_width=size.Xmax/20;
	int swf_height=size.Ymax/20;

	int window_width=p->width;
	int window_height=p->height;

	float scalex=window_width;
	scalex/=swf_width;
	float scaley=window_height;
	scaley/=swf_height;

	rt->width=window_width;
	rt->height=window_height;
	th->interactive_buffer=new uint32_t[window_width*window_height];
	unsigned int t2[3];

	Display* d=XOpenDisplay(NULL);

	int a,b;
	Bool glx_present=glXQueryVersion(d,&a,&b);
	if(!glx_present)
	{
		LOG(LOG_ERROR,"glX not present");
		return NULL;
	}
	int attrib[10]={GLX_BUFFER_SIZE,24,GLX_DEPTH_SIZE,24,None};
	GLXFBConfig* fb=glXChooseFBConfig(d, 0, attrib, &a);
	if(!fb)
	{
		attrib[2]=None;
		fb=glXChooseFBConfig(d, 0, attrib, &a);
		LOG(LOG_ERROR,"Falling back to no depth buffer");
	}
	if(!fb)
	{
		LOG(LOG_ERROR,"Could not find any GLX configuration");
		::abort();
	}
	int i;
	for(i=0;i<a;i++)
	{
		int id;
		glXGetFBConfigAttrib(d,fb[i],GLX_VISUAL_ID,&id);
		if(id==(int)p->visual)
			break;
	}
	if(i==a)
	{
		//No suitable id found
		LOG(LOG_ERROR,"No suitable graphics configuration available");
		return NULL;
	}
	th->mFBConfig=fb[i];
	XFree(fb);

	th->mContext = glXCreateNewContext(d,th->mFBConfig,GLX_RGBA_TYPE ,NULL,1);
	glXMakeContextCurrent(d, p->window, p->window, th->mContext);
	if(!glXIsDirect(d,th->mContext))
		printf("Indirect!!\n");

	th->commonGLInit(window_width, window_height, t2);
	
	ThreadProfile* profile=sys->allocateProfiler(RGB(200,0,0));
	profile->setTag("Render");
	FTTextureFont font("/usr/share/fonts/truetype/ttf-liberation/LiberationSerif-Regular.ttf");
	if(font.Error())
		throw RunTimeException("Unable to load font");
	
	font.FaceSize(20);

	glEnable(GL_TEXTURE_2D);
	try
	{
		while(1)
		{
			sem_wait(&th->render);
			Chronometer chronometer;
			
			//Before starting rendering, cleanup all the request arrived in the meantime
			int fakeRenderCount=0;
			while(sem_trywait(&th->render)==0)
			{
				if(th->m_sys->isShuttingDown())
					pthread_exit(0);
				fakeRenderCount++;
			}
			
			if(fakeRenderCount)
				LOG(LOG_NO_INFO,"Faking " << fakeRenderCount << " renderings");
			if(th->m_sys->isShuttingDown())
				pthread_exit(0);

			if(th->m_sys->isOnError())
			{
				glUseProgram(0);

				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				glDrawBuffer(GL_BACK);
				glLoadIdentity();

				glClearColor(0,0,0,1);
				glClear(GL_COLOR_BUFFER_BIT);
				glColor3f(0.8,0.8,0.8);
					    
				font.Render("We're sorry, Lightspark encountered a yet unsupported Flash file",
					    -1,FTPoint(0,th->height/2));

				stringstream errorMsg;
				errorMsg << "SWF file: " << th->m_sys->getOrigin();
				font.Render(errorMsg.str().c_str(),
					    -1,FTPoint(0,th->height/2-20));
					    
				errorMsg.str("");
				errorMsg << "Cause: " << th->m_sys->errorCause;
				font.Render(errorMsg.str().c_str(),
					    -1,FTPoint(0,th->height/2-40));
				
				glFlush();
				glXSwapBuffers(d,p->window);
			}
			else
			{
				glXSwapBuffers(d,p->window);

				if(th->inputNeeded)
				{
					glBindTexture(GL_TEXTURE_2D,t2[2]);
					glGetTexImage(GL_TEXTURE_2D,0,GL_BGRA,GL_UNSIGNED_BYTE,th->interactive_buffer);
					th->inputNeeded=false;
					sem_post(&th->inputDone);
				}

				glBindFramebuffer(GL_FRAMEBUFFER, rt->fboId);
				glDrawBuffer(GL_COLOR_ATTACHMENT0);

				RGB bg=sys->getBackground();
				glClearColor(bg.Red/255.0F,bg.Green/255.0F,bg.Blue/255.0F,0);
				glClearDepth(0xffff);
				glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
				glLoadIdentity();
				glScalef(scalex,scaley,1);
				
				sys->Render();

				glFlush();

				glLoadIdentity();

				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				glDrawBuffer(GL_BACK);

				glClearColor(0,0,0,1);
				glClear(GL_COLOR_BUFFER_BIT);

				glBindTexture(GL_TEXTURE_2D,((sys->showInteractiveMap)?t2[2]:t2[0]));
				glColor4f(0,0,1,0);
				glBegin(GL_QUADS);
					glTexCoord2f(0,1);
					glVertex2i(0,0);
					glTexCoord2f(1,1);
					glVertex2i(th->width,0);
					glTexCoord2f(1,0);
					glVertex2i(th->width,th->height);
					glTexCoord2f(0,0);
					glVertex2i(0,th->height);
				glEnd();

				if(sys->showProfilingData)
				{
					glUseProgram(0);
					glDisable(GL_TEXTURE_2D);

					//Draw bars
					glColor4f(0.7,0.7,0.7,0.7);
					glBegin(GL_LINES);
					for(int i=1;i<10;i++)
					{
						glVertex2i(0,(i*th->height/10));
						glVertex2i(th->width,(i*th->height/10));
					}
					glEnd();
				
					list<ThreadProfile>::iterator it=sys->profilingData.begin();
					for(;it!=sys->profilingData.end();it++)
						it->plot(1000000/sys->getFrameRate(),&font);

					glEnable(GL_TEXTURE_2D);
					glUseProgram(rt->gpu_program);
				}
				//Call glFlush to offload work on the GPU
				glFlush();
			}
			profile->accountTime(chronometer.checkpoint());
		}
	}
	catch(const char* e)
	{
		LOG(LOG_ERROR,"Exception caught " << e);
		::abort();
	}
	glDisable(GL_TEXTURE_2D);
	th->commonGLDeinit(t2);
	glXMakeContextCurrent(d,None,None,NULL);
	glXDestroyContext(d,th->mContext);
	XCloseDisplay(d);
	delete p;
}
#endif

bool RenderThread::loadShaderPrograms()
{
	//Create render program
	assert(glCreateShader);
	GLuint f = glCreateShader(GL_FRAGMENT_SHADER);
	
	const char *fs = NULL;
	fs = dataFileRead(DATADIR "/lightspark.frag");
	if(fs==NULL)
		throw RunTimeException("Fragment shader code not found");
	assert(glShaderSource);
	glShaderSource(f, 1, &fs,NULL);
	free((void*)fs);

	bool ret=true;
	char str[1024];
	int a;
	assert(glCompileShader);
	glCompileShader(f);
	assert(glGetShaderInfoLog);
	glGetShaderInfoLog(f,1024,&a,str);
	LOG(LOG_NO_INFO,"Fragment shader compilation " << str);

	assert(glCreateProgram);
	gpu_program = glCreateProgram();
	assert(glAttachShader);
	glAttachShader(gpu_program,f);

	assert(glLinkProgram);
	glLinkProgram(gpu_program);
	assert(glGetProgramiv);
	glGetProgramiv(gpu_program,GL_LINK_STATUS,&a);
	if(a==GL_FALSE)
	{
		ret=false;
		return ret;
	}
	
	//Create the blitter shader
	GLuint v = glCreateShader(GL_VERTEX_SHADER);

	fs = dataFileRead(DATADIR "/lightspark.vert");
	if(fs==NULL)
		throw RunTimeException("Vertex shader code not found");
	glShaderSource(v, 1, &fs,NULL);
	free((void*)fs);

	glCompileShader(v);
	glGetShaderInfoLog(v,1024,&a,str);
	LOG(LOG_NO_INFO,"Vertex shader compilation " << str);

	blitter_program = glCreateProgram();
	glAttachShader(blitter_program,v);
	
	glLinkProgram(blitter_program);
	glGetProgramiv(blitter_program,GL_LINK_STATUS,&a);
	if(a==GL_FALSE)
	{
		ret=false;
		return ret;
	}

	assert(ret);
	return true;
}

#if 0
void* RenderThread::glx_worker(RenderThread* th)
{
	sys=th->m_sys;
	rt=th;

	RECT size=sys->getFrameSize();
	int width=size.Xmax/10;
	int height=size.Ymax/10;

	int attrib[20];
	attrib[0]=GLX_RGBA;
	attrib[1]=GLX_DOUBLEBUFFER;
	attrib[2]=GLX_DEPTH_SIZE;
	attrib[3]=24;
	attrib[4]=GLX_RED_SIZE;
	attrib[5]=8;
	attrib[6]=GLX_GREEN_SIZE;
	attrib[7]=8;
	attrib[8]=GLX_BLUE_SIZE;
	attrib[9]=8;
	attrib[10]=GLX_ALPHA_SIZE;
	attrib[11]=8;

	attrib[12]=None;

	XVisualInfo *vi;
	XSetWindowAttributes swa;
	Colormap cmap; 
	th->mDisplay = XOpenDisplay(0);
	vi = glXChooseVisual(th->mDisplay, DefaultScreen(th->mDisplay), attrib);

	int a;
	attrib[0]=GLX_VISUAL_ID;
	attrib[1]=vi->visualid;
	attrib[2]=GLX_DEPTH_SIZE;
	attrib[3]=24;
	attrib[4]=GLX_RED_SIZE;
	attrib[5]=8;
	attrib[6]=GLX_GREEN_SIZE;
	attrib[7]=8;
	attrib[8]=GLX_BLUE_SIZE;
	attrib[9]=8;
	attrib[10]=GLX_ALPHA_SIZE;
	attrib[11]=8;
	attrib[12]=GLX_DRAWABLE_TYPE;
	attrib[13]=GLX_PBUFFER_BIT;

	attrib[14]=None;
	GLXFBConfig* fb=glXChooseFBConfig(th->mDisplay, 0, attrib, &a);

	//We create a pair of context, window and offscreen
	th->mContext = glXCreateContext(th->mDisplay, vi, 0, GL_TRUE);

	attrib[0]=GLX_PBUFFER_WIDTH;
	attrib[1]=width;
	attrib[2]=GLX_PBUFFER_HEIGHT;
	attrib[3]=height;
	attrib[4]=None;
	th->mPbuffer = glXCreatePbuffer(th->mDisplay, fb[0], attrib);

	XFree(fb);

	cmap = XCreateColormap(th->mDisplay, RootWindow(th->mDisplay, vi->screen), vi->visual, AllocNone);
	swa.colormap = cmap; 
	swa.border_pixel = 0; 
	swa.event_mask = StructureNotifyMask; 

	th->mWindow = XCreateWindow(th->mDisplay, RootWindow(th->mDisplay, vi->screen), 100, 100, width, height, 0, vi->depth, 
			InputOutput, vi->visual, CWBorderPixel|CWEventMask|CWColormap, &swa);
	
	XMapWindow(th->mDisplay, th->mWindow);
	glXMakeContextCurrent(th->mDisplay, th->mWindow, th->mWindow, th->mContext); 

	glEnable( GL_DEPTH_TEST );
	glDepthFunc(GL_LEQUAL);

	glViewport(0,0,width,height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0,width,height,0,-100,0);
	glScalef(0.1,0.1,1);

	glMatrixMode(GL_MODELVIEW);

	unsigned int t;
	glGenTextures(1,&t);

	glBindTexture(GL_TEXTURE_1D,t);

	glTexParameteri(GL_TEXTURE_1D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_1D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_1D,GL_TEXTURE_WRAP_S,GL_CLAMP);

	//Load shaders
	rt->loadShaderPrograms();
	int tex=glGetUniformLocation(rt->gpu_program,"g_tex");
	glUniform1i(tex,0);
	glUseProgram(rt->gpu_program);

	float* buffer=new float[640*240];
	try
	{
		while(1)
		{
			sem_wait(&th->render);
			glXSwapBuffers(th->mDisplay,th->mWindow);
			RGB bg=sys->getBackground();
			glClearColor(bg.Red/255.0F,bg.Green/255.0F,bg.Blue/255.0F,0);
			glClearDepth(0xffff);
			glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
			glLoadIdentity();

			sys->Render();

			if(sys->isShuttingDown())
			{
				delete[] buffer;
				pthread_exit(0);
			}
		}
	}
	catch(const char* e)
	{
		LOG(LOG_ERROR, "Exception caught " << e);
		delete[] buffer;
		::abort();
	}
}
#endif

float RenderThread::getIdAt(int x, int y)
{
	//TODO: use floating point textures
	return (interactive_buffer[y*width+x]&0xff)/255.0f;
}

void RootMovieClip::initialize()
{
	if(!initialized && sys->currentVm)
	{
		initialized=true;
		//Let's see if we have to bind the root movie clip itself
		if(bindName.len())
			sys->currentVm->addEvent(NULL,new BindClassEvent(this,bindName));
		//Now signal the completion for this root
		sys->currentVm->addEvent(loaderInfo,Class<Event>::getInstanceS("init"));
		//Wait for handling of all previous events
		SynchronizationEvent* se=new SynchronizationEvent;
		bool added=sys->currentVm->addEvent(NULL, se);
		if(!added)
		{
			se->decRef();
			throw RunTimeException("Could not add event");
		}
		se->wait();
		se->decRef();
	}
}

bool RootMovieClip::getBounds(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const
{
	RECT f=getFrameSize();
	xmin=0;
	ymin=0;
	xmax=f.Xmax;
	ymax=f.Ymax;
	return true;
}

void RootMovieClip::Render()
{
	sem_wait(&sem_frames);
	while(1)
	{
		//Check if the next frame we are going to play is available
		if(state.next_FP<frames.size())
			break;

		sem_post(&sem_frames);
		sem_wait(&new_frame);
		if(parsingIsFailed)
			return;
		sem_wait(&sem_frames);
	}

	MovieClip::Render();
	sem_post(&sem_frames);
}

void RenderThread::commonGLDeinit(unsigned int t2[3])
{
	glDeleteTextures(1,&rt->data_tex);
	glDeleteTextures(3,t2);
	glBindFramebuffer(GL_FRAMEBUFFER,0);
	glDeleteFramebuffers(1,&rt->fboId);
}

void RenderThread::commonGLInit(int width, int height, unsigned int t2[3])
{
	//Now we can initialize GLEW
	glewExperimental = GL_TRUE;
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		LOG(LOG_ERROR,"Cannot initialize GLEW");
		cout << glewGetErrorString(err) << endl;
		::abort();
	}
	if(!GLEW_VERSION_2_0)
	{
		LOG(LOG_ERROR,"Video card does not support OpenGL 2.0... Aborting");
		::abort();
	}

	//Load shaders
	rt->loadShaderPrograms();

	int tex=glGetUniformLocation(rt->gpu_program,"g_tex1");
	glUniform1i(tex,0);

	glUseProgram(rt->gpu_program);

	glDisable(GL_DEPTH_TEST);
	glDepthFunc(GL_ALWAYS);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	
	glViewport(0,0,width,height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0,width,0,height,-100,0);

	glMatrixMode(GL_MODELVIEW);

	GLuint dt;
	glGenTextures(1,&dt);
	rt->data_tex=dt;

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D,dt);

	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
 	glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );

 	glGenTextures(3,t2);
	glBindTexture(GL_TEXTURE_2D,t2[0]);

	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
	glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );

	while(glGetError()!=GL_NO_ERROR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

	glBindTexture(GL_TEXTURE_2D,t2[1]);
	rt->spare_tex=t2[1];

	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
	glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

	glBindTexture(GL_TEXTURE_2D,t2[2]);

	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
	glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	
	// create a framebuffer object
	glGenFramebuffers(1, &rt->fboId);
	glBindFramebuffer(GL_FRAMEBUFFER, rt->fboId);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D, t2[0], 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,GL_TEXTURE_2D, t2[1], 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2,GL_TEXTURE_2D, t2[2], 0);
	
	// check FBO status
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if(status != GL_FRAMEBUFFER_COMPLETE)
	{
		LOG(LOG_ERROR,"Incomplete FBO status " << status << "... Aborting");
		while(err!=GL_NO_ERROR)
		{
			LOG(LOG_ERROR,"GL errors during initialization: " << err);
			err=glGetError();
		}
		::abort();
	}
}

void* RenderThread::sdl_worker(RenderThread* th)
{
	sys=th->m_sys;
	rt=th;
	RECT size=sys->getFrameSize();
	int width=size.Xmax/20;
	int height=size.Ymax/20;
	rt->width=width;
	rt->height=height;
	th->interactive_buffer=new uint32_t[width*height];
	unsigned int t2[3];
	SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 8 );
	SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 8 );
	SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 8 );
	SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 24 );
	SDL_GL_SetAttribute( SDL_GL_SWAP_CONTROL, 0 );
	SDL_GL_SetAttribute( SDL_GL_ACCELERATED_VISUAL, 1); 
	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

	SDL_SetVideoMode( width, height, 24, SDL_OPENGL );
	th->commonGLInit(width, height, t2);

	ThreadProfile* profile=sys->allocateProfiler(RGB(200,0,0));
	profile->setTag("Render");
	FTTextureFont font("/usr/share/fonts/truetype/ttf-liberation/LiberationSerif-Regular.ttf");
	if(font.Error())
		throw RunTimeException("Unable to load font");
	
	font.FaceSize(20);
	try
	{
		//Texturing must be enabled otherwise no tex coord will be sent to the shader
		glEnable(GL_TEXTURE_2D);
		Chronometer chronometer;
		while(1)
		{
			sem_wait(&th->render);
			chronometer.checkpoint();

			SDL_GL_SwapBuffers( );

			if(th->inputNeeded)
			{
				glBindTexture(GL_TEXTURE_2D,t2[2]);
				glGetTexImage(GL_TEXTURE_2D,0,GL_BGRA,GL_UNSIGNED_BYTE,th->interactive_buffer);
				th->inputNeeded=false;
				sem_post(&th->inputDone);
			}

			//Before starting rendering, cleanup all the request arrived in the meantime
			int fakeRenderCount=0;
			while(sem_trywait(&th->render)==0)
			{
				if(th->m_sys->isShuttingDown())
					pthread_exit(0);
				fakeRenderCount++;
			}

			if(fakeRenderCount)
				LOG(LOG_NO_INFO,"Faking " << fakeRenderCount << " renderings");

			if(th->m_sys->isShuttingDown())
				pthread_exit(0);
			SDL_PumpEvents();

			if(th->m_sys->isOnError())
			{
				glUseProgram(0);

				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				glDrawBuffer(GL_BACK);
				glLoadIdentity();

				glClearColor(0,0,0,1);
				glClear(GL_COLOR_BUFFER_BIT);
				glColor3f(0.8,0.8,0.8);
					    
				font.Render("We're sorry, Lightspark encountered a yet unsupported Flash file",
						-1,FTPoint(0,th->height/2));

				stringstream errorMsg;
				errorMsg << "SWF file: " << th->m_sys->getOrigin();
				font.Render(errorMsg.str().c_str(),
						-1,FTPoint(0,th->height/2-20));
					    
				errorMsg.str("");
				errorMsg << "Cause: " << th->m_sys->errorCause;
				font.Render(errorMsg.str().c_str(),
						-1,FTPoint(0,th->height/2-40));

				font.Render("Press 'Q' to exit",-1,FTPoint(0,th->height/2-60));
				
				glFlush();
				SDL_GL_SwapBuffers( );
			}
			else
			{
				glBindFramebuffer(GL_FRAMEBUFFER, rt->fboId);
				
				//Clear the id buffer
				glDrawBuffer(GL_COLOR_ATTACHMENT2);
				glClearColor(0,0,0,0);
				glClear(GL_COLOR_BUFFER_BIT);
				
				//Clear the back buffer
				glDrawBuffer(GL_COLOR_ATTACHMENT0);
				RGB bg=sys->getBackground();
				glClearColor(bg.Red/255.0F,bg.Green/255.0F,bg.Blue/255.0F,1);
				glClear(GL_COLOR_BUFFER_BIT);
				
				glLoadIdentity();
				glTranslatef(th->m_sys->xOffset,th->m_sys->yOffset,0);
				
				th->m_sys->Render();

				glFlush();

				glLoadIdentity();

				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				glDrawBuffer(GL_BACK);
				glUseProgram(0);
				glDisable(GL_BLEND);

				glBindTexture(GL_TEXTURE_2D,((sys->showInteractiveMap)?t2[2]:t2[0]));
				glBegin(GL_QUADS);
					glTexCoord2f(0,1);
					glVertex2i(0,0);
					glTexCoord2f(1,1);
					glVertex2i(width,0);
					glTexCoord2f(1,0);
					glVertex2i(width,height);
					glTexCoord2f(0,0);
					glVertex2i(0,height);
				glEnd();
				
				if(th->m_sys->showDebug)
				{
					glDisable(GL_TEXTURE_2D);
					if(th->selectedDebug)
						th->selectedDebug->debugRender(&font, true);
					else
						th->m_sys->debugRender(&font, true);
					glEnable(GL_TEXTURE_2D);
				}

				if(th->m_sys->showProfilingData)
				{
					glColor3f(0,0,0);
					char frameBuf[20];
					snprintf(frameBuf,20,"Frame %u",th->m_sys->state.FP);
					font.Render(frameBuf,-1,FTPoint(0,0));

					//Draw bars
					glColor4f(0.7,0.7,0.7,0.7);
					glBegin(GL_LINES);
					for(int i=1;i<10;i++)
					{
						glVertex2i(0,(i*height/10));
						glVertex2i(width,(i*height/10));
					}
					glEnd();
					
					list<ThreadProfile>::iterator it=th->m_sys->profilingData.begin();
					for(;it!=th->m_sys->profilingData.end();it++)
						it->plot(1000000/sys->getFrameRate(),&font);
				}
				//Call glFlush to offload work on the GPU
				glFlush();
				glUseProgram(th->gpu_program);
				glEnable(GL_BLEND);
			}
			profile->accountTime(chronometer.checkpoint());
		}
		glDisable(GL_TEXTURE_2D);
	}
	catch(const char* e)
	{
		LOG(LOG_ERROR, "Exception caught " << e);
		::abort();
	}
	th->commonGLDeinit(t2);
	return NULL;
}

void RenderThread::draw()
{
	sem_post(&render);
#ifdef CLOCK_REALTIME
	clock_gettime(CLOCK_REALTIME,&td);
	uint32_t diff=timeDiff(ts,td);
	if(diff>1000)
	{
		ts=td;
		LOG(LOG_NO_INFO,"FPS: " << dec << frameCount);
		frameCount=0;
		secsCount++;
	}
	else
		frameCount++;
#endif
}

void RenderThread::tick()
{
	draw();
}

void RootMovieClip::setFrameCount(int f)
{
	sem_wait(&sem_frames);
	totalFrames=f;
	state.max_FP=f;
	assert(cur_frame==&frames.back());
	//Reserving guarantees than the vector is never invalidated
	frames.reserve(f);
	cur_frame=&frames.back();
	sem_post(&sem_frames);
}

void RootMovieClip::setFrameSize(const lightspark::RECT& f)
{
	frameSize=f;
	assert(f.Xmin==0 && f.Ymin==0);
	sem_post(&sem_valid_size);
}

lightspark::RECT RootMovieClip::getFrameSize() const
{
	//This is a sync semaphore the first time and then a mutex
	sem_wait(&sem_valid_size);
	lightspark::RECT ret=frameSize;
	sem_post(&sem_valid_size);
	return ret;
}

void RootMovieClip::setFrameRate(float f)
{
	frameRate=f;
	//Now frame rate is valid, start the rendering

	sys->addTick(1000/f,this);
	sem_post(&sem_valid_rate);
}

float RootMovieClip::getFrameRate() const
{
	//This is a sync semaphore the first time and then a mutex
	sem_wait(&sem_valid_rate);
	float ret=frameRate;
	sem_post(&sem_valid_rate);
	return ret;
}

void RootMovieClip::addToDictionary(DictionaryTag* r)
{
	sem_wait(&mutex);
	dictionary.push_back(r);
	sem_post(&mutex);
}

void RootMovieClip::addToFrame(DisplayListTag* t)
{
	sem_wait(&mutex);
	MovieClip::addToFrame(t);
	sem_post(&mutex);
}

void RootMovieClip::addToFrame(ControlTag* t)
{
	cur_frame->controls.push_back(t);
}

void RootMovieClip::commitFrame(bool another)
{
	sem_wait(&sem_frames);
	framesLoaded=frames.size();
	if(another)
	{
		frames.push_back(Frame());
		cur_frame=&frames.back();
	}
	else
		cur_frame=NULL;
	
	assert(frames.size()<=frames.capacity());

	if(framesLoaded==1)
	{
		//Let's initialize the first frame of this movieclip
		bootstrap();
	}
	sem_post(&new_frame);
	sem_post(&sem_frames);
}

void RootMovieClip::revertFrame()
{
	sem_wait(&sem_frames);
	assert(frames.size() && framesLoaded==(frames.size()-1));
	frames.pop_back();
	cur_frame=NULL;
	sem_post(&sem_frames);
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
	sem_wait(&mutex);
	list< DictionaryTag*>::iterator it = dictionary.begin();
	for(;it!=dictionary.end();it++)
	{
		if((*it)->getId()==id)
			break;
	}
	if(it==dictionary.end())
	{
		LOG(LOG_ERROR,"No such Id on dictionary " << id);
		sem_post(&mutex);
		throw RunTimeException("Could not find an object on the dictionary");
	}
	DictionaryTag* ret=*it;
	sem_post(&mutex);
	return ret;
}

void RootMovieClip::tick()
{
	//Should go to the next frame
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

long lightspark::timeDiff(timespec& s, timespec& d)
{
	timespec temp;
	if ((d.tv_nsec-s.tv_nsec)<0) {
		temp.tv_sec = d.tv_sec-s.tv_sec-1;
		temp.tv_nsec = 1000000000+d.tv_nsec-s.tv_nsec;
	} else {
		temp.tv_sec = d.tv_sec-s.tv_sec;
		temp.tv_nsec = d.tv_nsec-s.tv_nsec;
	}
	return temp.tv_sec*1000+(temp.tv_nsec)/1000000;
}

