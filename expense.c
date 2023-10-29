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
static char *read_field_date(char *startp, date_t *dt);
static char *read_field_double(char *startp, double *field);
static char *read_field_str(char *startp, str_t *str);
static void read_xp_line(char *buf, Expense *xp);
static int array_contains_str(str_t *a[], size_t a_len, str_t *str);

Expense *create_expense(arena_t *arena) {
    Expense *xp = arena_alloc(arena, sizeof(Expense));
    init_expense(xp, arena);
    return xp;
}

void init_expense(Expense *xp, arena_t *arena) {
    xp->rowid = 0;
    xp->dt = default_date();
    xp->time = new_str(arena, 5);
    xp->desc = new_str(arena, 0);
    xp->amt = 0.0;
    xp->cat = new_str(arena, 10);
}

void init_context(ExpContext *ctx, arena_t *arena, arena_t *scratch) {
    ctx->arena = arena;
    ctx->scratch = scratch;
    ctx->xpfile = new_str(arena, STR_SMALL);
    memset(ctx->all_xps, 0, sizeof(ctx->all_xps));
    memset(ctx->view_xps, 0, sizeof(ctx->view_xps));
    ctx->all_xps_count = 0;
    ctx->view_xps_count = 0;
    memset(ctx->expenses_years, 0, sizeof(ctx->expenses_years));
    memset(ctx->expenses_cats, 0, sizeof(ctx->expenses_cats));
    ctx->expenses_cats_count = 0;

    ctx->view_year = 0;
    ctx->view_month = 0;
    ctx->view_cat = new_str(arena, 10);
    ctx->view_wait_id = 0;

    ctx->mainwin = NULL;
    ctx->menubar = NULL;
    ctx->notebook = NULL;
    ctx->expenses_view = NULL;
    ctx->txt_filter = NULL;
    ctx->cb_year = NULL;
    ctx->cb_month = NULL;
    ctx->btn_add = NULL;
    ctx->btn_edit = NULL;
    ctx->btn_del = NULL;
    ctx->statusbar = NULL;
}

void reset_context(ExpContext *ctx) {
    memset(ctx->all_xps, 0, sizeof(ctx->all_xps));
    memset(ctx->view_xps, 0, sizeof(ctx->view_xps));
    ctx->all_xps_count = 0;
    ctx->view_xps_count = 0;
    memset(ctx->expenses_years, 0, sizeof(ctx->expenses_years));
    memset(ctx->expenses_cats, 0, sizeof(ctx->expenses_cats));

    ctx->view_year = 0;
    ctx->view_month = 0;
    ctx->view_wait_id = 0;

    arena_reset(ctx->arena);
    arena_reset(ctx->scratch);

    ctx->xpfile = new_str(ctx->arena, STR_SMALL);
    ctx->view_cat = new_str(ctx->arena, 10);
}

// Return 0 for success, 1 for failure with errno set.
void load_expense_file(FILE *f, Expense *xps[], size_t xps_size, size_t *ret_count_xps, arena_t *arena) {
    Expense *xp;
    size_t count_xps = 0;
    char *buf;
    size_t buf_size;
    int i, z;

    buf = malloc(BUFLINE_SIZE);
    buf_size = BUFLINE_SIZE;

    for (i=0; i < xps_size; i++) {
        errno = 0;
        z = getline(&buf, &buf_size, f);
        if (z == -1 && errno != 0) {
            printf("getline() error: %s\n", strerror(errno));
            break;
        }
        if (z == -1)
            break;
        chomp(buf);

        xp = create_expense(arena);
        xp->rowid = count_xps;
        read_xp_line(buf, xp);
        xps[count_xps] = xp;
        count_xps++;
    }

    if (i == xps_size)
        printf("Maximum number of expenses (%ld) reached. Wasn't able to load all expenses.\n", xps_size);

    free(buf);
    *ret_count_xps = count_xps;
}

// Remove trailing \n or \r chars.
static void chomp(char *buf) {
    ssize_t buf_len = strlen(buf);
    for (int i=buf_len-1; i >= 0; i--) {
        if (buf[i] == '\n' || buf[i] == '\r')
            buf[i] = 0;
    }
}

static void read_xp_line(char *buf, Expense *xp) {
    // Sample expense line:
    // 2016-05-01; 00:00; Mochi Cream coffee; 100.00; coffee

    char *p = buf;
    p = read_field_date(p, &xp->dt);
    p = read_field_str(p, &xp->time);
    p = read_field_str(p, &xp->desc);
    p = read_field_double(p, &xp->amt);
    p = read_field_str(p, &xp->cat);
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

static int compare_expense_date_asc(void *xp1, void *xp2) {
    date_t *dt1 = &((Expense*)xp1)->dt;
    date_t *dt2 = &((Expense*)xp2)->dt;
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
void sort_expenses_by_date_asc(Expense *xps[], size_t xps_len) {
    sort_array((void **)xps, xps_len, compare_expense_date_asc);
}
void sort_expenses_by_date_desc(Expense *xps[], size_t xps_len) {
    sort_array((void **)xps, xps_len, compare_expense_date_desc);
}

static int compare_str_asc(void *p1, void *p2) {
    str_t *str1 = (str_t *)p1;
    str_t *str2 = (str_t *)p2;
    return strcmp(str1->s, str2->s);
}
void sort_cats_asc(str_t *cats[], size_t cats_len) {
    sort_array((void **)cats, cats_len, compare_str_asc);
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
                     char *filter,
                     uint month, uint year,
                     char *cat) {
    size_t dest_xps_len;
    size_t count_dest_xps = 0;

    printf("filter_expenses() filter: '%s' year: %d month: %d cat: '%s'\n", filter, year, month, cat);

    if (strlen(filter) == 0)
        filter = NULL;
    if (strlen(cat) == 0)
        cat = NULL;

    for (int i=0; i < src_xps_len; i++) {
        Expense *xp = src_xps[i];
        if (month != 0 && xp->dt.month != month)
            continue;
        if (year != 0 && xp->dt.year != year)
            continue;
        if (cat != NULL && strcmp(xp->cat.s, cat) != 0)
            continue;
        if (filter != NULL && strcasestr(xp->desc.s, filter) == NULL)
            continue;

        dest_xps[count_dest_xps] = xp;
        count_dest_xps++;
    }

    *ret_dest_xps_len = count_dest_xps;
}

void get_expenses_years(Expense *xps[], size_t xps_len, uint years[], size_t years_size) {
    int lowest_year = 10000;
    size_t j = 0;

    for (int i=0; i < xps_len; i++) {
        Expense *xp = xps[i];
        uint xp_year = xp->dt.year;
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

static int array_contains_str(str_t *a[], size_t a_len, str_t *str) {
    for (int i=0; i < a_len; i++) {
        str_t *a_str = a[i];
        if (strcmp(a_str->s, str->s) == 0) // str in array
            return 1;
    }
    return 0;
}

size_t get_expenses_categories(Expense *xps[], size_t xps_len, str_t *cats[], size_t cats_size) {
    size_t count_cats = 0;

    for (int i=0; i < xps_len; i++) {
        Expense *xp = xps[i];
        str_t *xp_cat = &xp->cat;
        if (!array_contains_str(cats, count_cats, xp_cat)) {
            cats[count_cats] = xp_cat;
            count_cats++;
        }

        if (count_cats >= cats_size-1)
            break;
    }

    cats[count_cats] = 0;
    return count_cats;
}

