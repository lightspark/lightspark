/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2010-2013  Alessandro Pignotti (a.pignotti@sssup.it)
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

#include <iostream>

#include "backends/audio.h"
#include "backends/config.h"
#include "backends/decoder.h"
#include "logger.h"
#include "platforms/engineutils.h"
#include "swf.h"


using namespace lightspark;

TimeSpec AudioStream::getPlayedTime()
{
	if (!mixingStarted)
		return playedTime;
	return playedTime + compat_now() - startTime;
}
bool AudioStream::init(number_t volume)
{
	unmuteVolume = curVolume = volume;
	mixerChannel = manager.engineData->audio_StreamInit(this);

	return !(_isPaused &= mixerChannel < 0);
}

void AudioStream::deinit()
{
	if (!isDone)
		manager.engineData->audio_StreamDeinit(mixerChannel);
	mixerChannel = -1;
}

void AudioStream::startMixing()
{
	if (mixingStarted)
		return;

	mixingStarted = true;
	startTime = compat_now();
}

AudioStream::AudioStream
(
	AudioManager& _manager,
	AudioDecoder& _decoder,
	IThreadJob* _producer,
	const TimeSpec& _playedTime
) :
manager(_manager),
decoder(_decoder),
producer(_producer),
hasStarted(false),
_isPaused(true),
mixingStarted(false),
isDone(false),
curVolume(1),
unmuteVolume(1),
playedtime(_playedtime),
mixerChannel(-1)
{
}

void AudioStream::setPause(bool flag)
{
	if (_isPaused = flag)
		playedtime = getPlayedTime();
	else
		mixingStarted = false;

	manager.engineData->audio_StreamPause(mixerChannel, _isPaused);
}

void AudioStream::mute()
{
	unmuteVolume = curVolume;
	setVolume(0);
}

void AudioStream::unmute()
{
	setVolume(unmuteVolume);
}

void AudioStream::setIsDone()
{
	RELEASE_WRITE(isDone, true);
	decoder.skipAll();
}

EventSoundDecoder::EventSoundDecoder
(
	EngineData* engineData,
	size_t bufferTime,
	StreamDecoder& _decoder,
	const SOUNDINFO& info,
	size_t samples,
	size_t _skipSamples
) :
AudioDecoder(bufferTime, engineData),
decoder(_decoder),
loops(info.LoopCount),
startSample(0),
endSample(0),
curSample(0),
skipSamples(_skipSamples)
exhausted(false)
{
	assert_and_throw(decoder.audioDecoder != nullptr);
	sampleRate = decoder.audioDecoder->sampleRate;
	auto sampleDiv = 44100.0 / number_t(sampleRate);
	startSample = (info.InPoint / sampleDiv) + skipSamples;

	endSample =
	(
		info.OutPoint ?
		info.OutPoint / sampleDiv :
		samples
	) + skipSamples;
	nextLoop();
}

void EventSoundDecoder::switchCodec
(
	LS_AUDIO_CODEC codec,
	uint8_t* initData,
	uint32_t size
)
{
	decoder.audioDecoder->switchCodec(codec, initData, size);
}

AudioDecoder::F32SamplePair EventSoundDecoder::getNextSampleF32()
{
	assert(engine->audio_useFloatSampleFormat());
	if (exhausted)
		return { 0, 0 };

	auto audioDecoder = decoder.audioDecoder;
	if (!audioDecoder->hasDecodedFrames())
	{
		nextLoop();
		return getNextSampleF32();
	}

	auto sample = audioDecoder->getNextSampleF32();
	if (endSample && ++curSample > endSample)
		nextLoop();
	return sample;
}

AudioDecoder::S16SamplePair EventSoundDecoder::getNextSampleS16()
{
	if (exhausted)
		return { 0, 0 };

	auto audioDecoder = decoder.audioDecoder;
	if (!audioDecoder->hasDecodedFrames())
	{
		nextLoop();
		return getNextSampleS16();
	}

	auto sample = audioDecoder->getNextSampleS16();
	if (endSample && ++curSample > endSample)
		nextLoop();
	return sample;
}

template<typename T>
size_t EventSoundDecoder::getSamplesImpl(Span<T> span)
{
	if (exhausted)
		return 0;

	auto _span = !endSample ? span : span.getFirst(std::min
	(
		endSample - curSample,
		span.getSize()
	));

	auto ret = decoder.audioDecoder->getSamples(_span);
	assert_and_throw(ret <= span.getSize());

	curSample += ret;
	if (ret == span.getSize())
		return ret;

	nextLoop();
	return ret + getSamplesImpl(span.subSpan(ret));
}

void EventSoundDecoder::nextLoop()
{
	if (exhausted |= !loops)
		return;
	--loops;
	decoder.jumpToPosition(startSample * 1000 / number_t(sampleRate));
	curSample = startSample;
}

AudioManager::AudioManager(EngineData* engine) :
muteAllStreams(false),
audioAvailable(engine->audio_ManagerInit()),
mixerOpened(false),
engineData(engine)
{
}

void AudioManager::forEachStream(std::function<void(AudioStream&)> func)
{
	Locker l(streamMutex);
	forEachStreamNoLock(func);
}

void AudioManager::muteAll()
{
	Locker l(streamMutex);
	muteAllStreams = true;
	forEachStreamNoLock([](auto& stream) { stream.mute(); });
}

void AudioManager::unmuteAll()
{
	Locker l(streamMutex);
	muteAllStreams = false;
	forEachStreamNoLock([](auto& stream) { stream.unmute(); });
}

void AudioManager::removeStream(AudioStream& stream)
{
	{
		Locker l(streamMutex);
		auto it = std::find_if
		(
			streams.begin(),
			streams.end(),
			[&](const auto& it) { return &it == &stream; }
		);

		if (it != streams.end())
		{
			it->deinit();
			streams.erase(it);
		}

		if (!streams.empty())
			return;
	}

	Locker l(managerMutex);
	if (mixerOpened)
		engineData->audio_ManagerCloseMixer(this);
	mixerOpened = false;
}

void AudioManager::stopAllSounds()
{
	// use temporary list of producers to avoid deadlock, as threadAbort() leads to removeStream();
	std::list<IThreadJob*> producers;
	forEachStream([&](auto& stream)
	{
		if (stream.producer != nullptr)
			producers.emplace_back(stream.producer);
	});

	for (auto& producer : producers)
		producer->threadAbort();
}

AudioStream* AudioManager::createStream
(
	AudioDecoder& decoder,
	bool startPaused,
	IThreadJob* producer,
	const TimeSpec& playedTime,
	number_t volume
)
{
	if (!audioAvailable)
		return nullptr;

	[&]
	{
		Locker l(managerMutex);
		if (mixerOpened)
			return;
		if (engineData->audio_ManagerOpenMixer(this))
		{
			mixerOpened = true;
			return;
		}

		LOG(LOG_ERROR, "Couldn't open mixer");
		audioAvailable = false;
	}();

	if (!audioAvailable)
		return nullptr;

	Locker l(streamMutex);
	streams.emplace_back(*this, decoder, producer, playedTime);
	auto& stream = streams.back();
	if (!stream.init(volume))
	{
		streams.pop_back();
		return nullptr;
	}

	if (startPaused)
		stream.pause();
	else
		stream.hasStarted = true;
	return &stream;
}


AudioManager::~AudioManager()
{
	Locker l(managerMutex);
	if (mixerOpened)
		engineData->audio_ManagerCloseMixer(this);
	if (audioAvailable)
		engineData->audio_ManagerDeinit();
}
