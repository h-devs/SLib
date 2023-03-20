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

#include "slib/ui/button.h"

#include "slib/ui/view_attributes.h"
#include "slib/ui/priv/view_state_map.h"
#include "slib/ui/core.h"
#include "slib/ui/cursor.h"
#include "slib/graphics/util.h"
#include "slib/core/safe_static.h"
#include "slib/core/new_helper.h"

#if defined(SLIB_UI_IS_MACOS) || defined(SLIB_UI_IS_WIN32) || defined(SLIB_UI_IS_GTK)
#	define HAS_NATIVE_WIDGET_IMPL 1
#else
#	define HAS_NATIVE_WIDGET_IMPL 0
#endif

#define BUTTON_TEXT_DEFAULT_COLOR Color(0, 100, 200)

namespace slib
{

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(ButtonCategory)

	ButtonCategory::ButtonCategory()
	{
	}


	SLIB_DEFINE_OBJECT(Button, View)

	Button::Button()
	{
		setSupportedNativeWidget(HAS_NATIVE_WIDGET_IMPL);

		setCursor(Cursor::getHand());
		setSavingCanvasState(sl_false);
		setUsingFont(sl_true);
		setFocusable(sl_true);
		setRedrawingOnChangeState(sl_true);

		m_flagDefaultButton = sl_false;
	}

	Button::~Button()
	{
	}

	void Button::init()
	{
		View::init();
		setPadding(1, 1, 1, 1, UIUpdateMode::Init);
		setAntiAlias(sl_true, UIUpdateMode::Init);
	}

	String Button::getText()
	{
		return m_text;
	}

	sl_bool Button::isHyperText()
	{
		if (m_cell.isNotNull()) {
			return m_cell->flagHyperText;
		}
		return sl_false;
	}

	void Button::setText(const String& text, UIUpdateMode mode)
	{
		Ptr<IButtonInstance> instance = getButtonInstance();
		if (instance.isNotNull()) {
			SLIB_VIEW_RUN_ON_UI_THREAD(setText, text, mode)
		} else {
			if (isMnemonic()) {
				setMnemonicKeyFromText(text);
			}
		}
		m_text = text;
		if (m_cell.isNotNull()) {
			m_cell->flagHyperText = sl_false;
			m_cell->text = text;
		}
		if (instance.isNotNull()) {
			instance->setText(this, text);
			if (!SLIB_UI_UPDATE_MODE_IS_UPDATE_LAYOUT(mode)) {
				return;
			}
		}
		invalidateLayoutOfWrappingControl(mode);
	}

	void Button::setHyperText(const String& text, UIUpdateMode mode)
	{
		_initCell();
		if (m_cell.isNotNull()) {
			m_cell->text = text;
			m_cell->flagHyperText = sl_true;
			invalidateLayoutOfWrappingControl(mode);
		}
	}

	MultiLineMode Button::getMultiLine()
	{
		if (m_cell.isNotNull()) {
			return m_cell->multiLineMode;
		}
		return MultiLineMode::Single;
	}

	void Button::setMultiLine(MultiLineMode multiLineMode, UIUpdateMode updateMode)
	{
		if (multiLineMode != MultiLineMode::Single) {
			_initCell();
		}
		if (m_cell.isNotNull()) {
			m_cell->multiLineMode = multiLineMode;
			invalidateLayoutOfWrappingControl(updateMode);
		}
	}

	sl_uint32 Button::getLineCount()
	{
		if (m_cell.isNotNull()) {
			return m_cell->lineCount;
		}
		return 1;
	}

	void Button::setLineCount(sl_uint32 nLines, UIUpdateMode updateMode)
	{
		_initCell();
		if (m_cell.isNotNull()) {
			m_cell->lineCount = nLines;
			invalidateLayoutOfWrappingControl(updateMode);
		}
	}

	sl_bool Button::isMnemonic()
	{
		if (m_cell.isNotNull()) {
			return m_cell->flagMnemonic;
		}
		return sl_true;
	}

	void Button::setMnemonic(sl_bool flag)
	{
		if (!flag) {
			_initCell();
		}
		if (m_cell.isNotNull()) {
			m_cell->flagMnemonic = flag;
		}
	}

	Alignment Button::getGravity()
	{
		_initCell();
		if (m_cell.isNotNull()) {
			return m_cell->gravity;
		}
		return Alignment::Default;
	}

	void Button::setGravity(const Alignment& gravity, UIUpdateMode updateMode)
	{
		if (gravity != Alignment::Default) {
			_initCell();
		}
		if (m_cell.isNotNull()) {
			m_cell->gravity = gravity;
			invalidate(updateMode);
		}
	}

	EllipsizeMode Button::getEllipsize()
	{
		_initCell();
		if (m_cell.isNotNull()) {
			return m_cell->ellipsizeMode;
		}
		return EllipsizeMode::None;
	}

	void Button::setEllipsize(EllipsizeMode ellipsizeMode, UIUpdateMode updateMode)
	{
		if (ellipsizeMode != EllipsizeMode::None) {
			_initCell();
		}
		if (m_cell.isNotNull()) {
			m_cell->ellipsizeMode = ellipsizeMode;
			invalidate(updateMode);
		}
	}

	sl_bool Button::isDefaultButton()
	{
		return m_flagDefaultButton;
	}

	void Button::setDefaultButton(sl_bool flag, UIUpdateMode mode)
	{
		Ptr<IButtonInstance> instance = getButtonInstance();
		if (instance.isNotNull()) {
			SLIB_VIEW_RUN_ON_UI_THREAD(setDefaultButton, flag, mode)
		}
		m_flagDefaultButton = flag;
		if (m_cell.isNotNull()) {
			m_cell->category = flag ? 1 : 0;
		}
		if (instance.isNotNull()) {
			instance->setDefaultButton(this, flag);
		} else {
			invalidate(mode);
		}
	}

	void Button::setCategories(const Array<ButtonCategory>& categories)
	{
		m_categories = categories;
	}

	sl_uint32 Button::getCategoryCount()
	{
		if (m_cell.isNotNull()) {
			return (sl_uint32)(m_cell->categories.getCount());
		}
		return 2;
	}

	sl_uint32 Button::getCurrentCategory()
	{
		if (m_cell.isNotNull()) {
			return m_cell->category;
		}
		return 0;
	}

	void Button::setCurrentCategory(sl_uint32 n, UIUpdateMode mode)
	{
		_initCell();
		if (m_cell.isNotNull()) {
			if (n >= m_cell->categories.getCount()) {
				return;
			}
			m_cell->category = n;
			invalidate(mode);
		}
	}

	const UISize& Button::getIconSize()
	{
		if (m_cell.isNotNull()) {
			return m_cell->iconSize;
		}
		return UISize::zero();
	}

	void Button::setIconSize(const UISize& size, UIUpdateMode mode)
	{
		_initCell();
		if (m_cell.isNotNull()) {
			m_cell->iconSize = size;
			invalidateLayoutOfWrappingControl(mode);
		}
	}

	void Button::setIconSize(sl_ui_len width, sl_ui_len height, UIUpdateMode mode)
	{
		setIconSize(UISize(width, height), mode);
	}

	void Button::setIconSize(sl_ui_len size, UIUpdateMode mode)
	{
		setIconSize(UISize(size, size), mode);
	}

	sl_ui_len Button::getIconWidth()
	{
		if (m_cell.isNotNull()) {
			return m_cell->iconSize.x;
		}
		return 0;
	}

	void Button::setIconWidth(sl_ui_len width, UIUpdateMode mode)
	{
		_initCell();
		if (m_cell.isNotNull()) {
			m_cell->iconSize.x = width;
			invalidateLayoutOfWrappingControl(mode);
		}
	}

	sl_ui_len Button::getIconHeight()
	{
		if (m_cell.isNotNull()) {
			return m_cell->iconSize.y;
		}
		return 0;
	}

	void Button::setIconHeight(sl_ui_len height, UIUpdateMode mode)
	{
		_initCell();
		if (m_cell.isNotNull()) {
			m_cell->iconSize.y = height;
			invalidateLayoutOfWrappingControl(mode);
		}
	}

	Alignment Button::getIconAlignment()
	{
		if (m_cell.isNotNull()) {
			return m_cell->iconAlignment;
		}
		return Alignment::MiddleCenter;
	}

	void Button::setIconAlignment(const Alignment& align, UIUpdateMode mode)
	{
		_initCell();
		if (m_cell.isNotNull()) {
			m_cell->iconAlignment = align;
			invalidate(mode);
		}
	}

	Alignment Button::getTextAlignment()
	{
		if (m_cell.isNotNull()) {
			return m_cell->textAlignment;
		}
		return Alignment::MiddleCenter;
	}

	void Button::setTextAlignment(const Alignment& align, UIUpdateMode mode)
	{
		_initCell();
		if (m_cell.isNotNull()) {
			m_cell->textAlignment = align;
			invalidate(mode);
		}
	}

	sl_bool Button::isTextBeforeIcon()
	{
		if (m_cell.isNotNull()) {
			return m_cell->flagTextBeforeIcon;
		}
		return sl_false;
	}

	void Button::setTextBeforeIcon(sl_bool flag, UIUpdateMode mode)
	{
		_initCell();
		if (m_cell.isNotNull()) {
			m_cell->flagTextBeforeIcon = flag;
			invalidate(mode);
		}
	}

	sl_bool Button::isExtendTextFrame()
	{
		if (m_cell.isNotNull()) {
			return m_cell->flagExtendTextFrame;
		}
		return sl_false;
	}

	void Button::setExtendTextFrame(sl_bool flag, UIUpdateMode mode)
	{
		_initCell();
		if (m_cell.isNotNull()) {
			m_cell->flagExtendTextFrame = flag;
			invalidateLayoutOfWrappingControl(mode);
		}
	}

	LayoutOrientation Button::getLayoutOrientation()
	{
		if (m_cell.isNotNull()) {
			return m_cell->layoutOrientation;
		}
		return LayoutOrientation::Horizontal;
	}

	void Button::setLayoutOrientation(LayoutOrientation orientation, UIUpdateMode mode)
	{
		_initCell();
		if (m_cell.isNotNull()) {
			m_cell->layoutOrientation = orientation;
			invalidateLayoutOfWrappingControl(mode);
		}
	}

	void Button::setIconMargin(sl_ui_pos left, sl_ui_pos top, sl_ui_pos right, sl_ui_pos bottom, UIUpdateMode mode)
	{
		_initCell();
		if (m_cell.isNotNull()) {
			m_cell->iconMarginLeft = left;
			m_cell->iconMarginTop = top;
			m_cell->iconMarginRight = right;
			m_cell->iconMarginBottom = bottom;
			invalidateLayoutOfWrappingControl(mode);
		}
	}

	void Button::setIconMargin(sl_ui_pos margin, UIUpdateMode mode)
	{
		setIconMargin(margin, margin, margin, margin, mode);
	}

	sl_ui_pos Button::getIconMarginLeft()
	{
		_initCell();
		if (m_cell.isNotNull()) {
			return m_cell->iconMarginLeft;
		}
		return 0;
	}

	void Button::setIconMarginLeft(sl_ui_pos margin, UIUpdateMode mode)
	{
		_initCell();
		if (m_cell.isNotNull()) {
			m_cell->iconMarginLeft = margin;
			invalidateLayoutOfWrappingControl(mode);
		}
	}

	sl_ui_pos Button::getIconMarginTop()
	{
		_initCell();
		if (m_cell.isNotNull()) {
			return m_cell->iconMarginTop;
		}
		return 0;
	}

	void Button::setIconMarginTop(sl_ui_pos margin, UIUpdateMode mode)
	{
		_initCell();
		if (m_cell.isNotNull()) {
			m_cell->iconMarginTop = margin;
			invalidateLayoutOfWrappingControl(mode);
		}
	}

	sl_ui_pos Button::getIconMarginRight()
	{
		_initCell();
		if (m_cell.isNotNull()) {
			return m_cell->iconMarginRight;
		}
		return 0;
	}

	void Button::setIconMarginRight(sl_ui_pos margin, UIUpdateMode mode)
	{
		_initCell();
		if (m_cell.isNotNull()) {
			m_cell->iconMarginRight = margin;
			invalidateLayoutOfWrappingControl(mode);
		}
	}

	sl_ui_pos Button::getIconMarginBottom()
	{
		_initCell();
		if (m_cell.isNotNull()) {
			return m_cell->iconMarginBottom;
		}
		return 0;
	}

	void Button::setIconMarginBottom(sl_ui_pos margin, UIUpdateMode mode)
	{
		_initCell();
		if (m_cell.isNotNull()) {
			m_cell->iconMarginBottom = margin;
			invalidateLayoutOfWrappingControl(mode);
		}
	}

	void Button::setTextMargin(sl_ui_pos left, sl_ui_pos top, sl_ui_pos right, sl_ui_pos bottom, UIUpdateMode mode)
	{
		_initCell();
		if (m_cell.isNotNull()) {
			m_cell->textMarginLeft = left;
			m_cell->textMarginTop = top;
			m_cell->textMarginRight = right;
			m_cell->textMarginBottom = bottom;
			invalidateLayoutOfWrappingControl(mode);
		}
	}

	void Button::setTextMargin(sl_ui_pos margin, UIUpdateMode mode)
	{
		setTextMargin(margin, margin, margin, margin, mode);
	}

	sl_ui_pos Button::getTextMarginLeft()
	{
		_initCell();
		if (m_cell.isNotNull()) {
			return m_cell->textMarginLeft;
		}
		return 0;
	}

	void Button::setTextMarginLeft(sl_ui_pos margin, UIUpdateMode mode)
	{
		_initCell();
		if (m_cell.isNotNull()) {
			m_cell->textMarginLeft = margin;
			invalidateLayoutOfWrappingControl(mode);
		}
	}

	sl_ui_pos Button::getTextMarginTop()
	{
		_initCell();
		if (m_cell.isNotNull()) {
			return m_cell->textMarginTop;
		}
		return 0;
	}

	void Button::setTextMarginTop(sl_ui_pos margin, UIUpdateMode mode)
	{
		_initCell();
		if (m_cell.isNotNull()) {
			m_cell->textMarginTop = margin;
			invalidateLayoutOfWrappingControl(mode);
		}
	}

	sl_ui_pos Button::getTextMarginRight()
	{
		_initCell();
		if (m_cell.isNotNull()) {
			return m_cell->textMarginRight;
		}
		return 0;
	}

	void Button::setTextMarginRight(sl_ui_pos margin, UIUpdateMode mode)
	{
		_initCell();
		if (m_cell.isNotNull()) {
			m_cell->textMarginRight = margin;
			invalidateLayoutOfWrappingControl(mode);
		}
	}

	sl_ui_pos Button::getTextMarginBottom()
	{
		_initCell();
		if (m_cell.isNotNull()) {
			return m_cell->textMarginBottom;
		}
		return 0;
	}

	void Button::setTextMarginBottom(sl_ui_pos margin, UIUpdateMode mode)
	{
		_initCell();
		if (m_cell.isNotNull()) {
			m_cell->textMarginBottom = margin;
			invalidateLayoutOfWrappingControl(mode);
		}
	}

#define DEFINE_STATE_MAP_FUNCS_SUB(FUNC, NAME, TYPE, CHECK_NOT_NULL, NULL_VALUE, SET_TYPE, SET_CHECK_NOT_NULL, SET_VALUE) \
	TYPE Button::get##FUNC(sl_uint32 category, ViewState state) \
	{ \
		if (m_cell.isNotNull()) { \
			if (category < m_cell->categories.getCount()) { \
				TYPE value = m_cell->categories[category].NAME.get(state); \
				if (value.CHECK_NOT_NULL()) { \
					return value; \
				} \
			} \
		} \
		return NULL_VALUE; \
	} \
	void Button::set##FUNC(sl_uint32 category, SET_TYPE value, ViewState state, UIUpdateMode mode) \
	{ \
		_initCell(); \
		if (m_cell.isNotNull()) { \
			if (category < m_cell->categories.getCount()) { \
				if (value SET_CHECK_NOT_NULL) { \
					m_cell->categories[category].NAME.set(state, SET_VALUE); \
				} else { \
					m_cell->categories[category].NAME.remove(state); \
				} \
				invalidate(mode); \
			} \
		} \
	} \
	void Button::set##FUNC(sl_uint32 category, SET_TYPE value, UIUpdateMode mode) \
	{ \
		set##FUNC(category, value, ViewState::Default, mode); \
	}

#define DEFINE_STATE_MAP_FUNCS(FUNC, NAME, TYPE, CHECK_NOT_NULL, NULL_VALUE, SET_TYPE, SET_CHECK_NOT_NULL, SET_VALUE) \
	DEFINE_STATE_MAP_FUNCS_SUB(FUNC, NAME, TYPE, CHECK_NOT_NULL, NULL_VALUE, SET_TYPE, SET_CHECK_NOT_NULL, SET_VALUE) \
	TYPE Button::get##FUNC(ViewState state) \
	{ \
		return get##FUNC(0, state); \
	} \
	void Button::set##FUNC(SET_TYPE value, ViewState state, UIUpdateMode mode) \
	{ \
		sl_uint32 nCategories = getCategoryCount(); \
		for (sl_uint32 i = 0; i < nCategories; i++) { \
			set##FUNC(i, value, state, mode); \
		} \
	} \
	void Button::set##FUNC(SET_TYPE value, UIUpdateMode mode) \
	{ \
		set##FUNC(value, ViewState::Default, mode); \
	}

	DEFINE_STATE_MAP_FUNCS(TextColor, textColors, Color, isNotZero, Color::zero(), const Color&, .isNotZero(), value)
	DEFINE_STATE_MAP_FUNCS(Icon, icons, Ref<Drawable>, isNotNull, sl_null, const Ref<Drawable>&, .isNotNull(), value)
	DEFINE_STATE_MAP_FUNCS_SUB(Background, backgrounds, Ref<Drawable>, isNotNull, sl_null, const Ref<Drawable>&, .isNotNull(), value)

	Color Button::getBackgroundColor(sl_uint32 category, ViewState state)
	{
		Color color;
		if (ColorDrawable::check(getBackground(category, state), &color)) {
			return color;
		}
		return Color::zero();
	}

	void Button::setBackgroundColor(sl_uint32 category, const Color& color, ViewState state, UIUpdateMode mode)
	{
		setBackground(category, Drawable::fromColor(color), state, mode);
	}

	void Button::setBackgroundColor(sl_uint32 category, const Color& color, UIUpdateMode mode)
	{
		setBackground(category, Drawable::fromColor(color), mode);
	}

	DEFINE_STATE_MAP_FUNCS_SUB(Border, borders, Ref<Pen>, isNotNull, sl_null, const Ref<Pen>&, .isNotNull(), value)

	void Button::setBorder(sl_uint32 category, const PenDesc& desc, ViewState state, UIUpdateMode mode)
	{
		setBorder(category, Pen::create(desc, getBorder(category, state)), state, mode);
	}

	void Button::setBorder(sl_uint32 category, const PenDesc& desc, UIUpdateMode mode)
	{
		setBorder(category, desc, ViewState::Default, mode);
	}

	DEFINE_STATE_MAP_FUNCS(ColorFilter, filters, Shared<ColorMatrix>, isNotNull, sl_null, ColorMatrix*, , Shared<ColorMatrix>::create(*value))

	void Button::setColorOverlay(sl_uint32 category, const Color& color, ViewState state, UIUpdateMode mode)
	{
		if (color.isZero()) {
			setColorFilter(category, sl_null, state, mode);
		} else {
			ColorMatrix cm;
			cm.setOverlay(color);
			setColorFilter(category, &cm, state, mode);
		}
	}

	void Button::setColorOverlay(sl_uint32 category, const Color& color, UIUpdateMode mode)
	{
		setColorOverlay(category, color, ViewState::Default, mode);
	}

	void Button::setColorOverlay(const Color& color, ViewState state, UIUpdateMode mode)
	{
		sl_uint32 nCategories = getCategoryCount();
		for (sl_uint32 i = 0; i < nCategories; i++) {
			setColorOverlay(i, color, state, mode);
		}
	}

	void Button::setColorOverlay(const Color& color, UIUpdateMode mode)
	{
		setColorOverlay(color, ViewState::Default, mode);
	}

	sl_bool Button::isUsingDefaultColorFilter()
	{
		if (m_cell.isNotNull()) {
			return m_cell->flagUseDefaultColorFilter;
		}
		return sl_true;
	}

	void Button::setUsingDefaultColorFilter(sl_bool flag, UIUpdateMode mode)
	{
		_initCell();
		if (m_cell.isNotNull()) {
			m_cell->flagUseDefaultColorFilter = flag;
			invalidate(mode);
		}
	}

	Ref<Drawable> Button::getCurrentBackground()
	{
		ViewState state = getState();
		if (m_cell.isNotNull()) {
			return m_cell->getFinalBackground(state);
		} else {
			return View::getCurrentBackground();
		}
	}

	Ref<Pen> Button::getCurrentBorder()
	{
		ViewState state = getState();
		if (m_cell.isNotNull()) {
			return m_cell->getFinalBorder(state);
		} else {
			return View::getCurrentBorder();
		}
	}

	void Button::prepareButtonCellLayout(ButtonCell* cell)
	{
		cell->flagWrapping = isWidthWrapping();
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

	void Button::dispatchDraw(Canvas* canvas)
	{
		ButtonCell* cell = m_cell.get();
		if (cell) {
			if (isLayer()) {
				cell->shadowOpacity = 0;
			} else {
				sl_real shadowOpacity = (sl_real)(getShadowOpacity());
				cell->shadowOpacity = shadowOpacity;
				if (shadowOpacity > 0) {
					cell->shadowRadius = getShadowRadius();
					cell->shadowColor = getShadowColor();
					cell->shadowOffset = getShadowOffset();
				}
			}
			prepareButtonCellLayout(cell);
		}
		View::dispatchDraw(canvas);
	}

	void Button::onDraw(Canvas* canvas)
	{
		if (m_cell.isNotNull()) {
			m_cell->onDrawContent(canvas);
		}
	}

	void Button::onKeyEvent(UIEvent* ev)
	{
		switch (ev->getKeycode()) {
			case Keycode::Enter:
			case Keycode::NumpadEnter:
#if !defined(SLIB_PLATFORM_IS_WIN32)
				if (isNativeWidget()) {
					return;
				}
#endif
				break;
			case Keycode::Space:
				if (isNativeWidget()) {
					return;
				}
				break;
			default:
				break;
		}
		if (m_cell.isNotNull()) {
			m_cell->onKeyEvent(ev);
		}
	}

	void Button::onMnemonic(UIEvent* ev)
	{
		setFocus();
		sl_bool flag = ev->isInternal();
		ev->setInternal(sl_true);
		dispatchClickEvent(ev);
		ev->setInternal(flag);
		ev->stopPropagation();
		ev->preventDefault();
	}

	void Button::onChangeFocus(sl_bool flagFocused)
	{
		if (m_cell.isNotNull()) {
			m_cell->setFocused(flagFocused);
			invalidate();
		}
	}

	void Button::onUpdateLayout()
	{
		sl_bool flagHorizontalWrapping = isWidthWrapping();
		sl_bool flagVerticalWrapping = isHeightWrapping();

		if (!flagHorizontalWrapping && !flagVerticalWrapping) {
			return;
		}

		Ptr<IButtonInstance> instance = getButtonInstance();
		if (instance.isNotNull()) {
			UISize size;
			if (instance->measureSize(this, size)) {
				if (flagHorizontalWrapping) {
					setLayoutWidth(size.x);
				}
				if (flagVerticalWrapping) {
					setLayoutHeight(size.y);
				}
				return;
			}
		}

		if (m_cell.isNull()) {
			if (isCreatingNativeWidget()) {
				TextBox box;
				TextBoxParam param;
				param.font = getFont();
				param.text = m_text;
				param.multiLineMode = MultiLineMode::Single;
				param.flagMnemonic = sl_true;
				box.update(param);
				if (flagHorizontalWrapping) {
					setLayoutWidth((sl_ui_len)(box.getContentWidth()));
				}
				if (flagVerticalWrapping) {
					setLayoutHeight((sl_ui_len)(box.getContentHeight()));
				}
				return;
			} else {
				_initCell();
			}
		}
		ButtonCell* cell = m_cell.get();
		if (cell) {
			prepareButtonCellLayout(cell);
			updateLayoutByViewCell(cell);
		}
	}

	Ref<ButtonCell> Button::createButtonCell()
	{
		if (m_categories.isNotNull()) {
			return new ButtonCell(m_categories);
		} else {
			return new ButtonCell();
		}
	}

	void Button::_initCell()
	{
		if (m_cell.isNotNull()) {
			return;
		}
		ObjectLocker lock(this);
		if (m_cell.isNotNull()) {
			return;
		}
		Ref<ButtonCell> cell = createButtonCell();
		if (cell.isNotNull()) {
			cell->setView(this, sl_true);
			cell->text = m_text;
			cell->category = m_flagDefaultButton ? 1 : 0;
			cell->onClick = SLIB_FUNCTION_WEAKREF(this, dispatchClickEvent);
			m_cell = cell;
		}
	}


#if !HAS_NATIVE_WIDGET_IMPL
	Ref<ViewInstance> Button::createNativeWidget(ViewInstance* parent)
	{
		return sl_null;
	}

	Ptr<IButtonInstance> Button::getButtonInstance()
	{
		return sl_null;
	}
#endif


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

		static const sl_real g_colorMatrix_disabled_buf[20] = {
			0.2f, 0.2f, 0.2f, 0,
			0.2f, 0.2f, 0.2f, 0,
			0.2f, 0.2f, 0.2f, 0,
			0, 0, 0, 1,
			0, 0, 0, 0
		};
		static const ColorMatrix& g_colorMatrix_disabled = *((const ColorMatrix*)((void*)g_colorMatrix_disabled_buf));

		static const sl_real g_colorMatrix_focused_buf[20] = {
			0.5f, 0, 0, 0,
			0, 0.5f, 0, 0,
			0, 0, 0.5f, 0,
			0, 0, 0, 1,
			0.2f, 0.3f, 0.6f, 0
		};
		static const ColorMatrix& g_colorMatrix_focused = *((const ColorMatrix*)((void*)g_colorMatrix_focused_buf));

		static const sl_real g_colorMatrix_focused_hover_buf[20] = {
			0.5f, 0, 0, 0,
			0, 0.5f, 0, 0,
			0, 0, 0.5f, 0,
			0, 0, 0, 1,
			0.2f, 0.4f, 0.6f, 0
		};
		static const ColorMatrix& g_colorMatrix_focused_hover = *((const ColorMatrix*)((void*)g_colorMatrix_focused_hover_buf));

		class Categories
		{
		public:
			Ref<Pen> defaultButtonPen;

		public:
			Categories()
			{
				defaultButtonPen = Pen::create(PenStyle::Solid, 3, Color(0, 100, 250));
			}

		public:
			static Array<ButtonCategory> createDefault()
			{
				SLIB_SAFE_LOCAL_STATIC(Categories, context)
				if (SLIB_SAFE_STATIC_CHECK_FREED(context)) {
					return sl_null;
				}
				Array<ButtonCategory> ret = Array<ButtonCategory>::create(2);
				if (ret.isNotNull()) {
					ButtonCategory* c = ret.getData();
					c[1].borders.defaultValue = context.defaultButtonPen;
					return ret;
				}
				return sl_null;
			}
		};

	}

	SLIB_DEFINE_OBJECT(ButtonCell, LabelViewCell)

	ButtonCell::ButtonCell(): ButtonCell(Categories::createDefault())
	{
	}

	ButtonCell::ButtonCell(const Array<ButtonCategory>& _categories)
	{
		categories = _categories;

		gravity = Alignment::Default;

		category = 0;

		iconSize.x = 0;
		iconSize.y = 0;
		iconAlignment = Alignment::MiddleCenter;
		textAlignment = Alignment::MiddleCenter;
		flagTextBeforeIcon = sl_false;
		flagExtendTextFrame = sl_false;
		layoutOrientation = LayoutOrientation::Horizontal;

		iconMarginLeft = 1;
		iconMarginTop = 1;
		iconMarginRight = 1;
		iconMarginBottom = 1;

		textMarginLeft = 1;
		textMarginTop = 1;
		textMarginRight = 1;
		textMarginBottom = 1;

		flagUseDefaultColorFilter = sl_true;

	}

	ButtonCell::~ButtonCell()
	{
	}

	Ref<Drawable> ButtonCell::getFinalBackground(ViewState state)
	{
		sl_bool flagUseDefaultBackground;
		Ref<Drawable> background = categories[category].backgrounds.evaluate(state, &flagUseDefaultBackground);
		if (background.isNull()) {
			Ref<View> view = m_view;
			if (view.isNull()) {
				return sl_null;
			}
			background = view->getFinalBackground(state, &flagUseDefaultBackground);
			if (background.isNull()) {
				return sl_null;
			}
		}
		ColorMatrix cm;
		if (getFinalColorFilter(cm, state, flagUseDefaultBackground && flagUseDefaultColorFilter)) {
			background = background->filter(cm);
		}
		return background;
	}

	Ref<Pen> ButtonCell::getFinalBorder(ViewState state)
	{
		Ref<Pen> border = categories[category].borders.evaluate(state);
		if (border.isNotNull()) {
			return border;
		}
		Ref<View> view = m_view;
		if (view.isNotNull()) {
			return view->getFinalBorder(state);
		} else {
			return sl_null;
		}
	}

	Color ButtonCell::getFinalTextColor(ViewState state)
	{
		sl_bool flagUseDefaultColor;
		Color color = categories[category].textColors.evaluate(state, &flagUseDefaultColor);
		if (color.isZero()) {
			flagUseDefaultColor = sl_true;
			color = BUTTON_TEXT_DEFAULT_COLOR;
		}
		ColorMatrix cm;
		if (getFinalColorFilter(cm, state, flagUseDefaultColor && flagUseDefaultColorFilter)) {
			color = cm.transformColor(color);
		}
		return color;
	}

	Ref<Drawable> ButtonCell::getFinalIcon(ViewState state)
	{
		sl_bool flagUseDefaultIcon;
		Ref<Drawable> icon = categories[category].icons.evaluate(state, &flagUseDefaultIcon);
		if (icon.isNull()) {
			return sl_null;
		}
		ColorMatrix cm;
		if (getFinalColorFilter(cm, state, flagUseDefaultIcon && flagUseDefaultColorFilter)) {
			icon = icon->filter(cm);
		}
		return icon;
	}

	sl_bool ButtonCell::getFinalColorFilter(ColorMatrix& _out, ViewState state, sl_bool flagUseDefaultFilter)
	{
		Shared<ColorMatrix> cm = categories[category].filters.evaluate(state);
		if (cm.isNotNull()) {
			_out = *cm;
			return sl_true;
		}
		if (category) {
			cm = categories[0].filters.evaluate(state);
			if (cm.isNotNull()) {
				_out = *cm;
				return sl_true;
			}
		}
		if (!flagUseDefaultFilter) {
			return sl_false;
		}
		switch (state) {
			case ViewState::Hover:
				_out = g_colorMatrix_hover;
				break;
			case ViewState::Pressed:
				_out = g_colorMatrix_pressed;
				break;
			case ViewState::FocusedNormal:
				return &g_colorMatrix_focused;
			case ViewState::FocusedHover:
				return &g_colorMatrix_focused_hover;
			case ViewState::FocusedPressed:
				return &g_colorMatrix_pressed;
			case ViewState::Disabled:
				_out = g_colorMatrix_disabled;
				break;
			default:
				return sl_false;
		}
		return sl_true;
	}

	UISize ButtonCell::measureContentSize(sl_ui_len widthFrame, sl_ui_len heightFrame)
	{
		UISize size;
		layoutIconAndText(widthFrame, heightFrame, size, sl_null, sl_null);
		if (size.x < 0) {
			size.x = 0;
		}
		if (size.y < 0) {
			size.y = 0;
		}
		return size;
	}

	void ButtonCell::layoutIconAndText(sl_ui_len widthFrame, sl_ui_len heightFrame, UISize& sizeContent, UIRect* pOutFrameIcon, UIRect* pOutFrameText)
	{
		sl_uint32 nCategories = (sl_uint32)(categories.getCount());
		ButtonCategory* categories = this->categories.getData();

		sl_bool flagUseText = String(text).isNotEmpty();

		sl_ui_pos widthIcon = iconSize.x;
		sl_ui_pos heightIcon = iconSize.y;
		if (widthIcon < 0) {
			widthIcon = 0;
		}
		if (heightIcon < 0) {
			heightIcon = 0;
		}

		sl_bool flagUseIcon = widthIcon > 0 || heightIcon > 0;
		if (!flagUseIcon) {
			for (sl_uint32 i = 0; i < nCategories; i++) {
				ButtonCategory& props = categories[i];
				if (props.icons.defaultValue.isNotNull() || props.icons.values.isNotNull()) {
					flagUseIcon = sl_true;
					break;
				}
			}
		}

		if (flagUseIcon) {
			if (widthIcon <= 0) {
				if (heightIcon > 0) {
					widthIcon = heightIcon;
				}
			} else {
				if (heightIcon <= 0) {
					heightIcon = widthIcon;
				}
			}
		}

		sl_ui_pos widthText = 0;
		sl_ui_pos heightText = 0;
		if (flagUseText) {
			sl_ui_len widthTextLayout = widthFrame;
			sl_bool flagWrapping = sl_false;
			if (widthFrame <= 0) {
				flagWrapping = sl_true;
				widthFrame = 0;
			}
			if (flagWrapping && maxWidth) {
				flagWrapping = sl_false;
				widthTextLayout = maxWidth;
			}
			if (!flagWrapping && flagUseIcon && layoutOrientation == LayoutOrientation::Horizontal) {
				if (widthIcon <= 0) {
					Ref<Font> font = getFont();
					if (font.isNotNull()) {
						widthIcon = (sl_ui_len)(font->getFontHeight());
					} else {
						widthIcon = 20;
					}
					heightIcon = widthIcon;
				}
				widthTextLayout -= widthIcon + iconMarginLeft + iconMarginRight;
			}
			if (!flagWrapping) {
				if (widthTextLayout < 1) {
					widthTextLayout = 1;
				}
			}
			_updateTextBox(flagWrapping, widthTextLayout, textMarginLeft + textMarginRight, textAlignment);
			if (flagWrapping || !flagExtendTextFrame) {
				widthText = (sl_ui_len)(m_textBox.getContentWidth()) + textMarginLeft + textMarginRight;
			} else {
				widthText = widthTextLayout;
			}
			if (widthText < 0) {
				widthText = 0;
			}
			heightText = (sl_ui_len)(m_textBox.getContentHeight()) + textMarginTop + textMarginBottom;
			if (heightText < 0) {
				heightText = 0;
			}
		}

		if (flagUseIcon) {
			sl_ui_len marginWidth = iconMarginLeft + iconMarginRight;
			sl_ui_len marginHeight = iconMarginTop + iconMarginBottom;
			if (widthIcon <= 0 && heightIcon <= 0) {
				if (flagUseText) {
					sl_ui_len defaultHeight = heightText;
					if (defaultHeight <= 0) {
						Ref<Font> font = getFont();
						if (font.isNotNull()) {
							defaultHeight = (sl_ui_len)(font->getFontHeight());
						} else {
							defaultHeight = 20;
						}
					}
					defaultHeight = (sl_ui_len)(defaultHeight * 0.9f);
					widthIcon = defaultHeight;
					heightIcon = defaultHeight;
				} else {
					if (widthFrame <= 0) {
						if (heightFrame <= 0) {
							Ref<Font> font = getFont();
							if (font.isNotNull()) {
								widthIcon = (sl_ui_len)(font->getFontHeight());
							} else {
								widthIcon = 20;
							}
							heightIcon = widthIcon;
						} else {
							widthIcon = heightFrame;
							heightIcon = heightFrame;
						}
					} else {
						if (heightFrame <= 0) {
							widthIcon = widthFrame;
							heightIcon = widthFrame;
						} else {
							widthIcon = widthFrame;
							heightIcon = heightFrame;
						}
					}
				}
				widthIcon -= marginWidth;
				heightIcon -= marginHeight;
				widthIcon = Math::min(widthIcon, heightIcon);
				heightIcon = widthIcon;
			}
			widthIcon += marginWidth;
			if (widthIcon < 0) {
				widthIcon = 0;
			}
			heightIcon += marginHeight;
			if (heightIcon < 0) {
				heightIcon = 0;
			}
		}

		sl_ui_pos widthContent = 0;
		sl_ui_pos heightContent = 0;
		if (layoutOrientation == LayoutOrientation::Horizontal) {
			widthContent = widthIcon + widthText;
			heightContent = Math::max(heightIcon, heightText);
		} else {
			widthContent = Math::max(widthIcon, widthText);
			heightContent = heightIcon + heightText;
		}
		if (widthContent < 0) {
			widthContent = 0;
		}
		if (heightContent < 0) {
			heightContent = 0;
		}

		if (widthFrame <= 0 || !flagExtendTextFrame) {
			widthFrame = widthContent;
		}
		if (heightFrame <= 0 || !flagExtendTextFrame) {
			heightFrame = heightContent;
		}

		if (pOutFrameIcon || pOutFrameText) {
			UIRect rcIconExtend;
			UIRect rcTextExtend;
			if (layoutOrientation == LayoutOrientation::Horizontal) {
				rcIconExtend.top = 0;
				rcIconExtend.bottom = heightFrame;
				rcTextExtend.top = 0;
				rcTextExtend.bottom = heightFrame;
				if (flagTextBeforeIcon) {
					rcIconExtend.left = widthFrame - widthIcon;
					rcIconExtend.right = widthFrame;
					rcTextExtend.left = 0;
					rcTextExtend.right = rcIconExtend.left;
				} else {
					rcIconExtend.left = 0;
					rcIconExtend.right = widthIcon;
					rcTextExtend.left = rcIconExtend.right;
					rcTextExtend.right = widthFrame;
				}
			} else {
				rcIconExtend.left = 0;
				rcIconExtend.right = widthFrame;
				rcTextExtend.left = 0;
				rcTextExtend.right = widthFrame;
				if (flagTextBeforeIcon) {
					rcIconExtend.top = heightFrame - heightIcon;
					rcIconExtend.bottom = heightFrame;
					rcTextExtend.top = 0;
					rcTextExtend.bottom = rcIconExtend.top;
				} else {
					rcIconExtend.top = 0;
					rcIconExtend.bottom = heightIcon;
					rcTextExtend.top = rcIconExtend.bottom;
					rcTextExtend.bottom = heightFrame;
				}
			}

			if (pOutFrameIcon) {
				UIRect& frameIcon = *pOutFrameIcon;
				frameIcon.setLeftTop(GraphicsUtil::calculateAlignPosition(rcIconExtend, (sl_real)widthIcon, (sl_real)heightIcon, iconAlignment));
				frameIcon.right = frameIcon.left + widthIcon - iconMarginRight;
				frameIcon.bottom = frameIcon.top + heightIcon - iconMarginBottom;
				frameIcon.left += iconMarginLeft;
				frameIcon.top += iconMarginTop;
				frameIcon.fixSizeError();
			}
			if (pOutFrameText) {
				UIRect& frameText = *pOutFrameText;
				frameText.setLeftTop(GraphicsUtil::calculateAlignPosition(rcTextExtend, (sl_real)widthText, (sl_real)heightText, textAlignment));
				frameText.right = frameText.left + widthText - textMarginRight;
				frameText.bottom = frameText.top + heightText - textMarginBottom;
				frameText.left += textMarginLeft;
				frameText.top += textMarginTop;
				frameText.fixSizeError();
			}
		}

		sizeContent.x = widthContent;
		sizeContent.y = heightContent;

	}

	void ButtonCell::onDraw(Canvas* canvas)
	{
		ViewState state = getState();
		UIRect frame = getFrame();
		Ref<Drawable> background = getFinalBackground(state);
		if (background.isNotNull()) {
			canvas->draw(frame, background);
		}
		onDrawContent(canvas);
		Ref<Pen> border = getFinalBorder(state);
		if (border.isNotNull()) {
			sl_bool flagAntiAlias = canvas->isAntiAlias();
			canvas->setAntiAlias(sl_false);
			canvas->drawRectangle(frame, border);
			canvas->setAntiAlias(flagAntiAlias);
		}
	}

	void ButtonCell::onDrawContent(Canvas* canvas)
	{
		ButtonCategory& props = categories[category];
		ViewState state = getState();
		sl_bool flagText = text.isNotNull();

		Color color;
		if (flagText) {
			getFinalTextColor(state);
		}
		Ref<Drawable> icon = getFinalIcon(state);
		if (!flagText && icon.isNull()) {
			return;
		}

		UIRect bound = getFrame();
		sl_ui_pos widthFrame = bound.getWidth();
		sl_ui_pos heightFrame = bound.getHeight();
		if (widthFrame <= 0 || heightFrame <= 0) {
			return;
		}

		UIRect rcIcon, rcText;
		UISize sizeContent;
		layoutIconAndText(widthFrame, heightFrame, sizeContent, &rcIcon, &rcText);
		UIPoint pt = GraphicsUtil::calculateAlignPosition(bound, (sl_real)(sizeContent.x), (sl_real)(sizeContent.y), gravity);

		if (icon.isNotNull() && rcIcon.getWidth() > 0 && rcIcon.getHeight() > 0) {
			rcIcon.left += pt.x;
			rcIcon.top += pt.y;
			rcIcon.right += pt.x;
			rcIcon.bottom += pt.y;
			if (iconSize.x > 0 && iconSize.y > 0) {
				canvas->draw(rcIcon, icon);
			} else {
				canvas->draw(rcIcon, icon, ScaleMode::Contain, Alignment::MiddleCenter);
			}
		}

		if (flagText && rcText.getWidth() > 0 && rcText.getHeight() > 0) {

			rcText.left += pt.x;
			rcText.top += pt.y;
			rcText.right += pt.x;
			rcText.bottom += pt.y;

			TextBox::DrawParam param;
			param.frame = rcText;
			param.textColor = color;
			if (shadowOpacity > 0) {
				param.shadowOpacity = shadowOpacity;
				param.shadowRadius = (sl_real)shadowRadius;
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
			m_textBox.draw(canvas, param);
		}
	}

	void ButtonCell::onKeyEvent(UIEvent* ev)
	{
		switch (ev->getKeycode()) {
			case Keycode::Enter:
			case Keycode::NumpadEnter:
				if (ev->getAction() == UIAction::KeyDown) {
					onClick(ev);
					ev->preventDefault();
					ev->stopPropagation();
				}
				break;
			case Keycode::Space:
				switch (ev->getAction()) {
				case UIAction::KeyDown:
					setPressedState();
					ev->preventDefault();
					ev->stopPropagation();
					break;
				case UIAction::KeyUp:
					if (isPressedState()) {
						setPressedState(sl_false);
						onClick(ev);
						ev->preventDefault();
						ev->stopPropagation();
					}
					break;
				default:
					break;
				}
				break;
			default:
				break;
		}
	}

	void ButtonCell::onMeasure(UISize& size, sl_bool flagHorizontalWrapping, sl_bool flagVerticalWrapping)
	{
		sl_ui_len width = 0;
		sl_ui_len height = 0;
		if (!flagHorizontalWrapping) {
			width = size.x;
			if (width < 1) {
				if (flagVerticalWrapping) {
					size.y = 0;
				}
				return;
			}
		}
		if (!flagVerticalWrapping) {
			height = size.y;
			if (height < 1) {
				if (flagHorizontalWrapping) {
					size.x = 0;
				}
				return;
			}
		}
		UISize sizeContent = measureContentSize(width, height);
		if (flagHorizontalWrapping) {
			size.x = sizeContent.x;
		}
		if (flagVerticalWrapping) {
			size.y = sizeContent.y;
		}
	}

}
