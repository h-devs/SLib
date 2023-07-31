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

#include "slib/ui/combo_box.h"

#include "slib/graphics/font.h"

#include "label_list_base_impl.h"

#if defined(SLIB_UI_IS_WIN32) || defined(SLIB_UI_IS_MACOS) || defined(SLIB_UI_IS_GTK)
#	define HAS_NATIVE_WIDGET_IMPL 1
#else
#	define HAS_NATIVE_WIDGET_IMPL 0
#endif

namespace slib
{

	SLIB_DEFINE_OBJECT(ComboBox, View)
	SLIB_DEFINE_LABEL_LIST_INSTANCE_NOTIFY_FUNCTIONS(ComboBox, sl_int32, IComboBoxInstance, getComboBoxInstance)
	template class SingleSelectionViewBase<ComboBox, sl_int32>;

	ComboBox::ComboBox()
	{
		setSupportedNativeWidget(HAS_NATIVE_WIDGET_IMPL);
		setCreatingNativeWidget(HAS_NATIVE_WIDGET_IMPL);

		setUsingFont(sl_true);
		setBorder(sl_true, UIUpdateMode::Init);
		setBackgroundColor(Color::White, UIUpdateMode::Init);
		setSavingCanvasState(sl_false);
		setFocusable(sl_true);

		m_indexSelected = -1;
	}

	ComboBox::~ComboBox()
	{
	}

	String ComboBox::getText()
	{
		return m_text;
	}

	String ComboBox::getInstanceText()
	{
		Ptr<IComboBoxInstance> instance = getComboBoxInstance();
		if (instance.isNotNull()) {
			if (UI::isUiThread()) {
				String text;
				if (instance->getText(this, text)) {
					m_text = Move(text);
				}
			}
		}
		return m_text;
	}

	void ComboBox::setText(const String& text, UIUpdateMode mode)
	{
		Ptr<IComboBoxInstance> instance = getComboBoxInstance();
		if (instance.isNotNull()) {
			SLIB_VIEW_RUN_ON_UI_THREAD(setText, text, mode)
		}
		String _text = text;
		_changeText(instance.get(), _text, sl_null, mode);
	}

	void ComboBox::_changeText(IComboBoxInstance* instance, String& text, UIEvent* ev, UIUpdateMode mode)
	{
		ObjectLocker locker(this);
		if (text == m_text) {
			return;
		}
		invokeChanging(text, ev);
		if (text == m_text) {
			return;
		}
		m_text = text;
		if (instance) {
			if (!ev) {
				instance->setText(this, text);
			}
		} else {
			invalidate(mode);
		}
		locker.unlock();
		invokeChange(text, ev);
	}

	void ComboBox::_onChange_NW(IComboBoxInstance* instance, String& text)
	{
		Ref<UIEvent> ev = UIEvent::createUnknown(Time::now());
		if (ev.isNotNull()) {
			_changeText(instance, text, ev.get(), UIUpdateMode::None);
		}
	}

	void ComboBox::notifySelectItem(sl_int32 index, UIEvent* ev, UIUpdateMode mode)
	{
		Ptr<IComboBoxInstance> instance = getComboBoxInstance();
		String text = getItemTitle(index);
		if (instance.isNotNull()) {
			String newText = text;
			_changeText(instance.get(), newText, ev, mode);
			if (newText != text) {
				instance->setText(this, newText);
			}
		} else {
			_changeText(sl_null, text, ev, mode);
		}
		ObjectLocker locker(this);
		sl_int32 former = m_indexSelected;
		if (former == index) {
			return;
		}
		m_indexSelected = index;
		if (instance.isNotNull()) {
			if (!ev) {
				instance->selectItem((ComboBox*)this, index);
			}
		} else {
			if (m_cell.isNotNull()) {
				m_cell->selectedIndex = m_indexSelected;
				m_cell->text = text;
			}
			invalidate(mode);
		}
		locker.unlock();
		invokeSelectItem(index, former, ev);
	}

	void ComboBox::_onSelectItem_NW(sl_int32 index)
	{
		Ref<UIEvent> ev = UIEvent::createUnknown(Time::now());
		if (ev.isNotNull()) {
			notifySelectItem(index, ev.get(), UIUpdateMode::None);
		}
	}

	void ComboBox::onUpdateLayout()
	{
		sl_bool flagHorizontalWrapping = isLastWidthWrapping();
		sl_bool flagVerticalWrapping = isLastHeightWrapping();

		if (!flagVerticalWrapping && !flagHorizontalWrapping) {
			return;
		}

		Ref<Font> font = getFont();
		if (flagHorizontalWrapping) {
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

		if (flagVerticalWrapping) {
			sl_ui_pos height = 0;
			do {
				Ptr<IComboBoxInstance> instance = getComboBoxInstance();
				if (instance.isNotNull()) {
					height = instance->measureHeight(this);
					if (height > 0) {
						break;
					}
				}
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
			} while (0);
			setLayoutHeight(height);
		}
	}

	SLIB_DEFINE_EVENT_HANDLER(ComboBox, SelectItem, (sl_int32 index, sl_int32 former, UIEvent* ev), index, former, ev)

	SLIB_DEFINE_EVENT_HANDLER(ComboBox, Changing, (String& text, UIEvent* ev /* nullable */), text, ev)

	SLIB_DEFINE_EVENT_HANDLER(ComboBox, Change, (const String& text, UIEvent* ev /* nullable */), text, ev)

	SLIB_DEFINE_EVENT_HANDLER(ComboBox, ReturnKey, ())

	void ComboBox::onKeyEvent(UIEvent* ev)
	{
		if (ev->getAction() == UIAction::KeyDown) {
			Keycode keycode = ev->getKeycode();
			if (keycode == Keycode::Enter || keycode == Keycode::NumpadEnter) {
				invokeReturnKey();
			}
		}
		View::onKeyEvent(ev);
	}

#if !HAS_NATIVE_WIDGET_IMPL
	Ref<ViewInstance> ComboBox::createNativeWidget(ViewInstance* parent)
	{
		return sl_null;
	}

	Ptr<IComboBoxInstance> ComboBox::getComboBoxInstance()
	{
		return sl_null;
	}
#endif

	sl_ui_len IComboBoxInstance::measureHeight(ComboBox* view)
	{
		return 0;
	}


	SLIB_DEFINE_OBJECT(ComboBoxCell, SingleSelectionViewCellBase<sl_uint32>)

	ComboBoxCell::ComboBoxCell()
	{
	}

	ComboBoxCell::~ComboBoxCell()
	{
	}

}
