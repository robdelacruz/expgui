#ifndef EXPENSEUI_H
#define EXPENSEUI_H

#include <gtk/gtk.h>
#include "expense.h"

typedef struct {
    GtkDialog *dlg;
    GtkEntry *txt_date;
    GtkEntry *txt_desc;
    GtkEntry *txt_amt;
    GtkEntry *txt_cat;
} ExpenseEditDialog;

void set_screen_css(char *cssfile);
GtkWidget *create_scroll_window(GtkWidget *child);
GtkWidget *create_frame(gchar *label, GtkWidget *child, int xpadding, int ypadding);
GtkWidget *create_label_with_margin(gchar *s, int start, int end, int top, int bottom);
void add_accel(GtkWidget *w, GtkAccelGroup *a, guint key, GdkModifierType mods);

GtkWidget *create_expenses_section(ExpContext *ctx);
void refresh_expenses_treeview(GtkTreeView *tv, Expense *xps[], size_t xps_len, gboolean reset_cursor);
GtkWidget *create_cat_listbox();
GtkWidget *create_sidebar_controls(ExpContext *ctx);

GtkWidget *create_filter_section(ExpContext *ctx);
void refresh_filter_ui(ExpContext *ctx);

void edit_expense_row(GtkTreeView *tv, GtkTreeIter *it, ExpContext *ctx);

#endif

