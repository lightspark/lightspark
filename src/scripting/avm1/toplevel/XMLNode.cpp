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
#include <sstream>

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
	for (auto node = this; node != nullptr; node = node->getParent())
	{
		// Iterate through `attributes` in definition order, meaning
		// the first matching URI is returned.
		auto attrProps = node->attributes->getOwnProps();
		for (const auto& prop : attrProps)
		{
			#if 1
			if (!prop.first.startsWith("xmlns"))
				continue;
			auto ns = prop.first.stripPrefix("xmlns:");
			#else
			auto ns = prop.first.tryStripPrefix("xmlns");
			if (!ns.hasValue())
				continue;
			ns = ns->tryStripPrefix(':');
			#endif
			// NOTE: An empty prefix matches every attribute that starts
			// with `xmlns` (with, or without a colon).
			if (prefix.empty() || ns == prefix)
				return prop.second;
		}
	}
	return {};
}

void AVM1XMLNode::writeToString
(
	AVM1Activation& act,
	tiny_string& str
) const
{
	auto escape = [](const tiny_string& str)
	{
		std::stringstream s;
		for (auto ch : str)
		{
			switch (ch)
			{
				default: s << tiny_string::fromChar(ch); break;
				case '<': s << "&lt;"; break;
				case '>': s << "&gt;"; break;
				case '&': s << "&amp;"; break;
				case '\'': s << "&apos;"; break;
				case '"': s << "&quot;"; break;
			}
		}
		return s.str();
	};

	if (type != XMLNodeType::Element)
	{
		str += escape(value);
		return;
	}

	if (value.empty())
	{
		for (auto child : children)
			child->writeToString(act, str);
		return;
	}

	std::ostringstream s("<", std::ios::ate);
	s << value;

	auto attrProps = attributes->getOwnProps();
	for (const auto& prop : attrProps)
	{
		s << ' ' << prop.first << "=\"" <<
		escape(prop.second.toString(act)) << '"';
	}

	if (children.empty())
	{
		str += (s << " />").str();
		return;
	}

	str += (s << '>').str();
	for (auto child : children)
		child->writeToString(act, str);
	s.str(str);
	str = (s << "</" << value << ">").str();
}

using AVM1XMLNode;

static constexpr auto protoDecls =
{
	AVM1_FUNCTION_TYPE_PROTO(AVM1XMLNode, cloneNode),
	AVM1_FUNCTION_TYPE_PROTO(AVM1XMLNode, removeNode),
	AVM1_FUNCTION_TYPE_PROTO(AVM1XMLNode, insertBefore),
	AVM1_FUNCTION_TYPE_PROTO(AVM1XMLNode, appendChild),
	AVM1_FUNCTION_TYPE_PROTO(AVM1XMLNode, hasChildNodes),
	AVM1_FUNCTION_TYPE_PROTO(AVM1XMLNode, toString),
	AVM1_FUNCTION_TYPE_PROTO(AVM1XMLNode, getNamespaceForPrefix),
	AVM1_FUNCTION_TYPE_PROTO(AVM1XMLNode, getPrefixForNamespace),
	AVM1_GETTER_TYPE_PROTO(AVM1XMLNode, attributes, Attributes),
	AVM1_GETTER_TYPE_PROTO(AVM1XMLNode, childNodes, ChildNodes),
	AVM1_GETTER_TYPE_PROTO(AVM1XMLNode, lastChild, LastChild),
	AVM1_GETTER_TYPE_PROTO(AVM1XMLNode, nextSibling, NextSibling),
	AVM1_PROPERTY_TYPE_PROTO(AVM1XMLNode, nodeName, NodeName),
	AVM1_GETTER_TYPE_PROTO(AVM1XMLNode, nodeType, NodeType),
	AVM1_PROPERTY_TYPE_PROTO(AVM1XMLNode, nodeValue, NodeValue),
	AVM1_GETTER_TYPE_PROTO(AVM1XMLNode, parentNode, ParentNode),
	AVM1_GETTER_TYPE_PROTO(AVM1XMLNode, previousSibling, PreviousSibling),
	AVM1_GETTER_TYPE_PROTO(AVM1XMLNode, prefix, Prefix),
	AVM1_GETTER_TYPE_PROTO(AVM1XMLNode, localName, LocalName),
	AVM1_GETTER_TYPE_PROTO(AVM1XMLNode, namespaceURI, NamespaceURI)
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
	XMLNodeType nodeType;
	tiny_string nodeValue;
	AVM1_ARG_UNPACK(nodeType, XMLNodeType::Text)(nodeValue);

	INPLACE_NEW_GC_PTR(act.getGcCtx(), _this, AVM1XMLNode
	(
		nodeType,
		nodeValue
	));

	return _this;
}

AVM1_FUNCTION_TYPE_BODY(AVM1XMLNode, AVM1XMLNode, cloneNode)
{
	bool deep;
	AVM1_ARG_UNPACK(deep, false);

	return _this->duplicate(act, deep);
}

AVM1_FUNCTION_TYPE_BODY(AVM1XMLNode, AVM1XMLNode, removeNode)
{
	_this->removeNode();
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_TYPE_BODY(AVM1XMLNode, AVM1XMLNode, insertBefore)
{
	AVM1XMLNodePtr child;
	AVM1XMLNodePtr insertPoint;
	AVM1_ARG_CHECK(AVM1_ARG_UNPACK(child)(insertPoint));

	if (_this->hasChild(child))
		return AVM1Value::undefinedVal;

	auto pos = _this->getChildPos(insertPoint);
	if (pos == -1)
		return AVM1Value::undefinedVal;

	_this->insertChild(pos)
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_TYPE_BODY(AVM1XMLNode, AVM1XMLNode, appendChild)
{
	AVM1XMLNodePtr child;
	AVM1_ARG_CHECK(AVM1_ARG_UNPACK(child));

	if (_this->hasChild(child))
		return AVM1Value::undefinedVal;

	_this->insertChild(child, _this->children.size());
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_TYPE_BODY(AVM1XMLNode, AVM1XMLNode, hasChildNodes)
{
	return !_this->getChildren().empty();
}

AVM1_FUNCTION_TYPE_BODY(AVM1XMLNode, AVM1XMLNode, toString)
{
	return _this->toString(act);
}

AVM1_FUNCTION_TYPE_BODY(AVM1XMLNode, AVM1XMLNode, getNamespaceForPrefix)
{
	tiny_string prefix;
	AVM1_ARG_CHECK(AVM1_ARG_UNPACK(prefix));

	return _this->lookupNamespaceURI
	(
		prefix
	).valueOr(AVM1Value::nullVal);
}

AVM1_FUNCTION_TYPE_BODY(AVM1XMLNode, AVM1XMLNode, getPrefixForNamespace)
{
	tiny_string uri;
	AVM1_ARG_CHECK(AVM1_ARG_UNPACK(uri));

	for (auto node = _this; !node.isNull(); node = node->getParent())
	{
		// Iterate through `attributes` in definition order, meaning
		// the first matching URI is returned.
		auto attrProps = node->getAttributes()->getOwnProps();
		for (const auto& prop : attrProps)
		{
			auto value = prop.second.toString(act);
			if (value != uri || !prop.first.startsWith("xmlns"))
				continue;
			auto ns = prop.first.stripPrefix("xmlns");
			if (!ns.startsWith(':'))
				return "";
			return ns.substr(1, tiny_string::npos);
		}
	}
	return AVM1Value::nullVal;
}

AVM1_GETTER_TYPE_BODY(AVM1XMLNode, AVM1XMLNode, Attributes)
{
	return _this->getAttributes();
}

AVM1_GETTER_TYPE_BODY(AVM1XMLNode, AVM1XMLNode, ChildNodes)
{
	std::vector<AVM1Value> children;
	children.reserve(_this->children.size());

	for (auto child : _this->getChildren())
		children.emplace_back(child);

	return NEW_GC_PTR(act.getGcCtx(), AVM1Array(act, children));
}

AVM1_GETTER_TYPE_BODY(AVM1XMLNode, AVM1XMLNode, LastChild)
{
	if (_this->getChildren().empty())
		return AVM1Value::nullVal;
	return _this->getChildren().back();
}

AVM1_GETTER_TYPE_BODY(AVM1XMLNode, AVM1XMLNode, NextSibling)
{
	if (_this->getNextSibling().isNull())
		return AVM1Value::nullVal;
	return _this->getNextSibling();
}

AVM1_GETTER_TYPE_BODY(AVM1XMLNode, AVM1XMLNode, NodeName)
{
	return _this->getName().transformOr
	(
		AVM1Value::nullVal,
		[&](const auto& name) { return name; }
	);
}

// NOTE: `node{Name,Value}`'s setters set the same value.
AVM1_SETTER_TYPE_BODY(AVM1XMLNode, AVM1XMLNode, NodeName)
{
	if (!value.is<UndefValue>())
		_this->setValue(value.toString(act));
}

AVM1_GETTER_TYPE_BODY(AVM1XMLNode, AVM1XMLNode, NodeType)
{
	return number_t(_this->getNodeType());
}

AVM1_GETTER_TYPE_BODY(AVM1XMLNode, AVM1XMLNode, NodeValue)
{
	return _this->getValue().transformOr
	(
		AVM1Value::nullVal,
		[&](const auto& value) { return value; }
	);
}

// NOTE: `node{Name,Value}`'s setters set the same value.
AVM1_SETTER_TYPE_BODY(AVM1XMLNode, AVM1XMLNode, NodeValue)
{
	if (!value.is<UndefValue>())
		_this->setValue(value.toString(act));
}

AVM1_GETTER_TYPE_BODY(AVM1XMLNode, AVM1XMLNode, ParentNode)
{
	if (_this->getParent().isNull())
		return AVM1Value::nullVal;
	return _this->getParent();
}

AVM1_GETTER_TYPE_BODY(AVM1XMLNode, AVM1XMLNode, PreviousSibling)
{
	if (_this->getPrevSibling().isNull())
		return AVM1Value::nullVal;
	return _this->getPrevSibling();
}

AVM1_GETTER_TYPE_BODY(AVM1XMLNode, AVM1XMLNode, Prefix)
{
	return _this->getPrefix().transformOr
	(
		AVM1Value::nullVal,
		[&](const auto& prefix) { return prefix; }
	);
}

AVM1_GETTER_TYPE_BODY(AVM1XMLNode, AVM1XMLNode, LocalName)
{
	return _this->getLocalName().transformOr
	(
		AVM1Value::nullVal,
		[&](const auto& localName) { return localName; }
	);
}

AVM1_GETTER_TYPE_BODY(AVM1XMLNode, AVM1XMLNode, NamespaceURI)
{
	return _this->getPrefix().transformOr
	(
		AVM1Value::nullVal,
		[&](const auto& prefix)
		{
			return _this->lookupNamespaceURI(prefix).valueOr("");
		}
	);
}
