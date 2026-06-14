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

_NR<DisplayObject> SimpleButton::hitTestImpl(const Vector2f& globalPoint, const Vector2f& localPoint, HIT_TYPE type,bool interactiveObjectsOnly)
{
	_NR<DisplayObject> ret = NullRef;

	BUTTONOBJECTTYPE checkstate = type==MOUSE_CLICK_HIT || type==DOUBLE_CLICK_HIT ? BUTTONOBJECTTYPE_HIT : BUTTONOBJECTTYPE_UP;
	DisplayObject* hitTestState = parentSprite[checkstate];
	if(hitTestState)
	{
		if(!hitTestState->getMatrix().isInvertible())
			return NullRef;

		const auto hitPoint = hitTestState->getMatrix().getInverted().multiply2D(localPoint);
		ret = hitTestState->hitTest(globalPoint, hitPoint, type,false);
	}
	else
	{
		for (auto it = states[checkstate].begin(); it != states[checkstate].end(); it++)
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
	if(ret && (type==MOUSE_CLICK_HIT || type==DOUBLE_CLICK_HIT))
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
		if (i==BUTTONOBJECTTYPE_HIT)
			continue;
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
		for (auto it = states[i].begin(); it != states[i].end(); it++)
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
	if (state == BUTTONOBJECTTYPE_UP && o->is<MovieClip>())
		upStateHasMovieClip=true;
}

SimpleButton::SimpleButton(ASWorker* wrk, Class_base* c, DefineButtonTag *tag)
	: DisplayObjectContainer(wrk,c)
	,parentSprite{nullptr,nullptr,nullptr,nullptr}
	,buttontag(tag)
	,statesdirty(true)
	,currentState(STATE_OUT)
	,oldstate(STATE_OUT)
	,enabled(true)
	,useHandCursor(true)
	,hasMouse(false)
	,upStateHasMovieClip(false)
{
	subtype = SUBTYPE_SIMPLEBUTTON;
	if (tag)
		this->loadedFrom = tag->loadedFrom;
	if (!needsActionScript3())
		handleConstruction();
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
	for (uint32_t i=0; i<4; i++)
	{
		clearStateObject((BUTTONOBJECTTYPE)i);
	}
}

bool SimpleButton::destruct()
{
	for (uint32_t i=0; i<4; i++)
		clearStateObject((BUTTONOBJECTTYPE)i);
	enabled=true;
	useHandCursor=true;
	hasMouse=false;
	upStateHasMovieClip=false;
	buttontag=nullptr;
	statesdirty=true;
	return DisplayObjectContainer::destruct();
}

void SimpleButton::prepareShutdown()
{
	if (preparedforshutdown)
		return;
	for (uint32_t i=0; i<4; i++)
	{
		while (!states[i].empty())
		{
			DisplayObject* d = states[i].back().second;
			states[i].pop_back();
			d->prepareShutdown();
			d->removeStoredMember();
		}
		if (parentSprite[i])
		{
			DisplayObject* d = parentSprite[i];
			parentSprite[i]=nullptr;
			d->prepareShutdown();
			d->removeStoredMember();
		}
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
	DisplayObjectContainer::prepareShutdown();
}

bool SimpleButton::countCylicMemberReferences(garbagecollectorstate& gcstate)
{
	bool ret = DisplayObjectContainer::countCylicMemberReferences(gcstate);
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
	q->addToInvalidateQueue(this);
}

uint32_t SimpleButton::getTagID() const
{
	return buttontag ? uint32_t(buttontag->getId()) : 0;
}

void SimpleButton::beforeConstruction(bool _explicit)
{
	DisplayObjectContainer::beforeConstruction(_explicit);
	if (needsActionScript3()
		&& this->buttontag
		&& this->upStateHasMovieClip
		)
	{
		// for some reason adobe seems to execute some kind of "inner goto" handling before and after calling the builtin constructor of the button
		// if the button has a MovieClip in its "up" state
		// see ruffle test avm2/button_nested_frame

		setNameOnParent();
		getSystemState()->handleBroadcastEvent("frameConstructed");
		getSystemState()->stage->executeFrameScript();
		getSystemState()->handleBroadcastEvent("exitFrame");
	}
}

void SimpleButton::afterConstruction(bool _explicit)
{
	if (needsActionScript3()
		&& this->buttontag
		&& this->upStateHasMovieClip
		)
	{
		// for some reason adobe seems to execute some kind of "inner goto" handling before and after calling the builtin constructor of the button
		// if the button has a MovieClip in its "up" state
		// see ruffle test avm2/button_nested_frame
		getSystemState()->stage->initFrame();
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
	this->_removeAllChildren(false,false);

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
			parentSprite[type]->as<Sprite>()->isInaccessibleParent=true;
			parentSprite[type]->addStoredMember();
			parentSprite[type]->loadedFrom=this->loadedFrom;
			parentSprite[type]->constructionComplete(true);
			parentSprite[type]->afterConstruction(true);
			parentSprite[type]->name = UINT32_MAX;
		}
		else if (parentSprite[type]->is<DisplayObjectContainer>())
		{
			parentSprite[type]->as<DisplayObjectContainer>()->_removeAllChildren(false,false);
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
		if (obj->is<SimpleButton>())
		{
			obj->as<SimpleButton>()->currentState=this->currentState;
			obj->as<SimpleButton>()->reflectState(currentState);
		}
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
