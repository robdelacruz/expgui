#ifndef UI_H
#define UI_H

#include <gtk/gtk.h>

void set_screen_css(char *cssfile);
GtkWidget *create_scroll_window(GtkWidget *child);
GtkWidget *create_frame(gchar *label, GtkWidget *child, int xpadding, int ypadding);
GtkWidget *create_label_with_margin(gchar *s, int start, int end, int top, int bottom);
void add_accel(GtkWidget *w, GtkAccelGroup *a, guint key, GdkModifierType mods);

#endif
