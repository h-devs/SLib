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

#include "slib/ui/event.h"
#include "slib/ui/drag.h"

#include "slib/ui/core.h"
#include "slib/ui/view.h"
#include "slib/ui/cursor.h"
#include "slib/graphics/drawable.h"
#include "slib/core/hash_table.h"
#include "slib/core/string_buffer.h"
#include "slib/core/safe_static.h"

namespace slib
{

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(TouchPoint)

	TouchPoint::TouchPoint(): pressure(0), phase(TouchPhase::Move), pointerId(0)
	{
	}

	TouchPoint::TouchPoint(const UIPointF& _point) : point(_point), pressure(0), phase(TouchPhase::Move), pointerId(0)
	{
	}

	TouchPoint::TouchPoint(const UIPointF& _point, sl_real _pressure) : point(_point), pressure(_pressure), phase(TouchPhase::Move), pointerId(0)
	{
	}

	TouchPoint::TouchPoint(const UIPointF& _point, sl_real _pressure, TouchPhase _phase) : point(_point), pressure(_pressure), phase(_phase), pointerId(0)
	{
	}

	TouchPoint::TouchPoint(const UIPointF& _point, sl_real _pressure, TouchPhase _phase, sl_uint64 _pointerId) : point(_point), pressure(_pressure), phase(_phase), pointerId(_pointerId)
	{
	}

	TouchPoint::TouchPoint(sl_ui_posf x, sl_ui_posf y) : point(x, y), pressure(0), phase(TouchPhase::Move), pointerId(0)
	{
	}

	TouchPoint::TouchPoint(sl_ui_posf x, sl_ui_posf y, sl_real _pressure) : point(x, y), pressure(_pressure), phase(TouchPhase::Move), pointerId(0)
	{
	}

	TouchPoint::TouchPoint(sl_ui_posf x, sl_ui_posf y, sl_real _pressure, TouchPhase _phase) : point(x, y), pressure(_pressure), phase(_phase), pointerId(0)
	{
	}

	TouchPoint::TouchPoint(sl_ui_posf x, sl_ui_posf y, sl_real _pressure, TouchPhase _phase, sl_uint64 _pointerId) : point(x, y), pressure(_pressure), phase(_phase), pointerId(_pointerId)
	{
	}


#define DEFINE_MODIFIER_FUNCS(NAME) \
void KeycodeAndModifiers::set##NAME##Key() \
{ \
	SLIB_SET_FLAG(value, Modifiers::NAME); \
} \
void KeycodeAndModifiers::clear##NAME##Key() \
{ \
	SLIB_RESET_FLAG(value, Modifiers::NAME); \
} \
sl_bool KeycodeAndModifiers::is##NAME##Key() const \
{ \
	return SLIB_CHECK_FLAG(value, Modifiers::NAME); \
} \
void UIEvent::set##NAME##Key() \
{ \
	SLIB_SET_FLAG(m_keycodeAndModifiers.value, Modifiers::NAME); \
} \
void UIEvent::clear##NAME##Key() \
{ \
	SLIB_RESET_FLAG(m_keycodeAndModifiers.value, Modifiers::NAME); \
} \
sl_bool UIEvent::is##NAME##Key() const \
{ \
	return SLIB_CHECK_FLAG(m_keycodeAndModifiers.value, Modifiers::NAME); \
}

	DEFINE_MODIFIER_FUNCS(Shift)
	DEFINE_MODIFIER_FUNCS(Alt)
	DEFINE_MODIFIER_FUNCS(Option)
	DEFINE_MODIFIER_FUNCS(Control)
	DEFINE_MODIFIER_FUNCS(Windows)
	DEFINE_MODIFIER_FUNCS(Command)

	KeycodeAndModifiers KeycodeAndModifiers::none()
	{
		return 0;
	}

	Keycode KeycodeAndModifiers::getKeycode() const
	{
		return (Keycode)(value & 0xFFFF);
	}

	void KeycodeAndModifiers::setKeycode(Keycode keycode)
	{
		value = (value & Modifiers::Mask) | (int)(keycode);
	}

	Modifiers KeycodeAndModifiers::getModifiers() const
	{
		return value & Modifiers::Mask;
	}

	void KeycodeAndModifiers::setModifiers(const Modifiers& modifiers)
	{
		value = modifiers | (value & 0xFFFF);
	}

	String KeycodeAndModifiers::toString() const
	{
		StringBuffer sb;
		if (isWindowsKey()) {
#if defined(SLIB_PLATFORM_IS_APPLE)
			sb.addStatic("Command+");
#else
			sb.addStatic("Win+");
#endif
		}
		if (isControlKey()) {
			sb.addStatic("Ctrl+");
		}
		if (isShiftKey()) {
			sb.addStatic("Shift+");
		}
		if (isAltKey()) {
#if defined(SLIB_PLATFORM_IS_APPLE)
			sb.addStatic("Option+");
#else
			sb.addStatic("Alt+");
#endif
		}
		sb.add(UIEvent::getKeyName(getKeycode(), sl_true));
		return sb.merge();
	}

	sl_bool KeycodeAndModifiers::parse(const StringParam& _str)
	{
		StringData str(_str);
		KeycodeAndModifiers km;
		ListElements<StringView> list(str.split("+"));
		for (sl_size i = 0; i < list.count; i++) {
			if (km.getKeycode() != Keycode::Unknown) {
				return sl_false;
			}
			String s = list[i].toLower();
			if (s == "control" || s == "ctrl") {
				km |= Modifiers::Control;
			} else if (s == "shift") {
				km |= Modifiers::Shift;
			} else if (s == "alt" || s == "option") {
				km |= Modifiers::Alt;
			} else if (s == "command" || s == "window" || s == "win") {
				km |= Modifiers::Windows;
			} else {
				Keycode keycode = UIEvent::getKeycodeFromName(s);
				if (keycode == Keycode::Unknown) {
					return sl_false;
				}
				km.setKeycode(keycode);
			}
		}
		if (km.getKeycode() == Keycode::Unknown) {
			return sl_false;
		}
		*this = km;
		return sl_true;
	}

	KeycodeAndModifiers& KeycodeAndModifiers::operator|=(int modifiers)
	{
		value |= modifiers;
		return *this;
	}

	KeycodeAndModifiers KeycodeAndModifiers::operator|(int modifiers)
	{
		return KeycodeAndModifiers(value | modifiers);
	}

	KeycodeAndModifiers operator|(int modifiers, const KeycodeAndModifiers& km)
	{
		return KeycodeAndModifiers(km.value | modifiers);
	}

	KeycodeAndModifiers operator|(Keycode keycode, int modifiers)
	{
		return KeycodeAndModifiers(keycode, modifiers);
	}

	KeycodeAndModifiers operator|(int modifiers, Keycode keycode)
	{
		return KeycodeAndModifiers(keycode, modifiers);
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(DragItem)

	DragItem::DragItem(): m_frame(0, 0, 1, 1)
	{
	}

	void DragItem::clear()
	{
		m_text.setNull();
		m_files.setNull();
	}

	const String& DragItem::getText() const
	{
		return m_text;
	}

	void DragItem::setText(const String& text)
	{
		m_text = text;
	}

	const List<String>& DragItem::getFiles() const
	{
		return m_files;
	}

	void DragItem::setFiles(const List<String>& files)
	{
		m_files = files;
	}

	const UIRect& DragItem::getFrame() const
	{
		return m_frame;
	}

	void DragItem::setFrame(const UIRect& frame)
	{
		m_frame = frame;
	}

	void DragItem::setDraggingSize(sl_ui_pos width, sl_ui_pos height)
	{
		m_frame.setSize(width, height);
	}

	const Ref<Drawable>& DragItem::getDraggingImage() const
	{
		return m_image;
	}

	void DragItem::setDraggingImage(const Ref<Drawable>& image)
	{
		m_image = image;
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(DragContext)

	DragContext::DragContext(): operation(0), operationMask(DragOperations::All)
	{
	}

	sl_bool DragContext::isAlive() const
	{
		return view.isNotNull();
	}

	void DragContext::release()
	{
		view.setNull();
	}


	SLIB_DEFINE_ROOT_OBJECT(UIEvent)

	UIEvent::UIEvent() : m_flags(0), m_action(UIAction::Unknown), m_time(0)
	{
	}

	UIEvent::UIEvent(UIAction action, const Time& time) : m_flags(0), m_action(action), m_time(time)
	{
	}

	UIEvent::UIEvent(UIAction action, const UIEventFlags& flags, const Time& time) : m_flags(flags), m_action(action), m_time(time)
	{
	}

	UIEvent::~UIEvent()
	{
	}

	Ref<UIEvent> UIEvent::createUnknown(const UIEventFlags& flags, const Time& time)
	{
		return new UIEvent(UIAction::Unknown, flags, time);
	}

	Ref<UIEvent> UIEvent::createUnknown(const Time& time)
	{
		return new UIEvent(UIAction::Unknown, time);
	}
	namespace {
		class KeyboardEvent : public UIEvent
		{
			SLIB_DECLARE_OBJECT

		public:
			sl_uint32 m_code;

		public:
			KeyboardEvent(UIAction action, const Time& time, sl_uint32 code): UIEvent(action, time), m_code(code)
			{
			}

		public:
			Ref<UIEvent> duplicate() const override
			{
				KeyboardEvent* ret = new KeyboardEvent(m_action, m_time, m_code);
				if (ret) {
					ret->_copyProperties(this);
					return ret;
				}
				return sl_null;
			}

		};

		SLIB_DEFINE_OBJECT(KeyboardEvent, UIEvent)
	}

	Ref<UIEvent> UIEvent::createKeyEvent(UIAction action, Keycode keycode, sl_uint32 systemKeycode, const Time& time)
	{
		Ref<KeyboardEvent> ret = new KeyboardEvent(action, time, systemKeycode);
		if (ret.isNotNull()) {
			ret->setKeycode(keycode);
			return Ref<UIEvent>::from(ret);
		}
		return sl_null;
	}

	namespace {
		class MouseEvent : public UIEvent
		{
			SLIB_DECLARE_OBJECT

		public:
			TouchPoint m_pt;

		public:
			MouseEvent(UIAction action, const Time& time, sl_ui_posf x, sl_ui_posf y): UIEvent(action, time), m_pt(x, y)
			{
			}

			MouseEvent(UIAction action, const Time& time, const TouchPoint& pt): UIEvent(action, time), m_pt(pt)
			{
			}

		public:
			Ref<UIEvent> duplicate() const override
			{
				MouseEvent* ret = new MouseEvent(m_action, m_time, m_pt);
				if (ret) {
					ret->_copyProperties(this);
					return ret;
				}
				return sl_null;
			}

		};

		SLIB_DEFINE_OBJECT(MouseEvent, UIEvent)

		class MouseEventWithDelta : public MouseEvent
		{
			SLIB_DECLARE_OBJECT

		public:
			sl_real m_deltaX;
			sl_real m_deltaY;

		public:
			MouseEventWithDelta(UIAction action, const Time& time, sl_ui_posf x, sl_ui_posf y, sl_real deltaX, sl_real deltaY): MouseEvent(action, time, x, y), m_deltaX(deltaX), m_deltaY(deltaY)
			{
			}

		public:
			Ref<UIEvent> duplicate() const override
			{
				MouseEventWithDelta* ret = new MouseEventWithDelta(m_action, m_time, m_pt.point.x, m_pt.point.y, m_deltaX, m_deltaY);
				if (ret) {
					ret->_copyProperties(this);
					return ret;
				}
				return sl_null;
			}

		};

		SLIB_DEFINE_OBJECT(MouseEventWithDelta, MouseEvent)

	}

	Ref<UIEvent> UIEvent::createMouseEvent(UIAction action, sl_ui_posf x, sl_ui_posf y, const Time& time)
	{
		return new MouseEvent(action, time, x, y);
	}

	Ref<UIEvent> UIEvent::createMouseEvent(UIAction action, sl_ui_posf x, sl_ui_posf y, sl_real deltaX, sl_real deltaY, const Time& time)
	{
		return new MouseEventWithDelta(action, time, x, y, deltaX, deltaY);
	}

	Ref<UIEvent> UIEvent::createMouseWheelEvent(sl_ui_posf mouseX, sl_ui_posf mouseY, sl_real deltaX, sl_real deltaY, const Time& time)
	{
		return new MouseEventWithDelta(UIAction::MouseWheel, time, mouseX, mouseY, deltaX, deltaY);
	}

	namespace {
		class TouchEvent : public MouseEvent
		{
			SLIB_DECLARE_OBJECT

		public:
			Array<TouchPoint> m_points;

		public:
			TouchEvent(UIAction action, const Time& time, const TouchPoint& pt) : MouseEvent(action, time, pt)
			{
			}

			TouchEvent(UIAction action, const Time& time, const Array<TouchPoint>& points) : MouseEvent(action, time, 0, 0), m_points(points)
			{
				if (points.getCount()) {
					m_pt = points[0];
				}
			}

		public:
			Ref<UIEvent> duplicate() const override
			{
				if (m_points.isNotNull()) {
					TouchEvent* ret = new TouchEvent(m_action, m_time, m_points.duplicate());
					if (ret) {
						ret->_copyProperties(this);
						ret->m_pt = m_pt;
						return ret;
					}
				} else {
					TouchEvent* ret = new TouchEvent(m_action, m_time, m_pt);
					if (ret) {
						ret->_copyProperties(this);
						return ret;
					}
				}
				return sl_null;
			}

		};

		SLIB_DEFINE_OBJECT(TouchEvent, MouseEvent)
	}

	Ref<UIEvent> UIEvent::createTouchEvent(UIAction action, const Array<TouchPoint>& points, const Time& time)
	{
		return new TouchEvent(action, time, points);
	}

	Ref<UIEvent> UIEvent::createTouchEvent(UIAction action, const TouchPoint& point, const Time& time)
	{
		return new TouchEvent(action, time, Array<TouchPoint>::create(&point, 1));
	}

	namespace {
		class SetCursorEvent : public MouseEvent
		{
			SLIB_DECLARE_OBJECT

		public:
			Ref<Cursor> cursor;
			String toolTip;
			sl_uint64 toolTipOwnerId;

		public:
			SetCursorEvent(const Time& time, sl_ui_posf x, sl_ui_posf y): MouseEvent(UIAction::SetCursor, time, x, y)
			{
			}

		public:
			Ref<UIEvent> duplicate() const override
			{
				SetCursorEvent* ret = new SetCursorEvent(m_time, m_pt.point.x, m_pt.point.y);
				if (ret) {
					ret->_copyProperties(this);
					return ret;
				}
				return sl_null;
			}

		};

		SLIB_DEFINE_OBJECT(SetCursorEvent, MouseEvent)
	}

	Ref<UIEvent> UIEvent::createSetCursorEvent(sl_ui_posf x, sl_ui_posf y, const Time& time)
	{
		return new SetCursorEvent(time, x, y);
	}

	namespace {
		class DragEvent : public MouseEvent
		{
			SLIB_DECLARE_OBJECT

		public:
			DragContext m_context;

		public:
			DragEvent(UIAction action, const Time& time, sl_ui_posf x, sl_ui_posf y, const DragContext& context) : MouseEvent(action, time, x, y), m_context(context)
			{
			}

		public:
			Ref<UIEvent> duplicate() const override
			{
				DragEvent* ret = new DragEvent(m_action, m_time, m_pt.point.x, m_pt.point.y, m_context);
				if (ret) {
					ret->_copyProperties(this);
					return ret;
				}
				return sl_null;
			}

		};

		SLIB_DEFINE_OBJECT(DragEvent, MouseEvent)
	}

	Ref<UIEvent> UIEvent::createDragEvent(UIAction action, sl_ui_posf x, sl_ui_posf y, const DragContext& context, const Time& time)
	{
		return new DragEvent(action, time, x, y, context);
	}

	UIAction UIEvent::getAction() const
	{
		return m_action;
	}

	void UIEvent::setAction(UIAction action)
	{
		m_action = action;
	}

	sl_bool UIEvent::isKeyEvent() const
	{
		return ((sl_uint32)m_action & SLIB_UI_ACTION_TYPE_KEYBOARD) == SLIB_UI_ACTION_TYPE_KEYBOARD;
	}

	sl_bool UIEvent::isMouseEvent() const
	{
		return ((sl_uint32)m_action & SLIB_UI_ACTION_TYPE_MOUSE) == SLIB_UI_ACTION_TYPE_MOUSE;
	}

	sl_bool UIEvent::isTouchEvent() const
	{
		return ((sl_uint32)m_action & SLIB_UI_ACTION_TYPE_TOUCH) == SLIB_UI_ACTION_TYPE_TOUCH;
	}

	sl_bool UIEvent::isDragEvent() const
	{
		return ((sl_uint32)m_action & SLIB_UI_ACTION_TYPE_DRAG) == SLIB_UI_ACTION_TYPE_DRAG;
	}

	Time UIEvent::getTime() const
	{
		return m_time;
	}

	void UIEvent::setTime(const Time& time)
	{
		m_time = time;
	}

	const KeycodeAndModifiers& UIEvent::getKeycodeAndModifiers() const
	{
		return m_keycodeAndModifiers;
	}

	void UIEvent::setKeycodeAndModifiers(const KeycodeAndModifiers& km)
	{
		m_keycodeAndModifiers = km;
	}

	Keycode UIEvent::getKeycode() const
	{
		return m_keycodeAndModifiers.getKeycode();
	}

	void UIEvent::setKeycode(Keycode keycode)
	{
		m_keycodeAndModifiers.setKeycode(keycode);
	}

	Modifiers UIEvent::getModifiers() const
	{
		return m_keycodeAndModifiers.getModifiers();
	}

	void UIEvent::setModifiers(const Modifiers& modifiers)
	{
		m_keycodeAndModifiers.setModifiers(modifiers);
	}

	sl_uint32 UIEvent::getSystemKeycode() const
	{
		if (IsInstanceOf<KeyboardEvent>(this)) {
			return ((KeyboardEvent*)this)->m_code;
		}
		return 0;
	}

	void UIEvent::setSystemKeycode(sl_uint32 keycode)
	{
		if (IsInstanceOf<KeyboardEvent>(this)) {
			((KeyboardEvent*)this)->m_code = keycode;
		}
	}

	sl_char32 UIEvent::getChar() const
	{
		if (IsInstanceOf<KeyboardEvent>(this)) {
			return ((KeyboardEvent*)this)->m_code;
		}
		return 0;
	}

	void UIEvent::setChar(sl_char32 code)
	{
		if (IsInstanceOf<KeyboardEvent>(this)) {
			((KeyboardEvent*)this)->m_code = code;
		}
	}

	const UIPointF& UIEvent::getPoint() const
	{
		if (IsInstanceOf<MouseEvent>(this)) {
			return ((MouseEvent*)this)->m_pt.point;
		}
		return UIPointF::zero();
	}

	void UIEvent::setPoint(const UIPointF& pt)
	{
		if (IsInstanceOf<MouseEvent>(this)) {
			((MouseEvent*)this)->m_pt.point = pt;
		}
	}

	void UIEvent::setPoint(sl_ui_posf x, sl_ui_posf y)
	{
		if (IsInstanceOf<MouseEvent>(this)) {
			((MouseEvent*)this)->m_pt.point.x = x;
			((MouseEvent*)this)->m_pt.point.y = y;
		}
	}

	sl_ui_posf UIEvent::getX() const
	{
		if (IsInstanceOf<MouseEvent>(this)) {
			return ((MouseEvent*)this)->m_pt.point.x;
		}
		return 0;
	}

	void UIEvent::setX(sl_ui_posf x)
	{
		if (IsInstanceOf<MouseEvent>(this)) {
			((MouseEvent*)this)->m_pt.point.x = x;
		}
	}

	sl_ui_posf UIEvent::getY() const
	{
		if (IsInstanceOf<MouseEvent>(this)) {
			return ((MouseEvent*)this)->m_pt.point.y;
		}
		return 0;
	}

	void UIEvent::setY(sl_ui_posf y)
	{
		if (IsInstanceOf<MouseEvent>(this)) {
			((MouseEvent*)this)->m_pt.point.y = y;
		}
	}

	sl_real UIEvent::getDelta() const
	{
		if (IsInstanceOf<MouseEventWithDelta>(this)) {
			MouseEventWithDelta* ev = (MouseEventWithDelta*)this;
			if (Math::abs(ev->m_deltaY) > Math::abs(ev->m_deltaX)) {
				return ev->m_deltaY;
			} else {
				return ev->m_deltaX;
			}
		}
		return 0;
	}

	sl_real UIEvent::getDeltaX() const
	{
		if (IsInstanceOf<MouseEventWithDelta>(this)) {
			return ((MouseEventWithDelta*)this)->m_deltaX;
		}
		return 0;
	}

	void UIEvent::setDeltaX(sl_real x)
	{
		if (IsInstanceOf<MouseEventWithDelta>(this)) {
			((MouseEventWithDelta*)this)->m_deltaX = x;
		}
	}

	sl_real UIEvent::getDeltaY() const
	{
		if (IsInstanceOf<MouseEventWithDelta>(this)) {
			return ((MouseEventWithDelta*)this)->m_deltaY;
		}
		return 0;
	}

	void UIEvent::setDeltaY(sl_real y)
	{
		if (IsInstanceOf<MouseEventWithDelta>(this)) {
			((MouseEventWithDelta*)this)->m_deltaY = y;
		}
	}

	const TouchPoint& UIEvent::getTouchPoint() const
	{
		if (IsInstanceOf<MouseEvent>(this)) {
			return ((MouseEvent*)this)->m_pt;
		}
		static const char zero[sizeof(TouchPoint)] = {0};
		return *((TouchPoint*)(void*)zero);
	}

	void UIEvent::setTouchPoint(const TouchPoint& pt)
	{
		if (IsInstanceOf<MouseEvent>(this)) {
			((MouseEvent*)this)->m_pt = pt;
		}
	}

	void UIEvent::setTouchPoint(const UIPointF& pt)
	{
		if (IsInstanceOf<MouseEvent>(this)) {
			((MouseEvent*)this)->m_pt.point = pt;
			((MouseEvent*)this)->m_pt.pressure = 0;
		}
	}

	void UIEvent::setTouchPoint(const UIPointF& pt, sl_real pressure)
	{
		if (IsInstanceOf<MouseEvent>(this)) {
			((MouseEvent*)this)->m_pt.point = pt;
			((MouseEvent*)this)->m_pt.pressure = pressure;
		}
	}

	void UIEvent::setTouchPoint(sl_ui_posf x, sl_ui_posf y)
	{
		if (IsInstanceOf<MouseEvent>(this)) {
			((MouseEvent*)this)->m_pt.point.x = x;
			((MouseEvent*)this)->m_pt.point.y = y;
			((MouseEvent*)this)->m_pt.pressure = 0;
		}
	}

	void UIEvent::setTouchPoint(sl_ui_posf x, sl_ui_posf y, sl_real pressure)
	{
		if (IsInstanceOf<MouseEvent>(this)) {
			((MouseEvent*)this)->m_pt.point.x = x;
			((MouseEvent*)this)->m_pt.point.y = y;
			((MouseEvent*)this)->m_pt.pressure = pressure;
		}
	}

	sl_real UIEvent::getPressure() const
	{
		if (IsInstanceOf<MouseEvent>(this)) {
			return ((MouseEvent*)this)->m_pt.pressure;
		}
		return 0;
	}

	void UIEvent::setPressure(sl_real pressure)
	{
		if (IsInstanceOf<MouseEvent>(this)) {
			((MouseEvent*)this)->m_pt.pressure = pressure;
		}
	}

	const Array<TouchPoint>& UIEvent::getTouchPoints() const
	{
		if (IsInstanceOf<TouchEvent>(this)) {
			return ((TouchEvent*)this)->m_points;
		}
		return Array<TouchPoint>::null();
	}

	sl_uint32 UIEvent::getTouchPointCount() const
	{
		if (IsInstanceOf<TouchEvent>(this)) {
			return (sl_uint32)(((TouchEvent*)this)->m_points.getCount());
		}
		return 0;
	}

	const TouchPoint& UIEvent::getTouchPoint(sl_uint32 index) const
	{
		if (IsInstanceOf<TouchEvent>(this)) {
			if (index < ((TouchEvent*)this)->m_points.getCount()) {
				return ((TouchEvent*)this)->m_points[index];
			} else {
				return ((TouchEvent*)this)->m_pt;
			}
		}
		return getTouchPoint();
	}

	void UIEvent::setTouchPoints(const Array<TouchPoint>& points)
	{
		if (IsInstanceOf<TouchEvent>(this)) {
			((TouchEvent*)this)->m_points = points;
		}
	}

	void UIEvent::transformPoints(const Matrix3T<float>& mat)
	{
		if (IsInstanceOf<MouseEvent>(this)) {
			((MouseEvent*)this)->m_pt.point = mat.transformPosition(((MouseEvent*)this)->m_pt.point);
		}
		if (IsInstanceOf<TouchEvent>(this)) {
			Array<TouchPoint>& points = ((TouchEvent*)this)->m_points;
			sl_size n = points.getCount();
			TouchPoint* pts = points.getData();
			for (sl_size i = 0; i < n; i++) {
				pts[i].point = mat.transformPosition(pts[i].point);
			}
		}
	}

	void UIEvent::transformPoints(const Matrix3T<double>& mat)
	{
		if (IsInstanceOf<MouseEvent>(this)) {
			((MouseEvent*)this)->m_pt.point = mat.transformPosition(((MouseEvent*)this)->m_pt.point);
		}
		if (IsInstanceOf<TouchEvent>(this)) {
			Array<TouchPoint>& points = ((TouchEvent*)this)->m_points;
			sl_size n = points.getCount();
			TouchPoint* pts = points.getData();
			for (sl_size i = 0; i < n; i++) {
				pts[i].point = mat.transformPosition(pts[i].point);
			}
		}
	}

	const Ref<Cursor>& UIEvent::getCursor() const
	{
		if (IsInstanceOf<SetCursorEvent>(this)) {
			return ((SetCursorEvent*)this)->cursor;
		}
		return Ref<Cursor>::null();
	}

	void UIEvent::setCursor(const Ref<Cursor>& cursor)
	{
		if (IsInstanceOf<SetCursorEvent>(this)) {
			((SetCursorEvent*)this)->cursor = cursor;
		}
	}

	const String& UIEvent::getToolTip() const
	{
		if (IsInstanceOf<SetCursorEvent>(this)) {
			return ((SetCursorEvent*)this)->toolTip;
		}
		return String::null();
	}

	sl_uint64 UIEvent::getToolTipOwnerId() const
	{
		if (IsInstanceOf<SetCursorEvent>(this)) {
			return ((SetCursorEvent*)this)->toolTipOwnerId;
		}
		return 0;
	}

	void UIEvent::setToolTip(sl_uint64 ownerId, const String& toolTip)
	{
		if (IsInstanceOf<SetCursorEvent>(this)) {
			((SetCursorEvent*)this)->toolTip = toolTip;
			((SetCursorEvent*)this)->toolTipOwnerId = ownerId;
		}
	}

	const DragItem& UIEvent::getDragItem() const
	{
		if (IsInstanceOf<DragEvent>(this)) {
			return ((DragEvent*)this)->m_context.item;
		}
		SLIB_SAFE_LOCAL_STATIC(DragItem, item);
		return item;
	}

	void UIEvent::setDragItem(const DragItem& item)
	{
		if (IsInstanceOf<DragEvent>(this)) {
			((DragEvent*)this)->m_context.item = item;
		}
	}

	DragOperations UIEvent::getDragOperationMask() const
	{
		if (IsInstanceOf<DragEvent>(this)) {
			return ((DragEvent*)this)->m_context.operationMask;
		}
		return 0;
	}

	void UIEvent::setDragOperationMask(const DragOperations& mask)
	{
		if (IsInstanceOf<DragEvent>(this)) {
			((DragEvent*)this)->m_context.operationMask = mask;
		}
	}

	DragOperations UIEvent::getDragOperation() const
	{
		if (IsInstanceOf<DragEvent>(this)) {
			return ((DragEvent*)this)->m_context.operation;
		}
		return 0;
	}

	void UIEvent::setDragOperation(const DragOperations& op)
	{
		if (IsInstanceOf<DragEvent>(this)) {
			((DragEvent*)this)->m_context.operation = op;
		}
	}

	UIEventFlags UIEvent::getFlags() const
	{
		return m_flags;
	}

	void UIEvent::setFlags(const UIEventFlags& flags)
	{
		m_flags = flags;
	}

	void UIEvent::addFlag(const UIEventFlags& flags)
	{
		SLIB_SET_FLAG(m_flags, flags);
	}

	void UIEvent::removeFlag(const UIEventFlags& flags)
	{
		SLIB_RESET_FLAG(m_flags, flags);
	}

	void UIEvent::accept()
	{
		SLIB_SET_FLAG(m_flags, UIEventFlags::Accepted);
		SLIB_SET_FLAG(m_flags, UIEventFlags::NotInvokeNative);
	}

	void UIEvent::acceptByNative()
	{
		SLIB_SET_FLAG(m_flags, UIEventFlags::Accepted);
	}

	sl_bool UIEvent::isAccepted() const
	{
		return SLIB_CHECK_FLAG(m_flags, UIEventFlags::Accepted);
	}

	void UIEvent::setAccepted(sl_bool flag)
	{
		if (flag) {
			SLIB_SET_FLAG(m_flags, UIEventFlags::Accepted);
		} else {
			SLIB_RESET_FLAG(m_flags, UIEventFlags::Accepted);
		}
	}

	void UIEvent::passToNext()
	{
		SLIB_SET_FLAG(m_flags, UIEventFlags::PassToNext);
	}

	sl_bool UIEvent::isPassedToNext()
	{
		return SLIB_CHECK_FLAG(m_flags, UIEventFlags::PassToNext);
	}

	void UIEvent::setPassedToNext(sl_bool flag)
	{
		if (flag) {
			SLIB_SET_FLAG(m_flags, UIEventFlags::PassToNext);
		} else {
			SLIB_RESET_FLAG(m_flags, UIEventFlags::PassToNext);
		}
	}

	Ref<UIEvent> UIEvent::duplicate() const
	{
		Ref<UIEvent> ret = new UIEvent(m_action, m_time);
		if (ret.isNotNull()) {
			ret->_copyProperties(this);
			return ret;
		}
		return sl_null;
	}

	void UIEvent::_copyProperties(const UIEvent* other)
	{
		m_flags = other->m_flags;
		m_keycodeAndModifiers = other->m_keycodeAndModifiers;
	}

	namespace {

		class KeyNameMapper
		{
		private:
			HashTable<sl_uint32, String> mapLong;
			HashTable<sl_uint32, String> mapShort;
			HashTable<String, sl_uint32> mapName;
			String nameInvalid;

		public:

#define PRIV_MAP_KEY(NAME) \
			{ \
				SLIB_STATIC_STRING(_s, #NAME); \
				mapLong.put((sl_uint32)(Keycode::NAME), _s); \
				mapShort.put((sl_uint32)(Keycode::NAME), _s); \
				mapName.put(_s.toLower(), (sl_uint32)(Keycode::NAME)); \
			}
#define PRIV_MAP_KEY2(NAME, SHORT_NAME) \
			{ \
				SLIB_STATIC_STRING(_s1, #NAME); \
				mapLong.put((sl_uint32)(Keycode::NAME), _s1); \
				SLIB_STATIC_STRING(_s2, SHORT_NAME); \
				mapShort.put((sl_uint32)(Keycode::NAME), _s2); \
				mapName.put(_s1.toLower(), (sl_uint32)(Keycode::NAME)); \
				mapName.put(_s2.toLower(), (sl_uint32)(Keycode::NAME)); \
			}

			KeyNameMapper()
			{
				SLIB_STATIC_STRING(_invalid, "Invalid")
				nameInvalid = _invalid;
				PRIV_MAP_KEY(Unknown);

				PRIV_MAP_KEY2(Backspace, "Back")
				PRIV_MAP_KEY(Tab)
				PRIV_MAP_KEY(Enter)
				PRIV_MAP_KEY2(Escape, "Esc")

				PRIV_MAP_KEY(Space)
				PRIV_MAP_KEY2(Grave, "`")
				PRIV_MAP_KEY2(Equal, "=")
				PRIV_MAP_KEY2(Semicolon, ";")
				PRIV_MAP_KEY2(Backslash, "\\")
				PRIV_MAP_KEY2(LeftBaracket, "[")
				PRIV_MAP_KEY2(RightBaracket, "]")
				PRIV_MAP_KEY2(Quote, "'")
				PRIV_MAP_KEY2(Comma, ",")
				PRIV_MAP_KEY2(Minus, "-")
				PRIV_MAP_KEY2(Period, ".")
				PRIV_MAP_KEY2(Divide, "/")

				PRIV_MAP_KEY2(Num0, "0")
				PRIV_MAP_KEY2(Num1, "1")
				PRIV_MAP_KEY2(Num2, "2")
				PRIV_MAP_KEY2(Num3, "3")
				PRIV_MAP_KEY2(Num4, "4")
				PRIV_MAP_KEY2(Num5, "5")
				PRIV_MAP_KEY2(Num6, "6")
				PRIV_MAP_KEY2(Num7, "7")
				PRIV_MAP_KEY2(Num8, "8")
				PRIV_MAP_KEY2(Num9, "9")

				PRIV_MAP_KEY(A)
				PRIV_MAP_KEY(B)
				PRIV_MAP_KEY(C)
				PRIV_MAP_KEY(D)
				PRIV_MAP_KEY(E)
				PRIV_MAP_KEY(F)
				PRIV_MAP_KEY(G)
				PRIV_MAP_KEY(H)
				PRIV_MAP_KEY(I)
				PRIV_MAP_KEY(J)
				PRIV_MAP_KEY(K)
				PRIV_MAP_KEY(L)
				PRIV_MAP_KEY(M)
				PRIV_MAP_KEY(N)
				PRIV_MAP_KEY(O)
				PRIV_MAP_KEY(P)
				PRIV_MAP_KEY(Q)
				PRIV_MAP_KEY(R)
				PRIV_MAP_KEY(S)
				PRIV_MAP_KEY(T)
				PRIV_MAP_KEY(U)
				PRIV_MAP_KEY(V)
				PRIV_MAP_KEY(W)
				PRIV_MAP_KEY(X)
				PRIV_MAP_KEY(Y)
				PRIV_MAP_KEY(Z)

				PRIV_MAP_KEY(Numpad0)
				PRIV_MAP_KEY(Numpad1)
				PRIV_MAP_KEY(Numpad2)
				PRIV_MAP_KEY(Numpad3)
				PRIV_MAP_KEY(Numpad4)
				PRIV_MAP_KEY(Numpad5)
				PRIV_MAP_KEY(Numpad6)
				PRIV_MAP_KEY(Numpad7)
				PRIV_MAP_KEY(Numpad8)
				PRIV_MAP_KEY(Numpad9)

				PRIV_MAP_KEY2(NumpadDivide, "Numpad/")
				PRIV_MAP_KEY2(NumpadMultiply, "Numpad*")
				PRIV_MAP_KEY2(NumpadMinus, "Numpad-")
				PRIV_MAP_KEY2(NumpadPlus, "Numpad+")
				PRIV_MAP_KEY2(NumpadEnter, "NumpadEnter")
				PRIV_MAP_KEY2(NumpadDecimal, "Numpad.")

				PRIV_MAP_KEY(F1)
				PRIV_MAP_KEY(F2)
				PRIV_MAP_KEY(F3)
				PRIV_MAP_KEY(F4)
				PRIV_MAP_KEY(F5)
				PRIV_MAP_KEY(F6)
				PRIV_MAP_KEY(F7)
				PRIV_MAP_KEY(F8)
				PRIV_MAP_KEY(F9)
				PRIV_MAP_KEY(F10)
				PRIV_MAP_KEY(F11)
				PRIV_MAP_KEY(F12)

				PRIV_MAP_KEY2(PageUp, "PgUp")
				PRIV_MAP_KEY2(PageDown, "PgDn")
				PRIV_MAP_KEY(Home)
				PRIV_MAP_KEY(End)
				PRIV_MAP_KEY(Left)
				PRIV_MAP_KEY(Up)
				PRIV_MAP_KEY(Right)
				PRIV_MAP_KEY(Down)
				PRIV_MAP_KEY2(PrintScreen, "PrtSc")
				PRIV_MAP_KEY2(Insert, "Ins")
				PRIV_MAP_KEY2(Delete, "Del")
				PRIV_MAP_KEY(Sleep)
				PRIV_MAP_KEY(Pause)

				PRIV_MAP_KEY(GoHome)
				PRIV_MAP_KEY(GoMenu)
				PRIV_MAP_KEY(GoBack)
				PRIV_MAP_KEY(Camera)
				PRIV_MAP_KEY(VolumeMute)
				PRIV_MAP_KEY(VolumeDown)
				PRIV_MAP_KEY(VolumeUp)
				PRIV_MAP_KEY(MediaPrev)
				PRIV_MAP_KEY(MediaNext)
				PRIV_MAP_KEY(MediaPause)
				PRIV_MAP_KEY(MediaStop)
				PRIV_MAP_KEY2(PhoneStar, "Dial*")
				PRIV_MAP_KEY2(PhonePound, "Dial#")

				PRIV_MAP_KEY2(LeftShift, "LShift")
				PRIV_MAP_KEY2(RightShift, "RShift")
				PRIV_MAP_KEY2(LeftControl, "LCtrl")
				PRIV_MAP_KEY2(RightControl, "RCtrl")
#if defined(SLIB_PLATFORM_IS_APPLE)
				PRIV_MAP_KEY2(LeftAlt, "LAlt")
				PRIV_MAP_KEY2(RightAlt, "RAlt")
				PRIV_MAP_KEY2(LeftWin, "LWin")
				PRIV_MAP_KEY2(RightWin, "RWin")
				PRIV_MAP_KEY2(LeftOption, "LAlt")
				PRIV_MAP_KEY2(RightOption, "RAlt")
				PRIV_MAP_KEY2(LeftCommand, "LCmd")
				PRIV_MAP_KEY2(RightCommand, "RCmd")
#else
				PRIV_MAP_KEY2(LeftOption, "LAlt")
				PRIV_MAP_KEY2(RightOption, "RAlt")
				PRIV_MAP_KEY2(LeftCommand, "LCmd")
				PRIV_MAP_KEY2(RightCommand, "RCmd")
				PRIV_MAP_KEY2(LeftAlt, "LAlt")
				PRIV_MAP_KEY2(RightAlt, "RAlt")
				PRIV_MAP_KEY2(LeftWin, "LWin")
				PRIV_MAP_KEY2(RightWin, "RWin")
#endif
				PRIV_MAP_KEY(CapsLock)
				PRIV_MAP_KEY(ScrollLock)
				PRIV_MAP_KEY(NumLock)
				PRIV_MAP_KEY(ContextMenu)

				PRIV_MAP_KEY(Chinese)
				PRIV_MAP_KEY(Korean)
			}

			String get(Keycode code, sl_bool flagShort)
			{
				String ret;
				if (flagShort) {
					if (mapShort.get((sl_uint32)code, &ret)) {
						return ret;
					}
				} else {
					if (mapLong.get((sl_uint32)code, &ret)) {
						return ret;
					}
				}
				return nameInvalid;
			}

			Keycode getCode(const String& keyName)
			{
				sl_uint32 keycode;
				if (mapName.get(keyName.toLower(), &keycode)) {
					return (Keycode)keycode;
				}
				return Keycode::Unknown;
			}

		};

		SLIB_SAFE_STATIC_GETTER(KeyNameMapper, GetKeyNameMapper)
	}

	String UIEvent::getKeyName(Keycode code, sl_bool flagShort)
	{
		KeyNameMapper* mapper = GetKeyNameMapper();
		if (mapper) {
			return mapper->get(code, flagShort);
		}
		return sl_null;
	}

	Keycode UIEvent::getKeycodeFromName(const String& keyName)
	{
		KeyNameMapper* mapper = GetKeyNameMapper();
		if (mapper) {
			return mapper->getCode(keyName);
		}
		return Keycode::Unknown;
	}

#define KEYCODE_CHAR_MAPPING(CODE, ch) \
	case Keycode::CODE: \
		return ch;
#define KEYCODE_CHAR_MAPPING2(CODE, chLower, chUpper) \
	case Keycode::CODE: \
		if (flagUpper) { \
			return chUpper; \
		} else { \
			return chLower; \
		}

	sl_char8 UIEvent::getCharFromKeycode(Keycode code, sl_bool flagUpper)
	{
		switch (code) {
			KEYCODE_CHAR_MAPPING(Tab, '\t')
			KEYCODE_CHAR_MAPPING(Enter, '\n')
			KEYCODE_CHAR_MAPPING(Space, ' ')
			KEYCODE_CHAR_MAPPING2(Grave, '`', '~')
			KEYCODE_CHAR_MAPPING2(Equal, '=', '+')
			KEYCODE_CHAR_MAPPING2(Semicolon, ';', ':')
			KEYCODE_CHAR_MAPPING2(Backslash, '\\', '|')
			KEYCODE_CHAR_MAPPING2(LeftBaracket, '[', '{')
			KEYCODE_CHAR_MAPPING2(RightBaracket, ']', '}')
			KEYCODE_CHAR_MAPPING2(Quote, '\'', '"')
			KEYCODE_CHAR_MAPPING2(Comma, ',', '<')
			KEYCODE_CHAR_MAPPING2(Minus, '-', '_')
			KEYCODE_CHAR_MAPPING2(Period, '.', '>')
			KEYCODE_CHAR_MAPPING2(Divide, '/', '?')
			KEYCODE_CHAR_MAPPING2(Num0, '0', ')')
			KEYCODE_CHAR_MAPPING2(Num1, '1', '!')
			KEYCODE_CHAR_MAPPING2(Num2, '2', '@')
			KEYCODE_CHAR_MAPPING2(Num3, '3', '#')
			KEYCODE_CHAR_MAPPING2(Num4, '4', '$')
			KEYCODE_CHAR_MAPPING2(Num5, '5', '%')
			KEYCODE_CHAR_MAPPING2(Num6, '6', '^')
			KEYCODE_CHAR_MAPPING2(Num7, '7', '&')
			KEYCODE_CHAR_MAPPING2(Num8, '8', '*')
			KEYCODE_CHAR_MAPPING2(Num9, '9', '(')
			KEYCODE_CHAR_MAPPING2(A, 'a', 'A')
			KEYCODE_CHAR_MAPPING2(B, 'b', 'B')
			KEYCODE_CHAR_MAPPING2(C, 'c', 'C')
			KEYCODE_CHAR_MAPPING2(D, 'd', 'D')
			KEYCODE_CHAR_MAPPING2(E, 'e', 'E')
			KEYCODE_CHAR_MAPPING2(F, 'f', 'F')
			KEYCODE_CHAR_MAPPING2(G, 'g', 'G')
			KEYCODE_CHAR_MAPPING2(H, 'h', 'H')
			KEYCODE_CHAR_MAPPING2(I, 'i', 'I')
			KEYCODE_CHAR_MAPPING2(J, 'j', 'J')
			KEYCODE_CHAR_MAPPING2(K, 'k', 'K')
			KEYCODE_CHAR_MAPPING2(L, 'l', 'L')
			KEYCODE_CHAR_MAPPING2(M, 'm', 'M')
			KEYCODE_CHAR_MAPPING2(N, 'n', 'N')
			KEYCODE_CHAR_MAPPING2(O, 'o', 'O')
			KEYCODE_CHAR_MAPPING2(P, 'p', 'P')
			KEYCODE_CHAR_MAPPING2(Q, 'q', 'Q')
			KEYCODE_CHAR_MAPPING2(R, 'r', 'R')
			KEYCODE_CHAR_MAPPING2(S, 's', 'S')
			KEYCODE_CHAR_MAPPING2(T, 't', 'T')
			KEYCODE_CHAR_MAPPING2(U, 'u', 'U')
			KEYCODE_CHAR_MAPPING2(V, 'v', 'V')
			KEYCODE_CHAR_MAPPING2(W, 'w', 'W')
			KEYCODE_CHAR_MAPPING2(X, 'x', 'X')
			KEYCODE_CHAR_MAPPING2(Y, 'y', 'Y')
			KEYCODE_CHAR_MAPPING2(Z, 'z', 'Z')
			KEYCODE_CHAR_MAPPING(Numpad0, '0')
			KEYCODE_CHAR_MAPPING(Numpad1, '1')
			KEYCODE_CHAR_MAPPING(Numpad2, '2')
			KEYCODE_CHAR_MAPPING(Numpad3, '3')
			KEYCODE_CHAR_MAPPING(Numpad4, '4')
			KEYCODE_CHAR_MAPPING(Numpad5, '5')
			KEYCODE_CHAR_MAPPING(Numpad6, '6')
			KEYCODE_CHAR_MAPPING(Numpad7, '7')
			KEYCODE_CHAR_MAPPING(Numpad8, '8')
			KEYCODE_CHAR_MAPPING(Numpad9, '9')
			KEYCODE_CHAR_MAPPING(NumpadDivide, '/')
			KEYCODE_CHAR_MAPPING(NumpadMultiply, '*')
			KEYCODE_CHAR_MAPPING(NumpadMinus, '-')
			KEYCODE_CHAR_MAPPING(NumpadPlus, '+')
			KEYCODE_CHAR_MAPPING(NumpadEnter, '\n')
			KEYCODE_CHAR_MAPPING(NumpadDecimal, '.')
			default:
				break;
		}
		return 0;
	}

#define KEYCODE_WIN32_MAPPING(keycode, win) case win: return Keycode::keycode;
	Keycode UIEvent::getKeycodeFromWin32Keycode(sl_uint32 code)
	{
		switch (code) {
			KEYCODE_WIN32_MAPPING(Tab, 0x09) // VK_TAB
			KEYCODE_WIN32_MAPPING(Enter, 0x0D); // VK_RETURN
			KEYCODE_WIN32_MAPPING(Escape, 0x1B); // VK_ESCAPE
			KEYCODE_WIN32_MAPPING(Space, 0x20); // VK_SPACE
			KEYCODE_WIN32_MAPPING(Grave, 0xC0); // VK_OEM_3
			KEYCODE_WIN32_MAPPING(Equal, 0xBB); // VK_OEM_PLUS
			KEYCODE_WIN32_MAPPING(Semicolon, 0xBA); // VK_OEM_1
			KEYCODE_WIN32_MAPPING(Backslash, 0xDC); // VK_OEM_5
			KEYCODE_WIN32_MAPPING(LeftBaracket, 0xDB); // VK_OEM_4
			KEYCODE_WIN32_MAPPING(RightBaracket, 0xDD); // VK_OEM_6
			KEYCODE_WIN32_MAPPING(Quote, 0xDE); // VK_OEM_7
			KEYCODE_WIN32_MAPPING(Comma, 0xBC); // VK_OEM_COMMA
			KEYCODE_WIN32_MAPPING(Minus, 0xBD); // VK_OEM_MINUS
			KEYCODE_WIN32_MAPPING(Period, 0xBE); // VK_OEM_PERIOD
			KEYCODE_WIN32_MAPPING(Divide, 0xBF); // VK_OEM_2
			KEYCODE_WIN32_MAPPING(Num0, '0');
			KEYCODE_WIN32_MAPPING(Num1, '1');
			KEYCODE_WIN32_MAPPING(Num2, '2');
			KEYCODE_WIN32_MAPPING(Num3, '3');
			KEYCODE_WIN32_MAPPING(Num4, '4');
			KEYCODE_WIN32_MAPPING(Num5, '5');
			KEYCODE_WIN32_MAPPING(Num6, '6');
			KEYCODE_WIN32_MAPPING(Num7, '7');
			KEYCODE_WIN32_MAPPING(Num8, '8');
			KEYCODE_WIN32_MAPPING(Num9, '9');
			KEYCODE_WIN32_MAPPING(A, 'A');
			KEYCODE_WIN32_MAPPING(B, 'B');
			KEYCODE_WIN32_MAPPING(C, 'C');
			KEYCODE_WIN32_MAPPING(D, 'D');
			KEYCODE_WIN32_MAPPING(E, 'E');
			KEYCODE_WIN32_MAPPING(F, 'F');
			KEYCODE_WIN32_MAPPING(G, 'G');
			KEYCODE_WIN32_MAPPING(H, 'H');
			KEYCODE_WIN32_MAPPING(I, 'I');
			KEYCODE_WIN32_MAPPING(J, 'J');
			KEYCODE_WIN32_MAPPING(K, 'K');
			KEYCODE_WIN32_MAPPING(L, 'L');
			KEYCODE_WIN32_MAPPING(M, 'M');
			KEYCODE_WIN32_MAPPING(N, 'N');
			KEYCODE_WIN32_MAPPING(O, 'O');
			KEYCODE_WIN32_MAPPING(P, 'P');
			KEYCODE_WIN32_MAPPING(Q, 'Q');
			KEYCODE_WIN32_MAPPING(R, 'R');
			KEYCODE_WIN32_MAPPING(S, 'S');
			KEYCODE_WIN32_MAPPING(T, 'T');
			KEYCODE_WIN32_MAPPING(U, 'U');
			KEYCODE_WIN32_MAPPING(V, 'V');
			KEYCODE_WIN32_MAPPING(W, 'W');
			KEYCODE_WIN32_MAPPING(X, 'X');
			KEYCODE_WIN32_MAPPING(Y, 'Y');
			KEYCODE_WIN32_MAPPING(Z, 'Z');
			KEYCODE_WIN32_MAPPING(Numpad0, 0x60); // VK_NUMPAD0
			KEYCODE_WIN32_MAPPING(Numpad1, 0x61);
			KEYCODE_WIN32_MAPPING(Numpad2, 0x62);
			KEYCODE_WIN32_MAPPING(Numpad3, 0x63);
			KEYCODE_WIN32_MAPPING(Numpad4, 0x64);
			KEYCODE_WIN32_MAPPING(Numpad5, 0x65);
			KEYCODE_WIN32_MAPPING(Numpad6, 0x66);
			KEYCODE_WIN32_MAPPING(Numpad7, 0x67);
			KEYCODE_WIN32_MAPPING(Numpad8, 0x68);
			KEYCODE_WIN32_MAPPING(Numpad9, 0x69);
			KEYCODE_WIN32_MAPPING(NumpadDivide, 0x6F); // VK_DIVIDE
			KEYCODE_WIN32_MAPPING(NumpadMultiply, 0x6A); // VK_MULTIPLY
			KEYCODE_WIN32_MAPPING(NumpadMinus, 0x6D); // VK_SUBTRACT
			KEYCODE_WIN32_MAPPING(NumpadPlus, 0x6B); // VK_ADD
			KEYCODE_WIN32_MAPPING(NumpadDecimal, 0x6E); // VK_DECIMAL
			KEYCODE_WIN32_MAPPING(F1, 0x70); // VK_F1
			KEYCODE_WIN32_MAPPING(F2, 0x71);
			KEYCODE_WIN32_MAPPING(F3, 0x72);
			KEYCODE_WIN32_MAPPING(F4, 0x73);
			KEYCODE_WIN32_MAPPING(F5, 0x74);
			KEYCODE_WIN32_MAPPING(F6, 0x75);
			KEYCODE_WIN32_MAPPING(F7, 0x76);
			KEYCODE_WIN32_MAPPING(F8, 0x77);
			KEYCODE_WIN32_MAPPING(F9, 0x78);
			KEYCODE_WIN32_MAPPING(F10, 0x79);
			KEYCODE_WIN32_MAPPING(F11, 0x7A);
			KEYCODE_WIN32_MAPPING(F12, 0x7B);
			KEYCODE_WIN32_MAPPING(Backspace, 0x08); // VK_BACK
			KEYCODE_WIN32_MAPPING(PageUp, 0x21); // VK_PRIOR
			KEYCODE_WIN32_MAPPING(PageDown, 0x22); // VK_NEXT
			KEYCODE_WIN32_MAPPING(Home, 0x24); // VK_HOME
			KEYCODE_WIN32_MAPPING(End, 0x23); // VK_END
			KEYCODE_WIN32_MAPPING(Left, 0x25); // VK_LEFT
			KEYCODE_WIN32_MAPPING(Up, 0x26); // VK_UP
			KEYCODE_WIN32_MAPPING(Right, 0x27); // VK_RIGHT
			KEYCODE_WIN32_MAPPING(Down, 0x28); // VK_DOWN
			KEYCODE_WIN32_MAPPING(PrintScreen, 0x2C); // VK_SNAPSHOT
			KEYCODE_WIN32_MAPPING(Insert, 0x2D); // VK_INSERT
			KEYCODE_WIN32_MAPPING(Delete, 0x2E); // VK_DELETE
			KEYCODE_WIN32_MAPPING(Sleep, 0x5F); // VK_SLEEP
			KEYCODE_WIN32_MAPPING(Pause, 0x13); // VK_PAUSE
			KEYCODE_WIN32_MAPPING(VolumeMute, 0xAD); // VK_VOLUME_MUTE
			KEYCODE_WIN32_MAPPING(VolumeDown, 0xAE); // VK_VOLUME_DOWN
			KEYCODE_WIN32_MAPPING(VolumeUp, 0xAF); // VK_VOLUME_UP
			KEYCODE_WIN32_MAPPING(MediaPrev, 0xB1); // VK_MEDIA_PREV_TRACK
			KEYCODE_WIN32_MAPPING(MediaNext, 0xB0); // VK_MEDIA_NEXT_TRACK
			KEYCODE_WIN32_MAPPING(MediaPause, 0xB3); // VK_MEDIA_PLAY_PAUSE
			KEYCODE_WIN32_MAPPING(MediaStop, 0xB2); // VK_MEDIA_STOP
			KEYCODE_WIN32_MAPPING(LeftShift, 0xA0); // VK_LSHIFT
			KEYCODE_WIN32_MAPPING(RightShift, 0xA1); // VK_RSHIFT
			KEYCODE_WIN32_MAPPING(LeftControl, 0xA2); // VK_LCONTROL
			KEYCODE_WIN32_MAPPING(RightControl, 0xA3); // VK_RCONTROL
			KEYCODE_WIN32_MAPPING(LeftAlt, 0xA4); // VK_LMENU
			KEYCODE_WIN32_MAPPING(RightAlt, 0xA5); // VK_RMENU
			KEYCODE_WIN32_MAPPING(LeftWin, 0x5B); // VK_LWIN
			KEYCODE_WIN32_MAPPING(RightWin, 0x5C); // VK_RWIN
			KEYCODE_WIN32_MAPPING(CapsLock, 0x14); // VK_CAPITAL
			KEYCODE_WIN32_MAPPING(ScrollLock, 0x91); // VK_SCROLL
			KEYCODE_WIN32_MAPPING(NumLock, 0x90); // VK_NUMLOCK
			KEYCODE_WIN32_MAPPING(ContextMenu, 0x5D); // VK_APPS
			KEYCODE_WIN32_MAPPING(Chinese, 0x19); // VK_HANJA
			KEYCODE_WIN32_MAPPING(Korean, 0x15); // VK_HANGUL
		default:
			return Keycode::Unknown;
		}
	}

	namespace {
		static DragContext g_currentDragContext;
	}

	DragContext& UIEvent::getCurrentDragContext()
	{
		return g_currentDragContext;
	}

#if !defined(SLIB_UI_IS_WIN32) && !defined(SLIB_UI_IS_MACOS)
	sl_bool UI::isKeyPressed(Keycode key)
	{
		return sl_false;
	}

	sl_bool UI::isScrollLockOn()
	{
		return sl_false;
	}

	sl_bool UI::isNumLockOn()
	{
		return sl_false;
	}

	sl_bool UI::isLeftButtonPressed()
	{
		return sl_false;
	}

	sl_bool UI::isRightButtonPressed()
	{
		return sl_false;
	}

	sl_bool UI::isMiddleButtonPressed()
	{
		return sl_false;
	}
#endif

#if !defined(SLIB_UI_IS_WIN32) && !defined(SLIB_UI_IS_MACOS) && !defined(SLIB_UI_IS_GTK)
	sl_bool UI::isCapsLockOn()
	{
		return sl_false;
	}

	UIPoint UI::getCursorPos()
	{
		return UIPoint(0, 0);
	}
#endif

#if !defined(SLIB_UI_IS_WIN32)
	void UI::sendKeyEvent(UIAction action, Keycode key)
	{
	}

	void UI::sendMouseEvent(UIAction action, sl_ui_pos x, sl_ui_pos y, sl_bool flagAbsolutePos)
	{
	}
#endif

}
