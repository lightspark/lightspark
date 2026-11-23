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

#ifndef BACKENDS_REQUEST_H
#define BACKENDS_REQUEST_H 1

#include <cstdint>
#include <tsl/ordered_map.h>
#include <vector>

#include "tiny_string.h"
#include "utils/optional.h"

namespace lightspark
{

// Based on Ruffle's `backend::navigator::NavigationMethod`.
enum class RequestMethod
{
	Get,
	Post,
};

// Based on Ruffle's `backend::navigator::Request`.
class Request
{
public:
	using BodyPair = std::pair<std::vector<uint8_t>, tiny_string>;
	using HeaderMap = tsl::ordered_map<tiny_string, tiny_string>;
	using HeaderPair = HeaderMap::value_type;
private:
	tiny_string url;
	RequestMethod method;
	Optional<BodyPair> body;
	tsl::ordered_map<tiny_string, tiny_string> headers;
public:
	Request(const tiny_string& _url) : Request
	(
		RequestMethod::Get,
		_url,
		{}
	) {}

	Request
	(
		const tiny_string& _url,
		Optional<const BodyPair&> _body
	) : Request(RequestMethod::Post, _url, _body) {}

	Request
	(
		const RequestMethod& _method,
		const tiny_string& _url,
		Optional<const BodyPair&> _body
	) : url(_url), method(_method), body(_body) {}

	const tiny_string& getURL() const { return url; }
	void setURL(const tiny_string& _url) { url = _url; }
	const RequestMethod& getMethod() const { return method; }
	Optional<const BodyPair&> getBody() const { return body.asRef(); }
	void setBody(const BodyPair& _body) { body = _body; }
	const HeaderMap& getHeaders() const { return headers; }
	void setHeaders(const HeaderMap& _headers) { headers = _headers; }
	void addHeader(const HeaderPair& header)
	{
		headers.insert(header);
	}

	void addHeader(const tiny_string& name, const tiny_string& value)
	{
		headers.emplace(name, value);
	}
};

}
#endif /* BACKENDS_REQUEST_H */
