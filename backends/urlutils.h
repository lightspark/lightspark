/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009,2010  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef _URL_UTILS_H
#define _URL_UTILS_H

#include "compat.h"
#include <string>
#include <inttypes.h>
#include "swftypes.h"

namespace lightspark
{

class DLL_PUBLIC URLInfo
{
private:
	tiny_string url; //The URL space encoded
	tiny_string parsedURL; //The URL normalized and space encoded
	tiny_string protocol; //Part after
	tiny_string hostname; //Part after :// after protocol
	uint16_t port; //Part after first : after hostname
	tiny_string path; //Part after first / after hostname
	tiny_string pathDirectory;
	tiny_string pathFile;
	tiny_string query; //Part after first ?
	tiny_string fragment; //Part after first #
public:
	URLInfo() {};
	URLInfo(const tiny_string& u);
	~URLInfo() {};

	//Go to the specified URL, using the current URL as the root url for unqualified URLs
	const URLInfo goToURL(const tiny_string& u) const;

	//Check if the given URL is a sub-URL of the given URL
	bool isSubOf(const tiny_string& u) const { return isSubOf(URLInfo(u)); };
	bool isSubOf(const URLInfo& u) const;

	//Remove extraneous slashes, .. and . from URLs
	tiny_string normalizePath(const tiny_string& u);

	//ENCODE_SPACES is used for safety, it can run over another ENCODING without corrupting data
	enum ENCODING { ENCODE_SPACES, ENCODE_FORM, ENCODE_URI, ENCODE_URICOMPONENT, ENCODE_ESCAPE };
	static tiny_string encode(const tiny_string& u, ENCODING type=ENCODE_URICOMPONENT)
	{
		return tiny_string(encode(std::string(u.raw_buf()), type));
	};
	static std::string encode(const std::string& u, ENCODING type=ENCODE_URICOMPONENT);
	static tiny_string decode(const tiny_string& u, ENCODING type=ENCODE_URICOMPONENT)
	{
		return tiny_string(decode(std::string(u.raw_buf()), type));
	};
	static std::string decode(const std::string& u, ENCODING type=ENCODE_URICOMPONENT);

	const tiny_string& getURL() const { return url; };
	const tiny_string& getParsedURL() const { return parsedURL; };
	const tiny_string& getProtocol() const { return protocol; };
	const tiny_string& getHostname() const { return hostname; };
	uint16_t getPort() const { return port; };
	const tiny_string& getPath() const { return path; };
	const tiny_string& getPathDirectory() const { return pathDirectory; };
	const tiny_string& getPathFile() const { return pathFile; };
	const tiny_string& getQuery() const { return query; };
	const tiny_string& getFragment() const { return fragment; };
};

};
#endif
