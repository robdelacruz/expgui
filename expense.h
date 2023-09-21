#ifndef EXPENSE_H
#define EXPENSE_H

#include <gtk/gtk.h>
#include "bslib.h"

typedef struct {
    BSDate *dt;
    char *time;
    char *desc;
    double amt;
    char *cat;
} Expense;

typedef struct {
    char *xpfile;
    BSArray *xps;
    BSArray *filtered_xps;
    uint filter_month;
    uint filter_year;
    guint filter_keysourceid;

    GtkWidget *mainwin;
    GtkWidget *menubar;
    GtkWidget *notebook;
    GtkWidget *tv_xps;
    GtkWidget *txt_filter;
    GtkWidget *cb_year;
    GtkWidget *cb_month;
} ExpContext;

ExpContext *create_context();
void free_context(ExpContext *ctx);
void print_context(ExpContext *ctx);

BSArray *new_xps();
int load_expense_file(const char *xpfile, BSArray *xps);
void sort_expenses_bydate_asc(BSArray *xps);
void sort_expenses_bydate_desc(BSArray *xps);
void print_expenselines(BSArray *xps);
void filter_xps(BSArray *src_xps, BSArray *dest_xps, const char *filter, uint month, uint year);

#endif

