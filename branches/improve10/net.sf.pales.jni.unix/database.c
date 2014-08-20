#include "database.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>

#define PATHSEP '/'

static const char *DB_DIR_PATH = NULL;
static const char *PROCESS_ID = NULL;
static int DB_DIR_DESC = -1;

int db_open(const char *path, const char *procid)
{
    assert(DB_DIR_PATH == NULL);
    assert(PROCESS_ID == NULL);
    assert(DB_DIR_DESC == -1);
    assert(path != NULL);
    assert(procid != NULL);

    if ((DB_DIR_DESC = open(path, O_DIRECTORY)) == -1) {
	return -1;
    }
    DB_DIR_PATH = path;
    PROCESS_ID = procid;
    return 0;
}

static int write_int_to_file(const char *filepath, int num)
{
    int retval = -1;
    int fileHandle = -1;
    char buf[16];
    int len;

    len = snprintf(buf, sizeof(buf), "%d", num);
    if (len < 0 || len >= sizeof(buf)) {
        return -1;
    }

    if ((fileHandle = open(filepath, O_CREAT | O_EXCL | O_WRONLY, 0640)) == -1) {
	return -1;
    }
    if (write(fileHandle, buf, len) == len && fsync(fileHandle) == 0 && fsync(DB_DIR_DESC) == 0) {
        retval = 0;
    }
    close(fileHandle);
    return retval;
}

static char *process_encode(const char *procid, procstat_t status)
{
    char *filepath, *s;
    size_t len;

    len = strlen(DB_DIR_PATH) + 2 + strlen(procid) + 3;
    filepath = (char*) malloc(len);
    if (filepath == NULL) {
	return NULL;
    }
    s = filepath;
    strcpy(s, DB_DIR_PATH);
    s += strlen(DB_DIR_PATH);
    *s++ = PATHSEP;
    *s++ = '0';
    strcpy(s, procid);
    s += strlen(procid);
    *s++ = '.';
    switch (status) {
    case running:
	*s++ = 'R';
	break;
    case finished:
	*s++ = 'F';
	break;
    case cancelled:
	*s++ = 'C';
	break;
    case error:
	*s++ = 'E';
	break;
    }
    *s = '\0';
    return filepath;
}

static int create_empty_file(const char *filepath) {
    int fd = -1, retval = -1;

    fd = open(filepath, O_CREAT | O_EXCL | O_WRONLY, 0640);
    if (fd == -1) {
	return -1;
    }
    if (fsync(fd) == 0 && fsync(DB_DIR_DESC) == 0) {
	retval = 0;
    }
    close(fd);
    return retval;
}

int writeErrorMessage(const char *filePath, const char *format, va_list args)
{
    va_list argsCopy;
    ssize_t bufsz;
    int fileHandle, retval = -1;
    char *buf = NULL;

    va_copy(argsCopy, args);
    bufsz = vsnprintf(NULL, 0, format, args) + 1;
    if ((buf = (char*) malloc(bufsz)) == NULL) {
	return -1;
    }
    vsnprintf(buf, bufsz, format, argsCopy);
    va_end(argsCopy);

    if ((fileHandle = open(filePath, O_CREAT | O_APPEND | O_WRONLY, 0640)) == -1) {
	free(buf);
	return -1;
    }
    if (write(fileHandle, buf, bufsz - 1) == bufsz - 1 && fsync(fileHandle) == 0 && fsync(DB_DIR_DESC) == 0) {
	retval = 0;
    }
    free(buf);
    return retval;
}

int db_update(procstat_t status, ...)
{
    int rv = -1;
    char *path = process_encode(PROCESS_ID, status);

    if (path == NULL) {
	return -1;
    }
    char *sep = strrchr(path, PATHSEP);
    if (status == running) {
	va_list args;
	pid_t pid;

	va_start(args, status);
	pid = va_arg(args, pid_t);
	va_end(args);
	sep[1] = L'1';
	if (write_int_to_file(path, pid) != 0) {
	    goto cleanup;
	}
	sep[1] = L'0';
    }
    else if (status == finished) {
	va_list args;
	int exitCode;

	va_start(args, status);
	exitCode = va_arg(args, int);
	va_end(args);
	sep[1] = L'1';
	if (write_int_to_file(path, exitCode) != 0) {
	    goto cleanup;
	}
	sep[1] = L'0';
    }
    if (status == error) {
	va_list args;
	const char *format;
	sep[1] = L'1';
	va_start(args, status);
	format = va_arg(args, const char*);
	writeErrorMessage(path, format, args);
	va_end(args);
	sep[1] = L'0';
    }
    if (create_empty_file(path) != 0) {
	goto cleanup;
    }
    if (status == finished || status == cancelled || status == error) {
	path[strlen(path) - 1] = 'X';
	rv = create_empty_file(path);
    }
    else {
	rv = 0;
    }
cleanup:
    free(path);
    return rv;
}

int db_close() {
    return close(DB_DIR_DESC);
}
