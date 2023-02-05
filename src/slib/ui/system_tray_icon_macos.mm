/*
 *   Copyright (c) 2008-2019 SLIBIO <https://github.com/SLIBIO>
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

#include "slib/ui/definition.h"

#if defined(SLIB_UI_IS_MACOS)

#include "slib/ui/system_tray_icon.h"

#include "slib/core/safe_static.h"
#include "slib/ui/notification.h"
#include "slib/ui/platform.h"

#define NOTIFICATION_ID_PREFIX "system_tray_icon.notify."

namespace slib {
	namespace {
		class SystemTrayIconImpl;
	}
}

@interface SLIBSystemTrayIconListener : NSObject
{
	@public slib::WeakRef<slib::SystemTrayIconImpl> m_object;
}
- (void)onAction;
@end

@interface SLIBSystemTrayIconSubView : NSView
{
	@public slib::WeakRef<slib::SystemTrayIconImpl> m_object;
}
@end

namespace slib
{

	namespace {

		class StaticContext
		{
		public:
			CHashMap< String, WeakRef<SystemTrayIcon> > icons;
			Function<void(UserNotificationMessage&)> callbackClick;

		public:
			StaticContext()
			{
				UserNotification::start();
				UserNotification::requestAuthorization(UserNotificationAuthorizationOptions::Alert | UserNotificationAuthorizationOptions::Sound, sl_null);
				callbackClick = [this](UserNotificationMessage& msg) {
					if (msg.identifier.startsWith(NOTIFICATION_ID_PREFIX)) {
						Ref<SystemTrayIcon> icon = icons.getValue(msg.identifier.substring(StringView(NOTIFICATION_ID_PREFIX).getLength()));
						if (icon.isNotNull()) {
							Ref<UIEvent> ev = UIEvent::createUnknown(Time::now());
							icon->dispatchClick(ev.get());
						}
					}
				};
				UserNotification::addOnClickMessage(callbackClick);
			}

			~StaticContext()
			{
				UserNotification::removeOnClickMessage(callbackClick);
			}

		};

		SLIB_SAFE_STATIC_GETTER(StaticContext, GetStaticContext)

		class SystemTrayIconImpl : public SystemTrayIcon
		{
		public:
			NSStatusItem* m_item;
			SLIBSystemTrayIconListener* m_listener;

		public:
			SystemTrayIconImpl()
			{
				m_item = nil;
				m_listener = nil;
			}

			~SystemTrayIconImpl()
			{
				StaticContext* context = GetStaticContext();
				if (context) {
					context->icons.remove(m_iconName);
				}
				if (m_item != nil) {
					NSStatusBar* bar = [NSStatusBar systemStatusBar];
					if (bar != nil) {
						[bar removeStatusItem:m_item];
					}
				}
			}

		public:
			static Ref<SystemTrayIconImpl> create(const SystemTrayIconParam& param)
			{
				StaticContext* context = GetStaticContext();
				if (!context) {
					return sl_null;
				}
				NSStatusBar* bar = [NSStatusBar systemStatusBar];
				if (bar != nil) {
					NSStatusItem* item = [bar statusItemWithLength:NSSquareStatusItemLength];
					if (item != nil) {
						Ref<SystemTrayIconImpl> ret = new SystemTrayIconImpl;
						if (ret.isNotNull()) {
							ret->m_item = item;
							ret->_init(param);
							context->icons.add(param.iconName, ret);
							return ret;
						}
					}
				}
				return sl_null;
			}

		public:
			void _init(const SystemTrayIconParam& param)
			{
				SystemTrayIcon::_init(param);
				if (@available(macos 10.12, *)) {
					m_item.visible = YES;
				}
				if (param.icon.isNotNull() || param.iconName.isNotEmpty()) {
					setIcon_NI(param.icon, param.iconName);
				}
				if (param.toolTip.isNotEmpty()) {
					setToolTip_NI(param.toolTip);
				}
				if (param.menu.isNotNull()) {
					setMenu_NI(param.menu);
				}
				m_item.highlightMode = param.flagHighlight;
				if (@available(macos 10.10, *)) {
					if (m_onClick.isNotNull() || m_onRightClick.isNotNull() || m_onEvent.isNotNull()) {
						NSStatusBarButton* button = m_item.button;
						SLIBSystemTrayIconSubView* subview = [SLIBSystemTrayIconSubView new];
						subview->m_object = this;
						[subview setFrame:button.frame];
						[button addSubview:subview];
					}
				} else {
					SLIBSystemTrayIconListener* listener = [SLIBSystemTrayIconListener new];
					listener->m_object = this;
					m_listener = listener;
					m_item.target = listener;
					m_item.action = @selector(onAction);
				}
			}

			void setIcon_NI(const Ref<Drawable>& icon, const String& name) override
			{
				NSImage* image = nil;
				if (name.isNotEmpty()) {
					image = [NSImage imageNamed:Apple::getNSStringFromString(name)];
				}
				if (image == nil && icon.isNotNull()) {
					image = GraphicsPlatform::getNSImage(icon);
					if (image != nil) {
						CGFloat height = [[NSStatusBar systemStatusBar] thickness];
						if (image.size.width > height + 0.5 && image.size.height > height + 0.5) {
							[image setSize:NSMakeSize(height, height)];
						}
					}
				}
				if (@available(macos 10.10, *)) {
					m_item.button.image = image;
					m_item.button.alternateImage = image;
				} else {
					m_item.image = image;
					m_item.alternateImage = image;
				}
			}

			void setToolTip_NI(const String& toolTip) override
			{
				NSString* str = Apple::getNSStringFromString(toolTip);
				if (@available(macos 10.10, *)) {
					m_item.button.toolTip = str;
				} else {
					m_item.toolTip = str;
				}
			}

			void setMenu_NI(const Ref<Menu>& menu) override
			{
				m_item.menu = UIPlatform::getMenuHandle(menu.get());
			}

			void notify_NI(const SystemTrayIconNotifyParam& param) override
			{
				UserNotificationMessage msg;
				msg.identifier = String::concat(NOTIFICATION_ID_PREFIX, m_iconName);
				msg.title = param.title;
				msg.content = param.message;
				msg.flagSound = param.flagSound;
				UserNotification::add(msg);
			}

			UIEventFlags _onEvent(UIAction action, NSEvent* event)
			{
				if (event == nil) {
					event = [NSApp currentEvent];
					if (event == nil) {
						Ref<UIEvent> ev = UIEvent::createUnknown(Time::now());
						if (ev.isNotNull()) {
							dispatchEvent(ev.get());
						}
						return 0;
					}
				}
				if (action == UIAction::Unknown) {
					switch ([event type]) {
						case NSLeftMouseDown:
							action = UIAction::LeftButtonDown;
							break;
						case NSLeftMouseUp:
							action = UIAction::LeftButtonUp;
							break;
						case NSLeftMouseDragged:
							action = UIAction::LeftButtonDrag;
							break;
						case NSRightMouseDown:
							action = UIAction::RightButtonDown;
							break;
						case NSRightMouseUp:
							action = UIAction::RightButtonUp;
							break;
						case NSRightMouseDragged:
							action = UIAction::RightButtonDrag;
							break;
						case NSOtherMouseDown:
							action = UIAction::MiddleButtonDown;
							break;
						case NSOtherMouseUp:
							action = UIAction::MiddleButtonUp;
							break;
						case NSOtherMouseDragged:
							action = UIAction::MiddleButtonDrag;
							break;
						default:
							return 0;
					}
				}
				Time t;
				t.setSecondCountf([event timestamp]);
				NSPoint pt = [NSEvent mouseLocation];
				sl_ui_posf x = (sl_ui_posf)(pt.x);
				sl_ui_posf y = (sl_ui_posf)(pt.y);
				Ref<UIEvent> ev = UIEvent::createMouseEvent(action, x, y, t);
				if (ev.isNotNull()) {
					UIPlatform::applyEventModifiers(ev.get(), event);
					dispatchEvent(ev.get());
					if (!(ev->isPreventedDefault())) {
						switch (action) {
							case UIAction::LeftButtonDown:
								dispatchClick(ev.get());
								break;
							case UIAction::RightButtonDown:
								dispatchRightClick(ev.get());
								break;
							default:
								break;
						}
					}
					return ev->getFlags();
				}
				return 0;
			}

			void tryHighlight()
			{
				if (@available(macos 10.10, *)) {
					if (m_flagHighlight) {
						NSStatusBarButton* button = m_item.button;
						[button highlight:YES];
						dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.1 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
							[button highlight:NO];
						});
					}
				}
			}

		};

	}

	Ref<SystemTrayIcon> SystemTrayIcon::create(const SystemTrayIconParam& param)
	{
		return Ref<SystemTrayIcon>::from(SystemTrayIconImpl::create(param));
	}

}

using namespace slib;

@implementation SLIBSystemTrayIconListener

- (void)onAction
{
	Ref<SystemTrayIconImpl> object(m_object);
	if (object.isNotNull()) {
		object->_onEvent(UIAction::Unknown, nil);
	}
}

@end

@implementation SLIBSystemTrayIconSubView

- (void)mouseDown:(NSEvent *)theEvent
{
	Ref<SystemTrayIconImpl> object(m_object);
	if (object.isNotNull()) {
		UIEventFlags flags = object->_onEvent(UIAction::LeftButtonDown, theEvent);
		if (flags & UIEventFlags::PreventDefault) {
			object->tryHighlight();
			return;
		}
	}
	[[self nextResponder] mouseDown:theEvent];
}

- (void)mouseUp:(NSEvent *)theEvent
{
	Ref<SystemTrayIconImpl> object(m_object);
	if (object.isNotNull()) {
		UIEventFlags flags = object->_onEvent(UIAction::LeftButtonUp, theEvent);
		NSInteger clicks = [theEvent clickCount];
		if (clicks == 2) {
			flags |= object->_onEvent(UIAction::LeftButtonDoubleClick, theEvent);
		}
		if (flags & UIEventFlags::PreventDefault) {
			return;
		}
	}
	[[self nextResponder] mouseUp:theEvent];
}

- (void)mouseDragged:(NSEvent *)theEvent
{
	Ref<SystemTrayIconImpl> object(m_object);
	if (object.isNotNull()) {
		UIEventFlags flags = object->_onEvent(UIAction::LeftButtonDrag, theEvent);
		if (flags & UIEventFlags::PreventDefault) {
			return;
		}
	}
	[[self nextResponder] mouseDragged:theEvent];
}

- (void)rightMouseDown:(NSEvent *)theEvent
{
	Ref<SystemTrayIconImpl> object(m_object);
	if (object.isNotNull()) {
		UIEventFlags flags = object->_onEvent(UIAction::RightButtonDown, theEvent);
		if (flags & UIEventFlags::PreventDefault) {
			object->tryHighlight();
			return;
		}
	}
	[[self nextResponder] rightMouseDown:theEvent];
}

- (void)rightMouseUp:(NSEvent *)theEvent
{
	Ref<SystemTrayIconImpl> object(m_object);
	if (object.isNotNull()) {
		UIEventFlags flags = object->_onEvent(UIAction::RightButtonUp, theEvent);
		NSInteger clicks = [theEvent clickCount];
		if (clicks == 2) {
			flags |= object->_onEvent(UIAction::RightButtonDoubleClick, theEvent);
		}
		if (flags & UIEventFlags::PreventDefault) {
			return;
		}
	}
	[[self nextResponder] rightMouseUp:theEvent];
}

- (void)rightMouseDragged:(NSEvent *)theEvent
{
	Ref<SystemTrayIconImpl> object(m_object);
	if (object.isNotNull()) {
		UIEventFlags flags = object->_onEvent(UIAction::RightButtonDrag, theEvent);
		if (flags & UIEventFlags::PreventDefault) {
			return;
		}
	}
	[[self nextResponder] rightMouseDragged:theEvent];
}

- (void)otherMouseDown:(NSEvent *)theEvent
{
	Ref<SystemTrayIconImpl> object(m_object);
	if (object.isNotNull()) {
		UIEventFlags flags = object->_onEvent(UIAction::MiddleButtonDown, theEvent);
		if (flags & UIEventFlags::PreventDefault) {
			object->tryHighlight();
			return;
		}
	}
	[[self nextResponder] otherMouseDown:theEvent];
}

- (void)otherMouseUp:(NSEvent *)theEvent
{
	Ref<SystemTrayIconImpl> object(m_object);
	if (object.isNotNull()) {
		UIEventFlags flags = object->_onEvent(UIAction::MiddleButtonUp, theEvent);
		NSInteger clicks = [theEvent clickCount];
		if (clicks == 2) {
			flags |= object->_onEvent(UIAction::MiddleButtonDoubleClick, theEvent);
		}
		if (flags & UIEventFlags::PreventDefault) {
			return;
		}
	}
	[[self nextResponder] otherMouseUp:theEvent];
}

- (void)otherMouseDragged:(NSEvent *)theEvent
{
	Ref<SystemTrayIconImpl> object(m_object);
	if (object.isNotNull()) {
		UIEventFlags flags = object->_onEvent(UIAction::MiddleButtonDrag, theEvent);
		if (flags & UIEventFlags::PreventDefault) {
			return;
		}
	}
	[[self nextResponder] otherMouseDragged:theEvent];
}
@end

#endif
