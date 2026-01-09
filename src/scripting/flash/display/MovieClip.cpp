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

#include "scripting/flash/display/flashdisplay.h"
#include "scripting/flash/display/FrameContainer.h"
#include "scripting/flash/display/RootMovieClip.h"
#include "parsing/tags.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include "scripting/avm1/avm1display.h"
#include "scripting/avm1/avm1text.h"
#include "scripting/flash/display/Loader.h"
#include "scripting/flash/geom/flashgeom.h"
#include "scripting/flash/geom/Point.h"
#include "scripting/flash/geom/Rectangle.h"
#include "scripting/toplevel/AVM1Function.h"
#include "scripting/toplevel/Array.h"
#include "scripting/toplevel/Integer.h"
#include "scripting/flash/ui/keycodes.h"

using namespace std;
using namespace lightspark;

void MovieClip::sinit(Class_base* c)
{
	CLASS_SETUP(c, Sprite, _constructor, CLASS_DYNAMIC_NOT_FINAL);
	c->isReusable = true;
	c->setDeclaredMethodByQName("currentFrame","",c->getSystemState()->getBuiltinFunction(_getCurrentFrame,0,Class<Integer>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("totalFrames","",c->getSystemState()->getBuiltinFunction(_getTotalFrames,0,Class<Integer>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("framesLoaded","",c->getSystemState()->getBuiltinFunction(_getFramesLoaded,0,Class<Integer>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("currentFrameLabel","",c->getSystemState()->getBuiltinFunction(_getCurrentFrameLabel,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("currentLabel","",c->getSystemState()->getBuiltinFunction(_getCurrentLabel,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("currentLabels","",c->getSystemState()->getBuiltinFunction(_getCurrentLabels,0,Class<Array>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("scenes","",c->getSystemState()->getBuiltinFunction(_getScenes,0,Class<Array>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("currentScene","",c->getSystemState()->getBuiltinFunction(_getCurrentScene,0,Class<Scene>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("isPlaying","",c->getSystemState()->getBuiltinFunction(_getIsPlaying,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("stop","",c->getSystemState()->getBuiltinFunction(stop),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("play","",c->getSystemState()->getBuiltinFunction(play),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("gotoAndStop","",c->getSystemState()->getBuiltinFunction(gotoAndStop),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("gotoAndPlay","",c->getSystemState()->getBuiltinFunction(gotoAndPlay),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("prevFrame","",c->getSystemState()->getBuiltinFunction(prevFrame),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("nextFrame","",c->getSystemState()->getBuiltinFunction(nextFrame),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("prevScene","",c->getSystemState()->getBuiltinFunction(prevScene),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("nextScene","",c->getSystemState()->getBuiltinFunction(nextScene),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("addFrameScript","",c->getSystemState()->getBuiltinFunction(addFrameScript),NORMAL_METHOD,true);
	REGISTER_GETTER_SETTER_RESULTTYPE(c, enabled,Boolean);
}

ASFUNCTIONBODY_GETTER_SETTER(MovieClip, enabled)

MovieClip::MovieClip(ASWorker* wrk, Class_base* c):Sprite(wrk,c),fromDefineSpriteTag(UINT32_MAX),lastFrameScriptExecuted(UINT32_MAX),lastratio(0),inExecuteFramescript(false)
	,inAVM1Attachment(false),isAVM1Loaded(false)
	,forAVM1InitAction(false)
	,framecontainer(nullptr)
	,actions(nullptr)
	,avm1clipeventlistenercount(0)
	,totalFrames_unreliable(1)
	,enabled(true)
{
	subtype=SUBTYPE_MOVIECLIP;
}

MovieClip::MovieClip(ASWorker* wrk, Class_base* c, FrameContainer* f, uint32_t defineSpriteTagID):Sprite(wrk,c),fromDefineSpriteTag(defineSpriteTagID),lastFrameScriptExecuted(UINT32_MAX),lastratio(0),inExecuteFramescript(false)
	,inAVM1Attachment(false),isAVM1Loaded(false)
	,forAVM1InitAction(false)
	,framecontainer(f)
	,actions(nullptr)
	,avm1clipeventlistenercount(0)
	,totalFrames_unreliable(f->getFramesLoaded())
	,enabled(true)
{
	subtype=SUBTYPE_MOVIECLIP;
	//For sprites totalFrames_unreliable is the actual frame count
	//For the root movie, it's the frame count from the header
}

bool MovieClip::destruct()
{
	framecontainer=nullptr;
	inAVM1Attachment=false;
	isAVM1Loaded=false;
	auto it = frameScripts.begin();
	while (it != frameScripts.end())
	{
		ASATOM_REMOVESTOREDMEMBER(it->second);
		it++;
	}
	frameScripts.clear();

	fromDefineSpriteTag = UINT32_MAX;
	lastFrameScriptExecuted = UINT32_MAX;
	lastratio=0;
	totalFrames_unreliable = 1;
	avm1clipeventlistenercount=0;
	inExecuteFramescript=false;

	state.reset();
	actions=nullptr;

	enabled = true;
	while (!avm1loaderlist.empty())
	{
		avm1loaderlist.front()->removeStoredMember();
		avm1loaderlist.pop_front();
	}
	avm1context.setScope(nullptr);
	avm1context.setGlobalScope(nullptr);
	return Sprite::destruct();
}

void MovieClip::finalize()
{
	framecontainer=nullptr;
	auto it = frameScripts.begin();
	while (it != frameScripts.end())
	{
		ASATOM_REMOVESTOREDMEMBER(it->second);
		it++;
	}
	frameScripts.clear();
	state.reset();
	while (!avm1loaderlist.empty())
	{
		avm1loaderlist.front()->removeStoredMember();
		avm1loaderlist.pop_front();
	}
	for (auto it = this->AVM1FrameContexts.begin(); it != this->AVM1FrameContexts.end(); it++)
	{
		it->second.setScope(nullptr);
		it->second.setGlobalScope(nullptr);
	}
	avm1context.setScope(nullptr);
	avm1context.setGlobalScope(nullptr);
	Sprite::finalize();
}

void MovieClip::prepareShutdown()
{
	if (preparedforshutdown)
		return;
	if (actions)
	{
		if (actions->AllEventFlags.ClipEventMouseDown ||
			actions->AllEventFlags.ClipEventMouseMove ||
			actions->AllEventFlags.ClipEventRollOver ||
			actions->AllEventFlags.ClipEventRollOut ||
			actions->AllEventFlags.ClipEventPress ||
			actions->AllEventFlags.ClipEventMouseUp)
		{
			getSystemState()->stage->AVM1RemoveMouseListener(asAtomHandler::fromObjectNoPrimitive(this));
		}
		if (actions->AllEventFlags.ClipEventKeyDown ||
			actions->AllEventFlags.ClipEventKeyUp ||
			actions->AllEventFlags.ClipEventKeyPress)
			getSystemState()->stage->AVM1RemoveKeyboardListener(asAtomHandler::fromObjectNoPrimitive(this));
		if (actions->AllEventFlags.ClipEventEnterFrame)
			getSystemState()->unregisterFrameListener(this);

		while (avm1clipeventlistenercount--)
		{
			getSystemState()->stage->AVM1RemoveEventListener(this);
		}

	}
	Sprite::prepareShutdown();
	auto it = frameScripts.begin();
	while (it != frameScripts.end())
	{
		ASObject* o = asAtomHandler::getObject(it->second);
		if (o)
			o->prepareShutdown();
		it++;
	}
	avm1context.setScope(nullptr);
	avm1context.setGlobalScope(nullptr);
	while (!avm1loaderlist.empty())
	{
		ASObject* o = avm1loaderlist.front();
		avm1loaderlist.pop_front();
		o->prepareShutdown();
		o->removeStoredMember();
	}
}

bool MovieClip::countCylicMemberReferences(garbagecollectorstate &gcstate)
{
	if (skipCountCylicMemberReferences(gcstate))
		return gcstate.hasMember(this);
	bool ret = Sprite::countCylicMemberReferences(gcstate);
	for (auto it = frameScripts.begin(); it != frameScripts.end(); it++)
	{
		ASObject* o = asAtomHandler::getObject(it->second);
		if (o)
			ret = o->countAllCylicMemberReferences(gcstate) || ret;
	}
	for (auto it = avm1loaderlist.begin();it != avm1loaderlist.end(); it++)
		ret = (*it)->countAllCylicMemberReferences(gcstate) || ret;
	if (!needsActionScript3())
	{
		for (auto it = this->AVM1FrameContexts.begin(); it != this->AVM1FrameContexts.end(); it++)
		{
			if (it->second.getScope())
				ret |= it->second.getScope()->countAllCyclicMemberReferences(gcstate);
			if (it->second.getGlobalScope())
				ret |= it->second.getGlobalScope()->countAllCyclicMemberReferences(gcstate);
		}
		if (avm1context.getScope())
			ret |= avm1context.getScope()->countAllCyclicMemberReferences(gcstate);
		if (avm1context.getGlobalScope())
			ret |= avm1context.getGlobalScope()->countAllCyclicMemberReferences(gcstate);
	}
	return ret;
}

ASFUNCTIONBODY_ATOM(MovieClip,addFrameScript)
{
	MovieClip* th=Class<MovieClip>::cast(asAtomHandler::getObject(obj));
	assert_and_throw(argslen>=2 && argslen%2==0);

	for(uint32_t i=0;i<argslen;i+=2)
	{
		uint32_t frame=asAtomHandler::toInt(args[i]);
		// NOTE: `addFrameScript()` will remove the script asscociated
		// with this frame, if the argument isn't a function.
		if (!asAtomHandler::isFunction(args[i+1]))
		{
			LOG(LOG_ERROR,"Not a function");

			auto it = th->frameScripts.find(frame);
			if (it != th->frameScripts.end())
			{
				ASATOM_REMOVESTOREDMEMBER(it->second);
				th->frameScripts.erase(it);
			}
			continue;
		}

		IFunction* func = asAtomHandler::as<IFunction>(args[i+1]);
		func->incRef();
		func->addStoredMember();
		auto it = th->frameScripts.find(frame);
		if (it != th->frameScripts.end())
			ASATOM_REMOVESTOREDMEMBER(it->second);
		th->frameScripts[frame]=args[i+1];
	}
}

ASFUNCTIONBODY_ATOM(MovieClip,swapDepths)
{
	LOG(LOG_NOT_IMPLEMENTED,"Called swapDepths");
}

ASFUNCTIONBODY_ATOM(MovieClip,stop)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	th->setStopped();
}

ASFUNCTIONBODY_ATOM(MovieClip,play)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	th->setPlaying();
}
void MovieClip::setPlaying()
{
	if (state.stop_FP)
	{
		state.stop_FP=false;
		if (!needsActionScript3() && state.next_FP == state.FP)
		{
			if (state.FP == framecontainer->getFramesLoaded()-1)
				state.next_FP = 0;
			else
				state.next_FP++;
		}
		if (isOnStage())
		{
			if (needsActionScript3())
				advanceFrame(true);
		}
		else
			getSystemState()->stage->addHiddenObject(this);
	}
}
void MovieClip::setStopped()
{
	if (!state.stop_FP)
	{
		state.stop_FP=true;
		// if we reset state.next_FP when it was set explicitly, we get into an infinite loop (see ruffle test avm2/goto_framescript_queued)
		if (!state.explicit_FP)
			state.next_FP=state.FP;
	}
}
void MovieClip::gotoAnd(asAtom* args, const unsigned int argslen, bool stop)
{
	uint32_t next_FP=0;
	tiny_string sceneName;
	assert_and_throw(argslen==1 || argslen==2);
	if(argslen==2 && needsActionScript3())
	{
		sceneName = asAtomHandler::toString(args[1],getInstanceWorker());
	}
	uint32_t dest=FRAME_NOT_FOUND;
	if (asAtomHandler::isInteger(args[0]) || asAtomHandler::isUInteger(args[0]))
	{
		uint32_t inFrameNo = asAtomHandler::toInt(args[0]);
		if(inFrameNo == 0)
			inFrameNo = 1;

		dest = framecontainer->getFrameIdByNumber(inFrameNo-1, sceneName,state.FP);
	}
	else
	{
		tiny_string label = asAtomHandler::toString(args[0],getInstanceWorker());
		number_t ret=0;
		if (Integer::fromStringFlashCompatible(label.raw_buf(),ret,10,true))
			dest = framecontainer->getFrameIdByNumber(ret-1, sceneName,state.FP);
		else
			dest = framecontainer->getFrameIdByLabel(label, sceneName);

		if(dest==FRAME_NOT_FOUND)
		{
			dest=0;
			LOG(LOG_ERROR, (stop ? "gotoAndStop: label not found:" : "gotoAndPlay: label not found:") <<asAtomHandler::toString(args[0],getInstanceWorker())<<" in scene "<<sceneName<<" at movieclip "<<getTagID()<<" "<<this->state.FP);
		}
	}
	if (dest!=FRAME_NOT_FOUND)
	{
		next_FP=dest;
		while(next_FP >= framecontainer->getFramesLoaded())
		{
			if (hasFinishedLoading() || getSystemState()->runSingleThreaded || isMainThread())
			{
				if (next_FP >= framecontainer->getFramesLoaded())
				{
					LOG(LOG_ERROR, next_FP << "= next_FP >= state.max_FP = " << framecontainer->getFramesLoaded() << " on "<<this->toDebugString()<<" "<<this->getTagID());
					next_FP = framecontainer->getFramesLoaded()-1;
				}
				break;
			}
			else
				getSystemState()->sleep_ms(100);
		}
	}
	bool newframe = state.FP != next_FP;
	if (!stop && !newframe && !needsActionScript3())
	{
		// for AVM1 gotoandplay if we are not switching to a new frame we just act like a normal "play"
		setPlaying();
		return;
	}
	state.next_FP = next_FP;
	state.explicit_FP = true;
	state.stop_FP = stop;
	if (newframe)
	{
		if (!needsActionScript3() || !inExecuteFramescript)
			runGoto(true);
		else
			state.gotoQueued = true;
	}
	else if (needsActionScript3())
	{
		state.gotoQueued = false;
		getSystemState()->runInnerGotoFrame(this);
	}
}
void MovieClip::runGoto(bool newFrame)
{
	if (!needsActionScript3())
	{
		advanceFrame(false);
		return;
	}

	if (newFrame)
	{
		if (!state.creatingframe)
			lastFrameScriptExecuted=UINT32_MAX;
		skipFrame = false;
		advanceFrame(false);
	}
	getSystemState()->runInnerGotoFrame(this, removedFrameScripts);
}

AVM1context* MovieClip::AVM1getCurrentFrameContext()
{

	auto it = AVM1FrameContexts.insert(make_pair(state.FP,AVM1context())).first;
	return &it->second;
}
void MovieClip::AVM1gotoFrameLabel(const tiny_string& label,bool stop, bool switchplaystate)
{
	uint32_t dest=framecontainer->getFrameIdByLabel(label, "");
	if(dest==FRAME_NOT_FOUND)
	{
		LOG(LOG_ERROR, "gotoFrameLabel: label not found:" <<label<<" "<<this->toDebugString());
		return;
	}
	AVM1gotoFrame(dest, stop, switchplaystate,true);
}
void MovieClip::AVM1gotoFrame(int frame, bool stop, bool switchplaystate, bool advanceFrame)
{
	if (frame < 0)
		frame = 0;
	state.next_FP = frame;
	state.explicit_FP = true;
	bool newframe = (int)state.FP != frame;
	if (switchplaystate)
		state.stop_FP = stop;
	if (advanceFrame)
		runGoto(newframe);
}

ASFUNCTIONBODY_ATOM(MovieClip,gotoAndStop)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	th->gotoAnd(args,argslen,true);
}

ASFUNCTIONBODY_ATOM(MovieClip,gotoAndPlay)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	th->gotoAnd(args,argslen,false);
}

ASFUNCTIONBODY_ATOM(MovieClip,nextFrame)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	assert_and_throw(th->state.FP<th->framecontainer->getFramesLoaded());
	bool newframe = !th->hasFinishedLoading() || th->state.FP != th->framecontainer->getFramesLoaded()-1;
	th->state.next_FP = th->hasFinishedLoading() && th->state.FP == th->framecontainer->getFramesLoaded()-1 ? th->state.FP : th->state.FP+1;
	th->state.explicit_FP=true;
	th->state.stop_FP=true;
	th->runGoto(newframe);
}

ASFUNCTIONBODY_ATOM(MovieClip,prevFrame)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	assert_and_throw(th->state.FP<th->framecontainer->getFramesLoaded());
	bool newframe = th->state.FP != 0;
	th->state.next_FP = th->state.FP == 0 ? th->state.FP : th->state.FP-1;
	th->state.explicit_FP=true;
	th->state.stop_FP=true;
	th->runGoto(newframe);
}

ASFUNCTIONBODY_ATOM(MovieClip,nextScene)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	assert_and_throw(th->state.FP<th->framecontainer->getFramesLoaded());
	if (th->hasFinishedLoading() && th->state.FP == th->framecontainer->getFramesLoaded()-1)
		return;
	bool newframe = !th->hasFinishedLoading() || th->state.FP != th->framecontainer->getFramesLoaded()-1;
	th->state.next_FP = th->framecontainer->nextScene(th->state.FP);
	th->state.explicit_FP=true;
	th->state.stop_FP=false;
	th->runGoto(newframe);
}

ASFUNCTIONBODY_ATOM(MovieClip,prevScene)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	assert_and_throw(th->state.FP<th->framecontainer->getFramesLoaded());
	if (th->hasFinishedLoading() && th->state.FP == 0)
		return;
	bool newframe = th->state.FP != 0;
	th->state.next_FP = th->framecontainer->prevScene(th->state.FP);
	th->state.explicit_FP=true;
	th->state.stop_FP=false;
	th->runGoto(newframe);
}

ASFUNCTIONBODY_ATOM(MovieClip,_getFramesLoaded)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	if (th->framecontainer->getFramesLoaded())
		asAtomHandler::setUInt(ret,wrk,th->framecontainer->getFramesLoaded());
	else
		asAtomHandler::setUInt(ret,wrk,1); // no frames available yet, report 1 frame anyway
}

ASFUNCTIONBODY_ATOM(MovieClip,_getTotalFrames)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	asAtomHandler::setUInt(ret,wrk,th->totalFrames_unreliable);
}

ASFUNCTIONBODY_ATOM(MovieClip,_getScenes)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	ret = th->framecontainer->getScenes(wrk,th->totalFrames_unreliable);
}

ASFUNCTIONBODY_ATOM(MovieClip,_getCurrentScene)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	ret = th->framecontainer->createCurrentScene(wrk,th->state.FP,th->totalFrames_unreliable);
}

ASFUNCTIONBODY_ATOM(MovieClip,_getCurrentFrame)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	if (th->framecontainer->getFramesLoaded())
	{
		//currentFrame is 1-based and relative to current scene
		asAtomHandler::setInt(ret,wrk,th->state.FP+1 - th->framecontainer->getCurrentSceneStartFrame(th->state.FP));
	}
	else
		asAtomHandler::setInt(ret,wrk,0);
}

ASFUNCTIONBODY_ATOM(MovieClip,_getCurrentFrameLabel)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	ret = th->framecontainer->getCurrentFrameLabel(wrk,th->state.FP);
}

ASFUNCTIONBODY_ATOM(MovieClip,_getCurrentLabel)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	ret = th->framecontainer->getCurrentLabel(wrk,th->state.FP);
}

ASFUNCTIONBODY_ATOM(MovieClip,_getCurrentLabels)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	ret = th->framecontainer->getCurrentLabels(wrk,th->state.FP);
}

ASFUNCTIONBODY_ATOM(MovieClip,_getIsPlaying)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	asAtomHandler::setBool(ret,!th->state.stop_FP);
}

ASFUNCTIONBODY_ATOM(MovieClip,_constructor)
{
	Sprite::_constructor(ret,wrk,obj,nullptr,0);
}

void MovieClip::afterLegacyInsert()
{
	if(!getConstructIndicator() && !needsActionScript3())
		handleConstruction();
	lastratio = this->Ratio;
	Sprite::afterLegacyInsert();
}

void MovieClip::afterLegacyDelete(bool inskipping)
{
	getSystemState()->stage->AVM1RemoveMouseListener(asAtomHandler::fromObjectNoPrimitive(this));
	getSystemState()->stage->AVM1RemoveKeyboardListener(asAtomHandler::fromObjectNoPrimitive(this));
	Sprite::afterLegacyDelete(inskipping);
}
bool MovieClip::AVM1HandleKeyboardEvent(KeyboardEvent *e)
{
	if (this->actions)
	{
		for (auto it = actions->ClipActionRecords.begin(); it != actions->ClipActionRecords.end(); it++)
		{
			bool exec= false;
			if( (e->type == "keyDown" && it->EventFlags.ClipEventKeyDown) ||
				(e->type == "keyUp" && it->EventFlags.ClipEventKeyDown))
				exec = true;
			else if (e->type == "keyDown" && it->EventFlags.ClipEventKeyPress)
			{
				switch (it->KeyCode)
				{
					case 1:
						exec = it->KeyCode == AS3KeyCode::AS3KEYCODE_LEFT;
						break;
					case 2:
						exec = it->KeyCode == AS3KeyCode::AS3KEYCODE_RIGHT;
						break;
					case 3:
						exec = it->KeyCode == AS3KeyCode::AS3KEYCODE_HOME;
						break;
					case 4:
						exec = it->KeyCode == AS3KeyCode::AS3KEYCODE_END;
						break;
					case 5:
						exec = it->KeyCode == AS3KeyCode::AS3KEYCODE_INSERT;
						break;
					case 6:
						exec = it->KeyCode == AS3KeyCode::AS3KEYCODE_DELETE;
						break;
					case 8:
						exec = it->KeyCode == AS3KeyCode::AS3KEYCODE_BACKSPACE;
						break;
					case 13:
						exec = it->KeyCode == AS3KeyCode::AS3KEYCODE_ENTER;
						break;
					case 14:
						exec = it->KeyCode == AS3KeyCode::AS3KEYCODE_UP;
						break;
					case 15:
						exec = it->KeyCode == AS3KeyCode::AS3KEYCODE_DOWN;
						break;
					case 16:
						exec = it->KeyCode == AS3KeyCode::AS3KEYCODE_PAGE_UP;
						break;
					case 17:
						exec = it->KeyCode == AS3KeyCode::AS3KEYCODE_PAGE_DOWN;
						break;
					case 18:
						exec = it->KeyCode == AS3KeyCode::AS3KEYCODE_TAB;
						break;
					case 19:
						exec = it->KeyCode == AS3KeyCode::AS3KEYCODE_ESCAPE;
						break;
					default:
						exec = it->KeyCode == e->getCharCode();
						break;
				}
			}
			if (exec)
			{
				ACTIONRECORD::executeActions(this,this->AVM1getCurrentFrameContext(),it->actions,it->startactionpos);
			}
		}
	}
	Sprite::AVM1HandleKeyboardEvent(e);
	return false;
}
bool MovieClip::AVM1HandleMouseEvent(EventDispatcher *dispatcher, MouseEvent *e)
{
	if (!this->isOnStage() || !this->enabled)
		return false;
	if (dispatcher->is<DisplayObject>())
	{
		DisplayObject* dispobj=nullptr;
		if (dispatcher == this)
			dispobj=this;
		else
		{
			number_t x,y,xg,yg;
			// TODO: Add overloads for Vector2f.
			dispatcher->as<DisplayObject>()->localToGlobal(e->localX,e->localY,xg,yg);
			this->globalToLocal(xg,yg,x,y);
			_NR<DisplayObject> d =hitTest(Vector2f(xg,yg), Vector2f(x,y), MOUSE_CLICK_HIT,true);
			dispobj = d.getPtr();
		}
		if (actions)
		{
			for (auto it = actions->ClipActionRecords.begin(); it != actions->ClipActionRecords.end(); it++)
			{
				// according to https://docstore.mik.ua/orelly/web2/action/ch10_09.htm
				// mouseUp/mouseDown/mouseMove events are sent to all MovieClips on the Stage
				if( (e->type == "mouseDown" && it->EventFlags.ClipEventMouseDown)
					|| (e->type == "mouseUp" && it->EventFlags.ClipEventMouseUp)
					|| (e->type == "mouseMove" && it->EventFlags.ClipEventMouseMove)
					)
				{
					ACTIONRECORD::executeActions(this,this->AVM1getCurrentFrameContext(),it->actions,it->startactionpos);
				}
				if (this->dragged && it->EventFlags.ClipEventRelease && e->type == "mouseUp")
				{
					ACTIONRECORD::executeActions(this,this->AVM1getCurrentFrameContext(),it->actions,it->startactionpos);
				}
				else if( dispobj &&
					((e->type == "mouseUp" && it->EventFlags.ClipEventRelease && !this->dragged)
					|| (e->type == "mouseDown" && it->EventFlags.ClipEventPress)
					|| (e->type == "rollOver" && it->EventFlags.ClipEventRollOver)
					|| (e->type == "rollOut" && it->EventFlags.ClipEventRollOut)
					|| (e->type == "releaseOutside" && it->EventFlags.ClipEventReleaseOutside)
					))
				{
					ACTIONRECORD::executeActions(this,this->AVM1getCurrentFrameContext(),it->actions,it->startactionpos);
				}
			}
		}
		AVM1HandleMouseEventStandard(dispobj,e);
	}
	return false;
}
void MovieClip::AVM1HandleEvent(EventDispatcher *dispatcher, Event* e)
{
	if (this->actions)
	{
		DisplayObject* p = this;
		while (p && p != dispatcher)
			p = p->getParent();
		if (p == dispatcher)
		{
			for (auto it = actions->ClipActionRecords.begin(); it != actions->ClipActionRecords.end(); it++)
			{
				if (e->type == "complete" && it->EventFlags.ClipEventLoad)
				{
					ACTIONRECORD::executeActions(this,this->AVM1getCurrentFrameContext(),it->actions,it->startactionpos);
				}
			}
		}
	}
	if (this->loaderInfo && dispatcher == this->loaderInfo && this->loaderInfo->getContent()==this)
	{
		if (this->actions)
		{
			for (auto it = actions->ClipActionRecords.begin(); it != actions->ClipActionRecords.end(); it++)
			{
				if (e->type == "unload" && it->EventFlags.ClipEventUnload)
				{
					ACTIONRECORD::executeActions(this,this->AVM1getCurrentFrameContext(),it->actions,it->startactionpos);
				}
			}
		}
		if (e->type == "unload")
		{
			if (!this->is<DisplayObject>() || this->is<MovieClip>())
			{
				asAtom func=asAtomHandler::invalidAtom;
				multiname m(nullptr);
				m.name_type=multiname::NAME_STRING;
				m.isAttribute = false;
				asAtom ret=asAtomHandler::invalidAtom;
				asAtom obj = asAtomHandler::fromObject(this);
				ASWorker* wrk = getInstanceWorker();
				m.name_s_id=BUILTIN_STRINGS::STRING_ONUNLOAD;
				AVM1getVariableByMultiname(func,m,GET_VARIABLE_OPTION::NONE,wrk);
				if (asAtomHandler::is<AVM1Function>(func))
					asAtomHandler::as<AVM1Function>(func)->call(&ret,&obj,nullptr,0);
				ASATOM_DECREF(func);
			}
		}
	}
}

void MovieClip::AVM1AfterAdvance()
{
	state.frameadvanced=false;
	state.last_FP=state.FP;
	state.explicit_FP=false;
}


void MovieClip::setupActions(const CLIPACTIONS &clipactions)
{
	actions = &clipactions;
	if (clipactions.AllEventFlags.ClipEventMouseDown ||
			clipactions.AllEventFlags.ClipEventMouseMove ||
			clipactions.AllEventFlags.ClipEventRollOver ||
			clipactions.AllEventFlags.ClipEventRollOut ||
			clipactions.AllEventFlags.ClipEventPress ||
			clipactions.AllEventFlags.ClipEventMouseUp)
	{
		setMouseEnabled(true);
		getSystemState()->stage->AVM1AddMouseListener(asAtomHandler::fromObjectNoPrimitive(this));
		avm1mouselistenercount++;
	}
	if (clipactions.AllEventFlags.ClipEventKeyDown ||
		clipactions.AllEventFlags.ClipEventKeyUp ||
		clipactions.AllEventFlags.ClipEventKeyPress)
		getSystemState()->stage->AVM1AddKeyboardListener(asAtomHandler::fromObjectNoPrimitive(this));

	if (clipactions.AllEventFlags.ClipEventLoad ||
			clipactions.AllEventFlags.ClipEventUnload)
		getSystemState()->stage->AVM1AddEventListener(this);
	if (clipactions.AllEventFlags.ClipEventEnterFrame)
	{
		getSystemState()->registerFrameListener(this);
		getSystemState()->stage->AVM1AddEventListener(this);
	}
}

void MovieClip::AVM1SetupMethods(Class_base* c)
{
	InteractiveObject::AVM1SetupMethods(c);
	c->setDeclaredMethodByQName("attachMovie","",c->getSystemState()->getBuiltinFunction(AVM1AttachMovie),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("loadMovie","",c->getSystemState()->getBuiltinFunction(AVM1LoadMovie),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("unloadMovie","",c->getSystemState()->getBuiltinFunction(AVM1UnloadMovie),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("loadMovieNum","",c->getSystemState()->getBuiltinFunction(AVM1LoadMovieNum),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("createEmptyMovieClip","",c->getSystemState()->getBuiltinFunction(AVM1CreateEmptyMovieClip),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("removeMovieClip","",c->getSystemState()->getBuiltinFunction(AVM1RemoveMovieClip),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("duplicateMovieClip","",c->getSystemState()->getBuiltinFunction(AVM1DuplicateMovieClip),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("clear","",c->getSystemState()->getBuiltinFunction(AVM1Clear),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("moveTo","",c->getSystemState()->getBuiltinFunction(AVM1MoveTo),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("lineTo","",c->getSystemState()->getBuiltinFunction(AVM1LineTo),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("lineStyle","",c->getSystemState()->getBuiltinFunction(AVM1LineStyle),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("beginFill","",c->getSystemState()->getBuiltinFunction(AVM1BeginFill),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("beginGradientFill","",c->getSystemState()->getBuiltinFunction(AVM1BeginGradientFill),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("endFill","",c->getSystemState()->getBuiltinFunction(AVM1EndFill),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("useHandCursor","",c->getSystemState()->getBuiltinFunction(Sprite::_getter_useHandCursor),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("useHandCursor","",c->getSystemState()->getBuiltinFunction(Sprite::_setter_useHandCursor),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("getNextHighestDepth","",c->getSystemState()->getBuiltinFunction(AVM1GetNextHighestDepth),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("attachBitmap","",c->getSystemState()->getBuiltinFunction(AVM1AttachBitmap),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("gotoAndStop","",c->getSystemState()->getBuiltinFunction(gotoAndStop),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("gotoAndPlay","",c->getSystemState()->getBuiltinFunction(gotoAndPlay),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("gotoandstop","",c->getSystemState()->getBuiltinFunction(gotoAndStop),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("gotoandplay","",c->getSystemState()->getBuiltinFunction(gotoAndPlay),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("stop","",c->getSystemState()->getBuiltinFunction(stop),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("play","",c->getSystemState()->getBuiltinFunction(play),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getInstanceAtDepth","",c->getSystemState()->getBuiltinFunction(AVM1getInstanceAtDepth),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getSWFVersion","",c->getSystemState()->getBuiltinFunction(AVM1getSWFVersion),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("menu","",c->getSystemState()->getBuiltinFunction(_getter_contextMenu),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("menu","",c->getSystemState()->getBuiltinFunction(_setter_contextMenu),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("prevFrame","",c->getSystemState()->getBuiltinFunction(prevFrame),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("nextFrame","",c->getSystemState()->getBuiltinFunction(nextFrame),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("createTextField","",c->getSystemState()->getBuiltinFunction(AVM1CreateTextField),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("enabled","",c->getSystemState()->getBuiltinFunction(InteractiveObject::_getMouseEnabled,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("enabled","",c->getSystemState()->getBuiltinFunction(InteractiveObject::_setMouseEnabled),SETTER_METHOD,true);

	c->prototype->setDeclaredMethodByQName("attachMovie","",c->getSystemState()->getBuiltinFunction(AVM1AttachMovie),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("loadMovie","",c->getSystemState()->getBuiltinFunction(AVM1LoadMovie),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("unloadMovie","",c->getSystemState()->getBuiltinFunction(AVM1UnloadMovie),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("loadMovieNum","",c->getSystemState()->getBuiltinFunction(AVM1LoadMovieNum),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("createEmptyMovieClip","",c->getSystemState()->getBuiltinFunction(AVM1CreateEmptyMovieClip),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("removeMovieClip","",c->getSystemState()->getBuiltinFunction(AVM1RemoveMovieClip),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("duplicateMovieClip","",c->getSystemState()->getBuiltinFunction(AVM1DuplicateMovieClip),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("clear","",c->getSystemState()->getBuiltinFunction(AVM1Clear),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("moveTo","",c->getSystemState()->getBuiltinFunction(AVM1MoveTo),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("lineTo","",c->getSystemState()->getBuiltinFunction(AVM1LineTo),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("curveTo","",c->getSystemState()->getBuiltinFunction(AVM1CurveTo),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("lineStyle","",c->getSystemState()->getBuiltinFunction(AVM1LineStyle),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("beginFill","",c->getSystemState()->getBuiltinFunction(AVM1BeginFill),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("beginGradientFill","",c->getSystemState()->getBuiltinFunction(AVM1BeginGradientFill),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("endFill","",c->getSystemState()->getBuiltinFunction(AVM1EndFill),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("useHandCursor","",c->getSystemState()->getBuiltinFunction(Sprite::_getter_useHandCursor),GETTER_METHOD,false);
	c->prototype->setDeclaredMethodByQName("useHandCursor","",c->getSystemState()->getBuiltinFunction(Sprite::_setter_useHandCursor),SETTER_METHOD,false);
	c->prototype->setDeclaredMethodByQName("getNextHighestDepth","",c->getSystemState()->getBuiltinFunction(AVM1GetNextHighestDepth),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("attachBitmap","",c->getSystemState()->getBuiltinFunction(AVM1AttachBitmap),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("gotoAndStop","",c->getSystemState()->getBuiltinFunction(gotoAndStop),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("gotoAndPlay","",c->getSystemState()->getBuiltinFunction(gotoAndPlay),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("gotoandstop","",c->getSystemState()->getBuiltinFunction(gotoAndStop),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("gotoandplay","",c->getSystemState()->getBuiltinFunction(gotoAndPlay),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("stop","",c->getSystemState()->getBuiltinFunction(stop),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("play","",c->getSystemState()->getBuiltinFunction(play),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("getInstanceAtDepth","",c->getSystemState()->getBuiltinFunction(AVM1getInstanceAtDepth),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("getSWFVersion","",c->getSystemState()->getBuiltinFunction(AVM1getSWFVersion),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("menu","",c->getSystemState()->getBuiltinFunction(_getter_contextMenu),GETTER_METHOD,false);
	c->prototype->setDeclaredMethodByQName("menu","",c->getSystemState()->getBuiltinFunction(_setter_contextMenu),SETTER_METHOD,false);
	c->prototype->setDeclaredMethodByQName("prevFrame","",c->getSystemState()->getBuiltinFunction(prevFrame),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("nextFrame","",c->getSystemState()->getBuiltinFunction(nextFrame),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("createTextField","",c->getSystemState()->getBuiltinFunction(AVM1CreateTextField),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("enabled","",c->getSystemState()->getBuiltinFunction(InteractiveObject::_getMouseEnabled,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
	c->prototype->setDeclaredMethodByQName("enabled","",c->getSystemState()->getBuiltinFunction(InteractiveObject::_setMouseEnabled),SETTER_METHOD,false);
}

void MovieClip::AVM1ExecuteFrameActionsFromLabel(const tiny_string &label)
{
	uint32_t dest=framecontainer->getFrameIdByLabel(label, "");
	if(dest==FRAME_NOT_FOUND)
	{
		LOG(LOG_INFO, "AVM1ExecuteFrameActionsFromLabel: label not found:" <<label);
		return;
	}
	framecontainer->AVM1ExecuteFrameActions(dest,this);
}

ASFUNCTIONBODY_ATOM(MovieClip,AVM1AttachMovie)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	if (argslen != 3 && argslen != 4)
	{
		LOG(LOG_ERROR,"AVM1: invalid number of arguments for attachMovie");
		return;
	}
	int Depth = asAtomHandler::toInt(args[2]);
	ApplicationDomain* appDomain = th->loadedFrom;
	auto it = wrk->AVM1callStack.rbegin();
	while (it != wrk->AVM1callStack.rend())
	{
		if ((*it)->getScope()->getLocalsPtr()->is<DisplayObject>())
		{
			appDomain = (*it)->getScope()->getLocalsPtr()->as<DisplayObject>()->loadedFrom;
			break;
		}
		++it;
	}
	uint32_t nameId = asAtomHandler::toStringId(args[1],wrk);

	DictionaryTag* placedTag = appDomain->dictionaryLookupByName(asAtomHandler::toStringId(args[0],wrk),true);
	if (!placedTag)
	{
		ret=asAtomHandler::undefinedAtom;
		return;
	}
	ASObject *instance = placedTag->instance();
	DisplayObject* toAdd=dynamic_cast<DisplayObject*>(instance);
	if(!toAdd)
	{
		if (instance)
			LOG(LOG_NOT_IMPLEMENTED, "AVM1: attachMovie adding non-DisplayObject to display list:"<<instance->toDebugString());
		else
			LOG(LOG_NOT_IMPLEMENTED, "AVM1: attachMovie couldn't create instance of item:"<<placedTag->getId());
		return;
	}
	toAdd->name = nameId;
	if (toAdd->is<MovieClip>())
	{
		toAdd->as<MovieClip>()->inAVM1Attachment=true;
		toAdd->as<MovieClip>()->advanceFrame(true);
	}
	if (argslen == 4)
	{
		ASObject* o = asAtomHandler::getObject(args[3]);
		if (o)
			o->copyValues(toAdd,wrk);
	}
	if(th->hasLegacyChildAt(Depth) )
	{
		th->deleteLegacyChildAt(Depth,false);
		th->insertLegacyChildAt(Depth,toAdd,false,false);
	}
	else
		th->insertLegacyChildAt(Depth,toAdd,false,false);
	if (toAdd->is<MovieClip>())
		toAdd->as<MovieClip>()->inAVM1Attachment=false;
	toAdd->constructionComplete();
	toAdd->afterConstruction();
	toAdd->incRef();
	if (argslen == 4)
	{
		// update all bindings _after_ the clip is constructed
		ASObject* o = asAtomHandler::getObject(args[3]);
		if (o)
			o->AVM1UpdateAllBindings(toAdd,wrk);
	}
	ret=asAtomHandler::fromObjectNoPrimitive(toAdd);
}
ASFUNCTIONBODY_ATOM(MovieClip,AVM1CreateEmptyMovieClip)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	if (argslen < 2)
	{
		LOG(LOG_ERROR,"AVM1: invalid number of arguments for CreateEmptyMovieClip");
		return;
	}
	int Depth = asAtomHandler::toInt(args[1]);
	uint32_t nameId = asAtomHandler::toStringId(args[0],wrk);
	multiname m(nullptr);
	m.name_type=multiname::NAME_STRING;
	m.name_s_id=nameId;
	if (th->hasPropertyByMultiname(m,true,false,wrk))
	{
		asAtom oldclip = asAtomHandler::invalidAtom;
		th->getVariableByMultiname(oldclip,m,GET_VARIABLE_OPTION::NONE,wrk);
		if (asAtomHandler::is<DisplayObject>(oldclip))
			th->_removeChild(asAtomHandler::as<DisplayObject>(oldclip));
		ASATOM_DECREF(oldclip);
	}
	AVM1MovieClip* toAdd= Class<AVM1MovieClip>::getInstanceSNoArgs(wrk);
	toAdd->loadedFrom = th->loadedFrom;
	toAdd->name = nameId;
	toAdd->setMouseEnabled(true);
	m.name_type=multiname::NAME_STRING;
	m.name_s_id=BUILTIN_STRINGS::STRING_LOCKROOT;
	toAdd->setVariableByMultiname(m,asAtomHandler::falseAtom,CONST_ALLOWED,nullptr,wrk);
	if(th->hasLegacyChildAt(Depth) )
	{
		th->deleteLegacyChildAt(Depth,false);
		th->insertLegacyChildAt(Depth,toAdd,false,false);
	}
	else
		th->insertLegacyChildAt(Depth,toAdd,false,false);
	toAdd->setNameOnParent();
	toAdd->constructionComplete();
	toAdd->afterConstruction();
	toAdd->isAVM1Loaded=true;
	toAdd->incRef();
	ret=asAtomHandler::fromObject(toAdd);
}
ASFUNCTIONBODY_ATOM(MovieClip,AVM1RemoveMovieClip)
{
	if (!asAtomHandler::is<MovieClip>(obj))
		return;
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	if (th->getParent() && !th->is<RootMovieClip>())
	{
		if (th->name != BUILTIN_STRINGS::EMPTY && th->name != UINT32_MAX)
		{
			multiname m(nullptr);
			m.name_type=multiname::NAME_STRING;
			m.name_s_id=th->name;
			m.ns.emplace_back(wrk->getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
			m.isAttribute = false;
			// don't remove the child by name here because another DisplayObject may have been added with this name after this clip was added
			th->getParent()->deleteVariableByMultinameWithoutRemovingChild(m,wrk);
		}
		th->getParent()->_removeChild(th);
	}
}
ASFUNCTIONBODY_ATOM(MovieClip,AVM1DuplicateMovieClip)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	if (argslen < 2)
	{
		LOG(LOG_ERROR,"AVM1: invalid number of arguments for DuplicateMovieClip");
		return;
	}
	if (!th->getParent())
	{
		LOG(LOG_ERROR,"calling DuplicateMovieClip on clip without parent");
		ret = asAtomHandler::undefinedAtom;
		return;
	}
	int Depth = asAtomHandler::toInt(args[1]);
	ASObject* initobj = nullptr;
	if (argslen > 2)
		initobj = asAtomHandler::toObject(args[2],wrk);
	MovieClip* toAdd=th->AVM1CloneSprite(args[0],Depth,initobj);
	toAdd->incRef();
	ret=asAtomHandler::fromObject(toAdd);
}
MovieClip* MovieClip::AVM1CloneSprite(asAtom target, uint32_t Depth,ASObject* initobj)
{
	uint32_t nameId = asAtomHandler::toStringId(target,getInstanceWorker());
	AVM1MovieClip* toAdd=nullptr;
	DefineSpriteTag* tag = getTagID() != UINT32_MAX ? (DefineSpriteTag*)loadedFrom->dictionaryLookup(getTagID()) : nullptr;
	if (tag)
		toAdd= tag->instance()->as<AVM1MovieClip>();
	else
		toAdd= Class<AVM1MovieClip>::getInstanceSNoArgs(getInstanceWorker());

	if (initobj)
		initobj->copyValues(toAdd,getInstanceWorker());
	toAdd->loadedFrom=this->loadedFrom;
	toAdd->setLegacyMatrix(getMatrix());
	toAdd->colorTransform = this->colorTransform;
	toAdd->name = nameId;
	if (this->actions)
		toAdd->setupActions(*actions);
	if(getParent()->hasLegacyChildAt(Depth))
	{
		getParent()->deleteLegacyChildAt(Depth,false);
		getParent()->insertLegacyChildAt(Depth,toAdd,false,false);
	}
	else
		getParent()->insertLegacyChildAt(Depth,toAdd,false,false);
	toAdd->constructionComplete();
	toAdd->afterConstruction();
	if (state.creatingframe)
		toAdd->advanceFrame(true);
	return toAdd;
}

string MovieClip::toDebugString() const
{
	std::string res = Sprite::toDebugString();
	res += " state=";
	char buf[100];
	sprintf(buf,"%d/%u/%u%s",state.last_FP,state.FP,state.next_FP,state.stop_FP?" stopped":"");
	res += buf;
	return res;
}
ASFUNCTIONBODY_ATOM(MovieClip,AVM1Clear)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	Graphics* g = th->getGraphics();
	asAtom o = asAtomHandler::fromObject(g);
	Graphics::clear(ret,wrk,o,args,argslen);
}
ASFUNCTIONBODY_ATOM(MovieClip,AVM1MoveTo)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	Graphics* g = th->getGraphics();
	asAtom o = asAtomHandler::fromObject(g);
	Graphics::moveTo(ret,wrk,o,args,argslen);
}
ASFUNCTIONBODY_ATOM(MovieClip,AVM1LineTo)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	Graphics* g = th->getGraphics();
	asAtom o = asAtomHandler::fromObject(g);
	Graphics::lineTo(ret,wrk,o,args,argslen);
}
ASFUNCTIONBODY_ATOM(MovieClip,AVM1CurveTo)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	Graphics* g = th->getGraphics();
	asAtom o = asAtomHandler::fromObject(g);
	Graphics::curveTo(ret,wrk,o,args,argslen);
}

ASFUNCTIONBODY_ATOM(MovieClip,AVM1LineStyle)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	Graphics* g = th->getGraphics();
	asAtom o = asAtomHandler::fromObject(g);
	Graphics::lineStyle(ret,wrk,o,args,argslen);
}
ASFUNCTIONBODY_ATOM(MovieClip,AVM1BeginFill)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	Graphics* g = th->getGraphics();
	asAtom o = asAtomHandler::fromObject(g);
	if(argslen>=2)
		args[1]=asAtomHandler::fromNumber(wrk,asAtomHandler::toNumber(args[1])/100.0,false);

	Graphics::beginFill(ret,wrk,o,args,argslen);
}
ASFUNCTIONBODY_ATOM(MovieClip,AVM1BeginGradientFill)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	Graphics* g = th->getGraphics();
	asAtom o = asAtomHandler::fromObject(g);
	Graphics::beginGradientFill(ret,wrk,o,args,argslen);
}
ASFUNCTIONBODY_ATOM(MovieClip,AVM1EndFill)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	Graphics* g = th->getGraphics();
	asAtom o = asAtomHandler::fromObject(g);
	Graphics::endFill(ret,wrk,o,args,argslen);
}
ASFUNCTIONBODY_ATOM(MovieClip,AVM1GetNextHighestDepth)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	uint32_t n = th->getMaxLegacyChildDepth();
	asAtomHandler::setUInt(ret,wrk,n == UINT32_MAX ? 0 : n+1);
}
ASFUNCTIONBODY_ATOM(MovieClip,AVM1AttachBitmap)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	if (argslen < 2)
		throw RunTimeException("AVM1: invalid number of arguments for attachBitmap");
	int Depth = asAtomHandler::toInt(args[1]);
	if (!asAtomHandler::is<BitmapData>(args[0]))
	{
		LOG(LOG_ERROR,"AVM1AttachBitmap invalid type:"<<asAtomHandler::toDebugString(args[0]));
		throw RunTimeException("AVM1: attachBitmap first parameter is no BitmapData");
	}

	AVM1BitmapData* data = asAtomHandler::as<AVM1BitmapData>(args[0]);
	data->incRef();
	Bitmap* toAdd = Class<AVM1Bitmap>::getInstanceS(wrk,_MR(data));
	if (argslen > 2)
		toAdd->pixelSnapping = asAtomHandler::toString(args[2],wrk);
	if (argslen > 3)
		toAdd->smoothing = asAtomHandler::Boolean_concrete(args[3]);
	if(th->hasLegacyChildAt(Depth) )
	{
		th->deleteLegacyChildAt(Depth,false);
		th->insertLegacyChildAt(Depth,toAdd,false,false);
	}
	else
		th->insertLegacyChildAt(Depth,toAdd,false,false);
	toAdd->incRef();
	ret=asAtomHandler::fromObject(toAdd);
}
ASFUNCTIONBODY_ATOM(MovieClip,AVM1getInstanceAtDepth)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	int32_t depth;
	ARG_CHECK(ARG_UNPACK(depth));
	if (th->hasLegacyChildAt(depth))
	{
		DisplayObject* o = th->getLegacyChildAt(depth);
		o->incRef();
		ret = asAtomHandler::fromObjectNoPrimitive(o);
	}
	else
		ret = asAtomHandler::undefinedAtom;
}
ASFUNCTIONBODY_ATOM(MovieClip,AVM1getSWFVersion)
{
	asAtomHandler::setUInt(ret,wrk,wrk->getSystemState()->getSwfVersion());
}

ASFUNCTIONBODY_ATOM(MovieClip,AVM1LoadMovie)
{
	AVM1MovieClip* th=asAtomHandler::as<AVM1MovieClip>(obj);
	tiny_string url;
	tiny_string method;
	ARG_CHECK(ARG_UNPACK(url,"")(method,"GET"));

	auto l = Class<AVM1MovieClipLoader>::getInstanceSNoArgs(wrk);
	wrk->getSystemState()->stage->AVM1AddEventListener(l);
	l->addStoredMember();
	th->avm1loaderlist.push_back(l);
	l->load(url,method,th);
}
ASFUNCTIONBODY_ATOM(MovieClip,AVM1UnloadMovie)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	if (th->loaderInfo && th->loaderInfo->hasAVM1Target())
		th->loaderInfo->getLoader()->unload();
	th->setOnStage(false,false);
	th->tokens=nullptr;
}
ASFUNCTIONBODY_ATOM(MovieClip,AVM1LoadMovieNum)
{
	AVM1MovieClip* th=asAtomHandler::as<AVM1MovieClip>(obj);
	tiny_string url;
	tiny_string method;
	int32_t level;
	ARG_CHECK(ARG_UNPACK(url,"")(level,0)(method,"GET"));

	auto l = Class<AVM1MovieClipLoader>::getInstanceSNoArgs(wrk);
	wrk->getSystemState()->stage->AVM1AddEventListener(l);
	l->addStoredMember();
	th->avm1loaderlist.push_back(l);
	l->load(url,method,th,level);
}
ASFUNCTIONBODY_ATOM(MovieClip,AVM1CreateTextField)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	tiny_string instanceName;
	int depth;
	int x;
	int y;
	uint32_t width;
	uint32_t height;
	ARG_CHECK(ARG_UNPACK(instanceName)(depth)(x)(y)(width)(height));

	AVM1TextField* tf = Class<AVM1TextField>::getInstanceS(wrk);
	tf->loadedFrom = th->loadedFrom;
	tf->name = wrk->getSystemState()->getUniqueStringId(instanceName);
	tf->setX(x);
	tf->setY(y);
	tf->width = width;
	tf->height = height;
	if (th->hasLegacyChildAt(depth))
		th->deleteLegacyChildAt(depth, false);
	th->insertLegacyChildAt(depth, tf, false, false);
	if (wrk->getSystemState()->getSwfVersion() >= 8)
	{
		tf->incRef();
		ret = asAtomHandler::fromObjectNoPrimitive(tf);
	}
}

void MovieClip::AVM1HandleConstruction(bool forInitAction)
{
	if (needsActionScript3() || inAVM1Attachment || constructorCallComplete)
		return;
	setConstructIndicator();
	if (!forInitAction)
		getSystemState()->stage->AVM1AddDisplayObject(this);
	setConstructorCallComplete();
	AVM1Function* constr = this->getInstanceWorker()->AVM1getClassConstructor(this);
	if (constr)
	{
		asAtom pr = constr->getprop_prototypeAtom();
		if (!asAtomHandler::isObject(pr))
			pr = constr->prototype;
		setprop_prototype(pr);
		setprop_prototype(pr, BUILTIN_STRINGS::STRING_PROTO);
		asAtom ret = asAtomHandler::invalidAtom;
		asAtom obj = asAtomHandler::fromObjectNoPrimitive(this);
		constr->call(&ret,&obj,nullptr,0);
		AVM1registerPrototypeListeners();
	}
	if (this != getSystemState()->mainClip)
		afterConstruction();
}
/* Go through the hierarchy and add all
 * legacy objects which are new in the current
 * frame top-down. At the same time, call their
 * constructors in reverse order (bottom-up).
 * This is called in vm's thread context */
void MovieClip::declareFrame(bool implicit)
{
	if (state.frameadvanced && (implicit || needsActionScript3()))
		return;
	/* Go through the list of frames.
	 * If our next_FP is after our current,
	 * we construct all frames from current
	 * to next_FP.
	 * If our next_FP is before our current,
	 * we purge all objects on the 0th frame
	 * and then construct all frames from
	 * the 0th to the next_FP.
	 * We also will run the constructor on objects that got placed and deleted
	 * before state.FP (which may get us an segfault).
	 *
	 */
	if((int)state.FP < state.last_FP)
	{
		purgeLegacyChildren(implicit);
		resetToStart();
	}
	//Declared traits must exists before legacy objects are added
	if (getClass())
		getClass()->setupDeclaredTraits(this);

	bool newFrame = (int)state.FP != state.last_FP;
	if (newFrame ||!state.frameadvanced)
	{
		framecontainer->declareFrame(this);
		if (newFrame)
			state.frameadvanced=true;
	}
	// remove all legacy objects that have not been handled in the PlaceObject/RemoveObject tags
	LegacyChildEraseDeletionMarked();
	if (needsActionScript3())
		DisplayObjectContainer::declareFrame(implicit);
}
void MovieClip::AVM1AddScriptEvents()
{
	if (actions)
	{
		for (auto it = actions->ClipActionRecords.begin(); it != actions->ClipActionRecords.end(); it++)
		{
			if ((it->EventFlags.ClipEventLoad && !isAVM1Loaded) ||
				(it->EventFlags.ClipEventEnterFrame && isAVM1Loaded))
			{
				AVM1scriptToExecute script;
				script.actions = &(*it).actions;
				script.startactionpos = (*it).startactionpos;
				script.avm1context = this->AVM1getCurrentFrameContext();
				script.event_name_id = UINT32_MAX;
				script.isEventScript = true;
				this->incRef(); // will be decreffed after script handler was executed
				script.clip=this;
				getSystemState()->stage->AVM1AddScriptToExecute(script);
			}
		}
	}
	AVM1scriptToExecute script;
	script.actions = nullptr;
	script.startactionpos = 0;
	script.avm1context = nullptr;
	this->incRef(); // will be decreffed after script handler was executed
	script.clip=this;
	script.event_name_id = isAVM1Loaded ? BUILTIN_STRINGS::STRING_ONENTERFRAME : BUILTIN_STRINGS::STRING_ONLOAD;
	script.isEventScript = true;
	getSystemState()->stage->AVM1AddScriptToExecute(script);

	isAVM1Loaded=true;
}
void MovieClip::initFrame()
{
	if (!needsActionScript3())
		return;
	if (!isConstructed() && !state.stop_FP && state.last_FP==-1)
	{
		// ensure frame is advanced in first tick after construction
		state.next_FP=1;
	}
	/* Set last_FP to reflect the frame that we have initialized currently.
	 * This must be set before the constructor of this MovieClip is run,
	 * or it will call initFrame(). */
	state.last_FP=state.FP;

	/* call our own constructor, if necassary */
	Sprite::initFrame();
	state.creatingframe=false;
}
void MovieClip::updateVariableBindings()
{
	auto itbind = variablebindings.begin();
	while (itbind != variablebindings.end())
	{
		asAtom v = getVariableBindingValue(getSystemState()->getStringFromUniqueId((*itbind).first));
		(*itbind).second->UpdateVariableBinding(v);
		ASATOM_DECREF(v);
		itbind++;
	}
}
void MovieClip::executeFrameScript()
{
	if (!needsActionScript3())
		return;
	state.explicit_FP=false;
	uint32_t f = frameScripts.count(state.FP) ? state.FP : UINT32_MAX;
	if (f != UINT32_MAX && !markedForLegacyDeletion && !inExecuteFramescript)
	{
		if (lastFrameScriptExecuted != f)
		{
			lastFrameScriptExecuted = f;
			inExecuteFramescript = true;
			this->getInstanceWorker()->rootClip->executingFrameScriptCount++;
			asAtom v=asAtomHandler::invalidAtom;
			asAtom closure_this = asAtomHandler::as<IFunction>(frameScripts[f])->closure_this;
			if (asAtomHandler::isInvalid(closure_this))
				closure_this = getInstanceWorker()->getCurrentGlobalAtom(asAtomHandler::nullAtom);
			ASATOM_INCREF(closure_this);
			asAtom obj = closure_this;
			ASATOM_INCREF(frameScripts[f]);
			try
			{
				asAtomHandler::callFunction(frameScripts[f],getInstanceWorker(),v,obj,nullptr,0,false);
			}
			catch(ASObject*& e)
			{
				setStopped();
				ASATOM_DECREF(frameScripts[f]);
				ASATOM_DECREF(v);
				ASATOM_DECREF(closure_this);
				this->getInstanceWorker()->rootClip->executingFrameScriptCount--;
				inExecuteFramescript = false;
				throw;
			}
			ASATOM_DECREF(frameScripts[f]);
			ASATOM_DECREF(v);
			ASATOM_DECREF(closure_this);
			this->getInstanceWorker()->rootClip->executingFrameScriptCount--;
			inExecuteFramescript = false;
		}
	}

	if (state.gotoQueued)
	{
		state.gotoQueued=false;
		runGoto(true);
	}
	Sprite::executeFrameScript();
}

void MovieClip::checkRatio(uint32_t ratio, bool inskipping)
{
	// according to http://wahlers.com.br/claus/blog/hacking-swf-2-placeobject-and-ratio/
	// if the ratio value is different from the previous ratio value for this MovieClip, this clip is resetted to frame 0
	if (ratio != 0 && ratio != lastratio && !state.stop_FP)
	{
		this->state.next_FP=0;
	}
	lastratio=ratio;
}


void MovieClip::enterFrame(bool implicit)
{
	std::vector<_R<DisplayObject>> list;
	cloneDisplayList(list);
	for (auto it = list.rbegin(); it != list.rend(); ++it)
	{
		auto child = *it;
		child->skipFrame = skipFrame ? true : child->skipFrame;
		child->enterFrame(implicit);
	}
	if (skipFrame)
	{
		skipFrame = false;
		return;
	}
	if (needsActionScript3() && !state.stop_FP)
	{
		state.inEnterFrame = true;
		advanceFrame(implicit);
		state.inEnterFrame = false;
	}
}

/* Update state.last_FP. If enough frames
 * are available, set state.FP to state.next_FP.
 * This is run in vm's thread context.
 */
void MovieClip::advanceFrame(bool implicit)
{
	if (implicit && !getSystemState()->mainClip->needsActionScript3() && state.frameadvanced && state.last_FP==-1)
		return; // frame was already advanced after construction
	if (!framecontainer)
		framecontainer=new FrameContainer();
	if (state.last_FP!=-1)
		state.advancedByTick=true;
	checkSound(state.next_FP);
	if (state.frameadvanced && state.explicit_FP)
	{
		// frame was advanced more than once in one EnterFrame event, so initFrame was not called
		// set last_FP to the FP set by previous advanceFrame
		state.last_FP=state.FP;
	}
	if (needsActionScript3() || getSystemState()->mainClip->needsActionScript3())
		state.frameadvanced=false;
	state.creatingframe=true;
	/* A MovieClip can only have frames if
	 * 1a. It is a RootMovieClip
	 * 1b. or it is a DefineSpriteTag
	 * 2. and is exported as a subclass of MovieClip (see bindedTo)
	 */
	if(!this->is<RootMovieClip>() && (fromDefineSpriteTag==UINT32_MAX
	   || (!getClass()->isSubClass(Class<MovieClip>::getClass(getSystemState()))
		   && (needsActionScript3() || !getClass()->isSubClass(Class<AVM1MovieClip>::getClass(getSystemState()))))))
	{
		if (int(state.FP) >= state.last_FP && !state.inEnterFrame && implicit) // no need to advance frame if we are moving backwards in the timline, as the timeline will be rebuild anyway
			DisplayObjectContainer::advanceFrame(true);
		declareFrame(implicit);
		updateVariableBindings();
		return;
	}

	//If we have not yet loaded enough frames delay advancement
	if(state.next_FP>=(uint32_t)framecontainer->getFramesLoaded())
	{
		if(hasFinishedLoading())
		{
			if (framecontainer->getFramesLoaded() > 1)
				LOG(LOG_ERROR,"state.next_FP >= getFramesLoaded:"<< state.next_FP<<" "<<framecontainer->getFramesLoaded() <<" "<<toDebugString()<<" "<<getTagID());
			state.next_FP = state.FP;
		}
		else
			return;
	}

	if (state.next_FP != state.FP)
	{
		if (!inExecuteFramescript)
			lastFrameScriptExecuted=UINT32_MAX;
		state.FP=state.next_FP;
	}
	if(implicit && !state.stop_FP && framecontainer->getFramesLoaded()>0)
	{
		if (hasFinishedLoading())
			state.next_FP=imin(state.FP+1,framecontainer->getFramesLoaded()-1);
		else
			state.next_FP=state.FP+1;
		if(hasFinishedLoading() && state.FP == framecontainer->getFramesLoaded()-1)
			state.next_FP = 0;
	}
	// ensure the legacy objects of the current frame are created
	if (int(state.FP) >= state.last_FP && !state.inEnterFrame && implicit) // no need to advance frame if we are moving backwards in the timeline, as the timeline will be rebuild anyway
		DisplayObjectContainer::advanceFrame(true);

	declareFrame(implicit);
	// setting state.frameadvanced ensures that the frame is not declared multiple times
	// if it was set by an actionscript command.
	state.frameadvanced = true;
	markedForLegacyDeletion=false;
	updateVariableBindings();
}

void MovieClip::constructionComplete(bool _explicit, bool forInitAction)
{
	Sprite::constructionComplete(_explicit,forInitAction);

	/* If this object was 'new'ed from AS code, the first
	 * frame has not been initalized yet, so init the frame
	 * now */
	if (!framecontainer)
		framecontainer=new FrameContainer();
	if(state.last_FP == -1
		&& !needsActionScript3()
		&& !forInitAction)
	{
		if (!framecontainer)
			framecontainer=new FrameContainer();
		advanceFrame(true);
		if (getSystemState()->getFramePhase() != FramePhase::ADVANCE_FRAME)
			initFrame();
	}
	AVM1HandleConstruction(forInitAction);
}
void MovieClip::beforeConstruction(bool _explicit)
{
	if(state.last_FP == -1 && needsActionScript3())
	{
		if (!framecontainer)
			framecontainer=new FrameContainer();
		advanceFrame(true);
	}
	Sprite::beforeConstruction(_explicit);
}
void MovieClip::afterConstruction(bool _explicit)
{
	if (needsActionScript3())
		setConstructorCallComplete();
	Sprite::afterConstruction(_explicit);
}

uint32_t MovieClip::getFrameCount()
{
	if (framecontainer)
		return framecontainer->getFramesSize();
	return 0;
}
