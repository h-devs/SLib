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

#ifndef CHECKHEADER_SLIB_DATA_JSON_SET
#define CHECKHEADER_SLIB_DATA_JSON_SET

#include "core.h"

#include "../../core/set.h"
#include "../../core/hash_set.h"

namespace slib
{

	namespace priv
	{
		namespace json
		{

			template <class SET>
			static void GetSetFromJson(SET& _out, const Json& json)
			{
				if (json.isUndefined()) {
					return;
				}
				_out.setNull();
				if (json.getType() == VariantType::List) {
					JsonList list = json.getJsonList();
					if (list.isNotNull()) {
						ListLocker<Json> src(list);
						if (src.count) {
							for (sl_size i = 0; i < src.count; i++) {
								typename SET::VALUE_TYPE v;
								FromJson(src[i], v);
								_out.add_NoLock(Move(v));
							}
							return;
						}
					}
				} else {
					Ref<Collection> src = json.getCollection();
					if (src.isNotNull()) {
						sl_size n = (sl_size)(src->getElementCount());
						if (n) {
							for (sl_size i = 0; i < n; i++) {
								typename SET::VALUE_TYPE v;
								Variant value = src->getElement(i);
								FromJson(*(static_cast<const Json*>(&value)), v);
								_out.add_NoLock(Move(v));
							}
							return;
						}
					}
				}
			}

			template <class SET>
			static void GetJsonFromSet(Json& _out, const SET& _in)
			{
				if (_in.isNotNull()) {
					MutexLocker locker(_in.getLocker());
					JsonList list;
					auto node = _in.getFirstNode();
					while (node) {
						list.add_NoLock(node->key);
						node = node->getNext();
					}
					_out = Move(list);
				} else {
					_out.setNull();
				}
			}

		}
	}

	template <class T>
	static void ToJson(Json& json, const Set<T>& _in)
	{
		priv::json::GetJsonFromSet(json, _in);
	}

	template <class T>
	static void FromJson(const Json& json, Set<T>& _out)
	{
		priv::json::GetSetFromJson(_out, json);
	}

	template <class T>
	static void ToJson(Json& json, const HashSet<T>& _in)
	{
		priv::json::GetJsonFromSet(json, _in);
	}

	template <class T>
	static void FromJson(const Json& json, HashSet<T>& _out)
	{
		priv::json::GetSetFromJson(_out, json);
	}

}

#endif
