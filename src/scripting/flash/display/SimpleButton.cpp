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

#include <list>

#include "scripting/flash/display/SimpleButton.h"
#include "scripting/flash/display/RootMovieClip.h"
#include "parsing/tags.h"
#include "scripting/class.h"
#include "scripting/flash/media/flashmedia.h"
#include "backends/cachedsurface.h"
#include "scripting/argconv.h"


using namespace std;
using namespace lightspark;

void SimpleButton::sinit(Class_base* c)
{
	CLASS_SETUP(c, InteractiveObject, _constructor, CLASS_SEALED);
	c->isReusable=true;
	c->setDeclaredMethodByQName("upState","",c->getSystemState()->getBuiltinFunction(_getUpState),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("upState","",c->getSystemState()->getBuiltinFunction(_setUpState),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("downState","",c->getSystemState()->getBuiltinFunction(_getDownState),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("downState","",c->getSystemState()->getBuiltinFunction(_setDownState),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("overState","",c->getSystemState()->getBuiltinFunction(_getOverState),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("overState","",c->getSystemState()->getBuiltinFunction(_setOverState),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("hitTestState","",c->getSystemState()->getBuiltinFunction(_getHitTestState),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("hitTestState","",c->getSystemState()->getBuiltinFunction(_setHitTestState),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("enabled","",c->getSystemState()->getBuiltinFunction(_getEnabled),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("enabled","",c->getSystemState()->getBuiltinFunction(_setEnabled),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("useHandCursor","",c->getSystemState()->getBuiltinFunction(_getUseHandCursor),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("useHandCursor","",c->getSystemState()->getBuiltinFunction(_setUseHandCursor),SETTER_METHOD,true);
}

void SimpleButton::afterLegacyInsert()
{
	getSystemState()->stage->AVM1AddKeyboardListener(this);
	getSystemState()->stage->AVM1AddMouseListener(this);
	DisplayObjectContainer::afterLegacyInsert();
}

void SimpleButton::afterLegacyDelete(bool inskipping)
{
	getSystemState()->stage->AVM1RemoveKeyboardListener(this);
	getSystemState()->stage->AVM1RemoveMouseListener(this);
}

bool SimpleButton::AVM1HandleMouseEvent(EventDispatcher* dispatcher, MouseEvent *e)
{
	if (!this->isOnStage() || !this->enabled || !this->isVisible() || this->loadedFrom->needsActionScript3())
		return false;
	if (!dispatcher->is<DisplayObject>())
		return false;
	DisplayObject* dispobj=nullptr;
	if(e->type == "mouseOut")
	{
		if (dispatcher!= this)
			return false;
	}
	else
	{
		if (dispatcher == this)
			dispobj=this;
		else
		{
			number_t x,y;
			// TODO: Add an overload for Vector2f.
			dispatcher->as<DisplayObject>()->localToGlobal(e->localX,e->localY,x,y);
			number_t x1,y1;
			// TODO: Add an overload for Vector2f.
			this->globalToLocal(x,y,x1,y1);
			_NR<DisplayObject> d = hitTest(Vector2f(x,y), Vector2f(x1,y1), DisplayObject::MOUSE_CLICK_HIT,true);
			dispobj=d.getPtr();
		}
		if (dispobj!= this)
			return false;
	}
	BUTTONSTATE oldstate = currentState;
	if(e->type == "mouseDown")
	{
		currentState = DOWN;
		reflectState(oldstate);
	}
	else if(e->type == "mouseUp")
	{
		currentState = UP;
		reflectState(oldstate);
	}
	else if(e->type == "mouseOver")
	{
		currentState = OVER;
		reflectState(oldstate);
	}
	else if(e->type == "mouseOut")
	{
		currentState = STATE_OUT;
		reflectState(oldstate);
	}
	bool handled = false;
	if (buttontag)
	{
		for (auto it = buttontag->condactions.begin(); it != buttontag->condactions.end(); it++)
		{
			if (  (it->CondIdleToOverDown && currentState==DOWN)
				||(it->CondOutDownToIdle && oldstate==DOWN && currentState==STATE_OUT)
				||(it->CondOutDownToOverDown && oldstate==DOWN && currentState==OVER)
				||(it->CondOverDownToOutDown && (oldstate==DOWN || oldstate==OVER) && currentState==STATE_OUT)
				||(it->CondOverDownToOverUp && (oldstate==DOWN || oldstate==OVER) && currentState==UP)
				||(it->CondOverUpToOverDown && (oldstate==UP || oldstate==OVER) && currentState==DOWN)
				||(it->CondOverUpToIdle && (oldstate==UP || oldstate==OVER) && currentState==STATE_OUT)
				||(it->CondIdleToOverUp && oldstate==STATE_OUT && currentState==OVER)
				||(it->CondOverDownToIdle && oldstate==DOWN && currentState==OVER)
				)
			{
				DisplayObjectContainer* c = getParent();
				while (c && !c->is<MovieClip>())
					c = c->getParent();
				if (c)
				{
					std::map<uint32_t,asAtom> m;
					ACTIONRECORD::executeActions(c->as<MovieClip>(),c->as<MovieClip>()->getCurrentFrame()->getAVM1Context(),it->actions,it->startactionpos,m);
					handled = true;
				}
				
			}
		}
	}
	handled |= AVM1HandleMouseEventStandard(dispobj,e);
	return handled;
}

void SimpleButton::handleMouseCursor(bool rollover)
{
	if (rollover)
	{
		hasMouse=true;
		getSystemState()->setMouseHandCursor(this->useHandCursor);
	}
	else
	{
		getSystemState()->setMouseHandCursor(false);
		hasMouse=false;
	}
}

bool SimpleButton::AVM1HandleKeyboardEvent(KeyboardEvent *e)
{
	bool handled=false;
	for (auto it = this->buttontag->condactions.begin(); it != this->buttontag->condactions.end(); it++)
	{
		bool execute=false;
		uint32_t code = e->getSDLScanCode();
		if (e->getModifiers() & KMOD_SHIFT)
		{
			switch (it->CondKeyPress)
			{
				case 33:// !
					execute = code==SDL_SCANCODE_1;break;
				case 34:// "
					execute = code==SDL_SCANCODE_APOSTROPHE;break;
				case 35:// #
					execute = code==SDL_SCANCODE_3;break;
				case 36:// $
					execute = code==SDL_SCANCODE_4;break;
				case 37:// %
					execute = code==SDL_SCANCODE_5;break;
				case 38:// &
					execute = code==SDL_SCANCODE_7;break;
				case 40:// (
					execute = code==SDL_SCANCODE_9;break;
				case 41:// )
					execute = code==SDL_SCANCODE_0;break;
				case 42:// *
					execute = code==SDL_SCANCODE_8;break;
				case 43:// +
					execute = code==SDL_SCANCODE_EQUALS;break;
				case 58:// :
					execute = code==SDL_SCANCODE_SEMICOLON;break;
				case 60:// <
					execute = code==SDL_SCANCODE_COMMA;break;
				case 62:// >
					execute = code==SDL_SCANCODE_PERIOD;break;
				case 63:// ?
					execute = code==SDL_SCANCODE_SLASH;break;
				case 64:// @
					execute = code==SDL_SCANCODE_2;break;
				case 94:// ^
					execute = code==SDL_SCANCODE_6;break;
				case 95:// _
					execute = code==SDL_SCANCODE_MINUS;break;
				case 123:// {
					execute = code==SDL_SCANCODE_LEFTBRACKET;break;
				case 124:// |
					execute = code==SDL_SCANCODE_BACKSLASH;break;
				case 125:// }
					execute = code==SDL_SCANCODE_RIGHTBRACKET;break;
				case 126:// ~
					execute = code==SDL_SCANCODE_GRAVE;break;
				default:// A-Z
					execute = it->CondKeyPress>=65
							&& it->CondKeyPress<=90
							&& code-SDL_SCANCODE_A==it->CondKeyPress-65;
					break;
			}
		}
		else
		{
			switch (it->CondKeyPress)
			{
				case 1:
					execute = code==SDL_SCANCODE_LEFT;break;
				case 2:
					execute = code==SDL_SCANCODE_RIGHT;break;
				case 3:
					execute = code==SDL_SCANCODE_HOME;break;
				case 4:
					execute = code==SDL_SCANCODE_END;break;
				case 5:
					execute = code==SDL_SCANCODE_INSERT;break;
				case 6:
					execute = code==SDL_SCANCODE_DELETE;break;
				case 8:
					execute = code==SDL_SCANCODE_BACKSPACE;break;
				case 13:
					execute = code==SDL_SCANCODE_RETURN;break;
				case 14:
					execute = code==SDL_SCANCODE_UP;break;
				case 15:
					execute = code==SDL_SCANCODE_DOWN;break;
				case 16:
					execute = code==SDL_SCANCODE_PAGEUP;break;
				case 17:
					execute = code==SDL_SCANCODE_PAGEDOWN;break;
				case 18:
					execute = code==SDL_SCANCODE_TAB;break;
				case 19:
					execute = code==SDL_SCANCODE_ESCAPE;break;
				case 32:
					execute = code==SDL_SCANCODE_SPACE;break;
				case 39:// '
					execute = code==SDL_SCANCODE_APOSTROPHE;break;
				case 44:// ,
					execute = code==SDL_SCANCODE_COMMA;break;
				case 45:// -
					execute = code==SDL_SCANCODE_MINUS;break;
				case 46:// .
					execute = code==SDL_SCANCODE_PERIOD;break;
				case 47:// /
					execute = code==SDL_SCANCODE_SLASH;break;
				case 48:// 0
					execute = code==SDL_SCANCODE_0;break;
				case 49:// 1
					execute = code==SDL_SCANCODE_1;break;
				case 50:// 2
					execute = code==SDL_SCANCODE_2;break;
				case 51:// 3
					execute = code==SDL_SCANCODE_3;break;
				case 52:// 4
					execute = code==SDL_SCANCODE_4;break;
				case 53:// 5
					execute = code==SDL_SCANCODE_5;break;
				case 54:// 6
					execute = code==SDL_SCANCODE_6;break;
				case 55:// 7
					execute = code==SDL_SCANCODE_7;break;
				case 56:// 8
					execute = code==SDL_SCANCODE_8;break;
				case 57:// 9
					execute = code==SDL_SCANCODE_9;break;
				case 59:// ;
					execute = code==SDL_SCANCODE_SEMICOLON;break;
				case 61:// =
					execute = code==SDL_SCANCODE_EQUALS;break;
				case 91:// [
					execute = code==SDL_SCANCODE_LEFTBRACKET;break;
				case 92:// 
					execute = code==SDL_SCANCODE_BACKSLASH;break;
				case 93:// ]
					execute = code==SDL_SCANCODE_RIGHTBRACKET;break;
				case 96:// `
					execute = code==SDL_SCANCODE_GRAVE;break;
				default:// a-z
					execute = it->CondKeyPress>=97
							&& it->CondKeyPress<=122
							&& code-SDL_SCANCODE_A==it->CondKeyPress-97;
					break;
			}
		}
		if (execute)
		{
			DisplayObjectContainer* c = getParent();
			while (c && !c->is<MovieClip>())
				c = c->getParent();
			std::map<uint32_t,asAtom> m;
			ACTIONRECORD::executeActions(c->as<MovieClip>(),c->as<MovieClip>()->getCurrentFrame()->getAVM1Context(),it->actions,it->startactionpos,m);
			handled=true;
		}
	}
	if (!handled)
		DisplayObjectContainer::AVM1HandleKeyboardEvent(e);
	return handled;
}


_NR<DisplayObject> SimpleButton::hitTestImpl(const Vector2f& globalPoint, const Vector2f& localPoint, DisplayObject::HIT_TYPE type,bool interactiveObjectsOnly)
{
	_NR<DisplayObject> ret = NullRef;
	if(hitTestState)
	{
		if(!hitTestState->getMatrix().isInvertible())
			return NullRef;

		const auto hitPoint = hitTestState->getMatrix().getInverted().multiply2D(localPoint);
		ret = hitTestState->hitTest(globalPoint, hitPoint, type,false);
	}
	/* mouseDown events, for example, are never dispatched to the hitTestState,
	 * but directly to this button (and with event.target = this). This has been
	 * tested with the official flash player. It cannot work otherwise, as
	 * hitTestState->parent == nullptr. (This has also been verified)
	 */
	if(ret)
	{
		if(interactiveObjectsOnly && !isHittable(type))
			return NullRef;

		if (ret.getPtr() != this)
		{
			this->incRef();
			ret = _MR(this);
		}
	}
	return ret;
}

void SimpleButton::defaultEventBehavior(_R<Event> e)
{
	bool is_valid = true;
	BUTTONSTATE oldstate = currentState;
	if(e->type == "mouseDown")
		currentState = DOWN;
	else if(e->type == "releaseOutside")
		currentState = UP;
	else if(e->type == "rollOver" || e->type == "mouseOver" || e->type == "mouseUp")
		currentState = OVER;
	else if(e->type == "rollOut" || e->type == "mouseOut")
		currentState = STATE_OUT;
	else
		is_valid = false;

	if (is_valid)
		reflectState(oldstate);
	else
		DisplayObjectContainer::defaultEventBehavior(e);
}

bool SimpleButton::boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax, bool visibleOnly)
{
	if (visibleOnly && !this->isVisible())
		return false;
	bool ret = false;
	number_t txmin,txmax,tymin,tymax;
	if (upState && upState->getBounds(txmin,txmax,tymin,tymax,upState->getMatrix()))
	{
		if(ret==true)
		{
			xmin = min(xmin,txmin);
			xmax = max(xmax,txmax);
			ymin = min(ymin,tymin);
			ymax = max(ymax,tymax);
		}
		else
		{
			xmin=txmin;
			xmax=txmax;
			ymin=tymin;
			ymax=tymax;
			ret=true;
		}
	}
	if (overState && overState->getBounds(txmin,txmax,tymin,tymax,overState->getMatrix()))
	{
		if(ret==true)
		{
			xmin = min(xmin,txmin);
			xmax = max(xmax,txmax);
			ymin = min(ymin,tymin);
			ymax = max(ymax,tymax);
		}
		else
		{
			xmin=txmin;
			xmax=txmax;
			ymin=tymin;
			ymax=tymax;
			ret=true;
		}
	}
	if (downState && downState->getBounds(txmin,txmax,tymin,tymax,downState->getMatrix()))
	{
		if(ret==true)
		{
			xmin = min(xmin,txmin);
			xmax = max(xmax,txmax);
			ymin = min(ymin,tymin);
			ymax = max(ymax,tymax);
		}
		else
		{
			xmin=txmin;
			xmax=txmax;
			ymin=tymin;
			ymax=tymax;
			ret=true;
		}
	}
	return ret;
}

SimpleButton::SimpleButton(ASWorker* wrk, Class_base* c, DisplayObject *dS, DisplayObject *hTS,
				DisplayObject *oS, DisplayObject *uS, DefineButtonTag *tag)
	: DisplayObjectContainer(wrk,c), downState(dS), hitTestState(hTS), overState(oS), upState(uS),
	  buttontag(tag),currentState(STATE_OUT),enabled(true),useHandCursor(true),hasMouse(false)
{
	subtype = SUBTYPE_SIMPLEBUTTON;
	/* When called from DefineButton2Tag::instance, they are not constructed yet
	 * TODO: construct them here for once, or each time they become visible?
	 */
	if(dS)
	{
		dS->addStoredMember();
		dS->advanceFrame(false);
		dS->initFrame();
		getSystemState()->stage->removeHiddenObject(dS); // avoid any changes when not visible
	}
	if(hTS)
	{
		hTS->addStoredMember();
		hTS->advanceFrame(false);
		hTS->initFrame();
		getSystemState()->stage->removeHiddenObject(hTS); // avoid any changes when not visible
	}
	if(oS)
	{
		oS->addStoredMember();
		oS->advanceFrame(false);
		oS->initFrame();
		getSystemState()->stage->removeHiddenObject(oS); // avoid any changes when not visible
	}
	if(uS)
	{
		uS->addStoredMember();
		uS->advanceFrame(false);
		uS->initFrame();
		getSystemState()->stage->removeHiddenObject(uS); // avoid any changes when not visible
	}
	if (tag)
		this->loadedFrom = tag->loadedFrom;
	if (!needsActionScript3())
	{
		asAtom obj = asAtomHandler::fromObjectNoPrimitive(this);
		getClass()->handleConstruction(obj,nullptr,0,true);
	}
	if (tag && tag->sounds)
	{
		if (tag->sounds->SoundID0_OverUpToIdle)
		{
			DefineSoundTag* sound = dynamic_cast<DefineSoundTag*>(tag->loadedFrom->dictionaryLookup(tag->sounds->SoundID0_OverUpToIdle));
			if (sound)
				soundchannel_OverUpToIdle = _MR(sound->createSoundChannel(&tag->sounds->SoundInfo0_OverUpToIdle));
			else
				LOG(LOG_ERROR,"ButtonSound not found for OverUpToIdle:"<<tag->sounds->SoundID0_OverUpToIdle << " on button "<<tag->getId());
		}
		if (tag->sounds->SoundID1_IdleToOverUp)
		{
			DefineSoundTag* sound = dynamic_cast<DefineSoundTag*>(tag->loadedFrom->dictionaryLookup(tag->sounds->SoundID1_IdleToOverUp));
			if (sound)
				soundchannel_IdleToOverUp = _MR(sound->createSoundChannel(&tag->sounds->SoundInfo1_IdleToOverUp));
			else
				LOG(LOG_ERROR,"ButtonSound not found for IdleToOverUp:"<<tag->sounds->SoundID1_IdleToOverUp << " on button "<<tag->getId());
		}
		if (tag->sounds->SoundID2_OverUpToOverDown)
		{
			DefineSoundTag* sound = dynamic_cast<DefineSoundTag*>(tag->loadedFrom->dictionaryLookup(tag->sounds->SoundID2_OverUpToOverDown));
			if (sound)
				soundchannel_OverUpToOverDown = _MR(sound->createSoundChannel(&tag->sounds->SoundInfo2_OverUpToOverDown));
			else
				LOG(LOG_ERROR,"ButtonSound not found for OverUpToOverDown:"<<tag->sounds->SoundID2_OverUpToOverDown << " on button "<<tag->getId());
		}
		if (tag->sounds->SoundID3_OverDownToOverUp)
		{
			DefineSoundTag* sound = dynamic_cast<DefineSoundTag*>(tag->loadedFrom->dictionaryLookup(tag->sounds->SoundID3_OverDownToOverUp));
			if (sound)
				soundchannel_OverDownToOverUp = _MR(sound->createSoundChannel(&tag->sounds->SoundInfo3_OverDownToOverUp));
			else
				LOG(LOG_ERROR,"ButtonSound not found for OverUpToOverDown:"<<tag->sounds->SoundID3_OverDownToOverUp << " on button "<<tag->getId());
		}
	}
	tabEnabled = true;
}

void SimpleButton::enterFrame(bool implicit)
{
	if (needsActionScript3())
	{
		if (hitTestState)
			hitTestState->enterFrame(implicit);
		if (upState)
			upState->enterFrame(implicit);
		if (downState)
			downState->enterFrame(implicit);
		if (overState)
			overState->enterFrame(implicit);
	}
}

void SimpleButton::constructionComplete(bool _explicit)
{
	reflectState(STATE_OUT);
	DisplayObjectContainer::constructionComplete(_explicit);
}
void SimpleButton::finalize()
{
	DisplayObjectContainer::finalize();
	if (downState)
		downState->removeStoredMember();
	downState=nullptr;
	if (hitTestState)
		hitTestState->removeStoredMember();
	hitTestState=nullptr;
	if (overState)
		overState->removeStoredMember();
	overState=nullptr;
	if (upState)
		upState->removeStoredMember();
	upState=nullptr;

	enabled=true;
	useHandCursor=true;
	hasMouse=false;
	buttontag=nullptr;
}

bool SimpleButton::destruct()
{
	if (downState)
		downState->removeStoredMember();
	downState=nullptr;
	if (hitTestState)
		hitTestState->removeStoredMember();
	hitTestState=nullptr;
	if (overState)
		overState->removeStoredMember();
	overState=nullptr;
	if (upState)
		upState->removeStoredMember();
	upState=nullptr;

	enabled=true;
	useHandCursor=true;
	hasMouse=false;
	buttontag=nullptr;
	return DisplayObjectContainer::destruct();
}

void SimpleButton::prepareShutdown()
{
	if (preparedforshutdown)
		return;
	DisplayObjectContainer::prepareShutdown();
	if(downState)
		downState->prepareShutdown();
	if(hitTestState)
		hitTestState->prepareShutdown();
	if(overState)
		overState->prepareShutdown();
	if(upState)
		upState->prepareShutdown();
	if(soundchannel_OverUpToIdle)
		soundchannel_OverUpToIdle->prepareShutdown();
	if(soundchannel_IdleToOverUp)
		soundchannel_IdleToOverUp->prepareShutdown();
	if(soundchannel_OverUpToOverDown)
		soundchannel_OverUpToOverDown->prepareShutdown();
	if(soundchannel_OverDownToOverUp)
		soundchannel_OverDownToOverUp->prepareShutdown();
}

bool SimpleButton::countCylicMemberReferences(garbagecollectorstate& gcstate)
{
	if (gcstate.checkAncestors(this))
		return false;
	bool ret = DisplayObjectContainer::countCylicMemberReferences(gcstate);
	if (downState)
		ret = downState->countAllCylicMemberReferences(gcstate) || ret;
	if (hitTestState)
		ret = hitTestState->countAllCylicMemberReferences(gcstate) || ret;
	if (overState)
		ret = overState->countAllCylicMemberReferences(gcstate) || ret;
	if (upState)
		ret = upState->countAllCylicMemberReferences(gcstate) || ret;
	return ret;
	
}
IDrawable *SimpleButton::invalidate(bool smoothing)
{
	return DisplayObjectContainer::invalidate(smoothing);
}
void SimpleButton::requestInvalidation(InvalidateQueue* q, bool forceTextureRefresh)
{
	requestInvalidationFilterParent(q);
	DisplayObjectContainer::requestInvalidation(q,forceTextureRefresh);
	hasChanged=true;
	incRef();
	q->addToInvalidateQueue(_MR(this));
	
}

uint32_t SimpleButton::getTagID() const
{
	return buttontag ? uint32_t(buttontag->getId()) : 0;
}

ASFUNCTIONBODY_ATOM(SimpleButton,_constructor)
{
	/* This _must_ not call the DisplayObjectContainer
	 * see note at the class declaration.
	 */
	InteractiveObject::_constructor(ret,wrk,obj,nullptr,0);
	SimpleButton* th=asAtomHandler::as<SimpleButton>(obj);
	_NR<DisplayObject> upState;
	_NR<DisplayObject> overState;
	_NR<DisplayObject> downState;
	_NR<DisplayObject> hitTestState;
	ARG_CHECK(ARG_UNPACK(upState, NullRef)(overState, NullRef)(downState, NullRef)(hitTestState, NullRef));

	if (!upState.isNull())
	{
		th->upState = upState.getPtr();
		th->upState->incRef();
		th->upState->addStoredMember();
	}
	if (!overState.isNull())
	{
		th->overState = overState.getPtr();
		th->overState->incRef();
		th->overState->addStoredMember();
	}
	if (!downState.isNull())
	{
		th->downState = downState.getPtr();
		th->downState->incRef();
		th->downState->addStoredMember();
	}
	if (!hitTestState.isNull())
	{
		th->hitTestState = hitTestState.getPtr();
		th->hitTestState->incRef();
		th->hitTestState->addStoredMember();
	}
}

void SimpleButton::reflectState(BUTTONSTATE oldstate)
{
	assert(dynamicDisplayList.empty() || dynamicDisplayList.size() == 1);
	if (isConstructed() && oldstate==currentState)
		return;
	if(!dynamicDisplayList.empty())
	{
		_removeChild(dynamicDisplayList.front(),true);
	}

	if((currentState == UP || currentState == STATE_OUT) && upState)
	{
		resetStateToStart(upState);
		upState->incRef();
		_addChildAt(upState,0);
	}
	else if(currentState == DOWN && downState)
	{
		resetStateToStart(downState);
		downState->incRef();
		_addChildAt(downState,0);
	}
	else if(currentState == OVER && overState)
	{
		resetStateToStart(overState);
		overState->incRef();
		_addChildAt(overState,0);
	}
	if ((oldstate == OVER || oldstate == UP) && currentState == STATE_OUT && soundchannel_OverUpToIdle)
		soundchannel_OverUpToIdle->play();
	if (oldstate == STATE_OUT && (currentState == OVER || currentState == UP) && soundchannel_IdleToOverUp)
		soundchannel_IdleToOverUp->play();
	if ((oldstate == OVER || oldstate == UP) && currentState == DOWN && soundchannel_OverUpToOverDown)
		soundchannel_OverUpToOverDown->play();
	if (oldstate == DOWN && currentState == UP && soundchannel_OverDownToOverUp)
		soundchannel_OverDownToOverUp->play();
}
void SimpleButton::resetStateToStart(DisplayObject* obj)
{
	// reset the MovieClips belonging to the current State to frame 0, so animations will start from the beginning when state has changed
	asAtom arg = asAtomHandler::fromInt(0);
	if (obj->getSubtype()== SUBTYPE_SPRITE) // really a sprite, this means it contains the "real" DisplayObjects for the current state
	{
		std::vector<_R<DisplayObject>> tmplist;
		obj->as<Sprite>()->cloneDisplayList(tmplist);
		for (auto it = tmplist.begin(); it != tmplist.end(); it++)
		{
			if ((*it)->is<MovieClip>())
				(*it)->as<MovieClip>()->gotoAnd(&arg,1,false);
		}
	}
	else if (obj->is<MovieClip>())
		obj->as<MovieClip>()->gotoAnd(&arg,1,false);
}

ASFUNCTIONBODY_ATOM(SimpleButton,_getUpState)
{
	SimpleButton* th=asAtomHandler::as<SimpleButton>(obj);
	if(!th->upState)
	{
		asAtomHandler::setNull(ret);
		return;
	}

	th->upState->incRef();
	ret = asAtomHandler::fromObjectNoPrimitive(th->upState);
}

ASFUNCTIONBODY_ATOM(SimpleButton,_setUpState)
{
	assert_and_throw(argslen == 1);
	SimpleButton* th=asAtomHandler::as<SimpleButton>(obj);
	if(th->upState)
		th->upState->removeStoredMember();
	th->upState = asAtomHandler::as<DisplayObject>(args[0]);
	if (th->upState)
	{
		th->upState->incRef();
		th->upState->addStoredMember();
	}
	th->reflectState(th->currentState);
}

ASFUNCTIONBODY_ATOM(SimpleButton,_getHitTestState)
{
	SimpleButton* th=asAtomHandler::as<SimpleButton>(obj);
	if(!th->hitTestState)
	{
		asAtomHandler::setNull(ret);
		return;
	}

	th->hitTestState->incRef();
	ret = asAtomHandler::fromObjectNoPrimitive(th->hitTestState);
}

ASFUNCTIONBODY_ATOM(SimpleButton,_setHitTestState)
{
	assert_and_throw(argslen == 1);
	SimpleButton* th=asAtomHandler::as<SimpleButton>(obj);
	if(th->hitTestState)
		th->hitTestState->removeStoredMember();
	th->hitTestState = asAtomHandler::as<DisplayObject>(args[0]);
	if (th->hitTestState)
	{
		th->hitTestState->incRef();
		th->hitTestState->addStoredMember();
	}
}

ASFUNCTIONBODY_ATOM(SimpleButton,_getOverState)
{
	SimpleButton* th=asAtomHandler::as<SimpleButton>(obj);
	if(!th->overState)
	{
		asAtomHandler::setNull(ret);
		return;
	}

	th->overState->incRef();
	ret = asAtomHandler::fromObjectNoPrimitive(th->overState);
}

ASFUNCTIONBODY_ATOM(SimpleButton,_setOverState)
{
	assert_and_throw(argslen == 1);
	SimpleButton* th=asAtomHandler::as<SimpleButton>(obj);
	if(th->overState)
		th->overState->removeStoredMember();
	th->overState = asAtomHandler::as<DisplayObject>(args[0]);
	if (th->overState)
	{
		th->overState->incRef();
		th->overState->addStoredMember();
	}
	th->reflectState(th->currentState);
}

ASFUNCTIONBODY_ATOM(SimpleButton,_getDownState)
{
	SimpleButton* th=asAtomHandler::as<SimpleButton>(obj);
	if(!th->downState)
	{
		asAtomHandler::setNull(ret);
		return;
	}

	th->downState->incRef();
	ret = asAtomHandler::fromObjectNoPrimitive(th->downState);
}

ASFUNCTIONBODY_ATOM(SimpleButton,_setDownState)
{
	assert_and_throw(argslen == 1);
	SimpleButton* th=asAtomHandler::as<SimpleButton>(obj);
	if(th->downState)
		th->downState->removeStoredMember();
	th->downState = asAtomHandler::as<DisplayObject>(args[0]);
	if (th->downState)
	{
		th->downState->incRef();
		th->downState->addStoredMember();
	}
	th->reflectState(th->currentState);
}

ASFUNCTIONBODY_ATOM(SimpleButton,_setEnabled)
{
	SimpleButton* th=asAtomHandler::as<SimpleButton>(obj);
	assert_and_throw(argslen==1);
	th->enabled=asAtomHandler::Boolean_concrete(args[0]);
}

ASFUNCTIONBODY_ATOM(SimpleButton,_getEnabled)
{
	SimpleButton* th=asAtomHandler::as<SimpleButton>(obj);
	asAtomHandler::setBool(ret,th->enabled);
}

ASFUNCTIONBODY_ATOM(SimpleButton,_setUseHandCursor)
{
	SimpleButton* th=asAtomHandler::as<SimpleButton>(obj);
	assert_and_throw(argslen==1);
	th->useHandCursor=asAtomHandler::Boolean_concrete(args[0]);
	th->handleMouseCursor(th->hasMouse);
}

ASFUNCTIONBODY_ATOM(SimpleButton,_getUseHandCursor)
{
	SimpleButton* th=asAtomHandler::as<SimpleButton>(obj);
	asAtomHandler::setBool(ret,th->useHandCursor);
}
