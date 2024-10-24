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

#ifndef CHECKHEADER_SLIB_UI_GLOBAL_EVENT_MONITOR
#define CHECKHEADER_SLIB_UI_GLOBAL_EVENT_MONITOR

#include "event.h"

#include "../core/function.h"
#include "../core/object.h"

/*
 [macOS] Input monitoring (for CGEventTap) or Accessibility (for NSEvent monitoring) authentication is required for global keyboard monitoring. See `Setting::isInputMonitoringEnabled()`, `Setting::isAccessibilityEnabled()`.
*/

namespace slib
{

	class SLIB_EXPORT GlobalEventMask
	{
	public:
		sl_bool flagKeyDown;
		sl_bool flagKeyUp;
		sl_bool flagLeftButtonDown;
		sl_bool flagLeftButtonUp;
		sl_bool flagLeftButtonDrag;
		sl_bool flagRightButtonDown;
		sl_bool flagRightButtonUp;
		sl_bool flagRightButtonDrag;
		sl_bool flagMiddleButtonDown;
		sl_bool flagMiddleButtonUp;
		sl_bool flagMiddleButtonDrag;
		sl_bool flagMouseMove;
		sl_bool flagMouseWheel;

	public:
		GlobalEventMask();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(GlobalEventMask)

	public:
		void setMouseEvents();

		void clearMouseEvents();

		void setKeyEvents();

		void clearKeyEvents();

	};

	class SLIB_EXPORT GlobalEventMonitorParam : public GlobalEventMask
	{
	public:
		sl_bool flagEventTap; // Used in macOS
		sl_bool flagSessionEventTap; // Used in macOS

		Function<void(UIEvent*)> onEvent;

	public:
		GlobalEventMonitorParam();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(GlobalEventMonitorParam)
	};

	class SLIB_EXPORT GlobalEventMonitor : public Object
	{
		SLIB_DECLARE_OBJECT

	public:
		GlobalEventMonitor();

		~GlobalEventMonitor();

	public:
		static Ref<GlobalEventMonitor> create(const GlobalEventMonitorParam& param);

		static Ref<GlobalEventMonitor> create(const Function<void(UIEvent*)>& onEvent, sl_bool flagKeyboard = sl_true, sl_bool flagMouse = sl_true);

	public:
		virtual void release() = 0;

	protected:
		void _initialize(const GlobalEventMonitorParam& param);

		void _onEvent(UIEvent* ev);

	protected:
		Function<void(UIEvent*)> m_onEvent;
	};

}

#endif
