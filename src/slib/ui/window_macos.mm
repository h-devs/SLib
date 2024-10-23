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

#include "slib/ui/definition.h"

#if defined(SLIB_UI_IS_MACOS)

#include "window.h"

#include "slib/ui/platform.h"

#include "view_macos.h"

namespace slib
{
	namespace
	{
		class macOS_WindowInstance;
	}
}

@interface SLIBWindowHandle : NSWindow<NSWindowDelegate> {

	@public slib::WeakRef<slib::macOS_WindowInstance> m_instance;

	@public sl_bool m_flagStateResizingWidth;

	@private NSView* m_handleLastHitMouse;

}
@end

namespace slib
{

	namespace
	{
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
				styleMask = NSWindowStyleMaskTitled;
			} else {
				if (window->isBorderless()) {
					styleMask = NSWindowStyleMaskBorderless | NSWindowStyleMaskMiniaturizable;
				} else {
					if (window->isTitleBarVisible()) {
						styleMask = NSWindowStyleMaskTitled;
					} else {
						styleMask = NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable;
					}
					if (window->isCloseButtonEnabled()) {
						styleMask |= NSWindowStyleMaskClosable;
					}
					if (window->isMinimizeButtonEnabled()) {
						styleMask |= NSWindowStyleMaskMiniaturizable;
					}
					if (window->isResizable()) {
						styleMask |= NSWindowStyleMaskResizable;
					}
				}
			}
			return styleMask;
		}

		class macOS_WindowInstance : public WindowInstance
		{
		public:
			NSWindow* m_handle;
			__weak NSWindow* m_parent;

			sl_ui_len m_heightScreen;

			AtomicRef<ViewInstance> m_viewContent;

			sl_bool m_flagClosing;
			sl_bool m_flagSheet;
			sl_bool m_flagDoModal;

		public:
			macOS_WindowInstance()
			{
				m_handle = nil;
				m_parent = nil;

				m_flagClosing = sl_false;
				m_flagSheet = sl_false;
				m_flagDoModal = sl_false;
			}

			~macOS_WindowInstance()
			{
				release();
			}

		public:
			static Ref<macOS_WindowInstance> create(NSWindow* handle)
			{
				if (handle != nil) {
					sl_ui_pos heightScreen;
					NSScreen* screen = [handle screen];
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
						ret->m_handle = handle;
						ret->m_heightScreen = heightScreen;
						NSView* view = [handle contentView];
						if (view != nil) {
							Ref<ViewInstance> content = UIPlatform::createViewInstance(view);
							if (content.isNotNull()) {
								content->setWindowContent(sl_true);
								ret->m_viewContent = content;
								if ([view isKindOfClass:[SLIBViewHandle class]]) {
									((SLIBViewHandle*)view)->m_viewInstance = Ref<PlatformViewInstance>::cast(content);
								}
							}
						}
						UIPlatform::registerWindowInstance(handle, ret.get());
						return ret;
					}
				}
				return sl_null;
			}

			static Ref<WindowInstance> create(Window* window)
			{
				Ref<WindowInstance> _parent;
				NSWindow* parent = (__bridge NSWindow*)(window->getParentHandle(_parent));

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

						handle->m_instance = ret;

						{
							NSString* title = Apple::getNSStringFromString(window->getTitle());
							[handle setTitle: title];

							Color _color = window->getBackgroundColor();
							if (_color.isNotZero()) {
								NSColor* color = GraphicsPlatform::getNSColorFromColor(_color);
								[handle setBackgroundColor:color];
							} else {
								if (window->isLayered()) {
									[handle setBackgroundColor:[NSColor clearColor]];
									[handle setHasShadow:NO];
								}
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

							if (window->isExcludingFromCapture()) {
								[handle setSharingType:NSWindowSharingNone];
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

		public:
			void* getHandle() override
			{
				return (__bridge void*)m_handle;
			}

			void close() override
			{
				m_viewContent.setNull();
				NSWindow* handle = m_handle;
				if (handle != nil) {
					UIPlatform::removeWindowInstance(handle);
					if (m_flagSheet) {
						m_flagSheet = sl_false;
						NSWindow* parent = m_parent;
						if (parent != nil) {
							[parent endSheet: handle];
							return;
						}
					}
					if (m_flagDoModal) {
						m_flagDoModal = sl_false;
						[NSApp stopModal];
					}
					if (!m_flagClosing) {
						[handle close];
					}
					m_handle = nil;
				}
			}

			sl_bool isClosed() override
			{
				return m_handle == nil;
			}


			void setParentHandle(void* parent) override
			{
				NSWindow* handle = m_handle;
				if (handle != nil) {
					if (parent) {
						NSWindow* p = (__bridge NSWindow*)parent;
						m_parent = p;
						if (p != nil && handle.parentWindow == nil) {
							[p addChildWindow:handle ordered:NSWindowAbove];
						}
					} else {
						NSWindow* p = handle.parentWindow;
						if (p != nil) {
							[p removeChildWindow:handle];
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
				NSWindow* handle = m_handle;
				if (handle != nil) {
					NSRect rect = [handle frame];
					ToUIRect(_out, rect, m_heightScreen);
					return sl_true;
				}
				return sl_false;
			}

			void setFrame(const UIRect& frame) override
			{
				NSWindow* handle = m_handle;
				if (handle != nil) {
					NSRect rect;
					ToNSRect(rect, frame, m_heightScreen);
					[handle setFrame:rect display:TRUE];
				}
			}

			void setTitle(const String& title) override
			{
				NSWindow* handle = m_handle;
				if (handle != nil) {
					[handle setTitle: Apple::getNSStringFromString(title)];
				}
			}

			sl_bool isActive() override
			{
				NSWindow* handle = m_handle;
				if (handle != nil) {
					return [handle isKeyWindow];
				}
				return sl_false;
			}

			void activate() override
			{
				NSWindow* handle = m_handle;
				if (handle != nil) {
					[handle makeKeyAndOrderFront:NSApp];
				}
			}

			void setBackgroundColor(const Color& _color) override
			{
				NSWindow* handle = m_handle;
				if (handle != nil) {
					NSColor* color = GraphicsPlatform::getNSColorFromColor(_color);
					[handle setBackgroundColor:color];
				}
			}

			void resetBackgroundColor() override
			{
				NSWindow* handle = m_handle;
				if (handle != nil) {
					[handle setBackgroundColor:[NSColor windowBackgroundColor]];
				}
			}

			void isMinimized(sl_bool& _out) override
			{
				NSWindow* handle = m_handle;
				if (handle != nil) {
					_out = [handle isMiniaturized] ? sl_true : sl_false;
				}
			}

			void setMinimized(sl_bool flag) override
			{
				NSWindow* handle = m_handle;
				if (handle != nil) {
					if (flag) {
						[handle miniaturize:nil];
					} else {
						[handle deminiaturize:nil];
					}
				}
			}

			void isMaximized(sl_bool& _out) override
			{
				NSWindow* handle = m_handle;
				if (handle != nil) {
					_out = [handle isZoomed] ? sl_true : sl_false;
				}
			}

			void setMaximized(sl_bool flag) override
			{
				NSWindow* handle = m_handle;
				if (handle != nil) {
					sl_bool f1 = [handle isZoomed] ? sl_true : sl_false;
					sl_bool f2 = flag ? sl_true : sl_false;
					if (f1 != f2) {
						[handle zoom:nil];
						UI::dispatchToUiThread([handle]() {
							[handle invalidateShadow];
						}, 100);
					}
				}
			}

			void isFullScreen(sl_bool& _out) override
			{
				NSWindow* handle = m_handle;
				if (handle != nil) {
					_out = (handle.styleMask & NSWindowStyleMaskFullScreen) ? sl_true : sl_false;
				}
			}

			void setFullScreen(sl_bool flag) override
			{
				NSWindow* handle = m_handle;
				if (handle != nil) {
					sl_bool f1 = (handle.styleMask & NSWindowStyleMaskFullScreen) ? sl_true : sl_false;
					sl_bool f2 = flag ? sl_true : sl_false;
					if (f1 != f2) {
						[handle toggleFullScreen:nil];
						if (flag) {
							setFullScreenButtonEnabled(sl_true);
						}
					}
				}
			}

			void setVisible(sl_bool flag) override
			{
				NSWindow* handle = m_handle;
				if (handle != nil) {
					if (flag) {
						NSWindow* parent = m_parent;
						if (parent != nil) {
							if (handle.parentWindow == nil) {
								[parent addChildWindow:handle ordered:NSWindowAbove];
							}
						} else {
							[handle orderFrontRegardless];
						}
					} else {
						[handle orderOut:nil];
					}
				}
			}

			void setAlwaysOnTop(sl_bool flag) override
			{
				NSWindow* handle = m_handle;
				if (handle != nil) {
					if (flag) {
						[handle setLevel:NSFloatingWindowLevel];
					} else {
						[handle setLevel:NSNormalWindowLevel];
					}
				}
			}

			void setCloseButtonEnabled(sl_bool flag) override
			{
				NSWindow* handle = m_handle;
				if (handle != nil) {
					NSUInteger style = [handle styleMask];
					if (!(style & NSWindowStyleMaskTitled)) {
						return;
					}
					sl_bool f1 = (style & NSWindowStyleMaskClosable) ? sl_true : sl_false;
					sl_bool f2 = flag ? sl_true : sl_false;
					if (f1 != f2) {
						if (flag) {
							style = style | NSWindowStyleMaskClosable;
						} else {
							style = style & (~NSWindowStyleMaskClosable);
						}
						[handle setStyleMask:style];
					}
				}
			}

			void setMinimizeButtonEnabled(sl_bool flag) override
			{
				NSWindow* handle = m_handle;
				if (handle != nil) {
					NSUInteger style = [handle styleMask];
					if (!(style & NSWindowStyleMaskTitled)) {
						return;
					}
					sl_bool f1 = (style & NSWindowStyleMaskMiniaturizable) ? sl_true : sl_false;
					sl_bool f2 = flag ? sl_true : sl_false;
					if (f1 != f2) {
						if (flag) {
							style = style | NSWindowStyleMaskMiniaturizable;
						} else {
							style = style & (~NSWindowStyleMaskMiniaturizable);
						}
						[handle setStyleMask:style];
					}
				}
			}

			void setFullScreenButtonEnabled(sl_bool flag) override
			{
				NSWindow* handle = m_handle;
				if (handle != nil) {
					NSWindowCollectionBehavior behavior;
					if (flag || (handle.styleMask & NSWindowStyleMaskFullScreen)) {
						behavior = NSWindowCollectionBehaviorFullScreenPrimary;
					} else {
						behavior = NSWindowCollectionBehaviorFullScreenAuxiliary;
					}
					if (behavior != handle.collectionBehavior) {
						handle.collectionBehavior = behavior;
					}
				}
			}

			void setResizable(sl_bool flag) override
			{
				NSWindow* handle = m_handle;
				if (handle != nil) {
					NSUInteger style = [handle styleMask];
					sl_bool f1 = (style & NSWindowStyleMaskResizable) ? sl_true : sl_false;
					sl_bool f2 = flag ? sl_true : sl_false;
					if (f1 != f2) {
						if (flag) {
							style = style | NSWindowStyleMaskResizable;
						} else {
							style = style & (~NSWindowStyleMaskResizable);
						}
						[handle setStyleMask:style];
					}
				}
			}

			void setAlpha(sl_real _alpha) override
			{
				NSWindow* handle = m_handle;
				if (handle != nil) {
					sl_real alpha = _alpha;
					if (alpha < 0) {
						alpha = 0;
					}
					if (alpha > 1) {
						alpha = 1;
					}
					[handle setAlphaValue:alpha];
				}
			}

			void setTransparent(sl_bool flag) override
			{
				NSWindow* handle = m_handle;
				if (handle != nil) {
					[handle setIgnoresMouseEvents:(flag?TRUE:FALSE)];
				}
			}

			sl_bool getClientInsets(UIEdgeInsets& _out) override
			{
				NSWindow* handle = m_handle;
				if (handle != nil) {
					NSRect frame = [handle frame];
					NSRect client = [handle contentRectForFrameRect:frame];
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
				NSWindow* handle = m_handle;
				if (handle != nil) {
					if (m_flagSheet) {
						return sl_false;
					}
					m_flagDoModal = sl_true;
					[NSApp runModalForWindow:handle];
					return sl_true;
				}
				return sl_false;
			}

			void doPostCreate() override
			{
				NSWindow* handle = m_handle;
				if (handle != nil) {
					NSRect frame = [handle frame];
					NSRect client = [handle contentRectForFrameRect:frame];
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


	Ref<WindowInstance> UIPlatform::createWindowInstance(NSWindow* handle)
	{
		Ref<WindowInstance> ret = UIPlatform::_getWindowInstance((__bridge void*)handle);
		if (ret.isNotNull()) {
			return ret;
		}
		return macOS_WindowInstance::create(handle);
	}

	void UIPlatform::registerWindowInstance(NSWindow* handle, WindowInstance* instance)
	{
		UIPlatform::_registerWindowInstance((__bridge void*)handle, instance);
	}

	Ref<WindowInstance> UIPlatform::getWindowInstance(NSWindow* handle)
	{
		return UIPlatform::_getWindowInstance((__bridge void*)handle);
	}

	void UIPlatform::removeWindowInstance(NSWindow* handle)
	{
		UIPlatform::_removeWindowInstance((__bridge void*)handle);
	}

	NSWindow* UIPlatform::getWindowHandle(WindowInstance* _instance)
	{
		macOS_WindowInstance* instance = (macOS_WindowInstance*)_instance;
		if (instance) {
			return instance->m_handle;
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
					return _instance->m_handle;
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
		NSWindow* handle = [NSApp keyWindow];
		if (handle != nil) {
			return handle;
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
	Ref<macOS_WindowInstance> instance = m_instance;
	if (instance.isNotNull()) {
		instance->m_flagClosing = sl_true;
		if (instance->onClose()) {
			instance->close();
			ret = YES;
		} else {
			ret = NO;
		}
		instance->m_flagClosing = sl_false;
	}
	return ret;
}

- (void)windowWillClose:(NSNotification *)notification
{
	Ref<macOS_WindowInstance> instance = m_instance;
	if (instance.isNotNull()) {
		UI::dispatchToUiThread(SLIB_FUNCTION_REF(Move(instance), release));
	}
}

- (void)windowDidBecomeKey:(NSNotification *)notification
{
	Ref<macOS_WindowInstance> instance = m_instance;
	if (instance.isNotNull()) {
		instance->onActivate();
	}
}

- (void)windowDidResignKey:(NSNotification *)notification
{
	Ref<macOS_WindowInstance> instance = m_instance;
	if (instance.isNotNull()) {
		instance->onDeactivate();
	}
}

- (NSSize)windowWillResize:(NSWindow *)sender toSize:(NSSize)frameSize
{
	Ref<macOS_WindowInstance> instance = m_instance;
	if (instance.isNotNull()) {
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
		instance->onResizing(size, m_flagStateResizingWidth);
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
	Ref<macOS_WindowInstance> instance = m_instance;
	if (instance.isNotNull()) {
		NSRect fr = [self frame];
		NSRect cr = [self contentRectForFrameRect:fr];
		instance->onResize((sl_ui_len)(cr.size.width), (sl_ui_len)(cr.size.height));
	}
}

- (void)windowDidMove:(NSNotification *)notification
{
	Ref<macOS_WindowInstance> instance = m_instance;
	if (instance.isNotNull()) {
		instance->onMove();
	}
}

- (void)windowWillMiniaturize:(NSNotification *)notification
{
	Ref<macOS_WindowInstance> instance = m_instance;
	if (instance.isNotNull()) {
		instance->onMinimize();
	}
}

- (void)windowDidDeminiaturize:(NSNotification *)notification
{
	Ref<macOS_WindowInstance> instance = m_instance;
	if (instance.isNotNull()) {
		instance->onDeminimize();
	}
}

- (void)windowDidEnterFullScreen:(NSNotification *)notification
{
	Ref<macOS_WindowInstance> instance = m_instance;
	if (instance.isNotNull()) {
		instance->onEnterFullScreen();
	}
}

- (void)windowDidExitFullScreen:(NSNotification *)notification
{
	Ref<macOS_WindowInstance> instance = m_instance;
	if (instance.isNotNull()) {
		instance->onExitFullScreen();
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
				break;
			}
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
					return;
				}
				break;
			}
		default:
			break;
	}
	[super sendEvent:event];
}

@end

#endif
