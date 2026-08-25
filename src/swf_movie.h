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

#ifndef SWF_MOVIE_H
#define SWF_MOVIE_H 1

#include <cstdint>
#include <cstdlib>
#include <utility>
#include <vector>

#include "backends/security.h"
#include "backends/urlutils.h"
#include "parsing/tags.h"
#include "swftypes.h"
#include "tiny_string.h"
#include "utils/optional.h"
#include "utils/span.h"

namespace lightspark
{

class SystemState;

// Based on Ruffle's `ruffle_common::tag_utils::SwfMovie`.
class SWFMovie
{
	using FlashVarPair = std::pair<tiny_string, tiny_string>;
	using SandboxType = SecurityManager::SANDBOXTYPE;
private:
	SystemState* sys;
	SWFHeader header;
	FileAttributesTag fileAttrs;
	Optional<RGB> backgroundColor;
	size_t size;

	Span<const uint8_t> data;
	URLInfo url;
	Optional<URLInfo> loaderURL;
	std::vector<FlashVarPair> flashVars;
	TextEncoding encoding;
	size_t compressedSize;
	bool _isMovie;
	SandboxType sandboxType;

	static SandboxType inferSandboxType(const URLInfo& url);
public:
	SWFMovie
	(
		SystemState* _sys,
		uint8_t swfVersion,
		Optional<const URLInfo&> _loaderURL = {}
	);

	SWFMovie
	(
		SystemState* _sys,
		uint8_t swfVersion,
		size_t _compresedSize,
		Optional<const URLInfo&> _loaderURL = {}
	);

	SWFMovie
	(
		SystemState* _sys,
		uint8_t swfVersion,
		Span<const uint8_t> compressedData,
		Optional<const URLInfo&> _loaderURL = {}
	);

	SWFMovie(SystemState* _sys, const URLInfo& _url);

	SWFMovie
	(
		SystemState* _sys,
		Span<const uint8_t> _data,
		const URLInfo& _url,
		Optional<const URLInfo&> _loaderURL = {}
	);

	SWFMovie(SystemState* _sys, const URLInfo& _url, size_t _size);

	SystemState* getSys() const { return sys; }
	const SWFHeader& getHeader() const { return header; }
	uint8_t getVersion() const { return header.getVersion(); }
	const Span<const uint8_t>& getData() const { return data; }
	const TextEncoding& getEncoding() const { return encoding; }
	const Twips& getWidth() const { return getStageSize().x; }
	const Twips& getHeight() const { return getStageSize().y; }
	const URLInfo& getURL() const { return url; }
	void setURL(const URLInfo& _url) { url = _url; }
	Optional<const URLInfo&> getLoaderURL() const
	{
		return loaderURL.asRef();
	}

	Span<const FlashVarPair> getFlashVars() const
	{
		return makeSpan(flashVars);
	}

	template<typename Iter>
	void addFlashVars(Iter begin, Iter end)
	{
		flashVars.insert(flashVars.end(), begin, end);
	}

	void addFlashVars(Span<const FlashVarPair> vars)
	{
		addFlashVars(vars.begin(), vars.end());
	}

	size_t getCompressedSize() const { return compressedSize; }
	size_t getSize() const { return size; }
	bool isAS3() const { return fileAttrs.ActionScript3; }
	const Vector2Twips& getStageSize() const
	{
		return header.getStageSize();
	}

	uint16_t getTotalFrames() const
	{
		return header.getTotalFrames();
	}

	const Fixed8& getFrameRate() const
	{
		return header.getFrameRate();
	}

	bool isMovie() const { return _isMovie; }
	const SandboxType getSandboxType() { return sandboxType; }
};

class EmptyTag {};

// Based on Ruffle's `ruffle_common::tag_utils::SwfSlice`.
class SWFSpan
{
private:
	SWFMovie& movie;
	size_t start;
	size_t end;
public:
	SWFSpan(SWFMovie& _movie, EmptyTag) : SWFSpan(_movie, 0, 0) {}
	SWFSpan(SWFMovie& _movie) : SWFSpan
	(
		_movie,
		0,
		_movie.getData().getSize()
	) {}

	SWFSpan
	(
		SWFMovie& _movie,
		size_t _start,
		size_t _end
	) : movie(_movie), start(_start), end(_end) {}

	SWFSpan toSubSpan(Span<const uint8_t> span) const;
	SWFSpan toUnboundedSubSpan(Span<const uint8_t> span) const;
	SWFSpan subSpan(size_t _start, size_t _end) const;

	Span<const uint8_t> getData() const
	{
		return movie.getData().subSpan(start, getSize());
	}

	uint8_t getVersion() const { return movie.getVersion(); }
	bool isEmpty() const { return end == start; }
	bool empty() const { return isEmpty(); }
	size_t getSize() const { return end - start; }
};

}
#endif /* SWF_MOVIE_H */
