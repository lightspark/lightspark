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

#ifndef SCRIPTING_AVM1_TOPLEVEL_XMLNODE_H
#define SCRIPTING_AVM1_TOPLEVEL_XMLNODE_H 1

#include "gc/ptr.h"
#include "scripting/avm1/function.h"
#include "scripting/avm1/object.h"
#include "utils/optional.h"
#include "tiny_string.h"

// Based on Ruffle's `avm1::globals::xml_node` crate.

namespace lightspark
{

class AVM1Activation;
class AVM1Context;
class AVM1DeclContext;
class AVM1SystemClass;
class AVM1Value;
class SystemState;

enum class XMLNodeType : uint8_t
{
	Unknown,
	Element,
	Attribute,
	Text,
	Cdata_section,
	Entity_reference,
	Entity,
	Processing_instruction,
	Comment,
	Document,
	Document_type,
	Document_fragment,
	Notation,
};

class AVM1XMLNode : public AVM1Object
{
private:
	using AVM1XMLNodePtr = NullableGcPtr<AVM1XMLNode>;

	// The parent of this node.
	NullableGcPtr<AVM1XMLNode> parent;

	// The previous sibling of this node.
	NullableGcPtr<AVM1XMLNode> prevSibling;

	// The next sibling of this node.
	NullableGcPtr<AVM1XMLNode> nextSibling;

	// The type of this node.
	// NOTE: This is usually either `ELEMENT_NODE`, or `TEXT_NODE`, but
	// other values are accepted as well.
	XMLNodeType type;

	// The value of this node. The meaning of the value depends on what
	// the node's `nodeType` is.
	// For `ELEMENT_NODE`, it's the element name.
	// For `TEXT_NODE`, it's the text content.
	tiny_string value;

	// The attributes of this node.
	GcPtr<AVM1Object> attributes;

	// The children of this node.
	std::vector<GcPtr<AVM1XMLNode>> children;

	// Remove a node from the child list.
	void orphanChild(GcPtr<AVM1XMLNode> child);

	// Write the contents of this node, and it's children to the supplied string.
	void writeToString(AVM1Activation& act, tiny_string& str) const;
public:
	AVM1XMLNode
	(
		AVM1Activation& act,
		GcPtr<AVM1Object> proto,
		const XMLNodeType& nodeType = XMLNodeType::Element,
		const tiny_string& nodeValue = ""
	);

	AVM1XMLNode
	(
		AVM1Activation& act,
		const XMLNodeType& nodeType = XMLNodeType::Element,
		const tiny_string& nodeValue = ""
	);

	static GcPtr<AVM1SystemClass> createClass
	(
		AVM1DeclContext& ctx,
		const GcPtr<AVM1Object>& superProto
	);


	// Get the parent, if we have one.
	AVM1XMLNodePtr getParent() const { return parent; }

	// Set the node's parent.
	void setParent(AVM1XMLNodePtr _parent) { parent = _parent; }

	// Get the previous sibling, if we have one.
	AVM1XMLNodePtr getPrevSibling() const { return prevSibling; }

	// Set the node's previous sibling.
	void setPrevSibling(AVM1XMLNodePtr node) { prevSibling = node; }

	// Get the next sibling, if we have one.
	AVM1XMLNodePtr getNextSibling() const { return nextSibling; }

	// Set the node's next sibling.
	void setNextSibling(AVM1XMLNodePtr node) { nextSibling = node; }

	// Get the node's type.
	const XMLNodeType& getType() const { return type; }

	// Get the node's name, if it has one.
	Optional<const tiny_string&> getName() const;

	// Get the node's local name.
	Optional<tiny_string> getLocalName() const;

	// Get the node's prefix.
	Optional<tiny_string> getPrefix() const;

	// Get the node's value, if it has one.
	Optional<const tiny_string&> getValue() const;

	// Set the node's value.
	void setValue(const tiny_string& _value) { value = _value; }

	// Get the position of a child node.
	//
	// NOTE: This returns `-1` if the node can't accept children, or the
	// child node isn't a child of this node.
	size_t getChildPos(GcPtr<AVM1XMLNode> child) const;

	// Checks if `child` is a direct decendant of this node.
	bool hasChild(GcPtr<AVM1XMLNode> child) const;

	// Get a given child, by index.
	AVM1XMLNodePtr getChild(size_t i) const;

	// Get the node's children.
	const std::vector<GcPtr<AVM1XMLNode>>& getChildren() const
	{
		return children;
	}

	// Get the node's attributes.
	GcPtr<AVM1Object> getAttributes() const { return attributes; }

	// Create a copy of this node.
	//
	// NOTE: If `deep` is `true`, the entire node tree will be copied.
	GcPtr<AVM1XMLNode> duplicate(AVM1Activation& act, bool deep = false) const;

	// Inserts `child` into this node's children list.
	//
	// NOTE: Because `child` will be inserted into the current tree, all
	// child references to other nodes, or documents will be adjusted to
	// reflect it's new position in the tree. This might remove it from
	// any existing trees, or documents.
	//
	// `pos` is the position of `child` in this node's children list.
	// This is used to find, and link `child`'s siblings to each other.
	void insertChild(GcPtr<AVM1XMLNode> child, size_t pos);

	// Append a child node to the end of the node's child list.
	void appendChild(GcPtr<AVM1XMLNode> child)
	{
		insertChild(child, children.size());
	}

	// Remove this node from it's parent.
	void removeNode();

	// Get the URI of the given prefix.
	//
	// XML namespaces are determined by the `xmlns` namespace attributes
	// on either this node, or it's parent.
	Optional<AVM1Value> lookupNamespaceURI(const tiny_string& prefix) const;

	// Convert the node to a string of UTF-8 encoded XML.
	tiny_string toString(AVM1Activation& act) const
	{
		tiny_string ret;
		writeToString(act, ret);
		return ret;
	}

	AVM1_FUNCTION_DECL(ctor);

	AVM1_FUNCTION_TYPE_DECL(AVM1XMLNode, cloneNode);
	AVM1_FUNCTION_TYPE_DECL(AVM1XMLNode, removeNode);
	AVM1_FUNCTION_TYPE_DECL(AVM1XMLNode, insertBefore);
	AVM1_FUNCTION_TYPE_DECL(AVM1XMLNode, appendChild);
	AVM1_FUNCTION_TYPE_DECL(AVM1XMLNode, hasChildNodes);
	AVM1_FUNCTION_TYPE_DECL(AVM1XMLNode, toString);
	AVM1_FUNCTION_TYPE_DECL(AVM1XMLNode, getNamespaceForPrefix);
	AVM1_FUNCTION_TYPE_DECL(AVM1XMLNode, getPrefixForNamespace);
	AVM1_GETTER_TYPE_DECL(AVM1XMLNode, Attributes);
	AVM1_GETTER_TYPE_DECL(AVM1XMLNode, ChildNodes);
	AVM1_GETTER_TYPE_DECL(AVM1XMLNode, LastChild);
	AVM1_GETTER_TYPE_DECL(AVM1XMLNode, NextSibling);
	AVM1_PROPERTY_TYPE_DECL(AVM1XMLNode, NodeName);
	AVM1_GETTER_TYPE_DECL(AVM1XMLNode, NodeType);
	AVM1_PROPERTY_TYPE_DECL(AVM1XMLNode, NodeValue);
	AVM1_GETTER_TYPE_DECL(AVM1XMLNode, ParentNode);
	AVM1_GETTER_TYPE_DECL(AVM1XMLNode, PreviousSibling);
	AVM1_GETTER_TYPE_DECL(AVM1XMLNode, Prefix);
	AVM1_GETTER_TYPE_DECL(AVM1XMLNode, LocalName);
	AVM1_GETTER_TYPE_DECL(AVM1XMLNode, NamespaceURI);
};

}
#endif /* SCRIPTING_AVM1_TOPLEVEL_XMLNODE_H */
