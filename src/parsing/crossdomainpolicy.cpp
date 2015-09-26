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

#include "parsing/crossdomainpolicy.h"

using namespace lightspark;

CrossDomainPolicy::CrossDomainPolicy(const unsigned char* buffer, size_t length, 
		POLICYFILETYPE _type, POLICYFILESUBTYPE _subtype, bool _master):
	type(_type),subtype(_subtype),master(_master),first(true),siteControlFound(false)
{
	xml.load_buffer(buffer, length);
}

CrossDomainPolicy::ELEMENT CrossDomainPolicy::getNextElement()
{
	while (true)
	{
		if (first)
		{
			currentnode = xml.root().first_child();
			if (strcmp(currentnode.name(), "cross-domain-policy"))
				return INVALID;
			currentnode = currentnode.first_child();
		}
		else
			currentnode = currentnode.next_sibling();
		first = false;
		// last node reached
		if (currentnode.type() == pugi::node_null)
			break;
		//We only handle elements
		if (currentnode.type() != pugi::node_element)
			continue;

		tagName = currentnode.name();
		attrCount = 0;
		for (auto it = currentnode.attributes_begin(); it !=currentnode.attributes_end(); it++)
			attrCount++;

		//We are inside the cross-domain-policy tag so lets handle elements.
		
		//Handle the site-control element, if this is a master file and if the element has attributes
		if(tagName == "site-control")
		{
			if(!siteControlFound && master && attrCount == 1)
			{
				siteControlFound = true;
				permittedPolicies = currentnode.attribute("permitted-cross-domain-policies").value();
				//Found the required attribute, passing control
				if(permittedPolicies != "")
					return SITE_CONTROL;
				else
					return INVALID;
			}
			else
				return INVALID;
		}
		//Handle the allow-access-from element if the element has attributes
		else if(tagName == "allow-access-from")
		{
			if(attrCount >= 1 && attrCount <= 3)
			{
				domain = currentnode.attribute("domain").value();
				toPorts = currentnode.attribute("to-ports").value();
				secure = false;
				secureSpecified = false;
				if(!strcmp(currentnode.attribute("secure").value(), "false"))
				{
					secure = false;
					secureSpecified = true;
				}
				else if(!strcmp(currentnode.attribute("secure").value(), "true"))
				{
					secure = true;
					secureSpecified = true;
				}
				//We found one of the required attributes, passing control
				if(type == URL && domain != "")
					return ALLOW_ACCESS_FROM;
				else if(type == SOCKET && domain != "" && toPorts != "")
					return ALLOW_ACCESS_FROM;
				else
					return INVALID;
			}
			else
				return INVALID;
		}
		//Handle the allow-http-request-headers-from element if the element has attributes and if the policy file type is HTTP(S)
		else if(tagName == "allow-http-request-headers-from")
		{
			if(type == URL && (subtype == HTTP || subtype == HTTPS) && attrCount >= 2 && attrCount <= 3)
			{
				domain = currentnode.attribute("domain").value();
				headers = currentnode.attribute("headers").value();
				secure = false;
				secureSpecified = false;
				if(!strcmp(currentnode.attribute("secure").value(), "false"))
				{
					secure = false;
					secureSpecified = true;
				}
				else if(!strcmp(currentnode.attribute("secure").value(), "true"))
				{
					secure = true;
					secureSpecified = true;
				}
				//We found the required attributes, passing control
				if(domain != "" && headers != "")
					return ALLOW_HTTP_REQUEST_HEADERS_FROM;
				else
					return INVALID;
			}
			else
				return INVALID;
		}
	}
	return END;
}
