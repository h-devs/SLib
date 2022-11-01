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

#include "slib/core/console.h"

#include <stdio.h>

#if defined(SLIB_PLATFORM_IS_WIN32)
#	include <conio.h>
#   include "slib/core/memory.h"
#	include "slib/core/win32/windows.h"
#	define scanf scanf_s
#else
#	include <stdlib.h>
#	if !defined(SLIB_PLATFORM_IS_MOBILE)
#		include <termios.h>
#	endif
#endif

#include "slib/core/base.h"

namespace slib
{

	namespace priv
	{
		namespace console
		{
#if defined(SLIB_PLATFORM_IS_WIN32)
			void PrintConsole(const void* data, sl_size size)
			{
				HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
				if (handle) {
					DWORD dwWritten = 0;
					WriteFile(handle, data, (DWORD)size, &dwWritten, NULL);
				}
			}
#endif
		}
	}

	using namespace priv::console;

	void Console::print(const StringParam& _s)
	{
#if defined(SLIB_PLATFORM_IS_WIN32)
		StringCstr16 s(_s);
		if (s.isEmpty()) {
			return;
		}
		Memory mem = Charsets::encode16(s.getData(), s.getLength(), Charset::ANSI);
		PrintConsole(mem.getData(), mem.getSize());
#else
		StringCstr s(_s);
		printf("%s", s.getData());
#endif
	}

	void Console::println(const StringParam& _s)
	{
#if defined(SLIB_PLATFORM_IS_WIN32)
		StringCstr16 s(_s);
		if (s.isNotEmpty()) {
			sl_size len = Charsets::encode16(s.getData(), s.getLength(), Charset::ANSI, sl_null, -1);
			if (len) {
				Memory mem = Memory::create(len + 1);
				if (mem.isNotNull()) {
					Charsets::encode16(s.getData(), s.getLength(), Charset::ANSI, mem.getData(), len);
					((char*)(mem.getData()))[len] = '\n';
					PrintConsole(mem.getData(), mem.getSize());
				}
			}
			Memory mem = Charsets::encode16(s.getData(), s.getLength(), Charset::ANSI);
		} else {
			PrintConsole(L"\n", 1);
		}
#else
		StringCstr s(_s);
		printf("%s\n", s.getData());
#endif
	}

}

#if defined(SLIB_PLATFORM_IS_MOBILE)

namespace slib
{

	String Console::readLine()
	{
		return sl_null;
	}

	sl_char16 Console::readChar(sl_bool flagEcho)
	{
		return 0;
	}

	sl_bool Console::readInt32(sl_int32* _out)
	{
		return sl_false;
	}

	sl_bool Console::readUint32(sl_uint32* _out)
	{
		return sl_false;
	}

	sl_bool Console::readInt64(sl_int64* _out)
	{
		return sl_false;
	}

	sl_bool Console::readUint64(sl_uint64* _out)
	{
		return sl_false;
	}

	sl_bool Console::readFloat(float* _out)
	{
		return sl_false;
	}

	sl_bool Console::readDouble(double* _out)
	{
		return sl_false;
	}

	String Console::readString()
	{
		return sl_null;
	}

}

#else

namespace slib
{

	String Console::readLine()
	{
#if defined(SLIB_PLATFORM_IS_WIN32)
		char line[1024];
		char* l = gets_s(line, sizeof(line));
		sl_size len = Base::getStringLength(l);
		if (len) {
			return Charsets::decode8(Charset::ANSI, line, len).trimLine();
		}
		return String::getEmpty();
#else
		String ret;
		char* line = NULL;
		size_t len = 0;
		ssize_t nRet = getline(&line, &len, stdin);
		if (line) {
			if (nRet >= 0) {
				ret = StringView(line, Base::getStringLength(line, (sl_int32)len)).trimLine();
			}
			free(line);
		}
		return ret;
#endif
	}

	sl_char16 Console::readChar(sl_bool flagPrintEcho)
	{
#if defined(SLIB_PLATFORM_IS_WIN32)
		if (flagPrintEcho) {
			return (sl_char16)(_getche());
		} else {
			return (sl_char16)(_getch());
		}
#else
		termios tOld, tNew;
		tcgetattr(0, &tOld);
		tNew = tOld;
		tNew.c_lflag &= ~ICANON;
		if (flagPrintEcho) {
			tNew.c_lflag |= ECHO;
		} else {
			tNew.c_lflag &= ~ECHO;
		}
		tcsetattr(0, TCSANOW, &tNew);
		sl_char16 ch = (sl_char16)(::getchar());
		tcsetattr(0, TCSANOW, &tOld);
		return ch;
#endif
	}

	sl_bool Console::readInt32(sl_int32* _out)
	{
		int n = 0;
		if (scanf("%d", &n) == 1) {
			if (_out) {
				*_out = (sl_int32)n;
			}
			return sl_true;
		}
		return sl_false;
	}

	sl_bool Console::readUint32(sl_uint32* _out)
	{
		unsigned int n = 0;
		if (scanf("%u", &n) == 1) {
			if (_out) {
				*_out = (sl_uint32)n;
			}
			return sl_true;
		}
		return sl_false;
	}

	sl_bool Console::readInt64(sl_int64* _out)
	{
		sl_int64 n = 0;
#ifdef SLIB_PLATFORM_IS_WIN32
		if (scanf("%I64d", &n) == 1) {
#else
		if (scanf("%lld", &n) == 1) {
#endif
			if (_out) {
				*_out = n;
			}
			return sl_true;
		}
		return sl_false;
	}

	sl_bool Console::readUint64(sl_uint64* _out)
	{
		sl_uint64 n = 0;
#ifdef SLIB_PLATFORM_IS_WIN32
		if (scanf("%I64u", &n) == 1) {
#else
		if (scanf("%llu", &n) == 1) {
#endif
			if (_out) {
				*_out = n;
			}
			return sl_true;
		}
		return sl_false;
	}

	sl_bool Console::readFloat(float* _out)
	{
		float n = 0;
		if (scanf("%f", &n) == 1) {
			if (_out) {
				*_out = n;
			}
			return sl_true;
		}
		return sl_false;
	}

	sl_bool Console::readDouble(double* _out)
	{
		double n = 0;
		if (scanf("%lf", &n) == 1) {
			if (_out) {
				*_out = n;
			}
			return sl_true;
		}
		return sl_false;
	}

	String Console::readString()
	{
		char sz[1024];
		sz[0] = 0;
#if defined(SLIB_PLATFORM_IS_WIN32)
		if (scanf("%s", sz, (unsigned int)(sizeof(sz))) == 1) {
#else
		if (scanf("%s", sz) == 1) {
#endif
			sl_size len = Base::getStringLength(sz, sizeof(sz));
			if (len) {
#if defined(SLIB_PLATFORM_IS_WIN32)
				return Charsets::decode8(Charset::ANSI, sz, len).trimLine();
#else
				return String(sz, len);
#endif
			}
			return String::getEmpty();
		}
		return sz;
	}

}

#endif

namespace slib
{

	sl_int32 Console::readInt32(sl_int32 def)
	{
		sl_int32 ret;
		if (readInt32(&ret)) {
			return ret;
		} else {
			return def;
		}
	}

	sl_uint32 Console::readUint32(sl_uint32 def)
	{
		sl_uint32 ret;
		if (readUint32(&ret)) {
			return ret;
		} else {
			return def;
		}
	}

	sl_int64 Console::readInt64(sl_int64 def)
	{
		sl_int64 ret;
		if (readInt64(&ret)) {
			return ret;
		} else {
			return def;
		}
	}

	sl_uint64 Console::readUint64(sl_uint64 def)
	{
		sl_uint64 ret;
		if (readUint64(&ret)) {
			return ret;
		} else {
			return def;
		}
	}

	sl_bool Console::readInt(sl_reg* _out)
	{
#ifdef SLIB_ARCH_IS_64BIT
		return readInt64(_out);
#else
		return readInt32(_out);
#endif
	}

	sl_reg Console::readInt(sl_reg def)
	{
#ifdef SLIB_ARCH_IS_64BIT
		return readInt64(def);
#else
		return readInt32(def);
#endif
	}

	sl_bool Console::readUint(sl_size* _out)
	{
#ifdef SLIB_ARCH_IS_64BIT
		return readUint64(_out);
#else
		return readUint32(_out);
#endif
	}

	sl_size Console::readUint(sl_size def)
	{
#ifdef SLIB_ARCH_IS_64BIT
		return readUint64(def);
#else
		return readUint32(def);
#endif
	}

	float Console::readFloat(float def)
	{
		float ret;
		if (readFloat(&ret)) {
			return ret;
		} else {
			return def;
		}
	}

	double Console::readDouble(double def)
	{
		double ret;
		if (readDouble(&ret)) {
			return ret;
		} else {
			return def;
		}
	}

#if defined(SLIB_UI_IS_WIN32)
	sl_bool Console::open()
	{
		return AllocConsole() != 0;
	}

	sl_bool Console::close()
	{
		return FreeConsole() != 0;
	}
#else
	sl_bool Console::open()
	{
		return sl_false;
	}

	sl_bool Console::close()
	{
		return sl_false;
	}
#endif

}
