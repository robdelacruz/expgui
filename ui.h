#ifndef UI_H
#define UI_H

#include <gtk/gtk.h>
#include "exp.h"

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

uictx_t *uictx_new();
void uictx_free(uictx_t *ctx);
void uictx_reset(uictx_t *ctx);

void setup_ui(uictx_t *ctx);
int open_expense_file(uictx_t *ctx, char *xpfile);

#endif
