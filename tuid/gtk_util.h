#ifndef GTK_UTIL_H 
#define GTK_UTIL_H

#include <gtk/gtk.h>

void gtk_util_tree_view_insert_text_column(GtkTreeView* treeView, guint colId, const char* name);
GtkWidget* gtk_util_notebook_create_closable_tab_label(GtkNotebook* tabs, guint pageNumber, const char* label);

#endif
