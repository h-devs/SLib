/*
 *   Copyright (c) 2008-2023 SLIBIO <https://github.com/SLIBIO>
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

#include "slib/ui/list_control.h"

#include "view_macos.h"


namespace slib {
	namespace {
		class ListControlInstance;
	}
}

@interface SLIBListControlHandle_TableView : NSTableView
{
	@public slib::WeakRef<slib::ListControlInstance> m_viewInstance;
}
@end

@interface SLIBListControlHandle : NSScrollView<NSTableViewDataSource, NSTableViewDelegate>
{
	@public SLIBListControlHandle_TableView* table;
	@public slib::WeakRef<slib::ListControlInstance> m_viewInstance;
	@public slib::CList<NSTableColumn*> m_columns;
	@public NSFont* m_font;
}
@end

namespace slib
{

	namespace {

		static NSTextAlignment TranslateAlignment(Alignment _align)
		{
			Alignment align = _align & Alignment::HorizontalMask;
			if (align == Alignment::Left) {
				return NSTextAlignmentLeft;
			} else if (align == Alignment::Right) {
				return NSTextAlignmentRight;
			} else {
				return NSTextAlignmentCenter;
			}
		}

		class ListControlHelper : public ListControl
		{
		public:
			void applyColumnCount(SLIBListControlHandle* tv)
			{
				ObjectLocker lock(this);
				CList<NSTableColumn*>& _columns = tv->m_columns;
				sl_uint32 nOrig = (sl_uint32)(_columns.getCount());
				sl_uint32 nNew = (sl_uint32)(m_columns.getCount());
				if (nOrig == nNew) {
					return;
				}
				if (nOrig > nNew) {
					ListLocker<NSTableColumn*> columns(_columns);
					for (sl_uint32 i = nNew; i < columns.count; i++) {
						[tv->table removeTableColumn:(columns[i])];
					}
					_columns.setCount(nNew);
				} else {
					_columns.setCount(nNew);
					ListLocker<NSTableColumn*> columns(_columns);
					for (sl_uint32 i = nOrig; i < columns.count; i++) {
						NSTableColumn* column = [[NSTableColumn alloc] initWithIdentifier:[NSString stringWithFormat:@"%d",i]];
						[tv->table addTableColumn:column];
						columns[i] = column;
					}
				}
			}

			void copyColumns(SLIBListControlHandle* tv)
			{
				ObjectLocker lock(this);
				ListLocker<Column> columns(m_columns);
				applyColumnCount(tv);
				for (sl_uint32 i = 0; i < columns.count; i++) {
					NSTableColumn* tc = tv->m_columns.getValueAt(i, nil);
					if (tc != nil) {
						[tc setTitle:(Apple::getNSStringFromString(columns[i].title))];
						[tc setWidth:(CGFloat)(columns[i].width)];
						NSTableHeaderCell* headerCell = [tc headerCell];
						NSCell* dataCell = [tc dataCell];
						[headerCell setAlignment:TranslateAlignment(columns[i].headerAlign)];
						[dataCell setAlignment:TranslateAlignment(columns[i].align)];
						[dataCell setEditable:FALSE];
					}
				}
			}

			void applyFont(SLIBListControlHandle* tv, const Ref<Font>& font)
			{
				if (font.isNull()) {
					return;
				}
				NSFont* hFont = GraphicsPlatform::getNativeFont(font.get());
				if (hFont == nil) {
					return;
				}
				tv->m_font = hFont;
				ListLocker<Column> columns(m_columns);
				for (sl_uint32 i = 0; i < columns.count; i++) {
					NSTableColumn* tc = tv->m_columns.getValueAt(i, nil);
					if (tc != nil) {
						NSCell* dataCell = [tc dataCell];
						[dataCell setFont:hFont];
					}
				}
				[tv->table setRowHeight:([hFont pointSize] - [hFont descender])];
			}

			using ListControl::_onSelectRow_NW;
			using ListControl::_onClickRow_NW;
			using ListControl::_onRightButtonClickRow_NW;
			using ListControl::_onDoubleClickRow_NW;
		};

		class ListControlInstance : public macOS_ViewInstance, public IListControlInstance
		{
			SLIB_DECLARE_OBJECT

		public:
			SLIBListControlHandle* getHandle()
			{
				return (SLIBListControlHandle*)m_handle;
			}

			Ref<ListControlHelper> getHelper()
			{
				return CastRef<ListControlHelper>(getView());
			}

			void initialize(View* _view) override
			{
				SLIBListControlHandle* handle = getHandle();
				ListControlHelper* view = (ListControlHelper*)_view;

				[handle setHasVerticalScroller:TRUE];
				[handle setHasHorizontalScroller:TRUE];
				[handle setBorderType:NSBezelBorder];
				handle->table->m_viewInstance = this;
				view->copyColumns(handle);
				view->applyFont(handle, view->getFont());
				[handle->table reloadData];
			}

			void refreshColumnCount(ListControl* view) override
			{
				SLIBListControlHandle* handle = getHandle();
				if (handle != nil) {
					static_cast<ListControlHelper*>(view)->applyColumnCount(handle);
					[handle->table reloadData];
				}
			}

			void refreshRowCount(ListControl* view) override
			{
				SLIBListControlHandle* handle = getHandle();
				if (handle != nil) {
					[handle->table reloadData];
				}
			}

			void setHeaderText(ListControl* view, sl_uint32 iCol, const String& text) override
			{
				SLIBListControlHandle* handle = getHandle();
				if (handle != nil) {
					NSTableColumn* tc = handle->m_columns.getValueAt(iCol, nil);
					if (tc != nil) {
						[tc setTitle:Apple::getNSStringFromString(text)];
					}
				}
			}

			void setColumnWidth(ListControl* view, sl_uint32 iCol, sl_ui_len width) override
			{
				SLIBListControlHandle* handle = getHandle();
				if (handle != nil) {
					NSTableColumn* tc = handle->m_columns.getValueAt(iCol, nil);
					if (tc != nil) {
						[tc setWidth:(CGFloat)width];
					}
				}
			}

			void setHeaderAlignment(ListControl* view, sl_uint32 iCol, const Alignment& align) override
			{
				SLIBListControlHandle* handle = getHandle();
				if (handle != nil) {
					NSTableColumn* tc = handle->m_columns.getValueAt(iCol, nil);
					if (tc != nil) {
						[[tc headerCell] setAlignment:TranslateAlignment(align)];
					}
				}
			}

			void setColumnAlignment(ListControl* view, sl_uint32 iCol, const Alignment& align) override
			{
				SLIBListControlHandle* handle = getHandle();
				if (handle != nil) {
					NSTableColumn* tc = handle->m_columns.getValueAt(iCol, nil);
					if (tc != nil) {
						[[tc dataCell] setAlignment:TranslateAlignment(align)];
					}
				}
			}

			sl_bool getSelectedRow(ListControl* view, sl_int32& _out) override
			{
				SLIBListControlHandle* handle = getHandle();
				if (handle != nil) {
					_out = (sl_int32)([handle->table selectedRow]);
					return sl_true;
				}
				return sl_false;
			}

			void setFont(View* view, const Ref<Font>& font) override
			{
				SLIBListControlHandle* handle = getHandle();
				if (handle != nil) {
					static_cast<ListControlHelper*>(view)->applyFont(handle, font);
					[handle->table reloadData];
				}
			}

			void onMouseDown(SLIBListControlHandle_TableView* tv, NSEvent* ev)
			{
				Ref<ListControlHelper> helper = getHelper();
				if (helper.isNotNull()) {
					NSPoint ptWindow = [ev locationInWindow];
					NSPoint ptView = [tv convertPoint:ptWindow fromView:nil];
					NSInteger indexRow = [tv rowAtPoint:ptView];
					if (indexRow >= 0) {
						sl_ui_posf x = (sl_ui_posf)(ptView.x);
						sl_ui_posf y = (sl_ui_posf)(ptView.y);
						NSInteger clicks = [ev clickCount];
						if (clicks == 1) {
							helper->_onClickRow_NW((sl_uint32)(indexRow), UIPointF(x, y));
						} else if (clicks == 2) {
							helper->_onDoubleClickRow_NW((sl_uint32)(indexRow), UIPointF(x, y));
						}
					}
				}
			}

			void onRightMouseDown(SLIBListControlHandle_TableView* tv, NSEvent* ev)
			{
				Ref<ListControlHelper> helper = getHelper();
				if (helper.isNotNull()) {
					NSPoint ptWindow = [ev locationInWindow];
					NSPoint ptView = [tv convertPoint:ptWindow fromView:nil];
					NSInteger indexRow = [tv rowAtPoint:ptView];
					if (indexRow >= 0) {
						sl_ui_posf x = (sl_ui_posf)(ptView.x);
						sl_ui_posf y = (sl_ui_posf)(ptView.y);
						helper->_onRightButtonClickRow_NW((sl_uint32)(indexRow), UIPointF(x, y));
					}
				}
			}

		};

		SLIB_DEFINE_OBJECT(ListControlInstance, macOS_ViewInstance)

	}

	Ref<ViewInstance> ListControl::createNativeWidget(ViewInstance* parent)
	{
		return macOS_ViewInstance::create<ListControlInstance, SLIBListControlHandle>(this, parent);
	}

	Ptr<IListControlInstance> ListControl::getListControlInstance()
	{
		return CastRef<ListControlInstance>(getViewInstance());
	}

}

using namespace slib;

@implementation SLIBListControlHandle

-(id)initWithFrame:(NSRect)frame
{
	self = [super initWithFrame:frame];
	if (self != nil) {
		self->m_columns.setCount(0);
		self->table = [[SLIBListControlHandle_TableView alloc] init];
		[self->table setDelegate:self];
		[self->table setDataSource:self];
		[self->table setRowSizeStyle:NSTableViewRowSizeStyleCustom];
		[self->table setGridStyleMask:NSTableViewSolidVerticalGridLineMask | NSTableViewSolidHorizontalGridLineMask];
		[self setDocumentView:table];
	}
	return self;
}

- (Ref<ListControlHelper>)getHelper
{
	Ref<ListControlInstance> instance = self->m_viewInstance;
	if (instance.isNotNull()) {
		return instance->getHelper();
	}
	return sl_null;
}

- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView{
	Ref<ListControlHelper> helper = [self getHelper];
	if (helper.isNotNull()) {
		return helper->getRowCount();
	}
	return 0;
}

- (id)tableView:(NSTableView *)tableView objectValueForTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)row{
	Ref<ListControlHelper> helper = [self getHelper];
	if (helper.isNotNull()) {
		NSString* _id = tableColumn.identifier;
		if (_id != nil) {
			sl_uint32 iRow = (sl_uint32)(row);
			sl_uint32 iCol = (sl_uint32)(_id.intValue);
			return Apple::getNSStringFromString(helper->getItemText(iRow, iCol));
		}
	}
	return @"";
}

- (void)tableViewSelectionDidChange:(NSNotification *)aNotification
{
	Ref<ListControlInstance> instance = self->m_viewInstance;
	if (instance.isNotNull()) {
		Ref<ListControlHelper> helper = instance->getHelper();
		if (helper.isNotNull()) {
			sl_int32 n = (sl_int32)([table selectedRow]);
			if (n >= 0) {
				helper->_onSelectRow_NW(instance.get(), n);
			}
		}
	}
}

@end

@implementation SLIBListControlHandle_TableView

MACOS_VIEW_DEFINE_ON_CHILD_VIEW

- (void)mouseDown:(NSEvent *)theEvent
{
	[super mouseDown:theEvent];
	Ref<ListControlInstance> instance = m_viewInstance;
	if (instance.isNotNull()) {
		instance->onMouseDown(self, theEvent);
	}
}

- (void)rightMouseDown:(NSEvent *)theEvent
{
	[super rightMouseDown:theEvent];
	Ref<ListControlInstance> instance = m_viewInstance;
	if (instance.isNotNull()) {
		instance->onRightMouseDown(self, theEvent);
	}
}

@end

#endif
