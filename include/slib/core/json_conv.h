/*
 *   Copyright (c) 2008-2020 SLIBIO <https://github.com/SLIBIO>
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

#include "variant.h"
#include "cast.h"

#ifdef SLIB_SUPPORT_STD_TYPES
#include <initializer_list>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#endif

namespace slib
{
	
	namespace priv
	{
		namespace json
		{

			template <class T, sl_bool isClass = __is_class(T), sl_bool isEnum = __is_enum(T)>
			class Converter
			{
			};

			template <class T>
			class Converter<T, sl_true, sl_false>
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
			class Converter<T, sl_false, sl_true>
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

		}
	}

	class Json;
	
	typedef List<Json> JsonList;
	typedef AtomicList<Json> AtomicJsonList;
	typedef HashMap<String, Json> JsonMap;
	typedef AtomicHashMap<String, Json> AtomicJsonMap;
	typedef List< HashMap<String, Json> > JsonMapList;
	typedef AtomicList< HashMap<String, Json> > AtomicJsonMapList;
	typedef Pair<String, Json> JsonItem;

	class BigInt;
	
	void FromJson(const Json& json, Json& _out);
	void ToJson(Json& json, const Json& _in);
	
	void FromJson(const Json& json, Variant& _out);
	void ToJson(Json& json, const Variant& _in);
	
	void FromJson(const Json& json, AtomicVariant& _out);
	void ToJson(Json& json, const AtomicVariant& _in);
	
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

	void FromJson(const Json& json, AtomicString& _out);
	void FromJson(const Json& json, AtomicString& _out, const String& def);
	void ToJson(Json& json, const AtomicString& _in);
	
	void FromJson(const Json& json, String16& _out);
	void FromJson(const Json& json, String16& _out, const String16& def);
	void ToJson(Json& json, const String16& _in);
	void ToJson(Json& json, const StringView16& _in);

	void FromJson(const Json& json, AtomicString16& _out);
	void FromJson(const Json& json, AtomicString16& _out, const String16& def);
	void ToJson(Json& json, const AtomicString16& _in);
	
	void ToJson(Json& json, const sl_char8* sz8);
	void ToJson(Json& json, const sl_char16* sz16);
	
	void FromJson(const Json& json, StringParam& _out);
	void ToJson(Json& json, const StringParam& _in);

#ifdef SLIB_SUPPORT_STD_TYPES
	void FromJson(const Json& json, std::string& _out);
	void FromJson(const Json& json, std::string& _out, const std::string& def);
	void ToJson(Json& json, const std::string& _in);
	
	void FromJson(const Json& json, std::u16string& _out);
	void FromJson(const Json& json, std::u16string& _out, const std::u16string& def);
	void ToJson(Json& json, const std::u16string& _in);
#endif
	
	void FromJson(const Json& json, Time& _out);
	void FromJson(const Json& json, Time& _out, const Time& def);
	void ToJson(Json& json, const Time& _in);
	
	void FromJson(const Json& json, Memory& _out);
	void ToJson(Json& json, const Memory& _in);
	
	void FromJson(const Json& json, BigInt& _out);
	void ToJson(Json& json, const BigInt& _in);
	
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
	static void FromJson(const Json& json, Ref<T>& _out)
	{
		if (json.isUndefined()) {
			return;
		}
		if (json.isNotNull()) {
			Ref<T> o = new T;
			if (o.isNotNull()) {
				FromJson(json, *(o.ptr));
				_out = Move(o);
				return;
			}
		}
		_out.setNull();
	}

	template <class T>
	static void ToJson(Json& json, const Ref<T>& ref)
	{
		if (_in.isNotNull()) {
			json = _in->toJson();
		} else {
			json.setNull();
		}
	}
	
	template <class T>
	static void FromJson(const Json& json, AtomicRef<T>& _out)
	{
		if (json.isUndefined()) {
			return;
		}
		Ref<T> t;
		FromJson(json, t);
		_out = Move(t);
	}

	template <class T>
	static void ToJson(Json& json, const AtomicRef<T>& _in)
	{
		ToJson(json, Ref<T>(_in));
	}
	

	template <class T>
	static void ToJson(Json& json, const WeakRef<T>& _in)
	{
		ToJson(json, Ref<T>(_in));
	}

	template <class T>
	static void ToJson(Json& json, const AtomicWeakRef<T>& _in)
	{
		ToJson(json, Ref<T>(_in));
	}
	
	void FromJson(const Json& json, List<Variant>& _out);
	void ToJson(Json& json, const List<Variant>& _in);
	
	void FromJson(const Json& json, AtomicList<Variant>& _out);
	void ToJson(Json& json, const AtomicList<Variant>& _in);
	
	void FromJson(const Json& json, Map<String, Variant>& _out);
	void ToJson(Json& json, const Map<String, Variant>& _in);
	
	void FromJson(const Json& json, AtomicMap<String, Variant>& _out);
	void ToJson(Json& json, const AtomicMap<String, Variant>& _in);
	
	void FromJson(const Json& json, HashMap<String, Variant>& _out);
	void ToJson(Json& json, const HashMap<String, Variant>& _in);
	
	void FromJson(const Json& json, AtomicHashMap<String, Variant>& _out);
	void ToJson(Json& json, const AtomicHashMap<String, Variant>& _in);
	
	void FromJson(const Json& json, List< Map<String, Variant> >& _out);
	void ToJson(Json& json, const List< Map<String, Variant> >& _in);
	
	void FromJson(const Json& json, AtomicList< Map<String, Variant> >& _out);
	void ToJson(Json& json, const AtomicList< Map<String, Variant> >& _in);
	
	void FromJson(const Json& json, List< HashMap<String, Variant> >& _out);
	void ToJson(Json& json, const List< HashMap<String, Variant> >& _in);
	
	void FromJson(const Json& json, AtomicList< HashMap<String, Variant> >& _out);
	void ToJson(Json& json, const AtomicList< HashMap<String, Variant> >& _in);
	
	void FromJson(const Json& json, JsonList& _out);
	void ToJson(Json& json, const JsonList& _in);
	
	void FromJson(const Json& json, AtomicJsonList& _out);
	void ToJson(Json& json, const AtomicJsonList& _in);
	
	void FromJson(const Json& json, JsonMap& _out);
	void ToJson(Json& json, const JsonMap& _in);
	
	void FromJson(const Json& json, AtomicJsonMap& _out);
	void ToJson(Json& json, const AtomicJsonMap& _in);
	
	void FromJson(const Json& json, JsonMapList& _out);
	void ToJson(Json& json, const JsonMapList& _in);
	
	void FromJson(const Json& json, AtomicJsonMapList& _out);
	void ToJson(Json& json, const AtomicJsonMapList& _in);
	
	template <class T>
	static void FromJson(const Json& json, List<T>& _out)
	{
		if (json.isUndefined()) {
			return;
		}
		List<T> dst;
		Ref<Referable> obj(json.getObject());
		if (obj.isNotNull()) {
			if (CList<Variant>* s1 = CastInstance< CList<Variant> >(obj.get())) {
				ListLocker<Variant> src(*s1);
				for (sl_size i = 0; i < src.count; i++) {
					Json& v = *(static_cast<Json*>(&(src[i])));
					T o;
					FromJson(v, o);
					dst.add_NoLock(Move(o));
				}
			} else if (CList< Map<String, Variant> >* s2 = CastInstance< CList< Map<String, Variant> > >(obj.get())) {
				ListLocker< Map<String, Variant> > src(*s2);
				for (sl_size i = 0; i < src.count; i++) {
					Json v(src[i]);
					T o;
					FromJson(v, o);
					dst.add_NoLock(Move(o));
				}
			} else if (CList< HashMap<String, Variant> >* s3 = CastInstance< CList< HashMap<String, Variant> > >(obj.get())) {
				ListLocker< HashMap<String, Variant> > src(*s3);
				for (sl_size i = 0; i < src.count; i++) {
					Json v(src[i]);
					T o;
					FromJson(v, o);
					dst.add_NoLock(Move(o));
				}
			}
		}
		_out = Move(dst);
	}

	template <class T>
	static void ToJson(Json& json, const List<T>& _in)
	{
		if (_in.isNotNull()) {
			JsonList list;
			ListLocker<T> src(_in);
			for (sl_size i = 0; i < src.count; i++) {
				T& o = src[i];
				list.add_NoLock(Json(o));
			}
			json = Move(list);
		} else {
			json.setNull();
		}
	}

	template <class T>
	static void FromJson(const Json& json, AtomicList<T>& _out)
	{
		if (json.isUndefined()) {
			return;
		}
		List<T> t;
		FromJson(json, t);
		_out = Move(t);
	}

	template <class T>
	static void ToJson(Json& json, const AtomicList<T>& _in)
	{
		ToJson(json, List<T>(_in));
	}
	
	template <class T>
	static void ToJson(Json& json, const ListParam<T>& _in)
	{
		if (_in.isNotNull()) {
			JsonList list;
			ListLocker<T> src(_in);
			for (sl_size i = 0; i < src.count; i++) {
				T& o = src[i];
				list.add_NoLock(Json(o));
			}
			json = Move(list);
		} else {
			json.setNull();
		}
	}
	
	template <class KT, class VT, class KEY_COMPARE>
	static void FromJson(const Json& json, Map<KT, VT, KEY_COMPARE>& _out)
	{
		if (json.isUndefined()) {
			return;
		}
		Map<KT, VT, KEY_COMPARE> dst;
		Ref<Referable> obj(json.getObject());
		if (obj.isNotNull()) {
			if (CMap<String, Variant>* s1 = CastInstance< CMap<String, Variant> >(obj.get())) {
				CMap<String, Variant>& map = *s1;
				MutexLocker locker(map.getLocker());
				for (auto& pair : map) {
					Json& v = *(static_cast<Json*>(&(pair.value)));
					VT o;
					FromJson(v, o);
					dst.add_NoLock(Cast<String, KT>()(pair.key), Move(o));
				}
			} else if (CHashMap<String, Variant>* s2 = CastInstance< CHashMap<String, Variant> >(obj.get())) {
				CHashMap<String, Variant>& map = *s2;
				MutexLocker locker(map.getLocker());
				for (auto& pair : map) {
					Json& v = *(static_cast<Json*>(&(pair.value)));
					VT o;
					FromJson(v, o);
					dst.add_NoLock(Cast<String, KT>()(pair.key), Move(o));
				}
			}
		}
		_out = Move(dst);
	}

	template <class KT, class VT, class KEY_COMPARE>
	static void ToJson(Json& json, const Map<KT, VT, KEY_COMPARE>& _in)
	{
		if (_in.isNotNull()) {
			MutexLocker locker(_in.getLocker());
			JsonMap map;
			for (auto& pair : _in) {
				map.put_NoLock(Cast<KT, String>()(pair.key), Json(pair.value));
			}
			json = Move(map);
		} else {
			json.setNull();
		}
	}
	
	template <class KT, class VT, class KEY_COMPARE>
	static void FromJson(const Json& json, AtomicMap<KT, VT, KEY_COMPARE>& _out)
	{
		if (json.isUndefined()) {
			return;
		}
		Map<KT, VT, KEY_COMPARE> t;
		FromJson(json, t);
		_out = Move(t);
	}

	template <class KT, class VT, class KEY_COMPARE>
	static void ToJson(Json& json, const AtomicMap<KT, VT, KEY_COMPARE>& _in)
	{
		ToJson(json, Map<KT, VT, KEY_COMPARE>(_in));
	}

	
	template <class KT, class VT, class HASH, class KEY_COMPARE>
	static void FromJson(const Json& json, HashMap<KT, VT, HASH, KEY_COMPARE>& _out)
	{
		if (json.isUndefined()) {
			return;
		}
		HashMap<KT, VT, HASH, KEY_COMPARE> dst;
		Ref<Referable> obj(json.getObject());
		if (obj.isNotNull()) {
			if (CMap<String, Variant>* s1 = CastInstance< CMap<String, Variant> >(obj.get())) {
				CMap<String, Variant>& map = *s1;
				MutexLocker locker(map.getLocker());
				for (auto& pair : map) {
					Json& v = *(static_cast<Json*>(&(pair.value)));
					VT o;
					FromJson(v, o);
					dst.add_NoLock(Cast<String, KT>()(pair.key), Move(o));
				}
			} else if (CHashMap<String, Variant>* s2 = CastInstance< CHashMap<String, Variant> >(obj.get())) {
				CHashMap<String, Variant>& map = *s2;
				MutexLocker locker(map.getLocker());
				for (auto& pair : map) {
					Json& v = *(static_cast<Json*>(&(pair.value)));
					VT o;
					FromJson(v, o);
					dst.add_NoLock(Cast<String, KT>()(pair.key), Move(o));
				}
			}
		}
		_out = Move(dst);
	}

	template <class KT, class VT, class HASH, class KEY_COMPARE>
	static void ToJson(Json& json, const HashMap<KT, VT, HASH, KEY_COMPARE>& _in)
	{
		if (_in.isNotNull()) {
			MutexLocker locker(_in.getLocker());
			JsonMap map;
			for (auto& pair : _in) {
				map.put_NoLock(Cast<KT, String>()(pair.key), Json(pair.value));
			}
			json = Move(map);
		} else {
			json.setNull();
		}
	}
	
	template <class KT, class VT, class HASH, class KEY_COMPARE>
	static void FromJson(const Json& json, AtomicHashMap<KT, VT, HASH, KEY_COMPARE>& _out)
	{
		if (json.isUndefined()) {
			return;
		}
		HashMap<KT, VT, HASH, KEY_COMPARE> t;
		FromJson(json, t);
		_out = Move(t);
	}

	template <class KT, class VT, class HASH, class KEY_COMPARE>
	static void ToJson(Json& json, const AtomicHashMap<KT, VT, HASH, KEY_COMPARE>& _in)
	{
		ToJson(json, HashMap<KT, VT, HASH, KEY_COMPARE>(_in));
	}
	
#ifdef SLIB_SUPPORT_STD_TYPES
	template <class T>
	static void FromJson(const Json& json, std::vector<T>& _out)
	{
		if (json.isUndefined()) {
			return;
		}
		_out.clear();
		Ref<Referable> obj(json.getObject());
		if (obj.isNotNull()) {
			if (CList<Variant>* s1 = CastInstance< CList<Variant> >(obj.get())) {
				ListLocker<Variant> src(*s1);
				for (sl_size i = 0; i < src.count; i++) {
					Json& v = *(static_cast<Json*>(&(src[i])));
					T o;
					FromJson(v, o);
					_out.emplace_back(Move(o));
				}
			} else if (CList< Map<String, Variant> >* s2 = CastInstance< CList< Map<String, Variant> > >(obj.get())) {
				ListLocker< Map<String, Variant> > src(*s2);
				for (sl_size i = 0; i < src.count; i++) {
					Json v(src[i]);
					T o;
					FromJson(v, o);
					_out.emplace_back(Move(o));
				}
			} else if (CList< HashMap<String, Variant> >* s3 = CastInstance< CList< HashMap<String, Variant> > >(obj.get())) {
				ListLocker< HashMap<String, Variant> > src(*s3);
				for (sl_size i = 0; i < src.count; i++) {
					Json v(src[i]);
					T o;
					FromJson(v, o);
					_out.emplace_back(Move(o));
				}
			}
		}
	}
	template <class T>
	static void ToJson(Json& json, const std::vector<T>& _in)
	{
		JsonList list;
		for (auto& item : _in) {
			list.add_NoLock(Json(item));
		}
		json = Move(list);
	}
	
	template <class KT, class VT, class COMPARE, class ALLOC>
	static void FromJson(const Json& json, std::map<KT, VT, COMPARE, ALLOC>& _out)
	{
		if (json.isUndefined()) {
			return;
		}
		_out.clear();
		Ref<Referable> obj(json.getObject());
		if (obj.isNotNull()) {
			if (CMap<String, Variant>* s1 = CastInstance< CMap<String, Variant> >(obj.get())) {
				const CMap<String, Variant>& map = *s1;
				MutexLocker locker(map.getLocker());
				for (auto& pair : map) {
					Json& v = *(static_cast<Json*>(&(pair.value)));
					VT o;
					FromJson(v, o);
					_out.emplace(pair.key, Move(o));
				}
			} else if (CHashMap<String, Variant>* s2 = CastInstance< CHashMap<String, Variant> >(obj.get())) {
				const CHashMap<String, Variant>& map = *s2;
				MutexLocker locker(map.getLocker());
				for (auto& pair : map) {
					Json& v = *(static_cast<Json*>(&(pair.value)));
					VT o;
					FromJson(v, o);
					_out.emplace(pair.key, Move(o));
				}
			}
		}
	}

	template <class KT, class VT, class COMPARE, class ALLOC>
	static void ToJson(Json& json, const std::map<KT, VT, COMPARE, ALLOC>& _in)
	{
		JsonMap map;
		for (auto& item : _in) {
			map.put_NoLock(Cast<KT, String>()(item.first), Json(item.second));
		}
		json = Move(map);
	}
	
	template <class KT, class VT, class HASH, class PRED, class ALLOC>
	static void FromJson(const Json& json, std::unordered_map<KT, VT, HASH, PRED, ALLOC>& _out)
	{
		if (json.isUndefined()) {
			return;
		}
		_out.clear();
		Ref<Referable> obj(json.getObject());
		if (obj.isNotNull()) {
			if (CMap<String, Variant>* s1 = CastInstance< CMap<String, Variant> >(obj.get())) {
				const CMap<String, Variant>& map = *s1;
				MutexLocker locker(map.getLocker());
				for (auto& pair : map) {
					Json& v = *(static_cast<Json*>(&(pair.value)));
					VT o;
					FromJson(v, o);
					_out.emplace(pair.key, Move(o));
				}
			} else if (CHashMap<String, Variant>* s2 = CastInstance< CHashMap<String, Variant> >(obj.get())) {
				const CHashMap<String, Variant>& map = *s2;
				MutexLocker locker(map.getLocker());
				for (auto& pair : map) {
					Json& v = *(static_cast<Json*>(&(pair.value)));
					VT o;
					FromJson(v, o);
					_out.emplace(pair.key, Move(o));
				}
			}
		}
	}

	template <class KT, class VT, class HASH, class PRED, class ALLOC>
	static void ToJson(Json& json, const std::unordered_map<KT, VT, HASH, PRED, ALLOC>& _in)
	{
		JsonMap map;
		for	(auto& item : _in) {
			map.put_NoLock(Cast<KT, String>()(item.first), Json(item.second));
		}
		json = Move(map);
	}
#endif

	template <class T>
	static void FromJson(const Json& json, T& _out)
	{
		priv::json::Converter<T>::fromJson(json, _out);
	}
	
	template <class T>
	static void ToJson(Json& json, const T& _in)
	{
		priv::json::Converter<T>::toJson(json, _in);
	}

}

#endif
