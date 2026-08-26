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

#include "parsing/streams.h"
#include "swf.h"
#include "swf_movie.h"

using namespace lightspark;

static const char* getInvalidReasonStr(const URLInfo& url)
{
	using InvalidReason = URLInfo::INVALID_REASON;
	switch (url.getInvalidReason())
	{
		case InvalidReason::IS_EMPTY:
			return "URL is empty";
		case InvalidReason::MISSING_PROTOCOL:
			return "URL has no protocol";
		case InvalidReason::MISSING_PATH:
			return "URL has no path";
		case InvalidReason::MISSING_HOSTNAME:
			return "URL has no hostname";
		case InvalidReason::INVALID_PORT:
			return "URL has an invalid port";
		default:
			return "Unknown reason";
	}
}

// Based on Ruffle's `ruffle_common::sandbox::SandboxType::infer()`.
SWFMovie::SandboxType SWFMovie::inferSandboxType
(
	const URLInfo& url,
	const SWFExtHeader& header
)
{
	if (!url.isValid())
	{
		LOG
		(
			LOG_ERROR,
			"SWFMovie::inferSandboxType(): "
			"Failed to parse URL \"" << url << "\". "
			"Reason: " << getInvalidReasonStr(url) << ". "
			"Using LOCAL_WITH_FILE."
		);
		return SandboxType::LOCAL_WITH_FILE;
	}

	if (url.getProtocol() != "file")
		return SandboxType::REMOTE;
	return
	(
		header.useNetworkSandbox() ?
		SandboxType::LOCAL_WITH_NETWORK :
		SandboxType::LOCAL_WITH_FILE
	);
}

// Based on Ruffle's `SwfMovie::append_parameters_from_url()`.
void SWFMovie::addFlashVarsFromURL()
{
	if (!url.isValid())
	{
		LOG
		(
			LOG_ERROR,
			"SWFMovie::addFlashVarsFromURL(): "
			"Failed to parse URL while extracting it's query parameters. "
			"Reason: " << getInvalidReasonStr(url) << '.'
		);
		return;
	}

	auto _flashVars = url.getQueryKeyValue();
	addFlashvars(_flashVars.begin(), _flashVars.end());
}

// Based on Ruffle's `SwfMovie::empty()`.
SWFMovie::SWFMovie
(
	SystemState* _sys,
	uint8_t swfVersion,
	Optional<const URLInfo&> _loaderURL
) :
sys(_sys),
header(swfVersion),
url("file:///"),
loaderURL(_loaderURL),
encoding(TextEncoding::UTF8),
compressedSize(0),
_isMovie(false),
sandboxType(inferSandboxType(url, header))
{
}

// Based on Ruffle's `SwfMovie::fake_with_compressed_len()`.
SWFMovie::SWFMovie
(
	SystemState* _sys,
	uint8_t swfVersion,
	size_t _compresedSize,
	Optional<const URLInfo&> _loaderURL
) :
sys(_sys),
header(swfVersion),
url("file:///"),
loaderURL(_loaderURL),
encoding(TextEncoding::UTF8),
compressedSize(_compressedSize),
_isMovie(false),
sandboxType(inferSandboxType(url, header))
{
}

// Based on Ruffle's `SwfMovie::fake_with_compressed_data()`.
SWFMovie::SWFMovie
(
	SystemState* _sys,
	uint8_t swfVersion,
	Span<const uint8_t> compressedData,
	Optional<const URLInfo&> _loaderURL
) :
sys(_sys),
header(swfVersion),
data(compressedData.begin(), compressedData.end()),
url("file:///"),
loaderURL(_loaderURL),
encoding(TextEncoding::UTF8),
compressedSize(compressedData.getSize()),
_isMovie(false),
sandboxType(inferSandboxType(url, header))
{
}

// Based on Ruffle's `SwfMovie::error_movie()`.
SWFMovie::SWFMovie(SystemState* _sys, const URLInfo& _url) :
sys(_sys),
url(_url),
encoding(TextEncoding::UTF8),
compressedSize(0),
_isMovie(false),
sandboxType(inferSandboxType(_url, header))
{
}

// Based on Ruffle's `SwfMovie::from_data()`.
SWFMovie::SWFMovie
(
	SystemState* _sys,
	Span<const uint8_t> _data,
	const URLInfo& _url,
	Optional<const URLInfo&> _loaderURL
) :
sys(_sys),
url(_url),
loaderURL(_loaderURL),
compressedSize(_data.getSize()),
_isMovie(true)
{
	bytes_buf buf(_data.getData(), _data.getSize());
	auto swfBuf = ParseThread(std::istream(&buf)).decompressSWF();

	header = swfBuf.getHeader();
	data = swfBuf.getData();
	encoding = tiny_string::getSwfEncoding(header.getVersion());
	sandboxType = inferSandboxType(_url, header);
}

// Based on Ruffle's `SwfMovie::from_loaded_image()`.
SWFMovie::SWFMovie(SystemState* _sys, const URLInfo& _url, size_t size) :
sys(_sys),
header(size),
url(_url),
encoding(TextEncoding::UTF8),
compressedSize(size),
_isMovie(false),
sandboxType(inferSandboxType(_url, header))
{
	addFlashVarsFromURL();
}

// Based on Ruffle's `SwfSlice::to_subslice()`.
SWFSpan SWFSpan::toSubSpan(Span<const uint8_t> span) const
{
	auto data = movie.getData();
	auto dataPtr = data.getData();
	auto spanPtr = span.getData();
	if (spanPtr < (dataPtr + start) || spanPtr >= (dataPtr + end))
		return SWFSpan(movie, EmptyTag {});

	size_t _start = spanPtr - dataPtr;
	return SWFSpan(movie, _start, _start + span.getSize());
}

// Based on Ruffle's `SwfSlice::to_unbounded_subslice()`.
SWFSpan SWFSpan::toUnboundedSubSpan(Span<const uint8_t> span) const
{
	auto data = movie.getData();
	auto dataPtr = data.getData();
	auto dataSize = data.getSize();
	auto spanPtr = span.getData();
	if (spanPtr < dataPtr || spanPtr >= (dataPtr + dataSize))
		return SWFSpan(movie, EmptyTag {});

	size_t _start = spanPtr - dataPtr;
	return SWFSpan(movie, _start, _start + span.getSize());
}

// Based on Ruffle's `SwfSlice::to_start_end()`.
SWFSpan SWFSpan::subSpan(size_t _start, size_t _end) const
{
	auto data = movie.getData();
	auto span = data.trySubSpan(start + _start, end + _end);
	if (span.empty())
		return SWFSpan(movie, EmptyTag {});
	return toSubSpan(span);
}
