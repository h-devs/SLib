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

#include "key_value_store.h"

#include "../core/string.h"

namespace slib
{

	class SLIB_EXPORT ObjectStorageParam
	{
	public:
		StringParam path;
		Ref<KeyValueStore> store;

	public:
		ObjectStorageParam();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(ObjectStorageParam)

	};

	class SLIB_EXPORT ObjectStorage : public Object
	{
		SLIB_DECLARE_OBJECT

	public:
		ObjectStorage();

		~ObjectStorage();

	public:
		KeyValueStore* getStore() noexcept;

		virtual Variant createObject(const StringParam& key) = 0;

		virtual Variant createObject(const Variant& parent, const StringParam& key) = 0;
		
		virtual Variant getItem(const StringParam& key) = 0;

		virtual sl_bool putItem(const StringParam& key, const Variant& value) = 0;

		virtual sl_bool removeItem(const StringParam& key) = 0;

		virtual Iterator<String, Variant> getItemIterator() = 0;

		Json toJson();

	public:
		Variant getProperty(const StringParam& name) override;

		sl_bool setProperty(const StringParam& name, const Variant& value) override;

		sl_bool clearProperty(const StringParam& name) override;

		PropertyIterator getPropertyIterator() override;

	public:
		static Ref<ObjectStorage> open(const ObjectStorageParam& param);
		
		static Ref<ObjectStorage> open(const StringParam& path);

	protected:
		Ref<KeyValueStore> m_store;

	};

}

#endif
