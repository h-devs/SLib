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

namespace slib
{

	class StringRawData;

	namespace priv
	{
		namespace string_param
		{
			struct ConstContainer
			{
				void* value;
				sl_size length;
			};

			extern const ConstContainer g_undefined;
			extern const ConstContainer g_null;
		}
	}

	class SLIB_EXPORT StringParam
	{
	public:
		enum class Type : sl_reg
		{
			String8_Ref = -1,
			String16_Ref = -2,
			String32_Ref = -3,
			String8_NoRef = -4,
			String16_NoRef = -5,
			String32_NoRef = -6,
			Sz8 = -7,
			Sz16 = -8,
			Sz32 = -9
		};

	public:
		union {
			void* _value;

			const sl_char8* _sz8;
			const sl_char16* _sz16;
			const sl_char16* _sz32;
			const StringContainer* _string;
			const StringContainer16* _string16;
			const StringContainer32* _string32;
		};
		union {
			Type _type;
			sl_reg _length;
		};

	public:
		SLIB_CONSTEXPR StringParam(): _value(sl_null), _length(0) {}

		SLIB_CONSTEXPR StringParam(sl_null_t): _value(sl_null), _length(1) {}

		StringParam(StringParam&& other) noexcept;

		StringParam(const StringParam& other) noexcept;

		~StringParam() noexcept;

	public:
		StringParam(const String& value) noexcept;

		StringParam(String&& value) noexcept;

		StringParam(const String16& value) noexcept;

		StringParam(String16&& value) noexcept;

		StringParam(const String32& value) noexcept;

		StringParam(String32&& value) noexcept;

		StringParam(const AtomicString& value) noexcept;

		StringParam(const AtomicString16& value) noexcept;

		StringParam(const AtomicString32& value) noexcept;

		StringParam(const StringView& value) noexcept;

		StringParam(const StringView16& value) noexcept;

		StringParam(const StringView32& value) noexcept;

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
		static const StringParam& undefined() noexcept
		{
			return *(reinterpret_cast<StringParam const*>(&(priv::string_param::g_undefined)));
		}

		static const StringParam& null() noexcept
		{
			return *(reinterpret_cast<StringParam const*>(&(priv::string_param::g_null)));
		}

		template <class CHAR, sl_size N>
		static StringParam literal(const CHAR (&s)[N]) noexcept
		{
			return StringParam(s, N - 1);
		}

		static StringParam fromUtf(const void* data, sl_size size);

		static StringParam fromUtf(const MemoryView& mem);

	public:
		StringParam& operator=(const StringParam& other) noexcept;
		StringParam& operator=(StringParam&& other) noexcept;

		template <class T>
		StringParam& operator=(T&& other) noexcept
		{
			_free();
			new (this) StringParam(Forward<T>(other));
			return *this;
		}

	public:
		void setUndefined() noexcept;

		SLIB_CONSTEXPR sl_bool isUndefined() const
		{
			return (!_value) && (!_length);
		}

		SLIB_CONSTEXPR sl_bool isNotUndefined() const
		{
			return _value || _length;
		}

		void setNull() noexcept;

		SLIB_CONSTEXPR sl_bool isNull() const
		{
			return !_value;
		}

		SLIB_CONSTEXPR sl_bool isNotNull() const
		{
			return _value != 0;
		}


		sl_bool isEmpty() const noexcept;

		sl_bool isNotEmpty() const noexcept;


		sl_bool is8BitsStringType() const noexcept;

		sl_bool is16BitsStringType() const noexcept;

		sl_bool is32BitsStringType() const noexcept;

		sl_bool isStringObject8() const noexcept;

		sl_bool isStringObject16() const noexcept;

		sl_bool isStringObject32() const noexcept;

		sl_bool isStringView8() const noexcept;

		sl_bool isStringView16() const noexcept;

		sl_bool isStringView32() const noexcept;


		void getData(StringRawData& outData) const noexcept;


		String toString() const noexcept;

		String16 toString16() const noexcept;

		String32 toString32() const noexcept;

		String newString() const noexcept;

		String16 newString16() const noexcept;

		String32 newString32() const noexcept;

		Variant toVariant() const noexcept;

		sl_size getHashCode() const noexcept;

	public:
		void _free() noexcept;

		friend class StringData;
		friend class StringData16;
		friend class StringData32;
		friend class StringCstr;
		friend class StringCstr16;
		friend class StringCstr32;
	};


	class SLIB_EXPORT StringData : public StringView
	{
	public:
		String string;

	public:
		StringData(const StringParam& param) noexcept;

		StringData(StringParam&& param) noexcept;

		StringData(String&& str) noexcept;

		StringData(const sl_char8* data) noexcept;

		StringData(const sl_char8* data, sl_size length) noexcept;

		StringData(const sl_char16* data) noexcept;

		StringData(const sl_char16* data, sl_size length) noexcept;

		StringData(const sl_char32* data) noexcept;

		StringData(const sl_char32* data, sl_size length) noexcept;

		StringData(const String& str) noexcept;

		StringData(const AtomicString& str) noexcept;

		StringData(const String16& str) noexcept;

		StringData(const AtomicString16& str) noexcept;

		StringData(const String32& str) noexcept;

		StringData(const AtomicString32& str) noexcept;

		StringData(const StringView& str) noexcept;

		StringData(const StringView16& str) noexcept;

		StringData(const StringView32& str) noexcept;

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(StringData)

	public:
		template <sl_size N>
		static StringData literal(const sl_char8 (&s)[N]) noexcept
		{
			return StringData(s, N - 1);
		}

		String toString(const StringParam& param);

		template <class T>
		StringData& operator=(T&& t)
		{
			this->~StringData();
			new (this) StringData(Forward<T>(t));
			return *this;
		}

	};

	class SLIB_EXPORT StringData16 : public StringView16
	{
	public:
		String16 string;

	public:
		StringData16(const StringParam& param) noexcept;

		StringData16(StringParam&& param) noexcept;

		StringData16(String16&& str) noexcept;

		StringData16(const sl_char8* data) noexcept;

		StringData16(const sl_char8* data, sl_size length) noexcept;

		StringData16(const sl_char16* data) noexcept;

		StringData16(const sl_char16* data, sl_size length) noexcept;

		StringData16(const sl_char32* data) noexcept;

		StringData16(const sl_char32* data, sl_size length) noexcept;

		StringData16(const String& str) noexcept;

		StringData16(const AtomicString& str) noexcept;

		StringData16(const String16& str) noexcept;

		StringData16(const AtomicString16& str) noexcept;

		StringData16(const String32& str) noexcept;

		StringData16(const AtomicString32& str) noexcept;

		StringData16(const StringView& str) noexcept;

		StringData16(const StringView16& str) noexcept;

		StringData16(const StringView32& str) noexcept;

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(StringData16)

	public:
		template <sl_size N>
		static StringData16 literal(const sl_char16 (&s)[N]) noexcept
		{
			return StringData16(s, N - 1);
		}

		String16 toString16(const StringParam& param);

		template <class T>
		StringData16& operator=(T&& t)
		{
			this->~StringData16();
			new (this) StringData16(Forward<T>(t));
			return *this;
		}

	};

	class SLIB_EXPORT StringData32 : public StringView32
	{
	public:
		String32 string;

	public:
		StringData32(const StringParam& param) noexcept;

		StringData32(StringParam&& param) noexcept;

		StringData32(String32&& str) noexcept;

		StringData32(const sl_char8* data) noexcept;

		StringData32(const sl_char8* data, sl_size length) noexcept;

		StringData32(const sl_char16* data) noexcept;

		StringData32(const sl_char16* data, sl_size length) noexcept;

		StringData32(const sl_char32* data) noexcept;

		StringData32(const sl_char32* data, sl_size length) noexcept;

		StringData32(const String& str) noexcept;

		StringData32(const AtomicString& str) noexcept;

		StringData32(const String16& str) noexcept;

		StringData32(const AtomicString16& str) noexcept;

		StringData32(const String32& str) noexcept;

		StringData32(const AtomicString32& str) noexcept;

		StringData32(const StringView& str) noexcept;

		StringData32(const StringView16& str) noexcept;

		StringData32(const StringView32& str) noexcept;

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(StringData32)

	public:
		template <sl_size N>
		static StringData32 literal(const sl_char32(&s)[N]) noexcept
		{
			return StringData32(s, N - 1);
		}

		String32 toString32(const StringParam& param);

		template <class T>
		StringData32& operator=(T&& t)
		{
			this->~StringData32();
			new (this) StringData32(Forward<T>(t));
			return *this;
		}

	};

	class SLIB_EXPORT StringCstr : public StringView
	{
	public:
		String string;

	public:
		StringCstr() noexcept;

		StringCstr(const StringParam& param) noexcept;

		StringCstr(StringParam&& param) noexcept;

		StringCstr(String&& str) noexcept;

		StringCstr(const sl_char8* data) noexcept;

		StringCstr(const sl_char8* data, sl_size length) noexcept;

		StringCstr(const sl_char16* data) noexcept;

		StringCstr(const sl_char16* data, sl_size length) noexcept;

		StringCstr(const sl_char32* data) noexcept;

		StringCstr(const sl_char32* data, sl_size length) noexcept;

		StringCstr(const String& str) noexcept;

		StringCstr(const AtomicString& str) noexcept;

		StringCstr(const String16& str) noexcept;

		StringCstr(const AtomicString16& str) noexcept;

		StringCstr(const String32& str) noexcept;

		StringCstr(const AtomicString32& str) noexcept;

		StringCstr(const StringView& str) noexcept;

		StringCstr(const StringView16& str) noexcept;

		StringCstr(const StringView32& str) noexcept;

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(StringCstr)

	public:
		template <sl_size N>
		static StringCstr literal(const sl_char8 (&s)[N]) noexcept
		{
			return StringCstr(s, N - 1);
		}

		String toString(const StringParam& param);

		template <class T>
		StringCstr& operator=(T&& t)
		{
			this->~StringCstr();
			new (this) StringCstr(Forward<T>(t));
			return *this;
		}

	};

	class SLIB_EXPORT StringCstr16 : public StringView16
	{
	public:
		String16 string;

	public:
		StringCstr16() noexcept;

		StringCstr16(const StringParam& param) noexcept;

		StringCstr16(StringParam&& param) noexcept;

		StringCstr16(String16&& str) noexcept;

		StringCstr16(const sl_char8* data) noexcept;

		StringCstr16(const sl_char8* data, sl_size length) noexcept;

		StringCstr16(const sl_char16* data) noexcept;

		StringCstr16(const sl_char16* data, sl_size length) noexcept;

		StringCstr16(const sl_char32* data) noexcept;

		StringCstr16(const sl_char32* data, sl_size length) noexcept;

		StringCstr16(const String& str) noexcept;

		StringCstr16(const AtomicString& str) noexcept;

		StringCstr16(const String16& str) noexcept;

		StringCstr16(const AtomicString16& str) noexcept;

		StringCstr16(const String32& str) noexcept;

		StringCstr16(const AtomicString32& str) noexcept;

		StringCstr16(const StringView& str) noexcept;

		StringCstr16(const StringView16& str) noexcept;

		StringCstr16(const StringView32& str) noexcept;

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(StringCstr16)

	public:
		template <sl_size N>
		static StringCstr16 literal(const sl_char16 (&s)[N]) noexcept
		{
			return StringCstr16(s, N - 1);
		}

		String16 toString16(const StringParam& param);

		template <class T>
		StringCstr16& operator=(T&& t)
		{
			this->~StringCstr16();
			new (this) StringCstr16(Forward<T>(t));
			return *this;
		}

	};

	class SLIB_EXPORT StringCstr32 : public StringView32
	{
	public:
		String32 string;

	public:
		StringCstr32() noexcept;

		StringCstr32(const StringParam& param) noexcept;

		StringCstr32(StringParam&& param) noexcept;

		StringCstr32(String32&& str) noexcept;

		StringCstr32(const sl_char8* data) noexcept;

		StringCstr32(const sl_char8* data, sl_size length) noexcept;

		StringCstr32(const sl_char16* data) noexcept;

		StringCstr32(const sl_char16* data, sl_size length) noexcept;

		StringCstr32(const sl_char32* data) noexcept;

		StringCstr32(const sl_char32* data, sl_size length) noexcept;

		StringCstr32(const String& str) noexcept;

		StringCstr32(const AtomicString& str) noexcept;

		StringCstr32(const String16& str) noexcept;

		StringCstr32(const AtomicString16& str) noexcept;

		StringCstr32(const String32& str) noexcept;

		StringCstr32(const AtomicString32& str) noexcept;

		StringCstr32(const StringView& str) noexcept;

		StringCstr32(const StringView16& str) noexcept;

		StringCstr32(const StringView32& str) noexcept;

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(StringCstr32)

	public:
		template <sl_size N>
		static StringCstr32 literal(const sl_char32(&s)[N]) noexcept
		{
			return StringCstr32(s, N - 1);
		}

		String32 toString32(const StringParam& param);

		template <class T>
		StringCstr32& operator=(T&& t)
		{
			this->~StringCstr32();
			new (this) StringCstr32(Forward<T>(t));
			return *this;
		}

	};

	template <class... ARGS>
	String String::concat(const StringParam& s1, const StringParam& s2, ARGS&&... args) noexcept
	{
		StringParam params[] = {s1, s2, Forward<ARGS>(args)...};
		return join(params, 2 + sizeof...(args));
	}

	template <class... ARGS>
	String16 String16::concat(const StringParam& s1, const StringParam& s2, ARGS&&... args) noexcept
	{
		StringParam params[] = {s1, s2, Forward<ARGS>(args)...};
		return join(params, 2 + sizeof...(args));
	}

	template <class... ARGS>
	String32 String32::concat(const StringParam& s1, const StringParam& s2, ARGS&&... args) noexcept
	{
		StringParam params[] = {s1, s2, Forward<ARGS>(args)...};
		return join(params, 2 + sizeof...(args));
	}

}

#endif
