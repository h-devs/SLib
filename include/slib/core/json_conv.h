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

#ifndef CHECKHEADER_SLIB_CORE_JSON_CONV
#define CHECKHEADER_SLIB_CORE_JSON_CONV

namespace slib
{
	
	namespace priv
	{
		namespace json
		{

			template <class T, sl_bool isClass = __is_class(T), sl_bool isEnum = __is_enum(T)>
			class DoJsonHelper
			{
			};

			template <class T>
			class DoJsonHelper<T, sl_true, sl_false>
			{
			public:
				static void fromJson(const Json& json, T& _out)
				{
					_out.fromJson(json);
				}

				static void toJson(Json& json, const T& _in)
				{
					json = _in.toJson();
				}
			};

			template <class T>
			class DoJsonHelper<T, sl_false, sl_true>
			{
			public:
				static void fromJson(const Json& json, T& _out)
				{
					_out = (T)(json.getInt64((sl_int64)_out));
				}

				static void toJson(Json& json, const T& _in)
				{
					json.setInt64((sl_int64)_in);
				}
			};

			template <class LIST>
			class ListHelper
			{
			public:
				static void clear(LIST& list)
				{
					list.setNull();
				}

				static sl_bool create(LIST& list, sl_size n)
				{
					list = LIST::create(n);
					return list.isNotNull();
				}

				static typename LIST::ELEMENT_TYPE* getData(LIST& list)
				{
					return list.getData();
				}

			};

			template <class MAP>
			class MapHelper
			{
			public:
				static void clear(MAP& map)
				{
					map.setNull();
				}

				template <class... ARGS>
				static void add(MAP& map, ARGS&&... args)
				{
					map.add_NoLock(Forward<ARGS>(args)...);
				}

			};

#ifdef SLIB_SUPPORT_STD_TYPES
			template <class T, class ALLOC>
			class ListHelper< std::vector<T, ALLOC> >
			{
			public:
				static void clear(std::vector<T, ALLOC>& list)
				{
					list.clear();
				}
				
				static sl_bool create(std::vector<T, ALLOC>& list, sl_size n)
				{
					list.resize(n);
					return list.size() == n;
				}

				static T* getData(std::vector<T, ALLOC>& list)
				{
					return list.data();
				}

			};


			template <class KT, class... TYPES>
			class MapHelper< std::map<KT, TYPES...> >
			{
			public:
				static void clear(std::map<KT, TYPES...>& map)
				{
					map.clear();
				}

				template <class... ARGS>
				static void add(std::map<KT, TYPES...>& map, ARGS&&... args)
				{
					map.emplace(Forward<ARGS>(args)...);
				}

			};

			template <class KT, class... TYPES>
			class MapHelper< std::unordered_map<KT, TYPES...> >
			{
			public:
				static void clear(std::unordered_map<KT, TYPES...>& map)
				{
					map.clear();
				}

				template <class... ARGS>
				static void add(std::unordered_map<KT, TYPES...>& map, ARGS&&... args)
				{
					map.emplace(Forward<ARGS>(args)...);
				}

			};
#endif

			template <class LIST>
			static void GetListFromJson(LIST& _out, const Json& json)
			{
				if (json.isUndefined()) {
					return;
				}
				if (json.getType() == VariantType::List) {
					JsonList list = json.getJsonList();
					if (list.isNotNull()) {
						ListLocker<Json> src(list);
						if (src.count) {
							if (ListHelper<LIST>::create(_out, src.count)) {
								auto dst = ListHelper<LIST>::getData(_out);
								for (sl_size i = 0; i < src.count; i++) {
									FromJson(src[i], dst[i]);
								}
							}
							return;
						}
					}
				} else {
					Ref<Collection> src = json.getCollection();
					if (src.isNotNull()) {
						sl_size n = (sl_size)(src->getElementsCount());
						if (n) {
							if (ListHelper<LIST>::create(_out, n)) {
								auto dst = ListHelper<LIST>::getData(_out);
								for (sl_size i = 0; i < n; i++) {
									Json v(src->getElement(i));
									FromJson(v, dst[i]);
								}
							}
							return;
						}
					}
				}
				ListHelper<LIST>::clear(_out);
			}

			template <class MAP>
			static void GetMapFromJson(MAP& _out, const Json& json)
			{
				if (json.isUndefined()) {
					return;
				}
				MapHelper<MAP>::clear(_out);
				if (json.getType() == VariantType::Map) {
					JsonMap src = json.getJsonMap();
					if (src.isNotNull()) {
						MutexLocker lock(src.getLocker());
						auto node = src.getFirstNode();
						while (node) {
							typename MAP::VALUE_TYPE v;
							FromJson(node->value, v);
							MapHelper<MAP>::add(_out, Cast<String, typename MAP::KEY_TYPE>()(node->key), Move(v));
							node = node->getNext();
						}
					}
				} else {
					Ref<Object> src = json.getObject();
					if (src.isNotNull()) {
						src->enumerateProperties([&_out](const StringParam& name, const Variant& value) {
							typename MAP::VALUE_TYPE v;
							FromJson(*(static_cast<const Json*>(&value)), v);
							MapHelper<MAP>::add(_out, Cast<StringParam, typename MAP::KEY_TYPE>()(name), Move(v));
							return sl_true;
						});
					}
				}
			}

			template <class MAP>
			static void ToJsonMap(Json& json, const MAP& _in)
			{
				if (_in.isNotNull()) {
					MutexLocker locker(_in.getLocker());
					JsonMap map;
					auto node = _in.getFirstNode();
					while (node) {
						map.put_NoLock(Cast<typename MAP::KEY_TYPE, String>()(node->key), Json(node->value));
						node = node->getNext();
					}
					json = Move(map);
				} else {
					json.setNull();
				}
			}

		}
	}

	void FromJson(const Json& json, Json& _out);
	void ToJson(Json& json, const Json& _in);

	void FromJson(const Json& json, Variant& _out);
	void ToJson(Json& json, const Variant& _in);
	
	void FromJson(const Json& json, signed char& _out);
	void FromJson(const Json& json, signed char& _out, signed char def);
	void ToJson(Json& json, signed char _in);
	
	void FromJson(const Json& json, unsigned char& _out);
	void FromJson(const Json& json, unsigned char& _out, unsigned char def);
	void ToJson(Json& json, unsigned char _in);
	
	void FromJson(const Json& json, short& _out);
	void FromJson(const Json& json, short& _out, short def);
	void ToJson(Json& json, short _in);
	
	void FromJson(const Json& json, unsigned short& _out);
	void FromJson(const Json& json, unsigned short& _out, unsigned short def);
	void ToJson(Json& json, unsigned short _in);
	
	void FromJson(const Json& json, int& _out);
	void FromJson(const Json& json, int& _out, int def);
	void ToJson(Json& json, int _in);
	
	void FromJson(const Json& json, unsigned int& _out);
	void FromJson(const Json& json, unsigned int& _out, unsigned int def);
	void ToJson(Json& json, unsigned int _in);
	
	void FromJson(const Json& json, long& _out);
	void FromJson(const Json& json, long& _out, long def);
	void ToJson(Json& json, long _in);
	
	void FromJson(const Json& json, unsigned long& _out);
	void FromJson(const Json& json, unsigned long& _out, unsigned long def);
	void ToJson(Json& json, unsigned long _in);
	
	void FromJson(const Json& json, sl_int64& _out);
	void FromJson(const Json& json, sl_int64& _out, sl_int64 def);
	void ToJson(Json& json, sl_int64 _in);
	
	void FromJson(const Json& json, sl_uint64& _out);
	void FromJson(const Json& json, sl_uint64& _out, sl_uint64 def);
	void ToJson(Json& json, sl_uint64 _in);
	
	void FromJson(const Json& json, float& _out);
	void FromJson(const Json& json, float& _out, float def);
	void ToJson(Json& json, float _in);
	
	void FromJson(const Json& json, double& _out);
	void FromJson(const Json& json, double& _out, double def);
	void ToJson(Json& json, double _in);
	
	void FromJson(const Json& json, bool& _out);
	void FromJson(const Json& json, bool& _out, bool def);
	void ToJson(Json& json, bool _in);
	
	void FromJson(const Json& json, String& _out);
	void FromJson(const Json& json, String& _out, const String& def);
	void ToJson(Json& json, const String& _in);
	void ToJson(Json& json, const StringView& _in);

	void FromJson(const Json& json, String16& _out);
	void FromJson(const Json& json, String16& _out, const String16& def);
	void ToJson(Json& json, const String16& _in);
	void ToJson(Json& json, const StringView16& _in);

	void ToJson(Json& json, const sl_char8* sz8);
	void ToJson(Json& json, const sl_char16* sz16);
	
	void FromJson(const Json& json, StringParam& _out);
	void ToJson(Json& json, const StringParam& _in);

#ifdef SLIB_SUPPORT_STD_TYPES
	void FromJson(const Json& json, std::string& _out);
	void ToJson(Json& json, const std::string& _in);
	
	void FromJson(const Json& json, std::u16string& _out);
	void ToJson(Json& json, const std::u16string& _in);
#endif
	
	void FromJson(const Json& json, Time& _out);
	void FromJson(const Json& json, Time& _out, const Time& def);
	void ToJson(Json& json, const Time& _in);

	void FromJson(const Json& json, Memory& _out);
	void ToJson(Json& json, const Memory& _in);

	template <class T>
	static void FromJson(const Json& json, SharedPtr<T>& _out)
	{
		if (json.isUndefined()) {
			return;
		}
		if (json.isNull()) {
			_out.setNull();
		} else {
			_out = SharedPtr<T>::create();
			FromJson(json, *(_out.get()));
		}
	}

	template <class T>
	static void ToJson(Json& json, const SharedPtr<T>& _in)
	{
		if (_in.isNull()) {
			json.setNull();
		} else {
			ToJson(json, *(_in.get()));
		}
	}

	template <class T>
	static void FromJson(const Json& json, Ref<T>& _out)
	{
		if (json.isUndefined()) {
			return;
		}
		if (json.isNotNull()) {
			_out = new T(json);
		} else {
			_out.setNull();
		}
	}

	template <class T>
	static void ToJson(Json& json, const Ref<T>& _in)
	{
		if (_in.isNotNull()) {
			json = _in->toJson();
		} else {
			json.setNull();
		}
	}
	
	template <class T>
	static void ToJson(Json& json, const WeakRef<T>& _in)
	{
		ToJson(json, Ref<T>(_in));
	}

	void FromJson(const Json& json, VariantList& _out);
	void ToJson(Json& json, const VariantList& _in);
	
	void FromJson(const Json& json, VariantMap& _out);
	void ToJson(Json& json, const VariantMap& _in);

	void ToJson(Json& json, const List<VariantMap>& _in);

	void FromJson(const Json& json, JsonList& _out);
	void ToJson(Json& json, const JsonList& _in);
	
	void FromJson(const Json& json, JsonMap& _out);
	void ToJson(Json& json, const JsonMap& _in);

	void ToJson(Json& json, const List<JsonMap>& _in);

	template <class T>
	static void FromJson(const Json& json, Array<T>& _out)
	{
		priv::json::GetListFromJson(_out, json);
	}

	template <class T>
	static void ToJson(Json& json, const Array<T>& _in)
	{
		json = JsonList::create(_in);
	}

	template <class T>
	static void FromJson(const Json& json, List<T>& _out)
	{
		priv::json::GetListFromJson(_out, json);
	}

	template <class T>
	static void ToJson(Json& json, const List<T>& _in)
	{
		json = JsonList::createCopy(_in);
	}

	template <class T>
	static void ToJson(Json& json, const ListParam<T>& _in)
	{
		if (_in.isNotNull()) {
			ListLocker<T> src(_in);
			json = JsonList::create(src.data, src.count);
		} else {
			json.setNull();
		}
	}

	template <class KT, class VT, class KEY_COMPARE>
	static void FromJson(const Json& json, Map<KT, VT, KEY_COMPARE>& _out)
	{
		priv::json::GetMapFromJson(_out, json);
	}

	template <class KT, class VT, class KEY_COMPARE>
	static void ToJson(Json& json, const Map<KT, VT, KEY_COMPARE>& _in)
	{
		priv::json::ToJsonMap(json, _in);
	}
	
	template <class KT, class VT, class HASH, class KEY_COMPARE>
	static void FromJson(const Json& json, HashMap<KT, VT, HASH, KEY_COMPARE>& _out)
	{
		priv::json::GetMapFromJson(_out, json);
	}

	template <class KT, class VT, class HASH, class KEY_COMPARE>
	static void ToJson(Json& json, const HashMap<KT, VT, HASH, KEY_COMPARE>& _in)
	{
		priv::json::ToJsonMap(json, _in);
	}
	
#ifdef SLIB_SUPPORT_STD_TYPES
	template <class T, class ALLOC>
	static void FromJson(const Json& json, std::vector<T, ALLOC>& _out)
	{
		priv::json::GetListFromJson(_out, json);
	}

	template <class T, class ALLOC>
	static void ToJson(Json& json, const std::vector<T, ALLOC>& _in)
	{
		json = JsonList::create(_in.data(), _in.size());
	}
	
	template <class KT, class... TYPES>
	static void FromJson(const Json& json, std::map<KT, TYPES...>& _out)
	{
		priv::json::GetMapFromJson(_out, json);
	}

	template <class KT, class... TYPES>
	static void ToJson(Json& json, const std::map<KT, TYPES...>& _in)
	{
		JsonMap map;
		for (auto& item : _in) {
			map.put_NoLock(Cast<KT, String>()(item.first), Json(item.second));
		}
		json = Move(map);
	}
	
	template <class KT, class... TYPES>
	static void FromJson(const Json& json, std::unordered_map<KT, TYPES...>& _out)
	{
		priv::json::GetMapFromJson(_out, json);
	}

	template <class KT, class... TYPES>
	static void ToJson(Json& json, const std::unordered_map<KT, TYPES...>& _in)
	{
		JsonMap map;
		for	(auto& item : _in) {
			map.put_NoLock(Cast<KT, String>()(item.first), Json(item.second));
		}
		json = Move(map);
	}
#endif

	template <class T>
	static void FromJson(const Json& json, Nullable<T>& _out)
	{
		if (json.isUndefined()) {
			return;
		}
		if (json.isNull()) {
			_out.setNull();
		} else {
			_out.flagNull = sl_false;
			FromJson(json, _out.value);
		}
	}

	template <class T>
	static void ToJson(Json& json, const Nullable<T>& _in)
	{
		if (_in.isNull()) {
			json.setNull();
		} else {
			ToJson(json, _in.value);
		}
	}

	template <class T>
	static void FromJson(const Json& json, Atomic<T>& _out)
	{
		if (json.isUndefined()) {
			return;
		}
		T t;
		FromJson(json, t);
		_out = Move(t);
	}

	template <class T, class DEF>
	static void FromJson(const Json& json, Atomic<T>& _out, DEF&& def)
	{
		if (json.isUndefined()) {
			return;
		}
		T t;
		FromJson(json, t, Forward<DEF>(def));
		_out = Move(t);
	}

	template <class T>
	static void ToJson(Json& json, const Atomic<T>& _in)
	{
		ToJson(json, T(_in));
	}

	template <class T>
	static void FromJson(const Json& json, T& _out)
	{
		priv::json::DoJsonHelper<T>::fromJson(json, _out);
	}
	
	template <class T>
	static void ToJson(Json& json, const T& _in)
	{
		priv::json::DoJsonHelper<T>::toJson(json, _in);
	}

}

#endif
