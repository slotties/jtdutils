#include "gtk_util.h"

void gtk_util_tree_view_insert_text_column(GtkTreeView* treeView, guint colId, const char* name) {
	GtkCellRenderer* renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes(treeView, -1,
		name, renderer, "text", colId, NULL);
	
	GtkTreeViewColumn* col = gtk_tree_view_get_column(treeView, colId);
	gtk_tree_view_column_set_resizable(col, TRUE);
	gtk_tree_view_column_set_sort_column_id(col, colId);
}

static void close_tab_panel(GtkButton* btn, GtkNotebook* tabs) {
	guint pageNumber = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(btn), "orig-tab-pageNumber"));
	
	for (int i = 0; i < gtk_notebook_get_n_pages(tabs); i++) {
		GtkWidget* next = gtk_notebook_get_nth_page(tabs, i);
		GtkWidget* box = gtk_notebook_get_tab_label(tabs, next);
		guint origPageNumber = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(box), "orig-tab-pageNumber"));
		
		if (origPageNumber == pageNumber) {
			gtk_notebook_remove_page(tabs, i);
			return;
		}
	}
}

GtkWidget* gtk_util_notebook_create_closable_tab_label(GtkNotebook* tabs, guint pageNumber, const char* labelName) {
	GtkWidget* label = gtk_label_new(labelName);
	
	GtkWidget* box = gtk_hbox_new (FALSE, 3);
	gtk_box_pack_start (GTK_BOX(box), label, TRUE, TRUE, 0);
	
	GtkWidget* closeBtn = gtk_button_new();
	gtk_widget_set_name(closeBtn, "tab-close-btn");
	gtk_button_set_relief(GTK_BUTTON(closeBtn), GTK_RELIEF_NONE);
	
	gtk_box_pack_start(GTK_BOX(box), closeBtn, FALSE, FALSE, 0);
	GtkWidget* image = gtk_image_new_from_stock(GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU);
	gtk_button_set_image(GTK_BUTTON(closeBtn), image);
	
	gtk_widget_show_all(box);
		
	// remember original page number on the box so we can find the correct tab in close_tab_panel
	g_object_set_data(G_OBJECT(box), "orig-tab-pageNumber", GUINT_TO_POINTER(pageNumber));
	
	g_object_set_data(G_OBJECT(closeBtn), "orig-tab-pageNumber", GUINT_TO_POINTER(pageNumber));
	g_signal_connect(closeBtn, "clicked", G_CALLBACK(close_tab_panel), tabs);
	
	return box;	
}
