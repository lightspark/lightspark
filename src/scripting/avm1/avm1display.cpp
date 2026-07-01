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

#include "backends/cachedsurface.h"
#include "scripting/avm1/avm1display.h"
#include "scripting/avm1/avm1key.h"
#include "scripting/avm1/avm1array.h"
#include "scripting/flash/display/LoaderInfo.h"
#include "scripting/flash/display/Loader.h"
#include "scripting/flash/display/RootMovieClip.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include "scripting/flash/geom/flashgeom.h"
#include "scripting/flash/geom/Rectangle.h"
#include "scripting/flash/net/flashnet.h"
#include "scripting/toplevel/toplevel.h"
#include "scripting/toplevel/AVM1Function.h"
#include "scripting/toplevel/Array.h"
#include "scripting/toplevel/UInteger.h"
#include "scripting/toplevel/Integer.h"
#include "parsing/tags.h"
#include "scripting/abc.h"
#include "backends/security.h"
#include "backends/input.h"

using namespace std;
using namespace lightspark;

void AVM1MovieClip::afterConstruction(bool _explicit)
{
	MovieClip::afterConstruction(_explicit);
}

void AVM1MovieClip::finalize()
{
	MovieClip::finalize();
	if (droptarget)
		droptarget->removeStoredMember();
	droptarget=nullptr;
	color.reset();
}

bool AVM1MovieClip::destruct()
{
	if (droptarget)
		droptarget->removeStoredMember();
	droptarget=nullptr;
	color.reset();
	focusEnabled=asAtomHandler::undefinedAtom;
	return MovieClip::destruct();
}

void AVM1MovieClip::prepareShutdown()
{
	if (preparedforshutdown)
		return;
	MovieClip::prepareShutdown();
	if (color)
		color->prepareShutdown();
	if (droptarget)
		droptarget->prepareShutdown();
}
bool AVM1MovieClip::countCylicMemberReferences(garbagecollectorstate& gcstate)
{
	if (skipCountCylicMemberReferences(gcstate))
		return gcstate.hasMember(this);
	bool ret = MovieClip::countCylicMemberReferences(gcstate);
	if (droptarget)
		ret = droptarget->countAllCylicMemberReferences(gcstate) || ret;
	return ret;
}

void AVM1MovieClip::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_DYNAMIC_NOT_FINAL);
	MovieClip::AVM1SetupMethods(c);
	c->prototype->setDeclaredMethodByQName("_totalframes","",c->getSystemState()->getBuiltinFunction(_getTotalFrames),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("_currentframe","",c->getSystemState()->getBuiltinFunction(_getCurrentFrame),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("_framesloaded","",c->getSystemState()->getBuiltinFunction(_getFramesLoaded),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("_name","",c->getSystemState()->getBuiltinFunction(_getter_name),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("_name","",c->getSystemState()->getBuiltinFunction(_setter_name),SETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("startDrag","",c->getSystemState()->getBuiltinFunction(startDrag),NORMAL_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("stopDrag","",c->getSystemState()->getBuiltinFunction(stopDrag),NORMAL_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("attachAudio","",c->getSystemState()->getBuiltinFunction(attachAudio),NORMAL_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("focusEnabled","",c->getSystemState()->getBuiltinFunction(getFocusEnabled),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("focusEnabled","",c->getSystemState()->getBuiltinFunction(setFocusEnabled),SETTER_METHOD,false,false);
}

void AVM1MovieClip::setColor(AVM1Color* c)
{
	c->incRef();
	this->color = _MNR(c);
}

bool AVM1MovieClip::isFocusable(bool fromMouse)
{
	if (fromMouse)
		return false;
	else if (this->isVisible()
			   && asAtomHandler::AVM1toBool(focusEnabled,getInstanceWorker(),getInstanceWorker()->AVM1getSwfVersion()))
		return true;
	else if (this->AVM1hasListeners())
		return (asAtomHandler::isUndefined(tabEnabled)
				|| asAtomHandler::AVM1toBool(tabEnabled,getInstanceWorker(),getInstanceWorker()->AVM1getSwfVersion()));
	else
		return asAtomHandler::AVM1toBool(tabEnabled,getInstanceWorker(),getInstanceWorker()->AVM1getSwfVersion());
}

ASFUNCTIONBODY_ATOM(AVM1MovieClip,startDrag)
{
	bool lockcenter=false;
	if (argslen > 0)
		lockcenter = asAtomHandler::AVM1toNumber(args[0],wrk->AVM1getSwfVersion());

	Rectangle* rect = nullptr;
	if (argslen > 4)
	{
		rect = Class<Rectangle>::getInstanceS(wrk);
		number_t x1=0, y1=0, x2=0, y2=0;
		x1 = asAtomHandler::AVM1toNumber(args[1],wrk->AVM1getSwfVersion());
		y1 = asAtomHandler::AVM1toNumber(args[2],wrk->AVM1getSwfVersion());
		x2 = asAtomHandler::AVM1toNumber(args[3],wrk->AVM1getSwfVersion());
		y2 = asAtomHandler::AVM1toNumber(args[4],wrk->AVM1getSwfVersion());
		rect->x = x1;
		rect->y = y1;
		rect->width = x2-x1;
		rect->height = y2-y1;
	}
	asAtom fargs[2];
	fargs[0] = asAtomHandler::fromBool(lockcenter);
	if (rect)
		fargs[1] = asAtomHandler::fromObject(rect);
	if (asAtomHandler::is<AVM1MovieClip>(obj))
	{
		AVM1MovieClip* th=asAtomHandler::as<AVM1MovieClip>(obj);
		if (th->droptarget)
		{
			th->droptarget->removeStoredMember();
			th->droptarget=nullptr;
		}
		Sprite::_startDrag(ret,wrk,obj,fargs,rect ? 2 : 1);
	}
	if (rect)
		rect->decRef();
}

ASFUNCTIONBODY_ATOM(AVM1MovieClip,stopDrag)
{
	if (!asAtomHandler::is<DisplayObject>(obj))
		return;
	DisplayObject* dt = wrk->getSystemState()->getInputThread()->stopDrag(asAtomHandler::as<DisplayObject>(obj));
	if (asAtomHandler::is<AVM1MovieClip>(obj))
	{
		AVM1MovieClip* th=asAtomHandler::as<AVM1MovieClip>(obj);
		if (th->droptarget)
			th->droptarget->removeStoredMember();
		th->droptarget = dt;
		if (th->droptarget)
		{
			th->droptarget->incRef();
			th->droptarget->addStoredMember();
		}
	}
}

ASFUNCTIONBODY_ATOM(AVM1MovieClip,attachAudio)
{
//	AVM1MovieClip* th=asAtomHandler::as<AVM1MovieClip>(obj);
	LOG(LOG_NOT_IMPLEMENTED,"AVM1MovieClip.attachAudio");
}

ASFUNCTIONBODY_ATOM(AVM1MovieClip,getFocusEnabled)
{
	ret = asAtomHandler::falseAtom;
	if (asAtomHandler::is<AVM1MovieClip>(obj))
	{
		AVM1MovieClip* th=asAtomHandler::as<AVM1MovieClip>(obj);
		ret = th->focusEnabled;
	}
}
ASFUNCTIONBODY_ATOM(AVM1MovieClip,setFocusEnabled)
{
	if (asAtomHandler::is<AVM1MovieClip>(obj))
	{
		AVM1MovieClip* th=asAtomHandler::as<AVM1MovieClip>(obj);
		if (argslen > 0)
			th->focusEnabled=asAtomHandler::fromBool(asAtomHandler::AVM1toBool(args[0],wrk,wrk->AVM1getSwfVersion()));
	}
}


void AVM1Shape::sinit(Class_base* c)
{
	Shape::sinit(c);
	DisplayObject::AVM1SetupMethods(c);
}

void AVM1SimpleButton::finalize()
{
	if (lastParent)
		lastParent->removeStoredMember();
	lastParent=nullptr;
	SimpleButton::finalize();
}

bool AVM1SimpleButton::destruct()
{
	if (lastParent)
		lastParent->removeStoredMember();
	lastParent=nullptr;
	return SimpleButton::destruct();
}

void AVM1SimpleButton::prepareShutdown()
{
	if (preparedforshutdown)
		return;
	if (lastParent)
	{
		DisplayObject* d = lastParent;
		lastParent=nullptr;
		d->prepareShutdown();
		d->removeStoredMember();
	}
	SimpleButton::prepareShutdown();
}

bool AVM1SimpleButton::countCylicMemberReferences(garbagecollectorstate& gcstate)
{
	bool ret = SimpleButton::countCylicMemberReferences(gcstate);
	if (lastParent)
		ret = lastParent->countAllCylicMemberReferences(gcstate) || ret;
	return ret;
}


ASObject* AVM1SimpleButton::AVM1getClassPrototypeObject() const
{
	// for some reason buttons don't allow access to its prototype on swf6 (see ruffle tests avm1/focusrect_property_swf6)
	if (this->loadedFrom->version < 7)
		return nullptr;
	return SimpleButton::AVM1getClassPrototypeObject();
}
void AVM1SimpleButton::afterConstruction(bool _explicit)
{
	asAtom proto = asAtomHandler::fromObject(this->getClass()->prototype->getObj());
	setprop_prototype(proto,BUILTIN_STRINGS::STRING_PROTO);
	SimpleButton::afterConstruction(_explicit);
}

void AVM1SimpleButton::afterLegacyInsert()
{
	AVM1addOneKeyboardEventListener();
	AVM1addOneMouseEventListener();
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
	SimpleButton::afterLegacyInsert();
}

bool AVM1SimpleButton::AVM1HandleKeyboardEvent(KeyboardEvent *e)
{
	keyPressedHandled=false;
	if (getSystemState()->stage->getFocusTarget()== this)
		return DisplayObjectContainer::AVM1HandleKeyboardEvent(e);
	return false;
}
bool AVM1SimpleButton::AVM1HandleKeyPressedEvent(KeyboardEvent *e)
{
	if (!this->isVisible())
		return false;
	bool handled = false;
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
			handled = true;
			keyPressedHandled = true;
		}
	}
	return handled;
}
bool AVM1SimpleButton::AVM1HandleMouseEvent(EventDispatcher* dispatcher, MouseEvent *e)
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
	if (e->type == "mouseUp")
	{
		if (AVM1isHitByMouseEvent(dispatcher,e))
		{
			asAtom func=asAtomHandler::invalidAtom;
			multiname m(nullptr);
			m.name_type=multiname::NAME_STRING;
			m.isAttribute = false;
			m.name_s_id=BUILTIN_STRINGS::STRING_ONRELEASE;
			AVM1getVariableByMultiname(func,m,GET_VARIABLE_OPTION::NONE,getInstanceWorker());
			if (asAtomHandler::is<AVM1Function>(func))
			{
				asAtom obj = asAtomHandler::fromObject(this);
				asAtomHandler::as<AVM1Function>(func)->call(nullptr,&obj,nullptr,0);
			}
			ASATOM_DECREF(func);
		}
	}
	return false;
}
bool AVM1SimpleButton::AVM1HandlePressedEvent(ASObject* dispobj, bool fromMouse)
{
	if (keyPressedHandled)
		return false;
	if (buttontag)
	{
		for (auto it = buttontag->condactions.begin(); it != buttontag->condactions.end(); it++)
		{
			if (it->CondOverUpToOverDown)
			{
				if (fromMouse && dispobj == this)
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
	}
	if (fromMouse)
		return DisplayObjectContainer::AVM1HandlePressedEvent(dispobj,fromMouse);
	return false;
}
void AVM1SimpleButton::AVM1HandleReleasedEvent(ASObject* dispobj, bool fromMouse)
{
	if (keyPressedHandled)
		return;
	if (buttontag)
	{
		for (auto it = buttontag->condactions.begin(); it != buttontag->condactions.end(); it++)
		{
			if (it->CondOverDownToOverUp)
			{
				if (fromMouse && dispobj == this)
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
	}
	if (fromMouse)
		DisplayObjectContainer::AVM1HandleReleasedEvent(dispobj,fromMouse);
}

void AVM1SimpleButton::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_DYNAMIC_NOT_FINAL);
	c->isSealed = false;
	InteractiveObject::AVM1SetupMethods(c);
	c->prototype->setDeclaredMethodByQName("useHandCursor","",c->getSystemState()->getBuiltinFunction(_getUseHandCursor),GETTER_METHOD,false);
	c->prototype->setDeclaredMethodByQName("useHandCursor","",c->getSystemState()->getBuiltinFunction(_setUseHandCursor),SETTER_METHOD,false);
}
bool AVM1SimpleButton::isFocusable(bool fromMouse)
{
	return !fromMouse && isVisible() && getEnabled() && asAtomHandler::AVM1toBool(tabEnabled,getInstanceWorker(),getInstanceWorker()->AVM1getSwfVersion());
}

void AVM1Stage::sinit(Class_base* c)
{
	// in AVM1 Stage is no DisplayObject and all methods/properties are static
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED);

	c->setVariableAtomByQName("_listeners",nsNameAndKind(),asAtomHandler::fromObjectNoPrimitive(Class<AVM1Array>::getInstanceSNoArgs(c->getInstanceWorker())),CONSTANT_TRAIT);

	c->prototype->setDeclaredMethodByQName("width","",c->getSystemState()->getBuiltinFunction(_getStageWidth),GETTER_METHOD,false);
	c->prototype->setDeclaredMethodByQName("width","",c->getSystemState()->getBuiltinFunction(_setStageWidth),SETTER_METHOD,false);
	c->prototype->setDeclaredMethodByQName("height","",c->getSystemState()->getBuiltinFunction(_getStageHeight),GETTER_METHOD,false);
	c->prototype->setDeclaredMethodByQName("height","",c->getSystemState()->getBuiltinFunction(_setStageHeight),SETTER_METHOD,false);
	c->prototype->setDeclaredMethodByQName("displayState","",c->getSystemState()->getBuiltinFunction(_getDisplayState),GETTER_METHOD,false);
	c->prototype->setDeclaredMethodByQName("displayState","",c->getSystemState()->getBuiltinFunction(_setDisplayState),SETTER_METHOD,false);
	c->prototype->setDeclaredMethodByQName("scaleMode","",c->getSystemState()->getBuiltinFunction(_getScaleMode),GETTER_METHOD,false);
	c->prototype->setDeclaredMethodByQName("scaleMode","",c->getSystemState()->getBuiltinFunction(_setScaleMode),SETTER_METHOD,false);
	c->prototype->setDeclaredMethodByQName("showMenu","",c->getSystemState()->getBuiltinFunction(_getShowMenu),GETTER_METHOD,false);
	c->prototype->setDeclaredMethodByQName("showMenu","",c->getSystemState()->getBuiltinFunction(_setShowMenu),SETTER_METHOD,false);
	c->prototype->setDeclaredMethodByQName("align","",c->getSystemState()->getBuiltinFunction(getAlign),GETTER_METHOD,false);
	c->prototype->setDeclaredMethodByQName("align","",c->getSystemState()->getBuiltinFunction(setAlign),SETTER_METHOD,false);
	c->prototype->setDeclaredMethodByQName("addListener","",c->getSystemState()->getBuiltinFunction(AVM1Broadcaster::addListener),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("removeListener","",c->getSystemState()->getBuiltinFunction(AVM1Broadcaster::removeListener),NORMAL_METHOD,false);
}

ASFUNCTIONBODY_ATOM(AVM1Stage,_getDisplayState)
{
	ret = asAtomHandler::fromString(wrk->getSystemState(),wrk->getSystemState()->stage->displayState);
}
ASFUNCTIONBODY_ATOM(AVM1Stage,_setDisplayState)
{
	ARG_CHECK(ARG_UNPACK(wrk->getSystemState()->stage->displayState));
	wrk->getSystemState()->stage->onDisplayState(wrk->getSystemState()->stage->displayState);
}
ASFUNCTIONBODY_ATOM(AVM1Stage,_getShowMenu)
{
	ret = asAtomHandler::fromBool(wrk->getSystemState()->stage->showDefaultContextMenu);
}
ASFUNCTIONBODY_ATOM(AVM1Stage,_setShowMenu)
{
	ARG_CHECK(ARG_UNPACK(wrk->getSystemState()->stage->showDefaultContextMenu));
}
ASFUNCTIONBODY_ATOM(AVM1Stage,getAlign)
{
	ret = asAtomHandler::fromStringID(wrk->getSystemState()->stage->align);
}
ASFUNCTIONBODY_ATOM(AVM1Stage,setAlign)
{
	uint32_t oldalign=wrk->getSystemState()->stage->align;
	tiny_string newalign;
	ARG_CHECK(ARG_UNPACK(newalign));
	wrk->getSystemState()->stage->align = wrk->getSystemState()->getUniqueStringId(newalign);
	wrk->getSystemState()->stage->onAlign(oldalign);
}


void AVM1MovieClipLoader::addLoader(URLRequest* r, DisplayObject* target,int level)
{
	getSystemState()->stage->AVM1AddEventListener(this);
	loadermutex.lock();
	Loader* ldr = Class<Loader>::getInstanceSNoArgs(getInstanceWorker());
	ldr->constructionComplete();
	ldr->afterConstruction();
	ldr->addStoredMember();
	ldr->loadedFrom=target->loadedFrom;
	ldr->AVM1setup(level,this);
	loaderlist.insert(ldr);
	loadermutex.unlock();
	ldr->loadIntern(r,nullptr,target);
	r->decRef();
}

void AVM1MovieClipLoader::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_DYNAMIC_NOT_FINAL);
	c->isReusable = true;

	c->prototype->setDeclaredMethodByQName("loadClip","",c->getSystemState()->getBuiltinFunction(loadClip),NORMAL_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("addListener","",c->getSystemState()->getBuiltinFunction(AVM1Broadcaster::addListener),NORMAL_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("removeListener","",c->getSystemState()->getBuiltinFunction(AVM1Broadcaster::removeListener),NORMAL_METHOD,false,false);
}
ASFUNCTIONBODY_ATOM(AVM1MovieClipLoader,_constructor)
{
	AVM1MovieClipLoader* th=asAtomHandler::as<AVM1MovieClipLoader>(obj);
	th->setVariableAtomByQName("_listeners",nsNameAndKind(),asAtomHandler::fromObject(Class<AVM1Array>::getInstanceSNoArgs(wrk)),DYNAMIC_TRAIT);
}
ASFUNCTIONBODY_ATOM(AVM1MovieClipLoader,loadClip)
{
	AVM1MovieClipLoader* th=asAtomHandler::as<AVM1MovieClipLoader>(obj);
	asAtom url = asAtomHandler::invalidAtom;
	asAtom target = asAtomHandler::invalidAtom;
	ARG_CHECK(ARG_UNPACK(url)(target));
	ret = asAtomHandler::falseAtom;
	if (!asAtomHandler::isString(url))
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError,"url");
		return;
	}
	tiny_string strurl= asAtomHandler::toString(url,wrk);
	DisplayObject* t =nullptr;
	if (asAtomHandler::isNumeric(target))
	{
		uint32_t level = asAtomHandler::toUInt(target);
		if (level > 0)
		{
			tiny_string strtarget = "_level";
			strtarget += Number::toString(level);
			t = wrk->rootClip->AVM1GetClipFromPath(strtarget);
			ret = asAtomHandler::trueAtom;
		}
	}
	else if (asAtomHandler::isString(target))
	{
		tiny_string strtarget = asAtomHandler::toString(target,wrk);
		t = wrk->rootClip->AVM1GetClipFromPath(strtarget);
	}
	else if (asAtomHandler::is<DisplayObject>(target))
	{
		t = asAtomHandler::getObject(target)->as<DisplayObject>();
	}
	else
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError,"target");
		return;
	}
	if (!t)
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError,"target");
		return;
	}
	URLRequest* r = Class<URLRequest>::getInstanceS(wrk,strurl,"GET",asAtomHandler::invalidAtom,t->loadedFrom);
	ret = asAtomHandler::trueAtom;
	th->addLoader(r,t,-1);
}
void AVM1MovieClipLoader::AVM1HandleEvent(EventDispatcher *dispatcher, Event* e)
{
	loadermutex.lock();
	Loader* ldr = nullptr;
	auto itldr = loaderlist.begin();
	while (itldr != loaderlist.end())
	{
		if ((*itldr)->getContentLoaderInfo() == dispatcher)
		{
			ldr = (*itldr);
			break;
		}
		itldr++;
	}
	loadermutex.unlock();
	if (ldr)
	{
		ASWorker* wrk = getInstanceWorker();
		
		multiname mlisteners(nullptr);
		mlisteners.name_type=multiname::NAME_STRING;
		mlisteners.isAttribute = false;
		mlisteners.name_s_id=getSystemState()->getUniqueStringId("_listeners");
		asAtom l = asAtomHandler::invalidAtom;
		this->getVariableByMultiname(l,mlisteners,GET_VARIABLE_OPTION::NONE,wrk);
		if (!asAtomHandler::is<AVM1Array>(l))
			return;
		AVM1Array* listeners = asAtomHandler::as<AVM1Array>(l);
		std::vector<ASObject*> tmplisteners;
		tmplisteners.push_back(this);
		for (uint32_t i = 0; i < listeners->size(); i++)
		{
			asAtom a= asAtomHandler::invalidAtom;
			listeners->at_nocheck(a,i);
			ASObject* o = asAtomHandler::getObject(a);
			if (o)
				tmplisteners.push_back(o);
		}
		auto it = tmplisteners.begin();
		while (it != tmplisteners.end())
		{
			if (e->type == "open")
			{
				asAtom func=asAtomHandler::invalidAtom;
				multiname m(nullptr);
				m.name_type=multiname::NAME_STRING;
				m.isAttribute = false;
				m.name_s_id=getSystemState()->getUniqueStringId("onLoadStart");
				(*it)->getVariableByMultiname(func,m,GET_VARIABLE_OPTION::NONE,wrk);
				if (asAtomHandler::is<AVM1Function>(func))
				{
					asAtom ret=asAtomHandler::invalidAtom;
					asAtom obj = asAtomHandler::fromObject(this);
					
					asAtom args[1];
					if (ldr->getContent())
						args[0] = asAtomHandler::fromObject(ldr->getContent());
					else
						args[0] = asAtomHandler::undefinedAtom;
					asAtomHandler::as<AVM1Function>(func)->call(&ret,&obj,args,1);
				}
				ASATOM_DECREF(func);
			}
			else if (e->type == "avm1_init")
			{
				asAtom func=asAtomHandler::invalidAtom;
				multiname m(nullptr);
				m.name_type=multiname::NAME_STRING;
				m.isAttribute = false;
				m.name_s_id=getSystemState()->getUniqueStringId("onLoadInit");
				(*it)->getVariableByMultiname(func,m,GET_VARIABLE_OPTION::NONE,wrk);
				if (asAtomHandler::is<AVM1Function>(func))
				{
					AVM1Function* f = asAtomHandler::as<AVM1Function>(func);
					asAtom ret=asAtomHandler::invalidAtom;
					asAtom obj = asAtomHandler::fromObject(f->getClip());
					asAtom args[1];
					if (ldr->getContent())
						args[0] = asAtomHandler::fromObject(ldr->getContent());
					else
						args[0] = asAtomHandler::undefinedAtom;

					f->call(&ret,&obj,args,1);
				}
				ASATOM_DECREF(func);
				getSystemState()->stage->AVM1RemoveEventListener(this);
			}
			else if (e->type == "progress")
			{
				asAtom func=asAtomHandler::invalidAtom;
				multiname m(nullptr);
				m.name_type=multiname::NAME_STRING;
				m.isAttribute = false;
				m.name_s_id=getSystemState()->getUniqueStringId("onLoadProgress");
				(*it)->getVariableByMultiname(func,m,GET_VARIABLE_OPTION::NONE,wrk);
				if (asAtomHandler::is<AVM1Function>(func))
				{
					ProgressEvent* ev = e->as<ProgressEvent>();
					asAtom ret=asAtomHandler::invalidAtom;
					asAtom obj = asAtomHandler::fromObject(this);
					asAtom args[3];
					if (ldr->getContent())
						args[0] = asAtomHandler::fromObject(ldr->getContent());
					else
						args[0] = asAtomHandler::undefinedAtom;
					args[1] = asAtomHandler::fromInt(ev->bytesLoaded);
					args[2] = asAtomHandler::fromInt(ev->bytesTotal);
					asAtomHandler::as<AVM1Function>(func)->call(&ret,&obj,args,3);
				}
				ASATOM_DECREF(func);
			}
			else if (e->type == "complete")
			{
				asAtom func=asAtomHandler::invalidAtom;
				multiname m(nullptr);
				m.name_type=multiname::NAME_STRING;
				m.isAttribute = false;
				m.name_s_id=getSystemState()->getUniqueStringId("onLoadComplete");
				(*it)->getVariableByMultiname(func,m,GET_VARIABLE_OPTION::NONE,wrk);
				if (asAtomHandler::is<AVM1Function>(func))
				{
					asAtom ret=asAtomHandler::invalidAtom;
					asAtom obj = asAtomHandler::fromObject(this);
					asAtom args[1];
					if (ldr->getContent())
						args[0] = asAtomHandler::fromObject(ldr->getContent());
					else
						args[0] = asAtomHandler::undefinedAtom;
					asAtomHandler::as<AVM1Function>(func)->call(&ret,&obj,args,1);
				}
				ASATOM_DECREF(func);
			}
			else if (e->type == "ioError")
			{
				asAtom func=asAtomHandler::invalidAtom;
				multiname m(nullptr);
				m.name_type=multiname::NAME_STRING;
				m.isAttribute = false;
				m.name_s_id=getSystemState()->getUniqueStringId("onLoadError");
				(*it)->getVariableByMultiname(func,m,GET_VARIABLE_OPTION::NONE,wrk);
				if (asAtomHandler::is<AVM1Function>(func))
				{
					asAtom ret=asAtomHandler::invalidAtom;
					asAtom obj = asAtomHandler::fromObject(this);
					asAtomHandler::as<AVM1Function>(func)->call(&ret,&obj,nullptr,0);
				}
				ASATOM_DECREF(func);
			}
			it++;
		}
	}
}

void AVM1MovieClipLoader::finalize()
{
	getSystemState()->stage->AVM1RemoveEventListener(this);
	loadermutex.lock();
	auto itldr = loaderlist.begin();
	while (itldr != loaderlist.end())
	{
		Loader* o = (*itldr);
		itldr = loaderlist.erase(itldr);
		o->removeStoredMember();
	}
	loadermutex.unlock();

}

bool AVM1MovieClipLoader::destruct()
{
	loadermutex.lock();
	auto itldr = loaderlist.begin();
	while (itldr != loaderlist.end())
	{
		Loader* o = (*itldr);
		itldr = loaderlist.erase(itldr);
		o->removeStoredMember();
	}
	loadermutex.unlock();
	return ASObject::destruct();
}

void AVM1MovieClipLoader::prepareShutdown()
{
	if (preparedforshutdown)
		return;
	ASObject::prepareShutdown();
	loadermutex.lock();
	auto itldr = loaderlist.begin();
	while (itldr != loaderlist.end())
	{
		Loader* ldr = (*itldr);
		itldr = loaderlist.erase(itldr);
		ldr->prepareShutdown();
		ldr->removeStoredMember();
	}
	loadermutex.unlock();
}
bool AVM1MovieClipLoader::countCylicMemberReferences(garbagecollectorstate& gcstate)
{
	bool ret = ASObject::countCylicMemberReferences(gcstate);
	loadermutex.lock();
	auto itldr = loaderlist.begin();
	while (itldr != loaderlist.end())
	{
		ret = (*itldr)->countAllCylicMemberReferences(gcstate) || ret;
		itldr++;
	}
	loadermutex.unlock();
	return ret;
}


void AVM1MovieClipLoader::load(const tiny_string& url, const tiny_string& method, AVM1MovieClip* target, int level)
{
	URLRequest* r = Class<URLRequest>::getInstanceS(getInstanceWorker(),url,method,asAtomHandler::invalidAtom,target->loadedFrom);
	addLoader(r, target, level);
}

void AVM1Color::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_FINAL);
	c->isReusable = true;
	c->prototype->setDeclaredMethodByQName("getRGB","",c->getSystemState()->getBuiltinFunction(getRGB),NORMAL_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("setRGB","",c->getSystemState()->getBuiltinFunction(setRGB),NORMAL_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("getTransform","",c->getSystemState()->getBuiltinFunction(getTransform),NORMAL_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("setTransform","",c->getSystemState()->getBuiltinFunction(setTransform),NORMAL_METHOD,false,false);
}
ASFUNCTIONBODY_ATOM(AVM1Color,_constructor)
{
	AVM1Color* th=asAtomHandler::as<AVM1Color>(obj);
	if(argslen == 0)
	{
		createError<ArgumentError>(wrk,kWrongArgumentCountError, "Color", "1", "0");
		return;
	}
	DisplayObject* t = nullptr;
	if (asAtomHandler::isString(args[0]))
	{
		tiny_string s = asAtomHandler::toString(args[0],wrk);
		t = wrk->rootClip->AVM1GetClipFromPath(s);
	}
	else if (asAtomHandler::is<DisplayObject>(args[0]))
		t = asAtomHandler::as<DisplayObject>(args[0]);
	if (t)
	{
		if (t->is<AVM1MovieClip>())
			t->as<AVM1MovieClip>()->setColor(th);
		else
			LOG(LOG_NOT_IMPLEMENTED,"constructing AVM1Color without MovieClip as target:"<<t->toDebugString());
		th->target=t->as<AVM1MovieClip>();
		t->addOwnedObject(th);
	}
}
ASFUNCTIONBODY_ATOM(AVM1Color,getRGB)
{
	AVM1Color* th=asAtomHandler::as<AVM1Color>(obj);
	if (th->target)
		asAtomHandler::setUInt(ret,th->target->colorTransform.getRGB());
	else
		ret = asAtomHandler::undefinedAtom;
}
ASFUNCTIONBODY_ATOM(AVM1Color,setRGB)
{
	AVM1Color* th=asAtomHandler::as<AVM1Color>(obj);
	assert_and_throw(argslen==1);
	if (th->target)
	{
		uint32_t tmp=asAtomHandler::toInt(args[0]);
		th->target->colorTransform.setfromRGB(tmp);
	}
}

ASFUNCTIONBODY_ATOM(AVM1Color,getTransform)
{
	AVM1Color* th=asAtomHandler::as<AVM1Color>(obj);
	ASObject* o = new_asobject(wrk);
	if (th->target)
	{
		asAtom a= asAtomHandler::undefinedAtom;
		multiname m(nullptr);
		m.isAttribute = false;
		m.name_type=multiname::NAME_STRING;

		m.name_s_id=wrk->getSystemState()->getUniqueStringId("ra");
		a = asAtomHandler::fromNumber(th->target->colorTransform.redMultiplier*100.0);
		o->setVariableByMultiname(m,a,CONST_ALLOWED,nullptr,wrk);

		m.name_s_id=wrk->getSystemState()->getUniqueStringId("rb");
		a = asAtomHandler::fromNumber(th->target->colorTransform.redOffset);
		o->setVariableByMultiname(m,a,CONST_ALLOWED,nullptr,wrk);

		m.name_s_id=wrk->getSystemState()->getUniqueStringId("ga");
		a = asAtomHandler::fromNumber(th->target->colorTransform.greenMultiplier*100.0);
		o->setVariableByMultiname(m,a,CONST_ALLOWED,nullptr,wrk);

		m.name_s_id=wrk->getSystemState()->getUniqueStringId("gb");
		a = asAtomHandler::fromNumber(th->target->colorTransform.greenOffset);
		o->setVariableByMultiname(m,a,CONST_ALLOWED,nullptr,wrk);

		m.name_s_id=wrk->getSystemState()->getUniqueStringId("ba");
		a = asAtomHandler::fromNumber(th->target->colorTransform.blueMultiplier*100.0);
		o->setVariableByMultiname(m,a,CONST_ALLOWED,nullptr,wrk);

		m.name_s_id=wrk->getSystemState()->getUniqueStringId("bb");
		a = asAtomHandler::fromNumber(th->target->colorTransform.blueOffset);
		o->setVariableByMultiname(m,a,CONST_ALLOWED,nullptr,wrk);

		m.name_s_id=wrk->getSystemState()->getUniqueStringId("aa");
		a = asAtomHandler::fromNumber(th->target->colorTransform.alphaMultiplier*100.0);
		o->setVariableByMultiname(m,a,CONST_ALLOWED,nullptr,wrk);

		m.name_s_id=wrk->getSystemState()->getUniqueStringId("ab");
		a = asAtomHandler::fromNumber(th->target->colorTransform.alphaOffset);
		o->setVariableByMultiname(m,a,CONST_ALLOWED,nullptr,wrk);

	}
	ret = asAtomHandler::fromObject(o);
}

ASFUNCTIONBODY_ATOM(AVM1Color,setTransform)
{
	AVM1Color* th=asAtomHandler::as<AVM1Color>(obj);
	assert_and_throw(argslen==1);
	ASObject* o = asAtomHandler::toObject(args[0],wrk);
	if (th->target)
	{
		asAtom a = asAtomHandler::invalidAtom;
		multiname m(nullptr);
		m.isAttribute = false;
		m.name_type=multiname::NAME_STRING;

		m.name_s_id=wrk->getSystemState()->getUniqueStringId("ra");
		a = asAtomHandler::invalidAtom;
		o->getVariableByMultiname(a,m,GET_VARIABLE_OPTION::NONE,wrk);
		if (asAtomHandler::isValid(a))
			th->target->colorTransform.redMultiplier = asAtomHandler::toNumber(a)/100.0;
		ASATOM_DECREF(a);

		m.name_s_id=wrk->getSystemState()->getUniqueStringId("rb");
		a = asAtomHandler::invalidAtom;
		o->getVariableByMultiname(a,m,GET_VARIABLE_OPTION::NONE,wrk);
		if (asAtomHandler::isValid(a))
			th->target->colorTransform.redOffset = asAtomHandler::toNumber(a);
		ASATOM_DECREF(a);

		m.name_s_id=wrk->getSystemState()->getUniqueStringId("ga");
		a = asAtomHandler::invalidAtom;
		o->getVariableByMultiname(a,m,GET_VARIABLE_OPTION::NONE,wrk);
		if (asAtomHandler::isValid(a))
			th->target->colorTransform.greenMultiplier = asAtomHandler::toNumber(a)/100.0;
		ASATOM_DECREF(a);

		m.name_s_id=wrk->getSystemState()->getUniqueStringId("gb");
		a = asAtomHandler::invalidAtom;
		o->getVariableByMultiname(a,m,GET_VARIABLE_OPTION::NONE,wrk);
		if (asAtomHandler::isValid(a))
			th->target->colorTransform.greenOffset = asAtomHandler::toNumber(a);
		ASATOM_DECREF(a);

		m.name_s_id=wrk->getSystemState()->getUniqueStringId("ba");
		a = asAtomHandler::invalidAtom;
		o->getVariableByMultiname(a,m,GET_VARIABLE_OPTION::NONE,wrk);
		if (asAtomHandler::isValid(a))
			th->target->colorTransform.blueMultiplier = asAtomHandler::toNumber(a)/100.0;
		ASATOM_DECREF(a);

		m.name_s_id=wrk->getSystemState()->getUniqueStringId("bb");
		a = asAtomHandler::invalidAtom;
		o->getVariableByMultiname(a,m,GET_VARIABLE_OPTION::NONE,wrk);
		if (asAtomHandler::isValid(a))
			th->target->colorTransform.blueOffset = asAtomHandler::toNumber(a);
		ASATOM_DECREF(a);

		m.name_s_id=wrk->getSystemState()->getUniqueStringId("aa");
		a = asAtomHandler::invalidAtom;
		o->getVariableByMultiname(a,m,GET_VARIABLE_OPTION::NONE,wrk);
		if (asAtomHandler::isValid(a))
			th->target->colorTransform.alphaMultiplier = asAtomHandler::toNumber(a)/100.0;
		ASATOM_DECREF(a);

		m.name_s_id=wrk->getSystemState()->getUniqueStringId("ab");
		a = asAtomHandler::invalidAtom;
		o->getVariableByMultiname(a,m,GET_VARIABLE_OPTION::NONE,wrk);
		if (asAtomHandler::isValid(a))
			th->target->colorTransform.alphaOffset = asAtomHandler::toNumber(a);
		ASATOM_DECREF(a);
		th->target->hasChanged=true;
		th->target->requestInvalidation(wrk->getSystemState());
	}
}
bool AVM1Color::destruct()
{
	target=nullptr;
	return ASObject::destruct();
}

void AVM1Broadcaster::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_FINAL);
	c->isReusable = true;
	c->prototype->setDeclaredMethodByQName("initialize","",c->getSystemState()->getBuiltinFunction(initialize),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("broadcastMessage","",c->getSystemState()->getBuiltinFunction(broadcastMessage),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("addListener","",c->getSystemState()->getBuiltinFunction(addListener),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("removeListener","",c->getSystemState()->getBuiltinFunction(removeListener),NORMAL_METHOD,false);
}
ASFUNCTIONBODY_ATOM(AVM1Broadcaster,initialize)
{
	_NR<ASObject> listener;
	ARG_CHECK(ARG_UNPACK(listener));
	if (!listener.isNull())
	{
		Array* listeners = Class<Array>::getInstanceSNoArgs(wrk);
		listener->setVariableAtomByQName("broadcastMessage",nsNameAndKind(),asAtomHandler::fromObjectNoPrimitive(wrk->getSystemState()->getBuiltinFunction(broadcastMessage)),DYNAMIC_TRAIT);
		listener->setVariableAtomByQName("addListener",nsNameAndKind(),asAtomHandler::fromObjectNoPrimitive(wrk->getSystemState()->getBuiltinFunction(addListener)),DYNAMIC_TRAIT);
		listener->setVariableAtomByQName("removeListener",nsNameAndKind(),asAtomHandler::fromObjectNoPrimitive(wrk->getSystemState()->getBuiltinFunction(removeListener)),DYNAMIC_TRAIT);
		listener->setVariableAtomByQName("_listeners",nsNameAndKind(),asAtomHandler::fromObjectNoPrimitive(listeners),DYNAMIC_TRAIT);
	}
}
ASFUNCTIONBODY_ATOM(AVM1Broadcaster,broadcastMessage)
{
	ASObject* th = asAtomHandler::getObject(obj);
	tiny_string msg;
	ARG_CHECK(ARG_UNPACK(msg));
	asAtom l = asAtomHandler::invalidAtom;
	multiname m(nullptr);
	m.name_type=multiname::NAME_STRING;
	m.name_s_id=wrk->getSystemState()->getUniqueStringId("_listeners");
	th->getVariableByMultiname(l,m,GET_VARIABLE_OPTION::NO_INCREF,wrk);
	if (asAtomHandler::isArray(l))
	{
		multiname mmsg(nullptr);
		mmsg.name_type=multiname::NAME_STRING;
		mmsg.name_s_id=wrk->getSystemState()->getUniqueStringId(msg);
		Array* listeners = asAtomHandler::as<Array>(l);
		for (uint32_t i =0; i < listeners->size(); i++)
		{
			asAtom o;
			listeners->at_nocheck(o,i);
			if (asAtomHandler::isObject(o))
			{
				ASObject* listener = asAtomHandler::getObjectNoCheck(o);
				asAtom f = asAtomHandler::invalidAtom;
				listener->getVariableByMultiname(f,mmsg,GET_VARIABLE_OPTION::NONE,wrk);
				asAtom res = asAtomHandler::invalidAtom;
				if (asAtomHandler::is<Function>(f))
				{
					asAtomHandler::as<Function>(f)->call(res,wrk,o,argslen > 1 ? &args[1] : nullptr,argslen-1);
					asAtomHandler::as<Function>(f)->decRef();
				}
				else if (asAtomHandler::is<SyntheticFunction>(f))
				{
					asAtomHandler::as<SyntheticFunction>(f)->call(wrk,res,o,argslen > 1 ? &args[1] : nullptr,argslen-1,false,false);
				}
				else if (asAtomHandler::is<AVM1Function>(f))
				{
					asAtomHandler::as<AVM1Function>(f)->call(&res,&o,argslen > 1 ? &args[1] : nullptr,argslen-1);
					asAtomHandler::as<AVM1Function>(f)->decRef();
				}
			}
		}
	}
}

ASFUNCTIONBODY_ATOM(AVM1Broadcaster,addListener)
{
	ASObject* th = asAtomHandler::getObject(obj);
	asAtom listener = asAtomHandler::invalidAtom;
	ARG_CHECK(ARG_UNPACK(listener));
	if (asAtomHandler::isInvalid(listener))
	{
		ret = asAtomHandler::falseAtom;
		return;
	}
	asAtom l = asAtomHandler::invalidAtom;
	multiname m(nullptr);
	m.name_type=multiname::NAME_STRING;
	m.name_s_id=wrk->getSystemState()->getUniqueStringId("_listeners");

	th->getVariableByMultiname(l,m,GET_VARIABLE_OPTION::NO_INCREF,wrk);
	if (asAtomHandler::isArray(l))
	{
		Array* listeners = asAtomHandler::as<Array>(l);
		bool found = false;
		for (uint32_t i=0; i < listeners->size(); i++)
		{
			asAtom a = asAtomHandler::invalidAtom;
			listeners->at_nocheck(a,i);
			if (a.uintval == args[0].uintval)
			{
				found = true;
				break;
			}
		}
		if (!found)
		{
			ASATOM_INCREF(listener);
			listeners->push(listener);
		}
	}
	if (th == Class<AVM1Key>::getRef(wrk->getSystemState()).getPtr())
		ret = asAtomHandler::fromBool(wrk->getSystemState()->stage->AVM1AddKeyboardListener(args[0]));
	else if (th == Class<AVM1Mouse>::getRef(wrk->getSystemState()).getPtr())
		ret = asAtomHandler::fromBool(wrk->getSystemState()->stage->AVM1AddMouseListener(args[0]));
	else if (th == Class<AVM1Stage>::getRef(wrk->getSystemState()).getPtr())
		wrk->getSystemState()->stage->AVM1AddResizeListener(asAtomHandler::getObject(listener));
	else if (th->is<AVM1MovieClipLoader>())
		wrk->getSystemState()->stage->AVM1AddEventListener(th);
	else
		ret = asAtomHandler::trueAtom;
}
ASFUNCTIONBODY_ATOM(AVM1Broadcaster,removeListener)
{
	ASObject* th = asAtomHandler::getObject(obj);
	asAtom listener = asAtomHandler::invalidAtom;
	ARG_CHECK(ARG_UNPACK(listener));
	ret = asAtomHandler::falseAtom;
	if (asAtomHandler::isInvalid(listener))
		return;
	asAtom l = asAtomHandler::invalidAtom;
	multiname m(nullptr);
	m.name_type=multiname::NAME_STRING;
	m.name_s_id=wrk->getSystemState()->getUniqueStringId("_listeners");
	th->getVariableByMultiname(l,m,GET_VARIABLE_OPTION::NO_INCREF,wrk);
	if (asAtomHandler::isArray(l))
	{
		Array* listeners = asAtomHandler::as<Array>(l);
		for (uint32_t i =0; i < listeners->size(); i++)
		{
			asAtom o=asAtomHandler::invalidAtom;
			listeners->at_nocheck(o,i);
			LOG(LOG_ERROR,"***removelist:"<<asAtomHandler::toDebugString(o)<<" "<<i<<"/"<<listeners->size()<<" "<<asAtomHandler::toDebugString(args[0]));
			if (asAtomHandler::isEqualStrict(o,wrk,args[0]))
			{
				asAtom res;
				asAtom index = asAtomHandler::fromUInt(i);
				Array::removeAt(res,wrk,l,&index,1);
				ASATOM_DECREF(res);
				ret=asAtomHandler::trueAtom;
				break;
			}
		}
	}
	if (th == Class<AVM1Key>::getRef(wrk->getSystemState()).getPtr())
		ret = asAtomHandler::fromBool(wrk->getSystemState()->stage->AVM1RemoveKeyboardListener(args[0]));
	else if (th == Class<AVM1Mouse>::getRef(wrk->getSystemState()).getPtr())
		ret = asAtomHandler::fromBool(wrk->getSystemState()->stage->AVM1RemoveMouseListener(args[0]));
	else if (th == Class<AVM1Stage>::getRef(wrk->getSystemState()).getPtr())
		wrk->getSystemState()->stage->AVM1RemoveResizeListener(asAtomHandler::getObject(listener));
	else if (th->is<AVM1MovieClipLoader>())
		wrk->getSystemState()->stage->AVM1RemoveEventListener(th);
}

void AVM1BitmapData::sinit(Class_base *c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED);
	c->isReusable=true;

	c->prototype->setDeclaredMethodByQName("clone","",c->getSystemState()->getBuiltinFunction(clone,0,Class<AVM1BitmapData>::getClassUninitialized(c->getSystemState())),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("applyFilter","",c->getSystemState()->getBuiltinFunction(applyFilter),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("clone","",c->getSystemState()->getBuiltinFunction(clone,0,Class<AVM1BitmapData>::getClassUninitialized(c->getSystemState())),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("colorTransform","",c->getSystemState()->getBuiltinFunction(colorTransform),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("compare","",c->getSystemState()->getBuiltinFunction(compare,1,Class<ASObject>::getClassUninitialized(c->getSystemState())),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("copyChannel","",c->getSystemState()->getBuiltinFunction(copyChannel),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("copyPixels","",c->getSystemState()->getBuiltinFunction(copyPixels),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("dispose","",c->getSystemState()->getBuiltinFunction(dispose),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("draw","",c->getSystemState()->getBuiltinFunction(draw),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("fillRect","",c->getSystemState()->getBuiltinFunction(fillRect),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("floodFill","",c->getSystemState()->getBuiltinFunction(floodFill),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("generateFilterRect","",c->getSystemState()->getBuiltinFunction(generateFilterRect),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("getColorBoundsRect","",c->getSystemState()->getBuiltinFunction(getColorBoundsRect,2,Class<Rectangle>::getClassUninitialized(c->getSystemState())),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("getPixel","",c->getSystemState()->getBuiltinFunction(getPixel,2,Class<UInteger>::getClassUninitialized(c->getSystemState())),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("getPixel32","",c->getSystemState()->getBuiltinFunction(getPixel32,2,Class<UInteger>::getClassUninitialized(c->getSystemState())),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("height","",c->getSystemState()->getBuiltinFunction(_getHeight,0,Class<Integer>::getClassUninitialized(c->getSystemState())),GETTER_METHOD,false);
	c->prototype->setDeclaredMethodByQName("hitTest","",c->getSystemState()->getBuiltinFunction(hitTest,2,Class<Boolean>::getClassUninitialized(c->getSystemState())),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("loadBitmap","",c->getSystemState()->getBuiltinFunction(loadBitmap),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("merge","",c->getSystemState()->getBuiltinFunction(threshold),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("noise","",c->getSystemState()->getBuiltinFunction(noise),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("paletteMap","",c->getSystemState()->getBuiltinFunction(paletteMap),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("perlinNoise","",c->getSystemState()->getBuiltinFunction(perlinNoise),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("pixelDissolve","",c->getSystemState()->getBuiltinFunction(pixelDissolve,3,Class<UInteger>::getClassUninitialized(c->getSystemState())),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("rectangle","",c->getSystemState()->getBuiltinFunction(getRect,0,Class<Rectangle>::getClassUninitialized(c->getSystemState())),GETTER_METHOD,false);
	c->prototype->setDeclaredMethodByQName("scroll","",c->getSystemState()->getBuiltinFunction(scroll),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("setPixel","",c->getSystemState()->getBuiltinFunction(setPixel),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("setPixel32","",c->getSystemState()->getBuiltinFunction(setPixel32),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("threshold","",c->getSystemState()->getBuiltinFunction(threshold),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("transparent","",c->getSystemState()->getBuiltinFunction(_getTransparent,0,Class<Boolean>::getClassUninitialized(c->getSystemState())),GETTER_METHOD,false);
	c->prototype->setDeclaredMethodByQName("width","",c->getSystemState()->getBuiltinFunction(_getWidth,0,Class<Integer>::getClassUninitialized(c->getSystemState())),GETTER_METHOD,false);
}

ASFUNCTIONBODY_ATOM(AVM1BitmapData,loadBitmap)
{
	tiny_string name;
	ARG_CHECK(ARG_UNPACK(name));
	BitmapTag* tag = dynamic_cast<BitmapTag*>( wrk->rootClip->applicationDomain->dictionaryLookupByName(wrk->getSystemState()->getUniqueStringId(name)));
	if (tag)
		ret = asAtomHandler::fromObjectNoPrimitive(tag->instance());
	else
		LOG(LOG_ERROR,"BitmapData.loadBitmap tag not found:"<<name);
}

AVM1Bitmap::AVM1Bitmap(ASWorker* wrk, Class_base* c, LoaderInfo* li, istream* s, FILE_TYPE type):Bitmap(wrk,c,li,s,type)
{
}

void AVM1Bitmap::sinit(Class_base *c)
{
	Bitmap::sinit(c);
	DisplayObject::AVM1SetupMethods(c);
	c->setDeclaredMethodByQName("forceSmoothing","",c->getSystemState()->getBuiltinFunction(_getter_smoothing),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("forceSmoothing","",c->getSystemState()->getBuiltinFunction(_setter_smoothing),SETTER_METHOD,true);
}
