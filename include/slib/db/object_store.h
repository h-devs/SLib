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

#ifndef CHECKHEADER_SLIB_DB_OBJECT_STORE
#define CHECKHEADER_SLIB_DB_OBJECT_STORE

#include "definition.h"

#include "../core/variant.h"

namespace slib
{

	class ObjectStoreDictionary;
	class ObjectStoreManager;
	class KeyValueStore;

	class SLIB_EXPORT ObjectStoreParam
	{
	public:
		StringParam path;
		Ref<KeyValueStore> store;

	public:
		ObjectStoreParam();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(ObjectStoreParam)

	};

	class SLIB_EXPORT ObjectStore
	{
	public:
		ObjectStore() noexcept;

		ObjectStore(sl_null_t) noexcept;

		ObjectStore(ObjectStoreDictionary* dictionary) noexcept;
		ObjectStore(const Ref<ObjectStoreDictionary>& dictionary) noexcept;
		ObjectStore(Ref<ObjectStoreDictionary>&& dictionary) noexcept;
		template <class T>
		ObjectStore(T* dictionary) noexcept: ObjectStore((ObjectStoreDictionary*)dictionary) {}

		template <class T>
		ObjectStore(T&& _value) noexcept: value(_value) {}

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(ObjectStore)

	public:
		Ref<ObjectStoreManager> getManager() const;

		sl_bool isDictionary() const noexcept;

		Ref<ObjectStoreDictionary> getDictionary() const noexcept;

		ObjectStore createDictionary(const StringView& key) const;

		ObjectStore getDictionary(const StringView& key) const;

		sl_bool removeDictionary(const StringView& key) const;

		Iterator<String, ObjectStore> getDictionaryIterator() const;

		Variant getItem(const StringView& key) const;

		sl_bool putItem(const StringView& key, const Variant& value) const;

		sl_bool removeItem(const StringView& key) const;

		PropertyIterator getItemIterator() const;

	public:
		static const ObjectStore& undefined() noexcept
		{
			return *(reinterpret_cast<ObjectStore const*>(&(priv::variant::g_undefined)));
		}

		static const ObjectStore& null() noexcept
		{
			return *(reinterpret_cast<ObjectStore const*>(&(priv::variant::g_null)));
		}

		sl_bool isUndefined() const noexcept;

		sl_bool isNotUndefined() const noexcept;

		sl_bool isNull() const noexcept;

		sl_bool isNotNull() const noexcept;


		sl_bool isInt32() const noexcept;

		sl_bool getInt32(sl_int32* _out) const noexcept;

		sl_int32 getInt32(sl_int32 def = 0) const noexcept;

		sl_bool isUint32() const noexcept;

		sl_bool getUint32(sl_uint32* _out) const noexcept;

		sl_uint32 getUint32(sl_uint32 def = 0) const noexcept;

		sl_bool isInt64() const noexcept;

		sl_bool getInt64(sl_int64* _out) const noexcept;

		sl_int64 getInt64(sl_int64 def = 0) const noexcept;

		sl_bool isUint64() const noexcept;

		sl_bool getUint64(sl_uint64* _out) const noexcept;

		sl_uint64 getUint64(sl_uint64 def = 0) const noexcept;

		sl_bool isIntegerType() const noexcept;

		sl_bool isSignedIntegerType() const noexcept;

		sl_bool isUnsignedIntegerType() const noexcept;

		sl_bool isFloat() const noexcept;

		sl_bool getFloat(float* _out) const noexcept;

		float getFloat(float def = 0) const noexcept;

		sl_bool isDouble() const noexcept;

		sl_bool getDouble(double* _out) const noexcept;

		double getDouble(double def = 0) const noexcept;

		sl_bool isNumberType() const noexcept;


		sl_bool isBoolean() const noexcept;

		sl_bool isTrue() const noexcept;

		sl_bool isFalse() const noexcept;

		sl_bool getBoolean(sl_bool def = sl_false) const noexcept;


		sl_bool isStringType() const noexcept;

		String getString(const String& def) const noexcept;

		String getString() const noexcept;

		String16 getString16(const String16& def) const noexcept;

		String16 getString16() const noexcept;


		sl_bool isTime() const noexcept;

		Time getTime(const Time& def) const noexcept;

		Time getTime() const noexcept;


		sl_bool isCollection() const noexcept;

		Ref<Collection> getCollection() const noexcept;

		sl_bool isVariantList() const noexcept;

		VariantList getVariantList() const noexcept;

		sl_bool isJsonList() const noexcept;

		JsonList getJsonList() const noexcept;


		sl_bool isObject() const noexcept;

		Ref<Object> getObject() const noexcept;

		sl_bool isVariantMap() const noexcept;

		VariantMap getVariantMap() const noexcept;

		sl_bool isJsonMap() const noexcept;

		JsonMap getJsonMap() const noexcept;


		sl_bool isMemory() const noexcept;

		Memory getMemory() const noexcept;

	public:
		template <class T>
		ObjectStore& operator=(T&& t)
		{
			value.~Variant();
			new (&value) Variant(Forward<T>(t));
			return *this;
		}

		ObjectStore operator[](const String& name) noexcept;
		Variant operator[](sl_size index) noexcept;

	public:
		static ObjectStore open(const ObjectStoreParam& param);
		
		static ObjectStore open(const StringParam& path);

	public:
		Variant value;

	};

	class SLIB_EXPORT ObjectStoreDictionary : public Referable
	{
		SLIB_DECLARE_OBJECT

	public:
		ObjectStoreDictionary();

		~ObjectStoreDictionary();

	public:
		virtual Ref<ObjectStoreManager> getManager() = 0;

		virtual Ref<ObjectStoreDictionary> createDictionary(const StringView& key) = 0;

		virtual Ref<ObjectStoreDictionary> getDictionary(const StringView& key) = 0;

		virtual sl_bool removeDictionary(const StringView& key) = 0;

		virtual Iterator<String, ObjectStore> getDictionaryIterator() = 0;

		virtual Variant getItem(const StringView& key) = 0;

		virtual sl_bool putItem(const StringView& key, const Variant& value) = 0;

		virtual sl_bool removeItem(const StringView& key) = 0;

		virtual PropertyIterator getItemIterator() = 0;

	};

	class ObjectStoreManager : public Object
	{
		SLIB_DECLARE_OBJECT

	public:
		ObjectStoreManager();

		~ObjectStoreManager();

	public:
		virtual Ref<KeyValueStore> getStore() = 0;
		
		virtual Ref<ObjectStoreDictionary> getRootDictionary() = 0;

	};

}

#endif
