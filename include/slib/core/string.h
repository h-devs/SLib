/*
 *   Copyright (c) 2008-2018 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_CORE_STRING
#define CHECKHEADER_SLIB_CORE_STRING

#include "charset.h"
#include "ref.h"
#include "default_members.h"

#ifdef SLIB_SUPPORT_STD_TYPES
#include <string>
#endif

#define SLIB_CHAR_IS_ALPHA(c) (((c) >= 'A' && (c) <= 'Z') || ((c) >= 'a' && (c) <= 'z'))
#define SLIB_CHAR_IS_ALPHA_UPPER(c) ((c) >= 'A' && (c) <= 'Z')
#define SLIB_CHAR_IS_ALPHA_LOWER(c) ((c) >= 'a' && (c) <= 'z')
#define SLIB_CHAR_IS_DIGIT(c) ((c) >= '0' && (c) <= '9')
#define SLIB_CHAR_IS_ALNUM(c) (((c) >= '0' && (c) <= '9') || ((c) >= 'A' && (c) <= 'Z') || ((c) >= 'a' && (c) <= 'z'))
#define SLIB_CHAR_IS_HEX(c) (((c) >= '0' && (c) <= '9') || ((c) >= 'A' && (c) <= 'F') || ((c) >= 'a' && (c) <= 'f'))
#define SLIB_CHAR_IS_WHITE_SPACE(c) (c == ' ' || c == '\t' || c == '\r' || c == '\n')
#define SLIB_CHAR_IS_SPACE_TAB(c) (c == ' ' || c == '\t')
#define SLIB_CHAR_IS_C_NAME(c) (((c) >= '0' && (c) <= '9') || ((c) >= 'A' && (c) <= 'Z') || ((c) >= 'a' && (c) <= 'z') || c == '_')

#define SLIB_CHAR_DIGIT_TO_INT(c) (((c) >= '0' && (c) <= '9') ? ((c) - '0') : 10)
#define SLIB_CHAR_HEX_TO_INT(c) (((c) >= '0' && (c) <= '9') ? ((c) - '0') : (((c) >= 'A' && (c) <= 'F') ? ((c) -  55) : ((c) >= 'a' && (c) <= 'f') ? ((c) -  87) : 16))
#define SLIB_CHAR_UPPER_TO_LOWER(c) (((c) >= 'A' && (c) <= 'Z')?((c) + ('a' - 'A')):(c))
#define SLIB_CHAR_LOWER_TO_UPPER(c) (((c) >= 'a' && (c) <= 'z')?((c) - ('a' - 'A')):(c))

namespace slib
{

	class Memory;
	class MemoryData;
	class Locale;
	class Time;

	template <class T> class List;
	template <class T> class ListParam;

	class String;
	class String16;
	class StringContainer;
	class StringContainer16;
	class StringView;
	class StringView16;
	class StringParam;
	class StringStorage;
	
	typedef Atomic<String> AtomicString;
	typedef Atomic<String16> AtomicString16;


	template <class CharType>
	struct StringTypeFromCharType;
	
	template <>
	struct StringTypeFromCharType<sl_char8> { typedef String Type; };
	
	template <>
	struct StringTypeFromCharType<sl_char16> { typedef String16 Type; };
	
	
	template <class CharType>
	struct StringViewTypeFromCharType;
	
	template <>
	struct StringViewTypeFromCharType<sl_char8> { typedef StringView Type; };
	
	template <>
	struct StringViewTypeFromCharType<sl_char16> { typedef StringView16 Type; };
	
	
	template <class StringType>
	struct CharTypeFromStringType;

	template <>
	struct CharTypeFromStringType<String> { typedef sl_char8 Type; };

	template <>
	struct CharTypeFromStringType< Atomic<String> > { typedef sl_char8 Type; };

	template <>
	struct CharTypeFromStringType<String16> { typedef sl_char16 Type; };
	
	template <>
	struct CharTypeFromStringType< Atomic<String16> > { typedef sl_char16 Type; };
	
	template <>
	struct CharTypeFromStringType<StringView> { typedef sl_char8 Type; };

	template <>
	struct CharTypeFromStringType<StringView16> { typedef sl_char16 Type; };


	namespace priv
	{
		namespace string
		{

			extern StringContainer* const g_null;
			extern StringContainer* const g_empty;
			
			extern StringContainer16* const g_null16;
			extern StringContainer16* const g_empty16;

			extern const char* g_conv_radixPatternUpper;
			extern const char* g_conv_radixPatternLower;
			extern const sl_uint8* g_conv_radixInversePatternBig;
			extern const sl_uint8* g_conv_radixInversePatternSmall;

		}
	}

}

#define PRIV_SLIB_DECLARE_STRING_CLASS_OP_TEMPLATE(RET, FUNC) \
	template <class CHAR, sl_size N> RET FUNC(CHAR (&other)[N]) const noexcept; \
	template <class ARG> RET FUNC(const ARG& other) const noexcept;

#define PRIV_SLIB_DECLARE_STRING_CLASS_OP(STRING, RET, FUNC) \
	RET FUNC(const STRING& other) const noexcept; \
	RET FUNC(const Atomic<STRING>& other) const noexcept; \
	PRIV_SLIB_DECLARE_STRING_CLASS_OP_TEMPLATE(RET, FUNC)

#include "string8.h"
#include "string16.h"
#include "string_view.h"
#include "string_param.h"
#include "string_op.h"

namespace slib
{

	class SLIB_EXPORT StringStorage
	{
	public:
		union {
			const sl_char8* data8;
			const sl_char16* data16;
			const sl_char32* data32;
		};
		sl_size length;
		Ref<Referable> ref;
		String string8;
		String16 string16;

	public:
		StringStorage() noexcept;

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(StringStorage)

	};

}

#endif
