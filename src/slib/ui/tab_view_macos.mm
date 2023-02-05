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

#include "slib/ui/definition.h"

#if defined(SLIB_UI_IS_MACOS)

#include "slib/ui/tab_view.h"

#include "view_macos.h"

namespace slib {
	namespace {
		class TabViewInstance;
	}
}

@interface SLIBTabViewHandle : NSTabView<NSTabViewDelegate>
{
	@public slib::WeakRef<slib::TabViewInstance> m_viewInstance;
}
@end

@interface SLIBTabViewHandle_EmptyView : NSView {}
@end

namespace slib
{

	namespace {

		class TabViewHelper : public TabView
		{
		public:
			void applyTabCount(NSTabView* tv)
			{
				ObjectLocker lock(this);
				sl_uint32 nNew = (sl_uint32)(m_items.getCount());
				sl_uint32 nOrig = (sl_uint32)([tv numberOfTabViewItems]);
				if (nOrig == nNew) {
					return;
				}
				if (nOrig > nNew) {
					for (sl_uint32 i = nOrig; i > nNew; i--) {
						NSTabViewItem* item = [tv tabViewItemAtIndex:(i - 1)];
						if (item != nil) {
							[tv removeTabViewItem:item];
						}
					}
				} else {
					for (sl_uint32 i = nOrig; i < nNew; i++) {
						NSTabViewItem* item = [[NSTabViewItem alloc] initWithIdentifier:[NSString stringWithFormat:@"%d",i]];
						if (item != nil) {
							[tv addTabViewItem:item];
						}
					}
				}
			}

			void copyTabs(NSTabView* tv)
			{
				ListLocker<TabViewItem> items(m_items);
				applyTabCount(tv);
				for (sl_uint32 i = 0; i < items.count; i++) {
					NSTabViewItem* t = [tv tabViewItemAtIndex:i];
					if (t != nil) {
						[t setLabel:Apple::getNSStringFromString(items[i].label)];
						setTabContentView(tv, i, items[i].contentView);
					}
				}
				if ([tv numberOfTabViewItems] > 0) {
					[tv selectTabViewItemAtIndex:m_indexSelected];
				}
			}

			void setTabContentView(NSTabView* tv, sl_uint32 index, const Ref<View>& view)
			{
				NSTabViewItem* item = [tv tabViewItemAtIndex:index];
				if (item == nil) {
					return;
				}
				NSView* handle = nil;
				if (view.isNotNull()) {
					Ref<ViewInstance> instance = view->attachToNewInstance(sl_null);
					if (instance.isNotNull()) {
						handle = UIPlatform::getViewHandle(instance.get());
					}
					NSRect rc = [tv contentRect];
					view->setFrame((sl_ui_pos)(rc.origin.x), (sl_ui_pos)(rc.origin.y), (sl_ui_pos)(rc.size.width), (sl_ui_pos)(rc.size.height));
					view->setParent(this);
				}
				if (handle == nil) {
					handle = [[SLIBTabViewHandle_EmptyView alloc] init];
				}
				[handle setHidden:NO];
				[item setView:handle];
			}

			void updateContentViewSize(SLIBTabViewHandle* tv)
			{
				NSRect rc = [tv contentRect];
				UIRect frame;
				sl_ui_pos w = (sl_ui_pos)(rc.size.width);
				sl_ui_pos h = (sl_ui_pos)(rc.size.height);
				frame.left = (sl_ui_pos)(rc.origin.x);
				frame.top = (sl_ui_pos)(rc.origin.y);
				frame.right = frame.left + w;
				frame.bottom = frame.top + h;
				ListLocker<TabViewItem> items(m_items);
				for (sl_size i = 0; i < items.count; i++) {
					Ref<View> view = items[i].contentView;
					if (view.isNotNull()) {
						view->setFrame(frame);
					}
				}
			}

		};

		class TabViewInstance : public macOS_ViewInstance, public ITabViewInstance
		{
			SLIB_DECLARE_OBJECT

		public:
			NSTabView* getHandle()
			{
				return (NSTabView*)m_handle;
			}

			Ref<TabViewHelper> getHelper()
			{
				return CastRef<TabViewHelper>(getView());
			}

			void initialize(View* _view) override
			{
				TabViewHelper* view = (TabViewHelper*)_view;
				NSTabView* handle = getHandle();

				setHandleFont(handle, view->getFont());
				view->copyTabs(handle);
			}

			void refreshTabCount(TabView* view) override
			{
				NSTabView* handle = getHandle();
				if (handle != nil) {
					static_cast<TabViewHelper*>(view)->applyTabCount(handle);
				}
			}

			void refreshSize(TabView* view) override
			{
			}

			void setTabLabel(TabView* view, sl_uint32 index, const String& text) override
			{
				NSTabView* handle = getHandle();
				if (handle != nil) {
					NSTabViewItem* t = [handle tabViewItemAtIndex:index];
					if (t != nil) {
						[t setLabel:Apple::getNSStringFromString(text)];
					}
				}
			}

			void setTabContentView(TabView* view, sl_uint32 index, const Ref<View>& content) override
			{
				NSTabView* handle = getHandle();
				if (handle != nil) {
					static_cast<TabViewHelper*>(view)->setTabContentView(handle, index, content);
				}
			}

			void selectTab(TabView* view, sl_uint32 index) override
			{
				NSTabView* handle = getHandle();
				if (handle != nil) {
					[handle selectTabViewItemAtIndex:index];
				}
			}

			sl_bool getContentViewSize(TabView* view, UISize& _out) override
			{
				NSTabView* handle = getHandle();
				if (handle != nil) {
					NSTabView* tv = (NSTabView*)handle;
					NSRect rc = [tv contentRect];
					_out.x = (sl_ui_pos)(rc.size.width);
					_out.y = (sl_ui_pos)(rc.size.height);
					return sl_true;
				}
				return sl_false;
			}

			void setFont(View* view, const Ref<Font>& font) override
			{
				NSTabView* handle = getHandle();
				if (handle != nil) {
					setHandleFont(handle, font);
				}
			}

			void onSelectTab(NSTabView* tv)
			{
				Ref<TabViewHelper> helper = getHelper();
				if (helper.isNotNull()) {
					helper->dispatchSelectTab((sl_uint32)([tv indexOfTabViewItem:[tv selectedTabViewItem]]));
				}
			}

		};

		SLIB_DEFINE_OBJECT(TabViewInstance, macOS_ViewInstance)

	}

	Ref<ViewInstance> TabView::createNativeWidget(ViewInstance* parent)
	{
		return macOS_ViewInstance::create<TabViewInstance, SLIBTabViewHandle>(this, parent);
	}

	Ptr<ITabViewInstance> TabView::getTabViewInstance()
	{
		return CastRef<TabViewInstance>(getViewInstance());
	}

}

using namespace slib;

@implementation SLIBTabViewHandle

MACOS_VIEW_DEFINE_ON_CHILD_VIEW

-(id)initWithFrame:(NSRect)frame
{
	self = [super initWithFrame:frame];
	if (self != nil) {
		[self setDelegate:self];
	}
	return self;
}

-(void)setFrame:(NSRect)frame
{
	[super setFrame:frame];
	Ref<TabViewInstance> instance = m_viewInstance;
	if (instance.isNotNull()) {
		Ref<TabViewHelper> helper = instance->getHelper();
		if (helper.isNotNull()) {
			helper->updateContentViewSize(self);
		}
	}
}

- (void)tabView:(NSTabView *)tabView didSelectTabViewItem:(NSTabViewItem *)tabViewItem
{
	Ref<TabViewInstance> instance = m_viewInstance;
	if (instance.isNotNull()) {
		instance->onSelectTab(self);
	}
}
@end

@implementation SLIBTabViewHandle_EmptyView

@end

#endif
