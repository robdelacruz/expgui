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

#define BUFLINE_SIZE 255
void load_expense_file(FILE *f, array_t *destxps) {
    exp_t *xp;
    size_t count_xps = 0;
    char *buf;
    size_t buf_size;
    int i, z;

    buf = malloc(BUFLINE_SIZE);
    buf_size = BUFLINE_SIZE;

    for (i=0; i < destxps->cap; i++) {
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
        destxps->items[count_xps] = xp;
        count_xps++;
    }
    destxps->len = count_xps;

    free(buf);
    if (i == destxps->cap)
        printf("Maximum number of expenses (%ld) reached.\n", destxps->cap);
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

void filter_expenses(array_t *srcxps, array_t *destxps, char *filter, uint year, uint month) {
    exp_t *xp;
    size_t count_match_xps = 0;

    if (strlen(filter) == 0)
        filter = NULL;

    for (int i=0; i < srcxps->len; i++) {
        xp = srcxps->items[i];
        if (filter != NULL && strcasestr(xp->desc->s, filter) == NULL)
            continue;
        if (year != 0 && year != xp->dt.year)
            continue;
        if (month != 0 && month != xp->dt.month)
            continue;

        destxps->items[count_match_xps] = xp;
        count_match_xps++;
    }
    destxps->len = count_match_xps;
}

void update_expense(array_t *xps, exp_t *savexp) {
    exp_t *xp;
    for (int i=0; i < xps->len; i++) {
        xp = xps->items[i];
        if (xp->rowid == savexp->rowid) {
            exp_dup(xp, savexp);
            return;
        }
    }
}

void add_expense(array_t *xps, exp_t *newxp) {
    exp_t *xp;
    assert(xps->len <= xps->cap);

    if (xps->len == xps->cap) {
        printf("Maximum number of expenses (%ld) reached.\n", xps->cap);
        return;
    }

    xp = exp_new();
    exp_dup(xp, newxp);
    xp->rowid = xps->len;
    xps->items[xps->len] = xp;
    xps->len++;
}
