#ifndef DATABASE_H
#define DATABASE_H

#include "process.h"

struct database {
	const wchar_t *path;
};

typedef struct database database_t;

int db_open(database_t *db, const wchar_t *path);
int db_update(database_t *db, process_t *proc);
int db_close(database_t *db);

#endif
