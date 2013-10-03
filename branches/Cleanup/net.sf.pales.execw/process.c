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

static int build_cmdline(char *cmdline, int clen, int argc, char **argv)
{
	int len = 1;
	bool quote[argc];
	char *s;

	for (int i = 0; i < argc; i++) {
		quote[i] = false;
		len += strlen(argv[i]);
		for (int j = 0; j < strlen(argv[i]); j++) {
			if (isspace(argv[i][j])) {
				quote[i] = true;
				break;
			}
		}
		if (quote[i]) {
			len += 2;
		}
		if (i > 0) {
			len++;
		}
	}
	if (len >= clen) {
		return -1;
	}
	s = cmdline;
	for (int i = 0; i < argc; i++) {
		int n = strlen(argv[i]);
		if (i > 0) {
			*s++ = ' ';
		}
		if (quote[i]) {
			*s++ = '"';
		}
		strcpy(s, argv[i]);
		s += n;
		if (quote[i]) {
			*s++ = '"';
		}
	}
	*s = '\0';
	return 0;
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

static HANDLE create_std_handle(DWORD std_handle, const char *path, io_mode_t mode)
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


int launch_request_new(const char *id,
	const char *dbpath,
	const char *workdir,
	const char *outpath,
	const char *errpath,
	launch_request_t *request)
{
	memset(*request, 0, sizeof(*request));

	if (strlcpy(request->id, id, sizeof(request->id) >= sizeof(request->id))) {
		return -1;
	}
	if (strlcpy(request->dbpath, id, sizeof(request->dbpath) >= sizeof(request->dbpath))) {
		return -1;
	}

	if (workdir != NULL && strlcpy(request->workdir, workdir, sizeof(request->workdir) >= sizeof(request->workdir))) {
		return -1;
	}
	if (strlcpy(request->outpath, outpath != NULL ? outpath : "NUL", sizeof(request->outpath)) >= sizeof(request->outpath)) {
		return -1;
	}
	if (strlcpy(request->errpath, errpath != NULL ? errpath : "NUL", sizeof(request->errpath)) >= sizeof(request->errpath)) {
		return -1;
	}
	return 0;
}

int process_new(const char *id, size_t idlen, process_t **process) {
	char *procid;
	*process = (process_t*) malloc(sizeof(process_t));
	if (*process == NULL) {
		return -1;
	}
	memset(*process, 0, sizeof(**process));
	procid = (char*) malloc(idlen + 1);
	if (procid == NULL) {
		free(*process);
		*process = NULL;
		return -1;
	}
	strncpy(procid, id, idlen);
	procid[idlen] = '\0';
	(*process)->id = procid;
	return 0;
}

void process_free(process_t *p) {
	FREE_IF_NOT_NULL(p->id);
	if (p->handle != NULL) {
		CloseHandle(p->handle);
	}
	if (p->job != NULL) {
		CloseHandle(p->job);
	}
	free(p);
}

int process_launch(const launch_request_t *request, int argc, char **argv, process_t **process)
{
	char cmdline[32768];
	int retval = -1;
	STARTUPINFO sinfo;
	PROCESS_INFORMATION pinfo;
	JOBOBJECT_EXTENDED_LIMIT_INFORMATION limit;

	memset(&sinfo, 0, sizeof(sinfo));
	memset(&pinfo, 0, sizeof(pinfo));

	if (request->id == NULL || request->dbpath == NULL) {
		goto cleanup;
	}

	if (build_cmdline(cmdline, argc, argv) != 0) {
		goto cleanup;
	}

	sinfo.cb = sizeof(sinfo);
	sinfo.dwFlags = STARTF_USESTDHANDLES;

	sinfo.hStdInput = create_std_handle(STD_INPUT_HANDLE, "NUL", in);
	sinfo.hStdOutput = create_std_handle(STD_OUTPUT_HANDLE, request->outpath, out);
	sinfo.hStdError = create_std_handle(STD_ERROR_HANDLE, request->errpath, out);

	if (sinfo.hStdInput == INVALID_HANDLE_VALUE || sinfo.hStdOutput == INVALID_HANDLE_VALUE || sinfo.hStdError == INVALID_HANDLE_VALUE) {
		goto cleanup;
	}

	*process = malloc(sizeof(**process));
	if (*process == NULL) {
		goto cleanup;
	}
	memset(*process, 0, sizeof(**process));
	if (((*process)->id = strdup(request->id)) == NULL) {
		goto cleanup;
	}
	if (!CreateProcess(argv[0], cmdline, NULL, NULL, true,
			CREATE_NO_WINDOW | CREATE_NEW_PROCESS_GROUP | CREATE_SUSPENDED | CREATE_BREAKAWAY_FROM_JOB,
			NULL, request->workdir, &sinfo, &pinfo)) {
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
			// FREE_IF_NOT_NULL((*process)->id);
			// free(*process);
			// *process = NULL;
			(*process)->status = error;
		}
	}
	return retval;
}
