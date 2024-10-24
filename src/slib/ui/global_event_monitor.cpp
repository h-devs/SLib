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

#include "slib/ui/global_event_monitor.h"

namespace slib
{

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(GlobalEventMask)

	GlobalEventMask::GlobalEventMask()
	{
		clearKeyEvents();
		clearMouseEvents();
	}

	void GlobalEventMask::setMouseEvents()
	{
		flagLeftButtonDown = sl_true;
		flagLeftButtonUp = sl_true;
		flagLeftButtonDrag = sl_true;
		flagRightButtonDown = sl_true;
		flagRightButtonUp = sl_true;
		flagRightButtonDrag = sl_true;
		flagMiddleButtonDown = sl_true;
		flagMiddleButtonUp = sl_true;
		flagMiddleButtonDrag = sl_true;
		flagMouseMove = sl_true;
		flagMouseWheel = sl_true;
	}

	void GlobalEventMask::clearMouseEvents()
	{
		flagLeftButtonDown = sl_false;
		flagLeftButtonUp = sl_false;
		flagLeftButtonDrag = sl_false;
		flagRightButtonDown = sl_false;
		flagRightButtonUp = sl_false;
		flagRightButtonDrag = sl_false;
		flagMiddleButtonDown = sl_false;
		flagMiddleButtonUp = sl_false;
		flagMiddleButtonDrag = sl_false;
		flagMouseMove = sl_false;
		flagMouseWheel = sl_false;
	}

	void GlobalEventMask::setKeyEvents()
	{
		flagKeyDown = sl_true;
		flagKeyUp = sl_true;
	}

	void GlobalEventMask::clearKeyEvents()
	{
		flagKeyDown = sl_false;
		flagKeyUp = sl_false;
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(GlobalEventMonitorParam)

	GlobalEventMonitorParam::GlobalEventMonitorParam()
	{
		flagEventTap = sl_true;
		flagSessionEventTap = sl_false;
	}


	SLIB_DEFINE_OBJECT(GlobalEventMonitor, Object)

	GlobalEventMonitor::GlobalEventMonitor()
	{
	}

	GlobalEventMonitor::~GlobalEventMonitor()
	{
	}

	Ref<GlobalEventMonitor> GlobalEventMonitor::create(const Function<void(UIEvent*)>& onEvent, sl_bool flagKeyboard, sl_bool flagMouse)
	{
		GlobalEventMonitorParam param;
		if (flagKeyboard) {
			param.setKeyEvents();
		}
		if (flagMouse) {
			param.setMouseEvents();
		}
		param.onEvent = onEvent;
		return create(param);
	}

	void GlobalEventMonitor::_initialize(const GlobalEventMonitorParam& param)
	{
		m_onEvent = param.onEvent;
	}

	void GlobalEventMonitor::_onEvent(UIEvent* ev)
	{
		m_onEvent(ev);
	}

#if !defined(SLIB_PLATFORM_IS_WIN32) && !defined(SLIB_PLATFORM_IS_MACOS) && !defined(SLIB_PLATFORM_IS_LINUX_DESKTOP)
	Ref<GlobalEventMonitor> GlobalEventMonitor::create(const GlobalEventMonitorParam& param)
	{
		return sl_null;
	}
#endif

}
