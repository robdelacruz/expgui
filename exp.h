#ifndef EXP_H
#define EXP_H

#include "clib.h"

typedef struct {
    uint rowid;
    date_t dt;
    str_t *time;
    str_t *desc;
    str_t *cat;
    double amt;
} exp_t;

exp_t *exp_new();
void exp_free(Expense *xp);
void exp_dup(Expense *destxp, Expense *srcxp);

#endif
