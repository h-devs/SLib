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

#if defined(SLIB_PLATFORM_IS_MACOS)

#include "slib/ui/global_event_monitor.h"

#include "slib/core/thread.h"
#include "slib/ui/core.h"
#include "slib/ui/platform.h"

namespace slib
{

	namespace
	{
		static sl_bool IsInjectedEvent(CGEventRef ev)
		{
			CGEventSourceRef source = CGEventCreateSourceFromEvent(ev);
			if (source) {
				sl_bool bRet;
				if (CGEventSourceGetSourceStateID(source) != kCGEventSourceStateHIDSystemState) {
					bRet = sl_true;
				} else {
					bRet = sl_false;
				}
				CFRelease(source);
				return bRet;
			} else {
				return sl_true;
			}
		}

		class CGEventTapMonitor : public GlobalEventMonitor
		{
		public:
			CFMachPortRef m_tap = sl_null;
			CFRunLoopSourceRef m_loopSource = sl_null;

			Ref<Thread> m_thread;

		public:
			CGEventTapMonitor()
			{
			}

			~CGEventTapMonitor()
			{
				release();
			}

		public:
			static Ref<CGEventTapMonitor> create(const GlobalEventMonitorParam& param)
			{
				CGEventMask mask = 0;
				if (param.flagKeyDown) {
					mask |= CGEventMaskBit(kCGEventKeyDown);
				}
				if (param.flagKeyUp) {
					mask |= CGEventMaskBit(kCGEventKeyUp);
				}
				if (param.flagLeftButtonDown) {
					mask |= CGEventMaskBit(kCGEventLeftMouseDown);
				}
				if (param.flagLeftButtonUp) {
					mask |= CGEventMaskBit(kCGEventLeftMouseUp);
				}
				if (param.flagLeftButtonDrag) {
					mask |= CGEventMaskBit(kCGEventLeftMouseDragged);
				}
				if (param.flagRightButtonDown) {
					mask |= CGEventMaskBit(kCGEventRightMouseDown);
				}
				if (param.flagRightButtonUp) {
					mask |= CGEventMaskBit(kCGEventRightMouseUp);
				}
				if (param.flagRightButtonDrag) {
					mask |= CGEventMaskBit(kCGEventRightMouseDragged);
				}
				if (param.flagMiddleButtonDown) {
					mask |= CGEventMaskBit(kCGEventOtherMouseDown);
				}
				if (param.flagMiddleButtonUp) {
					mask |= CGEventMaskBit(kCGEventOtherMouseUp);
				}
				if (param.flagMiddleButtonDrag) {
					mask |= CGEventMaskBit(kCGEventOtherMouseDragged);
				}
				if (param.flagMouseMove) {
					mask |= CGEventMaskBit(kCGEventMouseMoved);
				}
				if (param.flagMouseWheel) {
					mask |= CGEventMaskBit(kCGEventScrollWheel);
				}
				if (!mask) {
					return sl_null;
				}
				Ref<CGEventTapMonitor> ret = new CGEventTapMonitor;
				if (ret.isNull()) {
					return sl_null;
				}
				CFMachPortRef tap = CGEventTapCreate(param.flagSessionEventTap ? kCGSessionEventTap : kCGHIDEventTap, kCGHeadInsertEventTap, kCGEventTapOptionListenOnly, mask, &onEventCallback, ret.get());
				if (!tap) {
					return sl_null;
				}
				CFRunLoopSourceRef loopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, tap, 0);
				if (loopSource) {
					ret->_initialize(param);
					ret->m_tap = tap;
					ret->m_loopSource = loopSource;
					Ref<Thread> thread = Thread::start(SLIB_FUNCTION_WEAKREF(ret, onRunLoop));
					if (thread.isNotNull()) {
						ret->m_thread = Move(thread);
						return ret;
					}
					return sl_null;
				}
				CFRelease(tap);
				return sl_null;
			}

		public:
			void release() override
			{
				ObjectLocker lock(this);
				Ref<Thread> thread = Move(m_thread);
				CFMachPortRef tap = m_tap;
				m_tap = sl_null;
				CFRunLoopSourceRef loopSource = m_loopSource;
				m_loopSource = sl_null;
				lock.unlock();
				if (thread.isNotNull()) {
					thread->finishAndWait();
				}
				if (loopSource) {
					CFRelease(loopSource);
				}
				if (tap) {
					CFRelease(tap);
				}
			}

			void onRunLoop()
			{
				CFRunLoopRef loop = CFRunLoopGetCurrent();
				if (!loop) {
					return;
				}
				CFRunLoopAddSource(loop, m_loopSource, kCFRunLoopCommonModes);
				CGEventTapEnable(m_tap, true);

				CurrentThread thread;
				while (thread.isNotStopping()) {
					@autoreleasepool {
						CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.01, NO);
					}
				}
			}

			void prepareEvent(UIEvent* ev, CGEventRef event)
			{
				Time t;
				t.setSecondCountF(CGEventGetTimestamp(event) / 1000000000.0);
				CGEventFlags flags = CGEventGetFlags(event);
				if (flags & kCGEventFlagMaskControl) {
					ev->setControlKey();
				}
				if (flags & kCGEventFlagMaskAlternate) {
					ev->setAltKey();
				}
				if (flags & kCGEventFlagMaskCommand) {
					ev->setCommandKey();
				}
				if (flags & kCGEventFlagMaskShift) {
					ev->setShiftKey();
				}
				if (IsInjectedEvent(event)) {
					ev->addFlag(UIEventFlags::Injected);
				}
			}

			void processKeyEvent(UIAction action, CGEventRef event)
			{
				sl_uint32 vkey = (sl_uint32)(CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode));
				Keycode key = UIEvent::getKeycodeFromSystemKeycode(vkey);
				Ref<UIEvent> ev = UIEvent::createKeyEvent(action, key, vkey, Time::zero());
				if (ev.isNotNull()) {
					prepareEvent(ev.get(), event);
					_onEvent(ev.get());
				}
			}

			void processMouseEvent(UIAction action, CGEventRef event)
			{
				CGPoint pt = CGEventGetLocation(event);
				sl_ui_posf x = (sl_ui_posf)(pt.x);
				sl_ui_posf y = (sl_ui_posf)(pt.y);
				Ref<UIEvent> ev = UIEvent::createMouseEvent(action, x, y, Time::zero());
				if (ev.isNotNull()) {
					prepareEvent(ev.get(), event);
					_onEvent(ev.get());
				}
			}

			void processMouseWheelEvent(CGEventRef event)
			{
				sl_int32 deltaX = (sl_int32)(CGEventGetIntegerValueField(event, kCGScrollWheelEventDeltaAxis2));
				sl_int32 deltaY = (sl_int32)(CGEventGetIntegerValueField(event, kCGScrollWheelEventDeltaAxis1));
				if (!deltaX && !deltaY) {
					return;
				}
				CGPoint pt = CGEventGetLocation(event);
				sl_ui_posf x = (sl_ui_posf)(pt.x);
				sl_ui_posf y = (sl_ui_posf)(pt.y);
				Ref<UIEvent> ev = UIEvent::createMouseWheelEvent(x, y, (sl_real)deltaX, (sl_real)deltaY, Time::zero());
				if (ev.isNotNull()) {
					prepareEvent(ev.get(), event);
					_onEvent(ev.get());
				}
			}

			void processEvent(CGEventType type, CGEventRef ev)
			{
				switch (type) {
					case kCGEventKeyDown:
						processKeyEvent(UIAction::KeyDown, ev);
						break;
					case kCGEventKeyUp:
						processKeyEvent(UIAction::KeyUp, ev);
						break;
					case kCGEventLeftMouseDown:
						processMouseEvent(UIAction::LeftButtonDown, ev);
						break;
					case kCGEventLeftMouseUp:
						processMouseEvent(UIAction::LeftButtonUp, ev);
						break;
					case kCGEventLeftMouseDragged:
						processMouseEvent(UIAction::LeftButtonDrag, ev);
						break;
					case kCGEventRightMouseDown:
						processMouseEvent(UIAction::RightButtonDown, ev);
						break;
					case kCGEventRightMouseUp:
						processMouseEvent(UIAction::RightButtonUp, ev);
						break;
					case kCGEventRightMouseDragged:
						processMouseEvent(UIAction::RightButtonDrag, ev);
						break;
					case kCGEventOtherMouseDown:
						processMouseEvent(UIAction::MiddleButtonDown, ev);
						break;
					case kCGEventOtherMouseUp:
						processMouseEvent(UIAction::MiddleButtonUp, ev);
						break;
					case kCGEventOtherMouseDragged:
						processMouseEvent(UIAction::MiddleButtonDrag, ev);
						break;
					case kCGEventMouseMoved:
						processMouseEvent(UIAction::MouseMove, ev);
						break;
					case kCGEventScrollWheel:
						processMouseWheelEvent(ev);
						break;
					case kCGEventTapDisabledByTimeout:
						CGEventTapEnable(m_tap, true);
						break;
					default:
						break;
				}
			}

			static CGEventRef onEventCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef ev, void* user)
			{
				CGEventTapMonitor* monitor = (CGEventTapMonitor*)user;
				monitor->processEvent(type, ev);
				return ev;
			}
		};

		class NSEventMonitor : public GlobalEventMonitor
		{
		public:
			id m_monitorGlobal = nil;
			id m_monitorLocal = nil;

		public:
			NSEventMonitor()
			{
			}

			~NSEventMonitor()
			{
				release();
			}

		public:
			static Ref<NSEventMonitor> create(const GlobalEventMonitorParam& param)
			{
				NSEventMask mask = 0;
				if (param.flagKeyDown) {
					mask |= NSEventMaskKeyDown;
				}
				if (param.flagKeyUp) {
					mask |= NSEventMaskKeyUp;
				}
				if (param.flagLeftButtonDown) {
					mask |= NSEventMaskLeftMouseDown;
				}
				if (param.flagLeftButtonUp) {
					mask |= NSEventMaskLeftMouseUp;
				}
				if (param.flagLeftButtonDrag) {
					mask |= NSEventMaskLeftMouseDragged;
				}
				if (param.flagRightButtonDown) {
					mask |= NSEventMaskRightMouseDown;
				}
				if (param.flagRightButtonUp) {
					mask |= NSEventMaskRightMouseUp;
				}
				if (param.flagRightButtonDrag) {
					mask |= NSEventMaskRightMouseDragged;
				}
				if (param.flagMiddleButtonDown) {
					mask |= NSEventMaskOtherMouseDown;
				}
				if (param.flagMiddleButtonUp) {
					mask |= NSEventMaskOtherMouseUp;
				}
				if (param.flagMiddleButtonDrag) {
					mask |= NSEventMaskOtherMouseDragged;
				}
				if (param.flagMouseMove) {
					mask |= NSEventMaskMouseMoved;
				}
				if (param.flagMouseWheel) {
					mask |= NSEventMaskScrollWheel;
				}
				if (!mask) {
					return sl_null;
				}
				Ref<NSEventMonitor> ret = new NSEventMonitor;
				if (ret.isNull()) {
					return sl_null;
				}
				WeakRef<NSEventMonitor> weak(ret);
				id monitorGlobal = [NSEvent addGlobalMonitorForEventsMatchingMask:mask handler:^(NSEvent* ev) {
					Ref<NSEventMonitor> monitor(weak);
					if (monitor.isNotNull()) {
						monitor->processEvent(ev);
					}
				}];
				if (monitorGlobal == nil) {
					return sl_null;
				}
				id monitorLocal = [NSEvent addLocalMonitorForEventsMatchingMask:mask handler:^(NSEvent* ev) {
					Ref<NSEventMonitor> monitor(weak);
					if (monitor.isNotNull()) {
						monitor->processEvent(ev);
					}
					return ev;
				}];
				if (monitorLocal == nil) {
					return sl_null;
				}
				ret->_initialize(param);
				ret->m_monitorGlobal = monitorGlobal;
				ret->m_monitorLocal = monitorLocal;
				return ret;
			}

		public:
			void release() override
			{
				ObjectLocker lock(this);
				id monitorGlobal = m_monitorGlobal;
				id monitorLocal = m_monitorLocal;
				m_monitorGlobal = nil;
				m_monitorLocal = nil;
				lock.unlock();
				if (monitorGlobal != nil || monitorLocal != nil) {
					UI::runOnUiThread([monitorGlobal, monitorLocal]() {
						if (monitorGlobal != nil) {
							[NSEvent removeMonitor:monitorGlobal];
						}
						if (monitorLocal != nil) {
							[NSEvent removeMonitor:monitorLocal];
						}
					});
				}
			}

			void prepareEvent(UIEvent* ev, NSEvent* event)
			{
				Time t;
				t.setSecondCountF([event timestamp]);
				UIPlatform::applyEventModifiers(ev, event);
				CGEventRef cev = [event CGEvent];
				if (cev) {
					if (IsInjectedEvent(cev)) {
						ev->addFlag(UIEventFlags::Injected);
					}
				}
			}

			void processKeyEvent(UIAction action, NSEvent* event)
			{
				sl_uint32 vkey = [event keyCode];
				Keycode key = UIEvent::getKeycodeFromSystemKeycode(vkey);
				Ref<UIEvent> ev = UIEvent::createKeyEvent(action, key, vkey, Time::zero());
				if (ev.isNotNull()) {
					prepareEvent(ev.get(), event);
					_onEvent(ev.get());
				}
			}

			void processMouseEvent(UIAction action, NSEvent* event)
			{
				NSPoint pt = event.locationInWindow;
				NSWindow* window = event.window;
				if (window != nil) {
					pt = [window convertPointToScreen:pt];
				}
				sl_ui_posf x = (sl_ui_posf)(pt.x);
				sl_ui_posf y = (sl_ui_posf)(pt.y);
				Ref<UIEvent> ev = UIEvent::createMouseEvent(action, x, y, Time::zero());
				if (ev.isNotNull()) {
					prepareEvent(ev.get(), event);
					_onEvent(ev.get());
				}
			}

			void processMouseWheelEvent(NSEvent* event)
			{
				sl_real deltaX = (sl_real)([event deltaX]);
				sl_real deltaY = (sl_real)([event deltaY]);
				if (Math::isAlmostZero(deltaX) && Math::isAlmostZero(deltaY)) {
					return;
				}
				NSPoint pt = event.locationInWindow;
				NSWindow* window = event.window;
				if (window != nil) {
					pt = [window convertPointToScreen:pt];
				}
				sl_ui_posf x = (sl_ui_posf)(pt.x);
				sl_ui_posf y = (sl_ui_posf)(pt.y);
				Ref<UIEvent> ev = UIEvent::createMouseWheelEvent(x, y, deltaX, deltaY, Time::zero());
				if (ev.isNotNull()) {
					prepareEvent(ev.get(), event);
					_onEvent(ev.get());
				}
			}

			void processEvent(NSEvent* ev)
			{
				NSEventType type = ev.type;
				switch (type) {
					case NSEventTypeKeyDown:
						processKeyEvent(UIAction::KeyDown, ev);
						break;
					case NSEventTypeKeyUp:
						processKeyEvent(UIAction::KeyUp, ev);
						break;
					case NSEventTypeLeftMouseDown:
						processMouseEvent(UIAction::LeftButtonDown, ev);
						break;
					case NSEventTypeLeftMouseUp:
						processMouseEvent(UIAction::LeftButtonUp, ev);
						break;
					case NSEventTypeLeftMouseDragged:
						processMouseEvent(UIAction::LeftButtonDrag, ev);
						break;
					case NSEventTypeRightMouseDown:
						processMouseEvent(UIAction::RightButtonDown, ev);
						break;
					case NSEventTypeRightMouseUp:
						processMouseEvent(UIAction::RightButtonUp, ev);
						break;
					case NSEventTypeRightMouseDragged:
						processMouseEvent(UIAction::RightButtonDrag, ev);
						break;
					case NSEventTypeOtherMouseDown:
						processMouseEvent(UIAction::MiddleButtonDown, ev);
						break;
					case NSEventTypeOtherMouseUp:
						processMouseEvent(UIAction::MiddleButtonUp, ev);
						break;
					case NSEventTypeOtherMouseDragged:
						processMouseEvent(UIAction::MiddleButtonDrag, ev);
						break;
					case NSEventTypeMouseMoved:
						processMouseEvent(UIAction::MouseMove, ev);
						break;
					case NSEventTypeScrollWheel:
						processMouseWheelEvent(ev);
						break;
					default:
						break;
				}
			}
		};
	}

	Ref<GlobalEventMonitor> GlobalEventMonitor::create(const GlobalEventMonitorParam& param)
	{
		if (param.flagEventTap) {
			return Ref<GlobalEventMonitor>::cast(CGEventTapMonitor::create(param));
		} else {
			return Ref<GlobalEventMonitor>::cast(NSEventMonitor::create(param));
		}
	}

}

#endif
