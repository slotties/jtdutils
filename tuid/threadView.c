#include <string.h>

#include "tuid.h"
#include "threadView.h"
#include "gtk_util.h"

#define TV_STACKVIEW "stacktrace"
#define TV_LOCKVIEW "locks"

enum {
	L_COL_OWNER = 0,
	L_COL_OBJ_CLASS,
	L_COL_OBJ_ID,
	L_COL_THREAD,
	L_NUM
};

GtkWidget* createThreadView() {
	GtkWidget* tabs = gtk_notebook_new();
		
	GtkWidget* stackView = gtk_text_view_new_with_buffer(NULL);
	gtk_text_view_set_editable(GTK_TEXT_VIEW(stackView), FALSE);
	gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(stackView), FALSE);
	gtk_notebook_append_page(GTK_NOTEBOOK(tabs), stackView, gtk_label_new("Stacktrace"));
	g_object_set_data(G_OBJECT(tabs), TV_STACKVIEW, stackView);
	
	GtkTreeStore* store = gtk_tree_store_new(
		L_NUM, 
		G_TYPE_STRING,
		G_TYPE_STRING,
		G_TYPE_STRING,
		G_TYPE_STRING
	);
	GtkTreeModel* model = GTK_TREE_MODEL(store);
	
	GtkWidget* listView = gtk_tree_view_new();
	GtkTreeView* listViewView = GTK_TREE_VIEW(listView);
	
	gtk_tree_view_set_rules_hint(listViewView, TRUE);
	
	gtk_tree_view_set_enable_search(listViewView, TRUE);
	gtk_tree_view_set_search_column(listViewView, L_COL_OBJ_CLASS);
	
	gtk_tree_view_set_headers_clickable(listViewView, TRUE);
		
	gtk_util_tree_view_insert_text_column(listViewView, L_COL_OWNER, "Owner?");
	gtk_util_tree_view_insert_text_column(listViewView, L_COL_OBJ_CLASS, "Type");
	gtk_util_tree_view_insert_text_column(listViewView, L_COL_OBJ_ID, "Address");
	gtk_util_tree_view_insert_text_column(listViewView, L_COL_THREAD, "Thread");
		
	GtkTreeSortable* sortable = GTK_TREE_SORTABLE(store);
	gtk_tree_sortable_set_sort_column_id(sortable, L_COL_OWNER, GTK_SORT_ASCENDING);
				
	gtk_tree_view_set_model(listViewView, model);
		
	g_object_unref(model);
	
	gtk_notebook_append_page(GTK_NOTEBOOK(tabs), listView, gtk_label_new("Locks"));
	g_object_set_data(G_OBJECT(tabs), TV_LOCKVIEW, listView);
	
		
	return tabs;
}

static inline void updateStackView(td_thread* thread, GtkWidget* stackView) {
	// TODO: create some nice text view
	GString* str = g_string_new(NULL);
	g_string_append(str, thread->name);
	g_string_append(str, "\n");
	
	char tmp_lineBuffer[32]; // buffer for converting line-number into a string
	
	list* stack = thread->stacktrace;
	while (stack) {
		td_line* line = stack->data;
		if (line->type == CODE || line->type == CONSTRUCTOR) {
			g_string_append(str, "\t");
			g_string_append(str, line->class);
			if (line->type == CONSTRUCTOR) {
				g_string_append(str, "<init>");
			}
			
			g_string_append(str, " (");
			if (td_isnative(line)) {
				g_string_append(str, "Native");
			} else {
				g_string_append(str, line->file);
				g_string_append(str, ":");
				snprintf(tmp_lineBuffer, 32, "%d", line->line);
				g_string_append(str, tmp_lineBuffer);
			}
			g_string_append(str, ")\n");
		}
		
		stack = stack->next;
	}
		
	gchar* data = g_string_free(str, FALSE);
	
	GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(stackView));
	gtk_text_buffer_set_text(buffer, data, strlen(data));	
}

static inline void updateLockView(td_dump* dump, td_thread* thread, GtkWidget* lockList) {		
	GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(lockList));
	GtkTreeStore* store = GTK_TREE_STORE(model);
	
	gtk_tree_store_clear(store);
	
	GtkTreeIter i;
	gtk_tree_model_get_iter_first(model, &i);
	
	list* stack = thread->stacktrace;
	while (stack) {
		td_line* line = stack->data;
		if (line->type == LOCK) {
			gtk_tree_store_append(store, &i, NULL);
			gtk_tree_store_set(store, &i, 
				L_COL_OWNER, line->isLockOwner ? "Yes" : "No", 
				L_COL_OBJ_CLASS, line->class,
				L_COL_OBJ_ID, line->lockObjId,
				L_COL_THREAD, thread->name,
				-1);
			
			GtkTreeIter child;
			
			for (list* l = dump->threads; l; l = l->next) {
				td_thread* next = l->data;
				
				for (list* s = next->stacktrace; s; s = s->next) {
					td_line* nextline = s->data;
					if (nextline->type == LOCK && line != nextline && strcmp(line->lockObjId, nextline->lockObjId) == 0) {
						gtk_tree_store_append(store, &child, &i);
						gtk_tree_store_set(store, &child, 
							L_COL_OWNER, nextline->isLockOwner ? "Yes" : "No", 
							L_COL_OBJ_CLASS, nextline->class,
							L_COL_OBJ_ID, nextline->lockObjId,
							L_COL_THREAD, next->name,
							-1
						);
					}
				}
			}			
		}
		
		stack = stack->next;
	}
}

void updateThreadView(td_dump* dump, td_thread* thread, GtkWidget* view) {
	GtkWidget* stackView = g_object_get_data(G_OBJECT(view), TV_STACKVIEW);
	updateStackView(thread, stackView);
	
	GtkWidget* lockView = g_object_get_data(G_OBJECT(view), TV_LOCKVIEW);
	updateLockView(dump, thread, lockView);
}
