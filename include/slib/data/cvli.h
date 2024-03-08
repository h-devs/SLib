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

#ifndef CHECKHEADER_SLIB_DATA_CVLI
#define CHECKHEADER_SLIB_DATA_CVLI

#include "definition.h"

#include "../core/endian.h"

#include "priv/cvli.h"

namespace slib
{

	// Chain Variable Length Integer
	class SLIB_EXPORT CVLI
	{
	public:
		// returns the size of bytes written, or zero on error
		template <class T>
		static sl_uint32 encode(sl_uint8* output, const T& value, EndianType endian)
		{
			if (endian == Endian::Big) {
				return cvli::EncoderBE<T>::encode(output, value);
			} else {
				return cvli::EncoderLE<T>::encode(output, value);
			}
		}

		// returns the size of bytes written, or zero on error
		template <class T>
		static sl_uint32 encode(sl_uint8* output, const T& value)
		{
			return cvli::EncoderLE<T>::encode(output, value);
		}

		// returns the size of bytes read, or zero on error
		template <class T>
		static sl_uint32 decode(const void* input, sl_size size, T& value, EndianType endian)
		{
			if (endian == EndianType::Big) {
				return cvli::DecodeBE(input, size, value);
			} else {
				return cvli::DecodeLE(input, size, value);
			}
		}

		// returns the size of bytes read, or zero on error
		template <class T>
		static sl_uint32 decode(const void* input, sl_size size, T& value)
		{
			return cvli::DecodeLE(input, size, value);
		}

		template <class WRITER>
		static sl_bool write(WRITER* writer, sl_size value, EndianType endian = Endian::Little)
		{
			sl_uint8 t[16];
			sl_uint32 n = encode(t, value, endian);
			return writer->writeFully(t, n) == n;
		}

		template <class WRITER>
		static sl_bool write32(WRITER* writer, sl_uint32 value, EndianType endian = Endian::Little)
		{
			sl_uint8 t[16];
			sl_uint32 n = encode(t, value, endian);
			return writer->writeFully(t, n) == n;
		}

		template <class WRITER>
		static sl_bool write64(WRITER* writer, sl_uint64 value, EndianType endian = Endian::Little)
		{
			sl_uint8 t[16];
			sl_uint32 n = encode(t, value, endian);
			return writer->writeFully(t, n) == n;
		}

		template <class READER>
		static sl_bool read(READER* reader, sl_size* output, EndianType endian = Endian::Little)
		{
			return cvli::Read(reader, output, endian);
		}

		template <class READER>
		static sl_size read(READER* reader, sl_size def = 0, EndianType endian = Endian::Little)
		{
			return cvli::Read(reader, def, endian);
		}

		template <class READER>
		static sl_bool read32(READER* reader, sl_uint32* output, EndianType endian = Endian::Little)
		{
			return cvli::Read(reader, output, endian);
		}

		template <class READER>
		static sl_uint32 read32(READER* reader, sl_uint32 def = 0, EndianType endian = Endian::Little)
		{
			return cvli::Read(reader, def, endian);
		}

		template <class READER>
		static sl_bool read64(READER* reader, sl_uint64* output, EndianType endian = Endian::Little)
		{
			return cvli::Read(reader, output, endian);
		}

		template <class READER>
		static sl_uint64 read64(READER* reader, sl_uint64 def = 0, EndianType endian = Endian::Little)
		{
			return cvli::Read(reader, def, endian);
		}

		template <class T>
		static sl_bool serialize(sl_uint8*& output, const T& value)
		{
			output += cvli::EncoderLE<T>::encode(output, value);
			return sl_true;
		}

		template <class T>
		static sl_bool serialize(sl_uint8** output, const T& value)
		{
			*output += cvli::EncoderLE<T>::encode(*output, value);
			return sl_true;
		}

		template <class OUTPUT, class T>
		static sl_bool serialize(OUTPUT* output, const T& value)
		{
			return cvli::EncoderLE<T>::serialize(output, value);
		}

		template <class T>
		static sl_bool deserialize(sl_uint8*& output, T& value)
		{
			output += cvli::DecodeLE(output, value);
			return sl_true;
		}

		template <class T>
		static sl_bool deserialize(sl_uint8** output, T& value)
		{
			*output += cvli::DecodeLE(*output, value);
			return sl_true;
		}

		template <class INPUT, class T>
		static sl_bool deserialize(INPUT* input, T& value)
		{
			value = 0;
			sl_uint32 m = 0;
			sl_uint8 n;
			while (DeserializeByte(input, n)) {
				value |= (((T)(n & 127)) << m);
				m += 7;
				if (!(n & 128)) {
					return sl_true;
				}
			}
			return sl_false;
		}

	};

}

#endif