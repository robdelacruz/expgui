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
    time_t time;
    struct tm *tm;
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

date_t *date_new(time_t t);
date_t *date_new_today();
date_t *date_new_cal(uint year, uint month, uint day);
date_t *date_new_iso(char *isodate);
void date_free(date_t *dt);
void date_assign_time(date_t *dt, time_t time);
int date_assign_cal(date_t *dt, uint year, uint month, uint day);
int date_assign_iso(date_t *dt, char *isodate);
void date_to_iso(date_t *dt, char *buf, size_t buf_len);
void date_strftime(date_t *dt, char *fmt, char *buf, size_t buf_len);
void date_dup(date_t *dest, date_t *src);
time_t date_time(date_t *dt);
int date_year(date_t *dt);
int date_month(date_t *dt);
int date_day(date_t *dt);

array_t *array_new(size_t cap);
void array_free(array_t *a);
void array_assign(array_t *a, void **items, size_t len, size_t cap);
void array_clear(array_t *a);
int array_add(array_t *a, void *p);
void array_del(array_t *a, uint idx);

intarray_t *intarray_new(size_t cap);
void intarray_free(intarray_t *a);
void intarray_assign(intarray_t *a, int *items, size_t len, size_t cap);
void intarray_clear(intarray_t *a);

typedef int (*sort_compare_func_t)(void *a, void *b);
void sort_array(void *array[], size_t array_len, sort_compare_func_t cmpfunc);

#endif
