/*
 *   Copyright (c) 2008-2024 SLIBIO <https://github.com/SLIBIO>
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

namespace slib
{

	class Memory;
	class MemoryView;
	class MemoryData;
	class Locale;
	class Time;

	template <class T> class List;
	template <class T> class ListParam;

	class String;
	class String16;
	class String32;
	class StringContainer;
	class StringContainer16;
	class StringContainer32;
	class StringView;
	class StringView16;
	class StringView32;
	class StringParam;
	class StringStorage;

	typedef Atomic<String> AtomicString;
	typedef Atomic<String16> AtomicString16;
	typedef Atomic<String32> AtomicString32;


	template <class CharType>
	struct StringTypeFromCharType;
	template <>
	struct StringTypeFromCharType<sl_char8> { typedef String Type; };
	template <>
	struct StringTypeFromCharType<sl_char16> { typedef String16 Type; };
	template <>
	struct StringTypeFromCharType<sl_char32> { typedef String32 Type; };

	template <class CharType>
	struct StringContainerTypeFromCharType;
	template <>
	struct StringContainerTypeFromCharType<sl_char8> { typedef StringContainer Type; };
	template <>
	struct StringContainerTypeFromCharType<sl_char16> { typedef StringContainer16 Type; };
	template <>
	struct StringContainerTypeFromCharType<sl_char32> { typedef StringContainer32 Type; };

	template <class CharType>
	struct StringViewTypeFromCharType;
	template <>
	struct StringViewTypeFromCharType<sl_char8> { typedef StringView Type; };
	template <>
	struct StringViewTypeFromCharType<sl_char16> { typedef StringView16 Type; };
	template <>
	struct StringViewTypeFromCharType<sl_char32> { typedef StringView32 Type; };


	template <class CharType>
	struct OtherCharType;
	template <>
	struct OtherCharType<sl_char8> { typedef sl_char16 Type1; typedef sl_char32 Type2; };
	template <>
	struct OtherCharType<sl_char16> { typedef sl_char8 Type1; typedef sl_char32 Type2; };
	template <>
	struct OtherCharType<sl_char32> { typedef sl_char8 Type1; typedef sl_char16 Type2; };


	namespace priv
	{
		namespace string
		{

			extern StringContainer* const g_null;
			extern StringContainer* const g_empty;

			extern StringContainer16* const g_null16;
			extern StringContainer16* const g_empty16;

			extern StringContainer32* const g_null32;
			extern StringContainer32* const g_empty32;

			extern const char* g_conv_radixPatternUpper;
			extern const char* g_conv_radixPatternLower;
			extern const sl_uint8* g_conv_radixInversePatternBig;
			extern const sl_uint8* g_conv_radixInversePatternSmall;

		}
	}

}

#if defined(SLIB_SUPPORT_STD_TYPES)
#define PRIV_SLIB_DECLARE_STD_STRING_OPS_SUB(CLASS, STRING) \
	STRING operator+(const StdString& other) const noexcept; \
	friend STRING operator+(const StdString& s1, const CLASS& s2) noexcept; \
	friend sl_bool operator==(const StdString& s1, const CLASS& s2) noexcept; \
	friend sl_bool operator!=(const StdString& s1, const CLASS& s2) noexcept; \
	friend sl_compare_result operator>=(const StdString& s1, const CLASS& s2); \
	friend sl_compare_result operator<=(const StdString& s1, const CLASS& s2); \
	friend sl_compare_result operator>(const StdString& s1, const CLASS& s2); \
	friend sl_compare_result operator<(const StdString& s1, const CLASS& s2);
#else
#define PRIV_SLIB_DECLARE_STD_STRING_OPS_SUB(CLASS, STRING)
#endif

#define PRIV_SLIB_DECLARE_STRING_OPS_SUB(CLASS, STRING, VIEW) \
	template <class T> sl_bool operator==(const T& other) const noexcept { return equals(other); } \
	template <class T> sl_bool operator!=(const T& other) const noexcept { return !(equals(other)); } \
	template <class T> sl_compare_result operator>=(const T& other) const noexcept { return compare(other) >= 0; } \
	template <class T> sl_compare_result operator<=(const T& other) const noexcept { return compare(other) <= 0; } \
	template <class T> sl_compare_result operator>(const T& other) const noexcept { return compare(other) > 0; } \
	template <class T> sl_compare_result operator<(const T& other) const noexcept { return compare(other) < 0; } \
	friend STRING operator+(const Char* s1, const CLASS& s2) noexcept; \
	friend sl_bool operator==(const Char* sz, const CLASS& str) noexcept; \
	friend sl_bool operator!=(const Char* sz, const CLASS& str) noexcept; \
	friend sl_compare_result operator>=(const Char* sz, const CLASS& str); \
	friend sl_compare_result operator<=(const Char* sz, const CLASS& str); \
	friend sl_compare_result operator>(const Char* sz, const CLASS& str); \
	friend sl_compare_result operator<(const Char* sz, const CLASS& str); \
	PRIV_SLIB_DECLARE_STD_STRING_OPS_SUB(CLASS, STRING)

#define PRIV_SLIB_DECLARE_STRING_OPS(CLASS) \
	PRIV_SLIB_DECLARE_STRING_OPS_SUB(CLASS, CLASS, StringViewType) \
	CLASS operator+(CLASS&& other) const& noexcept; \
	CLASS operator+(const CLASS& other) const& noexcept; \
	CLASS operator+(const StringViewType& other) const& noexcept; \
	CLASS operator+(const Char* sz) const& noexcept; \
	CLASS operator+(CLASS&& other)&& noexcept; \
	CLASS operator+(const CLASS& other)&& noexcept; \
	CLASS operator+(const StringViewType& other)&& noexcept; \
	CLASS operator+(const Char* sz)&& noexcept; \
	CLASS& operator+=(CLASS&& other) noexcept; \
	CLASS& operator+=(const CLASS& other) noexcept; \
	CLASS& operator+=(const StringViewType& other) noexcept; \
	CLASS& operator+=(const Char* sz) noexcept;

#define PRIV_SLIB_DECLARE_STRING_VIEW_OPS(CLASS) \
	PRIV_SLIB_DECLARE_STRING_OPS_SUB(CLASS, StringType, CLASS) \
	StringType operator+(const StringType& other) const noexcept; \
	StringType operator+(const CLASS& other) const noexcept; \
	StringType operator+(const Char* sz) const noexcept;

#include "priv/string8.h"
#include "priv/string16.h"
#include "priv/string32.h"
#include "priv/string_view.h"
#include "priv/string_param.h"
#include "priv/static_string.h"
#include "priv/encrypt_string.h"

namespace slib
{

	class SLIB_EXPORT StringRawData
	{
	public:
		union {
			void* data;
			sl_char8* data8;
			sl_char16* data16;
			sl_char32* data32;
		};
		sl_reg length;
		sl_uint8 charSize; // In Bytes (1/2/4)

	};

	class SLIB_EXPORT StringStorage
	{
	public:
		union {
			void* data;
			sl_char8* data8;
			sl_char16* data16;
			sl_char32* data32;
		};
		sl_size length;
		sl_uint8 charSize; // In Bytes (1/2/4)
		Ref<CRef> ref;
		String string8;
		String16 string16;
		String32 string32;

	public:
		StringStorage() noexcept;

		StringStorage(const String& str) noexcept;
		StringStorage(String&& str) noexcept;

		StringStorage(const String16& str) noexcept;
		StringStorage(String16&& str) noexcept;

		StringStorage(const String32& str) noexcept;
		StringStorage(String32&& str) noexcept;

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(StringStorage)

	};

}

#endif
