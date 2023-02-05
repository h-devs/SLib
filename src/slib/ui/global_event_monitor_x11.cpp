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

#if defined(SLIB_PLATFORM_IS_LINUX_DESKTOP)

#include "slib/ui/global_event_monitor.h"

#include "slib/core/thread.h"
#include "slib/core/safe_static.h"
#include "slib/dl/linux/x11.h"

namespace slib
{

	namespace {

		class GlobalEventMonitorHelper : public GlobalEventMonitor
		{
		public:
			using GlobalEventMonitor::_onEvent;

			friend class Monitor;
		};

		class Monitor
		{
		public:
			Ref<Thread> m_thread;
			sl_uint32 m_mask;

			Display* m_display;

		public:
			Monitor()
			{
				m_mask = 0;
			}

			~Monitor()
			{
				if (m_thread.isNotNull()) {
					m_thread->finishAndWait();
				}
			}

		public:
			sl_bool update(sl_uint32 mask)
			{
				if (mask != m_mask) {
					if (m_thread.isNotNull()) {
						m_thread->finishAndWait();
						m_thread.setNull();
					}
					if (mask) {
						m_thread = Thread::start(SLIB_FUNCTION_MEMBER(this, run));
						if (m_thread.isNull()) {
							return sl_false;
						}
					}
					m_mask = mask;
					return sl_true;
				} else {
					return sl_true;
				}
			}

			void run()
			{
				Thread* thread = Thread::getCurrent();
				if (!thread) {
					return;
				}
				Display* displayControl = XOpenDisplay(sl_null);
				if (displayControl) {
					m_display = displayControl;
					Display* displayRecord = XOpenDisplay(sl_null);
					if (displayRecord) {
						XSynchronize(displayControl, 1);
						XRecordRange* range = XRecordAllocRange();
						if (range) {
							if (m_mask & GlobalEventMonitorHelper::MASK_MOUSE) {
								if (m_mask & GlobalEventMonitorHelper::MASK_KEYBOARD) {
									range->device_events.first = KeyPress;
									range->device_events.last = MotionNotify;
								} else {
									range->device_events.first = ButtonPress;
									range->device_events.last = MotionNotify;
								}
							} else {
								range->device_events.first = KeyPress;
								range->device_events.last = KeyRelease;
							}
							XRecordClientSpec spec = XRecordAllClients;
							XRecordContext context = XRecordCreateContext(displayRecord, 0, &spec, 1, &range, 1);
							if (context) {
								if (XRecordEnableContextAsync(displayRecord, context, &callbackEvent, (XPointer)this)) {
									while (thread->isNotStopping()) {
										XRecordProcessReplies(displayRecord);
										thread->wait(10);
									}
									XRecordDisableContext(displayControl, context);
								}
								XRecordFreeContext(displayRecord, context);
							}
							XFree(range);
						}
						XCloseDisplay(displayRecord);
					}
					XCloseDisplay(displayControl);
				}
			}

			static void callbackEvent(XPointer closure, XRecordInterceptData* data)
			{
				((Monitor*)closure)->onEvent(data);
			}

			void onEvent(XRecordInterceptData* hook)
			{
				if (hook->category == XRecordFromServer && !(hook->client_swapped)) {
					xEvent* data = (xEvent*)(hook->data);
					BYTE type = data->u.u.type;
					switch (type) {
						case KeyPress:
						case KeyRelease:
							{
								KeySym sym = XkbKeycodeToKeysym(m_display, data->u.u.detail, 0, 0);
								Keycode key = UIEvent::getKeycodeFromSystemKeycode(sym);
								Ref<UIEvent> ev = UIEvent::createKeyEvent(type == KeyPress ? UIAction::KeyDown : UIAction::KeyUp, key, sym, Time::now());
								if (ev.isNotNull()) {
									GlobalEventMonitorHelper::_onEvent(ev.get());
								}
							}
							break;
						case ButtonPress:
						case ButtonRelease:
						case MotionNotify:
							{
								UIAction action;
								BYTE button = data->u.u.detail;
								if (type == MotionNotify) {
									action = UIAction::MouseMove;
								} else {
									if (button == 1) {
										if (type == ButtonPress) {
											action = UIAction::LeftButtonDown;
										} else {
											action = UIAction::LeftButtonUp;
										}
									} else if (button == 2) {
										if (type == ButtonPress) {
											action = UIAction::MiddleButtonDown;
										} else {
											action = UIAction::MiddleButtonUp;
										}
									} else {
										if (type == ButtonPress) {
											action = UIAction::RightButtonDown;
										} else {
											action = UIAction::RightButtonUp;
										}
									}
								}
								sl_ui_posf x = (sl_ui_posf)(data->u.keyButtonPointer.rootX);
								sl_ui_posf y = (sl_ui_posf)(data->u.keyButtonPointer.rootY);
								Ref<UIEvent> ev = UIEvent::createMouseEvent(action, x, y, Time::now());
								if (ev.isNotNull()) {
									GlobalEventMonitorHelper::_onEvent(ev.get());
								}
							}
							break;
					}
				}
				XRecordFreeData(hook);
			}

		};

		SLIB_SAFE_STATIC_GETTER(Monitor, GetMonitor)

	}

	sl_bool GlobalEventMonitor::_updateMonitor(sl_uint32 mask)
	{
		Monitor* context = GetMonitor();
		if (!context) {
			return sl_false;
		}
		return context->update(mask);
	}

}

#endif
