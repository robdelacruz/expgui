#ifndef UI_H
#define UI_H

#include <gtk/gtk.h>
#include "clib.h"
#include "exp.h"

#define MAX_EXPENSES 32768
typedef int (*CompareFunc)(void *a, void *b);

type struct {
    str_t *xpfile;
    exp_t *all_xps[MAX_EXPENSES];
    exp_t *view_xps[MAX_EXPENSES];
    array_t all_xps_array;
    array_t view_xps_array;

    uint view_year;
    uint view_month;
    guint view_wait_id;
} uictx_t;

uictx_t *uictx_new();
void uictx_free(uictx_t *ctx);
void uictx_reset(uictx_t *ctx);

void load_expense_file(uictx_t *ctx, FILE *f);
void filter_expenses(uictx_t *ctx);

#endif
