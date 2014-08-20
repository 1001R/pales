#ifndef DATABASE_H
#define DATABASE_H

typedef enum procstat {
    running,
    finished,
    cancelled,
    error
}
procstat_t;

int db_open(const char *path, const char *procid);
int db_update(procstat_t status, ...);
int db_close();

#endif
