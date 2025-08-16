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
#include "scripting/flash/display/Stage.h"
#include "scripting/flash/system/ApplicationDomain.h"
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
	if (!needsActionScript3())
	{
		getSystemState()->stage->AVM1AddKeyboardListener(asAtomHandler::fromObjectNoPrimitive(this));
		getSystemState()->stage->AVM1AddMouseListener(asAtomHandler::fromObjectNoPrimitive(this));
	}
	if (lastParent)
		lastParent->removeStoredMember();
	// keep track of the DisplayObjectContainer this was added to
	// this is needed to handle AVM1 mouse events, as they might be executed after this button was removed from stage
	lastParent = getParent();
	if (lastParent)
	{
		lastParent->incRef();
		lastParent->addStoredMember();
	}
	DisplayObjectContainer::afterLegacyInsert();
}

bool SimpleButton::AVM1HandleMouseEvent(EventDispatcher* dispatcher, MouseEvent *e)
{
	if (this->loadedFrom->usesActionScript3)
		return false;
	if (!dispatcher->is<DisplayObject>())
		return false;
	oldstate = currentState;
	bool CondIdleToOverDown = false;
	bool CondOutDownToIdle = false;
	bool CondOutDownToOverDown = false;
	bool CondOverDownToOutDown = false;
	bool CondOverDownToOverUp = false;
	bool CondOverUpToIdle = false;
	bool CondIdleToOverUp = false;
	bool CondOverDownToIdle = false;

	if(e->type == "mouseDown")
	{
		if(dispatcher==this)
		{
			currentState = STATE_DOWN;
			reflectState(oldstate);
		}
		else
		{
			number_t xmin, xmax, ymin, ymax;
			if (this->boundsRectGlobal(xmin, xmax, ymin, ymax))
			{
				if (xmin<=e->stageX &&
						xmax>=e->stageX &&
						ymin<=e->stageY &&
						ymax>=e->stageY)
				{
					currentState = STATE_DOWN;
					reflectState(oldstate);
				}
			}
		}
	}
	else if(e->type == "mouseUp")
	{
		CondOverDownToOverUp = oldstate!=STATE_UP  && dispatcher == this;
		currentState = STATE_UP;
		reflectState(oldstate);
	}
	else if(e->type == "mouseOver")
	{
		CondIdleToOverDown = oldstate == STATE_OUT && e->buttonDown && dispatcher != this;
		CondOutDownToOverDown = oldstate == STATE_DOWN && e->buttonDown  && dispatcher != this;
		CondIdleToOverUp = oldstate==STATE_OUT && !e->buttonDown && dispatcher == this;
		CondOverDownToIdle = oldstate==STATE_DOWN && !e->buttonDown && dispatcher != this;
		if (dispatcher == this)
		{
			currentState = STATE_OVER;
			reflectState(oldstate);
		}
	}
	else if(e->type == "mouseOut" && dispatcher == this)
	{
		CondOverDownToOutDown = oldstate!=STATE_OUT && e->buttonDown;
		CondOverUpToIdle = (oldstate==STATE_UP || oldstate==STATE_OVER) && !e->buttonDown;
		if (!e->buttonDown)
		{
			currentState = STATE_OUT;
			reflectState(oldstate);
		}
	}
	else if(e->type == "releaseOutside" && dispatcher == this)
	{
		CondOutDownToIdle = oldstate!=STATE_OUT;
		currentState = STATE_OUT;
		reflectState(oldstate);
	}
	else
		return false;
	if (buttontag)
	{
		for (auto it = buttontag->condactions.begin(); it != buttontag->condactions.end(); it++)
		{
			if (  (it->CondIdleToOverDown && CondIdleToOverDown)
				||(it->CondOutDownToIdle && CondOutDownToIdle)
				||(it->CondOutDownToOverDown && CondOutDownToOverDown)
				||(it->CondOverDownToOutDown && CondOverDownToOutDown)
				||(it->CondOverDownToOverUp && CondOverDownToOverUp)
				||(it->CondOverUpToIdle && CondOverUpToIdle)
				||(it->CondIdleToOverUp && CondIdleToOverUp)
				||(it->CondOverDownToIdle && CondOverDownToIdle)
				)
			{
				DisplayObjectContainer* c = lastParent;
				while (c && !c->is<MovieClip>())
					c = c->getParent();
				if (c)
				{
					asAtom obj = asAtomHandler::fromObjectNoPrimitive(this->getParent());
					ACTIONRECORD::executeActions(c->as<MovieClip>(),c->as<MovieClip>()->AVM1getCurrentFrameContext(),it->actions,it->startactionpos,nullptr,false,nullptr,&obj);
				}
			}
		}
	}
	return AVM1HandleMouseEventStandard(dispatcher,e);
}
void SimpleButton::AVM1HandlePressedEvent(ASObject* dispatcher)
{
	if (buttontag)
	{
		for (auto it = buttontag->condactions.begin(); it != buttontag->condactions.end(); it++)
		{
			if ((it->CondOverUpToOverDown && (oldstate==STATE_UP || oldstate==STATE_OVER || oldstate==STATE_OUT) && currentState==STATE_DOWN)
				)
			{
				DisplayObjectContainer* c = lastParent;
				while (c && !c->is<MovieClip>())
					c = c->getParent();
				if (c)
				{
					asAtom obj = asAtomHandler::fromObjectNoPrimitive(this->getParent());
					ACTIONRECORD::executeActions(c->as<MovieClip>(),c->as<MovieClip>()->AVM1getCurrentFrameContext(),it->actions,it->startactionpos,nullptr,false,nullptr,&obj);
				}
			}
		}
	}
	DisplayObjectContainer::AVM1HandlePressedEvent(dispatcher);
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
	bool handled = false;
	if (getSystemState()->stage->getFocusTarget()== this)
		handled = DisplayObjectContainer::AVM1HandleKeyboardEvent(e);
	if (e->type!="keyDown")
		return handled;
	if (getSystemState()->stage->getFocusTarget()!= this)
		return false;
	for (auto it = this->buttontag->condactions.begin(); it != this->buttontag->condactions.end(); it++)
	{
		bool execute=false;
		AS3KeyCode code = e->getKeyCode();
		if (e->getModifiers() & LSModifier::Shift)
		{
			switch (it->CondKeyPress)
			{
				case 33:// !
					execute = code==AS3KEYCODE_NUMBER_1;break;
				case 34:// "
					execute = code==AS3KEYCODE_QUOTE;break;
				case 35:// #
					execute = code==AS3KEYCODE_NUMBER_3;break;
				case 36:// $
					execute = code==AS3KEYCODE_NUMBER_4;break;
				case 37:// %
					execute = code==AS3KEYCODE_NUMBER_5;break;
				case 38:// &
					execute = code==AS3KEYCODE_NUMBER_7;break;
				case 40:// (
					execute = code==AS3KEYCODE_NUMBER_9;break;
				case 41:// )
					execute = code==AS3KEYCODE_NUMBER_0;break;
				case 42:// *
					execute = code==AS3KEYCODE_NUMBER_8;break;
				case 43:// +
					execute = code==AS3KEYCODE_EQUAL;break;
				case 58:// :
					execute = code==AS3KEYCODE_SEMICOLON;break;
				case 60:// <
					execute = code==AS3KEYCODE_COMMA;break;
				case 62:// >
					execute = code==AS3KEYCODE_PERIOD;break;
				case 63:// ?
					execute = code==AS3KEYCODE_SLASH;break;
				case 64:// @
					execute = code==AS3KEYCODE_NUMBER_2;break;
				case 94:// ^
					execute = code==AS3KEYCODE_NUMBER_6;break;
				case 95:// _
					execute = code==AS3KEYCODE_MINUS;break;
				case 123:// {
					execute = code==AS3KEYCODE_LEFTBRACKET;break;
				case 124:// |
					execute = code==AS3KEYCODE_BACKSLASH;break;
				case 125:// }
					execute = code==AS3KEYCODE_RIGHTBRACKET;break;
				case 126:// ~
					execute = code==AS3KEYCODE_BACKQUOTE;break;
				default:// A-Z
					execute = it->CondKeyPress>=65
							  && it->CondKeyPress<=90
							  && (uint32_t)code-(uint32_t)AS3KEYCODE_A==it->CondKeyPress-65;
					break;
			}
		}
		else
		{
			switch (it->CondKeyPress)
			{
				case 1:
					execute = code==AS3KEYCODE_LEFT;break;
				case 2:
					execute = code==AS3KEYCODE_RIGHT;break;
				case 3:
					execute = code==AS3KEYCODE_HOME;break;
				case 4:
					execute = code==AS3KEYCODE_END;break;
				case 5:
					execute = code==AS3KEYCODE_INSERT;break;
				case 6:
					execute = code==AS3KEYCODE_DELETE;break;
				case 8:
					execute = code==AS3KEYCODE_BACKSPACE;break;
				case 13:
					execute = code==AS3KEYCODE_ENTER;break;
				case 14:
					execute = code==AS3KEYCODE_UP;break;
				case 15:
					execute = code==AS3KEYCODE_DOWN;break;
				case 16:
					execute = code==AS3KEYCODE_PAGE_UP;break;
				case 17:
					execute = code==AS3KEYCODE_PAGE_DOWN;break;
				case 18:
					execute = code==AS3KEYCODE_TAB;break;
				case 19:
					execute = code==AS3KEYCODE_ESCAPE;break;
				case 32:
					execute = code==AS3KEYCODE_SPACE;break;
				case 39:// '
					execute = code==AS3KEYCODE_QUOTE;break;
				case 44:// ,
					execute = code==AS3KEYCODE_COMMA;break;
				case 45:// -
					execute = code==AS3KEYCODE_MINUS;break;
				case 46:// .
					execute = code==AS3KEYCODE_PERIOD;break;
				case 47:// /
					execute = code==AS3KEYCODE_SLASH;break;
				case 48:// 0
					execute = code==AS3KEYCODE_NUMBER_0;break;
				case 49:// 1
					execute = code==AS3KEYCODE_NUMBER_1;break;
				case 50:// 2
					execute = code==AS3KEYCODE_NUMBER_2;break;
				case 51:// 3
					execute = code==AS3KEYCODE_NUMBER_3;break;
				case 52:// 4
					execute = code==AS3KEYCODE_NUMBER_4;break;
				case 53:// 5
					execute = code==AS3KEYCODE_NUMBER_5;break;
				case 54:// 6
					execute = code==AS3KEYCODE_NUMBER_6;break;
				case 55:// 7
					execute = code==AS3KEYCODE_NUMBER_7;break;
				case 56:// 8
					execute = code==AS3KEYCODE_NUMBER_8;break;
				case 57:// 9
					execute = code==AS3KEYCODE_NUMBER_9;break;
				case 59:// ;
					execute = code==AS3KEYCODE_SEMICOLON;break;
				case 61:// =
					execute = code==AS3KEYCODE_EQUAL;break;
				case 91:// [
					execute = code==AS3KEYCODE_LEFTBRACKET;break;
				case 92://
					execute = code==AS3KEYCODE_BACKSLASH;break;
				case 93:// ]
					execute = code==AS3KEYCODE_RIGHTBRACKET;break;
				case 96:// `
					execute = code==AS3KEYCODE_BACKQUOTE;break;
				default:// a-z
					execute = it->CondKeyPress>=97
							  && it->CondKeyPress<=122
							  && (uint32_t)code-(uint32_t)AS3KEYCODE_A==it->CondKeyPress-97;
					break;
			}
		}
		if (execute)
		{
			DisplayObjectContainer* c = lastParent;
			while (c && !c->is<MovieClip>())
				c = c->getParent();
			if (c)
				ACTIONRECORD::executeActions(c->as<MovieClip>(),c->as<MovieClip>()->AVM1getCurrentFrameContext(),it->actions,it->startactionpos);
		}
	}
	return handled;
}


_NR<DisplayObject> SimpleButton::hitTestImpl(const Vector2f& globalPoint, const Vector2f& localPoint, HIT_TYPE type,bool interactiveObjectsOnly)
{
	_NR<DisplayObject> ret = NullRef;
	DisplayObject* hitTestState = parentSprite[BUTTONOBJECTTYPE_HIT];
	if(hitTestState)
	{
		if(!hitTestState->getMatrix().isInvertible())
			return NullRef;

		const auto hitPoint = hitTestState->getMatrix().getInverted().multiply2D(localPoint);
		ret = hitTestState->hitTest(globalPoint, hitPoint, type,false);
	}
	else
	{
		for (auto it = states[BUTTONOBJECTTYPE_HIT].begin(); it != states[BUTTONOBJECTTYPE_HIT].end(); it++)
		{
			hitTestState = (*it).second;
			if(!hitTestState->getMatrix().isInvertible())
				return NullRef;

			const auto hitPoint = hitTestState->getMatrix().getInverted().multiply2D(localPoint);
			ret = hitTestState->hitTest(globalPoint, hitPoint, type,false);
			if (ret)
				break;
		}
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
	if (e->is<MouseEvent>())
	{
		if(e->type == "mouseDown")
			currentState = STATE_DOWN;
		else if(e->type == "releaseOutside")
			currentState = STATE_UP;
		else if(e->type == "rollOver" || e->type == "mouseOver" || e->type == "mouseUp")
			currentState = STATE_OVER;
		else if((e->type == "rollOut" || e->type == "mouseOut") && !e->as<MouseEvent>()->buttonDown)
			currentState = STATE_OUT;
		else
			is_valid = false;
	}
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
	for (uint32_t i=0; i<4; i++)
	{
		if (parentSprite[i] && parentSprite[i]->getBounds(txmin,txmax,tymin,tymax,parentSprite[i]->getMatrix()))
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
		for (auto it = states[type].begin(); it != states[type].end(); it++)
		{
			if ((*it).second->getBounds(txmin,txmax,tymin,tymax,(*it).second->getMatrix()))
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
		}
	}
	return ret;
}
void SimpleButton::addDisplayObject(BUTTONOBJECTTYPE state, uint32_t depth, DisplayObject* o)
{
	o->addStoredMember();
	states[state].push_back(make_pair(depth,o));
}

SimpleButton::SimpleButton(ASWorker* wrk, Class_base* c, DefineButtonTag *tag)
	: DisplayObjectContainer(wrk,c), lastParent(nullptr),parentSprite{nullptr,nullptr,nullptr,nullptr},
	buttontag(tag),statesdirty(true),
	currentState(STATE_OUT),oldstate(STATE_OUT),enabled(true),useHandCursor(true),hasMouse(false)
{
	subtype = SUBTYPE_SIMPLEBUTTON;
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
	tabEnabled = asAtomHandler::trueAtom;
}

void SimpleButton::finalize()
{
	DisplayObjectContainer::finalize();
	if (lastParent)
		lastParent->removeStoredMember();
	lastParent=nullptr;

	for (uint32_t i=0; i<4; i++)
	{
		clearStateObject((BUTTONOBJECTTYPE)i);
	}
}

bool SimpleButton::destruct()
{
	if (lastParent)
		lastParent->removeStoredMember();
	lastParent=nullptr;
	for (uint32_t i=0; i<4; i++)
		clearStateObject((BUTTONOBJECTTYPE)i);
	enabled=true;
	useHandCursor=true;
	hasMouse=false;
	buttontag=nullptr;
	statesdirty=true;
	return DisplayObjectContainer::destruct();
}

void SimpleButton::prepareShutdown()
{
	if (preparedforshutdown)
		return;
	DisplayObjectContainer::prepareShutdown();
	if (lastParent)
		lastParent->prepareShutdown();
	for (uint32_t i=0; i<4; i++)
	{
		for (auto it = states[i].begin(); it != states[i].end(); it++)
		{
			(*it).second->prepareShutdown();
		}
		if (parentSprite[i])
			parentSprite[i]->prepareShutdown();
	}
	if(soundchannel_OverUpToIdle)
		soundchannel_OverUpToIdle->prepareShutdown();
	if(soundchannel_IdleToOverUp)
		soundchannel_IdleToOverUp->prepareShutdown();
	if(soundchannel_OverUpToOverDown)
		soundchannel_OverUpToOverDown->prepareShutdown();
	if(soundchannel_OverDownToOverUp)
		soundchannel_OverDownToOverUp->prepareShutdown();
	buttontag=nullptr;
}

bool SimpleButton::countCylicMemberReferences(garbagecollectorstate& gcstate)
{
	bool ret = DisplayObjectContainer::countCylicMemberReferences(gcstate);
	if (lastParent)
		ret = lastParent->countAllCylicMemberReferences(gcstate) || ret;
	for (uint32_t i=0; i<4; i++)
	{
		for (auto it = states[i].begin(); it != states[i].end(); it++)
			ret = (*it).second->countAllCylicMemberReferences(gcstate) || ret;
		if (parentSprite[i])
			ret = parentSprite[i]->countAllCylicMemberReferences(gcstate) || ret;
	}
	return ret;

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

void SimpleButton::beforeConstruction(bool _explicit)
{
	if (needsActionScript3()
			&& this->buttontag
			&& this->buttontag->bindedTo)
	{
		// for some reason adobe seems to execute some kind of "inner goto" handling _before_ calling the builtin constructor of the button
		// if the button is binded to a class
		// see ruffle test avm2/button_nested_frame
		getSystemState()->stage->initFrame();
		getSystemState()->handleBroadcastEvent("frameConstructed");
		setNameOnParent();
		getSystemState()->stage->executeFrameScript();
		getSystemState()->handleBroadcastEvent("exitFrame");
	}
	DisplayObjectContainer::beforeConstruction(_explicit);
}

void SimpleButton::constructionComplete(bool _explicit, bool forInitAction)
{
	DisplayObjectContainer::constructionComplete(_explicit,forInitAction);

	if (needsActionScript3()
			&& this->buttontag
			&& !this->buttontag->bindedTo
			&& this->states[STATE_UP].size()>1)
	{
		// for some reason adobe seems to execute some kind of "inner goto" handling _after_ calling the builtin constructor of the button
		// if the button is _not_ binded to a class and the up state consist of more than one sprite
		// see ruffle test avm2/button_nested_frame
		getSystemState()->stage->initFrame();
		getSystemState()->handleBroadcastEvent("frameConstructed");
		getSystemState()->stage->executeFrameScript();
		getSystemState()->handleBroadcastEvent("exitFrame");
	}
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
		upState->incRef();
		th->addDisplayObject(BUTTONOBJECTTYPE_UP,0,upState.getPtr());
	}
	if (!downState.isNull())
	{
		downState->incRef();
		th->addDisplayObject(BUTTONOBJECTTYPE_DOWN,0,downState.getPtr());
	}
	if (!hitTestState.isNull())
	{
		hitTestState->incRef();
		th->addDisplayObject(BUTTONOBJECTTYPE_HIT,0,hitTestState.getPtr());
	}
	if (!overState.isNull())
	{
		overState->incRef();
		th->addDisplayObject(BUTTONOBJECTTYPE_OVER,0,overState.getPtr());
	}
}

void SimpleButton::reflectState(BUTTONSTATE oldstate)
{
	if (!statesdirty && isConstructed() && oldstate==currentState)
		return;
	this->_removeAllChildren();

	if((currentState == STATE_UP || currentState == STATE_OUT))
		resetStateToStart(BUTTONOBJECTTYPE_UP);
	else if(currentState == STATE_DOWN)
		resetStateToStart(BUTTONOBJECTTYPE_DOWN);
	else if(currentState == STATE_OVER)
		resetStateToStart(BUTTONOBJECTTYPE_OVER);
	if ((oldstate == STATE_OVER || oldstate == STATE_UP) && currentState == STATE_OUT && soundchannel_OverUpToIdle)
		soundchannel_OverUpToIdle->play();
	if (oldstate == STATE_OUT && (currentState == STATE_OVER || currentState == STATE_UP) && soundchannel_IdleToOverUp)
		soundchannel_IdleToOverUp->play();
	if ((oldstate == STATE_OVER || oldstate == STATE_UP) && currentState == STATE_DOWN && soundchannel_OverUpToOverDown)
		soundchannel_OverUpToOverDown->play();
	if (oldstate == STATE_DOWN && currentState == STATE_UP && soundchannel_OverDownToOverUp)
		soundchannel_OverDownToOverUp->play();
	statesdirty=false;
}
void SimpleButton::resetStateToStart(BUTTONOBJECTTYPE type)
{
	// reset the MovieClips belonging to the current State to frame 0, so animations will start from the beginning when state has changed
	assert(this->dynamicDisplayList.empty());
	if(this->needsActionScript3())
	{
		if (!parentSprite[type])
		{
			/* There maybe multiple DisplayObjects for one state. The official
			 * implementation for AS3 seems to create a Sprite in that case with
			 * all DisplayObjects as children.
			 */
			parentSprite[type] = Class<Sprite>::getInstanceSNoArgs(loadedFrom->getInstanceWorker());
			parentSprite[type]->addStoredMember();
			parentSprite[type]->loadedFrom=this->loadedFrom;
			parentSprite[type]->constructionComplete(true);
			parentSprite[type]->afterConstruction(true);
			parentSprite[type]->name = UINT32_MAX;
		}
		else if (parentSprite[type]->is<DisplayObjectContainer>())
		{
			parentSprite[type]->as<DisplayObjectContainer>()->_removeAllChildren();
		}
	}
	for (auto it = states[type].begin(); it != states[type].end(); it++)
	{
		DisplayObject* obj = (*it).second;
		if (obj->is<MovieClip>())
		{
			obj->as<MovieClip>()->state.next_FP=0;
			obj->as<MovieClip>()->state.stop_FP=false;
		}
		obj->incRef();
		if (this->needsActionScript3() && parentSprite[type]->is<DisplayObjectContainer>())
		{
			parentSprite[type]->incRef();
			parentSprite[type]->as<DisplayObjectContainer>()->insertLegacyChildAt((*it).first,obj);
			insertLegacyChildAt((*it).first,parentSprite[type],false,false);
		}
		else
			insertLegacyChildAt((*it).first,obj);
	}
}
void SimpleButton::clearStateObject(BUTTONOBJECTTYPE type)
{
	if(parentSprite[type])
	{
		parentSprite[type]->removeStoredMember();
		parentSprite[type]=nullptr;
	}
	for (auto it = states[type].begin(); it != states[type].end(); it++)
	{
		(*it).second->removeStoredMember();
	}
	states[type].clear();
}
void SimpleButton::getStateObject(BUTTONOBJECTTYPE type,asAtom& ret)
{
	assert(this->needsActionScript3());
	if(parentSprite[type]==nullptr)
		ret = asAtomHandler::nullAtom;
	else
	{
		parentSprite[type]->incRef();
		ret = asAtomHandler::fromObjectNoPrimitive(parentSprite[type]);
	}
}
void SimpleButton::setStateObject(BUTTONOBJECTTYPE type,asAtom o)
{
	assert(this->needsActionScript3());
	clearStateObject(type);
	if (asAtomHandler::is<DisplayObject>(o))
	{
		parentSprite[type] = asAtomHandler::as<DisplayObject>(o);
		parentSprite[type]->incRef();
		parentSprite[type]->addStoredMember();
	}
}
ASFUNCTIONBODY_ATOM(SimpleButton,_getUpState)
{
	SimpleButton* th=asAtomHandler::as<SimpleButton>(obj);
	th->getStateObject(BUTTONOBJECTTYPE_UP,ret);
}

ASFUNCTIONBODY_ATOM(SimpleButton,_setUpState)
{
	assert_and_throw(argslen == 1);
	SimpleButton* th=asAtomHandler::as<SimpleButton>(obj);
	th->setStateObject(BUTTONOBJECTTYPE_UP,args[0]);
	th->reflectState(th->currentState);
}

ASFUNCTIONBODY_ATOM(SimpleButton,_getHitTestState)
{
	SimpleButton* th=asAtomHandler::as<SimpleButton>(obj);
	th->getStateObject(BUTTONOBJECTTYPE_HIT,ret);
}

ASFUNCTIONBODY_ATOM(SimpleButton,_setHitTestState)
{
	assert_and_throw(argslen == 1);
	SimpleButton* th=asAtomHandler::as<SimpleButton>(obj);
	th->setStateObject(BUTTONOBJECTTYPE_HIT,args[0]);
}

ASFUNCTIONBODY_ATOM(SimpleButton,_getOverState)
{
	SimpleButton* th=asAtomHandler::as<SimpleButton>(obj);
	th->getStateObject(BUTTONOBJECTTYPE_OVER,ret);
}

ASFUNCTIONBODY_ATOM(SimpleButton,_setOverState)
{
	assert_and_throw(argslen == 1);
	SimpleButton* th=asAtomHandler::as<SimpleButton>(obj);
	th->setStateObject(BUTTONOBJECTTYPE_OVER,args[0]);
	th->reflectState(th->currentState);
}

ASFUNCTIONBODY_ATOM(SimpleButton,_getDownState)
{
	SimpleButton* th=asAtomHandler::as<SimpleButton>(obj);
	th->getStateObject(BUTTONOBJECTTYPE_DOWN,ret);
}

ASFUNCTIONBODY_ATOM(SimpleButton,_setDownState)
{
	assert_and_throw(argslen == 1);
	SimpleButton* th=asAtomHandler::as<SimpleButton>(obj);
	th->setStateObject(BUTTONOBJECTTYPE_DOWN,args[0]);
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
