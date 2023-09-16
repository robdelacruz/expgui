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
} Expense;

typedef struct {
    char *xpfile;
    BSArray *xps;
    uint filter_month;
    uint filter_year;

    GtkWidget *mainwin;
    GtkWidget *menubar;
    GtkWidget *notebook;
    GtkWidget *tv_xps;
} Context;

Context *create_context();
void free_context(Context *ctx);

BSArray *new_xps();
int load_expense_file(Context *ctx, const char *xpfile);
void sort_expenses_bydate(BSArray *xps);
void print_expenselines(BSArray *xps);

#endif

