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

#ifndef CHECKHEADER_SLIB_UI_PICKER_VIEW
#define CHECKHEADER_SLIB_UI_PICKER_VIEW

#include "definition.h"

#include "view.h"
#include "motion_tracker.h"

namespace slib
{
	
	class IPickerViewInstance;

	class SLIB_EXPORT PickerView : public View
	{
		SLIB_DECLARE_OBJECT
		
	public:
		PickerView();
		
		~PickerView();

	public:
		sl_uint32 getItemsCount();
		
		virtual void setItemsCount(sl_uint32 n, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		void removeAllItems(UIUpdateMode mode = UIUpdateMode::Redraw);
		
		
		String getItemValue(sl_uint32 index);
		
		virtual void setItemValue(sl_uint32 index, const String& value);
		
		List<String> getValues();
		
		virtual void setValues(const List<String>& values);
		
		
		String getItemTitle(sl_uint32 index);
		
		virtual void setItemTitle(sl_uint32 index, const String& title, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		List<String> getTitles();
		
		virtual void setTitles(const List<String>& values, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		
		virtual void selectItem(sl_uint32 index, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		virtual void selectValue(const String& value, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		sl_uint32 getSelectedIndex();
		
		String getSelectedValue();
		
		String getSelectedTitle();
		
		
		Color getTextColor();
		
		virtual void setTextColor(const Color& color, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		
	public:		
		SLIB_DECLARE_EVENT_HANDLER(PickerView, SelectItem, sl_uint32 index)

	protected:
		void onDraw(Canvas* canvas) override;
		
		void onMouseEvent(UIEvent* ev) override;
		
	protected:
		Ref<ViewInstance> createNativeWidget(ViewInstance* parent) override;
		
		virtual Ptr<IPickerViewInstance> getPickerViewInstance();
		
	private:
		void _selectItemInner(sl_int32 index);
		
		sl_uint32 _getCircularIndex(sl_int32 index);
		
		sl_ui_len _getLineHeight();
		
		void _flow(sl_ui_pos offset);
		
		void _startFlow(sl_real speed);
		
		void _stopFlow();
		
		void _animationCallback(Timer* timer);
		
	protected:
		AtomicList<String> m_values;
		AtomicList<String> m_titles;
		sl_uint32 m_indexSelected;
		Color m_textColor;
		
		sl_uint32 m_linesHalfCount;
		sl_bool m_flagCircular;
		
		sl_ui_pos m_yOffset;
		
		MotionTracker m_motionTracker;
		Ref<Timer> m_timerFlow;
		sl_real m_speedFlow;
		Time m_timeFlowFrameBefore;
		
	};
	
	class SLIB_EXPORT IPickerViewInstance
	{
	public:
		virtual void select(PickerView* view, sl_uint32 index) = 0;
		
		virtual void refreshItemsCount(PickerView* view) = 0;
		
		virtual void refreshItemsContent(PickerView* view) = 0;
		
		virtual void setItemTitle(PickerView* view, sl_uint32 index, const String& title) = 0;
		
	};

}

#endif
