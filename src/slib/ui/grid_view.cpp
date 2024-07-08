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

#include "slib/ui/grid_view.h"

#include "slib/ui/priv/view_state_map.h"
#include "slib/ui/cursor.h"
#include "slib/ui/clipboard.h"
#include "slib/ui/core.h"
#include "slib/graphics/canvas.h"
#include "slib/core/safe_static.h"

#define DEFAULT_TEXT_COLOR Color::Black
#define MAX_RECORDS_PER_SCREEN 1000
#define COLUMN_RESIZER_SIZE 3

namespace slib
{

	namespace {

		static const sl_real g_colorMatrix_hover_buf[20] = {
			0.5f, 0, 0, 0,
			0, 0.5f, 0, 0,
			0, 0, 0.5f, 0,
			0, 0, 0, 1,
			0.2f, 0.3f, 0.4f, 0
		};
		static const ColorMatrix& g_colorMatrix_hover = *((const ColorMatrix*)((void*)g_colorMatrix_hover_buf));

		static const sl_real g_colorMatrix_pressed_buf[20] = {
			0.5f, 0, 0, 0,
			0, 0.5f, 0, 0,
			0, 0, 0.5f, 0,
			0, 0, 0, 1,
			0.3f, 0.4f, 0.6f, 0
		};
		static const ColorMatrix& g_colorMatrix_pressed = *((const ColorMatrix*)((void*)g_colorMatrix_pressed_buf));

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

		class DefaultSortIcon : public Drawable
		{
		public:
			Ref<Brush> m_brush;
			Point m_pts[3];

		public:
			DefaultSortIcon(const Color& color, sl_bool flagAsc)
			{
				m_brush = Brush::createSolidBrush(color);
				if (flagAsc) {
					m_pts[0] = Point(0.2f, 0.7f);
					m_pts[1] = Point(0.5f, 0.3f);
					m_pts[2] = Point(0.8f, 0.7f);
				} else {
					m_pts[0] = Point(0.2f, 0.3f);
					m_pts[1] = Point(0.5f, 0.7f);
					m_pts[2] = Point(0.8f, 0.3f);
				}
			}

		public:
			sl_real getDrawableWidth() override
			{
				return 16;
			}

			sl_real getDrawableHeight() override
			{
				return 16;
			}

			void onDrawAll(Canvas* canvas, const Rectangle& rectDst, const DrawParam& param) override
			{
				if (m_brush.isNotNull()) {
					Point pts[3];
					for (int i = 0; i < 3; i++) {
						pts[i].x = rectDst.left + rectDst.getWidth() * m_pts[i].x;
						pts[i].y = rectDst.top + rectDst.getHeight() * m_pts[i].y;
					}
					canvas->fillPolygon(pts, 3, m_brush);
				}
			}

		};

	}

	SLIB_DEFINE_NESTED_CLASS_DEFAULT_MEMBERS(GridView, DrawCellParam)

	GridView::DrawCellParam::DrawCellParam(): state(ViewState::Normal)
	{
	}

	SLIB_DEFINE_NESTED_CLASS_DEFAULT_MEMBERS(GridView, CellAttribute)

	GridView::CellAttribute::CellAttribute(): multiLineMode(MultiLineMode::Single), ellipsizeMode(EllipsizeMode::None), lineCount(0), align(Alignment::MiddleCenter), flagSelectable(sl_false), flagEditable(sl_false), flagBackgroundAntiAlias(sl_false), flagContentAntiAlias(sl_true), iconWidth(-1), iconScale(ScaleMode::Contain), iconAlign(Alignment::Default), colspan(1), rowspan(1), width(0), height(0)
	{
		padding.left = padding.top = padding.right = padding.bottom = 1;
		iconMargin.left = iconMargin.top = iconMargin.right = iconMargin.bottom = 2;
	}

	SLIB_DEFINE_NESTED_CLASS_DEFAULT_MEMBERS(GridView, CellParam)

	GridView::CellParam::CellParam(): view(sl_null), attr(sl_null), record(0), row(0), column(0)
	{
	}

	GridView::Cell::Cell(): m_defaultTextColor(Color::Black), m_flagSelectable(sl_false), m_flagDefaultFilter(sl_false), m_flagUseContentState(sl_false)
	{
	}

	GridView::Cell::~Cell()
	{
	}

	Variant GridView::Cell::getValue()
	{
		String& field = attr->field;
		if (field.isNotEmpty() && record >= 0 && recordData.isNotUndefined()) {
			return recordData.getItemByPath(field);
		}
		return Variant();
	}

	String GridView::Cell::getText()
	{
		TextGetter& textGetter = attr->textGetter;
		if (textGetter.isNotNull()) {
			return textGetter(this);
		} else {
			return getInternalText();
		}
	}

	String GridView::Cell::getInternalText()
	{
		String& text = attr->text;
		if (text.isNotNull()) {
			if (record >= 0 && recordData.isNotUndefined()) {
				return String::format(text, recordData);
			} else {
				return text;
			}
		} else {
			String& field = attr->field;
			if (field.isNotEmpty() && record >= 0 && recordData.isNotUndefined()) {
				return recordData.getItemByPath(field).toString();
			}
		}
		return sl_null;
	}

	String GridView::Cell::getToolTip()
	{
		TextGetter& toolTipGetter = attr->toolTipGetter;
		if (toolTipGetter.isNotNull()) {
			return toolTipGetter(this);
		} else {
			return getInternalToolTip();
		}
	}

	String GridView::Cell::getInternalToolTip()
	{
		String& toolTip = attr->toolTip;
		if (toolTip.isNotNull()) {
			if (record >= 0 && recordData.isNotUndefined()) {
				return String::format(toolTip, recordData);
			} else {
				return toolTip;
			}
		}
		return sl_null;
	}

	Ref<Drawable> GridView::Cell::getBackground(ViewState state)
	{
		DrawableGetter& getter = attr->backgroundGetter;
		if (getter.isNotNull()) {
			return filter(getter(this, state), state, sl_false);
		} else {
			return getInternalBackground(state);
		}
	}

	Ref<Drawable> GridView::Cell::getInternalBackground(ViewState state)
	{
		return attr->backgrounds.evaluate(state, sl_null);
	}

	Color GridView::Cell::getTextColor(ViewState state)
	{
		ColorGetter& getter = attr->textColorGetter;
		if (getter.isNotNull()) {
			return filter(getter(this, state), state, sl_false);
		} else {
			return getInternalTextColor(state);
		}
	}

	Color GridView::Cell::getInternalTextColor(ViewState state)
	{
		sl_bool flagDefault = sl_false;
		Color color = attr->textColors.evaluate(state, &flagDefault);
		if (flagDefault && color.isZero()) {
			color = m_defaultTextColor;
		}
		return filter(color, state, flagDefault);
	}

	Ref<Drawable> GridView::Cell::getIcon(ViewState state)
	{
		DrawableGetter& getter = attr->iconGetter;
		if (getter.isNotNull()) {
			return filter(getter(this, state), state, sl_false);
		} else {
			return getInternalIcon(state);
		}
	}

	Ref<Drawable> GridView::Cell::getInternalIcon(ViewState state)
	{
		sl_bool flagDefault = sl_false;
		Ref<Drawable> icon = attr->icons.evaluate(state, &flagDefault);
		return filter(icon, state, flagDefault);
	}

	sl_bool GridView::Cell::getColorFilter(ColorMatrix& _out, ViewState state, sl_bool flagDefaultFilter)
	{
		Shared<ColorMatrix> cm = attr->filters.evaluate(state);
		if (cm.isNotNull()) {
			_out = *cm;
			return sl_true;
		}
		if (!flagDefaultFilter) {
			return sl_false;
		}
		if (!m_flagDefaultFilter) {
			return sl_false;
		}
		switch (state) {
			case ViewState::Hover:
			case ViewState::FocusedHover:
			case ViewState::SelectedHover:
				_out = g_colorMatrix_hover;
				break;
			case ViewState::Pressed:
			case ViewState::FocusedPressed:
			case ViewState::SelectedPressed:
				_out = g_colorMatrix_pressed;
				break;
			default:
				return sl_false;
		}
		return sl_true;
	}

	Ref<Drawable> GridView::Cell::filter(const Ref<Drawable>& drawable, ViewState state, sl_bool flagDefaultFilter)
	{
		if (drawable.isNull()) {
			return sl_null;
		}
		ColorMatrix cm;
		if (getColorFilter(cm, state, flagDefaultFilter)) {
			return drawable->filter(cm);
		} else {
			return drawable;
		}
	}

	Color GridView::Cell::filter(const Color& color, ViewState state, sl_bool flagDefaultFilter)
	{
		ColorMatrix cm;
		if (getColorFilter(cm, state, flagDefaultFilter)) {
			return cm.transformColor(color);
		} else {
			return color;
		}
	}

	void GridView::Cell::draw(Canvas* canvas, DrawParam& param)
	{
		UIRect frame(param.x, param.y, param.x + attr->width, param.y + attr->height);
		Ref<Drawable> background = getBackground(param.state);
		if (background.isNotNull()) {
			CanvasAntiAliasScope scope(canvas, attr->flagBackgroundAntiAlias);
			canvas->draw(frame, background);
		}
		CanvasAntiAliasScope scope(canvas, attr->flagContentAntiAlias);
		Ref<Drawable> icon = getIcon(param.contentState);
		if (icon.isNotNull()) {
			UIRect iconFrame = frame;
			iconFrame.left += m_iconFrame.left;
			iconFrame.top += m_iconFrame.top;
			iconFrame.setWidth(m_iconFrame.getWidth());
			iconFrame.setHeight(m_iconFrame.getHeight());
			canvas->draw(iconFrame, icon, attr->iconScale, attr->iconAlign);
		}
		frame.left += m_contentFrame.left;
		frame.top += m_contentFrame.top;
		frame.setWidth(m_contentFrame.getWidth());
		frame.setHeight(m_contentFrame.getHeight());
		onDrawContent(canvas, frame, param);
	}

	void GridView::Cell::onInit()
	{
		m_flagSelectable = attr->flagSelectable;
		m_contentFrame.left = attr->padding.left;
		m_contentFrame.top = attr->padding.top;
		m_contentFrame.right = attr->width - attr->padding.right;
		m_contentFrame.bottom = attr->height - attr->padding.bottom;
		if (attr->iconGetter.isNotNull() || attr->icons.isNotNone()) {
			sl_ui_len iconWidth = attr->iconWidth;
			if (iconWidth < 0) {
				if (font.isNotNull()) {
					iconWidth = (sl_ui_len)(font->getFontHeight());
				} else {
					iconWidth = 0;
				}
			}
			m_iconFrame.left = attr->padding.left + attr->iconMargin.left;
			m_iconFrame.top = attr->padding.top + attr->iconMargin.top;
			m_iconFrame.right = m_iconFrame.left + iconWidth;
			m_iconFrame.bottom = m_contentFrame.bottom - attr->iconMargin.bottom;
			m_contentFrame.left = m_iconFrame.right + attr->iconMargin.right;
		} else {
			m_iconFrame.setZero();
		}
		if (attr->flagDefaultFilter.isNotNull()) {
			m_flagDefaultFilter = attr->flagDefaultFilter.value;
		}
	}

	void GridView::Cell::onDrawContent(Canvas* canvas, const UIRect& frame, DrawParam& param)
	{
	}

	void GridView::Cell::onClick(UIEvent* ev)
	{
	}

	void GridView::Cell::onEvent(UIEvent* ev)
	{
		UIAction action = ev->getAction();
		if (action == UIAction::SetCursor) {
			String toolTip = getToolTip();
			if (toolTip.isNotNull()) {
				ev->setToolTip((sl_uint64)((void*)this), toolTip);
			}
			Ref<Cursor>& cursor = attr->cursor;
			if (cursor.isNotNull()) {
				ev->setCursor(cursor);
				ev->accept();
			}
		}
	}

	void GridView::Cell::onCopy()
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
		Cell::onInit();
		TextBoxParam tp;
		tp.width = (sl_real)(m_contentFrame.getWidth());
		onPrepareTextBox(tp);
		m_textBox.update(tp);
	}

	void GridView::TextCell::onDrawContent(Canvas* canvas, const UIRect& frame, DrawParam& param)
	{
		TextBox::DrawParam tp;
		tp.frame = frame;
		tp.textColor = getTextColor(param.contentState);
		m_textBox.draw(canvas, tp);
	}

	void GridView::TextCell::onCopy()
	{
		Clipboard::setText(m_textBox.getText());
	}

	void GridView::TextCell::onPrepareTextBox(TextBoxParam& param)
	{
		param.text = getText();
		param.font = font;
		param.align = attr->align;
		param.multiLineMode = attr->multiLineMode;
		param.ellipsizeMode = attr->ellipsizeMode;
		param.lineCount = attr->lineCount;
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

	GridView::SortCell::SortCell()
	{
	}

	GridView::SortCell::~SortCell()
	{
	}

	const GridView::CellCreator& GridView::SortCell::creator()
	{
		SLIB_SAFE_LOCAL_STATIC(CellCreator, ret, [](CellParam&) {
			return new SortCell;
		})
		return ret;
	}

	void GridView::SortCell::onDrawContent(Canvas* canvas, const UIRect& frame, DrawParam& param)
	{
		if (attr->field.isNotEmpty() && view->m_flagSorting && view->m_cellSort == attr) {
			Ref<Drawable> icon;
			if (view->m_flagSortAsc) {
				icon = view->m_iconAsc;
			} else {
				icon = view->m_iconDesc;
			}
			if (icon.isNotNull()) {
				UIRect textFrame = frame;
				textFrame.right -= view->getSortIconSize();
				TextCell::onDrawContent(canvas, textFrame, param);
				UIRect iconFrame = frame;
				iconFrame.left = textFrame.right;
				CanvasAntiAliasScope scope(canvas, sl_true);
				canvas->draw(iconFrame, icon, ScaleMode::Contain, Alignment::MiddleCenter);
				return;
			}
		}
		TextCell::onDrawContent(canvas, frame, param);
	}

	void GridView::SortCell::onClick(UIEvent* ev)
	{
		if (attr->field.isEmpty() || !(view->m_flagSorting)) {
			return;
		}
		view->_sort(this);
	}

	GridView::IconCell::IconCell()
	{
	}

	GridView::IconCell::~IconCell()
	{
	}

	const GridView::CellCreator& GridView::IconCell::creator()
	{
		SLIB_SAFE_LOCAL_STATIC(CellCreator, ret, [](CellParam&) {
			return new IconCell;
		})
		return ret;
	}

	void GridView::IconCell::onInit()
	{
		Cell::onInit();
		m_iconFrame.right = attr->width - attr->padding.right - attr->iconMargin.right;
		if (attr->iconAlign == Alignment::Default) {
			attr->iconAlign = attr->align;
		}
	}

	GridView::ButtonCell::ButtonCell()
	{
		m_defaultTextColor = Color(0, 100, 200);
		m_flagDefaultFilter = sl_true;
		m_flagUseContentState = sl_true;
	}

	GridView::ButtonCell::~ButtonCell()
	{
	}

	const GridView::CellCreator& GridView::ButtonCell::creator()
	{
		SLIB_SAFE_LOCAL_STATIC(CellCreator, ret, [](CellParam&) {
			return new ButtonCell;
		})
		return ret;
	}

	void GridView::ButtonCell::onInit()
	{
		TextCell::onInit();
		m_flagSelectable = sl_false;
		if (attr->textGetter.isNull() && attr->text.isNull()) {
			m_iconFrame.right = attr->width - attr->padding.right - attr->iconMargin.right;
			if (attr->iconAlign == Alignment::Default) {
				attr->iconAlign = attr->align;
			}
		}
	}

	void GridView::ButtonCell::onEvent(UIEvent* ev)
	{
		TextCell::onEvent(ev);
		if (ev->isAccepted()) {
			return;
		}
		UIAction action = ev->getAction();
		if (action == UIAction::SetCursor) {
			ev->setCursor(Cursor::getHand());
			ev->accept();
		}
	}


	SLIB_DEFINE_NESTED_CLASS_DEFAULT_MEMBERS(GridView, CellProp)

	GridView::CellProp::CellProp(): flagCoveredX(sl_false), flagCoveredY(sl_false)
	{
	}

	void GridView::CellProp::inheritFrom(const CellProp& other)
	{
		creator = other.creator;
		field = other.field;
		text = other.text;
		textGetter = other.textGetter;
		font = other.font;
		cursor = other.cursor;
		toolTip = other.toolTip;
		toolTipGetter = other.toolTipGetter;
		padding = other.padding;
		multiLineMode = other.multiLineMode;
		ellipsizeMode = other.ellipsizeMode;
		lineCount = other.lineCount;
		align = other.align;
		flagSelectable = other.flagSelectable;
		flagEditable = other.flagEditable;
		flagBackgroundAntiAlias = other.flagBackgroundAntiAlias;
		flagContentAntiAlias = other.flagContentAntiAlias;
		flagDefaultFilter = other.flagDefaultFilter;
		backgroundGetter = other.backgroundGetter;
		textColorGetter = other.textColorGetter;
		iconGetter = other.iconGetter;
		iconWidth = other.iconWidth;
		iconMargin = other.iconMargin;
		iconScale = other.iconScale;
		iconAlign = other.iconAlign;
		backgrounds.copyFrom(other.backgrounds);
		textColors.copyFrom(other.textColors);
		filters.copyFrom(other.filters);
		icons.copyFrom(other.icons);
	}
	
	SLIB_DEFINE_NESTED_CLASS_DEFAULT_MEMBERS(GridView, BodyCellProp)

	GridView::BodyCellProp::BodyCellProp()
	{
	}

	SLIB_DEFINE_NESTED_CLASS_DEFAULT_MEMBERS(GridView, FixedCellProp)

	GridView::FixedCellProp::FixedCellProp(): flagMadeCell(sl_false)
	{
	}


	GridViewColumn::GridViewColumn(GridView* view): m_view(view)
	{
		m_index = -1;
		m_width = 0;
		m_fixedWidth = 0;
		m_flagDefaultWidth = sl_true;
		m_flagVisible = sl_true;
		m_flagResizable = sl_true;
		m_flagBodyVerticalGrid = sl_true;
		m_flagHeaderVerticalGrid = sl_true;
		m_flagFooterVerticalGrid = sl_true;
	}

	GridViewColumn::~GridViewColumn()
	{
	}

	Ref<GridView> GridViewColumn::getView()
	{
		return m_view;
	}

	sl_uint32 GridViewColumn::getIndex()
	{
		return m_index;
	}

	void GridViewColumn::_invalidate(UIUpdateMode mode)
	{
		if (SLIB_UI_UPDATE_MODE_IS_INIT(mode)) {
			return;
		}
		if (m_index < 0) {
			return;
		}
		Ref<GridView> view = m_view;
		if (view.isNull()) {
			return;
		}
		view->invalidate(mode);
	}

	void GridViewColumn::_invalidateLayout(UIUpdateMode mode)
	{
		if (SLIB_UI_UPDATE_MODE_IS_INIT(mode)) {
			return;
		}
		if (m_index < 0) {
			return;
		}
		Ref<GridView> view = m_view;
		if (view.isNull()) {
			return;
		}
		ObjectLocker lock(view.get());
		view->_invalidateLayout();
		view->refreshContentWidth(mode);
	}


	GridViewRow::GridViewRow(GridView* view): m_view(view)
	{
		m_section = GridView::OUTSIDE;
		m_height = -1;
		m_index = -1;
		m_fixedHeight = 0;
		m_flagVisible = sl_true;
		m_flagHorizontalGrid = sl_true;
	}

	GridViewRow::~GridViewRow()
	{
	}

	Ref<GridView> GridViewRow::getView()
	{
		return m_view;
	}

	sl_bool GridViewRow::isBody()
	{
		return m_section == GridView::BODY;
	}

	sl_bool GridViewRow::isHeader()
	{
		return m_section == GridView::HEADER;
	}

	sl_bool GridViewRow::isFooter()
	{
		return m_section == GridView::FOOTER;
	}

	sl_uint32 GridViewRow::getIndex()
	{
		return m_index;
	}

	void GridViewRow::_invalidate(UIUpdateMode mode)
	{
		if (SLIB_UI_UPDATE_MODE_IS_INIT(mode)) {
			return;
		}
		if (m_index < 0) {
			return;
		}
		Ref<GridView> view = m_view;
		if (view.isNull()) {
			return;
		}
		view->invalidate(mode);
	}

	void GridViewRow::_invalidateLayout(UIUpdateMode mode)
	{
		if (SLIB_UI_UPDATE_MODE_IS_INIT(mode)) {
			return;
		}
		if (m_index < 0) {
			return;
		}
		Ref<GridView> view = m_view;
		if (view.isNull()) {
			return;
		}
		ObjectLocker lock(view.get());
		if (m_section == GridView::HEADER) {
			view->m_flagInvalidateHeaderLayout = sl_true;
		} else if (m_section == GridView::FOOTER) {
			view->m_flagInvalidateFooterLayout = sl_true;
		} else {
			view->m_flagInvalidateBodyLayout = sl_true;
		}
		view->refreshContentHeight(mode);
	}


	SLIB_DEFINE_NESTED_CLASS_DEFAULT_MEMBERS(GridView, Selection)

	GridView::Selection::Selection(): record(GridView::OUTSIDE), row(-1), column(-1)
	{
	}

	sl_bool GridView::Selection::operator==(const Selection& other) const
	{
		return row == other.row && column == other.column && record == other.record;
	}

	sl_bool GridView::Selection::isNone() const
	{
		return record == OUTSIDE && row < 0 && column < 0;
	}

	sl_bool GridView::Selection::isCell() const
	{
		return record != OUTSIDE && row >= 0 && column >= 0;
	}

	sl_bool GridView::Selection::match(RecordIndex _record, sl_int32 _row, sl_int32 _column) const
	{
		if (isNone()) {
			return sl_false;
		}
		if (record != OUTSIDE && _record != OUTSIDE && record != _record) {
			return sl_false;
		}
		if (row >= 0 && _row >= 0 && row != _row) {
			return sl_false;
		}
		if (column >= 0 && _column >= 0 && column != _column) {
			return sl_false;
		}
		return sl_true;
	}

	sl_bool GridView::Selection::match(Cell* cell) const
	{
		return match(cell->record, cell->row, cell->column);
	}

	sl_bool GridView::Selection::matchCell(RecordIndex _record, sl_uint32 _row, sl_uint32 _column) const
	{
		if (!(isCell())) {
			return sl_false;
		}
		if (record != _record) {
			return sl_false;
		}
		if (row != _row) {
			return sl_false;
		}
		if (column != _column) {
			return sl_false;
		}
		return sl_true;
	}

	sl_bool GridView::Selection::matchCell(Cell* cell) const
	{
		return matchCell(cell->record, cell->row, cell->column);
	}


	SLIB_DEFINE_OBJECT(GridView, View)

	GridView::GridView()
	{
		m_nRecords = 0;
		m_nLeftColumns = 0;
		m_nRightColumns = 0;

		m_flagSetGridBody = sl_false;
		m_flagSetGridHeader = sl_false;
		m_flagSetGridFooter = sl_false;
		m_flagSetGridLeft = sl_false;
		m_flagSetGridRight = sl_false;

		m_selectionBorder = Pen::create(PenStyle::Solid, 3, Color(33, 115, 70));
		m_selectionMode = SelectionMode::Cell;

		m_flagSorting = sl_true;
		m_flagDefinedSorting = sl_false;
		m_iconAsc = new DefaultSortIcon(Color::Black, sl_true);
		m_iconDesc = new DefaultSortIcon(Color::Black, sl_false);
		m_sortIconSize = -1;

		m_defaultColumnWidth = 80;
		m_defaultColumnMinWidth = 30;
		m_defaultColumnMaxWidth = -1;
		m_defaultColumnResizable = sl_true;

		m_defaultBodyRowHeight = -1;
		m_defaultHeaderRowHeight = -1;
		m_defaultFooterRowHeight = -1;

		m_defaultBodyVerticalGrid = sl_true;
		m_defaultHeaderVerticalGrid = sl_true;
		m_defaultFooterVerticalGrid = sl_true;
		m_defaultBodyHorizontalGrid = sl_true;
		m_defaultHeaderHorizontalGrid = sl_true;
		m_defaultFooterHorizontalGrid = sl_true;

		m_defaultBodyProps.creator = TextCell::creator();
		m_defaultBodyProps.flagSelectable = sl_true;
		m_defaultBodyProps.backgrounds.set(ViewState::Selected, Drawable::fromColor(Color(200, 235, 255)));
		m_defaultBodyProps.backgrounds.set(ViewState::Hover, Drawable::fromColor(Color(235, 250, 255)));
		m_defaultHeaderProps.creator = SortCell::creator();
		m_defaultHeaderProps.backgrounds.setDefault(Drawable::fromColor(Color(230, 230, 230)));
		m_defaultFooterProps.creator = TextCell::creator();
		m_defaultFooterProps.backgrounds.setDefault(Drawable::fromColor(Color(240, 240, 240)));

		m_flagInvalidateBodyLayout = sl_true;
		m_flagInvalidateHeaderLayout = sl_true;
		m_flagInvalidateFooterLayout = sl_true;

		m_flagInitialize = sl_true;

		m_resizingColumn.index = -1;
		m_cellSort = sl_null;
		m_flagSortAsc = sl_false;
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

	sl_bool GridView::_inheritColumn(Column* col)
	{
#define INHERIT_COLUMN_SECTION(SECTION) \
		col->m_width = m_defaultColumnWidth; \
		col->m_minWidth = m_defaultColumnMinWidth; \
		col->m_maxWidth = m_defaultColumnMaxWidth; \
		col->m_flagResizable = m_defaultColumnResizable; \
		col->m_flagBodyVerticalGrid = m_defaultBodyVerticalGrid; \
		col->m_flagHeaderVerticalGrid = m_defaultHeaderVerticalGrid; \
		col->m_flagFooterVerticalGrid = m_defaultFooterVerticalGrid; \
		col->m_default##SECTION##Props.inheritFrom(m_default##SECTION##Props); \
		{ \
			sl_uint32 nRows = (sl_uint32)(m_list##SECTION##Row.getCount()); \
			if (nRows) { \
				if (!(col->m_list##SECTION##Cell.setCount_NoLock(nRows))) { \
					return sl_false; \
				} \
				ListElements< Ref<Row> > rows(m_list##SECTION##Row); \
				if (nRows > rows.count) { \
					nRows = (sl_uint32)(rows.count); \
				} \
				SECTION##CellProp* props = col->m_list##SECTION##Cell.getData(); \
				for (sl_uint32 i = 0; i < nRows; i++) { \
					props[i].inheritFrom(rows[i]->m_defaultProps); \
				} \
			} \
		}

		INHERIT_COLUMN_SECTION(Body)
		INHERIT_COLUMN_SECTION(Header)
		INHERIT_COLUMN_SECTION(Footer)

		return sl_true;
	}

	sl_bool GridView::setColumnCount(sl_uint32 count, UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		sl_uint32 nOldCount = (sl_uint32)(m_columns.getCount());
		if (nOldCount == count) {
			return sl_true;
		}
		if (count > nOldCount) {
			if (!(m_columns.setCount_NoLock(count))) {
				return sl_false;
			}
			Ref<Column>* columns = m_columns.getData();
			for (sl_uint32 iCol = nOldCount; iCol < count; iCol++) {
				Ref<Column> col = new Column(this);
				if (col.isNotNull()) {
					col->m_index = iCol;
					_inheritColumn(col.get());
					columns[iCol] = Move(col);
				} else {
					m_columns.setCount_NoLock(nOldCount);
					return sl_false;
				}
			}
		} else {
			Ref<Column>* columns = m_columns.getData();
			for (sl_uint32 i = count; i < nOldCount; i++) {
				columns[i]->m_index = -1;
			}
			if (!(m_columns.setCount_NoLock(count))) {
				return sl_false;
			}
		}
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

	Ref<GridViewColumn> GridView::getColumn(sl_uint32 index)
	{
		ObjectLocker lock(this);
		return m_columns.getValueAt_NoLock(index);
	}

	Ref<GridViewColumn> GridView::addColumn(UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		sl_uint32 nColumns = (sl_uint32)(m_columns.getCount());
		Ref<Column> col = new Column(this);
		if (col.isNotNull()) {
			col->m_index = nColumns;
			_inheritColumn(col.get());
			if (m_columns.add_NoLock(col)) {
				refreshContentWidth(mode);
				return col;
			}
		}
		return sl_null;
	}

	Ref<GridViewColumn> GridView::insertColumn(sl_uint32 index, UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		sl_uint32 nColumns = (sl_uint32)(m_columns.getCount());
		if (index > nColumns) {
			index = nColumns;
		}
		Ref<Column> col = new Column(this);
		if (col.isNotNull()) {
			col->m_index = index;
			_inheritColumn(col.get());
			if (m_columns.insert_NoLock(index, col)) {
				Ref<Column>* columns = m_columns.getData();
				for (sl_uint32 i = index + 1; i <= nColumns; i++) {
					columns[i]->m_index = i;
				}
				_invalidateLayout();
				refreshContentWidth(mode);
				return col;
			}
		}
		return sl_null;
	}

	sl_bool GridView::removeColumn(sl_uint32 index, UIUpdateMode mode)
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
			_invalidateLayout();
			refreshContentWidth(mode);
			return sl_true;
		}
		return sl_false;
	}

	sl_bool GridViewColumn::remove(UIUpdateMode mode)
	{
		if (m_index >= 0) {
			Ref<GridView> view = m_view;
			if (view.isNotNull()) {
				return view->removeColumn(m_index, mode);
			}
		}
		return sl_false;
	}

	String GridView::getColumnId(sl_uint32 index)
	{
		ObjectLocker lock(this);
		Ref<Column> col = m_columns.getValueAt_NoLock(index);
		if (col.isNotNull()) {
			return col->getId();
		}
		return sl_null;
	}

	String GridViewColumn::getId()
	{
		return m_id;
	}

	void GridView::setColumnId(sl_uint32 index, const String& _id)
	{
		ObjectLocker lock(this);
		Ref<Column> col = m_columns.getValueAt_NoLock(index);
		if (col.isNotNull()) {
			col->setId(_id);
		}
	}

	void GridViewColumn::setId(const String& _id)
	{
		m_id = _id;
	}

	sl_ui_len GridView::getColumnWidth(sl_uint32 index)
	{
		ObjectLocker lock(this);
		Ref<Column> col = m_columns.getValueAt_NoLock(index);
		if (col.isNotNull()) {
			return col->m_width;
		}
		return 0;
	}

	sl_ui_len GridViewColumn::getWidth()
	{
		return m_width;
	}

	void GridView::setColumnWidth(sl_uint32 index, sl_ui_len width, UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		Ref<Column> col = m_columns.getValueAt_NoLock(index);
		if (col.isNotNull()) {
			if (width < 0) {
				col->setDefaultWidth(m_defaultColumnWidth, UIUpdateMode::Init);
			} else {
				col->setWidth(width, UIUpdateMode::Init);
			}
			_invalidateLayout();
			refreshContentWidth(mode);
		}
	}

	void GridViewColumn::setWidth(sl_ui_len width, UIUpdateMode mode)
	{
		if (m_maxWidth >= 0 && width > m_maxWidth) {
			width = m_maxWidth;
		}
		if (width < m_minWidth) {
			width = m_minWidth;
		}
		m_width = width;
		m_flagDefaultWidth = sl_false;
		_invalidateLayout(mode);
	}

	void GridViewColumn::setDefaultWidth(sl_ui_len width, UIUpdateMode mode)
	{
		if (m_maxWidth >= 0 && width > m_maxWidth) {
			width = m_maxWidth;
		}
		if (width < m_minWidth) {
			width = m_minWidth;
		}
		m_width = width;
		m_flagDefaultWidth = sl_true;
		_invalidateLayout(mode);
	}

	void GridView::setColumnWidth(sl_ui_len width, UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		ListElements< Ref<Column> > columns(m_columns);
		for (sl_size i = 0; i < columns.count; i++) {
			columns[i]->setWidth(width, UIUpdateMode::Init);
		}
		if (m_defaultColumnMaxWidth >= 0 && width > m_defaultColumnMaxWidth) {
			width = m_defaultColumnMaxWidth;
		}
		if (width < m_defaultColumnMinWidth) {
			width = m_defaultColumnMinWidth;
		}
		m_defaultColumnWidth = width;
		_invalidateLayout();
		refreshContentWidth(mode);
	}

	sl_ui_len GridView::getMinimumColumnWidth(sl_uint32 index)
	{
		ObjectLocker lock(this);
		Ref<Column> col = m_columns.getValueAt_NoLock(index);
		if (col.isNotNull()) {
			return col->m_minWidth;
		}
		return 0;
	}

	sl_ui_len GridViewColumn::getMinimumWidth()
	{
		return m_minWidth;
	}

	void GridView::setMinimumColumnWidth(sl_uint32 index, sl_ui_len width, UIUpdateMode mode)
	{
		if (width < 0) {
			width = 0;
		}
		ObjectLocker lock(this);
		Ref<Column> col = m_columns.getValueAt_NoLock(index);
		if (col.isNotNull()) {
			col->m_minWidth = width;
			if (col->m_width < width) {
				col->m_width = width;
				_invalidateLayout();
				refreshContentWidth(mode);
			}
		}
	}

	void GridViewColumn::setMinimumWidth(sl_ui_len width, UIUpdateMode mode)
	{
		if (width < 0) {
			width = 0;
		}
		m_minWidth = width;
		if (m_width < width) {
			m_width = width;
			_invalidateLayout(mode);
		}
	}

	void GridView::setMinimumColumnWidth(sl_ui_len width, UIUpdateMode mode)
	{
		if (width < 0) {
			return;
		}
		sl_bool flagChange = sl_false;
		ObjectLocker lock(this);
		ListElements< Ref<Column> > columns(m_columns);
		for (sl_size i = 0; i < columns.count; i++) {
			Column* col = columns[i].get();
			col->m_minWidth = width;
			if (col->m_width < width) {
				col->m_width = width;
				flagChange = sl_true;
			}
		}
		m_defaultColumnMinWidth = width;
		if (m_defaultColumnWidth < width) {
			m_defaultColumnWidth = width;
		}
		if (flagChange) {
			_invalidateLayout();
			refreshContentWidth(mode);
		}
	}

	sl_ui_len GridView::getMaximumColumnWidth(sl_uint32 index)
	{
		ObjectLocker lock(this);
		Ref<Column> col = m_columns.getValueAt_NoLock(index);
		if (col.isNotNull()) {
			return col->m_maxWidth;
		}
		return 0;
	}

	sl_ui_len GridViewColumn::getMaximumWidth()
	{
		return m_maxWidth;
	}

	void GridView::setMaximumColumnWidth(sl_uint32 index, sl_ui_len width, UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		Ref<Column> col = m_columns.getValueAt_NoLock(index);
		if (col.isNotNull()) {
			col->m_maxWidth = width;
			if (width >= 0 && col->m_width > width) {
				col->m_width = width;
				_invalidateLayout();
				refreshContentWidth(mode);
			}
		}
	}

	void GridViewColumn::setMaximumWidth(sl_ui_len width, UIUpdateMode mode)
	{
		m_maxWidth = width;
		if (width >= 0 && m_width > width) {
			m_width = width;
			_invalidateLayout(mode);
		}
	}

	void GridView::setMaximumColumnWidth(sl_ui_len width, UIUpdateMode mode)
	{
		sl_bool flagChange = sl_false;
		ObjectLocker lock(this);
		ListElements< Ref<Column> > columns(m_columns);
		for (sl_size i = 0; i < columns.count; i++) {
			Column* col = columns[i].get();
			col->m_maxWidth = width;
			if (width >= 0 && col->m_width > width) {
				col->m_width = width;
				flagChange = sl_true;
			}
		}
		m_defaultColumnMaxWidth = width;
		if (width >= 0 && m_defaultColumnWidth > width) {
			m_defaultColumnWidth = width;
		}
		if (flagChange) {
			_invalidateLayout();
			refreshContentWidth(mode);
		}
	}

	sl_bool GridView::isColumnVisible(sl_uint32 index)
	{
		ObjectLocker lock(this);
		Ref<Column> col = m_columns.getValueAt_NoLock(index);
		if (col.isNotNull()) {
			return col->m_flagVisible;
		} else {
			return sl_false;
		}
	}

	sl_bool GridViewColumn::isVisible()
	{
		return m_flagVisible;
	}

	void GridView::setColumnVisible(sl_uint32 index, sl_bool flag, UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		Ref<Column> col = m_columns.getValueAt_NoLock(index);
		if (col.isNotNull()) {
			col->m_flagVisible = flag;
			_invalidateLayout();
			refreshContentWidth(mode);
		}
	}

	void GridViewColumn::setVisible(sl_bool flag, UIUpdateMode mode)
	{
		m_flagVisible = flag;
		_invalidateLayout(mode);
	}

	sl_bool GridView::isColumnResizable(sl_uint32 index)
	{
		ObjectLocker lock(this);
		Ref<Column> col = m_columns.getValueAt_NoLock(index);
		if (col.isNotNull()) {
			return col->m_flagResizable;
		} else {
			return sl_false;
		}
	}

	sl_bool GridViewColumn::isResizable()
	{
		return m_flagResizable;
	}

	void GridView::setColumnResizable(sl_uint32 index, sl_bool flag)
	{
		ObjectLocker lock(this);
		Ref<Column> col = m_columns.getValueAt_NoLock(index);
		if (col.isNotNull()) {
			col->m_flagResizable = flag;
		}
	}

	void GridViewColumn::setResizable(sl_bool flag)
	{
		m_flagResizable = flag;
	}

	void GridView::setColumnResizable(sl_bool flag)
	{
		ObjectLocker lock(this);
		ListElements< Ref<Column> > columns(m_columns);
		for (sl_size i = 0; i < columns.count; i++) {
			columns[i]->m_flagResizable = flag;
		}
		m_defaultColumnResizable = flag;
	}

#define DEFINE_GET_SET_VERTICAL_GRID(SECTION) \
	sl_bool GridView::is##SECTION##VerticalGrid(sl_uint32 iCol) \
	{ \
		ObjectLocker lock(this); \
		Ref<Column> col = m_columns.getValueAt_NoLock(iCol); \
		if (col.isNotNull()) { \
			return col->m_flag##SECTION##VerticalGrid; \
		} else { \
			return sl_false; \
		} \
	} \
	sl_bool GridViewColumn::is##SECTION##VerticalGrid() \
	{ \
		return m_flag##SECTION##VerticalGrid; \
	} \
	void GridView::set##SECTION##VerticalGrid(sl_uint32 iCol, sl_bool flag, UIUpdateMode mode) \
	{ \
		ObjectLocker lock(this); \
		Ref<Column> col = m_columns.getValueAt_NoLock(iCol); \
		if (col.isNotNull()) { \
			col->m_flag##SECTION##VerticalGrid = flag; \
			invalidate(mode); \
		} \
	} \
	void GridViewColumn::set##SECTION##VerticalGrid(sl_bool flag, UIUpdateMode mode) \
	{ \
		m_flag##SECTION##VerticalGrid = flag; \
		_invalidate(mode); \
	} \
	void GridView::set##SECTION##VerticalGrid(sl_bool flag, UIUpdateMode mode) \
	{ \
		ObjectLocker lock(this); \
		ListElements< Ref<Column> > columns(m_columns); \
		for (sl_size iCol = 0; iCol < columns.count; iCol++) { \
			Column* col = columns[iCol].get(); \
			col->m_flag##SECTION##VerticalGrid = flag; \
		} \
		m_default##SECTION##VerticalGrid = flag; \
		invalidate(mode); \
	}

	DEFINE_GET_SET_VERTICAL_GRID(Body)
	DEFINE_GET_SET_VERTICAL_GRID(Header)
	DEFINE_GET_SET_VERTICAL_GRID(Footer)

	void GridView::setVerticalGrid(sl_uint32 iCol, sl_bool flag, UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		Ref<Column> col = m_columns.getValueAt_NoLock(iCol);
		if (col.isNotNull()) {
			col->m_flagBodyVerticalGrid = flag;
			col->m_flagHeaderVerticalGrid = flag;
			col->m_flagFooterVerticalGrid = flag;
			invalidate(mode);
		}
	}

	void GridViewColumn::setVerticalGrid(sl_bool flag, UIUpdateMode mode)
	{
		m_flagBodyVerticalGrid = flag;
		m_flagHeaderVerticalGrid = flag;
		m_flagFooterVerticalGrid = flag;
		_invalidate(mode);
	}

	void GridView::setVerticalGrid(sl_bool flag, UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		ListElements< Ref<Column> > columns(m_columns);
		for (sl_size iCol = 0; iCol < columns.count; iCol++) {
			Column* col = columns[iCol].get();
			col->m_flagBodyVerticalGrid = flag;
			col->m_flagHeaderVerticalGrid = flag;
			col->m_flagFooterVerticalGrid = flag;
		}
		m_defaultBodyVerticalGrid = flag;
		m_defaultHeaderVerticalGrid = flag;
		m_defaultFooterVerticalGrid = flag;
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
		if (m_selection.record >= (RecordIndex)count) {
			m_selection.record = OUTSIDE;
		}
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

#define DEFINE_SET_ROW_COUNT(SECTION, SECTION_VALUE) \
	sl_bool GridView::set##SECTION##RowCount(sl_uint32 count, UIUpdateMode mode) \
	{ \
		ObjectLocker lock(this); \
		sl_uint32 nOld = (sl_uint32)(m_list##SECTION##Row.getCount()); \
		if (nOld == count) { \
			return sl_true; \
		} \
		ListElements< Ref<Column> > columns(m_columns); \
		for (sl_size iCol = 0; iCol < columns.count; iCol++) { \
			Column* col = columns[iCol].get(); \
			nOld = (sl_uint32)(col->m_list##SECTION##Cell.getCount()); \
			if (!(col->m_list##SECTION##Cell.setCount_NoLock(count))) { \
				return sl_false; \
			} \
			if (nOld < count) { \
				SECTION##CellProp* props = col->m_list##SECTION##Cell.getData(); \
				for (sl_uint32 i = nOld; i < count; i++) { \
					props[i].inheritFrom(col->m_default##SECTION##Props); \
				} \
			} \
		} \
		if (nOld < count) { \
			if (!(m_list##SECTION##Row.setCount_NoLock(count))) { \
				return sl_false; \
			} \
			Ref<Row>* rows = m_list##SECTION##Row.getData(); \
			for (sl_uint32 i = nOld; i < count; i++) { \
				Ref<Row> row = new Row(this); \
				if (row.isNull()) { \
					m_list##SECTION##Row.setCount_NoLock(nOld); \
					return sl_false; \
				} \
				row->m_section = SECTION_VALUE; \
				row->m_index = i; \
				row->m_height = m_default##SECTION##RowHeight; \
				row->m_flagHorizontalGrid = m_default##SECTION##HorizontalGrid; \
				row->m_defaultProps.inheritFrom(m_default##SECTION##Props); \
				rows[i] = Move(row); \
			} \
		} else { \
			Ref<Row>* rows = m_list##SECTION##Row.getData(); \
			for (sl_uint32 i = count; i < nOld; i++) { \
				rows[i]->m_index = -1; \
			} \
			if (!(m_list##SECTION##Row.setCount_NoLock(count))) { \
				return sl_false; \
			} \
		} \
		refreshContentHeight(mode); \
		return sl_true; \
	}

	DEFINE_SET_ROW_COUNT(Body, BODY)
	DEFINE_SET_ROW_COUNT(Header, HEADER)
	DEFINE_SET_ROW_COUNT(Footer, FOOTER)

#define DEFINE_GET_ROW(SECTION) \
	Ref<GridViewRow> GridView::get##SECTION##Row(sl_uint32 index) \
	{ \
		ObjectLocker lock(this); \
		return m_list##SECTION##Row.getValueAt_NoLock(index); \
	}

	DEFINE_GET_ROW(Body)
	DEFINE_GET_ROW(Header)
	DEFINE_GET_ROW(Footer)

#define DEFINE_ADD_ROW(SECTION, SECTION_VALUE) \
	Ref<GridViewRow> GridView::add##SECTION##Row(UIUpdateMode mode) \
	{ \
		ObjectLocker lock(this); \
		sl_uint32 nRows = (sl_uint32)(m_list##SECTION##Row.getCount()); \
		Ref<Row> row = new Row(this); \
		if (row.isNull()) { \
			return sl_null; \
		} \
		ListElements< Ref<Column> > columns(m_columns); \
		for (sl_size iCol = 0; iCol < columns.count; iCol++) { \
			Column* col = columns[iCol].get(); \
			sl_uint32 n = (sl_uint32)(col->m_list##SECTION##Cell.getCount()); \
			if (!(col->m_list##SECTION##Cell.add_NoLock())) { \
				return sl_null; \
			} \
			SECTION##CellProp* prop = col->m_list##SECTION##Cell.getPointerAt(n); \
			if (!prop) { \
				return sl_null; \
			} \
			prop->inheritFrom(col->m_default##SECTION##Props); \
		} \
		row->m_section = SECTION_VALUE; \
		row->m_index = nRows; \
		row->m_defaultProps.inheritFrom(m_default##SECTION##Props); \
		if (!(m_list##SECTION##Row.add_NoLock(row))) { \
			return sl_null; \
		} \
		refreshContentHeight(mode); \
		return row; \
	}

	DEFINE_ADD_ROW(Body, BODY)
	DEFINE_ADD_ROW(Header, HEADER)
	DEFINE_ADD_ROW(Footer, FOOTER)

#define DEFINE_INSERT_ROW(SECTION, SECTION_VALUE) \
	Ref<GridViewRow> GridView::insert##SECTION##Row(sl_uint32 index, UIUpdateMode mode) \
	{ \
		ObjectLocker lock(this); \
		sl_uint32 nRows = (sl_uint32)(m_list##SECTION##Row.getCount()); \
		if (index > nRows) { \
			index = nRows; \
		} \
		Ref<Row> row = new Row(this); \
		if (row.isNull()) { \
			return sl_null; \
		} \
		ListElements< Ref<Column> > columns(m_columns); \
		for (sl_size iCol = 0; iCol < columns.count; iCol++) { \
			Column* col = columns[iCol].get(); \
			if (!(col->m_list##SECTION##Cell.insert_NoLock(index))) { \
				return sl_null; \
			} \
			SECTION##CellProp* prop = col->m_list##SECTION##Cell.getPointerAt(index); \
			if (!prop) { \
				return sl_null; \
			} \
			prop->inheritFrom(col->m_default##SECTION##Props); \
		} \
		row->m_section = SECTION_VALUE; \
		row->m_index = index; \
		row->m_defaultProps.inheritFrom(m_default##SECTION##Props); \
		if (!(m_list##SECTION##Row.insert_NoLock(index, row))) { \
			return sl_null; \
		} \
		Ref<Row>* rows = m_list##SECTION##Row.getData(); \
		for (sl_uint32 iRow = index + 1; iRow <= nRows; iRow++) { \
			rows[iRow]->m_index = iRow; \
		} \
		m_flagInvalidate##SECTION##Layout = sl_true; \
		refreshContentHeight(mode); \
		return row; \
	}

	DEFINE_INSERT_ROW(Body, BODY)
	DEFINE_INSERT_ROW(Header, HEADER)
	DEFINE_INSERT_ROW(Footer, FOOTER)

#define DEFINE_REMOVE_ROW(SECTION) \
	sl_bool GridView::remove##SECTION##Row(sl_uint32 index, UIUpdateMode mode) \
	{ \
		ObjectLocker lock(this); \
		Ref<Row> row; \
		if (m_list##SECTION##Row.removeAt_NoLock(index, &row)) { \
			row->m_index = -1; \
			{ \
				ListElements< Ref<Row> > rows(m_list##SECTION##Row); \
				for (sl_size i = index; i < rows.count; i++) { \
					rows[i]->m_index = (sl_uint32)i; \
				} \
			} \
			{ \
				ListElements< Ref<Column> > columns(m_columns); \
				for (sl_size i = 0; i < columns.count; i++) { \
					columns[i]->m_list##SECTION##Cell.removeAt_NoLock(index); \
				} \
			} \
			m_flagInvalidate##SECTION##Layout = sl_true; \
			refreshContentHeight(mode); \
			return sl_true; \
		} \
		return sl_false; \
	}

	DEFINE_REMOVE_ROW(Body)
	DEFINE_REMOVE_ROW(Header)
	DEFINE_REMOVE_ROW(Footer)

	sl_bool GridViewRow::remove(UIUpdateMode mode)
	{
		if (m_index >= 0) {
			Ref<GridView> view = m_view;
			if (view.isNotNull()) {
				if (m_section == GridView::HEADER) {
					return view->removeHeaderRow(m_index, mode);
				} else if (m_section == GridView::FOOTER) {
					return view->removeFooterRow(m_index, mode);
				} else {
					return view->removeBodyRow(m_index, mode);
				}
			}
		}
		return sl_false;
	}

#define DEFINE_GET_SET_ROW_ID(SECTION) \
	String GridView::get##SECTION##RowId(sl_uint32 index) \
	{ \
		ObjectLocker lock(this); \
		Ref<Row> row = m_list##SECTION##Row.getValueAt_NoLock(index); \
		if (row.isNotNull()) { \
			return row->getId(); \
		} \
		return sl_null; \
	} \
	void GridView::set##SECTION##RowId(sl_uint32 index, const String& _id) \
	{ \
		ObjectLocker lock(this); \
		Ref<Row> row = m_list##SECTION##Row.getValueAt_NoLock(index); \
		if (row.isNotNull()) { \
			row->setId(_id); \
		} \
	}

	DEFINE_GET_SET_ROW_ID(Body)
	DEFINE_GET_SET_ROW_ID(Header)
	DEFINE_GET_SET_ROW_ID(Footer)

	String GridViewRow::getId()
	{
		return m_id;
	}

	void GridViewRow::setId(const String& _id)
	{
		m_id = _id;
	}

	namespace
	{
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
		ListElements< Ref<Row> > rows(m_list##SECTION##Row); \
		sl_ui_len defaultRowHeight = _getDefaultRowHeight(); \
		for (sl_size i = 0; i < rows.count; i++) { \
			Row* row = rows[i].get(); \
			if (row->m_flagVisible) { \
				height += FixLength(row->m_height, defaultRowHeight); \
			} \
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
		Ref<Row> row = m_list##SECTION##Row.getValueAt_NoLock(index); \
		if (row.isNotNull()) { \
			return FixLength(row->m_height, _getDefaultRowHeight()); \
		} else { \
			return 0; \
		} \
	} \
	void GridView::set##SECTION##RowHeight(sl_uint32 index, sl_ui_len height, UIUpdateMode mode) \
	{ \
		if (height < 0) { \
			height = 0; \
		} \
		ObjectLocker lock(this); \
		Ref<Row> row = m_list##SECTION##Row.getValueAt_NoLock(index); \
		if (row.isNotNull()) { \
			row->m_height = height; \
			m_flagInvalidate##SECTION##Layout = sl_true; \
			refreshContentHeight(mode); \
		} \
	}

	DEFINE_GET_SET_ROW_HEIGHT(Body)
	DEFINE_GET_SET_ROW_HEIGHT(Header)
	DEFINE_GET_SET_ROW_HEIGHT(Footer)

	sl_ui_len GridViewRow::getHeight()
	{
		if (m_height >= 0) {
			return m_height;
		}
		if (m_index >= 0) {
			Ref<GridView> view = m_view;
			if (view.isNotNull()) {
				return view->_getDefaultRowHeight();
			}
		}
		return 0;
	}

	void GridViewRow::setHeight(sl_ui_len height, UIUpdateMode mode)
	{
		if (height < 0) {
			height = 0;
		}
		m_height = height;
		_invalidateLayout(mode);
	}

#define DEFINE_SET_ALL_ROW_HEIGHT(SECTION) \
	void GridView::set##SECTION##RowHeight(sl_ui_len height, UIUpdateMode mode) \
	{ \
		if (height < 0) { \
			height = 0; \
		} \
		ObjectLocker lock(this); \
		ListElements< Ref<Row> > rows(m_list##SECTION##Row); \
		for (sl_size i = 0; i < rows.count; i++) { \
			rows[i]->m_height = height; \
		} \
		m_default##SECTION##RowHeight = height; \
		m_flagInvalidate##SECTION##Layout = sl_true; \
		refreshContentHeight(mode); \
	}

	DEFINE_SET_ALL_ROW_HEIGHT(Body)
	DEFINE_SET_ALL_ROW_HEIGHT(Header)
	DEFINE_SET_ALL_ROW_HEIGHT(Footer)

	void GridView::setRowHeight(sl_ui_len height, UIUpdateMode mode)
	{
		if (height < 0) {
			height = 0;
		}
		ObjectLocker lock(this);
		{
			m_defaultBodyRowHeight = height;
			ListElements< Ref<Row> > rows(m_listBodyRow);
			for (sl_size i = 0; i < rows.count; i++) {
				rows[i]->m_height = height;
			}
		}
		{
			m_defaultHeaderRowHeight = height;
			ListElements< Ref<Row> > rows(m_listHeaderRow);
			for (sl_size i = 0; i < rows.count; i++) {
				rows[i]->m_height = height;
			}
		}
		{
			m_defaultFooterRowHeight = height;
			ListElements< Ref<Row> > rows(m_listFooterRow);
			for (sl_size i = 0; i < rows.count; i++) {
				rows[i]->m_height = height;
			}
		}
		_invalidateLayout();
		refreshContentHeight(mode);
	}
	
#define DEFINE_GET_SET_ROW_VISIBLE(SECTION) \
	sl_bool GridView::is##SECTION##RowVisible(sl_uint32 index) \
	{ \
		ObjectLocker lock(this); \
		Ref<Row> row = m_list##SECTION##Row.getValueAt_NoLock(index); \
		if (row.isNotNull()) { \
			return row->m_flagVisible; \
		} else { \
			return sl_false; \
		} \
	} \
	void GridView::set##SECTION##RowVisible(sl_uint32 index, sl_bool flag, UIUpdateMode mode) \
	{ \
		ObjectLocker lock(this); \
		Ref<Row> row = m_list##SECTION##Row.getValueAt_NoLock(index); \
		if (row.isNotNull()) { \
			if (row->m_flagVisible != flag) { \
				row->m_flagVisible = flag; \
				m_flagInvalidate##SECTION##Layout = sl_true; \
				refreshContentHeight(mode); \
			} \
		} \
	}

	DEFINE_GET_SET_ROW_VISIBLE(Body)
	DEFINE_GET_SET_ROW_VISIBLE(Header)
	DEFINE_GET_SET_ROW_VISIBLE(Footer)

	sl_bool GridViewRow::isVisible()
	{
		return m_flagVisible;
	}

	void GridViewRow::setVisible(sl_bool flag, UIUpdateMode mode)
	{
		if (m_flagVisible != flag) {
			m_flagVisible = flag;
			_invalidateLayout(mode);
		}
	}
	
#define DEFINE_GET_SET_HORIZONTAL_GRID(SECTION) \
	sl_bool GridView::is##SECTION##HorizontalGrid(sl_uint32 iRow) \
	{ \
		ObjectLocker lock(this); \
		Ref<Row> row = m_list##SECTION##Row.getValueAt_NoLock(iRow); \
		if (row.isNotNull()) { \
			return row->m_flagHorizontalGrid; \
		} else { \
			return sl_false; \
		} \
	} \
	void GridView::set##SECTION##HorizontalGrid(sl_uint32 iRow, sl_bool flag, UIUpdateMode mode) \
	{ \
		ObjectLocker lock(this); \
		Ref<Row> row = m_list##SECTION##Row.getValueAt_NoLock(iRow); \
		if (row.isNotNull()) { \
			row->m_flagHorizontalGrid = flag; \
			invalidate(mode); \
		} \
	} \
	void GridView::set##SECTION##HorizontalGrid(sl_bool flag, UIUpdateMode mode) \
	{ \
		ObjectLocker lock(this); \
		ListElements< Ref<Row> > rows(m_list##SECTION##Row); \
		for (sl_size i = 0; i < rows.count; i++) { \
			rows[i]->m_flagHorizontalGrid = flag; \
		} \
		m_default##SECTION##HorizontalGrid = flag; \
		invalidate(mode); \
	}

	DEFINE_GET_SET_HORIZONTAL_GRID(Body)
	DEFINE_GET_SET_HORIZONTAL_GRID(Header)
	DEFINE_GET_SET_HORIZONTAL_GRID(Footer)

	void GridView::setHorizontalGrid(sl_bool flag, UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		{
			m_defaultBodyHorizontalGrid = flag;
			ListElements< Ref<Row> > rows(m_listBodyRow);
			for (sl_size i = 0; i < rows.count; i++) {
				rows[i]->m_flagHorizontalGrid = flag;
			}
		}
		{
			m_defaultHeaderHorizontalGrid = flag;
			ListElements< Ref<Row> > rows(m_listHeaderRow);
			for (sl_size i = 0; i < rows.count; i++) {
				rows[i]->m_flagHorizontalGrid = flag;
			}
		}
		{
			m_defaultFooterHorizontalGrid = flag;
			ListElements< Ref<Row> > rows(m_listFooterRow);
			for (sl_size i = 0; i < rows.count; i++) {
				rows[i]->m_flagHorizontalGrid = flag;
			}
		}
		invalidate(mode);
	}

	sl_bool GridViewRow::isHorizontalGrid()
	{
		return m_flagHorizontalGrid;
	}

	void GridViewRow::setHorizontalGrid(sl_bool flag, UIUpdateMode mode)
	{
		m_flagHorizontalGrid = flag;
		_invalidate(mode);
	}

#define DEFINE_GET_SET_GRID(SECTION, DEFAULT) \
	Ref<Pen> GridView::get##SECTION##Grid() \
	{ \
		if (m_flagSetGrid##SECTION) { \
			return m_grid##SECTION; \
		} \
		Ref<Pen> grid = m_grid##SECTION; \
		if (grid.isNotNull()) { \
			return grid; \
		} \
		return DEFAULT; \
	} \
	void GridView::set##SECTION##Grid(const Ref<Pen>& pen, UIUpdateMode mode) \
	{ \
		m_grid##SECTION = pen; \
		m_flagSetGrid##SECTION = sl_true; \
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
		m_flagSetGridBody = sl_true;
		m_flagSetGridHeader = sl_true;
		m_flagSetGridFooter = sl_true;
		m_flagSetGridLeft = sl_true;
		m_flagSetGridRight = sl_true;
		invalidate(mode);
	}

	void GridView::setGrid(const PenDesc& desc, UIUpdateMode mode)
	{
		m_gridBody = Pen::create(desc, getBodyGrid());
		m_gridHeader = Pen::create(desc, getHeaderGrid());
		m_gridFooter = Pen::create(desc, getFooterGrid());
		m_gridLeft = Pen::create(desc, getLeftGrid());
		m_gridRight = Pen::create(desc, getRightGrid());
		m_flagSetGridBody = sl_true;
		m_flagSetGridHeader = sl_true;
		m_flagSetGridFooter = sl_true;
		m_flagSetGridLeft = sl_true;
		m_flagSetGridRight = sl_true;
		invalidate(mode);
	}

	Ref<Pen> GridView::getSelectionBorder()
	{
		return m_selectionBorder;
	}

	void GridView::setSelectionBorder(const Ref<Pen>& pen, UIUpdateMode mode)
	{
		m_selectionBorder = pen;
		invalidate(mode);
	}

	void GridView::setSelectionBorder(const PenDesc& desc, UIUpdateMode mode)
	{
		m_selectionBorder = Pen::create(desc, m_selectionBorder);
		invalidate(mode);
	}

	sl_bool GridView::isSorting()
	{
		return m_flagSorting;
	}

	void GridView::setSorting(sl_bool flag)
	{
		m_flagSorting = flag;
		m_flagDefinedSorting = sl_true;
	}

	Ref<Drawable> GridView::getAscendingIcon()
	{
		return m_iconAsc;
	}

	void GridView::setAscendingIcon(const Ref<Drawable>& icon, UIUpdateMode mode)
	{
		Color c;
		if (ColorDrawable::check(icon, &c)) {
			m_iconAsc = new DefaultSortIcon(c, sl_true);
		} else {
			m_iconAsc = icon;
		}
		invalidate(mode);
	}

	Ref<Drawable> GridView::getDescendingIcon()
	{
		return m_iconDesc;
	}

	void GridView::setDescendingIcon(const Ref<Drawable>& icon, UIUpdateMode mode)
	{
		Color c;
		if (ColorDrawable::check(icon, &c)) {
			m_iconDesc = new DefaultSortIcon(c, sl_true);
		} else {
			m_iconDesc = icon;
		}
		invalidate(mode);
	}

	sl_ui_len GridView::getSortIconSize()
	{
		sl_ui_len size = m_sortIconSize;
		if (size < 0) {
			Ref<Font> font = getFont();
			if (font.isNotNull()) {
				size = (sl_ui_len)(font->getFontHeight());
			} else {
				size = 0;
			}
		}
		return size;
	}

	void GridView::setSortIconSize(sl_ui_len size, UIUpdateMode mode)
	{
		m_sortIconSize = size;
		invalidate(mode);
	}

	void GridView::refreshContentWidth(UIUpdateMode mode)
	{
		if (SLIB_UI_UPDATE_MODE_IS_INIT(mode)) {
			m_flagInitialize = sl_true;
			return;
		}
		ObjectLocker lock(this);
		ListElements< Ref<Column> > columns(m_columns);
		sl_ui_len width = getWidth();
		sl_uint32 nColumns = (sl_uint32)(columns.count);
		sl_uint32 nLeft = m_nLeftColumns;
		sl_uint32 nRight = m_nRightColumns;
		FixLeftRightColumnCount(nColumns, nLeft, nRight);

		sl_uint32 iRight = nColumns - nRight;
		sl_ui_len xContent = 0;
		sl_ui_len xFixed = 0;
		sl_uint32 nDefault = 0;
		{
			for (sl_uint32 i = 0; i < nColumns; i++) {
				Column* col = columns[i].get();
				if (col->m_flagVisible) {
					sl_ui_len w = col->m_width;
					if (i < m_nLeftColumns || i >= iRight) {
						xFixed += w;
					} else {
						if (col->m_flagDefaultWidth) {
							nDefault++;
						} else {
							xContent += w;
						}
					}
				}
			}
		}
		sl_ui_len widthMid = width - xFixed;
		// Fix default width of mid columns
		if (nDefault) {
			if (xContent < widthMid) {
				sl_ui_len w = (widthMid - xContent) / nDefault;
				for (sl_uint32 i = nLeft; i < iRight; i++) {
					Column* col = columns[i].get();
					if (col->m_flagVisible && col->m_flagDefaultWidth) {
						sl_ui_len oldWidth = col->m_width;
						if (i == iRight - 1 && xContent < widthMid) {
							col->setDefaultWidth(widthMid - xContent, UIUpdateMode::Init);
						} else {
							col->setDefaultWidth(w, UIUpdateMode::Init);
						}
						xContent += col->m_width;;
						if (oldWidth != col->m_width) {
							_invalidateLayout();
						}
					}
				}
			} else {
				for (sl_uint32 i = nLeft; i < iRight; i++) {
					Column* col = columns[i].get();
					if (col->m_flagVisible && col->m_flagDefaultWidth) {
						xContent += col->m_width;
					}
				}
			}
		}
		if (xFixed < width) {
			setPageWidth((sl_scroll_pos)(width - xFixed), SLIB_UI_UPDATE_MODE_IS_INIT(mode) ? UIUpdateMode::Init : UIUpdateMode::None);
			setContentWidth((sl_scroll_pos)xContent, UIUpdateMode::None);
		} else {
			setPageWidth(0, SLIB_UI_UPDATE_MODE_IS_INIT(mode) ? UIUpdateMode::Init : UIUpdateMode::None);
			setContentWidth(0, UIUpdateMode::None);
		}
		invalidate(mode);
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
			setContentHeight((sl_scroll_pos)content, UIUpdateMode::None);
		} else {
			setPageHeight(0, SLIB_UI_UPDATE_MODE_IS_INIT(mode) ? UIUpdateMode::Init : UIUpdateMode::None);
			setContentHeight(0, UIUpdateMode::None);
		}
		invalidate(mode);
	}

	GridView::DataGetter GridView::getDataGetter()
	{
		return m_recordData;
	}

	void GridView::setDataGetter(const DataGetter& func, UIUpdateMode mode)
	{
		m_recordData = func;
		if (SLIB_UI_UPDATE_MODE_IS_INIT(mode)) {
			return;
		}
		ObjectLocker lock(this);
		_invalidateBodyAllCells();
		invalidate(mode);
	}

	namespace {
		class CompareRecord
		{
		public:
			CompareRecord(const String& field, sl_bool flagAsc): m_field(field), m_flagAsc(flagAsc)
			{
			}

		public:
			sl_compare_result operator()(const Variant& v1, const Variant& v2) const
			{
				sl_compare_result result = v1.getItemByPath(m_field).compare(v2.getItemByPath(m_field));
				if (!m_flagAsc) {
					result = -result;
				}
				return result;
			}

		public:
			String m_field;
			sl_bool m_flagAsc;
		};
	}

	void GridView::_setData(const VariantList& list)
	{
		setDataGetter([list](sl_uint64 record) {
			return list.getValueAt((sl_size)record);
		}, UIUpdateMode::None);
		setRecordCount(list.getCount(), UIUpdateMode::None);
	}

	void GridView::_setData(const List<VariantMap>& list)
	{
		setDataGetter([list](sl_uint64 record) {
			return list.getValueAt((sl_size)record);
		}, UIUpdateMode::None);
		setRecordCount(list.getCount(), UIUpdateMode::None);
	}

	void GridView::_setData(const Variant& data)
	{
		setDataGetter([data](sl_uint64 record) {
			return data.getElement((sl_size)record);
		}, UIUpdateMode::None);
		setRecordCount(data.getElementCount(), UIUpdateMode::None);
	}

	namespace
	{
		static sl_bool IsMatched(const Variant& var, const String& filter);

		static sl_bool IsMapMatched(const VariantMap& map, const String& filter)
		{
			for (auto&& item : map) {
				if (IsMatched(item.value, filter)) {
					return sl_true;
				}
			}
			return sl_false;
		}

		static sl_bool IsListMatched(const VariantList& list, const String& filter)
		{
			for (auto&& item : list) {
				if (IsMatched(item, filter)) {
					return sl_true;
				}
			}
			return sl_false;
		}

		static sl_bool IsMatched(const Variant& var, const String& filter)
		{
			if (var.isVariantMap()) {
				return IsMapMatched(var.getVariantMap(), filter);
			} else if (var.isVariantList()) {
				return IsListMatched(var.getVariantList(), filter);
			} else {
				return var.getString().contains_IgnoreCase(filter);
			}
		}

		static sl_bool IsFilterMatched(const Variant& var, const VariantMap& filter)
		{
			for (auto&& item : filter) {
				if (!(IsMatched(var[item.key], item.value.getString()))) {
					return sl_false;
				}
			}
			return sl_true;
		}

		static sl_bool IsFilterMapMatched(const VariantMap& map, const VariantMap& filter)
		{
			for (auto&& item : filter) {
				if (!(IsMatched(map.getValue(item.key), item.value.getString()))) {
					return sl_false;
				}
			}
			return sl_true;
		}

		static VariantList DuplicateData(const VariantList& list)
		{
			return list.duplicate();
		}

		static VariantList FilterData(const VariantList& list, const Variant& filter)
		{
			if (filter.isNull()) {
				return sl_null;
			}
			if (filter.isStringType()) {
				String strFilter = filter.getString().trim();
				if (strFilter.isEmpty()) {
					return sl_null;
				}
				VariantList ret = VariantList::create();
				for (auto& item : list) {
					if (IsMatched(item, strFilter)) {
						ret.add_NoLock(item);
					}
				}
				return ret;
			} else if (filter.isVariantMap()) {
				VariantMap mapFilter = filter.getVariantMap();
				if (mapFilter.isEmpty()) {
					return sl_null;
				}
				VariantList ret = VariantList::create();
				for (auto& item : list) {
					if (IsFilterMatched(item, mapFilter)) {
						ret.add_NoLock(item);
					}
				}
				return ret;
			} else {
				return sl_null;
			}
		}

		static VariantList DuplicateData(const List<VariantMap>& list)
		{
			VariantList ret;
			ListLocker<VariantMap> items(list);
			for (sl_size i = 0; i < items.count; i++) {
				ret.add_NoLock(items[i]);
			}
			return ret;
		}

		static VariantList FilterData(const List<VariantMap>& list, const Variant& filter)
		{
			if (filter.isNull()) {
				return sl_null;
			}
			if (filter.isStringType()) {
				String strFilter = filter.getString().trim();
				if (strFilter.isEmpty()) {
					return sl_null;
				}
				VariantList ret = VariantList::create();
				for (auto& item : list) {
					if (IsMapMatched(item, strFilter)) {
						ret.add_NoLock(item);
					}
				}
				return ret;
			} else if (filter.isVariantMap()) {
				VariantMap mapFilter = filter.getVariantMap();
				if (mapFilter.isEmpty()) {
					return sl_null;
				}
				VariantList ret = VariantList::create();
				for (auto& item : list) {
					if (IsFilterMapMatched(item, mapFilter)) {
						ret.add_NoLock(item);
					}
				}
				return ret;
			} else {
				return sl_null;
			}
		}

		static VariantList DuplicateData(const Variant& data)
		{
			if (data.isVariantList()) {
				return data.getVariantList().duplicate();
			}
			VariantList ret;
			Ref<Collection> collection = data.getCollection();
			if (collection.isNotNull()) {
				sl_size n = (sl_size)(collection->getElementCount());
				for (sl_size i = 0; i < n; i++) {
					ret.add_NoLock(collection->getElement(i));
				}
			}
			return ret;
		}

		static VariantList FilterData(const Variant& data, const Variant& filter)
		{
			if (filter.isNull()) {
				return sl_null;
			}
			if (data.isVariantList()) {
				return FilterData(data.getVariantList(), filter);
			}
			if (filter.isStringType()) {
				String strFilter = filter.getString().trim();
				if (strFilter.isEmpty()) {
					return sl_null;
				}
				VariantList ret = VariantList::create();
				Ref<Collection> collection = data.getCollection();
				if (collection.isNotNull()) {
					sl_size n = (sl_size)(collection->getElementCount());
					for (sl_size i = 0; i < n; i++) {
						Variant item = collection->getElement(i);
						if (IsMatched(item, strFilter)) {
							ret.add_NoLock(item);
						}
					}
				}
				return ret;
			} else if (filter.isVariantMap()) {
				VariantMap mapFilter = filter.getVariantMap();
				if (mapFilter.isEmpty()) {
					return sl_null;
				}
				VariantList ret = VariantList::create();
				Ref<Collection> collection = data.getCollection();
				if (collection.isNotNull()) {
					sl_size n = (sl_size)(collection->getElementCount());
					for (sl_size i = 0; i < n; i++) {
						Variant item = collection->getElement(i);
						if (IsFilterMatched(item, mapFilter)) {
							ret.add_NoLock(item);
						}
					}
				}
				return ret;
			} else {
				return sl_null;
			}
		}
	}

#define DEFINE_SET_DATA(DATA_TYPE) \
	void GridView::setData(const DATA_TYPE& data, UIUpdateMode mode) \
	{ \
		if (data.isNull()) { \
			clearData(mode); \
			return; \
		} \
		ObjectLocker lock(this); \
		if (!m_flagDefinedSorting) { \
			setSorting(sl_true); \
		} \
		m_filteredData.setNull(); \
		m_onChangeDataFilter = [this, data](const Variant& filter, UIUpdateMode mode) { \
			m_filteredData = FilterData(data, filter); \
			if (m_cellSort) { \
				m_onSort(m_cellSort->field, m_flagSortAsc, mode); \
			} else { \
				m_onSort(sl_null, sl_false, mode); \
			} \
		}; \
		m_sortedData.setNull(); \
		m_onSort = [this, data](const String& field, sl_bool flagAsc, UIUpdateMode mode) { \
			if (field.isNotEmpty()) { \
				VariantList cache = m_sortedData; \
				if (cache.isNull()) { \
					if (m_filteredData.isNotNull()) { \
						cache = DuplicateData(VariantList(m_filteredData)); \
					} else { \
						cache = DuplicateData(data); \
					} \
					m_sortedData = cache; \
				} \
				cache.sort_NoLock(CompareRecord(field, flagAsc)); \
				_setData(cache); \
			} else { \
				if (m_filteredData.isNotNull()) { \
					_setData(VariantList(m_filteredData)); \
				} else { \
					_setData(data); \
				} \
			} \
			invalidate(mode); \
		}; \
		m_onChangeDataFilter(Variant(m_dataFilter), mode); \
	}

	DEFINE_SET_DATA(VariantList)
	DEFINE_SET_DATA(List<VariantMap>)
	DEFINE_SET_DATA(Variant)

	void GridView::setData(const JsonList& data, UIUpdateMode mode)
	{
		setData(*(reinterpret_cast<const VariantList*>(&data)), mode);
	}

	void GridView::clearData(UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		setDataGetter(sl_null, UIUpdateMode::None);
		setRecordCount(0, UIUpdateMode::None);
		m_filteredData.setNull();
		m_onChangeDataFilter.setNull();
		m_sortedData.setNull();
		m_onSort.setNull();
		invalidate(mode);
	}

	void GridView::setModel(const Ref<TableModel>& source, UIUpdateMode mode)
	{
		if (source.isNull()) {
			clearData(mode);
			return;
		}
		ObjectLocker lock(this);
		if (source->isSortable()) {
			if (!m_flagDefinedSorting) {
				setSorting(sl_true);
			}
		}
		sl_ui_len recordHeight = getRecordHeight();
		if (recordHeight > 0) {
			source->setCacheItemCount(UI::getScreenHeight() * 12 / recordHeight / 10 + 1);
		}
		setDataGetter([source](sl_uint64 record) {
			return source->getRecord(record);
		}, UIUpdateMode::None);
		setRecordCount(source->getRecordCount(), UIUpdateMode::None);
		m_filteredData.setNull();
		m_onChangeDataFilter = [this, source](const Variant& filter, UIUpdateMode mode) {
			source->filter(filter);
			ObjectLocker lock(this);
			_invalidateBodyAllCells();
			invalidate(mode);
		};
		m_sortedData.setNull();
		m_onSort = [this, source](const String& field, sl_bool flagAsc, UIUpdateMode mode) {
			source->sort(field, flagAsc);
			ObjectLocker lock(this);
			_invalidateBodyAllCells();
			invalidate(mode);
		};
		Variant filter = m_dataFilter;
		if (filter.isNotNull()) {
			source->filter(filter);
			if (m_cellSort) {
				source->sort(m_cellSort->field, m_flagSortAsc);
			}
			_invalidateBodyAllCells();
		} else {
			if (m_cellSort) {
				source->sort(m_cellSort->field, m_flagSortAsc);
				_invalidateBodyAllCells();
			}
		}
		invalidate(mode);
	}

	void GridView::setDataFilter(const Variant& filter, UIUpdateMode mode)
	{
		m_dataFilter = filter;
		m_onChangeDataFilter(filter, mode);
	}

	GridView::CellProp* GridView::_getCellProp(RecordIndex section, sl_uint32 iRow, sl_uint32 iCol)
	{
		Ref<Column> col = m_columns.getValueAt_NoLock(iCol);
		if (col.isNotNull()) {
			if (section == HEADER) {
				return col->m_listHeaderCell.getPointerAt(iRow);
			} else if (section == FOOTER) {
				return col->m_listFooterCell.getPointerAt(iRow);
			} else {
				return col->m_listBodyCell.getPointerAt(iRow);
			}
		}
		return sl_null;
	}

#define DEFINE_GET_CELL_PROP(SECTION) \
	GridView::SECTION##CellProp* GridView::_get##SECTION##CellProp(sl_uint32 iRow, sl_uint32 iCol) \
	{ \
		Ref<Column> col = m_columns.getValueAt_NoLock(iCol); \
		if (col.isNotNull()) { \
			return col->m_list##SECTION##Cell.getPointerAt(iRow); \
		} \
		return sl_null; \
	}

	DEFINE_GET_CELL_PROP(Body)
	DEFINE_GET_CELL_PROP(Header)
	DEFINE_GET_CELL_PROP(Footer)

#define DEFINE_SET_SECTION_DEFAULT_ATTR_SUB(SECTION, ...) \
	{ \
		m_default##SECTION##Props.__VA_ARGS__; \
		ListElements< Ref<Row> > rows(m_list##SECTION##Row); \
		for (sl_size k = 0; k < rows.count; k++) { \
			rows[k]->m_defaultProps.__VA_ARGS__; \
		} \
	}

#define DEFINE_SET_SECTION_COLUMN_ATTR_SUB(SECTION, col, ...) \
	{ \
		(col)->m_default##SECTION##Props.__VA_ARGS__; \
		ListElements<SECTION##CellProp> props((col)->m_list##SECTION##Cell); \
		for (sl_size k = 0; k < props.count; k++) { \
			props[k].__VA_ARGS__; \
		} \
	}

#define DEFINE_SET_SECTION_ROW_ATTR_SUB(SECTION, row, ...) \
	{ \
		(row)->m_defaultProps.__VA_ARGS__; \
		ListElements< Ref<Column> > columns(m_columns); \
		for (sl_size i = 0; i < columns.count; i++) { \
			Column* col = columns[i].get(); \
			SECTION##CellProp* prop = col->m_list##SECTION##Cell.getPointerAt(iRow); \
			if (prop) { \
				prop->__VA_ARGS__; \
			} \
		} \
	}

#define DEFINE_SET_SECTION_ALL_ATTR_SUB(SECTION, ...) \
	{ \
		ListElements< Ref<Column> > columns(m_columns); \
		for (sl_size i = 0; i < columns.count; i++) { \
			Column* col = columns[i].get(); \
			DEFINE_SET_SECTION_COLUMN_ATTR_SUB(SECTION, col, __VA_ARGS__) \
		} \
	} \
	DEFINE_SET_SECTION_DEFAULT_ATTR_SUB(SECTION, __VA_ARGS__) \

#define DEFINE_SET_COLUMN_ATTR_SUB(col, ...) \
	DEFINE_SET_SECTION_COLUMN_ATTR_SUB(Body, col, __VA_ARGS__) \
	DEFINE_SET_SECTION_COLUMN_ATTR_SUB(Header, col, __VA_ARGS__) \
	DEFINE_SET_SECTION_COLUMN_ATTR_SUB(Footer, col, __VA_ARGS__)

#define DEFINE_SET_ALL_ATTR_SUB(...) \
	{ \
		ListElements< Ref<Column> > columns(m_columns); \
		for (sl_size iCol = 0; iCol < columns.count; iCol++) { \
			Column* col = columns[iCol].get(); \
			DEFINE_SET_COLUMN_ATTR_SUB(col, __VA_ARGS__) \
		} \
	} \
	DEFINE_SET_SECTION_DEFAULT_ATTR_SUB(Body, __VA_ARGS__) \
	DEFINE_SET_SECTION_DEFAULT_ATTR_SUB(Header, __VA_ARGS__) \
	DEFINE_SET_SECTION_DEFAULT_ATTR_SUB(Footer, __VA_ARGS__)


#define DEFINE_SET_CELL_GENERIC_ATTR_SUB(SECTION, FUNC, ARG, NAME) \
	void GridView::set##SECTION##FUNC(sl_int32 iRow, sl_int32 iCol, ARG value) \
	{ \
		ObjectLocker lock(this); \
		if (iCol >= 0) { \
			if (iRow >= 0) { \
				SECTION##CellProp* prop = _get##SECTION##CellProp(iRow, iCol); \
				if (prop) { \
					prop->NAME = value; \
				} \
			} else { \
				Ref<Column> col = m_columns.getValueAt_NoLock(iCol); \
				if (col.isNotNull()) { \
					Column* pCol = col.get(); \
					DEFINE_SET_SECTION_COLUMN_ATTR_SUB(SECTION, pCol, NAME = value) \
				} \
			} \
		} else { \
			if (iRow >= 0) { \
				Ref<Row> row = m_list##SECTION##Row.getValueAt_NoLock(iRow); \
				if (row.isNotNull()) { \
					Row* pRow = row.get(); \
					DEFINE_SET_SECTION_ROW_ATTR_SUB(SECTION, pRow, NAME = value) \
				} \
			} else { \
				DEFINE_SET_SECTION_ALL_ATTR_SUB(SECTION, NAME = value) \
			} \
		} \
	}

#define DEFINE_GET_SET_CELL_GENERIC_ATTR_SUB(SECTION, FUNC, RET, ARG, NAME, DEF) \
	RET GridView::get##SECTION##FUNC(sl_uint32 iRow, sl_uint32 iCol) \
	{ \
		ObjectLocker lock(this); \
		SECTION##CellProp* prop = _get##SECTION##CellProp(iRow, iCol); \
		if (prop) { \
			return prop->NAME; \
		} \
		return DEF; \
	} \
	DEFINE_SET_CELL_GENERIC_ATTR_SUB(SECTION, FUNC, ARG, NAME)

#define DEFINE_SET_COLUMN_GENERIC_ATTR(FUNC, ARG, NAME) \
	void GridView::setColumn##FUNC(sl_int32 iCol, ARG value) \
	{ \
		ObjectLocker lock(this); \
		if (iCol >= 0) { \
			Ref<Column> col = m_columns.getValueAt_NoLock(iCol); \
			if (col.isNotNull()) { \
				Column* pCol = col.get(); \
				DEFINE_SET_COLUMN_ATTR_SUB(pCol, NAME = value) \
			} \
		} else { \
			DEFINE_SET_ALL_ATTR_SUB(NAME = value) \
		} \
	} \
	void GridView::setCell##FUNC(ARG value) \
	{ \
		setColumn##FUNC(-1, value); \
	}

#define DEFINE_SET_CELL_GENERIC_ATTR(FUNC, ARG, NAME) \
	DEFINE_SET_CELL_GENERIC_ATTR_SUB(Body, FUNC, ARG, NAME) \
	DEFINE_SET_CELL_GENERIC_ATTR_SUB(Header, FUNC, ARG, NAME) \
	DEFINE_SET_CELL_GENERIC_ATTR_SUB(Footer, FUNC, ARG, NAME) \
	DEFINE_SET_COLUMN_GENERIC_ATTR(FUNC, ARG, NAME)

#define DEFINE_GET_SET_CELL_GENERIC_ATTR(FUNC, RET, ARG, NAME, DEF) \
	DEFINE_GET_SET_CELL_GENERIC_ATTR_SUB(Body, FUNC, RET, ARG, NAME, DEF) \
	DEFINE_GET_SET_CELL_GENERIC_ATTR_SUB(Header, FUNC, RET, ARG, NAME, DEF) \
	DEFINE_GET_SET_CELL_GENERIC_ATTR_SUB(Footer, FUNC, RET, ARG, NAME, DEF) \
	DEFINE_SET_COLUMN_GENERIC_ATTR(FUNC, ARG, NAME)


#define DEFINE_GET_SET_CELL_BOOL_ATTR_SUB(SECTION, FUNC, NAME, DEF) \
	sl_bool GridView::is##SECTION##FUNC(sl_uint32 iRow, sl_uint32 iCol) \
	{ \
		ObjectLocker lock(this); \
		SECTION##CellProp* prop = _get##SECTION##CellProp(iRow, iCol); \
		if (prop) { \
			return prop->NAME; \
		} \
		return DEF; \
	} \
	DEFINE_SET_CELL_GENERIC_ATTR_SUB(SECTION, FUNC, sl_bool, NAME)

#define DEFINE_GET_SET_CELL_BOOL_ATTR(FUNC, NAME, DEF) \
	DEFINE_GET_SET_CELL_BOOL_ATTR_SUB(Body, FUNC, NAME, DEF) \
	DEFINE_GET_SET_CELL_BOOL_ATTR_SUB(Header, FUNC, NAME, DEF) \
	DEFINE_GET_SET_CELL_BOOL_ATTR_SUB(Footer, FUNC, NAME, DEF) \
	DEFINE_SET_COLUMN_GENERIC_ATTR(FUNC, sl_bool, NAME)


#define DEFINE_SET_CELL_LAYOUT_ATTR_SUB(SECTION, FUNC, ARG, NAME) \
	void GridView::set##SECTION##FUNC(sl_int32 iRow, sl_int32 iCol, ARG value, UIUpdateMode mode) \
	{ \
		ObjectLocker lock(this); \
		if (iCol >= 0) { \
			if (iRow >= 0) { \
				SECTION##CellProp* prop = _get##SECTION##CellProp(iRow, iCol); \
				if (prop) { \
					prop->NAME = value; \
					if (SLIB_UI_UPDATE_MODE_IS_INIT(mode)) { \
						return; \
					} \
					_invalidate##SECTION##Cell(*prop); \
					invalidate(mode); \
				} \
			} else { \
				Ref<Column> col = m_columns.getValueAt_NoLock(iCol); \
				if (col.isNotNull()) { \
					Column* pCol = col.get(); \
					DEFINE_SET_SECTION_COLUMN_ATTR_SUB(SECTION, pCol, NAME = value) \
					if (SLIB_UI_UPDATE_MODE_IS_INIT(mode)) { \
						return; \
					} \
					_invalidate##SECTION##ColumnCells(pCol); \
					invalidate(mode); \
				} \
			} \
		} else { \
			if (iRow >= 0) { \
				Ref<Row> row = m_list##SECTION##Row.getValueAt_NoLock(iRow); \
				if (row.isNotNull()) { \
					Row* pRow = row.get(); \
					DEFINE_SET_SECTION_ROW_ATTR_SUB(SECTION, pRow, NAME = value) \
					if (SLIB_UI_UPDATE_MODE_IS_INIT(mode)) { \
						return; \
					} \
					_invalidate##SECTION##RowCells(pRow); \
					invalidate(mode); \
				} \
			} else { \
				DEFINE_SET_SECTION_ALL_ATTR_SUB(SECTION, NAME = value) \
				if (SLIB_UI_UPDATE_MODE_IS_INIT(mode)) { \
					return; \
				} \
				_invalidate##SECTION##AllCells(); \
				invalidate(mode); \
			} \
		} \
	}

#define DEFINE_GET_SET_CELL_LAYOUT_ATTR_SUB(SECTION, FUNC, RET, ARG, NAME, DEF) \
	RET GridView::get##SECTION##FUNC(sl_uint32 iRow, sl_uint32 iCol) \
	{ \
		ObjectLocker lock(this); \
		SECTION##CellProp* prop = _get##SECTION##CellProp(iRow, iCol); \
		if (prop) { \
			return prop->NAME; \
		} \
		return DEF; \
	} \
	DEFINE_SET_CELL_LAYOUT_ATTR_SUB(SECTION, FUNC, ARG, NAME)

#define DEFINE_SET_COLUMN_LAYOUT_ATTR(FUNC, ARG, NAME) \
	void GridView::setColumn##FUNC(sl_int32 iCol, ARG value, UIUpdateMode mode) \
	{ \
		ObjectLocker lock(this); \
		if (iCol >= 0) { \
			Ref<Column> col = m_columns.getValueAt_NoLock(iCol); \
			if (col.isNotNull()) { \
				Column* pCol = col.get(); \
				DEFINE_SET_COLUMN_ATTR_SUB(pCol, NAME = value) \
				if (SLIB_UI_UPDATE_MODE_IS_INIT(mode)) { \
					return; \
				} \
				_invalidateBodyColumnCells(pCol); \
				_invalidateHeaderColumnCells(pCol); \
				_invalidateFooterColumnCells(pCol); \
				invalidate(mode); \
			} \
		} else { \
			DEFINE_SET_ALL_ATTR_SUB(NAME = value) \
			if (SLIB_UI_UPDATE_MODE_IS_INIT(mode)) { \
				return; \
			} \
			_invalidateAllCells(); \
			invalidate(mode); \
		} \
	} \
	void GridView::setCell##FUNC(ARG value, UIUpdateMode mode) \
	{ \
		setColumn##FUNC(-1, value, mode); \
	}

#define DEFINE_SET_CELL_LAYOUT_ATTR(FUNC, ARG, NAME) \
	DEFINE_SET_CELL_LAYOUT_ATTR_SUB(Body, FUNC, ARG, NAME) \
	DEFINE_SET_CELL_LAYOUT_ATTR_SUB(Header, FUNC, ARG, NAME) \
	DEFINE_SET_CELL_LAYOUT_ATTR_SUB(Footer, FUNC, ARG, NAME) \
	DEFINE_SET_COLUMN_LAYOUT_ATTR(FUNC, ARG, NAME)

#define DEFINE_GET_SET_CELL_LAYOUT_ATTR(FUNC, RET, ARG, NAME, DEF) \
	DEFINE_GET_SET_CELL_LAYOUT_ATTR_SUB(Body, FUNC, RET, ARG, NAME, DEF) \
	DEFINE_GET_SET_CELL_LAYOUT_ATTR_SUB(Header, FUNC, RET, ARG, NAME, DEF) \
	DEFINE_GET_SET_CELL_LAYOUT_ATTR_SUB(Footer, FUNC, RET, ARG, NAME, DEF) \
	DEFINE_SET_COLUMN_LAYOUT_ATTR(FUNC, ARG, NAME)

#define DEFINE_GET_SET_CELL_BOOL_LAYOUT_ATTR_SUB(SECTION, FUNC, NAME, DEF) \
	sl_bool GridView::is##SECTION##FUNC(sl_uint32 iRow, sl_uint32 iCol) \
	{ \
		ObjectLocker lock(this); \
		SECTION##CellProp* prop = _get##SECTION##CellProp(iRow, iCol); \
		if (prop) { \
			return prop->NAME; \
		} \
		return DEF; \
	} \
	DEFINE_SET_CELL_LAYOUT_ATTR_SUB(SECTION, FUNC, sl_bool, NAME)

#define DEFINE_GET_SET_CELL_BOOL_LAYOUT_ATTR(FUNC, NAME, DEF) \
	DEFINE_GET_SET_CELL_BOOL_LAYOUT_ATTR_SUB(Body, FUNC, NAME, DEF) \
	DEFINE_GET_SET_CELL_BOOL_LAYOUT_ATTR_SUB(Header, FUNC, NAME, DEF) \
	DEFINE_GET_SET_CELL_BOOL_LAYOUT_ATTR_SUB(Footer, FUNC, NAME, DEF) \
	DEFINE_SET_COLUMN_LAYOUT_ATTR(FUNC, sl_bool, NAME)

#define DEFINE_GET_SET_CELL_STATE_ATTR_SUB(SECTION, FUNC, RET, ARG, NAME, DEF) \
	RET GridView::get##SECTION##FUNC(sl_uint32 iRow, sl_uint32 iCol, ViewState state) \
	{ \
		ObjectLocker lock(this); \
		SECTION##CellProp* prop = _get##SECTION##CellProp(iRow, iCol); \
		if (prop) { \
			return prop->NAME##s.get(state); \
		} \
		return DEF; \
	} \
	void GridView::set##SECTION##FUNC(sl_int32 iRow, sl_int32 iCol, ARG value, ViewState state, UIUpdateMode mode) \
	{ \
		ObjectLocker lock(this); \
		if (iCol >= 0) { \
			if (iRow >= 0) { \
				SECTION##CellProp* prop = _get##SECTION##CellProp(iRow, iCol); \
				if (prop) { \
					prop->NAME##s.set(state, value); \
					invalidate(mode); \
				} \
			} else { \
				Ref<Column> col = m_columns.getValueAt_NoLock(iCol); \
				if (col.isNotNull()) { \
					Column* pCol = col.get(); \
					DEFINE_SET_SECTION_COLUMN_ATTR_SUB(SECTION, pCol, NAME##s.set(state, value)) \
					invalidate(mode); \
				} \
			} \
		} else { \
			if (iRow >= 0) { \
				Ref<Row> row = m_list##SECTION##Row.getValueAt_NoLock(iRow); \
				if (row.isNotNull()) { \
					Row* pRow = row.get(); \
					DEFINE_SET_SECTION_ROW_ATTR_SUB(SECTION, pRow, NAME##s.set(state, value)) \
					invalidate(mode); \
				} \
			} else { \
				DEFINE_SET_SECTION_ALL_ATTR_SUB(SECTION, NAME##s.set(state, value)) \
				invalidate(mode); \
			} \
		} \
	}

#define DEFINE_SET_COLUMN_STATE_ATTR(FUNC, ARG, NAME) \
	void GridView::setColumn##FUNC(sl_int32 iCol, ARG value, ViewState state, UIUpdateMode mode) \
	{ \
		ObjectLocker lock(this); \
		if (iCol >= 0) { \
			Ref<Column> col = m_columns.getValueAt_NoLock(iCol); \
			if (col.isNotNull()) { \
				Column* pCol = col.get(); \
				DEFINE_SET_COLUMN_ATTR_SUB(pCol, NAME##s.set(state, value)) \
				invalidate(mode); \
			} \
		} else { \
			DEFINE_SET_ALL_ATTR_SUB(NAME##s.set(state, value)) \
			invalidate(mode); \
		} \
	} \
	void GridView::setCell##FUNC(ARG value, ViewState state, UIUpdateMode mode) \
	{ \
		setColumn##FUNC(-1, value, state, mode); \
	}

#define DEFINE_GET_SET_CELL_STATE_ATTR(FUNC, RET, ARG, NAME, DEF) \
	DEFINE_GET_SET_CELL_STATE_ATTR_SUB(Body, FUNC, RET, ARG, NAME, DEF) \
	DEFINE_GET_SET_CELL_STATE_ATTR_SUB(Header, FUNC, RET, ARG, NAME, DEF) \
	DEFINE_GET_SET_CELL_STATE_ATTR_SUB(Footer, FUNC, RET, ARG, NAME, DEF) \
	DEFINE_SET_COLUMN_STATE_ATTR(FUNC, ARG, NAME)

#define DEFINE_GET_FONT(SECTION) \
	Ref<Font> GridView::get##SECTION##Font(sl_uint32 iRow, sl_uint32 iCol) \
	{ \
		ObjectLocker lock(this); \
		SECTION##CellProp* prop = _get##SECTION##CellProp(iRow, iCol); \
		if (prop) { \
			return prop->font; \
		} \
		return getFont(); \
	}

	DEFINE_GET_SET_CELL_LAYOUT_ATTR(Creator, GridView::CellCreator, const CellCreator&, creator, sl_null)
	DEFINE_GET_SET_CELL_LAYOUT_ATTR(Field, String, const String&, field, sl_null)
	DEFINE_GET_SET_CELL_LAYOUT_ATTR(Text, String, const String&, text, sl_null)
	DEFINE_GET_SET_CELL_LAYOUT_ATTR(TextGetter, GridView::TextGetter, const TextGetter&, textGetter, sl_null)
	DEFINE_SET_CELL_LAYOUT_ATTR(Font, const Ref<Font>&, font)
	DEFINE_GET_FONT(Body)
	DEFINE_GET_FONT(Header)
	DEFINE_GET_FONT(Footer)
	DEFINE_GET_SET_CELL_GENERIC_ATTR(Cursor, Ref<Cursor>, const Ref<Cursor>&, cursor, sl_null)
	DEFINE_GET_SET_CELL_GENERIC_ATTR(ToolTip, String, const String&, toolTip, sl_null)
	DEFINE_GET_SET_CELL_LAYOUT_ATTR(ToolTipGetter, GridView::TextGetter, const TextGetter&, toolTipGetter, sl_null)
	DEFINE_SET_CELL_LAYOUT_ATTR(Padding, sl_ui_len, padding)
	DEFINE_GET_SET_CELL_LAYOUT_ATTR(PaddingLeft, sl_ui_len, sl_ui_len, padding.left, 0)
	DEFINE_GET_SET_CELL_LAYOUT_ATTR(PaddingTop, sl_ui_len, sl_ui_len, padding.top, 0)
	DEFINE_GET_SET_CELL_LAYOUT_ATTR(PaddingRight, sl_ui_len, sl_ui_len, padding.right, 0)
	DEFINE_GET_SET_CELL_LAYOUT_ATTR(PaddingBottom, sl_ui_len, sl_ui_len, padding.bottom, 0)
	DEFINE_GET_SET_CELL_LAYOUT_ATTR(MultiLine, MultiLineMode, MultiLineMode, multiLineMode, MultiLineMode::Single)
	DEFINE_GET_SET_CELL_LAYOUT_ATTR(Ellipsize, EllipsizeMode, EllipsizeMode, ellipsizeMode, EllipsizeMode::None)
	DEFINE_GET_SET_CELL_LAYOUT_ATTR(LineCount, sl_uint32, sl_uint32, lineCount, 0)
	DEFINE_GET_SET_CELL_LAYOUT_ATTR(Alignment, Alignment, const Alignment&, align, 0)
	DEFINE_GET_SET_CELL_BOOL_ATTR(Selectable, flagSelectable, sl_false)
	DEFINE_GET_SET_CELL_BOOL_ATTR(Editable, flagEditable, sl_false)
	DEFINE_GET_SET_CELL_BOOL_LAYOUT_ATTR(BackgroundAntiAlias, flagBackgroundAntiAlias, sl_false)
	DEFINE_GET_SET_CELL_BOOL_LAYOUT_ATTR(ContentAntiAlias, flagContentAntiAlias, sl_true)
	DEFINE_GET_SET_CELL_BOOL_ATTR(UsingDefaultColorFilter, flagDefaultFilter, sl_false)
	DEFINE_GET_SET_CELL_LAYOUT_ATTR(BackgroundGetter, GridView::DrawableGetter, const DrawableGetter&, backgroundGetter, sl_null)
	DEFINE_GET_SET_CELL_LAYOUT_ATTR(TextColorGetter, GridView::ColorGetter, const ColorGetter&, textColorGetter, sl_null)
	DEFINE_GET_SET_CELL_LAYOUT_ATTR(IconGetter, GridView::DrawableGetter, const DrawableGetter&, iconGetter, sl_null)
	DEFINE_GET_SET_CELL_STATE_ATTR(Icon, Ref<Drawable>, const Ref<Drawable>&, icon, sl_null)
	DEFINE_GET_SET_CELL_LAYOUT_ATTR(IconWidth, sl_ui_len, sl_ui_len, iconWidth, -1)
	DEFINE_SET_CELL_LAYOUT_ATTR(IconMargin, sl_ui_len, iconMargin)
	DEFINE_GET_SET_CELL_LAYOUT_ATTR(IconMarginLeft, sl_ui_len, sl_ui_len, iconMargin.left, 0)
	DEFINE_GET_SET_CELL_LAYOUT_ATTR(IconMarginTop, sl_ui_len, sl_ui_len, iconMargin.top, 0)
	DEFINE_GET_SET_CELL_LAYOUT_ATTR(IconMarginRight, sl_ui_len, sl_ui_len, iconMargin.right, 0)
	DEFINE_GET_SET_CELL_LAYOUT_ATTR(IconMarginBottom, sl_ui_len, sl_ui_len, iconMargin.bottom, 0)
	DEFINE_GET_SET_CELL_LAYOUT_ATTR(IconScaleMode, ScaleMode, ScaleMode, iconScale, ScaleMode::Contain)
	DEFINE_GET_SET_CELL_LAYOUT_ATTR(IconAlignment, Alignment, const Alignment&, iconAlign, Alignment::MiddleCenter)
	DEFINE_GET_SET_CELL_STATE_ATTR(Background, Ref<Drawable>, const Ref<Drawable>&, background, sl_null)
	DEFINE_GET_SET_CELL_STATE_ATTR(TextColor, Color, const Color&, textColor, Color::zero())
	DEFINE_GET_SET_CELL_STATE_ATTR(ColorFilter, Shared<ColorMatrix>, const Shared<ColorMatrix>&, filter, sl_null)

	void GridView::setBodyAntiAlias(sl_int32 row, sl_int32 column, sl_bool flag, UIUpdateMode mode)
	{
		setBodyBackgroundAntiAlias(row, column, flag, UIUpdateMode::Init);
		setBodyContentAntiAlias(row, column, flag, mode);
	}

	void GridView::setHeaderAntiAlias(sl_int32 row, sl_int32 column, sl_bool flag, UIUpdateMode mode)
	{
		setHeaderBackgroundAntiAlias(row, column, flag, UIUpdateMode::Init);
		setHeaderContentAntiAlias(row, column, flag, mode);
	}

	void GridView::setFooterAntiAlias(sl_int32 row, sl_int32 column, sl_bool flag, UIUpdateMode mode)
	{
		setFooterBackgroundAntiAlias(row, column, flag, UIUpdateMode::Init);
		setFooterContentAntiAlias(row, column, flag, mode);
	}

	void GridView::setColumnAntiAlias(sl_int32 column, sl_bool flag, UIUpdateMode mode)
	{
		setColumnBackgroundAntiAlias(column, flag, UIUpdateMode::Init);
		setColumnContentAntiAlias(column, flag, mode);
	}

	void GridView::setCellAntiAlias(sl_bool flag, UIUpdateMode mode)
	{
		setBackgroundAntiAlias(flag, UIUpdateMode::Init);
		setContentAntiAlias(flag, mode);
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
			m_flagInvalidate##SECTION##Layout = sl_true; \
			invalidate(mode); \
		} \
	} \
	void GridView::set##SECTION##Colspan(sl_uint32 iRow, sl_uint32 iCol, sl_uint32 span, UIUpdateMode mode) \
	{ \
		ObjectLocker lock(this); \
		SECTION##CellProp* prop = _get##SECTION##CellProp(iRow, iCol); \
		if (prop) { \
			prop->colspan = span; \
			m_flagInvalidate##SECTION##Layout = sl_true; \
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
			m_flagInvalidate##SECTION##Layout = sl_true; \
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
		Ref<Column> col = m_columns.getValueAt_NoLock(iCol);
		if (col.isNotNull()) {
			if (record == HEADER) {
				HeaderCellProp* prop = col->m_listHeaderCell.getPointerAt(iRow);
				if (prop) {
					return prop->cell;
				}
			} else if (record == FOOTER) {
				FooterCellProp* prop = col->m_listFooterCell.getPointerAt(iRow);
				if (prop) {
					return prop->cell;
				}
			} else {
				BodyCellProp* prop = col->m_listBodyCell.getPointerAt(iRow);
				if (prop) {
					return prop->cells.getValue_NoLock(record);
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
		ListElements< Ref<Row> > rows(m_list##SECTION##Row); \
		if (rows.count) { \
			sl_ui_pos t = 0; \
			for (sl_size i = 0; i < rows.count - 1; i++) { \
				sl_ui_len h = rows[i]->m_fixedHeight; \
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
		ListElements< Ref<Column> > columns(m_columns);
		sl_uint32 nColumns = (sl_uint32)(columns.count);
		sl_uint32 nLeft = m_nLeftColumns;
		sl_uint32 nRight = m_nRightColumns;
		FixLeftRightColumnCount(nColumns, nLeft, nRight);
		sl_ui_len pos = 0;
		sl_uint32 i;
		for (i = 0; i < nLeft; i++) {
			sl_ui_len w = columns[i]->m_fixedWidth;
			if (x < pos + w) {
				return i;
			}
			pos += w;
		}
		sl_ui_len widthLeft = pos;
		pos = widthView;
		for (i = 0; i < nRight; i++) {
			sl_uint32 k = nColumns - 1 - i;
			sl_ui_len w = columns[k]->m_fixedWidth;
			if (pos - w <= x) {
				return k;
			}
			pos -= w;
		}
		nColumns -= nRight;
		x += (sl_ui_len)(getScrollX()) - widthLeft;
		pos = 0;
		for (i = nLeft; i < nColumns; i++) {
			sl_ui_len w = columns[i]->m_fixedWidth;
			if (x < pos + w) {
				return i;
			}
			pos += w;
		}
		return -1;
	}

	sl_bool GridView::_fixCellAddress(RecordIndex iRecord, sl_uint32 iRow, sl_uint32* outRow, sl_uint32 iCol, sl_uint32* outCol)
	{
		CellProp* prop = _getCellProp(iRecord, iRow, iCol);
		if (prop) {
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
			if (outCol) {
				while (prop->flagCoveredX && iCol > 0) {
					iCol--;
					prop = _getCellProp(iRecord, iRow, iCol);
					if (!prop) {
						return sl_false;
					}
				}
				*outCol = iCol;
			}
			return sl_true;
		}
		return sl_false;
	}

	void GridView::_fixSelection(Selection& sel)
	{
		if (sel.row < 0 || sel.column < 0) {
			return;
		}
		_fixCellAddress(sel.record, sel.row, (sl_uint32*)(&(sel.row)), sel.column, (sl_uint32*)(&(sel.column)));
	}

	sl_bool GridView::getCellAt(sl_ui_pos x, sl_ui_pos y, sl_uint32* outRow, sl_uint32* outColumn, RecordIndex* outRecord)
	{
		if (outRow || outColumn) {
			sl_int32 iRow;
			RecordIndex iRecord = _getRowAt(&iRow, y, sl_true, sl_true, sl_true);
			if (iRecord != OUTSIDE && iRow >= 0) {
				sl_int32 iCol = getColumnAt(x);
				if (iCol >= 0) {
					if (outRecord) {
						*outRecord = iRecord;
					}
					ObjectLocker lock(this);
					return _fixCellAddress(iRecord, iRow, outRow, iCol, outColumn);
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

	sl_bool GridView::getSelectionAt(sl_ui_pos x, sl_ui_pos y, Selection& sel)
	{
		sel.record = _getRowAt(&(sel.row), y, sl_true, sl_true, sl_true);
		if (sel.record != OUTSIDE && sel.row >= 0) {
			sel.column = getColumnAt(x);
			if (sel.column >= 0) {
				ObjectLocker lock(this);
				return _fixCellAddress(sel.record, sel.row, (sl_uint32*)(&(sel.row)), sel.column, (sl_uint32*)(&(sel.column)));
			} else {
				if (m_selectionMode == SelectionMode::Record || m_selectionMode == SelectionMode::Row) {
					return sl_true;
				}
			}
		}
		return sl_false;
	}

	sl_bool GridView::getCellLocation(UIPoint& _out, RecordIndex iRecord, sl_int32 _iRow, sl_int32 _iCol)
	{
		sl_ui_pos x, y;
		ObjectLocker lock(this);
		if (_iCol >= 0) {
			sl_uint32 iCol = _iCol;
			ListElements< Ref<Column> > columns(m_columns);
			sl_uint32 nColumns = (sl_uint32)(columns.count);
			if (iCol >= nColumns) {
				return sl_false;
			}
			sl_uint32 nLeft = m_nLeftColumns;
			sl_uint32 nRight = m_nRightColumns;
			FixLeftRightColumnCount(nColumns, nLeft, nRight);
			if (iCol < nLeft) {
				x = 0;
				for (sl_uint32 i = 0; i < iCol; i++) {
					x += columns[i]->m_fixedWidth;
				}
			} else {
				sl_uint32 iRight = nColumns - nRight;
				if (iCol < iRight) {
					x = -(sl_ui_pos)(getScrollX());
					for (sl_uint32 i = 0; i < iCol; i++) {
						x += columns[i]->m_fixedWidth;
					}
				} else {
					x = getWidth();
					for (sl_uint32 i = nColumns; i > iCol;) {
						i--;
						x -= columns[i]->m_fixedWidth;
					}
				}
			}
		} else {
			x = 0;
		}
		if (iRecord >= 0) {
			y = getHeaderHeight() + (sl_ui_pos)(iRecord * getRecordHeight() - (sl_int64)(getScrollY()));
			if (_iRow >= 0) {
				sl_uint32 iRow = _iRow;
				ListElements< Ref<Row> > rows(m_listBodyRow);
				if (iRow >= rows.count) {
					return sl_false;
				}
				for (sl_uint32 i = 0; i < iRow; i++) {
					y += rows[i]->m_fixedHeight;
				}
			}
		} else if (iRecord == HEADER) {
			y = 0;
			if (_iRow >= 0) {
				sl_uint32 iRow = _iRow;
				ListElements< Ref<Row> > rows(m_listHeaderRow);
				if (iRow >= rows.count) {
					return sl_false;
				}
				for (sl_uint32 i = 0; i < iRow; i++) {
					y += rows[i]->m_fixedHeight;
				}
			}
		} else if (iRecord == FOOTER) {
			y = getHeight() - getFooterHeight();
			if (_iRow >= 0) {
				sl_uint32 iRow = _iRow;
				ListElements< Ref<Row> > rows(m_listFooterRow);
				if (iRow >= rows.count) {
					return sl_false;
				}
				for (sl_uint32 i = 0; i < iRow; i++) {
					y += rows[i]->m_fixedHeight;
				}
			}
		} else {
			y = 0;
		}
		_out.x = x;
		_out.y = y;
		return sl_true;
	}

	sl_bool GridView::getCellFrame(UIRect& _out, RecordIndex iRecord, sl_int32 iRow, sl_int32 iCol)
	{
		UIPoint pt;
		if (getCellLocation(pt, iRecord, iRow, iCol)) {
			_out.setLeftTop(pt);
			ObjectLocker lock(this);
			if (iRow >= 0) {
				if (iCol >= 0) {
					CellProp* prop = _getCellProp(iRecord, iRow, iCol);
					if (prop) {
						_out.setWidth(prop->width);
						_out.setHeight(prop->height);
						return sl_true;
					}
				} else {
					Ref<Row> row;
					if (iRecord == HEADER) {
						row = m_listHeaderRow.getValueAt_NoLock(iRow);
					} else if (iRecord == FOOTER) {
						row = m_listFooterRow.getValueAt_NoLock(iRow);
					} else {
						row = m_listBodyRow.getValueAt_NoLock(iRow);
					}
					if (row.isNotNull()) {
						_out.setHeight(row->m_height);
						_out.setWidth(getWidth());
						return sl_true;
					}
				}
			} else {
				if (iRecord == HEADER) {
					_out.setHeight(getHeaderHeight());
				} else if (iRecord == FOOTER) {
					_out.setHeight(getFooterHeight());
				} else if (iRecord == OUTSIDE) {
					_out.setHeight(getHeight());
				} else {
					_out.setHeight(getRecordHeight());
				}
				if (iCol >= 0) {
					Ref<Column> col = m_columns.getValueAt_NoLock(iCol);
					if (col.isNotNull()) {
						_out.setWidth(col->m_fixedWidth);
						return sl_true;
					}
				} else {
					_out.setWidth(getWidth());
					return sl_true;
				}
			}
		}
		return sl_false;
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
		if (m_selectionMode == SelectionMode::Cell && !(cell->m_flagSelectable)) {
			return ViewState::Normal;
		}
		return getCellState(cell->record, cell->row, cell->column);
	}
	
	ViewState GridView::getCellContentState(RecordIndex record, sl_uint32 row, sl_uint32 column)
	{
		ViewState state;
		if (m_hover.matchCell(record, row, column)) {
			if (isPressedState()) {
				state = ViewState::Pressed;
			} else {
				state = ViewState::Hover;
			}
		} else {
			state = ViewState::Normal;
		}
		if (m_selection.matchCell(record, row, column)) {
			return (ViewState)((int)state + (int)(ViewState::Selected));
		} else {
			return state;
		}
	}

	ViewState GridView::getCellContentState(Cell* cell)
	{
		return getCellContentState(cell->record, cell->row, cell->column);
	}

	SLIB_DEFINE_EVENT_HANDLER_WITHOUT_ON(GridView, ClickCell, (GridView::Cell* cell, UIEvent* ev), cell, ev)

	void GridView::onClickCell(Cell* cell, UIEvent* ev)
	{
		cell->onClick(ev);
		if (ev->isAccepted()) {
			return;
		}
		if (cell->m_flagSelectable) {
			Selection selection;
			selection.record = cell->record;
			selection.row = cell->row;
			selection.column = cell->column;
			_select(selection, ev);
		}
	}

	SLIB_DEFINE_EVENT_HANDLER_WITHOUT_ON(GridView, RightButtonClickCell, (GridView::Cell* cell, UIEvent* ev), cell, ev)

	void GridView::onRightButtonClickCell(Cell* cell, UIEvent* ev)
	{
		cell->onEvent(ev);
	}

	SLIB_DEFINE_EVENT_HANDLER_WITHOUT_ON(GridView, DoubleClickCell, (GridView::Cell* cell, UIEvent* ev), cell, ev)

	void GridView::onDoubleClickCell(Cell* cell, UIEvent* ev)
	{
		cell->onEvent(ev);
	}

	SLIB_DEFINE_EVENT_HANDLER(GridView, Select, (const GridView::Selection& selection, const GridView::Selection& former, UIEvent* ev), selection, former, ev)

	SLIB_DEFINE_EVENT_HANDLER(GridView, Sort, (const String& field, sl_bool flagAscending), field, flagAscending)

	void GridView::_sort(Cell* cell)
	{
		if (m_cellSort == cell->attr) {
			if (m_flagSortAsc) {
				m_flagSortAsc = sl_false;
				m_onSort(cell->attr->field, sl_false, UIUpdateMode::Redraw);
				invokeSort(cell->attr->field, sl_false);
			} else {
				m_cellSort = sl_null;
				m_onSort(sl_null, sl_false, UIUpdateMode::Redraw);
				invokeSort(sl_null, sl_false);
			}
		} else {
			m_cellSort = cell->attr;
			m_flagSortAsc = sl_true;
			m_onSort(cell->attr->field, sl_true, UIUpdateMode::Redraw);
			invokeSort(cell->attr->field, sl_true);
		}
		invalidate();
	}

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

		m_currentFont = getFont();

		sl_ui_len defaultRowHeight = _getDefaultRowHeight();

		ListElements< Ref<Column> > columns(m_columns);
		sl_uint32 nColumns = (sl_uint32)(columns.count);
		sl_uint32 nLeft = m_nLeftColumns;
		sl_uint32 nRight = m_nRightColumns;
		FixLeftRightColumnCount(nColumns, nLeft, nRight);
		sl_uint32 iRight = nColumns - nRight;
		if (nColumns) {
			for (sl_uint32 i = 0; i < nColumns; i++) {
				Column* col = columns[i].get();
				if (col->m_flagVisible) {
					col->m_fixedWidth = col->m_width;
				} else {
					col->m_fixedWidth = 0;
				}
			}
		} else {
			return;
		}

		sl_ui_pos xLeft = 0;
		{
			for (sl_uint32 i = 0; i < nLeft; i++) {
				xLeft += columns[i]->m_fixedWidth;
			}
		}
		sl_ui_pos xRight = widthView;
		if (nRight) {
			sl_uint32 iRight = nColumns - nRight;
			sl_uint32 i = nColumns - 1;
			for (;;) {
				xRight -= columns[i]->m_fixedWidth;
				if (i > iRight) {
					i--;
				} else {
					break;
				}
			}
		}
		sl_bool flagExtendMid = sl_false;
		{
			sl_ui_len widthMid = 0;
			for (sl_uint32 i = nLeft; i < iRight; i++) {
				widthMid += columns[i]->m_fixedWidth;
			}
			if (widthMid < xRight - xLeft) {
				flagExtendMid = sl_true;
			}
		}

		sl_uint32 iStartMidColumn = nLeft;
		sl_uint32 nMidColumns = 0;
		sl_ui_pos xStartMidColumn = -(sl_ui_len)(getScrollX());
		{
			{
				for (sl_uint32 i = nLeft; i < iRight; i++) {
					sl_ui_len w = columns[i]->m_fixedWidth;
					if (0 < xStartMidColumn + w) {
						iStartMidColumn = i;
						break;
					}
					xStartMidColumn += w;
				}
			}
			if (iStartMidColumn > nLeft) {
				sl_uint32 newStart = iStartMidColumn;
				_fixHeaderStartMidColumn(columns.data, nColumns, nLeft, iStartMidColumn, newStart);
				_fixFooterStartMidColumn(columns.data, nColumns, nLeft, iStartMidColumn, newStart);
				_fixBodyStartMidColumn(columns.data, nColumns, nLeft, iStartMidColumn, newStart);
				for (sl_uint32 i = newStart; i < iStartMidColumn; i++) {
					xStartMidColumn -= columns[i]->m_fixedWidth;
				}
				iStartMidColumn = newStart;
			}
			{
				sl_uint32 i = iStartMidColumn;
				sl_ui_pos x = xStartMidColumn;
				for (; i < iRight && x < xRight; i++) {
					x += columns[i]->m_fixedWidth;
				}
				nMidColumns = i - iStartMidColumn;
			}
		}

		sl_ui_len heightHeader = 0;
		{
			ListElements< Ref<Row> > rows(m_listHeaderRow);
			for (sl_size i = 0; i < rows.count; i++) {
				Row* row = rows[i].get();
				if (row->m_flagVisible) {
					row->m_fixedHeight = FixLength(row->m_height, defaultRowHeight);
					heightHeader += row->m_fixedHeight;
				} else {
					row->m_fixedHeight = 0;
				}
			}
		}
		sl_ui_len heightFooter = 0;
		{
			ListElements< Ref<Row> > rows(m_listFooterRow);
			if (rows.count) {
				for (sl_size i = 0; i < rows.count; i++) {
					Row* row = rows[i].get();
					if (row->m_flagVisible) {
						row->m_fixedHeight = FixLength(row->m_height, defaultRowHeight);
						heightFooter += row->m_fixedHeight;
					} else {
						row->m_fixedHeight = 0;
					}
				}
			}
		}
		sl_ui_pos yFooter = heightView - heightFooter;

		if (m_nRecords) {
			ListElements< Ref<Row> > rows(m_listBodyRow);
			if (rows.count) {
				for (sl_size i = 0; i < rows.count; i++) {
					Row* row = rows[i].get();
					if (row->m_flagVisible) {
						row->m_fixedHeight = FixLength(row->m_height, defaultRowHeight);
					} else {
						row->m_fixedHeight = 0;
					}
				}
				CanvasStateScope scope(canvas);
				canvas->clipToRectangle(0, (sl_real)heightHeader, (sl_real)widthView, (sl_real)(yFooter - heightHeader));
				_drawRecords(canvas, heightHeader, yFooter, columns.data, nColumns, nLeft, xLeft, nRight, xRight, iStartMidColumn, nMidColumns, xStartMidColumn, flagExtendMid);
			}
		}
		if (heightFooter) {
			CanvasStateScope scope(canvas);
			canvas->clipToRectangle(0, (sl_real)heightHeader, (sl_real)widthView, (sl_real)(heightView - heightHeader));
			_drawFooter(canvas, yFooter, heightView, columns.data, nColumns, nLeft, xLeft, nRight, xRight, iStartMidColumn, nMidColumns, xStartMidColumn, flagExtendMid);
		}
		if (heightHeader) {
			_drawHeader(canvas, 0, heightHeader, columns.data, nColumns, nLeft, xLeft, nRight, xRight, iStartMidColumn, nMidColumns, xStartMidColumn, flagExtendMid);
		}

		if (xRight < widthView) {
			CanvasStateScope scope(canvas);
			canvas->clipToRectangle((sl_real)xLeft, 0, (sl_real)(widthView - xLeft), (sl_real)heightView);
			_drawVertOuterGrid(canvas, xRight, 0, heightHeader, yFooter, heightView, getHeaderGrid(), getRightGrid(), getFooterGrid());
		}
		if (xLeft > 0) {
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

		Selection selection = m_selection;
		if (selection.isCell()) {
			_fixSelection(selection);
			Ref<Pen> border = m_selectionBorder;
			if (border.isNotNull()) {
				UIRect frame;
				if (getCellFrame(frame, selection.record, selection.row, selection.column)) {
					if (selection.column >= 0) {
						sl_uint32 iCol = selection.column;
						if (iCol >= nLeft && iCol < iRight) {
							if (frame.left < xLeft) {
								frame.left = xLeft;
							}
							if (frame.right > xRight) {
								frame.right = xRight;
							}
						}
					}
					if (selection.record >= 0) {
						if (frame.top < heightHeader) {
							frame.top = heightHeader;
						}
						if (frame.bottom > yFooter) {
							frame.bottom = yFooter;
						}
					}
					if (frame.right > frame.left && frame.bottom > frame.top) {
						canvas->drawRectangle(frame, border);
					}
				}
			}
		}
	}

#define DEFINE_FIX_BODY_START_MID_COLUMN(SECTION) \
	void GridView::_fix##SECTION##StartMidColumn(Ref<Column>* columns, sl_uint32 nColumns, sl_uint32 nLeft, sl_uint32 iStart, sl_uint32& newStart) \
	{ \
		ListElements<SECTION##CellProp> props(columns[iStart]->m_list##SECTION##Cell); \
		for (sl_size i = 0; i < props.count; i++) { \
			if (props[i].flagCoveredX) { \
				sl_uint32 k = iStart; \
				while (k > nLeft) { \
					SECTION##CellProp* prop = columns[k]->m_list##SECTION##Cell.getPointerAt(i); \
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

	void GridView::_drawRecords(Canvas* canvas, sl_ui_len top, sl_ui_len bottom, Ref<Column>* columns, sl_uint32 nColumns, sl_uint32 nLeft, sl_ui_pos xLeft, sl_uint32 nRight, sl_ui_pos xRight, sl_uint32 iStartMidColumn, sl_uint32 nMidColumns, sl_ui_pos xStartMidColumn, sl_bool flagExtendMid)
	{
		sl_ui_len widthView = getWidth();

		_prepareBodyLayout(columns, nColumns);

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

		{
			for (sl_uint32 iCol = 0; iCol < nColumns; iCol++) {
				Column* col = columns[iCol].get();
				ListElements<BodyCellProp> props(col->m_listBodyCell);
				for (sl_size iRow = 0; iRow < props.count; iRow++) {
					BodyCellProp& prop = props[iRow];
					prop.cache = Move(prop.cells);
				}
			}
		}

		Function<Variant(sl_uint64 record)> recordDataFunc(m_recordData);
		ListElements< Ref<Row> > rows(m_listBodyRow);

		sl_ui_pos yRecord = yRecordFirst;
		for (sl_uint32 i = 0; i < nRecords; i++) {
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
					Column* col = columns[iCol].get();
					_drawBodyColumn(canvas, xCell, yRecord, col, iCol, rows.data, (sl_uint32)(rows.count), iRecord, recordData);
					xCell += col->m_fixedWidth;
				}
				if (flagExtendMid) {
					_drawExtendedColumn(canvas, xCell, xRight, yRecord, rows.data, (sl_uint32)(rows.count), iRecord, recordData);
				}
			}
			if (nRight) {
				CanvasStateScope scope(canvas);
				canvas->clipToRectangle((sl_real)xLeft, (sl_real)yRecord, (sl_real)(widthView - xLeft), (sl_real)heightRecord);
				sl_uint32 iCol = nColumns - 1;
				sl_ui_pos x = widthView;
				for (;;) {
					Column* col = columns[iCol].get();
					x -= col->m_fixedWidth;
					_drawBodyColumn(canvas, x, yRecord, col, iCol, rows.data, (sl_uint32)(rows.count), iRecord, recordData);
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
					Column* col = columns[iCol].get();
					_drawBodyColumn(canvas, x, yRecord, col, iCol, rows.data, (sl_uint32)(rows.count), iRecord, recordData);
					x += col->m_fixedWidth;
				}
			}
			yRecord += heightRecord;
			iRecord++;
		}

		{
			for (sl_uint32 iCol = 0; iCol < nColumns; iCol++) {
				Column* col = columns[iCol].get();
				ListElements<BodyCellProp> props(col->m_listBodyCell);
				for (sl_size iRow = 0; iRow < props.count; iRow++) {
					props[iRow].cache.setNull();
				}
			}
		}

		// Draw Grid
		{
			{
				CanvasStateScope scope(canvas);
				canvas->clipToRectangle((sl_real)xLeft, (sl_real)top, (sl_real)(xRight - xLeft), (sl_real)(bottom - top));
				_drawBodyInnerGrid(canvas, xLeft + xStartMidColumn, flagExtendMid ? xRight : 0, yRecordFirst, bottom, columns + iStartMidColumn, nMidColumns, rows.data, (sl_uint32)(rows.count), nRecords, sl_true, getBodyGrid());
			}
			{
				CanvasStateScope scope(canvas);
				canvas->clipToRectangle((sl_real)xLeft, (sl_real)top, (sl_real)(widthView - xLeft), (sl_real)(bottom - top));
				_drawBodyInnerGrid(canvas, xRight, 0, yRecordFirst, bottom, columns + iRight, nRight, rows.data, (sl_uint32)(rows.count), nRecords, sl_true, getRightGrid());
			}
			{
				_drawBodyInnerGrid(canvas, 0, 0, yRecordFirst, bottom, columns, nLeft, rows.data, (sl_uint32)(rows.count), nRecords, sl_true, getLeftGrid());
			}
		}
	}

#define DEFINE_DRAW_FIXED(SECTION, SECTION_CONSTANT) \
	void GridView::_draw##SECTION(Canvas* canvas, sl_ui_len top, sl_ui_len bottom, Ref<Column>* columns, sl_uint32 nColumns, sl_uint32 nLeft, sl_ui_pos xLeft, sl_uint32 nRight, sl_ui_pos xRight, sl_uint32 iStartMidColumn, sl_uint32 nMidColumns, sl_ui_pos xStartMidColumn, sl_bool flagExtendMid) \
	{ \
		sl_ui_len widthView = getWidth(); \
		_prepare##SECTION##Layout(columns, nColumns); \
		ListElements< Ref<Row> > rows(m_list##SECTION##Row); \
		sl_uint32 iRight = nColumns - nRight; \
		Ref<Pen> grid = get##SECTION##Grid(); \
		{ \
			CanvasStateScope scope(canvas); \
			canvas->clipToRectangle((sl_real)xLeft, (sl_real)top, (sl_real)(xRight - xLeft), (sl_real)(bottom - top)); \
			sl_ui_pos xCell = xLeft + xStartMidColumn; \
			for (sl_uint32 k = 0; k < nMidColumns; k++) { \
				sl_uint32 iCol = iStartMidColumn + k; \
				Column* col = columns[iCol].get(); \
				_draw##SECTION##Column(canvas, xCell, top, col, iCol, rows.data, (sl_uint32)(rows.count)); \
				xCell += col->m_fixedWidth; \
			} \
			if (flagExtendMid) { \
				_drawExtendedColumn(canvas, xCell, xRight, top, rows.data, (sl_uint32)(rows.count), SECTION_CONSTANT, Variant()); \
			} \
			_draw##SECTION##InnerGrid(canvas, xLeft + xStartMidColumn, flagExtendMid ? xRight : 0, top, bottom, columns + iStartMidColumn, nMidColumns, rows.data, (sl_uint32)(rows.count), 1, sl_false, grid); \
		} \
		if (nRight) { \
			CanvasStateScope scope(canvas); \
			canvas->clipToRectangle((sl_real)xLeft, (sl_real)top, (sl_real)(widthView - xLeft), (sl_real)(bottom - top)); \
			sl_uint32 iCol = nColumns - 1; \
			sl_ui_pos x = widthView; \
			for (;;) { \
				Column* col = columns[iCol].get(); \
				x -= col->m_fixedWidth; \
				_draw##SECTION##Column(canvas, x, top, col, iCol, rows.data, (sl_uint32)(rows.count)); \
				if (iCol > iRight) { \
					iCol--; \
				} else { \
					break; \
				} \
			} \
			_draw##SECTION##InnerGrid(canvas, xRight, 0, top, bottom, columns + iCol, nRight, rows.data, (sl_uint32)(rows.count), 1, sl_false, grid); \
		} \
		{ \
			sl_ui_pos x = 0; \
			for (sl_uint32 iCol = 0; iCol < nLeft; iCol++) { \
				Column* col = columns[iCol].get(); \
				_draw##SECTION##Column(canvas, x, top, col, iCol, rows.data, (sl_uint32)(rows.count)); \
				x += col->m_fixedWidth; \
			} \
			_draw##SECTION##InnerGrid(canvas, 0, 0, top, bottom, columns, nLeft, rows.data, (sl_uint32)(rows.count), 1, sl_false, grid); \
		} \
	}

	DEFINE_DRAW_FIXED(Header, HEADER)
	DEFINE_DRAW_FIXED(Footer, FOOTER)

	void GridView::_drawBodyColumn(Canvas* canvas, sl_ui_pos x, sl_ui_pos y, Column* col, sl_uint32 iCol, Ref<Row>* rows, sl_uint32 nRows, sl_uint64 iRecord, const Variant& recordData)
	{
		ListElements<BodyCellProp> props(col->m_listBodyCell);
		if (props.count > nRows) {
			props.count = nRows;
		}
		for (sl_uint32 iRow = 0; iRow < props.count; iRow++) {
			BodyCellProp& prop = props[iRow];
			if (prop.width && prop.height) {
				Ref<Cell> cell;
				if (!(prop.cells.get_NoLock(iRecord, &cell))) {
					cell = prop.cache.getValue_NoLock(iRecord);
					if (cell.isNull()) {
						cell = _createBodyCell(prop, iRecord, iRow, iCol, recordData);
					}
					prop.cells.add_NoLock(iRecord, cell);
				}
				if (cell.isNotNull()) {
					_drawCell(canvas, x, y, cell.get());
				}
			}
			y += rows[iRow]->m_fixedHeight;
		}
	}

#define DEFINE_DRAW_INNER_GRID(SECTION) \
	void GridView::_draw##SECTION##InnerGrid(Canvas* canvas, sl_ui_pos _x, sl_ui_pos xExtend, sl_ui_pos top, sl_ui_pos bottom, Ref<Column>* columns, sl_uint32 nColumns, Ref<Row>* rows, sl_uint32 nRows, sl_uint32 nRecords, sl_bool flagBody, const Ref<Pen>& pen) \
	{ \
		if (nColumns) { \
			/* Vertical Lines*/ \
			sl_ui_pos x = _x; \
			for (sl_uint32 iCol = 1; iCol < nColumns; iCol++) { \
				Column& colPrev = *(columns[iCol-1].get()); \
				sl_ui_len w = colPrev.m_fixedWidth; \
				if (!w) { \
					continue; \
				} \
				x += w; \
				if (!(colPrev.m_flag##SECTION##VerticalGrid)) { \
					continue; \
				} \
				ListElements<SECTION##CellProp> props(columns[iCol]->m_list##SECTION##Cell); \
				if (props.count > nRows) { \
					props.count = nRows; \
				} \
				sl_ui_pos y = top; \
				sl_ui_pos start = y; \
				for (sl_uint32 iRecord = 0; iRecord < nRecords; iRecord++) { \
					for (sl_uint32 iRow = 0; iRow < props.count; iRow++) { \
						sl_ui_pos y2 = y + rows[iRow]->m_fixedHeight; \
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
			if (xExtend) { \
				x += columns[nColumns-1]->m_fixedWidth; \
				canvas->drawLine((sl_real)x, (sl_real)top, (sl_real)x, (sl_real)bottom, pen); \
			} \
		} \
		if (nRecords && nRows) { \
			/* Horizontal Lines */ \
			sl_ui_pos xEnd; \
			if (xExtend) { \
				xEnd = xExtend; \
			} else { \
				xEnd = _x; \
				if (nRecords) { \
					for (sl_uint32 iCol = 0; iCol < nColumns; iCol++) { \
						xEnd += columns[iCol]->m_fixedWidth; \
					} \
				} \
			} \
			sl_ui_pos y = top; \
			for (sl_uint32 iRecord = 0; iRecord < nRecords; iRecord++) { \
				for (sl_uint32 iRow = 1; iRow < nRows; iRow++) { \
					Row& rowPrev = *(rows[iRow - 1].get()); \
					sl_ui_len h = rowPrev.m_fixedHeight; \
					if (!h) { \
						continue; \
					} \
					y += h; \
					if (!(rowPrev.m_flagHorizontalGrid)) { \
						continue; \
					} \
					sl_ui_pos x = _x; \
					sl_ui_pos start = x; \
					for (sl_uint32 iCol = 0; iCol < nColumns; iCol++) { \
						Column* col = columns[iCol].get(); \
						sl_ui_pos x2 = x + col->m_fixedWidth; \
						SECTION##CellProp* prop = col->m_list##SECTION##Cell.getPointerAt(iRow); \
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
					if (xExtend) { \
						x = xExtend; \
					} \
					if (x != start) { \
						canvas->drawLine((sl_real)start, (sl_real)y, (sl_real)x, (sl_real)y, pen); \
					} \
				} \
				y += rows[nRows-1]->m_fixedHeight; \
				if ((flagBody || iRecord + 1 < nRecords) && rows[nRows-1]->m_flagHorizontalGrid) { \
					canvas->drawLine((sl_real)_x, (sl_real)y, (sl_real)xEnd, (sl_real)y, pen); \
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
	void GridView::_draw##SECTION##Column(Canvas* canvas, sl_ui_pos x, sl_ui_pos y, Column* col, sl_uint32 iCol, Ref<Row>* rows, sl_uint32 nRows) \
	{ \
		ListElements<SECTION##CellProp> props(col->m_list##SECTION##Cell); \
		if (props.count > nRows) { \
			props.count = nRows; \
		} \
		for (sl_uint32 iRow = 0; iRow < props.count; iRow++) { \
			SECTION##CellProp& prop = props[iRow]; \
			if (prop.width && prop.height) { \
				Cell* cell = _getFixedCell(prop, SECTION_CONSTANT, iRow, iCol); \
				if (cell) { \
					_drawCell(canvas, x, y, cell); \
				} \
			} \
			y += rows[iRow]->m_fixedHeight; \
		} \
	}

	DEFINE_DRAW_FIXED_COLUMN(Header, HEADER)
	DEFINE_DRAW_FIXED_COLUMN(Footer, FOOTER)
	
	void GridView::_drawExtendedColumn(Canvas* canvas, sl_ui_pos x, sl_ui_pos xExtend, sl_ui_pos y, Ref<Row>* rows, sl_uint32 nRows, RecordIndex record, const Variant& recordData)
	{
		for (sl_uint32 iRow = 0; iRow < nRows; iRow++) {
			Ref<Row>& row = rows[iRow];
			sl_ui_len h = row->m_fixedHeight;
			Ref<Drawable> background;
			if (row->m_defaultProps.backgroundGetter.isNotNull()) {
				ViewState state = ViewState::Normal;
				if (m_selectionMode == SelectionMode::Record || m_selectionMode == SelectionMode::Row) {
					state = getCellState(record, iRow, -1);
				}
				Ref<Cell> cell = new Cell;
				if (cell.isNull()) {
					return;
				}
				cell->view = this;
				cell->record = record;
				cell->recordData = recordData;
				cell->row = iRow;
				cell->column = (sl_uint32)-1;
				cell->attr = &(row->m_defaultProps);
				background = row->m_defaultProps.backgroundGetter(cell.get(), state);
			} else {
				if (m_selectionMode == SelectionMode::Record || m_selectionMode == SelectionMode::Row) {
					background = row->m_defaultProps.backgrounds.evaluate(getCellState(record, iRow, -1));
				} else {
					background = row->m_defaultProps.backgrounds.getDefault();
				}
			}
			if (background.isNotNull()) {
				canvas->draw((sl_real)x, (sl_real)y, (sl_real)(xExtend - x), (sl_real)h, background);
			}
			y += h;
		}
	}

	void GridView::_drawCell(Canvas* canvas, sl_ui_pos x, sl_ui_pos y, Cell* cell)
	{
		CellAttribute* attr = cell->attr;
		ViewState state = getCellState(cell);

		CanvasStateScope scope(canvas);
		canvas->clipToRectangle((sl_real)x, (sl_real)y, (sl_real)(attr->width), (sl_real)(attr->height));

		DrawCellParam param;
		param.x = x;
		param.y = y;
		param.state = state;
		if (cell->m_flagUseContentState) {
			param.contentState = getCellContentState(cell);
		} else {
			param.contentState = state;
		}
		cell->draw(canvas, param);
	}

#define DEFINE_PREPARE_LAYOUT(SECTION) \
	void GridView::_prepare##SECTION##Layout(Ref<Column>* columns, sl_uint32 nColumns) \
	{ \
		if (!m_flagInvalidate##SECTION##Layout) { \
			return; \
		} \
		m_flagInvalidate##SECTION##Layout = sl_false; \
		{ \
			for (sl_uint32 iCol = 0; iCol < nColumns; iCol++) { \
				Column* col = columns[iCol].get(); \
				ListElements<SECTION##CellProp> props(col->m_list##SECTION##Cell); \
				for (sl_size iRow = 0; iRow < props.count; iRow++) { \
					SECTION##CellProp& prop = props[iRow]; \
					prop.flagCoveredX = sl_false; \
					prop.flagCoveredY = sl_false; \
				} \
			} \
		} \
		{ \
			ListElements< Ref<Row> > rows(m_list##SECTION##Row); \
			for (sl_uint32 iCol = 0; iCol < nColumns; iCol++) { \
				Column* col = columns[iCol].get(); \
				ListElements<SECTION##CellProp> props(col->m_list##SECTION##Cell); \
				sl_uint32 nRows = (sl_uint32)(props.count); \
				if (nRows > rows.count) { \
					nRows = (sl_uint32)(rows.count); \
				} \
				for (sl_uint32 iRow = 0; iRow < nRows; iRow++) { \
					SECTION##CellProp& prop = props[iRow]; \
					sl_ui_len oldWidth = prop.width; \
					sl_ui_len oldHeight = prop.height; \
					prop.width = col->m_fixedWidth; \
					prop.height = rows[iRow]->m_fixedHeight; \
					if (prop.flagCoveredX || prop.flagCoveredY) { \
						continue; \
					} \
					sl_uint32 iRowEnd = iRow + prop.rowspan; \
					if (iRowEnd > nRows) { \
						iRowEnd = nRows; \
					} \
					while (iRowEnd > iRow + 1) { \
						if (rows[iRowEnd - 1]->m_fixedHeight) { \
							break; \
						} \
						iRowEnd--; \
					} \
					sl_uint32 iColEnd = iCol + prop.colspan; \
					if (iColEnd > nColumns) { \
						iColEnd = nColumns; \
					} \
					while (iColEnd > iCol + 1) { \
						if (columns[iColEnd - 1]->m_fixedWidth) { \
							break; \
						} \
						iColEnd--; \
					} \
					sl_uint32 i; \
					for (i = iRow + 1; i < iRowEnd; i++) { \
						props[i].flagCoveredY = sl_true; \
						prop.height += rows[i]->m_fixedHeight; \
					} \
					for (i = iCol + 1; i < iColEnd; i++) { \
						Column* spanCol = columns[i].get(); \
						prop.width += spanCol->m_fixedWidth; \
						ListElements<SECTION##CellProp> spanProps(spanCol->m_list##SECTION##Cell); \
						if (iRowEnd <= spanProps.count) { \
							spanProps[iRow].flagCoveredX = sl_true; \
							for (sl_uint32 j = iRow + 1; j < iRowEnd; j++) { \
								SECTION##CellProp& cell = spanProps[j]; \
								cell.flagCoveredX = sl_true; \
								cell.flagCoveredY = sl_true; \
							} \
						} \
					} \
					if (oldWidth != prop.width || oldHeight != prop.height) { \
						_invalidate##SECTION##Cell(prop); \
					} \
				} \
			} \
		} \
	}

	DEFINE_PREPARE_LAYOUT(Body)
	DEFINE_PREPARE_LAYOUT(Header)
	DEFINE_PREPARE_LAYOUT(Footer)


	Ref<GridView::Cell> GridView::_createBodyCell(BodyCellProp& prop, RecordIndex iRecord, sl_uint32 iRow, sl_uint32 iCol, const Variant& recordData)
	{
		if (prop.flagCoveredX || prop.flagCoveredY) {
			return sl_null;
		}
		CellParam param;
		param.view = this;
		param.font = prop.font.isNotNull() ? prop.font : m_currentFont;
		param.attr = &prop;
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
		CellParam param;
		param.view = this;
		param.font = prop.font.isNotNull() ? prop.font : m_currentFont;
		param.attr = &prop;
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
		} else {
			if (m_selectionMode == SelectionMode::Record || m_selectionMode == SelectionMode::Row) {
				Selection selection;
				if (getSelectionAt((sl_ui_pos)(ev->getX()), (sl_ui_pos)(ev->getY()), selection)) {
					sl_bool flagSelectable = sl_false;
					ObjectLocker lock(this);
					if (selection.record >= 0) {
						if (selection.row >= 0) {
							Ref<Row>* row = m_listBodyRow.getPointerAt(selection.row);
							if (row) {
								flagSelectable = (*row)->m_defaultProps.flagSelectable;
							}
						} else {
							flagSelectable = m_defaultBodyProps.flagSelectable;
						}
					} else if (selection.record == HEADER) {
						if (selection.row >= 0) {
							Ref<Row>* row = m_listHeaderRow.getPointerAt(selection.row);
							if (row) {
								flagSelectable = (*row)->m_defaultProps.flagSelectable;
							}
						} else {
							flagSelectable = m_defaultHeaderProps.flagSelectable;
						}
					} else if (selection.record == FOOTER) {
						if (selection.row >= 0) {
							Ref<Row>* row = m_listFooterRow.getPointerAt(selection.row);
							if (row) {
								flagSelectable = (*row)->m_defaultProps.flagSelectable;
							}
						} else {
							flagSelectable = m_defaultFooterProps.flagSelectable;
						}
					}
					if (flagSelectable) {
						_select(selection, ev);
					}
				}
			}
		}
	}

	Ref<GridView::Cell> GridView::_getEventCell(UIEvent* ev)
	{
		if (ev->isMouseEvent()) {
			return getVisibleCellAt((sl_ui_pos)(ev->getX()), (sl_ui_pos)(ev->getY()));
		}
		return sl_null;
	}

	sl_int32 GridView::_getColumnForResizing(UIEvent* ev, sl_bool& flagRight, sl_bool& flagDual)
	{
		flagRight = sl_false;
		flagDual = sl_false;
		sl_ui_pos y = (sl_ui_pos)(ev->getY());
		sl_ui_len heightHeader = getHeaderHeight();
		if (heightHeader) {
			if (y > heightHeader) {
				return -1;
			}
		}
		ObjectLocker lock(this);
		ListElements< Ref<Column> > columns(m_columns);
		sl_uint32 nColumns = (sl_uint32)(columns.count);
		sl_uint32 nLeft = m_nLeftColumns;
		sl_uint32 nRight = m_nRightColumns;
		FixLeftRightColumnCount(nColumns, nLeft, nRight);
		sl_uint32 iRight = nColumns - nRight;
		sl_ui_pos xRight = getWidth();
		sl_ui_pos xLeft = 0;
		{
			for (sl_uint32 i = 0; i < nLeft; i++) {
				xLeft += columns[i]->m_fixedWidth;
			}
		}
		sl_ui_pos xEndMid = xLeft - (sl_ui_len)(getScrollX());
		{
			for (sl_uint32 i = nLeft; i < iRight; i++) {
				xEndMid += columns[i]->m_fixedWidth;
			}
		}
		if (nRight) {
			sl_uint32 i = nColumns - 1;
			for (;;) {
				xRight -= columns[i]->m_fixedWidth;
				if (i > iRight) {
					i--;
				} else {
					break;
				}
			}
		}
		sl_ui_pos x = (sl_ui_pos)(ev->getX());
		sl_ui_pos x2 = x;
		if (nRight && x + COLUMN_RESIZER_SIZE >= xRight) {
			if (x2 <= xRight) {
				x2 = xRight + 1;
			}
		} else if (nLeft && x <= xLeft + COLUMN_RESIZER_SIZE) {
			if (x2 >= xLeft) {
				x2 = xLeft - 1;
			}
		} else if (iRight > nLeft && x <= xEndMid + COLUMN_RESIZER_SIZE) {
			if (x2 >= xEndMid) {
				x2 = xEndMid - 1;
			}
		}
		RecordIndex iRecord;
		sl_uint32 iRow;
		sl_uint32 iCol;
		if (getCellAt(x2, y, &iRow, &iCol, &iRecord)) {
			UIRect rect;
			if (getCellFrame(rect, iRecord, iRow, iCol)) {
				if (x > rect.left + COLUMN_RESIZER_SIZE && x < rect.right - COLUMN_RESIZER_SIZE) {
					return -1;
				}
				sl_int32 _iCol = getColumnAt(x2);
				if (_iCol < 0) {
					return -1;
				}
				iCol = _iCol;
				if (x <= rect.left + COLUMN_RESIZER_SIZE) {
					if (iCol < iRight) {
						if (!iCol) {
							return -1;
						}
						iCol--;
					}
				} else {
					if (iCol >= iRight) {
						if (iCol + 1 == nColumns) {
							return -1;
						}
						iCol++;
					}
				}
				if (iCol >= iRight) {
					flagRight = sl_true;
				}
				if (ev->isShiftKey()) {
					if (iCol < nLeft) {
						if (iCol + 1 < nLeft) {
							flagDual = sl_true;
						}
					} else if (iCol < iRight) {
						if (iCol + 1 < iRight) {
							flagDual = sl_true;
						}
					} else {
						if (iCol != iRight) {
							flagDual = sl_true;
							iCol--;
						}
					}
				}
				if (!(isColumnResizable(iCol))) {
					return -1;
				}
				if (flagDual) {
					if (!(isColumnResizable(iCol + 1))) {
						return -1;
					}
				}
				return iCol;
			}
		}
		return -1;
	}

	sl_bool GridView::_processResizingColumn(UIEvent* ev)
	{
		ResizingColumn& attr = m_resizingColumn;
		UIAction action = ev->getAction();
		switch (action) {
			case UIAction::LeftButtonDown:
			case UIAction::TouchBegin:
				attr.index = _getColumnForResizing(ev, attr.flagRight, attr.flagDual);
				if (attr.index >= 0) {
					attr.formerWidth = getColumnWidth(attr.index);
					attr.formerWidth2 = getColumnWidth(attr.index + 1);
					attr.formerEventX = (sl_ui_pos)(ev->getX());
					return sl_true;
				}
				break;
			case UIAction::LeftButtonDrag:
			case UIAction::TouchMove:
				if (attr.index >= 0) {
					sl_ui_len dx = (sl_ui_pos)(ev->getX()) - attr.formerEventX;
					if (attr.flagDual) {
						setColumnWidth(attr.index, attr.formerWidth + dx, UIUpdateMode::None);
						setColumnWidth(attr.index + 1, attr.formerWidth2 - dx);
					} else if (attr.flagRight) {
						setColumnWidth(attr.index, attr.formerWidth - dx);
					} else {
						setColumnWidth(attr.index, attr.formerWidth + dx);
					}
					return sl_true;
				}
				break;
			case UIAction::LeftButtonUp:
			case UIAction::TouchEnd:
				if (attr.index >= 0) {
					attr.index = -1;
					return sl_true;
				}
				break;
			default:
				break;
		}
		return sl_false;
	}

	void GridView::onMouseEvent(UIEvent* ev)
	{
		if (_processResizingColumn(ev)) {
			return;
		}
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
					Selection selection;
					if (!(getSelectionAt((sl_ui_pos)(ev->getX()), (sl_ui_pos)(ev->getY()), selection))) {
						selection.record = OUTSIDE;
						selection.row = -1;
						selection.column = -1;
					}
					if (!(m_hover == selection)) {
						m_hover = selection;
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

	void GridView::onSetCursor(UIEvent* ev)
	{
		sl_bool flagRight, flagBalance;
		if (_getColumnForResizing(ev, flagRight, flagBalance) >= 0) {
			ev->setCursor(Cursor::getResizeLeftRight());
			ev->accept();
			return;
		}
		Ref<Cell> cell = _getEventCell(ev);
		if (cell.isNotNull()) {
			cell->onEvent(ev);
		}
	}

	sl_ui_len GridView::_getMiddleColumnOffset(sl_uint32 iCol)
	{
		ObjectLocker lock(this);
		ListElements< Ref<Column> > columns(m_columns);
		sl_uint32 nColumns = (sl_uint32)(columns.count);
		sl_uint32 nLeft = m_nLeftColumns;
		sl_uint32 nRight = m_nRightColumns;
		FixLeftRightColumnCount(nColumns, nLeft, nRight);
		if (iCol >= nLeft && iCol + nRight < nColumns) {
			sl_ui_len offset = 0;
			for (sl_uint32 i = nLeft; i < iCol; i++) {
				offset += columns[i]->m_fixedWidth;
			}
			return offset;
		} else {
			return -1;
		}
	}

	void GridView::onKeyEvent(UIEvent* ev)
	{
		if (ev->getAction() == UIAction::KeyDown) {
			Keycode keycode = ev->getKeycode();
			switch (keycode) {
				case Keycode::C:
					if ((ev->isControlKey() || ev->isCommandKey())) {
						Ref<Cell> cell = getVisibleCell(m_selection.record, m_selection.row, m_selection.column);
						if (cell.isNotNull()) {
							cell->onCopy();
							return;
						}
					}
					break;
				case Keycode::Left:
					if (m_selection.column > 0) {
						m_selection.column--;
						sl_ui_len offset = _getMiddleColumnOffset(m_selection.column);
						if (offset >= 0) {
							if ((sl_ui_len)(getScrollX()) > offset) {
								scrollToX((sl_scroll_pos)offset, UIUpdateMode::None);
							}
						}
						invalidate();
						return;
					}
					break;
				case Keycode::Right:
					if (m_selection.column >= 0 && (sl_uint32)(m_selection.column + 1) < m_columns.getCount()) {
						m_selection.column++;
						sl_ui_len offset = _getMiddleColumnOffset(m_selection.column);
						if (offset >= 0) {
							Ref<Column> col = m_columns.getValueAt_NoLock(m_selection.column);
							if (col.isNotNull()) {
								offset += col->m_fixedWidth;
								sl_ui_len page = (sl_ui_len)(getPageWidth());
								if (offset > (sl_ui_len)(getScrollX()) + page) {
									scrollToX(offset - page, UIUpdateMode::None);
								}
							}
						}
						invalidate();
						return;
					}
					break;
				case Keycode::Up:
					if (m_selection.record >= 0) {
						if (m_selectionMode == SelectionMode::Record || m_selection.row < 0) {
							if (m_selection.record > 0) {
								m_selection.record--;
							} else {
								break;
							}
						} else {
							if (m_selection.row > 0 || m_selection.record > 0) {
								if (m_selection.row > 0) {
									m_selection.row--;
								} else {
									sl_uint32 nRows = (sl_uint32)(m_listBodyRow.getCount());
									if (!nRows) {
										break;
									}
									m_selection.record--;
									m_selection.row = nRows - 1;
								}
							} else {
								break;
							}
						}
						sl_int64 offset = m_selection.record * getRecordHeight();
						if ((sl_ui_len)(getScrollY()) > offset) {
							scrollToY((sl_scroll_pos)offset, UIUpdateMode::None);
						}
						invalidate();
						return;
					} else {
						if (m_selection.row > 0) {
							m_selection.row--;
							invalidate();
							return;
						}
					}
					break;
				case Keycode::Down:
					if (m_selection.record >= 0) {
						if (m_selectionMode == SelectionMode::Record || m_selection.row < 0) {
							if ((sl_uint64)(m_selection.record + 1) < m_nRecords) {
								m_selection.record++;
							} else {
								break;
							}
						} else {
							sl_uint32 nRows = (sl_uint32)(m_listBodyRow.getCount());
							if ((sl_uint32)(m_selection.row + 1) < nRows) {
								m_selection.row++;
							} else {
								if ((sl_uint64)(m_selection.record + 1) < m_nRecords) {
									m_selection.record++;
									m_selection.row = 0;
								} else {
									break;
								}
							}
						}
						sl_int64 offset = (m_selection.record + 1) * getRecordHeight();
						sl_ui_len page = (sl_ui_len)(getPageHeight());
						if ((sl_ui_len)(getScrollY()) + page < offset) {
							scrollToY((sl_scroll_pos)(offset - page), UIUpdateMode::None);
						}
						invalidate();
						return;
					} else {
						if (m_selection.row < 0) {
							break;
						}
						if (m_selection.record == HEADER) {
							sl_uint32 nRows = (sl_uint32)(m_listHeaderRow.getCount());
							if ((sl_uint32)(m_selection.row + 1) < nRows) {
								m_selection.row++;
								invalidate();
								return;
							}
						} else if (m_selection.record == FOOTER) {
							sl_uint32 nRows = (sl_uint32)(m_listFooterRow.getCount());
							if ((sl_uint32)(m_selection.row + 1) < nRows) {
								m_selection.row++;
								invalidate();
								return;
							}
						}
					}
					break;
				default:
					break;
			}
		}
		View::onKeyEvent(ev);
	}

	void GridView::onResize(sl_ui_len width, sl_ui_len height)
	{
		View::onResize(width, height);
		refreshContentWidth(UIUpdateMode::None);
		refreshContentHeight(UIUpdateMode::None);
	}

	void GridView::onUpdateFont(const Ref<Font>& font)
	{
		ObjectLocker lock(this);
		_invalidateAllCells();
		refreshContentHeight(UIUpdateMode::None);
	}

	void GridView::_invalidateLayout()
	{
		m_flagInvalidateBodyLayout = sl_true;
		m_flagInvalidateHeaderLayout = sl_true;
		m_flagInvalidateFooterLayout = sl_true;
	}

	void GridView::_invalidateBodyCell(BodyCellProp& prop)
	{
		prop.cache.setNull();
		prop.cells.setNull();
	}

#define DEFINE_INVALIDATE_FIXED_CELL(SECTION) \
	void GridView::_invalidate##SECTION##Cell(SECTION##CellProp& prop) \
	{ \
		prop.cell.setNull(); \
		prop.flagMadeCell = sl_false; \
	}

	DEFINE_INVALIDATE_FIXED_CELL(Header)
	DEFINE_INVALIDATE_FIXED_CELL(Footer)

#define DEFINE_INVALIDATE_COLUMN_CELLS(SECTION) \
	void GridView::_invalidate##SECTION##ColumnCells(Column* col) \
	{ \
		ListElements<SECTION##CellProp> props(col->m_list##SECTION##Cell); \
		for (sl_size iRow = 0; iRow < props.count; iRow++) { \
			SECTION##CellProp& prop = props[iRow]; \
			_invalidate##SECTION##Cell(prop); \
		} \
	}

	DEFINE_INVALIDATE_COLUMN_CELLS(Body)
	DEFINE_INVALIDATE_COLUMN_CELLS(Header)
	DEFINE_INVALIDATE_COLUMN_CELLS(Footer)

#define DEFINE_INVALIDATE_ROW_CELLS(SECTION) \
	void GridView::_invalidate##SECTION##RowCells(Row* row) \
	{ \
		sl_uint32 index = row->m_index; \
		ListElements< Ref<Column> > columns(m_columns); \
		for (sl_size iCol = 0; iCol < columns.count; iCol++) { \
			Column* col = columns[iCol].get(); \
			SECTION##CellProp* prop = col->m_list##SECTION##Cell.getPointerAt(index); \
			if (prop) { \
				_invalidate##SECTION##Cell(*prop); \
			} \
		} \
	}

	DEFINE_INVALIDATE_ROW_CELLS(Body)
	DEFINE_INVALIDATE_ROW_CELLS(Header)
	DEFINE_INVALIDATE_ROW_CELLS(Footer)
		
#define DEFINE_INVALIDATE_ALL_CELLS(SECTION) \
	void GridView::_invalidate##SECTION##AllCells() \
	{ \
		ListElements< Ref<Column> > columns(m_columns); \
		for (sl_size iCol = 0; iCol < columns.count; iCol++) { \
			Column* col = columns[iCol].get(); \
			_invalidate##SECTION##ColumnCells(col); \
		} \
	}

	DEFINE_INVALIDATE_ALL_CELLS(Body)
	DEFINE_INVALIDATE_ALL_CELLS(Header)
	DEFINE_INVALIDATE_ALL_CELLS(Footer)

	void GridView::_invalidateAllCells()
	{
		ListElements< Ref<Column> > columns(m_columns);
		for (sl_size iCol = 0; iCol < columns.count; iCol++) {
			Column* col = columns[iCol].get();
			_invalidateBodyColumnCells(col);
			_invalidateHeaderColumnCells(col);
			_invalidateFooterColumnCells(col);
		}
	}

}
