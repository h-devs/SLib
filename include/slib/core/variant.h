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

#ifndef CHECKHEADER_SLIB_CORE_VARIANT
#define CHECKHEADER_SLIB_CORE_VARIANT

#include "time.h"
#include "memory.h"
#include "hash_map.h"
#include "promise.h"
#include "priv/string_cast.h"
#include "priv/variant_def.h"
#include "priv/variant_type.h"
#include "../data/object_id.h"

#ifdef SLIB_SUPPORT_STD_TYPES
#include <string>
#endif

namespace slib
{

	namespace priv
	{
		namespace variant
		{

			struct ConstContainer
			{
				sl_uint64 value;
				sl_uint8 value2[7];
				sl_uint8 type;
				sl_int32 lock;
			};

			extern const ConstContainer g_undefined;
			extern const ConstContainer g_null;

			template <class LIST>
			LIST CreateListFromCollection(Collection* collection);

			template <class LIST>
			LIST CreateListFromVariant(const Variant& var);

			template <class MAP>
			void BuildMapFromObject(MAP& map, Object* object);

			template <class MAP>
			void BuildMapFromVariant(MAP& map, const Variant& var);

		}
	}

	class VariantWrapper;
	class BigInt;
	class CBigInt;

	// No thread-safe
	class SLIB_EXPORT Variant
	{
	public:
		union {
			sl_uint64 _value;
			sl_uint8 _m1[8];

			sl_int32 _m_int32;
			sl_uint32 _m_uint32;
			sl_int64 _m_int64;
			sl_uint64 _m_uint64;
			float _m_float;
			double _m_double;
			sl_bool _m_boolean;
			const StringContainer* _m_string8;
			const StringContainer16* _m_string16;
			const sl_char8* _m_sz8;
			const sl_char16* _m_sz16;
			CRef* _m_ref;
			CWeakRef* _m_wref;
			Collection* _m_collection;
			CMemory* _m_mem;
			CBigInt* _m_bigInt;
			CPromise<Variant>* _m_promise;
			Callable<Variant(Variant&)>* _m_function;
		};
		union {
			sl_uint32 _value2;
			sl_uint8 _m2[4];
			sl_uint32 _size;
		};
		union {
			sl_uint16 _value3;
			sl_uint8 _m3[2];
		};
		sl_uint8 _tag;
		sl_uint8 _type;

	public:
		Variant() noexcept: _value(0), _tag(0), _type(VariantType::Null) {}

		Variant(sl_null_t) noexcept: _value(1), _tag(0), _type(VariantType::Null) {}

		Variant(const Variant& other) noexcept;

		Variant(Variant&& other) noexcept;

		Variant(const Atomic<Variant>& other) noexcept;

		Variant(Atomic<Variant>& other) noexcept;

		Variant(Atomic<Variant>&& other) noexcept;

		Variant(const Json& other) noexcept;

		Variant(Json&& other) noexcept;

		~Variant() noexcept;

	public:
		Variant(signed char value) noexcept;

		Variant(unsigned char value) noexcept;

		Variant(char value) noexcept;

		Variant(short value) noexcept;

		Variant(unsigned short value) noexcept;

		Variant(int value) noexcept;

		Variant(unsigned int value) noexcept;

		Variant(long value) noexcept;

		Variant(unsigned long value) noexcept;

		Variant(sl_int64 value) noexcept;

		Variant(sl_uint64 value) noexcept;

		Variant(sl_char16 value) noexcept;

		Variant(sl_char32 value) noexcept;

		Variant(float value) noexcept;

		Variant(double value) noexcept;

		Variant(sl_bool value) noexcept;

		Variant(const String& value) noexcept;

		Variant(String&& value) noexcept;

		Variant(const String16& value) noexcept;

		Variant(String16&& value) noexcept;

		Variant(const String32& value) noexcept;

		Variant(String32&& value) noexcept;

		Variant(const StringView& value) noexcept;

		Variant(const StringView16& value) noexcept;

		Variant(const StringView32& value) noexcept;

		Variant(const StringData& value) noexcept;

		Variant(const StringData16& value) noexcept;

		Variant(const StringData32& value) noexcept;

		Variant(const StringCstr& value) noexcept;

		Variant(const StringCstr16& value) noexcept;

		Variant(const StringCstr32& value) noexcept;

#ifdef SLIB_SUPPORT_STD_TYPES
		Variant(const std::string& value) noexcept;

		Variant(const std::u16string& value) noexcept;

		Variant(const std::u32string& value) noexcept;
#endif

		Variant(const sl_char8* sz8) noexcept;
		Variant(sl_char8* sz8) noexcept;

		Variant(const sl_char16* sz16) noexcept;
		Variant(sl_char16* sz16) noexcept;

		Variant(const sl_char32* sz32) noexcept;
		Variant(sl_char32* sz32) noexcept;

		Variant(const StringParam& str) noexcept;

		Variant(const Time& value) noexcept;

		template <class T>
		Variant(T* ptr) noexcept
		{
			_tag = 0;
			if (ptr) {
				_type = VariantType::Pointer;
				*((T**)(void*)&_value) = ptr;
			} else {
				_type = VariantType::Null;
				_value = 1;
			}
		}

		Variant(const ObjectId& _id) noexcept;

		template <class T>
		Variant(const Ref<T>& ref) noexcept
		{
			_constructorRef(&ref, VariantType::Ref);
		}

		template <class T>
		Variant(Ref<T>&& ref) noexcept
		{
			_constructorMoveRef(&ref, VariantType::Ref);
		}

		template <class T>
		Variant(const WeakRef<T>& weak) noexcept
		{
			_constructorRef(&weak, VariantType::Weak);
		}

		template <class T>
		Variant(WeakRef<T>&& weak) noexcept
		{
			_constructorMoveRef(&weak, VariantType::Weak);
		}

		Variant(const VariantList& list) noexcept;

		Variant(VariantList&& list) noexcept;

		Variant(const VariantMap& map) noexcept;

		Variant(VariantMap&& map) noexcept;

		Variant(const JsonList& list) noexcept;

		Variant(JsonList&& list) noexcept;

		Variant(const JsonMap& map) noexcept;

		Variant(JsonMap&& map) noexcept;

		template <class T>
		Variant(const Array<T>& arr);

		template <class T>
		Variant(const List<T>& list);

		template <class KT, class VT, class KEY_COMPARE>
		Variant(const Map<KT, VT, KEY_COMPARE>& map);

		template <class KT, class VT, class HASH, class KEY_COMPARE>
		Variant(const HashMap<KT, VT, HASH, KEY_COMPARE>& map);

		Variant(const Memory& mem) noexcept;

		Variant(Memory&& mem) noexcept;

		Variant(const BigInt& n) noexcept;

		Variant(BigInt&& n) noexcept;

		Variant(const Promise<Variant>& promise) noexcept;

		Variant(Promise<Variant>&& promise) noexcept;

		template <class T>
		Variant(const Promise<T>& promise) noexcept: Variant(Promise<Variant>::from(promise)) {}

		template <class T>
		Variant(const Function<Variant(T&)>& func) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(Callable<Variant(T&)>*, Callable<Variant(Variant&)>*)
			_constructorRef(&func, VariantType::Function);
		}

		template <class T>
		Variant(Function<Variant(T&)>&& func) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(Callable<Variant(T&)>*, Callable<Variant(Variant&)>*)
			_constructorMoveRef(&func, VariantType::Function);
		}

		template <class T>
		Variant(const Nullable<T>& value) noexcept
		{
			if (value.isNotNull()) {
				new (this) Variant(value.value);
			} else {
				_tag = 0;
				_type = VariantType::Null;
				_value = 0;
			}
		}

		template <class T>
		Variant(const Atomic<T>& t) noexcept: Variant(T(t)) {}

		Variant(const VariantWrapper& t) noexcept;
	
		Variant(VariantWrapper&& t) noexcept;

		template <class T>
		Variant(const T& value);

		template <class T>
		Variant(T&& arg, sl_uint8 tag): Variant(Forward<T>(arg))
		{
			_tag = tag;
		}

	public:
		Variant& operator=(const Variant& other) noexcept;

		Variant& operator=(Variant&& other) noexcept;

		Variant& operator=(const Atomic<Variant>& other) noexcept;

		Variant& operator=(Atomic<Variant>& other) noexcept;

		Variant& operator=(Atomic<Variant>&& other) noexcept;

		Variant& operator=(const Json& other) noexcept;

		Variant& operator=(Json&& other) noexcept;

		Variant& operator=(sl_null_t) noexcept;

		template <class T>
		Variant& operator=(T&& value) noexcept
		{
			set(Forward<T>(value));
			return *this;
		}

		Variant operator+(const Variant& other) const noexcept;

		Variant operator-(const Variant& other) const noexcept;

		Variant operator*(const Variant& other) const noexcept;

		Variant operator/(const Variant& other) const noexcept;

		Variant operator%(const Variant& other) const noexcept;

		Variant operator-() const noexcept;

		explicit operator sl_bool() const noexcept;

		sl_bool operator!() const noexcept;

		Variant operator~() const noexcept;

		Variant operator||(const Variant& other) const noexcept;

		Variant operator&&(const Variant& other) const noexcept;

		Variant operator|(const Variant& other) const noexcept;

		Variant operator&(const Variant& other) const noexcept;

		Variant operator^(const Variant& other) const noexcept;

		Variant operator>>(const Variant& other) const noexcept;

		Variant operator<<(const Variant& other) const noexcept;

		Variant operator[](sl_uint64 index) const noexcept;

		Variant operator[](const String& key) const noexcept;

		SLIB_DEFINE_CLASS_DEFAULT_COMPARE_OPERATORS

	public:
		static const Variant& undefined() noexcept
		{
			return *(reinterpret_cast<Variant const*>(&(priv::variant::g_undefined)));
		}

		static const Variant& null() noexcept
		{
			return *(reinterpret_cast<Variant const*>(&(priv::variant::g_null)));
		}

	public:
		SLIB_CONSTEXPR sl_uint8 getType() const
		{
			return _type;
		}

		SLIB_CONSTEXPR sl_uint8 getTag() const
		{
			return _tag;
		}

		void setTag(sl_uint8 tag) noexcept
		{
			_tag = tag;
		}

		void setUndefined() noexcept;

		SLIB_CONSTEXPR sl_bool isUndefined() const
		{
			return _type == VariantType::Null && !_value;
		}

		SLIB_CONSTEXPR sl_bool isNotUndefined() const
		{
			return _type != VariantType::Null || _value != 0;
		}

		void setNull() noexcept;

		SLIB_CONSTEXPR sl_bool isNull() const
		{
			return _type == VariantType::Null;
		}

		SLIB_CONSTEXPR sl_bool isNotNull() const
		{
			return _type != VariantType::Null;
		}

		sl_bool isInt32() const noexcept;

		sl_bool getInt32(sl_int32* _out) const noexcept;

		sl_int32 getInt32(sl_int32 def = 0) const noexcept;

		void setInt32(sl_int32 value) noexcept;

		sl_bool getInt8(sl_int8* _out) const noexcept;

		sl_int8 getInt8(sl_int8 def = 0) const noexcept;

		void setInt8(sl_int8 value) noexcept;

		sl_bool getInt16(sl_int16* _out) const noexcept;

		sl_int16 getInt16(sl_int16 def = 0) const noexcept;

		void setInt16(sl_int16 value) noexcept;

		sl_bool isUint32() const noexcept;

		sl_bool getUint32(sl_uint32* _out) const noexcept;

		sl_uint32 getUint32(sl_uint32 def = 0) const noexcept;

		void setUint32(sl_uint32 value) noexcept;

		sl_bool getUint8(sl_uint8* _out) const noexcept;

		sl_uint8 getUint8(sl_uint8 def = 0) const noexcept;

		void setUint8(sl_uint8 value) noexcept;

		sl_bool getUint16(sl_uint16* _out) const noexcept;

		sl_uint16 getUint16(sl_uint16 def = 0) const noexcept;

		void setUint16(sl_uint16 value) noexcept;

		sl_bool isInt64() const noexcept;

		sl_bool getInt64(sl_int64* _out) const noexcept;

		sl_int64 getInt64(sl_int64 def = 0) const noexcept;

		void setInt64(sl_int64 value) noexcept;

		sl_bool isUint64() const noexcept;

		sl_bool getUint64(sl_uint64* _out) const noexcept;

		sl_uint64 getUint64(sl_uint64 def = 0) const noexcept;

		void setUint64(sl_uint64 value) noexcept;

		sl_bool isIntegerType() const noexcept;

		sl_bool isSignedIntegerType() const noexcept;

		sl_bool isUnsignedIntegerType() const noexcept;

		sl_uint32 getIntegerSize() const noexcept;

		sl_bool isFloat() const noexcept;

		sl_bool getFloat(float* _out) const noexcept;

		float getFloat(float def = 0) const noexcept;

		void setFloat(float value) noexcept;

		sl_bool isDouble() const noexcept;

		sl_bool getDouble(double* _out) const noexcept;

		double getDouble(double def = 0) const noexcept;

		void setDouble(double value) noexcept;

		sl_bool isNaN() const noexcept;

		sl_bool isInfinite() const noexcept;

		sl_bool isPositiveInfinite() const noexcept;

		sl_bool isNegativeInfinite() const noexcept;

		sl_bool isNumberType() const noexcept;


		sl_bool isBoolean() const noexcept;

		sl_bool isTrue() const noexcept;

		sl_bool isFalse() const noexcept;

		sl_bool getBoolean(sl_bool def = sl_false) const noexcept;

		void setBoolean(sl_bool value) noexcept;


		sl_bool isStringType() const noexcept;

		sl_bool is8BitsStringType() const noexcept;

		sl_bool is16BitsStringType() const noexcept;

		sl_bool is32BitsStringType() const noexcept;

		sl_bool isStringObject8() const noexcept;

		sl_bool isStringObject16() const noexcept;

		sl_bool isStringObject32() const noexcept;

		sl_bool isStringView8() const noexcept;

		sl_bool isStringView16() const noexcept;

		sl_bool isStringView32() const noexcept;

		sl_bool isSz8() const noexcept;

		sl_bool isSz16() const noexcept;

		sl_bool isSz32() const noexcept;

		String getString(const String& def) const noexcept;

		String getString() const noexcept;

		String16 getString16(const String16& def) const noexcept;

		String16 getString16() const noexcept;

		String32 getString32(const String32& def) const noexcept;

		String32 getString32() const noexcept;

		StringView getStringView(const StringView& def) const noexcept;

		StringView getStringView() const noexcept;

		StringView16 getStringView16(const StringView16& def) const noexcept;

		StringView16 getStringView16() const noexcept;

		StringView32 getStringView32(const StringView32& def) const noexcept;

		StringView32 getStringView32() const noexcept;

		sl_char8* getSz8(const sl_char8* def = sl_null) const noexcept;

		sl_char16* getSz16(const sl_char16* def = sl_null) const noexcept;

		sl_char32* getSz32(const sl_char32* def = sl_null) const noexcept;

		StringParam getStringParam(const StringParam& def) const noexcept;

		StringParam getStringParam() const noexcept;

		sl_bool getStringData(StringRawData& data) const noexcept;

		void setString(const String& value) noexcept;

		void setString(String&& value) noexcept;

		void setString(const String16& value) noexcept;

		void setString(String16&& value) noexcept;

		void setString(const String32& value) noexcept;

		void setString(String32&& value) noexcept;

		void setString(const AtomicString& value) noexcept;

		void setString(const AtomicString16& value) noexcept;

		void setString(const AtomicString32& value) noexcept;

		void setString(const StringView& value) noexcept;

		void setString(const StringView16& value) noexcept;

		void setString(const StringView32& value) noexcept;

		void setString(const sl_char8* sz8) noexcept;

		void setString(const sl_char16* sz16) noexcept;

		void setString(const sl_char32* sz32) noexcept;

#ifdef SLIB_SUPPORT_STD_TYPES
		std::string getStdString() const noexcept;

		std::u16string getStdString16() const noexcept;

		std::u32string getStdString32() const noexcept;

		void setString(const std::string& value) noexcept;

		void setString(const std::u16string& value) noexcept;

		void setString(const std::u32string& value) noexcept;
#endif

		void setString(const StringParam& value) noexcept;


		sl_bool isTime() const noexcept;

		sl_bool getTime(Time* _out) const noexcept;

		Time getTime(const Time& def) const noexcept;

		Time getTime() const noexcept;

		void setTime(const Time& value) noexcept;


		sl_bool isPointer() const noexcept;

		void* getPointer(const void* def = sl_null) const noexcept;

		void setPointer(const void* ptr) noexcept;


		sl_bool isObjectId() const noexcept;

		ObjectId getObjectId() const noexcept;

		sl_bool getObjectId(ObjectId* _out) const noexcept;

		void setObjectId(const ObjectId& _id) noexcept;


		sl_bool isRef() const noexcept;

		Ref<CRef> getRef() const noexcept;

		template <class T>
		Ref<T> getRef(const Ref<T>& def) const noexcept
		{
			return CastRef<T>(getRef(), def);
		}

		template <class T>
		void setRef(T&& t) noexcept
		{
			_setRef(Forward<T>(t), VariantType::Ref);
		}

		template <class T>
		static Variant fromRef(T&& t) noexcept
		{
			Variant ret;
			ret._initRef(Forward<T>(t), VariantType::Ref);
			return ret;
		}

		sl_object_type getObjectType() const noexcept;


		sl_bool isWeak() const noexcept;

		template <class T>
		void setWeak(const WeakRef<T>& weak) noexcept
		{
			_assignRef(&weak, VariantType::Weak);
		}

		template <class T>
		void setWeak(WeakRef<T>&& weak) noexcept
		{
			_assignMoveRef(&weak, VariantType::Weak);
		}

		template <class T>
		Variant fromWeak(const WeakRef<T>& weak) noexcept
		{
			Variant ret;
			ret._initRef(&weak, VariantType::Weak);
			return ret;
		}

		template <class T>
		Variant fromWeak(WeakRef<T>&& weak) noexcept
		{
			Variant ret;
			ret._initRef(&weak, VariantType::Weak);
			return ret;
		}


		sl_bool isCollection() const noexcept;

		Ref<Collection> getCollection() const noexcept;

		template <class T>
		void setCollection(T&& t) noexcept
		{
			_setRef(Forward<T>(t), VariantType::Collection);
		}

		template <class T>
		static Variant fromCollection(T&& t) noexcept
		{
			Variant ret;
			ret._initRef(Forward<T>(t), VariantType::Collection);
			return ret;
		}

		sl_bool isVariantList() const noexcept;

		VariantList getVariantList() const noexcept;

		void setVariantList(const VariantList& list) noexcept;

		void setVariantList(VariantList&& list) noexcept;

		sl_bool isJsonList() const noexcept;

		JsonList getJsonList() const noexcept;

		void setJsonList(const JsonList& list) noexcept;

		void setJsonList(JsonList&& list) noexcept;

		sl_uint64 getElementCount() const;

		Variant getElement(sl_uint64 index) const;

		sl_bool setElement(sl_uint64 index, const Variant& value) const;

		sl_bool addElement(const Variant& value) const;

		sl_bool addElement(const Variant& value);


		sl_bool isObject() const noexcept;

		Ref<Object> getObject() const noexcept;

		template <class T>
		void setObject(T&& t) noexcept
		{
			_setRef(Forward<T>(t), VariantType::Object);
		}

		template <class T>
		static Variant fromObject(T&& t) noexcept
		{
			Variant ret;
			ret._initRef(Forward<T>(t), VariantType::Object);
			return ret;
		}

		sl_bool isVariantMap() const noexcept;

		VariantMap getVariantMap() const noexcept;

		void setVariantMap(const VariantMap& map) noexcept;

		void setVariantMap(VariantMap&& map) noexcept;

		sl_bool isJsonMap() const noexcept;

		JsonMap getJsonMap() const noexcept;

		void setJsonMap(const JsonMap& map) noexcept;

		void setJsonMap(JsonMap&& map) noexcept;

		Variant getItem(const String& key) const;

		sl_bool putItem(const String& key, const Variant& value) const;

		sl_bool putItem(const String& key, const Variant& value);

		sl_bool putItems(const Variant& other) const;

		sl_bool putItems(const Variant& other);

		sl_bool removeItem(const String& key) const;

		PropertyIterator getItemIterator() const;


		sl_bool isMemory() const noexcept;

		Memory getMemory() const noexcept;

		void setMemory(const Memory& mem) noexcept;

		void setMemory(Memory&& mem) noexcept;


		sl_bool isBigInt() const noexcept;

		BigInt getBigInt() const noexcept;

		void setBigInt(const BigInt& n) noexcept;

		void setBigInt(BigInt&& n) noexcept;


		sl_bool isVariantPromise() const noexcept;

		Promise<Variant> getVariantPromise() const noexcept;

		void setVariantPromise(const Promise<Variant>& promise) noexcept;

		void setVariantPromise(Promise<Variant>&& promise) noexcept;

		template <class T>
		void setVariantPromise(const Promise<T>& promise) noexcept
		{
			setVariantPromise(Promise<Variant>::from(promise));
		}


		sl_bool isVariantFunction() const noexcept;

		Function<Variant(Variant&)> getVariantFunction() const noexcept;

		void setVariantFunction(const Function<Variant(Variant&)>& promise) noexcept;

		void setVariantFunction(Function<Variant(Variant&)>&& promise) noexcept;


		void merge(const Variant& other);

		Variant duplicate() const;


		String toString() const;

		sl_bool toJsonString(StringBuffer& buf) const;

		String toJsonString() const;

		Memory serialize() const;

		sl_bool serialize(MemoryBuffer* buf) const;

		template <class OUTPUT>
		sl_bool serialize(OUTPUT* output) const;

		sl_size deserialize(const void* data, sl_size size);

		sl_size deserialize(const MemoryView& mem);

		template <class INPUT>
		sl_bool deserialize(INPUT* input);

		template <class... ARGS>
		static Variant getDeserialized(ARGS&&... args)
		{
			Variant ret;
			if (ret.deserialize(Forward<ARGS>(args)...)) {
				return ret;
			}
			return Variant();
		}


		sl_compare_result compare(const Variant& other) const noexcept;

		sl_bool equals(const Variant& other) const noexcept;

		sl_size getHashCode() const noexcept;


		template <class T>
		void set(T&& t)
		{
			_free(_type, _value);
			new (this) Variant(Forward<T>(t));
		}

	protected:
		void _assign(const Variant& other) noexcept;

		void _assignMove(Variant& other) noexcept;

		void _constructorRef(const void* ptr, sl_uint8 type) noexcept;

		void _constructorMoveRef(void* ptr, sl_uint8 type) noexcept;

		void _assignRef(const void* ptr, sl_uint8 type) noexcept;

		void _assignMoveRef(void* ptr, sl_uint8 type) noexcept;

		static void _free(sl_uint8 type, sl_uint64 value) noexcept;

		template <class T>
		void _initRef(T* ref, sl_uint8 type) noexcept
		{
			_constructorRef(&ref, type);
		}

		template <class T>
		void _initRef(const Ref<T>& ref, sl_uint8 type) noexcept
		{
			_constructorRef(&ref, type);
		}

		template <class T>
		void _initRef(Ref<T>&& ref, sl_uint8 type) noexcept
		{
			_constructorMoveRef(&ref, type);
		}

		template <class T>
		void _initRef(const AtomicRef<T>& ref, sl_uint8 type) noexcept
		{
			_initRef(Ref<T>(ref), type);
		}

		template <class T>
		void _initRef(const WeakRef<T>& ref, sl_uint8 type) noexcept
		{
			_initRef(Ref<T>(ref), type);
		}

		template <class T>
		void _initRef(const AtomicWeakRef<T>& ref, sl_uint8 type) noexcept
		{
			_initRef(Ref<T>(ref), type);
		}

		template <class T>
		void _setRef(T* ref, sl_uint8 type) noexcept
		{
			_assignRef(&ref, type);
		}

		template <class T>
		void _setRef(const Ref<T>& ref, sl_uint8 type) noexcept
		{
			_assignRef(&ref, type);
		}

		template <class T>
		void _setRef(Ref<T>&& ref, sl_uint8 type) noexcept
		{
			_assignMoveRef(&ref, type);
		}

		template <class T>
		void _setRef(const AtomicRef<T>& ref, sl_uint8 type) noexcept
		{
			_setRef(Ref<T>(ref), type);
		}

		template <class T>
		void _setRef(const WeakRef<T>& ref, sl_uint8 type) noexcept
		{
			_setRef(Ref<T>(ref), type);
		}

		template <class T>
		void _setRef(const AtomicWeakRef<T>& ref, sl_uint8 type) noexcept
		{
			_setRef(Ref<T>(ref), type);
		}

	};

	void FromVariant(const Variant& var, Variant& _out) noexcept;
	void FromVariant(const Variant& var, Atomic<Variant>& _out) noexcept;
	void FromVariant(const Variant& var, Json& _out) noexcept;

	void FromVariant(const Variant& var, signed char& _out) noexcept;
	void FromVariant(const Variant& var, signed char& _out, signed char def) noexcept;
	void FromVariant(const Variant& var, unsigned char& _out) noexcept;
	void FromVariant(const Variant& var, unsigned char& _out, unsigned char def) noexcept;
	void FromVariant(const Variant& var, short& _out) noexcept;
	void FromVariant(const Variant& var, short& _out, short def) noexcept;
	void FromVariant(const Variant& var, unsigned short& _out) noexcept;
	void FromVariant(const Variant& var, unsigned short& _out, unsigned short def) noexcept;
	void FromVariant(const Variant& var, int& _out) noexcept;
	void FromVariant(const Variant& var, int& _out, int def) noexcept;
	void FromVariant(const Variant& var, unsigned int& _out) noexcept;
	void FromVariant(const Variant& var, unsigned int& _out, unsigned int def) noexcept;
	void FromVariant(const Variant& var, long& _out) noexcept;
	void FromVariant(const Variant& var, long& _out, long def) noexcept;
	void FromVariant(const Variant& var, unsigned long& _out) noexcept;
	void FromVariant(const Variant& var, unsigned long& _out, unsigned long def) noexcept;
	void FromVariant(const Variant& var, sl_int64& _out) noexcept;
	void FromVariant(const Variant& var, sl_int64& _out, sl_int64 def) noexcept;
	void FromVariant(const Variant& var, sl_uint64& _out) noexcept;
	void FromVariant(const Variant& var, sl_uint64& _out, sl_uint64 def) noexcept;
	void FromVariant(const Variant& var, float& _out) noexcept;
	void FromVariant(const Variant& var, float& _out, float def) noexcept;
	void FromVariant(const Variant& var, double& _out) noexcept;
	void FromVariant(const Variant& var, double& _out, double def) noexcept;
	void FromVariant(const Variant& var, bool& _out) noexcept;
	void FromVariant(const Variant& var, bool& _out, bool def) noexcept;

	void FromVariant(const Variant& var, String& _out) noexcept;
	void FromVariant(const Variant& var, String& _out, const String& def) noexcept;
	void FromVariant(const Variant& var, AtomicString& _out) noexcept;
	void FromVariant(const Variant& var, AtomicString& _out, const String& def) noexcept;
	void FromVariant(const Variant& var, String16& _out) noexcept;
	void FromVariant(const Variant& var, String16& _out, const String16& def) noexcept;
	void FromVariant(const Variant& var, AtomicString16& _out) noexcept;
	void FromVariant(const Variant& var, AtomicString16& _out, const String16& def) noexcept;
	void FromVariant(const Variant& var, String32& _out) noexcept;
	void FromVariant(const Variant& var, String32& _out, const String32& def) noexcept;
	void FromVariant(const Variant& var, AtomicString32& _out) noexcept;
	void FromVariant(const Variant& var, AtomicString32& _out, const String32& def) noexcept;
#ifdef SLIB_SUPPORT_STD_TYPES
	void FromVariant(const Variant& var, std::string& _out) noexcept;
	void FromVariant(const Variant& var, std::u16string& _out) noexcept;
	void FromVariant(const Variant& var, std::u32string& _out) noexcept;
#endif

	void FromVariant(const Variant& var, Time& _out) noexcept;
	void FromVariant(const Variant& var, Time& _out, const Time& def) noexcept;

	template <class T>
	static void FromVariant(const Variant& var, Ref<T>& _out) noexcept
	{
		_out = CastRef<T>(var.getRef());
	}

	template <class T>
	static void FromVariant(const Variant& var, WeakRef<T>& _out) noexcept
	{
		_out = CastRef<T>(var.getRef());
	}

	void FromVariant(const Variant& var, VariantList& _out) noexcept;
	void FromVariant(const Variant& var, VariantMap& _out) noexcept;
	void FromVariant(const Variant& var, JsonList& _out) noexcept;
	void FromVariant(const Variant& var, JsonMap& _out) noexcept;

	template <class T>
	static void FromVariant(const Variant& var, Array<T>& _out) noexcept
	{
		_out = priv::variant::CreateListFromVariant< Array<T> >(var);
	}

	template <class T>
	static void FromVariant(const Variant& var, List<T>& _out) noexcept
	{
		_out = priv::variant::CreateListFromVariant< List<T> >(var);
	}

	template <class KT, class VT, class KEY_COMPARE>
	static void FromVariant(const Variant& var, Map<KT, VT, KEY_COMPARE>& _out) noexcept
	{
		_out.setNull();
		priv::variant::BuildMapFromVariant(_out, var);
	}

	template <class KT, class VT, class HASH, class KEY_COMPARE>
	static void FromVariant(const Variant& var, HashMap<KT, VT, HASH, KEY_COMPARE>& _out)
	{
		_out.setNull();
		priv::variant::BuildMapFromVariant(_out, var);
	}

	void FromVariant(const Variant& var, Memory& _out) noexcept;

	void FromVariant(const Variant& var, Promise<Variant>& _out) noexcept;

	template <class T>
	static void FromVariant(const Variant& var, Nullable<T>& _out) noexcept
	{
		if (var.isUndefined()) {
			_out->setNull();
		} else {
			_out.flagNull = sl_false;
			FromVariant(var, _out.value);
		}
	}

	template <class T>
	static void FromVariant(const Variant& var, Atomic<T>& _out) noexcept
	{
		T t;
		FromVariant(var, t);
		_out = Move(t);
	}

	template <class T>
	static void FromVariant(const Variant& var, T& _out)
	{
		_out.setJson(*(reinterpret_cast<const Json*>(&var)));
	}

	template <class T>
	static Variant ToVariant(const T& _in)
	{
		return _in.toJson();
	}

	namespace priv
	{
		namespace variant
		{

			template <class T, sl_bool isEnum=__is_enum(typename RemoveReference<T>::Type)>
			class InitHelper
			{
			public:
				template <class V>
				static Variant from(V&& v)
				{
					return ToVariant(Forward<V>(v));
				}

			};

			template <class T>
			class InitHelper<T, sl_true>
			{
			public:
				constexpr static sl_uint64 from(T v) noexcept
				{
					return (sl_uint64)v;
				}

			};

		}
	}

	template <class T>
	Variant::Variant(const T& value): Variant(priv::variant::InitHelper<T>::from(value))
	{
	}

	class VariantWrapper
	{
	public:
		Variant value;
	};

	template <class... ARGS>
	SLIB_INLINE String String::format(const StringView& strFormat, ARGS&&... args) noexcept
	{
		Variant params[] = {Forward<ARGS>(args)...};
		return formatBy(strFormat, params, sizeof...(args));
	}

	template <class... ARGS>
	SLIB_INLINE String16 String16::format(const StringView16& strFormat, ARGS&&... args) noexcept
	{
		Variant params[] = {Forward<ARGS>(args)...};
		return formatBy(strFormat, params, sizeof...(args));
	}

	template <class... ARGS>
	SLIB_INLINE String32 String32::format(const StringView32& strFormat, ARGS&&... args) noexcept
	{
		Variant params[] = { Forward<ARGS>(args)... };
		return formatBy(strFormat, params, sizeof...(args));
	}

	template <class... ARGS>
	SLIB_INLINE String String::format(const Locale& locale, const StringView& strFormat, ARGS&&... args) noexcept
	{
		Variant params[] = {Forward<ARGS>(args)...};
		return formatBy(locale, strFormat, params, sizeof...(args));
	}

	template <class... ARGS>
	SLIB_INLINE String16 String16::format(const Locale& locale, const StringView16& strFormat, ARGS&&... args) noexcept
	{
		Variant params[] = {Forward<ARGS>(args)...};
		return formatBy(locale, strFormat, params, sizeof...(args));
	}

	template <class... ARGS>
	SLIB_INLINE String32 String32::format(const Locale& locale, const StringView32& strFormat, ARGS&&... args) noexcept
	{
		Variant params[] = { Forward<ARGS>(args)... };
		return formatBy(locale, strFormat, params, sizeof...(args));
	}

	template <class T>
	class Cast<T, Variant>
	{
	public:
		Variant operator()(const T& v) const noexcept
		{
			return v;
		}
	};

	template <>
	class Cast<Variant, Variant>
	{
	public:
		const Variant& operator()(const Variant& v) const noexcept;
	};

	template <>
	class Cast<Variant, String>
	{
	public:
		String operator()(const Variant& var) const noexcept;
	};

	template <>
	class Cast<Variant, String16>
	{
	public:
		String16 operator()(const Variant& var) const noexcept;
	};

	template <>
	class Cast<Variant, String32>
	{
	public:
		String32 operator()(const Variant& var) const noexcept;
	};

	namespace priv
	{
		namespace variant
		{

			template <class LIST>
			LIST CreateListFromCollection(Collection* collection)
			{
				if (collection) {
					sl_size n = (sl_size)(collection->getElementCount());
					if (n) {
						LIST ret = LIST::create(n);
						if (ret.isNotNull()) {
							auto data = ret.getData();
							for (sl_size i = 0; i < n; i++) {
								FromVariant(collection->getElement(i), data[i]);
							}
							return ret;
						}
					}
				}
				return sl_null;
			}

			template <class LIST>
			LIST CreateListFromVariant(const Variant& var)
			{
				if (var.getType() == VariantType::List) {
					VariantList list = var.getVariantList();
					if (list.isNotNull()) {
						ListLocker<Variant> src(list);
						if (src.count) {
							LIST ret = LIST::create(src.count);
							if (ret.isNotNull()) {
								auto dst = ret.getData();
								for (sl_size i = 0; i < src.count; i++) {
									FromVariant(src[i], dst[i]);
								}
								return ret;
							}
						}
					}
				} else {
					Ref<Collection> src = var.getCollection();
					if (src.isNotNull()) {
						return CreateListFromCollection<LIST>(src.get());
					}
				}
				return sl_null;
			}

			template <class MAP>
			void BuildMapFromObject(MAP& map, Object* object)
			{
				if (object) {
					PropertyIterator iterator = object->getPropertyIterator();
					while (iterator.moveNext()) {
						typename MAP::VALUE_TYPE v;
						FromVariant(iterator.getValue(), v);
						map.add_NoLock(Cast<String, typename MAP::KEY_TYPE>()(iterator.getKey()), Move(v));
					}
				}
			}

			template <class MAP>
			void BuildMapFromVariant(MAP& _out, const Variant& var)
			{
				if (var.getType() == VariantType::Map) {
					VariantMap src = var.getVariantMap();
					if (src.isNotNull()) {
						MutexLocker lock(src.getLocker());
						auto node = src.getFirstNode();
						while (node) {
							typename MAP::VALUE_TYPE v;
							FromVariant(node->value, v);
							_out.add_NoLock(Cast<String, typename MAP::KEY_TYPE>()(node->key), Move(v));
							node = node->getNext();
						}
					}
				} else {
					Ref<Object> src = var.getObject();
					if (src.isNotNull()) {
						BuildMapFromObject(_out, src.get());
						return;
					}
				}
			}

		}
	}

}

#endif
