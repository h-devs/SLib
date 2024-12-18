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

#include "slib/ui/date_picker.h"

#include "slib/ui/core.h"
#include "slib/graphics/font.h"

#if defined(SLIB_UI_IS_MACOS) || defined(SLIB_UI_IS_WIN32)
#	define HAS_NATIVE_WIDGET_IMPL 1
#else
#	define HAS_NATIVE_WIDGET_IMPL 0
#endif

namespace slib
{

	SLIB_DEFINE_OBJECT(DatePicker, View)

	DatePicker::DatePicker()
	{
		setSupportedNativeWidget(HAS_NATIVE_WIDGET_IMPL);
		setCreatingNativeWidget(HAS_NATIVE_WIDGET_IMPL);

		setUsingFont(sl_true);
		setFocusable(sl_true);

		m_date = Time::now();
	}

	DatePicker::~DatePicker()
	{
	}

	Time DatePicker::getDate()
	{
		return m_date;
	}

	Time DatePicker::getInstanceDate()
	{
		Ptr<IDatePickerInstance> instance = getDatePickerInstance();
		if (instance.isNotNull()) {
			instance->getDate(this, m_date);
		}
		return m_date;
	}

	void DatePicker::setDate(const Time& date, UIUpdateMode mode)
	{
		Ptr<IDatePickerInstance> instance = getDatePickerInstance();
		if (instance.isNotNull()) {
			SLIB_VIEW_RUN_ON_UI_THREAD(setDate, date, mode)
		}
		Time _date = date;
		_change(instance.get(), _date, sl_null, mode);
	}

	SLIB_DEFINE_EVENT_HANDLER(DatePicker, Changing, (Time& date, UIEvent* ev /* nullable */), date, ev)

	SLIB_DEFINE_EVENT_HANDLER(DatePicker, Change, (const Time& date, UIEvent* ev /* nullable */), date, ev)

	void DatePicker::_change(IDatePickerInstance* instance, Time& date, UIEvent* ev, UIUpdateMode mode)
	{
		ObjectLocker locker(this);
		if (m_date == date) {
			return;
		}
		invokeChanging(date, ev);
		if (m_date == date) {
			return;
		}
		m_date = date;
		if (instance) {
			if (!ev) {
				instance->setDate(this, date);
			}
		} else {
			invalidate(mode);
		}
		locker.unlock();
		invokeChange(date, ev);
	}

	void DatePicker::_onChange_NW(IDatePickerInstance* instance, Time& date)
	{
		Ref<UIEvent> ev = UIEvent::createUnknown(Time::now());
		if (ev.isNotNull()) {
			_change(instance, date, ev.get());
		}
	}

	void DatePicker::onUpdateLayout()
	{
		sl_bool flagHorizontal = isLastWidthWrapping();
		sl_bool flagVertical = isLastHeightWrapping();

		if (!flagVertical && !flagHorizontal) {
			return;
		}

		Ptr<IDatePickerInstance> instance = getDatePickerInstance();
		if (instance.isNotNull()) {
			UISize size;
			if (instance->measureSize(this, size)) {
				if (flagHorizontal) {
					setLayoutWidth(size.x);
				}
				if (flagVertical) {
					setLayoutHeight(size.y);
				}
				return;
			}
		}

		Ref<Font> font = getFont();
		if (flagHorizontal) {
			sl_ui_pos width = getPaddingLeft() + getPaddingRight();
			if (font.isNotNull()) {
				sl_ui_pos t = (sl_ui_pos)(font->getFontHeight());
				if (t > 0) {
					width += t * 4;
				}
			}
			if (width < 0) {
				width = 0;
			}
			setLayoutWidth(width);
		}
		if (flagVertical) {
			sl_ui_pos height = 0;
			if (font.isNotNull()) {
				height = (sl_ui_pos)(font->getFontHeight() * 1.5f);
				if (height < 0) {
					height = 0;
				}
			}
			height += getPaddingTop() + getPaddingBottom();
			if (height < 0) {
				height = 0;
			}
			setLayoutHeight(height);
		}
	}

#if !HAS_NATIVE_WIDGET_IMPL
	Ref<ViewInstance> DatePicker::createNativeWidget(ViewInstance* parent)
	{
		return sl_null;
	}

	Ptr<IDatePickerInstance> DatePicker::getDatePickerInstance()
	{
		return sl_null;
	}
#endif

}
