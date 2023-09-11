#ifndef EXPENSE_H
#define EXPENSE_H

#include "bslib.h"

typedef struct {
    char *date;
    char *time;
    char *desc;
    double amt;
    char *cat;
} Expense;

void clear_expense(void *pitem);
BSArray *load_expense_file(const char *expfile);
int save_expense_file(BSArray *exps, const char *expfile);

#endif

