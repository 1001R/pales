#include "database.h"
#include "process.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <windows.h>

#define SEPARATOR L'-'
#define PATHSEP L'\\'

int db_open(database_t *db, const wchar_t *path)
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

static wchar_t *process_encode(database_t *db, process_t *proc)
{
	wchar_t *filename, *s;
	size_t len;

	/*
	if (proc->status == running) {
		_snwprintf(pid, sizeof(pid) / sizeof(wchar_t), L"%ld", proc->pid);
	}
	if (proc->status == finished) {
		_snwprintf(exitcode, sizeof(exitcode) / sizeof(wchar_t), L"%ld", proc->exitcode);
	}
	*/
	len = wcslen(db->path) + 2 + wcslen(proc->id) + 3;
	/*
	if (proc->status == running) {
		len += wcslen(pid) + 1;
	}
	if (proc->status == finished) {
		len += wcslen(exitcode) + 1;
	}
	*/
	filename = (wchar_t *)malloc(len * sizeof(wchar_t));
	if (filename == NULL) {
		return NULL;
	}
	s = filename;
	wcscpy(s, db->path);
	s += wcslen(db->path);
	*s++ = PATHSEP;
	*s++ = L'0';
	wcscpy(s, proc->id);
	s += wcslen(proc->id);
	*s++ = L'.';
	switch (proc->status) {
	case running:
		*s++ = L'R';
		break;
	case finished:
		*s++ = L'F';
		break;
	case cancelled:
		*s++ = L'C';
		break;
	}
	*s = L'\0';
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

static int create_empty_file(const wchar_t *filepath)
{
	HANDLE h;
	/*
	wchar_t *path, *s;
	size_t len = 0;
	bool addpathsep = false;

	len = wcslen(directory);
	if (directory[len - 1] != PATHSEP) {
		addpathsep = true;
		len++;
	}
	len += wcslen(filename);
	if ((path = malloc((len + 1) * sizeof(wchar_t))) == NULL) {
		return -1;
	}
	s = path;
	wcscpy(s, directory);
	s += wcslen(directory);
	if (addpathsep) {
		*s++ = PATHSEP;
	}
	wcscpy(s, filename);
	*/
	h = CreateFile(filepath, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
	if (h == INVALID_HANDLE_VALUE) {
		return -1;
	}
	FlushFileBuffers(h);
	CloseHandle(h);
	return 0;
}

static int write_int_to_file(const wchar_t *filepath, DWORD pid)
{
	int retval = -1;
	HANDLE fileHandle;
	char buf[16];
	DWORD len, writeCount;

	len = _snprintf(buf, sizeof(buf), "%ld", pid);
	if (len < 0 || (len == sizeof(buf) && buf[len - 1] != '\0')) {
		return -1;
	}
	fileHandle = CreateFile(filepath, FILE_GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
	if (fileHandle == INVALID_HANDLE_VALUE) {
		return -1;
	}
	if (WriteFile(fileHandle, buf, len, &writeCount, NULL)) {
		retval = 0;
	}
	FlushFileBuffers(fileHandle);
	CloseHandle(fileHandle);
	return retval;
}

int db_update(database_t *db, process_t *proc)
{
	int rv;
	wchar_t *path = process_encode(db, proc);

	wchar_t *sep = wcsrchr(path, PATHSEP);

	if (proc->status == running) {
		sep[1] = L'1';
		write_int_to_file(path, proc->pid);
		sep[1] = L'0';
	}
	else if (proc->status == finished) {
		sep[1] = L'1';
		write_int_to_file(path, proc->exitcode);
		sep[1] = L'0';
	}

	rv = create_empty_file(path);
	free(path);
	return rv;
}
