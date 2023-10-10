#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <assert.h>
#include <errno.h>
#include <time.h>

#include "bslib.h"

static void panic_err(char *s) {
    if (s)
        fprintf(stderr, "%s: %s\n", s, strerror(errno));
    abort();
}
static void panic(char *s) {
    if (s)
        fprintf(stderr, "%s\n", s);
    abort();
}

static inline void memzero(void *p, size_t len) {
    memset(p, 0, len);
}

date_t default_date() {
    date_t dt = {.month = 1, .day = 1, .year = 1970};
    return dt;
}

/*** date_t functions ***/
static int is_valid_date(uint month, uint day, uint year) {
    if (month < 1 || month > 12)
        return 0;
    if (day < 1 || day > 31)
        return 0;
    if (year > 9999)
        return 0;
    return 1;
}

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
    return default_date();
}

date_t todays_date() {
    uint month=1, day=1, year=1970;
    date_t dt = {.year = year, .month = month, .day = day};
    return dt;
}

void format_date_iso(date_t dt, char buf[], size_t buf_len) {
    // 1900-01-01
    snprintf(buf, buf_len, "%04d-%02d-%02d", dt.year, dt.month, dt.day);
}

arena_t new_arena(uint64_t cap) {
    arena_t a; 

    if (cap == 0)
        cap = ARENA_DEFAULT;

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

str_t new_str(arena_t *arena, size_t cap) {
    str_t str;

    if (cap == 0)
        cap = STR_DEFAULT;

    str.arena = arena;
    str.s = arena_alloc(arena, cap);
    str.s[0] = 0;
    str.len = 0;
    str.cap = cap;

    return str;
}

void str_assign(str_t *str, char *s) {
    size_t s_len = strlen(s);
    if (s_len+1 > str->cap) {
        str->cap *= 2;
        str->s = arena_alloc(str->arena, str->cap);
    }

    strncpy(str->s, s, s_len);
    str->s[s_len] = 0;
    str->len = s_len;
}

