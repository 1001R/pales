#ifndef DATABASE_H
#define DATABASE_H

#include "process.h"

struct database {
	const wchar_t *path;
};

typedef struct database database_t;

int db_open(const wchar_t *path, const wchar_t *procid);
int db_update(procstat_t status, ...);
int db_close();

#endif
