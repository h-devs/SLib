/*
 *   Copyright (c) 2008-2020 SLIBIO <https://github.com/SLIBIO>
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

#include "slib/ui/label_view.h"

#include "slib/ui/core.h"
#include "slib/ui/cursor.h"
#include "slib/ui/clipboard.h"
#include "slib/graphics/util.h"

#include "../resources.h"

namespace slib
{

	SLIB_DEFINE_OBJECT(LabelView, View)

	LabelView::LabelView()
	{
		setSavingCanvasState(sl_false);
		setUsingFont(sl_true);

		setPadding(1, 1, 1, 1, UIUpdateMode::Init);

		m_cell = new LabelViewCell;
		m_flagContextMenu = sl_false;
	}

	LabelView::~LabelView()
	{
	}

	void LabelView::init()
	{
		View::init();

		setAntiAlias(sl_true, UIUpdateMode::Init);

		m_cell->setView(this, sl_true);
		m_cell->onClickLink = SLIB_FUNCTION_WEAKREF(this, invokeClickLink);
	}

	String LabelView::getText()
	{
		return m_cell->text;
	}

	sl_bool LabelView::isHyperText()
	{
		return m_cell->flagHyperText;
	}

	void LabelView::setText(const String& text, UIUpdateMode mode)
	{
		m_cell->text = text;
		if (m_cell->flagMnemonic) {
			setMnemonicKeyFromText(text);
		}
		m_cell->flagHyperText = sl_false;
		invalidateLayoutOfWrappingControl(mode);
	}

	void LabelView::setHyperText(const String& text, UIUpdateMode mode)
	{
		m_cell->text = text;
		m_cell->flagHyperText = sl_true;
		invalidateLayoutOfWrappingControl(mode);
	}

	String LabelView::getPlainText()
	{
		return m_cell->getPlainText();
	}

	MultiLineMode LabelView::getMultiLine()
	{
		return m_cell->multiLineMode;
	}

	void LabelView::setMultiLine(MultiLineMode multiLineMode, UIUpdateMode updateMode)
	{
		m_cell->multiLineMode = multiLineMode;
		invalidateLayoutOfWrappingControl(updateMode);
	}

	sl_uint32 LabelView::getLineCount()
	{
		return m_cell->lineCount;
	}

	void LabelView::setLineCount(sl_uint32 nLines, UIUpdateMode updateMode)
	{
		m_cell->lineCount = nLines;
		invalidateLayoutOfWrappingControl(updateMode);
	}

	sl_bool LabelView::isMnemonic()
	{
		return m_cell->flagMnemonic;
	}

	void LabelView::setMnemonic(sl_bool flag)
	{
		m_cell->flagMnemonic = flag;
	}

	Color LabelView::getTextColor(ViewState state)
	{
		return m_cell->textColors.get(state);
	}

	void LabelView::setTextColor(const Color& color, ViewState state, UIUpdateMode updateMode)
	{
		m_cell->textColors.set(state, color);
		if (state != ViewState::Default) {
			setRedrawingOnChangeState(sl_true);
		}
		invalidate(updateMode);
	}

	void LabelView::setTextColor(const Color& color, UIUpdateMode updateMode)
	{
		m_cell->textColors.defaultValue = color;
		invalidate(updateMode);
	}

	Alignment LabelView::getGravity()
	{
		return m_cell->gravity;
	}

	void LabelView::setGravity(const Alignment& align, UIUpdateMode updateMode)
	{
		m_cell->gravity = align;
		invalidate(updateMode);
	}

	EllipsizeMode LabelView::getEllipsize()
	{
		return m_cell->ellipsizeMode;
	}

	void LabelView::setEllipsize(EllipsizeMode ellipsizeMode, UIUpdateMode updateMode)
	{
		m_cell->ellipsizeMode = ellipsizeMode;
		invalidate(updateMode);
	}

	sl_bool LabelView::isDetectingHyperlinksInPlainText()
	{
		return m_cell->flagEnabledHyperlinksInPlainText;
	}

	void LabelView::setDetectingHyperlinksInPlainText(sl_bool flag, UIUpdateMode updateMode)
	{
		m_cell->flagEnabledHyperlinksInPlainText = flag;
		invalidate(updateMode);
	}

	Color LabelView::getLinkColor()
	{
		Color color = m_cell->linkColor;
		if (color.isNotZero()) {
			return color;
		}
		return TextParagraph::getDefaultLinkColor();
	}

	void LabelView::setLinkColor(const Color& color, UIUpdateMode updateMode)
	{
		m_cell->linkColor = color;
		invalidate(updateMode);
	}

	Color LabelView::getLineColor()
	{
		Color color = m_cell->lineColor;
		if (color.isNotZero()) {
			return color;
		}
		return getTextColor();
	}

	void LabelView::setLineColor(const Color& color, UIUpdateMode updateMode)
	{
		m_cell->lineColor = color;
		invalidate(updateMode);
	}

	sl_bool LabelView::isUsingContextMenu()
	{
		return m_flagContextMenu;
	}

	void LabelView::setUsingContextMenu(sl_bool flag)
	{
		m_flagContextMenu = flag;
	}

	UISize LabelView::measureSize()
	{
		return m_cell->measureSize();
	}

	SLIB_DEFINE_EVENT_HANDLER_WITHOUT_ON(LabelView, ClickLink, (const String& href, UIEvent* ev), href, ev)

	void LabelView::onClickLink(const String& href, UIEvent *ev)
	{
		UI::openUrl(href);
	}

	void LabelView::onDraw(Canvas* canvas)
	{
		if (isLayer() || getCurrentBackground().isNotNull()) {
			m_cell->shadowOpacity = 0;
		} else {
			sl_real shadowOpacity = (sl_real)(getShadowOpacity());
			m_cell->shadowOpacity = shadowOpacity;
			if (shadowOpacity > 0) {
				m_cell->shadowRadius = getShadowRadius();
				m_cell->shadowColor = getShadowColor();
				m_cell->shadowOffset = getShadowOffset();
			}
		}
		prepareLabelViewCellLayout(m_cell.get());
		m_cell->onDraw(canvas);
	}

	void LabelView::onClickEvent(UIEvent* ev)
	{
		View::onClickEvent(ev);
		m_cell->onClickEvent(ev);
		if (ev->isAccepted()) {
			return;
		}
		if (m_flagContextMenu) {
			auto menu = menu::label_view_context::get();
			if (menu) {
				Ref<LabelView> label = this;
				menu->copy->setAction([label]() {
					Clipboard::setText(label->getPlainText());
				});
				menu->root->show(convertCoordinateToScreen(ev->getPoint()));
			}
		}
	}

	void LabelView::onSetCursor(UIEvent* ev)
	{
		m_cell->onSetCursor(ev);
	}

	void LabelView::onUpdateLayout()
	{
		prepareLabelViewCellLayout(m_cell.get());
		updateLayoutByViewCell(m_cell.get());
	}

	void LabelView::prepareLabelViewCellLayout(LabelViewCell* cell)
	{
		cell->flagWrapping = isLastWidthWrapping();
		if (isMaximumWidthDefined()) {
			sl_ui_len width = getMaximumWidth() - getPaddingLeft() - getPaddingRight();
			if (width < 1) {
				width = 1;
			}
			cell->maxWidth = width;
		} else {
			cell->maxWidth = 0;
		}
	}


	SLIB_DEFINE_OBJECT(LabelViewCell, ViewCell)

	LabelViewCell::LabelViewCell()
	{
		flagHyperText = sl_false;
		flagMnemonic = sl_true;
		multiLineMode = MultiLineMode::Single;
		lineCount = 0;

		textColors.defaultValue = Color::Black;
		gravity = Alignment::Left;
		ellipsizeMode = EllipsizeMode::None;
		flagEnabledHyperlinksInPlainText = sl_false;
		linkColor = Color::Zero;

		shadowOpacity = 0;
		shadowRadius = 3;
		shadowOffset.x = 0;
		shadowOffset.y = 0;
		shadowColor = Color::Black;

		flagWrapping = sl_false;
		maxWidth = 0;

		m_textHeight = 0;
	}

	LabelViewCell::~LabelViewCell()
	{
	}

	String LabelViewCell::getPlainText()
	{
		if (flagHyperText) {
			return m_textBox.getPlainText();
		} else {
			return text;
		}
	}

	UISize LabelViewCell::measureSize()
	{
		_updateTextBox(getWidth());
		sl_ui_len width = (sl_ui_len)(m_textBox.getContentWidth());
		sl_ui_len height = (sl_ui_len)(m_textBox.getContentHeight());
		return UISize(width, height);
	}

	void LabelViewCell::_updateTextBox(sl_ui_len width)
	{
		_updateTextBox(flagWrapping, width, 0, gravity);
	}

	void LabelViewCell::_updateTextBox(sl_bool flagWrapping, sl_ui_len width, sl_ui_len padding, const Alignment& align)
	{
		if (flagWrapping) {
			if (maxWidth) {
				width = maxWidth - padding;
				if (width < 1) {
					width = 0;
				}
			} else {
				width = 0;
			}
		} else {
			width -= padding;
			if (width < 1) {
				width = 1;
			}
		}
		TextBoxParam param;
		param.font = getFont();
		param.text = text;
		param.flagHyperText = flagHyperText;
		param.flagMnemonic = flagMnemonic;
		param.width = (sl_real)width;
		param.multiLineMode = multiLineMode;
		param.lineCount = lineCount;
		param.align = align;
		param.ellipsizeMode = ellipsizeMode;
		param.flagEnabledHyperlinksInPlainText = flagEnabledHyperlinksInPlainText;
		m_textBox.update(param);

		if (param.text.isEmpty()) {
			if (param.font.isNotNull()) {
				m_textHeight = (sl_ui_len)(param.font->getFontHeight());
			} else {
				m_textHeight = 0;
			}
		} else {
			m_textHeight = (sl_ui_len)(m_textBox.getContentHeight());
		}
	}

	void LabelViewCell::onDraw(Canvas* canvas)
	{
		UIRect bounds = getFrame();
		if (bounds.getWidth() < 1 || bounds.getHeight() < 1) {
			return;
		}
		_updateTextBox(bounds.getWidth());
		TextBox::DrawParam param;
		param.frame = bounds;
		param.textColor = textColors.evaluate(getState());
		if (shadowOpacity > 0) {
			param.shadowOpacity = shadowOpacity;
			param.shadowRadius = shadowRadius;
			param.shadowColor = shadowColor;
			param.shadowOffset = shadowOffset;
		}
		param.lineThickness = UI::dpToPixel(1);
		if (param.lineThickness < 1) {
			param.lineThickness = 1;
		}
		param.linkColor = linkColor;
		if (param.linkColor.isZero()) {
			param.linkColor = TextParagraph::getDefaultLinkColor();
		}
		param.lineColor = lineColor;
		m_textBox.draw(canvas, param);
	}

	void LabelViewCell::onClickEvent(UIEvent* ev)
	{
		Ref<TextItem> item = m_textBox.getTextItemAtLocation(ev->getX(), ev->getY(), getFrame());
		if (item.isNotNull()) {
			Ref<TextStyle> style = item->getStyle();
			if (style.isNotNull()) {
				if (style->flagLink) {
					onClickLink(style->href, ev);
					ev->accept();
				}
			}
		}
	}

	void LabelViewCell::onSetCursor(UIEvent* ev)
	{
		Ref<TextItem> item = m_textBox.getTextItemAtLocation(ev->getX(), ev->getY(), getFrame());
		if (item.isNotNull()) {
			Ref<TextStyle> style = item->getStyle();
			if (style.isNotNull()) {
				if (style->flagLink) {
					ev->setCursor(Cursor::getHand());
					ev->accept();
				}
			}
		}
	}

	void LabelViewCell::onMeasure(UISize& size, sl_bool flagHorizontalWrapping, sl_bool flagVerticalWrapping)
	{
		if (!flagVerticalWrapping && !flagHorizontalWrapping) {
			return;
		}
		_updateTextBox(size.x);
		if (flagHorizontalWrapping) {
			size.x = (sl_ui_len)(m_textBox.getContentWidth());
		}
		if (flagVerticalWrapping) {
			size.y = m_textHeight;
		}
	}

}
