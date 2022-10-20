/*
 *   Copyright (c) 2008-2021 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_CORE_XML
#define CHECKHEADER_SLIB_CORE_XML

/************************************************************
 
			XML (Extensible Markup Language)
 
 XML 1.0 => https://www.w3.org/TR/REC-xml/
			https://www.w3.org/TR/2008/REC-xml-20081126/
 XML 1.1 => http://www.w3.org/TR/2006/REC-xml11-20060816/
 
 
 Supports DOM & SAX parsers
 
************************************************************/

#include "variant.h"

namespace slib
{
	
	class XmlNodeGroup;
	class XmlDocument;
	class XmlElement;
	class XmlText;
	class XmlProcessingInstruction;
	class XmlComment;
	class XmlParseControl;
	class StringBuffer;

	enum class XmlNodeType
	{
		Document = 1,
		Element = 2,
		Text = 3,
		ProcessingInstruction = 4,
		Comment = 5,
		WhiteSpace = 6
	};
	
	class SLIB_EXPORT XmlNode : public Referable
	{
		SLIB_DECLARE_OBJECT

	public:
		XmlNode(XmlNodeType type);
		
		~XmlNode();

	public:
		XmlNodeType getType() const;

		virtual sl_bool buildText(StringBuffer& output) const = 0;

		virtual sl_bool buildXml(StringBuffer& output) const = 0;

		virtual String getText() const;

		String toString() const;

		sl_bool isDocumentNode() const;

		Ref<XmlDocument> toDocumentNode() const;

		sl_bool isElementNode() const;

		Ref<XmlElement> toElementNode() const;

		sl_bool isTextNode() const;

		Ref<XmlText> toTextNode() const;

		sl_bool isProcessingInstructionNode() const;

		Ref<XmlProcessingInstruction> toProcessingInstructionNode() const;

		sl_bool isCommentNode() const;

		Ref<XmlComment> toCommentNode() const;

		Ref<XmlDocument> getDocument() const;

		Ref<XmlElement> getRoot() const;

		Ref<XmlNodeGroup> getParent() const;

		Ref<XmlElement> getParentElement() const;

		String getSourceFilePath() const;

		void setSourceFilePath(const String& path);

		sl_size getStartPositionInSource() const;

		void setStartPositionInSource(sl_size pos);

		sl_size getEndPositionInSource() const;

		void setEndPositionInSource(sl_size pos);

		sl_size getLineNumberInSource() const;

		void setLineNumberInSource(sl_size line);

		sl_size getColumnNumberInSource() const;

		void setColumnNumberInSource(sl_size line);
	
	protected:
		XmlNodeType m_type;
		WeakRef<XmlNodeGroup> m_parent;
		WeakRef<XmlDocument> m_document;
		AtomicString m_sourceFilePath;
		sl_size m_positionStartInSource;
		sl_size m_positionEndInSource;
		sl_size m_lineInSource;
		sl_size m_columnInSource;
	
	protected:
		void _setDocument(const Ref<XmlDocument>& documentNew);

		friend class XmlNodeGroup;

	};
	
	class SLIB_EXPORT XmlNodeGroup : public XmlNode
	{
		SLIB_DECLARE_OBJECT

	public:
		XmlNodeGroup(XmlNodeType type);
		
		~XmlNodeGroup();

	public:
		sl_bool buildText(StringBuffer& output) const override;

		sl_bool buildInnerXml(StringBuffer& output) const;

		String getInnerXml() const;
	
		sl_size getChildCount() const;

		Ref<XmlNode> getChild(sl_size index) const;

		sl_bool addChild(const Ref<XmlNode>& node);
	
		sl_bool removeChild(const Ref<XmlNode>& node, sl_bool flagUnregisterDocument = sl_true);

		void removeAllChildren(sl_bool flagUnregisterDocument = sl_true);

		String getChildText(sl_size index) const;

		Ref<XmlElement> getChildElement(sl_size index) const;

		List< Ref<XmlElement> > getChildElements() const;

		sl_size getChildElementCount() const;

		List< Ref<XmlElement> > getChildElements(const StringView& tagName) const;

		List< Ref<XmlElement> > getChildElements(const StringView& uri, const StringView& localName) const;

		Ref<XmlElement> getFirstChildElement() const;

		Ref<XmlElement> getFirstChildElement(const StringView& tagName) const;

		Ref<XmlElement> getFirstChildElement(const StringView& uri, const StringView& localName) const;

		String getFirstChildElementText() const;

		String getFirstChildElementText(const StringView& tagName) const;

		String getFirstChildElementText(const StringView& uri, const StringView& localName) const;

		List< Ref<XmlElement> > getDescendantElements(const StringView& tagName) const;

		void getDescendantElements(const StringView& tagName, List< Ref<XmlElement> >& list) const;

		List< Ref<XmlElement> > getDescendantElements(const StringView& uri, const StringView& localName) const;

		void getDescendantElements(const StringView& uri, const StringView& localName, List< Ref<XmlElement> >& list) const;

		Ref<XmlElement> getFirstDescendantElement(const StringView& tagName) const;

		Ref<XmlElement> getFirstDescendantElement(const StringView& uri, const StringView& localName) const;

		String getFirstDescendantElementText(const StringView& tagName) const;

		String getFirstDescendantElementText(const StringView& uri, const StringView& localName) const;

		Ref<XmlElement> findChildElementByAttribute(const StringView& name, const StringView& value) const;

		Ref<XmlElement> findChildElementById(const StringView& _id) const;
	
	protected:
		CList< Ref<XmlNode> > m_children;
	
		friend class XmlDocument;

	};
	
	class SLIB_EXPORT XmlAttribute
	{
	public:
		String name;
		String uri;
		String localName;
		String value;
		String whiteSpacesBeforeName;
		
	public:
		XmlAttribute() noexcept;
		
		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(XmlAttribute)
		
	};
	
	class SLIB_EXPORT XmlElement : public XmlNodeGroup
	{
		SLIB_DECLARE_OBJECT

	public:
		XmlElement();
		
		~XmlElement();

	public:
		static Ref<XmlElement> create(const String& name);

		static Ref<XmlElement> create(const String& name, const String& uri, const String& localName);

		sl_bool buildXml(StringBuffer& output) const override;
	
		String getName() const;

		String getUri() const;

		String getLocalName() const;
	
		sl_bool setName(const String& name);

		sl_bool setName(const String& name, const String& uri, const String& localName);

		sl_size getAttributeCount() const;

		sl_bool getAttribute(sl_size index, XmlAttribute* _out = sl_null) const;

		String getAttribute(const String& name) const;

		String getAttributeIgnoreCase(const StringView& name) const;

		String getAttribute(const StringView& uri, const StringView& localName) const;

		String getAttributeIgnoreCase(const StringView& uri, const StringView& localName) const;

		sl_bool containsAttribute(const String& name) const;

		sl_bool containsAttributeIgnoreCase(const StringView& name) const;

		sl_bool setAttribute(sl_size index, const String& value);

		sl_bool setAttribute(sl_size index, const String& uri, const String& localName, const String& value);

		sl_bool setAttribute(const String& name, const String& value);

		sl_bool setAttribute(const XmlAttribute& attr);

		sl_bool setAttribute(const StringView& uri, const StringView& localName, const String& value);

		sl_bool removeAttribute(sl_size index);

		sl_bool removeAttribute(const String& name);

		void removeAllAttributes();
		
		sl_size getStartContentPositionInSource() const;
		
		void setStartContentPositionInSource(sl_size pos);

		sl_size getEndContentPositionInSource() const;
		
		void setEndContentPositionInSource(sl_size pos);

	protected:
		Atomic<String> m_name;
		Atomic<String> m_uri;
		Atomic<String> m_localName;
		List<XmlAttribute> m_attributes;
		HashMap<String, String> m_mapAttributes;
		Mutex m_lockAttributes;
		sl_size m_positionStartContentInSource;
		sl_size m_positionEndContentInSource;

	protected:
		friend class XmlNode;
		friend class XmlNodeGroup;

	};
	
	class SLIB_EXPORT XmlDocument : public XmlNodeGroup
	{
		SLIB_DECLARE_OBJECT

	public:
		XmlDocument();
		
		~XmlDocument();

	public:
		static Ref<XmlDocument> create();

		sl_bool buildXml(StringBuffer& output) const override;
	
		Ref<XmlElement> getElementById(const StringView& _id) const;

		void registerElementsById(const StringView& idAttributeName);

		void registerElementsById(const Ref<XmlElement>& element, const StringView& idAttributeName);

		sl_bool checkWellFormed() const;

	protected:
		HashMap< String, WeakRef<XmlElement> > m_elementsById;

	};
	
	class SLIB_EXPORT XmlText : public XmlNode
	{
		SLIB_DECLARE_OBJECT

	public:
		XmlText();
		
		~XmlText();

	public:
		static Ref<XmlText> create(const String& text, sl_bool flagCDATA = sl_false);
	
		static Ref<XmlText> createCDATA(const String& text);

		sl_bool buildText(StringBuffer& output) const override;

		sl_bool buildXml(StringBuffer& output) const override;
	
		String getText() const override;

		void setText(const String& text);

		sl_bool isCDATA() const;

		void setCDATA(sl_bool flag);

	protected:
		Atomic<String> m_text;
		sl_bool m_flagCDATA;
	
	};
	
	class SLIB_EXPORT XmlProcessingInstruction : public XmlNode
	{
		SLIB_DECLARE_OBJECT

	public:
		XmlProcessingInstruction();
		
		~XmlProcessingInstruction();

	public:
		static Ref<XmlProcessingInstruction> create(const String& target, const String& content);

		sl_bool buildText(StringBuffer& output) const override;

		sl_bool buildXml(StringBuffer& output) const override;

		String getTarget() const;

		sl_bool setTarget(const String& target);

		String getContent() const;

		void setContent(const String& content);
	
	protected:
		Atomic<String> m_target;
		Atomic<String> m_content;
	
	};
	
	class SLIB_EXPORT XmlComment : public XmlNode
	{
		SLIB_DECLARE_OBJECT

	public:
		XmlComment();
		
		~XmlComment();

	public:
		static Ref<XmlComment> create(const String& comment);

		sl_bool buildText(StringBuffer& output) const override;

		sl_bool buildXml(StringBuffer& output) const override;

		String getComment() const;

		void setComment(const String& comment);

	protected:
		Atomic<String> m_comment;

	};
	
	class SLIB_EXPORT XmlWhiteSpace : public XmlNode
	{
		SLIB_DECLARE_OBJECT

	public:
		XmlWhiteSpace();
		
		~XmlWhiteSpace();

	public:
		static Ref<XmlWhiteSpace> create(const String& content);

		sl_bool buildText(StringBuffer& output) const override;

		sl_bool buildXml(StringBuffer& output) const override;

		String getContent() const;

		void setContent(const String& comment);

	protected:
		Atomic<String> m_content;

	};
	
	class SLIB_EXPORT XmlParseControl
	{
	public:
		// read & write
		StringStorage source;

		sl_uint32 characterSize;

		// write only
		sl_bool flagChangeSource;
	
		// read & write
		sl_size parsingPosition;

		// write only
		sl_bool flagStopParsing;

		// read only
		XmlNode* currentNode;

	public:
		XmlParseControl();
		
		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(XmlParseControl)

	};
	
	class SLIB_EXPORT XmlParseParam
	{
	public:
		// in
		sl_bool flagCreateDocument;
		// in
		sl_bool flagCreateCommentNodes;
		// in
		sl_bool flagCreateProcessingInstructionNodes;
		// in
		sl_bool flagCreateTextNodes;
		// in
		sl_bool flagCreateWhiteSpaces;

		// in
		sl_bool flagProcessNamespaces;
		// in
		sl_bool flagCheckWellFormed;
		// in
		sl_bool flagSupportCpp11String;

		// in, callbacks
		Function<void(XmlParseControl*, XmlDocument*)> onStartDocument;
		Function<void(XmlParseControl*, XmlDocument*)> onEndDocument;
		Function<void(XmlParseControl*, XmlElement*)> onStartElement;
		Function<void(XmlParseControl*, XmlElement*)> onEndElement;
		Function<void(XmlParseControl*, const String& text)> onText;
		Function<void(XmlParseControl*, const String& text)> onCDATA;
		Function<void(XmlParseControl*, const String& target, const String& content)> onProcessingInstruction;
		Function<void(XmlParseControl*, const String& content)> onComment;
		Function<void(XmlParseControl*, const String& prefix, const String& uri)> onStartPrefixMapping;
		Function<void(XmlParseControl*, const String& prefix)> onEndPrefixMapping;

		// in
		sl_bool flagLogError;
		// in
		String sourceFilePath;

		// out
		sl_bool flagError;
		// out
		sl_size errorPosition;
		// out
		sl_size errorLine;
		// out
		sl_size errorColumn;
		// out
		String errorMessage;

	public:
		XmlParseParam();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(XmlParseParam)

	public:
		String getErrorText();

		void setCreatingAll();

		void setCreatingOnlyElements();

		void setCreatingOnlyElementsAndTexts();

	};
	
	/**
	 * @class Xml
	 * @brief provides utilities for parsing and build XML.
	 */
	class SLIB_EXPORT Xml
	{
	public:
		/**
		 * parses XML text contained in `xml`
		 *
		 * @param[in] xml String value containing XML text
		 * @param[in] length length of the XML text
		 * @param[in] param options for XML parsing
		 *
		 * @return XmlDocument object on success
		 * @return nullptr on failure
		 */
		static Ref<XmlDocument> parse(const sl_char8* xml, sl_size length, XmlParseParam& param);
		static Ref<XmlDocument> parse(const sl_char16* xml, sl_size length, XmlParseParam& param);
		static Ref<XmlDocument> parse(const sl_char32* xml, sl_size length, XmlParseParam& param);

		/**
		 * parses XML text contained in `xml`.
		 * XML parser uses default option for parsing.
		 *
		 * @param[in] xml String value containing XML text
		 * @param[in] length length of the XML text
		 *
		 * @return XmlDocument object on success
		 * @return nullptr on failure
		 */
		static Ref<XmlDocument> parse(const sl_char8* xml, sl_size length);
		static Ref<XmlDocument> parse(const sl_char16* xml, sl_size length);
		static Ref<XmlDocument> parse(const sl_char32* xml, sl_size length);
		static Ref<XmlDocument> parse(const StringParam& xml, XmlParseParam& param);
		static Ref<XmlDocument> parse(const StringParam& xml);

		/**
		 * parses XML text file located in `filePath`.
		 * The character positions are calculated by UTF-16 encoding.
		 *
		 * @param[in] filePath XML text file (UTF-8, UTF-16BE, UTF-16LE)
		 * @param[in] param options for XML parsing
		 *
		 * @return XmlDocument object on success
		 * @return nullptr on failure
		 */
		static Ref<XmlDocument> parseTextFile(const StringParam& filePath, XmlParseParam& param);

		/**
		 * parses XML text file located in `filePath`.
		 * The character positions are calculated by UTF-16 encoding.
		 * XML parser uses default option for parsing.
		 *
		 * @param[in] filePath XML text file (UTF-8, UTF-16BE, UTF-16LE)
		 *
		 * @return XmlDocument object on success
		 * @return nullptr on failure
		 */
		static Ref<XmlDocument> parseTextFile(const StringParam& filePath);


		/**
		 * Encodes speical characters (&lt; &gt; &amp; &quot; &apos;) to XML entities.
		 *
		 * @param[in] text String value containing the original text
		 *
		 * @return Encoded result text with XML entities for special characters
		 */
		static String encodeTextToEntities(const String& text);

		/**
		 * Encodes speical characters (&lt; &gt; &amp; &quot; &apos;) to XML entities.
		 * Encoded result text will be stored in `output` buffer.
		 *
		 * @param[in] text String value containing the original text
		 * @param[out] output StringBuffer that receives the encoded result text
		 *
		 * @return `true` on success
		 */
		static sl_bool encodeTextToEntities(const String& text, StringBuffer& output);
		
		/**
		 * Decodes XML entities (&amp;lt; &amp;gt; &amp;amp; ...) contained in `text`.
		 *
		 * @param[in] text String value that may contain XML entities.
		 *
		 * @return decoded text
		 */
		static String decodeTextFromEntities(const StringView& text);

		/**
		 * Check the `name` can be used as XML tag name
		 *
		 * @param[in] name XML tag name
		 * @param[in] len length of the tag name
		 *
		 * @return `true` if the `name` is valid for XML tag name
		 */
		static sl_bool checkName(const sl_char8* name, sl_size len);
		static sl_bool checkName(const sl_char16* name, sl_size len);
		static sl_bool checkName(const sl_char32* name, sl_size len);
		static sl_bool checkName(const String& name);

	};

}

#endif
