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
#include "view_state_map.h"

#include "../graphics/text.h"
#include "../core/variant.h"

namespace slib
{

	class SLIB_EXPORT GridView : public View
	{
		SLIB_DECLARE_OBJECT

	public:
		class DrawCellParam : public TextBox::DrawParam
		{
		public:
			DrawCellParam();
			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(DrawCellParam)
		};

		class Cell;
		typedef Function<String(Cell*)> TextFormatter;

		class CellAttribute
		{
		public:
			String field;
			String text;
			TextFormatter formatter;
			Ref<Font> font;
			MultiLineMode multiLineMode;
			EllipsizeMode ellipsizeMode;
			sl_uint32 lineCount;
			Alignment align;
			sl_bool flagSelectable;
			sl_bool flagEditable;

			ViewStateMap< Ref<Drawable> > backgrounds;
			ViewStateMap<Color> textColors;

			sl_uint32 colspan;
			sl_uint32 rowspan;

			sl_ui_len width;
			sl_ui_len height;

		public:
			CellAttribute();
			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(CellAttribute)

		};

		// Special values for `RecordIndex`
		enum {
			BODY = 0, // and positive values
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

		class Cell : public CRef, public CellParam
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
			virtual void onCopy();

		public:
			String getText();
			String getFormattedText();

		};

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
			void onCopy() override;

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

		class SortCell : public TextCell
		{
		public:
			SortCell();
			~SortCell();

		public:
			static const CellCreator& creator();

		public:
			void onDraw(Canvas*, DrawParam&) override;

			void onEvent(UIEvent*) override;

		protected:
			sl_bool m_flagSort;
			sl_bool m_flagAsc;
		};

		class Selection
		{
		public:
			sl_int64 record;
			sl_int32 row;
			sl_int32 column;

		public:
			Selection();
			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(Selection)

		public:
			sl_bool operator==(const Selection& other) const;

		public:
			sl_bool isNone() const;
			sl_bool match(RecordIndex record, sl_int32 row, sl_int32 column) const;
			sl_bool match(Cell* cell) const;

		};

	public:
		GridView();

		~GridView();

	protected:
		void init() override;

	public:
		class Column;
		class Row;

	public:
		sl_uint32 getColumnCount();
		sl_bool setColumnCount(sl_uint32 count, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_uint32 getLeftColumnCount();
		void setLeftColumnCount(sl_uint32 count, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_uint32 getRightColumnCount();
		void setRightColumnCount(sl_uint32 count, UIUpdateMode mode = UIUpdateMode::Redraw);

		Ref<Column> getColumn(sl_uint32 index);
		Ref<Column> addColumn(UIUpdateMode mode = UIUpdateMode::Redraw);
		Ref<Column> insertColumn(sl_uint32 index, UIUpdateMode mode = UIUpdateMode::Redraw);
		sl_bool removeColumn(sl_uint32 index, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_ui_len getColumnWidth(sl_uint32 index);
		void setColumnWidth(sl_uint32 index, sl_ui_len width, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setColumnWidth(sl_ui_len width, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_ui_len getMinimumColumnWidth(sl_uint32 index);
		void setMinimumColumnWidth(sl_uint32 index, sl_ui_len width, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setMinimumColumnWidth(sl_ui_len width, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_ui_len getMaximumColumnWidth(sl_uint32 index);
		void setMaximumColumnWidth(sl_uint32 index, sl_ui_len width, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setMaximumColumnWidth(sl_ui_len width, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_bool isColumnVisible(sl_uint32 index);
		void setColumnVisible(sl_uint32 index, sl_bool flagVisible = sl_true, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_bool isColumnResizable(sl_uint32 index);
		void setColumnResizable(sl_uint32 index, sl_bool flagResizable = sl_true);
		void setColumnResizable(sl_bool flagResizable = sl_true);

		sl_uint64 getRecordCount();
		void setRecordCount(sl_uint64 count, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_uint32 getBodyRowCount();
		sl_uint32 getHeaderRowCount();
		sl_uint32 getFooterRowCount();

		sl_bool setBodyRowCount(sl_uint32 count, UIUpdateMode mode = UIUpdateMode::Redraw);
		sl_bool setHeaderRowCount(sl_uint32 count, UIUpdateMode mode = UIUpdateMode::Redraw);
		sl_bool setFooterRowCount(sl_uint32 count, UIUpdateMode mode = UIUpdateMode::Redraw);

		Ref<Row> getBodyRow(sl_uint32 index);
		Ref<Row> getHeaderRow(sl_uint32 index);
		Ref<Row> getFooterRow(sl_uint32 index);

		Ref<Row> addBodyRow(UIUpdateMode mode = UIUpdateMode::Redraw);
		Ref<Row> addHeaderRow(UIUpdateMode mode = UIUpdateMode::Redraw);
		Ref<Row> addFooterRow(UIUpdateMode mode = UIUpdateMode::Redraw);

		Ref<Row> insertBodyRow(sl_uint32 index, UIUpdateMode mode = UIUpdateMode::Redraw);
		Ref<Row> insertHeaderRow(sl_uint32 index, UIUpdateMode mode = UIUpdateMode::Redraw);
		Ref<Row> insertFooterRow(sl_uint32 index, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_bool removeBodyRow(sl_uint32 index, UIUpdateMode mode = UIUpdateMode::Redraw);
		sl_bool removeHeaderRow(sl_uint32 index, UIUpdateMode mode = UIUpdateMode::Redraw);
		sl_bool removeFooterRow(sl_uint32 index, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_ui_len getRecordHeight();
		sl_ui_len getHeaderHeight();
		sl_ui_len getFooterHeight();
		sl_uint64 getBodyHeight();

		sl_ui_len getBodyRowHeight(sl_uint32 index);
		sl_ui_len getHeaderRowHeight(sl_uint32 index);
		sl_ui_len getFooterRowHeight(sl_uint32 index);

		void setBodyRowHeight(sl_uint32 index, sl_ui_len height, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setHeaderRowHeight(sl_uint32 index, sl_ui_len height, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setFooterRowHeight(sl_uint32 index, sl_ui_len height, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setRowHeight(sl_ui_len height, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setBodyRowHeight(sl_ui_len height, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setHeaderRowHeight(sl_ui_len height, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setFooterRowHeight(sl_ui_len height, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_bool isBodyRowVisible(sl_uint32 index);
		sl_bool isHeaderRowVisible(sl_uint32 index);
		sl_bool isFooterRowVisible(sl_uint32 index);

		void setBodyRowVisible(sl_uint32 index, sl_bool flagVisible, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setHeaderRowVisible(sl_uint32 index, sl_bool flagVisible, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setFooterRowVisible(sl_uint32 index, sl_bool flagVisible, UIUpdateMode mode = UIUpdateMode::Redraw);

		Ref<Pen> getBodyGrid();
		Ref<Pen> getHeaderGrid();
		Ref<Pen> getFooterGrid();
		Ref<Pen> getLeftGrid();
		Ref<Pen> getRightGrid();

		void setGrid(const Ref<Pen>& pen, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setGrid(const PenDesc& desc, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setBodyGrid(const Ref<Pen>& pen, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setBodyGrid(const PenDesc& desc, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setHeaderGrid(const Ref<Pen>& pen, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setHeaderGrid(const PenDesc& desc, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setFooterGrid(const Ref<Pen>& pen, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setFooterGrid(const PenDesc& desc, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setLeftGrid(const Ref<Pen>& pen, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setLeftGrid(const PenDesc& desc, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setRightGrid(const Ref<Pen>& pen, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setRightGrid(const PenDesc& desc, UIUpdateMode mode = UIUpdateMode::Redraw);

		Ref<Pen> getSelectionBorder();
		void setSelectionBorder(const Ref<Pen>& pen, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setSelectionBorder(const PenDesc& desc, UIUpdateMode mode = UIUpdateMode::Redraw);

		Ref<Drawable> getAscendingIcon();
		void setAscendingIcon(const Ref<Drawable>& icon);

		Ref<Drawable> getDescendingIcon();
		void setDescendingIcon(const Ref<Drawable>& icon);

		void refreshContentWidth(UIUpdateMode mode = UIUpdateMode::Redraw);
		void refreshContentHeight(UIUpdateMode mode = UIUpdateMode::Redraw);

		Function<Variant(sl_uint64 record)> getDataFunction();
		void setDataFunction(const Function<Variant(sl_uint64 record)>& func, UIUpdateMode mode = UIUpdateMode::Redraw);

		CellCreator getBodyCreator(sl_uint32 row, sl_uint32 column);
		CellCreator getHeaderCreator(sl_uint32 row, sl_uint32 column);
		CellCreator getFooterCreator(sl_uint32 row, sl_uint32 column);

		void setBodyCreator(sl_int32 row, sl_int32 column, const CellCreator& creator, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setHeaderCreator(sl_int32 row, sl_int32 column, const CellCreator& creator, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setFooterCreator(sl_int32 row, sl_int32 column, const CellCreator& creator, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setColumnCreator(sl_int32 column, const CellCreator& creator, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setCellCreator(const CellCreator& creator, UIUpdateMode mode = UIUpdateMode::Redraw);

		String getBodyField(sl_uint32 row, sl_uint32 column);
		String getHeaderField(sl_uint32 row, sl_uint32 column);
		String getFooterField(sl_uint32 row, sl_uint32 column);

		void setBodyField(sl_int32 row, sl_int32 column, const String& field, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setHeaderField(sl_int32 row, sl_int32 column, const String& field, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setFooterField(sl_int32 row, sl_int32 column, const String& field, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setColumnField(sl_int32 column, const String& field, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setCellField(const String& field, UIUpdateMode mode = UIUpdateMode::Redraw);

		String getBodyText(sl_uint32 row, sl_uint32 column);
		String getHeaderText(sl_uint32 row, sl_uint32 column);
		String getFooterText(sl_uint32 row, sl_uint32 column);

		void setBodyText(sl_int32 row, sl_int32 column, const String& text, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setHeaderText(sl_int32 row, sl_int32 column, const String& text, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setFooterText(sl_int32 row, sl_int32 column, const String& text, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setColumnText(sl_int32 column, const String& text, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setCellText(const String& text, UIUpdateMode mode = UIUpdateMode::Redraw);

		Function<String(Cell*)> getBodyTextFormatter(sl_uint32 row, sl_uint32 column);
		Function<String(Cell*)> getHeaderTextFormatter(sl_uint32 row, sl_uint32 column);
		Function<String(Cell*)> getFooterTextFormatter(sl_uint32 row, sl_uint32 column);

		void setBodyTextFormatter(sl_int32 row, sl_int32 column, const Function<String(Cell*)>& formatter, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setHeaderTextFormatter(sl_int32 row, sl_int32 column, const Function<String(Cell*)>& formatter, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setFooterTextFormatter(sl_int32 row, sl_int32 column, const Function<String(Cell*)>& formatter, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setColumnTextFormatter(sl_int32 column, const Function<String(Cell*)>& formatter, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setCellTextFormatter(const Function<String(Cell*)>& formatter, UIUpdateMode mode = UIUpdateMode::Redraw);

		Ref<Font> getBodyFont(sl_uint32 row, sl_uint32 column);
		Ref<Font> getHeaderFont(sl_uint32 row, sl_uint32 column);
		Ref<Font> getFooterFont(sl_uint32 row, sl_uint32 column);

		void setBodyFont(sl_int32 row, sl_int32 column, const Ref<Font>& font, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setHeaderFont(sl_int32 row, sl_int32 column, const Ref<Font>& font, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setFooterFont(sl_int32 row, sl_int32 column, const Ref<Font>& font, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setColumnFont(sl_int32 column, const Ref<Font>& font, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setCellFont(const Ref<Font>& font, UIUpdateMode mode = UIUpdateMode::Redraw);

		MultiLineMode getBodyMultiLine(sl_uint32 row, sl_uint32 column);
		MultiLineMode getHeaderMultiLine(sl_uint32 row, sl_uint32 column);
		MultiLineMode getFooterMultiLine(sl_uint32 row, sl_uint32 column);

		void setBodyMultiLine(sl_int32 row, sl_int32 column, MultiLineMode multiLineMode, UIUpdateMode updateMode = UIUpdateMode::Redraw);
		void setHeaderMultiLine(sl_int32 row, sl_int32 column, MultiLineMode multiLineMode, UIUpdateMode updateMode = UIUpdateMode::Redraw);
		void setFooterMultiLine(sl_int32 row, sl_int32 column, MultiLineMode multiLineMode, UIUpdateMode updateMode = UIUpdateMode::Redraw);
		void setColumnMultiLine(sl_int32 column, MultiLineMode multiLineMode, UIUpdateMode updateMode = UIUpdateMode::Redraw);
		void setCellMultiLine(MultiLineMode multiLineMode, UIUpdateMode updateMode = UIUpdateMode::Redraw);

		EllipsizeMode getBodyEllipsize(sl_uint32 row, sl_uint32 column);
		EllipsizeMode getHeaderEllipsize(sl_uint32 row, sl_uint32 column);
		EllipsizeMode getFooterEllipsize(sl_uint32 row, sl_uint32 column);

		void setBodyEllipsize(sl_int32 row, sl_int32 column, EllipsizeMode ellipsizeMode, UIUpdateMode updateMode = UIUpdateMode::Redraw);
		void setHeaderEllipsize(sl_int32 row, sl_int32 column, EllipsizeMode ellipsizeMode, UIUpdateMode updateMode = UIUpdateMode::Redraw);
		void setFooterEllipsize(sl_int32 row, sl_int32 column, EllipsizeMode ellipsizeMode, UIUpdateMode updateMode = UIUpdateMode::Redraw);
		void setColumnEllipsize(sl_int32 column, EllipsizeMode ellipsizeMode, UIUpdateMode updateMode = UIUpdateMode::Redraw);
		void setCellEllipsize(EllipsizeMode ellipsizeMode, UIUpdateMode updateMode = UIUpdateMode::Redraw);

		sl_uint32 getBodyLineCount(sl_uint32 row, sl_uint32 column);
		sl_uint32 getHeaderLineCount(sl_uint32 row, sl_uint32 column);
		sl_uint32 getFooterLineCount(sl_uint32 row, sl_uint32 column);

		void setBodyLineCount(sl_int32 row, sl_int32 column, sl_uint32 lineCount, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setHeaderLineCount(sl_int32 row, sl_int32 column, sl_uint32 lineCount, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setFooterLineCount(sl_int32 row, sl_int32 column, sl_uint32 lineCount, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setColumnLineCount(sl_int32 column, sl_uint32 lineCount, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setCellLineCount(sl_uint32 lineCount, UIUpdateMode mode = UIUpdateMode::Redraw);

		Alignment getBodyAlignment(sl_uint32 row, sl_uint32 column);
		Alignment getHeaderAlignment(sl_uint32 row, sl_uint32 column);
		Alignment getFooterAlignment(sl_uint32 row, sl_uint32 column);

		void setBodyAlignment(sl_int32 row, sl_int32 column, const Alignment& align, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setHeaderAlignment(sl_int32 row, sl_int32 column, const Alignment& align, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setFooterAlignment(sl_int32 row, sl_int32 column, const Alignment& align, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setColumnAlignment(sl_int32 column, const Alignment& align, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setCellAlignment(const Alignment& align, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_bool isBodySelectable(sl_uint32 row, sl_uint32 column);
		sl_bool isHeaderSelectable(sl_uint32 row, sl_uint32 column);
		sl_bool isFooterSelectable(sl_uint32 row, sl_uint32 column);

		void setBodySelectable(sl_int32 row, sl_int32 column, sl_bool flag);
		void setHeaderSelectable(sl_int32 row, sl_int32 column, sl_bool flag);
		void setFooterSelectable(sl_int32 row, sl_int32 column, sl_bool flag);
		void setColumnSelectable(sl_int32 column, sl_bool flag);
		void setCellSelectable(sl_bool flag);

		sl_bool isBodyEditable(sl_uint32 row, sl_uint32 column);
		sl_bool isHeaderEditable(sl_uint32 row, sl_uint32 column);
		sl_bool isFooterEditable(sl_uint32 row, sl_uint32 column);

		void setBodyEditable(sl_int32 row, sl_int32 column, sl_bool flag);
		void setHeaderEditable(sl_int32 row, sl_int32 column, sl_bool flag);
		void setFooterEditable(sl_int32 row, sl_int32 column, sl_bool flag);
		void setColumnEditable(sl_int32 column, sl_bool flag);
		void setCellEditable(sl_bool flag);

		Ref<Drawable> getBodyBackground(sl_uint32 row, sl_uint32 column, ViewState state = ViewState::Default);
		Ref<Drawable> getHeaderBackground(sl_uint32 row, sl_uint32 column, ViewState state = ViewState::Default);
		Ref<Drawable> getFooterBackground(sl_uint32 row, sl_uint32 column, ViewState state = ViewState::Default);

		void setBodyBackground(sl_int32 row, sl_int32 column, const Ref<Drawable>& drawable, ViewState state = ViewState::Default, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setHeaderBackground(sl_int32 row, sl_int32 column, const Ref<Drawable>& drawable, ViewState state = ViewState::Default, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setFooterBackground(sl_int32 row, sl_int32 column, const Ref<Drawable>& drawable, ViewState state = ViewState::Default, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setColumnBackground(sl_int32 column, const Ref<Drawable>& drawable, ViewState state = ViewState::Default, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setCellBackground(const Ref<Drawable>& drawable, ViewState state = ViewState::Default, UIUpdateMode mode = UIUpdateMode::Redraw);

		Color getBodyTextColor(sl_uint32 row, sl_uint32 column, ViewState state = ViewState::Default);
		Color getHeaderTextColor(sl_uint32 row, sl_uint32 column, ViewState state = ViewState::Default);
		Color getFooterTextColor(sl_uint32 row, sl_uint32 column, ViewState state = ViewState::Default);

		void setBodyTextColor(sl_int32 row, sl_int32 column, const Color& color, ViewState state = ViewState::Default, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setHeaderTextColor(sl_int32 row, sl_int32 column, const Color& color, ViewState state = ViewState::Default, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setFooterTextColor(sl_int32 row, sl_int32 column, const Color& color, ViewState state = ViewState::Default, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setColumnTextColor(sl_int32 column, const Color& color, ViewState state = ViewState::Default, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setCellTextColor(const Color& color, ViewState state = ViewState::Default, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_uint32 getBodyRowspan(sl_uint32 row, sl_uint32 column);
		sl_uint32 getHeaderRowspan(sl_uint32 row, sl_uint32 column);
		sl_uint32 getFooterRowspan(sl_uint32 row, sl_uint32 column);

		sl_uint32 getBodyColspan(sl_uint32 row, sl_uint32 column);
		sl_uint32 getHeaderColspan(sl_uint32 row, sl_uint32 column);
		sl_uint32 getFooterColspan(sl_uint32 row, sl_uint32 column);

		void setBodyRowspan(sl_uint32 row, sl_uint32 column, sl_uint32 span, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setHeaderRowspan(sl_uint32 row, sl_uint32 column, sl_uint32 span, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setFooterRowspan(sl_uint32 row, sl_uint32 column, sl_uint32 span, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setBodyColspan(sl_uint32 row, sl_uint32 column, sl_uint32 span, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setHeaderColspan(sl_uint32 row, sl_uint32 column, sl_uint32 span, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setFooterColspan(sl_uint32 row, sl_uint32 column, sl_uint32 span, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setBodySpan(sl_uint32 row, sl_uint32 column, sl_uint32 rowspan, sl_uint32 colspan, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setHeaderSpan(sl_uint32 row, sl_uint32 column, sl_uint32 rowspan, sl_uint32 colspan, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setFooterSpan(sl_uint32 row, sl_uint32 column, sl_uint32 rowspan, sl_uint32 colspan, UIUpdateMode mode = UIUpdateMode::Redraw);

		enum class SelectionMode
		{
			Cell = 0,
			Row = 1,
			Column = 2,
			Record = 3
		};

		SelectionMode getSelectionMode();
		void setSelectionMode(SelectionMode mode);

		Ref<Cell> getVisibleCell(RecordIndex record, sl_uint32 row, sl_uint32 column);

		sl_int64 getSelectedRecord();
		sl_int32 getSelectedRow();
		sl_int32 getSelectedColumn();

		void select(sl_int32 row, sl_int32 column, sl_int64 record = 0, UIUpdateMode mode = UIUpdateMode::Redraw);
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
		Ref<Cell> getVisibleCellAt(sl_ui_pos x, sl_ui_pos y);

		sl_bool getCellLocation(UIPoint& _out, RecordIndex record, sl_int32 row, sl_int32 column);
		sl_bool getCellFrame(UIRect& _out, RecordIndex record, sl_int32 row, sl_int32 column);

		ViewState getCellState(RecordIndex record, sl_int32 row, sl_int32 column);
		ViewState getCellState(Cell* cell);
		
	public:
		SLIB_DECLARE_EVENT_HANDLER(GridView, ClickCell, Cell*, UIEvent*)
		SLIB_DECLARE_EVENT_HANDLER(GridView, RightButtonClickCell, Cell*, UIEvent*)
		SLIB_DECLARE_EVENT_HANDLER(GridView, DoubleClickCell, Cell*, UIEvent*)

		SLIB_DECLARE_EVENT_HANDLER(GridView, Select, const Selection& selection, const Selection& former, UIEvent* /* nullable */)

	public:
		void onDraw(Canvas* canvas) override;
		void onClickEvent(UIEvent* ev) override;
		void onMouseEvent(UIEvent* ev) override;
		void onSetCursor(UIEvent* ev) override;
		void onKeyEvent(UIEvent* ev) override;
		void onResize(sl_ui_len width, sl_ui_len height) override;

	protected:
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

		public:
			void inheritFrom(const CellProp& other);
		};

		class BodyCellProp : public CellProp
		{
		public:
			HashMap< sl_uint64, Ref<Cell> > cells;
			HashMap< sl_uint64, Ref<Cell> > cache;

		public:
			BodyCellProp();
			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(BodyCellProp)
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

		typedef FixedCellProp HeaderCellProp;
		typedef FixedCellProp FooterCellProp;

	public:
		class Column : public CRef
		{
		public:
			Column(GridView* view);

			~Column();

		public:
			Ref<GridView> getView();
			sl_uint32 getIndex();

			sl_bool remove(UIUpdateMode mode = UIUpdateMode::Redraw);

			sl_ui_len getWidth();
			void setWidth(sl_ui_len width, UIUpdateMode mode = UIUpdateMode::Redraw);

			sl_ui_len getMinimumWidth();
			void setMinimumWidth(sl_ui_len width, UIUpdateMode mode = UIUpdateMode::Redraw);

			sl_ui_len getMaximumWidth();
			void setMaximumWidth(sl_ui_len width, UIUpdateMode mode = UIUpdateMode::Redraw);

			sl_bool isVisible();
			void setVisible(sl_bool flagVisible = sl_true, UIUpdateMode mode = UIUpdateMode::Redraw);

			sl_bool isResizable();
			void setResizable(sl_bool flagResizable = sl_true);

		private:
			void _invalidateLayout(UIUpdateMode mode);

		private:
			WeakRef<GridView> m_view;
			sl_int32 m_index;

			sl_ui_len m_width;
			sl_ui_len m_fixedWidth;
			sl_ui_len m_minWidth;
			sl_ui_len m_maxWidth;
			sl_bool m_flagVisible;
			sl_bool m_flagResizable;

			List<BodyCellProp> m_listBodyCell;
			List<HeaderCellProp> m_listHeaderCell;
			List<FooterCellProp> m_listFooterCell;

			CellProp m_defaultBodyProps;
			CellProp m_defaultHeaderProps;
			CellProp m_defaultFooterProps;

			friend class GridView;
		};

		class Row : public CRef
		{
		public:
			Row(GridView* view);

			~Row();

		public:
			Ref<GridView> getView();

			sl_bool isBody();
			sl_bool isHeader();
			sl_bool isFooter();

			sl_uint32 getIndex();

			sl_bool remove(UIUpdateMode mode = UIUpdateMode::Redraw);

			sl_ui_len getHeight();
			void setHeight(sl_ui_len height, UIUpdateMode mode = UIUpdateMode::Redraw);

			sl_bool isVisible();
			void setVisible(sl_bool flagVisible = sl_true, UIUpdateMode mode = UIUpdateMode::Redraw);

		private:
			void _invalidateLayout(UIUpdateMode mode);

		public:
			WeakRef<GridView> m_view;
			RecordIndex m_section;
			sl_int32 m_index;

			sl_ui_len m_height;
			sl_ui_len m_fixedHeight;
			sl_bool m_flagVisible;

			CellProp m_defaultProps;

			friend class GridView;
		};

	private:
		sl_bool _inheritColumn(Column* col);

		sl_ui_len _getDefaultRowHeight();

		CellProp* _getCellProp(RecordIndex section, sl_uint32 row, sl_uint32 column);
		BodyCellProp* _getBodyCellProp(sl_uint32 row, sl_uint32 col);
		HeaderCellProp* _getHeaderCellProp(sl_uint32 row, sl_uint32 col);
		FooterCellProp* _getFooterCellProp(sl_uint32 row, sl_uint32 col);

		sl_uint32 _getBodyRowAt(sl_ui_pos y);
		sl_uint32 _getHeaderRowAt(sl_ui_pos y);
		sl_uint32 _getFooterRowAt(sl_ui_pos y);

		RecordIndex _getRowAt(sl_int32* outRow, sl_ui_pos y, sl_bool flagRecord, sl_bool flagHeader, sl_bool flagFooter);
		sl_bool _fixCellAddress(RecordIndex record, sl_uint32 row, sl_uint32* outRow, sl_uint32 col, sl_uint32* outCol);
		void _fixSelection(Selection& sel);

		void _select(const Selection& selection, UIEvent* ev, UIUpdateMode mode = UIUpdateMode::Redraw);

		void _fixBodyStartMidColumn(Ref<Column>* columns, sl_uint32 nColumns, sl_uint32 nLeft, sl_uint32 iStart, sl_uint32& newStart);
		void _fixHeaderStartMidColumn(Ref<Column>* columns, sl_uint32 nColumns, sl_uint32 nLeft, sl_uint32 iStart, sl_uint32& newStart);
		void _fixFooterStartMidColumn(Ref<Column>* columns, sl_uint32 nColumns, sl_uint32 nLeft, sl_uint32 iStart, sl_uint32& newStart);

		void _drawRecords(Canvas* canvas, sl_ui_len top, sl_ui_len bottom, Ref<Column>* columns, sl_uint32 nColumns, sl_uint32 nLeft, sl_ui_pos xLeft, sl_uint32 nRight, sl_ui_pos xRight, sl_uint32 iStartMidColumn, sl_uint32 nMidColumns, sl_ui_pos xStartMidColumn);
		void _drawHeader(Canvas* canvas, sl_ui_len top, sl_ui_len bottom, Ref<Column>* columns, sl_uint32 nColumns, sl_uint32 nLeft, sl_ui_pos xLeft, sl_uint32 nRight, sl_ui_pos xRight, sl_uint32 iStartMidColumn, sl_uint32 nMidColumns, sl_ui_pos xStartMidColumn);
		void _drawFooter(Canvas* canvas, sl_ui_len top, sl_ui_len bottom, Ref<Column>* columns, sl_uint32 nColumns, sl_uint32 nLeft, sl_ui_pos xLeft, sl_uint32 nRight, sl_ui_pos xRight, sl_uint32 iStartMidColumn, sl_uint32 nMidColumns, sl_ui_pos xStartMidColumn);

		void _drawBodyColumn(Canvas* canvas, sl_ui_pos x, sl_ui_pos y, Column* column, sl_uint32 iColumn, Ref<Row>* rows, sl_uint32 nRows, sl_uint64 iRecord, const Variant& recordData);
		void _drawHeaderColumn(Canvas* canvas, sl_ui_pos x, sl_ui_pos y, Column* column, sl_uint32 iColumn, Ref<Row>* rows, sl_uint32 nRows);
		void _drawFooterColumn(Canvas* canvas, sl_ui_pos x, sl_ui_pos y, Column* column, sl_uint32 iColumn, Ref<Row>* rows, sl_uint32 nRows);

		void _drawBodyInnerGrid(Canvas* canvas, sl_ui_pos x, sl_ui_pos top, sl_ui_pos bottom, Ref<Column>* columns, sl_uint32 nColumns, Ref<Row>* rows, sl_uint32 nRows, sl_uint32 nRecords, sl_bool flagBody, const Ref<Pen>& pen);
		void _drawHeaderInnerGrid(Canvas* canvas, sl_ui_pos x, sl_ui_pos top, sl_ui_pos bottom, Ref<Column>* columns, sl_uint32 nColumns, Ref<Row>* rows, sl_uint32 nRows, sl_uint32 nRecords, sl_bool flagBody, const Ref<Pen>& pen);
		void _drawFooterInnerGrid(Canvas* canvas, sl_ui_pos x, sl_ui_pos top, sl_ui_pos bottom, Ref<Column>* columns, sl_uint32 nColumns, Ref<Row>* rows, sl_uint32 nRows, sl_uint32 nRecords, sl_bool flagBody, const Ref<Pen>& pen);

		void _drawHorzOuterGrid(Canvas* canvas, sl_ui_pos x1, sl_ui_pos x2, sl_ui_pos x3, sl_ui_pos x4, sl_ui_pos y, const Ref<Pen>& penLeft, const Ref<Pen>& penMid, const Ref<Pen>& penRight);
		void _drawVertOuterGrid(Canvas* canvas, sl_ui_pos x, sl_ui_pos y1, sl_ui_pos y2, sl_ui_pos y3, sl_ui_pos y4, const Ref<Pen>& penTop, const Ref<Pen>& penMid, const Ref<Pen>& penBottom);

		void _drawCell(Canvas* canvas, sl_ui_pos x, sl_ui_pos y, Cell* cell);

		void _prepareBodyLayout(Ref<Column>* columns, sl_uint32 nColumns);
		void _prepareHeaderLayout(Ref<Column>* columns, sl_uint32 nColumns);
		void _prepareFooterLayout(Ref<Column>* columns, sl_uint32 nColumns);

		Ref<Cell> _createBodyCell(BodyCellProp& prop, RecordIndex iRecord, sl_uint32 iRow, sl_uint32 iCol, const Variant& recordData);

		Cell* _getFixedCell(FixedCellProp& prop, RecordIndex iRecord, sl_uint32 iRow, sl_uint32 iCol);

		Ref<Cell> _getEventCell(UIEvent* ev);

		sl_int32 _getColumnForResizing(UIEvent* ev, sl_bool& flagRight, sl_bool& flagDual);
		void _processResizingColumn(UIEvent* ev);

		sl_ui_len _getMiddleColumnOffset(sl_uint32 iCol);

		void _invalidateLayout();

		void _invalidateBodyCell(BodyCellProp& prop);
		void _invalidateHeaderCell(HeaderCellProp& prop);
		void _invalidateFooterCell(FooterCellProp& prop);

		void _invalidateBodyColumnCells(Column* column);
		void _invalidateHeaderColumnCells(Column* column);
		void _invalidateFooterColumnCells(Column* column);

		void _invalidateBodyRowCells(Row* row);
		void _invalidateHeaderRowCells(Row* row);
		void _invalidateFooterRowCells(Row* row);

		void _invalidateBodyAllCells();
		void _invalidateHeaderAllCells();
		void _invalidateFooterAllCells();

		void _invalidateAllCells();

	private:
		CList< Ref<Column> > m_columns;

		sl_uint64 m_nRecords;
		sl_uint32 m_nLeftColumns;
		sl_uint32 m_nRightColumns;

		CList< Ref<Row> > m_listBodyRow;
		CList< Ref<Row> > m_listHeaderRow;
		CList< Ref<Row> > m_listFooterRow;

		sl_ui_len m_defaultColumnWidth;
		sl_ui_len m_defaultColumnMinWidth;
		sl_ui_len m_defaultColumnMaxWidth;
		sl_bool m_defaultColumnResizable;

		sl_ui_len m_defaultBodyRowHeight;
		sl_ui_len m_defaultHeaderRowHeight;
		sl_ui_len m_defaultFooterRowHeight;

		CellProp m_defaultBodyProps;
		CellProp m_defaultHeaderProps;
		CellProp m_defaultFooterProps;

		AtomicRef<Pen> m_gridBody;
		AtomicRef<Pen> m_gridHeader;
		AtomicRef<Pen> m_gridFooter;
		AtomicRef<Pen> m_gridLeft;
		AtomicRef<Pen> m_gridRight;
		AtomicRef<Pen> m_selectionBorder;

		AtomicRef<Drawable> m_iconAsc;
		AtomicRef<Drawable> m_iconDesc;

		typedef Function<Variant(sl_uint64 record)> DataFunction;
		Atomic<DataFunction> m_recordData;

		SelectionMode m_selectionMode;
		Selection m_hover;
		Selection m_selection;

		sl_bool m_flagInitialize;
		sl_bool m_flagInvalidateBodyLayout;
		sl_bool m_flagInvalidateHeaderLayout;
		sl_bool m_flagInvalidateFooterLayout;

		struct ResizingColumn
		{
			sl_int32 index;
			sl_bool flagRight;
			sl_bool flagDual;
			sl_ui_len formerWidth;
			sl_ui_len formerWidth2;
			sl_ui_pos formerEventX;
		} m_resizingColumn;

		Ref<Cell> m_cellSorting;
	};

}

#endif
