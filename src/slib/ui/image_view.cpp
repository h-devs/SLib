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

#include "slib/ui/image_view.h"

#include "slib/graphics/canvas.h"
#include "slib/core/timer.h"

namespace slib
{
	SLIB_DEFINE_OBJECT(ImageView, View)
	
	ImageView::ImageView()
	{
		setSavingCanvasState(sl_false);
		
		m_scaleMode = ScaleMode::Stretch;
		m_gravity = Alignment::MiddleCenter;
		
		m_flagAutoAspectRatio = sl_false;
		m_aspectRatioMin = -1;
		m_aspectRatioMax = -1;
	}
	
	ImageView::~ImageView()
	{
	}

	Ref<ImageView> ImageView::create(sl_ui_len width, sl_ui_len height, const Ref<slib::Drawable> &drawable)
	{
		Ref<ImageView> ret = new ImageView;
		if (ret.isNotNull()) {
			ret->setSource(drawable, UIUpdateMode::Init);
			ret->setWidth(width, UIUpdateMode::Init);
			ret->setHeight(height, UIUpdateMode::Init);
			return ret;
		}
		return sl_null;
	}
	
	Ref<Drawable> ImageView::getSource()
	{
		return m_source;
	}
	
	void ImageView::setSource(const Ref<Drawable>& source, UIUpdateMode mode)
	{
		if (m_source == source) {
			return;
		}
		m_source = source;
		m_timerAnimation.setNull();
		if (source.isNotNull()) {
			if (m_flagAutoAspectRatio) {
				sl_real h = source->getDrawableHeight();
				if (h > 0.0000001) {
					sl_real ratio = source->getDrawableWidth() / h;
					sl_real min = m_aspectRatioMin;
					if (min > 0.0000001) {
						if (ratio < min) {
							ratio = min;
						}
					}
					sl_real max = m_aspectRatioMax;
					if (max > 0) {
						if (ratio > max) {
							ratio = max;
						}
					}
					setAspectRatio(ratio, mode);
				}
			}
			DrawableAnimationInfo animation;
			if (source->getAnimationInfo(&animation)) {
				float fps = animation.framesPerSecond;
				if (fps < 1) {
					fps = 1;
				}
				sl_uint32 interval = (sl_int32)(1000 / fps);
				if (interval > 60) {
					interval = 60;
				}
				m_timeStartAnimation = Time::now();
				m_timerAnimation = startTimer(SLIB_FUNCTION_WEAKREF(this, onAnimationFrame), interval);
			}
		}
		invalidateLayoutOfWrappingControl(mode);
	}
	
	ScaleMode ImageView::getScaleMode()
	{
		return m_scaleMode;
	}
	
	void ImageView::setScaleMode(ScaleMode scaleMode, UIUpdateMode updateMode)
	{
		m_scaleMode = scaleMode;
		invalidate(updateMode);
	}
	
	Alignment ImageView::getGravity()
	{
		return m_gravity;
	}
	
	void ImageView::setGravity(const Alignment& align, UIUpdateMode mode)
	{
		m_gravity = align;
		invalidate(mode);
	}
	
	sl_bool ImageView::isAutoAspectRatio()
	{
		return m_flagAutoAspectRatio;
	}
	
	void ImageView::setAutoAspectRatio(sl_bool flag)
	{
		m_flagAutoAspectRatio = flag;
	}
	
	sl_real ImageView::getMinimumAutoAspectRatio()
	{
		return m_aspectRatioMin;
	}
	
	void ImageView::setMinimumAutoAspectRatio(sl_real ratio)
	{
		m_aspectRatioMin = ratio;
	}
	
	sl_real ImageView::getMaximumAutoAspectRatio()
	{
		return m_aspectRatioMax;
	}
	
	void ImageView::setMaximumAutoAspectRatio(sl_real ratio)
	{
		m_aspectRatioMax = ratio;
	}
	
	void ImageView::onDraw(Canvas* canvas)
	{
		if (m_timerAnimation.isNotNull()) {
			double time = (Time::now() - m_timeStartAnimation).getSecondsCountf();
			DrawParam param;
			param.time = (float)time;
			canvas->draw(getBoundsInnerPadding(), m_source, m_scaleMode, m_gravity, param);
		} else {
			canvas->draw(getBoundsInnerPadding(), m_source, m_scaleMode, m_gravity);
		}
	}
	
	void ImageView::onUpdateLayout()
	{
		sl_bool flagHorizontal = isWidthWrapping();
		sl_bool flagVertical = isHeightWrapping();
		
		if (!flagVertical && !flagHorizontal) {
			return;
		}
		
		Ref<Drawable> source = m_source;
		if (source.isNotNull()) {
			if (flagHorizontal) {
				setLayoutWidth((sl_ui_len)(source->getDrawableWidth()));
			}
			if (flagVertical) {
				setLayoutHeight((sl_ui_len)(source->getDrawableHeight()));
			}
		}
	}
	
	void ImageView::onAnimationFrame(Timer* timer)
	{
		invalidate();
	}

}
