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

#ifndef CHECKHEADER_SLIB_CORE_SERIALIZE_PRIMITIVE
#define CHECKHEADER_SLIB_CORE_SERIALIZE_PRIMITIVE

#include "serialize_io.h"
#include "mio.h"

namespace slib
{

	template <class OUTPUT>
	static sl_bool Serialize(OUTPUT* output, signed char _in)
	{
		return SerializeByte(output, _in);
	}

	template <class INPUT>
	static sl_bool Deserialize(INPUT* input, signed char& _out)
	{
		return DeserializeByte(input, *((sl_uint8*)&_out));
	}

	template <class OUTPUT>
	static sl_bool Serialize(OUTPUT* output, unsigned char _in)
	{
		return SerializeByte(output, _in);
	}

	template <class INPUT>
	static sl_bool Deserialize(INPUT* input, unsigned char& _out)
	{
		return DeserializeByte(input, &_out);
	}


	template <class OUTPUT>
	static sl_bool Serialize(OUTPUT* output, short _in)
	{
		sl_uint8 buf[2];
		MIO::writeInt16LE(buf, _in);
		return SerializeRaw(output, buf, 2);
	}

	template <class INPUT>
	static sl_bool Deserialize(INPUT* input, short& _out)
	{
		sl_uint8 buf[2];
		if (DeserializeRaw(input, buf, 2)) {
			_out = MIO::readInt16LE(buf);
			return sl_true;
		}
		return sl_false;
	}

	template <class OUTPUT>
	static sl_bool Serialize(OUTPUT* output, unsigned short _in)
	{
		sl_uint8 buf[2];
		MIO::writeUint16LE(buf, _in);
		return SerializeRaw(output, buf, 2);
	}

	template <class INPUT>
	static sl_bool Deserialize(INPUT* input, unsigned short& _out)
	{
		sl_uint8 buf[2];
		if (DeserializeRaw(input, buf, 2)) {
			_out = MIO::readUint16LE(buf);
			return sl_true;
		}
		return sl_false;
	}


	template <class OUTPUT>
	static sl_bool Serialize(OUTPUT* output, int _in)
	{
		sl_uint8 buf[4];
		MIO::writeInt32LE(buf, (sl_int32)_in);
		return SerializeRaw(output, buf, 4);
	}

	template <class INPUT>
	static sl_bool Deserialize(INPUT* input, int& _out)
	{
		sl_uint8 buf[4];
		if (DeserializeRaw(input, buf, 4)) {
			_out = (int)(MIO::readInt32LE(buf));
			return sl_true;
		}
		return sl_false;
	}

	template <class OUTPUT>
	static sl_bool Serialize(OUTPUT* output, unsigned int _in)
	{
		sl_uint8 buf[4];
		MIO::writeUint32LE(buf, (sl_uint32)_in);
		return SerializeRaw(output, buf, 4);
	}

	template <class INPUT>
	static sl_bool Deserialize(INPUT* input, unsigned int& _out)
	{
		sl_uint8 buf[4];
		if (DeserializeRaw(input, buf, 4)) {
			_out = (unsigned int)(MIO::readUint32LE(buf));
			return sl_true;
		}
		return sl_false;
	}


	template <class OUTPUT>
	static sl_bool Serialize(OUTPUT* output, long _in)
	{
		sl_uint8 buf[4];
		MIO::writeInt32LE(buf, (sl_int32)_in);
		return SerializeRaw(output, buf, 4);
	}

	template <class INPUT>
	static sl_bool Deserialize(INPUT* input, long& _out)
	{
		sl_uint8 buf[4];
		if (DeserializeRaw(input, buf, 4)) {
			_out = (long)(MIO::readInt32LE(buf));
			return sl_true;
		}
		return sl_false;
	}

	template <class OUTPUT>
	static sl_bool Serialize(OUTPUT* output, unsigned long _in)
	{
		sl_uint8 buf[4];
		MIO::writeUint32LE(buf, (sl_uint32)_in);
		return SerializeRaw(output, buf, 4);
	}

	template <class INPUT>
	static sl_bool Deserialize(INPUT* input, unsigned long& _out)
	{
		sl_uint8 buf[4];
		if (DeserializeRaw(input, buf, 4)) {
			_out = (unsigned long)(MIO::readUint32LE(buf));
			return sl_true;
		}
		return sl_false;
	}


	template <class OUTPUT>
	static sl_bool Serialize(OUTPUT* output, sl_int64 _in)
	{
		sl_uint8 buf[8];
		MIO::writeInt64LE(buf, _in);
		return SerializeRaw(output, buf, 8);
	}

	template <class INPUT>
	static sl_bool Deserialize(INPUT* input, sl_int64& _out)
	{
		sl_uint8 buf[8];
		if (DeserializeRaw(input, buf, 8)) {
			_out = MIO::readInt64LE(buf);
			return sl_true;
		}
		return sl_false;
	}

	template <class OUTPUT>
	static sl_bool Serialize(OUTPUT* output, sl_uint64 _in)
	{
		sl_uint8 buf[8];
		MIO::writeUint64LE(buf, _in);
		return SerializeRaw(output, buf, 8);
	}

	template <class INPUT>
	static sl_bool Deserialize(INPUT* input, sl_uint64& _out)
	{
		sl_uint8 buf[8];
		if (DeserializeRaw(input, buf, 8)) {
			_out = MIO::readUint64LE(buf);
			return sl_true;
		}
		return sl_false;
	}


	template <class OUTPUT>
	static sl_bool Serialize(OUTPUT* output, float _in)
	{
		sl_uint8 buf[4];
		MIO::writeFloatLE(buf, _in);
		return SerializeRaw(output, buf, 4);
	}

	template <class INPUT>
	static sl_bool Deserialize(INPUT* input, float& _out)
	{
		sl_uint8 buf[4];
		if (DeserializeRaw(input, buf, 4)) {
			_out = MIO::readFloatLE(buf);
			return sl_true;
		}
		return sl_false;
	}

	template <class OUTPUT>
	static sl_bool Serialize(OUTPUT* output, double _in)
	{
		sl_uint8 buf[8];
		MIO::writeDoubleLE(buf, _in);
		return SerializeRaw(output, buf, 8);
	}

	template <class INPUT>
	static sl_bool Deserialize(INPUT* input, double& _out)
	{
		sl_uint8 buf[8];
		if (DeserializeRaw(input, buf, 8)) {
			_out = MIO::readDoubleLE(buf);
			return sl_true;
		}
		return sl_false;
	}


	template <class OUTPUT>
	static sl_bool Serialize(OUTPUT* output, bool _in)
	{
		return SerializeByte(output, _in ? 1 : 0);
	}

	template <class INPUT>
	static sl_bool Deserialize(INPUT* input, bool& _out)
	{
		sl_uint8 v;
		if (DeserializeByte(input, v)) {
			_out = v != 0;
			return sl_true;
		}
		return sl_false;
	}

}

#endif
