/*
 *   Copyright (c) 2008-2022 SLIBIO <https://github.com/SLIBIO>
 *
 *   Permission is hereby granted, free of charge, to any person obtaining a copy
 *   of this software and associated documentation files (the "Software"), to deal
 *   in the Software without restriction, including without limitation the rights
 *   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *   copies of the Software, and to permit persons to whom the Software is
 *   furnished to do so, subject to the following conditions:
 *
 *   The above copyright notice and this permission notice shall be included in
 *   all copies or substantial portions of the Software.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *   THE SOFTWARE.
 */

#include "slib/data/xml.h"

#include "slib/io/file.h"
#include "slib/core/string_buffer.h"
#include "slib/core/stringx.h"
#include "slib/core/log.h"

namespace slib
{

	SLIB_DEFINE_ROOT_OBJECT(XmlNode)

	XmlNode::XmlNode(XmlNodeType type): m_type(type), m_positionStartInSource(0), m_positionEndInSource(0), m_lineInSource(1), m_columnInSource(1)
	{
	}

	XmlNode::~XmlNode()
	{
	}

	XmlNodeType XmlNode::getType() const
	{
		return m_type;
	}

	String XmlNode::getText() const
	{
		StringBuffer buf;
		if (buildText(buf)) {
			return buf.merge();
		}
		return sl_null;
	}

	String XmlNode::toString() const
	{
		StringBuffer buf;
		if (buildXml(buf)) {
			return buf.merge();
		}
		return sl_null;
	}

	sl_bool XmlNode::isDocumentNode() const
	{
		return m_type == XmlNodeType::Document;
	}

	Ref<XmlDocument> XmlNode::toDocumentNode() const
	{
		if (m_type == XmlNodeType::Document) {
			return (XmlDocument*)this;
		}
		return sl_null;
	}

	sl_bool XmlNode::isElementNode() const
	{
		return m_type == XmlNodeType::Element;
	}

	Ref<XmlElement> XmlNode::toElementNode() const
	{
		if (m_type == XmlNodeType::Element) {
			return (XmlElement*)this;
		}
		return sl_null;
	}

	sl_bool XmlNode::isTextNode() const
	{
		return m_type == XmlNodeType::Text;
	}

	Ref<XmlText> XmlNode::toTextNode() const
	{
		if (m_type == XmlNodeType::Text) {
			return (XmlText*)this;
		}
		return sl_null;
	}

	sl_bool XmlNode::isProcessingInstructionNode() const
	{
		return m_type == XmlNodeType::ProcessingInstruction;
	}

	Ref<XmlProcessingInstruction> XmlNode::toProcessingInstructionNode() const
	{
		if (m_type == XmlNodeType::ProcessingInstruction) {
			return (XmlProcessingInstruction*)this;
		}
		return sl_null;
	}

	sl_bool XmlNode::isCommentNode() const
	{
		return m_type == XmlNodeType::Comment;
	}

	Ref<XmlComment> XmlNode::toCommentNode() const
	{
		if (m_type == XmlNodeType::ProcessingInstruction) {
			return (XmlComment*)this;
		}
		return sl_null;
	}

	Ref<XmlDocument> XmlNode::getDocument() const
	{
		if (m_type == XmlNodeType::Document) {
			return (XmlDocument*)(this);
		} else {
			return m_document;
		}
		return sl_null;
	}

	Ref<XmlElement> XmlNode::getRoot() const
	{
		if (m_type == XmlNodeType::Document) {
			XmlDocument* doc = (XmlDocument*)this;
			return doc->getFirstChildElement();
		} else {
			Ref<XmlDocument> doc = m_document;
			if (doc.isNotNull()) {
				return doc->getFirstChildElement();
			}
		}
		return sl_null;
	}

	Ref<XmlNodeGroup> XmlNode::getParent() const
	{
		return m_parent;
	}

	void XmlNode::setParent(const Ref<XmlNodeGroup>& parent)
	{
		m_parent = parent;
	}

	Ref<XmlElement> XmlNode::getParentElement() const
	{
		Ref<XmlNodeGroup> parent = m_parent;
		if (parent.isNotNull()) {
			return parent->toElementNode();
		}
		return sl_null;
	}

	const String& XmlNode::getSourceFilePath() const
	{
		return m_sourceFilePath;
	}

	void XmlNode::setSourceFilePath(const String& path)
	{
		m_sourceFilePath = path;
	}

	sl_size XmlNode::getStartPositionInSource() const
	{
		return m_positionStartInSource;
	}

	void XmlNode::setStartPositionInSource(sl_size pos)
	{
		m_positionStartInSource = pos;
	}

	sl_size XmlNode::getEndPositionInSource() const
	{
		return m_positionEndInSource;
	}

	void XmlNode::setEndPositionInSource(sl_size pos)
	{
		m_positionEndInSource = pos;
	}

	sl_size XmlNode::getLineNumberInSource() const
	{
		return m_lineInSource;
	}

	void XmlNode::setLineNumberInSource(sl_size line)
	{
		m_lineInSource = line;
	}

	sl_size XmlNode::getColumnNumberInSource() const
	{
		return m_columnInSource;
	}

	void XmlNode::setColumnNumberInSource(sl_size line)
	{
		m_columnInSource = line;
	}

	void XmlNode::_setDocument(const Ref<XmlDocument>& documentNew)
	{
		if (m_type == XmlNodeType::Document) {
			return;
		}
		if (m_type == XmlNodeType::Element) {
			XmlElement* e = (XmlElement*)this;
			m_document = documentNew;
			ListElements< Ref<XmlNode> > nodes(e->m_children);
			for (sl_size i = 0; i < nodes.count; i++) {
				nodes[i]->_setDocument(documentNew);
			}
		} else {
			m_document = documentNew;
		}
	}


	SLIB_DEFINE_OBJECT(XmlNodeGroup, XmlNode)

	XmlNodeGroup::XmlNodeGroup(XmlNodeType type): XmlNode(type)
	{
	}

	XmlNodeGroup::~XmlNodeGroup()
	{
	}

	sl_bool XmlNodeGroup::buildText(StringBuffer& output) const
	{
		ListElements< Ref<XmlNode> > children(m_children);
		for (sl_size i = 0; i < children.count; i++) {
			if (children[i].isNotNull()) {
				if (!(children[i]->buildText(output))) {
					return sl_false;
				}
			}
		}
		return sl_true;
	}

	sl_bool XmlNodeGroup::buildInnerXml(StringBuffer& output) const
	{
		ListElements< Ref<XmlNode> > children(m_children);
		for (sl_size i = 0; i < children.count; i++) {
			if (children[i].isNotNull()) {
				if (!(children[i]->buildXml(output))) {
					return sl_false;
				}
			}
		}
		return sl_true;
	}

	String XmlNodeGroup::getInnerXml() const
	{
		StringBuffer buf;
		if (buildInnerXml(buf)) {
			return buf.merge();
		}
		return sl_null;
	}

	sl_size XmlNodeGroup::getChildCount() const
	{
		return m_children.getCount();
	}

	Ref<XmlNode> XmlNodeGroup::getChild(sl_size index) const
	{
		return m_children.getValueAt_NoLock(index);
	}

	sl_bool XmlNodeGroup::addChild(const Ref<XmlNode>& node)
	{
		if (node.isNotNull() && node->getType() != XmlNodeType::Document) {
			node->m_parent = this;
			node->_setDocument(getDocument());
			return m_children.add_NoLock(node);
		}
		return sl_false;
	}

	sl_bool XmlNodeGroup::removeChild(const Ref<XmlNode>& node, sl_bool flagUnregisterDocument)
	{
		if (node.isNotNull()) {
			sl_reg index = m_children.indexOf_NoLock(node);
			if (index >= 0) {
				if (flagUnregisterDocument) {
					node->_setDocument(Ref<XmlDocument>::null());
				}
				node->m_parent.setNull();
				return m_children.removeAt_NoLock(index);
			}
		}
		return sl_false;
	}

	void XmlNodeGroup::removeAllChildren(sl_bool flagUnregisterDocument)
	{
		ListElements< Ref<XmlNode> > nodes(m_children);
		for (sl_size i = 0; i < nodes.count; i++) {
			if (flagUnregisterDocument) {
				nodes[i]->_setDocument(Ref<XmlDocument>::null());
			}
			nodes[i]->m_parent.setNull();
		}
		m_children.removeAll_NoLock();
	}

	String XmlNodeGroup::getChildText(sl_size index) const
	{
		Ref<XmlNode> node = m_children.getValueAt_NoLock(index);
		if (node.isNotNull()) {
			return node->getText();
		}
		return sl_null;
	}

	Ref<XmlElement> XmlNodeGroup::getChildElement(sl_size index) const
	{
		Ref<XmlNode> node = m_children.getValueAt_NoLock(index);
		if (node.isNotNull()) {
			return node->toElementNode();
		}
		return sl_null;
	}

	List< Ref<XmlElement> > XmlNodeGroup::getChildElements() const
	{
		List< Ref<XmlElement> > ret;
		ListElements< Ref<XmlNode> > nodes(m_children);
		for (sl_size i = 0; i < nodes.count; i++) {
			Ref<XmlElement> e = nodes[i]->toElementNode();
			if (e.isNotNull()) {
				ret.add_NoLock(e);
			}
		}
		return ret;
	}

	sl_size XmlNodeGroup::getChildElementCount() const
	{
		sl_size n = 0;
		ListElements< Ref<XmlNode> > nodes(m_children);
		for (sl_size i = 0; i < nodes.count; i++) {
			Ref<XmlElement> e = nodes[i]->toElementNode();
			if (e.isNotNull()) {
				n++;
			}
		}
		return n;
	}

	List< Ref<XmlElement> > XmlNodeGroup::getChildElements(const StringView& tagName) const
	{
		List< Ref<XmlElement> > ret;
		ListElements< Ref<XmlNode> > nodes(m_children);
		for (sl_size i = 0; i < nodes.count; i++) {
			Ref<XmlElement> e = nodes[i]->toElementNode();
			if (e.isNotNull()) {
				if (e->m_name == tagName) {
					ret.add_NoLock(e);
				}
			}
		}
		return ret;
	}

	List< Ref<XmlElement> > XmlNodeGroup::getChildElements(const StringView& uri, const StringView& localName) const
	{
		List< Ref<XmlElement> > ret;
		ListElements< Ref<XmlNode> > nodes(m_children);
		for (sl_size i = 0; i < nodes.count; i++) {
			Ref<XmlElement> e = nodes[i]->toElementNode();
			if (e.isNotNull()) {
				if (e->m_uri == uri && e->m_localName == localName) {
					ret.add_NoLock(e);
				}
			}
		}
		return ret;
	}

	Ref<XmlElement> XmlNodeGroup::getFirstChildElement() const
	{
		ListElements< Ref<XmlNode> > nodes(m_children);
		for (sl_size i = 0; i < nodes.count; i++) {
			Ref<XmlElement> e = nodes[i]->toElementNode();
			if (e.isNotNull()) {
				return e;
			}
		}
		return sl_null;
	}

	Ref<XmlElement> XmlNodeGroup::getFirstChildElement(const StringView& tagName) const
	{
		if (tagName.isEmpty()) {
			return getFirstChildElement();
		}
		ListElements< Ref<XmlNode> > nodes(m_children);
		for (sl_size i = 0; i < nodes.count; i++) {
			Ref<XmlElement> e = nodes[i]->toElementNode();
			if (e.isNotNull()) {
				if (e->m_name == tagName) {
					return e;
				}
			}
		}
		return sl_null;
	}

	Ref<XmlElement> XmlNodeGroup::getFirstChildElement(const StringView& uri, const StringView& localName) const
	{
		ListElements< Ref<XmlNode> > nodes(m_children);
		for (sl_size i = 0; i < nodes.count; i++) {
			Ref<XmlElement> e = nodes[i]->toElementNode();
			if (e.isNotNull()) {
				if (e->m_uri == uri && e->m_localName == localName) {
					return e;
				}
			}
		}
		return sl_null;
	}

	String XmlNodeGroup::getFirstChildElementText() const
	{
		Ref<XmlElement> e = getFirstChildElement();
		if (e.isNotNull()) {
			return e->getText();
		}
		return sl_null;
	}

	String XmlNodeGroup::getFirstChildElementText(const StringView& tagName) const
	{
		if (tagName.isEmpty()) {
			return getFirstChildElementText();
		}
		Ref<XmlElement> e = getFirstChildElement(tagName);
		if (e.isNotNull()) {
			return e->getText();
		}
		return sl_null;
	}

	String XmlNodeGroup::getFirstChildElementText(const StringView& uri, const StringView& localName) const
	{
		Ref<XmlElement> e = getFirstChildElement(uri, localName);
		if (e.isNotNull()) {
			return e->getText();
		}
		return sl_null;
	}

	List< Ref<XmlElement> > XmlNodeGroup::getDescendantElements(const StringView& tagName) const
	{
		List< Ref<XmlElement> > ret;
		getDescendantElements(tagName, ret);
		return ret;
	}

	void XmlNodeGroup::getDescendantElements(const StringView& tagName, List< Ref<XmlElement> >& list) const
	{
		ListElements< Ref<XmlNode> > nodes(m_children);
		for (sl_size i = 0; i < nodes.count; i++) {
			Ref<XmlElement> e = nodes[i]->toElementNode();
			if (e.isNotNull()) {
				if (e->m_name == tagName) {
					list.add_NoLock(e);
				}
				e->getDescendantElements(tagName, list);
			}
		}
	}

	List< Ref<XmlElement> > XmlNodeGroup::getDescendantElements(const StringView& uri, const StringView& localName) const
	{
		List< Ref<XmlElement> > ret;
		getDescendantElements(uri, localName, ret);
		return ret;
	}

	void XmlNodeGroup::getDescendantElements(const StringView& uri, const StringView& localName, List< Ref<XmlElement> >& list) const
	{
		ListElements< Ref<XmlNode> > nodes(m_children);
		for (sl_size i = 0; i < nodes.count; i++) {
			Ref<XmlElement> e = nodes[i]->toElementNode();
			if (e.isNotNull()) {
				if (e->m_uri == uri && e->m_localName == localName) {
					list.add_NoLock(e);
				}
				e->getDescendantElements(uri, localName, list);
			}
		}
	}

	Ref<XmlElement> XmlNodeGroup::getFirstDescendantElement(const StringView& tagName) const
	{
		ListElements< Ref<XmlNode> > nodes(m_children);
		for (sl_size i = 0; i < nodes.count; i++) {
			Ref<XmlElement> e = nodes[i]->toElementNode();
			if (e.isNotNull()) {
				if (e->m_name == tagName) {
					return e;
				} else {
					e = e->getFirstDescendantElement(tagName);
					if (e.isNotNull()) {
						return e;
					}
				}
			}
		}
		return sl_null;
	}

	Ref<XmlElement> XmlNodeGroup::getFirstDescendantElement(const StringView& uri, const StringView& localName) const
	{
		ListElements< Ref<XmlNode> > nodes(m_children);
		for (sl_size i = 0; i < nodes.count; i++) {
			Ref<XmlElement> e = nodes[i]->toElementNode();
			if (e.isNotNull()) {
				if (e->m_uri == uri && e->m_localName == localName) {
					return e;
				} else {
					e = e->getFirstDescendantElement(uri, localName);
					if (e.isNotNull()) {
						return e;
					}
				}
			}
		}
		return sl_null;
	}

	String XmlNodeGroup::getFirstDescendantElementText(const StringView& tagName) const
	{
		Ref<XmlElement> e = getFirstDescendantElement(tagName);
		if (e.isNotNull()) {
			return e->getText();
		}
		return sl_null;
	}

	String XmlNodeGroup::getFirstDescendantElementText(const StringView& uri, const StringView& localName) const
	{
		Ref<XmlElement> e = getFirstDescendantElement(uri, localName);
		if (e.isNotNull()) {
			return e->getText();
		}
		return sl_null;
	}

	Ref<XmlElement> XmlNodeGroup::findElement(const StringView& attrName, const StringView& attrValue) const
	{
		ListElements< Ref<XmlNode> > nodes(m_children);
		for (sl_size i = 0; i < nodes.count; i++) {
			Ref<XmlElement> e = nodes[i]->toElementNode();
			if (e.isNotNull()) {
				String value = e->getAttribute(attrName);
				if (value == attrValue) {
					return e;
				}
				Ref<XmlElement> found = e->findElement(attrName, attrValue);
				if (found.isNotNull()) {
					return found;
				}
			}
		}
		return sl_null;
	}

	Ref<XmlElement> XmlNodeGroup::getElementById(const StringView& id) const
	{
		return findElement(StringView::literal("id"), id);
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(XmlAttribute)

	XmlAttribute::XmlAttribute() noexcept
	{
	}


	SLIB_DEFINE_OBJECT(XmlElement, XmlNodeGroup)

	XmlElement::XmlElement(): XmlNodeGroup(XmlNodeType::Element)
	{
		m_positionStartContentInSource = 0;
		m_positionEndContentInSource = 0;
	}

	XmlElement::~XmlElement()
	{
	}

	Ref<XmlElement> XmlElement::create(const String& name)
	{
		if (Xml::checkName(name)) {
			Ref<XmlElement> ret = new XmlElement;
			if (ret.isNotNull()) {
				ret->m_name = name;
				return ret;
			}
		}
		return sl_null;
	}

	Ref<XmlElement> XmlElement::create(const String& name, const String& uri, const String& localName)
	{
		if (Xml::checkName(name)) {
			Ref<XmlElement> ret = new XmlElement;
			if (ret.isNotNull()) {
				ret->m_name = name;
				ret->m_uri = uri;
				ret->m_localName = localName;
				return ret;
			}
		}
		return sl_null;
	}

	sl_bool XmlElement::buildXml(StringBuffer& output) const
	{
		String name = m_name;
		if (name.isEmpty()) {
			return sl_false;
		}
		if (!(output.addStatic("<"))) {
			return sl_false;
		}
		if (!(output.add(name))) {
			return sl_false;
		}
		// attributes
		{
			ListElements<XmlAttribute> attrs(m_attributes);
			for (sl_size i = 0; i < attrs.count; i++) {
				if (attrs[i].whiteSpacesBeforeName.isEmpty()) {
					if (!(output.addStatic(" "))) {
						return sl_false;
					}
				} else {
					if (!(output.add(attrs[i].whiteSpacesBeforeName))) {
						return sl_false;
					}
				}
				if (!(output.add(attrs[i].name))) {
					return sl_false;
				}
				if (!(output.addStatic("=\""))) {
					return sl_false;
				}
				if (!(Xml::encodeTextToEntities(attrs[i].value, output))) {
					return sl_false;
				}
				if (!(output.addStatic("\""))) {
					return sl_false;
				}
			}
		}
		// children
		{
			if (m_children.isEmpty()) {
				if (!(output.addStatic(" />"))) {
					return sl_false;
				}
			} else {
				if (!(output.addStatic(">"))) {
					return sl_false;
				}
				if (!(buildInnerXml(output))) {
					return sl_false;
				}
				if (!(output.addStatic("</"))) {
					return sl_false;
				}
				if (!(output.add(name))) {
					return sl_false;
				}
				if (!(output.addStatic(">"))) {
					return sl_false;
				}
			}
		}
		return sl_true;
	}

	const String& XmlElement::getName() const
	{
		return m_name;
	}

	const String& XmlElement::getUri() const
	{
		return m_uri;
	}

	const String& XmlElement::getNamespace() const
	{
		return m_namespace;
	}

	const String& XmlElement::getLocalName() const
	{
		return m_localName;
	}

	sl_bool XmlElement::setName(const String& name)
	{
		if (Xml::checkName(name)) {
			m_name = name;
			return sl_true;
		}
		return sl_false;
	}

	sl_bool XmlElement::setName(const String& name, const String& uri, const String& prefix, const String& localName)
	{
		if (Xml::checkName(name)) {
			m_name = name;
			m_uri = uri;
			m_namespace = prefix;
			m_localName = localName;
			return sl_true;
		}
		return sl_false;
	}

	sl_size XmlElement::getAttributeCount() const
	{
		return m_attributes.getCount();
	}

	sl_bool XmlElement::getAttribute(sl_size index, XmlAttribute* _out) const
	{
		return m_attributes.getAt_NoLock(index, _out);
	}

	String XmlElement::getAttribute(const String& name) const
	{
		return m_mapAttributes.getValue_NoLock(name, String::null());
	}

	String XmlElement::getAttribute_IgnoreCase(const StringView& name) const
	{
		ListElements<XmlAttribute> attrs(m_attributes);
		for (sl_size i = 0; i < attrs.count; i++) {
			if (attrs[i].name.equals_IgnoreCase(name)) {
				return attrs[i].value;
			}
		}
		return sl_null;
	}

	String XmlElement::getAttribute(const StringView& uri, const StringView& localName) const
	{
		ListElements<XmlAttribute> attrs(m_attributes);
		for (sl_size i = 0; i < attrs.count; i++) {
			if (attrs[i].uri == uri && attrs[i].localName == localName) {
				return attrs[i].value;
			}
		}
		return sl_null;
	}

	String XmlElement::getAttribute_IgnoreCase(const StringView& uri, const StringView& localName) const
	{
		ListElements<XmlAttribute> attrs(m_attributes);
		for (sl_size i = 0; i < attrs.count; i++) {
			if (attrs[i].uri == uri && attrs[i].localName.equals_IgnoreCase(localName)) {
				return attrs[i].value;
			}
		}
		return sl_null;
	}

	sl_bool XmlElement::containsAttribute(const String& name) const
	{
		return m_mapAttributes.find_NoLock(name) != sl_null;
	}

	sl_bool XmlElement::containsAttribute_IgnoreCase(const StringView& name) const
	{
		ListElements<XmlAttribute> attrs(m_attributes);
		for (sl_size i = 0; i < attrs.count; i++) {
			if (attrs[i].name.equals_IgnoreCase(name)) {
				return sl_true;
			}
		}
		return sl_false;
	}

	sl_bool XmlElement::setAttribute(sl_size index, const String& value)
	{
		XmlAttribute attr;
		if (m_attributes.getAt_NoLock(index, &attr)) {
			m_mapAttributes.put_NoLock(attr.name, value);
			attr.value = value;
			return m_attributes.setAt_NoLock(index, attr);
		}
		return sl_false;
	}

	sl_bool XmlElement::setAttribute(sl_size index, const String& uri, const String& localName, const String& value)
	{
		XmlAttribute* attr = m_attributes.getPointerAt(index);
		if (attr) {
			m_mapAttributes.put_NoLock(attr->name, value);
			attr->uri = uri;
			attr->localName = localName;
			attr->value = value;
			return sl_true;
		}
		return sl_false;
	}

	sl_bool XmlElement::setAttribute(const String& name, const String& value)
	{
		if (!(Xml::checkName(name))) {
			return sl_false;
		}
		if (m_mapAttributes.find_NoLock(name)) {
			m_mapAttributes.put_NoLock(name, value);
			ListElements<XmlAttribute> attrs(m_attributes);
			for (sl_size i = 0; i < attrs.count; i++) {
				if (attrs[i].name == name) {
					attrs[i].value = value;
					return sl_true;
				}
			}
		} else {
			m_mapAttributes.put_NoLock(name, value);
			XmlAttribute attr;
			attr.name = name;
			attr.value = value;
			return m_attributes.add_NoLock(attr);
		}
		return sl_false;
	}

	sl_bool XmlElement::setAttribute(const XmlAttribute& attr)
	{
		if (!(Xml::checkName(attr.name))) {
			return sl_false;
		}
		if (m_mapAttributes.find_NoLock(attr.name)) {
			m_mapAttributes.put_NoLock(attr.name, attr.value);
			ListElements<XmlAttribute> attrs(m_attributes);
			for (sl_size i = 0; i < attrs.count; i++) {
				if (attrs[i].name == attr.name) {
					attrs[i] = attr;
					return sl_true;
				}
			}
		} else {
			m_mapAttributes.put_NoLock(attr.name, attr.value);
			return m_attributes.add_NoLock(attr);
		}
		return sl_false;
	}

	sl_bool XmlElement::setAttribute(const StringView& uri, const StringView& localName, const String& value)
	{
		ListElements<XmlAttribute> attrs(m_attributes);
		for (sl_size i = 0; i < attrs.count; i++) {
			if (attrs[i].uri == uri && attrs[i].localName == localName) {
				attrs[i].value = value;
				return sl_true;
			}
		}
		return sl_false;
	}

	sl_bool XmlElement::removeAttribute(sl_size index)
	{
		XmlAttribute attr;
		if (m_attributes.getAt_NoLock(index, &attr)) {
			m_mapAttributes.remove_NoLock(attr.name);
			return m_attributes.removeAt_NoLock(index);
		}
		return sl_false;
	}

	sl_bool XmlElement::removeAttribute(const String& name)
	{
		if (m_mapAttributes.remove_NoLock(name)) {
			ListElements<XmlAttribute> attrs(m_attributes);
			for (sl_size i = 0; i < attrs.count; i++) {
				if (attrs[i].name == name) {
					return m_attributes.removeAt_NoLock(i);
				}
			}
		}
		return sl_false;
	}

	void XmlElement::removeAllAttributes()
	{
		m_attributes.removeAll_NoLock();
		m_mapAttributes.removeAll_NoLock();
	}

	sl_size XmlElement::getStartContentPositionInSource() const
	{
		return m_positionStartContentInSource;
	}

	void XmlElement::setStartContentPositionInSource(sl_size pos)
	{
		m_positionStartContentInSource = pos;
	}

	sl_size XmlElement::getEndContentPositionInSource() const
	{
		return m_positionEndContentInSource;
	}

	void XmlElement::setEndContentPositionInSource(sl_size pos)
	{
		m_positionEndContentInSource = pos;
	}

	Ref<XmlElement> XmlElement::duplicate()
	{
		Ref<XmlElement> ret = new XmlElement;
		if (ret.isNotNull()) {
			ret->m_parent = m_parent;
			ret->m_document = m_document;
			ret->m_sourceFilePath = m_sourceFilePath;
			ret->m_positionStartInSource = m_positionStartInSource;
			ret->m_positionEndInSource = m_positionEndInSource;
			ret->m_lineInSource = m_lineInSource;
			ret->m_columnInSource = m_columnInSource;
			ret->m_children.addAll_NoLock(&m_children);
			ret->m_name = m_name;
			ret->m_uri = m_uri;
			ret->m_namespace = m_namespace;
			ret->m_localName = m_localName;
			ret->m_attributes = m_attributes.duplicate_NoLock();
			ret->m_mapAttributes = m_mapAttributes.duplicate_NoLock();
			ret->m_positionStartContentInSource = m_positionStartContentInSource;
			ret->m_positionEndContentInSource = m_positionEndContentInSource;
			return ret;
		}
		return sl_null;
	}

	SLIB_DEFINE_OBJECT(XmlDocument, XmlNodeGroup)

	XmlDocument::XmlDocument() : XmlNodeGroup(XmlNodeType::Document)
	{
	}

	XmlDocument::~XmlDocument()
	{
	}

	Ref<XmlDocument> XmlDocument::create()
	{
		return new XmlDocument;
	}

	sl_bool XmlDocument::buildXml(StringBuffer& output) const
	{
		return buildInnerXml(output);
	}

	Ref<XmlElement> XmlDocument::getElementById(const StringView& _id) const
	{
		return m_elementsById.getValue(_id, Ref<XmlElement>::null());
	}

	void XmlDocument::registerElementsById(const StringView& idAttributeName)
	{
		ListElements< Ref<XmlNode> > nodes(m_children);
		for (sl_size i = 0; i < nodes.count; i++) {
			Ref<XmlElement> e = nodes[i]->toElementNode();
			if (e.isNotNull()) {
				registerElementsById(e, idAttributeName);
			}
		}
	}

	void XmlDocument::registerElementsById(const Ref<XmlElement>& element, const StringView& idAttributeName)
	{
		if (element.isNotNull()) {
			String value = element->getAttribute(idAttributeName);
			if (value.isNotEmpty()) {
				m_elementsById.put_NoLock(value, element);
			}
			ListElements< Ref<XmlNode> > nodes(element->m_children);
			for (sl_size i = 0; i < nodes.count; i++) {
				Ref<XmlElement> e = nodes[i]->toElementNode();
				if (e.isNotNull()) {
					registerElementsById(e, idAttributeName);
				}
			}
		}
	}

	sl_bool XmlDocument::checkWellFormed() const
	{
		sl_bool flagFoundRoot = sl_false;
		ListElements< Ref<XmlNode> > nodes(m_children);
		for (sl_size i = 0; i < nodes.count; i++) {
			XmlNodeType type = nodes[i]->getType();
			if (type == XmlNodeType::Element) {
				if (flagFoundRoot) {
					return sl_false;
				}
				flagFoundRoot = sl_true;
			} else if (type == XmlNodeType::Text) {
				return sl_false;
			}
		}
		return flagFoundRoot;
	}


	SLIB_DEFINE_OBJECT(XmlText, XmlNode)

	XmlText::XmlText() : XmlNode(XmlNodeType::Text), m_flagCDATA(sl_false)
	{
	}

	XmlText::~XmlText()
	{
	}

	Ref<XmlText> XmlText::create(const String& text, sl_bool flagCDATA)
	{
		Ref<XmlText> ret = new XmlText;
		if (ret.isNotNull()) {
			ret->m_text = text;
			ret->m_flagCDATA = flagCDATA;
			return ret;
		}
		return sl_null;
	}

	Ref<XmlText> XmlText::createCDATA(const String& text)
	{
		return create(text, sl_true);
	}

	sl_bool XmlText::buildText(StringBuffer& output) const
	{
		return output.add(m_text);
	}

	sl_bool XmlText::buildXml(StringBuffer& output) const
	{
		String text = m_text;
		if (text.isEmpty()) {
			return sl_true;
		}
		if (m_flagCDATA) {
			if (!(output.addStatic("<![CDATA["))) {
				return sl_false;
			}
			StringStorage data(text);
			sl_char8* str = data.data8;
			sl_size len = data.length;
			sl_size start = 0;
			for (sl_size i = 0; i + 2 < len; i++) {
				if (str[i] == ']' && str[i+1] == ']' && str[i+2] == '>') {
					if (i > start) {
						data.data8 = str + start;
						data.length = i - start;
						if (!(output.add(data))) {
							return sl_false;
						}
						start = i + 3;
						i = start - 1;
					}
					if (!(output.addStatic("]]]]><![CDATA[>"))) {
						return sl_false;
					}
				}
			}
			if (len > start) {
				data.data8 = str + start;
				data.length = len - start;
				if (!(output.add(data))) {
					return sl_false;
				}
			}
			if (!(output.addStatic("]]>"))) {
				return sl_false;
			}
			return sl_true;
		} else {
			return Xml::encodeTextToEntities(text, output);
		}
	}

	String XmlText::getText() const
	{
		return m_text;
	}

	void XmlText::setText(const String& text)
	{
		m_text = text;
	}

	sl_bool XmlText::isCDATA() const
	{
		return m_flagCDATA;
	}

	void XmlText::setCDATA(sl_bool flag)
	{
		m_flagCDATA = flag;
	}


	SLIB_DEFINE_OBJECT(XmlProcessingInstruction, XmlNode)

	XmlProcessingInstruction::XmlProcessingInstruction() : XmlNode(XmlNodeType::ProcessingInstruction)
	{
	}

	XmlProcessingInstruction::~XmlProcessingInstruction()
	{
	}

	Ref<XmlProcessingInstruction> XmlProcessingInstruction::create(const String& target, const String& content)
	{
		if (Xml::checkName(target)) {
			Ref<XmlProcessingInstruction> ret = new XmlProcessingInstruction;
			if (ret.isNotNull()) {
				ret->m_target = target;
				ret->m_content = content;
				return ret;
			}
		}
		return sl_null;
	}

	sl_bool XmlProcessingInstruction::buildText(StringBuffer& output) const
	{
		return sl_true;
	}

	sl_bool XmlProcessingInstruction::buildXml(StringBuffer& output) const
	{
		String target = m_target;
		if (target.isEmpty()) {
			return sl_false;
		}
		if (!(output.addStatic("<?"))) {
			return sl_false;
		}
		if (!(output.add(target))) {
			return sl_false;
		}
		if (!(output.addStatic(" "))) {
			return sl_false;
		}
		// content
		{
			String content = m_content;
			StringStorage data(content);
			sl_char8* str = data.data8;
			sl_size len = data.length;
			sl_size start = 0;
			for (sl_size i = 0; i + 1 < len; i++) {
				if (str[i] == '?' && str[i+1] == '>') {
					if (i > start) {
						data.data8 = str + start;
						data.length = i - start;
						if (!(output.add(data))) {
							return sl_false;
						}
						start = i + 2;
						i = start - 1;
					}
				}
			}
			if (len > start) {
				data.data8 = str + start;
				data.length = len - start;
				if (!(output.add(data))) {
					return sl_false;
				}
			}
		}
		if (!(output.addStatic("?>"))) {
			return sl_false;
		}
		return sl_true;
	}

	String XmlProcessingInstruction::getTarget() const
	{
		return m_target;
	}

	sl_bool XmlProcessingInstruction::setTarget(const String& target)
	{
		if (Xml::checkName(target)) {
			m_target = target;
			return sl_true;
		}
		return sl_false;
	}

	String XmlProcessingInstruction::getContent() const
	{
		return m_content;
	}

	void XmlProcessingInstruction::setContent(const String& content)
	{
		m_content = content;
	}


	SLIB_DEFINE_OBJECT(XmlComment, XmlNode)

	XmlComment::XmlComment() : XmlNode(XmlNodeType::Comment)
	{
	}

	XmlComment::~XmlComment()
	{
	}

	Ref<XmlComment> XmlComment::create(const String& comment)
	{
		Ref<XmlComment> ret = new XmlComment;
		if (ret.isNotNull()) {
			ret->m_comment = comment;
			return ret;
		}
		return sl_null;
	}

	sl_bool XmlComment::buildText(StringBuffer& output) const
	{
		return sl_true;
	}

	sl_bool XmlComment::buildXml(StringBuffer& output) const
	{
		String comment = m_comment;
		if (comment.isEmpty()) {
			return sl_true;
		}
		if (!(output.addStatic("<!--"))) {
			return sl_false;
		}
		// comment
		{
			StringStorage data(comment);
			sl_char8* str = data.data8;
			sl_size len = data.length;
			sl_size start = 0;
			for (sl_size i = 0; i + 1 < len; i++) {
				if (str[i] == '-' && str[i+1] == '-') {
					if (i > start) {
						data.data8 = str + start;
						data.length = i - start;
						if (!(output.add(data))) {
							return sl_false;
						}
						start = i + 2;
						i = start - 1;
					}
				}
			}
			if (len > start) {
				data.data8 = str + start;
				data.length = len - start;
				if (!(output.add(data))) {
					return sl_false;
				}
			}
		}
		if (!(output.addStatic("-->"))) {
			return sl_false;
		}
		return sl_true;
	}

	const String& XmlComment::getComment() const
	{
		return m_comment;
	}

	void XmlComment::setComment(const String& comment)
	{
		m_comment = comment;
	}


	SLIB_DEFINE_OBJECT(XmlWhiteSpace, XmlNode)

	XmlWhiteSpace::XmlWhiteSpace() : XmlNode(XmlNodeType::WhiteSpace)
	{
	}

	XmlWhiteSpace::~XmlWhiteSpace()
	{
	}

	Ref<XmlWhiteSpace> XmlWhiteSpace::create(const String& content)
	{
		Ref<XmlWhiteSpace> ret = new XmlWhiteSpace;
		if (ret.isNotNull()) {
			ret->m_content = content;
			return ret;
		}
		return sl_null;
	}

	sl_bool XmlWhiteSpace::buildText(StringBuffer& output) const
	{
		return sl_true;
	}

	sl_bool XmlWhiteSpace::buildXml(StringBuffer& output) const
	{
		if (!(output.add(m_content))) {
			return sl_false;
		}
		return sl_true;
	}

	const String& XmlWhiteSpace::getContent() const
	{
		return m_content;
	}

	void XmlWhiteSpace::setContent(const String& content)
	{
		m_content = content;
	}


	SLIB_DEFINE_OBJECT(XmlDocumentTypeDefinition, XmlNode)

	XmlDocumentTypeDefinition::XmlDocumentTypeDefinition(): XmlNode(XmlNodeType::DocumentTypeDefinition), m_kind(XmlDocumentTypeDefinitionKind::None)
	{
	}

	XmlDocumentTypeDefinition::~XmlDocumentTypeDefinition()
	{
	}

	Ref<XmlDocumentTypeDefinition> XmlDocumentTypeDefinition::create(const String& rootElement, XmlDocumentTypeDefinitionKind kind, const String& publicIndentifier, const String& uri, const String& subsets)
	{
		Ref<XmlDocumentTypeDefinition> ret = new XmlDocumentTypeDefinition;
		if (ret.isNotNull()) {
			ret->m_rootElement = rootElement;
			ret->m_kind = kind;
			ret->m_publicIdentifier = publicIndentifier;
			ret->m_uri = uri;
			ret->m_subsets = subsets;
			return ret;
		}
		return sl_null;
	}

	sl_bool XmlDocumentTypeDefinition::buildText(StringBuffer& output) const
	{
		return sl_true;
	}

	sl_bool XmlDocumentTypeDefinition::buildXml(StringBuffer& output) const
	{
		if (!(output.addStatic("<!DOCTYPE "))) {
			return sl_false;
		}
		if (!(output.add(m_rootElement))) {
			return sl_false;
		}
		if (m_kind == XmlDocumentTypeDefinitionKind::Public) {
			if (!(output.addStatic(" PUBLIC \""))) {
				return sl_false;
			}
			if (!(Xml::encodeTextToEntities(m_publicIdentifier, output))) {
				return sl_false;
			}
			if (m_uri.isNotNull()) {
				if (!(output.addStatic("\" \""))) {
					return sl_false;
				}
				if (!(Xml::encodeTextToEntities(m_uri, output))) {
					return sl_false;
				}
			}
			if (!(output.addStatic("\""))) {
				return sl_false;
			}
		} else if (m_kind == XmlDocumentTypeDefinitionKind::System) {
			if (!(output.addStatic(" SYSTEM \""))) {
				return sl_false;
			}
			if (!(Xml::encodeTextToEntities(m_uri, output))) {
				return sl_false;
			}
			if (!(output.addStatic("\""))) {
				return sl_false;
			}
		}
		if (m_subsets.isNotNull()) {
			if (!(output.addStatic(" ["))) {
				return sl_false;
			}
			if (!(output.add(m_subsets))) {
				return sl_false;
			}
			if (!(output.addStatic("]"))) {
				return sl_false;
			}
		}
		if (!(output.addStatic(">"))) {
			return sl_false;
		}
		return sl_true;
	}

	const String& XmlDocumentTypeDefinition::getRootElement() const
	{
		return m_rootElement;
	}

	void XmlDocumentTypeDefinition::setRootElement(const String& value)
	{
		m_rootElement = value;
	}

	XmlDocumentTypeDefinitionKind XmlDocumentTypeDefinition::getKind() const
	{
		return m_kind;
	}

	void XmlDocumentTypeDefinition::setKind(XmlDocumentTypeDefinitionKind value)
	{
		m_kind = value;
	}

	const String& XmlDocumentTypeDefinition::getPublicIndentifier() const
	{
		return m_publicIdentifier;
	}

	void XmlDocumentTypeDefinition::setPublicIdentifier(const String& value)
	{
		m_publicIdentifier = value;
	}

	const String& XmlDocumentTypeDefinition::getUri() const
	{
		return m_uri;
	}

	void XmlDocumentTypeDefinition::setUri(const String& value)
	{
		m_uri = value;
	}

	const String& XmlDocumentTypeDefinition::getSubsets() const
	{
		return m_subsets;
	}

	void XmlDocumentTypeDefinition::setSubsets(const String& value)
	{
		m_subsets = value;
	}


	SLIB_DEFINE_NESTED_CLASS_DEFAULT_MEMBERS(Xml, ParseParam)

	Xml::ParseParam::ParseParam()
	{
		flagCreateDocument = sl_true;
		flagCreateCommentNodes = sl_false;
		flagCreateProcessingInstructionNodes = sl_true;
		flagCreateTextNodes = sl_true;
		flagCreateWhiteSpaces = sl_false;

		flagProcessNamespaces = sl_true;
		flagCheckWellFormed = sl_true;
		flagSupportCpp11String = sl_false;

		flagLogError = sl_true;

		flagError = sl_false;
		errorLine = 0;
		errorColumn = 0;
		errorPosition = 0;
	}

	String Xml::ParseParam::getErrorText()
	{
		if (flagError) {
			return String::concat("(", String::fromSize(errorLine), ":", String::fromSize(errorColumn), ") ", errorMessage);
		}
		return sl_null;
	}

	void Xml::ParseParam::setCreatingAll()
	{
		flagCreateDocument = sl_true;
		flagCreateCommentNodes = sl_true;
		flagCreateProcessingInstructionNodes = sl_true;
		flagCreateTextNodes = sl_true;
		flagCreateWhiteSpaces = sl_true;
	}

	void Xml::ParseParam::setCreatingOnlyElements()
	{
		flagCreateDocument = sl_true;
		flagCreateCommentNodes = sl_false;
		flagCreateProcessingInstructionNodes = sl_false;
		flagCreateTextNodes = sl_false;
		flagCreateWhiteSpaces = sl_false;
	}

	void Xml::ParseParam::setCreatingOnlyElementsAndTexts()
	{
		flagCreateDocument = sl_true;
		flagCreateCommentNodes = sl_false;
		flagCreateProcessingInstructionNodes = sl_false;
		flagCreateTextNodes = sl_true;
		flagCreateWhiteSpaces = sl_false;
	}


	SLIB_DEFINE_NESTED_CLASS_DEFAULT_MEMBERS(Xml, ParseControl)

	Xml::ParseControl::ParseControl()
	{
		characterSize = 0;
		flagChangeSource = sl_false;
		parsingPosition = 0;
		flagStopParsing = sl_false;
		currentNode = sl_null;
	}

	namespace {

		template <class CHAR>
		class XmlParser
		{
		public:
			typedef typename StringTypeFromCharType<CHAR>::Type StringType;
			typedef typename StringViewTypeFromCharType<CHAR>::Type StringViewType;
			typedef typename StringBufferTypeFromCharType<CHAR>::Type StringBufferType;
			const CHAR* buf;
			sl_size len;
			sl_size pos;
			sl_size lineNumber;
			sl_size columnNumber;
			sl_size posForLineColumn;

			Ref<XmlDocument> document;
			Xml::ParseControl control;

			Xml::ParseParam param;

			sl_bool flagError;
			String errorMessage;

		public:
			XmlParser();

		public:
			void escapeWhiteSpaces();

			void calcLineNumber();

			void createWhiteSpace(XmlNodeGroup* parent, sl_size posStart, sl_size posEnd);

			void unescapeEntity(StringBufferType* buf);

			void parseName(String& name);

			void parseComment(XmlNodeGroup* parent);

			void parseCDATA(XmlNodeGroup* parent);

			void parseDOCTYPE(XmlNodeGroup* parent);

			void parsePI(XmlNodeGroup* parent);

			void processPrefix(const String& name, const String& defNamespace, const HashMap<String, String>& namespaces, String& prefix, String& uri, String& localName);

			void parseAttribute(String& name, String& value);

			void parseAttributeValue(String& value);

			void parseElement(XmlNodeGroup* parent, const String& defNamespace, const HashMap<String, String>& namespaces);

			void parseText(XmlNodeGroup* parent);

			void parseNodes(XmlNodeGroup* parent, const String& defNamespace, const HashMap<String, String>& namespaces);

			void parse();

			static Ref<XmlDocument> parse(const CHAR* buf, sl_size len, Xml::ParseParam& param);

		};

		// 1: valid, 2: invalid for starting, 0: invalid
		static const sl_uint8 g_patternCheckName[128] = {
			/*		NUL		SOH		STX		ETX		EOT		ENQ		ACK		BEL		*/
			/*00*/	0,		0,		0,		0,		0,		0,		0,		0,
			/*		BS		HT		LF		VT		FF		CR		SO		SI		*/
			/*08*/	0,		0,		0,		0,		0,		0,		0,		0,
			/*		DLE		DC1		DC2		DC3		DC4		NAK		SYN		ETB		*/
			/*10*/	0,		0,		0,		0,		0,		0,		0,		0,
			/*		CAN		EM		SUB		ESC		FS		GS		RS		US		*/
			/*18*/	0,		0,		0,		0,		0,		0,		0,		0,
			/*		SP		!		"		#		$		%		&		'		*/
			/*20*/	0,		0,		0,		0,		0,		0,		0,		0,
			/*		(		)		*		+		,		-		.		/		*/
			/*28*/	0,		0,		0,		0,		0,		2,		2,		0,
			/*		0		1		2		3		4		5		6		7		*/
			/*30*/	2,		2,		2,		2,		2,		2,		2,		2,
			/*		8		9		:		;		<		=		>		?		*/
			/*38*/	2,		2,		1,		0,		0,		0,		0,		0,
			/*		@		A		B		C		D		E		F		G		*/
			/*40*/	0,		1,		1,		1,		1,		1,		1,		1,
			/*		H		I		J		K		L		M		N		O		*/
			/*48*/	1,		1,		1,		1,		1,		1,		1,		1,
			/*		P		Q		R		S		T		U		V		W		*/
			/*50*/	1,		1,		1,		1,		1,		1,		1,		1,
			/*		X		Y		Z		[		\		]		^		_		*/
			/*58*/	1,		1,		1,		0,		0,		0,		0,		1,
			/*		`		a		b		c		d		e		f		g		*/
			/*60*/	0,		1,		1,		1,		1,		1,		1,		1,
			/*		h		i		j		k		l		m		n		o		*/
			/*68*/	1,		1,		1,		1,		1,		1,		1,		1,
			/*		p		q		r		s		t		u		v		w		*/
			/*70*/	1,		1,		1,		1,		1,		1,		1,		1,
			/*		x		y		z		{		|		}		~		DEL		*/
			/*78*/	1,		1,		1,		0,		0,		0,		1,		0
		};

		SLIB_STATIC_STRING(g_strError_unknown, "Unknown Error")
		SLIB_STATIC_STRING(g_strError_memory_lack, "Lack of Memory")
		SLIB_STATIC_STRING(g_strError_user_stop, "User stopped parsing")
		SLIB_STATIC_STRING(g_strError_invalid_escape, "Invalid escaping entity")
		SLIB_STATIC_STRING(g_strError_escape_not_end, "Missing semi-colon(;) at the end of entity definition")
		SLIB_STATIC_STRING(g_strError_invalid_markup, "Invalid Markup")
		SLIB_STATIC_STRING(g_strError_comment_double_hyphen, "Double-hyphen(--) is not allowed in comment text")
		SLIB_STATIC_STRING(g_strError_comment_not_end, "Comment Section must be ended with -->")
		SLIB_STATIC_STRING(g_strError_CDATA_not_end, "CDATA Section must be ended with ]]>")
		SLIB_STATIC_STRING(g_strError_name_missing, "Name definition is missing")
		SLIB_STATIC_STRING(g_strError_name_invalid_start, "Name definition is starting with invalid character")
		SLIB_STATIC_STRING(g_strError_name_invalid_char, "Name definition is containing invalid character")
		SLIB_STATIC_STRING(g_strError_DOCTYPE_not_end, "DOCTYPE section must be ended with >")
		SLIB_STATIC_STRING(g_strError_PI_not_end, "Processing Instruction Section must be ended with ?>")
		SLIB_STATIC_STRING(g_strError_element_tag_not_end, "Element tag definition must be ended with > or />")
		SLIB_STATIC_STRING(g_strError_element_tag_not_matching_end_tag, "Element must be terminated by the matching end-tag")
		SLIB_STATIC_STRING(g_strError_element_attr_required_assign, "An assign(=) symbol is required for attribute definition")
		SLIB_STATIC_STRING(g_strError_element_attr_required_quot, "Attribute value definition must be started with \" or ' symbol")
		SLIB_STATIC_STRING(g_strError_element_attr_not_end, "Attribute value definition does not be ended")
		SLIB_STATIC_STRING(g_strError_element_attr_end_with_invalid_char, "Attribute value definition must be followed by >, /, or whitespaces")
		SLIB_STATIC_STRING(g_strError_element_attr_duplicate, "Attribute name is already specified")
		SLIB_STATIC_STRING(g_strError_content_include_lt, "Content must not include less-than(<) character")
		SLIB_STATIC_STRING(g_strError_document_not_wellformed, "Document must be well-formed")

#define CALL_CALLBACK(NAME, NODE, ...) \
{ \
	auto _callback = param.NAME; \
	if (_callback.isNotNull()) { \
		control.parsingPosition = pos; \
		control.flagChangeSource = sl_false; \
		control.currentNode = NODE; \
		_callback(&control, __VA_ARGS__); \
		if (control.flagStopParsing) { \
			flagError = sl_true; \
			errorMessage = g_strError_user_stop; \
			return; \
		} \
		if (control.flagChangeSource) { \
			buf = (CHAR*)(control.source.data8); \
			len = control.source.length; \
		} \
		pos = control.parsingPosition; \
	} \
}

#define REPORT_ERROR(MSG) \
flagError = sl_true; \
errorMessage = MSG; \
return;

		template <class CHAR>
		XmlParser<CHAR>::XmlParser()
		{
			pos = 0;
			lineNumber = 1;
			columnNumber = 1;
			posForLineColumn = 0;

			buf = sl_null;
			len = 0;

			flagError = sl_false;
		}

		template <class CHAR>
		void XmlParser<CHAR>::escapeWhiteSpaces()
		{
			while (pos < len) {
				CHAR ch = buf[pos];
				if (!(SLIB_CHAR_IS_WHITE_SPACE(ch))) {
					break;
				}
				pos++;
			}
		}

		template <class CHAR>
		void XmlParser<CHAR>::calcLineNumber()
		{
			for (sl_size i = posForLineColumn; i < pos; i++) {
				CHAR ch = buf[i];
				if (ch == '\r') {
					lineNumber++;
					columnNumber = 1;
					if (i + 1 < len && buf[i + 1] == '\n') {
						i++;
					}
				} else if (ch == '\n') {
					if (!i || buf[i - 1] != '\r') {
						lineNumber++;
						columnNumber = 1;
					}
				} else {
					columnNumber++;
				}
			}
			posForLineColumn = pos;
		}

		template <class CHAR>
		void XmlParser<CHAR>::createWhiteSpace(XmlNodeGroup* parent, sl_size posStart, sl_size posEnd)
		{
			if (posEnd <= posStart) {
				return;
			}
			if (param.flagCreateWhiteSpaces) {
				if (parent) {
					String content = String::create(buf + posStart, posEnd - posStart);
					if (content.isNull()) {
						REPORT_ERROR(g_strError_memory_lack)
					}
					Ref<XmlWhiteSpace> node = XmlWhiteSpace::create(content);
					if (node.isNull()) {
						REPORT_ERROR(g_strError_memory_lack)
					}
					calcLineNumber();
					node->setSourceFilePath(param.sourceFilePath);
					node->setStartPositionInSource(posStart);
					node->setEndPositionInSource(posEnd);
					node->setLineNumberInSource(lineNumber);
					node->setColumnNumberInSource(columnNumber);
					if (!(parent->addChild(node))) {
						REPORT_ERROR(g_strError_memory_lack)
					}
				}
			}
		}

		template <class CHAR>
		void XmlParser<CHAR>::unescapeEntity(StringBufferType* sb)
		{
			if (pos + 2 < len && buf[pos] == 'l' && buf[pos + 1] == 't' && buf[pos + 2] == ';') {
				static CHAR sc = '<';
				if (sb) {
					if (!(sb->addStatic(&sc, 1))) {
						REPORT_ERROR(g_strError_memory_lack)
					}
				}
				pos += 3;
			} else if (pos + 2 < len && buf[pos] == 'g' && buf[pos + 1] == 't' && buf[pos + 2] == ';') {
				static CHAR sc = '>';
				if (sb) {
					if (!(sb->addStatic(&sc, 1))) {
						REPORT_ERROR(g_strError_memory_lack)
					}
				}
				pos += 3;
			} else if (pos + 3 < len && buf[pos] == 'a' && buf[pos + 1] == 'm' && buf[pos + 2] == 'p' && buf[pos + 3] == ';') {
				static CHAR sc = '&';
				if (sb) {
					if (!(sb->addStatic(&sc, 1))) {
						REPORT_ERROR(g_strError_memory_lack)
					}
				}
				pos += 4;
			} else if (pos + 4 < len && buf[pos] == 'a' && buf[pos + 1] == 'p' && buf[pos + 2] == 'o' && buf[pos + 3] == 's' && buf[pos + 4] == ';') {
				static CHAR sc = '\'';
				if (sb) {
					if (!(sb->addStatic(&sc, 1))) {
						REPORT_ERROR(g_strError_memory_lack)
					}
				}
				pos += 5;
			} else if (pos + 4 < len && buf[pos] == 'q' && buf[pos + 1] == 'u' && buf[pos + 2] == 'o' && buf[pos + 3] == 't' && buf[pos + 4] == ';') {
				static CHAR sc = '\"';
				if (sb) {
					if (!(sb->addStatic(&sc, 1))) {
						REPORT_ERROR(g_strError_memory_lack)
					}
				}
				pos += 5;
			} else if (pos + 2 < len && buf[pos] == '#') {
				pos++;
				sl_uint32 n;
				sl_reg parseRes;
				if (buf[pos] == 'x') {
					pos++;
					parseRes = StringType::parseUint32(16, &n, buf, pos, len);
				} else {
					parseRes = StringType::parseUint32(10, &n, buf, pos, len);
				}
				if (parseRes == SLIB_PARSE_ERROR) {
					REPORT_ERROR(g_strError_invalid_escape)
				}
				pos = parseRes;
				if (pos >= len) {
					REPORT_ERROR(g_strError_escape_not_end)
				}
				if (buf[pos] != ';') {
					REPORT_ERROR(g_strError_escape_not_end)
				}
				sl_char32 _n = (sl_char32)n;
				StringType s = StringType::create(&_n, 1);
				if (s.isNull()) {
					REPORT_ERROR(g_strError_memory_lack)
				}
				if (sb) {
					if (!(sb->add(s))) {
						REPORT_ERROR(g_strError_memory_lack)
					}
				}
				pos++;
			} else {
				REPORT_ERROR(g_strError_invalid_escape)
			}
		}

		template <class CHAR>
		void XmlParser<CHAR>::parseName(String& name)
		{
			if (pos >= len) {
				REPORT_ERROR(g_strError_name_missing)
			}
			sl_uint32 ch = (sl_uint32)(buf[pos]);
			if (ch < 128 && g_patternCheckName[ch] != 1) {
				REPORT_ERROR(g_strError_name_invalid_start)
			}
			sl_size start = pos;
			pos++;
			while (pos < len) {
				ch = (sl_uint32)(buf[pos]);
				if (ch < 128 && !(g_patternCheckName[ch])) {
					break;
				}
				pos++;
			}
			name = String::create(buf + start, pos - start);
			if (name.isNull()) {
				REPORT_ERROR(g_strError_memory_lack)
			}
		}


		template <class CHAR>
		void XmlParser<CHAR>::parseComment(XmlNodeGroup* parent)
		{
			calcLineNumber();
			sl_size startLine = lineNumber;
			sl_size startColumn = columnNumber;
			sl_size startComment = pos;
			sl_bool flagEnded = sl_false;
			while (pos + 2 < len) {
				if (buf[pos] == '-' && buf[pos + 1] == '-') {
					if (buf[pos + 2] == '>') {
						if (param.flagCreateCommentNodes) {
							String str = String::create(buf + startComment, pos - startComment);
							if (str.isNull()) {
								REPORT_ERROR(g_strError_memory_lack)
							}
							if (parent) {
								Ref<XmlComment> comment = XmlComment::create(str);
								if (comment.isNull()) {
									REPORT_ERROR(g_strError_memory_lack)
								}
								comment->setSourceFilePath(param.sourceFilePath);
								comment->setStartPositionInSource(startComment);
								comment->setEndPositionInSource(pos + 3);
								comment->setLineNumberInSource(startLine);
								comment->setColumnNumberInSource(startColumn);
								if (!(parent->addChild(comment))) {
									REPORT_ERROR(g_strError_memory_lack)
								}
								CALL_CALLBACK(onComment, comment.get(), str)
							} else {
								CALL_CALLBACK(onComment, sl_null, str)
							}
						}
						pos += 3;
						flagEnded = sl_true;
						break;
					} else {
						REPORT_ERROR(g_strError_comment_double_hyphen)
					}
				} else {
					pos++;
				}
			}
			if (!flagEnded) {
				REPORT_ERROR(g_strError_comment_not_end)
			}
		}

		template <class CHAR>
		void XmlParser<CHAR>::parseCDATA(XmlNodeGroup* parent)
		{
			calcLineNumber();
			sl_size startLine = lineNumber;
			sl_size startColumn = columnNumber;
			sl_size startCDATA = pos;
			sl_bool flagEnded = sl_false;
			while (pos + 2 < len) {
				if (buf[pos] == ']' && buf[pos + 1] == ']' && buf[pos + 2] == '>') {
					if (param.flagCreateTextNodes) {
						String str = String::create(buf + startCDATA, pos - startCDATA);
						if (str.isNull()) {
							REPORT_ERROR(g_strError_memory_lack)
						}
						if (parent) {
							Ref<XmlText> text = XmlText::createCDATA(str);
							if (text.isNull()) {
								REPORT_ERROR(g_strError_memory_lack)
							}
							text->setSourceFilePath(param.sourceFilePath);
							text->setStartPositionInSource(startCDATA);
							text->setEndPositionInSource(pos + 3);
							text->setLineNumberInSource(startLine);
							text->setColumnNumberInSource(startColumn);
							if (!(parent->addChild(text))) {
								REPORT_ERROR(g_strError_memory_lack)
							}
							CALL_CALLBACK(onCDATA, text.get(), str)
						} else {
							CALL_CALLBACK(onCDATA, sl_null, str)
						}
					}
					pos += 3;
					flagEnded = sl_true;
					break;
				} else {
					pos++;
				}
			}
			if (!flagEnded) {
				REPORT_ERROR(g_strError_CDATA_not_end)
			}
		}

		template <class CHAR>
		void XmlParser<CHAR>::parseDOCTYPE(XmlNodeGroup* parent)
		{
			sl_size startLine = lineNumber;
			sl_size startColumn = columnNumber;
			sl_size startDTD = pos;
			String rootElement;
			parseName(rootElement);
			if (flagError) {
				return;
			}
			escapeWhiteSpaces();
			XmlDocumentTypeDefinitionKind kind = XmlDocumentTypeDefinitionKind::None;
			String fpi, uri;
			if (pos + 6 < len) {
				CHAR c6 = buf[pos + 6];
				if (buf[pos] == 'P' && buf[pos + 1] == 'U' && buf[pos + 2] == 'B' && buf[pos + 3] == 'L' && buf[pos + 4] == 'I' && buf[pos + 5] == 'C' && SLIB_CHAR_IS_WHITE_SPACE(c6)) {
					kind = XmlDocumentTypeDefinitionKind::Public;
					pos += 6;
					escapeWhiteSpaces();
					parseAttributeValue(fpi);
					if (flagError) {
						return;
					}
					escapeWhiteSpaces();
					if (pos >= len) {
						REPORT_ERROR(g_strError_DOCTYPE_not_end)
					}
					if (buf[pos] == '\"') {
						parseAttributeValue(uri);
						if (flagError) {
							return;
						}
						escapeWhiteSpaces();
					}
				} else if (buf[pos] == 'S' && buf[pos + 1] == 'Y' && buf[pos + 2] == 'S' && buf[pos + 3] == 'T' && buf[pos + 4] == 'E' && buf[pos + 5] == 'M' && SLIB_CHAR_IS_WHITE_SPACE(c6)) {
					kind = XmlDocumentTypeDefinitionKind::System;
					pos += 6;
					escapeWhiteSpaces();
					parseAttributeValue(uri);
					if (flagError) {
						return;
					}
					escapeWhiteSpaces();
				}
			}
			// Internal subset
			if (pos >= len) {
				REPORT_ERROR(g_strError_DOCTYPE_not_end)
			}
			String subsets;
			if (buf[pos] == '[') {
				pos++;
				sl_size startSubsets = pos;
				while (pos < len) {
					if (buf[pos] == ']') {
						subsets = String::create(buf + startSubsets, pos - startSubsets);
						break;
					}
					pos++;
				}
				if (pos + 1 >= len) {
					REPORT_ERROR(g_strError_DOCTYPE_not_end)
				}
				pos++;
				escapeWhiteSpaces();
			}
			if (buf[pos] != '>') {
				REPORT_ERROR(g_strError_DOCTYPE_not_end)
			}
			pos++;
			Ref<XmlDocumentTypeDefinition> dtd = XmlDocumentTypeDefinition::create(rootElement, kind, fpi, uri, subsets);
			if (dtd.isNull()) {
				REPORT_ERROR(g_strError_memory_lack)
			}
			dtd->setSourceFilePath(param.sourceFilePath);
			dtd->setStartPositionInSource(startDTD);
			dtd->setEndPositionInSource(pos);
			dtd->setLineNumberInSource(startLine);
			dtd->setColumnNumberInSource(startColumn);
			if (parent) {
				if (!(parent->addChild(dtd))) {
					REPORT_ERROR(g_strError_memory_lack)
				}
			}
			CALL_CALLBACK(onDTD, dtd.get(), dtd.get())
		}

		template <class CHAR>
		void XmlParser<CHAR>::parsePI(XmlNodeGroup* parent)
		{
			String target;
			parseName(target);
			if (flagError) {
				return;
			}
			if (pos >= len) {
				REPORT_ERROR(g_strError_PI_not_end)
			}
			CHAR ch = buf[pos];
			if (ch != '?') {
				if (SLIB_CHAR_IS_WHITE_SPACE(ch)) {
					pos++;
					escapeWhiteSpaces();
				} else {
					REPORT_ERROR(g_strError_name_invalid_char)
				}
			}
			calcLineNumber();
			sl_size startLine = lineNumber;
			sl_size startColumn = columnNumber;
			sl_size startPI = pos;
			sl_bool flagEnded = sl_false;
			while (pos + 1 < len) {
				if (buf[pos] == '?' && buf[pos + 1] == '>') {
					if (param.flagCreateProcessingInstructionNodes) {
						String str = String::create(buf + startPI, pos - startPI);
						if (str.isNull()) {
							REPORT_ERROR(g_strError_memory_lack)
						}
						if (parent) {
							Ref<XmlProcessingInstruction> PI = XmlProcessingInstruction::create(target, str);
							if (PI.isNull()) {
								REPORT_ERROR(g_strError_memory_lack)
							}
							PI->setSourceFilePath(param.sourceFilePath);
							PI->setStartPositionInSource(startPI);
							PI->setEndPositionInSource(pos + 2);
							PI->setLineNumberInSource(startLine);
							PI->setColumnNumberInSource(startColumn);
							if (!(parent->addChild(PI))) {
								REPORT_ERROR(g_strError_memory_lack)
							}
							CALL_CALLBACK(onProcessingInstruction, PI.get(), target, str)
						} else {
							CALL_CALLBACK(onProcessingInstruction, sl_null, target, str)
						}
					}
					pos += 2;
					flagEnded = sl_true;
					break;
				} else {
					pos++;
				}
			}
			if (!flagEnded) {
				REPORT_ERROR(g_strError_PI_not_end)
			}
		}

		template <class CHAR>
		SLIB_INLINE void XmlParser<CHAR>::processPrefix(const String& name, const String& defNamespace, const HashMap<String, String>& namespaces, String& prefix, String& uri, String& localName)
		{
			sl_reg index = name.indexOf(':');
			if (index >= 0) {
				prefix = name.substring(0, index);
				localName = name.substring(index + 1);
				namespaces.get(prefix, &uri);
			} else {
				localName = name;
				uri = defNamespace;
			}
		}

		template <class CHAR>
		void XmlParser<CHAR>::parseAttribute(String& name, String& value)
		{
			parseName(name);
			if (flagError) {
				return;
			}
			if (pos >= len) {
				REPORT_ERROR(g_strError_element_tag_not_end)
			}
			CHAR ch = buf[pos];
			if (ch != '=') {
				if (SLIB_CHAR_IS_WHITE_SPACE(ch)) {
					pos++;
					escapeWhiteSpaces();
				} else {
					REPORT_ERROR(g_strError_name_invalid_char)
				}
			}
			if (pos >= len) {
				REPORT_ERROR(g_strError_element_attr_required_assign)
			}
			if (buf[pos] != '=') {
				REPORT_ERROR(g_strError_element_attr_required_assign)
			}
			pos++;
			escapeWhiteSpaces();
			if (pos >= len) {
				REPORT_ERROR(g_strError_element_attr_required_quot)
			}
			parseAttributeValue(value);
		}

		template <class CHAR>
		void XmlParser<CHAR>::parseAttributeValue(String& value)
		{
			CHAR ch = buf[pos];
			if (ch == '\"' || ch == '\'') {
				pos++;
				sl_size startAttrValue = pos;
				sl_bool flagEnded = sl_false;
				StringBufferType sb;
				CHAR chQuot = ch;
				while (pos < len) {
					ch = buf[pos];
					if (ch == '&') {
						if (pos > startAttrValue) {
							if (!(sb.addStatic(buf + startAttrValue, pos - startAttrValue))) {
								REPORT_ERROR(g_strError_memory_lack)
							}
						}
						pos++;
						unescapeEntity(&sb);
						if (flagError) {
							return;
						}
						startAttrValue = pos;
					} else if (ch == '<') {
						REPORT_ERROR(g_strError_content_include_lt)
					} else if (ch == chQuot) {
						if (pos > startAttrValue) {
							if (!(sb.addStatic(buf + startAttrValue, pos - startAttrValue))) {
								REPORT_ERROR(g_strError_memory_lack)
							}
						}
						pos++;
						flagEnded = sl_true;
						value = String::from(sb.merge());
						if (value.isNull()) {
							REPORT_ERROR(g_strError_memory_lack)
						}
						break;
					} else {
						pos++;
					}
				}
				if (!flagEnded) {
					REPORT_ERROR(g_strError_element_attr_not_end)
				}
			} else if (ch == 'R' && param.flagSupportCpp11String) {
				pos++;
				escapeWhiteSpaces();
				if (pos >= len) {
					REPORT_ERROR(g_strError_element_attr_required_quot)
				}
				if (buf[pos] != '\"') {
					REPORT_ERROR(g_strError_element_attr_required_quot)
				}
				pos++;
				sl_size posDelimiterBegin = pos;
				sl_size posDelimiterEnd = 0;
				while (pos < len) {
					if (buf[pos] == '(') {
						posDelimiterEnd = pos;
						pos++;
						break;
					}
					pos++;
				}
				if (pos >= len || !posDelimiterEnd) {
					REPORT_ERROR(g_strError_element_attr_not_end)
				}
				sl_size lenDelimiter = posDelimiterEnd - posDelimiterBegin;
				sl_size posValueBegin = pos;
				sl_size posValueEnd = 0;
				while (pos + lenDelimiter + 2 <= len) {
					if (buf[pos] == ')' && buf[pos + lenDelimiter + 1] == '\"' && Base::equalsMemory(buf + pos + 1, buf + posDelimiterBegin, lenDelimiter * sizeof(CHAR))) {
						posValueEnd = pos;
						pos += lenDelimiter + 2;
						break;
					}
					pos++;
				}
				if (!posValueEnd) {
					REPORT_ERROR(g_strError_element_attr_not_end)
				}
				value = String::create(buf + posValueBegin, posValueEnd - posValueBegin);
			} else {
				REPORT_ERROR(g_strError_element_attr_required_quot)
			}
		}

		template <class CHAR>
		void XmlParser<CHAR>::parseElement(XmlNodeGroup* parent, const String& _defNamespace, const HashMap<String, String>& _namespaces)
		{
			String defNamespace = _defNamespace;
			HashMap<String, String> namespaces = _namespaces;

			calcLineNumber();
			sl_size startLine = lineNumber;
			sl_size startColumn = columnNumber;
			sl_size posNameStart = pos;
			String name;
			parseName(name);
			if (flagError) {
				return;
			}
			sl_size lenName = pos - posNameStart;

			Ref<XmlElement> element = new XmlElement;
			if (element.isNull()) {
				REPORT_ERROR(g_strError_memory_lack)
			}

			CList<String> listPrefixMappings;
			sl_size indexAttr = 0;

			while (pos < len) {

				sl_size startWhiteSpace = pos;
				sl_size endWhiteSpace = pos;

				CHAR ch = buf[pos];

				if (ch != '>' && ch != '/') {
					if (SLIB_CHAR_IS_WHITE_SPACE(ch)) {
						pos++;
						escapeWhiteSpaces();
						endWhiteSpace = pos;
					} else {
						if (indexAttr) {
							REPORT_ERROR(g_strError_element_attr_end_with_invalid_char)
						} else {
							REPORT_ERROR(g_strError_name_invalid_char)
						}
					}
				}
				if (pos >= len) {
					REPORT_ERROR(g_strError_element_tag_not_end)
				}

				ch = buf[pos];
				if (ch == '>' || ch == '/') {
					break;
				} else {
					XmlAttribute attr;
					parseAttribute(attr.name, attr.value);
					if (flagError) {
						return;
					}
					if (element->containsAttribute(attr.name)) {
						REPORT_ERROR(g_strError_element_attr_duplicate)
					}
					processPrefix(attr.name, defNamespace, namespaces, attr.prefix, attr.uri, attr.localName);
					if (param.flagCreateWhiteSpaces) {
						if (endWhiteSpace > startWhiteSpace) {
							String ws = String::create(buf + startWhiteSpace, endWhiteSpace - startWhiteSpace);
							if (ws.isNull()) {
								REPORT_ERROR(g_strError_memory_lack)
							}
							attr.whiteSpacesBeforeName = ws;
						}
					}
					if (!(element->setAttribute(attr))) {
						REPORT_ERROR(g_strError_memory_lack)
					}
					if (param.flagProcessNamespaces) {
						if (attr.name == "xmlns") {
							defNamespace = attr.value;
							if (!(listPrefixMappings.add_NoLock(String::null()))) {
								REPORT_ERROR(g_strError_memory_lack)
							}
							CALL_CALLBACK(onStartPrefixMapping, element.get(), String::null(), defNamespace);
						} else if (attr.prefix == "xmlns" && attr.localName.isNotEmpty() && attr.value.isNotEmpty()) {
							if (namespaces == _namespaces) {
								namespaces = _namespaces.duplicate_NoLock();
							}
							if (!(namespaces.put_NoLock(attr.localName, attr.value))) {
								REPORT_ERROR(g_strError_memory_lack)
							}
							if (!(listPrefixMappings.add_NoLock(attr.localName))) {
								REPORT_ERROR(g_strError_memory_lack)
							}
							CALL_CALLBACK(onStartPrefixMapping, element.get(), attr.localName, attr.value)
						}
					}
				}

				indexAttr++;

			}

			if (pos >= len) {
				REPORT_ERROR(g_strError_element_tag_not_end)
			}
			sl_bool flagEmptyTag = sl_false;
			if (buf[pos] == '/') {
				if (pos + 1 < len && buf[pos + 1] == '>') {
					flagEmptyTag = sl_true;
					pos += 2;
				} else {
					REPORT_ERROR(g_strError_element_tag_not_end)
				}
			} else {
				pos++;
			}

			element->setSourceFilePath(param.sourceFilePath);
			element->setStartPositionInSource(posNameStart);
			element->setLineNumberInSource(startLine);
			element->setColumnNumberInSource(startColumn);
			element->setEndPositionInSource(pos);
			element->setStartContentPositionInSource(posNameStart);
			element->setEndContentPositionInSource(posNameStart);

			String prefix, uri, localName;
			processPrefix(name, defNamespace, namespaces, prefix, uri, localName);
			if (!(element->setName(name, uri, prefix, localName))) {
				REPORT_ERROR(g_strError_unknown)
			}

			if (parent) {
				if (!(parent->addChild(element))) {
					REPORT_ERROR(g_strError_memory_lack)
				}
			}
			CALL_CALLBACK(onStartElement, element.get(), element.get())
			if (!flagEmptyTag) {
				element->setStartContentPositionInSource(pos);
				parseNodes(parent ? element.get() : sl_null, defNamespace, namespaces);
				if (flagError) {
					return;
				}
				if (pos + 3 + lenName > len) {
					REPORT_ERROR(g_strError_element_tag_not_matching_end_tag)
				}
				if (buf[pos] != '<' || buf[pos + 1] != '/') {
					REPORT_ERROR(g_strError_element_tag_not_matching_end_tag)
				}
				element->setEndContentPositionInSource(pos);
				pos += 2;
				if (!(Base::equalsMemory(buf + posNameStart, buf + pos, lenName * sizeof(CHAR)))) {
					REPORT_ERROR(g_strError_element_tag_not_matching_end_tag)
				}
				pos += lenName;
				CHAR ch = buf[pos];
				if (ch != '>') {
					if (SLIB_CHAR_IS_WHITE_SPACE(ch)) {
						pos++;
						escapeWhiteSpaces();
					} else {
						REPORT_ERROR(g_strError_name_invalid_char)
					}
				}
				if (pos >= len) {
					REPORT_ERROR(g_strError_element_tag_not_end)
				}
				if (buf[pos] != '>') {
					REPORT_ERROR(g_strError_element_tag_not_end)
				}
				pos++;
			}
			element->setEndPositionInSource(pos);
			CALL_CALLBACK(onEndElement, element.get(), element.get());
			if (param.flagProcessNamespaces) {
				ListElements<String> prefixes(listPrefixMappings);
				for (sl_size i = 0; i < prefixes.count; i++) {
					CALL_CALLBACK(onEndPrefixMapping, element.get(), prefixes[i]);
				}
			}
		}

		template <class CHAR>
		void XmlParser<CHAR>::parseText(XmlNodeGroup* parent)
		{
			calcLineNumber();
			sl_size startLine = lineNumber;
			sl_size startColumn = columnNumber;
			sl_size startWhiteSpace = pos;
			escapeWhiteSpaces();
			if (pos > startWhiteSpace) {
				createWhiteSpace(parent, startWhiteSpace, pos);
				if (flagError) {
					return;
				}
			}
			sl_size startText = pos;
			StringBufferType _sb;
			StringBufferType* sb = param.flagCreateTextNodes ? &_sb : sl_null;
			while (pos < len) {
				CHAR ch = buf[pos];
				if (ch == '&') {
					if (sb) {
						if (pos > startText) {
							if (!(sb->addStatic(buf + startText, pos - startText))) {
								REPORT_ERROR(g_strError_memory_lack)
							}
						}
					}
					pos++;
					unescapeEntity(sb);
					if (flagError) {
						return;
					}
					startText = pos;
				} else if (ch == '<') {
					break;
				} else {
					pos++;
				}
			}
			if (sb) {
				if (pos > startText) {
					sl_size endText = pos - 1;
					while (endText >= startText) {
						CHAR ch = buf[endText];
						if (SLIB_CHAR_IS_WHITE_SPACE(ch)) {
							endText--;
						} else {
							break;
						}
					}
					if (endText >= startText) {
						if (!(sb->addStatic(buf + startText, endText - startText + 1))) {
							REPORT_ERROR(g_strError_memory_lack)
						}
					}
					startWhiteSpace = endText + 1;
				} else {
					startWhiteSpace = pos;
				}
				String text = String::from(sb->merge());
				if (text.isNull()) {
					REPORT_ERROR(g_strError_memory_lack)
				}
				if (text.isNotEmpty()) {
					if (parent) {
						Ref<XmlText> node = XmlText::create(text);
						if (node.isNull()) {
							REPORT_ERROR(g_strError_memory_lack)
						}
						node->setSourceFilePath(param.sourceFilePath);
						node->setStartPositionInSource(startText);
						node->setEndPositionInSource(pos);
						node->setLineNumberInSource(startLine);
						node->setColumnNumberInSource(startColumn);
						if (!(parent->addChild(node))) {
							REPORT_ERROR(g_strError_memory_lack)
						}
						CALL_CALLBACK(onText, node.get(), text)
					} else {
						CALL_CALLBACK(onText, sl_null, text)
					}
				}
				createWhiteSpace(parent, startWhiteSpace, pos);
				if (flagError) {
					return;
				}
			}
		}

		template <class CHAR>
		void XmlParser<CHAR>::parseNodes(XmlNodeGroup* parent, const String& defNamespace, const HashMap<String, String>& namespaces)
		{
			while (pos < len) {
				if (buf[pos] == '<') { // Element, Comment, PI, CDATA, DOCTYPE
					pos++;
					CHAR ch = buf[pos];
					if (ch == '!') { // Comment, CDATA, DOCTYPE
						pos++;
						if (pos + 1 < len && buf[pos] == '-' && buf[pos + 1] == '-') { // Comment
							pos += 2;
							parseComment(parent);
							if (flagError) {
								return;
							}
						} else if (pos + 6 < len && buf[pos] == '[' && buf[pos + 1] == 'C' && buf[pos + 2] == 'D' && buf[pos + 3] == 'A' && buf[pos + 4] == 'T' && buf[pos + 5] == 'A' && buf[pos + 6] == '[') { // CDATA
							pos += 7;
							parseCDATA(parent);
							if (flagError) {
								return;
							}
						} else if (pos + 7 < len && buf[pos] == 'D' && buf[pos + 1] == 'O' && buf[pos + 2] == 'C' && buf[pos + 3] == 'T' && buf[pos + 4] == 'Y' && buf[pos + 5] == 'P' && buf[pos + 6] == 'E' && SLIB_CHAR_IS_WHITE_SPACE(buf[pos + 7])) { // DOCTYPE
							pos += 7;
							escapeWhiteSpaces();
							parseDOCTYPE(parent);
							if (flagError) {
								return;
							}
						} else {
							REPORT_ERROR(g_strError_invalid_markup)
						}
					} else if (ch == '?') { // PI
						pos++;
						parsePI(parent);
						if (flagError) {
							return;
						}
					} else if (ch == '/') { // Element End Tag
						pos--;
						return;
					} else { // Element
						parseElement(parent, defNamespace, namespaces);
						if (flagError) {
							return;
						}
					}
				} else {
					parseText(parent);
					if (flagError) {
						return;
					}
				}
			}
		}

		template <class CHAR>
		void XmlParser<CHAR>::parse()
		{
			CALL_CALLBACK(onStartDocument, document.get(), document.get())
				parseNodes(document.get(), String::null(), HashMap<String, String>::null());
			if (flagError) {
				return;
			}
			if (pos < len) {
				REPORT_ERROR(g_strError_document_not_wellformed);
			}
			if (document.isNotNull() && param.flagCheckWellFormed) {
				if (!(document->checkWellFormed())) {
					REPORT_ERROR(g_strError_document_not_wellformed);
				}
			}
			CALL_CALLBACK(onEndDocument, document.get(), document.get())
		}

		template <class CHAR>
		Ref<XmlDocument> XmlParser<CHAR>::parse(const CHAR* buf, sl_size len, Xml::ParseParam& param)
		{
			param.flagError = sl_false;

			XmlParser<CHAR> parser;
			parser.buf = buf;
			parser.len = len;

			if (param.flagCreateDocument) {
				parser.document = XmlDocument::create();
				if (parser.document.isNull()) {
					parser.document->setStartPositionInSource(0);
					parser.document->setEndPositionInSource(len);
					param.flagError = sl_true;
					param.errorMessage = g_strError_memory_lack;
					return sl_null;
				}
			}
			parser.param = param;
			parser.control.source.data8 = (sl_char8*)(buf);
			parser.control.source.length = len;
			parser.control.characterSize = sizeof(CHAR);

			parser.parse();

			if (!(parser.flagError)) {
				return parser.document;
			}

			param.flagError = sl_true;
			param.errorPosition = parser.pos;
			param.errorMessage = parser.errorMessage;
			param.errorLine = Stringx::countLineNumber(StringViewType(buf, parser.pos), &(param.errorColumn));

			if (param.flagLogError) {
				LogError("Xml", param.getErrorText());
			}

			return sl_null;

		}

	}

	Ref<XmlDocument> Xml::parse(const sl_char8* str, sl_size len, ParseParam& param)
	{
		return XmlParser<sl_char8>::parse(str, len, param);
	}

	Ref<XmlDocument> Xml::parse(const sl_char16* str, sl_size len, ParseParam& param)
	{
		return XmlParser<sl_char16>::parse(str, len, param);
	}

	Ref<XmlDocument> Xml::parse(const sl_char32* str, sl_size len, ParseParam& param)
	{
		return XmlParser<sl_char32>::parse(str, len, param);
	}

	Ref<XmlDocument> Xml::parse(const sl_char8* sz, sl_size len)
	{
		ParseParam param;
		return parse(sz, len, param);
	}

	Ref<XmlDocument> Xml::parse(const sl_char16* str, sl_size len)
	{
		ParseParam param;
		return parse(str, len, param);
	}

	Ref<XmlDocument> Xml::parse(const sl_char32* str, sl_size len)
	{
		ParseParam param;
		return parse(str, len, param);
	}

	Ref<XmlDocument> Xml::parse(const StringParam& _xml, ParseParam& param)
	{
		if (_xml.isEmpty()) {
			return sl_null;
		}
		if (_xml.is8BitsStringType()) {
			StringData xml(_xml);
			return XmlParser<sl_char8>::parse(xml.getData(), xml.getLength(), param);
		} else if (_xml.is16BitsStringType()) {
			StringData16 xml(_xml);
			return XmlParser<sl_char16>::parse(xml.getData(), xml.getLength(), param);
		} else {
			StringData32 xml(_xml);
			return XmlParser<sl_char32>::parse(xml.getData(), xml.getLength(), param);
		}
	}

	Ref<XmlDocument> Xml::parse(const StringParam& xml)
	{
		ParseParam param;
		return parse(xml, param);
	}

	Ref<XmlDocument> Xml::parse(const MemoryView& utf, ParseParam& param)
	{
		if (!(utf.size)) {
			return sl_null;
		}
		return parse(StringParam::fromUtf(utf), param);
	}

	Ref<XmlDocument> Xml::parse(const MemoryView& utf)
	{
		if (!(utf.size)) {
			return sl_null;
		}
		ParseParam param;
		return parse(utf, param);
	}

	Ref<XmlDocument> Xml::parseTextFile(const StringParam& filePath, ParseParam& param)
	{
		if (param.sourceFilePath.isNull()) {
			param.sourceFilePath = filePath.toString();
		}
		return parse(File::readAllText(filePath), param);
	}

	Ref<XmlDocument> Xml::parseTextFile(const StringParam& filePath)
	{
		ParseParam param;
		return parseTextFile(filePath, param);
	}


	String Xml::encodeTextToEntities(const String& text)
	{
		StringBuffer buf;
		if (encodeTextToEntities(text, buf)) {
			return buf.merge();
		}
		return sl_null;
	}

	sl_bool Xml::encodeTextToEntities(const String& text, StringBuffer& output)
	{
		StringStorage data(text);
		StringStorage dataEscape;
		dataEscape.charSize = 1;
		sl_char8* sz = data.data8;
		sl_size len = data.length;
		sl_size start = 0;
		for (sl_size i = 0; i < len; i++) {
			sl_char16 ch = sz[i];
			sl_bool flagEscape = sl_false;
			if (ch == '<') {
				flagEscape = sl_true;
				dataEscape.data8 = (char*)"&lt;";
				dataEscape.length = 4;
			} else if (ch == '>') {
				flagEscape = sl_true;
				dataEscape.data8 = (char*)"&gt;";
				dataEscape.length = 4;
			} else if (ch == '&') {
				flagEscape = sl_true;
				dataEscape.data8 = (char*)"&amp;";
				dataEscape.length = 5;
			} else if (ch == '\'') {
				flagEscape = sl_true;
				dataEscape.data8 = (char*)"&apos;";
				dataEscape.length = 6;
			} else if (ch == '\"') {
				flagEscape = sl_true;
				dataEscape.data8 = (char*)"&quot;";
				dataEscape.length = 6;
			}
			if (flagEscape) {
				if (i > start) {
					data.data8 = sz + start;
					data.length = i - start;
					if (!(output.add(data))) {
						return sl_false;
					}
				}
				start = i + 1;
				if (!(output.add(dataEscape))) {
					return sl_false;
				}
			}
		}
		if (len > start) {
			data.data8 = sz + start;
			data.length = len - start;
			if (!(output.add(data))) {
				return sl_false;
			}
		}
		return sl_true;
	}

	String Xml::decodeTextFromEntities(const StringView& text)
	{
		String ret = String::allocate(text.getLength());
		if (ret.isNull()) {
			return sl_null;
		}

		sl_char8* buf = text.getData();
		sl_char8* output = ret.getData();
		sl_size len = text.getLength();
		sl_size pos = 0;

		sl_size posOutput = 0;

		while (pos < len) {
			if (buf[pos] == '&' && pos + 1 < len) {
				pos++;
				if (pos + 2 < len && buf[pos] == 'l' && buf[pos+1] == 't' && buf[pos+2] == ';') {
					output[posOutput] = '<';
					pos += 3;
					posOutput++;
				} else if (pos + 2 < len && buf[pos] == 'g' && buf[pos+1] == 't' && buf[pos+2] == ';') {
					output[posOutput] = '>';
					pos += 3;
					posOutput++;
				} else if (pos + 3 < len && buf[pos] == 'a' && buf[pos+1] == 'm' && buf[pos+2] == 'p' && buf[pos+3] == ';') {
					output[posOutput] = '&';
					pos += 4;
					posOutput++;
				} else if (pos + 4 < len && buf[pos] == 'a' && buf[pos+1] == 'p' && buf[pos+2] == 'o' && buf[pos+3] == 's' && buf[pos+4] == ';') {
					output[posOutput] = '\'';
					pos += 5;
					posOutput++;
				} else if (pos + 4 < len && buf[pos] == 'q' && buf[pos+1] == 'u' && buf[pos+2] == 'o' && buf[pos+3] == 't' && buf[pos+4] == ';') {
					output[posOutput] = '\"';
					pos += 5;
					posOutput++;
				} else if (pos + 2 < len && buf[pos] == '#'){
					sl_size posOld = pos;
					do {
						pos++;
						sl_uint32 n;
						sl_reg parseRes;
						if (buf[pos] == 'x') {
							pos++;
							parseRes = String::parseUint32(16, &n, buf, pos, len);
						} else {
							parseRes = String::parseUint32(10, &n, buf, pos, len);
						}
						if (parseRes != SLIB_PARSE_ERROR) {
							pos = parseRes;
							if (pos < len && buf[pos] == ';') {
								sl_char32 _n = (sl_char32)n;
								String s = String::create(&_n, 1);
								if (s.isNotEmpty()) {
									sl_size t = s.getLength();
									Base::copyMemory(output + posOutput, s.getData(), t);
									posOutput += t;
									pos++;
									break;
								}
							}
						}
						pos = posOld;
						output[posOutput] = '&';
						posOutput++;
					} while (0);
				} else {
					output[posOutput] = '&';
					posOutput++;
				}
			} else {
				output[posOutput] = buf[pos];
				posOutput++;
				pos++;
			}
		}

		output[posOutput] = 0;
		ret.setLength(posOutput);

		return ret;
	}

	namespace {
		template <class CHAR>
		static sl_bool CheckName(const CHAR* str, sl_size len)
		{
			if (!len) {
				return sl_false;
			}
			sl_uint32 ch = (sl_uint32)(*str);
			if (ch < 128 && g_patternCheckName[ch] != 1) {
				return sl_false;
			}
			for (sl_size i = 1; i < len; i++) {
				ch = (sl_uint32)(str[i]);
				if (ch < 128 && !(g_patternCheckName[ch])) {
					return sl_false;
				}
			}
			return sl_true;
		}
	}

	sl_bool Xml::checkName(const sl_char8* sz, sl_size len)
	{
		return CheckName(sz, len);
	}

	sl_bool Xml::checkName(const sl_char16* sz, sl_size len)
	{
		return CheckName(sz, len);
	}

	sl_bool Xml::checkName(const sl_char32* sz, sl_size len)
	{
		return CheckName(sz, len);
	}

	sl_bool Xml::checkName(const String& tagName)
	{
		return checkName(tagName.getData(), tagName.getLength());
	}

}
