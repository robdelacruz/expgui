#ifndef EXP_H
#define EXP_H

#include <gtk/gtk.h>
#include "clib.h"

#define MAX_CATEGORIES 50
#define MAX_EXPENSES 32768
#define MAX_YEARS 50

typedef int (*CompareFunc)(void *a, void *b);

typedef struct {
    uint id;
    str_t *name;
} cat_t;

typedef struct {
    uint rowid;
    date_t dt;
    str_t *time;
    str_t *desc;
    uint catid;
    double amt;
} exp_t;

typedef struct {
    array_t *cats;
    array_t *all_xps;
    array_t *view_xps;
    intarray_t *years;

    str_t *view_filter;
    uint view_year;
    uint view_month;
} db_t;

cat_t *cat_new();
void cat_free(cat_t *cat);
int cat_is_valid(cat_t *cat);

exp_t *exp_new();
void exp_free(exp_t *xp);
void exp_dup(exp_t *destxp, exp_t *srcxp);
int exp_is_valid(exp_t *xp);
void sort_expenses_by_date_asc(array_t *xps);
void sort_expenses_by_date_desc(array_t *xps);

db_t *db_new();
void db_free(db_t *l);
void db_reset(db_t *l);

void db_load_expense_file(db_t *l, FILE *f);
void db_apply_filter(db_t *l);
void db_update_expense(db_t *l, exp_t *savexp);
void db_add_expense(db_t *l, exp_t *newxp);

#endif
