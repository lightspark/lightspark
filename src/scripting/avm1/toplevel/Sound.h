/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2026  mr b0nk 500 (b0nk@b0nk.xyz)

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

#ifndef SCRIPTING_AVM1_TOPLEVEL_SOUND_H
#define SCRIPTING_AVM1_TOPLEVEL_SOUND_H 1

#include "backends/colortransformbase.h"
#include "gc/ptr.h"
#include "scripting/avm1/function.h"
#include "scripting/avm1/object.h"

// Based on Ruffle's `avm1::globals::sound` crate.

namespace lightspark
{

class AVM1Activation;
class AVM1Context;
class AVM1DeclContext;
class AVM1Sound;
class AVM1SystemClass;
class AVM1Value;
class MovieClip;
class Sound;
class SystemState;

struct QueuedPlay
{
	GcPtr<AVM1Sound> soundObj;
	number_t startOffset;
	size_t loops;
};

struct EmptySound {};

class SoundState
{
public:
	enum Type
	{
		// An empty sound object, playback isn't allowed.
		Empty,
		// The sound is loading, all plays are queued.
		Loading,
		// The sound is loaded, all plays are started immediately.
		Loaded,
	};
private:
	Type type
	union
	{
		std::vector<QueuedPlay> queuedPlays;
		// The sound that's attached to this object.
		Sound* sound;
	};
public:
	SoundState() : type(Type::Empty) {}
	SoundState
	(
		const std::vector<QueuedPlay>& _queuedPlays
	) : type(Type::Loading) queuedPlays(_queuedPlays) {}
	SoundState(Sound* _sound) : type(Type::Loaded), sound(_sound) {}

	~SoundState();
	SoundState& operator=(const SoundState& other);

	template<typename V>
	constexpr auto visit(V&& visitor)
	{
		switch (type)
		{
			case Type::Empty: return visitor(EmptySound {}); break;
			case Type::Loading: return visitor(queuedPlays); break;
			case Type::Loaded: return visitor(sound); break;
			default: assert(false); break;
		}
	}
};

class AVM1Sound : public AVM1Object
{
private:
	SoundState state;

	// The instance of the last played sound.
	SoundInstance* soundInstance;

	// NOTE: `Sound`s in AVM1 are tied to a specific `MovieClip`.
	NullableGcPtr<MovieClip> clip;

	// The position of the last playing sound, in milliseconds.
	size_t soundPos;

	// The duration of the currently attached sound, in milliseconds.
	Optional<size_t> duration;

	// Whether this sound's an external streaming MP3.
	// This will be `true` if `Sound.loadSound()` was called with an
	// `isStreaming` of `true`.
	// NOTE: A streaming sound can only have a single active instance.
	bool isStreaming;
public:
	AVM1Sound
	(
		AVM1Activation& act,
		NullableGcPtr<MovieClip> _clip = NullGc
	);

	static GcPtr<AVM1SystemClass> createClass
	(
		AVM1DeclContext& ctx,
		const GcPtr<AVM1Object>& superProto
	);

	const Optional<size_t>& getDuration() const { return duration; }
	void getDuration(const Optional<size_t>& _duration) { duration = _duration; }
	Sound* getSound() const;
	SoundInstance* getSoundInstance() const { return soundInstance; }
	void setSoundInstance(SoundInstance* instance) { soundInstance = instance; }
	NullableGcPtr<MovieClip> getClip() const { return clip; }
	size_t getPos() const { return soundPos; }
	void setPos(size_t pos) { soundPos = pos; }
	bool getIsStreaming() const { return isStreaming; }
	void setIsStreaming(bool _isStreaming) { isStreaming = _isStreaming; }

	void play(SystemState* sys, const QueuedPlay& play);
	void setIsLoading();
	void loadSound(AVM1Activation& act, Sound* sound);
	void loadID3(AVM1Activation& act, const Span<uint8_t>& bytes);

	AVM1_FUNCTION_DECL(ctor);
	// NOTE: These aren't properties.
	// `AVM1_PROPERTY_TYPE_DECL` is used here to declare both `[gs]et`
	// functions in a single line.
	AVM1_PROPERTY_TYPE_DECL(AVM1Sound, Pan);
	AVM1_PROPERTY_TYPE_DECL(AVM1Sound, Transform);
	AVM1_PROPERTY_TYPE_DECL(AVM1Sound, Volume);
	AVM1_FUNCTION_TYPE_DECL(AVM1Sound, stop);
	AVM1_FUNCTION_TYPE_DECL(AVM1Sound, attachSound);
	AVM1_FUNCTION_TYPE_DECL(AVM1Sound, start);
	AVM1_PROPERTY_TYPE_DECL(AVM1Sound, Duration);
	AVM1_PROPERTY_TYPE_DECL(AVM1Sound, Position);
	AVM1_FUNCTION_TYPE_DECL(AVM1Sound, loadSound);
	AVM1_FUNCTION_TYPE_DECL(AVM1Sound, getBytesLoaded);
	AVM1_FUNCTION_TYPE_DECL(AVM1Sound, getBytesTotal);
};

}
#endif /* SCRIPTING_AVM1_TOPLEVEL_SOUND_H */
