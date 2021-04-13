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

#ifndef CHECKHEADER_SLIB_CORE_SERIALIZE_MAP
#define CHECKHEADER_SLIB_CORE_SERIALIZE_MAP

#include "serialize_primitive.h"
#include "variable_length_integer.h"
#include "map_helper.h"

namespace slib
{

	template <class OUTPUT, class MAP>
	static sl_bool SerializeMap(OUTPUT* output, const MAP& _in)
	{
		sl_size count = _in.getCount();
		if (!(CVLI::serialize(output, count))) {
			return sl_false;
		}
		if (count) {
			MutexLocker locker(_in.getLocker());
			auto node = _in.getFirstNode();
			while (node) {
				if (!(Serialize(output, node->key))) {
					return sl_false;
				}
				if (!(Serialize(output, node->value))) {
					return sl_false;
				}
				node = node->getNext();
			}
		}
		return sl_true;
	}

	template <class INPUT, class MAP>
	static sl_bool DeserializeMap(INPUT* input, MAP& _out)
	{
		sl_size count;
		if (!(CVLI::deserialize(input, count))) {
			return sl_false;
		}
		MapHelper<MAP>::clear(_out);
		for (sl_size i = 0; i < count; i++) {
			typename MAP::KEY_TYPE k;
			if (!(Deserialize(input, k))) {
				return sl_false;
			}
			typename MAP::VALUE_TYPE v;
			if (!(Deserialize(input, v))) {
				return sl_false;
			}
			if (!(MapHelper<MAP>::add(_out, Move(k), Move(v)))) {
				return sl_false;
			}
		}
		return sl_true;
	}


	template <class OUTPUT, class KT, class VT, class KEY_COMPARE>
	static sl_bool Serialize(OUTPUT* output, const Map<KT, VT, KEY_COMPARE>& _in)
	{
		return SerializeMap(output, _in);
	}

	template <class INPUT, class KT, class VT, class KEY_COMPARE>
	static sl_bool Deserialize(INPUT* input, Map<KT, VT, KEY_COMPARE>& _out)
	{
		return DeserializeMap(input, _out);
	}


	template <class OUTPUT, class KT, class VT, class HASH, class KEY_COMPARE>
	static sl_bool Serialize(OUTPUT* output, const HashMap<KT, VT, HASH, KEY_COMPARE>& _in)
	{
		return SerializeMap(output, _in);
	}

	template <class INPUT, class KT, class VT, class HASH, class KEY_COMPARE>
	static sl_bool Deserialize(INPUT* input, HashMap<KT, VT, HASH, KEY_COMPARE>& _out)
	{
		return DeserializeMap(input, _out);
	}


#ifdef SLIB_SUPPORT_STD_TYPES
	template <class OUTPUT, class KT, class... TYPES>
	static sl_bool Serialize(OUTPUT* output, const std::map<KT, TYPES...>& _in)
	{
		sl_size count = _in.size();
		if (!(CVLI::serialize(output, count))) {
			return sl_false;
		}
		if (count) {
			for (auto& item : _in) {
				if (!(Serialize(output, item.first))) {
					return sl_false;
				}
				if (!(Serialize(output, item.second))) {
					return sl_false;
				}
			}
		}
		return sl_true;
	}

	template <class INPUT, class KT, class... TYPES>
	static sl_bool Deserialize(INPUT* input, std::map<KT, TYPES...>& _out)
	{
		return DeserializeMap(input, _out);
	}

	template <class OUTPUT, class KT, class... TYPES>
	static sl_bool Serialize(OUTPUT* output, const std::unordered_map<KT, TYPES...>& _in)
	{
		sl_size count = _in.size();
		if (!(CVLI::serialize(output, count))) {
			return sl_false;
		}
		if (count) {
			for (auto& item : _in) {
				if (!(Serialize(output, item.first))) {
					return sl_false;
				}
				if (!(Serialize(output, item.second))) {
					return sl_false;
				}
			}
		}
		return sl_true;
	}

	template <class INPUT, class KT, class... TYPES>
	static sl_bool Deserialize(INPUT* input, std::unordered_map<KT, TYPES...>& _out)
	{
		return DeserializeMap(input, _out);
	}
#endif

}

#endif
