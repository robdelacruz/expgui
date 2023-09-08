#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

void quit(const char *s);

typedef struct {
    char *date;
    char *desc;
    uint32_t amt;
    char *cat;
} Expense;

typedef struct {
    size_t item_size;
    size_t len;
    size_t size;
    void *data;
} Array;

// Pointer to array element i.
#define ARRAY_ITEM_PTR(a, i) (a->data + ((i) * a->item_size))

// Set item to ith position in the array.
#define ARRAY_SET(a, i, item) memcpy(ARRAY_ITEM_PTR(a, i), item, a->item_size);

Array *array_new(size_t item_size, size_t init_size) {
    assert(item_size > 0);
    if (init_size == 0)
        init_size = 8;

    Array *a = malloc(sizeof(Array));
    a->item_size = item_size;
    a->len = 0;
    a->size = init_size;
    a->data = calloc(a->item_size, a->size);
    return a;
}

void array_append(Array *a, void *item) {
    assert(a->len <= a->size);

    a->len++;
    if (a->len > a->size) {
        a->size += 16;
        a->data = reallocarray(a->data, a->item_size, a->size);
        memset(ARRAY_ITEM_PTR(a, a->len), 0, (a->size - a->len) * a->item_size);
    }
    ARRAY_SET(a, a->len-1, item);
}

void *array_get(Array *a, uint i) {
    if (i >= a->len) {
        return NULL;
    }
    return ARRAY_ITEM_PTR(a, i);
}

int main(int argc, char *argv[]) {
    if (argc <= 1) {
        quit("No expense file arg.");
    }

    char *expfile = argv[1];
    printf("Expense file: %s\n", expfile);

    Array *expenses = array_new(sizeof(Expense), 0);
    Expense exp;

    exp.date = strdup("2023-09-02");
    exp.desc = strdup("rustans");
    exp.amt = 188050;   // 1880.50
    exp.cat = strdup("grocery");
    array_append(expenses, &exp);

    exp.date = strdup("2023-09-03");
    exp.desc = strdup("grab");
    exp.amt = 17200;   // 172.00
    exp.cat = strdup("commute");
    array_append(expenses, &exp);

    printf("List expenses...\n");
    for (int i=0; i < expenses->len; i++) {
        Expense *p = array_get(expenses, i);
        printf("%d: %12s %20s %9.2f %15s\n", i, p->date, p->desc, (float)(p->amt / 10), p->cat);
    }
}

void quit(const char *s) {
    printf("Error: %s\n", s);
    exit(1);
}




