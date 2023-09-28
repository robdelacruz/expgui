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

ExpenseEditDialog *create_expense_edit_dialog(Expense *xp);
void free_expense_edit_dialog(ExpenseEditDialog *d);
void get_edit_expense(ExpenseEditDialog *d, Expense *xp);

#endif

