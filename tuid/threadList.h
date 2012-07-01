#include <gtk/gtk.h>

#include "td.h"

// FIXME: naming
GtkWidget* createThreadList(td_dump*);
void updateThreadList(GtkWidget* threadList, td_dump* dump);
char* gtk_thread_list_get_pid(GtkTreeView* v);

