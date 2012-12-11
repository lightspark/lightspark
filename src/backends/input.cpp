/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2012  Alessandro Pignotti (a.pignotti@sssup.it)

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
#include "backends/audio.h"
#include "backends/input.h"
#include "backends/rendering.h"
#include "compat.h"
#include "scripting/flash/ui/keycodes.h"

#if GTK_CHECK_VERSION (2,21,8)
#include <gdk/gdkkeysyms-compat.h> 
#else
#include <gdk/gdkkeysyms.h>
#endif 

using namespace lightspark;
using namespace std;

InputThread::InputThread(SystemState* s):m_sys(s),engineData(NULL),terminated(false),threaded(false),
	curDragged(),currentMouseOver(),lastMouseDownTarget(),
	dragLimit(NULL)
{
	LOG(LOG_INFO,_("Creating input thread"));
}

void InputThread::start(EngineData* e)
{
	initKeyTable();
	engineData = e;
	engineData->setInputHandler(sigc::mem_fun(this, &InputThread::worker));
}

InputThread::~InputThread()
{
	if(engineData)
		engineData->removeInputHandler();
	wait();
}

void InputThread::wait()
{
	if(terminated)
		return;
	if(threaded)
		t->join();
	terminated=true;
}

//This is guarded gdk_threads_enter/leave
bool InputThread::worker(GdkEvent *event)
{
	//Set sys to this SystemState
	setTLSSys(m_sys);
	gboolean ret=FALSE;
	switch(event->type)
	{
		case GDK_KEY_PRESS:
		{
			bool handled = handleKeyboardShortcuts(&event->key);
			if (!handled)
				sendKeyEvent(&event->key);
			ret=TRUE;
			break;
		}
		case GDK_KEY_RELEASE:
		{
			sendKeyEvent(&event->key);
			ret=TRUE;
			break;
		}
		case GDK_EXPOSE:
		{
			//Signal the renderThread
			m_sys->getRenderThread()->draw(false);
			ret=TRUE;
			break;
		}
		case GDK_BUTTON_PRESS:
		{
			if(event->button.button == 1)
			{
				//Grab focus, to receive keypresses
				engineData->grabFocus();

				int stageX, stageY;
				m_sys->windowToStageCoordinates(event->button.x,event->button.y,stageX,stageY);
				handleMouseDown(stageX,stageY,event->button.state);
			}
			ret=TRUE;
			break;
		}
		case GDK_2BUTTON_PRESS:
		{
			if(event->button.button == 1)
			{
				int stageX, stageY;
				m_sys->windowToStageCoordinates(event->button.x,event->button.y,stageX,stageY);
				handleMouseDoubleClick(stageX,stageY,event->button.state);
			}
			ret=TRUE;
			break;
		}
		case GDK_BUTTON_RELEASE:
		{
			int stageX, stageY;
			m_sys->windowToStageCoordinates(event->button.x,event->button.y,stageX,stageY);
			handleMouseUp(stageX,stageY,event->button.state);
			ret=TRUE;
			break;
		}
		case GDK_MOTION_NOTIFY:
		{
			int stageX, stageY;
			m_sys->windowToStageCoordinates(event->motion.x,event->motion.y,stageX,stageY);
			handleMouseMove(stageX,stageY,event->button.state);
			ret=TRUE;
			break;
		}
		default:
//#ifdef EXPENSIVE_DEBUG
//			LOG(LOG_INFO, "GDKTYPE " << event->type);
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
		_NR<DisplayObject> dispobj=m_sys->mainClip->getStage()->hitTest(NullRef,x,y, type);
		if(!dispobj.isNull() && dispobj->is<InteractiveObject>())
		{
			dispobj->incRef();
			selected=_MNR(dispobj->as<InteractiveObject>());
		}
	}
	catch(LightsparkException& e)
	{
		LOG(LOG_ERROR,_("Error in input handling ") << e.cause);
		m_sys->setError(e.cause);
		return NullRef;
	}
	assert(selected); /* atleast we hit the stage */
	assert_and_throw(selected->getClass()->isSubClass(Class<InteractiveObject>::getClass()));
	return selected;
}

void InputThread::handleMouseDown(uint32_t x, uint32_t y, unsigned int buttonState)
{
	if(m_sys->currentVm == NULL)
		return;
	Locker locker(mutexListeners);
	_NR<InteractiveObject> selected = getMouseTarget(x, y, DisplayObject::MOUSE_CLICK);
	number_t localX, localY;
	selected->globalToLocal(x,y,localX,localY);
	m_sys->currentVm->addEvent(selected,
		_MR(Class<MouseEvent>::getInstanceS("mouseDown",localX,localY,true,buttonState)));
	lastMouseDownTarget=selected;
}

void InputThread::handleMouseDoubleClick(uint32_t x, uint32_t y, unsigned int buttonState)
{
	if(m_sys->currentVm == NULL)
		return;
	Locker locker(mutexListeners);
	_NR<InteractiveObject> selected = getMouseTarget(x, y, DisplayObject::DOUBLE_CLICK);
	number_t localX, localY;
	selected->globalToLocal(x,y,localX,localY);
	m_sys->currentVm->addEvent(selected,
		_MR(Class<MouseEvent>::getInstanceS("doubleClick",localX,localY,true,buttonState)));
}

void InputThread::handleMouseUp(uint32_t x, uint32_t y, unsigned int buttonState)
{
	if(m_sys->currentVm == NULL)
		return;
	Locker locker(mutexListeners);
	_NR<InteractiveObject> selected = getMouseTarget(x, y, DisplayObject::MOUSE_CLICK);
	number_t localX, localY;
	selected->globalToLocal(x,y,localX,localY);
	m_sys->currentVm->addEvent(selected,
		_MR(Class<MouseEvent>::getInstanceS("mouseUp",localX,localY,true,buttonState)));
	if(lastMouseDownTarget==selected)
	{
		//Also send the click event
		m_sys->currentVm->addEvent(selected,
			_MR(Class<MouseEvent>::getInstanceS("click",localX,localY,true,buttonState)));
	}
	lastMouseDownTarget=NullRef;
}

void InputThread::handleMouseMove(uint32_t x, uint32_t y, unsigned int buttonState)
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
		_NR<InteractiveObject> selected = getMouseTarget(x, y, DisplayObject::MOUSE_CLICK);
		number_t localX, localY;
		selected->globalToLocal(x,y,localX,localY);
		if(currentMouseOver == selected)
			m_sys->currentVm->addEvent(selected,
				_MR(Class<MouseEvent>::getInstanceS("mouseMove",localX,localY,true,buttonState)));
		else
		{
			if(!currentMouseOver.isNull())
			{
				number_t clocalX, clocalY;
				currentMouseOver->globalToLocal(x,y,clocalX,clocalY);
				m_sys->currentVm->addEvent(currentMouseOver,
					_MR(Class<MouseEvent>::getInstanceS("mouseOut",clocalX,clocalY,true,buttonState,selected)));
				m_sys->currentVm->addEvent(currentMouseOver,
					_MR(Class<MouseEvent>::getInstanceS("rollOut",clocalX,clocalY,true,buttonState,selected)));
			}
			m_sys->currentVm->addEvent(selected,
				_MR(Class<MouseEvent>::getInstanceS("mouseOver",localX,localY,true,buttonState,currentMouseOver)));
			m_sys->currentVm->addEvent(selected,
				_MR(Class<MouseEvent>::getInstanceS("rollOver",localX,localY,true,buttonState,currentMouseOver)));
			currentMouseOver = selected;
		}
	}
}

void InputThread::initKeyTable()
{
	int i = 0;
	while (hardwareKeycodes[i].keyname)
	{
		// Map GDK keyvals to hardware keycodes.
		//
		// NOTE: The keycodes returned by GDK are different
		// from the keycodes in the Flash documentation.
		// Should add mapping from GDK codes to Flash codes
		// for AS files that use raw numerical values instead
		// of Keyboard.* constants.
		GdkKeymapKey *keys;
		int keys_len;
		const char *keyname = hardwareKeycodes[i].keyname;
		unsigned keyval = hardwareKeycodes[i].gdkKeyval;
		if (gdk_keymap_get_entries_for_keyval(NULL, keyval, &keys, &keys_len))
		{
			KeyNameCodePair key;
			key.keyname = keyname;
			key.keycode = keys[0].keycode;
			keyNamesAndCodes.push_back(key);
			g_free(keys);
		}

		i++;
	}
}

const std::vector<KeyNameCodePair>& InputThread::getKeyNamesAndCodes()
{
	// No locking needed, because keyNamesAndCodes is not modified
	// after being initialized
	return keyNamesAndCodes;
}

bool InputThread::handleKeyboardShortcuts(const GdkEventKey *keyevent)
{
	bool handled = false;

	if ((keyevent->state & GDK_MODIFIER_MASK) != GDK_CONTROL_MASK)
		return handled;

	switch(keyevent->keyval)
	{
		case GDK_q:
			handled = true;
			if(m_sys->standalone)
				m_sys->setShutdownFlag();
			break;
		case GDK_p:
			handled = true;
			m_sys->showProfilingData=!m_sys->showProfilingData;
			break;
		case GDK_m:
			handled = true;
			if (!m_sys->audioManager->pluginLoaded())
				break;
			m_sys->audioManager->toggleMuteAll();
			if(m_sys->audioManager->allMuted())
				LOG(LOG_INFO, "All sounds muted");
			else
				LOG(LOG_INFO, "All sounds unmuted");
			break;
		case GDK_c:
			handled = true;
			if(m_sys->hasError())
			{
				GtkClipboard *clipboard;
				clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
				gtk_clipboard_set_text(clipboard, m_sys->getErrorCause().c_str(),
						       m_sys->getErrorCause().size());
				clipboard = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
				gtk_clipboard_set_text(clipboard, m_sys->getErrorCause().c_str(),
						       m_sys->getErrorCause().size());
				LOG(LOG_INFO, "Copied error to clipboard");
			}
			else
				LOG(LOG_INFO, "No error to be copied to clipboard");
			break;
		default:
			break;
	}

	return handled;
}

void InputThread::sendKeyEvent(const GdkEventKey *keyevent)
{
	if(m_sys->currentVm == NULL)
		return;

	Locker locker(mutexListeners);

	_NR<DisplayObject> target = m_sys->mainClip->getStage()->getFocusTarget();
	if (target.isNull())
		return;

	tiny_string type;
	if (keyevent->type == GDK_KEY_PRESS)
		type = "keyDown";
	else
		type = "keyUp";

	uint32_t charcode = keyevent->keyval;
	if (keyevent->is_modifier)
		charcode = 0;

	m_sys->currentVm->addEvent(target,
	    _MR(Class<KeyboardEvent>::getInstanceS(type, charcode, keyevent->hardware_keycode, keyevent->state)));
}

void InputThread::addListener(InteractiveObject* ob)
{
	Locker locker(mutexListeners);
	assert(ob);

#ifndef NDEBUG
	vector<InteractiveObject*>::const_iterator it=find(listeners.begin(),listeners.end(),ob);
	//Object is already register, should not happen
	if(it != listeners.end())
	{
		LOG(LOG_ERROR, "Trying to addListener an InteractiveObject that's already added.");
		return;
	}
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
