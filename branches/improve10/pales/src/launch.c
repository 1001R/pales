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
	launch_request_t *request = NULL;
	process_t *process = NULL;

	if (parse_args(&opts, argc, argv, &optind) != 0) {
		goto cleanup;
	}
	
	if (db_open(opts.datadir, opts.procid) != 0) {
		goto cleanup;
	}

	if (launch_request_new(opts.procid, opts.datadir, opts.workdir, opts.outfile, opts.errfile, &request) != 0) {
		goto cleanup;
	}

	h_event = CreateEvent(NULL, FALSE, FALSE, opts.procid);
	if (h_event == INVALID_HANDLE_VALUE) {
		db_update(error, L"Cannot create event object for cancellation");
		goto cleanup;
	}

	if (process_launch(request, argc - optind, argv + optind, &process) != 0) {
		goto cleanup;
	}
	if (db_update(process->status, process->pid) != 0) {
		goto cleanup;
	}

	h_wait[0] = process->handle;
	h_wait[1] = h_event;

	r = WaitForMultipleObjects(2, h_wait, FALSE, INFINITE);
	if (r != WAIT_FAILED) {
		int i = r - WAIT_OBJECT_0;
		if (i == 1) {
			// cancel process
			if (process_cancel(process) != 0) {
				goto cleanup;
			}
			db_update(cancelled);
			WaitForSingleObject(process->handle, 10000);
		}
		else {
			if (!GetExitCodeProcess(process->handle, &(process->exitcode))) {
				process->exitcode = 0;
			}
			if (process_wait(process) != 0) {
				goto cleanup;
			}
			db_update(finished, process->exitcode);
		}
		exit_code = EXIT_SUCCESS;
	}
	else {
		db_update(error, L"System call WaitForMultipleObjects failed with error %d", GetLastError());
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
