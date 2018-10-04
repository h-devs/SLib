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

#ifndef CHECKHEADER_SLIB_GRAPHICS_CSS
#define CHECKHEADER_SLIB_GRAPHICS_CSS

#include "definition.h"

#include "../data/xml.h"

/*
	CSS - Cascading Style Sheets
*/

namespace slib
{

	enum class CascadingStyleCombinator
	{
		None = 0,
		Descendant = 1, // A B
		Child = 2, // A>B
		Sibling = 3, // A~B
		Adjacent = 4 // A+B
	};

	enum class CascadingStyleMatchType
	{
		Exist = 0, // [attr]: elements with an attribute name of `attr`
		Equal = 1, // [attr="value"]: elements with an attribute name of `attr` whose value is exactly `value`
		Contains_Word = 2, // [attr~="value"]: elements with an attribute name of `attr` whose value is a whitespace-separated list of words, one of which is exactly `value`
		LocalePrefix = 3, // [attr|="value"]: elements with an attribute name of `attr` whose value can be exactly `value` or can begin with `value` immediately followed by a hyphen(-). It is often used for language subcode matches
		Start = 4, // [attr^="value"]: elements with an attribute name of `attr` whose value is prefixed (preceded) by `value`
		End = 5, // [attr$="value"]: elements with an attribute name of `attr` whose value is suffixed (followed) by `value`
		Contain = 6, // [attr*="value"]: elements with an attribute name of `attr` whose value contains at least one occurrence of `value` within the string
	};

	enum class CascadingStyleValueType
	{
		Normal = 0,
		Variable = 1, // var(--name[, default])
	};

	class StringBuffer;

	class SLIB_EXPORT CascadingStyleValue : public Referable
	{
	public:
		CascadingStyleValue(CascadingStyleValueType type);

		~CascadingStyleValue();

	public:
		CascadingStyleValueType getType()
		{
			return m_type;
		}

		sl_bool isImportant()
		{
			return m_flagImportant;
		}

		void setImportant(sl_bool flag = sl_true)
		{
			m_flagImportant = flag;
		}

		virtual sl_bool toString(StringBuffer& output) = 0;

		String toString();

	protected:
		sl_bool toString_Suffix(StringBuffer& output);

	protected:
		CascadingStyleValueType m_type;
		sl_bool m_flagImportant : 1; // !important

	};

	class SLIB_EXPORT CascadingStyleNormalValue : public CascadingStyleValue
	{
	public:
		CascadingStyleNormalValue(String&& value);

		~CascadingStyleNormalValue();

	public:
		const String& getValue()
		{
			return m_value;
		}

		sl_bool toString(StringBuffer& output) override;

	protected:
		String m_value;

	};

	class SLIB_EXPORT CascadingStyleVariableValue : public CascadingStyleValue
	{
	public:
		CascadingStyleVariableValue(String&& name, String&& defaultValue);

		CascadingStyleVariableValue(String&& name);

		~CascadingStyleVariableValue();

	public:
		const String& getName()
		{
			return m_name;
		}

		const String& getDefaultValue()
		{
			return m_defaultValue;
		}

		sl_bool toString(StringBuffer& output) override;

	protected:
		String m_name;
		String m_defaultValue;

	};

	class SLIB_EXPORT CascadingStyleAttributeMatch
	{
	public:
		CascadingStyleMatchType type;
		String name;
		String value;
		sl_bool flagIgnoreCase;

	public:
		CascadingStyleAttributeMatch(): type(CascadingStyleMatchType::Exist), flagIgnoreCase(sl_false) {}
		
		template <class NAME>
		CascadingStyleAttributeMatch(CascadingStyleMatchType _type, NAME&& _name): type(_type), name(Forward<NAME>(_name)), flagIgnoreCase(sl_false) {}

		template <class NAME, class VALUE>
		CascadingStyleAttributeMatch(CascadingStyleMatchType _type, NAME&& _name, VALUE&& _value, sl_bool _flagIgnoreCase = sl_false): type(_type), name(Forward<NAME>(_name)), value(Forward<VALUE>(_value)), flagIgnoreCase(_flagIgnoreCase) {}
		
		SLIB_DEFINE_CLASS_DEFAULT_MEMBERS_INLINE(CascadingStyleAttributeMatch)

	};

	class SLIB_EXPORT CascadingStyleSelector : public Referable
	{
	public:
		String namespaceName;
		sl_bool flagNamespace;
		String elementName;
		sl_bool flagUniversal;
		String id;
		List<String> classNames;
		List<CascadingStyleAttributeMatch> attributes;
		List<String> pseudoClasses;
		String pseudoElement;
		CascadingStyleCombinator combinator; // for before
		Ref<CascadingStyleSelector> before;

	public:
		CascadingStyleSelector();

		~CascadingStyleSelector();

	public:
		sl_bool matchElement(const Ref<XmlElement>& element);

		sl_bool toString(StringBuffer& output);

	};

	class CascadingStyleRule;
	class CascadingStyleAtRule;

	class SLIB_EXPORT CascadingStyleStatements
	{
	public:
		List<CascadingStyleRule> rules;
		List<CascadingStyleAtRule> atRules;

	public:
		CascadingStyleStatements();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(CascadingStyleStatements)

	public:
		sl_bool toString(StringBuffer& output, sl_uint32 tabLevel);

	};

	typedef HashMap< String, Ref<CascadingStyleValue> > CascadingStyleDeclarations;

	class SLIB_EXPORT CascadingStyleRule
	{
	public:
		Ref<CascadingStyleSelector> selector;
		CascadingStyleDeclarations declarations;

	public:
		sl_bool toString(StringBuffer& output, sl_uint32 tabLevel);

	};

	class SLIB_EXPORT CascadingStyleAtRule
	{
	public:
		String identifier;
		String rule;
		CascadingStyleDeclarations declarations;
		CascadingStyleStatements statements;

	public:
		sl_bool toString(StringBuffer& output, sl_uint32 tabLevel);

	};

	class CascadingStylesParam
	{
	public:
		// Input
		sl_bool flagIgnoreErrors;

		// Output
		sl_bool flagError;

	public:
		CascadingStylesParam();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(CascadingStylesParam)

	};

	class SLIB_EXPORT CascadingStyleSheet
	{
	public:
		CascadingStyleSheet();

		~CascadingStyleSheet();

	public:
		void addStyles(const StringParam& styles, CascadingStylesParam& param);

		sl_bool addStyles(const StringParam& styles);

		sl_bool toString(StringBuffer& output);

		String toString();

		CascadingStyleDeclarations getElementDeclarations(const Ref<XmlElement>& element);

		static CascadingStyleDeclarations parseDeclarations(const StringParam& input);

		static void mergeDeclarations(CascadingStyleDeclarations& to, const CascadingStyleDeclarations& from);

		static String getDeclarationValue(const CascadingStyleDeclarations& decls, const String& key);

		static sl_bool writeDeclarationsString(StringBuffer& _out, const CascadingStyleDeclarations& decls, sl_uint32 tabLevel);

	protected:
		CascadingStyleStatements m_statements;

	};

}

#endif
