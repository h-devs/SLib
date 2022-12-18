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
		Exist = 0, // [attr]: E` elements with an attribute name of `attr`
		Equal = 1, // [attr="value"]: elements with an attribute name of `attr` whose value is exactly `value`
		Contains_Word = 2, // [attr~="value"]: elements with an attribute name of `attr` whose value is a whitespace-separated list of words, one of which is exactly `value`
		Contains_WordHyphen = 3, // [attr|="value"]: elements with an attribute name of `attr` whose value can be exactly `value` or can begin with `value` immediately followed by a hyphen(-). It is often used for language subcode matches
		Start = 4, // [attr^="value"]: elements with an attribute name of `attr` whose value is prefixed (preceded) by `value`
		End = 5, // [attr$="value"]: elements with an attribute name of `attr` whose value is suffixed (followed) by `value`
		Contain = 6, // [attr*="value"]: elements with an attribute name of `attr` whose value contains at least one occurrence of `value` within the string
	};

	class SLIB_EXPORT CascadingStyleValue
	{
	public:
		String value;
		sl_bool flagVariable : 1;
		sl_bool flagImportant : 1;

	public:
		CascadingStyleValue(): flagVariable(sl_false), flagImportant(sl_false) {}

		template <class VALUE>
		CascadingStyleValue(VALUE&& _value, sl_bool _flagVariable = sl_false, sl_bool _flagImportant = sl_false): value(Forward<VALUE>(_value)), flagVariable(_flagVariable), flagImportant(_flagImportant) {}

		SLIB_DEFINE_CLASS_DEFAULT_MEMBERS_INLINE(CascadingStyleValue)

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
		String className;
		String id;
		List<CascadingStyleAttributeMatch> attributes;
		List<String> pseudoClasses;
		List<String> pseudoElements;
		Ref<CascadingStyleSelector> parent;
		CascadingStyleCombinator combinator; // for parent

	public:
		CascadingStyleSelector();

		~CascadingStyleSelector();

	};

	class SLIB_EXPORT CascadingStyleRule
	{
	public:
		Ref<CascadingStyleSelector> selector;
		HashMap<String, CascadingStyleValue> properties;
	};

	class SLIB_EXPORT CascadingStyleSheet
	{
	public:
		CascadingStyleSheet();

		~CascadingStyleSheet();

	public:
		sl_bool addStyles(const StringParam& styles);

	protected:
		CHashMap< String, List<CascadingStyleRule> > m_rulesById;
		CHashMap< String, List<CascadingStyleRule> > m_rulesByClass;
		CHashMap< String, List<CascadingStyleRule> > m_rulesByElement;
		CHashMap< String, List<CascadingStyleRule> > m_rulesByOther;

	};

}

#endif
