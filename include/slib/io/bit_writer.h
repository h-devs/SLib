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

#ifndef CHECKHEADER_SLIB_IO_BIT_WRITER
#define CHECKHEADER_SLIB_IO_BIT_WRITER

#include "definition.h"

#include "../core/cpp_helper.h"

namespace slib
{

	template <class WRITER>
	class SLIB_EXPORT BitWriterLE
	{
	public:
		WRITER writer;
		sl_uint32 bitNo;
		sl_uint8 byte;

	public:
		template <class T>
		BitWriterLE(T&& t): writer(Forward<T>(t))
		{
			bitNo = 0;
			byte = 0;
		}

		~BitWriterLE()
		{
			flush();
		}

	public:
		template <class T>
		sl_bool write(T bit)
		{
			sl_uint8 old = byte;
			if (bit) {
				byte = old | (1 << bitNo);
			}
			if (bitNo == 7) {
				if (writer->writeUint8(byte)) {
					bitNo = 0;
					byte = 0;
					return sl_true;
				} else {
					byte = old;
					return sl_false;
				}
			} else {
				bitNo++;
				return sl_true;
			}
		}

		sl_bool flush()
		{
			if (bitNo) {
				if (writer->writeUint8(byte)) {
					bitNo = 0;
					byte = 0;
					return sl_true;
				}
				return sl_false;
			} else {
				return sl_true;
			}
		}

	};

	template <class WRITER>
	class SLIB_EXPORT BitWriterBE
	{
	public:
		WRITER writer;
		sl_uint32 bitNo;
		sl_uint8 byte;

	public:
		template <class T>
		BitWriterBE(T&& t): writer(Forward<T>(t))
		{
			bitNo = 7;
			byte = 0;
		}

		~BitWriterBE()
		{
			flush();
		}

	public:
		template <class T>
		sl_bool write(T bit)
		{
			sl_uint8 old = byte;
			if (bit) {
				byte = old | (1 << bitNo);
			}
			if (bitNo) {
				bitNo--;
				return sl_true;
			} else {
				if (writer->writeUint8(byte)) {
					bitNo = 7;
					byte = 0;
					return sl_true;
				} else {
					byte = old;
					return sl_false;
				}
			}
		}

		sl_bool flush()
		{
			if (bitNo == 7) {
				return sl_true;
			} else {
				if (writer->writeUint8(byte)) {
					bitNo = 7;
					byte = 0;
				}
				return sl_false;
			}
		}

	};

}

#endif
