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

#ifndef CHECKHEADER_SLIB_EXTRA_INPUT_SENDER
#define CHECKHEADER_SLIB_EXTRA_INPUT_SENDER

#include <slib/ui/constants.h>
#include <slib/core/function.h>

namespace slib
{

	class InputSender
	{
	public:
		static sl_bool prepare(const StringParam& serviceName);

		static void sendKeyEvent(UIAction action, Keycode key);

		static void sendMouseEvent(UIAction action, sl_ui_pos x, sl_ui_pos y, sl_bool flagAbsolutePos = sl_true);


		static void setServiceName(const StringParam& serviceName);

		static void setOnStartService(const Function<void()>& callback);

		static void setOnStopService(const Function<void()>& callback);

		static void setOnInstallService(const Function<void()>& callback);

		static void setOnCheckInstall(const Function<sl_bool()>& callback);

		static void setOnStartAgent(const Function<void()>& callback);

		static void setOnStopAgent(const Function<void()>& callback);


		static void onStartService();

		static void onStopService();

		static void runAgent();

	};

}

#endif
