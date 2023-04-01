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

#include "slib/ui/combo_box.h"

#include "view_macos.h"

namespace slib {
	namespace {
		class ComboBoxInstance;
	}
}

@interface SLIBComboBoxHandle : NSComboBox
{
	@public slib::WeakRef<slib::ComboBoxInstance> m_viewInstance;
}
@end

namespace slib
{

	namespace {

		class ComboBoxHelper : public ComboBox
		{
		public:
			void refreshItems(NSComboBox* v)
			{
				[v removeAllItems];
				sl_uint32 n = (sl_uint32)(getItemCount());
				for (sl_uint32 i = 0; i < n; i++) {
					[v addItemWithObjectValue:Apple::getNSStringFromString(getItemTitle(i), @"")];
				}
				sl_uint32 indexSelected = m_indexSelected;
				if (indexSelected >= 0 && indexSelected < n) {
					[v selectItemAtIndex:indexSelected];
				}
			}

			using ComboBox::_onChange_NW;
			using ComboBox::_onSelectItem_NW;

		};

		class ComboBoxInstance : public macOS_ViewInstance, public IComboBoxInstance
		{
			SLIB_DECLARE_OBJECT

		public:
			NSComboBox* getHandle()
			{
				return (NSComboBox*)m_handle;
			}

			void initialize(View* _view) override
			{
				ComboBoxHelper* view = (ComboBoxHelper*)_view;
				NSComboBox* handle = getHandle();

				view->refreshItems(handle);
				[handle setStringValue:Apple::getNSStringFromString(view->getText())];
			}

			void selectItem(ComboBox* view, sl_int32 index) override
			{
				NSComboBox* handle = getHandle();
				if (handle != nil) {
					if (index >= 0) {
						[handle selectItemAtIndex:index];
					}
				}
			}

			void refreshItems(ComboBox* view) override
			{
				NSComboBox* handle = getHandle();
				if (handle != nil) {
					static_cast<ComboBoxHelper*>(view)->refreshItems(handle);
				}
			}

			void insertItem(ComboBox* view, sl_int32 index, const String& title) override
			{
				NSComboBox* handle = getHandle();
				if (handle != nil) {
					[handle insertItemWithObjectValue:Apple::getNSStringFromString(title, @"") atIndex:index];
				}
			}

			void removeItem(ComboBox* view, sl_int32 index) override
			{
				NSComboBox* handle = getHandle();
				if (handle != nil) {
					[handle removeItemAtIndex:index];
				}
			}

			void setItemTitle(ComboBox* view, sl_int32 index, const String& title) override
			{
				NSComboBox* handle = getHandle();
				if (handle != nil) {
					[handle removeItemAtIndex:index];
					[handle insertItemWithObjectValue:Apple::getNSStringFromString(title, @"") atIndex:index];
				}
			}

			sl_bool getText(ComboBox* view, String& _out) override
			{
				NSComboBox* handle = getHandle();
				if (handle != nil) {
					_out = Apple::getStringFromNSString([handle stringValue]);
					return sl_true;
				}
				return sl_false;
			}

			void setText(ComboBox* view, const String& text) override
			{
				NSComboBox* handle = getHandle();
				if (handle != nil) {
					[handle setStringValue:Apple::getNSStringFromString(text)];
				}
			}

			sl_ui_len measureHeight(ComboBox* view) override
			{
				UISize size;
				if (UIPlatform::measureNativeWidgetFittingSize(this, size)) {
					return size.y;
				}
				return 0;
			}

			void onSelectItem(NSComboBox* handle)
			{
				Ref<ComboBoxHelper> view = CastRef<ComboBoxHelper>(getView());
				if (view.isNotNull()) {
					view->_onSelectItem_NW((sl_uint32)([handle indexOfSelectedItem]));
				}
			}

		};

		SLIB_DEFINE_OBJECT(ComboBoxInstance, macOS_ViewInstance)

	}

	Ref<ViewInstance> ComboBox::createNativeWidget(ViewInstance* parent)
	{
		return macOS_ViewInstance::create<ComboBoxInstance, SLIBComboBoxHandle>(this, parent);
	}

	Ptr<IComboBoxInstance> ComboBox::getComboBoxInstance()
	{
		return CastRef<ComboBoxInstance>(getViewInstance());
	}

}

using namespace slib;

@implementation SLIBComboBoxHandle

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
	Ref<ComboBoxInstance> instance = m_viewInstance;
	if (instance.isNotNull()) {
		instance->onSelectItem(self);
	}
}

@end

#endif
