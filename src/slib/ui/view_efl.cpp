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

#if defined(SLIB_UI_IS_EFL)

#include "view_efl.h"

#include "slib/ui/core.h"

#include <Elementary.h>

namespace slib
{

	namespace
	{
		static const void* g_strEventTouchBegin = "tb";
		static const void* g_strEventTouchEnd = "te";
		static const void* g_strEventTouchMove = "tm";
		static const void* g_strEventMultiTouchBegin = "mtb";
		static const void* g_strEventMultiTouchEnd = "mte";
		static const void* g_strEventMultiTouchMove = "mtm";
	}

	SLIB_DEFINE_OBJECT(PlatformViewInstance, ViewInstance)

	PlatformViewInstance::PlatformViewInstance()
	{
		m_type = EFL_ViewType::Generic;
		m_handle = sl_null;
		m_flagFreeOnRelease = sl_true;
	}

	PlatformViewInstance::~PlatformViewInstance()
	{
		_release();
	}

	sl_bool PlatformViewInstance::initialize(EFL_ViewType type, Evas_Object* handle)
	{
		if (handle) {
			m_type = type;
			m_handle = handle;
			UIPlatform::registerViewInstance(handle, this);
			return sl_true;
		}
		return sl_false;
	}

	sl_bool PlatformViewInstance::applyProperties(View* view, ViewInstance* _parent)
	{
		PlatformViewInstance* parent = static_cast<PlatformViewInstance*>(_parent);
		Evas_Object* handle = m_handle;
		if (handle) {
			UIRect frame = view->getFrame();
			if (parent && parent->m_type == EFL_ViewType::Grid) {
				Evas_Object* handleParent = parent->m_handle;
				if (handleParent) {
					elm_grid_pack(handleParent, handle, frame.left, frame.top, frame.getWidth(), frame.getHeight());
				}
			} else {
				evas_object_move(handle, frame.left, frame.top);
				evas_object_resize(handle, frame.getWidth(), frame.getHeight());
			}
			if (m_type == EFL_ViewType::Grid) {
				elm_grid_size_set(handle, frame.getWidth(), frame.getHeight());
			}
			if (view->isVisibleInInstance()) {
				evas_object_show(handle);
			}
			sl_real alpha = view->getAlpha();
			if (alpha < 0.005f) {
				evas_object_color_set(handle, 255, 255, 255, 0);
			} else if (alpha < 0.995f) {
				evas_object_color_set(handle, 255, 255, 255, (int)(255 * alpha));
			}
			return sl_true;
		}
		return sl_false;
	}

	void PlatformViewInstance::_release()
	{
		Evas_Object* handle = m_handle;
		if (handle) {
			UIPlatform::removeViewInstance(handle);
			if (m_flagFreeOnRelease) {
				freeHandle(handle);
			}
			m_handle = sl_null;
		}
	}

	void PlatformViewInstance::freeHandle(Evas_Object* handle)
	{
		if (handle) {
			evas_object_del(handle);
		}
	}

	EFL_ViewType PlatformViewInstance::getType()
	{
		return m_type;
	}

	Evas_Object* PlatformViewInstance::getHandle()
	{
		return m_handle;
	}

	sl_bool PlatformViewInstance::isValid()
	{
		return sl_true;
	}

	void PlatformViewInstance::setFocus(View* view, sl_bool flag)
	{
		Evas_Object* handle = m_handle;
		if (handle) {
			if (flag) {
				m_flagSettingFocus = sl_true;
				elm_object_focus_set(handle, EINA_TRUE);
				m_flagSettingFocus = sl_false;
			}
		}
	}

	void PlatformViewInstance::invalidate(View* view)
	{
	}

	void PlatformViewInstance::invalidate(View* view, const UIRect& _rect)
	{
	}

	void PlatformViewInstance::setFrame(View* view, const UIRect& frame)
	{
		if (m_type != EFL_ViewType::Window) {
			Evas_Object* handle = m_handle;
			if (handle) {
				elm_grid_pack_set(handle, frame.left, frame.top, frame.getWidth(), frame.getHeight());
				if (m_type == EFL_ViewType::Grid) {
					elm_grid_size_set(handle, frame.getWidth(), frame.getHeight());
				} else {
					evas_object_move(handle, frame.left, frame.top);
					evas_object_resize(handle, frame.getWidth(), frame.getHeight());
				}
			}
		}
	}

	void PlatformViewInstance::setTransform(View* view, const Matrix3& m)
	{
	}

	void PlatformViewInstance::setVisible(View* view, sl_bool flag)
	{
		Evas_Object* handle = m_handle;
		if (handle) {
			if (flag) {
				evas_object_show(handle);
			} else {
				evas_object_hide(handle);
			}
		}
	}

	void PlatformViewInstance::setEnabled(sl_bool flag)
	{
	}

	void PlatformViewInstance::setOpaque(sl_bool flag)
	{
	}

	void PlatformViewInstance::setAlpha(View* view, sl_real alpha)
	{
		Evas_Object* handle = m_handle;
		if (handle) {
			if (alpha <= 0.005f) {
				evas_object_color_set(handle, 255, 255, 255, 0);
			} else if (alpha <= 0.995) {
				evas_object_color_set(handle, 255, 255, 255, (int)(alpha * 255));
			} else {
				evas_object_color_set(handle, 255, 255, 255, 255);
			}
		}
	}

	void PlatformViewInstance::setClipping(View* view, sl_bool flag)
	{
	}

	void PlatformViewInstance::setDrawing(View* view, sl_bool flag)
	{
	}

	UIPointF PlatformViewInstance::convertCoordinateFromScreenToView(View* view, const UIPointF& ptScreen)
	{
		Evas_Object* handle = m_handle;
		if (handle) {
			Evas_Coord x = 0, y = 0;
			evas_object_geometry_get(handle, &x, &y, sl_null, sl_null);
			return UIPointF((sl_ui_posf)(ptScreen.x - x), (sl_ui_posf)(ptScreen.y - y));
		}
		return ptScreen;
	}

	UIPointF PlatformViewInstance::convertCoordinateFromViewToScreen(View* view, const UIPointF& ptView)
	{
		Evas_Object* handle = m_handle;
		if (handle) {
			Evas_Coord x = 0, y = 0;
			evas_object_geometry_get(handle, &x, &y, sl_null, sl_null);
			return UIPointF((sl_ui_posf)(ptView.x + x), (sl_ui_posf)(ptView.y + y));
		}
		return ptView;
	}

	void PlatformViewInstance::addChildInstance(View* view, const Ref<ViewInstance>& _child)
	{
		PlatformViewInstance* child = static_cast<PlatformViewInstance*>(_child.get());
		if (child) {
			Evas_Object* handle = m_handle;
			if (handle) {
				Evas_Object* handleChild = child->m_handle;
				if (handleChild) {
					if (m_type == EFL_ViewType::Grid) {
						UIRect frame;
						Ref<View> view = child->getView();
						if (view.isNotNull()) {
							frame = view->getFrame();
						} else {
							frame = UIRect::zero();
						}
						elm_grid_pack(handle, handleChild, frame.left, frame.top, frame.getWidth(), frame.getHeight());
					}
				}
			}
		}
	}

	void PlatformViewInstance::removeChildInstance(View* view, const Ref<ViewInstance>& _child)
	{
		PlatformViewInstance* child = static_cast<PlatformViewInstance*>(_child.get());
		if (child) {
			Evas_Object* handle = m_handle;
			if (handle) {
				Evas_Object* handleChild = child->m_handle;
				if (handleChild) {
					if (m_type == EFL_ViewType::Grid) {
						elm_grid_unpack(handle, handleChild);
					}
				}
			}
		}
	}

	void PlatformViewInstance::bringToFront(View* view)
	{
		Evas_Object* handle = m_handle;
		if (handle) {
			evas_object_raise(handle);
		}
	}

	PlatformViewInstance::TouchEventInfo::TouchEventInfo()
	{
		m_count = 0;
	}

	PlatformViewInstance::TouchEventInfo::~TouchEventInfo()
	{
	}

	void PlatformViewInstance::TouchEventInfo::setPoint(sl_uint32 index, TouchPhase phase, Evas_Coord x, Evas_Coord y)
	{
		if (index >= SLIB_EFL_MULTI_TOUCH_COUNT_MAX) {
			return;
		}
		if (index >= m_count) {
			for (sl_uint32 i = m_count; i < index; i++) {
				m_points[i].phase = TouchPhase::Cancel;
			}
			m_count = index + 1;
		}
		m_points[index].point.x = (sl_ui_posf)x;
		m_points[index].point.y = (sl_ui_posf)y;
		m_points[index].phase = phase;
	}

	void PlatformViewInstance::TouchEventInfo::clear()
	{
		m_count = 0;
	}

	sl_uint32 PlatformViewInstance::TouchEventInfo::getCount()
	{
		return m_count;
	}

	Array<TouchPoint> PlatformViewInstance::TouchEventInfo::getPoints()
	{
		sl_uint32 n = m_count;
		Array<TouchPoint> arr = Array<TouchPoint>::create(n);
		if (arr.isNotNull()) {
			TouchPoint* pts = arr.getData();
			sl_uint32 m = 0;
			for (sl_uint32 i = 0; i < n; i++) {
				if (m_points[i].phase != TouchPhase::Cancel) {
					pts[m] = m_points[i];
					m++;
				}
			}
			return arr.sub(0, m);
		}
		return sl_null;
	}

	void PlatformViewInstance::TouchEventInfo::finishPhases()
	{
		sl_uint32 n = m_count;
		sl_uint32 m = 0;
		for (sl_uint32 i = 0; i < n; i++) {
			TouchPoint& pt = m_points[i];
			if (pt.phase == TouchPhase::Begin) {
				pt.phase = TouchPhase::Move;
			} else if (pt.phase == TouchPhase::End) {
				pt.phase = TouchPhase::Cancel;
			}
			if (pt.phase != TouchPhase::Cancel) {
				m = i;
			}
		}
		m_count = m;
	}

	void PlatformViewInstance::onTouch(const void *eventType, void *event_info)
	{
		Evas_Object* handle = m_handle;
		if (!handle) {
			return;
		}

		Ref<View> view = getView();
		if (view.isNull()) {
			return;
		}

		if (m_touchEventInfo.isNull()) {
			m_touchEventInfo = new TouchEventInfo;
			if (m_touchEventInfo.isNull()) {
				return;
			}
		}

		Evas_Coord x, y;
		evas_object_geometry_get(handle, &x, &y, sl_null, sl_null);

		UIAction action;
		unsigned int timestamp;

		if (eventType == g_strEventTouchBegin) {
			Evas_Event_Mouse_Down* ev = (Evas_Event_Mouse_Down*)event_info;
			m_touchEventInfo->setPoint(0, TouchPhase::Begin, ev->canvas.x - x, ev->canvas.y - y);
			timestamp = ev->timestamp;
			action = UIAction::TouchBegin;
		} else if (eventType == g_strEventMultiTouchBegin) {
			Evas_Event_Multi_Down* ev = (Evas_Event_Multi_Down*)event_info;
			m_touchEventInfo->setPoint(ev->device, TouchPhase::Begin, ev->canvas.x - x, ev->canvas.y - y);
			timestamp = ev->timestamp;
			action = UIAction::TouchMove;
		} else if (eventType == g_strEventTouchMove) {
			Evas_Event_Mouse_Move* ev = (Evas_Event_Mouse_Move*)event_info;
			m_touchEventInfo->setPoint(0, TouchPhase::Move, ev->cur.canvas.x - x, ev->cur.canvas.y - y);
			timestamp = ev->timestamp;
			action = UIAction::TouchMove;
		} else if (eventType == g_strEventMultiTouchMove) {
			Evas_Event_Multi_Move* ev = (Evas_Event_Multi_Move*)event_info;
			m_touchEventInfo->setPoint(ev->device, TouchPhase::Move, ev->cur.canvas.x - x, ev->cur.canvas.y - y);
			timestamp = ev->timestamp;
			action = UIAction::TouchMove;
		} else if (eventType == g_strEventTouchEnd) {
			Evas_Event_Mouse_Up* ev = (Evas_Event_Mouse_Up*)event_info;
			m_touchEventInfo->setPoint(0, TouchPhase::End, ev->canvas.x - x, ev->canvas.y - y);
			timestamp = ev->timestamp;
			action = UIAction::TouchEnd;
		} else if (eventType == g_strEventMultiTouchEnd) {
			Evas_Event_Multi_Up* ev = (Evas_Event_Multi_Up*)event_info;
			m_touchEventInfo->setPoint(ev->device, TouchPhase::End, ev->canvas.x - x, ev->canvas.y - y);
			timestamp = ev->timestamp;
			action = UIAction::TouchEnd;
		} else {
			return;
		}

		if (action == UIAction::TouchEnd && m_touchEventInfo->getCount() >= 2) {
			action = UIAction::TouchMove;
		}

		Time t;
		t.setMillisecondCount(timestamp);
		Ref<UIEvent> ev = UIEvent::createTouchEvent(action, m_touchEventInfo->getPoints(), t);
		if (ev.isNotNull()) {
			onTouchEvent(ev.get());
		}

		m_touchEventInfo->finishPhases();
	}

	void PlatformViewInstance::_onTouch(void *data, Evas *evas, Evas_Object *obj, void *event_info)
	{
		Ref<PlatformViewInstance> instance = Ref<PlatformViewInstance>::cast(UIPlatform::getViewInstance(obj));
		if (instance.isNotNull()) {
			instance->onTouch((const void*)data, event_info);
		}
	}

	void PlatformViewInstance::installTouchEvents()
	{
		Evas_Object* handle = m_handle;
		if (handle) {
			evas_object_event_callback_add(handle, EVAS_CALLBACK_MOUSE_DOWN, &(PlatformViewInstance::_onTouch), g_strEventTouchBegin);
			evas_object_event_callback_add(handle, EVAS_CALLBACK_MOUSE_UP, &(PlatformViewInstance::_onTouch), g_strEventTouchEnd);
			evas_object_event_callback_add(handle, EVAS_CALLBACK_MOUSE_MOVE, &(PlatformViewInstance::_onTouch), g_strEventTouchMove);
			evas_object_event_callback_add(handle, EVAS_CALLBACK_MULTI_DOWN, &(PlatformViewInstance::_onTouch), g_strEventMultiTouchBegin);
			evas_object_event_callback_add(handle, EVAS_CALLBACK_MULTI_UP, &(PlatformViewInstance::_onTouch), g_strEventMultiTouchEnd);
			evas_object_event_callback_add(handle, EVAS_CALLBACK_MULTI_MOVE, &(PlatformViewInstance::_onTouch), g_strEventMultiTouchMove);
		}
	}

	Ref<ViewInstance> View::createTypicalInstance(ViewInstance* _parent)
	{
		PlatformViewInstance* parent = static_cast<PlatformViewInstance*>(_parent);
		if (parent) {
			Evas_Object* handleParent = parent->getHandle();
			if (handleParent) {
				if (m_flagCreatingChildInstances) {
					Evas_Object* handle = ::elm_grid_add(handleParent);
					if (handle) {
						Ref<PlatformViewInstance> ret = PlatformViewInstance::create<PlatformViewInstance>(this, parent, EFL_ViewType::Grid, handle, sl_true);
						if (ret.isNotNull()) {
							ret->installTouchEvents();
							return ret;
						}
					}
				}
			}
		}
		return sl_null;
	}

	Ref<ViewInstance> UIPlatform::createViewInstance(EFL_ViewType type, Evas_Object* handle, sl_bool flagFreeOnRelease)
	{
		Ref<ViewInstance> ret = UIPlatform::_getViewInstance(handle);
		if (ret.isNotNull()) {
			return ret;
		}
		return PlatformViewInstance::create<PlatformViewInstance>(type, handle, flagFreeOnRelease);
	}

	void UIPlatform::registerViewInstance(Evas_Object* handle, ViewInstance* instance)
	{
		UIPlatform::_registerViewInstance(handle, instance);
	}

	Ref<ViewInstance> UIPlatform::getViewInstance(Evas_Object* handle)
	{
		return UIPlatform::_getViewInstance(handle);
	}

	Ref<View> UIPlatform::getView(Evas_Object* handle)
	{
		Ref<ViewInstance> instance = UIPlatform::_getViewInstance(handle);
		if (instance.isNotNull()) {
			return instance->getView();
		}
		return sl_null;
	}

	void UIPlatform::removeViewInstance(Evas_Object* handle)
	{
		UIPlatform::_removeViewInstance(handle);
	}

	Evas_Object* UIPlatform::getViewHandle(ViewInstance* _instance)
	{
		PlatformViewInstance* instance = static_cast<PlatformViewInstance*>(_instance);
		if (instance) {
			return instance->getHandle();
		}
		return sl_null;
	}

	Evas_Object* UIPlatform::getViewHandle(View* view)
	{
		if (view) {
			Ref<ViewInstance> _instance = view->getViewInstance();
			if (_instance.isNotNull()) {
				PlatformViewInstance* instance = static_cast<PlatformViewInstance*>(_instance.get());
				return instance->getHandle();
			}
		}
		return sl_null;
	}

}

#endif


