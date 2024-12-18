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

#ifndef CHECKHEADER_SLIB_UI_DRAWER
#define CHECKHEADER_SLIB_UI_DRAWER

#include "view.h"
#include "motion_tracker.h"

namespace slib
{

	class SLIB_EXPORT Drawer : public ViewGroup
	{
		SLIB_DECLARE_OBJECT

	public:
		Drawer();

		~Drawer();

	protected:
		void init() override;

	public:
		sl_bool isOpened();

		virtual void open();

		virtual void close();

		Ref<View> getContent();

		void setContent(const Ref<View>& view);

		sl_ui_len getDrawerSize();

		void setDrawerSize(sl_ui_len size);

		sl_ui_len getDragEdgeSize();

		void setDragEdgeSize(sl_ui_len size);

		Alignment getGravity();

		void setGravity(const Alignment& align);

	protected:
		void onChangeParent(View* oldParent, View* newParent) override;

	public:
		void dispatchMouseEvent(UIEvent* ev) override;

		void dispatchTouchEvent(UIEvent* ev) override;

		void onCancel() override;

	private:
		void _onEndOpenAnimation();

		void _onEndCloseAnimation();

		void _onParentMouseEvent(View* view, UIEvent* ev);

		void _onMouseEvent(UIEvent* ev);

		Vector2 _makeContentTranslation(sl_real t);

		sl_real _getContentTranslation();

		void _setContentTranslation(sl_real t);


	private:
		sl_bool m_flagOpened;
		sl_ui_len m_drawerSize;
		sl_ui_len m_dragEdgeSize;
		Alignment m_gravity;

		AtomicRef<Animation> m_animation;
		View* m_parent;
		Function<void(View*, UIEvent*)> m_parentMouseEventHandler;

		sl_bool m_flagMouseDown;
		sl_real m_posMouseDown;
		MotionTracker m_motionTracker;
	};

}

#endif
