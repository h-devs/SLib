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

#include "slib/core/win32_message_loop.h"

#include "slib/core/thread.h"

namespace slib
{

	namespace priv
	{
		namespace win32_msg_loop
		{

			static LRESULT CALLBACK LoopWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
			{
				Win32MessageLoop* loop = (Win32MessageLoop*)(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
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

	Win32MessageLoop::Win32MessageLoop()
	{
		m_hWnd = sl_null;
		m_flagRunning = sl_false;
	}

	Win32MessageLoop::~Win32MessageLoop()
	{
		release();
	}

	Ref<Win32MessageLoop> Win32MessageLoop::create(const StringParam& _name, const Function<sl_bool(UINT, WPARAM, LPARAM, LRESULT&)>& handler, sl_bool flagAutoStart)
	{
		String16 name = _name.toString16();
		if (name.isEmpty()) {
			return sl_null;
		}
		Ref<Win32MessageLoop> ret = new Win32MessageLoop;
		if (ret.isNotNull()) {
			ret->m_name = name;
			ret->m_handler = handler;
			if (flagAutoStart) {
				ret->start();
			}
			return ret;
		}
		return sl_null;
	}

	void Win32MessageLoop::release()
	{
		ObjectLocker lock(this);
		HWND hWnd = m_hWnd;
		if (hWnd) {
			PostMessageW(hWnd, WM_QUIT, 0, 0);
		}
		if (m_thread.isNotNull()) {
			m_thread->finish();
			lock.unlock();
			m_thread->finishAndWait();
		}
	}

	void Win32MessageLoop::start()
	{
		ObjectLocker lock(this);
		if (m_thread.isNull()) {
			m_thread = Thread::start(SLIB_FUNCTION_MEMBER(Win32MessageLoop, _run, this));
			if (m_thread.isNotNull()) {
				m_flagRunning = sl_true;
			}
		}
	}

	sl_bool Win32MessageLoop::isRunning()
	{
		return m_flagRunning;
	}

	sl_bool Win32MessageLoop::dispatch(const Function<void()>& task, sl_uint64 delay_ms)
	{
		HWND hWnd = m_hWnd;
		if (!hWnd) {
			return sl_false;
		}
		if (delay_ms) {
			WeakRef<Win32MessageLoop> thiz = this;
			return Dispatch::setTimeout([this, thiz, task]() {
				Ref<Win32MessageLoop> ref = thiz;
				if (ref.isNull()) {
					return;
				}
				dispatch(task);
			}, delay_ms);
		} else {
			if (m_tasks.push(task)) {
				if (PostMessageW(hWnd, WM_COMMAND, 0, 0)) {
					return sl_true;
				}
			}
			return sl_false;
		}
	}

	HWND Win32MessageLoop::getWindowHandle()
	{
		return m_hWnd;
	}

	sl_bool Win32MessageLoop::onMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& result)
	{
		HWND hWnd = m_hWnd;
		if (!hWnd) {
			return sl_false;
		}
		if (uMsg == WM_COMMAND) {
			Function<void()> task;
			sl_size n = m_tasks.getCount();
			for (sl_size i = 0; i < n; i++) {
				if (m_tasks.pop(&task)) {
					task();
				} else {
					break;
				}
			}
		}
		return m_handler(uMsg, wParam, lParam, result);
	}

	void Win32MessageLoop::_run()
	{
		HINSTANCE hInstance = GetModuleHandleW(NULL);

		WNDCLASSEXW wc;
		Base::zeroMemory(&wc, sizeof(wc));
		wc.cbSize = sizeof(wc);
		wc.lpszClassName = (LPCWSTR)(m_name.getData());
		wc.lpfnWndProc = LoopWindowProc;
		wc.hInstance = hInstance;

		ATOM atom = RegisterClassExW(&wc);
		if (atom) {
			HWND hWnd = CreateWindowW((LPCWSTR)atom, L"", 0, 0, 0, 0, 0, HWND_MESSAGE, 0, 0, 0);
			if (hWnd) {
				ObjectLocker lock(this);
				if (Thread::isNotStoppingCurrent()) {
					SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)(this));
					m_hWnd = hWnd;
					lock.unlock();
					MSG msg;
					while (GetMessageW(&msg, NULL, 0, 0)) {
						DispatchMessageW(&msg);
					}
					m_hWnd = sl_null;
				}
				DestroyWindow(hWnd);
			}
			UnregisterClassW((LPCWSTR)atom, wc.hInstance);
		}
		m_flagRunning = sl_false;
	}

}

#endif
