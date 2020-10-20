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

#include "slib/core/definition.h"

#if defined(SLIB_UI_IS_GTK)

#include "slib/ui/edit_view.h"

#include "slib/core/log.h"

#include "view_gtk.h"

namespace slib
{
	namespace priv
	{
		namespace edit_view
		{
			class EditViewInstance : public GTK_ViewInstance, public IEditViewInstance
			{
				SLIB_DECLARE_OBJECT

			public:
				EditViewInstance()
				{
				}

				~EditViewInstance()
				{
				}

			public:
				void initialize(View* _view) override
				{
					EditView* view = (EditView*)_view;
					GtkEntry* handle = (GtkEntry*)m_handle;

					GTK_WIDGET_SET_FLAGS(handle, GTK_CAN_FOCUS);
					String text = view->getText();
					if (text.isNotEmpty()) {
						gtk_entry_set_text(handle, text.getData());
					}
					if(view->isPassword()) {
						gtk_entry_set_visibility(handle, 0);
					}
					setTextColor(view, view->getTextColor());
					setGravity(view, view->getGravity());
					setReadOnly(view, view->isReadOnly());

					g_signal_connect((GtkEditable*)handle, "changed", G_CALLBACK(onChange), handle);
				}

				sl_bool getText(EditView* view, String& _out) override
				{
					GtkEntry* handle = (GtkEntry*)m_handle;
					if (handle) {
						_out = gtk_entry_get_text(handle);
						return sl_true;
					}
					return sl_false;
				}

				void setText(EditView* view, const String& text) override
				{
					GtkEntry* handle = (GtkEntry*)m_handle;
					if (handle) {
						gtk_entry_set_text(handle, text.getData());
					}
				}

				void setGravity(EditView* view, const Alignment& gravity) override
				{
					GtkEntry* handle = (GtkEntry*)m_handle;
					if (handle) {
						float alignment = 0;
						Alignment align = gravity & Alignment::HorizontalMask;
						if (align == Alignment::Center) {
							alignment = 0.5;
						} else if (align == Alignment::Right) {
							alignment = 1;
						} 
						gtk_entry_set_alignment(handle, alignment);
					}
				}

				void setTextColor(EditView* view, const Color& color) override
				{
					GtkWidget* handle = m_handle;
					if (handle) {
						GdkColor gdkColor;
						UIPlatform::getGdkColor(color, &gdkColor);
						gtk_widget_modify_text(handle, GTK_STATE_NORMAL, &gdkColor);
					}
				}

				void setHintText(EditView* view, const String& text) override
				{
				}

				void setHintGravity(EditView* view, const Alignment& gravity) override
				{
				}

				void setHintTextColor(EditView* view, const Color& color) override
				{
				}

				void setHintFont(EditView* view, const Ref<Font>& font) override
				{
				}

				void setReadOnly(EditView* view, sl_bool flag) override
				{
					GtkEntry* handle = (GtkEntry*)m_handle;
					if (handle) {
						handle->editable = !flag;
					}
				}

				void setPassword(EditView* view, sl_bool flag) override
				{
					GtkEntry* handle = (GtkEntry*)m_handle;
					if (handle) {
						gtk_entry_set_visibility(handle, !flag);
					}
				}

				void setMultiLine(EditView* view, MultiLineMode mode) override
				{
				}

				void setBackgroundColor(View* view, const Color& color) override
				{
					GtkWidget* handle = m_handle;
					if (handle) {
						GdkColor gdkColor;
						UIPlatform::getGdkColor(color, &gdkColor);
						gtk_widget_modify_bg(handle, GTK_STATE_NORMAL, &gdkColor);
					}
				}

				sl_ui_len measureHeight(EditView* view) override
				{
					Ref<Font> font = view->getFont();
					if (font.isNotNull()) {
						return (sl_ui_len)(font->getFontHeight() * 1.5f) + 2;
					}
					return 0;
				}

				static void onChange(GtkEditable*, gpointer user_data)
				{
					GtkEntry* handle = (GtkEntry*)user_data;
					Ref<EditView> view = CastRef<EditView>(UIPlatform::getView((GtkWidget*)handle));
					if (view.isNotNull()) {
						String text = gtk_entry_get_text(handle);
						String textNew = text;
						view->dispatchChange(textNew);
						if (text != textNew) {
							gtk_entry_set_text(handle, textNew.getData());
						}
					}
				}

			};


			SLIB_DEFINE_OBJECT(EditViewInstance, GTK_ViewInstance)

			class TextAreaInstance: public GTK_ViewInstance, public IEditViewInstance
			{
				SLIB_DECLARE_OBJECT

			public:
				GtkTextView* m_handleTextView;

			public:
				TextAreaInstance()
				{
					m_handleTextView = sl_null;
				}

			public:
				void initialize(View* _view) override
				{
					GtkScrolledWindow* handle = (GtkScrolledWindow*)m_handle;
					TextArea* view = (TextArea*)_view;

					gtk_scrolled_window_set_policy(handle, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
					gtk_scrolled_window_set_shadow_type(handle, GTK_SHADOW_ETCHED_IN);

					GtkTextView* handleText = (GtkTextView*)(gtk_text_view_new());
					if (handleText) {
						m_handleTextView = handleText;

						GTK_WIDGET_SET_FLAGS(handleText, GTK_CAN_FOCUS);
						gtk_container_add((GtkContainer*)handle, (GtkWidget*)handleText);
						gtk_widget_show((GtkWidget*)handleText);

						setText(view, view->getText());
						setTextColor(view, view->getTextColor());
						setGravity(view, view->getGravity());
						setReadOnly(view, view->isReadOnly());
						setFont(view, view->getFont());				
		
						GtkTextBuffer* buffer = gtk_text_view_get_buffer(handleText);
						if (buffer) {
							g_signal_connect(buffer, "changed", G_CALLBACK(onChange), handle);
						}
					}
				}

				static String _getText(GtkTextBuffer* buffer)
				{
					GtkTextIter start, end;
					gtk_text_buffer_get_start_iter(buffer, &start);
					gtk_text_buffer_get_end_iter(buffer, &end);
					gchar* sz = gtk_text_buffer_get_text(buffer, &start, &end, 1);
					if (sz) {
						String ret = sz;
						g_free(sz);
						return ret;
					}
					return sl_null;
				}

				static String _getText(GtkTextView* handle)
				{
					GtkTextBuffer* buffer = gtk_text_view_get_buffer(handle);
					if (buffer) {
						return _getText(buffer);
					}
					return sl_null;
				}

				sl_bool getText(EditView* view, String& _out) override
				{
					GtkTextView* handle = m_handleTextView;
					if (handle) {
						_out = _getText(handle);
						return sl_true;
					}
					return sl_false;
				}

				void setText(EditView* view, const String& text) override
				{
					GtkTextView* handle = m_handleTextView;
					if (handle) {
						GtkTextBuffer* buffer = gtk_text_view_get_buffer(handle);
						gtk_text_buffer_set_text(buffer, text.getData(), text.getLength());
					}
				}

				void setGravity(EditView* view, const Alignment& gravity) override
				{
					GtkTextView* handle = m_handleTextView;
					if (handle) {
						GtkJustification alignment = GTK_JUSTIFY_LEFT;
						Alignment align = gravity & Alignment::HorizontalMask;
						if (align == Alignment::Center) {
							alignment = GTK_JUSTIFY_CENTER;
						} else if (align == Alignment::Right) {
							alignment = GTK_JUSTIFY_RIGHT;
						}
						gtk_text_view_set_justification(handle, alignment);
					}
				}

				void setTextColor(EditView* view, const Color& color) override
				{
					GtkTextView* handle = m_handleTextView;
					if (handle) {
						GdkColor gdkColor;
						UIPlatform::getGdkColor(color, &gdkColor);
						gtk_widget_modify_text((GtkWidget*)handle, GTK_STATE_NORMAL, &gdkColor);
					}
				}

				void setHintText(EditView* view, const String& text) override
				{
				}

				void setHintGravity(EditView* view, const Alignment& gravity) override
				{
				}

				void setHintTextColor(EditView* view, const Color& color) override
				{
				}

				void setHintFont(EditView* view, const Ref<Font>& font) override
				{
				}

				void setReadOnly(EditView* view, sl_bool flag) override
				{
					GtkTextView* handle = m_handleTextView;
					if (handle) {
						gtk_text_view_set_editable(handle, !flag);
					}
				}

				void setPassword(EditView* view, sl_bool flag) override
				{
				}

				void setMultiLine(EditView* view, MultiLineMode mode) override
				{
				}

				void setPadding(View* view, const UIEdgeInsets& inset) override
				{
					GtkTextView* handle = m_handleTextView;
					if (handle) {
						gtk_text_view_set_left_margin(handle, inset.left);
						gtk_text_view_set_right_margin (handle, inset.right);
					}
				}

				void setFocus(View *view, sl_bool flag) override
				{
					GtkWidget* handle = (GtkWidget*)m_handleTextView;
					if (handle) {
						if (flag) {
							gtk_widget_grab_focus(handle);
						}
					}
				}

				void setFont(View *view, const Ref<Font>& font) override
				{
					GtkWidget* handle = (GtkWidget*)m_handleTextView;
					if (handle) {
						UIPlatform::setWidgetFont(handle, font);
					}
				}

				sl_ui_len measureHeight(EditView* view) override
				{
					GtkTextView* handle = m_handleTextView;
					if (handle) {
						GtkTextBuffer* buffer = gtk_text_view_get_buffer(handle);
						if (buffer) {
							GtkTextIter end;
							gtk_text_buffer_get_end_iter(buffer, &end);
							gint y;
							gint height;
							gtk_text_view_get_line_yrange(handle, &end, &y, &height);
							return (sl_ui_len)(y + height + 4);
						}
					}
					return 0;
				}

				gboolean onKeyEvent(GdkEventKey* gevent) override
				{
					return 0;
				}

				static void onChange(GtkTextBuffer* buffer, gpointer user_data)
				{
					GtkWidget* handle = (GtkWidget*)user_data;
					Ref<TextArea> view = CastRef<TextArea>(UIPlatform::getView(handle));
					if (view.isNotNull()) {
						String text = _getText(buffer);
						String textNew = text;
						view->dispatchChange(textNew);
						if (text != textNew) {
							gtk_text_buffer_set_text(buffer, textNew.getData(), textNew.getLength());
						}
					}
				}

			};

			SLIB_DEFINE_OBJECT(TextAreaInstance, GTK_ViewInstance)

		}
	}

	using namespace priv::edit_view;

	Ref<ViewInstance> EditView::createNativeWidget(ViewInstance* _parent)
	{
		GTK_ViewInstance* parent = static_cast<GTK_ViewInstance*>(_parent);		
		GtkWidget* handle = gtk_entry_new();
		return GTK_ViewInstance::create<EditViewInstance>(this, parent, handle);
	}

	Ptr<IEditViewInstance> EditView::getEditViewInstance()
	{
		return CastRef<EditViewInstance>(getViewInstance());
	}

	Ref<ViewInstance> TextArea::createNativeWidget(ViewInstance* _parent)
	{
		GTK_ViewInstance* parent = static_cast<GTK_ViewInstance*>(_parent);
		GtkScrolledWindow* handle = (GtkScrolledWindow*)(gtk_scrolled_window_new(sl_null, sl_null));
		return GTK_ViewInstance::create<TextAreaInstance>(this, parent, (GtkWidget*)handle);
	}
	
	Ptr<IEditViewInstance> TextArea::getEditViewInstance()
	{
		return CastRef<TextAreaInstance>(getViewInstance());
	}

}

#endif
