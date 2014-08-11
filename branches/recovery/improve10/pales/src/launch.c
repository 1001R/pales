#define _CRT_RAND_S

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <windows.h>
#include "options.h"
#include "process.h"
#include "database.h"


int wmain(int argc, wchar_t **argv)
{
	int optind;
	options_t opts;
	int exit_code = EXIT_FAILURE;
	HANDLE h_event = INVALID_HANDLE_VALUE;
	HANDLE h_wait[2];
	int r;
	database_t db;
	launch_request_t *request = NULL;
	process_t *process = NULL;


	if (parse_args(&opts, argc, argv, &optind) != 0) {
		goto cleanup;
	}
	
	if (launch_request_new(opts.procid, opts.datadir, opts.workdir, opts.outfile, opts.errfile, &request) != 0) {
		goto cleanup;
	}

	if (db_open(&db, opts.datadir) != 0) {
		goto cleanup;
	}
	
	if (process_launch(request, argc - optind, argv + optind, &process) != 0) {
		goto cleanup;
	}
	db_update(&db, process);

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
			if (!GetExitCodeProcess(process->handle, &(process->exitcode))) {
				process->exitcode = 0;
			}
			process_wait(process);
			process->status = finished;
		}
		db_update(&db, process);
		exit_code = EXIT_SUCCESS;
	}
cleanup:
	if (process != NULL) {
		process_free(process);
	}
	if (h_event != NULL) {
		CloseHandle(h_event);
	}
	return exit_code;
}
