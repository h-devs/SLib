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

#include "slib/ui/definition.h"

#if defined(SLIB_UI_IS_ANDROID)

#include "slib/ui/select_view.h"

#include "view_android.h"

namespace slib
{

	namespace {

		static void JNICALL OnSelect(JNIEnv* env, jobject _this, jlong instance, jint n);

		SLIB_JNI_BEGIN_CLASS(JSelectView, "slib/android/ui/view/UiSelectView")

			SLIB_JNI_STATIC_METHOD(create, "_create", "(Landroid/content/Context;)Lslib/android/ui/view/UiSelectView;");

			SLIB_JNI_STATIC_METHOD(applyList, "_applyList", "(Landroid/view/View;[Ljava/lang/String;)V");
			SLIB_JNI_STATIC_METHOD(select, "_select", "(Landroid/view/View;I)V");
			SLIB_JNI_STATIC_METHOD(setAlignment, "_setAlignment", "(Landroid/view/View;I)Z");
			SLIB_JNI_STATIC_METHOD(setTextColor, "_setTextColor", "(Landroid/view/View;I)Z");
			SLIB_JNI_STATIC_METHOD(setBorder, "_setBorder", "(Landroid/view/View;Z)Z");
			SLIB_JNI_STATIC_METHOD(setBackgroundColor, "_setBackgroundColor", "(Landroid/view/View;I)Z");
			SLIB_JNI_STATIC_METHOD(setFont, "_setFont", "(Landroid/view/View;Lslib/android/ui/UiFont;)Z");

			SLIB_JNI_NATIVE(nativeOnSelect, "nativeOnSelect", "(JI)V", OnSelect);

		SLIB_JNI_END_CLASS

		static void Select(jobject jview, sl_uint32 n)
		{
			JSelectView::select.call(sl_null, jview, n);
		}

		class SelectViewHelper : public SelectView
		{
		public:
			void refreshItems(jobject jview)
			{
				sl_uint32 n = (sl_uint32)(getItemCount());
				JniLocal<jobjectArray> arr = Jni::newStringArray(n);
				if (arr.isNotNull()) {
					for (sl_uint32 i = 0; i < n; i++) {
						Jni::setStringArrayElement(arr, i, getItemTitle(i));
					}
					JSelectView::applyList.call(sl_null, jview, arr.get());
					Select(jview, m_indexSelected);
				}
			}

			using SelectView::_onSelectItem_NW;

		};

		class SelectViewInstance : public PlatformViewInstance, public ISelectViewInstance
		{
			SLIB_DECLARE_OBJECT

		public:
			void initialize(View* _view) override
			{
				SelectView* view = (SelectView*)_view;
				jobject jhandle = getHandle();

				JSelectView::setAlignment.callBoolean(sl_null, jhandle, view->getGravity().value);
				JSelectView::setTextColor.callBoolean(sl_null, jhandle, view->getTextColor().getARGB());
				JSelectView::setBorder.callBoolean(sl_null, jhandle, view->hasBorder());
				JSelectView::setBackgroundColor.callBoolean(sl_null, jhandle, view->getBackgroundColor().getARGB());
				setFont(view, view->getFont());
				refreshItems(view);
			}

			void selectItem(SelectView* view, sl_uint32 index) override
			{
				jobject handle = m_handle.get();
				if (handle) {
					Select(handle, index);
				}
			}

			void refreshItems(SelectView* view) override
			{
				jobject handle = m_handle.get();
				if (handle) {
					(static_cast<SelectViewHelper*>(view))->refreshItems(handle);
				}
			}

			void setGravity(SelectView* view, const Alignment& align) override
			{
				jobject handle = m_handle.get();
				if (handle) {
					JSelectView::setAlignment.callBoolean(sl_null, handle, align.value);
				}
			}

			void setTextColor(SelectView* view, const Color& color) override
			{
				jobject handle = m_handle.get();
				if (handle) {
					JSelectView::setTextColor.callBoolean(sl_null, handle, color.getARGB());
				}
			}

			void setBorder(View* view, sl_bool flag) override
			{
				jobject handle = m_handle.get();
				if (handle) {
					JSelectView::setBorder.callBoolean(sl_null, handle, flag);
				}
			}

			void setBackgroundColor(View* view, const Color& color) override
			{
				jobject handle = m_handle.get();
				if (handle) {
					JSelectView::setBackgroundColor.callBoolean(sl_null, handle, color.getARGB());
				}
			}

			void setFont(View* view, const Ref<Font>& font) override
			{
				jobject handle = m_handle.get();
				if (handle) {
					jobject jfont = GraphicsPlatform::getNativeFont(font.get());
					if (jfont) {
						JSelectView::setFont.callBoolean(sl_null, handle, jfont);
					}
				}
			}

		};

		SLIB_DEFINE_OBJECT(SelectViewInstance, PlatformViewInstance)

		void JNICALL OnSelect(JNIEnv* env, jobject _this, jlong instance, jint n)
		{
			Ref<SelectViewHelper> helper = CastRef<SelectViewHelper>(PlatformViewInstance::findView(instance));
			if (helper.isNotNull()) {
				helper->_onSelectItem_NW(n);
			}
		}

	}

	Ref<ViewInstance> SelectView::createNativeWidget(ViewInstance* _parent)
	{
		PlatformViewInstance* parent = (PlatformViewInstance*)_parent;
		JniLocal<jobject> handle = JSelectView::create.callObject(sl_null, parent->getContext());
		return PlatformViewInstance::create<SelectViewInstance>(this, parent, handle.get());
	}

	Ptr<ISelectViewInstance> SelectView::getSelectViewInstance()
	{
		return CastRef<SelectViewInstance>(getViewInstance());
	}

}

#endif
