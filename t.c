#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <errno.h>
#include <gtk/gtk.h>

#include "clib.h"
#include "exp.h"
#include "ui.h"

int main(int argc, char *argv[]) {
    uictx_t *ctx = uictx_new();
    int z;

    gtk_init(&argc, &argv);
    setupui(ctx);

    if (argc > 1) {
        z = open_expense_file(ctx, argv[1]);
        if (z != 0)
            print_error("Error reading expense file");
    }

    gtk_main();
    uictx_free(ctx);
    return 0;
}

