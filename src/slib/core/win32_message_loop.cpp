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

#include "slib/core/definition.h"

#if defined(SLIB_PLATFORM_IS_WIN32)

#include "slib/core/win32/message_loop.h"

#include "slib/core/thread.h"

namespace slib
{

	namespace priv
	{
		namespace win32_msg_loop
		{

			static LRESULT CALLBACK LoopWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
			{
				win32::MessageLoop* loop = (win32::MessageLoop*)(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
				if (loop) {
					LRESULT result = 0;
					if (loop->onMessage(uMsg, wParam, lParam, result)) {
						return result;
					}
				}
				return DefWindowProcW(hWnd, uMsg, wParam, lParam);
			}

		}
	}

	using namespace priv::win32_msg_loop;

	namespace win32
	{

		SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(MessageLoopParam)

			MessageLoopParam::MessageLoopParam()
		{
			flagAutoStart = sl_true;

			classStyle = 0;
			windowStyle = 0;
			extendedWindowStyle = 0;
			hWndParent = HWND_MESSAGE;
		}


		MessageLoop::MessageLoop()
		{
			m_hWnd = sl_null;
			m_flagRunning = sl_false;
		}

		MessageLoop::~MessageLoop()
		{
			stop();
		}

		Ref<MessageLoop> MessageLoop::create(const MessageLoopParam& param)
		{
			String16 name = param.name.toString16();
			if (name.isEmpty()) {
				return sl_null;
			}

			Ref<MessageLoop> ret = new MessageLoop;
			if (ret.isNotNull()) {

				ret->m_name = name;
				ret->m_onCreateWindow = param.onCreateWindow;
				ret->m_onMessage = param.onMessage;

				ret->m_styleClass = param.classStyle;
				ret->m_styleWindow = param.windowStyle;
				ret->m_styleWindowEx = param.extendedWindowStyle;
				ret->m_hWndParent = param.hWndParent;

				if (param.flagAutoStart) {
					ret->start();
				}

				return ret;
			}

			return sl_null;
		}

		void MessageLoop::start()
		{
			ObjectLocker lock(this);
			if (m_flagRunning) {
				return;
			}
			m_flagRunning = sl_true;
			m_thread = Thread::start(SLIB_FUNCTION_MEMBER(MessageLoop, _run, this));
			if (m_thread.isNull()) {
				m_flagRunning = sl_false;
			}
		}

		void MessageLoop::stop()
		{
			ObjectLocker lock(this);
			if (!m_flagRunning) {
				return;
			}
			HWND hWnd = m_hWnd;
			if (hWnd) {
				PostMessageW(hWnd, WM_QUIT, 0, 0);
				m_hWnd = sl_null;
			}
			m_tasks.removeAll();
			Thread* thread = m_thread.get();
			if (thread) {
				m_thread.setNull();
				if (thread->isRunning()) {
					thread->finishAndWait();
				}
			}
			m_flagRunning = sl_false;
		}

		sl_bool MessageLoop::isRunning()
		{
			return m_flagRunning;
		}

		sl_bool MessageLoop::dispatch(const Function<void()>& task, sl_uint64 delayMillis)
		{
			if (delayMillis) {
				return setTimeoutByDefaultDispatchLoop(task, delayMillis);
			}
			if (m_tasks.push(task)) {
				HWND hWnd = m_hWnd;
				if (hWnd) {
					PostMessageW(hWnd, WM_COMMAND, 0, 0);
				}
				return sl_true;
			}
			return sl_false;
		}

		void MessageLoop::setOnCreateWindow(const Function<void(HWND)>& callback)
		{
			m_onCreateWindow = callback;
		}

		HWND MessageLoop::getWindowHandle()
		{
			return m_hWnd;
		}

		sl_bool MessageLoop::onMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& result)
		{
			HWND hWnd = m_hWnd;
			if (!hWnd) {
				return sl_false;
			}
			if (!m_flagRunning) {
				return sl_false;
			}
			if (m_onMessage.isNotNull()) {
				return m_onMessage(uMsg, wParam, lParam, result);
			}
			return sl_false;
		}

		void MessageLoop::_run()
		{
			HINSTANCE hInstance = GetModuleHandleW(NULL);

			WNDCLASSEXW wc;
			Base::zeroMemory(&wc, sizeof(wc));
			wc.cbSize = sizeof(wc);
			wc.style = m_styleClass;
			wc.lpszClassName = (LPCWSTR)(m_name.getData());
			wc.lpfnWndProc = LoopWindowProc;
			wc.hInstance = hInstance;

			ATOM atom = RegisterClassExW(&wc);
			if (atom) {
				HWND hWnd = CreateWindowExW(m_styleWindowEx, (LPCWSTR)atom, L"", m_styleWindow, 0, 0, 0, 0, m_hWndParent, 0, 0, 0);
				if (hWnd) {
					ObjectLocker lock(this);
					if (Thread::isNotStoppingCurrent()) {
						SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)(this));
						m_hWnd = hWnd;
						lock.unlock();
						m_onCreateWindow(hWnd);
						if (_processTasks()) {
							MSG msg;
							while (GetMessageW(&msg, NULL, 0, 0)) {
								if (msg.message == WM_COMMAND) {
									if (!(_processTasks())) {
										break;
									}
								} else {
									DispatchMessageW(&msg);
								}
							}
						}
						m_hWnd = sl_null;
					}
					DestroyWindow(hWnd);
				}
				UnregisterClassW((LPCWSTR)atom, wc.hInstance);
			}
			m_flagRunning = sl_false;
		}

		sl_bool MessageLoop::_processTasks()
		{
			Function<void()> task;
			for (;;) {
				sl_size n = m_tasks.getCount();
				if (!n) {
					break;
				}
				for (sl_size i = 0; i < n; i++) {
					if (m_tasks.pop(&task)) {
						task();
					} else {
						break;
					}
				}
				MSG msg;
				if (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
					if (msg.message == WM_QUIT) {
						return sl_false;
					}
					if (msg.message != WM_COMMAND) {
						DispatchMessageW(&msg);
					}
				}
			}
			return sl_true;
		}

	}

}

#endif
