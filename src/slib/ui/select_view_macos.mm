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

#include "slib/ui/select_view.h"

#include "view_macos.h"

namespace slib {
	namespace {
		class SelectViewInstance;
	}
}

@interface SLIBSelectViewHandle : NSPopUpButton
{
	@public slib::WeakRef<slib::SelectViewInstance> m_viewInstance;
}
@end

namespace slib
{

	namespace {

		class SelectViewHelper : public SelectView
		{
		public:
			void refreshItems(NSPopUpButton* v)
			{
				[v removeAllItems];
				sl_uint32 n = (sl_uint32)(getItemCount());
				for (sl_uint32 i = 0; i < n; i++) {
					[v addItemWithTitle:@"____dummy____"];
					NSMenuItem* item = [v lastItem];
					if (item != nil) {
						[item setTitle:Apple::getNSStringFromString(getItemTitle(i), @"")];
					}
				}
				sl_uint32 indexSelected = m_indexSelected;
				if (indexSelected < n) {
					[v selectItemAtIndex:indexSelected];
				}
			}

			using SelectView::_onSelectItem_NW;

		};

		class SelectViewInstance : public macOS_ViewInstance, public ISelectViewInstance
		{
			SLIB_DECLARE_OBJECT

		public:
			NSPopUpButton* getHandle()
			{
				return (NSPopUpButton*)m_handle;
			}

			void initialize(View* _view) override
			{
				SelectViewHelper* view = (SelectViewHelper*)_view;
				NSPopUpButton* handle = getHandle();

				[handle setPullsDown:NO];
				view->refreshItems(handle);
			}

			void selectItem(SelectView* view, sl_uint32 index) override
			{
				NSPopUpButton* handle = getHandle();
				if (handle != nil) {
					[handle selectItemAtIndex:index];
				}
			}

			void refreshItems(SelectView* view) override
			{
				NSPopUpButton* handle = getHandle();
				if (handle != nil) {
					static_cast<SelectViewHelper*>(view)->refreshItems(handle);
				}
			}

			void insertItem(SelectView* view, sl_uint32 index, const String& title) override
			{
				NSPopUpButton* handle = getHandle();
				if (handle != nil) {
					[handle insertItemWithTitle:@"____dummy____" atIndex:index];
					NSMenuItem* item = [handle itemAtIndex:index];
					if (item != nil) {
						[item setTitle:Apple::getNSStringFromString(title, @"")];
					}
				}
			}

			void removeItem(SelectView* view, sl_uint32 index) override
			{
				NSPopUpButton* handle = getHandle();
				if (handle != nil) {
					[handle removeItemAtIndex:index];
				}
			}

			void setItemTitle(SelectView* view, sl_uint32 index, const String& title) override
			{
				NSPopUpButton* handle = getHandle();
				if (handle != nil) {
					NSMenuItem* item = [handle itemAtIndex:index];
					if (item != nil) {
						[item setTitle:Apple::getNSStringFromString(title, @"")];
					}
				}
			}

			sl_bool measureSize(SelectView* view, UISize& _out) override
			{
				return UIPlatform::measureNativeWidgetFittingSize(this, _out);
			}

			void onSelectItem(NSPopUpButton* handle)
			{
				Ref<SelectViewHelper> view = CastRef<SelectViewHelper>(getView());
				if (view.isNotNull()) {
					view->_onSelectItem_NW((sl_uint32)([handle indexOfSelectedItem]));
				}
			}
		};

		SLIB_DEFINE_OBJECT(SelectViewInstance, macOS_ViewInstance)

	}

	Ref<ViewInstance> SelectView::createNativeWidget(ViewInstance* parent)
	{
		return macOS_ViewInstance::create<SelectViewInstance, SLIBSelectViewHandle>(this, parent);
	}

	Ptr<ISelectViewInstance> SelectView::getSelectViewInstance()
	{
		return CastRef<SelectViewInstance>(getViewInstance());
	}

}

using namespace slib;

@implementation SLIBSelectViewHandle

MACOS_VIEW_DEFINE_ON_CHILD_VIEW

-(id)initWithFrame:(NSRect)frame
{
	self = [super initWithFrame:frame];
	if (self != nil) {
		[self setAction: @selector(onSelect)];
		[self setTarget:self];
	}
	return self;
}

-(void)onSelect
{
	Ref<SelectViewInstance> instance = m_viewInstance;
	if (instance.isNotNull()) {
		instance->onSelectItem(self);
	}
}

@end

#endif
