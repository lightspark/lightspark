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

#ifndef SCRIPTING_AVM1_TOPLEVEL_XML_H
#define SCRIPTING_AVM1_TOPLEVEL_XML_H 1

#include "gc/ptr.h"
#include "scripting/avm1/function.h"
#include "scripting/avm1/object.h"
#include "scripting/avm1/toplevel/XMLNode.h"
#include "utils/optional.h"
#include "tiny_string.h"

// Based on Ruffle's `avm1::globals::xml` crate.

namespace lightspark
{

class AVM1Activation;
class AVM1Context;
class AVM1DeclContext;
class AVM1SystemClass;
class AVM1Value;
class SystemState;

enum class XMLStatus
{
	// No error: Parsing was successful.
	NoError = 0,

	// A `CDATA` section wasn't properly terminated.
	CDATANotTerminated = -2,

	// The document's XML declaration wasn't properly terminated.
	XMLDeclNotTerminated = -3,

	// The `DOCTYPE` declaration wasn't properly terminated.
	DOCTYPENotTerminated = -4,

	// A comment wasn't properly terminated.
	CommentNotTerminated = -5,

	// An XML element was malformed.
	ElementMalformed = -6,

	// Out of memory.
	OutOfMemory = -7,

	// An attribute wasn't properly terminated.
	AttrNotTerminated = -8,

	// A mismatched start tag.
	MismatchedStartTag = -9,

	// A mismatched end tag.
	MismatchedEndTag = -10,
};

class AVM1XML : public AVM1XMLNode
{
private:
	// The document's XML declaration, if any.
	Optional<tiny_string> xmlDecl;

	// The document's `DOCTYPE`, if any.
	Optional<tiny_string> docType;

	// The last parse error encountered, if any.
	XMLStatus status;
public:
	AVM1XML(AVM1Activation& act);

	static GcPtr<AVM1SystemClass> createClass
	(
		AVM1DeclContext& ctx,
		const GcPtr<AVM1Object>& superProto
	);

	// Get the document's XML declaration.
	Optional<const tiny_string&> getXMLDecl() const { return xmlDecl; }

	// Get the first `DOCTYPE` node in the document.
	Optional<const tiny_string&> getDocType() const { return docType; }

	// Get the document's last parse error.
	const XMLStatus& getStatus() const { return status; }

	// Parse a string into the document, replacing the previous content.
	void parse
	(
		AVM1Activation& act,
		const tiny_string& str,
		bool ignoreWhite
	);

	void load
	(
		AVM1Activation& act,
		GcPtr<AVM1Object> loaderObj,
		const tiny_string& url,
		NullableGcPtr<AVM1XMLNode> sendObj = NullGc
	);

	AVM1_FUNCTION_DECL(ctor);

	AVM1_FUNCTION_TYPE_DECL(AVM1XML, createElement);
	AVM1_FUNCTION_TYPE_DECL(AVM1XML, createTextNode);
	AVM1_FUNCTION_TYPE_DECL(AVM1XML, parseXML);
	AVM1_FUNCTION_DECL(load);
	AVM1_FUNCTION_TYPE_DECL(AVM1XML, sendAndLoad);
	AVM1_FUNCTION_TYPE_DECL(AVM1XML, onData);
	AVM1_FUNCTION_TYPE_DECL(AVM1XML, getBytesLoaded);
	AVM1_FUNCTION_TYPE_DECL(AVM1XML, getBytesTotal);
	AVM1_GETTER_TYPE_DECL(AVM1XML, DocTypeDecl);
	AVM1_GETTER_TYPE_DECL(AVM1XML, Status);
	AVM1_GETTER_TYPE_DECL(AVM1XML, XMLDecl);
};

}
#endif /* SCRIPTING_AVM1_TOPLEVEL_XML_H */
