#ifndef EXP_H
#define EXP_H

#include <gtk/gtk.h>
#include "clib.h"

#define MAX_EXPENSES 32768
typedef int (*CompareFunc)(void *a, void *b);

typedef struct {
    uint rowid;
    date_t dt;
    str_t *time;
    str_t *desc;
    str_t *cat;
    double amt;
} exp_t;

typedef struct {
    str_t *xpfile;
    exp_t *_XPS1[MAX_EXPENSES];
    exp_t *_XPS2[MAX_EXPENSES];
    array_t all_xps;
    array_t view_xps;

    uint view_year;
    uint view_month;
    guint view_wait_id;

    GtkWidget *mainwin;
    GtkWidget *expenses_view;
    GtkWidget *txt_filter;
} uictx_t;

exp_t *exp_new();
void exp_free(exp_t *xp);
void exp_dup(exp_t *destxp, exp_t *srcxp);

uictx_t *uictx_new();
void uictx_free(uictx_t *ctx);
void uictx_reset(uictx_t *ctx);

void load_expense_file(uictx_t *ctx, FILE *f);
void filter_expenses(uictx_t *ctx);

void update_expense(exp_t *savexp, uictx_t *ctx);
void add_expense(exp_t *newxp, uictx_t *ctx);

#endif
