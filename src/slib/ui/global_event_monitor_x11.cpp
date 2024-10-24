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

#include "slib/core/definition.h"

#if defined(SLIB_PLATFORM_IS_LINUX_DESKTOP)

#include "slib/ui/global_event_monitor.h"

#include "slib/core/thread.h"
#include "slib/ui/core.h"
#include "slib/dl/linux/x11.h"

namespace slib
{

	namespace
	{
		class X11InputMonitor : public GlobalEventMonitor
		{
		public:
			Display* m_displayControl = sl_null;
			Display* m_displayRecord = sl_null;
			XRecordRange* m_range = sl_null;
			XRecordContext m_context = 0;
			sl_bool m_flagContextEnabled = sl_false;

			GlobalEventMask m_mask;
			Ref<Thread> m_thread;

		public:
			X11InputMonitor()
			{
			}

			~X11InputMonitor()
			{
				release();
			}

		public:
			static Ref<X11InputMonitor> create(const GlobalEventMonitorParam& param)
			{
				auto funcOpenDisplay = XOpenDisplay;
				if (!funcOpenDisplay) {
					return sl_null;
				}
				auto funcAllocRange = XRecordAllocRange;
				if (!funcAllocRange) {
					return sl_null;
				}
				sl_bool flagKeyboard = param.flagKeyDown || param.flagKeyUp;
				sl_bool flagMouse = param.flagLeftButtonDown || param.flagLeftButtonUp || param.flagRightButtonDown || param.flagRightButtonUp || param.flagMiddleButtonDown || param.flagMiddleButtonUp || param.flagMouseMove || param.flagMouseWheel;
				if (!flagKeyboard && !flagMouse) {
					return sl_null;
				}
				Display* displayControl = funcOpenDisplay(sl_null);
				if (!displayControl) {
					return sl_null;
				}
				Display* displayRecord = funcOpenDisplay(sl_null);
				if (displayRecord) {
					XRecordRange* range = funcAllocRange();
					if (range) {
						if (flagMouse) {
							if (flagKeyboard) {
								range->device_events.first = KeyPress;
							} else {
								range->device_events.first = ButtonPress;
								range->device_events.last = MotionNotify;
							}
							if (param.flagMouseMove) {
								range->device_events.last = MotionNotify;
							} else {
								range->device_events.last = ButtonRelease;
							}
						} else {
							range->device_events.first = KeyPress;
							range->device_events.last = KeyRelease;
						}
						XRecordClientSpec spec = XRecordAllClients;
						XRecordContext context = XRecordCreateContext(displayRecord, 0, &spec, 1, &range, 1);
						if (context) {
							Ref<X11InputMonitor> ret = new X11InputMonitor;
							if (ret.isNotNull()) {
								ret->_initialize(param);
								ret->m_mask = param;
								ret->m_displayControl = displayControl;
								ret->m_displayRecord = displayRecord;
								ret->m_range = range;
								ret->m_context = context;
								if (XRecordEnableContextAsync(displayRecord, context, &onEventCallback, (XPointer)this)) {
									ret->m_flagContextEnabled = sl_true;
									Ref<Thread> thread = Thread::start(SLIB_FUNCTION_WEAKREF(ret, onRun));
									if (thread.isNotNull()) {
										ret->m_thread = Move(thread);
										return ret;
									}
								}
								return sl_null;
							}
							XRecordFreeContext(displayRecord, context);
						}
						XFree(range);
					}
					XCloseDisplay(displayRecord);
				}
				XCloseDisplay(displayControl);
				return sl_null;
			}

		public:
			void release() override
			{
				ObjectLocker lock(this);
				Ref<Thread> thread = Move(m_thread);
				Display* displayControl = m_displayControl;
				m_displayControl = sl_null;
				Display* displayRecord = m_displayRecord;
				m_displayRecord = sl_null;
				XRecordRange* range = m_range;
				m_range = sl_null;
				XRecordContext context = m_context;
				m_context = 0;
				sl_bool flagContextEnabled = m_flagContextEnabled;
				m_flagContextEnabled = sl_false;
				lock.unlock();
				if (thread.isNotNull()) {
					thread->finishAndWait();
				}
				if (context) {
					if (flagContextEnabled) {
						XRecordDisableContext(displayControl, context);
						XFlush(displayControl);
					}
					XRecordFreeContext(displayRecord, context);
				}
				if (range) {
					XFree(range);
				}
				if (displayRecord) {
					XCloseDisplay(displayRecord);
				}
				if (displayControl) {
					XCloseDisplay(displayControl);
				}
			}

			void onRun()
			{
				CurrentThread thread;
				while (thread.isNotStopping()) {
					XRecordProcessReplies(m_displayRecord);
					thread.wait(10);
				}
			}

			void processKeyEvent(UIAction action, xEvent* event)
			{
				KeySym sym = XkbKeycodeToKeysym(m_display, event->u.u.detail, 0, 0);
				Keycode key = UIEvent::getKeycodeFromSystemKeycode(sym);
				Ref<UIEvent> ev = UIEvent::createKeyEvent(action, key, sym, Time::now());
				if (ev.isNotNull()) {
					_onEvent(ev.get());
				}
			}

			void processMouseEvent(UIAction action, xEvent* event)
			{
				sl_ui_posf x = (sl_ui_posf)(event->u.keyButtonPointer.rootX);
				sl_ui_posf y = (sl_ui_posf)(event->u.keyButtonPointer.rootY);
				Ref<UIEvent> ev = UIEvent::createMouseEvent(action, x, y, Time::now());
				if (ev.isNotNull()) {
					_onEvent(ev.get());
				}
			}

			void processMouseWheelEvent(xEvent* event, sl_int32 dx, sl_int32 dy)
			{
				sl_ui_posf x = (sl_ui_posf)(event->u.keyButtonPointer.rootX);
				sl_ui_posf y = (sl_ui_posf)(event->u.keyButtonPointer.rootY);
				sl_int32 delta = 3;
				Ref<UIEvent> ev = UIEvent::createMouseWheelEvent(x, y, (sl_real)(dx * delta), (sl_real)(dy * delta), Time::now());
				if (ev.isNotNull()) {
					_onEvent(ev.get());
				}
			}

			void processEvent(XRecordInterceptData* data)
			{
				if (data->category != XRecordFromServer) {
					return;
				}
				if (data->client_swapped) {
					return;
				}
				xEvent* event = (xEvent*)(data->data);
				BYTE type = event->u.u.type;
				switch (type) {
					case KeyPress:
						if (m_mask.flagKeyDown) {
							processKeyEvent(UIAction::KeyDown, event);
						}
						break;
					case KeyRelease:
						if (m_mask.flagKeyUp) {
							processKeyEvent(UIAction::KeyUp, event);
						}
						break;
					case ButtonPress:
						switch (event->u.u.detail) {
							case 1:
								if (m_mask.flagLeftButtonDown) {
									processMouseEvent(UIAction::LeftButtonDown, event);
								}
								break;
							case 2:
								if (m_mask.flagMiddleButtonDown) {
									processMouseEvent(UIAction::MiddleButtonDown, event);
								}
								break;
							case 3:
								if (m_mask.flagRightButtonDown) {
									processMouseEvent(UIAction::RightButtonDown, event);
								}
								break;
							case 4:
								if (m_mask.flagMouseWheel) {
									processMouseWheelEvent(event, 0, -1);
								}
								break;
							case 5:
								if (m_mask.flagMouseWheel) {
									processMouseWheelEvent(event, 0, 1);
								}
								break;
							case 6:
								if (m_mask.flagMouseWheel) {
									processMouseWheelEvent(event, -1, 0);
								}
								break;
							case 7:
								if (m_mask.flagMouseWheel) {
									processMouseWheelEvent(event, 1, 0);
								}
								break;
							default:
								break;
						}
						break;
					case ButtonRelease:
						switch (event->u.u.detail) {
							case 1:
								if (m_mask.flagLeftButtonUp) {
									processMouseEvent(UIAction::LeftButtonUp, event);
								}
								break;
							case 2:
								if (m_mask.flagMiddleButtonUp) {
									processMouseEvent(UIAction::MiddleButtonUp, event);
								}
								break;
							case 3:
								if (m_mask.flagRightButtonUp) {
									processMouseEvent(UIAction::RightButtonUp, event);
								}
								break;
							default:
								break;
						}
						break;
					case MotionNotify:
						if (m_mask.flagMouseMove) {
							processMouseEvent(UIAction::MouseMove, event);
						}
						break;
					default:
						break;
				}
			}

			static void onEventCallback(XPointer closure, XRecordInterceptData* data)
			{
				((X11InputMonitor*)closure)->processEvent(data);
				XRecordFreeData(data);
			}
		};
	}

	Ref<GlobalEventMonitor> GlobalEventMonitor::create(const GlobalEventMonitorParam& param)
	{
		return Ref<GlobalEventMonitor>::cast(X11InputMonitor::create(param));
	}

}

#endif
