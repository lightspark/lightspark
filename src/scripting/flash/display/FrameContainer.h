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

#ifndef SCRIPTING_FLASH_DISPLAY_FRAMECONTAINER_H
#define SCRIPTING_FLASH_DISPLAY_FRAMECONTAINER_H 1

#include "asobject.h"


namespace lightspark
{
class DisplayListTag;
class AVM1InitActionTag;

#define FRAME_NOT_FOUND 0xffffffff //Used by getFrameIdBy*

struct FrameLabel_data
{
	FrameLabel_data() : frame(0) {}
	FrameLabel_data(uint32_t _frame, tiny_string _name) : name(_name),frame(_frame){}
	tiny_string name;
	uint32_t frame;
};

class FrameLabel: public ASObject, public FrameLabel_data
{
public:
	FrameLabel(ASWorker* wrk,Class_base* c);
	FrameLabel(ASWorker* wrk,Class_base* c, const FrameLabel_data& data);
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(_getFrame);
	ASFUNCTION_ATOM(_getName);
};

struct Scene_data
{
	Scene_data() : startframe(0) {}
	//this vector is sorted with respect to frame
	std::vector<FrameLabel_data> labels;
	tiny_string name;
	uint32_t startframe;
	void addFrameLabel(uint32_t frame, const tiny_string& label);
};

class Scene: public ASObject, public Scene_data
{
	uint32_t numFrames;
public:
	Scene(ASWorker* wrk,Class_base* c);
	Scene(ASWorker* wrk,Class_base* c, const Scene_data& data, uint32_t _numFrames);
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(_getLabels);
	ASFUNCTION_ATOM(_getName);
	ASFUNCTION_ATOM(_getNumFrames);
};

class Frame
{
public:
	std::list<DisplayListTag*> blueprint;
	std::list<AVM1InitActionTag*> avm1initactiontags;
	void execute(DisplayObjectContainer* displayList, bool inskipping, std::vector<_R<DisplayObject>>& removedFrameScripts);
	void AVM1executeActions(DisplayObjectContainer* clip);
	/**
	 * destroyTags must be called only by the tag destructor, not by
	 * the objects that are instance of tags
	 */
	void destroyTags();
};

class FrameContainer
{
protected:
	/* This list is accessed by both the vm thread and the parsing thread,
	 * but the parsing thread only accesses frames.back(), while
	 * the vm thread only accesses the frames before that frame (until
	 * the parsing finished; then it can also access the last frame).
	 * To make that easier for the vm thread, the member framesLoaded keep
	 * track of how many frames the vm may access. Access to framesLoaded
	 * is guarded by a spinlock.
	 * For non-RootMovieClips, the parser fills the frames member before
	 * handing the object to the vm, so there is no issue here.
	 * RootMovieClips use the new_frame semaphore to wait
	 * for a finished frame from the parser.
	 * It cannot be implemented as std::vector, because then reallocation
	 * would break concurrent access.
	 */
	std::list<Frame> frames;
	std::vector<Scene_data> scenes;
private:
	//No need for any lock, just make sure accesses are atomic
	ATOMIC_INT32(framesLoaded);
public:
	FrameContainer();
	void setFramesLoaded(uint32_t fl) { framesLoaded = fl; }
	void addToFrame(DisplayListTag *r);
	void addFrameLabel(uint32_t frame, const tiny_string& label);
	uint32_t getFramesLoaded() { return framesLoaded; }
	void addAVM1InitAction(AVM1InitActionTag* t);
	void destroyTags();
	void addFrame();
	uint32_t getCurrentScene(uint32_t frame) const;
	const Scene_data *getScene(uint32_t frame, const tiny_string &sceneName, uint32_t currentframe) const;
	uint32_t getFrameIdByNumber(uint32_t i, const tiny_string& sceneName, uint32_t currentframe) const;
	uint32_t getFrameIdByLabel(const tiny_string& l, const tiny_string& sceneName) const;
	uint32_t nextScene(uint32_t frame) const;
	uint32_t prevScene(uint32_t frame) const;
	asAtom getScenes(ASWorker* wrk, uint32_t framecount) const;
	asAtom createCurrentScene(ASWorker* wrk, uint32_t frame, uint32_t framecount) const;
	uint32_t getCurrentSceneStartFrame(uint32_t frame) const;
	asAtom getCurrentFrameLabel(ASWorker* wrk, uint32_t frame) const;
	asAtom getCurrentLabel(ASWorker* wrk, uint32_t frame) const;
	asAtom getCurrentLabels(ASWorker* wrk, uint32_t frame) const;
	void addScene(uint32_t sceneNo, uint32_t startframe, const tiny_string& name);
	void AVM1ExecuteFrameActions(uint32_t frame, MovieClip* clip);
	void declareFrame(MovieClip* clip);
	uint32_t getFramesSize() const { return frames.size(); }
	void pop_frame();
};

}

#endif /* SCRIPTING_FLASH_DISPLAY_FRAMECONTAINER_H */
