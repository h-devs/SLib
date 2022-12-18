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

#ifndef CHECKHEADER_SLIB_DATA_SERIALIZE_VARIABLE_LENGTH_INTEGER
#define CHECKHEADER_SLIB_DATA_SERIALIZE_VARIABLE_LENGTH_INTEGER

#include "io.h"

#include "../../core/endian.h"

namespace slib
{

	template <class T, sl_bool isClass = __is_class(T)>
	class CVLISerializerLE
	{
	};

	template <class T>
	class CVLISerializerLE<T, sl_true>
	{
	public:
		template <class OUTPUT>
		static sl_uint32 serialize(OUTPUT* output, T value)
		{
			sl_uint32 count = 0;
			sl_bool flagContinue = sl_true;
			do {
				sl_uint8 n = ((sl_uint8)value) & 127;
				value >>= 7;
				if (value) {
					n |= 128;
				} else {
					flagContinue = sl_false;
				}
				if (SerializeByte(output, n)) {
					count++;
				} else {
					return 0;
				}
			} while (flagContinue);
			return count;
		}

	};

	template <class T>
	class CVLISerializerLE<T, sl_false>
	{
	public:
		static sl_uint32 serialize(sl_uint8*& output, T value)
		{
			if (value < 128) {
				*(output++) = (sl_uint8)value;
				return 1;
			}
			sl_uint8* start = output;
			*(output++) = (((sl_uint8)value) & 127) | 128;
			value >>= 7;
			for (;;) {
				sl_uint8 n = ((sl_uint8)value) & 127;
				value >>= 7;
				if (value) {
					*(output++) = n | 128;
				} else {
					*(output++) = n;
					break;
				}
			}
			return (sl_uint32)(output - start);
		}

		static sl_uint32 serialize(sl_uint8** output, const T& value)
		{
			return serialize(*output, value);
		}

		template <class OUTPUT>
		static sl_uint32 serialize(OUTPUT* output, T value)
		{
			sl_uint8 octets[(sizeof(T) << 3) / 7 + 1];
			sl_uint32 count = 0;
			sl_bool flagContinue = sl_true;
			do {
				sl_uint8 n = ((sl_uint8)value) & 127;
				value >>= 7;
				if (value) {
					n |= 128;
				} else {
					flagContinue = sl_false;
				}
				octets[count] = n;
				count++;
			} while (flagContinue);
			return SerializeRaw(output, octets, count);
		}

	};

	template <class T, sl_bool isClass = __is_class(T)>
	class CVLISerializerBE
	{
	};

	template <class T>
	class CVLISerializerBE<T, sl_false>
	{
	public:
		template <class OUTPUT>
		static sl_uint32 serialize(OUTPUT* output, T value)
		{
			sl_uint8 octets[(sizeof(T) << 3) / 7 + 1];
			sl_uint32 count = 1;
			sl_uint32 pos = sizeof(octets) - 1;
			octets[pos] = ((sl_uint8)value) & 127;
			value >>= 7;
			while (value) {
				pos--;
				octets[pos] = 128 | (((sl_uint8)value) & 127);
				count++;
				value >>= 7;
			}
			if (SerializeRaw(output, octets + pos, count)) {
				return count;
			} else {
				return 0;
			}
		}

		static sl_uint32 serialize(sl_uint8*& output, const T& value)
		{
			return serialize(&output, value);
		}

	};

	// Chain Variable Length Integer
	class SLIB_EXPORT CVLI
	{
	public:
		// returns the size of bytes written, or zero on error
		template <class OUTPUT, class T>
		static sl_uint32 serializeLE(OUTPUT* output, const T& value)
		{
			return CVLISerializerLE<T>::serialize(output, value);
		}

		// returns the size of bytes written, or zero on error
		template <class OUTPUT, class T>
		static sl_uint32 serializeBE(OUTPUT* output, const T& value)
		{
			return CVLISerializerBE<T>::serialize(output, value);
		}

		// returns the size of bytes written, or zero on error
		template <class OUTPUT, class T>
		static sl_uint32 serialize(OUTPUT* output, const T& value)
		{
			return CVLISerializerLE<T>::serialize(output, value);
		}

		// returns the size of bytes written, or zero on error
		template <class OUTPUT, class T>
		static sl_uint32 serialize(OUTPUT* output, const T& value, EndianType endian)
		{
			if (endian == Endian::Big) {
				return CVLISerializerBE<T>::serialize(output, value);
			} else {
				return CVLISerializerLE<T>::serialize(output, value);
			}
		}

		// returns the size of bytes read, or zero on error
		template <class INPUT, class T>
		static sl_uint32 deserializeLE(INPUT* input, T& value)
		{
			value = 0;
			sl_uint32 count = 0;
			sl_uint32 m = 0;
			sl_uint8 n;
			while (DeserializeByte(input, n)) {
				value |= (((T)(n & 127)) << m);
				m += 7;
				count++;
				if (!(n & 128)) {
					return count;
				}
			}
			return 0;
		}

		// returns the size of bytes read, or zero on error
		template <class INPUT, class T>
		static sl_uint32 deserializeBE(INPUT* input, T& value)
		{
			value = 0;
			sl_uint32 count = 0;
			sl_uint8 n;
			while (DeserializeByte(input, n)) {
				value = (value << 7) | (n & 127);
				count++;
				if (!(n & 128)) {
					return count;
				}
			}
			return 0;
		}

		// returns the size of bytes read, or zero on error
		template <class INPUT, class T>
		static sl_uint32 deserialize(INPUT* input, T& value)
		{
			return deserializeLE(input, value);
		}

		// returns the size of bytes read, or zero on error
		template <class INPUT, class T>
		static sl_uint32 deserialize(INPUT* input, T& value, EndianType endian)
		{
			if (endian == Endian::Big) {
				return deserializeBE(input, value);
			} else {
				return deserializeLE(input, value);
			}
		}

		// returns the size of bytes read, or zero on error
		template <class T>
		static sl_uint32 deserializeLE(const void* _input, sl_size size, T& value)
		{
			sl_uint8* input = (sl_uint8*)_input;
			value = 0;
			sl_uint32 m = 0;
			sl_uint32 count = 0;
			while (count < size) {
				sl_uint8 n = *(input++);
				value |= (((T)(n & 127)) << m);
				m += 7;
				count++;
				if (!(n & 128)) {
					return count;
				}
			}
			return 0;
		}

		// returns the size of bytes read, or zero on error
		template <class T>
		static sl_uint32 deserializeBE(const void* _input, sl_size size, T& value)
		{
			sl_uint8* input = (sl_uint8*)_input;
			value = 0;
			sl_uint32 count = 0;
			while (count < size) {
				sl_uint8 n = *(input++);
				value = (value << 7) | (n & 127);
				count++;
				if (!(n & 128)) {
					return count;
				}
			}
			return 0;
		}

		// returns the size of bytes read, or zero on error
		template <class T>
		static sl_uint32 deserialize(const void* input, sl_size size, T& value)
		{
			return deserializeLE(input, size, value);
		}

		// returns the size of bytes read, or zero on error
		template <class T>
		static sl_uint32 deserialize(const void* input, sl_size size, T& value, EndianType endian)
		{
			if (endian == EndianType::Big) {
				return deserializeBE(input, size, value);
			} else {
				return deserializeLE(input, size, value);
			}
		}

	};

}

#endif