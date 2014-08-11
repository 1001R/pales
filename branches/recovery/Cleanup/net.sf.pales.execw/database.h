#ifndef DATABASE_H
#define DATABASE_H

#include "process.h"

struct database {
	const char *path;
};

typedef struct database database_t;

int db_open(database_t *db, const char *path);
int db_update(database_t *db, process_t *proc);
int db_update_error(database_t *db, const char *id);
int db_close(database_t *db);

#endif
