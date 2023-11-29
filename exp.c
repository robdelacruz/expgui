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
static char *read_field_uint(char *startp, uint *n);
static char *read_field_str(char *startp, str_t *str);
static void read_xp_line(char *buf, exp_t *xp);
static void read_cat_line(char *buf, cat_t *cat);

static void add_xp(char *buf, int lineno, array_t *all_xps, size_t *count_xps);
static void add_cat(char *buf, int lineno, array_t *cats, size_t *count_cats);

static void init_year_selections(intarray_t *years, array_t *xps);

exp_t *exp_new() {
    exp_t *xp = malloc(sizeof(exp_t));
    xp->rowid = 0;
    xp->dt = date_current();
    xp->time = str_new(5);
    xp->desc = str_new(0);
    xp->catid = 0;
    xp->amt = 0.0;
    return xp;
}
void exp_free(exp_t *xp) {
    str_free(xp->time);
    str_free(xp->desc);
    free(xp);
}

void exp_dup(exp_t *destxp, exp_t *srcxp) {
    destxp->dt = srcxp->dt;
    str_assign(destxp->time, srcxp->time->s);
    str_assign(destxp->desc, srcxp->desc->s);
    destxp->catid = srcxp->catid;
    destxp->amt = srcxp->amt;
    destxp->rowid = srcxp->rowid;
}

int exp_is_valid(exp_t *xp) {
    if (date_is_zero(xp->dt) || xp->desc->len == 0)
        return 0;
    return 1;
}

static int exp_compare_date_asc(void *xp1, void *xp2) {
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
static int exp_compare_date_desc(void *xp1, void *xp2) {
    return -exp_compare_date_asc(xp1, xp2);
}
void sort_expenses_by_date_asc(array_t *xps) {
    sort_array(xps->items, xps->len, exp_compare_date_asc);
}
void sort_expenses_by_date_desc(array_t *xps) {
    sort_array(xps->items, xps->len, exp_compare_date_desc);
}

cat_t *cat_new() {
    cat_t *cat = malloc(sizeof(cat_t));
    cat->id = 0;
    cat->name = str_new(15);
    return cat;
}
void cat_free(cat_t *cat) {
    str_free(cat->name);
    free(cat);
}
int cat_is_valid(cat_t *cat) {
    if (cat->id == 0 || cat->name->len == 0)
        return 0;
    return 1;
}

db_t *db_new() {
    db_t *db = malloc(sizeof(db_t));

    db->cats = array_new(MAX_CATEGORIES);

    db->all_xps = array_new(MAX_EXPENSES);
    db->view_xps = array_new(MAX_EXPENSES);
    db->years = intarray_new(MAX_YEARS);
    db->view_filter = str_new(0);
    db->view_year = 0;
    db->view_month = 0;

    // Initialize years selection to single "All" (0) item.
    db->years->items[0] = 0;
    db->years->len = 1;

    return db;
}
void db_free(db_t *db) {
    array_free(db->all_xps);
    array_free(db->view_xps);
    intarray_free(db->years);
    str_free(db->view_filter);
    free(db);
}
void db_reset(db_t *db) {
    array_t *all_xps = db->all_xps;
    for (int i=0; i < all_xps->len; i++) {
        assert(all_xps->items[i] != NULL);
        exp_free(all_xps->items[i]);
    }

    array_clear(db->all_xps);
    array_clear(db->view_xps);

    date_t today = date_current();
    db->view_year = today.year;
    db->view_month = today.month;
    str_assign(db->view_filter, "");
}

#define BUFLINE_SIZE 255
enum loadstate_t {DB_LOAD_NONE, DB_LOAD_CAT, DB_LOAD_EXP};

void db_load_expense_file(db_t *db, FILE *f) {
    array_t *all_xps = db->all_xps;
    array_t *cats = db->cats;
    size_t count_xps = 0;
    size_t count_cats = 0;
    int lineno = 1;
    char *buf;
    size_t buf_size;
    int z;
    enum loadstate_t state = DB_LOAD_NONE;

    db_reset(db);

    buf = malloc(BUFLINE_SIZE);
    buf_size = BUFLINE_SIZE;

    while (1) {
        errno = 0;
        z = getline(&buf, &buf_size, f);
        if (z == -1 && errno != 0) {
            print_error("getline() error");
            break;
        }
        if (z == -1)
            break;
        chomp(buf);

        if (strlen(buf) == 0)
            continue;
        if (buf[0] == '#') {
            lineno++;
            continue;
        }
        if (buf[0] == '%') {
            if (strcmp(buf+1, "categories") == 0)
                state = DB_LOAD_CAT;
            else if (strcmp(buf+1, "expenses") == 0)
                state = DB_LOAD_EXP;
            else
                fprintf(stderr, "Skipping invalid line: '%s'\n", buf);

            lineno++;
            continue;
        }

        if (state == DB_LOAD_EXP)
            add_xp(buf, lineno, all_xps, &count_xps);
        else if (state == DB_LOAD_CAT)
            add_cat(buf, lineno, cats, &count_cats);
        lineno++;
    }
    all_xps->len = count_xps;
    cats->len = count_cats;

    free(buf);
    if (count_xps > all_xps->cap-1)
        fprintf(stderr, "Max expenses (%ld) reached.\n", all_xps->cap);
    if (count_cats > cats->cap-1)
        fprintf(stderr, "Max categories (%ld) reached.\n", cats->cap);

    sort_expenses_by_date_desc(all_xps);

    init_year_selections(db->years, db->all_xps);

    if (db->all_xps->len > 0) {
        exp_t *xp = db->all_xps->items[0];
        db->view_year = xp->dt.year;
        db->view_month = xp->dt.month;
    } else {
        db->view_year = 0;
        db->view_month = 0;
    }

    db_apply_filter(db);
}

static void add_xp(char *buf, int lineno, array_t *all_xps, size_t *count_xps) {
    exp_t *xp;
    size_t count = *count_xps;

    if (count > all_xps->cap-1)
        return;

    xp = exp_new();
    xp->rowid = count;
    read_xp_line(buf, xp);
    if (!exp_is_valid(xp)) {
        fprintf(stderr, "Skipping invalid expense line: %d\n", lineno);
        exp_free(xp);
        return;
    }

    all_xps->items[count] = xp;
    (*count_xps)++;
}
static void add_cat(char *buf, int lineno, array_t *cats, size_t *count_cats) {
    cat_t *cat;
    size_t count = *count_cats;

    if (count > cats->cap-1)
        return;

    cat = cat_new();
    read_cat_line(buf, cat);
    if (!cat_is_valid(cat)) {
        fprintf(stderr, "Skipping invalid category line: %d\n", lineno);
        cat_free(cat);
        return;
    }
    cats->items[count] = cat;
    (*count_cats)++;
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
    p = read_field_uint(p, &xp->catid);
}
static void read_cat_line(char *buf, cat_t *cat) {
    char *p = buf;
    p = read_field_uint(p, &cat->id);
    p = read_field_str(p, cat->name);
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
static char *read_field_uint(char *startp, uint *n) {
    char *sfield;
    char *p = read_field(startp, &sfield);
    *n = atoi(sfield);
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

void db_apply_filter(db_t *db) {
    exp_t *xp;
    size_t count_match_xps = 0;

    array_t *all_xps = db->all_xps;
    array_t *view_xps = db->view_xps;
    char *filter = db->view_filter->s;
    uint year = db->view_year;
    uint month = db->view_month;

    if (db->view_filter->len == 0)
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

void db_update_expense(db_t *db, exp_t *savexp) {
    array_t *all_xps = db->all_xps;
    exp_t *xp;

    for (int i=0; i < all_xps->len; i++) {
        xp = all_xps->items[i];
        if (xp->rowid == savexp->rowid) {
            exp_dup(xp, savexp);
            return;
        }
    }
}

void db_add_expense(db_t *db, exp_t *newxp) {
    array_t *all_xps = db->all_xps;
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

