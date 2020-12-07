/*
 *   Copyright (c) 2008-2018 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_CORE_CONSOLE
#define CHECKHEADER_SLIB_CORE_CONSOLE

#include "definition.h"

#include "string.h"

namespace slib
{
	
	class SLIB_EXPORT Console
	{
	public:
		static void print(const StringParam& s);

		template <class... ARGS>
		static void print(const StringParam& format, ARGS&&... args);
	
		static void println(const StringParam& s);

		template <class... ARGS>
		static void println(const StringParam& format, ARGS&&... args);
	
		static String readLine();

		static sl_char16 readChar(sl_bool flagPrintEcho = sl_false);

		static sl_bool readInt32(sl_int32* _out);

		static sl_int32 readInt32(sl_int32 def = 0);

		static sl_bool readUint32(sl_uint32* _out);

		static sl_uint32 readUint32(sl_uint32 def = 0);

		static sl_bool readInt64(sl_int64* _out);

		static sl_int64 readInt64(sl_int64 def = 0);

		static sl_bool readUint64(sl_uint64* _out);

		static sl_uint64 readUint64(sl_uint64 def = 0);

		static sl_bool readInt(sl_reg* _out);

		static sl_reg readInt(sl_reg def = 0);

		static sl_bool readUint(sl_size* _out);

		static sl_size readUint(sl_size def = 0);

		static sl_bool readFloat(float* _out);

		static float readFloat(float def = 0);

		static sl_bool readDouble(double* _out);

		static double readDouble(double def = 0);

		static String readString();

	};
	
	template <class... ARGS>
	void Printf(const StringParam& format, ARGS&&... args);
	
	template <class... ARGS>
	void Println(const StringParam& format, ARGS&&... args);
	
}

#include "detail/console.inc"

#endif
