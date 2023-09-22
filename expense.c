#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <errno.h>

#include "bslib.h"
#include "expense.h"

#define BUFLINE_SIZE 255

static void clear_expense(void *xp);
static int compare_expense_date(void *xp1, void *xp2);
static void chomp(char *buf);
static char *skip_ws(char *startp);
static char *read_field(char *startp, char **field);
static char *read_field_double(char *startp, double *field);
static void read_xp_line(char *buf, Expense *xp);

ExpContext *create_context() {
    ExpContext *ctx = bs_malloc(sizeof(ExpContext));

    ctx->xpfile = strdup("");
    ctx->xps = new_xps();
    ctx->filtered_xps = bs_array_type_new(Expense, 0);
    ctx->filter_year = 0;
    ctx->filter_month = 0;
    ctx->filter_wait_id = 0;

    ctx->mainwin = NULL;
    ctx->menubar = NULL;
    ctx->notebook = NULL;
    ctx->tv_xps = NULL;
    ctx->txt_filter = NULL;
    ctx->cb_year = NULL;
    ctx->cb_month = NULL;

    memset(ctx->xps_years, 0, sizeof(ctx->xps_years));
    memset(ctx->xps_months, 0, sizeof(ctx->xps_months));

    return ctx;
}

void free_context(ExpContext *ctx) {
    if (ctx->xpfile)
        free(ctx->xpfile);
    if (ctx->xps)
        bs_array_free(ctx->xps);
    if (ctx->filtered_xps)
        bs_array_free(ctx->filtered_xps);
    free(ctx);
}

void print_context(ExpContext *ctx) {
    printf("ExpContext:\n");
    printf("xpfile: '%s'\n", ctx->xpfile);
}

BSArray *new_xps() {
    BSArray *xps = bs_array_type_new(Expense, 12);
    bs_array_set_clear_func(xps, clear_expense);
    return xps;
}

static void clear_expense(void *xp) {
    Expense *p = xp;
    if (p->dt)
        bs_date_free(p->dt);
    if (p->time)
        free(p->time);
    if (p->desc)
        free(p->desc);
    if (p->cat)
        free(p->cat);
}

// Return 0 for success, 1 for failure with errno set.
int load_xpfile(const char *xpfile, BSArray *xps) {
    Expense xp;
    FILE *f;
    char *buf;
    size_t buf_size;
    int z;

    f = fopen(xpfile, "r");
    if (f == NULL)
        return 1;

    bs_array_clear(xps);
    buf = malloc(BUFLINE_SIZE);
    buf_size = BUFLINE_SIZE;

    while (1) {
        errno = 0;
        z = getline(&buf, &buf_size, f);
        if (z == -1 && errno != 0) {
            printf("error: z:%d\n", z);
            free(buf);
            fclose(f);
            bs_array_clear(xps);
            return 1;
        }
        if (z == -1)
            break;

        chomp(buf);
        read_xp_line(buf, &xp);
        bs_array_append(xps, &xp);
    }

    free(buf);
    fclose(f);
    return 0;
}

// Remove trailing \n or \r chars.
static void chomp(char *buf) {
    ssize_t buf_len = strlen(buf);
    for (int i=buf_len-1; i >= 0; i--) {
        if (buf[i] == '\n' || buf[i] == '\r')
            buf[i] = 0;
    }
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
        *field = bs_strdup(startp);
        return skip_ws(p+1);
    }

    *field = bs_strdup(startp);
    return p;
}

static char *read_field_date(char *startp, BSDate **dt) {
    char *sfield;
    char *p = read_field(startp, &sfield);
    *dt = bs_date_iso_new(sfield);
    bs_free(sfield);
    return p;
}

static char *read_field_double(char *startp, double *field) {
    char *sfield;
    char *p = read_field(startp, &sfield);
    *field = atof(sfield);
    bs_free(sfield);
    return p;
}

static void read_xp_line(char *buf, Expense *xp) {
    // Sample expense line:
    // 2016-05-01; 00:00; Mochi Cream coffee; 100.00; coffee

    char *p = buf;
    p = read_field_date(p, &xp->dt);
    p = read_field(p, &xp->time);
    p = read_field(p, &xp->desc);
    p = read_field_double(p, &xp->amt);
    p = read_field(p, &xp->cat);
}

static int compare_xp_date_asc(void *xp1, void *xp2) {
    Expense *p1 = (Expense *)xp1;
    Expense *p2 = (Expense *)xp2;
    return strcmp(p1->dt->s, p2->dt->s);
}
static int compare_xp_date_desc(void *xp1, void *xp2) {
    Expense *p1 = (Expense *)xp1;
    Expense *p2 = (Expense *)xp2;
    return strcmp(p2->dt->s, p1->dt->s);
}
void sort_xps_bydate_asc(BSArray *xps) {
    bs_array_sort(xps, compare_xp_date_asc);
}
void sort_xps_bydate_desc(BSArray *xps) {
    bs_array_sort(xps, compare_xp_date_desc);
}

void print_xps_lines(BSArray *xps) {
    for (int i=0; i < xps->len; i++) {
        Expense *xp = bs_array_get(xps, i);
        printf("%d: %-12s %-35s %9.2f  %-15s\n", i, xp->dt->s, xp->desc, xp->amt, xp->cat);
    }
}

void filter_xps(BSArray *src_xps, BSArray *dest_xps, const char *filter, uint month, uint year) {
    bs_array_clear(dest_xps);

    for (int i=0; i < src_xps->len; i++) {
        Expense *xp = bs_array_get(src_xps, i);

        if (month != 0 && xp->dt->month != month)
            continue;
        if (year != 0 && xp->dt->year != year)
            continue;
        if (strcasestr(xp->desc, filter) == NULL)
            continue;

        bs_array_append(dest_xps, xp);
    }
}

void get_xps_years(BSArray *xps_desc, uint years[], size_t years_size) {
    int lowest_year = 10000;
    int j = 0;

    for (int i=0; i < xps_desc->len; i++) {
        Expense *xp = bs_array_get(xps_desc, i);
        uint xp_year = xp->dt->year;
        if (xp_year < lowest_year) {
            years[j] = xp_year;
            j++;
            lowest_year = xp_year;
        }

        if (j >= years_size-1)
            break;
    }

    years[j] = 0;
}

void get_xps_months(BSArray *xps_desc, uint months[], size_t months_size) {
    months[0] = 0;
}

