#ifndef UI_H
#define UI_H

#include <gtk/gtk.h>
#include "exp.h"

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

GtkWidget *create_expenses_section(uictx_t *ctx);
GtkWidget *create_sidebar_controls(uictx_t *ctx);
void refresh_expenses_treeview(GtkTreeView *tv, array_t *xps, gboolean reset_cursor);

void add_expense_row(uictx_t *ctx);
void edit_expense_row(GtkTreeView *tv, GtkTreeIter *it, uictx_t *ctx);

#endif
