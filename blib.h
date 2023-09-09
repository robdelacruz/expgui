#ifndef BLIB_H
#define BLIB_H

#include <stdint.h>

/*** BArray ***/
typedef struct {
    void *data;
    size_t item_size;
    size_t len;
    size_t size;
} BArray;

#define b_array_type_new(type, init_size) \
    b_array_new(sizeof(type), init_size)
BArray *b_array_new(size_t item_size, size_t init_size);
void b_array_append(BArray *a, void *item);
void *b_array_get(BArray *a, uint i);

/*** BString ***/
typedef struct {
    char *s;
    size_t len;
    size_t size;
} BString;

#endif

