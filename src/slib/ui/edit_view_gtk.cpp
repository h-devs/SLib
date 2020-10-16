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

#include "view_gtk.h"
#include "slib/core/log.h"

namespace slib
{
    namespace priv
	{
		namespace edit_view
		{
			class EditViewInstance : public GTK_ViewInstance, public IEditViewInstance
			{
				SLIB_DECLARE_OBJECT

			protected:
				Color m_colorText;
				Color m_colorBackground;

			public:
				EditViewInstance()
				{
					m_colorText = Color::zero();
					m_colorBackground = Color::zero();
				}

				~EditViewInstance()
				{
				}

			public:
				Ref<EditView> getView()
				{
					return CastRef<EditView>(GTK_ViewInstance::getView());
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

				void setTextColor(const Color& color)
				{
					m_colorText = color;
				}

				void setTextColor(EditView* view, const Color& color) override
				{
					m_colorText = color;
                    GtkWidget* handle = m_handle;
					if (handle) {
                        GdkColor gdkColor;
                        gdkColor.red = (color.r << 8) + 0xff;
                        gdkColor.green = (color.g << 8) + 0xff;
                        gdkColor.blue = (color.b << 8) + 0xff;
                        gtk_widget_modify_text (handle, GTK_STATE_NORMAL, &gdkColor);
					}
				}

				void setHintText(EditView* view, const String& text) override
                {
					GtkWidget* handle = m_handle;
					if (handle) {
						String16 s = String16::from(text);

					}
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
						gtk_entry_set_invisible_char(handle, '*');
						gtk_entry_set_visibility(handle, !flag);
					}
				}

				void setMultiLine(EditView* view, MultiLineMode mode) override
				{

				}

				void apply(EditView* view)
				{
					GtkEntry* handle = (GtkEntry*)m_handle;
					String text = view->getText();
					gtk_entry_set_text(handle, text.getData());
					if(view->isPassword()) {
						setPassword(view, true);
					}
                    setTextColor(view, view->getTextColor());
					setGravity(view, view->getGravity());
					setReadOnly(view, view->isReadOnly());
				}

				sl_ui_len measureHeight(EditView* view) override
				{
					GtkEntry* handle = (GtkEntry*)m_handle;
					if (handle) {
/*
						int nLines = 1;
						if (view->getMultiLine() != MultiLineMode::Single) {
							nLines = (int)(SendMessageW(handle, EM_GETLINECOUNT, 0, 0));
							if (nLines < 1) {
								nLines = 1;
							}
						}
						Ref<Font> font = m_font;
						if (font.isNotNull()) {
							sl_ui_len height = nLines * (sl_ui_len)(font->getFontHeight());
							height += 4;
							if (view->isBorder()) {
								height += 2;
							}
							return height;
						}
*/
					}
					return 0;
				}

				void setBackgroundColor(const Color& color)
				{
					if (m_colorBackground == color) {
						return;
					}
					m_colorBackground = color;
				}

				void setBackgroundColor(View* view, const Color& color) override
				{
					// This code should be updated later.
					GtkWidget* handle = m_handle;
					if (handle) {
						GdkColor gdkColor;
						gdkColor.red = color.r;
						gdkColor.green = color.g;
						gdkColor.blue = color.b;
						setBackgroundColor(color);
						G_GNUC_BEGIN_IGNORE_DEPRECATIONS
							GtkWidget* parent = gtk_widget_get_parent(handle);
							if(parent){
								gtk_widget_modify_bg(parent, GTK_STATE_NORMAL, &gdkColor);
							}	
							
						G_GNUC_END_IGNORE_DEPRECATIONS
					}
				}
				void installEventHandlers()
				{
					GtkWidget* handle = m_handle;
					GtkEntryBuffer *buffer = gtk_entry_get_buffer((GtkEntry*)handle);
					g_signal_connect((GtkEditable*)handle, "changed", G_CALLBACK(onChange), handle);
					g_signal_connect(handle, "key-press-event", G_CALLBACK(eventCallback), handle);
				}
				static void onChange(GtkEditable *editable, gpointer user_data)
				{
					Ref<GTK_ViewInstance> instance = Ref<GTK_ViewInstance>::from(UIPlatform::getViewInstance((GtkWidget*)user_data));
					Ref<EditViewInstance> editViewInstance = CastRef<EditViewInstance>(instance);
					if (instance.isNotNull()) {
						GtkEntryBuffer *buffer = gtk_entry_get_buffer((GtkEntry*)editable);
						String text = gtk_entry_buffer_get_text(buffer);
						String textNew = text;
						Ref<EditView> view = CastRef<EditView>(instance->getView());
						view->dispatchChange(&textNew);
						if (text != textNew) {
							editViewInstance->setText(view, textNew);
						}
					}
				}

			};

			SLIB_DEFINE_OBJECT(EditViewInstance, GTK_ViewInstance)

			class TextAreaInstance: public GTK_ViewInstance, public IEditViewInstance
			{
				SLIB_DECLARE_OBJECT

			public:
				String16 m_hintText;
				Alignment m_hintGravity;
				Color m_hintTextColor;
				Ref<Font> m_hintFont;
				sl_uint32 m_heightRequested;

			public:
				TextAreaInstance()
				{
					m_hintTextColor = Color(120, 120, 120);
					m_heightRequested = 0;
				}

			public:
				Ref<TextArea> getView()
				{
					return CastRef<TextArea>(GTK_ViewInstance::getView());
				}

				sl_bool getText(EditView* view, String& _out) override
				{
					GtkContainer* container = (GtkContainer*)m_handle;
					GList *children = gtk_container_get_children (container);
					GtkTextView* handle = (GtkTextView*)children->data;
					if (handle) {
						GtkTextBuffer *buffer = gtk_text_view_get_buffer (handle);
						GtkTextIter start, end;
						gtk_text_buffer_get_start_iter (buffer, &start);
						gtk_text_buffer_get_end_iter (buffer, &end);
						gtk_text_buffer_get_text (buffer, &start, &end, true);
						return sl_true;
					}
					return sl_false;
				}

				void setText(EditView* view, const String& text) override
				{
					GtkContainer* container = (GtkContainer*)m_handle;
					GList *children = gtk_container_get_children (container);
					GtkTextView* handle = (GtkTextView*)children->data;
					if (handle) {
						GtkTextBuffer *buffer = gtk_text_view_get_buffer (handle);
						gtk_text_buffer_set_text (buffer, text.getData(), -1);
					}
				}

				void setGravity(EditView* view, const Alignment& gravity) override
				{
					GtkContainer* container = (GtkContainer*)m_handle;
					GList *children = gtk_container_get_children (container);
					GtkTextView* handle = (GtkTextView*)children->data;
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
					GtkContainer* container = (GtkContainer*)m_handle;
					GList *children = gtk_container_get_children (container);
					GtkWidget* handle = (GtkWidget*)children->data;
					if (handle) {
						GdkColor gdkColor;
						gdkColor.red = (color.r << 8) + 0xff;
						gdkColor.green = (color.g << 8) + 0xff;
						gdkColor.blue = (color.b << 8) + 0xff;
						gtk_widget_modify_text (handle, GTK_STATE_NORMAL, &gdkColor);
					}
				}

				void setHintText(EditView* view, const String& text) override
				{
					m_hintText = String16::from(text);
					GtkWidget* handle = m_handle;
					if (handle) {

					}
				}

				void setHintGravity(EditView* view, const Alignment& gravity) override
				{
					m_hintGravity = gravity;
					GtkWidget* handle = m_handle;
					if (handle) {

					}
				}

				void setHintTextColor(EditView* view, const Color& color) override
				{
					m_hintTextColor = color;
					GtkWidget* handle = m_handle;
					if (handle) {

					}
				}

				void setHintFont(EditView* view, const Ref<Font>& font) override
				{
					m_hintFont = font;
					GtkWidget* handle = m_handle;
					if (handle) {

					}
				}

				void setReadOnly(EditView* view, sl_bool flag) override
				{
					GtkContainer* container = (GtkContainer*)m_handle;
					GList *children = gtk_container_get_children (container);
					GtkTextView* handle = (GtkTextView*)children->data;
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

				sl_ui_len measureHeight(EditView* view) override
				{
					GtkWidget* handle = m_handle;
					if (handle) {
						sl_ui_len height = m_heightRequested;
						if (height > 0) {
							if (view->isBorder()) {
								height += 8;
							}
							return height;
						}
					}
					return 0;
				}
/*
				void setBackgroundColor(View* view, const Color& color) override
				{
					GtkWidget* handle = m_handle;
					if (handle) {
						if (color.a == 0) {
							SendMessageW(handle, EM_SETBKGNDCOLOR, 0, (LPARAM)(0xFFFFFF));
						} else {
							SendMessageW(handle, EM_SETBKGNDCOLOR, 0, (LPARAM)(GraphicsPlatform::getColorRef(color)));
						}
					}
				}
*/
				void setPadding(View* view, const UIEdgeInsets& inset) override
				{
					GtkContainer* container = (GtkContainer*)m_handle;
					GList *children = gtk_container_get_children (container);
					GtkTextView* handle = (GtkTextView*)children->data;
					if (handle) {
						gtk_text_view_set_left_margin(handle, inset.left);
						gtk_text_view_set_right_margin (handle, inset.right);
					}
				}
				void apply(EditView* view)
				{
					GtkContainer* container = (GtkContainer*)m_handle;
					GList *children = gtk_container_get_children (container);
					GtkWidget* handle = (GtkWidget*)children->data;
					if(handle)
					{
						gtk_widget_show_all(handle);
					}
					String text = view->getText();
					setText(view, text);
					setTextColor(view, view->getTextColor());
					setGravity(view, view->getGravity());
					setReadOnly(view, view->isReadOnly());
				}
				void installEventHandlers()
				{
					GtkContainer* container = (GtkContainer*)m_handle;
					GList *children = gtk_container_get_children (container);
					GtkTextView* handle = (GtkTextView*)children->data;
					if (handle) {
						GtkTextBuffer *buffer = gtk_text_view_get_buffer (handle);
						g_signal_connect(buffer, "changed", G_CALLBACK(onChange), m_handle);
						g_signal_connect(handle, "key-press-event", G_CALLBACK(eventCallback), m_handle);
					}
				}
				static void onChange(GtkTextBuffer *buffer, gpointer user_data)
				{
					Ref<GTK_ViewInstance> instance = Ref<GTK_ViewInstance>::from(UIPlatform::getViewInstance((GtkWidget*)user_data));
					if (instance.isNotNull()) {
						GtkTextIter start, end;
						gtk_text_buffer_get_start_iter (buffer, &start);
						gtk_text_buffer_get_end_iter (buffer, &end);
						String text = gtk_text_buffer_get_text(buffer, &start, &end, true) ;
						String textNew = text;
						Ref<TextArea> view = CastRef<TextArea>(instance->getView());
						view->dispatchChange(&textNew);
						if (text != textNew) {
							Ref<TextAreaInstance> textAreaInstance = CastRef<TextAreaInstance>(instance);
							textAreaInstance->setText(view, textNew);
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
		if (handle) {
			GTK_WIDGET_SET_FLAGS(handle, GTK_CAN_FOCUS);
			Ref<EditViewInstance> ret = GTK_ViewInstance::create<EditViewInstance>(this, parent, handle);
			if (ret.isNotNull()) {
				ret->apply(this);
				ret->installEventHandlers();
				return ret;
			} else {
				g_object_ref_sink(handle);
				g_object_unref(handle);
			}
		}
		return sl_null;
	}

	Ptr<IEditViewInstance> EditView::getEditViewInstance()
	{
		return CastRef<EditViewInstance>(getViewInstance());
	}

	Ref<ViewInstance> TextArea::createNativeWidget(ViewInstance* _parent)
	{
		GTK_ViewInstance* parent = static_cast<GTK_ViewInstance*>(_parent);
		GtkWidget* textView = gtk_text_view_new();
		GtkWidget *handle = gtk_scrolled_window_new (NULL, NULL);
		gtk_scrolled_window_set_policy ((GtkScrolledWindow*)handle,
											  GTK_POLICY_AUTOMATIC,
											  GTK_POLICY_AUTOMATIC);
		gtk_scrolled_window_set_shadow_type ((GtkScrolledWindow*)handle, GTK_SHADOW_ETCHED_IN);
		gtk_container_add ((GtkContainer*)handle, textView);

		gtk_widget_show_all (handle);
		if (handle) {
			//GTK_WIDGET_UNSET_FLAGS(handle, GTK_NO_WINDOW);
			GTK_WIDGET_SET_FLAGS(handle, GTK_CAN_FOCUS);
			Ref<TextAreaInstance> ret = GTK_ViewInstance::create<TextAreaInstance>(this, parent, handle);
			if (ret.isNotNull()) {
				ret->apply(this);
				ret->installEventHandlers();
				return ret;
			} else {
				g_object_ref_sink(handle);
				g_object_unref(handle);
			}
		}
		return sl_null;
	}
	
	Ptr<IEditViewInstance> TextArea::getEditViewInstance()
	{
		return CastRef<TextAreaInstance>(getViewInstance());
	}

}

#endif
