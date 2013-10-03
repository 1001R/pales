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

static char *process_encode(database_t *db, process_t *proc, char *path, int pathlen)
{
	char pid[32];
	char *s;
	int len;

	if (proc->status == running) {
		snprintf(pid, sizeof(pid), "%ld", proc->pid);
	}
	
	len = strlen(db->path) + 1 + strlen(proc->id) + 3;
	if (proc->status == running) {
		len += strlen(pid) + 1;
	}
	if (len > pathlen) {
		return NULL;
	}
	/*
	filename = (char *)malloc(len);
	if (filename == NULL) {
		return NULL;
	}
	*/
	s = path;
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
		break;
	case cancelled:
		*s++ = 'C';
		break;
	}
	*s = '\0';
	return path;
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
	char path[256], *s;
	int len = 0;
	HANDLE h;
	bool addpathsep = false;

	len = strlen(directory);
	if (directory[len - 1] != PATHSEP) {
		addpathsep = true;
		len++;
	}
	len += strlen(filename);
	if (len + 1 > sizeof(path)) {
		return -1;
	}
	s = path;
	strcpy(s, directory);
	s += strlen(directory);
	if (addpathsep) {
		*s++ = PATHSEP;
	}
	strcpy(s, filename);
	h = CreateFile(path, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
	if (h == INVALID_HANDLE_VALUE) {
		return -1;
	}
	CloseHandle(h);
	return 0;
}

int db_update(database_t *db, process_t *proc)
{
	int rv;
	char path[256], *sep;

	if (process_encode(db, proc, path, sizeof(path)) == NULL) {
		return -1;
	}
	sep = strrchr(path, PATHSEP);

	*sep = '\0';
	rv = create_empty_file(path, sep + 1);
	return rv;
}

int db_update_error(database_t *db, const char *id)
{
	int rv;
	char *path; // = process_encode(db, proc);
	char *sep = strrchr(path, PATHSEP);

	char *filename, *s;
	int len;


	len = strlen(db->path) + 1 + strlen(id) + 3;
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
		break;
	case cancelled:
		*s++ = 'C';
		break;
	}
	*s = '\0';
	return filename;



	*sep = '\0';
	rv = create_empty_file(path, sep + 1);
	free(path);
	return rv;
}


