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

#include "slib/ui/grid_view.h"

#include "slib/ui/priv/view_state_map.h"
#include "slib/graphics/canvas.h"
#include "slib/core/safe_static.h"

#define DEFAULT_TEXT_COLOR Color::Black
#define MAX_RECORDS_PER_SCREEN 1000

namespace slib
{

	SLIB_DEFINE_NESTED_CLASS_DEFAULT_MEMBERS(GridView, DrawCellParam)

	GridView::DrawCellParam::DrawCellParam()
	{
	}

	SLIB_DEFINE_NESTED_CLASS_DEFAULT_MEMBERS(GridView, CellAttribute)

	GridView::CellAttribute::CellAttribute(): multiLineMode(MultiLineMode::Single), ellipsizeMode(EllipsizeMode::None), lineCount(0), align(Alignment::MiddleCenter), colspan(1), rowspan(1), width(0), height(0)
	{
		textColors.defaultValue = Color::Black;
	}

	SLIB_DEFINE_NESTED_CLASS_DEFAULT_MEMBERS(GridView, CellParam)

	GridView::CellParam::CellParam(): view(sl_null), record(0), row(0), column(0)
	{
	}

	GridView::CellParam::CellParam(const CellAttribute& attr): CellAttribute(attr), view(sl_null), row(0), column(0), record(0)
	{
	}

	GridView::Cell::Cell()
	{
	}

	GridView::Cell::~Cell()
	{
	}

	void GridView::Cell::onInit()
	{
	}

	void GridView::Cell::onDraw(Canvas* canvas, DrawParam& param)
	{
	}

	void GridView::Cell::onEvent(UIEvent* ev)
	{
	}

	GridView::TextCell::TextCell()
	{
	}

	GridView::TextCell::~TextCell()
	{
	}

	const GridView::CellCreator& GridView::TextCell::creator()
	{
		SLIB_SAFE_LOCAL_STATIC(CellCreator, ret, [](CellParam&) {
			return new TextCell;
		})
		return ret;
	}

	void GridView::TextCell::onInit()
	{
		TextBoxParam tp;
		onPrepareTextBox(tp);
		m_textBox.update(tp);
	}

	void GridView::TextCell::onDraw(Canvas* canvas, DrawParam& param)
	{
		m_textBox.draw(canvas, param);
	}

	void GridView::TextCell::onPrepareTextBox(TextBoxParam& param)
	{
		if (text.isNotNull()) {
			if (record >= 0 && recordData.isNotUndefined()) {
				param.text = String::format(text, recordData);
			} else {
				param.text = text;
			}
		}
		if (font.isNotNull()) {
			param.font = font;
		} else {
			param.font = view->getFont();
		}
		param.align = align;
		param.multiLineMode = multiLineMode;
		param.ellipsizeMode = ellipsizeMode;
		param.lineCount = lineCount;
	}

	GridView::HyperTextCell::HyperTextCell()
	{
	}

	GridView::HyperTextCell::~HyperTextCell()
	{
	}

	const GridView::CellCreator& GridView::HyperTextCell::creator()
	{
		SLIB_SAFE_LOCAL_STATIC(CellCreator, ret, [](CellParam&) {
			return new HyperTextCell;
		})
		return ret;
	}

	void GridView::HyperTextCell::onPrepareTextBox(TextBoxParam& param)
	{
		TextCell::onPrepareTextBox(param);
		param.flagHyperText = sl_true;
	}

	GridView::NumeroCell::NumeroCell(sl_int64 start): m_start(start)
	{
	}

	GridView::NumeroCell::~NumeroCell()
	{
	}
	
	const GridView::CellCreator& GridView::NumeroCell::creator()
	{
		SLIB_SAFE_LOCAL_STATIC(CellCreator, ret, [](CellParam&) {
			return new NumeroCell;
		})
		return ret;
	}

	GridView::CellCreator GridView::NumeroCell::creator(sl_int64 start)
	{
		return [start](CellParam&) {
			return new NumeroCell(start);
		};
	}

	void GridView::NumeroCell::onPrepareTextBox(TextBoxParam& param)
	{
		TextCell::onPrepareTextBox(param);
		if (record >= 0) {
			param.text = String::fromInt64(m_start + record);
		}
	}


	SLIB_DEFINE_NESTED_CLASS_DEFAULT_MEMBERS(GridView, CellProp)

	GridView::CellProp::CellProp(): flagCoveredX(sl_false), flagCoveredY(sl_false)
	{
	}

	void GridView::CellProp::inheritFrom(const CellProp& other)
	{
		creator = other.creator;
		font = other.font;
		multiLineMode = other.multiLineMode;
		ellipsizeMode = other.ellipsizeMode;
		lineCount = other.lineCount;
		align = other.align;
		backgrounds.copyFrom(other.backgrounds);
		textColors.copyFrom(other.textColors);
	}

	SLIB_DEFINE_NESTED_CLASS_DEFAULT_MEMBERS(GridView, FixedCellProp)

	GridView::FixedCellProp::FixedCellProp(): flagMadeCell(sl_false)
	{
	}
	
	SLIB_DEFINE_NESTED_CLASS_DEFAULT_MEMBERS(GridView, Column)

	GridView::Column::Column(): width(0), fixedWidth(0)
	{
	}
	
	SLIB_DEFINE_NESTED_CLASS_DEFAULT_MEMBERS(GridView, Row)

	GridView::Row::Row(): height(-1), fixedHeight(0)
	{
	}

	SLIB_DEFINE_NESTED_CLASS_DEFAULT_MEMBERS(GridView, Selection)

	GridView::Selection::Selection(): record(GridView::OUTSIDE), row(-1), column(-1)
	{
	}

	sl_bool GridView::Selection::operator==(const Selection& other) const
	{
		return row == other.row && column == other.column && record == other.record;
	}

	sl_bool GridView::Selection::match(RecordIndex _record, sl_int32 _row, sl_int32 _column) const
	{
		if (record == OUTSIDE && row < 0 && column < 0) {
			return sl_false;
		}
		if (record != OUTSIDE && _record != OUTSIDE && record != _record) {
			return sl_false;
		}
		if (row >= 0 && _row >= 0 && row != _row) {
			return sl_false;
		}
		if (column >= 0 && _column >=0 && column != _column) {
			return sl_false;
		}
		return sl_true;
	}

	sl_bool GridView::Selection::match(Cell* cell) const
	{
		return match(cell->record, cell->row, cell->column);
	}


	SLIB_DEFINE_OBJECT(GridView, View)

	GridView::GridView()
	{
		m_nRecords = 0;
		m_nLeftColumns = 0;
		m_nRightColumns = 0;

		m_selectionMode = SelectionMode::Cell;

		m_defaultColumnWidth = 80;
		m_defaultBodyRowHeight = -1;
		m_defaultHeaderRowHeight = -1;
		m_defaultFooterRowHeight = -1;

		m_defaultBodyProps.creator = TextCell::creator();
		m_defaultHeaderProps.creator = TextCell::creator();
		m_defaultHeaderProps.backgrounds.defaultValue = Drawable::fromColor(Color(230, 230, 230));
		m_defaultFooterProps.creator = TextCell::creator();

		m_flagInvalidateBodyRows = sl_true;
		m_flagInvalidateHeaderRows = sl_true;
		m_flagInvalidateFooterRows = sl_true;

		m_flagInitialize = sl_true;
	}

	void GridView::init()
	{
		View::init();

		setCanvasScrolling(sl_false);
		setVerticalScrolling(sl_true, UIUpdateMode::Init);
		setHorizontalScrolling(sl_true, UIUpdateMode::Init);
		setContentScrollingByMouse(sl_false);
		setFocusable();
		setUsingFont();
		setRedrawingOnChangeState();
		setClipping(sl_true, UIUpdateMode::Init);
		setBorder(sl_true, UIUpdateMode::Init);
		setBackgroundColor(Color::White, UIUpdateMode::Init);
	}

	GridView::~GridView()
	{
	}

	sl_uint32 GridView::getColumnCount()
	{
		return (sl_uint32)(m_columns.getCount());
	}

	sl_bool GridView::setColumnCount(sl_uint32 count, UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		sl_uint32 nOldCount = (sl_uint32)(m_columns.getCount());
		if (nOldCount == count) {
			return sl_true;
		}
		if (!(m_columns.setCount_NoLock(count))) {
			return sl_false;
		}
		if (count > nOldCount) {
			Column* columns = m_columns.getData();
			for (sl_uint32 iCol = nOldCount; iCol < count; iCol++) {
				Column& col = columns[iCol];
				col.width = m_defaultColumnWidth;

#define SET_COLUMN_COUNT_INHERIT(SECTION) \
				col.default##SECTION##Props.inheritFrom(m_default##SECTION##Props); \
				{ \
					sl_uint32 nRows = (sl_uint32)(m_list##SECTION##Row.getCount()); \
					if (nRows) { \
						if (!(col.list##SECTION##Cell.setCount_NoLock(nRows))) { \
							return sl_false; \
						} \
						if (nOldCount) { \
							ListElements<Row> rows(m_list##SECTION##Row); \
							if (nRows > rows.count) { \
								nRows = (sl_uint32)(rows.count); \
							} \
							SECTION##CellProp* props = col.list##SECTION##Cell.getData(); \
							for (sl_uint32 i = 0; i < nRows; i++) { \
								props[i].inheritFrom(rows[i].defaultProps); \
							} \
						} \
					} \
				}

				SET_COLUMN_COUNT_INHERIT(Body)
				SET_COLUMN_COUNT_INHERIT(Header)
				SET_COLUMN_COUNT_INHERIT(Footer)
			}
		}
		if (SLIB_UI_UPDATE_MODE_IS_INIT(mode)) {
			return sl_true;
		}
		_invalidateHeaderCells();
		_invalidateFooterCells();
		_invalidateBodyCells();
		refreshContentWidth(mode);
		return sl_true;
	}

	sl_uint32 GridView::getLeftColumnCount()
	{
		return m_nLeftColumns;
	}

	void GridView::setLeftColumnCount(sl_uint32 count, UIUpdateMode mode)
	{
		m_nLeftColumns = count;
		invalidate(mode);
	}

	sl_uint32 GridView::getRightColumnCount()
	{
		return m_nRightColumns;
	}

	void GridView::setRightColumnCount(sl_uint32 count, UIUpdateMode mode)
	{
		m_nRightColumns = count;
		invalidate(mode);
	}

	sl_uint64 GridView::getRecordCount()
	{
		return m_nRecords;
	}

	void GridView::setRecordCount(sl_uint64 count, UIUpdateMode mode)
	{
		if (m_nRecords == count) {
			return;
		}
		m_nRecords = count;
		refreshContentHeight(mode);
	}

	sl_uint32 GridView::getBodyRowCount()
	{
		return (sl_uint32)(m_listBodyRow.getCount());
	}

	sl_uint32 GridView::getHeaderRowCount()
	{
		return (sl_uint32)(m_listHeaderRow.getCount());
	}

	sl_uint32 GridView::getFooterRowCount()
	{
		return (sl_uint32)(m_listFooterRow.getCount());
	}

#define DEFINE_SET_ROW_COUNT(SECTION, ADDTIONAL) \
	sl_bool GridView::set##SECTION##RowCount(sl_uint32 count, UIUpdateMode mode) \
	{ \
		ObjectLocker lock(this); \
		sl_uint32 nOld = (sl_uint32)(m_list##SECTION##Row.getCount()); \
		if (nOld == count) { \
			return sl_true; \
		} \
		ListElements<Column> columns(m_columns); \
		for (sl_size iCol = 0; iCol < columns.count; iCol++) { \
			Column& col = columns[iCol]; \
			nOld = (sl_uint32)(col.list##SECTION##Cell.getCount()); \
			if (!(col.list##SECTION##Cell.setCount_NoLock(count))) { \
				return sl_false; \
			} \
			if (nOld < count) { \
				SECTION##CellProp* props = col.list##SECTION##Cell.getData(); \
				for (sl_uint32 i = nOld; i < count; i++) { \
					props[i].inheritFrom(col.default##SECTION##Props); \
				} \
			} \
		} \
		if (!(m_list##SECTION##Row.setCount_NoLock(count))) { \
			return sl_false; \
		} \
		if (nOld < count) { \
			Row* rows = m_list##SECTION##Row.getData(); \
			for (sl_uint32 i = nOld; i < count; i++) { \
				Row& row = rows[i]; \
				row.height = m_default##SECTION##RowHeight; \
				row.defaultProps.inheritFrom(m_default##SECTION##Props); \
			} \
		} \
		if (SLIB_UI_UPDATE_MODE_IS_INIT(mode)) { \
			return sl_true; \
		} \
		ADDTIONAL \
		refreshContentHeight(mode); \
		return sl_true; \
	}

	DEFINE_SET_ROW_COUNT(Body, _invalidateBodyCells();)
	DEFINE_SET_ROW_COUNT(Header,)
	DEFINE_SET_ROW_COUNT(Footer, )

	sl_ui_len GridView::getColumnWidth(sl_uint32 index)
	{
		ObjectLocker lock(this);
		Column* col = m_columns.getPointerAt(index);
		if (col) {
			return col->width;
		}
		return 0;
	}

	sl_bool GridView::setColumnWidth(sl_uint32 index, sl_ui_len width, UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		Column* col = m_columns.getPointerAt(index);
		if (col) {
			col->width = width;
			if (SLIB_UI_UPDATE_MODE_IS_INIT(mode)) {
				return sl_true;
			}
			_invalidateHeaderCells(*col, index);
			_invalidateFooterCells(*col, index);
			_invalidateBodyCells(*col, index);
			refreshContentWidth(mode);
			return sl_true;
		} else {
			return sl_false;
		}
	}

	void GridView::setColumnWidth(sl_ui_len width, UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		ListElements<Column> columns(m_columns);
		for (sl_size i = 0; i < columns.count; i++) {
			columns[i].width = width;
		}
		m_defaultColumnWidth = width;
		if (SLIB_UI_UPDATE_MODE_IS_INIT(mode)) {
			return;
		}
		_invalidateHeaderCells();
		_invalidateFooterCells();
		_invalidateBodyCells();
		refreshContentWidth(mode);
	}

	namespace {
		SLIB_INLINE static sl_ui_len FixLength(sl_ui_len value, sl_ui_len defaultRowHeight)
		{
			if (value >= 0) {
				return value;
			} else {
				return defaultRowHeight;
			}
		}
	}

	SLIB_INLINE sl_ui_len GridView::_getDefaultRowHeight()
	{
		Ref<Font> font = getFont();
		if (font.isNotNull()) {
			return (sl_ui_len)(font->getSize() * 1.5f);
		} else {
			return 0;
		}
	}

#define DEFINE_GET_SECTION_HEIGHT(PREFIX, SECTION) \
	sl_ui_len GridView::get##PREFIX##Height() \
	{ \
		sl_ui_len height = 0; \
		ObjectLocker lock(this); \
		ListElements<Row> rows(m_list##SECTION##Row); \
		sl_ui_len defaultRowHeight = _getDefaultRowHeight(); \
		for (sl_size i = 0; i < rows.count; i++) { \
			height += FixLength(rows[i].height, defaultRowHeight); \
		} \
		return height; \
	}

	DEFINE_GET_SECTION_HEIGHT(Record, Body)
	DEFINE_GET_SECTION_HEIGHT(Header, Header)
	DEFINE_GET_SECTION_HEIGHT(Footer, Footer)

	sl_uint64 GridView::getBodyHeight()
	{
		return (sl_uint64)(getRecordHeight()) * m_nRecords;
	}

#define DEFINE_GET_SET_ROW_HEIGHT(SECTION) \
	sl_ui_len GridView::get##SECTION##RowHeight(sl_uint32 index) \
	{ \
		ObjectLocker lock(this); \
		Row* row = m_list##SECTION##Row.getPointerAt(index); \
		if (row) { \
			return FixLength(row->height, _getDefaultRowHeight()); \
		} else { \
			return 0; \
		} \
	} \
	void GridView::set##SECTION##RowHeight(sl_uint32 index, sl_ui_len height, UIUpdateMode mode) \
	{ \
		ObjectLocker lock(this); \
		Row* row = m_list##SECTION##Row.getPointerAt(index); \
		if (row) { \
			row->height = height; \
		} \
		if (SLIB_UI_UPDATE_MODE_IS_INIT(mode)) { \
			return; \
		} \
		_invalidate##SECTION##Cells(); \
		refreshContentHeight(mode); \
	}

	DEFINE_GET_SET_ROW_HEIGHT(Body)
	DEFINE_GET_SET_ROW_HEIGHT(Header)
	DEFINE_GET_SET_ROW_HEIGHT(Footer)

#define DEFINE_SET_ALL_ROW_HEIGHT(SECTION) \
	void GridView::set##SECTION##RowHeight(sl_ui_len height, UIUpdateMode mode) \
	{ \
		ObjectLocker lock(this); \
		ListElements<Row> rows(m_list##SECTION##Row); \
		for (sl_size i = 0; i < rows.count; i++) { \
			rows[i].height = height; \
		} \
		m_default##SECTION##RowHeight = height; \
		if (SLIB_UI_UPDATE_MODE_IS_INIT(mode)) { \
			return; \
		} \
		_invalidate##SECTION##Cells(); \
		refreshContentHeight(mode); \
	}

	DEFINE_SET_ALL_ROW_HEIGHT(Body)
	DEFINE_SET_ALL_ROW_HEIGHT(Header)
	DEFINE_SET_ALL_ROW_HEIGHT(Footer)

	void GridView::setRowHeight(sl_ui_len height, UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		{
			m_defaultBodyRowHeight = height;
			ListElements<Row> rows(m_listBodyRow);
			for (sl_size i = 0; i < rows.count; i++) {
				rows[i].height = height;
			}
		}
		{
			m_defaultHeaderRowHeight = height;
			ListElements<Row> rows(m_listHeaderRow);
			for (sl_size i = 0; i < rows.count; i++) {
				rows[i].height = height; \
			}
		}
		{
			m_defaultFooterRowHeight = height;
			ListElements<Row> rows(m_listFooterRow);
			for (sl_size i = 0; i < rows.count; i++) {
				rows[i].height = height; \
			}
		}
		if (SLIB_UI_UPDATE_MODE_IS_INIT(mode)) {
			return;
		}
		_invalidateBodyCells();
		_invalidateHeaderCells();
		_invalidateFooterCells();
		refreshContentHeight(mode);
	}

	namespace {
		SLIB_INLINE static void DissolveSectionValue(unsigned int section, int& v, int& h)
		{
			h = (int)(section & 3) - 1;
			v = (int)((section >> 2) & 3) - 1;
		}
	}

#define DEFINE_GET_SET_GRID(SECTION, DEFAULT) \
	Ref<Pen> GridView::get##SECTION##Grid() \
	{ \
		Ref<Pen> grid = m_grid##SECTION; \
		if (grid.isNotNull()) { \
			return grid; \
		} \
		return DEFAULT; \
	} \
	void GridView::set##SECTION##Grid(const Ref<Pen>& pen, UIUpdateMode mode) \
	{ \
		m_grid##SECTION = pen; \
		invalidate(mode); \
	} \
	void GridView::set##SECTION##Grid(const PenDesc& desc, UIUpdateMode mode) \
	{ \
		set##SECTION##Grid(Pen::create(desc, get##SECTION##Grid()), mode); \
	}

	DEFINE_GET_SET_GRID(Body, getBorder())
	DEFINE_GET_SET_GRID(Header, getBorder())
	DEFINE_GET_SET_GRID(Footer, getBorder())
	DEFINE_GET_SET_GRID(Left, getBodyGrid())
	DEFINE_GET_SET_GRID(Right, getBodyGrid())

	void GridView::setGrid(const Ref<Pen>& pen, UIUpdateMode mode)
	{
		m_gridBody = pen;
		m_gridHeader = pen;
		m_gridFooter = pen;
		m_gridLeft = pen;
		m_gridRight = pen;
		invalidate(mode);
	}

	void GridView::setGrid(const PenDesc& desc, UIUpdateMode mode)
	{
		m_gridBody = Pen::create(desc, getBodyGrid());
		m_gridHeader = Pen::create(desc, getHeaderGrid());
		m_gridFooter = Pen::create(desc, getFooterGrid());
		m_gridLeft = Pen::create(desc, getLeftGrid());
		m_gridRight = Pen::create(desc, getRightGrid());
		invalidate(mode);
	}

	void GridView::refreshContentWidth(UIUpdateMode mode)
	{
		if (SLIB_UI_UPDATE_MODE_IS_INIT(mode)) {
			m_flagInitialize = sl_true;
			return;
		}
		ListElements<Column> columns(m_columns);
		sl_ui_len width = getWidth();
		sl_ui_len content = 0;
		sl_ui_len fixed = 0;
		for (sl_size i = 0; i < columns.count; i++) {
			sl_ui_len w = columns[i].width;
			if (i < m_nLeftColumns || i >= columns.count - m_nRightColumns) {
				fixed += w;
			} else {
				content += w;
			}
		}
		if (fixed < width) {
			setPageWidth((sl_scroll_pos)(width - fixed), SLIB_UI_UPDATE_MODE_IS_INIT(mode) ? UIUpdateMode::Init : UIUpdateMode::None);
			setContentWidth((sl_scroll_pos)content, mode);
		} else {
			setPageWidth(0, SLIB_UI_UPDATE_MODE_IS_INIT(mode) ? UIUpdateMode::Init : UIUpdateMode::None);
			setContentWidth(0, mode);
		}
	}

	void GridView::refreshContentHeight(UIUpdateMode mode)
	{
		if (SLIB_UI_UPDATE_MODE_IS_INIT(mode)) {
			m_flagInitialize = sl_true;
			return;
		}
		sl_ui_len height = getHeight();
		sl_ui_len fixed = getHeaderHeight() + getFooterHeight();
		if (fixed < height) {
			sl_int64 content = getRecordHeight() * (sl_int64)m_nRecords;
			setPageHeight((sl_scroll_pos)(height - fixed), SLIB_UI_UPDATE_MODE_IS_INIT(mode) ? UIUpdateMode::Init : UIUpdateMode::None);
			setContentHeight((sl_scroll_pos)content, mode);
		} else {
			setPageHeight(0, SLIB_UI_UPDATE_MODE_IS_INIT(mode) ? UIUpdateMode::Init : UIUpdateMode::None);
			setContentHeight(0, mode);
		}
	}

	GridView::DataFunction GridView::getDataFunction()
	{
		return m_recordData;
	}

	void GridView::setDataFunction(const DataFunction& func, UIUpdateMode mode)
	{
		m_recordData = func;
		ObjectLocker lock(this);
		_invalidateBodyCells();
		invalidate(mode);
	}

	GridView::CellProp* GridView::_getCellProp(RecordIndex iRecord, sl_uint32 iRow, sl_uint32 iCol)
	{
		Column* col = m_columns.getPointerAt(iCol);
		if (col) {
			if (iRecord == HEADER) {
				return col->listHeaderCell.getPointerAt(iRow);
			} else if (iRecord == FOOTER) {
				return col->listFooterCell.getPointerAt(iRow);
			} else {
				return col->listBodyCell.getPointerAt(iRow);
			}
		}
		return sl_null;
	}

#define DEFINE_GET_CELL_PROP(SECTION) \
	GridView::SECTION##CellProp* GridView::_get##SECTION##CellProp(sl_uint32 iRow, sl_uint32 iCol) \
	{ \
		Column* column = m_columns.getPointerAt(iCol); \
		if (column) { \
			return column->list##SECTION##Cell.getPointerAt(iRow); \
		} \
		return sl_null; \
	}

	DEFINE_GET_CELL_PROP(Body)
	DEFINE_GET_CELL_PROP(Header)
	DEFINE_GET_CELL_PROP(Footer)

#define DEFINE_SET_CELL_ATTR_SUB(SECTION, FUNC, RET, ARG, NAME, DEF) \
	void GridView::set##SECTION##FUNC(sl_int32 iRow, sl_int32 iCol, ARG NAME, UIUpdateMode mode) \
	{ \
		ObjectLocker lock(this); \
		if (iCol >= 0) { \
			if (iRow >= 0) { \
				SECTION##CellProp* prop = _get##SECTION##CellProp(iRow, iCol); \
				if (prop) { \
					prop->NAME = NAME; \
					if (SLIB_UI_UPDATE_MODE_IS_INIT(mode)) { \
						return; \
					} \
					_invalidate##SECTION##Cell(*prop, iRow, iCol); \
					invalidate(mode); \
				} \
			} else { \
				Column* col = m_columns.getPointerAt(iCol); \
				if (col) { \
					col->default##SECTION##Props.NAME = NAME; \
					ListElements<SECTION##CellProp> props(col->list##SECTION##Cell); \
					for (sl_size i = 0; i < props.count; i++) { \
						props[i].NAME = NAME; \
					} \
					if (SLIB_UI_UPDATE_MODE_IS_INIT(mode)) { \
						return; \
					} \
					_invalidate##SECTION##Cells(*col, iCol); \
					invalidate(mode); \
				} \
			} \
		} else { \
			if (iRow >= 0) { \
				Row* row = m_list##SECTION##Row.getPointerAt(iRow); \
				if (row) { \
					row->defaultProps.NAME = NAME; \
				} \
			} else { \
				m_default##SECTION##Props.NAME = NAME; \
				ListElements<Row> rows(m_list##SECTION##Row); \
				for (sl_size k = 0; k < rows.count; k++) { \
					rows[k].defaultProps.NAME = NAME; \
				} \
			} \
			ListElements<Column> columns(m_columns); \
			for (sl_size k = 0; k < columns.count; k++) { \
				Column& col = columns[k]; \
				if (iRow >= 0) { \
					SECTION##CellProp* prop = col.list##SECTION##Cell.getPointerAt(iRow); \
					if (prop) { \
						prop->NAME = NAME; \
					} \
				} else { \
					col.default##SECTION##Props.NAME = NAME; \
					ListElements<SECTION##CellProp> props(col.list##SECTION##Cell); \
					for (sl_size i = 0; i < props.count; i++) { \
						props[i].NAME = NAME; \
					} \
				} \
			} \
			if (SLIB_UI_UPDATE_MODE_IS_INIT(mode)) { \
				return; \
			} \
			_invalidate##SECTION##Cells(); \
			invalidate(mode); \
		} \
	}

#define DEFINE_GET_SET_CELL_ATTR_SUB(SECTION, FUNC, RET, ARG, NAME, DEF) \
	RET GridView::get##SECTION##FUNC(sl_uint32 iRow, sl_uint32 iCol) \
	{ \
		ObjectLocker lock(this); \
		SECTION##CellProp* prop = _get##SECTION##CellProp(iRow, iCol); \
		if (prop) { \
			return prop->NAME; \
		} \
		return DEF; \
	} \
	DEFINE_SET_CELL_ATTR_SUB(SECTION, FUNC, RET, ARG, NAME, DEF)

#define DEFINE_SET_COLUMN_ATTR_SUB(...) \
	{ \
		col.defaultBodyProps.__VA_ARGS__; \
		ListElements<BodyCellProp> props(col.listBodyCell); \
		for (sl_size i = 0; i < props.count; i++) { \
			props[i].__VA_ARGS__; \
		} \
	} \
	{ \
		col.defaultHeaderProps.__VA_ARGS__; \
		ListElements<HeaderCellProp> props(col.listHeaderCell); \
		for (sl_size i = 0; i < props.count; i++) { \
			props[i].__VA_ARGS__; \
		} \
	} \
	{ \
		col.defaultFooterProps.__VA_ARGS__; \
		ListElements<FooterCellProp> props(col.listFooterCell); \
		for (sl_size i = 0; i < props.count; i++) { \
			props[i].__VA_ARGS__; \
		} \
	}

#define DEFINE_SET_COLUMN_ATTR(FUNC, ARG, NAME) \
	void GridView::setCell##FUNC(sl_int32 iCol, ARG NAME, UIUpdateMode mode) \
	{ \
		ObjectLocker lock(this); \
		if (iCol >= 0) { \
			Column* pCol = m_columns.getPointerAt(iCol); \
			if (pCol) { \
				Column& col = *pCol; \
				DEFINE_SET_COLUMN_ATTR_SUB(NAME = NAME) \
				if (SLIB_UI_UPDATE_MODE_IS_INIT(mode)) { \
					return; \
				} \
				_invalidateBodyCells(col, iCol); \
				_invalidateHeaderCells(col, iCol); \
				_invalidateFooterCells(col, iCol); \
				invalidate(mode); \
			} \
		} else { \
			{ \
				ListElements<Column> columns(m_columns); \
				for (sl_size k = 0; k < columns.count; k++) { \
					Column& col = columns[k]; \
					DEFINE_SET_COLUMN_ATTR_SUB(NAME = NAME) \
				} \
			} \
			{ \
				m_defaultBodyProps.NAME = NAME; \
				ListElements<Row> rows(m_listBodyRow); \
				for (sl_size k = 0; k < rows.count; k++) { \
					rows[k].defaultProps.NAME = NAME; \
				} \
			} \
			{ \
				m_defaultHeaderProps.NAME = NAME; \
				ListElements<Row> rows(m_listHeaderRow); \
				for (sl_size k = 0; k < rows.count; k++) { \
					rows[k].defaultProps.NAME = NAME; \
				} \
			} \
			{ \
				m_defaultFooterProps.NAME = NAME; \
				ListElements<Row> rows(m_listFooterRow); \
				for (sl_size k = 0; k < rows.count; k++) { \
					rows[k].defaultProps.NAME = NAME; \
				} \
			} \
			if (SLIB_UI_UPDATE_MODE_IS_INIT(mode)) { \
				return; \
			} \
			_invalidateColumns(); \
			invalidate(mode); \
		} \
	}

#define DEFINE_SET_CELL_ATTR(FUNC, RET, ARG, NAME, DEF) \
	DEFINE_SET_CELL_ATTR_SUB(Body, FUNC, RET, ARG, NAME, DEF) \
	DEFINE_SET_CELL_ATTR_SUB(Header, FUNC, RET, ARG, NAME, DEF) \
	DEFINE_SET_CELL_ATTR_SUB(Footer, FUNC, RET, ARG, NAME, DEF) \
	DEFINE_SET_COLUMN_ATTR(FUNC, ARG, NAME)

#define DEFINE_GET_SET_CELL_ATTR(FUNC, RET, ARG, NAME, DEF) \
	DEFINE_GET_SET_CELL_ATTR_SUB(Body, FUNC, RET, ARG, NAME, DEF) \
	DEFINE_GET_SET_CELL_ATTR_SUB(Header, FUNC, RET, ARG, NAME, DEF) \
	DEFINE_GET_SET_CELL_ATTR_SUB(Footer, FUNC, RET, ARG, NAME, DEF) \
	DEFINE_SET_COLUMN_ATTR(FUNC, ARG, NAME)

	DEFINE_GET_SET_CELL_ATTR(Creator, GridView::CellCreator, const CellCreator&, creator, sl_null)

	DEFINE_GET_SET_CELL_ATTR(Text, String, const String&, text, sl_null)

	DEFINE_SET_CELL_ATTR(Font, Ref<Font>, const Ref<Font>&, font, sl_null)

#define DEFINE_GET_SET_FONT(SECTION) \
	Ref<Font> GridView::get##SECTION##Font(sl_uint32 iRow, sl_uint32 iCol) \
	{ \
		ObjectLocker lock(this); \
		SECTION##CellProp* prop = _get##SECTION##CellProp(iRow, iCol); \
		if (prop) { \
			return prop->font; \
		} \
		return getFont(); \
	} \
	void GridView::set##SECTION##Font(sl_int32 iRow, sl_int32 iCol, const FontDesc& desc, UIUpdateMode mode) \
	{ \
		set##SECTION##Font(iRow, iCol, Font::create(desc, get##SECTION##Font(iRow, iCol)), mode); \
	}

	DEFINE_GET_SET_FONT(Body)
	DEFINE_GET_SET_FONT(Header)
	DEFINE_GET_SET_FONT(Footer)

	void GridView::setCellFont(sl_int32 iCol, const FontDesc& desc, UIUpdateMode mode)
	{
		setCellFont(iCol, Font::create(desc, getFont()), mode);
	}

	DEFINE_GET_SET_CELL_ATTR(MultiLine, MultiLineMode, MultiLineMode, multiLineMode, MultiLineMode::Single)

	void GridView::setCellMultiLine(MultiLineMode multiLineMode, UIUpdateMode updateMode)
	{
		setCellMultiLine(-1, multiLineMode, updateMode);
	}

	DEFINE_GET_SET_CELL_ATTR(Ellipsize, EllipsizeMode, EllipsizeMode, ellipsizeMode, EllipsizeMode::None)

	void GridView::setCellEllipsize(EllipsizeMode ellipsizeMode, UIUpdateMode updateMode)
	{
		setCellEllipsize(-1, ellipsizeMode, updateMode);
	}

	DEFINE_GET_SET_CELL_ATTR(LineCount, sl_uint32, sl_uint32, lineCount, 0)

	void GridView::setCellLineCount(sl_uint32 lineCount, UIUpdateMode mode)
	{
		setCellLineCount(-1, lineCount, mode);
	}

	DEFINE_GET_SET_CELL_ATTR(Alignment, Alignment, const Alignment&, align, 0)

	void GridView::setCellAlignment(const Alignment& align, UIUpdateMode mode)
	{
		setCellAlignment(-1, align, mode);
	}

#define DEFINE_GET_SET_CELL_DRAWING_ATTR_SUB(SECTION, FUNC, RET, ARG, NAME, DEF) \
	RET GridView::get##SECTION##FUNC(sl_uint32 iRow, sl_uint32 iCol, ViewState state) \
	{ \
		ObjectLocker lock(this); \
		SECTION##CellProp* prop = _get##SECTION##CellProp(iRow, iCol); \
		if (prop) { \
			return prop->NAME##s.get(state); \
		} \
		return DEF; \
	} \
	void GridView::set##SECTION##FUNC(sl_int32 iRow, sl_int32 iCol, ARG NAME, ViewState state, UIUpdateMode mode) \
	{ \
		ObjectLocker lock(this); \
		if (iCol >= 0) { \
			if (iRow >= 0) { \
				SECTION##CellProp* prop = _get##SECTION##CellProp(iRow, iCol); \
				if (prop) { \
					prop->NAME##s.set(state, NAME); \
					invalidate(mode); \
				} \
			} else { \
				Column* col = m_columns.getPointerAt(iCol); \
				if (col) { \
					col->default##SECTION##Props.NAME##s.set(state, NAME); \
					ListElements<SECTION##CellProp> props(col->list##SECTION##Cell); \
					for (sl_size i = 0; i < props.count; i++) { \
						props[i].NAME##s.set(state, NAME); \
					} \
					invalidate(mode); \
				} \
			} \
		} else { \
			if (iRow >= 0) { \
				Row* row = m_list##SECTION##Row.getPointerAt(iRow); \
				if (row) { \
					row->defaultProps.NAME##s.set(state, NAME); \
				} \
			} else { \
				m_default##SECTION##Props.NAME##s.set(state, NAME); \
				ListElements<Row> rows(m_list##SECTION##Row); \
				for (sl_size k = 0; k < rows.count; k++) { \
					rows[k].defaultProps.NAME##s.set(state, NAME); \
				} \
			} \
			ListElements<Column> columns(m_columns); \
			for (sl_size k = 0; k < columns.count; k++) { \
				Column& col = columns[k]; \
				if (iRow >= 0) { \
					SECTION##CellProp* prop = col.list##SECTION##Cell.getPointerAt(iRow); \
					if (prop) { \
						prop->NAME##s.set(state, NAME); \
					} \
				} else { \
					col.default##SECTION##Props.NAME##s.set(state, NAME); \
					ListElements<SECTION##CellProp> props(col.list##SECTION##Cell); \
					for (sl_size i = 0; i < props.count; i++) { \
						props[i].NAME##s.set(state, NAME); \
					} \
				} \
			} \
			invalidate(mode); \
		} \
	}

#define DEFINE_SET_COLUMN_DRAWING_ATTR(FUNC, ARG, NAME) \
	void GridView::setCell##FUNC(sl_int32 iCol, ARG NAME, ViewState state, UIUpdateMode mode) \
	{ \
		ObjectLocker lock(this); \
		if (iCol >= 0) { \
			Column* pCol = m_columns.getPointerAt(iCol); \
			if (pCol) { \
				Column& col = *pCol; \
				DEFINE_SET_COLUMN_ATTR_SUB(NAME##s.set(state, NAME)) \
				invalidate(mode); \
			} \
		} else { \
			ListElements<Column> columns(m_columns); \
			for (sl_size k = 0; k < columns.count; k++) { \
				Column& col = columns[k]; \
				DEFINE_SET_COLUMN_ATTR_SUB(NAME##s.set(state, NAME)) \
			} \
			{ \
				m_defaultBodyProps.NAME##s.set(state, NAME); \
				ListElements<Row> rows(m_listBodyRow); \
				for (sl_size k = 0; k < rows.count; k++) { \
					rows[k].defaultProps.NAME##s.set(state, NAME); \
				} \
			} \
			{ \
				m_defaultHeaderProps.NAME##s.set(state, NAME); \
				ListElements<Row> rows(m_listHeaderRow); \
				for (sl_size k = 0; k < rows.count; k++) { \
					rows[k].defaultProps.NAME##s.set(state, NAME); \
				} \
			} \
			{ \
				m_defaultFooterProps.NAME##s.set(state, NAME); \
				ListElements<Row> rows(m_listFooterRow); \
				for (sl_size k = 0; k < rows.count; k++) { \
					rows[k].defaultProps.NAME##s.set(state, NAME); \
				} \
			} \
			invalidate(mode); \
		} \
	}

#define DEFINE_GET_SET_CELL_DRAWING_ATTR(FUNC, RET, ARG, NAME, DEF) \
	DEFINE_GET_SET_CELL_DRAWING_ATTR_SUB(Body, FUNC, RET, ARG, NAME, DEF) \
	DEFINE_GET_SET_CELL_DRAWING_ATTR_SUB(Header, FUNC, RET, ARG, NAME, DEF) \
	DEFINE_GET_SET_CELL_DRAWING_ATTR_SUB(Footer, FUNC, RET, ARG, NAME, DEF) \
	DEFINE_SET_COLUMN_DRAWING_ATTR(FUNC, ARG, NAME)

	DEFINE_GET_SET_CELL_DRAWING_ATTR(Background, Ref<Drawable>, const Ref<Drawable>&, background, sl_null)

	void GridView::setCellBackground(const Ref<Drawable>& background, ViewState state, UIUpdateMode mode)
	{
		setCellBackground(-1, background, state, mode);
	}

	DEFINE_GET_SET_CELL_DRAWING_ATTR(TextColor, Color, const Color&, textColor, Color::zero())

	void GridView::setCellTextColor(const Color& color, ViewState state, UIUpdateMode mode)
	{
		setCellTextColor(-1, color, state, mode);
	}

#define DEFINE_GET_SET_SPAN(SECTION) \
	sl_uint32 GridView::get##SECTION##Rowspan(sl_uint32 iRow, sl_uint32 iCol) \
	{ \
		ObjectLocker lock(this); \
		SECTION##CellProp* prop = _get##SECTION##CellProp(iRow, iCol); \
		if (prop) { \
			return prop->rowspan; \
		} \
		return 0; \
	} \
	sl_uint32 GridView::get##SECTION##Colspan(sl_uint32 iRow, sl_uint32 iCol) \
	{ \
		ObjectLocker lock(this); \
		SECTION##CellProp* prop = _get##SECTION##CellProp(iRow, iCol); \
		if (prop) { \
			return prop->colspan; \
		} \
		return 0; \
	} \
	void GridView::set##SECTION##Rowspan(sl_uint32 iRow, sl_uint32 iCol, sl_uint32 span, UIUpdateMode mode) \
	{ \
		ObjectLocker lock(this); \
		SECTION##CellProp* prop = _get##SECTION##CellProp(iRow, iCol); \
		if (prop) { \
			prop->rowspan = span; \
			if (SLIB_UI_UPDATE_MODE_IS_INIT(mode)) { \
				return; \
			} \
			_invalidate##SECTION##Cells(); \
			invalidate(mode); \
		} \
	} \
	void GridView::set##SECTION##Colspan(sl_uint32 iRow, sl_uint32 iCol, sl_uint32 span, UIUpdateMode mode) \
	{ \
		ObjectLocker lock(this); \
		SECTION##CellProp* prop = _get##SECTION##CellProp(iRow, iCol); \
		if (prop) { \
			prop->colspan = span; \
			if (SLIB_UI_UPDATE_MODE_IS_INIT(mode)) { \
				return; \
			} \
			_invalidate##SECTION##Cells(); \
			invalidate(mode); \
		} \
	}

	DEFINE_GET_SET_SPAN(Body)
	DEFINE_GET_SET_SPAN(Header)
	DEFINE_GET_SET_SPAN(Footer)

#define DEFINE_SET_ALL_SPAN(SECTION) \
	void GridView::set##SECTION##Span(sl_uint32 iRow, sl_uint32 iCol, sl_uint32 rowspan, sl_uint32 colspan, UIUpdateMode mode) \
	{ \
		ObjectLocker lock(this); \
		SECTION##CellProp* prop = _get##SECTION##CellProp(iRow, iCol); \
		if (prop) { \
			prop->rowspan = rowspan; \
			prop->colspan = colspan; \
			if (SLIB_UI_UPDATE_MODE_IS_INIT(mode)) { \
				return; \
			} \
			_invalidate##SECTION##Cells(); \
			invalidate(mode); \
		} \
	}

	DEFINE_SET_ALL_SPAN(Body)
	DEFINE_SET_ALL_SPAN(Header)
	DEFINE_SET_ALL_SPAN(Footer)

	GridView::SelectionMode GridView::getSelectionMode()
	{
		return m_selectionMode;
	}

	void GridView::setSelectionMode(SelectionMode mode)
	{
		m_selectionMode = mode;
	}

	Ref<GridView::Cell> GridView::getVisibleCell(RecordIndex record, sl_uint32 iRow, sl_uint32 iCol)
	{
		ObjectLocker lock(this);
		if (record >= 0) {
			List< Ref<Cell> > cells = m_mapRecordCache.getValue_NoLock(record);
			if (cells.isNotNull()) {
				return cells.getValueAt_NoLock(iRow + iCol * m_listBodyRow.getCount());
			}
		} else {
			Column* col = m_columns.getPointerAt(iCol);
			if (col) {
				if (record == HEADER) {
					HeaderCellProp* prop = col->listHeaderCell.getPointerAt(iRow);
					if (prop) {
						return prop->cell;
					}
				} else if (record == FOOTER) {
					FooterCellProp* prop = col->listFooterCell.getPointerAt(iRow);
					if (prop) {
						return prop->cell;
					}
				}
			}
		}
		return sl_null;
	}

	sl_int64 GridView::getSelectedRecord()
	{
		return m_selection.record;
	}

	sl_int32 GridView::getSelectedRow()
	{
		return m_selection.row;
	}

	sl_int32 GridView::getSelectedColumn()
	{
		return m_selection.column;
	}

	void GridView::select(sl_int32 row, sl_int32 column, sl_int64 record, UIUpdateMode mode)
	{
		Selection selection;
		selection.record = record;
		selection.row = row;
		selection.column = column;
		_select(selection, sl_null, mode);
	}

	void GridView::selectRecord(sl_uint64 record, UIUpdateMode mode)
	{
		select(-1, -1, record, mode);
	}

	void GridView::selectRow(sl_uint32 row, sl_uint64 record, UIUpdateMode mode)
	{
		select(row, -1, record, mode);
	}

	void GridView::selectColumn(sl_uint32 column, UIUpdateMode mode)
	{
		select(-1, column, OUTSIDE, mode);
	}

	void GridView::selectNone(UIUpdateMode mode)
	{
		select(-1, -1, OUTSIDE, mode);
	}

	void GridView::_select(const Selection& selection, UIEvent* ev, UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		Selection former = m_selection;
		if (former == selection) {
			return;
		}
		m_selection = selection;
		lock.unlock();
		invalidate(mode);
		invokeSelect(selection, former, ev);
	}

	GridView::RecordIndex GridView::getRecordAt(sl_ui_pos y, sl_int32* outRow)
	{
		return _getRowAt(outRow, y, sl_true, sl_false, sl_false);
	}

	sl_int32 GridView::getRowAt(sl_ui_pos y)
	{
		sl_int32 row;
		_getRowAt(&row, y, sl_true, sl_false, sl_false);
		return row;
	}

	sl_int32 GridView::getHeaderRowAt(sl_ui_pos y)
	{
		sl_int32 row;
		_getRowAt(&row, y, sl_false, sl_true, sl_false);
		return row;
	}

	sl_int32 GridView::getFooterRowAt(sl_ui_pos y)
	{
		sl_int32 row;
		_getRowAt(&row, y, sl_false, sl_false, sl_true);
		return row;
	}

#define DEFINE_GET_ROW_AT(SECTION) \
	sl_uint32 GridView::_get##SECTION##RowAt(sl_ui_pos y) \
	{ \
		if (y < 0) { \
			return 0; \
		} \
		ObjectLocker lock(this); \
		ListElements<Row> rows(m_list##SECTION##Row); \
		if (rows.count) { \
			sl_ui_pos t = 0; \
			for (sl_size i = 0; i < rows.count - 1; i++) { \
				sl_ui_len h = rows[i].fixedHeight; \
				if (y < t + h) { \
					return (sl_uint32)i; \
				} \
				t += h; \
			} \
			return (sl_uint32)(rows.count - 1); \
		} else { \
			return 0; \
		} \
	}

	DEFINE_GET_ROW_AT(Body)
	DEFINE_GET_ROW_AT(Header)
	DEFINE_GET_ROW_AT(Footer)

	GridView::RecordIndex GridView::_getRowAt(sl_int32* outRow, sl_ui_pos y, sl_bool flagRecord, sl_bool flagHeader, sl_bool flagFooter)
	{
		sl_ui_len heightView = getHeight();
		if (y >= 0 && y < heightView) {
			sl_ui_len heightHeader = getHeaderHeight();
			if (y <= heightHeader) {
				// Header area
				if (outRow) {
					if (flagHeader) {
						*outRow = _getHeaderRowAt(y);
					} else {
						*outRow = -1;
					}
				}
				return HEADER;
			}
			sl_ui_len yFooter = heightView - getFooterHeight();
			if (y > yFooter) {
				// Footer area
				if (outRow) {
					if (flagFooter) {
						*outRow = _getFooterRowAt(y - yFooter);
					} else {
						*outRow = -1;
					}
				}
				return FOOTER;
			}
			// Record area
			if (!flagRecord) {
				if (outRow) {
					*outRow = -1;
				}
				return 0;
			}
			sl_ui_len heightRecord = getRecordHeight();
			if (heightRecord > 0) {
				sl_int64 pos = (sl_int64)(getScrollY()) + y - heightHeader;
				sl_int64 record = pos / (sl_int32)heightRecord;
				if (record >= 0 && (sl_uint64)record < m_nRecords) {
					if (outRow) {
						pos -= record * (sl_int32)heightRecord;
						*outRow = _getBodyRowAt((sl_ui_pos)pos);
					}
					return record;
				}
			}
		}
		if (outRow) {
			*outRow = -1;
		}
		return OUTSIDE;
	}

	namespace {
		SLIB_INLINE static void FixLeftRightColumnCount(sl_uint32 count, sl_uint32& nLeft, sl_uint32& nRight)
		{
			if (nLeft > count) {
				nLeft = count;
			}
			sl_uint32 n = count - nLeft;
			if (nRight > n) {
				nRight = n;
			}
		}
	}

	sl_int32 GridView::getColumnAt(sl_ui_pos x)
	{
		if (x < 0) {
			return -1;
		}
		sl_ui_len widthView = getWidth();
		if (x >= widthView) {
			return -1;
		}
		ObjectLocker lock(this);
		ListElements<Column> columns(m_columns);
		sl_uint32 nColumns = (sl_uint32)(columns.count);
		sl_uint32 nLeft = m_nLeftColumns;
		sl_uint32 nRight = m_nRightColumns;
		FixLeftRightColumnCount(nColumns, nLeft, nRight);
		sl_ui_len pos = 0;
		sl_uint32 i;
		for (i = 0; i < nLeft; i++) {
			sl_ui_len w = columns[i].fixedWidth;
			if (x < pos + w) {
				return i;
			}
			pos += w;
		}
		sl_ui_len widthLeft = pos;
		pos = widthView;
		for (i = 0; i < nRight; i++) {
			sl_uint32 k = nColumns - 1 - i;
			sl_ui_len w = columns[k].fixedWidth;
			if (pos - w <= x) {
				return k;
			}
			pos -= w;
		}
		nColumns -= nRight;
		x += (sl_ui_len)(getScrollX()) - widthLeft;
		pos = 0;
		for (i = nLeft; i < nColumns; i++) {
			sl_ui_len w = columns[i].fixedWidth;
			if (x < pos + w) {
				return i;
			}
			pos += w;
		}
		return -1;
	}

	sl_bool GridView::getCellAt(sl_ui_pos x, sl_ui_pos y, sl_uint32* outRow, sl_uint32* outColumn, RecordIndex* outRecord)
	{
		if (outRow || outColumn) {
			sl_int32 iRow;
			RecordIndex iRecord = _getRowAt(&iRow, y, sl_true, sl_true, sl_true);
			if (iRecord != OUTSIDE && iRow >= 0) {
				sl_int32 iCol = getColumnAt(x);
				if (iCol >= 0) {
					ObjectLocker lock(this);
					CellProp* prop = _getCellProp(iRecord, iRow, iCol);
					if (prop) {
						if (outRecord) {
							*outRecord = iRecord;
						}
						if (outRow) {
							while (prop->flagCoveredY && iRow > 0) {
								iRow--;
								prop = _getCellProp(iRecord, iRow, iCol);
								if (!prop) {
									return sl_false;
								}
							}
							*outRow = iRow;
						}
						if (outColumn) {
							while (prop->flagCoveredX && iCol > 0) {
								iCol--;
								prop = _getCellProp(iRecord, iRow, iCol);
								if (!prop) {
									return sl_false;
								}
							}
							*outColumn = iCol;
						}
						return sl_true;
					}
				}
			}
		} else {
			RecordIndex iRecord = _getRowAt(sl_null, y, sl_true, sl_true, sl_true);
			if (iRecord != OUTSIDE) {
				if (outRecord) {
					*outRecord = iRecord;
				}
				return sl_true;
			}
		}
		return sl_false;
	}

	Ref<GridView::Cell> GridView::getVisibleCellAt(sl_ui_pos x, sl_ui_pos y)
	{
		sl_uint32 iRow, iCol;
		RecordIndex iRecord;
		if (getCellAt(x, y, &iRow, &iCol, &iRecord)) {
			return getVisibleCell(iRecord, iRow, iCol);
		}
		return sl_null;
	}

	ViewState GridView::getCellState(RecordIndex record, sl_int32 row, sl_int32 column)
	{
		SelectionMode mode = m_selectionMode;
		switch (mode) {
			case SelectionMode::Column:
				record = OUTSIDE;
				row = -1;
				break;
			case SelectionMode::Row:
				column = -1;
				break;
			case SelectionMode::Record:
				row = -1;
				column = -1;
				break;
			default:
				break;
		}
		ViewState state;
		if (m_hover.match(record, row, column)) {
			if (isPressedState()) {
				state = ViewState::Pressed;
			} else {
				state = ViewState::Hover;
			}
		} else {
			state = ViewState::Normal;
		}
		if (m_selection.match(record, row, column)) {
			return (ViewState)((int)state + (int)(ViewState::Selected));
		} else {
			return state;
		}
	}

	ViewState GridView::getCellState(Cell* cell)
	{
		return getCellState(cell->record, cell->row, cell->column);
	}

	SLIB_DEFINE_EVENT_HANDLER_WITHOUT_ON(GridView, ClickCell, (GridView::Cell* cell, UIEvent* ev), cell, ev)

	void GridView::onClickCell(Cell* cell, UIEvent* ev)
	{
		if (cell->record >= 0) {
			Selection selection;
			selection.record = cell->record;
			selection.row = cell->row;
			selection.column = cell->column;
			_select(selection, ev);
		}
	}

	SLIB_DEFINE_EVENT_HANDLER(GridView, RightButtonClickCell, (GridView::Cell* cell, UIEvent* ev), cell, ev)
	SLIB_DEFINE_EVENT_HANDLER(GridView, DoubleClickCell, (GridView::Cell* cell, UIEvent* ev), cell, ev)

	SLIB_DEFINE_EVENT_HANDLER(GridView, Select, (const GridView::Selection& selection, const GridView::Selection& former, UIEvent* ev), selection, former, ev)

	void GridView::onDraw(Canvas* canvas)
	{
		ObjectLocker lock(this);

		if (m_flagInitialize) {
			m_flagInitialize = sl_false;
			refreshContentWidth(UIUpdateMode::None);
			refreshContentHeight();
		}

		sl_ui_len widthView = getWidth();
		sl_ui_len heightView = getHeight();
		if (widthView < 0 || heightView <= 0) {
			return;
		}

		sl_ui_len defaultRowHeight = _getDefaultRowHeight();

		ListElements<Column> columns(m_columns);
		sl_uint32 nColumns = (sl_uint32)(columns.count);
		if (nColumns) {
			for (sl_uint32 i = 0; i < nColumns; i++) {
				Column& col = columns[i];
				col.fixedWidth = col.width;
			}
		} else {
			return;
		}

		sl_uint32 nLeft = m_nLeftColumns;
		sl_uint32 nRight = m_nRightColumns;
		FixLeftRightColumnCount(nColumns, nLeft, nRight);

		sl_ui_pos xLeft = 0;
		{
			for (sl_uint32 i = 0; i < nLeft; i++) {
				xLeft += columns[i].fixedWidth;
			}
		}
		sl_ui_pos xRight = widthView;
		if (nRight) {
			sl_uint32 iRight = nColumns - nRight;
			sl_uint32 i = nColumns - 1;
			for (;;) {
				xRight -= columns[i].fixedWidth;
				if (i > iRight) {
					i--;
				} else {
					break;
				}
			}
		}
		sl_uint32 iRight = nColumns - nRight;
		if (xLeft < xRight && iRight > nLeft) {
			sl_ui_len wMid = 0;
			{
				for (sl_uint32 i = nLeft; i < iRight; i++) {
					wMid += columns[i].fixedWidth;
				}
			}
			if (wMid) {
				sl_ui_len wView = xRight - xLeft;
				if (wMid < wView) {
					sl_ui_len t = 0;
					for (sl_uint32 i = nLeft; i + 1 < iRight; i++) {
						Column& col = columns[i];
						col.fixedWidth = col.fixedWidth * wView / wMid;
						t += col.fixedWidth;
					}
					columns[iRight - 1].fixedWidth = wView - t;
				}
			}
		}

		sl_uint32 iStartMidColumn = nLeft;
		sl_uint32 nMidColumns = 0;
		sl_ui_pos xStartMidColumn = -(sl_ui_len)(getScrollX());
		{
			{
				for (sl_uint32 i = nLeft; i < iRight; i++) {
					sl_ui_len w = columns[i].fixedWidth;
					if (0 < xStartMidColumn + w) {
						iStartMidColumn = i;
						break;
					}
					xStartMidColumn += w;
				}
			}
			if (iStartMidColumn > nLeft) {
				Column& col = columns[iStartMidColumn];
				sl_uint32 newStart = iStartMidColumn;
				_fixHeaderStartMidColumn(columns.data, nColumns, nLeft, iStartMidColumn, newStart);
				_fixFooterStartMidColumn(columns.data, nColumns, nLeft, iStartMidColumn, newStart);
				_fixBodyStartMidColumn(columns.data, nColumns, nLeft, iStartMidColumn, newStart);
				for (sl_uint32 i = newStart; i < iStartMidColumn; i++) {
					xStartMidColumn -= columns[i].fixedWidth;
				}
				iStartMidColumn = newStart;
			}
			{
				sl_uint32 i = iStartMidColumn;
				sl_ui_pos x = xStartMidColumn;
				for (; i < iRight && x < xRight; i++) {
					x += columns[i].fixedWidth;
				}
				nMidColumns = i - iStartMidColumn;
			}
		}

		sl_ui_len heightHeader = 0;
		{
			ListElements<Row> rows(m_listHeaderRow);
			for (sl_size i = 0; i < rows.count; i++) {
				Row& row = rows[i];
				row.fixedHeight = FixLength(row.height, defaultRowHeight);
				heightHeader += row.fixedHeight;
			}
		}
		sl_ui_len heightFooter = 0;
		{
			ListElements<Row> rows(m_listFooterRow);
			if (rows.count) {
				for (sl_size i = 0; i < rows.count; i++) {
					Row& row = rows[i];
					row.fixedHeight = FixLength(row.height, defaultRowHeight);
					heightFooter += row.fixedHeight;
				}
			}
		}
		sl_ui_pos yFooter = heightView - heightFooter;

		if (m_nRecords) {
			ListElements<Row> rows(m_listBodyRow);
			if (rows.count) {
				for (sl_size i = 0; i < rows.count; i++) {
					Row& row = rows[i];
					row.fixedHeight = FixLength(row.height, defaultRowHeight);
				}
				CanvasStateScope scope(canvas);
				canvas->clipToRectangle(0, (sl_real)heightHeader, (sl_real)widthView, (sl_real)(yFooter - heightHeader));
				_drawRecords(canvas, heightHeader, yFooter, columns.data, nColumns, nLeft, xLeft, nRight, xRight, iStartMidColumn, nMidColumns, xStartMidColumn);
			}
		}
		if (heightFooter) {
			CanvasStateScope scope(canvas);
			canvas->clipToRectangle(0, (sl_real)heightHeader, (sl_real)widthView, (sl_real)(heightView - heightHeader));
			_drawFooter(canvas, yFooter, heightView, columns.data, nColumns, nLeft, xLeft, nRight, xRight, iStartMidColumn, nMidColumns, xStartMidColumn);
		}
		if (heightHeader) {
			_drawHeader(canvas, 0, heightHeader, columns.data, nColumns, nLeft, xLeft, nRight, xRight, iStartMidColumn, nMidColumns, xStartMidColumn);
		}

		if (nRight) {
			CanvasStateScope scope(canvas);
			canvas->clipToRectangle((sl_real)xLeft, 0, (sl_real)(widthView - xLeft), (sl_real)heightView);
			_drawVertOuterGrid(canvas, xRight, 0, heightHeader, yFooter, heightView, getHeaderGrid(), getRightGrid(), getFooterGrid());
		}
		if (nLeft) {
			_drawVertOuterGrid(canvas, xLeft, 0, heightHeader, yFooter, heightView, getHeaderGrid(), getLeftGrid(), getFooterGrid());
		}
		if (heightFooter) {
			Ref<Pen> grid = getFooterGrid();
			CanvasStateScope scope(canvas);
			canvas->clipToRectangle(0, (sl_real)heightHeader, (sl_real)widthView, (sl_real)(heightView - heightHeader));
			_drawHorzOuterGrid(canvas, 0, xLeft, xRight, widthView, yFooter, grid, grid, grid);
		}
		if (heightHeader) {
			Ref<Pen> grid = getHeaderGrid();
			_drawHorzOuterGrid(canvas, 0, xLeft, xRight, widthView, heightHeader, grid, grid, grid);
		}
	}

#define DEFINE_FIX_BODY_START_MID_COLUMN(SECTION) \
	void GridView::_fix##SECTION##StartMidColumn(Column* columns, sl_uint32 nColumns, sl_uint32 nLeft, sl_uint32 iStart, sl_uint32& newStart) \
	{ \
		ListElements<SECTION##CellProp> props(columns[iStart].list##SECTION##Cell); \
		for (sl_size i = 0; i < props.count; i++) { \
			if (props[i].flagCoveredX) { \
				sl_uint32 k = iStart; \
				while (k > nLeft) { \
					SECTION##CellProp* prop = columns[k].list##SECTION##Cell.getPointerAt(i); \
					if (prop) { \
						if (!(prop->flagCoveredX)) { \
							break; \
						} \
					} else { \
						break; \
					} \
					k--; \
				} \
				if (k < newStart) { \
					newStart = k; \
				} \
			} \
		} \
	}

	DEFINE_FIX_BODY_START_MID_COLUMN(Body)
	DEFINE_FIX_BODY_START_MID_COLUMN(Header)
	DEFINE_FIX_BODY_START_MID_COLUMN(Footer)

	void GridView::_drawRecords(Canvas* canvas, sl_ui_len top, sl_ui_len bottom, Column* columns, sl_uint32 nColumns, sl_uint32 nLeft, sl_ui_pos xLeft, sl_uint32 nRight, sl_ui_pos xRight, sl_uint32 iStartMidColumn, sl_uint32 nMidColumns, sl_ui_pos xStartMidColumn)
	{
		ListElements<Row> rows(m_listBodyRow);
		sl_size nCells = rows.count * nColumns;
		if (!nCells) {
			return;
		}

		sl_ui_len widthView = getWidth();

		_prepareBodyCells(columns, nColumns);

		sl_ui_len heightRecord = getRecordHeight();
		if (heightRecord <= 0) {
			return;
		}
		sl_int64 sy = (sl_int64)(getScrollY());
		sl_uint64 iRecord = Math::max(sy / heightRecord, (sl_int64)0);
		if (iRecord >= m_nRecords) {
			return;
		}
		sl_ui_pos yRecordFirst = (sl_ui_pos)((sl_int64)iRecord * heightRecord - sy) + top;
		sl_uint32 nRecords = Math::clamp((sl_int32)((bottom - yRecordFirst + heightRecord - 1) / heightRecord), (sl_int32)0, (sl_int32)MAX_RECORDS_PER_SCREEN);
		nRecords = Math::min(nRecords, (sl_uint32)(m_nRecords - iRecord));
		sl_uint32 iRight = nColumns - nRight;

		Function<Variant(sl_uint64 record)> recordDataFunc(m_recordData);
		HashMap< sl_uint64, List< Ref<Cell> > > cache = Move(m_mapRecordCache);
		HashMap< sl_uint64, List< Ref<Cell> > > map;

		sl_ui_pos yRecord = yRecordFirst;
		for (sl_uint32 i = 0; i < nRecords; i++) {
			List< Ref<Cell> > cells = cache.getValue_NoLock(iRecord);
			if (cells.getCount() != nCells) {
				cells = List< Ref<Cell> >::create(nCells);
				if (cells.isNull()) {
					return;
				}
			}
			Variant recordData;
			if (recordDataFunc.isNotNull()) {
				recordData = recordDataFunc(iRecord);
				if (recordData.isUndefined()) {
					recordData.setNull();
				}
			}
			{
				CanvasStateScope scope(canvas);
				canvas->clipToRectangle((sl_real)xLeft, (sl_real)yRecord, (sl_real)(xRight - xLeft), (sl_real)heightRecord);
				sl_ui_len xCell = xLeft + xStartMidColumn;
				for (sl_uint32 k = 0; k < nMidColumns; k++) {
					sl_uint32 iCol = iStartMidColumn + k;
					Column& col = columns[iCol];
					_drawBodyColumn(canvas, xCell, yRecord, col, iCol, rows.data, (sl_uint32)(rows.count), iRecord, recordData, cells);
					xCell += col.fixedWidth;
				}
			}
			if (nRight) {
				CanvasStateScope scope(canvas);
				canvas->clipToRectangle((sl_real)xLeft, (sl_real)yRecord, (sl_real)(widthView - xLeft), (sl_real)heightRecord);
				sl_uint32 iCol = nColumns - 1;
				sl_ui_pos x = widthView;
				for (;;) {
					Column& col = columns[iCol];
					x -= col.fixedWidth;
					_drawBodyColumn(canvas, x, yRecord, col, iCol, rows.data, (sl_uint32)(rows.count), iRecord, recordData, cells);
					if (iCol > iRight) {
						iCol--;
					} else {
						break;
					}
				}
			}
			{
				sl_ui_pos x = 0;
				for (sl_uint32 iCol = 0; iCol < nLeft; iCol++) {
					Column& col = columns[iCol];
					_drawBodyColumn(canvas, x, yRecord, col, iCol, rows.data, (sl_uint32)(rows.count), iRecord, recordData, cells);
					x += col.fixedWidth;
				}
			}
			map.put_NoLock(iRecord, Move(cells));
			yRecord += heightRecord;
			iRecord++;
		}
		// Draw Grid
		{
			{
				CanvasStateScope scope(canvas);
				canvas->clipToRectangle((sl_real)xLeft, (sl_real)top, (sl_real)(xRight - xLeft), (sl_real)(bottom - top));
				_drawBodyInnerGrid(canvas, xLeft + xStartMidColumn, yRecordFirst, bottom, columns + iStartMidColumn, nMidColumns, rows.data, (sl_uint32)(rows.count), nRecords, sl_true, getBodyGrid());
			}
			{
				CanvasStateScope scope(canvas);
				canvas->clipToRectangle((sl_real)xLeft, (sl_real)top, (sl_real)(widthView - xLeft), (sl_real)(bottom - top));
				_drawBodyInnerGrid(canvas, xRight, yRecordFirst, bottom, columns + iRight, nRight, rows.data, (sl_uint32)(rows.count), nRecords, sl_true, getRightGrid());
			}
			{
				_drawBodyInnerGrid(canvas, 0, yRecordFirst, bottom, columns, nLeft, rows.data, (sl_uint32)(rows.count), nRecords, sl_true, getLeftGrid());
			}
		}

		m_mapRecordCache = Move(map);
	}

#define DEFINE_DRAW_FIXED(SECTION) \
	void GridView::_draw##SECTION(Canvas* canvas, sl_ui_len top, sl_ui_len bottom, Column* columns, sl_uint32 nColumns, sl_uint32 nLeft, sl_ui_pos xLeft, sl_uint32 nRight, sl_ui_pos xRight, sl_uint32 iStartMidColumn, sl_uint32 nMidColumns, sl_ui_pos xStartMidColumn) \
	{ \
		sl_ui_len widthView = getWidth(); \
		_prepare##SECTION##Cells(columns, nColumns); \
		ListElements<Row> rows(m_list##SECTION##Row); \
		sl_uint32 iRight = nColumns - nRight; \
		Ref<Pen> grid = get##SECTION##Grid(); \
		{ \
			CanvasStateScope scope(canvas); \
			canvas->clipToRectangle((sl_real)xLeft, (sl_real)top, (sl_real)(xRight - xLeft), (sl_real)(bottom - top)); \
			sl_ui_pos xCell = xLeft + xStartMidColumn; \
			for (sl_uint32 k = 0; k < nMidColumns; k++) { \
				sl_uint32 iCol = iStartMidColumn + k; \
				Column& col = columns[iCol]; \
				_draw##SECTION##Column(canvas, xCell, top, col, iCol, rows.data, (sl_uint32)(rows.count)); \
				xCell += col.fixedWidth; \
			} \
			_draw##SECTION##InnerGrid(canvas, xLeft + xStartMidColumn, top, bottom, columns + iStartMidColumn, nMidColumns, rows.data, (sl_uint32)(rows.count), 1, sl_false, grid); \
		} \
		if (nRight) { \
			CanvasStateScope scope(canvas); \
			canvas->clipToRectangle((sl_real)xLeft, (sl_real)top, (sl_real)(widthView - xLeft), (sl_real)(bottom - top)); \
			sl_uint32 iCol = nColumns - 1; \
			sl_ui_pos x = widthView; \
			for (;;) { \
				Column& col = columns[iCol]; \
				x -= col.fixedWidth; \
				_draw##SECTION##Column(canvas, x, top, col, iCol, rows.data, (sl_uint32)(rows.count)); \
				if (iCol > iRight) { \
					iCol--; \
				} else { \
					break; \
				} \
			} \
			_draw##SECTION##InnerGrid(canvas, xRight, top, bottom, columns + iCol, nRight, rows.data, (sl_uint32)(rows.count), 1, sl_false, grid); \
		} \
		{ \
			sl_ui_pos x = 0; \
			for (sl_uint32 iCol = 0; iCol < nLeft; iCol++) { \
				Column& col = columns[iCol]; \
				_draw##SECTION##Column(canvas, x, top, col, iCol, rows.data, (sl_uint32)(rows.count)); \
				x += col.fixedWidth; \
			} \
			_draw##SECTION##InnerGrid(canvas, 0, top, bottom, columns, nLeft, rows.data, (sl_uint32)(rows.count), 1, sl_false, grid); \
		} \
	}

	DEFINE_DRAW_FIXED(Header)
	DEFINE_DRAW_FIXED(Footer)

	void GridView::_drawBodyColumn(Canvas* canvas, sl_ui_pos x, sl_ui_pos y, Column& col, sl_uint32 iCol, Row* rows, sl_uint32 nRows, sl_uint64 iRecord, const Variant& recordData, List< Ref<Cell> >& cells)
	{
		ListElements<BodyCellProp> props(col.listBodyCell);
		if (props.count > nRows) {
			props.count = nRows;
		}
		sl_uint32 iCell = iCol * nRows;
		for (sl_uint32 iRow = 0; iRow < props.count; iRow++) {
			BodyCellProp& prop = props[iRow];
			Ref<Cell>* pCell = cells.getPointerAt(iCell);
			Cell* cell = pCell->get();
			if (!cell) {
				Ref<Cell> newCell = _createBodyCell(prop, iRecord, iRow, iCol, recordData);
				if (newCell.isNotNull()) {
					cell = newCell.get();
					*pCell = Move(newCell);
				}
			}
			if (cell) {
				_drawCell(canvas, x, y, cell);
			}
			y += rows[iRow].fixedHeight;
			iCell++;
		}
	}

#define DEFINE_DRAW_INNER_GRID(SECTION) \
	void GridView::_draw##SECTION##InnerGrid(Canvas* canvas, sl_ui_pos _x, sl_ui_pos top, sl_ui_pos bottom, Column* columns, sl_uint32 nColumns, Row* rows, sl_uint32 nRows, sl_uint32 nRecords, sl_bool flagBody, const Ref<Pen>& pen) \
	{ \
		/* Vertical Lines */ { \
			sl_ui_pos x = _x; \
			for (sl_uint32 iCol = 1; iCol < nColumns; iCol++) { \
				x += columns[iCol-1].fixedWidth; \
				ListElements<SECTION##CellProp> props(columns[iCol].list##SECTION##Cell); \
				if (props.count > nRows) { \
					props.count = nRows; \
				} \
				sl_ui_pos y = top; \
				sl_ui_pos start = y; \
				for (sl_uint32 iRecord = 0; iRecord < nRecords; iRecord++) { \
					for (sl_uint32 iRow = 0; iRow < props.count; iRow++) { \
						sl_ui_pos y2 = y + rows[iRow].fixedHeight; \
						if (props[iRow].flagCoveredX) { \
							if (y != start) { \
								canvas->drawLine((sl_real)x, (sl_real)start, (sl_real)x, (sl_real)y, pen); \
							} \
							start = y2; \
						} \
						y = y2; \
					} \
				} \
				if (y < bottom) { \
					y = bottom; \
				} \
				if (y != start) { \
					canvas->drawLine((sl_real)x, (sl_real)start, (sl_real)x, (sl_real)y, pen); \
				} \
			} \
		} \
		/* Horizontal Lines */ { \
			sl_ui_pos w = 0; \
			if (nRecords) { \
				for (sl_uint32 iCol = 0; iCol < nColumns; iCol++) { \
					w += columns[iCol].fixedWidth; \
				} \
			} \
			sl_ui_pos y = top; \
			for (sl_uint32 iRecord = 0; iRecord < nRecords; iRecord++) { \
				for (sl_uint32 iRow = 1; iRow < nRows; iRow++) { \
					y += rows[iRow-1].fixedHeight; \
					sl_ui_pos x = _x; \
					sl_ui_pos start = x; \
					for (sl_uint32 iCol = 0; iCol < nColumns; iCol++) { \
						Column& col = columns[iCol]; \
						sl_ui_pos x2 = x + col.fixedWidth; \
						SECTION##CellProp* prop = col.list##SECTION##Cell.getPointerAt(iRow); \
						if (prop) { \
							if (prop->flagCoveredY) { \
								if (x != start) { \
									canvas->drawLine((sl_real)start, (sl_real)y, (sl_real)x, (sl_real)y, pen); \
								} \
								start = x2; \
							} \
						} \
						x = x2; \
					} \
					if (x != start) { \
						canvas->drawLine((sl_real)start, (sl_real)y, (sl_real)x, (sl_real)y, pen); \
					} \
				} \
				y += rows[nRows-1].fixedHeight; \
				if (flagBody || iRecord + 1 < nRecords) { \
					canvas->drawLine((sl_real)_x, (sl_real)y, (sl_real)(_x + w), (sl_real)y, pen); \
				} \
			} \
		} \
	}

	DEFINE_DRAW_INNER_GRID(Body)
	DEFINE_DRAW_INNER_GRID(Header)
	DEFINE_DRAW_INNER_GRID(Footer)

	void GridView::_drawHorzOuterGrid(Canvas* canvas, sl_ui_pos x1, sl_ui_pos x2, sl_ui_pos x3, sl_ui_pos x4, sl_ui_pos y, const Ref<Pen>& penLeft, const Ref<Pen>& penMid, const Ref<Pen>& penRight)
	{
		if (x3 < x2) {
			x3 = x2;
		}
		if (penLeft == penMid) {
			x2 = x1;
		} else {
			if (x1 != x2) {
				canvas->drawLine((sl_real)x1, (sl_real)y, (sl_real)x2, (sl_real)y, penLeft);
			}
		}
		if (penRight == penMid) {
			x3 = x4;
		} else {
			if (x3 != x4) {
				canvas->drawLine((sl_real)x3, (sl_real)y, (sl_real)x4, (sl_real)y, penLeft);
			}
		}
		if (x2 != x3) {
			canvas->drawLine((sl_real)x2, (sl_real)y, (sl_real)x3, (sl_real)y, penMid);
		}
	}

	void GridView::_drawVertOuterGrid(Canvas* canvas, sl_ui_pos x, sl_ui_pos y1, sl_ui_pos y2, sl_ui_pos y3, sl_ui_pos y4, const Ref<Pen>& penTop, const Ref<Pen>& penMid, const Ref<Pen>& penBottom)
	{
		if (y3 < y2) {
			y3 = y2;
		}
		if (penTop == penMid) {
			y2 = y1;
		} else {
			if (y1 != y2) {
				canvas->drawLine((sl_real)x, (sl_real)y1, (sl_real)x, (sl_real)y2, penTop);
			}
		}
		if (penBottom == penMid) {
			y3 = y4;
		} else {
			if (y3 != y4) {
				canvas->drawLine((sl_real)x, (sl_real)y3, (sl_real)x, (sl_real)y4, penBottom);
			}
		}
		if (y2 != y3) {
			canvas->drawLine((sl_real)x, (sl_real)y2, (sl_real)x, (sl_real)y3, penMid);
		}
	}

#define DEFINE_DRAW_FIXED_COLUMN(SECTION, SECTION_CONSTANT) \
	void GridView::_draw##SECTION##Column(Canvas* canvas, sl_ui_pos x, sl_ui_pos y, Column& col, sl_uint32 iCol, Row* rows, sl_uint32 nRows) \
	{ \
		ListElements<SECTION##CellProp> props(col.list##SECTION##Cell); \
		if (props.count > nRows) { \
			props.count = nRows; \
		} \
		for (sl_uint32 iRow = 0; iRow < props.count; iRow++) { \
			SECTION##CellProp& prop = props[iRow]; \
			Cell* cell = _getFixedCell(prop, SECTION_CONSTANT, iRow, iCol); \
			if (cell) { \
				_drawCell(canvas, x, y, cell); \
			} \
			y += rows[iRow].fixedHeight; \
		} \
	}

	DEFINE_DRAW_FIXED_COLUMN(Header, HEADER)
	DEFINE_DRAW_FIXED_COLUMN(Footer, FOOTER)

	void GridView::_drawCell(Canvas* canvas, sl_ui_pos x, sl_ui_pos y, Cell* cell)
	{
		Rectangle frame((sl_real)x, (sl_real)y, (sl_real)(x + cell->width), (sl_real)(y + cell->height));
		CanvasStateScope scope(canvas);
		canvas->clipToRectangle(frame);
		ViewState state = getCellState(cell);
		Ref<Drawable> background = cell->backgrounds.evaluate(state);
		if (background.isNotNull()) {
			canvas->draw(frame, background);
		}
		DrawCellParam param;
		param.frame = frame;
		param.textColor = cell->textColors.evaluate(state);
		cell->onDraw(canvas, param);
	}

#define DEFINE_PREPARE_CELLS(SECTION) \
	void GridView::_prepare##SECTION##Cells(Column* columns, sl_uint32 nColumns) \
	{ \
		if (!m_flagInvalidate##SECTION##Rows) { \
			return; \
		} \
		m_flagInvalidate##SECTION##Rows = sl_false; \
		ListElements<Row> rows(m_list##SECTION##Row); \
		for (sl_uint32 iCol = 0; iCol < nColumns; iCol++) { \
			Column& col = columns[iCol]; \
			ListElements<SECTION##CellProp> props(col.list##SECTION##Cell); \
			sl_uint32 nRows = (sl_uint32)(props.count); \
			if (nRows > rows.count) { \
				nRows = (sl_uint32)(rows.count); \
			} \
			for (sl_uint32 iRow = 0; iRow < nRows; iRow++) { \
				SECTION##CellProp& prop = props[iRow]; \
				prop.width = col.fixedWidth; \
				prop.height = rows[iRow].fixedHeight; \
				if (prop.flagCoveredX || prop.flagCoveredY) { \
					continue; \
				} \
				sl_uint32 iRowEnd = iRow + prop.rowspan; \
				if (iRowEnd > nRows) { \
					iRowEnd = nRows; \
				} \
				sl_uint32 iColEnd = iCol + prop.colspan; \
				if (iColEnd > nColumns) { \
					iColEnd = nColumns; \
				} \
				sl_uint32 i; \
				for (i = iRow + 1; i < iRowEnd; i++) { \
					props[i].flagCoveredY = sl_true; \
					prop.height += rows[i].fixedHeight; \
				} \
				for (i = iCol + 1; i < iColEnd; i++) { \
					Column& spanCol = columns[i]; \
					prop.width += spanCol.fixedWidth; \
					ListElements<SECTION##CellProp> spanProps(spanCol.list##SECTION##Cell); \
					if (iRowEnd <= spanProps.count) { \
						spanProps[iRow].flagCoveredX = sl_true; \
						for (sl_uint32 j = iRow + 1; j < iRowEnd; j++) { \
							SECTION##CellProp& cell = spanProps[j]; \
							cell.flagCoveredX = sl_true; \
							cell.flagCoveredY = sl_true; \
						} \
					} \
				} \
			} \
		} \
	}

	DEFINE_PREPARE_CELLS(Body)
	DEFINE_PREPARE_CELLS(Header)
	DEFINE_PREPARE_CELLS(Footer)


	Ref<GridView::Cell> GridView::_createBodyCell(BodyCellProp& prop, RecordIndex iRecord, sl_uint32 iRow, sl_uint32 iCol, const Variant& recordData)
	{
		if (prop.flagCoveredX || prop.flagCoveredY) {
			return sl_null;
		}
		CellParam param(prop);
		param.view = this;
		param.record = iRecord;
		param.row = iRow;
		param.column = iCol;
		param.recordData = recordData;
		Ref<Cell> cell = prop.creator(param);
		if (cell.isNull()) {
			return sl_null;
		}
		(CellParam&)(*cell) = Move(param);
		cell->onInit();
		return cell;
	}

	GridView::Cell* GridView::_getFixedCell(FixedCellProp& prop, RecordIndex iRecord, sl_uint32 iRow, sl_uint32 iCol)
	{
		if (prop.flagCoveredX || prop.flagCoveredY) {
			return sl_null;
		}
		if (prop.flagMadeCell) {
			return prop.cell.get();
		}
		prop.flagMadeCell = sl_true;
		CellParam param(prop);
		param.view = this;
		param.record = iRecord;
		param.row = iRow;
		param.column = iCol;
		prop.cell = prop.creator(param);
		if (prop.cell.isNull()) {
			return sl_null;
		}
		(CellParam&)(*(prop.cell)) = Move(param);
		prop.cell->onInit();
		return prop.cell.get();
	}

	void GridView::onClickEvent(UIEvent* ev)
	{
		View::onClickEvent(ev);

		Ref<Cell> cell = _getEventCell(ev);
		if (cell.isNotNull()) {
			invokeClickCell(cell.get(), ev);
		}
	}

	void GridView::onMouseEvent(UIEvent* ev)
	{
		UIAction action = ev->getAction();
		switch (action) {
			case UIAction::RightButtonDown:
				{
					Ref<Cell> cell = _getEventCell(ev);
					if (cell.isNotNull()) {
						invokeRightButtonClickCell(cell.get(), ev);
					}
				}
				break;
			case UIAction::LeftButtonDoubleClick:
				{
					Ref<Cell> cell = _getEventCell(ev);
					if (cell.isNotNull()) {
						invokeDoubleClickCell(cell.get(), ev);
					}
				}
				break;
			case UIAction::MouseMove:
				{
					sl_uint32 row = -1;
					sl_uint32 col = -1;
					sl_int64 record = OUTSIDE;
					getCellAt((sl_ui_pos)(ev->getX()), (sl_ui_pos)(ev->getY()), &row, &col, &record);
					if (m_hover.record != record || m_hover.row != row || m_hover.column != col) {
						m_hover.record = record;
						m_hover.row = row;
						m_hover.column = col;
						invalidate();
					}
				}
				break;
			case UIAction::MouseLeave:
				{
					m_hover.record = OUTSIDE;
					m_hover.row = -1;
					m_hover.column = -1;
					invalidate();
				}
				break;
			default:
				break;
		}
		View::onMouseEvent(ev);
	}

	Ref<GridView::Cell> GridView::_getEventCell(UIEvent* ev)
	{
		if (ev->isMouseEvent()) {
			return getVisibleCellAt((sl_ui_pos)(ev->getX()), (sl_ui_pos)(ev->getY()));
		}
		return sl_null;
	}

	void GridView::onKeyEvent(UIEvent* ev)
	{
		View::onKeyEvent(ev);
	}

	void GridView::onResize(sl_ui_len width, sl_ui_len height)
	{
		View::onResize(width, height);
		_invalidateBodyCells();
		_invalidateHeaderCells();
		_invalidateFooterCells();
		refreshContentWidth(UIUpdateMode::None);
		refreshContentHeight();
	}

	void GridView::onUpdateFont(const Ref<Font>& font)
	{
		ObjectLocker lock(this);
		_invalidateColumns();
		refreshContentHeight(UIUpdateMode::None);
	}

	void GridView::_invalidateBodyCells()
	{
		m_flagInvalidateBodyRows = sl_true;
		m_mapRecordCache.setNull();
		ListElements<Column> columns(m_columns);
		for (sl_size iCol = 0; iCol < columns.count; iCol++) {
			Column& col = columns[iCol];
			ListElements<BodyCellProp> props(col.listBodyCell);
			for (sl_size iRow = 0; iRow < props.count; iRow++) {
				BodyCellProp& prop = props[iRow];
				prop.flagCoveredX = sl_false;
				prop.flagCoveredY = sl_false;
			}
		}
	}

	void GridView::_invalidateBodyCells(Column& col, sl_uint32 iCol)
	{
		sl_uint32 nRows = (sl_uint32)(m_listBodyRow.getCount());
		if (!nRows) {
			return;
		}
		auto node = m_mapRecordCache.getFirstNode();
		while (node) {
			for (sl_uint32 iRow = 0; iRow < nRows; iRow++) {
				node->value.setAt_NoLock(iCol * nRows + iRow, sl_null);
			}
			node = node->getNext();
		}
	}

	void GridView::_invalidateBodyCell(BodyCellProp& prop, sl_uint32 iRow, sl_uint32 iCol)
	{
		sl_uint32 nRows = (sl_uint32)(m_listBodyRow.getCount());
		if (!nRows) {
			return;
		}
		auto node = m_mapRecordCache.getFirstNode();
		while (node) {
			node->value.setAt_NoLock(iCol * nRows + iRow, sl_null);
			node = node->getNext();
		}
	}

	void GridView::_invalidateColumns()
	{
		_invalidateBodyCells();
		ListElements<Column> columns(m_columns);
		for (sl_size iCol = 0; iCol < columns.count; iCol++) {
			Column& col = columns[iCol];
			_invalidateHeaderCells(col, (sl_uint32)iCol);
			_invalidateFooterCells(col, (sl_uint32)iCol);
		}
	}

#define DEFINE_INVALIDATE_FIXED_CELLS(SECTION) \
	void GridView::_invalidate##SECTION##Cell(SECTION##CellProp& prop, sl_uint32 iRow, sl_uint32 iCol) \
	{ \
		prop.cell.setNull(); \
		prop.flagMadeCell = sl_false; \
	} \
	void GridView::_invalidate##SECTION##Cells(Column& col, sl_uint32 iCol, sl_bool flagAllColumns) \
	{ \
		ListElements<SECTION##CellProp> props(col.list##SECTION##Cell); \
		for (sl_size iRow = 0; iRow < props.count; iRow++) { \
			SECTION##CellProp& prop = props[iRow]; \
			prop.cell.setNull(); \
			prop.flagMadeCell = sl_false; \
			if (flagAllColumns) { \
				prop.flagCoveredX = sl_false; \
				prop.flagCoveredY = sl_false; \
			} \
		} \
	} \
	void GridView::_invalidate##SECTION##Cells() \
	{ \
		m_flagInvalidate##SECTION##Rows = sl_true; \
		ListElements<Column> columns(m_columns); \
		for (sl_size iCol = 0; iCol < columns.count; iCol++) { \
			Column& col = columns[iCol]; \
			_invalidate##SECTION##Cells(col, (sl_uint32)iCol, sl_true); \
		} \
	}

	DEFINE_INVALIDATE_FIXED_CELLS(Header)
	DEFINE_INVALIDATE_FIXED_CELLS(Footer)

}
