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

#ifndef CHECKHEADER_SLIB_UI_VIEW_ATTRIBUTES
#define CHECKHEADER_SLIB_UI_VIEW_ATTRIBUTES

#include "view.h"
#include "view_state_map.h"
#include "motion_tracker.h"

#include "../math/matrix3.h"
#include "../core/linked_list.h"
#include "../core/function.h"
#include "../core/string.h"
#include "../core/shared.h"

namespace slib
{

	class View::LayoutAttributes : public CRef
	{
	public:
		sl_bool flagMarginLeftWeight : 1;
		sl_bool flagMarginTopWeight : 1;
		sl_bool flagMarginRightWeight : 1;
		sl_bool flagMarginBottomWeight : 1;
		sl_bool flagCustomLayout : 1;
		sl_bool flagMatchParentWidth : 1;
		sl_bool flagMatchParentHeight : 1;

		sl_bool flagInvalidLayoutInParent : 1;
		sl_bool flagRequestedFrame : 1;
		sl_bool flagLastWidthWrapping : 1;
		sl_bool flagLastHeightWrapping : 1;

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

	class View::PaddingAttributes : public CRef
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

	class View::TransformAttributes : public CRef
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

	class View::DrawAttributes : public CRef
	{
	public:
		sl_bool flagUsingFont : 1;
		sl_bool flagOpaque : 1;
		sl_bool flagAntiAlias : 1;
		sl_bool flagLayer : 1;

		sl_bool flagForcedDraw : 1;
		sl_bool flagInvalidatedLayer : 1;
		sl_bool flagInvalidatedWholeLayer : 1;

		ViewStateMap< Ref<Drawable> > backgrounds;
		ScaleMode backgroundScaleMode;
		Alignment backgroundAlignment;

		ViewStateMap< Ref<Pen> > borders;

		BoundShape boundShape;
		Size boundRadius;
		AtomicRef<GraphicsPath> boundPath;

		BoundShape contentShape;
		Size contentRadius;
		AtomicRef<GraphicsPath> contentBoundPath;

		AtomicRef<Font> font;
		sl_real alpha;
		Color colorKey;

		AtomicRef<Bitmap> bitmapLayer;
		AtomicRef<Canvas> canvasLayer;
		UIRect rectInvalidatedLayer;

		float shadowOpacity;
		sl_ui_posf shadowRadius;
		UIPointF shadowOffset;
		Color shadowColor;

		LinkedList< Function<void()> > runAfterDrawCallbacks;

	public:
		DrawAttributes();
		~DrawAttributes();
		SLIB_DELETE_CLASS_DEFAULT_MEMBERS(DrawAttributes)

	};

	class View::ScrollAttributes : public CRef
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
		sl_bool flagDownContent : 1;

		AtomicRef<ScrollBar> horz;
		AtomicRef<ScrollBar> vert;
		sl_scroll_pos x;
		sl_scroll_pos y;
		sl_scroll_pos contentWidth;
		sl_scroll_pos contentHeight;
		sl_scroll_pos pageWidth;
		sl_scroll_pos pageHeight;
		sl_ui_len barWidth;

		Point mousePointDown;
		Point mousePointBefore;
		sl_uint64 touchPointerIdBefore;
		Time timeLastInside;

		struct SmoothFlow
		{
			MotionTracker motionTracker;
			Ref<Timer> timer;
			Time timeFrameBefore;
			ScrollEvent::Source source;
			sl_bool flagTarget;
			sl_scroll_pos speedX;
			sl_scroll_pos speedY;
			sl_scroll_pos targetX;
			sl_scroll_pos targetY;
		};
		Shared<SmoothFlow> smooth;

	public:
		ScrollAttributes();
		~ScrollAttributes();
		SLIB_DELETE_CLASS_DEFAULT_MEMBERS(ScrollAttributes)

	};

	class View::ChildAttributes : public CRef
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
		AtomicRef<View> childFocal;

		AtomicFunction<sl_bool(const UIPoint& pt)> hitTestCapturingChildInstanceEvents;

	public:
		ChildAttributes();
		~ChildAttributes();
		SLIB_DELETE_CLASS_DEFAULT_MEMBERS(ChildAttributes)

	};

	class View::OtherAttributes : public CRef
	{
	public:
		AtomicString _id;
		AtomicWeakRef<View> viewNextTabStop;
		AtomicWeakRef<View> viewPrevTabStop;
		AtomicRef<Cursor> cursor;
		AtomicString toolTip;
		AtomicRef<GestureDetector> gestureDetector;
		AtomicShared<DragItem> dragItem;
		DragOperations dragOperationMask;
		char mnemonicKey;

	public:
		OtherAttributes();
		~OtherAttributes();
		SLIB_DELETE_CLASS_DEFAULT_MEMBERS(OtherAttributes)

	};

	class View::EventAttributes : public CRef
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
		AtomicFunction<void(View*, sl_bool)> onChangeFocus;
		AtomicFunction<void(View*, sl_ui_pos, sl_ui_pos)> onMove;
		AtomicFunction<void(View*, sl_ui_len, sl_ui_len)> onResize;
		AtomicFunction<void(View*, Visibility, Visibility)> onChangeVisibility;
		AtomicFunction<void(View*, ScrollEvent*)> onScroll;
		AtomicFunction<void(View*, GestureEvent*)> onSwipe;
		AtomicFunction<void(View*)> onOK;
		AtomicFunction<void(View*)> onCancel;
		AtomicFunction<void(View*, UIEvent*)> onMnemonic;

	public:
		EventAttributes();
		~EventAttributes();
		SLIB_DELETE_CLASS_DEFAULT_MEMBERS(EventAttributes)

	};

}

#endif
