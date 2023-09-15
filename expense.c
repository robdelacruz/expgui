#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <errno.h>

#include "bslib.h"
#include "expense.h"

#define BUFLINE_SIZE 255

static void clear_expenseline(void *pitem);
static int compare_expense_date(void *e1, void *e2);
static void chomp(char *buf);
static char *skip_ws(char *startp);
static char *read_field(char *startp, char **field);
static char *read_field_double(char *startp, double *field);
static void read_expense_line(char *buf, ExpenseLine *e);

BSArray *load_expense_file(const char *expfile) {
    ExpenseLine e;
    BSArray *ee;
    FILE *f;
    char *buf;
    size_t buf_size;
    int z;

    f = fopen(expfile, "r");
    if (f == NULL)
        return NULL;

    ee = bs_array_type_new(ExpenseLine, 0);
    bs_array_set_clear_func(ee, clear_expenseline);
    buf = malloc(BUFLINE_SIZE);
    buf_size = BUFLINE_SIZE;

    while (1) {
        errno = 0;
        z = getline(&buf, &buf_size, f);
        if (z == -1 && errno != 0) {
            free(buf);
            fclose(f);
            bs_array_free(ee);
            return NULL;
        }
        if (z == -1)
            break;

        chomp(buf);
        read_expense_line(buf, &e);
        bs_array_append(ee, &e);
    }

    free(buf);
    fclose(f);
    return ee;
}

static void clear_expenseline(void *pitem) {
    ExpenseLine *e = pitem;
    if (e->date)
        free(e->date);
    if (e->time)
        free(e->time);
    if (e->desc)
        free(e->desc);
    if (e->cat)
        free(e->cat);
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

static char *read_field_double(char *startp, double *field) {
    char *sfield;
    char *p = read_field(startp, &sfield);
    *field = atof(sfield);
    bs_free(sfield);
    return p;
}

static void read_expense_line(char *buf, ExpenseLine *e) {
    // Sample expense line:
    // 2016-05-01; 00:00; Mochi Cream coffee; 100.00; coffee

    char *p = buf;
    p = read_field(p, &e->date);
    p = read_field(p, &e->time);
    p = read_field(p, &e->desc);
    p = read_field_double(p, &e->amt);
    p = read_field(p, &e->cat);
}

int save_expense_file(BSArray *ee, const char *expfile) {
    return 0;
}

static int compare_expense_date(void *e1, void *e2) {
    ExpenseLine *p1 = (ExpenseLine *)e1;
    ExpenseLine *p2 = (ExpenseLine *)e2;
    return strcmp(p1->date, p2->date);
}

void sort_expenses_bydate(BSArray *ee) {
    bs_array_sort(ee, compare_expense_date);
}

void print_expenselines(BSArray *ee) {
    for (int i=0; i < ee->len; i++) {
        ExpenseLine *p = bs_array_get(ee, i);
        printf("%d: %-12s %-35s %9.2f  %-15s\n", i, p->date, p->desc, p->amt, p->cat);
    }
}

