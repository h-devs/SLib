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

#define DEFAULT_COLUMN_WIDTH 80
#define DEFAULT_TEXT_COLOR Color::Black
#define MAX_RECORDS_PER_SCREEN 1000

namespace slib
{

	SLIB_DEFINE_NESTED_CLASS_DEFAULT_MEMBERS(GridView, DrawCellParam)

	GridView::DrawCellParam::DrawCellParam()
	{
	}

	SLIB_DEFINE_NESTED_CLASS_DEFAULT_MEMBERS(GridView, CellEventParam)

	GridView::CellEventParam::CellEventParam(): row(0), column(0), record(0)
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

	GridView::LabelCell::LabelCell()
	{
	}

	GridView::LabelCell::~LabelCell()
	{
	}

	const GridView::CellCreator& GridView::LabelCell::creator()
	{
		SLIB_SAFE_LOCAL_STATIC(CellCreator, ret, [](CellParam&) {
			return new LabelCell;
		})
		return ret;
	}

	void GridView::LabelCell::onInit()
	{
		TextBoxParam tp;
		onPrepareTextBox(tp);
		m_textBox.update(tp);
	}

	void GridView::LabelCell::onDraw(Canvas* canvas, DrawParam& param)
	{
		m_textBox.draw(canvas, param);
	}

	void GridView::LabelCell::onPrepareTextBox(TextBoxParam& param)
	{
		if (text.isNotNull()) {
			if (record >= 0 && recordData.isNotUndefined()) {
				param.text = String::format(text, recordData);
			} else {
				param.text = text;
			}
		}
		param.font = font;
		param.align = align;
		param.multiLineMode = multiLineMode;
		param.ellipsizeMode = ellipsizeMode;
		param.lineCount = lineCount;
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

	void GridView::TextCell::onPrepareTextBox(TextBoxParam& param)
	{
		LabelCell::onPrepareTextBox(param);
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
		LabelCell::onPrepareTextBox(param);
		if (record >= 0) {
			param.text = String::fromInt64(m_start + record);
		}
	}

	
	SLIB_DEFINE_NESTED_CLASS_DEFAULT_MEMBERS(GridView, CellProp)

	GridView::CellProp::CellProp(): flagCoveredX(sl_false), flagCoveredY(sl_false)
	{
	}
	
	SLIB_DEFINE_NESTED_CLASS_DEFAULT_MEMBERS(GridView, FixedCellProp)

	GridView::FixedCellProp::FixedCellProp(): flagMadeCell(sl_false)
	{
	}
	
	SLIB_DEFINE_NESTED_CLASS_DEFAULT_MEMBERS(GridView, Column)

	GridView::Column::Column(): width(-1)
	{
	}
	
	SLIB_DEFINE_NESTED_CLASS_DEFAULT_MEMBERS(GridView, Row)

	GridView::Row::Row(): height(-1)
	{
	}

	SLIB_DEFINE_NESTED_CLASS_DEFAULT_MEMBERS(GridView, Location)

	GridView::Location::Location(): record(-1), row(-1), column(-1)
	{
	}

	sl_bool GridView::Location::match(Cell* cell)
	{
		if (record < 0 && row < 0 && column < 0) {
			return sl_false;
		}
		if (record >= 0) {
			if (record != cell->record) {
				return sl_false;
			}
		}
		if (row >= 0) {
			if (row != cell->row) {
				return sl_false;
			}
		}
		if (column >= 0) {
			if (column != cell->column) {
				return sl_false;
			}
		}
		return sl_true;
	}


	SLIB_DEFINE_OBJECT(GridView, View)

	GridView::GridView()
	{
		setCanvasScrolling(sl_false);
		setVerticalScrolling(sl_true, UIUpdateMode::Init);
		setHorizontalScrolling(sl_true, UIUpdateMode::Init);
		setContentScrollingByMouse(sl_false);
		setFocusable();
		setUsingFont();

		m_nRecords = 1;
		m_nLeftColumns = 0;
		m_nRightColumns = 0;

		m_flagInvalidateBodyRows = sl_true;
		m_flagInvalidateHeaderRows = sl_true;
		m_flagInvalidateFooterRows = sl_true;
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
		if (m_columns.getCount() == count) {
			return sl_true;
		}
		if (!(m_columns.setCount_NoLock(count))) {
			return sl_false;
		}
		if (SLIB_UI_UPDATE_MODE_IS_INIT(mode)) {
			return sl_true;
		}
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
		if (m_list##SECTION##Row.getCount() == count) { \
			return sl_true; \
		} \
		ListElements<Column> columns(m_columns); \
		for (sl_size i = 0; i < columns.count; i++) { \
			Column& col = columns[i]; \
			if (!(col.list##SECTION##Cell.setCount_NoLock(count))) { \
				return sl_false; \
			} \
		} \
		if (!(m_list##SECTION##Row.setCount_NoLock(count))) { \
			return sl_false; \
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

	namespace {
		SLIB_INLINE static sl_ui_len FixColumnWidth(sl_ui_len width)
		{
			if (width >= 0) {
				return width;
			} else {
				return DEFAULT_COLUMN_WIDTH;
			}
		}
	}

	sl_ui_len GridView::getColumnWidth(sl_uint32 index)
	{
		ObjectLocker lock(this);
		Column* col = m_columns.getPointerAt(index);
		if (col) {
			return FixColumnWidth(col->width);
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

	namespace {
		SLIB_INLINE static sl_ui_len GetDefaultRowHeight(const Ref<Font>& font)
		{
			return (sl_ui_len)(font->getSize() * 1.5f);
		}
	}

#define DEFINE_GET_SECTION_HEIGHT(PREFIX, SECTION) \
	sl_ui_len GridView::get##PREFIX##Height() \
	{ \
		sl_ui_len height = 0; \
		ObjectLocker lock(this); \
		ListElements<Row> rows(m_list##SECTION##Row); \
		sl_uint32 nDefault = 0; \
		for (sl_size i = 0; i < rows.count; i++) { \
			Row& row = rows[i]; \
			if (row.height >= 0) { \
				height += row.height; \
			} else { \
				nDefault++; \
			} \
		} \
		height += GetDefaultRowHeight(getFont()) * nDefault; \
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
			return _get##SECTION##RowHeight(*row); \
		} else { \
			return 0; \
		} \
	} \
	sl_ui_len GridView::_get##SECTION##RowHeight(Row& row) \
	{ \
		if (row.height >= 0) { \
			return row.height; \
		} else { \
			return GetDefaultRowHeight(getFont()); \
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
			ListElements<Row> rows(m_listBodyRow);
			for (sl_size i = 0; i < rows.count; i++) {
				rows[i].height = height; \
			}
		}
		{
			ListElements<Row> rows(m_listHeaderRow);
			for (sl_size i = 0; i < rows.count; i++) {
				rows[i].height = height; \
			}
		}
		{
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

	void GridView::refreshContentWidth(UIUpdateMode mode)
	{
		if (SLIB_UI_UPDATE_MODE_IS_INIT(mode)) {
			return;
		}
		ListElements<Column> columns(m_columns);
		sl_ui_len total = 0;
		for (sl_size i = 0; i < columns.count; i++) {
			total += FixColumnWidth(columns[i].width);
		}
		setContentWidth(total, mode);
	}

	void GridView::refreshContentHeight(UIUpdateMode mode)
	{
		if (SLIB_UI_UPDATE_MODE_IS_INIT(mode)) {
			return;
		}
		sl_int64 total = getRecordHeight() * (sl_int64)m_nRecords + getHeaderHeight() + getFooterHeight();
		setContentHeight((sl_scroll_pos)total, mode);
	}

	GridView::DataFunction GridView::getDataFunction()
	{
		return m_recordData;
	}

	void GridView::setDataFunction(const DataFunction& func, UIUpdateMode mode)
	{
		m_recordData = func;
		ObjectLocker locker(this);
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
			ListElements<Column> columns(m_columns); \
			for (sl_size k = 0; k < columns.count; k++) { \
				Column& col = columns[k]; \
				if (iRow >= 0) { \
					SECTION##CellProp* prop = col.list##SECTION##Cell.getPointerAt(iRow); \
					if (prop) { \
						prop->NAME = NAME; \
					} \
				} else { \
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

#define DEFINE_SET_COLUMN_ATTR_SUB(...) \
	{ \
		ListElements<BodyCellProp> props(col.listBodyCell); \
		for (sl_size i = 0; i < props.count; i++) { \
			props[i].__VA_ARGS__; \
		} \
	} \
	{ \
		ListElements<HeaderCellProp> props(col.listHeaderCell); \
		for (sl_size i = 0; i < props.count; i++) { \
			props[i].__VA_ARGS__; \
		} \
	} \
	{ \
		ListElements<FooterCellProp> props(col.listFooterCell); \
		for (sl_size i = 0; i < props.count; i++) { \
			props[i].__VA_ARGS__; \
		} \
	}

#define DEFINE_SET_COLUMN_ATTR(FUNC, ARG, NAME) \
	void GridView::setColumn##FUNC(sl_int32 iCol, ARG NAME, UIUpdateMode mode) \
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
			ListElements<Column> columns(m_columns); \
			for (sl_size k = 0; k < columns.count; k++) { \
				Column& col = columns[k]; \
				DEFINE_SET_COLUMN_ATTR_SUB(NAME = NAME) \
			} \
			if (SLIB_UI_UPDATE_MODE_IS_INIT(mode)) { \
				return; \
			} \
			_invalidateBodyCells(); \
			_invalidateFixedCells(); \
			invalidate(mode); \
		} \
	}

#define DEFINE_GET_SET_CELL_ATTR(FUNC, RET, ARG, NAME, DEF) \
	DEFINE_GET_SET_CELL_ATTR_SUB(Body, FUNC, RET, ARG, NAME, DEF) \
	DEFINE_GET_SET_CELL_ATTR_SUB(Header, FUNC, RET, ARG, NAME, DEF) \
	DEFINE_GET_SET_CELL_ATTR_SUB(Footer, FUNC, RET, ARG, NAME, DEF) \
	DEFINE_SET_COLUMN_ATTR(FUNC, ARG, NAME)

	DEFINE_GET_SET_CELL_ATTR(Creator, GridView::CellCreator, const CellCreator&, creator, sl_null)
	DEFINE_GET_SET_CELL_ATTR(Text, String, const String&, text, sl_null)
	DEFINE_GET_SET_CELL_ATTR(Font, Ref<Font>, const Ref<Font>&, font, sl_null)
	DEFINE_GET_SET_CELL_ATTR(MultiLine, MultiLineMode, MultiLineMode, multiLineMode, MultiLineMode::Single)
	DEFINE_GET_SET_CELL_ATTR(Ellipsize, EllipsizeMode, EllipsizeMode, ellipsizeMode, EllipsizeMode::None)
	DEFINE_GET_SET_CELL_ATTR(LineCount, sl_uint32, sl_uint32, lineCount, 0)
	DEFINE_GET_SET_CELL_ATTR(Alignment, Alignment, const Alignment&, align, 0)

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
					ListElements<SECTION##CellProp> props(col->list##SECTION##Cell); \
					for (sl_size i = 0; i < props.count; i++) { \
						props[i].NAME##s.set(state, NAME); \
					} \
					invalidate(mode); \
				} \
			} \
		} else { \
			ListElements<Column> columns(m_columns); \
			for (sl_size k = 0; k < columns.count; k++) { \
				Column& col = columns[k]; \
				if (iRow >= 0) { \
					SECTION##CellProp* prop = col.list##SECTION##Cell.getPointerAt(iRow); \
					if (prop) { \
						prop->NAME##s.set(state, NAME); \
					} \
				} else { \
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
	void GridView::setColumn##FUNC(sl_int32 iCol, ARG NAME, ViewState state, UIUpdateMode mode) \
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
			invalidate(mode); \
		} \
	}

#define DEFINE_GET_SET_CELL_DRAWING_ATTR(FUNC, RET, ARG, NAME, DEF) \
	DEFINE_GET_SET_CELL_DRAWING_ATTR_SUB(Body, FUNC, RET, ARG, NAME, DEF) \
	DEFINE_GET_SET_CELL_DRAWING_ATTR_SUB(Header, FUNC, RET, ARG, NAME, DEF) \
	DEFINE_GET_SET_CELL_DRAWING_ATTR_SUB(Footer, FUNC, RET, ARG, NAME, DEF) \
	DEFINE_SET_COLUMN_DRAWING_ATTR(FUNC, ARG, NAME)

	DEFINE_GET_SET_CELL_DRAWING_ATTR(Background, Ref<Drawable>, const Ref<Drawable>&, background, sl_null)

#define DEFINE_GET_SET_BACKGROUND(SECTION) \
	Ref<Drawable> GridView::get##SECTION##Background() \
	{ \
		return m_background##SECTION; \
	} \
	void GridView::set##SECTION##Background(const Ref<Drawable>& drawable, UIUpdateMode mode) \
	{ \
		m_background##SECTION = drawable; \
		invalidate(mode); \
	}

	DEFINE_GET_SET_BACKGROUND(Body)
	DEFINE_GET_SET_BACKGROUND(Header)
	DEFINE_GET_SET_BACKGROUND(Footer)

	DEFINE_GET_SET_CELL_DRAWING_ATTR(TextColor, Color, const Color&, textColor, Color::zero())

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

	sl_int64 GridView::getSelectedRecord()
	{
		return m_locationSelected.record;
	}

	sl_int32 GridView::getSelectedRow()
	{
		return m_locationSelected.row;
	}

	sl_int32 GridView::getSelectedColumn()
	{
		return m_locationSelected.column;
	}

	void GridView::selectCell(sl_uint32 row, sl_uint32 column, sl_uint64 record, UIUpdateMode mode)
	{
		m_locationSelected.record = record;
		m_locationSelected.row = row;
		m_locationSelected.column = column;
		invalidate(mode);
	}

	void GridView::selectRecord(sl_uint64 record, UIUpdateMode mode)
	{
		selectCell(-1, -1, record, mode);
	}

	void GridView::selectRow(sl_uint32 row, sl_uint64 record, UIUpdateMode mode)
	{
		selectCell(row, -1, record, mode);
	}

	void GridView::selectColumn(sl_uint32 column, UIUpdateMode mode)
	{
		selectCell(-1, column, -1, mode);
	}

	void GridView::selectNone(UIUpdateMode mode)
	{
		m_locationSelected.row = -1;
		m_locationSelected.column = -1;
		m_locationSelected.record = -1;
		invalidate(mode);
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
				sl_ui_len h = _get##SECTION##RowHeight(rows[i]); \
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
			sl_ui_len w = FixColumnWidth(columns[i].width);
			if (x < pos + w) {
				return i;
			}
			pos += w;
		}
		sl_ui_len widthLeft = pos;
		pos = widthView;
		for (i = 0; i < nRight; i++) {
			sl_uint32 k = nColumns - 1 - i;
			sl_ui_len w = FixColumnWidth(columns[k].width);
			if (pos - w <= x) {
				return k;
			}
			pos -= w;
		}
		nColumns -= nRight;
		x += (sl_ui_len)(getScrollX()) - widthLeft;
		pos = 0;
		for (i = nLeft; i < nColumns; i++) {
			sl_ui_len w = FixColumnWidth(columns[i].width);
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

	ViewState GridView::getCellState(Cell* cell)
	{
		ViewState state;
		if (m_locationHover.match(cell)) {
			if (isPressedState()) {
				state = ViewState::Pressed;
			} else {
				state = ViewState::Hover;
			}
		} else {
			state = ViewState::Normal;
		}
		if (m_locationSelected.match(cell)) {
			return (ViewState)((int)state + (int)(ViewState::Selected));
		} else {
			return state;
		}
	}

	SLIB_DEFINE_EVENT_HANDLER(GridView, ClickBody, UIEvent*, GridView::CellEventParam&)

	void GridView::dispatchClickBody(UIEvent* ev, CellEventParam& param)
	{
		SLIB_INVOKE_EVENT_HANDLER(ClickBody, ev, param)
		if (ev->isPreventedDefault()) {
			return;
		}
		if (m_locationSelected.record != param.record || m_locationSelected.row != param.row || m_locationSelected.column != param.column) {
			m_locationSelected.record = param.record;
			m_locationSelected.row = param.row;
			m_locationSelected.column = param.column;
			dispatchSelectCell(ev, param);
		}
	}

#define DEFINE_ON_CLICK(SECTION) \
	SLIB_DEFINE_EVENT_HANDLER(GridView, Click##SECTION, UIEvent*, GridView::CellEventParam&) \
	void GridView::dispatchClick##SECTION(UIEvent* ev, CellEventParam& param) \
	{ \
		SLIB_INVOKE_EVENT_HANDLER(Click##SECTION, ev, param) \
	}

	DEFINE_ON_CLICK(Header)
	DEFINE_ON_CLICK(Footer)

#define DEFINE_ON_RIGHT_BUTTON_CLICK(SECTION) \
	SLIB_DEFINE_EVENT_HANDLER(GridView, RightButtonClick##SECTION, UIEvent*, GridView::CellEventParam&) \
	void GridView::dispatchRightButtonClick##SECTION(UIEvent* ev, CellEventParam& param) \
	{ \
		SLIB_INVOKE_EVENT_HANDLER(RightButtonClick##SECTION, ev, param) \
	}

	DEFINE_ON_RIGHT_BUTTON_CLICK(Body)
	DEFINE_ON_RIGHT_BUTTON_CLICK(Header)
	DEFINE_ON_RIGHT_BUTTON_CLICK(Footer)

#define DEFINE_ON_DOUBLE_CLICK(SECTION) \
	SLIB_DEFINE_EVENT_HANDLER(GridView, DoubleClick##SECTION, UIEvent*, GridView::CellEventParam&) \
	void GridView::dispatchDoubleClick##SECTION(UIEvent* ev, CellEventParam& param) \
	{ \
		SLIB_INVOKE_EVENT_HANDLER(DoubleClick##SECTION, ev, param) \
	}

	DEFINE_ON_DOUBLE_CLICK(Body)
	DEFINE_ON_DOUBLE_CLICK(Header)
	DEFINE_ON_DOUBLE_CLICK(Footer)

	SLIB_DEFINE_EVENT_HANDLER(GridView, SelectCell, UIEvent*, GridView::CellEventParam&)

	void GridView::dispatchSelectCell(UIEvent* ev, CellEventParam& param)
	{
		SLIB_INVOKE_EVENT_HANDLER(SelectCell, ev, param)
	}
	
	void GridView::onDraw(Canvas* canvas)
	{
		ObjectLocker lock(this);
		ListElements<Column> columns(m_columns);
		sl_uint32 nColumns = (sl_uint32)(columns.count);
		sl_uint32 nLeft = m_nLeftColumns;
		sl_uint32 nRight = m_nRightColumns;
		FixLeftRightColumnCount(nColumns, nLeft, nRight);
		sl_uint32 iStartMidColumn = nLeft;
		sl_ui_pos xStartMidColumn = -(sl_ui_len)(getScrollX());
		{
			sl_uint32 iRight = nColumns - nRight;
			for (sl_uint32 i = nLeft; i < iRight; i++) {
				sl_ui_len w = FixColumnWidth(columns[i].width);
				if (0 < xStartMidColumn + w) {
					iStartMidColumn = i;
					break;
				}
				xStartMidColumn += w;
			}
		}
		
		sl_ui_len heightHeader = getHeaderHeight();
		_drawHeader(canvas, 0, heightHeader, columns.data, nColumns, nLeft, nRight, iStartMidColumn, xStartMidColumn);
		sl_ui_len heightView = getHeight();
		sl_ui_len yFooter = heightView - getFooterHeight();
		_drawFooter(canvas, yFooter, heightView, columns.data, nColumns, nLeft, nRight, iStartMidColumn, xStartMidColumn);
		_drawRecords(canvas, heightHeader, yFooter, columns.data, nColumns, nLeft, nRight, iStartMidColumn, xStartMidColumn);
	}

	void GridView::_drawRecords(Canvas* canvas, sl_ui_len top, sl_ui_len bottom, Column* columns, sl_uint32 nColumns, sl_uint32 nLeft, sl_uint32 nRight, sl_uint32 iStartMidColumn, sl_ui_pos xStartMidColumn)
	{
		ListElements<Row> rows(m_listBodyRow);
		sl_size nCells = rows.count * nColumns;
		if (!nCells) {
			return;
		}
		sl_ui_len widthView = getWidth();
		Ref<Drawable> background = m_backgroundBody;
		if (background.isNotNull()) {
			canvas->draw(UIRect(0, top, widthView, bottom), background);
		}
		_prepareBodyCells(columns, nColumns);
		sl_ui_len heightRecord = getRecordHeight();
		if (heightRecord <= 0) {
			return;
		}
		sl_int64 sy = (sl_int64)(getScrollY());
		sl_uint64 iRecord = Math::max(sy / heightRecord, (sl_int64)0);
		sl_ui_pos yRecord = (sl_ui_pos)((sl_int64)iRecord * heightRecord - sy) + top;
		sl_uint32 nRecords = Math::clamp((sl_int32)((bottom - yRecord + heightRecord - 1) / heightRecord), (sl_int32)0, (sl_int32)MAX_RECORDS_PER_SCREEN);
		Function<Variant(sl_uint64 record)> recordDataFunc(m_recordData);
		HashMap< sl_uint64, List< Ref<Cell> > > cache = Move(m_mapRecordCache);
		HashMap< sl_uint64, List< Ref<Cell> > > map;
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
			sl_ui_pos xLeft = 0;
			{
				for (sl_uint32 iCol = 0; iCol < nLeft; iCol++) {
					Column& col = columns[iCol];
					_drawBodyColumn(canvas, xLeft, top, col, iCol, rows.data, (sl_uint32)(rows.count), iRecord, recordData, cells);
					xLeft += col.width;
				}
			}
			sl_uint32 iRight = nColumns - nRight;
			sl_ui_pos xRight = widthView;
			if (nRight) {
				sl_uint32 iCol = nColumns - 1;
				for (;;) {
					Column& col = columns[iCol];
					xRight -= col.width;
					_drawBodyColumn(canvas, xRight, top, col, iCol, rows.data, (sl_uint32)(rows.count), iRecord, recordData, cells);
					if (iCol > iRight) {
						iCol--;
					} else {
						break;
					}
				}
			}
			{
				sl_ui_len xCell = xLeft + xStartMidColumn;
				for (sl_uint32 iCol = iStartMidColumn; iCol < iRight && xCell < xRight; iCol++) {
					Column& col = columns[iCol];
					_drawBodyColumn(canvas, xCell, top, col, iCol, rows.data, (sl_uint32)(rows.count), iRecord, recordData, cells);
					xCell += col.width;
				}
			}
			map.put_NoLock(iRecord, Move(cells));
			top += heightRecord;
			iRecord++;
		}
		m_mapRecordCache = Move(map);
	}

#define DEFINE_DRAW_FIXED(SECTION) \
	void GridView::_draw##SECTION(Canvas* canvas, sl_ui_len top, sl_ui_len bottom, Column* columns, sl_uint32 nColumns, sl_uint32 nLeft, sl_uint32 nRight, sl_uint32 iStartMidColumn, sl_ui_pos xStartMidColumn) \
	{ \
		sl_ui_len widthView = getWidth(); \
		Ref<Drawable> background = m_background##SECTION; \
		if (background.isNotNull()) { \
			canvas->draw(UIRect(0, top, widthView, bottom), background); \
		} \
		_prepare##SECTION##Cells(columns, nColumns); \
		ListElements<Row> rows(m_list##SECTION##Row); \
		sl_ui_pos xLeft = 0; \
		{ \
			for (sl_uint32 iCol = 0; iCol < nLeft; iCol++) { \
				Column& col = columns[iCol]; \
				_draw##SECTION##Column(canvas, xLeft, top, col, iCol, rows.data, (sl_uint32)(rows.count)); \
				xLeft += col.width; \
			} \
		} \
		sl_uint32 iRight = nColumns - nRight; \
		sl_ui_pos xRight = widthView; \
		if (nRight) { \
			sl_uint32 iCol = nColumns - 1; \
			for (;;) { \
				Column& col = columns[iCol]; \
				xRight -= col.width; \
				_draw##SECTION##Column(canvas, xRight, top, col, iCol, rows.data, (sl_uint32)(rows.count)); \
				if (iCol > iRight) { \
					iCol--; \
				} else { \
					break; \
				} \
			} \
		} \
		{ \
			sl_ui_pos xCell = xLeft + xStartMidColumn; \
			for (sl_uint32 iCol = iStartMidColumn; iCol < iRight && xCell < xRight; iCol++) { \
				Column& col = columns[iCol]; \
				_draw##SECTION##Column(canvas, xCell, top, col, iCol, rows.data, (sl_uint32)(rows.count)); \
				xCell += col.width; \
			} \
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
			y += rows[iRow].height;
			iCell++;
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
			y += rows[iRow].height; \
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
				prop.width = col.width; \
				prop.height = rows[iRow].height; \
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
					prop.height += rows[i].height; \
				} \
				for (i = iCol + 1; i < iColEnd; i++) { \
					Column& spanCol = columns[i]; \
					prop.width += spanCol.width; \
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
		if (prop.creator.isNull()) {
			return sl_null;
		}
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
		if (cell.isNotNull()) {
			(CellParam&)(*cell) = Move(param);
			cell->onInit();
		}
		return cell;
	}

	GridView::Cell* GridView::_getFixedCell(FixedCellProp& prop, RecordIndex iRecord, sl_uint32 iRow, sl_uint32 iCol)
	{
		if (prop.creator.isNull()) {
			return sl_null;
		}
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
		if (prop.cell.isNotNull()) {
			(CellParam&)(*(prop.cell)) = Move(param);
			prop.cell->onInit();
		}
		return prop.cell.get();
	}

	void GridView::onClickEvent(UIEvent* ev)
	{
		CellEventParam param;
		if (_prepareMouseEventParam(ev, param)) {
			if (param.record == HEADER) {
				dispatchClickHeader(ev, param);
			} else if (param.record == FOOTER) {
				dispatchClickFooter(ev, param);
			} else if (param.record >= 0) {
				dispatchClickBody(ev, param);
			}
		}
	}

	void GridView::onMouseEvent(UIEvent* ev)
	{
		UIAction action = ev->getAction();
		switch (action) {
			case UIAction::LeftButtonDown:
				{
					CellEventParam param;
					if (_prepareMouseEventParam(ev, param)) {
						if (param.record == HEADER) {
							dispatchRightButtonClickHeader(ev, param);
						} else if (param.record == FOOTER) {
							dispatchRightButtonClickHeader(ev, param);
						} else if (param.record >= 0) {
							dispatchRightButtonClickBody(ev, param);
						}
					}
				}
				break;
			case UIAction::LeftButtonDoubleClick:
				{
					CellEventParam param;
					if (_prepareMouseEventParam(ev, param)) {
						if (param.record == HEADER) {
							dispatchDoubleClickHeader(ev, param);
						} else if (param.record == FOOTER) {
							dispatchDoubleClickHeader(ev, param);
						} else if (param.record >= 0) {
							dispatchDoubleClickBody(ev, param);
						}
					}
				}
				break;
			case UIAction::MouseMove:
				{
					sl_uint32 row = 0;
					sl_uint32 col = 0;
					sl_int64 record = -1;
					if (getCellAt((sl_ui_pos)(ev->getX()), (sl_ui_pos)(ev->getY()), &row, &col, &record)) {
						if (record < 0) {
							row = 0;
							col = 0;
							record = -1;
						}
					}
					if (m_locationHover.record != record || m_locationHover.row != row || m_locationHover.column != col) {
						m_locationHover.record = record;
						m_locationHover.row = row;
						m_locationHover.column = col;
						invalidate();
					}
				}
				break;
			case UIAction::MouseLeave:
				{
					m_locationHover.record = -1;
					m_locationHover.row = -1;
					m_locationHover.column = -1;
					invalidate();
				}
				break;
			default:
				break;
		}
	}

	sl_bool GridView::_prepareMouseEventParam(UIEvent* ev, CellEventParam& param)
	{
		if (ev->isMouseEvent()) {
			if (getCellAt((sl_ui_pos)(ev->getX()), (sl_ui_pos)(ev->getY()), &(param.row), &(param.column), &(param.record))) {
				if (param.record >= 0) {
					param.recordData = m_recordData(param.record);
				}
				return sl_true;
			}
		}
		return sl_false;
	}

	void GridView::onKeyEvent(UIEvent* ev)
	{
	}

	void GridView::onUpdateFont(const Ref<Font>& font)
	{
		ObjectLocker lock(this);
		_invalidateBodyCells();
		_invalidateFixedCells();
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

	void GridView::_invalidateFixedCells()
	{
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
