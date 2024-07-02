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
#include "parsing/tags.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include "scripting/avm1/avm1display.h"
#include "scripting/avm1/avm1text.h"
#include "scripting/flash/geom/flashgeom.h"
#include "scripting/flash/geom/Point.h"
#include "scripting/flash/geom/Rectangle.h"
#include "scripting/toplevel/AVM1Function.h"
#include "scripting/toplevel/Array.h"
#include "scripting/toplevel/Integer.h"

#define FRAME_NOT_FOUND 0xffffffff //Used by getFrameIdBy*

using namespace std;
using namespace lightspark;

FrameLabel::FrameLabel(ASWorker* wrk,Class_base* c):ASObject(wrk,c)
{
}

FrameLabel::FrameLabel(ASWorker* wrk, Class_base* c, const FrameLabel_data& data):ASObject(wrk,c),FrameLabel_data(data)
{
}

void FrameLabel::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setDeclaredMethodByQName("frame","",c->getSystemState()->getBuiltinFunction(_getFrame),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("name","",c->getSystemState()->getBuiltinFunction(_getName),GETTER_METHOD,true);
}

ASFUNCTIONBODY_ATOM(FrameLabel,_getFrame)
{
	FrameLabel* th=asAtomHandler::as<FrameLabel>(obj);
	asAtomHandler::setUInt(ret,wrk,th->frame);
}

ASFUNCTIONBODY_ATOM(FrameLabel,_getName)
{
	FrameLabel* th=asAtomHandler::as<FrameLabel>(obj);
	ret = asAtomHandler::fromObject(abstract_s(wrk,th->name));
}

/*
 * Adds a frame label to the internal vector and keep
 * the vector sorted with respect to frame
 */
void Scene_data::addFrameLabel(uint32_t frame, const tiny_string& label)
{
	for(vector<FrameLabel_data>::iterator j=labels.begin();
		j != labels.end();++j)
	{
		FrameLabel_data& fl = *j;
		if(fl.frame == frame)
		{
			LOG(LOG_INFO,"existing frame label found:"<<fl.name<<", new value:"<<label);
			fl.name = label;
			return;
		}
		else if(fl.frame > frame)
		{
			labels.insert(j,FrameLabel_data(frame,label));
			return;
		}
	}

	labels.push_back(FrameLabel_data(frame,label));
}

Scene::Scene(ASWorker* wrk,Class_base* c):ASObject(wrk,c)
{
}

Scene::Scene(ASWorker* wrk, Class_base* c, const Scene_data& data, uint32_t _numFrames):ASObject(wrk,c),Scene_data(data),numFrames(_numFrames)
{
}

void Scene::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setDeclaredMethodByQName("labels","",c->getSystemState()->getBuiltinFunction(_getLabels,0,Class<Array>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("name","",c->getSystemState()->getBuiltinFunction(_getName,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("numFrames","",c->getSystemState()->getBuiltinFunction(_getNumFrames,0,Class<Integer>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
}

ASFUNCTIONBODY_ATOM(Scene,_getLabels)
{
	Scene* th=asAtomHandler::as<Scene>(obj);
	Array* res = Class<Array>::getInstanceSNoArgs(wrk);
	res->resize(th->labels.size());
	for(size_t i=0; i<th->labels.size(); ++i)
	{
		asAtom v = asAtomHandler::fromObject(Class<FrameLabel>::getInstanceS(wrk,th->labels[i]));
		res->set(i, v,false,false);
	}
	ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(Scene,_getName)
{
	Scene* th=asAtomHandler::as<Scene>(obj);
	ret = asAtomHandler::fromObject(abstract_s(wrk,th->name));
}

ASFUNCTIONBODY_ATOM(Scene,_getNumFrames)
{
	Scene* th=asAtomHandler::as<Scene>(obj);
	ret = asAtomHandler::fromUInt(th->numFrames);
}

void Frame::destroyTags()
{
	auto it=blueprint.begin();
	for(;it!=blueprint.end();++it)
		delete (*it);
}

void Frame::execute(DisplayObjectContainer* displayList, bool inskipping, std::vector<_R<DisplayObject>>& removedFrameScripts)
{
	auto it=blueprint.begin();
	for(;it!=blueprint.end();++it)
	{
		RemoveObject2Tag* obj = static_cast<RemoveObject2Tag*>(*it);
		if (obj != nullptr && displayList->hasLegacyChildAt(obj->getDepth()))
		{
			DisplayObject* child = displayList->getLegacyChildAt(obj->getDepth());
			child->incRef();
			removedFrameScripts.push_back(_MR(child));
		}
		(*it)->execute(displayList,inskipping);
	}
	displayList->checkClipDepth();
}
void Frame::AVM1executeActions(MovieClip* clip)
{
	auto it=blueprint.begin();
	for(;it!=blueprint.end();++it)
	{
		if ((*it)->getType() == AVM1ACTION_TAG)
			(*it)->execute(clip,false);
	}
}

FrameContainer::FrameContainer():framesLoaded(0)
{
	frames.emplace_back(Frame());
	scenes.resize(1);
}

FrameContainer::FrameContainer(const FrameContainer& f):frames(f.frames),scenes(f.scenes),framesLoaded((int)f.framesLoaded)
{
}

/* This runs in parser thread context,
 * but no locking is needed here as it only accesses the last frame.
 * See comment on the 'frames' member. */
void FrameContainer::addToFrame(DisplayListTag* t)
{
	frames.back().blueprint.push_back(t);
}
/**
 * Find the scene to which the given frame belongs and
 * adds the frame label to that scene.
 * The labels of the scene will stay sorted by frame.
 */
void FrameContainer::addFrameLabel(uint32_t frame, const tiny_string& label)
{
	for(size_t i=0; i<scenes.size();++i)
	{
		if(frame < scenes[i].startframe)
		{
			scenes[i-1].addFrameLabel(frame,label);
			return;
		}
	}
	scenes.back().addFrameLabel(frame,label);
}

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
	c->setDeclaredMethodByQName("stop","",c->getSystemState()->getBuiltinFunction(stop),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("play","",c->getSystemState()->getBuiltinFunction(play),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("gotoAndStop","",c->getSystemState()->getBuiltinFunction(gotoAndStop),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("gotoAndPlay","",c->getSystemState()->getBuiltinFunction(gotoAndPlay),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("prevFrame","",c->getSystemState()->getBuiltinFunction(prevFrame),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("nextFrame","",c->getSystemState()->getBuiltinFunction(nextFrame),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("addFrameScript","",c->getSystemState()->getBuiltinFunction(addFrameScript),NORMAL_METHOD,true);
	REGISTER_GETTER_SETTER_RESULTTYPE(c, enabled,Boolean);
}

ASFUNCTIONBODY_GETTER_SETTER(MovieClip, enabled)

MovieClip::MovieClip(ASWorker* wrk, Class_base* c):Sprite(wrk,c),fromDefineSpriteTag(UINT32_MAX),lastFrameScriptExecuted(UINT32_MAX),lastratio(0),inExecuteFramescript(false)
  ,inAVM1Attachment(false),isAVM1Loaded(false),AVM1EventScriptsAdded(false)
  ,actions(nullptr),totalFrames_unreliable(1),enabled(true)
{
	subtype=SUBTYPE_MOVIECLIP;
}

MovieClip::MovieClip(ASWorker* wrk, Class_base* c, const FrameContainer& f, uint32_t defineSpriteTagID):Sprite(wrk,c),FrameContainer(f),fromDefineSpriteTag(defineSpriteTagID),lastFrameScriptExecuted(UINT32_MAX),lastratio(0),inExecuteFramescript(false)
  ,inAVM1Attachment(false),isAVM1Loaded(false),AVM1EventScriptsAdded(false)
  ,actions(nullptr),totalFrames_unreliable(frames.size()),enabled(true)
{
	subtype=SUBTYPE_MOVIECLIP;
	//For sprites totalFrames_unreliable is the actual frame count
	//For the root movie, it's the frame count from the header
}

bool MovieClip::destruct()
{
	frames.clear();
	inAVM1Attachment=false;
	isAVM1Loaded=false;
	setFramesLoaded(0);
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
	inExecuteFramescript=false;

	scenes.clear();
	setFramesLoaded(0);
	frames.emplace_back(Frame());
	scenes.resize(1);
	state.reset();
	actions=nullptr;

	enabled = true;
	avm1loader.reset();
	return Sprite::destruct();
}

void MovieClip::finalize()
{
	frames.clear();
	auto it = frameScripts.begin();
	while (it != frameScripts.end())
	{
		ASATOM_REMOVESTOREDMEMBER(it->second);
		it++;
	}
	frameScripts.clear();
	scenes.clear();
	state.reset();
	avm1loader.reset();
	Sprite::finalize();
}

void MovieClip::prepareShutdown()
{
	if (preparedforshutdown)
		return;
	Sprite::prepareShutdown();
	auto it = frameScripts.begin();
	while (it != frameScripts.end())
	{
		ASObject* o = asAtomHandler::getObject(it->second);
		if (o)
			o->prepareShutdown();
		it++;
	}
	if (avm1loader)
		avm1loader->prepareShutdown();
}

bool MovieClip::countCylicMemberReferences(garbagecollectorstate &gcstate)
{
	if (gcstate.checkAncestors(this))
		return false;
	bool ret = DisplayObjectContainer::countCylicMemberReferences(gcstate);
	for (auto it = frameScripts.begin(); it != frameScripts.end(); it++)
	{
		ASObject* o = asAtomHandler::getObject(it->second);
		if (o)
			ret = o->countAllCylicMemberReferences(gcstate) || ret;
	}
	return ret;
}
/* Returns a Scene_data pointer for a scene called sceneName, or for
 * the current scene if sceneName is empty. Returns nullptr, if not found.
 */
const Scene_data *MovieClip::getScene(const tiny_string &sceneName) const
{
	if (sceneName.empty())
	{
		return &scenes[getCurrentScene()];
	}
	else
	{
		//Find scene by name
		for (auto it=scenes.begin(); it!=scenes.end(); ++it)
		{
			if (it->name == sceneName)
				return &*it;
		}
	}

	return nullptr;  //Not found!
}

/* Return global frame index for a named frame. If sceneName is not
 * empty, return a frame only if it belong to the named scene.
 */
uint32_t MovieClip::getFrameIdByLabel(const tiny_string& label, const tiny_string& sceneName) const
{
	if (sceneName.empty())
	{
		//Find frame in any scene
		for(size_t i=0;i<scenes.size();++i)
		{
			for(size_t j=0;j<scenes[i].labels.size();++j)
				if(scenes[i].labels[j].name == label || scenes[i].labels[j].name.lowercase() == label.lowercase())
					return scenes[i].labels[j].frame;
		}
	}
	else
	{
		//Find frame in the named scene only
		const Scene_data *scene = getScene(sceneName);
		if (scene)
		{
			for(size_t j=0;j<scene->labels.size();++j)
			{
				if(scene->labels[j].name == label || scene->labels[j].name.lowercase() == label.lowercase())
					return scene->labels[j].frame;
			}
		}
	}

	return FRAME_NOT_FOUND;
}

/* Return global frame index for frame i (zero-based) in a scene
 * called sceneName. If sceneName is empty, use the current scene.
 */
uint32_t MovieClip::getFrameIdByNumber(uint32_t i, const tiny_string& sceneName) const
{
	const Scene_data *sceneData = getScene(sceneName);
	if (!sceneData)
		return FRAME_NOT_FOUND;

	//Should we check if the scene has at least i frames?
	return sceneData->startframe + i;
}

ASFUNCTIONBODY_ATOM(MovieClip,addFrameScript)
{
	MovieClip* th=Class<MovieClip>::cast(asAtomHandler::getObject(obj));
	assert_and_throw(argslen>=2 && argslen%2==0);

	for(uint32_t i=0;i<argslen;i+=2)
	{
		uint32_t frame=asAtomHandler::toInt(args[i]);
		if (asAtomHandler::isNull(args[i+1])) // argument null seems to imply that the script currently attached to the frame is removed
		{
			auto it = th->frameScripts.find(frame);
			if (it != th->frameScripts.end())
			{
				ASATOM_REMOVESTOREDMEMBER(it->second);
				th->frameScripts.erase(it);
			}
			continue;
		}
		else if(!asAtomHandler::isFunction(args[i+1]))
		{
			LOG(LOG_ERROR,"Not a function");
			return;
		}
		IFunction* func = asAtomHandler::as<IFunction>(args[i+1]);
		func->incRef();
		func->addStoredMember();
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
			if (state.FP == getFramesLoaded()-1)
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
	if(asAtomHandler::isString(args[0]))
	{
		tiny_string label = asAtomHandler::toString(args[0],getInstanceWorker());
		dest=getFrameIdByLabel(label, sceneName);
		if(dest==FRAME_NOT_FOUND)
		{
			number_t ret=0;
			if (Integer::fromStringFlashCompatible(label.raw_buf(),ret,10,true))
			{
				// it seems that at least for AVM1 Adobe treats number strings as frame numbers
				dest = getFrameIdByNumber(ret-1, sceneName);
			}
			if(dest==FRAME_NOT_FOUND)
			{
				dest=0;
				LOG(LOG_ERROR, (stop ? "gotoAndStop: label not found:" : "gotoAndPlay: label not found:") <<asAtomHandler::toString(args[0],getInstanceWorker())<<" in scene "<<sceneName<<" at movieclip "<<getTagID()<<" "<<this->state.FP);
			}
		}
	}
	else
	{
		uint32_t inFrameNo = asAtomHandler::toInt(args[0]);
		if(inFrameNo == 0)
			inFrameNo = 1;

		dest = getFrameIdByNumber(inFrameNo-1, sceneName);
	}
	if (dest!=FRAME_NOT_FOUND)
	{
		next_FP=dest;
		while(next_FP >= getFramesLoaded())
		{
			if (hasFinishedLoading())
			{
				if (next_FP >= getFramesLoaded())
				{
					LOG(LOG_ERROR, next_FP << "= next_FP >= state.max_FP = " << getFramesLoaded() << " on "<<this->toDebugString()<<" "<<this->getTagID());
					next_FP = getFramesLoaded()-1;
				}
				break;
			}
			else
				compat_msleep(100);
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
		getSystemState()->runInnerGotoFrame(this);
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
void MovieClip::AVM1gotoFrameLabel(const tiny_string& label,bool stop, bool switchplaystate)
{
	uint32_t dest=getFrameIdByLabel(label, "");
	if(dest==FRAME_NOT_FOUND)
	{
		LOG(LOG_ERROR, "gotoFrameLabel: label not found:" <<label);
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
	assert_and_throw(th->state.FP<th->getFramesLoaded());
	bool newframe = !th->hasFinishedLoading() || th->state.FP != th->getFramesLoaded()-1;
	th->state.next_FP = th->hasFinishedLoading() && th->state.FP == th->getFramesLoaded()-1 ? th->state.FP : th->state.FP+1;
	th->state.explicit_FP=true;
	th->state.stop_FP=true;
	th->runGoto(newframe);
}

ASFUNCTIONBODY_ATOM(MovieClip,prevFrame)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	assert_and_throw(th->state.FP<th->getFramesLoaded());
	bool newframe = th->state.FP != 0;
	th->state.next_FP = th->state.FP == 0 ? th->state.FP : th->state.FP-1;
	th->state.explicit_FP=true;
	th->state.stop_FP=true;
	th->runGoto(newframe);
}

ASFUNCTIONBODY_ATOM(MovieClip,_getFramesLoaded)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	asAtomHandler::setUInt(ret,wrk,th->getFramesLoaded());
}

ASFUNCTIONBODY_ATOM(MovieClip,_getTotalFrames)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	asAtomHandler::setUInt(ret,wrk,th->totalFrames_unreliable);
}

ASFUNCTIONBODY_ATOM(MovieClip,_getScenes)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	Array* res = Class<Array>::getInstanceSNoArgs(wrk);
	res->resize(th->scenes.size());
	uint32_t numFrames;
	for(size_t i=0; i<th->scenes.size(); ++i)
	{
		if(i == th->scenes.size()-1)
			numFrames = th->totalFrames_unreliable - th->scenes[i].startframe;
		else
			numFrames = th->scenes[i].startframe - th->scenes[i+1].startframe;
		asAtom v = asAtomHandler::fromObject(Class<Scene>::getInstanceS(wrk,th->scenes[i],numFrames));
		res->set(i, v,false,false);
	}
	ret = asAtomHandler::fromObject(res);
}

uint32_t MovieClip::getCurrentScene() const
{
	for(size_t i=0;i<scenes.size();++i)
	{
		if(state.FP < scenes[i].startframe)
			return i-1;
	}
	return scenes.size()-1;
}

ASFUNCTIONBODY_ATOM(MovieClip,_getCurrentScene)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	uint32_t numFrames;
	uint32_t curScene = th->getCurrentScene();
	if(curScene == th->scenes.size()-1)
		numFrames = th->totalFrames_unreliable - th->scenes[curScene].startframe;
	else
		numFrames = th->scenes[curScene].startframe - th->scenes[curScene+1].startframe;

	ret = asAtomHandler::fromObject(Class<Scene>::getInstanceS(wrk,th->scenes[curScene],numFrames));
}

ASFUNCTIONBODY_ATOM(MovieClip,_getCurrentFrame)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	//currentFrame is 1-based and relative to current scene
	if (th->state.explicit_FP)
		// if frame is set explicitly, the currentframe property should be set to next_FP, even if it is not displayed yet
		asAtomHandler::setInt(ret,wrk,th->state.next_FP+1 - th->scenes[th->getCurrentScene()].startframe);
	else
		asAtomHandler::setInt(ret,wrk,th->state.FP+1 - th->scenes[th->getCurrentScene()].startframe);
}

ASFUNCTIONBODY_ATOM(MovieClip,_getCurrentFrameLabel)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	for(size_t i=0;i<th->scenes.size();++i)
	{
		for(size_t j=0;j<th->scenes[i].labels.size();++j)
			if(th->scenes[i].labels[j].frame == th->state.FP)
			{
				ret = asAtomHandler::fromObject(abstract_s(wrk,th->scenes[i].labels[j].name));
				return;
			}
	}
	asAtomHandler::setNull(ret);
}

ASFUNCTIONBODY_ATOM(MovieClip,_getCurrentLabel)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	tiny_string label;
	for(size_t i=0;i<th->scenes.size();++i)
	{
		if(th->scenes[i].startframe > th->state.FP)
			break;
		for(size_t j=0;j<th->scenes[i].labels.size();++j)
		{
			if(th->scenes[i].labels[j].frame > th->state.FP)
				break;
			if(!th->scenes[i].labels[j].name.empty())
				label = th->scenes[i].labels[j].name;
		}
	}

	if(label.empty())
		asAtomHandler::setNull(ret);
	else
		ret = asAtomHandler::fromObject(abstract_s(wrk,label));
}

ASFUNCTIONBODY_ATOM(MovieClip,_getCurrentLabels)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	Scene_data& sc = th->scenes[th->getCurrentScene()];

	Array* res = Class<Array>::getInstanceSNoArgs(wrk);
	res->resize(sc.labels.size());
	for(size_t i=0; i<sc.labels.size(); ++i)
	{
		asAtom v = asAtomHandler::fromObject(Class<FrameLabel>::getInstanceS(wrk,sc.labels[i]));
		res->set(i, v,false,false);
	}
	ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(MovieClip,_constructor)
{
	Sprite::_constructor(ret,wrk,obj,nullptr,0);
}

void MovieClip::addScene(uint32_t sceneNo, uint32_t startframe, const tiny_string& name)
{
	if(sceneNo == 0)
	{
		//we always have one scene, but this call may set its name
		scenes[0].name = name;
	}
	else
	{
		assert(scenes.size() == sceneNo);
		scenes.resize(sceneNo+1);
		scenes[sceneNo].name = name;
		scenes[sceneNo].startframe = startframe;
	}
}

void MovieClip::afterLegacyInsert()
{
	if(!getConstructIndicator() && !needsActionScript3())
	{
		asAtom obj = asAtomHandler::fromObjectNoPrimitive(this);
		getClass()->handleConstruction(obj,nullptr,0,true);
	}
	Sprite::afterLegacyInsert();
}

void MovieClip::afterLegacyDelete(bool inskipping)
{
	getSystemState()->stage->AVM1RemoveMouseListener(this);
	getSystemState()->stage->AVM1RemoveKeyboardListener(this);
}
bool MovieClip::AVM1HandleKeyboardEvent(KeyboardEvent *e)
{
	if (this->actions)
	{
		for (auto it = actions->ClipActionRecords.begin(); it != actions->ClipActionRecords.end(); it++)
		{
			if( (e->type == "keyDown" && it->EventFlags.ClipEventKeyDown) ||
				(e->type == "keyUp" && it->EventFlags.ClipEventKeyDown))
			{
				std::map<uint32_t,asAtom> m;
				ACTIONRECORD::executeActions(this,this->getCurrentFrame()->getAVM1Context(),it->actions,it->startactionpos,m);
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
			_NR<DisplayObject> d =hitTest(Vector2f(xg,yg), Vector2f(x,y), DisplayObject::MOUSE_CLICK_HIT,true);
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
					std::map<uint32_t,asAtom> m;
					ACTIONRECORD::executeActions(this,this->getCurrentFrame()->getAVM1Context(),it->actions,it->startactionpos,m);
				}
				if( dispobj &&
					((e->type == "mouseUp" && it->EventFlags.ClipEventRelease)
					|| (e->type == "mouseDown" && it->EventFlags.ClipEventPress)
					|| (e->type == "rollOver" && it->EventFlags.ClipEventRollOver)
					|| (e->type == "rollOut" && it->EventFlags.ClipEventRollOut)
					|| (e->type == "releaseOutside" && it->EventFlags.ClipEventReleaseOutside)
					))
				{
					std::map<uint32_t,asAtom> m;
					ACTIONRECORD::executeActions(this,this->getCurrentFrame()->getAVM1Context(),it->actions,it->startactionpos,m);
				}
			}
		}
		AVM1HandleMouseEventStandard(dispobj,e);
	}
	return false;
}
void MovieClip::AVM1HandleEvent(EventDispatcher *dispatcher, Event* e)
{
	std::map<uint32_t,asAtom> m;
	if (dispatcher == this)
	{
		if (this->actions)
		{
			for (auto it = actions->ClipActionRecords.begin(); it != actions->ClipActionRecords.end(); it++)
			{
				if (e->type == "complete" && it->EventFlags.ClipEventLoad)
				{
					ACTIONRECORD::executeActions(this,this->getCurrentFrame()->getAVM1Context(),it->actions,it->startactionpos,m);
				}
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
		getSystemState()->stage->AVM1AddMouseListener(this);
	}
	if (clipactions.AllEventFlags.ClipEventKeyDown ||
			clipactions.AllEventFlags.ClipEventKeyUp)
		getSystemState()->stage->AVM1AddKeyboardListener(this);

	if (clipactions.AllEventFlags.ClipEventLoad)
		getSystemState()->stage->AVM1AddEventListener(this);
	if (clipactions.AllEventFlags.ClipEventEnterFrame)
	{
		getSystemState()->registerFrameListener(this);
		getSystemState()->stage->AVM1AddEventListener(this);
	}
}

void MovieClip::AVM1SetupMethods(Class_base* c)
{
	DisplayObject::AVM1SetupMethods(c);
	c->setDeclaredMethodByQName("attachMovie","",c->getSystemState()->getBuiltinFunction(AVM1AttachMovie),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("loadMovie","",c->getSystemState()->getBuiltinFunction(AVM1LoadMovie),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("unloadMovie","",c->getSystemState()->getBuiltinFunction(AVM1UnloadMovie),NORMAL_METHOD,true);
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
	c->prototype->setDeclaredMethodByQName("createEmptyMovieClip","",c->getSystemState()->getBuiltinFunction(AVM1CreateEmptyMovieClip),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("removeMovieClip","",c->getSystemState()->getBuiltinFunction(AVM1RemoveMovieClip),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("duplicateMovieClip","",c->getSystemState()->getBuiltinFunction(AVM1DuplicateMovieClip),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("clear","",c->getSystemState()->getBuiltinFunction(AVM1Clear),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("moveTo","",c->getSystemState()->getBuiltinFunction(AVM1MoveTo),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("lineTo","",c->getSystemState()->getBuiltinFunction(AVM1LineTo),NORMAL_METHOD,false);
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
	uint32_t dest=getFrameIdByLabel(label, "");
	if(dest==FRAME_NOT_FOUND)
	{
		LOG(LOG_INFO, "AVM1ExecuteFrameActionsFromLabel: label not found:" <<label);
		return;
	}
	AVM1ExecuteFrameActions(dest);
}

void MovieClip::AVM1ExecuteFrameActions(uint32_t frame)
{
	auto it=frames.begin();
	uint32_t i=0;
	while(it != frames.end() && i < frame)
	{
		++i;
		++it;
	}
	if (it != frames.end())
	{
		it->AVM1executeActions(this);
	}
}

ASFUNCTIONBODY_ATOM(MovieClip,AVM1AttachMovie)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	if (argslen != 3 && argslen != 4)
		throw RunTimeException("AVM1: invalid number of arguments for attachMovie");
	int Depth = asAtomHandler::toInt(args[2]);
	uint32_t nameId = asAtomHandler::toStringId(args[1],wrk);
	RootMovieClip* root = th->is<RootMovieClip>() ? th->as<RootMovieClip>() : th->loadedFrom;
	DictionaryTag* placedTag = root->dictionaryLookupByName(asAtomHandler::toStringId(args[0],wrk));
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
	if (argslen == 4)
	{
		ASObject* o = asAtomHandler::getObject(args[3]);
		if (o)
			o->copyValues(toAdd,wrk);
	}
	if (toAdd->is<MovieClip>())
		toAdd->as<MovieClip>()->inAVM1Attachment=true;
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
		throw RunTimeException("AVM1: invalid number of arguments for CreateEmptyMovieClip");
	int Depth = asAtomHandler::toInt(args[1]);
	uint32_t nameId = asAtomHandler::toStringId(args[0],wrk);
	AVM1MovieClip* toAdd= Class<AVM1MovieClip>::getInstanceSNoArgs(wrk);
	toAdd->loadedFrom = th->loadedFrom;
	toAdd->name = nameId;
	toAdd->setMouseEnabled(false);
	if(th->hasLegacyChildAt(Depth) )
	{
		th->deleteLegacyChildAt(Depth,false);
		th->insertLegacyChildAt(Depth,toAdd,false,false);
	}
	else
		th->insertLegacyChildAt(Depth,toAdd,false,false);
	toAdd->constructionComplete();
	toAdd->afterConstruction();
	toAdd->incRef();
	ret=asAtomHandler::fromObject(toAdd);
}
ASFUNCTIONBODY_ATOM(MovieClip,AVM1RemoveMovieClip)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	if (th->getParent() && !th->is<RootMovieClip>())
	{
		if (th->name != BUILTIN_STRINGS::EMPTY)
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
		throw RunTimeException("AVM1: invalid number of arguments for DuplicateMovieClip");
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
	DefineSpriteTag* tag = (DefineSpriteTag*)loadedFrom->dictionaryLookup(getTagID());
	if (tag)
	{
		toAdd= tag->instance()->as<AVM1MovieClip>();
		toAdd->legacy=true;
	}
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
	toAdd->tokens.filltokens = this->tokens.filltokens;
	toAdd->tokens.stroketokens = this->tokens.stroketokens;
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
	
	AVM1MovieClipLoader* ld = Class<AVM1MovieClipLoader>::getInstanceSNoArgs(wrk);
	th->avm1loader = _MR(ld);
	ld->load(url,method,th);
}
ASFUNCTIONBODY_ATOM(MovieClip,AVM1UnloadMovie)
{
	MovieClip* th=asAtomHandler::as<MovieClip>(obj);
	th->setOnStage(false,false);
	th->tokens.clear();
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
	tf->name = wrk->getSystemState()->getUniqueStringId(instanceName);
	tf->setX(x);
	tf->setY(y);
	tf->width = width;
	tf->height = height;
	th->_addChildAt(tf,depth);
	if(tf->name != BUILTIN_STRINGS::EMPTY)
	{
		tf->incRef();
		multiname objName(nullptr);
		objName.name_type=multiname::NAME_STRING;
		objName.name_s_id=tf->name;
		objName.ns.emplace_back(wrk->getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
		asAtom v = asAtomHandler::fromObjectNoPrimitive(tf);
		th->setVariableByMultiname(objName,v,ASObject::CONST_NOT_ALLOWED,nullptr,wrk);
	}
	if (wrk->getSystemState()->getSwfVersion() >= 8)
	{
		tf->incRef();
		ret = asAtomHandler::fromObjectNoPrimitive(tf);
	}
}

void MovieClip::AVM1HandleConstruction()
{
	if (inAVM1Attachment || constructorCallComplete)
		return;
	setConstructIndicator();
	getSystemState()->stage->AVM1AddDisplayObject(this);
	setConstructorCallComplete();
	AVM1Function* constr = this->loadedFrom->AVM1getClassConstructor(fromDefineSpriteTag);
	if (constr)
	{
		constr->incRef();
		_NR<ASObject> pr = _MNR(constr);
		setprop_prototype(pr);
		asAtom ret = asAtomHandler::invalidAtom;
		asAtom obj = asAtomHandler::fromObjectNoPrimitive(this);
		constr->call(&ret,&obj,nullptr,0);
		AVM1registerPrototypeListeners();
	}
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
		purgeLegacyChildren();
		resetToStart();
	}
	//Declared traits must exists before legacy objects are added
	if (getClass())
		getClass()->setupDeclaredTraits(this);

	bool newFrame = (int)state.FP != state.last_FP;
	if (!needsActionScript3() && implicit && !state.frameadvanced)
		AVM1AddScriptEvents();
	if (newFrame ||!state.frameadvanced)
	{
		if(getFramesLoaded())
		{
			if (this->as<MovieClip>()->state.last_FP > (int)this->as<MovieClip>()->state.next_FP)
			{
				// we are moving backwards in the timeline, so we keep the current list of legacy children available for reusing
				this->rememberLastFrameChildren();
			}
			auto iter=frames.begin();
			uint32_t frame = state.FP;
			removedFrameScripts.clear();
			for(state.FP=0;state.FP<=frame;state.FP++)
			{
				if((int)frame < state.last_FP || (int)state.FP > state.last_FP)
				{
					iter->execute(this,state.FP!=frame,removedFrameScripts);
				}
				++iter;
			}
			state.FP = frame;
			this->clearLastFrameChildren();
		}
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
	if (this->AVM1EventScriptsAdded) // ensure that event scripts are only executed once per frame
		return;
	this->AVM1EventScriptsAdded=true;
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
				script.avm1context = this->getCurrentFrame()->getAVM1Context();
				script.event_name_id = UINT32_MAX;
				script.isEventScript = true;
				this->incRef(); // will be decreffed after script handler was executed 
				script.clip=this;
				getSystemState()->stage->AVM1AddScriptToExecute(script);
			}
		}
	}
	AVM1scriptToExecute script;
	script.actions = nullptr;;
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
	/* Set last_FP to reflect the frame that we have initialized currently.
	 * This must be set before the constructor of this MovieClip is run,
	 * or it will call initFrame(). */
	state.last_FP=state.FP;

	/* call our own constructor, if necassary */
	if (!isConstructed())
	{
		// contrary to http://www.senocular.com/flash/tutorials/orderofoperations/
		// the constructors of the children are _not_ really called bottom-up but in a "mixed" fashion:
		// - the constructor of the parent is called first. that leads to calling the constructors of all super classes of the parent
		// - after the builtin super constructor of the parent was called, the constructors of the children are called
		// - after that, the remaining code of the the parents constructor is executed
		// this ensures that code from the constructor that is placed _before_ the super() call is executed before the children are constructed
		// example:
		// class testsprite : MovieClip
		// {
		//   public var childclip:MovieClip;
		//   public function testsprite()
		//   {
		//      // code here will be executed _before_ childclip is constructed
		//      super();
		//      // code here will be executed _after_ childclip was constructed
		//  }
		DisplayObject::initFrame();
	}
	else if (!placedByActionScript || isConstructed())
	{
		// work on a copy because initframe may alter the displaylist
		std::vector<_R<DisplayObject>> tmplist;
		cloneDisplayList(tmplist);
		// DisplayObjectContainer's ActionScript constructor is responsible
		// for calling `initFrame()` on the first frame.
		for (auto child : tmplist)
		{
			if (initializingFrame && !child->isConstructed())
				continue;
			child->initFrame();
		}
	}
	state.creatingframe=false;
}

void MovieClip::executeFrameScript()
{
	auto itbind = variablebindings.begin();
	while (itbind != variablebindings.end())
	{
		asAtom v = getVariableBindingValue(getSystemState()->getStringFromUniqueId((*itbind).first));
		(*itbind).second->UpdateVariableBinding(v);
		ASATOM_DECREF(v);
		itbind++;
	}
	if (!needsActionScript3())
		return;
	state.explicit_FP=false;
	state.gotoQueued=false;
	uint32_t f = frameScripts.count(state.FP) ? state.FP : UINT32_MAX;
	if (f != UINT32_MAX && !markedForLegacyDeletion && !inExecuteFramescript)
	{
		if (lastFrameScriptExecuted != f)
		{
			lastFrameScriptExecuted = f;
			inExecuteFramescript = true;
			this->getInstanceWorker()->rootClip->executingFrameScriptCount++;
			asAtom v=asAtomHandler::invalidAtom;
			ASObject* closure_this = asAtomHandler::as<IFunction>(frameScripts[f])->closure_this;
			if (!closure_this)
				closure_this=this;
			closure_this->incRef();
			asAtom obj = asAtomHandler::fromObjectNoPrimitive(closure_this);
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
				closure_this->decRef();
				this->getInstanceWorker()->rootClip->executingFrameScriptCount--;
				inExecuteFramescript = false;
				throw;
			}
			ASATOM_DECREF(frameScripts[f]);
			ASATOM_DECREF(v);
			closure_this->decRef();
			this->getInstanceWorker()->rootClip->executingFrameScriptCount--;
			inExecuteFramescript = false;
		}
	}

	if (state.gotoQueued)
		runGoto(true);
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
		return;
	}

	//If we have not yet loaded enough frames delay advancement
	if(state.next_FP>=(uint32_t)getFramesLoaded())
	{
		if(hasFinishedLoading())
		{
			if (getFramesLoaded() != 0)
				LOG(LOG_ERROR,"state.next_FP >= getFramesLoaded:"<< state.next_FP<<" "<<getFramesLoaded() <<" "<<toDebugString()<<" "<<getTagID());
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
	if(!state.stop_FP && getFramesLoaded()>0)
	{
		if (hasFinishedLoading())
			state.next_FP=imin(state.FP+1,getFramesLoaded()-1);
		else
			state.next_FP=state.FP+1;
		if(hasFinishedLoading() && state.FP == getFramesLoaded()-1)
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
}

void MovieClip::constructionComplete(bool _explicit)
{
	Sprite::constructionComplete(_explicit);

	/* If this object was 'new'ed from AS code, the first
	 * frame has not been initalized yet, so init the frame
	 * now */
	if(state.last_FP == -1)
	{
		advanceFrame(true);
		if (getSystemState()->getFramePhase() != FramePhase::ADVANCE_FRAME)
			initFrame();
	}
	// another weird behaviour of framescript execution:
	// it seems that the framescripts are also executed if
	// - we are in an inner goto
	// - the builtin MovieClip constructor was called
	// and before we continue executing the constructor of this clip?!?
	if (getSystemState()->inInnerGoto())
	{
		getSystemState()->stage->executeFrameScript();
	}
	AVM1HandleConstruction();
}
void MovieClip::beforeConstruction(bool _explicit)
{
	DisplayObject::beforeConstruction(_explicit);
}
void MovieClip::afterConstruction(bool _explicit)
{
	DisplayObject::afterConstruction(_explicit);
}

Frame *MovieClip::getCurrentFrame()
{
	if (state.FP >= frames.size())
	{
		LOG(LOG_ERROR,"MovieClip.getCurrentFrame invalid frame:"<<state.FP<<" "<<frames.size()<<" "<<this->toDebugString());
		throw RunTimeException("invalid current frame");
	}
	auto it = frames.begin();
	uint32_t i = 0;
	while (i < state.FP)
	{
		it++;
		i++;
	}
	return &(*it);
}
