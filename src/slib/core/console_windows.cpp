/*
 *   Copyright (c) 2008-2019 SLIBIO <https://github.com/SLIBIO>
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

#include "slib/core/definition.h"

#if defined(SLIB_PLATFORM_IS_WINDOWS)

#include "slib/core/console.h"

#include <stdio.h>
#include <conio.h>

namespace slib
{

	void Console::print(const StringParam& _s)
	{
#if defined(SLIB_PLATFORM_IS_WIN32)
		StringCstr16 s(_s);
		if (s.isEmpty()) {
			return;
		}
		Memory mem = Charsets::encode16(s.getData(), s.getLength() + 1, Charset::ANSI);
		printf("%s", (char*)(mem.getData()));
#endif
	}

	void Console::println(const StringParam& _s)
	{
#if defined(SLIB_PLATFORM_IS_WIN32)
		StringCstr16 s(_s);
		if (s.isEmpty()) {
			return;
		}
		Memory mem = Charsets::encode16(s.getData(), s.getLength() + 1, Charset::ANSI);
		printf("%s\n", (char*)(mem.getData()));
#endif
	}

#if defined(SLIB_PLATFORM_IS_WIN32)
	String Console::readLine()
	{
		char line[512];
		char* l = gets_s(line);
		line[511] = 0;
		return Charsets::decode8(Charset::ANSI, line, Base::getStringLength(line));
	}

	sl_char16 Console::readChar(sl_bool flagPrintEcho)
	{
		if (flagPrintEcho) {
			return (sl_char16)(_getche());
		} else {
			return (sl_char16)(_getch());
		}
	}
#endif

}

#endif