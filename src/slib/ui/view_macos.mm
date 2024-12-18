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

#include "view_macos.h"

#include "slib/ui/view.h"
#include "slib/ui/drag.h"
#include "slib/ui/text.h"
#include "slib/math/transform2d.h"

@interface SLIBDraggingSource : NSObject<NSDraggingSource>
{
	@public slib::DragContext context;
}
@end

@interface SLIBToolTip : NSObject
{
	@public sl_uint64 m_id;
	@public slib::String m_text;
	@public NSString* m_content;
}
@end

namespace slib
{

	SLIB_DEFINE_OBJECT(PlatformViewInstance, ViewInstance)

	PlatformViewInstance::PlatformViewInstance()
	{
		m_handle = nil;
		m_frame.setZero();
		m_transform = Matrix3::identity();
	}

	PlatformViewInstance::~PlatformViewInstance()
	{
		_release();
	}

	void PlatformViewInstance::_release()
	{
		UIPlatform::removeViewInstance(m_handle);
		m_handle = nil;
	}

	void PlatformViewInstance::initWithHandle(NSView* handle)
	{
		m_handle = handle;
		UIPlatform::registerViewInstance(handle, this);
	}

	namespace
	{
		static void SetDropTarget(NSView* handle, sl_bool flag)
		{
			if (flag) {
				if (handle.registeredDraggedTypes == nil || handle.registeredDraggedTypes.count == 0) {
					if (@available(macOS 10.13, *)) {
						[handle registerForDraggedTypes:@[NSPasteboardTypeString, NSPasteboardTypeFileURL]];
					} else {
						[handle registerForDraggedTypes:@[NSPasteboardTypeString, NSFilenamesPboardType]];
					}
				}
			} else {
				if (handle.registeredDraggedTypes != nil && handle.registeredDraggedTypes.count) {
					[handle unregisterDraggedTypes];
				}
			}
		}
	}

	void PlatformViewInstance::initWithHandle(NSView* handle, NSView* parent, View* view)
	{
		initWithHandle(handle);

		if (view->isCreatingNativeLayer()) {
			[handle setWantsLayer:YES];
		}

		[handle setHidden:(view->isVisibleInInstance() ? NO : YES)];
		if (!(view->isEnabled())) {
			if ([handle isKindOfClass:[NSControl class]]) {
				NSControl* control = (NSControl*)(handle);
				[control setEnabled:FALSE];
			}
		}
		if ([handle isKindOfClass:[SLIBViewBaseHandle class]]) {
			((SLIBViewBaseHandle*)handle)->m_flagOpaque = view->isOpaque();
			((SLIBViewBaseHandle*)handle)->m_flagClipping = view->isClipping();
			((SLIBViewBaseHandle*)handle)->m_flagDrawing = view->isDrawing();
		}
		if (parent != nil) {
			[parent addSubview:handle];
		}

		CALayer* layer = handle.layer;
		if (layer != nil) {
			float shadowOpacity = view->getShadowOpacity();
			if (shadowOpacity > SLIB_EPSILON) {
				layer.shadowOpacity = shadowOpacity;
				layer.shadowRadius = (CGFloat)(view->getShadowRadius());
				UIPointF offset = view->getShadowOffset();
				layer.shadowOffset = CGSizeMake((CGFloat)(offset.x), (CGFloat)(offset.y));
				CGColorRef color = GraphicsPlatform::getCGColorFromColor(view->getShadowColor());
				if (color) {
					layer.shadowColor = color;
					CFRelease(color);
				}
			}
		}

		if (view->isDropTarget()) {
			SetDropTarget(handle, sl_true);
		}
	}

	NSView* PlatformViewInstance::getHandle()
	{
		return m_handle;
	}

	sl_bool PlatformViewInstance::isValid(View* view)
	{
		return sl_true;
	}

	void PlatformViewInstance::setFocus(View* view, sl_bool flagFocus)
	{
		NSView* handle = m_handle;
		if (handle != nil) {
			NSWindow* window = [handle window];
			if (window != nil) {
				if (flagFocus) {
					m_flagSettingFocus = sl_true;
					[window makeFirstResponder:handle];
					m_flagSettingFocus = sl_false;
				} else {
					if (window.firstResponder == handle) {
						[window makeFirstResponder:nil];
					}
				}
			}
		}
	}

	void PlatformViewInstance::invalidate(View* view)
	{
		NSView* handle = m_handle;
		if (handle != nil) {
			if ([NSThread isMainThread]) {
				[handle setNeedsDisplay: YES];
				dispatch_async(dispatch_get_main_queue(), ^{
					[handle displayIfNeeded];
				});
			} else {
				dispatch_async(dispatch_get_main_queue(), ^{
					[handle setNeedsDisplay: YES];
					[handle displayIfNeeded];
				});
			}
		}
	}

	void PlatformViewInstance::invalidate(View* view, const UIRect& rect)
	{
		NSView* handle = m_handle;
		if (handle != nil) {
			NSRect _rect;
			_rect.origin.x = (CGFloat)(rect.left);
			_rect.origin.y = (CGFloat)(rect.top);
			_rect.size.width = (CGFloat)(rect.getWidth());
			_rect.size.height = (CGFloat)(rect.getHeight());
			if ([NSThread isMainThread]) {
				[handle setNeedsDisplayInRect: _rect];
				dispatch_async(dispatch_get_main_queue(), ^{
					[handle displayIfNeeded];
				});
			} else {
				dispatch_async(dispatch_get_main_queue(), ^{
					[handle setNeedsDisplayInRect: _rect];
					[handle displayIfNeeded];
				});
			}
		}
	}

	void PlatformViewInstance::setFrame(View* view, const UIRect& frame)
	{
		m_frame = frame;
		updateFrameAndTransform();
	}

	void PlatformViewInstance::setTransform(View* view, const Matrix3& transform)
	{
		m_transform = transform;
		updateFrameAndTransform();
	}

	void PlatformViewInstance::setVisible(View* view, sl_bool flag)
	{
		NSView* handle = m_handle;
		if (handle != nil) {
			[handle setHidden:(flag ? NO : YES)];
		}
	}

	void PlatformViewInstance::setEnabled(View* view, sl_bool flag)
	{
		NSView* handle = m_handle;
		if (handle != nil) {
			if ([handle isKindOfClass:[NSControl class]]) {
				NSControl* control = (NSControl*)handle;
				[control setEnabled:(flag ? YES : NO)];
			}
		}
	}

	void PlatformViewInstance::setOpaque(View* view, sl_bool flag)
	{
		NSView* handle = m_handle;
		if (handle != nil) {
			if ([handle isKindOfClass:[SLIBViewBaseHandle class]]) {
				SLIBViewBaseHandle* control = (SLIBViewBaseHandle*)handle;
				control->m_flagOpaque = flag;
			}
		}
	}

	void PlatformViewInstance::setAlpha(View* view, sl_real alpha)
	{
		NSView* handle = m_handle;
		if (handle != nil) {
			[handle setNeedsDisplay: TRUE];
		}
	}

	void PlatformViewInstance::setClipping(View* view, sl_bool flag)
	{
		NSView* handle = m_handle;
		if (handle != nil) {
			if ([handle isKindOfClass:[SLIBViewBaseHandle class]]) {
				SLIBViewBaseHandle* control = (SLIBViewBaseHandle*)handle;
				control->m_flagClipping = flag;
			}
		}
	}

	void PlatformViewInstance::setDrawing(View* view, sl_bool flag)
	{
		NSView* handle = m_handle;
		if (handle != nil) {
			if ([handle isKindOfClass:[SLIBViewBaseHandle class]]) {
				SLIBViewBaseHandle* control = (SLIBViewBaseHandle*)handle;
				control->m_flagDrawing = flag;
			}
		}
	}

	UIPointF PlatformViewInstance::convertCoordinateFromScreenToView(View* view, const UIPointF& ptScreen)
	{
		NSView* handle = m_handle;
		if (handle != nil) {
			NSWindow* window = [handle window];
			if (window != nil) {
				NSScreen* screen = [window screen];
				NSRect rect;
				rect.origin.x = (CGFloat)(ptScreen.x);
				rect.origin.y = [screen frame].size.height - 1 - (CGFloat)(ptScreen.y);
				rect.size.width = 0;
				rect.size.height = 0;
				rect = [window convertRectFromScreen:rect];
				NSPoint pw;
				pw.x = rect.origin.x;
				pw.y = rect.origin.y;
				NSPoint pt = [handle convertPoint:pw fromView:nil];
				UIPointF ret;
				ret.x = (sl_ui_posf)(pt.x);
				ret.y = (sl_ui_posf)(pt.y);
				return ret;
			}
		}
		return ptScreen;
	}

	UIPointF PlatformViewInstance::convertCoordinateFromViewToScreen(View* view, const UIPointF& ptView)
	{
		NSView* handle = m_handle;
		if (handle != nil) {
			NSWindow* window = [handle window];
			if (window != nil) {
				NSScreen* screen = [window screen];
				NSPoint pt;
				pt.x = (CGFloat)(ptView.x);
				pt.y = (CGFloat)(ptView.y);
				NSPoint pw = [handle convertPoint:pt toView:nil];
				NSRect rect;
				rect.origin.x = pw.x;
				rect.origin.y = pw.y;
				rect.size.width = 0;
				rect.size.height = 0;
				rect = [window convertRectToScreen:rect];
				UIPointF ret;
				ret.x = (sl_ui_posf)(rect.origin.x);
				ret.y = (sl_ui_posf)([screen frame].size.height - 1 - rect.origin.y);
				return ret;
			}
		}
		return ptView;
	}

	void PlatformViewInstance::addChildInstance(View* view, const Ref<ViewInstance>& _child)
	{
		NSView* handle = m_handle;
		if (handle != nil) {
			PlatformViewInstance* child = (PlatformViewInstance*)(_child.get());
			if (child) {
				NSView* child_handle = child->m_handle;
				if (child_handle != nil) {
					[handle addSubview:child_handle];
				}
			}
		}
	}

	void PlatformViewInstance::removeChildInstance(View* view, const Ref<ViewInstance>& _child)
	{
		PlatformViewInstance* child = (PlatformViewInstance*)(_child.get());
		if (child) {
			NSView* child_handle = child->m_handle;
			if (child_handle != nil) {
				[child_handle removeFromSuperview];
			}
		}
	}

	void PlatformViewInstance::bringToFront(View* view)
	{
		NSView* handle = m_handle;
		if (handle != nil) {
			NSView* parent = handle.superview;
			if (parent != nil) {
				[handle removeFromSuperviewWithoutNeedingDisplay];
				[parent addSubview:handle];
			}
		}
	}

	void PlatformViewInstance::setShadowOpacity(View* view, float opacity)
	{
		NSView* handle = m_handle;
		if (handle != nil) {
			CALayer* layer = handle.layer;
			if (layer != nil) {
				layer.shadowOpacity = opacity;
			}
		}
	}

	void PlatformViewInstance::setShadowRadius(View* view, sl_ui_posf radius)
	{
		NSView* handle = m_handle;
		if (handle != nil) {
			CALayer* layer = handle.layer;
			if (layer != nil) {
				layer.shadowRadius = (CGFloat)radius;
			}
		}
	}

	void PlatformViewInstance::setShadowOffset(View* view, sl_ui_posf x, sl_ui_posf y)
	{
		NSView* handle = m_handle;
		if (handle != nil) {
			CALayer* layer = handle.layer;
			if (layer != nil) {
				layer.shadowOffset = CGSizeMake((CGFloat)x, (CGFloat)y);
			}
		}
	}

	void PlatformViewInstance::setShadowColor(View* view, const Color& _color)
	{
		NSView* handle = m_handle;
		if (handle != nil) {
			CALayer* layer = handle.layer;
			if (layer != nil) {
				CGColorRef color = GraphicsPlatform::getCGColorFromColor(_color);
				if (color) {
					layer.shadowColor = color;
					CFRelease(color);
				}
			}
		}
	}

	void PlatformViewInstance::setDropTarget(View* view, sl_bool flag)
	{
		NSView* handle = m_handle;
		if (handle != nil) {
			SetDropTarget(handle, flag);
		}
	}

	void PlatformViewInstance::updateToolTip(sl_uint64 ownerId, const String& toolTip)
	{
		NSView* handle = m_handle;
		if (handle == nil) {
			return;
		}
		if (toolTip.isEmpty()) {
			if (m_toolTip != nil) {
				[handle removeAllToolTips];
				m_toolTip = nil;
			}
			return;
		}
		SLIBToolTip* obj = m_toolTip;
		if (obj != nil && obj->m_id == ownerId && obj->m_text == toolTip) {
			return;
		}
		if (obj == nil) {
			obj = [[SLIBToolTip alloc] init];
			m_toolTip = obj;
		}
		obj->m_id = ownerId;
		obj->m_text = toolTip;
		obj->m_content = Apple::getNSStringFromString(toolTip);
		NSSize size = [handle frame].size;
		[handle removeAllToolTips];
		[handle addToolTipRect:NSMakeRect(0, 0, size.width, size.height) owner:obj userData:NULL];
	}

	NSRect PlatformViewInstance::getViewFrameAndTransform(const UIRect& frame, const Matrix3& transform, sl_real& rotation)
	{
		rotation = Transform2::getRotationAngleFromMatrix(transform);
		Vector2 translation = Transform2::getTranslationFromMatrix(transform);
		NSRect ret;
		ret.origin.x = frame.left + translation.x;
		ret.origin.y = frame.top + translation.y;
		ret.size.width = frame.getWidth();
		ret.size.height = frame.getHeight();
		if (!(Math::isAlmostZero(rotation))) {
			sl_real ax = frame.getWidth() / 2;
			sl_real ay = frame.getHeight() / 2;
			sl_real cr = Math::cos(rotation);
			sl_real sr = Math::sin(rotation);
			ret.origin.x += (- ax * cr + ay * sr) + ax;
			ret.origin.y += (- ax * sr - ay * cr) + ay;
		}
		return ret;
	}

	void PlatformViewInstance::onDraw(NSRect rcDirty)
	{
		Ref<View> view = getView();
		if (view.isNull()) {
			return;
		}
		NSView* handle = m_handle;
		if (handle == nil) {
			return;
		}
		NSGraphicsContext* graphics = [NSGraphicsContext currentContext];
		if (graphics == nil) {
			return;
		}

		CGContextRef context = (CGContextRef)([graphics graphicsPort]);
		NSRect rectBound = [handle bounds];
		Ref<Canvas> canvas = GraphicsPlatform::createCanvas(CanvasType::View, context, (sl_uint32)(rectBound.size.width), (sl_uint32)(rectBound.size.height));
		if (canvas.isNull()) {
			return;
		}

		canvas->setInvalidatedRect(Rectangle((sl_real)(rcDirty.origin.x), (sl_real)(rcDirty.origin.y), (sl_real)(rcDirty.origin.x + rcDirty.size.width), (sl_real)(rcDirty.origin.y + rcDirty.size.height)));

		sl_real alpha = view->getAlpha();
		Ref<View> parent = view->getParent();
		while (parent.isNotNull()) {
			alpha *= parent->getAlpha();
			parent = parent->getParent();
		}
		if (alpha < 0.005f) {
			return;
		}
		if (alpha < 0.995f) {
			canvas->setAlpha(alpha);
		}

		view->dispatchDraw(canvas.get());
	}

	UIEventFlags PlatformViewInstance::onEventKey(sl_bool flagDown, NSEvent* event)
	{
		NSView* handle = m_handle;
		if (handle != nil) {
			UIAction action = flagDown ? UIAction::KeyDown : UIAction::KeyUp;
			sl_uint32 vkey = [event keyCode];
			Keycode key = UIEvent::getKeycodeFromSystemKeycode(vkey);
			Time t;
			t.setSecondCountF([event timestamp]);
			Ref<UIEvent> ev = UIEvent::createKeyEvent(action, key, vkey, t);
			if (ev.isNotNull()) {
				UIPlatform::applyEventModifiers(ev.get(), event);
				onKeyEvent(ev.get());
				UIEventFlags flags = ev->getFlags();
				if (flagDown) {
					if (flags & UIEventFlags::NotInvokeNative) {
						return flags;
					}
					Ref<View> view = m_view;
					if (view.isNotNull()) {
						Ref<View> focus = view->getFocusedView();
						if (focus.isNotNull()) {
							if (focus->isUsingIME()) {
								[handle interpretKeyEvents:[NSArray arrayWithObject:event]];
								flags |= UIEventFlags::NotInvokeNative;
							}
						}
					}
				}
				return flags;
			}
		}
		return 0;
	}

	void PlatformViewInstance::onEventChar(sl_char32 code)
	{
		NSView* handle = m_handle;
		if (handle != nil) {
			Ref<UIEvent> ev = UIEvent::createKeyEvent(UIAction::Char, Keycode::Unknown, code, Time::now());
			if (ev.isNotNull()) {
				onKeyEvent(ev.get());
			}
		}
	}

	UIEventFlags PlatformViewInstance::onEventMouse(UIAction action, NSEvent* event)
	{
		NSView* handle = m_handle;
		if (handle != nil) {
			NSWindow* window = [handle window];
			if (window != nil) {
				NSPoint pw = [event locationInWindow];
				NSPoint pt = [handle convertPoint:pw fromView:nil];
				sl_ui_posf x = (sl_ui_posf)(pt.x);
				sl_ui_posf y = (sl_ui_posf)(pt.y);
				Time t;
				t.setSecondCountF([event timestamp]);
				Ref<UIEvent> ev = UIEvent::createMouseEvent(action, x, y, t);
				if (ev.isNotNull()) {
					switch (action) {
						case UIAction::LeftButtonDown:
						case UIAction::RightButtonDown:
						case UIAction::MiddleButtonDown:
						case UIAction::LeftButtonDoubleClick:
						case UIAction::RightButtonDoubleClick:
						case UIAction::MiddleButtonDoubleClick:
							{
								sl_bool flagCapture = sl_false;
								Ref<View> view = getView();
								if (view.isNotNull()) {
									if (view->isEnabled()) {
										if (view->isCapturingChildInstanceEvents((sl_ui_pos)(pt.x), (sl_ui_pos)(pt.y))) {
											flagCapture = sl_true;
										}
									}
								}
								if (!flagCapture) {
									for (NSView* subview in [handle subviews]) {
										if (!(subview.isHidden)) {
											NSRect frame = subview.frame;
											if (NSPointInRect(pt, frame)) {
												ev->addFlag(UIEventFlags::NotDispatchToChildren);
												break;
											}
										}
									}
								}
							}
							break;
						case UIAction::LeftButtonDrag:
						case UIAction::RightButtonDrag:
						case UIAction::MiddleButtonDrag:
						case UIAction::LeftButtonUp:
						case UIAction::RightButtonUp:
						case UIAction::MiddleButtonUp:
						case UIAction::MouseMove:
						case UIAction::MouseEnter:
						case UIAction::MouseLeave:
							break;
						default:
							break;
					}
					UIPlatform::applyEventModifiers(ev.get(), event);
					onMouseEvent(ev.get());
					return ev->getFlags();
				}
			}
		}
		return 0;
	}

	UIEventFlags PlatformViewInstance::onEventMouse(UIAction action, const NSPoint& pt)
	{
		NSView* handle = m_handle;
		if (handle != nil) {
			NSWindow* window = [handle window];
			if (window != nil) {
				sl_ui_posf x = (sl_ui_posf)(pt.x);
				sl_ui_posf y = (sl_ui_posf)(pt.y);
				Time t;
				t.setSecondCountF([[NSProcessInfo processInfo] systemUptime]);
				Ref<UIEvent> ev = UIEvent::createMouseEvent(action, x, y, t);
				if (ev.isNotNull()) {
					onMouseEvent(ev.get());
					return ev->getFlags();
				}
			}
		}
		return 0;
	}

	UIEventFlags PlatformViewInstance::onEventMouseWheel(NSEvent* event)
	{
		NSView* handle = m_handle;
		if (handle != nil) {
			sl_real deltaX = (sl_real)([event deltaX]);
			sl_real deltaY = (sl_real)([event deltaY]);
			if (Math::isAlmostZero(deltaX) && Math::isAlmostZero(deltaY)) {
				return sl_false;
			}
			NSPoint pw = [event locationInWindow];
			NSPoint pt = [handle convertPoint:pw fromView:nil];
			sl_ui_posf x = (sl_ui_posf)(pt.x);
			sl_ui_posf y = (sl_ui_posf)(pt.y);
			Time t;
			t.setSecondCountF([event timestamp]);
			Ref<UIEvent> ev = UIEvent::createMouseWheelEvent(x, y, deltaX, deltaY, t);
			if (ev.isNotNull()) {
				UIPlatform::applyEventModifiers(ev.get(), event);
				onMouseWheelEvent(ev.get());
				return ev->getFlags();
			}
		}
		return 0;
	}

	UIEventFlags PlatformViewInstance::onEventUpdateCursor(NSEvent* event)
	{
		NSView* handle = m_handle;
		if (handle != nil) {
			NSPoint pw = [event locationInWindow];
			NSPoint pt = [handle convertPoint:pw fromView:nil];
			sl_ui_posf x = (sl_ui_posf)(pt.x);
			sl_ui_posf y = (sl_ui_posf)(pt.y);
			Time t;
			t.setSecondCountF([event timestamp]);
			Ref<UIEvent> ev = UIEvent::createSetCursorEvent(x, y, t);
			if (ev.isNotNull()) {
				onSetCursor(ev.get());
				updateToolTip(ev->getToolTipOwnerId(), ev->getToolTip());
				return ev->getFlags();
			}
		}
		return 0;
	}

	namespace
	{
		static NSDragOperation ToNSDragOperation(int op)
		{
			NSDragOperation ret = 0;
			if (op & DragOperations::Copy) {
				ret |= NSDragOperationCopy;
			}
			if (op & DragOperations::Link) {
				ret |= NSDragOperationLink;
			}
			if (op & DragOperations::Generic) {
				ret |= NSDragOperationGeneric;
			}
			if (op & DragOperations::Private) {
				ret |= NSDragOperationPrivate;
			}
			if (op & DragOperations::Move) {
				ret |= NSDragOperationMove;
			}
			if (op & DragOperations::Delete) {
				ret |= NSDragOperationDelete;
			}
			return ret;
		}

		static int FromNSDragOperation(NSDragOperation operation)
		{
			int op = 0;
			if (operation & NSDragOperationCopy) {
				op |= DragOperations::Copy;
			}
			if (operation & NSDragOperationLink) {
				op |= DragOperations::Link;
			}
			if (operation & NSDragOperationGeneric) {
				op |= DragOperations::Generic;
			}
			if (operation & NSDragOperationPrivate) {
				op |= DragOperations::Private;
			}
			if (operation & NSDragOperationMove) {
				op |= DragOperations::Move;
			}
			if (operation & NSDragOperationDelete) {
				op |= DragOperations::Delete;
			}
			return op;
		}
	}

	Ref<UIEvent> PlatformViewInstance::onEventDrop(UIAction action, id<NSDraggingInfo> info)
	{
		NSView* handle = m_handle;
		if (handle != nil) {
			DragContext context;
			context.operationMask = FromNSDragOperation(info.draggingSourceOperationMask);
			NSPasteboard* paste = info.draggingPasteboard;
			context.item.setText(Apple::getStringFromNSString([paste stringForType:NSPasteboardTypeString]));
			NSArray* fileUrls = [paste readObjectsForClasses:@[[NSURL class]] options:@{ NSPasteboardURLReadingFileURLsOnlyKey: [NSNumber numberWithBool:YES] }];
			if (fileUrls != nil) {
				List<String> files;
				for (NSURL* url in fileUrls) {
					String path = Apple::getFilePathFromNSURL(url);
					if (path.isNotEmpty()) {
						files.add_NoLock(Move(path));
					}
				}
				if (files.isNotNull()) {
					context.item.setFiles(files);
				}
			}
			NSPoint loc = info.draggingLocation;
			NSPoint pt = [handle convertPoint:loc fromView:nil];
			Ref<UIEvent> ev = UIEvent::createDragEvent(action, (sl_ui_posf)(pt.x), (sl_ui_posf)(pt.y), context, Time::now());
			if (ev.isNotNull()) {
				onDragDropEvent(ev.get());
				return ev;
			}
		}
		return sl_null;
	}

	void PlatformViewInstance::updateFrameAndTransform()
	{
		NSView* handle = m_handle;
		if (handle != nil) {
			sl_real rotation = 0;
			handle.frame = getViewFrameAndTransform(m_frame, m_transform, rotation);
			handle.frameRotation = Math::getDegreesFromRadian(rotation);
			[handle setNeedsDisplay:YES];
		}
	}


	Ref<ViewInstance> View::createTypicalInstance(ViewInstance* parent)
	{
		return PlatformViewInstance::create<PlatformViewInstance, SLIBViewHandle>(this, parent);
	}


	Ref<ViewInstance> UIPlatform::createViewInstance(NSView* handle)
	{
		Ref<ViewInstance> ret = UIPlatform::_getViewInstance((__bridge void*)handle);
		if (ret.isNotNull()) {
			return ret;
		}
		return PlatformViewInstance::create<PlatformViewInstance>(handle);
	}

	void UIPlatform::registerViewInstance(NSView* handle, ViewInstance* instance)
	{
		UIPlatform::_registerViewInstance((__bridge void*)handle, instance);
	}

	Ref<ViewInstance> UIPlatform::getViewInstance(NSView* handle)
	{
		return UIPlatform::_getViewInstance((__bridge void*)handle);
	}

	void UIPlatform::removeViewInstance(NSView* handle)
	{
		UIPlatform::_removeViewInstance((__bridge void*)handle);
	}

	NSView* UIPlatform::getViewHandle(ViewInstance* _instance)
	{
		PlatformViewInstance* instance = (PlatformViewInstance*)_instance;
		if (instance) {
			return instance->getHandle();
		} else {
			return nil;
		}
	}

	NSView* UIPlatform::getViewHandle(View* view)
	{
		if (view) {
			Ref<ViewInstance> instance = view->getViewInstance();
			if (instance.isNotNull()) {
				PlatformViewInstance* _instance = (PlatformViewInstance*)(instance.get());
				return _instance->getHandle();
			}
		}
		return nil;
	}

	sl_bool UIPlatform::measureNativeWidgetFittingSize(View* view, UISize& _out)
	{
		NSView* handle = UIPlatform::getViewHandle(view);
		if (handle != nil) {
			NSSize size = handle.fittingSize;
			_out.x = (sl_ui_len)(size.width);
			_out.y = (sl_ui_len)(size.height);
			return sl_true;
		}
		return sl_false;
	}

	sl_bool UIPlatform::measureNativeWidgetFittingSize(ViewInstance* instance, UISize& _out)
	{
		NSView* handle = UIPlatform::getViewHandle(instance);
		if (handle != nil) {
			NSSize size = handle.fittingSize;
			_out.x = (sl_ui_len)(size.width);
			_out.y = (sl_ui_len)(size.height);
			return sl_true;
		}
		return sl_false;
	}

}

using namespace slib;

@implementation SLIBViewBaseHandle

- (BOOL)isFlipped
{
	return TRUE;
}

- (BOOL)isOpaque
{
	return m_flagOpaque ? YES : NO;
}

@end

@implementation SLIBViewHandle

MACOS_VIEW_DEFINE_ON_FOCUS

- (BOOL)acceptsFirstResponder
{
	return TRUE;
}

- (void)drawRect:(NSRect)dirtyRect
{
	if (m_flagDrawing) {
		Ref<PlatformViewInstance> instance = m_viewInstance;
		if (instance.isNotNull()) {
			instance->onDraw(dirtyRect);
		}
	}
}

- (void)keyDown:(NSEvent*)theEvent
{
	Ref<PlatformViewInstance> instance = m_viewInstance;
	if (instance.isNotNull()) {
		UIEventFlags flags = instance->onEventKey(sl_true, theEvent);
		if (flags & UIEventFlags::NotInvokeNative) {
			return;
		}
	}
	[[self nextResponder] keyDown:theEvent];
}

- (void)keyUp:(NSEvent*)theEvent
{
	Ref<PlatformViewInstance> instance = m_viewInstance;
	if (instance.isNotNull()) {
		UIEventFlags flags = instance->onEventKey(sl_false, theEvent);
		if (flags & UIEventFlags::NotInvokeNative) {
			return;
		}
	}
	[[self nextResponder] keyUp:theEvent];
}

- (void)mouseDown:(NSEvent *)theEvent
{
	Ref<PlatformViewInstance> instance = m_viewInstance;
	if (instance.isNotNull()) {
		DragContext& dragContext = UIEvent::getCurrentDragContext();
		dragContext.release();
		UIEventFlags flags = instance->onEventMouse(UIAction::LeftButtonDown, theEvent);
		if (dragContext.view.isNotNull()) {
			NSDraggingItem* drag = nil;
			{
				DragItem& item = dragContext.item;
				UIRect frame = item.getFrame();
				sl_ui_pos w = frame.getWidth();
				sl_ui_pos h = frame.getHeight();
				if (w > 0 && h > 0) {
					UIRect frameInInstance = dragContext.view->getFrameInInstance();
					NSImage* dragImage = GraphicsPlatform::getNSImage(item.getDraggingImage());
					NSPasteboardItem* paste = [[NSPasteboardItem alloc] init];
					[paste setString:Apple::getNSStringFromString(item.getText()) forType:NSPasteboardTypeString];
					drag = [[NSDraggingItem alloc] initWithPasteboardWriter:paste];
					[drag setDraggingFrame:NSMakeRect((CGFloat)(frame.left + frameInInstance.left), (CGFloat)(frame.top + frameInInstance.top), (CGFloat)w, (CGFloat)h) contents:dragImage];
				}
			}
			if (drag != nil) {
				SLIBDraggingSource* source = [[SLIBDraggingSource alloc] init];
				source->context = dragContext;
				[self beginDraggingSessionWithItems:@[drag] event:theEvent source:source];
			}
		}
		if (flags & UIEventFlags::NotInvokeNative) {
			return;
		}
	}
	[[self nextResponder] mouseDown:theEvent];
}

- (void)mouseUp:(NSEvent *)theEvent
{
	Ref<PlatformViewInstance> instance = m_viewInstance;
	if (instance.isNotNull()) {
		UIEventFlags flags = instance->onEventMouse(UIAction::LeftButtonUp, theEvent);
		NSInteger clicks = [theEvent clickCount];
		if (clicks == 2) {
			flags |= instance->onEventMouse(UIAction::LeftButtonDoubleClick, theEvent);
		}
		if (flags & UIEventFlags::NotInvokeNative) {
			return;
		}
	}
	[[self nextResponder] mouseUp:theEvent];
}

- (void)mouseDragged:(NSEvent *)theEvent
{
	Ref<PlatformViewInstance> instance = m_viewInstance;
	if (instance.isNotNull()) {
		UIEventFlags flags = instance->onEventMouse(UIAction::LeftButtonDrag, theEvent);
		if (flags & UIEventFlags::NotInvokeNative) {
			return;
		}
	}
	[[self nextResponder] mouseDragged:theEvent];
}

- (void)rightMouseDown:(NSEvent *)theEvent
{
	Ref<PlatformViewInstance> instance = m_viewInstance;
	if (instance.isNotNull()) {
		UIEventFlags flags = instance->onEventMouse(UIAction::RightButtonDown, theEvent);
		if (flags & UIEventFlags::NotInvokeNative) {
			return;
		}
	}
	[[self nextResponder] rightMouseDown:theEvent];
}

- (void)rightMouseUp:(NSEvent *)theEvent
{
	Ref<PlatformViewInstance> instance = m_viewInstance;
	if (instance.isNotNull()) {
		UIEventFlags flags = instance->onEventMouse(UIAction::RightButtonUp, theEvent);
		NSInteger clicks = [theEvent clickCount];
		if (clicks == 2) {
			flags |= instance->onEventMouse(UIAction::RightButtonDoubleClick, theEvent);
		}
		if (flags & UIEventFlags::NotInvokeNative) {
			return;
		}
	}
	[[self nextResponder] rightMouseUp:theEvent];
}

- (void)rightMouseDragged:(NSEvent *)theEvent
{
	Ref<PlatformViewInstance> instance = m_viewInstance;
	if (instance.isNotNull()) {
		UIEventFlags flags = instance->onEventMouse(UIAction::RightButtonDrag, theEvent);
		if (flags & UIEventFlags::NotInvokeNative) {
			return;
		}
	}
	[[self nextResponder] rightMouseDragged:theEvent];
}

- (void)otherMouseDown:(NSEvent *)theEvent
{
	Ref<PlatformViewInstance> instance = m_viewInstance;
	if (instance.isNotNull()) {
		UIEventFlags flags = instance->onEventMouse(UIAction::MiddleButtonDown, theEvent);
		if (flags & UIEventFlags::NotInvokeNative) {
			return;
		}
	}
	[[self nextResponder] otherMouseDown:theEvent];
}

- (void)otherMouseUp:(NSEvent *)theEvent
{
	Ref<PlatformViewInstance> instance = m_viewInstance;
	if (instance.isNotNull()) {
		UIEventFlags flags = instance->onEventMouse(UIAction::MiddleButtonUp, theEvent);
		NSInteger clicks = [theEvent clickCount];
		if (clicks == 2) {
			flags |= instance->onEventMouse(UIAction::MiddleButtonDoubleClick, theEvent);
		}
		if (flags & UIEventFlags::NotInvokeNative) {
			return;
		}
	}
	[[self nextResponder] otherMouseUp:theEvent];
}

- (void)otherMouseDragged:(NSEvent *)theEvent
{
	Ref<PlatformViewInstance> instance = m_viewInstance;
	if (instance.isNotNull()) {
		UIEventFlags flags = instance->onEventMouse(UIAction::MiddleButtonDrag, theEvent);
		if (flags & UIEventFlags::NotInvokeNative) {
			return;
		}
	}
	[[self nextResponder] otherMouseDragged:theEvent];
}

- (void)mouseMoved:(NSEvent *)theEvent
{
	[[self window] invalidateCursorRectsForView:self];
	Ref<PlatformViewInstance> instance = m_viewInstance;
	if (instance.isNotNull()) {
		UIEventFlags flags = instance->onEventMouse(UIAction::MouseMove, theEvent);
		if (flags & UIEventFlags::NotInvokeNative) {
			return;
		}
	}
	[[self nextResponder] mouseMoved:theEvent];
}

-(void)onMouseEntered:(NSEvent *)theEvent
{
	Ref<PlatformViewInstance> instance = m_viewInstance;
	if (instance.isNotNull()) {
		instance->onEventMouse(UIAction::MouseEnter, theEvent);
	}
}

- (void)mouseEntered:(NSEvent *)theEvent
{
	Ref<PlatformViewInstance> instance = m_viewInstance;
	if (instance.isNotNull()) {
		UIEventFlags flags = instance->onEventMouse(UIAction::MouseEnter, theEvent);
		if (flags & UIEventFlags::NotInvokeNative) {
			return;
		}
	}
	[[self nextResponder] mouseEntered:theEvent];
}

-(void)onMouseExited:(NSEvent *)theEvent
{
	Ref<PlatformViewInstance> instance = m_viewInstance;
	if (instance.isNotNull()) {
		instance->onEventMouse(UIAction::MouseLeave, theEvent);
	}
}

- (void)mouseExited:(NSEvent *)theEvent
{
	Ref<PlatformViewInstance> instance = m_viewInstance;
	if (instance.isNotNull()) {
		UIEventFlags flags = instance->onEventMouse(UIAction::MouseLeave, theEvent);
		if (flags & UIEventFlags::NotInvokeNative) {
			return;
		}
	}
	[[self nextResponder] mouseExited:theEvent];
}

- (void)scrollWheel:(NSEvent *)theEvent
{
	Ref<PlatformViewInstance> instance = m_viewInstance;
	if (instance.isNotNull()) {
		UIEventFlags flags = instance->onEventMouseWheel(theEvent);
		if (flags & UIEventFlags::NotInvokeNative) {
			return;
		}
	}
	[[self nextResponder] scrollWheel:theEvent];
}

- (void)removeCursor: (NSView*)view
{
	[view discardCursorRects];
	for (NSView* child in view.subviews) {
		[self removeCursor: child];
	}
}

- (void)cursorUpdate:(NSEvent *)theEvent
{
	Ref<PlatformViewInstance> instance = m_viewInstance;
	if (instance.isNotNull()) {
		UIEventFlags flags = instance->onEventUpdateCursor(theEvent);
		if (flags & UIEventFlags::NotInvokeNative) {
			[self removeCursor: self];
			return;
		}
	}
	[super cursorUpdate: theEvent];
}

- (NSView *)hitTest:(NSPoint)aPoint
{
	NSPoint pt = [self convertPoint:aPoint fromView:[self superview]];
	Ref<PlatformViewInstance> instance = m_viewInstance;
	if (instance.isNotNull()) {
		Ref<View> view = instance->getView();
		if (view.isNotNull()) {
			if (!(view->isEnabled())) {
				return nil;
			}
			if (view->isCapturingChildInstanceEvents((sl_ui_pos)(pt.x), (sl_ui_pos)(pt.y))) {
				return self;
			}
		}
	}
	for (NSView* subview in [self subviews]) {
		if (!(subview.isHidden)) {
			if (NSPointInRect(pt, subview.frame)) {
				NSView* view = [subview hitTest:pt];
				if (view != nil) {
					return view;
				}
			}
		}
	}
	if (NSPointInRect(aPoint, self.frame)) {
		return self;
	}
	return nil;
}

- (NSDragOperation)draggingEntered:(id<NSDraggingInfo>)sender
{
	Ref<PlatformViewInstance> instance = m_viewInstance;
	if (instance.isNotNull()) {
		Ref<UIEvent> ev = instance->onEventDrop(UIAction::DragEnter, sender);
		if (ev.isNotNull()) {
			return ToNSDragOperation(ev->getDragOperation());
		}
	}
	return NSDragOperationNone;
}

- (NSDragOperation)draggingUpdated:(id<NSDraggingInfo>)sender
{
	Ref<PlatformViewInstance> instance = m_viewInstance;
	if (instance.isNotNull()) {
		Ref<UIEvent> ev = instance->onEventDrop(UIAction::DragOver, sender);
		if (ev.isNotNull()) {
			return ToNSDragOperation(ev->getDragOperation());
		}
	}
	return NSDragOperationNone;
}

- (void)draggingExited:(id<NSDraggingInfo>)sender
{
	Ref<PlatformViewInstance> instance = m_viewInstance;
	if (instance.isNotNull()) {
		instance->onEventDrop(UIAction::DragLeave, sender);
	}
}

- (BOOL)performDragOperation:(id <NSDraggingInfo>)sender;
{
	Ref<PlatformViewInstance> instance = m_viewInstance;
	if (instance.isNotNull()) {
		Ref<UIEvent> ev = instance->onEventDrop(UIAction::Drop, sender);
		if (ev.isNotNull()) {
			if (ev->getDragOperation() != DragOperations::None) {
				return YES;
			}
		}
	}
	return NO;
}

// Adoption of NSTextInputClient protocol
- (NSAttributedString *)attributedSubstringForProposedRange:(NSRange)range actualRange:(nullable NSRangePointer)actualRange
{
	return nil;
}

- (NSUInteger)characterIndexForPoint:(NSPoint)point
{
	return 0;
}

- (void)doCommandBySelector:(SEL)selector
{
	if ([self respondsToSelector: @selector(selector)]) {
		[self performSelector: @selector(selector) withObject: nil];
	}
}

- (NSRect)firstRectForCharacterRange:(NSRange)range actualRange:(nullable NSRangePointer)actualRange
{
	return NSMakeRect(0, 0, 0, 0);
}

- (BOOL)hasMarkedText
{
	return NO;
}

- (void)insertText:(nonnull id)aText replacementRange:(NSRange)range
{
	Ref<PlatformViewInstance> instance = m_viewInstance;
	if (instance.isNull()) {
		return;
	}
	NSString* _text;
	if ([aText isKindOfClass:[NSString class]]) {
		_text = (NSString*)aText;
	} else if ([aText isKindOfClass:[NSAttributedString class]]) {
		_text = [aText string];
	} else {
		return;
	}
	String32 text = Apple::getString32FromNSString(_text);
	sl_size nText = text.getLength();
	if (nText) {
		sl_char32* s = text.getData();
		for (sl_size i = 0; i < nText; i++) {
			instance->onEventChar(s[i]);
		}
	} else {
		return;
	}
	TextInput* input = instance->getTextInput();
	if (input) {
		input->replaceText(slib::TextRange((sl_text_pos)(range.location), (sl_text_pos)(range.length)), text);
	}
}

- (NSRange)markedRange
{
	return NSMakeRange(0, 0);
}

- (NSRange)selectedRange
{
	return NSMakeRange(0, 0);
}

- (void)setMarkedText:(nonnull id)string selectedRange:(NSRange)selectedRange replacementRange:(NSRange)replacementRange
{
}

- (void)unmarkText
{
}

- (NSArray<NSAttributedStringKey> *)validAttributesForMarkedText
{
	return nil;
}

@end

@implementation SLIBDraggingSource

- (NSDragOperation)draggingSession:(nonnull NSDraggingSession *)session sourceOperationMaskForDraggingContext:(NSDraggingContext)context
{
	return ToNSDragOperation(self->context.operationMask);
}

- (void)draggingSession:(NSDraggingSession *)session willBeginAtPoint:(NSPoint)screenPoint
{
	Ref<View>& view = self->context.view;
	if (view.isNotNull()) {
		Ref<UIEvent> ev = UIEvent::createDragEvent(UIAction::DragStart, (sl_ui_posf)(screenPoint.x), (sl_ui_posf)(screenPoint.y), self->context, Time::now());
		if (ev.isNotNull()) {
			view->dispatchDragDropEvent(ev.get());
		}
	}
}

- (void)draggingSession:(NSDraggingSession *)session movedToPoint:(NSPoint)screenPoint
{
	Ref<View>& view = self->context.view;
	if (view.isNotNull()) {
		Ref<UIEvent> ev = UIEvent::createDragEvent(UIAction::Drag, (sl_ui_posf)(screenPoint.x), (sl_ui_posf)(screenPoint.y), self->context, Time::now());
		if (ev.isNotNull()) {
			view->dispatchDragDropEvent(ev.get());
		}
	}
}

- (void)draggingSession:(NSDraggingSession *)session endedAtPoint:(NSPoint)screenPoint operation:(NSDragOperation)operation
{
	Ref<View>& view = self->context.view;
	if (view.isNotNull()) {
		view->cancelPressedState();
		view->cancelHoverState();
		self->context.operation = FromNSDragOperation(operation);
		Ref<UIEvent> ev = UIEvent::createDragEvent(UIAction::DragEnd, (sl_ui_posf)(screenPoint.x), (sl_ui_posf)(screenPoint.y), self->context, Time::now());
		if (ev.isNotNull()) {
			view->dispatchDragDropEvent(ev.get());
		}
	}
}

@end

@implementation SLIBToolTip

-(NSString*)view:(NSView*)view stringForToolTip:(NSToolTipTag)tag point:(NSPoint)point userData:(void*)data
{
	return m_content;
}

@end

#endif
