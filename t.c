#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <errno.h>

#include "bslib.h"
#include "expense.h"

static inline void quit(const char *s);
static inline void print_error(const char *s);
static inline void panic(const char *s);

int compare_expense_date(void *a, void *b) {
    Expense *expa = (Expense *)a;
    Expense *expb = (Expense *)b;
    
    return strcmp(expa->date, expb->date);
}

int main(int argc, char *argv[]) {
    if (argc <= 1) {
        quit("No expense file arg.");
    }

    char *expfile = argv[1];
    BSArray *exps = load_expense_file(expfile);
    if (exps == NULL)
        panic("Error reading expense file");

    bs_array_shuffle(exps);
    //bs_array_reverse(exps);
    bs_array_sort(exps, compare_expense_date);

    printf("List expenses...\n");
    for (int i=0; i < exps->len; i++) {
        Expense *p = bs_array_get(exps, i);
        printf("%d: %-12s %-35s %9.2f  %-15s\n", i, p->date, p->desc, p->amt, p->cat);
    }
    bs_array_free(exps);
}

static inline void quit(const char *s) {
    if (s)
        printf("%s\n", s);
    exit(0);
}
static inline void print_error(const char *s) {
    if (s)
        fprintf(stderr, "%s: %s\n", s, strerror(errno));
    else
        fprintf(stderr, "%s\n", strerror(errno));
}
static inline void panic(const char *s) {
    print_error(s);
    exit(1);
}




