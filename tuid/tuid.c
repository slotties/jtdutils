#include <gtk/gtk.h>
#include <string.h>
#include <stdlib.h>

#include "td.h"
#include "common.h"
#include "td_format.h"

#include "tuid.h"
#include "threadList.h"
#include "threadView.h"
#include "gtk_util.h"

struct UiState {
	GtkWidget* threadView;
	GtkNotebook* dumpTabs;
	dump_parser parser;
	GtkWidget* wnd;
};
struct UiState UI_STATE;

static bool find_thread_by_id(td_thread* thread, char* pid) {
	return strcmp(thread->native_id, pid) == 0;
}

static void callUpdateThreadView(GtkTreeView* v, td_dump* dump) {
	char* pid = gtk_thread_list_get_pid(v);
	td_thread* thread = list_find(dump->threads, find_thread_by_id, pid);
	if (thread) {
		updateThreadView(dump, thread, UI_STATE.threadView);
	}
}

static void onWndClose(GtkWidget* w, gpointer d) {
	// TODO: free & close open stuff
	
	gtk_main_quit();
}

static void gtk_widget_destroy_callback(GtkWidget* w, gpointer d) {
	gtk_widget_destroy(GTK_WIDGET(d));
}

static void addDumpTab(td_dump* dump) {
	GtkWidget* threadList = createThreadList(dump);
	gtk_signal_connect(GTK_OBJECT(threadList), "cursor-changed", G_CALLBACK(callUpdateThreadView), dump);
	
	// put it into some scroll window
	GtkWidget* threadListScroll = gtk_scrolled_window_new(
		gtk_tree_view_get_hadjustment(GTK_TREE_VIEW(threadList)),
		gtk_tree_view_get_vadjustment(GTK_TREE_VIEW(threadList))
	);	
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(threadListScroll), 
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(threadListScroll), threadList);
	
	GtkNotebook* tabs = GTK_NOTEBOOK(UI_STATE.dumpTabs);
	GtkWidget* tabLabel = gtk_util_notebook_create_closable_tab_label(tabs, 
		gtk_notebook_get_n_pages(tabs),
		dump->id);
	
	gint n = gtk_notebook_append_page(tabs, threadListScroll, tabLabel);
	gtk_notebook_set_tab_reorderable(tabs, threadListScroll, TRUE);
	
	GtkWidget* page = gtk_notebook_get_nth_page(tabs, n);
	g_object_set_data(G_OBJECT(page), "thread-dump", dump);
	g_object_set_data(G_OBJECT(page), "thread-list", threadList);
	
	gtk_widget_show_all(GTK_WIDGET(UI_STATE.dumpTabs));
}

static void fillThreadList(GtkNotebook* n, GObject* p, guint num, gpointer d) {
	td_dump* dump = g_object_get_data(p, "thread-dump");
	if (!dump) {
		return;
	}
	
	GtkWidget* threadList = g_object_get_data(p, "thread-list");
	if (!threadList) {
		return;
	}
	updateThreadList(threadList, dump);
}

static void onFileOpen(GtkWidget* w, GtkFileSelection* selection) {
	const char* filename = gtk_file_selection_get_filename(selection);
	
	if (UI_STATE.parser) {
		td_free_parser(UI_STATE.parser);
	}
	
	UI_STATE.parser = td_init();
	FILE* file = fopen(filename, "r");
	if (!file) {
		// TODO: message
		// TODO: free parser
		free(file);
		
		return;
	}
	
	td_parse(file, UI_STATE.parser);
	free(file);
	
	list* dumps = td_getdumps(UI_STATE.parser);
	while (dumps) {
		addDumpTab(dumps->data);
		dumps = dumps->next;
	}
}

static void openFile(GtkWidget* w, gpointer d) {
	// FIXME: re-use the instance?
	GtkWidget* fileDlg = gtk_file_selection_new("Open file...");
	
	gtk_signal_connect(
		GTK_OBJECT(GTK_FILE_SELECTION(fileDlg)->ok_button),
		"clicked", G_CALLBACK(onFileOpen), fileDlg);
		
	gtk_signal_connect(
		GTK_OBJECT(GTK_FILE_SELECTION(fileDlg)->ok_button),
		"clicked", G_CALLBACK(gtk_widget_destroy_callback), fileDlg);
	gtk_signal_connect(
		GTK_OBJECT(GTK_FILE_SELECTION(fileDlg)->cancel_button),
		"clicked", G_CALLBACK(gtk_widget_destroy_callback), fileDlg);
	
	gtk_widget_show(fileDlg);
}

static inline GtkWidget* initMenu() {
	GtkItemFactoryEntry entries[] = {
		{ "/_File/_Open_", "<CTRL>-O", openFile, 0, "<Item>" },
		{ "/_File/_Quit", "<CTRL>-Q", onWndClose, 0, "<Item>" }
	};
	
	GtkItemFactory* factory = gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<main>", NULL);
	gtk_item_factory_create_items(factory, sizeof(entries) / sizeof(GtkItemFactoryEntry), entries, NULL);
	
	return gtk_item_factory_get_widget(factory, "<main>");
}

int main(int argc, char** argv) {
	gtk_init(&argc, &argv);
	gtk_rc_parse_string (
            "style \"tab-close-btn-style\"\n"
            "{\n"
            "  GtkWidget::focus-padding = 0\n"
            "  GtkWidget::focus-line-width = 0\n"
            "  xthickness = 0\n"
            "  ythickness = 0\n"
            "}\n"
            "widget \"*.tab-close-btn\" style \"tab-close-btn-style\""); 
	
	GtkWidget* wnd = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	UI_STATE.wnd = wnd;
	
	g_signal_connect(wnd, "destroy", G_CALLBACK(onWndClose), NULL);
	// FIXME: dimensions
	gtk_widget_set_size_request (GTK_WIDGET(wnd), 800, 600);
	
	GtkWidget* menubar = initMenu();
	
	GtkWidget* dumpTabs = gtk_notebook_new();
	gtk_notebook_set_scrollable(GTK_NOTEBOOK(dumpTabs), TRUE);
	gtk_signal_connect(GTK_OBJECT(dumpTabs), "switch-page", G_CALLBACK(fillThreadList), NULL);
	
	UI_STATE.dumpTabs = GTK_NOTEBOOK(dumpTabs);
	
	GtkWidget* threadView = createThreadView();
	UI_STATE.threadView = threadView;
	
	GtkWidget* mainPaned = gtk_vpaned_new();
	gtk_paned_add1(GTK_PANED(mainPaned), GTK_WIDGET(dumpTabs));
	gtk_paned_add2(GTK_PANED(mainPaned), threadView);
	
	GtkWidget* mainBox = gtk_vbox_new(FALSE, 0);
	
	gtk_box_pack_start(GTK_BOX(mainBox), menubar, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(mainBox), mainPaned, TRUE, TRUE, 0);
	
	gtk_container_add(GTK_CONTAINER(wnd), mainBox);
	
	
	// set size to 1/3 of the window size
	gint height;
	gtk_widget_get_size_request(GTK_WIDGET(wnd), NULL, &height); 
	gtk_paned_set_position(GTK_PANED(mainPaned), height / 3);
	
	gtk_widget_show_all(wnd);
	gtk_main();
	
	return 0;
}
