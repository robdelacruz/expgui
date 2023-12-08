#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <errno.h>
#include <time.h>

#include "sqlite3.h"
#include "clib.h"

int open_db_file(char *dbfile, sqlite3 **db);

int main(int argc, char *argv[]) {
    int z;
    sqlite3 *db;
    char *dbfile;

    if (argc > 1)
        dbfile = argv[1];
    else
        dbfile = "myexpenses.db";

    if (open_db_file(dbfile, &db) != 0)
        return 1;

    return 0;
}

int open_db_file(char *dbfile, sqlite3 **db) {
    int z = sqlite3_open(dbfile, db);
    if (z != SQLITE_OK) {
        fprintf(stderr, "Error opening dbfile '%s': %s\n", dbfile, sqlite3_errmsg(*db));
        sqlite3_close(*db);
        return 1;
    }
    return 0;
}

