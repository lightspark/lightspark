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

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS
#include <string.h>
#include <pthread.h>
#include <SDL.h>
#include <algorithm>

#include "abc.h"
#include "flashdisplay.h"
#include "flashevents.h"
#include "swf.h"
#include "logger.h"
#include "actions.h"
#include "streams.h"
#include "asobjects.h"
#include "textfile.h"
#include "compat.h"
#include "class.h"

#include <GL/glew.h>
#ifndef WIN32
//#include <GL/glext.h>
#include <GL/glx.h>
#endif

using namespace std;
using namespace lightspark;

int ParseThread::error(0);

list<IDisplayListElem*> null_list;
int RenderThread::error(0);

extern TLSDATA SystemState* sys;
extern TLSDATA RenderThread* rt;
extern TLSDATA ParseThread* pt;

SWF_HEADER::SWF_HEADER(istream& in)
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
		abort();
	}
	pt->version=Version;
	in >> FrameSize >> FrameRate >> FrameCount;
	LOG(LOG_NO_INFO,"FrameRate " << FrameRate);
	pt->root->frame_rate=FrameRate;
	pt->root->frame_rate/=256;
}

RootMovieClip::RootMovieClip()
{
	root=this;
	sem_init(&mutex,0,1);
	sem_init(&sem_frames,0,1);
	sem_init(&sem_valid_frame_size,0,0);
}

SystemState::SystemState():shutdown(false),currentVm(NULL),cur_thread_pool(NULL)
{
	sys=this;
	sem_init(&new_frame,0,0);
	loaderInfo=Class<LoaderInfo>::getInstanceS(true);
	stage=Class<Stage>::getInstanceS(true);
	startTime=compat_msectiming();
}

void SystemState::setUrl(const tiny_string& url)
{
	loaderInfo->url=url;
	loaderInfo->loaderURL=url;
}

SystemState::~SystemState()
{
	if(currentVm)
		delete currentVm;
	if(cur_thread_pool)
		delete cur_thread_pool;
	obj->implementation=NULL;
	delete obj;
}

void SystemState::setShutdownFlag()
{
	sem_wait(&mutex);
	shutdown=true;
	if(sys->currentVm)
		sys->currentVm->addEvent(NULL,new ShutdownEvent());

	sem_post(&mutex);
}

void* ParseThread::worker(ParseThread* th)
{
	sys=th->m_sys;
	pt=th;
	try
	{
		SWF_HEADER h(th->f);
		pt->root->setFrameSize(h.getFrameSize());
		pt->root->setFrameCount(h.FrameCount);

		int done=0;

		TagFactory factory(th->f);
		while(1)
		{
			if(error)
			{
				LOG(LOG_NO_INFO,"Terminating parsing thread on error state");
				pthread_exit(NULL);
			}
			Tag* tag=factory.readTag();
			switch(tag->getType())
			{
			//	case TAG:
				case END_TAG:
				{
					LOG(LOG_NO_INFO,"End of parsing @ " << th->f.tellg());
					th->root->commitFrame();
					//Wait for handling of all previous events
					SynchronizationEvent* sync=new SynchronizationEvent;
					sys->currentVm->addEvent(NULL, sync);
					sync->wait();
					//Now signal the completion for this root
					sys->currentVm->addEvent(th->root->loaderInfo,Class<Event>::getInstanceS(true,"init"));
					pthread_exit(NULL);
				}
				case DICT_TAG:
					th->root->addToDictionary(static_cast<DictionaryTag*>(tag));
					break;
				case DISPLAY_LIST_TAG:
					th->root->addToFrame(static_cast<DisplayListTag*>(tag));
					break;
				case SHOW_TAG:
				{
					th->root->commitFrame();
					break;
				}
				case CONTROL_TAG:
					static_cast<ControlTag*>(tag)->execute();
					break;
			}
			if(sys->shutdown)
				pthread_exit(0);
		}
	}
	catch(const char* s)
	{
		LOG(LOG_ERROR,"Exception caught: " << s);
		exit(-1);
	}
}

ParseThread::ParseThread(SystemState* s,RootMovieClip* r,istream& in):f(in),
	parsingDisplayList(&r->displayList),parsingTarget(r)
{
	m_sys=s;
	root=r;
	error=0;
	pthread_create(&t,NULL,(thread_worker)worker,this);
}

ParseThread::~ParseThread()
{
	void* ret;
	pthread_cancel(t);
	pthread_join(t,&ret);
}

void RenderThread::wait()
{
	pthread_cancel(t);
	pthread_join(t,NULL);
}

void ParseThread::wait()
{
	pthread_join(t,NULL);
}

InputThread::InputThread(SystemState* s,ENGINE e, void* param)
{
	m_sys=s;
	LOG(LOG_NO_INFO,"Creating input thread");
	sem_init(&sem_listeners,0,1);
	if(e==SDL)
		pthread_create(&t,NULL,(thread_worker)sdl_worker,this);
#ifndef WIN32
	else
	{
		npapi_params=(NPAPI_params*)param;
		pthread_create(&t,NULL,(thread_worker)npapi_worker,this);
	}
#endif
}

InputThread::~InputThread()
{
	void* ret;
	pthread_cancel(t);
	pthread_join(t,&ret);
}

void InputThread::wait()
{
	pthread_join(t,NULL);
}

#ifndef WIN32
void* InputThread::npapi_worker(InputThread* th)
{
/*	sys=th->m_sys;
	NPAPI_params* p=(NPAPI_params*)in_ptr;
//	Display* d=XOpenDisplay(NULL);
	XSelectInput(p->display,p->window,PointerMotionMask|ExposureMask);

	XEvent e;
	while(XWindowEvent(p->display,p->window,PointerMotionMask|ExposureMask, &e))
	{
		exit(-1);
	}*/
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
					case SDLK_q:
						sys->setShutdownFlag();
						pthread_exit(0);
						break;
					case SDLK_s:
						sys->state.stop_FP=true;
						break;
				}
				break;
			}
			case SDL_MOUSEBUTTONDOWN:
			{

				float selected=sys->cur_render_thread->getIdAt(event.button.x,event.button.y);
				cout << "mouse" << endl;
				if(selected==0)
					break;

				sem_wait(&th->sem_listeners);

				selected--;
				int index=int(th->listeners.count("")*selected);
				
				pair< multimap<tiny_string, EventDispatcher*>::const_iterator,
					multimap<tiny_string, EventDispatcher*>::const_iterator > range=
					th->listeners.equal_range("");

				//Get the selected item
				multimap<tiny_string, EventDispatcher*>::const_iterator it=range.first;
				while(index)
				{
					it++;
					index--;
				}

				//Add event to the event queue
				sys->currentVm->addEvent(it->second,new Event("mouseDown"));

				sem_post(&th->sem_listeners);
				break;
			}
		}
	}
	return NULL;
}

void InputThread::addListener(const tiny_string& t, EventDispatcher* ob)
{
	sem_wait(&sem_listeners);
	LOG(LOG_TRACE,"Adding listener to " << t);

	//the empty string is the *any* event
	pair< multimap<tiny_string, EventDispatcher*>::const_iterator, 
		multimap<tiny_string, EventDispatcher*>::const_iterator > range=
		listeners.equal_range("");


	bool already_known=false;

	multimap<tiny_string, EventDispatcher*>::const_iterator it=range.first;
	int count=0;
	for(it;it!=range.second;it++)
	{
		count++;
		if(it->second==ob)
		{
			already_known=true;
			break;
		}
	}
	range=listeners.equal_range(t);

	if(already_known)
	{
		//Check if this object is alreasy registered for this event
		it=range.first;
		int count=0;
		for(it;it!=range.second;it++)
		{
			count++;
			if(it->second==ob)
			{
				LOG(LOG_TRACE,"Already added");
				sem_post(&sem_listeners);
				return;
			}
		}
	}
	else
		listeners.insert(make_pair("",ob));

	//Register the listener
	listeners.insert(make_pair(t,ob));
	count++;

	range=listeners.equal_range("");
	it=range.first;
	//Set a unique id for listeners in the range [0,1]
	//count is the number of listeners, this is correct so that no one gets 0
	float increment=1.0f/count;
	float cur=increment;
	cout << "increment " << increment << endl;
	for(it;it!=range.second;it++)
	{
		if(it->second)
			it->second->setId(cur);
		cout << "setting to " << cur << endl;
		cur+=increment;
	}

	sem_post(&sem_listeners);
}

void InputThread::broadcastEvent(const tiny_string& t)
{
	sem_wait(&sem_listeners);

	pair< multimap<tiny_string,EventDispatcher*>::const_iterator, 
		multimap<tiny_string, EventDispatcher*>::const_iterator > range=
		listeners.equal_range(t);

	for(range.first;range.first!=range.second;range.first++)
		sys->currentVm->addEvent(range.first->second,Class<Event>::getInstanceS(true,t));

	sem_post(&sem_listeners);
}

RenderThread::RenderThread(SystemState* s,ENGINE e,void* params):interactive_buffer(NULL),currentClip(s)
{
	LOG(LOG_NO_INFO,"RenderThread this=" << this);
	m_sys=s;
	sem_init(&mutex,0,1);
	sem_init(&render,0,0);
	sem_init(&end_render,0,0);
	error=0;
	if(e==SDL)
		pthread_create(&t,NULL,(thread_worker)sdl_worker,this);
#ifndef WIN32
	else if(e==NPAPI)
	{
		npapi_params=(NPAPI_params*)params;
		pthread_create(&t,NULL,(thread_worker)npapi_worker,this);
	}
	else if(e==GLX)
	{
		pthread_create(&t,NULL,(thread_worker)glx_worker,this);
	}
#endif
}

RenderThread::~RenderThread()
{
	void* ret;
	pthread_cancel(t);
	pthread_join(t,&ret);
	LOG(LOG_NO_INFO,"~RenderThread this=" << this);
}

#ifndef WIN32
void* RenderThread::npapi_worker(RenderThread* th)
{
	sys=th->m_sys;
	rt=th;

	NPAPI_params* p=th->npapi_params;

	Display* d=XOpenDisplay(NULL);

	XFontStruct *mFontInfo;
	if (!mFontInfo)
	{
		if (!(mFontInfo = XLoadQueryFont(d, "9x15")))
			printf("Cannot open 9X15 font\n");
	}

	XGCValues v;
	v.foreground=BlackPixel(d, 0);
	v.background=WhitePixel(d, 0);
	v.font=XLoadFont(d,"9x15");
	th->mGC=XCreateGC(d,p->window,GCForeground|GCBackground|GCFont,&v);

    	int a,b;
    	Bool glx_present=glXQueryVersion(d,&a,&b);
	if(!glx_present)
	{
		printf("glX not present\n");
		return NULL;
	}
	int attrib[10];
	attrib[0]=GLX_BUFFER_SIZE;
	attrib[1]=24;
	attrib[2]=GLX_VISUAL_ID;
	attrib[3]=p->visual;
	attrib[4]=GLX_DEPTH_SIZE;
	attrib[5]=24;

	attrib[6]=None;
	GLXFBConfig* fb=glXChooseFBConfig(d, 0, attrib, &a);
//	printf("returned %x pointer and %u elements\n",fb, a);
	if(!fb)
	{
		attrib[0]=0;
		fb=glXChooseFBConfig(d, 0, NULL, &a);
		LOG(LOG_ERROR,"Falling back to no depth and no stencil");
	}
	int i;
	for(i=0;i<a;i++)
	{
		int id,v;
		glXGetFBConfigAttrib(d,fb[i],GLX_BUFFER_SIZE,&v);
		glXGetFBConfigAttrib(d,fb[i],GLX_VISUAL_ID,&id);
//		printf("ID 0x%x size %u\n",id,v);
		if(id==p->visual)
		{
//			printf("good id %x\n",id);
			break;
		}
	}
	th->mFBConfig=fb[i];
	XFree(fb);

	th->mContext = glXCreateNewContext(d,th->mFBConfig,GLX_RGBA_TYPE ,NULL,1);
	glXMakeContextCurrent(d, p->window, p->window, th->mContext);
	if(!glXIsDirect(d,th->mContext))
		printf("Indirect!!\n");

	//Load fragment shaders
	rt->gpu_program=load_program();

	int tex=glGetUniformLocation(rt->gpu_program,"g_tex1");
	glUniform1i(tex,0);

	glUseProgram(rt->gpu_program);

	glDisable(GL_DEPTH_TEST);
	glDepthFunc(GL_ALWAYS);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	RECT size=sys->getFrameSize();
	int width=p->width;
	int height=p->height;
	rt->width=width;
	rt->height=height;
	float scalex=p->width;
	scalex/=size.Xmax;
	float scaley=p->height;
	scaley/=size.Ymax;
	glViewport(0,0,width,height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0,width,height,0,-100,0);

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

	unsigned int t2[3];
 	glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );
	glGenTextures(3,t2);
	glBindTexture(GL_TEXTURE_2D,t2[0]);

	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

	glBindTexture(GL_TEXTURE_2D,t2[1]);
	rt->spare_tex=t2[1];

	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

	glBindTexture(GL_TEXTURE_2D,t2[2]);

	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	/*// create a renderbuffer object to store depth info
	GLuint rboId[1];
	glGenRenderbuffersEXT(1, rboId);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, rboId[0]);
	glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT,width,height);

	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);*/
	
	// create a framebuffer object
	glGenFramebuffersEXT(1, &rt->fboId);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, rt->fboId);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,GL_TEXTURE_2D, t2[0], 0);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT1_EXT,GL_TEXTURE_2D, t2[1], 0);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT2_EXT,GL_TEXTURE_2D, t2[2], 0);
	
	// check FBO status
	GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	if(status != GL_FRAMEBUFFER_COMPLETE_EXT)
	{
		cout << status << endl;
		abort();
	}

	try
	{
		while(1)
		{
			sem_wait(&th->render);
			if(error)
			{
				glXMakeContextCurrent(d, 0, 0, NULL);
				unsigned int h = p->height/2;
				unsigned int w = 3 * p->width/4;
				int x = 0;
				int y = h/2;
				GC gc = XCreateGC(d, p->window, 0, NULL);
				const char *string = "ERROR";
				int l = strlen(string);
				int fmba = mFontInfo->max_bounds.ascent;
				int fmbd = mFontInfo->max_bounds.descent;
				int fh = fmba + fmbd;
				y += fh;
				x += 32;
				XDrawString(d, p->window, gc, x, y, string, l);
				XFreeGC(d, gc);
			}
			else
			{
				sem_wait(&th->mutex);

				glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, rt->fboId);
				glReadBuffer(GL_COLOR_ATTACHMENT2_EXT);
				//glReadPixels(0,0,width,height,GL_RED,GL_FLOAT,th->interactive_buffer);

				glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);

				RGB bg=sys->getBackground();
				glClearColor(bg.Red/255.0F,bg.Green/255.0F,bg.Blue/255.0F,0);
				glClearDepth(0xffff);
				glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
				glLoadIdentity();
				glScalef(scalex,scaley,1);
				
				sys->Render();

				glLoadIdentity();

				glColor3f(0,0,1);
				glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
				glDrawBuffer(GL_BACK);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				glBindTexture(GL_TEXTURE_2D,t2[0]);
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

				glXSwapBuffers(d,p->window);
				sem_post(&th->mutex);
			}
			sem_post(&th->end_render);
			if(sys->shutdown)
				pthread_exit(0);
		}
	}
	catch(const char* e)
	{
		LOG(LOG_ERROR,"Exception caught " << e);
		exit(-1);
	}
	delete p;
}
#endif

int RenderThread::load_program()
{
	GLuint f = glCreateShader(GL_FRAGMENT_SHADER);
	GLuint v = glCreateShader(GL_VERTEX_SHADER);

	const char *fs = NULL;
	fs = dataFileRead("lightspark.frag");
	//fs = dataFileRead("/home/alex/lightspark/lightspark.frag");
	glShaderSource(f, 1, &fs,NULL);
	free((void*)fs);

/*	fs = dataFileRead("lightspark.vert");
//	fs = dataFileRead("/home/alex/lightspark/lightspark.vert");
	glShaderSource(v, 1, &fs,NULL);
	free((void*)fs);*/

	char str[1024];
	int a;
	glCompileShader(f);
	glGetShaderInfoLog(f,1024,&a,str);
	printf("Fragment shader: %s\n",str);

/*	glCompileShader(v);
	glGetShaderInfoLog(v,1024,&a,str);
	printf("Vertex shader: %s\n",str);*/

	int ret = glCreateProgram();
	glAttachShader(ret,f);

	glLinkProgram(ret);
	return ret;
}

#ifndef WIN32
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
	cout << fb << endl;
	cout << a  << endl;

	//We create a pair of context, window and offscreen
	th->mContext = glXCreateContext(th->mDisplay, vi, 0, GL_TRUE);

	attrib[0]=GLX_PBUFFER_WIDTH;
	attrib[1]=width;
	attrib[2]=GLX_PBUFFER_HEIGHT;
	attrib[3]=height;
	attrib[4]=None;
	th->mPbuffer = glXCreatePbuffer(th->mDisplay, fb[0], attrib);
	cout << th->mPbuffer << endl;

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

	//Load fragment shaders
	rt->gpu_program=load_program();
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

			sem_post(&th->end_render);
			if(sys->shutdown)
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
		exit(-1);
	}
}
#endif

float RenderThread::getIdAt(int x, int y)
{
	return interactive_buffer[y*width+x];
}

void RootMovieClip::Render()
{
	sem_wait(&sem_frames);
	while(1)
	{
		if(state.FP<frames.size())
			break;

		sem_post(&sem_frames);
		sem_wait(&new_frame);
		sem_wait(&sem_frames);
	}

	//We stop execution until execution engine catches up
	if(sys->currentVm)
	{
		SynchronizationEvent* s=new SynchronizationEvent;
		sys->currentVm->addEvent(NULL, s);
		s->wait();
		delete s;
	}
	//Now the bindings are effective

	MovieClip::Render();
	sem_post(&sem_frames);
}

void* RenderThread::sdl_worker(RenderThread* th)
{
	sys=th->m_sys;
	rt=th;
	SDL_Init(SDL_INIT_VIDEO);
	SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 8 );
	SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 8 );
	SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 8 );
	SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 24 );
	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute( SDL_GL_SWAP_CONTROL, 0 );
	SDL_GL_SetAttribute( SDL_GL_ACCELERATED_VISUAL, 1); 
	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

	RECT size=sys->getFrameSize();
	int width=size.Xmax/20;
	int height=size.Ymax/20;
	rt->width=width;
	rt->height=height;
	SDL_SetVideoMode( width, height, 24, SDL_OPENGL );
	//Now we can initialize GLEW
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		LOG(LOG_ERROR,"Cannot initialize GLEW");
		abort();
	}
	th->interactive_buffer=new float[width*height];

	//Load fragment shaders
	rt->gpu_program=load_program();

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
	glOrtho(0,width,height,0,-100,0);


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

	unsigned int t2[3];
 	glGenTextures(3,t2);
	glBindTexture(GL_TEXTURE_2D,t2[0]);
	//glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );

	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

	glBindTexture(GL_TEXTURE_2D,t2[1]);
	rt->spare_tex=t2[1];

	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

	glBindTexture(GL_TEXTURE_2D,t2[2]);

	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	
	// create a framebuffer object
	glGenFramebuffersEXT(1, &rt->fboId);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, rt->fboId);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,GL_TEXTURE_2D, t2[0], 0);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT1_EXT,GL_TEXTURE_2D, t2[1], 0);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT2_EXT,GL_TEXTURE_2D, t2[2], 0);
	
	// check FBO status
	GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	if(status != GL_FRAMEBUFFER_COMPLETE_EXT)
	{
		cout << status << endl;
		abort();
	}

	try
	{
		//Texturing must be enabled otherwise no tex coord will be sent to the shader
		glEnable(GL_TEXTURE_2D);
		while(1)
		{
			sem_wait(&th->render);
			SDL_PumpEvents();
			SDL_GL_SwapBuffers( );
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, rt->fboId);
			//glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
			//glReadPixels(0,0,width,height,GL_RED,GL_FLOAT,th->interactive_buffer);

			glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);

			RGB bg=sys->getBackground();
			glClearColor(bg.Red/255.0F,bg.Green/255.0F,bg.Blue/255.0F,1);
			glClear(GL_COLOR_BUFFER_BIT);
			glLoadIdentity();
			//glScalef(0.05,0.05,1);
			
			sys->Render();

			glLoadIdentity();

			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
			glDrawBuffer(GL_BACK);

			glClearColor(0,0,0,1);
			glClear(GL_COLOR_BUFFER_BIT);

			glBindTexture(GL_TEXTURE_2D,t2[0]);
			glColor4f(0,0,1,0);
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
			
			sem_post(&th->end_render);
			if(sys->shutdown)
				pthread_exit(0);
		}
		glDisable(GL_TEXTURE_2D);
		//glUseProgram(rt->gpu_program);
	}
	catch(const char* e)
	{
		LOG(LOG_ERROR, "Exception caught " << e);
		exit(-1);
	}
	return NULL;
}

void RenderThread::draw()
{
	sys->cur_input_thread->broadcastEvent("enterFrame");
	sem_post(&render);
	compat_msleep(1000/sys->root->frame_rate);
	sem_wait(&end_render);
}

void RootMovieClip::setFrameCount(int f)
{
	sem_wait(&sem_frames);
	totalFrames=f;
	state.max_FP=f;
	frames.reserve(f);
	sem_post(&sem_frames);
}

void RootMovieClip::setFrameSize(const lightspark::RECT& f)
{
	frame_size=f;
	//Also set the width and height properties
	assert(f.Xmin==0 && f.Ymin==0);
	width=f.Xmax;
	height=f.Ymax;
	sem_post(&sem_valid_frame_size);
}

lightspark::RECT RootMovieClip::getFrameSize()
{
	//This is a sync semaphore the first time and then a mutex
	sem_wait(&sem_valid_frame_size);
	lightspark::RECT ret=frame_size;
	sem_post(&sem_valid_frame_size);
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

void RootMovieClip::commitFrame()
{
	sem_wait(&sem_frames);
	frames.push_back(cur_frame);
	cur_frame=Frame(&dynamicDisplayList);
	framesLoaded=frames.size();
	sem_post(&new_frame);
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
	for(it;it!=dictionary.end();it++)
	{
		if((*it)->getId()==id)
			break;
	}
	if(it==dictionary.end())
	{
		LOG(LOG_ERROR,"No such Id on dictionary " << id);
		abort();
	}
	DictionaryTag* ret=*it;
	sem_post(&mutex);
	return ret;
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

ASFUNCTIONBODY(SystemState,getBounds)
{
	return new Undefined;
}
