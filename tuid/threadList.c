#include "tuid.h"
#include "threadList.h"
#include "gtk_util.h"

enum {
	COL_NAME = 0,
	COL_PID,
	COL_STATE,
	NUM_COLS
};

GtkWidget* createThreadList(td_dump* dump) {
	GtkListStore* store = gtk_list_store_new(
		NUM_COLS, 
		G_TYPE_STRING, // name
		G_TYPE_STRING, // pid
		G_TYPE_STRING // state
	);
	GtkTreeModel* model = GTK_TREE_MODEL(store);
	
	GtkWidget* listView = gtk_tree_view_new();
	GtkTreeView* listViewView = GTK_TREE_VIEW(listView);
	
	gtk_tree_view_set_rules_hint(listViewView, TRUE);
	
	gtk_tree_view_set_enable_search(listViewView, TRUE);
	gtk_tree_view_set_search_column(listViewView, COL_NAME);
	
	gtk_tree_view_set_headers_clickable(listViewView, TRUE);
		
	gtk_util_tree_view_insert_text_column(listViewView, COL_NAME, "Name");
	gtk_util_tree_view_insert_text_column(listViewView, COL_PID, "PID");
	gtk_util_tree_view_insert_text_column(listViewView, COL_STATE, "State");
	
	// TODO: some more columns?
	
	GtkTreeSortable* sortable = GTK_TREE_SORTABLE(store);
	gtk_tree_sortable_set_sort_column_id(sortable, COL_NAME, GTK_SORT_ASCENDING);
				
	gtk_tree_view_set_model(listViewView, model);
		
	g_object_unref(model);
	
	return listView;
}

void updateThreadList(GtkWidget* threadList, td_dump* dump) {
	GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(threadList));
	GtkListStore* store = GTK_LIST_STORE(model);
	
	GtkTreeIter i;
	if (gtk_tree_model_get_iter_first(model, &i) == TRUE) {
		// thread list is already built, nothing to do
		return;
	}
	
	list* threads = dump->threads;
	while (threads) {
		td_thread* thread = threads->data;
		gtk_list_store_append(store, &i);
		gtk_list_store_set(store, &i, COL_NAME, thread->name, COL_PID, thread->native_id, -1);
		
		threads = threads->next;
	}
}

char* gtk_thread_list_get_pid(GtkTreeView* v) {
	GtkTreeSelection* sel = gtk_tree_view_get_selection(v);
	GtkTreeModel* model = gtk_tree_view_get_model(v);
	GList* l = gtk_tree_selection_get_selected_rows(sel, &model);
	GtkTreeIter i;
	gtk_tree_model_get_iter(model, &i, l->data);
	
	g_list_free(l);
	
	char* pid;
	gtk_tree_model_get(model, &i, COL_PID, &pid, -1);

	return pid;
}
