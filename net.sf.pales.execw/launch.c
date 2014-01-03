#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <windows.h>
#include "options.h"
#include "process.h"
#include "database.h"
#include "log.h"
#include "error.h"

#define WIDEN2(x) L ## x
#define WIDEN(x) WIDEN2(x)
#define WFILE WIDEN(__FILE__)

void handle_error(const wchar_t *dbdir, const wchar_t *procid, const char *errmsg) {
	size_t dblen, idlen, pathlen;
	bool addsep = false;
	wchar_t *path, *s;
	HANDLE f;

	pathlen = dblen = wcslen(dbdir);
	if (dbdir[dblen - 1] != L'\\') {
		addsep = true;
		pathlen++;
	}
	idlen = wcslen(procid);
	pathlen += idlen;
	pathlen += 7;

	s = path = alloca(pathlen * sizeof(wchar_t));
	wcsncpy(s, dbdir, dblen);
	s += dblen;
	if (addsep) {
		*s++ = L'\\';
	}
	wcsncpy(s, procid, idlen);
	s += idlen;
	wcsncpy(s, L"-E.msg", 7);

	if (errmsg != NULL && errmsg[0] != '\0') {
		f = CreateFile(path, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
		if (f != INVALID_HANDLE_VALUE) {
			DWORD nwritten = 0;
			WriteFile(f, errmsg, strlen(errmsg), &nwritten, NULL);
			CloseHandle(f);
		}
		else {
//			log(ERROR, L"Cannot create file `%s'", path);
		}
	}
	path[pathlen - 4] = L'\0';
	f = CreateFile(path, 0, FILE_SHARE_DELETE, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
	if (f != INVALID_HANDLE_VALUE) {
		CloseHandle(f);
	}
	else {
//		log(ERROR, L"Cannot create file `%s'", path);
	}
}

int wmain(int argc, const wchar_t **argv)
{
	options_t opts;
	int exit_code = EXIT_FAILURE;
	HANDLE h_event = NULL;
	HANDLE h_wait[2] = { NULL, NULL };
	int r;
	database_t db;
	process_t *process = NULL;
	error_t error;

	error_init(&error);

	if (!parse_args(&opts, argc, argv, &error)) {
		goto cleanup;
	}
	
	if (db_open(&db, opts.datadir, &error) != 0) {
		goto cleanup;
	}
	
	if (process_launch(&opts, &process, &error) != 0) {
		goto cleanup;
	}
	db_update(&db, process, &error);

	h_wait[0] = process->handle;
	h_event = CreateEvent(NULL, FALSE, FALSE, process->id);
	if (h_event == INVALID_HANDLE_VALUE) {
		goto cleanup;
	}
	h_wait[1] = h_event;

	r = WaitForMultipleObjects(2, h_wait, FALSE, INFINITE);
	if (r != WAIT_FAILED) {
		int i = r - WAIT_OBJECT_0;
		if (i == 1) {
			// cancel process
			process_cancel(process);
			process->status = cancelled;
			WaitForSingleObject(process->handle, 10000);
		}
		else {
			process->status = finished;
		}
		db_update(&db, process, &error);
		exit_code = EXIT_SUCCESS;
	}
cleanup:
	if (error.message != NULL) {
		db_error(&db, process, &error);
	}
	if (process != NULL) {
		process_free(process);
	}
	if (h_event != NULL) {
		CloseHandle(h_event);
	}

	return exit_code;
}
