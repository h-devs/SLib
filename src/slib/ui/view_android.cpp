/*
 *   Copyright (c) 2008-2024 SLIBIO <https://github.com/SLIBIO>
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

#include "slib/ui/definition.h"

#if defined(SLIB_UI_IS_ANDROID)

#include "view_android.h"

#include "slib/ui/gesture.h"
#include "slib/ui/scroll_view.h"
#include "slib/math/transform2d.h"

namespace slib
{

	namespace
	{
		SLIB_JNI_BEGIN_CLASS(JPoint, "android/graphics/Point")
			SLIB_JNI_INT_FIELD(x);
			SLIB_JNI_INT_FIELD(y);
		SLIB_JNI_END_CLASS

		SLIB_JNI_BEGIN_CLASS(JTouchPoint, "slib/android/ui/view/UiTouchPoint")
			SLIB_JNI_FLOAT_FIELD(x);
			SLIB_JNI_FLOAT_FIELD(y);
			SLIB_JNI_FLOAT_FIELD(pressure);
			SLIB_JNI_INT_FIELD(phase);
			SLIB_JNI_INT_FIELD(pointerId);
		SLIB_JNI_END_CLASS

		static void JNICALL OnDraw(JNIEnv* env, jobject _this, jlong jinstance, jobject jcanvas, jint left, jint top, jint right, jint bottom)
		{
			Ref<Android_ViewInstance> instance = Android_ViewInstance::findInstance(jinstance);
			if (instance.isNotNull()) {
				Ref<Canvas> canvas = GraphicsPlatform::createCanvas(CanvasType::View, jcanvas);
				if (canvas.isNotNull()) {
					canvas->setInvalidatedRect(Rectangle((sl_real)left, (sl_real)top, (sl_real)right, (sl_real)bottom));
					instance->onDraw(canvas.get());
				}
			}
		}

		static jboolean JNICALL OnKeyEvent(JNIEnv* env, jobject _this, jlong jinstance, jboolean flagDown, int keycode, jboolean flagControl, jboolean flagShift, jboolean flagAlt, jboolean flagWin, jlong time, jboolean flagDispatchToParent, jboolean flagNotDispatchToChildren)
		{
			Ref<Android_ViewInstance> instance = Android_ViewInstance::findInstance(jinstance);
			if (instance.isNotNull()) {
				UIAction action = flagDown ? UIAction::KeyDown : UIAction::KeyUp;
				sl_uint32 vkey = keycode;
				Keycode key = UIEvent::getKeycodeFromSystemKeycode(keycode);
				Time t;
				t.setMillisecondCount(time);
				Ref<UIEvent> ev = UIEvent::createKeyEvent(action, key, vkey, t);
				if (ev.isNotNull()) {
					if (flagControl) {
						ev->setControlKey();
					}
					if (flagShift) {
						ev->setShiftKey();
					}
					if (flagAlt) {
						ev->setAltKey();
					}
					if (flagWin) {
						ev->setWindowsKey();
					}
					if (flagDispatchToParent) {
						ev->addFlag(UIEventFlags::DispatchToParent);
					}
					if (flagNotDispatchToChildren) {
						ev->addFlag(UIEventFlags::NotDispatchToChildren);
					}
					instance->onKeyEvent(ev.get());
					if (ev->getFlags() & UIEventFlags::NotInvokeNative) {
						return 1;
					}
				}
			}
			return 0;
		}

		jint JNICALL OnTouchEvent(JNIEnv* env, jobject _this, jlong jinstance, int _action, jobjectArray jpoints, jlong time, jboolean flagDispatchToParent, jboolean flagNotDispatchToChildren)
		{
			Ref<Android_ViewInstance> instance = Android_ViewInstance::findInstance(jinstance);
			if (instance.isNotNull()) {
				UIAction action;
				switch (_action) {
					case 1:
						action = UIAction::TouchBegin;
						break;
					case 2:
						action = UIAction::TouchMove;
						break;
					case 3:
						action = UIAction::TouchEnd;
						break;
					case 4:
						action = UIAction::TouchCancel;
						break;
					default:
						return 0;
				}
				sl_uint32 nPts = Jni::getArrayLength(jpoints);
				if (nPts > 0) {
					Array<TouchPoint> points = Array<TouchPoint>::create(nPts);
					if (points.isNotNull()) {
						TouchPoint* pts = points.getData();
						for (sl_uint32 i = 0; i < nPts; i++) {
							JniLocal<jobject> jpt = Jni::getObjectArrayElement(jpoints, i);
							if (jpt.isNotNull()) {
								pts[i].point.x = (sl_ui_posf)(JTouchPoint::x.get(jpt));
								pts[i].point.y = (sl_ui_posf)(JTouchPoint::y.get(jpt));
								pts[i].pressure = JTouchPoint::pressure.get(jpt);
								TouchPhase phase;
								switch (JTouchPoint::phase.get(jpt)) {
									case 0:
										phase = TouchPhase::Move;
										break;
									case 1:
										phase = TouchPhase::Begin;
										break;
									case 2:
										phase = TouchPhase::End;
										break;
									case 3:
										phase = TouchPhase::Cancel;
										break;
									default:
										return 0;
								}
								pts[i].phase = phase;
								pts[i].pointerId = JTouchPoint::pointerId.get(jpt);
							}
						}
						Ref<UIEvent> ev = UIEvent::createTouchEvent(action, points, Time::withMilliseconds(time));
						if (ev.isNotNull()) {
							if (flagDispatchToParent) {
								ev->addFlag(UIEventFlags::DispatchToParent);
							}
							if (flagNotDispatchToChildren) {
								ev->addFlag(UIEventFlags::NotDispatchToChildren);
							}
							instance->onTouchEvent(ev.get());
							return ev->getFlags();
						}
					}
				}
			}
			return 0;
		}

		static void JNICALL OnSetFocus(JNIEnv* env, jobject _this, jlong jinstance)
		{
			Ref<Android_ViewInstance> instance = Android_ViewInstance::findInstance(jinstance);
			if (instance.isNotNull()) {
				instance->onSetFocus();
			}
		}

		static void JNICALL OnClick(JNIEnv* env, jobject _this, jlong jinstance)
		{
			Ref<Android_ViewInstance> instance = Android_ViewInstance::findInstance(jinstance);
			if (instance.isNotNull()) {
				instance->onClick();
			}
		}

		static jboolean JNICALL HitTestTouchEvent(JNIEnv* env, jobject _this, jlong jinstance, int x, int y)
		{
			Ref<Android_ViewInstance> instance = Android_ViewInstance::findInstance(jinstance);
			if (instance.isNotNull()) {
				Ref<View> view = instance->getView();
				if (view.isNotNull()) {
					if (!(view->isEnabled())) {
						return 1;
					}
					if (view->isCapturingChildInstanceEvents((sl_ui_pos)x, (sl_ui_pos)y)) {
						return 1;
					}
				}
			}
			return 0;
		}

		static void JNICALL OnSwipe(JNIEnv* env, jobject _this, jlong jinstance, int type)
		{
			Ref<Android_ViewInstance> instance = Android_ViewInstance::findInstance(jinstance);
			if (instance.isNotNull()) {
				instance->onSwipe((GestureType)type);
			}
		}

		SLIB_JNI_BEGIN_CLASS(JView, "slib/android/ui/view/UiView")

			SLIB_JNI_STATIC_METHOD(getContext, "getContext", "(Landroid/view/View;)Landroid/content/Context;");
			SLIB_JNI_STATIC_METHOD(setInstance, "setInstance", "(Landroid/view/View;J)V");
			SLIB_JNI_STATIC_METHOD(freeView, "freeView", "(Landroid/view/View;)V");

			SLIB_JNI_STATIC_METHOD(createGeneric, "createGeneric", "(Landroid/content/Context;)Landroid/view/View;");
			SLIB_JNI_STATIC_METHOD(createGroup, "createGroup", "(Landroid/content/Context;)Landroid/view/View;");
			SLIB_JNI_STATIC_METHOD(createScrollContent, "createScrollContent", "(Landroid/content/Context;)Landroid/view/View;");

			SLIB_JNI_STATIC_METHOD(setFocus, "setFocus", "(Landroid/view/View;Z)V");
			SLIB_JNI_STATIC_METHOD(invalidate, "invalidate", "(Landroid/view/View;)V");
			SLIB_JNI_STATIC_METHOD(invalidateRect, "invalidateRect", "(Landroid/view/View;IIII)V");
			SLIB_JNI_STATIC_METHOD(setFrame, "setFrame", "(Landroid/view/View;IIII)Z");
			SLIB_JNI_STATIC_METHOD(setTransform, "setTransform", "(Landroid/view/View;FFFFFFF)V");
			SLIB_JNI_STATIC_METHOD(isVisible, "isVisible", "(Landroid/view/View;)Z");
			SLIB_JNI_STATIC_METHOD(setVisible, "setVisible", "(Landroid/view/View;Z)V");
			SLIB_JNI_STATIC_METHOD(isEnabled, "isEnabled", "(Landroid/view/View;)Z");
			SLIB_JNI_STATIC_METHOD(setEnabled, "setEnabled", "(Landroid/view/View;Z)V");
			SLIB_JNI_STATIC_METHOD(setAlpha, "setAlpha", "(Landroid/view/View;F)V");
			SLIB_JNI_STATIC_METHOD(setClipping, "setClipping", "(Landroid/view/View;Z)V");
			SLIB_JNI_STATIC_METHOD(setDrawing, "setDrawing", "(Landroid/view/View;Z)V");
			SLIB_JNI_STATIC_METHOD(setLayered, "setLayered", "(Landroid/view/View;)V");
			SLIB_JNI_STATIC_METHOD(setShadow, "setShadow", "(Landroid/view/View;FF)V");
			SLIB_JNI_STATIC_METHOD(convertCoordinateFromScreenToView, "convertCoordinateFromScreenToView", "(Landroid/view/View;II)Landroid/graphics/Point;");
			SLIB_JNI_STATIC_METHOD(convertCoordinateFromViewToScreen, "convertCoordinateFromViewToScreen", "(Landroid/view/View;II)Landroid/graphics/Point;");

			SLIB_JNI_STATIC_METHOD(addChild, "addChild", "(Landroid/view/View;Landroid/view/View;)V");
			SLIB_JNI_STATIC_METHOD(removeChild, "removeChild", "(Landroid/view/View;Landroid/view/View;)V");
			SLIB_JNI_STATIC_METHOD(bringToFront, "bringToFront", "(Landroid/view/View;)V");
			SLIB_JNI_STATIC_METHOD(enableGesture, "enableGesture", "(Landroid/view/View;)V");

			SLIB_JNI_NATIVE(onDraw, "nativeOnDraw", "(JLslib/android/ui/Graphics;IIII)V", OnDraw);
			SLIB_JNI_NATIVE(onKeyEvent, "nativeOnKeyEvent", "(JZIZZZZJZZ)Z", OnKeyEvent);
			SLIB_JNI_NATIVE(onTouchEvent, "nativeOnTouchEvent", "(JI[Lslib/android/ui/view/UiTouchPoint;JZZ)I", OnTouchEvent);
			SLIB_JNI_NATIVE(onSetFocus, "nativeOnSetFocus", "(J)V", OnSetFocus);
			SLIB_JNI_NATIVE(onClick, "nativeOnClick", "(J)V", OnClick);
			SLIB_JNI_NATIVE(hitTestTouchEvent, "nativeHitTestTouchEvent", "(JII)Z", HitTestTouchEvent);
			SLIB_JNI_NATIVE(onSwipe, "nativeOnSwipe", "(JI)V", OnSwipe);

		SLIB_JNI_END_CLASS
	}

	SLIB_DEFINE_OBJECT(Android_ViewInstance, ViewInstance)

	Android_ViewInstance::Android_ViewInstance()
	{
	}

	Android_ViewInstance::~Android_ViewInstance()
	{
		jobject handle = m_handle;
		if (handle) {
			UIPlatform::removeViewInstance(handle);
			JView::freeView.call(sl_null, handle);
		}
	}

	sl_bool Android_ViewInstance::initWithHandle(jobject handle)
	{
		if (handle) {
			JniGlobal<jobject> context = JView::getContext.callObject(sl_null, handle);
			JniGlobal<jobject> ghandle = JniGlobal<jobject>::create(handle);
			if (context.isNotNull() && ghandle.isNotNull()) {
				handle = ghandle.get();
				m_context = Move(context);
				m_handle = Move(ghandle);
				jlong instance = (jlong)(handle);
				JView::setInstance.call(sl_null, handle, instance);
				UIPlatform::registerViewInstance(handle, this);
				return sl_true;
			}
		}
		return sl_false;
	}

	sl_bool Android_ViewInstance::applyProperties(View* view, ViewInstance* parent)
	{
		jobject handle = m_handle;
		if (handle) {
			UIRect frame = view->getFrameInInstance();
			JView::setFrame.callBoolean(sl_null, handle, (int)(frame.left), (int)(frame.top), (int)(frame.right), (int)(frame.bottom));
			JView::setVisible.call(sl_null, handle, view->isVisibleInInstance());
			JView::setEnabled.call(sl_null, handle, view->isEnabled());
			sl_real alpha = view->getAlpha();
			JView::setClipping.call(sl_null, handle, view->isClipping());
			JView::setDrawing.call(sl_null, handle, view->isDrawing());
			if (alpha < 0.995f) {
				JView::setAlpha.call(sl_null, handle, alpha);
			}
			if (view->isCreatingNativeLayer()) {
				JView::setLayered.call(sl_null, handle);
			}
			float opacity = view->getShadowOpacity();
			if (opacity > SLIB_EPSILON) {
				JView::setShadow.call(sl_null, handle, (jfloat)opacity, (jfloat)(view->getShadowRadius()));
			}

			Matrix3 transform = view->getTransformInInstance();
			Vector2 t = Transform2::getTranslationFromMatrix(transform);
			sl_real r = Transform2::getRotationAngleFromMatrix(transform);
			Vector2 s = Transform2::getScaleFromMatrix(transform);
			JView::setTransform.call(sl_null, handle, t.x, t.y, r, s.x, s.y, 0, 0);

			if (parent) {
				jobject jparent = UIPlatform::getViewHandle(parent);
				if (jparent) {
					JView::addChild.call(sl_null, jparent, handle);
				}
			}
			return sl_true;
		}
		return sl_false;
	}

	Ref<Android_ViewInstance> Android_ViewInstance::findInstance(jlong jinstance)
	{
		return Ref<Android_ViewInstance>::cast(UIPlatform::getViewInstance((jobject)jinstance));
	}

	Ref<View> Android_ViewInstance::findView(jlong jinstance)
	{
		Ref<ViewInstance> instance = UIPlatform::getViewInstance((jobject)jinstance);
		if (instance.isNotNull()) {
			return instance->getView();
		}
		return sl_null;
	}

	jobject Android_ViewInstance::getHandle()
	{
		return m_handle.get();
	}

	jobject Android_ViewInstance::getContext()
	{
		return m_context.get();
	}

	sl_bool Android_ViewInstance::isValid(View* view)
	{
		return sl_true;
	}

	void Android_ViewInstance::setFocus(View* view, sl_bool flag)
	{
		jobject handle = m_handle;
		if (handle) {
			m_flagSettingFocus = sl_true;
			JView::setFocus.call(sl_null, handle, flag ? sl_true : sl_false);
			m_flagSettingFocus = sl_false;
		}
	}

	void Android_ViewInstance::invalidate(View* view)
	{
		jobject handle = m_handle;
		if (handle) {
			JView::invalidate.call(sl_null, handle);
		}
	}

	void Android_ViewInstance::invalidate(View* view, const UIRect& rect)
	{
		jobject handle = m_handle;
		if (handle) {
			JView::invalidateRect.call(sl_null, handle, (int)(rect.left), (int)(rect.top), (int)(rect.right), (int)(rect.bottom));
		}
	}

	void Android_ViewInstance::setFrame(View* view, const UIRect& frame)
	{
		jobject handle = m_handle;
		if (handle) {
			JView::setFrame.callBoolean(sl_null, handle, (int)(frame.left), (int)(frame.top), (int)(frame.right), (int)(frame.bottom));
		}
	}

	void Android_ViewInstance::setTransform(View* view, const Matrix3& transform)
	{
		jobject handle = m_handle;
		if (handle) {
			Vector2 t = Transform2::getTranslationFromMatrix(transform);
			sl_real r = Transform2::getRotationAngleFromMatrix(transform);
			Vector2 s = Transform2::getScaleFromMatrix(transform);
			JView::setTransform.call(sl_null, handle, t.x, t.y, r, s.x, s.y, 0, 0);
		}
	}

	void Android_ViewInstance::setVisible(View* view, sl_bool flag)
	{
		jobject handle = m_handle;
		if (handle) {
			JView::setVisible.call(sl_null, handle, flag);
		}
	}

	void Android_ViewInstance::setEnabled(View* view, sl_bool flag)
	{
		jobject handle = m_handle;
		if (handle) {
			JView::setEnabled.call(sl_null, handle, flag);
		}
	}

	void Android_ViewInstance::setOpaque(View* view, sl_bool flag)
	{
	}

	void Android_ViewInstance::setAlpha(View* view, sl_real alpha)
	{
		jobject handle = m_handle;
		if (handle) {
			JView::setAlpha.call(sl_null, handle, (float)alpha);
		}
	}

	void Android_ViewInstance::setClipping(View* view, sl_bool flag)
	{
		jobject handle = m_handle;
		if (handle) {
			JView::setClipping.call(sl_null, handle, flag);
		}
	}

	void Android_ViewInstance::setDrawing(View* view, sl_bool flag)
	{
		jobject handle = m_handle;
		if (handle) {
			JView::setDrawing.call(sl_null, handle, flag);
		}
	}

	UIPointF Android_ViewInstance::convertCoordinateFromScreenToView(View* view, const UIPointF& ptScreen)
	{
		jobject handle = m_handle;
		if (handle) {
			JniLocal<jobject> jpt = JView::convertCoordinateFromScreenToView.callObject(sl_null, handle, 0, 0);
			if (jpt.isNotNull()) {
				UIPointF ret;
				ret.x = ptScreen.x + (sl_ui_pos)(JPoint::x.get(jpt));
				ret.y = ptScreen.y + (sl_ui_pos)(JPoint::y.get(jpt));
				return ret;
			}
		}
		return ptScreen;
	}

	UIPointF Android_ViewInstance::convertCoordinateFromViewToScreen(View* view, const UIPointF& ptView)
	{
		jobject handle = m_handle;
		if (handle) {
			JniLocal<jobject> jpt = JView::convertCoordinateFromViewToScreen.callObject(sl_null, handle, 0, 0);
			if (jpt.isNotNull()) {
				UIPointF ret;
				ret.x = ptView.x + (sl_ui_pos)(JPoint::x.get(jpt));
				ret.y = ptView.y + (sl_ui_pos)(JPoint::y.get(jpt));
				return ret;
			}
		}
		return ptView;
	}

	void Android_ViewInstance::addChildInstance(View* view, const Ref<ViewInstance>& _child)
	{
		jobject handle = m_handle;
		jobject child = UIPlatform::getViewHandle(_child.get());
		if (handle && child) {
			JView::addChild.call(sl_null, handle, child);
		}
	}

	void Android_ViewInstance::removeChildInstance(View* view, const Ref<ViewInstance>& _child)
	{
		jobject handle = m_handle;
		jobject child = UIPlatform::getViewHandle(_child.get());
		if (handle && child) {
			JView::removeChild.call(sl_null, handle, child);
		}
	}

	void Android_ViewInstance::bringToFront(View* view)
	{
		jobject handle = m_handle;
		if (handle) {
			JView::bringToFront.call(sl_null, handle);
		}
	}

	void Android_ViewInstance::setShadowOpacity(View* view, float opacity)
	{
		jobject handle = m_handle;
		if (handle) {
			JView::setShadow.call(sl_null, (jfloat)opacity, (jfloat)(view->getShadowRadius()));
		}
	}

	void Android_ViewInstance::setShadowRadius(View* view, sl_ui_posf radius)
	{
		jobject handle = m_handle;
		if (handle) {
			JView::setShadow.call(sl_null, (jfloat)(view->getShadowOpacity()), (jfloat)(radius));
		}
	}


	Ref<ViewInstance> View::createGenericInstance(ViewInstance* _parent)
	{
		Android_ViewInstance* parent = (Android_ViewInstance*)_parent;
		if (parent) {
			JniLocal<jobject> handle;
			if (IsInstanceOf<ScrollView>(getParent()) && isCreatingLargeContent()) {
				handle = JView::createScrollContent.callObject(sl_null, parent->getContext());
			} else {
				if (m_flagCreatingChildInstances) {
					handle = JView::createGroup.callObject(sl_null, parent->getContext());
				} else {
					handle = JView::createGeneric.callObject(sl_null, parent->getContext());
				}
			}
			return Android_ViewInstance::create<Android_ViewInstance>(this, parent, handle.get());
		}
		return sl_null;
	}


	Ref<ViewInstance> UIPlatform::createViewInstance(jobject handle)
	{
		Ref<ViewInstance> ret = UIPlatform::_getViewInstance((void*)handle);
		if (ret.isNotNull()) {
			return ret;
		}
		return Android_ViewInstance::create<Android_ViewInstance>(handle);
	}

	void UIPlatform::registerViewInstance(jobject handle, ViewInstance* instance)
	{
		UIPlatform::_registerViewInstance((void*)(handle), instance);
	}

	Ref<ViewInstance> UIPlatform::getViewInstance(jobject handle)
	{
		return UIPlatform::_getViewInstance((void*)handle);
	}

	void UIPlatform::removeViewInstance(jobject handle)
	{
		UIPlatform::_removeViewInstance((void*)handle);
	}

	jobject UIPlatform::getViewHandle(ViewInstance* _instance)
	{
		Android_ViewInstance* instance = (Android_ViewInstance*)_instance;
		if (instance) {
			return instance->getHandle();
		}
		return 0;
	}

	jobject UIPlatform::getViewHandle(View* view)
	{
		if (view) {
			Ref<Android_ViewInstance> instance = Ref<Android_ViewInstance>::cast(view->getViewInstance());
			if (instance.isNotNull()) {
				return instance->getHandle();
			}
		}
		return 0;
	}

	sl_bool GestureDetector::_enableNative(const Ref<View>& view, GestureType type)
	{
		Ref<ViewInstance> _instance = view->getViewInstance();
		Android_ViewInstance* instance = static_cast<Android_ViewInstance*>(_instance.get());
		if (instance) {
			jobject handle = instance->getHandle();
			if (handle) {
				switch (type) {
					case GestureType::SwipeLeft:
					case GestureType::SwipeRight:
					case GestureType::SwipeUp:
					case GestureType::SwipeDown:
						JView::enableGesture.call(sl_null, handle);
						return sl_true;
					default:
						break;
				}
			}
		}
		return sl_false;
	}

}

#endif
