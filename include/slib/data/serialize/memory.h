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

#ifndef CHECKHEADER_SLIB_DATA_SERIALIZE_MEMORY
#define CHECKHEADER_SLIB_DATA_SERIALIZE_MEMORY

#include "primitive.h"
#include "variable_length_integer.h"
#include "generic.h"

#include "../../core/memory_buffer.h"

namespace slib
{

	template <class OUTPUT>
	static sl_bool Serialize(OUTPUT* output, const MemoryView& _in)
	{
		if (!(CVLI::serialize(output, _in.size))) {
			return sl_false;
		}
		if (_in.size) {
			return SerializeRaw(output, _in.data, _in.size);
		} else {
			return sl_true;
		}
	}

	template <class OUTPUT>
	sl_bool Memory::serialize(OUTPUT* output) const
	{
		return Serialize(output, MemoryView(getData(), getSize()));
	}

	template <class INPUT>
	sl_bool Memory::deserialize(INPUT* input)
	{
		sl_size size;
		if (!(CVLI::deserialize(input, size))) {
			return sl_false;
		}
		if (size) {
			Memory ret = Memory::create(size);
			if (ret.isNull()) {
				return sl_false;
			}
			if (DeserializeRaw(input, ret.getData(), size)) {
				*this = Move(ret);
				return sl_true;
			}
			return sl_false;
		} else {
			setNull();
			return sl_true;
		}
	}

	template <class OUTPUT>
	static sl_bool SerializeRaw(OUTPUT* output, MemoryBuffer& buf)
	{
		MemoryData data;
		while (buf.pop(data)) {
			if (!(SerializeRaw(output, Move(data)))) {
				return sl_false;
			}
		}
		return sl_true;
	}

	template <class T>
	static Memory SerializeToMemory(const T& t)
	{
		SerializeOutput output;
		if (Serialize(&output, t)) {
			return output.releaseToMemory();
		}
		return sl_null;
	}

	template <class T>
	static sl_bool DeserializeFromMemory(T& t, const void* data, sl_size size)
	{
		if (data && size) {
			SerializeBuffer buf(data, size);
			return Deserialize(&buf, t);
		}
		return sl_false;
	}

	template <class T>
	static sl_bool DeserializeFromMemory(T& t, const MemoryView& mem)
	{
		return DeserializeFromMemory(t, mem.data, mem.size);
	}

}

#endif
