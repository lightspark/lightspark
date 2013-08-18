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

#include <libxml++/nodes/textnode.h>
#include "backends/xml_support.h"
#include "logger.h"

using namespace lightspark;
using namespace std;

#ifdef XMLPP_2_35_1
RecoveryDocument::RecoveryDocument(_xmlDoc* d):xmlpp::Document(d)
{
}

void RecoveryDomParser::parse_memory_raw(const unsigned char* contents, size_type bytes_count)
{
	release_underlying(); //Free any existing document.

	//The following is based on the implementation of xmlParseFile(), in xmlSAXParseFileWithData():
	context_ = xmlCreateMemoryParserCtxt((const char*)contents, bytes_count);
	if(!context_)
		throw xmlpp::internal_error("Couldn't create parsing context");

	xmlSAXHandlerV1* handler=(xmlSAXHandlerV1*)calloc(1,sizeof(xmlSAXHandlerV1));
	initxmlDefaultSAXHandler(handler, 0);
	context_->recovery=1;
	free(context_->sax);
	context_->sax=(xmlSAXHandler*)handler;
	context_->keepBlanks = 0;
	handler->ignorableWhitespace = xmlSAX2IgnorableWhitespace;

	//The following is based on the implementation of xmlParseFile(), in xmlSAXParseFileWithData():
	//and the implementation of xmlParseMemory(), in xmlSaxParseMemoryWithData().
	initialize_context();

	if(!context_)
		throw xmlpp::internal_error("Context not initialized");

	xmlParseDocument(context_);

	check_for_exception();

	if(!context_->wellFormed)
		LOG(LOG_ERROR, "XML data not well formed!");

	if (context_->myDoc)
		doc_ = new RecoveryDocument(context_->myDoc);
	// This is to indicate to release_underlying that we took the
	// ownership on the doc.
	context_->myDoc = 0;

	//Free the parse context, but keep the document alive so people can navigate the DOM tree:
	//TODO: Why not keep the context alive too?
	Parser::release_underlying();

	check_for_exception();
}
#endif

xmlpp::Node* XMLBase::buildFromString(const string& str,
				      bool ignoreEmptyTextNodes, bool *hasParent,
				      const string& default_ns)
{
	string buf = parserQuirks(str);
	try
	{
		parser.parse_memory_raw((const unsigned char*)buf.c_str(), buf.size());
	}
	catch(const exception& e)
	{
	}
	xmlpp::Document* doc=parser.get_document();
	if(doc && doc->get_root_node())
	{
		*hasParent = true;
		xmlpp::Element *root = doc->get_root_node();
		// It would be better to remove empty nodes during
		// parsing, but xmlpp doesn't offer an interface.
		if (ignoreEmptyTextNodes)
			removeWhitespaceNodes(root);
		addDefaultNamespace(root, default_ns);
		return root;
	}

	//If everything fails, create a fake document and add a single text string child
	// see 10.3.1 in ECMA 357
	if (default_ns.empty())
		buf="<parent></parent>";
	else
		buf="<parent xmlns=\"" + default_ns + "\"></parent>";
	parser.parse_memory_raw((const unsigned char*)buf.c_str(), buf.size());

	*hasParent = false;
	return parser.get_document()->get_root_node()->add_child_text(str);
}

void XMLBase::addDefaultNamespace(xmlpp::Element *root, const string& default_ns)
{
	if(default_ns.empty() || !root->get_namespace_uri().empty())
		return;

	xmlNodePtr node = root->cobj();
	xmlNsPtr ns = xmlNewNs(node, BAD_CAST default_ns.c_str(), NULL);
	addDefaultNamespaceRecursive(node, ns);
}

void XMLBase::addDefaultNamespaceRecursive(xmlNodePtr node, xmlNsPtr ns)
{
	//Set the default namespace to nodes by descending until we
	//encounter another namespace.
	if ((node->type != XML_ELEMENT_NODE) || (node->ns != NULL))
		return;

	xmlSetNs(node, ns);

	xmlNodePtr child=node->children;
	while(child)
	{
		addDefaultNamespaceRecursive(child, ns);
		child = child->next;
	}
}

xmlpp::Node* XMLBase::buildCopy(const xmlpp::Node* src, bool *hasParent)
{
	const xmlpp::ContentNode* contentnode;
	const xmlpp::TextNode* textnode=dynamic_cast<const xmlpp::TextNode*>(src);
	if(textnode)
	{
		return buildFromString(textnode->get_content(), false,hasParent);
	}
	else if ((contentnode = dynamic_cast<const xmlpp::ContentNode*>(src)))
	{
		// ContentNode but not TextNode => comment, PI or CData
		// These can't be root nodes so we add a dummy root.
		*hasParent = false;
		xmlpp::Element* root = parser.get_document()->create_root_node("dummy_root");
		return root->import_node(contentnode);
	}
	else
	{
		*hasParent = true;
		return parser.get_document()->create_root_node_by_import(src);
	}
}

// Adobe player's XML parser accepts many strings which are not valid
// XML according to the specs. This function attempts to massage
// invalid-but-accepted-by-Adobe strings into valid XML so that
// libxml++ parser doesn't throw an error.
string XMLBase::parserQuirks(const string& str)
{
	string buf = quirkCData(str);
	buf = quirkXMLDeclarationInMiddle(buf);
	return buf;
}

string XMLBase::quirkCData(const string& str) {
	//if this is a CDATA node replace CDATA tags to make it look like a text-node
	//for compatibility with the Adobe player
	if (str.compare(0, 9, "<![CDATA[") == 0) {
		return "<a>"+str.substr(9, str.size()-12)+"</a>";
	}
	else
		return str;
}

string XMLBase::quirkXMLDeclarationInMiddle(const string& str) {
	string buf(str);

	// Adobe player ignores XML declarations in the middle of a
	// string.
	while (true)
	{
		size_t start = buf.find("<?xml ", 1);
		if (start == buf.npos)
			break;
		
		size_t end = buf.find("?>", start+5);
		if (end == buf.npos)
			break;
		end += 2;
		
		buf.erase(start, end-start);
	}

	return buf;
}

void XMLBase::removeWhitespaceNodes(xmlpp::Element *node)
{
	xmlpp::Node::NodeList children = node->get_children();
	xmlpp::Node::NodeList::iterator it;
	for (it=children.begin(); it!=children.end(); ++it)
	{
		xmlpp::Element *element = dynamic_cast<xmlpp::Element*>(*it);
		xmlNode *xmlnode = (*it)->cobj();
		if (xmlnode->type == XML_TEXT_NODE && xmlIsBlankNode(xmlnode))
		{
			node->remove_child(*it);
		}
		else if (element)
		{
			removeWhitespaceNodes(element);
		}
	}
}
