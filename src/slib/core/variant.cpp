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

#define SLIB_SUPPORT_STD_TYPES

#include "slib/data/json.h"

#include "slib/core/string_buffer.h"
#include "slib/core/stringx.h"
#include "slib/core/object_op.h"
#include "slib/core/priv/list_collection.h"
#include "slib/core/priv/map_object.h"
#include "slib/data/serialize.h"
#include "slib/math/math.h"
#include "slib/math/bigint.h"

#define PTR_VAR(TYPE, x) ((TYPE*)((void*)(&(x))))
#define REF_VAR(TYPE, x) (*PTR_VAR(TYPE, x))

namespace slib
{

	namespace priv
	{
		namespace variant
		{
			const ConstContainer g_undefined = {0, {0}, VariantType::Null, 0};
			const ConstContainer g_null = {1, {0}, VariantType::Null, 0};
		}
	}

	namespace {

		SLIB_INLINE static void CopyBytes16(void* _dst, const void* _src)
		{
			sl_uint64* dst = (sl_uint64*)_dst;
			const sl_uint64* src = (const sl_uint64*)_src;
			*(dst++) = *(src++);
			*dst = *src;
		}

		SLIB_INLINE static void CopyBytes12(Variant& dst, const void* src)
		{
			dst._value = *((const sl_uint64*)src);
			dst._value2 = *(((const sl_uint32*)src) + 2);
		}

		SLIB_INLINE static void ZeroBytes12(void* dst)
		{
			*((sl_uint64*)dst) = 0;
			*((sl_uint32*)dst + 2) = 0;
		}

		SLIB_INLINE static sl_bool IsRef(sl_uint8 type)
		{
			return type >= VariantType::Ref;
		}

		SLIB_INLINE static sl_bool IsStringType(sl_uint8 type)
		{
			return type >= VariantType::String8 && type <= VariantType::StringData32;
		}

		SLIB_INLINE static sl_bool IsStringViewType(sl_uint8 type)
		{
			return type >= VariantType::Sz8 && type <= VariantType::StringData32;
		}

		SLIB_INLINE static void Init(Variant& var, sl_uint8 type)
		{
			var._tag = 0;
			var._type = type;
		}

		static void Copy(Variant& dst, const Variant& src) noexcept
		{
			dst._tag = src._tag;
			sl_uint8 type = src._type;
			dst._type = type;
			switch (type) {
				case VariantType::String8:
					new PTR_VAR(String, dst._value) String(REF_VAR(String, src._value));
					break;
				case VariantType::String16:
					new PTR_VAR(String16, dst._value) String16(REF_VAR(String16, src._value));
					break;
				case VariantType::String32:
					new PTR_VAR(String32, dst._value) String32(REF_VAR(String32, src._value));
					break;
				case VariantType::ObjectId:
				case VariantType::StringData8:
				case VariantType::StringData16:
				case VariantType::StringData32:
					dst._value = src._value;
					dst._value2 = src._value2;
					break;
				default:
					if (IsRef(type)) {
						new PTR_VAR(Ref<CRef>, dst._value) Ref<CRef>(REF_VAR(Ref<CRef>, src._value));
					} else {
						dst._value = src._value;
					}
					break;
			}
		}

		static void Free(sl_uint8 type, sl_uint64 value) noexcept
		{
			switch (type)
			{
				case VariantType::String8:
					REF_VAR(String, value).String::~String();
					break;
				case VariantType::String16:
					REF_VAR(String16, value).String16::~String16();
					break;
				case VariantType::String32:
					REF_VAR(String32, value).String32::~String32();
					break;
				default:
					if (IsRef(type)) {
						REF_VAR(Ref<CRef>, value).Ref<CRef>::~Ref();
					}
					break;
			}
		}

	}

	void Variant::_assign(const Variant& other) noexcept
	{
		if (this != &other) {
			Free(_type, _value);
			Copy(*this, other);
		}
	}

	void Variant::_assignMove(Variant& other) noexcept
	{
		if (this != &other) {
			Free(_type, _value);
			CopyBytes16(this, &other);
			other._type = VariantType::Null;
		}
	}

	void Variant::_constructorRef(const void* ptr, sl_uint8 type) noexcept
	{
		const Ref<CRef>& ref = *reinterpret_cast<Ref<CRef> const*>(ptr);
		if (ref.isNotNull()) {
			Init(*this, type);
			new (reinterpret_cast<Ref<CRef>*>(&_value)) Ref<CRef>(ref);
		} else {
			Init(*this, VariantType::Null);
			_value = 1;
		}
	}

	void Variant::_constructorMoveRef(void* ptr, sl_uint8 type) noexcept
	{
		Ref<CRef>& ref = *reinterpret_cast<Ref<CRef>*>(ptr);
		if (ref.isNotNull()) {
			Init(*this, type);
			new (reinterpret_cast<Ref<CRef>*>(&_value)) Ref<CRef>(Move(ref));
		} else {
			Init(*this, VariantType::Null);
			_value = 1;
		}
	}

	void Variant::_assignRef(const void* ptr, sl_uint8 type) noexcept
	{
		Free(_type, _value);
		_constructorRef(ptr, type);
	}

	void Variant::_assignMoveRef(void* ptr, sl_uint8 type) noexcept
	{
		Free(_type, _value);
		_constructorMoveRef(ptr, type);
	}

	void Variant::_free(sl_uint8 type, sl_uint64 value) noexcept
	{
		Free(type, value);
	}

	Variant::Variant(const Variant& other) noexcept
	{
		Copy(*this, other);
	}

	Variant::Variant(Variant&& other) noexcept
	{
		CopyBytes16(this, &other);
		other._type = VariantType::Null;
	}

	Variant::Variant(const Atomic<Variant>& other) noexcept
	{
		other._retain_construct(this);
	}

	Variant::Variant(const Json& other) noexcept
	{
		Copy(*this, other);
	}

	Variant::Variant(Json&& other) noexcept
	{
		CopyBytes16(this, &other);
		other._type = VariantType::Null;
	}

	Variant::~Variant() noexcept
	{
		Free(_type, _value);
	}

	Variant::Variant(signed char value) noexcept
	{
		Init(*this, VariantType::Int32);
		REF_VAR(sl_int32, _value) = value;
	}

	Variant::Variant(unsigned char value) noexcept
	{
		Init(*this, VariantType::Uint32);
		REF_VAR(sl_uint32, _value) = value;
	}

	Variant::Variant(char value) noexcept
	{
		Init(*this, VariantType::Int32);
		REF_VAR(sl_int32, _value) = (sl_int32)value;
	}

	Variant::Variant(short value) noexcept
	{
		Init(*this, VariantType::Int32);
		REF_VAR(sl_int32, _value) = value;
	}

	Variant::Variant(unsigned short value) noexcept
	{
		Init(*this, VariantType::Uint32);
		REF_VAR(sl_uint32, _value) = value;
	}

	Variant::Variant(int value) noexcept
	{
		Init(*this, VariantType::Int32);
		REF_VAR(sl_int32, _value) = (sl_int32)value;
	}

	Variant::Variant(unsigned int value) noexcept
	{
		Init(*this, VariantType::Uint32);
		REF_VAR(sl_uint32, _value) = (sl_uint32)value;
	}

	Variant::Variant(long value) noexcept
	{
#if SLIB_LONG_SIZE == 8
		Init(*this, VariantType::Int64);
		REF_VAR(sl_int64, _value) = (sl_int64)value;
#else
		Init(*this, VariantType::Int32);
		REF_VAR(sl_int32, _value) = (sl_int32)value;
#endif
	}

	Variant::Variant(unsigned long value) noexcept
	{
#if SLIB_LONG_SIZE == 8
		Init(*this, VariantType::Uint64);
		REF_VAR(sl_uint64, _value) = (sl_uint64)value;
#else
		Init(*this, VariantType::Uint32);
		REF_VAR(sl_uint32, _value) = (sl_uint32)value;
#endif
	}

	Variant::Variant(sl_int64 value) noexcept
	{
		Init(*this, VariantType::Int64);
		REF_VAR(sl_int64, _value) = value;
	}

	Variant::Variant(sl_uint64 value) noexcept
	{
		Init(*this, VariantType::Uint64);
		REF_VAR(sl_uint64, _value) = value;
	}

	Variant::Variant(sl_char16 value) noexcept
	{
		Init(*this, VariantType::Uint32);
		REF_VAR(sl_uint32, _value) = (sl_uint32)value;
	}

	Variant::Variant(sl_char32 value) noexcept
	{
		Init(*this, VariantType::Uint32);
		REF_VAR(sl_uint32, _value) = (sl_uint32)value;
	}

	Variant::Variant(float value) noexcept
	{
		Init(*this, VariantType::Float);
		REF_VAR(float, _value) = value;
	}

	Variant::Variant(double value) noexcept
	{
		Init(*this, VariantType::Double);
		REF_VAR(double, _value) = value;
	}

	Variant::Variant(sl_bool value) noexcept
	{
		Init(*this, VariantType::Boolean);
		REF_VAR(sl_bool, _value) = value;
	}

	Variant::Variant(const String& value) noexcept
	{
		if (value.isNotNull()) {
			Init(*this, VariantType::String8);
			new PTR_VAR(String, _value) String(value);
		} else {
			Init(*this, VariantType::Null);
			_value = 1;
		}
	}

	Variant::Variant(String&& value) noexcept
	{
		if (value.isNotNull()) {
			Init(*this, VariantType::String8);
			new PTR_VAR(String, _value) String(Move(value));
		} else {
			Init(*this, VariantType::Null);
			_value = 1;
		}
	}

	Variant::Variant(const String16& value) noexcept
	{
		if (value.isNotNull()) {
			Init(*this, VariantType::String16);
			new PTR_VAR(String16, _value) String16(value);
		} else {
			Init(*this, VariantType::Null);
			_value = 1;
		}
	}

	Variant::Variant(String16&& value) noexcept
	{
		if (value.isNotNull()) {
			Init(*this, VariantType::String16);
			new PTR_VAR(String16, _value) String16(Move(value));
		} else {
			Init(*this, VariantType::Null);
			_value = 1;
		}
	}

	Variant::Variant(const String32& value) noexcept
	{
		if (value.isNotNull()) {
			Init(*this, VariantType::String32);
			new PTR_VAR(String32, _value) String32(value);
		} else {
			Init(*this, VariantType::Null);
			_value = 1;
		}
	}

	Variant::Variant(String32&& value) noexcept
	{
		if (value.isNotNull()) {
			Init(*this, VariantType::String32);
			new PTR_VAR(String32, _value) String32(Move(value));
		} else {
			Init(*this, VariantType::Null);
			_value = 1;
		}
	}

	Variant::Variant(const StringView& value) noexcept
	{
		if (value.isNotNull()) {
			sl_reg len = value.getUnsafeLength();
			if (len >= 0) {
				Init(*this, VariantType::StringData8);
				_value2 = (sl_uint32)len;
				REF_VAR(const sl_char8*, _value) = value.getUnsafeData();
			} else {
				Init(*this, VariantType::Sz8);
				REF_VAR(const sl_char8*, _value) = value.getUnsafeData();
			}
		} else {
			Init(*this, VariantType::Null);
			_value = 1;
		}
	}

	Variant::Variant(const StringView16& value) noexcept
	{
		if (value.isNotNull()) {
			sl_reg len = value.getUnsafeLength();
			if (len >= 0) {
				Init(*this, VariantType::StringData16);
				_value2 = (sl_uint32)len;
				REF_VAR(const sl_char16*, _value) = value.getUnsafeData();
			} else {
				Init(*this, VariantType::Sz16);
				REF_VAR(const sl_char16*, _value) = value.getUnsafeData();
			}
		} else {
			Init(*this, VariantType::Null);
			_value = 1;
		}
	}

	Variant::Variant(const StringView32& value) noexcept
	{
		if (value.isNotNull()) {
			sl_reg len = value.getUnsafeLength();
			if (len >= 0) {
				Init(*this, VariantType::StringData32);
				_value2 = (sl_uint32)len;
				REF_VAR(const sl_char32*, _value) = value.getUnsafeData();
			} else {
				Init(*this, VariantType::Sz32);
				REF_VAR(const sl_char32*, _value) = value.getUnsafeData();
			}
		} else {
			Init(*this, VariantType::Null);
			_value = 1;
		}
	}

	Variant::Variant(const StringData& value) noexcept: Variant((const StringView&)value)
	{
	}

	Variant::Variant(const StringData16& value) noexcept: Variant((const StringView16&)value)
	{
	}

	Variant::Variant(const StringData32& value) noexcept: Variant((const StringView32&)value)
	{
	}

	Variant::Variant(const StringCstr& value) noexcept : Variant((const StringView&)value)
	{
	}

	Variant::Variant(const StringCstr16& value) noexcept : Variant((const StringView16&)value)
	{
	}

	Variant::Variant(const StringCstr32& value) noexcept : Variant((const StringView32&)value)
	{
	}

	Variant::Variant(const sl_char8* sz8) noexcept
	{
		if (sz8) {
			Init(*this, VariantType::Sz8);
			REF_VAR(const sl_char8*, _value) = sz8;
		} else {
			Init(*this, VariantType::Null);
			_value = 1;
		}
	}

	Variant::Variant(sl_char8* sz8) noexcept
	{
		if (sz8) {
			Init(*this, VariantType::Sz8);
			REF_VAR(sl_char8*, _value) = sz8;
		} else {
			Init(*this, VariantType::Null);
			_value = 1;
		}
	}

	Variant::Variant(const sl_char16* sz16) noexcept
	{
		if (sz16) {
			Init(*this, VariantType::Sz16);
			REF_VAR(const sl_char16*, _value) = sz16;
		} else {
			Init(*this, VariantType::Null);
			_value = 1;
		}
	}

	Variant::Variant(sl_char16* sz16) noexcept
	{
		if (sz16) {
			Init(*this, VariantType::Sz16);
			REF_VAR(sl_char16*, _value) = sz16;
		} else {
			Init(*this, VariantType::Null);
			_value = 1;
		}
	}

	Variant::Variant(const sl_char32* sz32) noexcept
	{
		if (sz32) {
			Init(*this, VariantType::Sz32);
			REF_VAR(const sl_char32*, _value) = sz32;
		} else {
			Init(*this, VariantType::Null);
			_value = 1;
		}
	}

	Variant::Variant(sl_char32* sz32) noexcept
	{
		if (sz32) {
			Init(*this, VariantType::Sz32);
			REF_VAR(sl_char32*, _value) = sz32;
		} else {
			Init(*this, VariantType::Null);
			_value = 1;
		}
	}

	Variant::Variant(const StringParam& str) noexcept: Variant(str.toVariant())
	{
	}

	Variant::Variant(const std::string& value) noexcept: Variant(String::create(value))
	{
	}

	Variant::Variant(const std::u16string& value) noexcept: Variant(String16::create(value))
	{
	}

	Variant::Variant(const std::u32string& value) noexcept : Variant(String32::create(value))
	{
	}

	Variant::Variant(const Time& value) noexcept
	{
		Init(*this, VariantType::Time);
		REF_VAR(Time, _value) = value;
	}

	Variant::Variant(const ObjectId& _id) noexcept
	{
		Init(*this, VariantType::ObjectId);
		CopyBytes12(*this, _id.data);
	}

	Variant::Variant(const VariantList& list) noexcept
	{
		_constructorRef(&list, VariantType::List);
	}

	Variant::Variant(VariantList&& list) noexcept
	{
		_constructorMoveRef(&list, VariantType::List);
	}

	Variant::Variant(const VariantMap& map) noexcept
	{
		_constructorRef(&map, VariantType::Map);
	}

	Variant::Variant(VariantMap&& map) noexcept
	{
		_constructorMoveRef(&map, VariantType::Map);
	}

	Variant::Variant(const JsonList& list) noexcept
	{
		_constructorRef(&list, VariantType::List);
	}

	Variant::Variant(JsonList&& list) noexcept
	{
		_constructorMoveRef(&list, VariantType::List);
	}

	Variant::Variant(const JsonMap& map) noexcept
	{
		_constructorRef(&map, VariantType::Map);
	}

	Variant::Variant(JsonMap&& map) noexcept
	{
		_constructorMoveRef(&map, VariantType::Map);
	}

	Variant::Variant(const Memory& mem) noexcept
	{
		_constructorRef(&mem, VariantType::Memory);
	}

	Variant::Variant(Memory&& mem) noexcept
	{
		_constructorMoveRef(&mem, VariantType::Memory);
	}

	Variant::Variant(const BigInt& n) noexcept
	{
		_constructorRef(&n, VariantType::BigInt);
	}

	Variant::Variant(BigInt&& n) noexcept
	{
		_constructorMoveRef(&n, VariantType::BigInt);
	}

	Variant::Variant(const Promise<Variant>& promise) noexcept
	{
		_constructorRef(&promise, VariantType::Promise);
	}

	Variant::Variant(Promise<Variant>&& promise) noexcept
	{
		_constructorMoveRef(&promise, VariantType::Promise);
	}

	Variant& Variant::operator=(const Variant& other) noexcept
	{
		_assign(other);
		return *this;
	}

	Variant& Variant::operator=(Variant&& other) noexcept
	{
		_assignMove(other);
		return *this;
	}

	Variant& Variant::operator=(const Atomic<Variant>& other) noexcept
	{
		return *this = Variant(other);
	}

	Variant& Variant::operator=(const Json& other) noexcept
	{
		_assign(other);
		return *this;
	}

	Variant& Variant::operator=(Json&& other) noexcept
	{
		_assignMove(other);
		return *this;
	}

	Variant& Variant::operator=(sl_null_t) noexcept
	{
		setNull();
		return *this;
	}

#define VARIANT_OPERATOR_CALL_REF(OP_NAME) \
	if (IsRef(_type)) { \
		Ref<CRef> ref = getRef(); \
		if (ref.isNotNull()) { \
			Variant result; \
			if (ref->runOperator(ObjectOperator::OP_NAME, result, other, sl_true)) { \
				return result; \
			} \
		} \
	} \
	if (IsRef(other._type)) { \
		Ref<CRef> ref = other.getRef(); \
		if (ref.isNotNull()) { \
			Variant result; \
			if (ref->runOperator(ObjectOperator::OP_NAME, result, *this, sl_false)) { \
				return result; \
			} \
		} \
	}

	Variant Variant::operator+(const Variant& other) const noexcept
	{
		if (_type == other._type) {
			switch (_type) {
				case VariantType::Null:
					return sl_null;
				case VariantType::Int32:
					return REF_VAR(sl_int32 const, _value) + REF_VAR(sl_int32 const, other._value);
				case VariantType::Uint32:
					return REF_VAR(sl_uint32 const, _value) + REF_VAR(sl_uint32 const, other._value);
				case VariantType::Int64:
				case VariantType::Time:
					return REF_VAR(sl_int64 const, _value) + REF_VAR(sl_int64 const, other._value);
				case VariantType::Uint64:
					return REF_VAR(sl_uint64 const, _value) + REF_VAR(sl_uint64 const, other._value);
				case VariantType::Float:
					return REF_VAR(float const, _value) + REF_VAR(float const, other._value);
				case VariantType::Double:
					return REF_VAR(double const, _value) + REF_VAR(double const, other._value);
				case VariantType::String8:
					return REF_VAR(String const, _value) + REF_VAR(String const, other._value);
				case VariantType::String16:
					return REF_VAR(String16 const, _value) + REF_VAR(String16 const, other._value);
				case VariantType::String32:
					return REF_VAR(String32 const, _value) + REF_VAR(String32 const, other._value);
				case VariantType::Sz8:
					return StringView(REF_VAR(sl_char8 const* const, _value)) + StringView(REF_VAR(sl_char8 const* const, other._value));
				case VariantType::Sz16:
					return StringView16(REF_VAR(sl_char16 const* const, _value)) + StringView16(REF_VAR(sl_char16 const* const, other._value));
				case VariantType::Sz32:
					return StringView32(REF_VAR(sl_char32 const* const, _value)) + StringView32(REF_VAR(sl_char32 const* const, other._value));
				case VariantType::StringData8:
					return StringView(REF_VAR(sl_char8 const* const, _value), _value2) + StringView(REF_VAR(sl_char8 const* const, other._value), other._value2);
				case VariantType::StringData16:
					return StringView16(REF_VAR(sl_char16 const* const, _value), _value2) + StringView16(REF_VAR(sl_char16 const* const, other._value), other._value2);
				case VariantType::StringData32:
					return StringView32(REF_VAR(sl_char32 const* const, _value), _value2) + StringView32(REF_VAR(sl_char32 const* const, other._value), other._value2);
				case VariantType::BigInt:
					return REF_VAR(BigInt const, _value) + REF_VAR(BigInt const, other._value);
				default:
					break;
			}
		} else {
			if (other._type == VariantType::Null) {
				return *this;
			}
			switch (_type) {
				case VariantType::Null:
					return other;
				case VariantType::Int32:
				case VariantType::Uint32:
				case VariantType::Int64:
				case VariantType::Uint64:
					if (other.isIntegerType()) {
						return getInt64() + other.getInt64();
					}
					if (other._type == VariantType::Float || other._type == VariantType::Double) {
						return getDouble() + other.getDouble();
					}
					break;
				case VariantType::Float:
				case VariantType::Double:
					if (other.isNumberType()) {
						return getDouble() + other.getDouble();
					}
					break;
				case VariantType::String8:
				case VariantType::Sz8:
				case VariantType::StringData8:
					if (other.is8BitsStringType()) {
						return getStringView() + other.getStringView();
					}
					break;
				case VariantType::String16:
				case VariantType::Sz16:
				case VariantType::StringData16:
					if (other.is16BitsStringType()) {
						return getStringView16() + other.getStringView16();
					}
					break;
				case VariantType::String32:
				case VariantType::Sz32:
				case VariantType::StringData32:
					if (other.is8BitsStringType()) {
						return getStringView32() + other.getStringView32();
					}
					break;
				default:
					break;
			}
		}
		VARIANT_OPERATOR_CALL_REF(Add)
		return toString() + other.toString();
	}

	Variant Variant::operator-(const Variant& other) const noexcept
	{
		if (_type == other._type) {
			switch (_type) {
				case VariantType::Null:
					return sl_null;
				case VariantType::Int32:
					return REF_VAR(sl_int32 const, _value) - REF_VAR(sl_int32 const, other._value);
				case VariantType::Uint32:
					return REF_VAR(sl_uint32 const, _value) - REF_VAR(sl_uint32 const, other._value);
				case VariantType::Int64:
				case VariantType::Time:
					return REF_VAR(sl_int64 const, _value) - REF_VAR(sl_int64 const, other._value);
				case VariantType::Uint64:
					return REF_VAR(sl_uint64 const, _value) - REF_VAR(sl_uint64 const, other._value);
				case VariantType::Float:
					return REF_VAR(float const, _value) - REF_VAR(float const, other._value);
				case VariantType::Double:
					return REF_VAR(double const, _value) - REF_VAR(double const, other._value);
				case VariantType::BigInt:
					return REF_VAR(BigInt const, _value) - REF_VAR(BigInt const, other._value);
				default:
					break;
			}
		} else {
			if (other._type == VariantType::Null) {
				return *this;
			}
			switch (_type) {
				case VariantType::Null:
					return -other;
				case VariantType::Int32:
				case VariantType::Uint32:
				case VariantType::Int64:
				case VariantType::Uint64:
					if (other.isIntegerType()) {
						return getInt64() - other.getInt64();
					}
					if (other._type == VariantType::Float || other._type == VariantType::Double) {
						return getDouble() - other.getDouble();
					}
					break;
				case VariantType::Float:
				case VariantType::Double:
					if (other.isNumberType()) {
						return getDouble() - other.getDouble();
					}
					break;
				default:
					break;
			}
		}
		VARIANT_OPERATOR_CALL_REF(Subtract)
		return Variant();
	}

	Variant Variant::operator*(const Variant& other) const noexcept
	{
		if (_type == other._type) {
			switch (_type) {
				case VariantType::Null:
					return sl_null;
				case VariantType::Int32:
					return REF_VAR(sl_int32 const, _value) * REF_VAR(sl_int32 const, other._value);
				case VariantType::Uint32:
					return REF_VAR(sl_uint32 const, _value) * REF_VAR(sl_uint32 const, other._value);
				case VariantType::Int64:
					return REF_VAR(sl_int64 const, _value) * REF_VAR(sl_int64 const, other._value);
				case VariantType::Uint64:
					return REF_VAR(sl_uint64 const, _value) * REF_VAR(sl_uint64 const, other._value);
				case VariantType::Float:
					return REF_VAR(float const, _value) * REF_VAR(float const, other._value);
				case VariantType::Double:
					return REF_VAR(double const, _value) * REF_VAR(double const, other._value);
				case VariantType::BigInt:
					return REF_VAR(BigInt const, _value) * REF_VAR(BigInt const, other._value);
				default:
					break;
			}
		} else {
			if (other._type == VariantType::Null) {
				return sl_null;
			}
			switch (_type) {
				case VariantType::Null:
					return sl_null;
				case VariantType::Int32:
				case VariantType::Uint32:
				case VariantType::Int64:
				case VariantType::Uint64:
					if (other.isIntegerType()) {
						return getInt64() * other.getInt64();
					}
					if (other._type == VariantType::Float || other._type == VariantType::Double) {
						return getDouble() * other.getDouble();
					}
					break;
				case VariantType::Float:
				case VariantType::Double:
					if (other.isNumberType()) {
						return getDouble() * other.getDouble();
					}
					break;
				default:
					break;
			}
		}
		VARIANT_OPERATOR_CALL_REF(Multiply)
		return Variant();
	}

	Variant Variant::operator/(const Variant& other) const noexcept
	{
		if (_type == other._type) {
			switch (_type) {
				case VariantType::Int32:
					return REF_VAR(sl_int32 const, _value) / REF_VAR(sl_int32 const, other._value);
				case VariantType::Uint32:
					return REF_VAR(sl_uint32 const, _value) / REF_VAR(sl_uint32 const, other._value);
				case VariantType::Int64:
					return REF_VAR(sl_int64 const, _value) / REF_VAR(sl_int64 const, other._value);
				case VariantType::Uint64:
					return REF_VAR(sl_uint64 const, _value) / REF_VAR(sl_uint64 const, other._value);
				case VariantType::Float:
					return REF_VAR(float const, _value) / REF_VAR(float const, other._value);
				case VariantType::Double:
					return REF_VAR(double const, _value) / REF_VAR(double const, other._value);
				case VariantType::BigInt:
					return REF_VAR(BigInt const, _value) / REF_VAR(BigInt const, other._value);
				default:
					break;
			}
		} else {
			if (other._type == VariantType::Null) {
				return Variant();
			}
			switch (_type) {
				case VariantType::Null:
					if (other.isNumberType()) {
						return sl_null;
					}
					break;
				case VariantType::Int32:
				case VariantType::Uint32:
				case VariantType::Int64:
				case VariantType::Uint64:
					if (other.isIntegerType()) {
						return getInt64() / other.getInt64();
					}
					if (other._type == VariantType::Float || other._type == VariantType::Double) {
						return getDouble() / other.getDouble();
					}
					break;
				case VariantType::Float:
				case VariantType::Double:
					if (other.isNumberType()) {
						return getDouble() / other.getDouble();
					}
					break;
				default:
					break;
			}
		}
		VARIANT_OPERATOR_CALL_REF(Divide)
		return Variant();
	}

	Variant Variant::operator%(const Variant& other) const noexcept
	{
		if (_type == other._type) {
			switch (_type) {
				case VariantType::Null:
					return 0;
				case VariantType::Int32:
					return REF_VAR(sl_int32 const, _value) % REF_VAR(sl_int32 const, other._value);
				case VariantType::Uint32:
					return REF_VAR(sl_uint32 const, _value) % REF_VAR(sl_uint32 const, other._value);
				case VariantType::Int64:
					return REF_VAR(sl_int64 const, _value) % REF_VAR(sl_int64 const, other._value);
				case VariantType::Uint64:
					return REF_VAR(sl_uint64 const, _value) % REF_VAR(sl_uint64 const, other._value);
				case VariantType::BigInt:
					return REF_VAR(BigInt const, _value) % REF_VAR(BigInt const, other._value);
				default:
					break;
			}
		} else {
			if (other._type == VariantType::Null) {
				return Variant();
			}
			switch (_type) {
				case VariantType::Null:
					if (other.isIntegerType()) {
						return 0;
					}
					break;
				case VariantType::Int32:
				case VariantType::Uint32:
				case VariantType::Int64:
				case VariantType::Uint64:
					if (other.isIntegerType()) {
						return getInt64() % other.getInt64();
					}
					break;
				default:
					break;
			}
		}
		VARIANT_OPERATOR_CALL_REF(Remainder)
		return Variant();
	}

	Variant Variant::operator-() const noexcept
	{
		switch (_type) {
			case VariantType::Null:
				return sl_null;
			case VariantType::Int32:
				return - REF_VAR(sl_int32 const, _value);
			case VariantType::Uint32:
				{
					sl_uint32 n = REF_VAR(sl_uint32 const, _value);
					if (n & 0x80000000) {
						return -((sl_int64)n);
					} else {
						return -((sl_int32)n);
					}
				}
			case VariantType::Int64:
			case VariantType::Uint64:
				return - REF_VAR(sl_int64 const, _value);
			case VariantType::Float:
				return - REF_VAR(float const, _value);
			case VariantType::Double:
				return - REF_VAR(double const, _value);
			case VariantType::BigInt:
				return - REF_VAR(BigInt const, _value);
			default:
				break;
		}
		if (IsRef(_type)) {
			Ref<CRef> ref = getRef();
			if (ref.isNotNull()) {
				Variant result;
				if (ref->runOperator(ObjectOperator::UnaryMinus, result, Variant(), sl_false)) {
					return result;
				}
			}
		}
		return Variant();
	}

	Variant::operator sl_bool() const noexcept
	{
		switch (_type) {
			case VariantType::Null:
				return sl_false;
			case VariantType::Int32:
			case VariantType::Uint32:
				return REF_VAR(sl_uint32 const, _value) != 0;
			case VariantType::Int64:
			case VariantType::Uint64:
			case VariantType::Time:
				return REF_VAR(sl_uint64 const, _value) != 0;
			case VariantType::Boolean:
				return REF_VAR(sl_bool const, _value);
			case VariantType::Float:
				return REF_VAR(float const, _value) != 0;
			case VariantType::Double:
				return REF_VAR(double const, _value) != 0;
			case VariantType::ObjectId:
				return REF_VAR(ObjectId const, _value).isNotZero();
			case VariantType::BigInt:
				return REF_VAR(BigInt const, _value).isNotZero();
			default:
				break;
		}
		if (IsRef(_type)) {
			Ref<CRef> ref = getRef();
			if (ref.isNotNull()) {
				Variant result;
				if (ref->runOperator(ObjectOperator::LogicalNot, result, Variant(), sl_false)) {
					return !(result.getBoolean());
				}
				return sl_true;
			} else {
				return sl_false;
			}
		}
		return sl_true;
	}

	sl_bool Variant::operator!() const noexcept
	{
		return !((sl_bool)*this);
	}

	Variant Variant::operator~() const noexcept
	{
		switch (_type) {
			case VariantType::Null:
				return sl_null;
			case VariantType::Int32:
				return ~ REF_VAR(sl_int32 const, _value);
			case VariantType::Uint32:
				return ~ REF_VAR(sl_uint32 const, _value);
			case VariantType::Int64:
				return ~ REF_VAR(sl_int64 const, _value);
			case VariantType::Uint64:
				return ~ REF_VAR(sl_uint64 const, _value);
			case VariantType::BigInt:
				return ~ REF_VAR(BigInt const, _value);
			default:
				break;
		}
		if (IsRef(_type)) {
			Ref<CRef> ref = getRef();
			if (ref.isNotNull()) {
				Variant result;
				if (ref->runOperator(ObjectOperator::BitwiseNot, result, Variant(), sl_false)) {
					return result;
				}
			}
		}
		return Variant();
	}

	Variant Variant::operator||(const Variant& other) const noexcept
	{
		if (*this) {
			return *this;
		}
		return other;
	}

	Variant Variant::operator&&(const Variant& other) const noexcept
	{
		if (*this) {
			return other;
		}
		return *this;
	}

	Variant Variant::operator|(const Variant& other) const noexcept
	{
		if (_type == other._type) {
			switch (_type) {
				case VariantType::Null:
					return sl_null;
				case VariantType::Int32:
					return REF_VAR(sl_int32 const, _value) | REF_VAR(sl_int32 const, other._value);
				case VariantType::Uint32:
					return REF_VAR(sl_uint32 const, _value) | REF_VAR(sl_uint32 const, other._value);
				case VariantType::Int64:
					return REF_VAR(sl_int64 const, _value) | REF_VAR(sl_int64 const, other._value);
				case VariantType::Uint64:
					return REF_VAR(sl_uint64 const, _value) | REF_VAR(sl_uint64 const, other._value);
				case VariantType::BigInt:
					return REF_VAR(BigInt const, _value) | REF_VAR(BigInt const, other._value);
				default:
					break;
			}
		} else {
			if (other._type == VariantType::Null) {
				return *this;
			}
			switch (_type) {
				case VariantType::Null:
					return other;
				case VariantType::Int32:
				case VariantType::Uint32:
					if (other._type == VariantType::Int32 || other._type == VariantType::Uint32) {
						return REF_VAR(sl_int32 const, _value) | REF_VAR(sl_int32 const, other._value);
					} else if (other._type == VariantType::Int64 || other._type == VariantType::Uint64) {
						return getInt64() | other.getInt64();
					}
					break;
				case VariantType::Int64:
				case VariantType::Uint64:
					if (other.isIntegerType()) {
						return getInt64() | other.getInt64();
					}
					break;
				default:
					break;
			}
		}
		VARIANT_OPERATOR_CALL_REF(BitwiseOr)
		return Variant();
	}

	Variant Variant::operator&(const Variant& other) const noexcept
	{
		if (_type == other._type) {
			switch (_type) {
				case VariantType::Null:
					return sl_null;
				case VariantType::Int32:
					return REF_VAR(sl_int32 const, _value) & REF_VAR(sl_int32 const, other._value);
				case VariantType::Uint32:
					return REF_VAR(sl_uint32 const, _value) & REF_VAR(sl_uint32 const, other._value);
				case VariantType::Int64:
					return REF_VAR(sl_int64 const, _value) & REF_VAR(sl_int64 const, other._value);
				case VariantType::Uint64:
					return REF_VAR(sl_uint64 const, _value) & REF_VAR(sl_uint64 const, other._value);
				case VariantType::BigInt:
					return REF_VAR(BigInt const, _value) & REF_VAR(BigInt const, other._value);
				default:
					break;
			}
		} else {
			if (other._type == VariantType::Null) {
				return *this;
			}
			switch (_type) {
				case VariantType::Null:
					return other;
				case VariantType::Int32:
				case VariantType::Uint32:
					if (other._type == VariantType::Int32 || other._type == VariantType::Uint32) {
						return REF_VAR(sl_int32 const, _value) & REF_VAR(sl_int32 const, other._value);
					} else if (other._type == VariantType::Int64 || other._type == VariantType::Uint64) {
						return getInt64() & other.getInt64();
					}
					break;
				case VariantType::Int64:
				case VariantType::Uint64:
					if (other.isIntegerType()) {
						return getInt64() & other.getInt64();
					}
					break;
				default:
					break;
			}
		}
		VARIANT_OPERATOR_CALL_REF(BitwiseAnd)
		return Variant();
	}

	Variant Variant::operator^(const Variant& other) const noexcept
	{
		if (_type == other._type) {
			switch (_type) {
				case VariantType::Null:
					return sl_null;
				case VariantType::Int32:
					return REF_VAR(sl_int32 const, _value) ^ REF_VAR(sl_int32 const, other._value);
				case VariantType::Uint32:
					return REF_VAR(sl_uint32 const, _value) ^ REF_VAR(sl_uint32 const, other._value);
				case VariantType::Int64:
					return REF_VAR(sl_int64 const, _value) ^ REF_VAR(sl_int64 const, other._value);
				case VariantType::Uint64:
					return REF_VAR(sl_uint64 const, _value) ^ REF_VAR(sl_uint64 const, other._value);
				case VariantType::BigInt:
					return REF_VAR(BigInt const, _value) ^ REF_VAR(BigInt const, other._value);
				default:
					break;
			}
		} else {
			if (other._type == VariantType::Null) {
				return *this;
			}
			switch (_type) {
				case VariantType::Null:
					return other;
				case VariantType::Int32:
				case VariantType::Uint32:
					if (other._type == VariantType::Int32 || other._type == VariantType::Uint32) {
						return REF_VAR(sl_int32 const, _value) ^ REF_VAR(sl_int32 const, other._value);
					} else if (other._type == VariantType::Int64 || other._type == VariantType::Uint64) {
						return getInt64() ^ other.getInt64();
					}
					break;
				case VariantType::Int64:
				case VariantType::Uint64:
					if (other.isIntegerType()) {
						return getInt64() ^ other.getInt64();
					}
					break;
				default:
					break;
			}
		}
		VARIANT_OPERATOR_CALL_REF(BitwiseXor)
		return Variant();
	}

	Variant Variant::operator>>(const Variant& other) const noexcept
	{
		switch (_type) {
			case VariantType::Null:
				return sl_null;
			case VariantType::Int32:
				return REF_VAR(sl_int32 const, _value) >> other.getUint32();
			case VariantType::Uint32:
				return REF_VAR(sl_uint32 const, _value) >> other.getUint32();
			case VariantType::Int64:
				return REF_VAR(sl_int64 const, _value) >> other.getUint32();
			case VariantType::Uint64:
				return REF_VAR(sl_uint64 const, _value) >> other.getUint32();
			case VariantType::BigInt:
				return REF_VAR(BigInt const, _value) >> other.getUint32();
			default:
				break;
		}
		if (IsRef(_type)) {
			Ref<CRef> ref = getRef();
			if (ref.isNotNull()) {
				Variant result;
				if (ref->runOperator(ObjectOperator::ShiftRight, result, other, sl_false)) {
					return result;
				}
			}
		}
		return Variant();
	}

	Variant Variant::operator<<(const Variant& other) const noexcept
	{
		switch (_type) {
			case VariantType::Null:
				return sl_null;
			case VariantType::Int32:
				return REF_VAR(sl_int32 const, _value) << other.getUint32();
			case VariantType::Uint32:
				return REF_VAR(sl_uint32 const, _value) << other.getUint32();
			case VariantType::Int64:
				return REF_VAR(sl_int64 const, _value) << other.getUint32();
			case VariantType::Uint64:
				return REF_VAR(sl_uint64 const, _value) << other.getUint32();
			case VariantType::BigInt:
				return REF_VAR(BigInt const, _value) << other.getUint32();
			default:
				break;
		}
		if (IsRef(_type)) {
			Ref<CRef> ref = getRef();
			if (ref.isNotNull()) {
				Variant result;
				if (ref->runOperator(ObjectOperator::ShiftLeft, result, other, sl_false)) {
					return result;
				}
			}
		}
		return Variant();
	}

	Variant Variant::operator[](sl_uint64 index) const noexcept
	{
		return getElement(index);
	}

	Variant Variant::operator[](const String& key) const noexcept
	{
		return getItem(key);
	}

	void Variant::setUndefined() noexcept
	{
		if (_type != VariantType::Null) {
			Free(_type, _value);
			Init(*this, VariantType::Null);
		} else {
			_tag = 0;
		}
		_value = 0;
	}

	void Variant::setNull() noexcept
	{
		if (_type != VariantType::Null) {
			Free(_type, _value);
			Init(*this, VariantType::Null);
		} else {
			_tag = 0;
		}
		_value = 1;
	}

	namespace {

		template <class STRING>
		SLIB_INLINE static sl_bool ParseNumber(const STRING& str, sl_int32* _out) noexcept
		{
			return str.parseInt32(_out);
		}

		template <class STRING>
		SLIB_INLINE static sl_bool ParseNumber(const STRING& str, sl_uint32* _out) noexcept
		{
			return str.parseUint32(_out);
		}

		template <class STRING>
		SLIB_INLINE static sl_bool ParseNumber(const STRING& str, sl_int64* _out) noexcept
		{
			return str.parseInt64(_out);
		}

		template <class STRING>
		SLIB_INLINE static sl_bool ParseNumber(const STRING& str, sl_uint64* _out) noexcept
		{
			return str.parseUint64(_out);
		}

		template <class STRING>
		SLIB_INLINE static sl_bool ParseNumber(const STRING& str, float* _out) noexcept
		{
			return str.parseFloat(_out);
		}

		template <class STRING>
		SLIB_INLINE static sl_bool ParseNumber(const STRING& str, double* _out) noexcept
		{
			return str.parseDouble(_out);
		}

		template <class NUMBER>
		SLIB_INLINE static void GetNumberFromTime(const Time& t, NUMBER& _out) noexcept
		{
			_out = (NUMBER)(t.toUnixTime());
		}

		template <>
		SLIB_INLINE void GetNumberFromTime<float>(const Time& t, float& _out) noexcept
		{
			_out = (float)(t.toUnixTimeF());
		}

		template <>
		SLIB_INLINE void GetNumberFromTime<double>(const Time& t, double& _out) noexcept
		{
			_out = t.toUnixTimeF();
		}

		SLIB_INLINE void GetNumberFromBigInt(const BigInt& n, sl_int32& _out) noexcept
		{
			_out = n.getInt32();
		}

		SLIB_INLINE void GetNumberFromBigInt(const BigInt& n, sl_uint32& _out) noexcept
		{
			_out = n.getUint32();
		}

		SLIB_INLINE void GetNumberFromBigInt(const BigInt& n, sl_int64& _out) noexcept
		{
			_out = n.getInt64();
		}

		SLIB_INLINE void GetNumberFromBigInt(const BigInt& n, sl_uint64& _out) noexcept
		{
			_out = n.getUint64();
		}

		SLIB_INLINE void GetNumberFromBigInt(const BigInt& n, float& _out) noexcept
		{
			_out = n.getFloat();
		}

		SLIB_INLINE void GetNumberFromBigInt(const BigInt& n, double& _out) noexcept
		{
			_out = n.getDouble();
		}

		template <class NUMBER>
		static sl_bool GetNumber(const Variant& var, NUMBER* _out) noexcept
		{
			switch (var._type) {
				case VariantType::Int32:
					if (_out) {
						*_out = (NUMBER)(REF_VAR(sl_int32 const, var._value));
					}
					return sl_true;
				case VariantType::Uint32:
					if (_out) {
						*_out = (NUMBER)(REF_VAR(sl_uint32 const, var._value));
					}
					return sl_true;
				case VariantType::Int64:
					if (_out) {
						*_out = (NUMBER)(REF_VAR(sl_int64 const, var._value));
					}
					return sl_true;
				case VariantType::Uint64:
					if (_out) {
						*_out = (NUMBER)(REF_VAR(sl_uint64 const, var._value));
					}
					return sl_true;
				case VariantType::Float:
					if (_out) {
						*_out = (NUMBER)(REF_VAR(float const, var._value));
					}
					return sl_true;
				case VariantType::Double:
					if (_out) {
						*_out = (NUMBER)(REF_VAR(double const, var._value));
					}
					return sl_true;
				case VariantType::Boolean:
					if (_out) {
						*_out = (REF_VAR(sl_bool const, var._value)) ? (NUMBER)1 : (NUMBER)0;
					}
					return sl_true;
				case VariantType::String8:
					return ParseNumber(REF_VAR(String const, var._value), _out);
				case VariantType::String16:
					return ParseNumber(REF_VAR(String16 const, var._value), _out);
				case VariantType::String32:
					return ParseNumber(REF_VAR(String32 const, var._value), _out);
				case VariantType::Sz8:
					return ParseNumber(StringView(REF_VAR(sl_char8 const* const, var._value)), _out);
				case VariantType::Sz16:
					return ParseNumber(StringView16(REF_VAR(sl_char16 const* const, var._value)), _out);
				case VariantType::Sz32:
					return ParseNumber(StringView32(REF_VAR(sl_char32 const* const, var._value)), _out);
				case VariantType::StringData8:
					return ParseNumber(StringView(REF_VAR(sl_char8 const* const, var._value), var._value2), _out);
				case VariantType::StringData16:
					return ParseNumber(StringView16(REF_VAR(sl_char16 const* const, var._value), var._value2), _out);
				case VariantType::StringData32:
					return ParseNumber(StringView32(REF_VAR(sl_char32 const* const, var._value), var._value2), _out);
				case VariantType::Pointer:
					if (_out) {
						*_out = (NUMBER)(REF_VAR(const sl_size, var._value));
					}
					return sl_true;
				case VariantType::Time:
					if (_out) {
						GetNumberFromTime(REF_VAR(Time const, var._value), *_out);
					}
					return sl_true;
				case VariantType::BigInt:
					if (_out) {
						GetNumberFromBigInt(REF_VAR(BigInt const, var._value), *_out);
					}
					return sl_true;
				default:
					break;
			}
			return sl_false;
		}

	}

	sl_bool Variant::isInt32() const noexcept
	{
		return _type == VariantType::Int32;
	}

	sl_bool Variant::getInt32(sl_int32* _out) const noexcept
	{
		return GetNumber(*this, _out);
	}

	sl_int32 Variant::getInt32(sl_int32 def) const noexcept
	{
		sl_int32 ret;
		if (getInt32(&ret)) {
			return ret;
		}
		return def;
	}

	void Variant::setInt32(sl_int32 value) noexcept
	{
		Free(_type, _value);
		Init(*this, VariantType::Int32);
		REF_VAR(sl_int32, _value) = value;
	}

	sl_bool Variant::isUint32() const noexcept
	{
		return _type == VariantType::Uint32;
	}

	sl_bool Variant::getUint32(sl_uint32* _out) const noexcept
	{
		return GetNumber(*this, _out);
	}

	sl_uint32 Variant::getUint32(sl_uint32 def) const noexcept
	{
		sl_uint32 ret;
		if (getUint32(&ret)) {
			return ret;
		}
		return def;
	}

	void Variant::setUint32(sl_uint32 value) noexcept
	{
		Free(_type, _value);
		Init(*this, VariantType::Uint32);
		REF_VAR(sl_uint32, _value) = value;
	}

	sl_bool Variant::isInt64() const noexcept
	{
		return _type == VariantType::Int64;
	}

	sl_bool Variant::getInt64(sl_int64* _out) const noexcept
	{
		return GetNumber(*this, _out);
	}

	sl_int64 Variant::getInt64(sl_int64 def) const noexcept
	{
		sl_int64 ret;
		if (getInt64(&ret)) {
			return ret;
		}
		return def;
	}

	void Variant::setInt64(sl_int64 value) noexcept
	{
		Free(_type, _value);
		Init(*this, VariantType::Int64);
		REF_VAR(sl_int64, _value) = value;
	}

	sl_bool Variant::isUint64() const noexcept
	{
		return _type == VariantType::Uint64;
	}

	sl_bool Variant::getUint64(sl_uint64* _out) const noexcept
	{
		return GetNumber(*this, _out);
	}

	sl_uint64 Variant::getUint64(sl_uint64 def) const noexcept
	{
		sl_uint64 ret;
		if (getUint64(&ret)) {
			return ret;
		}
		return def;
	}

	void Variant::setUint64(sl_uint64 value) noexcept
	{
		Free(_type, _value);
		Init(*this, VariantType::Uint64);
		REF_VAR(sl_uint64, _value) = value;
	}

	sl_bool Variant::isIntegerType() const noexcept
	{
		return _type == VariantType::Int32 || _type == VariantType::Uint32 || _type == VariantType::Int64 || _type == VariantType::Uint64;
	}

	sl_bool Variant::isSignedIntegerType() const noexcept
	{
		return _type == VariantType::Int32 || _type == VariantType::Int64;
	}

	sl_bool Variant::isUnsignedIntegerType() const noexcept
	{
		return _type == VariantType::Uint32 || _type == VariantType::Uint64;
	}

	sl_bool Variant::isFloat() const noexcept
	{
		return _type == VariantType::Float;
	}

	sl_bool Variant::getFloat(float* _out) const noexcept
	{
		return GetNumber(*this, _out);
	}

	float Variant::getFloat(float def) const noexcept
	{
		float ret;
		if (getFloat(&ret)) {
			return ret;
		}
		return def;
	}

	void Variant::setFloat(float value) noexcept
	{
		Free(_type, _value);
		Init(*this, VariantType::Float);
		REF_VAR(float, _value) = value;
	}

	sl_bool Variant::isDouble() const noexcept
	{
		return _type == VariantType::Double;
	}

	sl_bool Variant::getDouble(double* _out) const noexcept
	{
		return GetNumber(*this, _out);
	}

	double Variant::getDouble(double def) const noexcept
	{
		double ret;
		if (getDouble(&ret)) {
			return ret;
		}
		return def;
	}

	void Variant::setDouble(double value) noexcept
	{
		Free(_type, _value);
		Init(*this, VariantType::Double);
		REF_VAR(double, _value) = value;
	}

	sl_bool Variant::isNaN() const noexcept
	{
		if (_type == VariantType::Float) {
			return Math::isNaN(REF_VAR(float, _value));
		} else if (_type == VariantType::Double) {
			return Math::isNaN(REF_VAR(double, _value));
		}
		return sl_false;
	}

	sl_bool Variant::isInfinite() const noexcept
	{
		if (_type == VariantType::Float) {
			return Math::isInfinite(REF_VAR(float, _value));
		} else if (_type == VariantType::Double) {
			return Math::isInfinite(REF_VAR(double, _value));
		}
		return sl_false;
	}

	sl_bool Variant::isPositiveInfinite() const noexcept
	{
		if (_type == VariantType::Float) {
			return Math::isPositiveInfinite(REF_VAR(float, _value));
		} else if (_type == VariantType::Double) {
			return Math::isPositiveInfinite(REF_VAR(double, _value));
		}
		return sl_false;
	}

	sl_bool Variant::isNegativeInfinite() const noexcept
	{
		if (_type == VariantType::Float) {
			return Math::isNegativeInfinite(REF_VAR(float, _value));
		} else if (_type == VariantType::Double) {
			return Math::isNegativeInfinite(REF_VAR(double, _value));
		}
		return sl_false;
	}

	sl_bool Variant::isNumberType() const noexcept
	{
		return isIntegerType() || _type == VariantType::Float || _type == VariantType::Double;
	}

	sl_bool Variant::isBoolean() const noexcept
	{
		return _type == VariantType::Boolean;
	}

	sl_bool Variant::isTrue() const noexcept
	{
		return _type == VariantType::Boolean && REF_VAR(sl_bool const, _value) != sl_false;
	}

	sl_bool Variant::isFalse() const noexcept
	{
		return _type == VariantType::Boolean && REF_VAR(sl_bool const, _value) == sl_false;
	}

	sl_bool Variant::getBoolean(sl_bool def) const noexcept
	{
		switch (_type) {
			case VariantType::Int32:
			case VariantType::Uint32:
				return REF_VAR(sl_uint32 const, _value) != 0;
			case VariantType::Int64:
			case VariantType::Uint64:
				return REF_VAR(sl_uint64 const, _value) != 0;
			case VariantType::Boolean:
				return REF_VAR(sl_bool const, _value);
			case VariantType::String8:
				return (REF_VAR(String const, _value)).parseBoolean(def);
			case VariantType::String16:
				return (REF_VAR(String16 const, _value)).parseBoolean(def);
			case VariantType::String32:
				return (REF_VAR(String32 const, _value)).parseBoolean(def);
			case VariantType::Sz8:
				return StringView(REF_VAR(sl_char8 const* const, _value)).parseBoolean(def);
			case VariantType::Sz16:
				return StringView16(REF_VAR(sl_char16 const* const, _value)).parseBoolean(def);
			case VariantType::Sz32:
				return StringView32(REF_VAR(sl_char32 const* const, _value)).parseBoolean(def);
			case VariantType::StringData8:
				return StringView(REF_VAR(sl_char8 const* const, _value), _value2).parseBoolean(def);
			case VariantType::StringData16:
				return StringView16(REF_VAR(sl_char16 const* const, _value), _value2).parseBoolean(def);
			case VariantType::StringData32:
				return StringView32(REF_VAR(sl_char32 const* const, _value), _value2).parseBoolean(def);
			default:
				break;
		}
		return def;
	}

	void Variant::setBoolean(sl_bool value) noexcept
	{
		Free(_type, _value);
		Init(*this, VariantType::Boolean);
		REF_VAR(sl_bool, _value) = value;
	}

	sl_bool Variant::isStringType() const noexcept
	{
		return IsStringType(_type);
	}

	sl_bool Variant::is8BitsStringType() const noexcept
	{
		return _type == VariantType::String8 || _type == VariantType::Sz8 || _type == VariantType::StringData8;
	}

	sl_bool Variant::is16BitsStringType() const noexcept
	{
		return _type == VariantType::String16 || _type == VariantType::Sz16 || _type == VariantType::StringData16;
	}

	sl_bool Variant::is32BitsStringType() const noexcept
	{
		return _type == VariantType::String32 || _type == VariantType::Sz32 || _type == VariantType::StringData32;
	}

	sl_bool Variant::isStringObject8() const noexcept
	{
		return _type == VariantType::String8;
	}

	sl_bool Variant::isStringObject16() const noexcept
	{
		return _type == VariantType::String16;
	}

	sl_bool Variant::isStringObject32() const noexcept
	{
		return _type == VariantType::String32;
	}

	sl_bool Variant::isStringView8() const noexcept
	{
		return _type == VariantType::Sz8 || _type == VariantType::StringData8;
	}

	sl_bool Variant::isStringView16() const noexcept
	{
		return _type == VariantType::Sz16 || _type == VariantType::StringData16;
	}

	sl_bool Variant::isStringView32() const noexcept
	{
		return _type == VariantType::Sz32 || _type == VariantType::StringData32;
	}

	sl_bool Variant::isSz8() const noexcept
	{
		return _type == VariantType::Sz8;
	}

	sl_bool Variant::isSz16() const noexcept
	{
		return _type == VariantType::Sz16;
	}

	sl_bool Variant::isSz32() const noexcept
	{
		return _type == VariantType::Sz32;
	}

	namespace {

		template <class STRING>
		static STRING GetStringFromBoolean(sl_bool flag) noexcept;

		template <>
		String GetStringFromBoolean<String>(sl_bool flag) noexcept
		{
			if (flag) {
				SLIB_RETURN_STRING("true")
			} else {
				SLIB_RETURN_STRING("false")
			}
		}

		template <>
		String16 GetStringFromBoolean<String16>(sl_bool flag) noexcept
		{
			if (flag) {
				SLIB_RETURN_STRING16("true")
			} else {
				SLIB_RETURN_STRING16("false")
			}
		}

		template <>
		String32 GetStringFromBoolean<String32>(sl_bool flag) noexcept
		{
			if (flag) {
				SLIB_RETURN_STRING32("true")
			} else {
				SLIB_RETURN_STRING32("false")
			}
		}

		template <class STRING>
		static STRING GetString(const Variant& var, const STRING& def) noexcept
		{
			switch (var._type) {
				case VariantType::Int32:
					return STRING::fromInt32(REF_VAR(sl_int32 const, var._value));
				case VariantType::Uint32:
					return STRING::fromUint32(REF_VAR(sl_uint32 const, var._value));
				case VariantType::Int64:
					return STRING::fromInt64(REF_VAR(sl_int64 const, var._value));
				case VariantType::Uint64:
					return STRING::fromUint64(REF_VAR(sl_uint64 const, var._value));
				case VariantType::Float:
					return STRING::fromFloat(REF_VAR(float const, var._value));
				case VariantType::Double:
					return STRING::fromDouble(REF_VAR(double const, var._value));
				case VariantType::Boolean:
					return GetStringFromBoolean<STRING>(REF_VAR(sl_bool const, var._value));
				case VariantType::Time:
					return STRING::from(REF_VAR(Time const, var._value).toString());
				case VariantType::String8:
					return STRING::from(REF_VAR(String const, var._value));
				case VariantType::String16:
					return STRING::from(REF_VAR(String16 const, var._value));
				case VariantType::String32:
					return STRING::from(REF_VAR(String32 const, var._value));
				case VariantType::Sz8:
					return STRING::create(REF_VAR(sl_char8 const* const, var._value));
				case VariantType::Sz16:
					return STRING::create(REF_VAR(sl_char16 const* const, var._value));
				case VariantType::Sz32:
					return STRING::create(REF_VAR(sl_char32 const* const, var._value));
				case VariantType::StringData8:
					return STRING::create(REF_VAR(sl_char8 const* const, var._value), var._value2);
				case VariantType::StringData16:
					return STRING::create(REF_VAR(sl_char16 const* const, var._value), var._value2);
				case VariantType::StringData32:
					return STRING::create(REF_VAR(sl_char32 const* const, var._value), var._value2);
				case VariantType::Pointer:
					{
						typename STRING::Char ch = '#';
						return typename STRING::StringViewType(&ch, 1) + STRING::fromPointerValue(REF_VAR(void const* const, var._value));
					}
				case VariantType::ObjectId:
					{
						ObjectId& _id = REF_VAR(ObjectId, var._value);
						return STRING::makeHexString(_id.data, sizeof(ObjectId));
					}
				case VariantType::Null:
					if (var._value) {
						return sl_null;
					}
					break;
				default:
					if (var.isMemory()) {
						return STRING::fromMemory(REF_VAR(Memory, var._value));
					} else if (var.isBigInt()) {
						return STRING::from(REF_VAR(BigInt, var._value).toString());
					}
					break;
			}
			return def;
		}

	}

	String Variant::getString(const String& def) const noexcept
	{
		return GetString(*this, def);
	}

	String Variant::getString() const noexcept
	{
		return GetString<String>(*this, sl_null);
	}

	String16 Variant::getString16(const String16& def) const noexcept
	{
		return GetString(*this, def);
	}

	String16 Variant::getString16() const noexcept
	{
		return GetString<String16>(*this, sl_null);
	}

	String32 Variant::getString32(const String32& def) const noexcept
	{
		return GetString(*this, def);
	}

	String32 Variant::getString32() const noexcept
	{
		return GetString<String32>(*this, sl_null);
	}

#define PRIV_VARIANT_GET_STRING_VIEW_IMPL(BITS, DEF) \
	switch (_type) { \
		case VariantType::Boolean: \
			if (REF_VAR(sl_bool const, _value)) { \
				static const sl_char##BITS _s[] = {'t', 'r', 'u', 'e', 0}; \
				return _s; \
			} else { \
				static const sl_char##BITS _s[] = {'f', 'a', 'l', 's', 'e', 0}; \
				return _s; \
			} \
		case VariantType::Sz##BITS: \
			return REF_VAR(sl_char##BITS const* const, _value); \
		case VariantType::String##BITS: \
			return REF_VAR(typename StringTypeFromCharType<sl_char##BITS>::Type, _value); \
		case VariantType::StringData##BITS: \
			return typename StringViewTypeFromCharType<sl_char##BITS>::Type(REF_VAR(sl_char##BITS const* const, _value), _value2); \
		default: \
			break; \
	} \
	return DEF;

	StringView Variant::getStringView(const StringView& def) const noexcept
	{
		PRIV_VARIANT_GET_STRING_VIEW_IMPL(8, def)
	}

	StringView Variant::getStringView() const noexcept
	{
		PRIV_VARIANT_GET_STRING_VIEW_IMPL(8, sl_null)
	}

	StringView16 Variant::getStringView16(const StringView16& def) const noexcept
	{
		PRIV_VARIANT_GET_STRING_VIEW_IMPL(16, def)
	}

	StringView16 Variant::getStringView16() const noexcept
	{
		PRIV_VARIANT_GET_STRING_VIEW_IMPL(16, sl_null)
	}

	StringView32 Variant::getStringView32(const StringView32& def) const noexcept
	{
		PRIV_VARIANT_GET_STRING_VIEW_IMPL(32, def)
	}

	StringView32 Variant::getStringView32() const noexcept
	{
		PRIV_VARIANT_GET_STRING_VIEW_IMPL(32, sl_null)
	}

	sl_char8* Variant::getSz8(const sl_char8* def) const noexcept
	{
		if (_type == VariantType::Sz8) {
			return REF_VAR(sl_char8*, _value);
		}
		return (sl_char8*)def;
	}

	sl_char16* Variant::getSz16(const sl_char16* def) const noexcept
	{
		if (_type == VariantType::Sz16) {
			return REF_VAR(sl_char16*, _value);
		}
		return (sl_char16*)def;
	}

	sl_char32* Variant::getSz32(const sl_char32* def) const noexcept
	{
		if (_type == VariantType::Sz32) {
			return REF_VAR(sl_char32*, _value);
		}
		return (sl_char32*)def;
	}

	StringParam Variant::getStringParam(const StringParam& def) const noexcept
	{
		switch (_type) {
			case VariantType::String8:
				{
					String s = REF_VAR(String const, _value);
					return Move(s);
				}
			case VariantType::String16:
				{
					String16 s = REF_VAR(String16 const, _value);
					return Move(s);
			}
			case VariantType::String32:
				{
					String32 s = REF_VAR(String32 const, _value);
					return Move(s);
			}
			case VariantType::Sz8:
				return REF_VAR(sl_char8 const* const, _value);
			case VariantType::Sz16:
				return REF_VAR(sl_char16 const* const, _value);
			case VariantType::Sz32:
				return REF_VAR(sl_char32 const* const, _value);
			case VariantType::StringData8:
				return StringParam(REF_VAR(sl_char8 const* const, _value), _value2);
			case VariantType::StringData16:
				return StringParam(REF_VAR(sl_char16 const* const, _value), _value2);
			case VariantType::StringData32:
				return StringParam(REF_VAR(sl_char32 const* const, _value), _value2);
			case VariantType::Null:
				break;
			default:
				{
					String s = getString();
					if (s.isNotNull()) {
						return Move(s);
					}
				}
				break;
		}
		return def;
	}

	StringParam Variant::getStringParam() const noexcept
	{
		return getStringParam(StringParam::null());
	}

	sl_bool Variant::getStringData(StringRawData& data) const noexcept
	{
		sl_size len;
		switch (_type) {
			case VariantType::String8:
				data.charSize = 1;
				data.data8 = REF_VAR(String const, _value).getData(len);
				data.length = len;
				return sl_true;
			case VariantType::String16:
				data.charSize = 2;
				data.data16 = REF_VAR(String16 const, _value).getData(len);
				data.length = len;
				return sl_true;
			case VariantType::String32:
				data.charSize = 4;
				data.data32 = REF_VAR(String32 const, _value).getData(len);
				data.length = len;
				return sl_true;
			case VariantType::Sz8:
				data.charSize = 1;
				data.data8 = REF_VAR(sl_char8*, _value);
				data.length = -1;
				return sl_true;
			case VariantType::Sz16:
				data.charSize = 2;
				data.data16 = REF_VAR(sl_char16*, _value);
				data.length = -1;
				return sl_true;
			case VariantType::Sz32:
				data.charSize = 4;
				data.data32 = REF_VAR(sl_char32*, _value);
				data.length = -1;
				return sl_true;
			case VariantType::StringData8:
				data.charSize = 1;
				data.data8 = REF_VAR(sl_char8*, _value);
				data.length = _value2;
				return sl_true;
			case VariantType::StringData16:
				data.charSize = 2;
				data.data16 = REF_VAR(sl_char16*, _value);
				data.length = _value2;
				return sl_true;
			case VariantType::StringData32:
				data.charSize = 4;
				data.data32 = REF_VAR(sl_char32*, _value);
				data.length = _value2;
				return sl_true;
			default:
				break;
		}
		return sl_false;
	}

	void Variant::setString(const String& value) noexcept
	{
		if (value.isNotNull()) {
			Free(_type, _value);
			Init(*this, VariantType::String8);
			new PTR_VAR(String, _value) String(value);
		} else {
			setNull();
		}
	}

	void Variant::setString(String&& value) noexcept
	{
		if (value.isNotNull()) {
			Free(_type, _value);
			Init(*this, VariantType::String8);
			new PTR_VAR(String, _value) String(Move(value));
		} else {
			setNull();
		}
	}

	void Variant::setString(const String16& value) noexcept
	{
		if (value.isNotNull()) {
			Free(_type, _value);
			Init(*this, VariantType::String16);
			new PTR_VAR(String16, _value) String16(value);
		} else {
			setNull();
		}
	}

	void Variant::setString(String16&& value) noexcept
	{
		if (value.isNotNull()) {
			Free(_type, _value);
			Init(*this, VariantType::String16);
			new PTR_VAR(String16, _value) String16(Move(value));
		} else {
			setNull();
		}
	}

	void Variant::setString(const String32& value) noexcept
	{
		if (value.isNotNull()) {
			Free(_type, _value);
			Init(*this, VariantType::String32);
			new PTR_VAR(String32, _value) String32(value);
		} else {
			setNull();
		}
	}

	void Variant::setString(String32&& value) noexcept
	{
		if (value.isNotNull()) {
			Free(_type, _value);
			Init(*this, VariantType::String32);
			new PTR_VAR(String32, _value) String32(Move(value));
		} else {
			setNull();
		}
	}

	void Variant::setString(const AtomicString& s) noexcept
	{
		String value(s);
		if (value.isNotNull()) {
			Free(_type, _value);
			Init(*this, VariantType::String8);
			new PTR_VAR(String, _value) String(Move(value));
		} else {
			setNull();
		}
	}

	void Variant::setString(const AtomicString16& s) noexcept
	{
		String16 value(s);
		if (value.isNotNull()) {
			Free(_type, _value);
			Init(*this, VariantType::String16);
			new PTR_VAR(String16, _value) String16(Move(value));
		} else {
			setNull();
		}
	}

	void Variant::setString(const AtomicString32& s) noexcept
	{
		String32 value(s);
		if (value.isNotNull()) {
			Free(_type, _value);
			Init(*this, VariantType::String32);
			new PTR_VAR(String32, _value) String32(Move(value));
		} else {
			setNull();
		}
	}

	void Variant::setString(const StringView& value) noexcept
	{
		if (value.isNotNull()) {
			Free(_type, _value);
			REF_VAR(const sl_char8*, _value) = value.getUnsafeData();
			sl_reg len = value.getUnsafeLength();
			if (len < 0) {
				Init(*this, VariantType::Sz8);
			} else {
				Init(*this, VariantType::StringData8);
				_value2 = (sl_uint32)len;
			}
		} else {
			setNull();
		}
	}

	void Variant::setString(const StringView16& value) noexcept
	{
		if (value.isNotNull()) {
			Free(_type, _value);
			REF_VAR(const sl_char16*, _value) = value.getUnsafeData();
			sl_reg len = value.getUnsafeLength();
			if (len < 0) {
				Init(*this, VariantType::Sz16);
			} else {
				Init(*this, VariantType::StringData16);
				_value2 = (sl_uint32)len;
			}
		} else {
			setNull();
		}
	}

	void Variant::setString(const StringView32& value) noexcept
	{
		if (value.isNotNull()) {
			Free(_type, _value);
			REF_VAR(const sl_char32*, _value) = value.getUnsafeData();
			sl_reg len = value.getUnsafeLength();
			if (len < 0) {
				Init(*this, VariantType::Sz32);
			} else {
				Init(*this, VariantType::StringData32);
				_value2 = (sl_uint32)len;
			}
		} else {
			setNull();
		}
	}

	void Variant::setString(const sl_char8* value) noexcept
	{
		if (value) {
			Free(_type, _value);
			Init(*this, VariantType::Sz8);
			REF_VAR(const sl_char8*, _value) = value;
		} else {
			setNull();
		}
	}

	void Variant::setString(const sl_char16* value) noexcept
	{
		if (value) {
			Free(_type, _value);
			Init(*this, VariantType::Sz16);
			REF_VAR(const sl_char16*, _value) = value;
		} else {
			setNull();
		}
	}

	void Variant::setString(const sl_char32* value) noexcept
	{
		if (value) {
			Free(_type, _value);
			Init(*this, VariantType::Sz32);
			REF_VAR(const sl_char32*, _value) = value;
		} else {
			setNull();
		}
	}

	std::string Variant::getStdString() const noexcept
	{
		return getString().toStd();
	}

	std::u16string Variant::getStdString16() const noexcept
	{
		return getString16().toStd();
	}

	std::u32string Variant::getStdString32() const noexcept
	{
		return getString32().toStd();
	}

	void Variant::setString(const std::string& value) noexcept
	{
		setString(String::create(value));
	}

	void Variant::setString(const std::u16string& value) noexcept
	{
		setString(String16::create(value));
	}

	void Variant::setString(const std::u32string& value) noexcept
	{
		setString(String32::create(value));
	}

	void Variant::setString(const StringParam& value) noexcept
	{
		set(value.toVariant());
	}

	sl_bool Variant::isTime() const noexcept
	{
		return _type == VariantType::Time;
	}

	sl_bool Variant::getTime(Time* _out) const noexcept
	{
		switch (_type) {
			case VariantType::Int32:
				if (_out) {
					*_out = Time::fromUnixTime(REF_VAR(sl_int32 const, _value));
				}
				return sl_true;
			case VariantType::Uint32:
				if (_out) {
					*_out = Time::fromUnixTime(REF_VAR(sl_uint32 const, _value));
				}
				return sl_true;
			case VariantType::Int64:
				if (_out) {
					*_out = Time::fromUnixTime(REF_VAR(sl_int64 const, _value));
				}
				return sl_true;
			case VariantType::Uint64:
				if (_out) {
					*_out = Time::fromUnixTime(REF_VAR(sl_uint64 const, _value));
				}
				return sl_true;
			case VariantType::Float:
				if (_out) {
					*_out = Time::fromUnixTimeF(REF_VAR(float const, _value));
				}
				return sl_true;
			case VariantType::Double:
				if (_out) {
					*_out = Time::fromUnixTimeF(REF_VAR(double const, _value));
				}
				return sl_true;
			case VariantType::Time:
				if (_out) {
					*_out = REF_VAR(Time const, _value);
				}
				return sl_true;
			case VariantType::String8:
				if (_out) {
					return _out->parse(REF_VAR(String const, _value));
				} else {
					Time t;
					return t.parse(REF_VAR(String const, _value));
				}
			case VariantType::String16:
				if (_out) {
					return _out->parse(REF_VAR(String16 const, _value));
				} else {
					Time t;
					return t.parse(REF_VAR(String16 const, _value));
				}
			case VariantType::String32:
				if (_out) {
					return _out->parse(REF_VAR(String32 const, _value));
				} else {
					Time t;
					return t.parse(REF_VAR(String32 const, _value));
				}
			case VariantType::Sz8:
				if (_out) {
					return _out->parse(REF_VAR(sl_char8 const* const, _value));
				} else {
					Time t;
					return t.parse(REF_VAR(sl_char8 const* const, _value));
				}
			case VariantType::Sz16:
				if (_out) {
					return _out->parse(REF_VAR(sl_char16 const* const, _value));
				} else {
					Time t;
					return t.parse(REF_VAR(sl_char16 const* const, _value));
				}
			case VariantType::Sz32:
				if (_out) {
					return _out->parse(REF_VAR(sl_char32 const* const, _value));
				} else {
					Time t;
					return t.parse(REF_VAR(sl_char32 const* const, _value));
				}
			case VariantType::StringData8:
				if (_out) {
					return _out->parse(StringParam(REF_VAR(sl_char8 const* const, _value), _value2));
				} else {
					Time t;
					return t.parse(StringParam(REF_VAR(sl_char8 const* const, _value), _value2));
				}
			case VariantType::StringData16:
				if (_out) {
					return _out->parse(StringParam(REF_VAR(sl_char16 const* const, _value), _value2));
				} else {
					Time t;
					return t.parse(StringParam(REF_VAR(sl_char16 const* const, _value), _value2));
				}
			case VariantType::StringData32:
				if (_out) {
					return _out->parse(StringParam(REF_VAR(sl_char32 const* const, _value), _value2));
				} else {
					Time t;
					return t.parse(StringParam(REF_VAR(sl_char32 const* const, _value), _value2));
				}
			default:
				break;
		}
		return sl_false;
	}

	Time Variant::getTime(const Time& def) const noexcept
	{
		Time t;
		if (getTime(&t)) {
			return t;
		}
		return def;
	}

	Time Variant::getTime() const noexcept
	{
		Time t;
		if (getTime(&t)) {
			return t;
		}
		return 0;
	}

	void Variant::setTime(const Time& value) noexcept
	{
		Free(_type, _value);
		Init(*this, VariantType::Time);
		REF_VAR(Time, _value) = value;
	}

	sl_bool Variant::isPointer() const noexcept
	{
		return _type == VariantType::Pointer || IsStringViewType(_type) || IsRef(_type);
	}

	void* Variant::getPointer(const void* def) const noexcept
	{
		if (_type == VariantType::Pointer || IsStringViewType(_type) || IsRef(_type)) {
			return REF_VAR(void* const, _value);
		}
		return (void*)def;
	}

	void Variant::setPointer(const void* ptr) noexcept
	{
		if (ptr) {
			Free(_type, _value);
			Init(*this, VariantType::Pointer);
			REF_VAR(const void*, _value) = ptr;
		} else {
			setNull();
		}
	}

	namespace {

		template <class T, sl_uint8 type>
		static sl_bool IsObject(const Variant& v)
		{
			if (v._type == type) {
				return sl_true;
			}
			if (v._type == VariantType::Weak) {
				Ref<CRef> ref(REF_VAR(WeakRef<CRef> const, v._value));
				if (ref.isNotNull()) {
					return IsInstanceOf<T>(ref);
				}
			} else if (IsRef(v._type)) {
				return IsInstanceOf<T>(REF_VAR(Ref<CRef> const, v._value));
			}
			return sl_false;
		}

		template <class T, class OT, sl_uint8 type>
		static OT GetObjectT(const Variant& v)
		{
			if (v._type == type) {
				return REF_VAR(OT, v._value);
			}
			if (v._type == VariantType::Weak) {
				Ref<CRef> ref(REF_VAR(WeakRef<CRef> const, v._value));
				if (IsInstanceOf<T>(ref)) {
					return REF_VAR(OT, ref);
				}
			} else if (IsRef(v._type)) {
				if (IsInstanceOf<T>(REF_VAR(CRef*, v._value))) {
					return REF_VAR(OT, v._value);
				}
			}
			return OT();
		}

	}

#define GET_COLLECTION(v) GetObjectT<Collection, Ref<Collection>, VariantType::Collection>(v)
#define GET_OBJECT(v) GetObjectT<Object, Ref<Object>, VariantType::Object>(v)

	sl_bool Variant::isObjectId() const noexcept
	{
		return _type == VariantType::ObjectId;
	}

	ObjectId Variant::getObjectId() const noexcept
	{
		if (_type == VariantType::ObjectId) {
			return REF_VAR(ObjectId, _value);
		} else if (isStringType()) {
			return ObjectId(getStringParam());
		} else if (_type == VariantType::Memory) {
			Memory& mem = REF_VAR(Memory, _value);
			if (mem.getSize() == 12) {
				return ObjectId((sl_uint8*)(mem.getData()));
			}
		} else if (_type == VariantType::Map) {
			VariantMap& map = REF_VAR(VariantMap, _value);
			SLIB_STATIC_STRING(oid, "$oid");
			Variant item;
			if (map.get(oid, &item)) {
				if (item.isStringType()) {
					return ObjectId(item.getStringParam());
				}
			}
		} else if (isRef()) {
			Memory mem = GetObjectT<CMemory, Memory, VariantType::Memory>(*this);
			if (mem.getSize() == 12) {
				return ObjectId((sl_uint8*)(mem.getData()));
			}
		}
		return sl_null;
	}

	sl_bool Variant::getObjectId(ObjectId* _out) const noexcept
	{
		if (_type == VariantType::ObjectId) {
			if (_out) {
				*_out = REF_VAR(ObjectId, _value);
			}
			return sl_true;
		} else if (isStringType()) {
			if (_out) {
				return _out->parse(getStringParam());
			} else {
				ObjectId ret;
				return ret.parse(getStringParam());
			}
		} else if (_type == VariantType::Memory) {
			Memory& mem = REF_VAR(Memory, _value);
			if (mem.getSize() == 12) {
				if (_out) {
					Base::copyMemory(_out->data, mem.getData(), 12);
				}
				return sl_true;
			}
		} else if (_type == VariantType::Map) {
			VariantMap& map = REF_VAR(VariantMap, _value);
			SLIB_STATIC_STRING(oid, "$oid");
			Variant item;
			if (map.get(oid, &item)) {
				if (item.isStringType()) {
					if (_out) {
						return _out->parse(item.getStringParam());
					} else {
						ObjectId ret;
						return ret.parse(item.getStringParam());
					}
				}
			}
		} else if (isRef()) {
			Memory mem = GetObjectT<CMemory, Memory, VariantType::Memory>(*this);
			if (mem.getSize() == 12) {
				if (_out) {
					Base::copyMemory(_out->data, mem.getData(), 12);
				}
				return sl_true;
			}
		}
		return sl_false;
	}

	void Variant::setObjectId(const ObjectId& _id) noexcept
	{
		Free(_type, _value);
		Init(*this, VariantType::ObjectId);
		CopyBytes12(*this, _id.data);
	}

	sl_bool Variant::isRef() const noexcept
	{
		return IsRef(_type);
	}

	Ref<CRef> Variant::getRef() const noexcept
	{
		if (_type == VariantType::Weak) {
			return REF_VAR(WeakRef<CRef> const, _value);
		} else if (IsRef(_type)) {
			return REF_VAR(Ref<CRef> const, _value);
		}
		return sl_null;
	}

	sl_object_type Variant::getObjectType() const noexcept
	{
		if (_type == VariantType::Weak) {
			Ref<CRef> ref(REF_VAR(WeakRef<CRef> const, _value));
			if (ref.isNotNull()) {
				return ref->getObjectType();
			}
		} else if (IsRef(_type)) {
			return REF_VAR(Ref<CRef> const, _value)->getObjectType();
		}
		return 0;
	}

	sl_bool Variant::isWeak() const noexcept
	{
		return _type == VariantType::Weak;
	}

	sl_bool Variant::isCollection() const noexcept
	{
		if (_type == VariantType::List) {
			return sl_true;
		}
		return IsObject<Collection, VariantType::Collection>(*this);
	}

	Ref<Collection> Variant::getCollection() const noexcept
	{
		if (_type == VariantType::List) {
			return REF_VAR(VariantList, _value).toCollection();
		}
		return GET_COLLECTION(*this);
	}

	sl_bool Variant::isVariantList() const noexcept
	{
		return _type == VariantType::List;
	}

	VariantList Variant::getVariantList() const noexcept
	{
		if (_type == VariantType::List) {
			return REF_VAR(VariantList, _value);
		} else {
			Ref<Collection> collection(GET_COLLECTION(*this));
			if (collection.isNotNull()) {
				return VariantList::create(collection.get());
			}
		}
		return sl_null;
	}

	void Variant::setVariantList(const VariantList& list) noexcept
	{
		_assignRef(&list, VariantType::List);
	}

	void Variant::setVariantList(VariantList&& list) noexcept
	{
		_assignMoveRef(&list, VariantType::List);
	}

	sl_bool Variant::isJsonList() const noexcept
	{
		return _type == VariantType::List;
	}

	JsonList Variant::getJsonList() const noexcept
	{
		if (_type == VariantType::List) {
			return REF_VAR(JsonList, _value);
		} else {
			Ref<Collection> collection(GET_COLLECTION(*this));
			if (collection.isNotNull()) {
				return JsonList::create(collection.get());
			}
		}
		return sl_null;
	}

	void Variant::setJsonList(const JsonList& list) noexcept
	{
		_assignRef(&list, VariantType::List);
	}

	void Variant::setJsonList(JsonList&& list) noexcept
	{
		_assignMoveRef(&list, VariantType::List);
	}

	sl_uint64 Variant::getElementCount() const
	{
		switch (_type) {
			case VariantType::List:
				return REF_VAR(VariantList, _value).getCount();
			case VariantType::String8:
				return REF_VAR(String, _value).getLength();
			case VariantType::String16:
				return REF_VAR(String16, _value).getLength();
			case VariantType::String32:
				return REF_VAR(String32, _value).getLength();
			case VariantType::StringData8:
			case VariantType::StringData16:
			case VariantType::StringData32:
				return _value2;
			case VariantType::Sz8:
				return Base::getStringLength(REF_VAR(sl_char8*, _value));
			case VariantType::Sz16:
				return Base::getStringLength2(REF_VAR(sl_char16*, _value));
			case VariantType::Sz32:
				return Base::getStringLength4(REF_VAR(sl_char32*, _value));
			case VariantType::Memory:
				return REF_VAR(Memory, _value).getSize();
			default:
				break;
		}
		Ref<Collection> collection(GET_COLLECTION(*this));
		if (collection.isNotNull()) {
			return collection->getElementCount();
		}
		return 0;
	}

	Variant Variant::getElement(sl_uint64 index) const
	{
		switch (_type) {
			case VariantType::List:
				return REF_VAR(VariantList, _value).getValueAt_NoLock((sl_size)index);
			case VariantType::String8:
				return REF_VAR(String, _value).getAt((sl_size)index);
			case VariantType::String16:
				return REF_VAR(String16, _value).getAt((sl_size)index);
			case VariantType::String32:
				return REF_VAR(String32, _value).getAt((sl_size)index);
			case VariantType::StringData8:
			case VariantType::Sz8:
				return REF_VAR(sl_char8*, _value)[(sl_size)index];
			case VariantType::StringData16:
			case VariantType::Sz16:
				return REF_VAR(sl_char16*, _value)[(sl_size)index];
			case VariantType::StringData32:
			case VariantType::Sz32:
				return REF_VAR(sl_char32*, _value)[(sl_size)index];
			case VariantType::Memory:
				return ((sl_uint8*)(REF_VAR(Memory, _value).getData()))[(sl_size)index];
			default:
				break;
		}
		Ref<Collection> collection(GET_COLLECTION(*this));
		if (collection.isNotNull()) {
			return collection->getElement(index);
		}
		return Variant();
	}

	sl_bool Variant::setElement(sl_uint64 index, const Variant& value) const
	{
		if (_type == VariantType::List) {
			if (value.isNotUndefined()) {
				return REF_VAR(VariantList, _value).setAt_NoLock((sl_size)index, value);
			} else {
				return REF_VAR(VariantList, _value).removeAt_NoLock((sl_size)index);
			}
		} else {
			Ref<Collection> collection(GET_COLLECTION(*this));
			if (collection.isNotNull()) {
				return collection->setElement(index, value);
			}
		}
		return sl_false;
	}

	sl_bool Variant::addElement(const Variant& value) const
	{
		if (isNotNull()) {
			if (_type == VariantType::List) {
				return REF_VAR(VariantList, _value).add_NoLock(value);
			} else {
				Ref<Collection> collection(GET_COLLECTION(*this));
				if (collection.isNotNull()) {
					return collection->addElement(value);
				}
			}
		}
		return sl_false;
	}

	sl_bool Variant::addElement(const Variant& value)
	{
		if (isNotNull()) {
			if (_type == VariantType::List) {
				return REF_VAR(VariantList, _value).add_NoLock(value);
			} else {
				Ref<Collection> collection(GET_COLLECTION(*this));
				if (collection.isNotNull()) {
					return collection->addElement(value);
				}
			}
		} else {
			auto list = VariantList::createFromElement(value);
			if (list.isNotNull()) {
				setVariantList(list);
				return sl_true;
			}
		}
		return sl_false;
	}

	sl_bool Variant::isObject() const noexcept
	{
		if (_type == VariantType::Map) {
			return sl_true;
		}
		return IsObject<Object, VariantType::Object>(*this);
	}

	Ref<Object> Variant::getObject() const noexcept
	{
		if (_type == VariantType::Map) {
			return REF_VAR(VariantMap, _value).toObject();
		} else {
			return GET_OBJECT(*this);
		}
	}

	sl_bool Variant::isVariantMap() const noexcept
	{
		return _type == VariantType::Map;
	}

	VariantMap Variant::getVariantMap() const noexcept
	{
		if (_type == VariantType::Map) {
			return REF_VAR(VariantMap, _value);
		} else {
			Ref<Object> object(GET_OBJECT(*this));
			if (object.isNotNull()) {
				return VariantMap::create(object.get());
			}
		}
		return sl_null;
	}

	void Variant::setVariantMap(const VariantMap& map) noexcept
	{
		_assignRef(&map, VariantType::Map);
	}

	void Variant::setVariantMap(VariantMap&& map) noexcept
	{
		_assignMoveRef(&map, VariantType::Map);
	}

	sl_bool Variant::isJsonMap() const noexcept
	{
		return _type == VariantType::Map;
	}

	JsonMap Variant::getJsonMap() const noexcept
	{
		if (_type == VariantType::Map) {
			return REF_VAR(JsonMap, _value);
		} else {
			Ref<Object> object(GET_OBJECT(*this));
			if (object.isNotNull()) {
				return JsonMap::create(object.get());
			}
		}
		return sl_null;
	}

	void Variant::setJsonMap(const JsonMap& map) noexcept
	{
		_assignRef(&map, VariantType::Map);
	}

	void Variant::setJsonMap(JsonMap&& map) noexcept
	{
		_assignMoveRef(&map, VariantType::Map);
	}

	Variant Variant::getItem(const String& key) const
	{
		if (_type == VariantType::Map) {
			return REF_VAR(VariantMap, _value).getValue_NoLock(key);
		} else {
			Ref<Object> object(GET_OBJECT(*this));
			if (object.isNotNull()) {
				return object->getProperty(key);
			} else {
				sl_uint64 index;
				if (StringView(key).trim().parseUint64(&index)) {
					return getElement(index);
				}
			}
		}
		return Variant();
	}

	sl_bool Variant::putItem(const String& key, const Variant& value) const
	{
		if (value.isUndefined()) {
			return removeItem(key);
		}
		if (isNotNull()) {
			if (_type == VariantType::Map) {
				return REF_VAR(VariantMap, _value).put_NoLock(key, value) != sl_null;
			} else {
				Ref<Object> object(GET_OBJECT(*this));
				if (object.isNotNull()) {
					return object->setProperty(key, value);
				} else {
					sl_uint64 index;
					if (StringView(key).trim().parseUint64(&index)) {
						return setElement(index, value);
					}
				}
			}
		}
		return sl_false;
	}

	sl_bool Variant::putItem(const String& key, const Variant& value)
	{
		if (value.isUndefined()) {
			return removeItem(key);
		}
		if (isNotNull()) {
			if (_type == VariantType::Map) {
				return REF_VAR(VariantMap, _value).put_NoLock(key, value) != sl_null;
			} else {
				Ref<Object> object(GET_OBJECT(*this));
				if (object.isNotNull()) {
					return object->setProperty(key, value);
				} else {
					sl_uint64 index;
					if (StringView(key).trim().parseUint64(&index)) {
						return setElement(index, value);
					}
				}
			}
		} else {
			VariantMap map = VariantMap::create();
			if (map.isNotNull()) {
				if (map.put_NoLock(key, value)) {
					setVariantMap(map);
					return sl_true;
				}
			}
		}
		return sl_false;
	}

	sl_bool Variant::removeItem(const String& key) const
	{
		if (_type == VariantType::Map) {
			return REF_VAR(VariantMap, _value).remove_NoLock(key);
		} else {
			Ref<Object> object(GET_OBJECT(*this));
			if (object.isNotNull()) {
				return object->clearProperty(key);
			} else {
				sl_uint64 index;
				if (StringView(key).trim().parseUint64(&index)) {
					return setElement(index, Variant());
				}
			}
		}
		return sl_false;
	}

	PropertyIterator Variant::getItemIterator() const
	{
		if (_type == VariantType::Map) {
			VariantMap& map = REF_VAR(VariantMap, _value);
			return new MapIterator< CHashMap<String, Variant> >(map.ref);
		} else {
			Ref<Object> object(GET_OBJECT(*this));
			if (object.isNotNull()) {
				return object->getPropertyIterator();
			}
		}
		return sl_null;
	}

	sl_bool Variant::isMemory() const noexcept
	{
		return IsObject<CMemory, VariantType::Memory>(*this);
	}

	Memory Variant::getMemory() const noexcept
	{
		if (_type == VariantType::Memory) {
			return REF_VAR(Memory, _value);
		} else if (isStringType()) {
			return getString().toMemory();
		} else if (_type == VariantType::Map) {
			return Memory::createFromExtendedJson(REF_VAR(VariantMap, _value));
		} else if (isRef()) {
			return GetObjectT<CMemory, Memory, VariantType::Memory>(*this);
		}
		return sl_null;
	}

	void Variant::setMemory(const Memory& mem) noexcept
	{
		_assignRef(&mem, VariantType::Memory);
	}

	void Variant::setMemory(Memory&& mem) noexcept
	{
		_assignMoveRef(&mem, VariantType::Memory);
	}

	sl_bool Variant::isBigInt() const noexcept
	{
		return IsObject<CBigInt, VariantType::BigInt>(*this);
	}

	BigInt Variant::getBigInt() const noexcept
	{
		switch (_type) {
			case VariantType::Int32:
				return REF_VAR(sl_int32 const, _value);
			case VariantType::Uint32:
				return REF_VAR(sl_uint32 const, _value);
			case VariantType::Int64:
				return REF_VAR(sl_int64 const, _value);
			case VariantType::Uint64:
				return REF_VAR(sl_uint64 const, _value);
			case VariantType::Boolean:
				return (sl_uint32)(REF_VAR(sl_bool const, _value) ? 1 : 0);
			case VariantType::BigInt:
				return REF_VAR(BigInt, _value);
			default:
				if (isStringType()) {
					return BigInt::fromString(getStringParam());
				} else if (isRef()) {
					return GetObjectT<CBigInt, BigInt, VariantType::BigInt>(*this);
				}
		}
		return sl_null;
	}

	void Variant::setBigInt(const BigInt& n) noexcept
	{
		_assignRef(&n, VariantType::BigInt);
	}

	void Variant::setBigInt(BigInt&& n) noexcept
	{
		_assignMoveRef(&n, VariantType::BigInt);
	}

	sl_bool Variant::isVariantPromise() const noexcept
	{
		return _type == VariantType::Promise;
	}

	Promise<Variant> Variant::getVariantPromise() const noexcept
	{
		if (_type == VariantType::Promise) {
			return REF_VAR(Promise<Variant> const, _value);
		}
		return sl_null;
	}

	void Variant::setVariantPromise(const Promise<Variant>& promise) noexcept
	{
		_assignRef(&promise, VariantType::Promise);
	}

	void Variant::setVariantPromise(Promise<Variant>&& promise) noexcept
	{
		_assignMoveRef(&promise, VariantType::Promise);
	}

	sl_bool Variant::isVariantFunction() const noexcept
	{
		return _type == VariantType::Function;
	}

	Function<Variant(Variant&)> Variant::getVariantFunction() const noexcept
	{
		if (_type == VariantType::Function) {
			return REF_VAR(Function<Variant(Variant&)> const, _value);
		}
		return sl_null;
	}

	void Variant::setVariantFunction(const Function<Variant(Variant&)>& func) noexcept
	{
		_assignRef(&func, VariantType::Function);
	}

	void Variant::setVariantFunction(Function<Variant(Variant&)>&& func) noexcept
	{
		_assignMoveRef(&func, VariantType::Function);
	}

	void Variant::merge(const Variant& other)
	{
		if (other.isNull()) {
			return;
		}
		if (isNull()) {
			*this = other;
			return;
		}
		if (_type == VariantType::Map) {
			VariantMap& dst = REF_VAR(VariantMap, _value);
			if (other._type == VariantType::Map) {
				VariantMap& src = REF_VAR(VariantMap, other._value);
				dst.putAll(src);
			} else {
				Ref<Object> src(getObject());
				if (src.isNotNull()) {
					MutexLocker lock(dst.getLocker());
					PropertyIterator iterator = src->getPropertyIterator();
					while (iterator.moveNext()) {
						dst.put_NoLock(iterator.getKey(), iterator.getValue());
					}
				}
			}
		} else if (_type == VariantType::List) {
			VariantList& dst = REF_VAR(VariantList, _value);
			if (other._type == VariantType::List) {
				VariantList& src = REF_VAR(VariantList, other._value);
				dst.addAll(src);
			} else {
				Ref<Collection> src(getCollection());
				if (src.isNotNull()) {
					MutexLocker lock(dst.getLocker());
					sl_size n = (sl_size)(src->getElementCount());
					for (sl_size i = 0; i < n; i++) {
						dst.add_NoLock(src->getElement(i));
					}
				}
			}
		} else if (IsRef(_type)) {
			Ref<CRef> ref = getRef();
			if (IsInstanceOf<Object>(ref)) {
				Ref<Object>& dst = Ref<Object>::from(ref);
				if (other._type == VariantType::Map) {
					VariantMap& src = REF_VAR(VariantMap, other._value);
					MutexLocker lock(src.getLocker());
					auto node = src.getFirstNode();
					while (node) {
						dst->setProperty(node->key, node->value);
						node = node->getNext();
					}
				} else {
					Ref<Object> src(getObject());
					if (src.isNotNull()) {
						PropertyIterator iterator = src->getPropertyIterator();
						while (iterator.moveNext()) {
							dst->setProperty(iterator.getKey(), iterator.getValue());
						}
					}
				}
			} else if (IsInstanceOf<Collection>(ref)) {
				Ref<Collection>& dst = Ref<Collection>::from(ref);
				if (other._type == VariantType::List) {
					ListLocker<Variant> src(REF_VAR(VariantList, other._value));
					for (sl_size i = 0; i < src.count; i++) {
						dst->addElement(src[i]);
					}
				} else {
					Ref<Collection> src(getCollection());
					if (src.isNotNull()) {
						sl_size n = (sl_size)(src->getElementCount());
						for (sl_size i = 0; i < n; i++) {
							dst->addElement(src->getElement(i));
						}
					}
				}
			}
		}
	}

	String Variant::toString() const
	{
		switch (_type) {
			case VariantType::Null:
				return sl_null;
			case VariantType::Int32:
			case VariantType::Uint32:
			case VariantType::Int64:
			case VariantType::Uint64:
			case VariantType::Float:
			case VariantType::Double:
			case VariantType::Boolean:
			case VariantType::String8:
			case VariantType::String16:
			case VariantType::String32:
			case VariantType::Sz8:
			case VariantType::Sz16:
			case VariantType::Sz32:
			case VariantType::StringData8:
			case VariantType::StringData16:
			case VariantType::StringData32:
			case VariantType::Time:
			case VariantType::Pointer:
			case VariantType::ObjectId:
			case VariantType::Memory:
			case VariantType::BigInt:
				return getString();
			case VariantType::Weak:
				{
					Ref<CRef> ref(getRef());
					if (ref.isNotNull()) {
						return ref->toString();
					} else {
						return "<null>";
					}
				}
			case VariantType::List:
			case VariantType::Map:
				return toJsonString();
			default:
				if (IsRef(_type)) {
					return REF_VAR(CRef*, _value)->toString();
				}
				return "<error-type>";
		}
	}

	sl_bool Variant::toJsonString(StringBuffer& buf) const
	{
		switch (_type) {
			case VariantType::List:
				{
					ListLocker<Variant> list(REF_VAR(VariantList, _value));
					if (!(buf.addStatic("["))) {
						return sl_false;
					}
					for (sl_size i = 0; i < list.count; i++) {
						if (i) {
							if (!(buf.addStatic(", "))) {
								return sl_false;
							}
						}
						if (!(list[i].toJsonString(buf))) {
							return sl_false;
						}
					}
					if (!(buf.addStatic("]"))) {
						return sl_false;
					}
					return sl_true;
				}
			case VariantType::Map:
				{
					VariantMap& map = REF_VAR(VariantMap, _value);
					MutexLocker locker(map.getLocker());
					if (!(buf.addStatic("{"))) {
						return sl_false;
					}
					sl_bool flagFirst = sl_true;
					auto node = map.getFirstNode();
					while (node) {
						Variant& v = node->value;
						if (v.isNotUndefined()) {
							if (!flagFirst) {
								if (!(buf.addStatic(", "))) {
									return sl_false;
								}
							}
							if (!(buf.add(Stringx::applyBackslashEscapes(node->key)))) {
								return sl_false;
							}
							if (!(buf.addStatic(": "))) {
								return sl_false;
							}
							if (!(v.toJsonString(buf))) {
								return sl_false;
							}
							flagFirst = sl_false;
						}
						node = node->getNext();
					}
					if (!(buf.addStatic("}"))) {
						return sl_false;
					}
					return sl_true;
				}
			case VariantType::Weak:
				{
					Ref<CRef> ref(getRef());
					if (ref.isNotNull()) {
						return ref->toJsonString(buf);
					} else {
						return buf.addStatic("null");
					}
				}
			default:
				if (IsRef(_type)) {
					return REF_VAR(CRef*, _value)->toJsonString(buf);
				} else {
					return buf.add(toJsonString());
				}
		}
	}

	String Variant::toJsonString() const
	{
		switch (_type) {
			case VariantType::Null:
				break;
			case VariantType::Int32:
			case VariantType::Uint32:
			case VariantType::Int64:
			case VariantType::Uint64:
			case VariantType::Float:
			case VariantType::Double:
			case VariantType::Boolean:
				return getString();
			case VariantType::Time:
			case VariantType::String8:
			case VariantType::String16:
			case VariantType::String32:
			case VariantType::Sz8:
			case VariantType::Sz16:
			case VariantType::Sz32:
			case VariantType::StringData8:
			case VariantType::StringData16:
			case VariantType::StringData32:
				return Stringx::applyBackslashEscapes(getString());
			case VariantType::ObjectId:
				return REF_VAR(ObjectId, _value).toJson().toJsonString();
			default:
				if (IsRef(_type)) {
					StringBuffer buf;
					if (toJsonString(buf)) {
						return buf.merge();
					}
				}
				break;
		}
		SLIB_RETURN_STRING("null")
	}

	sl_bool SerializeJsonBinary(MemoryBuffer* output, CRef* ref)
	{
		return ref->toJsonBinary(*output);
	}

	sl_size SerializeVariantPrimitive(const Variant& var, void* _buf, sl_size size)
	{
		sl_uint8* buf = (sl_uint8*)_buf;
		sl_size nPrefix;
		if (var._tag) {
			nPrefix = 2;
		} else {
			nPrefix = 1;
		}
		switch (var._type) {
			case VariantType::Int32:
			case VariantType::Uint32:
			case VariantType::Float:
				if (size < nPrefix + 4) {
					return 0;
				}
				MIO::writeUint32LE(buf + nPrefix, *((sl_uint32*)((void*)&(var._value))));
				size = nPrefix + 4;
				break;
			case VariantType::Int64:
			case VariantType::Uint64:
			case VariantType::Double:
			case VariantType::Time:
				if (size < nPrefix + 8) {
					return 0;
				}
				MIO::writeUint64LE(buf + nPrefix, var._value);
				size = nPrefix + 8;
				break;
			case VariantType::ObjectId:
				if (size < nPrefix + 12) {
					return 0;
				}
				Base::copyMemory(buf + nPrefix, &(var._value), 12);
				size = nPrefix + 12;
				break;
			case VariantType::Boolean:
				if (size < nPrefix + 1) {
					return 0;
				}
				buf[nPrefix] = *((bool*)((void*)&(var._value))) ? 1 : 0;
				size = nPrefix + 1;
				break;
			case VariantType::Null:
				if (size < 1) {
					return 0;
				}
				size = 1;
				break;
			default:
				return 0;
		}
		if (var._tag) {
			buf[0] = var._type | 0x80;
			buf[1] = var._tag;
		} else {
			buf[0] = var._type;
		}
		return size;
	}

#define SERIALIZE_PREPARE_MEMORY(BUF, SIZE, REQ_SIZE, PMEM) \
	if (SIZE < REQ_SIZE) { \
		if (PMEM) { \
			Memory mem = Memory::create(REQ_SIZE); \
			if (mem.isNotNull()) { \
				BUF = (sl_uint8*)(mem.getData()); \
				*PMEM = Move(mem); \
			} else { \
				return 0; \
			} \
		} else { \
			return 0; \
		} \
	}

	sl_size SerializeVariant(const Variant& var, void* _buf, sl_size size, Memory* pOutMemoryIfInsufficient, const void* prefix, sl_size sizePrefix)
	{
		sl_uint8* buf = (sl_uint8*)_buf;
		if (size > sizePrefix) {
			sl_size nWritten = SerializeVariantPrimitive(var, buf + sizePrefix, size - sizePrefix);
			if (nWritten) {
				if (sizePrefix) {
					Base::copyMemory(buf, prefix, sizePrefix);
				}
				return sizePrefix + nWritten;
			}
		}
		if (var._type == VariantType::Memory) {
			Memory& m = *((Memory*)(void*)&(var._value));
			sl_size n = m.getSize();
			sl_size nReq = sizePrefix + 12 + n;
			SERIALIZE_PREPARE_MEMORY(buf, size, nReq, pOutMemoryIfInsufficient)
			if (sizePrefix) {
				Base::copyMemory(buf, prefix, sizePrefix);
			}
			sl_size l;
			if (var._tag) {
				buf[sizePrefix] = (sl_uint8)(VariantType::Memory) | 0x80;
				buf[sizePrefix + 1] = var._tag;
				l = sizePrefix + 2;
			} else {
				buf[sizePrefix] = VariantType::Memory;
				l = sizePrefix + 1;
			}
			l += CVLI::encode(buf + l, n);
			Base::copyMemory(buf + l, m.getData(), n);
			return l + n;
		} else if (IsStringType(var._type)) {
			StringData str(var.getStringParam());
			sl_size n = str.getLength();
			sl_size nReq = sizePrefix + 12 + n;
			SERIALIZE_PREPARE_MEMORY(buf, size, nReq, pOutMemoryIfInsufficient)
			if (sizePrefix) {
				Base::copyMemory(buf, prefix, sizePrefix);
			}
			sl_size l;
			if (var._tag) {
				buf[sizePrefix] = (sl_uint8)(VariantType::String8) | 0x80;
				buf[sizePrefix + 1] = var._tag;
				l = sizePrefix + 2;
			} else {
				buf[sizePrefix] = VariantType::String8;
				l = sizePrefix + 1;
			}
			l += CVLI::encode(buf + l, n);
			Base::copyMemory(buf + l, str.getData(), n);
			return l + n;
		} else if (IsRef(var._type)) {
			MemoryBuffer mb;
			if (sizePrefix) {
				if (!(mb.addStatic(prefix, sizePrefix))) {
					return 0;
				}
			}
			if (var.serialize(&mb)) {
				Memory mem = mb.merge();
				sl_size n = mem.getSize();
				if (n) {
					if (size >= n) {
						Base::copyMemory(buf, mem.getData(), n);
						return n;
					} else {
						if (pOutMemoryIfInsufficient) {
							*pOutMemoryIfInsufficient = Move(mem);
							return n;
						}
					}
				}
			}
		}
		return 0;
	}

	Memory Variant::serialize() const
	{
		Memory mem;
		SerializeVariant(*this, sl_null, 0, &mem);
		return mem;
	}

	sl_bool Variant::serialize(MemoryBuffer* buf) const
	{
		return serialize<MemoryBuffer>(buf);
	}

	sl_size Variant::deserialize(const void* data, sl_size size)
	{
		SerializeBuffer buf(data, size);
		if (deserialize(&buf)) {
			return buf.getOffset();
		} else {
			return 0;
		}
	}

	sl_size Variant::deserialize(const MemoryView& mem)
	{
		SerializeBuffer buf(mem);
		if (deserialize(&buf)) {
			return buf.getOffset();
		} else {
			return 0;
		}
	}

	sl_compare_result Variant::compare(const Variant& other) const noexcept
	{
		if (_type == other._type) {
			switch (_type) {
				case VariantType::Null:
					return 0;
				case VariantType::Int32:
					return ComparePrimitiveValues(REF_VAR(sl_int32 const, _value), REF_VAR(sl_int32 const, other._value));
				case VariantType::Uint32:
					return ComparePrimitiveValues(REF_VAR(sl_uint32 const, _value), REF_VAR(sl_uint32 const, other._value));
				case VariantType::Boolean:
					return ComparePrimitiveValues((sl_uint32)(REF_VAR(sl_bool const, _value)), (sl_uint32)(REF_VAR(sl_bool const, other._value)));
				case VariantType::Int64:
					return ComparePrimitiveValues(REF_VAR(sl_int64 const, _value), REF_VAR(sl_int64 const, other._value));
				case VariantType::Uint64:
				case VariantType::Time:
					return ComparePrimitiveValues(_value, other._value);
				case VariantType::Float:
					return ComparePrimitiveValues(REF_VAR(float const, _value), REF_VAR(float const, other._value));
				case VariantType::Double:
					return ComparePrimitiveValues(REF_VAR(double const, _value), REF_VAR(double const, other._value));
				case VariantType::String8:
					return REF_VAR(String const, _value).compare(REF_VAR(String const, other._value));
				case VariantType::String16:
					return REF_VAR(String16 const, _value).compare(REF_VAR(String16 const, other._value));
				case VariantType::String32:
					return REF_VAR(String32 const, _value).compare(REF_VAR(String32 const, other._value));
				case VariantType::Sz8:
					return Base::compareString(REF_VAR(sl_char8 const* const, _value), REF_VAR(sl_char8 const* const, other._value));
				case VariantType::Sz16:
					return Base::compareString2(REF_VAR(sl_char16 const* const, _value), REF_VAR(sl_char16 const* const, other._value));
				case VariantType::Sz32:
					return Base::compareString4(REF_VAR(sl_char32 const* const, _value), REF_VAR(sl_char32 const* const, other._value));
				case VariantType::StringData8:
					return StringView(REF_VAR(sl_char8 const* const, _value), _value2).compare(StringView(REF_VAR(sl_char8 const* const, other._value), other._value2));
				case VariantType::StringData16:
					return StringView16(REF_VAR(sl_char16 const* const, _value), _value2).compare(StringView16(REF_VAR(sl_char16 const* const, other._value), other._value2));
				case VariantType::StringData32:
					return StringView32(REF_VAR(sl_char32 const* const, _value), _value2).compare(StringView32(REF_VAR(sl_char32 const* const, other._value), other._value2));
				case VariantType::Pointer:
					return ComparePrimitiveValues(REF_VAR(sl_size const, _value), REF_VAR(sl_size const, other._value));
				case VariantType::ObjectId:
					return REF_VAR(ObjectId const, _value).compare(REF_VAR(ObjectId const, other._value));
				case VariantType::BigInt:
					return REF_VAR(BigInt const, _value).compare(REF_VAR(BigInt const, other._value));
				default:
					if (IsRef(_type)) {
						{
							Ref<CRef> ref = getRef();
							if (ref.isNotNull()) {
								Variant result;
								if (ref->runOperator(ObjectOperator::Compare, result, other, sl_true)) {
									return result.getInt32();
								}
							}
						}
						{
							Ref<CRef> ref = other.getRef();
							if (ref.isNotNull()) {
								Variant result;
								if (ref->runOperator(ObjectOperator::Compare, result, *this, sl_false)) {
									return result.getInt32();
								}
							}
						}
						return ComparePrimitiveValues(REF_VAR(sl_size const, _value), REF_VAR(sl_size const, other._value));
					} else {
						return ComparePrimitiveValues(_value, other._value);
					}
			}
		} else {
			if (other._type == VariantType::Null) {
				return 1;
			}
			switch (_type) {
				case VariantType::Null:
					return -1;
				case VariantType::Int32:
				case VariantType::Uint32:
				case VariantType::Int64:
				case VariantType::Uint64:
					if (other.isIntegerType()) {
						return ComparePrimitiveValues(getInt64(), other.getInt64());
					}
					if (other._type == VariantType::Float || other._type == VariantType::Double) {
						return ComparePrimitiveValues(getDouble(), other.getDouble());
					}
					break;
				case VariantType::Float:
				case VariantType::Double:
					if (other.isNumberType()) {
						return ComparePrimitiveValues(getDouble(), other.getDouble());
					}
					break;
				case VariantType::String8:
				case VariantType::Sz8:
				case VariantType::StringData8:
					if (other.is8BitsStringType()) {
						return getStringView().compare(other.getStringView());
					}
					break;
				case VariantType::String16:
				case VariantType::Sz16:
				case VariantType::StringData16:
					if (other.is16BitsStringType()) {
						return getStringView16().compare(other.getStringView16());
					}
					break;
				case VariantType::String32:
				case VariantType::Sz32:
				case VariantType::StringData32:
					if (other.is8BitsStringType()) {
						return getStringView32().compare(other.getStringView32());
					}
					break;
				default:
					if (IsRef(_type)) {
						Ref<CRef> ref = getRef();
						if (ref.isNotNull()) {
							Variant result;
							if (ref->runOperator(ObjectOperator::Compare, result, other, sl_true)) {
								return result.getInt32();
							}
						}
					}
					if (IsRef(other._type)) {
						Ref<CRef> ref = other.getRef();
						if (ref.isNotNull()) {
							Variant result;
							if (ref->runOperator(ObjectOperator::Compare, result, *this, sl_false)) {
								return -(result.getInt32());
							}
						}
						if (IsRef(_type)) {
							return ComparePrimitiveValues(REF_VAR(sl_size const, _value), REF_VAR(sl_size const, other._value));
						}
					}
					break;
			}
			if (_type > other._type) {
				return 1;
			} else {
				return -1;
			}
		}
	}

	sl_bool Variant::equals(const Variant& other) const noexcept
	{
		if (_type == other._type) {
			switch (_type) {
				case VariantType::Null:
					return sl_true;
				case VariantType::Int32:
				case VariantType::Uint32:
					return REF_VAR(sl_uint32 const, _value) == REF_VAR(sl_uint32 const, other._value);
				case VariantType::Int64:
				case VariantType::Uint64:
				case VariantType::Time:
					return _value == other._value;
				case VariantType::Float:
					return REF_VAR(float const, _value) == REF_VAR(float const, other._value);
				case VariantType::Double:
					return REF_VAR(double const, _value) == REF_VAR(double const, other._value);
				case VariantType::Boolean:
					return REF_VAR(sl_bool const, _value) == REF_VAR(sl_bool const, other._value);
				case VariantType::String8:
					return REF_VAR(String const, _value) == REF_VAR(String const, other._value);
				case VariantType::String16:
					return REF_VAR(String16 const, _value) == REF_VAR(String16 const, other._value);
				case VariantType::String32:
					return REF_VAR(String32 const, _value) == REF_VAR(String32 const, other._value);
				case VariantType::Sz8:
					return Base::equalsString(REF_VAR(sl_char8 const* const, _value), REF_VAR(sl_char8 const* const, other._value));
				case VariantType::Sz16:
					return Base::equalsString2(REF_VAR(sl_char16 const* const, _value), REF_VAR(sl_char16 const* const, other._value));
				case VariantType::Sz32:
					return Base::equalsString4(REF_VAR(sl_char32 const* const, _value), REF_VAR(sl_char32 const* const, other._value));
				case VariantType::StringData8:
					return StringView(REF_VAR(sl_char8 const* const, _value), _value2) == StringView(REF_VAR(sl_char8 const* const, other._value), other._value2);
				case VariantType::StringData16:
					return StringView16(REF_VAR(sl_char16 const* const, _value), _value2) == StringView16(REF_VAR(sl_char16 const* const, other._value), other._value2);
				case VariantType::StringData32:
					return StringView32(REF_VAR(sl_char32 const* const, _value), _value2) == StringView32(REF_VAR(sl_char32 const* const, other._value), other._value2);
				case VariantType::Pointer:
					return REF_VAR(void const* const, _value) == REF_VAR(void const* const, other._value);
				case VariantType::ObjectId:
					return REF_VAR(ObjectId const, _value).equals(REF_VAR(ObjectId const, other._value));
				case VariantType::BigInt:
					return REF_VAR(BigInt const, _value).equals(REF_VAR(BigInt const, other._value));
				default:
					if (IsRef(_type)) {
						if (REF_VAR(void const* const, _value) == REF_VAR(void const* const, other._value)) {
							return sl_true;
						}
						{
							Ref<CRef> ref = getRef();
							if (ref.isNotNull()) {
								Variant result;
								if (ref->runOperator(ObjectOperator::Equals, result, other, sl_true)) {
									return result.getBoolean();
								}
							}
						}
						{
							Ref<CRef> ref = other.getRef();
							if (ref.isNotNull()) {
								Variant result;
								if (ref->runOperator(ObjectOperator::Equals, result, *this, sl_false)) {
									return result.getBoolean();
								}
							}
						}
						return sl_false;
					} else {
						return _value == other._value;
					}
			}
		} else {
			if (other._type == VariantType::Null) {
				return sl_false;
			}
			switch (_type) {
				case VariantType::Null:
					return sl_false;
				case VariantType::Int32:
				case VariantType::Uint32:
				case VariantType::Int64:
				case VariantType::Uint64:
					if (other.isIntegerType()) {
						return getInt64() == other.getInt64();
					}
					if (other._type == VariantType::Float || other._type == VariantType::Double) {
						return getDouble() == other.getDouble();
					}
					break;
				case VariantType::Float:
				case VariantType::Double:
					if (other.isNumberType()) {
						return getDouble() == other.getDouble();
					}
					break;
				case VariantType::String8:
				case VariantType::Sz8:
				case VariantType::StringData8:
					if (other.is8BitsStringType()) {
						return getStringView() == other.getStringView();
					}
					break;
				case VariantType::String16:
				case VariantType::Sz16:
				case VariantType::StringData16:
					if (other.is16BitsStringType()) {
						return getStringView16() == other.getStringView16();
					}
					break;
				case VariantType::String32:
				case VariantType::Sz32:
				case VariantType::StringData32:
					if (other.is8BitsStringType()) {
						return getStringView32() == other.getStringView32();
					}
					break;
				default:
					if (IsRef(_type)) {
						if (IsRef(other._type)) {
							if (REF_VAR(void const* const, _value) == REF_VAR(void const* const, other._value)) {
								return sl_true;
							}
						}
						Ref<CRef> ref = getRef();
						if (ref.isNotNull()) {
							Variant result;
							if (ref->runOperator(ObjectOperator::Equals, result, other, sl_true)) {
								return result.getBoolean();
							}
						}
					}
					if (IsRef(other._type)) {
						Ref<CRef> ref = other.getRef();
						if (ref.isNotNull()) {
							Variant result;
							if (ref->runOperator(ObjectOperator::Equals, result, *this, sl_false)) {
								return result.getBoolean();
							}
						}
					}
					break;
			}
		}
		return sl_false;
	}

	sl_size Variant::getHashCode() const noexcept
	{
		sl_uint8 type = _type;
		switch (type) {
			case VariantType::Null:
				return 0;
			case VariantType::Int32:
			case VariantType::Uint32:
			case VariantType::Boolean:
			case VariantType::Float:
				return Rehash32(REF_VAR(sl_int32 const, _value));
			case VariantType::String8:
				return REF_VAR(String const, _value).getHashCode();
			case VariantType::String16:
				return REF_VAR(String16 const, _value).getHashCode();
			case VariantType::String32:
				return REF_VAR(String32 const, _value).getHashCode();
			case VariantType::Sz8:
				return String::getHashCode(REF_VAR(sl_char8 const* const, _value));
			case VariantType::Sz16:
				return String16::getHashCode(REF_VAR(sl_char16 const* const, _value));
			case VariantType::Sz32:
				return String32::getHashCode(REF_VAR(sl_char32 const* const, _value));
			case VariantType::StringData8:
				return String::getHashCode(REF_VAR(sl_char8 const* const, _value), _value2);
			case VariantType::StringData16:
				return String16::getHashCode(REF_VAR(sl_char16 const* const, _value), _value2);
			case VariantType::StringData32:
				return String32::getHashCode(REF_VAR(sl_char32 const* const, _value), _value2);
			case VariantType::Pointer:
			case VariantType::Object:
				return Rehash(REF_VAR(sl_size const, _value));
			default:
				return Rehash64ToSize(_value);
		}
		return 0;
	}

	void FromVariant(const Variant& var, Variant& _out) noexcept
	{
		_out = var;
	}

	void FromVariant(const Variant& var, Atomic<Variant>& _out) noexcept
	{
		_out = var;
	}

	void FromVariant(const Variant& var, Json& _out) noexcept
	{
		_out = var;
	}

	void FromVariant(const Variant& var, signed char& _out) noexcept
	{
		_out = (signed char)(var.getInt32());
	}

	void FromVariant(const Variant& var, signed char& _out, signed char def) noexcept
	{
		_out = (signed char)(var.getInt32((sl_int32)def));
	}

	void FromVariant(const Variant& var, unsigned char& _out) noexcept
	{
		_out = (unsigned char)(var.getUint32());
	}

	void FromVariant(const Variant& var, unsigned char& _out, unsigned char def) noexcept
	{
		_out = (unsigned char)(var.getUint32((sl_uint32)def));
	}

	void FromVariant(const Variant& var, char& _out) noexcept
	{
		_out = (char)(var.getInt32());
	}

	void FromVariant(const Variant& var, char& _out, char def) noexcept
	{
		_out = (char)(var.getInt32((sl_int32)def));
	}

	void FromVariant(const Variant& var, short& _out) noexcept
	{
		_out = (short)(var.getInt32());
	}

	void FromVariant(const Variant& var, short& _out, short def) noexcept
	{
		_out = (short)(var.getInt32((sl_int32)def));
	}

	void FromVariant(const Variant& var, unsigned short& _out) noexcept
	{
		_out = (unsigned short)(var.getUint32());
	}

	void FromVariant(const Variant& var, unsigned short& _out, unsigned short def) noexcept
	{
		_out = (unsigned short)(var.getUint32((sl_uint32)def));
	}

	void FromVariant(const Variant& var, int& _out) noexcept
	{
		_out = (int)(var.getInt32());
	}

	void FromVariant(const Variant& var, int& _out, int def) noexcept
	{
		_out = (int)(var.getInt32((sl_int32)def));
	}

	void FromVariant(const Variant& var, unsigned int& _out) noexcept
	{
		_out = (unsigned int)(var.getUint32());
	}

	void FromVariant(const Variant& var, unsigned int& _out, unsigned int def) noexcept
	{
		_out = (unsigned int)(var.getUint32((sl_uint32)def));
	}

	void FromVariant(const Variant& var, long& _out) noexcept
	{
		_out = (long)(var.getInt32());
	}

	void FromVariant(const Variant& var, long& _out, long def) noexcept
	{
		_out = (long)(var.getInt32((sl_int32)def));
	}

	void FromVariant(const Variant& var, unsigned long& _out) noexcept
	{
		_out = (unsigned long)(var.getUint32());
	}

	void FromVariant(const Variant& var, unsigned long& _out, unsigned long def) noexcept
	{
		_out = (unsigned long)(var.getUint32((sl_uint32)def));
	}

	void FromVariant(const Variant& var, sl_int64& _out) noexcept
	{
		_out = var.getInt64();
	}

	void FromVariant(const Variant& var, sl_int64& _out, sl_int64 def) noexcept
	{
		_out = var.getInt64(def);
	}

	void FromVariant(const Variant& var, sl_uint64& _out) noexcept
	{
		_out = var.getUint64();
	}

	void FromVariant(const Variant& var, sl_uint64& _out, sl_uint64 def) noexcept
	{
		_out = var.getUint64(def);
	}

	void FromVariant(const Variant& var, sl_char16& _out) noexcept
	{
		_out = (sl_char16)(var.getUint32());
	}

	void FromVariant(const Variant& var, sl_char16& _out, sl_char16 def) noexcept
	{
		_out = (sl_char16)(var.getUint32((sl_uint32)def));
	}

	void FromVariant(const Variant& var, sl_char32& _out) noexcept
	{
		_out = (sl_char32)(var.getUint32());
	}

	void FromVariant(const Variant& var, sl_char32& _out, sl_char32 def) noexcept
	{
		_out = (sl_char32)(var.getUint32((sl_uint32)def));
	}

	void FromVariant(const Variant& var, float& _out) noexcept
	{
		_out = var.getFloat();
	}

	void FromVariant(const Variant& var, float& _out, float def) noexcept
	{
		_out = var.getFloat(def);
	}

	void FromVariant(const Variant& var, double& _out) noexcept
	{
		_out = var.getDouble();
	}

	void FromVariant(const Variant& var, double& _out, double def) noexcept
	{
		_out = var.getDouble(def);
	}

	void FromVariant(const Variant& var, bool& _out) noexcept
	{
		_out = var.getBoolean();
	}

	void FromVariant(const Variant& var, bool& _out, bool def) noexcept
	{
		_out = var.getBoolean(def);
	}

	void FromVariant(const Variant& var, String& _out) noexcept
	{
		_out = var.getString();
	}

	void FromVariant(const Variant& var, String& _out, const String& def) noexcept
	{
		_out = var.getString(def);
	}

	void FromVariant(const Variant& var, AtomicString& _out) noexcept
	{
		_out = var.getString();
	}

	void FromVariant(const Variant& var, AtomicString& _out, const String& def) noexcept
	{
		_out = var.getString(def);
	}

	void FromVariant(const Variant& var, String16& _out) noexcept
	{
		_out = var.getString16();
	}

	void FromVariant(const Variant& var, String16& _out, const String16& def) noexcept
	{
		_out = var.getString16(def);
	}

	void FromVariant(const Variant& var, AtomicString16& _out) noexcept
	{
		_out = var.getString16();
	}

	void FromVariant(const Variant& var, AtomicString16& _out, const String16& def) noexcept
	{
		_out = var.getString16(def);
	}

	void FromVariant(const Variant& var, String32& _out) noexcept
	{
		_out = var.getString32();
	}

	void FromVariant(const Variant& var, String32& _out, const String32& def) noexcept
	{
		_out = var.getString32(def);
	}

	void FromVariant(const Variant& var, AtomicString32& _out) noexcept
	{
		_out = var.getString32();
	}

	void FromVariant(const Variant& var, AtomicString32& _out, const String32& def) noexcept
	{
		_out = var.getString32(def);
	}

	void FromVariant(const Variant& var, std::string& _out) noexcept
	{
		_out = var.getString().toStd();
	}

	void FromVariant(const Variant& var, std::u16string& _out) noexcept
	{
		_out = var.getString16().toStd();
	}

	void FromVariant(const Variant& var, std::u32string& _out) noexcept
	{
		_out = var.getString32().toStd();
	}

	void FromVariant(const Variant& var, Time& _out) noexcept
	{
		_out = var.getTime();
	}

	void FromVariant(const Variant& var, Time& _out, const Time& def) noexcept
	{
		_out = var.getTime(def);
	}

	void FromVariant(const Variant& var, VariantList& _out) noexcept
	{
		_out = var.getVariantList();
	}

	void FromVariant(const Variant& var, VariantMap& _out) noexcept
	{
		_out = var.getVariantMap();
	}

	void FromVariant(const Variant& var, JsonList& _out) noexcept
	{
		_out = var.getJsonList();
	}

	void FromVariant(const Variant& var, JsonMap& _out) noexcept
	{
		_out = var.getJsonMap();
	}

	void FromVariant(const Variant& var, Memory& _out) noexcept
	{
		_out = var.getMemory();
	}

	void FromVariant(const Variant& var, Promise<Variant>& _out) noexcept
	{
		_out = var.getVariantPromise();
	}


	const Variant& Cast<Variant, Variant>::operator()(const Variant& var) const noexcept
	{
		return var;
	}

	String Cast<Variant, String>::operator()(const Variant& var) const noexcept
	{
		return String::from(var);
	}

	String16 Cast<Variant, String16>::operator()(const Variant& var) const noexcept
	{
		return String16::from(var);
	}

	String32 Cast<Variant, String32>::operator()(const Variant& var) const noexcept
	{
		return String32::from(var);
	}


	Variant::Variant(const VariantWrapper& t) noexcept: Variant(t.value)
	{
	}

	Variant::Variant(VariantWrapper&& t) noexcept: Variant(Move(t.value))
	{
	}


	ObjectId::ObjectId() noexcept
	{
	}

	ObjectId::ObjectId(sl_null_t) noexcept
	{
		ZeroBytes12(data);
	}

	ObjectId::ObjectId(const StringParam& _id) noexcept
	{
		if (!(parse(_id))) {
			ZeroBytes12(data);
		}
	}

	ObjectId::ObjectId(const sl_uint8* _id) noexcept: Bytes(_id)
	{
	}

	ObjectId ObjectId::generate() noexcept
	{
		static sl_uint64 random = 0;
		if (!random) {
			Math::randomMemory(&random, sizeof(random));
		}
		static volatile sl_reg counter = 0;
		sl_reg n = Base::interlockedIncrement((sl_reg*)&counter);
		ObjectId ret;
		MIO::writeUint32BE(ret.data, (sl_uint32)(Time::now().toUnixTime()));
		MIO::writeUint64BE(ret.data + 4, random + n);
		return ret;
	}

	sl_size ObjectId::getHashCode() const noexcept
	{
		return Rehash64ToSize(*(sl_uint64*)data ^ *(sl_uint32*)(data + 8));
	}

}
