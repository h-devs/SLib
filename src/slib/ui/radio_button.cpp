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

#include "slib/ui/radio_button.h"

#include "slib/ui/resource.h"
#include "slib/ui/priv/view_state_map.h"
#include "slib/core/safe_static.h"

#if defined(SLIB_UI_IS_MACOS) || defined(SLIB_UI_IS_WIN32) || defined(SLIB_UI_IS_GTK)
#	define HAS_NATIVE_WIDGET_IMPL 1
#else
#	define HAS_NATIVE_WIDGET_IMPL 0
#endif

namespace slib
{

	SLIB_DEFINE_OBJECT(RadioButton, CheckBox)

	RadioButton::RadioButton()
	{
		setSupportedNativeWidget(HAS_NATIVE_WIDGET_IMPL);
	}

	RadioButton::~RadioButton()
	{
	}

	Ref<RadioGroup> RadioButton::getGroup()
	{
		return m_group;
	}

	String RadioButton::getValue()
	{
		return m_value;
	}

	void RadioButton::setValue(const String& value)
	{
		m_value = value;
	}

	Ref<ButtonCell> RadioButton::createButtonCell()
	{
		if (m_categories.isNotNull()) {
			return new RadioButtonCell(m_categories);
		} else {
			return new RadioButtonCell();
		}
	}

	void RadioButton::onClickEvent(UIEvent* ev)
	{
		Button::onClickEvent(ev);
		Ptr<ICheckBoxInstance> instance = getCheckBoxInstance();
		_change(instance.get(), sl_true, ev, UIUpdateMode::Redraw);
	}

	void RadioButton::onChange(sl_bool value, UIEvent* ev)
	{
		CheckBox::onChange(value, ev);
		if (value) {
			Ref<RadioGroup> group = m_group;
			if (group.isNotNull()) {
				group->_select(this, ev, UIUpdateMode::Redraw);
			}
		}
	}

	namespace {

		class Icon : public Drawable
		{
		public:
			Ref<Pen> m_penBorder;
			Ref<Brush> m_brushBack;
			Ref<Brush> m_brushCheck;

		public:
			Icon(const Ref<Pen>& penBorder, const Color& backColor, const Color& checkColor)
			{
				m_penBorder = penBorder;
				if (backColor.a > 0) {
					m_brushBack = Brush::createSolidBrush(backColor);
				}
				if (checkColor.a > 0) {
					m_brushCheck = Brush::createSolidBrush(checkColor);
				}
			}

		public:
			void onDrawAll(Canvas* canvas, const Rectangle& rect, const DrawParam& param) override
			{
				canvas->drawEllipse(rect, m_penBorder, m_brushBack);
				if (m_brushCheck.isNotNull()) {
					Rectangle rcCheck;
					rcCheck.setLeftTop(rect.getCenter());
					sl_real w = rect.getWidth() / 2;
					sl_real h = rect.getHeight() / 2;
					rcCheck.left -= w / 2;
					rcCheck.top -= h / 2;
					rcCheck.right = rcCheck.left + w;
					rcCheck.bottom = rcCheck.top + h;
					canvas->fillEllipse(rcCheck, m_brushCheck);
				}
			}

		};

		class Categories
		{
		public:
			Ref<Drawable> iconDefault[2];
			Ref<Drawable> iconDisabled[2];
			Ref<Drawable> iconHover[2];
			Ref<Drawable> iconPressed[2];

		public:
			Categories()
			{
				sl_real w = (sl_real)(UIResource::toUiPos(UIResource::dpToPixel(1)));
				Color colorBackNormal = Color::White;
				Color colorBackHover = Color::White;
				Color colorBackDown(220, 230, 255);
				Color colorBackDisabled = Color(220, 220, 220);
				Ref<Pen> penNormal = Pen::createSolidPen(w, Color::Black);
				Ref<Pen> penHover = Pen::createSolidPen(w, Color(0, 80, 200));
				Ref<Pen> penDown = penHover;
				Ref<Pen> penDisabled = Pen::createSolidPen(w, Color(90, 90, 90));
				Color colorCheckNormal = Color::Black;
				Color colorCheckDisabled = Color(90, 90, 90);
				Color colorCheckHover = Color(0, 80, 200);
				Color colorCheckDown = colorCheckHover;
				iconDefault[0] = new Icon(penNormal, colorBackNormal, Color::zero());
				iconDisabled[0] = new Icon(penDisabled, colorBackDisabled, Color::zero());
				iconHover[0] = new Icon(penHover, colorBackHover, Color::zero());
				iconPressed[0] = new Icon(penDown, colorBackDown, Color::zero());

				iconDefault[1] = new Icon(penNormal, colorBackNormal, colorCheckNormal);
				iconDisabled[1] = new Icon(penDisabled, colorBackDisabled, colorCheckDisabled);
				iconHover[1] = new Icon(penHover, colorBackHover, colorCheckHover);
				iconPressed[1] = new Icon(penDown, colorBackDown, colorCheckDown);
			}

			static Array<ButtonCategory> createDefault()
			{
				SLIB_SAFE_LOCAL_STATIC(Categories, s)
				if (SLIB_SAFE_STATIC_CHECK_FREED(s)) {
					return sl_null;
				}
				Array<ButtonCategory> ret = Array<ButtonCategory>::create(2);
				if (ret.isNotNull()) {
					ButtonCategory* c = ret.getData();
					for (sl_size i = 0; i < 2; i++) {
						c[i].icons.setDefault(s.iconDefault[i]);
						c[i].icons.set(ViewState::Disabled, s.iconDisabled[i]);
						c[i].icons.set(ViewState::Hover, s.iconHover[i]);
						c[i].icons.set(ViewState::Focused, s.iconHover[i]);
						c[i].icons.set(ViewState::Pressed, s.iconPressed[i]);
						c[i].icons.set(ViewState::FocusedPressed, s.iconPressed[i]);
					}
					return ret;
				}
				return sl_null;
			}

		};

	}

	SLIB_DEFINE_OBJECT(RadioButtonCell, ViewCell)

	RadioButtonCell::RadioButtonCell() : RadioButtonCell(Categories::createDefault())
	{
	}

	RadioButtonCell::RadioButtonCell(const Array<ButtonCategory>& categories) : CheckBoxCell(categories)
	{
	}

	RadioButtonCell::~RadioButtonCell()
	{
	}


	SLIB_DEFINE_OBJECT(RadioGroup, Object)

	RadioGroup::RadioGroup()
	{
	}

	RadioGroup::~RadioGroup()
	{
	}

	List< Ref<RadioButton> > RadioGroup::getButtons()
	{
		ObjectLocker lock(this);
		return m_buttons.duplicate_NoLock();
	}

	void RadioGroup::add(const Ref<RadioButton>& button)
	{
		if (button.isNull()) {
			return;
		}
		button->m_group = this;
		ObjectLocker lock(this);
		m_buttons.addIfNotExist_NoLock(button);
		if (button->isChecked()) {
			if (button != m_buttonSelected) {
				if (m_buttonSelected.isNotNull()) {
					m_buttonSelected->setChecked(sl_false);
				}
				m_buttonSelected = button;
			}
		}
	}

	void RadioGroup::remove(const Ref<RadioButton>& button)
	{
		if (button.isNull()) {
			return;
		}
		button->m_group.setNull();
		ObjectLocker lock(this);
		m_buttons.remove_NoLock(button);
		if (m_buttonSelected == button) {
			m_buttonSelected.setNull();
		}
	}

	Ref<RadioButton> RadioGroup::getSelected()
	{
		ObjectLocker lock(this);
		return m_buttonSelected;
	}

	void RadioGroup::select(const Ref<RadioButton>& button, UIUpdateMode mode)
	{
		if (button.isNotNull()) {
			button->setChecked(sl_true, mode);
		}
	}

	void RadioGroup::selectValue(const String& value, UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		ListElements< Ref<RadioButton> > buttons(m_buttons);
		Ref<RadioButton> selected;
		for (sl_size i = 0; i < buttons.count; i++) {
			Ref<RadioButton>& button = buttons[i];
			if (String(button->m_value) == value) {
				selected = button;
				break;
			}
		}
		select(selected, mode);
	}

	String RadioGroup::getSelectedValue()
	{
		ObjectLocker lock(this);
		if (m_buttonSelected.isNotNull()) {
			return m_buttonSelected->m_value;
		}
		return sl_null;
	}

	void RadioGroup::_select(RadioButton* button, UIEvent* ev, UIUpdateMode mode)
	{
		ObjectLocker locker(this);
		Ref<RadioButton> former = m_buttonSelected;
		if (button == former) {
			return;
		}
		m_buttonSelected = button;
		if (former.isNotNull()) {
			former->setChecked(sl_false, mode);
		}
		locker.unlock();
		invokeSelect(button, former.get(), ev);
	}

	SLIB_DEFINE_EVENT_HANDLER(RadioGroup, Select, (RadioButton* button, RadioButton* former, UIEvent* ev), button, former, ev)

#if !HAS_NATIVE_WIDGET_IMPL
	Ref<ViewInstance> RadioButton::createNativeWidget(ViewInstance* parent)
	{
		return sl_null;
	}
#endif

}

