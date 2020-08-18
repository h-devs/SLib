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

#include "slib/ui/button.h"

#include "slib/ui/core.h"
#include "slib/graphics/util.h"
#include "slib/core/safe_static.h"
#include "slib/core/new_helper.h"

namespace slib
{

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(ButtonCategoryProperties)

	ButtonCategoryProperties::ButtonCategoryProperties()
	{
		textColor = Color::zero();
		flagFilter = sl_false;
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(ButtonCategory)

	ButtonCategory::ButtonCategory()
	{
	}


	namespace priv
	{
		namespace button
		{

			const sl_real g_colorMatrix_hover_buf[20] = {
				0.5f, 0, 0, 0,
				0, 0.5f, 0, 0,
				0, 0, 0.5f, 0,
				0, 0, 0, 1,
				0.2f, 0.3f, 0.4f, 0
			};
			const ColorMatrix& g_colorMatrix_hover = *((const ColorMatrix*)((void*)g_colorMatrix_hover_buf));

			const sl_real g_colorMatrix_focused_buf[20] = {
				0.5f, 0, 0, 0,
				0, 0.5f, 0, 0,
				0, 0, 0.5f, 0,
				0, 0, 0, 1,
				0.2f, 0.3f, 0.6f, 0
			};
			const ColorMatrix& g_colorMatrix_focused = *((const ColorMatrix*)((void*)g_colorMatrix_focused_buf));

			const sl_real g_colorMatrix_focused_hover_buf[20] = {
				0.5f, 0, 0, 0,
				0, 0.5f, 0, 0,
				0, 0, 0.5f, 0,
				0, 0, 0, 1,
				0.2f, 0.4f, 0.6f, 0
			};
			const ColorMatrix& g_colorMatrix_focused_hover = *((const ColorMatrix*)((void*)g_colorMatrix_focused_hover_buf));

			const sl_real g_colorMatrix_pressed_buf[20] = {
				0.5f, 0, 0, 0,
				0, 0.5f, 0, 0,
				0, 0, 0.5f, 0,
				0, 0, 0, 1,
				0.3f, 0.4f, 0.6f, 0

			};
			const ColorMatrix& g_colorMatrix_pressed = *((const ColorMatrix*)((void*)g_colorMatrix_pressed_buf));

			const sl_real g_colorMatrix_disabled_buf[20] = {
				0.2f, 0.2f, 0.2f, 0,
				0.2f, 0.2f, 0.2f, 0,
				0.2f, 0.2f, 0.2f, 0,
				0, 0, 0, 1,
				0, 0, 0, 0
			};
			const ColorMatrix& g_colorMatrix_disabled = *((const ColorMatrix*)((void*)g_colorMatrix_disabled_buf));

			class Categories
			{
			public:
				ButtonCategory categories[2];

			public:
				Categories()
				{
					categories[1].properties[(int)(ButtonState::Default)].border = Pen::create(PenStyle::Solid, 3, Color(0, 100, 250));
				}

			public:
				static ButtonCategory* getCategories()
				{
					SLIB_SAFE_LOCAL_STATIC(Categories, ret)
					if (SLIB_SAFE_STATIC_CHECK_FREED(ret)) {
						return sl_null;
					}
					return ret.categories;
				}
			};

		}
	}


	SLIB_DEFINE_OBJECT(Button, View)

	Button::Button() : Button(2)
	{
	}

	Button::Button(sl_uint32 nCategories, ButtonCategory* categories)
	{
#if !defined(SLIB_PLATFORM_IS_MOBILE)
		setFocusable(sl_true);
#endif
		m_flagDefaultButton = sl_false;

		m_state = ButtonState::Normal;
		m_category = 0;
#if defined(SLIB_PLATFORM_IS_MOBILE)
		m_flagUseFocusedState = sl_false;
#else
		m_flagUseFocusedState = sl_true;
#endif

		m_iconSize.x = 0;
		m_iconSize.y = 0;
		m_gravity = Alignment::MiddleCenter;
		m_iconAlignment = Alignment::MiddleCenter;
		m_textAlignment = Alignment::MiddleCenter;
		m_flagTextBeforeIcon = sl_false;
		m_flagExtendTextFrame = sl_false;
		m_layoutOrientation = LayoutOrientation::Horizontal;

		m_iconMarginLeft = 1;
		m_iconMarginTop = 1;
		m_iconMarginRight = 1;
		m_iconMarginBottom = 1;

		m_textMarginLeft = 1;
		m_textMarginTop = 1;
		m_textMarginRight = 1;
		m_textMarginBottom = 1;

		setPadding(1, 1, 1, 1, UIUpdateMode::Init);

		m_flagUseDefaultColorFilter = sl_true;

		if (nCategories == 0) {
			nCategories = 1;
		}
		m_nCategories = nCategories;
		m_categories = NewHelper<ButtonCategory>::create(nCategories);
		if (!categories) {
			categories = priv::button::Categories::getCategories();
			if (nCategories > 2) {
				nCategories = 2;
			}
			if (!categories) {
				nCategories = 1;
			}
		}
		if (categories) {
			for (sl_uint32 i = 0; i < nCategories; i++) {
				m_categories[i] = categories[i];
			}
		}

		setUsingFont(sl_true);

		m_textColor = Color(0, 100, 200);

	}

	Button::~Button()
	{
		NewHelper<ButtonCategory>::free(m_categories, m_nCategories);
	}

	void Button::setText(const String& text, UIUpdateMode mode)
	{
		Ptr<IButtonInstance> instance = getButtonInstance();
		if (instance.isNotNull()) {
			SLIB_VIEW_RUN_ON_UI_THREAD(&Button::setText, text, mode)
				LabelView::setText(text, UIUpdateMode::Init);
			instance->setText(this, text);
			if (!SLIB_UI_UPDATE_MODE_IS_UPDATE_LAYOUT(mode)) {
				return;
			}
			invalidateLayoutOfWrappingControl(mode);
		} else {
			LabelView::setText(text, mode);
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
			SLIB_VIEW_RUN_ON_UI_THREAD(&Button::setDefaultButton, flag, mode)
		}
		m_flagDefaultButton = flag;
		_invalidateButtonCategory(UIUpdateMode::None);
		if (instance.isNotNull()) {
			instance->setDefaultButton(this, flag);
		} else {
			invalidate(mode);
		}
	}

	sl_uint32 Button::getCategoriesCount()
	{
		return m_nCategories;
	}

	ButtonState Button::getButtonState()
	{
		return m_state;
	}

	sl_bool Button::isUsingFocusedState()
	{
		return m_flagUseFocusedState;
	}

	void Button::setUsingFocusedState(sl_bool flag)
	{
		m_flagUseFocusedState = flag;
	}

	sl_uint32 Button::getCurrentCategory()
	{
		return m_category;
	}

	void Button::setCurrentCategory(sl_uint32 n, UIUpdateMode mode)
	{
		if (n < m_nCategories) {
			m_category = n;
			invalidate(mode);
		}
	}

	const UISize& Button::getIconSize()
	{
		return m_iconSize;
	}

	void Button::setIconSize(const UISize& size, UIUpdateMode mode)
	{
		m_iconSize = size;
		invalidateLayoutOfWrappingControl(mode);
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
		return m_iconSize.x;
	}

	void Button::setIconWidth(sl_ui_len width, UIUpdateMode mode)
	{
		setIconSize(UISize(width, m_iconSize.y), mode);
	}

	sl_ui_len Button::getIconHeight()
	{
		return m_iconSize.y;
	}

	void Button::setIconHeight(sl_ui_len height, UIUpdateMode mode)
	{
		setIconSize(UISize(m_iconSize.x, height), mode);
	}

	Alignment Button::getIconAlignment()
	{
		return m_iconAlignment;
	}

	void Button::setIconAlignment(const Alignment& align, UIUpdateMode mode)
	{
		m_iconAlignment = align;
		invalidate(mode);
	}

	Alignment Button::getTextAlignment()
	{
		return m_textAlignment;
	}

	void Button::setTextAlignment(const Alignment& align, UIUpdateMode mode)
	{
		m_textAlignment = align;
		invalidate(mode);
	}

	sl_bool Button::isTextBeforeIcon()
	{
		return m_flagTextBeforeIcon;
	}

	void Button::setTextBeforeIcon(sl_bool flag, UIUpdateMode mode)
	{
		m_flagTextBeforeIcon = flag;
		invalidate(mode);
	}

	sl_bool Button::isExtendTextFrame()
	{
		return m_flagExtendTextFrame;
	}

	void Button::setExtendTextFrame(sl_bool flag, UIUpdateMode mode)
	{
		m_flagExtendTextFrame = flag;
		invalidateLayoutOfWrappingControl(mode);
	}

	LayoutOrientation Button::getLayoutOrientation()
	{
		return m_layoutOrientation;
	}

	void Button::setLayoutOrientation(LayoutOrientation orientation, UIUpdateMode mode)
	{
		m_layoutOrientation = orientation;
		invalidateLayoutOfWrappingControl(mode);
	}

	void Button::setIconMargin(sl_ui_pos left, sl_ui_pos top, sl_ui_pos right, sl_ui_pos bottom, UIUpdateMode mode)
	{
		m_iconMarginLeft = left;
		m_iconMarginTop = top;
		m_iconMarginRight = right;
		m_iconMarginBottom = bottom;
		invalidateLayoutOfWrappingControl(mode);
	}

	void Button::setIconMargin(sl_ui_pos margin, UIUpdateMode mode)
	{
		setIconMargin(margin, margin, margin, margin, mode);
	}

	sl_ui_pos Button::getIconMarginLeft()
	{
		return m_iconMarginLeft;
	}

	void Button::setIconMarginLeft(sl_ui_pos margin, UIUpdateMode mode)
	{
		setIconMargin(margin, m_iconMarginTop, m_iconMarginRight, m_iconMarginBottom, mode);
	}

	sl_ui_pos Button::getIconMarginTop()
	{
		return m_iconMarginTop;
	}

	void Button::setIconMarginTop(sl_ui_pos margin, UIUpdateMode mode)
	{
		setIconMargin(m_iconMarginLeft, margin, m_iconMarginRight, m_iconMarginBottom, mode);
	}

	sl_ui_pos Button::getIconMarginRight()
	{
		return m_iconMarginRight;
	}

	void Button::setIconMarginRight(sl_ui_pos margin, UIUpdateMode mode)
	{
		setIconMargin(m_iconMarginLeft, m_iconMarginTop, margin, m_iconMarginBottom, mode);
	}

	sl_ui_pos Button::getIconMarginBottom()
	{
		return m_iconMarginBottom;
	}

	void Button::setIconMarginBottom(sl_ui_pos margin, UIUpdateMode mode)
	{
		setIconMargin(m_iconMarginLeft, m_iconMarginTop, m_iconMarginRight, margin, mode);
	}

	void Button::setTextMargin(sl_ui_pos left, sl_ui_pos top, sl_ui_pos right, sl_ui_pos bottom, UIUpdateMode mode)
	{
		m_textMarginLeft = left;
		m_textMarginTop = top;
		m_textMarginRight = right;
		m_textMarginBottom = bottom;
		invalidateLayoutOfWrappingControl(mode);
	}

	void Button::setTextMargin(sl_ui_pos margin, UIUpdateMode mode)
	{
		setTextMargin(margin, margin, margin, margin, mode);
	}

	sl_ui_pos Button::getTextMarginLeft()
	{
		return m_textMarginLeft;
	}

	void Button::setTextMarginLeft(sl_ui_pos margin, UIUpdateMode mode)
	{
		setTextMargin(margin, m_textMarginTop, m_textMarginRight, m_textMarginBottom, mode);
	}

	sl_ui_pos Button::getTextMarginTop()
	{
		return m_textMarginTop;
	}

	void Button::setTextMarginTop(sl_ui_pos margin, UIUpdateMode mode)
	{
		setTextMargin(m_textMarginLeft, margin, m_textMarginRight, m_textMarginBottom, mode);
	}

	sl_ui_pos Button::getTextMarginRight()
	{
		return m_textMarginRight;
	}

	void Button::setTextMarginRight(sl_ui_pos margin, UIUpdateMode mode)
	{
		setTextMargin(m_textMarginLeft, m_textMarginTop, margin, m_textMarginBottom, mode);
	}

	sl_ui_pos Button::getTextMarginBottom()
	{
		return m_textMarginBottom;
	}

	void Button::setTextMarginBottom(sl_ui_pos margin, UIUpdateMode mode)
	{
		setTextMargin(m_textMarginLeft, m_textMarginTop, m_textMarginRight, margin, mode);
	}

	Color Button::getTextColor(ButtonState state, sl_uint32 category)
	{
		if (category < m_nCategories && (int)state < (int)(ButtonState::Count)) {
			return m_categories[category].properties[(int)state].textColor;
		} else {
			return Color::zero();
		}
	}

	void Button::setTextColor(const Color& color, ButtonState state, sl_uint32 category, UIUpdateMode mode)
	{
		if (category < m_nCategories) {
			if ((int)state < (int)(ButtonState::Count)) {
				m_categories[category].properties[(int)state].textColor = color;
				invalidate(mode);
			}
		}
	}

	Ref<Drawable> Button::getIcon(ButtonState state, sl_uint32 category)
	{
		if (category < m_nCategories && (int)state < (int)(ButtonState::Count)) {
			return m_categories[category].properties[(int)state].icon;
		} else {
			return sl_null;
		}
	}

	void Button::setIcon(const Ref<Drawable>& icon, ButtonState state, sl_uint32 category, UIUpdateMode mode)
	{
		if (category < m_nCategories) {
			if ((int)state < (int)(ButtonState::Count)) {
				m_categories[category].properties[(int)state].icon = icon;
				invalidateLayoutOfWrappingControl(mode);
			}
		}
	}

	Ref<Drawable> Button::getIcon()
	{
		return m_iconDefault;
	}

	void Button::setIcon(const Ref<Drawable>& icon, UIUpdateMode mode)
	{
		m_iconDefault = icon;
		invalidateLayoutOfWrappingControl(mode);
	}

	Ref<Drawable> Button::getBackground(ButtonState state, sl_uint32 category)
	{
		if (category < m_nCategories && (int)state < (int)(ButtonState::Count)) {
			return m_categories[category].properties[(int)state].background;
		} else {
			return sl_null;
		}
	}

	void Button::setBackground(const Ref<Drawable>& background, ButtonState state, sl_uint32 category, UIUpdateMode mode)
	{
		if (category < m_nCategories) {
			if ((int)state < (int)(ButtonState::Count)) {
				m_categories[category].properties[(int)state].background = background;
				invalidate(mode);
			}
		}
	}

	void Button::setBackground(const Color& color, ButtonState state, sl_uint32 category, UIUpdateMode mode)
	{
		setBackground(Drawable::createColorDrawable(color), state, category, mode);
	}

	Ref<Pen> Button::getBorder(ButtonState state, sl_uint32 category)
	{
		if (category < m_nCategories && (int)state < (int)(ButtonState::Count)) {
			return m_categories[category].properties[(int)state].border;
		} else {
			return sl_null;
		}
	}

	void Button::setBorder(const Ref<Pen>& pen, ButtonState state, sl_uint32 category, UIUpdateMode mode)
	{
		if (category < m_nCategories) {
			if ((int)state < (int)(ButtonState::Count)) {
				m_categories[category].properties[(int)state].border = pen;
				invalidate(mode);
			}
		}
	}

	ColorMatrix* Button::getColorFilter(ButtonState state, sl_uint32 category)
	{
		if (category < m_nCategories && (int)state < (int)(ButtonState::Count)) {
			ButtonCategoryProperties& props = m_categories[category].properties[(int)state];
			if (props.flagFilter) {
				return &(props.filter);
			}
		}
		return sl_null;
	}
	
	void Button::setColorFilter(ColorMatrix* filter, ButtonState state, sl_uint32 category, UIUpdateMode mode)
	{
		setUsingDefaultColorFilter(sl_false, UIUpdateMode::None);
		if (category < m_nCategories) {
			if ((int)state < (int)(ButtonState::Count)) {
				ButtonCategoryProperties& props = m_categories[category].properties[(int)state];
				if (filter) {
					props.flagFilter = sl_true;
					props.filter = *filter;
				} else {
					props.flagFilter = sl_false;
				}
				invalidate(mode);
			}
		}
	}
	
	ColorMatrix* Button::getColorFilter()
	{
		return getColorFilter(ButtonState::Default);
	}
	
	void Button::setColorFilter(ColorMatrix* filter, UIUpdateMode mode)
	{
		setColorFilter(filter, ButtonState::Default, 0, mode);
	}
	
	void Button::setColorOverlay(const Color& color, ButtonState state, sl_uint32 category, UIUpdateMode mode)
	{
		if (color.isZero()) {
			setColorFilter(sl_null, state, category, mode);
		} else {
			ColorMatrix cm;
			cm.setOverlay(color);
			setColorFilter(&cm, state, category, mode);
		}
	}
	
	void Button::setColorOverlay(const Color& color, UIUpdateMode mode)
	{
		setColorOverlay(color, ButtonState::Normal, 0, mode);
	}
	
	sl_bool Button::isUsingDefaultColorFilter()
	{
		return m_flagUseDefaultColorFilter;
	}

	void Button::setUsingDefaultColorFilter(sl_bool flag, UIUpdateMode mode)
	{
		m_flagUseDefaultColorFilter = flag;
		invalidate(mode);
	}

	void Button::setEnabled(sl_bool flagEnabled, UIUpdateMode mode)
	{
		if (isEnabled() != flagEnabled) {
			View::setEnabled(flagEnabled, UIUpdateMode::None);
			_invalidateButtonState(mode);
		}
	}

	void Button::setPressedState(sl_bool flagState, UIUpdateMode mode)
	{
		if (isPressedState() != flagState) {
			View::setPressedState(flagState, UIUpdateMode::None);
			_invalidateButtonState(mode);
		}
	}

	void Button::setHoverState(sl_bool flagState, UIUpdateMode mode)
	{
		if (isHoverState() != flagState) {
			View::setHoverState(flagState, UIUpdateMode::None);
			_invalidateButtonState(mode);
		}
	}

	void Button::onDraw(Canvas* canvas)
	{
		ButtonCategoryProperties& params = m_categories[m_category].properties[(int)m_state];
		ButtonCategoryProperties& paramsDefault = m_categories[m_category].properties[(int)(ButtonState::Default)];

		sl_bool flagText = m_text.isNotEmpty();

		Color textColor = params.textColor;		
		const ColorMatrix* cm = sl_null;
		if (flagText) {
			cm = getCurrentColorFilter(textColor.isZero() && m_flagUseDefaultColorFilter);
			if (textColor.isZero()) {
				textColor = paramsDefault.textColor;
				if (textColor.isZero()) {
					textColor = m_textColor;
				}
			}
		}
		Ref<Drawable> icon = params.icon;
		{
			const ColorMatrix* cm = getCurrentColorFilter(icon.isNull() && m_flagUseDefaultColorFilter);
			if (icon.isNull()) {
				icon = paramsDefault.icon;
				if (icon.isNull()) {
					icon = m_iconDefault;
				}
			}
			if (icon.isNotNull() && cm) {
				icon = icon->filter(*cm);
			}
		}

		if (!flagText && icon.isNull()) {
			return;
		}

		UIRect bound = getBoundsInnerPadding();
		sl_ui_pos widthFrame = bound.getWidth();
		sl_ui_pos heightFrame = bound.getHeight();
		if (widthFrame <= 0 || heightFrame <= 0) {
			return;
		}

		UIRect rcIcon, rcText;
		UISize sizeContent;
		layoutIconAndText(widthFrame, heightFrame, sizeContent, &rcIcon, &rcText);
		UIPoint pt = GraphicsUtil::calculateAlignPosition(bound, (sl_real)(sizeContent.x), (sl_real)(sizeContent.y), m_gravity);

		if (icon.isNotNull() && rcIcon.getWidth() > 0 && rcIcon.getHeight() > 0) {
			rcIcon.left += pt.x;
			rcIcon.top += pt.y;
			rcIcon.right += pt.x;
			rcIcon.bottom += pt.y;
			if (m_iconSize.x > 0 && m_iconSize.y > 0) {
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
			SimpleTextBoxDrawParam param;
			param.frame = rcText;
			sl_real shadowOpacity = getShadowOpacity();
			if (shadowOpacity > 0 && !(isLayer()) && getCurrentBackground().isNull()) {
				param.shadowOpacity = shadowOpacity;
				param.shadowRadius = (sl_real)(getShadowRadius());
				param.shadowColor = getShadowColor();
				param.shadowOffset = getShadowOffset();
			}
			param.lineThickness = UI::dpToPixel(1);
			if (param.lineThickness < 1) {
				param.lineThickness = 1;
			}
			_applyLabelAppearance(param);
			param.color = textColor;
			param.colorMatrix = cm;
			m_textBox.draw(canvas, param);
		}
	}

	void Button::onDrawBackground(Canvas* canvas)
	{
		ButtonCategoryProperties& params = m_categories[m_category].properties[(int)m_state];
		sl_bool flagUseDefaultColorFilter = m_flagUseDefaultColorFilter;

		Ref<Drawable> background = params.background;
		if (background.isNull()) {
			ButtonCategoryProperties& paramsDefault = m_categories[m_category].properties[(int)(ButtonState::Default)];
			background = paramsDefault.background;
			if (background.isNull()) {
				Ref<DrawAttributes>& attrs = m_drawAttrs;
				if (attrs.isNotNull()) {
					switch (m_state) {
					case ButtonState::Hover:
						background = attrs->backgroundHover;
						break;
					case ButtonState::Pressed:
						background = attrs->backgroundPressed;
						break;
					default:
						break;
					}
					if (background.isNull()) {
						background = attrs->background;
					} else {
						flagUseDefaultColorFilter = sl_false;
					}
				}
			}
		} else {
			flagUseDefaultColorFilter = sl_false;
		}
		if (background.isNotNull()) {
			const ColorMatrix* cm = getCurrentColorFilter(flagUseDefaultColorFilter);
			if (cm) {
				background = background->filter(*cm);
			}
			drawBackground(canvas, background);
		}
	}

	void Button::onDrawBorder(Canvas* canvas)
	{
		ButtonCategoryProperties& params = m_categories[m_category].properties[(int)m_state];
		Ref<Pen> pen = params.border;
		if (pen.isNull()) {
			ButtonCategoryProperties& paramsDefault = m_categories[m_category].properties[(int)(ButtonState::Default)];
			pen = paramsDefault.border;
			if (pen.isNull()) {
				pen = getBorder();
				if (pen.isNull()) {
					return;
				}
			}
		}
		drawBorder(canvas, pen);
	}
	
	void Button::onDrawShadow(Canvas* canvas)
	{
		if (drawLayerShadow(canvas)) {
			return;
		}
		if (getCurrentButtonBackground().isNotNull()) {
			drawBoundShadow(canvas);
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
		sl_ui_len width = 0;
		sl_ui_len height = 0;
		sl_ui_len widthPadding = getPaddingLeft() + getPaddingRight();
		sl_ui_len heightPadding = getPaddingTop() + getPaddingBottom();
		if (!flagHorizontalWrapping) {
			width = getLayoutWidth() - widthPadding;
			if (width < 1) {
				if (flagVerticalWrapping) {
					setLayoutHeight(heightPadding);
				}
				return;
			}
		}
		if (!flagVerticalWrapping) {
			height = getLayoutHeight() - heightPadding;
			if (height < 1) {
				if (flagHorizontalWrapping) {
					setLayoutWidth(widthPadding);
				}
				return;
			}
		}
		UISize size = measureLayoutContentSize(width, height);
		if (getChildrenCount()) {
			UISize sizeLayout = measureLayoutWrappingSize(flagHorizontalWrapping, flagVerticalWrapping);
			if (sizeLayout.x > size.x) {
				size.x = sizeLayout.x;
			}
			if (sizeLayout.y > size.y) {
				size.y = sizeLayout.y;
			}
		}
		if (flagHorizontalWrapping) {
			setLayoutWidth(size.x + widthPadding);
		}
		if (flagVerticalWrapping) {
			setLayoutHeight(size.y + heightPadding);
		}
	}

	void Button::onKeyEvent(UIEvent* ev)
	{
		switch (ev->getKeycode()) {
		case Keycode::Enter:
		case Keycode::NumpadEnter:
#if !defined(SLIB_PLATFORM_IS_WIN32)
			if (isNativeWidget()) {
				break;
			}
#endif
			if (ev->getAction() == UIAction::KeyDown) {
				dispatchClick();
				ev->preventDefault();
				ev->stopPropagation();
			}
			break;
		case Keycode::Space:
			if (isNativeWidget()) {
				break;
			}
			switch (ev->getAction()) {
			case UIAction::KeyDown:
				m_state = ButtonState::Pressed;
				invalidate();
				ev->preventDefault();
				ev->stopPropagation();
				break;
			case UIAction::KeyUp:
				if (m_state == ButtonState::Pressed) {
					_invalidateButtonState(UIUpdateMode::Redraw);
					dispatchClick();
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

	void Button::onMnemonic(UIEvent* ev)
	{
		setFocus();
		dispatchClickEvent(ev);
	}

	void Button::onChangeFocus(sl_bool flagFocused)
	{
		_invalidateButtonState(UIUpdateMode::Redraw);
	}

	UISize Button::measureContentSize(sl_ui_len widthFrame, sl_ui_len heightFrame)
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

	UISize Button::measureLayoutContentSize(sl_ui_len widthFrame, sl_ui_len heightFrame)
	{
		return measureContentSize(widthFrame, heightFrame);
	}
	
	void Button::layoutIconAndText(sl_ui_len widthFrame, sl_ui_len heightFrame, UISize& sizeContent, UIRect* pOutFrameIcon, UIRect* pOutFrameText)
	{
		sl_bool flagUseText = m_text.isNotEmpty();

		Alignment alignIcon = m_iconAlignment;
		Alignment alignText = m_textAlignment;

		sl_ui_pos widthIcon = m_iconSize.x;
		sl_ui_pos heightIcon = m_iconSize.y;	
		if (widthIcon < 0) {
			widthIcon = 0;
		}
		if (heightIcon < 0) {
			heightIcon = 0;
		}

		sl_bool flagUseIcon = m_iconDefault.isNotNull() || widthIcon > 0 || heightIcon > 0;
		if (!flagUseIcon) {
			for (sl_uint32 i = 0; i < m_nCategories; i++) {
				ButtonCategory& category = m_categories[i];
				for (sl_uint32 j = 0; j < (sl_uint32)(ButtonState::Count); j++) {
					if (category.properties[j].icon.isNotNull()) {
						flagUseIcon = sl_true;
					}
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
			if (flagWrapping && isMaximumWidthDefined()) {
				flagWrapping = sl_false;
				widthTextLayout = getMaximumWidth() - getPaddingLeft() - getPaddingRight();
			}
			if (!flagWrapping && flagUseIcon && m_layoutOrientation == LayoutOrientation::Horizontal) {
				if (widthIcon <= 0) {
					Ref<Font> font = getFont();
					if (font.isNotNull()) {
						widthIcon = (sl_ui_len)(font->getFontHeight());
					} else {
						widthIcon = 20;
					}
					heightIcon = widthIcon;
				}
				widthTextLayout -= widthIcon + m_iconMarginLeft + m_iconMarginRight;
			}
			if (!flagWrapping) {
				if (widthTextLayout < 1) {
					widthTextLayout = 1;
				}
			}
			_updateTextBox(flagWrapping, widthTextLayout, m_textMarginLeft + m_textMarginRight, alignText);
			if (flagWrapping || !m_flagExtendTextFrame) {
				widthText = (sl_ui_len)(m_textBox.getContentWidth()) + m_textMarginLeft + m_textMarginRight;
			} else {
				widthText = widthTextLayout;
			}
			if (widthText < 0) {
				widthText = 0;
			}
			heightText = (sl_ui_len)(m_textBox.getContentHeight()) + +m_textMarginTop + m_textMarginBottom;
			if (heightText < 0) {
				heightText = 0;
			}
		}

		if (flagUseIcon) {
			sl_ui_len marginWidth = m_iconMarginLeft + m_iconMarginRight;
			sl_ui_len marginHeight = m_iconMarginTop + m_iconMarginBottom;
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
		if (m_layoutOrientation == LayoutOrientation::Horizontal) {
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

		if (widthFrame <= 0 || !m_flagExtendTextFrame) {
			widthFrame = widthContent;
		}
		if (heightFrame <= 0 || !m_flagExtendTextFrame) {
			heightFrame = heightContent;
		}

		if (pOutFrameIcon || pOutFrameText) {
			UIRect rcIconExtend;
			UIRect rcTextExtend;
			if (m_layoutOrientation == LayoutOrientation::Horizontal) {
				rcIconExtend.top = 0;
				rcIconExtend.bottom = heightFrame;
				rcTextExtend.top = 0;
				rcTextExtend.bottom = heightFrame;
				if (m_flagTextBeforeIcon) {
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
				if (m_flagTextBeforeIcon) {
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
				frameIcon.setLeftTop(GraphicsUtil::calculateAlignPosition(rcIconExtend, (sl_real)widthIcon, (sl_real)heightIcon, alignIcon));
				frameIcon.right = frameIcon.left + widthIcon - m_iconMarginRight;
				frameIcon.bottom = frameIcon.top + heightIcon - m_iconMarginBottom;
				frameIcon.left += m_iconMarginLeft;
				frameIcon.top += m_iconMarginTop;
				frameIcon.fixSizeError();
			}
			if (pOutFrameText) {
				UIRect& frameText = *pOutFrameText;
				frameText.setLeftTop(GraphicsUtil::calculateAlignPosition(rcTextExtend, (sl_real)widthText, (sl_real)heightText, alignText));
				frameText.right = frameText.left + widthText - m_textMarginRight;
				frameText.bottom = frameText.top + heightText - m_textMarginBottom;
				frameText.left += m_textMarginLeft;
				frameText.top += m_textMarginTop;
				frameText.fixSizeError();
			}
		}

		sizeContent.x = widthContent;
		sizeContent.y = heightContent;

	}

	const ColorMatrix* Button::getCurrentColorFilter(sl_bool flagUseDefaultFilter)
	{
		ButtonCategoryProperties& params = m_categories[m_category].properties[(int)m_state];
		if (params.flagFilter) {
			return &(params.filter);
		}
		ButtonCategoryProperties& paramsDefault = m_categories[m_category].properties[(int)(ButtonState::Default)];
		if (paramsDefault.flagFilter) {
			return &(paramsDefault.filter);
		}
		if (flagUseDefaultFilter) {
			switch (m_state) {
				case ButtonState::Hover:
					return &(priv::button::g_colorMatrix_hover);
				case ButtonState::Focused:
					return &(priv::button::g_colorMatrix_focused);
				case ButtonState::FocusedHover:
					return &(priv::button::g_colorMatrix_focused_hover);
				case ButtonState::Pressed:
					return &(priv::button::g_colorMatrix_pressed);
				case ButtonState::Disabled:
					return &(priv::button::g_colorMatrix_disabled);
				default:
					break;
			}
		}
		return sl_null;
	}
	
	Ref<Drawable> Button::getCurrentButtonBackground()
	{
		ButtonCategoryProperties& params = m_categories[m_category].properties[(int)m_state];
		if (params.background.isNotNull()) {
			return params.background;
		}
		ButtonCategoryProperties& paramsDefault = m_categories[m_category].properties[(int)(ButtonState::Default)];
		if (paramsDefault.flagFilter) {
			return paramsDefault.background;
		}
		Ref<DrawAttributes>& attrs = m_drawAttrs;
		if (attrs.isNull()) {
			return sl_null;
		}
		switch (m_state) {
			case ButtonState::Hover:
				if (attrs->backgroundHover.isNotNull()) {
					return attrs->backgroundHover;
				}
				break;
			case ButtonState::Pressed:
				if (attrs->backgroundPressed.isNotNull()) {
					return attrs->backgroundPressed;
				}
				break;
			default:
				break;
		}
		return attrs->background;
	}

	void Button::_invalidateButtonState(UIUpdateMode mode)
	{
		if (isEnabled()) {
			if (isPressedState()) {
				m_state = ButtonState::Pressed;
			} else if (isHoverState()) {
				m_state = ButtonState::Hover;
				if (m_flagUseFocusedState) {
					if (isFocused()) {
						m_state = ButtonState::FocusedHover;
					}
				}
			} else {
				m_state = ButtonState::Normal;
				if (m_flagUseFocusedState) {
					if (isFocused()) {
						m_state = ButtonState::Focused;
					}
				}
			}
		} else {
			m_state = ButtonState::Disabled;
		}
		invalidate(mode);
	}

	void Button::_invalidateButtonCategory(UIUpdateMode mode)
	{
		if (m_flagDefaultButton) {
			setCurrentCategory(1, mode);
		} else {
			setCurrentCategory(0, mode);
		}
	}

#if !defined(SLIB_UI_IS_MACOS) && !defined(SLIB_UI_IS_WIN32)
	Ref<ViewInstance> Button::createNativeWidget(ViewInstance* parent)
	{
		return sl_null;
	}
	
	Ptr<IButtonInstance> Button::getButtonInstance()
	{
		return sl_null;
	}
#endif
	
}
