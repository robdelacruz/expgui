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
    uint32_t amt;
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

char *read_field(char *startp, char **field) {
    char *p = startp;
    while (*p != '\0') {
        if (*p == ';') {
            *p = '\0';
            *field = strdup(startp);
            return p+1;
        }
        p++;
    }
    *field = strdup("");
    return NULL;
}

char *skip_ws(char *startp) {
    char *p = startp;
    while (*p != '\0') {
        if (*p != ' ')
            return p;
        p++;
    }
    return NULL;
}

//2016-05-01; 00:00; Mochi Cream coffee; 100.00; coffee
void read_expense_line(char *buf, Expense *exp) {
    char *p = buf;

    while (1) {
        p = read_field(p, &exp->date);
        if (p == NULL)
            break;
        p = skip_ws(p);
        if (p == NULL)
            break;

        p = read_field(p, &exp->time);
        if (p == NULL)
            break;
        p = skip_ws(p);
        if (p == NULL)
            break;
    
        p = read_field(p, &exp->desc);
        if (p == NULL)
            break;
        p = skip_ws(p);
        if (p == NULL)
            break;
    
        char *amt;
        p = read_field(p, &amt);
        if (p == NULL)
            break;
        p = skip_ws(p);
        if (p == NULL)
            break;
        exp->amt = 12300; // $$

        p = read_field(p, &exp->cat);
        if (p == NULL)
            break;
        p = skip_ws(p);
        if (p == NULL)
            break;

        break;
    }
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

    char *buf = malloc(BUFLINE_SIZE);
    size_t buf_size = BUFLINE_SIZE;
    while (1) {
        errno = 0;
        int z = getline(&buf, &buf_size, f);
        if (z == -1 && errno != 0) {
            free(buf);
            fclose(f);
            panic("Error reading expense line");
        }
        if (z == -1)
            break;

        chomp(buf);
        read_expense_line(buf, &exp);
        bs_array_append(exps, &exp);
    }

    printf("List expenses...\n");
    for (int i=0; i < exps->len; i++) {
        Expense *p = bs_array_get(exps, i);
        printf("%d: %-12s %-35s %9.2f %-15s\n", i, p->date, p->desc, (float)(p->amt / 10), p->cat);
    }

    bs_array_free(exps);

    fclose(f);
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




