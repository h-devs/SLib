/*
 *   Copyright (c) 2008-2018 SLIBIO <https://github.com/SLIBIO>
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
#include "slib/core/safe_static.h"

#if defined(SLIB_UI_IS_MACOS) || defined(SLIB_UI_IS_WIN32)
#	define HAS_NATIVE_WIDGET_IMPL 1
#else
#	define HAS_NATIVE_WIDGET_IMPL 0
#endif

namespace slib
{

	namespace priv
	{
		namespace check_box
		{

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
					canvas->setAntiAlias(sl_false);
					canvas->drawRectangle(rect, m_penBorder, m_brush);
					canvas->setAntiAlias(flagAntiAlias);

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
				}

			};

			class Categories
			{
			public:
				ButtonCategory categories[2];
				Array<ButtonCategory> arrCategories;

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
					categories[0].properties[(int)ButtonState::Normal].icon = new Icon(penNormal, colorBackNormal, Ref<Pen>::null());
					categories[0].properties[(int)ButtonState::Disabled].icon = new Icon(penDisabled, colorBackDisabled, Ref<Pen>::null());
					categories[0].properties[(int)ButtonState::Focused].icon =
						categories[0].properties[(int)ButtonState::FocusedHover].icon =
						categories[0].properties[(int)ButtonState::Hover].icon =
						new Icon(penHover, colorBackHover, Ref<Pen>::null());
					categories[0].properties[(int)ButtonState::Pressed].icon = new Icon(penDown, colorBackDown, Ref<Pen>::null());

					categories[1] = categories[0];
					categories[1].properties[(int)ButtonState::Normal].icon = new Icon(penNormal, colorBackNormal, penCheckNormal);
					categories[1].properties[(int)ButtonState::Disabled].icon = new Icon(penDisabled, colorBackDisabled, penCheckDisabled);
					categories[1].properties[(int)ButtonState::Focused].icon =
						categories[1].properties[(int)ButtonState::FocusedHover].icon =
						categories[1].properties[(int)ButtonState::Hover].icon =
						new Icon(penHover, colorBackHover, penCheckHover);
					categories[1].properties[(int)ButtonState::Pressed].icon = new Icon(penDown, colorBackDown, penCheckDown);

					arrCategories = Array<ButtonCategory>::createStatic(categories, 2);
				}

			public:
				static Array<ButtonCategory> getInitialCategories()
				{
					SLIB_SAFE_LOCAL_STATIC(Categories, s)
					if (SLIB_SAFE_STATIC_CHECK_FREED(s)) {
						return sl_null;
					}
					return s.arrCategories;
				}
			};

		}
	}

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
			SLIB_VIEW_RUN_ON_UI_THREAD(&CheckBox::setChecked, flag, mode)
				m_flagChecked = flag;
			setCurrentCategory(flag ? 1 : 0, UIUpdateMode::None);
			instance->setChecked(this, flag);
		} else {
			m_flagChecked = flag;
			setCurrentCategory(flag ? 1 : 0, mode);
		}
	}

	Ref<ButtonCell> CheckBox::createButtonCell()
	{
		if (m_categories.isNotNull()) {
			return new CheckBoxCell(m_categories);
		} else {
			return new CheckBoxCell();
		}
	}

	SLIB_DEFINE_EVENT_HANDLER(CheckBox, Change, sl_bool newValue)

	void CheckBox::dispatchChange(sl_bool newValue)
	{
		SLIB_INVOKE_EVENT_HANDLER(Change, newValue)
	}

	void CheckBox::dispatchClickEvent(UIEvent* ev)
	{
		if (!(ev->isInternal()) && isNativeWidget()) {
			sl_bool valueOld = m_flagChecked;
			sl_bool valueNew = isCheckedInstance();
			if (valueOld != valueNew) {
				dispatchChange(valueNew);
			}
		} else {
			sl_bool valueNew = !m_flagChecked;
			setChecked(valueNew);
			dispatchChange(valueNew);
		}
		Button::dispatchClickEvent(ev);
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


	SLIB_DEFINE_OBJECT(CheckBoxCell, ButtonCell)

	CheckBoxCell::CheckBoxCell(): CheckBoxCell(priv::check_box::Categories::getInitialCategories().duplicate())
	{
	}

	CheckBoxCell::CheckBoxCell(const Array<ButtonCategory>& categories): ButtonCell(categories)
	{
		gravity = Alignment::MiddleLeft;
		textColor = Color::Black;
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

