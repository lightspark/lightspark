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

#include "backends/xml_support.h"
#include "logger.h"
#include "scripting/abc.h"
#include "scripting/toplevel/Error.h"
#include "scripting/class.h"

using namespace lightspark;
using namespace std;

const pugi::xml_node XMLBase::buildFromString(const tiny_string& str,
										unsigned int xmlparsemode,
										pugi::xml_parse_result* parseresult)
{
	tiny_string buf = str.removeWhitespace().encodeNull();
	if (buf.numBytes() > 0 && buf.charAt(0) == '<')
	{
		pugi::xml_parse_result res = xmldoc.load_buffer((void*)buf.raw_buf(),buf.numBytes(),xmlparsemode);
		if (parseresult)
		{
			// error handling is done in the caller
			*parseresult = res;
			return xmldoc.root();
		}
		switch (res.status)
		{
			case pugi::status_ok:
				break;
			case pugi::status_bad_start_element:
				createError<TypeError>(getWorker(),kXMLBadQName);
				break;
			case pugi::status_end_element_mismatch:
				createError<TypeError>(getWorker(),kXMLUnterminatedElementTag);
				break;
			case pugi::status_unrecognized_tag:
				createError<TypeError>(getWorker(),kXMLMalformedElement);
				break;
			case pugi::status_bad_pi:
				createError<TypeError>(getWorker(),kXMLUnterminatedProcessingInstruction);
				break;
			case pugi::status_bad_declaration:
				createError<TypeError>(getWorker(),kXMLUnterminatedXMLDecl);
				break;
			case pugi::status_bad_attribute:
				createError<TypeError>(getWorker(),kXMLUnterminatedAttribute);
				break;
			case pugi::status_bad_cdata:
				createError<TypeError>(getWorker(),kXMLUnterminatedCData);
				break;
			case pugi::status_bad_doctype:
				createError<TypeError>(getWorker(),kXMLUnterminatedDocTypeDecl);
				break;
			case pugi::status_bad_comment:
				createError<TypeError>(getWorker(),kXMLUnterminatedComment);
				break;
			case pugi::status_bad_end_element:
				createError<TypeError>(getWorker(),kXMLMalformedElement);
				break;
			default:
				LOG(LOG_ERROR,"xml parser error:"<<buf<<" "<<res.status<<" "<<res.description());
				break;
		}
	}
	else
	{
		pugi::xml_node n = xmldoc.append_child(pugi::node_pcdata);
		n.set_value(str.raw_buf());
	}
	return xmldoc.root();
}
const tiny_string XMLBase::encodeToXML(const tiny_string value, bool bIsAttribute)
{

	tiny_string res;
	auto it = value.begin();
	while (it != value.end())
	{
		switch (*it)
		{
			case '<':
				res += "&lt;";
				break;
			case '>':
				res += bIsAttribute ? ">" : "&gt;";
				break;
			case '&':
				res += "&amp;";
				break;
			case '\"':
				res += bIsAttribute ? "&quot;" : "\"";
				break;
			case '\r':
				res += bIsAttribute ? "&#xD;" : "\r";
				break;
			case '\n':
				res += bIsAttribute ? "&#xA;" : "\n";
				break;
			case '\t':
				res += bIsAttribute ? "&#x9;" : "\t";
				break;
			default:
				res += *it;
				break;
		}
		it++;
	}
	return res;
}
