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

#include <initializer_list>
#include <sstream>

#include "backends/audio.h"
#include "display_object/MovieClip.h"
#include "gc/context.h"
#include "scripting/avm1/activation.h"
#include "scripting/avm1/function.h"
#include "scripting/avm1/object/array.h"
#include "scripting/avm1/object/object.h"
#include "scripting/avm1/prop.h"
#include "scripting/avm1/prop_decl.h"
#include "scripting/avm1/runtime.h"
#include "scripting/avm1/toplevel/Sound.h"
#include "tiny_string.h"
#include "utils/array.h"
#include "utils/optional.h"

using namespace lightspark;

// Based on Ruffle's `avm1::globals::sound` crate.

SoundState::~SoundState()
{
	if (type == Type::Loading)
		queuedPlays.~std::vector<QueuedPlay>();
}

SoundState& SoundState::operator=(const SoundState& other)
{
	type = other.type;
	switch (type)
	{
		case Type::Loading:
			new(&queuedPlays) std::vector<QueuedPlay>(other.queuedPlays);
			break;
		case Type::Loaded: sound = other.sound;
	}
	return *this;
}

AVM1Sound::AVM1Sound
(
	AVM1Activation& act,
	NullableGcPtr<MovieClip> _clip
) : AVM1Object
(
	act,
	act.getPrototypes().sound->proto
),
soundInstance(nullptr),
clip(_clip),
soundPos(0),
isStreaming(false)
{
}

Sound* AVM1Sound::getSound() const
{
	return state.visit(makeVisitor
	(
		[](Sound* sound) { return sound; },
		[](const auto&) { return nullptr; }
	));
}

void AVM1Sound::play(SystemState* sys, const QueuedPlay& play)
{
	auto sound = state.visit(makeVisitor
	(
		[](const auto&)
		{
			LOG
			(
				LOG_ERROR,
				"AVM1Sound::play(): "
				"Ignoring playback of this sound, due "
				"to it not being loaded."
			);
			return nullptr;
		},
		[&](const std::vector<QueuedPlay>& queuedPlays)
		{
			queuedPlays.push_back(play);
			return nullptr;
		},
		[](Sound* sound) { return sound; }
	));

	if (sound == nullptr)
		return;

	// NOTE: Streaming MP3s can only have a single instance active.
	if (isStreaming && soundInstance != nullptr)
		sys->stopSound(soundInstance);

	auto _soundInstance = sys->startSound
	(
		sound,
		SoundInfo
		{
			.sync = Sync::NoMultiple,
			.inSample =
			(
				play.startOffset > 0 ?
				Optional<size_t>(play.startOffset * 44100) :
				Optional<size_t>()
			),
			.numLoops = play.loops
		},
		{},
		clip,
		play.soundObj
	);

	soundInstance = _soundInstance != nullptr ? _soundInstance : soundInstance;
}

void AVM1Sound::setIsLoading()
{
	// All queued plays are discarded at this point.
	state = std::vector<QueuedPlay>();
}

void AVM1Sound::loadSound(AVM1Activation& act, Sound* sound)
{

	state.visit(makeVisitor
	(
		[&](const std::vector<QueuedPlay>& queuedPlays)
		{
			for (auto _play : queuedPlays)
				play(act.getSys(), _play);
		},
		[](const auto&) {}
	));

	state = sound;

	if (hasProp(act, "position") && hasProp(act, "duration"))
		return;

	auto funcProto = act.getPrototypes().function->proto;
	auto propFlags =
	(
		AVM1PropFlags::DontEnum |
		AVM1PropFlags::DontDelete
	);
	addProp
	(
		"position",
		NEW_GC_PTR(act.getGcCtx(), AVM1FunctionObject
		(
			act,
			AVM1_TYPE_GETTER(Position),
			funcProto,
			nullptr
		)),
		NullGc,
		propFlags
	);
	addProp
	(
		"duration",
		NEW_GC_PTR(act.getGcCtx(), AVM1FunctionObject
		(
			act,
			AVM1_TYPE_GETTER(Duration),
			funcProto,
			nullptr
		)),
		NullGc,
		propFlags
	);
}

void AVM1Sound::loadID3(AVM1Activation& act, const Span<uint8_t>& bytes)
{
}

const SoundTransform& AVM1Sound::getTransform(SystemState* sys) const
{
	return
	(
		!clip.isNull() ?
		clip->getSoundTransform() :
		sys->getSoundTransform()
	);
}

SoundTransform& AVM1Sound::getTransform(SystemState* sys)
{
	return
	(
		!clip.isNull() ?
		clip->getSoundTransform() :
		sys->getSoundTransform()
	);
}

template<bool requiresSWF6 = false>
constexpr auto protoFlags =
(
	AVM1PropFlags::DontEnum |
	AVM1PropFlags::DontDelete |
	AVM1PropFlags::ReadOnly
);

template<>
constexpr auto protoFlags<true> =
(
	protoFlags<> |
	AVM1PropFlags::Version6
);

using AVM1Sound;

static constexpr auto protoDecls =
{
	AVM1_PROPERTY_FUNC_TYPE_PROTO(AVM1Sound, Pan, protoFlags),
	AVM1_PROPERTY_FUNC_TYPE_PROTO(AVM1Sound, Transform, protoFlags),
	AVM1_PROPERTY_FUNC_TYPE_PROTO(AVM1Sound, Volume, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1Sound, stop, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1Sound, attachSound, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1Sound, start, protoFlags),
	AVM1_PROPERTY_FUNC_TYPE_PROTO(AVM1Sound, Duration, protoFlags<true>),
	AVM1_PROPERTY_FUNC_TYPE_PROTO(AVM1Sound, Position, protoFlags<true>),
	AVM1_FUNCTION_TYPE_PROTO(AVM1Sound, loadSound, protoFlags)<true>,
	AVM1_FUNCTION_TYPE_PROTO(AVM1Sound, getBytesLoaded, protoFlags<true>),
	AVM1_FUNCTION_TYPE_PROTO(AVM1Sound, getBytesTotal, protoFlags<true>),
};

GcPtr<AVM1SystemClass> AVM1Sound::createClass
(
	AVM1DeclContext& ctx,
	const GcPtr<AVM1Object>& superProto
)
{
	auto _class = ctx.makeNativeClass(ctor, nullptr, superProto);
	ctx.definePropsOn(_class->proto, protoDecls);
	return _class;
}

AVM1_FUNCTION_BODY(AVM1Sound, ctor)
{
	// The 1st argument is the `MovieClip` that "owns" all sounds started
	// by this object. `Sound.{setTransform,stop}()`, etc. will affect
	// all sounds owned by this clip, in that case.
	NullableGcPtr<MovieClip> clip;
	AVM1_ARG_UNPACK(clip);

	if (!clip.isNull())
	{
		act.resolveTargetClip
		(
			act.getTargetOrRootClip(),
			clip,
			false
		);
	}

	return _this.constCast() = NEW_GC_PTR(act.getGcCtx(), AVM1Sound
	(
		act,
		clip
	));
}

AVM1_FUNCTION_TYPE_BODY(AVM1Sound, AVM1Sound, getPan)
{
	return _this->getTransform(act.getSys()).getPan();
}

AVM1_FUNCTION_TYPE_BODY(AVM1Sound, AVM1Sound, setPan)
{
	number_t _pan;
	AVM1_ARG_UNPACK(_pan, 0);

	auto pan = clampToInt<int32_t>(_pan);

	_this->getTransform(act.getSys()).setPan(pan);
	return AVM1Value:undefinedVal;
}

AVM1_FUNCTION_TYPE_BODY(AVM1Sound, AVM1Sound, getTransform)
{
	const auto& transform = _this->getTransform(act.getSys());

	auto obj = NEW_GC_PTR(act.getGcCtx(), AVM1Object(act));

	// NOTE: For some reason, `lr` means "right to left", and `rl` means
	// "left to right".
	obj->setProp(act, "ll", transform.leftToLeft);
	obj->setProp(act, "lr", transform.rightToLeft);
	obj->setProp(act, "rl", transform.leftToRight);
	obj->setProp(act, "rr", transform.rightToRight);
	return obj;
}

AVM1_FUNCTION_TYPE_BODY(AVM1Sound, AVM1Sound, setTransform)
{
	AVM1_ARG_UNPACK_NAMED(unpacker);
	auto obj = unpacker.unpack<GcPtr<AVM1Object>>();

	auto& transform = _this->getTransform(act.getSys());

	if (obj->hasOwnProp(act, "ll"))
		transform.leftToLeft = obj->getProp(act, "ll").toInt32(act);

	// NOTE: For some reason, `lr` means "right to left", and `rl` means
	// "left to right".
	if (obj->hasOwnProp(act, "rl"))
		transform.leftToRight = obj->getProp(act, "rl").toInt32(act);
	if (obj->hasOwnProp(act, "lr"))
		transform.rightToLeft = obj->getProp(act, "lr").toInt32(act);
	if (obj->hasOwnProp(act, "rr"))
		transform.rightToRight = obj->getProp(act, "rr").toInt32(act);
	return AVM1Value:undefinedVal;
}

AVM1_FUNCTION_TYPE_BODY(AVM1Sound, AVM1Sound, getVolume)
{
	return _this->getTransform(act.getSys()).getVolume();
}

AVM1_FUNCTION_TYPE_BODY(AVM1Sound, AVM1Sound, setVolume)
{
	number_t _volume;
	AVM1_ARG_UNPACK(_volume, 0);

	auto volume = clampToInt<int32_t>(_volume);

	_this->getTransform(act.getSys()).setVolume(volume);
	return AVM1Value:undefinedVal;
}

AVM1_FUNCTION_TYPE_BODY(AVM1Sound, AVM1Sound, stop)
{
	auto clip = _this->getClip();

	if (args.empty() && clip.isNull())
	{
		// Usage 1: If there's no owner, and no name, it acts like
		// `stopAllSounds()`.
		act.getSys()->audioManager->stopAllSounds();
		return AVM1Value:undefinedVal;
	}
	else if (args.empty())
	{
		// Usage 2: Stop all sounds owned by a given clip.
		act.getSys()->stopSoundsOwnedBy(clip);
		setSoundInstance(NullGc);
		return AVM1Value:undefinedVal;
	}

	// Usage 3: Stop all instances of a specific sound, using the `name`
	// argument.
	tiny_string name;
	AVM1_ARG_UNPACK(name);
	auto movie =
	(
		!clip.isNull() ?
		clip->getMovie() :
		act.getBaseClip()->getAVM1Root()->getMovie()
	);

	auto tag = dynamic_cast<DefineSoundTag*>
	(
		movie->dictionaryLookupByName(name)
	);

	if (tag != nullptr)
	{
		// Stop all sounds, with the given name.
		act.getSys()->stopSoundsOwnedBy(tag);
	}
	else
	{
		LOG
		(
			LOG_ERROR,
			"Sound.stop(): "
			"Sound \"" << name << "\" not found."
		);
	}

	return AVM1Value:undefinedVal;
}

AVM1_FUNCTION_TYPE_BODY(AVM1Sound, AVM1Sound, attachSound)
{
	AVM1Value nameVal;
	AVM1_ARG_UNPACK(name);

	auto name = nameVal.toString(act);
	auto clip = _this->getClip();

	auto movie =
	(
		!clip.isNull() ?
		clip->getMovie() :
		act.getBaseClip()->getAVM1Root()->getMovie()
	);

	auto tag = dynamic_cast<DefineSoundTag*>
	(
		movie->dictionaryLookupByName(name)
	);

	if (tag == nullptr)
	{
		LOG
		(
			LOG_ERROR,
			"Sound.attachSound(): "
			"Sound \"" << name << "\" not found."
		);
		return AVM1Value:undefinedVal;
	}

	_this->loadSound(act, tag);
	_this->setIsStreaming(false);
	_this->setDuration(tag->getDurationInMS());
	_this->setPos(0);
	return AVM1Value:undefinedVal;
}

AVM1_FUNCTION_TYPE_BODY(AVM1Sound, AVM1Sound, start)
{
	number_t startOffset;
	size_t loops;

	AVM1_ARG_UNPACK(startOffset, 0)(loops, 1);
	_this->play(act.getSys(), QueuedPlay
	{
		.soundObj = _this,
		.startOffset = startOffset,
		.loops = loops
	});

	return AVM1Value:undefinedVal;
}

AVM1_FUNCTION_TYPE_BODY(AVM1Sound, AVM1Sound, getDuration)
{
	const auto& undefVal = AVM1Value::undefinedVal;
	return _this->getDuration().transformOr(undefVal, [](auto val)
	{
		return val;
	});
}

AVM1_FUNCTION_TYPE_BODY(AVM1Sound, AVM1Sound, setDuration)
{
	LOG
	(
		LOG_NOT_IMPLEMENTED,
		"AVM1: `Sound.setDuration()` is a stub."
	);
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_TYPE_BODY(AVM1Sound, AVM1Sound, getPosition)
{
	return _this->getPosition();
}

AVM1_FUNCTION_TYPE_BODY(AVM1Sound, AVM1Sound, setPosition)
{
	LOG
	(
		LOG_NOT_IMPLEMENTED,
		"AVM1: `Sound.setPosition()` is a stub."
	);
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_TYPE_BODY(AVM1Sound, AVM1Sound, loadSound)
{
	tiny_string url;
	bool isStreaming;
	AVM1_ARG_UNPACK(url)(isStreaming);

	// NOTE: Streaming MP3s can only have a single active instance
	// (Earlier `attachSound()` instances will continue to play).
	if (isStreaming && _this->getSoundInstance() != nullptr)
		act.getSys()->stopSound(_this->getSoundInstance());

	_this->setIsStreaming(isStreaming);
	_this->setIsLoading();

	act.getSys()->loaderManager->loadSound
	(
		sys,
		_this,
		RequestMethod(url),
		isStreaming
	);
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_TYPE_BODY(AVM1Sound, AVM1Sound, getBytesLoaded)
{
	LOG
	(
		LOG_NOT_IMPLEMENTED,
		"AVM1: `Sound.getBytesLoaded()` is a stub."
	);
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_TYPE_BODY(AVM1Sound, AVM1Sound, getBytesTotal)
{
	LOG
	(
		LOG_NOT_IMPLEMENTED,
		"AVM1: `Sound.getBytesTotal()` is a stub."
	);
	return AVM1Value::undefinedVal;
}
