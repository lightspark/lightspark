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

#ifndef INTERFACES_BACKENDS_DECODER_H
#define INTERFACES_BACKENDS_DECODER_H 1

#include <cstdint>
#include <cstdlib>
#include <utility>

namespace lightspark
{

enum LS_VIDEO_CODEC;
class TimeSpec;
template<typename T, size_t N = size_t(-1)>
class Span;

class IAudioDecoder
{
public:
	template<typename T>
	using SamplePair = std::pair<T, T>;
	using F32SamplePair = SamplePair<float>;
	using S16SamplePair = SamplePair<int16_t>;

	virtual ~IAudioDecoder() {}
	virtual void switchCodec
	(
		const LS_VIDEO_CODEC& codecId,
		Span<const uint8_t> initData
	) = 0;

	virtual size_t decodeData
	(
		Span<const uint8_t> data,
		const TimeSpec& time
	) = 0;

	virtual bool isResampled() const = 0;
	virtual bool hasDecodedFrames() const = 0;
	virtual size_t getSampleRate() const = 0;
	virtual F32SamplePair getNextSampleF32() = 0;
	virtual S16SamplePair getNextSampleS16() = 0;
	virtual size_t getSamples(Span<F32SamplePair> span) = 0;
	virtual size_t getSamples(Span<S16SamplePair> span) = 0;
};

class ISeekableAudioDecoder : public IAudioDecoder
{
public:
	virtual void seekToPos(const TimeSpec& pos) = 0;
	virtual void seekToSampleFrame(size_t frame) = 0;
};

}
#endif /* INTERFACES_BACKENDS_DECODER_H */
