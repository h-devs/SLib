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
#include "../core/shared.h"

namespace slib
{

	class Cursor;

	class SLIB_EXPORT GridView : public View
	{
		SLIB_DECLARE_OBJECT

	public:
		class Cell;
		typedef Function<String(Cell*)> TextGetter;
		typedef Function<Ref<Drawable>(Cell*, ViewState)> DrawableGetter;
		typedef Function<Color(Cell*, ViewState)> ColorGetter;

		class CellAttribute
		{
		public:
			String field;
			String text;
			TextGetter textGetter;
			Ref<Font> font;
			Ref<Cursor> cursor;
			String toolTip;
			TextGetter toolTipGetter;
			UIEdgeInsets padding;
			MultiLineMode multiLineMode;
			EllipsizeMode ellipsizeMode;
			sl_uint32 lineCount;
			Alignment align;
			sl_bool flagSelectable : 1;
			sl_bool flagEditable : 1;
			sl_bool flagBackgroundAntiAlias : 1;
			sl_bool flagContentAntiAlias : 1;

			DrawableGetter backgroundGetter;
			ColorGetter textColorGetter;
			Nullable<sl_bool> flagDefaultFilter;

			DrawableGetter iconGetter;
			sl_ui_len iconWidth;
			UIEdgeInsets iconMargin;
			ScaleMode iconScale;
			Alignment iconAlign;

			ViewStateMap< Ref<Drawable> > backgrounds;
			ViewStateMap<Color> textColors;
			ViewStateMap< Shared<ColorMatrix> > filters;
			ViewStateMap< Ref<Drawable> > icons;

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

		class CellParam
		{
		public:
			GridView* view;
			CellAttribute* attr;
			Ref<Font> font;
			sl_uint32 row;
			sl_uint32 column;
			RecordIndex record;
			Variant recordData;

		public:
			CellParam();
			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(CellParam)
		};

		class DrawCellParam
		{
		public:
			ViewState state;
			ViewState contentState;
			sl_ui_len x;
			sl_ui_len y;

		public:
			DrawCellParam();
			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(DrawCellParam)
		};

		class Cell : public CRef, public CellParam
		{
		public:
			typedef DrawCellParam DrawParam;

		public:
			Cell();
			~Cell();

		public:
			String getText();
			String getInternalText();

			String getToolTip();
			String getInternalToolTip();

			Ref<Drawable> getBackground(ViewState state);
			Ref<Drawable> getInternalBackground(ViewState state);

			Color getTextColor(ViewState state);
			Color getInternalTextColor(ViewState state);

			Ref<Drawable> getIcon(ViewState state);
			Ref<Drawable> getInternalIcon(ViewState state);

			sl_bool getColorFilter(ColorMatrix& _out, ViewState state, sl_bool flagDefaultFilter);
			Ref<Drawable> filter(const Ref<Drawable>& drawable, ViewState state, sl_bool flagDefaultFilter);
			Color filter(const Color& color, ViewState state, sl_bool flagDefaultFilter);

		public:
			void draw(Canvas*, DrawParam&);

			virtual void onInit();
			virtual void onDrawContent(Canvas*, const UIRect& frame, DrawParam&);
			virtual void onClick(UIEvent*);
			virtual void onEvent(UIEvent*);
			virtual void onCopy();

		protected:
			UIRect m_iconFrame; // Relative
			UIRect m_contentFrame; // Relative

			Color m_defaultTextColor;
			sl_bool m_flagSelectable : 1;
			sl_bool m_flagDefaultFilter : 1;
			sl_bool m_flagUseContentState : 1;

			friend class GridView;
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
			void onDrawContent(Canvas*, const UIRect& frame, DrawParam&) override;
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
			void onDrawContent(Canvas*, const UIRect& frame, DrawParam&) override;

			void onClick(UIEvent*) override;

		};

		class IconCell : public Cell
		{
		public:
			IconCell();
			~IconCell();

		public:
			static const CellCreator& creator();

		public:
			void onInit() override;
		};

		class ButtonCell : public TextCell
		{
		public:
			ButtonCell();
			~ButtonCell();

		public:
			static const CellCreator& creator();

		public:
			void onInit() override;
			void onEvent(UIEvent*) override;
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
			sl_bool isCell() const;
			sl_bool match(RecordIndex record, sl_int32 row, sl_int32 column) const;
			sl_bool match(Cell* cell) const;
			sl_bool matchCell(RecordIndex record, sl_uint32 row, sl_uint32 column) const;
			sl_bool matchCell(Cell* cell) const;

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

		sl_bool isBodyVerticalGrid(sl_uint32 column);
		sl_bool isHeaderVerticalGrid(sl_uint32 column);
		sl_bool isFooterVerticalGrid(sl_uint32 column);

		void setVerticalGrid(sl_uint32 column, sl_bool flag = sl_true, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setBodyVerticalGrid(sl_uint32 column, sl_bool flag = sl_true, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setHeaderVerticalGrid(sl_uint32 column, sl_bool flag = sl_true, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setFooterVerticalGrid(sl_uint32 column, sl_bool flag = sl_true, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setVerticalGrid(sl_bool flag = sl_true, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setBodyVerticalGrid(sl_bool flag = sl_true, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setHeaderVerticalGrid(sl_bool flag = sl_true, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setFooterVerticalGrid(sl_bool flag = sl_true, UIUpdateMode mode = UIUpdateMode::Redraw);

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

		void setBodyRowVisible(sl_uint32 index, sl_bool flagVisible = sl_true, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setHeaderRowVisible(sl_uint32 index, sl_bool flagVisible = sl_true, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setFooterRowVisible(sl_uint32 index, sl_bool flagVisible = sl_true, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_bool isBodyHorizontalGrid(sl_uint32 row);
		sl_bool isHeaderHorizontalGrid(sl_uint32 row);
		sl_bool isFooterHorizontalGrid(sl_uint32 row);

		void setBodyHorizontalGrid(sl_uint32 row, sl_bool flag = sl_true, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setHeaderHorizontalGrid(sl_uint32 row, sl_bool flag = sl_true, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setFooterHorizontalGrid(sl_uint32 row, sl_bool flag = sl_true, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setHorizontalGrid(sl_bool flag = sl_true, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setBodyHorizontalGrid(sl_bool flag = sl_true, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setHeaderHorizontalGrid(sl_bool flag = sl_true, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setFooterHorizontalGrid(sl_bool flag = sl_true, UIUpdateMode mode = UIUpdateMode::Redraw);

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

		sl_bool isSorting();
		void setSorting(sl_bool flag);

		Ref<Drawable> getAscendingIcon();
		void setAscendingIcon(const Ref<Drawable>& icon, UIUpdateMode mode = UIUpdateMode::Redraw);

		Ref<Drawable> getDescendingIcon();
		void setDescendingIcon(const Ref<Drawable>& icon, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_ui_len getSortIconSize();
		void setSortIconSize(sl_ui_len size, UIUpdateMode mode = UIUpdateMode::Redraw);

		void refreshContentWidth(UIUpdateMode mode = UIUpdateMode::Redraw);
		void refreshContentHeight(UIUpdateMode mode = UIUpdateMode::Redraw);

		Function<Variant(sl_uint64 record)> getDataGetter();
		void setDataGetter(const Function<Variant(sl_uint64 record)>& func, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setData(const VariantList& data, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setData(const JsonList& data, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setData(const List<VariantMap>& data, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setData(const Variant& data, UIUpdateMode mode = UIUpdateMode::Redraw);

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

		Function<String(Cell*)> getBodyTextGetter(sl_uint32 row, sl_uint32 column);
		Function<String(Cell*)> getHeaderTextGetter(sl_uint32 row, sl_uint32 column);
		Function<String(Cell*)> getFooterTextGetter(sl_uint32 row, sl_uint32 column);

		void setBodyTextGetter(sl_int32 row, sl_int32 column, const Function<String(Cell*)>& getter, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setHeaderTextGetter(sl_int32 row, sl_int32 column, const Function<String(Cell*)>& getter, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setFooterTextGetter(sl_int32 row, sl_int32 column, const Function<String(Cell*)>& getter, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setColumnTextGetter(sl_int32 column, const Function<String(Cell*)>& getter, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setCellTextGetter(const Function<String(Cell*)>& getter, UIUpdateMode mode = UIUpdateMode::Redraw);

		Ref<Font> getBodyFont(sl_uint32 row, sl_uint32 column);
		Ref<Font> getHeaderFont(sl_uint32 row, sl_uint32 column);
		Ref<Font> getFooterFont(sl_uint32 row, sl_uint32 column);

		void setBodyFont(sl_int32 row, sl_int32 column, const Ref<Font>& font, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setHeaderFont(sl_int32 row, sl_int32 column, const Ref<Font>& font, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setFooterFont(sl_int32 row, sl_int32 column, const Ref<Font>& font, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setColumnFont(sl_int32 column, const Ref<Font>& font, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setCellFont(const Ref<Font>& font, UIUpdateMode mode = UIUpdateMode::Redraw);

		Ref<Cursor> getBodyCursor(sl_uint32 row, sl_uint32 column);
		Ref<Cursor> getHeaderCursor(sl_uint32 row, sl_uint32 column);
		Ref<Cursor> getFooterCursor(sl_uint32 row, sl_uint32 column);

		void setBodyCursor(sl_int32 row, sl_int32 column, const Ref<Cursor>& cursor);
		void setHeaderCursor(sl_int32 row, sl_int32 column, const Ref<Cursor>& cursor);
		void setFooterCursor(sl_int32 row, sl_int32 column, const Ref<Cursor>& cursor);
		void setColumnCursor(sl_int32 column, const Ref<Cursor>& cursor);
		void setCellCursor(const Ref<Cursor>& cursor);

		String getBodyToolTip(sl_uint32 row, sl_uint32 column);
		String getHeaderToolTip(sl_uint32 row, sl_uint32 column);
		String getFooterToolTip(sl_uint32 row, sl_uint32 column);

		void setBodyToolTip(sl_int32 row, sl_int32 column, const String& toolTip);
		void setHeaderToolTip(sl_int32 row, sl_int32 column, const String& toolTip);
		void setFooterToolTip(sl_int32 row, sl_int32 column, const String& toolTip);
		void setColumnToolTip(sl_int32 column, const String& toolTip);
		void setCellToolTip(const String& toolTip);

		Function<String(Cell*)> getBodyToolTipGetter(sl_uint32 row, sl_uint32 column);
		Function<String(Cell*)> getHeaderToolTipGetter(sl_uint32 row, sl_uint32 column);
		Function<String(Cell*)> getFooterToolTipGetter(sl_uint32 row, sl_uint32 column);

		void setBodyToolTipGetter(sl_int32 row, sl_int32 column, const Function<String(Cell*)>& getter, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setHeaderToolTipGetter(sl_int32 row, sl_int32 column, const Function<String(Cell*)>& getter, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setFooterToolTipGetter(sl_int32 row, sl_int32 column, const Function<String(Cell*)>& getter, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setColumnToolTipGetter(sl_int32 column, const Function<String(Cell*)>& getter, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setCellToolTipGetter(const Function<String(Cell*)>& getter, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_ui_len getBodyPaddingLeft(sl_uint32 row, sl_uint32 column);
		sl_ui_len getBodyPaddingTop(sl_uint32 row, sl_uint32 column);
		sl_ui_len getBodyPaddingRight(sl_uint32 row, sl_uint32 column);
		sl_ui_len getBodyPaddingBottom(sl_uint32 row, sl_uint32 column);
		sl_ui_len getHeaderPaddingLeft(sl_uint32 row, sl_uint32 column);
		sl_ui_len getHeaderPaddingTop(sl_uint32 row, sl_uint32 column);
		sl_ui_len getHeaderPaddingRight(sl_uint32 row, sl_uint32 column);
		sl_ui_len getHeaderPaddingBottom(sl_uint32 row, sl_uint32 column);
		sl_ui_len getFooterPaddingLeft(sl_uint32 row, sl_uint32 column);
		sl_ui_len getFooterPaddingTop(sl_uint32 row, sl_uint32 column);
		sl_ui_len getFooterPaddingRight(sl_uint32 row, sl_uint32 column);
		sl_ui_len getFooterPaddingBottom(sl_uint32 row, sl_uint32 column);

		void setBodyPadding(sl_int32 row, sl_int32 column, sl_ui_len padding, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setBodyPaddingLeft(sl_int32 row, sl_int32 column, sl_ui_len padding, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setBodyPaddingTop(sl_int32 row, sl_int32 column, sl_ui_len padding, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setBodyPaddingRight(sl_int32 row, sl_int32 column, sl_ui_len padding, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setBodyPaddingBottom(sl_int32 row, sl_int32 column, sl_ui_len padding, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setHeaderPadding(sl_int32 row, sl_int32 column, sl_ui_len padding, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setHeaderPaddingLeft(sl_int32 row, sl_int32 column, sl_ui_len padding, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setHeaderPaddingTop(sl_int32 row, sl_int32 column, sl_ui_len padding, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setHeaderPaddingRight(sl_int32 row, sl_int32 column, sl_ui_len padding, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setHeaderPaddingBottom(sl_int32 row, sl_int32 column, sl_ui_len padding, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setFooterPadding(sl_int32 row, sl_int32 column, sl_ui_len padding, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setFooterPaddingLeft(sl_int32 row, sl_int32 column, sl_ui_len padding, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setFooterPaddingTop(sl_int32 row, sl_int32 column, sl_ui_len padding, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setFooterPaddingRight(sl_int32 row, sl_int32 column, sl_ui_len padding, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setFooterPaddingBottom(sl_int32 row, sl_int32 column, sl_ui_len padding, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setColumnPadding(sl_int32 column, sl_ui_len padding, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setColumnPaddingLeft(sl_int32 column, sl_ui_len padding, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setColumnPaddingTop(sl_int32 column, sl_ui_len padding, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setColumnPaddingRight(sl_int32 column, sl_ui_len padding, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setColumnPaddingBottom(sl_int32 column, sl_ui_len padding, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setCellPadding(sl_ui_len padding, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setCellPaddingLeft(sl_ui_len padding, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setCellPaddingTop(sl_ui_len padding, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setCellPaddingRight(sl_ui_len padding, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setCellPaddingBottom(sl_ui_len padding, UIUpdateMode mode = UIUpdateMode::Redraw);

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

		void setBodySelectable(sl_int32 row, sl_int32 column, sl_bool flag = sl_true);
		void setHeaderSelectable(sl_int32 row, sl_int32 column, sl_bool flag = sl_true);
		void setFooterSelectable(sl_int32 row, sl_int32 column, sl_bool flag = sl_true);
		void setColumnSelectable(sl_int32 column, sl_bool flag = sl_true);
		void setCellSelectable(sl_bool flag = sl_true);

		sl_bool isBodyEditable(sl_uint32 row, sl_uint32 column);
		sl_bool isHeaderEditable(sl_uint32 row, sl_uint32 column);
		sl_bool isFooterEditable(sl_uint32 row, sl_uint32 column);

		void setBodyEditable(sl_int32 row, sl_int32 column, sl_bool flag = sl_true);
		void setHeaderEditable(sl_int32 row, sl_int32 column, sl_bool flag = sl_true);
		void setFooterEditable(sl_int32 row, sl_int32 column, sl_bool flag = sl_true);
		void setColumnEditable(sl_int32 column, sl_bool flag = sl_true);
		void setCellEditable(sl_bool flag = sl_true);

		sl_bool isBodyBackgroundAntiAlias(sl_uint32 row, sl_uint32 column);
		sl_bool isHeaderBackgroundAntiAlias(sl_uint32 row, sl_uint32 column);
		sl_bool isFooterBackgroundAntiAlias(sl_uint32 row, sl_uint32 column);

		void setBodyBackgroundAntiAlias(sl_int32 row, sl_int32 column, sl_bool flag = sl_true, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setHeaderBackgroundAntiAlias(sl_int32 row, sl_int32 column, sl_bool flag = sl_true, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setFooterBackgroundAntiAlias(sl_int32 row, sl_int32 column, sl_bool flag = sl_true, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setColumnBackgroundAntiAlias(sl_int32 column, sl_bool flag = sl_true, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setCellBackgroundAntiAlias(sl_bool flag = sl_true, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_bool isBodyContentAntiAlias(sl_uint32 row, sl_uint32 column);
		sl_bool isHeaderContentAntiAlias(sl_uint32 row, sl_uint32 column);
		sl_bool isFooterContentAntiAlias(sl_uint32 row, sl_uint32 column);

		void setBodyContentAntiAlias(sl_int32 row, sl_int32 column, sl_bool flag = sl_true, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setHeaderContentAntiAlias(sl_int32 row, sl_int32 column, sl_bool flag = sl_true, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setFooterContentAntiAlias(sl_int32 row, sl_int32 column, sl_bool flag = sl_true, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setColumnContentAntiAlias(sl_int32 column, sl_bool flag = sl_true, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setCellContentAntiAlias(sl_bool flag = sl_true, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setBodyAntiAlias(sl_int32 row, sl_int32 column, sl_bool flag = sl_true, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setHeaderAntiAlias(sl_int32 row, sl_int32 column, sl_bool flag = sl_true, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setFooterAntiAlias(sl_int32 row, sl_int32 column, sl_bool flag = sl_true, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setColumnAntiAlias(sl_int32 column, sl_bool flag = sl_true, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setCellAntiAlias(sl_bool flag = sl_true, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_bool isBodyUsingDefaultColorFilter(sl_uint32 row, sl_uint32 column);
		sl_bool isHeaderUsingDefaultColorFilter(sl_uint32 row, sl_uint32 column);
		sl_bool isFooterUsingDefaultColorFilter(sl_uint32 row, sl_uint32 column);

		void setBodyUsingDefaultColorFilter(sl_int32 row, sl_int32 column, sl_bool flag);
		void setHeaderUsingDefaultColorFilter(sl_int32 row, sl_int32 column, sl_bool flag);
		void setFooterUsingDefaultColorFilter(sl_int32 row, sl_int32 column, sl_bool flag);
		void setColumnUsingDefaultColorFilter(sl_int32 column, sl_bool flag);
		void setCellUsingDefaultColorFilter(sl_bool flag);

		Ref<Drawable> getBodyIcon(sl_uint32 row, sl_uint32 column, ViewState state = ViewState::Default);
		Ref<Drawable> getHeaderIcon(sl_uint32 row, sl_uint32 column, ViewState state = ViewState::Default);
		Ref<Drawable> getFooterIcon(sl_uint32 row, sl_uint32 column, ViewState state = ViewState::Default);

		void setBodyIcon(sl_int32 row, sl_int32 column, const Ref<Drawable>& drawable, ViewState state = ViewState::Default, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setHeaderIcon(sl_int32 row, sl_int32 column, const Ref<Drawable>& drawable, ViewState state = ViewState::Default, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setFooterIcon(sl_int32 row, sl_int32 column, const Ref<Drawable>& drawable, ViewState state = ViewState::Default, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setColumnIcon(sl_int32 column, const Ref<Drawable>& drawable, ViewState state = ViewState::Default, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setCellIcon(const Ref<Drawable>& drawable, ViewState state = ViewState::Default, UIUpdateMode mode = UIUpdateMode::Redraw);

		Function<Ref<Drawable>(Cell*, ViewState)> getBodyIconGetter(sl_uint32 row, sl_uint32 column);
		Function<Ref<Drawable>(Cell*, ViewState)> getHeaderIconGetter(sl_uint32 row, sl_uint32 column);
		Function<Ref<Drawable>(Cell*, ViewState)> getFooterIconGetter(sl_uint32 row, sl_uint32 column);

		void setBodyIconGetter(sl_int32 row, sl_int32 column, const Function<Ref<Drawable>(Cell*, ViewState)>& getter, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setHeaderIconGetter(sl_int32 row, sl_int32 column, const Function<Ref<Drawable>(Cell*, ViewState)>& getter, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setFooterIconGetter(sl_int32 row, sl_int32 column, const Function<Ref<Drawable>(Cell*, ViewState)>& getter, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setColumnIconGetter(sl_int32 column, const Function<Ref<Drawable>(Cell*, ViewState)>& getter, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setCellIconGetter(const Function<Ref<Drawable>(Cell*, ViewState)>& getter, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_ui_len getBodyIconWidth(sl_uint32 row, sl_uint32 column);
		sl_ui_len getHeaderIconWidth(sl_uint32 row, sl_uint32 column);
		sl_ui_len getFooterIconWidth(sl_uint32 row, sl_uint32 column);

		void setBodyIconWidth(sl_int32 row, sl_int32 column, sl_ui_len width, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setHeaderIconWidth(sl_int32 row, sl_int32 column, sl_ui_len width, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setFooterIconWidth(sl_int32 row, sl_int32 column, sl_ui_len width, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setColumnIconWidth(sl_int32 column, sl_ui_len width, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setCellIconWidth(sl_ui_len width, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_ui_len getBodyIconMarginLeft(sl_uint32 row, sl_uint32 column);
		sl_ui_len getBodyIconMarginTop(sl_uint32 row, sl_uint32 column);
		sl_ui_len getBodyIconMarginRight(sl_uint32 row, sl_uint32 column);
		sl_ui_len getBodyIconMarginBottom(sl_uint32 row, sl_uint32 column);
		sl_ui_len getHeaderIconMarginLeft(sl_uint32 row, sl_uint32 column);
		sl_ui_len getHeaderIconMarginTop(sl_uint32 row, sl_uint32 column);
		sl_ui_len getHeaderIconMarginRight(sl_uint32 row, sl_uint32 column);
		sl_ui_len getHeaderIconMarginBottom(sl_uint32 row, sl_uint32 column);
		sl_ui_len getFooterIconMarginLeft(sl_uint32 row, sl_uint32 column);
		sl_ui_len getFooterIconMarginTop(sl_uint32 row, sl_uint32 column);
		sl_ui_len getFooterIconMarginRight(sl_uint32 row, sl_uint32 column);
		sl_ui_len getFooterIconMarginBottom(sl_uint32 row, sl_uint32 column);

		void setBodyIconMargin(sl_int32 row, sl_int32 column, sl_ui_len margin, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setBodyIconMarginLeft(sl_int32 row, sl_int32 column, sl_ui_len margin, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setBodyIconMarginTop(sl_int32 row, sl_int32 column, sl_ui_len margin, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setBodyIconMarginRight(sl_int32 row, sl_int32 column, sl_ui_len margin, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setBodyIconMarginBottom(sl_int32 row, sl_int32 column, sl_ui_len margin, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setHeaderIconMargin(sl_int32 row, sl_int32 column, sl_ui_len margin, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setHeaderIconMarginLeft(sl_int32 row, sl_int32 column, sl_ui_len margin, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setHeaderIconMarginTop(sl_int32 row, sl_int32 column, sl_ui_len margin, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setHeaderIconMarginRight(sl_int32 row, sl_int32 column, sl_ui_len margin, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setHeaderIconMarginBottom(sl_int32 row, sl_int32 column, sl_ui_len margin, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setFooterIconMargin(sl_int32 row, sl_int32 column, sl_ui_len margin, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setFooterIconMarginLeft(sl_int32 row, sl_int32 column, sl_ui_len margin, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setFooterIconMarginTop(sl_int32 row, sl_int32 column, sl_ui_len margin, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setFooterIconMarginRight(sl_int32 row, sl_int32 column, sl_ui_len margin, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setFooterIconMarginBottom(sl_int32 row, sl_int32 column, sl_ui_len margin, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setColumnIconMargin(sl_int32 column, sl_ui_len margin, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setColumnIconMarginLeft(sl_int32 column, sl_ui_len margin, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setColumnIconMarginTop(sl_int32 column, sl_ui_len margin, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setColumnIconMarginRight(sl_int32 column, sl_ui_len margin, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setColumnIconMarginBottom(sl_int32 column, sl_ui_len margin, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setCellIconMargin(sl_ui_len margin, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setCellIconMarginLeft(sl_ui_len margin, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setCellIconMarginTop(sl_ui_len margin, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setCellIconMarginRight(sl_ui_len margin, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setCellIconMarginBottom(sl_ui_len margin, UIUpdateMode mode = UIUpdateMode::Redraw);

		ScaleMode getBodyIconScaleMode(sl_uint32 row, sl_uint32 column);
		ScaleMode getHeaderIconScaleMode(sl_uint32 row, sl_uint32 column);
		ScaleMode getFooterIconScaleMode(sl_uint32 row, sl_uint32 column);

		void setBodyIconScaleMode(sl_int32 row, sl_int32 column, ScaleMode scale, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setHeaderIconScaleMode(sl_int32 row, sl_int32 column, ScaleMode scale, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setFooterIconScaleMode(sl_int32 row, sl_int32 column, ScaleMode scale, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setColumnIconScaleMode(sl_int32 column, ScaleMode scale, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setCellIconScaleMode(ScaleMode scale, UIUpdateMode mode = UIUpdateMode::Redraw);

		Alignment getBodyIconAlignment(sl_uint32 row, sl_uint32 column);
		Alignment getHeaderIconAlignment(sl_uint32 row, sl_uint32 column);
		Alignment getFooterIconAlignment(sl_uint32 row, sl_uint32 column);

		void setBodyIconAlignment(sl_int32 row, sl_int32 column, const Alignment& align, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setHeaderIconAlignment(sl_int32 row, sl_int32 column, const Alignment& align, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setFooterIconAlignment(sl_int32 row, sl_int32 column, const Alignment& align, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setColumnIconAlignment(sl_int32 column, const Alignment& align, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setCellIconAlignment(const Alignment& align, UIUpdateMode mode = UIUpdateMode::Redraw);

		Ref<Drawable> getBodyBackground(sl_uint32 row, sl_uint32 column, ViewState state = ViewState::Default);
		Ref<Drawable> getHeaderBackground(sl_uint32 row, sl_uint32 column, ViewState state = ViewState::Default);
		Ref<Drawable> getFooterBackground(sl_uint32 row, sl_uint32 column, ViewState state = ViewState::Default);

		void setBodyBackground(sl_int32 row, sl_int32 column, const Ref<Drawable>& drawable, ViewState state = ViewState::Default, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setHeaderBackground(sl_int32 row, sl_int32 column, const Ref<Drawable>& drawable, ViewState state = ViewState::Default, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setFooterBackground(sl_int32 row, sl_int32 column, const Ref<Drawable>& drawable, ViewState state = ViewState::Default, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setColumnBackground(sl_int32 column, const Ref<Drawable>& drawable, ViewState state = ViewState::Default, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setCellBackground(const Ref<Drawable>& drawable, ViewState state = ViewState::Default, UIUpdateMode mode = UIUpdateMode::Redraw);

		Function<Ref<Drawable>(Cell*, ViewState)> getBodyBackgroundGetter(sl_uint32 row, sl_uint32 column);
		Function<Ref<Drawable>(Cell*, ViewState)> getHeaderBackgroundGetter(sl_uint32 row, sl_uint32 column);
		Function<Ref<Drawable>(Cell*, ViewState)> getFooterBackgroundGetter(sl_uint32 row, sl_uint32 column);

		void setBodyBackgroundGetter(sl_int32 row, sl_int32 column, const Function<Ref<Drawable>(Cell*, ViewState)>& getter, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setHeaderBackgroundGetter(sl_int32 row, sl_int32 column, const Function<Ref<Drawable>(Cell*, ViewState)>& getter, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setFooterBackgroundGetter(sl_int32 row, sl_int32 column, const Function<Ref<Drawable>(Cell*, ViewState)>& getter, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setColumnBackgroundGetter(sl_int32 column, const Function<Ref<Drawable>(Cell*, ViewState)>& getter, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setCellBackgroundGetter(const Function<Ref<Drawable>(Cell*, ViewState)>& getter, UIUpdateMode mode = UIUpdateMode::Redraw);

		Color getBodyTextColor(sl_uint32 row, sl_uint32 column, ViewState state = ViewState::Default);
		Color getHeaderTextColor(sl_uint32 row, sl_uint32 column, ViewState state = ViewState::Default);
		Color getFooterTextColor(sl_uint32 row, sl_uint32 column, ViewState state = ViewState::Default);

		void setBodyTextColor(sl_int32 row, sl_int32 column, const Color& color, ViewState state = ViewState::Default, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setHeaderTextColor(sl_int32 row, sl_int32 column, const Color& color, ViewState state = ViewState::Default, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setFooterTextColor(sl_int32 row, sl_int32 column, const Color& color, ViewState state = ViewState::Default, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setColumnTextColor(sl_int32 column, const Color& color, ViewState state = ViewState::Default, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setCellTextColor(const Color& color, ViewState state = ViewState::Default, UIUpdateMode mode = UIUpdateMode::Redraw);

		Function<Color(Cell*, ViewState)> getBodyTextColorGetter(sl_uint32 row, sl_uint32 column);
		Function<Color(Cell*, ViewState)> getHeaderTextColorGetter(sl_uint32 row, sl_uint32 column);
		Function<Color(Cell*, ViewState)> getFooterTextColorGetter(sl_uint32 row, sl_uint32 column);

		void setBodyTextColorGetter(sl_int32 row, sl_int32 column, const Function<Color(Cell*, ViewState)>& getter, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setHeaderTextColorGetter(sl_int32 row, sl_int32 column, const Function<Color(Cell*, ViewState)>& getter, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setFooterTextColorGetter(sl_int32 row, sl_int32 column, const Function<Color(Cell*, ViewState)>& getter, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setColumnTextColorGetter(sl_int32 column, const Function<Color(Cell*, ViewState)>& getter, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setCellTextColorGetter(const Function<Color(Cell*, ViewState)>& getter, UIUpdateMode mode = UIUpdateMode::Redraw);

		Shared<ColorMatrix> getBodyColorFilter(sl_uint32 row, sl_uint32 column, ViewState state = ViewState::Default);
		Shared<ColorMatrix> getHeaderColorFilter(sl_uint32 row, sl_uint32 column, ViewState state = ViewState::Default);
		Shared<ColorMatrix> getFooterColorFilter(sl_uint32 row, sl_uint32 column, ViewState state = ViewState::Default);

		void setBodyColorFilter(sl_int32 row, sl_int32 column, const Shared<ColorMatrix>& filter, ViewState state = ViewState::Default, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setHeaderColorFilter(sl_int32 row, sl_int32 column, const Shared<ColorMatrix>& filter, ViewState state = ViewState::Default, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setFooterColorFilter(sl_int32 row, sl_int32 column, const Shared<ColorMatrix>& filter, ViewState state = ViewState::Default, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setColumnColorFilter(sl_int32 column, const Shared<ColorMatrix>& filter, ViewState state = ViewState::Default, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setCellColorFilter(const Shared<ColorMatrix>& filter, ViewState state = ViewState::Default, UIUpdateMode mode = UIUpdateMode::Redraw);

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

		sl_bool getSelectionAt(sl_ui_pos x, sl_ui_pos y, Selection& _out);

		sl_bool getCellLocation(UIPoint& _out, RecordIndex record, sl_int32 row, sl_int32 column);
		sl_bool getCellFrame(UIRect& _out, RecordIndex record, sl_int32 row, sl_int32 column);

		ViewState getCellState(RecordIndex record, sl_int32 row, sl_int32 column);
		ViewState getCellState(Cell* cell);
		ViewState getCellContentState(RecordIndex record, sl_uint32 row, sl_uint32 column);
		ViewState getCellContentState(Cell* cell);

	public:
		SLIB_DECLARE_EVENT_HANDLER(GridView, ClickCell, Cell*, UIEvent*)
		SLIB_DECLARE_EVENT_HANDLER(GridView, RightButtonClickCell, Cell*, UIEvent*)
		SLIB_DECLARE_EVENT_HANDLER(GridView, DoubleClickCell, Cell*, UIEvent*)

		SLIB_DECLARE_EVENT_HANDLER(GridView, Select, const Selection& selection, const Selection& former, UIEvent* /* nullable */)

		SLIB_DECLARE_EVENT_HANDLER(GridView, Sort, const String& field, sl_bool flagAscending)

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
			sl_bool flagCoveredX : 1;
			sl_bool flagCoveredY : 1;

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
			void setDefaultWidth(sl_ui_len width, UIUpdateMode mode = UIUpdateMode::Redraw);

			sl_ui_len getMinimumWidth();
			void setMinimumWidth(sl_ui_len width, UIUpdateMode mode = UIUpdateMode::Redraw);

			sl_ui_len getMaximumWidth();
			void setMaximumWidth(sl_ui_len width, UIUpdateMode mode = UIUpdateMode::Redraw);

			sl_bool isVisible();
			void setVisible(sl_bool flag = sl_true, UIUpdateMode mode = UIUpdateMode::Redraw);

			sl_bool isResizable();
			void setResizable(sl_bool flag = sl_true);

			sl_bool isBodyVerticalGrid();
			sl_bool isHeaderVerticalGrid();
			sl_bool isFooterVerticalGrid();

			void setBodyVerticalGrid(sl_bool flag = sl_true, UIUpdateMode mode = UIUpdateMode::Redraw);
			void setHeaderVerticalGrid(sl_bool flag = sl_true, UIUpdateMode mode = UIUpdateMode::Redraw);
			void setFooterVerticalGrid(sl_bool flag = sl_true, UIUpdateMode mode = UIUpdateMode::Redraw);
			void setVerticalGrid(sl_bool flag = sl_true, UIUpdateMode mode = UIUpdateMode::Redraw);

		private:
			void _invalidate(UIUpdateMode mode);
			
			void _invalidateLayout(UIUpdateMode mode);

		private:
			WeakRef<GridView> m_view;
			sl_int32 m_index;

			sl_ui_len m_width;
			sl_ui_len m_fixedWidth;
			sl_bool m_flagDefaultWidth;
			sl_ui_len m_minWidth;
			sl_ui_len m_maxWidth;
			sl_bool m_flagVisible;
			sl_bool m_flagResizable;
			sl_bool m_flagBodyVerticalGrid;
			sl_bool m_flagHeaderVerticalGrid;
			sl_bool m_flagFooterVerticalGrid;

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
			void setVisible(sl_bool flag = sl_true, UIUpdateMode mode = UIUpdateMode::Redraw);

			sl_bool isHorizontalGrid();
			void setHorizontalGrid(sl_bool flag = sl_true, UIUpdateMode mode = UIUpdateMode::Redraw);

		private:
			void _invalidate(UIUpdateMode mode);

			void _invalidateLayout(UIUpdateMode mode);

		public:
			WeakRef<GridView> m_view;
			RecordIndex m_section;
			sl_int32 m_index;

			sl_ui_len m_height;
			sl_ui_len m_fixedHeight;
			sl_bool m_flagVisible;
			sl_bool m_flagHorizontalGrid;

			CellProp m_defaultProps;

			friend class GridView;
		};

	private:
		sl_bool _inheritColumn(Column* col);

		sl_ui_len _getDefaultRowHeight();

		void _setData(const VariantList& list);
		void _setData(const List<VariantMap>& list);
		void _setData(const Variant& data);

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
		void _sort(Cell* cell);

		void _fixBodyStartMidColumn(Ref<Column>* columns, sl_uint32 nColumns, sl_uint32 nLeft, sl_uint32 iStart, sl_uint32& newStart);
		void _fixHeaderStartMidColumn(Ref<Column>* columns, sl_uint32 nColumns, sl_uint32 nLeft, sl_uint32 iStart, sl_uint32& newStart);
		void _fixFooterStartMidColumn(Ref<Column>* columns, sl_uint32 nColumns, sl_uint32 nLeft, sl_uint32 iStart, sl_uint32& newStart);

		void _drawRecords(Canvas* canvas, sl_ui_len top, sl_ui_len bottom, Ref<Column>* columns, sl_uint32 nColumns, sl_uint32 nLeft, sl_ui_pos xLeft, sl_uint32 nRight, sl_ui_pos xRight, sl_uint32 iStartMidColumn, sl_uint32 nMidColumns, sl_ui_pos xStartMidColumn, sl_bool flagExtendMid);
		void _drawHeader(Canvas* canvas, sl_ui_len top, sl_ui_len bottom, Ref<Column>* columns, sl_uint32 nColumns, sl_uint32 nLeft, sl_ui_pos xLeft, sl_uint32 nRight, sl_ui_pos xRight, sl_uint32 iStartMidColumn, sl_uint32 nMidColumns, sl_ui_pos xStartMidColumn, sl_bool flagExtendMid);
		void _drawFooter(Canvas* canvas, sl_ui_len top, sl_ui_len bottom, Ref<Column>* columns, sl_uint32 nColumns, sl_uint32 nLeft, sl_ui_pos xLeft, sl_uint32 nRight, sl_ui_pos xRight, sl_uint32 iStartMidColumn, sl_uint32 nMidColumns, sl_ui_pos xStartMidColumn, sl_bool flagExtendMid);

		void _drawBodyColumn(Canvas* canvas, sl_ui_pos x, sl_ui_pos y, Column* column, sl_uint32 iColumn, Ref<Row>* rows, sl_uint32 nRows, sl_uint64 iRecord, const Variant& recordData);
		void _drawHeaderColumn(Canvas* canvas, sl_ui_pos x, sl_ui_pos y, Column* column, sl_uint32 iColumn, Ref<Row>* rows, sl_uint32 nRows);
		void _drawFooterColumn(Canvas* canvas, sl_ui_pos x, sl_ui_pos y, Column* column, sl_uint32 iColumn, Ref<Row>* rows, sl_uint32 nRows);
		void _drawExtendedColumn(Canvas* canvas, sl_ui_pos x, sl_ui_pos xExtend, sl_ui_pos y, Ref<Row>* rows, sl_uint32 nRows, RecordIndex record, const Variant& recordData);

		void _drawBodyInnerGrid(Canvas* canvas, sl_ui_pos x, sl_ui_pos xExtend, sl_ui_pos top, sl_ui_pos bottom, Ref<Column>* columns, sl_uint32 nColumns, Ref<Row>* rows, sl_uint32 nRows, sl_uint32 nRecords, sl_bool flagBody, const Ref<Pen>& pen);
		void _drawHeaderInnerGrid(Canvas* canvas, sl_ui_pos x, sl_ui_pos xExtend, sl_ui_pos top, sl_ui_pos bottom, Ref<Column>* columns, sl_uint32 nColumns, Ref<Row>* rows, sl_uint32 nRows, sl_uint32 nRecords, sl_bool flagBody, const Ref<Pen>& pen);
		void _drawFooterInnerGrid(Canvas* canvas, sl_ui_pos x, sl_ui_pos xExtend, sl_ui_pos top, sl_ui_pos bottom, Ref<Column>* columns, sl_uint32 nColumns, Ref<Row>* rows, sl_uint32 nRows, sl_uint32 nRecords, sl_bool flagBody, const Ref<Pen>& pen);

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
		sl_bool _processResizingColumn(UIEvent* ev);

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

		sl_bool m_defaultBodyVerticalGrid;
		sl_bool m_defaultHeaderVerticalGrid;
		sl_bool m_defaultFooterVerticalGrid;
		sl_bool m_defaultBodyHorizontalGrid;
		sl_bool m_defaultHeaderHorizontalGrid;
		sl_bool m_defaultFooterHorizontalGrid;

		CellProp m_defaultBodyProps;
		CellProp m_defaultHeaderProps;
		CellProp m_defaultFooterProps;

		AtomicRef<Pen> m_gridBody;
		AtomicRef<Pen> m_gridHeader;
		AtomicRef<Pen> m_gridFooter;
		AtomicRef<Pen> m_gridLeft;
		AtomicRef<Pen> m_gridRight;
		sl_bool m_flagSetGridBody;
		sl_bool m_flagSetGridHeader;
		sl_bool m_flagSetGridFooter;
		sl_bool m_flagSetGridLeft;
		sl_bool m_flagSetGridRight;
		AtomicRef<Pen> m_selectionBorder;

		sl_bool m_flagSorting;
		sl_bool m_flagDefinedSorting;
		AtomicRef<Drawable> m_iconAsc;
		AtomicRef<Drawable> m_iconDesc;
		sl_ui_len m_sortIconSize;

		typedef Function<Variant(sl_uint64 record)> DataGetter;
		Atomic<DataGetter> m_recordData;
		AtomicList<Variant> m_cacheData;

		SelectionMode m_selectionMode;
		Selection m_hover;
		Selection m_selection;

		sl_bool m_flagInitialize;
		sl_bool m_flagInvalidateBodyLayout;
		sl_bool m_flagInvalidateHeaderLayout;
		sl_bool m_flagInvalidateFooterLayout;
		Ref<Font> m_currentFont;

		struct ResizingColumn
		{
			sl_int32 index;
			sl_bool flagRight;
			sl_bool flagDual;
			sl_ui_len formerWidth;
			sl_ui_len formerWidth2;
			sl_ui_pos formerEventX;
		} m_resizingColumn;

		CellAttribute* m_cellSort;
		sl_bool m_flagSortAsc;
	};

}

#endif
