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

#include "slib/ui/check_box.h"

#include "slib/ui/core.h"
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

	SLIB_DEFINE_OBJECT(CheckBox, Button)

	CheckBox::CheckBox()
	{
		setSupportedNativeWidget(HAS_NATIVE_WIDGET_IMPL);

		m_flagChecked = sl_false;
	}

	CheckBox::~CheckBox()
	{
	}

	sl_bool CheckBox::isChecked()
	{
		return m_flagChecked;
	}

	sl_bool CheckBox::isCheckedInstance()
	{
		Ptr<ICheckBoxInstance> instance = getCheckBoxInstance();
		if (instance.isNotNull()) {
			sl_bool flag;
			if (instance->getChecked(this, flag)) {
				m_flagChecked = flag;
			}
		}
		return m_flagChecked;
	}

	void CheckBox::setChecked(sl_bool flag, UIUpdateMode mode)
	{
		Ptr<ICheckBoxInstance> instance = getCheckBoxInstance();
		if (instance.isNotNull()) {
			SLIB_VIEW_RUN_ON_UI_THREAD(setChecked, flag, mode)
		}
		_change(instance.get(), flag, sl_null, mode);
	}

	Ref<ButtonCell> CheckBox::createButtonCell()
	{
		if (m_categories.isNotNull()) {
			return new CheckBoxCell(m_categories);
		} else {
			return new CheckBoxCell();
		}
	}

	SLIB_DEFINE_EVENT_HANDLER(CheckBox, Change, (sl_bool value, UIEvent* ev), value, ev)

	void CheckBox::onClickEvent(UIEvent* ev)
	{
		Button::onClickEvent(ev);
		Ptr<ICheckBoxInstance> instance = getCheckBoxInstance();
		if (instance.isNotNull()) {
			sl_bool flag;
			if (instance->getChecked(this, flag)) {
				m_flagChecked = flag;
				_change(instance.get(), flag, ev, UIUpdateMode::None);
			}
		} else {
			_change(sl_null, !m_flagChecked, ev, UIUpdateMode::Redraw);
		}
	}

	void CheckBox::_change(ICheckBoxInstance* instance, sl_bool value, UIEvent* ev, UIUpdateMode mode)
	{
		ObjectLocker locker(this);
		if (m_flagChecked == value) {
			return;
		}
		m_flagChecked = value;
		if (instance) {
			if (!ev) {
				setCurrentCategory(value ? 1 : 0, UIUpdateMode::None);
				instance->setChecked(this, value);
			}
		} else {
			setCurrentCategory(value ? 1 : 0, mode);
		}
		locker.unlock();
		invokeChange(value, ev);
	}

	void CheckBox::_onChange_NW(ICheckBoxInstance* instance, sl_bool value)
	{
		Ref<UIEvent> ev = UIEvent::createUnknown(Time::now());
		if (ev.isNotNull()) {
			_change(instance, value, ev.get(), UIUpdateMode::None);
		}
	}

#if !HAS_NATIVE_WIDGET_IMPL
	Ref<ViewInstance> CheckBox::createNativeWidget(ViewInstance* parent)
	{
		return sl_null;
	}

	Ptr<ICheckBoxInstance> CheckBox::getCheckBoxInstance()
	{
		return sl_null;
	}
#endif


	namespace {

		class Icon : public Drawable
		{
		public:
			Ref<Pen> m_penBorder;
			Ref<Brush> m_brush;
			Ref<Pen> m_penCheck;

		public:
			Icon(const Ref<Pen>& penBorder, const Color& backColor, const Ref<Pen>& penCheck)
			{
				m_penBorder = penBorder;
				if (backColor.a > 0) {
					m_brush = Brush::createSolidBrush(backColor);
				}
				m_penCheck = penCheck;
			}

		public:
			void onDrawAll(Canvas* canvas, const Rectangle& rect, const DrawParam& param) override
			{
				sl_bool flagAntiAlias = canvas->isAntiAlias();
				if (flagAntiAlias) {
					canvas->setAntiAlias(sl_false);
				}
				canvas->drawRectangle(rect, m_penBorder, m_brush);
				canvas->setAntiAlias(sl_true);
				if (m_penCheck.isNotNull()) {
					Point pts[3];
					pts[0] = Point(0.2f, 0.6f);
					pts[1] = Point(0.4f, 0.8f);
					pts[2] = Point(0.8f, 0.3f);
					for (int i = 0; i < 3; i++) {
						pts[i].x = rect.left + rect.getWidth() * pts[i].x;
						pts[i].y = rect.top + rect.getHeight() * pts[i].y;
					}
					canvas->drawLines(pts, 3, m_penCheck);
				}
				if (!flagAntiAlias) {
					canvas->setAntiAlias(sl_false);
				}
			}

		};

		class Categories
		{
		public:
			Ref<Icon> iconDefault[2];
			Ref<Icon> iconDisabled[2];
			Ref<Icon> iconHover[2];
			Ref<Icon> iconPressed[2];

		public:
			Categories()
			{
				sl_real w = (sl_real)(UIResource::toUiPos(UIResource::dpToPixel(1)));
				Color colorBackNormal = Color::White;
				Color colorBackHover = Color::White;
				Color colorBackDown(220, 230, 255);
				Color colorBackDisabled(220, 220, 220);
				Ref<Pen> penNormal = Pen::createSolidPen(w, Color::Black);
				Ref<Pen> penHover = Pen::createSolidPen(w, Color(0, 80, 200));
				Ref<Pen> penDown = penHover;
				Ref<Pen> penDisabled = Pen::createSolidPen(w, Color(90, 90, 90));
				Ref<Pen> penCheckNormal = Pen::createSolidPen(w * 2, Color::Black);
				Ref<Pen> penCheckHover = Pen::createSolidPen(w * 2, Color(0, 80, 200));
				Ref<Pen> penCheckDown = penCheckHover;
				Ref<Pen> penCheckDisabled = Pen::createSolidPen(w * 2, Color(90, 90, 90));
				iconDefault[0] = new Icon(penNormal, colorBackNormal, Ref<Pen>::null());
				iconDisabled[0] = new Icon(penDisabled, colorBackDisabled, Ref<Pen>::null());
				iconHover[0] = new Icon(penHover, colorBackHover, Ref<Pen>::null());
				iconPressed[0] = new Icon(penDown, colorBackDown, Ref<Pen>::null());

				iconDefault[1] = new Icon(penNormal, colorBackNormal, penCheckNormal);
				iconDisabled[1] = new Icon(penDisabled, colorBackDisabled, penCheckDisabled);
				iconHover[1] = new Icon(penHover, colorBackHover, penCheckHover);
				iconPressed[1] = new Icon(penDown, colorBackDown, penCheckDown);
			}

		public:
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

	SLIB_DEFINE_OBJECT(CheckBoxCell, ButtonCell)

	CheckBoxCell::CheckBoxCell(): CheckBoxCell(Categories::createDefault())
	{
	}

	CheckBoxCell::CheckBoxCell(const Array<ButtonCategory>& categories): ButtonCell(categories)
	{
		gravity = Alignment::Left;
		textColors.setDefault(Color::Black);
		textMarginLeft = 2 * UIResource::toUiPos(UIResource::dpToPixel(1));
		textMarginTop = 1;
		textMarginRight = 1;
		textMarginBottom = 2;
		iconMarginLeft = 1;
		iconMarginTop = 2;
		iconMarginRight = 1;
		iconMarginBottom = 1;
	}

	CheckBoxCell::~CheckBoxCell()
	{
	}

}

