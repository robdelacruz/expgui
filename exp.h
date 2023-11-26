#ifndef EXP_H
#define EXP_H

#include <gtk/gtk.h>
#include "clib.h"

typedef int (*CompareFunc)(void *a, void *b);

typedef struct {
    uint rowid;
    date_t dt;
    str_t *time;
    str_t *desc;
    str_t *cat;
    double amt;
} exp_t;

exp_t *exp_new();
void exp_free(exp_t *xp);
void exp_dup(exp_t *destxp, exp_t *srcxp);

void load_expense_file(FILE *f, array_t *destxps);
void filter_expenses(array_t *srcxps, array_t *destxps, char *filter, uint year, uint month);

void update_expense(array_t *xps, exp_t *savexp);
void add_expense(array_t *xps, exp_t *newxp);

void sort_expenses_by_date_asc(array_t *xps);
void sort_expenses_by_date_desc(array_t *xps);

#endif
