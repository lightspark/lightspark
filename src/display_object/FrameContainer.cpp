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

#include "display_object/DisplayObject.h"
#include "display_object/DisplayObjectContainer.h"
#include "display_object/FrameContainer.h"
#include "display_object/MovieClip.h"
#include "display_object/RootMovieClip.h"
#include "parsing/tags.h"

using namespace lightspark;

/*
 * Adds a frame label to the internal vector and keep
 * the vector sorted with respect to frame
 */
void Scene_data::addFrameLabel(size_t frame, const tiny_string& label)
{
	auto it = std::find_if
	(
		labels.begin(),
		labels.end(),
		[&](const auto& frameLabel)
		{
			return frameLabel.frame >= frame;
		}
	);

	if (it == labels.end())
		labels.emplace_back(frame, label);
	else if (it->frame != frame)
		labels.emplace(it, frame, label);
}

void Frame::destroyTags()
{
	while (!blueprint.empty())
	{
		auto tag = blueprint.back();
		blueprint.pop_back();
		delete tag;
	}

	while (!avm1InitActionTags.empty())
	{
		auto tag = avm1InitActionTags.back();
		avm1InitActionTags.pop_back();
		delete tag;
	}
}

void Frame::execute
(
	DisplayObjectContainer& displayList,
	bool inskipping,
	std::vector<DisplayObjectRef>& removedFrameScripts
)
{
	AVM1executeInitActions(displayList);
	for (auto tag : blueprint)
	{
		auto obj = dynamic_cast<RemoveObject2Tag*>(tag);
		if (obj != nullptr && displayList.hasChildAt(obj->getDepth()))
		{
			removedFrameScripts.emplace_back
			(
				displayList.getChildAt(obj->getDepth())
			);
		}
		tag->execute(displayList, inskipping, false);
	}
	displayList.checkClipDepth();
}

void Frame::AVM1executeInitActions(DisplayObjectContainer& displayList)
{
	while (!avm1InitActionTags.empty())
	{
		auto tag = avm1InitActionTags.front();
		// a new instance of the sprite may be constructed during code execution, so we remove it from the initactionlist before executing the code to ensure it's only executed once
		avm1InitActionTags.pop_front();
		tag->execute(displayList.getRoot());
		delete tag;
	}
}

void Frame::fillExecutionList
(
	TagExecutionList& list,
	DisplayObjectContainer& displayList,
	std::vector<DisplayObjectRef>& removedFrameScripts
)
{
	for (auto tag : blueprint)
	{
		auto obj = dynamic_cast<RemoveObject2Tag*>(tag);
		if (obj != nullptr && displayList.hasChildAt(obj->getDepth()))
		{
			removedFrameScripts.emplace_back
			(
				displayList.getChildAt(obj->getDepth())
			);
		}
		tag->fillExecutionList(list);
	}
}
void Frame::AVM1executeActionsDirect(MovieClip& clip)
{
	for (auto tag : blueprint)
	{
		if (tag->getType() != AVM1ACTION_TAG)
			continue;
		static_cast<AVM1ActionTag*>(tag)->executeDirect(clip);
	}
}

FrameContainer::FrameContainer() :
framesLoaded(0),
frames({ Frame() }),
scenes(1)
{
}

/* This runs in parser thread context,
 * but no locking is needed here as it only accesses the last frame.
 * See comment on the 'frames' member. */
void FrameContainer::addToFrame(DisplayListTag* tag)
{
	frames.back().blueprint.emplace_back(tag);
}

void FrameContainer::addAVM1InitAction(AVM1InitActionTag* tag)
{
	frames.back().avm1InitactionTags.emplace_back(tag);
}
/**
 * Find the scene to which the given frame belongs and
 * adds the frame label to that scene.
 * The labels of the scene will stay sorted by frame.
 */
void FrameContainer::addFrameLabel(size_t frame, const tiny_string& label)
{
	assert(!scenes.empty())
	auto it = std::find_if
	(
		scenes.begin(),
		scenes.end(),
		[&](const auto& scene)
		{
			return frame < scene.startFrame;
		}
	);

	// MOTE: This is allowed because we always have at least a single
	// scene, and always use the previous scene.
	--it->addFrameLabel(frame - it->startFrame, label);
}

void FrameContainer::destroyTags()
{
	for (auto& frame : frames)
		frame.destroyTags();
}

void FrameContainer::addFrame()
{
	frames.emplace_back();
}

/* Returns a Scene_data pointer for a scene called sceneName, or for
 * the current scene if sceneName is empty. Returns nullptr, if not found.
 */
const Scene_data* FrameContainer::getScene
(
	const tiny_string& sceneName,
	size_t currentFrame
) const
{
	if (sceneName.empty())
	{
		return
		(
			scenes.empty() ?
			nullptr :
			&scenes[getCurrentScene(currentFrame)]
		);
	}

	//Find scene by name
	auto it = std::find_if
	(
		scenes.begin(),
		scenes.end(),
		[&](const auto& scene)
		{
			return scene.name == sceneName;
		}
	);

	return it != scenes.end() ? &*it : nullptr;
}

/* Return global frame index for a named frame. If sceneName is not
 * empty, return a frame only if it belong to the named scene.
 */
size_t FrameContainer::getFrameIdByLabel
(
	const tiny_string& name,
	const tiny_string& sceneName,
	size_t currentFrame
) const
{
	number_t ret = 0;
	if (Integer::fromStringFlashCompatible(name.raw_buf(), ret, 10, true))
		return getFrameIdByNumber(ret - 1, sceneName, currentFrame);

	auto findFrameId = [&](const Scene_data& scene) -> size_t
	{
		for (const auto& label : scene.labels)
		{
			if (label.name.caselessEquals(name))
				return label.frame + scene.startFrame;
		}

		return FRAME_NOT_FOUND;
	};

	if (sceneName.empty())
	{
		//Find frame in any scene
		for (const auto& scene : scenes)
		{
			auto frameId = findFrameId(scene);
			if (frameId != FRAME_NOT_FOUND)
				return frameId;
		}
		return FRAME_NOT_FOUND;
	}

	//Find frame in the named scene only
	auto scene = getScene(sceneName, currentFrame);
	return scene != nullptr ? findFrameId(*scene) : -1;
}

size_t FrameContainer::nextScene(size_t frame) const
{
	auto end = std::next(scenes.begin(), std::min
	(
		getCurrentScene(frame) + 2,
		scenes.size() - 1
	));
	return std::accumulate
	(
		scenes.begin(),
		end,
		size_t(0),
		[](const auto& a, const auto& scene)
		{
			return a + scene.startFrame;
		}
	) + end == scenes.end();
}

size_t FrameContainer::prevScene(size_t frame) const
{
	auto sceneId = getCurrentScene(frame);
	return !sceneId ? 0 : std::accumulate
	(
		scenes.begin(),
		std::next(scenes.begin(), sceneId - 1),
		size_t(0),
		[](const auto& a, const auto& scene)
		{
			return a + scene.startFrame;
		}
	);
}

std::vector<Scene> FrameContainer::getScenes(size_t frameCount) const
{
	auto end = std::prev(scenes.end());
	std::vector<Scene> ret;
	ret.reserve(scenes.size());
	for (auto it = scenes.begin(); it != end; ++it)
	{
		ret.emplace_back
		(
			*it,
			std::next(it)->startFrame - it->startFrame
		);
	}

	ret.emplace_back(*it, frameCount - it->startFrame);
	return ret;
}

/* Return global frame index for frame i (zero-based) in a scene
 * called sceneName. If sceneName is empty, use the current scene.
 */
size_t FrameContainer::getFrameIdByNumber
(
	size_t i,
	const tiny_string& sceneName,
	size_t currentFrame
) const
{
	auto scene = getScene(i, sceneName, currentFrame);
	//Should we check if the scene has at least i frames?
	return scene != nullptr ? scene->startFrame + i : FRAME_NOT_FOUND;
}

size_t FrameContainer::getCurrentScene(size_t frame) const
{
	return std::distance(scenes.begin(), std::find_if
	(
		scenes.begin(),
		scenes.end(),
		[&](const auto& scene) { return frame < scene.startFrame; }
	));
}

Scene FrameContainer::createCurrentScene(size_t frame, size_t frameCount) const
{
	size_t curScene = getCurrentScene(frame);
	auto endFrame =
	(
		curScene < scenes.size() - 1 ?
		scenes[curScene + 1].startFrame :
		frameCount
	);

	return Scene
	(
		scenes[curScene],
		endFrame - scenes[curScene].startFrame
	);
}

size_t FrameContainer::getCurrentSceneStartFrame(size_t frame) const
{
	return scenes[getCurrentScene(frame)].startFrame;
}

tiny_string FrameContainer::getCurrentFrameLabel(size_t frame) const
{
	for (const auto& scene : scenes)
	{
		for (const auto& label : scene.labels)
		{
			if (label.frame + scene.startFrame != frame)
				return label.name;
		}
	}
	return "";
}

tiny_string FrameContainer::getCurrentLabel(size_t frame) const
{
	tiny_string ret;
	for (const auto& scene : scenes)
	{
		if (scene.startFrame > frame)
			break;
		for (const auto& label : scene.labels)
		{
			if (label.frame + scene.startFrame > frame)
				break;
			if (!label.name.empty())
				ret = label.name;
		}
	}
	return ret;
}

std::vector<tiny_string> FrameContainer::getCurrentLabels(size_t frame) const
{
	std::vector<tiny_string> ret;
	const auto& scene = scenes[getCurrentScene(frame)];
	ret.reserve(scene.labels.size());

	for (const auto& label : scene.labels)
		ret.emplace_back(label.name);
	return ret;
}

void FrameContainer::addScene(size_t scene, size_t startFrame, const tiny_string& name)
{
	if (!scene)
	{
		//we always have one scene, but this call may set its name
		scenes[0].name = name;
		return;
	}

	assert(scenes.size() == scene);
	scenes.resize(scene + 1);
	scenes[scene].name = name;
	scenes[scene].startframe = startFrame;
}
void FrameContainer::AVM1ExecuteFrameActionsDirect(size_t frame, MovieClip& clip)
{
	if (frame >= frames.size())
		return;
	std::next(frames.begin(), frame)->AVM1executeActionsDirect(clip);
}

void FrameContainer::declareFrame(MovieClip& clip)
{
	if (!getFramesLoaded())
		return;

	size_t frame = clip->state.FP;
	clip.removedFrameScripts.clear();

	if (clip.state.last_FP == frame - 1)
	{
		// common case moving forward one frame => no need to create execution list, just execute all tags of the current frame
		std::next(frames.begin(), frame - 1)->execute
		(
			clip,
			false,
			clip.removedFrameScripts
		);
		return;
	}

	auto it = frames.begin();
	TagExecutionList execList;
	execList.originalDepthMap = &clip.mapDepthToChild;
	// fill all tags to be executed up to current frame
	for (clip.state.FP = 0; clip.state.FP <= frame; ++clip.state.FP, ++it)
	{
		if
		(
			frame >= clip.state.last_FP &&
			clip.state.FP <= clip.state.last_FP
		)
			continue;
		execList.inSkipping = clip.state.FP != frame;
		it->AVM1executeInitActions(clip);
		it->fillExecutionList
		(
			executionList,
			clip,
			clip.removedFrameScripts
		);
	}

	clip.state.FP = frame;
	// execute all tags
	for (const auto& pair : execList.executionList)
	{
		// entry may be nullptr in case a DisplayObject was placed _and_ removed before we reach the current frame
		if (pair.first == nullptr)
			continue;
		pair.first->execute
		(
			clip,
			pair.second,
			frame < clip.state.last_FP
		);
		clip.checkClipDepth();
	}
}

void FrameContainer::popFrame()
{
	assert(!frames.empty());
	frames.pop_back();
}
