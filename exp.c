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

uictx_t *uictx_new() {
    uictx_t *ctx = malloc(sizeof(uictx_t));

    ctx->xpfile = str_new(0);

    array_assign(&ctx->all_xps,
                 (void **)ctx->_XPS1, 0, countof(ctx->_XPS1));
    array_assign(&ctx->view_xps,
                 (void **)ctx->_XPS2, 0, countof(ctx->_XPS2));

    date_t today = current_date();
    ctx->view_year = today.year;
    ctx->view_month = today.month;
    ctx->view_month = 0;

    ctx->mainwin = NULL;
    ctx->expenses_view = NULL;
    ctx->txt_filter = NULL;

    return ctx;
}
void uictx_free(uictx_t *ctx) {
    str_free(ctx->xpfile);
    free(ctx);
}

void uictx_reset(uictx_t *ctx) {
    str_assign(ctx->xpfile, "");

    for (int i=0; i < ctx->all_xps.len; i++) {
        assert(ctx->all_xps.items[i] != NULL);
        exp_free(ctx->all_xps.items[i]);
    }

    array_clear(&ctx->all_xps);
    array_clear(&ctx->view_xps);

    date_t today = current_date();
    ctx->view_year = today.year;
    ctx->view_month = today.month;
    ctx->view_month = 0;
}

#define BUFLINE_SIZE 255
void load_expense_file(uictx_t *ctx, FILE *f) {
    exp_t *xp;
    size_t count_xps = 0;
    char *buf;
    size_t buf_size;
    int i, z;

    uictx_reset(ctx);

    buf = malloc(BUFLINE_SIZE);
    buf_size = BUFLINE_SIZE;

    array_t *xps = &ctx->all_xps;
    for (i=0; i < xps->cap; i++) {
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
        xps->items[count_xps] = xp;
        count_xps++;
    }
    xps->len = count_xps;

    free(buf);
    if (i == xps->cap)
        printf("Maximum number of expenses (%ld) reached.\n", xps->cap);

    filter_expenses(ctx);
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

void filter_expenses(uictx_t *ctx) {
    exp_t *xp;
    size_t count_match_xps = 0;
    const gchar *filter;

    array_t *all_xps = &ctx->all_xps;
    array_t *view_xps = &ctx->view_xps;

    filter = gtk_entry_get_text(GTK_ENTRY(ctx->txt_filter));
    if (strlen(filter) == 0)
        filter = NULL;

    for (int i=0; i < all_xps->len; i++) {
        xp = all_xps->items[i];
        if (filter != NULL && strcasestr(xp->desc->s, filter) == NULL)
            continue;
        if (ctx->view_year != 0 && ctx->view_year != xp->dt.year)
            continue;
        if (ctx->view_month != 0 && ctx->view_month != xp->dt.month)
            continue;

        view_xps->items[count_match_xps] = xp;
        count_match_xps++;
    }
    view_xps->len = count_match_xps;
}

void update_expense(exp_t *savexp, uictx_t *ctx) {
    exp_t *xp;
    for (int i=0; i < ctx->all_xps.len; i++) {
        xp = ctx->all_xps.items[i];
        if (xp->rowid == savexp->rowid) {
            exp_dup(xp, savexp);
            return;
        }
    }
}

void add_expense(exp_t *newxp, uictx_t *ctx) {
    exp_t *xp;
    assert(ctx->all_xps.len <= ctx->all_xps.cap);

    if (ctx->all_xps.len == ctx->all_xps.cap) {
        printf("Maximum number of expenses (%ld) reached.\n", ctx->all_xps.cap);
        return;
    }

    xp = exp_new();
    exp_dup(xp, newxp);
    xp->rowid = ctx->all_xps.len;
    ctx->all_xps.items[ctx->all_xps.len] = xp;
    ctx->all_xps.len++;
}
