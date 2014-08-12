#include "database.h"
#include "process.h"
#include "log.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <windows.h>

#define SEPARATOR L'-'
#define PATHSEP L'\\'

int db_open(database_t *db, const wchar_t *path)
{
	DWORD attr;

	memset(db, 0, sizeof(*db));
	assert(path != NULL);
	if ((attr = GetFileAttributes(path)) == INVALID_FILE_ATTRIBUTES) {
		log_message(L"Database directory does not exist: %s", path);
		return -1;
	}
 	if ((attr & FILE_ATTRIBUTE_DIRECTORY) == 0) {
		log_message(L"Database directory path does not point to a directory: %s", path);
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

	len = wcslen(db->path) + 2 + wcslen(proc->id) + 3;
	filename = (wchar_t *)malloc(len * sizeof(wchar_t));
	if (filename == NULL) {
		log_message(L"Out of memory");
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

static int create_empty_file(const wchar_t *filepath)
{
	HANDLE h;
	h = CreateFile(filepath, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
	if (h == INVALID_HANDLE_VALUE) {
		log_message(L"Cannot create empty file: %s", filepath);
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
		log_message(L"String representation of %d too long", pid);
		return -1;
	}
	fileHandle = CreateFile(filepath, FILE_GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
	if (fileHandle == INVALID_HANDLE_VALUE) {
		log_message(L"Cannot create file and open it for writing: %s", filepath);
		return -1;
	}
	if (WriteFile(fileHandle, buf, len, &writeCount, NULL)) {
		retval = 0;
	}
	else {
		log_message(L"Cannot write to file: %s", filepath);
	}
	FlushFileBuffers(fileHandle);
	CloseHandle(fileHandle);
	return retval;
}

int db_update(database_t *db, process_t *proc)
{
	int rv = -1;
	wchar_t *path = process_encode(db, proc);
	if (path == NULL) {
		return -1;
	}
	wchar_t *sep = wcsrchr(path, PATHSEP);
	if (proc->status == running) {
		sep[1] = L'1';
		if (write_int_to_file(path, proc->pid) != 0) {
			goto cleanup;
		}
		sep[1] = L'0';
	}
	else if (proc->status == finished) {
		sep[1] = L'1';
		if (write_int_to_file(path, proc->exitcode) != 0) {
			goto cleanup;
		}
		sep[1] = L'0';
	}
	if (create_empty_file(path) != 0) {
		goto cleanup;
	}
	if (proc->status == finished || proc->status == cancelled) {
		path[wcslen(path) - 1] = 'X';
		rv = create_empty_file(path);
	}
	else {
		rv = 0;
	}
cleanup:
	free(path);
	return rv;
}
