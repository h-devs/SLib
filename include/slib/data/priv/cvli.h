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

#ifndef CHECKHEADER_SLIB_DATA_PRIV_CVLI
#define CHECKHEADER_SLIB_DATA_PRIV_CVLI

#include "../serialize/io.h"

namespace slib
{
	namespace cvli
	{

		template <class T, sl_bool isClass = __is_class(T)>
		class EncoderLE
		{
		};

		template <class T>
		class EncoderLE<T, sl_true>
		{
		public:
			static sl_uint32 encode(sl_uint8* output, T value)
			{
				sl_uint8* start = output;
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

			template <class OUTPUT>
			static sl_bool serialize(OUTPUT* output, T value)
			{
				for (;;) {
					sl_uint8 n = ((sl_uint8)value) & 127;
					value >>= 7;
					if (value) {
						if (!(SerializeByte(output, n | 128))) {
							return sl_false;
						}
					} else {
						if (!(SerializeByte(output, n))) {
							return sl_false;
						}
						break;
					}
				}
				return sl_true;
			}

		};

		template <class T>
		class EncoderLE<T, sl_false>
		{
		public:
			static sl_uint32 encode(sl_uint8* output, T value)
			{
				if (value < 128) {
					*output = (sl_uint8)value;
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

			template <class OUTPUT>
			static sl_bool serialize(OUTPUT* output, T value)
			{
				sl_uint8 octets[(sizeof(T) << 3) / 7 + 1];
				sl_uint32 n = encode(octets, value);
				return SerializeRaw(output, octets, n);
			}

		};

		template <class T, sl_bool isClass = __is_class(T)>
		class EncoderBE
		{
		};

		template <class T>
		class EncoderBE<T, sl_false>
		{
		public:
			static sl_uint8* encode(sl_uint8* output, T value)
			{
				sl_uint8 octets[(sizeof(T) << 3) / 7 + 1];
				sl_uint32 pos = sizeof(octets) - 1;
				octets[pos] = ((sl_uint8)value) & 127;
				value >>= 7;
				while (value) {
					pos--;
					octets[pos] = 128 | (((sl_uint8)value) & 127);
					value >>= 7;
				}
				while (pos < sizeof(octets)) {
					*(output++) = octets[pos];
				}
				return output;
			}

		};

		template <class T>
		static sl_uint32 DecodeLE(const void* _input, sl_size size, T& value)
		{
			sl_uint8* input = (sl_uint8*)_input;
			sl_uint8* start = input;
			sl_uint8* end = input + size;
			value = 0;
			sl_uint32 m = 0;
			while (input < end) {
				sl_uint8 n = *(input++);
				value |= (((T)(n & 127)) << m);
				m += 7;
				if (!(n & 128)) {
					return (sl_uint32)(input - start);
				}
			}
			return 0;
		}

		template <class T>
		static sl_uint32 DecodeLE(const void* _input, T& value)
		{
			sl_uint8* input = (sl_uint8*)_input;
			sl_uint8* start = input;
			value = 0;
			sl_uint32 m = 0;
			for (;;) {
				sl_uint8 n = *(input++);
				value |= (((T)(n & 127)) << m);
				m += 7;
				if (!(n & 128)) {
					break;
				}
			}
			return (sl_uint32)(input - start);
		}

		// returns the size of bytes read, or zero on error
		template <class T>
		static sl_uint32 DecodeBE(const void* _input, sl_size size, T& value)
		{
			sl_uint8* input = (sl_uint8*)_input;
			sl_uint8* start = input;
			sl_uint8* end = input + size;
			value = 0;
			while (input < end) {
				sl_uint8 n = *(input++);
				value = (value << 7) | (n & 127);
				if (!(n & 128)) {
					return (sl_uint32)(input - start);
				}
			}
			return 0;
		}

		template <class READER, class T>
		static sl_bool Read(READER* reader, T* output, EndianType endian)
		{
			T value = 0;
			sl_uint8 n;
			sl_uint32 m = 0;
			while (reader->readUint8(&n)) {
				if (endian == EndianType::Little) {
					value |= (((T)(n & 127)) << m);
					m += 7;
				} else {
					value = (value << 7) | (n & 127);
				}
				if (!(n & 128)) {
					if (output) {
						*output = value;
					}
					return sl_true;
				}
			}
			return sl_false;
		}

		template <class READER, class T>
		static T Read(READER* reader, T def, EndianType endian)
		{
			T v;
			if (Read(reader, &v, endian)) {
				return v;
			} else {
				return def;
			}
		}

	}
}

#endif
