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

#include "string.h"
#include "time.h"
#include "memory.h"
#include "hash_map.h"
#include "promise.h"
#include "string_cast.h"
#include "object_id.h"
#include "variant_def.h"
#include "variant_type.h"

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

			template <class T, sl_bool isEnum=__is_enum(typename RemoveReference<T>::Type)>
			class VariantExHelper
			{
			public:
				constexpr static T&& forward(typename RemoveReference<T>::Type& v) noexcept
				{
					return static_cast<T&&>(v);
				}

				constexpr static T&& forward(typename RemoveReference<T>::Type&& v) noexcept
				{
					static_assert(!(IsLValue<T>()), "Can't forward an rvalue as an lvalue.");
					return static_cast<T&&>(v);
				}

			};

			template <class T>
			class VariantExHelper<T, sl_true>
			{
			public:
				constexpr static sl_uint64 forward(T v) noexcept
				{
					return (sl_uint64)v;
				}

			};

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

	class ObjectStorage;

	class SLIB_EXPORT Variant
	{
	public:
		union {
			sl_uint64 _value;

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
			Referable* _m_ref;
			CWeakRef* _m_wref;
			Collection* _m_collection;
			CPromise<Variant>* _m_promise;
			CMemory* _m_mem;
		};
		sl_uint8 _value2[7];
		sl_uint8 _type;

	public:
		Variant() noexcept : _value(0), _type(VariantType::Null) {}

		Variant(sl_null_t) noexcept : _value(1), _type(VariantType::Null) {}

		Variant(const Variant& other) noexcept;

		Variant(Variant&& other) noexcept;

		Variant(const Atomic<Variant>& other) noexcept;

		Variant(const Json& other) noexcept;

		Variant(Json&& other) noexcept;

		~Variant() noexcept;

	public:
		Variant(signed char value) noexcept;

		Variant(unsigned char value) noexcept;

		Variant(short value) noexcept;

		Variant(unsigned short value) noexcept;

		Variant(int value) noexcept;

		Variant(unsigned int value) noexcept;

		Variant(long value) noexcept;

		Variant(unsigned long value) noexcept;

		Variant(sl_int64 value) noexcept;

		Variant(sl_uint64 value) noexcept;

		Variant(float value) noexcept;

		Variant(double value) noexcept;

		Variant(sl_bool value) noexcept;

		Variant(const String& value) noexcept;

		Variant(String&& value) noexcept;

		Variant(const String16& value) noexcept;

		Variant(String16&& value) noexcept;

		Variant(const StringView& value) noexcept;

		Variant(const StringView16& value) noexcept;

#ifdef SLIB_SUPPORT_STD_TYPES
		Variant(const std::string& value) noexcept;

		Variant(const std::u16string& value) noexcept;
#endif

		Variant(const sl_char8* sz8) noexcept;
		Variant(sl_char8* sz8) noexcept;

		Variant(const sl_char16* sz16) noexcept;
		Variant(sl_char16* sz16) noexcept;

		Variant(const StringParam& str) noexcept;

		Variant(const Time& value) noexcept;

		template <class T>
		Variant(T* ptr) noexcept
		{
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
			_constructorRef(&ref, VariantType::Referable);
		}

		template <class T>
		Variant(Ref<T>&& ref) noexcept
		{
			_constructorMoveRef(&ref, VariantType::Referable);
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

		Variant(const Promise<Variant>& promise) noexcept;

		Variant(Promise<Variant>&& promise) noexcept;

		template <class T>
		Variant(const Promise<T>& promise) noexcept: Variant(Promise<Variant>::from(promise)) {}

		template <class T>
		Variant(const Nullable<T>& value) noexcept
		{
			if (value.isNotNull()) {
				new (this) Variant(value.value);
			} else {
				_type = VariantType::Null;
				_value = 0;
			}
		}

		template <class T>
		Variant(const Atomic<T>& t) noexcept: Variant(T(t)) {}

		Variant(const ObjectStorage& t) noexcept;
		Variant(ObjectStorage&& t) noexcept;

	public:
		Variant& operator=(const Variant& other) noexcept;

		Variant& operator=(Variant&& other) noexcept;

		Variant& operator=(const Atomic<Variant>& other) noexcept;

		Variant& operator=(const Json& other) noexcept;

		Variant& operator=(Json&& other) noexcept;

		Variant& operator=(sl_null_t) noexcept;

		template <class T>
		Variant& operator=(T&& value) noexcept
		{
			set(Forward<T>(value));
			return *this;
		}

		Variant operator[](sl_uint64 index) const noexcept;

		Variant operator[](const StringParam& key) const noexcept;

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
		sl_uint8 getType() const noexcept
		{
			return _type;
		}

		void setUndefined() noexcept;

		sl_bool isUndefined() const noexcept
		{
			return _type == VariantType::Null && !_value;
		}

		sl_bool isNotUndefined() const noexcept
		{
			return _type != VariantType::Null || _value != 0;
		}

		void setNull() noexcept;

		sl_bool isNull() const noexcept
		{
			return _type == VariantType::Null;
		}

		sl_bool isNotNull() const noexcept
		{
			return _type != VariantType::Null;
		}

		sl_bool isInt32() const noexcept;

		sl_bool getInt32(sl_int32* _out) const noexcept;

		sl_int32 getInt32(sl_int32 def = 0) const noexcept;

		void setInt32(sl_int32 value) noexcept;

		sl_bool isUint32() const noexcept;

		sl_bool getUint32(sl_uint32* _out) const noexcept;

		sl_uint32 getUint32(sl_uint32 def = 0) const noexcept;

		void setUint32(sl_uint32 value) noexcept;

		sl_bool isInt64() const noexcept;

		sl_bool getInt64(sl_int64* _out) const noexcept;

		sl_int64 getInt64(sl_int64 def = 0) const noexcept;

		void setInt64(sl_int64 value) noexcept;

		sl_bool isUint64() const noexcept;

		sl_bool getUint64(sl_uint64* _out) const noexcept;

		sl_uint64 getUint64(sl_uint64 def = 0) const noexcept;

		void setUint64(sl_uint64 value) noexcept;

		sl_bool isInteger() const noexcept;

		sl_bool isSignedInteger() const noexcept;

		sl_bool isUnsignedInteger() const noexcept;

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

		sl_bool isNumber() const noexcept;


		sl_bool isBoolean() const noexcept;

		sl_bool isTrue() const noexcept;

		sl_bool isFalse() const noexcept;

		sl_bool getBoolean(sl_bool def = sl_false) const noexcept;

		void setBoolean(sl_bool value) noexcept;


		sl_bool isString() const noexcept;

		sl_bool isString8() const noexcept;

		sl_bool isString16() const noexcept;

		sl_bool isSz8() const noexcept;

		sl_bool isSz16() const noexcept;

		String getString(const String& def) const noexcept;

		String getString() const noexcept;

		String16 getString16(const String16& def) const noexcept;

		String16 getString16() const noexcept;

		const sl_char8* getSz8(const sl_char8* def = sl_null) const noexcept;

		const sl_char16* getSz16(const sl_char16* def = sl_null) const noexcept;

		StringParam getStringParam(const StringParam& def) const noexcept;

		StringParam getStringParam() const noexcept;

		void setString(const String& value) noexcept;

		void setString(String&& value) noexcept;

		void setString(const String16& value) noexcept;

		void setString(String16&& value) noexcept;

		void setString(const AtomicString& value) noexcept;

		void setString(const AtomicString16& value) noexcept;

		void setString(const StringView& value) noexcept;

		void setString(const StringView16& value) noexcept;

		void setString(const sl_char8* sz8) noexcept;

		void setString(const sl_char16* sz16) noexcept;

#ifdef SLIB_SUPPORT_STD_TYPES
		std::string getStdString() const noexcept;

		std::u16string getStdString16() const noexcept;

		void setString(const std::string& value) noexcept;

		void setString(const std::u16string& value) noexcept;
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

		Ref<Referable> getRef() const noexcept;

		template <class T>
		Ref<T> getRef(const Ref<T>& def) const noexcept
		{
			return CastRef<T>(getRef(), def);
		}

		template <class T>
		void setRef(T&& t) noexcept
		{
			_setRef(Forward<T>(t), VariantType::Referable);
		}

		template <class T>
		static Variant fromRef(T&& t) noexcept
		{
			Variant ret;
			ret._initRef(Forward<T>(t), VariantType::Referable);
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

		sl_uint64 getElementsCount() const;

		Variant getElement_NoLock(sl_uint64 index) const;

		Variant getElement(sl_uint64 index) const;

		sl_bool setElement_NoLock(sl_uint64 index, const Variant& value) const;

		sl_bool setElement(sl_uint64 index, const Variant& value) const;

		sl_bool addElement_NoLock(const Variant& value) const;
		sl_bool addElement_NoLock(const Variant& value);

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

		Variant getItem_NoLock(const StringParam& key) const;

		Variant getItem(const StringParam& key) const;

		sl_bool putItem_NoLock(const StringParam& key, const Variant& value) const;
		sl_bool putItem_NoLock(const StringParam& key, const Variant& value);

		sl_bool putItem(const StringParam& key, const Variant& value) const;
		sl_bool putItem(const StringParam& key, const Variant& value);

		sl_bool removeItem_NoLock(const StringParam& key) const;

		sl_bool removeItem(const StringParam& key) const;

		PropertyIterator getItemIterator() const;


		sl_bool isMemory() const noexcept;

		Memory getMemory() const noexcept;

		void setMemory(const Memory& mem) noexcept;

		void setMemory(Memory&& mem) noexcept;


		sl_bool isVariantPromise() const noexcept;

		Promise<Variant> getVariantPromise() const noexcept;

		void setVariantPromise(const Promise<Variant>& promise) noexcept;

		void setVariantPromise(Promise<Variant>&& promise) noexcept;

		template <class T>
		void setVariantPromise(const Promise<T>& promise) noexcept
		{
			setVariantPromise(Promise<Variant>::from(promise));
		}


		void merge(const Variant& other);


		String toString() const;

		sl_bool toJsonString(StringBuffer& buf) const;
	
		String toJsonString() const;

		Memory serialize() const;

		sl_bool serialize(MemoryBuffer* buf) const;

		template <class OUTPUT>
		sl_bool serialize(OUTPUT* output) const;

		sl_size deserialize(const void* data, sl_size size);

		sl_size deserialize(const MemoryData& data);

		sl_size deserialize(MemoryData&& data);

		sl_size deserialize(const Memory& mem);

		sl_size deserialize(Memory&& mem);

		template <class INPUT>
		sl_bool deserialize(INPUT* input);

		
		sl_compare_result compare(const Variant& other) const noexcept;
		
		sl_bool equals(const Variant& other) const noexcept;
		
		sl_size getHashCode() const noexcept;

	public:
		template <class T>
		void set(T&& t)
		{
			_free(_type, _value);
			new (this) Variant(Forward<T>(t));
		}

		void get(Variant& _out) const noexcept;
		void get(Atomic<Variant>& _out) const noexcept;
		void get(Json& _out) const noexcept;

		void get(signed char& _out) const noexcept;
		void get(signed char& _out, signed char def) const noexcept;
		void get(unsigned char& _out) const noexcept;
		void get(unsigned char& _out, unsigned char def) const noexcept;
		void get(short& _out) const noexcept;
		void get(short& _out, short def) const noexcept;
		void get(unsigned short& _out) const noexcept;
		void get(unsigned short& _out, unsigned short def) const noexcept;
		void get(int& _out) const noexcept;
		void get(int& _out, int def) const noexcept;
		void get(unsigned int& _out) const noexcept;
		void get(unsigned int& _out, unsigned int def) const noexcept;
		void get(long& _out) const noexcept;
		void get(long& _out, long def) const noexcept;
		void get(unsigned long& _out) const noexcept;
		void get(unsigned long& _out, unsigned long def) const noexcept;
		void get(sl_int64& _out) const noexcept;
		void get(sl_int64& _out, sl_int64 def) const noexcept;
		void get(sl_uint64& _out) const noexcept;
		void get(sl_uint64& _out, sl_uint64 def) const noexcept;
		void get(float& _out) const noexcept;
		void get(float& _out, float def) const noexcept;
		void get(double& _out) const noexcept;
		void get(double& _out, double def) const noexcept;
		void get(bool& _out) const noexcept;
		void get(bool& _out, bool def) const noexcept;

		void get(String& _out) const noexcept;
		void get(String& _out, const String& def) const noexcept;
		void get(AtomicString& _out) const noexcept;
		void get(AtomicString& _out, const String& def) const noexcept;
		void get(String16& _out) const noexcept;
		void get(String16& _out, const String16& def) const noexcept;
		void get(AtomicString16& _out) const noexcept;
		void get(AtomicString16& _out, const String16& def) const noexcept;
#ifdef SLIB_SUPPORT_STD_TYPES
		void get(std::string& _out) const noexcept;
		void get(std::u16string& _out) const noexcept;
#endif

		void get(Time& _out) const noexcept;
		void get(Time& _out, const Time& def) const noexcept;

		template <class T>
		void get(Ref<T>& _out) const noexcept
		{
			_out = CastRef<T>(getRef());
		}

		template <class T>
		void get(WeakRef<T>& _out) const noexcept
		{
			_out = CastRef<T>(getRef());
		}

		void get(VariantList& _out) const noexcept;
		void get(VariantMap& _out) const noexcept;
		void get(JsonList& _out) const noexcept;
		void get(JsonMap& _out) const noexcept;

		template <class T>
		void get(Array<T>& _out) const noexcept
		{
			_out = priv::variant::CreateListFromVariant< Array<T> >(*this);
		}

		template <class T>
		void get(List<T>& _out) const noexcept
		{
			_out = priv::variant::CreateListFromVariant< List<T> >(*this);
		}

		template <class KT, class VT, class KEY_COMPARE>
		void get(const Map<KT, VT, KEY_COMPARE>& _out) const noexcept
		{
			_out.setNull();
			priv::variant::BuildMapFromVariant(_out, *this);
		}

		template <class KT, class VT, class HASH, class KEY_COMPARE>
		void get(const HashMap<KT, VT, HASH, KEY_COMPARE>& _out)
		{
			_out.setNull();
			priv::variant::BuildMapFromVariant(_out, *this);
		}

		void get(Memory& _out) const noexcept;

		void get(Promise<Variant>& _out) const noexcept;

		template <class T>
		void get(Nullable<T>& _out) const noexcept
		{
			if (isUndefined()) {
				_out->setNull();
			} else {
				_out.flagNull = sl_false;
				get(_out.value);
			}
		}

		template <class T>
		void get(Atomic<T>& _out) const noexcept
		{
			T t;
			get(t);
			_out = Move(t);
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

	SLIB_DECLARE_DEFAULT_COMPARE_OPERATORS(Variant)
	
	class VariantEx : public Variant
	{
	public:
		VariantEx() noexcept {}

		~VariantEx() noexcept {}

		template <class T>
		VariantEx(T&& t): Variant(priv::variant::VariantExHelper<T>::forward(t)) {}

	};

	template <class... ARGS>
	SLIB_INLINE String String::format(const StringParam& strFormat, ARGS&&... args) noexcept
	{
		VariantEx params[] = {Forward<ARGS>(args)...};
		return formatBy(strFormat, params, sizeof...(args));
	}

	template <class... ARGS>
	SLIB_INLINE String16 String16::format(const StringParam& strFormat, ARGS&&... args) noexcept
	{
		VariantEx params[] = {Forward<ARGS>(args)...};
		return formatBy(strFormat, params, sizeof...(args));
	}

	template <class... ARGS>
	SLIB_INLINE String String::format(const Locale& locale, const StringParam& strFormat, ARGS&&... args) noexcept
	{
		VariantEx params[] = {Forward<ARGS>(args)...};
		return formatBy(locale, strFormat, params, sizeof...(args));
	}

	template <class... ARGS>
	SLIB_INLINE String16 String16::format(const Locale& locale, const StringParam& strFormat, ARGS&&... args) noexcept
	{
		VariantEx params[] = {Forward<ARGS>(args)...};
		return formatBy(locale, strFormat, params, sizeof...(args));
	}

	template <class... ARGS>
	SLIB_INLINE String String::arg(ARGS&&... args) const noexcept
	{
		VariantEx params[] = {Forward<ARGS>(args)...};
		return argBy(params, sizeof...(args));
	}
	
	template <class... ARGS>
	SLIB_INLINE String16 String16::arg(ARGS&&... args) const noexcept
	{
		VariantEx params[] = {Forward<ARGS>(args)...};
		return argBy(params, sizeof...(args));
	}
	
	template <class... ARGS>
	SLIB_INLINE String Atomic<String>::arg(ARGS&&... args) const noexcept
	{
		VariantEx params[] = {Forward<ARGS>(args)...};
		return argBy(params, sizeof...(args));
	}
	
	template <class... ARGS>
	SLIB_INLINE String16 Atomic<String16>::arg(ARGS&&... args) const noexcept
	{
		VariantEx params[] = {Forward<ARGS>(args)...};
		return argBy(params, sizeof...(args));
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

	namespace priv
	{
		namespace variant
		{

			template <class LIST>
			LIST CreateListFromCollection(Collection* collection)
			{
				if (collection) {
					sl_size n = (sl_size)(collection->getElementsCount());
					if (n) {
						LIST ret = LIST::create(n);
						if (ret.isNotNull()) {
							auto data = ret.getData();
							for (sl_size i = 0; i < n; i++) {
								collection->getElement(i).get(data[i]);
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
									src[i].get(dst[i]);
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
						iterator.getValue().get(v);
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
							node->value.get(v);
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
