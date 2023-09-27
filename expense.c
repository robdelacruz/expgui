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

static void chomp(char *buf);
static char *skip_ws(char *startp);
static char *read_field(char *startp, char **field);
static char *read_field_double(char *startp, double *field);
static Expense *read_xp_line(char *buf);

Expense *create_expense(char *isodate, char *time, char *desc, double amt, char *cat) {
    Expense *xp = bs_malloc(sizeof(Expense));
    xp->dt = bs_date_iso_new(isodate);
    xp->time = bs_strdup(time);
    xp->desc = bs_strdup(desc);
    xp->amt = amt;
    xp->cat = bs_strdup(cat);
    return xp;
}

void free_expense(Expense *xp) {
    bs_date_free(xp->dt);
    bs_free(xp->time);
    bs_free(xp->desc);
    bs_free(xp->cat);
    bs_free(xp);
}

ExpContext *create_context() {
    ExpContext *ctx = bs_malloc(sizeof(ExpContext));

    ctx->xpfile = strdup("");
    memset(ctx->all_xps, 0, sizeof(ctx->all_xps));
    memset(ctx->view_xps, 0, sizeof(ctx->view_xps));

    ctx->view_year = 0;
    ctx->view_month = 0;
    ctx->view_wait_id = 0;

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
        bs_free(ctx->xpfile);

    for (int i=0; i < countof(ctx->all_xps); i++) {
        if (ctx->all_xps[i] != NULL)
            free_expense(ctx->all_xps[i]);
    }

    bs_free(ctx);
}

void print_context(ExpContext *ctx) {
    printf("ExpContext:\n");
    printf("xpfile: '%s'\n", ctx->xpfile);
}

// Return 0 for success, 1 for failure with errno set.
int load_expense_file(const char *xpfile, Expense *xps[], size_t xps_size, size_t *ret_count_xps) {
    Expense *xp;
    size_t count_xps = 0;
    FILE *f;
    char *buf;
    size_t buf_size;
    int i, z;

    f = fopen(xpfile, "r");
    if (f == NULL)
        return 1;

    buf = bs_malloc(BUFLINE_SIZE);
    buf_size = BUFLINE_SIZE;

    for (i=0; i < xps_size; i++) {
        errno = 0;
        z = getline(&buf, &buf_size, f);
        if (z == -1 && errno != 0) {
            printf("error: z:%d\n", z);
            bs_free(buf);
            fclose(f);
            *ret_count_xps = 0;
            return 1;
        }
        if (z == -1)
            break;

        chomp(buf);
        xp = read_xp_line(buf);
        xps[count_xps] = xp;
        count_xps++;
    }

    if (i == xps_size)
        printf("Maximum number of expenses (%ld) reached. Wasn't able to load all expenses.\n", xps_size);

    bs_free(buf);
    fclose(f);
    *ret_count_xps = count_xps;
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

static Expense *read_xp_line(char *buf) {
    Expense *xp = bs_malloc(sizeof(Expense));

    // Sample expense line:
    // 2016-05-01; 00:00; Mochi Cream coffee; 100.00; coffee

    char *p = buf;
    p = read_field_date(p, &xp->dt);
    p = read_field(p, &xp->time);
    p = read_field(p, &xp->desc);
    p = read_field_double(p, &xp->amt);
    p = read_field(p, &xp->cat);
    return xp;
}

static int compare_expense_date_asc(void *xp1, void *xp2) {
    Expense *p1 = (Expense *)xp1;
    Expense *p2 = (Expense *)xp2;
    return strcmp(p1->dt->s, p2->dt->s);
}
void sort_expenses_by_date_asc(Expense *xps[], size_t xps_len) {
    sort_array((void **)xps, xps_len, compare_expense_date_asc);
}
static int compare_expense_date_desc(void *xp1, void *xp2) {
    Expense *p1 = (Expense *)xp1;
    Expense *p2 = (Expense *)xp2;
    return strcmp(p2->dt->s, p1->dt->s);
}
void sort_expenses_by_date_desc(Expense *xps[], size_t xps_len) {
    sort_array((void **)xps, xps_len, compare_expense_date_desc);
}
static void swap_array(void *array[], int i, int j) {
    void *tmp = array[i];
    array[i] = array[j];
    array[j] = tmp;
}
static int sort_array_partition(void *array[], int start, int end, CompareFunc compare_func) {
    int imid = start;
    void *pivot = array[end];

    for (int i=start; i < end; i++) {
        if (compare_func(array[i], pivot) < 0) {
            swap_array(array, imid, i);
            imid++;
        }
    }
    swap_array(array, imid, end);
    return imid;
}
static void sort_array_part(void *array[], int start, int end, CompareFunc compare_func) {
    if (start >= end)
        return;

    int pivot = sort_array_partition(array, start, end, compare_func);
    sort_array_part(array, start, pivot-1, compare_func);
    sort_array_part(array, pivot+1, end, compare_func);
}
void sort_array(void *array[], size_t array_len, CompareFunc compare_func) {
    sort_array_part(array, 0, array_len-1, compare_func);
}

void filter_expenses(Expense *src_xps[], size_t src_xps_len,
                     Expense *dest_xps[], size_t *ret_dest_xps_len,
                     const char *filter, uint month, uint year) {
    size_t dest_xps_len;
    size_t count_dest_xps = 0;

    for (int i=0; i < src_xps_len; i++) {
        Expense *xp = src_xps[i];

        if (month != 0 && xp->dt->month != month)
            continue;
        if (year != 0 && xp->dt->year != year)
            continue;
        if (strcasestr(xp->desc, filter) == NULL)
            continue;

        dest_xps[count_dest_xps] = xp;
        count_dest_xps++;
    }

    *ret_dest_xps_len = count_dest_xps;
}

void get_expenses_years(Expense *xps[], size_t xps_len, uint years[], size_t years_size) {
    int lowest_year = 10000;
    int j = 0;

    for (int i=0; i < xps_len; i++) {
        Expense *xp = xps[i];
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

void get_expenses_months(Expense *xps[], size_t xps_len, uint months[], size_t months_size) {
    months[0] = 0;
}

