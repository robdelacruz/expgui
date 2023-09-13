#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <errno.h>

#include "bslib.h"
#include "expense.h"

#define BUFLINE_SIZE 255

static void clear_expense(void *pitem) {
    Expense *exp = pitem;
    if (exp->date)
        free(exp->date);
    if (exp->time)
        free(exp->time);
    if (exp->desc)
        free(exp->desc);
    if (exp->cat)
        free(exp->cat);
}

static int compare_expense_date(void *a, void *b) {
    Expense *expa = (Expense *)a;
    Expense *expb = (Expense *)b;
    
    return strcmp(expa->date, expb->date);
}

void sort_expenses_bydate(BSArray *exps) {
    bs_array_sort(exps, compare_expense_date);
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

static void read_expense_line(char *buf, Expense *exp) {
    // Sample expense line:
    // 2016-05-01; 00:00; Mochi Cream coffee; 100.00; coffee

    char *p = buf;
    p = read_field(p, &exp->date);
    p = read_field(p, &exp->time);
    p = read_field(p, &exp->desc);
    p = read_field_double(p, &exp->amt);
    p = read_field(p, &exp->cat);
}

BSArray *load_expense_file(const char *expfile) {
    Expense exp;
    BSArray *exps;
    FILE *f;
    char *buf;
    size_t buf_size;
    int z;

    f = fopen(expfile, "r");
    if (f == NULL)
        return NULL;

    exps = bs_array_type_new(Expense, 0);
    bs_array_set_clear_func(exps, clear_expense);
    buf = malloc(BUFLINE_SIZE);
    buf_size = BUFLINE_SIZE;

    while (1) {
        errno = 0;
        z = getline(&buf, &buf_size, f);
        if (z == -1 && errno != 0) {
            free(buf);
            fclose(f);
            bs_array_free(exps);
            return NULL;
        }
        if (z == -1)
            break;

        chomp(buf);
        read_expense_line(buf, &exp);
        bs_array_append(exps, &exp);
    }

    free(buf);
    fclose(f);
    return exps;
}

int save_expense_file(BSArray *exps, const char *expfile) {
    return 0;
}

void print_expenses(BSArray *exps) {
    for (int i=0; i < exps->len; i++) {
        Expense *p = bs_array_get(exps, i);
        printf("%d: %-12s %-35s %9.2f  %-15s\n", i, p->date, p->desc, p->amt, p->cat);
    }
}

