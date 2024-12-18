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

#ifndef CHECKHEADER_SLIB_CORE_COMMAND_LINE
#define CHECKHEADER_SLIB_CORE_COMMAND_LINE

#include "string.h"

namespace slib
{

	class SLIB_EXPORT CommandLine
	{
	public:
		static List<String> parse(const StringParam& commandLine);

		static List<String> parseForWin32(const StringParam& commandLine);

		static List<String> parseForUnix(const StringParam& commandLine);

		static String makeSafeArgument(const StringParam& arg);

		static String makeSafeArgumentForWin32(const StringParam& arg);

		static String makeSafeArgumentForUnix(const StringParam& arg);

		static String build(const StringParam* argv, sl_size argc);
		static String build(const String* argv, sl_size argc);

		template <class... ARGS>
		static String build(const StringParam& arg0, const StringParam& arg1, ARGS&&... args)
		{
			StringParam params[] = { arg0, arg1, Forward<ARGS>(args)... };
			return build(params, 2 + sizeof...(args));
		}

		static String buildForWin32(const StringParam* argv, sl_size argc);
		static String buildForWin32(const String* argv, sl_size argc);

		template <class... ARGS>
		static String buildForWin32(const StringParam& arg0, const StringParam& arg1, ARGS&&... args)
		{
			StringParam params[] = { arg0, arg1, Forward<ARGS>(args)... };
			return buildForWin32(params, 2 + sizeof...(args));
		}

		static String buildForUnix(const StringParam* argv, sl_size argc);
		static String buildForUnix(const String* argv, sl_size argc);

		template <class... ARGS>
		static String buildForUnix(const StringParam& arg0, const StringParam& arg1, ARGS&&... args)
		{
			StringParam params[] = { arg0, arg1, Forward<ARGS>(args)... };
			return buildForUnix(params, 2 + sizeof...(args));
		}

		static String build(const StringParam& executable, const StringParam* argv = sl_null, sl_size argc = 0);

		static String buildForWin32(const StringParam& executable, const StringParam* argv = sl_null, sl_size argc = 0);

		static String buildForUnix(const StringParam& executable, const StringParam* argv = sl_null, sl_size argc = 0);

	};

}

#endif
