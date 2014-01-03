#ifndef DATABASE_H
#define DATABASE_H


#include "process.h"
#include "error.h"
#include <stdbool.h>

struct database {
	const wchar_t *dbdir;
};

typedef struct database database_t;

int db_open(database_t *db, const wchar_t *path, error_t *err);
bool db_update(database_t *db, process_t *proc, error_t *err);
int db_close(database_t *db);
void db_error(const database_t *db, const process_t *proc, const error_t *error);

#endif
