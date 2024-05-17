/*
 *   Copyright (c) 2008-2024 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_DATA_SERIALIZE_LIST
#define CHECKHEADER_SLIB_DATA_SERIALIZE_LIST

#include "primitive.h"

#include "../cvli.h"
#include "../../core/priv/list_helper.h"

namespace slib
{

	template <class OUTPUT, class T>
	static sl_bool SerializeList(OUTPUT* output, const T* data, sl_size count)
	{
		if (!(CVLI::serialize(output, count))) {
			return sl_false;
		}
		for (sl_size i = 0; i < count; i++) {
			if (!(Serialize(output, data[i]))) {
				return sl_false;
			}
		}
		return sl_true;
	}

	template <class INPUT, class LIST>
	static sl_bool DeserializeList(INPUT* input, LIST& _out)
	{
		sl_size count;
		if (!(CVLI::deserialize(input, count))) {
			return sl_false;
		}
		if (count) {
			if (!(ListHelper<LIST>::create(_out, count))) {
				return sl_false;
			}
			auto data = ListHelper<LIST>::getData(_out);
			for (sl_size i = 0; i < count; i++) {
				if (!(Deserialize(input, data[i]))) {
					return sl_false;
				}
			}
			return sl_true;
		} else {
			return ListHelper<LIST>::create(_out);
		}
	}


	template <class OUTPUT, class T>
	static sl_bool Serialize(OUTPUT* output, const Array<T>& _in)
	{
		return SerializeList(output, _in.getData(), _in.getCount());
	}

	template <class INPUT, class T>
	static sl_bool Deserialize(INPUT* input, Array<T>& _out)
	{
		return DeserializeList(input, _out);
	}


	template <class OUTPUT, class T>
	static sl_bool Serialize(OUTPUT* output, const List<T>& _in)
	{
		ListLocker<T> list(_in);
		return SerializeList(output, list.data, list.count);
	}

	template <class INPUT, class T>
	static sl_bool Deserialize(INPUT* input, List<T>& _out)
	{
		return DeserializeList(input, _out);
	}

	template <class OUTPUT, class T>
	static sl_bool Serialize(OUTPUT* output, const ListParam<T>& _in)
	{
		ListLocker<T> list(_in);
		return SerializeList(output, list.data, list.count);
	}


	template <class OUTPUT, class T, sl_size_t N>
	static sl_bool Serialize(OUTPUT* output, T arr[N])
	{
		for (sl_size_t i = 0; i < N; i++) {
			if (!(Serialize(output, arr[i]))) {
				return sl_false;
			}
		}
		return sl_true;
	}

	template <class INPUT, class T, sl_size_t N>
	static sl_bool Deserialize(INPUT* input, T arr[N])
	{
		for (sl_size_t i = 0; i < N; i++) {
			if (!(Deserialize(input, arr[i]))) {
				return sl_false;
			}
		}
		return sl_true;
	}


#ifdef SLIB_SUPPORT_STD_TYPES
	template <class OUTPUT, class T, class ALLOC>
	static sl_bool Serialize(OUTPUT* output, const std::vector<T, ALLOC>& _in)
	{
		return SerializeList(output, _in.data(), _in.size());
	}

	template <class INPUT, class T, class ALLOC>
	static sl_bool Deserialize(INPUT* input, std::vector<T, ALLOC>& _out)
	{
		return DeserializeList(input, _out);
	}
#endif

}

#endif
