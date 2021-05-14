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

#ifndef CHECKHEADER_SLIB_CORE_SERIALIZE_REF
#define CHECKHEADER_SLIB_CORE_SERIALIZE_REF

#include "primitive.h"

#include "../ref.h"

namespace slib
{

	template <class OUTPUT, class T>
	static sl_bool Serialize(OUTPUT* output, const Ref<T>& _in)
	{
		if (_in.isNull()) {
			return SerializeStatic(output, "", 1);
		} else {
			if (!(SerializeStatic(output, "\x01", 1))) {
				return sl_false;
			}
			return Serialize(output, *(_in.get()));
		}
	}

	template <class OUTPUT, class T>
	static sl_bool Serialize(OUTPUT* output, const WeakRef<T>& _in)
	{
		return Serialize(output, Ref<T>(_in));
	}

	template <class INPUT, class T>
	static sl_bool Deserialize(INPUT* input, Ref<T>& _out)
	{
		sl_uint8 f;
		if (!(DeserializeByte(input, f))) {
			return sl_false;
		}
		if (f) {
			T* t = new T;
			if (t) {
				_out = t;
				return Deserialize(input, *t);
			} else {
				return sl_false;
			}
		} else {
			_out.setNull();
			return sl_true;
		}
	}

}

#endif
