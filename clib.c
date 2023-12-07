#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <assert.h>
#include <errno.h>
#include <time.h>

#include "clib.h"

void quit(const char *s) {
    if (s)
        printf("%s\n", s);
    exit(0);
}
void print_error(const char *s) {
    if (s)
        fprintf(stderr, "%s: %s\n", s, strerror(errno));
    else
        fprintf(stderr, "%s\n", strerror(errno));
}
void panic(char *s) {
    if (s)
        fprintf(stderr, "%s\n", s);
    abort();
}
void panic_err(char *s) {
    if (s)
        fprintf(stderr, "%s: %s\n", s, strerror(errno));
    abort();
}

date_t *date_new() {
    date_t *dt = malloc(sizeof(date_t));
    dt->tm = malloc(sizeof(struct tm));

    dt->time = 0;
    localtime_r(&dt->time, dt->tm);
    return dt;
}
date_t *date_new_current() {
    date_t *dt = malloc(sizeof(date_t));
    dt->tm = malloc(sizeof(struct tm));

    dt->time = time(NULL);
    localtime_r(&dt->time, dt->tm);
    return dt;
}
void date_free(date_t *dt) {
    free(dt->tm);
    free(dt);
}
static void date_copy_tm(struct tm *dest, struct tm *src) {
    dest->tm_sec = src->tm_sec;
    dest->tm_min = src->tm_min;
    dest->tm_hour = src->tm_hour;
    dest->tm_mday = src->tm_mday;
    dest->tm_mon = src->tm_mon;
    dest->tm_year = src->tm_year;
    dest->tm_wday = src->tm_wday;
    dest->tm_yday = src->tm_yday;
    dest->tm_isdst = src->tm_isdst;
}
void date_assign_time(date_t *dt, time_t time) {
    dt->time = time;
    localtime_r(&dt->time, dt->tm);
}
void date_assign(date_t *dt, uint year, uint month, uint day) {
    time_t time;
    struct tm tm;
    memset(&tm, 0, sizeof(struct tm));

    tm.tm_year = year - 1900;
    tm.tm_mon = month-1;
    tm.tm_mday = day;
    time = mktime(&tm);
    if (time == -1) {
        fprintf(stderr, "date_assign(%d, %d, %d) mktime() error\n", year, month, day);
        return;
    }

    dt->time = time;
    date_copy_tm(dt->tm, &tm);
}
void date_assign_iso(date_t *dt, char *isodate) {
    time_t time;
    struct tm tm;
    memset(&tm, 0, sizeof(struct tm));

    if (strptime(isodate, "%F", &tm) == NULL) {
        fprintf(stderr, "date_assign_iso('%s') strptime() error\n", isodate);
        return;
    }
    time = mktime(&tm);
    if (time == -1) {
        fprintf(stderr, "date_assign_iso('%s') mktime() error\n", isodate);
        return;
    }

    dt->time = time;
    date_copy_tm(dt->tm, &tm);
}
void date_to_iso(date_t *dt, char *buf, size_t buf_len) {
    strftime(buf, buf_len, "%F", dt->tm);
}
void date_strftime(date_t *dt, char *fmt, char *buf, size_t buf_len) {
    strftime(buf, buf_len, fmt, dt->tm);
}
void date_dup(date_t *dest, date_t *src) {
    dest->time = src->time;
    date_copy_tm(dest->tm, src->tm);
}
time_t date_time(date_t *dt) {
    return dt->time;
}
int date_year(date_t *dt) {
    return dt->tm->tm_year + 1900;
}
int date_month(date_t *dt) {
    return dt->tm->tm_mon+1;
}
int date_day(date_t *dt) {
    return dt->tm->tm_mday;
}

#if 0
date_t date_from_iso(char *s) {
    char buf[11];
    uint month, day, year;

    // s should be yyyy-mm-dd format
    if (strlen(s) != 10)
        goto error;
    if (s[4] != '-' && s[7] != '-')
        goto error;

    strcpy(buf, s);
    buf[4] = 0;
    buf[7] = 0;
    year = atoi(buf+0);
    month = atoi(buf+5);
    day = atoi(buf+8);

    if (!is_valid_date(month, day, year))
        goto error;

    date_t dt = {.year = year, .month = month, .day = day};
    return dt;

error:
    return date_zero();
}

void date_to_iso(date_t dt, char buf[], size_t buf_len) {
    // 1900-01-01
    snprintf(buf, buf_len, "%04d-%02d-%02d", dt.year, dt.month, dt.day);
}
#endif

arena_t new_arena(uint64_t cap) {
    arena_t a; 

    if (cap == 0)
        cap = SIZE_MEDIUM;

    a.base = malloc(cap);
    if (!a.base)
        panic("Not enough memory to initialize arena");

    a.pos = 0;
    a.cap = cap;
    return a;
}

void free_arena(arena_t a) {
    free(a.base);
}

void *arena_alloc(arena_t *a, uint64_t size) {
    if (a->pos + size > a->cap)
        panic("arena_alloc() not enough memory");

    void *p = a->base + a->pos;
    a->pos += size;
    return p;
}

void arena_reset(arena_t *a) {
    a->pos = 0;
}

str_t *str_new(size_t cap) {
    str_t *str;

    if (cap == 0)
        cap = SIZE_SMALL;

    str = malloc(sizeof(str_t));
    str->s = malloc(cap);
    memset(str->s, 0, cap);
    str->len = 0;
    str->cap = cap;

    return str;
}
void str_free(str_t *str) {
    memset(str->s, 0, str->cap);
    free(str->s);
    free(str);
}

void str_assign(str_t *str, char *s) {
    size_t s_len = strlen(s);
    if (s_len+1 > str->cap) {
        str->cap *= 2;
        str->s = malloc(str->cap);
    }

    strncpy(str->s, s, s_len);
    str->s[s_len] = 0;
    str->len = s_len;
}

array_t *array_new(size_t cap) {
    array_t *a = malloc(sizeof(array_t));
    a->items = malloc(sizeof(a->items[0]) * cap);
    a->len = 0;
    a->cap = cap;
    return a;
}
void array_free(array_t *a) {
    memset(a->items, 0, a->cap);
    free(a->items);
    free(a);
}
void array_assign(array_t *a, void **items, size_t len, size_t cap) {
    a->items = items;
    a->len = len;
    a->cap = cap;
}
void array_clear(array_t *a) {
    memset(a->items, 0, a->cap);
    a->len = 0;
}
intarray_t *intarray_new(size_t cap) {
    intarray_t *a = malloc(sizeof(intarray_t));
    a->items = malloc(sizeof(a->items[0]) * cap);
    a->len = 0;
    a->cap = cap;
    return a;
}
void intarray_free(intarray_t *a) {
    memset(a->items, 0, a->cap);
    free(a->items);
    free(a);
}
void intarray_assign(intarray_t *a, int *items, size_t len, size_t cap) {
    a->items = items;
    a->len = len;
    a->cap = cap;
}
void intarray_clear(intarray_t *a) {
    memset(a->items, 0, a->cap);
    a->len = 0;
}

// sort_array() implementation functions
static void swap_array(void *array[], int i, int j) {
    void *tmp = array[i];
    array[i] = array[j];
    array[j] = tmp;
}
static int sort_array_partition(void *array[], int start, int end, sort_compare_func_t cmp) {
    int imid = start;
    void *pivot = array[end];

    for (int i=start; i < end; i++) {
        if (cmp(array[i], pivot) < 0) {
            swap_array(array, imid, i);
            imid++;
        }
    }
    swap_array(array, imid, end);
    return imid;
}
static void sort_array_part(void *array[], int start, int end, sort_compare_func_t cmp) {
    if (start >= end)
        return;

    int pivot = sort_array_partition(array, start, end, cmp);
    sort_array_part(array, start, pivot-1, cmp);
    sort_array_part(array, pivot+1, end, cmp);
}
void sort_array(void *array[], size_t array_len, sort_compare_func_t cmpfunc) {
    sort_array_part(array, 0, array_len-1, cmpfunc);
}

