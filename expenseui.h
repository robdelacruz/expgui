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

GtkWidget *create_scroll_window(GtkWidget *child);
GtkWidget *create_frame(gchar *label, GtkWidget *child);
GtkWidget *create_expenses_treeview(ExpContext *ctx);
void refresh_expenses_treeview(GtkTreeView *tv, Expense *xps[], size_t xps_len, gboolean reset_cursor);
GtkWidget *create_cat_listbox();
GtkWidget *create_button_group_vertical();

GtkWidget *create_filter_section(ExpContext *ctx);
void refresh_filter_ui(ExpContext *ctx);

ExpenseEditDialog *create_expense_edit_dialog(arena_t *arena, Expense *xp);
void free_expense_edit_dialog(ExpenseEditDialog *d);
void get_edit_expense(ExpenseEditDialog *d, Expense *xp);

#endif

