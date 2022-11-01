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

#ifndef CHECKHEADER_SLIB_UI_SELECT_VIEW
#define CHECKHEADER_SLIB_UI_SELECT_VIEW

#include "label_list.h"

namespace slib
{

	class ISelectViewInstance;

	class SelectSwitchCell;

	class SLIB_EXPORT SelectView : public View, public SingleSelectionViewBase<SelectView, sl_uint32>
	{
		SLIB_DECLARE_OBJECT

	public:
		SelectView();

		~SelectView();

	public:
		Alignment getGravity();

		void setGravity(const Alignment& align, UIUpdateMode mode = UIUpdateMode::Redraw);

		Color getTextColor();

		void setTextColor(const Color& color, UIUpdateMode mode = UIUpdateMode::Redraw);

	public:
		SLIB_DECLARE_EVENT_HANDLER(SelectView, SelectItem, sl_uint32 index)

	protected:
		void onDraw(Canvas* canvas) override;

		void onMouseEvent(UIEvent* ev) override;

		void onUpdateLayout() override;

	protected:
		Ref<ViewInstance> createNativeWidget(ViewInstance* parent) override;

		virtual Ptr<ISelectViewInstance> getSelectViewInstance();

	private:
		void _initCell();

	public:
		SLIB_DECLARE_SINGLE_SELECTION_VIEW_NOTIFY_FUNCTIONS(SelectView, sl_uint32)

	protected:
		Ref<SelectSwitchCell> m_cell;

		Alignment m_gravity;
		Color m_textColor;

	};

	class SLIB_EXPORT ISelectViewInstance
	{
	public:
		SLIB_DECLARE_SINGLE_SELECTION_VIEW_INSTANCE_NOTIFY_FUNCTIONS(SelectView, sl_uint32)

		virtual void setGravity(SelectView* view, const Alignment& gravity);

		virtual void setTextColor(SelectView* view, const Color& color);

		virtual sl_bool measureSize(SelectView* view, UISize& _out);

	};


	class SLIB_EXPORT SelectSwitch : public View, public SingleSelectionViewBase<SelectSwitch, sl_uint32>
	{
		SLIB_DECLARE_OBJECT

	public:
		SelectSwitch();

		~SelectSwitch();

	protected:
		void init() override;

	public:
		const UISize& getIconSize();

		virtual void setIconSize(const UISize& size, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		void setIconSize(sl_ui_len width, sl_ui_len height, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		void setIconSize(sl_ui_len size, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		sl_ui_len getIconWidth();

		void setIconWidth(sl_ui_len width, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		sl_ui_len getIconHeight();

		void setIconHeight(sl_ui_len height, UIUpdateMode mode = UIUpdateMode::UpdateLayout);


		Ref<Drawable> getLeftIcon();

		virtual void setLeftIcon(const Ref<Drawable>& icon, UIUpdateMode mode = UIUpdateMode::Redraw);

		Ref<Drawable> getRightIcon();

		virtual void setRightIcon(const Ref<Drawable>& icon, UIUpdateMode mode = UIUpdateMode::Redraw);


		Alignment getGravity();

		virtual void setGravity(const Alignment& align, UIUpdateMode mode = UIUpdateMode::Redraw);

		Color getTextColor();

		virtual void setTextColor(const Color& color, UIUpdateMode mode = UIUpdateMode::Redraw);

	public:
		SLIB_DECLARE_EVENT_HANDLER(SelectSwitch, SelectItem, sl_uint32 index)

	protected:
		void onDraw(Canvas* canvas) override;

		void onMouseEvent(UIEvent* ev) override;

		void onUpdateLayout() override;

	public:
		SLIB_DECLARE_SINGLE_SELECTION_VIEW_NOTIFY_FUNCTIONS(SelectSwitch, sl_uint32)

	protected:
		Ref<SelectSwitchCell> m_cell;

	};

	class SLIB_EXPORT SelectSwitchCell : public SingleSelectionViewCellBase<sl_uint32>
	{
		SLIB_DECLARE_OBJECT

	public:
		Alignment gravity;
		Color textColor;

		UISize iconSize;
		AtomicRef<Drawable> leftIcon;
		AtomicRef<Drawable> rightIcon;

		Function<void(sl_uint32 index)> onSelectItem;

	public:
		SelectSwitchCell();

		~SelectSwitchCell();

	public:
		void onDraw(Canvas* canvas) override;

		void onMouseEvent(UIEvent* ev) override;

		void onMeasure(UISize& size, sl_bool flagHorizontalWrapping, sl_bool flagVerticalWrapping) override;

	protected:
		UIRect getLeftIconRegion();

		UIRect getRightIconRegion();

	protected:
		int m_clickedIconNo;

	};

}

#endif
