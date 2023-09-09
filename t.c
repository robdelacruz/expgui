#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#include "blib.h"

void quit(const char *s);

typedef struct {
    char *date;
    char *desc;
    uint32_t amt;
    char *cat;
} Expense;

int main(int argc, char *argv[]) {
    if (argc <= 1) {
        quit("No expense file arg.");
    }

    char *expfile = argv[1];
    printf("Expense file: %s\n", expfile);

    BArray *exps = b_array_type_new(Expense, 0);
    Expense exp;

    exp.date = strdup("2023-09-02");
    exp.desc = strdup("rustans");
    exp.amt = 188050;   // 1880.50
    exp.cat = strdup("grocery");
    b_array_append(exps, &exp);

    exp.date = strdup("2023-09-03");
    exp.desc = strdup("grab");
    exp.amt = 17200;   // 172.00
    exp.cat = strdup("commute");
    b_array_append(exps, &exp);

    printf("List expenses...\n");
    for (int i=0; i < exps->len; i++) {
        Expense *p = b_array_get(exps, i);
        printf("%d: %12s %20s %9.2f %15s\n", i, p->date, p->desc, (float)(p->amt / 10), p->cat);
    }
}

void quit(const char *s) {
    printf("Error: %s\n", s);
    exit(1);
}




