#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <assert.h>

#include "bslib.h"

/*** BSArray ***/
BSArray *bs_array_new(size_t item_size, size_t init_size) {
    assert(item_size > 0);
    if (init_size == 0)
        init_size = 8;

    BSArray *a = bs_malloc(sizeof(BSArray));
    a->item_size = item_size;
    a->len = 0;
    a->size = init_size;
    a->data = bs_calloc(a->item_size, a->size);
    a->clearfunc = NULL;
    return a;
}

void bs_array_free(BSArray *a) {
    if (a->clearfunc) {
        void *pitem = a->data;
        for (int i=0; i < a->len; i++) {
            (*a->clearfunc)(pitem);
            pitem += a->item_size;
        }
    }
    bs_free(a->data);
    bs_free(a);
}

void bs_array_set_clear_func(BSArray *a, BSDestroyFunc clearfunc) {
    a->clearfunc = clearfunc;
}

// Return ptr to a->data element i
static inline void *bs_array_data_offset(BSArray *a, uint i) {
    return a->data + (i * a->item_size);
}

// Copy pitem into a->data element i
static inline void bs_array_set_data_item(BSArray *a, uint i, void *pitem) {
    memcpy(bs_array_data_offset(a, i), pitem, a->item_size);
}

// Append item, allocating more memory as needed.
void bs_array_append(BSArray *a, void *item) {
    assert(a->len <= a->size);

    a->len++;
    if (a->len > a->size) {
        a->size += 16;
        a->data = bs_reallocarray(a->data, a->item_size, a->size);
        memset(bs_array_data_offset(a, a->len), 0, (a->size - a->len) * a->item_size);
    }
    bs_array_set_data_item(a, a->len-1, item);
}

// Return ptr to item at index i.
void *bs_array_get(BSArray *a, uint i) {
    if (i >= a->len) {
        return NULL;
    }
    return bs_array_data_offset(a, i);
}

// Run function for each array item.
void bs_array_foreach(BSArray *a, BSForeachFunc func) {
    void *pitem = a->data;
    for (int i=0; i < a->len; i++) {
        (*func)(i, pitem);
        pitem += a->item_size;
    }
}

/*** BSString ***/
static inline void zero_s(BSString *str) {
    memset(str->s, 0, str->size+1);
}
// Zero out the free unused space
static inline void zero_unused_s(BSString *str) {
    memset(str->s + str->len, 0, str->size+1 - str->len);
}

BSString *bs_string_new(char *s) {
    if (s == NULL) {
        s = "";
    }
    size_t s_len = strlen(s);

    BSString *str = bs_malloc(sizeof(BSString));
    str->len = s_len;
    str->size = s_len;
    str->s = bs_malloc(str->size+1);
    zero_s(str);
    strcpy(str->s, s);

    return str;
}
BSString *bs_string_size_new(size_t size) {
    BSString *str = bs_malloc(sizeof(BSString));

    str->len = 0;
    str->size = size;
    str->s = bs_malloc(str->size+1);
    zero_s(str);

    return str;
}
void bs_string_free(BSString *str) {
    assert(str->s != NULL);

    zero_s(str);
    bs_free(str->s);
    str->s = NULL;
    bs_free(str);
}
void bs_string_assign(BSString *str, char *s) {
    size_t s_len = strlen(s);
    if (s_len > str->size) {
        str->size = s_len;
        str->s = bs_realloc(str->s, str->size+1);
    }
    zero_s(str);
    strcpy(str->s, s);
    str->len = s_len;
}

void bs_string_assign_sprintf(BSString *str, char *fmt, ...) {
    va_list args;
    char *ps;
    va_start(args, fmt);
    int z = vasprintf(&ps, fmt, args);
    va_end(args);
    if (z == -1)
        return;

    bs_string_assign(str, ps);
    bs_free(ps);
}

void bs_string_append_sprintf(BSString *str, char *fmt, ...) {
    va_list args;
    char *ps;
    va_start(args, fmt);
    int z = vasprintf(&ps, fmt, args);
    va_end(args);
    if (z == -1) return;

    bs_string_append(str, ps);
    bs_free(ps);
}

void bs_string_append(BSString *str, char *s) {
    size_t s_len = strlen(s);
    if (str->len + s_len > str->size) {
        str->size = str->len + s_len;
        str->s = bs_realloc(str->s, str->size+1);
        zero_unused_s(str);
    }

    strncpy(str->s + str->len, s, s_len);
    str->len = str->len + s_len;
}

void bs_string_append_char(BSString *str, char c) {
    if (str->len + 1 > str->size) {
        // Grow string by ^2
        str->size = str->len + ((str->len+1) * 2);
        str->s = bs_realloc(str->s, str->size+1);
        zero_unused_s(str);
    }

    str->s[str->len] = c;
    str->s[str->len+1] = '\0';
    str->len++;
}


