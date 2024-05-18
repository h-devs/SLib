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

#ifndef CHECKHEADER_SLIB_UI_TABLE_LAYOUT
#define CHECKHEADER_SLIB_UI_TABLE_LAYOUT

#include "scroll_view.h"
#include "adapter.h"

#include "../core/string.h"

namespace slib
{

	class SLIB_EXPORT TableLayout : public ViewGroup
	{
		SLIB_DECLARE_OBJECT

	public:
		TableLayout();

		~TableLayout();

	public:
		class Column;
		class Row;

		sl_uint32 getColumnCount();

		sl_bool setColumnCount(sl_uint32 nColumns, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		Ref<Column> getColumn(sl_uint32 index);

		Ref<Column> addColumn();

		Ref<Column> insertColumn(sl_uint32 index);

		sl_bool removeColumn(sl_uint32 index, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		String getColumnId(sl_uint32 column);

		void setColumnId(sl_uint32 column, const String& _id);

		SizeMode getColumnWidthMode(sl_uint32 column);

		sl_ui_len getColumnWidth(sl_uint32 column);

		void setColumnWidth(sl_uint32 column, sl_ui_len width, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		sl_bool isColumnWidthFixed(sl_uint32 column);

		sl_real getColumnWidthWeight(sl_uint32 column);

		sl_bool isColumnWidthFilling(sl_uint32 column);

		void setColumnWidthFilling(sl_uint32 column, sl_real weight = 1, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		sl_bool isColumnWidthWrapping(sl_uint32 column);

		void setColumnWidthWrapping(sl_uint32 column, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		void setColumnWidthWrapping(sl_uint32 column, sl_bool flagWrapping, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

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

		sl_bool isColumnVisible(sl_uint32 column);

		void setColumnVisible(sl_uint32 column, sl_bool flagVisible, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		sl_uint32 getRowCount();

		sl_bool setRowCount(sl_uint32 nRows, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		Ref<Row> getRow(sl_uint32 index);

		Ref<Row> addRow();

		Ref<Row> insertRow(sl_uint32 index);

		sl_bool removeRow(sl_uint32 index, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		String getRowId(sl_uint32 row);

		void setRowId(sl_uint32 row, const String& _id);

		SizeMode getRowHeightMode(sl_uint32 row);

		sl_ui_len getRowHeight(sl_uint32 row);

		void setRowHeight(sl_uint32 row, sl_ui_len height, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		sl_bool isRowHeightFixed(sl_uint32 row);

		sl_real getRowHeightWeight(sl_uint32 row);

		sl_bool isRowHeightFilling(sl_uint32 row);

		void setRowHeightFilling(sl_uint32 row, sl_real weight = 1, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		sl_bool isRowHeightWrapping(sl_uint32 row);

		void setRowHeightWrapping(sl_uint32 row, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		void setRowHeightWrapping(sl_uint32 row, sl_bool flagWrapping, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

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

		sl_bool isRowVisible(sl_uint32 row);

		void setRowVisible(sl_uint32 row, sl_bool flagVisible, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		Ref<View> getCell(sl_uint32 row, sl_uint32 col);

		void setCell(sl_uint32 row, sl_uint32 col, const Ref<View>& view, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		void setCell(sl_uint32 row, sl_uint32 col, const Ref<View>& view, sl_uint32 rowspan, sl_uint32 colspan, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		sl_uint32 getRowspan(sl_uint32 row, sl_uint32 col);

		void setRowspan(sl_uint32 row, sl_uint32 col, sl_uint32 rowspan, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		sl_uint32 getColspan(sl_uint32 row, sl_uint32 col);

		void setColspan(sl_uint32 row, sl_uint32 col, sl_uint32 colspan, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		void setCellSpan(sl_uint32 row, sl_uint32 col, sl_uint32 rowspan, sl_uint32 colspan, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		Ref<Pen> getHorizontalGrid();

		void setHorizontalGrid(const Ref<Pen>& pen, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setHorizontalGrid(const PenDesc& desc, UIUpdateMode mode = UIUpdateMode::Redraw);

		Ref<Pen> getVerticalGrid();

		void setVerticalGrid(const Ref<Pen>& pen, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setVerticalGrid(const PenDesc& desc, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setGrid(const Ref<Pen>& pen, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		void setGrid(const PenDesc& desc, UIUpdateMode mode = UIUpdateMode::Redraw);

	protected:
		void onUpdateLayout() override;

		void onDraw(Canvas* canvas) override;

	private:
		void _applySpan(sl_uint32 row, sl_uint32 col, sl_uint32 rowspan, sl_uint32 colspan);

		void _drawGrids(View*, Canvas*);

	private:
		class Cell
		{
		public:
			Ref<View> view;
			sl_uint32 rowspan;
			sl_uint32 colspan;
			sl_bool flagSelfHorzAlign;
			sl_bool flagSelfVertAlign;

		public:
			Cell();

			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(Cell)
		};

	public:
		class Column : public CRef
		{
		public:
			Column(TableLayout* table);

			~Column();

		public:
			Ref<TableLayout> getTable();

			sl_uint32 getIndex();

			sl_bool remove(UIUpdateMode mode = UIUpdateMode::UpdateLayout);

			String getId();

			void setId(const String& _id);

			SizeMode getWidthMode();

			sl_ui_len getWidth();

			void setWidth(sl_ui_len width, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

			sl_bool isWidthFixed();

			sl_real getWidthWeight();

			sl_bool isWidthFilling();

			void setWidthFilling(sl_real weight = 1, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

			sl_bool isWidthWrapping();

			void setWidthWrapping(UIUpdateMode mode = UIUpdateMode::UpdateLayout);

			void setWidthWrapping(sl_bool flagWrapping, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

			sl_bool isWidthWeight();

			void setWidthWeight(sl_real weight, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

			sl_ui_len getMinimumWidth();

			void setMinimumWidth(sl_ui_len width, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

			sl_bool isMaximumWidthDefined();

			sl_ui_len getMaximumWidth();

			void setMaximumWidth(sl_ui_len width, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

			sl_ui_len getMarginLeft();

			void setMarginLeft(sl_ui_len margin, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

			sl_ui_len getMarginRight();

			void setMarginRight(sl_ui_len margin, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

			void setMargin(sl_ui_len margin, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

			sl_ui_len getPaddingLeft();

			void setPaddingLeft(sl_ui_len padding, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

			sl_ui_len getPaddingRight();

			void setPaddingRight(sl_ui_len padding, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

			void setPadding(sl_ui_len padding, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

			Ref<Drawable> getBackground();

			void setBackground(const Ref<Drawable>& background, UIUpdateMode mode = UIUpdateMode::Redraw);

			void setBackgroundColor(const Color& color, UIUpdateMode mode = UIUpdateMode::Redraw);

			Alignment getAlignment();

			void setAlignment(const Alignment& align, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

			sl_bool isVisible();

			void setVisible(sl_bool flagVisible = sl_true, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		private:
			sl_ui_len _restrictWidth(sl_ui_len width);

			sl_ui_len _getFixedWidth();

			sl_ui_len _getWeightWidth(sl_ui_len widthParent);

			void _invalidate(UIUpdateMode mode);

			void _invalidateLayout(UIUpdateMode mode);

		private:
			WeakRef<TableLayout> m_table;
			sl_int32 m_index;
			AtomicString m_id;

			SizeMode m_widthMode;
			sl_ui_len m_widthLayout;
			sl_ui_len m_widthFixed;
			sl_real m_widthWeight;

			sl_ui_len m_minWidth;
			sl_ui_len m_maxWidth;
			sl_bool m_flagMaxWidthDefined;

			sl_ui_len m_marginLeft;
			sl_ui_len m_marginRight;
			sl_ui_len m_paddingLeft;
			sl_ui_len m_paddingRight;

			AtomicRef<Drawable> m_background;
			Alignment m_align;
			sl_bool m_flagVisible;

			friend class TableLayout;
		};

		class Row : public CRef
		{
		public:
			Row(TableLayout* table);

			~Row();

		public:
			Ref<TableLayout> getTable();

			sl_uint32 getIndex();

			sl_bool remove(UIUpdateMode mode = UIUpdateMode::UpdateLayout);

			String getId();

			void setId(const String& _id);

			SizeMode getHeightMode();

			sl_ui_len getHeight();

			void setHeight(sl_ui_len height, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

			sl_bool isHeightFixed();

			sl_real getHeightWeight();

			sl_bool isHeightFilling();

			void setHeightFilling(sl_real weight = 1, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

			sl_bool isHeightWrapping();

			void setHeightWrapping(UIUpdateMode mode = UIUpdateMode::UpdateLayout);

			void setHeightWrapping(sl_bool flagWrapping, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

			sl_bool isHeightWeight();

			void setHeightWeight(sl_real weight, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

			sl_ui_len getMinimumHeight();

			void setMinimumHeight(sl_ui_len height, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

			sl_bool isMaximumHeightDefined();

			sl_ui_len getMaximumHeight();

			void setMaximumHeight(sl_ui_len height, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

			sl_ui_len getMarginTop();

			void setMarginTop(sl_ui_len margin, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

			sl_ui_len getMarginBottom();

			void setMarginBottom(sl_ui_len margin, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

			void setMargin(sl_ui_len margin, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

			sl_ui_len getPaddingTop();

			void setPaddingTop(sl_ui_len padding, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

			sl_ui_len getPaddingBottom();

			void setPaddingBottom(sl_ui_len padding, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

			void setPadding(sl_ui_len padding, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

			Ref<Drawable> getBackground();

			void setBackground(const Ref<Drawable>& background, UIUpdateMode mode = UIUpdateMode::Redraw);

			void setBackgroundColor(const Color& color, UIUpdateMode mode = UIUpdateMode::Redraw);

			Alignment getAlignment();

			void setAlignment(const Alignment& align, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

			sl_bool isVisible();

			void setVisible(sl_bool flagVisible, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		private:
			sl_ui_len _restrictHeight(sl_ui_len height);

			sl_ui_len _getFixedHeight();

			sl_ui_len _getWeightHeight(sl_ui_len heightParent);

			void _invalidate(UIUpdateMode mode);

			void _invalidateLayout(UIUpdateMode mode);

		private:
			WeakRef<TableLayout> m_table;
			sl_int32 m_index;
			AtomicString m_id;

			SizeMode m_heightMode;
			sl_ui_len m_heightLayout;
			sl_ui_len m_heightFixed;
			sl_real m_heightWeight;

			sl_ui_len m_minHeight;
			sl_ui_len m_maxHeight;
			sl_bool m_flagMaxHeightDefined;

			sl_ui_len m_marginTop;
			sl_ui_len m_marginBottom;
			sl_ui_len m_paddingTop;
			sl_ui_len m_paddingBottom;

			AtomicRef<Drawable> m_background;
			Alignment m_align;
			sl_bool m_flagVisible;

			CList<Cell> m_cells;

			friend class TableLayout;
		};

	private:
		Cell* _getCell(sl_uint32 row, sl_uint32 col);

		Cell* _allocCell(sl_uint32 row, sl_uint32 col);

		Alignment _getCellAlign(sl_uint32 row, sl_uint32 col);

		void _initCellAlign(Cell* cell, sl_uint32 row, sl_uint32 col);

		void _applyCellAlign(Cell* cell, sl_uint32 row, sl_uint32 col, UIUpdateMode mode);

		void _setColumnAlignment(Column* col, sl_uint32 iCol, const Alignment& align, UIUpdateMode mode);

		void _setColumnVisible(Column* col, sl_uint32 iCol, sl_bool flagVisible, UIUpdateMode mode);

		void _setRowAlignment(Row* row, sl_uint32 iRow, const Alignment& align, UIUpdateMode mode);

		void _setRowVisible(Row* row, sl_uint32 iRow, sl_bool flagVisible, UIUpdateMode mode);

	protected:
		CList< Ref<Column> > m_columns;
		CList< Ref<Row> > m_rows;
		AtomicRef<Pen> m_penHorzGrid;
		AtomicRef<Pen> m_penVertGrid;
		sl_bool m_flagSetHorzGrid;
		sl_bool m_flagSetVertGrid;

	};

}

#endif
