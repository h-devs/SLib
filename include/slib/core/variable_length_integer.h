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

#ifndef CHECKHEADER_SLIB_CORE_VARIABLE_LENGTH_INTEGER
#define CHECKHEADER_SLIB_CORE_VARIABLE_LENGTH_INTEGER

#include "serialize_io.h"

namespace slib
{

	// Chain Variable Length Integer
	class CVLI
	{
	public:
		// returns the size of bytes written, or zero on error
		template <class OUTPUT, class T>
		static sl_uint32 serialize(OUTPUT* output, T value)
		{
			sl_bool flagContinue = sl_true;
			sl_uint32 count = 0;
			do {
				sl_uint8 n = ((sl_uint8)value) & 127;
				value >>= 7;
				if (value) {
					n |= 128;
				} else {
					flagContinue = sl_false;
				}
				if (SerializeByte(output, (sl_uint8)value)) {
					count++;
				} else {
					return 0;
				}
			} while (flagContinue);
			return count;
		}
		
		// returns the size of bytes read, or zero on error
		template <class INPUT, class T>
		static sl_uint32 deserialize(INPUT* input, T& value)
		{
			value = 0;
			sl_uint32 count = 0;
			sl_uint32 m = 0;
			for (;;) {
				sl_uint8 n;
				if (DeserializeByte(input, n)) {
					value += (((sl_uint32)(n & 127)) << m);
					m += 7;
					count++;
					if (!(n & 128)) {
						return count;
					}
				} else {
					break;
				}
			}
			return 0;
		}

		template <class OUTPUT, class T>
		static sl_uint32 serializeSigned(OUTPUT* output, const T& value)
		{
			if (value >= 0) {
				return serialize(output, typename UnsignedTypeFromSignedType<T>::Type(value << 1));
			} else {
				return serialize(output, typename UnsignedTypeFromSignedType<T>::Type(((-value - 1) << 1) | 1));
			}
		}

		// returns the size of bytes parsed from `buf`, or zero on error
		template <class INPUT, class T>
		static sl_uint32 deserializeSigned(INPUT* input, T& value)
		{
			typename UnsignedTypeFromSignedType<T>::Type v;
			sl_uint32 n = deserialize(input, v);
			if (n) {
				if (v & 1) {
					value = -(T)(v >> 1) - 1;
				} else {
					value = v >> 1;
				}
			}
			return n;
		}

	};

}

#endif