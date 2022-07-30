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

#ifndef CHECKHEADER_SLIB_UI_TABLE_LAYOUT
#define CHECKHEADER_SLIB_UI_TABLE_LAYOUT

#include "scroll_view.h"
#include "adapter.h"

namespace slib
{
	
	namespace priv
	{
		namespace table_layout
		{
			class Cell;
			class Column;
			class Row;
		}
	}
	
	class SLIB_EXPORT TableLayout : public ViewGroup
	{
		SLIB_DECLARE_OBJECT
		
	public:
		TableLayout();
		
		~TableLayout();
		
	public:
		sl_uint32 getColumnCount();
		
		void setColumnCount(sl_uint32 nColumns, UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		SizeMode getColumnWidthMode(sl_uint32 column);
		
		sl_ui_len getColumnWidth(sl_uint32 column);
		
		void setColumnWidth(sl_uint32 column, sl_ui_len width, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		sl_bool isColumnWidthFixed(sl_uint32 column);

		sl_real getColumnWidthWeight(sl_uint32 column);
		
		sl_bool isColumnWidthFilling(sl_uint32 column);
		
		void setColumnWidthFilling(sl_uint32 column, sl_real weight = 1, UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		sl_bool isColumnWidthWrapping(sl_uint32 column);
		
		void setColumnWidthWrapping(sl_uint32 column, UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		sl_bool isColumnWidthWeight(sl_uint32 column);
		
		void setColumnWidthWeight(sl_uint32 column, sl_real weight, UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		sl_ui_len getColumnMinimumWidth(sl_uint32 column);
		
		void setColumnMinimumWidth(sl_uint32 column, sl_ui_len width, UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		sl_bool isColumnMaximumWidthDefined(sl_uint32 column);

		sl_ui_len getColumnMaximumWidth(sl_uint32 column);
		
		void setColumnMaximumWidth(sl_uint32 column, sl_ui_len width, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		sl_ui_len getColumnMarginLeft(sl_uint32 column);

		void setColumnMarginLeft(sl_uint32 column, sl_ui_len margin, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		sl_ui_len getColumnMarginRight(sl_uint32 column);

		void setColumnMarginRight(sl_uint32 column, sl_ui_len margin, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		void setColumnMargin(sl_uint32 column, sl_ui_len margin, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		sl_ui_len getColumnPaddingLeft(sl_uint32 column);

		void setColumnPaddingLeft(sl_uint32 column, sl_ui_len padding, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		sl_ui_len getColumnPaddingRight(sl_uint32 column);

		void setColumnPaddingRight(sl_uint32 column, sl_ui_len padding, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		void setColumnPadding(sl_uint32 column, sl_ui_len padding, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		Ref<Drawable> getColumnBackground(sl_uint32 column);

		void setColumnBackground(sl_uint32 column, const Ref<Drawable>& background, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setColumnBackgroundColor(sl_uint32 column, const Color& color, UIUpdateMode mode = UIUpdateMode::Redraw);

		Alignment getColumnAlignment(sl_uint32 column);

		void setColumnAlignment(sl_uint32 column, const Alignment& align, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		sl_uint32 getRowCount();
		
		void setRowCount(sl_uint32 nRows, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		SizeMode getRowHeightMode(sl_uint32 row);

		sl_ui_len getRowHeight(sl_uint32 row);
		
		void setRowHeight(sl_uint32 row, sl_ui_len height, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		sl_bool isRowHeightFixed(sl_uint32 row);
		
		sl_real getRowHeightWeight(sl_uint32 row);
		
		sl_bool isRowHeightFilling(sl_uint32 row);
		
		void setRowHeightFilling(sl_uint32 row, sl_real weight = 1, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		sl_bool isRowHeightWrapping(sl_uint32 row);
		
		void setRowHeightWrapping(sl_uint32 row, UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		sl_bool isRowHeightWeight(sl_uint32 row);
		
		void setRowHeightWeight(sl_uint32 row, sl_real weight, UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		sl_ui_len getRowMinimumHeight(sl_uint32 row);
		
		void setRowMinimumHeight(sl_uint32 row, sl_ui_len height, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		sl_bool isRowMaximumHeightDefined(sl_uint32 row);

		sl_ui_len getRowMaximumHeight(sl_uint32 row);
		
		void setRowMaximumHeight(sl_uint32 row, sl_ui_len height, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		sl_ui_len getRowMarginTop(sl_uint32 row);

		void setRowMarginTop(sl_uint32 row, sl_ui_len margin, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		sl_ui_len getRowMarginBottom(sl_uint32 row);

		void setRowMarginBottom(sl_uint32 row, sl_ui_len margin, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		void setRowMargin(sl_uint32 row, sl_ui_len margin, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		sl_ui_len getRowPaddingTop(sl_uint32 row);

		void setRowPaddingTop(sl_uint32 row, sl_ui_len padding, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		sl_ui_len getRowPaddingBottom(sl_uint32 row);

		void setRowPaddingBottom(sl_uint32 row, sl_ui_len padding, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		void setRowPadding(sl_uint32 row, sl_ui_len padding, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		Ref<Drawable> getRowBackground(sl_uint32 row);

		void setRowBackground(sl_uint32 row, const Ref<Drawable>& background, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setRowBackgroundColor(sl_uint32 row, const Color& color, UIUpdateMode mode = UIUpdateMode::Redraw);

		Alignment getRowAlignment(sl_uint32 row);

		void setRowAlignment(sl_uint32 row, const Alignment& align, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		Ref<View> getCell(sl_uint32 row, sl_uint32 col);

		void setCell(sl_uint32 row, sl_uint32 col, const Ref<View>& view, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		void setCell(sl_uint32 row, sl_uint32 col, const Ref<View>& view, sl_uint32 rowspan, sl_uint32 colspan, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		sl_uint32 getRowspan(sl_uint32 row, sl_uint32 col);

		void setRowspan(sl_uint32 row, sl_uint32 col, sl_uint32 rowspan, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		sl_uint32 getColspan(sl_uint32 row, sl_uint32 col);

		void setColspan(sl_uint32 row, sl_uint32 col, sl_uint32 colspan, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		void setCellSpan(sl_uint32 row, sl_uint32 col, sl_uint32 rowspan, sl_uint32 colspan, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

	protected:
		void onUpdateLayout() override;

		void onDraw(Canvas* canvas) override;
		
	private:
		priv::table_layout::Cell* _getCell(sl_uint32 row, sl_uint32 col);

		priv::table_layout::Cell* _allocCell(sl_uint32 row, sl_uint32 col);

		Alignment _getCellAlign(sl_uint32 row, sl_uint32 col);

		void _initCellAlign(priv::table_layout::Cell* cell, sl_uint32 row, sl_uint32 col);

		void _applyCellAlign(priv::table_layout::Cell* cell, sl_uint32 row, sl_uint32 col, UIUpdateMode mode);
		
	protected:
		List<priv::table_layout::Column> m_columns;
		List<priv::table_layout::Row> m_rows;
		
	};

}

#endif
