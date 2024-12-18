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

#include "window.h"

#include "view_android.h"

namespace slib
{

	namespace
	{
		SLIB_JNI_BEGIN_CLASS(JPoint, "android/graphics/Point")
			SLIB_JNI_INT_FIELD(x);
			SLIB_JNI_INT_FIELD(y);
		SLIB_JNI_END_CLASS

		SLIB_JNI_BEGIN_CLASS(JRect, "android/graphics/Rect")
			SLIB_JNI_INT_FIELD(left);
			SLIB_JNI_INT_FIELD(top);
			SLIB_JNI_INT_FIELD(right);
			SLIB_JNI_INT_FIELD(bottom);
		SLIB_JNI_END_CLASS

		static void JNICALL OnResize(JNIEnv* env, jobject _this, jlong instance, int w, int h);
		static jboolean JNICALL OnClose(JNIEnv* env, jobject _this, jlong instance);

		SLIB_JNI_BEGIN_CLASS(JWindow, "slib/android/ui/window/UiWindow")

			SLIB_JNI_STATIC_METHOD(create, "create", "(Landroid/app/Activity;ZZIIII)Lslib/android/ui/window/UiWindow;");

			SLIB_JNI_LONG_FIELD(instance);

			SLIB_JNI_METHOD(getContentView, "getContentView", "()Landroid/view/View;");
			SLIB_JNI_METHOD(close, "close", "()V");
			SLIB_JNI_METHOD(isActive, "isActive", "()Z");
			SLIB_JNI_METHOD(activate, "activate", "()V");
			SLIB_JNI_METHOD(getFrame, "getFrame", "()Landroid/graphics/Rect;");
			SLIB_JNI_METHOD(setFrame, "setFrame", "(IIII)V");
			SLIB_JNI_METHOD(setBackgroundColor, "setWindowBackgroundColor", "(I)V");
			SLIB_JNI_METHOD(setVisible, "setVisible", "(Z)V");
			SLIB_JNI_METHOD(setAlwaysOnTop, "setAlwaysOnTop", "(Z)V");
			SLIB_JNI_METHOD(setAlpha, "setWindowAlpha", "(F)V");

			SLIB_JNI_NATIVE(onResize, "nativeOnResize", "(JII)V", OnResize);
			SLIB_JNI_NATIVE(onClose, "nativeOnClose", "(J)Z", OnClose);

		SLIB_JNI_END_CLASS

		class Android_WindowInstance : public WindowInstance
		{
		public:
			JniGlobal<jobject> m_handle;
			AtomicRef<ViewInstance> m_viewContent;
			sl_bool m_flagClosed;

		public:
			Android_WindowInstance()
			{
				m_flagClosed = sl_false;
			}

			~Android_WindowInstance()
			{
				close();
			}

		public:
			static Ref<Android_WindowInstance> create(jobject jwindow)
			{
				if (!jwindow) {
					return sl_null;
				}
				JniLocal<jobject> jcontent = JWindow::getContentView.callObject(jwindow);
				if (jcontent.isNotNull()) {
					JniGlobal<jobject> handle = JniGlobal<jobject>::create(jwindow);
					if (handle.isNotNull()) {
						Ref<ViewInstance> content = UIPlatform::createViewInstance(jcontent);
						if (content.isNotNull()) {
							Ref<Android_WindowInstance> ret = new Android_WindowInstance();
							if (ret.isNotNull()) {
								content->setWindowContent(sl_true);
								jwindow = handle.get();
								ret->m_handle = Move(handle);
								ret->m_viewContent = Move(content);
								jlong instance = (jlong)(jwindow);
								JWindow::instance.set(jwindow, instance);
								UIPlatform::registerWindowInstance(jwindow, ret.get());
								return ret;
							}
						}
					}
				}
				return sl_null;
			}

			static JniLocal<jobject> createHandle(Window* window)
			{
				jobject context = (jobject)(window->getActivity());
				if (!context) {
					context = Android::getCurrentContext();
					if (!context) {
						return sl_null;
					}
				}
				JniLocal<jobject> jwindow = JWindow::create.callObject(sl_null, context, window->isFullScreen(), window->isCenterScreen(), (int)(window->getLeft()), (int)(window->getTop()), (int)(window->getWidth()), (int)(window->getHeight()));
				if (jwindow) {
					if (!(window->isDefaultBackgroundColor())) {
						Color color = window->getBackgroundColor();
						JWindow::setBackgroundColor.call(jwindow, color.getARGB());
					}
					sl_real alpha = window->getAlpha();
					if (alpha <= 0.9999f) {
						JWindow::setAlpha.call(jwindow, (jfloat)alpha);
					}
					if (window->isAlwaysOnTop()) {
						JWindow::setAlwaysOnTop.call(jwindow, 1);
					}
					return jwindow;
				}
				return sl_null;
			}

		public:
			void* getHandle() override
			{
				return m_handle;
			}

			void close() override
			{
				ObjectLocker lock(this);
				m_viewContent.setNull();
				if (m_flagClosed) {
					return;
				}
				m_flagClosed = sl_true;
				jobject jwindow = m_handle;
				if (jwindow) {
					UIPlatform::removeWindowInstance(jwindow);
					JWindow::close.call(jwindow);
					m_handle.setNull();
				}
			}

			sl_bool isClosed() override
			{
				return m_handle.isNull();
			}

			void setParentHandle(void* parent) override
			{
			}

			Ref<ViewInstance> getContentView() override
			{
				return m_viewContent;
			}

			sl_bool getFrame(UIRect& _out) override
			{
				if (m_flagClosed) {
					return sl_false;
				}
				jobject jwindow = m_handle;
				if (jwindow) {
					JniLocal<jobject> rect = JWindow::getFrame.callObject(jwindow);
					if (rect.isNotNull()) {
						_out.left = (sl_ui_pos)(JRect::left.get(rect));
						_out.top = (sl_ui_pos)(JRect::top.get(rect));
						_out.right = (sl_ui_pos)(JRect::right.get(rect));
						_out.bottom = (sl_ui_pos)(JRect::bottom.get(rect));
						_out.fixSizeError();
						return sl_true;
					}
				}
				return sl_false;
			}

			void setFrame(const UIRect& frame) override
			{
				if (m_flagClosed) {
					return;
				}
				jobject jwindow = m_handle;
				if (jwindow) {
					JWindow::setFrame.call(jwindow, (int)(frame.left), (int)(frame.top), (int)(frame.right), (int)(frame.bottom));
				}
			}

			sl_bool isActive() override
			{
				if (m_flagClosed) {
					return sl_false;
				}
				jobject jwindow = m_handle;
				if (jwindow) {
					return JWindow::isActive.callBoolean(jwindow);
				}
				return sl_false;
			}

			void activate() override
			{
				if (m_flagClosed) {
					return;
				}
				jobject jwindow = m_handle;
				if (jwindow) {
					JWindow::activate.call(jwindow);
				}
			}

			void setBackgroundColor(const Color& color) override
			{
				if (m_flagClosed) {
					return;
				}
				jobject jwindow = m_handle;
				if (jwindow) {
					JWindow::setBackgroundColor.call(jwindow, color.getARGB());
				}
			}

			void setVisible(sl_bool flag) override
			{
				if (m_flagClosed) {
					return;
				}
				jobject jwindow = m_handle;
				if (jwindow) {
					JWindow::setVisible.call(jwindow, flag);
				}
			}

			void setAlwaysOnTop(sl_bool flag) override
			{
				if (m_flagClosed) {
					return;
				}
				jobject jwindow = m_handle;
				if (jwindow) {
					JWindow::setAlwaysOnTop.call(jwindow, flag);
				}
			}

			void setAlpha(sl_real alpha) override
			{
				if (m_flagClosed) {
					return;
				}
				jobject jwindow = m_handle;
				if (jwindow) {
					JWindow::setAlpha.call(jwindow, (jfloat)alpha);
				}
			}
		};

		SLIB_INLINE static Ref<Android_WindowInstance> GetWindowInstance(jlong instance)
		{
			return Ref<Android_WindowInstance>::cast(UIPlatform::getWindowInstance((jobject)instance));
		}

		void JNICALL OnResize(JNIEnv* env, jobject _this, jlong _instance, int w, int h)
		{
			Ref<Android_WindowInstance> instance = GetWindowInstance(_instance);
			if (instance.isNotNull()) {
				instance->onResize((sl_ui_pos)w, (sl_ui_pos)h);
			}
		}

		jboolean JNICALL OnClose(JNIEnv* env, jobject _this, jlong _instance)
		{
			Ref<Android_WindowInstance> instance = GetWindowInstance(_instance);
			if (instance.isNotNull()) {
				return instance->onClose();
			}
			return 1;
		}
	}

	Ref<WindowInstance> Window::createWindowInstance()
	{
		JniLocal<jobject> jwindow = Android_WindowInstance::createHandle(this);
		if (jwindow.isNotNull()) {
			return Android_WindowInstance::create(jwindow);
		}
		return sl_null;
	}


	Ref<WindowInstance> UIPlatform::createWindowInstance(jobject jwindow)
	{
		Ref<WindowInstance> instance = UIPlatform::_getWindowInstance((void*)jwindow);
		if (instance.isNotNull()) {
			return instance;
		}
		return Android_WindowInstance::create(jwindow);
	}

	void UIPlatform::registerWindowInstance(jobject jwindow, WindowInstance* instance)
	{
		UIPlatform::_registerWindowInstance((void*)jwindow, instance);
	}

	Ref<WindowInstance> UIPlatform::getWindowInstance(jobject jwindow)
	{
		return UIPlatform::_getWindowInstance((void*)jwindow);
	}

	void UIPlatform::removeWindowInstance(jobject jwindow)
	{
		UIPlatform::_removeWindowInstance((void*)jwindow);
	}

	jobject UIPlatform::getWindowHandle(WindowInstance* _instance)
	{
		Android_WindowInstance* instance = (Android_WindowInstance*)_instance;
		if (instance) {
			return instance->m_handle.get();
		} else {
			return 0;
		}
	}

}

#endif
