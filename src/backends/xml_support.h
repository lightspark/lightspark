/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2012-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef BACKENDS_XML_SUPPORT_H
#define BACKENDS_XML_SUPPORT_H 1

#include "tiny_string.h"
#include <3rdparty/pugixml/src/pugixml.hpp>
namespace lightspark
{


/*
 * Base class for both XML and XMLNode
 */
class XMLBase
{
protected:
	//The parser will destroy the document and all the childs on destruction
	pugi::xml_document xmldoc;
	// if parseresult is not null, this method will not throw an exception on invalid xml
	const pugi::xml_node buildFromString(const tiny_string& str,
										unsigned int xmlparsemode,
										pugi::xml_parse_result* parseresult=nullptr);

public:
	static const tiny_string encodeToXML(const tiny_string value, bool bIsAttribute);
};

}

#endif /* BACKENDS_XML_SUPPORT_H */
