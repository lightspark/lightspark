/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2010-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef PARSING_CROSSDOMAINPOLICY_H
#define PARSING_CROSSDOMAINPOLICY_H 1

#include "compat.h"
#include <string>
#include "swftypes.h"
#include <3rdparty/pugixml/src/pugixml.hpp>

namespace lightspark
{
	class CrossDomainPolicy
	{
	public:
		enum POLICYFILETYPE { URL, SOCKET };
		enum POLICYFILESUBTYPE { NONE, HTTP, HTTPS, FTP };
	private:
		pugi::xml_document xml;
		pugi::xml_node currentnode;
		POLICYFILETYPE type;
		POLICYFILESUBTYPE subtype;
		bool master;
		bool first;

		//Ease-of-use variables
		std::string tagName;
		int attrCount;
		std::string attrValue;

		bool siteControlFound;

		//Parsed element attributes
		//site-control
		std::string permittedPolicies;
		//allow-access-from & allow-http-request-headers-from
		std::string domain;
		bool secure;
		bool secureSpecified;
		//allow-access-from
		std::string toPorts;
		//allow-http-request-headers-from
		std::string headers;
	public:
		CrossDomainPolicy(const unsigned char* buffer, size_t length, POLICYFILETYPE _type, POLICYFILESUBTYPE _subtype, bool _master);
		enum ELEMENT { END, INVALID, SITE_CONTROL, ALLOW_ACCESS_FROM, ALLOW_HTTP_REQUEST_HEADERS_FROM };
		ELEMENT getNextElement();
		//site-control
		const std::string& getPermittedPolicies() const { return permittedPolicies; }
		//allow-access-from & allow-http-request-headers-from
		const std::string& getDomain() const { return domain; }
		bool getSecure() const { return secure; };
		bool getSecureSpecified() const { return secureSpecified; }
		//allow-access-from
		const std::string& getToPorts() const { return toPorts; }
		//allow-http-request-headers-from
		const std::string& getHeaders() const { return headers; }
	};
}

#endif /* PARSING_CROSSDOMAINPOLICY_H */
