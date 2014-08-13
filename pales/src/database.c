#include "database.h"
#include "process.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <windows.h>

#define SEPARATOR L'-'
#define PATHSEP L'\\'

static const wchar_t *DB_DIR_PATH = NULL;
static const wchar_t *PROCESS_ID = NULL;

int db_open(const wchar_t *path, const wchar_t *procid)
{
	DWORD attr;

	assert(DB_DIR_PATH == NULL);
	assert(PROCESS_ID == NULL);
	assert(path != NULL);
	assert(procid != NULL);

	if ((attr = GetFileAttributes(path)) == INVALID_FILE_ATTRIBUTES) {
		return -1;
	}
 	if ((attr & FILE_ATTRIBUTE_DIRECTORY) == 0) {
		return -1;
	}
	DB_DIR_PATH = path;
	PROCESS_ID = procid;
	return 0;
}

int db_close() {
	return 0;
}

static wchar_t *process_encode(const wchar_t *procid, procstat_t status)
{
	wchar_t *filename, *s;
	size_t len;

	len = wcslen(DB_DIR_PATH) + 2 + wcslen(procid) + 3;
	filename = (wchar_t*) calloc(len, sizeof(wchar_t));
	if (filename == NULL) {
		return NULL;
	}
	s = filename;
	wcscpy(s, DB_DIR_PATH);
	s += wcslen(DB_DIR_PATH);
	*s++ = PATHSEP;
	*s++ = L'0';
	wcscpy(s, procid);
	s += wcslen(procid);
	*s++ = L'.';
	switch (status) {
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
		return -1;
	}
	fileHandle = CreateFile(filepath, FILE_GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
	if (fileHandle == INVALID_HANDLE_VALUE) {
		return -1;
	}
	if (WriteFile(fileHandle, buf, len, &writeCount, NULL)) {
		retval = FlushFileBuffers(fileHandle) ? 0 : -1;
	}
	CloseHandle(fileHandle);
	return retval;
}

int writeErrorMessage(const wchar_t *filePath, const wchar_t *format, va_list args)
{
	wchar_t *wbuf = NULL;
	va_list argsCopy;
	DWORD bufsz, writeCount, retval = -1;
	char *buf = NULL;
	HANDLE fileHandle = INVALID_HANDLE_VALUE;

	va_copy(argsCopy, args);
	int wlen = _vscwprintf(format, args);
	if ((wbuf = calloc(wlen + 1, sizeof(wchar_t))) == NULL) {
		va_end(argsCopy);
		goto cleanup;
	}
	vswprintf(wbuf, wlen + 1, format, argsCopy);
	va_end(argsCopy);

	bufsz = WideCharToMultiByte(CP_UTF8, 0, wbuf, wlen + 1, NULL, 0, NULL, FALSE);
	if ((buf = calloc(bufsz, sizeof(char))) == NULL) {
		goto cleanup;
	}
	if (!WideCharToMultiByte(CP_UTF8, 0, wbuf, wlen + 1, buf, bufsz, NULL, FALSE)) {
		goto cleanup;
	}

	fileHandle = CreateFile(filePath, FILE_GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_DELETE | FILE_SHARE_READ, NULL, OPEN_ALWAYS, 0, NULL);
	if (fileHandle == INVALID_HANDLE_VALUE) {
		goto cleanup;
	}
	if (!WriteFile(fileHandle, buf, bufsz - 1, &writeCount, NULL) || !FlushFileBuffers(fileHandle)) {
		goto cleanup;
	}
	if (!FlushFileBuffers(fileHandle)) {
		goto cleanup;
	}
	retval = 0;
cleanup:
	if (wbuf != NULL) {
		free(wbuf);
	}
	if (buf != NULL) {
		free(buf);
	}
	if (fileHandle != INVALID_HANDLE_VALUE) {
		CloseHandle(fileHandle);
	}
	return retval;
}

int db_update(procstat_t status, ...)
{
	int rv = -1;
	wchar_t *path = process_encode(PROCESS_ID, status);
	if (path == NULL) {
		return -1;
	}
	wchar_t *sep = wcsrchr(path, PATHSEP);
	if (status == running) {
		va_list args;
		DWORD pid;

		va_start(args, status);
		pid = va_arg(args, DWORD);
		va_end(args);
		sep[1] = L'1';
		if (write_int_to_file(path, pid) != 0) {
			goto cleanup;
		}
		sep[1] = L'0';
	}
	else if (status == finished) {
		va_list args;
		DWORD exitCode;

		va_start(args, status);
		exitCode = va_arg(args, DWORD);
		va_end(args);
		sep[1] = L'1';
		if (write_int_to_file(path, exitCode) != 0) {
			goto cleanup;
		}
		sep[1] = L'0';
	}
	if (status == error) {
		va_list args;
		const wchar_t *format;
		wchar_t *sep = wcsrchr(path, PATHSEP);
		sep[1] = L'1';
		va_start(args, status);
		format = va_arg(args, const wchar_t*);
		writeErrorMessage(path, format, args);
		va_end(args);
		sep[1] = L'0';
	}
	if (create_empty_file(path) != 0) {
		goto cleanup;
	}
	if (status == finished || status == cancelled || status == error) {
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

