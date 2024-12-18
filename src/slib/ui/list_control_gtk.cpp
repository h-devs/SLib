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

#if defined(SLIB_UI_IS_GTK)

#include "slib/ui/list_control.h"

#include "slib/ui/core.h"

#include "view_gtk.h"

namespace slib
{

	namespace {

		static gfloat TranslateAlignment(Alignment _align)
		{
			gfloat align;
			if(_align == Alignment::Left){
				align = 0;
			} else if(_align == Alignment::Right){
				align = 1.0f;
			} else {
				align = 0.5f;
			}
			return align;
		}

		static int GetModelRows(GtkTreeModel* model)
		{
			return (int)((sl_size)(g_object_get_data((GObject*)model, "rows")));
		}

		static void SetModelRows(GtkTreeModel* model, int rows)
		{
			g_object_set_data((GObject*)model, "rows", (gpointer)(sl_size)rows);
		}

		class ListControlHelper : public ListControl
		{
		public:
			static sl_uint32 getColumnCountFromListView(GtkTreeView* handle)
			{
				GList* _list = gtk_tree_view_get_columns(handle);
				return g_list_length(_list);
			}

			void applyColumnCount(GtkTreeView* handle)
			{
				ObjectLocker lock(this);
				sl_uint32 nNew = (sl_uint32)(m_columns.getCount());
				sl_uint32 nOrig = getColumnCountFromListView(handle);

				if (nOrig == nNew) {
					return;
				}
				if (nOrig > nNew) {
					for (sl_uint32 i = nOrig; i > nNew; i--) {
						GtkTreeViewColumn *column = gtk_tree_view_get_column(handle, i - 1);
						gtk_tree_view_remove_column(handle, column);
					}
				} else {
					for (sl_uint32 i = nOrig; i < nNew; i++) {
						GtkCellRenderer* renderer = gtk_cell_renderer_text_new ();
						GtkTreeViewColumn* column = gtk_tree_view_column_new_with_attributes("", renderer, "text", i, sl_null);
						gtk_tree_view_append_column(handle, column);
					}
				}
			}

			void copyColumns(GtkTreeView* handle)
			{
				applyColumnCount(handle);
				ListLocker<Column> columns(m_columns);
				for (sl_size i = 0; i < columns.count; i++) {
					Column& column = columns[i];
					int width = (int)(column.width);
					if (width < 0) {
						width = 0;
					}
					GtkTreeViewColumn *treecolumn = gtk_tree_view_get_column(handle, i);
					if(treecolumn){
						StringCstr title = column.title;
						gtk_tree_view_column_set_title(treecolumn, title.getData());
						gtk_tree_view_column_set_fixed_width(treecolumn, width);
						gtk_tree_view_column_set_alignment(treecolumn, TranslateAlignment(column.align));
					}
				}
			}

			void applyRowCount(GtkTreeView* handle)
			{
				GtkTreeModel *model = gtk_tree_view_get_model(handle);
				gtk_tree_view_set_model(handle, sl_null);
				SetModelRows(model, getRowCount());
				gtk_tree_view_set_model(handle, model);

			}

			void setupModel(GtkTreeView* view);

			using ListControl::_onSelectRow_NW;
			using ListControl::_onClickRow_NW;
			using ListControl::_onRightButtonClickRow_NW;
			using ListControl::_onDoubleClickRow_NW;
		};

		static ListControlHelper* GetModelView(GtkTreeModel* model)
		{
			return (ListControlHelper*)((sl_size)(g_object_get_data((GObject*)model, "view")));
		}

		static void SetModelView(GtkTreeModel* model, ListControl* view)
		{
			g_object_set_data((GObject*)model, "view", view);
		}

		struct SlibListControlModel
		{
			GObject parent;
		};

		struct SlibListControlModelClass
		{
			GObjectClass parent_class;
		};

		static void slib_list_control_model_class_init(SlibListControlModelClass* cls)
		{
		}

		static void slib_list_control_model_init(SlibListControlModel* obj)
		{
		}

		static gboolean list_control_model_get_iter(GtkTreeModel* model, GtkTreeIter* iter, GtkTreePath* path)
		{
			int rows = GetModelRows(model);
			if (rows) {
				iter->stamp = gtk_tree_path_get_indices(path)[0];;
				return 1;
			} else {
				return 0;
			}
		}

		static gboolean list_control_model_iter_next(GtkTreeModel* model, GtkTreeIter* iter)
		{
			int rows = GetModelRows(model);
			int index = iter->stamp;
			if (index < rows - 1 && index >= 0) {
				iter->stamp = index + 1;
				return 1;
			}else{
				return 0;
			}
		}

		static gboolean list_control_model_iter_has_child(GtkTreeModel* model, GtkTreeIter* iter)
		{
			return 0;
		}

		static gboolean list_control_model_iter_children(GtkTreeModel* model, GtkTreeIter* iter, GtkTreeIter* parent)
		{
			return 0;
		}

		static gint list_control_model_iter_n_children(GtkTreeModel* model, GtkTreeIter* iter)
		{
			return GetModelRows(model);
		}

		static gboolean list_control_model_iter_nth_child(GtkTreeModel* model, GtkTreeIter* iter, GtkTreeIter* parent, gint n)
		{
			return 0;
		}

		static gboolean list_control_model_iter_parent(GtkTreeModel* model, GtkTreeIter* iter, GtkTreeIter* child)
		{
			return 0;
		}

		static GtkTreePath* list_control_model_get_path(GtkTreeModel* model, GtkTreeIter* iter)
		{
			int rows = GetModelRows(model);
			if (iter->stamp >= rows) {
				return sl_null;
			}
			return gtk_tree_path_new_from_indices(iter->stamp, -1);
		}

		static void list_control_model_get_value(GtkTreeModel* model, GtkTreeIter* iter, gint column, GValue* value)
		{
			ListControlHelper* helper = GetModelView(model);
			g_value_init(value, G_TYPE_STRING);
			StringCstr str(helper->getItemText(iter->stamp, column));
			g_value_set_string(value, (gchar*)(str.getData()));
		}

		static gint list_control_model_get_n_columns(GtkTreeModel* model)
		{
			ListControlHelper* helper = GetModelView(model);
			return helper->getColumnCount();
		}

		static GType list_control_model_get_column_type(GtkTreeModel* model, gint  index)
		{
			return G_TYPE_STRING;
		}

		static GtkTreeModelFlags list_control_model_get_flags (GtkTreeModel *tree_model)
		{
			return (GtkTreeModelFlags)(GTK_TREE_MODEL_ITERS_PERSIST | GTK_TREE_MODEL_LIST_ONLY);
		}

		static void slib_list_control_model_tree_model_init(GtkTreeModelIface *iface)
		{
			iface->get_flags = list_control_model_get_flags;
			iface->get_n_columns = list_control_model_get_n_columns;
			iface->get_column_type = list_control_model_get_column_type;
			iface->get_value = list_control_model_get_value;
			iface->get_iter = list_control_model_get_iter;
			iface->get_path = list_control_model_get_path;
			iface->iter_next = list_control_model_iter_next;
			iface->iter_children = list_control_model_iter_children;
			iface->iter_has_child = list_control_model_iter_has_child;
			iface->iter_parent = list_control_model_iter_parent;
			iface->iter_n_children = list_control_model_iter_n_children;
			iface->iter_nth_child = list_control_model_iter_nth_child;
		}

		G_DEFINE_TYPE_WITH_CODE(
				SlibListControlModel,
				slib_list_control_model,
				G_TYPE_OBJECT,
				G_IMPLEMENT_INTERFACE(GTK_TYPE_TREE_MODEL, slib_list_control_model_tree_model_init))

		GtkTreeModel* list_control_model_new ()
		{
			SlibListControlModel *result;
			result = (SlibListControlModel *)g_object_new(slib_list_control_model_get_type(), NULL);
			return GTK_TREE_MODEL(result);
		}

		void ListControlHelper::setupModel(GtkTreeView* view)
		{
			GtkTreeModel* model = list_control_model_new();
			SetModelView(model, this);
			SetModelRows(model, getRowCount());
			gtk_tree_view_set_model(view, model);
		}

		class ListControlInstance : public PlatformViewInstance, public IListControlInstance
		{
			SLIB_DECLARE_OBJECT

		public:
			GtkTreeView* m_handleTreeView;

		public:
			ListControlInstance()
			{
				m_handleTreeView = sl_null;
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

					gtk_widget_set_can_focus((GtkWidget*)handle, 1);
					gtk_container_add((GtkContainer*)handleScrollWindow, (GtkWidget*)handle);
					gtk_widget_show((GtkWidget*)handle);

					GtkTreeSelection* selection = gtk_tree_view_get_selection(handle);
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
					refreshRowCount(view);

					g_signal_connect((GtkWidget*)selection, "changed", G_CALLBACK(_callback_selection_changed), handleScrollWindow);
					g_signal_connect((GtkWidget*)handle, "button-press-event", G_CALLBACK(_callback_button_press_event), handleScrollWindow);
				}
			}

			void refreshColumnCount(ListControl* view) override
			{
				GtkTreeView* handle = getHandle();
				if (handle) {
					(static_cast<ListControlHelper*>(view))->applyColumnCount(handle);
				}
			}

			void refreshRowCount(ListControl* view) override
			{
				GtkTreeView* handle = getHandle();
				if (handle) {
					(static_cast<ListControlHelper*>(view))->applyRowCount(handle);
				}
			}

			void setHeaderText(ListControl* view, sl_uint32 iCol, const String& _text) override
			{
				GtkTreeView* handle = getHandle();
				if (handle) {
					GtkTreeViewColumn* column = gtk_tree_view_get_column(handle, iCol);
					if (column) {
						StringCstr text(_text);
						gtk_tree_view_column_set_title(column, text.getData());
					}
				}
			}

			void setColumnWidth(ListControl* view, sl_uint32 iCol, sl_ui_len width) override
			{
				GtkTreeView* handle = getHandle();
				if (handle) {
					GtkTreeViewColumn* column = gtk_tree_view_get_column(handle, iCol);
					if (column) {
						gtk_tree_view_column_set_fixed_width(column, width);
					}
				}
			}

			void setHeaderAlignment(ListControl* view, sl_uint32 iCol, const Alignment& align) override
			{
				GtkTreeView* handle = getHandle();
				if (handle) {
					GtkTreeViewColumn* column = gtk_tree_view_get_column(handle, iCol);
					if (column) {
						gtk_tree_view_column_set_alignment(column, TranslateAlignment(align));
					}
				}
			}

			void setColumnAlignment(ListControl* view, sl_uint32 iCol, const Alignment& align) override
			{
				GtkTreeView* handle = getHandle();
				if (handle) {
					GtkTreeViewColumn* column = gtk_tree_view_get_column(handle, iCol);
					if (column) {
						gtk_tree_view_column_set_alignment(column, TranslateAlignment(align));
					}
				}
			}

			sl_bool getSelectedRow(ListControl* view, sl_int32& _out) override
			{
				GtkTreeView* handle = getHandle();
				if (handle) {
					GtkTreeSelection* selection = gtk_tree_view_get_selection(handle);
					GtkTreeIter iter;
					gboolean bRet = gtk_tree_selection_get_selected(selection, sl_null, &iter);
					if (bRet){
						_out = iter.stamp;
						return sl_true;
					}
				}
				return sl_false;
			}

			static void _callback_selection_changed(GtkTreeSelection* selection, gpointer user_data)
			{
				Ref<ListControlInstance> instance = CastRef<ListControlInstance>(UIPlatform::getViewInstance((GtkWidget*)user_data));
				if (instance.isNotNull()) {
					Ref<ListControlHelper> helper = CastRef<ListControlHelper>(instance->getView());
					if (helper.isNotNull()) {
						GtkTreeIter iter;
						gtk_tree_selection_get_selected(selection, sl_null, &iter);
						helper->_onSelectRow_NW(instance.get(), iter.stamp);
					}
				}
			}

			static gboolean _callback_button_press_event(GtkWidget*, GdkEvent* _event, gpointer user_data)
			{
				Ref<ListControlInstance> instance = CastRef<ListControlInstance>(UIPlatform::getViewInstance((GtkWidget*)user_data));
				if (instance.isNotNull()) {
					return 0;
				}
				Ref<ListControlHelper> helper = CastRef<ListControlHelper>(instance->getView());
				if (helper.isNull()) {
					return 0;
				}
				GtkTreeView* handle = instance->getHandle();
				if (!handle) {
					return 0;
				}
				GtkTreeModel* model = gtk_tree_view_get_model(handle);
				if (!model) {
					return 0;
				}
				int nRows = GetModelRows(model);
				if (!nRows) {
					return 0;
				}
				GList* columns = gtk_tree_view_get_columns(handle);
				if (!columns) {
					return 0;
				}
				int nColumns =  g_list_length(columns);
				if (!nColumns) {
					return 0;
				}
				GtkTreePath * path = gtk_tree_path_new_first();
				if (!path) {
					return 0;
				}

				GdkEventButton* event = (GdkEventButton*)_event;

				gint x = event->x;
				gint y = event->y;

				GtkAdjustment* adjust_h = gtk_tree_view_get_hadjustment(handle);
				x += gtk_adjustment_get_value(adjust_h);
				GtkAdjustment* adjust_v = gtk_tree_view_get_vadjustment(handle);
				y += gtk_adjustment_get_value(adjust_v);
				if (y < 0) {
					y = 0;
				}

				int iCol = -1;
				int heightRow = 1;
				{
					for(int i = 0; i < nColumns; i++){
						GtkTreeViewColumn* column = gtk_tree_view_get_column(handle, i);
						GdkRectangle rect;
						gtk_tree_view_get_cell_area(handle, path, column, &rect);
						if(x > rect.x && x < rect.x + rect.width){
							iCol = i;
						}
						heightRow = rect.height;
					}
					if (heightRow < 1) {
						heightRow = 1;
					}
				}

				int iRow = y / heightRow;
				if (iRow >= nRows) {
					iRow = -1;
				}

				sl_int32 iSel = -1;
				instance->getSelectedRow(helper, iSel);
				if(iRow != iSel){
					helper->_onSelectRow_NW(instance.get(), iRow);
				}

				UIPoint pt = helper->convertCoordinateFromScreen(UI::getCursorPos());

				if (event->button == 1) {
					if (event->type == GDK_BUTTON_PRESS) {
						helper->_onClickRow_NW(iRow, pt);
					} else if (event->type == GDK_2BUTTON_PRESS) {
						helper->_onDoubleClickRow_NW(iRow, pt);
					}
				} else if (event->button == 3) {
					if (event->type == GDK_BUTTON_PRESS) {
						helper->_onRightButtonClickRow_NW(iRow, pt);
					}
				}

				return 0;
			}

		};

		SLIB_DEFINE_OBJECT(ListControlInstance, PlatformViewInstance)

	}

	Ref<ViewInstance> ListControl::createNativeWidget(ViewInstance* parent)
	{
		GtkWidget* handle = gtk_scrolled_window_new(sl_null, sl_null);
		return PlatformViewInstance::create<ListControlInstance>(this, parent, handle);
	}

	Ptr<IListControlInstance> ListControl::getListControlInstance()
	{
		return CastRef<ListControlInstance>(getViewInstance());
	}

}

#endif
