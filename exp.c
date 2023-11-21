#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <errno.h>

#include "clib.h"
#include "exp.h"

#define BUFLINE_SIZE 255

exp_t *exp_new() {
    exp_t *xp = malloc(sizeof(Expense));
    xp->rowid = 0;
    xp->dt = current_date();
    xp->time = str_new(5);
    xp->desc = str_new(0);
    xp->cat = str_new(10);
    xp->amt = 0.0;
    return xp;
}
void exp_free(exp_t *xp) {
    str_free(xp->time);
    str_free(xp->desc);
    str_free(xp->cat);
    free(xp);
}

void exp_dup(exp_t *destxp, Expense *srcxp) {
    destxp->dt = srcxp->dt;
    str_assign(destxp->time, srcxp->time->s);
    str_assign(destxp->desc, srcxp->desc->s);
    str_assign(destxp->cat, srcxp->cat->s);
    destxp->amt = srcxp->amt;
    destxp->rowid = srcxp->rowid;
}

