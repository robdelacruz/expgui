#ifndef EXPENSE_H
#define EXPENSE_H

#include <gtk/gtk.h>
#include "bslib.h"

#define MAX_EXPENSES 32768

typedef int (*CompareFunc)(void *a, void *b);

typedef struct {
    uint rowid;
    date_t dt;
    str_t time;
    str_t desc;
    double amt;
    str_t cat;
} Expense;

typedef struct {
    arena_t *arena;
    arena_t *scratch;
    str_t xpfile;
    Expense *all_xps[MAX_EXPENSES];
    Expense *view_xps[MAX_EXPENSES];
    size_t all_xps_count;
    size_t view_xps_count;
    uint expenses_years[100];
    str_t *expenses_cats[100];
    size_t expenses_cats_count;

    uint view_year;
    uint view_month;
    str_t view_cat;
    guint view_wait_id;

    GtkWidget *mainwin;
    GtkWidget *menubar;
    GtkWidget *notebook;
    GtkWidget *expenses_view;
    GtkWidget *txt_filter;
    GtkWidget *cb_year;
    GtkWidget *cb_month;
    GtkWidget *btn_add;
    GtkWidget *btn_edit;
    GtkWidget *btn_del;
    GtkWidget *statusbar;
} ExpContext;

Expense *create_expense(arena_t *arena);
void init_expense(Expense *xp, arena_t *arena);
void init_context(ExpContext *ctx, arena_t *arena, arena_t *scratch);
void reset_context(ExpContext *ctx);

void load_expense_file(FILE *f, Expense *xps[], size_t xps_size, size_t *ret_count_xps, arena_t *arena);
void sort_expenses_by_date_asc(Expense *xps[], size_t xps_len);
void sort_expenses_by_date_desc(Expense *xps[], size_t xps_len);
void sort_cats_asc(str_t *cats[], size_t cats_len);
void sort_array(void *array[], size_t array_len, CompareFunc compare_func);

void filter_expenses(Expense *src_xps[], size_t src_xps_len,
                     Expense *dest_xps[], size_t *ret_dest_xps_len,
                     char *filter,
                     uint month, uint year,
                     char *cat);

void get_expenses_years(Expense *xps[], size_t xps_len, uint years[], size_t years_size);
size_t get_expenses_categories(Expense *xps[], size_t xps_len, str_t *cats[], size_t cats_size);

#endif

