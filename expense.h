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

BSArray *load_expense_file(const char *expfile);
int save_expense_file(BSArray *exps, const char *expfile);
void sort_expenses_bydate(BSArray *exps);
void print_expenses(BSArray *exps);

#endif

