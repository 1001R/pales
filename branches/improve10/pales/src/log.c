#include <Windows.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include "win32.h"
#include "log.h"

static const wchar_t *DB_DIR_PATH = NULL;
static const wchar_t *PROCESS_ID = NULL;

void log_init(const wchar_t *dbDirPath, const wchar_t *processId) {
	DB_DIR_PATH = dbDirPath;
	PROCESS_ID = processId;
}

static wchar_t *createErrorDataFilePath() {
	wchar_t *path, *s;
	bool sepNeeded = false;
	int len = wcslen(DB_DIR_PATH);
	sepNeeded = DB_DIR_PATH[len - 1] != PATHSEP;
	if (sepNeeded) {
		len++;
	}
	len += wcslen(PROCESS_ID) + 4;
	path = calloc(len, sizeof(wchar_t));
	if (path == NULL) {
		return NULL;
	}
	s = wcpcpy(path, DB_DIR_PATH);
	if (sepNeeded) {
		*s++ = PATHSEP;
	}
	*s++ = L'1';
	s = wcpcpy(s, PROCESS_ID);
	s = wcpcpy(s, L".E");
	return path;
}

static void convertToStatusFilePath(wchar_t *dataFilePath) {
	wchar_t *sep = wcsrchr(dataFilePath, PATHSEP);
	sep[1] = L'0';
}

int log_message(const wchar_t *message, ...)
{
	wchar_t *wbuf = NULL, *filePath = NULL;
	va_list args;
	DWORD bufsz, writeCount, retval = -1;
	char *buf = NULL;
	HANDLE fileHandle = INVALID_HANDLE_VALUE;

	va_start(args, message);
	int wlen = _vscwprintf(message, args);
	va_end(args);
	if ((wbuf = calloc(wlen + 1, sizeof(wchar_t))) == NULL) {
		goto cleanup;
	}
	va_start(args, message);
	vswprintf(wbuf, wlen + 1, message, args);
	va_end(args);

	bufsz = WideCharToMultiByte(CP_UTF8, 0, wbuf, wlen + 1, NULL, 0, NULL, FALSE);
	if ((buf = calloc(bufsz, sizeof(char))) == NULL) {
		goto cleanup;
	}
	if (!WideCharToMultiByte(CP_UTF8, 0, wbuf, wlen + 1, buf, bufsz, NULL, FALSE)) {
		goto cleanup;
	}

	if ((filePath = createErrorDataFilePath()) == NULL) {
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
	CloseHandle(fileHandle);
	fileHandle = INVALID_HANDLE_VALUE;
	convertToStatusFilePath(filePath);
	if (CreateEmptyFile(filePath) != 0) {
		goto cleanup;
	}
	filePath[wcslen(filePath) - 1] = 'X';
	if (CreateEmptyFile(filePath) != 0) {
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