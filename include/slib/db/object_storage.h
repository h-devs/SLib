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

#ifndef CHECKHEADER_SLIB_DB_OBJECT_STORAGE
#define CHECKHEADER_SLIB_DB_OBJECT_STORAGE

#include "definition.h"

#include "../core/variant.h"

namespace slib
{

	class StorageObject;
	class ObjectStorageManager;
	class KeyValueStore;

	class SLIB_EXPORT ObjectStorageParam
	{
	public:
		StringParam path;
		Ref<KeyValueStore> store;

	public:
		ObjectStorageParam();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(ObjectStorageParam)

	};

	class SLIB_EXPORT ObjectStorage
	{
	public:
		ObjectStorage() noexcept;

		ObjectStorage(sl_null_t) noexcept;

		ObjectStorage(StorageObject* object) noexcept;
		ObjectStorage(const Ref<StorageObject>& object) noexcept;
		ObjectStorage(Ref<StorageObject>&& object) noexcept;
		template <class T>
		ObjectStorage(T* object) noexcept: ObjectStorage((StorageObject*)object) {}

		template <class T>
		ObjectStorage(T&& _value) noexcept: value(_value) {}

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(ObjectStorage)

	public:
		Ref<ObjectStorageManager> getManager() const;

		ObjectStorage createObject(const StringParam& key) const;

		sl_bool isStorageObject() const noexcept;

		Ref<StorageObject> getStorageObject() const noexcept;

		ObjectStorage getItem(const StringParam& key) const;

		sl_bool putItem(const StringParam& key, const Variant& value) const;

		sl_bool removeItem(const StringParam& key) const;

		Iterator<String, ObjectStorage> getItemIterator() const;

	public:
		static const ObjectStorage& undefined() noexcept
		{
			return *(reinterpret_cast<ObjectStorage const*>(&(priv::variant::g_undefined)));
		}

		static const ObjectStorage& null() noexcept
		{
			return *(reinterpret_cast<ObjectStorage const*>(&(priv::variant::g_null)));
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

		sl_bool isInteger() const noexcept;

		sl_bool isSignedInteger() const noexcept;

		sl_bool isUnsignedInteger() const noexcept;

		sl_bool isFloat() const noexcept;

		sl_bool getFloat(float* _out) const noexcept;

		float getFloat(float def = 0) const noexcept;

		sl_bool isDouble() const noexcept;

		sl_bool getDouble(double* _out) const noexcept;

		double getDouble(double def = 0) const noexcept;

		sl_bool isNumber() const noexcept;


		sl_bool isBoolean() const noexcept;

		sl_bool isTrue() const noexcept;

		sl_bool isFalse() const noexcept;

		sl_bool getBoolean(sl_bool def = sl_false) const noexcept;


		sl_bool isString() const noexcept;

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
		ObjectStorage& operator=(T&& t)
		{
			value.~Variant();
			new (&value) Variant(Forward<T>(t));
			return *this;
		}

		ObjectStorage operator[](const StringParam& name) noexcept;
		Variant operator[](sl_size index) noexcept;

	public:
		static ObjectStorage open(const ObjectStorageParam& param);
		
		static ObjectStorage open(const StringParam& path);

	public:
		Variant value;

	};

	class SLIB_EXPORT StorageObject : public Object
	{
		SLIB_DECLARE_OBJECT

	public:
		StorageObject();

		~StorageObject();

	public:
		virtual Ref<ObjectStorageManager> getManager() = 0;

		virtual Ref<StorageObject> createObject(const StringParam& key) = 0;

		virtual ObjectStorage getItem(const StringParam& key) = 0;

		virtual sl_bool putItem(const StringParam& key, const Variant& value) = 0;

		virtual sl_bool removeItem(const StringParam& key) = 0;

		virtual Iterator<String, ObjectStorage> getItemIterator() = 0;

	public:
		Variant getProperty(const StringParam& name) override;

		sl_bool setProperty(const StringParam& name, const Variant& value) override;

		sl_bool clearProperty(const StringParam& name) override;

		PropertyIterator getPropertyIterator() override;

	};

	class ObjectStorageManager : public Object
	{
		SLIB_DECLARE_OBJECT

	public:
		ObjectStorageManager();

		~ObjectStorageManager();

	public:
		virtual Ref<KeyValueStore> getStore() = 0;

		virtual Ref<StorageObject> createRootObject() = 0;

	};

}

#endif
