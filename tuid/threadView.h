#include <gtk/gtk.h>

#include "tuid.h"
#include "td.h"

GtkWidget* createThreadView();
void updateThreadView(td_dump* dump, td_thread* thread, GtkWidget* view);
