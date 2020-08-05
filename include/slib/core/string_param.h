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

#ifndef CHECKHEADER_SLIB_CORE_STRING_PARAM
#define CHECKHEADER_SLIB_CORE_STRING_PARAM

#include "string8.h"
#include "string16.h"

#ifdef SLIB_SUPPORT_STD_TYPES
#include <string>
#endif

namespace slib
{

	class Variant;

	enum class StringType : sl_reg
	{
		String8_Ref = -1,
		String16_Ref = -2,
		String8_NoRef = -3,
		String16_NoRef = -4,
		Sz8 = -5,
		Sz16 = -6
	};
	
	class SLIB_EXPORT StringParam
	{
	public:
		union {
			void* _value;
			
			const sl_char8* _sz8;
			const sl_char16* _sz16;
			const StringContainer* _string;
			const StringContainer16* _string16;
		};
		union {
			StringType _type;
			sl_reg _length;
		};
		
	public:
		SLIB_INLINE constexpr StringParam() noexcept : _value(sl_null), _length(0) {}
		
		SLIB_INLINE constexpr StringParam(sl_null_t) noexcept : _value(sl_null), _length(1) {}
		
		StringParam(StringParam&& other) noexcept;
		
		StringParam(const StringParam& other) noexcept;
		
		~StringParam() noexcept;
		
	public:
		StringParam(const String& value) noexcept;
		
		StringParam(String&& value) noexcept;
		
		StringParam(const String16& value) noexcept;
		
		StringParam(String16&& value) noexcept;
		
		StringParam(const AtomicString& value) noexcept;
		
		StringParam(const AtomicString16& value) noexcept;
		
		StringParam(const StringView& value) noexcept;
		
		StringParam(const StringView16& value) noexcept;
		
		StringParam(const char* sz) noexcept;
		
		StringParam(const wchar_t* sz) noexcept;
		
		StringParam(const char16_t* sz) noexcept;
		
		StringParam(const char32_t* sz) noexcept;

		StringParam(const char* str, sl_reg length) noexcept;
		
		StringParam(const wchar_t* str, sl_reg length) noexcept;
		
		StringParam(const char16_t* str, sl_reg length) noexcept;
		
		StringParam(const char32_t* str, sl_reg length) noexcept;
		
#ifdef SLIB_SUPPORT_STD_TYPES
		StringParam(const std::string& str) noexcept;

		StringParam(const std::wstring& str) noexcept;
		
		StringParam(const std::u16string& str) noexcept;
		
		StringParam(const std::u32string& str) noexcept;
#endif
		
	public:
		static const StringParam& undefined() noexcept;
		
		static const StringParam& null() noexcept;
		
		template <sl_size N>
		static StringParam literal(const sl_char8 (&s)[N]) noexcept;
		
		template <sl_size N>
		static StringParam literal(const sl_char16 (&s)[N]) noexcept;
		
	public:
		StringParam& operator=(StringParam&& other) noexcept;
		
		StringParam& operator=(const StringParam& other) noexcept;

		StringParam& operator=(sl_null_t) noexcept;
		
		StringParam& operator=(const String& value) noexcept;
		
		StringParam& operator=(String&& value) noexcept;
		
		StringParam& operator=(const String16& value) noexcept;
		
		StringParam& operator=(String16&& value) noexcept;

		StringParam& operator=(const AtomicString& value) noexcept;
		
		StringParam& operator=(const AtomicString16& value) noexcept;
		
		StringParam& operator=(const StringView& value) noexcept;
		
		StringParam& operator=(const StringView16& value) noexcept;
		
		StringParam& operator=(const char* sz) noexcept;
		
		StringParam& operator=(const wchar_t* sz) noexcept;
		
		StringParam& operator=(const char16_t* sz) noexcept;
		
		StringParam& operator=(const char32_t* sz) noexcept;
		
#ifdef SLIB_SUPPORT_STD_TYPES
		StringParam& operator=(const std::string& str) noexcept;

		StringParam& operator=(const std::wstring& str) noexcept;
		
		StringParam& operator=(const std::u16string& str) noexcept;
		
		StringParam& operator=(const std::u32string& str) noexcept;
#endif
		
	public:
		void setUndefined() noexcept;
		
		sl_bool isUndefined() const noexcept;
		
		sl_bool isNotUndefined() const noexcept;
		
		void setNull() noexcept;
		
		sl_bool isNull() const noexcept;
		
		sl_bool isNotNull() const noexcept;
		
		
		sl_bool isEmpty() const noexcept;
		
		sl_bool isNotEmpty() const noexcept;

		
		sl_bool isString8() const noexcept;
		
		sl_bool isString16() const noexcept;
		
		sl_bool isSz8() const noexcept;
		
		sl_bool isSz16() const noexcept;
		
		sl_bool is8() const noexcept;

		sl_bool is16() const noexcept;
		
		
		String toString() const noexcept;
		
		String16 toString16() const noexcept;
		
		Variant toVariant() const noexcept;
		
	public:
		PRIV_SLIB_DECLARE_STRING_CLASS_OP_TEMPLATE(sl_bool, equals)

		PRIV_SLIB_DECLARE_STRING_CLASS_OP_TEMPLATE(sl_compare_result, compare)

		sl_size getHashCode() const noexcept;

	public:
		void _free() noexcept;
		
		void _assign(const sl_char8* str, sl_reg length) noexcept;
		void _assign(const sl_char16* str, sl_reg length) noexcept;

		friend class StringData;
		friend class StringData16;
		friend class StringCstr;
		friend class StringCstr16;
	};
	
	template <>
	class Compare<StringParam>
	{
	public:
		sl_compare_result operator()(const StringParam& a, const StringParam& b) const noexcept;
	};
	
	template <>
	class Equals<StringParam>
	{
	public:
		sl_bool operator()(const StringParam& a, const StringParam& b) const noexcept;
	};
	
	template <>
	class Hash<StringParam>
	{
	public:
		sl_size operator()(const StringParam& a) const noexcept;
	};


	class SLIB_EXPORT StringData : public StringView
	{
	public:
		String string;
		
	public:
		StringData(const StringParam& param) noexcept;
		
		StringData(const sl_char8* data) noexcept;
		
		StringData(const sl_char8* data, sl_size length) noexcept;
		
		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(StringData)
		
	public:
		template <sl_size N>
		SLIB_INLINE static StringData literal(const sl_char8 (&s)[N]) noexcept
		{
			return StringData(s, N - 1);
		}
		
	};

	class SLIB_EXPORT StringData16 : public StringView16
	{
	public:
		String16 string;
		
	public:
		StringData16(const StringParam& param) noexcept;
		
		StringData16(const sl_char16* data) noexcept;
		
		StringData16(const sl_char16* data, sl_size length) noexcept;
		
		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(StringData16)
		
	public:
		template <sl_size N>
		SLIB_INLINE static StringData16 literal(const sl_char16 (&s)[N]) noexcept
		{
			return StringData16(s, N - 1);
		}

	};
	
	class SLIB_EXPORT StringCstr : public StringView
	{
	public:
		String string;
		
	public:
		StringCstr(const StringParam& param) noexcept;
		
		StringCstr(const sl_char8* data) noexcept;
		
		StringCstr(const sl_char8* data, sl_size length) noexcept;
		
		StringCstr(const String& str) noexcept;
		
		StringCstr(const AtomicString& str) noexcept;
		
		StringCstr(const StringView& str) noexcept;

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(StringCstr)
		
	public:
		template <sl_size N>
		SLIB_INLINE static StringCstr literal(const sl_char8 (&s)[N]) noexcept
		{
			return StringCstr(s, N - 1);
		}
		
	};

	class SLIB_EXPORT StringCstr16 : public StringView16
	{
	public:
		String16 string;
		
	public:
		StringCstr16(const StringParam& param) noexcept;
		
		StringCstr16(const sl_char16* data) noexcept;
		
		StringCstr16(const sl_char16* data, sl_size length) noexcept;
		
		StringCstr16(const String16& str) noexcept;
		
		StringCstr16(const AtomicString16& str) noexcept;
		
		StringCstr16(const StringView16& str) noexcept;

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(StringCstr16)
		
	public:
		template <sl_size N>
		SLIB_INLINE static StringCstr16 literal(const sl_char16 (&s)[N]) noexcept
		{
			return StringCstr16(s, N - 1);
		}
		
	};

}

#endif
