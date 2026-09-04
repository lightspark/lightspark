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

#ifndef BACKENDS_AUDIO_H
#define BACKENDS_AUDIO_H 1

#include <vector>

#include "backends/decoder.h"
#include "compat.h"
#include "threading.h"
#include "utils/span.h"
#include "utils/timespec.h"

namespace lightspark
{

class AudioDecoder;
class AudioStream;
class EngineData;
class SOUNDINFO;
class StreamDecoder;

// TODO: Move this into `decoder.{cpp,h}` at some point.
// Loosely based off Ruffle's `backend::audio::EventSoundStream`.
class EventSoundDecoder : public AudioDecoder
{
private:
	StreamDecoder& decoder;
	size_t loops;
	size_t startSample;
	size_t endSample;
	size_t curSample;
	size_t skipSamples;
	bool exhausted;

	template<typename T>
	size_t getSamplesImpl(Span<T> span);
public:
	EventSoundDecoder
	(
		EngineData* engineData,
		size_t bufferTime,
		StreamDecoder& _decoder,
		const SOUNDINFO& info,
		size_t samples,
		size_t _skipSamples
	);

	void switchCodec
	(
		LS_AUDIO_CODEC codec,
		uint8_t* initData,
		uint32_t size
	) override;

	uint32_t decodeData
	(
		uint8_t* data,
		uint32_t size,
		uint32_t time
	) override
	{
		return 0;
	}

	F32SamplePair getNextSampleF32() override;
	S16SamplePair getNextSampleS16() override;
	size_t getSamples(Span<F32SamplePair> span) override
	{
		return getSamplesImpl(span);
	}

	size_t getSamples(Span<S16SamplePair> span) override
	{
		return getSamplesImpl(span);
	}

	void nextLoop();
	const StreamDecoder& getDecoder() const { return decoder; }
	StreamDecoder& getDecoder() { return decoder; }
	size_t getLoops() const { return loops; }
	size_t getStartSample() const { return startSample; }
	size_t getEndSample() const { return endSample; }
	size_t getCurSample() const { return curSample; }
	size_t getSkipSamples() const { return skipSamples; }
	bool isExhausted() const { return exhausted; }
};

class AudioManager
{
	friend class AudioStream;
private:
	bool muteAllStreams;
	bool audioAvailable;
	bool mixerOpened;
	EngineData* engineData;
	Mutex streamMutex;
	Mutex managerMutex;
	std::vector<AudioStream> streams;
public:
	AudioManager(EngineData* engine);
	AudioStream* createStream
	(
		AudioDecoder& decoder,
		bool startPaused,
		IThreadJob* producer,
		const TimeSpec& playedTime,
		number_t volume
	);

	template<typename F>
	void forEachStreamNoLock(F&& func)
	{
		for (auto& stream : streams)
			func(stream);
	}

	void forEachStream(std::function<void(AudioStream&)> func);
	void toggleMuteAll() { muteAllStreams ? unmuteAll() : muteAll(); }
	bool allMuted() const { return muteAllStreams; }
	void muteAll();
	void unmuteAll();
	void removeStream(AudioStream& stream);
	void stopAllSounds();
	~AudioManager();
};

class DLL_PUBLIC AudioStream
{
	friend class AudioManager;
	friend class NetStream;
private:
	AudioManager& manager;
	AudioDecoder& decoder;
	IThreadJob* producer;
	bool hasStarted;
	bool _isPaused;
	bool mixingStarted;
	ACQUIRE_RELEASE_FLAG(isDone);
	number_t curVolume;
	number_t unmuteVolume;
	SoundTransform panning;
	TimeSpec playedTime;
	TimeSpec startTime;
	int32_t mixerChannel;
public:
	bool init(number_t volume);
	void deinit();
	void startMixing();
	AudioStream
	(
		AudioManager& _manager,
		AudioDecoder& _decoder,
		IThreadJob* _producer,
		const TimeSpec& _playedTime
	);

	void setPause(bool flag);
	TimeSpec getPlayedTime();
	bool isPaused() const { return _isPaused; }
	void mute();
	void unmute();
	void pause() { setPause(true); }
	void resume() { setPause(false); }
	void setVolume(number_t volume) { curVolume = volume; }
	void setPlayedTime(const TimeSpec& time) { playedTime = time; }
	void setPanning(const SoundTransform& xform) { panning = xform; }

	void setIsDone();
	bool getIsDone() const { return ACQUIRE_READ(isDone); }
	number_t getVolume() const { return curVolume; }
	const SoundTransform& getPanning() { return panning; }
	AudioDecoder* getDecoder() const { return decoder; }
	~AudioStream() {}
};

}
#endif /* BACKENDS_AUDIO_H */
