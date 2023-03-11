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

#ifndef CHECKHEADER_SLIB_UI_IMAGE_VIEW
#define CHECKHEADER_SLIB_UI_IMAGE_VIEW

#include "view.h"

namespace slib
{

	class UrlRequestParam;

	class SLIB_EXPORT ImageView : public View
	{
		SLIB_DECLARE_OBJECT

	public:
		ImageView();

		~ImageView();

	protected:
		void init() override;

	public:
		static Ref<ImageView> create(sl_ui_len width, sl_ui_len height, const Ref<Drawable>& drawable);

	public:
		Ref<Drawable> getSource();

		virtual void setSource(const Ref<Drawable>& drawable, UIUpdateMode mode = UIUpdateMode::UpdateLayout);


		ScaleMode getScaleMode();

		virtual void setScaleMode(ScaleMode scaleMode, UIUpdateMode updateMode = UIUpdateMode::Redraw);


		Alignment getGravity();

		virtual void setGravity(const Alignment& align, UIUpdateMode mode = UIUpdateMode::Redraw);


		sl_bool isAutoAspectRatio();

		void setAutoAspectRatio(sl_bool flag);

		sl_real getMinimumAutoAspectRatio();

		void setMinimumAutoAspectRatio(sl_real ratio);

		sl_real getMaximumAutoAspectRatio();

		void setMaximumAutoAspectRatio(sl_real ratio);


		void loadUrl(const String& url);

		void loadUrl(const UrlRequestParam& param);

	protected:
		void onDraw(Canvas* canvas) override;

		void onUpdateLayout() override;

		virtual void onAnimationFrame(Timer* timer);

	protected:
		AtomicRef<Drawable> m_source;
		ScaleMode m_scaleMode;
		Alignment m_gravity;
		sl_bool m_flagAutoAspectRatio;
		sl_real m_aspectRatioMin;
		sl_real m_aspectRatioMax;

		AtomicRef<Timer> m_timerAnimation;
		Time m_timeStartAnimation;
		AtomicRef<CRef> m_request;

	};

}

#endif
