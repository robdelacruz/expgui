#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <errno.h>
#include <time.h>

#define countof(v, type) (sizeof(v) / sizeof(type))

void bs_qsort(int a[], size_t a_len);
void print_a(int a[], size_t a_len);
void gen_rand_array(int a[], size_t a_len, int max);

int main(int argc, char *argv[]) {
    srand(time(NULL));

    int a[100];
    size_t a_len = countof(a, int);

    gen_rand_array(a, a_len, 100);
    print_a(a, a_len);
    bs_qsort(a, a_len);
    print_a(a, a_len);
    printf("\n");

    return 0;
}

void print_a(int a[], size_t a_len) {
    printf("[");
    for (int i=0; i < a_len; i++) {
        printf("%d", a[i]);
        if (i < a_len-1)
            printf(", ");
    }
    printf("]\n");
}

void gen_rand_array(int a[], size_t a_len, int max) {
    for (int i=0; i < a_len; i++) {
        a[i] = (rand() % max) + 1;
    }
}

void swap(int a[], int i, int j) {
    int tmp = a[i];
    a[i] = a[j];
    a[j] = tmp;
}

static int partition(int a[], int start, int end) {
    int imid = start;
    int pivot = a[end];

    for (int i=start; i < end; i++) {
        if (a[i] < pivot) {
            swap(a, imid, i);
            imid++;
        }
    }
    swap(a, imid, end);
    return imid;
}

static void qsort_part(int a[], int start, int end) {
    if (start >= end)
        return;

    int pivot = partition(a, start, end);
    qsort_part(a, start, pivot-1);
    qsort_part(a, pivot+1, end);
}

void bs_qsort(int a[], size_t a_len) {
    qsort_part(a, 0, a_len-1);
}

