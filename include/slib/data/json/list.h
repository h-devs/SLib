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

#ifndef CHECKHEADER_SLIB_DATA_JSON_LIST
#define CHECKHEADER_SLIB_DATA_JSON_LIST

#include "core.h"

#include "../../core/priv/list_helper.h"

namespace slib
{

	namespace priv
	{

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
					sl_size n = (sl_size)(src->getElementCount());
					if (n) {
						if (ListHelper<LIST>::create(_out, n)) {
							auto dst = ListHelper<LIST>::getData(_out);
							for (sl_size i = 0; i < n; i++) {
								Variant value = src->getElement(i);
								FromJson(*(static_cast<const Json*>(&value)), dst[i]);
							}
						}
						return;
					}
				}
			}
			ListHelper<LIST>::clear(_out);
		}

	}

	template <class T>
	static void ToJson(Json& json, const Array<T>& _in)
	{
		json = JsonList::create(_in);
	}

	template <class T>
	static void FromJson(const Json& json, Array<T>& _out)
	{
		priv::GetListFromJson(_out, json);
	}

	template <class T>
	static void ToJson(Json& json, const List<T>& _in)
	{
		json = JsonList::createCopy(_in);
	}

	template <class T>
	static void FromJson(const Json& json, List<T>& _out)
	{
		priv::GetListFromJson(_out, json);
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

#ifdef SLIB_SUPPORT_STD_TYPES
	template <class T, class ALLOC>
	static void FromJson(const Json& json, std::vector<T, ALLOC>& _out)
	{
		priv::GetListFromJson(_out, json);
	}

	template <class T, class ALLOC>
	static void ToJson(Json& json, const std::vector<T, ALLOC>& _in)
	{
		json = JsonList::create(_in.data(), _in.size());
	}
#endif

}

#endif
