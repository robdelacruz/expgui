#ifndef EXPENSE_H
#define EXPENSE_H

#include <gtk/gtk.h>
#include "bslib.h"

typedef struct {
    char *date;
    char *time;
    char *desc;
    double amt;
    char *cat;
} ExpenseLine;

BSArray *load_expense_file(const char *expfile);
int save_expense_file(BSArray *exps, const char *expfile);
void sort_expenses_bydate(BSArray *ee);
void print_expenselines(BSArray *ee);

typedef struct {
    GtkWidget *mainwin;
    GtkWidget *menubar;
    GtkWidget *nb;
    GtkWidget *tv_expenses;
} ExpenseUI;

typedef struct {
    uint month;
    uint year;
} ExpenseFilter;

typedef struct {
    BSArray *exps;
    ExpenseFilter filter;
    ExpenseUI ui;
} ExpenseContext;

#endif

