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

#include "slib/ui/table_layout.h"

#include "slib/core/scoped_buffer.h"
#include "slib/graphics/canvas.h"

namespace slib
{

	namespace priv
	{
		namespace table_layout
		{

			class Cell
			{
			public:
				Ref<View> view;
				sl_uint32 rowspan = 1;
				sl_uint32 colspan = 1;
				sl_bool flagSelfHorzAlign = sl_false;
				sl_bool flagSelfVertAlign = sl_false;

			};

			class Column
			{
			public:
				SizeMode widthMode = SizeMode::Filling;
				sl_ui_len widthLayout = 0;
				sl_ui_len widthFixed = 0;
				sl_real widthWeight = 1;

				sl_ui_len minWidth = 0;
				sl_ui_len maxWidth = 0;
				sl_bool flagMaxWidthDefined = sl_false;

				sl_ui_len marginLeft = 0;
				sl_ui_len marginRight = 0;
				sl_ui_len paddingLeft = 0;
				sl_ui_len paddingRight = 0;

				Ref<Drawable> background;
				Alignment align = Alignment::Default;

			public:
				sl_ui_len restrictWidth(sl_ui_len width)
				{
					if (width < minWidth) {
						return minWidth;
					}
					if (flagMaxWidthDefined) {
						if (width > maxWidth) {
							return maxWidth;
						}
					}
					return width;
				}

				sl_ui_len getFixedWidth()
				{
					return restrictWidth(widthFixed);
				}

				sl_ui_len getWeightWidth(sl_ui_len widthParent)
				{
					return restrictWidth((sl_ui_len)(widthParent * widthWeight));
				}

			};

			class Row
			{
			public:
				SizeMode heightMode = SizeMode::Filling;
				sl_ui_len heightLayout = 0;
				sl_ui_len heightFixed = 0;
				sl_real heightWeight = 1;

				sl_ui_len minHeight = 0;
				sl_ui_len maxHeight = 0;
				sl_bool flagMaxHeightDefined = sl_false;

				sl_ui_len marginTop = 0;
				sl_ui_len marginBottom = 0;
				sl_ui_len paddingTop = 0;
				sl_ui_len paddingBottom = 0;

				Ref<Drawable> background;
				Alignment align = Alignment::Default;

				CList<Cell> cells;

			public:
				sl_ui_len restrictHeight(sl_ui_len height)
				{
					if (height < minHeight) {
						return minHeight;
					}
					if (flagMaxHeightDefined) {
						if (height > maxHeight) {
							return maxHeight;
						}
					}
					return height;
				}

				sl_ui_len getFixedHeight()
				{
					return restrictHeight(heightFixed);
				}

				sl_ui_len getWeightHeight(sl_ui_len heightParent)
				{
					return restrictHeight((sl_ui_len)(heightParent * heightWeight));
				}

			};

		}
	}

	using namespace priv::table_layout;

	SLIB_DEFINE_OBJECT(TableLayout, ViewGroup)

	TableLayout::TableLayout()
	{
		setCustomLayout(sl_true);
		setSavingCanvasState(sl_false);
	}

	TableLayout::~TableLayout()
	{
	}

	sl_uint32 TableLayout::getColumnCount()
	{
		ObjectLocker lock(this);
		return (sl_uint32)(m_columns.getCount());
	}

	void TableLayout::setColumnCount(sl_uint32 nColumns, UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		sl_uint32 nColumnsOld = (sl_uint32)(m_columns.getCount());
		if (nColumnsOld == nColumns) {
			return;
		}
		if (nColumnsOld < nColumns) {
			m_columns.setCount_NoLock(nColumns);
			return;
		}
		sl_bool flagRemovedChild = sl_false;
		UIUpdateMode modeNone = (mode == UIUpdateMode::Init) ? (UIUpdateMode::Init) : (UIUpdateMode::None);
		ListElements<Row> rows(m_rows);
		for (sl_size i = 0; i < rows.count; i++) {
			Row& row = rows[i];
			ListElements<Cell> cells(row.cells);
			if (cells.count > nColumns) {
				for (sl_size k = nColumns; k < cells.count; k++) {
					Ref<View>& view = cells[k].view;
					if (view.isNotNull()) {
						removeChild(view, modeNone);
						flagRemovedChild = sl_true;
					}
				}
				row.cells.setCount_NoLock(nColumns);
			}
		}
		m_columns.setCount_NoLock(nColumns);
		if (flagRemovedChild) {
			invalidateLayout(mode);
		}
	}

	SizeMode TableLayout::getColumnWidthMode(sl_uint32 iCol)
	{
		ObjectLocker lock(this);
		Column* col = m_columns.getPointerAt(iCol);
		if (col) {
			return col->widthMode;
		}
		return SizeMode::Filling;
	}

	sl_ui_len TableLayout::getColumnWidth(sl_uint32 iCol)
	{
		ObjectLocker lock(this);
		Column* col = m_columns.getPointerAt(iCol);
		if (col) {
			if (col->widthMode == SizeMode::Fixed) {
				return col->getFixedWidth();
			} else {
				return col->widthLayout;
			}
		}
		return 0;
	}

	void TableLayout::setColumnWidth(sl_uint32 iCol, sl_ui_len width, UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		Column* col = m_columns.getPointerAt(iCol);
		if (col) {
			if (width < 0) {
				width = 0;
			}
			col->widthFixed = width;
			col->widthMode = SizeMode::Fixed;
			invalidateLayout(mode);
		}
	}

	sl_bool TableLayout::isColumnWidthFixed(sl_uint32 iCol)
	{
		ObjectLocker lock(this);
		Column* col = m_columns.getPointerAt(iCol);
		if (col) {
			return col->widthMode == SizeMode::Fixed;
		}
		return sl_false;
	}

	sl_real TableLayout::getColumnWidthWeight(sl_uint32 iCol)
	{
		ObjectLocker lock(this);
		Column* col = m_columns.getPointerAt(iCol);
		if (col) {
			return col->widthWeight;
		}
		return sl_false;
	}
	
	sl_bool TableLayout::isColumnWidthFilling(sl_uint32 iCol)
	{
		ObjectLocker lock(this);
		Column* col = m_columns.getPointerAt(iCol);
		if (col) {
			return col->widthMode == SizeMode::Filling;
		}
		return sl_false;
	}
	
	void TableLayout::setColumnWidthFilling(sl_uint32 iCol, sl_real weight, UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		Column* col = m_columns.getPointerAt(iCol);
		if (col) {
			if (weight < 0) {
				weight = 0;
			}
			col->widthWeight = weight;
			col->widthMode = SizeMode::Filling;
			invalidateLayout(mode);
		}
	}
	
	sl_bool TableLayout::isColumnWidthWrapping(sl_uint32 iCol)
	{
		ObjectLocker lock(this);
		Column* col = m_columns.getPointerAt(iCol);
		if (col) {
			return col->widthMode == SizeMode::Wrapping;
		}
		return sl_false;
	}
	
	void TableLayout::setColumnWidthWrapping(sl_uint32 iCol, UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		Column* col = m_columns.getPointerAt(iCol);
		if (col) {
			col->widthMode = SizeMode::Wrapping;
			invalidateLayout(mode);
		}
	}
	
	sl_bool TableLayout::isColumnWidthWeight(sl_uint32 iCol)
	{
		ObjectLocker lock(this);
		Column* col = m_columns.getPointerAt(iCol);
		if (col) {
			return col->widthMode == SizeMode::Weight;
		}
		return sl_false;
	}
	
	void TableLayout::setColumnWidthWeight(sl_uint32 iCol, sl_real weight, UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		Column* col = m_columns.getPointerAt(iCol);
		if (col) {
			if (weight < 0) {
				weight = 0;
			}
			col->widthWeight = weight;
			col->widthMode = SizeMode::Weight;
			invalidateLayout(mode);
		}
	}
	
	sl_ui_len TableLayout::getColumnMinimumWidth(sl_uint32 iCol)
	{
		ObjectLocker lock(this);
		Column* col = m_columns.getPointerAt(iCol);
		if (col) {
			return col->minWidth;
		}
		return 0;
	}
	
	void TableLayout::setColumnMinimumWidth(sl_uint32 iCol, sl_ui_len width, UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		Column* col = m_columns.getPointerAt(iCol);
		if (col) {
			if (width < 0) {
				width = 0;
			}
			col->minWidth = width;
			invalidateLayout(mode);
		}
	}
	
	sl_bool TableLayout::isColumnMaximumWidthDefined(sl_uint32 iCol)
	{
		ObjectLocker lock(this);
		Column* col = m_columns.getPointerAt(iCol);
		if (col) {
			return col->flagMaxWidthDefined;
		}
		return 0;
	}

	sl_ui_len TableLayout::getColumnMaximumWidth(sl_uint32 iCol)
	{
		ObjectLocker lock(this);
		Column* col = m_columns.getPointerAt(iCol);
		if (col) {
			return col->maxWidth;
		}
		return 0;
	}
	
	void TableLayout::setColumnMaximumWidth(sl_uint32 iCol, sl_ui_len width, UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		Column* col = m_columns.getPointerAt(iCol);
		if (col) {
			if (width < 0) {
				width = 0;
			}
			col->maxWidth = width;
			col->flagMaxWidthDefined = sl_true;
			invalidateLayout(mode);
		}
	}

	sl_ui_len TableLayout::getColumnMarginLeft(sl_uint32 iCol)
	{
		ObjectLocker lock(this);
		Column* col = m_columns.getPointerAt(iCol);
		if (col) {
			return col->marginLeft;
		}
		return 0;
	}

	void TableLayout::setColumnMarginLeft(sl_uint32 iCol, sl_ui_len margin, UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		Column* col = m_columns.getPointerAt(iCol);
		if (col) {
			col->marginLeft = margin;
			invalidateLayout(mode);
		}
	}

	sl_ui_len TableLayout::getColumnMarginRight(sl_uint32 iCol)
	{
		ObjectLocker lock(this);
		Column* col = m_columns.getPointerAt(iCol);
		if (col) {
			return col->marginRight;
		}
		return 0;
	}

	void TableLayout::setColumnMarginRight(sl_uint32 iCol, sl_ui_len margin, UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		Column* col = m_columns.getPointerAt(iCol);
		if (col) {
			col->marginRight = margin;
			invalidateLayout(mode);
		}
	}

	void TableLayout::setColumnMargin(sl_uint32 iCol, sl_ui_len margin, UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		Column* col = m_columns.getPointerAt(iCol);
		if (col) {
			col->marginLeft = margin;
			col->marginRight = margin;
			invalidateLayout(mode);
		}
	}

	sl_ui_len TableLayout::getColumnPaddingLeft(sl_uint32 iCol)
	{
		ObjectLocker lock(this);
		Column* col = m_columns.getPointerAt(iCol);
		if (col) {
			return col->paddingLeft;
		}
		return 0;
	}

	void TableLayout::setColumnPaddingLeft(sl_uint32 iCol, sl_ui_len padding, UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		Column* col = m_columns.getPointerAt(iCol);
		if (col) {
			col->paddingLeft = padding;
			invalidateLayout(mode);
		}
	}

	sl_ui_len TableLayout::getColumnPaddingRight(sl_uint32 iCol)
	{
		ObjectLocker lock(this);
		Column* col = m_columns.getPointerAt(iCol);
		if (col) {
			return col->paddingRight;
		}
		return 0;
	}

	void TableLayout::setColumnPaddingRight(sl_uint32 iCol, sl_ui_len padding, UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		Column* col = m_columns.getPointerAt(iCol);
		if (col) {
			col->paddingRight = padding;
			invalidateLayout(mode);
		}
	}

	void TableLayout::setColumnPadding(sl_uint32 iCol, sl_ui_len padding, UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		Column* col = m_columns.getPointerAt(iCol);
		if (col) {
			col->paddingLeft = padding;
			col->paddingRight = padding;
			invalidateLayout(mode);
		}
	}

	Ref<Drawable> TableLayout::getColumnBackground(sl_uint32 iCol)
	{
		ObjectLocker lock(this);
		Column* col = m_columns.getPointerAt(iCol);
		if (col) {
			return col->background;
		}
		return sl_null;
	}

	void TableLayout::setColumnBackground(sl_uint32 iCol, const Ref<Drawable>& background, UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		Column* col = m_columns.getPointerAt(iCol);
		if (col) {
			col->background = background;
			invalidateLayout(mode);
		}
	}

	void TableLayout::setColumnBackgroundColor(sl_uint32 iCol, const Color& color, UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		Column* col = m_columns.getPointerAt(iCol);
		if (col) {
			col->background = Drawable::createColorDrawable(color);
			invalidateLayout(mode);
		}
	}

	Alignment TableLayout::getColumnAlignment(sl_uint32 iCol)
	{
		ObjectLocker lock(this);
		Column* col = m_columns.getPointerAt(iCol);
		if (col) {
			return col->align;
		}
		return Alignment::Default;
	}

	void TableLayout::setColumnAlignment(sl_uint32 iCol, const Alignment& align, UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		Column* col = m_columns.getPointerAt(iCol);
		if (col) {
			col->align = align;
			sl_uint32 nRows = (sl_uint32)(m_rows.getCount());
			for (sl_uint32 i = 0; i < nRows; i++) {
				Cell* cell = _getCell(i, iCol);
				if (cell) {
					_applyCellAlign(cell, i, iCol, mode);
				}
			}
			invalidateLayout(mode);
		}
	}

	sl_uint32 TableLayout::getRowCount()
	{
		ObjectLocker lock(this);
		return (sl_uint32)(m_rows.getCount());
	}
	
	void TableLayout::setRowCount(sl_uint32 nRows, UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		sl_uint32 nRowsOld = (sl_uint32)(m_rows.getCount());
		if (nRowsOld == nRows) {
			return;
		}
		if (nRowsOld < nRows) {
			m_rows.setCount_NoLock(nRows);
			return;
		}
		sl_bool flagRemovedChild = sl_false;
		UIUpdateMode modeNone = (mode == UIUpdateMode::Init) ? (UIUpdateMode::Init) : (UIUpdateMode::None);
		ListElements<Row> rows(m_rows);
		for (sl_size i = nRows; i < rows.count; i++) {
			Row& row = rows[i];
			ListElements<Cell> cells(row.cells);
			for (sl_size k = 0; k < cells.count; k++) {
				Ref<View>& view = cells[k].view;
				if (view.isNotNull()) {
					removeChild(view, modeNone);
					flagRemovedChild = sl_true;
				}
			}
		}
		m_rows.setCount_NoLock(nRows);
		if (flagRemovedChild) {
			invalidateLayout(mode);
		}
	}

	SizeMode TableLayout::getRowHeightMode(sl_uint32 iRow)
	{
		ObjectLocker lock(this);
		Row* row = m_rows.getPointerAt(iRow);
		if (row) {
			return row->heightMode;
		}
		return SizeMode::Filling;
	}

	sl_ui_len TableLayout::getRowHeight(sl_uint32 iRow)
	{
		ObjectLocker lock(this);
		Row* row = m_rows.getPointerAt(iRow);
		if (row) {
			if (row->heightMode == SizeMode::Fixed) {
				return row->getFixedHeight();
			} else {
				return row->heightLayout;
			}
		}
		return 0;
	}

	void TableLayout::setRowHeight(sl_uint32 iRow, sl_ui_len height, UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		Row* row = m_rows.getPointerAt(iRow);
		if (row) {
			if (height < 0) {
				height = 0;
			}
			row->heightFixed = height;
			row->heightMode = SizeMode::Fixed;
			invalidateLayout(mode);
		}
	}

	sl_bool TableLayout::isRowHeightFixed(sl_uint32 iRow)
	{
		ObjectLocker lock(this);
		Row* row = m_rows.getPointerAt(iRow);
		if (row) {
			return row->heightMode == SizeMode::Fixed;
		}
		return sl_false;
	}
	
	sl_real TableLayout::getRowHeightWeight(sl_uint32 iRow)
	{
		ObjectLocker lock(this);
		Row* row = m_rows.getPointerAt(iRow);
		if (row) {
			return row->heightWeight;
		}
		return 0;
	}
	
	sl_bool TableLayout::isRowHeightFilling(sl_uint32 iRow)
	{
		ObjectLocker lock(this);
		Row* row = m_rows.getPointerAt(iRow);
		if (row) {
			return row->heightMode == SizeMode::Filling;
		}
		return sl_false;
	}
	
	void TableLayout::setRowHeightFilling(sl_uint32 iRow, sl_real weight, UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		Row* row = m_rows.getPointerAt(iRow);
		if (row) {
			if (weight < 0) {
				weight = 0;
			}
			row->heightWeight = weight;
			row->heightMode = SizeMode::Filling;
			invalidateLayout(mode);
		}
	}

	sl_bool TableLayout::isRowHeightWrapping(sl_uint32 iRow)
	{
		ObjectLocker lock(this);
		Row* row = m_rows.getPointerAt(iRow);
		if (row) {
			return row->heightMode == SizeMode::Wrapping;
		}
		return sl_false;
	}
	
	void TableLayout::setRowHeightWrapping(sl_uint32 iRow, UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		Row* row = m_rows.getPointerAt(iRow);
		if (row) {
			row->heightMode = SizeMode::Wrapping;
			invalidateLayout(mode);
		}
	}
	
	sl_bool TableLayout::isRowHeightWeight(sl_uint32 iRow)
	{
		ObjectLocker lock(this);
		Row* row = m_rows.getPointerAt(iRow);
		if (row) {
			return row->heightMode == SizeMode::Weight;
		}
		return sl_false;
	}
	
	void TableLayout::setRowHeightWeight(sl_uint32 iRow, sl_real weight, UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		Row* row = m_rows.getPointerAt(iRow);
		if (row) {
			if (weight < 0) {
				weight = 0;
			}
			row->heightWeight = weight;
			row->heightMode = SizeMode::Weight;
			invalidateLayout(mode);
		}
	}
	
	sl_ui_len TableLayout::getRowMinimumHeight(sl_uint32 iRow)
	{
		ObjectLocker lock(this);
		Row* row = m_rows.getPointerAt(iRow);
		if (row) {
			return row->minHeight;
		}
		return 0;
	}
	
	void TableLayout::setRowMinimumHeight(sl_uint32 iRow, sl_ui_len height, UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		Row* row = m_rows.getPointerAt(iRow);
		if (row) {
			if (height < 0) {
				height = 0;
			}
			row->minHeight = height;
			invalidateLayout(mode);
		}
	}

	sl_bool TableLayout::isRowMaximumHeightDefined(sl_uint32 iRow)
	{
		ObjectLocker lock(this);
		Row* row = m_rows.getPointerAt(iRow);
		if (row) {
			return row->flagMaxHeightDefined;
		}
		return sl_false;
	}

	sl_ui_len TableLayout::getRowMaximumHeight(sl_uint32 iRow)
	{
		ObjectLocker lock(this);
		Row* row = m_rows.getPointerAt(iRow);
		if (row) {
			return row->maxHeight;
		}
		return 0;
	}
	
	void TableLayout::setRowMaximumHeight(sl_uint32 iRow, sl_ui_len height, UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		Row* row = m_rows.getPointerAt(iRow);
		if (row) {
			if (height < 0) {
				height = 0;
			}
			row->maxHeight = height;
			row->flagMaxHeightDefined = sl_true;
			invalidateLayout(mode);
		}
	}

	sl_ui_len TableLayout::getRowMarginTop(sl_uint32 iRow)
	{
		ObjectLocker lock(this);
		Row* row = m_rows.getPointerAt(iRow);
		if (row) {
			return row->marginTop;
		}
		return 0;
	}

	void TableLayout::setRowMarginTop(sl_uint32 iRow, sl_ui_len margin, UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		Row* row = m_rows.getPointerAt(iRow);
		if (row) {
			row->marginTop = margin;
			invalidateLayout(mode);
		}
	}

	sl_ui_len TableLayout::getRowMarginBottom(sl_uint32 iRow)
	{
		ObjectLocker lock(this);
		Row* row = m_rows.getPointerAt(iRow);
		if (row) {
			return row->marginBottom;
		}
		return 0;
	}

	void TableLayout::setRowMarginBottom(sl_uint32 iRow, sl_ui_len margin, UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		Row* row = m_rows.getPointerAt(iRow);
		if (row) {
			row->marginBottom = margin;
			invalidateLayout(mode);
		}
	}

	void TableLayout::setRowMargin(sl_uint32 iRow, sl_ui_len margin, UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		Row* row = m_rows.getPointerAt(iRow);
		if (row) {
			row->marginTop = margin;
			row->marginBottom = margin;
			invalidateLayout(mode);
		}
	}

	sl_ui_len TableLayout::getRowPaddingTop(sl_uint32 iRow)
	{
		ObjectLocker lock(this);
		Row* row = m_rows.getPointerAt(iRow);
		if (row) {
			return row->paddingTop;
		}
		return 0;
	}

	void TableLayout::setRowPaddingTop(sl_uint32 iRow, sl_ui_len padding, UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		Row* row = m_rows.getPointerAt(iRow);
		if (row) {
			row->paddingTop = padding;
			invalidateLayout(mode);
		}
	}

	sl_ui_len TableLayout::getRowPaddingBottom(sl_uint32 iRow)
	{
		ObjectLocker lock(this);
		Row* row = m_rows.getPointerAt(iRow);
		if (row) {
			return row->paddingBottom;
		}
		return 0;
	}

	void TableLayout::setRowPaddingBottom(sl_uint32 iRow, sl_ui_len padding, UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		Row* row = m_rows.getPointerAt(iRow);
		if (row) {
			row->paddingBottom = padding;
			invalidateLayout(mode);
		}
	}

	void TableLayout::setRowPadding(sl_uint32 iRow, sl_ui_len padding, UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		Row* row = m_rows.getPointerAt(iRow);
		if (row) {
			row->paddingTop = padding;
			row->paddingBottom = padding;
			invalidateLayout(mode);
		}
	}

	Ref<Drawable> TableLayout::getRowBackground(sl_uint32 iRow)
	{
		ObjectLocker lock(this);
		Row* row = m_rows.getPointerAt(iRow);
		if (row) {
			return row->background;
		}
		return sl_null;
	}

	void TableLayout::setRowBackground(sl_uint32 iRow, const Ref<Drawable>& background, UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		Row* row = m_rows.getPointerAt(iRow);
		if (row) {
			row->background = background;
			invalidateLayout(mode);
		}
	}

	void TableLayout::setRowBackgroundColor(sl_uint32 iRow, const Color& color, UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		Row* row = m_rows.getPointerAt(iRow);
		if (row) {
			row->background = Drawable::createColorDrawable(color);
			invalidateLayout(mode);
		}
	}

	Alignment TableLayout::getRowAlignment(sl_uint32 iRow)
	{
		ObjectLocker lock(this);
		Row* row = m_rows.getPointerAt(iRow);
		if (row) {
			return row->align;
		}
		return Alignment::Default;
	}

	void TableLayout::setRowAlignment(sl_uint32 iRow, const Alignment& align, UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		Row* row = m_rows.getPointerAt(iRow);
		if (row) {
			row->align = align;
			ListElements<Cell> cells(row->cells);
			for (sl_size i = 0; i < cells.count; i++) {
				_applyCellAlign(cells.data + i, iRow, (sl_uint32)i, mode);
			}
			invalidateLayout(mode);
		}
	}

	Cell* TableLayout::_getCell(sl_uint32 iRow, sl_uint32 iCol)
	{
		Row* row = m_rows.getPointerAt(iRow);
		if (row) {
			return row->cells.getPointerAt(iCol);
		}
		return sl_null;
	}

	priv::table_layout::Cell* TableLayout::_allocCell(sl_uint32 iRow, sl_uint32 iCol)
	{
		if (iCol >= m_columns.getCount() || iRow >= m_rows.getCount()) {
			return sl_null;
		}
		Row* row = m_rows.getData() + iRow;
		if (iCol < row->cells.getCount()) {
			return row->cells.getData() + iCol;
		} else {
			if (row->cells.setCount_NoLock(iCol + 1)) {
				return row->cells.getData() + iCol;
			}
		}
		return sl_null;
	}

	Alignment TableLayout::_getCellAlign(sl_uint32 iRow, sl_uint32 iCol)
	{
		Alignment ret = Alignment::Default;
		Row* row = m_rows.getPointerAt(iRow);
		if (row) {
			ret = row->align;
		}
		Column* col = m_columns.getPointerAt(iCol);
		if (col) {
			if (!(ret & Alignment::HorizontalMask)) {
				ret |= col->align & Alignment::HorizontalMask;
			}
			if (!(ret & Alignment::VerticalMask)) {
				ret |= col->align & Alignment::VerticalMask;
			}
		}
		return ret;
	}

	void TableLayout::_initCellAlign(priv::table_layout::Cell* cell, sl_uint32 iRow, sl_uint32 iCol)
	{
		View* view = cell->view.get();
		sl_bool flagHorz = view->isLeftFree() && view->isRightFree();
		sl_bool flagVert = view->isTopFree() && view->isBottomFree();
		if (flagHorz || flagVert) {
			Alignment align = _getCellAlign(iRow, iCol);
			if (flagHorz) {
				Alignment horz = align & Alignment::HorizontalMask;
				if (horz == Alignment::Left) {
					view->setAlignParentLeft(UIUpdateMode::Init);
				} else if (horz == Alignment::Right) {
					view->setAlignParentRight(UIUpdateMode::Init);
				} else {
					view->setCenterHorizontal(UIUpdateMode::Init);
				}
			}
			if (flagVert) {
				Alignment vert = align & Alignment::VerticalMask;
				if (vert == Alignment::Top) {
					view->setAlignParentTop(UIUpdateMode::Init);
				} else if (vert == Alignment::Bottom) {
					view->setAlignParentBottom(UIUpdateMode::Init);
				} else {
					view->setCenterVertical(UIUpdateMode::Init);
				}
			}
		}
		cell->flagSelfHorzAlign = !flagHorz;
		cell->flagSelfVertAlign = !flagVert;
	}

	void TableLayout::_applyCellAlign(priv::table_layout::Cell* cell, sl_uint32 iRow, sl_uint32 iCol, UIUpdateMode mode)
	{
		if (!SLIB_UI_UPDATE_MODE_IS_INIT(mode)) {
			mode = UIUpdateMode::None;
		}
		View* view = cell->view.get();
		if (view && !(cell->flagSelfHorzAlign && cell->flagSelfVertAlign)) {
			Alignment align = _getCellAlign(iRow, iCol);
			if (!(cell->flagSelfHorzAlign)) {
				Alignment horz = align & Alignment::HorizontalMask;
				if (horz == Alignment::Left) {
					if (!(view->isAlignParentLeft())) {
						view->setRightFree(UIUpdateMode::Init);
						view->setAlignParentLeft(mode);
					}
				} else if (horz == Alignment::Right) {
					if (!(view->isAlignParentRight())) {
						view->setLeftFree(UIUpdateMode::Init);
						view->setAlignParentRight(mode);
					}
				} else {
					if (!(view->isCenterHorizontal())) {
						view->setRightFree(UIUpdateMode::Init);
						view->setCenterHorizontal(mode);
					}
				}
			}
			if (!(cell->flagSelfVertAlign)) {
				Alignment vert = align & Alignment::VerticalMask;
				if (vert == Alignment::Top) {
					if (!(view->isAlignParentTop())) {
						view->setBottomFree(UIUpdateMode::Init);
						view->setAlignParentTop(mode);
					}
				} else if (vert == Alignment::Bottom) {
					if (!(view->isAlignParentRight())) {
						view->setTopFree(UIUpdateMode::Init);
						view->setAlignParentRight(mode);
					}
				} else {
					if (!(view->isCenterVertical())) {
						view->setBottomFree(UIUpdateMode::Init);
						view->setCenterVertical(mode);
					}
				}
			}
		}
	}

	Ref<View> TableLayout::getCell(sl_uint32 iRow, sl_uint32 iCol)
	{
		ObjectLocker lock(this);
		Cell* cell = _getCell(iRow, iCol);
		if (cell) {
			return cell->view.get();
		}
		return sl_null;
	}

	void TableLayout::setCell(sl_uint32 iRow, sl_uint32 iCol, const Ref<View>& view, UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		Cell* cell;
		if (view.isNotNull()) {
			cell = _allocCell(iRow, iCol);
		} else {
			cell = _getCell(iRow, iCol);
		}
		if (!cell) {
			return;
		}
		if (cell->view.isNotNull()) {
			removeChild(cell->view, mode == UIUpdateMode::Init ? UIUpdateMode::Init : UIUpdateMode::None);
		}
		cell->view = view;
		if (view.isNotNull()) {
			_initCellAlign(cell, iRow, iCol);
			addChild(view, mode);
		}
	}

	void TableLayout::setCell(sl_uint32 iRow, sl_uint32 iCol, const Ref<View>& view, sl_uint32 rowspan, sl_uint32 colspan, UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		Cell* cell;
		if (view.isNotNull() || rowspan >= 2 || colspan >= 2) {
			cell = _allocCell(iRow, iCol);
		} else {
			cell = _getCell(iRow, iCol);
		}
		if (!cell) {
			return;
		}
		if (cell->view.isNotNull()) {
			removeChild(cell->view, mode == UIUpdateMode::Init ? UIUpdateMode::Init : UIUpdateMode::None);
		}
		cell->view = view;
		cell->rowspan = rowspan;
		cell->colspan = colspan;
		if (view.isNotNull()) {
			_initCellAlign(cell, iRow, iCol);
			addChild(view, mode);
		}
	}

	sl_uint32 TableLayout::getRowspan(sl_uint32 iRow, sl_uint32 iCol)
	{
		ObjectLocker lock(this);
		Cell* cell = _getCell(iRow, iCol);
		if (cell) {
			return cell->rowspan;
		}
		return 1;
	}

	void TableLayout::setRowspan(sl_uint32 iRow, sl_uint32 iCol, sl_uint32 rowspan, UIUpdateMode mode)
	{
		if (rowspan < 1) {
			rowspan = 1;
		}
		ObjectLocker lock(this);
		Cell* cell;
		if (rowspan >= 2) {
			cell = _allocCell(iRow, iCol);
		} else {
			cell = _getCell(iRow, iCol);
		}
		if (cell) {
			cell->rowspan = rowspan;
			invalidate(mode);
		}
	}

	sl_uint32 TableLayout::getColspan(sl_uint32 iRow, sl_uint32 iCol)
	{
		ObjectLocker lock(this);
		Cell* cell = _getCell(iRow, iCol);
		if (cell) {
			return cell->colspan;
		}
		return 1;
	}

	void TableLayout::setColspan(sl_uint32 iRow, sl_uint32 iCol, sl_uint32 colspan, UIUpdateMode mode)
	{
		if (colspan < 1) {
			colspan = 1;
		}
		ObjectLocker lock(this);
		Cell* cell;
		if (colspan >= 2) {
			cell = _allocCell(iRow, iCol);
		} else {
			cell = _getCell(iRow, iCol);
		}
		if (cell) {
			cell->colspan = colspan;
			invalidate(mode);
		}
	}

	void TableLayout::setCellSpan(sl_uint32 iRow, sl_uint32 iCol, sl_uint32 rowspan, sl_uint32 colspan, UIUpdateMode mode)
	{
		if (colspan < 1) {
			colspan = 1;
		}
		ObjectLocker lock(this);
		Cell* cell;
		if (rowspan >= 2 || colspan >= 2) {
			cell = _allocCell(iRow, iCol);
		} else {
			cell = _getCell(iRow, iCol);
		}
		if (cell) {
			cell->rowspan = rowspan;
			cell->colspan = colspan;
			invalidate(mode);
		}
	}

	void TableLayout::onUpdateLayout()
	{
		ObjectLocker lock(this);

		sl_uint32 nRows = (sl_uint32)(m_rows.getCount());
		sl_uint32 nCols = (sl_uint32)(m_columns.getCount());
		sl_bool flagWidthWrapping = isWidthWrapping();
		sl_bool flagHeightWrapping = isHeightWrapping();
		if (!nRows && !nCols) {
			if (flagWidthWrapping) {
				setLayoutWidth(getPaddingLeft() + getPaddingRight());
			}
			if (flagHeightWrapping) {
				setLayoutHeight(getPaddingTop() + getPaddingBottom());
			}
			return;
		}

		UIRect layoutFrameContainer = getLayoutFrame();
		sl_ui_len paddingContainerLeft = getPaddingLeft();
		sl_ui_len paddingContainerTop = getPaddingTop();
		sl_ui_len widthContainer = layoutFrameContainer.getWidth() - paddingContainerLeft - getPaddingTop();
		sl_ui_len heightContainer = layoutFrameContainer.getHeight() - paddingContainerTop - getPaddingBottom();

		sl_uint32 iRow, iCol;
		Row* rows = m_rows.getData();
		Column* cols = m_columns.getData();

		sl_uint32 nFillRows = 0;
		sl_uint32 nFillCols = 0;
		sl_ui_len sumWidth = 0;
		sl_ui_len sumHeight = 0;
		sl_real sumRowFillWeights = 0;
		sl_real sumColFillWeights = 0;
		sl_bool flagWrappingRows = sl_false;
		sl_bool flagWrappingCols = sl_false;
		
		SLIB_SCOPED_BUFFER(SizeMode, 64, colWidthModes, nCols)
		SLIB_SCOPED_BUFFER(SizeMode, 64, rowHeightModes, nRows)
		if (!(colWidthModes && rowHeightModes)) {
			return;
		}

		for (iRow = 0; iRow < nRows; iRow++) {
			Row& row = rows[iRow];
			if (flagHeightWrapping) {
				if (row.heightMode == SizeMode::Fixed) {
					rowHeightModes[iRow] = SizeMode::Fixed;
					row.heightLayout = row.getFixedHeight();
					sumHeight += row.heightLayout + row.marginTop + row.marginBottom;
				} else {
					rowHeightModes[iRow] = SizeMode::Wrapping;
					row.heightLayout = 0;
					flagWrappingRows = sl_true;
				}
			} else {
				rowHeightModes[iRow] = row.heightMode;
				switch (row.heightMode) {
					case SizeMode::Fixed:
						row.heightLayout = row.getFixedHeight();
						sumHeight += row.heightLayout + row.marginTop + row.marginBottom;
						break;
					case SizeMode::Weight:
						row.heightLayout = row.getWeightHeight(heightContainer);
						sumHeight += row.heightLayout + row.marginTop + row.marginBottom;
						break;
					case SizeMode::Filling:
						sumHeight += row.marginTop + row.marginBottom;
						nFillRows++;
						sumRowFillWeights += row.heightWeight;
						row.heightLayout = 0;
						break;
					case SizeMode::Wrapping:
						row.heightLayout = 0;
						flagWrappingRows = sl_true;
						break;
				}
			}
		}
		for (iCol = 0; iCol < nCols; iCol++) {
			Column& col = cols[iCol];
			if (flagWidthWrapping) {
				if (col.widthMode == SizeMode::Fixed) {
					colWidthModes[iCol] = SizeMode::Fixed;
					col.widthLayout = col.getFixedWidth();
					sumWidth += col.widthLayout + col.marginLeft + col.marginRight;
				} else {
					colWidthModes[iCol] = SizeMode::Wrapping;
					col.widthLayout = 0;
					flagWrappingCols = sl_true;
				}
			} else {
				colWidthModes[iCol] = col.widthMode;
				switch (col.widthMode) {
					case SizeMode::Fixed:
						col.widthLayout = col.getFixedWidth();
						sumWidth += col.widthLayout + col.marginLeft + col.marginRight;
						break;
					case SizeMode::Weight:
						col.widthLayout = col.getWeightWidth(widthContainer);
						sumWidth += col.widthLayout + col.marginLeft + col.marginRight;
						break;
					case SizeMode::Filling:
						sumWidth += col.marginLeft + col.marginRight;
						nFillCols++;
						sumColFillWeights += col.widthWeight;
						col.widthLayout = 0;
						break;
					case SizeMode::Wrapping:
						col.widthLayout = 0;
						flagWrappingCols = sl_true;
						break;
				}
			}
		}

		if (flagWrappingCols) {
			UpdateLayoutFrameParam updateLayoutParam;
			updateLayoutParam.parentContentFrame.left = 0;
			updateLayoutParam.parentContentFrame.top = 0;
			updateLayoutParam.flagUseLayout = sl_true;
			updateLayoutParam.flagHorizontal = sl_true;
			updateLayoutParam.flagVertical = sl_false;
			for (iRow = 0; iRow < nRows; iRow++) {
				Row& row = rows[iRow];
				Cell* cells = row.cells.getData();
				sl_uint32 nCells = Math::min((sl_uint32)(row.cells.getCount()), nCols);
				for (iCol = 0; iCol < nCells; iCol++) {
					if (colWidthModes[iCol] == SizeMode::Wrapping) {
						Cell& cell = cells[iCol];
						Column& col = cols[iCol];
						View* view = cell.view.get();
						if (view && cell.colspan == 1) {
							SizeMode mode = view->getWidthMode();
							if (mode == SizeMode::Fixed || mode == SizeMode::Wrapping) {
								updateLayoutParam.parentContentFrame.right = col.widthLayout - col.paddingLeft - col.paddingRight;
								updateLayoutParam.parentContentFrame.bottom = row.heightLayout - row.paddingTop - row.paddingBottom;
								view->setInvalidateLayoutFrameInParent();
								view->updateLayoutFrameInParent(updateLayoutParam);
								sl_ui_len w = col.restrictWidth(view->getLayoutWidth() + view->getMarginLeft() + view->getMarginRight() + col.paddingLeft + col.paddingRight);
								if (w > col.widthLayout) {
									col.widthLayout = w;
								}
							}
						}
					}
				}
			}
			for (iCol = 0; iCol < nCols; iCol++) {
				Column& col = cols[iCol];
				if (colWidthModes[iCol] == SizeMode::Wrapping) {
					sumWidth += col.widthLayout + col.marginLeft + col.marginRight;;
				}
			}
		}
		if (nFillCols) {
			if (sumWidth < 0) {
				sumWidth = 0;
			}
			sl_ui_len widthRemain;
			if (widthContainer > sumWidth) {
				widthRemain = widthContainer - sumWidth;
				if (widthRemain < 0) {
					widthRemain = 0;
				}
			} else {
				widthRemain = 0;
			}
			if (sumColFillWeights < SLIB_EPSILON) {
				sumColFillWeights = 1;
			}
			for (iCol = 0; iCol < nCols; iCol++) {
				Column& col = cols[iCol];
				if (colWidthModes[iCol] == SizeMode::Filling) {
					col.widthLayout = col.restrictWidth((sl_ui_len)(widthRemain * col.widthWeight / sumColFillWeights));
				}
			}
		}

		if (flagWrappingRows) {
			UpdateLayoutFrameParam updateLayoutParam;
			updateLayoutParam.parentContentFrame.left = 0;
			updateLayoutParam.parentContentFrame.top = 0;
			updateLayoutParam.flagUseLayout = sl_true;
			updateLayoutParam.flagHorizontal = sl_false;
			updateLayoutParam.flagVertical = sl_true;
			for (iRow = 0; iRow < nRows; iRow++) {
				Row& row = rows[iRow];
				if (rowHeightModes[iRow] == SizeMode::Wrapping) {
					Cell* cells = row.cells.getData();
					sl_uint32 nCells = Math::min((sl_uint32)(row.cells.getCount()), nCols);
					for (iCol = 0; iCol < nCells; iCol++) {
						Cell& cell = cells[iCol];
						Column& col = cols[iCol];
						View* view = cell.view.get();
						if (view && cell.rowspan == 1) {
							SizeMode mode = view->getHeightMode();
							if (mode == SizeMode::Fixed || mode == SizeMode::Wrapping) {
								updateLayoutParam.parentContentFrame.right = col.widthLayout - col.paddingLeft - col.paddingRight;
								updateLayoutParam.parentContentFrame.bottom = row.heightLayout - row.paddingTop - row.paddingBottom;
								view->setInvalidateLayoutFrameInParent();
								view->updateLayoutFrameInParent(updateLayoutParam);
								sl_ui_len h = row.restrictHeight(view->getLayoutHeight() + view->getMarginTop() + view->getMarginBottom() + row.paddingTop + row.paddingBottom);
								if (h > row.heightLayout) {
									row.heightLayout = h;
								}
							}
						}
					}
				}
			}
			for (iRow = 0; iRow < nRows; iRow++) {
				Row& row = rows[iRow];
				if (rowHeightModes[iRow] == SizeMode::Wrapping) {
					sumHeight += row.heightLayout + row.marginTop + row.marginBottom;
				}
			}
		}
		if (nFillRows) {
			if (sumHeight < 0) {
				sumHeight = 0;
			}
			sl_ui_len heightRemain;
			if (heightContainer > sumHeight) {
				heightRemain = heightContainer - sumHeight;
				if (heightRemain < 0) {
					heightRemain = 0;
				}
			} else {
				heightRemain = 0;
			}
			if (sumRowFillWeights < SLIB_EPSILON) {
				sumRowFillWeights = 1;
			}
			for (iRow = 0; iRow < nRows; iRow++) {
				Row& row = rows[iRow];
				if (rowHeightModes[iRow] == SizeMode::Filling) {
					row.heightLayout = row.restrictHeight((sl_ui_len)(heightRemain * row.heightWeight / sumRowFillWeights));
				}
			}
		}

		UpdateLayoutFrameParam updateLayoutParam;
		updateLayoutParam.flagUseLayout = sl_true;
		updateLayoutParam.flagHorizontal = sl_true;
		updateLayoutParam.flagVertical = sl_true;
		sl_ui_len y = paddingContainerTop;
		for (iRow = 0; iRow < nRows; iRow++) {
			Row& row = rows[iRow];
			Cell* cells = row.cells.getData();
			sl_uint32 nCells = Math::min((sl_uint32)(row.cells.getCount()), nCols);
			sl_ui_len x = paddingContainerLeft;
			for (iCol = 0; iCol < nCells; iCol++) {
				Cell& cell = cells[iCol];
				Column& col = cols[iCol];
				View* view = cell.view.get();
				if (view) {
					updateLayoutParam.parentContentFrame.left = x + col.marginLeft + col.paddingLeft;
					updateLayoutParam.parentContentFrame.top = y + row.marginTop + row.paddingTop;
					updateLayoutParam.parentContentFrame.right = x + col.marginLeft + col.widthLayout - col.paddingRight;
					updateLayoutParam.parentContentFrame.bottom = y + row.marginTop + row.heightLayout - row.paddingBottom;
					if (cell.colspan > 1) {
						for (sl_uint32 k = 1; k < cell.colspan; k++) {
							updateLayoutParam.parentContentFrame.right += cols[iCol + k - 1].paddingRight + cols[iCol + k - 1].marginRight;
							if (iCol + k >= nCols) {
								break;
							}
							updateLayoutParam.parentContentFrame.right += cols[iCol + k].marginLeft + cols[iCol + k].widthLayout - cols[iCol + k].paddingRight;
						}
					}
					if (cell.rowspan > 1) {
						for (sl_uint32 k = 1; k < cell.rowspan; k++) {
							updateLayoutParam.parentContentFrame.bottom += rows[iRow + k - 1].paddingBottom + rows[iRow + k - 1].marginBottom;
							if (iRow + k >= nRows) {
								break;
							}
							updateLayoutParam.parentContentFrame.bottom += rows[iRow + k].marginTop + rows[iRow + k].heightLayout - rows[iRow + k].paddingBottom;
						}
					}
					view->setInvalidateLayoutFrameInParent();
					view->updateLayoutFrameInParent(updateLayoutParam);
				}
				x += col.widthLayout + col.marginLeft + col.marginRight;
			}
			y += row.heightLayout + row.marginTop + row.marginBottom;
		}
		if (isWidthWrapping()) {
			sl_ui_len x = 0;
			for (iCol = 0; iCol < nCols; iCol++) {
				Column& col = cols[iCol];
				x += col.widthLayout + col.marginLeft + col.marginRight;
			}
			setLayoutWidth(x);
		}
		if (isHeightWrapping()) {
			setLayoutHeight(y);
		}
	}

	void TableLayout::onDraw(Canvas* canvas)
	{
		ObjectLocker lock(this);
		
		UIRect layoutFrameContainer = getLayoutFrame();
		sl_ui_len paddingContainerLeft = getPaddingLeft();
		sl_ui_len paddingContainerTop = getPaddingTop();
		
		{
			UIRect rc;
			rc.left = paddingContainerLeft;
			rc.right = rc.left + layoutFrameContainer.getWidth() - paddingContainerLeft - getPaddingTop();
			rc.top = paddingContainerTop;
			Row* rows = m_rows.getData();
			sl_uint32 nRows = (sl_uint32)(m_rows.getCount());
			for (sl_uint32 iRow = 0; iRow < nRows; iRow++) {
				Row& row = rows[iRow];
				rc.top += row.marginTop;
				rc.bottom = rc.top + row.heightLayout;
				if (row.background.isNotNull()) {
					canvas->draw(rc, row.background);
				}
				rc.top = rc.bottom + row.marginBottom;
			}
		}
		{
			UIRect rc;
			rc.top = paddingContainerTop;
			rc.bottom = rc.top + layoutFrameContainer.getHeight() - paddingContainerTop - getPaddingBottom();
			rc.left = paddingContainerLeft;
			Column* cols = m_columns.getData();
			sl_uint32 nCols = (sl_uint32)(m_columns.getCount());
			for (sl_uint32 iCol = 0; iCol < nCols; iCol++) {
				Column& col = cols[iCol];
				rc.left += col.marginLeft;
				rc.right = rc.left + col.widthLayout;
				if (col.background.isNotNull()) {
					canvas->draw(rc, col.background);
				}
				rc.left = rc.right + col.marginRight;
			}
		}
	}
	
}
