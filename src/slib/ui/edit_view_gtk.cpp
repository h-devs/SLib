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

#include "slib/ui/definition.h"

#if defined(SLIB_UI_IS_GTK)

#include "slib/ui/edit_view.h"

#include "slib/core/log.h"

#include "view_gtk.h"

namespace slib
{
	namespace {

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

				gtk_widget_set_can_focus((GtkWidget*)handle, 1);
				StringCstr text = view->getText();
				if (text.isNotEmpty()) {
					gtk_entry_set_text(handle, text.getData());
				}
				if (view->isPassword()) {
					gtk_entry_set_visibility(handle, 0);
				}
				setTextColor(view, view->getTextColor());
				if (view->getWidthMode() == SizeMode::Fixed) {
					gtk_entry_set_width_chars(handle, 0);
				}
				setGravity(view, view->getGravity());
				if (view->isReadOnly()) {
					setReadOnly(view, sl_true);
				}
				if (!(view->isBorder())) {
					setBorder(view, sl_false);
				}
				Color backColor = view->getBackgroundColor();
				if (backColor.isNotZero()) {
					setBackgroundColor(view, backColor);
				}
				sl_reg indexSelection = view->getRawSelectionStart();
				if (indexSelection >= 0) {
					setSelection(view, indexSelection, view->getRawSelectionEnd());
				}

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

			void setText(EditView* view, const String& _text) override
			{
				GtkEntry* handle = (GtkEntry*)m_handle;
				if (handle) {
					StringCstr text(_text);
					gtk_entry_set_text(handle, text.getData());
				}
			}

			void setGravity(EditView* view, const Alignment& gravity) override
			{
				GtkEntry* handle = (GtkEntry*)m_handle;
				if (handle) {
					float alignment;
					Alignment align = gravity & Alignment::HorizontalMask;
					if (align == Alignment::Left) {
						alignment = 0;
					} else if (align == Alignment::Right) {
						alignment = 1;
					} else {
						alignment = 0.5f;
					}
					gtk_entry_set_alignment(handle, alignment);
				}
			}

			void setTextColor(EditView* view, const Color& color) override
			{
				GtkWidget* handle = m_handle;
				if (handle) {
					if (UIPlatform::isSupportedGtk(3)) {
						String strColor = String::concat("rgb(", String::fromUint32(color.r), ",", String::fromUint32(color.g), ",", String::fromUint32(color.b));
						String style = String::concat("* { color: ", strColor, "); caret-color: ", strColor, "); }");
						UIPlatform::setWidgetGtk3Style((GtkWidget*)handle, "text-color-provider", style);
					} else {
						GdkColor gdkColor;
						UIPlatform::getGdkColor(color, &gdkColor);
						gtk_widget_modify_text(handle, GTK_STATE_NORMAL, &gdkColor);
					}
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
					GValue value = G_VALUE_INIT;
					g_value_init(&value, G_TYPE_BOOLEAN);
					g_value_set_boolean(&value, !flag);
					g_object_set_property((GObject*)handle, "editable", &value);
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

			void setSelection(EditView* view, sl_reg start, sl_reg end) override
			{
				GtkEntry* handle = (GtkEntry*)m_handle;
				if (handle) {
					GtkEditable* editable = GTK_EDITABLE(handle);
					if (editable) {
						gtk_editable_select_region(editable, (gint)start, (gint)end);
					}
				}
			}

			void setBorder(View* view, sl_bool flag) override
			{
				GtkEntry* handle = (GtkEntry*)m_handle;
				if (handle) {
					gtk_entry_set_has_frame(handle, flag ? 1 : 0);
					if (UIPlatform::isSupportedGtk(3)) {
						StringView style;
						if (!flag) {
							style = StringView::literal("* { border: none; box-shadow: none; }");
						}
						UIPlatform::setWidgetGtk3Style((GtkWidget*)handle, "outline-color-provider", style);
					}
				}
			}

			void setBackgroundColor(View* view, const Color& color) override
			{
				GtkWidget* handle = m_handle;
				if (handle) {
					UIPlatform::setWidgetBackgroundColor(handle, color);
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
					if (view->isChangeEventEnabled()) {
						String text = gtk_entry_get_text(handle);
						String textNew = text;
						view->dispatchChange(textNew);
						if (text != textNew) {
							StringCstr _text(textNew);
							gtk_entry_set_text(handle, _text.getData());
						}
					} else {
						view->invalidateText();
					}
					view->dispatchPostChange();
				}
			}

		};

		SLIB_DEFINE_OBJECT(EditViewInstance, GTK_ViewInstance)

	}

	Ref<ViewInstance> EditView::createNativeWidget(ViewInstance* _parent)
	{
		GTK_ViewInstance* parent = static_cast<GTK_ViewInstance*>(_parent);
		if (getMultiLine() == MultiLineMode::Single) {
			GtkWidget* handle = gtk_entry_new();
			return GTK_ViewInstance::create<EditViewInstance>(this, parent, handle);
		} else {
			GtkScrolledWindow* handle = (GtkScrolledWindow*)(gtk_scrolled_window_new(sl_null, sl_null));
			return GTK_ViewInstance::create<TextAreaInstance>(this, parent, (GtkWidget*)handle);
		}
	}

	Ptr<IEditViewInstance> EditView::getEditViewInstance()
	{
		Ref<ViewInstance> instance = getViewInstance();
		if (IsInstanceOf<TextAreaInstance>(instance)) {
			return CastRef<TextAreaInstance>(Move(instance));
		} else {
			return CastRef<EditViewInstance>(Move(instance));
		}
	}

	namespace {

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

					gtk_widget_set_can_focus((GtkWidget*)handleText, 1);
					gtk_container_add((GtkContainer*)handle, (GtkWidget*)handleText);
					gtk_widget_show((GtkWidget*)handleText);

					String text = view->getText();
					if (text.isNotEmpty()) {
						setText(view, text);
					}
					setTextColor(view, view->getTextColor());
					setGravity(view, view->getGravity());
					if (view->isReadOnly()) {
						setReadOnly(view, sl_true);
					}
					sl_reg indexSelection = view->getRawSelectionStart();
					if (indexSelection >= 0) {
						setSelection(view, indexSelection, view->getRawSelectionEnd());
					}
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

			sl_bool appendText(EditView* view, const StringParam& _text) override
			{
				GtkTextView* handle = m_handleTextView;
				if (handle) {
					GtkTextBuffer* buffer = gtk_text_view_get_buffer(handle);
					GtkTextIter iter;
					gtk_text_buffer_get_end_iter(buffer, &iter);
					StringData text(_text);
					gtk_text_buffer_insert(buffer, &iter, text.getData(), text.getLength());
					return sl_true;
				}
				return sl_false;
			}

			void setGravity(EditView* view, const Alignment& gravity) override
			{
				GtkTextView* handle = m_handleTextView;
				if (handle) {
					GtkJustification alignment;
					Alignment align = gravity & Alignment::HorizontalMask;
					if (align == Alignment::Left) {
						alignment = GTK_JUSTIFY_LEFT;
					} else if (align == Alignment::Right) {
						alignment = GTK_JUSTIFY_RIGHT;
					} else {
						alignment = GTK_JUSTIFY_CENTER;
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

			void setSelection(EditView* view, sl_reg start, sl_reg end) override
			{
				GtkTextView* handle = m_handleTextView;
				if (handle) {
					GtkTextBuffer* buffer = gtk_text_view_get_buffer(handle);
					GtkTextIter iterStart, iterEnd;
					if (start < 0) {
						gtk_text_buffer_get_end_iter(buffer, &iterStart);
						gtk_text_buffer_get_end_iter(buffer, &iterEnd);
					} else {
						gtk_text_buffer_get_iter_at_offset(buffer, &iterStart, (gint)start);
						if (end < 0) {
							gtk_text_buffer_get_end_iter(buffer, &iterEnd);
						} else {
							gtk_text_buffer_get_iter_at_offset(buffer, &iterEnd, (gint)end);
						}
					}
					gtk_text_buffer_select_range(buffer, &iterStart, &iterEnd);
				}
			}

			void setPadding(View* view, const UIEdgeInsets& inset) override
			{
				GtkTextView* handle = m_handleTextView;
				if (handle) {
					gtk_text_view_set_left_margin(handle, inset.left);
					gtk_text_view_set_right_margin(handle, inset.right);
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
					if (view->isChangeEventEnabled()) {
						String text = _getText(buffer);
						String textNew = text;
						view->dispatchChange(textNew);
						if (text != textNew) {
							gtk_text_buffer_set_text(buffer, textNew.getData(), textNew.getLength());
						}
					} else {
						view->invalidateText();
					}
					view->dispatchPostChange();
				}
			}

		};

		SLIB_DEFINE_OBJECT(TextAreaInstance, GTK_ViewInstance)

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
