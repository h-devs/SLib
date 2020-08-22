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

#include "slib/ui/picker_view.h"

#include "slib/ui/resource.h"
#include "slib/ui/core.h"
#include "slib/core/safe_static.h"
#include "slib/core/math.h"
#include "slib/core/timer.h"

#include "label_list_base_impl.h"

#define ANIMATE_FRAME_MS 15

#if defined(SLIB_UI_IS_IOS)
#	define HAS_NATIVE_WIDGET_IMPL 1
#else
#	define HAS_NATIVE_WIDGET_IMPL 0
#endif

namespace slib
{

	PickerViewCellParent::PickerViewCellParent()
	{
		m_textColor = Color::Black;

		m_linesCount = 5;
		m_flagCircular = sl_false;
	}

	PickerViewCellParent::~PickerViewCellParent()
	{
	}

	Color PickerViewCellParent::getTextColor()
	{
		return m_textColor;
	}

	void PickerViewCellParent::setTextColor(const Color& color, UIUpdateMode mode)
	{
		m_textColor = color;
	}

	sl_uint32 PickerViewCellParent::getLinesCount()
	{
		return m_linesCount;
	}

	void PickerViewCellParent::setLinesCount(sl_uint32 count)
	{
		m_linesCount = count;
	}

	sl_bool PickerViewCellParent::isCircular()
	{
		return m_flagCircular;
	}

	void PickerViewCellParent::setCircular(sl_bool flag)
	{
		m_flagCircular = flag;
	}

	SLIB_DEFINE_OBJECT(PickerView, View)
	SLIB_DEFINE_SINGLE_SELECTION_VIEW_INSTANCE_NOTIFY_FUNCTIONS(PickerView, sl_uint32, IPickerViewInstance, getPickerViewInstance)

	PickerView::PickerView()
	{
		setSupportedNativeWidget(HAS_NATIVE_WIDGET_IMPL);
		setCreatingNativeWidget(HAS_NATIVE_WIDGET_IMPL);

		setUsingFont(sl_true);
		setClipping(sl_true, UIUpdateMode::Init);
	}

	PickerView::~PickerView()
	{
	}

	void PickerView::setTextColor(const Color& color, UIUpdateMode mode)
	{
		m_textColor = color;
		PickerViewCellParent::setTextColor(color);
		invalidate(mode);
	}

	void PickerView::onDraw(Canvas* canvas)
	{
		_initCell();
		if (m_cell.isNotNull()) {
			m_cell->draw(canvas);
		}
	}

	void PickerView::onMouseEvent(UIEvent* ev)
	{
		if (m_cell.isNotNull()) {
			m_cell->processMouseEvent(ev);
		}
	}

	SLIB_DEFINE_EVENT_HANDLER(PickerView, SelectItem, sl_uint32 index)

	void PickerView::dispatchSelectItem(sl_uint32 index)
	{
		SLIB_INVOKE_EVENT_HANDLER(SelectItem, index)
	}

	List<String> PickerView::getTitles_Cell()
	{
		return m_titles;
	}

	sl_uint32 PickerView::getSelectedIndex_Cell()
	{
		return getSelectedIndex();
	}

	void PickerView::onSelectItem_Cell(sl_uint32 index)
	{
		m_indexSelected = index;
		dispatchSelectItem(index);
	}

	void PickerView::_initCell()
	{
		if (m_cell.isNull()) {
			m_cell = new PickerViewCell(this, this);
		}
	}

#if !HAS_NATIVE_WIDGET_IMPL
	Ref<ViewInstance> PickerView::createNativeWidget(ViewInstance* parent)
	{
		return sl_null;
	}

	Ptr<IPickerViewInstance> PickerView::getPickerViewInstance()
	{
		return sl_null;
	}
#endif


	SLIB_DEFINE_OBJECT(PickerViewCell, ViewCell)

	PickerViewCell::PickerViewCell(View* view, PickerViewCellParent* _parent): ViewCell(view), parent(_parent)
	{
		m_yOffset = 0;
		m_speedFlow = 0;
	}

	PickerViewCell::~PickerViewCell()
	{
	}

	void PickerViewCell::draw(Canvas* canvas)
	{
		Ref<Font> font = getFont();
		if (font.isNull()) {
			return;
		}
		sl_int32 nLinesHalf = parent->getLinesCount() >> 1;
		sl_real lineHeight = (sl_real)(_getLineHeight());
		sl_real height = (nLinesHalf * 2 + 1) * lineHeight;

		Rectangle rect = getFrame();
		sl_real yStart = rect.top + (rect.getHeight() - height) / 2;

		sl_bool flagCircular = parent->isCircular();
		sl_int32 indexSelected = parent->getSelectedIndex_Cell();
		Color textColor = parent->getTextColor();

		ListLocker<String> titles(parent->getTitles_Cell());
		sl_int32 i;

		{
			CanvasStateScope scope(canvas);
			canvas->clipToRectangle(rect.left, yStart, rect.right - rect.left, nLinesHalf * lineHeight);
			rect.top = yStart - lineHeight + m_yOffset;
			for (i = -nLinesHalf - 1; i <= 0; i++) {
				rect.bottom = rect.top + lineHeight;
				sl_int32 index;
				if (flagCircular) {
					index = _getCircularIndex( + i);
				} else {
					index = indexSelected + i;
				}
				if (index >= 0 && index < (sl_int32)(titles.count)) {
					sl_int32 alpha = 50 + 100 * (nLinesHalf + 1 - Math::abs(i)) / (nLinesHalf + 1);
					Color c = textColor;
					c.a = (sl_uint8)((sl_int32)(c.a) * alpha / 256);
					canvas->drawText(titles[index], rect, font, c, Alignment::Center);
				}
				rect.top = rect.bottom;
			}
		}

		{
			CanvasStateScope scope(canvas);
			canvas->clipToRectangle(rect.left, yStart + nLinesHalf * lineHeight + lineHeight, rect.right - rect.left, lineHeight * nLinesHalf);
			rect.top = yStart + nLinesHalf * lineHeight + m_yOffset;
			for (i = 0; i <= nLinesHalf + 1; i++) {
				rect.bottom = rect.top + lineHeight;
				sl_int32 index;
				if (flagCircular) {
					index = _getCircularIndex(indexSelected + i);
				} else {
					index = indexSelected + i;
				}
				if (index >= 0 && index < (sl_int32)(titles.count)) {
					sl_int32 alpha = 50 + 100 * (nLinesHalf + 1 - Math::abs(i)) / (nLinesHalf + 1);
					Color c = textColor;
					c.a = (sl_uint8)((sl_int32)(c.a) * alpha / 256);
					canvas->drawText(titles[index], rect, font, c, Alignment::Center);
				}
				rect.top = rect.bottom;
			}
		}

		{
			CanvasStateScope scope(canvas);
			canvas->clipToRectangle(rect.left, yStart + nLinesHalf * lineHeight, rect.right - rect.left, lineHeight);
			rect.top = yStart + nLinesHalf * lineHeight - lineHeight + m_yOffset;
			Color c = textColor;
			for (i = -1; i <= 1; i++) {
				rect.bottom = rect.top + lineHeight;
				sl_int32 index;
				if (flagCircular) {
					index = _getCircularIndex(indexSelected + i);
				} else {
					index = indexSelected + i;
				}
				if (index >= 0 && index < (sl_int32)(titles.count)) {
					canvas->drawText(titles[index], rect, font, c, Alignment::Center);
				}
				rect.top = rect.bottom;
			}
		}
	}

	void PickerViewCell::processMouseEvent(UIEvent* ev)
	{
		UIAction action = ev->getAction();

		if (action == UIAction::LeftButtonDown || action == UIAction::TouchBegin) {
			_stopFlow();
			m_motionTracker.clearMovements();
			m_motionTracker.addMovement(ev->getPoint());
			invalidate();
		} else if (action == UIAction::LeftButtonDrag || action == UIAction::TouchMove) {
			_stopFlow();
			Point ptLast;
			if (m_motionTracker.getLastPosition(&ptLast)) {
				sl_real offset = ev->getY() - ptLast.y;
				_flow((sl_ui_pos)offset);
				invalidate();
			}
			m_motionTracker.addMovement(ev->getPoint());
		} else if (action == UIAction::LeftButtonUp || action == UIAction::TouchEnd || action == UIAction::TouchCancel) {
			m_motionTracker.addMovement(ev->getPoint());
			sl_real speed = 0;
			m_motionTracker.getVelocity(sl_null, &speed);
			m_motionTracker.clearMovements();
			_startFlow(speed);
			invalidate();
		}
	}

	void PickerViewCell::_selectItemInner(sl_int32 index)
	{
		if (parent->isCircular()) {
			index = _getCircularIndex(index);
		} else {
			sl_int32 n = (sl_uint32)(parent->getTitles_Cell().getCount());
			if (n <= 0) {
				return;
			}
			if (index >= n) {
				index = n - 1;
			} else if (index < 0) {
				index = 0;
			}
		}
		if (parent->getSelectedIndex_Cell() != (sl_uint32)index) {
			parent->onSelectItem_Cell(index);
		}
	}

	sl_uint32 PickerViewCell::_getCircularIndex(sl_int32 index)
	{
		sl_int32 n = (sl_uint32)(parent->getTitles_Cell().getCount());
		if (n <= 0) {
			return 0;
		}
		index = index % n;
		if (index < 0) {
			index += n;
		}
		return index;
	}

	sl_ui_len PickerViewCell::_getLineHeight()
	{
		Ref<Font> font = getFont();
		if (font.isNotNull()) {
			return (sl_ui_len)(font->getFontHeight() * 1.2f);
		}
		return 10;
	}

	void PickerViewCell::_flow(sl_ui_pos offset)
	{
		sl_int32 k = (sl_int32)(m_yOffset + offset);
		sl_int32 lineHeight = (sl_int32)(_getLineHeight());
		if (lineHeight == 0) {
			return;
		}
		sl_int32 indexSelected = parent->getSelectedIndex_Cell();
		sl_int32 index = indexSelected;
		if (k >= 0) {
			sl_int32 n = k / lineHeight;
			sl_int32 m = k - n * lineHeight;
			if (m > lineHeight / 2) {
				n += 1;
			}
			_selectItemInner(index - n);
		} else {
			k = -k;
			sl_int32 n = k / lineHeight;
			sl_int32 m = k - n * lineHeight;
			if (m > lineHeight / 2) {
				n += 1;
			}
			_selectItemInner(index + n);
		}
		indexSelected = parent->getSelectedIndex_Cell();
		m_yOffset = (sl_ui_pos)(m_yOffset + offset - (index - indexSelected) * lineHeight);
		if (m_yOffset > lineHeight) {
			m_yOffset = lineHeight;
			m_speedFlow = 0;
		}
		if (m_yOffset < -lineHeight) {
			m_yOffset = -lineHeight;
			m_speedFlow = 0;
		}
	}

	void PickerViewCell::_startFlow(sl_real speed)
	{
		m_speedFlow = speed;
		m_timeFlowFrameBefore = Time::now();
		m_timerFlow = startTimer(SLIB_FUNCTION_WEAKREF(PickerViewCell, _animationCallback, this), ANIMATE_FRAME_MS);
	}

	void PickerViewCell::_stopFlow()
	{
		m_timerFlow.setNull();
	}

	void PickerViewCell::_animationCallback(Timer* timer)
	{
		Time time = Time::now();
		sl_real ellapsed = (sl_real)((time - m_timeFlowFrameBefore).getSecondsCountf());
		m_timeFlowFrameBefore = time;

		float T = UIResource::getScreenMinimum() /
#ifdef SLIB_PLATFORM_IS_MOBILE
			2.0f;
#else
			4.0f;
#endif
		if (Math::abs(m_speedFlow) <= T) {
			if (Math::abs(m_yOffset) >= 1) {
				if (m_yOffset > 0) {
					m_speedFlow = -T;
				} else {
					m_speedFlow = T;
				}
				sl_ui_pos f = (sl_ui_pos)(m_speedFlow * ellapsed);
				if (Math::abs(f) > Math::abs(m_yOffset)) {
					_stopFlow();
					m_yOffset = 0;
					invalidate();
					return;
				} else {
					_flow(f);
				}
			} else {
				_stopFlow();
				m_yOffset = 0;
				invalidate();
				return;
			}
		} else {
			_flow((sl_ui_pos)(m_speedFlow * ellapsed));
		}

		invalidate();

		m_speedFlow *= 0.97f;

	}

}
