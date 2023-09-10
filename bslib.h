#ifndef BSLIB_H
#define BSLIB_H

#include <stdint.h>

typedef void (*BSDestroyFunc)(void *);
typedef void (*BSForeachFunc)(uint, void *);

/*** BSArray ***/
typedef struct {
    void *data;
    size_t item_size;
    size_t len;
    size_t size;
    BSDestroyFunc clearfunc;
} BSArray;

#define bs_array_type_new(type, init_size) \
    bs_array_new(sizeof(type), init_size)
BSArray *bs_array_new(size_t item_size, size_t init_size);
void bs_array_free(BSArray *a);
void bs_array_set_clear_func(BSArray *a, BSDestroyFunc clearfunc);
void bs_array_append(BSArray *a, void *item);
void *bs_array_get(BSArray *a, uint i);
void bs_array_foreach(BSArray *a, BSForeachFunc func);


/*** BSString ***/
typedef struct {
    char *s;
    size_t len;
    size_t size;
} BSString;

BSString *bs_string_new(char *s);
BSString *bs_string_size_new(size_t size);
void bs_string_free(BSString *str);
void bs_string_assign(BSString *str, char *s);
void bs_string_assign_sprintf(BSString *str, char *fmt, ...);
void bs_string_append(BSString *str, char *s);
void bs_string_append_sprintf(BSString *str, char *fmt, ...);
void bs_string_append_char(BSString *str, char c);

#endif

