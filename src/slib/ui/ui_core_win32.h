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

#ifndef CHECKHEADER_SLIB_UI_PLATFORM_WIN32
#define CHECKHEADER_SLIB_UI_PLATFORM_WIN32

#include "slib/ui/definition.h"

#if defined(SLIB_UI_IS_WIN32)

#include "slib/ui/core.h"

#include "slib/ui/platform.h"

namespace slib
{

	class Win32_UI_Shared
	{
	public:
		HINSTANCE hInstance;
		ATOM wndClassForView;
		HWND hWndMessage;

	private:
		ATOM m_wndClassForWindow;
		ATOM m_wndClassForWindowNoClose;
		ATOM m_wndClassForMessage;
		Mutex m_lock;

	public:
		Win32_UI_Shared();

		~Win32_UI_Shared();

	public:
		static Win32_UI_Shared* get();

		static void initialize();

	public:
		ATOM getWndClassForWindow();

		ATOM getWndClassForWindowNoClose();

	private:
		void prepareClassForView(WNDCLASSEXW& wc);

		void prepareClassForWindow(WNDCLASSEXW& wc);

	};

}

#define PRIV_SLIB_UI_GENERIC_WINDOW_CLASS_NAME L"FB4A9373-CA06-414D-B486-5FFC7FB13933"
#define PRIV_SLIB_UI_NOCLOSE_WINDOW_CLASS_NAME L"D9FF2361-B4CC-40D5-B55C-3D85DE89438F"
#define PRIV_SLIB_UI_VIEW_WINDOW_CLASS_NAME L"6F5719AC-5FAF-4F90-8A1D-FA35D3F24E87"
#define PRIV_SLIB_UI_MESSAGE_WINDOW_CLASS_NAME L"F619E22E-D761-4E18-987E-0458117D32E4"

#endif

#endif
