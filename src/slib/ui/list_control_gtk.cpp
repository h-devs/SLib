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
			const char* celldata(int row, int col){
				if(col==0 && row==0)
					return "111";
				else if(col==0 && row==1)
					return "222";
				else if(col==1 && row==0)
					return "333";
				else if(col==1 && row==1)
					return "444";
				else
					return "other";
			}
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
					if(nOrig != nNew){
						GType *type = (GType*)g_malloc(sizeof(GType)*nNew);
						for(int i=0; i<nNew; i++){
							type[i] = G_TYPE_STRING;
						}
						GtkTreeModel* newmodel = (GtkTreeModel*)gtk_tree_store_newv(nNew, type);
						g_free(type);
						GtkTreeModelIface *iface = GTK_TREE_MODEL_GET_IFACE(newmodel);
						iface->get_value = tree_store_get_value;
						gtk_tree_view_set_model(handle, newmodel);

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
					sl_uint32 nNew = m_nRows;
					sl_uint32 nOrig = gtk_tree_model_iter_n_children(model, NULL);
					int columncount = gtk_tree_model_get_n_columns(model);
					GtkTreeIter iter;
					if (nOrig == nNew) {
						return;
					}
					if (nOrig > nNew) {
						GtkTreePath* path = gtk_tree_path_new_from_indices(0);
						for (sl_uint32 i = nOrig; i > nNew; i--) {
							if(path){
								gtk_tree_model_get_iter(model, &iter, path);
								gtk_tree_store_remove((GtkTreeStore*)model, &iter);
							}
						}
						gtk_tree_path_free(path);
					} else {
						for (sl_uint32 i = nOrig; i < nNew; i++) {
							gtk_tree_store_append((GtkTreeStore*)model, &iter, NULL);
							GNode *node = (GNode*)(iter.user_data);
							node->data = GINT_TO_POINTER(i);
						}
					}
				}
				static void tree_store_get_value (GtkTreeModel *tree_model, GtkTreeIter  *iter, gint column, GValue *value)
				{
					GtkTreeStore *store = (GtkTreeStore *)(tree_model);
					int row = 0;
					GNode *node = (GNode*)(iter->user_data);
					if(node){
						row = GPOINTER_TO_INT(node->data);
					}
					g_value_init(value, G_TYPE_STRING);
					//g_value_set_string(value,  (gchar*)getItemText(row, column));
					g_value_set_string(value,  (gchar*)celldata(row, column));
				}

			};
			class ListControlInstance : public GTK_ViewInstance, public IListControlInstance
			{
				SLIB_DECLARE_OBJECT
				
			public:
				GtkTreeView* getHandle()
				{
					return (GtkTreeView*)m_handle;
				}
				
				Ref<ListControlHelper> getHelper()
				{
					return CastRef<ListControlHelper>(getView());
				}

				void initialize(View* _view) override
				{
					GtkTreeView* handle = getHandle();
					ListControlHelper* view = (ListControlHelper*)_view;
					GtkTreeSelection *selection = gtk_tree_view_get_selection(handle);
					gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
					gtk_tree_view_set_hadjustment(handle, NULL);
					gtk_tree_view_set_vadjustment(handle, NULL);
/*
					[handle setHasVerticalScroller:TRUE];
					[handle setHasHorizontalScroller:TRUE];
					[handle setBorderType:NSBezelBorder];
					handle->table->m_viewInstance = this;*/
					view->copyColumns(handle);
					refreshRowsCount(view);
					//view->applyFont(handle, view->getFont());
				}
				
				void refreshColumnsCount(ListControl* view) override
				{
					GtkTreeView* handle = getHandle();
					if (handle) {
						static_cast<ListControlHelper*>(view)->applyColumnsCount(handle);
						//[handle->table reloadData];
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
						gtk_tree_selection_get_selected(selection, NULL, &iter);
						if(!g_sequence_iter_is_end((GSequenceIter*)iter.user_data)){
							_out = g_sequence_iter_get_position((GSequenceIter*)iter.user_data);
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

				/*
				void onMouseDown(GtkTreeView_TableView* tv, NSEvent* ev)
				{

					Ref<ListControlHelper> helper = getHelper();
					if (helper.isNotNull()) {
						NSInteger indexRowBefore = [tv selectedRow];
						NSPoint ptWindow = [ev locationInWindow];
						NSPoint ptView = [tv convertPoint:ptWindow fromView:nil];
						NSInteger indexRow = [tv rowAtPoint:ptView];
						if (indexRow >= 0) {
							if (indexRow == indexRowBefore) {
								// don't call event callback when it is new selection because it is already called by default
								helper->dispatchSelectRow((sl_uint32)(indexRow));
							}
							sl_ui_posf x = (sl_ui_posf)(ptView.x);
							sl_ui_posf y = (sl_ui_posf)(ptView.y);
							NSInteger clicks = [ev clickCount];
							if (clicks == 1) {
								helper->dispatchClickRow((sl_uint32)(indexRow), UIPointf(x, y));
							} else if (clicks == 2) {
								helper->dispatchDoubleClickRow((sl_uint32)(indexRow), UIPointf(x, y));
							}
						}
					}
				}
				
				void onRightMouseDown(GtkTreeView_TableView* tv, NSEvent* ev)
				{
					Ref<ListControlHelper> helper = getHelper();
					if (helper.isNotNull()) {
						NSPoint ptWindow = [ev locationInWindow];
						NSPoint ptView = [tv convertPoint:ptWindow fromView:nil];
						NSInteger indexRow = [tv rowAtPoint:ptView];
						if (indexRow >= 0) {
							sl_ui_posf x = (sl_ui_posf)(ptView.x);
							sl_ui_posf y = (sl_ui_posf)(ptView.y);
							helper->dispatchRightButtonClickRow((sl_uint32)(indexRow), UIPointf(x, y));
						}
					}
				}*/
				
			};
			
			SLIB_DEFINE_OBJECT(ListControlInstance, GTK_ViewInstance)
		}
	}

	using namespace priv::list_control;

	Ref<ViewInstance> ListControl::createNativeWidget(ViewInstance* parent)
	{
		GtkWidget* handle = gtk_tree_view_new();
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
