/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2013  Alessandro Pignotti (a.pignotti@sssup.it)
    Copyright (C) 2024, 2026  mr b0nk 500 (b0nk@b0nk.xyz)

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

#ifndef DISPLAY_OBJECT_FRAMECONTAINER_H
#define DISPLAY_OBJECT_FRAMECONTAINER_H 1

#include <cstddef>
#include <cstdint>
#include <functional>
#include <list>
#include <vector>

#include "compat.h"
#include "swftypes.h"
#include "tiny_string.h"

namespace lightspark
{

class AVM1InitActionTag;
class DisplayListTag;
class DisplayObject;
class DisplayObjectContainer;
class MovieClip;
struct TagExecutionList;

#define FRAME_NOT_FOUND size_t(-1) //Used by getFrameIdBy*

struct FrameLabel_data
{
	tiny_string name;
	size_t frame;

	FrameLabel_data() : frame(0) {}
	FrameLabel_data
	(
		size_t _frame,
		const tiny_string& _name
	) : name(_name), frame(_frame) {}
};

struct Scene_data
{
	//this vector is sorted with respect to frame
	std::vector<FrameLabel_data> labels;
	tiny_string name;
	size_t startFrame;

	Scene_data() : startFrame(0) {}
	void addFrameLabel(size_t frame, const tiny_string& label);
};

struct Scene : public Scene_data
{
	size_t numFrames;

	Scene() : numFrames(0) {}
	Scene
	(
		const Scene_data& data,
		size_t _numFrames
	) : Scene_data(data), numFrames(_numFrames) {}
};

class Frame
{
	using DisplayObjectRef = std::reference_wrapper<DisplayObject>;
public:
	std::list<DisplayListTag*> blueprint;
	std::list<AVM1InitActionTag*> avm1InitActionTags;

	void execute
	(
		DisplayObjectContainer& displayList,
		bool inskipping,
		std::vector<DisplayObjectRef>& removedFrameScripts
	);

	void fillExecutionList
	(
		TagExecutionList& list,
		DisplayObjectContainer& displayList,
		std::vector<DisplayObjectRef>& removedFrameScripts
	);

	void AVM1executeInitActions(DisplayObjectContainer& displayList);
	void AVM1executeActionsDirect(MovieClip& clip);
	/**
	 * destroyTags must be called only by the tag destructor, not by
	 * the objects that are instance of tags
	 */
	void destroyTags();
};

class FrameContainer
{
private:
	//No need for any lock, just make sure accesses are atomic
	ACQUIRE_RELEASE_VARIABLE(size_t, framesLoaded);
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
public:
	FrameContainer();
	void setFramesLoaded(size_t fl) { framesLoaded = fl; }
	void addToFrame(DisplayListTag* tag);
	void addFrameLabel(size_t frame, const tiny_string& label);
	size_t getFramesLoaded() const { return framesLoaded; }
	void addAVM1InitAction(AVM1InitActionTag* tag);
	void destroyTags();
	void addFrame();
	size_t getCurrentScene(size_t frame) const;
	const Scene_data* getScene
	(
		const tiny_string& sceneName,
		size_t currentFrame
	) const;

	size_t getFrameIdByNumber
	(
		size_t i,
		const tiny_string& sceneName,
		size_t currentFrame
	) const;

	size_t getFrameIdByLabel
	(
		const tiny_string& name,
		const tiny_string& sceneName,
		size_t currentFrame
	) const;

	size_t nextScene(size_t frame) const;
	size_t prevScene(size_t frame) const;
	std::vector<Scene> getScenes(size_t frameCount) const;
	Scene createCurrentScene(size_t frame, size_t frameCount) const;
	size_t getCurrentSceneStartFrame(size_t frame) const;
	tiny_string getCurrentFrameLabel(size_t frame) const;
	tiny_string getCurrentLabel(size_t frame) const;
	std::vector<tiny_string> getCurrentLabels(size_t frame) const;
	void addScene(size_t scene, size_t startFrame, const tiny_string& name);
	void AVM1ExecuteFrameActionsDirect(size_t frame, MovieClip& clip);
	void declareFrame(MovieClip& clip);
	size_t getFramesSize() const { return frames.size(); }
	void popFrame();
};

}
#endif /* DISPLAY_OBJECT_FRAMECONTAINER_H */
