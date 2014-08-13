#include "process.h"
#include "win32.h"
#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include <windows.h>
//#include <wincrypt.h>
/*
#include <io.h>
#include <fcntl.h>
*/

#define FREE_IF_NOT_NULL(x) if ((x) != NULL) { free((void*) (x)); }

typedef enum io_mode {
	in, out
}
io_mode_t;

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
	log_message(L"Job termination failed with error %d", GetLastError());
	return -1;
}

static
int GetActiveProcessCount(HANDLE job) {
	JOBOBJECT_BASIC_ACCOUNTING_INFORMATION basicAccountingInfo;
	if (!QueryInformationJobObject(job, JobObjectBasicAccountingInformation, &basicAccountingInfo, sizeof(basicAccountingInfo), NULL)) {
		return -1;
	}
	return basicAccountingInfo.ActiveProcesses;
}

int process_wait(process_t *proc)
{
	for (int i = 0; i < 60 * 15; i++) {
		int processCount = GetActiveProcessCount(proc->job);
		if (processCount == -1) {
			log_message(L"Cannot determine number of active processes associated with job");
			return -1;
		}
		if (processCount == 0) {
			return 0;
		}
		Sleep(1000);
	}
	log_message(L"Process did not die after being terminated");
	return -1;
	/*
	if (!QueryInformationJobObject(proc->job, JobObjectBasicAccountingInformation, &basicAccountingInfo, sizeof(basicAccountingInfo), NULL)) {
		return -1;
	}
	if (basicAccountingInfo.ActiveProcesses == 0) {
		return 0;
	}
	
	completionPortHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1);
	if (proc->completionPortHandle == NULL) {
		return -1;
	}
	completionPort.CompletionKey = proc->job;
	completionPort.CompletionPort = completionPortHandle;
	if (!SetInformationJobObject(proc->job,
			JobObjectAssociateCompletionPortInformation,
			&completionPort, sizeof(completionPort))) {
		DWORD e = GetLastError();
		CloseHandle(completionPortHandle);
		return -1;
	}
	if (!QueryInformationJobObject(proc->job, JobObjectBasicAccountingInformation, &basicAccountingInfo, sizeof(basicAccountingInfo), NULL)) {
		CloseHandle(completionPortHandle);
		return -1;
	}
	if (basicAccountingInfo.ActiveProcesses == 0) {
		CloseHandle(completionPortHandle);
		return 0;
	}
	while (true) {
		if (!GetQueuedCompletionStatus(proc->completionPortHandle, &completionCode,
		          (PULONG_PTR) &completionKey, &ovl, INFINITE)) {
			CloseHandle(completionPortHandle);
			return -1;
		}
		if (completionKey == proc->job && completionCode == JOB_OBJECT_MSG_ACTIVE_PROCESS_ZERO) {
			return 0;
		}
	}
	*/
}

/*
static
int create_completion_port(process_t *proc)
{
	JOBOBJECT_ASSOCIATE_COMPLETION_PORT completionPort;
	proc->completionPortHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1);
	if (proc->completionPortHandle == NULL) {
		return -1;
	}
	completionPort.CompletionKey = proc->job;
	completionPort.CompletionPort = proc->completionPortHandle;
	if (!SetInformationJobObject(proc->job,
			JobObjectAssociateCompletionPortInformation,
			&completionPort, sizeof(completionPort))) {
		CloseHandle(proc->completionPortHandle);
		proc->completionPortHandle = NULL;
		return -1;
	}
	return 0;
}
*/

/*
static wchar_t *build_cmdline(int argc, wchar_t **argv)
{
	int len = 1;
	bool *quote;
	wchar_t *cmdline, *s;


	wcscspn()

	quote = _alloca(sizeof(bool) * argc);
	for (int i = 0; i < argc; i++) {
		quote[i] = false;
		size_t arglen = wcslen(argv[i]);
		len += arglen;
		for (size_t j = 0; j < arglen; j++) {
			if (iswspace(argv[i][j])) {
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
	cmdline = (wchar_t*) malloc(len * sizeof(wchar_t));
	if (cmdline == NULL) {
		return NULL;
	}
	s = cmdline;
	for (int i = 0; i < argc; i++) {
		int n = wcslen(argv[i]);
		if (i > 0) {
			*s++ = L' ';
		}
		if (quote[i]) {
			*s++ = L'"';
		}
		wcscpy(s, argv[i]);
		s += n;
		if (quote[i]) {
			*s++ = L'"';
		}
	}
	*s = L'\0';
	return cmdline;
}
*/

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
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (h == INVALID_HANDLE_VALUE) {
		log_message(L"Failed to open file for I/O redirection: %s", path);
		return INVALID_HANDLE_VALUE;
	}
	SetHandleInformation(h, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
	return h;
}


int launch_request_new(const wchar_t *id,
	const wchar_t *dbpath,
	const wchar_t *workdir,
	const wchar_t *outpath,
	const wchar_t *errpath,
	launch_request_t **request)
{
	*request = malloc(sizeof(**request));
	if (*request == NULL) {
		log_message(L"Out of memory");
		return -1;
	}
	memset(*request, 0, sizeof(**request));
	if (((*request)->id = _wcsdup(id)) == NULL) {
		log_message(L"Out of memory");
		launch_request_free(*request);
		log_message(L"Out of memory");
		return -1;
	}
	if (((*request)->dbpath = _wcsdup(dbpath)) == NULL) {
		launch_request_free(*request);
		log_message(L"Out of memory");
		return -1;
	}
	if (workdir != NULL && ((*request)->workdir = _wcsdup(workdir)) == NULL) {
		launch_request_free(*request);
		log_message(L"Out of memory");
		return -1;
	}
	if (((*request)->outpath = _wcsdup(outpath != NULL ? outpath : L"NUL")) == NULL) {
		launch_request_free(*request);
		log_message(L"Out of memory");
		return -1;
	}
	if (((*request)->errpath = _wcsdup(errpath != NULL ? errpath : L"NUL")) == NULL) {
		launch_request_free(*request);
		log_message(L"Out of memory");
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


int process_new(const wchar_t *id, size_t idlen, process_t **process) {
	wchar_t *procid;
	*process = (process_t*) malloc(sizeof(process_t));
	if (*process == NULL) {
		return -1;
	}
	memset(*process, 0, sizeof(**process));
	procid = (wchar_t*) malloc((idlen + 1) * sizeof(wchar_t));
	if (procid == NULL) {
		free(*process);
		*process = NULL;
		return -1;
	}
	wcsncpy(procid, id, idlen);
	procid[idlen] = L'\0';
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

int process_launch(const launch_request_t *request, int argc, wchar_t **argv, process_t **process)
{
	wchar_t *cmdline = NULL;
	int retval = -1;
	STARTUPINFO sinfo;
	PROCESS_INFORMATION pinfo;
	JOBOBJECT_EXTENDED_LIMIT_INFORMATION limit;

	memset(&sinfo, 0, sizeof(sinfo));
	memset(&pinfo, 0, sizeof(pinfo));

	assert(request->id != NULL);
	assert(request->dbpath != NULL);

	cmdline = BuildCommandLine(argc, argv);
	if (cmdline == NULL) {
		goto cleanup;
	}

	sinfo.cb = sizeof(sinfo);
	sinfo.dwFlags = STARTF_USESTDHANDLES;

	sinfo.hStdInput = create_std_handle(STD_INPUT_HANDLE, L"NUL", in);
	sinfo.hStdOutput = create_std_handle(STD_OUTPUT_HANDLE, request->outpath, out);
	sinfo.hStdError = create_std_handle(STD_ERROR_HANDLE, request->errpath, out);

	if (sinfo.hStdInput == INVALID_HANDLE_VALUE || sinfo.hStdOutput == INVALID_HANDLE_VALUE || sinfo.hStdError == INVALID_HANDLE_VALUE) {
		goto cleanup;
	}

	*process = malloc(sizeof(**process));
	if (*process == NULL) {
		log_message(L"Out of memory");
		goto cleanup;
	}
	memset(*process, 0, sizeof(**process));
	if (((*process)->id = _wcsdup(request->id)) == NULL) {
		log_message(L"Out of memory");
		goto cleanup;
	}
	if (!CreateProcess(argv[0], cmdline, NULL, NULL, true,
			CREATE_NO_WINDOW | CREATE_NEW_PROCESS_GROUP | CREATE_SUSPENDED | CREATE_BREAKAWAY_FROM_JOB,
			NULL, request->workdir, &sinfo, &pinfo)) {
		log_message(L"Failed to spawn process");
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
		log_message(L"Failed to create job and process to it");
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
			}
			if ((*process)->job != NULL) {
				CloseHandle((*process)->job);
			}
			FREE_IF_NOT_NULL((*process)->id);
			free(*process);
			*process = NULL;
		}
	}
	return retval;
}
