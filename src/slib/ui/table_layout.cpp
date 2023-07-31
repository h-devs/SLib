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

#include "slib/ui/table_layout.h"

#include "slib/core/scoped_buffer.h"
#include "slib/graphics/canvas.h"

namespace slib
{

	SLIB_DEFINE_NESTED_CLASS_DEFAULT_MEMBERS(TableLayout, Cell)

	TableLayout::Cell::Cell()
	{
		rowspan = 1;
		colspan = 1;
		flagSelfHorzAlign = sl_false;
		flagSelfVertAlign = sl_false;
	};


	TableLayout::Column::Column(TableLayout* table): m_table(table)
	{
		m_index = -1;

		m_widthMode = SizeMode::Filling;
		m_widthLayout = 0;
		m_widthFixed = 0;
		m_widthWeight = 1;

		m_minWidth = 0;
		m_maxWidth = 0;
		m_flagMaxWidthDefined = sl_false;

		m_marginLeft = 0;
		m_marginRight = 0;
		m_paddingLeft = 0;
		m_paddingRight = 0;

		m_align = Alignment::Default;
		m_flagVisible = sl_true;
	}

	TableLayout::Column::~Column()
	{
	}

	Ref<TableLayout> TableLayout::Column::getTable()
	{
		return m_table;
	}

	sl_uint32 TableLayout::Column::getIndex()
	{
		return m_index;
	}

	sl_ui_len TableLayout::Column::_restrictWidth(sl_ui_len width)
	{
		if (width < m_minWidth) {
			return m_minWidth;
		}
		if (m_flagMaxWidthDefined) {
			if (width > m_maxWidth) {
				return m_maxWidth;
			}
		}
		return width;
	}

	sl_ui_len TableLayout::Column::_getFixedWidth()
	{
		return _restrictWidth(m_widthFixed);
	}

	sl_ui_len TableLayout::Column::_getWeightWidth(sl_ui_len widthParent)
	{
		return _restrictWidth((sl_ui_len)(widthParent * m_widthWeight));
	}

	void TableLayout::Column::_invalidate(UIUpdateMode mode)
	{
		if (SLIB_UI_UPDATE_MODE_IS_INIT(mode)) {
			return;
		}
		if (m_index < 0) {
			return;
		}
		Ref<TableLayout> table = m_table;
		if (table.isNotNull()) {
			table->invalidate(mode);
		}
	}

	void TableLayout::Column::_invalidateLayout(UIUpdateMode mode)
	{
		if (SLIB_UI_UPDATE_MODE_IS_INIT(mode)) {
			return;
		}
		if (m_index < 0) {
			return;
		}
		Ref<TableLayout> table = m_table;
		if (table.isNotNull()) {
			table->invalidateLayout(mode);
		}
	}


	TableLayout::Row::Row(TableLayout* table): m_table(table)
	{
		m_index = -1;

		m_heightMode = SizeMode::Filling;
		m_heightLayout = 0;
		m_heightFixed = 0;
		m_heightWeight = 1;

		m_minHeight = 0;
		m_maxHeight = 0;
		m_flagMaxHeightDefined = sl_false;

		m_marginTop = 0;
		m_marginBottom = 0;
		m_paddingTop = 0;
		m_paddingBottom = 0;

		m_align = Alignment::Default;
		m_flagVisible = sl_true;
	}

	TableLayout::Row::~Row()
	{
	}

	Ref<TableLayout> TableLayout::Row::getTable()
	{
		return m_table;
	}

	sl_uint32 TableLayout::Row::getIndex()
	{
		return m_index;
	}

	sl_ui_len TableLayout::Row::_restrictHeight(sl_ui_len height)
	{
		if (height < m_minHeight) {
			return m_minHeight;
		}
		if (m_flagMaxHeightDefined) {
			if (height > m_maxHeight) {
				return m_maxHeight;
			}
		}
		return height;
	}

	sl_ui_len TableLayout::Row::_getFixedHeight()
	{
		return _restrictHeight(m_heightFixed);
	}

	sl_ui_len TableLayout::Row::_getWeightHeight(sl_ui_len heightParent)
	{
		return _restrictHeight((sl_ui_len)(heightParent * m_heightWeight));
	}

	void TableLayout::Row::_invalidate(UIUpdateMode mode)
	{
		if (SLIB_UI_UPDATE_MODE_IS_INIT(mode)) {
			return;
		}
		if (m_index < 0) {
			return;
		}
		Ref<TableLayout> table = m_table;
		if (table.isNotNull()) {
			table->invalidate(mode);
		}
	}

	void TableLayout::Row::_invalidateLayout(UIUpdateMode mode)
	{
		if (SLIB_UI_UPDATE_MODE_IS_INIT(mode)) {
			return;
		}
		if (m_index < 0) {
			return;
		}
		Ref<TableLayout> table = m_table;
		if (table.isNotNull()) {
			table->invalidateLayout(mode);
		}
	}


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

	sl_bool TableLayout::setColumnCount(sl_uint32 nColumns, UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		sl_uint32 nColumnsOld = (sl_uint32)(m_columns.getCount());
		if (nColumnsOld == nColumns) {
			return sl_true;
		}
		if (nColumnsOld < nColumns) {
			if (m_columns.setCount_NoLock(nColumns)) {
				Ref<Column>* columns = m_columns.getData();
				for (sl_uint32 i = nColumnsOld; i < nColumns; i++) {
					Ref<Column> col = new Column(this);
					if (col.isNotNull()) {
						col->m_index = (sl_uint32)i;
						columns[i] = Move(col);
					} else {
						m_columns.setCount_NoLock(nColumnsOld);
						return sl_false;
					}
				}
				return sl_true;
			}
		} else {
			{
				UIUpdateMode modeNone = (mode == UIUpdateMode::Init) ? (UIUpdateMode::Init) : (UIUpdateMode::None);
				ListElements< Ref<Row> > rows(m_rows);
				for (sl_size i = 0; i < rows.count; i++) {
					Row* row = rows[i].get();
					ListElements<Cell> cells(row->m_cells);
					if (cells.count > nColumns) {
						for (sl_size k = nColumns; k < cells.count; k++) {
							Ref<View>& view = cells[k].view;
							if (view.isNotNull()) {
								removeChild(view, modeNone);
							}
						}
						row->m_cells.setCount_NoLock(nColumns);
					}
				}
			}
			{
				Ref<Column>* columns = m_columns.getData();
				for (sl_uint32 i = nColumns; i < nColumnsOld; i++) {
					columns[i]->m_index = -1;
				}
			}
			if (m_columns.setCount_NoLock(nColumns)) {
				invalidateLayout(mode);
				return sl_true;
			}
		}
		return sl_false;
	}

	Ref<TableLayout::Column> TableLayout::getColumn(sl_uint32 index)
	{
		ObjectLocker lock(this);
		return m_columns.getValueAt_NoLock(index);
	}

	Ref<TableLayout::Column> TableLayout::addColumn()
	{
		ObjectLocker lock(this);
		sl_uint32 nColumns = (sl_uint32)(m_columns.getCount());
		Ref<Column> col = new Column(this);
		if (col.isNotNull()) {
			col->m_index = nColumns;
			if (m_columns.add_NoLock(col)) {
				return col;
			}
		}
		return sl_null;
	}

	Ref<TableLayout::Column> TableLayout::insertColumn(sl_uint32 index)
	{
		ObjectLocker lock(this);
		sl_uint32 nColumns = (sl_uint32)(m_columns.getCount());
		if (index > nColumns) {
			index = nColumns;
		}
		Ref<Column> col = new Column(this);
		if (col.isNotNull()) {
			col->m_index = index;
			if (m_columns.insert_NoLock(index, col)) {
				{
					Ref<Column>* columns = m_columns.getData();
					for (sl_uint32 i = index + 1; i <= nColumns; i++) {
						columns[i]->m_index = i;
					}
				}
				{
					ListElements< Ref<Row> > rows(m_rows);
					for (sl_size i = 0; i < rows.count; i++) {
						if (!(rows[i]->m_cells.insert_NoLock(index))) {
							return sl_null;
						}
					}
				}
				return col;
			}
		}
		return sl_null;
	}

	sl_bool TableLayout::removeColumn(sl_uint32 index, UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		Ref<Column> col;
		if (m_columns.removeAt_NoLock(index, &col)) {
			col->m_index = -1;
			{
				ListElements< Ref<Column> > columns(m_columns);
				for (sl_size i = index; i < columns.count; i++) {
					columns[i]->m_index = (sl_uint32)i;
				}
			}
			UIUpdateMode modeNone = (mode == UIUpdateMode::Init) ? (UIUpdateMode::Init) : (UIUpdateMode::None);
			{
				ListElements< Ref<Row> > rows(m_rows);
				for (sl_size i = 0; i < rows.count; i++) {
					Row* row = rows[i].get();
					Cell* cell = row->m_cells.getPointerAt(index);
					if (cell) {
						if (cell->view.isNotNull()) {
							removeChild(cell->view, modeNone);
						}
						row->m_cells.removeAt_NoLock(index);
					}
				}
			}
			invalidateLayout(mode);
			return sl_true;
		}
		return sl_false;
	}

	sl_bool TableLayout::Column::remove(UIUpdateMode mode)
	{
		if (m_index >= 0) {
			Ref<TableLayout> table = m_table;
			if (table.isNotNull()) {
				return table->removeColumn(m_index, mode);
			}
		}
		return sl_false;
	}

	SizeMode TableLayout::getColumnWidthMode(sl_uint32 iCol)
	{
		Ref<Column> col = getColumn(iCol);
		if (col.isNotNull()) {
			return col->m_widthMode;
		}
		return SizeMode::Filling;
	}

	SizeMode TableLayout::Column::getWidthMode()
	{
		return m_widthMode;
	}

	sl_ui_len TableLayout::getColumnWidth(sl_uint32 iCol)
	{
		Ref<Column> col = getColumn(iCol);
		if (col.isNotNull()) {
			return col->getWidth();
		}
		return 0;
	}

	sl_ui_len TableLayout::Column::getWidth()
	{
		if (m_widthMode == SizeMode::Fixed) {
			return _getFixedWidth();
		} else {
			return m_widthLayout;
		}
	}

	void TableLayout::setColumnWidth(sl_uint32 iCol, sl_ui_len width, UIUpdateMode mode)
	{
		Ref<Column> col = getColumn(iCol);
		if (col.isNotNull()) {
			col->setWidth(width, UIUpdateMode::Init);
			invalidateLayout(mode);
		}
	}

	void TableLayout::Column::setWidth(sl_ui_len width, UIUpdateMode mode)
	{
		if (width < 0) {
			width = 0;
		}
		m_widthFixed = width;
		m_widthMode = SizeMode::Fixed;
		_invalidateLayout(mode);
	}

	sl_bool TableLayout::isColumnWidthFixed(sl_uint32 iCol)
	{
		Ref<Column> col = getColumn(iCol);
		if (col.isNotNull()) {
			return col->isWidthFixed();
		}
		return sl_false;
	}

	sl_bool TableLayout::Column::isWidthFixed()
	{
		return m_widthMode == SizeMode::Fixed;
	}

	sl_real TableLayout::getColumnWidthWeight(sl_uint32 iCol)
	{
		Ref<Column> col = getColumn(iCol);
		if (col.isNotNull()) {
			return col->m_widthWeight;
		}
		return sl_false;
	}

	sl_real TableLayout::Column::getWidthWeight()
	{
		return m_widthWeight;
	}

	sl_bool TableLayout::isColumnWidthFilling(sl_uint32 iCol)
	{
		Ref<Column> col = getColumn(iCol);
		if (col.isNotNull()) {
			return col->isWidthFilling();
		}
		return sl_false;
	}

	sl_bool TableLayout::Column::isWidthFilling()
	{
		return m_widthMode == SizeMode::Filling;
	}

	void TableLayout::setColumnWidthFilling(sl_uint32 iCol, sl_real weight, UIUpdateMode mode)
	{
		Ref<Column> col = getColumn(iCol);
		if (col.isNotNull()) {
			col->setWidthFilling(weight, UIUpdateMode::Init);
			invalidateLayout(mode);
		}
	}

	void TableLayout::Column::setWidthFilling(sl_real weight, UIUpdateMode mode)
	{
		m_widthWeight = Math::abs(weight);
		m_widthMode = SizeMode::Filling;
		_invalidateLayout(mode);
	}

	sl_bool TableLayout::isColumnWidthWrapping(sl_uint32 iCol)
	{
		Ref<Column> col = getColumn(iCol);
		if (col.isNotNull()) {
			return col->isWidthWrapping();
		}
		return sl_false;
	}

	sl_bool TableLayout::Column::isWidthWrapping()
	{
		return m_widthMode == SizeMode::Wrapping;
	}

	void TableLayout::setColumnWidthWrapping(sl_uint32 iCol, UIUpdateMode mode)
	{
		Ref<Column> col = getColumn(iCol);
		if (col.isNotNull()) {
			col->setWidthWrapping(UIUpdateMode::Init);
			invalidateLayout(mode);
		}
	}

	void TableLayout::Column::setWidthWrapping(UIUpdateMode mode)
	{
		m_widthMode = SizeMode::Wrapping;
		_invalidateLayout(mode);
	}

	void TableLayout::setColumnWidthWrapping(sl_uint32 iCol, sl_bool flag, UIUpdateMode mode)
	{
		Ref<Column> col = getColumn(iCol);
		if (col.isNotNull()) {
			col->setWidthWrapping(flag, UIUpdateMode::Init);
			invalidateLayout(mode);
		}
	}

	void TableLayout::Column::setWidthWrapping(sl_bool flag, UIUpdateMode mode)
	{
		if (flag) {
			setWidthWrapping(mode);
		} else {
			if (m_widthMode == SizeMode::Wrapping) {
				m_widthMode = SizeMode::Fixed;
				m_widthFixed = m_widthLayout;
				_invalidateLayout(mode);
			}
		}
	}

	sl_bool TableLayout::isColumnWidthWeight(sl_uint32 iCol)
	{
		Ref<Column> col = getColumn(iCol);
		if (col.isNotNull()) {
			return col->isWidthWeight();
		}
		return sl_false;
	}

	sl_bool TableLayout::Column::isWidthWeight()
	{
		return m_widthMode == SizeMode::Weight;
	}

	void TableLayout::setColumnWidthWeight(sl_uint32 iCol, sl_real weight, UIUpdateMode mode)
	{
		Ref<Column> col = getColumn(iCol);
		if (col.isNotNull()) {
			col->setWidthWeight(weight, UIUpdateMode::Init);
			invalidateLayout(mode);
		}
	}

	void TableLayout::Column::setWidthWeight(sl_real weight, UIUpdateMode mode)
	{
		m_widthWeight = Math::abs(weight);
		m_widthMode = SizeMode::Weight;
		_invalidateLayout(mode);
	}

	sl_ui_len TableLayout::getColumnMinimumWidth(sl_uint32 iCol)
	{
		Ref<Column> col = getColumn(iCol);
		if (col.isNotNull()) {
			return col->m_minWidth;
		}
		return 0;
	}

	sl_ui_len TableLayout::Column::getMinimumWidth()
	{
		return m_minWidth;
	}

	void TableLayout::setColumnMinimumWidth(sl_uint32 iCol, sl_ui_len width, UIUpdateMode mode)
	{
		Ref<Column> col = getColumn(iCol);
		if (col.isNotNull()) {
			col->setMinimumWidth(width, UIUpdateMode::Init);
			invalidateLayout(mode);
		}
	}

	void TableLayout::Column::setMinimumWidth(sl_ui_len width, UIUpdateMode mode)
	{
		if (width < 0) {
			width = 0;
		}
		m_minWidth = width;
		_invalidateLayout(mode);
	}

	sl_bool TableLayout::isColumnMaximumWidthDefined(sl_uint32 iCol)
	{
		Ref<Column> col = getColumn(iCol);
		if (col.isNotNull()) {
			return col->m_flagMaxWidthDefined;
		}
		return sl_false;
	}

	sl_bool TableLayout::Column::isMaximumWidthDefined()
	{
		return m_flagMaxWidthDefined;
	}

	sl_ui_len TableLayout::getColumnMaximumWidth(sl_uint32 iCol)
	{
		Ref<Column> col = getColumn(iCol);
		if (col.isNotNull()) {
			return col->m_maxWidth;
		}
		return 0;
	}

	sl_ui_len TableLayout::Column::getMaximumWidth()
	{
		return m_maxWidth;
	}

	void TableLayout::setColumnMaximumWidth(sl_uint32 iCol, sl_ui_len width, UIUpdateMode mode)
	{
		Ref<Column> col = getColumn(iCol);
		if (col.isNotNull()) {
			col->setMaximumWidth(width, UIUpdateMode::Init);
			invalidateLayout(mode);
		}
	}

	void TableLayout::Column::setMaximumWidth(sl_ui_len width, UIUpdateMode mode)
	{
		if (width < 0) {
			width = 0;
		}
		m_maxWidth = width;
		m_flagMaxWidthDefined = sl_true;
		_invalidateLayout(mode);
	}

	sl_ui_len TableLayout::getColumnMarginLeft(sl_uint32 iCol)
	{
		Ref<Column> col = getColumn(iCol);
		if (col.isNotNull()) {
			return col->m_marginLeft;
		}
		return 0;
	}

	sl_ui_len TableLayout::Column::getMarginLeft()
	{
		return m_marginLeft;
	}

	void TableLayout::setColumnMarginLeft(sl_uint32 iCol, sl_ui_len margin, UIUpdateMode mode)
	{
		Ref<Column> col = getColumn(iCol);
		if (col.isNotNull()) {
			col->setMarginLeft(margin, UIUpdateMode::Init);
			invalidateLayout(mode);
		}
	}

	void TableLayout::Column::setMarginLeft(sl_ui_len margin, UIUpdateMode mode)
	{
		m_marginLeft = margin;
		_invalidateLayout(mode);
	}

	sl_ui_len TableLayout::getColumnMarginRight(sl_uint32 iCol)
	{
		Ref<Column> col = getColumn(iCol);
		if (col.isNotNull()) {
			return col->m_marginRight;
		}
		return 0;
	}

	sl_ui_len TableLayout::Column::getMarginRight()
	{
		return m_marginRight;
	}

	void TableLayout::setColumnMarginRight(sl_uint32 iCol, sl_ui_len margin, UIUpdateMode mode)
	{
		Ref<Column> col = getColumn(iCol);
		if (col.isNotNull()) {
			col->setMarginRight(margin, UIUpdateMode::Init);
			invalidateLayout(mode);
		}
	}

	void TableLayout::Column::setMarginRight(sl_ui_len margin, UIUpdateMode mode)
	{
		m_marginRight = margin;
		_invalidateLayout(mode);
	}

	void TableLayout::setColumnMargin(sl_uint32 iCol, sl_ui_len margin, UIUpdateMode mode)
	{
		Ref<Column> col = getColumn(iCol);
		if (col.isNotNull()) {
			col->setMargin(margin, UIUpdateMode::Init);
			invalidateLayout(mode);
		}
	}

	void TableLayout::Column::setMargin(sl_ui_len margin, UIUpdateMode mode)
	{
		m_marginLeft = margin;
		m_marginRight = margin;
		_invalidateLayout(mode);
	}

	sl_ui_len TableLayout::getColumnPaddingLeft(sl_uint32 iCol)
	{
		Ref<Column> col = getColumn(iCol);
		if (col.isNotNull()) {
			return col->m_paddingLeft;
		}
		return 0;
	}

	sl_ui_len TableLayout::Column::getPaddingLeft()
	{
		return m_paddingLeft;
	}

	void TableLayout::setColumnPaddingLeft(sl_uint32 iCol, sl_ui_len padding, UIUpdateMode mode)
	{
		Ref<Column> col = getColumn(iCol);
		if (col.isNotNull()) {
			col->setPaddingLeft(padding, UIUpdateMode::Init);
			invalidateLayout(mode);
		}
	}

	void TableLayout::Column::setPaddingLeft(sl_ui_len padding, UIUpdateMode mode)
	{
		m_paddingLeft = padding;
		_invalidateLayout(mode);
	}

	sl_ui_len TableLayout::getColumnPaddingRight(sl_uint32 iCol)
	{
		Ref<Column> col = getColumn(iCol);
		if (col.isNotNull()) {
			return col->m_paddingRight;
		}
		return 0;
	}

	sl_ui_len TableLayout::Column::getPaddingRight()
	{
		return m_paddingRight;
	}

	void TableLayout::setColumnPaddingRight(sl_uint32 iCol, sl_ui_len padding, UIUpdateMode mode)
	{
		Ref<Column> col = getColumn(iCol);
		if (col.isNotNull()) {
			col->setPaddingRight(padding, UIUpdateMode::Init);
			invalidateLayout(mode);
		}
	}

	void TableLayout::Column::setPaddingRight(sl_ui_len padding, UIUpdateMode mode)
	{
		m_paddingRight = padding;
		_invalidateLayout(mode);
	}

	void TableLayout::setColumnPadding(sl_uint32 iCol, sl_ui_len padding, UIUpdateMode mode)
	{
		Ref<Column> col = getColumn(iCol);
		if (col.isNotNull()) {
			col->setPadding(padding, UIUpdateMode::Init);
			invalidateLayout(mode);
		}
	}

	void TableLayout::Column::setPadding(sl_ui_len padding, UIUpdateMode mode)
	{
		m_paddingLeft = padding;
		m_paddingRight = padding;
		_invalidateLayout(mode);
	}

	Ref<Drawable> TableLayout::getColumnBackground(sl_uint32 iCol)
	{
		Ref<Column> col = getColumn(iCol);
		if (col.isNotNull()) {
			return col->m_background;
		}
		return sl_null;
	}

	Ref<Drawable> TableLayout::Column::getBackground()
	{
		return m_background;
	}

	void TableLayout::setColumnBackground(sl_uint32 iCol, const Ref<Drawable>& background, UIUpdateMode mode)
	{
		Ref<Column> col = getColumn(iCol);
		if (col.isNotNull()) {
			col->setBackground(background, UIUpdateMode::Init);
			invalidate(mode);
		}
	}

	void TableLayout::Column::setBackground(const Ref<Drawable>& _background, UIUpdateMode mode)
	{
		m_background = _background;
		_invalidate(mode);
	}

	void TableLayout::setColumnBackgroundColor(sl_uint32 iCol, const Color& color, UIUpdateMode mode)
	{
		Ref<Column> col = getColumn(iCol);
		if (col.isNotNull()) {
			col->setBackgroundColor(color, UIUpdateMode::Init);
			invalidate(mode);
		}
	}

	void TableLayout::Column::setBackgroundColor(const Color& color, UIUpdateMode mode)
	{
		m_background = Drawable::fromColor(color);
		_invalidate(mode);
	}

	Alignment TableLayout::getColumnAlignment(sl_uint32 iCol)
	{
		Ref<Column> col = getColumn(iCol);
		if (col.isNotNull()) {
			return col->m_align;
		}
		return Alignment::Default;
	}

	Alignment TableLayout::Column::getAlignment()
	{
		return m_align;
	}

	void TableLayout::_setColumnAlignment(Column* col, sl_uint32 iCol, const Alignment& align, UIUpdateMode mode)
	{
		col->m_align = align;
		sl_uint32 nRows = (sl_uint32)(m_rows.getCount());
		for (sl_uint32 i = 0; i < nRows; i++) {
			Cell* cell = _getCell(i, iCol);
			if (cell) {
				_applyCellAlign(cell, i, iCol, mode);
			}
		}
		invalidateLayout(mode);
	}

	void TableLayout::setColumnAlignment(sl_uint32 iCol, const Alignment& align, UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		Ref<Column> col = m_columns.getValueAt_NoLock(iCol);
		if (col.isNotNull()) {
			_setColumnAlignment(col.get(), iCol, align, mode);
		}
	}

	void TableLayout::Column::setAlignment(const Alignment& align, UIUpdateMode mode)
	{
		Ref<TableLayout> table = m_table;
		if (table.isNotNull()) {
			ObjectLocker lock(table.get());
			if (m_index >= 0) {
				table->_setColumnAlignment(this, m_index, align, mode);
			}
		}
	}

	sl_bool TableLayout::isColumnVisible(sl_uint32 iCol)
	{
		Ref<Column> col = getColumn(iCol);
		if (col.isNotNull()) {
			return col->m_flagVisible;
		}
		return sl_false;
	}

	sl_bool TableLayout::Column::isVisible()
	{
		return m_flagVisible;
	}

	void TableLayout::_setColumnVisible(Column* col, sl_uint32 iCol, sl_bool flagVisible, UIUpdateMode mode)
	{
		col->m_flagVisible = flagVisible;
		if (SLIB_UI_UPDATE_MODE_IS_INIT(mode)) {
			return;
		}
		ListElements< Ref<Row> > rows(m_rows);
		for (sl_size iRow = 0; iRow < rows.count; iRow++) {
			Row* row = rows[iRow].get();
			sl_bool f = flagVisible;
			if (f) {
				f = row->m_flagVisible;
			}
			Cell* cell = row->m_cells.getPointerAt(iCol);
			if (cell) {
				if (cell->view.isNotNull()) {
					cell->view->setVisible(f, UIUpdateMode::None);
				}
			}
		}
		invalidateLayout(mode);
	}

	void TableLayout::setColumnVisible(sl_uint32 iCol, sl_bool flagVisible, UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		Ref<Column> col = m_columns.getValueAt_NoLock(iCol);
		if (col.isNotNull()) {
			_setColumnVisible(col.get(), iCol, flagVisible, mode);
		}
	}

	void TableLayout::Column::setVisible(sl_bool flagVisible, UIUpdateMode mode)
	{
		Ref<TableLayout> table = m_table;
		if (table.isNotNull()) {
			ObjectLocker lock(table.get());
			if (m_index >= 0) {
				table->_setColumnVisible(this, m_index, flagVisible, mode);
			}
		}
	}

	sl_uint32 TableLayout::getRowCount()
	{
		ObjectLocker lock(this);
		return (sl_uint32)(m_rows.getCount());
	}

	sl_bool TableLayout::setRowCount(sl_uint32 nRows, UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		sl_uint32 nRowsOld = (sl_uint32)(m_rows.getCount());
		if (nRowsOld == nRows) {
			return sl_true;
		}
		if (nRowsOld < nRows) {
			if (m_rows.setCount_NoLock(nRows)) {
				Ref<Row>* rows = m_rows.getData();
				for (sl_uint32 i = nRowsOld; i < nRows; i++) {
					Ref<Row> row = new Row(this);
					if (row.isNotNull()) {
						row->m_index = i;
						rows[i] = Move(row);
					} else {
						m_rows.setCount_NoLock(nRowsOld);
						return sl_false;
					}
				}
				return sl_true;
			}
		} else {
			sl_bool flagRemovedChild = sl_false;
			UIUpdateMode modeNone = (mode == UIUpdateMode::Init) ? (UIUpdateMode::Init) : (UIUpdateMode::None);
			Ref<Row>* rows = m_rows.getData();
			for (sl_uint32 i = nRows; i < nRowsOld; i++) {
				Row* row = rows[i].get();
				row->m_index = -1;
				ListElements<Cell> cells(row->m_cells);
				for (sl_size k = 0; k < cells.count; k++) {
					Ref<View>& view = cells[k].view;
					if (view.isNotNull()) {
						removeChild(view, modeNone);
						flagRemovedChild = sl_true;
					}
				}
			}
			if (m_rows.setCount_NoLock(nRows)) {
				if (flagRemovedChild) {
					invalidateLayout(mode);
				}
				return sl_true;
			}
		}
		return sl_false;
	}

	Ref<TableLayout::Row> TableLayout::getRow(sl_uint32 index)
	{
		ObjectLocker lock(this);
		return m_rows.getValueAt_NoLock(index);
	}

	Ref<TableLayout::Row> TableLayout::addRow()
	{
		ObjectLocker lock(this);
		sl_uint32 nRows = (sl_uint32)(m_rows.getCount());
		Ref<Row> row = new Row(this);
		if (row.isNotNull()) {
			row->m_index = nRows;
			if (m_rows.add_NoLock(row)) {
				return row;
			}
		}
		return sl_null;
	}

	Ref<TableLayout::Row> TableLayout::insertRow(sl_uint32 index)
	{
		ObjectLocker lock(this);
		sl_uint32 nRows = (sl_uint32)(m_rows.getCount());
		if (index > nRows) {
			index = nRows;
		}
		Ref<Row> row = new Row(this);
		if (row.isNotNull()) {
			row->m_index = index;
			if (m_rows.insert_NoLock(index, row)) {
				Ref<Row>* rows = m_rows.getData();
				for (sl_uint32 i = index + 1; i <= nRows; i++) {
					rows[i]->m_index = i;
				}
				return row;
			}
		}
		return sl_null;
	}

	sl_bool TableLayout::removeRow(sl_uint32 index, UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		Ref<Row> row;
		if (m_rows.removeAt_NoLock(index, &row)) {
			row->m_index = -1;
			{
				ListElements< Ref<Row> > rows(m_rows);
				for (sl_size i = index; i < rows.count; i++) {
					rows[i]->m_index = (sl_uint32)i;
				}
			}
			UIUpdateMode modeNone = (mode == UIUpdateMode::Init) ? (UIUpdateMode::Init) : (UIUpdateMode::None);
			{
				ListElements<Cell> cells(row->m_cells);
				for (sl_size i = 0; i < cells.count; i++) {
					Cell& cell = cells[i];
					if (cell.view.isNotNull()) {
						removeChild(cell.view, modeNone);
					}
				}
			}
			invalidateLayout(mode);
			return sl_true;
		}
		return sl_false;
	}

	sl_bool TableLayout::Row::remove(UIUpdateMode mode)
	{
		if (m_index >= 0) {
			Ref<TableLayout> table = m_table;
			if (table.isNotNull()) {
				return table->removeRow(m_index, mode);
			}
		}
		return sl_false;
	}

	SizeMode TableLayout::getRowHeightMode(sl_uint32 iRow)
	{
		Ref<Row> row = getRow(iRow);
		if (row.isNotNull()) {
			return row->m_heightMode;
		}
		return SizeMode::Filling;
	}

	SizeMode TableLayout::Row::getHeightMode()
	{
		return m_heightMode;
	}

	sl_ui_len TableLayout::getRowHeight(sl_uint32 iRow)
	{
		Ref<Row> row = getRow(iRow);
		if (row.isNotNull()) {
			return row->getHeight();
		}
		return 0;
	}

	sl_ui_len TableLayout::Row::getHeight()
	{
		if (m_heightMode == SizeMode::Fixed) {
			return _getFixedHeight();
		} else {
			return m_heightLayout;
		}
	}

	void TableLayout::setRowHeight(sl_uint32 iRow, sl_ui_len height, UIUpdateMode mode)
	{
		Ref<Row> row = getRow(iRow);
		if (row.isNotNull()) {
			row->setHeight(height, UIUpdateMode::Init);
			invalidateLayout(mode);
		}
	}

	void TableLayout::Row::setHeight(sl_ui_len height, UIUpdateMode mode)
	{
		if (height < 0) {
			height = 0;
		}
		m_heightFixed = height;
		m_heightMode = SizeMode::Fixed;
		_invalidateLayout(mode);
	}

	sl_bool TableLayout::isRowHeightFixed(sl_uint32 iRow)
	{
		Ref<Row> row = getRow(iRow);
		if (row.isNotNull()) {
			return row->isHeightFixed();
		}
		return sl_false;
	}

	sl_bool TableLayout::Row::isHeightFixed()
	{
		return m_heightMode == SizeMode::Fixed;
	}

	sl_real TableLayout::getRowHeightWeight(sl_uint32 iRow)
	{
		Ref<Row> row = getRow(iRow);
		if (row.isNotNull()) {
			return row->m_heightWeight;
		}
		return 0;
	}

	sl_real TableLayout::Row::getHeightWeight()
	{
		return m_heightWeight;
	}

	sl_bool TableLayout::isRowHeightFilling(sl_uint32 iRow)
	{
		Ref<Row> row = getRow(iRow);
		if (row.isNotNull()) {
			return row->isHeightFilling();
		}
		return sl_false;
	}

	sl_bool TableLayout::Row::isHeightFilling()
	{
		return m_heightMode == SizeMode::Filling;
	}

	void TableLayout::setRowHeightFilling(sl_uint32 iRow, sl_real weight, UIUpdateMode mode)
	{
		Ref<Row> row = getRow(iRow);
		if (row.isNotNull()) {
			row->setHeightFilling(weight, UIUpdateMode::Init);
			invalidateLayout(mode);
		}
	}

	void TableLayout::Row::setHeightFilling(sl_real weight, UIUpdateMode mode)
	{
		if (weight < 0) {
			weight = 0;
		}
		m_heightWeight = weight;
		m_heightMode = SizeMode::Filling;
		_invalidateLayout(mode);
	}

	sl_bool TableLayout::isRowHeightWrapping(sl_uint32 iRow)
	{
		Ref<Row> row = getRow(iRow);
		if (row.isNotNull()) {
			return row->isHeightWrapping();
		}
		return sl_false;
	}

	sl_bool TableLayout::Row::isHeightWrapping()
	{
		return m_heightMode == SizeMode::Wrapping;
	}

	void TableLayout::setRowHeightWrapping(sl_uint32 iRow, UIUpdateMode mode)
	{
		Ref<Row> row = getRow(iRow);
		if (row.isNotNull()) {
			row->setHeightWrapping(UIUpdateMode::Init);
			invalidateLayout(mode);
		}
	}

	void TableLayout::Row::setHeightWrapping(UIUpdateMode mode)
	{
		m_heightMode = SizeMode::Wrapping;
		_invalidateLayout(mode);
	}

	void TableLayout::setRowHeightWrapping(sl_uint32 iRow, sl_bool flag, UIUpdateMode mode)
	{
		Ref<Row> row = getRow(iRow);
		if (row.isNotNull()) {
			row->setHeightWrapping(flag, UIUpdateMode::Init);
			invalidateLayout(mode);
		}
	}

	void TableLayout::Row::setHeightWrapping(sl_bool flag, UIUpdateMode mode)
	{
		if (flag) {
			setHeightWrapping(mode);
		} else {
			if (m_heightMode == SizeMode::Wrapping) {
				m_heightMode = SizeMode::Fixed;
				m_heightFixed = m_heightLayout;
				_invalidateLayout(mode);
			}
		}
	}

	sl_bool TableLayout::isRowHeightWeight(sl_uint32 iRow)
	{
		Ref<Row> row = getRow(iRow);
		if (row.isNotNull()) {
			return row->isHeightWeight();
		}
		return sl_false;
	}

	sl_bool TableLayout::Row::isHeightWeight()
	{
		return m_heightMode == SizeMode::Weight;
	}

	void TableLayout::setRowHeightWeight(sl_uint32 iRow, sl_real weight, UIUpdateMode mode)
	{
		Ref<Row> row = getRow(iRow);
		if (row.isNotNull()) {
			row->setHeightWeight(weight, UIUpdateMode::Init);
			invalidateLayout(mode);
		}
	}

	void TableLayout::Row::setHeightWeight(sl_real weight, UIUpdateMode mode)
	{
		if (weight < 0) {
			weight = 0;
		}
		m_heightWeight = weight;
		m_heightMode = SizeMode::Weight;
		_invalidateLayout(mode);
	}

	sl_ui_len TableLayout::getRowMinimumHeight(sl_uint32 iRow)
	{
		Ref<Row> row = getRow(iRow);
		if (row.isNotNull()) {
			return row->m_minHeight;
		}
		return 0;
	}

	sl_ui_len TableLayout::Row::getMinimumHeight()
	{
		return m_minHeight;
	}

	void TableLayout::setRowMinimumHeight(sl_uint32 iRow, sl_ui_len height, UIUpdateMode mode)
	{
		Ref<Row> row = getRow(iRow);
		if (row.isNotNull()) {
			row->setMinimumHeight(height, UIUpdateMode::Init);
			invalidateLayout(mode);
		}
	}

	void TableLayout::Row::setMinimumHeight(sl_ui_len height, UIUpdateMode mode)
	{
		if (height < 0) {
			height = 0;
		}
		m_minHeight = height;
		_invalidateLayout(mode);
	}

	sl_bool TableLayout::isRowMaximumHeightDefined(sl_uint32 iRow)
	{
		Ref<Row> row = getRow(iRow);
		if (row.isNotNull()) {
			return row->m_flagMaxHeightDefined;
		}
		return sl_false;
	}

	sl_bool TableLayout::Row::isMaximumHeightDefined()
	{
		return m_flagMaxHeightDefined;
	}

	sl_ui_len TableLayout::getRowMaximumHeight(sl_uint32 iRow)
	{
		Ref<Row> row = getRow(iRow);
		if (row.isNotNull()) {
			return row->m_maxHeight;
		}
		return 0;
	}

	sl_ui_len TableLayout::Row::getMaximumHeight()
	{
		return m_maxHeight;
	}

	void TableLayout::setRowMaximumHeight(sl_uint32 iRow, sl_ui_len height, UIUpdateMode mode)
	{
		Ref<Row> row = getRow(iRow);
		if (row.isNotNull()) {
			row->setMaximumHeight(height, UIUpdateMode::Init);
			invalidateLayout(mode);
		}
	}

	void TableLayout::Row::setMaximumHeight(sl_ui_len height, UIUpdateMode mode)
	{
		if (height < 0) {
			height = 0;
		}
		m_maxHeight = height;
		m_flagMaxHeightDefined = sl_true;
		_invalidateLayout(mode);
	}

	sl_ui_len TableLayout::getRowMarginTop(sl_uint32 iRow)
	{
		Ref<Row> row = getRow(iRow);
		if (row.isNotNull()) {
			return row->m_marginTop;
		}
		return 0;
	}

	sl_ui_len TableLayout::Row::getMarginTop()
	{
		return m_marginTop;
	}

	void TableLayout::setRowMarginTop(sl_uint32 iRow, sl_ui_len margin, UIUpdateMode mode)
	{
		Ref<Row> row = getRow(iRow);
		if (row.isNotNull()) {
			row->setMarginTop(margin, UIUpdateMode::Init);
			invalidateLayout(mode);
		}
	}

	void TableLayout::Row::setMarginTop(sl_ui_len margin, UIUpdateMode mode)
	{
		m_marginTop = margin;
		_invalidateLayout(mode);
	}

	sl_ui_len TableLayout::getRowMarginBottom(sl_uint32 iRow)
	{
		Ref<Row> row = getRow(iRow);
		if (row.isNotNull()) {
			return row->m_marginBottom;
		}
		return 0;
	}

	sl_ui_len TableLayout::Row::getMarginBottom()
	{
		return m_marginBottom;
	}

	void TableLayout::setRowMarginBottom(sl_uint32 iRow, sl_ui_len margin, UIUpdateMode mode)
	{
		Ref<Row> row = getRow(iRow);
		if (row.isNotNull()) {
			row->setMarginBottom(margin, UIUpdateMode::Init);
			invalidateLayout(mode);
		}
	}

	void TableLayout::Row::setMarginBottom(sl_ui_len margin, UIUpdateMode mode)
	{
		m_marginBottom = margin;
		_invalidateLayout(mode);
	}

	void TableLayout::setRowMargin(sl_uint32 iRow, sl_ui_len margin, UIUpdateMode mode)
	{
		Ref<Row> row = getRow(iRow);
		if (row.isNotNull()) {
			row->setMargin(margin, UIUpdateMode::Init);
			invalidateLayout(mode);
		}
	}

	void TableLayout::Row::setMargin(sl_ui_len margin, UIUpdateMode mode)
	{
		m_marginTop = margin;
		m_marginBottom = margin;
		_invalidateLayout(mode);
	}

	sl_ui_len TableLayout::getRowPaddingTop(sl_uint32 iRow)
	{
		Ref<Row> row = getRow(iRow);
		if (row.isNotNull()) {
			return row->m_paddingTop;
		}
		return 0;
	}

	sl_ui_len TableLayout::Row::getPaddingTop()
	{
		return m_paddingTop;
	}

	void TableLayout::setRowPaddingTop(sl_uint32 iRow, sl_ui_len padding, UIUpdateMode mode)
	{
		Ref<Row> row = getRow(iRow);
		if (row.isNotNull()) {
			row->setPaddingTop(padding, UIUpdateMode::Init);
			invalidateLayout(mode);
		}
	}

	void TableLayout::Row::setPaddingTop(sl_ui_len padding, UIUpdateMode mode)
	{
		m_paddingTop = padding;
		_invalidateLayout(mode);
	}

	sl_ui_len TableLayout::getRowPaddingBottom(sl_uint32 iRow)
	{
		Ref<Row> row = getRow(iRow);
		if (row.isNotNull()) {
			return row->m_paddingBottom;
		}
		return 0;
	}

	sl_ui_len TableLayout::Row::getPaddingBottom()
	{
		return m_paddingBottom;
	}

	void TableLayout::setRowPaddingBottom(sl_uint32 iRow, sl_ui_len padding, UIUpdateMode mode)
	{
		Ref<Row> row = getRow(iRow);
		if (row.isNotNull()) {
			row->setPaddingBottom(padding, UIUpdateMode::Init);
			invalidateLayout(mode);
		}
	}

	void TableLayout::Row::setPaddingBottom(sl_ui_len padding, UIUpdateMode mode)
	{
		m_paddingBottom = padding;
		_invalidateLayout(mode);
	}

	void TableLayout::setRowPadding(sl_uint32 iRow, sl_ui_len padding, UIUpdateMode mode)
	{
		Ref<Row> row = getRow(iRow);
		if (row.isNotNull()) {
			row->setPadding(padding, UIUpdateMode::Init);
			invalidateLayout(mode);
		}
	}

	void TableLayout::Row::setPadding(sl_ui_len padding, UIUpdateMode mode)
	{
		m_paddingTop = padding;
		m_paddingBottom = padding;
		_invalidateLayout(mode);
	}

	Ref<Drawable> TableLayout::getRowBackground(sl_uint32 iRow)
	{
		Ref<Row> row = getRow(iRow);
		if (row.isNotNull()) {
			return row->m_background;
		}
		return sl_null;
	}

	Ref<Drawable> TableLayout::Row::getBackground()
	{
		return m_background;
	}

	void TableLayout::setRowBackground(sl_uint32 iRow, const Ref<Drawable>& background, UIUpdateMode mode)
	{
		Ref<Row> row = getRow(iRow);
		if (row.isNotNull()) {
			row->setBackground(background, UIUpdateMode::Init);
			invalidate(mode);
		}
	}

	void TableLayout::Row::setBackground(const Ref<Drawable>& _background, UIUpdateMode mode)
	{
		m_background = _background;
		_invalidate(mode);
	}

	void TableLayout::setRowBackgroundColor(sl_uint32 iRow, const Color& color, UIUpdateMode mode)
	{
		Ref<Row> row = getRow(iRow);
		if (row.isNotNull()) {
			row->setBackgroundColor(color, UIUpdateMode::Init);
			invalidateLayout(mode);
		}
	}

	void TableLayout::Row::setBackgroundColor(const Color& color, UIUpdateMode mode)
	{
		m_background = Drawable::fromColor(color);
		_invalidate(mode);
	}

	Alignment TableLayout::getRowAlignment(sl_uint32 iRow)
	{
		Ref<Row> row = getRow(iRow);
		if (row.isNotNull()) {
			return row->m_align;
		}
		return Alignment::Default;
	}

	Alignment TableLayout::Row::getAlignment()
	{
		return m_align;
	}

	void TableLayout::_setRowAlignment(Row* row, sl_uint32 iRow, const Alignment& align, UIUpdateMode mode)
	{
		row->m_align = align;
		ListElements<Cell> cells(row->m_cells);
		for (sl_size i = 0; i < cells.count; i++) {
			_applyCellAlign(cells.data + i, iRow, (sl_uint32)i, mode);
		}
		invalidateLayout(mode);
	}

	void TableLayout::setRowAlignment(sl_uint32 iRow, const Alignment& align, UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		Ref<Row> row = m_rows.getValueAt_NoLock(iRow);
		if (row) {
			_setRowAlignment(row.get(), iRow, align, mode);
		}
	}

	void TableLayout::Row::setAlignment(const Alignment& align, UIUpdateMode mode)
	{
		Ref<TableLayout> table = m_table;
		if (table.isNotNull()) {
			ObjectLocker lock(table.get());
			if (m_index >= 0) {
				table->_setRowAlignment(this, m_index, align, mode);
			}
		}
	}

	sl_bool TableLayout::isRowVisible(sl_uint32 iRow)
	{
		Ref<Row> row = getRow(iRow);
		if (row.isNotNull()) {
			return row->m_flagVisible;
		}
		return sl_false;
	}

	sl_bool TableLayout::Row::isVisible()
	{
		return m_flagVisible;
	}

	void TableLayout::_setRowVisible(Row* row, sl_uint32 iRow, sl_bool flagVisible, UIUpdateMode mode)
	{
		row->m_flagVisible = flagVisible;
		if (SLIB_UI_UPDATE_MODE_IS_INIT(mode)) {
			return;
		}
		ListElements<Cell> cells(row->m_cells);
		for (sl_size iCol = 0; iCol < cells.count; iCol++) {
			sl_bool f = flagVisible;
			if (f) {
				Ref<Column> col = m_columns.getValueAt_NoLock(iCol);
				if (col.isNotNull()) {
					f = col->m_flagVisible;
				}
			}
			Ref<View>& view = cells[iCol].view;
			if (view.isNotNull()) {
				view->setVisible(f, UIUpdateMode::None);
			}
		}
		invalidateLayout(mode);
	}

	void TableLayout::setRowVisible(sl_uint32 iRow, sl_bool flagVisible, UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		Ref<Row> row = m_rows.getValueAt_NoLock(iRow);
		if (row.isNotNull()) {
			_setRowVisible(row.get(), iRow, flagVisible, mode);
		}
	}

	void TableLayout::Row::setVisible(sl_bool flagVisible, UIUpdateMode mode)
	{
		Ref<TableLayout> table = m_table;
		if (table.isNotNull()) {
			ObjectLocker lock(table.get());
			if (m_index >= 0) {
				table->_setRowVisible(this, m_index, flagVisible, mode);
			}
		}
	}

	TableLayout::Cell* TableLayout::_getCell(sl_uint32 iRow, sl_uint32 iCol)
	{
		Ref<Row> row = m_rows.getValueAt_NoLock(iRow);
		if (row.isNotNull()) {
			return row->m_cells.getPointerAt(iCol);
		}
		return sl_null;
	}

	TableLayout::Cell* TableLayout::_allocCell(sl_uint32 iRow, sl_uint32 iCol)
	{
		if (iCol >= m_columns.getCount()) {
			return sl_null;
		}
		Ref<Row> row = m_rows.getValueAt_NoLock(iRow);
		if (row.isNull()) {
			return sl_null;
		}
		if (iCol < row->m_cells.getCount()) {
			return row->m_cells.getData() + iCol;
		} else {
			if (row->m_cells.setCount_NoLock(iCol + 1)) {
				return row->m_cells.getData() + iCol;
			}
		}
		return sl_null;
	}

	Alignment TableLayout::_getCellAlign(sl_uint32 iRow, sl_uint32 iCol)
	{
		Alignment ret = Alignment::Default;
		Ref<Row> row = m_rows.getValueAt_NoLock(iRow);
		if (row.isNotNull()) {
			ret = row->m_align;
		}
		Ref<Column> col = m_columns.getValueAt_NoLock(iCol);
		if (col.isNotNull()) {
			if (!(ret & Alignment::HorizontalMask)) {
				ret |= col->m_align & Alignment::HorizontalMask;
			}
			if (!(ret & Alignment::VerticalMask)) {
				ret |= col->m_align & Alignment::VerticalMask;
			}
		}
		return ret;
	}

	void TableLayout::_initCellAlign(Cell* cell, sl_uint32 iRow, sl_uint32 iCol)
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

	void TableLayout::_applyCellAlign(Cell* cell, sl_uint32 iRow, sl_uint32 iCol, UIUpdateMode mode)
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
		Ref<Column> col = m_columns.getValueAt_NoLock(iCol);
		if (col.isNull()) {
			return;
		}
		Ref<Row> row = m_rows.getValueAt_NoLock(iRow);
		if (row.isNull()) {
			return;
		}
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
			view->setVisible(col->m_flagVisible && row->m_flagVisible);
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
			invalidateLayout(mode);
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
			invalidateLayout(mode);
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
			invalidateLayout(mode);
		}
	}

	void TableLayout::onUpdateLayout()
	{
		ObjectLocker lock(this);

		sl_uint32 nRows = (sl_uint32)(m_rows.getCount());
		sl_uint32 nCols = (sl_uint32)(m_columns.getCount());
		sl_bool flagWidthWrapping = isLastWidthWrapping();
		sl_bool flagHeightWrapping = isLastHeightWrapping();
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
		UIEdgeInsets paddingContainer = getPadding();
		sl_ui_len widthContainer = layoutFrameContainer.getWidth() - paddingContainer.left - paddingContainer.right;
		sl_ui_len heightContainer = layoutFrameContainer.getHeight() - paddingContainer.top - paddingContainer.bottom;

		sl_uint32 iRow, iCol;
		Ref<Row>* rows = m_rows.getData();
		Ref<Column>* cols = m_columns.getData();

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
			Row& row = *(rows[iRow].get());
			if (!(row.m_flagVisible)) {
				rowHeightModes[iRow] = SizeMode::Fixed;
				row.m_heightLayout = 0;
				continue;
			}
			if (flagHeightWrapping) {
				if (row.m_heightMode == SizeMode::Fixed) {
					rowHeightModes[iRow] = SizeMode::Fixed;
					row.m_heightLayout = row._getFixedHeight();
					sumHeight += row.m_heightLayout + row.m_marginTop + row.m_marginBottom;
				} else {
					rowHeightModes[iRow] = SizeMode::Wrapping;
					row.m_heightLayout = 0;
					flagWrappingRows = sl_true;
				}
			} else {
				rowHeightModes[iRow] = row.m_heightMode;
				switch (row.m_heightMode) {
					case SizeMode::Fixed:
						row.m_heightLayout = row._getFixedHeight();
						sumHeight += row.m_heightLayout + row.m_marginTop + row.m_marginBottom;
						break;
					case SizeMode::Weight:
						row.m_heightLayout = row._getWeightHeight(heightContainer);
						sumHeight += row.m_heightLayout + row.m_marginTop + row.m_marginBottom;
						break;
					case SizeMode::Filling:
						sumHeight += row.m_marginTop + row.m_marginBottom;
						nFillRows++;
						sumRowFillWeights += row.m_heightWeight;
						row.m_heightLayout = 0;
						break;
					case SizeMode::Wrapping:
						row.m_heightLayout = 0;
						flagWrappingRows = sl_true;
						break;
				}
			}
		}
		for (iCol = 0; iCol < nCols; iCol++) {
			Column& col = *(cols[iCol].get());
			if (!(col.m_flagVisible)) {
				colWidthModes[iCol] = SizeMode::Fixed;
				col.m_widthLayout = 0;
				continue;
			}
			if (flagWidthWrapping) {
				if (col.m_widthMode == SizeMode::Fixed) {
					colWidthModes[iCol] = SizeMode::Fixed;
					col.m_widthLayout = col._getFixedWidth();
					sumWidth += col.m_widthLayout + col.m_marginLeft + col.m_marginRight;
				} else {
					colWidthModes[iCol] = SizeMode::Wrapping;
					col.m_widthLayout = 0;
					flagWrappingCols = sl_true;
				}
			} else {
				colWidthModes[iCol] = col.m_widthMode;
				switch (col.m_widthMode) {
					case SizeMode::Fixed:
						col.m_widthLayout = col._getFixedWidth();
						sumWidth += col.m_widthLayout + col.m_marginLeft + col.m_marginRight;
						break;
					case SizeMode::Weight:
						col.m_widthLayout = col._getWeightWidth(widthContainer);
						sumWidth += col.m_widthLayout + col.m_marginLeft + col.m_marginRight;
						break;
					case SizeMode::Filling:
						sumWidth += col.m_marginLeft + col.m_marginRight;
						nFillCols++;
						sumColFillWeights += col.m_widthWeight;
						col.m_widthLayout = 0;
						break;
					case SizeMode::Wrapping:
						col.m_widthLayout = 0;
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
				Row& row = *(rows[iRow].get());
				if (!(row.m_flagVisible)) {
					continue;
				}
				Cell* cells = row.m_cells.getData();
				sl_uint32 nCells = Math::min((sl_uint32)(row.m_cells.getCount()), nCols);
				for (iCol = 0; iCol < nCells; iCol++) {
					if (!(cols[iCol]->m_flagVisible)) {
						continue;
					}
					if (colWidthModes[iCol] == SizeMode::Wrapping) {
						Cell& cell = cells[iCol];
						Column& col = *(cols[iCol].get());
						View* view = cell.view.get();
						if (view && cell.colspan == 1) {
							SizeMode mode = view->getWidthMode();
							if (mode == SizeMode::Fixed || mode == SizeMode::Wrapping) {
								updateLayoutParam.parentContentFrame.right = col.m_widthLayout - col.m_paddingLeft - col.m_paddingRight;
								updateLayoutParam.parentContentFrame.bottom = row.m_heightLayout - row.m_paddingTop - row.m_paddingBottom;
								view->setInvalidateLayoutFrameInParent();
								view->updateLayoutFrameInParent(updateLayoutParam);
								sl_ui_len w = col._restrictWidth(view->getLayoutWidth() + view->getMarginLeft() + view->getMarginRight() + col.m_paddingLeft + col.m_paddingRight);
								if (w > col.m_widthLayout) {
									col.m_widthLayout = w;
								}
							}
						}
					}
				}
			}
			for (iCol = 0; iCol < nCols; iCol++) {
				Column& col = *(cols[iCol].get());
				if (!(col.m_flagVisible)) {
					continue;
				}
				if (colWidthModes[iCol] == SizeMode::Wrapping) {
					sumWidth += col.m_widthLayout + col.m_marginLeft + col.m_marginRight;;
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
				Column& col = *(cols[iCol].get());
				if (!(col.m_flagVisible)) {
					continue;
				}
				if (colWidthModes[iCol] == SizeMode::Filling) {
					col.m_widthLayout = col._restrictWidth((sl_ui_len)(widthRemain * col.m_widthWeight / sumColFillWeights));
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
				Row& row = *(rows[iRow].get());
				if (!(row.m_flagVisible)) {
					continue;
				}
				if (rowHeightModes[iRow] == SizeMode::Wrapping) {
					Cell* cells = row.m_cells.getData();
					sl_uint32 nCells = Math::min((sl_uint32)(row.m_cells.getCount()), nCols);
					for (iCol = 0; iCol < nCells; iCol++) {
						Cell& cell = cells[iCol];
						Column& col = *(cols[iCol].get());
						if (!(col.m_flagVisible)) {
							continue;
						}
						View* view = cell.view.get();
						if (view && cell.rowspan == 1) {
							SizeMode mode = view->getHeightMode();
							if (mode == SizeMode::Fixed || mode == SizeMode::Wrapping) {
								updateLayoutParam.parentContentFrame.right = col.m_widthLayout - col.m_paddingLeft - col.m_paddingRight;
								updateLayoutParam.parentContentFrame.bottom = row.m_heightLayout - row.m_paddingTop - row.m_paddingBottom;
								view->setInvalidateLayoutFrameInParent();
								view->updateLayoutFrameInParent(updateLayoutParam);
								sl_ui_len h = row._restrictHeight(view->getLayoutHeight() + view->getMarginTop() + view->getMarginBottom() + row.m_paddingTop + row.m_paddingBottom);
								if (h > row.m_heightLayout) {
									row.m_heightLayout = h;
								}
							}
						}
					}
				}
			}
			for (iRow = 0; iRow < nRows; iRow++) {
				Row& row = *(rows[iRow].get());
				if (!(row.m_flagVisible)) {
					continue;
				}
				if (rowHeightModes[iRow] == SizeMode::Wrapping) {
					sumHeight += row.m_heightLayout + row.m_marginTop + row.m_marginBottom;
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
				Row& row = *(rows[iRow].get());
				if (!(row.m_flagVisible)) {
					continue;
				}
				if (rowHeightModes[iRow] == SizeMode::Filling) {
					row.m_heightLayout = row._restrictHeight((sl_ui_len)(heightRemain * row.m_heightWeight / sumRowFillWeights));
				}
			}
		}

		UpdateLayoutFrameParam updateLayoutParam;
		updateLayoutParam.flagUseLayout = sl_true;
		updateLayoutParam.flagHorizontal = sl_true;
		updateLayoutParam.flagVertical = sl_true;
		sl_ui_len y = paddingContainer.top;
		for (iRow = 0; iRow < nRows; iRow++) {
			Row& row = *(rows[iRow].get());
			if (!(row.m_flagVisible)) {
				continue;
			}
			Cell* cells = row.m_cells.getData();
			sl_uint32 nCells = Math::min((sl_uint32)(row.m_cells.getCount()), nCols);
			sl_ui_len x = paddingContainer.left;
			for (iCol = 0; iCol < nCells; iCol++) {
				Cell& cell = cells[iCol];
				Column& col = *(cols[iCol].get());
				if (!(col.m_flagVisible)) {
					continue;
				}
				View* view = cell.view.get();
				if (view) {
					updateLayoutParam.parentContentFrame.left = x + col.m_marginLeft + col.m_paddingLeft;
					updateLayoutParam.parentContentFrame.top = y + row.m_marginTop + row.m_paddingTop;
					updateLayoutParam.parentContentFrame.right = x + col.m_marginLeft + col.m_widthLayout - col.m_paddingRight;
					updateLayoutParam.parentContentFrame.bottom = y + row.m_marginTop + row.m_heightLayout - row.m_paddingBottom;
					if (cell.colspan > 1) {
						for (sl_uint32 k = 1; k < cell.colspan; k++) {
							updateLayoutParam.parentContentFrame.right += cols[iCol + k - 1]->m_paddingRight + cols[iCol + k - 1]->m_marginRight;
							if (iCol + k >= nCols) {
								break;
							}
							updateLayoutParam.parentContentFrame.right += cols[iCol + k]->m_marginLeft + cols[iCol + k]->m_widthLayout - cols[iCol + k]->m_paddingRight;
						}
					}
					if (cell.rowspan > 1) {
						for (sl_uint32 k = 1; k < cell.rowspan; k++) {
							updateLayoutParam.parentContentFrame.bottom += rows[iRow + k - 1]->m_paddingBottom + rows[iRow + k - 1]->m_marginBottom;
							if (iRow + k >= nRows) {
								break;
							}
							updateLayoutParam.parentContentFrame.bottom += rows[iRow + k]->m_marginTop + rows[iRow + k]->m_heightLayout - rows[iRow + k]->m_paddingBottom;
						}
					}
					view->setInvalidateLayoutFrameInParent();
					view->updateLayoutFrameInParent(updateLayoutParam);
				}
				x += col.m_widthLayout + col.m_marginLeft + col.m_marginRight;
			}
			y += row.m_heightLayout + row.m_marginTop + row.m_marginBottom;
		}
		if (flagWidthWrapping) {
			sl_ui_len x = paddingContainer.left;
			for (iCol = 0; iCol < nCols; iCol++) {
				Column& col = *(cols[iCol].get());
				if (!(col.m_flagVisible)) {
					continue;
				}
				x += col.m_widthLayout + col.m_marginLeft + col.m_marginRight;
			}
			setLayoutWidth(x + paddingContainer.right);
		}
		if (flagHeightWrapping) {
			setLayoutHeight(y + paddingContainer.bottom);
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
			Ref<Row>* rows = m_rows.getData();
			sl_uint32 nRows = (sl_uint32)(m_rows.getCount());
			for (sl_uint32 iRow = 0; iRow < nRows; iRow++) {
				Row& row = *(rows[iRow].get());
				rc.top += row.m_marginTop;
				rc.bottom = rc.top + row.m_heightLayout;
				if (row.m_background.isNotNull()) {
					canvas->draw(rc, row.m_background);
				}
				rc.top = rc.bottom + row.m_marginBottom;
			}
		}
		{
			UIRect rc;
			rc.top = paddingContainerTop;
			rc.bottom = rc.top + layoutFrameContainer.getHeight() - paddingContainerTop - getPaddingBottom();
			rc.left = paddingContainerLeft;
			Ref<Column>* cols = m_columns.getData();
			sl_uint32 nCols = (sl_uint32)(m_columns.getCount());
			for (sl_uint32 iCol = 0; iCol < nCols; iCol++) {
				Column& col = *(cols[iCol].get());
				rc.left += col.m_marginLeft;
				rc.right = rc.left + col.m_widthLayout;
				if (col.m_background.isNotNull()) {
					canvas->draw(rc, col.m_background);
				}
				rc.left = rc.right + col.m_marginRight;
			}
		}
	}

}
