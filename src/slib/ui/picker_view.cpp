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

#include "slib/core/timer.h"
#include "slib/core/safe_static.h"

#include "label_list_base_impl.h"

#define ANIMATE_FRAME_MS 15

#if defined(SLIB_UI_IS_IOS)
#	define HAS_NATIVE_WIDGET_IMPL 1
#else
#	define HAS_NATIVE_WIDGET_IMPL 0
#endif

namespace slib
{

	SLIB_DEFINE_OBJECT(PickerView, View)
	SLIB_DEFINE_SINGLE_SELECTION_VIEW_INSTANCE_NOTIFY_FUNCTIONS(PickerView, sl_uint32, IPickerViewInstance, getPickerViewInstance)

	PickerView::PickerView()
	{
		setSupportedNativeWidget(HAS_NATIVE_WIDGET_IMPL);
		setCreatingNativeWidget(HAS_NATIVE_WIDGET_IMPL);

		setUsingFont(sl_true);
		setClipping(sl_true, UIUpdateMode::Init);

		m_textColor = Color::Black;

		m_lineCount = 5;
		m_flagCircular = sl_false;
	}

	PickerView::~PickerView()
	{
	}

	Color PickerView::getTextColor()
	{
		return m_textColor;
	}

	void PickerView::setTextColor(const Color& color, UIUpdateMode mode)
	{
		m_textColor = color;
		if (m_cell.isNotNull()) {
			m_cell->textColor = color;
		}
		invalidate(mode);
	}

	sl_uint32 PickerView::getLineCount()
	{
		return m_lineCount;
	}

	void PickerView::setLineCount(sl_uint32 count)
	{
		m_lineCount = count;
	}

	sl_bool PickerView::isCircular()
	{
		return m_flagCircular;
	}

	void PickerView::setCircular(sl_bool flag)
	{
		m_flagCircular = flag;
	}

	void PickerView::onDraw(Canvas* canvas)
	{
		_initCell();
		if (m_cell.isNotNull()) {
			m_cell->onDraw(canvas);
		}
	}

	void PickerView::onMouseEvent(UIEvent* ev)
	{
		if (m_cell.isNotNull()) {
			m_cell->onMouseEvent(ev);
		}
	}

	SLIB_DEFINE_EVENT_HANDLER(PickerView, SelectItem, sl_uint32 index)

	void PickerView::dispatchSelectItem(sl_uint32 index)
	{
		ObjectLocker lock(this);
		if (m_indexSelected == index) {
			return;
		}
		m_indexSelected = index;
		lock.unlock();

		SLIB_INVOKE_EVENT_HANDLER(SelectItem, index)
	}

	void PickerView::_initCell()
	{
		if (m_cell.isNull()) {
			Ref<PickerViewCell> cell = new PickerViewCell;
			if (cell.isNotNull()) {
				cell->setView(this, sl_true);
				cell->initLabelList(this);
				cell->textColor = m_textColor;
				cell->lineCount = m_lineCount;
				cell->flagCircular = m_flagCircular;
				cell->onSelectItem = SLIB_FUNCTION_WEAKREF(this, dispatchSelectItem);
				m_cell = cell;
			}
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


	SLIB_DEFINE_OBJECT(PickerViewCell, SingleSelectionViewCellBase<sl_uint32>)

	PickerViewCell::PickerViewCell()
	{
		textColor = Color::Black;
		lineCount = 5;
		flagCircular = sl_false;

		m_yOffset = 0;
		m_speedFlow = 0;
	}

	PickerViewCell::~PickerViewCell()
	{
	}

	void PickerViewCell::onDraw(Canvas* canvas)
	{
		Ref<Font> font = getFont();
		if (font.isNull()) {
			return;
		}
		sl_int32 nLinesHalf = lineCount >> 1;
		sl_real lineHeight = (sl_real)(_getLineHeight());
		sl_real height = (nLinesHalf * 2 + 1) * lineHeight;

		Rectangle rect = getFrame();
		sl_real yStart = rect.top + (rect.getHeight() - height) / 2;

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
					index = selectedIndex + i;
				}
				if (index >= 0 && index < (sl_int32)itemCount) {
					sl_int32 alpha = 50 + 100 * (nLinesHalf + 1 - Math::abs(i)) / (nLinesHalf + 1);
					Color c = textColor;
					c.a = (sl_uint8)((sl_int32)(c.a) * alpha / 256);
					canvas->drawText(titleGetter(index), rect, font, c, Alignment::Center);
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
					index = _getCircularIndex(selectedIndex + i);
				} else {
					index = selectedIndex + i;
				}
				if (index >= 0 && index < (sl_int32)itemCount) {
					sl_int32 alpha = 50 + 100 * (nLinesHalf + 1 - Math::abs(i)) / (nLinesHalf + 1);
					Color c = textColor;
					c.a = (sl_uint8)((sl_int32)(c.a) * alpha / 256);
					canvas->drawText(titleGetter(index), rect, font, c, Alignment::Center);
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
					index = _getCircularIndex(selectedIndex + i);
				} else {
					index = selectedIndex + i;
				}
				if (index >= 0 && index < (sl_int32)itemCount) {
					canvas->drawText(titleGetter(index), rect, font, c, Alignment::Center);
				}
				rect.top = rect.bottom;
			}
		}
	}

	void PickerViewCell::onMouseEvent(UIEvent* ev)
	{
		UIAction action = ev->getAction();

		if (action == UIAction::LeftButtonDown || action == UIAction::TouchBegin) {
			_stopFlow();
			m_motionTracker.clearMovements();
			m_motionTracker.addMovement(ev->getPoint());
			ev->useDrag();
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
		if (flagCircular) {
			index = _getCircularIndex(index);
		} else {
			sl_int32 n = itemCount;
			if (n <= 0) {
				return;
			}
			if (index >= n) {
				index = n - 1;
			} else if (index < 0) {
				index = 0;
			}
		}
		if (selectedIndex != (sl_uint32)index) {
			selectedIndex = index;
			onSelectItem(index);
		}
	}

	sl_uint32 PickerViewCell::_getCircularIndex(sl_int32 index)
	{
		sl_int32 n = itemCount;
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
		sl_int32 index = selectedIndex;
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
		m_yOffset = (sl_ui_pos)(m_yOffset + offset - (index - (sl_int32)selectedIndex) * lineHeight);
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
		m_timerFlow = startTimer(SLIB_FUNCTION_WEAKREF(this, _animationCallback), ANIMATE_FRAME_MS);
	}

	void PickerViewCell::_stopFlow()
	{
		m_timerFlow.setNull();
	}

	void PickerViewCell::_animationCallback(Timer* timer)
	{
		Time time = Time::now();
		sl_real ellapsed = (sl_real)((time - m_timeFlowFrameBefore).getSecondCountf());
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
