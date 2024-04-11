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

#ifndef CHECKHEADER_SLIB_EXTRA_WINHOOK_HOOK
#define CHECKHEADER_SLIB_EXTRA_WINHOOK_HOOK

#include <slib/core/function.h>
#include <slib/ui/event.h>

namespace slib
{

	typedef Function<void(UIEvent*)> HookInputCallback;

	class SLIB_EXPORT HookInputParam
	{
	public:
		HookInputCallback onInput;
		sl_bool flagBlockKeyboard;

	public:
		HookInputParam();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(HookInputParam)

	};

	class SLIB_EXPORT HookInput
	{
	public:
		static String getDllPath();

		static void setDllPath(const String& dir, const String& fileName);

		static void setDllDirectory(const String& dir);

		static void setDllName(const String& fileName);

		static sl_bool install();

		static sl_bool start(const HookInputParam& param);

		static sl_bool start(const HookInputCallback& callback);

		static void stop();

	};

}

#endif
