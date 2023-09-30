#ifndef EXPENSE_H
#define EXPENSE_H

#include <gtk/gtk.h>
#include "bslib.h"

#define MAX_EXPENSES 32768

typedef int (*CompareFunc)(void *a, void *b);

typedef struct {
    BSDate *dt;
    char *time;
    char *desc;
    double amt;
    char *cat;
} Expense;

typedef struct {
    char *xpfile;
    Expense *all_xps[MAX_EXPENSES];
    Expense *view_xps[MAX_EXPENSES];
    size_t all_xps_len;
    size_t view_xps_len;

    uint view_year;
    uint view_month;
    guint view_wait_id;

    GtkWidget *mainwin;
    GtkWidget *menubar;
    GtkWidget *notebook;
    GtkWidget *expenses_view;
    GtkWidget *txt_filter;
    GtkWidget *cb_year;
    GtkWidget *cb_month;

    uint expenses_years[100];
} ExpContext;

Expense *create_expense(char *isodate, char *time, char *desc, double amt, char *cat);
void free_expense(Expense *xp);

ExpContext *create_context();
void free_context(ExpContext *ctx);

void free_expense_array(Expense *xps[], size_t xps_size);
int load_expense_file(const char *xpfile, Expense *xps[], size_t xps_size, size_t *ret_count_xps);
void sort_expenses_by_date_asc(Expense *xps[], size_t xps_len);
void sort_expenses_by_date_desc(Expense *xps[], size_t xps_len);
void sort_array(void *array[], size_t array_len, CompareFunc compare_func);
void filter_expenses(Expense *src_xps[], size_t src_xps_len,
                     Expense *dest_xps[], size_t *ret_dest_xps_len,
                     const char *filter, uint month, uint year);

void get_expenses_years(Expense *xps[], size_t xps_len, uint years[], size_t years_size);

#endif

