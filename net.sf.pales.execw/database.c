#include "database.h"
#include "process.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <windows.h>

#define SEPARATOR '-'
#define PATHSEP '\\'

int db_open(database_t *db, const char *path)
{
	DWORD attr;

	memset(db, 0, sizeof(*db));
	if (path == NULL) {
		return -1;
	}
	if ((attr = GetFileAttributes(path)) == INVALID_FILE_ATTRIBUTES) {
		return -1;
	}
 	if ((attr & FILE_ATTRIBUTE_DIRECTORY) == 0) {
		return -1;
	}
	db->path = path;
	return 0;
}

int db_close(database_t *db)
{
	return 0;
}

static char *process_encode(database_t *db, process_t *proc)
{
	char pid[32], exitcode[32];
	char *filename, *s;
	int len;

	if (proc->status == running) {
		_snprintf(pid, sizeof(pid), "%ld", proc->pid);
	}
	if (proc->status == finished) {
		_snprintf(exitcode, sizeof(exitcode), "%ld", proc->exitcode);
	}
	
	len = strlen(db->path) + 1 + strlen(proc->id) + 3;
	if (proc->status == running) {
		len += strlen(pid) + 1;
	}
	if (proc->status == finished) {
		len += strlen(exitcode) + 1;
	}
	filename = (char *)malloc(len);
	if (filename == NULL) {
		return NULL;
	}
	s = filename;
	strcpy(s, db->path);
	s += strlen(db->path);
	*s++ = '\\';
	strcpy(s, proc->id);
	s += strlen(proc->id);
	*s++ = SEPARATOR;
	switch (proc->status) {
	case running:
		*s++ = 'R';
		*s++ = SEPARATOR;
		strcpy(s, pid);
		s += strlen(pid);
		break;
	case finished:
		*s++ = 'F';
		*s++ = SEPARATOR;
		strcpy(s, exitcode);
		s += strlen(exitcode);
		break;
	case cancelled:
		*s++ = 'C';
		break;
	}
	*s = '\0';
	return filename;
}

/*
static process_t *process_decode(database_t *db, const char *path)
{
	char *s;
	process_t *proc;

	if (strncmp(db->path, path, strlen(db->path)) == 0) {
		path += strlen(db->path);
	}
	if (*path == PATHSEP) {
		path++;
	}
	s = strchr(path, SEPARATOR);
	if (s == NULL) {
		return NULL;
	}

	if (process_new(path, s - path, &proc) != 0) {
		return NULL;
	}
	s++;
	switch (*s) {
	case 'R':
		proc->status = running;
		break;
	case 'F':
		break;
	case 'C':
		break;
	}
	return proc;
}

static char *path_join(const char *directory, const char *filename)
{
	char *s;
	char *result;
	int len;

	len = strlen(directory);
	while (len >=0 && directory[len - 1] == PATHSEP) {
		len--;
	}
	result = s = (char *)malloc(len + 1 + strlen(filename) + 1);
	if (result != NULL) {
		strncpy(s, directory, len);
		s += len;
		*s++ = PATHSEP;
		strcpy(s, filename);
		s += strlen(filename);
		*s++ = '\0';
	}
	return result;
}
*/

static int create_empty_file(const char *directory, const char *filename)
{
	char *path, *s;
	int len = 0;
	HANDLE h;
	bool addpathsep = false;

	len = strlen(directory);
	if (directory[len - 1] != PATHSEP) {
		addpathsep = true;
		len++;
	}
	len += strlen(filename);
	if ((path = malloc(len + 1)) == NULL) {
		return -1;
	}
	s = path;
	strcpy(s, directory);
	s += strlen(directory);
	if (addpathsep) {
		*s++ = PATHSEP;
	}
	strcpy(s, filename);
	h = CreateFile(path, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
	free(path);
	if (h == INVALID_HANDLE_VALUE) {
		return -1;
	}
	CloseHandle(h);
	return 0;
}

int db_update(database_t *db, process_t *proc)
{
	int rv;
	char *path = process_encode(db, proc);
	char *sep = strrchr(path, PATHSEP);

	*sep = '\0';
	rv = create_empty_file(path, sep + 1);
	free(path);
	return rv;
}


