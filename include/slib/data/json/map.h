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

#ifndef CHECKHEADER_SLIB_DATA_JSON_MAP
#define CHECKHEADER_SLIB_DATA_JSON_MAP

#include "core.h"

#include "../../core/priv/map_helper.h"

namespace slib
{

	namespace priv
	{

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
					PropertyIterator iterator = src->getPropertyIterator();
					while (iterator.moveNext()) {
						typename MAP::VALUE_TYPE v;
						Variant value = iterator.getValue();
						FromJson(*(static_cast<const Json*>(&value)), v);
						MapHelper<MAP>::add(_out, Cast<String, typename MAP::KEY_TYPE>()(iterator.getKey()), Move(v));
					}
				}
			}
		}

		template <class MAP>
		static void ToJsonMap(Json& _out, const MAP& _in)
		{
			if (_in.isNotNull()) {
				MutexLocker locker(_in.getLocker());
				JsonMap map;
				auto node = _in.getFirstNode();
				while (node) {
					map.put_NoLock(Cast<typename MAP::KEY_TYPE, String>()(node->key), node->value);
					node = node->getNext();
				}
				_out = Move(map);
			} else {
				_out.setNull();
			}
		}

	}

	template <class KT, class VT, class KEY_COMPARE>
	static void FromJson(const Json& json, Map<KT, VT, KEY_COMPARE>& _out)
	{
		priv::GetMapFromJson(_out, json);
	}

	template <class KT, class VT, class KEY_COMPARE>
	static void ToJson(Json& json, const Map<KT, VT, KEY_COMPARE>& _in)
	{
		priv::ToJsonMap(json, _in);
	}

	template <class KT, class VT, class HASH, class KEY_COMPARE>
	static void FromJson(const Json& json, HashMap<KT, VT, HASH, KEY_COMPARE>& _out)
	{
		priv::GetMapFromJson(_out, json);
	}

	template <class KT, class VT, class HASH, class KEY_COMPARE>
	static void ToJson(Json& json, const HashMap<KT, VT, HASH, KEY_COMPARE>& _in)
	{
		priv::ToJsonMap(json, _in);
	}

#ifdef SLIB_SUPPORT_STD_TYPES
	template <class KT, class... TYPES>
	static void FromJson(const Json& json, std::map<KT, TYPES...>& _out)
	{
		priv::GetMapFromJson(_out, json);
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
		priv::GetMapFromJson(_out, json);
	}

	template <class KT, class... TYPES>
	static void ToJson(Json& json, const std::unordered_map<KT, TYPES...>& _in)
	{
		JsonMap map;
		for (auto& item : _in) {
			map.put_NoLock(Cast<KT, String>()(item.first), Json(item.second));
		}
		json = Move(map);
	}
#endif

}

#endif
