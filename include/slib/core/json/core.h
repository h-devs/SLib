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

#ifndef CHECKHEADER_SLIB_CORE_JSON_CORE
#define CHECKHEADER_SLIB_CORE_JSON_CORE

#include "../variant.h"
#include "../array_collection.h"
#include "../list_collection.h"
#include "../map_object.h"

#ifdef SLIB_SUPPORT_STD_TYPES
#include <initializer_list>
#include <string>
#endif

namespace slib
{

	class JsonItem;

	class SLIB_EXPORT JsonParseParam
	{
	public:
		// in
		sl_bool flagSupportComments;
		// in
		sl_bool flagLogError;

		// out
		sl_bool flagError;
		// out
		sl_size errorPosition;
		// out
		sl_size errorLine;
		// out
		sl_size errorColumn;
		// out
		String errorMessage;

	public:
		JsonParseParam();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(JsonParseParam)

	public:
		String getErrorText();

	};
	
	class SLIB_EXPORT Json : public Variant
	{
	public:
		Json();
		
		Json(const Json& other);
		
		Json(Json&& other);

		Json(const Atomic<Json>& other);

		Json(const Variant& other);

		Json(Variant&& other);

		Json(const Atomic<Variant>& other);
		
		~Json();
		
	public:
		Json(sl_null_t);
		
		Json(signed char value);
		
		Json(unsigned char value);
		
		Json(short value);
		
		Json(unsigned short value);
		
		Json(int value);
		
		Json(unsigned int value);
		
		Json(long value);
		
		Json(unsigned long value);
		
		Json(sl_int64 value);
		
		Json(sl_uint64 value);
		
		Json(float value);
		
		Json(double value);
		
		Json(sl_bool value);
		
		Json(const String& value);

		Json(String&& value);

		Json(const String16& value);

		Json(String16&& value);

		Json(const String32& value);

		Json(String32&& value);

		Json(const StringView& value);

		Json(const StringView16& value);

		Json(const StringView32& value);

		Json(const sl_char8* sz8);
		
		Json(const sl_char16* sz16);

		Json(const sl_char32* sz32);

		Json(const StringParam& param);
		
#ifdef SLIB_SUPPORT_STD_TYPES
		Json(const std::string& value);

		Json(const std::u16string& value);

		Json(const std::u32string& value);
#endif
		
		Json(const Time& value);

		Json(const Memory& value);

		Json(Memory&& mem);

		Json(const ObjectId& value);

		Json(const JsonList& list);

		Json(JsonList&& list);

		Json(const JsonMap& map);

		Json(JsonMap&& map);

		Json(const VariantList& list);

		Json(VariantList&& list);

		Json(const VariantMap& map);

		Json(VariantMap&& map);

#ifdef SLIB_SUPPORT_STD_TYPES
		Json(const std::initializer_list<JsonItem>& pairs);
#endif

		template <class T>
		Json(const Atomic<T>& t): Json(T(t)) {}

		template <class T>
		Json(const T& value)
		{
			ToJson(*this, value);
		}
		
		Json(const ObjectStore& t) noexcept;
		Json(ObjectStore&& t) noexcept;

		template <class T>
		Json(T&& arg, sl_uint8 tag): Json(Forward<T>(arg))
		{
			_tag = tag;
		}

	public:
		static const Json& undefined()
		{
			return *(reinterpret_cast<Json const*>(&(priv::variant::g_undefined)));
		}

		static const Json& null()
		{
			return *(reinterpret_cast<Json const*>(&(priv::variant::g_null)));
		}
		
		static Json createList();
		
		static Json createMap();
		
	public:
		Json& operator=(const Json& json);
		
		Json& operator=(Json&& json);

		Json& operator=(const Atomic<Json>& json);

		Json& operator=(const Variant& variant);

		Json& operator=(Variant&& variant);

		Json& operator=(const Atomic<Variant>& variant);
		
		Json& operator=(sl_null_t);
		
#ifdef SLIB_SUPPORT_STD_TYPES
		Json& operator=(const std::initializer_list<JsonItem>& pairs);
#endif

		template <class T>
		Json& operator=(T&& value) noexcept
		{
			set(Forward<T>(value));
			return *this;
		}

		Json operator[](sl_size index) const;

		Json operator[](const String& key) const;

	public:
		template <class T>
		void get(T& value) const
		{
			FromJson(*this, value);
		}

		template <class T>
		void get(T& value, const T& defaultValue) const
		{
			FromJson(*this, value, defaultValue);
		}

		template <class T>
		void set(T&& t)
		{
			_free(_type, _value);
			new (this) Json(Forward<T>(t));
		}

		Json getElement_NoLock(sl_size index) const;

		template <class T>
		void getElement_NoLock(sl_size index, T& _out) const
		{
			FromJson(getElement_NoLock(index), _out);
		}

		Json getElement(sl_size index) const;

		template <class T>
		void getElement(sl_size index, T& _out) const
		{
			FromJson(getElement(index), _out);
		}

		sl_bool setElement_NoLock(sl_uint64 index, const Json& value);

		sl_bool setElement(sl_uint64 index, const Json& value);

		sl_bool addElement_NoLock(const Json& value);

		sl_bool addElement(const Json& value);

		Json getItem_NoLock(const String& key) const;

		template <class T>
		void getItem_NoLock(const String& key, T& _out) const
		{
			FromJson(getItem_NoLock(key), _out);
		}

		Json getItem(const String& key) const;
		
		template <class T>
		void getItem(const String& key, T& _out) const
		{
			FromJson(getItem(key), _out);
		}

		sl_bool putItem_NoLock(const String& key, const Json& value);

		sl_bool putItem(const String& key, const Json& value);

	public:
		static Json parseJson(const sl_char8* sz, sl_size len, JsonParseParam& param);

		static Json parseJson(const sl_char8* sz, sl_size len);

		static Json parseJson(const sl_char16* sz, sl_size len, JsonParseParam& param);

		static Json parseJson(const sl_char16* sz, sl_size len);

		static Json parseJson(const sl_char32* sz, sl_size len, JsonParseParam& param);

		static Json parseJson(const sl_char32* sz, sl_size len);

		static Json parseJson(const StringParam& str, JsonParseParam& param);

		static Json parseJson(const StringParam& str);

		static Json parseJsonFromTextFile(const StringParam& filePath, JsonParseParam& param);

		static Json parseJsonFromTextFile(const StringParam& filePath);

	protected:
		String toString() const;
		
	};

	class SLIB_EXPORT JsonItem : public Pair<String, Json>
	{
	public:
		JsonItem() noexcept {}

		JsonItem(const String& key, const Json& value) noexcept : Pair(key, value) {}

		JsonItem(String&& key, const Json& value) noexcept : Pair(Move(key), value) {}

		JsonItem(const String& key, Json&& value) noexcept : Pair(key, Move(value)) {}

		JsonItem(String&& key, Json&& value) noexcept : Pair(Move(key), Move(value)) {}

	};

}

#endif
