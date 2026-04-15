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

#include <initializer_list>

#include "gc/context.h"
#include "scripting/avm1/activation.h"
#include "scripting/avm1/function.h"
#include "scripting/avm1/object/object.h"
#include "scripting/avm1/prop.h"
#include "scripting/avm1/prop_decl.h"
#include "scripting/avm1/runtime.h"
#include "scripting/avm1/toplevel/XMLNode.h"
#include "tiny_string.h"
#include "utils/array.h"
#include "utils/optional.h"

using namespace lightspark;

// Based on Ruffle's `avm1::globals::xml_node` crate.

AVM1XMLNode::AVM1XMLNode
(
	AVM1Activation& act,
	GcPtr<AVM1Object> proto,
	const XMLNodeType& nodeType,
	const tiny_string& nodeValue
) :
AVM1Object(act, proto),
type(nodeType),
value(nodeValue),
attrubutes(NEW_GC_PTR(act.getGcCtx(), AVM1Object(act, NullGc)))
{
}

AVM1XMLNode::AVM1XMLNode
(
	AVM1Activation& act,
	const XMLNodeType& nodeType,
	const tiny_string& nodeValue
) : AVM1XMLNode
(
	act,
	act.getPrototypes().xmlNode->proto
	nodeType,
	nodeValue
)
{
}

AVM1XML::AVM1XML(AVM1Activation& act) : AVM1XMLNode
(
	act,
	act.getPrototypes().xml->proto
), status(XMLStatus::NoError)
{
}

Optional<const tiny_string&> AVM1XMLNode::getName() const
{
	if (type == XMLNodeType::Element)
		return value;
	return {};
}

Optional<tiny_string> AVM1XMLNode::getLocalName() const
{
	return getName().transform([](const auto& name)
	{
		auto pos = name.find(":");
		if (pos == tiny_string::npos || pos + 1 >= name.numChars())
			return name;
		return name.substr(pos + 1, tiny_string::npos);
	});
}

Optional<tiny_string> AVM1XMLNode::getPrefix() const
{
	return getName().transform([](const auto& name)
	{
		auto pos = name.find(":");
		if (pos == tiny_string::npos || pos + 1 >= name.numChars())
			return "";
		return name.substr(0, pos);
	});
}

Optional<const tiny_string&> AVM1XMLNode::getValue() const
{
	if (type != XMLNodeType::Element)
		return value;
	return {};
}

size_t AVM1XMLNode::getChildPos(GcPtr<AVM1XMLNode> child) const
{
	auto it = std::find(children.cbegin(), children.cend(), child);
	return
	(
		it != children.cend() ?
		std::distance(it, children.cend()) :
		-1
	);
}

bool AVM1XMLNode::hasChild(GcPtr<AVM1XMLNode> child) const
{
	return
	(
		!child->getParent().isNull() &&
		child->getParent() == this
	);
}

NullableGcPtr<AVM1XMLNode> AVM1XMLNode::getChild(size_t i) const
{
	if (i >= children.size())
		return NullGc;
	return children[i];
}

GcPtr<AVM1XMLNode> AVM1XMLNode::duplicate
(
	AVM1Activation& act,
	bool deep
) const
{
	auto ret = NEW_GC_PTR(act.getGcCtx(), AVM1XMLNode
	(
		act,
		type,
		value
	));

	if (!deep)
		return ret;

	for (size_t i = 0; i < children.size(); ++i)
		ret->insertChild(children[i]->duplicate(act, deep), i);
	return ret;
}

void AVM1XMLNode::orphanChild(GcPtr<AVM1XMLNode> child)
{
	auto it = std::find(children.begin(), children.end(), child);
	if (it != children.end())
		children.erase(it);
}

void AVM1XMLNode::insertChild(GcPtr<AVM1XMLNode> child, size_t pos)
{
	// Check if `child` is an ancestor of this node.
	for (auto node = this; node != nullptr; node = node->getParent())
	{
		// Bail, if `node` is `child`.
		if (node == child)
			return;
	}

	if (!child->getParent().isNull() && this != child->getParent())
		child->getParent()->orphanChild(child);

	child->setParent(this);
	children.insert(children.cbegin() + pos, child);

	AVM1XMLNodePtr prev;
	AVM1XMLNodePtr next;

	if (pos > 0 && pos - 1 < children.size())
	{
		prev = children[pos - 1];
		prev->nextSibling = this;
	}

	if (pos != -1 && pos + 1 < children.size())
	{
		next = children[pos + 1];
		next->prevSibling = this;
	}

	prevSibling = prev;
	nextSibling = next;
}

void AVM1XMLNode::removeNode()
{
	if (parent.isNull())
		return;

	auto it = std::find
	(
		parent->children.begin(),
		parent->children.end(),
		this
	);

	parent->children.erase(it);

	if (!prevSibling.isNull())
		prevSibling->nextSibling = nextSibling;
	if (!nextSibling.isNull())
		nextSibling->prevSibling = prevSibling;

	prevSibling = NullGc;
	nextSibling = NullGc;
}

Optional<AVM1Value> AVM1XMLNode::lookupNamespaceURI
(
	const tiny_string& prefix
) const
{
}

void AVM1XMLNode::writeToString
(
	AVM1Activation& act,
	tiny_string& str
) const
{
}

using AVM1XMLNode;

static constexpr auto protoDecls =
{
	AVM1_FUNCTION_TYPE_PROTO(AVM1XMLNode, cloneNode),
	AVM1_FUNCTION_TYPE_PROTO(AVM1XMLNode, removeNode),
	AVM1_FUNCTION_TYPE_PROTO(AVM1XMLNode, insertBefore),
	AVM1_FUNCTION_TYPE_PROTO(AVM1XMLNode, appendChild),
	AVM1_FUNCTION_TYPE_PROTO(AVM1XMLNode, hasChildNode),
	AVM1_FUNCTION_TYPE_PROTO(AVM1XMLNode, toString),
	AVM1_FUNCTION_TYPE_PROTO(AVM1XMLNode, getNamespaceForPrefix),
	AVM1_FUNCTION_TYPE_PROTO(AVM1XMLNode, getPrefixForNamespace),
	AVM1_PROPERTY_TYPE_PROTO(AVM1XMLNode, attributes, Attributes),
	AVM1_PROPERTY_TYPE_PROTO(AVM1XMLNode, childNodes, ChildNodes),
	AVM1_PROPERTY_TYPE_PROTO(AVM1XMLNode, lastChild, LastChild),
	AVM1_PROPERTY_TYPE_PROTO(AVM1XMLNode, nextSibling, NextSibling),
	AVM1_PROPERTY_TYPE_PROTO(AVM1XMLNode, nodeName, NodeName),
	AVM1_PROPERTY_TYPE_PROTO(AVM1XMLNode, nodeType, NodeType),
	AVM1_PROPERTY_TYPE_PROTO(AVM1XMLNode, nodeValue, NodeValue),
	AVM1_PROPERTY_TYPE_PROTO(AVM1XMLNode, parentNode, ParentNode),
	AVM1_PROPERTY_TYPE_PROTO(AVM1XMLNode, previousSibling, PreviousSibling),
	AVM1_PROPERTY_TYPE_PROTO(AVM1XMLNode, prefix, Prefix),
	AVM1_PROPERTY_TYPE_PROTO(AVM1XMLNode, localName, LocalName),
	AVM1_PROPERTY_TYPE_PROTO(AVM1XMLNode, namespaceURI, NamespaceURI)
};

GcPtr<AVM1SystemClass> AVM1XMLNode::createClass
(
	AVM1DeclContext& ctx,
	const GcPtr<AVM1Object>& superProto
)
{
	auto _class = ctx.makeClass(ctor, superProto);
	ctx.definePropsOn(_class->proto, protoDecls);
	return _class;
}

AVM1_FUNCTION_BODY(AVM1XMLNode, ctor)
{
}

AVM1_FUNCTION_TYPE_BODY(AVM1XMLNode, AVM1XMLNode, cloneNode)
{
}

AVM1_FUNCTION_TYPE_BODY(AVM1XMLNode, AVM1XMLNode, removeNode)
{
}

AVM1_FUNCTION_TYPE_BODY(AVM1XMLNode, AVM1XMLNode, insertBefore)
{
}

AVM1_FUNCTION_TYPE_BODY(AVM1XMLNode, AVM1XMLNode, appendChild)
{
}

AVM1_FUNCTION_TYPE_BODY(AVM1XMLNode, AVM1XMLNode, hasChildNode)
{
}

AVM1_FUNCTION_TYPE_BODY(AVM1XMLNode, AVM1XMLNode, toString)
{
}

AVM1_FUNCTION_TYPE_BODY(AVM1XMLNode, AVM1XMLNode, getNamespaceForPrefix)
{
}

AVM1_FUNCTION_TYPE_BODY(AVM1XMLNode, AVM1XMLNode, getPrefixForNamespace)
{
}

AVM1_GETTER_TYPE_BODY(AVM1XMLNode, AVM1XMLNode, Attributes)
{
}

AVM1_SETTER_TYPE_BODY(AVM1XMLNode, AVM1XMLNode, Attributes)
{
}

AVM1_GETTER_TYPE_BODY(AVM1XMLNode, AVM1XMLNode, ChildNodes)
{
}

AVM1_SETTER_TYPE_BODY(AVM1XMLNode, AVM1XMLNode, ChildNodes)
{
}

AVM1_GETTER_TYPE_BODY(AVM1XMLNode, AVM1XMLNode, LastChild)
{
}

AVM1_SETTER_TYPE_BODY(AVM1XMLNode, AVM1XMLNode, LastChild)
{
}

AVM1_GETTER_TYPE_BODY(AVM1XMLNode, AVM1XMLNode, NextSibling)
{
}

AVM1_SETTER_TYPE_BODY(AVM1XMLNode, AVM1XMLNode, NextSibling)
{
}

AVM1_GETTER_TYPE_BODY(AVM1XMLNode, AVM1XMLNode, NodeName)
{
}

AVM1_SETTER_TYPE_BODY(AVM1XMLNode, AVM1XMLNode, NodeName)
{
}

AVM1_GETTER_TYPE_BODY(AVM1XMLNode, AVM1XMLNode, NodeType)
{
}

AVM1_SETTER_TYPE_BODY(AVM1XMLNode, AVM1XMLNode, NodeType)
{
}

AVM1_GETTER_TYPE_BODY(AVM1XMLNode, AVM1XMLNode, NodeValue)
{
}

AVM1_SETTER_TYPE_BODY(AVM1XMLNode, AVM1XMLNode, NodeValue)
{
}

AVM1_GETTER_TYPE_BODY(AVM1XMLNode, AVM1XMLNode, ParentNode)
{
}

AVM1_SETTER_TYPE_BODY(AVM1XMLNode, AVM1XMLNode, ParentNode)
{
}

AVM1_GETTER_TYPE_BODY(AVM1XMLNode, AVM1XMLNode, PreviousSibling)
{
}

AVM1_SETTER_TYPE_BODY(AVM1XMLNode, AVM1XMLNode, PreviousSibling)
{
}

AVM1_GETTER_TYPE_BODY(AVM1XMLNode, AVM1XMLNode, Prefix)
{
}

AVM1_SETTER_TYPE_BODY(AVM1XMLNode, AVM1XMLNode, Prefix)
{
}

AVM1_GETTER_TYPE_BODY(AVM1XMLNode, AVM1XMLNode, LocalName)
{
}

AVM1_SETTER_TYPE_BODY(AVM1XMLNode, AVM1XMLNode, LocalName)
{
}

AVM1_GETTER_TYPE_BODY(AVM1XMLNode, AVM1XMLNode, NamespaceURI)
{
}

AVM1_SETTER_TYPE_BODY(AVM1XMLNode, AVM1XMLNode, NamespaceURI)
{
}
