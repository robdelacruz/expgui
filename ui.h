#ifndef UI_H
#define UI_H

#include <gtk/gtk.h>
#include "exp.h"

#define MAX_EXPENSES 32768
#define MAX_YEARS 50

typedef struct {
    str_t *xpfile;
    db_t *db;

    str_t *view_filter;
    uint view_year;
    uint view_month;
    guint view_wait_id;

    GtkWidget *mainwin;
    GtkWidget *expenses_view;
    GtkWidget *txt_filter;
    GtkWidget *yearmenu;
    GtkWidget *yearbtn;
    GtkWidget *monthbtn;
    GtkWidget *yearbtnlabel;
    GtkWidget *monthbtnlabel;
} uictx_t;

uictx_t *uictx_new();
void uictx_free(uictx_t *ctx);
void uictx_reset(uictx_t *ctx);

void uictx_setup_ui(uictx_t *ctx);
int uictx_open_expense_file(uictx_t *ctx, char *xpfile);

#endif
