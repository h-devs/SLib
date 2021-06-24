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

#ifndef CHECKHEADER_SLIB_UI_VIEW_ATTRIBUTES
#define CHECKHEADER_SLIB_UI_VIEW_ATTRIBUTES

#include "constants.h"
#include "motion_tracker.h"

#include "../math/matrix3.h"
#include "../graphics/color.h"
#include "../core/linked_list.h"
#include "../core/function.h"
#include "../core/string.h"
#include "../core/shared.h"

namespace slib
{

	class View;
	class GraphicsPath;
	class Pen;
	class Font;
	class Drawable;
	class Bitmap;
	class Canvas;
	class Timer;
	class ScrollBar;
	class Animation;
	class UIEvent;
	class GestureEvent;

	class ViewLayoutAttributes : public Referable
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
		ViewLayoutAttributes();
		
		~ViewLayoutAttributes();
		
		SLIB_DELETE_CLASS_DEFAULT_MEMBERS(ViewLayoutAttributes)
		
	public:
		void applyMarginWeightsX(sl_ui_pos parentWidth);
		void applyMarginWeightsY(sl_ui_pos parentHeight);
		void applyMarginWeights(sl_ui_pos parentWidth, sl_ui_pos parentHeight);

	};
		
	class ViewPaddingAttributes : public Referable
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
		ViewPaddingAttributes();
		
		~ViewPaddingAttributes();
		
		SLIB_DELETE_CLASS_DEFAULT_MEMBERS(ViewPaddingAttributes)

	public:
		void applyPaddingWeightsX(sl_ui_pos width);
		void applyPaddingWeightsY(sl_ui_pos height);
		void applyPaddingWeights(sl_ui_pos width, sl_ui_pos height);
		
	};
	
	class ViewTransformAttributes : public Referable
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
		ViewTransformAttributes();
		
		~ViewTransformAttributes();
		
		SLIB_DELETE_CLASS_DEFAULT_MEMBERS(ViewTransformAttributes)

	};
	
	class ViewDrawAttributes : public Referable
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
		ViewDrawAttributes();
		
		~ViewDrawAttributes();
		
		SLIB_DELETE_CLASS_DEFAULT_MEMBERS(ViewDrawAttributes)

	};

	class ViewScrollAttributes : public Referable
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
		ViewScrollAttributes();
		
		~ViewScrollAttributes();
		
		SLIB_DELETE_CLASS_DEFAULT_MEMBERS(ViewScrollAttributes)

	};

	class ViewChildAttributes : public Referable
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
		ViewChildAttributes();
		
		~ViewChildAttributes();
		
		SLIB_DELETE_CLASS_DEFAULT_MEMBERS(ViewChildAttributes)

	};

	class ViewOtherAttributes : public Referable
	{
	public:
		AtomicString _id;
		AtomicWeakRef<View> viewNextTabStop;
		AtomicWeakRef<View> viewPrevTabStop;
		AtomicRef<Cursor> cursor;
		AtomicRef<GestureDetector> gestureDetector;
		AtomicShared<DragItem> dragItem;
		DragOperations dragOperationMask;
		char mnemonicKey;

	public:
		ViewOtherAttributes();
		
		~ViewOtherAttributes();
		
		SLIB_DELETE_CLASS_DEFAULT_MEMBERS(ViewOtherAttributes)

	};

	class ViewEventAttributes : public Referable
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
		AtomicFunction<void(View*, UIEvent*)> onDragDropEvent;
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
		ViewEventAttributes();
		
		~ViewEventAttributes();
		
		SLIB_DELETE_CLASS_DEFAULT_MEMBERS(ViewEventAttributes)

	};

}

#endif
