#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <errno.h>

#include "clib.h"
#include "exp.h"

static void chomp(char *buf);
static char *skip_ws(char *startp);
static char *read_field(char *startp, char **field);
static char *read_field_date(char *startp, date_t *dt);
static char *read_field_double(char *startp, double *field);
static char *read_field_str(char *startp, str_t *str);
static void read_xp_line(char *buf, exp_t *xp);

static void init_year_selections(intarray_t *years, array_t *xps);

exp_t *exp_new() {
    exp_t *xp = malloc(sizeof(exp_t));
    xp->rowid = 0;
    xp->dt = current_date();
    xp->time = str_new(5);
    xp->desc = str_new(0);
    xp->cat = str_new(10);
    xp->amt = 0.0;
    return xp;
}
void exp_free(exp_t *xp) {
    str_free(xp->time);
    str_free(xp->desc);
    str_free(xp->cat);
    free(xp);
}

void exp_dup(exp_t *destxp, exp_t *srcxp) {
    destxp->dt = srcxp->dt;
    str_assign(destxp->time, srcxp->time->s);
    str_assign(destxp->desc, srcxp->desc->s);
    str_assign(destxp->cat, srcxp->cat->s);
    destxp->amt = srcxp->amt;
    destxp->rowid = srcxp->rowid;
}

expledger_t *expledger_new() {
    expledger_t *l = malloc(sizeof(expledger_t));

    l->all_xps = array_new(MAX_EXPENSES);
    l->view_xps = array_new(MAX_EXPENSES);
    l->years = intarray_new(MAX_YEARS);
    l->view_filter = str_new(0);
    l->view_year = 0;
    l->view_month = 0;

    // Initialize years selection to single "All" (0) item.
    l->years->items[0] = 0;
    l->years->len = 1;

    return l;
}
void expledger_free(expledger_t *l) {
    array_free(l->all_xps);
    array_free(l->view_xps);
    intarray_free(l->years);
    str_free(l->view_filter);
    free(l);
}
void expledger_reset(expledger_t *l) {
    array_t *all_xps = l->all_xps;
    for (int i=0; i < all_xps->len; i++) {
        assert(all_xps->items[i] != NULL);
        exp_free(all_xps->items[i]);
    }

    array_clear(l->all_xps);
    array_clear(l->view_xps);

    date_t today = current_date();
    l->view_year = today.year;
    l->view_month = today.month;
    str_assign(l->view_filter, "");
}

#define BUFLINE_SIZE 255
void expledger_load_expense_file(expledger_t *l, FILE *f) {
    array_t *all_xps = l->all_xps;
    exp_t *xp;
    size_t count_xps = 0;
    char *buf;
    size_t buf_size;
    int i, z;

    expledger_reset(l);

    buf = malloc(BUFLINE_SIZE);
    buf_size = BUFLINE_SIZE;

    for (i=0; i < all_xps->cap; i++) {
        errno = 0;
        z = getline(&buf, &buf_size, f);
        if (z == -1 && errno != 0) {
            printf("getline() error: %s\n", strerror(errno));
            break;
        }
        if (z == -1)
            break;
        chomp(buf);

        xp = exp_new();
        xp->rowid = count_xps;
        read_xp_line(buf, xp);
        all_xps->items[count_xps] = xp;
        count_xps++;
    }
    all_xps->len = count_xps;

    free(buf);
    if (i == all_xps->cap)
        printf("Maximum number of expenses (%ld) reached.\n", all_xps->cap);

    sort_expenses_by_date_desc(all_xps);

    init_year_selections(l->years, l->all_xps);

    if (l->all_xps->len > 0) {
        exp_t *xp = l->all_xps->items[0];
        l->view_year = xp->dt.year;
        l->view_month = xp->dt.month;
    } else {
        l->view_year = 0;
        l->view_month = 0;
    }

    expledger_apply_filter(l);
}
// Remove trailing \n or \r chars.
static void chomp(char *buf) {
    ssize_t buf_len = strlen(buf);
    for (int i=buf_len-1; i >= 0; i--) {
        if (buf[i] == '\n' || buf[i] == '\r')
            buf[i] = 0;
    }
}
static void read_xp_line(char *buf, exp_t *xp) {
    // Sample expense line:
    // 2016-05-01; 00:00; Mochi Cream coffee; 100.00; coffee

    char *p = buf;
    p = read_field_date(p, &xp->dt);
    p = read_field_str(p, xp->time);
    p = read_field_str(p, xp->desc);
    p = read_field_double(p, &xp->amt);
    p = read_field_str(p, xp->cat);
}
static char *skip_ws(char *startp) {
    char *p = startp;
    while (*p == ' ')
        p++;
    return p;
}
static char *read_field(char *startp, char **field) {
    char *p = startp;
    while (*p != '\0' && *p != ';')
        p++;

    if (*p == ';') {
        *p = '\0';
        *field = startp;
        return skip_ws(p+1);
    }

    *field = startp;
    return p;
}
static char *read_field_date(char *startp, date_t *dt) {
    char *sfield;
    char *p = read_field(startp, &sfield);
    *dt = date_from_iso(sfield);
    return p;
}
static char *read_field_double(char *startp, double *f) {
    char *sfield;
    char *p = read_field(startp, &sfield);
    *f = atof(sfield);
    return p;
}
static char *read_field_str(char *startp, str_t *str) {
    char *sfield;
    char *p = read_field(startp, &sfield);
    str_assign(str, sfield);
    return p;
}

static void init_year_selections(intarray_t *years, array_t *xps) {
// Assumes that xps is sorted descending order by date.
    uint lowest_year = 10000;
    size_t j = 0;

    assert(years->cap > 0);

    years->items[j] = 0;
    j++;

    for (int i=0; i < xps->len; i++) {
        if (j > years->cap-1)
            break;

        exp_t *xp = xps->items[i];
        uint xp_year = xp->dt.year;
        if (xp_year < lowest_year) {
            years->items[j] = (int)xp_year;
            lowest_year = xp_year;
            j++;
        }
    }
    years->len = j;
}

void expledger_apply_filter(expledger_t *l) {
    exp_t *xp;
    size_t count_match_xps = 0;

    array_t *all_xps = l->all_xps;
    array_t *view_xps = l->view_xps;
    char *filter = l->view_filter->s;
    uint year = l->view_year;
    uint month = l->view_month;

    if (l->view_filter->len == 0)
        filter = NULL;

    for (int i=0; i < all_xps->len; i++) {
        xp = all_xps->items[i];
        if (filter != NULL && strcasestr(xp->desc->s, filter) == NULL)
            continue;
        if (year != 0 && year != xp->dt.year)
            continue;
        if (month != 0 && month != xp->dt.month)
            continue;

        view_xps->items[count_match_xps] = xp;
        count_match_xps++;
    }
    view_xps->len = count_match_xps;
}

void expledger_update_expense(expledger_t *l, exp_t *savexp) {
    array_t *all_xps = l->all_xps;
    exp_t *xp;

    for (int i=0; i < all_xps->len; i++) {
        xp = all_xps->items[i];
        if (xp->rowid == savexp->rowid) {
            exp_dup(xp, savexp);
            return;
        }
    }
}

void expledger_add_expense(expledger_t *l, exp_t *newxp) {
    array_t *all_xps = l->all_xps;
    exp_t *xp;

    assert(all_xps->len <= all_xps->cap);
    if (all_xps->len == all_xps->cap) {
        printf("Maximum number of expenses (%ld) reached.\n", all_xps->cap);
        return;
    }

    xp = exp_new();
    exp_dup(xp, newxp);
    xp->rowid = all_xps->len;
    all_xps->items[all_xps->len] = xp;
    all_xps->len++;
}

static int compare_expense_date_asc(void *xp1, void *xp2) {
    date_t *dt1 = &((exp_t *)xp1)->dt;
    date_t *dt2 = &((exp_t *)xp2)->dt;
    if (dt1->year > dt2->year)
        return 1;
    if (dt1->year < dt2->year)
        return -1;
    if (dt1->month > dt2->month)
        return 1;
    if (dt1->month < dt2->month)
        return -1;
    if (dt1->day > dt2->day)
        return 1;
    if (dt1->day < dt2->day)
        return -1;
    return 0;
}
static int compare_expense_date_desc(void *xp1, void *xp2) {
    return -compare_expense_date_asc(xp1, xp2);
}
void sort_expenses_by_date_asc(array_t *xps) {
    sort_array(xps->items, xps->len, compare_expense_date_asc);
}
void sort_expenses_by_date_desc(array_t *xps) {
    sort_array(xps->items, xps->len, compare_expense_date_desc);
}

