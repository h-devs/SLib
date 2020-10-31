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

#include "slib/core/definition.h"

#if defined(SLIB_UI_IS_GTK)

#include "slib/ui/list_control.h"

#include "view_gtk.h"


namespace slib
{
	namespace priv
	{
		namespace list_control
		{
			class ListControlInstance;
		}
	}
}

namespace slib
{

	namespace priv
	{
		namespace list_control
		{
			static gfloat TranslateAlignment(Alignment _align)
			{
				gfloat align = 0;
				if(_align == Alignment::Center){
					align = 0.5f;
				}else if(_align == Alignment::Right){
					align = 1.0f;
				}
				return align;
			}
			
			class ListControlHelper : public ListControl
			{
			public:
				int viewRows = 0;
				static sl_uint32 getColumnsCountFromListView(GtkTreeView* handle)
				{
					GList* _list = gtk_tree_view_get_columns(handle);
					return g_list_length(_list);
				}

				void applyColumnsCount(GtkTreeView* handle)
				{
					ObjectLocker lock(this);
					sl_uint32 nNew = (sl_uint32)(m_columns.getCount());
					sl_uint32 nOrig = getColumnsCountFromListView(handle);

					if (nOrig == nNew) {
						return;
					}
					if (nOrig > nNew) {
						for (sl_uint32 i = nOrig; i > nNew; i--) {
							GtkTreeViewColumn *column = gtk_tree_view_get_column(handle, i-1);
							gtk_tree_view_remove_column(handle, column);
						}
					} else {
						for (sl_uint32 i = nOrig; i < nNew; i++) {
							GtkCellRenderer   *renderer = gtk_cell_renderer_text_new ();
							GtkTreeViewColumn* column = gtk_tree_view_column_new_with_attributes("", renderer, "text", i, NULL);
							gtk_tree_view_append_column(handle, column);
						}
					}
				}

				void copyColumns(GtkTreeView* handle)
				{
					applyColumnsCount(handle);
					ListLocker<ListControlColumn> columns(m_columns);
					for (sl_size i = 0; i < columns.count; i++) {
						ListControlColumn& column = columns[i];
						String title = column.title;
						int width = (int)(column.width);
						if (width < 0) {
							width = 0;
						}
						char* tmp = title.getData();
						GtkTreeViewColumn *treecolumn = gtk_tree_view_get_column(handle, i);
						if(treecolumn){
							gtk_tree_view_column_set_title(treecolumn, title.getData());
							gtk_tree_view_column_set_fixed_width(treecolumn, width);
							gtk_tree_view_column_set_alignment(treecolumn, TranslateAlignment(column.align));
						}
					}
				}

				void applyRowsCount(GtkTreeView* handle)
				{
					GtkTreeModel *model = gtk_tree_view_get_model(handle);
					gtk_tree_view_set_model(handle, sl_null);
					viewRows = m_nRows;
					gtk_tree_view_set_model(handle, model);

				}
				void setupModel(GtkTreeView* view)
				{
					GtkTreeModel* model = (GtkTreeModel*)gtk_list_store_new(1, GTK_TYPE_STRING);
					g_object_set_data((GObject*)model, "view", this);
					g_object_set_data((GObject*)model, "rows", &viewRows);
					viewRows = m_nRows;

					GtkTreeModelIface *iface = GTK_TREE_MODEL_GET_IFACE(model);
					iface->get_n_columns = list_store_get_n_columns;
					iface->get_column_type = list_store_get_column_type;
					iface->get_value = list_store_get_value;
					iface->get_iter = list_store_get_iter;
					iface->get_path = list_store_get_path;
					iface->iter_next = list_store_iter_next;
					iface->iter_children = list_store_iter_children;
					iface->iter_has_child = list_store_iter_has_child;
					iface->iter_parent = list_store_iter_parent;
					iface->iter_n_children = list_store_iter_n_children;
					iface->iter_nth_child = list_store_iter_nth_child;

					gtk_tree_view_set_model(view, model);

				}
				static gboolean list_store_get_iter(GtkTreeModel *tree_model, GtkTreeIter  *iter, GtkTreePath  *path)
				{
					int rows = *(int*)(g_object_get_data((GObject*)tree_model, "rows"));
					if (rows) {
						iter->stamp = gtk_tree_path_get_indices (path)[0];;
						return true;
					} else {
						return false;
					}
				}
				static gboolean list_store_iter_next(GtkTreeModel *tree_model, GtkTreeIter  *iter)
				{
					int rows = *(int*)(g_object_get_data((GObject*)tree_model, "rows"));
					int index = iter->stamp;
					if (index < rows - 1 && index > -1) {
						iter->stamp = index + 1;
						return true;
					}else{
						return false;
					}
				}
				static gboolean list_store_iter_has_child(GtkTreeModel *tree_model, GtkTreeIter *iter)
				{
					return false;
				}
				static gboolean list_store_iter_children(GtkTreeModel *tree_model,GtkTreeIter  *iter, GtkTreeIter  *parent)
				{
					return false;
				}
				static gint list_store_iter_n_children(GtkTreeModel *tree_model,GtkTreeIter  *iter)
				{
					return *(int*)(g_object_get_data((GObject*)tree_model, "rows"));
				}
				static gboolean list_store_iter_nth_child(GtkTreeModel *tree_model, GtkTreeIter  *iter,GtkTreeIter  *parent, gint n)
				{
					return false;
				}
				static gboolean list_store_iter_parent(GtkTreeModel *tree_model,GtkTreeIter  *iter, GtkTreeIter  *child)
				{
					return false;
				}
				static GtkTreePath * list_store_get_path(GtkTreeModel *tree_model, GtkTreeIter  *iter)
				{
					int rows = *(int*)(g_object_get_data((GObject*)tree_model, "rows"));
					if (iter->stamp >= rows)
						return NULL;
					return gtk_tree_path_new_from_indices(iter->stamp, -1);
				}
				static void list_store_get_value(GtkTreeModel *tree_model, GtkTreeIter  *iter, gint column, GValue *value)
				{
					ListControlHelper *helper = (ListControlHelper*)(g_object_get_data((GObject*)tree_model, "view"));
					g_value_init(value, G_TYPE_STRING);
					g_value_set_string(value,  (gchar*)helper->getItemText(iter->stamp, column).getData());
				}
				static gint list_store_get_n_columns(GtkTreeModel *tree_model)
				{
					ListControlHelper *helper = (ListControlHelper*)(g_object_get_data((GObject*)tree_model, "view"));
					return helper->getColumnsCount();
				}
				static GType list_store_get_column_type(GtkTreeModel *tree_model, gint  index)
				{
				  return G_TYPE_STRING;
				}
			};
			class ListControlInstance : public GTK_ViewInstance, public IListControlInstance
			{
				SLIB_DECLARE_OBJECT
			public:
				GtkTreeView* m_handleTreeView;
			public:
				ListControlInstance()
				{
					m_handleTreeView = NULL;
				}
				GtkTreeView* getHandle()
				{
					return (GtkTreeView*)m_handleTreeView;
				}
				
				Ref<ListControlHelper> getHelper()
				{
					return CastRef<ListControlHelper>(getView());
				}

				void initialize(View* _view) override
				{

					GtkScrolledWindow* handleScrollWindow = (GtkScrolledWindow*)m_handle;
					ListControlHelper* view = (ListControlHelper*)_view;

					gtk_scrolled_window_set_policy(handleScrollWindow, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
					gtk_scrolled_window_set_shadow_type(handleScrollWindow, GTK_SHADOW_ETCHED_IN);

					GtkTreeView* handle = (GtkTreeView*)gtk_tree_view_new();
					if (handle) {
						m_handleTreeView = handle;

						GTK_WIDGET_SET_FLAGS(handle, GTK_CAN_FOCUS);
						gtk_container_add((GtkContainer*)handleScrollWindow, (GtkWidget*)handle);
						gtk_widget_show((GtkWidget*)handle);

						GtkTreeSelection *selection = gtk_tree_view_get_selection(handle);
						gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
						GtkAdjustment *hadjustment, *vadjustment;
						hadjustment = gtk_tree_view_get_hadjustment (handle);
						vadjustment = gtk_tree_view_get_vadjustment (handle);
						gtk_adjustment_set_step_increment (hadjustment, 10.0);
						gtk_adjustment_set_step_increment (vadjustment, 10.0);
						gtk_tree_view_set_hadjustment(handle, hadjustment);
						gtk_tree_view_set_vadjustment(handle, vadjustment);
						view->copyColumns(handle);
						view->setupModel(handle);
						refreshRowsCount(view);
						g_signal_connect((GtkWidget*)selection, "changed", G_CALLBACK(onChanged), view);
						g_signal_connect((GtkWidget*)handle, "button-press-event", G_CALLBACK(onButtonEvent), handleScrollWindow);
					}
					//view->applyFont(handle, view->getFont());
				}
				static gboolean onButtonEvent(GtkWidget*, GdkEvent* _event, gpointer user_data)
				{
					Ref<ListControlInstance> instance = Ref<ListControlInstance>::from(UIPlatform::getViewInstance((GtkWidget*)user_data));
					GdkEventButton* event = (GdkEventButton*)_event;
					GtkTreeView* handle = instance->getHandle();
					if (instance.isNotNull()) {
						Ref<ListControlHelper> helper = Ref<ListControlHelper>::from(UIPlatform::getView((GtkWidget*)user_data));
						if(!helper->viewRows)
							return false;
						int _x = event->x;
						int _y = event->y;
						GtkAdjustment *vadjustment = gtk_tree_view_get_vadjustment (handle);
						GtkAdjustment *hadjustment = gtk_tree_view_get_hadjustment (handle);
						_y += gtk_adjustment_get_value(vadjustment);
						_x += gtk_adjustment_get_value(hadjustment);
						if (_y < 0)
						  _y = 0;
						GList* _list = gtk_tree_view_get_columns(handle);
						int colcount =  g_list_length(_list);
						GtkTreePath * path = gtk_tree_path_new_first();
						GdkRectangle rect;
						int colindex = -1;
						for(int i=0;i<colcount; i++){
							GtkTreeViewColumn *column = gtk_tree_view_get_column(handle, i);
							gtk_tree_view_get_cell_area(handle, path, column, &rect);
							if(_x > rect.x && _x < rect.x + rect.width){
								colindex = i;
							}
						}
						int rowindex = (int)(_y / rect.height);
						int curSelectedRow = -1;
						instance->getSelectedRow(helper, curSelectedRow);
						if(curSelectedRow != -1 && rowindex != curSelectedRow){
							helper->dispatchSelectRow(rowindex);
						}
						if(event->button == 1){
							if(event->type == GDK_BUTTON_PRESS){
								helper->dispatchClickRow(rowindex, UIPointf(_x, _y));
							}else if(event->type == GDK_BUTTON_PRESS){
								helper->dispatchDoubleClickRow(rowindex, UIPointf(_x, _y));
							}
						}else if(event->button == 3){
							if(event->type == GDK_BUTTON_PRESS){
								helper->dispatchRightButtonClickRow(rowindex, UIPointf(_x, _y));
							}
						}
					}
					return sl_false;
				}

				void refreshColumnsCount(ListControl* view) override
				{
					GtkTreeView* handle = getHandle();
					if (handle) {
						static_cast<ListControlHelper*>(view)->applyColumnsCount(handle);
					}
				}
				
				void refreshRowsCount(ListControl* view) override
				{
					GtkTreeView* handle = getHandle();
					if (handle) {
						(static_cast<ListControlHelper*>(view))->applyRowsCount(handle);
					}
				}
				
				void setHeaderText(ListControl* view, sl_uint32 iCol, const String& text) override
				{
					GtkTreeView* handle = getHandle();
					if (handle) {
						GtkTreeViewColumn *tc = gtk_tree_view_get_column(handle, iCol);
						if(tc){
							gtk_tree_view_column_set_title(tc, text.getData());
						}
					}
				}
				
				void setColumnWidth(ListControl* view, sl_uint32 iCol, sl_ui_len width) override
				{
					GtkTreeView* handle = getHandle();
					if (handle) {
						GtkTreeViewColumn *tc = gtk_tree_view_get_column(handle, iCol);
						if (tc) {
							gtk_tree_view_column_set_fixed_width(tc, width);
						}
					}
				}
				
				void setHeaderAlignment(ListControl* view, sl_uint32 iCol, const Alignment& align) override
				{
					GtkTreeView* handle = getHandle();
					if (handle) {
						GtkTreeViewColumn *tc = gtk_tree_view_get_column(handle, iCol);
						if (tc) {
							gtk_tree_view_column_set_alignment(tc, TranslateAlignment(align));
						}
					}
				}
				
				void setColumnAlignment(ListControl* view, sl_uint32 iCol, const Alignment& align) override
				{
					GtkTreeView* handle = getHandle();
					if (handle) {
						GtkTreeViewColumn *tc = gtk_tree_view_get_column(handle, iCol);
						if (tc) {
							gtk_tree_view_column_set_alignment(tc, TranslateAlignment(align));
						}
					}
				}
				
				sl_bool getSelectedRow(ListControl* view, sl_int32& _out) override
				{
					GtkTreeView* handle = getHandle();
					if (handle) {
						GtkTreeSelection *selection = gtk_tree_view_get_selection(handle);
						GtkTreeIter iter;
						bool ret = gtk_tree_selection_get_selected(selection, NULL, &iter);
						if(ret){
							_out = iter.stamp;
							return sl_true;
						}
					}
					return sl_false;
				}
				
				void setFont(View* view, const Ref<Font>& font) override
				{
					GtkTreeView* handle = getHandle();
					if (handle) {
						//static_cast<ListControlHelper*>(view)->applyFont(handle, font);
					}
				}
				static void onChanged (GtkTreeSelection *treeselection, gpointer handle)
				{
					GtkTreeIter iter;
					gtk_tree_selection_get_selected(treeselection, NULL, &iter);
					ListControlHelper *helper = (ListControlHelper*)handle;
					helper->dispatchSelectRow(iter.stamp);
				}
			};
			
			SLIB_DEFINE_OBJECT(ListControlInstance, GTK_ViewInstance)
		}
	}

	using namespace priv::list_control;

	Ref<ViewInstance> ListControl::createNativeWidget(ViewInstance* parent)
	{
		GtkWidget* handle = gtk_scrolled_window_new(sl_null, sl_null);
		Ref<ListControlInstance> ret = GTK_ViewInstance::create<ListControlInstance>(this, parent, handle);
		if (ret.isNotNull()) {
			return ret;
		}
		return sl_null;
	}
	
	Ptr<IListControlInstance> ListControl::getListControlInstance()
	{
		return CastRef<ListControlInstance>(getViewInstance());
	}

}

#endif
