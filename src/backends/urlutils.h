/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2010-2013  Alessandro Pignotti (a.pignotti@sssup.it)
    Copyright (C) 2010-2011  Timon Van Overveldt (timonvo@gmail.com)

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

#ifndef BACKENDS_URLUTILS_H
#define BACKENDS_URLUTILS_H 1

#include "compat.h"
#include <iostream>
#include <string>
#include <list>
#include <utility>
#include "swftypes.h"

namespace lightspark
{

class DLL_PUBLIC URLInfo
{
	friend std::ostream& operator<<(std::ostream& s, const URLInfo& u);
private:
	static std::list<uint32_t> uriReservedAndHash;
	static std::list<uint32_t> uriUnescaped;
	static std::list<uint32_t> uriReservedAndUnescapedAndHash;
	static const uint32_t REPLACEMENT_CHARACTER = 0xFFFD;
	tiny_string url; //The URL space encoded
	tiny_string parsedURL; //The URL normalized and space encoded
	tiny_string protocol; //Part after
	tiny_string hostname; //Part after :// after protocol
	tiny_string path; //Part after first / after hostname
	tiny_string pathDirectory;
	tiny_string pathFile;
	tiny_string query; //Part after first ?
	tiny_string fragment; //Part after first #
	tiny_string stream; //The requested stream, used for RTMP protocols
public:
	enum INVALID_REASON { IS_EMPTY, MISSING_PROTOCOL, MISSING_PATH, MISSING_HOSTNAME, INVALID_PORT };
private:
	INVALID_REASON invalidReason;
	uint16_t port; //Part after first : after hostname
	bool valid;
	static tiny_string encodeURI(const tiny_string& u, const std::list<uint32_t>& unescapedChars);
	static tiny_string encodeSurrogatePair(CharIterator& c, const CharIterator& end);
	static tiny_string encodeSingleChar(uint32_t codepoint);
	static tiny_string encodeOctet(char c);
	static tiny_string decodeURI(const tiny_string& u, const std::list<uint32_t>& reservedChars);
	static uint32_t decodeSingleEscapeSequence(CharIterator& c, const CharIterator& end);
	static uint32_t decodeSingleChar(CharIterator& c, const CharIterator& end);
	static uint32_t decodeRestOfUTF8Sequence(uint32_t firstOctet, CharIterator& c, const CharIterator& end);
	static uint32_t decodeHexDigit(CharIterator& c, const CharIterator& end);
	static bool isSurrogateUTF8Sequence(const char *octets, unsigned int numOctets);
public:
	URLInfo():invalidReason(IS_EMPTY),valid(false) {}
	URLInfo(const tiny_string& u);
	~URLInfo() {}
	bool isValid() const { return valid; }
	bool isEmpty() const { return !valid && invalidReason == IS_EMPTY; }
	INVALID_REASON getInvalidReason() const { return invalidReason; }

	//Remove extraneous slashes, .. and . from URLs
	tiny_string normalizePath(const tiny_string& u);

	//Go to the specified URL, using the current URL as the root url for unqualified URLs
	const URLInfo goToURL(const tiny_string& u) const;

	//Check if this URL is a sub-URL of the given URL (same protocol & hostname, subpath)
	bool isSubOf(const URLInfo& parent) const;
	//Check if this URL has a path that is a sub-path of the given URL
	bool isSubPathOf(const URLInfo& parent) const { return isSubPathOf(parent.getPathDirectory(), getPathDirectory()); }
	//Check if a given path is a sub-path of another given path
	static bool isSubPathOf(const tiny_string& parent, const tiny_string& child);
	//Check if this URL has a domain that is a subdomain of the given URL
	bool isSubDomainOf(const URLInfo& parent) const { return isSubDomainOf(parent.getHostname(), getHostname()); }
	//Check if a given domain is a sub-domain of another given domain
	static bool isSubDomainOf(const tiny_string& parent, const tiny_string& child);
	//Check if a given domain a matches a given domain expression (can be used with wildcard domains)
	static bool matchesDomain(const tiny_string& expression, const tiny_string& subject);
	//Check if the given url has same protocol, hostname and port
	bool sameHost(const URLInfo& other) const;

	bool operator==(const std::string& subject) const
	{ return getParsedURL() == tiny_string(subject); }
	bool operator==(const tiny_string& subject) const
	{ return getParsedURL() == subject; }
	bool operator==(const URLInfo& subject) const
	{ return getParsedURL() == subject.getParsedURL(); }

	const tiny_string& getURL() const { return url; }
	const tiny_string& getParsedURL() const { return valid ? parsedURL : url; }
	const tiny_string& getProtocol() const { return protocol; }
	const tiny_string& getHostname() const { return hostname; }
	uint16_t getPort() const { return port; }
	const tiny_string& getPath() const { return path; }
	const tiny_string& getPathDirectory() const { return pathDirectory; }
	const tiny_string& getPathFile() const { return pathFile; }
	const tiny_string& getQuery() const { return query; }
	const tiny_string& getFragment() const { return fragment; }
	std::list< std::pair<tiny_string, tiny_string> > getQueryKeyValue() const;

	bool isRTMP() const;
	//Accessors to the RTMP requested stream
	const tiny_string& getStream() const { return stream; }
	void setStream(const tiny_string& s) { stream=s; }
	//ENCODE_SPACES is used for safety, it can run over another ENCODING without corrupting data
	enum ENCODING { ENCODE_SPACES, ENCODE_FORM, ENCODE_URI, ENCODE_URICOMPONENT, ENCODE_ESCAPE };
	static tiny_string encode(const tiny_string& u, ENCODING type=ENCODE_URICOMPONENT);
	static std::string encode(const std::string& u, ENCODING type=ENCODE_URICOMPONENT)
	{
		return std::string(encode(tiny_string(u), type));
	}
	static tiny_string decode(const std::string& u, ENCODING type=ENCODE_URICOMPONENT);
};

};
#endif /* BACKENDS_URLUTILS_H */
