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

#ifndef CHECKHEADER_SLIB_CORE_WIN32_MESSAGE_LOOP
#define CHECKHEADER_SLIB_CORE_WIN32_MESSAGE_LOOP

#include "../definition.h"

#ifdef SLIB_PLATFORM_IS_WIN32

#include "../windows.h"
#include "../dispatch.h"
#include "../queue.h"
#include "../string.h"

namespace slib
{

	class Thread;

	namespace win32
	{

		class SLIB_EXPORT MessageLoopParam
		{
		public:
			StringParam name;
			Function<void(HWND)> onCreateWindow;
			Function<sl_bool(UINT, WPARAM, LPARAM, LRESULT&)> onMessage;
			sl_bool flagAutoStart;

			UINT classStyle;
			UINT windowStyle;
			UINT extendedWindowStyle;
			HWND hWndParent;

		public:
			MessageLoopParam();

			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(MessageLoopParam)

		};

		class SLIB_EXPORT MessageLoop : public Dispatcher
		{
		protected:
			MessageLoop();

			~MessageLoop();

		public:
			static Ref<MessageLoop> create(const MessageLoopParam& param);

		public:
			void start();

			void stop();

			sl_bool isRunning();

			sl_bool dispatch(const Function<void()>& task, sl_uint64 delayMillis = 0) override;

		public:
			void setOnCreateWindow(const Function<void(HWND)>& callback);

			HWND getWindowHandle();

		public:
			virtual sl_bool onMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& result);

		protected:
			void _run();

			sl_bool _processTasks();

		public:
			String16 m_name;
			AtomicFunction<void(HWND)> m_onCreateWindow;
			Function<sl_bool(UINT, WPARAM, LPARAM, LRESULT&)> m_onMessage;

			UINT m_styleClass;
			UINT m_styleWindow;
			UINT m_styleWindowEx;
			HWND m_hWndParent;

			HWND m_hWnd;
			sl_bool m_flagRunning;
			Ref<Thread> m_thread;
			Queue< Function<void()> > m_tasks;

		};
	
	}

}

#endif

#endif