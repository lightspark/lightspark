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
	DisplayObjectContainer* lastParent; // needed for AVM1 mouse events that may be handled after the button is removed from stage
	std::vector<pair<uint32_t,DisplayObject*>> states[4];// list of depth/DisplayObjects per state (down/hit/over/up)
	DisplayObject* parentSprite[4]; // container sprites for all states, only used in AVM2
	_NR<SoundChannel> soundchannel_OverUpToIdle;
	_NR<SoundChannel> soundchannel_IdleToOverUp;
	_NR<SoundChannel> soundchannel_OverUpToOverDown;
	_NR<SoundChannel> soundchannel_OverDownToOverUp;
	DefineButtonTag* buttontag;
	bool statesdirty;
public:
	enum BUTTONSTATE
	{
		STATE_UP,
		STATE_OVER,
		STATE_DOWN,
		STATE_OUT
	};
	enum BUTTONOBJECTTYPE {
		BUTTONOBJECTTYPE_DOWN=0,
		BUTTONOBJECTTYPE_HIT=1,
		BUTTONOBJECTTYPE_OVER=2,
		BUTTONOBJECTTYPE_UP=3
	};
private:
	BUTTONSTATE currentState;
	BUTTONSTATE oldstate;
	bool enabled;
	bool useHandCursor;
	bool hasMouse;
	void reflectState(BUTTONSTATE oldstate);
	_NR<DisplayObject> hitTestImpl(const Vector2f& globalPoint, const Vector2f& localPoint, HIT_TYPE type,bool interactiveObjectsOnly) override;
	/* This is called by the VM when an event is dispatched */
	void defaultEventBehavior(_R<Event> e) override;
	void resetStateToStart(BUTTONOBJECTTYPE type);
	void clearStateObject(BUTTONOBJECTTYPE type);
	void getStateObject(BUTTONOBJECTTYPE type, asAtom& ret);
	void setStateObject(BUTTONOBJECTTYPE type,asAtom o);
protected:
	bool boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax, bool visibleOnly) override;
public:
	SimpleButton(ASWorker* wrk,Class_base* c, DefineButtonTag* tag = nullptr);
	void finalize() override;
	bool destruct() override;
	void prepareShutdown() override;
	bool countCylicMemberReferences(garbagecollectorstate& gcstate) override;
	void requestInvalidation(InvalidateQueue* q, bool forceTextureRefresh=false) override;
	uint32_t getTagID() const override;
	void beforeConstruction(bool _explicit = false) override;
	void constructionComplete(bool _explicit = false, bool forInitAction = false) override;
	void addDisplayObject(BUTTONOBJECTTYPE state, uint32_t depth, DisplayObject* o);

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
	bool AVM1HandleKeyboardEvent(KeyboardEvent* e) override;
	bool AVM1HandleMouseEvent(EventDispatcher* dispatcher, MouseEvent *e) override;
	void AVM1HandlePressedEvent(ASObject* dispatcher) override;
	void handleMouseCursor(bool rollover) override;
	bool allowAsMask() const override
	{
		return !isEmpty();
	}

	BUTTONSTATE getCurrentState() const { return currentState; }
	void refreshCurrentState() { reflectState(currentState); }
};


}

#endif /* SCRIPTING_FLASH_DISPLAY_SIMPLEBUTTON_H */
