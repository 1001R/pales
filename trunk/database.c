#include "database.h"
#include "process.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <windows.h>

#define SEPARATOR '-'

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
char pid[32];
	char *filename, *s;
	int len;

	if (proc->status == running) {
		snprintf(pid, sizeof(pid), "%ld", proc->pid);
	}
	
	len = strlen(db->path) + 1 + strlen(proc->id) + 3;
	if (proc->status == running) {
		len += strlen(pid) + 1;
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
		break;
	case cancelled:
		*s++ = 'C';
		break;
	}
	*s = '\0';
	return filename;
}

static process_t *process_decode(database_t *db, const char *path)
{
	char *s;
	process_t *proc;

	if (strncmp(db->path, path, strlen(db->path)) == 0) {
		path += strlen(db->path);
	}
	if (*path == '\\') {
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
	while (len >=0 && directory[len - 1] == '\\') {
		len--;
	}
	result = s = (char *)malloc(len + 1 + strlen(filename) + 1);
	if (result != NULL) {
		strncpy(s, directory, len);
		s += len;
		*s++ = '\\';
		strcpy(s, filename);
		s += strlen(filename);
		*s++ = '\0';
	}
	return result;
}

static int create_empty_file(const char *directory, const char *filename)
{
	char tmp_path[MAX_PATH + 1];
	char path[MAX_PATH + 1];
	int i;

	if (GetTempFileName(directory, ".PL", 0, tmp_path) == 0) {
		return -1;
	}
	strcpy(path, directory);
	i = strlen(directory);
	if (path[i] != '\\') {
		path[i++] = '\\';
	}
	strcpy(path + i, filename);
	return MoveFile(tmp_path, path) ? 0 : -1;
}

int db_update(database_t *db, process_t *proc)
{
	HANDLE h_find = INVALID_HANDLE_VALUE;
	WIN32_FIND_DATA d_find;
	int retval = -1, idlen;
	char *pattern;
	int len, i;
	bool separator = false, done;

	i = strlen(db->path);
	len = i + 2;
	if (db->path[len - 1] != '\\') {
		len++;
		separator = true;
	}
	pattern = malloc(len);
	if (pattern == NULL) {
		goto cleanup;
	}
	strcpy(pattern, db->path);
	if (separator) {
		pattern[i++] = '\\';
	}
	pattern[i++] = '*';
	pattern[i++] = '\0';

	idlen = strlen(proc->id);
	h_find = FindFirstFile(pattern, &d_find);
	if (INVALID_HANDLE_VALUE == h_find) {
		goto cleanup;
	}
	done = false;
   	do {
		char *frompath, *topath;
		if (d_find.cFileName[0] == '.') {
			continue;
		}
		process_t *p = process_decode(db, d_find.cFileName);
		if (p == NULL) {
			continue;
		}
		if (strcmp(p->id, proc->id) == 0) {
			if (p->status == proc->status) {
				retval = 0;
			}
			else {
				topath = process_encode(db, proc);
				frompath = path_join(db->path, d_find.cFileName);
				MoveFile(frompath, topath);
				free(frompath);
				free(topath);
				retval = 0;
			}
			done = true;
		}
		process_free(p);
   	}
   	while (!done && FindNextFile(h_find, &d_find) != 0);
	if (!done) {
		char *path = process_encode(db, proc);
		char *sep = strrchr(path, '\\');
		int rv;

		*sep = '\0';
		rv = create_empty_file(path, sep + 1);
		free(path);
		if (rv != 0) {
			goto cleanup;
		}
		else {
			retval = 0;
		}
	}
cleanup:
	if (pattern != NULL) {
		free(pattern);
	}
	if (h_find != INVALID_HANDLE_VALUE) {
		FindClose(h_find);
	}
	return retval;
}


