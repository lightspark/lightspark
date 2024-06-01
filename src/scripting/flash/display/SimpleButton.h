/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef SCRIPTING_FLASH_DISPLAY_SIMPLEBUTTON_H
#define SCRIPTING_FLASH_DISPLAY_SIMPLEBUTTON_H 1

#include "flashdisplay.h"


namespace lightspark
{

/* This is really ugly, but the parent of the current
 * active state (e.g. upState) is set to the owning SimpleButton,
 * which is not a DisplayObjectContainer per spec.
 * We let it derive from DisplayObjectContainer, but
 * call only the InteractiveObject::_constructor
 * to make it look like an InteractiveObject to AS.
 */
class SimpleButton: public DisplayObjectContainer
{
private:
	DisplayObject* downState;
	DisplayObject* hitTestState;
	DisplayObject* overState;
	DisplayObject* upState;
	_NR<SoundChannel> soundchannel_OverUpToIdle;
	_NR<SoundChannel> soundchannel_IdleToOverUp;
	_NR<SoundChannel> soundchannel_OverUpToOverDown;
	_NR<SoundChannel> soundchannel_OverDownToOverUp;
	DefineButtonTag* buttontag;
	enum BUTTONSTATE
	{
		UP,
		OVER,
		DOWN,
		STATE_OUT
	};
	BUTTONSTATE currentState;
	bool enabled;
	bool useHandCursor;
	bool hasMouse;
	void reflectState(BUTTONSTATE oldstate);
	void resetStateToStart(DisplayObject* obj);
	_NR<DisplayObject> hitTestImpl(const Vector2f& globalPoint, const Vector2f& localPoint, DisplayObject::HIT_TYPE type,bool interactiveObjectsOnly) override;
	/* This is called by when an event is dispatched */
	void defaultEventBehavior(_R<Event> e) override;
protected:
	bool boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax, bool visibleOnly) override;
public:
	SimpleButton(ASWorker* wrk,Class_base* c, DisplayObject *dS = nullptr, DisplayObject *hTS = nullptr,
				 DisplayObject *oS = nullptr, DisplayObject *uS = nullptr, DefineButtonTag* tag = nullptr);
	void enterFrame(bool implicit) override;
	void constructionComplete(bool _explicit = false) override;
	void finalize() override;
	bool destruct() override;
	void prepareShutdown() override;
	bool countCylicMemberReferences(garbagecollectorstate& gcstate) override;
	IDrawable* invalidate(bool smoothing) override;
	void requestInvalidation(InvalidateQueue* q, bool forceTextureRefresh=false) override;
	uint32_t getTagID() const override;

	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(_getUpState);
	ASFUNCTION_ATOM(_setUpState);
	ASFUNCTION_ATOM(_getDownState);
	ASFUNCTION_ATOM(_setDownState);
	ASFUNCTION_ATOM(_getOverState);
	ASFUNCTION_ATOM(_setOverState);
	ASFUNCTION_ATOM(_getHitTestState);
	ASFUNCTION_ATOM(_setHitTestState);
	ASFUNCTION_ATOM(_getEnabled);
	ASFUNCTION_ATOM(_setEnabled);
	ASFUNCTION_ATOM(_getUseHandCursor);
	ASFUNCTION_ATOM(_setUseHandCursor);

	void afterLegacyInsert() override;
	void afterLegacyDelete(bool inskipping) override;
	bool AVM1HandleKeyboardEvent(KeyboardEvent* e) override;
	bool AVM1HandleMouseEvent(EventDispatcher* dispatcher, MouseEvent *e) override;
	void handleMouseCursor(bool rollover) override;
	bool allowAsMask() const override
	{
		if (needsActionScript3())
		{
			DisplayObject* stateChild = this->getStateChild();
			if (stateChild && stateChild->is<DisplayObjectContainer>())
				return stateChild->as<DisplayObjectContainer>()->isEmpty();
			return false;
		}
		else
			return !isEmpty();
	}

	DisplayObject* getStateChild() const
	{
		DisplayObject* ret = nullptr;
		switch (currentState)
		{
			case STATE_OUT:
			case UP:
				if (upState)
					ret = upState;
			break;
			case OVER:
				if (overState)
					ret = overState;
			break;
			case DOWN:
				if (downState)
					ret = downState;
			break;
			default: break;
		}
		return ret;
	}
};


}

#endif /* SCRIPTING_FLASH_DISPLAY_SIMPLEBUTTON_H */
