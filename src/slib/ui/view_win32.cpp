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

#if defined(SLIB_UI_IS_WIN32)

#include "view_win32.h"

#include "slib/ui/window.h"
#include "slib/ui/core.h"
#include "slib/ui/drag.h"
#include "slib/math/transform2d.h"
#include "slib/core/safe_static.h"
#include "slib/core/scoped_buffer.h"
#include "slib/platform/win32/com.h"
#include "slib/dl/win32/user32.h"

#include <commctrl.h>
#include <shellapi.h>

#pragma comment(lib, "imm32.lib")

#ifdef max
#undef max
#endif

namespace slib
{

	namespace
	{
		SLIB_GLOBAL_ZERO_INITIALIZED(Ref<Bitmap>, g_bitmapDoubleBuffer)
		SLIB_GLOBAL_ZERO_INITIALIZED(Ref<Canvas>, g_canvasDoubleBuffer)

		static Color GetDefaultBackColor()
		{
			return GraphicsPlatform::getColorFromColorRef(GetSysColor(COLOR_MENU));
		}

		static LRESULT CALLBACK DefaultViewInstanceProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
		{
			switch (uMsg) {
				case WM_COMMAND:
					{
						HWND hWndSender = (HWND)lParam;
						if (hWndSender) {
							Ref<PlatformViewInstance> instance = Ref<PlatformViewInstance>::cast(UIPlatform::getViewInstance(hWndSender));
							if (instance.isNotNull()) {
								SHORT code = HIWORD(wParam);
								LRESULT result = 0;
								if (instance->processCommand(code, result)) {
									return result;
								}
							}
						}
					}
					break;
				case WM_NOTIFY:
					{
						NMHDR* nh = (NMHDR*)(lParam);
						HWND hWndSender = (HWND)(nh->hwndFrom);
						Ref<PlatformViewInstance> instance = Ref<PlatformViewInstance>::cast(UIPlatform::getViewInstance(hWndSender));
						if (instance.isNotNull()) {
							LRESULT result = 0;
							if (instance->processNotify(nh, result)) {
								return result;
							}
						}
					}
					break;
				case WM_CTLCOLOREDIT:
				case WM_CTLCOLORSTATIC:
				case WM_CTLCOLORLISTBOX:
				case WM_CTLCOLORBTN:
				case WM_CTLCOLORSCROLLBAR:
					{
						HWND hWndSender = (HWND)lParam;
						Ref<PlatformViewInstance> instance = Ref<PlatformViewInstance>::cast(UIPlatform::getViewInstance(hWndSender));
						if (instance.isNotNull()) {
							HDC hDC = (HDC)(wParam);
							HBRUSH result = NULL;
							if (!(instance->processControlColor(uMsg, hDC, result))) {
								result = (HBRUSH)(DefWindowProcW(hWnd, uMsg, wParam, lParam));
							}
							instance->processPostControlColor(uMsg, hDC, result);
							return (LRESULT)result;
						}
					}
					break;
			}
			return DefWindowProcW(hWnd, uMsg, wParam, lParam);
		}

		static sl_bool DoCaptureChildInstanceEvents(View* view, UINT uMsg)
		{
			Ref<View> parent = view->getParent();
			if (parent.isNotNull()) {
				if (DoCaptureChildInstanceEvents(parent.get(), uMsg)) {
					return sl_true;
				}
				Ref<ViewInstance> _instance = parent->getViewInstance();
				if (_instance.isNotNull()) {
					PlatformViewInstance* instance = (PlatformViewInstance*)(_instance.get());
					HWND hWnd = instance->getHandle();
					if (hWnd) {
						DWORD lParam = GetMessagePos();
						POINT pt;
						pt.x = (short)(lParam & 0xffff);
						pt.y = (short)((lParam >> 16) & 0xffff);
						ScreenToClient(hWnd, &pt);
						if (parent->isCapturingChildInstanceEvents((sl_ui_pos)(pt.x), (sl_ui_pos)(pt.y))) {
							LPARAM lParam = POINTTOPOINTS(pt);
							instance->processWindowMessage(hWnd, uMsg, 0, lParam);
							return sl_true;
						}
					}
				}
			}
			return sl_false;
		}
	}

	namespace priv
	{
		LRESULT CALLBACK ViewInstanceProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
		{
			Ref<PlatformViewInstance> instance = Ref<PlatformViewInstance>::cast(UIPlatform::getViewInstance(hWnd));
			if (instance.isNotNull()) {
				return instance->processWindowMessage(hWnd, uMsg, wParam, lParam);
			}
			return DefaultViewInstanceProc(hWnd, uMsg, wParam, lParam);
		}

		sl_bool CaptureChildInstanceEvents(View* view, MSG& msg)
		{
			UINT uMsg = msg.message;
			switch (uMsg) {
				case WM_LBUTTONDOWN:
				case WM_LBUTTONDBLCLK:
				case WM_RBUTTONDOWN:
				case WM_RBUTTONDBLCLK:
				case WM_MBUTTONDOWN:
				case WM_MBUTTONDBLCLK:
				case WM_MOUSEMOVE:
				case 0x0240: // WM_TOUCH
					break;
				case WM_NCLBUTTONDOWN:
					uMsg = WM_LBUTTONDOWN;
					break;
				case WM_NCRBUTTONDOWN:
					uMsg = WM_RBUTTONDOWN;
					break;
				case WM_NCMBUTTONDOWN:
					uMsg = WM_MBUTTONDOWN;
					break;
				case WM_NCMOUSEMOVE:
					uMsg = WM_MOUSEMOVE;
					break;
				default:
					return sl_false;
			}
			return DoCaptureChildInstanceEvents(view, uMsg);
		}
	}

	using namespace priv;

	SLIB_DEFINE_OBJECT(PlatformViewInstance, ViewInstance)

	PlatformViewInstance::PlatformViewInstance()
	{
		m_handle = NULL;

		m_flagGenericView = sl_false;
		m_flagDestroyOnRelease = sl_false;
		m_flagRegisteredTouch = sl_false;

		m_dropTarget = NULL;
		m_imc = NULL;
	}

	PlatformViewInstance::~PlatformViewInstance()
	{
		if (m_handle) {
			if (m_imc) {
				ImmAssociateContext(m_handle, m_imc);
			}
			UIPlatform::removeViewInstance(m_handle);
			if (m_flagDestroyOnRelease) {
				_destroy(m_handle);
			}
		}
		if (m_dropTarget) {
			m_dropTarget->Release();
		}
	}

	void PlatformViewInstance::_destroy(HWND hWnd)
	{
		Win32_UI_Shared* shared = Win32_UI_Shared::get();
		if (shared) {
			PostMessageW(shared->hWndMessage, SLIB_UI_MESSAGE_CLOSE_VIEW, 0, (LPARAM)hWnd);
		} else {
			DestroyWindow(hWnd);
		}
	}

	namespace
	{
		static LRESULT CALLBACK ViewInstanceSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
		{
			Ref<PlatformViewInstance> instance = Ref<PlatformViewInstance>::cast(UIPlatform::getViewInstance(hWnd));
			if (instance.isNotNull()) {
				return instance->processSubclassMessage(hWnd, uMsg, wParam, lParam);
			}
			return DefSubclassProc(hWnd, uMsg, wParam, lParam);
		}
	}

	HWND PlatformViewInstance::createHandle(
		View* view, ViewInstance* parent,
		LPCWSTR wndClass, LPCWSTR text,
		const UIRect& frame, const Matrix3& transform,
		int style, int styleEx, int styleRemove)
	{

		HWND hWndParent = UIPlatform::getViewHandle(parent);

		if (hWndParent) {

			HINSTANCE hInst = GetModuleHandleW(NULL);
			style |= WS_CHILD;
			if (view->isVisibleInInstance()) {
				style |= WS_VISIBLE;
			}
			if (view->hasBorder()) {
				if (view->isClientEdge()) {
					styleEx |= WS_EX_CLIENTEDGE;
				} else {
					style |= WS_BORDER;
				}
			}
			if (view->isCreatingNativeLayer() || UIPlatform::getWindowAlpha(view->getAlpha()) != 255 || view->getColorKey().isNotZero()) {
				styleEx |= WS_EX_LAYERED;
			}
			if (view->isCreatingNativeWidget()) {
				if (view->isHorizontalScrollBarVisible()) {
					style |= WS_HSCROLL;
				}
				if (view->isVerticalScrollBarVisible()) {
					style |= WS_VSCROLL;
				}
			}
			style &= (~styleRemove);

			sl_ui_pos x = frame.left;
			sl_ui_pos y = frame.top;
			Vector2 t = Transform2::getTranslationFromMatrix(transform);
			x += (sl_ui_pos)(t.x);
			y += (sl_ui_pos)(t.y);

			HWND hWnd = CreateWindowExW(
				styleEx, // ex-style
				wndClass,
				text,
				style,
				(int)(x), (int)(y),
				(int)(frame.getWidth()), (int)(frame.getHeight()),
				hWndParent, // parent
				NULL, // menu
				hInst,
				NULL);
			if (hWnd) {
				if (!(view->isEnabled())) {
					EnableWindow(hWnd, FALSE);
				}
				Win32_UI_Shared* shared = Win32_UI_Shared::get();
				if (shared) {
					if (wndClass != (LPCWSTR)(shared->wndClassForView)) {
						SetWindowSubclass(hWnd, ViewInstanceSubclassProc, 0, 0);
					}
				}
				if (styleEx & WS_EX_LAYERED) {
					if (!(view->isCreatingNativeLayer())) {
						UIPlatform::initLayeredWindowAttributes(hWnd, UIPlatform::getWindowAlpha(view->getAlpha()), view->getColorKey());
					}
				}
				return hWnd;
			}
		}
		return NULL;
	}

	void PlatformViewInstance::initWithHandle(HWND hWnd, sl_bool flagDestroyOnRelease)
	{
		m_handle = hWnd;
		m_flagDestroyOnRelease = flagDestroyOnRelease;
		UIPlatform::registerViewInstance(hWnd, this);
	}

	void PlatformViewInstance::initWithHandle(HWND hWnd, View* view, const String16& text, const UIRect& frame, const Matrix3& transform)
	{
		initWithHandle(hWnd, sl_true);
		m_text = text;
		m_frame = frame;
		m_translation = Transform2::getTranslationFromMatrix(transform);
		if (view->isUsingFont()) {
			setFont(view, view->getFont());
		}
	}

	HWND PlatformViewInstance::getHandle()
	{
		return m_handle;
	}

	sl_bool PlatformViewInstance::isValid(View* view)
	{
		HWND hWnd = m_handle;
		if (hWnd) {
			if (IsWindow(hWnd)) {
				return sl_true;
			}
		}
		return sl_false;
	}

	void PlatformViewInstance::setFocus(View* view, sl_bool flag)
	{
		HWND hWnd = m_handle;
		if (hWnd) {
			if (flag) {
				updateIME(view);
				m_flagSettingFocus = sl_true;
				SetFocus(hWnd);
				m_flagSettingFocus = sl_false;
			} else {
				if (GetFocus() == hWnd) {
					SetFocus(NULL);
				}
			}
		}
	}

	namespace
	{
		static sl_bool g_flagDuringPaint = sl_false;
	}

	void PlatformViewInstance::invalidate(View* view)
	{
		if (!(UI::isUiThread()) || g_flagDuringPaint) {
			void (PlatformViewInstance::*func)(View*) = &ViewInstance::invalidate;
			UI::dispatchToUiThreadUrgently(Function<void()>::bindWeakRef(this, func, sl_null));
			return;
		}
		if (m_nativeLayer.isNotNull()) {
			m_nativeLayer->flagInvalidated = sl_true;
			UI::dispatchToUiThreadUrgently(SLIB_FUNCTION_WEAKREF(this, updateNativeLayer));
			return;
		}
		HWND hWnd = m_handle;
		if (hWnd) {
			InvalidateRect(hWnd, NULL, TRUE);
		}
	}

	void PlatformViewInstance::invalidate(View* view, const UIRect& rect)
	{
		if (!(UI::isUiThread()) || g_flagDuringPaint) {
			void (ViewInstance::*func)(View*, const UIRect&) = &ViewInstance::invalidate;
			UI::dispatchToUiThreadUrgently(Function<void()>::bindWeakRef(this, func, sl_null, rect));
			return;
		}
		if (m_nativeLayer.isNotNull()) {
			m_nativeLayer->flagInvalidated = sl_true;
			UI::dispatchToUiThreadUrgently(SLIB_FUNCTION_WEAKREF(this, updateNativeLayer));
			return;
		}
		HWND hWnd = m_handle;
		if (hWnd) {
			RECT rc;
			rc.left = (int)(rect.left);
			rc.top = (int)(rect.top);
			rc.right = (int)(rect.right);
			rc.bottom = (int)(rect.bottom);
			InvalidateRect(hWnd, &rc, TRUE);
		}
	}

	void PlatformViewInstance::setFrame(View* view, const UIRect& frame)
	{
		if (isWindowContent()) {
			return;
		}
		m_frame = frame;
		HWND hWnd = m_handle;
		if (hWnd) {
			UINT uFlags = SWP_NOREPOSITION | SWP_NOZORDER | SWP_NOACTIVATE
				| SWP_NOCOPYBITS
				| SWP_ASYNCWINDOWPOS;
			SetWindowPos(hWnd, NULL
				, (int)(frame.left + m_translation.x), (int)(frame.top + m_translation.y)
				, (int)(frame.getWidth()), (int)(frame.getHeight())
				, uFlags
			);
		}
		if (m_nativeLayer.isNotNull()) {
			updateNativeLayer();
		}
	}

	void PlatformViewInstance::setTransform(View* view, const Matrix3 &transform)
	{
		if (isWindowContent()) {
			return;
		}
		m_translation = Transform2::getTranslationFromMatrix(transform);
		HWND hWnd = m_handle;
		if (hWnd) {
			UINT uFlags = SWP_NOREPOSITION | SWP_NOZORDER | SWP_NOACTIVATE
				| SWP_NOCOPYBITS
				| SWP_ASYNCWINDOWPOS;
			SetWindowPos(hWnd, NULL
				, (int)(m_frame.left + m_translation.x), (int)(m_frame.top + m_translation.y)
				, (int)(m_frame.getWidth()), (int)(m_frame.getHeight())
				, uFlags
			);
		}
		if (m_nativeLayer.isNotNull()) {
			updateNativeLayer();
		}
	}

	void PlatformViewInstance::setVisible(View* view, sl_bool flag)
	{
		HWND hWnd = m_handle;
		if (hWnd) {
			sl_bool f1 = GetWindowLongW(hWnd, GWL_STYLE) & WS_VISIBLE ? sl_true : sl_false;
			sl_bool f2 = flag ? sl_true : sl_false;
			if (f1 != f2) {
				if (f2) {
					ShowWindowAsync(hWnd, SW_SHOW);
				} else {
					ShowWindowAsync(hWnd, SW_HIDE);
				}
			}
		}
	}

	void PlatformViewInstance::setEnabled(View* view, sl_bool flag)
	{
		HWND hWnd = m_handle;
		if (hWnd) {
			sl_bool f1 = IsWindowEnabled(hWnd) ? sl_true : sl_false;
			sl_bool f2 = flag ? sl_true : sl_false;
			if (f1 != f2) {
				EnableWindow(hWnd, f2);
			}
		}
	}

	void PlatformViewInstance::setOpaque(View* view, sl_bool flag)
	{
	}

	namespace
	{
		static void SetLayeredAttributes(HWND hWnd, sl_real alpha, const Color& colorKey)
		{
			sl_uint8 a = UIPlatform::getWindowAlpha(alpha);
			if (a == 255 && colorKey.isZero()) {
				UIPlatform::setWindowExStyle(hWnd, WS_EX_LAYERED, sl_false);
			} else {
				UIPlatform::setWindowExStyle(hWnd, WS_EX_LAYERED, sl_true);
			}
			UIPlatform::updateLayeredWindowAttributes(hWnd, a, colorKey);
			RedrawWindow(hWnd, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN);
		}
	}

	void PlatformViewInstance::setAlpha(View* view, sl_real alpha)
	{
		if (m_nativeLayer.isNotNull()) {
			return;
		}
		HWND hWnd = m_handle;
		if (hWnd) {
			SetLayeredAttributes(hWnd, alpha, view->getColorKey());
		}
	}

	void PlatformViewInstance::setColorKey(View* view, const Color& color)
	{
		if (m_nativeLayer.isNotNull()) {
			return;
		}
		HWND hWnd = m_handle;
		if (hWnd) {
			SetLayeredAttributes(hWnd, view->getAlpha(), color);
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
		HWND hWnd = m_handle;
		if (hWnd) {
			POINT pt;
			pt.x = (LONG)(ptScreen.x);
			pt.y = (LONG)(ptScreen.y);
			ScreenToClient(hWnd, &pt);
			return UIPointF((sl_ui_posf)(pt.x), (sl_ui_posf)(pt.y));
		}
		return ptScreen;
	}

	UIPointF PlatformViewInstance::convertCoordinateFromViewToScreen(View* view, const UIPointF& ptView)
	{
		HWND hWnd = m_handle;
		if (hWnd) {
			POINT pt;
			pt.x = (LONG)(ptView.x);
			pt.y = (LONG)(ptView.y);
			ClientToScreen(hWnd, &pt);
			return UIPointF((sl_ui_posf)(pt.x), (sl_ui_posf)(pt.y));
		}
		return ptView;
	}

	void PlatformViewInstance::addChildInstance(View* view, const Ref<ViewInstance>& _child)
	{
		HWND hWnd = m_handle;
		if (hWnd) {
			PlatformViewInstance* child = (PlatformViewInstance*)(_child.get());
			if (child) {
				HWND hWndChild = child->getHandle();
				if (hWndChild) {
					SetParent(hWndChild, hWnd);
				}
			}
		}
	}

	void PlatformViewInstance::removeChildInstance(View* view, const Ref<ViewInstance>& _child)
	{
		PlatformViewInstance* child = (PlatformViewInstance*)(_child.get());
		HWND hWnd = child->getHandle();
		if (hWnd) {
			SetParent(hWnd, HWND_MESSAGE);
		}
	}

	void PlatformViewInstance::bringToFront(View* view)
	{
		HWND hWnd = m_handle;
		if (hWnd) {
			BringWindowToTop(hWnd);
			Ref<View> view = getView();
			if (view.isNotNull()) {
				view->invalidateBoundsInParent();
			}
		}
	}

	void PlatformViewInstance::setFont(View* view, const Ref<Font>& font)
	{
		HWND handle = m_handle;
		if (handle) {
			HFONT hFont = GraphicsPlatform::getGdiFont(font.get());
			if (hFont) {
				SendMessageW(handle, WM_SETFONT, (WPARAM)hFont, TRUE);
				m_font = font;
			}
		}
	}

	void PlatformViewInstance::setBorder(View* view, sl_bool flag)
	{
		if (view->isClientEdge()) {
			UIPlatform::setWindowExStyle(m_handle, WS_EX_CLIENTEDGE, flag);
			UIPlatform::setWindowStyle(m_handle, WS_BORDER, sl_false);
		} else {
			UIPlatform::setWindowExStyle(m_handle, WS_EX_CLIENTEDGE, sl_false);
			UIPlatform::setWindowStyle(m_handle, WS_BORDER, flag);
		}
	}

	void PlatformViewInstance::setScrollBarsVisible(View* view, sl_bool flagHorizontal, sl_bool flagVertical)
	{
		HWND handle = m_handle;
		if (handle) {
			auto func = user32::getApi_ShowScrollBar();
			if (func) {
				func(handle, SB_HORZ, flagHorizontal);
				func(handle, SB_VERT, flagVertical);
			}
		}
	}

	sl_bool PlatformViewInstance::getScrollPosition(View* view, ScrollPosition& _out)
	{
		HWND handle = m_handle;
		if (handle) {
			SCROLLINFO si;
			Base::zeroMemory(&si, sizeof(si));
			si.cbSize = sizeof(si);
			si.fMask = SIF_POS;
			::GetScrollInfo(handle, SB_HORZ, &si);
			_out.x = (sl_scroll_pos)(si.nPos);
			::GetScrollInfo(handle, SB_VERT, &si);
			_out.y = (sl_scroll_pos)(si.nPos);
			return sl_true;
		}
		return sl_false;
	}

	sl_bool PlatformViewInstance::getScrollRange(View* view, ScrollPosition& _out)
	{
		HWND handle = m_handle;
		if (handle) {
			SCROLLINFO si;
			Base::zeroMemory(&si, sizeof(si));
			si.cbSize = sizeof(si);
			si.fMask = SIF_RANGE;
			::GetScrollInfo(handle, SB_HORZ, &si);
			int w = si.nMax;
			::GetScrollInfo(handle, SB_VERT, &si);
			int h = si.nMax;
			if (w < 0) {
				w = 0;
			}
			if (h < 0) {
				h = 0;
			}
			_out.x = (sl_scroll_pos)w;
			_out.y = (sl_scroll_pos)h;
			return sl_true;
		}
		return sl_false;
	}

	void PlatformViewInstance::scrollTo(View* view, sl_scroll_pos x, sl_scroll_pos y, sl_bool flagAnimate)
	{
		HWND handle = m_handle;
		if (handle) {
			SCROLLINFO si;
			Base::zeroMemory(&si, sizeof(si));
			si.cbSize = sizeof(si);
			si.fMask = SIF_POS;
			si.nPos = (int)x;
			SetScrollInfo(handle, SB_HORZ, &si, TRUE);
			si.nPos = (int)y;
			SetScrollInfo(handle, SB_VERT, &si, TRUE);
		}
	}

	void PlatformViewInstance::setScrollPos(sl_scroll_pos x, sl_scroll_pos y)
	{
		HWND handle = m_handle;
		if (handle) {
			SCROLLINFO si;
			Base::zeroMemory(&si, sizeof(si));
			si.cbSize = sizeof(si);
			si.fMask = SIF_POS;
			si.nPos = (int)x;
			SetScrollInfo(handle, SB_HORZ, &si, TRUE);
			SendMessageW(handle, WM_HSCROLL, SLIB_MAKE_DWORD2(Math::clamp(si.nPos, 0, 0xffff), SB_THUMBPOSITION), 0);
			si.nPos = (int)y;
			SetScrollInfo(handle, SB_VERT, &si, TRUE);
			SendMessageW(handle, WM_VSCROLL, SLIB_MAKE_DWORD2(Math::clamp(si.nPos, 0, 0xffff), SB_THUMBPOSITION), 0);
		}
	}

	sl_bool PlatformViewInstance::getClientSize(View* view, UISize& _out)
	{
		HWND handle = m_handle;
		if (handle) {
			RECT rc;
			GetClientRect(handle, &rc);
			_out.x = (sl_ui_len)(rc.right);
			_out.y = (sl_ui_len)(rc.bottom);
			return sl_true;
		}
		return sl_false;
	}

	namespace
	{
		static DWORD ToDropEffect(int op)
		{
			DWORD ret = 0;
			if (op & DragOperations::Copy) {
				ret |= DROPEFFECT_COPY;
			}
			if (op & DragOperations::Link) {
				ret |= DROPEFFECT_LINK;
			}
			if (op & DragOperations::Move) {
				ret |= DROPEFFECT_MOVE;
			}
			if (op & DragOperations::Scroll) {
				ret |= DROPEFFECT_SCROLL;
			}
			return ret;
		}

		static int FromDropEffect(DWORD dwEffect)
		{
			int ret = 0;
			if (dwEffect & DROPEFFECT_COPY) {
				ret |= DragOperations::Copy;
			}
			if (dwEffect & DROPEFFECT_LINK) {
				ret |= DragOperations::Link;
			}
			if (dwEffect & DROPEFFECT_MOVE) {
				ret |= DragOperations::Move;
			}
			if (dwEffect & DROPEFFECT_SCROLL) {
				ret |= DragOperations::Scroll;
			}
			return ret;
		}

		class ViewDropTarget : public IDropTarget
		{
		private:
			ULONG m_nRef;
			WeakRef<PlatformViewInstance> m_instance;

			DragContext m_context;
			POINTL m_ptLast;

		public:
			ViewDropTarget(PlatformViewInstance* instance): m_instance(instance)
			{
				m_nRef = 0;
			}

		public:
			ULONG STDMETHODCALLTYPE AddRef() override
			{
				return ++m_nRef;
			}

			ULONG STDMETHODCALLTYPE Release() override
			{
				ULONG nRef = --m_nRef;
				if (!nRef) {
					delete this;
				}
				return nRef;
			}

			HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject) override
			{
				if (!ppvObject) {
					return E_POINTER;
				}
				if (riid == __uuidof(IDropTarget) || riid == __uuidof(IUnknown)) {
					*ppvObject = this;
					AddRef();
					return S_OK;
				}
				return E_NOINTERFACE;
			}

			HRESULT STDMETHODCALLTYPE DragEnter(
				IDataObject *pDataObj,
				DWORD grfKeyState,
				POINTL pt,
				DWORD *pdwEffect
			) override
			{
				if (!pdwEffect) {
					return E_INVALIDARG;
				}
				if (setData(pDataObj)) {
					m_context.operationMask = FromDropEffect(*pdwEffect);
					m_ptLast = pt;
					*pdwEffect = onEvent(UIAction::DragEnter);
					return S_OK;
				}
				return E_UNEXPECTED;
			}

			HRESULT STDMETHODCALLTYPE DragOver(
				DWORD grfKeyState,
				POINTL pt,
				DWORD *pdwEffect
			) override
			{
				if (!pdwEffect) {
					return E_INVALIDARG;
				}
				m_ptLast = pt;
				*pdwEffect = onEvent(UIAction::DragOver);
				return S_OK;
			}

			HRESULT STDMETHODCALLTYPE DragLeave() override
			{
				onEvent(UIAction::DragLeave);
				return S_OK;
			}

			HRESULT STDMETHODCALLTYPE Drop(
				IDataObject *pDataObj,
				DWORD grfKeyState,
				POINTL pt,
				DWORD *pdwEffect
			) override
			{
				if (setData(pDataObj)) {
					m_context.operationMask = FromDropEffect(*pdwEffect);
					m_ptLast = pt;
					*pdwEffect = onEvent(UIAction::Drop);
					return S_OK;
				}
				return E_UNEXPECTED;
			}

		private:
			DWORD onEvent(UIAction action)
			{
				Ref<PlatformViewInstance> instance = m_instance;
				if (instance.isNotNull()) {
					POINT pt;
					pt.x = m_ptLast.x;
					pt.y = m_ptLast.y;
					ScreenToClient(instance->getHandle(), &pt);
					Ref<UIEvent> ev = UIEvent::createDragEvent(action, (sl_ui_posf)(pt.x), (sl_ui_posf)(pt.y), m_context, Time::now());
					if (ev.isNotNull()) {
						instance->onDragDropEvent(ev.get());
						return ToDropEffect(ev->getDragOperation());
					}
				}
				return DROPEFFECT_NONE;
			}

			sl_bool setData(IDataObject* data)
			{
				m_context.item.clear();
				if (setFiles(data)) {
					return sl_true;
				}
				if (setText(data)) {
					return sl_true;
				}
				return sl_false;
			}

			sl_bool setFiles(IDataObject* data)
			{
				FORMATETC fmt = { 0 };
				fmt.cfFormat = CF_HDROP;
				fmt.dwAspect = DVASPECT_CONTENT;
				fmt.lindex = -1;
				fmt.tymed = TYMED_HGLOBAL;

				STGMEDIUM medium = { 0 };
				medium.tymed = TYMED_HGLOBAL;

				HRESULT hr = data->GetData(&fmt, &medium);
				if (hr != S_OK) {
					return sl_false;
				}

				sl_bool flagValid = sl_false;
				if (medium.hGlobal) {
					HDROP handle = (HDROP)(medium.hGlobal);
					List<String> files;
					UINT nFiles = DragQueryFileW(handle, 0xFFFFFFFF, NULL, 0);
					for (UINT i = 0; i < nFiles; i++) {
						WCHAR sz[512];
						UINT len = DragQueryFileW(handle, i, sz, 512);
						files.add_NoLock(String::from(sz, len));
					}
					m_context.item.setFiles(Move(files));
					flagValid = sl_true;
				}

				ReleaseStgMedium(&medium);

				return flagValid;
			}

			sl_bool setText(IDataObject* data)
			{
				FORMATETC fmt = { 0 };
				fmt.cfFormat = CF_UNICODETEXT;
				fmt.dwAspect = DVASPECT_CONTENT;
				fmt.lindex = -1;
				fmt.tymed = TYMED_HGLOBAL;

				STGMEDIUM medium = { 0 };
				medium.tymed = TYMED_HGLOBAL;

				HRESULT hr = data->GetData(&fmt, &medium);
				if (hr != S_OK) {
					return sl_false;
				}

				sl_bool flagValid = sl_false;
				if (medium.hGlobal) {
					LPVOID pData = GlobalLock(medium.hGlobal);
					if (pData) {
						sl_char16* sz = (sl_char16*)pData;
						sl_size size = (sl_size)(GlobalSize(medium.hGlobal));
						sl_size len = size >> 1;
						String text = String::fromUtf16(sz, Base::getStringLength2(sz, len));
						m_context.item.setText(text);
						flagValid = sl_true;
						GlobalUnlock(medium.hGlobal);
					}
				}

				ReleaseStgMedium(&medium);

				return flagValid;
			}
		};

		class ViewDropSource : public IDropSource
		{
		private:
			ULONG m_nRef;
			DragContext* m_context;
			UIAction m_actionDown;

		public:
			ViewDropSource(DragContext* context, UIAction actionDown): m_context(context), m_actionDown(actionDown), m_nRef(0)
			{
			}

		public:
			ULONG STDMETHODCALLTYPE AddRef() override
			{
				return ++m_nRef;
			}

			ULONG STDMETHODCALLTYPE Release() override
			{
				ULONG nRef = --m_nRef;
				if (!nRef) {
					delete this;
				}
				return nRef;
			}

			HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject) override
			{
				if (!ppvObject) {
					return E_POINTER;
				}
				if (riid == __uuidof(IDropSource) || riid == __uuidof(IUnknown)) {
					*ppvObject = this;
					AddRef();
					return S_OK;
				}
				return E_NOINTERFACE;
			}

			HRESULT STDMETHODCALLTYPE QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState) override
			{
				if (fEscapePressed) {
					return DRAGDROP_S_CANCEL;
				}
				if (m_actionDown == UIAction::LeftButtonDown) {
					if (!(grfKeyState & MK_LBUTTON)) {
						return DRAGDROP_S_DROP;
					}
				} else if (m_actionDown == UIAction::RightButtonDown) {
					if (!(grfKeyState & MK_RBUTTON)) {
						return DRAGDROP_S_DROP;
					}
				} else if (m_actionDown == UIAction::MiddleButtonDown) {
					if (!(grfKeyState & MK_MBUTTON)) {
						return DRAGDROP_S_DROP;
					}
				}
				return S_OK;
			}

			HRESULT STDMETHODCALLTYPE GiveFeedback(DWORD dwEffect) override
			{
				Ref<View>& view = m_context->view;
				if (view.isNotNull()) {
					POINT pt;
					GetCursorPos(&pt);
					Ref<UIEvent> ev = UIEvent::createDragEvent(UIAction::Drag, (sl_ui_posf)(pt.x), (sl_ui_posf)(pt.y), *m_context, Time::now());
					if (ev.isNotNull()) {
						view->dispatchDragDropEvent(ev.get());
					}
				}
				return DRAGDROP_S_USEDEFAULTCURSORS;
			}
		};
	}

	void PlatformViewInstance::setDropTarget(View* view, sl_bool flag)
	{
		HWND handle = m_handle;
		if (handle) {
			if (flag) {
				if (m_dropTarget) {
					return;
				}
				ViewDropTarget* target = new ViewDropTarget(this);
				m_dropTarget = target;
				RegisterDragDrop(handle, target);
			} else {
				if (!m_dropTarget) {
					return;
				}
				RevokeDragDrop(handle);
				m_dropTarget = sl_null;
			}
		}
	}

	void PlatformViewInstance::setUsingTouchEvent(View* view, sl_bool flag)
	{
		HWND handle = m_handle;
		if (handle) {
			if (flag) {
				if (!m_flagRegisteredTouch) {
					m_flagRegisteredTouch = UIPlatform::registerTouchWindow(handle);
				}
			} else {
				if (m_flagRegisteredTouch) {
					m_flagRegisteredTouch = sl_false;
					UIPlatform::unregisterTouchWindow(handle);
				}
			}
		}
	}

	void PlatformViewInstance::setText(const StringParam& _text)
	{
		HWND handle = m_handle;
		if (handle) {
			String16 text = _text.toString16();
			UIPlatform::setWindowText(handle, text);
			m_text = Move(text);
		}
	}

	void PlatformViewInstance::initNativeLayer()
	{
		if (m_nativeLayer.isNull()) {
			m_nativeLayer = new Win32_NativeLayerContext;
		}
	}

	void PlatformViewInstance::updateNativeLayer()
	{
		Win32_NativeLayerContext* layer = m_nativeLayer.get();
		if (layer) {
			if (layer->flagInvalidated) {
				layer->flagInvalidated = sl_false;
				onDrawNativeLayer();
			}
		}
	}

	void PlatformViewInstance::releaseDragging()
	{
		DragContext& dragContext = UIEvent::getCurrentDragContext();
		dragContext.release();
	}

	sl_bool PlatformViewInstance::doDragging(UIAction action)
	{
		sl_bool bRet = sl_false;
		DragContext& dragContext = UIEvent::getCurrentDragContext();
		Ref<View>& viewDrag = dragContext.view;
		if (viewDrag.isNotNull()) {
			win32::GenericDataObject* data = new win32::GenericDataObject;
			if (data) {
				data->AddRef();
				data->setText(dragContext.item.getText());
				ViewDropSource* source = new ViewDropSource(&dragContext, action);
				if (source) {
					source->AddRef();
					{
						POINT pt;
						GetCursorPos(&pt);
						Ref<UIEvent> ev = UIEvent::createDragEvent(UIAction::DragStart, (sl_ui_posf)(pt.x), (sl_ui_posf)(pt.y), dragContext, Time::now());
						if (ev.isNotNull()) {
							viewDrag->dispatchDragDropEvent(ev.get());
						}
					}
					DWORD dwEffect = 0;
					HRESULT hr = DoDragDrop(data, source, ToDropEffect(dragContext.operationMask), &dwEffect);
					{
						viewDrag->cancelPressedState();
						viewDrag->cancelHoverState();
						dragContext.operation = FromDropEffect(dwEffect);
						POINT pt;
						GetCursorPos(&pt);
						Ref<UIEvent> ev = UIEvent::createDragEvent(UIAction::DragEnd, (sl_ui_posf)(pt.x), (sl_ui_posf)(pt.y), dragContext, Time::now());
						if (ev.isNotNull()) {
							viewDrag->dispatchDragDropEvent(ev.get());
						}
					}
					source->Release();
					bRet = sl_true;
				}
				data->Release();
			}
		}
		return bRet;
	}

	void PlatformViewInstance::updateToolTip(sl_uint64 ownerId, const String& toolTip)
	{
		if (m_tooltip.isNotNull()) {
			m_tooltip->update(this, ownerId, toolTip);
		} else {
			if (toolTip.isNotNull()) {
				m_tooltip = new Win32_ToolTipViewContext;
				if (m_tooltip.isNotNull()) {
					m_tooltip->update(this, ownerId, toolTip);
				}
			}
		}
	}

	void PlatformViewInstance::enableIME()
	{
		HWND handle = m_handle;
		if (handle) {
			if (m_imc) {
				ImmAssociateContext(handle, m_imc);
				m_imc = NULL;
			}
		}
	}

	void PlatformViewInstance::disableIME()
	{
		HWND handle = m_handle;
		if (handle) {
			if (!m_imc) {
				m_imc = ImmAssociateContext(handle, NULL);
			}
		}
	}

	void PlatformViewInstance::updateIME(View* view)
	{
		if (isNativeWidget()) {
			if (view->isUsingIME()) {
				enableIME();
				return;
			}
		} else {
			Ref<View> focus = view->getFocusedView();
			if (focus.isNotNull()) {
				if (focus->isUsingIME()) {
					enableIME();
					return;
				}
			}
		}
		disableIME();
	}

	void PlatformViewInstance::onPaint(Canvas* canvas)
	{
		Win32_NativeLayerContext* layer = m_nativeLayer.get();
		if (layer) {
			return;
		}

		Ref<View> view = m_view;
		if (view.isNull()) {
			return;
		}

		UIRect rc = canvas->getInvalidatedRect();

		sl_bool flagOpaque = sl_false;
		Color colorBack;
		if (view->isOpaque()) {
			flagOpaque = sl_true;
		} else {
			Color colorView = view->getBackgroundColor();
			if (colorView.a == 255) {
				flagOpaque = sl_true;
			} else {
				if (m_flagWindowContent) {
					Ref<Window> window = view->getWindow();
					if (window.isNotNull()) {
						colorBack = window->getBackgroundColor();
					}
				}
			}
			if (!flagOpaque) {
				if (colorBack.a < 255) {
					Color c = GetDefaultBackColor();
					c.blend_PA_NPA(colorBack);
					colorBack = c;
				}
			}
		}

		if (!(view->isDoubleBuffer())) {
			if (!flagOpaque) {
				if (colorBack.isNotZero()) {
					canvas->fillRectangle(rc, colorBack);
				} else {
					canvas->fillRectangle(rc, Color::White);
				}
			}
			view->dispatchDraw(canvas);
			return;
		}

		UISize size = rc.getSize();
		if (size.x < 1) {
			return;
		}
		if (size.y < 1) {
			return;
		}
		UISize screenSize = UI::getScreenSize();
		if (size.x > screenSize.x) {
			size.x = screenSize.x;
		}
		if (size.y > screenSize.y) {
			size.y = screenSize.y;
		}

		sl_uint32 widthBitmap = (sl_uint32)(size.x);
		sl_uint32 heightBitmap = (sl_uint32)(size.y);
		sl_uint32 widthOldBitmap = 0;
		sl_uint32 heightOldBitmap = 0;

		Ref<Bitmap>& bitmapBuffer = g_bitmapDoubleBuffer;
		Ref<Canvas>& canvasBuffer = g_canvasDoubleBuffer;
		if (bitmapBuffer.isNotNull()) {
			widthOldBitmap = bitmapBuffer->getWidth();
			heightOldBitmap = bitmapBuffer->getHeight();
		}
		if (bitmapBuffer.isNull() || canvasBuffer.isNull() || widthOldBitmap < widthBitmap || heightOldBitmap < heightBitmap) {
			Ref<Bitmap> bitmapNew = Bitmap::create(Math::max(widthOldBitmap, widthBitmap), Math::max(heightOldBitmap, heightBitmap));
			if (bitmapNew.isNull()) {
				return;
			}
			bitmapBuffer = bitmapNew;
			canvasBuffer = bitmapBuffer->getCanvas();
			if (canvasBuffer.isNull()) {
				return;
			}
			canvasBuffer->setAntiAlias(sl_false);
		}

		if (!flagOpaque) {
			if (colorBack.isNotZero()) {
				bitmapBuffer->resetPixels(0, 0, widthBitmap, heightBitmap, colorBack);
			} else {
				bitmapBuffer->resetPixels(0, 0, widthBitmap, 1, Color::White);
			}
		}
		rc.setSize(size);
		canvasBuffer->setInvalidatedRect(rc);
		CanvasStateScope scope(canvasBuffer.get());
		canvasBuffer->translate(-(sl_real)(rc.left), -(sl_real)(rc.top));
		view->dispatchDraw(canvasBuffer.get());
		canvasBuffer->translate((sl_real)(rc.left), (sl_real)(rc.top));
		canvas->draw(rc, bitmapBuffer, Rectangle(0, 0, (sl_real)(size.x), (sl_real)(size.y)));
	}

	void PlatformViewInstance::onPaint()
	{
		HWND hWnd = m_handle;
		if (!hWnd) {
			return;
		}
		PAINTSTRUCT ps;
		HDC hDC = BeginPaint(hWnd, &ps);
		if (!hDC) {
			return;
		}
		if (ps.rcPaint.right > ps.rcPaint.left && ps.rcPaint.bottom > ps.rcPaint.top) {
			RECT rectClient;
			GetClientRect(hWnd, &rectClient);
			Gdiplus::Graphics graphics(hDC);
			Ref<Canvas> canvas = GraphicsPlatform::createCanvas(CanvasType::View, &graphics, rectClient.right, rectClient.bottom, sl_null);
			if (canvas.isNotNull()) {
				canvas->setAntiAlias(sl_false);
				canvas->setInvalidatedRect(Rectangle((sl_real)(ps.rcPaint.left), (sl_real)(ps.rcPaint.top), (sl_real)(ps.rcPaint.right), (sl_real)(ps.rcPaint.bottom)));
				g_flagDuringPaint = sl_true;
				onPaint(canvas.get());
				g_flagDuringPaint = sl_false;
			}
		}
		EndPaint(hWnd, &ps);
	}

	void PlatformViewInstance::onDrawNativeLayer()
	{
		Win32_NativeLayerContext* layer = m_nativeLayer.get();
		if (!layer) {
			return;
		}

		HWND hWnd = m_handle;
		if (!hWnd) {
			return;
		}
		RECT rc;
		GetWindowRect(hWnd, &rc);
		sl_uint32 width = (sl_uint32)(rc.right - rc.left);
		sl_uint32 height = (sl_uint32)(rc.bottom - rc.top);
		if (width < 1 || height < 1) {
			return;
		}

		if (!(layer->prepare(width, height))) {
			return;
		}

		{
			Gdiplus::Graphics graphics(layer->hdcCache);
			Ref<Canvas> canvas = GraphicsPlatform::createCanvas(CanvasType::View, &graphics, width, height, sl_null);
			if (canvas.isNotNull()) {
				canvas->setAntiAlias(sl_false);
				canvas->setInvalidatedRect(Rectangle(0, 0, (sl_real)(width), (sl_real)(height)));
				Ref<View> view = getView();
				if (view.isNotNull()) {
					sl_bool flagOpaque = sl_false;
					Color colorBack;
					if (view->isOpaque()) {
						flagOpaque = sl_true;
					} else {
						Color colorView = view->getBackgroundColor();
						if (colorView.a == 255) {
							flagOpaque = sl_true;
						} else {
							if (m_flagWindowContent) {
								Ref<Window> window = view->getWindow();
								if (window.isNotNull()) {
									colorBack = window->getBackgroundColor();
								}
							}
						}
					}
					if (!flagOpaque) {
						graphics.Clear(Gdiplus::Color(colorBack.a, colorBack.r, colorBack.g, colorBack.b));
					}
					view->dispatchDraw(canvas.get());
				}
			}
		}

		BLENDFUNCTION bf;
		bf.AlphaFormat = AC_SRC_ALPHA;
		bf.SourceConstantAlpha = 255;
		bf.BlendFlags = 0;
		bf.BlendOp = AC_SRC_OVER;
		POINT ptSrc;
		ptSrc.x = 0;
		ptSrc.y = 0;
		SIZE size;
		size.cx = (LONG)width;
		size.cy = (LONG)height;

		if (!(UpdateLayeredWindow(hWnd, NULL, NULL, &size, layer->hdcCache, &ptSrc, 0, &bf, ULW_ALPHA))) {
			DWORD dwErr = GetLastError();
			if (dwErr = ERROR_INVALID_PARAMETER) {
				UIPlatform::setWindowExStyle(m_handle, WS_EX_LAYERED, sl_false);
				UIPlatform::setWindowExStyle(m_handle, WS_EX_LAYERED, sl_true);
				UpdateLayeredWindow(hWnd, NULL, NULL, &size, layer->hdcCache, &ptSrc, 0, &bf, ULW_ALPHA);
			}
		}

	}

	sl_bool PlatformViewInstance::onEventKey(UIAction action, WPARAM wParam, LPARAM lParam)
	{
		HWND hWnd = m_handle;
		if (hWnd) {
			sl_uint32 vkey = (sl_uint32)wParam;
			Keycode key;
			if (action != UIAction::Char) {
				UINT scanCode = (lParam & 0x00ff0000) >> 16;
				sl_bool flagExtended = (lParam & 0x01000000) != 0;
				switch (vkey) {
					case VK_SHIFT:
						vkey = MapVirtualKeyW(scanCode, MAPVK_VSC_TO_VK_EX);
						break;
					case VK_CONTROL:
						vkey = flagExtended ? VK_RCONTROL : VK_LCONTROL;
						break;
					case VK_MENU:
						vkey = flagExtended ? VK_RMENU : VK_LMENU;
						break;
				}
				key = UIEvent::getKeycodeFromSystemKeycode(vkey);
			} else {
				key = Keycode::Unknown;
			}
			Time t;
			t.setMillisecondCount(GetMessageTime());
			Ref<UIEvent> ev = UIEvent::createKeyEvent(action, key, vkey, t);
			if (ev.isNotNull()) {
				UIPlatform::applyEventModifiers(ev.get());
				ev->addFlag(UIEventFlags::DispatchToParent);
				onKeyEvent(ev.get());
				if (ev->getFlags() & UIEventFlags::NotInvokeNative) {
					return sl_true;
				}
			}
		}
		return sl_false;
	}

	sl_bool PlatformViewInstance::onEventMouse(UIAction action, WPARAM wParam, LPARAM lParam, sl_bool flagSetCapture, sl_bool flagReleaseCapture)
	{
		Time t;
		t.setMillisecondCount(GetMessageTime());
		sl_ui_posf x = (sl_ui_posf)((short)(lParam & 0xffff));
		sl_ui_posf y = (sl_ui_posf)((short)((lParam >> 16) & 0xffff));

		if (m_flagRegisteredTouch) {
			if (UIPlatform::isCurrentMessageFromTouch()) {
				if (flagSetCapture && action == UIAction::LeftButtonDown) {
					Ref<UIEvent> ev = UIEvent::createMouseEvent(action, x, y, t);
					if (ev.isNotNull()) {
						releaseDragging();
						UIPlatform::applyEventModifiers(ev.get());
						onDragDropEvent(ev.get());
						if (doDragging(action)) {
							return sl_true;
						}
					}
				}
				return sl_false;
			}
		}
		if (flagReleaseCapture) {
			if (GetCapture()) {
				ReleaseCapture();
			}
		}
		HWND hWnd = m_handle;
		if (hWnd) {
			if (action == UIAction::MouseMove) {
				if (wParam & MK_LBUTTON) {
					action = UIAction::LeftButtonDrag;
				} else if (wParam & MK_RBUTTON) {
					action = UIAction::RightButtonDrag;
				} else if (wParam & MK_MBUTTON) {
					action = UIAction::MiddleButtonDrag;
				}
			}
			Ref<UIEvent> ev = UIEvent::createMouseEvent(action, x, y, t);
			if (ev.isNotNull()) {
				UIPlatform::applyEventModifiers(ev.get());
				ev->addFlag(UIEventFlags::DispatchToParent);
				if (flagSetCapture) {
					ev->addFlag(UIEventFlags::SetCapture);
					releaseDragging();
				}
				onMouseEvent(ev.get());
				if (flagSetCapture) {
					if (doDragging(action)) {
						return sl_true;
					}
					if (ev->getFlags() & UIEventFlags::SetCapture) {
						SetCapture(hWnd);
						return sl_true;
					}
				}
				if (ev->getFlags() & UIEventFlags::NotInvokeNative) {
					return sl_true;
				}
			}
		}
		return sl_false;
	}

	sl_bool PlatformViewInstance::onEventMouseWheel(sl_bool flagVertical, WPARAM wParam, LPARAM lParam)
	{
		HWND hWnd = m_handle;
		if (hWnd) {
			int delta = (short)((wParam >> 16) & 0xffff);
			sl_real deltaX, deltaY;
			if (flagVertical) {
				deltaX = 0;
				deltaY = (sl_real)delta;
			} else {
				deltaX = (sl_real)delta;
				deltaY = 0;
			}
			const DWORD lParam = GetMessagePos();
			POINT pt;
			pt.x = (short)(lParam & 0xffff);
			pt.y = (short)((lParam >> 16) & 0xffff);
			ScreenToClient(hWnd, &pt);
			Time t;
			t.setMillisecondCount(GetMessageTime());
			Ref<UIEvent> ev = UIEvent::createMouseWheelEvent((sl_ui_posf)(pt.x), (sl_ui_posf)(pt.y), deltaX, deltaY, t);
			if (ev.isNotNull()) {
				UIPlatform::applyEventModifiers(ev.get());
				onMouseWheelEvent(ev.get());
				if (ev->getFlags() & UIEventFlags::NotInvokeNative) {
					return sl_true;
				}
			}
		}
		return sl_false;
	}

	sl_bool PlatformViewInstance::onEventTouch(HWND hWnd, WPARAM wParam, LPARAM lParam, sl_bool flagCapture)
	{
		sl_uint32 nTouch = (sl_uint32)(LOWORD(wParam));
		if (!nTouch) {
			return sl_false;
		}
		void* hTouch = (void*)lParam;
		SLIB_SCOPED_BUFFER(user32::TOUCHINPUT, 128, touches, nTouch)
		if (!((user32::getApi_GetTouchInputInfo())(hTouch, nTouch, touches, sizeof(user32::TOUCHINPUT)))) {
			return sl_false;
		}
		Array<TouchPoint> arrPts = Array<TouchPoint>::create(nTouch);
		if (arrPts.isNull()) {
			return sl_false;
		}
		TouchPoint* pts = arrPts.getData();
		sl_uint32 iPrimary = 0;
		sl_bool flagBegin = sl_true;
		sl_bool flagEnd = sl_true;
		for (sl_uint32 i = 0; i < nTouch; i++) {
			TouchPoint& pt = pts[i];
			user32::TOUCHINPUT& input = touches[i];
			POINT point;
			point.x = input.x / 100; // TOUCH_COORD_TO_PIXEL
			point.y = input.y / 100; // TOUCH_COORD_TO_PIXEL
			ScreenToClient(hWnd, &point);
			pt.point.x = (sl_real)(point.x);
			pt.point.y = (sl_real)(point.y);
			pt.pointerId = (sl_uint64)(input.dwID);
			if (input.dwFlags & 4) { // TOUCHEVENTF_UP
				pt.phase = TouchPhase::End;
				flagBegin = sl_false;
			} else if (input.dwFlags & 2) { // TOUCHEVENTF_DOWN
				pt.phase = TouchPhase::Begin;
				flagEnd = sl_false;
			} else {
				pt.phase = TouchPhase::Move;
				flagBegin = sl_false;
				flagEnd = sl_false;
			}
			if (input.dwFlags & 0x10) { // TOUCHEVENTF_PRIMARY
				iPrimary = i;
			}
		}
		UIAction action;
		if (flagEnd) {
			action = UIAction::TouchEnd;
		} else if (flagBegin) {
			action = UIAction::TouchBegin;
		} else {
			action = UIAction::TouchMove;
		}
		Time t;
		t.setMillisecondCount(GetMessageTime());
		Ref<UIEvent> ev = UIEvent::createTouchEvent(action, pts[iPrimary], t);
		if (ev.isNull()) {
			return sl_false;
		}
		ev->setTouchPoints(arrPts);
		onTouchEvent(ev.get());
		if (ev->getFlags() & UIEventFlags::NotInvokeNative) {
			(user32::getApi_CloseTouchInputHandle())(hTouch);
			return sl_true;
		}
		return sl_false;
	}

	sl_bool PlatformViewInstance::onEventSetCursor()
	{
		HWND hWnd = m_handle;
		if (hWnd) {
			const DWORD lParam = GetMessagePos();
			POINT pt;
			pt.x = (short)(lParam & 0xffff);
			pt.y = (short)((lParam >> 16) & 0xffff);
			ScreenToClient(hWnd, &pt);
			Time t;
			t.setMillisecondCount(GetMessageTime());
			Ref<UIEvent> ev = UIEvent::createSetCursorEvent((sl_ui_posf)(pt.x), (sl_ui_posf)(pt.y), t);
			if (ev.isNotNull()) {
				onSetCursor(ev.get());
				updateToolTip(ev->getToolTipOwnerId(), ev->getToolTip());
				if (ev->getFlags() & UIEventFlags::NotInvokeNative) {
					return sl_true;
				}
			}
		}
		return sl_false;
	}

	void PlatformViewInstance::setGenericView(sl_bool flag)
	{
		m_flagGenericView = flag;
	}

	LRESULT PlatformViewInstance::processWindowMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		switch (msg) {
			case WM_ERASEBKGND:
				return TRUE;
			case WM_PAINT:
				onPaint();
				return 0;
			case WM_LBUTTONDOWN:
				if (onEventMouse(UIAction::LeftButtonDown, wParam, lParam, sl_true, sl_false)) {
					return 0;
				}
				break;
			case WM_LBUTTONDBLCLK:
				onEventMouse(UIAction::LeftButtonDown, wParam, lParam, sl_true, sl_false);
				if (onEventMouse(UIAction::LeftButtonDoubleClick, wParam, lParam, sl_false, sl_false)) {
					return 0;
				}
				break;
			case WM_LBUTTONUP:
				if (onEventMouse(UIAction::LeftButtonUp, wParam, lParam, sl_false, sl_true)) {
					return 0;
				}
				break;
			case WM_RBUTTONDOWN:
				if (onEventMouse(UIAction::RightButtonDown, wParam, lParam, sl_true, sl_false)) {
					return 0;
				}
				break;
			case WM_RBUTTONDBLCLK:
				onEventMouse(UIAction::RightButtonDown, wParam, lParam, sl_true, sl_false);
				if (onEventMouse(UIAction::RightButtonDoubleClick, wParam, lParam, sl_false, sl_false)) {
					return 0;
				}
				break;
			case WM_RBUTTONUP:
				if (onEventMouse(UIAction::RightButtonUp, wParam, lParam, sl_false, sl_true)) {
					return 0;
				}
				break;
			case WM_MBUTTONDOWN:
				if (onEventMouse(UIAction::MiddleButtonDown, wParam, lParam, sl_true, sl_false)) {
					return 0;
				}
				break; 
			case WM_MBUTTONDBLCLK:
				onEventMouse(UIAction::MiddleButtonDown, wParam, lParam, sl_true, sl_false);
				if (onEventMouse(UIAction::MiddleButtonDoubleClick, wParam, lParam, sl_false, sl_false)) {
					return 0;
				}
				break;
			case WM_MBUTTONUP:
				if (onEventMouse(UIAction::MiddleButtonUp, wParam, lParam, sl_false, sl_true)) {
					return 0;
				}
				break;
			case WM_MOUSEMOVE:
				{
					TRACKMOUSEEVENT track;
					track.cbSize = sizeof(track);
					track.dwFlags = TME_LEAVE | TME_QUERY;
					track.hwndTrack = hWnd;
					track.dwHoverTime = 0;
					TrackMouseEvent(&track);
					if (!(track.dwFlags)) {
						track.cbSize = sizeof(track);
						track.dwFlags = TME_LEAVE;
						track.hwndTrack = hWnd;
						track.dwHoverTime = HOVER_DEFAULT;
						TrackMouseEvent(&track);
						onEventMouse(UIAction::MouseEnter, wParam, lParam, sl_false, sl_false);
					}
					if (onEventMouse(UIAction::MouseMove, wParam, lParam, sl_false, sl_false)) {
						return 0;
					}
					break;
				}
			case WM_MOUSELEAVE:
				if (onEventMouse(UIAction::MouseLeave, wParam, lParam, sl_false, sl_false)) {
					return 0;
				}
				break;
			case WM_MOUSEWHEEL:
				if (onEventMouseWheel(sl_true, wParam, lParam)) {
					return 0;
				}
				break;
			case 0x020E: // WM_MOUSEHWHEEL
				if (onEventMouseWheel(sl_false, wParam, lParam)) {
					return 0;
				}
				break;
			case 0x0240: // WM_TOUCH
				if (!(onEventTouch(hWnd, wParam, lParam, sl_true))) {
					(user32::getApi_CloseTouchInputHandle())((void*)lParam);
				}
				return 0;
			case WM_KEYDOWN:
			case WM_SYSKEYDOWN:
				if (onEventKey(UIAction::KeyDown, wParam, lParam)) {
					return 0;
				}
				break;
			case WM_KEYUP:
			case WM_SYSKEYUP:
				if (onEventKey(UIAction::KeyUp, wParam, lParam)) {
					return 0;
				}
				break;
			case WM_CHAR:
			case WM_SYSCHAR:
				if (onEventKey(UIAction::Char, wParam, lParam)) {
					return 0;
				}
				break;
			case WM_IME_CHAR:
				onEventKey(UIAction::Char, wParam, lParam);
				return 0;
			case WM_UNICHAR:
				if (wParam != UNICODE_NOCHAR) {
					// application processes this message
					return 1;
				}
				onEventKey(UIAction::Char, wParam, lParam);
				return 0;
			case WM_SETFOCUS:
				onSetFocus();
				break;
			case WM_KILLFOCUS:
				onKillFocus();
				break;
			case WM_SETCURSOR:
				if (onEventSetCursor()) {
					return TRUE;
				}
				break;
			case WM_IME_STARTCOMPOSITION:
				break;
			case WM_IME_COMPOSITION:
				break;
		}
		return DefaultViewInstanceProc(hWnd, msg, wParam, lParam);
	}

	LRESULT PlatformViewInstance::processSubclassMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg) {
			case WM_LBUTTONDOWN:
				if (onEventMouse(UIAction::LeftButtonDown, wParam, lParam, sl_false, sl_false)) {
					return 0;
				}
				break;
			case WM_LBUTTONDBLCLK:
				if (onEventMouse(UIAction::LeftButtonDoubleClick, wParam, lParam, sl_false, sl_false)) {
					return 0;
				}
				break;
			case WM_LBUTTONUP:
				if (onEventMouse(UIAction::LeftButtonUp, wParam, lParam, sl_false, sl_false)) {
					return 0;
				}
				break;
			case WM_RBUTTONDOWN:
				if (onEventMouse(UIAction::RightButtonDown, wParam, lParam, sl_false, sl_false)) {
					return 0;
				}
				break;
			case WM_RBUTTONDBLCLK:
				if (onEventMouse(UIAction::RightButtonDoubleClick, wParam, lParam, sl_false, sl_false)) {
					return 0;
				}
				break;
			case WM_RBUTTONUP:
				if (onEventMouse(UIAction::RightButtonUp, wParam, lParam, sl_false, sl_false)) {
					return 0;
				}
				break;
			case WM_MBUTTONDOWN:
				if (onEventMouse(UIAction::MiddleButtonDown, wParam, lParam, sl_false, sl_false)) {
					return 0;
				}
				break;
			case WM_MBUTTONDBLCLK:
				if (onEventMouse(UIAction::MiddleButtonDoubleClick, wParam, lParam, sl_false, sl_false)) {
					return 0;
				}
				break;
			case WM_MBUTTONUP:
				if (onEventMouse(UIAction::MiddleButtonUp, wParam, lParam, sl_false, sl_false)) {
					return 0;
				}
				break;
			case WM_MOUSEMOVE:
				if (onEventMouse(UIAction::MouseMove, wParam, lParam, sl_false, sl_false)) {
					return 0;
				}
				break;
			case WM_MOUSEWHEEL:
				if (onEventMouseWheel(sl_true, wParam, lParam)) {
					return 0;
				}
				break;
			case 0x020E: // WM_MOUSEHWHEEL
				if (onEventMouseWheel(sl_false, wParam, lParam)) {
					return 0;
				}
				break;
			case 0x0240: // WM_TOUCH
				if (onEventTouch(hWnd, wParam, lParam, sl_false)) {
					return 0;
				}
				break;
			case WM_KEYDOWN:
			case WM_SYSKEYDOWN:
				if (onEventKey(UIAction::KeyDown, wParam, lParam)) {
					return 0;
				}
				if (wParam == VK_TAB) {
					DefSubclassProc(hWnd, WM_CHAR, '\t', lParam);
				} else if (wParam == VK_RETURN) {
					DefSubclassProc(hWnd, WM_CHAR, '\r', lParam);
				}
				break;
			case WM_KEYUP:
			case WM_SYSKEYUP:
				if (onEventKey(UIAction::KeyUp, wParam, lParam)) {
					return 0;
				}
				break;
			case WM_CHAR:
			case WM_SYSCHAR:
				if (onEventKey(UIAction::Char, wParam, lParam)) {
					return 0;
				}
				if (wParam == '\t' || wParam == '\r' || wParam == '\n') {
					return 0;
				}
				break;
			case WM_SETFOCUS:
				onSetFocus();
				break;
			case WM_KILLFOCUS:
				onKillFocus();
				break;
			case WM_SETCURSOR:
				if (onEventSetCursor()) {
					return TRUE;
				}
				break;
		}
		return DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	sl_bool PlatformViewInstance::processCommand(SHORT code, LRESULT& result)
	{
		return sl_false;
	}

	sl_bool PlatformViewInstance::processNotify(NMHDR* nmhdr, LRESULT& result)
	{
		return sl_false;
	}

	sl_bool PlatformViewInstance::processControlColor(UINT msg, HDC hDC, HBRUSH& result)
	{
		return sl_false;
	}

	void PlatformViewInstance::processPostControlColor(UINT msg, HDC hDC, HBRUSH& result)
	{
	}


	Win32_NativeLayerContext::Win32_NativeLayerContext()
	{
		flagInvalidated = sl_false;

		hdcCache = NULL;
		hbmCache = NULL;
		hbmOld = NULL;
		widthCache = 0;
		heightCache = 0;
	}

	Win32_NativeLayerContext::~Win32_NativeLayerContext()
	{
		clear();
	}

	sl_bool Win32_NativeLayerContext::prepare(sl_uint32 width, sl_uint32 height)
	{
		if (!width || !height) {
			return sl_false;
		}
		if (widthCache >= width && heightCache >= height) {
			return sl_true;
		}
		clear();
		HDC hdc = CreateCompatibleDC(NULL);
		if (hdc) {
			BITMAPINFO bmi = { 0 };
			bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			bmi.bmiHeader.biWidth = width;
			bmi.bmiHeader.biHeight = height;
			bmi.bmiHeader.biPlanes = 1;
			bmi.bmiHeader.biBitCount = 32;
			bmi.bmiHeader.biCompression = BI_RGB;
			void* ppBits = sl_null;
			HBITMAP hbm = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &ppBits, NULL, 0);
			if (hbm) {
				hbmOld = SelectObject(hdc, hbm);
				hbmCache = hbm;
				hdcCache = hdc;
				widthCache = width;
				heightCache = height;
				return sl_true;
			}
			DeleteDC(hdc);
		}
		return sl_false;
	}

	void Win32_NativeLayerContext::clear()
	{
		if (hdcCache) {
			if (hbmOld) {
				SelectObject(hdcCache, hbmOld);
			}
			DeleteDC(hdcCache);
			hdcCache = NULL;
		}
		hbmOld = NULL;
		if (hbmCache) {
			DeleteObject(hbmCache);
			hbmCache = NULL;
		}
		widthCache = 0;
		heightCache = 0;
	}


	Win32_ToolTipViewContext::Win32_ToolTipViewContext(): hWndToolTip(sl_null), ownerId(0)
	{
	}

	Win32_ToolTipViewContext::~Win32_ToolTipViewContext()
	{
		if (hWndToolTip) {
			DestroyWindow(hWndToolTip);
		}
	}

	void Win32_ToolTipViewContext::update(PlatformViewInstance* instance, sl_uint64 _ownerId, const String& _toolTip)
	{
		Ref<View> view = instance->getView();
		if (view.isNull()) {
			return;
		}
		if (ownerId == _ownerId && toolTip == _toolTip) {
			return;
		}
		ownerId = _ownerId;
		toolTip = _toolTip;
		if (hWndToolTip) {
			DestroyWindow(hWndToolTip);
		}
		if (toolTip.isNotNull()) {
			HINSTANCE hInstance = GetModuleHandleW(NULL);
			HWND hWndParent = instance->getHandle();
			hWndToolTip = CreateWindowExW(
				0, TOOLTIPS_CLASS, NULL,
				WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
				CW_USEDEFAULT, CW_USEDEFAULT,
				CW_USEDEFAULT, CW_USEDEFAULT,
				hWndParent,
				NULL, hInstance, NULL);
			if (!hWndToolTip) {
				return;
			}
			SendMessageW(hWndToolTip, TTM_SETMAXTIPWIDTH, 0, (LPARAM)(GetSystemMetrics(SM_CXSCREEN) >> 1));
			TOOLINFOW toolInfo = { 0 };
			toolInfo.cbSize = sizeof(toolInfo);
			toolInfo.hwnd = hWndParent;
			toolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
			toolInfo.uId = (UINT_PTR)hWndParent;
			StringCstr16 text(toolTip);
			toolInfo.lpszText = (LPWSTR)(text.getData());
			SendMessageW(hWndToolTip, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);
		}
	}


	Ref<ViewInstance> View::createTypicalInstance(ViewInstance* parent)
	{
		Win32_UI_Shared* shared = Win32_UI_Shared::get();
		if (!shared) {
			return sl_null;
		}

		DWORD styleEx = 0;
		DWORD style = 0;
		if (m_flagCreatingChildInstances) {
			styleEx = WS_EX_CONTROLPARENT;
			style = WS_CLIPCHILDREN;
		}
		Ref<PlatformViewInstance> ret = PlatformViewInstance::create<PlatformViewInstance>(this, parent, (LPCWSTR)((LONG_PTR)(shared->wndClassForView)), sl_null, style, styleEx);
		if (ret.isNotNull()) {
			ret->setGenericView(sl_true);
			if (m_flagUsingTouch) {
				ret->setUsingTouchEvent(this, sl_true);
			}
			return ret;
		}
		return sl_null;
	}

	HWND UIPlatform::getViewHandle(View* view)
	{
		if (view) {
			Ref<ViewInstance> _instance = view->getViewInstance();
			PlatformViewInstance* instance = (PlatformViewInstance*)(_instance.get());
			if (instance) {
				return instance->getHandle();
			}
		}
		return 0;
	}

	Ref<ViewInstance> UIPlatform::createViewInstance(HWND hWnd, sl_bool flagDestroyOnRelease)
	{
		Ref<ViewInstance> ret = UIPlatform::_getViewInstance((void*)hWnd);
		if (ret.isNotNull()) {
			return ret;
		}
		return PlatformViewInstance::create<PlatformViewInstance>(hWnd, flagDestroyOnRelease);
	}

	void UIPlatform::registerViewInstance(HWND hWnd, ViewInstance* instance)
	{
		UIPlatform::_registerViewInstance((void*)hWnd, instance);
	}

	Ref<ViewInstance> UIPlatform::getViewInstance(HWND hWnd)
	{
		return UIPlatform::_getViewInstance((void*)hWnd);
	}

	void UIPlatform::removeViewInstance(HWND hWnd)
	{
		UIPlatform::_removeViewInstance((void*)hWnd);
	}

	HWND UIPlatform::getViewHandle(ViewInstance* _instance)
	{
		PlatformViewInstance* instance = (PlatformViewInstance*)_instance;
		if (instance) {
			return instance->getHandle();
		} else {
			return 0;
		}
	}


	namespace priv
	{
		sl_bool IsAnyViewPainting()
		{
			return g_flagDuringPaint;
		}
	}

}

#endif
