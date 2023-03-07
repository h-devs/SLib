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

#ifndef CHECKHEADER_SLIB_UI_GRID_VIEW
#define CHECKHEADER_SLIB_UI_GRID_VIEW

#include "view.h"

#include "../graphics/text.h"
#include "../core/variant.h"

namespace slib
{

	class SLIB_EXPORT GridView : public View
	{
		SLIB_DECLARE_OBJECT

	public:
		enum class CellState
		{
			Normal = 0,
			Hover = 1,
			Selected = 2,
			Count = 3
		};

		class DrawCellParam : public TextBox::DrawParam
		{
		public:
			DrawCellParam();
			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(DrawCellParam)
		};

		class CellAttribute
		{
		public:
			String text;
			String format;
			Ref<Font> font;
			MultiLineMode multiLineMode;
			EllipsizeMode ellipsizeMode;
			sl_uint32 lineCount;
			Alignment align;
			sl_uint32 colspan;
			sl_uint32 rowspan;

			Ref<Drawable> background[(sl_uint32)(CellState::Count)];
			Color textColor[(sl_uint32)(CellState::Count)];

			sl_ui_len width;
			sl_ui_len height;

		public:
			CellAttribute();
			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(CellAttribute)
		};

		// Special values for `RecordIndex`
		enum {
			HEADER = -1,
			FOOTER = -2,
			OUTSIDE = -3
		};
		typedef sl_int64 RecordIndex;

		class CellParam : public CellAttribute
		{
		public:
			GridView* view;
			sl_uint32 row;
			sl_uint32 column;
			RecordIndex record;
			Variant recordData;

		public:
			CellParam();
			CellParam(const CellAttribute& other);
			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(CellParam)
		};

		class Cell : public Referable, public CellParam
		{
		public:
			typedef DrawCellParam DrawParam;

		public:
			Cell();
			~Cell();

		public:
			virtual void onInit();
			virtual void onDraw(Canvas*, DrawParam&);
			virtual void onEvent(UIEvent*);
		};

		typedef Function<Variant(sl_uint64 record)> DataFunction;
		typedef Function<Ref<Cell>(CellParam&)> CellCreator;

		class TextCell : public Cell
		{
		public:
			TextCell();
			~TextCell();

		public:
			static const CellCreator& creator();

		public:
			void onInit() override;
			void onDraw(Canvas*, DrawParam&) override;

			virtual void onPrepareTextBox(TextBoxParam& param);

		private:
			TextBox m_textBox;
		};

		class HyperTextCell : public TextCell
		{
		public:
			HyperTextCell();
			~HyperTextCell();

		public:
			static const CellCreator& creator();

		public:
			void onPrepareTextBox(TextBoxParam& param) override;
		};

		class NumeroCell : public TextCell
		{
		public:
			NumeroCell(sl_int64 start = 1);
			~NumeroCell();

		public:
			static const CellCreator& creator();
			static CellCreator creator(sl_int64 start);

		public:
			void onPrepareTextBox(TextBoxParam& param) override;

		private:
			sl_int64 m_start;
		};

	public:
		GridView();

		~GridView();

	public:
		sl_uint32 getColumnCount();
		sl_bool setColumnCount(sl_uint32 count, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_uint32 getLeftColumnCount();
		void setLeftColumnCount(sl_uint32 count, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_uint32 getRightColumnCount();
		void setRightColumnCount(sl_uint32 count, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_uint64 getRecordCount();
		void setRecordCount(sl_uint64 count, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_uint32 getRowCount();
		sl_uint32 getHeaderRowCount();
		sl_uint32 getFooterRowCount();

		sl_bool setRowCount(sl_uint32 count, UIUpdateMode mode = UIUpdateMode::Redraw);
		sl_bool setHeaderRowCount(sl_uint32 count, UIUpdateMode mode = UIUpdateMode::Redraw);
		sl_bool setFooterRowCount(sl_uint32 count, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_ui_len getColumnWidth(sl_uint32 index);
		sl_bool setColumnWidth(sl_uint32 index, sl_ui_len width, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_ui_len getRecordHeight();
		sl_ui_len getHeaderHeight();
		sl_ui_len getFooterHeight();
		sl_uint64 getBodyHeight();

		sl_ui_len getRowHeight(sl_uint32 index);
		sl_ui_len getHeaderRowHeight(sl_uint32 index);
		sl_ui_len getFooterRowHeight(sl_uint32 index);

		void setRowHeight(sl_uint32 index, sl_ui_len height, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setHeaderRowHeight(sl_uint32 index, sl_ui_len height, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setFooterRowHeight(sl_uint32 index, sl_ui_len height, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setRowHeight(sl_ui_len height, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setBodyRowHeight(sl_ui_len height, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setHeaderRowHeight(sl_ui_len height, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setFooterRowHeight(sl_ui_len height, UIUpdateMode mode = UIUpdateMode::Redraw);

		void refreshContentWidth(UIUpdateMode mode = UIUpdateMode::Redraw);
		void refreshContentHeight(UIUpdateMode mode = UIUpdateMode::Redraw);

		DataFunction getDataFunction();
		void setDataFunction(const DataFunction& func, UIUpdateMode mode);

		CellCreator getCellCreator(sl_uint32 row, sl_uint32 column);
		CellCreator getHeaderCreator(sl_uint32 row, sl_uint32 column);
		CellCreator getFooterCreator(sl_uint32 row, sl_uint32 column);

		void setCellCreator(sl_int32 row, sl_int32 column, const CellCreator& creator, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setHeaderCreator(sl_int32 row, sl_int32 column, const CellCreator& creator, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setFooterCreator(sl_int32 row, sl_int32 column, const CellCreator& creator, UIUpdateMode mode = UIUpdateMode::Redraw);

		String getCellText(sl_uint32 row, sl_uint32 column);
		String getHeaderText(sl_uint32 row, sl_uint32 column);
		String getFooterText(sl_uint32 row, sl_uint32 column);

		void setCellText(sl_int32 row, sl_int32 column, const String& text, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setHeaderText(sl_int32 row, sl_int32 column, const String& text, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setFooterText(sl_int32 row, sl_int32 column, const String& text, UIUpdateMode mode = UIUpdateMode::Redraw);

		String getCellFormat(sl_uint32 row, sl_uint32 column);
		void setCellFormat(sl_int32 row, sl_int32 column, const String& format, UIUpdateMode mode = UIUpdateMode::Redraw);

		Ref<Font> getCellFont(sl_uint32 row, sl_uint32 column);
		Ref<Font> getHeaderFont(sl_uint32 row, sl_uint32 column);
		Ref<Font> getFooterFont(sl_uint32 row, sl_uint32 column);

		void setCellFont(sl_int32 row, sl_int32 column, const Ref<Font>& font, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setHeaderFont(sl_int32 row, sl_int32 column, const Ref<Font>& font, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setFooterFont(sl_int32 row, sl_int32 column, const Ref<Font>& font, UIUpdateMode mode = UIUpdateMode::Redraw);

		MultiLineMode getCellMultiLine(sl_uint32 row, sl_uint32 column);
		MultiLineMode getHeaderMultiLine(sl_uint32 row, sl_uint32 column);
		MultiLineMode getFooterMultiLine(sl_uint32 row, sl_uint32 column);

		void setCellMultiLine(sl_int32 row, sl_int32 column, MultiLineMode multiLineMode, UIUpdateMode updateMode = UIUpdateMode::Redraw);
		void setHeaderMultiLine(sl_int32 row, sl_int32 column, MultiLineMode multiLineMode, UIUpdateMode updateMode = UIUpdateMode::Redraw);
		void setFooterMultiLine(sl_int32 row, sl_int32 column, MultiLineMode multiLineMode, UIUpdateMode updateMode = UIUpdateMode::Redraw);

		EllipsizeMode getCellEllipsize(sl_uint32 row, sl_uint32 column);
		EllipsizeMode getHeaderEllipsize(sl_uint32 row, sl_uint32 column);
		EllipsizeMode getFooterEllipsize(sl_uint32 row, sl_uint32 column);

		void setCellEllipsize(sl_int32 row, sl_int32 column, EllipsizeMode ellipsizeMode, UIUpdateMode updateMode = UIUpdateMode::Redraw);
		void setHeaderEllipsize(sl_int32 row, sl_int32 column, EllipsizeMode ellipsizeMode, UIUpdateMode updateMode = UIUpdateMode::Redraw);
		void setFooterEllipsize(sl_int32 row, sl_int32 column, EllipsizeMode ellipsizeMode, UIUpdateMode updateMode = UIUpdateMode::Redraw);

		sl_uint32 getCellLineCount(sl_uint32 row, sl_uint32 column);
		sl_uint32 getHeaderLineCount(sl_uint32 row, sl_uint32 column);
		sl_uint32 getFooterLineCount(sl_uint32 row, sl_uint32 column);

		void setCellLineCount(sl_int32 row, sl_int32 column, sl_uint32 lineCount, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setHeaderLineCount(sl_int32 row, sl_int32 column, sl_uint32 lineCount, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setFooterLineCount(sl_int32 row, sl_int32 column, sl_uint32 lineCount, UIUpdateMode mode = UIUpdateMode::Redraw);

		Alignment getCellAlignment(sl_uint32 row, sl_uint32 column);
		Alignment getHeaderAlignment(sl_uint32 row, sl_uint32 column);
		Alignment getFooterAlignment(sl_uint32 row, sl_uint32 column);

		void setCellAlignment(sl_int32 row, sl_int32 column, const Alignment& align, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setHeaderAlignment(sl_int32 row, sl_int32 column, const Alignment& align, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setFooterAlignment(sl_int32 row, sl_int32 column, const Alignment& align, UIUpdateMode mode = UIUpdateMode::Redraw);

		Ref<Drawable> getCellBackground(sl_uint32 row, sl_uint32 column, CellState state = CellState::Normal);
		Ref<Drawable> getHeaderBackground(sl_uint32 row, sl_uint32 column, CellState state = CellState::Normal);
		Ref<Drawable> getFooterBackground(sl_uint32 row, sl_uint32 column, CellState state = CellState::Normal);

		void setCellBackground(sl_int32 row, sl_int32 column, const Ref<Drawable>& drawable, CellState state = CellState::Normal, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setHeaderBackground(sl_int32 row, sl_int32 column, const Ref<Drawable>& drawable, CellState state = CellState::Normal, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setFooterBackground(sl_int32 row, sl_int32 column, const Ref<Drawable>& drawable, CellState state = CellState::Normal, UIUpdateMode mode = UIUpdateMode::Redraw);

		Ref<Drawable> getBodyBackground();
		Ref<Drawable> getHeaderBackground();
		Ref<Drawable> getFooterBackground();

		void setBodyBackground(const Ref<Drawable>& drawable, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setHeaderBackground(const Ref<Drawable>& drawable, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setFooterBackground(const Ref<Drawable>& drawable, UIUpdateMode mode = UIUpdateMode::Redraw);

		Color getCellTextColor(sl_uint32 row, sl_uint32 column, CellState state = CellState::Normal);
		Color getHeaderTextColor(sl_uint32 row, sl_uint32 column, CellState state = CellState::Normal);
		Color getFooterTextColor(sl_uint32 row, sl_uint32 column, CellState state = CellState::Normal);

		void setCellTextColor(sl_int32 row, sl_int32 column, const Color& color, CellState state = CellState::Normal, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setHeaderTextColor(sl_int32 row, sl_int32 column, const Color& color, CellState state = CellState::Normal, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setFooterTextColor(sl_int32 row, sl_int32 column, const Color& color, CellState state = CellState::Normal, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_uint32 getRowspan(sl_uint32 row, sl_uint32 column);
		sl_uint32 getHeaderRowspan(sl_uint32 row, sl_uint32 column);
		sl_uint32 getFooterRowspan(sl_uint32 row, sl_uint32 column);

		sl_uint32 getColspan(sl_uint32 row, sl_uint32 column);
		sl_uint32 getHeaderColspan(sl_uint32 row, sl_uint32 column);
		sl_uint32 getFooterColspan(sl_uint32 row, sl_uint32 column);

		void setRowspan(sl_uint32 row, sl_uint32 column, sl_uint32 span, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setHeaderRowspan(sl_uint32 row, sl_uint32 column, sl_uint32 span, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setFooterRowspan(sl_uint32 row, sl_uint32 column, sl_uint32 span, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setColspan(sl_uint32 row, sl_uint32 column, sl_uint32 span, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setHeaderColspan(sl_uint32 row, sl_uint32 column, sl_uint32 span, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setFooterColspan(sl_uint32 row, sl_uint32 column, sl_uint32 span, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setCellSpan(sl_uint32 row, sl_uint32 column, sl_uint32 rowspan, sl_uint32 colspan, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setHeaderSpan(sl_uint32 row, sl_uint32 column, sl_uint32 rowspan, sl_uint32 colspan, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setFooterSpan(sl_uint32 row, sl_uint32 column, sl_uint32 rowspan, sl_uint32 colspan, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_int64 getSelectedRecord();
		sl_int32 getSelectedRow();
		sl_int32 getSelectedColumn();

		void selectCell(sl_uint32 row, sl_uint32 column, sl_uint64 record = 0, UIUpdateMode mode = UIUpdateMode::Redraw);
		void selectRecord(sl_uint64 record, UIUpdateMode mode = UIUpdateMode::Redraw);
		void selectRow(sl_uint32 row, sl_uint64 record = 0, UIUpdateMode mode = UIUpdateMode::Redraw);
		void selectColumn(sl_uint32 column, UIUpdateMode mode = UIUpdateMode::Redraw);
		void selectNone(UIUpdateMode mode = UIUpdateMode::Redraw);

		RecordIndex getRecordAt(sl_ui_pos y, sl_int32* outRow = sl_null);

		sl_int32 getRowAt(sl_ui_pos y);
		sl_int32 getHeaderRowAt(sl_ui_pos y);
		sl_int32 getFooterRowAt(sl_ui_pos y);

		sl_int32 getColumnAt(sl_ui_pos x);

		sl_bool getCellAt(sl_ui_pos x, sl_ui_pos y, sl_uint32* outRow = sl_null, sl_uint32* outColumn = sl_null, RecordIndex * outRecord = sl_null);

	public:
		class CellEventParam
		{
		public:
			sl_uint32 row;
			sl_uint32 column;
			RecordIndex record;
			Variant recordData;

		public:
			CellEventParam();
			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(CellEventParam)
		};

		SLIB_DECLARE_EVENT_HANDLER(GridView, ClickCell, UIEvent*, CellEventParam&)
		SLIB_DECLARE_EVENT_HANDLER(GridView, ClickHeader, UIEvent*, CellEventParam&)
		SLIB_DECLARE_EVENT_HANDLER(GridView, ClickFooter, UIEvent*, CellEventParam&)

		SLIB_DECLARE_EVENT_HANDLER(GridView, RightButtonClickCell, UIEvent*, CellEventParam&)
		SLIB_DECLARE_EVENT_HANDLER(GridView, RightButtonClickHeader, UIEvent*, CellEventParam&)
		SLIB_DECLARE_EVENT_HANDLER(GridView, RightButtonClickFooter, UIEvent*, CellEventParam&)

		SLIB_DECLARE_EVENT_HANDLER(GridView, DoubleClickCell, UIEvent*, CellEventParam&)
		SLIB_DECLARE_EVENT_HANDLER(GridView, DoubleClickHeader, UIEvent*, CellEventParam&)
		SLIB_DECLARE_EVENT_HANDLER(GridView, DoubleClickFooter, UIEvent*, CellEventParam&)

		SLIB_DECLARE_EVENT_HANDLER(GridView, SelectCell, UIEvent*, CellEventParam&)

	protected:
		void onDraw(Canvas* canvas) override;
		void onClickEvent(UIEvent* ev) override;
		void onMouseEvent(UIEvent* ev) override;
		void onKeyEvent(UIEvent* ev) override;
		void onUpdateFont(const Ref<Font>& font) override;

	private:
		class CellProp : public CellAttribute
		{
		public:
			CellCreator creator;
			sl_bool flagCoveredX;
			sl_bool flagCoveredY;

		public:
			CellProp();
			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(CellProp)
		};

		class FixedCellProp : public CellProp
		{
		public:
			Ref<Cell> cell;
			sl_bool flagMadeCell;

		public:
			FixedCellProp();
			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(FixedCellProp)
		};

		typedef CellProp BodyCellProp;
		typedef FixedCellProp HeaderCellProp;
		typedef FixedCellProp FooterCellProp;

		class Column
		{
		public:
			sl_ui_len width;
			List<BodyCellProp> listBodyCell;
			List<HeaderCellProp> listHeaderCell;
			List<FooterCellProp> listFooterCell;

		public:
			Column();
			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(Column)
		};

		class Row
		{
		public:
			sl_ui_len height;

		public:
			Row();
			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(Row)
		};

		class Location
		{
		public:
			sl_int64 record;
			sl_int32 row;
			sl_int32 column;

		public:
			Location();
			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(Location)

		public:
			sl_bool match(Cell* cell);
		};

		sl_ui_len _getBodyRowHeight(Row& row);
		sl_ui_len _getHeaderRowHeight(Row& row);
		sl_ui_len _getFooterRowHeight(Row& row);

		CellProp* _getCellProp(RecordIndex record, sl_uint32 row, sl_uint32 column);
		BodyCellProp* _getBodyCellProp(sl_uint32 row, sl_uint32 col);
		HeaderCellProp* _getHeaderCellProp(sl_uint32 row, sl_uint32 col);
		FooterCellProp* _getFooterCellProp(sl_uint32 row, sl_uint32 col);

		sl_uint32 _getBodyRowAt(sl_ui_pos y);
		sl_uint32 _getHeaderRowAt(sl_ui_pos y);
		sl_uint32 _getFooterRowAt(sl_ui_pos y);

		RecordIndex _getRowAt(sl_int32* outRow, sl_ui_pos y, sl_bool flagRecord, sl_bool flagHeader, sl_bool flagFooter);

		void _drawRecords(Canvas* canvas, sl_ui_len top, sl_ui_len bottom, Column* columns, sl_uint32 nColumns, sl_uint32 nLeft, sl_uint32 nRight, sl_uint32 iStartMidColumn, sl_ui_pos xStartMidColumn);
		void _drawHeader(Canvas* canvas, sl_ui_len top, sl_ui_len bottom, Column* columns, sl_uint32 nColumns, sl_uint32 nLeft, sl_uint32 nRight, sl_uint32 iStartMidColumn, sl_ui_pos xStartMidColumn);
		void _drawFooter(Canvas* canvas, sl_ui_len top, sl_ui_len bottom, Column* columns, sl_uint32 nColumns, sl_uint32 nLeft, sl_uint32 nRight, sl_uint32 iStartMidColumn, sl_ui_pos xStartMidColumn);

		void _drawBodyColumn(Canvas* canvas, sl_ui_pos x, sl_ui_pos y, Column& column, sl_uint32 iColumn, Row* rows, sl_uint32 nRows, sl_uint64 iRecord, const Variant& recordData, List< Ref<Cell> >& cells);
		void _drawHeaderColumn(Canvas* canvas, sl_ui_pos x, sl_ui_pos y, Column& column, sl_uint32 iColumn, Row* rows, sl_uint32 nRows);
		void _drawFooterColumn(Canvas* canvas, sl_ui_pos x, sl_ui_pos y, Column& column, sl_uint32 iColumn, Row* rows, sl_uint32 nRows);

		void _drawCell(Canvas* canvas, sl_ui_pos x, sl_ui_pos y, Cell* cell);

		void _prepareBodyCells(Column* columns, sl_uint32 nColumns);
		void _prepareHeaderCells(Column* columns, sl_uint32 nColumns);
		void _prepareFooterCells(Column* columns, sl_uint32 nColumns);

		Ref<Cell> _createBodyCell(BodyCellProp& prop, RecordIndex iRecord, sl_uint32 iRow, sl_uint32 iCol, const Variant& recordData);

		Cell* _getFixedCell(FixedCellProp& prop, RecordIndex iRecord, sl_uint32 iRow, sl_uint32 iCol);

		sl_bool _prepareMouseEventParam(UIEvent* ev, CellEventParam& param);

		void _invalidateBodyCells();
		void _invalidateBodyCells(Column& column, sl_uint32 iCol);
		void _invalidateBodyCell(BodyCellProp& prop, sl_uint32 row, sl_uint32 column);

		void _invalidateFixedCells();

		void _invalidateHeaderCells();
		void _invalidateHeaderCells(Column& column, sl_uint32 iCol, sl_bool flagAllColumns = sl_false);
		void _invalidateHeaderCell(HeaderCellProp& prop, sl_uint32 row, sl_uint32 column);

		void _invalidateFooterCells();
		void _invalidateFooterCells(Column& column, sl_uint32 iCol, sl_bool flagAllColumns = sl_false);
		void _invalidateFooterCell(FooterCellProp& prop, sl_uint32 row, sl_uint32 column);

	private:
		CList<Column> m_columns;

		sl_uint64 m_nRecords;
		sl_uint32 m_nLeftColumns;
		sl_uint32 m_nRightColumns;

		CList<Row> m_listBodyRow;
		CList<Row> m_listHeaderRow;
		CList<Row> m_listFooterRow;
		sl_bool m_flagInvalidateBodyRows;
		sl_bool m_flagInvalidateHeaderRows;
		sl_bool m_flagInvalidateFooterRows;
		AtomicRef<Drawable> m_backgroundBody;
		AtomicRef<Drawable> m_backgroundHeader;
		AtomicRef<Drawable> m_backgroundFooter;

		Location m_locationHover;
		Location m_locationSelected;

		Atomic<DataFunction> m_recordData;

		HashMap< sl_uint64, List< Ref<Cell> > > m_mapRecordCache;

	};

}

#endif
