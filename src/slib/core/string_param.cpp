/*
 *   Copyright (c) 2008-2019 SLIBIO <https://github.com/SLIBIO>
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

#define SLIB_SUPPORT_STD_TYPES

#include "slib/core/string.h"

#include "slib/core/variant.h"

namespace slib
{

#define STRING_TYPE_STRING8_REF ((sl_reg)(StringParam::Type::String8_Ref))
#define STRING_TYPE_STRING16_REF ((sl_reg)(StringParam::Type::String16_Ref))
#define STRING_TYPE_STRING32_REF ((sl_reg)(StringParam::Type::String32_Ref))
#define STRING_TYPE_STRING8_NOREF ((sl_reg)(StringParam::Type::String8_NoRef))
#define STRING_TYPE_STRING16_NOREF ((sl_reg)(StringParam::Type::String16_NoRef))
#define STRING_TYPE_STRING32_NOREF ((sl_reg)(StringParam::Type::String32_NoRef))
#define STRING_TYPE_SZ8 ((sl_reg)(StringParam::Type::Sz8))
#define STRING_TYPE_SZ16 ((sl_reg)(StringParam::Type::Sz16))
#define STRING_TYPE_SZ32 ((sl_reg)(StringParam::Type::Sz32))

#ifdef SLIB_ARCH_IS_64BIT
#define IS_STR8(length) (((length) >> 61) == 0)
#define IS_STR16(length) (((length) >> 61) == 1)
#define IS_STR32(length) (((length) >> 61) == 2)
#define STRING_TYPE_STR16_PREFIX SLIB_UINT64(0x2000000000000000)
#define STRING_TYPE_STR32_PREFIX SLIB_UINT64(0x4000000000000000)
#define LENGTH_MASK SLIB_UINT64(0x1FFFFFFFFFFFFFFF)
#else
#define IS_STR8(length) (((length) >> 29) == 0)
#define IS_STR16(length) (((length) >> 29) == 1)
#define IS_STR32(length) (((length) >> 29) == 2)
#define STRING_TYPE_STR16_PREFIX 0x20000000
#define STRING_TYPE_STR32_PREFIX 0x40000000
#define LENGTH_MASK 0x1FFFFFFF
#endif

#define GET_LENGTH(length) ((length) & LENGTH_MASK)

#define STRING_PTR(s) ((String*)(void*)(&((s)._value)))
#define STRING_REF(s) (*STRING_PTR(s))
#define STRING16_PTR(s) ((String16*)(void*)(&((s)._value)))
#define STRING16_REF(s) (*STRING16_PTR(s))
#define STRING32_PTR(s) ((String32*)(void*)(&((s)._value)))
#define STRING32_REF(s) (*STRING32_PTR(s))
#define STRING_CONTAINER(s) (*((void**)(void*)(&(s))))
#define STRING_CONTAINER16(s) (*((void**)(void*)(&(s))))
#define STRING_CONTAINER32(s) (*((void**)(void*)(&(s))))

	namespace priv
	{
		namespace string_param
		{
			const ConstContainer g_undefined = { sl_null, 0 };
			const ConstContainer g_null = { sl_null, 1 };
		}
	}

	namespace {

		SLIB_INLINE static void CopyParam(StringParam& dst, const StringParam& src) noexcept
		{
			dst._value = src._value;
			dst._length = src._length;
			if (src._value) {
				switch (src._length) {
					case STRING_TYPE_STRING8_REF:
						dst._length = STRING_TYPE_STRING8_NOREF;
						break;
					case STRING_TYPE_STRING16_REF:
						dst._length = STRING_TYPE_STRING16_NOREF;
						break;
					case STRING_TYPE_STRING32_REF:
						dst._length = STRING_TYPE_STRING32_NOREF;
						break;
				}
			}
		}

		template <class STRING>
		static STRING ToString(const StringParam& param) noexcept
		{
			if (!(param._value)) {
				return sl_null;
			}
			switch (param._length) {
				case STRING_TYPE_STRING8_REF:
				case STRING_TYPE_STRING8_NOREF:
					return STRING::from(STRING_REF(param));
				case STRING_TYPE_STRING16_REF:
				case STRING_TYPE_STRING16_NOREF:
					return STRING::from(STRING16_REF(param));
				case STRING_TYPE_STRING32_REF:
				case STRING_TYPE_STRING32_NOREF:
					return STRING::from(STRING32_REF(param));
				case STRING_TYPE_SZ8:
					return STRING::create((sl_char8*)(param._value));
				case STRING_TYPE_SZ16:
					return STRING::create((sl_char16*)(param._value));
				case STRING_TYPE_SZ32:
					return STRING::create((sl_char32*)(param._value));
				default:
					if (IS_STR8(param._length)) {
						return STRING::create((sl_char8*)(param._value), param._length);
					} else if (IS_STR16(param._length)) {
						return STRING::create((sl_char16*)(param._value), GET_LENGTH(param._length));
					} else if (IS_STR32(param._length)) {
						return STRING::create((sl_char32*)(param._value), GET_LENGTH(param._length));
					} else {
						return sl_null;
					}
			}
		}

	}

	void StringParam::_free() noexcept
	{
		switch (_length)
		{
			case STRING_TYPE_STRING8_REF:
				STRING_REF(*this).String::~String();
				break;
			case STRING_TYPE_STRING16_REF:
				STRING16_REF(*this).String16::~String16();
				break;
			case STRING_TYPE_STRING32_REF:
				STRING32_REF(*this).String32::~String32();
				break;
			default:
				break;
		}
	}

	StringParam::StringParam(StringParam&& other) noexcept
	{
		_value = other._value;
		_length = other._length;
		other._value = sl_null;
		other._length = 0;
	}

	StringParam::StringParam(const StringParam& other) noexcept
	{
		CopyParam(*this, other);
	}

	StringParam::~StringParam() noexcept
	{
		_free();
	}

	StringParam::StringParam(const String& value) noexcept
	{
		if (value.isNotNull()) {
			_length = STRING_TYPE_STRING8_NOREF;
			_value = STRING_CONTAINER(value);
		} else {
			_value = sl_null;
			_length = 1;
		}
	}

	StringParam::StringParam(String&& value) noexcept
	{
		if (value.isNotNull()) {
			_length = STRING_TYPE_STRING8_REF;
			new STRING_PTR(*this) String(Move(value));
		} else {
			_value = sl_null;
			_length = 1;
		}
	}

	StringParam::StringParam(const String16& value) noexcept
	{
		if (value.isNotNull()) {
			_length = STRING_TYPE_STRING16_NOREF;
			_value = STRING_CONTAINER16(value);
		} else {
			_value = sl_null;
			_length = 1;
		}
	}

	StringParam::StringParam(String16&& value) noexcept
	{
		if (value.isNotNull()) {
			_length = STRING_TYPE_STRING16_REF;
			new STRING16_PTR(*this) String16(Move(value));
		} else {
			_value = sl_null;
			_length = 1;
		}
	}

	StringParam::StringParam(const String32& value) noexcept
	{
		if (value.isNotNull()) {
			_length = STRING_TYPE_STRING32_NOREF;
			_value = STRING_CONTAINER32(value);
		} else {
			_value = sl_null;
			_length = 1;
		}
	}

	StringParam::StringParam(String32&& value) noexcept
	{
		if (value.isNotNull()) {
			_length = STRING_TYPE_STRING32_REF;
			new STRING32_PTR(*this) String32(Move(value));
		} else {
			_value = sl_null;
			_length = 1;
		}
	}

	StringParam::StringParam(const AtomicString& s) noexcept : StringParam(String(s))
	{
	}

	StringParam::StringParam(const AtomicString16& s) noexcept : StringParam(String16(s))
	{
	}

	StringParam::StringParam(const AtomicString32& s) noexcept : StringParam(String32(s))
	{
	}

	StringParam::StringParam(const StringView& str) noexcept : StringParam(str.getUnsafeData(), str.getUnsafeLength())
	{
	}

	StringParam::StringParam(const StringView16& str) noexcept : StringParam(str.getUnsafeData(), str.getUnsafeLength())
	{
	}

	StringParam::StringParam(const StringView32& str) noexcept : StringParam(str.getUnsafeData(), str.getUnsafeLength())
	{
	}

	StringParam::StringParam(const char* sz) noexcept
	{
		if (sz) {
			_value = (void*)sz;
			_length = STRING_TYPE_SZ8;
		} else {
			_value = sl_null;
			_length = 1;
		}
	}

	StringParam::StringParam(const wchar_t* sz) noexcept
	{
		if (sz) {
			_value = (void*)sz;
			if (sizeof(wchar_t) == 2) {
				_length = STRING_TYPE_SZ16;
			} else {
				_length = STRING_TYPE_SZ32;
			}
		} else {
			_value = sl_null;
			_length = 1;
		}
	}

	StringParam::StringParam(const char16_t* sz) noexcept
	{
		if (sz) {
			_value = (void*)sz;
			_length = STRING_TYPE_SZ16;
		} else {
			_value = sl_null;
			_length = 1;
		}
	}

	StringParam::StringParam(const char32_t* sz) noexcept
	{
		if (sz) {
			_value = (void*)sz;
			_length = STRING_TYPE_SZ32;
		} else {
			_value = sl_null;
			_length = 1;
		}
	}

	StringParam::StringParam(const char* str, sl_reg length) noexcept
	{
		if (str) {
			_value = (void*)str;
			if (length < 0) {
				_length = STRING_TYPE_SZ8;
			} else {
				_length = GET_LENGTH(length);
			}
		} else {
			_value = sl_null;
			_length = 1;
		}
	}

	StringParam::StringParam(const wchar_t* str, sl_reg length) noexcept
	{
		if (str) {
			_value = (void*)str;
			if (sizeof(wchar_t) == 2) {
				if (length < 0) {
					_length = STRING_TYPE_SZ16;
				} else {
					_length = STRING_TYPE_STR16_PREFIX | GET_LENGTH(length);
				}
			} else {
				if (length < 0) {
					_length = STRING_TYPE_SZ32;
				} else {
					_length = STRING_TYPE_STR32_PREFIX | GET_LENGTH(length);
				}
			}
		} else {
			_value = sl_null;
			_length = 1;
		}
	}

	StringParam::StringParam(const char16_t* str, sl_reg length) noexcept
	{
		if (str) {
			_value = (void*)str;
			if (length < 0) {
				_length = STRING_TYPE_SZ16;
			} else {
				_length = STRING_TYPE_STR16_PREFIX | GET_LENGTH(length);
			}
		} else {
			_value = sl_null;
			_length = 1;
		}
	}

	StringParam::StringParam(const char32_t* str, sl_reg length) noexcept
	{
		if (str) {
			_value = (void*)str;
			if (length < 0) {
				_length = STRING_TYPE_SZ32;
			} else {
				_length = STRING_TYPE_STR32_PREFIX | GET_LENGTH(length);
			}
		} else {
			_value = sl_null;
			_length = 1;
		}
	}

	StringParam::StringParam(const std::string& str) noexcept : StringParam(str.c_str(), str.length())
	{
	}

	StringParam::StringParam(const std::wstring& str) noexcept : StringParam(str.c_str(), str.length())
	{
	}

	StringParam::StringParam(const std::u16string& str) noexcept : StringParam(str.c_str(), str.length())
	{
	}

	StringParam::StringParam(const std::u32string& str) noexcept : StringParam(str.c_str(), str.length())
	{
	}

	StringParam& StringParam::operator=(const StringParam& other) noexcept
	{
		_free();
		CopyParam(*this, other);
		return *this;
	}

	StringParam& StringParam::operator=(StringParam&& other) noexcept
	{
		_free();
		_value = other._value;
		_length = other._length;
		other._value = sl_null;
		other._length = 0;
		return *this;
	}

	StringParam StringParam::fromUtf(const void* _data, sl_size size)
	{
		const sl_uint8* data = (const sl_uint8*)_data;
		if (!data || !size) {
			return sl_null;
		}
		if (size >= 2) {
			if (*data == 0xFF && data[1] == 0xFE) {
				if (Endian::isLE() && !(((sl_size)data) & 1)) {
					return StringParam((sl_char16*)(data + 2), (size - 2) >> 1);
				} else {
					return String16::fromUtf16LE(data + 2, size - 2);
				}
			}
			if (*data == 0xFE && data[1] == 0xFF) {
				if (Endian::isBE() && !(((sl_size)data) & 1)) {
					return StringParam((sl_char16*)(data + 2), (size - 2) >> 1);
				} else {
					return String16::fromUtf16BE(data + 2, size - 2);
				}
			}
		}
		if (size >= 3) {
			if (*data == 0xEF && data[1] == 0xBB && data[2] == 0xBF) {
				return StringParam((sl_char8*)data + 3, size - 3);
			}
		}
		return StringParam((sl_char8*)data, size);
	}

	StringParam StringParam::fromUtf(const MemoryView& mem)
	{
		return fromUtf(mem.data, mem.size);
	}

	void StringParam::setUndefined() noexcept
	{
		if (_value) {
			_free();
			_value = sl_null;
		}
		_length = 0;
	}

	void StringParam::setNull() noexcept
	{
		if (_value) {
			_free();
			_value = sl_null;
		}
		_length = 1;
	}

	sl_bool StringParam::isEmpty() const noexcept
	{
		if (!_value) {
			return sl_true;
		}
		switch (_length) {
			case 0:
			case STRING_TYPE_STR16_PREFIX:
			case STRING_TYPE_STR32_PREFIX:
				return sl_true;
			case STRING_TYPE_STRING8_REF:
			case STRING_TYPE_STRING8_NOREF:
				return STRING_REF(*this).isEmpty();
			case STRING_TYPE_STRING16_REF:
			case STRING_TYPE_STRING16_NOREF:
				return STRING16_REF(*this).isEmpty();
			case STRING_TYPE_STRING32_REF:
			case STRING_TYPE_STRING32_NOREF:
				return STRING32_REF(*this).isEmpty();
			case STRING_TYPE_SZ8:
				return !(*((sl_char8*)_value));
			case STRING_TYPE_SZ16:
				return !(*((sl_char16*)_value));
			case STRING_TYPE_SZ32:
				return !(*((sl_char32*)_value));
			default:
				return sl_false;
		}
	}

	sl_bool StringParam::isNotEmpty() const noexcept
	{
		return !(isEmpty());
	}

	sl_bool StringParam::is8BitsStringType() const noexcept
	{
		return _value && (_length == STRING_TYPE_STRING8_REF || _length == STRING_TYPE_STRING8_NOREF || _length == STRING_TYPE_SZ8 || IS_STR8(_length));
	}

	sl_bool StringParam::is16BitsStringType() const noexcept
	{
		return _value && (_length == STRING_TYPE_STRING16_REF || _length == STRING_TYPE_STRING16_NOREF || _length == STRING_TYPE_SZ16 || IS_STR16(_length));
	}

	sl_bool StringParam::is32BitsStringType() const noexcept
	{
		return _value && (_length == STRING_TYPE_STRING32_REF || _length == STRING_TYPE_STRING32_NOREF || _length == STRING_TYPE_SZ32 || IS_STR32(_length));
	}

	sl_bool StringParam::isStringObject8() const noexcept
	{
		return _value && (_length == STRING_TYPE_STRING8_REF || _length == STRING_TYPE_STRING8_NOREF);
	}

	sl_bool StringParam::isStringObject16() const noexcept
	{
		return _value && (_length == STRING_TYPE_STRING16_REF || _length == STRING_TYPE_STRING16_NOREF);
	}

	sl_bool StringParam::isStringObject32() const noexcept
	{
		return _value && (_length == STRING_TYPE_STRING32_REF || _length == STRING_TYPE_STRING32_NOREF);
	}

	sl_bool StringParam::isStringView8() const noexcept
	{
		return _value && (_length == STRING_TYPE_SZ8 || IS_STR8(_length));
	}

	sl_bool StringParam::isStringView16() const noexcept
	{
		return _value && (_length == STRING_TYPE_SZ8 || IS_STR8(_length));
	}

	sl_bool StringParam::isStringView32() const noexcept
	{
		return _value && (_length == STRING_TYPE_SZ16 || IS_STR16(_length));
	}

	void StringParam::getData(StringRawData& outData) const noexcept
	{
		if (_value) {
			switch (_length) {
				case STRING_TYPE_STRING8_REF:
				case STRING_TYPE_STRING8_NOREF:
					{
						sl_size len;
						outData.data8 = STRING_REF(*this).getData(len);
						outData.length = len;
						outData.charSize = 1;
						return;
					}
				case STRING_TYPE_SZ8:
					{
						outData.data8 = (sl_char8*)_value;
						outData.length = -1;
						outData.charSize = 1;
						return;
					}
				case STRING_TYPE_STRING16_REF:
				case STRING_TYPE_STRING16_NOREF:
					{
						sl_size len;
						outData.data16 = STRING16_REF(*this).getData(len);
						outData.length = len;
						outData.charSize = 2;
						return;
					}
				case STRING_TYPE_SZ16:
					{
						outData.data16 = (sl_char16*)_value;
						outData.length = -1;
						outData.charSize = 2;
						return;
					}
				case STRING_TYPE_STRING32_REF:
				case STRING_TYPE_STRING32_NOREF:
					{
						sl_size len;
						outData.data32 = STRING32_REF(*this).getData(len);
						outData.length = len;
						outData.charSize = 4;
						return;
					}
				case STRING_TYPE_SZ32:
					{
						outData.data32 = (sl_char32*)_value;
						outData.length = -1;
						outData.charSize = 4;
						return;
					}
				default:
					if (IS_STR8(_length)) {
						outData.data8 = (sl_char8*)_value;
						outData.length = _length;
						outData.charSize = 1;
						return;
					} else if (IS_STR16(_length)) {
						outData.data16 = (sl_char16*)_value;
						outData.length = GET_LENGTH(_length);
						outData.charSize = 2;
						return;
					} else if (IS_STR32(_length)) {
						outData.data32 = (sl_char32*)_value;
						outData.length = GET_LENGTH(_length);
						outData.charSize = 4;
						return;
					}
			}
		}
		outData.data = sl_null;
		outData.length = 0;
		outData.charSize = 0;
	}

	String StringParam::toString() const noexcept
	{
		return ToString<String>(*this);
	}

	String16 StringParam::toString16() const noexcept
	{
		return ToString<String16>(*this);
	}

	String32 StringParam::toString32() const noexcept
	{
		return ToString<String32>(*this);
	}

	String StringParam::newString() const noexcept
	{
		if (!_value) {
			return sl_null;
		}
		switch (_length) {
			case STRING_TYPE_STRING8_REF:
			case STRING_TYPE_STRING8_NOREF:
				return STRING_REF(*this).duplicate();
			default:
				break;
		}
		return toString();
	}

	String16 StringParam::newString16() const noexcept
	{
		if (!_value) {
			return sl_null;
		}
		switch (_length) {
			case STRING_TYPE_STRING16_REF:
			case STRING_TYPE_STRING16_NOREF:
				return STRING16_REF(*this).duplicate();
			default:
				break;
		}
		return toString16();
	}

	String32 StringParam::newString32() const noexcept
	{
		if (!_value) {
			return sl_null;
		}
		switch (_length) {
			case STRING_TYPE_STRING32_REF:
			case STRING_TYPE_STRING32_NOREF:
				return STRING32_REF(*this).duplicate();
			default:
				break;
		}
		return toString32();
	}

	Variant StringParam::toVariant() const noexcept
	{
		if (!_value) {
			return sl_null;
		}
		switch (_length) {
			case STRING_TYPE_STRING8_REF:
			case STRING_TYPE_STRING8_NOREF:
				return STRING_REF(*this);
			case STRING_TYPE_STRING16_REF:
			case STRING_TYPE_STRING16_NOREF:
				return STRING16_REF(*this);
			case STRING_TYPE_STRING32_REF:
			case STRING_TYPE_STRING32_NOREF:
				return STRING32_REF(*this);
			case STRING_TYPE_SZ8:
				return (sl_char8*)_value;
			case STRING_TYPE_SZ16:
				return (sl_char16*)_value;
			case STRING_TYPE_SZ32:
				return (sl_char32*)_value;
			default:
				if (IS_STR8(_length)) {
					return StringView((sl_char8*)_value, _length);
				} else if (IS_STR16(_length)) {
					return StringView16((sl_char16*)_value, GET_LENGTH(_length));
				} else if (IS_STR32(_length)) {
					return StringView32((sl_char32*)_value, GET_LENGTH(_length));
				} else {
					return sl_null;
				}
		}
	}

	sl_size StringParam::getHashCode() const noexcept
	{
		if (!_value) {
			return 0;
		}
		switch (_length) {
			case STRING_TYPE_STRING8_REF:
			case STRING_TYPE_STRING8_NOREF:
				return STRING_REF(*this).getHashCode();
			case STRING_TYPE_STRING16_REF:
			case STRING_TYPE_STRING16_NOREF:
				return STRING16_REF(*this).getHashCode();
			case STRING_TYPE_STRING32_REF:
			case STRING_TYPE_STRING32_NOREF:
				return STRING32_REF(*this).getHashCode();
			case STRING_TYPE_SZ8:
				return String::getHashCode((sl_char8*)_value);
			case STRING_TYPE_SZ16:
				return String16::getHashCode((sl_char16*)_value);
			case STRING_TYPE_SZ32:
				return String32::getHashCode((sl_char32*)_value);
			default:
				if (IS_STR8(_length)) {
					return String::getHashCode((sl_char8*)_value, _length);
				} else if (IS_STR16(_length)) {
					return String16::getHashCode((sl_char16*)_value, GET_LENGTH(_length));
				} else if (IS_STR32(_length)) {
					return String32::getHashCode((sl_char32*)_value, GET_LENGTH(_length));
				} else {
					return 0;
				}
		}
	}

	namespace {

		template <class BASE>
		class StringDataHelper : public BASE
		{
		public:
			typedef typename BASE::StringType StringType;
			typedef typename BASE::Char Char;

			using BASE::data;
			using BASE::length;
			using BASE::string;

		public:
			SLIB_INLINE void construct(const StringType& str) noexcept
			{
				data = str.getData(*((sl_size*)&length));
			}

			SLIB_INLINE void constructMove(StringType& str, StringParam& param) noexcept
			{
				new (&string) StringType(Move(str));
				data = string.getData(*((sl_size*)&length));
				param._value = sl_null;
				param._length = 0;
			}

			SLIB_INLINE void construct(typename StringTypeFromCharType<typename OtherCharType<Char>::Type1>::Type const& str) noexcept
			{
				new (&string) StringType(StringType::create(str));
				data = string.getData(*((sl_size*)&length));
			}

			SLIB_INLINE void constructMove(typename StringTypeFromCharType<typename OtherCharType<Char>::Type1>::Type& str, StringParam& param) noexcept
			{
				construct(str);
			}

			SLIB_INLINE void construct(typename StringTypeFromCharType<typename OtherCharType<Char>::Type2>::Type const& str
			) noexcept
			{
				new (&string) StringType(StringType::create(str));
				data = string.getData(*((sl_size*)&length));
			}

			SLIB_INLINE void constructMove(typename StringTypeFromCharType<typename OtherCharType<Char>::Type2>::Type& str, StringParam& param) noexcept
			{
				construct(str);
			}

			SLIB_INLINE void construct(const Char* str, sl_reg len) noexcept
			{
				data = (Char*)str;
				length = len;
			}

			SLIB_INLINE void construct(typename OtherCharType<Char>::Type1 const* str, sl_reg len) noexcept
			{
				new (&string) StringType(StringType::create(str, len));
				data = string.getData(*((sl_size*)&length));
			}

			SLIB_INLINE void construct(typename OtherCharType<Char>::Type2 const* str, sl_reg len) noexcept
			{
				new (&string) StringType(StringType::create(str, len));
				data = string.getData(*((sl_size*)&length));
			}

		};

		template <class BASE>
		class StringCstrHelper : public BASE
		{
		public:
			typedef typename BASE::StringType StringType;
			typedef typename BASE::Char Char;

			using BASE::data;
			using BASE::length;
			using BASE::string;

		public:
			SLIB_INLINE void construct(const StringType& str) noexcept
			{
				data = str.getNullTerminatedData(*((sl_size*)&length), string);
			}

			SLIB_INLINE void constructMove(StringType& str, StringParam& param) noexcept
			{
				new (&string) StringType(Move(str));
				data = string.getNullTerminatedData(*((sl_size*)&length), string);
				param._value = sl_null;
				param._length = 0;
			}

			SLIB_INLINE void construct(typename StringTypeFromCharType<typename OtherCharType<Char>::Type1>::Type const& str) noexcept
			{
				new (&string) StringType(StringType::create(str));
				data = string.getData(*((sl_size*)&length));
			}

			SLIB_INLINE void constructMove(typename StringTypeFromCharType<typename OtherCharType<Char>::Type1>::Type& str, StringParam& param) noexcept
			{
				construct(str);
			}

			SLIB_INLINE void construct(typename StringTypeFromCharType<typename OtherCharType<Char>::Type2>::Type const& str) noexcept
			{
				new (&string) StringType(StringType::create(str));
				data = string.getData(*((sl_size*)&length));
			}

			SLIB_INLINE void constructMove(typename StringTypeFromCharType<typename OtherCharType<Char>::Type2>::Type& str, StringParam& param) noexcept
			{
				construct(str);
			}

			SLIB_INLINE void construct(const Char* str, sl_reg len) noexcept
			{
				if (len >= 0 && str[len]) {
					new (&string) StringType(str, len);
					data = string.getData(*((sl_size*)&length));
				} else {
					data = (Char*)str;
					length = len;
				}
			}

			SLIB_INLINE void construct(typename OtherCharType<Char>::Type1 const* str, sl_reg len) noexcept
			{
				new (&string) StringType(StringType::create(str, len));
				data = string.getData(*((sl_size*)&length));
			}

			SLIB_INLINE void construct(typename OtherCharType<Char>::Type2 const* str, sl_reg len) noexcept
			{
				new (&string) StringType(StringType::create(str, len));
				data = string.getData(*((sl_size*)&length));
			}

		};

		template <class HELPER>
		static void ConstructData(HELPER& data, const StringParam& param) noexcept
		{
			if (param._value) {
				switch (param._length) {
					case STRING_TYPE_STRING8_REF:
					case STRING_TYPE_STRING8_NOREF:
						data.construct(STRING_REF(param));
						return;
					case STRING_TYPE_STRING16_REF:
					case STRING_TYPE_STRING16_NOREF:
						data.construct(STRING16_REF(param));
						return;
					case STRING_TYPE_STRING32_REF:
					case STRING_TYPE_STRING32_NOREF:
						data.construct(STRING32_REF(param));
						return;
					case STRING_TYPE_SZ8:
						data.construct((sl_char8*)(param._value), -1);
						return;
					case STRING_TYPE_SZ16:
						data.construct((sl_char16*)(param._value), -1);
						return;
					case STRING_TYPE_SZ32:
						data.construct((sl_char32*)(param._value), -1);
						return;
					default:
						if (IS_STR8(param._length)) {
							data.construct((sl_char8*)(param._value), param._length);
							return;
						} else if (IS_STR16(param._length)) {
							data.construct((sl_char16*)(param._value), GET_LENGTH(param._length));
							return;
						} else if (IS_STR32(param._length)) {
							data.construct((sl_char32*)(param._value), GET_LENGTH(param._length));
							return;
						}
						break;
				}
			}
			data.setNull();
		}

		template <class HELPER>
		void ConstructDataMove(HELPER& data, StringParam& param) noexcept
		{
			if (param._value) {
				switch (param._length) {
					case STRING_TYPE_STRING8_REF:
						data.constructMove(STRING_REF(param), param);
						return;
					case STRING_TYPE_STRING16_REF:
						data.constructMove(STRING16_REF(param), param);
						return;
					case STRING_TYPE_STRING32_REF:
						data.constructMove(STRING32_REF(param), param);
						return;
					default:
						ConstructData(data, param);
						return;
				}
			}
			data.setNull();
		}

		template <class DATA>
		SLIB_INLINE static void ConstructStringData(DATA& data, const StringParam& param) noexcept
		{
			ConstructData(*((StringDataHelper<DATA>*)&data), param);
		}

		template <class DATA>
		SLIB_INLINE static void ConstructStringDataMove(DATA& data, StringParam& param) noexcept
		{
			ConstructDataMove(*((StringDataHelper<DATA>*)&data), param);
		}

		template <class CSTR>
		SLIB_INLINE static void ConstructStringCstr(CSTR& cstr, const StringParam& param) noexcept
		{
			ConstructData(*((StringCstrHelper<CSTR>*)&cstr), param);
		}

		template <class CSTR>
		SLIB_INLINE static void ConstructStringCstrMove(CSTR& cstr, StringParam& param) noexcept
		{
			ConstructDataMove(*((StringCstrHelper<CSTR>*)&cstr), param);
		}

	}

#define DEFINE_STRING_DATA_MEMBERS(DATA, BASE) \
	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(DATA) \
	\
	DATA::DATA(typename DATA::Char const* str) noexcept: BASE(str) {} \
	\
	DATA::DATA(typename OtherCharType<DATA::Char>::Type1 const* str) noexcept: string(StringType::create(str)) \
	{ \
		data = string.getData(*((sl_size*)&length)); \
	} \
	\
	DATA::DATA(typename OtherCharType<DATA::Char>::Type2 const* str) noexcept: string(StringType::create(str)) \
	{ \
		data = string.getData(*((sl_size*)&length)); \
	} \
	\
	DATA::DATA(typename DATA::Char const* str, sl_size length) noexcept: BASE(str, length) {} \
	\
	DATA::DATA(typename OtherCharType<DATA::Char>::Type1 const* str, sl_size length) noexcept: string(StringType::create(str, length)) \
	{ \
		data = string.getData(*((sl_size*)&length)); \
	} \
	\
	DATA::DATA(typename OtherCharType<DATA::Char>::Type2 const* str, sl_size length) noexcept: string(StringType::create(str, length)) \
	{ \
		data = string.getData(*((sl_size*)&length)); \
	} \
	\
	DATA::DATA(typename DATA::StringType const& str) noexcept: BASE(str) {} \
	\
	DATA::DATA(typename DATA::StringType&& str) noexcept: string(Move(str)) \
	{ \
		data = string.getData(*((sl_size*)&length)); \
	} \
	\
	DATA::DATA(typename StringTypeFromCharType<OtherCharType<DATA::Char>::Type1>::Type const& str) noexcept: string(StringType::create(str)) \
	{ \
		if (str.isNotNull()) { \
			data = string.getData(*((sl_size*)&length)); \
		} \
	} \
	\
	DATA::DATA(typename StringTypeFromCharType<OtherCharType<DATA::Char>::Type2>::Type const& str) noexcept: string(StringType::create(str)) \
	{ \
		if (str.isNotNull()) { \
			data = string.getData(*((sl_size*)&length)); \
		} \
	} \
	\
	DATA::DATA(Atomic<String> const& str) noexcept: DATA(String(str)) {} \
	DATA::DATA(Atomic<String16> const& str) noexcept: DATA(String16(str)) {} \
	DATA::DATA(Atomic<String32> const& str) noexcept: DATA(String32(str)) {} \
	\
	DATA::DATA(const BASE& str) noexcept: BASE(str) {} \
	\
	DATA::DATA(typename StringViewTypeFromCharType<OtherCharType<DATA::Char>::Type1>::Type const& str) noexcept: string(StringType::create(str)) \
	{ \
		if (str.isNotNull()) { \
			data = string.getData(*((sl_size*)&length)); \
		} \
	} \
	\
	DATA::DATA(typename StringViewTypeFromCharType<OtherCharType<DATA::Char>::Type2>::Type const& str) noexcept: string(StringType::create(str)) \
	{ \
		if (str.isNotNull()) { \
			data = string.getData(*((sl_size*)&length)); \
		} \
	} \
	\
	DATA::DATA(const StringParam& param) noexcept \
	{ \
		ConstructStringData(*this, param); \
	} \
	\
	DATA::DATA(StringParam&& param) noexcept \
	{ \
		ConstructStringDataMove(*this, param); \
	}

	DEFINE_STRING_DATA_MEMBERS(StringData, StringView)
	DEFINE_STRING_DATA_MEMBERS(StringData16, StringView16)
	DEFINE_STRING_DATA_MEMBERS(StringData32, StringView32)

	String StringData::toString(const StringParam& param)
	{
		if (string.isNotNull()) {
			return string;
		} else {
			return param.toString();
		}
	}

	String16 StringData16::toString16(const StringParam& param)
	{
		if (string.isNotNull()) {
			return string;
		} else {
			return param.toString16();
		}
	}

	String32 StringData32::toString32(const StringParam& param)
	{
		if (string.isNotNull()) {
			return string;
		} else {
			return param.toString32();
		}
	}


#define DEFINE_STRING_CSTR_MEMBERS(CSTR, BASE) \
	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(CSTR) \
	\
	CSTR::CSTR() noexcept {} \
	\
	CSTR::CSTR(typename CSTR::Char const* str) noexcept: BASE(str) {} \
	\
	CSTR::CSTR(typename OtherCharType<CSTR::Char>::Type1 const* str) noexcept: string(StringType::create(str)) \
	{ \
		data = string.getData(*((sl_size*)&length)); \
	} \
	\
	CSTR::CSTR(typename OtherCharType<CSTR::Char>::Type2 const* str) noexcept: string(StringType::create(str)) \
	{ \
		data = string.getData(*((sl_size*)&length)); \
	} \
	\
	CSTR::CSTR(typename CSTR::Char const* str, sl_size _length) noexcept: BASE(str, _length) \
	{ \
		if (str) { \
			if (_length > 0) { \
				if (str[_length]) { \
					new (&string) StringType(str, _length); \
					data = string.getData(*((sl_size*)&length)); \
				} \
			} else if (!_length) { \
				data = (Char*)((void*)"\0\0\0\0"); \
			} \
		} \
	} \
	\
	CSTR::CSTR(typename OtherCharType<CSTR::Char>::Type1 const* str, sl_size length) noexcept: string(StringType::create(str, length)) \
	{ \
		data = string.getData(*((sl_size*)&length)); \
	} \
	\
	CSTR::CSTR(typename OtherCharType<CSTR::Char>::Type2 const* str, sl_size length) noexcept: string(StringType::create(str, length)) \
	{ \
		data = string.getData(*((sl_size*)&length)); \
	} \
	\
	CSTR::CSTR(typename CSTR::StringType const& str) noexcept \
	{ \
		if (str.isNotNull()) { \
			data = str.getNullTerminatedData(*((sl_size*)&length), string); \
		} \
	} \
	\
	CSTR::CSTR(typename CSTR::StringType&& str) noexcept: string(Move(str)) \
	{ \
		data = string.getNullTerminatedData(*((sl_size*)&length), string); \
	} \
	\
	CSTR::CSTR(typename StringTypeFromCharType<OtherCharType<CSTR::Char>::Type1>::Type const& str) noexcept: string(StringType::create(str)) \
	{ \
		if (str.isNotNull()) { \
			data = string.getData(*((sl_size*)&length)); \
		} \
	} \
	\
	CSTR::CSTR(typename StringTypeFromCharType<OtherCharType<CSTR::Char>::Type2>::Type const& str) noexcept: string(StringType::create(str)) \
	{ \
		if (str.isNotNull()) { \
			data = string.getData(*((sl_size*)&length)); \
		} \
	} \
	\
	CSTR::CSTR(const StringParam& param) noexcept \
	{ \
		ConstructStringCstr(*this, param); \
	} \
	\
	CSTR::CSTR(StringParam&& param) noexcept \
	{ \
		ConstructStringCstrMove(*this, param); \
	} \
	\
	CSTR::CSTR(const Atomic<String>& str) noexcept: CSTR(String(str)) {} \
	CSTR::CSTR(const Atomic<String16>& str) noexcept: CSTR(String16(str)) {} \
	CSTR::CSTR(const Atomic<String32>& str) noexcept: CSTR(String32(str)) {} \
	\
	CSTR::CSTR(typename StringViewTypeFromCharType<CSTR::Char>::Type const& str) noexcept: BASE(str.getUnsafeData(), str.getUnsafeLength()) \
	{ \
		if (data) { \
			if (length > 0) { \
				if (data[length]) { \
					new (&string) StringType(data, length); \
					data = string.getData(*((sl_size*)&length)); \
				} \
			} else if (!length) { \
				data = (Char*)((void*)"\0\0\0\0"); \
			} \
		} \
	} \
	\
	CSTR::CSTR(typename StringViewTypeFromCharType<OtherCharType<CSTR::Char>::Type1>::Type const& str) noexcept: string(StringType::create(str)) \
	{ \
		if (str.isNotNull()) { \
			data = string.getData(*((sl_size*)&length)); \
		} \
	} \
	\
	CSTR::CSTR(typename StringViewTypeFromCharType<OtherCharType<CSTR::Char>::Type2>::Type const& str) noexcept: string(StringType::create(str)) \
	{ \
		if (str.isNotNull()) { \
			data = string.getData(*((sl_size*)&length)); \
		} \
	}

	DEFINE_STRING_CSTR_MEMBERS(StringCstr, StringView)
	DEFINE_STRING_CSTR_MEMBERS(StringCstr16, StringView16)
	DEFINE_STRING_CSTR_MEMBERS(StringCstr32, StringView32)

	String StringCstr::toString(const StringParam& param)
	{
		if (string.isNotNull()) {
			return string;
		} else {
			return param.toString();
		}
	}

	String16 StringCstr16::toString16(const StringParam& param)
	{
		if (string.isNotNull()) {
			return string;
		} else {
			return param.toString16();
		}
	}

	String32 StringCstr32::toString32(const StringParam& param)
	{
		if (string.isNotNull()) {
			return string;
		} else {
			return param.toString32();
		}
	}

}
