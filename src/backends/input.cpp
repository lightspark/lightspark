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

#include "scripting/abc.h"
#include "audio.h"
#include "input.h"
#include "swf.h"
#include "rendering.h"
#include "compat.h"

#if GTK_CHECK_VERSION (2,21,8)
#include <gdk/gdkkeysyms-compat.h> 
#else
#include <gdk/gdkkeysyms.h>
#endif 

using namespace lightspark;
using namespace std;

InputThread::InputThread(SystemState* s):m_sys(s),terminated(false),threaded(false),
	mutexListeners("Input listeners"),mutexDragged("Input dragged"),curDragged(NULL),currentMouseOver(NULL),lastMouseDownTarget(NULL),
	dragLimit(NULL)
{
	LOG(LOG_INFO,_("Creating input thread"));
}

void InputThread::start(const EngineData* e)
{
	GtkWidget* container=e->container;
	gtk_widget_set_can_focus(container,True);
	gtk_widget_add_events(container,GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK |
					GDK_POINTER_MOTION_MASK | GDK_EXPOSURE_MASK);
	g_signal_connect(G_OBJECT(container), "event", G_CALLBACK(worker), this);
}

InputThread::~InputThread()
{
	wait();
}

void InputThread::wait()
{
	if(terminated)
		return;
	if(threaded)
		pthread_join(t,NULL);
	terminated=true;
}

//This is a GTK event handler and the gdk lock is already acquired
gboolean InputThread::worker(GtkWidget *widget, GdkEvent *event, InputThread* th)
{
	//Set sys to this SystemState
	sys=th->m_sys;
	gboolean ret=FALSE;
	switch(event->type)
	{
		case GDK_KEY_PRESS:
		{
			//cout << "key press" << endl;
			switch(event->key.keyval)
			{
				case GDK_q:
					if(th->m_sys->standalone)
					{
						th->m_sys->setShutdownFlag();
						gtk_main_quit();
					}
					break;
				case GDK_p:
					th->m_sys->showProfilingData=!th->m_sys->showProfilingData;
					break;
				case GDK_m:
					if (!th->m_sys->audioManager->pluginLoaded())
						break;
					th->m_sys->audioManager->toggleMuteAll();
					if(th->m_sys->audioManager->allMuted())
						LOG(LOG_INFO, "All sounds muted");
					else
						LOG(LOG_INFO, "All sounds unmuted");
					break;
				case GDK_c:
					if(th->m_sys->hasError())
					{
						GtkClipboard *clipboard;
						clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
						gtk_clipboard_set_text(clipboard, th->m_sys->getErrorCause().c_str(),
								th->m_sys->getErrorCause().size());
						clipboard = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
						gtk_clipboard_set_text(clipboard, th->m_sys->getErrorCause().c_str(),
								th->m_sys->getErrorCause().size());
						LOG(LOG_INFO, "Copied error to clipboard");
					}
					else
						LOG(LOG_INFO, "No error to be coppied to clipboard");
					break;
				default:
					break;
			}
			ret=TRUE;
			break;
		}
		case GDK_EXPOSE:
		{
			//Signal the renderThread
			th->m_sys->getRenderThread()->draw(false);
			ret=TRUE;
			break;
		}
		case GDK_BUTTON_PRESS:
		{
			//Grab focus, to receive keypresses
			gtk_widget_grab_focus(widget);
			if(event->button.button == 1)
				th->handleMouseDown(event->button.x,event->button.y);
			ret=TRUE;
			break;
		}
		case GDK_2BUTTON_PRESS:
		{
			if(event->button.button == 1)
				th->handleMouseDoubleClick(event->button.x,event->button.y);
			ret=TRUE;
			break;
		}
		case GDK_BUTTON_RELEASE:
		{
			th->handleMouseUp(event->button.x,event->button.y);
			ret=TRUE;
			break;
		}
		case GDK_MOTION_NOTIFY:
		{
			th->handleMouseMove(event->motion.x,event->motion.y);
			ret=TRUE;
			break;
		}
		default:
//#ifdef EXPENSIVE_DEBUG
//			cout << "GDKTYPE " << event->type << endl;
//#endif
			break;
	}
	return ret;
}

_NR<InteractiveObject> InputThread::getMouseTarget(uint32_t x, uint32_t y, DisplayObject::HIT_TYPE type)
{
	_NR<InteractiveObject> selected = NullRef;
	try
	{
		selected=m_sys->getStage()->hitTest(NullRef,x,y, type);
	}
	catch(LightsparkException& e)
	{
		LOG(LOG_ERROR,_("Error in input handling ") << e.cause);
		m_sys->setError(e.cause);
		return NullRef;
	}
	assert(maskStack.empty());
	assert(selected); /* atleast we hit the stage */
	assert_and_throw(selected->getClass()->isSubClass(Class<InteractiveObject>::getClass()));
	return selected;
}

void InputThread::handleMouseDown(uint32_t x, uint32_t y)
{
	if(m_sys->currentVm == NULL)
		return;
	Locker locker(mutexListeners);
	_NR<InteractiveObject> selected = getMouseTarget(x, y, DisplayObject::GENERIC_HIT);
	number_t localX, localY;
	selected->globalToLocal(x,y,localX,localY);
	m_sys->currentVm->addEvent(selected,
		_MR(Class<MouseEvent>::getInstanceS("mouseDown",localX,localY,true)));
	lastMouseDownTarget=selected;
}

void InputThread::handleMouseDoubleClick(uint32_t x, uint32_t y)
{
	if(m_sys->currentVm == NULL)
		return;
	Locker locker(mutexListeners);
	_NR<InteractiveObject> selected = getMouseTarget(x, y, DisplayObject::DOUBLE_CLICK);
	number_t localX, localY;
	selected->globalToLocal(x,y,localX,localY);
	m_sys->currentVm->addEvent(selected,
		_MR(Class<MouseEvent>::getInstanceS("doubleClick",localX,localY,true)));
}

void InputThread::handleMouseUp(uint32_t x, uint32_t y)
{
	if(m_sys->currentVm == NULL)
		return;
	Locker locker(mutexListeners);
	_NR<InteractiveObject> selected = getMouseTarget(x, y, DisplayObject::GENERIC_HIT);
	number_t localX, localY;
	selected->globalToLocal(x,y,localX,localY);
	m_sys->currentVm->addEvent(selected,
		_MR(Class<MouseEvent>::getInstanceS("mouseUp",localX,localY,true)));
	if(lastMouseDownTarget==selected)
	{
		//Also send the click event
		m_sys->currentVm->addEvent(selected,
			_MR(Class<MouseEvent>::getInstanceS("click",localX,localY,true)));
	}
	lastMouseDownTarget=NullRef;
}

void InputThread::handleMouseMove(uint32_t x, uint32_t y)
{
	if(m_sys->currentVm == NULL)
		return;
	SpinlockLocker locker(inputDataSpinlock);
	mousePos = Vector2(x,y);
	Locker locker2(mutexDragged);
	// Handle current drag operation
	if(curDragged)
	{
		Vector2f local;
		_NR<DisplayObjectContainer> parent = curDragged->getParent();
		if(!parent)
		{
			stopDrag(curDragged.getPtr());
			return;
		}
		local = parent->getConcatenatedMatrix().getInverted().multiply2D(mousePos);
		local += dragOffset;
		if(dragLimit)
			local = local.projectInto(*dragLimit);

		curDragged->setX(local.x);
		curDragged->setY(local.y);
	}
	// Handle non-drag mouse movement
	else
	{
		_NR<InteractiveObject> selected = getMouseTarget(x, y, DisplayObject::GENERIC_HIT);
		number_t localX, localY;
		selected->globalToLocal(x,y,localX,localY);
		if(currentMouseOver == selected)
			m_sys->currentVm->addEvent(selected,
				_MR(Class<MouseEvent>::getInstanceS("mouseMove",localX,localY,true)));
		else
		{
			if(!currentMouseOver.isNull())
			{
				number_t clocalX, clocalY;
				currentMouseOver->globalToLocal(x,y,clocalX,clocalY);
				m_sys->currentVm->addEvent(currentMouseOver,
					_MR(Class<MouseEvent>::getInstanceS("mouseOut",clocalX,clocalY,true,selected)));
				m_sys->currentVm->addEvent(currentMouseOver,
					_MR(Class<MouseEvent>::getInstanceS("rollOut",clocalX,clocalY,true,selected)));
			}
			m_sys->currentVm->addEvent(selected,
				_MR(Class<MouseEvent>::getInstanceS("mouseOver",localX,localY,true,currentMouseOver)));
			m_sys->currentVm->addEvent(selected,
				_MR(Class<MouseEvent>::getInstanceS("rollOver",localX,localY,true,currentMouseOver)));
			currentMouseOver = selected;
		}
	}
}

void InputThread::addListener(InteractiveObject* ob)
{
	Locker locker(mutexListeners);
	assert(ob);

#ifndef NDEBUG
	vector<InteractiveObject*>::const_iterator it=find(listeners.begin(),listeners.end(),ob);
	//Object is already register, should not happen
	assert_and_throw(it==listeners.end());
#endif
	
	//Register the listener
	listeners.push_back(ob);
}

void InputThread::removeListener(InteractiveObject* ob)
{
	Locker locker(mutexListeners);

	vector<InteractiveObject*>::iterator it=find(listeners.begin(),listeners.end(),ob);
	if(it==listeners.end()) //Listener not found
		return;
	
	//Unregister the listener
	listeners.erase(it);
}

void InputThread::startDrag(_R<Sprite> s, const lightspark::RECT* limit, Vector2f offset)
{
	Locker locker(mutexDragged);
	if(s==curDragged)
		return;

	curDragged=s;
	dragLimit=limit;
	dragOffset=offset;
}

void InputThread::stopDrag(Sprite* s)
{
	Locker locker(mutexDragged);
	if(curDragged == s)
	{
		curDragged = NullRef;
		delete dragLimit;
		dragLimit = 0;
	}
}

bool InputThread::isMasked(number_t x, number_t y) const
{
	for(uint32_t i=0;i<maskStack.size();i++)
	{
		number_t localX, localY;
		maskStack[i].m.multiply2D(x,y,localX,localY);//m is the concatenated matrix
		if(maskStack[i].d->isOpaque(localX, localY))//If one of the masks is opaque then you are masked
			return true;
	}

	return false;
}
