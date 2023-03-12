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

#ifndef CHECKHEADER_SLIB_UI_BUTTON
#define CHECKHEADER_SLIB_UI_BUTTON

#include "label_view.h"

namespace slib
{

	class IButtonInstance;

	class ButtonCategory;
	class ButtonCell;

	class SLIB_EXPORT Button : public View
	{
		SLIB_DECLARE_OBJECT

	public:
		Button();

		~Button();

	protected:
		void init() override;

	public:
		String getText();

		sl_bool isHyperText();

		void setText(const String& text, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		void setHyperText(const String& text, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		MultiLineMode getMultiLine();

		void setMultiLine(MultiLineMode multiLineMode, UIUpdateMode updateMode = UIUpdateMode::UpdateLayout);

		sl_uint32 getLineCount();

		void setLineCount(sl_uint32 nLines, UIUpdateMode updateMode = UIUpdateMode::UpdateLayout);

		sl_bool isMnemonic();

		// call before `setText()`
		void setMnemonic(sl_bool flag = sl_true);


		Color getTextColor();

		void setTextColor(const Color& color, UIUpdateMode updateMode = UIUpdateMode::Redraw);

		Alignment getGravity();

		void setGravity(const Alignment& align, UIUpdateMode updateMode = UIUpdateMode::Redraw);

		EllipsizeMode getEllipsize();

		void setEllipsize(EllipsizeMode ellipsizeMode, UIUpdateMode updateMode = UIUpdateMode::UpdateLayout);


		sl_bool isDefaultButton();

		void setDefaultButton(sl_bool flag = sl_true, UIUpdateMode mode = UIUpdateMode::Redraw);


		void setCategories(const Array<ButtonCategory>& categories);

		sl_uint32 getCategoryCount();

		sl_uint32 getCurrentCategory();

		void setCurrentCategory(sl_uint32 n, UIUpdateMode mode = UIUpdateMode::Redraw);

		ButtonState getButtonState();

		sl_bool isUsingFocusedState();

		void setUsingFocusedState(sl_bool flag = sl_true);


		const UISize& getIconSize();

		void setIconSize(const UISize& size, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		void setIconSize(sl_ui_len width, sl_ui_len height, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		void setIconSize(sl_ui_len size, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		sl_ui_len getIconWidth();

		void setIconWidth(sl_ui_len width, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		sl_ui_len getIconHeight();

		void setIconHeight(sl_ui_len height, UIUpdateMode mode = UIUpdateMode::UpdateLayout);


		Alignment getIconAlignment();

		void setIconAlignment(const Alignment& align, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		Alignment getTextAlignment();

		void setTextAlignment(const Alignment& align, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		sl_bool isTextBeforeIcon();

		void setTextBeforeIcon(sl_bool flag = sl_true, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		sl_bool isExtendTextFrame();

		void setExtendTextFrame(sl_bool flag = sl_true, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		LayoutOrientation getLayoutOrientation();

		void setLayoutOrientation(LayoutOrientation orientation, UIUpdateMode mode = UIUpdateMode::UpdateLayout);


		void setIconMargin(sl_ui_pos left, sl_ui_pos top, sl_ui_pos right, sl_ui_pos bottom, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		void setIconMargin(sl_ui_pos margin, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		sl_ui_pos getIconMarginLeft();

		void setIconMarginLeft(sl_ui_pos margin, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		sl_ui_pos getIconMarginTop();

		void setIconMarginTop(sl_ui_pos margin, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		sl_ui_pos getIconMarginRight();

		void setIconMarginRight(sl_ui_pos margin, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		sl_ui_pos getIconMarginBottom();

		void setIconMarginBottom(sl_ui_pos margin, UIUpdateMode mode = UIUpdateMode::UpdateLayout);


		void setTextMargin(sl_ui_pos left, sl_ui_pos top, sl_ui_pos right, sl_ui_pos bottom, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		void setTextMargin(sl_ui_pos margin, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		sl_ui_pos getTextMarginLeft();

		void setTextMarginLeft(sl_ui_pos margin, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		sl_ui_pos getTextMarginTop();

		void setTextMarginTop(sl_ui_pos margin, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		sl_ui_pos getTextMarginRight();

		void setTextMarginRight(sl_ui_pos margin, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		sl_ui_pos getTextMarginBottom();

		void setTextMarginBottom(sl_ui_pos margin, UIUpdateMode mode = UIUpdateMode::UpdateLayout);


		Color getTextColor(ButtonState state, sl_uint32 category = 0);

		void setTextColor(const Color& color, ButtonState state, sl_uint32 category = 0, UIUpdateMode mode = UIUpdateMode::Redraw);


		Ref<Drawable> getIcon(ButtonState state, sl_uint32 category = 0);

		void setIcon(const Ref<Drawable>& icon, ButtonState state, sl_uint32 category = 0, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		Ref<Drawable> getIcon();

		void setIcon(const Ref<Drawable>& icon, UIUpdateMode mode = UIUpdateMode::UpdateLayout);


		using View::getBackground;

		using View::setBackground;

		Ref<Drawable> getBackground(ButtonState state, sl_uint32 category = 0);

		void setBackground(const Ref<Drawable>& background, ButtonState state, sl_uint32 category = 0, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setBackground(const Color& backgroundColor, ButtonState state, sl_uint32 category = 0, UIUpdateMode mode = UIUpdateMode::Redraw);


		using View::getBorder;

		using View::setBorder;

		Ref<Pen> getBorder(ButtonState state, sl_uint32 category = 0);

		void setBorder(const Ref<Pen>& pen, ButtonState state, sl_uint32 category = 0, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setBorder(const PenDesc& desc, ButtonState state, sl_uint32 category = 0, UIUpdateMode mode = UIUpdateMode::Redraw);


		ColorMatrix* getColorFilter(ButtonState state, sl_uint32 category = 0);

		void setColorFilter(ColorMatrix* filter, ButtonState state, sl_uint32 category = 0, UIUpdateMode mode = UIUpdateMode::Redraw);

		ColorMatrix* getColorFilter();

		void setColorFilter(ColorMatrix* filter, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setColorOverlay(const Color& color, ButtonState state, sl_uint32 category = 0, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setColorOverlay(const Color& color, UIUpdateMode mode = UIUpdateMode::Redraw);


		sl_bool isUsingDefaultColorFilter();

		void setUsingDefaultColorFilter(sl_bool flag = sl_true, UIUpdateMode mode = UIUpdateMode::Redraw);


		Ref<Drawable> getCurrentBackground();

		Ref<Pen> getCurrentBorder();

	protected:
		Ref<ViewInstance> createNativeWidget(ViewInstance* parent) override;

		virtual Ptr<IButtonInstance> getButtonInstance();

		virtual Ref<ButtonCell> createButtonCell();

	public:
		void setEnabled(sl_bool flagEnabled = sl_true, UIUpdateMode mode = UIUpdateMode::Redraw) override;

		void setPressedState(sl_bool flagState = sl_true, UIUpdateMode mode = UIUpdateMode::Redraw) override;

		void setHoverState(sl_bool flagState = sl_true, UIUpdateMode mode = UIUpdateMode::Redraw) override;

		void dispatchDraw(Canvas* canvas) override;

	protected:
		void onDraw(Canvas* canvas) override;

		void onDrawBackground(Canvas* canvas) override;

		void onDrawBorder(Canvas* canvas) override;

		void onDrawShadow(Canvas* canvas) override;

		void onUpdateLayout() override;

		void onKeyEvent(UIEvent* ev) override;

		void onMnemonic(UIEvent* ev) override;

		void onChangeFocus(sl_bool flagFocused) override;

	protected:
		void prepareButtonCellLayout(ButtonCell* cell);

	private:
		void _initCell();

	protected:
		AtomicString m_text;
		sl_bool m_flagDefaultButton;
		Array<ButtonCategory> m_categories;

		Ref<ButtonCell> m_cell;

	};

	class SLIB_EXPORT IButtonInstance
	{
	public:
		virtual void setText(Button* view, const String& text) = 0;

		virtual void setDefaultButton(Button* view, sl_bool flag) = 0;

		virtual sl_bool measureSize(Button* view, UISize& _out) = 0;

	};

	class SLIB_EXPORT ButtonCategoryProperties
	{
	public:
		Color textColor;
		AtomicRef<Drawable> background;
		AtomicRef<Pen> border;
		AtomicRef<Drawable> icon;
		sl_bool flagFilter;
		ColorMatrix filter;

	public:
		ButtonCategoryProperties();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(ButtonCategoryProperties)

	};

	class SLIB_EXPORT ButtonCategory
	{
	public:
		ButtonCategoryProperties properties[(int)(ButtonState::Count)];

	public:
		ButtonCategory();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(ButtonCategory)

	};

	class SLIB_EXPORT ButtonCell : public LabelViewCell
	{
		SLIB_DECLARE_OBJECT

	public:
		Array<ButtonCategory> categories;

		ButtonState state;
		sl_uint32 category;
		sl_bool flagUseFocusedState;

		UISize iconSize;
		Alignment iconAlignment;
		Alignment textAlignment;
		sl_bool flagTextBeforeIcon;
		sl_bool flagExtendTextFrame;
		LayoutOrientation layoutOrientation;
		sl_ui_pos iconMarginLeft;
		sl_ui_pos iconMarginTop;
		sl_ui_pos iconMarginRight;
		sl_ui_pos iconMarginBottom;
		sl_ui_pos textMarginLeft;
		sl_ui_pos textMarginTop;
		sl_ui_pos textMarginRight;
		sl_ui_pos textMarginBottom;

		AtomicRef<Drawable> iconDefault;
		sl_bool flagUseDefaultColorFilter;

		Function<void(UIEvent* ev)> onClick;

	public:
		ButtonCell();

		ButtonCell(const Array<ButtonCategory>& categories);

		~ButtonCell();

	public:
		void invalidateButtonState();

		Ref<Drawable> getCurrentBackground();

		Ref<Pen> getCurrentBorder();

		const ColorMatrix* getCurrentColorFilter(sl_bool flagUseDefaultFilter);

		UISize measureContentSize(sl_ui_len widthFrame, sl_ui_len heightFrame);

		void layoutIconAndText(sl_ui_len widthFrame, sl_ui_len heightFrame, UISize& sizeContent, UIRect* pOutFrameIcon, UIRect* pOutFrameText);

	public:
		void onDraw(Canvas* canvas) override;

		void onDrawContent(Canvas* canvas);

		void onKeyEvent(UIEvent* ev) override;

		void onMeasure(UISize& size, sl_bool flagHorizontalWrapping, sl_bool flagVerticalWrapping) override;

	};

}

#endif
