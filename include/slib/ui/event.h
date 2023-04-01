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

#ifndef CHECKHEADER_SLIB_UI_EVENT
#define CHECKHEADER_SLIB_UI_EVENT

#include "constants.h"

#include "../core/time.h"
#include "../math/matrix3.h"

namespace slib
{

	class Cursor;
	class View;
	class DragItem;
	class DragContext;

	class SLIB_EXPORT TouchPoint
	{
	public:
		UIPointF point;
		sl_real pressure;
		TouchPhase phase;
		sl_uint64 pointerId;

	public:
		TouchPoint();

		TouchPoint(const UIPointF& point);

		TouchPoint(const UIPointF& point, sl_real pressure);

		TouchPoint(const UIPointF& point, sl_real pressure, TouchPhase phase);

		TouchPoint(const UIPointF& point, sl_real pressure, TouchPhase phase, sl_uint64 pointerId);

		TouchPoint(sl_ui_posf x, sl_ui_posf y);

		TouchPoint(sl_ui_posf x, sl_ui_posf y, sl_real pressure);

		TouchPoint(sl_ui_posf x, sl_ui_posf y, sl_real pressure, TouchPhase phase);

		TouchPoint(sl_ui_posf x, sl_ui_posf y, sl_real pressure, TouchPhase phase, sl_uint64 pointerId);

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(TouchPoint)

	};

	class SLIB_EXPORT KeycodeAndModifiers
	{
	public:
		SLIB_DEFINE_PRIMITIVE_WRAPPER_MEMBERS(KeycodeAndModifiers, int, value)

	public:
		SLIB_CONSTEXPR KeycodeAndModifiers() : value(0) {}

		SLIB_CONSTEXPR KeycodeAndModifiers(Keycode keycode) : value((int)keycode) {}

		SLIB_CONSTEXPR KeycodeAndModifiers(Keycode keycode, const Modifiers& modifiers) : value((int)keycode | modifiers) {}

	public:
		static KeycodeAndModifiers none();

		Keycode getKeycode() const;

		void setKeycode(Keycode keycode);

		Modifiers getModifiers() const;

		void setModifiers(const Modifiers& modifiers);

		void setShiftKey();

		void clearShiftKey();

		sl_bool isShiftKey() const;

		void setAltKey();

		void clearAltKey();

		sl_bool isAltKey() const;

		void setOptionKey();

		void clearOptionKey();

		sl_bool isOptionKey() const;

		void setControlKey();

		void clearControlKey();

		sl_bool isControlKey() const;

		void setWindowsKey();

		void clearWindowsKey();

		sl_bool isWindowsKey() const;

		void setCommandKey();

		void clearCommandKey();

		sl_bool isCommandKey() const;

		String toString() const;

		sl_bool parse(const StringParam& str);

	public:
		KeycodeAndModifiers& operator|=(int modifiers);

		KeycodeAndModifiers operator|(int modifiers);

		friend KeycodeAndModifiers operator|(int modifiers, const KeycodeAndModifiers& km);

	};

	KeycodeAndModifiers operator|(Keycode keycode, int modifiers);

	KeycodeAndModifiers operator|(int modifiers, Keycode keycode);


	class SLIB_EXPORT UIEvent : public CRef
	{
		SLIB_DECLARE_OBJECT

	protected:
		UIEvent();

		UIEvent(UIAction action, const Time& time);

		UIEvent(UIAction action, const UIEventFlags& flags, const Time& time);

		~UIEvent();

		SLIB_DELETE_CLASS_DEFAULT_MEMBERS(UIEvent)

	public:
		static Ref<UIEvent> createUnknown(const UIEventFlags& flags, const Time& time);

		static Ref<UIEvent> createUnknown(const Time& time);

		static Ref<UIEvent> createKeyEvent(UIAction action, Keycode keycode, sl_uint32 systemKeycode, const Time& time);

		static Ref<UIEvent> createMouseEvent(UIAction action, sl_ui_posf x, sl_ui_posf y, const Time& time);

		static Ref<UIEvent> createMouseWheelEvent(sl_ui_posf mouseX, sl_ui_posf mouseY, sl_real deltaX, sl_real deltaY, const Time& time);

		static Ref<UIEvent> createTouchEvent(UIAction action, const Array<TouchPoint>& points, const Time& time);

		static Ref<UIEvent> createTouchEvent(UIAction action, const TouchPoint& point, const Time& time);

		static Ref<UIEvent> createSetCursorEvent(sl_ui_posf x, sl_ui_posf y, const Time& time);

		static Ref<UIEvent> createDragEvent(UIAction action, sl_ui_posf x, sl_ui_posf y, const DragContext& context, const Time& time);

	public:
		UIAction getAction() const;

		void setAction(UIAction action);

		sl_bool isKeyEvent() const;

		sl_bool isMouseEvent() const;

		sl_bool isTouchEvent() const;

		sl_bool isDragEvent() const;

		// Relative to a absolute time or to the system startup
		Time getTime() const;

		// Relative to a absolute time or to the system startup
		void setTime(const Time& time);

		// key
		const KeycodeAndModifiers& getKeycodeAndModifiers() const;

		void setKeycodeAndModifiers(const KeycodeAndModifiers& km);

		Keycode getKeycode() const;

		void setKeycode(Keycode keycode);

		Modifiers getModifiers() const;

		void setModifiers(const Modifiers& modifiers);

		sl_uint32 getSystemKeycode() const;

		void setSystemKeycode(sl_uint32 keycode);

		// mouse, touch
		const UIPointF& getPoint() const;

		void setPoint(const UIPointF& pt);

		void setPoint(sl_ui_posf x, sl_ui_posf y);

		sl_ui_posf getX() const;

		void setX(sl_ui_posf x);

		sl_ui_posf getY() const;

		void setY(sl_ui_posf y);

		// mouse wheel
		sl_real getDelta() const;

		sl_real getDeltaX() const;

		void setDeltaX(sl_real x);

		sl_real getDeltaY() const;

		void setDeltaY(sl_real y);

		// touch
		const TouchPoint& getTouchPoint() const;

		void setTouchPoint(const TouchPoint& pt);

		void setTouchPoint(const UIPointF& pt);

		void setTouchPoint(const UIPointF& pt, sl_real pressure);

		void setTouchPoint(sl_ui_posf x, sl_ui_posf y);

		void setTouchPoint(sl_ui_posf x, sl_ui_posf y, sl_real pressure);

		sl_real getPressure() const;

		void setPressure(sl_real pressure);

		const Array<TouchPoint>& getTouchPoints() const;

		sl_uint32 getTouchPointCount() const;

		const TouchPoint& getTouchPoint(sl_uint32 index) const;

		void setTouchPoints(const Array<TouchPoint>& points);

		void transformPoints(const Matrix3T<float>& mat);

		void transformPoints(const Matrix3T<double>& mat);

		// set cursor
		const Ref<Cursor>& getCursor() const;

		void setCursor(const Ref<Cursor>& cursor);

		const String& getToolTip() const;

		View* getToolTipView() const;

		void setToolTip(View* view, const String& toolTip);

		// drag & drop
		const DragItem& getDragItem() const;

		void setDragItem(const DragItem& item);

		DragOperations getDragOperationMask() const;

		void setDragOperationMask(const DragOperations& mask);

		DragOperations getDragOperation() const;

		void setDragOperation(const DragOperations& op);

		// modifiers
		void setShiftKey();

		void clearShiftKey();

		sl_bool isShiftKey() const;

		void setAltKey();

		void clearAltKey();

		sl_bool isAltKey() const;

		void setOptionKey();

		void clearOptionKey();

		sl_bool isOptionKey() const;

		void setControlKey();

		void clearControlKey();

		sl_bool isControlKey() const;

		void setWindowsKey();

		void clearWindowsKey();

		sl_bool isWindowsKey() const;

		void setCommandKey();

		void clearCommandKey();

		sl_bool isCommandKey() const;

		// flags
		UIEventFlags getFlags() const;

		void resetFlags();

		void addFlag(UIEventFlags flags);

		void removeFlag(UIEventFlags flags);

		void preventDefault();

		sl_bool isPreventedDefault() const;

		void setPreventedDefault(sl_bool flag);

		void stopPropagation();

		sl_bool isStoppedPropagation() const;

		void setStoppedPropagation(sl_bool flag);

		void passToNext();

		sl_bool isPassedToNext();

		void setPassedToNext(sl_bool flag);

		void useDrag();

		sl_bool isUsingDrag();

		void setUsingDrag(sl_bool flag);

		virtual Ref<UIEvent> duplicate() const;

		static sl_uint32 getSystemKeycode(Keycode key);

		static Keycode getKeycodeFromSystemKeycode(sl_uint32 systemKeycode);

		static Keycode getKeycodeFromWin32Keycode(sl_uint32 win32Keycode);

		static String getKeyName(Keycode key, sl_bool flagShort = sl_false);

		static Keycode getKeycodeFromName(const String& keyName);

		static sl_char8 getCharFromKeycode(Keycode key, sl_bool flagUpper);

		static DragContext& getCurrentDragContext();

	protected:
		UIAction m_action;
		Time m_time;
		UIEventFlags m_flags;
		KeycodeAndModifiers m_keycodeAndModifiers;

	protected:
		void _copyProperties(const UIEvent* other);

	};

}

#endif
