/*
 *   Copyright (c) 2008-2020 SLIBIO <https://github.com/SLIBIO>
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

#include "window.h"

#include "slib/ui/platform.h"

#include "view_macos.h"

namespace slib {
	namespace {
		class macOS_WindowInstance;
	}
}

@interface SLIBWindowHandle : NSWindow<NSWindowDelegate> {

	@public slib::WeakRef<slib::macOS_WindowInstance> m_window;

	@public sl_bool m_flagStateResizingWidth;

	@private NSView* m_handleLastHitMouse;

}
@end

namespace slib
{

	namespace {

		static void ApplyRectLimit(NSRect& rect)
		{
			if (rect.origin.x < -16000.0f) {
				rect.origin.x = -16000.0f;
			}
			if (rect.origin.x > 16000.0f) {
				rect.origin.x = 16000.0f;
			}
			if (rect.origin.y < -16000.0f) {
				rect.origin.y = -16000.0f;
			}
			if (rect.origin.y > 16000.0f) {
				rect.origin.y = 16000.0f;
			}
			if (rect.size.width < 0.0f) {
				rect.size.width = 0.0f;
			}
			if (rect.size.width > 10000.0f) {
				rect.size.width = 10000.0f;
			}
			if (rect.size.height < 0.0f) {
				rect.size.height = 0.0f;
			}
			if (rect.size.height > 10000.0f) {
				rect.size.height = 10000.0f;
			}
		}

		static void ToNSRect(NSRect& rectOut, const UIRect& rectIn, sl_ui_len heightScreen)
		{
			rectOut.origin.x = (int)(rectIn.left);
			rectOut.origin.y = (int)(heightScreen - rectIn.bottom);
			rectOut.size.width = (int)(rectIn.getWidth());
			rectOut.size.height = (int)(rectIn.getHeight());
			ApplyRectLimit(rectOut);
		}

		static void ToUIRect(UIRect& rectOut, const NSRect& rectIn, sl_ui_len heightScreen)
		{
			rectOut.left = (sl_ui_pos)(rectIn.origin.x);
			rectOut.top = heightScreen - (sl_ui_pos)(rectIn.origin.y + rectIn.size.height);
			rectOut.setWidth((sl_ui_pos)(rectIn.size.width));
			rectOut.setHeight((sl_ui_pos)(rectIn.size.height));
			rectOut.fixSizeError();
		}

		static int MakeWindowStyleMask(Window* window)
		{
			int styleMask;
			if (window->isSheet()) {
				styleMask = NSTitledWindowMask;
			} else {
				if (window->isBorderless()) {
					styleMask = NSBorderlessWindowMask;
				} else {
					if (window->isTitleBarVisible()) {
						styleMask = NSTitledWindowMask;
					} else {
						styleMask = NSClosableWindowMask | NSMiniaturizableWindowMask;
					}
					if (window->isCloseButtonEnabled()) {
						styleMask |= NSClosableWindowMask;
					}
					if (window->isMinimizeButtonEnabled()) {
						styleMask |= NSMiniaturizableWindowMask;
					}
					if (window->isResizable()) {
						styleMask |= NSResizableWindowMask;
					}
				}
			}
			return styleMask;
		}

		class macOS_WindowInstance : public WindowInstance
		{
		public:
			NSWindow* m_window;
			__weak NSWindow* m_parent;

			sl_ui_len m_heightScreen;

			AtomicRef<ViewInstance> m_viewContent;

			sl_bool m_flagClosing;
			sl_bool m_flagSheet;
			sl_bool m_flagDoModal;

		public:
			macOS_WindowInstance()
			{
				m_flagClosing = sl_false;
				m_flagSheet = sl_false;
				m_flagDoModal = sl_false;
			}

			~macOS_WindowInstance()
			{
				release();
			}

		public:
			static Ref<macOS_WindowInstance> create(NSWindow* window)
			{
				if (window != nil) {
					sl_ui_pos heightScreen;
					NSScreen* screen = [window screen];
					if (screen != nil) {
						NSRect rc = [screen frame];
						heightScreen = (sl_ui_pos)(rc.size.height);
						if (heightScreen < 0) {
							heightScreen = 0;
						}
					} else {
						heightScreen = 0;
					}
					Ref<macOS_WindowInstance> ret = new macOS_WindowInstance();
					if (ret.isNotNull()) {
						ret->m_window = window;
						ret->m_heightScreen = heightScreen;
						NSView* view = [window contentView];
						if (view != nil) {
							Ref<ViewInstance> content = UIPlatform::createViewInstance(view);
							if (content.isNotNull()) {
								content->setWindowContent(sl_true);
								ret->m_viewContent = content;
								if ([view isKindOfClass:[SLIBViewHandle class]]) {
									((SLIBViewHandle*)view)->m_viewInstance = Ref<macOS_ViewInstance>::cast(content);
								}
							}
						}
						UIPlatform::registerWindowInstance(window, ret.get());
						return ret;
					}
				}
				return sl_null;
			}

			static Ref<WindowInstance> create(Window* window)
			{

				Ref<Window> _parent = window->getParent();
				NSWindow* parent = UIPlatform::getWindowHandle(_parent.get());

				UIRect frameScreen;
				NSScreen* screen = nil;
				Ref<Screen> _screen = window->getScreen();
				if (_screen.isNotNull()) {
					frameScreen = _screen->getRegion();
					screen = UIPlatform::getScreenHandle(_screen.get());
				} else {
					frameScreen = UI::getScreenRegion();
				}

				sl_bool flagSheet = window->isSheet();
				if (parent == nil) {
					flagSheet = sl_false;
					window->setSheet(sl_false);
				}

				NSRect rect;
				int styleMask = MakeWindowStyleMask(window);
				if (flagSheet) {
					UISize size = window->getSize();
					rect.origin.x = 0;
					rect.origin.y = 0;
					rect.size.width = (CGFloat)(size.x);
					rect.size.height = (CGFloat)(size.y);
				} else {
					UIRect frameWindow = MakeWindowFrame(window);
					frameWindow = window->getClientFrameFromWindowFrame(frameWindow);
					ToNSRect(rect, frameWindow, frameScreen.getHeight());
				}

				SLIBWindowHandle* handle = [[SLIBWindowHandle alloc] initWithContentRect:rect styleMask:styleMask backing:NSBackingStoreBuffered defer:YES screen:screen];

				if (handle != nil) {

					[handle setDelegate: handle];
					handle.animationBehavior = NSWindowAnimationBehaviorDocumentWindow;

					handle->m_flagStateResizingWidth = sl_false;
					[handle setReleasedWhenClosed:NO];
					[handle setContentView:[[SLIBViewHandle alloc] init]];

					Ref<macOS_WindowInstance> ret = create(handle);

					if (ret.isNotNull()) {

						ret->m_parent = parent;

						handle->m_window = ret;

						{
							NSString* title = Apple::getNSStringFromString(window->getTitle());
							[handle setTitle: title];

							Color _color = window->getBackgroundColor();
							if (_color.isNotZero()) {
								NSColor* color = GraphicsPlatform::getNSColorFromColor(_color);
								[handle setBackgroundColor:color];
							}

							if (window->isFullScreenButtonEnabled()) {
								handle.collectionBehavior = NSWindowCollectionBehaviorFullScreenPrimary;
							}

							sl_real alpha = window->getAlpha();
							if (alpha < 0.9999f) {
								if (alpha < 0) {
									alpha = 0;
								}
								[handle setAlphaValue:alpha];
							}

							if (window->isTransparent()) {
								[handle setIgnoresMouseEvents: TRUE];
							}

							if (window->isAlwaysOnTop()) {
								[handle setLevel:NSFloatingWindowLevel];
							}
						}

						if (flagSheet) {
							ret->m_flagSheet = sl_true;
							WeakRef<macOS_WindowInstance> retWeak = ret;
							[parent beginSheet:handle completionHandler:^(NSModalResponse returnCode) {
								Ref<macOS_WindowInstance> w = retWeak;
								if (w.isNotNull()) {
									w->m_flagSheet = sl_false;
								}
							}];
						}

						if (window->isFullScreen()) {
							handle.animationBehavior = NSWindowAnimationBehaviorNone;
							UI::dispatchToUiThread([handle]() {
								handle.collectionBehavior = NSWindowCollectionBehaviorFullScreenPrimary;
								[handle toggleFullScreen: nil];
							});
						}
						return ret;
					}
				}
				return sl_null;
			}

			void release()
			{
				close();
			}

			void close() override
			{
				m_viewContent.setNull();
				NSWindow* window = m_window;
				if (window != nil) {
					UIPlatform::removeWindowInstance(window);
					if (m_flagSheet) {
						m_flagSheet = sl_false;
						NSWindow* parent = m_parent;
						if (parent != nil) {
							[parent endSheet: window];
							return;
						}
					}
					if (m_flagDoModal) {
						m_flagDoModal = sl_false;
						[NSApp stopModal];
					}
					if (!m_flagClosing) {
						[window close];
					}
					m_window = nil;
				}
			}

			sl_bool isClosed() override
			{
				return m_window == nil;
			}

			void setParent(const Ref<WindowInstance>& windowInst) override
			{
				NSWindow* window = m_window;
				if (window != nil) {
					if (windowInst.isNotNull()) {
						macOS_WindowInstance* w = static_cast<macOS_WindowInstance*>(windowInst.get());
						NSWindow* p = w->m_window;
						m_parent = p;
						if (p != nil && window.parentWindow == nil) {
							[p addChildWindow:window ordered:NSWindowAbove];
						}
					} else {
						NSWindow* p = window.parentWindow;
						if (p != nil) {
							[p removeChildWindow:window];
						}
						m_parent = nil;
					}
				}
			}

			Ref<ViewInstance> getContentView() override
			{
				return m_viewContent;
			}

			sl_bool getFrame(UIRect& _out) override
			{
				NSWindow* window = m_window;
				if (window != nil) {
					NSRect rect = [window frame];
					ToUIRect(_out, rect, m_heightScreen);
					return sl_true;
				}
				return sl_false;
			}

			void setFrame(const UIRect& frame) override
			{
				NSWindow* window = m_window;
				if (window != nil) {
					NSRect rect;
					ToNSRect(rect, frame, m_heightScreen);
					[window setFrame:rect display:TRUE];
				}
			}

			void setTitle(const String& title) override
			{
				NSWindow* window = m_window;
				if (window != nil) {
					[window setTitle: Apple::getNSStringFromString(title)];
				}
			}

			sl_bool isActive() override
			{
				NSWindow* window = m_window;
				if (window != nil) {
					return [window isKeyWindow];
				}
				return sl_false;
			}

			void activate() override
			{
				NSWindow* window = m_window;
				if (window != nil) {
					[window makeKeyAndOrderFront:NSApp];
				}
			}

			void setBackgroundColor(const Color& _color) override
			{
				NSWindow* window = m_window;
				if (window != nil) {
					NSColor* color = GraphicsPlatform::getNSColorFromColor(_color);
					[window setBackgroundColor:color];
				}
			}

			void resetBackgroundColor() override
			{
				NSWindow* window = m_window;
				if (window != nil) {
					[window setBackgroundColor:[NSColor windowBackgroundColor]];
				}
			}

			void isMinimized(sl_bool& _out) override
			{
				NSWindow* window = m_window;
				if (window != nil) {
					_out = [window isMiniaturized] ? sl_true : sl_false;
				}
			}

			void setMinimized(sl_bool flag) override
			{
				NSWindow* window = m_window;
				if (window != nil) {
					if (flag) {
						[window miniaturize:nil];
					} else {
						[window deminiaturize:nil];
					}
				}
			}

			void isMaximized(sl_bool& _out) override
			{
				NSWindow* window = m_window;
				if (window != nil) {
					_out = [window isZoomed] ? sl_true : sl_false;
				}
			}

			void setMaximized(sl_bool flag) override
			{
				NSWindow* window = m_window;
				if (window != nil) {
					sl_bool f1 = [window isZoomed] ? sl_true : sl_false;
					sl_bool f2 = flag ? sl_true : sl_false;
					if (f1 != f2) {
						[window zoom:nil];
						UI::dispatchToUiThread([window]() {
							[window invalidateShadow];
						}, 100);
					}
				}
			}

			void isFullScreen(sl_bool& _out) override
			{
				NSWindow* window = m_window;
				if (window != nil) {
					_out = (window.styleMask & NSWindowStyleMaskFullScreen) ? sl_true : sl_false;
				}
			}

			void setFullScreen(sl_bool flag) override
			{
				NSWindow* window = m_window;
				if (window != nil) {
					sl_bool f1 = (window.styleMask & NSWindowStyleMaskFullScreen) ? sl_true : sl_false;
					sl_bool f2 = flag ? sl_true : sl_false;
					if (f1 != f2) {
						[window toggleFullScreen:nil];
						if (flag) {
							setFullScreenButtonEnabled(sl_true);
						}
					}
				}
			}

			void setVisible(sl_bool flag) override
			{
				NSWindow* window = m_window;
				if (window != nil) {
					if (flag) {
						NSWindow* parent = m_parent;
						if (parent != nil) {
							if (window.parentWindow == nil) {
								[parent addChildWindow:window ordered:NSWindowAbove];
							}
						} else {
							[window orderFrontRegardless];
						}
					} else {
						[window orderOut:nil];
					}
				}
			}

			void setAlwaysOnTop(sl_bool flag) override
			{
				NSWindow* window = m_window;
				if (window != nil) {
					if (flag) {
						[window setLevel:NSFloatingWindowLevel];
					} else {
						[window setLevel:NSNormalWindowLevel];
					}
				}
			}

			void setCloseButtonEnabled(sl_bool flag) override
			{
				NSWindow* window = m_window;
				if (window != nil) {
					NSUInteger style = [window styleMask];
					if (!(style & NSTitledWindowMask)) {
						return;
					}
					sl_bool f1 = (style & NSClosableWindowMask) ? sl_true : sl_false;
					sl_bool f2 = flag ? sl_true : sl_false;
					if (f1 != f2) {
						if (flag) {
							style = style | NSClosableWindowMask;
						} else {
							style = style & (~NSClosableWindowMask);
						}
						[window setStyleMask:style];
					}
				}
			}

			void setMinimizeButtonEnabled(sl_bool flag) override
			{
				NSWindow* window = m_window;
				if (window != nil) {
					NSUInteger style = [window styleMask];
					if (!(style & NSTitledWindowMask)) {
						return;
					}
					sl_bool f1 = (style & NSMiniaturizableWindowMask) ? sl_true : sl_false;
					sl_bool f2 = flag ? sl_true : sl_false;
					if (f1 != f2) {
						if (flag) {
							style = style | NSMiniaturizableWindowMask;
						} else {
							style = style & (~NSMiniaturizableWindowMask);
						}
						[window setStyleMask:style];
					}
				}
			}

			void setFullScreenButtonEnabled(sl_bool flag) override
			{
				NSWindow* window = m_window;
				if (window != nil) {
					NSWindowCollectionBehavior behavior;
					if (flag || (window.styleMask & NSWindowStyleMaskFullScreen)) {
						behavior = NSWindowCollectionBehaviorFullScreenPrimary;
					} else {
						behavior = NSWindowCollectionBehaviorFullScreenAuxiliary;
					}
					if (behavior != window.collectionBehavior) {
						window.collectionBehavior = behavior;
					}
				}
			}

			void setResizable(sl_bool flag) override
			{
				NSWindow* window = m_window;
				if (window != nil) {
					NSUInteger style = [window styleMask];
					sl_bool f1 = (style & NSResizableWindowMask) ? sl_true : sl_false;
					sl_bool f2 = flag ? sl_true : sl_false;
					if (f1 != f2) {
						if (flag) {
							style = style | NSResizableWindowMask;
						} else {
							style = style & (~NSResizableWindowMask);
						}
						[window setStyleMask:style];
					}
				}
			}

			void setAlpha(sl_real _alpha) override
			{
				NSWindow* window = m_window;
				if (window != nil) {
					sl_real alpha = _alpha;
					if (alpha < 0) {
						alpha = 0;
					}
					if (alpha > 1) {
						alpha = 1;
					}
					[window setAlphaValue:alpha];
				}
			}

			void setTransparent(sl_bool flag) override
			{
				NSWindow* window = m_window;
				if (window != nil) {
					[window setIgnoresMouseEvents:(flag?TRUE:FALSE)];
				}
			}

			sl_bool getClientInsets(UIEdgeInsets& _out) override
			{
				NSWindow* window = m_window;
				if (window != nil) {
					NSRect frame = [window frame];
					NSRect client = [window contentRectForFrameRect:frame];
					_out.left = (sl_ui_len)(client.origin.x - frame.origin.x);
					_out.top = (sl_ui_len)(frame.origin.y + frame.size.height - (client.origin.y + client.size.height));
					_out.right = (sl_ui_len)(frame.origin.x + frame.size.width - (client.origin.x + client.size.width));
					_out.bottom = (sl_ui_len)(client.origin.y - frame.origin.y);
					return sl_true;
				}
				return sl_false;
			}

			sl_bool doModal() override
			{
				NSWindow* window = m_window;
				if (window != nil) {
					if (m_flagSheet) {
						return sl_false;
					}
					m_flagDoModal = sl_true;
					[NSApp runModalForWindow:window];
					return sl_true;
				}
				return sl_false;
			}

			void doPostCreate() override
			{
				NSWindow* window = m_window;
				if (window != nil) {
					NSRect frame = [window frame];
					NSRect client = [window contentRectForFrameRect:frame];
					onResize((sl_ui_len)(client.size.width), (sl_ui_len)(client.size.height));
				}
			}

			void onMove()
			{
				UIRect frame;
				if (getFrame(frame)) {
					WindowInstance::onMove(frame.left, frame.top);
				}
			}

		};

	}

	Ref<WindowInstance> Window::createWindowInstance()
	{
		return macOS_WindowInstance::create(this);
	}

	Ref<Window> Window::getActiveWindow()
	{
		NSWindow* handle = UIPlatform::getKeyWindow();
		if (handle != nil) {
			Ref<WindowInstance> instance = UIPlatform::getWindowInstance(handle);
			if (instance.isNotNull()) {
				return instance->getWindow();
			}
		}
		return sl_null;
	}

	sl_bool Window::_getClientInsets(UIEdgeInsets& _out)
	{
		int styleMask = MakeWindowStyleMask(this);
		NSRect frame = NSMakeRect(100, 100, 200, 200);
		NSRect client = [NSWindow contentRectForFrameRect:frame styleMask:styleMask];
		_out.left = (sl_ui_len)(client.origin.x - frame.origin.x);
		_out.top = (sl_ui_len)(frame.origin.y + frame.size.height - (client.origin.y + client.size.height));
		_out.right = (sl_ui_len)(frame.origin.x + frame.size.width - (client.origin.x + client.size.width));
		_out.bottom = (sl_ui_len)(client.origin.y - frame.origin.y);
		return sl_true;
	}


	Ref<WindowInstance> UIPlatform::createWindowInstance(NSWindow* window)
	{
		Ref<WindowInstance> ret = UIPlatform::_getWindowInstance((__bridge void*)window);
		if (ret.isNotNull()) {
			return ret;
		}
		return macOS_WindowInstance::create(window);
	}

	void UIPlatform::registerWindowInstance(NSWindow* window, WindowInstance* instance)
	{
		UIPlatform::_registerWindowInstance((__bridge void*)window, instance);
	}

	Ref<WindowInstance> UIPlatform::getWindowInstance(NSWindow* window)
	{
		return UIPlatform::_getWindowInstance((__bridge void*)window);
	}

	void UIPlatform::removeWindowInstance(NSWindow* window)
	{
		UIPlatform::_removeWindowInstance((__bridge void*)window);
	}

	NSWindow* UIPlatform::getWindowHandle(WindowInstance* instance)
	{
		macOS_WindowInstance* window = (macOS_WindowInstance*)instance;
		if (window) {
			return window->m_window;
		} else {
			return nil;
		}
	}

	NSWindow* UIPlatform::getWindowHandle(Window* window)
	{
		if (window) {
			Ref<WindowInstance> instance = window->getWindowInstance();
			if (instance.isNotNull()) {
				macOS_WindowInstance* _instance = (macOS_WindowInstance*)(instance.get());
				if (_instance) {
					return _instance->m_window;
				}
			}
		}
		return nil;
	}

	NSWindow* UIPlatform::getMainWindow()
	{
		return [NSApp mainWindow];
	}

	NSWindow* UIPlatform::getKeyWindow()
	{
		NSWindow* window = [NSApp keyWindow];
		if (window != nil) {
			return window;
		}
		return UIPlatform::getMainWindow();
	}

}

using namespace slib;

@implementation SLIBWindowHandle

- (BOOL)canBecomeKeyWindow
{
	return YES;
}

- (BOOL)windowShouldClose:(id)sender
{
	BOOL ret = YES;
	Ref<macOS_WindowInstance> window = m_window;
	if (window.isNotNull()) {
		window->m_flagClosing = sl_true;
		if (window->onClose()) {
			window->close();
			ret = YES;
		} else {
			ret = NO;
		}
		window->m_flagClosing = sl_false;
	}
	return ret;
}

- (void)windowWillClose:(NSNotification *)notification
{
	Ref<macOS_WindowInstance> window = m_window;
	if (window.isNotNull()) {
		UI::dispatchToUiThread(SLIB_FUNCTION_REF(Move(window), release));
	}
}

- (void)windowDidBecomeKey:(NSNotification *)notification
{
	Ref<macOS_WindowInstance> window = m_window;
	if (window.isNotNull()) {
		window->onActivate();
	}
}

- (void)windowDidResignKey:(NSNotification *)notification
{
	Ref<macOS_WindowInstance> window = m_window;
	if (window.isNotNull()) {
		window->onDeactivate();
	}
}

- (NSSize)windowWillResize:(NSWindow *)sender toSize:(NSSize)frameSize
{
	Ref<macOS_WindowInstance> window = m_window;
	if (window.isNotNull()) {
		UISize size((sl_ui_pos)(frameSize.width), (sl_ui_pos)(frameSize.height));
		if (size.x < 0) {
			size.x = 0;
		}
		if (size.y < 0) {
			size.y = 0;
		}
		if (size.x != self.frame.size.width) {
			m_flagStateResizingWidth = sl_true;
		}
		NSRect fr = [self frame];
		NSRect cr = [self contentRectForFrameRect:fr];
		sl_ui_len dw = (sl_ui_len)(fr.size.width - cr.size.width);
		sl_ui_len dh = (sl_ui_len)(fr.size.height - cr.size.height);
		size.x -= dw;
		size.y -= dh;
		window->onResizing(size, m_flagStateResizingWidth);
		size.x += dw;
		size.y += dh;
		frameSize.width = (CGFloat)(size.x);
		frameSize.height = (CGFloat)(size.y);
	}
	return frameSize;
}

- (void)windowWillStartLiveResize:(NSNotification *)notification
{
	m_flagStateResizingWidth = sl_false;
}

- (void)windowDidResize:(NSNotification *)notification
{
	Ref<macOS_WindowInstance> window = m_window;
	if (window.isNotNull()) {
		NSRect fr = [self frame];
		NSRect cr = [self contentRectForFrameRect:fr];
		window->onResize((sl_ui_len)(cr.size.width), (sl_ui_len)(cr.size.height));
	}
}

- (void)windowDidMove:(NSNotification *)notification
{
	Ref<macOS_WindowInstance> window = m_window;
	if (window.isNotNull()) {
		window->onMove();
	}
}

- (void)windowWillMiniaturize:(NSNotification *)notification
{
	Ref<macOS_WindowInstance> window = m_window;
	if (window.isNotNull()) {
		window->onMinimize();
	}
}

- (void)windowDidDeminiaturize:(NSNotification *)notification
{
	Ref<macOS_WindowInstance> window = m_window;
	if (window.isNotNull()) {
		window->onDeminimize();
	}
}

- (void)windowDidEnterFullScreen:(NSNotification *)notification
{
	Ref<macOS_WindowInstance> window = m_window;
	if (window.isNotNull()) {
		window->onEnterFullScreen();
	}
}

- (void)windowDidExitFullScreen:(NSNotification *)notification
{
	Ref<macOS_WindowInstance> window = m_window;
	if (window.isNotNull()) {
		window->onExitFullScreen();
	}
}

- (BOOL)acceptsFirstResponder
{
	return YES;
}

- (BOOL)acceptsMouseMovedEvents
{
	return YES;
}

- (void)sendEvent:(NSEvent *)event
{
	NSEventType type = event.type;
	switch (type) {
		case NSEventTypeKeyDown:
		case NSEventTypeKeyUp:
			{
				id view = [self firstResponder];
				if (type == NSEventTypeKeyDown) {
					if ([view isKindOfClass:[NSTextView class]]) {
						// Find NSTextField
						NSView* t = view;
						t = t.superview;
						if (!([t isKindOfClass:[NSTextField class]])) {
							t = t.superview;
						}
						if ([t isKindOfClass:[NSTextField class]]) {
							int c = event.keyCode;
							// Tab, Return, Escape
							if (c == 0x30 || c == 0x24 || c == 0x35) {
								// NSTextField can't get keyDown event, so we manually invoke this event
								[t keyDown:event];
								return;
							}
						}
					}
				}
				if (view == self) {
					view = self.contentView;
					if (view != nil) {
						[self makeFirstResponder:view];
					}
				}
			}
			break;
		case NSEventTypeMouseMoved:
			{
				NSPoint pt = [event locationInWindow];
				NSView* hit = nil;
				NSView* content = [self contentView];
				if (content != nil) {
					hit = [content hitTest:pt];
				}
				if (m_handleLastHitMouse != hit) {
					if (m_handleLastHitMouse != nil) {
						if ([m_handleLastHitMouse isKindOfClass:[SLIBViewHandle class]]) {
							[(SLIBViewHandle*)m_handleLastHitMouse onMouseExited:event];
						}
					}
					if (hit != nil) {
						if ([hit isKindOfClass:[SLIBViewHandle class]]) {
							[(SLIBViewHandle*)hit onMouseEntered:event];
						}
					}
					m_handleLastHitMouse = hit;
				}
				if (hit != nil) {
					[hit mouseMoved:event];
				}
			}
			return;
		default:
			break;
	}
	[super sendEvent:event];
}

@end

#endif
