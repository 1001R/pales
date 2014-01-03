#include "process.h"
#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <windows.h>
#include <wincrypt.h>

#define FREE_IF_NOT_NULL(x) if ((x) != NULL) { free((void*) (x)); }

typedef enum io_mode {
	in, out
}
io_mode_t;

extern char *strdup(const char *);

int process_cancel(process_t *proc)
{
	if (TerminateJobObject(proc->job, 1)) {
		CloseHandle(proc->job);
		CloseHandle(proc->handle);
		proc->job = NULL;
		proc->handle = NULL;
		proc->pid = 0;
		return 0;
	}
	return -1;
}


static bool quot_required(const wchar_t *arg, size_t *quotsz)
{
	bool retval = false;
	*quotsz = 0;
	for (const wchar_t *s = arg; *s != L'\0'; s++) {
		if (!retval) {
			if (iswspace(*s) || *s == L'"') {
				retval = true;
				quotsz += 2;
			}
		}
		(*quotsz)++;
		if (*s == L'"') {
			(*quotsz)++;
		}
	}
	return retval;
}

static wchar_t *string_copy(wchar_t *dst, const wchar_t *src, bool quote)
{
	if (quote) {
		*dst++ = L'"';
	}
	for (const wchar_t *s = src; *s != L'\0'; s++) {
		if (quote && *s == L'"') {
			*dst++ = L'\\';
		}
		*dst++ = *s;
	}
	if (quote) {
		*dst++ = L'"';
	}
	return dst;
}

static wchar_t *build_cmdline(int argc, const wchar_t **argv)
{
	bool quote[argc];
	size_t len = 1;
	wchar_t *cmdline, *s;

	for (int i = 0; i < argc; i++) {
		size_t arglen;
		quote[i] = quot_required(argv[i], &arglen);
		len += arglen;
		if (i > 0) {
			len++;
		}
	}
	cmdline = (wchar_t *)malloc(len * sizeof(wchar_t));
	if (cmdline == NULL) {
		return NULL;
	}
	s = cmdline;
	for (int i = 0; i < argc; i++) {
		if (i > 0) {
			*s++ = ' ';
		}
		s = string_copy(s, argv[i], quote[i]);
	}
	*s = L'\0';
	return cmdline;
}

/*
static int create_id(uint64_t *id)
{
	HCRYPTPROV provider;
	int success;

	if (!CryptAcquireContext(&provider, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {   
 		return -1; 
    }
	success = CryptGenRandom(provider, sizeof(uint64_t), (BYTE *)id);
	if (!CryptReleaseContext(provider, 0)) {
		return -1;
	}
 	return success ? 0 : -1;
}
*/

static HANDLE create_std_handle(DWORD std_handle, const wchar_t *path, io_mode_t mode)
{
	HANDLE h = GetStdHandle(std_handle);
	if (h != INVALID_HANDLE_VALUE) {
		DWORD info;
		if (!GetHandleInformation(h, &info)) {
			return INVALID_HANDLE_VALUE;
		}
		if ((info & HANDLE_FLAG_INHERIT) != 0) {
			SetHandleInformation(h, HANDLE_FLAG_INHERIT, 0);
		}
	}
	h = CreateFile(path, 
		mode == out ? GENERIC_WRITE : GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (h == INVALID_HANDLE_VALUE) {
		return INVALID_HANDLE_VALUE;
	}
	SetHandleInformation(h, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
	return h;
}

/*
int launch_request_new(const wchar_t *id,
	const wchar_t *dbpath,
	const wchar_t *workdir,
	const wchar_t *outpath,
	const wchar_t *errpath,
	launch_request_t **request)
{
	*request = malloc(sizeof(**request));
	if (*request == NULL) {
		return -1;
	}
	memset(*request, 0, sizeof(**request));
	if (((*request)->id = strdup(id)) == NULL) {
		launch_request_free(*request);
		return -1;
	}
	if (((*request)->dbpath = strdup(dbpath)) == NULL) {
		launch_request_free(*request);
		return -1;
	}
	if (workdir != NULL && ((*request)->workdir = strdup(workdir)) == NULL) {
		launch_request_free(*request);
		return -1;
	}
	if (((*request)->outpath = strdup(outpath != NULL ? outpath : "NUL")) == NULL) {
		launch_request_free(*request);
		return -1;
	}
	if (((*request)->errpath = strdup(errpath != NULL ? errpath : "NUL")) == NULL) {
		launch_request_free(*request);
		return -1;
	}
	return 0;
}


void launch_request_free(launch_request_t *request)
{
	FREE_IF_NOT_NULL(request->id);
	FREE_IF_NOT_NULL(request->dbpath);
	FREE_IF_NOT_NULL(request->workdir);
	FREE_IF_NOT_NULL(request->outpath);
	FREE_IF_NOT_NULL(request->errpath);
	FREE_IF_NOT_NULL(request);
}
*/


void process_free(process_t *p) {
	if (p->handle != NULL) {
		CloseHandle(p->handle);
	}
	if (p->job != NULL) {
		CloseHandle(p->job);
	}
	free(p);
}

int process_launch(const options_t *opts, process_t **process, error_t *err)
{
	wchar_t *cmdline = NULL;
	int retval = -1;
	STARTUPINFO sinfo;
	PROCESS_INFORMATION pinfo;
	JOBOBJECT_EXTENDED_LIMIT_INFORMATION limit;

	memset(&sinfo, 0, sizeof(sinfo));
	memset(&pinfo, 0, sizeof(pinfo));

	cmdline = build_cmdline(opts->argc, opts->argv);
	if (cmdline == NULL) {
		goto cleanup;
	}

	sinfo.cb = sizeof(sinfo);
	sinfo.dwFlags = STARTF_USESTDHANDLES;

	sinfo.hStdInput = create_std_handle(STD_INPUT_HANDLE, L"NUL", in);
	sinfo.hStdOutput = create_std_handle(STD_OUTPUT_HANDLE, opts->outfile, out);
	sinfo.hStdError = create_std_handle(STD_ERROR_HANDLE, opts->errfile, out);

	if (sinfo.hStdInput == INVALID_HANDLE_VALUE || sinfo.hStdOutput == INVALID_HANDLE_VALUE || sinfo.hStdError == INVALID_HANDLE_VALUE) {
		goto cleanup;
	}

	*process = malloc(sizeof(**process));
	if (*process == NULL) {
		goto cleanup;
	}
	memset(*process, 0, sizeof(**process));
	(*process)->id = opts->procid;
	if (!CreateProcess(opts->argv[0], cmdline, NULL, NULL, true,
			CREATE_NO_WINDOW | CREATE_NEW_PROCESS_GROUP | CREATE_SUSPENDED | CREATE_BREAKAWAY_FROM_JOB,
			NULL, opts->workdir, &sinfo, &pinfo)) {
		(*process)->status = error;
		error_apicall_fail(err, L"CreateProcess");
		error_set_message(err, L"Cannot create process");
		goto cleanup;
	}
	(*process)->pid = pinfo.dwProcessId;
	(*process)->handle = pinfo.hProcess;
	(*process)->status = running;

	memset(&limit, 0, sizeof(limit));
	if (((*process)->job = CreateJobObject(NULL, NULL)) == NULL ||
			!SetInformationJobObject((*process)->job, JobObjectExtendedLimitInformation, &limit, sizeof(limit)) ||
			!AssignProcessToJobObject((*process)->job, pinfo.hProcess) ||
			!ResumeThread(pinfo.hThread)) {
		goto cleanup;
	}
	retval = 0;
cleanup:
	FREE_IF_NOT_NULL(cmdline);
	if (sinfo.hStdInput != INVALID_HANDLE_VALUE) {
		CloseHandle(sinfo.hStdInput);
	}
	if (sinfo.hStdOutput != INVALID_HANDLE_VALUE) {
		CloseHandle(sinfo.hStdOutput);
	}
	if (sinfo.hStdError != INVALID_HANDLE_VALUE) {
		CloseHandle(sinfo.hStdError);
	}
	if (pinfo.hThread != NULL) {
		CloseHandle(pinfo.hThread);
	}

	if (retval != 0) {
		if (*process != NULL) {
			if ((*process)->handle != NULL) {
				TerminateProcess((*process)->handle, 1);
				CloseHandle((*process)->handle);
				(*process)->handle = NULL;
			}
			if ((*process)->job != NULL) {
				CloseHandle((*process)->job);
				(*process)->job = NULL;
			}
//			free(*process);
//			*process = NULL;
		}
	}
	return retval;
}
