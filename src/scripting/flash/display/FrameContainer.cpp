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
#include "scripting/flash/display/MovieClip.h"
#include "parsing/tags.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include "scripting/toplevel/Array.h"
#include "scripting/toplevel/ASString.h"
#include "scripting/toplevel/Integer.h"


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
	asAtomHandler::setUInt(ret,wrk,th->frame+1);
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
		if(fl.frame > frame)
		{
			labels.insert(j,FrameLabel_data(frame,label));
			return;
		}
		else if(fl.frame == frame)
		{
			// ignore new name, is that correct?
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
void Frame::AVM1executeActions(DisplayObjectContainer* clip)
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
	uint32_t startframe=0;
	for(size_t i=0; i<scenes.size();++i)
	{
		if(frame < scenes[i].startframe)
		{
			scenes[i-1].addFrameLabel(frame-startframe,label);
			return;
		}
		startframe=scenes[i].startframe;
	}
	scenes.back().addFrameLabel(frame-startframe,label);
}
void FrameContainer::destroyTags()
{
	for(auto it=frames.begin();it!=frames.end();++it)
		it->destroyTags();
}

void FrameContainer::addFrame()
{
	frames.emplace_back(Frame());
}

/* Returns a Scene_data pointer for a scene called sceneName, or for
 * the current scene if sceneName is empty. Returns nullptr, if not found.
 */
const Scene_data *FrameContainer::getScene(uint32_t frame, const tiny_string &sceneName, uint32_t currentframe) const
{
	if (sceneName.empty())
	{
		return scenes.empty() ? nullptr : &scenes[getCurrentScene(currentframe)];
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
uint32_t FrameContainer::getFrameIdByLabel(const tiny_string& label, const tiny_string& sceneName) const
{
	if (sceneName.empty())
	{
		//Find frame in any scene
		for(size_t i=0;i<scenes.size();++i)
		{
			for(size_t j=0;j<scenes[i].labels.size();++j)
				if(scenes[i].labels[j].name == label || scenes[i].labels[j].name.lowercase() == label.lowercase())
					return scenes[i].labels[j].frame+scenes[i].startframe;
		}
	}
	else
	{
		//Find frame in the named scene only
		for(size_t i=0;i<scenes.size();++i)
		{
			if (scenes[i].name == sceneName)
			{
				for(size_t j=0;j<scenes[i].labels.size();++j)
					if(scenes[i].labels[j].name == label || scenes[i].labels[j].name.lowercase() == label.lowercase())
						return scenes[i].labels[j].frame+scenes[i].startframe;
			}
		}
	}

	return FRAME_NOT_FOUND;
}

uint32_t FrameContainer::nextScene(uint32_t frame) const
{
	uint32_t scene = getCurrentScene(frame);
	uint32_t nextframe=0;
	for(size_t i=0;i<=scene+1 && i < scenes.size() ;++i)
		nextframe += scenes[i].startframe;
	if (scene == scenes.size()-1)
		++nextframe;
	return nextframe;
}
uint32_t FrameContainer::prevScene(uint32_t frame) const
{
	uint32_t scene = getCurrentScene(frame);
	uint32_t prevframe=0;
	if (scene != 0)
	{
		for(size_t i=0;i<scene-1;++i)
			prevframe += scenes[i].startframe;
	}
	return prevframe;
}

asAtom FrameContainer::getScenes(ASWorker* wrk, uint32_t framecount) const
{
	Array* res = Class<Array>::getInstanceSNoArgs(wrk);
	res->resize(scenes.size());
	uint32_t numFrames;
	for(size_t i=0; i<scenes.size(); ++i)
	{
		if(i == scenes.size()-1)
			numFrames = framecount - scenes[i].startframe;
		else
			numFrames = scenes[i+1].startframe - scenes[i].startframe;
		asAtom v = asAtomHandler::fromObject(Class<Scene>::getInstanceS(wrk,scenes[i],numFrames));
		res->set(i, v,false,false);
	}
	return asAtomHandler::fromObject(res);
}

/* Return global frame index for frame i (zero-based) in a scene
 * called sceneName. If sceneName is empty, use the current scene.
 */
uint32_t FrameContainer::getFrameIdByNumber(uint32_t i, const tiny_string& sceneName, uint32_t currentframe) const
{
	const Scene_data *sceneData = getScene(i,sceneName,currentframe);
	if (!sceneData)
		return FRAME_NOT_FOUND;

	//Should we check if the scene has at least i frames?
	return sceneData->startframe + i;
}


uint32_t FrameContainer::getCurrentScene(uint32_t frame) const
{
	for(size_t i=0;i<scenes.size();++i)
	{
		if(frame < scenes[i].startframe)
			return i-1;
	}
	return scenes.size()-1;
}
asAtom FrameContainer::createCurrentScene(ASWorker* wrk,uint32_t frame, uint32_t framecount) const
{
	uint32_t numFrames;
	uint32_t curScene = getCurrentScene(frame);
	if(curScene == scenes.size()-1)
		numFrames = framecount - scenes[curScene].startframe;
	else
		numFrames = scenes[curScene+1].startframe - scenes[curScene].startframe;

	return asAtomHandler::fromObject(Class<Scene>::getInstanceS(wrk,scenes[curScene],numFrames));
}

uint32_t FrameContainer::getCurrentSceneStartFrame(uint32_t frame) const
{
	return scenes[getCurrentScene(frame)].startframe;
}

asAtom FrameContainer::getCurrentFrameLabel(ASWorker* wrk,uint32_t frame) const
{
	for(size_t i=0;i<scenes.size();++i)
	{
		for(size_t j=0;j<scenes[i].labels.size();++j)
		{
			if(scenes[i].labels[j].frame+scenes[i].startframe == frame)
			{
				return asAtomHandler::fromObject(abstract_s(wrk,scenes[i].labels[j].name));
			}
		}
	}
	return asAtomHandler::nullAtom;
}

asAtom FrameContainer::getCurrentLabel(ASWorker* wrk,uint32_t frame) const
{
	tiny_string label;
	for(size_t i=0;i<scenes.size();++i)
	{
		if(scenes[i].startframe > frame)
			break;
		for(size_t j=0;j<scenes[i].labels.size();++j)
		{
			if(scenes[i].labels[j].frame+scenes[i].startframe > frame)
				break;
			if(!scenes[i].labels[j].name.empty())
				label = scenes[i].labels[j].name;
		}
	}

	if(label.empty())
		return asAtomHandler::nullAtom;
	else
		return asAtomHandler::fromStringID(wrk->getSystemState()->getUniqueStringId(label));
}

asAtom FrameContainer::getCurrentLabels(ASWorker* wrk,uint32_t frame) const
{
	const Scene_data& sc = scenes[getCurrentScene(frame)];

	Array* res = Class<Array>::getInstanceSNoArgs(wrk);
	res->resize(sc.labels.size());
	for(size_t i=0; i<sc.labels.size(); ++i)
	{
		asAtom v = asAtomHandler::fromObject(Class<FrameLabel>::getInstanceS(wrk,sc.labels[i]));
		res->set(i, v,false,false);
	}
	return asAtomHandler::fromObject(res);
}

void FrameContainer::addScene(uint32_t sceneNo, uint32_t startframe, const tiny_string& name)
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
void FrameContainer::AVM1ExecuteFrameActions(uint32_t frame, MovieClip* clip)
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
		it->AVM1executeActions(clip);
	}
}

void FrameContainer::declareFrame(MovieClip* clip)
{
	bool wasAVM1loaded = clip->isAVM1Loaded;
	if (!clip->state.frameadvanced && !clip->isAVM1Loaded)
		clip->AVM1AddScriptEvents();
	if(getFramesLoaded())
	{
		auto iter=frames.begin();
		uint32_t frame = clip->state.FP;
		clip->removedFrameScripts.clear();
		for(clip->state.FP=0;clip->state.FP<=frame;clip->state.FP++)
		{
			if((int)frame < clip->state.last_FP || (int)clip->state.FP > clip->state.last_FP)
			{
				iter->execute(clip,clip->state.FP!=frame,clip->removedFrameScripts);
			}
			++iter;
		}
		clip->state.FP = frame;
	}
	if (!clip->state.frameadvanced && wasAVM1loaded)
		clip->AVM1AddScriptEvents();
}

void FrameContainer::pop_frame()
{
	assert(!frames.empty());
	frames.pop_back();
}
