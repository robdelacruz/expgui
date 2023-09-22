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
    uint filter_year;
    uint filter_month;
    guint filter_wait_id;

    GtkWidget *mainwin;
    GtkWidget *menubar;
    GtkWidget *notebook;
    GtkWidget *tv_xps;
    GtkWidget *txt_filter;
    GtkWidget *cb_year;
    GtkWidget *cb_month;

    uint xps_years[100];
    uint xps_months[12];
} ExpContext;

ExpContext *create_context();
void free_context(ExpContext *ctx);
void print_context(ExpContext *ctx);

BSArray *new_xps();
int load_xpfile(const char *xpfile, BSArray *xps);
void sort_xps_bydate_asc(BSArray *xps);
void sort_xps_bydate_desc(BSArray *xps);
void print_xps_lines(BSArray *xps);
void filter_xps(BSArray *src_xps, BSArray *dest_xps, const char *filter, uint month, uint year);
void get_xps_years(BSArray *xps_desc, uint years[], size_t years_size);
void get_xps_months(BSArray *xps_desc, uint months[], size_t months_size);

#endif

