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
#include <inttypes.h>
#include "swftypes.h"

namespace lightspark
{

class URLInfo
{
private:
	tiny_string url;
	tiny_string encodedUrl;
	tiny_string protocol;
	tiny_string hostname;
	uint32_t port;
	tiny_string path;
	tiny_string pathDirectory;
	tiny_string pathFile;
	tiny_string query;
public:
	URLInfo() {};
	URLInfo(const tiny_string& u, bool encoded=false);
	~URLInfo();
	static URLInfo getQualified(const tiny_string& u, bool encoded=false, const tiny_string& root="");
	tiny_string normalizePath(const tiny_string& u);
	static tiny_string encode(const tiny_string& u);
	static tiny_string decode(const tiny_string& u);

	const tiny_string& getURL() { return url; };
	const tiny_string& getEncodedURL() { return encodedUrl; };
	const tiny_string& getProtocol() { return protocol; };
	const tiny_string& getHostname() { return hostname; };
	uint32_t getPort() { return port; };
	const tiny_string& getPath() { return path; };
	const tiny_string& getPathDirectory() { return pathDirectory; };
	const tiny_string& getPathFile() { return pathFile; };
	const tiny_string& getQuery() { return query; };
};

};
#endif
