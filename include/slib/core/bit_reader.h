/*
 *   Copyright (c) 2008-2020 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_CORE_BIT_READER
#define CHECKHEADER_SLIB_CORE_BIT_READER

#include "cpp_helper.h"

namespace slib
{
	
	template <class READER>
	class SLIB_EXPORT BitReaderLE
	{
	public:
		READER reader;
		sl_uint32 bitNo;
		sl_uint8 byte;

	public:
		template <class T>
		BitReaderLE(T&& t): reader(Forward<T>(t))
		{
			bitNo = 8;
			byte = 0;
		}

	public:
		sl_uint8 read()
		{
			if (bitNo == 8) {
				byte = reader->readUint8();
				bitNo = 0;
			}
			sl_uint8 ret = byte & 1;
			bitNo++;
			byte >>= 1;
			return ret;
		}

		template <class T>
		sl_bool read(T& _out)
		{
			if (bitNo == 8) {
				if (reader->readUint8(&byte)) {
					bitNo = 0;
				} else {
					return sl_false;
				}
			}
			_out = (T)(byte & 1);
			bitNo++;
			byte >>= 1;
			return sl_true;
		}

	};

	template <class READER>
	class SLIB_EXPORT BitReaderBE
	{
	public:
		READER reader;
		sl_uint32 bitNo;
		sl_uint8 byte;

	public:
		template <class T>
		BitReaderBE(T&& t) : reader(Forward<T>(t))
		{
			bitNo = 8;
			byte = 0;
		}

	public:
		sl_uint8 read()
		{
			if (bitNo == 8) {
				byte = reader->readUint8();
				bitNo = 0;
			}
			sl_uint8 ret = byte >> 7;
			bitNo++;
			byte <<= 1;
			return ret;
		}

		template <class T>
		sl_bool read(T& _out)
		{
			if (bitNo == 8) {
				if (reader->readUint8(&byte)) {
					bitNo = 0;
				} else {
					return sl_false;
				}
			}
			_out = (T)(byte >> 7);
			bitNo++;
			byte <<= 1;
			return sl_true;
		}

	};

}

#endif
