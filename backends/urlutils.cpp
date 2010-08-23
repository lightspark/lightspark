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

#include "swf.h"
#include "urlutils.h"
#include "compat.h"
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#ifdef ENABLE_CURL
#include <curl/curl.h>
#endif

using namespace lightspark;
extern TLSDATA SystemState* sys;

URLInfo::URLInfo(const tiny_string& u, bool encoded)
{
	if(encoded)
	{
		encodedUrl = u;
		url = decode(u);
	}
	else
	{
		url = u;
		encodedUrl = encode(u);
	}

	std::string str = std::string(url.raw_buf());

	//Check for :// marking that there is a protocol in this url
	size_t colonPos = str.find("://");
	if(colonPos == std::string::npos)
		throw RunTimeException("URLInfo: invalid url: no :// in url");

	protocol = str.substr(0, colonPos);

	size_t hostnamePos = colonPos+3;
	size_t portPos = std::string::npos;
	size_t pathPos = std::string::npos;
	size_t queryPos = std::string::npos;
	size_t cursor = hostnamePos;

	if(protocol == "file")
	{
		pathPos = cursor;
		if(pathPos == str.length()-1)
			throw RunTimeException("URLInfo: invalid file:// url: no path");
	}
	else
	{
		pathPos = str.find("/", cursor);
		cursor = pathPos;

		portPos = str.rfind(":", cursor);
		if(portPos < hostnamePos)
			portPos = std::string::npos;

		queryPos = str.find("?", cursor);

		if(portPos == hostnamePos || pathPos == hostnamePos || queryPos == hostnamePos)
			throw RunTimeException("URLInfo: invalid url: no hostname");
	}

	//Parse the host string
	hostname = str.substr(hostnamePos, std::min(std::min(pathPos, portPos), queryPos)-hostnamePos);

	port = 0;
	//Check if the port after ':' is not empty
	if(portPos != std::string::npos && portPos != std::min(std::min(str.length()-1, pathPos-1), queryPos-1))
	{
		portPos++;
		std::istringstream i(str.substr(portPos, std::min(pathPos, str.length())-portPos));
		if(!(i >> port))
			throw RunTimeException("URLInfo: invalid port");
	}

	//Parse the path string
	if(pathPos != std::string::npos)
	{
		path = normalizePath(tiny_string(str.substr(
						pathPos, std::min(str.length(), queryPos)-pathPos
						).c_str(), true).raw_buf());
		std::string pathStr = std::string(path.raw_buf());

		size_t lastSlash = pathStr.rfind("/");
		if(lastSlash != std::string::npos)
		{
			pathDirectory = pathStr.substr(0, lastSlash+1);
			pathFile = pathStr.substr(lastSlash+1);
		}
		else
		{
			pathDirectory = path;
			pathFile = "";
		}
	}
	else
	{
		path = "/";
		pathDirectory = path;
		pathFile = "";
	}

	//Copy the query string
	if(queryPos != std::string::npos && queryPos < str.length()-1)
		query = str.substr(queryPos+1);
	else
		query = "";
}

URLInfo::~URLInfo()
{
}

tiny_string URLInfo::normalizePath(const tiny_string& u)
{
	std::string pathStr(u.raw_buf());
	size_t doubleSlash = pathStr.find("//");
	while(doubleSlash != std::string::npos)
	{
		pathStr.replace(doubleSlash, 2, "/");
		doubleSlash = pathStr.find("//");
	}
	size_t doubleDot = pathStr.find("/../");
	size_t previousSlash;
	while(doubleDot != std::string::npos)
	{
		//We are at root, .. doesn't mean anything, just erase
		if(doubleDot == 0)
		{
			pathStr.replace(doubleDot, 3, "");
		}
		//Replace .. with one dir up
		else
		{
			previousSlash = pathStr.rfind("/", doubleDot-2);
			pathStr.replace(previousSlash, doubleDot-previousSlash+3, "");
		}
		doubleDot = pathStr.find("/../");
	}
	if(pathStr.substr(pathStr.length()-3, 3) == "/..")
	{
		previousSlash = pathStr.rfind("/", pathStr.length()-4);
		pathStr.replace(previousSlash, pathStr.length()-previousSlash+2, "/");
	}
	//Eliminate meaningless /./
	size_t singleDot = pathStr.find("/./");
	while(singleDot != std::string::npos)
	{
		pathStr.replace(singleDot, 2, "");
		singleDot = pathStr.find("/./");
	}


	//Remove redundant last dot
	if(pathStr.length() >= 2 && pathStr.substr(pathStr.length()-2, 2) == "/.")
		pathStr.replace(pathStr.length()-1, 1, "");

	return tiny_string(pathStr);
}

URLInfo URLInfo::getQualified(const tiny_string& u, bool encoded, const tiny_string& root)
{
	std::string str;
	if(encoded)
		str = u.raw_buf();
	else
		str = URLInfo::encode(u).raw_buf();
	//No protocol, treat this as an unqualified URL
	if(str.find(":") == std::string::npos)
	{
		std::string qualified = std::string();
		URLInfo rootUrl;
		if(root == "")
		{
			if(sys->getUrl() == "")
				throw RunTimeException("URLInfo::getQualified: no root url to qualify from");
			rootUrl = URLInfo(sys->getUrl());
		}
		else
			rootUrl = URLInfo(root);
		qualified = std::string(rootUrl.getProtocol().raw_buf());
		qualified += "://";
		qualified += std::string(rootUrl.getHostname().raw_buf());
		qualified += std::string(rootUrl.getPort() > 0 ? ":" + rootUrl.getPort() : "");
		qualified += std::string(rootUrl.getPathDirectory().raw_buf());
		qualified += str;
		return URLInfo(qualified.c_str(), true);
	}
	else //Protocol specified, treat this as a qualified URL
	{
		return URLInfo(u, encoded);
	}
}

tiny_string URLInfo::encode(const tiny_string& u)
{
	//TODO: Fully URL encode the string
	std::string tmp2;
	tmp2.reserve(u.len()*2);
	for(int i=0;i<u.len();i++)
	{
		if(u[i]==' ')
		{
			char buf[4];
			sprintf(buf,"%%%x",(unsigned char)u[i]);
			tmp2+=buf;
		}
		else
			tmp2.push_back(u[i]);
	}
	return tiny_string(tmp2);
}

tiny_string URLInfo::decode(const tiny_string& u)
{
	//TODO: Fully URL decode the string
	std::string tmp2;
	tmp2.reserve(u.len()*2);
	for(int i=0;i<u.len();i++)
	{
		if(i+2 < u.len() && u[i]=='%' && u[i+1]=='2' && u[i+2]=='0')
		{
			tmp2.push_back(' ');
			i+=2;
		}
		else
			tmp2.push_back(u[i]);
	}
	return tiny_string(tmp2);
}

