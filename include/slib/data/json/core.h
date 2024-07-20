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

#ifndef CHECKHEADER_SLIB_DATA_JSON_CORE
#define CHECKHEADER_SLIB_DATA_JSON_CORE

#include "../../core/variant.h"
#include "../../core/priv/array_collection.h"
#include "../../core/priv/list_collection.h"
#include "../../core/priv/map_object.h"

#ifdef SLIB_SUPPORT_STD_TYPES
#include <initializer_list>
#include <string>
#endif

namespace slib
{

	class JsonItem;

	// No thread-safe
	class SLIB_EXPORT Json : public Variant
	{
	public:
		Json();

		Json(const Json& other);

		Json(Json&& other);

		Json(const Atomic<Json>& other);

		Json(Atomic<Json>& other);

		Json(Atomic<Json>&& other);

		Json(const Variant& other);

		Json(Variant&& other);

		Json(const Atomic<Variant>& other);

		Json(Atomic<Variant>&& other);

		~Json();

	public:
		Json(sl_null_t);

		Json(signed char value);

		Json(unsigned char value);

		Json(char value);

		Json(short value);

		Json(unsigned short value);

		Json(int value);

		Json(unsigned int value);

		Json(long value);

		Json(unsigned long value);

		Json(sl_int64 value);

		Json(sl_uint64 value);

		Json(sl_char16 value);

		Json(sl_char32 value);

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

		Json(const StringData& value);

		Json(const StringData16& value);

		Json(const StringData32& value);

		Json(const StringCstr& value);

		Json(const StringCstr16& value);

		Json(const StringCstr32& value);

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

		Json(const List<JsonMap>& list);

		Json(const VariantList& list);

		Json(VariantList&& list);

		Json(const VariantMap& map);

		Json(VariantMap&& map);

		Json(const List<VariantMap>& list);

#ifdef SLIB_SUPPORT_STD_TYPES
		Json(const std::initializer_list<JsonItem>& pairs);
#endif

		template <class T>
		Json(const Array<T>& t);

		template <class T>
		Json(const List<T>& t);

		template <class T>
		Json(const ListParam<T>& t);

#ifdef SLIB_SUPPORT_STD_TYPES
		template <class T, class ALLOC>
		Json(const std::vector<T, ALLOC>& t);
#endif

		template <class KT, class VT, class KEY_COMPARE>
		Json(const Map<KT, VT, KEY_COMPARE>& t);

		template <class KT, class VT, class HASH, class KEY_COMPARE>
		Json(const HashMap<KT, VT, HASH, KEY_COMPARE>& t);

#ifdef SLIB_SUPPORT_STD_TYPES
		template <class KT, class... TYPES>
		Json(const std::map<KT, TYPES...>& t);

		template <class KT, class... TYPES>
		Json(const std::unordered_map<KT, TYPES...>& t);
#endif

		template <class T>
		Json(const Ref<T>& t);

		template <class T>
		Json(const WeakRef<T>& t);

		template <class T>
		Json(const Nullable<T>& t);

		template <class T>
		Json(const Atomic<T>& t);

		Json(const VariantWrapper& t) noexcept;

		Json(VariantWrapper&& t) noexcept;

		template <class T>
		Json(const T& value);

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
		void set(T&& t)
		{
			_free(_type, _value);
			new (this) Json(Forward<T>(t));
		}

		Json getElement(sl_size index) const;

		template <class T>
		void getElement(sl_size index, T& _out) const
		{
			FromJson(getElement(index), _out);
		}

		sl_bool setElement(sl_uint64 index, const Json& value);

		sl_bool addElement(const Json& value);

		Json getItem(const String& key) const;

		template <class T>
		void getItem(const String& key, T& _out) const
		{
			FromJson(getItem(key), _out);
		}

		sl_bool putItem(const String& key, const Json& value);

	public:
		class SLIB_EXPORT ParseParam
		{
		public:
			sl_bool flagSupportComments; // in
			sl_bool flagLogError; // in

			sl_bool flagError; // out
			sl_size errorPosition; // out
			sl_size errorLine; // out
			sl_size errorColumn; // out
			String errorMessage; // out

		public:
			ParseParam();
			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(ParseParam)

		public:
			String getErrorText();
		};

		static Json parse(const sl_char8* str, sl_size len, ParseParam& param);

		static Json parse(const sl_char8* str, sl_size len);

		static Json parse(const sl_char16* str, sl_size len, ParseParam& param);

		static Json parse(const sl_char16* str, sl_size len);

		static Json parse(const sl_char32* str, sl_size len, ParseParam& param);

		static Json parse(const sl_char32* str, sl_size len);

		static Json parse(const StringParam& str, ParseParam& param);

		static Json parse(const StringParam& str);

		static Json parse(const MemoryView& utf, ParseParam& param);

		static Json parse(const MemoryView& utf);

		static Json parseTextFile(const StringParam& filePath, ParseParam& param);

		static Json parseTextFile(const StringParam& filePath);

		template <class... ARGS>
		static Json getDeserialized(ARGS&&... args)
		{
			Json ret;
			if (ret.deserialize(Forward<ARGS>(args)...)) {
				return ret;
			}
			return Json();
		}

		Json duplicate() const;

	protected:
		String toString() const;

	};

	template <>
	Variant::Variant(const Atomic<Json>& t) noexcept;

	template <>
	Variant::Variant(Atomic<Json>&& t) noexcept;

	class SLIB_EXPORT JsonItem : public Pair<String, Json>
	{
	public:
		JsonItem() noexcept {}

		JsonItem(const String& key, const Json& value) noexcept: Pair(key, value) {}

		JsonItem(String&& key, const Json& value) noexcept: Pair(Move(key), value) {}

		JsonItem(const String& key, Json&& value) noexcept: Pair(key, Move(value)) {}

		JsonItem(String&& key, Json&& value) noexcept: Pair(Move(key), Move(value)) {}

	};

}

#endif
