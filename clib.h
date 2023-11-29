#ifndef CLIB_H
#define CLIB_H

#include <stdint.h>

#define countof(v) (sizeof(v) / sizeof((v)[0]))
#define memzero(p, v) (memset(p, 0, sizeof(v)))

#define SIZE_MB      1024*1024
#define SIZE_TINY    512
#define SIZE_SMALL   1024
#define SIZE_MEDIUM  32768
#define SIZE_LARGE   (1024*1024)
#define SIZE_HUGE    (1024*1024*1024)

#define ISO_DATE_LEN 10

typedef struct {
    void *base;
    uint64_t pos;
    uint64_t cap;
} arena_t;

typedef struct {
    char *s;
    size_t len;
    size_t cap;
} str_t;

typedef struct {
    uint month;
    uint day;
    uint year;
} date_t;

typedef struct {
    void **items;
    size_t len;
    size_t cap;
} array_t;

typedef struct {
    int *items;
    size_t len;
    size_t cap;
} intarray_t;

void quit(const char *s);
void print_error(const char *s);
void panic(char *s);
void panic_err(char *s);

typedef void *(allocfn_t)(size_t size);
arena_t new_arena(uint64_t cap);
void free_arena(arena_t a);
void *arena_alloc(arena_t *a, uint64_t size);
void arena_reset(arena_t *a);

str_t *str_new(size_t cap);
void str_free(str_t *str);
void str_assign(str_t *str, char *s);

date_t date_zero();
date_t date_default();
date_t date_current();
int date_is_zero(date_t dt);
date_t date_from_iso(char *s);
void date_to_iso(date_t dt, char buf[], size_t buf_len);

array_t *array_new(size_t cap);
void array_free(array_t *a);
void array_assign(array_t *a, void **items, size_t len, size_t cap);
void array_clear(array_t *a);

intarray_t *intarray_new(size_t cap);
void intarray_free(intarray_t *a);
void intarray_assign(intarray_t *a, int *items, size_t len, size_t cap);
void intarray_clear(intarray_t *a);

typedef int (*CompareFunc)(void *a, void *b);
void sort_array(void *array[], size_t array_len, CompareFunc compare_func);

#endif
