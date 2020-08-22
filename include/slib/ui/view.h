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

#ifndef CHECKHEADER_SLIB_UI_VIEW
#define CHECKHEADER_SLIB_UI_VIEW

#include "definition.h"

#include "constants.h"
#include "event.h"
#include "cursor.h"

#include "../core/object.h"
#include "../core/function.h"
#include "../core/animation.h"
#include "../core/time.h"

#include "../graphics/canvas.h"

namespace slib
{

	class Window;
	class ViewInstance;
	
	class Timer;
	class Dispatcher;
	class Bitmap;
	
	class ViewPage;
	class ScrollBar;
	class MotionTracker;
	class GestureDetector;
	class GestureEvent;

	class SLIB_EXPORT View : public Object
	{
		SLIB_DECLARE_OBJECT
		
	public:
		View();
		
		~View();
		
	public:
		Ref<ViewInstance> getViewInstance();
		
		Ref<ViewInstance> getNativeWidget();
		
		sl_bool isInstance();

		sl_bool isValidInstance();
		
		sl_bool isCreatingInstance();
		
		// set before attaching
		void setCreatingInstance(sl_bool flag);
		
		sl_bool isCreatingChildInstances();
		
		// set before attaching
		void setCreatingChildInstances(sl_bool flag);

		sl_bool isSupportedNativeWidget();

		// set at constructor
		void setSupportedNativeWidget(sl_bool flag);

		sl_bool isCreatingNativeWidget();
		
		// set before attaching
		void setCreatingNativeWidget(sl_bool flag);
		
		sl_bool isCreatingNativeLayer();
		
		// set before attaching
		void setCreatingNativeLayer(sl_bool flag);

		sl_bool isCreatingLargeContent();
		
		// set before attaching
		void setCreatingLargeContent(sl_bool flag);
		
		sl_bool isCreatingEmptyContent();
		
		// set before attaching
		void setCreatingEmptyContent(sl_bool flag);
		
		UIAttachMode getAttachMode();
		
		// set before attaching
		void setAttachMode(UIAttachMode mode);

		sl_bool isNativeWidget();
		
		
		Ref<Window> getWindow();
		
		void setWindow(const Ref<Window>& window);
		
		Ref<View> getParent();

		void setParent(const Ref<View>& parent);
		
		Ref<ViewInstance> attachToNewInstance(ViewInstance* parent);
		
		
		String getId();
		
		void setId(const String& _id);
		
		List< Ref<View> > getChildren();
		
		sl_size getChildrenCount();
		
		Ref<View> getChild(sl_size index);
		
		void addChild(const Ref<View>& view, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		void insertChild(sl_size index, const Ref<View>& view, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		void removeChild(sl_size index, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		void removeChild(const Ref<View>& view, UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		void removeAllChildren(UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		Ref<View> getChildAt(sl_ui_pos x, sl_ui_pos y);
		
		Ref<View> getChildAt(const UIPoint& point);
		
		Ref<View> getTopmostViewAt(sl_ui_pos x, sl_ui_pos y);
		
		Ref<View> getTopmostViewAt(const UIPoint& point);
		
		Ref<View> findViewById(const String& _id);
		
		Ref<View> getRootView();
		
		sl_bool isRootView();
		
		Ref<View> getNearestViewWithInstance();
		
		Ref<ViewInstance> getNearestViewInstance();
		
		Ref<View> getNearestViewCreatingChildInstances();
		
		Ref<ViewPage> getNearestViewPage();
		
		void removeFromParent();
		
		void bringToFront(UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		
		virtual void invalidate(UIUpdateMode mode = UIUpdateMode::Redraw);

		// local coordinate
		virtual void invalidate(const UIRect& rect, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		void invalidateBoundsInParent(UIUpdateMode mode = UIUpdateMode::Redraw);
		
		void updateAndInvalidateBoundsInParent(UIUpdateMode mode = UIUpdateMode::Redraw);
		
		
		// parent coordinate
		const UIRect& getFrame();
		
		// parent coordinate
		virtual void setFrame(const UIRect& frame, UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		// parent coordinate
		void setFrame(sl_ui_pos x, sl_ui_pos y, sl_ui_len width, sl_ui_len height, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		// parent coordinate
		void requestFrame(const UIRect& frame, UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		sl_ui_len getWidth();
		
		void setWidth(sl_ui_len width, UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		sl_ui_len getHeight();
		
		void setHeight(sl_ui_len height, UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		UISize getSize();
		
		void setSize(const UISize& size, UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		void setSize(sl_ui_len width, sl_ui_len height, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		// parent coordinate
		sl_ui_pos getLeft();
		
		// parent coordinate
		void setLeft(sl_ui_pos x, UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		// parent coordinate
		sl_ui_pos getTop();
		
		// parent coordinate
		void setTop(sl_ui_pos y, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		// parent coordinate
		UIPoint getLocation();
		
		// parent coordinate
		void setLocation(const UIPoint& point, UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		// parent coordinate
		void setLocation(sl_ui_pos x, sl_ui_pos y, UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		UIRect getFrameInInstance();
		
		// local coordinate
		virtual UIRect getBounds();
		
		// local coordinate
		UIRect getBoundsInnerPadding();
		
		// local coordinate
		UIRect getBoundsIncludingShadow();
		
		// parent coordinate
		UIRect getBoundsInParent();
		
		// local coordinate
		sl_bool getVisibleBounds(UIRect* outBounds = sl_null);

		
		Visibility getVisibility();
		
		void setVisibility(Visibility visibility, UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		sl_bool isVisible();
		
		sl_bool isVisibleInInstance();
		
		void setVisible(sl_bool flagVisible, UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		sl_bool isEnabled();
		
		virtual void setEnabled(sl_bool flagEnabled, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		sl_bool isClipping();
		
		void setClipping(sl_bool flagClipping, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		sl_bool isDrawing();
		
		void setDrawing(sl_bool flagDrawing, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		sl_bool isRendering();
		
		void setRendering(sl_bool flagRendering);
		
		sl_bool isSavingCanvasState();
		
		void setSavingCanvasState(sl_bool flag);
		
		
		sl_bool isHitTestable();
		
		void setHitTestable(sl_bool flag);
		
		// local coordinate
		virtual sl_bool hitTest(sl_ui_pos x, sl_ui_pos y);
		
		// local coordinate
		sl_bool hitTest(const UIPoint& point);

		
		sl_bool isFocusable();
		
		void setFocusable(sl_bool flagFocusable);
		
		sl_bool isFocused();
		
		void setFocus(sl_bool flagFocused = sl_true, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		Ref<View> getFocusedChild();
		
		Ref<View> getFocusedDescendant();
		
		
		sl_bool isPressedState();
		
		virtual void setPressedState(sl_bool flagState, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		void cancelPressedState();
		
		void cancelPressedStateOfChildren();
		
		sl_bool isHoverState();
		
		virtual void setHoverState(sl_bool flagState, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		void cancelHoverState();
		
		void cancelHoverStateOfChildren();
		
		sl_bool isLockScroll();
		
		virtual void setLockScroll(sl_bool flagLock);
		
		sl_bool isCapturingEvents();
		
		void setCapturingEvents(sl_bool flag);
		

		Ref<Cursor> getCursor();
		
		void setCursor(const Ref<Cursor>& cursor);

		
		UISize measureLayoutWrappingSize(sl_bool flagHorizontalWrapping, sl_bool flagVerticalWrapping);
		
		void measureAndSetLayoutWrappingSize(sl_bool flagHorizontalWrapping, sl_bool flagVerticalWrapping);

		void updateLayoutFrameWithRequestedFrame();

		void setInvalidateLayoutFrameInParent();

		struct UpdateLayoutFrameParam
		{
			UIRect parentContentFrame;
			sl_bool flagUseLayout;
			sl_bool flagHorizontal;
			sl_bool flagVertical;
		};
		
		void updateLayoutFrameInParent(const UpdateLayoutFrameParam& param);

		sl_bool isCustomLayout();
		
		void setCustomLayout(sl_bool flagCustom);
		
		const UIRect& getRequestedFrame();
		
		UISize getRequestedSize();
		
		sl_ui_len getRequestedWidth();
		
		sl_ui_len getRequestedHeight();
		
		const UIRect& getLayoutFrame();
		
		void setLayoutFrame(const UIRect& frame);
		
		UISize getLayoutSize();
		
		void setLayoutSize(sl_ui_len width, sl_ui_len height);
		
		void setLayoutSize(const UISize& size);
		
		sl_ui_len getLayoutWidth();
		
		void setLayoutWidth(sl_ui_len width);
		
		sl_ui_len getLayoutHeight();
		
		void setLayoutHeight(sl_ui_len height);

		void invalidateLayout(UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		void invalidateParentLayout(UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		void invalidateSelfAndParentLayout(UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		void invalidateLayoutOfWrappingControl(UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		void forceUpdateLayout();

		SizeMode getWidthMode();
		
		SizeMode getHeightMode();
		
		sl_bool isWidthFixed();
		
		void setWidthFixed(UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		sl_bool isHeightFixed();
		
		void setHeightFixed(UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		sl_real getWidthWeight();
		
		sl_real getHeightWeight();
		
		sl_bool isWidthFilling();
		
		void setWidthFilling(sl_real weight = 1, UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		sl_bool isHeightFilling();
		
		void setHeightFilling(sl_real weight = 1, UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		sl_bool isWidthWrapping();
		
		void setWidthWrapping(UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		sl_bool isHeightWrapping();
		
		void setHeightWrapping(UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		sl_bool isWidthWeight();
		
		void setWidthWeight(sl_real weight = 1, UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		sl_bool isHeightWeight();
		
		void setHeightWeight(sl_real weight = 1, UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		sl_bool isLeftFree();
		
		void setLeftFree(UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		sl_bool isAlignParentLeft();
		
		void setAlignParentLeft(UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		sl_bool isAlignLeft();
		
		void setAlignLeft(const Ref<View>& view, UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		sl_bool isRightOf();
		
		void setRightOf(const Ref<View>& view, UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		Ref<View> getLayoutLeftReferingView();
		
		sl_bool isRightFree();
		
		void setRightFree(UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		sl_bool isAlignParentRight();
		
		void setAlignParentRight(UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		sl_bool isAlignRight();
		
		void setAlignRight(const Ref<View>& view, UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		sl_bool isLeftOf();
		
		void setLeftOf(const Ref<View>& view, UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		Ref<View> getLayoutRightReferingView();

		sl_bool isTopFree();
		
		void setTopFree(UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		sl_bool isAlignParentTop();
		
		void setAlignParentTop(UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		sl_bool isAlignTop();
		
		void setAlignTop(const Ref<View>& view, UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		sl_bool isBelow();
		
		void setBelow(const Ref<View>& view, UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		Ref<View> getLayoutTopReferingView();
		
		sl_bool isBottomFree();
		
		void setBottomFree(UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		sl_bool isAlignParentBottom();
		
		void setAlignParentBottom(UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		sl_bool isAlignBottom();
		
		void setAlignBottom(const Ref<View>& view, UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		sl_bool isAbove();
		
		void setAbove(const Ref<View>& view, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		Ref<View> getLayoutBottomReferingView();
		
		sl_bool isCenterHorizontal();
		
		void setCenterHorizontal(UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		sl_bool isCenterVertical();
		
		void setCenterVertical(UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		void setCenterInParent(UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		sl_bool isAlignCenterHorizontal();
		
		void setAlignCenterHorizontal(const Ref<View>& view, UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		sl_bool isAlignCenterVertical();
		
		void setAlignCenterVertical(const Ref<View>& view, UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		
		sl_ui_len getMinimumWidth();
		
		void setMinimumWidth(sl_ui_len width, UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		sl_bool isMaximumWidthDefined();

		sl_ui_len getMaximumWidth();
		
		void setMaximumWidth(sl_ui_len width, UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		sl_ui_len getMinimumHeight();
		
		void setMinimumHeight(sl_ui_len height, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		sl_bool isMaximumHeightDefined();

		sl_ui_len getMaximumHeight();
		
		void setMaximumHeight(sl_ui_len height, UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		AspectRatioMode getAspectRatioMode();
		
		void setAspectRatioMode(AspectRatioMode aspectRatioMode, UIUpdateMode updateMode = UIUpdateMode::UpdateLayout);
		
		sl_real getAspectRatio();
		
		void setAspectRatio(sl_real ratio, UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		
		sl_ui_pos getMarginLeft();
		
		void setMarginLeft(sl_ui_pos margin, UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		sl_ui_pos getMarginTop();
		
		void setMarginTop(sl_ui_pos margin, UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		sl_ui_pos getMarginRight();
		
		void setMarginRight(sl_ui_pos margin, UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		sl_ui_pos getMarginBottom();
		
		void setMarginBottom(sl_ui_pos margin, UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		void setMargin(sl_ui_pos left, sl_ui_pos top, sl_ui_pos right, sl_ui_pos bottom, UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		void setMargin(sl_ui_pos margin, UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		UIEdgeInsets getMargin();
		
		void setMargin(const UIEdgeInsets& margin, UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		sl_bool isMarginLeftFixed();
		
		sl_real getMarginLeftWeight();
		
		void setMarginLeftWeight(sl_real weight, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		sl_bool isMarginTopFixed();
		
		sl_real getMarginTopWeight();
		
		void setMarginTopWeight(sl_real weight, UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		sl_bool isMarginRightFixed();

		sl_real getMarginRightWeight();
		
		void setMarginRightWeight(sl_real weight, UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		sl_bool isMarginBottomFixed();
		
		sl_real getMarginBottomWeight();
		
		void setMarginBottomWeight(sl_real weight, UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		
		sl_ui_pos getPaddingLeft();
		
		void setPaddingLeft(sl_ui_pos padding, UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		sl_ui_pos getPaddingTop();
		
		void setPaddingTop(sl_ui_pos padding, UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		sl_ui_pos getPaddingRight();
		
		void setPaddingRight(sl_ui_pos padding, UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		sl_ui_pos getPaddingBottom();
		
		void setPaddingBottom(sl_ui_pos padding, UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		void setPadding(sl_ui_pos left, sl_ui_pos top, sl_ui_pos right, sl_ui_pos bottom, UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		void setPadding(sl_ui_pos padding, UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		UIEdgeInsets getPadding();
		
		void setPadding(const UIEdgeInsets& padding, UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		sl_bool isPaddingLeftFixed();
		
		sl_real getPaddingLeftWeight();
		
		void setPaddingLeftWeight(sl_real weight, UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		sl_bool isPaddingTopFixed();
		
		sl_real getPaddingTopWeight();
		
		void setPaddingTopWeight(sl_real weight, UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		sl_bool isPaddingRightFixed();
		
		sl_real getPaddingRightWeight();
		
		void setPaddingRightWeight(sl_real weight, UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		sl_bool isPaddingBottomFixed();
		
		sl_real getPaddingBottomWeight();
		
		void setPaddingBottomWeight(sl_real weight, UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		
		sl_bool isUsingChildLayouts();
		
		void setUsingChildLayouts(sl_bool flag);
		
		
		sl_bool getFinalTransform(Matrix3* _out);
		
		sl_bool getFinalInverseTransform(Matrix3* _out);
		
		Matrix3 getFinalTransformInInstance();
		
		const Matrix3& getTransform();
		
		void setTransform(const Matrix3& matrix, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		void resetTransform(UIUpdateMode mode = UIUpdateMode::Redraw);
				
		sl_real getTranslationX();
		
		sl_real getTranslationY();
		
		const Vector2& getTranslation();
		
		void setTranslationX(sl_real x, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		void setTranslationY(sl_real y, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		void setTranslation(sl_real x, sl_real y, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		void setTranslation(const Vector2& t, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		sl_real getScaleX();
		
		sl_real getScaleY();
		
		const Vector2& getScale();
		
		void setScaleX(sl_real sx, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		void setScaleY(sl_real sy, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		void setScale(sl_real x, sl_real y, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		void setScale(sl_real factor, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		void setScale(const Vector2& factor, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		sl_real getRotation();
		
		void setRotation(sl_real radian, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		sl_real getAnchorOffsetX();
		
		sl_real getAnchorOffsetY();

		const Vector2& getAnchorOffset();
		
		void setAnchorOffsetX(sl_real x, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		void setAnchorOffsetY(sl_real y, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		void setAnchorOffset(sl_real x, sl_real y, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		void setAnchorOffset(const Vector2& pt, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		// Call in UI Thread
		UIPointf convertCoordinateFromScreen(const UIPointf& ptScreen);
		
		// Call in UI Thread
		UIPointf convertCoordinateToScreen(const UIPointf& ptView);
		
		// Call in UI Thread
		UIPointf convertCoordinateFromParent(const UIPointf& ptParent);
		
		// Call in UI Thread
		UIRectf convertCoordinateFromParent(const UIRectf& rectParent);
		
		// Call in UI Thread
		UIPointf convertCoordinateToParent(const UIPointf& ptView);

		// Call in UI Thread
		UIRectf convertCoordinateToParent(const UIRectf& rectView);

		
		Ref<Drawable> getBackground();
		
		virtual void setBackground(const Ref<Drawable>& drawable, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		Color getBackgroundColor();
		
		void setBackgroundColor(const Color& color, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		Ref<Drawable> getPressedBackground();
		
		virtual void setPressedBackground(const Ref<Drawable>& drawable, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		Color getPressedBackgroundColor();
		
		void setPressedBackgroundColor(const Color& color, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		Ref<Drawable> getHoverBackground();
		
		virtual void setHoverBackground(const Ref<Drawable>& drawable, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		Color getHoverBackgroundColor();
		
		void setHoverBackgroundColor(const Color& color, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		ScaleMode getBackgroundScaleMode();
		
		virtual void setBackgroundScaleMode(ScaleMode scaleMode, UIUpdateMode updateMode = UIUpdateMode::Redraw);
		
		Alignment getBackgroundAlignment();
		
		virtual void setBackgroundAlignment(const Alignment& align, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		Ref<Pen> getBorder();
		
		virtual void setBorder(const Ref<Pen>& pen, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		PenStyle getBorderStyle();
		
		void setBorderStyle(PenStyle style, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		sl_real getBorderWidth();
		
		void setBorderWidth(sl_real width, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		Color getBorderColor();
		
		void setBorderColor(const Color& color, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		sl_bool isBorder();
		
		void setBorder(sl_bool flagBorder, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		BoundShape getBoundShape();
		
		void setBoundShape(BoundShape shape, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		const Size& getBoundRadius();
		
		void setBoundRadius(const Size& radius, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		void setBoundRadius(sl_real rx, sl_real ry, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		void setBoundRadiusX(sl_real rx, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		void setBoundRadiusY(sl_real ry, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		void setBoundRadius(sl_real radius, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		Ref<GraphicsPath> getBoundPath();
		
		void setBoundPath(const Ref<GraphicsPath>& path, UIUpdateMode mode = UIUpdateMode::Redraw);

		BoundShape getContentShape();
		
		void setContentShape(BoundShape shape, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		const Size& getContentRadius();
		
		void setContentRadius(const Size& radius, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		void setContentRadius(sl_real rx, sl_real ry, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		void setContentRadiusX(sl_real rx, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		void setContentRadiusY(sl_real ry, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		void setContentRadius(sl_real radius, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		Ref<GraphicsPath> getContentBoundPath();
		
		void setContentBoundPath(const Ref<GraphicsPath>& path, UIUpdateMode mode = UIUpdateMode::Redraw);

		
		Ref<Font> getFont();
		
		virtual void setFont(const Ref<Font>& font, UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		sl_real getFontSize();
		
		void setFontSize(sl_real size, UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		String getFontFamily();
		
		void setFontFamily(const String& fontFamily, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		sl_bool isUsingFont();
		
		void setUsingFont(sl_bool flag);
		
		
		sl_bool isOpaque();
		
		virtual void setOpaque(sl_bool flagOpaque, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		sl_real getAlpha();
		
		virtual void setAlpha(sl_real alpha, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_bool isLayer();
		
		void setLayer(sl_bool flagLayer, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		void invalidateLayer();
		
		void invalidateLayer(const UIRect& rect);
		
		sl_bool isForcedDraw();
		
		void forceDraw(sl_bool flagInvalidate = sl_true);
		
				
		float getShadowOpacity();
		
		virtual void setShadowOpacity(float alpha, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		sl_ui_posf getShadowRadius();
		
		virtual void setShadowRadius(sl_ui_posf radius, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		const UIPointf& getShadowOffset();
		
		virtual void setShadowOffset(const UIPointf& offset, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		void setShadowOffset(sl_ui_posf x, sl_ui_posf y, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		void setShadowOffsetX(sl_ui_posf x, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		void setShadowOffsetY(sl_ui_posf y, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		Color getShadowColor();
		
		virtual void setShadowColor(const Color& color, UIUpdateMode mode = UIUpdateMode::Redraw);

		
		virtual Ref<AnimationLoop> getAnimationLoop();
		
		Ref<Animation> createAnimation(float duration);

		Ref<Animation> createAnimation(const Ref<AnimationTarget>& target, float duration, const Function<void()>& onStop = sl_null, AnimationCurve curve = AnimationCurve::Default, const AnimationFlags& flags = AnimationFlags::Default);
		
		Ref<Animation> startAnimation(const Ref<AnimationTarget>& target, float duration, const Function<void()>& onStop = sl_null, AnimationCurve curve = AnimationCurve::Default, const AnimationFlags& flags = AnimationFlags::Default);

		
		Ref<Animation> getTransformAnimation();
		
		void setTransformAnimation(const Ref<Animation>& animation, const AnimationFrames<Matrix3>& frames);
		
		void setTransformAnimation(const Ref<Animation>& animation, const Matrix3& startValue, const Matrix3& endValue);
		
		void setTransformAnimation(const Ref<Animation>& animation, const Matrix3& toValue);
		
		Ref<Animation> createTransformAnimation(const AnimationFrames<Matrix3>& frames, float duration, const Function<void()>& onStop = sl_null, AnimationCurve curve = AnimationCurve::Default, const AnimationFlags& flags = AnimationFlags::Default);
		
		Ref<Animation> startTransformAnimation(const AnimationFrames<Matrix3>& frames, float duration, const Function<void()>& onStop = sl_null, AnimationCurve curve = AnimationCurve::Default, const AnimationFlags& flags = AnimationFlags::Default);
		
		Ref<Animation> createTransformAnimation(const Matrix3& startValue, const Matrix3& endValue, float duration, const Function<void()>& onStop = sl_null, AnimationCurve curve = AnimationCurve::Default, const AnimationFlags& flags = AnimationFlags::Default);
		
		Ref<Animation> startTransformAnimation(const Matrix3& startValue, const Matrix3& endValue, float duration, const Function<void()>& onStop = sl_null, AnimationCurve curve = AnimationCurve::Default, const AnimationFlags& flags = AnimationFlags::Default);
		
		Ref<Animation> createTransformAnimationTo(const Matrix3& toValue, float duration, const Function<void()>& onStop = sl_null, AnimationCurve curve = AnimationCurve::Default, const AnimationFlags& flags = AnimationFlags::Default);
		
		Ref<Animation> startTransformAnimationTo(const Matrix3& toValue, float duration, const Function<void()>& onStop = sl_null, AnimationCurve curve = AnimationCurve::Default, const AnimationFlags& flags = AnimationFlags::Default);

		
		Ref<Animation> getTranslateAnimation();
		
		void setTranslateAnimation(const Ref<Animation>& animation, const AnimationFrames<Vector2>& frames);
		
		void setTranslateAnimation(const Ref<Animation>& animation, const Vector2& startValue, const Vector2& endValue);
		
		void setTranslateAnimation(const Ref<Animation>& animation, const Vector2& toValue);
		
		Ref<Animation> createTranslateAnimation(const AnimationFrames<Vector2>& frames, float duration, const Function<void()>& onStop = sl_null, AnimationCurve curve = AnimationCurve::Default, const AnimationFlags& flags = AnimationFlags::Default);
		
		Ref<Animation> startTranslateAnimation(const AnimationFrames<Vector2>& frames, float duration, const Function<void()>& onStop = sl_null, AnimationCurve curve = AnimationCurve::Default, const AnimationFlags& flags = AnimationFlags::Default);
		
		Ref<Animation> createTranslateAnimation(const Vector2& startValue, const Vector2& endValue, float duration, const Function<void()>& onStop = sl_null, AnimationCurve curve = AnimationCurve::Default, const AnimationFlags& flags = AnimationFlags::Default);
		
		Ref<Animation> startTranslateAnimation(const Vector2& startValue, const Vector2& endValue, float duration, const Function<void()>& onStop = sl_null, AnimationCurve curve = AnimationCurve::Default, const AnimationFlags& flags = AnimationFlags::Default);
		
		Ref<Animation> createTranslateAnimationTo(const Vector2& toValue, float duration, const Function<void()>& onStop = sl_null, AnimationCurve curve = AnimationCurve::Default, const AnimationFlags& flags = AnimationFlags::Default);
		
		Ref<Animation> startTranslateAnimationTo(const Vector2& toValue, float duration, const Function<void()>& onStop = sl_null, AnimationCurve curve = AnimationCurve::Default, const AnimationFlags& flags = AnimationFlags::Default);
		
		
		Ref<Animation> getScaleAnimation();
		
		void setScaleAnimation(const Ref<Animation>& animation, const AnimationFrames<Vector2>& frames);
		
		void setScaleAnimation(const Ref<Animation>& animation, const Vector2& startValue, const Vector2& endValue);
		
		void setScaleAnimation(const Ref<Animation>& animation, const Vector2& toValue);
		
		void setScaleAnimation(const Ref<Animation>& animation, sl_real startValue, sl_real endValue);
		
		void setScaleAnimation(const Ref<Animation>& animation, sl_real toValue);
		
		Ref<Animation> createScaleAnimation(const AnimationFrames<Vector2>& frames, float duration, const Function<void()>& onStop = sl_null, AnimationCurve curve = AnimationCurve::Default, const AnimationFlags& flags = AnimationFlags::Default);
		
		Ref<Animation> startScaleAnimation(const AnimationFrames<Vector2>& frames, float duration, const Function<void()>& onStop = sl_null, AnimationCurve curve = AnimationCurve::Default, const AnimationFlags& flags = AnimationFlags::Default);
		
		Ref<Animation> createScaleAnimation(const Vector2& startValue, const Vector2& endValue, float duration, const Function<void()>& onStop = sl_null, AnimationCurve curve = AnimationCurve::Default, const AnimationFlags& flags = AnimationFlags::Default);
		
		Ref<Animation> startScaleAnimation(const Vector2& startValue, const Vector2& endValue, float duration, const Function<void()>& onStop = sl_null, AnimationCurve curve = AnimationCurve::Default, const AnimationFlags& flags = AnimationFlags::Default);

		Ref<Animation> createScaleAnimationTo(const Vector2& toValue, float duration, const Function<void()>& onStop = sl_null, AnimationCurve curve = AnimationCurve::Default, const AnimationFlags& flags = AnimationFlags::Default);
		
		Ref<Animation> startScaleAnimationTo(const Vector2& toValue, float duration, const Function<void()>& onStop = sl_null, AnimationCurve curve = AnimationCurve::Default, const AnimationFlags& flags = AnimationFlags::Default);
		
		Ref<Animation> createScaleAnimation(sl_real startValue, sl_real endValue, float duration, const Function<void()>& onStop = sl_null, AnimationCurve curve = AnimationCurve::Default, const AnimationFlags& flags = AnimationFlags::Default);
		
		Ref<Animation> startScaleAnimation(sl_real startValue, sl_real endValue, float duration, const Function<void()>& onStop = sl_null, AnimationCurve curve = AnimationCurve::Default, const AnimationFlags& flags = AnimationFlags::Default);
		
		Ref<Animation> createScaleAnimationTo(sl_real toValue, float duration, const Function<void()>& onStop = sl_null, AnimationCurve curve = AnimationCurve::Default, const AnimationFlags& flags = AnimationFlags::Default);
		
		Ref<Animation> startScaleAnimationTo(sl_real toValue, float duration, const Function<void()>& onStop = sl_null, AnimationCurve curve = AnimationCurve::Default, const AnimationFlags& flags = AnimationFlags::Default);
		
		
		Ref<Animation> getRotateAnimation();
		
		void setRotateAnimation(const Ref<Animation>& animation, const AnimationFrames<sl_real>& frames);
		
		void setRotateAnimation(const Ref<Animation>& animation, sl_real startValue, sl_real endValue);
		
		void setRotateAnimation(const Ref<Animation>& animation, sl_real toValue);
		
		Ref<Animation> createRotateAnimation(const AnimationFrames<sl_real>& frames, float duration, const Function<void()>& onStop = sl_null, AnimationCurve curve = AnimationCurve::Default, const AnimationFlags& flags = AnimationFlags::Default);
		
		Ref<Animation> startRotateAnimation(const AnimationFrames<sl_real>& frames, float duration, const Function<void()>& onStop = sl_null, AnimationCurve curve = AnimationCurve::Default, const AnimationFlags& flags = AnimationFlags::Default);
		
		Ref<Animation> createRotateAnimation(sl_real startValue, sl_real endValue, float duration, const Function<void()>& onStop = sl_null, AnimationCurve curve = AnimationCurve::Default, const AnimationFlags& flags = AnimationFlags::Default);
		
		Ref<Animation> startRotateAnimation(sl_real startValue, sl_real endValue, float duration, const Function<void()>& onStop = sl_null, AnimationCurve curve = AnimationCurve::Default, const AnimationFlags& flags = AnimationFlags::Default);

		Ref<Animation> createRotateAnimationTo(sl_real toValue, float duration, const Function<void()>& onStop = sl_null, AnimationCurve curve = AnimationCurve::Default, const AnimationFlags& flags = AnimationFlags::Default);
		
		Ref<Animation> startRotateAnimationTo(sl_real toValue, float duration, const Function<void()>& onStop = sl_null, AnimationCurve curve = AnimationCurve::Default, const AnimationFlags& flags = AnimationFlags::Default);

		
		Ref<Animation> getFrameAnimation();
		
		void setFrameAnimation(const Ref<Animation>& animation, const AnimationFrames<Rectangle>& frames);
		
		void setFrameAnimation(const Ref<Animation>& animation, const Rectangle& startValue, const Rectangle& endValue);
		
		void setFrameAnimation(const Ref<Animation>& animation, const Rectangle& toValue);
		
		Ref<Animation> createFrameAnimation(const AnimationFrames<Rectangle>& frames, float duration, const Function<void()>& onStop = sl_null, AnimationCurve curve = AnimationCurve::Default, const AnimationFlags& flags = AnimationFlags::Default);
		
		Ref<Animation> startFrameAnimation(const AnimationFrames<Rectangle>& frames, float duration, const Function<void()>& onStop = sl_null, AnimationCurve curve = AnimationCurve::Default, const AnimationFlags& flags = AnimationFlags::Default);
		
		Ref<Animation> createFrameAnimation(const Rectangle& startValue, const Rectangle& endValue, float duration, const Function<void()>& onStop = sl_null, AnimationCurve curve = AnimationCurve::Default, const AnimationFlags& flags = AnimationFlags::Default);
		
		Ref<Animation> startFrameAnimation(const Rectangle& startValue, const Rectangle& endValue, float duration, const Function<void()>& onStop = sl_null, AnimationCurve curve = AnimationCurve::Default, const AnimationFlags& flags = AnimationFlags::Default);
		
		Ref<Animation> createFrameAnimationTo(const Rectangle& toValue, float duration, const Function<void()>& onStop = sl_null, AnimationCurve curve = AnimationCurve::Default, const AnimationFlags& flags = AnimationFlags::Default);
		
		Ref<Animation> startFrameAnimationTo(const Rectangle& toValue, float duration, const Function<void()>& onStop = sl_null, AnimationCurve curve = AnimationCurve::Default, const AnimationFlags& flags = AnimationFlags::Default);
		
		
		Ref<Animation> getAlphaAnimation();
		
		void setAlphaAnimation(const Ref<Animation>& animation, const AnimationFrames<sl_real>& frames);
		
		void setAlphaAnimation(const Ref<Animation>& animation, sl_real startValue, sl_real endValue);
		
		void setAlphaAnimation(const Ref<Animation>& animation, sl_real toValue);
		
		Ref<Animation> createAlphaAnimation(const AnimationFrames<sl_real>& frames, float duration, const Function<void()>& onStop = sl_null, AnimationCurve curve = AnimationCurve::Default, const AnimationFlags& flags = AnimationFlags::Default);
		
		Ref<Animation> startAlphaAnimation(const AnimationFrames<sl_real>& frames, float duration, const Function<void()>& onStop = sl_null, AnimationCurve curve = AnimationCurve::Default, const AnimationFlags& flags = AnimationFlags::Default);
		
		Ref<Animation> createAlphaAnimation(sl_real startValue, sl_real endValue, float duration, const Function<void()>& onStop = sl_null, AnimationCurve curve = AnimationCurve::Default, const AnimationFlags& flags = AnimationFlags::Default);
		
		Ref<Animation> startAlphaAnimation(sl_real startValue, sl_real endValue, float duration, const Function<void()>& onStop = sl_null, AnimationCurve curve = AnimationCurve::Default, const AnimationFlags& flags = AnimationFlags::Default);
		
		Ref<Animation> createAlphaAnimationTo(sl_real toValue, float duration, const Function<void()>& onStop = sl_null, AnimationCurve curve = AnimationCurve::Default, const AnimationFlags& flags = AnimationFlags::Default);
		
		Ref<Animation> startAlphaAnimationTo(sl_real toValue, float duration, const Function<void()>& onStop = sl_null, AnimationCurve curve = AnimationCurve::Default, const AnimationFlags& flags = AnimationFlags::Default);

		
		Ref<Animation> getBackgroundColorAnimation();
		
		void setBackgroundColorAnimation(const Ref<Animation>& animation, const AnimationFrames<Color4f>& frames);
		
		void setBackgroundColorAnimation(const Ref<Animation>& animation, const Color4f& startValue, const Color4f& endValue);
		
		void setBackgroundColorAnimation(const Ref<Animation>& animation, const Color4f& toValue);
		
		Ref<Animation> createBackgroundColorAnimation(const AnimationFrames<Color4f>& frames, float duration, const Function<void()>& onStop = sl_null, AnimationCurve curve = AnimationCurve::Default, const AnimationFlags& flags = AnimationFlags::Default);
		
		Ref<Animation> startBackgroundColorAnimation(const AnimationFrames<Color4f>& frames, float duration, const Function<void()>& onStop = sl_null, AnimationCurve curve = AnimationCurve::Default, const AnimationFlags& flags = AnimationFlags::Default);
		
		Ref<Animation> createBackgroundColorAnimation(const Color4f& startValue, const Color4f& endValue, float duration, const Function<void()>& onStop = sl_null, AnimationCurve curve = AnimationCurve::Default, const AnimationFlags& flags = AnimationFlags::Default);
		
		Ref<Animation> startBackgroundColorAnimation(const Color4f& startValue, const Color4f& endValue, float duration, const Function<void()>& onStop = sl_null, AnimationCurve curve = AnimationCurve::Default, const AnimationFlags& flags = AnimationFlags::Default);
		
		Ref<Animation> createBackgroundColorAnimationTo(const Color4f& toValue, float duration, const Function<void()>& onStop = sl_null, AnimationCurve curve = AnimationCurve::Default, const AnimationFlags& flags = AnimationFlags::Default);
		
		Ref<Animation> startBackgroundColorAnimationTo(const Color4f& toValue, float duration, const Function<void()>& onStop = sl_null, AnimationCurve curve = AnimationCurve::Default, const AnimationFlags& flags = AnimationFlags::Default);
		
		
		sl_bool isHorizontalScrolling();
		
		sl_bool isVerticalScrolling();

		void setHorizontalScrolling(sl_bool flagHorizontal, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		void setVerticalScrolling(sl_bool flagVertical, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		virtual void setScrolling(sl_bool flagHorizontal, sl_bool flagVertical, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		Ref<ScrollBar> getHorizontalScrollBar();
		
		Ref<ScrollBar> getVerticalScrollBar();
		
		void setHorizontalScrollBar(const Ref<ScrollBar>& bar, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		void setVerticalScrollBar(const Ref<ScrollBar>& bar, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		sl_bool isHorizontalScrollBarVisible();
		
		sl_bool isVerticalScrollBarVisible();
		
		virtual void setScrollBarsVisible(sl_bool flagHorizontal, sl_bool flagVertical, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		void setHorizontalScrollBarVisible(sl_bool flagVisible, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		void setVerticalScrollBarVisible(sl_bool flagVisible, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_bool isAutoHideScrollBar();

		void setAutoHideScrollBar(sl_bool flag);

		sl_bool isCanvasScrolling();

		void setCanvasScrolling(sl_bool flag);

		virtual Pointlf getScrollPosition();
		
		sl_scroll_pos getScrollX();
		
		sl_scroll_pos getScrollY();
		
		virtual void scrollTo(sl_scroll_pos x, sl_scroll_pos y, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		void scrollTo(const Pointlf& position, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		void scrollToX(sl_scroll_pos x, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		void scrollToY(sl_scroll_pos y, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		virtual void smoothScrollTo(sl_scroll_pos x, sl_scroll_pos y, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		void smoothScrollTo(const Pointlf& position, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		void smoothScrollToX(sl_scroll_pos x, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		void smoothScrollToY(sl_scroll_pos y, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		void scrollToEndX(UIUpdateMode mode = UIUpdateMode::Redraw);
		
		void scrollToEndY(UIUpdateMode mode = UIUpdateMode::Redraw);
		
		void smoothScrollToEndX(UIUpdateMode mode = UIUpdateMode::Redraw);
		
		void smoothScrollToEndY(UIUpdateMode mode = UIUpdateMode::Redraw);
		
		void setScrollX(sl_scroll_pos x, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		void setScrollY(sl_scroll_pos y, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		sl_scroll_pos getContentWidth();
		
		sl_scroll_pos getContentHeight();
		
		ScrollPoint getContentSize();
		
		virtual void setContentSize(sl_scroll_pos width, sl_scroll_pos height, UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		void setContentSize(const ScrollPoint& size, UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		void setContentWidth(sl_scroll_pos width, UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		void setContentHeight(sl_scroll_pos height, UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		virtual ScrollPoint getScrollRange();
		
		sl_ui_len getScrollBarWidth();
		
		void setScrollBarWidth(sl_ui_len width, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		void refreshScroll(UIUpdateMode mode = UIUpdateMode::Redraw);
		
		sl_bool isPaging();
		
		void setPaging(sl_bool flagPaging);
		
		sl_ui_len getPageWidth();
		
		void setPageWidth(sl_ui_len width);
		
		sl_ui_len getPageHeight();
		
		void setPageHeight(sl_ui_len height);
		
		
		sl_bool isContentScrollingByMouse();
		
		void setContentScrollingByMouse(sl_bool flag);
		
		sl_bool isContentScrollingByTouch();
		
		void setContentScrollingByTouch(sl_bool flag);
		
		sl_bool isContentScrollingByMouseWheel();
		
		void setContentScrollingByMouseWheel(sl_bool flag);
		
		sl_bool isContentScrollingByKeyboard();
		
		void setContentScrollingByKeyboard(sl_bool flag);
		
		sl_bool isSmoothContentScrolling();
		
		void setSmoothContentScrolling(sl_bool flag);

		
		sl_bool isTouchMultipleChildren();
		
		void setTouchMultipleChildren(sl_bool flag);
		
		sl_bool isPassingEventsToChildren();
		
		void setPassingEventsToChildren(sl_bool flag);
		
		
		sl_bool isOkCancelEnabled();
		
		void setOkCancelEnabled(sl_bool flag);
		
		void setOkOnClick();
		
		void setCancelOnClick();

		
		Ref<View> getNextFocusableView();
		
		Ref<View> getPreviousFocusableView();
		
		Ref<View> getFirstFocusableDescendant();
		
		Ref<View> getLastFocusableDescendant();
		
		sl_bool isTabStopEnabled();
		
		void setTabStopEnabled(sl_bool flag);
		
		Ref<View> getNextTabStop();
		
		void setNextTabStop(const Ref<View>& view);
		
		Ref<View> getPreviousTabStop();
		
		void setPreviousTabStop(const Ref<View>& view);

		char getMnemonicKey();

		void setMnemonicKey(char ch);

		Ref<View> findViewByMnemonicKey(char ch);


		sl_bool isKeepKeyboard();
		
		void setKeepKeyboard(sl_bool flag);

		
		sl_bool isDraggable();
		
		void setDraggable(sl_bool flag = sl_true);
		
		sl_bool isDroppable();
		
		void setDroppable(sl_bool flag = sl_true);
		
		const DragItem& getDragItem();
		
		void setDragItem(const DragItem& item);
		
		DragOperations getDragOperationMask();
		
		void setDragOperationMask(const DragOperations& mask);
		
		void beginDragging(const DragItem& item, DragOperations operationMask = DragOperations::All);
		

		sl_bool isPlaySoundOnClick();

		void setPlaySoundOnClick(sl_bool flag = sl_true);


		sl_bool isClientEdge();

		void setClientEdge(sl_bool flag);

		
		Function<sl_bool(const UIPoint& pt)> getCapturingChildInstanceEvents();
		
		void setCapturingChildInstanceEvents(const Function<sl_bool(const UIPoint& pt)>& hitTestCapture);
		
		
		Ref<UIEvent> getCurrentEvent();
		
		void setCurrentEvent(UIEvent* ev);
		
		
		Ref<GestureDetector> createGestureDetector();
		
		Ref<GestureDetector> getGestureDetector();
		
		
		Ref<Drawable> getCurrentBackground();
		
		void drawBackground(Canvas* canvas, const Ref<Drawable>& background);
		
		void drawBorder(Canvas* canvas, const Ref<Pen>& pen);
		
		void drawChildren(Canvas* canvas, const Ref<View>* children, sl_size count);
		
		virtual void renderChildren(Canvas* canvas, const Ref<View>* children, sl_size count);
		
		void drawContent(Canvas* canvas);
		
		Ref<Bitmap> drawLayer();
		
		void draw(Canvas* canvas);
		
		sl_bool drawLayerShadow(Canvas* canvas);
		
		void drawBoundShadow(Canvas* canvas);
		
		void clipBounds(Canvas* canvas);
		
		void clipContentBounds(Canvas* canvas);
		
		virtual Size measureText(const String& size, const Ref<Font>& font, sl_bool flagMultiLine = sl_false);
		
		
		void runAfterDraw(const Function<void()>& callback, sl_bool flagInvalidate = sl_true);
		
		virtual sl_bool isDrawingThread();
		
		virtual void dispatchToDrawingThread(const Function<void()>& callback, sl_uint32 delayMillis = 0);
		
		virtual void runOnDrawingThread(const Function<void()>& callback);
		
		static sl_bool isUiThread();
		
		static void dispatchToUiThread(const Function<void()>& callback, sl_uint32 delayMillis = 0);

		static void runOnUiThread(const Function<void()>& callback);
		
		virtual Ref<Dispatcher> getDispatcher();
		
		Ref<Timer> createTimer(const Function<void(Timer*)>& task, sl_uint32 interval_ms);
		
		Ref<Timer> startTimer(const Function<void(Timer*)>& task, sl_uint32 interval_ms);
		
	protected:
		virtual Ref<ViewInstance> createGenericInstance(ViewInstance* parent);
		
		virtual Ref<ViewInstance> createNativeWidget(ViewInstance* parent);
		
	protected:
		virtual void onChangeParent(View* oldParent, View* newParent);
		
		virtual void onAddChild(View* child);
		
		virtual void onRemoveChild(View* child);

		virtual void onAttachChild(View* child);
		
		virtual void onDetachChild(View* child);
		
		virtual void onUpdateLayout();

		virtual void onUpdateFont(const Ref<Font>& font);
		
		virtual void onChangePadding();
		
		virtual void onUpdatePaging();
		
		virtual void onDrawBackground(Canvas* canvas);
		
		virtual void onDrawBorder(Canvas* canvas);

		virtual void onResizeChild(View* child, sl_ui_len width, sl_ui_len height);

		virtual void onChangeVisibilityOfChild(View* child, Visibility oldVisibility, Visibility newVisibility);

		virtual void onResizeContent(sl_scroll_pos width, sl_scroll_pos height);
		
	public:
		SLIB_DECLARE_EVENT_HANDLER_FUNCTIONS(View, Attach)
		SLIB_DECLARE_EVENT_HANDLER_FUNCTIONS(View, Detach)
		
		SLIB_DECLARE_EVENT_HANDLER_FUNCTIONS(View, Draw, Canvas* canvas)
		SLIB_DECLARE_EVENT_HANDLER_FUNCTIONS_WITHOUT_ON(View, PreDraw, Canvas* canvas)
		SLIB_DECLARE_EVENT_HANDLER_FUNCTIONS_WITHOUT_ON(View, PostDraw, Canvas* canvas)
		SLIB_DECLARE_EVENT_HANDLER_FUNCTIONS(View, DrawShadow, Canvas* canvas)

		SLIB_DECLARE_EVENT_HANDLER_FUNCTIONS(View, MouseEvent, UIEvent* ev)
		sl_bool dispatchMouseEventToChildren(UIEvent* ev, const Ref<View>* children, sl_size count);
		void dispatchMouseEventToChild(UIEvent* ev, View* child, sl_bool flagTransformPoints = sl_true);

		SLIB_DECLARE_EVENT_HANDLER_FUNCTIONS(View, TouchEvent, UIEvent* ev)
		sl_bool dispatchTouchEventToChildren(UIEvent* ev, const Ref<View>* children, sl_size count);
		void dispatchTouchEventToMultipleChildren(UIEvent* ev, const Ref<View>* children, sl_size count);
		void dispatchTouchEventToChild(UIEvent* ev, View* child, sl_bool flagTransformPoints = sl_true);

		SLIB_DECLARE_EVENT_HANDLER_FUNCTIONS(View, MouseWheelEvent, UIEvent* ev)
		sl_bool dispatchMouseWheelEventToChildren(UIEvent* ev, const Ref<View>* children, sl_size count);
		void dispatchMouseWheelEventToChild(UIEvent* ev, View* child, sl_bool flagTransformPoints = sl_true);

		SLIB_DECLARE_EVENT_HANDLER_FUNCTIONS(View, KeyEvent, UIEvent* ev)
		
		SLIB_DECLARE_EVENT_HANDLER_FUNCTIONS_WITHOUT_ON(View, Click)
		void dispatchClick();
		SLIB_DECLARE_EVENT_HANDLER_FUNCTIONS(View, ClickEvent, UIEvent* ev)

		SLIB_DECLARE_EVENT_HANDLER_FUNCTIONS(View, SetCursor, UIEvent* ev)
		sl_bool dispatchSetCursorToChildren(UIEvent* ev, const Ref<View>* children, sl_size count);
		void dispatchSetCursorToChild(UIEvent* ev, View* child, sl_bool flagTransformPoints = sl_true);
		
		SLIB_DECLARE_EVENT_HANDLER_FUNCTIONS(View, DragEvent, UIEvent* ev)
		SLIB_DECLARE_EVENT_HANDLER_FUNCTIONS(View, DropEvent, UIEvent* ev)
		sl_bool dispatchDropEventToChildren(UIEvent* ev, const Ref<View>* children, sl_size count);
		void dispatchDropEventToChild(UIEvent* ev, View* child, sl_bool flagTransformPoints = sl_true);

		SLIB_DECLARE_EVENT_HANDLER_FUNCTIONS(View, ChangeFocus, sl_bool flagFocused)

		SLIB_DECLARE_EVENT_HANDLER_FUNCTIONS(View, Move, sl_ui_pos x, sl_ui_pos y)

		SLIB_DECLARE_EVENT_HANDLER_FUNCTIONS(View, Resize, sl_ui_len width, sl_ui_len height)
		
		SLIB_DECLARE_EVENT_HANDLER_FUNCTIONS(View, ChangeVisibility, Visibility oldVisibility, Visibility newVisibility)
		
		SLIB_DECLARE_EVENT_HANDLER_FUNCTIONS(View, Scroll, sl_scroll_pos x, sl_scroll_pos y)
		
		SLIB_DECLARE_EVENT_HANDLER_FUNCTIONS(View, Swipe, GestureEvent* ev)
		
		SLIB_DECLARE_EVENT_HANDLER_FUNCTIONS(View, OK, UIEvent* ev)
		void dispatchOK();

		SLIB_DECLARE_EVENT_HANDLER_FUNCTIONS(View, Cancel, UIEvent* ev)
		void dispatchCancel();

		SLIB_DECLARE_EVENT_HANDLER_FUNCTIONS(View, Mnemonic, UIEvent* ev)

	private:
		void _removeParent(View* parent = sl_null);

		void _attach(const Ref<ViewInstance>& instance);
		
		void _detach();
		
		void _detachAll();
		
		void _doAttach();
		
		Ref<ViewInstance> _createInstance(ViewInstance* parent);
		
		void _addChild(View* child, View* viewCreatingChildInstances, UIUpdateMode mode);

		void _removeChild(View* child);
		
		void _removeChildInstances(View* child);
		
		void _removeAllViewInstances();
		
		void _attachChild(const Ref<View>& child);
		
		void _invalidate(const UIRect& rect, sl_bool flagDrawOutside, UIUpdateMode mode);
		
		void _setInstanceVisible(sl_bool flagVisible);
		
		void _updateInstanceFrames();
		
		void _setFocus(sl_bool flagFocused, sl_bool flagApplyInstance, UIUpdateMode mode);
		
		void _setFocusedFlag(sl_bool flagFocused, sl_bool flagApplyInstance);
		
		void _killChildFocus();
		
		void _setFocusedChild(View* child, UIUpdateMode mode);
		
		void _restrictSize(sl_ui_len& width, sl_ui_len& height);
		
		void _restrictSize(UIRect& rect);
		
		UIRect _updateLayoutFrameInParent_getReferFrame(const UpdateLayoutFrameParam& param, View* refer);

		void _updateLayout();
		
		void _applyLayout(UIUpdateMode mode);
		
		void _updateAndApplyChildLayout(View* child);

		void _updateAndApplyLayoutWithMode(UIUpdateMode mode);

		void _updateAndApplyLayout();
		
		void _setInvalidateLayout();
		
		sl_ui_len _measureLayoutWrappingSize_Horz(View* view, Pair<sl_ui_len, sl_ui_len>& insets, HashMap< View*, Pair<sl_ui_len, sl_ui_len> >& map, sl_ui_pos paddingLeft, sl_ui_pos paddingRight);
		
		sl_ui_len _measureLayoutWrappingSize_Vert(View* view, Pair<sl_ui_len, sl_ui_len>& insets, HashMap< View*, Pair<sl_ui_len, sl_ui_len> >& map, sl_ui_pos paddingTop, sl_ui_pos paddingBottom);

		void _updateInstanceTransforms();
		
		void _applyCalcTransform(UIUpdateMode mode);
		
		void _applyFinalTransform(UIUpdateMode mode);
		
		void _refreshBorderPen(UIUpdateMode mode);
		
		void _setFontInvalidateChildren(const Ref<Font>& font);
		
		void _setInstanceFont(const Ref<Font>& font);
		
		void _setInstancePadding();
		
		
		void _attachNativeAnimations();
		
		void _attachNativeAnimation(const Ref<Animation>& animation);

		
		void _getScrollBars(Ref<View> views[2]);
		
		void _initScrollBars(UIUpdateMode mode);
		
		void _onScrollBarChangeValue(ScrollBar* scrollBar, sl_scroll_pos value);
				
		sl_bool _scrollTo(sl_scroll_pos x, sl_scroll_pos y, sl_bool flagPreprocess, sl_bool flagFinish, sl_bool flagAnimate);


		Ref<View> _findViewByMnemonicKey(char ch);


		void _processEventForStateAndClick(UIEvent* ev);
		
		void _processContentScrollingEvents(UIEvent* ev);
		
		void _startContentScrollingFlow(sl_bool flagSmoothTarget, const Pointlf& speedOrTarget);
		
		void _stopContentScrollingFlow();
		
		void _processContentScrollingFlow(Timer* timer);

		void _processAutoHideScrollBar(UIEvent* ev);
		
		void _setInstancePaging();
		
	protected:
		virtual void _onScroll_NW(sl_scroll_pos x, sl_scroll_pos y);
		
	private:
		AtomicRef<ViewInstance> m_instance;
		AtomicWeakRef<Window> m_window;
		AtomicWeakRef<View> m_parent;
		
		sl_bool m_flagCreatingInstance : 1;
		sl_bool m_flagCreatingChildInstances : 1;
		sl_bool m_flagSupportedNativeWidget : 1;
		sl_bool m_flagCreatingNativeWidget : 1;
		sl_bool m_flagCreatingNativeLayer : 1;
		sl_bool m_flagCreatingLargeContent: 1;
		sl_bool m_flagCreatingEmptyContent: 1;
		sl_bool m_flagUsingChildLayouts : 1;
		sl_bool m_flagEnabled : 1;
		sl_bool m_flagHitTestable : 1;
		sl_bool m_flagFocusable : 1;
		sl_bool m_flagClipping : 1;
		sl_bool m_flagDrawing : 1;
		sl_bool m_flagRendering : 1;
		sl_bool m_flagSavingCanvasState : 1;
		sl_bool m_flagOkCancelEnabled : 1;
		sl_bool m_flagTabStopEnabled : 1;
		sl_bool m_flagKeepKeyboard : 1;
		sl_bool m_flagDraggable : 1;
		sl_bool m_flagDroppable : 1;
		sl_bool m_flagPlaySoundOnClick : 1;
		sl_bool m_flagClientEdge : 1;

		sl_bool m_flagCurrentCreatingInstance : 1;
		sl_bool m_flagInvalidLayout : 1;
		sl_bool m_flagNeedApplyLayout : 1;
		sl_bool m_flagFocused : 1;
		sl_bool m_flagPressed : 1;
		sl_bool m_flagHover : 1;
		sl_bool m_flagLockScroll: 1;
		sl_bool m_flagCaptureEvents: 1;
		sl_bool m_flagClicking : 1;
		
		AtomicString m_id;
		UIAttachMode m_attachMode;
		Visibility m_visibility;

		UIRect m_frame;
		UIRect m_boundsInParent;
		sl_int32 m_idUpdateInvalidateLayout;
		
		UIAction m_actionMouseDown;
		AtomicRef<UIEvent> m_currentEvent;

	protected:
		class LayoutAttributes : public Referable
		{
		public:
			sl_bool flagMarginLeftWeight : 1;
			sl_bool flagMarginTopWeight : 1;
			sl_bool flagMarginRightWeight : 1;
			sl_bool flagMarginBottomWeight : 1;
			sl_bool flagCustomLayout : 1;
			
			sl_bool flagInvalidLayoutInParent : 1;
			sl_bool flagRequestedFrame : 1;

			UIRect layoutFrame;
			UIRect requestedFrame;

			SizeMode widthMode;
			SizeMode heightMode;
			sl_real widthWeight;
			sl_real heightWeight;
			
			PositionMode leftMode;
			PositionMode topMode;
			PositionMode rightMode;
			PositionMode bottomMode;
			AtomicWeakRef<View> leftReferingView;
			AtomicWeakRef<View> topReferingView;
			AtomicWeakRef<View> rightReferingView;
			AtomicWeakRef<View> bottomReferingView;
			
			sl_ui_len minWidth;
			sl_ui_len maxWidth;
			sl_ui_len minHeight;
			sl_ui_len maxHeight;
			AspectRatioMode aspectRatioMode;
			sl_real aspectRatio;
			
			sl_ui_pos marginLeft;
			sl_ui_pos marginTop;
			sl_ui_pos marginRight;
			sl_ui_pos marginBottom;
			sl_real marginLeftWeight;
			sl_real marginTopWeight;
			sl_real marginRightWeight;
			sl_real marginBottomWeight;
			
		public:
			LayoutAttributes();
			
			~LayoutAttributes();
			
			SLIB_DELETE_CLASS_DEFAULT_MEMBERS(LayoutAttributes)
			
		public:
			void applyMarginWeightsX(sl_ui_pos parentWidth);
			void applyMarginWeightsY(sl_ui_pos parentHeight);
			void applyMarginWeights(sl_ui_pos parentWidth, sl_ui_pos parentHeight);

		};
		Ref<LayoutAttributes> m_layoutAttrs;
		
		void _initializeLayoutAttributes();
		
		class PaddingAttributes : public Referable
		{
		public:
			sl_bool flagPaddingLeftWeight : 1;
			sl_bool flagPaddingTopWeight : 1;
			sl_bool flagPaddingRightWeight : 1;
			sl_bool flagPaddingBottomWeight : 1;

			sl_ui_pos paddingLeft;
			sl_ui_pos paddingTop;
			sl_ui_pos paddingRight;
			sl_ui_pos paddingBottom;
			sl_real paddingLeftWeight;
			sl_real paddingTopWeight;
			sl_real paddingRightWeight;
			sl_real paddingBottomWeight;
			
		public:
			PaddingAttributes();
			
			~PaddingAttributes();
			
			SLIB_DELETE_CLASS_DEFAULT_MEMBERS(PaddingAttributes)

		public:
			void applyPaddingWeightsX(sl_ui_pos width);
			void applyPaddingWeightsY(sl_ui_pos height);
			void applyPaddingWeights(sl_ui_pos width, sl_ui_pos height);
			
		};
		Ref<PaddingAttributes> m_paddingAttrs;
		
		void _initializePaddingAttributes();

		class TransformAttributes : public Referable
		{
		public:
			sl_bool flagTransformFinalInvalid : 1;
			sl_bool flagTransformFinal : 1;
			sl_bool flagInverseTransformFinalInvalid : 1;
			sl_bool flagInverseTransformFinal : 1;
			sl_bool flagTransform : 1;
			sl_bool flagTransformCalcInvalid : 1;
			sl_bool flagTransformCalc : 1;

			Matrix3 transformFinal;
			Matrix3 inverseTransformFinal;
			Matrix3 transform;
			Matrix3 transformCalc;
			Vector2 translation;
			Vector2 scale;
			sl_real rotationAngle;
			Vector2 anchorOffset;
			
			AtomicWeakRef<Animation> m_animationTransform;
			AtomicWeakRef<Animation> m_animationTranslate;
			AtomicWeakRef<Animation> m_animationScale;
			AtomicWeakRef<Animation> m_animationRotate;
			AtomicWeakRef<Animation> m_animationFrame;
			AtomicWeakRef<Animation> m_animationAlpha;
			AtomicWeakRef<Animation> m_animationBackgroundColor;

		public:
			TransformAttributes();
			
			~TransformAttributes();
			
			SLIB_DELETE_CLASS_DEFAULT_MEMBERS(TransformAttributes)

		};
		Ref<TransformAttributes> m_transformAttrs;
		
		void _initializeTransformAttributes();
		
		class DrawAttributes : public Referable
		{
		public:
			sl_bool flagUsingFont : 1;
			sl_bool flagOpaque : 1;
			sl_bool flagLayer : 1;
			
			sl_bool flagForcedDraw : 1;
			sl_bool flagInvalidatedLayer : 1;
			sl_bool flagInvalidatedWholeLayer : 1;

			AtomicRef<Drawable> background;
			AtomicRef<Drawable> backgroundPressed;
			AtomicRef<Drawable> backgroundHover;
			ScaleMode backgroundScaleMode;
			Alignment backgroundAlignment;
			
			AtomicRef<Pen> penBorder;
			PenStyle borderStyle;
			sl_real borderWidth;
			Color borderColor;
			
			BoundShape boundShape;
			Size boundRadius;
			AtomicRef<GraphicsPath> boundPath;
			
			BoundShape contentShape;
			Size contentRadius;
			AtomicRef<GraphicsPath> contentBoundPath;

			AtomicRef<Font> font;
			sl_real alpha;
			
			AtomicRef<Bitmap> bitmapLayer;
			AtomicRef<Canvas> canvasLayer;
			UIRect rectInvalidatedLayer;
			
			float shadowOpacity;
			sl_ui_posf shadowRadius;
			UIPointf shadowOffset;
			Color shadowColor;
			
			LinkedList< Function<void()> > runAfterDrawCallbacks;
			
		public:
			DrawAttributes();
			
			~DrawAttributes();
			
			SLIB_DELETE_CLASS_DEFAULT_MEMBERS(DrawAttributes)

		};
		Ref<DrawAttributes> m_drawAttrs;
		
		void _initializeDrawAttributes();

		class ScrollAttributes : public Referable
		{
		public:
			sl_bool flagHorz : 1;
			sl_bool flagVert : 1;
			sl_bool flagHorzScrollBarVisible : 1;
			sl_bool flagVertScrollBarVisible : 1;
			sl_bool flagPaging : 1;
			sl_bool flagContentScrollingByMouse : 1;
			sl_bool flagContentScrollingByTouch : 1;
			sl_bool flagContentScrollingByMouseWheel : 1;
			sl_bool flagContentScrollingByKeyboard : 1;
			sl_bool flagSmoothContentScrolling : 1;
			sl_bool flagAutoHideScrollBar : 1;
			sl_bool flagScrollCanvas : 1;
			
			sl_bool flagValidHorz : 1;
			sl_bool flagValidVert : 1;
			sl_bool flagInitHorzScrollBar : 1;
			sl_bool flagInitVertScrollBar : 1;
			sl_bool flagDownContent : 1;

			AtomicRef<ScrollBar> horz;
			AtomicRef<ScrollBar> vert;
			sl_scroll_pos x;
			sl_scroll_pos y;
			sl_scroll_pos contentWidth;
			sl_scroll_pos contentHeight;
			sl_ui_len barWidth;
			sl_ui_pos pageWidth;
			sl_ui_pos pageHeight;
			
			Point mousePointDown;
			Point mousePointBefore;
			sl_uint64 touchPointerIdBefore;
			MotionTracker motionTracker;
			Ref<Timer> timerFlow;
			Time timeFlowFrameBefore;
			Point speedFlow;
			sl_bool flagSmoothTarget;
			sl_scroll_pos xSmoothTarget;
			sl_scroll_pos ySmoothTarget;
			Time timeLastInside;
			
		public:
			ScrollAttributes();
			
			~ScrollAttributes();
			
			SLIB_DELETE_CLASS_DEFAULT_MEMBERS(ScrollAttributes)

		};
		Ref<ScrollAttributes> m_scrollAttrs;
		
		void _initializeScrollAttributes();
		
		class ChildAttributes : public Referable
		{
		public:
			sl_bool flagTouchMultipleChildren : 1;
			sl_bool flagPassEventToChildren : 1;
			
			sl_bool flagHasInstances : 1;
			
			AtomicList< Ref<View> > children;
			AtomicList< Ref<View> > childrenCache;
			
			List< Ref<View> > childrenMultiTouch;
			AtomicRef<View> childMouseMove;
			AtomicRef<View> childMouseDown;
			AtomicRef<View> childDragOver;
			AtomicRef<View> childFocused;

			AtomicFunction<sl_bool(const UIPoint& pt)> hitTestCapturingChildInstanceEvents;
			
		public:
			ChildAttributes();
			
			~ChildAttributes();
			
			SLIB_DELETE_CLASS_DEFAULT_MEMBERS(ChildAttributes)

		};
		Ref<ChildAttributes> m_childAttrs;

		void _initializeChildAttributes();

		class OtherAttributes : public Referable
		{
		public:
			AtomicWeakRef<View> viewNextTabStop;
			AtomicWeakRef<View> viewPrevTabStop;
			AtomicRef<Cursor> cursor;
			AtomicRef<GestureDetector> gestureDetector;
			AtomicPtr<DragItem> dragItem;
			DragOperations dragOperationMask;
			char mnemonicKey;

		public:
			OtherAttributes();
			
			~OtherAttributes();
			
			SLIB_DELETE_CLASS_DEFAULT_MEMBERS(OtherAttributes)

		};
		Ref<OtherAttributes> m_otherAttrs;
		
		void _initializeOtherAttributes();
		
		class EventAttributes : public Referable
		{
		public:
			AtomicFunction<void(View*)> onAttach;
			AtomicFunction<void(View*)> onDetach;
			AtomicFunction<void(View*, Canvas*)> onDraw;
			AtomicFunction<void(View*, Canvas*)> onPreDraw;
			AtomicFunction<void(View*, Canvas*)> onPostDraw;
			AtomicFunction<void(View*, Canvas*)> onDrawShadow;
			AtomicFunction<void(View*, UIEvent*)> onMouseEvent;
			AtomicFunction<void(View*, UIEvent*)> onTouchEvent;
			AtomicFunction<void(View*, UIEvent*)> onKeyEvent;
			AtomicFunction<void(View*, UIEvent*)> onMouseWheelEvent;
			AtomicFunction<void(View*)> onClick;
			AtomicFunction<void(View*, UIEvent*)> onClickEvent;
			AtomicFunction<void(View*, UIEvent*)> onSetCursor;
			AtomicFunction<void(View*, UIEvent*)> onDragEvent;
			AtomicFunction<void(View*, UIEvent*)> onDropEvent;
			AtomicFunction<void(View*, sl_bool flagFocused)> onChangeFocus;
			AtomicFunction<void(View*, sl_ui_pos, sl_ui_pos)> onMove;
			AtomicFunction<void(View*, sl_ui_len, sl_ui_len)> onResize;
			AtomicFunction<void(View*, Visibility, Visibility)> onChangeVisibility;
			AtomicFunction<void(View*, sl_scroll_pos, sl_scroll_pos)> onScroll;
			AtomicFunction<void(View*, GestureEvent*)> onSwipe;
			AtomicFunction<void(View*, UIEvent*)> onOK;
			AtomicFunction<void(View*, UIEvent*)> onCancel;
			AtomicFunction<void(View*, UIEvent*)> onMnemonic;

		public:
			EventAttributes();
			
			~EventAttributes();
			
		};
		
		Ref<EventAttributes> m_eventAttrs;
		
		void _initializeEventAttributes();

		friend class ViewInstance;
		friend class Window;
		friend class RenderView;
		friend class LinearLayout;
		friend class ListView;
		friend class CollectionView;
		friend class TableLayout;

	};

	class SLIB_EXPORT ViewInstance : public Object
	{
		SLIB_DECLARE_OBJECT

	public:
		ViewInstance();
		
		~ViewInstance();
		
	public:
		Ref<View> getView();
		
		void setView(View* view);
		
		sl_bool isNativeWidget();
		
		void setNativeWidget(sl_bool flag);

		sl_bool isWindowContent();

		void setWindowContent(sl_bool flag);

	public:
		virtual sl_bool isValid(View* view) = 0;
		
		virtual void setFocus(View* view, sl_bool flagFocus) = 0;
		
		virtual void invalidate(View* view) = 0;

		virtual void invalidate(View* view, const UIRect& rect) = 0;
		
		virtual void setFrame(View* view, const UIRect& frame) = 0;
		
		virtual void setTransform(View* view, const Matrix3& transform) = 0;
		
		virtual void setVisible(View* view, sl_bool flag) = 0;
		
		virtual void setEnabled(View* view, sl_bool flag) = 0;
		
		virtual void setOpaque(View* view, sl_bool flag) = 0;
		
		virtual void setAlpha(View* view, sl_real alpha) = 0;
		
		virtual void setClipping(View* view, sl_bool flag) = 0;
		
		virtual void setDrawing(View* view, sl_bool flag) = 0;

		virtual UIPointf convertCoordinateFromScreenToView(View* view, const UIPointf& ptScreen) = 0;
		
		virtual UIPointf convertCoordinateFromViewToScreen(View* view, const UIPointf& ptView) = 0;
		
		virtual void addChildInstance(View* view, const Ref<ViewInstance>& instance) = 0;
		
		virtual void removeChildInstance(View* view, const Ref<ViewInstance>& instance) = 0;

		virtual void bringToFront(View* view) = 0;
		
		// extended functions for view instances
		virtual void setShadowOpacity(View* view, float alpha);
		
		virtual void setShadowRadius(View* view, sl_ui_posf radius);
		
		virtual void setShadowOffset(View* view, sl_ui_posf x, sl_ui_posf y);

		virtual void setShadowColor(View* view, const Color& color);
		
		// extended functions for native widgets
		virtual sl_bool isDrawingEnabled(View* view);

		virtual void setBorder(View* view, sl_bool flag);
		
		virtual void setBackgroundColor(View* view, const Color& color);
		
		virtual void setFont(View* view, const Ref<Font>& font);

		virtual void setPadding(View* view, const UIEdgeInsets& padding);

		virtual void setScrollBarsVisible(View* view, sl_bool flagHorizontal, sl_bool flagVertical);

		virtual void scrollTo(View* view, sl_scroll_pos x, sl_scroll_pos y, sl_bool flagAnimate);
		
		virtual void setPaging(View* view, sl_bool flagPaging, sl_ui_len pageWidth, sl_ui_len pageHeight);

		virtual void setLockScroll(View* view, sl_bool flagLock);
		
		virtual void setDroppable(View* view, sl_bool flag);
		
	public:
		void onDraw(Canvas* canvas);
		
		void onClick();
		
		void onKeyEvent(UIEvent* event);
		
		void onMouseEvent(UIEvent* event);
		
		void onTouchEvent(UIEvent* event);
		
		void onMouseWheelEvent(UIEvent* event);
		
		void onSetCursor(UIEvent* event);
		
		void onDragEvent(UIEvent* event);

		void onDropEvent(UIEvent* event);
		
		void onSetFocus();
		
		void onSwipe(GestureType ev);
		
	protected:
		AtomicWeakRef<View> m_view;
		sl_bool m_flagNativeWidget;
		sl_bool m_flagWindowContent;

	};

	class SLIB_EXPORT ViewCell : public Object
	{
		SLIB_DECLARE_OBJECT

	public:
		ViewCell(View* view);

		~ViewCell();

	public:
		View* getView();

		void setView(View* view);

		Ref<Font> getFont();

		UIRect getFrame();

		void setFrame(const UIRect& frame);

		void invalidate();

		void invalidate(const UIRect& frame);

		Ref<Dispatcher> getCellDispatcher();

		Ref<Timer> createTimer(const Function<void(Timer*)>& task, sl_uint32 interval_ms);

		Ref<Timer> startTimer(const Function<void(Timer*)>& task, sl_uint32 interval_ms);

	protected:
		View* m_view;
		UIRect m_frame;
		sl_bool m_flagUseCustomFrame;

	};

	class ViewGroup : public View
	{
		SLIB_DECLARE_OBJECT
	public:
		ViewGroup();
		
		~ViewGroup();
		
	};

}

#endif
