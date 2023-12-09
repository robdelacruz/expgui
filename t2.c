#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <errno.h>
#include <time.h>

#include "sqlite3.h"
#include "clib.h"

static void print_sql_err(char *sql, char *err);
static void print_sql_err_db(char *sql, sqlite3 *db);
int db_add_exp(sqlite3 *db, date_t *dt, char *desc, double amt, uint catid);

int main(int argc, char *argv[]) {
    int z;
    sqlite3 *db;
    char *dbfile;
    char *s;
    char *err;

    if (argc > 1)
        dbfile = argv[1];
    else
        dbfile = "myexpenses.db";

    z = sqlite3_open(dbfile, &db);
    if (z != 0) {
        fprintf(stderr, "Error opening dbfile '%s': %s\n", dbfile, sqlite3_errmsg(db));
        sqlite3_close_v2(db);
        return 1;
    }

    s = "CREATE TABLE IF NOT EXISTS cat (cat_id INTEGER PRIMARY KEY NOT NULL, name TEXT);"
        "CREATE TABLE IF NOT EXISTS exp (exp_id INTEGER PRIMARY KEY NOT NULL, date TEXT NOT NULL, desc TEXT NOT NULL DEFAULT '', amt REAL NOT NULL DEFAULT 0.0, cat_id INTEGER NOT NULL DEFAULT 1);";
    z = sqlite3_exec(db, s, 0, 0, &err);
    if (z != 0) {
        print_sql_err(s, err);
        sqlite3_free(err);
        sqlite3_close_v2(db);
        return 1;
    }

    date_t *dt = date_new(0);
    uint catid = 1;
    db_add_exp(db, dt, "expense 1", 123.34, catid);
    dt = date_new_today();
    catid = ((catid+1) % 5) + 1;
    db_add_exp(db, dt, "expense 2", 123.34, catid);
    catid = ((catid+1) % 5) + 1;
    db_add_exp(db, dt, "expense 3", 123.34, catid);

    sqlite3_close_v2(db);
    return 0;
}

static void print_sql_err(char *sql, char *err) {
    fprintf(stderr, "SQL input: %s\nError: %s\n", sql, err);
}
static void print_sql_err_db(char *sql, sqlite3 *db) {
    print_sql_err(sql, (char *) sqlite3_errmsg(db));
}

int db_add_exp(sqlite3 *db, date_t *dt, char *desc, double amt, uint catid) {
    sqlite3_stmt *stmt;
    char *s;
    int z;
    char isodate[ISO_DATE_LEN+1];

    s = "INSERT INTO exp (date, desc, amt, cat_id) VALUES (?, ?, ?, ?);";
    z = sqlite3_prepare_v2(db, s, -1, &stmt, 0);
    if (z != 0) {
        sqlite3_finalize(stmt);
        print_sql_err_db(s, db);
        return 1;
    }
    date_to_iso(dt, isodate, sizeof(isodate));
    z = sqlite3_bind_text(stmt, 1, isodate, -1, NULL);
    assert(z == 0);
    z = sqlite3_bind_text(stmt, 2, desc, -1, NULL);
    assert(z == 0);
    z = sqlite3_bind_double(stmt, 3, amt);
    assert(z == 0);
    z = sqlite3_bind_int(stmt, 4, catid);
    assert(z == 0);

    z = sqlite3_step(stmt);
    if (z == SQLITE_ERROR) {
        print_sql_err_db(s, db);
        sqlite3_finalize(stmt);
        return 1;
    }
    assert(z == SQLITE_DONE);

    sqlite3_finalize(stmt);
    return 0;
}

