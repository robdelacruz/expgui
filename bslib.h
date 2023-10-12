#ifndef BSLIB_H
#define BSLIB_H

#include <stdint.h>

#define countof(v) (sizeof(v) / sizeof((v)[0]))

#define MB_BYTES      1024*1024
#define ARENA_SMALL   1024
#define ARENA_MEDIUM  32768
#define ARENA_LARGE   (1024*1024)
#define ARENA_HUGE    (1024*1024*1024)
#define ARENA_DEFAULT ARENA_MEDIUM

#define STR_TINY    512
#define STR_SMALL   1024
#define STR_MEDIUM  32768
#define STR_LARGE   (1024*1024)
#define STR_DEFAULT STR_SMALL

#define ISO_DATE_LEN 10

typedef struct {
    void *base;
    uint64_t pos;
    uint64_t cap;
} arena_t;

typedef struct {
    arena_t *arena;
    char *s;
    size_t len;
    size_t cap;
} str_t;

typedef struct {
    uint month;
    uint day;
    uint year;
} date_t;

date_t default_date();
date_t date_from_iso(char *s);
date_t todays_date();
void format_date_iso(date_t dt, char buf[], size_t buf_len);

arena_t new_arena(uint64_t cap);
void free_arena(arena_t a);
void *arena_alloc(arena_t *a, uint64_t size);
void arena_reset(arena_t *a);

str_t new_str(arena_t *arena, size_t cap);
void str_assign(str_t *str, char *s);

#endif

