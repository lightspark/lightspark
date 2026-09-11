/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2012-2013  Alessandro Pignotti (a.pignotti@sssup.it)
    Copyright (C) 2023-2026  mr b0nk 500 (b0nk@b0nk.xyz)

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

#ifndef DISPLAY_OBJECT_MOVIECLIP_H
#define DISPLAY_OBJECT_MOVIECLIP_H 1

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <utility>
#include <vector>

#include "backends/colortransformbase.h"
#include "backends/geometry.h"
#include "display_object/DisplayObject.h"
#include "display_object/DisplayObjectContainer.h"
#include "display_object/InteractiveObject.h"
#include "gc/ptr.h"
#include "exceptions.h"
#include "smartrefs.h"
#include "swftypes.h"
#include "tiny_string.h"
#include "utils/span.h"

// Based on Ruffle's `avm1::display_object::MovieClip`.

namespace lightspark
{

union asAtom;
class ASObject;
class AVM1Object;
class AVM1Value;
class CachedSurface;
class Class_base;
class DefineSpriteTag;
class Filter;
class FrameContainer;
class InvalidateQueue;
class LoaderInfo;
class Matrix3D;
template<typename T>
class Optional;
class RenderContext;
class SWFMovie;
class Stage;
enum VMCreator;

class MovieClip :
public InteractiveObject,
public DisplayObjectContainer,
public TokenContainer
{
	friend class Stage;
	friend class FrameContainer;
	enum class ClipFlags : uint8_t
	{
		Initialized = 1 << 0,
		ExecutingFrameScript = 1 << 1,
		InAVM1Attachment = 1 << 2,
		ForAVM1InitAction = 1 << 3,
		RunningInitFrame = 1 << 4,
		HasAVM1LoadEvent = 1 << 5,
		PostTimelineCreated = 1 << 6,
	};
private:
	std::map<uint16_t, _R<ASObject>> frameScripts;
	std::vector<DisplayObject&> removedFrameScripts;
	std::vector<AVM1VarBinding> avm1VarBindings;
	SWFMovie& movie;
	DefineSpriteTag* tag;
	LoaderInfo* loaderInfo;

	// `hitTarget` is non-null if another `DisplayObject` has registered
	// this clip as it's `hitArea`. Hits will be relayed to hitTarget.
	DisplayObject* hitTarget;
	DisplayObject* hitArea;
	DisplayObject* dropTarget;
	Graphics* graphics;

	SoundInstance* soundStream;
	size_t soundStartFrame;
	bool streamingSound;

	ClipFlags clipFlags;

	uint16_t totalFrames;
	uint32_t lastFrameScriptExecuted;
	uint32_t lastRatio;

	bool buttonMode;
	bool avm2Enabled;
	bool avm2UseHandCursor;

	void runGoto(bool newFrame);
protected:
	FrameContainer* frameContainer;
	const CLIPACTIONS* actions;

	void resetToStart() override;
	void checkSound(uint32_t frame);// start sound streaming if it is not already playing
	void stopSound();
	void markSoundFinished();
public:
	RunState state;

	uint32_t getFrameCount();
	FrameContainer* getFrameContainer() const { return frameContainer; }
	MovieClip
	(
		SystemState* sys,
		SWFMovie& _movie,
		FrameContainer* f = nullptr,
		DefineSpriteTag* _tag = nullptr,
		LoaderInfo* _loaderInfo = nullptr,
		Optional<const tiny_string&> name = {}
	);

	MovieClip
	(
		SystemState* sys,
		SWFMovie& _movie,
		Optional<const tiny_string&> name = {}
	) : MovieClip(sys, _movie, nullptr, nullptr, nullptr, name) {}

	bool hasClipFlag(const ClipFlags& flag) const
	{
		return clipFlags & flag;
	}

	void setClipFlag(const ClipFlags& flag, bool val)
	{
		if (val)
			clipFlags |= flag;
		else
			clipFlags &= ~flag;
	}

	void setPlaying();
	void setStopped();
	/*
	 * returns true if all frames of this MovieClip are loaded
	 * this is overwritten in RootMovieClip, because that one is
	 * executed while loading
	 */
	virtual bool hasFinishedLoading() { return true; }

	#define FLAG_GETTER_NAME_DECL(name, flag) bool name() const
	#define FLAG_GETTER_PREFIX_DECL(prefix, flag) \
		FLAG_GETTER_NAME_DECL(prefix##flag, flag)
	#define FLAG_GETTER_SUFFIX_DECL(suffix, flag) \
		FLAG_GETTER_NAME_DECL(get##suffix, flag)
	#define FLAG_GETTER_DECL(flag) FLAG_GETTER_PREFIX_DECL(get, flag)

	#define FLAG_SETTER_SUFFIX_DECL(suffix, flag) \
		void set##suffix(bool value)
	#define FLAG_SETTER_DECL(flag) FLAG_SETTER_SUFFIX_DECL(flag, flag)

	#define FLAG_GETTER_SETTER_NAME_DECL(name, flag) \
		FLAG_GETTER_NAME_DECL(name, flag); \
		FLAG_SETTER_DECL(flag)

	#define FLAG_GETTER_SETTER_PREFIX_DECL(prefix, flag) \
		FLAG_GETTER_SETTER_NAME_DECL(prefix##flag, flag)

	#define FLAG_GETTER_SETTER_SUFFIX_DECL(suffix, flag) \
		FLAG_GETTER_SUFFIX_DECL(suffix, flag); \
		FLAG_SETTER_SUFFIX_DECL(suffix, flag)

	#define FLAG_GETTER_SETTER_DECL(flag) \
		FLAG_GETTER_SETTER_PREFIX_DECL(get, flag)

	#define FLAG_GETTER_NAME(name, flag) \
		bool name() const { return hasClipFlag(ClipFlags::flag); }
	#define FLAG_GETTER_PREFIX(prefix, flag) \
		FLAG_GETTER_NAME(prefix##flag, flag)
	#define FLAG_GETTER_SUFFIX(suffix, flag) \
		FLAG_GETTER_NAME(get##suffix, flag)
	#define FLAG_GETTER(flag) FLAG_GETTER_PREFIX(get, flag)

	#define FLAG_SETTER_SUFFIX(suffix, flag) \
		void set##suffix(bool value) { setClipFlag(ClipFlags::flag, value); }
	#define FLAG_SETTER_SUFFIX_NO_ARG(suffix, flag, val) \
		void set##suffix() { setClipFlag(ClipFlags::flag, val); }
	#define FLAG_SETTER(flag) FLAG_SETTER_SUFFIX(flag, flag)
	#define FLAG_SETTER_NO_ARG(flag, val) \
		FLAG_SETTER_SUFFIX_NO_ARG(flag, flag, val)

	#define FLAG_GETTER_SETTER_NAME(name, flag) \
		FLAG_GETTER_NAME(name, flag); \
		FLAG_SETTER(flag)

	#define FLAG_GETTER_SETTER_PREFIX(prefix, flag) \
		FLAG_GETTER_SETTER_NAME(prefix##flag, flag)

	#define FLAG_GETTER_SETTER_SUFFIX(suffix, flag) \
		FLAG_GETTER_SUFFIX(suffix, flag); \
		FLAG_SETTER_SUFFIX(suffix, flag)

	#define FLAG_GETTER_SETTER(flag) \
		FLAG_GETTER_SETTER_PREFIX(get, flag)

	#define FLAG_GETTER_SETTER_NAME_NO_ARG(name, flag, val) \
		FLAG_GETTER_NAME(name, flag); \
		FLAG_SETTER_NO_ARG(flag, val)

	#define FLAG_GETTER_SETTER_PREFIX_NO_ARG(prefix, flag, val) \
		FLAG_GETTER_SETTER_NAME_NO_ARG(prefix##flag, flag, val)

	#define FLAG_GETTER_SETTER_SUFFIX_NO_ARG(suffix, flag, val) \
		FLAG_GETTER_SUFFIX(suffix, flag); \
		FLAG_SETTER_SUFFIX_NO_ARG(suffix, flag, val)

	#define FLAG_GETTER_SETTER_NO_ARG(flag) \
		FLAG_GETTER_SETTER_PREFIX_NO_ARG(get, flag, val)

	FLAG_GETTER_SETTER_PREFIX_NO_ARG(is, Initialized, true);
	FLAG_GETTER_SETTER_PREFIX(is, ExecutingFrameScript);
	FLAG_GETTER_SETTER(InAVM1Attachment);
	FLAG_GETTER_SETTER_NO_ARG(ForAVM1InitAction, true);
	FLAG_GETTER_SETTER_PREFIX(is, RunningInitFrame);
	FLAG_GETTER_SETTER_NAME(hasAVM1LoadEvent, HasAVM1LoadEvent);
	FLAG_GETTER_SETTER_PREFIX(is, PostTimelineCreated);

	#undef FLAG_GETTER_NAME_DECL
	#undef FLAG_GETTER_PREFIX_DECL
	#undef FLAG_GETTER_SUFFIX_DECL
	#undef FLAG_GETTER_DECL
	#undef FLAG_SETTER_SUFFIX_DECL
	#undef FLAG_SETTER_DECL
	#undef FLAG_GETTER_SETTER_NAME_DECL
	#undef FLAG_GETTER_SETTER_PREFIX_DECL
	#undef FLAG_GETTER_SETTER_SUFFIX_DECL
	#undef FLAG_GETTER_SETTER_DECL
	#undef FLAG_GETTER_NAME
	#undef FLAG_GETTER_PREFIX
	#undef FLAG_GETTER_SUFFIX
	#undef FLAG_GETTER
	#undef FLAG_SETTER_SUFFIX
	#undef FLAG_SETTER_SUFFIX_NO_ARG
	#undef FLAG_SETTER
	#undef FLAG_SETTER_NO_ARG
	#undef FLAG_GETTER_SETTER_NAME
	#undef FLAG_GETTER_SETTER_PREFIX
	#undef FLAG_GETTER_SETTER_SUFFIX
	#undef FLAG_GETTER_SETTER
	#undef FLAG_GETTER_SETTER_NAME_NO_ARG
	#undef FLAG_GETTER_SETTER_PREFIX_NO_ARG
	#undef FLAG_GETTER_SETTER_SUFFIX_NO_ARG
	#undef FLAG_GETTER_SETTER_NO_ARG

	void addFrameScript(uint16_t frame, _NR<ASObject> func);

	void prevFrame();
	void nextFrame();
	void prevScene();
	void nextScene();
	void gotoFrame(uint16_t frame, bool stop);

	void setSound(SoundInstance* sound, bool forStreaming);
	SoundInstance* getSoundStream() const { return soundStream; }
	void appendSound(Span<uint8_t> data, uint32_t frame);
	size_t getSoundStartFrame() const { return soundStartFrame; }
	void setSoundStartFrame(size_t frame) { soundStartFrame = frame; }

	uint16_t getCurrentFrame() const { return state.FP; }
	Optional<tiny_string> getCurrentFrameLabel() const;
	Optional<tiny_string> getCurrentLabel() const;
	std::vector<tiny_string> getCurrentLabels() const;
	uint16_t getTotalFrames() const { return totalFrames; }
	uint16_t getFramesLoaded() const;
	Span<const Scene> getScenes() const;
	Scene* getCurrentScene() const;
	bool getIsPlaying() const { return !state.stop_FP; }
	size_t getBytesTotal() const;
	size_t getBytesTotalCompressed() const;
	size_t getBytesLoaded() const;
	size_t getBytesLoadedCompressed() const;
	Graphics* tryGetGraphics() const { return graphics; }
	Graphics& getGraphics();

	bool hasGraphics() const { return graphics != nullptr; }

	Optional<uint16_t> frameLabelToNum(const tiny_string& label) const;
	void forEachFrameAction
	(
		uint16_t frame,
		std::function<void(Span<uint8_t>)> func
	);

	IDrawable* invalidate(bool smoothing) override;
	void markAsChanged() override
	{
		DisplayObjectContainer::markAsChanged();
	}

	void requestInvalidation
	(
		InvalidateQueue* q,
		bool forceTextureRefresh = false
	) override;

	void refreshSurfaceState() override;

	void enterFrame(bool implicit) override;
	void advanceFrame(bool implicit) override;
	void declareFrame(bool implicit) override;
	void initFrame() override;
	void executeFrameScript() override;
	void checkRatio(uint32_t ratio, bool inskipping) override;
	void handleMouseCursor(bool rollover) override;
	bool allowAsMask() const override
	{
		return !isEmpty() || graphics == nullptr;
	}

	number_t getScaleFactor() const override { return scaling; }

	void afterTimelineCreation() override;
	void afterTimelineDeletion(bool inskipping) override;

	uint32_t getTagID() const override;
	LoaderInfo* getLoaderInfo() const override { return loaderInfo; }
	const SWFMovie& getMovie() const override { return movie; }
	bool hitTestShape
	(
		const Vector2Twips& globalPoint,
		const Vector2Twips& localPoint,
		const HitTestFlags& flags
	) override;

	Optional<Rect<Twips>> tryBoundsRect(bool visibleOnly) override;
	Optional<Rect<Twips>> tryBoundsRectWithoutChildren(bool visibleOnly) override;

	void setupActions(const CLIPACTIONS& clipactions);

	void AVM1unloadMovie();
	void AVM1runFrame();
	void AVM1gotoFrameLabel
	(
		const tiny_string& label,
		bool stop,
		bool switchPlayState
	);

	void AVM1gotoFrame
	(
		uint16_t frame,
		bool stop,
		bool switchPlayState,
		bool advanceFrame = true
	);

	void AVM1handleConstruction
	(
		_NGC<AVM1Object> initObj,
		const VMCreator& createdBy,
		bool runFrame
	);

	std::string toDebugString() const override;
};

}
#endif /* DISPLAY_OBJECT_MOVIECLIP_H */
