#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <errno.h>

#include "bslib.h"

static inline void quit(const char *s);
static inline void print_error(const char *s);
static inline void panic(const char *s);

typedef struct {
    char *date;
    char *time;
    char *desc;
    double amt;
    char *cat;
} Expense;

void clear_expense(void *pitem) {
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

// Remove trailing \n or \r chars.
void chomp(char *buf) {
    ssize_t buf_len = strlen(buf);
    for (int i=buf_len-1; i >= 0; i--) {
        if (buf[i] == '\n' || buf[i] == '\r')
            buf[i] = 0;
    }
}

char *skip_ws(char *startp) {
    char *p = startp;
    while (*p == ' ')
        p++;
    return p;
}

char *read_field(char *startp, char **field) {
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

char *read_field_double(char *startp, double *field) {
    char *sfield;
    char *p = read_field(startp, &sfield);
    *field = atof(sfield);
    bs_free(sfield);
    return p;
}

//2016-05-01; 00:00; Mochi Cream coffee; 100.00; coffee
void read_expense_line(char *buf, Expense *exp) {
    char *p = buf;

    p = read_field(p, &exp->date);
    p = read_field(p, &exp->time);
    p = read_field(p, &exp->desc);
    p = read_field_double(p, &exp->amt);
    p = read_field(p, &exp->cat);
}

#define BUFLINE_SIZE 255

int main(int argc, char *argv[]) {
    if (argc <= 1) {
        quit("No expense file arg.");
    }

    char *expfile = argv[1];
    printf("Expense file: %s\n", expfile);

    FILE *f = fopen(expfile, "r");
    if (f == NULL)
        panic("Error opening expense file");

    BSArray *exps = bs_array_type_new(Expense, 0);
    bs_array_set_clear_func(exps, clear_expense);
    Expense exp;
    char *buf = bs_malloc(BUFLINE_SIZE);
    size_t buf_size = BUFLINE_SIZE;
    while (1) {
        errno = 0;
        int z = getline(&buf, &buf_size, f);
        if (z == -1 && errno != 0) {
            bs_free(buf);
            fclose(f);
            panic("Error reading expense line");
        }
        if (z == -1)
            break;

        chomp(buf);
        read_expense_line(buf, &exp);
        bs_array_append(exps, &exp);
    }
    bs_free(buf);
    fclose(f);

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




