#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#include "blib.h"

/*** BArray ***/
BArray *b_array_new(size_t item_size, size_t init_size) {
    assert(item_size > 0);
    if (init_size == 0)
        init_size = 8;

    BArray *a = malloc(sizeof(BArray));
    a->item_size = item_size;
    a->len = 0;
    a->size = init_size;
    a->data = calloc(a->item_size, a->size);
    return a;
}

// Return ptr to a->data element i
static inline void *b_array_data_offset(BArray *a, uint i) {
    return a->data + (i * a->item_size);
}

// Copy pitem into a->data element i
static inline void b_array_set_data_item(BArray *a, uint i, void *pitem) {
    memcpy(b_array_data_offset(a, i), pitem, a->item_size);
}

// Append item, expanding array memory as needed.
void b_array_append(BArray *a, void *item) {
    assert(a->len <= a->size);

    a->len++;
    if (a->len > a->size) {
        a->size += 16;
        a->data = reallocarray(a->data, a->item_size, a->size);
        memset(b_array_data_offset(a, a->len), 0, (a->size - a->len) * a->item_size);
    }
    b_array_set_data_item(a, a->len-1, item);
}

// Return ptr to item at index i.
void *b_array_get(BArray *a, uint i) {
    if (i >= a->len) {
        return NULL;
    }
    return b_array_data_offset(a, i);
}


