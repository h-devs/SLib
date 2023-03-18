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
#include "view_state_map.h"

#include "../core/shared.h"

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


		Color getTextColor(sl_uint32 category, ViewState state = ViewState::Default);

		void setTextColor(sl_uint32 category, const Color& color, ViewState state, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setTextColor(sl_uint32 category, const Color& color, UIUpdateMode mode = UIUpdateMode::Redraw);

		Color getTextColor(ViewState state = ViewState::Default);

		void setTextColor(const Color& color, ViewState state, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setTextColor(const Color& color, UIUpdateMode updateMode = UIUpdateMode::Redraw);


		Ref<Drawable> getIcon(sl_uint32 category, ViewState state = ViewState::Default);

		void setIcon(sl_uint32 category, const Ref<Drawable>& icon, ViewState state, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		void setIcon(sl_uint32 category, const Ref<Drawable>& icon, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		Ref<Drawable> getIcon(ViewState state = ViewState::Default);

		void setIcon(const Ref<Drawable>& icon, ViewState state, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		void setIcon(const Ref<Drawable>& icon, UIUpdateMode mode = UIUpdateMode::UpdateLayout);


		using View::getBackground;
		using View::setBackground;

		Ref<Drawable> getBackground(sl_uint32 category, ViewState state = ViewState::Default);

		void setBackground(sl_uint32 category, const Ref<Drawable>& background, ViewState state, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setBackground(sl_uint32 category, const Ref<Drawable>& background, UIUpdateMode mode = UIUpdateMode::Redraw);

		using View::getBackgroundColor;
		using View::setBackgroundColor;

		Color getBackgroundColor(sl_uint32 category, ViewState state = ViewState::Default);

		void setBackgroundColor(sl_uint32 category, const Color& color, ViewState state, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setBackgroundColor(sl_uint32 category, const Color& color, UIUpdateMode mode = UIUpdateMode::Redraw);

		using View::getBorder;
		using View::setBorder;

		Ref<Pen> getBorder(sl_uint32 category, ViewState state = ViewState::Default);

		void setBorder(sl_uint32 category, const Ref<Pen>& pen, ViewState state, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setBorder(sl_uint32 category, const Ref<Pen>& pen, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setBorder(sl_uint32 category, const PenDesc& desc, ViewState state, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setBorder(sl_uint32 category, const PenDesc& desc, UIUpdateMode mode = UIUpdateMode::Redraw);


		Shared<ColorMatrix> getColorFilter(sl_uint32 category, ViewState state = ViewState::Default);

		void setColorFilter(sl_uint32 category, ColorMatrix* filter, ViewState state, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setColorFilter(sl_uint32 category, ColorMatrix* filter, UIUpdateMode mode = UIUpdateMode::Redraw);

		Shared<ColorMatrix> getColorFilter(ViewState state = ViewState::Default);

		void setColorFilter(ColorMatrix* filter, ViewState state, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setColorFilter(ColorMatrix* filter, UIUpdateMode mode = UIUpdateMode::Redraw);


		void setColorOverlay(sl_uint32 category, const Color& color, ViewState state, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setColorOverlay(sl_uint32 category, const Color& color, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setColorOverlay(const Color& color, ViewState state, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setColorOverlay(const Color& color, UIUpdateMode mode = UIUpdateMode::Redraw);


		sl_bool isUsingDefaultColorFilter();

		void setUsingDefaultColorFilter(sl_bool flag = sl_true, UIUpdateMode mode = UIUpdateMode::Redraw);


		Ref<Drawable> getCurrentBackground() override;

		Ref<Pen> getCurrentBorder() override;

	protected:
		Ref<ViewInstance> createNativeWidget(ViewInstance* parent) override;

		virtual Ptr<IButtonInstance> getButtonInstance();

		virtual Ref<ButtonCell> createButtonCell();

	public:
		void dispatchDraw(Canvas* canvas) override;

	protected:
		void onDraw(Canvas* canvas) override;

		void onKeyEvent(UIEvent* ev) override;

		void onMnemonic(UIEvent* ev) override;

		void onChangeFocus(sl_bool flagFocused) override;

		void onUpdateLayout() override;

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

	class SLIB_EXPORT ButtonCategory
	{
	public:
		ViewStateMap<Color> textColors;
		ViewStateMap< Ref<Drawable> > backgrounds;
		ViewStateMap< Ref<Pen> > borders;
		ViewStateMap< Ref<Drawable> > icons;
		ViewStateMap< Shared<ColorMatrix> > filters;

	public:
		ButtonCategory();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(ButtonCategory)

	};

	class SLIB_EXPORT ButtonCell : public LabelViewCell
	{
		SLIB_DECLARE_OBJECT

	public:
		Array<ButtonCategory> categories;

		sl_uint32 category;

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
		sl_bool flagUseDefaultColorFilter;

		Function<void(UIEvent* ev)> onClick;

	public:
		ButtonCell();

		ButtonCell(const Array<ButtonCategory>& categories);

		~ButtonCell();

	public:
		Ref<Drawable> getFinalBackground(ViewState state);

		Ref<Pen> getFinalBorder(ViewState state);

		Color getFinalTextColor(ViewState state);

		Ref<Drawable> getFinalIcon(ViewState state);

		sl_bool getFinalColorFilter(ColorMatrix& _out, ViewState state, sl_bool flagUseDefaultFilter);

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
